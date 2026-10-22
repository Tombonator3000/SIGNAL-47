#pragma once

#include "CoreMinimal.h"
#include "S47RoadShape.h"

// drive/roadTerrain.ts, the maths of it: the layout of the drive in metres local to the road
// area (whose origin is the start), the survey track, the first stretch of the old Roswell road
// across from the diner, and the height of whatever is on top at a point. North is -Z. The state
// highway runs north-south along x = 0. The meshes it builds are in the exported scene; the
// tables the TypeScript works out when the module loads (the track's points, the desert grid)
// are worked out the same way here on first use.
// Also drive/corridors.ts's two numbers (SARO_FROM, STATION_FROM): the rest of that file builds
// meshes, and the posts and rocks it gives the drive come with the export (S47Legs.h).
namespace S47RoadTerrain
{
	using namespace S47RoadShape;

	struct FHwy { double Paved, Shoulder, Flat, Margin, Fence, Z0, Z1; };
	inline constexpr FHwy HWY = { HWY_W.Paved, HWY_W.Shoulder, 16.5, 18, 21, -2000, 3400 };
	struct FTrk { double Half, Soft, Flat, Margin; };
	inline constexpr FTrk TRK = { 2.3, 0.7, 11.5, 13 };
	struct FGuard { double X0, X1, Z0, Z1; };
	inline constexpr FGuard GUARD = { -22.2, -19.8, 517.6, 522.4 };
	struct FGate { double X, Z, Half; };
	inline constexpr FGate GATE = { -612, 532, 5 };                 // closed double gate, station fence
	struct FEndZone { double MinX, MaxX, MinZ, MaxZ; };
	inline constexpr FEndZone ENDZONE = { -609, -585, 524, 540 };
	struct FPasture { double West, North, South; };
	inline constexpr FPasture PASTURE = { -760, 440, 720 };           // (north: south of the motel)
	inline constexpr double DRIVE_N = -120, DRIVE_S = 2700;          // invisible ends of the highway (past the diner)
	/** Event 12: where the deputy's car stands on the east shoulder north of Caliche. */
	struct FDeputyCar { double X, Z; };
	inline constexpr FDeputyCar DEPUTY_CAR = { 6.9, 1690 };
	/** The road's own ground walked on foot, where the truck can be left by something to walk to:
	 *  road-local walk zones, each with the place shown in the corner of the screen. */
	struct FRoadStop { const TCHAR* Id; const TCHAR* Name; double MinX, MaxX, MinZ, MaxZ; };
	SIGNAL47_API const TArray<FRoadStop>& ROAD_STOPS();

	// ---------- the survey track: a smooth curve through hand-placed points, every 3 m ----------
	struct FTrack { TArray<FVector2D> Pts; TArray<double> S; double Length = 0; };
	SIGNAL47_API const FTrack& Track();
	/** The mouth by the highway is wider, so the turn can be made at a crawl. */
	SIGNAL47_API double TrackHalf(double S);
	struct FNear { double D, S; };
	/** Distance to the track centreline and the arc length there (d Infinity far from it). */
	SIGNAL47_API FNear TrackNearest(double X, double Z);

	// ---------- the old Roswell road's first stretch, off the highway across from the diner ----------
	struct FOld { double Half, Shoulder, Flat, Margin, Len; };
	inline constexpr FOld OLD = { 3.2, 4.4, 9, 11, 640 };
	struct FStubPt { double X, Z, Tx, Tz, S; };
	SIGNAL47_API const TArray<FStubPt>& OldStub();
	/** Distance to the old road's centreline here (Infinity off this stretch, or behind its start). */
	SIGNAL47_API FNear OldNearest(double X, double Z);

	// ---------- heights ----------
	/** The desert grid's axes and its heights (Float32Array in the TypeScript: float here too). */
	SIGNAL47_API const TArray<double>& GX();
	SIGNAL47_API const TArray<double>& GZ();
	/** The grid surface exactly as drawn (same triangle split as groundGeometry). */
	SIGNAL47_API double GridY(double X, double Z);
	/** Where the wheels have worn the track down (fractions of the way out, and the depth). */
	inline constexpr double RUT[3] = { 0.28, 0.14, 0.045 };
	/** Height of whatever is on top at (x, z): road, track, verge or desert. */
	SIGNAL47_API double SurfaceY(double X, double Z);

	// ---------- the ranch's road ----------
	SIGNAL47_API double RanchZ(double X);
	inline constexpr double RANCH_TO = 405;
	SIGNAL47_API bool OnRanchRoad(double X, double Z);

	// ---------- drive/corridors.ts ----------
	/** Road-local z where the site's own highway ends and SARO's stretch of the road begins (site z 36). */
	inline constexpr double SARO_FROM = -264;
	/** Arc length along the survey track where the station's stretch begins. */
	inline constexpr double STATION_FROM = 200;
}
