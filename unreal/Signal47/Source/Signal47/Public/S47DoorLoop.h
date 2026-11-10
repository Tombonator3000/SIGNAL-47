#pragma once
#include "CoreMinimal.h"
class US47Main;
class FS47OutsideLight;
enum class ES47Taker : uint8;

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
//
// N1110 (N1110-7, Tom: the loop also when one of them is after the player, with floodlight, strobe and colours through the windows).
// The chase: while something is coming for the player (a site or field visit's body hunting or walking at them, FS47FarmSite; the light
// looking for them, FS47Taken's dread or flood, which is also how the attention visit at SARO comes), an outer door used from inside a
// building (the SARO building's two, the old farmhouse's two, FS47FarmHouseDoors, or a room in town's one, S47CalicheAccess) may start
// the same loop at once, without arming: what was hunting gives way to it (the body goes, the light goes out) and the thing is now
// outside the building. Every door leads back in, the one they came in by when the room has only one. The same six steps and the same
// cap, but what marks them is outside: (1) steps along the wall, (2) a shape crossing the light at the window, (3) the handle of the
// door behind them tried. All the while a massive light pours in at the windows (FS47OutsideLight, as when being taken, but stronger):
// white, amber, blue and green, with bursts of strobe; with Calm flashes chosen it only breathes and its colours fade slowly, no strobe,
// and the shape crossing it is a slow dimming. The ways it ends: the sixth door lets them out, or a way out the loop does not hold, and
// the thing is gone (the light out, missing time, a field record, quiet minutes); or they stop using doors: after 25 s the handle of the
// nearest door turns and steps come round, and after 40 s it is in, and the ordinary taking takes over (FS47Taken::TakeNow).
// At most once a night, every third night at most, never the night an ordinary loop ran, the full tier, and only on a draw (ChaseOdds);
// the meeting is the hunt's own, so the night's one danger is not counted again. Nothing saved; the field record is door-loop-chase-d<day>.
class SIGNAL47_API FS47DoorLoop
{
public:
 enum class EStage : uint8 { None, Armed, Running, Done };
 enum class EStep : uint8 { Normal, Side, Ceiling, Floor, Out };
 static constexpr int32 Cap=6,SideLoops=3,FirstDay=2,WeekDays=7,ChaseEvery=3;
 static constexpr double ArmedFor=1200,IdleFor=240,CutAfter=.35,InsteadOdds=.5,LastHour=5.5,BigQuiet=90,RestAfter=420;
 static constexpr double ChaseOdds=.65,ChaseWarn=25,ChaseIdle=40,DevChaseFor=300;

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
 // N1110-7, the chase's own: the light at the windows (its strength, 0 to about 1.7, and its colour) Seconds into it; the shape crossing
 // it (a share, 1 untouched) Seconds after it began to cross, and how long that takes.
 static bool MayChase(int32 Day,int32 LastChase,int32 LastLoop){return Day>=FirstDay&&LastLoop!=Day&&(LastChase<=0||Day>=LastChase+ChaseEvery);}
 static double FloodK(double Seconds,bool bCalm);
 static FLinearColor FloodColor(double Seconds,bool bCalm);
 static double ShadeK(double Seconds,bool bCalm);
 static double ShadeFor(bool bCalm){return bCalm?1.4:.7;}
 static const TCHAR* ChaseThought(int32 N);
 static FString ChaseRecord(const FString& Title,int32 Minutes);

 explicit FS47DoorLoop(US47Main* Main):M(Main){}
 ~FS47DoorLoop();
 void Begin();
 void Stop();
 // Active seconds, while the session is not held.
 void Step(double Dt);
 // E on something: true if the loop took it (the door is not simply opened). SARO's doors by item id, the old house's by
 // ff:olddoor:<name>, a room in town's by access:<room>.
 bool Use(const FString& Id);
 // FS47Attention, at a visit's level, as the visit would begin: true if the loop goes instead (it is armed, and the visit is spent).
 bool TakeVisit();
 // F10: past the hour, the week and the dice; at SARO inside, on foot.
 FString DevArm();
 // F10 (N1110-7): something outside for the next five minutes, past the hour, the night and the dice: an outer door from inside SARO,
 // the old house or a room in town starts the chase.
 FString DevChase();
 void DevEnd();
 bool Active() const{return Run.Live();}
 // N1110-7: the loop is the chase: FS47Attention's visit waits for it, as it waits for the light.
 bool Chasing() const{return Run.Live()&&bChase;}
 EStage StageNow() const{return Run.Stage;}
 int32 UsesNow() const{return Run.Uses;}
 FString Status() const;
 static bool SelfTest(TArray<FString>& Lines);
private:
 // N1110-7: the building the loop holds and its doors to the outside. Out is square to the door, horizontal, pointing outside; Mid is
 // the middle of the doorway (three-space metres; only x and z count); Half is half its width along the wall.
 enum class EPlace : uint8 { None, Saro, House, Room };
 struct FGate { FString Id; FVector Mid=FVector::ZeroVector,Out=FVector::ForwardVector; double Half=.5; int32 Index=-1; };
 struct FPlace
 {
  EPlace Kind=EPlace::None;
  FString Area,Room,Title;
  TArray<FGate> Gates;
  FVector Up=FVector::ZeroVector; // where the fall from the ceiling is (x, z)
  double UpYaw=0;
  bool bUp=false,bUpYaw=false;
  bool Same(const FPlace& O) const{return Kind==O.Kind&&Room==O.Room;}
 };
 static FPlace SaroPlace();
 static double SideOf(const FGate& G,double X,double Z);           // past the door's plane, outward
 static bool InBand(const FGate& G,double X,double Z,double Pad);   // in front of the doorway, along the wall
 static double YawIn(const FGate& G);                               // facing in, away from the door
 int32 OtherOf(int32 Gate) const{return Where.Gates.Num()>1?(Gate+1)%Where.Gates.Num():Gate;}

 US47Main* M=nullptr;
 FMachine Run;
 FString Source;
 int32 LastDay=0,BigNight=0,From=0,PendingFrom=-1;
 EStep PendingStep=EStep::Normal;
 double ArmedUntil=0,Idle=0,Pending=-1,ThoughtIn=-1,RadioOff=-1,BigAfter=0;
 FString ThoughtText;
 bool bDimmed=false,bRadioOurs=false,bLinesAdded=false,bBigSeen=false;
 TArray<bool> WasIn;
 // A fall from the ceiling (Hang, then gravity) and a rise through the floor, on the walker's Lift.
 double FallV=0,FallHang=-1,RiseT=-1,RiseSecs=1.2,RiseDepth=1.75;
 bool bFalling=false;
 // The loop's building, and the one watched for a chase to start at (kept a moment after the player is out of it).
 FPlace Where,Watch;
 // N1110-7: the chase.
 bool bChase=false,bWarned=false;
 ES47Taker ChaseWho{};
 FString ChaseTitle;
 int32 ChaseDay=0,ChaseNight=0;
 double DevChaseLeft=0;
 TSharedPtr<FS47OutsideLight> Flood;
 double FloodT=0,FloodFind=0,ShadeT=-1;
 FVector FloodFrom=FVector::ZeroVector;
 bool bFloodWay=false;
 int32 StepsLeft=0,StepsTotal=0,KnobGate=-1;
 double StepsIn=0,KnobT=-1,KnobTick=0,KnobWait=0;
 FVector StepsA=FVector::ZeroVector,StepsB=FVector::ZeroVector;
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
 // The doors, whichever building they are in.
 bool GateOpen(const FPlace& P,int32 I) const;
 void GateShut(const FPlace& P,int32 I);
 void GateSwingShut(const FPlace& P,int32 I);
 void GateKnob(const FPlace& P,int32 I,double K);
 bool PlaceAt(FPlace& Out) const;
 bool InPlace() const;
 bool ThereStill() const;
 // The chase: what is after the player now, if anything, and whether it may become the loop.
 bool Chased(ES47Taker& Who,FString& Title) const;
 bool ChaseGates(FString& Why,bool bDev) const;
 bool StartChase(const FPlace& At);
 void YieldHunt();
 void Caught();
 int32 LastChaseDay() const;
 bool Calm() const;
 // False: it has ended (caught).
 bool StepChase(double Dt);
 void StepFlood(double Dt);
 void Footsteps(int32 Gate,int32 Count);
 void Knob(int32 Gate,double Seconds,double Wait);
 int32 NearestShut() const;
 // The door of P the player has just walked out through, standing open, from inside; -1 none. bAtDoor: near one of them.
 int32 Crossing(const FPlace& P,bool& bAtDoor);
};
