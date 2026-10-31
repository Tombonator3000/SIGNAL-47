#include "S47TripoFleet.h"
#include "S47Fleet.h"
#include "S47Truck.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47Props.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47TripoFleet,Log,All);
namespace
{
 UStaticMesh* Asset(const TCHAR* Id,const TCHAR* Name)
 {
  const FString Path=FString::Printf(TEXT("/Game/Licensed/TripoFleet/%s/%s.%s"),Id,Name,Name);
  UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);
  if(!Mesh||Mesh->GetNumLODs()!=3||Mesh->GetNumTriangles(0)>100000)return nullptr;
  for(const FStaticMaterial& M:Mesh->GetStaticMaterials())if(!M.MaterialInterface)return nullptr;
  return Mesh;
 }
 UStaticMeshComponent* Part(AS47Web* W,UStaticMesh* Mesh,int32 Parent,const FString& Name,const FVector& At=FVector::ZeroVector,double Heading=0)
 {
  const FS47WebNode* N=W->Node(Parent);if(!N||!N->Comp.IsValid())return nullptr;
  auto* C=NewObject<UStaticMeshComponent>(W);C->SetStaticMesh(Mesh);C->SetupAttachment(N->Comp.Get());
  C->SetMobility(EComponentMobility::Movable);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(true);C->RegisterComponent();
  double E[16];S47Three::Compose(At,S47Three::Euler(0,Heading,0),FVector::OneVector,E);C->SetRelativeTransform(S47Three::MatToUE(E));
  const int32 Node=W->AdoptNode(C,Parent,Name);
  if(Node<0){C->DestroyComponent();return nullptr;}
  // AdoptNode reads translation; keep its logical frame aligned with the
  // rendered component when a parked vehicle has a nonzero heading.
  W->ObjAt(Node).SetRotation(FVector(0,Heading,0));return C;
 }
 bool Within(AS47Web* W,int32 I,int32 Root)
 {
  for(;I>=0;){if(I==Root)return true;const FS47WebNode* N=W->Node(I);if(!N)return false;I=N->Parent;}return false;
 }
 void AddFoot(AS47Web* W,const TCHAR* Area,const FBox& B,const FString& Id)
 {
  FS47Rect R;R.Id=Id;R.MinX=B.Min.X/100.;R.MaxX=B.Max.X/100.;R.MinZ=B.Min.Y/100.;R.MaxZ=B.Max.Y/100.;
  auto& Rows=W->CollidersMutable(Area);if(!Rows.ContainsByPredicate([&](const FS47Rect& O){return O.Id==Id;}))Rows.Add(R);
 }
 void PreserveSideTruck(AS47Web* W)
 {
  const FS47Obj Parent=W->Obj(TEXT("world.grounds.group"));if(!Parent||Parent.Find(TEXT("tripo:retained-service-truck")))return;
  const TCHAR* Names[]={TEXT("SM_Saro07_Body"),TEXT("SM_Saro07_Shell"),TEXT("SM_Saro07_Glass"),TEXT("SM_Saro07_Cabin"),TEXT("SM_Saro07_Branding"),TEXT("SM_Saro07_HeadLens"),TEXT("SM_Saro07_TailLens"),TEXT("SM_Saro07_WheelFL"),TEXT("SM_Saro07_WheelFR"),TEXT("SM_Saro07_WheelRL"),TEXT("SM_Saro07_WheelRR")};
  TArray<UStaticMesh*> Meshes;
  for(int32 I=0;I<UE_ARRAY_COUNT(Names);++I)
  {
   const bool Own=I>=3&&I<=6;const FString P=FString(Own?TEXT("/Game/SaroFleet/"):TEXT("/Game/Free/ServiceFleet/"))+Names[I]+TEXT(".")+Names[I];
   auto* Mesh=LoadObject<UStaticMesh>(nullptr,*P);if(!Mesh)return;Meshes.Add(Mesh);
  }
  auto* Group=Part(W,nullptr,Parent.I,TEXT("tripo:retained-service-truck"),FVector(39,-.576,9.8));if(!Group)return;
  const int32 Root=W->NodeOf(Group);TArray<UStaticMeshComponent*> Made;
  const FVector Wheels[]={FVector(-.8003,.4198,-1.3427),FVector(.8143,.4237,-1.3394),FVector(-.8058,.3730,1.5063),FVector(.8087,.3767,1.5097)};
  for(int32 I=0;I<Meshes.Num();++I)
  {
   FVector At=I>=7?Wheels[I-7]:FVector::ZeroVector;if(I>=7)At.Y=Meshes[I]->GetBounds().BoxExtent.Z/100.;
   auto* C=Part(W,Meshes[I],Root,FString::Printf(TEXT("tripo:retained-truck:%d"),I),At);
   if(!C){for(auto* P:Made)P->DestroyComponent();Group->DestroyComponent();return;}Made.Add(C);
  }
  FBox Bounds(ForceInit);for(auto* C:Made){C->UpdateBounds();Bounds+=C->Bounds.GetBox();}AddFoot(W,TEXT("saro"),Bounds,TEXT("tripo:retained-service-truck"));
  UE_LOG(LogS47TripoFleet,Display,TEXT("S47: complete original SARO truck retained in east side bay; separate walk/drive footprint"));
 }
}

TUniquePtr<FS47Fleet> FS47TripoFleet::InstallJeep(AS47Web* W,FS47Truck& T)
{
 if(!W||T.Path==TEXT("world.oldTruck")||FParse::Param(FCommandLine::Get(),TEXT("S47LegacyFleet")))return {};
 UStaticMesh* Mesh=Asset(TEXT("classic_jeep"),TEXT("SM_TripoFleet_ClassicJeep"));if(!Mesh||!T.Group||!T.Body||!T.Cab)return {};
 // Source has one body atlas. A prepared lens mask is required so switching the
 // headlights can never turn the complete painted Jeep into an emissive object.
 FMaterialParameterInfo Head(TEXT("S47Headlamp"));FLinearColor Value;
 if(!Mesh->GetMaterial(0)->GetVectorParameterValue(Head,Value))return {};
 TSet<int32> Keep;
 for(const TCHAR* Key:{TEXT("needles"),TEXT("keys"),TEXT("compass.card")})
 {
  const FS47Obj O=W->Obj(T.Path+TEXT(".")+Key);if(!O)return {};
  Keep.Add(FString(Key)==TEXT("needles")||FString(Key)==TEXT("compass.card")?O.Parent().I:O.I);
 }
 const int32 Gauge=W->Material(T.Path+TEXT(".m.gauge")).Id;
 T.Cab.Traverse([&](FS47Obj O){const auto* N=W->Node(O.I);if(N&&N->Mats.Contains(Gauge))Keep.Add(O.I);});
 TArray<int32> Old;
 for(FS47Obj R:{T.Body,T.Shell,T.Cab,W->Obj(T.Path+TEXT(".wheels"))})R.Traverse([&](FS47Obj O)
 {
  const auto* N=W->Node(O.I);if(!N||!N->Comp.IsValid()||!N->Comp->IsA<UStaticMeshComponent>())return;
  bool Retain=false;for(int32 K:Keep)Retain|=Within(W,O.I,K);if(!Retain)Old.AddUnique(O.I);
 });
 auto* C=Part(W,Mesh,T.Body.I,TEXT("tripo:service-jeep:")+T.Path);if(!C)return {};
 auto F=MakeUnique<FS47Fleet>();F->Web=W;F->RetiredNodes=Old;F->NativeParts.Add(C);F->bTripoJeep=true;
 for(int32 I=0;I<C->GetNumMaterials();++I)
 {
  UMaterialInterface* Material=C->GetMaterial(I);FLinearColor MaskDefault;
  if(Material&&Material->GetVectorParameterValue(Head,MaskDefault))if(auto* Mid=C->CreateDynamicMaterialInstance(I))F->HeadMids.Add(Mid);
 }
 for(int32 I:Old)W->ObjAt(I).SetVisible(false);
 // These anchors are fitted to the inspected source cabin/lens regions, in
 // existing Truck controller terms (metres, Y up, nose -Z).
 T.DriverEye=FVector(-.41,1.70,.65);
 F->JeepHeadlamps[0]=FVector(-.31,1.18,-1.86);F->JeepHeadlamps[1]=FVector(.41,1.18,-1.86);
 // Source has its own seat/dash/steering geometry. Fit only the game's live
 // instrument faces/keys/compass to that dash; no C10 cabin or wheel overlay.
 for(int32 K:Keep)
 {
  bool Nested=false;for(int32 Other:Keep)if(K!=Other&&Within(W,K,Other)){Nested=true;break;}
  if(!Nested){FS47Obj O=W->ObjAt(K);O.SetPosition(O.Position()+FVector(0,.115,.242));}
 }
 F->bReady=true;
 if(T.Path==TEXT("world.saroTruck"))PreserveSideTruck(W);
 UE_LOG(LogS47TripoFleet,Display,TEXT("S47: source Tripo Jeep bound to %s; original transport/save/entry/exit/instruments retained; source wheels remain one mesh"),*T.Path);
 return F;
}

void FS47TripoFleet::InstallWorld(AS47Web* W)
{
 if(!W||FParse::Param(FCommandLine::Get(),TEXT("S47LegacyFleet")))return;
 auto* Police=Asset(TEXT("police_car"),TEXT("SM_TripoFleet_PoliceCar"));
 auto* Door=Asset(TEXT("police_car"),TEXT("SM_TripoFleet_PoliceCarDoor"));
 if(Police&&Door)AS47Props::RetargetPatrolCar(Police,Door,FVector(-.99,.83,-1.00));
 const FS47Obj Diner=W->Obj(TEXT("world.diner.group"));if(!Diner||Diner.Find(TEXT("tripo:mesa-tractor")))return;
 UStaticMesh* Tractor=Asset(TEXT("heavy_truck"),TEXT("SM_TripoFleet_HeavyTruck"));
 const FS47Obj Original=Diner.Find(TEXT("prop:mesaTruck"));
 if(Tractor&&Original)
 {
  auto* C=Part(W,Tractor,Diner.I,TEXT("tripo:mesa-tractor"),FVector(9.28,0,-14.15),-UE_DOUBLE_PI/2);
  if(C)
  {
   if(const auto* N=W->Node(Original.I))if(auto* Old=Cast<UStaticMeshComponent>(N->Comp.Get())){Old->SetHiddenInGame(true);Old->SetCastShadow(false);}
   C->UpdateBounds();const FBox Bounds=C->Bounds.GetBox();
   for(const TCHAR* Walk:{TEXT("road"),TEXT("diner")})
   {
    for(auto& R:W->CollidersMutable(Walk))if(FMath::IsNearlyEqual(R.MinX,7983.65,.001)&&FMath::IsNearlyEqual(R.MaxX,7994.8,.001)&&FMath::IsNearlyEqual(R.MinZ,1984.4,.001)&&FMath::IsNearlyEqual(R.MaxZ,1987.2,.001))R.bEnabled=false;
    AddFoot(W,Walk,Bounds,TEXT("tripo:mesa-tractor"));
   }
   UE_LOG(LogS47TripoFleet,Display,TEXT("S47: actual Tripo road tractor replaces independent diner rig; rig story proxy preserved; player Jeep stays distinct"));
  }
 }
}
