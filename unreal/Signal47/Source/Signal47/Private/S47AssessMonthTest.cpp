#include "S47Predecessor.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47Mail.h"
#include "S47Main.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Startup.h"
#include "S47PredecessorText.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

// M7 (9 November): Ward's assessment as it comes in a game, not forced. -S47Freeform -S47AssessMonthTest=<profile> with an isolated
// -UserDir=/tmp/...: a developer run (nothing is saved), the live rules on, and the clock advanced six hours at a time through the
// real FS47Predecessor::StepNews, with the post's round and the box emptied as a player would. One profile per run:
//  routine       the card only, nothing looked into: no Stays, a week's notice, then the station closes.
//  investigator  the locker, every tag, the binder's gap, wagon 23, the deputy's second file and the routed pay: KnowsEnough
//                when the work does not keep the station (or Stays, with the finale, if it does).
//  taken         three stretches of lost time (days 6, 8 and 10): Ward's warning after the second, somebody else the morning after
//                the third, "Not yet", and then an assessment by the work that is not Gone.
// Each checks the letter in the box at the road (and the flag of the box with something in it), that OPS says only ASSESSMENT MADE
// until the letter is read, the outcome after it, and the closing day. Heavy (about 30 game days, a few seconds each): a night job.
// The routine numbers of a whole month (27 of 27, three jobs a day, twenty night sheets) are in FS47Predecessor::SelfTest; filing
// real jobs here would need the receiver's own fixture.
namespace
{
 struct FState
 {
  bool bChecked=false,bWanted=false,bDone=false;
  FString Profile,Root;
  int32 Stage=0,Checks=0,LastFetchDay=0,TakenGiven=0;
  double Age=0,Hold=0,Since=0;
  TArray<FString> Lines;
 };
 FState& State(){static FState S;return S;}
}

void FS47AssessMonthTest::Step(FS47Predecessor& P,double Dt)
{
 FState& S=State();
 if(!S.bChecked)
 {
  S.bChecked=true;
  FString Profile;
  if(FParse::Value(FCommandLine::Get(),TEXT("S47AssessMonthTest="),Profile))S.bWanted=true;
  else if(FParse::Param(FCommandLine::Get(),TEXT("S47AssessMonthTest"))){S.bWanted=true;Profile=TEXT("routine");}
  S.Profile=Profile.ToLower();
 }
 if(!S.bWanted||S.bDone)return;
 US47Main* M=P.M;
 const auto Finish=[&S](bool bGood,const FString& Why)
 {
  if(S.bDone)return;S.bDone=true;
  auto O=S47J::New();S47J::Set(O,TEXT("status"),bGood?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("profile"),S.Profile);S47J::Set(O,TEXT("checks"),S.Checks);S47J::Set(O,TEXT("lines"),S.Lines);
  S47J::Set(O,TEXT("method"),TEXT("Developer run, live rules, the clock advanced six hours at a time through the real StepNews, the post's round made and the box emptied each day. Profiles are fixtures for the player's choices (papers read, lost time recorded), not natural play."));
  if(!S.Root.IsEmpty())FFileHelper::SaveStringToFile(S47J::Write(O),*(S.Root/FString::Printf(TEXT("assess-month-%s.json"),*S.Profile)));
  UE_LOG(LogTemp,Display,TEXT("S47 assess month %s: %s: %s (%d checks)"),*S.Profile,bGood?TEXT("PASS"):TEXT("FAIL"),*Why,S.Checks);
  FPlatformMisc::RequestExitWithStatus(false,bGood?0:1);
 };
 const auto Check=[&S,&Finish](bool bGood,const FString& What)
 {
  ++S.Checks;S.Lines.Add(FString::Printf(TEXT("%s %s"),bGood?TEXT("PASS"):TEXT("FAIL"),*What));
  UE_LOG(LogTemp,Display,TEXT("S47 assess month: %s %s"),bGood?TEXT("PASS"):TEXT("FAIL"),*What);
  if(!bGood)Finish(false,What);
  return bGood;
 };
 S.Age+=FMath::Min(Dt,.2);
 if(S.Age>2400){Finish(false,TEXT("the month did not get to its end in time"));return;}
 if(S.Hold>0){S.Hold-=Dt;return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->UI||!M->Freeform->Predecessor.IsValid())return;
 auto& F=*M->Freeform;
 if(F.Predecessor.Get()!=&P)return;
 using EO=FS47Predecessor::EOutcome;
 const bool bTaken=S.Profile==TEXT("taken"),bLooks=S.Profile==TEXT("investigator");

 // The post: the round, the box emptied, and what came read, once a day; the assessment is left for its own stage.
 const auto Post=[&]()
 {
  if(!F.Mail||!F.Mail->Live()||S.LastFetchDay==F.Day())return;
  const double Hour=FMath::Fmod(F.Clock(),86400.)/3600.;if(Hour<13.6)return;
  S.LastFetchDay=F.Day();
  FString Why;F.Mail->TestRound(Why);
  if(F.Mail->MailIn())
  {
   if(F.Mail->Holds(TEXT("ward:assessment")))Check(P.TaskLine().StartsWith(TEXT("MAILBOX / Dr. Ward's assessment")),FString::Printf(TEXT("day %d: the assessment is in the box at the road, and the list says so"),F.Day()));
   F.Mail->TestFetch();M->UI->Close(true);
  }
  TArray<FString> Read=P.Tray;
  for(const FString& Id:Read)if(Id!=TEXT("assessment")){P.ReadDoc(Id);M->UI->Close(true);}
 };

 if(S.Stage==0)
 {
  if(S.Age<3)return;
  FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
  if(!Check(User.StartsWith(TEXT("/tmp/"))&&M->Flow->bDevRun,TEXT("an isolated temporary profile and a developer run, which is never saved")))return;
  if(!Check(S.Profile==TEXT("routine")||bTaken||bLooks,TEXT("a known profile: routine, investigator or taken")))return;
  S.Root=FPaths::ProjectSavedDir()/TEXT("FreeformTest");IFileManager::Get().MakeDirectory(*S.Root,true);
  M->UI->Close(true);F.TestLiveRules();F.TestEndFirstShift();
  if(!Check(P.AssessmentDay()==FS47Predecessor::FirstAssess&&P.Result()==EO::None&&P.StatusLine().StartsWith(TEXT("UNDER REVIEW / DAY")),TEXT("a new game: assessed on day 30, under review")))return;
  S.Stage=1;S.Hold=2.5;return;
 }
 if(S.Stage==1)
 {
  // Ward's letter in the box at the road with the shift begun, the box showing it, and the list pointing at it.
  if(F.Mail&&F.Mail->Live()&&F.Mail->Holds(TEXT("ward:letter")))
  {
   if(!Check(F.Mail->MailIn()&&P.TaskLine().StartsWith(TEXT("MAILBOX / A letter")),TEXT("the first letter is in the box at the road, and on the list")))return;
   F.Mail->TestFetch();M->UI->Close(true);
  }
  if(!P.Tray.Contains(TEXT("letter"))&&!P.Found(TEXT("letter"))){if(S.Age>60)Finish(false,TEXT("Ward's letter never came"));return;}
  if(P.Tray.Contains(TEXT("letter"))){P.ReadDoc(TEXT("letter"));M->UI->Close(true);}
  if(bLooks)
  {
   // What an investigating player turns up in the first fortnight, all at once: the trail's own production paths.
   P.OpenLocker();M->UI->Close(true);
   for(const TCHAR* Id:{TEXT("note"),TEXT("list"),TEXT("home"),TEXT("pay"),TEXT("mail"),TEXT("gap")}){P.ReadDoc(Id);M->UI->Close(true);}
   for(const S47PredecessorText::FTagDef& D:S47PredecessorText::Tags()){P.ReadDoc(D.Doc);M->UI->Close(true);}
   P.OpenWagon();M->UI->Close(true);P.ReadDoc(TEXT("callout"));M->UI->Close(true);
   P.ShowTalk(TEXT("deputy"),TEXT("ortiz"));M->UI->Close(true);
   P.bPayReturned=true;
   if(!Check(P.Known()>=FS47Predecessor::KnownEnough&&P.Found(TEXT("wagon"))&&P.Found(TEXT("gap")),TEXT("the investigator knows what can be known")))return;
  }
  S.Stage=2;S.Since=S.Age;return;
 }
 if(S.Stage==2)
 {
  // The month, six hours at a time.
  Post();
  if(bTaken)
  {
   const int32 Want=F.Day()>=10?3:F.Day()>=8?2:F.Day()>=6?1:0;
   while(S.TakenGiven<Want){++S.TakenGiven;F.RecordEvidence(FString::Printf(TEXT("taken-assess-month-%d"),S.TakenGiven),TEXT("A stretch of the night nobody can account for (assess month fixture)."));}
   if(S.TakenGiven>=2&&F.Day()>=9&&!Check(P.Found(TEXT("ward-taken")),TEXT("after the second stretch Ward has written what a third does")))return;
   if(P.Result()==EO::Gone)
   {
    if(!Check(F.Day()==11&&F.TimesTaken()==3,TEXT("somebody else has the nights the morning after the third, not on day 30")))return;
    if(!Check(M->ModalOpen()&&P.StatusLine().StartsWith(TEXT("UNDER REVIEW")),TEXT("the letter to the next operator is open, and OPS gives nothing away")))return;
    P.TestNotYet();M->UI->Close(true);
    if(!Check(P.Result()==EO::None&&P.AssessmentDay()>=FS47Predecessor::FirstAssess&&P.TakenAtReprieve==3,TEXT("\"Not yet\": the nights back, the review no sooner than day 30, and the three remembered")))return;
    S.Stage=3;return;
   }
  }
  if(P.Result()!=EO::None){S.Stage=4;S.Since=S.Age;return;}
  if(F.Day()>P.AssessmentDay()+1){Finish(false,TEXT("the assessment day went by with nothing decided"));return;}
  if(!F.TestAdvance(6*3600.)){S.Hold=1;return;}
  M->UI->Close(true);S.Hold=2.6;return;
 }
 if(S.Stage==3)
 {
  // After the reprieve: the rest of the month with no new lost time.
  Post();
  if(!Check(P.Result()!=EO::Gone,TEXT("without a new stretch of lost time nobody is sent")))return;
  if(P.Result()!=EO::None){S.Stage=4;S.Since=S.Age;return;}
  if(F.Day()>P.AssessmentDay()+1){Finish(false,TEXT("the assessment after the reprieve went by with nothing decided"));return;}
  if(!F.TestAdvance(6*3600.)){S.Hold=1;return;}
  M->UI->Close(true);S.Hold=2.6;return;
 }
 if(S.Stage==4)
 {
  // Decided: OPS says only that it is made until the letter is read, and the letter comes by post.
  const EO Got=P.Result();
  if(!Check(Got!=EO::Gone,TEXT("the assessment by the work is not Gone")))return;
  if(!Check(P.StatusLine()==TEXT("UNDER REVIEW / ASSESSMENT MADE")&&!P.OutcomeRead(),TEXT("decided, not read: OPS says ASSESSMENT MADE and nothing else")))return;
  if(S.Profile==TEXT("routine")&&!Check(Got!=EO::Stays,TEXT("the card alone does not keep SARO open")))return;
  if(bLooks&&!Check(Got==EO::Stays||Got==EO::KnowsEnough,TEXT("the investigator stays, or knows enough")))return;
  const bool bInBox=F.Mail&&F.Mail->Live()?F.Mail->Holds(TEXT("ward:assessment"))||F.Mail->Coming(TEXT("ward:assessment")):P.Tray.Contains(TEXT("assessment"));
  if(!Check(bInBox||P.Tray.Contains(TEXT("assessment")),TEXT("the assessment is on its way by post, or in the tray")))return;
  S.Stage=5;return;
 }
 if(S.Stage==5)
 {
  if(!P.Tray.Contains(TEXT("assessment")))
  {
   if(F.Mail&&F.Mail->Live()&&F.Mail->Holds(TEXT("ward:assessment"))){F.Mail->TestFetch();M->UI->Close(true);return;}
   if(S.Age-S.Since>300){Finish(false,TEXT("the assessment never came"));return;}
   FString Why;if(F.Mail&&F.Mail->Live())F.Mail->TestRound(Why);
   S.Hold=1;return;
  }
  P.OpenAssessment();
  const EO Got=P.Result();
  if(!Check(P.OutcomeRead()&&M->ModalOpen(),TEXT("the letter is read")))return;
  M->UI->Close(true);
  if(Got==EO::Stays)
  {
   if(!Check(P.StatusLine()==TEXT("KEPT OPEN")&&P.KeptOpen()&&P.Found(TEXT("kept"))&&P.TaskLine().StartsWith(TEXT("SARO KEPT OPEN")),TEXT("read: KEPT OPEN, and the list has the station's new horizon")))return;
   if(bLooks&&!Check(F.FindingText(P.AssessKey()).Contains(TEXT("wagon 23")),TEXT("the Stays letter says what came of the wagon")))return;
   S.Stage=7;S.Since=S.Age;return;
  }
  if(!Check(P.StatusLine().StartsWith(TEXT("CLOSING"))&&P.ClosingDay()==P.AssessmentDay()+FS47Predecessor::NoticeDays&&P.TaskLine().StartsWith(TEXT("SARO CLOSES")),TEXT("read: CLOSING with a week's notice, and on the list")))return;
  S.Stage=6;return;
 }
 if(S.Stage==6)
 {
  // The notice runs out and the order takes effect.
  if(P.StationClosed())
  {
   if(!Check(F.Day()>=P.ClosingDay()&&P.StatusLine()==TEXT("CLOSED"),TEXT("the station closes on the day the letter gave")))return;
   M->UI->Close(true);Finish(true,FString::Printf(TEXT("%s: assessed, told by post, read, and closed on the day"),FS47Predecessor::NameOf(P.Result())));return;
  }
  if(F.Day()>P.ClosingDay()+1){Finish(false,TEXT("the closing day went by and the station stayed open"));return;}
  if(!F.TestAdvance(6*3600.)){S.Hold=1;return;}
  M->UI->Close(true);S.Hold=2.6;return;
 }
 if(S.Stage==7)
 {
  // Kept open: the array run up and back from Dale's bench, the first quiet daytime moment at the station.
  if(P.Found(TEXT("kept-salute")))
  {
   if(!Check(!M->Web->DishesMoving(),TEXT("the array is back where it was pointed")))return;
   Finish(true,TEXT("stays: assessed, told by post, read, and the station's finale"));return;
  }
  if(M->World&&M->World->Area!=TEXT("saro")){M->World->Enter(TEXT("saro"));S.Hold=2;return;}
  const double Hour=FMath::Fmod(F.Clock(),86400.)/3600.;
  if(P.SaluteStage==0&&(Hour<9||Hour>=18)){if(F.TestAdvance(3*3600.))M->UI->Close(true);S.Hold=2.6;return;}
  if(S.Age-S.Since>400){Finish(false,TEXT("the array was never run up after KEPT OPEN"));return;}
  M->UI->Close(true);S.Hold=1;return;
 }
}
