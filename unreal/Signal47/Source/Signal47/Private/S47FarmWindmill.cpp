#include "S47FarmWindmill.h"
#include "S47FarmWorld.h"
#include "S47HighwayLandscape.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"

namespace S47FarmWindmill
{
namespace
{
 // Where they stand, road-local: the tower over the old rods' feet (web/src/drive/roadPlaces.ts ranch(), 456,214 less 310),
 // the tank beside it, where its pipe can reach. A 30 ft tower and an 8 ft wheel, as the Aermotor-type mills of the plains.
 const FVector2D MillAt(146,214),TankAt(150.7,210.2);
 constexpr double TowerTop=9.,FootHalf=1.1,TopHalf=.2,WheelR=1.22,WheelY=9.55,TankR=2.3,TankH=.72;
 const TCHAR* SteelPath=TEXT("/Game/FarmBuildings/Materials/M_Farm_Steel.M_Farm_Steel");
 const TCHAR* TinPath=TEXT("/Game/FarmBuildings/Materials/M_Farm_Tin.M_Farm_Tin");
 const TCHAR* WoodPath=TEXT("/Game/FarmBuildings/Materials/M_Farm_Wood.M_Farm_Wood");

 TWeakObjectPtr<AS47Web> BuiltFor;
 TWeakObjectPtr<USceneComponent> Root;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Pieces;   // the steel and tin, which a look can stop on (the water cannot)
 FVector Base=FVector::ZeroVector;   // road-local, the tower's foot on the ground
 FString AppliedArea;int32 AppliedOn=-1;

 bool Exists(const TCHAR* ObjectPath){return FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath)));}
 // A half-width of the tapering tower at a height.
 double Half(double Y){return FootHalf+(TopHalf-FootHalf)*FMath::Clamp(Y/TowerTop,0.,1.);}

 void Build(AS47Web* W)
 {
  UStaticMesh* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
  UStaticMesh* Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
  UMaterialInterface* Steel=LoadObject<UMaterialInterface>(nullptr,SteelPath);
  UMaterialInterface* Tin=LoadObject<UMaterialInterface>(nullptr,TinPath);
  UMaterialInterface* Wood=Exists(WoodPath)?LoadObject<UMaterialInterface>(nullptr,WoodPath):Steel;
  UMaterialInterface* WaterBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
  if(!Cube||!Cylinder||!Steel||!Tin)return;
  auto* R=NewObject<USceneComponent>(W,TEXT("S47FarmWindmill"));
  R->SetupAttachment(W->GetRootComponent());R->SetMobility(EComponentMobility::Movable);R->RegisterComponent();
  Root=R;
  Base=FVector(MillAt.X,S47FarmWorld::FloorRoad(MillAt.X,MillAt.Y),MillAt.Y);
  // A piece of the cube from A to B (three-style metres from the tower's foot), T1 across along Side, T2 the other way.
  auto Beam=[&](const FVector& A,const FVector& B,double T1,double T2,UMaterialInterface* Mat,const FVector& Side)
  {
   const FVector Ua=S47Three::ToUE(A),Ub=S47Three::ToUE(B),D=Ub-Ua;const double L=D.Size();if(L<.5)return;
   FVector X=S47Three::DirToUE(Side);X-=(D/L)*FVector::DotProduct(X,D/L);
   if(X.SizeSquared()<1e-6)X=FMath::Abs(D.Z/L)<.9?FVector(0,0,1):FVector(1,0,0);
   const FQuat Q=FRotationMatrix::MakeFromZX(D/L,X.GetSafeNormal()).ToQuat();
   auto* C=NewObject<UStaticMeshComponent>(W);C->SetupAttachment(R);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Cube);C->SetMaterial(0,Mat);
   C->SetRelativeTransform(FTransform(Q,(Ua+Ub)*.5,FVector(T1,T2,L/100.)));
   C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
   C->SetCastShadow(true);C->bCastContactShadow=false;C->SetReceivesDecals(false);C->RegisterComponent();Pieces.Add(C);
  };
  const FVector Up(0,1,0),East(1,0,0),North(0,0,1);
  // The four legs, angle iron leaning in, their feet a hand into the ground.
  const double Sx[4]={-1,1,1,-1},Sz[4]={-1,-1,1,1};
  for(int32 K=0;K<4;++K)
   Beam(FVector(Sx[K]*FootHalf,-.25,Sz[K]*FootHalf),FVector(Sx[K]*TopHalf,TowerTop,Sz[K]*TopHalf),.075,.075,Steel,FVector(Sx[K],0,-Sz[K]));
  // Girts round the tower and crossed rod braces in every panel of every face.
  const double Levels[]={.2,1.7,3.5,5.3,7.1,8.6};
  for(int32 Li=0;Li<UE_ARRAY_COUNT(Levels);++Li)
  {
   const double Y=Levels[Li],H=Half(Y);
   for(int32 K=0;K<4;++K)
   {
    const int32 N=(K+1)%4;const FVector A(Sx[K]*H,Y,Sz[K]*H),B(Sx[N]*H,Y,Sz[N]*H);
    if(Li>0)Beam(A,B,.055,.035,Steel,Up);
    if(Li+1<UE_ARRAY_COUNT(Levels))
    {
     const double Y2=Levels[Li+1],H2=Half(Y2);
     const FVector A2(Sx[K]*H2,Y2,Sz[K]*H2),B2(Sx[N]*H2,Y2,Sz[N]*H2);
     Beam(A,B2,.02,.02,Steel,Up);Beam(B,A2,.02,.02,Steel,Up);
    }
   }
  }
  // The platform of boards under the head, the mast and the gearbox.
  Beam(FVector(-.62,8.62,0),FVector(.62,8.62,0),1.2,.045,Wood,North);
  Beam(FVector(0,8.6,0),FVector(0,WheelY+.12,0),.11,.11,Steel,East);
  Beam(FVector(0,WheelY,-.42),FVector(0,WheelY,.28),.3,.26,Steel,East);
  // The wheel faces south into the wind: eighteen pitched blades between two rings, and the hub.
  const FVector Hub(0,WheelY,-.62);
  Beam(Hub+FVector(0,0,-.1),Hub+FVector(0,0,.16),.2,.2,Steel,East);
  constexpr int32 Blades=18;
  for(int32 K=0;K<Blades;++K)
  {
   const double A=2*UE_DOUBLE_PI*K/Blades,A2=2*UE_DOUBLE_PI*(K+1)/Blades;
   const FVector Rad(FMath::Cos(A),FMath::Sin(A),0),Rad2(FMath::Cos(A2),FMath::Sin(A2),0),Tan(-FMath::Sin(A),FMath::Cos(A),0);
   const double Pitch=FMath::DegreesToRadians(28.);
   Beam(Hub+Rad*.42,Hub+Rad*WheelR,.24,.012,Steel,Tan*FMath::Cos(Pitch)+North*FMath::Sin(Pitch));
   for(const double Ring:{.44,WheelR*.97})Beam(Hub+Rad*Ring,Hub+Rad2*Ring,.028,.028,Steel,North);
  }
  // The tail: a boom back to a sheet-steel vane, to keep the wheel to the wind.
  Beam(FVector(0,WheelY,.2),FVector(0,WheelY+.1,1.9),.05,.05,Steel,Up);
  Beam(FVector(0,WheelY+.08,1.45),FVector(0,WheelY+.08,2.75),.92,.02,Steel,Up);
  // The pump rod down the middle, the pump stand at the foot, and its pipe over the tank's rim.
  Beam(FVector(.05,.7,.05),FVector(.05,WheelY-.1,.05),.025,.025,Steel,East);
  Beam(FVector(.05,-.1,.05),FVector(.05,.75,.05),.14,.14,Steel,East);
  const FVector Tank(TankAt.X-MillAt.X,S47FarmWorld::FloorRoad(TankAt.X,TankAt.Y)-Base.Y,TankAt.Y-MillAt.Y);
  const FVector ToTank=FVector(Tank.X,0,Tank.Z).GetSafeNormal();
  const FVector Spout=Tank-ToTank*(TankR-.25)+FVector(0,TankH+.12,0);
  Beam(FVector(.05,.6,.05),FVector(Spout.X,.6,Spout.Z),.06,.06,Steel,Up);
  Beam(FVector(Spout.X,.6,Spout.Z),Spout,.06,.06,Steel,East);
  // The stock tank: corrugated tin staves, a rolled rim, and the water.
  constexpr int32 Staves=40;
  for(int32 K=0;K<Staves;++K)
  {
   const double A=2*UE_DOUBLE_PI*(K+.5)/Staves,A2=2*UE_DOUBLE_PI*(K+1.5)/Staves;
   const FVector Rad(FMath::Cos(A),0,FMath::Sin(A)),Rad2(FMath::Cos(A2),0,FMath::Sin(A2)),Tan(-FMath::Sin(A),0,FMath::Cos(A));
   const double Width=2*UE_DOUBLE_PI*TankR/Staves*1.06;
   Beam(Tank+Rad*TankR+FVector(0,-.06,0),Tank+Rad*TankR+FVector(0,TankH,0),Width,.025,Tin,Tan);
   Beam(Tank+Rad*(TankR+.01)+FVector(0,TankH,0),Tank+Rad2*(TankR+.01)+FVector(0,TankH,0),.07,.06,Steel,Up);
  }
  if(WaterBase)
  {
   auto* Water=UMaterialInstanceDynamic::Create(WaterBase,W);
   Water->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(0x1d2620));Water->SetScalarParameterValue(TEXT("Roughness"),.06f);Water->SetScalarParameterValue(TEXT("NativeLighting"),1);
   auto* C=NewObject<UStaticMeshComponent>(W);C->SetupAttachment(R);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Cylinder);C->SetMaterial(0,Water);
   // The engine cylinder is a metre across and a metre tall about its middle.
   C->SetRelativeTransform(FTransform(FQuat::Identity,S47Three::ToUE(Tank+FVector(0,TankH-.14,0)),FVector(TankR*1.98,TankR*1.98,.02)));
   C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->RegisterComponent();
  }
  UE_LOG(LogTemp,Display,TEXT("S47: farm windmill and stock tank: %d pieces at road %.1f %.1f"),Pieces.Num(),MillAt.X,MillAt.Y);
 }
}

 bool Available()
 {
  static const bool bHave=Exists(SteelPath)&&Exists(TinPath);
  return bHave;
 }

 TArray<FBox2D> Footprints()
 {
  return { FBox2D(MillAt - FVector2D(FootHalf + .05), MillAt + FVector2D(FootHalf + .05)) };
 }

 FVector3d TankRound() { return FVector3d(TankAt.X, TankAt.Y, TankR + .05); }

 bool Retired(const FVector& P)
 {
  // The four rods (feet 1.1 m round 456,214), the wheel (1.3 m at 213.6) and the tail (to 215.6); the tank, 3 m round 462,200.
  const bool bMill=P.X>=454.4&&P.X<=457.6&&P.Z>=212.2&&P.Z<=216.0;
  const bool bTank=P.X>=458.8&&P.X<=465.2&&P.Z>=196.8&&P.Z<=203.2;
  return bMill||bTank;
 }

 void Reconcile(AS47Web* W,bool bOn)
 {
  if(!W||!Available())return;
  if(BuiltFor.Get()!=W){BuiltFor=W;Root.Reset();Pieces.Reset();AppliedArea.Reset();AppliedOn=-1;if(bOn)Build(W);}
  else if(bOn&&!Root.IsValid())Build(W);
  USceneComponent* R=Root.Get();if(!R)return;
  const FString Area=W->Area();
  if(Area==AppliedArea&&AppliedOn==int32(bOn))return;
  AppliedArea=Area;AppliedOn=int32(bOn);
  const bool bStation=Area==TEXT("station01");
  R->SetWorldLocationAndRotation(S47Three::ToUE(S47FarmWorld::RoadToArea(Area,Base)),FRotator(0,bStation?90:0,0));
  const bool bShow=bOn&&S47HighwayLandscape::Exterior(Area);
  R->SetVisibility(bShow,true);R->SetHiddenInGame(!bShow,true);
  for(const auto& P:Pieces)if(auto* C=P.Get())C->SetCollisionEnabled(bShow?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
 }
}
