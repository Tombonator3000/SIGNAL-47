#include "S47CalicheAccess.h"
#include <limits>
#include "S47Main.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Inter.h"
#include "S47Three.h"
#include "S47Audio.h"
#include "S47Props.h"
#include "S47UI.h"
#include "S47RoadTerrain.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace S47CalicheAccess
{
namespace
{
 TWeakObjectPtr<US47Main> Main;
 struct FRoom {FString Id,Use;FVector Root,Hinge;double X0,X1,Y0,Y1,Floor,Front,DoorY,Width,Height,StepGround=0;int32 Steps=1;bool bOpen=true;TWeakObjectPtr<UStaticMeshComponent> Leaf;};
 TArray<FRoom> Rooms;
 TArray<TWeakObjectPtr<UPrimitiveComponent>> Retired;
 TArray<TWeakObjectPtr<UPointLightComponent>> Lamps;
 FVector Vec(const TArray<TSharedPtr<FJsonValue>>& A){return FVector(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber());}
 FS47Rect Rect(double X0,double X1,double Z0,double Z1,const FString& Id){FS47Rect R;R.MinX=FMath::Min(X0,X1);R.MaxX=FMath::Max(X0,X1);R.MinZ=FMath::Min(Z0,Z1);R.MaxZ=FMath::Max(Z0,Z1);R.Id=Id;return R;}
 void Retire(UPrimitiveComponent* C){if(C){C->SetHiddenInGame(true,false);C->SetVisibility(false,false);C->SetCastShadow(false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);}}
 FRoom* Find(const FString& Id){return Rooms.FindByPredicate([&](const FRoom& R){return R.Id==Id||R.Use==Id||TEXT("town:")+R.Use==Id||TEXT("access:")+R.Id==Id;});}
 void Sync(FRoom& R)
 {
  if(auto* C=R.Leaf.Get())
  {
   const double A=R.bOpen?1.67*R.Front:0.;const FVector Closed=R.Root+FVector(R.Hinge.X,0,-R.Hinge.Y);
   C->SetWorldLocation(S47Three::ToUE(Closed));C->SetWorldRotation(FRotator(0,-FMath::RadiansToDegrees(A),0));
  }
  if(!Main.IsValid()||!Main->Web)return;
  for(const TCHAR* Area:{TEXT("road"),TEXT("diner")})for(auto& C:Main->Web->CollidersMutable(Area))if(C.Id==TEXT("access:door:")+R.Id)C.bEnabled=!R.bOpen;
 }
}
bool HasDoor(const FString& Id){return Find(Id)!=nullptr;}
bool IsOpen(const FString& Id){const auto* R=Find(Id);return R&&R->bOpen;}
bool UseDoor(const FString& Id)
{
 FRoom* R=Find(Id);if(!R)return false;R->bOpen=!R->bOpen;Sync(*R);if(Main.IsValid())Main->RequestAutosave(false);
 if(Main.IsValid()&&Main->Audio)Main->Audio->Sfx(Main->Audio->Or(TEXT("door_latch"),TEXT("switch")),FS47Sound());
 return true;
}
void Init(US47Main* M)
{
 Main=M;Rooms.Reset();Retired.Reset();Lamps.Reset();if(!M||!M->Web||!M->Inter)return;
 AS47Web* W=M->Web;FString Text;TSharedPtr<FJsonObject> J;
 if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("CalicheAccess/assembly.json")))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),J)||!J)return;
 if(J->GetStringField(TEXT("status"))!=TEXT("NATIVE_READY"))return;
 const int32 Parent=W->Handle(TEXT("world.town.group"));const auto* PN=W->Node(Parent);if(!PN||!PN->Comp.IsValid())return;const TWeakObjectPtr<USceneComponent> ParentComp=PN->Comp;
 const auto Parts=J->GetObjectField(TEXT("parts"));TMap<FString,UStaticMesh*> Meshes;
 for(const auto& Pair:Parts->Values)
 {
  const auto R=Pair.Value->AsObject();auto* Mesh=LoadObject<UStaticMesh>(nullptr,*R->GetStringField(TEXT("asset")));
  if(!Mesh||Mesh->GetNumLODs()!=3)return;Meshes.Add(FString(Pair.Key.ToView()),Mesh);
 }
 for(const auto& V:J->GetArrayField(TEXT("buildings")))
 {
  const auto B=V->AsObject();FRoom R;R.Id=B->GetStringField(TEXT("id"));B->TryGetStringField(TEXT("interact"),R.Use);if(R.Use==TEXT("laundro"))R.Use=TEXT("laundromat");R.Root=Vec(B->GetArrayField(TEXT("root")));R.Hinge=Vec(B->GetArrayField(TEXT("hinge")));
  const auto& F=B->GetArrayField(TEXT("footprint"));R.X0=F[0]->AsNumber();R.X1=F[1]->AsNumber();R.Y0=F[2]->AsNumber();R.Y1=F[3]->AsNumber();R.Floor=B->GetNumberField(TEXT("floor"));R.Front=B->GetNumberField(TEXT("front"));
  const auto& D=B->GetArrayField(TEXT("door"));R.DoorY=D[1]->AsNumber();R.Width=D[2]->AsNumber();R.Height=D[3]->AsNumber();
  FString OldPrefix;B->TryGetStringField(TEXT("old_prefix"),OldPrefix);
  if(!OldPrefix.IsEmpty())for(int32 I=0;W->Node(I);++I)if(const auto* N=W->Node(I);N->Name.StartsWith(TEXT("town-production:")+OldPrefix))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get())){Retired.Add(C);Retire(C);}
  const TArray<TSharedPtr<FJsonValue>>* RetireProps=nullptr;
  if(B->TryGetArrayField(TEXT("retire_props"),RetireProps))for(const auto& P:*RetireProps)
   for(int32 I=0;W->Node(I);++I)if(const auto* N=W->Node(I);N->Name==P->AsString())if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get())){Retired.Add(C);Retire(C);}
  double Legacy=-1;if(B->TryGetNumberField(TEXT("legacy_door_holder"),Legacy))if(const auto* N=W->Node(int32(Legacy)))
   for(int32 I:N->Children)if(const auto* Child=W->Node(I))if(auto* C=Cast<UPrimitiveComponent>(Child->Comp.Get())){Retired.Add(C);Retire(C);}
  for(const auto& Pair:Parts->Values)
  {
   const auto P=Pair.Value->AsObject();if(P->GetStringField(TEXT("family"))!=R.Id)continue;
   const FString Name(Pair.Key.ToView());const bool Door=P->GetStringField(TEXT("role"))==TEXT("door");
   auto* C=NewObject<UStaticMeshComponent>(W);C->SetupAttachment(ParentComp.Get());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Meshes[Name]);
   const bool Glass=Name.EndsWith(TEXT("Glass"));C->SetCollisionEnabled(Glass?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,Glass?ECR_Ignore:ECR_Block);C->SetCastShadow(!Glass);if(Door)C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);C->RegisterComponent();C->SetWorldLocation(S47Three::ToUE(Vec(P->GetArrayField(TEXT("root")))));
   const int32 Node=W->AdoptNode(C,Parent,TEXT("caliche-access:")+Name);
   if(Door)
   {
    R.Leaf=C;FS47Interactable It;It.Id=TEXT("access:")+R.Id;It.Object=FS47Obj(W,Node);It.Range=2.7;const FString Id=R.Id;
    It.Label=[Id]{return IsOpen(Id)?FString(TEXT("Close door")):FString(TEXT("Open door"));};It.Use=[Id]{UseDoor(Id);};M->Inter->Add(It);
   }
  }
  // Replace the former solid building rectangle with actual perimeter walls and an opening.
  const FS47Rect Foot=Rect(R.Root.X+R.X0,R.Root.X+R.X1,R.Root.Z-R.Y1,R.Root.Z-R.Y0,TEXT("access:room:")+R.Id);
  const double FX=R.Root.X+(R.Front>0?R.X1:R.X0),DZ=R.Root.Z-R.DoorY;
  for(const TCHAR* Area:{TEXT("road"),TEXT("diner")})
  {
   auto& Cs=W->CollidersMutable(Area);
   for(auto& C:Cs)if(C.MinX>=Foot.MinX-.3&&C.MaxX<=Foot.MaxX+1.0&&C.MinZ>=Foot.MinZ-.3&&C.MaxZ<=Foot.MaxZ+.3&&C.MaxX-C.MinX>2&&C.MaxZ-C.MinZ>2){C.bEnabled=false;C.MinX=C.MinZ=1e9;C.MaxX=C.MaxZ=1e9+.01;}
   // Former solid door stoop/blockers must no longer seal the actual aperture.
   for(auto& C:Cs)if(C.MinX>FX-1.1&&C.MaxX<FX+3.5&&C.MinZ<DZ&&C.MaxZ>DZ&&C.MaxZ-C.MinZ<FMath::Max(R.Width+1.2,12.)){C.bEnabled=false;C.MinX=C.MinZ=1e9;C.MaxX=C.MaxZ=1e9+.01;}
   const FString Prefix=TEXT("access:wall:")+R.Id;const double Back=R.Front>0?Foot.MinX:Foot.MaxX;
   Cs.Add(Rect(Back-.12,Back+.12,Foot.MinZ,Foot.MaxZ,Prefix));Cs.Add(Rect(Foot.MinX,Foot.MaxX,Foot.MinZ-.12,Foot.MinZ+.12,Prefix));Cs.Add(Rect(Foot.MinX,Foot.MaxX,Foot.MaxZ-.12,Foot.MaxZ+.12,Prefix));
   Cs.Add(Rect(FX-.12,FX+.12,Foot.MinZ,DZ-R.Width/2,Prefix));Cs.Add(Rect(FX-.12,FX+.12,DZ+R.Width/2,Foot.MaxZ,Prefix));
   Cs.Add(Rect(FX-.11,FX+.11,DZ-R.Width/2+.04,DZ+R.Width/2-.04,TEXT("access:door:")+R.Id));
   for(const auto& CV:B->GetArrayField(TEXT("furniture"))){const auto& A=CV->AsArray();Cs.Add(Rect(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber(),A[3]->AsNumber(),TEXT("access:furniture:")+R.Id));}
   W->ZonesMutable(Area).Add(Foot);
   W->ZonesMutable(Area).Add(Rect(FX-1.5,FX+1.5,DZ-R.Width/2-.4,DZ+R.Width/2+.4,TEXT("access:entry:")+R.Id));
  }
  // Actual continuous entry apron/plinth; root and interior stay fixed to retained story anchors.
  auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));auto* Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
  if(Cube&&Mat)
  {
   const double Platform=R.Root.Y+R.Floor;
   const FVector O=W->Obj(TEXT("world.town.group")).WorldPosition();
   const double Ground=O.Y+S47RoadTerrain::SurfaceY(FX-O.X+R.Front*3.,DZ-O.Z);
   const double Low=FMath::Min(Ground-.3,Platform-.4),High=R.Root.Y+R.Floor-.015;
   R.StepGround=Ground;R.Steps=FMath::Clamp(FMath::CeilToInt(FMath::Abs(Platform-Ground)/.15),1,16);
   auto* C=NewObject<UStaticMeshComponent>(W);C->SetupAttachment(ParentComp.Get());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Cube);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();C->SetWorldLocation(S47Three::ToUE(FX+R.Front*.65,(Low+High)/2,DZ));C->SetWorldScale3D(FVector(1.5,R.Width+.7,High-Low));
   auto* Mid=UMaterialInstanceDynamic::Create(Mat,W);Mid->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(0x777366));Mid->SetScalarParameterValue(TEXT("Roughness"),.95f);C->SetMaterial(0,Mid);W->AdoptNode(C,Parent,TEXT("caliche-access:entry-plinth:")+R.Id);
   for(int32 K=1;K<=R.Steps;++K)
   {
    const double Top=FMath::Lerp(Platform,Ground,double(K)/R.Steps),Bottom=FMath::Min(Ground-.2,Top-.18),At=(R.Id==TEXT("Sheriff")?1.63:1.4)+.32*(K-.5);
    auto* Step=NewObject<UStaticMeshComponent>(W);Step->SetupAttachment(ParentComp.Get());Step->SetMobility(EComponentMobility::Movable);Step->SetStaticMesh(Cube);Step->SetMaterial(0,Mid);Step->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Step->SetCollisionResponseToAllChannels(ECR_Ignore);Step->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Step->RegisterComponent();Step->SetWorldLocation(S47Three::ToUE(FX+R.Front*At,(Top+Bottom)/2,DZ));Step->SetWorldScale3D(FVector(.32,R.Width+.7,Top-Bottom));W->AdoptNode(Step,Parent,TEXT("caliche-access:entry-step:")+R.Id);
   }
  }
  auto* L=NewObject<UPointLightComponent>(W);L->SetupAttachment(ParentComp.Get());L->SetMobility(EComponentMobility::Movable);L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(650);L->SetLightColor(FLinearColor(1,.84,.63));L->SetAttenuationRadius(800);L->SetCastShadows(true);L->RegisterComponent();L->SetWorldLocation(S47Three::ToUE(R.Root+FVector((R.X0+R.X1)/2,2.6,-(R.Y0+R.Y1)/2)));Lamps.Add(L);
  Rooms.Add(R);Sync(Rooms.Last());
 }
 for(int32 I=0;W->Node(I);++I)if(const auto* N=W->Node(I);N->Name==TEXT("prop:radioStationDoor"))
 {
  FS47Interactable It;It.Id=TEXT("access:radioLobbyDoor");It.Object=FS47Obj(W,I);It.Range=2.7;
  It.Label=[]{return AS47Props::IsDoorOpen(TEXT("radioLobbyDoor"))?FString(TEXT("Close studio lobby door")):FString(TEXT("Open studio lobby door"));};It.Use=[]{AS47Props::UseDoor(TEXT("radioLobbyDoor"));};M->Inter->Add(It);
 }
 for(const TCHAR* Door:{TEXT("gasDoor"),TEXT("radioLobbyDoor")})if(!AS47Props::IsDoorOpen(Door))AS47Props::UseDoor(Door);
 UE_LOG(LogTemp,Display,TEXT("S47: Caliche playtest access ready, %d physical rooms and movable entry doors"),Rooms.Num());
}
void Update(){for(const auto& C:Retired)Retire(C.Get());}
void Save(const S47J::FObj& State)
{
 if(!State)return;auto Doors=S47J::New();for(const FRoom& R:Rooms)S47J::Set(Doors,*R.Id,R.bOpen);for(const TCHAR* Door:{TEXT("gasDoor"),TEXT("radioLobbyDoor")})S47J::Set(Doors,Door,AS47Props::IsDoorOpen(Door));S47J::Set(State,TEXT("calicheAccessDoors"),Doors);
}
void Restore(const S47J::FObj& State)
{
 const auto Doors=S47J::Obj(State,TEXT("calicheAccessDoors"));for(FRoom& R:Rooms){R.bOpen=S47J::Bool(Doors,*R.Id,true);Sync(R);}
 for(const TCHAR* Door:{TEXT("gasDoor"),TEXT("radioLobbyDoor")})if(AS47Props::IsDoorOpen(Door)!=S47J::Bool(Doors,Door,true))AS47Props::UseDoor(Door);
}

double FloorAt(double X,double Z)
{
 for(const FRoom& R:Rooms)
 {
  const double LX=X-R.Root.X,LY=R.Root.Z-Z;
  if(LX>=R.X0&&LX<=R.X1&&LY>=R.Y0&&LY<=R.Y1)return R.Root.Y+R.Floor;
  const double FX=R.Front>0?R.X1:R.X0,D=(LX-FX)*R.Front;
  const bool Apron=R.Id==TEXT("Sheriff")?LY>=R.Y0-.3&&LY<=R.Y1+.3:FMath::Abs(LY-R.DoorY)<R.Width/2+.35;
  const double Start=R.Id==TEXT("Sheriff")?1.63:1.4;
  if(D>=0&&D<=Start&&Apron)return R.Root.Y+R.Floor;
  if(R.Id==TEXT("Feed")&&D>=0&&D<=2.6&&LY>=R.Y0&&LY<=R.Y1)return R.Root.Y+R.Floor;
  if(D>Start&&D<=Start+.32*R.Steps&&FMath::Abs(LY-R.DoorY)<R.Width/2+.35)
  {
   const int32 K=FMath::Clamp(FMath::CeilToInt((D-Start)/.32),1,R.Steps);
   return FMath::Lerp(R.Root.Y+R.Floor,R.StepGround,double(K)/R.Steps);
  }
 }
 return std::numeric_limits<double>::quiet_NaN();
}
}
