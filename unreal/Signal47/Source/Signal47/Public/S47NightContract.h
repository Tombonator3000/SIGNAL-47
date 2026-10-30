#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;

// Opt-in real new-night prefix. Never grants the remaining SC04-SC13 acceptance.
// Every exported boundary is a complete production save from this actual live case.
namespace S47NightContract
{
 void Step(US47Main* Main,double Dt);
 // Shared only by the live night controller and HorrorTest's explicit night adapter.
 // Uses genuine manual save, a new disk reader, title and ordinary Continue.
 bool RoundTrip(US47Main* Main,const FString& Root,const FString& Name,
  S47J::FObj& Evidence,FString& Error,bool Restore=true);
}
