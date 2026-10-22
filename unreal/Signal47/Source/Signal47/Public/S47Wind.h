#pragma once

#include "CoreMinimal.h"

// world/wind.ts: the night wind, one set of values for every shader that moves with it (the brush
// and the grass of world/flora.ts: the flora materials read WindTime and WindAmp from MPC_Web, and
// their patch's WindDir). time runs on in seconds, the strength is how far a tip moves in metres at
// the most and breathes in slow gusts, dir is the way it blows on the ground (x, z), swinging a
// little about the south-west.
namespace S47Wind
{
	struct FWind { double Time = 0; double Strength = 0.07; FVector2D Dir = FVector2D(0.8, 0.6); };
	SIGNAL47_API FWind& Wind();
	/** windAt(t): moves the wind on to t seconds (flora.ts, before every picture). */
	SIGNAL47_API void WindAt(double T);
}
