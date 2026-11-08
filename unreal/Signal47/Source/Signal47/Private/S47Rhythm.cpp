#include "S47Rhythm.h"
#include "S47Aftermath.h"
#include "S47CaseFieldTests.h"
#include "S47DoorVisit.h"
#include "S47FarmSite.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47Glimpses.h"
#include "S47Happenings.h"
#include "S47HighwayUfo.h"
#include "S47IndoorDread.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47NightRhythm.h"
#include "S47SkyVisitors.h"
#include "S47SmallHints.h"
#include "S47Taken.h"
#include "S47Walker.h"
#include "S47Web.h"
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
  E.Areas=S47J::Strs(O,TEXT("areas"));
  if(const auto Draw=S47J::Obj(O,TEXT("draw"))){E.PerNight=S47J::Num(Draw,TEXT("per_night"),-1);E.DrawState=S47J::Str(Draw,TEXT("state"));if(E.PerNight>1){Why=Id+TEXT(": per_night");return false;}}
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
 UE_LOG(LogS47Rhythm,Warning,TEXT("S47: rhythm: OVERRIDE from %s by hand, %d events"),*OverrideFile(),Rules.Events.Num());
 return true;
}

FS47Verdict FS47Rhythm::May(FName Id,const FS47RhythmNow& Now,TFunction<bool(FName)> Need) const
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
 const FString IdText=Id.ToString();
 if(const int32* Last=Now.LastDays.Find(Id))
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
 if(E->Budget==TEXT("minor")&&Now.MinorLeft<=0)return No(TEXT("budget"),TEXT("small"));
 if(E->Budget==TEXT("invest")&&Now.InvestLeft<=0)return No(TEXT("budget"),TEXT("investigation"));
 if(E->Budget==TEXT("sky")&&Now.SkyLeft<=0)return No(TEXT("budget"),TEXT("sky"));
 if(E->Level==ES47RhythmLevel::Danger&&Rules.DangerPerNight>=0&&Now.DangerTonight>=Rules.DangerPerNight)return No(TEXT("danger_night"),FString::Printf(TEXT("%d tonight"),Now.DangerTonight));
 if(E->Level==ES47RhythmLevel::Danger&&Now.bHumorTonight)return No(TEXT("humor_night"));
 if(!E->Areas.IsEmpty()&&!E->Areas.Contains(Now.Area))return No(TEXT("not_here"),E->Areas[0]);
 if(Need)for(const FName N:E->Needs)if(!Need(N))return No(FString::Printf(TEXT("needs:%s"),*N.ToString()));
 if(!E->DrawState.IsEmpty())if(const int32* Drawn=Now.Draws.Find(E->DrawState))if(*Drawn==0)return No(TEXT("not_drawn_tonight"));
 return FS47Verdict();
}

FS47RhythmNow FS47Rhythm::ReadNow() const
{
 FS47RhythmNow N;
 auto* F=M&&M->IsFreeform()&&M->Freeform&&M->Freeform->Active()?M->Freeform.Get():nullptr;
 if(!F){N.bFreeform=false;return N;}
 N.Clock=F->Clock();N.Day=F->Day();N.bTutorial=F->InTutorial();
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
 if(F->Taken&&F->Taken->Busy())N.Busy=TEXT("taken");
 if(F->FarmSite&&F->FarmSite->VisitWantsGround())N.Busy=F->FarmSite->FieldVisit()?TEXT("field_visit"):TEXT("site_visit");
 if(M->HighwayUfo&&M->HighwayUfo->StageName()!=TEXT("idle")&&M->HighwayUfo->StageName()!=TEXT("done"))N.Busy=TEXT("highway_ufo");
 return N;
}

bool FS47Rhythm::Allow(FName Id,FName Within)
{
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

void FS47Rhythm::Began(FName Id)
{
 auto* F=M&&M->Freeform?M->Freeform.Get():nullptr;
 const auto* E=Rules.Find(Id);
 if(!F||!F->Director||!E)return;
 // Being taken at the end of a visit, at a door or under a night event's light (FS47Happenings::HoldsTaking) is the same meeting.
 if(Id==TEXT("taken")&&((F->IndoorDread&&F->IndoorDread->DoorActive())||(F->FarmSite&&F->FarmSite->VisitWantsGround())||F->Director->ActiveEvent()==TEXT("service_presence")||(F->Happenings&&F->Happenings->HoldsTaking())))return;
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

float FS47Rhythm::PerNight(FName Id,float Otherwise) const
{
 const auto* E=Rules.Find(Id);
 return E&&E->PerNight>=0?float(E->PerNight):Otherwise;
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
 T+=FString::Printf(TEXT("\nIgjen i dag: små %d  undersøkelser %d  himmel %d   Fare i natt %d/%s"),Now.MinorLeft,Now.InvestLeft,Now.SkyLeft,Now.DangerTonight,Rules.DangerPerNight<0?TEXT("-"):*FString::FromInt(Rules.DangerPerNight));
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
 {FS47RhythmNow A=N;A.LastDays.Add(TEXT("service_presence"),5);Check(Says(TEXT("service_presence"),A)==TEXT("cooldown"),TEXT("not on the day it was last"));A.LastDays.Add(TEXT("service_presence"),1);Check(R.May(TEXT("service_presence"),A).bMay,TEXT("and may four days on"));}
 {FS47RhythmNow A=N;A.Busy=TEXT("door_visit");Check(Says(TEXT("service_presence"),A)==TEXT("slot_taken:door_visit"),TEXT("one on the ground at a time"));Check(R.May(TEXT("sky_formation"),A).bMay==(A.Clock>=0&&InWindow(HourOf(A.Clock),21.5,5.5)),TEXT("the sky may run under it"));}
 {FS47RhythmNow A=N;A.RestLeft=100;Check(Says(TEXT("door_visit"),A)==TEXT("rest"),TEXT("no one comes during rest"));}
 {FS47RhythmNow A=N;A.Clock=4*86400.+23*3600.;A.MinorLeft=0;Check(Says(TEXT("flyover"),A)==TEXT("budget"),TEXT("the day's small budget stops a small one"));}
 {FS47RhythmNow A=N;A.Area=TEXT("road");Check(Says(TEXT("service_presence"),A)==TEXT("not_here"),TEXT("service_presence only at SARO"));}
 {FS47RhythmNow A=N;A.Draws.Add(TEXT("pendingThreatNight"),0);Check(Says(TEXT("service_presence"),A)==TEXT("not_drawn_tonight"),TEXT("a draw lost tonight says so"));}
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
