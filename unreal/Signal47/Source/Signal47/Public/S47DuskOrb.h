#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"

// Pure scheduling and world-space motion for the Director's low lights: the first dusk lead, and their own visits in the dark.
// Director owns rendering, active-event age, persistence and photographic claims.
class FS47DuskOrb
{
public:
 static constexpr double Duration=300.;
 void Begin(const S47J::FObj& Saved);
 S47J::FObj Save() const;
 bool Update(double Clock,bool bTutorial); // true when the schedule needs saving
 bool Ready(double Clock) const;
 void Started(double Clock); // consume this opportunity before presenting it
 // The dark hours have their own visits, apart from the dusk: every night or three, at a written-out hour, always ended
 // well before FS47FarmSite::TheirHours. One that is slept through or missed is rescheduled, never made up for.
 bool ReadyNight(double Clock) const;
 double NextNightClock() const { return NightClock; }
 int32 NightVisitCount() const { return NightVisits; }
 // Every visit so far, dusk and night: none yet is the first, which comes where it always has (ARRAY-ORBS).
 int32 VisitsSoFar() const { return Attempts + NightVisits; }
 // What this visit is, chosen when it starts and saved with it: how many lights come, for how long, and the seed of their flight.
 int32 VisitLights() const { return Lights; }
 double VisitSeconds() const { return Seconds; }
 int32 VisitSeed() const { return Seed; }
 void Photographed(); // the first photograph spaces out the evenings to come: about one in three from here on
 bool WasPhotographed() const { return bPhotographed; } // one paid record; the lights themselves keep coming
 // Tom, 7 November: sporadic, one to three, hovering about and investigating, as drones do. Index 0 owns the photographic
 // record; 1 and 2 keep it company when a visit has them. Each hovers with small quick corrections, looks over a piece of
 // equipment in short darts, and hops on to the next in its own time: still pure functions of age and the visit's seed,
 // never of the player's view. Each is one unshadowed point light (Director's effects), so Count is the cap on lights.
 static constexpr int32 Count=3;
 static constexpr double HomeX=-3.25,HomeZ=-47.,HomeUp=2.2; // the first orb's spot, between S-02 and S-03; Up is above the ground
 // What they stop at: Item is the world's own item for that thing (AS47Web::Item). X, Z are SARO metres, Up above the ground.
 // Scan: how far either way, in metres, the darts go over it.
 struct FStop { const TCHAR* Item; const TCHAR* Name; double X,Z,Up,Scan; };
 static TArrayView<const FStop> Stops();
 // The stop this orb is holding at, or -1 while it travels or has not yet reached its first.
 static int32 StopAt(double Age,int32 Index=0,int32 Seed=0);
 static constexpr double FleeRange=7.,FleeSeconds=1.4;
 static FVector Position(const FVector& Anchor,double Age,int32 Index=0,int32 Seed=0);
 // A glow shows no tilt of its own, so the light says it: it hangs under the body, trails its turns and dips while it looks.
 static FVector LightOffset(double Age,int32 Index=0,int32 Seed=0);
 // 0 travelling or hovering, 1 looking hard at the thing it has stopped at.
 static double ScanLevel(double Age,int32 Index=0,int32 Seed=0);
 // One orb's own fade: the first is there for the whole visit, the others come later and go earlier, each in its own time.
 static double Presence(double Age,int32 Index=0,int32 Seed=0,double VisitLength=Duration);
 // Over a whole visit, for what a test or a log line says about it.
 struct FFlight { int32 Hops=0; double ShortestHold=0,LongestHold=0,LongestHop=0,Fastest=0,FirstHopAt=0; };
 static FFlight Flight(int32 Index,int32 Seed,double VisitLength=Duration);
 // Straight up and gone within a second and a half when a person comes too close.
 static double FleeRise(double FleeAge);
 static double Visibility(double Age,double VisitLength=Duration);
 static bool SelfTest(TArray<FString>& Lines);
private:
 double DueClock=-1;
 int32 Attempts=0;
 bool bPhotographed=false;
 double NightClock=-1;
 int32 NightVisits=0;
 int32 Lights=Count,Seed=0;
 double Seconds=Duration;
};
