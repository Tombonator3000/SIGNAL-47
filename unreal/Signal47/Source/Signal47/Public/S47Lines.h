#pragma once

#include "CoreMinimal.h"

// web/src/story/lines.ts: where the 1947 lines A, B and C cross the ground (HENDELSER.md, section
// 3): the one place that says so. What the night's layer asks of it is here: which line a place is
// on (the seen log, story/seen.ts), where the marked events happen (PLACES), and the areas' terms on
// the map. The map is the road's terms: road-local metres, x east, z south, the highway's centre at
// x 0, SARO's site 300 m north of the road's start. (CROSSINGS, which the time away and the lights
// on B ask, comes with them: it needs the antenna field's layout.)
//
//   A  from the transit at STATION 01 through Caliche, over the main street at the sheriff's
//      substation; beyond the town, the highway north of it is still its ground.
//   B  from the transit to B-12's vane in SARO's service yard, on through the east half of the
//      antenna field: SARO's line, the motel and the herd within its reach (520 m).
//   C  the closing line: the station's own ground near the transit, and the old Roswell road near
//      the line z 0 of that area.
namespace S47Lines
{
	/** A point or a direction on the map: road-local metres, x east, z south. */
	using FMapPt = FVector2D;

	/** The transit's pier at STATION 01, and B-12's vane in SARO's service yard, on the map. */
	SIGNAL47_API FMapPt TRANSIT();
	SIGNAL47_API FMapPt B12();
	/** Where the night's marked events happen, on the map (phone, phone2, repeat, lamps, boy, regular,
	 *  cutout, car, lights, cows, cows2, tv, truck7m, ninth, truck4, truck7). */
	SIGNAL47_API const TMap<FString, FMapPt>& PLACES();

	/** A and B: a point they go through (the transit), the way they run from there towards `To`, and
	 *  how far from them their ground reaches. */
	struct FStraight { FMapPt Through, Dir; FString To; double Reach = 0; };
	SIGNAL47_API const FStraight& LINES(TCHAR Id);

	/** Signed distance of a map point from A or B (positive to the right as it runs towards `to`). */
	SIGNAL47_API double Offset(TCHAR Id, double X, double Z);
	SIGNAL47_API double DistTo(TCHAR Id, double X, double Z);
	/** Metres along A or B from the transit (positive towards `to`). */
	SIGNAL47_API double AlongFrom(TCHAR Id, double X, double Z);
	SIGNAL47_API FMapPt PointAt(TCHAR Id, double S);
	/** Where A or B crosses the highway's centre line (road x 0): road z. */
	SIGNAL47_API double HighwayZ(TCHAR Id);
	/** Metres south of SARO, as the notebook's map counts the highway. */
	inline double MSouth(double RoadZ) { return RoadZ - -300.0; }

	/** A point of an area in its own local terms on the map (room 6 is the motel's room 6); unset
	 *  where the area has no place on it (the old road). */
	SIGNAL47_API TOptional<FMapPt> ToMap(const FString& Area, double X, double Z);
	/** A map point in an area's own local terms (unset where the area has no place on the map). */
	SIGNAL47_API TOptional<FMapPt> FromMap(const FString& Area, double X, double Z);
	/** A direction on the map in an area's own terms (the station's are a quarter turn round). */
	SIGNAL47_API FMapPt DirIn(const FString& Area, const FMapPt& D);

	/** On a line: which ('A' | 'B' | 'C'), how far from it, and which way it runs there. */
	struct FOnLine { FString Line; double Dist = 0; FMapPt Along; };
	/** Which line's ground a point of an area is on (its local terms), or unset between the lines. */
	SIGNAL47_API TOptional<FOnLine> LineAt(const FString& Area, double X, double Z);
	/** The line a marked event's place is on (PLACES); unknown kinds are the town's, on A. */
	SIGNAL47_API FString LineOfMark(const FString& Kind);

	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
