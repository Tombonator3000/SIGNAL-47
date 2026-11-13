#include "S47Freeform.h"
#include "S47GameStyle.h"
#include "S47Main.h"
#include "S47Observing.h"
#include "S47Predecessor.h"
#include "S47RecordsBinder.h"
#include "S47StationFiles.h"
#include "S47UI.h"
#include "SS47Binder.h"

// RECORDS-BINDER (-> M3, Tom 12 November): the session's part of the records-room binder. What is in the binder is the session's
// own (BinderSource: the jobs with an original, the findings, the cases, the station's files); how it is filed is S47RecordsBinder,
// how it looks is SS47Binder. Filing itself is FilePacket, unchanged: this only asks it, and notes the day and clock it said yes.

S47Binder::FSource FS47FreeformSession::BinderSource() const
{
 S47Binder::FSource Src;Src.Today=DayNumber;Src.Day1=DateOfDay(1);Src.bAtBinder=NearBinder();
 // The packet the next filing puts in is FilePacket's own choice: the job in hand if its packet waits, else the first that does.
 const FJob* Waiting=CurrentJob()&&CurrentJob()->Stage==EJobStage::Packet?CurrentJob():Work.FindByPredicate([](const FJob& X){return X.Stage==EJobStage::Packet;});
 if(Waiting)Src.ToFileId=Waiting->Id;
 const auto& RunList=FS47Observing::Runs();
 for(const FJob& J:Work)
 {
  // An original exists once the chart is printed; before that there is no paper to list.
  if(!J.bPrinted&&J.OriginalPage.IsEmpty()&&J.Stage!=EJobStage::Packet&&J.Stage!=EJobStage::Filed)continue;
  S47Binder::FJobIn JobIn;JobIn.Id=J.Id;JobIn.Source=RunList.IsValidIndex(J.Source)?FString(RunList[J.Source].Source):FString(TEXT("SOURCE"));
  JobIn.Day=J.Day;JobIn.bFiled=J.Stage==EJobStage::Filed;JobIn.bPacket=J.Stage==EJobStage::Packet;JobIn.bControl=J.bControl;JobIn.bAnomaly=J.bAnomaly;
  JobIn.ControlAt=J.ControlAt;JobIn.MarkStart=J.MarkStart;JobIn.MarkEnd=J.MarkEnd;JobIn.Original=J.OriginalPage;
  JobIn.Paid=J.bPaid?J.PaidAmount:-1;JobIn.Scheduled=J.bGenerated?J.Card.SuggestedClock:-1.;
  if(const FBinderStamp* Stamp=BinderFiled.Find(J.Id)){JobIn.FiledDay=Stamp->Day;JobIn.FiledClock=Stamp->Clock;}
  if(J.CaseTest>=0&&J.CaseTest<FS47FreeformCases::TestCount&&J.CaseLine>=0&&J.CaseLine<FS47FreeformCases::CaseCount)
  {
   // A case test's recording: the test by name, and its result once it is registered.
   const FS47FreeformCases::FCase* Sheet=Cases.Get(J.CaseLine);
   JobIn.CaseLine=J.CaseLine;JobIn.TestName=FS47FreeformCases::Def(J.CaseLine).Tests[J.CaseTest].Name;
   JobIn.Showed=!J.bCaseValid?FString(TEXT("TEST INVALID / COUNTS FOR NOTHING"))
    :J.bCaseRecorded&&Sheet&&Sheet->Results[J.CaseTest].bDone?FS47FreeformCases::TestResultText(J.CaseLine,J.CaseTest,Sheet->Results[J.CaseTest].bPresent)
    :FString(TEXT("RESULT NOT YET REGISTERED"));
  }
  else
  {
   // The registered result in the station's own words (the same words the OPS record uses).
   JobIn.Showed=!J.bAnomaly?TEXT("ROUTINE RESULT"):!J.bAudioReviewed?TEXT("COMPARISON PENDING"):J.Phenomenon==TEXT("wrong_source")?TEXT("OFF-SOURCE RESPONSE")
    :J.Phenomenon==TEXT("time_disagreement")?TEXT("CLOCK DISAGREEMENT"):J.Phenomenon==TEXT("duplicate_print")?TEXT("DUPLICATE ORIGINAL"):TEXT("SOURCE UNRESOLVED");
   // An unusual original belongs with the case its kind opens, once that case is open.
   // Critic 12 Nov: the plain extra-tick take (GAMEPLAY-LOOP-01) has no phenomenon of its own: it belongs with Case 01, the pulsar's.
   if(J.bAnomaly)JobIn.CaseLine=PlainAnomaly(J)?0:FS47FreeformCases::LineForFinding(J.Phenomenon);
  }
  Src.Jobs.Add(MoveTemp(JobIn));
 }
 for(const TPair<FString,FString>& Kept:Findings)Src.Findings.Add({Kept.Key,Kept.Value,FindingDays.FindRef(Kept.Key)});
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)
 {
  const FS47FreeformCases::FCase* Sheet=Cases.Get(Line);if(!Sheet||!Sheet->bOpen)continue;
  const FS47FreeformCases::FCaseDef& Def=FS47FreeformCases::Def(Line);
  S47Binder::FCaseIn CaseIn;CaseIn.Line=Line;CaseIn.Title=Def.Title;CaseIn.Kind=CaseKind(Line);CaseIn.Question=Cases.QuestionText(Line);
  CaseIn.OpenDay=Sheet->OpenDay;CaseIn.OpenClock=Sheet->OpenClock;CaseIn.ConcludeDay=Sheet->ConcludeDay;CaseIn.Folded=Sheet->FoldedLeads;
  CaseIn.bConcluded=Sheet->bConcluded;CaseIn.bAssessment=Def.Mystery!=nullptr;CaseIn.bReplied=Sheet->bReplied;CaseIn.Leads=Sheet->Leads;
  if(Sheet->bConcluded)CaseIn.Next=CaseNextLine(Line);
  // The sheet's head as the case board has it, the explanations numbered: how it closes, where it came from, how each stands.
  CaseIn.Head=CaseClosesWhen(Line)+TEXT("\n")+CaseSource(Line);
  for(int32 Cause=0;Cause<FS47FreeformCases::CauseCount;++Cause)CaseIn.Head+=FString::Printf(TEXT("\n%d. %s / %s"),Cause+1,*FString(Def.Causes[Cause]).ToUpper(),*CaseStanding(Line,Cause));
  for(int32 Test=0;Test<FS47FreeformCases::TestCount;++Test)if(Sheet->Results[Test].bDone)
   CaseIn.Tests.Add(FString::Printf(TEXT("TEST %d / %s / D%02d / %s"),Test+1,*FString(Def.Tests[Test].Name).ToUpper(),Sheet->Results[Test].Day,*FS47FreeformCases::TestResultText(Line,Test,Sheet->Results[Test].bPresent)));
  CaseIn.Reply=FindingText(FString::Printf(TEXT("case%d-reply"),Line+1));
  Src.Cases.Add(MoveTemp(CaseIn));
 }
 // The station's own files, as the duty officer releases them.
 for(int32 At=0;At<S47StationFiles::Count();++At)
 {
  const S47StationFiles::FFile& File=S47StationFiles::Get(At);
  if(File.Day>DayNumber){++Src.FilesHeld;continue;}
  Src.Files.Add({File.Key,File.Title,File.Text,File.Day,At<32&&(StationFilesSeen&(1u<<At))!=0});
 }
 if(Predecessor)Src.Older=Predecessor->ArchiveLabel();
 BinderUnknowns(Src); // EVIDENCE-DOSSIER-20261113: the files of the unknown, behind UNKNOWNS (S47FreeformDossier.cpp)
 return Src;
}
void FS47FreeformSession::OpenBinder()
{
 // A packet in hand comes first, wherever the binder was left open: it opens at TODAY'S FILING with the packet chosen.
 if(Work.ContainsByPredicate([](const FJob& X){return X.Stage==EJobStage::Packet;}))BinderView->Tab=-1;
 ShowBinder();
}
void FS47FreeformSession::BinderShow(const FString& EntryKey,int32 Page)
{
 const S47Binder::FModel Built=S47Binder::Build(BinderSource());
 if(const int32 At=Built.Find(EntryKey);At>=0){BinderView->Tab=Built.Entries[At].Home;BinderView->Pick=EntryKey;BinderView->Page=FMath::Max(0,Page);}
 ShowBinder();
}
void FS47FreeformSession::ShowBinder()
{
 if(!M||!M->UI)return;
 const TWeakObjectPtr<US47Main> Owner=M;
 // The binder may outlive a session that ended under it: nothing of this session is touched then.
 const auto Mine=[this,Owner]{return Owner.IsValid()&&Owner->Freeform.Get()==this;};
 SetPanel(SNew(SS47Binder).View(BinderView)
  .Source([this,Mine]{return Mine()?S47Binder::Build(BinderSource()):S47Binder::FModel();})
  .OnFile([this,Mine](FString& Words){return Mine()&&FileAtBinder(Words);})
  .OnShown([this,Mine](const FString& EntryKey){if(Mine())BinderRead(EntryKey);})
  .OnOlder([this,Mine]{if(Mine()&&Predecessor)Predecessor->OpenGap();})
  .OnDo([this,Mine](const FString& Do,const FString& Typed,FString& Words){return Mine()&&DossierAct(Do,Typed,Words);}) // EVIDENCE-DOSSIER slice 3
  .OnLeave([this,Mine]{if(Mine()&&M&&M->UI)M->UI->Close();}));
}
bool FS47FreeformSession::FileAtBinder(FString& Words)
{
 const FJob* Waiting=CurrentJob()&&CurrentJob()->Stage==EJobStage::Packet?CurrentJob():Work.FindByPredicate([](const FJob& X){return X.Stage==EJobStage::Packet;});
 if(!Waiting){Words=TEXT("No packet in hand.");return false;}
 const FString Id=Waiting->Id;const int32 CashBefore=Cash,LoanBefore=Loan;
 const bool bFiled=FilePacket();
 UE_LOG(LogTemp,Display,TEXT("S47: binder filing %s: %s"),*Id,bFiled?TEXT("filed"):*Feedback);
 if(!bFiled){Words=Feedback;return false;}
 // The stamp: the shift day it went in and the clock of that day. Kept for the page; filing itself keeps nothing new.
 FBinderStamp Stamp;Stamp.Day=DayNumber;Stamp.Clock=FMath::Fmod(FMath::Max(0.,WorldClock),86400.);BinderFiled.Add(Id,Stamp);
 const int32 Credit=(Cash-CashBefore)+(LoanBefore-Loan);
 Words=FString::Printf(TEXT("FILED BEHIND DAILY WORK / %s %s."),*S47Game::DateLine(DateOfDay(int32(FMath::FloorToDouble(WorldClock/86400.))+1)),*FS47FreeformCases::Clock(WorldClock));
 if(Credit>0)Words+=FString::Printf(TEXT(" $%d CREDITED."),Credit);
 return true;
}
void FS47FreeformSession::BinderRead(const FString& EntryKey)
{
 // A station file on the page is a file read (StationFileDue), on paper as on the glass.
 const FString Head=S47Binder::FileKey(FString());if(!EntryKey.StartsWith(Head))return;
 const FString FileId=EntryKey.Mid(Head.Len());
 for(int32 At=0;At<S47StationFiles::Count()&&At<32;++At)if(FileId==S47StationFiles::Get(At).Key)StationFilesSeen|=1u<<At;
}
void FS47FreeformSession::LoadBinder(const S47J::FObj& Saved)
{
 BinderFiled.Reset();*BinderView=S47Binder::FView();
 for(const TSharedPtr<FJsonValue>& V:S47J::Arr(Saved,TEXT("binderFiled")))
 {
  if(!V||V->Type!=EJson::Object)continue;const S47J::FObj Kept=V->AsObject();if(!Kept)continue;
  const FString Id=S47J::Str(Kept,TEXT("id"));const double OnDay=S47J::Num(Kept,TEXT("day")),AtClock=S47J::Num(Kept,TEXT("clock"),-1);
  if(Id.IsEmpty()||Id.Len()>96||!FMath::IsFinite(OnDay)||!FMath::IsFinite(AtClock)||OnDay<1||AtClock<0||BinderFiled.Num()>=4096)continue;
  FBinderStamp Stamp;Stamp.Day=int32(FMath::Min(OnDay,36500.));Stamp.Clock=FMath::Min(AtClock,86399.);BinderFiled.Add(Id,Stamp);
 }
}
void FS47FreeformSession::SaveBinder(const S47J::FObj& Record) const
{
 // Only for packets still in the work list and still filed; a record with none writes nothing (an older save reads the same).
 TArray<TSharedPtr<FJsonValue>> Stamps;
 for(const FJob& J:Work)if(const FBinderStamp* Stamp=J.Stage==EJobStage::Filed?BinderFiled.Find(J.Id):nullptr)
 {
  const S47J::FObj Kept=S47J::New();S47J::Set(Kept,TEXT("id"),J.Id);S47J::Set(Kept,TEXT("day"),Stamp->Day);S47J::Set(Kept,TEXT("clock"),Stamp->Clock);
  Stamps.Add(MakeShared<FJsonValueObject>(Kept));
 }
 if(!Stamps.IsEmpty())Record->SetArrayField(TEXT("binderFiled"),Stamps);
}
bool FS47FreeformSession::BinderSelfTest(TArray<FString>& Lines)
{
 bool bOk=true;
 const auto Check=[&bOk,&Lines](bool bGood,const TCHAR* Name){bOk&=bGood;Lines.Add(FString(bGood?TEXT("PASS "):TEXT("FAIL "))+TEXT("RECORDS-BINDER: ")+Name);};
 FS47FreeformSession Old(nullptr);Old.Begin(nullptr);
 if(Old.Work.IsEmpty()){Check(false,TEXT("a new game has the day's cards to file"));Old.Stop();return false;}
 Check(!Old.Save()->HasField(TEXT("binderFiled"))&&Old.BinderSource().Jobs.IsEmpty()&&S47Binder::Build(Old.BinderSource()).Land()==S47Binder::SecIndex,
  TEXT("with nothing printed the binder has no original and opens at its index, and the record carries no stamp"));
 FJob& First=Old.Work[0];First.Stage=EJobStage::Filed;First.bPrinted=true;First.bPaid=true;const FString Id=First.Id;
 FBinderStamp Stamp;Stamp.Day=1;Stamp.Clock=21*3600.+14*60.;Old.BinderFiled.Add(Id,Stamp);Old.BinderFiled.Add(TEXT("NOT-A-JOB"),Stamp);
 const S47J::FObj Kept=S47J::Read(S47J::Write(Old.Save()));Old.Stop();
 {
  FS47FreeformSession Fresh(nullptr);Fresh.Begin(Kept);
  const FBinderStamp* Back=Fresh.BinderFiled.Find(Id);
  Check(Back&&Back->Day==1&&FMath::IsNearlyEqual(Back->Clock,Stamp.Clock)&&Fresh.BinderFiled.Num()==1,TEXT("the day and clock a packet went in survive a Continue; a stamp without its filed job is not kept"));
  const S47Binder::FSource Src=Fresh.BinderSource();
  const S47Binder::FJobIn* Handed=Src.Jobs.FindByPredicate([&Id](const S47Binder::FJobIn& X){return X.Id==Id;});
  const S47Binder::FModel Built=S47Binder::Build(Src);const int32 At=Built.Find(S47Binder::JobKey(Id));
  Check(Handed&&Handed->bFiled&&Handed->FiledDay==1&&At>=0&&Built.Entries[At].Home==S47Binder::SecDaily&&Built.Entries[At].StatusLine.Contains(TEXT(" 21:14"))&&Built.Entries[At].Stamp.StartsWith(TEXT("FILED "))&&Built.Latest.Contains(At),
   TEXT("a filed job is one stamped page behind DAILY WORK, dated from the record"));
  Fresh.Stop();
 }
 {
  Kept->RemoveField(TEXT("binderFiled"));
  FS47FreeformSession Before(nullptr);Before.Begin(Kept);
  const S47Binder::FModel Built=S47Binder::Build(Before.BinderSource());const int32 At=Built.Find(S47Binder::JobKey(Id));
  Check(Before.BinderFiled.IsEmpty()&&At>=0&&Built.Entries[At].Status==TEXT("FILED")&&Built.Entries[At].Stamp==TEXT("FILED")&&!Built.Entries[At].StatusLine.Contains(TEXT("21:14"))&&!Before.Save()->HasField(TEXT("binderFiled")),
   TEXT("a record from before the stamps reads as filed on its day alone, and writes none"));
  Before.Stop();
 }
 return bOk;
}
