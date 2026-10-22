#pragma once

#include "CoreMinimal.h"
#include "S47Drive.h"
#include "S47Obj.h"

// drive/RoadArea.ts, the drive's side of it: the highway south from SARO's stretch past the
// motel to Caliche, and the survey track west to the STATION 01 gate, around (8000, 0, 0). What
// it builds (the desert, the road, the posts and the lamps) is in the exported scene; what the
// drive asks of it is here: the surface and the height at a world point, the obstacles, the end
// zone, the lots, the start and end poses, the cattle guard. The obstacles the builders made (the
// posts, poles, fences, rocks, the gate, the cow that goes, the diner's and the town's walls) are
// the export's (US47World fills Obstacles from it).
class SIGNAL47_API FS47RoadArea : public IS47DriveArea
{
public:
	explicit FS47RoadArea(const FVector& InOrigin);

	struct FPose { FVector Pos = FVector::ZeroVector; double Heading = 0; };
	/** World pose to place the truck at: southbound lane, SARO and the motel behind, the track ahead. */
	FPose Start;
	/** World pose of the truck parked at the station gate. */
	FPose End;
	/** World waypoints of the way to the gate: the southbound lane, then the track. */
	TArray<FVector> Route;
	/** The road's own ground walked on foot where the truck can be left by something to walk to
	 *  (roadTerrain.ROAD_STOPS), world terms. */
	TArray<FS47Box2> Walks;
	/** Once per crossing of the cattle guard, with the truck's speed in m/s (play a rattle). */
	TFunction<void(double)> OnCattleGuard;
	/** The barricades across the old road (closed until chapter five sends the truck out on it). */
	FS47Obj ClosedGroup;

	/** A place beside the road (the diner): its walls and posts hold the truck, its lot is gravel
	 *  with its top at world height y. */
	void AddPlace(const TArray<FS47Box2>& InObstacles, const FS47Box2& Lot, double Y);
	/** More things beside the road that hold the truck (Caliche's buildings and posts). */
	void AddObstacles(const TArray<FS47Obstacle>& List) { Obstacles.Append(List); }
	bool RoadClosed() const { return bClosedOn; }
	void SetRoadClosed(bool bOn);

	virtual FS47Surface Surface(double X, double Z) const override;
	virtual bool HasHeight() const override { return true; }
	virtual double Height(double X, double Z) const override;
	/** Every frame while the area is shown: the cattle guard rattles while the front axle is on it. */
	void Update(double Dt, double T, const FVector& TruckPos);

	FVector Origin;
	struct FLot { FS47Box2 Box; FS47Surface Surface; double Y = 0; };
	TArray<FLot> Lots;

private:
	FVector Last = FVector::ZeroVector;
	bool bHaveLast = false;
	bool bOnGuard = false;
	bool bClosedOn = true;
	const FLot* LotAt(double X, double Z) const;
};
