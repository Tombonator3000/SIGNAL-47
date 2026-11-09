#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
class US47Main;
class AS47Web;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class FS47DriveController;
enum class ES47SurfaceKind : uint8;

// Freeform only. The ordinary things that make a day and a drive something: the truck wants water and can bog down or go flat,
// a trip camera can be left at a place overnight, a round of the station gives the day a shape and a log, and now and then
// somebody's car stands empty on the shoulder. Nobody starves and nothing here can leave the player stuck: the jack, the
// spade and the county's roadside service are always there.
//
// M2 critic (TRUCK-STRAND): the truck strands where it can be read and with warning, never on a roll of the dice. Sand lies in the
// dry wash west of the highway (S47RoadShape::WashAt, the trough the land has; pale patches lie in it) and at the washes out on the
// land. In it the truck drags and the wheel tugs; driven hard and slow the wheels spin, the sand hisses and the engine labours a few
// seconds before it sinks to the axle, and easing off, keeping way on or backing out in its own tracks saves it. A tyre goes on
// rough ground driven fast (never on the blacktop): a bang, a thump from the wheel each turn and a pull to one side. The radiator
// pings and hisses before it boils dry. Short words; the steps are in the cargo rack's menu and the task list.
class FS47TruckCare
{
public:
 explicit FS47TruckCare(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Stuck() const{return bStuck;}
 bool Flat() const{return bFlat;}
 double Water() const{return WaterLeft;}
 bool Hot() const{return WaterLeft<=0;}
 bool NeedsRescue() const{return bStuck||bFlat||Hot();}
 FString StatusLine() const;
 // What is wrong, what it takes and where it is, step by step (empty when the truck is sound). Shown when the truck strands,
 // on the task list and in the truck menu. The spade, jack and wheel brace ride behind the seat; the spare wheel is in the pouch.
 FString Steps(int32 Spares,int32 Waters) const;
 // The county tow when it cannot be mended: $35 to $60 by the distance driven from SARO, 45 to 90 minutes. Never refused: what
 // cash cannot cover goes on the weekly sheet.
 int32 TowFee() const{return TowFeeFor(LegMetres);}
 double TowMinutes() const{return TowMinutesFor(LegMetres);}
 static int32 TowFeeFor(double Metres){return FMath::Clamp(35+FMath::RoundToInt(FMath::Max(0.,Metres)/1000.*2.5),35,60);}
 static double TowMinutesFor(double Metres){return FMath::Clamp(45.+FMath::Max(0.,Metres)/1000.*3.,45.,90.);}
 // Each takes game time and says so. The spade and jack ride in the truck.
 bool DigOut();
 bool ChangeWheel();
 bool TopUp();
 void Clear(); // Roadside service leaves the truck as it should be.
 // Metres the player has driven it since the last weekly review: tyres, oil and wear go on that sheet (FS47FreeformSession).
 double WeekDriven() const{return WeekMetres;}
 void WeekBilled(){WeekMetres=0;}
 static constexpr double WaterRange=36000; // metres on a full radiator
 static constexpr double DigMinutes=25,WheelMinutes=15;
 // TOWN-USE-20261108: with the Feed & Seed's tow rope and come-along in stock, hooked to a fence post or a bumper: quicker out.
 static constexpr double RopeDigMinutes=10;
 double DigTime() const;
 // How soft the ground is here, road-local: 0 hard, 1 the deepest of a wash's bed.
 static double SandAt(double RoadX,double RoadZ);
 // How fast the wheels dig in (per second, toward 1 = stuck; below 0 they come out): deep, driven hard and slow is worst; way on,
 // easing off or backing out takes it back.
 static double BogRate(double Soft,double Load,double Mps,bool bReverse);
 // The chance of a tyre going in a hundred metres on this ground at this speed (m/s): rocks and ruts taken fast; none on asphalt.
 static double FlatChance(double Mps,ES47SurfaceKind Kind);
 // How far the wheels have dug in now (0 to 1: stuck at 1), for the sound and the tests.
 double Bogging() const{return Bog;}
 // STUCK-GRACE-20261109 (Tom: at least 10 to 15 seconds of driving before it sinks again; it sank again the moment it was dug
 // out). Out by the spade or the come-along, the brush is under the wheels: it cannot sink before it has been driven GraceSeconds
 // in the cab AND GraceMetres, the later of the two, so standing in the sand with the wheels spinning never runs it out. The way to
 // firm ground is measured at the dig and the metres are stretched to reach it. After that, still in soft sand, the wheels dig in
 // slowly at first and harder over ReturnSeconds, and it says so first: never stuck sooner than WarnLead seconds after the warning.
 // Saved while it runs (the dig's autosave would otherwise lose it).
 static constexpr double GraceSeconds=12,GraceMetres=40,ReturnSeconds=8,WarnLead=2.5,FirmReach=80;
 enum class ESand:uint8{None,Warn,Stuck};
 // One step of the sand on the wheels, with the grace and its warning (pure: no world; the tests run it). DigScale: the tires'
 // own rate of digging in (FS47Outfit), on the digging only.
 ESand SandStep(double Dt,double Soft,double Load,double Mps,bool bReverse,double DigScale=1);
 // Out of the sand by the player's own hand: the grace starts. FirmMetres: how far firm ground lies (<0: not measured).
 void Rescued(double FirmMetres=-1);
 bool InGrace() const{return bGrace;}
 bool AfterRescue() const{return bAfterRescue;}
 double GraceSecondsLeft() const{return bGrace?FMath::Max(0.,GraceSeconds-GraceT):0.;}
 double GraceMetresLeft() const{return bGrace?FMath::Max(0.,GraceNeed-GraceM):0.;}
 // The nearest firm ground (sand under .15) round a road-local point within FirmReach, by rings outward: false when none.
 static bool FirmNear(double RoadX,double RoadZ,double& OutX,double& OutZ);
 // "north-east": which way B lies from A, road-local (x east, z south).
 static FString WayTo(double FromX,double FromZ,double ToX,double ToZ);
 // Where the dash's temperature needle should stand, 0.5 normal to 1 in the red, as the radiator runs low (for the truck's dash).
 double TempNeedle() const{return .5+.5*(1.-FMath::SmoothStep(0.,.3,WaterLeft));}
 void TestStick(){bStuck=true;}
 void TestBog(double K){Bog=FMath::Clamp(K,0.,1.);}
 void TestFlat(){bFlat=true;}
 void TestWater(double Value){WaterLeft=Value;}
 void TestLeg(double Driven){LegMetres=Driven;}
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 double WaterLeft=1,Metres=0,WeekMetres=0,LegMetres=0; // LegMetres: driven since the truck was last at SARO
 bool bStuck=false,bFlat=false,bWarned=false,bPowerSet=false,bHeldSet=false;
 // the sand and the wheel as they are driven (not saved: a Continue starts on firm ground)
 double Bog=0,SpinIn=0,ThumpIn=0,PingIn=20,TugT=0,PullSide=1,GustT=0; // GustT: a strong wind's push on the truck (WEATHER-20261108)
 bool bSandHeld=false,bSandHinted=false,bHissed=false;
 // the grace after a dig (saved while bAfterRescue), and the last firm ground the truck was driven on (road-local; not saved)
 bool bGrace=false,bAfterRescue=false,bReWarned=false;
 double GraceT=0,GraceM=0,GraceNeed=GraceMetres,ReturnT=0,WarnT=0;
 FVector2D LastFirm=FVector2D(1e9,1e9);
 void Hold();
 void StepGround(double Dt,FS47DriveController& D,double RoadX,double RoadZ);
 // the pale sand where it lies, near the player
 TWeakObjectPtr<UInstancedStaticMeshComponent> SandShown;
 TWeakObjectPtr<AS47Web> SandScene;
 FString SandArea;FVector2D SandCentre=FVector2D(1e9,1e9);
 void ShowSand();
};

// Built at the bench (FS47Workshop "trip_camera"), left at a place or by the marks, read the next morning. It records what really
// passed within its reach that night while the world there was going on (GAME_BIBLE: "Ingen venting eller søvn konstruerer
// observasjoner"): a body near it, or a light standing over its ground. What it caught it also photographed, from where it stood,
// and that frame is a print in the album to develop. Nothing that did not pass is drawn for it; only an ordinary animal is.
class FS47TripCamera
{
public:
 explicit FS47TripCamera(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Out() const{return bOut;}
 bool CanSet() const;
 bool SetHere(const FString& Title,bool bAtMarks);
 // What it caught, strongest first: a body near it, a light over its ground, nothing.
 enum class ECatch:uint8{None,Light,Body};
 static constexpr double BodyReach=18,LightReach=60;
 // TOWN-USE-20261108: with a roll of the Feed & Seed's fence wire in stock the trip line runs further out: it catches a body this far.
 static constexpr double WireBodyReach=30;
 // 0 nothing, 1 an animal (ordinary life, from the night's seed), 2 a light from above, 3 something standing in the frame.
 static int32 Reading(ECatch Got,int32 Seed,int32 Night);
 static FString ReadingText(int32 Read,const FString& Place,int32 Seed,int32 Night,double When=-1,bool bFrame=false);
 ECatch CaughtTonight() const{return Caught;}
 bool Collect();
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 bool bOut=false,bAtMarksThen=false,bBuilt=false;
 FString Area,Title,PhotoId;
 FVector At=FVector::ZeroVector;
 double SetClock=0,CaughtClock=-1,LookIn=0;
 ECatch Caught=ECatch::None;
 TWeakObjectPtr<UStaticMeshComponent> PropSaro,PropRoad;
 // What is in front of it now, looked for a few times a second while it is out and the world it stands in is the loaded one.
 void Watch();
 bool Sees(const FVector& Lens,const FVector& P) const;
 FS47Obj ObjSaro,ObjRoad;
 TWeakObjectPtr<AS47Web> Scene;
 TSharedPtr<uint8> Life;
 bool Build();
 void Layout();
};

// A round of the station: three stops a day, drawn for the day, by daylight. Done before noon a coming fault is caught at a cost of
// one relay. A shift log comes in each morning, of what the record says about yesterday.
class FS47DayRound
{
public:
 explicit FS47DayRound(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Step(double Dt);
 S47J::FObj Save() const;
 static constexpr int32 StopCount=3,RoundPay=5;
 static const TArray<const TCHAR*>& AllStops();
 static TArray<FString> StopsFor(int32 Seed,int32 Day);
 static FString StopName(const FString& Id);
 FString TaskLine() const;
 bool DoneToday() const{return DoneDay==CurrentDay();}
 int32 StopsDone() const{return Ticks.Num();}
 static FString LogPage(int32 Day,int32 Filed,int32 Paid,bool bRound,int32 Records,const FString& Trouble);
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 int32 ForDay=0,DoneDay=0,LogDay=0;
 TArray<FString> Ticks;
 int32 CurrentDay() const;
 void WriteLog(int32 Day);
};

// Somebody's pickup on the shoulder with its door open, some mornings: seen, looked into, gone in a day or two. Nobody ever turns
// up for it. Not Ortiz's wagon: that has a story of its own.
class FS47RoadsideCar
{
public:
 explicit FS47RoadsideCar(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Step(double Dt);
 void Stop();
 S47J::FObj Save() const;
 static bool StandsOn(int32 Seed,int32 Day);
 static double RoadZ(int32 Seed,int32 Day);
 static FString Inside(int32 Seed,int32 Day);
 bool Shown() const{return bShown;}
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 TWeakObjectPtr<UStaticMeshComponent> Car;
 TWeakObjectPtr<AS47Web> Scene;
 FS47Obj Obj;
 TSharedPtr<uint8> Life;
 bool bBuilt=false,bShown=false;
 int32 ShownDay=0;
 bool Build();
 void Look();
};
