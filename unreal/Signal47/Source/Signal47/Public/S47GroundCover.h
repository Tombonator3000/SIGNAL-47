#pragma once

#include "CoreMinimal.h"
#include "S47Flora.h"

// world/groundCover.ts: where the desert's ground cover (S47Flora) grows in each area, and how thick:
// none on the roads, the tracks, the paths, the lots and the floors, thin at their edges, thick a
// metre or two out (where the grader leaves its windrow, in the ditches), along the fences and at
// the feet of walls, posts and poles, and only the low kinds where the player walks.
//
//   SARO        the site, the antenna field and the highway down to the motel and past it, one
//               patch in the road's terms: north of site z 80 the site's own rules, south of it the road's
//   the road    the highway from SARO to past Caliche, the survey track, the pasture, the diner's
//               and the town's surroundings
//   STATION 01  round the station (its own rules, Station01.clear) and the last of the track (the
//               road's), one patch in the road's terms turned into the station's
//   the old road   its own terms (OldRoad.ts builds it: its spec is here too)
//
// The same seed and the same rules give the same plants: where the truck goes from one area into
// the next, the cover beside it does not change. Colliders, walk zones and posts come from the
// export (the walks, the drive's legs), in world terms; each spec takes them into its own.
namespace S47GroundCover
{
	inline constexpr int32 COVER_SEED = 1986;

	/** The antenna field's gravel (arrayLayout.ts FIELD_ZONES: the walk zones fSpine, fRow*, fApron:*,
	 *  fSpur:* of SARO's walks), for onField and ARRAY_FLAT. Set before any SARO spec is made. */
	SIGNAL47_API void SetField(const TArray<FS47FloraRect>& Zones);

	SIGNAL47_API double RoadCover(double X, double Z);
	SIGNAL47_API double RoadSeam(double X, double Z);
	SIGNAL47_API double RoadSpill(double X, double Z);
	SIGNAL47_API FString RoadWays(double X, double Z);
	SIGNAL47_API double SiteCover(double X, double Z);
	SIGNAL47_API double SiteSeam(double X, double Z);
	SIGNAL47_API double SiteSpill(double X, double Z);
	SIGNAL47_API FString SiteWays(double X, double Z);
	/** world/Exterior.ts saroLand: SARO's own ground at a site point. */
	SIGNAL47_API double SaroLand(double X, double Z);

	/** A thing beside the road that holds the truck, as a block for the cover (circles only). */
	struct FObstacle { bool bCircle = true; double X = 0, Z = 0, R = 0, MinX = 0, MaxX = 0, MinZ = 0, MaxZ = 0; };
	/** postsOf: the posts, the poles, the signs and the big rocks (not the long walls), moved by -dx, -dz. */
	SIGNAL47_API TArray<FS47FloraRect> PostsOf(const TArray<FObstacle>& Obstacles, double Dx = 0, double Dz = 0);

	/** The road area's patch (road-local); blocks: the posts (and later the diner's and the town's walls). */
	SIGNAL47_API FS47FloraSpec RoadFlora(const TArray<FS47FloraRect>& Posts);
	/** SARO's patch in the road's terms: SARO's colliders and walks (site terms; the walks without the highway's). */
	SIGNAL47_API FS47FloraSpec SaroFlora(const TArray<FS47FloraRect>& Colliders, const TArray<FS47FloraRect>& Zones);
	/** STATION 01's patch in the road's terms; the station's colliders and zones in world terms, its origin. */
	SIGNAL47_API FS47FloraSpec StationFlora(const TArray<FS47FloraRect>& Colliders, const TArray<FS47FloraRect>& Zones, const FVector& Origin, const TArray<FS47FloraRect>& Posts);
	/** drive/OldRoad.ts's patch, in the old road's local terms (posts: what stands, local). */
	SIGNAL47_API FS47FloraSpec OldRoadFlora(const TArray<FS47FloraRect>& Posts);
	/** world/Station01.ts clear(x, z, rad): whether a station-local point is clear of everything there. */
	SIGNAL47_API bool StationClear(double X, double Z, double Rad);
}
