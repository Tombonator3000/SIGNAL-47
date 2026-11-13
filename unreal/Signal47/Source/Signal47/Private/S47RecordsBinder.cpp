#include "S47RecordsBinder.h"
#include "S47Dossier.h"
#include "S47FreeformCases.h"
#include "S47GameStyle.h"
#include "S47Script.h"

// RECORDS-BINDER (-> M3): the binder's model. See S47RecordsBinder.h.
namespace S47BinderLocal
{
 using namespace S47Binder;
 // The binder's own words. Everything else on its pages is the session's (the originals, the records, the cases, the files).
 const TCHAR* const Tabs[SecCount]={TEXT("INDEX"),TEXT("TODAY"),TEXT("CASES"),TEXT("DAILY WORK"),TEXT("FIELD + BENCH"),TEXT("UNKNOWNS"),TEXT("STATION FILES")};
 const TCHAR* const Titles[SecCount]={TEXT("INDEX"),TEXT("TODAY'S FILING"),TEXT("CASES"),TEXT("DAILY WORK ARCHIVE"),TEXT("FIELD AND BENCH RECORDS"),TEXT("UNKNOWNS"),TEXT("STATION FILES")};
 const TCHAR* const Empties[SecCount]={
  TEXT(""),
  TEXT("Nothing filed or retained today.\n\nA packet from the printer is filed here, by hand. What is retained in the field or at the bench is listed here the same day."),
  TEXT("No case opened.\n\nA case is opened at OPS. Its sheet is kept here, with every page that belongs to it listed under it."),
  TEXT("No original retained.\n\nEach observation's original is listed here by day once it is printed. It is marked FILED when its packet is in this binder."),
  TEXT("No record retained.\n\nWhat is kept from the field, the station round and the workshop bench is listed here by day."),
  TEXT("No file opened.\n\nWhat the operator logs in the field about something not yet known is kept here, one file for each, as the notebook has it. At this binder a file is given a working name, and two files can be filed as one."),
  TEXT("No file released.\n\nThe duty officer releases the station's files at 08:00.")};
 const TCHAR* const TodayGroups[4]={TEXT("TO FILE"),TEXT("FILED TODAY"),TEXT("RETAINED TODAY"),TEXT("STILL AT OPS")};
 const TCHAR* const FiledWords=TEXT("This original is in the binder under its reference.");
 const TCHAR* const PacketWords=TEXT("This packet is in hand. Filed here, it goes behind DAILY WORK.");
 const TCHAR* const HeldWords=TEXT("This original is held at OPS. No packet of it has been filed here.");
 const TCHAR* const OpenCaseWords=TEXT("OPEN. WORKED AT OPS, ON CASE BOARD. THIS SHEET KEEPS ITS PAPER TOGETHER.");
 const TCHAR* const QuestionWords=TEXT("STILL UNKNOWN / Whether this repeats under independent conditions.\nPOSSIBLE NEXT CHECK / Retain a later observation and its independent control; compare with this original.");
 const TCHAR* const UnknownKind=TEXT("OPERATOR'S FILE"); // EVIDENCE-DOSSIER-20261113
 const TCHAR* const FrontHead=TEXT("SARO / RECORDS\nRECORDS ROOM BINDER\nKEPT BY THE OPERATOR ON SHIFT");
 const TCHAR* const FrontRules=TEXT("HOW IT IS KEPT\n\n1. A packet from the printer is filed here, by hand. It goes behind DAILY WORK the day it is filed.\n\n2. A page that belongs to a case is listed again under that case.\n\n3. Field and bench records are listed by the day they were retained.\n\n4. Station files are read only. The duty officer releases them at 08:00.\n\nNothing is removed from this binder.");
 const TCHAR* const FrontCopy=TEXT("READING COPY. Filing is done at the binder in the records room.");
 const TCHAR* const RowWords[]={TEXT("TO FILE"),TEXT("IN HAND"),TEXT("WAITING"),TEXT("LAST FILED"),TEXT("ON THE SAME SHELF"),TEXT("OLDER BINDER"),TEXT("READ IT"),TEXT("TURN TO THE PAGE"),TEXT("TURN TO "),
  TEXT("FILE THE PACKET"),TEXT("FILED"),TEXT("AT OPS"),TEXT("RETAINED"),TEXT("READ"),TEXT("NEW"),TEXT("OPEN"),TEXT("CONCLUDED"),TEXT("HELD AT OPS / NOT IN THIS BINDER"),TEXT("TO FILE / PACKET IN HAND"),
  TEXT("RETAINED / READ ONLY"),TEXT("FILED ORIGINAL"),TEXT("ORIGINAL"),TEXT("FIELD RECORD"),TEXT("BENCH RECORD"),TEXT("CASE FILE"),TEXT("STATION FILE"),TEXT("THE ORIGINAL AS PRINTED"),TEXT("CONTROL   NOT RECORDED"),
  TEXT("TESTS REGISTERED"),TEXT("NO TEST REGISTERED."),TEXT("UNDATED"),TEXT("ALL FILES RELEASED / READ ONLY"),TEXT("MORE WITH THE DUTY OFFICER / RELEASED AT 08:00, NOT EVERY MORNING")};

 FString Clock(double DayClock){return FS47FreeformCases::Clock(DayClock);}
 bool Starts(const FString& Key,std::initializer_list<const TCHAR*> Heads){for(const TCHAR* H:Heads)if(Key.StartsWith(H))return true;return false;}
 // A record's own first sentence names it; internal keys stay internal.
 FString TitleOf(const FString& Record)
 {
  FString First=Record;int32 Stop=INDEX_NONE;
  if(First.FindChar(TEXT('\n'),Stop)&&Stop>0)First=First.Left(Stop);
  if(First.FindChar(TEXT('.'),Stop)&&Stop>0)First=First.Left(Stop);
  First.TrimStartAndEndInline();
  return First.Len()>58?First.Left(55)+TEXT("..."):First;
 }
 int32 Cost(const FString& Para,int32 Wide){return FMath::Max(1,FMath::DivideAndRoundUp(Para.Len(),Wide));}
}

namespace S47Binder
{
FString JobKey(const FString& Id){return TEXT("job:")+Id;}
FString RecordKey(const FString& FindingKey){return TEXT("rec:")+FindingKey;}
FString CaseKey(int32 Line){return FString::Printf(TEXT("case:%d"),Line);}
FString FileKey(const FString& Key){return TEXT("file:")+Key;}
FString UnknownKey(int32 Number){return FString::Printf(TEXT("unk:%d"),Number);}
FString DateOf(const FSource& In,int32 Day)
{
 return Day<=0?FString(TEXT("UNDATED")):S47Game::DateLine(In.Day1+FTimespan::FromDays(FMath::Min(Day,36500)-1));
}
bool BenchKey(const FString& Key)
{
 return S47BinderLocal::Starts(Key,{TEXT("service"),TEXT("workshop"),TEXT("donor"),TEXT("feeder-repair"),TEXT("sample")});
}
bool OpenQuestionKey(const FString& Key)
{
 return S47BinderLocal::Starts(Key,{TEXT("extra_pulse"),TEXT("feeder_fault"),TEXT("wrong_source"),TEXT("roof_steps"),TEXT("observer"),TEXT("peek-"),TEXT("flood-room-"),TEXT("door-visit-"),TEXT("door-marks-"),
  TEXT("landing_trace"),TEXT("time_disagreement"),TEXT("duplicate_print"),TEXT("carrier_")});
}
TArray<FString> Paginate(const FString& Text,int32 Wide,int32 Lines)
{
 const int32 Across=FMath::Max(8,Wide),Down=FMath::Max(2,Lines);
 TArray<FString> Out,Paras;Text.ParseIntoArray(Paras,TEXT("\n"),false);
 FString Page;int32 Used=0;
 const auto Flush=[&Out,&Page,&Used]{Page.TrimEndInline();if(!Page.IsEmpty())Out.Add(Page);Page.Reset();Used=0;};
 for(const FString& Para:Paras)
 {
  // A blank line is kept inside a page, never at its head.
  if(Para.TrimStartAndEnd().IsEmpty()){if(Used>0&&Used<Down){Page+=TEXT("\n");++Used;}continue;}
  FString Rest=Para;
  while(!Rest.IsEmpty())
  {
   const int32 Need=S47BinderLocal::Cost(Rest,Across);
   if(Used>0&&Used+Need>Down)Flush();
   if(Need>Down)
   {
    // Longer than a page by itself: cut at the last space that fits.
    int32 Cut=FMath::Min(Down*Across,Rest.Len()-1);while(Cut>Across&&Rest[Cut]!=TEXT(' '))--Cut;
    Page=Rest.Left(Cut);Rest=Rest.Mid(Cut).TrimStart();Used=Down;Flush();continue;
   }
   if(Used>0)Page+=TEXT("\n");
   Page+=Rest;Used+=Need;Rest.Reset();
  }
 }
 Flush();
 if(Out.IsEmpty())Out.Add(FString());
 return Out;
}
int32 FModel::Find(const FString& Key) const
{
 return Entries.IndexOfByPredicate([&Key](const FEntry& E){return E.Key==Key;});
}
TArray<FRow> FModel::Rows(int32 Section,const FString& PickId) const
{
 TArray<FRow> Out;
 if(Section<0||Section>=SecCount)return Out;
 const auto Heading=[&Out](const FString& Words){FRow R;R.bHead=true;R.Id=TEXT("head:")+Words;R.Left=Words;Out.Add(R);};
 const auto LineOf=[this](int32 At)
 {
  const FEntry& E=Entries[At];FRow R;R.Entry=At;R.Id=E.Key;R.Left=E.Ref.IsEmpty()?E.Kind:E.Ref;R.Mid=E.What;R.Right=E.Status;R.Tone=E.Tone;R.bNew=E.bNew;return R;
 };
 if(Section==SecIndex)
 {
  if(ToFile>=0)
  {
   FRow R;R.Id=TEXT("tofile");R.Left=TEXT("TO FILE");R.Mid=FString::Printf(TEXT("PACKET %s %s"),*Entries[ToFile].Ref,bAtBinder?TEXT("IN HAND"):TEXT("WAITING"));
   R.Right=TEXT("1");R.Tone=-1;R.bNew=true;R.Goto=SecToday;R.Act=FString(TEXT("TURN TO "))+Dividers[SecToday].Title;Out.Add(R);
  }
  for(int32 S=SecToday;S<SecCount;++S)
  {
   FRow R;R.Id=FString::Printf(TEXT("tab:%d"),S);R.Left=Dividers[S].Title;R.Mid=Dividers[S].Summary;R.Right=FString::FromInt(Dividers[S].Entries.Num());
   R.Goto=S;R.Act=FString(TEXT("TURN TO "))+Dividers[S].Title;R.bNew=Dividers[S].New>0;Out.Add(R);
  }
  if(!Latest.IsEmpty())
  {
   Heading(TEXT("LAST FILED"));
   for(const int32 At:Latest)
   {
    FRow R=LineOf(At);R.Id=TEXT("last:")+Entries[At].Key;R.Mid=Entries[At].What+TEXT(" / ")+Entries[At].Showed;R.Jump=Entries[At].Key;R.Act=TEXT("TURN TO THE PAGE");Out.Add(R);
   }
  }
  if(!Older.IsEmpty())
  {
   Heading(TEXT("ON THE SAME SHELF"));
   FRow R;R.Id=TEXT("older");R.Left=TEXT("OLDER BINDER");R.Mid=Older;R.bOlder=true;R.Act=TEXT("READ IT");Out.Add(R);
  }
  return Out;
 }
 if(Section==SecToday)
 {
  for(int32 Group=0;Group<4;++Group)
  {
   bool bAny=false;
   for(const int32 At:Dividers[SecToday].Entries)
   {
    const FEntry& E=Entries[At];if(E.TodayGroup!=Group)continue;
    if(!bAny){Heading(S47BinderLocal::TodayGroups[Group]);bAny=true;}
    FRow R=LineOf(At);R.Id=TEXT("today:")+E.Key;
    if(E.bToFile){if(At==ToFile&&bAtBinder){R.bFile=true;R.Act=TEXT("FILE THE PACKET");}}
    else{R.Jump=E.Key;R.Act=FString(TEXT("TURN TO "))+Dividers[E.Home].Title;}
    Out.Add(R);
   }
  }
  return Out;
 }
 if(Section==SecUnknowns)
 {
  // EVIDENCE-DOSSIER slice 3: at the binder itself the chosen file shows what can be done with it, as a case shows its pages
  for(const int32 At:Dividers[SecUnknowns].Entries)
  {
   const FEntry& E=Entries[At];Out.Add(LineOf(At));
   if(!bAtBinder||E.Unknown<1||(PickId!=E.Key&&!PickId.StartsWith(E.Key+TEXT(">"))))continue;
   const auto Child=[&Out,&E,At](const FString& Do,const FString& Words)
   {
    FRow R;R.Entry=At;R.Id=E.Key+TEXT(">")+Do;R.Indent=1;R.Mid=Words;R.Do=Do;R.Act=Words;Out.Add(R);
   };
   Child(FString::Printf(TEXT("name:%d"),E.Unknown),S47Dossier::NameRow());
   for(int32 N=0;N<E.Others.Num();++N)Child(FString::Printf(TEXT("merge:%d:%d"),E.Unknown,E.Others[N]),S47Dossier::MergeRow(E.Others[N],E.OthersBeside.IsValidIndex(N)&&E.OthersBeside[N]));
   for(const int32 From:E.TakeOut)Child(FString::Printf(TEXT("out:%d:%d"),E.Unknown,From),S47Dossier::TakeOutRow(From));
  }
  return Out;
 }
 if(Section==SecCases)
 {
  for(const int32 At:Dividers[SecCases].Entries)
  {
   const FEntry& E=Entries[At];Out.Add(LineOf(At));
   // A case shows its pages only while it, or one of them, is the line chosen.
   if(PickId!=E.Key&&!PickId.StartsWith(E.Key+TEXT(">")))continue;
   for(const FString& ChildKey:E.Children)
   {
    const int32 Child=Find(ChildKey);if(Child<0)continue;
    FRow R=LineOf(Child);R.Id=E.Key+TEXT(">")+ChildKey;R.Indent=1;R.Jump=ChildKey;R.Act=FString(TEXT("TURN TO "))+Dividers[Entries[Child].Home].Title;Out.Add(R);
   }
  }
  return Out;
 }
 int32 LastDay=MIN_int32;
 for(const int32 At:Dividers[Section].Entries)
 {
  const FEntry& E=Entries[At];
  if(Section!=SecStation&&Section!=SecUnknowns&&E.Day!=LastDay){Heading(E.DayLine);LastDay=E.Day;}
  FRow R=LineOf(At);if(!E.Link.IsEmpty()){R.Jump=E.Link;R.Act=E.LinkLabel;}
  Out.Add(R);
 }
 if(Section==SecStation&&!Out.IsEmpty()&&!Held.IsEmpty())Heading(Held);
 return Out;
}
FModel Build(const FSource& In)
{
 using namespace S47BinderLocal;
 FModel Out;Out.bAtBinder=In.bAtBinder;Out.Date=DateOf(In,In.Today);Out.Older=In.Older;
 for(int32 S=0;S<SecCount;++S){Out.Dividers[S].Tab=Tabs[S];Out.Dividers[S].Title=Titles[S];Out.Dividers[S].Empty=Empties[S];}
 TMap<int32,const FCaseIn*> ByLine;for(const FCaseIn& K:In.Cases)ByLine.Add(K.Line,&K);
 TMap<int32,TArray<FString>> Kids;
 // A page that belongs to a case that is open or closed in the file: it says so, and the case lists it.
 const auto WithCase=[&ByLine,&Kids](FEntry& E,int32 CaseNo)
 {
  if(!ByLine.Contains(CaseNo))return false;
  E.Link=CaseKey(CaseNo);E.LinkLabel=TEXT("TURN TO ")+ByLine[CaseNo]->Title.ToUpper();Kids.FindOrAdd(CaseNo).Add(E.Key);return true; // Tom 13 Nov: cases by title, never a number
 };
 int32 Seq=0,Filed=0,NotFiled=0,FieldCount=0,BenchCount=0,Unread=0,OpenCases=0,Packets=0;
 // DAILY WORK: every original, by day.
 for(const FJobIn& J:In.Jobs)
 {
  FEntry E;E.Key=JobKey(J.Id);E.Home=SecDaily;E.Day=J.Day;E.Rank=J.Day;E.Seq=Seq++;
  E.Kind=J.bFiled?TEXT("FILED ORIGINAL"):TEXT("ORIGINAL");E.Ref=J.Id;E.DayLine=DateOf(In,J.Day);
  E.When=E.DayLine+(J.Scheduled>=0?FString(TEXT(" / SCHEDULED "))+Clock(J.Scheduled):FString());
  E.What=J.TestName.IsEmpty()?J.Source:J.Source+TEXT(" / CASE TEST: ")+J.TestName.ToUpper();
  E.Showed=J.Showed;
  if(J.bFiled)
  {
   const bool bStamped=J.FiledDay>0&&J.FiledClock>=0;
   // The stamp's date is the calendar's: a shift day runs from 08:00, so a packet filed after midnight carries the next date.
   const FString Dated=bStamped?DateOf(In,J.FiledDay+(FMath::Fmod(J.FiledClock,86400.)<8*3600.?1:0)):FString();
   E.Status=TEXT("FILED");E.Tone=1;++Filed;
   E.StatusLine=bStamped?FString::Printf(TEXT("FILED %s %s"),*Dated,*Clock(J.FiledClock)):FString(TEXT("FILED"));
   if(J.Paid>=0)E.StatusLine+=FString::Printf(TEXT(" / CREDITED $%d"),J.Paid);
   E.Stamp=bStamped?FString(TEXT("FILED "))+Dated:FString(TEXT("FILED"));
   E.FiledAt=bStamped?J.FiledDay*86400.+FMath::Fmod(J.FiledClock,86400.):J.Day*86400.-1.;
   E.TodayGroup=(bStamped?J.FiledDay:J.Day)==In.Today?1:-1;
  }
  else if(J.bPacket)
  {
   E.Status=TEXT("TO FILE");E.Tone=-1;E.bToFile=true;E.bNew=true;E.StatusLine=TEXT("TO FILE / PACKET IN HAND");E.TodayGroup=0;++NotFiled;++Packets;
  }
  else
  {
   E.Status=TEXT("AT OPS");E.StatusLine=TEXT("HELD AT OPS / NOT IN THIS BINDER");E.TodayGroup=J.Day==In.Today?3:-1;++NotFiled;
  }
  FString Body=J.bControl?FString::Printf(TEXT("CONTROL   INDEPENDENT CONTROL %s/8 s"),*S47Script::TapeSeconds(J.ControlAt,1,true)):FString(TEXT("CONTROL   NOT RECORDED"));
  if(J.MarkStart>=0&&J.MarkEnd>J.MarkStart)Body+=FString::Printf(TEXT("\nMARK      %s TO %s s"),*S47Script::TapeSeconds(J.MarkStart,2),*S47Script::TapeSeconds(J.MarkEnd,2));
  Body+=FString(TEXT("\n\n"))+(J.bFiled?FiledWords:J.bPacket?PacketWords:HeldWords);
  if(!J.Original.IsEmpty())Body+=FString(TEXT("\n\nTHE ORIGINAL AS PRINTED\n"))+J.Original;
  E.Pages=Paginate(Body,SheetWide,SheetLines);
  if(J.CaseLine>=0)WithCase(E,J.CaseLine);
  Out.Entries.Add(MoveTemp(E));
 }
 // FIELD + BENCH: what was retained, newest first. A case's own records (a result, a conclusion, a reply) are on the case's sheet.
 TArray<FFindingIn> Kept=In.Findings;
 Kept.Sort([](const FFindingIn& A,const FFindingIn& B){return A.Day!=B.Day?A.Day>B.Day:A.Key<B.Key;});
 for(const FFindingIn& F:Kept)
 {
  if(FS47FreeformCases::IsCaseRecord(F.Key))continue;
  FEntry E;E.Key=RecordKey(F.Key);E.Home=SecField;E.Day=F.Day;E.Rank=F.Day;E.Seq=Seq++;
  const bool bBench=BenchKey(F.Key);(bBench?BenchCount:FieldCount)++;
  E.Kind=bBench?TEXT("BENCH RECORD"):TEXT("FIELD RECORD");E.What=TitleOf(F.Text);E.Showed=E.What;E.DayLine=DateOf(In,F.Day);E.When=FString(TEXT("RETAINED "))+E.DayLine;
  E.Status=TEXT("RETAINED");E.StatusLine=TEXT("RETAINED / READ ONLY");E.TodayGroup=F.Day>0&&F.Day==In.Today?2:-1;
  FString Body=F.Text;if(OpenQuestionKey(F.Key))Body+=FString(TEXT("\n\n"))+QuestionWords;
  E.Pages=Paginate(Body,SheetWide,SheetLines);
  const int32 CaseNo=FS47FreeformCases::LineForFinding(F.Key);
  if(CaseNo>=0&&WithCase(E,CaseNo)&&ByLine[CaseNo]->Leads.Contains(F.Key))E.Status=TEXT("CASE LEAD");
  Out.Entries.Add(MoveTemp(E));
 }
 // CASES: one sheet each, open ones first.
 for(const FCaseIn& K:In.Cases)
 {
  FEntry E;E.Key=CaseKey(K.Line);E.Home=SecCases;E.Day=K.bConcluded&&K.ConcludeDay>0?K.ConcludeDay:K.OpenDay;E.Rank=K.bConcluded?0:1;E.Seq=K.Line;
  E.Kind=TEXT("CASE FILE");E.Ref.Empty();E.What=K.Title;E.Showed=K.Question;E.DayLine=DateOf(In,K.OpenDay);
  E.When=FString::Printf(TEXT("OPENED %s %s"),*E.DayLine,*Clock(K.OpenClock));
  if(K.bConcluded&&K.ConcludeDay>0)E.When+=FString(TEXT(" / CLOSED "))+DateOf(In,K.ConcludeDay);
  if(!K.bConcluded){E.Status=TEXT("OPEN");++OpenCases;}
  else if(!K.bAssessment){E.Status=TEXT("CONCLUDED");E.Tone=1;}
  else{E.Status=K.bReplied?FString::Printf(TEXT("FILED D%02d / REPLY IN"),K.ConcludeDay):FString::Printf(TEXT("FILED D%02d / REPLY 08:00"),K.ConcludeDay);E.Tone=1;}
  E.StatusLine=E.Status;
  E.TodayGroup=K.OpenDay==In.Today||(K.bConcluded&&K.ConcludeDay==In.Today)?2:-1;
  if(const TArray<FString>* Mine=Kids.Find(K.Line))E.Children=*Mine;
  FString Body=K.Kind+TEXT("\n")+(K.bConcluded?K.Next:FString(OpenCaseWords));
  if(!K.Head.IsEmpty())Body+=FString(TEXT("\n\n"))+K.Head;
  Body+=FString(TEXT("\n\n"))+(K.Tests.IsEmpty()?TEXT("NO TEST REGISTERED."):TEXT("TESTS REGISTERED"));
  for(const FString& T:K.Tests)Body+=FString(TEXT("\n"))+T;
  if(!K.Reply.IsEmpty())Body+=FString(TEXT("\n\n"))+K.Reply;
  Body+=E.Children.IsEmpty()?FString(TEXT("\n\nNO PAGE IS KEPT WITH THIS CASE."))
   :FString::Printf(TEXT("\n\n%d %s KEPT WITH THIS CASE, LISTED UNDER IT."),E.Children.Num(),E.Children.Num()==1?TEXT("PAGE IS"):TEXT("PAGES ARE"));
  if(K.Folded>0)Body+=FString::Printf(TEXT("\n%d EARLIER %s COUNTED ON CASE BOARD AND KEPT BEHIND FIELD + BENCH."),K.Folded,K.Folded==1?TEXT("LEAD IS"):TEXT("LEADS ARE"));
  E.Pages=Paginate(Body,SheetWide,SheetLines);
  Out.Entries.Add(MoveTemp(E));
 }
 // STATION FILES: as released, in the station's own order.
 for(const FFileIn& F:In.Files)
 {
  FEntry E;E.Key=FileKey(F.Key);E.Home=SecStation;E.Day=F.Day;E.Rank=0;E.Seq=Seq++;E.Kind=TEXT("STATION FILE");
  const bool bNumbered=F.Title.Len()>3&&FChar::IsDigit(F.Title[0])&&FChar::IsDigit(F.Title[1])&&F.Title[2]==TEXT(' ');
  E.Ref=bNumbered?FString(TEXT("FILE "))+F.Title.Left(2):FString(TEXT("FILE"));E.What=bNumbered?F.Title.Mid(3):F.Title;
  E.DayLine=DateOf(In,F.Day);E.When=FString(TEXT("RELEASED "))+E.DayLine;E.Status=F.bSeen?TEXT("READ"):TEXT("NEW");E.Tone=F.bSeen?0:-1;E.bNew=!F.bSeen;Unread+=F.bSeen?0:1;
  E.StatusLine=FString::Printf(TEXT("RELEASED DAY %02d / READ ONLY"),F.Day);E.TodayGroup=F.Day==In.Today?2:-1;
  E.Pages=Paginate(F.Text,SheetWide,SheetLines);
  Out.Entries.Add(MoveTemp(E));
 }
 // UNKNOWNS (EVIDENCE-DOSSIER-20261113): the operator's own files, in the order of their numbers. Read only: nothing is filed here.
 for(const FUnknownIn& U:In.Unknowns)
 {
  FEntry E;E.Key=UnknownKey(U.Number);E.Home=SecUnknowns;E.Day=U.Latest;E.Rank=0;E.Seq=U.Number;
  // slice 3: by the player's name beside its number, once it has one
  E.Kind=UnknownKind;E.What=U.Name.IsEmpty()?U.Title:S47Dossier::Titled(U.Number,U.Name);E.Showed=E.What;E.DayLine=DateOf(In,U.Opened);
  E.When=FString(TEXT("OPENED "))+E.DayLine+(U.Latest>U.Opened?FString(TEXT(" / LAST ENTRY "))+DateOf(In,U.Latest):FString());
  E.Status=U.Stage;E.StatusLine=U.Stage+(U.bFiled?TEXT(" / FILED UNDER ITS NAME"):TEXT(""))+(In.bAtBinder?TEXT(""):TEXT(" / READ ONLY"));
  E.Tone=U.bAtMarks||U.bFiled?1:0;E.TodayGroup=U.Latest>0&&U.Latest==In.Today?2:-1;
  E.Unknown=U.Number;E.NameNow=U.Name;E.NameSuggest=U.Suggest;E.Others=U.Others;E.OthersBeside=U.OthersBeside;E.TakeOut=U.TakeOut;
  E.Pages=Paginate(U.Body,SheetWide,SheetLines);
  Out.Entries.Add(MoveTemp(E));
 }
 // Behind each divider: higher Rank first (the later day, the open case), then the order it came in.
 const TArray<FEntry>& All=Out.Entries;
 const auto Before=[&All](int32 A,int32 B){return All[A].Rank!=All[B].Rank?All[A].Rank>All[B].Rank:All[A].Seq<All[B].Seq;};
 for(int32 At=0;At<All.Num();++At)
 {
  Out.Dividers[All[At].Home].Entries.Add(At);
  if(All[At].TodayGroup>=0)Out.Dividers[SecToday].Entries.Add(At);
  if(All[At].FiledAt>=0)Out.Latest.Add(At);
 }
 for(int32 S=SecCases;S<SecCount;++S)Out.Dividers[S].Entries.Sort(Before);
 Out.Dividers[SecToday].Entries.Sort([&All,&Before](int32 A,int32 B){return All[A].TodayGroup!=All[B].TodayGroup?All[A].TodayGroup<All[B].TodayGroup:Before(A,B);});
 Out.Latest.Sort([&All](int32 A,int32 B){return All[A].FiledAt!=All[B].FiledAt?All[A].FiledAt>All[B].FiledAt:All[A].Seq>All[B].Seq;});
 if(Out.Latest.Num()>5)Out.Latest.SetNum(5);
 // The packet the next filing puts in: the session's choice, else the first in hand.
 if(Packets>0)
 {
  Out.ToFile=In.ToFileId.IsEmpty()?INDEX_NONE:Out.Find(JobKey(In.ToFileId));
  if(Out.ToFile<0||!All[Out.ToFile].bToFile)Out.ToFile=All.IndexOfByPredicate([](const FEntry& E){return E.bToFile;});
 }
 const int32 TodayCount=Out.Dividers[SecToday].Entries.Num();
 Out.Dividers[SecToday].Summary=(TodayCount==0?FString(TEXT("NOTHING YET TODAY")):FString::Printf(TEXT("%d %s TODAY"),TodayCount,TodayCount==1?TEXT("PAGE"):TEXT("PAGES")))+(Packets>0?TEXT(" / PACKET TO FILE"):TEXT(""));
 Out.Dividers[SecToday].New=Packets;
 Out.Dividers[SecCases].Summary=FString::Printf(TEXT("%d OPEN / %d CLOSED"),OpenCases,In.Cases.Num()-OpenCases);
 Out.Dividers[SecDaily].Summary=FString::Printf(TEXT("%d FILED / %d NOT IN THE BINDER"),Filed,NotFiled);
 Out.Dividers[SecField].Summary=FString::Printf(TEXT("%d FIELD / %d BENCH"),FieldCount,BenchCount);
 Out.Dividers[SecUnknowns].Summary=In.Unknowns.IsEmpty()?FString(TEXT("NO FILE OPENED")):FString::Printf(TEXT("%d %s KEPT APART"),In.Unknowns.Num(),In.Unknowns.Num()==1?TEXT("FILE"):TEXT("FILES"));
 Out.Dividers[SecStation].Summary=FString::Printf(TEXT("%d RELEASED / %d NOT READ"),In.Files.Num(),Unread);
 Out.Dividers[SecStation].New=Unread;
 Out.Held=In.FilesHeld>0?FString::Printf(TEXT("%d MORE WITH THE DUTY OFFICER / RELEASED AT 08:00, NOT EVERY MORNING"),In.FilesHeld):FString(TEXT("ALL FILES RELEASED / READ ONLY"));
 Out.Front=FString(FrontHead)+TEXT("\n\n")+Out.Date+FString::Printf(TEXT(" / DAY %02d"),In.Today)+TEXT("\n\n")+(In.bAtBinder?FString():FString(FrontCopy)+TEXT("\n\n"))+FrontRules;
 return Out;
}
FString Lint()
{
 using namespace S47BinderLocal;
 TArray<FString> Words;
 for(int32 S=0;S<SecCount;++S){Words.Add(Tabs[S]);Words.Add(Titles[S]);Words.Add(Empties[S]);}
 for(const TCHAR* W:TodayGroups)Words.Add(W);
 for(const TCHAR* W:RowWords)Words.Add(W);
 for(const TCHAR* W:{FiledWords,PacketWords,HeldWords,OpenCaseWords,QuestionWords,FrontHead,FrontRules,FrontCopy,UnknownKind})Words.Add(W);
 Words.Add(TEXT("NO FILE OPENED. 1 FILE KEPT APART. 3 FILES KEPT APART. OPENED MON APR 28 / LAST ENTRY TUE APR 29. OUTLINE / READ ONLY. HABITS / FILED UNDER ITS NAME."));
 // What Build prints around the session's own text, with stand-in numbers.
 Words.Add(TEXT("NO PAGE IS KEPT WITH THIS CASE. 2 PAGES ARE KEPT WITH THIS CASE, LISTED UNDER IT. 3 EARLIER LEADS ARE COUNTED ON CASE BOARD AND KEPT BEHIND FIELD + BENCH."));
 Words.Add(TEXT("2 OPEN / 1 CLOSED. 3 FILED / 2 NOT IN THE BINDER. 4 FIELD / 1 BENCH. 9 RELEASED / 2 NOT READ. NOTHING YET TODAY. 3 PAGES TODAY / PACKET TO FILE. CASE LEAD. TURN TO THE REPEATED INTERVAL. PACKET IN HAND."));
 static const TCHAR* const Not[]={TEXT("taken"),TEXT("cable"),TEXT("tomas"),TEXT("silhouette"),TEXT("8.15"),TEXT("1947"),TEXT("47 s"),TEXT("alien"),TEXT("ufo"),TEXT("abduct")};
 FString Bad;
 for(const FString& W:Words)
 {
  if(const FString Why=S47Script::LintText(W);!Why.IsEmpty())Bad+=FString::Printf(TEXT("'%s': %s | "),*W.Left(40),*Why);
  for(const TCHAR* N:Not)if(W.Contains(N))Bad+=FString::Printf(TEXT("'%s': says '%s' | "),*W.Left(40),N);
 }
 return Bad;
}
bool SelfTest(TArray<FString>& Lines)
{
 bool bOk=true;
 const auto Check=[&bOk,&Lines](bool bGood,const TCHAR* Name){bOk&=bGood;Lines.Add(FString(bGood?TEXT("PASS "):TEXT("FAIL "))+TEXT("RECORDS-BINDER: ")+Name);};
 const auto Selectable=[](const TArray<FRow>& Rows){int32 N=0;for(const FRow& R:Rows)N+=R.bHead?0:1;return N;};
 { // nothing retained: every divider says where its pages come from, and the binder opens at its index
  FSource None;None.Today=1;const FModel M0=Build(None);
  bool bWords=true;for(int32 S=SecToday;S<SecCount;++S)bWords&=M0.Dividers[S].Entries.IsEmpty()&&!M0.Dividers[S].Empty.IsEmpty()&&M0.Rows(S,FString()).IsEmpty();
  Check(M0.Entries.IsEmpty()&&bWords&&M0.ToFile<0&&M0.Latest.IsEmpty(),TEXT("an empty binder has no page and every divider has its own words for being empty"));
  const TArray<FRow> Front=M0.Rows(SecIndex,FString());
  Check(M0.Land()==SecIndex&&Front.Num()==SecCount-1&&Selectable(Front)==SecCount-1&&Front[0].Goto==SecToday&&Front.Last().Goto==SecStation&&M0.Front.Contains(TEXT("READING COPY")),
   TEXT("an empty binder opens at the index: one line for each divider, no LAST FILED, and away from the binder it is a reading copy"));
 }
 FSource In;In.Today=2;In.bAtBinder=true;In.FilesHeld=1;In.Older=TEXT("An older binder");
 { // a routine original filed on day 1, an unusual one with its packet in hand on day 2, one still at OPS, a case test's original
  FJobIn A;A.Id=TEXT("FF-D0001-J1");A.Source=TEXT("3C 286");A.Showed=TEXT("ROUTINE RESULT");A.Day=1;A.bFiled=true;A.Paid=20;A.Scheduled=19*3600.;In.Jobs.Add(A);
  FJobIn B;B.Id=TEXT("FF-D0002-J2");B.Source=TEXT("PSR B1133+16");B.Showed=TEXT("SOURCE UNRESOLVED");B.Day=2;B.bPacket=true;B.bAnomaly=true;B.bControl=true;B.ControlAt=8;B.MarkStart=9.3;B.MarkEnd=12.6;B.CaseLine=0;
  B.Original=TEXT("Extra ticks outside the reference period. Original retained.");In.Jobs.Add(B);
  FJobIn C2;C2.Id=TEXT("FF-D0002-J3");C2.Source=TEXT("HI STRIP 12");C2.Showed=TEXT("ROUTINE RESULT");C2.Day=2;In.Jobs.Add(C2);
  FJobIn D;D.Id=TEXT("FF-D0002-CASE1-T1-4");D.Source=TEXT("PSR B1133+16");D.Showed=TEXT("CASE TEST / NOT YET REGISTERED");D.TestName=TEXT("Generator");D.Day=2;D.CaseLine=0;In.Jobs.Add(D);
  In.ToFileId=B.Id;
  In.Findings.Add({TEXT("roof_steps"),TEXT("Steps on the roof over the records room. Nobody was up there."),2});
  In.Findings.Add({TEXT("service-pack"),TEXT("Battery pack built at the bench. Tested on the dummy load."),1});
  In.Findings.Add({TEXT("case1-test1"),TEXT("A case's own record."),2});
  FCaseIn K;K.Line=0;K.Title=TEXT("A case title");K.Kind=TEXT("TRIAL CASE");K.Question=TEXT("A question?");K.Head=TEXT("A sheet head");K.OpenDay=2;K.OpenClock=20*3600.;In.Cases.Add(K);
  FCaseIn V;V.Line=2;V.Title=TEXT("Another title");V.Kind=TEXT("TRIAL CASE");V.OpenDay=1;V.bConcluded=true;V.Next=TEXT("CLOSED");V.Leads.Add(TEXT("roof_steps"));In.Cases.Add(V);
  In.Files.Add({TEXT("station-01"),TEXT("01 STATION HISTORY"),FString::ChrN(1500,TEXT('a')).Replace(TEXT("aaaaaaa"),TEXT("aaaaaa ")),1,true});
  In.Files.Add({TEXT("station-07"),TEXT("07 REPORTS BY YEAR"),TEXT("A short file."),2,false});
 }
 const FModel M=Build(In);
 Check(M.Entries.Num()==4+2+2+2&&M.Find(RecordKey(TEXT("case1-test1")))<0&&M.Dividers[SecDaily].Entries.Num()==4&&M.Dividers[SecField].Entries.Num()==2&&M.Dividers[SecCases].Entries.Num()==2&&M.Dividers[SecStation].Entries.Num()==2,
  TEXT("every page is one of the session's own records behind one divider; a case's own record is on its sheet, not a page of its own"));
 {
  const TArray<int32>& Day=M.Dividers[SecDaily].Entries;const TArray<FRow> Rows=M.Rows(SecDaily,FString());
  Check(M.Entries[Day[0]].Day==2&&M.Entries[Day.Last()].Day==1&&M.Entries[Day[0]].Ref==TEXT("FF-D0002-J2")&&Rows[0].bHead&&Rows[0].Left==TEXT("TUE APR 29")&&Rows.Last().Left==TEXT("FF-D0001-J1")&&Rows.Num()==4+2,
   TEXT("DAILY WORK is by day, the latest day first and each day in the order worked, under its date"));
  const FEntry& Filed=M.Entries[M.Find(JobKey(TEXT("FF-D0001-J1")))];const FEntry& Held=M.Entries[M.Find(JobKey(TEXT("FF-D0002-J3")))];
  Check(Filed.Status==TEXT("FILED")&&Filed.Tone==1&&!Filed.Stamp.IsEmpty()&&Filed.StatusLine.Contains(TEXT("CREDITED $20"))&&Filed.When==TEXT("MON APR 28 / SCHEDULED 19:00")&&Held.Status==TEXT("AT OPS")&&Held.Stamp.IsEmpty()&&Held.TodayGroup==3,
   TEXT("an original carries its date, reference, source, result and status; only one in the binder is stamped FILED"));
 }
 {
  const TArray<FRow> Rows=M.Rows(SecToday,FString());const int32 Packet=M.Find(JobKey(TEXT("FF-D0002-J2")));
  Check(M.ToFile==Packet&&M.Land()==SecToday&&Rows.Num()>2&&Rows[0].bHead&&Rows[0].Left==TEXT("TO FILE")&&Rows[1].Entry==Packet&&Rows[1].bFile&&Rows[1].Act==TEXT("FILE THE PACKET")&&M.Dividers[SecToday].New==1,
   TEXT("with a packet in hand the binder opens at TODAY'S FILING with the packet first, and filing it is the one thing ENTER does there"));
  FSource Away=In;Away.bAtBinder=false;const FModel Copy=Build(Away);const TArray<FRow> Far=Copy.Rows(SecToday,FString());
  Check(Far[1].Entry==Packet&&!Far[1].bFile&&Far[1].Act.IsEmpty()&&Copy.Rows(SecIndex,FString())[0].Mid.Contains(TEXT("WAITING")),TEXT("away from the binder the packet is listed as waiting and cannot be filed"));
  bool bGroups=true;int32 Group=-1;for(const FRow& R:Rows)if(!R.bHead){bGroups&=M.Entries[R.Entry].TodayGroup>=Group;Group=M.Entries[R.Entry].TodayGroup;}
  Check(bGroups&&Selectable(Rows)==M.Dividers[SecToday].Entries.Num()&&Rows.ContainsByPredicate([](const FRow& R){return R.bHead&&R.Left==TEXT("STILL AT OPS");})&&!Rows.ContainsByPredicate([](const FRow& R){return R.Left==TEXT("FF-D0001-J1");}),
   TEXT("TODAY'S FILING lists today only: to file, filed, retained, still at OPS, in that order"));
 }
 { // the packet goes in: its page is stamped, it is first under LAST FILED, and it stays in its own divider
  FSource After=In;After.Jobs[1].bPacket=false;After.Jobs[1].bFiled=true;After.Jobs[1].FiledDay=2;After.Jobs[1].FiledClock=21*3600.+14*60.;After.Jobs[1].Paid=19;After.ToFileId.Empty();
  const FModel F=Build(After);const int32 At=F.Find(JobKey(TEXT("FF-D0002-J2")));const FEntry& E=F.Entries[At];
  Check(F.ToFile<0&&F.Land()==SecIndex&&E.Home==SecDaily&&E.TodayGroup==1&&E.StatusLine==TEXT("FILED TUE APR 29 21:14 / CREDITED $19")&&E.Stamp==TEXT("FILED TUE APR 29")&&F.Latest.Num()==2&&F.Latest[0]==At,
   TEXT("a filed packet is a stamped page behind DAILY WORK with the day and time it went in, under FILED TODAY and first under LAST FILED"));
  const TArray<FRow> Front=F.Rows(SecIndex,FString());
  const FRow* Last=Front.FindByPredicate([](const FRow& R){return R.Id==TEXT("last:job:FF-D0002-J2");});
  Check(Last&&Last->Left==TEXT("FF-D0002-J2")&&Last->Mid.Contains(TEXT("SOURCE UNRESOLVED"))&&Last->Right==TEXT("FILED")&&Last->Jump==E.Key&&Front.Last().bOlder,
   TEXT("the index lists the last pages filed by reference, result and state, and the older binder on the shelf"));
 }
 { // cross-references: a case lists its pages, and each of them turns to the case
  const FEntry& Case=M.Entries[M.Find(CaseKey(0))];const FEntry& Original=M.Entries[M.Find(JobKey(TEXT("FF-D0002-J2")))];const FEntry& Lead=M.Entries[M.Find(RecordKey(TEXT("roof_steps")))];
  Check(Case.Children.Num()==2&&Case.Children.Contains(Original.Key)&&Original.Link==Case.Key&&Original.LinkLabel==TEXT("TURN TO ")+Case.What.ToUpper()&&!Original.LinkLabel.Contains(TEXT("01"))&&Case.Ref.IsEmpty()&&Lead.Link==CaseKey(2)&&Lead.Status==TEXT("CASE LEAD"),
   TEXT("a page that belongs to a case says so, and the case lists it"));
  const TArray<FRow> Shut=M.Rows(SecCases,FString()),Open=M.Rows(SecCases,Case.Key),Inside=M.Rows(SecCases,Case.Key+TEXT(">")+Original.Key);
  Check(Shut.Num()==2&&Shut[0].Entry==M.Find(CaseKey(0))&&Open.Num()==4&&Open[1].Indent==1&&Open[1].Jump==Original.Key&&Open[1].Act==TEXT("TURN TO DAILY WORK ARCHIVE")&&Inside.Num()==4,
   TEXT("CASES lists the open case first; a case shows its pages while it or one of them is chosen, and each turns to its own divider"));
  const TArray<FRow> Field=M.Rows(SecField,FString());
  Check(Field.Num()==4&&Field[1].Left==TEXT("FIELD RECORD")&&Field[1].Jump==CaseKey(2)&&Field[3].Left==TEXT("BENCH RECORD")&&Field[3].Jump.IsEmpty()&&M.Entries[Field[3].Entry].What==TEXT("Battery pack built at the bench"),
   TEXT("FIELD + BENCH is by day, each record named by its own first sentence, its internal key never shown"));
 }
 { // station files: unread ones marked, long ones paged
  const TArray<FRow> Rows=M.Rows(SecStation,FString());const FEntry& Long=M.Entries[Rows[0].Entry];
  Check(Rows.Num()==3&&Rows[0].Left==TEXT("FILE 01")&&Rows[0].Mid==TEXT("STATION HISTORY")&&!Rows[0].bNew&&Rows[1].bNew&&Rows[1].Right==TEXT("NEW")&&Rows[2].bHead&&Rows[2].Left.StartsWith(TEXT("1 MORE"))&&M.Dividers[SecStation].New==1,
   TEXT("STATION FILES keeps the station's order, marks the unread and says how many the duty officer still holds"));
  bool bFits=true;FString Joined;for(const FString& Page:Long.Pages){TArray<FString> Paras;Page.ParseIntoArray(Paras,TEXT("\n"),false);int32 Used=0;for(const FString& P:Paras)Used+=FMath::Max(1,FMath::DivideAndRoundUp(P.Len(),SheetWide));bFits&=Used<=SheetLines&&!Page.IsEmpty();Joined+=Page+TEXT(" ");}
  TArray<FString> Was,Now;In.Files[0].Text.ParseIntoArray(Was,TEXT(" "),true);Joined.ParseIntoArray(Now,TEXT(" "),true);
  Check(Long.Pages.Num()>1&&bFits&&Was==Now&&Paginate(FString(),SheetWide,SheetLines).Num()==1&&Paginate(TEXT("a\n\nb"),SheetWide,SheetLines)[0]==TEXT("a\n\nb"),
   TEXT("a long file is read a page at a time: every page fits the sheet, no word is lost or split, and an empty text is one empty page"));
 }
 { // EVIDENCE-DOSSIER-20261113: the operator's files of the unknown, read only, in the order of their numbers
  FSource With=In;
  With.Unknowns.Add({2,TEXT("UNKNOWN 2"),TEXT("OUTLINE"),TEXT("STAGE     OUTLINE"),2,2,false});
  With.Unknowns.Add({1,TEXT("UNKNOWN 1"),TEXT("MARKS"),TEXT("STAGE     MARKS"),1,2,true});
  const FModel Kept=Build(With);const TArray<FRow> Rows=Kept.Rows(SecUnknowns,FString());
  const bool bRows=Rows.Num()==2&&!Rows[0].bHead&&Rows[0].Mid==TEXT("UNKNOWN 1")&&Rows[0].Right==TEXT("MARKS")&&Rows[1].Mid==TEXT("UNKNOWN 2")&&Rows[0].Jump.IsEmpty()&&!Rows[0].bFile&&Rows[0].Act.IsEmpty();
  const FEntry* First=bRows?&Kept.Entries[Rows[0].Entry]:nullptr;
  Check(Kept.Entries.Num()==M.Entries.Num()+2&&First&&First->Key==UnknownKey(1)&&First->StatusLine==TEXT("MARKS")&&Build([&With]{FSource Away=With;Away.bAtBinder=false;return Away;}()).Entries.Last().StatusLine==TEXT("MARKS / READ ONLY")
   &&First->Stamp.IsEmpty()&&First->When==TEXT("OPENED MON APR 28 / LAST ENTRY TUE APR 29")
   &&First->Pages.Num()==1&&First->Pages[0]==TEXT("STAGE     MARKS")&&Kept.Dividers[SecUnknowns].Summary==TEXT("2 FILES KEPT APART")&&Kept.ToFile==M.ToFile&&Kept.Land()==M.Land()
   &&M.Dividers[SecUnknowns].Entries.IsEmpty()&&M.Dividers[SecUnknowns].Summary==TEXT("NO FILE OPENED")&&Kept.Rows(SecToday,FString()).ContainsByPredicate([](const FRow& R){return R.Id==TEXT("today:unk:1");}),
   TEXT("UNKNOWNS keeps the operator's own files in the order of their numbers, read only, under no date; one that grew today is listed under TODAY'S FILING"));
  // EVIDENCE-DOSSIER slice 3: at the binder the chosen file offers its name and filing as one; a reading copy offers nothing
  FSource Named=With;
  Named.Unknowns[1].Name=TEXT("Bare feet");Named.Unknowns[1].Suggest=TEXT("Big head, bare feet");Named.Unknowns[1].Others={2};Named.Unknowns[1].OthersBeside={true};Named.Unknowns[0].TakeOut={3};
  const FModel At=Build(Named);
  const TArray<FRow> Shut=At.Rows(SecUnknowns,FString()),Open=At.Rows(SecUnknowns,UnknownKey(1)),Inside=At.Rows(SecUnknowns,UnknownKey(1)+TEXT(">name:1")),Second=At.Rows(SecUnknowns,UnknownKey(2));
  FSource Copy=Named;Copy.bAtBinder=false;
  const TArray<FRow> Far=Build(Copy).Rows(SecUnknowns,UnknownKey(1));
  Check(Shut.Num()==2&&Shut[0].Mid==TEXT("UNKNOWN 1 (Bare feet)")&&Open.Num()==4&&Open[1].Do==TEXT("name:1")&&Open[1].Indent==1&&Open[2].Do==TEXT("merge:1:2")&&Open[2].Mid.Contains(TEXT("KEPT BESIDE"))
   &&Open[3].Mid==TEXT("UNKNOWN 2")&&Inside.Num()==4&&Second.Num()==4&&Second[2].Do==TEXT("name:2")&&Second[3].Do==TEXT("out:2:3")&&Far.Num()==2&&Far[0].Do.IsEmpty()&&At.Entries[Open[1].Entry].NameSuggest==TEXT("Big head, bare feet"),
   TEXT("slice 3: at the binder a file chosen shows its name and what it can be filed as one with, and what can come out again; away from it nothing can be done"));
 }
 Check(Lint().IsEmpty(),TEXT("the binder's own words keep to Freeform's spoiler limits"));
 if(const FString Why=Lint();!Why.IsEmpty())Lines.Add(TEXT("     ")+Why);
 return bOk;
}
}
