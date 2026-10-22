#include "S47Walker.h"

#include "Camera/CameraComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "S47Three.h"
#include "S47Inter.h"
#include "S47Web.h"

using namespace S47Three;

AS47Walker::AS47Walker()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Eyes"));
	Camera->SetupAttachment(RootComponent);
	Camera->bConstrainAspectRatio = false;
}

void AS47Walker::Place(double X, double Z, double InYaw)
{
	// Player.place: the eye level drops onto the floor at once
	Pos = FVector(X, 0, Z);
	Yaw = InYaw;
	Pitch = -0.06;
	FloorY = FloorAt(X, Z);
	Settle = 0.5f;
}

void AS47Walker::Look(double DX, double DY)
{
	Yaw -= DX;
	Pitch = FMath::Clamp(Pitch - DY, -1.35, 1.25);
}

bool AS47Walker::Inside(double MinX, double MaxX, double MinZ, double MaxZ, double X, double Z) const
{
	const double R = Radius, E = 1e-6;
	return X >= MinX + R - E && X <= MaxX - R + E && Z >= MinZ + R - E && Z <= MaxZ - R + E;
}

bool AS47Walker::Walkable(double X, double Z) const
{
	if (!Web.IsValid()) return true;
	for (const FS47Rect& B : Web->Zones()) if (B.bEnabled && Inside(B.MinX, B.MaxX, B.MinZ, B.MaxZ, X, Z)) return true;
	return false;
}

void AS47Walker::Resolve(bool bX, double Old)
{
	// Player.resolve: slide along the edge of the zone stood in, then push out of colliders
	if (!Web.IsValid()) return;
	const double R = Radius;
	if (!Walkable(Pos.X, Pos.Z))
	{
		const double Want = bX ? Pos.X : Pos.Z;
		double Best = Old;
		for (const FS47Rect& B : Web->Zones())
		{
			if (!B.bEnabled) continue;
			const double Ox = bX ? Old : Pos.X, Oz = bX ? Pos.Z : Old;
			if (!Inside(B.MinX, B.MaxX, B.MinZ, B.MaxZ, Ox, Oz)) continue;
			const double V = bX ? FMath::Clamp(Want, B.MinX + R, B.MaxX - R) : FMath::Clamp(Want, B.MinZ + R, B.MaxZ - R);
			if (FMath::Abs(V - Want) < FMath::Abs(Best - Want)) Best = V;
		}
		if (bX) Pos.X = Best; else Pos.Z = Best;
	}
	for (const FS47Rect& C : Web->Colliders())
	{
		if (Pos.X + R <= C.MinX || Pos.X - R >= C.MaxX || Pos.Z + R <= C.MinZ || Pos.Z - R >= C.MaxZ) continue;
		if (bX)
		{
			const double Left = Pos.X + R - C.MinX, Right = C.MaxX - (Pos.X - R);
			Pos.X += Left < Right ? -Left : Right;
		}
		else
		{
			const double Front = Pos.Z + R - C.MinZ, Back = C.MaxZ - (Pos.Z - R);
			Pos.Z += Front < Back ? -Front : Back;
		}
	}
}

double AS47Walker::FloorAt(double X, double Z) const
{
	// the highest surface under the player, up to a stair step over the current floor (a desk
	// or a crate is higher: the web version's floor is a height per area, not the furniture)
	FHitResult Hit;
	FCollisionQueryParams P(SCENE_QUERY_STAT(S47Floor), true, this);
	const FVector From = ToUE(X, FloorY + 0.3, Z), To = ToUE(X, FloorY - 4.0, Z);
	if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, P)) return Hit.ImpactPoint.Z / 100.0;
	return FloorY;
}

FVector AS47Walker::EyeUE() const { return Camera->GetComponentLocation(); }
FVector AS47Walker::ForwardUE() const { return Camera->GetForwardVector(); }

void AS47Walker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const double Dt = DeltaSeconds;
	APlayerController* PC = Cast<APlayerController>(GetController());
	Focused.Reset();
	if (PC && bActive)
	{
		float DX = 0, DY = 0;
		PC->GetInputMouseDelta(DX, DY);
		if (Settle > 0.f) { Settle -= DeltaSeconds; DX = DY = 0.f; }
		// Input.ts: 0.0022 rad per pixel; Unreal's mouse delta is in pixels times its own scale,
		// and up is positive, where the browser's movementY grows downwards
		// (the settings' look speed and invert, main.ts step: player.look(look.x, invertY ? -look.y : look.y))
		Look(FMath::Clamp(DX, -200.f, 200.f) * 0.0022 * 4.0 * Sensitivity, (bInvertY ? 1.0 : -1.0) * FMath::Clamp(DY, -200.f, 200.f) * 0.0022 * 4.0 * Sensitivity);
		double Mx = 0, Mz = 0;
		if (PC->IsInputKeyDown(EKeys::W) || PC->IsInputKeyDown(EKeys::Up)) Mz -= 1;
		if (PC->IsInputKeyDown(EKeys::S) || PC->IsInputKeyDown(EKeys::Down)) Mz += 1;
		if (PC->IsInputKeyDown(EKeys::A) || PC->IsInputKeyDown(EKeys::Left)) Mx -= 1;
		if (PC->IsInputKeyDown(EKeys::D) || PC->IsInputKeyDown(EKeys::Right)) Mx += 1;
		const double L = FMath::Sqrt(Mx * Mx + Mz * Mz);
		if (L > 1) { Mx /= L; Mz /= L; }
		const bool bRun = PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift);
		// Player.update
		const double Sp = Speed * (bRun ? 1.6 : 1.0);
		const double S = FMath::Sin(Yaw), C = FMath::Cos(Yaw);
		const double Vx = (Mx * C + Mz * S) * Sp * Dt, Vz = (-Mx * S + Mz * C) * Sp * Dt;
		const FVector Before = Pos;
		Pos.X += Vx; Resolve(true, Before.X);
		Pos.Z += Vz; Resolve(false, Before.Z);
		const double Moved = FMath::Sqrt(FMath::Square(Pos.X - Before.X) + FMath::Square(Pos.Z - Before.Z));
		if (Moved > 0.0005)
		{
			Bob += Moved * 7.2;
			StepDist += Moved;
			if (StepDist > 0.8) { StepDist = 0; if (OnStep) OnStep(); }
		}
		else Bob *= 0.9;
	}
	const double Fy = FloorAt(Pos.X, Pos.Z);
	FloorY += (Fy - FloorY) * FMath::Min(1.0, Dt * 14.0);
	Shake = FMath::Max(0.0, Shake - Dt * 1.4);
	const double Sh = Shake * Shake;
	const FVector EyeThree(Pos.X + (FMath::FRand() - 0.5) * Sh * 0.08, FloorY + Eye + FMath::Sin(Bob) * 0.018 + (FMath::FRand() - 0.5) * Sh * 0.06, Pos.Z + (FMath::FRand() - 0.5) * Sh * 0.08);
	SetActorLocationAndRotation(ToUE(EyeThree), CameraRotation(Yaw, Pitch + (FMath::FRand() - 0.5) * Sh * 0.02, FMath::Sin(Bob * 0.5) * 0.004));
	FVector2D Size(16, 9);
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(Size);
	Camera->SetFieldOfView(HorizontalFov(Fov, float(Size.X / FMath::Max(1.0, Size.Y))));
	if (Web.IsValid()) Web->SetPlayerEye(ToUE(EyeThree));

	// Interaction.update: what the line of sight picks, within its range, with something to offer
	if (!bActive || !Inter.IsValid()) return;
	Focused = Inter->Update(Camera->GetComponentLocation(), Camera->GetForwardVector());
	if (!Focused.IsEmpty() && OnUse && PC && (PC->WasInputKeyJustPressed(EKeys::E) || PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)))
	{
		OnUse(Focused);
	}
}
