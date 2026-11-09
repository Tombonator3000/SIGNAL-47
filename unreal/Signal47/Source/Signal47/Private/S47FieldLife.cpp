#include "S47FieldLife.h"
#include "S47Audio.h"
#include "S47Drive.h"
#include "S47Album.h"
#include "S47FarmSite.h"
#include "S47FreeformDirector.h"
#include "S47Glimpses.h"
#include "S47Night.h"
#include "S47RoswellGroup.h"
#include "S47Taken.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47Legs.h"
#include "S47Main.h"
#include "S47NightRhythm.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47RoadShape.h"
#include "S47RoadTerrain.h"
#include "S47EventPlaces.h"
#include "S47FarmWorld.h"
#include "S47Geo.h"
#include "S47EngineSound.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Weather.h"
#include "S47Web.h"
#include "S47World.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "HAL/IConsoleManager.h"
#include "CoreGlobals.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "UObject/GarbageCollection.h"

// ---------- the truck
namespace S47TruckCareLocal
{
 TAutoConsoleVariable<int32> CvSandPatches(TEXT("s47.SandPatches"),1,TEXT("Freeform: pale sand lies in the dry wash where the truck can bog down (1), or the wash is left as the ground has it (0)."));
 const TCHAR* const SandLit=TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque");
 const TCHAR* const SandBall=TEXT("/Engine/BasicShapes/Sphere.Sphere");
 // how far round the player the pale sand is laid, and how far the player goes before it is laid again
 constexpr double SandReach=240,SandRelay=70;
 // the wheel's own turn: a flat thumps once a turn
 constexpr double WheelTurn=2.3;
}
double FS47TruckCare::SandAt(double RoadX,double RoadZ)
{
 if(!FMath::IsFinite(RoadX)||!FMath::IsFinite(RoadZ))return 0;
 // the wash's bed (the trough the land has, S47RoadShape::WashAt): soft in its middle, firm up its banks
 double Soft=FMath::Clamp((S47RoadShape::WashAt(RoadX,RoadZ)-.25)/.6,0.,1.);
 // and the dry washes out on the land (S47EventPlaces Wash): a soft bed round each, as far as it spreads
 static const struct { double X,Z,R; } Beds[]={{-900,1150,26},{-650,1650,26},{750,1100,26},{-560,1340,12}};
 for(const auto& Bed:Beds)
 {
  const double D=FVector2D::Distance(FVector2D(RoadX,RoadZ),FVector2D(Bed.X,Bed.Z));
  if(D<Bed.R)Soft=FMath::Max(Soft,1.-FMath::SmoothStep(Bed.R*.35,Bed.R,D));
 }
 return Soft;
}
double FS47TruckCare::BogRate(double Soft,double Load,double Mps,bool bReverse)
{
 const double Dp=FMath::Clamp(Soft,0.,1.),Ld=FMath::Clamp(Load,0.,1.),Sp=FMath::IsFinite(Mps)?FMath::Abs(Mps):0.;
 if(Dp<.15)return -.5;            // out of it
 if(bReverse)return -.6;          // back out in its own tracks
 if(Ld<.25)return -.3;            // eased off: the wheels stop digging
 if(Sp>=7)return .1*Ld*Dp-.2;     // way on: it floats over
 // driven hard and slow in deep sand: the wheels dig in, three or four seconds to the axle
 return .3*Ld*Dp*(.5+(7-Sp)/14.);
}
double FS47TruckCare::FlatChance(double Mps,ES47SurfaceKind Kind)
{
 if(Kind==ES47SurfaceKind::Asphalt||!FMath::IsFinite(Mps))return 0;
 const double Fast=FMath::Square(FMath::Clamp(FMath::Abs(Mps)/12.,0.,1.));
 return (Kind==ES47SurfaceKind::Dirt?.0035:.0012)*Fast;
}
void FS47TruckCare::Begin(const S47J::FObj& Saved)
{
 WaterLeft=1;Metres=0;WeekMetres=0;LegMetres=0;bStuck=bFlat=bWarned=bPowerSet=bHeldSet=false;Bog=0;bSandHeld=false;bHissed=false;
 if(!Saved.IsValid())return;
 const double Week=S47J::Num(Saved,TEXT("weekMetres"));WeekMetres=FMath::IsFinite(Week)?FMath::Clamp(Week,0.,1e7):0;
 const double W=S47J::Num(Saved,TEXT("water"),1);WaterLeft=FMath::IsFinite(W)?FMath::Clamp(W,0.,1.):1;
 bStuck=S47J::Bool(Saved,TEXT("stuck"));bFlat=S47J::Bool(Saved,TEXT("flat"));
 const double Leg=S47J::Num(Saved,TEXT("legMetres"));LegMetres=FMath::IsFinite(Leg)?FMath::Clamp(Leg,0.,1e6):0;
}
S47J::FObj FS47TruckCare::Save() const
{
 auto O=S47J::New();
 if(WaterLeft<1)S47J::Set(O,TEXT("water"),WaterLeft);
 if(bStuck)S47J::Set(O,TEXT("stuck"),true);
 if(bFlat)S47J::Set(O,TEXT("flat"),true);
 if(WeekMetres>=1)S47J::Set(O,TEXT("weekMetres"),FMath::RoundToDouble(WeekMetres));
 if(LegMetres>=1)S47J::Set(O,TEXT("legMetres"),FMath::RoundToDouble(LegMetres));
 return O;
}
FString FS47TruckCare::StatusLine() const
{
 if(bStuck)return TEXT("Sunk to the axle in sand. The spade is behind the seat.");
 if(bFlat)return TEXT("Flat tire. Jack and lug wrench behind the seat.");
 if(Hot())return TEXT("The radiator is dry and the needle is in the red. It will crawl, no more.");
 return WaterLeft<.25?TEXT("The temperature needle is climbing. It wants water soon."):FString();
}
FString FS47TruckCare::Steps(int32 Spares,int32 Waters) const
{
 if(!NeedsRescue())return FString();
 FString T;
 if(bStuck)
 {
  T+=TEXT("STUCK IN SAND. The back wheels have dug in; driving only digs deeper.\n");
  T+=TEXT("1. Get out of the truck (the exit key; the truck cannot move).\n");
  T+=FString::Printf(TEXT("2. Stand by the truck, look at the cargo rack on the back and use it: SERVICE TRUCK / CARGO opens.\n3. Choose \"Dig it out with the spade\" (the spade is behind the seat, about %.0f minutes%s).\n"),DigTime(),DigTime()<DigMinutes?TEXT(", with the come-along"):TEXT(""));
 }
 if(bFlat)
 {
  T+=Spares>0?TEXT("FLAT TIRE. Jack and lug wrench are behind the seat; the spare wheel is in your pouch (you have it).\n")
              :TEXT("FLAT TIRE, AND NO SPARE WHEEL. The jack is no use without one. Buy a spare at the Caliche gas station ($15), or call the county tow.\n");
  T+=FString::Printf(TEXT("%sGet out, use the cargo rack on the truck and choose \"Jack it up and change the wheel\" (about %.0f minutes).\n"),bStuck?TEXT("Then: "):TEXT("1. "),WheelMinutes);
 }
 if(Hot())
 {
  T+=Waters>0?TEXT("RADIATOR DRY. Get out, use the cargo rack and choose \"Fill the radiator from a can\" (you have water in the pouch). Let it cool first.\n")
              :TEXT("RADIATOR DRY AND NO WATER. A can of radiator water is $1 at the Caliche gas station. The truck will crawl there, no faster, or call the county tow.\n");
 }
 T+=FString::Printf(TEXT("Cannot be mended? Use the cargo rack and choose \"Call the county tow\": $%d (cash, or on the weekly sheet), about %.0f minutes, and the truck is left sound at SARO."),TowFee(),TowMinutes());
 return T;
}
void FS47TruckCare::Hold()
{
 auto* D=M->World->CabDrive();if(!D)return;
 // Bogged it does not move; flat it limps; dry it crawls. Always something it can still do, or a way to get help.
 const bool bPower=bFlat||Hot();
 if(bPower){D->Power=Hot()?.28:.65;bPowerSet=true;}
 else if(bPowerSet){D->Power=1;bPowerSet=false;}
 if(bStuck){M->World->HoldTruck(25.);bHeldSet=true;}
 else if(bHeldSet){M->World->HoldTruck(0);bHeldSet=false;}
}
void FS47TruckCare::StepGround(double Dt,FS47DriveController& D,double RoadX,double RoadZ)
{
 FS47EngineSound* Engine=M->World->EngineSound();
 const double Going=FMath::Abs(D.Speed);
 // ---- the sand: it drags and tugs the wheel; driven hard and slow the wheels dig in, and it tells before it sinks ----
 // (WEATHER-20261108: on a sand day it drifts across the dirt tracks, less on gravel, never on the asphalt)
 const S47Weather::FLive& Wx=S47Weather::Live();
 const double Drift=Wx.bOn?Wx.Dust*(D.Surface.Kind==ES47SurfaceKind::Dirt?.18:.08):0.;
 const double Soft=D.Surface.Kind==ES47SurfaceKind::Asphalt?0.:FMath::Min(1.,SandAt(RoadX,RoadZ)+Drift);
 // a strong wind pushes the truck off its line at speed, in gusts, the way it blows
 if(Wx.bOn&&Wx.Wind>.15&&Going>4)
 {
  GustT+=Dt;
  const double Gusting=.55+.45*FMath::Sin(GustT*.9)*FMath::Sin(GustT*.37+1.);
  const double Toward=-FMath::Cos(D.Heading)*Wx.DirX+FMath::Sin(D.Heading)*Wx.DirZ; // which way turning goes with the wind
  D.Heading+=Dt*Wx.Wind*.03*Gusting*FMath::Sign(Toward)*FMath::Min(1.,FMath::Abs(Toward)*2.)*FMath::Min(1.,Going/15.);
 }
 if(!bStuck)
 {
  Bog=FMath::Clamp(Bog+Dt*BogRate(Soft,D.Load,D.Speed,D.Reversing()),0.,1.);
  if(Soft>.05){D.Held=1.8*Soft;bSandHeld=true;}
  else if(bSandHeld){D.Held=0;bSandHeld=false;}
  // the wheel tugs this way and that in the soft ground
  if(Soft>.1&&Going>.5){TugT+=Dt;D.Heading+=Dt*Soft*.1*FMath::Sin(TugT*1.7)*FMath::Min(1.,Going/6.);}
  if(Bog>.2)
  {
   // the wheels spin: sand thrown up under the arches, the engine labouring, oftener the deeper it digs
   if((SpinIn-=Dt)<=0)
   {
    SpinIn=FMath::Lerp(.7,.3,Bog);
    if(M->Audio)M->Audio->Sfx(TEXT("gust"),FS47Sound{.Gain=float(.12+.3*Bog),.Rate=1.7f,.Lp=900.f,.Dur=.35f});
    if(Engine&&Bog>.45&&FMath::FRand()<.45f)Engine->Cough(.25+.3*Bog);
   }
   if(!bSandHinted&&Bog>.4&&M->UI){bSandHinted=true;M->UI->Toast(TEXT("Soft sand. Ease off, or back out."),4);}
  }
  if(Bog>=1)
  {
   bStuck=true;Bog=1;if(bSandHeld){bSandHeld=false;}Hold();
   if(M->Audio)M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.5f,.Rate=.6f});
   if(M->UI)M->UI->Toast(TEXT("The back end settles. Stuck to the axle in sand."),6);
   UE_LOG(LogTemp,Display,TEXT("S47: truck: bogged in sand at %.0f, %.0f (depth %.2f)"),RoadX,RoadZ,Soft);M->RequestAutosave();
  }
 }
 // ---- a flat: it pulls to one side and thumps once a turn of the wheel ----
 if(bFlat&&Going>1)
 {
  D.Heading+=Dt*PullSide*.06*FMath::Min(1.,Going/12.);
  if((ThumpIn-=Dt)<=0)
  {
   ThumpIn=FMath::Max(.15,S47TruckCareLocal::WheelTurn/Going);
   if(M->Audio)M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=float(FMath::Clamp(.12+Going*.02,.12,.4)),.Rate=.75f});
  }
 }
 // ---- the radiator running low: it pings now and then, and hisses once before it boils dry ----
 if(!Hot()&&WaterLeft<.12&&Engine)
 {
  if((PingIn-=Dt)<=0){PingIn=FMath::FRandRange(18.f,40.f);Engine->Cough(.2);}
  if(WaterLeft<.05&&!bHissed){bHissed=true;if(M->Audio)M->Audio->Sfx(TEXT("gust"),FS47Sound{.Gain=.2f,.Rate=2.2f,.Lp=4000.f,.Dur=1.4f});}
 }
}
void FS47TruckCare::Step(double Dt)
{
 if(!M||!M->Freeform||!M->World||!M->Freeform->RulesLive())return;
 ShowSand();
 if(!M->World->bDriving){if(M->World->TruckAt==TEXT("saro"))LegMetres=0;Bog=0;return;} // parked at SARO: the next tow is priced from there
 auto* D=M->World->CabDrive();if(!D)return;
 // (WEATHER-20261108: after a shower the dirt is slick for a few hours, the gravel a little, the asphalt hardly)
 {
  const S47Weather::FLive& Wx=S47Weather::Live();
  const double Wet=Wx.bOn?FMath::Clamp(Wx.Wet,0.,1.):0.;
  D->GripScale=1.-Wet*(D->Surface.Kind==ES47SurfaceKind::Dirt?.35:D->Surface.Kind==ES47SurfaceKind::Gravel?.2:.06);
 }
 Hold();
 if(!D->bHandling)return; // The autopilot's road trips have none of this.
 double RoadX=0,RoadZ=0;
 const bool bOnLand=M->Web&&S47OpenLand::RoadOf(M->Web->Area(),D->Pos.X,D->Pos.Z,RoadX,RoadZ);
 if(bOnLand)StepGround(FMath::Min(Dt,.2),*D,RoadX,RoadZ);
 const double Step=FMath::Abs(D->Speed)*FMath::Min(Dt,.2);
 if(Step<=0)return;
 const bool bWasHot=Hot();
 WaterLeft=FMath::Max(0.,WaterLeft-Step/WaterRange);
 WeekMetres+=Step;
 if(WaterLeft<.25&&!bWarned){bWarned=true;if(M->UI)M->UI->Toast(StatusLine(),5);}
 if(!bWasHot&&Hot())
 {
  if(M->UI)M->UI->Toast(StatusLine(),6);
  UE_LOG(LogTemp,Display,TEXT("S47: truck: radiator dry"));M->RequestAutosave();
 }
 LegMetres+=Step;
 Metres+=Step;if(Metres<100)return;Metres=0;
 if(bStuck||bFlat)return;
 // a tyre goes on rough ground taken fast: never on the blacktop, seldom at a crawl (no dice for the sand: that is StepGround's)
 if(FMath::FRand()<FlatChance(D->Speed,D->Surface.Kind))
 {
  bFlat=true;PullSide=FMath::RandBool()?1.:-1.;ThumpIn=.4;
  if(M->Audio)M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=1.f,.Rate=.5f});
  if(auto* Engine=M->World->EngineSound())Engine->Thump(FMath::Min(6.,FMath::Abs(D->Speed)*.5));
  if(M->UI)M->UI->Toast(TEXT("A bang under the truck, and a wheel starts to thump."),5);
  UE_LOG(LogTemp,Display,TEXT("S47: truck: flat tyre at %.1f m/s"),FMath::Abs(D->Speed));M->RequestAutosave();
 }
}
void FS47TruckCare::ShowSand()
{
 using namespace S47TruckCareLocal;
 if(!M||!M->Player||!M->Web||!IsValid(M->Web.Get()))return;
 UInstancedStaticMeshComponent* Shown=SandShown.Get();
 const FString Area=M->Web->Area();
 double Rx=0,Rz=0;
 const bool bArea=Area==TEXT("road")||Area==TEXT("saro")||Area==TEXT("diner")||Area==TEXT("motel");
 const bool bWant=CvSandPatches.GetValueOnGameThread()!=0&&M->IsFreeform()&&bArea&&S47OpenLand::RoadOf(Area,M->Player->Pos.X,M->Player->Pos.Z,Rx,Rz);
 // nothing to lay where neither the wash nor a bed is within reach
 const double ToWash=FMath::Abs(Rx+330+.25*(Rz-500))/FMath::Sqrt(1.0625);
 bool bBedNear=false;
 for(const FVector2D& Bed:{FVector2D(-900,1150),FVector2D(-650,1650),FVector2D(750,1100),FVector2D(-560,1340)})bBedNear|=FVector2D::Distance(Bed,FVector2D(Rx,Rz))<SandReach+30;
 if(!bWant||(ToWash>SandReach+20&&!bBedNear)){if(Shown&&Shown->IsVisible())Shown->SetVisibility(false);SandArea.Empty();return;}
 if(Shown&&SandScene.Get()==M->Web.Get()&&Area==SandArea&&FVector2D::Distance(SandCentre,FVector2D(Rx,Rz))<SandRelay){if(!Shown->IsVisible())Shown->SetVisibility(true);return;}
 if(!Shown||SandScene.Get()!=M->Web.Get())
 {
  UStaticMesh* Dome=LoadObject<UStaticMesh>(nullptr,SandBall);UMaterialInterface* Lit=LoadObject<UMaterialInterface>(nullptr,SandLit);
  if(!Dome||!Lit){SandArea=Area;SandCentre=FVector2D(Rx,Rz);return;}
  AS47Web* Web=M->Web.Get();
  auto* Ism=NewObject<UInstancedStaticMeshComponent>(Web,TEXT("ff:sand"));
  Ism->SetupAttachment(Web->GetRootComponent());Ism->SetMobility(EComponentMobility::Movable);Ism->SetStaticMesh(Dome);
  UMaterialInstanceDynamic* Mat=UMaterialInstanceDynamic::Create(Lit,Web);
  Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.60f,.53f,.42f));Mat->SetScalarParameterValue(TEXT("Roughness"),1.f);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1.f);
  Ism->SetMaterial(0,Mat);Ism->SetCastShadow(false);Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision);Ism->SetCanEverAffectNavigation(false);
  Ism->RegisterComponent();
  SandShown=Ism;SandScene=M->Web;Shown=Ism;
 }
 SandArea=Area;SandCentre=FVector2D(Rx,Rz);
 Shown->ClearInstances();
 // A low pale bank of sand laid on the ground in each soft place, a patch every few metres, each lying along the slope there.
 auto Lay=[&](double X,double Z,double Soft,int32 Nth)
 {
  FVector OnGround=S47OpenLand::FromRoad(Area,X,Z);
  const double Y=S47FarmWorld::FloorRoad(X,Z);
  const double Ex=(S47FarmWorld::FloorRoad(X+1,Z)-S47FarmWorld::FloorRoad(X-1,Z))*.5,Ez=(S47FarmWorld::FloorRoad(X,Z+1)-S47FarmWorld::FloorRoad(X,Z-1))*.5;
  if(!FMath::IsFinite(Y)||!FMath::IsFinite(Ex)||!FMath::IsFinite(Ez)||FMath::Abs(Ex)>.25||FMath::Abs(Ez)>.25)return;
  OnGround.Y=Y-.035;
  const FVector Up=S47Three::DirToUE(FVector(-Ex,1,-Ez).GetSafeNormal());
  const uint32 Hashed=uint32(Nth)*2654435761u;
  const double Yaw=double(Hashed%360u),Long=FMath::Lerp(5.0,7.5,double((Hashed>>9)%100u)/100.0)*(.7+.3*Soft),Wide=FMath::Lerp(3.5,5.0,double((Hashed>>17)%100u)/100.0)*(.7+.3*Soft);
  const FQuat Turn=FQuat::FindBetweenNormals(FVector::UpVector,Up)*FQuat(FVector::UpVector,FMath::DegreesToRadians(Yaw));
  Shown->AddInstance(FTransform(Turn,S47Three::ToUE(OnGround),FVector(Long,Wide,.1)),true);
 };
 int32 Laid=0;
 for(double Z=Rz-SandReach;Z<=Rz+SandReach;Z+=5)
 {
  const double Mid=-330-.25*(Z-500);
  for(double Across=-12;Across<=12;Across+=6)
  {
   const double X=Mid+Across;
   if(FVector2D::Distance(FVector2D(X,Z),FVector2D(Rx,Rz))>SandReach)continue;
   const double Soft=SandAt(X,Z);
   if(Soft>=.35)Lay(X,Z,Soft,++Laid);
  }
 }
 // the beds of the washes out on the land, where one is near
 static const FVector2D Beds[]={FVector2D(-900,1150),FVector2D(-650,1650),FVector2D(750,1100),FVector2D(-560,1340)};
 for(const FVector2D& Bed:Beds)
 {
  if(FVector2D::Distance(Bed,FVector2D(Rx,Rz))>SandReach+30)continue;
  for(int32 K=0;K<9;++K)
  {
   const double A=K*.7,Ring=K==0?0.:6.+2.*(K%3);
   const FVector2D At=Bed+FVector2D(FMath::Cos(A),FMath::Sin(A))*Ring;
   const double Soft=SandAt(At.X,At.Y);
   if(Soft>=.35)Lay(At.X,At.Y,Soft,++Laid);
  }
 }
 Shown->SetVisibility(Shown->GetInstanceCount()>0);
}
double FS47TruckCare::DigTime() const
{
 return M&&M->Freeform&&M->Freeform->InventoryCount(TEXT("rope"))>0?RopeDigMinutes:DigMinutes;
}
bool FS47TruckCare::DigOut()
{
 if(!bStuck||!M||!M->Freeform||!M->World)return false;
 const bool bRope=M->Freeform->InventoryCount(TEXT("rope"))>0;const double Minutes=DigTime();
 bStuck=false;Bog=0;M->Freeform->SpendTime(Minutes*60);if(bHeldSet){M->World->HoldTruck(0);bHeldSet=false;}
 if(M->UI)M->UI->Toast(bRope?FString::Printf(TEXT("%.0f minutes: the spade, then the come-along on a fence post, a click at a time. It comes out."),Minutes):FString::Printf(TEXT("%.0f minutes with the spade and some brush under the wheels. It comes out."),Minutes),6);
 UE_LOG(LogTemp,Display,TEXT("S47: truck: dug out"));M->RequestAutosave();return true;
}
bool FS47TruckCare::ChangeWheel()
{
 if(!bFlat||!M||!M->Freeform)return false;
 if(!M->Freeform->Consume(TEXT("spare"))){if(M->UI)M->UI->Toast(TEXT("The spare is already on. The gas station sells them, or call the county."),5);return false;}
 bFlat=false;M->Freeform->SpendTime(WheelMinutes*60);
 if(M->UI)M->UI->Toast(TEXT("Jack, lug wrench, spare. The flat goes in the back."),5);
 UE_LOG(LogTemp,Display,TEXT("S47: truck: wheel changed"));M->RequestAutosave();return true;
}
bool FS47TruckCare::TopUp()
{
 if(WaterLeft>=1||!M||!M->Freeform||!M->Freeform->Consume(TEXT("water")))return false;
 WaterLeft=1;bWarned=false;bHissed=false;
 if(M->UI)M->UI->Toast(TEXT("You fill the radiator from the can and wait for it to stop ticking."),5);
 UE_LOG(LogTemp,Display,TEXT("S47: truck: radiator topped up"));M->RequestAutosave();return true;
}
void FS47TruckCare::Clear(){bStuck=bFlat=false;Bog=0;bHissed=false;WaterLeft=1;LegMetres=0;bWarned=false;if(M&&M->World){if(bHeldSet)M->World->HoldTruck(0);if(auto* D=M->World->CabDrive();D&&bPowerSet)D->Power=1;}bHeldSet=bPowerSet=false;}
bool FS47TruckCare::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s truck care: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 FS47TruckCare C(nullptr);C.Begin(nullptr);
 Check(C.Save()->Values.Num()==0&&!C.NeedsRescue()&&C.StatusLine().IsEmpty(),TEXT("an older save has a sound truck and writes nothing"));
 C.TestStick();C.TestFlat();C.TestWater(0);
 Check(C.NeedsRescue()&&C.Hot()&&!C.StatusLine().IsEmpty(),TEXT("trouble says what it is"));
 FS47TruckCare R(nullptr);R.Begin(S47J::Read(S47J::Write(C.Save())));Check(R.Stuck()&&R.Flat()&&R.Water()==0,TEXT("trouble survives a Continue"));
 R.Clear();Check(!R.NeedsRescue()&&R.Water()==1,TEXT("roadside service leaves it sound"));
 auto Bad=S47J::New();S47J::Set(Bad,TEXT("water"),9.);FS47TruckCare X(nullptr);X.Begin(Bad);Check(X.Water()==1,TEXT("nonsense in a save is held in range"));
 // The strand explains itself and the tow is priced by distance, never above sixty dollars or below thirty-five.
 FS47TruckCare T(nullptr);T.Begin(nullptr);Check(T.Steps(1,2).IsEmpty(),TEXT("a sound truck has no steps to show"));
 T.TestStick();T.TestFlat();
 const FString WithSpare=T.Steps(1,2),NoSpare=T.Steps(0,2);
 Check(WithSpare.Contains(TEXT("spade"))&&WithSpare.Contains(TEXT("jack"))&&WithSpare.Contains(TEXT("spare wheel"))&&WithSpare.Contains(TEXT("Get out"))&&WithSpare.Contains(TEXT("county tow")),TEXT("a stranded truck says what is wrong, what it needs and where it is"));
 Check(NoSpare.Contains(TEXT("NO SPARE"))&&NoSpare.Contains(TEXT("$15")),TEXT("a flat with no spare says where to get one"));
 Check(FS47TruckCare::TowFeeFor(0)==35&&FS47TruckCare::TowFeeFor(4000)==45&&FS47TruckCare::TowFeeFor(1e6)==60&&FS47TruckCare::TowFeeFor(-5)==35,TEXT("the tow costs 35 to 60 dollars by distance"));
 Check(FS47TruckCare::TowMinutesFor(0)==45.&&FS47TruckCare::TowMinutesFor(1e6)==90.,TEXT("the tow takes 45 to 90 minutes"));
 T.TestLeg(10000);FS47TruckCare L(nullptr);L.Begin(S47J::Read(S47J::Write(T.Save())));
 Check(L.TowFee()==60&&FMath::IsNearlyEqual(L.TowMinutes(),75.),TEXT("the distance from SARO survives a Continue"));
 L.Clear();Check(L.TowFee()==35,TEXT("a truck back at SARO starts the distance again"));
 // M2 critic: the sand is where the land is soft, and it tells before it holds; a tyre goes on rough ground taken fast.
 {
  const double Bed=SandAt(-330,500),Bank=SandAt(-330+30,500),Hard=SandAt(200,200),Road=SandAt(0,1500);
  Check(Bed>.9&&Bank<Bed&&Hard==0&&Road==0&&SandAt(-900,1150)>.9&&SandAt(-900+40,1150)==0,TEXT("sand only in the dry wash's bed and the washes out on the land, nowhere else"));
  Check(BogRate(1,1,2,false)>0&&BogRate(1,1,2,false)<.35&&BogRate(.1,1,2,false)<0&&BogRate(1,1,2,true)<0&&BogRate(1,.1,2,false)<0&&BogRate(1,.8,12,false)<0,
   TEXT("driven hard and slow in deep sand the wheels dig in over a few seconds; easing off, keeping way on or backing out takes it back"));
  // from firm, full throttle at a crawl in the deepest sand: how long before it is stuck, and how long it is heard first
  double Dug=0,Took=0,Heard=-1;
  while(Dug<1&&Took<30){Dug=FMath::Clamp(Dug+.05*BogRate(1,1,2,false),0.,1.);Took+=.05;if(Heard<0&&Dug>.2)Heard=Took;}
  Check(Took>2.5&&Took<6&&Heard>0&&Took-Heard>2,TEXT("the wheels are heard spinning a couple of seconds before it sinks"));
  Check(FlatChance(20,ES47SurfaceKind::Asphalt)==0&&FlatChance(3,ES47SurfaceKind::Dirt)<FlatChance(12,ES47SurfaceKind::Dirt)&&FlatChance(12,ES47SurfaceKind::Gravel)<FlatChance(12,ES47SurfaceKind::Dirt),
   TEXT("never a flat on the blacktop; on rough ground oftener the faster it is taken"));
  // a working week: some twenty kilometres of dirt at about ten metres a second
  const double Week=200*FlatChance(10,ES47SurfaceKind::Dirt);
  Check(Week>.2&&Week<1,TEXT("about one flat in a week or two of ordinary field driving"));
  FS47TruckCare B(nullptr);B.Begin(nullptr);B.TestBog(.8);B.Begin(nullptr);
  Check(B.Bogging()==0&&FMath::IsNearlyEqual(B.TempNeedle(),.5),TEXT("a Continue starts on firm ground; the needle sits in the middle with a full radiator"));
  B.TestWater(.02);Check(B.TempNeedle()>.9,TEXT("the needle climbs as the radiator runs dry"));
 }
 return OK;
}

// ---------- the trip camera
namespace
{
 FVector TripWorld(const FVector& At){return At+FVector(0,.3,0);}
}
void FS47TripCamera::Begin(const S47J::FObj& Saved)
{
 Stop();Life=MakeShared<uint8>(0);bOut=false;Caught=ECatch::None;CaughtClock=-1;PhotoId.Empty();LookIn=0;
 if(Saved.IsValid()&&S47J::Bool(Saved,TEXT("out")))
 {
  const auto A=S47J::Nums(Saved,TEXT("at"));const double Clock=S47J::Num(Saved,TEXT("clock"));Area=S47J::Str(Saved,TEXT("area"));Title=S47J::Str(Saved,TEXT("title"));
  if(A.Num()==3&&FMath::IsFinite(Clock)&&!Area.IsEmpty())
  {
   At=FVector(A[0],A[1],A[2]);SetClock=Clock;bAtMarksThen=S47J::Bool(Saved,TEXT("marks"));bOut=true;
   Caught=ECatch(FMath::Clamp(S47J::Int(Saved,TEXT("caught")),0,2));CaughtClock=S47J::Num(Saved,TEXT("caughtClock"),-1);PhotoId=S47J::Str(Saved,TEXT("photo"));
  }
 }
 if(M&&IsValid(M->Web.Get())&&Scene.Get()!=M->Web.Get()){bBuilt=false;PropSaro.Reset();PropRoad.Reset();}
 if(M&&IsValid(M->Web.Get()))Build();
 Layout();
}
void FS47TripCamera::Stop(){Life.Reset();}
S47J::FObj FS47TripCamera::Save() const
{
 auto O=S47J::New();if(!bOut)return O;
 S47J::Set(O,TEXT("out"),true);S47J::Set(O,TEXT("area"),Area);S47J::Set(O,TEXT("title"),Title);S47J::Set(O,TEXT("clock"),SetClock);S47J::Set(O,TEXT("marks"),bAtMarksThen);S47J::Set(O,TEXT("at"),TArray<double>{At.X,At.Y,At.Z});
 if(Caught!=ECatch::None){S47J::Set(O,TEXT("caught"),int32(Caught));S47J::Set(O,TEXT("caughtClock"),CaughtClock);}
 if(!PhotoId.IsEmpty())S47J::Set(O,TEXT("photo"),PhotoId);
 return O;
}
bool FS47TripCamera::Build()
{
 if(bBuilt)return true;
 if(!M||!IsValid(M->Web.Get())||!IsValid(M->Inter))return false;
 const int32 SaroRoot=M->Web->Handle(TEXT("ext.group")),RoadRoot=M->Web->Handle(TEXT("world.legs.road.area.group"));if(SaroRoot<0||RoadRoot<0)return false;
 auto* Box=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));if(!Box||!Base)return false;
 const auto Prop=[&](int32 Root,const TCHAR* Name,FS47Obj& Out)
 {
  auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web.Get());Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.14f,.13f,.1f));Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
  auto* C=NewObject<UStaticMeshComponent>(M->Web.Get(),Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(M->Web->GetRootComponent());C->SetStaticMesh(Box);C->SetMaterial(0,Mat);
  C->SetWorldScale3D(FVector(.2,.14,.6));
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);C->RegisterComponent();
  Out=M->Web->ObjAt(M->Web->AdoptNode(C,Root,Name));return C;
 };
 PropSaro=Prop(SaroRoot,TEXT("ff:tripcam"),ObjSaro);PropRoad=Prop(RoadRoot,TEXT("ff:tripcam:road"),ObjRoad);
 const TWeakPtr<uint8> Guard=Life;
 FS47Interactable I;I.Id=TEXT("ff:tripcam");I.Object=ObjSaro;I.Range=2.8;
 I.Label=[this,Guard]{if(!Guard.IsValid()||!bOut)return FString();return FString(TEXT("Trip camera / collect and read it"));};
 I.Use=[this,Guard]{if(Guard.IsValid())Collect();};I.Examine=I.Label;
 M->Inter->Add(I);if(ObjSaro)M->Web->MarkUsable(ObjSaro.I,I.Id);if(ObjRoad)M->Web->MarkUsable(ObjRoad.I,I.Id);
 Scene=M->Web;bBuilt=true;return true;
}
void FS47TripCamera::Layout()
{
 if(!bBuilt)return;
 for(auto* C:{PropSaro.Get(),PropRoad.Get()})
 {
  if(!C)continue;
  const bool bThis=bOut&&(C==PropSaro.Get())==(Area==TEXT("saro"));
  C->SetVisibility(bThis);C->SetHiddenInGame(!bThis);C->SetCollisionEnabled(bThis?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
  if(bThis)C->SetWorldLocation(S47Three::ToUE(TripWorld(At)));
 }
}
bool FS47TripCamera::CanSet() const
{
 return M&&M->Freeform&&M->Freeform->Field&&!bOut&&M->Freeform->Field->Workshop().IsBuilt(TEXT("trip_camera"));
}
bool FS47TripCamera::SetHere(const FString& InTitle,bool bAtMarks)
{
 if(!CanSet()||!M->Player||!M->Web)return false;
 const auto Clock=M->Freeform->Clock();
 bOut=true;Title=InTitle;bAtMarksThen=bAtMarks;SetClock=Clock;Area=M->Web->Area();Caught=ECatch::None;CaughtClock=-1;PhotoId.Empty();LookIn=0;
 if(Area==TEXT("diner"))Area=TEXT("road");
 At=FVector(M->Player->Pos.X+1.5,M->Player->FloorY,M->Player->Pos.Z);
 if(!bBuilt)Build();
 Layout();
 if(M->UI)M->UI->Toast(M->Freeform->InventoryCount(TEXT("wire"))>0?TEXT("Trip camera set, and fence wire run out from it across the way in, thirty yards each side. It reads in the morning."):TEXT("Trip camera set, wire across the likely way in. It reads in the morning."),5);
 UE_LOG(LogTemp,Display,TEXT("S47: trip camera set at %s, marks %d"),*Title,bAtMarks);M->RequestAutosave();return true;
}
int32 FS47TripCamera::Reading(ECatch Got,int32 Seed,int32 Night)
{
 if(Got==ECatch::Body)return 3;
 if(Got==ECatch::Light)return 2;
 // Nothing strange passed: now and then an animal did. That is ordinary life, not an observation, and it is all the dice decide.
 return FS47NightRhythm::Roll(Seed,Night,0x54524950u)<.3f?1:0;
}
FString FS47TripCamera::ReadingText(int32 Read,const FString& Place,int32 Seed,int32 Night,double When,bool bFrame)
{
 const int32 Secs=When>=0?int32(FMath::Fmod(When,86400.)):int32(3600*(1+FS47NightRhythm::Roll(Seed,Night,0x4845u)*4));
 const int32 Hh=Secs/3600,Mm=(Secs/60)%60;
 const FString Print=bFrame?TEXT(" Its frame is on the roll: develop it at the wet bench like any other."):TEXT("");
 switch(Read)
 {
  case 0:return FString::Printf(TEXT("Trip camera at %s, read in the morning. The counter has not moved. The wire is as I set it. Covers only what crossed its wire."),*Place);
  case 1:return FString::Printf(TEXT("Trip camera at %s, read in the morning. Two frames by its clock at %02d:%02d. On the contact strip: a coyote, or most of one, close to the lens. Covers only what crossed its wire."),*Place,Hh,Mm);
  case 2:return FString::Printf(TEXT("Trip camera at %s, read in the morning. Frames by its clock from %02d:%02d. The wire was not touched: the flash cell fired on light. The ground in front of it is lit hard from above, and nothing stands in it.%s"),*Place,Hh,Mm,*Print);
  default:return FString::Printf(TEXT("Trip camera at %s, read in the morning. Six frames by its clock from %02d:%02d, all within a minute. Something stood at the edge of the frame, the wrong shape for anything that lives out here. The wire was not broken. It was unhooked.%s"),*Place,Hh,Mm,*Print);
 }
}
bool FS47TripCamera::Sees(const FVector& Lens,const FVector& P) const
{
 if(!M||!M->Web||!M->Web->GetWorld())return false;
 FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47TripCamera),true);
 if(auto* C=PropSaro.Get())Q.AddIgnoredComponent(C);
 if(auto* C=PropRoad.Get())Q.AddIgnoredComponent(C);
 const FVector A=S47Three::ToUE(Lens),B=S47Three::ToUE(P);
 return !M->Web->GetWorld()->LineTraceSingleByChannel(Hit,A,B,ECC_Visibility,Q)||Hit.Distance>FVector::Dist(A,B)-60.;
}
void FS47TripCamera::Watch()
{
 if(!bOut||!M||!M->Freeform||!M->Web||!M->Freeform->RulesLive()||Caught==ECatch::Body)return;
 // Only the world it stands in, while that world is going on: what is not loaded does not pass, and a sleep does not make it.
 FString Here=M->Web->Area();if(Here==TEXT("diner"))Here=TEXT("road");
 if(Here!=Area||M->Freeform->Held())return;
 auto& F=*M->Freeform;
 const double Hour=FMath::Fmod(F.Clock(),86400.)/3600.;
 if(!(Hour>=19||Hour<FS47NightRhythm::EndHour)||F.Clock()<SetClock)return;
 const FVector Lens=At+FVector(0,.9,0);
 const auto Flat=[&Lens](const FVector& P){return FVector2D::Distance(FVector2D(P.X,P.Z),FVector2D(Lens.X,Lens.Z));};
 TOptional<FVector> Body,Light;
 const double Reach=F.InventoryCount(TEXT("wire"))>0?WireBodyReach:BodyReach;
 if(M->Night&&M->Night->Glimpses)for(const auto& I:M->Night->Glimpses->Individuals())if(Flat(I.Pos)<Reach&&Sees(Lens,I.Eye)){Body=I.Eye;break;}
 if(!Body&&F.FarmSite)if(const TOptional<FVector> Eye=F.FarmSite->VisitBodyEye();Eye&&Flat(*Eye)<Reach&&Sees(Lens,*Eye))Body=*Eye;
 if(!Body)
 {
  if(F.Taken&&F.Taken->Stage()==FS47Taken::EStage::Flood&&Flat(F.Taken->LightFrom())<LightReach)Light=F.Taken->LightFrom();
  static const TCHAR* const Lights[]={TEXT("farm_lights"),TEXT("town_lights"),TEXT("dusk_orb"),TEXT("response_lights"),TEXT("flyover"),TEXT("close_pass")};
  if(!Light&&F.Director)for(const TCHAR* L:Lights)if(F.Director->ActiveEvent()==L&&Flat(F.Director->EventAnchor())<LightReach&&Sees(Lens,F.Director->EventAnchor())){Light=F.Director->EventAnchor();break;}
 }
 const ECatch Got=Body?ECatch::Body:Light?ECatch::Light:ECatch::None;
 if(int32(Got)<=int32(Caught))return;
 Caught=Got;CaughtClock=F.Clock();
 UE_LOG(LogTemp,Display,TEXT("S47: trip camera at %s caught %s at %.0f"),*Title,Got==ECatch::Body?TEXT("a body"):TEXT("a light"),CaughtClock);
 // The frame it took, from where it stands: one print a night, the best it got.
 const FVector Look=Body?*Body:FVector(Light->X,At.Y+.6,Light->Z);
 if(M->Night&&M->Night->Album&&(PhotoId.IsEmpty()||Got==ECatch::Body))
 {
  FString Id;
  const FString Text=Got==ECatch::Body?FString::Printf(TEXT("Trip camera, %s. Something at the edge of the frame."),*Title):FString::Printf(TEXT("Trip camera, %s. The ground lit from above."),*Title);
  if(M->Night->Album->ExposeFrom(Lens,Look,Text,Title,Id)){PhotoId=Id;UE_LOG(LogTemp,Display,TEXT("S47: trip camera frame %s"),*Id);}
 }
 M->RequestAutosave();
}
bool FS47TripCamera::Collect()
{
 if(!bOut||!M||!M->Freeform||!M->UI)return false;
 auto& F=*M->Freeform;
 const double Hour=FMath::Fmod(F.Clock(),86400.)/3600.;
 const int32 Night=FS47FreeformSession::NightOf(SetClock);
 // Set and fetched the same night: nothing has had a night to cross it.
 const bool bANight=F.Clock()-SetClock>=3600&&(FS47FreeformSession::NightOf(F.Clock())>Night||(Hour>=FS47NightRhythm::EndHour&&Hour<12));
 FString Page;
 if(!bANight)Page=TEXT("Too soon. The counter reads nothing yet. Taken up again.");
 else
 {
  const int32 Read=Reading(Caught,F.Seed(),Night);
  Page=ReadingText(Read,Title,F.Seed(),Night,CaughtClock,!PhotoId.IsEmpty());
  F.RecordEvidence(FString::Printf(TEXT("tripcam_d%d"),Night),Page);
  UE_LOG(LogTemp,Display,TEXT("S47: trip camera read, reading %d for night %d"),Read,Night);
 }
 bOut=false;Caught=ECatch::None;CaughtClock=-1;PhotoId.Empty();Layout();
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("TRIP CAMERA"),S47Kit::Col({S47Kit::Text(Page),S47Kit::Small(TEXT("Back in your kit. It can be set again."))}),760));
 M->RequestAutosave();return true;
}
void FS47TripCamera::Step(double Dt)
{
 if(bOut&&bBuilt&&!PropSaro.IsValid()){bBuilt=false;Build();Layout();}
 LookIn-=Dt;if(LookIn>0)return;LookIn=.25;
 Watch();
}
bool FS47TripCamera::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s trip camera: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 FS47TripCamera C(nullptr);C.Begin(nullptr);Check(C.Save()->Values.Num()==0&&!C.Out(),TEXT("an older save has none out"));
 int32 Strange=0,Animal=0;bool bStable=true;
 for(int32 N=1;N<=400;++N){const int32 R0=Reading(ECatch::None,47,N);Strange+=R0>=2?1:0;Animal+=R0==1?1:0;bStable&=R0==Reading(ECatch::None,47,N);}
 Check(bStable&&Strange==0&&Animal>0&&Animal<400,TEXT("nothing strange is drawn for a night that brought nothing past it; an animal sometimes; the same each time"));
 Check(Reading(ECatch::Light,47,3)==2&&Reading(ECatch::Body,47,3)==3,TEXT("what passed is what it shows"));
 bool bRecord=true;for(int32 R=0;R<4;++R){const FString T=ReadingText(R,TEXT("the mine"),47,3,2*3600.+7*60,R>=2);bRecord&=T.Contains(TEXT("the mine"))&&!T.ToLower().Contains(TEXT("alien"));if(R>=2)bRecord&=T.Contains(TEXT("02:07"))&&T.Contains(TEXT("develop"));}
 Check(bRecord,TEXT("each reading says where, and does not say what"));
 auto O=S47J::New();S47J::Set(O,TEXT("out"),true);S47J::Set(O,TEXT("area"),TEXT("road"));S47J::Set(O,TEXT("title"),TEXT("the mine"));S47J::Set(O,TEXT("clock"),90000.);S47J::Set(O,TEXT("at"),TArray<double>{1,2,3});
 FS47TripCamera B(nullptr);B.Begin(S47J::Read(S47J::Write(O)));Check(B.Out()&&S47J::Num(B.Save(),TEXT("clock"))==90000.,TEXT("one left out stays out after a Continue"));
 S47J::Set(O,TEXT("caught"),2);S47J::Set(O,TEXT("caughtClock"),97000.);S47J::Set(O,TEXT("photo"),TEXT("trip4@97000"));
 FS47TripCamera Kept(nullptr);Kept.Begin(S47J::Read(S47J::Write(O)));Check(Kept.CaughtTonight()==ECatch::Body&&S47J::Str(Kept.Save(),TEXT("photo"))==TEXT("trip4@97000"),TEXT("what it caught, and its frame, survive a Continue"));
 return OK;
}

// ---------- the round
const TArray<const TCHAR*>& FS47DayRound::AllStops()
{
 static const TArray<const TCHAR*> S={TEXT("ff:generator"),TEXT("ff:feeder"),TEXT("ff:service"),TEXT("ff:depot"),TEXT("ff:bench")};
 return S;
}
TArray<FString> FS47DayRound::StopsFor(int32 Seed,int32 Day)
{
 TArray<FString> Left;for(const TCHAR* S:AllStops())Left.Add(S);
 TArray<FString> Out;
 for(int32 I=0;I<StopCount&&!Left.IsEmpty();++I){const int32 Pick=int32(FS47NightRhythm::Roll(Seed,Day,0x524f554eu+I)*Left.Num())%Left.Num();Out.Add(Left[Pick]);Left.RemoveAt(Pick);}
 return Out;
}
FString FS47DayRound::StopName(const FString& Id)
{
 return Id==TEXT("ff:generator")?TEXT("the generator"):Id==TEXT("ff:feeder")?TEXT("the remote feeder"):Id==TEXT("ff:service")?TEXT("the cable junction"):Id==TEXT("ff:depot")?TEXT("the depot cabinet"):Id==TEXT("ff:bench")?TEXT("the repair bench"):Id;
}
int32 FS47DayRound::CurrentDay() const{return M&&M->Freeform?M->Freeform->Day():0;}
void FS47DayRound::Begin(const S47J::FObj& Saved)
{
 ForDay=DoneDay=LogDay=0;Ticks.Reset();
 if(!Saved.IsValid())return;
 ForDay=FMath::Max(0,S47J::Int(Saved,TEXT("day")));DoneDay=FMath::Max(0,S47J::Int(Saved,TEXT("done")));LogDay=FMath::Max(0,S47J::Int(Saved,TEXT("log")));
 for(const FString& T:S47J::Strs(Saved,TEXT("ticks")))if(AllStops().ContainsByPredicate([&](const TCHAR* S){return T==S;}))Ticks.AddUnique(T);
}
S47J::FObj FS47DayRound::Save() const
{
 auto O=S47J::New();
 if(ForDay>0)S47J::Set(O,TEXT("day"),ForDay);if(DoneDay>0)S47J::Set(O,TEXT("done"),DoneDay);if(LogDay>0)S47J::Set(O,TEXT("log"),LogDay);
 if(!Ticks.IsEmpty())S47J::Set(O,TEXT("ticks"),Ticks);
 return O;
}
FString FS47DayRound::TaskLine() const
{
 if(!M||!M->Freeform||!M->Freeform->RulesLive()||M->Freeform->InTutorial()||CurrentDay()<2||DoneDay==CurrentDay())return FString();
 const double Hour=FMath::Fmod(M->Freeform->Clock(),86400.)/3600.;if(Hour<6||Hour>=19)return FString();
 TArray<FString> Left;for(const FString& S:StopsFor(M->Freeform->Seed(),CurrentDay()))if(!Ticks.Contains(S))Left.Add(StopName(S));
 return Left.IsEmpty()?FString():TEXT("ROUND / Today: ")+FString::Join(Left,TEXT(", "))+FString::Printf(TEXT(". $%d on the sheet when it is done."),RoundPay);
}
void FS47DayRound::Step(double Dt)
{
 static_cast<void>(Dt);
 if(!M||!M->Freeform||!M->Freeform->RulesLive()||M->Freeform->InTutorial()||!M->Freeform->Field||!M->Player||!M->World)return;
 auto& F=*M->Freeform;const int32 Today=F.Day();
 if(Today>=2&&LogDay<Today-1){WriteLog(Today-1);LogDay=Today-1;M->RequestAutosave();}
 if(ForDay!=Today){ForDay=Today;Ticks.Reset();}
 if(Today<2||DoneDay==Today||M->World->bDriving)return;
 const double Hour=FMath::Fmod(F.Clock(),86400.)/3600.;if(Hour<6||Hour>=19)return;
 const FVector Foot=M->Player->Pos;
 for(const FString& S:StopsFor(F.Seed(),Today))
 {
  if(Ticks.Contains(S))continue;
  const FVector At=F.Field->InteractionPoint(S);
  if(FVector2D(At.X-Foot.X,At.Z-Foot.Z).Size()>5)continue;
  Ticks.Add(S);
  if(M->Audio)M->Audio->Beep(900.f,.05f,.04f);
  UE_LOG(LogTemp,Display,TEXT("S47: round: %s looked over, %d of %d"),*S,Ticks.Num(),StopCount);
  if(Ticks.Num()>=StopCount)
  {
   DoneDay=Today;F.Earn(RoundPay);
   FString Note=FString::Printf(TEXT("Round done. $%d on the sheet."),RoundPay);
   // A round before the fault's own hour catches it while it is still a worn contact.
   const bool bCaught=F.FaultDueLater()&&F.Consume(TEXT("relay"));
   if(bCaught){F.Field->SkipFaultToday();Note=FString::Printf(TEXT("Round done. A drive relay was going; you swapped it before it failed. $%d on the sheet."),RoundPay);UE_LOG(LogTemp,Display,TEXT("S47: round: today's fault caught in time"));}
   F.RecordEvidence(FString::Printf(TEXT("round-d%d"),Today),bCaught?TEXT("Station round. One drive relay swapped before it failed. Nothing else out of order."):TEXT("Station round. Nothing out of order."));
   if(M->UI)M->UI->Toast(Note,5);
  }
  else if(M->UI)M->UI->Toast(FString::Printf(TEXT("%s looked over."),*StopName(S).Left(1).ToUpper().Append(StopName(S).Mid(1))),3);
  M->RequestAutosave();
  break;
 }
}
FString FS47DayRound::LogPage(int32 Day,int32 Filed,int32 Paid,bool bRound,int32 Records,const FString& Trouble)
{
 FString Page=FString::Printf(TEXT("SARO / SHIFT LOG / DAY %d\nOBSERVATIONS FILED %d\nCREDITED $%d\nSTATION ROUND %s\nFIELD RECORDS KEPT %d\n"),Day,Filed,Paid,bRound?TEXT("DONE"):TEXT("NOT LOGGED"),Records);
 if(!Trouble.IsEmpty())Page+=TEXT("OPEN / ")+Trouble+TEXT("\n");
 return Page+TEXT("Countersigned by nobody. The sheet goes in the binder.");
}
void FS47DayRound::WriteLog(int32 Day)
{
 if(!M||!M->Story||!M->Freeform)return;
 auto& F=*M->Freeform;
 int32 Filed=0,Paid=0;
 for(const auto& J:F.Jobs())if(J.Day==Day&&J.Stage==FS47FreeformSession::EJobStage::Filed&&J.CaseTest<0){++Filed;Paid+=J.bDocked?10:20;}
 const FString Trouble=F.PowerFault()?TEXT("feeder fault"):F.Field&&F.Field->HasArrayFault()?TEXT("antenna fault"):FString();
 FS47Doc D;D.Id=FString::Printf(TEXT("shiftlog-%d"),Day);D.Title=FString::Printf(TEXT("Shift log, day %d"),Day);D.Kind=TEXT("typed");D.Stamp=FString::Printf(TEXT("SARO / DAY %d"),Day);
 D.Page=LogPage(Day,Filed,Paid,DoneDay==Day||F.HasFinding(FString::Printf(TEXT("round-d%d"),Day)),F.RecordsOnDay(Day),Trouble);D.Transcript=D.Page;
 M->Story->AddDoc(D);
 UE_LOG(LogTemp,Display,TEXT("S47: shift log for day %d: filed %d"),Day,Filed);
}
bool FS47DayRound::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s day round: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 bool bThree=true,bStable=true,bDiffers=false;
 for(int32 D=1;D<=60;++D){const auto A=StopsFor(47,D);TSet<FString> U(A);bThree&=A.Num()==StopCount&&U.Num()==StopCount;bStable&=A==StopsFor(47,D);bDiffers|=A!=StopsFor(47,D+1);}
 Check(bThree&&bStable&&bDiffers,TEXT("three different stops a day, the same all day, not the same every day"));
 const FString P=LogPage(4,2,40,true,3,TEXT("feeder fault"));
 Check(P.Contains(TEXT("DAY 4"))&&P.Contains(TEXT("FILED 2"))&&P.Contains(TEXT("$40"))&&P.Contains(TEXT("feeder fault")),TEXT("the log says what the record says"));
 FS47DayRound R(nullptr);R.Begin(nullptr);Check(R.Save()->Values.Num()==0&&R.StopsDone()==0,TEXT("an older save has no round behind it"));
 return OK;
}

// ---------- somebody's pickup on the shoulder
namespace
{
 const TCHAR* const PickupMesh=TEXT("/Game/Free/Vehicles/pickup_c10_1966/StaticMeshes/pickup_c10_1966.pickup_c10_1966");
}
bool FS47RoadsideCar::StandsOn(int32 Seed,int32 Day){const int32 Block=(Day-1)/2;return Day>=3&&FS47NightRhythm::Roll(Seed,Block,0x43415255u)<.18f;}
double FS47RoadsideCar::RoadZ(int32 Seed,int32 Day){return 720.+FS47NightRhythm::Roll(Seed,(Day-1)/2,0x43415a5au)*900.;}
FString FS47RoadsideCar::Inside(int32 Seed,int32 Day)
{
 static const TCHAR* const Lines[]={
  TEXT("A sixty-six pickup on the shoulder, driver's door open, keys in it. Thermos on the seat, still a little warm, and a lunch nobody has touched. No tracks going anywhere but the ones it made. This records what is here."),
  TEXT("A pickup left on the shoulder with the hood up and nothing wrong under it. A feed cap on the dash. The radio is on, turned low, on a station that has gone off the air. This records what is here."),
  TEXT("A pickup on the shoulder, doors shut, engine cold, a rifle still in the rack. The dust on the door has one handprint in it, low down, as if whoever it was had been sitting on the ground. This records what is here.")};
 return Lines[int32(FS47NightRhythm::Roll(Seed,(Day-1)/2,0x43415254u)*3)%3];
}
void FS47RoadsideCar::Begin(const S47J::FObj& Saved)
{
 Stop();Life=MakeShared<uint8>(0);ShownDay=Saved.IsValid()?FMath::Max(0,S47J::Int(Saved,TEXT("day"))):0;bShown=false;
 if(M&&IsValid(M->Web.Get())&&Scene.Get()!=M->Web.Get()){bBuilt=false;Car.Reset();}
}
void FS47RoadsideCar::Stop()
{
 Life.Reset();bShown=false;
 if(!IsEngineExitRequested()&&!IsGarbageCollecting())if(auto* C=Car.Get()){C->SetVisibility(false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
}
S47J::FObj FS47RoadsideCar::Save() const{auto O=S47J::New();if(ShownDay>0)S47J::Set(O,TEXT("day"),ShownDay);return O;}
bool FS47RoadsideCar::Build()
{
 if(bBuilt)return true;
 if(!M||!IsValid(M->Web.Get())||!IsValid(M->Inter))return false;
 const int32 RoadRoot=M->Web->Handle(TEXT("world.legs.road.area.group"));if(RoadRoot<0)return false;
 if(!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(PickupMesh))))return false;
 auto* Mesh=LoadObject<UStaticMesh>(nullptr,PickupMesh);if(!Mesh)return false;
 auto* C=NewObject<UStaticMeshComponent>(M->Web.Get(),TEXT("ff:roadcar"));C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(M->Web->GetRootComponent());C->SetStaticMesh(Mesh);C->SetCastShadow(true);
 C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);C->SetVisibility(false);C->RegisterComponent();
 Car=C;Obj=M->Web->ObjAt(M->Web->AdoptNode(C,RoadRoot,TEXT("ff:roadcar")));
 const TWeakPtr<uint8> Guard=Life;
 FS47Interactable I;I.Id=TEXT("ff:roadcar");I.Object=Obj;I.Range=3.5;
 I.Label=[this,Guard]{return Guard.IsValid()&&bShown?FString(TEXT("Empty pickup / look inside")):FString();};
 I.Use=[this,Guard]{if(Guard.IsValid())Look();};I.Examine=I.Label;
 M->Inter->Add(I);if(Obj)M->Web->MarkUsable(Obj.I,I.Id);
 Scene=M->Web;bBuilt=true;return true;
}
void FS47RoadsideCar::Look()
{
 if(!M||!M->Freeform||!M->UI||!bShown)return;
 auto& F=*M->Freeform;
 const FString Page=Inside(F.Seed(),F.Day());
 const int32 Block=(F.Day()-1)/2;const FString Key=FString::Printf(TEXT("roadcar_b%d"),Block);
 const bool bFirst=!F.HasFinding(Key);
 F.RecordEvidence(Key,Page);
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("EMPTY PICKUP"),S47Kit::Col({S47Kit::Text(Page),S47Kit::Small(bFirst?TEXT("Kept as a field record."):TEXT("Already in your field records."))}),760));
 UE_LOG(LogTemp,Display,TEXT("S47: roadside pickup looked into, block %d"),Block);
}
void FS47RoadsideCar::Step(double Dt)
{
 static_cast<void>(Dt);
 if(!M||!M->Freeform||!M->Freeform->RulesLive()||!M->Player||!M->Web)return;
 auto& F=*M->Freeform;const int32 Today=F.Day();
 if(!Build())return;
 const bool bWant=StandsOn(F.Seed(),Today);
 if(bWant==bShown&&(!bShown||ShownDay==Today||(ShownDay-1)/2==(Today-1)/2))return;
 const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 const double Z=RoadZ(F.Seed(),Today);
 // It comes and goes unseen.
 if(FVector2D(M->Player->Pos.X-(Road.X+7),M->Player->Pos.Z-(Road.Z+Z)).Size()<300)return;
 auto* C=Car.Get();if(!C)return;
 bShown=bWant;ShownDay=Today;
 C->SetVisibility(bWant);C->SetHiddenInGame(!bWant);C->SetCollisionEnabled(bWant?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
 if(bWant)
 {
  const double X=7.2,Y=Road.Y+S47RoadTerrain::SurfaceY(X,Z);
  double Mat[16];S47Three::Compose(FVector(Road.X+X,Y,Road.Z+Z),S47Three::Euler(0,UE_DOUBLE_PI+.12,0),FVector::OneVector,Mat);C->SetWorldTransform(S47Three::MatToUE(Mat));
  UE_LOG(LogTemp,Display,TEXT("S47: an empty pickup stands on the shoulder at road %.0f, day %d"),Z,Today);
 }
 M->RequestAutosave();
}
bool FS47RoadsideCar::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s roadside car: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 int32 On=0;bool bBands=true,bPairs=true;
 for(int32 D=1;D<=400;++D){On+=StandsOn(47,D)?1:0;const double Z=RoadZ(47,D);bBands&=Z>=720&&Z<=1620;if(D%2==1)bPairs&=StandsOn(47,D)==StandsOn(47,D+1);}
 Check(On>10&&On<160&&bPairs,TEXT("some days it stands there, two days at a time"));
 Check(bBands&&!StandsOn(47,1)&&!StandsOn(47,2),TEXT("out on the open road, and not in the first days"));
 Check(!Inside(47,9).IsEmpty(),TEXT("there is something to find in it"));
 return OK;
}
