#pragma once

#include "CoreMinimal.h"

// world/geo.ts: how the places of the night lie on one map. Each area is built in its own terms
// far from the others in the scene (World.ts); the road's terms (S47RoadTerrain.h) are the map:
//
//   SARO        road-local = site + (24, 0, -300): the site's highway is the road's
//   STATION 01  the road's parking spot by the gate (END) is the station's (STATION_TRUCK),
//               turned a quarter round: the survey track comes in from the south there
//
// With world/stationLayout.ts's numbers, which the drive needs without the station itself.
// (fromRoad and fromStation place meshes: the exported scene has them placed already.)
namespace S47Geo
{
	struct FRect { double MinX, MaxX, MinZ, MaxZ; };
	struct FXZ { double X, Z; };
	struct FAt { double X, Y, Z; };

	inline constexpr FXZ SARO_IN_ROAD = { 24, -300 };
	/** Sierra Motor Court in SARO's terms: 650 m south down the highway and 2.5 m further west,
	 *  its lot 8 cm above the flat land round it (roadShape.MOTEL). */
	SIGNAL47_API const FAt& MOTEL_AT();
	/** The motel's lot, walk and buildings (site). */
	SIGNAL47_API const FRect& MOTEL_RECT();

	// ---------- world/stationLayout.ts ----------
	/** The track in from the south: the pad by the gate first, then out towards the road (station-local). */
	SIGNAL47_API const TArray<FVector2D>& STATION_TRACK();
	struct FFence { double X0, X1, Z0, Z1; };
	inline constexpr FFence STATION_FENCE = { -30, 30, -24, 26 };
	/** The station's flat ground round the hut and the pad (station-local y). */
	inline constexpr double STATION_GROUND = -0.15;
	/** The gravel pad outside the gate, where the truck is left. */
	inline constexpr FRect STATION_PAD = { -27.6, -12.2, 26.3, 39.8 };
	/** The truck as it stands on the pad (the night it first comes here). */
	struct FTruckAt { double X, Z, Heading; };
	inline constexpr FTruckAt STATION_TRUCK = { -19.25, 31.4, 0 };

	/** The station's terms are the road's turned by this (rotation.y), about END. */
	SIGNAL47_API double STATION_TURN();
	SIGNAL47_API FXZ RoadToStation(double X, double Z);
	SIGNAL47_API FXZ StationToRoad(double X, double Z);
	/** Station height = road height + this: the road's land at END is the station's flat ground. */
	SIGNAL47_API double STATION_DY();

	/** The pieces of the road's ground both areas draw, road-local. */
	inline constexpr FRect SARO_PATCH = { -120, 120, -200, 600 };
	inline constexpr FRect STATION_PATCH = { -592, -190, 430, 610 };
	SIGNAL47_API bool InRect(const FRect& R, double X, double Z, double M = 0);
	/** How far (x, z) lies outside a rectangle; 0 inside. */
	SIGNAL47_API double Outside(const FRect& R, double X, double Z);
	/** The road's land brought down to the station's flat ground over the last of the track (metres to take off). */
	SIGNAL47_API double StationBend(double X, double Z);
	/** The road's land at a road-local point, in the station's terms. */
	SIGNAL47_API double StationLand(double X, double Z);
}
