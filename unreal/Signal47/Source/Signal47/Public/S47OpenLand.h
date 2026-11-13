#pragma once
#include "CoreMinimal.h"
#include "S47Legs.h"

class AS47Web;

// Freeform's open land (Tom, 7 November 2026: "usynlige vegger rundt hele spillet"). Story's areas are three places with roads
// between them, and what kept the truck and the walker on those roads were boxes nobody sees. Here the land is one piece: the
// road's terms own all of it, and SARO's and the station's areas are only what stands closest round their sites. Anyone on foot
// or at the wheel crosses from one area into the next wherever they leave it (all in road-local metres), between two frames.
// What stops them is a hill (S47Relief::HillStops) or, far out, the fence along the Rim, drawn where it stands. Without
// Live every rule is Story's. -S47OpenLandOff keeps them so in Freeform too, for a measure of what it was.
namespace S47OpenLand
{
	/** A little inside the rim of SM_FF_Ground_Road (-3000..2300, -2700..4400): past it is no ground. */
	inline constexpr double RimX0 = -2950, RimX1 = 2250, RimZ0 = -2650, RimZ1 = 4350;
	/** How far past an area's line one goes before changing to the next, each way: no flicker for one who stands on it. */
	inline constexpr double HandOverM = 8;

	SIGNAL47_API bool Off();
	SIGNAL47_API bool Live();
	SIGNAL47_API void SetLive(bool bOn);
	/** Off for as long as this lives, for a check that compares against fixed web numbers while a game may be running. */
	struct SIGNAL47_API FSuspend { FSuspend(); ~FSuspend(); bool bWas; };

	SIGNAL47_API bool InRim(double RoadX, double RoadZ, double Inset = 0);
	/** The site, the array, the motel and the farm, on a line where the two lands lie closest (at most 2.4 m apart; .9 by the farm and the highway). */
	SIGNAL47_API bool InSaroCore(double RoadX, double RoadZ, double Grow = 0);
	/** The compound and its flat ground (the two lands at most .8 m apart). */
	SIGNAL47_API bool InStationCore(double RoadX, double RoadZ, double Grow = 0);
	/** A wire fence or an unseen wall of Story's, by its shape: long and thin. Open in Freeform. */
	SIGNAL47_API bool IsBoundaryLine(double MinX, double MaxX, double MinZ, double MaxZ);
	/** SARO's truck ramp, its two open sides (SARO-GEN-RAMP-20261109, Tom's test of d37: "an invisible wall, one cannot get down off
	 *  the ramp"). They hold the truck to the ramp (US47World::ReconcileFarm) but not one on foot, who steps down off them anywhere and
	 *  up where the side is low enough (AS47Walker::Resolve); the pad's own sides stay shut to both. True for those two lines only. */
	SIGNAL47_API bool TruckRampSide(double MinX, double MaxX, double MinZ, double MaxZ);

	/** World terms of a leg (or the diner's area, or the motel's), to road-local and back. False: not a leg of the land. */
	SIGNAL47_API bool RoadOf(const FString& Leg, double X, double Z, double& OutRoadX, double& OutRoadZ);
	SIGNAL47_API FVector FromRoad(const FString& Leg, double RoadX, double RoadZ);
	/** A heading or a walker's yaw in the next leg's terms: the station's are the road's turned a quarter round. */
	SIGNAL47_API double Turned(const FString& FromLeg, const FString& ToLeg, double Heading);
	/** The leg this road-local point belongs to, for one now in FromLeg. FromLeg itself while within HandOverM of its line. */
	SIGNAL47_API FString LegFor(const FString& FromLeg, double RoadX, double RoadZ);
	/** Live's S47Legs::ExitFrom: out of an area wherever it ends, not only on the road. */
	SIGNAL47_API TOptional<S47Legs::FExit> Exit(const FString& Leg, double X, double Z, double Heading);

	/** On the line of the highway's right-of-way fences or the old pasture's (roadProps.ts fences). None of them holds anyone in Freeform (Tom, 7 November 2026).
	 *  Of the pasture's all that is left to see is a short worn stretch here and there, and now and then a few posts with no wire; of the highway's nothing
	 *  (HWY-FENCE-GONE-20261113, Tom 13 November: "det har ikke funksjon lenger"). */
	SIGNAL47_API bool OnRoadFence(double RoadX, double RoadZ);
	SIGNAL47_API bool FenceWireStays(double RoadX, double RoadZ);
	SIGNAL47_API bool FencePostStays(double RoadX, double RoadZ);

	/** The fence along the rim, posts and two wires, where there is open land to stand on: built once, shown in the road's area. And the old fence's posts taken away. */
	SIGNAL47_API void ReconcileFence(AS47Web* Web, bool bOn, const FString& Area);
	SIGNAL47_API int32 FencePosts();
	/** HWY-FENCE-GONE-20261113: nothing of the highway's fence stays in Freeform; the pasture's remnants do. */
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
