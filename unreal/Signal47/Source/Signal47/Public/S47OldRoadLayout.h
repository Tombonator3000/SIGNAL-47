#pragma once

#include "CoreMinimal.h"
#include "S47RoadShape.h"

// drive/oldRoadLayout.ts: the old Roswell road, chapter six. Laid out as on SARO's service map
// (E14): from the diner east-north-east, then a long bend to the north-east. C runs due east from
// STATION 01 and crosses the road 8.15 miles from the diner. Local metres, origin where C crosses
// the road; x is east, north is -z. C is the line z = 0.
//
// The road is a path of points every 4 m, s = 0 at the crossing, negative behind it, back to
// mile 0, where it leaves the state highway across from Mesa Diner. The path is worked out here
// on first use exactly as the module does when it loads.
namespace S47OldRoadLayout
{
	using S47RoadShape::MILE;
	using S47RoadShape::CROSS_MI;

	/** Miles from the diner at a distance s along the road, and back. */
	inline constexpr double MiAt(double S) { return CROSS_MI + S / MILE; }
	inline constexpr double SAt(double Mi) { return (Mi - CROSS_MI) * MILE; }
	inline double Bearing(double Mi) { return S47RoadShape::OldRoadBearing(Mi); }

	inline constexpr double S0 = SAt(0);   // the road is built from the highway at the diner
	inline constexpr double S1 = 1650;  // to well past the crossing
	inline constexpr double STEP = 4;
	inline constexpr double HALF = 3.2, SHOULDER = 4.4, FENCE = 17, POWER = -28;

	struct FRoadPt { double X = 0, Z = 0, Tx = 0, Tz = 0, S = 0, Y = 0; };

	/** The land, and near the highway at the diner the road area's own ground. */
	SIGNAL47_API double Terrain(double X, double Z);
	/** How much of the old road's own land there is at a point (0 by the highway). */
	SIGNAL47_API double OwnLand(double X);

	/** Mile 0: where the road leaves the highway, on its centreline (local terms). */
	struct FXZ { double X = 0, Z = 0; };
	SIGNAL47_API const FXZ& J();
	SIGNAL47_API const TArray<FRoadPt>& Path();
	/** Index of the crossing in the path. */
	SIGNAL47_API int32 I0();

	/** The point of the road at a distance s, interpolated. */
	SIGNAL47_API FRoadPt RoadAt(double S);
	/** A point beside the road: o metres to the right of travel (negative is left). */
	struct FBeside { double X, Z; FRoadPt P; };
	SIGNAL47_API FBeside Beside(double S, double O);
	/** Distance to the road's centreline (o is signed, right of travel positive), s and grade
	 *  there. Far from the road (more than about 150 m) d is Infinity. */
	struct FNearest { double D, S, O, Y; };
	SIGNAL47_API FNearest Nearest(double X, double Z);

	/** The road's cross-section: crown, shoulders, a shallow ditch, then up to the land. */
	SIGNAL47_API double Profile(double O);
	/** World-local height of the ground at (x, z). */
	SIGNAL47_API double HeightAt(double X, double Z);

	/** The truck at the start: the right lane at 5.9 miles, the engine running. */
	struct FStart { double X, Z, Heading, S; };
	SIGNAL47_API const FStart& START();
	/** C, the old survey line: due east through the crossing. */
	struct FCLine { double X0, X1, Z; };
	inline constexpr FCLine C_LINE = { -620, 940, 0 };
	/** Mesa on the east-north-east horizon where the sun comes up (azimuth, degrees). */
	inline constexpr double SUN_AZ = 79;
}
