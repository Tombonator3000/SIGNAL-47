#include "S47Predecessor.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47FreeformWorld.h"
#include "S47WardReport.h"
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
//  taken-again   as taken, and a fourth stretch on day 14 (critic round 5): somebody else the morning after, with no second "Not yet".
//  (CHAIN-2-20261109: Stays also wants one of the cases carried through to have something brought in from out on the land in it,
//  FS47FreeformSession::CaseFromField. Routine has none; the investigator carries no case and files no work, so still KnowsEnough.)
//  cases         the station's work and a case carried through (critic round 5: no profile went down the Stays path): the array
//                mended, the day's jobs filed as a fixture, the weekly reports sent, and THE SAME MINUTE with both its traces kept
//                and filed. Stays, KEPT OPEN read, and the run up of the array from Dale's bench.
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
 const bool bAgain=S.Profile==TEXT("taken-again"),bTaken=S.Profile==TEXT("taken")||bAgain,bLooks=S.Profile==TEXT("investigator"),bCases=S.Profile==TEXT("cases");
 // The cases profile's work, as a fixture (filing real jobs needs the receiver's own): each job up to today filed clean and paid, the
 // weekly report sent whole when it is due.
 const auto Work=[&]()
 {
  if(!bCases)return;
  for(FS47FreeformSession::FJob& J:F.Work)if(J.CaseTest<0&&J.Day<=F.Day()&&J.Stage!=FS47FreeformSession::EJobStage::Filed)
  {J.Stage=FS47FreeformSession::EJobStage::Filed;J.RecordingSoundVersion=2;J.RecordingQuality=.9;F.Earn(20);}
  if(F.WardReport)for(const FS47WardReport::FReport& Rep:TArray<FS47WardReport::FReport>(F.WardReport->Reports()))if(Rep.Sent<0)F.WardReport->Send(Rep.Week);
 };

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
   // Critic round 5: her Monday note in the box is on the list too, not only on the map.
   else if(F.Mail->Holds(TEXT("ward:week-1")))Check(P.TaskLine().StartsWith(TEXT("MAILBOX /")),FString::Printf(TEXT("day %d: Ward's Monday note is in the box at the road, and the list says so"),F.Day()));
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
  if(!Check(S.Profile==TEXT("routine")||bTaken||bLooks||bCases,TEXT("a known profile: routine, investigator, taken, taken-again or cases")))return;
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
  if(bCases)
  {
   // The array mended, and THE SAME MINUTE seen at the rack: open, and a filing on that alone would count for nothing.
   if(F.Field)F.Field->RestoreWholeArrayFixture();
   const int32 Line=FS47FreeformCases::MysteryLine;
   F.RecordEvidence(FS47FreeformCases::Def(Line).Mystery->Clues[0].Finding,F.CaseFile().ClueText(Line,0));
   const auto* K=F.CaseFile().Get(Line);
   if(!Check(K&&K->bOpen&&!F.CaseSupported(Line),TEXT("the same minute opens on the chart at the rack, and one trace has nothing behind it")))return;
   // The feeder looked at too: carried through, and filed.
   F.RecordEvidence(FS47FreeformCases::Def(Line).Mystery->Clues[1].Finding,F.CaseFile().ClueText(Line,1));
   int32 Cause=0;while(Cause<FS47FreeformCases::CauseCount-1&&F.CaseFile().Excluded(Line,Cause))++Cause;
   if(!Check(F.CaseSupported(Line)&&F.ConcludeCase(Line,Cause)&&F.CaseFile().Get(Line)->bConcluded,TEXT("with the feeder looked at too, the same minute is carried through and filed")))return;
   M->UI->Close(true);
   if(!Check(P.Count().Concluded==1&&P.Count().Guessed==0,TEXT("Ward counts the filing as a case carried through")))return;
   // CHAIN-2-20261109: the remote feeder is out on the land, so the case has something brought in from out there in it
   if(!Check(P.Count().Field==1&&F.CaseFromField(Line),TEXT("CHAIN-2: the feeder looked at out on the county road makes it a case with something from out on the land")))return;
   Work();
  }
  S.Stage=2;S.Since=S.Age;return;
 }
 if(S.Stage==2)
 {
  // The month, six hours at a time.
  Post();Work();
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
  // After the reprieve: the rest of the month with no new lost time; or (taken-again) a fourth stretch, and no second "Not yet".
  Post();
  if(bAgain)
  {
   if(F.Day()>=14&&S.TakenGiven<4){++S.TakenGiven;F.RecordEvidence(TEXT("taken-assess-month-4"),TEXT("Another stretch of the night nobody can account for (assess month fixture)."));}
   if(P.Result()==EO::Gone)
   {
    if(!Check(F.Day()==15&&F.TimesTaken()==4,TEXT("after the reprieve a fourth stretch sends somebody else the morning after")))return;
    if(!Check(!P.OffersNotYet()&&M->ModalOpen(),TEXT("the letter to the next operator has no second \"Not yet\"")))return;
    const int32 Was=P.AssessmentDay();P.TestNotYet();M->UI->Close(true);
    if(!Check(P.Result()==EO::Gone&&P.AssessmentDay()==Was,TEXT("and nothing gives the nights back a second time")))return;
    Finish(true,TEXT("taken again after \"Not yet\": somebody else, once and for all"));return;
   }
   if(F.Day()>16){Finish(false,TEXT("the fourth stretch of lost time sent nobody"));return;}
  }
  else if(!Check(P.Result()!=EO::Gone,TEXT("without a new stretch of lost time nobody is sent")))return;
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
  // CHAIN-2-20261109: and the Monday notes said where it stood, the same line every week
  if(S.Profile==TEXT("routine")&&!Check(P.Count().Field==0&&F.FindingText(TEXT("ward-week-1")).Contains(TEXT("brought in from out on the land: 0. I need one.")),TEXT("CHAIN-2: no case from out on the land, and her Monday note had the line for it")))return;
  if(bCases&&!Check(F.FindingText(TEXT("ward-week-1")).Contains(TEXT("brought in from out on the land: 1.")),TEXT("CHAIN-2: the cases profile's Monday note counts its case from out on the land")))return;
  // Critic round 5: the investigator carries no case through, so only knowing enough is right; the cases profile stays.
  if(bLooks&&!Check(Got==EO::KnowsEnough,TEXT("the investigator, with no case carried through, knows enough and does not stay")))return;
  if(bCases&&!Check(Got==EO::Stays,FString::Printf(TEXT("the work and a case carried through keep SARO open (points %d of %d, filed %d, array %d)"),FS47Predecessor::Points(P.Count()),FS47Predecessor::PointsToStay,P.Count().Filed,P.Count().Dishes)))return;
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
