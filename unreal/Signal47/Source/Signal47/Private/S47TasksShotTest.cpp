#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47Flow.h"
#include "S47Places.h"
#include "S47Story.h"
#include "S47Startup.h"
#include "S47TonightNote.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47World.h"
#include "UnrealClient.h"

// TASKS-TEN-20261113, -S47TasksShotTest: the task surfaces as the player sees them, shot for a look (not a human acceptance): the HUD's
// card bright and then resting dim in its corner, the pause menu's TONIGHT card, the notebook's NOW with OPEN MATTERS and a pinned row,
// the notebook's map with "OPS: N job cards up". Night 2 at 20:30 with a bearing slip out and Dale's ask kept, then 01:40, then the
// resting card on bright ground outdoors at noon. Fixtures: the clock jumps, the bearing (as a save has it), Dale's words, the pin.
void FS47FreeformTest::StepTasksShots(double Dt)
{
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>240){Finish(false,FString::Printf(TEXT("task shots exceeded the time bound at stage %d"),Stage));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->World)return;
 auto& F=*M->Freeform;
 auto Shoot=[&](const FString& Name){FScreenshotRequest::RequestScreenshot(Root/(Name+TEXT(".png")),true,false);UE_LOG(LogTemp,Display,TEXT("S47 task shot %s at %s: HUD «%s»"),*Name,*S47TonightNote::HourMinute(F.Clock()),*F.HudObjective());};
 // to the shift clock's HH:MM on the night's day (After: hours past 08:00 of the shift day)
 auto To=[&](int32 Day,double ShiftHours){for(int32 I=0;I<4&&F.Day()<Day;++I)F.TestAdvance(FMath::Fmod(8.2*3600.-FMath::Fmod(F.Clock(),86400.)+86400.,86400.)+60.);
  const double Want=FMath::Fmod(ShiftHours*3600.,86400.),Now=FMath::Fmod(F.Clock(),86400.);F.TestAdvance(FMath::Fmod(Want-Now+86400.,86400.));};
 if(Stage==0)
 {
  if(Age<3)return;M->UI->Close(true);F.TestLiveRules();F.TestEndFirstShift();
  To(2,20.5);
  if(!Check(F.Day()==2&&FMath::IsNearlyEqual(FMath::Fmod(F.Clock(),86400.),20.5*3600.,120.),FString::Printf(TEXT("the clock stands at 20:30 on night 2 (day %d, %s)"),F.Day(),*S47TonightNote::HourMinute(F.Clock()))))return;
  // a bearing out tonight, as a save keeps one (the self-test's own fixture)
  if(F.Places)
  {
   const FS47Places::FBearing Fix=FS47Places::StationFix(F.Day(),F.Clock(),0,33,-300,-560,1340,.5f,TEXT("mine"));
   auto Fx=S47J::New();S47J::Set(Fx,TEXT("day"),F.Day());S47J::Set(Fx,TEXT("az"),Fix.Az);S47J::Set(Fx,TEXT("spread"),Fix.Spread);S47J::Set(Fx,TEXT("target"),Fix.Target);
   S47J::Set(Fx,TEXT("clock"),Fix.Clock);S47J::Set(Fx,TEXT("until"),Fix.Until);S47J::Set(Fx,TEXT("to"),TArray<double>{Fix.ToX,Fix.ToZ});S47J::Set(Fx,TEXT("ring"),TArray<double>{Fix.RingX,Fix.RingZ,Fix.RingR});
   auto Pl=F.Places->Save();if(!Pl.IsValid())Pl=S47J::New();
   TArray<TSharedPtr<FJsonValue>> Fixes=Pl->HasTypedField<EJson::Array>(TEXT("fixes"))?Pl->GetArrayField(TEXT("fixes")):TArray<TSharedPtr<FJsonValue>>();
   Fixes.Add(MakeShared<FJsonValueObject>(Fx));Pl->SetArrayField(TEXT("fixes"),Fixes);F.Places->Begin(Pl);
  }
  if(!Check(F.SlipToFollow(),TEXT("a live bearing slip is out tonight")))return;
  F.DaleSaid(TEXT("Write it in the night log, what you saw and when."));
  if(!Check(F.DaleAskRows().Num()>=1,TEXT("Dale's ask is kept as a row")))return;
  Go(1,2.);return;
 }
 if(Stage==1){Shoot(TEXT("01-hud-2030-bright"));Go(2,8.);return;}
 if(Stage==2){Shoot(TEXT("02-hud-2030-dim"));Go(3,.4);return;}
 if(Stage==3){M->Flow->OpenPause();Go(4,1.);return;}
 if(Stage==4)
 {
  if(!Check(M->bPausedByMenu,TEXT("the pause menu is open")))return;
  Shoot(TEXT("03-pause-tonight-2030"));Go(5,.4);return;
 }
 if(Stage==5)
 {
  M->UI->Close();
  // pin the bearing slip's row, as F2 on NOW does
  for(const FS47Task& T:F.Tasks())if(T.Text.StartsWith(TEXT("BEARING SLIP /"))){F.PinRow(S47TonightNote::KeyOf(T.Text),T.Text);break;}
  if(!Check(!F.PinnedKey().IsEmpty(),TEXT("the bearing slip's row is pinned")))return;
  M->UI->NotebookAt(TEXT("tasks"));M->Flow->OpenNotebook();Go(6,1.);return;
 }
 if(Stage==6){Shoot(TEXT("04-notebook-now-2030-pinned"));Go(7,.4);return;}
 if(Stage==7){M->UI->Close();M->UI->NotebookAt(TEXT("map"));M->Flow->OpenNotebook();Go(8,1.2);return;}
 if(Stage==8){Shoot(TEXT("05-notebook-map-2030"));Go(9,.4);return;}
 if(Stage==9){M->UI->Close();F.PinRow(F.PinnedKey(),FString());To(2,25.+40./60.);Go(10,2.);return;}
 if(Stage==10){Shoot(TEXT("06-hud-0140-bright"));Go(11,8.);return;}
 if(Stage==11){Shoot(TEXT("07-hud-0140-dim"));M->Flow->OpenPause();Go(12,1.);return;}
 if(Stage==12){Shoot(TEXT("08-pause-tonight-0140"));Go(13,.4);return;}
 if(Stage==13){M->UI->Close();M->UI->NotebookAt(TEXT("tasks"));M->Flow->OpenNotebook();Go(14,1.);return;}
 if(Stage==14){Shoot(TEXT("09-notebook-now-0140"));Go(15,.4);return;}
 if(Stage==15)
 {
  // the resting card on bright ground: outdoors at noon, looking down at the yard
  M->UI->Close();To(3,13.);M->Player->Place(-12.,5.7,UE_DOUBLE_HALF_PI);M->Player->Pitch=-.35;Go(16,9.);return;
 }
 if(Stage==16){Shoot(TEXT("10-hud-1300-outdoors-dim"));Go(17,.6);return;}
 if(Stage==17)
 {
  Finish(true,TEXT("Task shots: the HUD card bright and resting, TONIGHT, NOW with OPEN MATTERS and a pin, the map's OPS line, at 20:30 and 01:40 of night 2 and outdoors at noon."));return;
 }
}
