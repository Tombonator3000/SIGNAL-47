#pragma once

#include "CoreMinimal.h"
#include "S47Drive.h"

// drive/OldRoad.ts, the drive's side of it: the old Roswell road at dawn, chapter six. The drive
// runs from the state highway across from Mesa Diner (mile 0) to C, just past the eight-mile
// post, 13.1 km. The area lies on the road area's map at the diner (oldRoadLayout.ts): this area's
// origin is where mile 0 comes onto the road area's highway centreline. Its meshes, lights and
// glints are in the exported scene; here are the queries the controller and the chapter ask:
// the surface, the height, what blocks the truck, where a point is along the road, the lane, C.
class SIGNAL47_API FS47OldRoad : public IS47DriveArea
{
public:
	/** roadOrigin: the road area's origin (World.ts ROAD_ORIGIN). */
	explicit FS47OldRoad(const FVector& RoadOrigin);

	struct FPose { FVector Pos = FVector::ZeroVector; double Heading = 0; };
	/** World pose for the truck at the start, in the right lane, facing the dawn. */
	FPose Start;
	/** World waypoints from the diner's lot across the highway and along the right lane to past C
	 *  (the autopilot, World.autoInput, and the coast to a stop): Routes["park"]. */
	TArray<FVector> Route;
	/** Set when the truck has run into the end of the open stretch of highway (the chapter says why). */
	bool bHitBack = false;
	/** The flood set's slots for the headlights (oldFlood, OLD_HEADLIGHTS). */
	static const TArray<int32>& OLD_HEADLIGHTS();

	/** A place by the road (the diner): its walls and posts hold the truck, its lot is gravel
	 *  with its top at world height y. */
	void AddPlace(const TArray<FS47Box2>& InObstacles, const FS47Box2& Lot, double Y);
	/** Where a world point is: distance along the road (s), signed offset (o) and miles from the
	 *  diner. Off the road (on the highway, on the diner's lot) it is mile 0. */
	struct FWhere { double S, O, D, Mi; };
	FWhere Where(double X, double Z) const;
	/** World height of the road where C crosses it. */
	double CrossY() const;
	/** World point in the right lane (o metres right of the centreline) at a distance s along the road. */
	FVector Lane(double S, double O = 1.6) const;
	/** The truck's pose in the right lane at a mile, facing the dawn (the developer menu). */
	FPose PoseAt(double Mi) const;
	/** Signed distance of a world point past C (positive once over the line, going north-east). */
	double PastC(double X, double Z) const;
	/** The local frame: world position of a local point. */
	FVector ToWorld(double X, double Y, double Z) const { return FVector(X + Origin.X, Y + Origin.Y, Z + Origin.Z); }

	virtual FS47Surface Surface(double X, double Z) const override;
	virtual bool HasHeight() const override { return true; }
	virtual double Height(double X, double Z) const override;
	/** The truck's centre may not leave the right of way: the old road's, or the highway's for a
	 *  little way either side of mile 0 (further, and the chapter says the bolt is ahead). */
	virtual bool Blocked(double X, double Z) override;

	FVector Origin;

private:
	struct FLot { FS47Box2 Box; double Y = 0; };
	TArray<FLot> Lots;
	const FLot* LotAt(double X, double Z) const;
};
