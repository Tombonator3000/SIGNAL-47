#include "S47Handling.h"
#include "S47Audio.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Inter.h"
#include "S47Keys.h"
#include "S47Main.h"
#include "S47Noise.h"
#include "S47Props.h"
#include "S47RoswellBrain.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "CollisionShape.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include <limits>
#include "UObject/GarbageCollection.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Handling, Log, All);

namespace
{
	const FS47Handling::FKindDef HandKinds[] = {
		// a stone the size of a fist, a coffee can, a cardboard carton, a slatted crate
		{ TEXT("stone"), TEXT("stone"), 0.45, FVector(5.5, 4.5, 3.8), 0x857868, .92f, .30, .45, 1.0, false },
		{ TEXT("can"), TEXT("coffee can"), 0.30, FVector(5.2, 5.2, 6.5), 0x8f908a, .38f, .42, .30, 1.25, false },
		{ TEXT("box"), TEXT("cardboard box"), 2.2, FVector(20, 15, 14), 0xa3865a, .95f, .12, .65, .6, true },
		{ TEXT("crate"), TEXT("wooden crate"), 9.0, FVector(30, 22, 21), 0x7a5f3e, .85f, .10, .70, 1.4, true },
	};
	constexpr double HandGravity = 980.0;   // cm/s^2
	constexpr double HandMaxLoose = 10;      // stones off the ground kept at once; the oldest is the next one picked up
	const TCHAR* const HandPutId = TEXT("phys:put");
	bool HandLive(const UObject* O) { return IsValid(O) && !O->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed); }
	// Unreal: X forward, Y right, Z up. The view's right hand.
	FVector HandRight(const FVector& Fwd) { return FVector::CrossProduct(FVector::UpVector, Fwd).GetSafeNormal(); }
	FVector HandFlat(const FVector& V) { const FVector F(V.X, V.Y, 0); return F.IsNearlyZero() ? FVector(1, 0, 0) : F.GetSafeNormal(); }
}

const FS47Handling::FKindDef& FS47Handling::Def(EKind K) { return HandKinds[FMath::Clamp(int32(K), 0, int32(EKind::Count) - 1)]; }

FS47Handling::~FS47Handling() { Life.Reset(); Release(true); }

// ---------- the rules ----------

double FS47Handling::ChargeOf(double HeldSeconds)
{
	if (!FMath::IsFinite(HeldSeconds)) return 0;
	// a tap is a put-down; after that the arm comes back over a second
	return FMath::Clamp((HeldSeconds - 0.18) / 0.95, 0.0, 1.0);
}

double FS47Handling::ThrowSpeed(double Mass, double Charge)
{
	const double C = FMath::Clamp(FMath::IsFinite(Charge) ? Charge : 0.0, 0.0, 1.0);
	// A good arm puts some 14 m/s into a stone; a heavier thing gets the same push, so it leaves slower (about as the square root).
	const double Arm = FMath::Clamp(14.0 / FMath::Sqrt(FMath::Max(Mass, 0.1) / 0.45), 1.6, 15.0);
	return Arm * (0.22 + 0.78 * C);
}

double FS47Handling::FollowRate(double Mass) { return FMath::Clamp(15.0 / FMath::Sqrt(1.0 + FMath::Max(Mass, 0.0)), 3.2, 14.0); }

double FS47Handling::WalkScale(double Mass) { return Mass <= 1.0 ? 1.0 : FMath::Clamp(1.0 - 0.045 * (Mass - 1.0), 0.6, 1.0); }

double FS47Handling::Loudness(EKind Kind, ESurface Surface, double ImpactMps)
{
	if (!FMath::IsFinite(ImpactMps) || ImpactMps <= 0) return 0;
	static const double Surf[] = { 0.85, 1.15, 1.05, 1.5, 0.4 }; // dirt, hard, wood, metal, soft
	const double S = Surf[FMath::Clamp(int32(Surface), 0, 4)];
	return FMath::Clamp(FMath::Min(ImpactMps / 7.0, 1.3) * Def(Kind).Loud * S, 0.0, 2.0);
}

const TCHAR* FS47Handling::SurfaceName(ESurface S)
{
	switch (S) { case ESurface::Hard: return TEXT("concrete"); case ESurface::Wood: return TEXT("wood"); case ESurface::Metal: return TEXT("metal"); case ESurface::Soft: return TEXT("carpet"); default: return TEXT("dirt"); }
}

// ---------- the world ----------

bool FS47Handling::Live() const
{
	return M && !IsEngineExitRequested() && !IsGarbageCollecting() && HandLive(M) && HandLive(M->Web.Get()) && HandLive(M->Player.Get()) && M->Web->GetWorld();
}

FString FS47Handling::HeldName() const { return Items.IsValidIndex(Held) ? FString(Def(Items[Held].Kind).Name) : FString(); }

FString FS47Handling::PutLabel() const
{
	if (!Items.IsValidIndex(Held)) return FString();
	const FString Key = S47Keys::Label(TEXT("throw"));
	return Key.IsEmpty() ? FString::Printf(TEXT("Set the %s down"), *HeldName()) : FString::Printf(TEXT("Set the %s down / hold %s to throw"), *HeldName(), *Key);
}

FCollisionQueryParams FS47Handling::Query(const FItem* Self) const
{
	FCollisionQueryParams Q(SCENE_QUERY_STAT(S47Handling), true);
	if (M && M->Player) Q.AddIgnoredActor(M->Player.Get());
	auto Skip = [&Q](const FItem& It)
	{
		if (UStaticMeshComponent* C = It.Comp.Get()) Q.AddIgnoredComponent(C);
		for (const auto& P : It.Parts) if (P.IsValid()) Q.AddIgnoredComponent(P.Get());
	};
	if (Self) Skip(*Self);
	if (Items.IsValidIndex(Held)) Skip(Items[Held]);
	return Q;
}

double FS47Handling::FloorUnder(const FVector& AtUE) const
{
	if (!M || !M->Player) return std::numeric_limits<double>::quiet_NaN();
	const FVector P = S47Three::ToThree(AtUE);
	double Y = AS47Props::FloorAt(P.X, P.Z);
	if (FMath::IsNaN(Y) && M->Player->LandFloor) Y = M->Player->LandFloor(P.X, P.Z);
	if (FMath::IsNaN(Y) && M->Player->Floor) Y = M->Player->Floor(P.X, P.Z);
	return FMath::IsFinite(Y) ? Y * 100.0 : std::numeric_limits<double>::quiet_NaN();
}

double FS47Handling::HalfUp(const FItem& It, const FVector& Up) const
{
	const FVector H = It.Half;
	return FMath::Abs(FVector::DotProduct(It.Q.GetAxisX(), Up)) * H.X + FMath::Abs(FVector::DotProduct(It.Q.GetAxisY(), Up)) * H.Y + FMath::Abs(FVector::DotProduct(It.Q.GetAxisZ(), Up)) * H.Z;
}

bool FS47Handling::Walkable(const FVector& AtUE) const
{
	if (!M || !M->Player) return true;
	const FVector P = S47Three::ToThree(AtUE);
	return M->Player->Walkable(P.X, P.Z);
}

FS47Handling::ESurface FS47Handling::SurfaceOf(const FHitResult* Hit, const FVector& AtUE, const FVector& Normal) const
{
	const UPrimitiveComponent* Part = Hit ? Hit->GetComponent() : nullptr;
	if (Part)
	{
		// one of these things itself
		for (const FItem& It : Items) if (It.Comp.Get() == Part) return It.Kind == EKind::Can ? ESurface::Metal : It.Kind == EKind::Stone ? ESurface::Hard : ESurface::Wood;
		FString Name = Part->GetName();
		if (const AActor* Owner = Part->GetOwner()) Name += TEXT(" ") + Owner->GetName();
		if (const UMaterialInterface* Mat = Part->GetMaterial(0)) Name += TEXT(" ") + Mat->GetName();
		if (M && M->Web) Name += TEXT(" ") + M->Web->UsableAt(Part);
		Name = Name.ToLower();
		for (const TCHAR* K : { TEXT("metal"), TEXT("steel"), TEXT("truck"), TEXT("jeep"), TEXT("vehicle"), TEXT("tank"), TEXT("corrug"), TEXT("rack"), TEXT("cabinet"), TEXT("shelving"), TEXT("dish"), TEXT("pump") })
			if (Name.Contains(K)) return ESurface::Metal;
		for (const TCHAR* K : { TEXT("wood"), TEXT("plank"), TEXT("crate"), TEXT("table"), TEXT("desk"), TEXT("bench"), TEXT("cot") })
			if (Name.Contains(K)) return ESurface::Wood;
		for (const TCHAR* K : { TEXT("carpet"), TEXT("bed"), TEXT("mattress"), TEXT("cloth") })
			if (Name.Contains(K)) return ESurface::Soft;
	}
	const FVector P = S47Three::ToThree(AtUE);
	if (Normal.Z < 0.5) return ESurface::Hard; // a wall, a post, the rock
	const FString Area = M && M->Web ? M->Web->Area() : FString();
	if (Area == TEXT("room6")) return ESurface::Soft;
	const double Floor = FloorUnder(AtUE);
	// up on something: a table top, a shelf
	if (FMath::IsFinite(Floor) && AtUE.Z - Floor > 30) return ESurface::Wood;
	if (M && M->World && M->World->Indoors(P)) return Area == TEXT("station01") ? ESurface::Wood : ESurface::Hard;
	// SARO's yard: the pads and the walks are concrete, the ground round them is not (US47Night::SurfaceAt)
	if (Area == TEXT("saro")) return P.Y < -0.3 ? ESurface::Dirt : ESurface::Hard;
	return ESurface::Dirt;
}

// ---------- the things ----------

int32 FS47Handling::Spawn(EKind Kind, const FVector& At, double YawRad, const FString& Area, bool bLoose)
{
	if (!Live()) return -1;
	AS47Web* W = M->Web.Get();
	if (!Base) Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
	static const TCHAR* Shapes[] = { TEXT("/Engine/BasicShapes/Sphere.Sphere"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), TEXT("/Engine/BasicShapes/Cube.Cube"), TEXT("/Engine/BasicShapes/Cube.Cube") };
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Shapes[FMath::Clamp(int32(Kind), 0, 3)]);
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (!Base || !Mesh || !Cube || !Cyl) return -1;
	const FKindDef& D = Def(Kind);
	FItem It;
	It.Kind = Kind; It.Area = Area; It.bLoose = bLoose;
	It.Id = FString::Printf(TEXT("phys:%d"), Items.Num());
	// each stone its own: a little bigger or smaller, flatter, lighter or darker (the game's own count, so the same each run)
	FRandomStream R(4701 + 31 * Items.Num());
	FVector Scale = D.Half / 50.0;
	uint32 Colour = D.Colour;
	if (Kind == EKind::Stone)
	{
		Scale *= FVector(R.FRandRange(.8f, 1.2f), R.FRandRange(.8f, 1.2f), R.FRandRange(.75f, 1.1f));
		const float K = R.FRandRange(.8f, 1.15f);
		const FLinearColor C = S47Three::Hex(D.Colour) * K;
		Colour = (uint32(FMath::Clamp(C.R, 0.f, 1.f) * 255) << 16) | (uint32(FMath::Clamp(C.G, 0.f, 1.f) * 255) << 8) | uint32(FMath::Clamp(C.B, 0.f, 1.f) * 255);
	}
	auto Mat = [&](uint32 Hex, float Rough)
	{
		auto* Mid = UMaterialInstanceDynamic::Create(Base, W);
		Mid->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(Hex));
		Mid->SetScalarParameterValue(TEXT("Roughness"), Rough);
		Mid->SetScalarParameterValue(TEXT("NativeLighting"), 1);
		Mid->SetScalarParameterValue(TEXT("EmissivePower"), 0);
		return Mid;
	};
	auto* C = NewObject<UStaticMeshComponent>(W, *FString::Printf(TEXT("S47Handling_%s_%d"), D.Id, Items.Num()));
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(Mesh);
	C->SetMaterial(0, Mat(Colour, D.Rough));
	C->SetupAttachment(W->GetRootComponent());
	C->SetCastShadow(true);
	C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	C->SetCollisionResponseToAllChannels(ECR_Ignore);
	C->RegisterComponent();
	C->SetWorldScale3D(Scale);
	It.Half = Scale * 50.0;
	// the parts that make it read as what it is, riding on it (no collision of their own)
	auto Part = [&](UStaticMesh* PMesh, const FVector& Rel, const FVector& RelScale, uint32 Hex, float Rough)
	{
		auto* P = NewObject<UStaticMeshComponent>(W);
		P->SetMobility(EComponentMobility::Movable);
		P->SetStaticMesh(PMesh);
		P->SetMaterial(0, Mat(Hex, Rough));
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->SetCastShadow(true);
		P->SetupAttachment(C);
		P->RegisterComponent();
		P->SetRelativeLocation(Rel);
		P->SetRelativeScale3D(RelScale);
		It.Parts.Add(P);
	};
	if (Kind == EKind::Crate)
	{
		// two dark bands of slat round it, and the lid's edge
		for (const double Z : { -28.0, 28.0 }) Part(Cube, FVector(0, 0, Z), FVector(1.035, 1.035, .14), 0x4e3b27, .9f);
		Part(Cube, FVector(0, 0, 50), FVector(.92, .9, .02), 0x5d4730, .9f);
		Part(Cube, FVector(0, -51, 0), FVector(.94, .02, .5), 0x5d4730, .9f);
	}
	else if (Kind == EKind::Box)
	{
		Part(Cube, FVector(0, 0, 50.5), FVector(.16, 1.01, .01), 0xc7b48a, .6f);  // the tape over the flaps
		Part(Cube, FVector(0, 50.5, 0), FVector(.16, .01, 1.0), 0xc7b48a, .6f);
	}
	else if (Kind == EKind::Can)
	{
		Part(Cyl, FVector(0, 0, -4), FVector(1.02, 1.02, .55), 0x7d2b22, .7f);    // a plain red paper band, no brand
		Part(Cyl, FVector(0, 0, 50), FVector(.96, .96, .02), 0x5c5c58, .35f);    // the rim of the open top
	}
	It.Comp = C;
	It.Node = W->AdoptNode(C, INDEX_NONE, It.Id);
	if (It.Node >= 0) W->MarkUsable(It.Node, It.Id);
	It.Q = FQuat(FVector::UpVector, -YawRad);
	// set on the ground there: its bottom on the floor or the ground under At
	FVector L = S47Three::ToUE(At);
	{
		FHitResult H;
		const FCollisionQueryParams Q = Query(nullptr);
		if (W->GetWorld()->LineTraceSingleByChannel(H, L + FVector(0, 0, 60), L - FVector(0, 0, 120), ECC_Visibility, Q)) L.Z = H.ImpactPoint.Z;
		else { const double F = FloorUnder(L); if (FMath::IsFinite(F)) L.Z = F; }
	}
	L.Z += HalfUp(It) + 0.15;
	It.Loc = It.HomeLoc = L; It.HomeQ = It.Q;
	It.bWasWalkable = Walkable(L);
	const int32 Index = Items.Add(MoveTemp(It));
	Collide(Items[Index], true);
	Apply(Items[Index]);
	Show(Items[Index]);
	AddUse(Index);
	return Index;
}

void FS47Handling::AddUse(int32 Index)
{
	if (!M || !M->Inter || !Items.IsValidIndex(Index)) return;
	const TWeakPtr<uint8> Guard = Life;
	FS47Interactable I;
	I.Id = Items[Index].Id;
	I.Object = M->Web->ObjAt(Items[Index].Node);
	I.Range = Items[Index].Kind == EKind::Crate ? 2.0 : 2.2;
	I.Label = [this, Guard, Index]() -> FString
	{
		if (!Guard.IsValid() || !M || !M->IsFreeform() || !Items.IsValidIndex(Index) || Holding() || Items[Index].State != EState::Rest) return FString();
		if (M->World && M->World->bDriving) return FString();
		if (M->Freeform && M->Freeform->Field && M->Freeform->Field->ParcelStage() == 2) return FString();
		return FString::Printf(TEXT("Pick up the %s"), Def(Items[Index].Kind).Name);
	};
	I.Use = [this, Guard, Index] { if (Guard.IsValid()) PickUp(Index); };
	I.Examine = [this, Guard, Index]() -> FString
	{
		if (!Guard.IsValid() || !Items.IsValidIndex(Index)) return FString();
		switch (Items[Index].Kind)
		{
		case EKind::Can: return TEXT("A coffee can with the label half off, a few cigarette ends in the bottom. Tin: it would carry, thrown.");
		case EKind::Box: return TEXT("A carton of spare chart paper, taped shut. Light enough to carry in both arms.");
		case EKind::Crate: return TEXT("A slatted crate of spare insulators, heavy. Both arms, and slowly.");
		default: return TEXT("A stone off the ground, about the size of a fist.");
		}
	};
	M->Inter->Add(I);
	if (!bPutAdded)
	{
		bPutAdded = true;
		FS47Interactable P;
		P.Id = HandPutId;
		P.Range = 99;
		P.Label = [this, Guard]() -> FString { return Guard.IsValid() ? PutLabel() : FString(); };
		P.Use = [this, Guard] { if (Guard.IsValid()) Put(); };
		M->Inter->Add(P);
	}
}

int32 FS47Handling::IndexOf(const FString& Id) const
{
	for (int32 I = 0; I < Items.Num(); ++I) if (Items[I].Id == Id) return I;
	return -1;
}

FS47Handling::FView FS47Handling::View(int32 Index) const
{
	FView V;
	if (!Items.IsValidIndex(Index)) return V;
	const FItem& It = Items[Index];
	V.Kind = It.Kind; V.Id = It.Id; V.Area = It.Area; V.At = S47Three::ToThree(It.Loc);
	V.bHeld = It.State == EState::Held; V.bFlying = It.State == EState::Fly; V.bResting = It.State == EState::Rest;
	V.Impacts = It.Impacts; V.Speed = It.Vel.Size() / 100.0;
	return V;
}

void FS47Handling::Collide(FItem& It, bool bWorld) const
{
	UStaticMeshComponent* C = It.Comp.Get();
	if (!C) return;
	// In the hand and in the air it answers no trace: the view looks past it and it does not stop itself.
	C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, bWorld ? ECR_Block : ECR_Ignore);
	C->SetCollisionResponseToChannel(ECC_Visibility, bWorld && Def(It.Kind).bBlocksSight ? ECR_Block : ECR_Ignore);
}

void FS47Handling::Apply(FItem& It) const
{
	if (UStaticMeshComponent* C = It.Comp.Get()) C->SetWorldLocationAndRotation(It.Loc, It.Q, false, nullptr, ETeleportType::TeleportPhysics);
}

void FS47Handling::Show(FItem& It) const
{
	UStaticMeshComponent* C = It.Comp.Get();
	if (!C || !M || !M->Web) return;
	const bool bHere = It.State == EState::Held || It.Area == M->Web->Area();
	if (C->IsVisible() != bHere) C->SetVisibility(bHere, true);
	C->SetCollisionEnabled(bHere ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void FS47Handling::Build()
{
	if (bBuilt || !Live() || !M->Web->IsLoaded() || M->Web->Area() != TEXT("saro")) return;
	bBuilt = true;
	// SARO's loose things, three-space floor points. Stones by the foot of the ramp from the fire exit, the butt can on the stoop,
	// a carton and a crate at the sides of the garage (clear of the Jeep's lane, x 33.6 to 37.2, and its doors).
	struct FHome { EKind Kind; double X, Z, Yaw; };
	const FHome Homes[] = {
		{ EKind::Stone, -11.6, 5.15, .4 }, { EKind::Stone, -12.15, 6.5, 1.9 }, { EKind::Stone, -11.2, 6.95, 3.0 },
		{ EKind::Can, -6.95, 6.35, 0 },
		{ EKind::Box, 32.55, 3.25, .15 },
		{ EKind::Crate, 37.7, 1.55, 0 },
	};
	for (const FHome& H : Homes) Spawn(H.Kind, FVector(H.X, 0, H.Z), H.Yaw, TEXT("saro"), false);
	UE_LOG(LogS47Handling, Display, TEXT("S47: handling: %d loose things at SARO"), Items.Num());
}

void FS47Handling::Begin()
{
	Release(true);
	// a new world (a save opened after the level came back): the old things went with it, and are made again
	if (Items.ContainsByPredicate([](const FItem& It) { return !It.Comp.IsValid(); })) { Items.Reset(); bBuilt = false; bPutAdded = false; StoneCount = 0; }
	Held = -1; bDown = bPressTaken = false; DownFor = 0;
	// what was moved goes back where it lives; a stone off the ground goes back to being the ground
	for (FItem& It : Items)
	{
		It.State = EState::Rest; It.Vel = It.Spin = FVector::ZeroVector; It.Air = It.Far = 0;
		if (It.bLoose) { It.Area = TEXT("none"); }
		else { It.Loc = It.HomeLoc; It.Q = It.HomeQ; It.Area = TEXT("saro"); }
		Collide(It, true); Apply(It); Show(It);
	}
	S47Noise::Clear();
}

void FS47Handling::Stop()
{
	Release(true);
	Held = -1; bDown = false;
	if (!Live()) return;
	for (FItem& It : Items) if (UStaticMeshComponent* C = It.Comp.Get()) { C->SetVisibility(false, true); C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
}

void FS47Handling::Release(bool bRestoreSpeed)
{
	if (bOwnSpeed && bRestoreSpeed && M && HandLive(M) && HandLive(M->Player.Get()) && FMath::IsNearlyEqual(M->Player->Speed, AppliedSpeed, .001)) M->Player->Speed = BaseSpeed;
	bOwnSpeed = false;
}

bool FS47Handling::PickUp(int32 Index)
{
	if (!Live() || !Items.IsValidIndex(Index) || Holding()) return false;
	FItem& It = Items[Index];
	if (It.State != EState::Rest || It.Area != M->Web->Area()) return false;
	if (M->World && M->World->bDriving) return false;
	if (M->Freeform && M->Freeform->Field && M->Freeform->Field->ParcelStage() == 2) return false;
	if (FVector::Dist(It.Loc, M->Player->EyeUE()) > 320) return false;
	Held = Index; It.State = EState::Held; It.Vel = It.Spin = FVector::ZeroVector; It.Far = 0;
	Collide(It, false);
	const FKindDef& D = Def(It.Kind);
	if (WalkScale(D.Mass) < 1)
	{
		BaseSpeed = M->Player->Speed; AppliedSpeed = BaseSpeed * WalkScale(D.Mass); M->Player->Speed = AppliedSpeed; bOwnSpeed = true;
	}
	if (M->Audio)
	{
		FS47Sound S; S.At = S47Three::ToThree(It.Loc);
		if (It.Kind == EKind::Stone) { S.Gain = .14f; S.Rate = 1.35f; M->Audio->Sfx(TEXT("stepGravel2"), S); }
		else if (It.Kind == EKind::Can) { S.Gain = .08f; S.Rate = 2.4f; M->Audio->Sfx(TEXT("doorMetal"), S); }
		else { S.Gain = It.Kind == EKind::Crate ? .22f : .12f; S.Rate = It.Kind == EKind::Crate ? .7f : 1.1f; M->Audio->Sfx(TEXT("thudSoft"), S); }
	}
	UE_LOG(LogS47Handling, Display, TEXT("S47: handling: picked up %s (%s)"), D.Name, *It.Id);
	return true;
}

bool FS47Handling::PickStone()
{
	if (!Live() || Holding() || (M->World && M->World->bDriving)) return false;
	if (M->Freeform && M->Freeform->Field && M->Freeform->Field->ParcelStage() == 2) return false;
	const FVector Eye = M->Player->EyeUE(), Fwd = M->Player->ForwardUE();
	FHitResult H;
	FCollisionQueryParams Q = Query(nullptr);
	for (const FItem& It : Items) if (It.Comp.IsValid()) Q.AddIgnoredComponent(It.Comp.Get());
	if (!M->Web->GetWorld()->LineTraceSingleByChannel(H, Eye, Eye + Fwd * 260, ECC_Visibility, Q) || H.ImpactNormal.Z < .75) return false;
	const FVector P = S47Three::ToThree(H.ImpactPoint);
	// the ground itself: not a table, not a floor under a roof, not concrete
	const double Floor = FloorUnder(H.ImpactPoint);
	if (FMath::IsFinite(Floor) && FMath::Abs(H.ImpactPoint.Z - Floor) > 20) return false;
	if (M->World && M->World->Indoors(P)) return false;
	if (SurfaceOf(&H, H.ImpactPoint, H.ImpactNormal) != ESurface::Dirt) return false;
	// the oldest stone taken off the ground is the one picked up again, once there are enough of them about
	int32 Index = -1, Loose = 0;
	for (int32 I = 0; I < Items.Num(); ++I) if (Items[I].bLoose) ++Loose;
	if (Loose >= HandMaxLoose)
	{
		int32 Seen = 0;
		for (int32 I = 0; I < Items.Num(); ++I) if (Items[I].bLoose && Items[I].State == EState::Rest && Seen++ == StoneCount % Loose) { Index = I; break; }
	}
	++StoneCount;
	if (Index >= 0)
	{
		FItem& It = Items[Index];
		It.Area = M->Web->Area(); It.Loc = H.ImpactPoint + FVector(0, 0, HalfUp(It) + .15);
		Apply(It); Show(It);
	}
	else Index = Spawn(EKind::Stone, P, FMath::FRandRange(0.f, 6.28f), M->Web->Area(), true);
	return Index >= 0 && PickUp(Index);
}

bool FS47Handling::ThrowNow(double Charge)
{
	if (!Live() || !Items.IsValidIndex(Held)) return false;
	FItem& It = Items[Held];
	const FKindDef& D = Def(It.Kind);
	const FVector Eye = M->Player->EyeUE(), Fwd = M->Player->ForwardUE();
	// out of the hand where the hand is, never from the far side of a wall it is pressed to
	{
		FHitResult H;
		if (M->Web->GetWorld()->SweepSingleByChannel(H, Eye, It.Loc, It.Q, ECC_Visibility, FCollisionShape::MakeBox(It.Half * .9), Query(&It)))
			It.Loc = H.bStartPenetrating ? Eye : H.Location;
	}
	const FVector Dir = (Fwd + FVector(0, 0, .1)).GetSafeNormal();
	const double Speed = ThrowSpeed(D.Mass, Charge) * 100.0;
	FRandomStream R(97 + 13 * Hits + 7 * StoneCount + Held);
	It.Vel = Dir * Speed + It.Vel * .25;
	const double SpinK = It.Kind == EKind::Stone ? 9 : It.Kind == EKind::Can ? 12 : 2.5;
	It.Spin = HandRight(Fwd) * SpinK * (.5 + .5 * Charge) + FVector(R.FRandRange(-1.f, 1.f), R.FRandRange(-1.f, 1.f), R.FRandRange(-1.f, 1.f)) * SpinK * .3;
	It.State = EState::Fly; It.Air = 0; It.bWasWalkable = Walkable(It.Loc);
	Held = -1;
	Release(true);
	if (M->Audio && Charge > .5) { FS47Sound S; S.At = S47Three::ToThree(Eye); S.Gain = .05f + .05f * float(Charge); S.Rate = 1.8f; S.Lp = 1600; M->Audio->Sfx(TEXT("gust"), S); }
	UE_LOG(LogS47Handling, Display, TEXT("S47: handling: threw the %s at %.1f m/s (charge %.2f)"), D.Name, Speed / 100.0, Charge);
	return true;
}

bool FS47Handling::Put()
{
	if (!Live() || !Items.IsValidIndex(Held)) return false;
	FItem& It = Items[Held];
	const FKindDef& D = Def(It.Kind);
	UWorld* W = M->Web->GetWorld();
	const FVector Eye = M->Player->EyeUE(), Fwd = M->Player->ForwardUE(), FlatFwd = HandFlat(Fwd);
	const FCollisionQueryParams Q = Query(&It);
	const FQuat Upright(FVector::UpVector, FMath::Atan2(FlatFwd.Y, FlatFwd.X) + (It.Kind == EKind::Crate || It.Kind == EKind::Box ? UE_HALF_PI : 0));
	FItem Probe = It; Probe.Q = Upright;
	const double Up = HalfUp(Probe);
	TArray<FVector> Tries;
	// where the view meets a level surface within reach: a table top, a shelf, the floor in front
	{
		FHitResult H;
		if (W->LineTraceSingleByChannel(H, Eye, Eye + Fwd * 190, ECC_Visibility, Q) && H.ImpactNormal.Z > .8) Tries.Add(H.ImpactPoint);
	}
	// else the ground a step in front, then closer to the feet
	for (const double Ahead : { 65.0, 45.0, 30.0 })
	{
		FHitResult H;
		const FVector Over = Eye + FlatFwd * Ahead;
		if (W->LineTraceSingleByChannel(H, Over, Over - FVector(0, 0, 260), ECC_Visibility, Q) && H.ImpactNormal.Z > .8) Tries.Add(H.ImpactPoint);
	}
	for (const FVector& S : Tries)
	{
		const FVector L = S + FVector(0, 0, Up + .2);
		// nothing between the hand and there, room for it there, and on this side of any wall
		FHitResult Block;
		if (W->SweepSingleByChannel(Block, Eye, L + FVector(0, 0, 4), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(4), Q)) continue;
		if (W->OverlapBlockingTestByChannel(L + FVector(0, 0, 1.5), Upright, ECC_Visibility, FCollisionShape::MakeBox(It.Half * .95), Q)) continue;
		if (!Walkable(L) && FMath::IsFinite(FloorUnder(L)) && L.Z - FloorUnder(L) < 30) continue;
		It.Loc = L; It.Q = Upright; It.State = EState::Rest; It.Vel = It.Spin = FVector::ZeroVector; It.Area = M->Web->Area();
		Held = -1;
		Release(true);
		Collide(It, true); Apply(It);
		const FVector N(0, 0, 1);
		const ESurface Surf = SurfaceOf(nullptr, L - FVector(0, 0, Up), N);
		if (M->Audio)
		{
			FS47Sound Snd; Snd.At = S47Three::ToThree(L); Snd.Gain = It.Kind == EKind::Crate ? .3f : .1f;
			Snd.Rate = It.Kind == EKind::Crate ? .75f : It.Kind == EKind::Can ? 2.2f : 1.3f;
			M->Audio->Sfx(It.Kind == EKind::Can ? TEXT("doorMetal") : It.Kind == EKind::Crate ? TEXT("stepWood1") : Surf == ESurface::Dirt ? TEXT("stepGravel1") : TEXT("thudSoft"), Snd);
		}
		// set down, not dropped: only a crate's weight is heard, and only close by
		S47Noise::Make(S47Three::ToThree(L), It.Kind == EKind::Crate ? .3 : .08, FString::Printf(TEXT("%s set down"), D.Name));
		UE_LOG(LogS47Handling, Display, TEXT("S47: handling: set the %s down at %s"), D.Name, *S47Three::ToThree(L).ToCompactString());
		return true;
	}
	if (M->UI) M->UI->Toast(FString::Printf(TEXT("No room to set the %s down there."), D.Name), 2.5f);
	return false;
}

void FS47Handling::Impact(FItem& It, const FHitResult* Hit, const FVector& Normal, double Mps)
{
	if (Clock - It.LastHitAt < .08) return;
	It.LastHitAt = Clock; ++It.Impacts; ++Hits;
	const FKindDef& D = Def(It.Kind);
	const ESurface S = SurfaceOf(Hit, It.Loc - Normal * HalfUp(It, Normal), Normal);
	const double Loud = Loudness(It.Kind, S, Mps);
	const FVector At3 = S47Three::ToThree(It.Loc);
	if (M && M->Audio)
	{
		const float Gain = float(FMath::Clamp(Mps / 9.0, .05, 1.0));
		const int32 V = Hits % 5;
		auto Play = [&](const FString& Name, float G, float Rate) { FS47Sound Snd; Snd.At = At3; Snd.Gain = G; Snd.Rate = Rate; M->Audio->Sfx(Name, Snd); };
		const FString Step = S == ESurface::Dirt ? FString::Printf(TEXT("stepGravel%d"), V) : S == ESurface::Wood ? FString::Printf(TEXT("stepWood%d"), V) : S == ESurface::Soft ? FString::Printf(TEXT("stepCarpet%d"), V) : FString::Printf(TEXT("stepConcrete%d"), V);
		switch (It.Kind)
		{
		case EKind::Stone:
			if (S == ESurface::Metal) Play(TEXT("doorMetal"), .45f * Gain, 2.1f);
			else Play(Step, .9f * Gain, S == ESurface::Dirt ? 1.3f : 1.75f);
			break;
		case EKind::Can:
			Play(TEXT("doorMetal"), .4f * Gain, 2.6f);
			if (S == ESurface::Dirt) Play(Step, .5f * Gain, 1.5f);
			break;
		case EKind::Box:
			Play(TEXT("thudSoft"), .7f * Gain, 1.05f);
			break;
		default:
			Play(S == ESurface::Metal ? FString(TEXT("doorMetal")) : FString::Printf(TEXT("stepWood%d"), V), .9f * Gain, S == ESurface::Metal ? 1.1f : .7f);
			Play(TEXT("thudSoft"), Gain, .62f);
			break;
		}
	}
	LastHit = FString::Printf(TEXT("%s on %s at %.1f m/s, heard to %.0f m"), D.Name, SurfaceName(S), Mps, S47Noise::Reach(Loud));
	S47Noise::Make(At3, Loud, FString::Printf(TEXT("%s on %s"), D.Name, SurfaceName(S)));
}

void FS47Handling::Settle(FItem& It, const FVector& N)
{
	// down on the face nearest the ground, level with it: it lies as it fell, and still
	const FVector Axes[] = { It.Q.GetAxisX(), It.Q.GetAxisY(), It.Q.GetAxisZ() };
	int32 Best = 2; double Most = -1;
	for (int32 I = 0; I < 3; ++I) { const double D = FMath::Abs(FVector::DotProduct(Axes[I], N)); if (D > Most) { Most = D; Best = I; } }
	const FVector From = Axes[Best] * (FVector::DotProduct(Axes[Best], N) >= 0 ? 1.0 : -1.0);
	It.Q = (FQuat::FindBetweenNormals(From, N) * It.Q).GetNormalized();
	FHitResult H;
	if (M->Web->GetWorld()->LineTraceSingleByChannel(H, It.Loc + N * 2, It.Loc - N * 80, ECC_Visibility, Query(&It))) It.Loc = H.ImpactPoint + N * (HalfUp(It, N) + .15);
	else { const double F = FloorUnder(It.Loc); if (FMath::IsFinite(F)) It.Loc.Z = F + HalfUp(It) + .15; }
	It.State = EState::Rest; It.Vel = It.Spin = FVector::ZeroVector; It.Air = 0;
	It.Area = M->Web->Area();
	Collide(It, true); Apply(It);
	UE_LOG(LogS47Handling, Display, TEXT("S47: handling: the %s came to rest at %s after %d impacts"), Def(It.Kind).Name, *S47Three::ToThree(It.Loc).ToCompactString(), It.Impacts);
}

void FS47Handling::StepFly(FItem& It, double Dt)
{
	UWorld* W = M->Web->GetWorld();
	const FKindDef& D = Def(It.Kind);
	const FCollisionShape Shape = FCollisionShape::MakeBox(It.Half);
	const FCollisionQueryParams Q = Query(&It);
	const int32 Steps = FMath::Clamp(FMath::CeilToInt(Dt * 120.0), 1, 24);
	const double H = Dt / Steps;
	for (int32 S = 0; S < Steps; ++S)
	{
		It.Air += H;
		It.Vel.Z -= HandGravity * H;
		It.Vel *= 1.0 - .03 * H;
		const FVector From = It.Loc, To = From + It.Vel * H;
		FHitResult Hit;
		bool bHit = W->SweepSingleByChannel(Hit, From, To, It.Q, ECC_Visibility, Shape, Q);
		if (bHit && Hit.bStartPenetrating)
		{
			// out of whatever it is in, along the way out, before anything else
			It.Loc = From + Hit.Normal * (Hit.PenetrationDepth + .3);
			if (Hit.Normal.Z > .7 && It.Vel.Z < 0) It.Vel.Z = 0;
			continue;
		}
		FVector Next = bHit ? Hit.Location + Hit.Normal * .15 : To;
		FVector N = bHit ? Hit.Normal : FVector::ZeroVector;
		bool bGuard = false;
		// The walk zones stand in for a wall the sweep did not see: out of them low down is into a wall, so it comes back.
		const double Floor = FloorUnder(Next);
		const bool bWalk = Walkable(Next);
		if (!bHit && It.bWasWalkable && !bWalk && (!FMath::IsFinite(Floor) || Next.Z - Floor < 260))
		{
			const FVector Along = HandFlat(It.Vel);
			N = -Along; bHit = bGuard = true;
			Next = FVector(From.X, From.Y, Next.Z);
		}
		else if (bWalk) It.bWasWalkable = true;
		// never under the ground
		if (FMath::IsFinite(Floor))
		{
			const double Up = HalfUp(It);
			if (Next.Z - Up < Floor - 1.0) { Next.Z = Floor + Up + .15; if (!bHit || N.Z < .7) { bHit = true; N = FVector::UpVector; } }
		}
		It.Loc = Next;
		const double SpinRate = It.Spin.Size();
		if (SpinRate > 1e-3) It.Q = (FQuat(It.Spin / SpinRate, SpinRate * H) * It.Q).GetNormalized();
		if (!bHit) continue;
		const double Vn = FVector::DotProduct(It.Vel, N);
		if (Vn < 0)
		{
			const FVector Vt = It.Vel - N * Vn;
			const double Mps = -Vn / 100.0;
			It.Vel = Vt * (1.0 - D.Grip * FMath::Clamp(Mps / 3.0, .25, 1.0)) - N * Vn * D.Bounce;
			if (Mps > .9) Impact(It, bGuard ? nullptr : &Hit, N, Mps);
			It.Spin = It.Spin * .5 + FVector::CrossProduct(N, Vt) / FMath::Max(3.0, It.Half.GetMin()) * .25;
			if (It.Spin.Size() > 30) It.Spin = It.Spin.GetSafeNormal() * 30;
		}
		if (N.Z > .7)
		{
			// rolling and sliding on the ground take the rest of it
			It.Vel.X *= 1.0 - FMath::Min(1.0, 4.0 * H * (1 + D.Grip * 3));
			It.Vel.Y *= 1.0 - FMath::Min(1.0, 4.0 * H * (1 + D.Grip * 3));
			if (It.Vel.Size() < 45) { Settle(It, N); return; }
		}
	}
	if (It.Air > 8) { Settle(It, FVector::UpVector); return; }
	Apply(It);
}

void FS47Handling::StepHeld(FItem& It, double Dt)
{
	UWorld* W = M->Web->GetWorld();
	const FKindDef& D = Def(It.Kind);
	const FCollisionQueryParams Q = Query(&It);
	const FVector Eye = M->Player->EyeUE(), Fwd = M->Player->ForwardUE(), FlatFwd = HandFlat(Fwd), Right = HandRight(Fwd);
	const bool bArms = It.Kind == EKind::Box || It.Kind == EKind::Crate;
	const double Charge = Charging() ? ChargeOf(DownFor) : 0;
	// a small thing in the right hand, low in the view; a big one in both arms in front of the belly
	FVector Target = bArms ? Eye + FlatFwd * (42 + It.Half.Y) - FVector(0, 0, It.Kind == EKind::Crate ? 72 : 58)
	                       : Eye + Fwd * 52 + Right * 17 - FVector(0, 0, 21);
	// the throw drawn back
	Target += bArms ? -FlatFwd * 14 * Charge : (-Fwd * 20 + Right * 6 + FVector(0, 0, 9)) * Charge;
	{
		// never held through a wall the player stands against
		FHitResult H;
		if (W->SweepSingleByChannel(H, Eye, Target, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(FMath::Max(2.0, It.Half.GetMin() * .8)), Q))
			Target = H.bStartPenetrating ? Eye : H.Location;
	}
	const double W0 = FollowRate(D.Mass), Zeta = .72;
	const FVector Acc = (Target - It.Loc) * W0 * W0 - It.Vel * 2 * Zeta * W0;
	It.Vel += Acc * Dt;
	if (It.Vel.Size() > 900) It.Vel = It.Vel.GetSafeNormal() * 900;
	FVector Next = It.Loc + It.Vel * Dt;
	FHitResult H;
	if (W->SweepSingleByChannel(H, It.Loc, Next, It.Q, ECC_Visibility, FCollisionShape::MakeBox(It.Half), Q))
	{
		if (H.bStartPenetrating) Next = It.Loc + H.Normal * (H.PenetrationDepth + .3);
		else
		{
			Next = H.Location + H.Normal * .15;
			const double Vn = FVector::DotProduct(It.Vel, H.Normal);
			if (Vn < 0) It.Vel -= H.Normal * Vn;
		}
	}
	It.Loc = Next;
	// it hangs and sways with what the hand does
	const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(FlatFwd.Y, FlatFwd.X)) + (bArms ? 90.0 : 0.0);
	const double Pitch = FMath::Clamp(-FVector::DotProduct(Acc, FlatFwd) * .0016, -14.0, 14.0);
	const double Roll = FMath::Clamp(FVector::DotProduct(Acc, Right) * .0016, -14.0, 14.0);
	const FQuat Want = FRotator(bArms ? Roll : Pitch, Yaw, bArms ? -Pitch : Roll).Quaternion();
	It.Q = FQuat::Slerp(It.Q, Want, float(FMath::Min(1.0, Dt * W0 * .55))).GetNormalized();
	It.Area = M->Web->Area();
	// left behind (a door frame, a corner): it slips out of the hands and falls where it is
	It.Far = FVector::Dist(Target, It.Loc) > 140 ? It.Far + Dt : 0;
	if (It.Far > .45)
	{
		UE_LOG(LogS47Handling, Display, TEXT("S47: handling: the %s slipped out of reach"), D.Name);
		Held = -1; Release(true); It.State = EState::Fly; It.Air = 0; It.bWasWalkable = Walkable(It.Loc);
	}
	Apply(It);
}

void FS47Handling::Step(double Dt)
{
	if (!Live() || !M->IsFreeform() || !M->Freeform || !M->Freeform->Active()) return;
	if (!bBuilt) Build();
	FS47FreeformSession* F = M->Freeform.Get();
	if (F->Held())
	{
		// a menu or a paper: nothing moves, and a throw half drawn is not let go when it closes
		bDown = false; DownFor = 0; bPressTaken = true;
		return;
	}
	if (!FMath::IsFinite(Dt) || Dt <= 0) return;
	Dt = FMath::Min(Dt, .05);
	Clock += Dt;
	// in the truck, or the parcel taken up: what was in the hand is put down by the feet
	if (Holding() && ((M->World && M->World->bDriving) || (F->Field && F->Field->ParcelStage() == 2)))
	{
		FItem& It = Items[Held];
		Held = -1; Release(true); It.State = EState::Fly; It.Vel = FVector::ZeroVector; It.Air = 0; It.bWasWalkable = Walkable(It.Loc);
	}
	APlayerController* PC = M->Web->GetWorld()->GetFirstPlayerController();
	const bool bKey = PC && S47Keys::Down(PC, TEXT("throw")) && !M->ModalOpen() && !(M->World && M->World->bDriving);
	if (bKey && !bDown)
	{
		bDown = true; DownFor = 0; bPressTaken = false;
		if (!Holding())
		{
			// empty-handed: what is in view, or a stone off the ground
			bPressTaken = true;
			const FString Focus = M->Player->FocusedId();
			if (Focus.StartsWith(TEXT("phys:")) && Focus != HandPutId) PickUp(IndexOf(Focus));
			else PickStone();
		}
	}
	else if (bKey) DownFor += Dt;
	else if (bDown)
	{
		bDown = false;
		if (!bPressTaken && Holding()) { if (DownFor < .18) Put(); else ThrowNow(ChargeOf(DownFor)); }
		bPressTaken = false; DownFor = 0;
	}
	for (int32 I = 0; I < Items.Num(); ++I)
	{
		FItem& It = Items[I];
		It.Age += Dt;
		if (It.State == EState::Held) StepHeld(It, Dt);
		else if (It.State == EState::Fly) StepFly(It, Dt);
		Show(It);
	}
}

// ---------- tested without a world ----------

bool FS47Handling::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool b, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s handling: %s"), b ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= b; };
	const double Stone = Def(EKind::Stone).Mass, Can = Def(EKind::Can).Mass, Box = Def(EKind::Box).Mass, Crate = Def(EKind::Crate).Mass;
	Check(Can < Stone && Stone < Box && Box < Crate && Crate >= 6 && Def(EKind::Crate).bBlocksSight && !Def(EKind::Stone).bBlocksSight, TEXT("a can, a stone, a carton and a crate, lightest to heaviest; only the big ones stand in a line of sight"));
	Check(ChargeOf(0.1) == 0 && ChargeOf(0.6) > 0 && ChargeOf(0.6) < ChargeOf(0.9) && ChargeOf(1.2) == 1 && ChargeOf(9) == 1 && ChargeOf(std::numeric_limits<double>::quiet_NaN()) == 0, TEXT("a tap is no throw; the longer the key is held, the harder, to a full throw at about a second"));
	Check(ThrowSpeed(Stone, 1) > 11 && ThrowSpeed(Stone, 1) < 15.5 && ThrowSpeed(Stone, 0) < ThrowSpeed(Stone, 1) * .4 && ThrowSpeed(Stone, 0) > 2, TEXT("a stone: a lob of a few metres a second to some fourteen"));
	Check(ThrowSpeed(Crate, 1) < 5 && ThrowSpeed(Box, 1) < ThrowSpeed(Stone, 1) * .6 && ThrowSpeed(Can, 1) <= 15, TEXT("heavier leaves the hand slower: a crate is only heaved"));
	Check(FollowRate(Stone) > 2 * FollowRate(Crate) && FollowRate(Crate) >= 3.2 && FollowRate(Stone) <= 14, TEXT("a stone follows the hand at once, a crate hangs behind it"));
	Check(WalkScale(Stone) == 1 && WalkScale(Can) == 1 && WalkScale(Crate) < .75 && WalkScale(Crate) >= .6 && WalkScale(Box) < 1 && WalkScale(Box) > WalkScale(Crate), TEXT("a crate in the arms slows the walk; a stone does not"));
	Check(Loudness(EKind::Stone, ESurface::Metal, 8) > Loudness(EKind::Stone, ESurface::Hard, 8) && Loudness(EKind::Stone, ESurface::Hard, 8) > Loudness(EKind::Stone, ESurface::Dirt, 8)
		&& Loudness(EKind::Stone, ESurface::Dirt, 8) > Loudness(EKind::Stone, ESurface::Soft, 8), TEXT("metal rings louder than concrete, concrete than dirt, dirt than carpet"));
	Check(S47Noise::Reach(Loudness(EKind::Stone, ESurface::Dirt, 9)) >= 9 && S47Noise::Reach(Loudness(EKind::Stone, ESurface::Metal, 9)) >= 15 && S47Noise::Reach(Loudness(EKind::Stone, ESurface::Dirt, 1)) < 2.5 && Loudness(EKind::Box, ESurface::Dirt, 9) < Loudness(EKind::Stone, ESurface::Dirt, 9),
		TEXT("a hard-thrown stone on dirt carries some ten metres, on metal fifteen or more; a tap a metre or two; a carton less than a stone"));
	Check(Loudness(EKind::Stone, ESurface::Dirt, 0) == 0 && Loudness(EKind::Stone, ESurface::Dirt, std::numeric_limits<double>::quiet_NaN()) == 0 && Loudness(EKind::Crate, ESurface::Metal, 100) <= 2, TEXT("still is silent, and nothing is louder than the listeners' scale"));
	// the noise board: posted, read as the loudest fresh one, counted once each
	S47Noise::Clear();
	const int32 Before = S47Noise::Count();
	S47Noise::Make(FVector(1, 0, 1), .02, TEXT("nothing"));
	S47Noise::Make(FVector(2, 0, 2), .6, TEXT("quiet"));
	S47Noise::Make(FVector(3, 0, 3), 1.1, TEXT("loud"));
	const TOptional<S47Noise::FNoise> L = S47Noise::Latest(1.0);
	Check(S47Noise::Count() == Before + 2 && L && L->What == TEXT("loud") && S47Noise::Last() && S47Noise::Last()->Serial == L->Serial, TEXT("a noise too quiet to hear is not posted; the loudest fresh one is what the listeners get"));
	S47Noise::Clear();
	Check(!S47Noise::Latest(1.0), TEXT("cleared, nothing is heard"));
	// The lure: a small one hunting, the player far off and still, a stone landing to its side. It goes to the stone.
	{
		FS47RoswellBrain::FWorld World;
		World.CanStand = [](const FVector& P) { return !P.ContainsNaN(); };
		World.CanMove = [](const FVector&, const FVector&) { return true; };
		World.FindPath = [](const FVector&, const FVector& To) { return TArray<FVector>{ To }; };
		World.Sight = [](const FVector&, const FVector&) { return true; };
		FS47RoswellBrain::FInput In; In.PlayerFeet = FVector(0, 0, 40); In.PlayerEye = In.PlayerFeet + FVector(0, 1.62, 0); In.PlayerSpeed = 0;
		const FVector StoneAt(7, 0, -3);
		FS47RoswellBrain Hunter; Hunter.Start(FS47RoswellBrain::EMode::Hunt, FVector::ZeroVector, 0, 11);
		Hunter.Tick(.1, In, World);
		const bool bNoMemory = !Hunter.bHasPlayerMemory;
		In.Noise = FS47RoswellBrain::FNoise{ StoneAt, Loudness(EKind::Stone, ESurface::Dirt, 9) };
		Hunter.Tick(.1, In, World);
		In.Noise.Reset();
		const double Start = FVector::Dist(Hunter.Pos, StoneAt);
		for (int32 I = 0; I < 40; ++I) Hunter.Tick(.1, In, World);
		Check(bNoMemory && Hunter.LastKnownPlayer.Equals(StoneAt) && FVector::Dist(Hunter.Pos, StoneAt) < Start - 1.5 && FVector::Dist(Hunter.Pos, In.PlayerFeet) > 30,
			TEXT("a hunting one that has not found the player goes to where the stone landed, away from the player"));
		FS47RoswellBrain Deaf; Deaf.Start(FS47RoswellBrain::EMode::Hunt, FVector::ZeroVector, 0, 11);
		In.Noise = FS47RoswellBrain::FNoise{ StoneAt, Loudness(EKind::Stone, ESurface::Dirt, 1) };
		Deaf.Tick(.1, In, World);
		Check(!Deaf.bHasPlayerMemory, TEXT("a stone dropped at the feet, eight metres from it, is not heard"));
	}
	return bOk;
}
