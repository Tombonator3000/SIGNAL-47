#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47Flow.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47World.h"
#include "UnrealClient.h"

// FIX-20261110 (Tom, point 2), -S47SkyCutTest: a light that crosses over SARO takes the mains. The feeder sags first (dips, torn
// screens), then the station is dark and the night goes on; the standby generator behind the control room carries it, the line
// recorder's strip is kept as a record, and the mains come back by themselves.
void FS47FreeformTest::StepSkyCut(double Dt)
{
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>150){Finish(false,FString::Printf(TEXT("sky cut exceeded its time bound at stage %d"),Stage));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->World||!M->Story||!M->Freeform->Director||!M->Freeform->Field)return;
 auto& F=*M->Freeform;auto& D=*F.Director;
 auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Root/Name,true,false);};
 if(Stage==0)
 {
  if(Age<3)return;M->UI->Close(true);
  const double Now=FMath::Fmod(F.Clock(),86400.);F.TestAdvance(FMath::Fmod(22*3600.-Now+86400.,86400.));
  F.Field->TestGeneratorFuel();
  if(!Check(F.HasPowerForReactiveLight()&&!F.SkyCutLive()&&!F.Field->GeneratorRunning(),TEXT("an ordinary evening: mains on, the generator fuelled and standing still")))return;
  RescueClock=F.Clock();D.SkyCutNext(6,8);
  if(!Check(D.Force(TEXT("flyover")),TEXT("the flyover starts")))return;
  if(!Check(F.SkySagging()&&D.SkyCutHeld()&&F.HasPowerForReactiveLight(),TEXT("the feeder sags first: the light waits, the mains still hold")))return;
  Go(1,2.5);return;
 }
 if(Stage==1)
 {
  if(!Check(F.SkySagging()&&F.ScreenInterference()>.15f,FString::Printf(TEXT("the screens tear while it sags (%.2f)"),F.ScreenInterference())))return;
  Shot(TEXT("sky-cut-sag.png"));Go(2);return;
 }
 if(Stage==2){if(!F.SkyMainsOut())return;Go(3,1.);return;}
 if(Stage==3)
 {
  if(!Check(!F.HasPowerForReactiveLight()&&M->Story->bDark,TEXT("then the mains are gone and the station is dark")))return;
  if(!Check(!D.SkyCutHeld()&&D.ActiveEvent()==TEXT("flyover"),TEXT("and the light comes over in the dark")))return;
  if(!Check(F.Clock()>RescueClock+1&&!F.PowerFault(),TEXT("the night goes on: the clock runs, and the feeder has no fault to mend")))return;
  Shot(TEXT("sky-cut-dark.png"));Go(4,.8);return;
 }
 if(Stage==4)
 {
  if(!Check(bool(M->Player->OnUse),TEXT("the use callback is ready")))return;
  M->Player->Place(4.9,9.2,0);M->Player->OnUse(TEXT("ff:generator"));
  if(!Check(F.Field->GeneratorRunning(),TEXT("the standby generator behind the control room starts on its tank")))return;
  Go(5,1.);return;
 }
 if(Stage==5)
 {
  if(!Check(F.HasPowerForReactiveLight()&&!M->Story->bDark&&F.SkyMainsOut(),TEXT("it carries the station while the mains are still out")))return;
  M->Player->Place(0,-1.2,UE_DOUBLE_PI);Go(6,1.);return;
 }
 if(Stage==6){Shot(TEXT("sky-cut-generator.png"));Go(7);return;}
 if(Stage==7)
 {
  if(F.SkyCutLive()||D.Busy())return;
  if(!Check(F.HasFinding(TEXT("mains-ride")),TEXT("the line recorder's strip is kept as a record of the passage")))return;
  M->Player->Place(4.9,9.2,0);M->Player->OnUse(TEXT("ff:generator"));
  if(!Check(!F.Field->GeneratorRunning()&&F.HasPowerForReactiveLight(),TEXT("the mains came back by themselves: the generator is stopped and the station stays lit")))return;
  if(!Check(!F.Save()->HasField(TEXT("skySag"))&&!F.Save()->HasField(TEXT("skyOut")),TEXT("nothing of it is in the save")))return;
  Finish(true,TEXT("Sky cut: the feeder sags, the mains go under the passage, the generator carries the station and leaves a record, the mains return by themselves."));return;
 }
}
