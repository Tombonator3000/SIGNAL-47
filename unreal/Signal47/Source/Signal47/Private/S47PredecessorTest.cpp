#include "S47FreeformTest.h"
#include "S47Album.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Places.h"
#include "S47Docs.h"
#include "S47Predecessor.h"
#include "S47PredecessorText.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Legs.h"

// -S47Freeform -S47FreeformTest -S47ReyesTest: the trail as a player meets it, with the live rules on. The date, Ward's letter, the
// locker, every tag at its own place and where it points, wagon 23 and its radio, the people, the review, each way it ends, and the
// keys and taking them back. Originals, photographs and findings are counted before and after: nothing here takes one away.
void FS47FreeformTest::StepReyes(double Dt)
{
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>600){Finish(false,TEXT("Reyes journey exceeded its time"));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->Inter)return;
 auto& F=*M->Freeform;if(!F.Predecessor||!F.Places)return;auto& P=*F.Predecessor;
 static int32 Findings=0,Photos=0,TagAt=0,OutcomeAt=1;
 const auto PhotoCount=[this]{return M->Night&&M->Night->Album?M->Night->Album->Shots().Num():0;};
 if(Stage==0)
 {
  if(Age<3)return;
  M->UI->Close(true);F.TestLiveRules();F.TestEndFirstShift();P.TestOn();
  const FDateTime Date=F.Date();
  if(!Check(Date.GetYear()==1986&&Date.GetMonth()==4&&Date.GetDay()==28&&Date.GetDayOfWeek()==EDayOfWeek::Monday&&!F.OldCalendar(),TEXT("a new game starts on Monday April 28, 1986")))return;
  if(!Check(F.Save()->HasField(TEXT("calendar"))&&S47J::Obj(F.Save(),TEXT("predecessor")).IsValid(),TEXT("the calendar and the trail are in the save")))return;
  if(!Check(F.DateOfDay(P.AssessmentDay()).GetMonth()==5&&F.DateOfDay(P.AssessmentDay()).GetDay()==27,TEXT("the assessment is thirty days out: Tuesday May 27")))return;
  if(!Check(P.StatusLine().Contains(TEXT("UNDER REVIEW")),TEXT("OPS says SARO is under review")))return;
  Stage=10;At=Age;return;
 }
 if(Stage==10)
 {
  if(!P.Found(TEXT("letter"))&&P.TrayCount()==0)
  {
   if(Age-At>30)return Finish(false,TEXT("Ward's letter never came"));
   return;
  }
  if(!Check(P.TaskLine().Contains(TEXT("letter")),TEXT("the letter stands on the list of things to do")))return;
  F.OpenOperations();Stage=11;At=Age;Hold=2.5;return;
 }
 if(Stage==11){Shot(TEXT("reyes-ops-tray"));Stage=12;Hold=.4;return;}
 if(Stage==12)
 {
  M->UI->Close(true);
  P.ReadDoc(TEXT("letter"));
  if(!Check(!M->Story->Docs().FindByPredicate([](const FS47Doc& D){return D.Id==TEXT("reyes_letter")&&!D.Page.Contains(TEXT("MAY 27"));}),TEXT("the letter names the date of the assessment")))return;
  if(!Check(!M->Story->Docs().FindByPredicate([](const FS47Doc& D){return D.Id==TEXT("reyes_letter")&&!D.Page.Contains(TEXT("nobody else wanted it"));}),TEXT("and says why you got the job")))return;
  Stage=13;Hold=.4;return;
 }
 if(Stage==13){Shot(TEXT("reyes-letter"));Stage=20;Hold=.4;return;
 }
 if(Stage==20)
 {
  M->UI->Close(true);
  Findings=F.FindingCount();Photos=PhotoCount();
  M->Player->Place(26.31+1.7,-.5,UE_DOUBLE_HALF_PI);M->Player->Pitch=0;
  Stage=19;Hold=1.5;return;
 }
 if(Stage==19){Shot(TEXT("reyes-locker-bank"));Stage=21;Hold=.5;return;}
 if(Stage==21)
 {
  M->Inter->UseId(TEXT("ff:reyes:locker"));
  if(!Check(P.Found(TEXT("locker")),TEXT("the locker opens where it stands")))return;
  Stage=23;Hold=.4;return;
 }
 if(Stage==23){Shot(TEXT("reyes-locker"));Stage=22;Hold=.4;return;}
 if(Stage==22)
 {
  M->UI->Close(true);
  for(const TCHAR* Doc:{TEXT("note"),TEXT("home"),TEXT("pay"),TEXT("mail"),TEXT("list")}){P.ReadDoc(Doc);M->UI->Close(true);}
  if(!Check(P.Heard(TEXT("windmill"))&&F.Places->Heard(TEXT("windmill")),TEXT("the clipboard points at the windmill: a question mark on the map")))return;
  M->World->Enter(TEXT("road"));TagAt=0;Stage=30;Hold=.5;return;
 }
 if(Stage==30)
 {
  const auto& Tags=S47PredecessorText::Tags();
  if(TagAt>=Tags.Num())
  {
   if(!Check(P.Known()>=10,TEXT("the locker and all the tags count")))return;
   Stage=40;return;
  }
  const auto& Tag=Tags[TagAt];FString Area;double X=0,Z=0;
  if(!Check(P.WorldSpot(Tag.Doc,Area,X,Z),TEXT("the tag stands somewhere")))return;
  M->Player->Place(X,Z+1.6,0);Stage=31;Hold=.4;return;
 }
 if(Stage==31)
 {
  const auto& Tag=S47PredecessorText::Tags()[TagAt];const auto* Doc=S47PredecessorText::Doc(Tag.Doc);
  M->Inter->UseId(FString(TEXT("ff:reyes:"))+Tag.Doc);
  if(!Check(P.Found(Tag.Doc),TEXT("the tag reads")))return;
  if(Doc->Next&&!Check(P.Heard(Doc->Next)&&F.Places->Heard(Doc->Next),TEXT("and points on to the next place")))return;
  ++TagAt;Stage=32;Hold=.4;return;
 }
 if(Stage==32){if(TagAt==1)Shot(TEXT("reyes-tag"));Stage=33;Hold=.2;return;}
 if(Stage==33){M->UI->Close(true);if(TagAt==1){Stage=34;Hold=1;return;}Stage=30;return;}
 if(Stage==34){Shot(TEXT("reyes-tag-marker"));Stage=30;Hold=.3;return;}
 if(Stage==40)
 {
  if(!Check(P.Talk(TEXT("deputy")),TEXT("the deputy has words for the new operator")))return;
  M->UI->Close(true);
  if(!Check(P.Talk(TEXT("dale")),TEXT("Dale picks up")))return;
  M->UI->Close(true);
  if(!Check(P.UsePhone(),TEXT("the control room phone offers Dale and Ward")))return;
  M->UI->Close(true);
  if(!Check(P.TestBark(),TEXT("Dale comes over the walkie: the locker, a tag or a letter has given him cause")))return;
  if(!Check(P.Talk(TEXT("attendant"))&&P.Talk(TEXT("host"))&&P.Talk(TEXT("radio_woman"))&&P.Talk(TEXT("souvenir_man"))&&P.Talk(TEXT("ward")),TEXT("so does everyone else")))return;
  M->UI->Close(true);
  P.TestFound(TEXT("gap"));M->UI->Close(true);
  P.TestHear(TEXT("wagon"));
  FString Area;double X=0,Z=0;
  if(!Check(P.WorldSpot(TEXT("wagon"),Area,X,Z)&&Area==TEXT("road"),TEXT("the wagon is on the land")))return;
  M->Player->Place(X,Z+9,0);M->Player->Pitch=0;Stage=44;Hold=2;return;
 }
 if(Stage==44){Shot(TEXT("reyes-wagon-world"));Stage=41;Hold=.4;return;}
 if(Stage==41)
 {
  M->Inter->UseId(TEXT("ff:reyes:wagon"));
  if(!Check(P.Found(TEXT("wagon"))&&P.RadioOn(),TEXT("wagon 23 stands there, with its radio on")))return;
  Stage=45;Hold=.4;return;
 }
 if(Stage==45){Shot(TEXT("reyes-wagon"));Stage=42;Hold=.4;return;}
 if(Stage==42)
 {
  M->UI->Close(true);
  P.ReadDoc(TEXT("callout"));M->UI->Close(true);
  if(!Check(P.TestRadioOff(),TEXT("the radio goes off")))return;
  F.TestAdvance(86400);
  if(!Check(P.RadioOn(),TEXT("and is on again the next day")))return;
  Stage=50;OutcomeAt=1;return;
 }
 if(Stage==50)
 {
  // Each way it can end: a letter, what it leaves, and nothing taken away.
  if(OutcomeAt>5){Stage=60;return;}
  const auto Outcome=FS47Predecessor::EOutcome(OutcomeAt);
  P.TestOutcome(Outcome);
  if(!Check(P.Result()==Outcome,TEXT("the outcome stands")))return;
  if(!Check(F.FindingCount()>=Findings&&PhotoCount()>=Photos,TEXT("no finding and no photograph has gone")))return;
  Hold=.4;Stage=52;return;
 }
 if(Stage==52){Shot(FString::Printf(TEXT("reyes-outcome-%s"),FS47Predecessor::NameOf(FS47Predecessor::EOutcome(OutcomeAt))));++OutcomeAt;Hold=.4;Stage=51;return;}
 if(Stage==51){M->UI->Close(true);Stage=50;return;}
 if(Stage==60)
 {
  // Through the real rule, from what this run really did.
  P.TestReset();
  if(!Check(F.FindingCount()>=Findings,TEXT("still everything found")))return;
  P.TestAssessNow();M->UI->Close(true);
  UE_LOG(LogTemp,Display,TEXT("S47: Reyes test: real assessment gives %s"),FS47Predecessor::NameOf(P.Result()));
  if(!Check(P.Result()!=FS47Predecessor::EOutcome::None,TEXT("the assessment reads the game and decides")))return;
  // Back through the save.
  const auto Saved=S47J::Read(S47J::Write(F.Save()));
  if(!Check(S47J::Str(S47J::Obj(Saved,TEXT("predecessor")),TEXT("outcome"))==FS47Predecessor::NameOf(P.Result()),TEXT("the outcome is saved")))return;
  Stage=70;return;
 }
 if(Stage==70)
 {
  M->World->Enter(TEXT("saro"));M->Player->Place(26.09+1.3,.16,0);Stage=71;Hold=.5;return;
 }
 if(Stage==71)
 {
  M->Inter->UseId(TEXT("ff:reyes:keys"));
  if(!Check(!P.HasLeft(),TEXT("the board asks first")))return;
  Stage=74;Hold=.4;return;
 }
 if(Stage==74){Shot(TEXT("reyes-keys-question"));Stage=72;Hold=.4;return;}
 if(Stage==72)
 {
  P.TestLeave();
  if(!Check(P.HasLeft()&&S47J::Bool(S47J::Obj(F.Save(),TEXT("predecessor")),TEXT("left")),TEXT("the keys are on the hook, and the save knows")))return;
  Stage=75;Hold=.4;return;
 }
 if(Stage==75){Shot(TEXT("reyes-left"));Stage=73;Hold=.4;return;}
 if(Stage==73)
 {
  P.TestTakeBack();
  if(!Check(!P.HasLeft()&&!S47J::Bool(S47J::Obj(F.Save(),TEXT("predecessor")),TEXT("left")),TEXT("and they can be taken back")))return;
  if(!Check(F.FindingCount()>=Findings&&PhotoCount()>=Photos,TEXT("every finding and photograph is still there at the end")))return;
  UE_LOG(LogTemp,Display,TEXT("S47: Reyes test: date, letter, locker, tags, wagon, people, outcomes, assessment and keys PASS"));
  Finish(true,TEXT("date, letter, locker, tags, wagon, people, outcomes, assessment and keys as played"));return;
 }
}
