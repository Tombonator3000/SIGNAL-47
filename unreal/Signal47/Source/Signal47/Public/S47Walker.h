#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "S47Bodycam.h"
#include "S47Walker.generated.h"

class UCameraComponent;
class AS47Web;

// Where the eye is when something else holds the camera (the truck's cab, World.ts drivingStep):
// three.js terms, the rotation as the walker's own (Euler YXZ: yaw, pitch, roll).
struct FS47View
{
	FVector Eye = FVector::ZeroVector;
	double Yaw = 0, Pitch = 0, Roll = 0;
};

// The player, as web/src/player/Player.ts has it: a point in the web version's space (metres,
// y up) that walks inside the union of the area's walk zones minus its colliders, with the
// eye 1.62 m over the floor and the same bob, speed and look. The floor under the player is
// read off the world itself (a trace), where the web version has a height function per area.
// The camera is placed from that, converted to Unreal; nothing here uses Unreal's physics.
UCLASS()
class SIGNAL47_API AS47Walker : public APawn
{
	GENERATED_BODY()

public:
	AS47Walker();
	virtual void Tick(float DeltaSeconds) override;

	void SetActive(bool bOn) { bActive = bOn; }
	bool IsActive() const { return bActive; }
	void Place(double X, double Z, double InYaw);
	// Normal on-foot movement and native scenarios share this exact axis-swept
	// collision path. Deltas are world X/Z metres, not input or teleportation.
	void MoveBy(double DeltaX, double DeltaZ);
	/** On the floor under the walker at once, where it stands; moving, it follows the floor over a moment. */
	void SettleOnFloor() { FloorY = FloorAt(Pos.X, Pos.Z); }
	void Look(double DX, double DY);
	FString FocusedId() const { return Focused; }
	TFunction<void(const FString&)> OnUse;
	TFunction<void()> OnStep;
	TWeakObjectPtr<AS47Web> Web;
	TWeakObjectPtr<class US47Inter> Inter; // what the line of sight picks (core/Interaction.ts)

	// three.js terms
	FVector Pos = FVector::ZeroVector;
	FVector MoveFrom = FVector::ZeroVector;   // where the move now being tried began: while it is tried, Pos is the place being asked about
	double Yaw = 0, Pitch = -0.06, Eye = 1.62, Radius = 0.27, Speed = 1.9, Bob = 0, StepDist = 0, Shake = 0, FloorY = 0;
	float Fov = 70.f; // three's vertical field of view
	float FovScale = 1.f; // times Fov at the camera: the stress model narrows the view a little (S47Stress.h); 1 when calm
	double Sensitivity = 1.0; // Input.sensitivity (Look speed)
	float PadLookSpeed = 1.f;
	bool bPadInvertY = false;
	bool bInvertY = false;    // Invert look
	bool bHeadMotion = true; // Walking bob, landing dip and shake; also read by the driving camera.
	double LookFine = 1.0;    // main.ts: 1 - 0.6 zoomK, a finer aim through the closer look

	// The camera held by something else (US47World while the truck is driven): given the frame's
	// step, the keys as on foot (move x right, z back: main.ts input.move) and the mouse's look
	// (radians, as Look takes them; zero while the walker is not active), it says where the eye is
	// and returns true; the walker then neither walks nor picks. False: the walker is itself.
	TFunction<bool(double Dt, const FVector2D& Move, const FVector2D& LookIn, FS47View& Out)> CameraOverride;

	FVector EyeUE() const;
	FVector ForwardUE() const;
	bool Walkable(double X, double Z) const;
	TFunction<double(double, double)> Floor; // Player.floor: the area supplies its exact height function (NaN: read it off the world there)

	// Ground round the truck where it was left with no walk zones of its area by it (US47World::
	// SetGround, Tom 22 October 2026: out of the truck anywhere; Player.ts walks only its zones): the
	// area it is in, and whether a point is on it. Walked as the zones are, in that area only.
	FString GroundArea;
	TFunction<bool(double X, double Z)> Ground;
	// The open land of the area the player is in (US47World::OnLand, Tom 22 October 2026: walk round
	// freely out there; Player.ts walks only its zones): walked as the zones are, the colliders as ever.
	TFunction<bool(double X, double Z)> OpenLand;
	// The ground's height on that open land (NaN off it, or in a zone: the area's Floor as before).
	TFunction<double(double X, double Z)> LandFloor;
	// The player's truck standing in an area, as it is turned (US47World::ParkAt). The walk lists keep
	// the box round it (a collider with the id 'truck', Player.ts's only way), which is far bigger
	// than the truck when it stands at an angle; the walker keeps to the truck itself instead.
	struct FTruckBox { FString Area; double X = 0, Z = 0, Heading = 0, HalfW = 1.05, HalfL = 2.75; };
	TOptional<FTruckBox> TruckBox;
	TOptional<FTruckBox> PersonalCarBox;
	bool InTruck(double X, double Z) const; // the player's width at X, Z is in the truck

	// A jump (Tom 22 October 2026; Player.ts has none): the Jump key on foot, when CanJump says it
	// may (not in the truck, not with a panel open or the camera raised: Space is its shutter then).
	// About 0.45 m up under 9.8 m/s², down onto the floor the walker uses, a dip of the eye on landing
	// and a footstep at each end. The walk zones and colliders hold as on the ground.
	void Jump();
	TFunction<bool()> CanJump;
	bool InAir() const { return bAir; }
	double Lift = 0; // the feet over the floor in a jump, metres

	// SNEAK-20261108: keep low (the Crouch key; the numbers are S47Sneak.h's). bCrouch is what was asked for, CrouchK the body
	// getting there, 0 standing to 1 down. Not in the truck or in the air; running or jumping stands the player up, and so does
	// being put somewhere (Place). Nothing of it is saved.
	bool bCrouch = false;
	double CrouchK = 0;
	void SetCrouch(bool bOn);
	double Sneak() const { return CrouchK; }
	/** A footstep's loudness now, 1 standing (US47Audio::StepScale is set from it). */
	double StepLoudness() const;

	// tests: walk as if these keys were held (x right, z back: main.ts input.move), or unset; and run
	TOptional<FVector2D> TestMove;
	double MoveScale = 1; // legs going heavy: held in the light
	double TiredWalk = 1; // D46-20261110: a tired walk is a little slower (FS47Fatigue::WalkScaleOf), never the run's business
	double RunScale = 1; // FATIGUE-20261108: how much of the run is left when tired (FS47Fatigue); the run is 1 + 0.6 times this
	bool bTestRun = false;

	// D45-20261110 (D45-19): BODYCAM 1986 on the chest (S47Bodycam.h). Only the camera component moves:
	// the head (Web->SetPlayerEye), the walk and the floor stay as they are. US47WorldLife reads Last().
	FS47Bodycam Bodycam;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	bool bActive = false;
	FString Focused;
	float Settle = 0.f;
	bool bAir = false;
	double LiftV = 0, AirT = 0, Top = 0, LandT = -1, LandK = 0;
	void Land(bool bSilent);
	void Resolve(bool bX, double Old);
	bool Inside(double MinX, double MaxX, double MinZ, double MaxZ, double X, double Z) const;
	double FloorAt(double X, double Z) const;
	// steps the chest camera and places the camera component; returns the field of view factor
	float StepBodycam(double Dt, bool bHeld, double Turn, double Tilt, double Strafe, bool bRunning, bool bStepped);
};
