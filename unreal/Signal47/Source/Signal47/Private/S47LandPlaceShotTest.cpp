#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47Flow.h"
#include "S47LandWindmill.h"
#include "S47Inter.h"
#include "S47Legs.h"
#include "S47OpenLand.h"
#include "S47Three.h"
#include "S47Places.h"
#include "S47Sky.h"
#include "S47Startup.h"
#include "S47UI.h"
#include "S47Weather.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

// LAND-PLACES-20261114, -S47LandPlaceShotTest=windmill: a place out on the land as a player sees it, for the pictures of a model's
// critique rounds: from 6 m, 40 m, 300 m and 1 km along a compass line from it (-S47LandPlaceAz=<degrees>, 195: from the south-
// south-west; the two far ones along -S47LandPlaceFarAz=, 285, where the land lets it stand against the sky), and its close-ups, at each hour of -S47LandPlaceHours=<h,h,...> (13,18.6,25: day, dusk (the game's sun sets at 18:27) and one in the night, when the
// moon is put up for the picture if the date's own is down). -S47ShotDir=<dir> takes the PNGs, named <view>-<hhmm>.png. With
// -S47LandMillOff the same views show the place as it was before its model. The run also holds the model's seat: its pieces against
// the drawn ground under them (S47LandWindmill::Audit). Pictures for a look; the seat is a check.
namespace S47LandPlaceShotLocal
{
 // Eye and target from the place's middle, metres (x east, z south; the target's y over its ground). Along: metres out on the line.
 struct FView{const TCHAR* Name;double Along,EX,EZ,TX,TY,TZ;float Fov;int32 Pose;int32 When;};   // Pose: 0 as the game has it, 1 lid open with the page, 2 lid wired shut, 3 across the tank toward the moon. When: 0 always, 1 by day, 2 by night
 const FView Views[]={
  {TEXT("d0006"),6,0,0,.9,1.5,0,70,0,0},
  {TEXT("d0040"),40,0,0,1.5,4.9,0,70,0,0},
  {TEXT("d0300"),300,0,0,0,5.2,0,70,0,0},
  {TEXT("d1000"),1000,0,0,0,5.2,0,70,0,0},
  {TEXT("c1-lid-open-page"),0,.28,1.62,0,.22,.27,55,1,0},
  {TEXT("c2-tank-water"),0,3.1,2.75,3.5,.52,.35,62,0,0},
  {TEXT("c3-pier"),0,-2.35,2.95,-1.25,.2,1.25,40,0,1},
  {TEXT("c4-head"),0,-2.4,7.2,0,9.6,-.2,22,0,1},
  {TEXT("c5-up-the-tower"),0,-3.6,4.8,0,6.2,0,80,0,1},
  {TEXT("c6-lid-wired"),0,.35,1.5,0,.5,.45,42,2,1},
  {TEXT("c7-water-moon"),0,0,0,0,.47,0,50,3,2},
 };
 constexpr int32 ViewCount=UE_ARRAY_COUNT(Views);
 // -S47LandPlaceShotTest=farmmill: the farm's windmill (road 146, 214; its tank 4.7 m east and 3.8 m north of it), the same way.
 const FView FarmViews[]={
  {TEXT("farm-d0006"),6,0,0,1.5,1.6,-1,70,0,0},
  {TEXT("farm-d0040"),40,0,0,2,4.9,-1,70,0,0},
  {TEXT("farm-tank"),0,2.2,1.2,4.7,.5,-3.8,70,0,0},
  {TEXT("farm-head"),0,-2.4,7.2,0,9.6,-.2,22,0,0},
 };
}

void FS47FreeformTest::StepLandPlaceShots(double Dt)
{
 using namespace S47LandPlaceShotLocal;
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>520){FS47Sky::DevMoon=FS47Sky::FDevMoon();Finish(false,FString::Printf(TEXT("land place shots exceeded the time bound at stage %d"),Stage));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->World||!M->Freeform->Places)return;
 auto& F=*M->Freeform;US47World& W=*M->World;
 static FString Dir;static TArray<double> Hours;static double LineAz=195,FarAz=285;static bool bSeatHeld=false,bSeatGood=true;
 if(Dir.IsEmpty())
 {
  if(!FParse::Value(FCommandLine::Get(),TEXT("S47ShotDir="),Dir))Dir=Root/TEXT("landplace");
  IFileManager::Get().MakeDirectory(*Dir,true);
  FString List;if(!FParse::Value(FCommandLine::Get(),TEXT("S47LandPlaceHours="),List,false))List=TEXT("13,18.6,25");
  TArray<FString> Bits;List.ParseIntoArray(Bits,TEXT(","));for(const FString& B:Bits)Hours.Add(FCString::Atod(*B));
  FParse::Value(FCommandLine::Get(),TEXT("S47LandPlaceAz="),LineAz);
  FParse::Value(FCommandLine::Get(),TEXT("S47LandPlaceFarAz="),FarAz);
 }
 static FString Which;if(Which.IsEmpty())FParse::Value(FCommandLine::Get(),TEXT("S47LandPlaceShotTest="),Which);
 const bool bFarm=Which==TEXT("farmmill");
 double Mx=146,Mz=214;if(!bFarm&&!F.Places->Spot(TEXT("windmill"),Mx,Mz))return;
 const FView* const Set=bFarm?FarmViews:Views;const int32 SetCount=bFarm?int32(UE_ARRAY_COUNT(FarmViews)):ViewCount;
 auto ToHour=[&](double Hour){const double Want=FMath::Fmod(Hour*3600.,86400.),Now=FMath::Fmod(F.Clock(),86400.);F.TestAdvance(FMath::Fmod(Want-Now+86400.,86400.));};
 auto Pose=[&](const FView& V)
 {
  if(W.bDriving)W.StopDriving();
  const double Rad=FMath::DegreesToRadians(V.Along>=300?FarAz:LineAz);   // (the far ones from where the land lets it stand against the sky)
  double EX=V.Along>0?FMath::Sin(Rad)*V.Along:V.EX,EZ=V.Along>0?-FMath::Cos(Rad)*V.Along:V.EZ,TX=V.TX,TZ=V.TZ;
  if(V.Pose==3)
  {
   // across the tank's water toward the moon, from where its light comes off the water into the eye
   const S47Weather::FMoon Mo=S47Weather::Moon(F.Date(),FMath::Fmod(F.Clock(),86400.));
   const double MoonAz=FMath::DegreesToRadians(FS47Sky::DevMoon.bOn?FS47Sky::DevMoon.Az:Mo.Az),MoonEl=FMath::Clamp(FS47Sky::DevMoon.bOn?FS47Sky::DevMoon.El:Mo.El,12.,60.);
   const FVector2D To(FMath::Sin(MoonAz),-FMath::Cos(MoonAz)),Tank(S47LandWindmill::TankX,S47LandWindmill::TankZ);
   const FVector2D Eye=Tank-To*FMath::Min(3.4,(1.62-S47LandWindmill::WaterY)/FMath::Tan(FMath::DegreesToRadians(MoonEl))),Look=Tank+To*1.2;
   EX=Eye.X;EZ=Eye.Y;TX=Look.X;TZ=Look.Y;
  }
  // on foot on the land: the area of the leg that has this ground (the road's land is walked as the area "diner", US47World::WalkOver)
  const FString Leg=S47OpenLand::LegFor(TEXT("road"),Mx+EX,Mz+EZ);
  const FString Want=Leg==TEXT("road")?FString(TEXT("diner")):Leg;
  if(W.Area!=Want){if(Want==TEXT("diner"))W.PlaceTruck(Want);W.Enter(Want);}
  const FVector Spot=S47OpenLand::FromRoad(Leg,Mx+EX,Mz+EZ);
  AS47Walker& P=*M->Player;
  P.bHeadMotion=false;P.Place(Spot.X,Spot.Z,0);P.SettleOnFloor();
  FVector Stand;bool bOnGround=false;const double PlaceY=!bFarm&&F.Places->Stand(TEXT("windmill"),Stand,bOnGround)?Stand.Y:P.FloorY;
  const double DX=TX-EX,DZ=TZ-EZ,DY=PlaceY+V.TY-(P.FloorY+P.Eye);
  P.Yaw=S47OpenLand::Turned(TEXT("road"),Leg,FMath::Atan2(-DX,-DZ));P.Pitch=FMath::Clamp(FMath::Atan2(DY,FVector2D(DX,DZ).Size()),-1.3,1.3);P.Fov=V.Fov;
  S47LandWindmill::DevPose(V.Pose==1||V.Pose==2,V.Pose==1,V.Pose==1,V.Pose==2);
 };
 // what the eye's ray finds to use, as the game picks it
 auto Picked=[&]
 {
  const AS47Walker& P=*M->Player;
  const FVector Dir(-FMath::Sin(P.Yaw)*FMath::Cos(P.Pitch),FMath::Sin(P.Pitch),-FMath::Cos(P.Yaw)*FMath::Cos(P.Pitch));
  return IsValid(M->Inter)?M->Inter->Pick(P.EyeUE(),S47Three::DirToUE(Dir),6):FString();
 };
 // Stages 9000 on: the Baca mill used as a player uses it (after the pictures).
 if(Stage>=9000)
 {
  const auto Done=[&]
  {
   FS47Sky::DevMoon=FS47Sky::FDevMoon();S47LandWindmill::DevPose(false,false,false,false);
   if(!Check(bSeatGood,TEXT("the windmill's model is placed and seated on the drawn ground (or the old shapes are shown)")))return;
   Finish(true,FString::Printf(TEXT("land place %s: %d views at %d hours in %s"),*Which,SetCount,Hours.Num(),*Dir));
  };
  if(bFarm||!S47LandWindmill::Available()){Done();return;}
  if(Stage==9000){S47LandWindmill::DevPose(false,false,false,false);Pose(Views[9]);S47LandWindmill::DevPose(false,false,false,false);Go(9001,2.5);return;}
  if(Stage==9001)
  {
   const FString Id=Picked();
   UE_LOG(LogTemp,Display,TEXT("S47 land place shots: at the housing the eye finds %s: %s"),*Id,*M->Inter->LabelOf(Id));
   if(!Check(Id==TEXT("ff:mill:lid")&&M->Inter->LabelOf(Id).Contains(TEXT("lift the lid")),TEXT("aimed at the pump housing, E offers to lift its lid")))return;
   M->Inter->UseId(Id);Go(9002,1.6);return;
  }
  if(Stage==9002)
  {
   if(!Check(S47LandWindmill::LidOpen()>.95&&M->Inter->LabelOf(TEXT("ff:mill:lid")).Contains(TEXT("shut the lid")),TEXT("used, the lid stands open and E offers to shut it")))return;
   FScreenshotRequest::RequestScreenshot(Dir/TEXT("f1-lid-lifted-by-hand.png"),false,false);
   Go(9003,.8);return;
  }
  if(Stage==9003){M->Inter->UseId(TEXT("ff:mill:lid"));Go(9004,1.6);return;}
  if(Stage==9004){if(!Check(S47LandWindmill::LidOpen()<.05,TEXT("used again, the lid is shut")))return;Pose(Views[5]);S47LandWindmill::DevPose(false,false,false,false);Go(9005,2.5);return;}
  if(Stage==9005)
  {
   const FString Id=Picked();
   UE_LOG(LogTemp,Display,TEXT("S47 land place shots: at the tank the eye finds %s: %s"),*Id,*M->Inter->LabelOf(Id));
   if(!Check(Id==FS47Places::UseId(TEXT("windmill"))&&!M->Inter->LabelOf(Id).IsEmpty(),TEXT("aimed at the tank, E is the place's own look (ff:place:windmill)")))return;
   S47LandWindmill::Knock(false);Go(9006,1.4);return;
  }
  if(Stage==9006)
  {
   if(!Check(S47LandWindmill::WheelRateNow()>60,FString::Printf(TEXT("at a knock of the night's windmill_knock the wheel turns (%.0f degrees a second) and the rod with it"),S47LandWindmill::WheelRateNow())))return;
   S47LandWindmill::KnockEnd();Go(9007,3.5);return;
  }
  if(!Check(S47LandWindmill::WheelRateNow()==0,TEXT("when it stops the wheel stands still")))return;
  Done();return;
 }
 // Stage 0: begin; 1000*H: the hour H is set; 1000*H+10*I: view I posed; +1: shot.
 if(Stage==0){if(Age<3)return;M->UI->Close(true);Go(1000,.1);return;}
 const int32 H=Stage/1000-1,Rest=Stage%1000;
 if(H>=Hours.Num()){Go(9000,.1);return;}
 const double Hour=Hours[H];const bool bDay=FMath::Fmod(Hour,24.)>=7&&FMath::Fmod(Hour,24.)<17.5,bNoon=FMath::Fmod(Hour,24.)>=6&&FMath::Fmod(Hour,24.)<19;
 if(Rest==0)
 {
  ToHour(Hour);
  if(!bDay&&FMath::Fmod(Hour,24.)<6)
  {
   // a night picture wants a moon: the date's own if it is up, else one put there for the picture (F10's knob; the stars are not touched)
   const S47Weather::FMoon Mo=S47Weather::Moon(F.Date(),FMath::Fmod(Hour,24.)*3600.);
   if(Mo.El<12||Mo.Lit<.3){FS47Sky::DevMoon.bOn=true;FS47Sky::DevMoon.Az=150;FS47Sky::DevMoon.El=42;FS47Sky::DevMoon.Lit=.8;}
   UE_LOG(LogTemp,Display,TEXT("S47 land place shots: the moon at %.1f h: elevation %.0f, lit %.2f%s"),Hour,Mo.El,Mo.Lit,FS47Sky::DevMoon.bOn?TEXT("; one put up for the picture (az 150, el 42, lit .8)"):TEXT(""));
  }
  Go(Stage+2,8.);return;
 }
 const int32 View=(Rest-2)/10;
 if(View>=SetCount){Go((H+2)*1000,.2);return;}
 const FView& V=Set[View];
 if((V.When==1&&!bDay)||(V.When==2&&bNoon)){Go(Stage+10,.02);return;}
 // posed twice: the first lands in the area as it comes up, the second on its floor
 if(Rest%10==2){Pose(V);Go(Stage+1,1.2);return;}
 if(Rest%10==3){Pose(V);Go(Stage+8,V.Along>=300?3.5:2.5);return;}
 if(!bSeatHeld&&View==0)
 {
  // the player stands at it: the place has measured its ground, and the model has followed
  bSeatHeld=true;
  TArray<FString> Lines;const bool bGood=bFarm||S47LandWindmill::Audit(M,Lines,true);
  for(const FString& L:Lines)UE_LOG(LogTemp,Display,TEXT("S47 land place shots: %s"),*L);
  bSeatGood=bGood;   // (held at the end: the pictures are taken either way)
 }
 const int32 Hh=int32(FMath::Fmod(Hour,24.)),Mm=int32(FMath::RoundToDouble(FMath::Frac(Hour)*60.));
 const FString Name=FString::Printf(TEXT("%s-%02d%02d"),V.Name,Hh,Mm);
 FScreenshotRequest::RequestScreenshot(Dir/(Name+TEXT(".png")),false,false);
 UE_LOG(LogTemp,Display,TEXT("S47 land place shot %s at %.1f,%.1f floor %.2f yaw %.2f pitch %.2f area %s"),*Name,M->Player->Pos.X,M->Player->Pos.Z,M->Player->FloorY,M->Player->Yaw,M->Player->Pitch,*W.Area);
 Go(Stage+1,.7);
}
