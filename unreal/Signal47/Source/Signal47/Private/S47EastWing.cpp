#include "S47EastWing.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47Story.h"
#include "S47Web.h"
#include "S47Obj.h"
#include "S47Three.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Props.h"
#include "S47Flow.h"
#include "S47Drive.h"
#include "S47SaroSite.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
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
 Build();bBuilt=true;
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

void FS47EastWing::Light(const FString& Name,const FVector& At,float Lumens,const FLinearColor& Colour)
{
 AS47Web* W=M->Web;UPointLightComponent* L=NewObject<UPointLightComponent>(W,*FString::Printf(TEXT("EastWingLight_%s"),*Name));
 L->SetMobility(EComponentMobility::Movable);L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(Lumens);L->SetLightColor(Colour);L->SetAttenuationRadius(900);L->SetSourceRadius(40);L->SetSoftSourceRadius(60);
 L->SetupAttachment(W->GetRootComponent());L->RegisterComponent();L->SetWorldLocation(S47Three::ToUE(At));Parts.Add(L);
}

void FS47EastWing::Build()
{
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
 Light(TEXT("Lab"),FVector(23.3,3.0,0),3200,Tube);Light(TEXT("Store"),FVector(28.8,3.0,-2.5),2200,Tube);
 Light(TEXT("Cal"),FVector(28.8,3.0,3.0),2200,Tube);Light(TEXT("Garage"),FVector(35.1,3.0,0),4200,Tube);
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
 R(X0,X1,Z0,Z0+T,TEXT("n"));R(X0,X0+T,Z0,Z1,TEXT("w"));R(X1-T,X1,Z0,Z1,TEXT("e"));
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
  for(const auto& P:Parts)if(USceneComponent* C=P.Get())
  {
   if(UPointLightComponent* L=Cast<UPointLightComponent>(C)){L->SetVisibility(Shown&&Powered);continue;}
   const FString Name=C->GetName();
   const bool Open=Name==TEXT("EastWing_DoorOpen"),Closed=Name==TEXT("EastWing_DoorClosed")||Name==TEXT("EastWing_GarageClosed");
   if(Open||Closed)
   {
    const bool Visible=Shown&&(Open?Free:!Free);C->SetVisibility(Visible);
    if(UStaticMeshComponent* Mesh=Cast<UStaticMeshComponent>(C)){Mesh->SetCollisionEnabled(Visible?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);Mesh->SetCastShadow(Visible);}
   }
  }
  for(auto& R:M->Web->CollidersMutable(TEXT("saro")))if(R.Id.StartsWith(TEXT("eastwing:lock-")))R.bEnabled=!Free;
 }
 // Developer views (-S47EastWingShots): exterior and each room, then exit.
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47EastWingShots"))||!M||!M->Player||!bBuilt||M->Mode!=US47Main::EMode::Play||!M->World||M->World->Area!=TEXT("saro"))return true;
 struct FView{const TCHAR* Name;double X,Z,Yaw,Pitch;};
 static const FView Views[]={
  {TEXT("01-exterior-south"),29.5,19.0,0.,-.04},{TEXT("02-exterior-southwest"),17.5,15.0,-.75,-.03},{TEXT("03-garage-door"),35.4,12.5,0.,-.02},
  {TEXT("04-test-lab"),24.8,4.4,.9,-.08},{TEXT("05-parts-storage"),30.6,-.2,1.9,-.08},{TEXT("06-calibration"),30.8,4.6,1.3,-.1},
  {TEXT("07-garage-in"),35.4,-4.2,PI,-.05},{TEXT("08-garage-out"),35.4,4.6,0.,-.05}};
 const double Now=FPlatformTime::Seconds();
 if(ShotStage==0){ShotAt=Now+6;ShotStage=1;return true;}
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
