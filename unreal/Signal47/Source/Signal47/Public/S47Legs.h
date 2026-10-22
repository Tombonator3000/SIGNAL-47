#pragma once

#include "CoreMinimal.h"
#include "S47Drive.h"

// drive/legs.ts: the drives are driven the whole way (Tom, 5 October: no cuts when the truck goes
// from one place to another). Each place is still its own area far from the others in the scene;
// where two of them meet on a road, the truck is handed from one to the next between two frames,
// at a spot where both show the same road, with its speed, its wheel and the driver's head as
// they were. This file has the drivable ground of SARO and STATION 01 (the road and the old road
// have their own), and the hand-overs between the areas.
//
//   saro      the pad, the ramp, the service road, and the highway past SARO and down to
//             Sierra Motor Court's lot (650 m south) and a little past it
//   road      the highway south from there, and the survey track (S47RoadArea.h)
//   station01 the track's last stretch and the pad by STATION 01's gate
//
// A leg is named as in TypeScript: 'saro' | 'road' | 'station01' | 'diner' | 'roswell' | 'motel'.
namespace S47Legs
{
	/** Each area's origin (the diner stands in the road area and the motel in SARO's). */
	SIGNAL47_API FVector ORIGINS(const FString& Leg);
	struct FExit { FString To; double X = 0, Z = 0, Heading = 0; };

	/** SARO's highway: along x = -24, 8 m wide. */
	struct FSaroHwy { double X, Half, Z0, Z1; };
	inline constexpr FSaroHwy SARO_HWY = { -24, 4, -200, 820 };
	/** Where the truck goes from SARO's area onto the road's, going south (site z), and back from
	 *  the road's going north (road z). */
	SIGNAL47_API double SARO_TO_ROAD();
	SIGNAL47_API double ROAD_TO_SARO();
	/** Where the truck is left at the motel: the lot north of the office. */
	SIGNAL47_API const FS47Box2& MOTEL_PARK();
	inline constexpr FS47Box2 SARO_PAD = { 6.6, 13.0, 3.4, 12.4 };
	/** The truck changes from the road to the station this far along the road's track. */
	inline constexpr double HANDOFF = 330;

	/** The way out the truck has just taken from an area, if any: where it goes on in the next. */
	SIGNAL47_API TOptional<FExit> ExitFrom(const FString& Leg, double X, double Z, double Heading);
	/** The autopilot's way on the road (world), towards the next area. */
	SIGNAL47_API const TArray<FVector>& RoadRoute(const FString& Next, double Lz);
	/** The leg a place is in. */
	SIGNAL47_API FString LegOf(const FString& P);
	/** The legs a trip from one place to another passes through, in order. */
	SIGNAL47_API TArray<FString> LegsBetween(const FString& From, const FString& To);
	/** On a drive of the player's own, where the truck is bound by the way it is going; empty while
	 *  it could still go either way. */
	SIGNAL47_API FString BoundOn(const FString& Leg, double X, double Z, double Heading, bool bReversing);

	/** World/Station01.ts groundAt: the station's terrain grid (flat round the hut and the pad, the
	 *  road's land further out), world terms. */
	SIGNAL47_API double StationGroundAt(double X, double Z);
	/** World/Grounds.ts floorAt, as far as SaroDrive asks it: the pad and the truck ramp. */
	SIGNAL47_API double SaroFloorAt(double X, double Z);
}

/** SARO: the pad, the ramp, the service road, the highway. */
class SIGNAL47_API FS47SaroDrive : public IS47DriveArea
{
public:
	/** colliders: SARO's, without the truck's own; road: the posts, poles and rocks of the road's
	 *  stretch south of the site (road-local circles). */
	FS47SaroDrive(const TArray<FS47Box2>& Colliders, const TArray<FS47Obstacle>& Road);
	FS47Box2 MotelPark;
	virtual FS47Surface Surface(double X, double Z) const override;
	virtual bool HasHeight() const override { return true; }
	virtual double Height(double X, double Z) const override;
	virtual bool Blocked(double X, double Z) override;
};

/** STATION 01: the last of the track, and the pad by the gate. */
class SIGNAL47_API FS47StationDrive : public IS47DriveArea
{
public:
	/** colliders: the station's, without the truck's own; road: the big rocks by the road's
	 *  stretch of track (road-local circles). */
	FS47StationDrive(const TArray<FS47Box2>& Colliders, const TArray<FS47Obstacle>& Road);
	virtual FS47Surface Surface(double X, double Z) const override;
	virtual bool HasHeight() const override { return true; }
	virtual double Height(double X, double Z) const override;
	virtual bool Blocked(double X, double Z) override;
private:
	FVector O;
};
