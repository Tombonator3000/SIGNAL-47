#pragma once

#include "CoreMinimal.h"

// drive/roadShape.ts: the road area's land in closed form, so the places beside the road (SARO's
// highway south of the site, the last of the survey track at STATION 01, the old road) follow the
// same ground without the road. Road-local metres, as in roadTerrain.ts (S47RoadTerrain.h).
// Values the TypeScript works out once when the module loads (DINER.y, MOTEL.y) are functions
// here, worked out on first use.
namespace S47RoadShape
{
	SIGNAL47_API double Smooth(double A, double B, double X);
	SIGNAL47_API double Hash(double X, double Z);

	struct FPose2 { double X, Z, Heading; };
	/** Where the drive used to start: the southbound lane, SARO 300 m behind. */
	inline constexpr FPose2 START = { -1.8, 0, UE_DOUBLE_PI };
	/** The truck parked at the station gate, facing it. */
	inline constexpr FPose2 END = { -603, 532, UE_DOUBLE_PI / 2 };

	SIGNAL47_API double WashAt(double X, double Z);

	/** Mesa Diner beside the highway 2 km south of the start: its own origin is here, its x 20 the
	 *  highway's centre, and the land round its lot is flat at DINER.y. */
	struct FDinerAt { double X, Z, FlatX, Y; };
	SIGNAL47_API const FDinerAt& DINER();
	/** Caliche: its main street is the highway, from the diner south. */
	struct FTownAt { double Z0, Z1, West, East; };
	SIGNAL47_API const FTownAt& TOWN();
	/** Sierra Motor Court beside the highway on the west side, about 650 m south of SARO. */
	struct FMotelAt { double FlatX, Z, Y; };
	SIGNAL47_API const FMotelAt& MOTEL();
	/** The cattle in the pasture south of the motel. */
	struct FHerdAt { double X, Z; int32 Missing; };
	inline constexpr FHerdAt HERD = { -37, 556, 4 };

	/** The graded land the roads follow. */
	SIGNAL47_API double HLow(double X, double Z);

	/** The highway's paved half-width and its gravel shoulder's. */
	struct FHwyW { double Paved, Shoulder; };
	inline constexpr FHwyW HWY_W = { 4.8, 6 };
	/** How far the highway's top stands above the graded land, by distance from its centre line. */
	SIGNAL47_API double HwyLift(double Ax);
	/** Height of the highway, its shoulders and the flat verge beside them, at a road-local point. */
	SIGNAL47_API double HighwayTop(double X, double Z);

	// ---------- the old Roswell road (oldRoadLayout.ts) ----------
	inline constexpr double MILE = 1609.34;
	/** Where C crosses the old road, in miles from the diner. */
	inline constexpr double CROSS_MI = 8.15;
	/** The old road's heading in degrees from north (clockwise), by miles from the diner. */
	SIGNAL47_API double OldRoadBearing(double Mi);
}
