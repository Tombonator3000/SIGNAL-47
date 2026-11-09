#include "S47WeekCase.h"
#include "S47Drive.h"
#include "S47FarmBuildings.h"
#include "S47FarmSite.h"
#include "S47FarmShedDoor.h"
#include "S47FarmWorld.h"
#include "S47FieldRadio.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47FreeformDirector.h"
#include "S47Happenings.h"
#include "S47HighwayUfo.h"
#include "S47Main.h"
#include "S47MessageQueue.h"
#include "S47NightRhythm.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47Rhythm.h"
#include "S47Script.h"
#include "S47ShedGlow.h"
#include "S47ShedSound.h"
#include "S47ScriptPlayer.h"
#include "S47Taken.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "CoreGlobals.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/GarbageCollection.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47WeekCase,Log,All);

// WEEKLY-CASE-20261109, patch 1: the owner, the gate and way out A; patch 2: the shed's door, its light and its sounds, driven by the
// phases here (StepShed), and put away by every way out (OnClean). See S47WeekCase.h. Dale's lines go over the walkie as the big
// night's do (S47BigNight.cpp, StepBigAfter): "DALE: " and his words, one at a time; with no set to hear them, a slip by the phone at
// SARO. Never 47, never 08:15 or 20:15, nothing of Story's far side (S47Hap::Clean and S47Script::LintText, the self-test).
namespace S47Hap
{
 bool Clean(const FString& Text);
 bool Concluding(const FString& Text);
 FString ClockText(double Clock);
}
namespace S47Week
{
 constexpr uint32 SaltAuto=0x57454b31u,SaltLen=0x57454b32u,SaltRhythm=0x57454b33u;
 // Which of the farm's buildings is the feed shed (S47FarmBuildings::Sites: barn, house, machine shed, feed shed).
 constexpr int32 ShedSite=3;
 const TCHAR* const Farm=TEXT("the Alderete place");
 // The morning after the big night: one more thing, after his own. Where and when, never what.
 // CRIT9-20261109: the case is the night after next (CaseNightFor): his words say tomorrow night, not tonight.
 const TCHAR* const HookLines[]={
  TEXT("One more thing. Since the dishes went over by themselves, something on the bearing past the Alderete place is chewing my signal."),
  TEXT("The last nights, a little after two, for a few minutes. Tomorrow night, I think. Where and when I can tell you. What, I cannot. Out.")};
 const TCHAR* const HookSlipWords=TEXT("\"Something on the bearing past the Alderete place chews my signal a little after two. Tomorrow night, I think. Where and when. Not what. D.\"");
 // Under the big night's slip when there is one, else a slip of its own.
 FString HookSlip(bool bSecond)
 {
  return FString(bSecond?TEXT("A second slip under the first, the same hard hand: "):TEXT("A slip by the phone, in a hard square hand: "))+HookSlipWords;
 }
 const TCHAR* const NightCall=TEXT("It's there again. Same bearing.");
 const TCHAR* const NightDesk=TEXT("WALKIE / From the set on the desk, Dale's voice, flat: \"It's there again. Same bearing.\"");
 // Way out A: the morning after, he has heard. Sarcastic; no pay, nothing for Ward.
 const TCHAR* const MorningA[]={
  TEXT("Good morning. The Alderete feed shed. The door is lying in the yard in pieces, and it broke outward. From the inside."),
  TEXT("The padlock is still on its chain, still locked. And the floor in there has gone hard and shiny, like a pot out of the kiln."),
  TEXT("But you were busy, I am sure. Nobody pays for a door we did not look at. Out.")};
 const TCHAR* const MorningSlipA=TEXT("A slip by the phone, in a hard square hand: \"Alderete feed shed. Door broken OUTWARD. Padlock still locked. Floor gone like glaze. You were busy, I am sure. D.\"");
 const TCHAR* const FindingA=TEXT("The feed shed at the Alderete place, the morning after, as it was reported to SARO: the door broken outward from the inside, the padlock still locked on its chain, and the floor inside gone hard and glossy, like glaze. Nobody from the station was there.");
 // Patch 3: during the case, his hail on the walkie; answered, the bearing, where to keep out of sight, and the engine.
 const TCHAR* const HailFirst=TEXT("SARO night. You are at the Alderete place, yes? Answer me.");
 const TCHAR* const HailAgain=TEXT("SARO night. Pick up. My needle is doing it again.");
 const TCHAR* const HailGiveUp=TEXT("Fine. Do not answer. Out.");
 const TCHAR* const HelpLines[]={
  TEXT("Listen. It is the small shed past the house, the one with the chain. My needle goes to it and stays there."),
  TEXT("If anything comes out of it, you are not standing in the open. The hay in the barn, under the tractor, behind the water tank, the ditch. Low, and no lamp."),
  TEXT("And the truck: one turn of the key, then you wait. Do not flood the engine. Out.")};
 // The morning after B to E, at SARO (A's is above): his words follow the way out.
 const TCHAR* const MorningB[]={
  TEXT("The Alderete feed shed. You were there, and you drove off before it broke. Smart, maybe."),
  TEXT("On my side a burst, a few minutes after you left. So now we have a sound, a log and a door lying in the yard. Somebody out there will write to us about it, you will see. Out.")};
 const TCHAR* const MorningC[]={
  TEXT("You stayed. You hid, and you drove when it let you. I did not think you had it in you."),
  TEXT("On the array the same minute as at that shed. Write down every minute of it, now, before you sleep. Out.")};
 const TCHAR* const MorningD[]={
  TEXT("Your truck is standing in the Alderete yard, and you walked home? At night? Out there?"),
  TEXT("Fetch it in daylight. It costs you the morning, and while you are there, you look at that shed. Out.")};
 const TCHAR* const MorningE[]={
  TEXT("Where were you? Hours, and nothing on the set. Your things are lying in the Alderete yard."),
  TEXT("No. Do not tell me. Not over the radio. Fetch them in daylight, and then we talk. Out.")};
 FString SlipOf(const TArray<FString>& Lines)
 {
  FString All=FString::Join(Lines,TEXT(" "));
  All.ReplaceInline(TEXT(" Out."),TEXT(""));
  return FString::Printf(TEXT("A slip by the phone, in a hard square hand: \"%s D.\""),*All);
 }
 FString FindingKey(int32 Night){return FString::Printf(TEXT("weekcase_shed_a_d%d"),Night);}
 FString Dale(const FString& Text){return FString(TEXT("DALE"))+TEXT(": ")+Text;}
 // The night's pace (F10's natural night runs its minutes at speed), as the big night counts its own seconds.
 double Pace(const US47Main* M){return M&&M->Freeform&&M->Freeform->Director?FMath::Max(1.,M->Freeform->Director->TestPaceNow()):1.;}
}

// A whole word in a lower-case line (the self-test's spoiler check).
static bool FS47WeekCaseLocalWord(const FString& L,const TCHAR* W)
{
 const int32 N=FCString::Strlen(W);
 for(int32 At=L.Find(W);At!=INDEX_NONE;At=L.Find(W,ESearchCase::IgnoreCase,ESearchDir::FromStart,At+1))
  if((At==0||!FChar::IsAlpha(L[At-1]))&&(At+N>=L.Len()||!FChar::IsAlpha(L[At+N])))return true;
 return false;
}

const TCHAR* FS47WeekCase::PhaseName(EPhase P)
{
 switch(P)
 {
  case EPhase::Hook:return TEXT("hook");case EPhase::Day:return TEXT("day");case EPhase::Night:return TEXT("night");
  case EPhase::Phase1:return TEXT("phase1");case EPhase::Phase2:return TEXT("phase2");case EPhase::Phase3:return TEXT("phase3");
  case EPhase::Out:return TEXT("out");default:return TEXT("done");
 }
}
const TCHAR* FS47WeekCase::OutcomeName(EOutcome O)
{
 switch(O){case EOutcome::A:return TEXT("A");case EOutcome::B:return TEXT("B");case EOutcome::C:return TEXT("C");case EOutcome::D:return TEXT("D");case EOutcome::E:return TEXT("E");default:return TEXT("none");}
}

// ---------------------------------------------------------------- the plan and the gate
FS47WeekCase::FPlan FS47WeekCase::PlanFor(int32 Seed,int32 BigNight)
{
 FPlan P;
 P.Night=CaseNightFor(BigNight);
 const int32 N=FMath::Max(1,P.Night);
 // About five minutes, never the same twice in a week of games, and never a round number of Story's.
 P.Phase1Auto=270.+60.*double(FS47NightRhythm::Roll(Seed,N,S47Week::SaltAuto));
 P.Phase2Len=80.+20.*double(FS47NightRhythm::Roll(Seed,N,S47Week::SaltLen));
 P.Rhythm=FMath::Clamp(int32(FS47NightRhythm::Roll(Seed,N,S47Week::SaltRhythm)*4.f),0,3);
 return P;
}
bool FS47WeekCase::MayEscalate(double Hour,double InRate)
{
 const double R=FMath::Clamp(InRate,0.,600.);
 if(Hour>=AfterStory)return true;
 return Hour+EscalateMax*R/3600.<StoryFrom;
}

// ---------------------------------------------------------------- the pure step
FS47WeekCase::FOut FS47WeekCase::StepState(FState& S,const FPlan& P,const FIn& In)
{
 FOut O;
 const auto To=[&](EPhase Next,const TCHAR* Why){S.Phase=Next;S.Age=0;O.Why=Why;};
 const auto Over=[&](EOutcome Out,bool bSettled,const TCHAR* Why){S.Outcome=Out;S.bSettled=bSettled;O.bOver=true;O.bSettled=bSettled;S.bHeldForStory=false;if(!S.bDoorBroken){S.bDoorBroken=true;S.DoorNight=S.CaseNight;}To(EPhase::Out,Why);};
 const double Dt=FMath::Max(0.,In.Dt);
 switch(S.Phase)
 {
 case EPhase::Hook:
 {
  if(S.bForced){To(EPhase::Night,TEXT("F10"));break;}
  if(In.BigNight<0){To(EPhase::Done,TEXT("no big night in this game"));O.bDone=true;break;}
  if(In.BigNight==0||!In.bLive)break;
  S.CaseNight=P.Night>0?P.Night:CaseNightFor(In.BigNight);
  // A game that comes in after its night (a save from before this case existed) does not make it up, as the big night does not.
  if(In.Night>S.CaseNight){To(EPhase::Done,TEXT("this game came in after its night"));O.bDone=true;break;}
  const int32 After=In.BigNight+1;
  if(In.Night==After&&In.Hour>=DaleHookAt&&In.Hour<DaleHookUntil&&!S.bDaleHook)O.bDaleHook=true;
  if(In.Night>=S.CaseNight)To(EPhase::Day,TEXT("the case's day"));
  break;
 }
 case EPhase::Day:
  if(!In.bLive&&!S.bForced)break;
  if(In.Night>S.CaseNight){Over(EOutcome::A,true,TEXT("its night slept through"));break;}
  // CRIT9-20261109: the bearing slip on the case's own evening (the night before, the farm has nothing yet).
  if(!S.bForced&&In.Night==S.CaseNight&&In.Hour>=SlipFrom&&In.Hour<SlipUntil&&!S.bSlip)O.bSlip=true;
  if(In.Night==S.CaseNight&&In.Hour>=NightCallAt)To(EPhase::Night,TEXT("ten to two"));
  break;
 case EPhase::Night:
 {
  if(!In.bLive&&!S.bForced)break;
  // CRIT9-20261109: too late is when phases one and two would no longer fit before the night's end (the case would end in B with
  // the player still at the door), not only four o'clock.
  const double Fits=(FMath::Max(Phase1Min,P.Phase1Auto)+FMath::Clamp(P.Phase2Len,30.,150.))*FMath::Clamp(In.Rate,0.,600.)/3600.;
  const bool bTooLate=!S.bForced&&(In.Hour>=ArriveBy||(In.Hour>=ArriveFrom&&In.Hour+Fits>=LastHour));
  if(In.Night>S.CaseNight||bTooLate){Over(EOutcome::A,true,TEXT("never came: let be"));break;}
  if(!S.bForced&&!S.bNightCall&&In.Hour>=NightCallAt&&In.Hour<NightCallUntil)O.bNightCall=true;
  // Not on top of anything else on the ground: it waits for the ground to be free.
  if(In.bAtFarm&&!In.bTaken&&In.bGroundFree&&(S.bForced||In.Hour>=ArriveFrom)){S.CaseAge=0;O.bBegan=true;To(EPhase::Phase1,TEXT("came to the farm"));}
  break;
 }
 case EPhase::Phase1:
 {
  S.Age+=Dt;S.CaseAge+=Dt;
  if(In.bTaken){Over(EOutcome::E,true,TEXT("taken"));break;}
  if(!In.bAtFarm){Over(EOutcome::B,true,TEXT("left before the breach"));break;}
  // The caps: the phase's, the whole case's and the night's.
  if(S.Age>=Phase1Cap||S.CaseAge>=CaseCap){Over(EOutcome::B,true,TEXT("cap: phase one ran out"));break;}
  // CRIT9-20261109: the night ending with the player still at the farm is not B (that is driving off): it wants out now. Past 02:41
  // by then, so Story's minute is behind it.
  if(!S.bForced&&In.Hour>=LastHour){S.bHeldForStory=false;O.bEscalated=true;To(EPhase::Phase2,TEXT("the night is ending: it wants out"));break;}
  const bool bReady=S.Age>=(In.bKnocked?Phase1Min:FMath::Max(Phase1Min,P.Phase1Auto));
  if(!bReady)break;
  if(!MayEscalate(In.Hour,In.Rate)){S.bHeldForStory=true;break;}
  S.bHeldForStory=false;O.bEscalated=true;To(EPhase::Phase2,In.bKnocked?TEXT("knocked: it wants out"):TEXT("no knock: it wants out"));
  break;
 }
 case EPhase::Phase2:
  S.Age+=Dt;S.CaseAge+=Dt;
  if(In.bTaken){Over(EOutcome::E,true,TEXT("taken"));break;}
  if(!In.bAtFarm){Over(EOutcome::B,true,TEXT("left before the breach"));break;}
  if(S.Age>=FMath::Clamp(P.Phase2Len,30.,150.)||S.CaseAge>=CaseCap){O.bBreach=true;S.bDoorBroken=true;S.DoorNight=S.CaseNight;To(EPhase::Phase3,TEXT("the breach"));}
  break;
 case EPhase::Phase3:
  S.Age+=Dt;S.CaseAge+=Dt;
  if(In.bTaken){Over(EOutcome::E,true,TEXT("taken"));break;}
  if(!In.bAtFarm){Over(In.bDriving?EOutcome::C:EOutcome::D,true,In.bDriving?TEXT("hid and drove"):TEXT("got away on foot"));break;}
  // The search gives up (patch 4: at ninety seconds or four looks; this is its cap): the shed goes dark, and the player is still here.
  if(S.Age>=Phase3Cap||S.CaseAge>=CaseCap){Over(EOutcome::C,false,TEXT("the search gave up"));}
  break;
 case EPhase::Out:
  S.Age+=Dt;
  if(!S.bSettled)
  {
   // Still at the farm after the search: how they leave says which way out it was; staying until the cap is hiding it out.
   if(In.bTaken){S.Outcome=EOutcome::E;S.bSettled=true;O.bSettled=true;}
   else if(!In.bAtFarm){S.Outcome=In.bDriving?EOutcome::C:EOutcome::D;S.bSettled=true;O.bSettled=true;}
   else if(S.Age>=OutCap){S.Outcome=EOutcome::C;S.bSettled=true;O.bSettled=true;}
   break;
  }
  if(S.bMorning){To(EPhase::Done,TEXT("its morning has come"));O.bDone=true;break;}
  // The wait for its morning has a cap too: two nights on, nobody at SARO to hear it, it is let go unsaid.
  if(In.Night>S.CaseNight+2){To(EPhase::Done,TEXT("its morning never came to SARO"));O.bDone=true;break;}
  // A: the morning after, from half past eight, wherever the player is (Dale on the walkie, or the slip at SARO). The rest: written
  // all together when the player is back at SARO (patch 5 gives them their records).
  if(S.Outcome==EOutcome::A?(In.Night>S.CaseNight&&In.Hour>=DaleHookAt):In.bAtSaro)O.bMorning=true;
  break;
 case EPhase::Done:
  break;
 }
 return O;
}
FS47WeekCase::EPhase FS47WeekCase::SavedPhase(const FState& S)
{
 if(S.Phase==EPhase::Phase1||S.Phase==EPhase::Phase2||S.Phase==EPhase::Phase3)return EPhase::Night;
 if(S.Phase==EPhase::Out&&!S.bSettled)return EPhase::Night;
 return S.Phase;
}
S47J::FObj FS47WeekCase::SaveState(const FState& S)
{
 if(S.Phase==EPhase::Hook&&S.CaseNight==0&&!S.bDaleHook&&!S.bSlip)return nullptr;
 auto O=S47J::New();
 const EPhase P=SavedPhase(S);
 S47J::Set(O,TEXT("phase"),PhaseName(P));
 S47J::Set(O,TEXT("night"),S.CaseNight);
 if(P==EPhase::Out||P==EPhase::Done)S47J::Set(O,TEXT("outcome"),OutcomeName(S.Outcome));
 if(S.bDaleHook)S47J::Set(O,TEXT("hook"),true);
 if(S.bSlip)S47J::Set(O,TEXT("slip"),true);
 if(S.bNightCall)S47J::Set(O,TEXT("call"),true);
 if(S.bMorning)S47J::Set(O,TEXT("morning"),true);
 if(S.bForced)S47J::Set(O,TEXT("forced"),true);
 // The door broken out: only with a way out (out there nothing is saved, and Continue finds it locked again).
 if((P==EPhase::Out||P==EPhase::Done)&&S.bDoorBroken){S47J::Set(O,TEXT("door"),TEXT("broken"));S47J::Set(O,TEXT("doorNight"),S.DoorNight);}
 return O;
}
FS47WeekCase::FState FS47WeekCase::LoadState(const S47J::FObj& O)
{
 FState S;
 if(!O.IsValid())return S;
 const FString P=S47J::Str(O,TEXT("phase"));
 for(EPhase E:{EPhase::Hook,EPhase::Day,EPhase::Night,EPhase::Out,EPhase::Done})if(P==PhaseName(E))S.Phase=E;
 S.CaseNight=FMath::Clamp(S47J::Int(O,TEXT("night")),0,100000);
 const FString W=S47J::Str(O,TEXT("outcome"));
 for(EOutcome E:{EOutcome::A,EOutcome::B,EOutcome::C,EOutcome::D,EOutcome::E})if(W==OutcomeName(E))S.Outcome=E;
 // An Out with no outcome is not a way out: the night is still waiting.
 if(S.Phase==EPhase::Out&&S.Outcome==EOutcome::None)S.Phase=EPhase::Night;
 S.bDaleHook=S47J::Bool(O,TEXT("hook"));S.bSlip=S47J::Bool(O,TEXT("slip"));S.bNightCall=S47J::Bool(O,TEXT("call"));
 S.bMorning=S47J::Bool(O,TEXT("morning"));S.bForced=S47J::Bool(O,TEXT("forced"));
 S.bDoorBroken=(S.Phase==EPhase::Out||S.Phase==EPhase::Done)&&S47J::Str(O,TEXT("door"))==TEXT("broken");
 S.DoorNight=S.bDoorBroken?FMath::Clamp(S47J::Int(O,TEXT("doorNight")),0,100000):0;
 S.bSettled=true;
 return S;
}

FS47FarmShedDoor::EState FS47WeekCase::DoorFor(const FState& S,int32 Night)
{
 using ED=FS47FarmShedDoor::EState;
 if(S.bDoorBroken&&(S.Phase==EPhase::Phase3||S.Phase==EPhase::Out||S.Phase==EPhase::Done)&&Night<=S.DoorNight+BrokenNights)return ED::Broken;
 if(S.Phase==EPhase::Day||S.Phase==EPhase::Night||S.Phase==EPhase::Phase1||S.Phase==EPhase::Phase2)return ED::Locked;
 return ED::Open;
}
FS47FarmShedDoor::EMood FS47WeekCase::MoodFor(EPhase P)
{
 return P==EPhase::Phase1?FS47FarmShedDoor::EMood::Rattle:P==EPhase::Phase2?FS47FarmShedDoor::EMood::Kicks:FS47FarmShedDoor::EMood::Still;
}

// ---------------------------------------------------------------- asked by everyone else
bool FS47WeekCase::Holds(const US47Main* M)
{
 const FS47FreeformSession* F=M&&M->IsFreeform()&&M->Freeform?M->Freeform.Get():nullptr;
 return F&&F->WeekCase&&F->WeekCase->HoldsGround();
}
bool FS47WeekCase::HoldsFarm(const US47Main* M)
{
 const FS47FreeformSession* F=M&&M->IsFreeform()&&M->Freeform?M->Freeform.Get():nullptr;
 return F&&F->WeekCase&&F->WeekCase->HoldsFarm();
}
FString FS47WeekCase::DangerOf(const US47Main* M){return Holds(M)?FString(TEXT("the week's case on the ground")):FString();}
FString FS47WeekCase::RestBlockOf(const US47Main* M){return Holds(M)?FString(TEXT("Not now. Not with that shed the way it is.")):FString();}

FVector2D FS47WeekCase::ShedRoad()
{
 const TArray<S47FarmBuildings::FSite>& Sites=S47FarmBuildings::Sites();
 if(!Sites.IsValidIndex(S47Week::ShedSite))return FVector2D(84.5,206.5);
 return FVector2D(Sites[S47Week::ShedSite].X,Sites[S47Week::ShedSite].Z);
}
FVector FS47WeekCase::ShedDoor()
{
 const FVector2D R=ShedRoad();
 return S47FarmWorld::RoadToArea(TEXT("saro"),FVector(R.X,S47FarmWorld::FloorRoad(R.X,R.Y),R.Y));
}

// ---------------------------------------------------------------- in the game
FS47WeekCase::~FS47WeekCase(){Stop();}
void FS47WeekCase::Begin(const S47J::FObj& Saved)
{
 Stop();
 S=LoadState(Saved);
 bShedFresh=true;
 bDevBefore=false;DaleLines.Reset();DaleOwns=EDale::None;bDoorWaitSaid=false;LastClock=-1;TryIn=0;Paced=0;HookSince=-1;SaidIn=0;
 if(Saved.IsValid())UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: loaded %s"),*StateText());
}
void FS47WeekCase::Stop()
{
 Clean(TEXT("stop"));
 if(DoorObj)DoorObj->Stop();
 if(GlowObj)GlowObj->Stop();
}
S47J::FObj FS47WeekCase::Save() const{return SaveState(S);}
FString FS47WeekCase::StateText() const
{
 const FString Shed=FString::Printf(TEXT("; %s; %s"),DoorObj?*DoorObj->StateText():TEXT("door not drawn"),GlowObj?*GlowObj->StateText():TEXT("glow out"));
 return FString::Printf(TEXT("%s, night %d, outcome %s%s%s"),PhaseName(S.Phase),S.CaseNight,OutcomeName(S.Outcome),S.bForced?TEXT(", F10"):TEXT(""),S.bHeldForStory?TEXT(", waiting out Story's minute"):TEXT(""))+Shed;
}

FS47WeekCase::FIn FS47WeekCase::ReadIn(double Dt)
{
 FIn In;
 auto* F=M->Freeform.Get();
 const double Clock=F->Clock();
 In.Night=FS47FreeformSession::NightOf(Clock);In.Hour=FS47Happenings::NightHour(Clock);
 const bool bHeld=F->Held()||M->ModalOpen();
 In.Dt=bHeld?0.:Dt*S47Week::Pace(M);
 // The clock's own rate, measured (a rest or a trip jumps it: those frames say nothing).
 if(LastClock>=0&&Dt>0&&!bHeld){const double R=(Clock-LastClock)/Dt;if(R>0&&R<900)Rate=FMath::Lerp(Rate,R,.1);}
 LastClock=Clock;In.Rate=Rate;
 In.BigNight=F->Happenings?F->Happenings->BigHappened():0;
 In.bLive=F->RulesLive()&&!F->InTutorial()&&!(M->Flow&&M->Flow->bDevRun);
 In.bDriving=M->World&&M->World->bDriving;
 if(M->Player&&M->Web&&M->Web->Area()==TEXT("saro"))
 {
  const FS47DriveController* D=In.bDriving&&M->World?M->World->CabDrive():nullptr;
  const FVector At=D?D->Pos:M->Player->Pos;const FVector Door=ShedDoor();
  In.bAtFarm=FVector2D::Distance(FVector2D(At.X,At.Z),FVector2D(Door.X,Door.Z))<FarmReach;
 }
 // CRIT9-20261109: the ground as everyone else has it (its own Busy is only on the ground, so not yet ours here).
 {
  const FName Busy=M->Rhythm?M->Rhythm->ReadNow().Busy:NAME_None;
  In.bGroundFree=(Busy.IsNone()||Busy==FName(TEXT("week_case")))&&!(F->Director&&F->Director->Busy())&&!(F->Taken&&F->Taken->Busy())
   &&!(F->FarmSite&&F->FarmSite->VisitWantsGround())&&!(M->HighwayUfo&&M->HighwayUfo->Busy());
 }
 In.bAtSaro=FS47FieldRadio::InSaroBuilding(M);
 In.bTaken=F->Taken&&(F->Taken->Stage()==FS47Taken::EStage::Lift||F->Taken->Stage()==FS47Taken::EStage::Away);
 // Patch 2: knocked at the shed's door (its knock closed, whether or not anything answered).
 In.bKnocked=DoorObj.IsValid()&&DoorObj->Knocked();
 // Patch 3: something at the truck (patch 4's searcher, or the reserve), from the truck if she is in it, else from her.
 {
  const FS47DriveController* D=In.bDriving&&M->World?M->World->CabDrive():nullptr;
  const FVector From=D?D->Pos:M->Player?M->Player->Pos:ShedDoor();
  ReadSearcher(From,In.bSearcherNear,In.SearcherDist);
 }
 return In;
}

void FS47WeekCase::Step(double Dt)
{
 if(!M||!M->IsFreeform()||!M->Freeform||!M->Freeform->Active())return;
 Paced+=Dt;
 const FPlan P=PlanFor(M->Freeform->Seed(),S.CaseNight>0&&S.bForced?S.CaseNight-2:(M->Freeform->Happenings?M->Freeform->Happenings->BigHappened():0));
 if(S.Phase!=EPhase::Done)
 {
  const EPhase Was=S.Phase;
  const FIn In=ReadIn(Dt);
  const FOut O=StepState(S,P,In);
  if(S.Phase!=Was)UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: %s -> %s (%s) at %s, night %d"),PhaseName(Was),PhaseName(S.Phase),*O.Why,*S47Hap::ClockText(M->Freeform->Clock()),In.Night);
  // CRIT9-20261109: from ten to two the farm is the case's: its lights' visit tonight yields, as it does to the big night.
  if(S.Phase==EPhase::Night&&Was!=EPhase::Night&&M->Freeform->FarmSite)M->Freeform->FarmSite->YieldTo(TEXT("week_case"));
  Apply(O);
  StepDale();
  // Patch 3: the truck on the yard and the cab, the thoughts, Dale's hail.
  StepCab(Dt);
  StepThoughts(M->Freeform->Held()||M->ModalOpen()?0.:Dt);
  StepHail();
 }
 // Patch 3: the morning's lines go on being said after the case is done (they were queued as it ended).
 else StepDale();
 // The door stands whatever the case is doing (open before it, broken a week after it).
 StepShed(Dt,P);
}

bool FS47WeekCase::PlayerInShed() const
{
 if(!M||!M->Player||(M->World&&M->World->bDriving)||!M->Web||M->Web->Area()!=TEXT("saro"))return false;
 const FVector Origin=S47FarmWorld::RoadToArea(TEXT("saro"),FVector::ZeroVector);
 return S47FarmBuildings::InShed(M->Player->Pos.X-Origin.X,M->Player->Pos.Z-Origin.Z,M->Player->Radius+.3);
}

// Patch 2: the phase gives the door its state and its mood, and the light its level. Held (a menu, a paper), nothing moves.
void FS47WeekCase::StepShed(double Dt,const FPlan& P)
{
 auto* F=M->Freeform.Get();
 const bool bHeld=F->Held()||M->ModalOpen();
 const double D=bHeld?0.:Dt;
 const int32 Night=FS47FreeformSession::NightOf(F->Clock());
 if(!DoorObj)DoorObj=MakeShared<FS47FarmShedDoor>(M);
 if(!GlowObj)GlowObj=MakeShared<FS47ShedGlow>(M);
 DoorObj->Plan(F->Seed(),S.CaseNight,P.Rhythm);
 FS47FarmShedDoor::EState Want=DoorFor(S,Night);
 // CRIT9-20261109: never locked with the player inside, or in the doorway: it stays open, and is tried again every frame.
 if(Want==FS47FarmShedDoor::EState::Locked&&DoorObj->State()==FS47FarmShedDoor::EState::Open&&PlayerInShed())
 {
  Want=FS47FarmShedDoor::EState::Open;
  if(!bDoorWaitSaid){bDoorWaitSaid=true;UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: door waits: player inside"));}
 }
 else if(Want!=FS47FarmShedDoor::EState::Open)bDoorWaitSaid=false;
 // A breach is animated only as it happens on the ground; a load or a later night simply finds the door so.
 // Patch 3: phase two's second turn of the key: the kicks stop for a while, something listening.
 DoorObj->Set(Want,Cab.Listen>0&&S.Phase==EPhase::Phase2?FS47FarmShedDoor::EMood::Still:MoodFor(S.Phase),!bShedFresh&&S.Phase==EPhase::Phase3);
 bShedFresh=false;
 DoorObj->Step(D);
 GlowObj->Step(D,FS47ShedGlow::LevelFor(S.Phase,S.Age),Want==FS47FarmShedDoor::EState::Broken);
}

void FS47WeekCase::Apply(const FOut& O)
{
 auto* F=M->Freeform.Get();
 if(O.bDaleHook&&Paced>=TryIn)SayDaleHook();
 if(O.bSlip&&Paced>=TryIn)GiveSlip();
 if(O.bNightCall&&Paced>=TryIn)SayNightCall();
 if(O.bBegan)
 {
  // The night's ground is the case's: the rhythm's one start (a danger, the night's slot), and no script on top of it.
  if(M->Rhythm){M->Rhythm->Began(TEXT("week_case"));bRhythmBegan=true;}
  // Patch 3: the cab, the thoughts and the hail are this visit's.
  Cab=FCab();TurnsSeen=0;SinceTurn=1e4;bThoughtLocked=bThoughtHeld=false;NearDoorFor=0;bHailed=false;ThoughtsDue.Reset();
  HoldScripts(true);
  // Patch 2: the door's knocks are this visit's; the light, the sounds and the omen go with every way out.
  if(DoorObj)DoorObj->Calm();
  {
   const TWeakPtr<FS47FarmShedDoor> WeakDoor=DoorObj;const TWeakPtr<FS47ShedGlow> WeakGlow=GlowObj;
   OnClean([WeakDoor,WeakGlow]{if(const TSharedPtr<FS47FarmShedDoor> D=WeakDoor.Pin())D->Calm();if(const TSharedPtr<FS47ShedGlow> G=WeakGlow.Pin())G->Off();});
  }
  UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: phase one at the shed, %s"),*S47Hap::ClockText(F->Clock()));
 }
 if(O.bEscalated)UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: phase two at %s"),*S47Hap::ClockText(F->Clock()));
 // Patch 3: the truck was the last sound before the breach: it goes there first (the reserve, until patch 4).
 if(O.bBreach&&SinceTurn<60)SinceTurn=0;
 if(O.bBreach)UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: phase three, the breach, at %s"),*S47Hap::ClockText(F->Clock()));
 if(O.bOver)
 {
  Clean(TEXT("over"));
  UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: over, way out %s%s"),OutcomeName(S.Outcome),S.bSettled?TEXT(""):TEXT(" (until the player leaves)"));
 }
 if(O.bSettled)UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: way out %s"),OutcomeName(S.Outcome));
 if(O.bMorning&&Paced>=TryIn)Morning();
 if(O.bDone){UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: done, way out %s"),OutcomeName(S.Outcome));M->RequestAutosave();}
}

// Every way out: the truck's stall, the view, the figures, the scripts and the rhythm's rest. Safe to call at any time, also as the
// engine goes down (then only the plain state is cleared).
void FS47WeekCase::Clean(const TCHAR* Why)
{
 const bool bWorld=!IsEngineExitRequested()&&!GExitPurge&&!IsGarbageCollecting()&&IsValid(M);
 if(bStall&&bWorld&&IsValid(M->World))M->World->Trouble.Release();
 // Patch 3: the stall from the yard catches on the next turn, the shake and the light end (also on a stop with nothing begun).
 StopCab();ThoughtsDue.Reset();
 bStall=false;
 if(AS47Walker* W=bWorld?Hooked.Get():nullptr)W->CameraOverride=Before;
 Hooked.Reset();Before=nullptr;
 TArray<TFunction<void()>> Fns=MoveTemp(Cleanups);Cleanups.Reset();
 if(bWorld)for(TFunction<void()>& Fn:Fns)if(Fn)Fn();
 HoldScripts(false);
 if(bRhythmBegan&&bWorld&&M->Rhythm)M->Rhythm->Ended(TEXT("week_case"),true);
 bRhythmBegan=false;
 if(bWorld&&FCString::Strcmp(Why,TEXT("stop"))!=0)UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: cleaned up (%s)"),Why);
}
void FS47WeekCase::HoldScripts(bool bOn)
{
 if(bOn==bScriptsHeld)return;
 bScriptsHeld=bOn;
 FS47ScriptPlayer::Hold(TEXT("week_case"),bOn);
}
void FS47WeekCase::HoldStall()
{
 if(!M||!M->World)return;
 bStall=true;
}
void FS47WeekCase::HookCamera(TFunction<bool(double,FS47View&)> Frame)
{
 AS47Walker* W=M?M->Player.Get():nullptr;
 if(!W||!Frame)return;
 if(Hooked.IsValid())W->CameraOverride=Before;
 Before=W->CameraOverride;
 TFunction<bool(double,const FVector2D&,const FVector2D&,FS47View&)> Prev=Before;
 W->CameraOverride=[Frame,Prev](double Dt,const FVector2D& Move,const FVector2D& LookIn,FS47View& Out)
 {
  if(Frame(Dt,Out))return true;
  return Prev?Prev(Dt,Move,LookIn,Out):false;
 };
 Hooked=W;
}

// ---------------------------------------------------------------- Dale, the slip, the morning
// CRIT9-20261109: only a set that hears him now (in hand, not left out, on his channel) takes the lines; otherwise the slip by the
// phone. What they say is marked said when the last is heard (DaleHeard), so that lines let go unheard leave the slip its turn.
bool FS47WeekCase::Dale(const TArray<FString>& Lines,EDale Who)
{
 if(!M->FieldRadio.IsValid()||!M->FieldRadio->HearsDale()||!DaleLines.IsEmpty())return false;
 for(const FString& L:Lines)DaleLines.Add(S47Week::Dale(L));
 DaleOwns=Who;SaidIn=Paced+.5;DaleUntil=Paced+240;
 return true;
}
void FS47WeekCase::DaleHeard(EDale Who)
{
 switch(Who)
 {
 case EDale::Hook:S.bDaleHook=true;UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: the hook heard on the walkie"));M->RequestAutosave();break;
 case EDale::Night:S.bNightCall=true;UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: ten to two heard on the walkie"));break;
 case EDale::Morning:MorningDone();break;
 default:break;
 }
}
void FS47WeekCase::StepDale()
{
 // Not heard in four minutes (the set off his channel all that time): he gives up on the rest, and what they said is not marked said.
 if(!DaleLines.IsEmpty()&&Paced>DaleUntil)
 {
  UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: %d of Dale's lines not heard, let go (the slip by the phone instead)"),DaleLines.Num());
  DaleLines.Reset();DaleOwns=EDale::None;TryIn=0;
 }
 if(DaleLines.IsEmpty()||Paced<SaidIn||!M->FieldRadio.IsValid()||M->ModalOpen())return;
 // Not over his own voice still playing, nor over a hail waiting to be answered.
 if(M->FieldRadio->DaleSpeaking()||M->FieldRadio->Hailing()){SaidIn=Paced+.5;return;}
 const FString Line=DaleLines[0];
 if(M->FieldRadio->Message(Line))
 {
  DaleLines.RemoveAt(0);
  // The set's own reading time (FS47FieldRadio::Message), so that one line is not cut by the next.
  SaidIn=Paced+FMath::Clamp(3.+double(Line.Len()-6)/18.,6.,11.)+.5;
  if(DaleLines.IsEmpty()){const EDale Who=DaleOwns;DaleOwns=EDale::None;DaleHeard(Who);}
 }
 else SaidIn=Paced+3;
}
void FS47WeekCase::SayDaleHook()
{
 TryIn=Paced+2;
 if(DaleOwns==EDale::Hook)return; // on the set now
 auto* F=M->Freeform.Get();
 FS47Happenings* H=F->Happenings.Get();
 // After his own morning about the big night: answered, slipped, or given up on by ten thirty.
 const double Hour=FS47Happenings::NightHour(F->Clock());
 const bool bHisOwn=H&&(H->BigHas(TEXT("dale"))||H->BigHas(TEXT("slip"))||H->BigHas(TEXT("missed")));
 if(!bHisOwn&&Hour<10.5)return;
 if(HookSince<0){HookSince=Paced;return;}
 // His own three lines take about twenty-five seconds on the set.
 if(Paced-HookSince<26||M->ModalOpen()||(M->FieldRadio.IsValid()&&(M->FieldRadio->DaleSpeaking()||M->FieldRadio->Hailing())))return;
 TArray<FString> Lines;for(const TCHAR* L:S47Week::HookLines)Lines.Add(L);
 if(Dale(Lines,EDale::Hook)){UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: the hook, Dale on the walkie"));return;}
 // No set, or it is off his channel: the slip by the phone, read at the station indoors.
 if(FS47FieldRadio::InSaroBuilding(M)&&M->UI)
 {
  M->UI->Toast(S47Week::HookSlip(H&&H->BigHas(TEXT("slip"))),10);
  S.bDaleHook=true;UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: the hook, Dale's slip by the phone"));M->RequestAutosave();
 }
}
void FS47WeekCase::GiveSlip()
{
 TryIn=Paced+5;
 auto* F=M->Freeform.Get();
 if(!F->Places)return;
 const FVector2D Shed=ShedRoad();
 if(F->Places->PointAt(S47Week::Farm,Shed.X,Shed.Y))
 {
  S.bSlip=true;UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: the evening's bearing slip points at the farm"));
  return;
 }
 // The night's one pointing-out went elsewhere first: no second, as the rule is (once said, not asked every frame).
 if(F->Director&&!F->Director->MayPointOut(FS47FreeformSession::NightOf(F->Clock())))
 {S.bSlip=true;UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: tonight's one bearing had already pointed elsewhere; no slip for the farm"));}
}
void FS47WeekCase::SayNightCall()
{
 TryIn=Paced+3;
 if(M->ModalOpen()||DaleOwns==EDale::Night)return;
 if(Dale({S47Week::NightCall},EDale::Night)){UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: ten to two, Dale: same bearing"));return;}
 if(FS47FieldRadio::InSaroBuilding(M)&&M->UI){M->UI->Toast(S47Week::NightDesk,7);S.bNightCall=true;UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: ten to two, Dale from the desk set"));}
}
void FS47WeekCase::Morning()
{
 TryIn=Paced+3;
 if(M->ModalOpen()||DaleOwns==EDale::Morning)return;
 // A's on the set or its slip; B to E, Dale's words at SARO follow the way out (their records are patch 5's). On the set they are
 // marked when the last is heard (DaleHeard), the slip at once.
 const TArray<FString> Lines=MorningLines(S.Outcome);
 if(Lines.IsEmpty()){MorningDone();return;}
 if(Dale(Lines,EDale::Morning))return;
 if(!FS47FieldRadio::InSaroBuilding(M)||!M->UI)return;
 M->UI->Toast(S.Outcome==EOutcome::A?FString(S47Week::MorningSlipA):S47Week::SlipOf(Lines),11);
 MorningDone();
}
void FS47WeekCase::MorningDone()
{
 auto* F=M->Freeform.Get();
 if(S.Outcome==EOutcome::A&&F)
 {
  // A faint line in case 8 (OUT ON THE LAND): a lead, no pay, and not a night record Ward counts (FS47Happenings::IsNightFinding).
  const FString Key=S47Week::FindingKey(S.CaseNight);
  if(!F->HasFinding(Key))F->RecordEvidence(Key,S47Week::FindingA);
 }
 // B to E: their records are patch 5's; here only the morning is marked.
 S.bMorning=true;
 UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: the morning after, way out %s"),OutcomeName(S.Outcome));
 M->RequestAutosave();
}

TArray<FString> FS47WeekCase::MorningLines(EOutcome O)
{
 TArray<FString> L;
 const auto Add=[&L](const auto& Arr){for(const TCHAR* T:Arr)L.Add(T);};
 switch(O)
 {
 case EOutcome::A:Add(S47Week::MorningA);break;case EOutcome::B:Add(S47Week::MorningB);break;case EOutcome::C:Add(S47Week::MorningC);break;
 case EOutcome::D:Add(S47Week::MorningD);break;case EOutcome::E:Add(S47Week::MorningE);break;default:break;
 }
 return L;
}
TArray<FString> FS47WeekCase::HelpLines(){TArray<FString> L;for(const TCHAR* T:S47Week::HelpLines)L.Add(T);return L;}

// Patch 3: Dale's hail during the case, a minute or so on the ground (or as the truck dies): heard only if she answers (T), then his
// help, one line at a time. Not answered, he gives up. One hail a visit; with no set on his channel it is simply not heard.
void FS47WeekCase::StepHail()
{
 if(bHailed||!HoldsGround()||!M->FieldRadio.IsValid()||M->ModalOpen())return;
 const bool bDue=(S.Phase==EPhase::Phase1&&S.Age>=70.)||(Cab.bStalled&&Cab.StallAge>=8.)||S.Phase==EPhase::Phase2;
 if(!bDue||!DaleLines.IsEmpty()||M->FieldRadio->DaleSpeaking()||M->FieldRadio->Hailing()||!M->FieldRadio->Owned())return;
 FS47FieldRadio::FHail H;H.First=S47Week::HailFirst;H.Again=S47Week::HailAgain;H.GiveUp=S47Week::HailGiveUp;
 const TWeakPtr<uint8> A=Alive;
 if(M->FieldRadio->Hail(H,[this,A]
  {
   if(!A.IsValid())return;
   UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: Dale answered on the walkie: his help"));
   if(Dale(HelpLines(),EDale::Help))bHelpSaid=true;
  },[A]{if(A.IsValid())UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: Dale's hail not answered"));}))
 {
  bHailed=true;UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: Dale hails on the walkie"));
 }
 else bHailed=true; // not heard (no set, another channel): not asked again every frame
}

// ---------------------------------------------------------------- F10
bool FS47WeekCase::DevStart(FString& Why)
{
 auto* F=M&&M->Freeform?M->Freeform.Get():nullptr;
 if(!F||!F->Active()||!M->IsFreeform()){Why=TEXT("Freeform only");return false;}
 if(!bDevBefore){DevBefore=S;bDevBefore=true;}
 Clean(TEXT("F10 start"));
 S=FState();S.Phase=EPhase::Night;S.bForced=true;S.bDaleHook=S.bSlip=S.bNightCall=true;
 S.CaseNight=FS47FreeformSession::NightOf(F->Clock());
 UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: F10 start, night %d: waiting at the farm"),S.CaseNight);
 return true;
}
bool FS47WeekCase::DevForceA(FString& Why)
{
 auto* F=M&&M->Freeform?M->Freeform.Get():nullptr;
 if(!F||!F->Active()||!M->IsFreeform()){Why=TEXT("Freeform only");return false;}
 if(!bDevBefore){DevBefore=S;bDevBefore=true;}
 Clean(TEXT("F10 way out A"));
 S=FState();S.Phase=EPhase::Out;S.Outcome=EOutcome::A;S.bSettled=true;S.bForced=true;S.bDaleHook=S.bSlip=S.bNightCall=true;
 // The night before this morning when it is morning (to noon); tonight otherwise (its morning is the next).
 const double Clock=F->Clock();const double Hour=FS47Happenings::NightHour(Clock);
 S.CaseNight=FS47FreeformSession::NightOf(Clock)-(Hour<12.?1:0);
 if(F->HasFinding(S47Week::FindingKey(S.CaseNight))){Why=TEXT("this night's record of it is already kept");}
 UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: F10 way out A for night %d; its morning is next"),S.CaseNight);
 return true;
}
void FS47WeekCase::DevStop()
{
 Clean(TEXT("F10 stop"));
 DaleLines.Reset();DaleOwns=EDale::None;
 S=bDevBefore?DevBefore:FState();
 if(S.Phase==EPhase::Phase1||S.Phase==EPhase::Phase2||S.Phase==EPhase::Phase3)S.Phase=EPhase::Night;
 bDevBefore=false;
 UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: F10 stop: %s"),*StateText());
}

bool FS47WeekCase::DevPhase(EPhase P,FString& Why)
{
 if(!HoldsGround()){Why=TEXT("the case is not on the ground at the shed");return false;}
 if(P!=EPhase::Phase1&&P!=EPhase::Phase2&&P!=EPhase::Phase3){Why=TEXT("only phases one to three");return false;}
 S.Phase=P;S.Age=0;S.bHeldForStory=false;
 if(P==EPhase::Phase3){S.bDoorBroken=true;S.DoorNight=S.CaseNight;}
 UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: F10 to %s"),PhaseName(P));
 return true;
}
bool FS47WeekCase::DevKnock(FString& Why)
{
 if(S.Phase!=EPhase::Phase1){Why=TEXT("only in phase one");return false;}
 if(!DoorObj||!DoorObj->DevKnock()){Why=TEXT("the door is not drawn or not locked");return false;}
 return true;
}

// ---------------------------------------------------------------- self-test
bool FS47WeekCase::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const FString& Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s week case: %s"),B?TEXT("PASS"):TEXT("FAIL"),*Why));};
 // The plan: two nights after the big night (the fourth or fifth), none without one, the same seed the same plan, never Story's number.
 {
  bool bNights=true,bSame=true,bNo47=true,bRange=true;
  for(int32 Seed=1;Seed<=2000;++Seed)
  {
   const FS47Happenings::FBigPlan B=FS47Happenings::BigPlanFor(Seed);
   const FPlan P=PlanFor(Seed,B.Night),Q=PlanFor(Seed,B.Night);
   bNights&=(P.Night==4||P.Night==5)&&P.Night==B.Night+2;
   bSame&=P.Night==Q.Night&&P.Phase1Auto==Q.Phase1Auto&&P.Phase2Len==Q.Phase2Len&&P.Rhythm==Q.Rhythm;
   bRange&=P.Phase1Auto>=Phase1Min&&P.Phase1Auto<=Phase1Cap&&P.Phase2Len>=60&&P.Phase2Len<=120;
   bNo47&=FMath::RoundToInt(P.Phase1Auto)!=47&&FMath::RoundToInt(P.Phase2Len)!=47&&FMath::RoundToInt(P.Phase1Auto/60.)!=47;
  }
  Check(bNights,TEXT("the case is always the fourth or fifth night, two after the big night"));
  Check(PlanFor(7,0).Night==0&&PlanFor(7,-1).Night==0,TEXT("no case night before the big night has come, or in a game without one"));
  Check(bSame&&bRange,TEXT("the same seed plans the same case, its waits inside their caps"));
  Check(bNo47&&Phase3Cap!=47&&Phase1Min!=47&&EscalateMax!=47&&OutCap!=47&&CaseCap!=47,TEXT("no wait or cap lasts Story's number of seconds"));
 }
 // The gate on Story's dark minute: phases two and three never run into 02:20 to 02:40.
 {
  bool bNever=true;
  for(double R:{1.,5.,14.67,30.})for(double H=26.;H<28.;H+=1./240.)
   if(MayEscalate(H,R)){const double End=H+EscalateMax*R/3600.;bNever&=!(H<StoryTo&&End>StoryFrom);}
  Check(bNever,TEXT("the escalation never overlaps 02:20 to 02:40, at any rate of the clock"));
  Check(MayEscalate(26.+5./60.,1.)&&!MayEscalate(26.+5./60.,14.67)&&!MayEscalate(26.5,1.)&&MayEscalate(AfterStory,14.67)&&!MayEscalate(26.+40./60.,1.),
   TEXT("before 02:13 only when it ends before 02:20; never inside; from 02:41 always"));
 }
 // Each way out, from the pure machine.
 const FPlan P=PlanFor(11,2);
 const auto At=[](int32 Night,double Hour){FIn In;In.Night=Night;In.Hour=Hour;In.BigNight=2;In.Rate=14.67;In.Dt=0;return In;};
 // Runs the state on with In for Secs seconds of play in steps of Step; returns the last step's out, and any flags seen on the way.
 // It stops where the case leaves the ground (or, begun in Out, where its way out is settled).
 const auto Run=[&P](FState& S,FIn In,double Secs,double Step,FOut* Any)
 {
  FOut Last;
  const bool bFromOut=S.Phase==EPhase::Out;
  for(double T=0;T<Secs;T+=Step)
  {
   In.Dt=Step;In.Hour+=Step*In.Rate/3600.;
   Last=StepState(S,P,In);
   if(Any){Any->bBegan|=Last.bBegan;Any->bEscalated|=Last.bEscalated;Any->bBreach|=Last.bBreach;Any->bOver|=Last.bOver;Any->bSettled|=Last.bSettled;Any->bMorning|=Last.bMorning;Any->bDone|=Last.bDone;}
   if(S.Phase==EPhase::Done||(S.Phase==EPhase::Out&&(!bFromOut||S.bSettled)))break;
  }
  return Last;
 };
 {
  // The hook: Dale the morning after, the slip that evening, the case's day, ten to two.
  FState S;
  FOut O=StepState(S,P,At(3,9.));
  const bool bHook=O.bDaleHook&&S.Phase==EPhase::Hook&&S.CaseNight==4;S.bDaleHook=true;
  O=StepState(S,P,At(3,20.));const bool bNoSlipYet=!O.bSlip&&!O.bDaleHook;
  O=StepState(S,P,At(4,9.));const bool bDay=S.Phase==EPhase::Day&&!O.bSlip;
  O=StepState(S,P,At(4,20.));const bool bSlip=O.bSlip&&S.Phase==EPhase::Day;S.bSlip=true;
  O=StepState(S,P,At(4,25.9));const bool bNight=S.Phase==EPhase::Night;
  O=StepState(S,P,At(4,25.9));const bool bCall=O.bNightCall;
  Check(bHook&&bNoSlipYet&&bSlip&&bDay&&bNight&&bCall,TEXT("the hook: Dale the morning after the big night, the case's day, the bearing slip on its own evening (not the night before), the call at ten to two"));
  FState N;O=StepState(N,P,At(3,9.));FIn Dev=At(3,9.);Dev.bLive=false;FState D;O=StepState(D,P,Dev);
  Check(!O.bDaleHook&&D.Phase==EPhase::Hook,TEXT("a developer run has no case of its own"));
  FIn Never=At(5,9.);Never.BigNight=-1;FState X;StepState(X,P,Never);
  Check(X.Phase==EPhase::Done,TEXT("a game without a big night has no week case"));
 }
 {
  // A: never came. Expired at four, the morning after its due at half past eight, then done.
  FState S;S.Phase=EPhase::Night;S.CaseNight=4;
  FOut O=StepState(S,P,At(4,27.));const bool bWaits=S.Phase==EPhase::Night&&!O.bOver;
  O=StepState(S,P,At(4,ArriveBy));const bool bOut=S.Phase==EPhase::Out&&S.Outcome==EOutcome::A&&O.bOver&&S.bSettled;
  O=StepState(S,P,At(4,31.));const bool bNotYet=!O.bMorning;
  O=StepState(S,P,At(5,8.6));const bool bMorning=O.bMorning;
  S.bMorning=true;O=StepState(S,P,At(5,8.7));
  Check(bWaits&&bOut&&bNotYet&&bMorning&&S.Phase==EPhase::Done&&O.bDone,TEXT("A: never came by four, let go, its morning at half past eight the next day, then done"));
  FState Z;Z.Phase=EPhase::Day;Z.CaseNight=4;StepState(Z,P,At(5,9.));
  Check(Z.Phase==EPhase::Out&&Z.Outcome==EOutcome::A,TEXT("A: its whole night slept through is let be too"));
 }
 {
  // Arrival before two waits for two; after two, phase one.
  FState S;S.Phase=EPhase::Night;S.CaseNight=4;S.bNightCall=true;
  FIn In=At(4,25.95);In.bAtFarm=true;FOut O=StepState(S,P,In);const bool bWait=S.Phase==EPhase::Night&&!O.bBegan;
  In.Hour=26.02;O=StepState(S,P,In);
  Check(bWait&&O.bBegan&&S.Phase==EPhase::Phase1,TEXT("phase one starts on arrival after two, not before"));
  // CRIT9-20261109: not on top of something else on the ground; it waits, and the night's window still ends in A.
  FState G;G.Phase=EPhase::Night;G.CaseNight=4;G.bNightCall=true;
  FIn Gi=At(4,26.1);Gi.bAtFarm=true;Gi.bGroundFree=false;const FOut GO=StepState(G,P,Gi);const bool bHeld=!GO.bBegan&&G.Phase==EPhase::Night;
  Gi.bGroundFree=true;const FOut GO2=StepState(G,P,Gi);
  Check(bHeld&&GO2.bBegan&&G.Phase==EPhase::Phase1,TEXT("phase one waits while something else holds the ground, then starts"));
  // CRIT9-20261109: a late arrival that phases one and two could not fit before the night's end is A, not phase one and then B.
  FState Lt;Lt.Phase=EPhase::Night;Lt.CaseNight=4;Lt.bNightCall=true;FIn La=At(4,27.9);La.bAtFarm=true;const FOut LtO=StepState(Lt,P,La);
  Check(!LtO.bBegan&&Lt.Phase==EPhase::Out&&Lt.Outcome==EOutcome::A,TEXT("arriving at 03:54 without time for it is A, never phase one cut off into B"));
  // The night ending with the player still at the door: it wants out (phase two), not B.
  FState En;En.Phase=EPhase::Phase1;En.CaseNight=4;En.Age=10;FIn Ei=At(4,LastHour);Ei.bAtFarm=true;Ei.Dt=1;const FOut EnO=StepState(En,P,Ei);
  Check(EnO.bEscalated&&En.Phase==EPhase::Phase2&&En.Outcome==EOutcome::None,TEXT("phase one at the night's end with the player still there escalates; B is only for leaving"));
 }
 {
  // B: left in phase one. No knock: phase two comes by itself at about five minutes; held while it would run into Story's minute.
  FState S;S.Phase=EPhase::Phase1;S.CaseNight=4;
  FIn In=At(4,26.75);In.bAtFarm=true;FOut Any;
  Run(S,In,P.Phase1Auto-5,1,&Any);const bool bStill=S.Phase==EPhase::Phase1;
  Run(S,In,10,1,&Any);
  Check(bStill&&Any.bEscalated&&S.Phase==EPhase::Phase2,TEXT("without a knock, phase two comes by itself after about five minutes"));
  FState K;K.Phase=EPhase::Phase1;K.CaseNight=4;FIn Kn=At(4,26.75);Kn.bAtFarm=true;Kn.bKnocked=true;FOut KA;
  Run(K,Kn,Phase1Min-2,1,&KA);const bool bMin=K.Phase==EPhase::Phase1;Run(K,Kn,4,1,&KA);
  Check(bMin&&K.Phase==EPhase::Phase2,TEXT("a knock brings phase two, but not before four minutes"));
  FState W;W.Phase=EPhase::Phase1;W.CaseNight=4;W.Age=P.Phase1Auto;FIn Wi=At(4,26.3);Wi.bAtFarm=true;Wi.Dt=1;StepState(W,P,Wi);
  Check(W.Phase==EPhase::Phase1&&W.bHeldForStory,TEXT("ready at 02:18 it waits: nothing comes in Story's minute"));
  FState L;L.Phase=EPhase::Phase1;L.CaseNight=4;FIn Le=At(4,26.75);Le.bAtFarm=false;Le.Dt=1;FOut LO=StepState(L,P,Le);
  Check(L.Phase==EPhase::Out&&L.Outcome==EOutcome::B&&LO.bOver,TEXT("B: left in phase one"));
  FState L2;L2.Phase=EPhase::Phase2;L2.CaseNight=4;LO=StepState(L2,P,Le);
  Check(L2.Outcome==EOutcome::B&&LO.bOver,TEXT("B: drove off before the breach"));
  FState Cap;Cap.Phase=EPhase::Phase1;Cap.CaseNight=4;FIn Ca=At(4,26.1);Ca.bAtFarm=true;Ca.Rate=1;Ca.bKnocked=false;
  // At a slow clock in the minute's shadow: held, until the phase's cap lets it go.
  Ca.Hour=26.34;Run(Cap,Ca,Phase1Cap+5,1,nullptr);
  Check(Cap.Phase==EPhase::Out&&Cap.Outcome==EOutcome::B,TEXT("phase one has a cap: held too long, it is over"));
 }
 {
  // C, D, E: phase two runs into the breach; leaving in the truck is C, on foot D; taken anywhere is E; the search's cap.
  const auto To3=[&](FState& S){S=FState();S.Phase=EPhase::Phase2;S.CaseNight=4;FIn In=At(4,26.8);In.bAtFarm=true;FOut Any;Run(S,In,P.Phase2Len+2,1,&Any);return Any.bBreach&&S.Phase==EPhase::Phase3;};
  FState S;const bool bBreach=To3(S);
  Check(bBreach,TEXT("phase two lasts about a minute and a half, then the breach"));
  FIn Car=At(4,26.9);Car.bAtFarm=false;Car.bDriving=true;Car.Dt=1;StepState(S,P,Car);
  Check(S.Outcome==EOutcome::C&&S.bSettled,TEXT("C: hid and drove away in the truck"));
  To3(S);FIn Foot=Car;Foot.bDriving=false;StepState(S,P,Foot);
  Check(S.Outcome==EOutcome::D,TEXT("D: away on foot"));
  To3(S);FIn Tk=At(4,26.9);Tk.bAtFarm=true;Tk.bTaken=true;Tk.Dt=1;FOut TO=StepState(S,P,Tk);
  FState S1;S1.Phase=EPhase::Phase1;S1.CaseNight=4;FOut T1=StepState(S1,P,Tk);
  Check(S.Outcome==EOutcome::E&&TO.bOver&&S1.Outcome==EOutcome::E&&T1.bOver,TEXT("E: taken, in any phase, stops it at once"));
  To3(S);FIn Stay=At(4,26.9);Stay.bAtFarm=true;FOut Any;Run(S,Stay,Phase3Cap+2,1,&Any);
  const bool bGave=S.Phase==EPhase::Out&&!S.bSettled&&Any.bOver;
  Run(S,Stay,OutCap+2,1,nullptr);
  Check(bGave&&S.Outcome==EOutcome::C&&S.bSettled,TEXT("the search has a cap; staying hidden until the end is C"));
  To3(S);Run(S,Stay,Phase3Cap+2,1,nullptr);StepState(S,P,Foot);
  Check(S.Outcome==EOutcome::D&&S.bSettled,TEXT("after the search, walking off without the truck is D"));
  // The morning of B to E is at SARO.
  FIn Saro=At(5,10.);Saro.bAtSaro=true;FIn Away=At(5,10.);
  const FOut Not=StepState(S,P,Away);const FOut Due=StepState(S,P,Saro);
  Check(!Not.bMorning&&Due.bMorning,TEXT("B to E are written at SARO"));
  FIn Later=At(7,10.);const FOut Gone=StepState(S,P,Later);
  Check(S.Phase==EPhase::Done&&Gone.bDone,TEXT("the wait for its morning has a cap: two nights on it is let go"));
 }
 {
  // The owner lets go: on the ground holds; Out and Done do not.
  FS47WeekCase C(nullptr);
  C.S.Phase=EPhase::Phase2;const bool bHolds=C.HoldsGround()&&C.HoldsFarm();
  C.S.Phase=EPhase::Night;const bool bFarm=!C.HoldsGround()&&C.HoldsFarm();
  C.S.Phase=EPhase::Out;const bool bOut=!C.HoldsGround()&&!C.HoldsFarm();
  C.S.Phase=EPhase::Done;const bool bDone=!C.HoldsGround()&&!C.HoldsFarm();
  Check(bHolds&&bFarm&&bOut&&bDone,TEXT("Busy and Danger hold on the ground only, the farm from ten to two, and both let go when it is over"));
  Check(DangerOf(nullptr).IsEmpty()&&RestBlockOf(nullptr).IsEmpty()&&!Holds(nullptr),TEXT("with no game nothing is held"));
 }
 {
  // Saving: on the ground is saved as the night still waiting; a load never resumes on the ground.
  FState S;S.Phase=EPhase::Phase3;S.CaseNight=5;S.bDaleHook=S.bSlip=S.bNightCall=true;
  FState L=LoadState(SaveState(S));
  FState O;O.Phase=EPhase::Out;O.Outcome=EOutcome::A;O.CaseNight=4;O.bDaleHook=true;
  FState LO=LoadState(SaveState(O));
  FState U;U.Phase=EPhase::Out;U.Outcome=EOutcome::C;U.bSettled=false;U.CaseNight=4;
  FState LU=LoadState(SaveState(U));
  Check(L.Phase==EPhase::Night&&L.CaseNight==5&&L.bNightCall&&LO.Phase==EPhase::Out&&LO.Outcome==EOutcome::A&&LU.Phase==EPhase::Night&&LU.Outcome==EOutcome::None,
   TEXT("saved at SARO: on the ground, or still at the farm after it, is the night waiting again (Continue redoes it); a way out keeps its letter"));
  Check(SaveState(FState())==nullptr,TEXT("a game that has not reached it saves nothing of it"));
 }
 // The evening's slip: the farm inside its sector, whatever the roll and however many branches are mended.
 {
  double SaroX=0,SaroZ=0;S47OpenLand::RoadOf(TEXT("saro"),9,0,SaroX,SaroZ);
  const FVector2D Shed=ShedRoad();bool bIn=true;
  for(int32 Br=0;Br<=5;++Br)for(int32 K=0;K<=20;++K)
  {
   const FS47Places::FBearing B=FS47Places::StationFix(4,3*86400.+20*3600.,Br,SaroX,SaroZ,Shed.X,Shed.Y,float(K)/20.f,TEXT("the Alderete place"));
   bIn&=FS47Places::InSector(B,Shed.X,Shed.Y)&&!FString::FromInt(int32(B.Az)).Contains(TEXT("47"));
  }
  Check(bIn,TEXT("the evening's bearing slip has the farm inside its sector, never on Story's number"));
 }
 // A game that comes in after its night does not make it up.
 {
  FState S;FIn Late;Late.Night=7;Late.Hour=10;Late.BigNight=2;StepState(S,PlanFor(11,2),Late);
  Check(S.Phase==EPhase::Done,TEXT("a save from after its night does not make the case up"));
 }
 // Its record for A is a faint lead in case 8, and not a night record Ward counts.
 {
  const FString Key=S47Week::FindingKey(4);
  Check(FS47FreeformCases::LineForFinding(Key)==FS47FreeformCases::LandLine&&!FS47Happenings::IsNightFinding(Key),TEXT("A's record goes to OUT ON THE LAND and is not counted as a night record"));
 }
 // The rhythm's file has it: a danger on the ground, from day four, after two.
 {
  FS47RhythmData D;FString Why;FString Text;
  if(FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("Director/director.json")))&&FS47Rhythm::Parse(Text,D,Why))
  {
   const FS47RhythmEvent* E=D.Find(TEXT("week_case"));
   Check(E&&E->Level==ES47RhythmLevel::Danger&&E->bGround&&E->FromDay>=4&&E->bEnforce,TEXT("director.json: week_case, a danger on the ground, from day four"));
  }
  else Lines.Add(TEXT("PASS week case: no director.json beside this build to read"));
 }
 // The words: clean of Story's far side, names, brands, Story's numbers and clock, and Dale never says Reyes.
 {
  TArray<FString> All;
  for(const TCHAR* T:S47Week::HookLines)All.Add(T);
  for(const TCHAR* T:S47Week::MorningA)All.Add(T);
  All.Add(S47Week::NightCall);All.Add(S47Week::FindingA);
  // The slips sign "D." and the desk line names him as the set's caption does; Clean keeps "Dale" out of the rest.
  All.Add(S47Week::HookSlip(true));All.Add(S47Week::HookSlip(false));All.Add(FString(S47Week::MorningSlipA));All.Add(FString(S47Week::NightDesk).Replace(TEXT("Dale's voice"),TEXT("his voice")));
  FString Bad;
  for(const FString& T:All)
  {
   if(!S47Hap::Clean(T)||!S47Script::LintText(T).IsEmpty()||T.Contains(TEXT("47"))||T.Contains(TEXT("08:15"))||T.Contains(TEXT("20:15"))||T.Contains(TEXT("8:15"))||T.Contains(TEXT("Reyes")))Bad+=T.Left(48)+TEXT(" / ");
  }
  Check(Bad.IsEmpty(),TEXT("every line is clean: no 47, no 08:15 or 20:15, nothing of Story's far side, no names or brands ")+Bad);
  // What the player is told before it, says where and when, never what (the spoiler limits).
  bool bNoWhat=true;
  for(const TCHAR* T:{S47Week::HookLines[0],S47Week::HookLines[1],S47Week::HookSlipWords,S47Week::NightCall})
  {const FString L=FString(T).ToLower();for(const TCHAR* W:{TEXT("creature"),TEXT("thing in"),TEXT("monster"),TEXT("arm"),TEXT("taken"),TEXT("light inside"),TEXT("breaks"),TEXT("shed door")})bNoWhat&=!L.Contains(W);}
  Check(bNoWhat,TEXT("the hook says where and when, never what is in the shed"));
 }
  // Patch 2: the door, the light and the sounds as the phases have them.
 {
  using ED=FS47FarmShedDoor::EState;using EM=FS47FarmShedDoor::EMood;
  FState D;D.CaseNight=4;
  bool bStates=true;
  D.Phase=EPhase::Hook;bStates&=DoorFor(D,3)==ED::Open;
  for(const EPhase Ph:{EPhase::Day,EPhase::Night,EPhase::Phase1,EPhase::Phase2})
  {
   D.Phase=Ph;bStates&=DoorFor(D,4)==ED::Locked;
  }
  bStates&=MoodFor(EPhase::Phase1)==EM::Rattle&&MoodFor(EPhase::Phase2)==EM::Kicks&&MoodFor(EPhase::Day)==EM::Still&&MoodFor(EPhase::Night)==EM::Still&&MoodFor(EPhase::Phase3)==EM::Still;
  // Through the machine: the breach breaks it, every way out leaves it broken, a week later it stands open again.
  const FPlan Q=PlanFor(11,2);
  FState B;B.Phase=EPhase::Phase2;B.CaseNight=4;FIn In;In.Night=4;In.Hour=26.8;In.BigNight=2;In.Rate=14.67;In.Dt=Q.Phase2Len+1;In.bAtFarm=true;
  StepState(B,Q,In);const bool bBreach=B.Phase==EPhase::Phase3&&DoorFor(B,4)==ED::Broken;
  FState A;A.Phase=EPhase::Night;A.CaseNight=4;FIn Late=In;Late.Hour=ArriveBy;Late.bAtFarm=false;Late.Dt=0;StepState(A,Q,Late);
  const bool bAOut=A.Phase==EPhase::Out&&DoorFor(A,5)==ED::Broken&&DoorFor(A,4+BrokenNights)==ED::Broken&&DoorFor(A,5+BrokenNights)==ED::Open;
  Check(bStates&&bBreach&&bAOut,TEXT("the shed door: open before its day, locked from its day to the breach, rattling in phase one and kicked in two, broken by the breach and by every way out, the rest of the week"));
  // Broken is kept by the save with its way out; out there nothing is, and Continue finds it locked again.
  FState C;C.Phase=EPhase::Out;C.Outcome=EOutcome::C;C.CaseNight=4;C.bDoorBroken=true;C.DoorNight=4;
  const FState LC=LoadState(SaveState(C));
  FState G;G.Phase=EPhase::Phase3;G.CaseNight=4;G.bDoorBroken=true;G.DoorNight=4;
  const FState LG=LoadState(SaveState(G));
  FState Dn;Dn.Phase=EPhase::Done;Dn.Outcome=EOutcome::A;Dn.CaseNight=4;Dn.bDoorBroken=true;Dn.DoorNight=4;
  const FState LD=LoadState(SaveState(Dn));
  Check(LC.bDoorBroken&&LC.DoorNight==4&&DoorFor(LC,5)==ED::Broken&&!LG.bDoorBroken&&DoorFor(LG,4)==ED::Locked&&LD.bDoorBroken&&DoorFor(LD,6)==ED::Broken,
   TEXT("the broken door is saved as a state with its way out; broken on the ground is not, and the night comes back locked"));
  // The answer to a knock: the plan's rhythm from the seed, answered in the same rhythm.
  bool bRhythm=true;
  for(int32 Sd=1;Sd<=200;++Sd)
  {
   const FPlan R=PlanFor(Sd,2),R2=PlanFor(Sd,2);
   const TArray<double> Knock=FS47FarmShedDoor::Pattern(R.Rhythm);
   bRhythm&=R.Rhythm==R2.Rhythm&&FS47FarmShedDoor::AnswerFor(Knock)==Knock&&FS47FarmShedDoor::AnswerFor(FS47FarmShedDoor::Pattern(R2.Rhythm))==Knock;
  }
  Check(bRhythm,TEXT("a knock in phase one is answered in the same rhythm, the plan's rhythm from the seed, the same every time"));
  // The light is out outside phases two and three, whatever the age.
  bool bDark=true;
  for(const EPhase Ph:{EPhase::Hook,EPhase::Day,EPhase::Night,EPhase::Phase1,EPhase::Out,EPhase::Done})for(double Age=0;Age<1500;Age+=11)bDark&=!FS47ShedGlow::LevelFor(Ph,Age).Lit();
  Check(bDark&&FS47ShedGlow::LevelFor(EPhase::Phase2,1).Lit()&&FS47ShedGlow::LevelFor(EPhase::Phase3,1).Lit(),TEXT("the light from inside is out but in phases two and three"));
 }
 // Patch 3: the truck on the yard and the cab.
 {
  const auto Dead=[](FCab& C,EPhase Ph){C=FCab();FCabIn In;In.Phase=Ph;In.bInCab=true;In.bAtYard=true;In.SearcherDist=30;return StepCab(C,In).bStallNow&&C.bStalled;};
  FCab C;
  FCabIn Night;Night.Phase=EPhase::Night;Night.bInCab=true;Night.bAtYard=true;
  const bool bNotBefore=!StepCab(C,Night).bStallNow&&!C.bStalled;
  const bool bDies=Dead(C,EPhase::Phase1);
  FCabIn Again;Again.Phase=EPhase::Phase1;Again.bInCab=true;Again.bAtYard=true;Again.Dt=1;
  const bool bOnce=!StepCab(C,Again).bStallNow;
  Check(bNotBefore&&bDies&&bOnce,TEXT("the truck dies on the yard once, on the ground only"));
  // Always after five minutes: turning the key every few seconds (never quiet), it near but never at the cab, in every phase.
  bool bFuse=true,bNotSooner=true;
  for(const EPhase Ph:{EPhase::Phase1,EPhase::Phase2,EPhase::Phase3})for(double Dist:{13.,25.,39.,80.})for(double Every:{2.,3.5,7.})
  {
   FCab K;Dead(K,Ph);
   FCabIn In;In.Phase=Ph;In.bInCab=true;In.SearcherDist=Dist;In.Dt=.5;
   double Next=Every;bool bCaught=false;
   for(double T=.5;T<CatchFuse+30&&!bCaught;T+=.5)
   {
    In.bTurned=T>=Next;if(In.bTurned)Next=T+Every;
    const FCabOut O=StepCab(K,In);
    if(O.bCatch){bCaught=true;bNotSooner&=K.StallAge>=CatchFuse||(Ph!=EPhase::Phase2&&Dist>CatchFar);}
   }
   bFuse&=bCaught;
  }
  Check(bFuse&&bNotSooner,TEXT("the truck always catches on a turn after five minutes dead, whatever else; never sooner while noisy and close"));
  // Not while it is at the cab: the key held, no turn counts, not even after the fuse; and nobody is taken in the cab in week one.
  bool bNever=true,bHeld=true,bSafe=true;
  for(const EPhase Ph:{EPhase::Phase1,EPhase::Phase2,EPhase::Phase3})
  {
   FCab K;Dead(K,Ph);
   FCabIn In;In.Phase=Ph;In.bInCab=true;In.bSearcherNear=true;In.SearcherDist=3;In.Dt=1;In.Week=1;
   for(double T=0;T<CatchFuse*2;T+=1)
   {
    In.bTurned=FMath::Fmod(T,5.)==0;In.bSearcherNear=FMath::Fmod(T,7.)!=3;In.SearcherDist=In.bSearcherNear?3:8; // within NearCab either way
    const FCabOut O=StepCab(K,In);
    bNever&=!O.bCatch&&!K.bCaught;bHeld&=O.bHoldKey&&O.bShake&&O.bSweep;bSafe&=!O.bTake;
   }
  }
  FCab K2;Dead(K2,EPhase::Phase3);FCabIn W2;W2.Phase=EPhase::Phase3;W2.bInCab=true;W2.bSearcherNear=true;W2.Dt=1;W2.Week=2;
  Check(bNever&&bHeld,TEXT("the truck never catches while it is at the cab, the key held, the truck rocked and the light through the glass"));
  Check(bSafe&&!MayTakeInCab(1)&&MayTakeInCab(2)&&StepCab(K2,W2).bTake,TEXT("nobody is taken in the cab in week one (from week two it may cost)"));
  // The rule: far and quiet catches in phases one and three; nineteen seconds is not quiet enough; phase two only cranks, and its
  // second turn stops the kicks.
  const auto TurnAfter=[&Dead](EPhase Ph,double Dist,double Quiet){FCab K;Dead(K,Ph);FCabIn In;In.Phase=Ph;In.bInCab=true;In.SearcherDist=Dist;In.Dt=Quiet;StepCab(K,In);In.Dt=0;In.bTurned=true;return StepCab(K,In).bCatch;};
  Check(TurnAfter(EPhase::Phase3,41,20.5)&&TurnAfter(EPhase::Phase1,60,21)&&!TurnAfter(EPhase::Phase3,41,19)&&!TurnAfter(EPhase::Phase3,39,60)&&!TurnAfter(EPhase::Phase2,80,60),
   TEXT("it catches when it is over 40 m off and she has been quiet 20 s; in phase two it only cranks"));
  FCab L;Dead(L,EPhase::Phase2);FCabIn P2;P2.Phase=EPhase::Phase2;P2.bInCab=true;P2.SearcherDist=30;P2.bTurned=true;P2.Dt=1;
  const FCabOut L1=StepCab(L,P2),L2=StepCab(L,P2);
  Check(L1.bHeard&&!L1.bListen&&L2.bHeard&&L2.bListen&&L.Listen>0,TEXT("every turn is heard; in phase two the second stops the kicks: something listens"));
  // The help text only when stuck.
  FCab St;Dead(St,EPhase::Phase3);FCabIn S3;S3.Phase=EPhase::Phase3;S3.bInCab=true;S3.SearcherDist=30;S3.Dt=1;
  bool bEarly=false;for(int32 I=0;I<20;++I)bEarly|=StepCab(St,S3).bStuck;
  int32 Said=0;S3.bTurned=true;for(int32 I=0;I<6;++I)Said+=StepCab(St,S3).bStuck?1:0;
  Check(!bEarly&&Said==1,TEXT("\"Don't flood it\" comes once, and only when she is stuck turning the key"));
  // The reserve: shut in until the breach; after it, to the truck after a turn, a while, then off.
  bool bN=false;double D=0;bool bRes=true;
  ReserveSearcher(EPhase::Phase2,30,0,bN,D);bRes&=!bN&&D==30;
  ReserveSearcher(EPhase::Phase3,30,ReserveCome+1,bN,D);bRes&=bN;
  ReserveSearcher(EPhase::Phase3,30,ReserveCome+ReserveStay+1,bN,D);bRes&=!bN&&D>CatchFar;
  ReserveSearcher(EPhase::Out,30,0,bN,D);bRes&=!bN&&D>CatchFar;
  Check(bRes,TEXT("the reserve searcher (until patch 4): in the shed, then to the last turn of the key, then gone"));
 }
 // Patch 3: the thoughts, through the one queue, one at a time, behind a voice; in order.
 {
  FS47MessageQueue Q;
  using FQ=FS47MessageQueue;
  Q.Push(TEXT("WALKIE 1, DALE: Listen."),3.f,FQ::EPri::Dale);
  Q.Push(S47Thought::Line(ThoughtLocked()),4.f,FQ::EPri::Radio);
  Q.Push(S47Thought::Line(ThoughtLockedIn()),4.5f,FQ::EPri::Radio);
  int32 Most=0;TArray<FString> Seen;double Now=0;
  for(int32 I=0;I<600;++I)
  {
   Now+=.05;Q.Step(Now,.05);
   const FQ::FItem* Sp=Q.Showing(FQ::ELane::Speech);const FQ::FItem* Hi=Q.Showing(FQ::ELane::Hint);
   Most=FMath::Max(Most,(Sp?1:0)+(Hi&&S47Thought::Is(Hi->Text)?1:0));
   if(Sp&&(Seen.IsEmpty()||Seen.Last()!=Sp->Text))Seen.Add(Sp->Text);
  }
  Check(Most==1&&Seen.Num()==3&&Seen[0].Contains(TEXT("DALE"))&&Seen[1]==S47Thought::Line(ThoughtLocked())&&Seen[2]==S47Thought::Line(ThoughtLockedIn())&&FQ::Classify(Seen[1])==FQ::EPri::Hint&&S47Thought::Words(Seen[1])==ThoughtLocked(),
   TEXT("the thoughts go through the one queue, one at a time, after a voice, in order"));
 }
 // Patch 3: Dale's morning follows the way out; every new line is clean.
 {
  bool bEach=true;TSet<FString> Firsts;
  for(const EOutcome O:{EOutcome::A,EOutcome::B,EOutcome::C,EOutcome::D,EOutcome::E}){const TArray<FString> L=MorningLines(O);bEach&=L.Num()>=2&&L.Last().EndsWith(TEXT("Out."));Firsts.Add(L.IsEmpty()?FString():L[0]);}
  Check(bEach&&Firsts.Num()==5&&MorningLines(EOutcome::None).IsEmpty()&&MorningLines(EOutcome::A)[0]==S47Week::MorningA[0],TEXT("Dale's morning after follows the way out: A to E each its own"));
  TArray<FString> All=HelpLines();
  for(const EOutcome O:{EOutcome::B,EOutcome::C,EOutcome::D,EOutcome::E}){All.Append(MorningLines(O));All.Add(S47Week::SlipOf(MorningLines(O)));}
  All.Add(S47Week::HailFirst);All.Add(S47Week::HailAgain);All.Add(S47Week::HailGiveUp);
  for(const TCHAR* T:{ThoughtLocked(),ThoughtLockedIn(),ThoughtStuck(),ThoughtHeld()})All.Add(T);
  FString Bad;
  for(const FString& T:All)
   if(!S47Hap::Clean(T)||!S47Script::LintText(T).IsEmpty()||T.Contains(TEXT("47"))||T.Contains(TEXT("8:15"))||T.Contains(TEXT("20:15"))||T.Contains(TEXT("Reyes"))||S47Hap::Concluding(T))Bad+=T.Left(48)+TEXT(" / ");
  Check(Bad.IsEmpty(),TEXT("patch 3's words are clean: Dale's hail, help and mornings, the slips, the thoughts ")+Bad);
  // What Dale says during it helps, but does not say what is in there.
  bool bNoWhat=true;
  for(const FString& T:HelpLines()){const FString L=T.ToLower();for(const TCHAR* Wd:{TEXT("creature"),TEXT("monster"),TEXT("arm"),TEXT("taken"),TEXT("alien"),TEXT("tall")})bNoWhat&=!FS47WeekCaseLocalWord(L,Wd);}
  Check(bNoWhat,TEXT("his help gives the bearing, the cover and the engine, never what it is"));
 }
 OK&=FS47FarmShedDoor::SelfTest(Lines);
 OK&=FS47ShedGlow::SelfTest(Lines);
 OK&=S47ShedSound::SelfTest(Lines);
 return OK;
}
