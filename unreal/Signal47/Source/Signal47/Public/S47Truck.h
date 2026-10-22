#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"

class US47Main;
class AS47Web;
class SS47Dash;
class FWidgetRenderer;
class UTextureRenderTarget2D;

// drive/Truck.ts, its behaviour: SARO 07, the observatory's service truck. Its meshes are in the
// exported scene (one copy per area, each lit by that area's floods: world.saroTruck on SARO's
// pad, world.legs.road.drive.truck on the road and at the diner, world.legs.station01.drive.truck
// at STATION 01, world.oldTruck on the old road); this moves them as Truck.ts does. Origin on the
// ground at the centre of the truck, forward is -Z, the driver sits left (-X).
//
//   Group   everything. Inside it:
//   Body    stays visible while driving: hood and fenders, bed, mirrors, wheels.
//   Shell   the cab from outside: hidden while driving (it would sit in front of the eye).
//   Cab     the interior. Shown only while driving.
//
// The group's rotation is three's Euler in the YXZ order (pitch, heading, roll), as the
// controller sets it (Drive.ts pose) and World.ts reads rotation.y as the heading.
struct SIGNAL47_API FS47Truck
{
	FS47Truck() = default;
	~FS47Truck();
	FS47Truck(const FS47Truck&) = delete;
	FS47Truck& operator=(const FS47Truck&) = delete;

	/** The truck's exported nodes and materials by its TypeScript path ("world.saroTruck"), its
	 *  cab's own flood set (an index of AS47Web's sets, -1 when unknown). Outer keeps its dash. */
	void Bind(US47Main* InM, const FString& InPath, int32 InCabSet, UObject* Outer);
	bool IsValid() const { return Group.IsValid(); }
	FString Path;

	FS47Obj Group, Body, Shell, Cab;
	/** The driver's eye, truck-local (left seat). */
	FVector DriverEye = FVector(-0.42, 1.48, 0.2);
	bool bDriving = false;

	// the group's pose: position, and rotation as three's Euler YXZ (x pitch, y heading, z roll)
	FVector Position = FVector::ZeroVector;
	FVector Rotation = FVector::ZeroVector;
	void SetPose(const FVector& P, const FVector& R);
	FQuat Quat() const;
	FVector LocalToWorld(const FVector& L) const;
	FVector WorldToLocal(const FVector& W) const;

	/** Where north is for the compass on the dash: the sky's turn in this area (World.skyYaw). */
	double CompassNorth = 0;
	/** Something pulls the compass card off north (story/truckOnB.ts): towards Deg, by K (0..1). */
	struct FPull { double Deg = 0, K = 0; };
	TOptional<FPull> CompassPull;
	/** The heading the compass on the dash shows now, under its lubber line (0 to 360). */
	int32 CompassReads() const;
	/** Where the compass ball is, in the world. */
	FVector CompassAt() const;
	/** The truck's heading on the map, degrees from north, clockwise. */
	double Bearing() const;

	/** Interior on, view-blocking shell off. The controller calls this while it drives. */
	void SetDriving(bool bOn);
	/** The camera has left the driver's seat (a cut): the whole truck again. */
	void ViewFrom(const FVector& CamThree);
	/** Take over another copy's needles and swinging keys (the truck goes on in another area). */
	void CopyCab(const FS47Truck& O);
	/** Dashboard values. Needles ease in Update(); the clock redraws at most 10 times a second. */
	void SetDash(double Mph, double Rpm, const FString& Clock, double Fuel);
	/** The radio's dial: the needle at x on the AM scale (canvas pixels), lit while it plays. */
	void SetRadio(double X, bool bOn);
	/** The odometer: the miles on it since the shift began (World.odometer), over MILES OUT. */
	void SetOdometer(double Tonight);
	/** The odometer as it reads now (miles, to the tenth). */
	double Odometer() const { return OdoShown / 10.0; }
	/** Frost on the windshield, 0..1 (story/truckTaken.ts). The frost decal is made at run time in
	 *  the web version and is not in the export: the value is kept, nothing is drawn yet. */
	void SetFrost(double K);
	double Frosted() const { return Frost; }
	/** A white light from straight over the truck, 0..1 (story/abduction.ts): the cab's sky slot. */
	void LightFromAbove(double K);
	/** -1 (full left) to 1 (full right). Turns the steering wheel. */
	void SetSteer(double V);
	void SetHeadlights(bool bOn);
	/** 0..1: headlights, tail lights and dash light together (0 is dark). */
	void SetLightLevel(double Level);
	/** How bright the headlights are now, 0..1. */
	double Beam() const { return bHeadOn ? LightLevel : 0; }
	void SetBrake(bool bOn);
	/** The headlights' colour (white when unset): the lamps, their glow and the pools on the ground. */
	void SetHeadColor(TOptional<FLinearColor> C);
	/** The instrument lights on their own, 0..1 (unset: they follow SetLightLevel). */
	void SetDashLevel(TOptional<double> K);
	/** The warning lamps on the cluster (drive/trouble.ts). */
	struct FWarn { bool bOil = false, bVolts = false, bBrake = false, bBelts = false; };
	void SetWarnings(const FWarn& W);
	/** Power to the electric gauges, 0..1: below 0.5 the fuel and temperature needles fall back. */
	void SetGaugePower(double K) { GaugePower = K; }
	/** Use these slots of a flood set as headlight pools on the ground (moved in Update()). */
	void HeadlightFloods(int32 Set, const TArray<int32>& Slots);
	/** How far each wheel hangs below its place on the body (fl, fr, rl, rr), in metres. */
	void SetWheelDrops(const double D[4]);
	/** Once per frame while the truck moves: wheels, needles, keys, headlight pools. */
	void Update(double Dt, double Speed, double WheelAngle = 0);
	/** Whether a ray (three.js world, from the eye) hits the radio's faceplate within Far metres
	 *  (main.ts radioAimed: truck.radioHit). */
	bool RadioHit(const FVector& From, const FVector& Dir, double Far = 1.6) const;
	/** The render target its dash is drawn into (US47World keeps it). */
	UTextureRenderTarget2D* DashTarget() const { return DashRT.Get(); }

private:
	US47Main* M = nullptr;
	AS47Web* Web = nullptr;
	FS47Obj Wheels, Needles, Keys, WheelSpin, CompassCard, GlowPoints;
	FS47Obj Lamps[4];
	FS47Mat MHead, MPark, MTail, MGauge, MNeedle, MCompass;
	TArray<TWeakObjectPtr<class UMaterialInstanceDynamic>> Glows; // the glow points' sprites, in their order
	TArray<int32> GiHead, GiPark, GiTail;
	double LightLevel = 1;
	bool bHeadOn = false, bBrakeOn = false;
	FLinearColor HeadColor;
	TOptional<double> DashLevel;
	double Spin = 0, WheelAngle = 0;
	double Drops[4] = { 0, 0, 0, 0 };
	struct FSwing { double A = 0, V = 0, Last = 0; } Swing;
	int32 FloodSet = -1;
	TArray<int32> FloodSlots;
	int32 CabSet = -1;
	FString ClockShown, ClockWanted;
	double ClockAt = -1;
	struct FGauge { double Speed = 0, Fuel = 0.6, Temp = 0.15, TSpeed = 0, TFuel = 0.6, TTemp = 0.5; } Gauge;
	double GaugePower = 1;
	struct FCompass { double A = 0, V = 0; bool bSet = false; } Compass;
	FString RadioShown = TEXT("214:false");
	int32 OdoShown = 0;
	double Frost = 0, Above = 0;
	FWarn WarnOn;

	// the dash canvas (truckTextures.ts dashAtlas): drawn with Slate into a render target
	TSharedPtr<SS47Dash> Dash;
	FWidgetRenderer* Renderer = nullptr;
	TWeakObjectPtr<UTextureRenderTarget2D> DashRT;
	bool bDashDirty = true;
	void DrawDash();

	double Now() const;
	void ApplyLights();
	void PlaceFloods();
	void FlushClock();
	void CompassStep(double Dt);
};

/** truckTextures.ts: the odometer when the shift began (the work order has it as MILES OUT). */
inline constexpr double S47_ODOMETER_OUT = 84217;
