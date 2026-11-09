#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47FarmShedDoor.h"
class US47Main;
class AS47Walker;
struct FS47View;
class FS47ShedGlow;

// WEEKLY-CASE-20261109, week one, "the shed" (Docs/Research/Gameplay/UKESAKER_2026-11-09.md, patch 1 of 6: the owner, the gate and way
// out A). Freeform only. One authored case a week; the first comes two nights after the first big night (FS47Happenings::BigPlan: the
// second or third night, so this is the fourth or fifth), after two in the morning, at the Alderete place's feed shed. Tom has not yet
// answered the document's three questions; until he does, its defaults hold: nights four to five, a shadow and fingers before a whole
// arm, and the truck's cab is safe in week one.
//
//   Hook    the morning after the big night, from half past eight, Dale on the walkie: something on the farm's bearing has been
//           chewing his signal (his slip by the phone without a set); the evening after, the night's one bearing slip points at the
//           farm (FS47Places::PointAt, the Director's one pointing-out a night).
//   Day     the case's own day, until ten to two that night (the shed by daylight is patch 2).
//   Night   from 01:50: Dale, "It's there again. Same bearing." The case waits for the player to come to the farm after two.
//   Phase1  on arrival after two, at least four minutes: look round, knock. Without a knock the next phase comes by itself.
//   Phase2  it wants out: about a minute and a half, the light from inside and the kicks.
//   Phase3  the breach and the search: at most three minutes (the searcher is patch 4).
//   Out     the case's ground is let go; what it leaves waits for the morning, or for SARO.
//   Done    over for this game.
//
// The phases run from the player's arrival, not from the clock. Story's dark minute (02:20 to 02:40) only gates the escalation: phases
// two and three start only when they would end before 02:20 or start at 02:41 or later (MayEscalate). Every wait has a cap, and so has
// the whole case. Ways out (only A has what it leaves in this patch): A let it be (never came), B drove before the breach, C hid and
// drove, D on foot, E taken. A: the morning after, a faint line in case 8 (the door broken out from inside, the lock still locked, a
// glazed floor), Dale sarcastic; no pay and nothing Ward counts.
//
// One owner: whoever asks about danger sees it (S47SaroSave::Danger, FS47Rhythm::ReadNow's Busy, the big night, FS47FarmSite's visits,
// the Director's starts and so the glimpses, the Director's RestBlock). Nothing is saved out there: the phase is saved at SARO, with the
// phases on the ground saved as the night still waiting, so that Continue goes back to the log filed when the player left SARO
// (S47SaroSave::QuitAwayText), and the seed makes the replay the same.
class FS47WeekCase
{
public:
 enum class EPhase : uint8 { Hook, Day, Night, Phase1, Phase2, Phase3, Out, Done };
 enum class EOutcome : uint8 { None, A, B, C, D, E };
 static const TCHAR* PhaseName(EPhase P);
 static const TCHAR* OutcomeName(EOutcome O);

 // ---- the pure machine (no world: the self-test steps it) ----
 // The night of the hour (FS47Happenings::NightHour: 8 to 32, past midnight 24 and over).
 static constexpr double DaleHookAt=8.5,DaleHookUntil=18.,SlipFrom=19.,SlipUntil=25.5,NightCallAt=25.+50./60.,NightCallUntil=26.5;
 static constexpr double ArriveFrom=26.,ArriveBy=28.,LastHour=29.;
 // Story's dark minute, and the first minute after it the escalation may start in.
 static constexpr double StoryFrom=26.+20./60.,StoryTo=26.+40./60.,AfterStory=26.+41./60.;
 // Seconds of play (at the night's pace), from the arrival.
 static constexpr double Phase1Min=240.,Phase1Cap=780.,Phase3Cap=180.,OutCap=600.,CaseCap=1500.;
 // How long phases two and three may take at most, together, for the gate.
 static constexpr double EscalateMax=Phase3Cap+105.;
 struct FPlan
 {
  int32 Night=0;          // the case's night, 0 none
  double Phase1Auto=300;  // with no knock, phase two comes by itself after this (about five minutes)
  double Phase2Len=90;    // it wants out (about a minute and a half)
  int32 Rhythm=0;         // the answer's rhythm to a knock, from the seed (patch 2 plays it)
 };
 // From the seed and the big night's night: two nights after it; none when the big night has not come or never will.
 static FPlan PlanFor(int32 Seed,int32 BigNight);
 static int32 CaseNightFor(int32 BigNight){return BigNight>0?BigNight+2:0;}
 // Whether phases two and three may start at this hour of the night: over before 02:20, or at 02:41 or later. Rate: game seconds a
 // second of play (the night's clock runs about fifteen to one).
 static bool MayEscalate(double Hour,double Rate);
 struct FState
 {
  EPhase Phase=EPhase::Hook;
  EOutcome Outcome=EOutcome::None;
  int32 CaseNight=0;
  double Age=0,CaseAge=0;  // seconds of play in this phase, and since the arrival
  bool bDaleHook=false,bSlip=false,bNightCall=false,bMorning=false; // what has been said or given (the world's part done)
  bool bSettled=true;      // Out: the outcome is final (false while the player is still at the farm after the search)
  bool bForced=false;      // F10's: the hours do not hold it back
  bool bHeldForStory=false;
  // Patch 2: the shed's door burst out (on the ground from the breach; on every way out, since it gets out whether she saw it or
  // not). Saved only with a way out, so that Continue from out there finds it locked again; kept BrokenNights nights.
  bool bDoorBroken=false;
  int32 DoorNight=0;
 };
 static constexpr int32 BrokenNights=6;
 // Patch 2: the door as the case has it now (open before its day, locked from its day to the breach, broken the rest of the week),
 // and what is going on behind it (rattling in phase one, kicks in phase two).
 static FS47FarmShedDoor::EState DoorFor(const FState& S,int32 Night);
 static FS47FarmShedDoor::EMood MoodFor(EPhase P);
 struct FIn
 {
  int32 Night=0;        // FS47FreeformSession::NightOf
  double Hour=0;        // FS47Happenings::NightHour
  double Dt=0;          // seconds of play since the last step (0 while held)
  double Rate=1;        // game seconds a second of play
  int32 BigNight=0;     // FS47Happenings::BigHappened: 0 not yet, -1 never in this game
  bool bLive=true;      // the night's own rules run (a developer run has no case of its own)
  bool bAtFarm=false,bAtSaro=false,bDriving=false,bTaken=false,bKnocked=false;
 };
 struct FOut
 {
  bool bDaleHook=false,bSlip=false,bNightCall=false; // due now and not done yet: the world says them, then marks them done
  bool bBegan=false,bEscalated=false,bBreach=false;  // phase one, two or three began this step
  bool bOver=false;     // the case's ground is let go this step: clean up, and the rhythm's rest
  bool bSettled=false;  // the outcome became final this step
  bool bMorning=false;  // the outcome's morning is due and not done yet
  bool bDone=false;
  FString Why;
 };
 static FOut StepState(FState& S,const FPlan& P,const FIn& In);
 // The phase a save writes: what is on the ground is the night still waiting; Out at the farm too.
 static EPhase SavedPhase(const FState& S);
 static S47J::FObj SaveState(const FState& S);
 static FState LoadState(const S47J::FObj& O);

 // ---- the owner, asked by everyone else ----
 // On the ground now (phases one to three): nothing else starts, no saving, no rest.
 bool HoldsGround() const{return S.Phase==EPhase::Phase1||S.Phase==EPhase::Phase2||S.Phase==EPhase::Phase3;}
 // The farm is the case's tonight (from the night call to the end on the ground): no ordinary visit there.
 bool HoldsFarm() const{return S.Phase==EPhase::Night||HoldsGround();}
 static bool Holds(const US47Main* M);
 static bool HoldsFarm(const US47Main* M);
 // S47SaroSave::Danger's word, and the Director's RestBlock's. Empty when the case does not hold the ground.
 static FString DangerOf(const US47Main* M);
 static FString RestBlockOf(const US47Main* M);

 explicit FS47WeekCase(US47Main* Main):M(Main){}
 ~FS47WeekCase();
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 EPhase Phase() const{return S.Phase;}
 EOutcome Outcome() const{return S.Outcome;}
 int32 CaseNight() const{return S.CaseNight;}
 FString StateText() const;
 double PhaseAge() const{return S.Age;}
 // Patch 2: the shed's door and its light (drawn whenever Freeform runs; the case drives them).
 const FS47FarmShedDoor* DoorNow() const{return DoorObj.Get();}
 const FS47ShedGlow* GlowNow() const{return GlowObj.Get();}
 // Where the shed's door is, in the area's terms (SARO's).
 static FVector ShedDoor();
 static FVector2D ShedRoad();
 // Within this of the shed is at the farm.
 static constexpr double FarmReach=60.;

 // ---- what later patches hold, and every way out gives back ----
 // The truck held dead (Trouble's Release on every way out).
 void HoldStall();
 // The view held (the walker's CameraOverride, given back on every way out, as FS47ArrayShot does).
 void HookCamera(TFunction<bool(double,FS47View&)> Frame);
 // Whatever else is drawn for it (the figures): put away on every way out.
 void OnClean(TFunction<void()> Fn){Cleanups.Add(MoveTemp(Fn));}
 void Clean(const TCHAR* Why);

 // ---- F10 (S47WeekCaseDev.cpp) ----
 // The case tonight, now, past its hours: the night waiting for the player at the farm.
 bool DevStart(FString& Why);
 // Way out A, settled, with its morning due at the next morning.
 bool DevForceA(FString& Why);
 // Over at once and cleaned up; the case as it was before F10 touched it.
 void DevStop();
 // On the ground already (DevStart and at the shed): to this phase now, its age from zero (patch 2: phase two's light and kicks,
 // phase three's breach).
 bool DevPhase(EPhase P,FString& Why);
 // In phase one: a knock at the door as if E were pressed with "Knock".
 bool DevKnock(FString& Why);
 bool DevTouched() const{return bDevBefore;}
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 FState S;
 FState DevBefore;bool bDevBefore=false;
 TSharedPtr<uint8> Alive=MakeShared<uint8>(0);
 double LastClock=-1,Rate=14.67,TryIn=0,Paced=0,HookSince=-1,SaidIn=0,DaleUntil=0;
 TArray<FString> DaleLines;
 bool bStall=false;
 TWeakObjectPtr<AS47Walker> Hooked;
 TFunction<bool(double,const FVector2D&,const FVector2D&,FS47View&)> Before;
 TArray<TFunction<void()>> Cleanups;
 bool bRhythmBegan=false,bScriptsHeld=false;
 TSharedPtr<FS47FarmShedDoor> DoorObj;
 TSharedPtr<FS47ShedGlow> GlowObj;
 bool bShedFresh=true;
 void StepShed(double Dt,const FPlan& P);
 FIn ReadIn(double Dt);
 void Apply(const FOut& O);
 void SayDaleHook();
 void GiveSlip();
 void SayNightCall();
 void Morning();
 bool Dale(const TArray<FString>& Lines);
 void StepDale();
 void HoldScripts(bool bOn);
};
