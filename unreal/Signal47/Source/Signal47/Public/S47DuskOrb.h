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
 void Photographed() { bPhotographed=true; }
 static FVector Position(const FVector& Anchor,double Age);
 static double Visibility(double Age);
 static bool SelfTest(TArray<FString>& Lines);
private:
 double DueClock=-1;
 int32 Attempts=0;
 bool bPhotographed=false;
};
