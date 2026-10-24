// Native production architecture. Existing movement, media, people and story objects remain authoritative.
#include "S47TownBuildings.h"
#include "S47Web.h"
#include "S47Three.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshResources.h"
DEFINE_LOG_CATEGORY_STATIC(LogS47TownBuildings,Log,All);
namespace
{
 TSharedPtr<FJsonObject> Json(const TCHAR* Name)
 {
  FString S;TSharedPtr<FJsonObject> J;
  if(!FFileHelper::LoadFileToString(S,*(FPaths::ProjectContentDir()/TEXT("TownBuildings")/Name))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S),J))return {};
  return J;
 }
 FVector Vec(const TArray<TSharedPtr<FJsonValue>>& A){return A.Num()==3?FVector(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber()):FVector::ZeroVector;}
 int32 Tris(UStaticMesh* M){return M&&M->GetRenderData()&&M->GetRenderData()->LODResources.Num()?M->GetNumTriangles(0):-1;}
 void Retire(UStaticMeshComponent* C){C->SetVisibility(false,false);C->SetHiddenInGame(true,false);C->SetCastShadow(false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
}
TSharedPtr<FS47TownBuildings> FS47TownBuildings::Build(AS47Web* W)
{
 if(!W||FParse::Param(FCommandLine::Get(),TEXT("S47TownBuildingsOff"))||FParse::Param(FCommandLine::Get(),TEXT("S47PropsOff")))return {};
 const auto C=Json(TEXT("assembly.json")),P=Json(TEXT("town-patches.json")),Room=Json(TEXT("room-patches.json"));
 if(!C||!P||!Room){UE_LOG(LogS47TownBuildings,Warning,TEXT("S47: town architecture metadata missing; complete originals retained"));return {};}
 for(const auto& Pair:Room->GetObjectField(TEXT("meshes"))->Values)P->GetObjectField(TEXT("meshes"))->SetField(FString(Pair.Key.ToView()),Pair.Value);
 struct FPart {UStaticMesh* Mesh;int32 Parent;FVector At;FString Name,Family;};
 struct FPatch {UStaticMeshComponent* C;UStaticMesh* Mesh;int32 Node;TArray<int32> Slots;};
 TArray<FPart> Ready;TArray<FPatch> Cuts;TArray<int32> Hide;
 auto Reject=[](const FString& Why){UE_LOG(LogS47TownBuildings,Error,TEXT("S47: town architecture rejected: %s; complete originals retained"),*Why);};
 const auto& Parts=C->GetObjectField(TEXT("parts"))->Values;
 if(Parts.Num()!=28){Reject(TEXT("28-part contract"));return {};}
 for(const auto& Pair:Parts)
 {
  const auto R=Pair.Value->AsObject();const FString Name(Pair.Key.ToView());const FString Path=TEXT("/Game/TownBuildings/")+Name+TEXT(".")+Name;
  auto* M=LoadObject<UStaticMesh>(nullptr,*Path);const int32 Parent=W->Handle(R->GetStringField(TEXT("parent_handle")));
  if(!M||!W->Node(Parent)||!W->Node(Parent)->Comp.IsValid()||!M->GetRenderData()||M->GetRenderData()->LODResources.Num()!=3){Reject(Name+TEXT(" asset/parent/LODs"));return {};}
  const auto& Lods=R->GetArrayField(TEXT("lod_triangles"));
  for(int32 L=0;L<3;++L)if(M->GetNumTriangles(L)!=int32(Lods[L]->AsNumber())){Reject(Name+TEXT(" triangle contract"));return {};}
  const auto& B=R->GetArrayField(TEXT("bounds_local_m"));const FVector Lo=Vec(B[0]->AsArray()),Hi=Vec(B[1]->AsArray());
  const FVector Center((Lo.X+Hi.X)*50,-(Lo.Y+Hi.Y)*50,(Lo.Z+Hi.Z)*50);
  if(!M->GetBounds().Origin.Equals(Center,.3)||!M->GetBounds().BoxExtent.Equals((Hi-Lo)*50,.3)){Reject(Name+TEXT(" metric pivot"));return {};}
  for(const auto& Slot:M->GetStaticMaterials())if(!Slot.MaterialInterface||Slot.MaterialSlotName.ToString()!=Slot.MaterialInterface->GetName()||!Slot.MaterialInterface->GetName().StartsWith(TEXT("M_Town_"))){Reject(Name+TEXT(" materials"));return {};}
  Ready.Add({M,Parent,Vec(R->GetArrayField(TEXT("site_world_three_m"))),Name,R->GetStringField(TEXT("building"))});
 }
 for(const auto& Pair:P->GetObjectField(TEXT("meshes"))->Values)
 {
  const auto R=Pair.Value->AsObject();const FString Key(Pair.Key.ToView());const int32 I=R->GetIntegerField(TEXT("node"));const auto* N=W->Node(I);
  auto* Comp=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;const int32 Current=Comp?Tris(Comp->GetStaticMesh()):-1;
  bool Accepted=Current==R->GetIntegerField(TEXT("tris"));const TArray<TSharedPtr<FJsonValue>>* Counts=nullptr;
  if(R->TryGetArrayField(TEXT("acceptedPreviousTris"),Counts))for(const auto& V:*Counts)Accepted|=Current==int32(V->AsNumber());
  if(!Comp||!Accepted||N->Geo!=FCString::Atoi(*Key.Mid(1))||N->Parent!=R->GetIntegerField(TEXT("parent"))){Reject(Key+TEXT(" predecessor"));return {};}
  if(R->GetBoolField(TEXT("hidden"))){Hide.Add(I);continue;}
  auto* Mesh=LoadObject<UStaticMesh>(nullptr,*R->GetStringField(TEXT("asset")));
  if(Tris(Mesh)!=R->GetIntegerField(TEXT("resultTris"))){Reject(Key+TEXT(" trim asset"));return {};}
  TArray<int32> Slots;const TArray<TSharedPtr<FJsonValue>>* Map=nullptr;
  if(R->TryGetArrayField(TEXT("originalMaterialIndices"),Map))for(const auto& V:*Map)Slots.Add(int32(V->AsNumber()));
  if(Slots.Num()!=Mesh->GetStaticMaterials().Num()){Reject(Key+TEXT(" slot count"));return {};}
  for(int32 Slot:Slots)if(Slot<0||Slot>=Comp->GetNumMaterials()){Reject(Key+TEXT(" slot source"));return {};}
  Cuts.Add({Comp,Mesh,I,Slots});
 }
 for(const auto& V:C->GetArrayField(TEXT("retire")))
 {
  const auto R=V->AsObject();const int32 I=R->GetIntegerField(TEXT("node"));const auto* N=W->Node(I);
  auto* Comp=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
  bool Accepted=Comp&&Tris(Comp->GetStaticMesh())==R->GetIntegerField(TEXT("tris"));
  for(const auto& Count:R->GetArrayField(TEXT("acceptedPreviousTris")))Accepted|=Comp&&Tris(Comp->GetStaticMesh())==int32(Count->AsNumber());
  if(!Comp||!Accepted||N->Geo!=R->GetIntegerField(TEXT("geo"))||N->Parent!=R->GetIntegerField(TEXT("parent"))){Reject(FString::Printf(TEXT("retirement source node%d g%d current%d"),I,N?N->Geo:-1,Comp?Tris(Comp->GetStaticMesh()):-1));return {};}
  Hide.AddUnique(I);
 }
 auto F=MakeShared<FS47TownBuildings>();F->Web=W;
 for(const FPart& R:Ready)
 {
  auto* Comp=NewObject<UStaticMeshComponent>(W);Comp->SetupAttachment(W->Node(R.Parent)->Comp.Get());Comp->SetMobility(EComponentMobility::Movable);Comp->SetStaticMesh(R.Mesh);
  const bool Glass=R.Name.Contains(TEXT("Glass"));Comp->SetCollisionEnabled(Glass?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryOnly);
  Comp->SetCollisionResponseToAllChannels(ECR_Ignore);Comp->SetCollisionResponseToChannel(ECC_Visibility,Glass?ECR_Ignore:ECR_Block);Comp->SetCastShadow(!Glass);Comp->RegisterComponent();
  Comp->SetWorldLocation(S47Three::ToUE(R.At));Comp->SetWorldRotation(FRotator::ZeroRotator);
  const int32 I=W->AdoptNode(Comp,R.Parent,TEXT("town-production:")+R.Name);F->Parts.Add(Comp);F->Nodes.Add(I);
  for(int32 K=0;K<Comp->GetNumMaterials();++K)
  {
   const FString Name=Comp->GetMaterial(K)->GetName();if(!Name.EndsWith(TEXT("Glow")))continue;
   if(auto* Mid=Comp->CreateDynamicMaterialInstance(K)){const FLinearColor Base=Mid->K2_GetVectorParameterValue(TEXT("EmissiveFactor"));F->Glows.Add({Mid,R.Family,Base});}
  }
 }
 // Commit only after every new part and every predecessor was validated.
 for(const FPatch& Cut:Cuts)
 {
  TArray<UMaterialInterface*> Mats;for(auto* M:Cut.C->GetMaterials())Mats.Add(M);Cut.C->SetStaticMesh(Cut.Mesh);
  for(int32 K=0;K<Cut.C->GetNumMaterials();++K){const int32 From=Cut.Slots.IsValidIndex(K)?Cut.Slots[K]:K;if(Mats.IsValidIndex(From))Cut.C->SetMaterial(K,Mats[From]);}
 }
 F->Retired=Hide;F->PatchCount=P->GetObjectField(TEXT("meshes"))->Values.Num();
 for(int32 I:Hide)if(auto* Comp=Cast<UStaticMeshComponent>(W->Node(I)->Comp.Get()))Retire(Comp);
 // Preserve all runtime panel state, but remove only the old photographic draw surfaces.
 for(const TCHAR* Key:{TEXT("sheriffWin"),TEXT("laundroWin"),TEXT("laundro"),TEXT("marquee")})
  for(const FString& Path:W->PathsUnder(FString(TEXT("world.town.runtime.panels."))+Key+TEXT(".")))
   if(const auto* N=W->Node(W->Handle(Path)))if(auto* Comp=Cast<UStaticMeshComponent>(N->Comp.Get())){F->Retired.AddUnique(W->Handle(Path));Retire(Comp);}
 for(const auto& R:Ready)if(R.Name.EndsWith(TEXT("Interior"))&&(R.Family==TEXT("Sheriff")||R.Family==TEXT("Laundry")))
 {
  auto* L=NewObject<UPointLightComponent>(W);L->SetupAttachment(W->Node(R.Parent)->Comp.Get());L->SetMobility(EComponentMobility::Movable);L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(0);L->SetLightColor(FLinearColor(1,.86,.66));L->SetAttenuationRadius(650);L->SetSourceRadius(60);L->SetCastShadows(true);L->RegisterComponent();L->SetWorldLocation(S47Three::ToUE(R.At+FVector(-3.1,2.65,0)));F->Lamps.Add({L,R.Parent,R.Family});
 }
 F->Update();UE_LOG(LogS47TownBuildings,Display,TEXT("S47: town architecture ready: 6 families, 28 native parts, %d composed patches, %d replaced draw meshes; gameplay anchors retained"),F->PatchCount,F->Retired.Num());return F;
}
void FS47TownBuildings::Update()
{
 AS47Web* W=Web.Get();if(!W)return;
 auto Level=[W](const FString& Family){if(Family==TEXT("Sheriff"))return FMath::Clamp(double(W->FloodPos(W->FloodSet(TEXT("town")),10).A)/4.5,0.,1.);if(Family==TEXT("Laundry"))return FMath::Clamp(double(W->FloodPos(W->FloodSet(TEXT("town")),11).A)/6.,0.,1.);return Family==TEXT("Feed")||Family==TEXT("Cinema")||Family==TEXT("Church")?0.:1.;};
 for(int32 I:Retired)if(const auto* N=W->Node(I))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get())){C->SetHiddenInGame(true,false);C->SetVisibility(false,false);}
 for(auto& G:Glows)if(auto* Mid=G.Mid.Get())Mid->SetVectorParameterValue(TEXT("EmissiveFactor"),G.Base*float(Level(G.Family)));
 for(auto& L:Lamps)if(auto* P=L.Light.Get()){const float V=FS47Obj(W,L.Parent).Shown()?float(Level(L.Family)*900):0;P->SetIntensity(V);P->SetVisibility(V>1);}
}
bool FS47TownBuildings::Audit(TArray<FString>& Lines) const
{
 AS47Web* W=Web.Get();bool Ok=W&&Parts.Num()==28&&PatchCount>0;
 for(const auto& P:Parts)Ok&=P.IsValid()&&P->GetStaticMesh()&&P->GetStaticMesh()->GetNumLODs()==3;
 for(int32 I:Retired){const auto* N=W?W->Node(I):nullptr;auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;Ok&=C&&!C->IsVisible()&&C->bHiddenInGame&&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision;}
 Lines.Add(FString::Printf(TEXT("town models %s parts=%d/28 patches=%d retired=%d"),Ok?TEXT("PASS"):TEXT("FAIL"),Parts.Num(),PatchCount,Retired.Num()));return Ok;
}
