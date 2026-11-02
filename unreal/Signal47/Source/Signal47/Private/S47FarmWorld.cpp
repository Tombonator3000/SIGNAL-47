#include "S47FarmWorld.h"
#include "S47Main.h"
#include "S47World.h"
#include "S47Web.h"
#include "S47Geo.h"
#include "S47RoadTerrain.h"
#include "S47HighwayLandscape.h"
#include "S47Three.h"
#include "S47TownLayer.h"
#include "S47Herd.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
namespace
{
 struct FPart {TWeakObjectPtr<UStaticMeshComponent> C;FVector Road,Size;};
 TWeakObjectPtr<AS47Web> Built,AppliedWorld;FString AppliedArea;bool AppliedOn=false;TArray<FPart> Parts;
 void Build(AS47Web* W)
 {
  if(Built==W)return;Built=W;Parts.Reset();auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));if(!Cube)return;
  auto* Wood=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/TownBuildings/Materials/M_Town_DarkWood.M_Town_DarkWood"));auto* Metal=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/TownBuildings/Materials/M_Town_Metal.M_Town_Metal"));
  UMaterialInterface* Gravel=nullptr;if(const auto* N=W->Node(1250))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))Gravel=C->GetMaterial(0);
  auto Box=[&](const FString& Name,FVector Road,FVector Size,UMaterialInterface* Material)
  {
   auto* C=NewObject<UStaticMeshComponent>(W);C->SetupAttachment(W->GetRootComponent());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Cube);if(Material)C->SetMaterial(0,Material);
   C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCastShadow(true);C->RegisterComponent();W->AdoptNode(C,INDEX_NONE,Name);Parts.Add({C,Road,Size});
  };
  // Visible road dressing supports the same authoritative driving/walking floor; no invisible floor bypass.
  for(int32 I=0;I<22;++I){const double X=8+I*5.;Box(FString::Printf(TEXT("farm:ground/path:%02d"),I),FVector(X,S47FarmWorld::FloorRoad(X,S47RoadTerrain::RanchZ(X))-.08,S47RoadTerrain::RanchZ(X)),FVector(5.03,.16,6.4),Gravel);}
  auto Fence=[&](double X0,double Z0,double X1,double Z1,int32 Id)
  {
   const double L=FVector2D(X1-X0,Z1-Z0).Size();const int32 Count=FMath::CeilToInt(L/4.);
   for(int32 I=0;I<=Count;++I){const double K=double(I)/Count,X=FMath::Lerp(X0,X1,K),Z=FMath::Lerp(Z0,Z1,K);Box(FString::Printf(TEXT("farm:fence:%d:post%d"),Id,I),FVector(X,S47FarmWorld::FloorRoad(X,Z)+.57,Z),FVector(.12,1.25,.12),Wood);}
   for(double H:{.45,.95}){const double X=(X0+X1)*.5,Z=(Z0+Z1)*.5;Box(FString::Printf(TEXT("farm:fence:%d:rail%.0f"),Id,H*100),FVector(X,S47FarmWorld::FloorRoad(X,Z)+H,Z),X0==X1?FVector(.05,.06,L):FVector(L,.06,.05),Metal);}
  };
  Fence(30,215,30,265,0);Fence(80,215,80,265,1);Fence(30,265,80,265,2);Fence(30,215,66,215,3);Fence(75,215,80,215,4);
  const double Y=S47FarmWorld::FloorRoad(76,251);Box(TEXT("farm:trough:bottom"),{76,Y+.15,251},{2,.16,.9},Metal);
  for(double DX:{-1.,1.})Box(TEXT("farm:trough:end"),{76+DX,Y+.40,251},{.10,.6,.9},Metal);
  for(double DZ:{-.45,.45})Box(TEXT("farm:trough:side"),{76,Y+.40,251+DZ},{2,.6,.08},Metal);
 }
}
namespace S47FarmWorld
{
 bool Enabled(const US47Main* M){return M&&M->IsFreeform();}
 FVector HouseOffset(const US47Main* M){return Enabled(M)?FVector(-310,0,0):FVector::ZeroVector;}
 FVector HerdOffset(const US47Main* M){return Enabled(M)?FVector(92,0,-316):FVector::ZeroVector;}
 FVector RoadToArea(const FString& Area,const FVector& P)
 {
  if(Area==TEXT("saro"))return P+FVector(-24,0,300);
  if(Area==TEXT("station01")){const auto S=S47Geo::RoadToStation(P.X,P.Z);return FVector(S.X,P.Y+S47Geo::STATION_DY(),S.Z+8000);}
  return P+FVector(8000,0,0);
 }
 bool InRoadAccess(double X,double Z)
 {
  return (X>=6&&X<=116&&FMath::Abs(Z-S47RoadTerrain::RanchZ(X))<=3.1)||(X>=27&&X<=161&&Z>=150&&Z<=268);
 }
 double FloorRoad(double X,double Z){return S47RoadTerrain::SurfaceY(X,Z);}
 bool HouseVisible(const US47Main* M)
 {
  if(!M||!M->Web)return false;int32 Count=0;for(int32 I=0;;++I){const auto* N=M->Web->Node(I);if(!N)break;if(N->Name.StartsWith(TEXT("highway:ranch:g")))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))if(C->IsVisible()&&!C->bHiddenInGame)++Count;}return Count==3;
 }
 FVector HousePhotoPoint(const US47Main* M)
 {
  if(!M||!M->Web)return FVector::ZeroVector;FBox Bounds(ForceInit);for(int32 I=0;;++I){const auto* N=M->Web->Node(I);if(!N)break;if(N->Name.StartsWith(TEXT("highway:ranch:g")))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))Bounds+=C->Bounds.GetBox();}return Bounds.IsValid?S47Three::ToThree(Bounds.GetCenter()):FVector::ZeroVector;
 }
 void Reconcile(US47Main* M)
 {
  if(!M||!M->Web||!M->World)return;auto* W=M->Web.Get();ReconcileRanch(M);const bool On=Enabled(M)&&GateReady(M);M->World->ReconcileFarm(On);Build(W);
  const bool Apply=AppliedWorld!=W||AppliedArea!=W->Area()||AppliedOn!=On;
  if(Apply)for(auto& P:Parts)if(auto* C=P.C.Get())
  {
   C->SetWorldLocation(S47Three::ToUE(RoadToArea(W->Area(),P.Road)));C->SetWorldRotation(W->Area()==TEXT("station01")?FRotator(0,90,0):FRotator::ZeroRotator);
   C->SetWorldScale3D(FVector(P.Size.X,P.Size.Z,P.Size.Y));C->SetVisibility(On&&S47HighwayLandscape::Exterior(W->Area()));C->SetHiddenInGame(!On);C->SetCollisionEnabled(On?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
  }
  AppliedWorld=W;AppliedArea=W->Area();AppliedOn=On;
  if(M->TownLayer)for(const auto& H:M->TownLayer->Herds)if(H){H->ReconcileMode();if(Enabled(M)){H->Set(false,-1,false);H->Update(M->T);}}
 }
 bool Ready(US47Main* M){return Enabled(M)&&GateReady(M)&&HouseVisible(M)&&Built==M->Web.Get()&&Parts.Num()>30;}
 bool Audit(US47Main* M,TArray<FString>& Lines)
 {
  const bool Good=M&&GateReady(M)&&(!Enabled(M)||Ready(M));Lines.Add(FString(Good?TEXT("PASS "):TEXT("FAIL "))+TEXT("farm exact gate partitions, native housegroup and mode-derived physical access"));return Good;
 }
 bool SelfTest(TArray<FString>& Lines)
 {
  const bool Good=InRoadAccess(21,S47RoadTerrain::RanchZ(21))&&InRoadAccess(96,196.8)&&InRoadAccess(55,240)&&!InRoadAccess(200,196)&&RoadToArea(TEXT("saro"),{21,0,196}).Equals({-3,0,496},.00001);
  Lines.Add(FString(Good?TEXT("PASS "):TEXT("FAIL "))+TEXT("Alderete road, yard, pasture and scene coordinates remain bounded"));return Good;
 }
}
