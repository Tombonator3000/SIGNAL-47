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

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	bool bActive = false;
	FString Focused;
	float Settle = 0.f;
	void Resolve(bool bX, double Old);
	bool Inside(double MinX, double MaxX, double MinZ, double MaxZ, double X, double Z) const;
	double FloorAt(double X, double Z) const;
};
