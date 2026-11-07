#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Album.h"
#include "S47FarmSite.h"
#include "S47FieldCamera.h"
#include "S47FieldRadio.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47FreeformDirector.h"
#include "S47Inter.h"
#include "S47LeftGear.h"
#include "S47Night.h"
#include "S47Startup.h"
#include "S47Taken.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"

// -S47Freeform -S47FreeformTest -S47FreeLifeTest: the chain as a player meets it, with the live rules on. A field visit from its
// own draw and dwell, taken, the gear left behind, the case, the note, going back for the gear, and the originals untouched.
void FS47FreeformTest::StepFreeLife(double Dt)
{
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>900){Finish(false,TEXT("life journey exceeded its time"));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->Inter)return;
 auto& F=*M->Freeform;if(!F.FarmSite||!F.Taken||!F.Gear)return;auto& Site=*F.FarmSite;
 static int32 ShotsBefore=0,Night=0;
 if(Stage==0)
 {
  if(Age<3)return;
  M->UI->Close(true);F.TestLiveRules();F.TestEndFirstShift();
  if(F.Director)F.Director->RestFor(100000);
  if(M->FieldRadio)M->FieldRadio->TestOwn();
  if(M->FieldCamera)M->FieldCamera->bHave=true;
  ShotsBefore=M->Night&&M->Night->Album?M->Night->Album->Shots().Num():0;
  // The first night after five that is not a calm one, an hour into their own hours.
  Night=6;while(F.QuietNight(Night))++Night;
  const double Midnight=double(Night)*86400.;
  const double Target=Midnight+(F.BodyStartHour(Night)+.25)*3600.;
  if(!Check(F.TestAdvance(Target-F.Clock())&&F.TheirHoursNow(F.Clock()),TEXT("the clock stands in the night's own hours")))return;
  if(!Check(!F.TheirHoursNow(Midnight+.5*3600.),TEXT("half an hour after midnight is not their hours")))return;
  // Past the farm, away from the station, in the open.
  M->Player->Place(90,620,0);
  Site.TestFieldRoll(0.f);Stage=10;At=Age;return;
 }
 if(Stage==10)
 {
  if(!F.TheirHoursNow(F.Clock()))return Finish(false,TEXT("the night ran out before the draw"));
  if(!Site.FieldVisit())
  {
   if(Age-At>FS47FarmSite::FieldDwellSeconds+40)return Finish(false,FString::Printf(TEXT("no field visit from the draw: drawn night %d, want %d, indoors %d"),Site.FieldDrawnNight(),Night,F.Taken->Indoors()));
   return;
  }
  if(!Check(Age-At>=FS47FarmSite::FieldDwellSeconds-1&&Site.FieldDrawnNight()==F.Day(),TEXT("a field visit starts from its own draw, once they have been out there a while")))return;
  if(!Check(Site.VisitStage()>=1&&Site.VisitWantsGround(),TEXT("the visit has begun")))return;
  Stage=20;At=Age;return;
 }
 if(Stage==20)
 {
  if(Site.VisitStage()<2)
  {
   if(Age-At>60)return Finish(false,TEXT("nothing came out on the field visit"));
   return;
  }
  UE_LOG(LogTemp,Display,TEXT("S47: free life: field visit body kind %d, %.1f m off, %.0f s"),Site.VisitKind(),Site.VisitBodyDistance(),Age-At);
  if(!Check(Site.VisitBodyDistance()>=0,TEXT("a body is out on the field visit")))return;
  if(!Check(F.Taken->TakeNow(ES47Taker::Tall,TEXT("the open ground past the SARO fence")),TEXT("reached by it")))return;
  Stage=30;At=Age;return;
 }
 if(Stage==30)
 {
  if(F.Taken->Busy()){if(Age-At>40)return Finish(false,TEXT("taken never finished"));return;}
  if(!Check(F.Gear->IsLeft(TEXT("radio"))&&F.Gear->IsLeft(TEXT("camera")),TEXT("the tall one leaves the set and the camera where it happened")))return;
  if(!Check(!M->FieldRadio->Owned()&&!M->FieldCamera->bHave&&F.CameraLeftOut()&&M->FieldRadio->LeftOut(),TEXT("neither is on the player, and neither has gone back to the desk or bench")))return;
  const int32 Shots=M->Night&&M->Night->Album?M->Night->Album->Shots().Num():0;
  if(!Check(Shots>=ShotsBefore,TEXT("no photograph has gone")))return;
  const auto* Lost=F.CaseFile().Get(FS47FreeformCases::LostLine);
  if(!Check(Lost&&Lost->bOpen,TEXT("the case of the lost hours is open")))return;
  if(!Check(F.Taken->LastNote().Contains(TEXT("must still be where I stood")),TEXT("the note says what is missing")))return;
  const auto Saved=F.Save();
  if(!Check(Saved->HasField(TEXT("gear")),TEXT("what lies out there is saved")))return;
  UE_LOG(LogTemp,Display,TEXT("S47: free life: %s"),*F.Taken->LastNote());
  Stage=40;At=Age;return;
 }
 if(Stage==40)
 {
  // Back to it on foot, or as good as: it is the player's hands that pick it up.
  const auto& Items=F.Gear->Items();
  if(Items.IsEmpty())
  {
   if(!Check(M->FieldRadio->Owned()&&M->FieldCamera->bHave&&!F.CameraLeftOut(),TEXT("both are back in hand")))return;
   if(!Check(F.HasFinding(TEXT("lost_ground")),TEXT("going back gave the case its trace")))return;
   Finish(true,TEXT("field visit, taken, left gear, case and fetch all as played"));return;
  }
  const FVector Where=Items[0].At;
  if(Age-At<1)return;At=Age;
  M->Player->Place(Where.X+.6,Where.Z,0);
  M->Inter->UseId(TEXT("ff:left:")+Items[0].Item);
  return;
 }
}
