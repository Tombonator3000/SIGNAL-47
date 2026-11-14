#include "S47Rhythm.h"
#include "S47DirectorCam.h"
#include "S47DevWaves.h"
#include "S47Aftermath.h"
#include "S47CaseFieldTests.h"
#include "S47DoorVisit.h"
#include "S47FarmSite.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47GameStyle.h"
#include "S47Glimpses.h"
#include "S47Happenings.h"
#include "S47WeekCase.h"
#include "S47HighwayUfo.h"
#include "S47IndoorDread.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47NightRhythm.h"
#include "S47Predecessor.h"
#include "S47ScriptPlayer.h"
#include "S47SkyVisitors.h"
#include "S47SmallHints.h"
#include "S47Taken.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47Weather.h"
#include "S47World.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "S47RhythmDefaults.inl"

DEFINE_LOG_CATEGORY_STATIC(LogS47Rhythm,Log,All);

namespace
{
 FString ContentFile(){return FPaths::ProjectContentDir()/TEXT("Director/director.json");}
 FString OverrideFile(){return FPaths::ProjectSavedDir()/TEXT("Director/director.override.json");}
 double HourOf(double Clock){return FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;}
 bool InWindow(double Hour,double From,double To){return From<=To?(Hour>=From&&Hour<To):(Hour>=From||Hour<To);}
 FString ClockText(double Clock){const int32 S=int32(FMath::Fmod(FMath::Max(0.,Clock),86400.));return FString::Printf(TEXT("%02d:%02d"),S/3600,(S/60)%60);}
 bool Range(const S47J::FObj& O,const TCHAR* K,double& A,double& B)
 {
  const auto V=S47J::Nums(O,K);
  if(V.Num()!=2||!FMath::IsFinite(V[0])||!FMath::IsFinite(V[1]))return false;
  A=V[0];B=V[1];return true;
 }
 const TCHAR* const KnownNeeds[]={TEXT("reference_filed"),TEXT("has_finding"),TEXT("no_power_fault"),TEXT("no_array_fault"),TEXT("roof_window_free"),TEXT("room_left_dark"),TEXT("native_body"),TEXT("no_bodies_out"),TEXT("near_feeder"),TEXT("near_service"),TEXT("indoors"),TEXT("outdoors"),TEXT("door_found"),TEXT("peek_place"),TEXT("flood_room"),TEXT("site_waiting"),TEXT("on_foot"),TEXT("not_driving"),TEXT("past_fence"),TEXT("driving")};
}

bool FS47Rhythm::Parse(const FString& Json,FS47RhythmData& Out,FString& Why)
{
 Out=FS47RhythmData();
 const auto Doc=S47J::Read(Json);
 if(!Doc.IsValid()){Why=TEXT("does not parse");return false;}
 if(S47J::Int(Doc,TEXT("version"))!=1){Why=TEXT("version is not 1");return false;}
 const auto G=S47J::Obj(Doc,TEXT("globals"));
 if(!G.IsValid()){Why=TEXT("no globals");return false;}
 const auto H=S47J::Obj(G,TEXT("their_hours"));
 Out.Earliest=S47J::Num(H,TEXT("earliest"),-1);Out.Latest=S47J::Num(H,TEXT("latest"),-1);Out.End=S47J::Num(H,TEXT("end"),-1);Out.DevFixed=S47J::Num(H,TEXT("dev_fixed"),2);
 if(!(Out.Earliest>=0&&Out.Earliest<=Out.Latest&&Out.Latest<=Out.End&&Out.End<=12)){Why=TEXT("their_hours are impossible");return false;}
 // The tiers (TIERS-20261109): optional, the code's own when absent; glimpses before full, both in the small hours or the late evening.
 if(const auto Tiers=S47J::Obj(G,TEXT("tiers")))
 {
  Out.TierGlimpse=S47J::Num(Tiers,TEXT("glimpse_from"),-1);Out.TierFull=S47J::Num(Tiers,TEXT("full_from"),-1);
  Out.TierLateGlimpse=S47J::Num(Tiers,TEXT("late_glimpse_from"),-1);Out.TierLateFull=S47J::Num(Tiers,TEXT("late_full_from"),-1);
  Out.TierLateNight=S47J::Int(Tiers,TEXT("late_from_night"),-1);
  const auto Night=[](double H){return H<0||H>=24?-1.:H<12?H+24:H;};
  if(!(Night(Out.TierGlimpse)>=18&&Night(Out.TierGlimpse)<=Night(Out.TierFull)&&Night(Out.TierLateGlimpse)>=18&&Night(Out.TierLateGlimpse)<=Night(Out.TierLateFull)&&Out.TierLateNight>=1))
  {Why=TEXT("tiers: glimpse_from before full_from, in the night, and late_from_night a night");return false;}
 }
 if(!Range(G,TEXT("tick_seconds"),Out.TickMin,Out.TickMax)||Out.TickMin<=0||Out.TickMin>Out.TickMax){Why=TEXT("tick_seconds");return false;}
 const auto Rest=S47J::Obj(G,TEXT("rest_after"));
 Out.RestDanger=S47J::Num(Rest,TEXT("danger"),-1);Out.RestOther=S47J::Num(Rest,TEXT("other"),-1);Out.RestTaken=S47J::Num(Rest,TEXT("taken"),-1);
 if(Out.RestDanger<0||Out.RestOther<0||Out.RestTaken<0){Why=TEXT("rest_after");return false;}
 const auto B=S47J::Obj(G,TEXT("budget_per_day"));
 double A0,A1;
 if(!Range(B,TEXT("minor"),A0,A1)||A0<0||A0>A1){Why=TEXT("budget minor");return false;}Out.MinorMin=int32(A0);Out.MinorMax=int32(A1);
 if(!Range(B,TEXT("invest"),A0,A1)||A0<0||A0>A1){Why=TEXT("budget invest");return false;}Out.InvestMin=int32(A0);Out.InvestMax=int32(A1);
 if(!Range(B,TEXT("sky"),A0,A1)||A0<0||A0>A1){Why=TEXT("budget sky");return false;}Out.SkyMin=int32(A0);Out.SkyMax=int32(A1);
 // null, or not there at all: no limit.
 Out.DangerPerNight=G->HasTypedField<EJson::Number>(TEXT("danger_per_night"))?S47J::Int(G,TEXT("danger_per_night")):-1;
 // LOOP-1-20261109: the night's pointing-out at the land (the bearing slip). Absent: one.
 Out.PointOutPerNight=G->HasTypedField<EJson::Number>(TEXT("point_out_per_night"))?S47J::Int(G,TEXT("point_out_per_night")):1;
 if(Out.PointOutPerNight<0||Out.PointOutPerNight>3){Why=TEXT("point_out_per_night: 0 to 3");return false;}
 // The moon: optional, 0.3 to 2 at each end.
 if(const auto Moon=S47J::Obj(G,TEXT("moon")))
 {
  Out.MoonFull=float(S47J::Num(Moon,TEXT("full"),1));Out.MoonNew=float(S47J::Num(Moon,TEXT("new"),1));
  if(!(Out.MoonFull>=.3f&&Out.MoonFull<=2.f&&Out.MoonNew>=.3f&&Out.MoonNew<=2.f)){Why=TEXT("moon: full and new 0.3 to 2");return false;}
 }
 // The curve, week by week: optional, but whole when it is there (a week that starts the game, in order, factors in range).
 if(const auto Curve=S47J::Obj(G,TEXT("curve")))
 {
  // One row: its factors in range, and its own danger limit (absent: the file's).
  const auto ReadRow=[&Why](const S47J::FObj& W,FS47RhythmWeek& Wk)
  {
   Wk.Hint=float(S47J::Num(W,TEXT("hint"),1));Wk.Observation=float(S47J::Num(W,TEXT("observation"),1));Wk.Presence=float(S47J::Num(W,TEXT("presence"),1));
   Wk.Danger=float(S47J::Num(W,TEXT("danger"),-1));Wk.Near=S47J::Num(W,TEXT("near"),1);
   Wk.DangerPerNight=W->HasTypedField<EJson::Number>(TEXT("danger_per_night"))?S47J::Int(W,TEXT("danger_per_night")):-1;
   for(const float K:{Wk.Hint,Wk.Observation,Wk.Presence})if(!(K>=0&&K<=3)){Why=TEXT("curve: a level's worth is 0 to 3");return false;}
   if(!(Wk.Danger>=0&&Wk.Danger<=1)||!(Wk.Near>.1&&Wk.Near<=2)){Why=TEXT("curve: danger 0 to 1, near above 0.1");return false;}
   if(W->HasField(TEXT("danger_per_night"))&&Wk.DangerPerNight<0){Why=TEXT("curve: danger_per_night a whole number");return false;}
   return true;
  };
  int32 Last=0;
  for(const auto& V:S47J::Arr(Curve,TEXT("weeks")))
  {
   const S47J::FObj* WPtr=nullptr;if(!V.IsValid()||!V->TryGetObject(WPtr)||!WPtr){Why=TEXT("curve: a week is not an object");return false;}
   const S47J::FObj& W=*WPtr;
   FS47RhythmWeek Wk;Wk.FromDay=S47J::Int(W,TEXT("from_day"),0);
   if(!ReadRow(W,Wk))return false;
   const bool bFirst=Out.Weeks.IsEmpty();
   if((bFirst&&Wk.FromDay!=1)||(!bFirst&&Wk.FromDay<=Last)){Why=TEXT("curve: weeks start on day 1, in order");return false;}
   Last=Wk.FromDay;Out.Weeks.Add(Wk);
  }
  // The last nights before the assessment (M7): a row of their own, read whenever the game knows that day.
  if(const auto Fin=S47J::Obj(Curve,TEXT("finale")))
  {
   FS47RhythmWeek Wk;Wk.bFinale=true;
   if(!ReadRow(Fin,Wk))return false;
   Out.FinaleBefore=S47J::Int(Fin,TEXT("before_assess"),-1);
   if(Out.FinaleBefore<0||Out.FinaleBefore>10){Why=TEXT("curve: finale before_assess 0 to 10");return false;}
   Out.Finale=Wk;Out.bHasFinale=true;
  }
 }
 for(const auto& Value:S47J::Arr(Doc,TEXT("events")))
 {
  const S47J::FObj* Ptr=nullptr;if(!Value.IsValid()||!Value->TryGetObject(Ptr)||!Ptr){Why=TEXT("an event is not an object");return false;}
  const S47J::FObj& O=*Ptr;
  FS47RhythmEvent E;const FString Id=S47J::Str(O,TEXT("id"));E.Id=FName(*Id);
  if(Id.IsEmpty()||Out.Find(E.Id)){Why=FString::Printf(TEXT("event id %s is empty or twice"),*Id);return false;}
  E.System=S47J::Str(O,TEXT("system"));E.Budget=S47J::Str(O,TEXT("budget"));E.bEnforce=S47J::Bool(O,TEXT("enforce"));E.bGround=S47J::Str(O,TEXT("slot"))!=TEXT("background");
  const FString Level=S47J::Str(O,TEXT("level"));
  if(Level==TEXT("hint"))E.Level=ES47RhythmLevel::Hint;else if(Level==TEXT("observation"))E.Level=ES47RhythmLevel::Observation;else if(Level==TEXT("presence"))E.Level=ES47RhythmLevel::Presence;else if(Level==TEXT("danger"))E.Level=ES47RhythmLevel::Danger;
  else{Why=Id+TEXT(": level");return false;}
  E.FromDay=S47J::Int(O,TEXT("from_day"),0);if(E.FromDay<1){Why=Id+TEXT(": from_day");return false;}
  const FString Hours=S47J::Str(O,TEXT("hours"));
  if(Hours==TEXT("any"))E.Hours=ES47RhythmHours::Any;else if(Hours==TEXT("night"))E.Hours=ES47RhythmHours::Night;else if(Hours==TEXT("their_hours"))E.Hours=ES47RhythmHours::Theirs;
  else if(Range(O,TEXT("hours"),E.From,E.To)&&E.From>=0&&E.From<=24&&E.To>=0&&E.To<=24)E.Hours=ES47RhythmHours::Window;
  else{Why=Id+TEXT(": hours");return false;}
  const FString TierName=S47J::Str(O,TEXT("tier"));
  if(TierName.IsEmpty())E.Tier=E.Hours==ES47RhythmHours::Theirs?ES47Allowed::Full:ES47Allowed::Hint;
  else if(!S47NightTiers::FromName(TierName,E.Tier)){Why=Id+TEXT(": tier is hint, glimpse or full");return false;}
  E.bEarly=S47J::Bool(O,TEXT("early"));
  E.Areas=S47J::Strs(O,TEXT("areas"));
  if(const auto Draw=S47J::Obj(O,TEXT("draw")))
  {
   E.PerNight=S47J::Num(Draw,TEXT("per_night"),-1);E.DrawState=S47J::Str(Draw,TEXT("state"));if(E.PerNight>1){Why=Id+TEXT(": per_night");return false;}
   // draw.at, clock hours: after midnight counted on past 24, as the night counts them.
   double At0=0,At1=0;
   if(Draw->HasField(TEXT("at")))
   {
    if(!Range(Draw,TEXT("at"),At0,At1)||At0<0||At0>24||At1<0||At1>24){Why=Id+TEXT(": draw.at");return false;}
    E.DrawFrom=At0<12?At0+24:At0;E.DrawTo=At1<12?At1+24:At1;
    if(E.DrawFrom<18||E.DrawTo>30||E.DrawTo<=E.DrawFrom){Why=Id+TEXT(": draw.at is not in the night, or ends before it starts");return false;}
   }
  }
  // later: the same event more often from a day on, after its own from_day and in order (M7).
  for(const auto& LV:S47J::Arr(O,TEXT("later")))
  {
   const S47J::FObj* LPtr=nullptr;if(!LV.IsValid()||!LV->TryGetObject(LPtr)||!LPtr){Why=Id+TEXT(": later is not a list of objects");return false;}
   const int32 LDay=S47J::Int(*LPtr,TEXT("from_day"),0);const double LPer=S47J::Num(*LPtr,TEXT("per_night"),-1);
   if(LDay<=(E.Later.IsEmpty()?E.FromDay:E.Later.Last().Key)||!(LPer>=0&&LPer<=1)){Why=Id+TEXT(": later, from_day after the last and per_night 0 to 1");return false;}
   E.Later.Add(TPair<int32,double>(LDay,LPer));
  }
  double C0,C1;if(Range(O,TEXT("cooldown_days"),C0,C1)){E.CoolMin=int32(C0);E.CoolMax=int32(C1);}
  E.MinNights=S47J::Int(O,TEXT("min_nights_between"));E.bFixedTable=S47J::Str(O,TEXT("schedule"))==TEXT("fixed_table");
  for(const FString& N:S47J::Strs(O,TEXT("needs")))
  {
   bool bKnown=false;for(const TCHAR* K:KnownNeeds)bKnown|=N==K;
   if(!bKnown){Why=FString::Printf(TEXT("%s: unknown need %s"),*Id,*N);return false;}
   E.Needs.Add(FName(*N));
  }
  Out.Events.Add(MoveTemp(E));
 }
 if(Out.Events.IsEmpty()){Why=TEXT("no events");return false;}
 return true;
}

int32 FS47Rhythm::CoolDays(const FString& Id,int32 Last,int32 Min,int32 Max)
{
 const uint32 V=FCrc::StrCrc32(*Id)^(uint32(Last)*2654435761u);
 Min=FMath::Max(0,Min);Max=FMath::Max(Min,Max);
 return Min+int32(((V>>7)^V)%uint32(Max-Min+1));
}

void FS47Rhythm::Load()
{
 LoadRules();
 BindRules();
}
void FS47Rhythm::BindRules()
{
 // The night's moon, from the real date of the shift day at eleven at night (S47Weather::Moon).
 Rules.LitOf=[this](int32 Day)
 {
  if(!M||!M->Freeform)return .5;
  return S47Weather::Moon(M->Freeform->DateOfDay(Day),23*3600.).Lit;
 };
 // The night's wind, from the day's weather (S47Weather::Day: the paper prints the same day). Not in a developer's fixtures.
 // M5 critic 3: only a night the wind holds into (it blows on through the small hours, S47Weather::Now), so that more comes near
 // while the wind can be heard; a night whose wind has dropped by one is an ordinary night.
 Rules.StormOf=[this](int32 Day)
 {
  if(!M||!M->Freeform||!M->Freeform->RulesLive())return 0.;
  const S47Weather::FDay D=S47Weather::Day(M->Freeform->Seed(),Day);
  return D.NightStorm>.5f?double(D.NightStorm):0.;
 };
 // The assessment while it is ahead (M7: the finale row), from Dr. Ward's own day: a decided one, or none, has no finale.
 Rules.AssessOf=[this]()->int32
 {
  if(!M||!M->Freeform||!M->Freeform->Predecessor||!M->Freeform->RulesLive())return 0;
  const FS47Predecessor& P=*M->Freeform->Predecessor;
  return P.Result()==FS47Predecessor::EOutcome::None?P.AssessmentDay():0;
 };
}
void FS47Rhythm::LoadRules()
{
 bOverride=false;
 FString Why;
 FString Text;
 const bool bAsked=FParse::Param(FCommandLine::Get(),TEXT("S47DirectorOverride"));
 if(bAsked&&FFileHelper::LoadFileToString(Text,*OverrideFile())&&Parse(Text,Rules,Why))
 {
  bOverride=true;SourceName=TEXT("override");
  UE_LOG(LogS47Rhythm,Warning,TEXT("S47: rhythm: OVERRIDE from %s, %d events"),*OverrideFile(),Rules.Events.Num());
  return;
 }
 if(bAsked)UE_LOG(LogS47Rhythm,Warning,TEXT("S47: rhythm: override asked for but not used: %s"),Why.IsEmpty()?TEXT("no file"):*Why);
 if(FFileHelper::LoadFileToString(Text,*ContentFile())&&Parse(Text,Rules,Why))
 {
  SourceName=TEXT("file");
  UE_LOG(LogS47Rhythm,Display,TEXT("S47: rhythm: %d events from %s"),Rules.Events.Num(),*ContentFile());
  return;
 }
 UE_LOG(LogS47Rhythm,Warning,TEXT("S47: rhythm: %s not used (%s): the embedded standard values"),*ContentFile(),Why.IsEmpty()?TEXT("no file"):*Why);
 FString Fallback;Parse(S47RhythmDefaultJson,Rules,Fallback);SourceName=TEXT("embedded");
}

bool FS47Rhythm::LoadOverride(FString& Why)
{
 FString Text;
 if(!FFileHelper::LoadFileToString(Text,*OverrideFile())){Why=FString::Printf(TEXT("no file at %s"),*OverrideFile());return false;}
 FS47RhythmData Got;
 if(!Parse(Text,Got,Why))return false;
 Rules=MoveTemp(Got);bOverride=true;SourceName=TEXT("override");
 // The parse has none of the game's own: without them the finale row, the moon and the wind were gone (critic round 5).
 BindRules();
 UE_LOG(LogS47Rhythm,Warning,TEXT("S47: rhythm: OVERRIDE from %s by hand, %d events"),*OverrideFile(),Rules.Events.Num());
 return true;
}

FS47Verdict FS47Rhythm::May(FName Id,const FS47RhythmNow& Now,TFunction<bool(FName)> Need,bool bLoosened) const
{
 const auto No=[](const FString& Why,const FString& Detail=FString(),double Retry=-1){FS47Verdict V;V.bMay=false;V.Why=FName(*Why);V.Detail=Detail;V.RetryClock=Retry;return V;};
 const auto* E=Rules.Find(Id);
 if(!E||!Now.bFreeform)return No(TEXT("off"));
 if(Now.bTutorial)return No(TEXT("tutorial"));
 if(Now.Day<E->FromDay)return No(TEXT("too_early_day"),FString::Printf(TEXT("day %d"),E->FromDay));
 const double Hour=HourOf(Now.Clock);
 switch(E->Hours)
 {
 case ES47RhythmHours::Night:if(!(Hour>=19||Hour<6))return No(TEXT("outside_hours"),TEXT("19 to 06"));break;
 case ES47RhythmHours::Theirs:if(!Now.bTheirHours)return No(TEXT("outside_hours"),Now.bQuiet?TEXT("a calm night"):TEXT("not yet their hours"));break;
 case ES47RhythmHours::Window:if(!InWindow(Hour,E->From,E->To))return No(TEXT("outside_hours"),FString::Printf(TEXT("%.1f to %.1f"),E->From,E->To));break;
 default:break;
 }
 // The night's tier (TIERS-20261109): a glimpse waits for midnight, a full meeting for two (an hour sooner in the fourth week), unless
 // the file sets it early on purpose. Being taken is the end of a meeting that was let start in its own tier: not held again here.
 {
  const int32 Night=FS47FreeformSession::NightOf(Now.Clock);
  if(Id!=TEXT("taken")&&!S47NightTiers::Allows(E->Tier,Hour,Night,E->bEarly))
   return No(TEXT("tier"),FString::Printf(TEXT("%s from %s"),S47NightTiers::Name(E->Tier),*ClockText(S47NightTiers::FromClockHour(E->Tier,Night)*3600.)));
 }
 const FString IdText=Id.ToString();
 if(const int32* Last=bLoosened?nullptr:Now.LastDays.Find(Id))
 {
  // As the Director has it now: the day-to-day answer from the id, so that the panel tells the truth about its own numbers.
  if(E->CoolMin>=0)
  {const int32 Cool=FS47Rhythm::CoolDays(IdText,*Last,E->CoolMin,E->CoolMax);if(Now.Day-*Last<Cool)return No(TEXT("cooldown"),FString::Printf(TEXT("%d days"),*Last+Cool-Now.Day));}
  if(E->MinNights>0&&Now.Day-*Last<E->MinNights)return No(TEXT("cooldown"),FString::Printf(TEXT("%d nights"),*Last+E->MinNights-Now.Day));
 }
 if(E->bGround&&!Now.Busy.IsNone())return No(FString::Printf(TEXT("slot_taken:%s"),*Now.Busy.ToString()));
 if(Now.RestLeft>0)return No(TEXT("rest"),FString::Printf(TEXT("%.0f s"),Now.RestLeft));
 // A moment after anything else began: not on top of it. Being taken and the dash clock after a passing are part of what began.
 if(Now.SinceStart<Now.StartGap&&Id!=TEXT("taken")&&Id!=TEXT("clock_ahead"))return No(TEXT("spacing"),FString::Printf(TEXT("%.0f s of the clock"),Now.StartGap-Now.SinceStart));
 if(!bLoosened)
 {
  if(E->Budget==TEXT("minor")&&Now.MinorLeft<=0)return No(TEXT("budget"),TEXT("small"));
  if(E->Budget==TEXT("invest")&&Now.InvestLeft<=0)return No(TEXT("budget"),TEXT("investigation"));
  if(E->Budget==TEXT("sky")&&Now.SkyLeft<=0)return No(TEXT("budget"),TEXT("sky"));
 }
 {
  // The week's own limit (the fourth week and the finale may have two), or the file's.
  const int32 Limit=Rules.DangerLimit(Now.Day,Now.AssessDay);
  if(E->Level==ES47RhythmLevel::Danger&&Limit>=0&&Now.DangerTonight>=Limit)return No(TEXT("danger_night"),FString::Printf(TEXT("%d tonight"),Now.DangerTonight));
 }
 if(E->Level==ES47RhythmLevel::Danger&&Now.bHumorTonight)return No(TEXT("humor_night"));
 if(!E->Areas.IsEmpty()&&!E->Areas.Contains(Now.Area))return No(TEXT("not_here"),E->Areas[0]);
 if(Need)for(const FName N:E->Needs)if(!Need(N))return No(FString::Printf(TEXT("needs:%s"),*N.ToString()));
 if(!bLoosened&&!E->DrawState.IsEmpty())if(const int32* Drawn=Now.Draws.Find(E->DrawState))if(*Drawn==0)return No(TEXT("not_drawn_tonight"));
 return FS47Verdict();
}

FS47RhythmNow FS47Rhythm::ReadNow() const
{
 FS47RhythmNow N;
 auto* F=M&&M->IsFreeform()&&M->Freeform&&M->Freeform->Active()?M->Freeform.Get():nullptr;
 if(!F){N.bFreeform=false;return N;}
 N.Clock=F->Clock();N.Day=F->Day();N.bTutorial=F->InTutorial();N.AssessDay=Rules.AssessNow();
 N.SinceStart=N.Clock-LastStartClock;N.StartGap=StartGap;
 N.bQuiet=F->QuietNight(FS47FreeformSession::NightOf(N.Clock));N.bTheirHours=F->TheirHoursNow(N.Clock);
 N.Area=M->Web?M->Web->Area():FString();
 if(F->Director)F->Director->FillRhythm(N);
 if(F->IndoorDread)
 {
  auto& Door=F->IndoorDread->Door();
  if(Door.RollDayOf()==N.Day)N.Draws.Add(TEXT("door"),Door.DrawnTonight()?1:0);
  if(F->IndoorDread->PeekRollDayOf()==N.Day)N.Draws.Add(TEXT("peek"),F->IndoorDread->PeekDrawnTonight()?1:0);
  if(Door.LastDayOf()>0)N.LastDays.Add(TEXT("door_visit"),Door.LastDayOf());
  if(F->IndoorDread->DoorActive())N.Busy=TEXT("door_visit");else if(F->IndoorDread->PeekActive())N.Busy=TEXT("peek");
 }
 if(F->Happenings)F->Happenings->FillRhythm(N);
 // CRIT6-20261109: a director script under way holds the ground: no meeting, no taking and no big night starts on top of it.
 if(FS47ScriptPlayer::Busy(M))N.Busy=TEXT("script");
 // WEEKLY-CASE-20261109: the week's case on the ground holds it (S47WeekCase.h).
 if(FS47WeekCase::Holds(M))N.Busy=TEXT("week_case");
 if(F->Taken&&F->Taken->Busy())N.Busy=TEXT("taken");
 if(F->FarmSite&&F->FarmSite->VisitWantsGround())N.Busy=F->FarmSite->FieldVisit()?TEXT("field_visit"):TEXT("site_visit");
 if(M->HighwayUfo&&M->HighwayUfo->StageName()!=TEXT("idle")&&M->HighwayUfo->StageName()!=TEXT("done"))N.Busy=TEXT("highway_ufo");
 return N;
}

bool FS47Rhythm::Allow(FName Id,FName Within)
{
 // DIRECTOR-MODE-01: no dangerous meeting begins while the developer flies the director's camera (S47DirectorCam.h).
 if(S47DirectorCam::On(M)||FS47DevWaves::On(M))return false;
 auto* F=M&&M->IsFreeform()&&M->Freeform?M->Freeform.Get():nullptr;
 if(!F||!F->RulesLive())return true;
 const auto* E=Rules.Find(Id);
 if(!E)return true;
 // Its own ground does not count against it, and its own needs it has just answered itself.
 FS47RhythmNow Now=ReadNow();
 if(Now.Busy==Id||(!Within.IsNone()&&Now.Busy==Within))Now.Busy=NAME_None;
 const FS47Verdict V=May(Id,Now);
 if(V.bMay){ShadowSaid.Remove(Id);return true;}
 // Once each time the answer changes: this is asked every frame while it waits.
 FName& Said=ShadowSaid.FindOrAdd(Id);
 if(Said!=V.Why)
 {
  Said=V.Why;
  UE_LOG(LogS47Rhythm,Display,TEXT("S47: rhythm %s %s: %s %s, day %d %s"),E->bEnforce?TEXT("stops"):TEXT("would stop"),*Id.ToString(),*V.Why.ToString(),*V.Detail,Now.Day,*ClockText(Now.Clock));
 }
 return !E->bEnforce;
}

bool FS47Rhythm::AllowFloor(FName Id)
{
 if(S47DirectorCam::On(M)||FS47DevWaves::On(M))return false;
 auto* F=M&&M->IsFreeform()&&M->Freeform?M->Freeform.Get():nullptr;
 if(!F||!F->RulesLive())return true;
 const auto* E=Rules.Find(Id);
 if(!E)return true;
 FS47RhythmNow Now=ReadNow();
 if(Now.Busy==Id)Now.Busy=NAME_None;
 const FS47Verdict V=May(Id,Now);
 if(V.bMay)return true;
 // Critic 12 Nov: May says only the first reason, and cooldown and budget come before the ground, the rest, the spacing and the danger
 // limits. Asked loosened, every hard rule is still asked: only a clean answer lets the floor through.
 if(Loosens(V.Why))
 {
  const FS47Verdict Hard=May(Id,Now,nullptr,true);
  if(Hard.bMay)
  {
   UE_LOG(LogS47Rhythm,Display,TEXT("S47: rhythm loosens %s for the night's floor: %s %s, day %d %s"),*Id.ToString(),*V.Why.ToString(),*V.Detail,Now.Day,*ClockText(Now.Clock));
   ShadowSaid.Remove(Id);
   return true;
  }
  // M1-FLOOR-20261113: the third natural tour's night 12 said "cooldown 3 days" for the figure the floor had let the animals go ahead
  // of; by its times what held it was the rest after the yard's presence (over at 03:21). Said once each time the answer changes.
  FName& Said=ShadowSaid.FindOrAdd(Id);
  if(Said!=Hard.Why)
  {
   Said=Hard.Why;
   UE_LOG(LogS47Rhythm,Display,TEXT("S47: rhythm %s %s for the night's floor: %s %s, day %d %s"),E->bEnforce?TEXT("stops"):TEXT("would stop"),*Id.ToString(),*Hard.Why.ToString(),*Hard.Detail,Now.Day,*ClockText(Now.Clock));
  }
  return !E->bEnforce;
 }
 return Allow(Id);
}
void FS47Rhythm::UseEmbedded()
{
 FString Why;
 Rules=FS47RhythmData();
 Parse(S47RhythmDefaultJson,Rules,Why);
 bOverride=false;SourceName=TEXT("embedded");
}

void FS47Rhythm::Began(FName Id)
{
 auto* F=M&&M->Freeform?M->Freeform.Get():nullptr;
 const auto* E=Rules.Find(Id);
 if(!F||!F->Director||!E)return;
 // Being taken at the end of a visit, at a door or under a night event's light (FS47Happenings::HoldsTaking) is the same meeting.
 if(Id==TEXT("taken")&&((F->IndoorDread&&F->IndoorDread->DoorActive())||(F->FarmSite&&F->FarmSite->VisitWantsGround())||F->Director->ActiveEvent()==TEXT("service_presence")||(F->Happenings&&F->Happenings->HoldsTaking())))return;
 // The night's danger slot (M7): the first that comes near while it is open has it, wherever the player is.
 const int32 Night=FS47FreeformSession::NightOf(F->Clock());
 if(BeganSeed!=F->Seed()){BeganNight.Reset();BeganSeed=F->Seed();}
 if(E->bGround&&E->Level>=ES47RhythmLevel::Presence&&DangerSlotOpen())
 {
  // The Director keeps it, and saves it (critic round 5): a load of the same night has it taken or not as it was then.
  F->Director->SlotNight=Night;
  UE_LOG(LogS47Rhythm,Display,TEXT("S47: rhythm danger slot of night %d taken by %s at %s"),Night,*Id.ToString(),*ClockText(F->Clock()));
 }
 BeganNight.Add(Id,Night);
 if(Id==TEXT("peek"))F->Director->PeekNight=Night;
 // The common log of starts: the next, of anything, waits two to six minutes of the clock (May, "spacing").
 LastStartClock=F->Clock();StartGap=120.+240.*FS47NightRhythm::Roll(F->Seed(),int32(LastStartClock/60.),0x53504143u);
 if(E->Level==ES47RhythmLevel::Danger)
 {
  F->Director->NoteDanger();
  UE_LOG(LogS47Rhythm,Display,TEXT("S47: rhythm danger %s: %d tonight"),*Id.ToString(),F->Director->DangerTonight());
 }
}

void FS47Rhythm::Ended(FName Id,bool bMet)
{
 auto* F=M&&M->Freeform?M->Freeform.Get():nullptr;
 const auto* E=Rules.Find(Id);
 if(!F||!F->Director||!E||!E->bEnforce||!F->RulesLive())return;
 F->Director->RestFor(E->Level==ES47RhythmLevel::Danger&&bMet?Rules.RestDanger:Rules.RestOther);
}

float FS47RhythmData::MoonOn(int32 Day) const
{
 if(!LitOf||Day<=0||(MoonFull==1.f&&MoonNew==1.f))return 1.f;
 return FMath::Lerp(MoonNew,MoonFull,float(FMath::Clamp(LitOf(Day),0.,1.)));
}
const FS47RhythmWeek& FS47RhythmData::Row(int32 Day,int32 Assess,int32* SpanFrom,int32* SpanTo) const
{
 static const FS47RhythmWeek Plain;
 if(bHasFinale&&Assess>0&&Day>=Assess-FinaleBefore&&Day<=Assess)
 {
  if(SpanFrom)*SpanFrom=Assess-FinaleBefore;
  if(SpanTo)*SpanTo=Assess;
  return Finale;
 }
 int32 At=INDEX_NONE;
 for(int32 K=0;K<Weeks.Num();++K)if(Day>=Weeks[K].FromDay)At=K;
 if(SpanFrom)*SpanFrom=At==INDEX_NONE?Day:Weeks[At].FromDay;
 // The last row's own week, for its moon: it holds to the end, but a full moon is measured against the nights round it. Critic
 // round 5: past its first week, against the week of seven nights the day is in (a fixed 22 to 28 blew June up by up to 1.67).
 if(SpanTo)*SpanTo=At==INDEX_NONE?Day:Weeks.IsValidIndex(At+1)?Weeks[At+1].FromDay-1:Weeks[At].FromDay+6;
 if(At!=INDEX_NONE&&!Weeks.IsValidIndex(At+1)&&Day>Weeks[At].FromDay+6)
 {
  const int32 Block=Weeks[At].FromDay+7*((Day-Weeks[At].FromDay)/7);
  if(SpanFrom)*SpanFrom=Block;
  if(SpanTo)*SpanTo=Block+6;
 }
 return At==INDEX_NONE?Plain:Weeks[At];
}
int32 FS47RhythmData::DangerLimit(int32 Day,int32 Assess) const
{
 const FS47RhythmWeek& R=Row(Day,Assess);
 return R.DangerPerNight>=0?R.DangerPerNight:DangerPerNight;
}
float FS47RhythmData::MoonIn(int32 Day,int32 Assess) const
{
 if(!LitOf||Day<=0||(MoonFull==1.f&&MoonNew==1.f))return 1.f;
 int32 A=Day,B=Day;Row(Day,Assess,&A,&B);
 A=FMath::Max(1,A);B=FMath::Clamp(B,A,A+13);
 double Sum=0;for(int32 D=A;D<=B;++D)Sum+=MoonOn(D);
 const double Mean=Sum/double(B-A+1);
 return Mean>0?float(double(MoonOn(Day))/Mean):1.f;
}
FS47RhythmWeek FS47RhythmData::Week(int32 Day) const
{
 const int32 Assess=AssessNow();
 FS47RhythmWeek Got=Row(Day,Assess);
 // The moon: a full one brighter and quieter, a new one darker and busier (MOON-20261108), against the other nights of the row (M7),
 // so that the build-up from week to week stands whatever the moon does. How near things come is the week's.
 const float K=MoonIn(Day,Assess);
 if(K!=1.f){Got.Hint*=K;Got.Observation*=K;Got.Presence*=K;Got.Danger=FMath::Clamp(Got.Danger*K,0.f,1.f);}
 // A night the wind holds into: more comes near under the noise of it (WEATHER-20261108). The small things are the moon's alone.
 const float St=StormOn(Day);
 if(St!=1.f){Got.Presence*=St;Got.Danger=FMath::Clamp(Got.Danger*St,0.f,1.f);}
 return Got;
}
float FS47RhythmData::StormOn(int32 Day) const
{
 if(!StormOf||Day<=0||StormNight==1.f)return 1.f;
 return FMath::Lerp(1.f,StormNight,float(FMath::Clamp(StormOf(Day),0.,1.)));
}
float FS47RhythmData::Worth(ES47RhythmLevel Level,int32 Day) const
{
 const FS47RhythmWeek& W=Week(Day);
 switch(Level)
 {
 case ES47RhythmLevel::Hint:return W.Hint;
 case ES47RhythmLevel::Observation:return W.Observation;
 case ES47RhythmLevel::Presence:return W.Presence;
 // A danger outside the slot (the door, the lit room, the yard's presence): the row's danger against the third week's. The slot's
 // own shapes are asked with no day (FS47Happenings::PlanTonight), since the slot already has the week's danger in it.
 default:return FMath::Clamp(W.Danger/DangerPivot,0.f,2.f);
 }
}
int32 FS47Rhythm::DayNow() const
{
 // As FS47RhythmNow has it (ReadNow), and only where the night's own rules run: a developer's fixtures keep their fixed numbers.
 const auto* F=M&&M->IsFreeform()&&M->Freeform&&M->Freeform->Active()?M->Freeform.Get():nullptr;
 return F&&F->RulesLive()?F->Day():0;
}
float FS47Rhythm::PerNight(FName Id,float Otherwise,int32 Day) const
{
 const auto* E=Rules.Find(Id);
 float Base=E&&E->PerNight>=0?float(E->PerNight):Otherwise;
 if(!E||Day<0)return Base;
 if(Day==0)Day=DayNow();
 if(Day<=0)return Base;
 if(Day<E->FromDay)return 0.f;
 for(const TPair<int32,double>& L:E->Later)if(Day>=L.Key)Base=float(L.Value);
 return FMath::Clamp(Base*Rules.Worth(E->Level,Day),0.f,1.f);
}
bool FS47Rhythm::SameGame() const{return M&&M->Freeform&&BeganSeed==M->Freeform->Seed();}
int32 FS47Rhythm::BeganOn(FName Id) const
{
 if(Id==TEXT("peek"))return M&&M->Freeform&&M->Freeform->Director?M->Freeform->Director->PeekNight:0;
 return SameGame()?BeganNight.FindRef(Id):0;
}
double FS47Rhythm::DangerSlotHour(int32 Seed,int32 Night,float SlotChance,double StartHour,bool bCalm)
{
 // Until five, from an hour in their hours: the same dice FS47Happenings has always thrown for it.
 if(bCalm||Night<2||SlotChance<=0||FS47NightRhythm::Roll(Seed,Night,0x534c4f54u)>=SlotChance)return -1;
 const double Start=StartHour+24;
 return Start+FMath::Max(0.,27.8-Start)*FS47NightRhythm::Roll(Seed,Night,0x534c4f55u);
}
double FS47Rhythm::SlotHourTonight() const
{
 const auto* F=M&&M->IsFreeform()&&M->Freeform&&M->Freeform->Active()?M->Freeform.Get():nullptr;
 if(!F||!F->RulesLive()||F->InTutorial())return -1;
 const int32 Night=FS47FreeformSession::NightOf(F->Clock());
 return DangerSlotHour(F->Seed(),Night,Rules.Week(Night).Danger,F->BodyStartHour(Night),F->QuietNight(Night));
}
bool FS47Rhythm::DangerSlotOpen(double Grace) const
{
 const auto* F=M&&M->IsFreeform()&&M->Freeform&&M->Freeform->Active()?M->Freeform.Get():nullptr;
 if(!F||!F->Director||F->Director->DangerTonight()>0||F->Director->SlotNight==FS47FreeformSession::NightOf(F->Clock()))return false;
 // Not while one of the night's own dangers is out there (met or not yet).
 if(F->Happenings)for(const TCHAR* Id:{TEXT("figure_near"),TEXT("hitchhiker"),TEXT("car_light"),TEXT("cb_echo")})if(F->Happenings->Live(Id))return false;
 const double Slot=SlotHourTonight();
 const double H=FS47Happenings::NightHour(F->Clock());
 return Slot>0&&H>=Slot+FMath::Max(0.,Grace)&&H<29.;
}
bool FS47Rhythm::DrawHours(FName Id,double& From,double& To) const
{
 const auto* E=Rules.Find(Id);
 if(!E||E->DrawFrom<0||E->DrawTo<=E->DrawFrom)return false;
 From=E->DrawFrom;To=E->DrawTo;return true;
}

bool FS47Rhythm::NeedHolds(FName Name) const
{
 auto* F=M&&M->Freeform?M->Freeform.Get():nullptr;
 if(!F)return false;
 const FString N=Name.ToString();
 const auto Glimpses=M->Night?M->Night->Glimpses:TSharedPtr<FS47Glimpses>();
 const bool bDriving=M->World&&M->World->bDriving;
 const auto Close=[&](const TCHAR* Point){if(!F->Field||!M->Player)return false;const FVector P=F->Field->InteractionPoint(Point);return FVector::Dist(FVector(P.X,0,P.Z),FVector(M->Player->Pos.X,0,M->Player->Pos.Z))<12;};
 if(N==TEXT("reference_filed"))return F->Jobs().ContainsByPredicate([](const FS47FreeformSession::FJob& J){return J.Source==0&&J.Stage==FS47FreeformSession::EJobStage::Filed;});
 if(N==TEXT("has_finding"))return F->FindingCount()>0;
 if(N==TEXT("no_power_fault"))return !F->PowerFault();
 if(N==TEXT("no_array_fault"))return F->Field&&!F->Field->HasArrayFault();
 if(N==TEXT("roof_window_free"))return !(F->CaseField&&F->CaseField->OwnsRoofWindow());
 if(N==TEXT("room_left_dark"))
 {
  // Read from the Director's save, as with the rest: a switch off, a real departure and a minute gone.
  const auto R=F->Director?S47J::Obj(F->Director->Save(),TEXT("reactiveLight")):S47J::FObj();
  return R.IsValid()&&!S47J::Str(R,TEXT("room")).IsEmpty()&&S47J::Bool(R,TEXT("left"))&&S47J::Num(R,TEXT("age"))>=60;
 }
 if(N==TEXT("native_body"))return Glimpses&&Glimpses->HasNativeBody();
 if(N==TEXT("no_bodies_out"))return Glimpses&&Glimpses->Individuals().IsEmpty();
 if(N==TEXT("near_feeder"))return Close(TEXT("ff:feeder"));
 if(N==TEXT("near_service"))return Close(TEXT("ff:service"));
 if(N==TEXT("indoors"))return F->Taken&&F->Taken->Indoors();
 if(N==TEXT("outdoors"))return !(F->Taken&&F->Taken->Indoors());
 if(N==TEXT("door_found"))return F->IndoorDread&&F->IndoorDread->Door().DoorHere();
 if(N==TEXT("peek_place"))return F->IndoorDread&&F->IndoorDread->PlayerInPeekPlace();
 if(N==TEXT("flood_room"))return F->IndoorDread&&F->IndoorDread->HasFloodRoom();
 if(N==TEXT("site_waiting"))return F->FarmSite&&F->FarmSite->WillStandAt(F->Clock());
 if(N==TEXT("on_foot")||N==TEXT("not_driving"))return !bDriving;
 if(N==TEXT("driving"))return bDriving;
 if(N==TEXT("past_fence"))return M->Player&&M->Web&&FS47FarmSite::OutOnFoot(M->Web->Area(),FVector2D(M->Player->Pos.X,M->Player->Pos.Z).Size(),bDriving,F->Taken&&F->Taken->Indoors());
 return false;
}

void FS47Rhythm::SetWatching(bool bOn)
{
 bWatching=bOn;
 if(bOn){bWatchInit=false;Since=1;}
}

void FS47Rhythm::Observed(FName Id,const FS47RhythmNow& At)
{
 const auto* E=Rules.Find(Id);
 // What the rules would have said just before it: the ground it is itself on does not count against it.
 FS47RhythmNow Asked=At;if(Asked.Busy==Id)Asked.Busy=NAME_None;
 // Taken at the end of a visit, at a door or by the yard's presence is that meeting's end, not a second one (as Began counts it):
 // krit10field night 3 said "danger 2" and "would stop" for one crab that took the player.
 const bool bSameMeeting=Id==TEXT("taken")&&(Asked.Busy==TEXT("field_visit")||Asked.Busy==TEXT("site_visit")||Asked.Busy==TEXT("door_visit")||Asked.Busy==TEXT("service_presence")||Asked.Busy==TEXT("empty_car")||FS47Happenings::IsDanger(Asked.Busy.ToString()));
 if(bSameMeeting)
 {
  UE_LOG(LogS47Rhythm,Display,TEXT("S47: rhythm start taken day %d %s, the end of %s, not another meeting"),At.Day,*ClockText(M&&M->Freeform?M->Freeform->Clock():0),*Asked.Busy.ToString());
  return;
 }
 // A frame's worth of rest is the frame the Director's own rest ran out in, not a start inside it.
 if(Asked.RestLeft<1)Asked.RestLeft=0;
 const FS47Verdict V=May(Id,Asked);
 if(E)
 {
  switch(E->Level)
  {
  case ES47RhythmLevel::Hint:++Tally.Hint;break;
  case ES47RhythmLevel::Observation:++Tally.Observation;break;
  case ES47RhythmLevel::Presence:++Tally.Presence;break;
  case ES47RhythmLevel::Danger:++Tally.Danger;break;
  }
  if(E->Budget==TEXT("minor"))++Tally.Minor;else if(E->Budget==TEXT("invest"))++Tally.Invest;else if(E->Budget==TEXT("sky"))++Tally.Sky;
 }
 Tally.WouldStop+=V.bMay?0:1;
 const double Clock=M&&M->Freeform?M->Freeform->Clock():0;
 if(Tally.FirstClock<0)Tally.FirstClock=Clock;
 Tally.LastClock=Clock;
 Recent.Add(FString::Printf(TEXT("%s  %s%s"),*ClockText(Clock),*Id.ToString(),V.bMay?TEXT(""):*FString::Printf(TEXT("  [%s]"),*V.Why.ToString())));
 if(Recent.Num()>7)Recent.RemoveAt(0);
 UE_LOG(LogS47Rhythm,Display,TEXT("S47: rhythm start %s day %d %s level %d, rule %s %s %s"),*Id.ToString(),At.Day,*ClockText(Clock),E?int32(E->Level):-1,V.bMay?TEXT("would allow"):TEXT("WOULD STOP"),*V.Why.ToString(),*V.Detail);
}

void FS47Rhythm::CloseNight(int32 NewNight)
{
 if(Tally.Night>0)UE_LOG(LogS47Rhythm,Display,TEXT("S47: rhythm night %d: hint %d observation %d presence %d danger %d, small %d investigation %d sky %d, rule would have stopped %d, first %s last %s"),Tally.Night,Tally.Hint,Tally.Observation,Tally.Presence,Tally.Danger,Tally.Minor,Tally.Invest,Tally.Sky,Tally.WouldStop,*ClockText(Tally.FirstClock),*ClockText(Tally.LastClock));
 Tally=FTally();Tally.Night=NewNight;
}

void FS47Rhythm::Observe(double Dt)
{
 if(!bWatching||!M||!M->IsFreeform()||!M->Freeform||!M->Freeform->Active())return;
 auto& F=*M->Freeform;
 const int32 Night=FS47FreeformSession::NightOf(F.Clock());
 if(Night!=Tally.Night)CloseNight(Night);
 // What the rules were looking at a frame ago: at speed a second is minutes of the night, and a stale look blames the wrong thing.
 Since+=Dt;
 const auto Look=[&]{Before=ReadNow();Since=0;};
 if(!bWatchInit){bWatchInit=true;Look();}
 const auto On=[&](FName Id,bool bNow)
 {
  bool& Was=WasOn.FindOrAdd(Id);
  if(bNow&&!Was)Observed(Id,Before);
  Was=bNow;
 };
 const FString Ev=F.Director?F.Director->ActiveEvent():FString();
 for(const FS47RhythmEvent& E:Rules.Events)if(E.System==TEXT("director")&&E.Id!=TEXT("mystery"))On(E.Id,Ev==E.Id.ToString());
 On(TEXT("door_visit"),F.IndoorDread&&F.IndoorDread->DoorActive());
 On(TEXT("peek"),F.IndoorDread&&F.IndoorDread->PeekActive());
 On(TEXT("taken"),F.Taken&&F.Taken->Busy());
 On(TEXT("site_visit"),F.FarmSite&&F.FarmSite->VisitWantsGround()&&!F.FarmSite->FieldVisit());
 On(TEXT("field_visit"),F.FarmSite&&F.FarmSite->VisitWantsGround()&&F.FarmSite->FieldVisit());
 const FString Road=M->HighwayUfo?M->HighwayUfo->StageName():FString();
 On(TEXT("highway_ufo"),Road==TEXT("approach")||Road==TEXT("pursuit")||Road==TEXT("pass")||Road==TEXT("depart"));
 On(TEXT("sky_formation"),M->SkyVisitors&&M->SkyVisitors->FormationData().bActive&&!M->SkyVisitors->FormationData().bDev);
 On(TEXT("sky_high"),M->SkyVisitors&&M->SkyVisitors->HighTrackOn());
 On(TEXT("mystery"),F.Director&&F.Director->MysteryNoticeClue()>=0);
 if(F.Happenings)for(const FString& H:FS47Happenings::Ids())On(FName(*H),F.Happenings->Live(H));
 Look();
}

FString FS47Rhythm::PanelText(bool bFull) const
{
 auto* F=M&&M->IsFreeform()&&M->Freeform&&M->Freeform->Active()?M->Freeform.Get():nullptr;
 if(!F)return TEXT("DIRECTOR  ikke i Freeform");
 const FS47RhythmNow Now=ReadNow();
 const double Hour=HourOf(Now.Clock);
 FString T=FString::Printf(TEXT("DIRECTOR  DAG %d  %s  %s%s%s"),Now.Day,*ClockText(Now.Clock),(Hour>=19||Hour<6)?TEXT("natt"):TEXT("dag"),Now.bQuiet?TEXT("  ROLIG NATT"):TEXT(""),bOverride?TEXT("  [OVERRIDE AKTIV]"):TEXT(""));
 const int32 Limit=Rules.DangerLimit(Now.Day,Now.AssessDay);
 T+=FString::Printf(TEXT("\nIgjen i dag: små %d  undersøkelser %d  himmel %d   Fare i natt %d/%s"),Now.MinorLeft,Now.InvestLeft,Now.SkyLeft,Now.DangerTonight,Limit<0?TEXT("-"):*FString::FromInt(Limit));
 {
  // The build-up this week (curve): what each level of the night events is worth, the night's danger slot, how near.
  const FS47RhythmWeek W=Rules.Week(Now.Day);
  const FString From=W.bFinale?FString::Printf(TEXT("FINALE, vurdering dag %d"),Now.AssessDay):FString::Printf(TEXT("uke fra dag %d"),W.FromDay);
  const double Slot=SlotHourTonight();
  T+=FString::Printf(TEXT("\nKURVE     %s: små x%.2f  sett x%.2f  nær x%.2f  farens plass %.0f %%  avstand x%.2f  måne x%.2f%s"),*From,W.Hint,W.Observation,W.Presence,W.Danger*100.f,W.Near,Rules.MoonIn(Now.Day,Now.AssessDay),
   Slot>0?*FString::Printf(TEXT("  farens time %s%s"),*ClockText(FMath::Fmod(Slot,24.)*3600.),DangerSlotOpen()?TEXT(" (åpen)"):TEXT("")):TEXT(""));
 }
 T+=FString::Printf(TEXT("\nNÅ        %s"),Now.Busy.IsNone()?TEXT("ingen"):*Now.Busy.ToString());
 if(Now.RestLeft>0)T+=FString::Printf(TEXT("\nRO        %.0f s igjen"),Now.RestLeft);
 // What comes next on its own clock.
 FString Next;
 if(F->Director)
 {
  const auto O=F->Director->Save();
  const double In=S47J::Num(O,TEXT("nextAt"))-S47J::Num(O,TEXT("activeTime"));
  Next+=FString::Printf(TEXT("Director ser om %.0f s"),FMath::Max(0.,In));
  const double OrbAt=F->Director->DuskOrbState().NextNightClock();
  if(OrbAt>Now.Clock)Next+=FString::Printf(TEXT("  lyskuler %s"),*ClockText(OrbAt));
 }
 if(M->SkyVisitors&&M->SkyVisitors->FormationData().NextClock>Now.Clock)Next+=FString::Printf(TEXT("  formasjon %s"),*ClockText(M->SkyVisitors->FormationData().NextClock));
 T+=FString::Printf(TEXT("\nNESTE     %s"),*Next);
 TArray<FString> Can,Cannot;
 const auto Need=[this](FName N){return NeedHolds(N);};
 for(const FS47RhythmEvent& E:Rules.Events)
 {
  const FS47Verdict V=May(E.Id,Now,Need);
  if(V.bMay)Can.Add(E.Id.ToString());
  else Cannot.Add(V.Detail.IsEmpty()?FString::Printf(TEXT("%-18s %s"),*E.Id.ToString(),*V.Why.ToString()):FString::Printf(TEXT("%-18s %s (%s)"),*E.Id.ToString(),*V.Why.ToString(),*V.Detail));
 }
 T+=TEXT("\nKAN NÅ    ")+(Can.IsEmpty()?FString(TEXT("ingen")):FString::Join(Can,TEXT("  ")));
 if(bFull)
 {
  T+=TEXT("\nKAN IKKE");
  for(const FString& L:Cannot)T+=TEXT("\n  ")+L;
 }
 else T+=FString::Printf(TEXT("\nKAN IKKE  %d. F10: Panel, full, for hvorfor."),Cannot.Num());
 if(!Recent.IsEmpty())
 {
  T+=FString::Printf(TEXT("\nI NATT    %d hendelser, regelen ville stoppet %d"),Tally.Hint+Tally.Observation+Tally.Presence+Tally.Danger,Tally.WouldStop);
  for(const FString& L:Recent)T+=TEXT("\n  ")+L;
 }
 return T;
}

void FS47Rhythm::LogPanel() const
{
 TArray<FString> Lines;PanelText(true).ParseIntoArrayLines(Lines,false);
 for(const FString& L:Lines)UE_LOG(LogS47Rhythm,Display,TEXT("S47: rhythm panel: %s"),*L);
}

bool FS47Rhythm::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const FString& Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s rhythm: %s"),B?TEXT("PASS"):TEXT("FAIL"),*Why));};
 FS47Rhythm R(nullptr);FString Why;
 Check(Parse(S47RhythmDefaultJson,R.Rules,Why),TEXT("the embedded file parses: ")+Why);
 const auto& D=R.Rules;
 // The numbers in the file are the numbers in the code, until the code reads the file.
 Check(D.Earliest==FS47NightRhythm::EarliestHour&&D.Latest==FS47NightRhythm::LatestHour&&D.End==FS47NightRhythm::EndHour,TEXT("their hours as FS47NightRhythm has them"));
 const auto Chance=[&](const TCHAR* Id){const auto* E=D.Find(Id);return E?E->PerNight:-1.;};
 Check(FMath::IsNearlyEqual(float(Chance(TEXT("door_visit"))),float(FS47DoorVisit::ComeChance),.001f),TEXT("door visit's chance as FS47DoorVisit has it"));
 Check(D.PointOutPerNight==1,TEXT("one pointing-out at the land a night (point_out_per_night, LOOP-1-20261109)"));
 Check(D.TickMin==240&&D.TickMax==420&&D.RestDanger==170&&D.RestOther==45&&D.RestTaken==1800&&D.MinorMin==2&&D.MinorMax==4&&D.InvestMin==0&&D.InvestMax==2&&D.SkyMin==0&&D.SkyMax==1,TEXT("ticks, rest and budgets as the Director has them"));
 // Every event a save can name.
 bool bAll=true;FString Missing;
 for(const FString& Id:FS47FreeformDirector::EventIds())if(!D.Find(FName(*Id))){bAll=false;Missing+=Id+TEXT(" ");}
 Check(bAll,TEXT("every event of the Director's catalogue is in the file ")+Missing);
 FS47RhythmData Bad;
 Check(!Parse(TEXT("{"),Bad,Why)&&!Parse(TEXT("{\"version\":2}"),Bad,Why),TEXT("a broken file or a new version is turned away"));
 Check(!Parse(FString(S47RhythmDefaultJson).Replace(TEXT("\"native_body\""),TEXT("\"no_such_need\"")),Bad,Why),TEXT("an unknown need is turned away"));
 // May's order, on a grid of what it reads.
 FS47RhythmNow N;N.Area=TEXT("saro");N.Day=5;N.Clock=4*86400.+3*3600.;N.bTheirHours=true;N.MinorLeft=2;N.InvestLeft=1;N.SkyLeft=1;
 const auto Says=[&](const TCHAR* Id,const FS47RhythmNow& At){return R.May(Id,At).Why;};
 Check(R.May(TEXT("service_presence"),N).bMay,TEXT("service_presence may at 03:00 on day 5"));
 {FS47RhythmNow A=N;A.bFreeform=false;Check(Says(TEXT("service_presence"),A)==TEXT("off"),TEXT("off outside Freeform"));}
 {FS47RhythmNow A=N;A.bTutorial=true;Check(Says(TEXT("service_presence"),A)==TEXT("tutorial"),TEXT("not in the first shift"));}
 {FS47RhythmNow A=N;A.Day=1;Check(Says(TEXT("service_presence"),A)==TEXT("too_early_day"),TEXT("not before its day"));}
 {
  // GAME_BIBLE: the first day has no dangerous hunt. Every danger in the file waits for day two.
  bool bFree=true;FString Early;for(const FS47RhythmEvent& E:D.Events)if(E.Level==ES47RhythmLevel::Danger&&E.FromDay<2){bFree=false;Early+=E.Id.ToString()+TEXT(" ");}
  Check(bFree,TEXT("no danger on the first night ")+Early);
 }
 {FS47RhythmNow A=N;A.bTheirHours=false;Check(Says(TEXT("service_presence"),A)==TEXT("outside_hours"),TEXT("not outside their hours"));}
 {FS47RhythmNow A=N;A.Clock=4*86400.+12*3600.;Check(Says(TEXT("flyover"),A)==TEXT("outside_hours"),TEXT("a night event not by day"));A.Clock=4*86400.+23*3600.;Check(R.May(TEXT("flyover"),A).bMay,TEXT("and may at 23:00"));}
 {
  // TIERS-20261109: the file's tiers are the code's, every event has one, and May holds each to it.
  Check(D.TierGlimpse==FMath::Fmod(S47NightTiers::GlimpseFrom,24.)&&D.TierFull==S47NightTiers::FullFrom-24.&&D.TierLateGlimpse==S47NightTiers::LateGlimpseFrom&&D.TierLateFull==S47NightTiers::LateFullFrom-24.&&D.TierLateNight==S47NightTiers::LateFromNight,TEXT("the tiers as S47NightTiers has them"));
  bool bTheirsFull=true;FString Off;
  for(const FS47RhythmEvent& E:D.Events)if((E.Hours==ES47RhythmHours::Theirs&&E.Tier!=ES47Allowed::Full)||E.bEarly){bTheirsFull=false;Off+=E.Id.ToString()+TEXT(" ");}
  Check(bTheirsFull&&R.TierOf(TEXT("horizon_figure"))==ES47Allowed::Glimpse&&R.TierOf(TEXT("big_night"))==ES47Allowed::Full&&R.TierOf(TEXT("flyover"))==ES47Allowed::Hint&&R.TierOf(TEXT("no_such_event"))==ES47Allowed::Full,TEXT("what comes in their hours is full, the figure on the horizon a glimpse, lights a hint, and nothing is early yet ")+Off);
  const auto At=[&](int32 Night,double Hour){FS47RhythmNow A=N;A.Day=Night;A.Clock=(Night-1)*86400.+Hour*3600.+(Hour<12?86400.:0.);return A;};
  Check(Says(TEXT("horizon_figure"),At(5,23.5))==TEXT("tier")&&R.May(TEXT("horizon_figure"),At(5,.5)).bMay,TEXT("the figure on the horizon not at 23:30 in the first week, and at 00:30"));
  Check(Says(TEXT("door_visit"),At(5,1.5))==TEXT("tier")&&R.May(TEXT("door_visit"),At(5,2+10/60.)).bMay,TEXT("a visit at the door not at 01:30, and at 02:10"));
  Check(R.May(TEXT("horizon_figure"),At(22,23+10/60.)).bMay&&Says(TEXT("door_visit"),At(22,.9))==TEXT("tier")&&R.May(TEXT("door_visit"),At(22,1+10/60.)).bMay,TEXT("in the fourth week a glimpse at 23:10 and the door at 01:10"));
  FS47Rhythm Ex(nullptr);FString W;const bool bExParsed=Parse(FString(S47RhythmDefaultJson).Replace(TEXT("\"id\": \"door_visit\","),TEXT("\"id\": \"door_visit\", \"early\": true,")),Ex.Rules,W);
  Check(bExParsed&&Ex.Rules.Find(TEXT("door_visit"))&&Ex.Rules.Find(TEXT("door_visit"))->bEarly&&Ex.May(TEXT("door_visit"),At(5,23.5)).bMay&&Says(TEXT("door_visit"),At(5,23.5))==TEXT("tier"),TEXT("an authored early start may come before its tier only when the file sets it"));
  FS47RhythmData Bad2;Check(!Parse(FString(S47RhythmDefaultJson).Replace(TEXT("\"tier\": \"glimpse\""),TEXT("\"tier\": \"near\"")),Bad2,W),TEXT("an unknown tier is turned away"));
 }
 {FS47RhythmNow A=N;A.LastDays.Add(TEXT("service_presence"),5);Check(Says(TEXT("service_presence"),A)==TEXT("cooldown"),TEXT("not on the day it was last"));A.LastDays.Add(TEXT("service_presence"),1);Check(R.May(TEXT("service_presence"),A).bMay,TEXT("and may four days on"));}
 {FS47RhythmNow A=N;A.Busy=TEXT("door_visit");Check(Says(TEXT("service_presence"),A)==TEXT("slot_taken:door_visit"),TEXT("one on the ground at a time"));Check(R.May(TEXT("sky_formation"),A).bMay==(A.Clock>=0&&InWindow(HourOf(A.Clock),21.5,5.5)),TEXT("the sky may run under it"));}
 {FS47RhythmNow A=N;A.RestLeft=100;Check(Says(TEXT("door_visit"),A)==TEXT("rest"),TEXT("no one comes during rest"));}
 {FS47RhythmNow A=N;A.Clock=4*86400.+23*3600.;A.MinorLeft=0;Check(Says(TEXT("flyover"),A)==TEXT("budget"),TEXT("the day's small budget stops a small one"));}
 {FS47RhythmNow A=N;A.Area=TEXT("road");Check(Says(TEXT("service_presence"),A)==TEXT("not_here"),TEXT("service_presence only at SARO"));}
 {FS47RhythmNow A=N;A.Draws.Add(TEXT("pendingThreatNight"),0);Check(Says(TEXT("service_presence"),A)==TEXT("not_drawn_tonight"),TEXT("a draw lost tonight says so"));}
 {
  // Critic 12 Nov (M1-FLOOR): asked loosened, the cooldown is let go and what holds the ground still holds it.
  FS47RhythmNow A=N;A.LastDays.Add(TEXT("service_presence"),5);
  const bool bCoolOnly=Says(TEXT("service_presence"),A)==TEXT("cooldown")&&R.May(TEXT("service_presence"),A,nullptr,true).bMay;
  A.Busy=TEXT("door_visit");A.RestLeft=100;
  const FS47Verdict Held=R.May(TEXT("service_presence"),A,nullptr,true);
  Check(bCoolOnly&&!Held.bMay&&Held.Why==TEXT("slot_taken:door_visit")&&Says(TEXT("service_presence"),A)==TEXT("cooldown"),TEXT("the floor loosens the cooldown only: under it the ground and the rest still hold"));
 }
 Check(R.May(TEXT("service_presence"),N,[](FName N2){return N2!=TEXT("native_body");}).Why==TEXT("needs:native_body"),TEXT("a need says its name"));
 {
  FS47Rhythm L(nullptr);FString W;Parse(S47RhythmDefaultJson,L.Rules,W);L.Rules.DangerPerNight=1;
  FS47RhythmNow A=N;A.DangerTonight=1;
  Check(L.May(TEXT("door_visit"),A).Why==TEXT("danger_night"),TEXT("one danger a night stops the next"));
  Check(L.May(TEXT("peek"),A).bMay,TEXT("looking in at a window does not count as danger"));
  Check(L.May(TEXT("sky_formation"),A).bMay||InWindow(HourOf(A.Clock),21.5,5.5)==false,TEXT("nor do lights"));
 }
 {
  // Tom, 8 November: something humorous never shares its night with a danger.
  FS47RhythmNow A=N;A.bHumorTonight=true;
  Check(Says(TEXT("door_visit"),A)==TEXT("humor_night"),TEXT("after a humorous one tonight no danger comes"));
  Check(R.May(TEXT("peek"),A).bMay,TEXT("and a look in at a window still may"));
 }
 {
  // The curve (Tom's critics, 8 November): week one is small things and things seen, the second comes nearer, the third and fourth
  // have more and worse. In numbers: the night's danger grows, things come nearer, what comes near is worth more each week.
  const FS47RhythmWeek &W1=D.Week(1),&W2=D.Week(9),&W3=D.Week(16),&W4=D.Week(28);
  Check(D.Weeks.Num()>=4&&W1.Danger<=.1f&&W1.Danger<W2.Danger&&W2.Danger<W3.Danger&&W3.Danger<=W4.Danger&&W1.Near>W2.Near&&W2.Near>W3.Near&&W3.Near>=W4.Near&&W1.Presence<W4.Presence,TEXT("the curve: more and nearer week by week, the first week small"));
  FS47Rhythm C(nullptr);FString W;Parse(S47RhythmDefaultJson,C.Rules,W);
  {
   // The moon (MOON-20261108): a new moon makes a night's levels and its danger worth more, a full one less; a half one about as the week.
   // M7: against the other nights of its week, so that the week as a whole is the week's whatever the moon does.
   FS47RhythmData Mn=D;
   Mn.LitOf=[](int32 Day){return Day==10?0.:Day==11?1.:.5;};
   const FS47RhythmWeek NewNight=Mn.Week(10),FullNight=Mn.Week(11),Plain=D.Week(12);
   double Sum=0;for(int32 Dd=8;Dd<=14;++Dd)Sum+=Mn.Week(Dd).Presence;
   Check(D.MoonNew>1.f&&D.MoonFull<1.f&&NewNight.Presence>FullNight.Presence&&NewNight.Danger>FullNight.Danger&&FMath::IsNearlyEqual(float(Sum/7.),Plain.Presence,1e-3f)&&NewNight.Near==FullNight.Near,
    TEXT("the moon: a new one darker and busier, a full one brighter and quieter, the week as a whole and how near things come the week's"));
  }
  {
   // M7, the rhythm review: every new game starts on 28 April 1986, the new moon on day 11 and the full one on day 26, and the moon
   // on the whole week made the fourth quieter than the second. With the real moon of those dates and the assessment on day 30:
   // the weeks' means rise, and the last nights before it are above the third week.
   FS47RhythmData Real=D;
   Real.LitOf=[](int32 Day){return S47Weather::Moon(S47Game::GameDate(true,Day,0,false),23*3600.).Lit;};
   Real.AssessOf=[]{return int32(FS47Predecessor::FirstAssess);};
   const auto Mean=[&Real](int32 A,int32 B,int32 Which)
   {
    double S=0;
    for(int32 Dd=A;Dd<=B;++Dd){const FS47RhythmWeek Wk=Real.Week(Dd);S+=Which==0?Wk.Observation:Which==1?Wk.Presence:Wk.Danger;}
    return S/double(B-A+1);
   };
   bool bRise=true;FString Said;
   for(int32 Which=0;Which<3;++Which)
   {
    const double M2=Mean(8,14,Which),M3=Mean(15,21,Which),M4=Mean(22,26,Which),Fin=Mean(27,30,Which);
    bRise&=M2<M3&&M3<M4&&M3<Fin;
    Said+=FString::Printf(TEXT(" %s %.2f %.2f %.2f %.2f;"),Which==0?TEXT("seen"):Which==1?TEXT("near"):TEXT("danger"),M2,M3,M4,Fin);
   }
   Check(bRise&&Real.Week(28).bFinale&&!Real.Week(26).bFinale,TEXT("with the real moon of May 1986 the weeks build, and the last nights are the fullest (weeks 2, 3, 4, finale):")+Said);
   // Critic round 5: past the last row's first week (KEPT OPEN, or a reprieve to day 44) the moon is measured against the week of
   // nights round the day, not against 22 to 28 around the full moon of 23 May: June's nights no longer come up to 1.67 times.
   FS47RhythmData Late=Real;Late.AssessOf=[]{return 0;};
   double Sum=0,Most=0;for(int32 Dd=29;Dd<=49;++Dd){const double K=Late.MoonIn(Dd,0);Sum+=K;Most=FMath::Max(Most,K);}
   Check(FMath::Abs(Sum/21.-1.)<=.01&&Most<=1.45&&Late.MoonIn(41,0)<1.3f,FString::Printf(TEXT("after day 28 the moon moves the nights round their own week: mean %.3f, most %.2f, day 41 %.2f"),Sum/21.,Most,Late.MoonIn(41,0)));
  }
  {
   // Critic round 5: a reload by hand (F10's override) keeps the game's own moon, wind and assessment on the rules.
   FS47Rhythm Bound(nullptr);FString Bw;Parse(S47RhythmDefaultJson,Bound.Rules,Bw);
   const bool bBare=!Bound.Rules.AssessOf&&!Bound.Rules.LitOf&&!Bound.Rules.StormOf;Bound.BindRules();
   Check(bBare&&Bound.Rules.AssessOf&&Bound.Rules.LitOf&&Bound.Rules.StormOf&&Bound.Rules.AssessNow()==0,TEXT("the rules read again by hand are bound to the game's moon, wind and assessment again"));
  }
  {
   // The finale's row from three days before the assessment until it, its own danger limit, and none once it has been decided.
   FS47RhythmData Fn=D;Fn.AssessOf=[]{return 30;};
   FS47RhythmData None=D;None.AssessOf=[]{return 0;};
   Check(D.bHasFinale&&Fn.Week(27).bFinale&&Fn.Week(30).bFinale&&!Fn.Week(26).bFinale&&!Fn.Week(31).bFinale&&!None.Week(28).bFinale&&Fn.Week(28).Presence>D.Week(28).Presence&&Fn.Week(28).Danger>=D.Week(28).Danger&&Fn.Week(28).Near<=D.Week(28).Near,
    TEXT("the finale: from three days before the assessment until it, nearer and fuller than the fourth week, and gone once it is decided"));
   Check(D.DangerLimit(9,0)==1&&D.DangerLimit(25,0)>=2&&D.DangerLimit(28,30)>=2,FString::Printf(TEXT("dangers a night: %d in the second week, %d in the fourth, %d in the finale"),D.DangerLimit(9,0),D.DangerLimit(25,0),D.DangerLimit(28,30)));
   FS47RhythmNow A=N;A.DangerTonight=1;A.Day=25;
   const bool bFourth=R.May(TEXT("door_visit"),A).bMay;A.Day=9;
   Check(bFourth&&Says(TEXT("door_visit"),A)==TEXT("danger_night"),TEXT("a second danger may come in the fourth week, not in the second"));
  }
  {
   // The night's wind (WEATHER-20261108): a night the storm holds into is a little busier, a still one as the week has it.
   FS47RhythmData Wx=D;
   Wx.StormOf=[](int32 Day){return Day==12?1.:0.;};
   const FS47RhythmWeek Stormy=Wx.Week(12),Still=Wx.Week(13),Plain12=D.Week(12),Plain13=D.Week(13);
   Check(Stormy.Presence>Plain12.Presence&&Stormy.Danger>=Plain12.Danger&&Stormy.Presence<=Plain12.Presence*1.2f&&Stormy.Hint==Plain12.Hint&&Still.Presence==Plain13.Presence&&Stormy.Near==Plain12.Near,
    TEXT("a stormy night: more comes near and the danger a little likelier, the small things and how near the week's"));
  }
  // M7: every system follows the curve, not only the night's: the Director's, the door, the window and the lit room.
  Check(C.PerNight(TEXT("cattle_ring"),0.f,2)<C.PerNight(TEXT("cattle_ring"),0.f,25)&&C.PerNight(TEXT("service_presence"),.35f,9)<C.PerNight(TEXT("service_presence"),.35f,25)&&C.PerNight(TEXT("observer"),.5f,2)<C.PerNight(TEXT("observer"),.5f,25)
   &&C.PerNight(TEXT("farm_lights"),.45f,2)<C.PerNight(TEXT("farm_lights"),.45f,25)&&C.PerNight(TEXT("door_visit"),float(FS47DoorVisit::ComeChance),9)<C.PerNight(TEXT("door_visit"),float(FS47DoorVisit::ComeChance),25)&&C.PerNight(TEXT("peek"),.5f,9)<C.PerNight(TEXT("peek"),.5f,25),
   TEXT("every system's chance follows the curve: the Director's, the door and the window too"));
  Check(C.PerNight(TEXT("peek"),.5f,2)==0.f&&C.PerNight(TEXT("door_visit"),float(FS47DoorVisit::ComeChance),2)==0.f&&C.PerNight(TEXT("lit_room"),.4f,2)==0.f&&C.PerNight(TEXT("service_presence"),.35f,2)==0.f&&C.PerNight(TEXT("peek"),.5f,9)>0.f&&C.PerNight(TEXT("door_visit"),float(FS47DoorVisit::ComeChance),9)>0.f,
   TEXT("the second night is quiet indoors: no face at the window, nobody at the door, no lit room, nothing in the yard while the week's danger is 0"));
  Check(C.PerNight(TEXT("service_presence"),.35f)==.35f&&C.PerNight(TEXT("figure_near"),.6f,-1)==float(C.Rules.Find(TEXT("figure_near"))->PerNight),TEXT("without a game, or for a shape of the danger slot, the file's own number"));
  {
   // later: the same shapes more often from day 15 and 22 (M7: nothing new came after day 8), and turned away out of order.
   const auto* Cp=C.Rules.Find(TEXT("close_pass"));
   Check(Cp&&Cp->Later.Num()>=2&&C.PerNight(TEXT("close_pass"),.1f,25)>2.f*C.PerNight(TEXT("close_pass"),.1f,12)&&C.PerNight(TEXT("close_pass"),.1f,-1)==.1f,TEXT("a later row: close_pass more often from the third week, its own number without a day"));
   FS47RhythmData Bad3;
   Check(!Parse(FString(S47RhythmDefaultJson).Replace(TEXT("{ \"from_day\": 15, \"per_night\": 0.15 }"),TEXT("{ \"from_day\": 1, \"per_night\": 0.15 }")),Bad3,W),TEXT("a later row before the event's own day is turned away"));
  }
  {
   // The night's one danger slot, as FS47Happenings draws it: none on a calm night, the first night or with no danger in the week;
   // always within their hours and before five.
   bool bSlots=true;int32 Had=0;
   for(int32 Seed=1;Seed<=50;++Seed)for(int32 Night=1;Night<=30;++Night)
   {
    const double Start=FS47NightRhythm::StartHour(Seed,Night);
    const double S1=DangerSlotHour(Seed,Night,1.f,Start,false);
    bSlots&=DangerSlotHour(Seed,Night,1.f,Start,true)<0&&DangerSlotHour(Seed,Night,0.f,Start,false)<0&&(Night>=2?(S1>=Start+24&&S1<=FMath::Max(Start+24,27.8)):S1<0);
    Had+=S1>0?1:0;
   }
   Check(bSlots&&Had==50*29,TEXT("the danger slot: in their hours, before five, never the first night, a calm one or a week without danger"));
  }
  double A=0,B=0;
  Check(C.DrawHours(TEXT("car_dies"),A,B)&&A>=19&&B<=30&&A<B&&!C.DrawHours(TEXT("service_presence"),A,B),TEXT("a night event is drawn in its own hours of the night"));
  FS47RhythmData Bad2;
  Check(!Parse(FString(S47RhythmDefaultJson).Replace(TEXT("\"from_day\": 8, \"hint\""),TEXT("\"from_day\": 0, \"hint\"")),Bad2,W),TEXT("a curve out of order is turned away"));
 }
 OK&=S47SmallHints::SelfTest(Lines);
 Check(CoolDays(TEXT("flyover"),3)>=2&&CoolDays(TEXT("flyover"),3)<=4&&CoolDays(TEXT("flyover"),3)==CoolDays(TEXT("flyover"),3),TEXT("cooldown two to four days, the same each time"));
 Check(CoolDays(TEXT("roof_tick"),3,0,0)==0&&CoolDays(TEXT("night_hush"),3,1,1)==1&&CoolDays(TEXT("x"),3,5,2)==5,TEXT("cooldown from the file's own range"));
 {
  // Tom, 7 November, "more must happen": a small deniable thing may come back the next night, so that a player who keeps to one
  // place still has a night around them (krit9: three of six nights at the farmhouse had nothing small at all).
  bool bShort=true;FString Long;
  for(const FString& Id:S47SmallHints::Ids()){const auto* E=D.Find(FName(*Id));if(!E||E->CoolMin<0||E->CoolMax>2){bShort=false;Long+=Id+TEXT(" ");}}
  Check(bShort,TEXT("every small thing comes back within two days ")+Long);
  FS47RhythmNow A=N;A.Clock=4*86400.+23*3600.;A.LastDays.Add(TEXT("roof_tick"),5);
  Check(R.May(TEXT("roof_tick"),A).bMay,TEXT("the roof can tick twice in one night"));
  A.LastDays.Add(TEXT("flyover"),5);Check(Says(TEXT("flyover"),A)==TEXT("cooldown"),TEXT("a light in the sky keeps its days"));
 }
 {
  // Tom, 8 November: someone out on open ground every night meets something about every second or third night, never the first.
  // The game's own field roll (FS47FarmSite::StepFieldDraw) over thirty nights and many games; calm nights draw nothing.
  int32 Visits=0,Nights=0;
  for(int32 Seed=1;Seed<=200;++Seed)
  {
   int32 Misses=0;
   for(int32 Night=2;Night<=31;++Night)
   {
    ++Nights;if(FS47NightRhythm::Quiet(Seed,Night))continue;
    if(FS47NightRhythm::Roll(Seed,Night,0x4649454cu)<FS47FarmSite::FieldChanceFor(Night,Misses,false)){++Visits;Misses=0;}else ++Misses;
   }
  }
  const double Per=Visits>0?double(Nights)/Visits:99.;
  Check(FS47FarmSite::FieldChanceFor(1,9,true)==0.f&&Per>=1.8&&Per<=3.,FString::Printf(TEXT("out in the open a visit every %.2f nights, none the first"),Per));
 }
 return OK;
}
