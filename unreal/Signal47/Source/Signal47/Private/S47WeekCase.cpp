#include "S47WeekCase.h"
#include "S47Drive.h"
#include "S47FarmBuildings.h"
#include "S47FarmWorld.h"
#include "S47FieldRadio.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47FreeformDirector.h"
#include "S47Happenings.h"
#include "S47Main.h"
#include "S47NightRhythm.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47Rhythm.h"
#include "S47Script.h"
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

// WEEKLY-CASE-20261109, patch 1: the owner, the gate and way out A. See S47WeekCase.h. Dale's lines go over the walkie as the big
// night's do (S47BigNight.cpp, StepBigAfter): "DALE: " and his words, one at a time; with no set to hear them, a slip by the phone at
// SARO. Never 47, never 08:15 or 20:15, nothing of Story's far side (S47Hap::Clean and S47Script::LintText, the self-test).
namespace S47Hap
{
 bool Clean(const FString& Text);
 FString ClockText(double Clock);
}
namespace S47Week
{
 constexpr uint32 SaltAuto=0x57454b31u,SaltLen=0x57454b32u,SaltRhythm=0x57454b33u;
 // Which of the farm's buildings is the feed shed (S47FarmBuildings::Sites: barn, house, machine shed, feed shed).
 constexpr int32 ShedSite=3;
 const TCHAR* const Farm=TEXT("the Alderete place");
 // The morning after the big night: one more thing, after his own. Where and when, never what.
 const TCHAR* const HookLines[]={
  TEXT("One more thing. Since the dishes went over by themselves, something on the bearing past the Alderete place is chewing my signal."),
  TEXT("Every night, a little after two, for a few minutes. Where and when I can tell you. What, I cannot. Out.")};
 const TCHAR* const HookSlip=TEXT("A second slip under the first, the same hard hand: \"Something on the bearing past the Alderete place chews my signal every night a little after two. Where and when. Not what. D.\"");
 const TCHAR* const NightCall=TEXT("It's there again. Same bearing.");
 const TCHAR* const NightDesk=TEXT("WALKIE / From the set on the desk, Dale's voice, flat: \"It's there again. Same bearing.\"");
 // Way out A: the morning after, he has heard. Sarcastic; no pay, nothing for Ward.
 const TCHAR* const MorningA[]={
  TEXT("Good morning. The Alderete feed shed. The door is lying in the yard in pieces, and it broke outward. From the inside."),
  TEXT("The padlock is still on its chain, still locked. And the floor in there has gone hard and shiny, like a pot out of the kiln."),
  TEXT("But you were busy, I am sure. Nobody pays for a door we did not look at. Out.")};
 const TCHAR* const MorningSlipA=TEXT("A slip by the phone, in a hard square hand: \"Alderete feed shed. Door broken OUTWARD. Padlock still locked. Floor gone like glaze. You were busy, I am sure. D.\"");
 const TCHAR* const FindingA=TEXT("The feed shed at the Alderete place, the morning after, as it came over the walkie: the door broken outward from the inside, the padlock still locked on its chain, and the floor inside gone hard and glossy, like glaze. Nobody from the station was there.");
 FString FindingKey(int32 Night){return FString::Printf(TEXT("weekcase_shed_a_d%d"),Night);}
 FString Dale(const FString& Text){return FString(TEXT("DALE"))+TEXT(": ")+Text;}
 // The night's pace (F10's natural night runs its minutes at speed), as the big night counts its own seconds.
 double Pace(const US47Main* M){return M&&M->Freeform&&M->Freeform->Director?FMath::Max(1.,M->Freeform->Director->TestPaceNow()):1.;}
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
 const auto Over=[&](EOutcome Out,bool bSettled,const TCHAR* Why){S.Outcome=Out;S.bSettled=bSettled;O.bOver=true;O.bSettled=bSettled;S.bHeldForStory=false;To(EPhase::Out,Why);};
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
  if(In.Night==After&&In.Hour>=SlipFrom&&In.Hour<SlipUntil&&!S.bSlip)O.bSlip=true;
  if(In.Night>=S.CaseNight)To(EPhase::Day,TEXT("the case's day"));
  break;
 }
 case EPhase::Day:
  if(!In.bLive&&!S.bForced)break;
  if(In.Night>S.CaseNight){Over(EOutcome::A,true,TEXT("its night slept through"));break;}
  if(In.Night==S.CaseNight&&In.Hour>=NightCallAt)To(EPhase::Night,TEXT("ten to two"));
  break;
 case EPhase::Night:
 {
  if(!In.bLive&&!S.bForced)break;
  if(In.Night>S.CaseNight||(!S.bForced&&In.Hour>=ArriveBy)){Over(EOutcome::A,true,TEXT("never came: let be"));break;}
  if(!S.bForced&&!S.bNightCall&&In.Hour>=NightCallAt&&In.Hour<NightCallUntil)O.bNightCall=true;
  if(In.bAtFarm&&!In.bTaken&&(S.bForced||In.Hour>=ArriveFrom)){S.CaseAge=0;O.bBegan=true;To(EPhase::Phase1,TEXT("came to the farm"));}
  break;
 }
 case EPhase::Phase1:
 {
  S.Age+=Dt;S.CaseAge+=Dt;
  if(In.bTaken){Over(EOutcome::E,true,TEXT("taken"));break;}
  if(!In.bAtFarm){Over(EOutcome::B,true,TEXT("left before the breach"));break;}
  // The caps: the phase's, the whole case's and the night's.
  if(S.Age>=Phase1Cap||S.CaseAge>=CaseCap||(!S.bForced&&In.Hour>=LastHour)){Over(EOutcome::B,true,TEXT("cap: phase one ran out"));break;}
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
  if(S.Age>=FMath::Clamp(P.Phase2Len,30.,150.)||S.CaseAge>=CaseCap){O.bBreach=true;To(EPhase::Phase3,TEXT("the breach"));}
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
 S.bSettled=true;
 return S;
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
 bDevBefore=false;DaleLines.Reset();LastClock=-1;TryIn=0;Paced=0;HookSince=-1;SaidIn=0;
 if(Saved.IsValid())UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: loaded %s"),*StateText());
}
void FS47WeekCase::Stop()
{
 Clean(TEXT("stop"));
}
S47J::FObj FS47WeekCase::Save() const{return SaveState(S);}
FString FS47WeekCase::StateText() const
{
 return FString::Printf(TEXT("%s, night %d, outcome %s%s%s"),PhaseName(S.Phase),S.CaseNight,OutcomeName(S.Outcome),S.bForced?TEXT(", F10"):TEXT(""),S.bHeldForStory?TEXT(", waiting out Story's minute"):TEXT(""));
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
 In.bAtSaro=FS47FieldRadio::InSaroBuilding(M);
 In.bTaken=F->Taken&&(F->Taken->Stage()==FS47Taken::EStage::Lift||F->Taken->Stage()==FS47Taken::EStage::Away);
 return In;
}

void FS47WeekCase::Step(double Dt)
{
 if(!M||!M->IsFreeform()||!M->Freeform||!M->Freeform->Active()||S.Phase==EPhase::Done)return;
 Paced+=Dt;
 const FPlan P=PlanFor(M->Freeform->Seed(),S.CaseNight>0&&S.bForced?S.CaseNight-2:(M->Freeform->Happenings?M->Freeform->Happenings->BigHappened():0));
 const EPhase Was=S.Phase;
 const FIn In=ReadIn(Dt);
 const FOut O=StepState(S,P,In);
 if(S.Phase!=Was)UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: %s -> %s (%s) at %s, night %d"),PhaseName(Was),PhaseName(S.Phase),*O.Why,*S47Hap::ClockText(M->Freeform->Clock()),In.Night);
 Apply(O);
 StepDale();
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
  HoldScripts(true);
  UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: phase one at the shed, %s"),*S47Hap::ClockText(F->Clock()));
 }
 if(O.bEscalated)UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: phase two at %s"),*S47Hap::ClockText(F->Clock()));
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
bool FS47WeekCase::Dale(const TArray<FString>& Lines)
{
 if(!M->FieldRadio.IsValid()||!M->FieldRadio->Owned()||!DaleLines.IsEmpty())return false;
 for(const FString& L:Lines)DaleLines.Add(S47Week::Dale(L));
 SaidIn=Paced+.5;DaleUntil=Paced+240;
 return true;
}
void FS47WeekCase::StepDale()
{
 // Not heard in four minutes (the set off his channel all that time): he gives up on the rest.
 if(!DaleLines.IsEmpty()&&Paced>DaleUntil){UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: %d of Dale's lines not heard, let go"),DaleLines.Num());DaleLines.Reset();}
 if(DaleLines.IsEmpty()||Paced<SaidIn||!M->FieldRadio.IsValid()||M->ModalOpen())return;
 if(M->FieldRadio->Message(DaleLines[0])){DaleLines.RemoveAt(0);SaidIn=Paced+7.5;}
 else SaidIn=Paced+3;
}
void FS47WeekCase::SayDaleHook()
{
 TryIn=Paced+2;
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
 if(Dale(Lines)){S.bDaleHook=true;UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: the hook, Dale on the walkie"));M->RequestAutosave();return;}
 // No set, or it is off his channel: the slip by the phone, read at the station indoors.
 if(FS47FieldRadio::InSaroBuilding(M)&&M->UI)
 {
  M->UI->Toast(S47Week::HookSlip,10);
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
 if(M->ModalOpen())return;
 if(Dale({S47Week::NightCall})){S.bNightCall=true;UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: ten to two, Dale: same bearing"));return;}
 if(FS47FieldRadio::InSaroBuilding(M)&&M->UI){M->UI->Toast(S47Week::NightDesk,7);S.bNightCall=true;UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: ten to two, Dale from the desk set"));}
}
void FS47WeekCase::Morning()
{
 TryIn=Paced+3;
 auto* F=M->Freeform.Get();
 if(M->ModalOpen())return;
 if(S.Outcome==EOutcome::A)
 {
  TArray<FString> Lines;for(const TCHAR* L:S47Week::MorningA)Lines.Add(L);
  bool bSaid=Dale(Lines);
  if(!bSaid&&FS47FieldRadio::InSaroBuilding(M)&&M->UI){M->UI->Toast(S47Week::MorningSlipA,11);bSaid=true;}
  if(!bSaid)return;
  // A faint line in case 8 (OUT ON THE LAND): a lead, no pay, and not a night record Ward counts (FS47Happenings::IsNightFinding).
  const FString Key=S47Week::FindingKey(S.CaseNight);
  if(!F->HasFinding(Key))F->RecordEvidence(Key,S47Week::FindingA);
 }
 // B to E: their records are patch 5's; here only the morning is marked, with the player back at SARO.
 S.bMorning=true;
 UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: the morning after, way out %s"),OutcomeName(S.Outcome));
 M->RequestAutosave();
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
 DaleLines.Reset();
 S=bDevBefore?DevBefore:FState();
 if(S.Phase==EPhase::Phase1||S.Phase==EPhase::Phase2||S.Phase==EPhase::Phase3)S.Phase=EPhase::Night;
 bDevBefore=false;
 UE_LOG(LogS47WeekCase,Display,TEXT("S47: week case: F10 stop: %s"),*StateText());
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
  O=StepState(S,P,At(3,20.));const bool bSlip=O.bSlip&&!O.bDaleHook;S.bSlip=true;
  O=StepState(S,P,At(4,9.));const bool bDay=S.Phase==EPhase::Day;
  O=StepState(S,P,At(4,25.9));const bool bNight=S.Phase==EPhase::Night;
  O=StepState(S,P,At(4,25.9));const bool bCall=O.bNightCall;
  Check(bHook&&bSlip&&bDay&&bNight&&bCall,TEXT("the hook: Dale the morning after the big night, the bearing slip that evening, the case's day, the call at ten to two"));
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
  All.Add(FString(S47Week::HookSlip));All.Add(FString(S47Week::MorningSlipA));All.Add(FString(S47Week::NightDesk).Replace(TEXT("Dale's voice"),TEXT("his voice")));
  FString Bad;
  for(const FString& T:All)
  {
   if(!S47Hap::Clean(T)||!S47Script::LintText(T).IsEmpty()||T.Contains(TEXT("47"))||T.Contains(TEXT("08:15"))||T.Contains(TEXT("20:15"))||T.Contains(TEXT("8:15"))||T.Contains(TEXT("Reyes")))Bad+=T.Left(48)+TEXT(" / ");
  }
  Check(Bad.IsEmpty(),TEXT("every line is clean: no 47, no 08:15 or 20:15, nothing of Story's far side, no names or brands ")+Bad);
  // What the player is told before it, says where and when, never what (the spoiler limits).
  bool bNoWhat=true;
  for(const TCHAR* T:{S47Week::HookLines[0],S47Week::HookLines[1],S47Week::HookSlip,S47Week::NightCall})
  {const FString L=FString(T).ToLower();for(const TCHAR* W:{TEXT("creature"),TEXT("thing in"),TEXT("monster"),TEXT("arm"),TEXT("taken"),TEXT("light inside"),TEXT("breaks"),TEXT("shed door")})bNoWhat&=!L.Contains(W);}
  Check(bNoWhat,TEXT("the hook says where and when, never what is in the shed"));
 }
 return OK;
}
