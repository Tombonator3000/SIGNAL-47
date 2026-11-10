#pragma once
#include "CoreMinimal.h"
class US47Main;

// N1110 (N1110-6, Tom 10 November: "prøver å gå ut en dør på SARO, men kommer inn sidedøra igjen og igjen; til slutt kommer man inn
// fra taket og så fra gulvet"). The door loop: Freeform only, inside the SARO building, in the full tier (after two, to half past five),
// never on the first night or in the first shift, never from the truck, at most once a week and only if no danger has come tonight.
//
// It is armed by the attention system at a visit's level, instead of the visit, when the player is inside with the power on (the visit
// would only hold over the roof and go), or after the big night has passed over, with the player inside. Arming is quiet: the night
// outside holds still (FS47Ambience::Hush), and three soft knocks come at the front door.
//
// The doors are the building's two outer doors, the front door (exitDoor, the visitor entrance at the west ramp) and the service door
// (doorEast, from the control room to the landing). E on one of them from inside, shut: the latch goes, and instead of going out the
// player is just inside the other one, facing in, as that door swings shut behind them. Walking out through one standing open does
// the same. Each time the station is a little more wrong:
//   1  the lights a step lower, the wall clock a minute on; "That was the front door." (or the service door)
//   2  lower again, two minutes more; the radio on the shelf comes on by itself for a few seconds; "No."
//   3  lower, three minutes more; the phone rings once; "I went out. I went OUT."
//   4  up near the corridor's ceiling: a moment's hang, a short fall, a thud and a small shake
//   5  the view comes up through the control room floor to eye height over 1.2 s, a low note under it
//   6  the door opens and the night is out there: the clock has lost 10 to 25 minutes in all, a field record says so, the lights
//      come back up, the night's sounds come back, and the Director rests (quiet minutes).
// Safe: six door uses at most (Cap), whatever happens; a spot that cannot be walked ends it at once; four idle minutes end it;
// anything else big (being taken, the sky cut, the truck, another area) ends it; Stop and Begin (Continue, load) put everything back.
// Nothing of it is saved: the week's rule reads the field record (door-loop-d<day>) the loop leaves.
class SIGNAL47_API FS47DoorLoop
{
public:
 enum class EStage : uint8 { None, Armed, Running, Done };
 enum class EStep : uint8 { Normal, Side, Ceiling, Floor, Out };
 static constexpr int32 Cap=6,SideLoops=3,FirstDay=2,WeekDays=7;
 static constexpr double ArmedFor=1200,IdleFor=240,CutAfter=.35,InsteadOdds=.5,LastHour=5.5,BigQuiet=90,RestAfter=420;

 // ---- the rule, pure ----
 // The loop's own state, nothing of the world: armed, then each door use is one step until the way out (or the cap).
 struct FMachine
 {
  EStage Stage=EStage::None;
  int32 Uses=0;
  bool Arm(){if(Stage!=EStage::None)return false;Stage=EStage::Armed;Uses=0;return true;}
  EStep Press();
  void Reset(){Stage=EStage::None;Uses=0;}
  bool Live() const{return Stage==EStage::Armed||Stage==EStage::Running;}
 };
 static EStep StepFor(int32 N);
 static double LightAt(int32 N);       // the room lights' scale after this use (1 before and after)
 static int32 JumpMinutes(int32 N);    // the clock's jump at this use
 static int32 MissingMinutes(int32 Seed,int32 Night); // in all, 10 to 25
 static int32 FinalMinutes(int32 Seed,int32 Night);   // what is left for the way out
 static bool MayLoop(int32 Day,int32 LastDay){return Day>=FirstDay&&(LastDay<=0||Day>=LastDay+WeekDays);}
 static const TCHAR* ThoughtFor(int32 N,bool bFromFront);
 static FString RecordText(int32 Minutes,bool bFromFront);

 explicit FS47DoorLoop(US47Main* Main):M(Main){}
 void Begin();
 void Stop();
 // Active seconds, while the session is not held.
 void Step(double Dt);
 // E on something: true if the loop took it (the door is not simply opened).
 bool Use(const FString& Id);
 // FS47Attention, at a visit's level, as the visit would begin: true if the loop goes instead (it is armed, and the visit is spent).
 bool TakeVisit();
 // F10: past the hour, the week and the dice; at SARO inside, on foot.
 FString DevArm();
 void DevEnd();
 bool Active() const{return Run.Live();}
 EStage StageNow() const{return Run.Stage;}
 int32 UsesNow() const{return Run.Uses;}
 FString Status() const;
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 FMachine Run;
 FString Source;
 int32 LastDay=0,BigNight=0,From=0,PendingFrom=-1;
 EStep PendingStep=EStep::Normal;
 double ArmedUntil=0,Idle=0,Pending=-1,ThoughtIn=-1,RadioOff=-1,BigAfter=0;
 FString ThoughtText;
 bool bDimmed=false,bRadioOurs=false,bLinesAdded=false,bBigSeen=false;
 bool WasIn[2]={false,false};
 // A fall from the ceiling (Hang, then gravity) and a rise through the floor, on the walker's Lift.
 double FallV=0,FallHang=-1,RiseT=-1,RiseSecs=1.2,RiseDepth=1.75;
 bool bFalling=false;
 bool Gates(FString& Why,bool bDev) const;
 bool InStation() const;
 int32 LastLoopDay() const;
 bool Arm(const TCHAR* Why,bool bDev);
 int32 DoorOf(const FString& Id) const;
 bool InsideOf(int32 Door) const;
 bool Arrival(int32 Door,double& X,double& Z,double& Yaw) const;
 void Start();
 void Cut(EStep S,int32 FromDoor);
 void Break(int32 FromDoor);
 void Finish(const TCHAR* Why);
 void Restore();
 void StepMotion(double Dt);
 void Think(const FString& Text,double After){ThoughtText=Text;ThoughtIn=After;}
 void AddDevLines();
};
