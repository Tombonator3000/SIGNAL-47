#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;

// N1110 (N1110-1 and N1110-2, Tom 10 November: "søker man litt for mye et punkt på himmelen eller plager noen Aliens for mye får man
// besøk på saro av noen som vil bortføre spiller, Aliens kan sabotere utstyr"). A hidden number for how much the player has been
// bothering them, 0 to 100. Freeform only, never shown as a number; the rule is told by the world (Dale's walkie, the field records).
//
// What raises it (all from public state that is already there; nothing new to do):
//   staring       the view held on a light out there (FS47Omen's sources) for more than a few seconds; faster through the raised
//                 field camera
//   the lamp      the flashlight on one of them, near (a body's source)
//   chasing       closing on one of them on foot
//   going back    walking into fresh marks again the same night (a residual source: the first time is the job, again is not)
//   photographs   a new claim of something strange (FS47FreeformDirector::PhotoClaims), more for a frame that fills the print
//   the array     an anomalous card recorded, more when the same source was recorded in the last three days, and a little for
//                 each second the array integrates on an anomaly
// It falls by itself, FallPerDay a day of the game's clock (a rest falls it as much as the hours it skips).
//
// What it does, each at most once a night, in the hours of its tier (S47NightTiers):
//   TellAt      (N1110 pace) from 12, once a night in the warning hours: the ambience dips for a second (attention_tell), nothing else
//   HintDay     (N1110 pace) on day 3, Dale's word on the stare (attention_hint_dale) is said if the first warning has not brought it
//   WarnAt      a warning (event scripts attention_warn_*: the TV in snow, two rings and nobody, the dog stopping, lights over
//               the station, a thought), the first with Dale's word (attention_hint_dale): don't stare at them too long
//   SabotageAt  one piece of equipment, never two at once, from day 2, 23:30 to 05:00 (the evenings stay deniable): a relay fault on the array, the mains
//               (the sky cut, which the standby generator rides), the flashlight's cell, the truck that wants the key. Each has
//               its existing repair, and a trace (attention_sab_*: frost on the cabinet logged by the printer, a scorched smell,
//               warm cells). The first sabotage brings Dale's second word (attention_hint_visit): lights on and stay inside.
//   VisitAt     a visit at SARO, from day 2, at most one in two nights, only in the full tier (after two; a visit script with
//               gates.early is the authored escalation). It comes as the mains sag (attention_visit), and when they are out
//               it looks: inside with power (mains or the generator) it holds and goes (attention_visit_held); inside in the
//               dark the light comes for the player as being taken always does (FS47Taken::Begin: steps on the roof, then
//               the light through the windows, out of which the player can keep); in the yard the light stands over them
//               first (FS47Taken::BeginOver). Elsewhere it finds nobody. Afterwards the number falls to AfterVisit.
// Saved under "attention" (all keys optional; an older save starts at nothing). Nothing of a visit under way is saved.
class SIGNAL47_API FS47Attention
{
public:
 explicit FS47Attention(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 // Active seconds, while the session is not held.
 void Step(double Dt);
 S47J::FObj Save() const;

 // ---- the rule, pure ----
 static constexpr double WarnAt=18,SabotageAt=50,VisitAt=75,Top=100,FallPerDay=20; // N1110 (critic): calm play must not creep up; N1110 (pace): the first warning at 18, so the rule is met in week 1
 static constexpr double TellAt=12; // N1110 (pace): below the warnings, once a night, the night's sound dips for a second and nothing else
 static constexpr double HintQuiet=10; // N1110 (pace, critic 2): game minutes since the night began and since Dale last spoke, before the day-3 hint
 static constexpr int32 HintDay=3; // N1110 (pace): Dale's word on the stare, said on day 3 if it has not been heard
 static constexpr double StareGrace=4,CameraGrace=2,StarePerSec=.25,CameraPerSec=.6,LampPerSec=1.5,ChasePerSec=.8,ChaseMps=1.5;
 static constexpr double ReturnAgain=6,PhotoBase=1,PhotoClose=6,TrackRecorded=5,TrackAgain=5,TrackPerSec=.04;
 static constexpr double WarnDelay=15,SabotageDelay=90,VisitDelay=30,VisitSag=30,VisitOut=150,VisitLook=VisitSag+6;
 static constexpr int32 FirstDay=2,RepeatDays=3;
 // 0 calm, 1 warnings, 2 sabotage, 3 a visit.
 static int32 TierOf(double Score);
 static double Fall(double Score,double GameSeconds);
 // What a held stare adds over Dt, HeldFor seconds into it.
 static double StareGain(double HeldFor,double Dt,bool bCamera);
 // What walking into the same fresh marks adds, the Nth time that night (1: the first, nothing).
 static double ReturnGain(int32 Times);
 static double PhotoGain(double Quality);
 // N1110 (pace, critic 2): the hint comes in a quiet moment (pure): minutes since 18:00 and since Dale's last line.
 // N1110 (pace, critic 3): Dale's word is not skippable: if the quiet never came it is said anyway after 02:00 on day 3, and at the first quiet moment (the other gates still hold) on day 4 or later.
 static bool HintForced(int32 Day,double Hour){return Day>HintDay||(Day==HintDay&&Hour>=2&&Hour<5.5);}
 static bool HintQuietEnough(double MinutesIntoNight,double MinutesSinceDale){return MinutesIntoNight>=HintQuiet&&MinutesSinceDale>=HintQuiet;}
 static bool WarnHours(double Hour);     // 18:00 to 05:30
 static bool SabotageHours(double Hour); // 23:30 to 05:00
 // The visit's own hours: the full tier to 05:30 (bEarly: the authored escalation, from midnight).
 static bool VisitHours(double Hour,int32 Night,bool bEarly);
 static bool VisitMay(int32 Day,int32 Night,int32 LastVisitNight);
 enum class EOutcome : uint8 { Gone, Held, Flood, Over };
 static EOutcome OutcomeFor(bool bAtSaro,bool bInside,bool bPower);
 // Where the number falls to after a visit: bTaken only for Flood and Over.
 static double AfterVisit(EOutcome O,bool bTaken,double Score);
 enum class ESab : uint8 { None, Relay, Mains, Lamp, Truck };
 static const TCHAR* SabName(ESab K);
 static ESab SabFrom(const FString& Name);
 // The first kind Can allows, from the turn Turn: at SARO the relay, the mains, the lamp; away the truck, the lamp.
 static ESab PickSab(bool bAtSaro,int32 Turn,TFunctionRef<bool(ESab)> Can);
 // The warning scripts, in turn; the last is a thought and plays anywhere.
 static const TArray<FString>& WarnScripts();

 // ---- read, and the developer's ----
 double Score() const{return Value;}
 int32 Tier() const{return TierOf(Value);}
 ESab Sabotage() const{return Sab;}
 bool Visiting() const{return Visit!=EVisit::None;}
 int32 Warnings() const{return WarnCount;}
 int32 Sabotages() const{return SabCount;}
 int32 Visits() const{return VisitCount;}
 FString Status() const;
 void Add(double Points,const TCHAR* Why);
 void TestSet(double Score){Value=FMath::Clamp(Score,0.,Top);}
 static bool SelfTest(TArray<FString>& Lines);
private:
 enum class EVisit : uint8 { None, Coming, Taking };
 US47Main* M=nullptr;
 double Value=0,LastClock=-1;
 int32 WarnNight=0,WarnCount=0,SabNight=0,SabCount=0,VisitNight=0,VisitCount=0,TakenBefore=0;
 ESab Sab=ESab::None;
 bool bBaseline=false,bDev=false,bTruckArmed=false,bLinesAdded=false;
 double WarnWait=0,SabWait=0,VisitWait=0,StareFor=0,VisitT=0,ChaseDist=-1,SpareIn=-1;
 int32 TellNight=0; // N1110 (pace): the night the tell was played (not saved)
 bool bHintSaid=false; // N1110 (pace): Dale's word has been played this session
 int32 LearnNight=0; // N1110 (critic): the night the weak gear flicker for a long stare was played (not saved)
 EVisit Visit=EVisit::None;
 int32 Who=0;
 // What has been counted already: anomalous cards recorded, photo claims. Saved, so a Continue does not count them again.
 TSet<FString> Tracked,Claims;
 // This night's fresh marks walked into, by where they lie (not saved: a night).
 TMap<FString,int32> Returns;TSet<FString> NearMarks;int32 ReturnNight=0;
 bool Live() const;
 void TakeBaseline();
 void Gains(double Dt);
 void StepWarn(double Dt,double Hour,int32 Night);
 void StepTell(double Hour,int32 Night);
 void StepHint(double Hour,int32 Day);
 void StepSabotage(double Dt,double Hour,int32 Night,int32 Day);
 void StepVisitDraw(double Dt,double Hour,int32 Night,int32 Day);
 void StepVisit(double Dt);
 bool Play(const FString& Id) const;
 bool Gated(const FString& Id) const;
 bool DoSabotage(bool bForced);
 bool CanSab(ESab K) const;
 bool SabOver() const;
 void BeginVisit();
 void EndVisit(EOutcome O,bool bTaken);
 bool AtSaro() const;
 void AddDevLines();
};
