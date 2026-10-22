#pragma once

#include "CoreMinimal.h"

struct FS47Truck;

// drive/Drive.ts: how the pieces fit together.
//
//   FS47Truck (S47Truck.h)        the SARO 07 pickup as the export has it: the exterior, the cab,
//                                 the lights, the dash and the wheels. It knows nothing about roads.
//   FS47DriveController (here)    arcade physics (kinematic bicycle), collisions, the first-person
//                                 camera at the truck's driverEye. Each Update() it places the
//                                 truck's group, calls its Update(), SetSteer(), SetBrake() and
//                                 SetDriving(true).
//   the areas (S47Legs.h)         SARO's ground, the road (RoadArea), STATION 01, the old road:
//                                 each answers Surface(x, z), Height(x, z), Obstacles, EndZone and
//                                 Blocked (IS47DriveArea below), with its headlight flood slots.
//   FS47EngineSound               the engine, tyres, cattle guard rattle and stall.
//
// Input: throttle 1 accelerates, -1 brakes and, once stopped, reverses (up to 5 m/s); positive
// steer turns right. Look: the mouse turns the head (yaw within 110 degrees of straight ahead,
// pitch clamped); driving forward eases the view back to the road. The camera is written to
// CamPos and CamQuat (three.js world terms), where Drive.ts writes camera.position and
// camera.quaternion; the walker shows it (AS47Walker::CameraOverride, set by US47World).

enum class ES47SurfaceKind : uint8 { Asphalt, Gravel, Dirt };
SIGNAL47_API const TCHAR* S47SurfaceName(ES47SurfaceKind K); // 'asphalt' | 'gravel' | 'dirt'

/** grip 0..1 scales acceleration, braking and cornering; top is the top speed in m/s; rough 0..1
 *  drives bumps and vibration (negative: unset, a default follows the kind). */
struct FS47Surface
{
	ES47SurfaceKind Kind = ES47SurfaceKind::Asphalt;
	double Grip = 1, Top = 30, Rough = -1;
};

struct FS47Box2
{
	double MinX = 0, MaxX = 0, MinZ = 0, MaxZ = 0;
	bool Contains(double X, double Z) const { return X >= MinX && X <= MaxX && Z >= MinZ && Z <= MaxZ; }
};

/** A circle (posts, poles, rocks) or a box (walls, fences), world terms. */
struct FS47Obstacle
{
	bool bCircle = false;
	double X = 0, Z = 0, R = 0;
	double MinX = 0, MaxX = 0, MinZ = 0, MaxZ = 0;
	static FS47Obstacle Circle(double X, double Z, double R) { FS47Obstacle O; O.bCircle = true; O.X = X; O.Z = Z; O.R = R; return O; }
	static FS47Obstacle Box(double MinX, double MaxX, double MinZ, double MaxZ) { FS47Obstacle O; O.MinX = MinX; O.MaxX = MaxX; O.MinZ = MinZ; O.MaxZ = MaxZ; return O; }
};

struct FS47DriveIn { double Steer = 0, Throttle = 0; };

/** Drive.ts DriveArea, with legs.ts TripArea's park and routes. */
class SIGNAL47_API IS47DriveArea
{
public:
	virtual ~IS47DriveArea() = default;
	virtual FS47Surface Surface(double X, double Z) const = 0;
	/** World height of the ground; flat ground at y = 0 if left out (HasHeight false). */
	virtual bool HasHeight() const { return false; }
	virtual double Height(double X, double Z) const { (void)X; (void)Z; return 0; }
	TArray<FS47Obstacle> Obstacles;
	FS47Box2 EndZone;
	/** true where the truck's centre may not go (a curved right of way; the old road). */
	virtual bool Blocked(double X, double Z) { (void)X; (void)Z; return false; }
	/** Flood slots for the headlight pools; the controller hands them to the truck. */
	struct FHeadlights { int32 Set = -1; TArray<int32> Slots; };
	TOptional<FHeadlights> Headlights;
	/** legs.ts TripArea: where the truck is left when it arrives here (world), and waypoints for
	 *  the autopilot towards a place (world). */
	TOptional<FS47Box2> Park;
	TMap<FString, TArray<FVector>> Routes;
};

class SIGNAL47_API FS47DriveController
{
public:
	FS47DriveController(FS47Truck* InTruck, IS47DriveArea* InArea);

	/** World position of the truck's centre on the ground. */
	FVector Pos = FVector::ZeroVector;
	/** Radians, like rotation.y: 0 faces -Z (north), PI/2 faces -X. */
	double Heading = 0;
	/** m/s along the heading, negative in reverse. */
	double Speed = 0;
	double Mph = 0;
	double Rpm;
	int32 Gear = 1;
	/** 0..1, how hard the engine works (throttle); for the engine sound. */
	double Load = 0;
	/** The surface under the truck now. */
	FS47Surface Surface;
	/** false: the engine has died. The truck coasts, brakes and steers, and rpm falls to 0. */
	bool bEngine = true;
	/** 0..1, how much of its pull the engine gives (a misfire, drive/trouble.ts); 1 normally. */
	double Power = 1;
	/** m/s² of something holding the truck back (story/truckTaken.ts); 0 normally. */
	double Held = 0;
	/** The head's look kept within this (radians; the waking in the cab, story/abduction.ts), or unset. */
	struct FLookBox { double Yaw0, Yaw1, Pitch0, Pitch1; };
	TOptional<FLookBox> LookBox;
	/** The head's look now, from straight ahead (yaw, left positive) and level (pitch, up positive). */
	FVector2D Look() const { return FVector2D(Yaw, Pitch); }
	/** Turn the head to this look at once, the camera with it in this frame. */
	void SetLook(double InYaw, double InPitch);
	TFunction<void()> OnArrive;
	TFunction<void(double)> OnBump;
	/** In reverse gear (S held at a stop). */
	bool Reversing() const { return bReverse; }

	/** The camera: where Drive.ts puts camera.position and camera.quaternion (three.js world). */
	FVector CamPos = FVector::ZeroVector;
	FQuat CamQuat = FQuat::Identity;

	FS47Truck* Truck = nullptr;
	IS47DriveArea* Area = nullptr;

	/** The steer input (-1 to 1) that bends the centre's path to curvature kappa (1/m, positive to
	 *  the right, as steer is) at the speed and on the ground the truck has now; clamped at full lock. */
	double SteerFor(double Kappa) const;
	/** Put the truck at a world position and heading, standing still, view straight ahead. */
	void Place(const FVector& P, double InHeading);
	/** Go on from another controller's truck in another area (the hand-over between two areas on a
	 *  drive): this truck at (x, z, heading), moving and steering as that one was. */
	void Adopt(const FS47DriveController& O, double X, double Z, double InHeading);
	void Update(double Dt, const FS47DriveIn& Input, const FVector2D& LookIn);

	static double LockAt(double InSpeed, double Grip);
	static constexpr double IDLE = 650;

private:
	double Steer = 0;
	bool bReverse = false;
	double ShiftHold = 0;
	double Yaw = 0, Pitch, Idle = 0;
	bool bArrived = false;
	double BumpT = 0;
	double T = 0;
	double Accel = 0;
	double YawRate = 0;
	struct FBody { double Y = 0, VY = 0, Pitch = 0, VP = 0, Roll = 0, VR = 0; } Body;
	struct FGround { double Y = 0, Pitch = 0, Roll = 0; double At[4] = { 0, 0, 0, 0 }; };
	FGround Ground() const;
	bool Hit() const;
	void Pose(double Dt, double WheelAngle, bool bSettle = true);
};

namespace S47Drive
{
	// the module's self tests (the -S47SelfTest block in S47HUD.cpp): the drive's maths against
	// numbers the web version gives for the same questions
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
