#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47Flow.h"
#include "S47Startup.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

// RECORDS-SHOTS-20261113, -S47RecordsShotTest: the SARO records room as it stands, from where a player stands in it, for Tom's
// reference images (a realism lift): the door, the four corners, the desk, the cabinets, the shelves, the north wall, the clerk desk
// and card index, the ceiling and the floor; at 21:00 with the ceiling lights on and at 13:00. -S47ShotDir=<dir> puts the PNGs there.
// Pictures for a look, no acceptance. Room (SARO metres): x -6.0..-1.0, z 6.8..10.5; Yaw 0 looks north (-z).
namespace
{
 struct FRecView{const TCHAR* Name;double X,Z,TX,TY,TZ;float Fov;};
 const FRecView RecViews[]={
  {TEXT("01-door-looking-in"),-2.95,7.05,-4.3,0.9,10.2,75},
  {TEXT("02-corner-ne"),-1.35,7.1,-5.0,1.0,10.0,80},
  {TEXT("03-corner-se"),-1.45,9.9,-5.2,1.0,7.2,80},
  {TEXT("04-corner-sw"),-5.2,9.45,-1.5,1.0,7.2,80},
  {TEXT("05-corner-nw"),-5.2,7.25,-1.6,1.0,10.0,80},
  {TEXT("06-desk-close"),-3.65,7.55,-3.65,0.8,8.6,65},
  {TEXT("07-filing-cabinets"),-4.45,8.2,-4.45,0.8,10.1,70},
  {TEXT("08-shelves-west"),-3.9,8.6,-5.8,1.2,8.6,75},
  {TEXT("09-north-wall-door"),-3.0,10.0,-3.5,1.4,6.8,80},
  {TEXT("10-clerk-desk-card-index"),-2.5,8.9,-1.4,0.9,7.4,70},
  {TEXT("11-bookcase-binders"),-2.4,8.9,-2.4,0.8,10.3,70},
  {TEXT("12-ceiling-lights"),-3.5,10.0,-3.5,2.8,7.6,80},
  {TEXT("13-floor"),-2.2,7.3,-4.0,0.0,9.4,75},
  // RECORDS-ROOM-LIFT-20261113: the lift's pieces close up, through a long lens from where one stands (21:00 only; names from h)
  {TEXT("h01-typewriter"),-4.12,7.85,-4.12,0.86,8.68,30},
  {TEXT("h02-bankers-lamp"),-3.05,7.80,-3.07,1.00,8.35,30},
  {TEXT("h03-table-top"),-2.45,8.00,-3.55,0.77,8.62,45},
  {TEXT("h04-shelving-cartons"),-4.85,8.95,-5.60,1.35,9.62,50},
  {TEXT("h05-filing-cabinets"),-4.46,9.25,-4.62,0.95,10.0,60},
  {TEXT("h06-desk-fan"),-3.60,9.30,-3.93,1.50,10.17,28},
  {TEXT("h07-extinguisher-switch"),-4.60,7.95,-4.95,1.00,6.85,45},
  {TEXT("h08-cork-board"),-2.30,7.95,-1.75,1.55,6.82,40},
  {TEXT("h09-second-shelving-nook"),-2.30,9.45,-1.30,1.10,8.90,60},
  {TEXT("h10-troffer"),-4.20,9.00,-4.60,2.70,8.00,50},
  {TEXT("h11-wall-base-floor"),-2.30,9.30,-1.05,0.40,9.70,45},
  {TEXT("h12-bookcase"),-2.40,9.30,-2.40,0.60,10.30,55},
 };
 constexpr int32 RecCount=UE_ARRAY_COUNT(RecViews);
}

void FS47FreeformTest::StepRecordsShots(double Dt)
{
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>300){Finish(false,FString::Printf(TEXT("records shots exceeded the time bound at stage %d"),Stage));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->World)return;
 auto& F=*M->Freeform;
 static FString Dir;
 if(Dir.IsEmpty()){if(!FParse::Value(FCommandLine::Get(),TEXT("S47ShotDir="),Dir))Dir=Root/TEXT("records");IFileManager::Get().MakeDirectory(*Dir,true);}
 auto ToHour=[&](double Hours){const double Want=FMath::Fmod(Hours*3600.,86400.),Now=FMath::Fmod(F.Clock(),86400.);F.TestAdvance(FMath::Fmod(Want-Now+86400.,86400.));};
 auto Pose=[&](const FRecView& V)
 {
  AS47Walker& P=*M->Player;
  P.bHeadMotion=false;P.Place(V.X,V.Z,0);P.SettleOnFloor();
  const double DX=V.TX-V.X,DZ=V.TZ-V.Z,DY=V.TY-(P.FloorY+P.Eye);
  P.Yaw=FMath::Atan2(-DX,-DZ);P.Pitch=FMath::Clamp(FMath::Atan2(DY,FVector2D(DX,DZ).Size()),-1.3,1.2);P.Fov=V.Fov;
 };
 // Stage 0: night 21:00; 100+I: pose I; 200+I: shoot I; Stage 1: day 13:00; then the same from 300/400.
 if(Stage==0){if(Age<3)return;M->UI->Close(true);ToHour(21.);Go(100,6.);return;}
 const bool bDay=Stage>=300;const int32 Base=bDay?300:100;const int32 I=(Stage-Base)%100;const bool bShoot=Stage-Base>=100;
 if(Stage==1){ToHour(13.);Go(300,8.);return;}
 if(I>=RecCount){if(!bDay){Go(1,.5);return;}Finish(true,FString::Printf(TEXT("records room: %d views at 21:00 and 13:00 in %s"),RecCount,*Dir));return;}
 if(bDay&&RecViews[I].Name[0]==TEXT('h')){Go(Stage+1,.05);return;}
 if(!bShoot){Pose(RecViews[I]);Go(Stage+100,I==0?4.:2.);return;}
 const FString Name=FString::Printf(TEXT("%s-%s"),RecViews[I].Name,bDay?TEXT("1300"):TEXT("2100"));
 FScreenshotRequest::RequestScreenshot(Dir/(Name+TEXT(".png")),false,false);
 UE_LOG(LogTemp,Display,TEXT("S47 records shot %s at %.2f,%.2f floor %.2f yaw %.2f pitch %.2f area %s"),*Name,M->Player->Pos.X,M->Player->Pos.Z,M->Player->FloorY,M->Player->Yaw,M->Player->Pitch,*M->World->Area);
 Go(Stage-100+1,.6);
}
