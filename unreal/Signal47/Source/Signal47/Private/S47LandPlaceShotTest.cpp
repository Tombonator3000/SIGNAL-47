#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47Flow.h"
#include "S47LandWindmill.h"
#include "S47Legs.h"
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
// critique rounds: from 6 m, 40 m, 300 m and 1 km along one compass line from it (-S47LandPlaceAz=<degrees>, 195: from the south-
// south-west), and its close-ups, at each hour of -S47LandPlaceHours=<h,h,...> (13,19.5,25: day, dusk and one in the night, when the
// moon is put up for the picture if the date's own is down). -S47ShotDir=<dir> takes the PNGs, named <view>-<hhmm>.png. With
// -S47LandMillOff the same views show the place as it was before its model. The run also holds the model's seat: its pieces against
// the drawn ground under them (S47LandWindmill::Audit). Pictures for a look; the seat is a check.
namespace S47LandPlaceShotLocal
{
 // Eye and target from the place's middle, metres (x east, z south; the target's y over its ground). Along: metres out on the line.
 struct FView{const TCHAR* Name;double Along,EX,EZ,TX,TY,TZ;float Fov;int32 Pose;bool bDayOnly;};   // Pose: 0 as the game has it, 1 lid open with the page, 2 lid wired shut
 const FView Views[]={
  {TEXT("d0006"),6,0,0,.9,1.5,0,85,0,false},
  {TEXT("d0040"),40,0,0,1.5,4.9,0,60,0,false},
  {TEXT("d0300"),300,0,0,0,5.2,0,75,0,false},
  {TEXT("d1000"),1000,0,0,0,5.2,0,75,0,false},
  {TEXT("c1-lid-open-page"),0,.28,1.62,0,.22,.27,55,1,false},
  {TEXT("c2-tank-water"),0,3.1,2.75,3.5,.52,.35,62,0,false},
  {TEXT("c3-pier"),0,-2.35,2.95,-1.25,.2,1.25,40,0,true},
  {TEXT("c4-head"),0,-2.4,7.2,0,9.6,-.2,22,0,true},
  {TEXT("c5-up-the-tower"),0,-3.6,4.8,0,6.2,0,80,0,true},
  {TEXT("c6-lid-wired"),0,.35,1.5,0,.5,.45,42,2,true},
 };
 constexpr int32 ViewCount=UE_ARRAY_COUNT(Views);
}

void FS47FreeformTest::StepLandPlaceShots(double Dt)
{
 using namespace S47LandPlaceShotLocal;
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>520){FS47Sky::DevMoon=FS47Sky::FDevMoon();Finish(false,FString::Printf(TEXT("land place shots exceeded the time bound at stage %d"),Stage));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->World||!M->Freeform->Places)return;
 auto& F=*M->Freeform;US47World& W=*M->World;
 static FString Dir;static TArray<double> Hours;static double LineAz=195;static bool bSeatHeld=false,bSeatGood=true;
 if(Dir.IsEmpty())
 {
  if(!FParse::Value(FCommandLine::Get(),TEXT("S47ShotDir="),Dir))Dir=Root/TEXT("landplace");
  IFileManager::Get().MakeDirectory(*Dir,true);
  FString List;if(!FParse::Value(FCommandLine::Get(),TEXT("S47LandPlaceHours="),List,false))List=TEXT("13,19.5,25");
  TArray<FString> Bits;List.ParseIntoArray(Bits,TEXT(","));for(const FString& B:Bits)Hours.Add(FCString::Atod(*B));
  FParse::Value(FCommandLine::Get(),TEXT("S47LandPlaceAz="),LineAz);
 }
 double Mx=0,Mz=0;if(!F.Places->Spot(TEXT("windmill"),Mx,Mz))return;
 const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 auto ToHour=[&](double Hour){const double Want=FMath::Fmod(Hour*3600.,86400.),Now=FMath::Fmod(F.Clock(),86400.);F.TestAdvance(FMath::Fmod(Want-Now+86400.,86400.));};
 auto Pose=[&](const FView& V)
 {
  if(W.bDriving)W.StopDriving();
  // on foot on the road's land is the area "diner" (US47World::WalkOver): its land has the floor, the drive's own area has none
  if(W.Area!=TEXT("diner")){W.PlaceTruck(TEXT("diner"));W.Enter(TEXT("diner"));}
  const double Rad=FMath::DegreesToRadians(LineAz);
  const double EX=V.Along>0?FMath::Sin(Rad)*V.Along:V.EX,EZ=V.Along>0?-FMath::Cos(Rad)*V.Along:V.EZ;
  AS47Walker& P=*M->Player;
  P.bHeadMotion=false;P.Place(Road.X+Mx+EX,Road.Z+Mz+EZ,0);P.SettleOnFloor();
  FVector Stand;bool bOnGround=false;const double PlaceY=F.Places->Stand(TEXT("windmill"),Stand,bOnGround)?Stand.Y:P.FloorY;
  const double DX=V.TX-EX,DZ=V.TZ-EZ,DY=PlaceY+V.TY-(P.FloorY+P.Eye);
  P.Yaw=FMath::Atan2(-DX,-DZ);P.Pitch=FMath::Clamp(FMath::Atan2(DY,FVector2D(DX,DZ).Size()),-1.3,1.3);P.Fov=V.Fov;
  S47LandWindmill::DevPose(V.Pose!=0,V.Pose==1,V.Pose==1,V.Pose==2);
 };
 // Stage 0: begin; 1000*H: the hour H is set; 1000*H+10*I: view I posed; +1: shot.
 if(Stage==0){if(Age<3)return;M->UI->Close(true);Go(1000,.1);return;}
 const int32 H=Stage/1000-1,Rest=Stage%1000,I=Rest/10;const bool bShoot=Rest%10==1;
 if(H>=Hours.Num())
 {
  FS47Sky::DevMoon=FS47Sky::FDevMoon();S47LandWindmill::DevPose(false,false,false,false);
  if(!Check(bSeatGood,TEXT("the windmill's model is placed and seated on the drawn ground (or the old shapes are shown)")))return;
  Finish(true,FString::Printf(TEXT("land place windmill: %d views at %d hours in %s"),ViewCount,Hours.Num(),*Dir));return;
 }
 const double Hour=Hours[H];const bool bDay=FMath::Fmod(Hour,24.)>=7&&FMath::Fmod(Hour,24.)<17.5;
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
 if(View>=ViewCount){Go((H+2)*1000,.2);return;}
 const FView& V=Views[View];
 if(V.bDayOnly&&!bDay){Go(Stage+10,.02);return;}
 // posed twice: the first lands in the area as it comes up, the second on its floor
 if(Rest%10==2){Pose(V);Go(Stage+1,1.2);return;}
 if(Rest%10==3){Pose(V);Go(Stage+8,V.Along>=300?3.5:2.5);return;}
 static_cast<void>(I);
 if(!bSeatHeld&&View==0)
 {
  // the player stands at it: the place has measured its ground, and the model has followed
  bSeatHeld=true;
  TArray<FString> Lines;const bool bGood=S47LandWindmill::Audit(M,Lines,true);
  for(const FString& L:Lines)UE_LOG(LogTemp,Display,TEXT("S47 land place shots: %s"),*L);
  bSeatGood=bGood;   // (held at the end: the pictures are taken either way)
 }
 const int32 Hh=int32(FMath::Fmod(Hour,24.)),Mm=int32(FMath::RoundToDouble(FMath::Frac(Hour)*60.));
 const FString Name=FString::Printf(TEXT("%s-%02d%02d"),V.Name,Hh,Mm);
 FScreenshotRequest::RequestScreenshot(Dir/(Name+TEXT(".png")),false,false);
 UE_LOG(LogTemp,Display,TEXT("S47 land place shot %s at %.1f,%.1f floor %.2f yaw %.2f pitch %.2f area %s"),*Name,M->Player->Pos.X,M->Player->Pos.Z,M->Player->FloorY,M->Player->Yaw,M->Player->Pitch,*W.Area);
 Go(Stage+1,.7);
}
