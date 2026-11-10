// LOOP-3-20261109 (HELHETLIG_LOKKE step 3, Tom 9 November: "Tidsstempel og samme minutt bærer bevisene"). What the player brings back
// from out on the land comes home as a question that only the station can answer: was there something on the array in the same
// minute? The array keeps its own log of what it caught off schedule (ArrayMarks); a time from the field is set against it when the
// player is back at SARO (the pin board's PinWhatCameBack: no new button), and the answer prints on the telex as "SAME MINUTE ON
// ARRAY: YES/NO". It is a field record of OUT ON THE LAND (FS47FreeformCases::LandLine): YES strikes Animals and weather there, by
// the visitors' own way of striking the wind (RuleOutWind), NO strikes nothing. No new trust, no threads: the time carries it.
// A photograph from one of the night's happenings out there is also a field record OPS pays for once, under the week's cap.
#include "S47Freeform.h"
#include "S47Album.h"
#include "S47FieldLogger.h"
#include "S47FreeformCases.h"
#include "S47FreeformWorld.h"
#include "S47Happenings.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Places.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47FreeformMinute, Log, All);

namespace S47FreeformMinuteLocal
{
 // The array's own records, by the start of their keys: what it caught off schedule, each with its "At 02:14" (S47Happenings,
 // S47BigNight, S47Answer). A landing seen from out there was on the array too: it logs whatever comes down, whoever is at SARO.
 const TCHAR* const ArrayKinds[]={TEXT("array_burst_d"),TEXT("landing_logged_d"),TEXT("landing_seen_d"),TEXT("answer_d")};
 FString KindOf(const FString& Id)
 {
  for(const TCHAR* K:ArrayKinds)if(Id.StartsWith(K)){FString Kind=K;Kind.RemoveFromEnd(TEXT("_d"));return Kind;}
  return FString();
 }
 // What is the array's own at SARO and what is out there: a photograph is from a night out there only by the latter (a landing seen
 // is both: on the array, and out there where the player stood).
 bool OutThere(const FString& Id){return FS47Happenings::IsNightFinding(Id)&&!Id.StartsWith(TEXT("array_burst_d"))&&!Id.StartsWith(TEXT("landing_logged_d"))&&!Id.StartsWith(TEXT("answer_d"));}
 bool NightHour(double Clock)
 {
  if(!FMath::IsFinite(Clock)||Clock<0)return false;
  const double H=FMath::Fmod(Clock,86400.)/3600.;
  return H>=19||H<6;
 }
 FString Safe(const FString& Id)
 {
  FString Out;
  for(const TCHAR C:Id)if(FChar::IsAlnum(C)||C==TEXT('-')||C==TEXT('_'))Out.AppendChar(C);
  return Out.Left(60);
 }
 // Day D runs from 08:00 to 08:00 the next morning (FS47FreeformSession::DayNumber): a time of day kept on day D, as a game clock.
 double OnDay(int32 Day,int32 Seconds){return (FMath::Max(1,Day)-(Seconds<8*3600?0:1))*86400.+Seconds;}
 bool TripFrame(const FS47AlbumShot& S){return S.Text.StartsWith(TEXT("Trip camera"));}
}

void FS47FreeformSession::NoteArrayMinute(double From,double To,const FString& Kind)
{
 if(!FMath::IsFinite(From)||!FMath::IsFinite(To)||From<0||To<From||To-From>6*3600.)return;
 // The same catch told twice (a landing's own end, then its record) is one line in the log: the log is of minutes, not of what.
 for(FArrayMark& X:ArrayMarks)if(From<=X.To+120&&To>=X.From-120){X.From=FMath::Min(X.From,From);X.To=FMath::Max(X.To,To);return;}
 FArrayMark A;A.From=From;A.To=To;A.Kind=Kind.Left(32);
 ArrayMarks.Add(A);
 ArrayMarks.Sort([](const FArrayMark& L,const FArrayMark& R){return L.From<R.From;});
 while(ArrayMarks.Num()>MostArrayMarks)ArrayMarks.RemoveAt(0);
 UE_LOG(LogS47FreeformMinute,Display,TEXT("S47: array log: %s from %.0f to %.0f"),*A.Kind,From,To);
 // (no autosave of its own: the record that comes with it, or the event's end, asks for one; a load reading it again asks for none)
}
bool FS47FreeformSession::SameMinute(const TArray<FArrayMark>& Marks,double Clock)
{
 if(!FMath::IsFinite(Clock))return false;
 for(const FArrayMark& X:Marks)if(Clock>=X.From-SameSlack&&Clock<=X.To+SameSlack)return true;
 return false;
}
bool FS47FreeformSession::ClockIn(const FString& Text,int32& OutSeconds)
{
 const auto Digits=[&Text](int32 At){return At+5<=Text.Len()&&FChar::IsDigit(Text[At])&&FChar::IsDigit(Text[At+1])&&Text[At+2]==TEXT(':')&&FChar::IsDigit(Text[At+3])&&FChar::IsDigit(Text[At+4]);};
 for(int32 I=0;I+3<Text.Len();++I)
 {
  const bool bAt=(Text[I]==TEXT('A')||Text[I]==TEXT('a'))&&Text[I+1]==TEXT('t')&&Text[I+2]==TEXT(' ')&&(I==0||!FChar::IsAlpha(Text[I-1]));
  if(!bAt||!Digits(I+3))continue;
  const int32 H=(Text[I+3]-'0')*10+(Text[I+4]-'0'),Mi=(Text[I+6]-'0')*10+(Text[I+7]-'0');
  if(H>23||Mi>59)continue;
  OutSeconds=H*3600+Mi*60;return true;
 }
 return false;
}
void FS47FreeformSession::NoteArrayRecord(const FString& Id,const FString& Text,bool bKeptNow)
{
 using namespace S47FreeformMinuteLocal;
 // When each night record out there was kept: a photograph is from that night if it was taken near it (NightPhotoFrom).
 if(bKeptNow&&OutThere(Id))
 {
  NightClocks.Add(Id,WorldClock);
  while(NightClocks.Num()>MostNightClocks)
  {
   FString Oldest;double At=1e18;
   for(const auto& KV:NightClocks)if(KV.Value<At){At=KV.Value;Oldest=KV.Key;}
   NightClocks.Remove(Oldest);
  }
 }
 const FString Kind=KindOf(Id);int32 Sec=0;
 if(Kind.IsEmpty()||!ClockIn(Text,Sec))return;
 double At=OnDay(FindingDays.Contains(Id)?FindingDays.FindRef(Id):DayNumber,Sec);
 // Kept the morning after (the big night slept through, an answer delivered later): the time was the night before.
 if(bKeptNow)while(At>WorldClock+120&&At>=86400.)At-=86400.;
 NoteArrayMinute(At,At+59,Kind);
}
FString FS47FreeformSession::NightPhotoFrom(double ShotClock) const
{
 if(!S47FreeformMinuteLocal::NightHour(ShotClock))return FString();
 FString Best;double Near=NightPhotoNear+1;
 for(const auto& KV:NightClocks){const double D=FMath::Abs(KV.Value-ShotClock);if(D<=NightPhotoNear&&D<Near){Near=D;Best=KV.Key;}}
 return Best;
}
bool FS47FreeformSession::AskedSameMinute(const FString& RecordId) const
{
 if(Asked.Contains(RecordId))return true;
 const FString Id=S47FreeformMinuteLocal::Safe(RecordId);
 return !Id.IsEmpty()&&(HasFinding(TEXT("minute_yes_")+Id)||HasFinding(TEXT("minute_no_")+Id));
}
// Over MostAsked: the oldest that has its record goes first (still asked by the record); a primed one only when none has.
static void S47TrimAsked(TArray<FString>& Asked,int32 Most,const TFunctionRef<bool(const FString&)> HasRecord)
{
 while(Asked.Num()>Most)
 {
  int32 Drop=0;
  for(int32 I=0;I<Asked.Num();++I)if(HasRecord(Asked[I])){Drop=I;break;}
  Asked.RemoveAt(Drop);
 }
}
bool FS47FreeformSession::AskSameMinute(const FString& RecordId,double Clock,const FString& What)
{
 using namespace S47FreeformMinuteLocal;
 if(RecordId.IsEmpty()||AskedSameMinute(RecordId)||!FMath::IsFinite(Clock)||Clock<0)return false;
 Asked.Add(RecordId);
 const bool bYes=SameMinute(ArrayMarks,Clock);
 const FString Id=Safe(RecordId),At=FS47Places::SlipClock(Clock);
 const FString Key=(bYes?TEXT("minute_yes_"):TEXT("minute_no_"))+Id;
 // The record first: it opens OUT ON THE LAND or is a lead in it (FS47FreeformCases::LineForFinding, OpenCaseFor).
 RecordEvidence(Key,bYes?FString::Printf(TEXT("At %s, by %s, OPS set that minute against the array's own log: same minute on array, yes. The array logged something off schedule in that minute."),*At,*What)
  :FString::Printf(TEXT("At %s, by %s, OPS set that minute against the array's own log: same minute on array, no. Nothing on the log within a minute of it."),*At,*What));
 const auto* Land=Cases.Get(FS47FreeformCases::LandLine);
 const bool bFiled=Land&&Land->bConcluded;
 const bool bWasStruck=Land&&Land->bOpen&&Cases.Excluded(FS47FreeformCases::LandLine,0);
 if(bYes&&!bFiled)Cases.RuleOutWind(FS47FreeformCases::LandLine);
 const bool bStruck=bYes&&!bFiled&&!bWasStruck&&Cases.Excluded(FS47FreeformCases::LandLine,0);
 FString Case;
 if(bFiled)Case=TEXT("OUT ON THE LAND / FILED ALREADY: KEPT AS A RECORD WITH ITS TIME.");
 else if(bStruck)Case=TEXT("OUT ON THE LAND / ANIMALS AND WEATHER DO NOT MOVE THE ARRAY: STRUCK.");
 else if(bYes&&bWasStruck)Case=TEXT("OUT ON THE LAND / KEPT AS A RECORD WITH ITS TIME. ANIMALS AND WEATHER STAND STRUCK.");
 else if(bYes)Case=TEXT("OUT ON THE LAND / KEPT AS A RECORD WITH ITS TIME. THE CONTROLS ALREADY DISAGREE: NOTHING STRUCK.");
 else Case=TEXT("OUT ON THE LAND / KEPT AS A RECORD WITH ITS TIME. NOTHING STRUCK.");
 const FString Page=FString::Printf(TEXT("SARO OPS V4.2  ARRAY LOG QUERY  %s\nRECORD / %s, %s\nASKED / THE ARRAY'S OWN LOG, THE SAME MINUTE\n\nSAME MINUTE ON ARRAY: %s\n%s\n%s"),
  *TerminalDate(WorldClock),*What.ToUpper(),*At,bYes?TEXT("YES"):TEXT("NO"),
  bYes?TEXT("THE ARRAY LOGGED SOMETHING OFF SCHEDULE IN THAT MINUTE. IT SAYS WHEN, NOT WHAT."):*FString::Printf(TEXT("NOTHING ON THE ARRAY'S LOG FROM %s TO %s."),*FS47Places::SlipClock(Clock-60),*FS47Places::SlipClock(Clock+60)),*Case);
 const bool bTelex=QueueTelex(TEXT("telex-minute-")+Id,TEXT("Array log query: ")+What,bYes?TEXT("SAME MINUTE / YES"):TEXT("SAME MINUTE / NO"),Page);
 UE_LOG(LogS47FreeformMinute,Display,TEXT("S47: same minute on array for %s at %s: %s%s%s"),*RecordId,*At,bYes?TEXT("yes"):TEXT("no"),bStruck?TEXT(", animals and weather struck"):TEXT(""),bTelex?TEXT(""):TEXT(" (telex full: the record only)"));
 S47TrimAsked(Asked,MostAsked,[this](const FString& A){const FString S=Safe(A);return HasFinding(TEXT("minute_yes_")+S)||HasFinding(TEXT("minute_no_")+S);});
 Changed();
 return true;
}
int32 FS47FreeformSession::AskWhatCameBack()
{
 using namespace S47FreeformMinuteLocal;
 if(!M||!M->Night||!M->Night->Album)return 0;
 struct FAsk{FString Id,What;double Clock=0;FAsk(const FString& I,const FString& W,double C):Id(I),What(W),Clock(C){}};
 TArray<FAsk> Due;int32 Look=Asked.Num()*7919+NightClocks.Num()*131;
 for(const FS47AlbumShot& S:M->Night->Album->Shots())
 {
  if(S.bUnavailable||S.Url.IsEmpty()||!S.Enlarged.IsEmpty()||!S.IsDeveloped())continue;
  Look+=1+S.LabStage;
  if(AskedSameMinute(S.Id))continue;
  const bool bTrip=TripFrame(S);
  if(!bTrip&&NightPhotoFrom(S.Clock).IsEmpty())continue;
  Due.Emplace(S.Id,bTrip?TEXT("the trip camera's frame"):TEXT("your photograph"),S.Clock);
 }
 if(Field)for(const FS47FieldLogger::FTape& T:Field->FieldLogger().Tapes)
 {
  if(!T.bRetrieved||T.Levels.IsEmpty()||T.Id.IsEmpty())continue;
  Look+=3;
  if(AskedSameMinute(T.Id)||!NightHour(T.Start))continue;
  // Its loudest minute: what the tape says the land had, and when.
  int32 Peak=0;for(int32 I=1;I<T.Levels.Num();++I)if(T.Levels[I]>T.Levels[Peak])Peak=I;
  Due.Emplace(T.Id,TEXT("the field logger's loudest minute"),T.Start+Peak*60.+30.);
 }
 if(Look==AskLook&&PrimeBefore<0)return 0;
 AskLook=Look;
 if(PrimeBefore>=0)
 {
  // An older save: what came back before there was a question is not asked now, all at once; what is newer than the load is.
  for(int32 I=Due.Num()-1;I>=0;--I)if(Due[I].Clock<=PrimeBefore){Asked.Add(Due[I].Id);Due.RemoveAt(I);}
  PrimeBefore=-1;S47TrimAsked(Asked,MostAsked,[this](const FString& A){const FString S=Safe(A);return HasFinding(TEXT("minute_yes_")+S)||HasFinding(TEXT("minute_no_")+S);});
 }
 Due.Sort([](const FAsk& L,const FAsk& R){return L.Clock>R.Clock;});
 int32 N=0;
 for(const FAsk& A:Due){if(N>=MostAskedAtOnce)break;N+=AskSameMinute(A.Id,A.Clock,A.What)?1:0;}
 if(N>0)AskLook=-1; // the rest next time
 return N;
}
int32 FS47FreeformSession::RegisterNightPhoto(const FString& ShotId,double ShotClock,bool bDeveloped,const FString& Title)
{
 if(ShotId.IsEmpty()||!bDeveloped||NightPhotoRegistered(ShotId))return -1;
 const FString From=NightPhotoFrom(ShotClock);const bool bTrip=Title.StartsWith(TEXT("Trip camera"));
 if(From.IsEmpty()&&!bTrip)return -1;
 // Field allowance, under the week's cap (EarnSide): Dr. Ward counts the photograph in the album whether or not it is registered here.
 // CHAIN-2-20261109: brought in the same night as tonight's lead (the card off, the slip out there), the best rate, still capped.
 const int32 ShotNight=NightOf(ShotClock);
 const bool bSameNight=!bTrip&&Happenings&&Happenings->Lead().IsSet()&&Happenings->Lead().bPointed&&Happenings->Lead().Night==ShotNight&&NightOf(WorldClock)==ShotNight;
 const int32 Offered=FieldRecordPay+(bSameNight?SameNightPay:0);
 const int32 Paid=EarnSide(Offered,TEXT("field record"));
 const FString SameLine=bSameNight&&Paid>FieldRecordPay?FString::Printf(TEXT("\n%s: brought in the night the station's own work pointed out there, $%d more."),SameNightMark,Paid-FieldRecordPay):FString();
 RecordEvidence(TEXT("field-registration.")+ShotId,FString::Printf(TEXT("OPS FIELD RECORD RECEIPT\nPhotograph: %s, %s\nFrom: %s\nField record credit: $%d%s.%s\nPaid once per photograph."),
  *ShotId,*FS47Places::SlipClock(ShotClock),bTrip?TEXT("the trip camera, read in the morning"):TEXT("a night out on the land"),Paid,Paid<Offered?TEXT(" (this week's field allowance is used up)"):TEXT(""),
  *SameLine));
 UE_LOG(LogS47FreeformMinute,Display,TEXT("S47: field record %s registered at OPS (%s), $%d"),*ShotId,bTrip?TEXT("trip camera"):*From,Paid);
 return Paid;
}
void FS47FreeformSession::LoadMinutes(const S47J::FObj& Saved)
{
 ArrayMarks.Reset();NightClocks.Reset();Asked.Reset();AskLook=-1;PrimeBefore=-1;
 const S47J::FObj O=S47J::Obj(Saved,TEXT("minutes"));
 if(!O)
 {
  // An older save: the array's log read again from its own records; nothing that came back before is asked all at once.
  if(Saved.IsValid()&&Saved->Values.Num()>0)PrimeBefore=WorldClock+60;
  for(const auto& KV:Findings)if(!S47FreeformMinuteLocal::KindOf(KV.Key).IsEmpty())NoteArrayRecord(KV.Key,KV.Value,false);
  return;
 }
 const TArray<double> L=S47J::Nums(O,TEXT("log"));const TArray<FString> K=S47J::Strs(O,TEXT("kinds"));
 for(int32 I=0;I+1<L.Num()&&I/2<K.Num();I+=2)
 {
  FArrayMark A;A.From=L[I];A.To=L[I+1];A.Kind=K[I/2].Left(32);
  if(FMath::IsFinite(A.From)&&FMath::IsFinite(A.To)&&A.From>=0&&A.To>=A.From&&A.To-A.From<=6*3600.&&ArrayMarks.Num()<MostArrayMarks)ArrayMarks.Add(A);
 }
 if(const S47J::FObj N=S47J::Obj(O,TEXT("night")))for(const auto& KV:N->Values)
 {
  const FString Key(*KV.Key);double C=0;if(KV.Value.IsValid()&&KV.Value->TryGetNumber(C)&&FMath::IsFinite(C)&&C>=0&&!Key.IsEmpty()&&Key.Len()<=160&&NightClocks.Num()<MostNightClocks)NightClocks.Add(Key,C);
 }
 for(const FString& A:S47J::Strs(O,TEXT("asked")))if(!A.IsEmpty()&&A.Len()<=160&&!Asked.Contains(A)&&Asked.Num()<MostAsked)Asked.Add(A);
}
void FS47FreeformSession::SaveMinutes(const S47J::FObj& Out) const
{
 if(!Out)return;
 // Always written once there is anything: a save that has the field is never read again as an older one.
 auto O=S47J::New();
 TArray<double> L;TArray<FString> K;for(const FArrayMark& A:ArrayMarks){L.Add(FMath::RoundToDouble(A.From));L.Add(FMath::RoundToDouble(A.To));K.Add(A.Kind);}
 if(!L.IsEmpty()){S47J::Set(O,TEXT("log"),L);S47J::Set(O,TEXT("kinds"),K);}
 if(!NightClocks.IsEmpty()){auto N=S47J::New();for(const auto& KV:NightClocks)S47J::Set(N,*KV.Key,FMath::RoundToDouble(KV.Value));S47J::Set(O,TEXT("night"),N);}
 if(!Asked.IsEmpty())S47J::Set(O,TEXT("asked"),Asked);
 S47J::Set(Out,TEXT("minutes"),O);
}

bool FS47FreeformSession::MinuteSelfTest(TArray<FString>& Lines)
{
 bool OK=true;auto Check=[&](bool B,const TCHAR* Name){OK&=B;Lines.Add(FString(B?TEXT("PASS "):TEXT("FAIL "))+TEXT("freeform minute: ")+Name);};
 int32 Sec=0;
 Check(ClockIn(TEXT("At 02:14 lights came down."),Sec)&&Sec==2*3600+14*60&&ClockIn(TEXT("An unscheduled printout, TUE APR 29 at 03:08: 31 pulses"),Sec)&&Sec==3*3600+8*60&&!ClockIn(TEXT("Flat 02:14"),Sec)&&!ClockIn(TEXT("nothing"),Sec),
  TEXT("the time in an array record: \"At 02:14\" or \"at 03:08\", nothing else"));
 const double Night4=3*86400.;
 Check(S47FreeformMinuteLocal::OnDay(4,22*3600)==Night4+22*3600.&&S47FreeformMinuteLocal::OnDay(4,2*3600)==4*86400.+2*3600.,TEXT("a time of day kept on day N is that day's evening or the morning after"));
 {
  // A landing out on the land at 22:40 (the array's own record), and a photograph taken there at 22:41: the same minute, YES on the
  // telex, a record of OUT ON THE LAND, and Animals and weather struck. Asked once. A photograph an hour later: NO, nothing struck.
  FS47FreeformSession S(nullptr);S.Begin(nullptr);S.Tutorial=0;
  S.WorldClock=Night4+22*3600.+40*60.;S.DayNumber=4;
  S.RecordEvidence(TEXT("landing_seen_d4"),TEXT("At 22:40 lights came down out on the land, about 110 degrees from SARO, give or take 18, and went out on the ground."));
  const bool bLogged=S.ArrayLog().Num()==1&&S.ArrayLog()[0].Kind==TEXT("landing_seen");
  const double Shot=Night4+22*3600.+41*60.+12;
  const FString From=S.NightPhotoFrom(Shot);
  const bool bAsked=S.AskSameMinute(TEXT("F-0012"),Shot,TEXT("your photograph"))&&!S.AskSameMinute(TEXT("F-0012"),Shot,TEXT("your photograph"));
  const auto* Land=S.CaseFile().Get(FS47FreeformCases::LandLine);
  const bool bTelex=S.TelexQueue().Num()==1&&S.TelexQueue()[0].Page.Contains(TEXT("SAME MINUTE ON ARRAY: YES"))&&S.TelexQueue()[0].Page.Contains(TEXT("STRUCK"))&&!S.TelexQueue()[0].Page.Contains(TEXT("47"));
  Check(bLogged&&From==TEXT("landing_seen_d4")&&bAsked&&S.HasFinding(TEXT("minute_yes_F-0012"))&&Land&&Land->bOpen&&Land->Leads.Contains(TEXT("minute_yes_F-0012"))&&S.CaseFile().Excluded(FS47FreeformCases::LandLine,0)&&bTelex,
   TEXT("a photograph from a night happening is the same minute on the array: YES on the telex, a record of OUT ON THE LAND, Animals and weather struck, asked once"));
  const double Later=Shot+3600;
  const bool bNo=S.AskSameMinute(TEXT("F-0013"),Later,TEXT("your photograph"))&&S.HasFinding(TEXT("minute_no_F-0013"))&&S.TelexQueue().Num()==2&&S.TelexQueue()[1].Page.Contains(TEXT("SAME MINUTE ON ARRAY: NO"))&&S.TelexQueue()[1].Page.Contains(TEXT("NOTHING STRUCK"));
  Check(bNo&&S.NightPhotoFrom(Night4+12*3600.).IsEmpty()&&S.NightPhotoFrom(Shot+2*3600).IsEmpty(),TEXT("an hour later it is NO and strikes nothing; a photograph by day or far from any night record is from no night out there"));
  // A Continue keeps the log, the night's records and what was asked; Animals and weather stay struck (Begin reads it again).
  FS47FreeformSession R(nullptr);R.Begin(S47J::Read(S47J::Write(S.Save())));
  Check(R.ArrayLog().Num()==1&&R.NightPhotoFrom(Shot)==TEXT("landing_seen_d4")&&R.AskedSameMinute(TEXT("F-0012"))&&!R.AskSameMinute(TEXT("F-0012"),Shot,TEXT("x"))&&R.CaseFile().Excluded(FS47FreeformCases::LandLine,0),
   TEXT("the array's log, the night's records and what was asked survive a Continue, and the struck explanation stays struck"));
  // Registration at OPS: once, as field allowance under the week's cap; a photograph from no night out there is refused.
  S.TestLiveRules();S.SideWeek=1;S.SidePaid=FS47FreeformSession::SideCap-5;const int32 Cash0=S.Budget();
  const int32 Paid=S.RegisterNightPhoto(TEXT("F-0012"),Shot,true,TEXT("Kodak"));
  Check(Paid==5&&S.Budget()==Cash0+5&&S.SidePaid==FS47FreeformSession::SideCap&&S.NightPhotoRegistered(TEXT("F-0012"))&&S.RegisterNightPhoto(TEXT("F-0012"),Shot,true,TEXT("Kodak"))==-1
   &&S.RegisterNightPhoto(TEXT("F-0020"),Night4+12*3600.,true,TEXT("Kodak"))==-1&&S.RegisterNightPhoto(TEXT("F-0021"),Shot,false,TEXT("Kodak"))==-1&&S.RegisterNightPhoto(TEXT("F-0022"),Shot+60,true,TEXT("Kodak"))==0,
   TEXT("a night photograph registered at OPS pays once, under the week's $35 cap (then nothing); undeveloped or from no night out there it is refused"));
  S.Stop();R.Stop();
 }
 {
  // An older save (no "minutes"): the array's log read again from its own records; what came back before is not asked all at once.
  FS47FreeformSession S(nullptr);S.Begin(nullptr);S.WorldClock=Night4+23*3600.;S.DayNumber=4;
  S.RecordEvidence(TEXT("array_burst_d4"),TEXT("At 21:05 the array caught a short burst off the main beam, about 120 degrees from SARO, give or take 18. The printer ran a line by itself."));
  auto Old=S.Save();Old->RemoveField(TEXT("minutes"));
  FS47FreeformSession O(nullptr);O.Begin(S47J::Read(S47J::Write(Old)));
  Check(O.ArrayLog().Num()==1&&SameMinute(O.ArrayLog(),Night4+21*3600.+5*60.+20)&&!SameMinute(O.ArrayLog(),Night4+21*3600.+9*60.)&&O.PrimeBefore>=0,TEXT("an older save's array log is read again from the array's own records"));
  S.Stop();O.Stop();
 }
 {
  // Asked is only a cache: with it emptied (trimmed past MostAsked), a record already asked is never asked again; its YES or NO
  // record is the mark. Past MostAsked the trim drops one with its record first, and keeps a primed one (no record).
  FS47FreeformSession S(nullptr);S.Begin(nullptr);S.Tutorial=0;S.WorldClock=Night4+23*3600.;S.DayNumber=4;
  const bool bOnce=S.AskSameMinute(TEXT("F-0030"),Night4+22*3600.,TEXT("your photograph"));
  S.Asked.Reset();
  const bool bNever=S.AskedSameMinute(TEXT("F-0030"))&&!S.AskSameMinute(TEXT("F-0030"),Night4+22*3600.,TEXT("your photograph"));
  TArray<FString> T={TEXT("primed-0"),TEXT("F-0030"),TEXT("primed-1")};
  S47TrimAsked(T,2,[&S](const FString& A){return S.HasFinding(TEXT("minute_no_")+A)||S.HasFinding(TEXT("minute_yes_")+A);});
  Check(bOnce&&bNever&&T.Num()==2&&T[0]==TEXT("primed-0")&&T[1]==TEXT("primed-1"),
   TEXT("asked once for good: the YES or NO record marks it, so trimming what was asked never asks it again"));
  S.Stop();
 }
 return OK;
}
