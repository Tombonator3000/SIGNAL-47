#include "S47EastWing.h"
#include "S47EastWingProps.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Story.h"
#include "S47Web.h"
#include "S47Obj.h"
#include "S47Three.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Props.h"
#include "S47Flow.h"
#include "S47Drive.h"
#include "S47Truck.h"
#include "S47Startup.h"
#include "Misc/FileHelper.h"
#include "S47SaroSite.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47EastWing,Log,All);

namespace
{
 // Exterior wall planes of the exported shell, wall thickness and openings (three.js metres).
 constexpr double X0=20.6,X1=38.6,Z0=-5.5,Z1=5.5,T=.25,H=3.6,Ceil=3.32;
 constexpr double LabX=26.0,GarageX=31.6,StoreZ=.5;
 struct FGap{double A,B,Y0,Y1;};
 const FGap SouthGaps[]={{21.3,22.4,0,2.15},{33.2,37.6,0,3.0}};            // personnel door, roll-up door
 const FGap NorthGaps[]={{21.4,23.0,1.0,2.25},{23.6,25.2,1.0,2.25},{27.0,28.6,1.0,2.25},{29.2,30.8,1.0,2.25},{33.0,34.6,1.6,2.4},{35.6,37.2,1.6,2.4}};
}

FS47EastWing::FS47EastWing(US47Main* InM):M(InM)
{
 Tick=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float Dt){return Step(Dt);}));
}
FS47EastWing::~FS47EastWing(){FTSTicker::GetCoreTicker().RemoveTicker(Tick);}

FBox2D FS47EastWing::Room(int32 I)
{
 switch(I)
 {
  case 0:return FBox2D(FVector2D(X0+T,Z0+T),FVector2D(LabX,Z1-T));
  case 1:return FBox2D(FVector2D(LabX,Z0+T),FVector2D(GarageX,StoreZ));
  case 2:return FBox2D(FVector2D(LabX,StoreZ),FVector2D(GarageX,Z1-T));
  default:return FBox2D(FVector2D(GarageX,Z0+T),FVector2D(X1-T,Z1-T));
 }
}

void FS47EastWing::Init()
{
 if(bBuilt||!M||!M->Web||!M->Web->IsLoaded()||!M->World)return;
 if(FParse::Param(FCommandLine::Get(),TEXT("S47EastWingDump")))Dump();
 if(FParse::Param(FCommandLine::Get(),TEXT("S47EastWingOld")))return; // developer comparison: keep the exported wing
 if(!Retire()){UE_LOG(LogS47EastWing,Error,TEXT("S47: east wing: exported shell not found exactly; original retained"));bBuilt=true;return;}
 if(FS47SaroSite* Site=M->Web->Site())Site->ReplaceWing(); // the earlier native facade yields to this building
 RelocateAccessDressing();Build();PropsHelper=MakeUnique<FS47EastWingProps>();PropsHelper->Build(M,Parts);bBuilt=true;
 UE_LOG(LogS47EastWing,Display,TEXT("S47: east wing native shell built (test lab, parts, calibration, garage)"));
}

void FS47EastWing::Dump() const
{
 AS47Web* W=M->Web;const FBox Wing=Footprint().ExpandBy(FVector(.6,.2,.6));
 int32 Inside=0,Crossing=0;
 for(int32 Node=0;Node<W->PartCount();++Node)
 {
  const FS47WebNode* N=W->Node(Node);if(!N||N->Geo<0)continue;
  const FBox B=W->ObjAt(Node).WorldBox();if(!B.IsValid||!B.Intersect(Wing))continue;
  const bool Full=Wing.IsInside(B.Min)&&Wing.IsInside(B.Max);(Full?Inside:Crossing)++;
  UE_LOG(LogS47EastWing,Display,TEXT("S47: eastwing node %d %s name=%s geo=%d min=(%.2f %.2f %.2f) max=(%.2f %.2f %.2f)"),Node,Full?TEXT("INSIDE"):TEXT("CROSS"),*N->Name,N->Geo,B.Min.X,B.Min.Y,B.Min.Z,B.Max.X,B.Max.Y,B.Max.Z);
 }
 UE_LOG(LogS47EastWing,Display,TEXT("S47: eastwing survey inside=%d crossing=%d"),Inside,Crossing);
}

bool FS47EastWing::Retire()
{
 // Exactly three exported pieces belong to the wing alone: its shell and the two layers of
 // its south-face lettering. Everything else on its roof and walls is shared site dressing.
 AS47Web* W=M->Web;const FBox Wing=Footprint().ExpandBy(FVector(.6,.2,.6));
 TArray<int32> Shell,Letters;
 for(int32 Node=0;Node<W->PartCount();++Node)
 {
  const FS47WebNode* N=W->Node(Node);if(!N||N->Geo<0)continue;
  const FBox B=W->ObjAt(Node).WorldBox();if(!B.IsValid||!Wing.IsInside(B.Min)||!Wing.IsInside(B.Max))continue;
  if(N->Name==TEXT("house:wing"))Shell.Add(Node);
  else if(N->Name==TEXT("look:wingLetters")||(N->Name.IsEmpty()&&B.Min.Z>5.4&&B.Max.Z<5.7&&B.Min.X>22&&B.Max.X<31))Letters.Add(Node);
 }
 if(Shell.Num()!=1||Letters.Num()!=2){UE_LOG(LogS47EastWing,Error,TEXT("S47: east wing retire expected shell=1 letters=2, found %d/%d"),Shell.Num(),Letters.Num());return false;}
 Parent=W->Node(Shell[0])->Parent;
 TArray<int32> All=Shell;All.Append(Letters);
 for(int32 Node:All)if(UPrimitiveComponent* C=Cast<UPrimitiveComponent>(W->Node(Node)->Comp.Get())){C->SetVisibility(false,true);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);}
 return true;
}

void FS47EastWing::Block(const FString& Name,const FVector& Min,const FVector& Max,UMaterialInterface* Mat,bool bShadow)
{
 static UStaticMesh* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 AS47Web* W=M->Web;if(!Cube||!W)return;
 const FVector C=(Min+Max)*.5,S=(Max-Min).GetAbs();if(S.X<1e-4||S.Y<1e-4||S.Z<1e-4)return;
 UStaticMeshComponent* Comp=NewObject<UStaticMeshComponent>(W,*FString::Printf(TEXT("EastWing_%s"),*Name));
 Comp->SetStaticMesh(Cube);Comp->SetMaterial(0,Mat);Comp->SetMobility(EComponentMobility::Movable);Comp->SetCastShadow(bShadow);
 Comp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Comp->SetCollisionResponseToAllChannels(ECR_Block);
 Comp->SetupAttachment(W->GetRootComponent());Comp->RegisterComponent();
 Comp->SetWorldTransform(FTransform(FQuat::Identity,S47Three::ToUE(C),FVector(S.X,S.Z,S.Y))); // engine cube is 1 m
 Parts.Add(Comp);
}

// Fluorescent fixture: a downward rect light the size of the SM_EW_FluorescentFixture diffuser (0.95 x 0.25 m),
// soft shadows and barn doors so ceilings stay darker than work surfaces (EASTWING photo pass).
void FS47EastWing::Light(const FString& Name,const FVector& At,float Lumens,const FLinearColor& Colour,double YawDegrees)
{
 AS47Web* W=M->Web;URectLightComponent* L=NewObject<URectLightComponent>(W,*FString::Printf(TEXT("EastWingLight_%s"),*Name));
 L->SetMobility(EComponentMobility::Movable);L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(Lumens);L->SetLightColor(Colour);
 L->SetAttenuationRadius(1100);L->SetSourceWidth(95);L->SetSourceHeight(25);L->SetBarnDoorAngle(70);L->SetBarnDoorLength(12);L->SetCastShadows(true);
 L->SetupAttachment(W->GetRootComponent());L->RegisterComponent();
 // Rect lights emit along +X; pitch down. Source width runs along local Y: yaw 0 = tube along site z
 // (the fixture mesh's long axis at its yaw 0), yaw 90 = along site x.
 L->SetWorldLocationAndRotation(S47Three::ToUE(At),FRotator(-90,YawDegrees,0));Parts.Add(L);
}

// EASTWING photo pass. Decal X projects into the surface; size is half-extent (depth, width, height) in cm.
void FS47EastWing::Grime()
{
 AS47Web* W=M->Web;
 auto Mat=[](const TCHAR* Name){return LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/EastWing/Decals/%s.%s"),Name,Name));};
 UMaterialInterface* Base=Mat(TEXT("M_EW_GrimeWallBase"));UMaterialInterface* Dust=Mat(TEXT("M_EW_FloorEdgeDust"));
 UMaterialInterface* Scuff=Mat(TEXT("M_EW_GrimeScuff"));UMaterialInterface* Streak=Mat(TEXT("M_EW_GrimeStreaks"));
 if(!Base||!Dust||!Scuff||!Streak){UE_LOG(LogS47EastWing,Warning,TEXT("S47: east wing grime decals unavailable; clean surfaces kept"));return;}
 int32 Count=0;
 auto Decal=[&](UMaterialInterface* Mt,const FVector& At,const FRotator& Rot,const FVector& HalfCm,float Opacity)
 {
  UDecalComponent* D=NewObject<UDecalComponent>(W,*FString::Printf(TEXT("EastWingGrime_%03d"),Count++));
  UMaterialInstanceDynamic* I=UMaterialInstanceDynamic::Create(Mt,D);I->SetScalarParameterValue(TEXT("Opacity"),Opacity);
  // -S47EastWingDecalProbe: paint wall/floor bands saturated red to verify texture orientation in screenshots.
  if(FParse::Param(FCommandLine::Get(),TEXT("S47EastWingDecalProbe"))&&(Mt==Base||Mt==Dust)){I->SetVectorParameterValue(TEXT("Tint"),FLinearColor(30,0,0));I->SetScalarParameterValue(TEXT("Opacity"),1.f);}
  D->SetDecalMaterial(I);D->DecalSize=HalfCm;D->SetFadeScreenSize(0.f);
  D->SetupAttachment(W->GetRootComponent());D->RegisterComponent();D->SetWorldLocationAndRotation(S47Three::ToUE(At),Rot);Parts.Add(D);
 };
 // Interior wall faces per room: bX true = wall at x=C spanning z A..B; Dir is +1/-1 toward the wall.
 struct FWall{bool bX;double C,A,B;int Dir;};
 static const FWall Walls_[]={
  {true,20.85,-5.25,5.25,-1},{true,25.925,-5.25,5.25,1},{false,-5.25,20.85,25.925,-1},{false,5.25,20.85,25.925,1},     // lab
  {true,26.075,-5.25,.425,-1},{true,31.525,-5.25,.425,1},{false,-5.25,26.075,31.525,-1},{false,.425,26.075,31.525,1}, // storage
  {true,26.075,.575,5.25,-1},{true,31.525,.575,5.25,1},{false,.575,26.075,31.525,-1},{false,5.25,26.075,31.525,1},    // calibration
  {true,31.675,-5.25,5.25,-1},{true,38.35,-5.25,5.25,1},{false,-5.25,31.675,38.35,-1},                                 // garage
  {false,5.25,31.675,33.2,1},{false,5.25,37.6,38.35,1}};
 for(const FWall& Wl:Walls_)
 {
  // Shallow boxes stopping short of corners: deeper boxes smear onto perpendicular returns and jambs.
  const double Mid=(Wl.A+Wl.B)*.5,Half=(Wl.B-Wl.A)*.5*100-6;
  const double Yaw=Wl.bX?(Wl.Dir<0?180.:0.):(Wl.Dir<0?-90.:90.);
  const FVector WallAt=Wl.bX?FVector(Wl.C,.28,Mid):FVector(Mid,.28,Wl.C);
  Decal(Base,WallAt,FRotator(0,Yaw,0),FVector(4,Half,28),.85f);
  // Floor dust strip, 0.22 m into the room, projected straight down.
  const FVector FloorAt=Wl.bX?FVector(Wl.C-Wl.Dir*.22,0,Mid):FVector(Mid,0,Wl.C-Wl.Dir*.22);
  Decal(Dust,FloorAt,FRotator(-90,Yaw,0),FVector(6,Half,22),.8f);
 }
 // Hand and hip scuffs on both jambs of every door (centre of jamb, room side and far side).
 struct FJamb{bool bX;double C,Edge;int Dir;};
 static const FJamb Jambs[]={
  {false,5.25,21.3,1},{false,5.25,22.4,1},                                 // personnel door (lab side)
  {true,25.925,2.4,1},{true,25.925,3.4,1},{true,26.075,2.4,-1},{true,26.075,3.4,-1},
  {true,31.525,-3.,1},{true,31.525,-2.,1},{true,31.675,-3.,-1},{true,31.675,-2.,-1},
  {true,31.525,2.9,1},{true,31.525,3.9,1},{true,31.675,2.9,-1},{true,31.675,3.9,-1},
  {false,.425,28.3,1},{false,.425,29.3,1},{false,.575,28.3,-1},{false,.575,29.3,-1},
  {true,38.35,1.2,1},{true,38.35,2.3,1}};
 for(const FJamb& J:Jambs)
 {
  const double Yaw=J.bX?(J.Dir<0?180.:0.):(J.Dir<0?-90.:90.);
  Decal(Scuff,J.bX?FVector(J.C,1.05,J.Edge):FVector(J.Edge,1.05,J.C),FRotator(0,Yaw,0),FVector(4,32,48),.7f);
 }
 // Outside: run-off streaks under each north window sill and under the south sign board.
 for(const FGap& G:NorthGaps)Decal(Streak,FVector((G.A+G.B)*.5,G.Y0-.5,Z0),FRotator(0,90,0),FVector(12,(G.B-G.A)*50+8,50),.75f);
 Decal(Streak,FVector(26.9,.62,Z1),FRotator(0,-90,0),FVector(12,350,45),.55f);
 UE_LOG(LogS47EastWing,Display,TEXT("S47: east wing grime: %d deferred decals (wall bases, floor dust, jamb scuffs, run-off)"),Count);
}

void FS47EastWing::Build()
{
 if(BuildDetailed())
 {
  // Cool-white 4500 K tubes, one rect light per fixture (fixtures listed in S47EastWingProps Layout).
  const FLinearColor Tube=FLinearColor::MakeFromColorTemperature(4500.f);
  struct FFix{const TCHAR* Name;double X,Z,Yaw;float Lumens;};
  static const FFix Fixtures[]={{TEXT("Lab"),23.4,-.7,0,90},{TEXT("Lab2"),23.4,2.6,0,90},{TEXT("Store"),28.75,-2.1,90,75},
   {TEXT("Cal"),28.85,3.1,90,75},{TEXT("Garage"),35.4,.1,0,100},{TEXT("Garage2"),35.4,-3.2,0,100},{TEXT("Garage3"),35.4,3.4,0,100}};
  for(const FFix& F:Fixtures)Light(F.Name,FVector(F.X,3.16,F.Z),F.Lumens,Tube,F.Yaw);
  Grime();Walls();return;
 }
 AS47Web* W=M->Web;
 UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 auto Mat=[W,Base](uint32 Color,float Rough=.85f,float Metal=0.f)->UMaterialInterface*
 {
  UMaterialInstanceDynamic* I=UMaterialInstanceDynamic::Create(Base,W);
  I->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(Color));I->SetScalarParameterValue(TEXT("NativeLighting"),1);
  I->SetScalarParameterValue(TEXT("Roughness"),Rough);I->SetScalarParameterValue(TEXT("Metalness"),Metal);I->SetScalarParameterValue(TEXT("EmissivePower"),0);return I;
 };
 UMaterialInterface *Stucco=Mat(0xc9bba0),*Masonry=Mat(0xd8cdb2),*Floor=Mat(0x8a8478,.7f),*Garage=Mat(0x7b776f,.6f),*Ceiling=Mat(0xb9b3a4),*Plinth=Mat(0x8f8576),*Door=Mat(0x3e5a4c,.5f),*Sign=Mat(0x324238,.6f),*Yellow=Mat(0xd6a51c,.6f);
 // Exterior shell: each wall split round its openings. Exterior skin stucco, inner face block.
 auto Wall=[&](const TCHAR* Name,bool bAlongX,double Fixed,double A,double B,const TArray<FGap>& Gaps,double Inward)
 {
  TArray<FGap> G=Gaps;G.Sort([](const FGap& L,const FGap& R){return L.A<R.A;});
  double At=A;int32 I=0;
  auto Piece=[&](double S0,double S1,double Y0,double Y1)
  {
   if(S1-S0<1e-3||Y1-Y0<1e-3)return;
   const double F0=FMath::Min(Fixed,Fixed+Inward),F1=FMath::Max(Fixed,Fixed+Inward);
   const FVector Mn=bAlongX?FVector(S0,Y0,F0):FVector(F0,Y0,S0),Mx=bAlongX?FVector(S1,Y1,F1):FVector(F1,Y1,S1);
   Block(FString::Printf(TEXT("%s_%d"),Name,I++),Mn,Mx,Stucco);
  };
  for(const FGap& Gap:G){Piece(At,Gap.A,0,H);Piece(Gap.A,Gap.B,Gap.Y1,H);Piece(Gap.A,Gap.B,0,Gap.Y0);At=Gap.B;}
  Piece(At,B,0,H);
 };
 TArray<FGap> S(SouthGaps,UE_ARRAY_COUNT(SouthGaps)),N(NorthGaps,UE_ARRAY_COUNT(NorthGaps)),None;
 Wall(TEXT("South"),true,Z1,X0,X1,S,-T);Wall(TEXT("North"),true,Z0,X0,X1,N,T);
 Wall(TEXT("West"),false,X0,Z0,Z1,None,T);Wall(TEXT("East"),false,X1,Z0,Z1,None,-T);
 // Plinth band round the base (the old shell stood on a 0.6 m plinth down to the ground).
 Block(TEXT("PlinthS"),FVector(X0,-.6,Z1-.02),FVector(X1,0,Z1+.06),Plinth);Block(TEXT("PlinthN"),FVector(X0,-.6,Z0-.06),FVector(X1,0,Z0+.02),Plinth);
 Block(TEXT("PlinthW"),FVector(X0-.06,-.6,Z0),FVector(X0+.02,0,Z1),Plinth);Block(TEXT("PlinthE"),FVector(X1-.02,-.6,Z0),FVector(X1+.06,0,Z1),Plinth);
 // Floors and ceiling.
 Block(TEXT("Floor"),FVector(X0+T,-.08,Z0+T),FVector(GarageX,0,Z1-T),Floor,false);
 Block(TEXT("GarageFloor"),FVector(GarageX,-.08,Z0+T),FVector(X1-T,0,Z1-T),Garage,false);
 Block(TEXT("Ceiling"),FVector(X0+T,Ceil,Z0+T),FVector(X1-T,Ceil+.06,Z1-T),Ceiling);
 // Interior partitions with doors: lab | middle, middle | garage, storage over calibration.
 const double P=.15;
 Block(TEXT("LabWallA"),FVector(LabX-P*.5,0,Z0+T),FVector(LabX+P*.5,Ceil,2.4),Masonry);
 Block(TEXT("LabWallB"),FVector(LabX-P*.5,0,3.4),FVector(LabX+P*.5,Ceil,Z1-T),Masonry);
 Block(TEXT("LabWallHead"),FVector(LabX-P*.5,2.15,2.4),FVector(LabX+P*.5,Ceil,3.4),Masonry);
 Block(TEXT("GarWallA"),FVector(GarageX-P*.5,0,Z0+T),FVector(GarageX+P*.5,Ceil,-3.0),Masonry);
 Block(TEXT("GarWallB"),FVector(GarageX-P*.5,0,-2.0),FVector(GarageX+P*.5,Ceil,2.9),Masonry);
 Block(TEXT("GarWallC"),FVector(GarageX-P*.5,0,3.9),FVector(GarageX+P*.5,Ceil,Z1-T),Masonry);
 Block(TEXT("GarWallHead1"),FVector(GarageX-P*.5,2.15,-3.0),FVector(GarageX+P*.5,Ceil,-2.0),Masonry);
 Block(TEXT("GarWallHead2"),FVector(GarageX-P*.5,2.15,2.9),FVector(GarageX+P*.5,Ceil,3.9),Masonry);
 Block(TEXT("StoreWallA"),FVector(LabX,0,StoreZ-P*.5),FVector(28.3,Ceil,StoreZ+P*.5),Masonry);
 Block(TEXT("StoreWallB"),FVector(29.3,0,StoreZ-P*.5),FVector(GarageX,Ceil,StoreZ+P*.5),Masonry);
 Block(TEXT("StoreWallHead"),FVector(28.3,2.15,StoreZ-P*.5),FVector(29.3,Ceil,StoreZ+P*.5),Masonry);
 // South facade: the retained SARO sign as a dark green board, personnel door, roll-up door, apron and ramps.
 Block(TEXT("SignBoard"),FVector(23.4,1.05,Z1),FVector(30.4,2.75,Z1+.08),Sign);
 // Keep an open leaf in Freeform and a visibly closed leaf in Story; custom wall locks follow mode.
 Block(TEXT("DoorClosed"),FVector(21.35,0,Z1-.06),FVector(22.35,2.12,Z1-.02),Door);
 Block(TEXT("DoorOpen"),FVector(21.30,0,Z1-1.02),FVector(21.35,2.12,Z1-.02),Door);
 Block(TEXT("GarageClosed"),FVector(33.2,0,Z1-.06),FVector(37.6,3.0,Z1-.02),Door);
 Block(TEXT("Awning"),FVector(21.0,2.35,Z1),FVector(22.7,2.42,Z1+.9),Door);
 Block(TEXT("RollUpHead"),FVector(33.1,3.0,Z1-.02),FVector(37.7,3.35,Z1+.25),Plinth);
 Block(TEXT("Apron"),FVector(32.6,-.6,Z1),FVector(38.2,-.02,Z1+5.0),Garage,false);
 Block(TEXT("DoorStep"),FVector(20.9,-.6,Z1),FVector(22.8,-.02,Z1+1.6),Garage,false);
 for(const double X:{32.7,38.1})Block(FString::Printf(TEXT("Bollard%.0f"),X*10),FVector(X-.09,-.58,Z1+.12),FVector(X+.09,.55,Z1+.3),Yellow);
 // Garage floor lines.
 for(const double X:{33.6,37.2})Block(FString::Printf(TEXT("Line%.0f"),X*10),FVector(X-.05,0,Z0+1.0),FVector(X+.05,.005,Z1-T),Yellow,false);
 // Blockout light per room (fluorescent white); replaced by fixtures in the detail pass.
 const FLinearColor Tube(1.f,.95f,.86f);
 Light(TEXT("Lab"),FVector(23.4,3.12,-.7),350,Tube);Light(TEXT("Store"),FVector(28.75,3.12,-2.1),250,Tube);
 Light(TEXT("Cal"),FVector(28.85,3.12,3.1),250,Tube);Light(TEXT("Garage"),FVector(35.4,3.12,.1),400,Tube);
 Walls();
}

void FS47EastWing::Walls()
{
 // Walking: the wing stops being an excluded block of open land; its floor is y 0 with ramps
 // down to the apron (-0.58) at the personnel door and the roll-up door; walls collide.
 const int32 Removed=M->World->RemoveLandOut(TEXT("saro"),20.2,39.0,-5.9,5.9);
 UE_LOG(LogS47EastWing,Display,TEXT("S47: east wing opened to walking (%d exclusion zone removed)"),Removed);
 AS47Props::AddFloor(33.2,37.6,Z1,Z1+4.0,0,-.576,TEXT("z"),true); // garage ramp to the apron
 AS47Props::AddFloor(21.0,22.7,Z1,Z1+1.6,0,-.576,TEXT("z"),true); // personnel door ramp
 AS47Props::AddFloor(X0,X1,Z0,Z1,0,0,TEXT("x"),false);             // floor inside
 TArray<FS47Rect>& C=M->Web->CollidersMutable(TEXT("saro"));
 auto R=[&C](double A,double B,double D,double E,const TCHAR* Id){C.Add({A,B,D,E,FString(TEXT("eastwing:"))+Id,true});};
 // South: west of door, between door and roll-up, east of roll-up. North and gables solid.
 R(X0,21.3,Z1-T,Z1,TEXT("s0"));R(22.4,33.2,Z1-T,Z1,TEXT("s1"));R(37.6,X1,Z1-T,Z1,TEXT("s2"));
 R(X0,X1,Z0,Z0+T,TEXT("n"));R(X0,X0+T,Z0,Z1,TEXT("w"));
 if(NativeDoors.Num()==3)
 {
  R(X1-T,X1,Z0,1.2,TEXT("e0"));R(X1-T,X1,2.3,Z1,TEXT("e1"));R(X1-T,X1,1.2,2.3,TEXT("lock-service"));
  AS47Props::AddFloor(X1,X1+1.6,1.,2.5,0,-.576,TEXT("x"),true);
 }
 else R(X1-T,X1,Z0,Z1,TEXT("e"));
 R(LabX-.08,LabX+.08,Z0,2.4,TEXT("lab0"));R(LabX-.08,LabX+.08,3.4,Z1,TEXT("lab1"));
 R(GarageX-.08,GarageX+.08,Z0,-3.0,TEXT("g0"));R(GarageX-.08,GarageX+.08,-2.0,2.9,TEXT("g1"));R(GarageX-.08,GarageX+.08,3.9,Z1,TEXT("g2"));
 R(LabX,28.3,StoreZ-.08,StoreZ+.08,TEXT("st0"));R(29.3,GarageX,StoreZ-.08,StoreZ+.08,TEXT("st1"));
 R(21.3,22.4,Z1-T,Z1,TEXT("lock-personnel"));R(33.2,37.6,Z1-T,Z1,TEXT("lock-garage"));
 for(auto& Rect:C)if(Rect.Id.StartsWith(TEXT("eastwing:lock-")))Rect.bEnabled=!M->IsFreeform();
}

bool FS47EastWing::Step(float Dt)
{
 if(!bBuilt&&M&&M->Web&&M->Web->IsLoaded()&&M->World)Init();
 if(bBuilt&&M&&M->Web)
 {
  // Not adopted into the exported hierarchy: shown only while SARO is the loaded area.
  const bool Shown=M->Web->Area()==TEXT("saro");
  if(Shown!=bShown){bShown=Shown;for(const auto& P:Parts)if(USceneComponent* C=P.Get())C->SetVisibility(Shown,true);}
  const bool Free=M->IsFreeform();
  const bool Powered=Free?(M->Freeform&&M->Freeform->HasPowerForReactiveLight()):(M->Story&&M->Story->bPowered);
  if(PropsHelper)PropsHelper->SetPowered(Shown&&Powered);
  for(const auto& P:Parts)if(USceneComponent* C=P.Get())
  {
   if(ULocalLightComponent* L=Cast<ULocalLightComponent>(C)){L->SetVisibility(Shown&&Powered);continue;}
   const FString Name=C->GetName();
   const bool Open=Name==TEXT("EastWing_DoorOpen"),Closed=Name==TEXT("EastWing_DoorClosed")||Name==TEXT("EastWing_GarageClosed");
   if(Open||Closed)
   {
    const bool Visible=Shown&&(Open?Free:!Free);C->SetVisibility(Visible);
    if(UStaticMeshComponent* Mesh=Cast<UStaticMeshComponent>(C)){Mesh->SetCollisionEnabled(Visible?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);Mesh->SetCastShadow(Visible);}
   }
  }
  for(int32 I=0;I<NativeDoors.Num();++I)if(UStaticMeshComponent* C=NativeDoors[I].Get())
  {
   const FVector At=I==0?FVector(21.3,0,5.5):I==1?FVector(35.4,3,5.5):FVector(38.6,0,1.2);
   const FQuat Q=Free?(I==1?S47Three::Euler(PI/2,0,0):S47Three::Euler(0,I==0?PI/2:-PI/2,0)):FQuat::Identity;
   double Matrix[16];S47Three::Compose(At,Q,FVector::OneVector,Matrix);C->SetWorldTransform(S47Three::MatToUE(Matrix));
   C->SetVisibility(Shown);C->SetCollisionEnabled(Shown&&!Free?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
  }
  for(auto& R:M->Web->CollidersMutable(TEXT("saro")))if(R.Id.StartsWith(TEXT("eastwing:lock-")))R.bEnabled=!Free;
 }
 DriveTest();
 // Developer views (-S47EastWingShots): exterior and each room, then exit.
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47EastWingShots"))||!M||!M->Player||!bBuilt||S47Startup::Blocking()||M->Mode!=US47Main::EMode::Play||!M->World||M->World->Area!=TEXT("saro"))return true;
 struct FView{const TCHAR* Name;double X,Z,Yaw,Pitch;};
 static const FView Views[]={
  {TEXT("01-exterior-south"),29.5,19.0,0.,-.04},{TEXT("02-exterior-southwest"),17.5,15.0,-.75,-.03},{TEXT("03-garage-door"),35.4,12.5,0.,-.02},
  {TEXT("04-test-lab"),25.0,1.0,2.87,-.12},{TEXT("05-parts-storage"),27.6,-1.5,-.4,-.08},{TEXT("06-calibration"),28.5,2.0,-2.56,-.12},
  {TEXT("07-garage-in"),35.4,-4.2,PI,-.05},{TEXT("08-garage-out"),35.4,4.6,0.,-.05},{TEXT("09-sample-analysis"),23.5,-1.4,-.12,-.12},{TEXT("10-service-entry"),42.0,1.8,PI/2,-.04},
  {TEXT("11-lab-west"),24.9,.1,PI/2,-.10},{TEXT("12-lab-east"),22.4,-.4,-PI/2,-.02},{TEXT("13-calibration-north"),29.6,4.2,0.,-.04},
  {TEXT("14-storage-west"),29.8,-2.4,PI/2,-.04},{TEXT("15-garage-west"),36.6,1.1,PI/2,-.04},
  {TEXT("16-roof-northeast"),48.0,-14.0,2.23,.04},
  {TEXT("17-garage-receiving"),34.4,-1.6,.9,-.10},
  {TEXT("18-lab-corner-low"),22.2,-3.6,PI*.75,-.35}};
 const double Now=FPlatformTime::Seconds();
 if(ShotStage==0)
 {
  int32 Hour=0;FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
  if(FParse::Value(FCommandLine::Get(),TEXT("S47EastWingHour="),Hour)&&Hour>=0&&Hour<24&&User.StartsWith(TEXT("/tmp/")))
  {
   if(M->IsFreeform()&&M->Freeform)
   {
    const double Delta=FMath::Fmod(Hour*3600.-FMath::Fmod(M->Freeform->Clock(),86400.)+86400.,86400.);
    M->Freeform->TestAdvance(Delta);M->Freeform->SetPowerFault(FParse::Param(FCommandLine::Get(),TEXT("S47EastWingPowerOff")));
   }
   else if(M->Story)M->Story->SetClock(Hour*3600.);
   UE_LOG(LogS47EastWing,Display,TEXT("S47: explicit isolated east wing lighting fixture hour=%d powerFault=%d"),Hour,M->Freeform&&M->Freeform->PowerFault());
  }
  if(User.StartsWith(TEXT("/tmp/"))&&FParse::Param(FCommandLine::Get(),TEXT("S47EastWingSampleFixture"))&&M->IsFreeform()&&M->Freeform&&M->Freeform->Field)
  {
   M->Freeform->Field->AddItem(TEXT("fragment"));
   UE_LOG(LogS47EastWing,Display,TEXT("S47: explicit isolated sample-holder visual fixture"));
  }
  ShotAt=Now+6;ShotStage=1;return true;
 }
 if(Now<ShotAt)return true;
 const int32 I=(ShotStage-1)/2;
 if(I>=int32(UE_ARRAY_COUNT(Views))){UE_LOG(LogS47EastWing,Display,TEXT("S47: east wing shots done"));FPlatformMisc::RequestExit(false);return false;}
 if((ShotStage-1)%2==0){const FView& V=Views[I];M->Player->Place(V.X,V.Z,V.Yaw);M->Player->Pitch=V.Pitch;M->Player->bHeadMotion=false;ShotAt=Now+1.6;}
 else
 {
  const FString Dir=FPaths::ProjectSavedDir()/TEXT("EastWingShots");IFileManager::Get().MakeDirectory(*Dir,true);
  FScreenshotRequest::RequestScreenshot(Dir/(FString(Views[I].Name)+TEXT(".png")),false,false);ShotAt=Now+1.2;
  UE_LOG(LogS47EastWing,Display,TEXT("S47: east wing shot %s floor %.2f"),Views[I].Name,M->Player->FloorY);
 }
 ++ShotStage;return true;
}

bool FS47EastWing::SelfTest(TArray<FString>& Lines)
{
 bool Good=true;double Area=0;for(int32 I=0;I<4;++I){const FBox2D R=Room(I);Good&=R.bIsValid&&R.GetArea()>15;Area+=R.GetArea();}
 Good&=Area>150&&Area<200&&Room(3).GetSize().X>6.5&&Room(3).GetSize().Y>10;
 Lines.Add(FString::Printf(TEXT("%s / East wing: four rooms on the exported footprint, garage wide and deep enough for the Jeep"),Good?TEXT("PASS"):TEXT("FAIL")));
 return Good;
}


void FS47EastWing::DriveTest()
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47EastWingDriveTest"))||DriveStage<0)return;
 US47World* W=M?M->World.Get():nullptr;const double Now=FPlatformTime::Seconds();
 auto Finish=[&](bool Good,const TCHAR* Why)
 {
  if(W)W->TestInput=nullptr;if(M&&M->Player)M->Player->TestMove.Reset();DriveStage=-1;
  DriveChecks.Add(FString(Good?TEXT("PASS / "):TEXT("FAIL / "))+Why);
  const FString Dir=FPaths::ProjectSavedDir()/TEXT("EastWingDriveTest");IFileManager::Get().MakeDirectory(*Dir,true);
  FFileHelper::SaveStringToFile(FString::Join(DriveChecks,TEXT("\n")),*(Dir/TEXT("result.txt")));
  for(const FString& Line:DriveChecks)UE_LOG(LogS47EastWing,Display,TEXT("%s"),*Line);
  FPlatformMisc::RequestExit(false);
 };
 if(DriveStarted==0){DriveStarted=Now;return;}
 if(Now-DriveStarted>100){Finish(false,TEXT("bounded actual Jeep integration timeout"));return;}
 if(S47Startup::Blocking()||!bBuilt||!M||!W||!M->Player||M->Mode!=US47Main::EMode::Play||W->Area!=TEXT("saro"))return;
 if(DriveStage==0)
 {
  if(Now-DriveStarted<5||W->bBusy)return;
  FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
  if(!User.StartsWith(TEXT("/tmp/"))){Finish(false,TEXT("isolated temporary profile required"));return;}
  if(!W->AuditEastWingDrive(DriveChecks)){Finish(false,TEXT("production controller corridor audit"));return;}
  if(!M->IsFreeform())
  {
   int32 Locks=0;for(const FS47Rect& R:M->Web->CollidersMutable(TEXT("saro")))if(R.bEnabled&&R.Id.StartsWith(TEXT("eastwing:lock-")))++Locks;
   bool Closed=NativeDoors.Num()==3;for(const auto& Door:NativeDoors)Closed&=Door.IsValid()&&Door->GetCollisionEnabled()==ECollisionEnabled::QueryOnly;
   Finish(Locks==3&&Closed,TEXT("Story enables three walking entrance locks and closes garage driving"));return;
  }
  M->Player->Place(21.85,9,0);M->Player->TestMove=FVector2D(0,-1);DriveStage=10;return;
 }
 if(DriveStage==10)
 {
  if(M->Player->Pos.Z>4.5)return;
  M->Player->TestMove.Reset();
  if(FMath::Abs(M->Player->FloorY)>.02){Finish(false,TEXT("personnel ramp leads to native walking floor"));return;}
  DriveChecks.Add(TEXT("PASS / Actual walking input crosses personnel apron and door onto lab floor"));
  M->Player->Place(42,1.75,PI/2);M->Player->TestMove=FVector2D(0,-1);DriveStage=11;return;
 }
 if(DriveStage==11)
 {
  if(M->Player->Pos.X>37.5)return;
  M->Player->TestMove.Reset();
  if(FMath::Abs(M->Player->FloorY)>.02){Finish(false,TEXT("east service ramp leads to native walking floor"));return;}
  DriveChecks.Add(TEXT("PASS / Actual walking input crosses east service ramp and door into garage"));
  auto Pose=S47J::New();S47J::Set(Pose,TEXT("at"),TEXT("saro"));S47J::Set(Pose,TEXT("x"),35.4);S47J::Set(Pose,TEXT("y"),-.576);S47J::Set(Pose,TEXT("z"),15.);S47J::Set(Pose,TEXT("h"),0.);
  W->PlaceTruck(TEXT("saro"),Pose);W->Roam(TEXT("saro"));DriveStage=1;return;
 }
 FS47DriveController* D=W->CabDrive();FS47Truck* Jeep=W->CabTruck();
 if(DriveStage==1)
 {
  if(!W->bDriving||!D||!Jeep)return;
  W->TestInput=[] {FS47DriveIn A;A.Throttle=.28;return A;};DriveStage=2;
 }
 if(DriveStage==2&&D&&Jeep)
 {
  if(D->Pos.Z>6&&D->Pos.Z<9&&Jeep->Position.Y>-.55&&Jeep->Position.Y<-.02)bDriveRampSeen=true;
  if(D->Pos.Z<1.){W->TestInput=[W]{FS47DriveIn A;const auto* C=W->CabDrive();A.Throttle=C&&C->Speed>.08?-1.:0.;return A;};DriveStage=3;}
 }
 else if(DriveStage==3&&D&&Jeep&&FMath::Abs(D->Speed)<.1)
 {
  DriveChecks.Add(FString::Printf(TEXT("MEASURE / ramp=%d position=(%.3f %.3f %.3f) half=(%.3f %.3f) lot=%d speed=%.3f"),bDriveRampSeen,Jeep->Position.X,Jeep->Position.Y,Jeep->Position.Z,Jeep->HalfWidth(),Jeep->HalfLength(),W->TruckOnLot(),D->Speed));
  if(!bDriveRampSeen||FMath::Abs(Jeep->Position.Y)>.08){Finish(false,TEXT("actual Jeep ramp height and level garage floor"));return;}
  DriveChecks.Add(TEXT("PASS / Actual Jeep drove from apron over ramp onto garage floor"));
  W->TestInput=nullptr;if(!W->GetOutHere()){Finish(false,TEXT("actual Jeep allows garage exit"));return;}DriveStage=4;return;
 }
 else if(DriveStage==4&&!W->bBusy&&!W->bDriving)
 {
  const auto Saved=W->TruckSave(),Pose=S47J::Obj(Saved,TEXT("pose"));
  if(!Pose||FMath::Abs(S47J::Num(Pose,TEXT("x"))-35.4)>.1||FMath::Abs(S47J::Num(Pose,TEXT("y")))>.08||!W->TruckOnLot()){Finish(false,TEXT("garage parking preserves actual Jeep save pose"));return;}
  DriveChecks.Add(TEXT("PASS / Actual garage exit preserves Jeep pose and parking state"));W->Roam(TEXT("saro"));DriveStage=5;return;
 }
 else if(DriveStage==5&&W->bDriving&&D)
 {
  W->TestInput=[] {FS47DriveIn A;A.Throttle=-.6;return A;};DriveStage=6;
 }
 else if(DriveStage==6&&D&&Jeep&&D->Pos.Z>13)
 {
  Finish(Jeep->Position.Y<-.5,TEXT("actual Jeep reverses out through garage opening to apron"));
 }
}


bool FS47EastWing::BuildDetailed()
{
 const TCHAR* Names[]={TEXT("SM_EW_Shell"),TEXT("SM_EW_Ceiling"),TEXT("SM_EW_Trim"),TEXT("SM_EW_WindowGlass"),TEXT("SM_EW_SignBoard"),TEXT("SM_EW_PersonnelDoor"),TEXT("SM_EW_RollUpDoor"),TEXT("SM_EW_ServiceDoor")};
 TArray<UStaticMesh*> Meshes;
 for(const TCHAR* Name:Names)
 {
  UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/EastWing/Shell/%s.%s"),Name,Name));
  if(!Mesh||Mesh->GetNumLODs()!=3||Mesh->GetNumTriangles(0)<=0)
  {UE_LOG(LogS47EastWing,Warning,TEXT("S47: east wing detailed shell unavailable %s; retaining blockout"),Name);return false;}
  Meshes.Add(Mesh);
 }
 UMaterialInterface* SignArt=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SaroSite/Materials/M_SaroSite_Sign.M_SaroSite_Sign"));
 if(!SignArt)return false;
 AS47Web* W=M->Web;
 for(int32 I=0;I<Meshes.Num();++I)
 {
  UStaticMeshComponent* C=NewObject<UStaticMeshComponent>(W,*FString::Printf(TEXT("EastWingNative_%s"),Names[I]));
  C->SetStaticMesh(Meshes[I]);C->SetMobility(EComponentMobility::Movable);C->SetCastShadow(I!=3);
  C->SetCollisionEnabled(I==0||I==1?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Block);
  if(I==4)for(int32 Slot=0;Slot<Meshes[I]->GetStaticMaterials().Num();++Slot)
   if(Meshes[I]->GetStaticMaterials()[Slot].MaterialSlotName==TEXT("EW_Shell_SignFace"))C->SetMaterial(Slot,SignArt);
  // Photo pass: neutralise the cream cinderblock toward a period off-white (TextureTint from the import recipe).
  for(int32 Slot=0;Slot<Meshes[I]->GetStaticMaterials().Num();++Slot)
   if(Meshes[I]->GetStaticMaterials()[Slot].MaterialSlotName==TEXT("EW_Shell_Masonry"))
    if(UMaterialInterface* Base=Meshes[I]->GetMaterial(Slot))
    {UMaterialInstanceDynamic* Paint=UMaterialInstanceDynamic::Create(Base,C);Paint->SetVectorParameterValue(TEXT("TextureTint"),FLinearColor(.95f,.97f,1.03f));C->SetMaterial(Slot,Paint);}
  // Photo pass: the authored ceiling reads as a dirty brown; repaint deck and ribs in a period off-white.
  if(I==1)for(int32 Slot=0;Slot<Meshes[I]->GetStaticMaterials().Num();++Slot)
  {
   const FString SlotName=Meshes[I]->GetStaticMaterials()[Slot].MaterialSlotName.ToString();
   if(!SlotName.Contains(TEXT("Ceiling")))continue;
   if(UMaterialInterface* Base=Meshes[I]->GetMaterial(Slot))
   {
    UMaterialInstanceDynamic* Paint=UMaterialInstanceDynamic::Create(Base,C);
    Paint->SetVectorParameterValue(TEXT("Color"),SlotName.Contains(TEXT("Steel"))?FLinearColor(.50f,.49f,.45f):FLinearColor(.64f,.62f,.56f));
    C->SetMaterial(Slot,Paint);
   }
  }
  C->SetupAttachment(W->GetRootComponent());C->RegisterComponent();C->SetWorldTransform(FTransform::Identity);Parts.Add(C);
  if(I>=5)NativeDoors.Add(C);
 }
 UE_LOG(LogS47EastWing,Display,TEXT("S47: east wing authored shell mounted: eight meshes, three LODs, retained sign art, native mode-controlled doors"));
 return true;
}


bool FS47EastWing::RelocateAccessDressing()
{
 // Preserve every unrelated atlas triangle and both established forecourt trims.
 UStaticMesh* Dressing=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/EastWing/Access/SM_EW_AccessDressing.SM_EW_AccessDressing"));
 UStaticMesh* Stains=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/EastWing/Access/SM_EW_AccessStains.SM_EW_AccessStains"));
 auto Valid=[](UStaticMesh* Mesh,int32 Count){if(!Mesh||Mesh->GetNumLODs()!=3||Mesh->GetStaticMaterials().Num()!=1)return false;for(int32 I=0;I<3;++I)if(Mesh->GetNumTriangles(I)!=Count)return false;return true;};
 // EASTWING-01: derivatives also drop the old east rain leader (72) and its streak (4) at the service door.
 if(!Valid(Dressing,10256)||!Valid(Stains,1233))return false;
 UStaticMeshComponent *OldDressing=nullptr,*OldStains=nullptr;int32 FoundDressing=0,FoundStains=0;
 for(int32 I=0;I<M->Web->PartCount();++I)if(const FS47WebNode* Node=M->Web->Node(I))
 {
  if(Node->Geo==454){OldDressing=Cast<UStaticMeshComponent>(Node->Comp.Get());++FoundDressing;}
  if(Node->Geo==455){OldStains=Cast<UStaticMeshComponent>(Node->Comp.Get());++FoundStains;}
 }
 if(FoundDressing!=1||FoundStains!=1||!OldDressing||!OldStains||!OldDressing->GetStaticMesh()||!OldStains->GetStaticMesh()
  ||OldDressing->GetStaticMesh()->GetName()!=TEXT("SM_SaroPark_ForecourtTrim")||OldDressing->GetStaticMesh()->GetNumTriangles(0)!=10328
  ||OldStains->GetStaticMesh()->GetNumTriangles(0)!=1224||!OldDressing->GetMaterial(0)||!OldStains->GetMaterial(0))return false;
 const FS47Rect Feet[]={{21.,21.6,6.45,7.05},{21.65,22.25,6.3,6.9},{21.781442517895076,22.818557482104925,7.080414960264715,7.919585039735285}};
 TArray<int32> Matches;auto& All=M->Web->CollidersMutable(TEXT("saro"));
 for(const FS47Rect& Foot:Feet)
 {
  int32 Count=0,Index=-1;for(int32 I=0;I<All.Num();++I)
   if(FMath::IsNearlyEqual(All[I].MinX,Foot.MinX,.0001)&&FMath::IsNearlyEqual(All[I].MaxX,Foot.MaxX,.0001)&&FMath::IsNearlyEqual(All[I].MinZ,Foot.MinZ,.0001)&&FMath::IsNearlyEqual(All[I].MaxZ,Foot.MaxZ,.0001)){++Count;Index=I;}
  if(Count!=1)return false;Matches.Add(Index);
 }
 UMaterialInterface* DressingAtlas=OldDressing->GetMaterial(0);UMaterialInterface* StainAtlas=OldStains->GetMaterial(0);
 OldDressing->SetStaticMesh(Dressing);OldDressing->SetMaterial(0,DressingAtlas);OldStains->SetStaticMesh(Stains);OldStains->SetMaterial(0,StainAtlas);
 for(int32 I:Matches){All[I].MinX-=2.6;All[I].MaxX-=2.6;}
 UE_LOG(LogS47EastWing,Display,TEXT("S47: east wing access: three drums and contact shadows moved west2.6m with exact walking feet; 10256/1233-triangle dressing retained (old east rain leader removed; grime clipped at service aperture)"));return true;
}
