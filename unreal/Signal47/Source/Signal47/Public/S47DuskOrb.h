#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"

// Pure scheduling and world-space motion for the Director's first dusk lead.
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
 void Photographed(); // the first photograph spaces out the evenings to come: about one in three from here on
 bool WasPhotographed() const { return bPhotographed; } // one paid record; the lights themselves keep coming
 // Index 0 owns the photographic record; 1 and 2 keep it company. All stop, sniff at a
 // spot and dart on to the next; still pure functions of age, never of the player's view.
 static constexpr int32 Count=3;
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
};
