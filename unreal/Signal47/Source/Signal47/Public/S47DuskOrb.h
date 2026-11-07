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
 void Photographed(); // the first photograph spaces out the evenings to come: about one in three from here on
 bool WasPhotographed() const { return bPhotographed; } // one paid record; the lights themselves keep coming
 // Index 0 owns the photographic record; 1 and 2 keep it company. All stop, sniff at a piece
 // of equipment and dart on to the next; still pure functions of age, never of the player's view.
 // Each is one unshadowed point light (Director's effects), so Count is also the cap on lights.
 static constexpr int32 Count=3;
 static constexpr double HomeX=-3.25,HomeZ=-47.,HomeUp=2.2; // the first orb's spot, between S-02 and S-03; Up is above the ground
 // What they stop at: Item is the world's own item for that thing (AS47Web::Item). X, Z are SARO metres, Up above the ground.
 struct FStop { const TCHAR* Item; const TCHAR* Name; double X,Z,Up; };
 static TArrayView<const FStop> Stops();
 // The stop this orb is holding at, or -1 while it travels or has not yet reached its first.
 static int32 StopAt(double Age,int32 Index=0);
 static constexpr double FleeRange=7.,FleeSeconds=1.4;
 static FVector Position(const FVector& Anchor,double Age,int32 Index=0);
 // Straight up and gone within a second and a half when a person comes too close.
 static double FleeRise(double FleeAge);
 static double Visibility(double Age);
 static bool SelfTest(TArray<FString>& Lines);
private:
 double DueClock=-1;
 int32 Attempts=0;
 bool bPhotographed=false;
 double NightClock=-1;
 int32 NightVisits=0;
};
