#include "S47Walker.h"

#include "Camera/CameraComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "S47Three.h"
#include "S47Inter.h"
#include "S47Keys.h"
#include "S47Gamepad.h"
#include "S47Web.h"
#include "S47Props.h"
#include "S47Relief.h"
#include "S47SubstationDetail.h"
#include "S47Startup.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"

using namespace S47Three;

DEFINE_LOG_CATEGORY_STATIC(LogS47Walker, Log, All);

namespace
{
	// the jump: a modest one for a grown man in boots (about 0.45 m up), the eye's dip on landing
	constexpr double GRAVITY = 9.8, JUMP_UP = 0.45, LAND_SECS = 0.28, LAND_DIP = 0.065;
	const double JUMP_V = FMath::Sqrt(2 * GRAVITY * JUMP_UP);
	bool InVehicle(const AS47Walker::FTruckBox& B, double X, double Z, double Radius)
	{
		const double C=FMath::Cos(B.Heading), S=FMath::Sin(B.Heading), Dx=X-B.X, Dz=Z-B.Z;
		const double Lx=Dx*C-Dz*S, Lz=Dx*S+Dz*C;
		const double Ox=FMath::Max(0.0,FMath::Abs(Lx)-B.HalfW), Oz=FMath::Max(0.0,FMath::Abs(Lz)-B.HalfL);
		return Ox*Ox+Oz*Oz < Radius*Radius;
	}
	// First interior hit of a segment against the same rounded vehicle box as
	// InVehicle. Two strips plus the four corner discs are its exact expansion
	// by the player's radius; an enclosing AABB would block open space at turns.
	double VehicleEntry(const AS47Walker::FTruckBox& B, double X, double Z, double EndX, double EndZ, double Radius)
	{
		if (InVehicle(B,X,Z,Radius)) return 1.; // retain escape from an already overlapping parked car
		const double C=FMath::Cos(B.Heading), S=FMath::Sin(B.Heading);
		const double Px=(X-B.X)*C-(Z-B.Z)*S, Pz=(X-B.X)*S+(Z-B.Z)*C;
		const double Dx=(EndX-X)*C-(EndZ-Z)*S, Dz=(EndX-X)*S+(EndZ-Z)*C;
		const double Length2=Dx*Dx+Dz*Dz;
		if (Length2<1e-24) return 1.;
		double First=1.;
		auto Strip=[&](double HalfX,double HalfZ)
		{
			double Enter=0.,Leave=1.;
			auto Slab=[&](double P,double D,double Half)
			{
				if (FMath::Abs(D)<1e-12) return P>-Half&&P<Half;
				const double A=(-Half-P)/D, BTime=(Half-P)/D;
				Enter=FMath::Max(Enter,FMath::Min(A,BTime));
				Leave=FMath::Min(Leave,FMath::Max(A,BTime));
				return Enter<Leave;
			};
			if (Slab(Px,Dx,HalfX)&&Slab(Pz,Dz,HalfZ)&&Leave>0.&&Enter<1.) First=FMath::Min(First,FMath::Max(0.,Enter));
		};
		Strip(B.HalfW,B.HalfL+Radius); Strip(B.HalfW+Radius,B.HalfL);
		for (int32 SideX : {-1,1}) for (int32 SideZ : {-1,1})
		{
			const double Ox=Px-SideX*B.HalfW, Oz=Pz-SideZ*B.HalfL;
			const double Dot=Ox*Dx+Oz*Dz, Cross=Ox*Dz-Oz*Dx;
			const double Disc=Length2*Radius*Radius-Cross*Cross;
			if (Disc<=0.) continue; // touching a rounded corner without entering remains walkable
			const double Root=FMath::Sqrt(Disc), Enter=(-Dot-Root)/Length2, Leave=(-Dot+Root)/Length2;
			if (Leave>0.&&Enter<1.) First=FMath::Min(First,FMath::Max(0.,Enter));
		}
		return First;
	}
}

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
	MoveFrom = Pos;
	Yaw = InYaw;
	Pitch = -0.06;
	FloorY = FloorAt(X, Z);
	Settle = 0.5f;
	// (put down somewhere: on its feet, not in the air)
	bAir = false; Lift = LiftV = 0; LandT = -1;
}

void AS47Walker::Jump()
{
	if (bAir || (CanJump && !CanJump())) return;
	bAir = true; LiftV = JUMP_V; AirT = 0; Top = 0; LandT = -1;
	UE_LOG(LogS47Walker, Display, TEXT("S47: jump at %.2f %.2f, floor %.3f"), Pos.X, Pos.Z, FloorY);
	if (OnStep) OnStep(); // (the push off: a footstep on what is underfoot)
}

void AS47Walker::Land(bool bSilent)
{
	const double Hit = FMath::Max(0.0, -LiftV);
	bAir = false; Lift = LiftV = 0;
	if (bSilent) return;
	LandT = 0; LandK = FMath::Clamp(Hit / JUMP_V, 0.5, 1.3);
	UE_LOG(LogS47Walker, Display, TEXT("S47: landed at %.2f %.2f after %.2f s, %.3f m up at the top, floor %.3f"), Pos.X, Pos.Z, AirT, Top, FloorY);
	if (OnStep) OnStep();
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
	// (and the area's open land, and the ground round the truck in its own area)
	if (OpenLand && OpenLand(X, Z)) return true;
	return Ground && GroundArea == Web->Area() && Ground(X, Z);
}

void AS47Walker::Resolve(bool bX, double Old)
{
	// Unlike the original web push-out loop, clip only the requested axis segment.
	// Sequential minimum translations could push through an earlier obstacle and
	// add movement the player never requested, depending on collider order.
	if (!Web.IsValid()) return;
	const double R = Radius, E = 1e-8;
	double& Axis=bX?Pos.X:Pos.Z;
	const double Requested=Axis;
	if (Requested==Old) return;
	auto ClipZone=[&]()
	{
		if (Walkable(Pos.X,Pos.Z)) return;
		const double Want=Axis;
		double Best = Old;
		for (const FS47Rect& B : Web->Zones())
		{
			if (!B.bEnabled) continue;
			const double Ox = bX ? Old : Pos.X, Oz = bX ? Pos.Z : Old;
			if (!Inside(B.MinX, B.MaxX, B.MinZ, B.MaxZ, Ox, Oz)) continue;
			const double V = bX ? FMath::Clamp(Want, B.MinX + R, B.MaxX - R) : FMath::Clamp(Want, B.MinZ + R, B.MaxZ - R);
			if (FMath::Abs(V - Want) < FMath::Abs(Best - Want)) Best = V;
		}
		// The zone tolerance must never move farther than this requested segment.
		Axis=FMath::Clamp(Best,FMath::Min(Old,Want),FMath::Max(Old,Want));
	};
	ClipZone();
	// (the truck here as it is turned, not the box round it)
	const bool bTruck = TruckBox && TruckBox->Area == Web->Area();
	const bool bPersonal = PersonalCarBox && PersonalCarBox->Area == Web->Area();
	const double Want=Axis, Other=bX?Pos.Z:Pos.X;
	double Limit=Want;
	for (const FS47Rect& C : Web->Colliders())
	{
		if (bTruck && C.Id == TEXT("truck")) continue;
		if (bPersonal && C.Id == TEXT("personalCar")) continue;
		if (!C.bEnabled) continue; // (S47Props unblock: a building walked into now)
		const double CrossMin=(bX?C.MinZ:C.MinX)-R, CrossMax=(bX?C.MaxZ:C.MaxX)+R;
		if (Other<=CrossMin+E||Other>=CrossMax-E) continue;
		const double Low=(bX?C.MinX:C.MinZ)-R, High=(bX?C.MaxX:C.MaxZ)+R;
		if (Want>Old)
		{
			if (Old<=Low+E&&Want>Low) Limit=FMath::Min(Limit,FMath::Max(Old,Low));
			else if (Old>Low+E&&Old<High-E&&Old-Low<High-Old-E) Limit=Old;
		}
		else if (Want<Old)
		{
			if (Old>=High-E&&Want<High) Limit=FMath::Max(Limit,FMath::Min(Old,High));
			else if (Old>Low+E&&Old<High-E&&High-Old<Old-Low-E) Limit=Old;
		}
	}
	// Overlapping starts are never teleported: only a requested step towards
	// the nearer exit on this axis is admitted. Office save recovery handles old
	// positions inside furniture; ordinary motion does not rewrite the case.
	Axis=Limit;
	// The SARO truck ramp has exposed sides, not invisible full-height walls.
	// Only entering across a side needs a step-height check; the front remains a
	// continuous slope. A jump supplies real foot clearance over the raised edge.
	if(bX&&Web->Area()==TEXT("saro")&&Pos.Z>=12.4&&Pos.Z<=16.6)
	{
		const double Left=6.6-R,Right=13.0+R;
		const bool FromLeft=Old<=Left+E&&Axis>Left;
		const bool FromRight=Old>=Right-E&&Axis<Right;
		if((FromLeft||FromRight)&&FloorAt(FromLeft?6.6:13.0,Pos.Z)>FloorY+Lift+.3+E)
			Axis=Limit=FromLeft?Left:Right;
	}
	const double StartX=bX?Old:Pos.X, StartZ=bX?Pos.Z:Old;
	double First=1.;
	if (bTruck) First=FMath::Min(First,VehicleEntry(*TruckBox,StartX,StartZ,Pos.X,Pos.Z,R));
	if (bPersonal) First=FMath::Min(First,VehicleEntry(*PersonalCarBox,StartX,StartZ,Pos.X,Pos.Z,R));
	// Keep a contact outside despite the rotated quadratic's final rounding;
	// otherwise the next frame could mistake it for an old overlapping save.
	if (First<1.&&Limit!=Old) First=FMath::Max(0.,First-E/FMath::Abs(Limit-Old));
	Axis=Old+(Limit-Old)*First;
	// A valid destination beyond a doorway does not make every intermediate
	// point walkable. A collider can stop us in a gap between enabled zones;
	// clip back within this already swept segment, never past a cleared obstacle.
	ClipZone();
}

void AS47Walker::MoveBy(double DeltaX,double DeltaZ)
{
	if (!FMath::IsFinite(DeltaX)||!FMath::IsFinite(DeltaZ)) return;
	const FVector Before=Pos;
	MoveFrom=Before;
	Pos.X+=DeltaX; Resolve(true,Before.X);
	Pos.Z+=DeltaZ; Resolve(false,Before.Z);
}

bool AS47Walker::InTruck(double X, double Z) const
{
	if (!TruckBox) return false;
	// into the truck's terms (World.ts AboutUp turned back), and the distance from its box
	return InVehicle(*TruckBox, X, Z, Radius);
}

double AS47Walker::FloorAt(double X, double Z) const
{
	// The narrow gravel shoulder uses the same measured triangles as its rendered surface.
	{ double Y; if (S47SubstationDetail::FloorAt(X, Z, Y)) return Y; }
	// a prop's own floor first (Unreal only: the gas station one walks into, S47Props), then the open
	// land's ground (US47World), then Player.floor, supplied by World.enter for SARO and Caliche
	{ const double Y = AS47Props::FloorAt(X, Z); if (!FMath::IsNaN(Y)) return Y; }
	if (LandFloor) { const double Y = LandFloor(X, Z); if (!FMath::IsNaN(Y)) return Y; }
	if (Floor) { const double Y = Floor(X, Z); if (!FMath::IsNaN(Y)) return Y; }
	// the highest surface under the player, up to a stair step over the current floor (a desk
	// or a crate is higher: the web version's floor is a height per area, not the furniture)
	FHitResult Hit;
	FCollisionQueryParams P(SCENE_QUERY_STAT(S47Floor), true, this);
	const FVector From = ToUE(X, FloorY + 0.3, Z), To = ToUE(X, FloorY - 4.0, Z);
	if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, P)) return Hit.ImpactPoint.Z / 100.0;
	// (Unreal only) Only a test stands in the drive's own area: it has no land and no floor of its own. On Freeform's relief one Place
	// can be metres from the last floor, past this reach, and the floor would stay where it was: the eye 9 cm off the ground or under it.
	if (S47Relief::Active() && Web.IsValid() && Web->Area() == TEXT("road") && GetWorld()
		&& GetWorld()->LineTraceSingleByChannel(Hit, ToUE(X, FloorY + 50.0, Z), ToUE(X, FloorY - 50.0, Z), ECC_Visibility, P)) return Hit.ImpactPoint.Z / 100.0;
	return FloorY;
}

FVector AS47Walker::EyeUE() const { return Camera->GetComponentLocation(); }
FVector AS47Walker::ForwardUE() const { return Camera->GetForwardVector(); }

void AS47Walker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const double Dt = DeltaSeconds;
	APlayerController* PC = Cast<APlayerController>(GetController());
	const bool bInput = bActive && !S47Startup::Blocking();
	Focused.Reset();
	// the keys and the look, as Input.ts gives them
	double LookX = 0, LookY = 0, Mx = 0, Mz = 0;
	FVector2D VehicleMove = FVector2D::ZeroVector;
	if (PC && bInput)
	{
		float DX = 0, DY = 0;
		PC->GetInputMouseDelta(DX, DY);
		if (Settle > 0.f) { Settle -= DeltaSeconds; DX = DY = 0.f; }
		// Input.ts: 0.0022 rad per pixel; Unreal's mouse delta is in pixels times its own scale,
		// and up is positive, where the browser's movementY grows downwards
		// (the settings' look speed and invert, main.ts step: player.look(look.x, invertY ? -look.y : look.y))
		LookX = FMath::Clamp(DX, -200.f, 200.f) * 0.0022 * 4.0 * Sensitivity;
		LookY = (bInvertY ? 1.0 : -1.0) * FMath::Clamp(DY, -200.f, 200.f) * 0.0022 * 4.0 * Sensitivity;
		const FVector2D PadLook = S47Gamepad::Look(Dt, PadLookSpeed, bPadInvertY);
		LookX += PadLook.X; LookY += PadLook.Y;
		const FVector2D PadMove = S47Gamepad::Move();
		// (the keys as bound in Settings > Controls: S47Keys, W A S D and the arrows unless changed)
		if (S47Keys::Down(PC, TEXT("forward"))) Mz -= 1;
		if (S47Keys::Down(PC, TEXT("back"))) Mz += 1;
		if (S47Keys::Down(PC, TEXT("left"))) Mx -= 1;
		if (S47Keys::Down(PC, TEXT("right"))) Mx += 1;
		// Keep car controls separate before radial normalization of walking input.
		const FVector2D PadDrive = S47Gamepad::Drive();
		VehicleMove = FVector2D(Mx + PadDrive.X, Mz - PadDrive.Y);
		Mx += PadMove.X; Mz += PadMove.Y;
		if (TestMove) { Mx = TestMove->X; Mz = TestMove->Y; VehicleMove = *TestMove; }
		const double L = FMath::Sqrt(Mx * Mx + Mz * Mz);
		if (L > 1) { Mx /= L; Mz /= L; }
	}
	// the camera held by something else (the truck's cab): its eye, no walking and no picking
	FS47View View;
	if (CameraOverride && CameraOverride(Dt, VehicleMove, FVector2D(LookX, LookY), View))
	{
		if (bAir) Land(true); // (no jump goes on in the truck)
		SetActorLocationAndRotation(ToUE(View.Eye), CameraRotation(View.Yaw, View.Pitch, View.Roll));
		FVector2D Size(16, 9);
		if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(Size);
		Camera->SetFieldOfView(HorizontalFov(Fov * FovScale, float(Size.X / FMath::Max(1.0, Size.Y))));
		if (Web.IsValid()) Web->SetPlayerEye(ToUE(View.Eye));
		return;
	}
	if (PC && bInput)
	{
		Look(LookX * LookFine, LookY * LookFine);
		const bool bRun = S47Keys::Down(PC, TEXT("run")) || (TestMove && bTestRun);
		// Player.update
		const double Sp = Speed * (bRun ? 1.6 : 1.0) * MoveScale;
		const double S = FMath::Sin(Yaw), C = FMath::Cos(Yaw);
		const double Vx = (Mx * C + Mz * S) * Sp * Dt, Vz = (-Mx * S + Mz * C) * Sp * Dt;
		const FVector Before = Pos;
		MoveBy(Vx,Vz);
		const double Moved = FMath::Sqrt(FMath::Square(Pos.X - Before.X) + FMath::Square(Pos.Z - Before.Z));
		// (no bob and no footsteps in the air)
		if (Moved > 0.0005 && !bAir)
		{
			Bob += Moved * 7.2;
			StepDist += Moved;
			if (StepDist > 0.8) { StepDist = 0; if (OnStep) OnStep(); }
		}
		else if (!bAir) Bob *= 0.9;
		// the Jump key as bound (Space unless changed); with the camera raised Space is the shutter
		// and CanJump says no
		if (S47Keys::Pressed(PC, TEXT("jump"))) Jump();
	}
	const double Fy = FloorAt(Pos.X, Pos.Z);
	FloorY += (Fy - FloorY) * FMath::Min(1.0, Dt * 14.0);
	// the jump's arc over the floor (the floor under the player as on the ground: a step up or down
	// in the air is the floor's), and down on it again
	if (bAir)
	{
		AirT += Dt;
		LiftV -= GRAVITY * Dt;
		Lift += LiftV * Dt;
		Top = FMath::Max(Top, Lift);
		if (Lift <= 0) Land(false);
	}
	// the eye gives a little on landing, and comes back up
	double Dip = 0;
	if (LandT >= 0)
	{
		LandT += Dt;
		if (LandT >= LAND_SECS) LandT = -1;
		else Dip = -LAND_DIP * LandK * FMath::Sin(UE_DOUBLE_PI * LandT / LAND_SECS);
	}
	Shake = FMath::Max(0.0, Shake - Dt * 1.4);
	const double Sh = bHeadMotion ? Shake * Shake : 0;
	const FVector EyeThree(Pos.X + (FMath::FRand() - 0.5) * Sh * 0.08, FloorY + Lift + Eye + (bHeadMotion ? Dip + FMath::Sin(Bob) * 0.018 : 0) + (FMath::FRand() - 0.5) * Sh * 0.06, Pos.Z + (FMath::FRand() - 0.5) * Sh * 0.08);
	SetActorLocationAndRotation(ToUE(EyeThree), CameraRotation(Yaw, Pitch + (FMath::FRand() - 0.5) * Sh * 0.02, bHeadMotion ? FMath::Sin(Bob * 0.5) * 0.004 : 0));
	FVector2D Size(16, 9);
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(Size);
	Camera->SetFieldOfView(HorizontalFov(Fov * FovScale, float(Size.X / FMath::Max(1.0, Size.Y))));
	if (Web.IsValid()) Web->SetPlayerEye(ToUE(EyeThree));

	// Interaction.update: what the line of sight picks, within its range, with something to offer
	if (!bInput || !Inter.IsValid()) return;
	Focused = Inter->Update(Camera->GetComponentLocation(), Camera->GetForwardVector());
	// A held parcel stays usable when looking at clear ground. Existing object
	// interactions (cargo rack, bench, doors) continue to take priority.
	if (Focused.IsEmpty() && !Inter->Holds.ContainsByPredicate([](const TFunction<bool()>& H){return H&&H();})) if (US47Main* M=US47Main::Get(this); M&&M->IsFreeform()&&M->Freeform&&M->Freeform->Field&&M->Freeform->Field->ParcelStage()==2) Focused=TEXT("ff:carry");
	if (!Focused.IsEmpty() && OnUse && PC && (S47Keys::Pressed(PC, TEXT("use")) || PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)))
	{
		OnUse(Focused);
	}
}
