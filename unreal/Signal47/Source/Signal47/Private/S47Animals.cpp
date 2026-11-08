#include "S47Animals.h"

#include "Animation/AnimSequence.h"
#include "AnimationRuntime.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/PackageName.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "S47Ambience.h"
#include "S47Audio.h"
#include "S47EventPlaces.h"
#include "S47FarmWorld.h"
#include "S47Flashlight.h"
#include "S47Freeform.h"
#include "S47Geo.h"
#include "S47Herd.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Omen.h"
#include "S47OpenLand.h"
#include "S47Seen.h"
#include "S47Stress.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47WebAudio.h"
#include "S47Web.h"
#include "S47World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Animals, Log, All);

namespace S47AnimalsLocal
{
	const TCHAR* const CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* const BallPath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* const LitPath = TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque");
	// How far each feels what comes, on FS47Omen's own reach: the birds and the dogs furthest.
	constexpr double CoveyWiden = 2.2, CoyoteWiden = 2.0, DogWiden = 2.2, HerdWiden = 2.0;
	constexpr double TrotSpeed = 3.2, BoltSpeed = 9.5, CoyoteGone = 120, CoyoteShy = 11, CoyoteCross = 32;
	constexpr double CoveyRelocate = 90, FlightSeconds = 3.2;
	constexpr double DogHear = 900, DogClose = 160, DogWhine = 260;
	// Where the dogs live, road-local: by the old house on the Alderete place, before the houses on the east side of Caliche, and at
	// the south end of town (on the ground outside the doors, S47EventPlaces).
	const FVector2D DogHomes[] = { FVector2D(95, 205), FVector2D(16, 2141), FVector2D(-11, 2231) };
	// The coyote's parts on its own frame: forward, right, up (metres), and size (forward, across, up).
	struct FPart { double F, S, U, L, W, H; };
	constexpr FPart TorsoPart{ 0, 0, .47, .80, .24, .27 }, NeckPart{ .40, 0, .58, .24, .14, .15 }, HeadPart{ .55, 0, .67, .20, .16, .14 },
		SnoutPart{ .69, 0, .63, .15, .08, .08 }, EarPart{ .52, .05, .78, .04, .06, .10 }, EyePart{ .635, .045, .69, .035, .035, .035 },
		TailPart{ -.55, 0, .40, .40, .08, .08 };
	constexpr double HipF = .30, HipS = .08, HipU = .44, LegLen = .44, NeckF = .45;

	double YawOf(const FVector& FlatThree) { return FMath::RadiansToDegrees(FMath::Atan2(FlatThree.Z, FlatThree.X)); }
	FVector Turned(const FVector& FlatThree, double Degrees)
	{
		const double Rad = FMath::DegreesToRadians(Degrees), Cs = FMath::Cos(Rad), Sn = FMath::Sin(Rad);
		return FVector(FlatThree.X * Cs - FlatThree.Z * Sn, 0, FlatThree.X * Sn + FlatThree.Z * Cs);
	}
	double Flat(const FVector& From, const FVector& To) { return FVector2D::Distance(FVector2D(From.X, From.Z), FVector2D(To.X, To.Z)); }
}

FS47Animals::~FS47Animals() = default;

// ---------- the rules ----------

FVector FS47Animals::FleeDir(const FVector& From, const FVector& At)
{
	const FVector Way(At.X - From.X, 0, At.Z - From.Z);
	return Way.SizeSquared() < 1e-8 ? FVector(1, 0, 0) : Way.GetSafeNormal();
}

double FS47Animals::FlushRange(double Sneak, bool bRunning)
{
	return bRunning ? 11.0 : FMath::Lerp(7.0, 3.5, FMath::Clamp(Sneak, 0.0, 1.0));
}

int32 FS47Animals::Fear(double Sense)
{
	return Sense < 0.06 ? 0 : Sense < 0.45 ? 1 : 2;
}

double FS47Animals::HerdLevel(double Sense)
{
	if (Sense < 0.06) return 0;
	// uneasy: a walk together, below S47Herd::SCATTER; afraid: a run
	if (Sense < 0.45) return FMath::Lerp(0.12, 0.45, (Sense - 0.06) / 0.39);
	return FMath::Lerp(0.6, 1.0, FMath::Clamp((Sense - 0.45) / 0.55, 0.0, 1.0));
}

double FS47Animals::DogQuiet(double Sense)
{
	return 25.0 + 40.0 * FMath::Clamp(Sense, 0.0, 1.0);
}

double FS47Animals::HerdFear(const FVector& HerdMiddle, FVector& OutFrom) const
{
	if (!M || !M->IsFreeform() || !M->Omen.IsValid()) return 0;
	FVector From = FVector::ZeroVector;
	const double Felt = M->Omen->SenseAt(HerdMiddle, S47AnimalsLocal::HerdWiden, &From, nullptr);
	if (Felt <= 0) return 0;
	OutFrom = From;
	return HerdLevel(Felt);
}

// ---------- the world ----------

FVector FS47Animals::EyeNow() const { return M && M->Player ? S47Three::ToThree(M->Player->EyeUE()) : FVector::ZeroVector; }
bool FS47Animals::Outdoors() const { return M && M->Space == TEXT("yard") && !(M->World && M->World->bDriving); }
double FS47Animals::Hour() const { return M && M->Freeform ? FMath::Fmod(M->Freeform->Clock(), 86400.0) / 3600.0 : 12.0; }
bool FS47Animals::NightNow() const { const double Hr = Hour(); return Hr >= 19.5 || Hr < 5.6; }

bool FS47Animals::RoadOf(const FVector& At, FVector2D& Out) const
{
	double Rx = 0, Rz = 0;
	if (!M || !M->Web || !S47OpenLand::RoadOf(M->Web->Area(), At.X, At.Z, Rx, Rz)) return false;
	Out = FVector2D(Rx, Rz);
	return true;
}

FVector FS47Animals::FromRoad(const FVector2D& RoadAt, double Up) const
{
	const FString Area = M && M->Web ? M->Web->Area() : FString(TEXT("road"));
	FVector Spot = S47OpenLand::FromRoad(Area, RoadAt.X, RoadAt.Y);
	double Floor = S47FarmWorld::FloorRoad(RoadAt.X, RoadAt.Y);
	if (Area == TEXT("station01")) Floor += S47Geo::STATION_DY();
	Spot.Y = Floor + Up;
	return Spot;
}

double FS47Animals::GroundAt(const FVector& At) const
{
	if (M && M->Player && M->Player->Floor)
	{
		const double Fl = M->Player->Floor(At.X, At.Z);
		if (FMath::IsFinite(Fl)) return Fl;
	}
	if (M && M->Web && M->Web->GetWorld())
	{
		FHitResult Hit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(S47AnimalGround), false);
		const FVector Above = S47Three::ToUE(FVector(At.X, At.Y + 30, At.Z)), Below = S47Three::ToUE(FVector(At.X, At.Y - 60, At.Z));
		if (M->Web->GetWorld()->LineTraceSingleByChannel(Hit, Above, Below, ECC_Visibility, Query)) return S47Three::ToThree(Hit.ImpactPoint).Y;
	}
	return At.Y;
}

bool FS47Animals::Build()
{
	if (!M || !IsValid(M->Web)) return false;
	if (bBuilt && Scene.Get() == M->Web) return Root.IsValid();
	bBuilt = true; Scene = M->Web;
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, S47AnimalsLocal::CubePath);
	UStaticMesh* BallMesh = LoadObject<UStaticMesh>(nullptr, S47AnimalsLocal::BallPath);
	UMaterialInterface* Lit = LoadObject<UMaterialInterface>(nullptr, S47AnimalsLocal::LitPath);
	if (!CubeMesh || !BallMesh || !Lit) return false;
	AS47Web* Web = M->Web.Get();
	USceneComponent* Holder = NewObject<USceneComponent>(Web, TEXT("S47Animals"));
	Holder->SetupAttachment(Web->GetRootComponent()); Holder->SetMobility(EComponentMobility::Movable); Holder->RegisterComponent();
	Root = Holder;
	auto Tint = [Lit, Web](const FLinearColor& Shade, float Rough)
	{
		UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Lit, Web);
		Mat->SetVectorParameterValue(TEXT("Color"), Shade); Mat->SetScalarParameterValue(TEXT("Roughness"), Rough); Mat->SetScalarParameterValue(TEXT("NativeLighting"), 1);
		return Mat;
	};
	// a dun brown bird, a grizzled tan coyote; the eyes dark until a light finds them
	Feather = Tint(FLinearColor(.16f, .13f, .10f), .85f);
	Fur = Tint(FLinearColor(.33f, .28f, .21f), .9f);
	UMaterialInstanceDynamic* Glow = Tint(FLinearColor(.04f, .04f, .03f), .3f);
	Glow->SetVectorParameterValue(TEXT("Emissive"), FLinearColor::Black); Glow->SetScalarParameterValue(TEXT("EmissivePower"), 0.f);
	EyeGlow = Glow;
	int32 Serial = 0;
	auto Make = [&Serial, Web, Holder](UStaticMesh* Mesh, UMaterialInterface* Mat, bool bShadow)
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Web, *FString::Printf(TEXT("S47Animal_%d"), Serial++));
		Part->SetupAttachment(Holder); Part->SetMobility(EComponentMobility::Movable); Part->SetStaticMesh(Mesh); Part->SetMaterial(0, Mat);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision); Part->SetCastShadow(bShadow);
		Part->SetVisibility(false); Part->SetHiddenInGame(true); Part->RegisterComponent();
		return Part;
	};
	for (FBird& Bird : Covey.Birds) { Bird.Body = Make(CubeMesh, Feather.Get(), false); Bird.WingL = Make(CubeMesh, Feather.Get(), false); Bird.WingR = Make(CubeMesh, Feather.Get(), false); }
	FCoyote& Cy = Coyote;
	Cy.Torso = Make(CubeMesh, Fur.Get(), true); Cy.Neck = Make(CubeMesh, Fur.Get(), true); Cy.Head = Make(CubeMesh, Fur.Get(), true);
	Cy.Snout = Make(CubeMesh, Fur.Get(), true); Cy.EarL = Make(CubeMesh, Fur.Get(), false); Cy.EarR = Make(CubeMesh, Fur.Get(), false);
	Cy.Tail = Make(CubeMesh, Fur.Get(), true);
	for (int32 I = 0; I < 4; ++I) Cy.Legs[I] = Make(CubeMesh, Fur.Get(), true);
	Cy.EyeL = Make(BallMesh, Glow, false); Cy.EyeR = Make(BallMesh, Glow, false);
	// the rigged ones, where this build has them: a dun coyote with a pale belly, a brown ranch dog with a cream chest
	BuildRig(CoyoteRig, TEXT("Coyote"), 70, Holder, Fur.Get(), Tint(FLinearColor(.62f, .55f, .44f), .9f), Tint(FLinearColor(.03f, .03f, .03f), .6f));
	BuildRig(DogRig, TEXT("Dog"), 62, Holder, Tint(FLinearColor(.17f, .11f, .07f), .9f), Tint(FLinearColor(.66f, .58f, .47f), .9f), Tint(FLinearColor(.03f, .03f, .03f), .6f));
	return true;
}

bool FS47Animals::BuildRig(FRig& Rig, const TCHAR* Animal, double HeightCm, USceneComponent* Holder, UMaterialInterface* Coat, UMaterialInterface* Pale, UMaterialInterface* Dark)
{
	Rig.bTried = true; Rig.Animal = Animal; Rig.bReady = false;
	const FString Package = FString::Printf(TEXT("/Game/Free/Animals/%s/SK_%s"), Animal, Animal);
	if (!FPackageName::DoesPackageExist(Package) || !M || !M->Web || !Holder) return false;
	USkeletalMesh* Body = LoadObject<USkeletalMesh>(nullptr, *(Package + TEXT(".SK_") + Animal));
	if (!Body) return false;
	// which way it faces and how tall it is, from its own skeleton and bounds
	const FReferenceSkeleton& Ref = Body->GetRefSkeleton();
	const int32 HeadIndex = Ref.FindBoneIndex(TEXT("Head")), BodyIndex = Ref.FindBoneIndex(TEXT("Body"));
	const FBoxSphereBounds Bounds = Body->GetBounds();
	const double Tall = Bounds.BoxExtent.Z * 2;
	if (HeadIndex == INDEX_NONE || BodyIndex == INDEX_NONE || Tall < 1)
	{
		UE_LOG(LogS47Animals, Warning, TEXT("S47: animals %s rig unusable (head %d, body %d, height %.1f cm): the boxes stay"), Animal, HeadIndex, BodyIndex, Tall);
		return false;
	}
	const FVector HeadAt = FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref, HeadIndex).GetLocation();
	const FVector BodyAt = FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref, BodyIndex).GetLocation();
	const FVector Ahead = FVector(HeadAt.X - BodyAt.X, HeadAt.Y - BodyAt.Y, 0).GetSafeNormal();
	if (Ahead.IsNearlyZero()) return false;
	Rig.Scale = HeightCm / Tall;
	Rig.YawOffset = FMath::RadiansToDegrees(FMath::Atan2(Ahead.Y, Ahead.X));
	Rig.Lift = -(Bounds.Origin.Z - Bounds.BoxExtent.Z) * Rig.Scale;
	Rig.Head = TEXT("Head");
	USkeletalMeshComponent* Part = NewObject<USkeletalMeshComponent>(M->Web.Get(), *FString::Printf(TEXT("S47Animal_%s"), Animal));
	Part->SetupAttachment(Holder); Part->SetMobility(EComponentMobility::Movable);
	Part->SetSkinnedAssetAndUpdate(Body);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision); Part->SetCastShadow(true);
	Part->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Part->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	// its colours, by the source's own slots: the coat, the pale under it, a dark nose; the eyes the ones that shine
	const TArray<FSkeletalMaterial>& Slots = Body->GetMaterials();
	for (int32 I = 0; I < Slots.Num(); ++I)
	{
		const FString Slot = Slots[I].MaterialSlotName.ToString();
		UMaterialInterface* Mat = Coat;
		if (Slot.Contains(TEXT("Eye")) || Slot.EndsWith(TEXT("002"))) Mat = EyeGlow.Get();
		else if (Slot.Contains(TEXT("Light")) || Slot.EndsWith(TEXT("003"))) Mat = Pale;
		else if (Slot.Contains(TEXT("Nose")) || Slot.EndsWith(TEXT("006"))) Mat = Dark;
		else if (Slot.EndsWith(TEXT("001"))) Mat = Pale;
		if (Mat) Part->SetMaterial(I, Mat);
	}
	Part->SetVisibility(false); Part->SetHiddenInGame(true);
	Part->RegisterComponent();
	Rig.Mesh = Part; Rig.bReady = true;
	UE_LOG(LogS47Animals, Display, TEXT("S47: animals %s rigged: %d bones, %.0f cm tall as imported, scale %.3f, faces %.0f deg, %d slots"),
		Animal, Ref.GetNum(), Tall, Rig.Scale, Rig.YawOffset, Slots.Num());
	return true;
}

void FS47Animals::PlayRig(FRig& Rig, const TCHAR* Clip, double Rate)
{
	USkeletalMeshComponent* Part = Rig.Mesh.Get();
	if (!Rig.bReady || !Part) return;
	if (Rig.Playing == Clip)
	{
		if (!FMath::IsNearlyEqual(Rig.Rate, Rate, 0.02)) { Rig.Rate = Rate; Part->SetPlayRate(float(Rate)); }
		return;
	}
	FString Name = FString::Printf(TEXT("A_%s_%s"), *Rig.Animal, Clip);
	FString Package = FString::Printf(TEXT("/Game/Free/Animals/%s/%s"), *Rig.Animal, *Name);
	if (!FPackageName::DoesPackageExist(Package))
	{
		// a clip this one has not got: it stands
		Name = FString::Printf(TEXT("A_%s_Idle"), *Rig.Animal);
		Package = FString::Printf(TEXT("/Game/Free/Animals/%s/%s"), *Rig.Animal, *Name);
		if (!FPackageName::DoesPackageExist(Package)) return;
	}
	UAnimSequence* Seq = LoadObject<UAnimSequence>(nullptr, *(Package + TEXT(".") + Name));
	if (!Seq) return;
	Part->PlayAnimation(Seq, true);
	Part->SetPlayRate(float(Rate));
	Rig.Playing = Clip; Rig.Rate = Rate;
}

void FS47Animals::PlaceRig(FRig& Rig, const FVector& FeetThree, double YawDeg)
{
	USkeletalMeshComponent* Part = Rig.Mesh.Get();
	if (!Rig.bReady || !Part) return;
	Part->SetWorldLocationAndRotation(S47Three::ToUE(FeetThree) + FVector(0, 0, Rig.Lift), FRotator(0, YawDeg - Rig.YawOffset, 0));
	Part->SetWorldScale3D(FVector(Rig.Scale));
}

void FS47Animals::Show(UStaticMeshComponent* Part, bool bOn) const
{
	if (!Part) return;
	if (Part->IsVisible() != bOn) Part->SetVisibility(bOn);
	Part->SetHiddenInGame(!bOn);
}

void FS47Animals::Put(UStaticMeshComponent* Part, const FVector& AtUE, const FRotator& Rot, const FVector& Size) const
{
	if (!Part) return;
	Part->SetWorldLocationAndRotation(AtUE, Rot);
	Part->SetWorldScale3D(Size);
}

void FS47Animals::Reset()
{
	for (FBird& Bird : Covey.Birds) { Show(Bird.Body.Get(), false); Show(Bird.WingL.Get(), false); Show(Bird.WingR.Get(), false); }
	FCoyote& Cy = Coyote;
	for (UStaticMeshComponent* Part : { Cy.Torso.Get(), Cy.Neck.Get(), Cy.Head.Get(), Cy.Snout.Get(), Cy.EarL.Get(), Cy.EarR.Get(), Cy.Tail.Get(), Cy.EyeL.Get(), Cy.EyeR.Get() }) Show(Part, false);
	for (const TWeakObjectPtr<UStaticMeshComponent>& Leg : Cy.Legs) Show(Leg.Get(), false);
	for (FRig* Rig : { &CoyoteRig, &DogRig })
		if (USkeletalMeshComponent* Body = Rig->Mesh.Get()) { Body->SetVisibility(false); Body->SetHiddenInGame(true); }
	DogShown = INDEX_NONE;
	Covey.Mode = ECovey::None; Covey.Wait = 20;
	Cy.Mode = ECoyote::None; Cy.Wait = 90;
	for (FDog& Dog : Dogs)
	{
		if (Dog.Playing.IsValid() && M && M->Audio && M->Audio->Ctx) Dog.Playing->Stop(M->Audio->Ctx->CurrentTime());
		Dog.Playing.Reset(); Dog.Left = 0; Dog.bStopped = false; Dog.bFrantic = false; Dog.Quiet = 0;
	}
	DevCoveyIn = DevCoyoteIn = DevDogIn = -1;
}

// ---------- the covey ----------

void FS47Animals::PlaceCovey(const FVector& Near, const FVector& Ahead)
{
	FCovey& Cv = Covey;
	const FVector Forward = FVector(Ahead.X, 0, Ahead.Z).GetSafeNormal();
	const FVector Way = S47AnimalsLocal::Turned(Forward.IsNearlyZero() ? FVector(1, 0, 0) : Forward, FMath::FRandRange(-60.f, 60.f));
	FVector Spot = Near + Way * FMath::FRandRange(16.f, 30.f);
	// not by a building (a door, a window, the station's own doors), and so never in the middle of town
	FVector2D RoadAt;
	if (M->Freeform && M->Freeform->EventPlaces && RoadOf(Spot, RoadAt) && M->Freeform->EventPlaces->NearestTo(RoadAt, 22, ES47PlaceTrait::Indoors))
	{
		Cv.Wait = 12; return;
	}
	Spot.Y = GroundAt(Spot);
	Cv.At = Spot; Cv.Mode = ECovey::Sitting; Cv.Age = 0;
}

void FS47Animals::Flush(const FVector& From, bool bDanger)
{
	FCovey& Cv = Covey;
	Cv.Mode = ECovey::Flying; Cv.Age = 0;
	++FlushCount; if (bDanger) ++DangerFlushes;
	const FVector Away = FleeDir(From, Cv.At);
	for (FBird& Bird : Cv.Birds)
	{
		Bird.From = Cv.At + FVector(FMath::FRandRange(-1.5f, 1.5f), 0.15, FMath::FRandRange(-1.5f, 1.5f));
		Bird.Dir = S47AnimalsLocal::Turned(Away, FMath::FRandRange(-28.f, 28.f));
		Bird.Speed = FMath::FRandRange(11.f, 16.f); Bird.Lift = FMath::FRandRange(1.4f, 2.8f); Bird.Phase = FMath::FRandRange(0.f, 6.28f);
	}
	PoseBirds();
	if (M->Audio && M->Audio->Night) M->Audio->Night->Whirr(EyeNow(), Cv.At + FVector(0, 0.6, 0), bDanger ? 1.0 : 0.8);
	// right under the feet in the dark, it makes anyone jump
	if (!bDanger && NightNow() && M->Stress) M->Stress->Startle(0.3);
	if (bLog)
	{
		UE_LOG(LogS47Animals, Display, TEXT("S47: animals covey up%s, %.0f m from the player, flying toward %.0f deg"), bDanger ? TEXT(" with nobody near") : TEXT(""),
			S47AnimalsLocal::Flat(Cv.At, EyeNow()), S47AnimalsLocal::YawOf(Away));
	}
}

void FS47Animals::PoseBirds()
{
	FCovey& Cv = Covey;
	const FVector Up(0, 0, 1);
	for (FBird& Bird : Cv.Birds)
	{
		const bool bAir = Cv.Mode == ECovey::Flying && Cv.Age <= S47AnimalsLocal::FlightSeconds;
		Show(Bird.Body.Get(), bAir); Show(Bird.WingL.Get(), bAir); Show(Bird.WingR.Get(), bAir);
		if (!bAir) continue;
		// up fast, a long low glide, then down into the brush and gone
		const double Climb = Bird.Lift * FMath::Sin(FMath::Min(1.0, Cv.Age / 0.45) * UE_DOUBLE_PI * 0.5);
		const double Sink = FMath::Clamp((Cv.Age - (S47AnimalsLocal::FlightSeconds - 0.8)) / 0.8, 0.0, 1.0);
		const FVector Pos = Bird.From + Bird.Dir * Bird.Speed * Cv.Age + FVector(0, Climb * (1.0 - Sink), 0);
		const double YawDeg = S47AnimalsLocal::YawOf(Bird.Dir);
		const FVector PosUE = S47Three::ToUE(Pos);
		const FVector Side = FVector::CrossProduct(Up, S47Three::DirToUE(Bird.Dir)).GetSafeNormal();
		Put(Bird.Body.Get(), PosUE, FRotator(0, YawDeg, 0), FVector(.17, .09, .08));
		// the wings whirr, up and down fast, then are held out in the glide
		const double Flap = Cv.Age < 1.6 ? 55.0 * FMath::Sin(Bird.Phase + Cv.Age * 62.0) : 8.0 * FMath::Sin(Bird.Phase + Cv.Age * 6.0);
		const double Rad = FMath::DegreesToRadians(Flap);
		for (int32 Hand = -1; Hand <= 1; Hand += 2)
		{
			const FVector WingAt = PosUE + Side * (Hand * 6.5 * FMath::Cos(Rad)) + Up * (6.5 * FMath::Sin(Rad));
			Put((Hand < 0 ? Bird.WingL : Bird.WingR).Get(), WingAt, FRotator(0, YawDeg, -Hand * Flap), FVector(.07, .13, .012));
		}
	}
}

void FS47Animals::StepCovey(double Dt)
{
	FCovey& Cv = Covey;
	const FVector Stand(M->Player->Pos.X, M->Player->FloorY, M->Player->Pos.Z);
	switch (Cv.Mode)
	{
	case ECovey::None:
		if (!Outdoors()) return;
		if ((Cv.Wait -= Dt) > 0) return;
		PlaceCovey(Stand, FS47Cam::Of(M->Player).Forward);
		return;
	case ECovey::Sitting:
	{
		const double Dist = S47AnimalsLocal::Flat(Stand, Cv.At);
		if (Dist > S47AnimalsLocal::CoveyRelocate || !Outdoors()) { Cv.Mode = ECovey::None; Cv.Wait = FMath::FRandRange(20.f, 60.f); return; }
		FVector From = FVector::ZeroVector;
		const double Felt = M->Omen ? M->Omen->SenseAt(Cv.At, S47AnimalsLocal::CoveyWiden, &From, nullptr) : 0;
		if (Fear(Felt) >= 1) { Flush(From, true); return; }
		if (Dist < FlushRange(M->Player->Sneak(), FeetSpeed > 3.2)) Flush(Stand, false);
		return;
	}
	case ECovey::Flying:
		Cv.Age += Dt;
		PoseBirds();
		if (Cv.Age > S47AnimalsLocal::FlightSeconds + 0.6) { Cv.Mode = ECovey::Gone; Cv.Wait = FMath::FRandRange(60.f, 160.f); PoseBirds(); }
		return;
	case ECovey::Gone:
		if ((Cv.Wait -= Dt) <= 0) { Cv.Mode = ECovey::None; Cv.Wait = FMath::FRandRange(30.f, 120.f); }
		return;
	}
}

// ---------- the coyote ----------

bool FS47Animals::StartCrossing(bool bAhead, double Dist)
{
	FCoyote& Cy = Coyote;
	const FVector Eye = EyeNow();
	FVector Forward = FS47Cam::Of(M->Player).Forward; Forward.Y = 0;
	Forward = Forward.IsNearlyZero() ? FVector(1, 0, 0) : Forward.GetSafeNormal();
	const FVector Look = bAhead ? S47AnimalsLocal::Turned(Forward, FMath::FRandRange(-30.f, 30.f)) : S47AnimalsLocal::Turned(Forward, FMath::FRandRange(60.f, 300.f));
	const FVector Mid = FVector(Eye.X, 0, Eye.Z) + Look * Dist;
	const FVector Across = S47AnimalsLocal::Turned(Look, FMath::RandBool() ? 90.0 : -90.0);
	FVector Start = Mid - Across * (S47AnimalsLocal::CoyoteCross * 0.5);
	FVector2D RoadAt;
	if (M->Freeform && M->Freeform->EventPlaces && RoadOf(Start, RoadAt) && M->Freeform->EventPlaces->NearestTo(RoadAt, 25, ES47PlaceTrait::Indoors)) return false;
	Start.Y = GroundAt(FVector(Start.X, M->Player->FloorY, Start.Z));
	Cy.Mode = ECoyote::Trot; Cy.At = Start; Cy.Dir = Across; Cy.Age = 0; Cy.Left = S47AnimalsLocal::CoyoteCross / S47AnimalsLocal::TrotSpeed;
	Cy.Gait = FMath::FRandRange(0.f, 6.f); Cy.Ground = Start.Y; Cy.GroundIn = 0.2; Cy.Yaw = S47AnimalsLocal::YawOf(Across); Cy.HeadYaw = 0; Cy.bLooked = false; Cy.Shine = 0;
	PoseCoyote(0);
	return true;
}

double FS47Animals::ShineOf(const FVector& EyeAt, const FVector& Facing) const
{
	if (!M || !M->Player) return 0;
	const FS47Cam Cam = FS47Cam::Of(M->Player);
	const FVector ToViewer = Cam.Pos - EyeAt;
	const double Dist = ToViewer.Size();
	if (Dist < 0.5) return 0;
	// it shines only looking toward the light
	const FVector Back = FVector(ToViewer.X, 0, ToViewer.Z).GetSafeNormal();
	const double Toward = FMath::SmoothStep(0.55, 0.95, FVector::DotProduct(Facing, Back));
	if (Toward <= 0) return 0;
	const double Aligned = FVector::DotProduct(Cam.Forward, (EyeAt - Cam.Pos) / Dist);
	if (M->World && M->World->bDriving)
		return Toward * FMath::SmoothStep(FMath::Cos(FMath::DegreesToRadians(15.0)), FMath::Cos(FMath::DegreesToRadians(4.0)), Aligned) * FMath::Clamp(1.0 - Dist / 130.0, 0.0, 1.0);
	const FS47Flashlight* Lamp = M->Night && M->Night->Flashlight ? M->Night->Flashlight.Get() : nullptr;
	if (!Lamp || Lamp->Intensity() <= 1) return 0;
	return Toward * FMath::SmoothStep(FMath::Cos(FMath::DegreesToRadians(20.0)), FMath::Cos(FMath::DegreesToRadians(5.0)), Aligned)
		* FMath::Clamp(1.0 - Dist / 70.0, 0.0, 1.0) * FMath::Clamp(Lamp->Intensity() / 64.0, 0.2, 1.0);
}

void FS47Animals::PoseCoyote(double Dt)
{
	using namespace S47AnimalsLocal;
	FCoyote& Cy = Coyote;
	const bool bOut = Cy.Mode != ECoyote::None;
	const bool bRigged = CoyoteRig.bReady && CoyoteRig.Mesh.IsValid();
	for (UStaticMeshComponent* Part : { Cy.Torso.Get(), Cy.Neck.Get(), Cy.Head.Get(), Cy.Snout.Get(), Cy.EarL.Get(), Cy.EarR.Get(), Cy.Tail.Get(), Cy.EyeL.Get(), Cy.EyeR.Get() }) Show(Part, bOut && !bRigged);
	for (const TWeakObjectPtr<UStaticMeshComponent>& Leg : Cy.Legs) Show(Leg.Get(), bOut && !bRigged);
	if (bRigged)
	{
		USkeletalMeshComponent* Part = CoyoteRig.Mesh.Get();
		if (Part->IsVisible() != bOut) Part->SetVisibility(bOut);
		Part->SetHiddenInGame(!bOut);
	}
	if (!bOut) return;
	if (bRigged)
	{
		// The model: the whole of it turns to what it stares at; it walks a trot, runs a gallop, stands with its head up.
		const bool bStill = Cy.Mode == ECoyote::Stare || Cy.Mode == ECoyote::Look;
		double Want = YawOf(Cy.Dir);
		if (bStill)
		{
			const FVector ToIt(Cy.LookAt.X - Cy.At.X, 0, Cy.LookAt.Z - Cy.At.Z);
			if (!ToIt.IsNearlyZero()) Want = YawOf(ToIt);
		}
		Cy.Yaw += FMath::UnwindDegrees(Want - Cy.Yaw) * FMath::Min(1.0, Dt * (Cy.Mode == ECoyote::Stare ? 8.0 : 4.0));
		Cy.HeadYaw = 0;
		if (Cy.Mode == ECoyote::Bolt) PlayRig(CoyoteRig, TEXT("Gallop"), 1.15);
		else if (Cy.Mode == ECoyote::Trot) PlayRig(CoyoteRig, TEXT("Walk"), 1.7);
		else if (Cy.Mode == ECoyote::Stare) PlayRig(CoyoteRig, TEXT("Idle"), 1.0);
		else PlayRig(CoyoteRig, TEXT("Idle2"), 1.0);
		PlaceRig(CoyoteRig, Cy.At, Cy.Yaw);
		const double YawRad0 = FMath::DegreesToRadians(Cy.Yaw);
		const FVector Facing0(FMath::Cos(YawRad0), 0, FMath::Sin(YawRad0));
		const FVector EyeAt0 = S47Three::ToThree(CoyoteRig.Mesh->GetSocketLocation(CoyoteRig.Head)) + Facing0 * 0.12;
		const double Shine0 = ShineOf(EyeAt0, Facing0);
		if (FMath::Abs(Shine0 - Cy.Shine) > 0.01 || (Shine0 <= 0 && Cy.Shine > 0))
		{
			Cy.Shine = Shine0;
			if (UMaterialInstanceDynamic* Glow0 = EyeGlow.Get())
			{
				Glow0->SetVectorParameterValue(TEXT("Emissive"), Shine0 > 0.02 ? FLinearColor(.75f, .95f, .4f) * float(90.0 * Shine0) : FLinearColor::Black);
				Glow0->SetScalarParameterValue(TEXT("EmissivePower"), Shine0 > 0.02 ? 1.f : 0.f);
			}
		}
		return;
	}
	// the body turns to where it goes; the head to what it looks at, and the body after it past a quarter turn
	const bool bHeld = Cy.Mode == ECoyote::Stare || Cy.Mode == ECoyote::Look;
	double WantYaw = YawOf(Cy.Dir), WantHead = 0;
	if (bHeld)
	{
		const FVector ToIt(Cy.LookAt.X - Cy.At.X, 0, Cy.LookAt.Z - Cy.At.Z);
		const double Aim = YawOf(ToIt.IsNearlyZero() ? Cy.Dir : ToIt);
		const double Rel = FMath::UnwindDegrees(Aim - Cy.Yaw);
		WantHead = FMath::Clamp(Rel, -80.0, 80.0);
		WantYaw = Cy.Yaw + (Rel - WantHead);
	}
	const double Ease = FMath::Min(1.0, Dt * (Cy.Mode == ECoyote::Stare ? 9.0 : 4.0));
	Cy.Yaw += FMath::UnwindDegrees(WantYaw - Cy.Yaw) * FMath::Min(1.0, Dt * 4.0);
	Cy.HeadYaw += (WantHead - Cy.HeadYaw) * Ease;
	const double YawRad = FMath::DegreesToRadians(Cy.Yaw), HeadRad = FMath::DegreesToRadians(Cy.HeadYaw);
	const FVector Fwd(FMath::Cos(YawRad), FMath::Sin(YawRad), 0), Lateral(-FMath::Sin(YawRad), FMath::Cos(YawRad), 0), Up(0, 0, 1);
	const FVector Planted = S47Three::ToUE(Cy.At);
	auto Body = [&](double Fd, double Sd, double Ud) { return Planted + (Fwd * Fd + Lateral * Sd + Up * Ud) * 100.0; };
	// the head and what is on it turn about the neck
	auto Skull = [&](double Fd, double Sd, double Ud) { const double Rf = Fd - NeckF; return Body(NeckF + Rf * FMath::Cos(HeadRad) - Sd * FMath::Sin(HeadRad), Rf * FMath::Sin(HeadRad) + Sd * FMath::Cos(HeadRad), Ud); };
	const double Bob = Cy.Mode == ECoyote::Bolt ? 0.05 * FMath::Abs(FMath::Sin(Cy.Gait)) : Cy.Mode == ECoyote::Trot ? 0.02 * FMath::Abs(FMath::Sin(Cy.Gait)) : 0.0;
	const double EarsUp = Cy.Mode == ECoyote::Stare ? 0.02 : 0.0;
	Put(Cy.Torso.Get(), Body(TorsoPart.F, TorsoPart.S, TorsoPart.U + Bob), FRotator(0, Cy.Yaw, 0), FVector(TorsoPart.L, TorsoPart.W, TorsoPart.H));
	Put(Cy.Neck.Get(), Body(NeckPart.F, NeckPart.S, NeckPart.U + Bob), FRotator(Cy.Mode == ECoyote::Bolt ? 15.0 : 35.0, Cy.Yaw + Cy.HeadYaw * 0.5, 0), FVector(NeckPart.L, NeckPart.W, NeckPart.H));
	const FRotator HeadRot(0, Cy.Yaw + Cy.HeadYaw, 0);
	Put(Cy.Head.Get(), Skull(HeadPart.F, HeadPart.S, HeadPart.U + Bob), HeadRot, FVector(HeadPart.L, HeadPart.W, HeadPart.H));
	Put(Cy.Snout.Get(), Skull(SnoutPart.F, SnoutPart.S, SnoutPart.U + Bob), HeadRot, FVector(SnoutPart.L, SnoutPart.W, SnoutPart.H));
	Put(Cy.EarL.Get(), Skull(EarPart.F, -EarPart.S, EarPart.U + Bob + EarsUp), HeadRot, FVector(EarPart.L, EarPart.W, EarPart.H));
	Put(Cy.EarR.Get(), Skull(EarPart.F, EarPart.S, EarPart.U + Bob + EarsUp), HeadRot, FVector(EarPart.L, EarPart.W, EarPart.H));
	Put(Cy.EyeL.Get(), Skull(EyePart.F, -EyePart.S, EyePart.U + Bob), HeadRot, FVector(EyePart.L));
	Put(Cy.EyeR.Get(), Skull(EyePart.F, EyePart.S, EyePart.U + Bob), HeadRot, FVector(EyePart.L));
	Put(Cy.Tail.Get(), Body(TailPart.F, TailPart.S, TailPart.U + Bob), FRotator(Cy.Mode == ECoyote::Bolt ? 12.0 : Cy.Mode == ECoyote::Stare ? 45.0 : 35.0, Cy.Yaw, 0), FVector(TailPart.L, TailPart.W, TailPart.H));
	// a trot is diagonal pairs; a run, all of them further; standing, straight
	const double Stride = Cy.Mode == ECoyote::Bolt ? 40.0 : Cy.Mode == ECoyote::Trot ? 25.0 : 0.0;
	const double Phases[4] = { 0, UE_DOUBLE_PI, UE_DOUBLE_PI, 0 };
	const double LegF[4] = { HipF, HipF, -HipF, -HipF }, LegS[4] = { -HipS, HipS, -HipS, HipS };
	for (int32 I = 0; I < 4; ++I)
	{
		const double Angle = Stride * FMath::Sin(Cy.Gait + Phases[I]);
		const double Rad = FMath::DegreesToRadians(Angle);
		Put(Cy.Legs[I].Get(), Body(LegF[I] + 0.5 * LegLen * FMath::Sin(Rad), LegS[I], HipU + Bob - 0.5 * LegLen * FMath::Cos(Rad)), FRotator(Angle, Cy.Yaw, 0), FVector(.06, .06, LegLen));
	}
	// the eyes: dark, unless the lamp or the headlights find them looking back
	const FVector EyeThree = S47Three::ToThree(Skull(EyePart.F, 0, EyePart.U + Bob));
	const double HeadYawAll = FMath::DegreesToRadians(Cy.Yaw + Cy.HeadYaw);
	const FVector Facing(FMath::Cos(HeadYawAll), 0, FMath::Sin(HeadYawAll));
	const double Shine = ShineOf(EyeThree, Facing);
	if (FMath::Abs(Shine - Cy.Shine) > 0.01 || (Shine <= 0 && Cy.Shine > 0))
	{
		Cy.Shine = Shine;
		if (UMaterialInstanceDynamic* Glow = EyeGlow.Get())
		{
			Glow->SetVectorParameterValue(TEXT("Emissive"), Shine > 0.02 ? FLinearColor(.75f, .95f, .4f) * float(90.0 * Shine) : FLinearColor::Black);
			Glow->SetScalarParameterValue(TEXT("EmissivePower"), Shine > 0.02 ? 1.f : 0.f);
		}
	}
}

void FS47Animals::StepCoyote(double Dt)
{
	using namespace S47AnimalsLocal;
	FCoyote& Cy = Coyote;
	const FVector Eye = EyeNow();
	const bool bAbout = Outdoors() || (M->World && M->World->bDriving);
	bool bInTown = false;
	{
		FVector2D RoadAt;
		if (M->Freeform && M->Freeform->EventPlaces && RoadOf(Eye, RoadAt))
			bInTown = M->Freeform->EventPlaces->NearestTo(RoadAt, 90, ES47PlaceTrait::Town) != nullptr;
	}
	DangerSpawnIn = FMath::Max(0.0, DangerSpawnIn - Dt);
	if (Cy.Mode == ECoyote::None)
	{
		if (!bAbout || !NightNow() || bInTown) return;
		// Something comes, and a coyote that was out there in the dark stops where the lamp can find it: now and then, not always.
		if (M->Omen && M->Omen->SinceOnset() < 0.6 && DangerSpawnIn <= 0 && Flat(M->Omen->OnsetFrom(), Eye) < 400)
		{
			DangerSpawnIn = 600;
			if (FMath::FRand() < 0.6f && StartCrossing(true, FMath::FRandRange(30.f, 42.f)))
			{
				Cy.Mode = ECoyote::Stare; Cy.LookAt = M->Omen->OnsetFrom(); Cy.Left = FMath::FRandRange(3.f, 6.f); ++StareCount;
			}
			return;
		}
		if ((Cy.Wait -= Dt) > 0) return;
		Cy.Wait = FMath::FRandRange(150.f, 420.f);
		StartCrossing(true, FMath::FRandRange(32.f, 50.f));
		return;
	}
	Cy.Age += Dt;
	const double ToPlayer = Flat(Cy.At, Eye);
	auto Gone = [&Cy]() { Cy.Mode = ECoyote::None; Cy.Wait = FMath::FRandRange(150.f, 420.f); };
	if (ToPlayer > CoyoteGone || (!bAbout && Cy.Mode != ECoyote::Bolt)) { Gone(); PoseCoyote(Dt); return; }
	FVector From = FVector::ZeroVector;
	const double Felt = M->Omen ? M->Omen->SenseAt(Cy.At, CoyoteWiden, &From, nullptr) : 0;
	switch (Cy.Mode)
	{
	case ECoyote::Trot:
	case ECoyote::Look:
		if (Fear(Felt) >= 1)
		{
			// it stops dead and looks at it
			Cy.Mode = ECoyote::Stare; Cy.LookAt = From; Cy.Left = Fear(Felt) >= 2 ? FMath::FRandRange(2.5f, 4.5f) : FMath::FRandRange(4.f, 7.f); ++StareCount;
			if (bLog)
			{
				UE_LOG(LogS47Animals, Display, TEXT("S47: animals coyote stares, %.0f m from the player, at something %.0f m off it"), ToPlayer, Flat(Cy.At, From));
			}
			break;
		}
		if (ToPlayer < CoyoteShy) { Cy.Mode = ECoyote::Bolt; Cy.Dir = FleeDir(Eye, Cy.At); Cy.Left = 6; break; }
		if (Cy.Mode == ECoyote::Trot)
		{
			Cy.At += Cy.Dir * TrotSpeed * Dt; Cy.Gait += Dt * 9.5;
			// the lamp on it: once, it may stop and look round into it
			if (!Cy.bLooked && ShineOf(Cy.At + FVector(0, EyePart.U, 0), FleeDir(Cy.At, Eye)) > 0.02 && FMath::FRand() < Dt * 1.2)
			{
				Cy.Mode = ECoyote::Look; Cy.LookAt = Eye; Cy.Left = FMath::FRandRange(1.4f, 2.8f); Cy.bLooked = true;
			}
			if ((Cy.Left -= Dt) <= 0) Gone();
		}
		else if ((Cy.Left -= Dt) <= 0) { Cy.Mode = ECoyote::Trot; Cy.Left = FMath::FRandRange(4.f, 8.f); }
		break;
	case ECoyote::Stare:
		if (Felt > 0) Cy.LookAt = From;
		if ((Cy.Left -= Dt) <= 0 || ToPlayer < CoyoteShy)
		{
			// then away from it, flat out
			Cy.Dir = FleeDir(ToPlayer < CoyoteShy ? Eye : Cy.LookAt, Cy.At); Cy.Mode = ECoyote::Bolt; Cy.Left = 6;
		}
		break;
	case ECoyote::Bolt:
		Cy.At += Cy.Dir * BoltSpeed * Dt; Cy.Gait += Dt * 16.0;
		if ((Cy.Left -= Dt) <= 0) Gone();
		break;
	case ECoyote::None:
		break;
	}
	if ((Cy.GroundIn -= Dt) <= 0) { Cy.GroundIn = 0.2; Cy.Ground = GroundAt(FVector(Cy.At.X, Cy.Ground, Cy.At.Z)); }
	Cy.At.Y = Cy.Ground;
	PoseCoyote(Dt);
}

// ---------- the dogs ----------

void FS47Animals::Bark(FDog& Dog, bool bFrantic)
{
	if (!M->Audio || !M->Audio->Night) return;
	const FVector Home = FromRoad(Dog.Home, 0.6);
	Dog.Playing = M->Audio->Night->PlayAt(FMath::RandBool() ? TEXT("dog0") : TEXT("dog1"), EyeNow(), Home, bFrantic ? 1.0 : 0.85,
		bFrantic ? FMath::FRandRange(1.1f, 1.2f) : FMath::FRandRange(0.95f, 1.05f), 35);
	++BarkCount;
}

void FS47Animals::StepDogs(double Dt)
{
	using namespace S47AnimalsLocal;
	if (Dogs.IsEmpty()) for (const FVector2D& Home : DogHomes) { FDog Dog; Dog.Home = Home; Dog.Next = FMath::FRandRange(5.f, 30.f); Dogs.Add(Dog); }
	FVector2D From;
	if (!RoadOf(EyeNow(), From)) return;
	const double Hr = Hour();
	const bool bNight = Hr >= 19.0 || Hr < 5.5;
	// coyotes heard just now: every dog in earshot answers them
	const int32 Calls = M->Audio && M->Audio->Night ? M->Audio->Night->FarHeard(TEXT("coyote")) : 0;
	const bool bAnswer = Calls > LastCoyoteCalls;
	LastCoyoteCalls = Calls;
	const double Now0 = M->Audio && M->Audio->Ctx ? M->Audio->Ctx->CurrentTime() : 0;
	for (FDog& Dog : Dogs)
	{
		const double Away = FVector2D::Distance(From, Dog.Home);
		if (Away > DogHear || !bNight) { Dog.Left = 0; continue; }
		Dog.Quiet = FMath::Max(0.0, Dog.Quiet - Dt);
		const FVector Home = FromRoad(Dog.Home, 0.6);
		FVector Fright = FVector::ZeroVector;
		const double Felt = M->Omen ? M->Omen->SenseAt(Home, DogWiden, &Fright, nullptr) : 0;
		if (Felt > 0) Dog.Feared = Fright;
		if (Fear(Felt) >= 1 && !Dog.bStopped)
		{
			if (Dog.Left > 0 && !Dog.bFrantic)
			{
				// in the middle of a bark: cut off, and nothing more
				if (Dog.Playing.IsValid()) Dog.Playing->Stop(Now0 + 0.04);
				Dog.Left = 0; Dog.bStopped = true;
			}
			else if (!Dog.bFrantic && FMath::FRand() < 0.6f) { Dog.bFrantic = true; Dog.Left = 3; Dog.BarkIn = 0; }
			else if (!Dog.bFrantic) Dog.bStopped = true;
			if (Dog.bStopped)
			{
				++StopCount;
				if (Away < DogWhine && M->Audio && M->Audio->Night) M->Audio->Night->Whine(EyeNow(), Home);
				if (bLog)
				{
					UE_LOG(LogS47Animals, Display, TEXT("S47: animals the dog %.0f m off stops"), Away);
				}
			}
		}
		if (Fear(Felt) >= 1) Dog.Quiet = FMath::Max(Dog.Quiet, DogQuiet(Felt));
		if (Dog.bStopped)
		{
			if (Dog.Quiet <= 0) { Dog.bStopped = false; Dog.bFrantic = false; Dog.Next = FMath::FRandRange(20.f, 60.f); }
			continue;
		}
		// a bout of barking now and then: at the player coming close, at coyotes, at nothing much
		if (Dog.Left <= 0)
		{
			if (bAnswer) Dog.Next = FMath::Min(Dog.Next, double(FMath::FRandRange(2.f, 6.f)));
			if ((Dog.Next -= Dt) <= 0)
			{
				Dog.Left = FMath::RandRange(3, 8); Dog.BarkIn = 0;
				Dog.Next = Away < DogClose ? FMath::FRandRange(8.f, 20.f) : FMath::FRandRange(28.f, 75.f);
			}
			continue;
		}
		if ((Dog.BarkIn -= Dt) <= 0)
		{
			Bark(Dog, Dog.bFrantic);
			--Dog.Left;
			Dog.BarkIn = Dog.bFrantic ? FMath::FRandRange(0.16f, 0.24f) : FMath::FRandRange(0.35f, 0.75f);
			// the frantic ones are the last before it goes quiet
			if (Dog.bFrantic && Dog.Left <= 0)
			{
				Dog.bStopped = true; ++StopCount;
				if (Away < DogWhine && M->Audio && M->Audio->Night) M->Audio->Night->Whine(EyeNow(), Home);
				if (bLog)
				{
					UE_LOG(LogS47Animals, Display, TEXT("S47: animals the dog %.0f m off barks hard, then stops"), Away);
				}
			}
		}
	}
}

void FS47Animals::StepDogBody(double Dt)
{
	// The nearest dog within sight is there to be seen, rigged: it faces you and barks, or, when something has come, stands with its
	// head low facing it and makes no sound. Only one at a time is near enough to see.
	FRig& Rig = DogRig;
	USkeletalMeshComponent* Part = Rig.Mesh.Get();
	if (!Rig.bReady || !Part) return;
	FVector2D From;
	int32 Nearest = INDEX_NONE; double Best = 150;
	if (RoadOf(EyeNow(), From))
		for (int32 I = 0; I < Dogs.Num(); ++I) { const double Gap = FVector2D::Distance(From, Dogs[I].Home); if (Gap < Best) { Best = Gap; Nearest = I; } }
	const bool bShow = Nearest != INDEX_NONE && !(M->World && M->World->bDriving && Best > 60);
	if (Part->IsVisible() != bShow) Part->SetVisibility(bShow);
	Part->SetHiddenInGame(!bShow);
	if (!bShow) { DogShown = INDEX_NONE; return; }
	const FDog& Dog = Dogs[Nearest];
	FVector Feet = FromRoad(Dog.Home, 0);
	Feet.Y = GroundAt(FVector(Feet.X, Feet.Y + 1, Feet.Z));
	const FVector Eye = EyeNow();
	const FVector ToYou(Eye.X - Feet.X, 0, Eye.Z - Feet.Z);
	double Want = DogYaw;
	if (Dog.bStopped && !Dog.Feared.IsZero()) Want = S47AnimalsLocal::YawOf(FVector(Dog.Feared.X - Feet.X, 0, Dog.Feared.Z - Feet.Z));
	else if (Best < 80 && !ToYou.IsNearlyZero()) Want = S47AnimalsLocal::YawOf(ToYou);
	if (DogShown != Nearest) { DogYaw = Want; DogShown = Nearest; }
	DogYaw += FMath::UnwindDegrees(Want - DogYaw) * FMath::Min(1.0, Dt * 3.0);
	if (Dog.bStopped) PlayRig(Rig, TEXT("HeadLow"), 1.0);
	else if (Dog.Left > 0) PlayRig(Rig, TEXT("Attack"), Dog.bFrantic ? 1.4 : 1.0);
	else PlayRig(Rig, NightNow() ? TEXT("Idle") : TEXT("Idle2"), 1.0);
	PlaceRig(Rig, Feet, DogYaw);
}

// ---------- in play ----------

void FS47Animals::Step(double Dt)
{
	if (!bLogChecked) { bLogChecked = true; bLog = FParse::Param(FCommandLine::Get(), TEXT("S47AnimalsLog")); }
	if (!M || !M->IsFreeform() || M->Mode != US47Main::EMode::Play || !M->Player || !M->Web || !M->Freeform)
	{
		if (Covey.Mode != ECovey::None || Coyote.Mode != ECoyote::None || DogShown != INDEX_NONE) Reset();
		return;
	}
	if (M->bPausedByMenu || M->ModalOpen() || !FMath::IsFinite(Dt) || Dt <= 0) return;
	if (!Build()) return;
	const FString Area = M->Web->Area();
	if (Area != LastArea) { if (!LastArea.IsEmpty()) Reset(); LastArea = Area; }
	const FVector Stand(M->Player->Pos.X, M->Player->FloorY, M->Player->Pos.Z);
	FeetSpeed = LastFeet.IsZero() ? 0 : FMath::Lerp(FeetSpeed, S47AnimalsLocal::Flat(LastFeet, Stand) / Dt, FMath::Min(1.0, Dt * 4));
	LastFeet = Stand;
	// F10: what was put in place gets something real to react to, a moment later
	if (DevCoveyIn >= 0 && (DevCoveyIn -= Dt) < 0 && M->Omen)
	{
		FVector Behind = FS47Cam::Of(M->Player).Forward; Behind.Y = 0;
		M->Omen->DevPresence(EyeNow() - Behind.GetSafeNormal() * 55 + FVector(0, 1, 0), 0.7, 14, true);
	}
	if (DevCoyoteIn >= 0 && (DevCoyoteIn -= Dt) < 0 && M->Omen && Coyote.Mode != ECoyote::None)
		M->Omen->DevPresence(Coyote.At + S47AnimalsLocal::Turned(Coyote.Dir, 90) * 70 + FVector(0, 1, 0), 0.6, 16, false);
	if (DevDogIn >= 0 && (DevDogIn -= Dt) < 0 && M->Omen) M->Omen->DevPresence(DevDogAt, 0.8, 25, false);
	StepCovey(Dt);
	StepCoyote(Dt);
	StepDogs(Dt);
	StepDogBody(Dt);
}

bool FS47Animals::DevCovey(FString& Why, bool bThreat)
{
	if (!M || !M->IsFreeform() || !M->Player || !Build()) { Why = TEXT("Freeform only"); return false; }
	if (!Outdoors()) { Why = TEXT("outdoors on foot only"); return false; }
	const FVector Stand(M->Player->Pos.X, M->Player->FloorY, M->Player->Pos.Z);
	FVector Ahead = FS47Cam::Of(M->Player).Forward; Ahead.Y = 0; Ahead = Ahead.GetSafeNormal();
	FVector Spot = Stand + Ahead * 14;
	Spot.Y = GroundAt(Spot);
	Covey.At = Spot; Covey.Mode = ECovey::Sitting; Covey.Age = 0;
	DevCoveyIn = bThreat ? 1.5 : -1;
	Why = bThreat ? TEXT("a covey sits 14 m ahead; in a moment something comes up behind you, 55 m off, and they go up away from it")
		: TEXT("a covey sits in the brush 14 m ahead; walk on (or keep low and slow) and see how close you get");
	return true;
}

bool FS47Animals::DevCoyote(FString& Why, bool bThreat)
{
	if (!M || !M->IsFreeform() || !M->Player || !Build()) { Why = TEXT("Freeform only"); return false; }
	if (!(Outdoors() || (M->World && M->World->bDriving))) { Why = TEXT("outdoors only"); return false; }
	if (!StartCrossing(true, 32)) { Why = TEXT("too near a building here"); return false; }
	DevCoyoteIn = bThreat ? 2.5 : -1;
	Why = bThreat ? TEXT("a coyote crosses 32 m ahead (the lamp finds its eyes); in a moment something comes 70 m to one side of it: it stops and stares, then runs")
		: TEXT("a coyote crosses 32 m ahead with nothing near it; put the lamp on it and it may look round");
	return true;
}

bool FS47Animals::DevDog(FString& Why)
{
	FVector2D From;
	if (!M || !M->IsFreeform() || !M->Player || !RoadOf(EyeNow(), From)) { Why = TEXT("Freeform only, on the land"); return false; }
	if (Dogs.IsEmpty()) StepDogs(0.001);
	int32 Best = INDEX_NONE; double Nearest = S47AnimalsLocal::DogHear;
	for (int32 I = 0; I < Dogs.Num(); ++I) { const double Gap = FVector2D::Distance(From, Dogs[I].Home); if (Gap < Nearest) { Nearest = Gap; Best = I; } }
	if (Best == INDEX_NONE) { Why = TEXT("no dog within 900 m: the Alderete yard or Caliche"); return false; }
	if (!NightNow()) { Why = TEXT("the dogs bark at night, from 19:30"); return false; }
	FDog& Dog = Dogs[Best];
	Dog.bStopped = false; Dog.bFrantic = false; Dog.Quiet = 0; Dog.Next = 0; Dog.Left = 0;
	DevDogAt = FromRoad(Dog.Home + FVector2D(60, 0), 1);
	DevDogIn = 5;
	Why = FString::Printf(TEXT("the dog %.0f m off barks now; in five seconds something comes near it, and it stops"), Nearest);
	return true;
}

bool FS47Animals::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s animals: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bCond; };
	const FVector Away = FleeDir(FVector(0, 5, 0), FVector(10, 0, 0));
	Check(Away.Equals(FVector(1, 0, 0), 1e-6) && FleeDir(FVector(3, 0, 3), FVector(3, 1, 3)).Equals(FVector(1, 0, 0)), TEXT("they go flat away from it; standing on it, some way"));
	Check(FlushRange(0, false) == 7 && FlushRange(1, false) == 3.5 && FlushRange(0, true) > 10, TEXT("the covey lets a careful walker closer than a running one"));
	Check(Fear(0) == 0 && Fear(0.05) == 0 && Fear(0.2) == 1 && Fear(0.8) == 2, TEXT("nothing, uneasy, afraid"));
	Check(HerdLevel(0) == 0 && HerdLevel(0.2) > 0.05 && HerdLevel(0.2) < S47Herd::SCATTER && HerdLevel(0.9) >= S47Herd::SCATTER && HerdLevel(1) <= 1,
		TEXT("the cattle walk off together from something some way off, and run from it close by"));
	bool bRises = true; double Prev = 0;
	for (int32 I = 0; I <= 100; ++I) { const double Lv = HerdLevel(I / 100.0); bRises &= Lv >= Prev; Prev = Lv; }
	Check(bRises, TEXT("the nearer it is, the more the herd minds it"));
	Check(DogQuiet(1) > DogQuiet(0.2) && DogQuiet(0) >= 25, TEXT("the dog keeps quiet longer the worse it was"));
	FS47Animals Bare(nullptr); Bare.Step(1); Bare.Reset();
	FVector From = FVector::ZeroVector;
	Check(Bare.HerdFear(FVector::ZeroVector, From) == 0 && !Bare.CoyoteOut() && !Bare.CoveyHere() && Bare.Flushes() == 0, TEXT("without a game no animal is out and nothing frightens the herd"));
	return bOk;
}
