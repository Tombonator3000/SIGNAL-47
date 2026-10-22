#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
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
	void Look(double DX, double DY);
	FString FocusedId() const { return Focused; }
	TFunction<void(const FString&)> OnUse;
	TFunction<void()> OnStep;
	TWeakObjectPtr<AS47Web> Web;
	TWeakObjectPtr<class US47Inter> Inter; // what the line of sight picks (core/Interaction.ts)

	// three.js terms
	FVector Pos = FVector::ZeroVector;
	double Yaw = 0, Pitch = -0.06, Eye = 1.62, Radius = 0.27, Speed = 1.9, Bob = 0, StepDist = 0, Shake = 0, FloorY = 0;
	float Fov = 70.f; // three's vertical field of view
	double Sensitivity = 1.0; // Input.sensitivity (Look speed)
	bool bInvertY = false;    // Invert look
	double LookFine = 1.0;    // main.ts: 1 - 0.6 zoomK, a finer aim through the closer look

	// The camera held by something else (US47World while the truck is driven): given the frame's
	// step, the keys as on foot (move x right, z back: main.ts input.move) and the mouse's look
	// (radians, as Look takes them; zero while the walker is not active), it says where the eye is
	// and returns true; the walker then neither walks nor picks. False: the walker is itself.
	TFunction<bool(double Dt, const FVector2D& Move, const FVector2D& LookIn, FS47View& Out)> CameraOverride;

	FVector EyeUE() const;
	FVector ForwardUE() const;
	bool Walkable(double X, double Z) const;
	TFunction<double(double, double)> Floor; // Player.floor: the area supplies its exact height function

	// Ground round the truck where it was left with no walk zones of its area by it (US47World::
	// SetGround, Tom 22 October 2026: out of the truck anywhere; Player.ts walks only its zones): the
	// area it is in, and whether a point is on it. Walked as the zones are, in that area only.
	FString GroundArea;
	TFunction<bool(double X, double Z)> Ground;
	// The player's truck standing in an area, as it is turned (US47World::ParkAt). The walk lists keep
	// the box round it (a collider with the id 'truck', Player.ts's only way), which is far bigger
	// than the truck when it stands at an angle; the walker keeps to the truck itself instead.
	struct FTruckBox { FString Area; double X = 0, Z = 0, Heading = 0, HalfW = 1.05, HalfL = 2.75; };
	TOptional<FTruckBox> TruckBox;
	bool InTruck(double X, double Z) const; // the player's width at X, Z is in the truck

	// A jump (Tom 22 October 2026; Player.ts has none): the Jump key on foot, when CanJump says it
	// may (not in the truck, not with a panel open or the camera raised: Space is its shutter then).
	// About 0.45 m up under 9.8 m/s², down onto the floor the walker uses, a dip of the eye on landing
	// and a footstep at each end. The walk zones and colliders hold as on the ground.
	void Jump();
	TFunction<bool()> CanJump;
	bool InAir() const { return bAir; }
	double Lift = 0; // the feet over the floor in a jump, metres

	// tests: walk as if these keys were held (x right, z back: main.ts input.move), or unset
	TOptional<FVector2D> TestMove;

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
};
