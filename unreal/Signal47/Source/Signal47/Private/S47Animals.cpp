#include "S47Animals.h"
#include "S47AnimalWarn.h"

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

TArray<FName> FS47Animals::BoneSpellings(const TCHAR* Bone)
{
	TArray<FName> Out;
	if (!Bone || !*Bone) return Out;
	const FString Given(Bone);
	Out.Add(FName(*Given));
	const FString Under = Given.Replace(TEXT("."), TEXT("_"));
	if (Under != Given) Out.Add(FName(*Under));
	const FString Dotted = Given.Replace(TEXT("_"), TEXT("."));
	if (Dotted != Given && Dotted != Under) Out.Add(FName(*Dotted));
	return Out;
}

bool FS47Animals::FindRigBones(const FReferenceSkeleton& Ref, FVector& OutHead, FVector OutPaws[4], FName& OutHeadName)
{
	auto BoneAt = [&Ref](const TCHAR* Bone, FVector& Out, FName* OutName)
	{
		for (const FName& Spelt : BoneSpellings(Bone))
		{
			const int32 Index = Ref.FindBoneIndex(Spelt);
			if (Index == INDEX_NONE) continue;
			Out = FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref, Index).GetLocation();
			if (OutName) *OutName = Spelt;
			return true;
		}
		return false;
	};
	static const TCHAR* const PawBones[2][4] = { { TEXT("FF.L"), TEXT("FF.R"), TEXT("FFB.L"), TEXT("FFB.R") },
		{ TEXT("FrontLowerLeg.L"), TEXT("FrontLowerLeg.R"), TEXT("BackLowerLeg.L"), TEXT("BackLowerLeg.R") } };
	bool bPaws = false;
	for (int32 Set = 0; Set < 2 && !bPaws; ++Set)
	{
		bPaws = true;
		for (int32 I = 0; I < 4 && bPaws; ++I) bPaws = BoneAt(PawBones[Set][I], OutPaws[I], nullptr);
	}
	return bPaws && BoneAt(TEXT("Head"), OutHead, &OutHeadName);
}

FVector FS47Animals::StareToward(const FVector& CoyoteAt, const FVector& Source, double SpreadDeg, float Roll)
{
	const FVector Way(Source.X - CoyoteAt.X, 0, Source.Z - CoyoteAt.Z);
	const double Dist = FMath::Max(5.0, Way.Size());
	const FVector Dir = Way.IsNearlyZero() ? FVector(1, 0, 0) : Way.GetSafeNormal();
	const double Off = (FMath::Clamp(double(Roll), 0.0, 1.0) * 2.0 - 1.0) * FMath::Abs(SpreadDeg);
	return CoyoteAt + S47AnimalsLocal::Turned(Dir, Off) * Dist;
}

double FS47Animals::CoyoteWait(int32 Seed, int32 Night, int32 Count)
{
	FRandomStream Draw(int32(uint32(Seed) * 2654435761u ^ uint32(Night) * 40503u ^ uint32(Count + 1) * 2246822519u));
	return Draw.FRandRange(600.f, 1500.f);
}

FVector FS47Animals::CoyoteStart(const FVector& Eye, const FVector& Forward, float RollSide, float RollAngle, float RollDist)
{
	FVector Ahead(Forward.X, 0, Forward.Z);
	Ahead = Ahead.IsNearlyZero() ? FVector(1, 0, 0) : Ahead.GetSafeNormal();
	const double Off = FMath::Lerp(50.0, 130.0, FMath::Clamp(double(RollAngle), 0.0, 1.0)) * (RollSide < 0.5f ? -1.0 : 1.0);
	return FVector(Eye.X, 0, Eye.Z) + S47AnimalsLocal::Turned(Ahead, Off) * FMath::Lerp(18.0, 30.0, FMath::Clamp(double(RollDist), 0.0, 1.0));
}

double FS47Animals::HerdFear(const FVector& HerdMiddle, FVector& OutFrom) const
{
	if (!M || !M->IsFreeform() || !M->Omen.IsValid()) return 0;
	FVector From = FVector::ZeroVector;
	const double Felt = M->Omen->SenseAt(HerdMiddle, S47AnimalsLocal::HerdWiden, &From, nullptr, ES47OmenChannel::Animals);
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
	// (PT-FARM-GROUND-20261109: the colours go on copies of the model's own glTF materials, which may be drawn on a skeletal mesh; the
	//  game's lit material may not, and in the packaged game the dog and the coyote were the engine's grey, Tom's "something big")
	BuildRig(CoyoteRig, TEXT("Coyote"), 70, Holder, { FLinearColor(.33f, .28f, .21f), .9f }, { FLinearColor(.62f, .55f, .44f), .9f }, { FLinearColor(.03f, .03f, .03f), .6f });
	BuildRig(DogRig, TEXT("Dog"), 62, Holder, { FLinearColor(.17f, .11f, .07f), .9f }, { FLinearColor(.66f, .58f, .47f), .9f }, { FLinearColor(.03f, .03f, .03f), .6f });
	return true;
}

bool FS47Animals::RigFramesAgree(double PawsUp, double HeadUp, double Sole, double Tall, FString& Why)
{
	// Along the skeleton's up, from the foot of the mesh's bounds, as a share of its height: a quadruped's paws are at the foot
	// (the Quaternius paw bones some 2 % up) and its head well up in it (some 80 %). A mesh in another frame from its bones has
	// its length along that up instead: the paws come out a third or more of the way up, or under it.
	if (!(Tall > 0) || !FMath::IsFinite(PawsUp) || !FMath::IsFinite(HeadUp) || !FMath::IsFinite(Sole)) { Why = TEXT("no height"); return false; }
	const double Paws = (PawsUp - Sole) / Tall, Head = (HeadUp - Sole) / Tall;
	Why = FString::Printf(TEXT("paws %.0f %% and head %.0f %% up the mesh"), Paws * 100, Head * 100);
	return Paws > -.05 && Paws < .2 && Head > .35 && Head < 1.05;
}

void FS47Animals::SetEyeShine(double Shine)
{
	const bool bOn = Shine > 0.02;
	const FLinearColor Glow = bOn ? FLinearColor(.75f, .95f, .4f) * float(90.0 * Shine) : FLinearColor::Black;
	if (UMaterialInstanceDynamic* Box = EyeGlow.Get())
	{
		Box->SetVectorParameterValue(TEXT("Emissive"), Glow);
		Box->SetScalarParameterValue(TEXT("EmissivePower"), bOn ? 1.f : 0.f);
	}
	// the glTF material's own emissive (MF_Emissive: factor times strength)
	for (FRig* Rig : { &CoyoteRig, &DogRig }) if (UMaterialInstanceDynamic* Own = Rig->Eye.Get())
	{
		Own->SetVectorParameterValue(TEXT("EmissiveFactor"), Glow);
		Own->SetScalarParameterValue(TEXT("EmissiveStrength"), bOn ? 1.f : 0.f);
	}
}

FString FS47Animals::RigReport() const
{
	FString Out;
	for (const FRig* Rig : { &CoyoteRig, &DogRig })
	{
		if (!Rig->bTried) continue;
		FString Mats;
		if (const USkeletalMeshComponent* Part = Rig->Mesh.Get())
		{
			int32 Good = 0;
			for (int32 I = 0; I < Part->GetNumMaterials(); ++I) if (const UMaterialInterface* Mat = Part->GetMaterial(I)) Good += Mat->GetUsageByFlag(MATUSAGE_SkeletalMesh);
			Mats = FString::Printf(TEXT(", %d of %d materials drawable on a skeletal mesh"), Good, Part->GetNumMaterials());
		}
		Out += FString::Printf(TEXT("%s%s %s (%s)%s"), Out.IsEmpty() ? TEXT("") : TEXT("; "), *Rig->Animal, Rig->bReady ? TEXT("rigged") : TEXT("not shown"), *Rig->Note, *Mats);
	}
	return Out.IsEmpty() ? FString(TEXT("no rig tried yet (Freeform, outdoors, once)")) : Out;
}

bool FS47Animals::BuildRig(FRig& Rig, const TCHAR* Animal, double HeightCm, USceneComponent* Holder, const FCoat& Coat, const FCoat& Pale, const FCoat& Nose)
{
	Rig.bTried = true; Rig.Animal = Animal; Rig.bReady = false; Rig.Note = TEXT("no model in this build");
	const FString Package = FString::Printf(TEXT("/Game/Free/Animals/%s/SK_%s"), Animal, Animal);
	if (!FPackageName::DoesPackageExist(Package) || !M || !M->Web || !Holder) return false;
	USkeletalMesh* Body = LoadObject<USkeletalMesh>(nullptr, *(Package + TEXT(".SK_") + Animal));
	if (!Body) return false;
	// How it stands, from its own skeleton, whatever axes and scale the import gave it (the glTF came in on its side and far too big):
	// its height is the bounds along its own up, and it is scaled to its real one.
	const FReferenceSkeleton& Ref = Body->GetRefSkeleton();
	// The four paws (the rig's foot targets, else the lower legs) say it: back to front is ahead, right to left is across, and up is
	// square to both, on the head's side. (The rig's "Body" is its root near the ground, so body to paws would lean.) The glTF names
	// them "FF.L"; Unreal's import made that "FF_L" (M2 critic: the old lookup found neither, and the boxes stayed).
	FVector HeadAt, Paws[4];
	FName HeadName;
	if (!FindRigBones(Ref, HeadAt, Paws, HeadName))
	{
		Rig.Note = TEXT("no head or paws in the skeleton");
		UE_LOG(LogS47Animals, Warning, TEXT("S47: animals %s rig has no head or paws (%d bones): the boxes stay"), Animal, Ref.GetNum());
		return false;
	}
	const FVector &FrontL = Paws[0], &FrontR = Paws[1], &BackL = Paws[2], &BackR = Paws[3];
	const FVector PawsAt = (FrontL + FrontR + BackL + BackR) * 0.25;
	FVector Ahead = ((FrontL + FrontR) - (BackL + BackR)).GetSafeNormal();
	const FVector Across = ((FrontL + BackL) - (FrontR + BackR)).GetSafeNormal();
	FVector Up = FVector::CrossProduct(Ahead, Across).GetSafeNormal();
	if (FVector::DotProduct(Up, HeadAt - PawsAt) < 0) Up = -Up;
	if (FVector::DotProduct(Ahead, HeadAt - PawsAt) < 0) Ahead = -Ahead;
	const FBoxSphereBounds Bounds = Body->GetBounds();
	const FVector Ext = Bounds.BoxExtent;
	const double Tall = 2.0 * (FMath::Abs(Up.X) * Ext.X + FMath::Abs(Up.Y) * Ext.Y + FMath::Abs(Up.Z) * Ext.Z);
	if (Up.IsNearlyZero() || Ahead.IsNearlyZero() || Tall < 0.01)
	{
		Rig.Note = FString::Printf(TEXT("unusable, height %.3f"), Tall);
		UE_LOG(LogS47Animals, Warning, TEXT("S47: animals %s rig unusable (height %.3f): the boxes stay"), Animal, Tall);
		return false;
	}
	Rig.Fix = FRotationMatrix::MakeFromXZ(Ahead, Up).ToQuat().Inverse();
	// the feet: under the paws, at the bottom of the bounds along up
	const double Sole = FVector::DotProduct(Bounds.Origin, Up) - 0.5 * Tall;
	// PT-FARM-GROUND-20261109: the bounds are the mesh's, the paws and the head the skeleton's. When they are not in one frame (the
	// 8 November import: up from the bones, but the mesh's length along that up, 38936 units "tall" for a dog), the skinned body
	// sweeps through the ground as it plays and stands nowhere near its feet. Then it is not shown: the dog is heard, the coyote
	// is the boxes, until it is imported again (Tools/animals/check_rig.py says which).
	FString Frames;
	if (!RigFramesAgree(FVector::DotProduct(PawsAt, Up), FVector::DotProduct(HeadAt, Up), Sole, Tall, Frames)
		&& !FParse::Param(FCommandLine::Get(), TEXT("S47RigsAsImported")))
	{
		Rig.Note = TEXT("mesh and skeleton in different frames: ") + Frames;
		UE_LOG(LogS47Animals, Warning, TEXT("S47: animals %s rig not shown: the mesh and its skeleton are in different frames (%s; %.0f units along up); %s"),
			Animal, *Frames, Tall, FCString::Strcmp(Animal, TEXT("Dog")) == 0 ? TEXT("the dog is only heard") : TEXT("the boxes stay"));
		return false;
	}
	Rig.Feet = PawsAt - Up * (FVector::DotProduct(PawsAt, Up) - Sole);
	Rig.Scale = HeightCm / Tall;
	Rig.YawOffset = 0; Rig.Lift = 0;
	Rig.Head = HeadName;
	USkeletalMeshComponent* Part = NewObject<USkeletalMeshComponent>(M->Web.Get(), *FString::Printf(TEXT("S47Animal_%s"), Animal));
	Part->SetupAttachment(Holder); Part->SetMobility(EComponentMobility::Movable);
	Part->SetSkinnedAssetAndUpdate(Body);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision); Part->SetCastShadow(true);
	Part->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Part->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	// its colours, by the source's own slots: the coat, the pale under it, a dark nose; the eyes the ones that shine. Each on a copy of
	// the slot's own material (the glTF import's MI_Default_Opaque, whose parent is made for skeletal meshes: BaseColorFactor,
	// RoughnessFactor, EmissiveFactor). A slot whose material cannot be drawn on a skeletal mesh keeps what it has.
	const TArray<FSkeletalMaterial>& Slots = Body->GetMaterials();
	int32 Coloured = 0;
	for (int32 I = 0; I < Slots.Num(); ++I)
	{
		const FString Slot = Slots[I].MaterialSlotName.ToString();
		UMaterialInterface* Own = Slots[I].MaterialInterface;
		if (!Own || !Own->GetUsageByFlag(MATUSAGE_SkeletalMesh)) continue;
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Own, M->Web.Get());
		if (!Mid) continue;
		const bool bEye = Slot.Contains(TEXT("Eye")) || Slot.EndsWith(TEXT("002"));
		const FCoat& C = bEye ? FCoat{ FLinearColor(.04f, .04f, .03f), .3f }
			: (Slot.Contains(TEXT("Light")) || Slot.EndsWith(TEXT("003")) || Slot.EndsWith(TEXT("001"))) ? Pale
			: (Slot.Contains(TEXT("Nose")) || Slot.EndsWith(TEXT("006"))) ? Nose : Coat;
		Mid->SetVectorParameterValue(TEXT("BaseColorFactor"), C.Colour);
		Mid->SetScalarParameterValue(TEXT("RoughnessFactor"), C.Rough);
		Mid->SetScalarParameterValue(TEXT("MetallicFactor"), 0.f);
		if (bEye) { Mid->SetVectorParameterValue(TEXT("EmissiveFactor"), FLinearColor::Black); Mid->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.f); Rig.Eye = Mid; }
		Part->SetMaterial(I, Mid);
		++Coloured;
	}
	Rig.Note = FString::Printf(TEXT("%d of %d slots coloured, %s"), Coloured, Slots.Num(), *Frames);
	Part->SetVisibility(false); Part->SetHiddenInGame(true);
	Part->RegisterComponent();
	Rig.Mesh = Part; Rig.bReady = true;
	UE_LOG(LogS47Animals, Display, TEXT("S47: animals %s rigged: %d bones, %.0f units tall as imported, scale %.5f, up (%.2f %.2f %.2f), ahead (%.2f %.2f %.2f), %d slots"),
		Animal, Ref.GetNum(), Tall, Rig.Scale, Up.X, Up.Y, Up.Z, Ahead.X, Ahead.Y, Ahead.Z, Slots.Num());
	return true;
}

void FS47Animals::PlayRig(FRig& Rig, const TCHAR* Clip, double Pace)
{
	USkeletalMeshComponent* Part = Rig.Mesh.Get();
	if (!Rig.bReady || !Part) return;
	if (Rig.Playing == Clip)
	{
		if (!FMath::IsNearlyEqual(Rig.Rate, Pace, 0.02)) { Rig.Rate = Pace; Part->SetPlayRate(float(Pace)); }
		return;
	}
	// asked once per change of clip: the same one again is the early return above
	Rig.Playing = Clip; Rig.Rate = Pace;
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
	Part->SetPlayRate(float(Pace));
}

void FS47Animals::PlaceRig(FRig& Rig, const FVector& FeetThree, double YawDeg)
{
	USkeletalMeshComponent* Part = Rig.Mesh.Get();
	if (!Rig.bReady || !Part) return;
	// stood up and turned to its heading, its feet where it stands
	const FQuat Turn = FQuat(FRotator(0, YawDeg, 0)) * Rig.Fix;
	Part->SetWorldLocationAndRotation(S47Three::ToUE(FeetThree) - Turn.RotateVector(Rig.Feet * Rig.Scale), Turn);
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
	Cy.Mode = ECoyote::None; Cy.Wait = -1;
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

void FS47Animals::Flush(const FVector& From, EFlush Why)
{
	FCovey& Cv = Covey;
	const bool bDanger = Why == EFlush::Danger;
	Cv.Mode = ECovey::Flying; Cv.Age = 0;
	++FlushCount; if (bDanger) ++DangerFlushes; if (Why == EFlush::Wind) ++OrdinaryFlushes;
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
	if (Why == EFlush::Player && NightNow() && M->Stress) M->Stress->Startle(0.3);
	// up with nobody near it: something to notice, and which way they went from
	if (Why != EFlush::Player && M->Omen && S47AnimalsLocal::Flat(Cv.At, EyeNow()) < 45)
		M->Omen->Felt(ES47OmenChannel::Animals, TEXT("quail went up with nobody near them"), bDanger, From, true);
	if (bLog)
	{
		UE_LOG(LogS47Animals, Display, TEXT("S47: animals covey up%s, %.0f m from the player, flying toward %.0f deg"),
			bDanger ? TEXT(" with nobody near (something real)") : Why == EFlush::Wind ? TEXT(" with nobody near (a gust)") : TEXT(""),
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
		const double Felt = M->Omen ? M->Omen->SenseAt(Cv.At, S47AnimalsLocal::CoveyWiden, &From, nullptr, ES47OmenChannel::Animals) : 0;
		if (Fear(Felt) >= 1) { Flush(From, EFlush::Danger); return; }
		// a hard gust through the brush puts them up too, flying with it
		double GustFrom = 0;
		const double Gust = M->Omen ? M->Omen->Ordinary(ES47Ordinary::Gust, &GustFrom) : 0;
		if (Gust > 0.55 && Dist < 45) { Flush(Cv.At + FVector(FMath::Sin(GustFrom), 0, -FMath::Cos(GustFrom)) * 30.0, EFlush::Wind); return; }
		if (Dist < FlushRange(M->Player->Sneak(), FeetSpeed > 3.2)) Flush(Stand, EFlush::Player);
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

bool FS47Animals::StartOut(double StareFor)
{
	// A coyote out on its own business: off to one side of where the player looks, 18 to 30 m out on foot (inside the lamp's reach),
	// trotting across or away; from the cab, crossing in the headlights further on. Never by a building.
	FCoyote& Cy = Coyote;
	const FVector Eye = EyeNow();
	FVector Forward = FS47Cam::Of(M->Player).Forward; Forward.Y = 0;
	Forward = Forward.IsNearlyZero() ? FVector(1, 0, 0) : Forward.GetSafeNormal();
	const bool bCab = M->World && M->World->bDriving;
	FVector Start;
	FVector Way;
	if (bCab)
	{
		Start = FVector(Eye.X, 0, Eye.Z) + S47AnimalsLocal::Turned(Forward, FMath::FRandRange(-18.f, 18.f)) * FMath::FRandRange(40.f, 60.f);
		Way = S47AnimalsLocal::Turned(Forward, FMath::RandBool() ? 90.0 : -90.0);
		Start -= Way * 9.0;
	}
	else
	{
		Start = CoyoteStart(Eye, Forward, FMath::FRand(), FMath::FRand(), FMath::FRand());
		// across the line to the player, or on away from them
		const FVector Outward = FleeDir(Eye, Start);
		Way = S47AnimalsLocal::Turned(Outward, FMath::RandBool() ? FMath::FRandRange(70.f, 110.f) : FMath::FRandRange(-110.f, -70.f));
		if (FMath::FRand() < 0.3f) Way = S47AnimalsLocal::Turned(Outward, FMath::FRandRange(-30.f, 30.f));
	}
	FVector2D RoadAt;
	if (M->Freeform && M->Freeform->EventPlaces && RoadOf(Start, RoadAt) && M->Freeform->EventPlaces->NearestTo(RoadAt, 25, ES47PlaceTrait::Indoors)) return false;
	Start.Y = GroundAt(FVector(Start.X, M->Player->FloorY, Start.Z));
	Cy.Mode = ECoyote::Trot; Cy.At = Start; Cy.Dir = Way; Cy.Age = 0; Cy.Left = bCab ? 18.0 / S47AnimalsLocal::TrotSpeed : FMath::FRandRange(25.f, 45.f);
	Cy.Gait = FMath::FRandRange(0.f, 6.f); Cy.Ground = Start.Y; Cy.GroundIn = 0.2; Cy.Yaw = S47AnimalsLocal::YawOf(Way); Cy.HeadYaw = 0; Cy.bLooked = false; Cy.Shine = 0;
	Cy.SniffIn = FMath::FRandRange(5.f, 12.f); Cy.Out = 0; Cy.Glance = 0; Cy.bAtNothing = false;
	++CoyotesOut;
	// N1110: a warning (Warn): it is already stopped, looking at the player, and stays so for StareFor seconds
	if (StareFor > 0)
	{
		BeginStare(Eye, false);
		Cy.StareAt = StareToward(Cy.At, Eye, 6.0, FMath::FRand());
		Cy.Left = StareFor; Cy.Glance = 0.3;
	}
	PoseCoyote(0);
	if (bLog)
	{
		UE_LOG(LogS47Animals, Display, TEXT("S47: animals a coyote is out, %.0f m from the player, %.0f deg off where they look"),
			S47AnimalsLocal::Flat(Start, Eye), FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Forward, FleeDir(Eye, Start)), -1.0, 1.0))));
	}
	return true;
}

void FS47Animals::Huff(const FVector& At)
{
	if (!M || !M->Audio || !M->Audio->Night || !M->Audio->Ctx) return;
	if (!HuffRec.IsValid())
	{
		// a short breathy chuff through the nose: noise, low and rounded, a quick rise and a quicker fall
		const float SampleHz = M->Audio->Ctx->SampleRate;
		const int32 Frames = FMath::Max(1, int32(SampleHz * 0.26f));
		TSharedRef<FS47AudioBuffer> Rec = M->Audio->Ctx->CreateBuffer(1, Frames, SampleHz);
		float* Px = Rec->GetChannelData(0);
		FRandomStream Hiss(4711);
		const double LowA = 1.0 - FMath::Exp(-2.0 * UE_DOUBLE_PI * 1100.0 / SampleHz), BassA = 1.0 - FMath::Exp(-2.0 * UE_DOUBLE_PI * 160.0 / SampleHz);
		double Low = 0, Bass = 0;
		for (int32 I = 0; I < Frames; ++I)
		{
			const double When = I / double(SampleHz);
			const double Env = When < 0.014 ? When / 0.014 : FMath::Exp(-(When - 0.014) / 0.045);
			Low += LowA * (Hiss.FRandRange(-1.f, 1.f) - Low);
			Bass += BassA * (Low - Bass);
			Px[I] = float((Low - Bass) * Env * 2.6);
		}
		HuffRec = Rec;
	}
	M->Audio->Night->PlayBufferAt(HuffRec, EyeNow(), At, 0.9, FMath::FRandRange(0.88f, 1.08f), 12);
}

void FS47Animals::BeginStare(const FVector& Toward, bool bAtNothing)
{
	// It stops where it is: a glance at the light first (the eyes show, if the lamp is on it), one huff, then it stares off toward
	// what it heard, give or take, and says nothing more.
	FCoyote& Cy = Coyote;
	Cy.Mode = ECoyote::Stare; Cy.StareAt = StareToward(Cy.At, Toward, StareSpread, FMath::FRand());
	Cy.Glance = FMath::FRandRange(0.4f, 0.8f); Cy.LookAt = EyeNow(); Cy.bAtNothing = bAtNothing;
	++StareCount; if (bAtNothing) ++OrdinaryStares;
	const FVector Head = Cy.At + FVector(0, 0.6, 0);
	Huff(Head);
	const double ToPlayer = S47AnimalsLocal::Flat(Cy.At, EyeNow());
	if (M->Omen && ToPlayer < 40) M->Omen->Felt(ES47OmenChannel::Animals, TEXT("a coyote stopped dead and stared"), !bAtNothing, Cy.StareAt, true);
	if (bLog)
	{
		UE_LOG(LogS47Animals, Display, TEXT("S47: animals coyote stares%s, %.0f m from the player, %.0f deg off the true way"), bAtNothing ? TEXT(" at nothing") : TEXT(""), ToPlayer,
			FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(FleeDir(Cy.At, Toward), FleeDir(Cy.At, Cy.StareAt)), -1.0, 1.0))));
	}
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
		const bool bStill = Cy.Mode == ECoyote::Stare || Cy.Mode == ECoyote::Look || Cy.Mode == ECoyote::Sniff;
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
		else if (Cy.Mode == ECoyote::Sniff) PlayRig(CoyoteRig, TEXT("Eating"), 0.8);
		else PlayRig(CoyoteRig, TEXT("Idle2"), 1.0);
		PlaceRig(CoyoteRig, Cy.At, Cy.Yaw);
		const double YawRad0 = FMath::DegreesToRadians(Cy.Yaw);
		const FVector Facing0(FMath::Cos(YawRad0), 0, FMath::Sin(YawRad0));
		const FVector EyeAt0 = S47Three::ToThree(CoyoteRig.Mesh->GetSocketLocation(CoyoteRig.Head)) + Facing0 * 0.12;
		const double Shine0 = ShineOf(EyeAt0, Facing0);
		if (FMath::Abs(Shine0 - Cy.Shine) > 0.01 || (Shine0 <= 0 && Cy.Shine > 0))
		{
			Cy.Shine = Shine0;
			SetEyeShine(Shine0);
		}
		return;
	}
	// the body turns to where it goes; the head to what it looks at, and the body after it past a quarter turn
	const bool bHeld = Cy.Mode == ECoyote::Stare || Cy.Mode == ECoyote::Look || Cy.Mode == ECoyote::Sniff;
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
		SetEyeShine(Shine);
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
	auto NextWait = [this]() { return CoyoteWait(M->Freeform ? M->Freeform->Seed() : 0, M->Freeform ? FS47FreeformSession::NightOf(M->Freeform->Clock()) : 0, CoyotesOut); };
	if (Cy.Mode == ECoyote::None)
	{
		// Out now and then, on its own (every ten to twenty-five minutes): never put in front of the player because something came.
		if (!bAbout || !NightNow() || bInTown) return;
		if (Cy.Wait < 0) Cy.Wait = FMath::Min(NextWait(), 300.0 + FMath::FRandRange(0.f, 600.f));
		if ((Cy.Wait -= Dt) > 0) return;
		Cy.Wait = NextWait();
		StartOut();
		return;
	}
	Cy.Age += Dt; Cy.Out += Dt;
	const double ToPlayer = Flat(Cy.At, Eye);
	auto Gone = [&Cy, &NextWait]() { Cy.Mode = ECoyote::None; Cy.Wait = NextWait(); };
	if (ToPlayer > CoyoteGone || (!bAbout && Cy.Mode != ECoyote::Bolt)) { Gone(); PoseCoyote(Dt); return; }
	FVector From = FVector::ZeroVector;
	const double Felt = M->Omen ? M->Omen->SenseAt(Cy.At, CoyoteWiden, &From, nullptr, ES47OmenChannel::Animals) : 0;
	double ShyWay = 0;
	const double Shy = M->Omen ? M->Omen->Ordinary(ES47Ordinary::Shy, &ShyWay) : 0;
	switch (Cy.Mode)
	{
	case ECoyote::Trot:
	case ECoyote::Look:
	case ECoyote::Sniff:
		if (Fear(Felt) >= 1)
		{
			// it stops dead where it is and looks off toward it
			BeginStare(From, false);
			Cy.Left = Fear(Felt) >= 2 ? FMath::FRandRange(2.5f, 4.5f) : FMath::FRandRange(4.f, 7.f);
			break;
		}
		if (Shy > 0 && Cy.Mode != ECoyote::Look && Cy.Out > 3)
		{
			// and now and then for nothing anyone can see
			BeginStare(Cy.At + FVector(FMath::Sin(ShyWay), 0, -FMath::Cos(ShyWay)) * 60.0, true);
			Cy.Left = FMath::FRandRange(3.f, 6.f);
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
			// now and then it stops to nose the ground, and goes on
			else if ((Cy.SniffIn -= Dt) <= 0 && !(M->World && M->World->bDriving))
			{
				Cy.SniffIn = FMath::FRandRange(8.f, 16.f);
				Cy.Mode = ECoyote::Sniff; Cy.LookAt = Cy.At + Cy.Dir * 3.0; Cy.Left = FMath::FRandRange(2.f, 4.5f);
			}
			if (Cy.Mode == ECoyote::Trot && (Cy.Left -= Dt) <= 0) Gone();
		}
		else if ((Cy.Left -= Dt) <= 0) { Cy.Mode = ECoyote::Trot; Cy.Left = FMath::FRandRange(6.f, 14.f); }
		break;
	case ECoyote::Stare:
		// the glance at the light, then off toward it, a little off the true way, held there
		if (Cy.Glance > 0) { Cy.Glance -= Dt; Cy.LookAt = Eye; }
		else Cy.LookAt = Cy.StareAt;
		if ((Cy.Left -= Dt) <= 0 || ToPlayer < CoyoteShy)
		{
			// then away from it, flat out
			Cy.Dir = FleeDir(ToPlayer < CoyoteShy ? Eye : Cy.StareAt, Cy.At); Cy.Mode = ECoyote::Bolt; Cy.Left = 6;
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
		const double Felt = M->Omen ? M->Omen->SenseAt(Home, DogWiden, &Fright, nullptr, ES47OmenChannel::Animals) : 0;
		if (Felt > 0) Dog.Feared = Fright;
		// and now and then a dog just quits in the middle of it, for nothing anyone else can hear
		double QuitWay = 0;
		if (Fear(Felt) < 1 && !Dog.bStopped && Dog.Left > 0 && !Dog.bFrantic && M->Omen && M->Omen->Ordinary(ES47Ordinary::Shy, &QuitWay) > 0)
		{
			if (Dog.Playing.IsValid()) Dog.Playing->Stop(Now0 + 0.04);
			Dog.Left = 0; Dog.bStopped = true; Dog.Quiet = FMath::FRandRange(20.f, 40.f);
			Dog.Feared = Home + FVector(FMath::Sin(QuitWay), 0, -FMath::Cos(QuitWay)) * 40.0;
			++StopCount;
			if (Away < DogWhine) M->Omen->Felt(ES47OmenChannel::Animals, TEXT("the dog quit barking"), false, Home, true);
			continue;
		}
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
				if (Away < DogWhine && M->Omen) M->Omen->Felt(ES47OmenChannel::Animals, TEXT("the dog quit barking"), true, Home, true);
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
	// without a threat: nothing made up still out there (a run before this one may have left one), and the covey right at the feet,
	// inside the distance it lets a standing walker come, so that it goes up from the player and from nothing else
	if (!bThreat && M->Omen) M->Omen->DevClear();
	FVector Spot = Stand + Ahead * (bThreat ? 14.0 : 0.6 * FlushRange(M->Player->Sneak(), false));
	Spot.Y = GroundAt(Spot);
	Covey.At = Spot; Covey.Mode = ECovey::Sitting; Covey.Age = 0;
	DevCoveyIn = bThreat ? 1.5 : -1;
	Why = bThreat ? TEXT("a covey sits 14 m ahead; in a moment something comes up behind you, 55 m off, and they go up away from it")
		: TEXT("a covey sits in the brush at your feet, nothing else near: it goes up from you (the count of quail up from something stays as it was)");
	return true;
}

bool FS47Animals::DevCoyote(FString& Why, bool bThreat)
{
	if (!M || !M->IsFreeform() || !M->Player || !Build()) { Why = TEXT("Freeform only"); return false; }
	if (!(Outdoors() || (M->World && M->World->bDriving))) { Why = TEXT("outdoors only"); return false; }
	if (!StartCrossing(true, 32)) { Why = TEXT("too near a building here"); return false; }
	DevCoyoteIn = bThreat ? 2.5 : -1;
	if (!bThreat && M->Omen) M->Omen->DevClear();
	Why = bThreat ? TEXT("a coyote crosses 32 m ahead (the lamp finds its eyes); in a moment something comes 70 m to one side of it: it stops, huffs and stares off that way (give or take), then runs")
		: TEXT("a coyote crosses 32 m ahead with nothing near it; put the lamp on it and it may look round");
	return true;
}

bool FS47Animals::Warn(bool bBirds, const FVector& Threat, double Lead, FString& Why)
{
	if (!M || !M->IsFreeform() || !M->Player || !Build()) { Why = TEXT("Freeform only"); return false; }
	if (!Outdoors()) { Why = TEXT("not out on foot"); return false; }
	const FVector Stand(M->Player->Pos.X, M->Player->FloorY, M->Player->Pos.Z);
	if (bBirds)
	{
		if (Covey.Mode == ECovey::Flying) { Why = TEXT("a covey is up already"); return false; }
		// one sitting near is the one; else one is put near, 16 to 30 m off, where there is cover (not by a building)
		if (Covey.Mode != ECovey::Sitting || S47AnimalsLocal::Flat(Stand, Covey.At) > 40)
		{
			Covey.Mode = ECovey::None;
			PlaceCovey(Stand, FS47Cam::Of(M->Player).Forward);
		}
		if (Covey.Mode != ECovey::Sitting) { Why = TEXT("no cover for a covey here"); return false; }
		Flush(Threat, EFlush::Danger);
		return true;
	}
	if (!NightNow()) { Why = TEXT("coyotes are out at night"); return false; }
	if (Coyote.Mode != ECoyote::None) { Why = TEXT("one is out already"); return false; }
	FVector2D RoadAt;
	if (M->Freeform && M->Freeform->EventPlaces && RoadOf(EyeNow(), RoadAt) && M->Freeform->EventPlaces->NearestTo(RoadAt, 90, ES47PlaceTrait::Town)) { Why = TEXT("not in town"); return false; }
	if (!StartOut(Lead)) { Why = TEXT("too near a building here"); return false; }
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
	{
		FString Why;
		Check(!Bare.Warn(true, FVector::ZeroVector, 7, Why) && !Bare.Warn(false, FVector::ZeroVector, 7, Why) && !Bare.CoyoteOut() && Bare.Flushes() == 0, TEXT("without a game the warning puts no animal out"));
	}
	Check(Bare.HerdFear(FVector::ZeroVector, From) == 0 && !Bare.CoyoteOut() && !Bare.CoveyHere() && Bare.Flushes() == 0, TEXT("without a game no animal is out and nothing frightens the herd"));
	// M2 critic: the rig's bones are "FF.L" in the glTF and "FF_L" as Unreal imported them; both are looked for
	{
		const TArray<FName> Spelt = BoneSpellings(TEXT("FF.L"));
		const TArray<FName> Back = BoneSpellings(TEXT("BackLowerLeg_R"));
		Check(Spelt.Num() == 2 && Spelt[0] == FName(TEXT("FF.L")) && Spelt[1] == FName(TEXT("FF_L")) && Back.Contains(FName(TEXT("BackLowerLeg.R"))) && BoneSpellings(TEXT("Head")).Num() == 1,
			TEXT("a rig's bone is looked for as the glTF spells it and as the import does"));
	}
	for (const TCHAR* Animal : { TEXT("Coyote"), TEXT("Dog") })
	{
		const FString Package = FString::Printf(TEXT("/Game/Free/Animals/%s/SK_%s"), Animal, Animal);
		if (!FPackageName::DoesPackageExist(Package)) { Lines.Add(FString::Printf(TEXT("ok animals: no %s model in this build (the boxes, or only its sound)"), Animal)); continue; }
		const USkeletalMesh* Body = LoadObject<USkeletalMesh>(nullptr, *(Package + TEXT(".SK_") + Animal));
		FVector HeadAt = FVector::ZeroVector, Paws[4]; FName HeadName;
		const bool bFound = Body && FindRigBones(Body->GetRefSkeleton(), HeadAt, Paws, HeadName);
		Check(bFound && !HeadName.IsNone() && !(Paws[0] - Paws[2]).IsNearlyZero(), FCString::Strcmp(Animal, TEXT("Coyote")) == 0
			? TEXT("the coyote's model is there, and its head and four paws are found: it is the rigged coyote, not the boxes")
			: TEXT("the dog's model is there, and its head and four paws are found: the dog is seen, not only heard"));
		// PT-FARM-GROUND-20261109: its own materials can be coloured on a skeletal mesh (the game's lit one cannot: the engine's grey)
		if (Body)
		{
			int32 Drawable = 0;
			for (const FSkeletalMaterial& Slot : Body->GetMaterials()) Drawable += Slot.MaterialInterface && Slot.MaterialInterface->GetUsageByFlag(MATUSAGE_SkeletalMesh);
			Check(Drawable > 0, FCString::Strcmp(Animal, TEXT("Coyote")) == 0 ? TEXT("the coyote's own materials may be drawn on a skeletal mesh: it is coloured, not the engine's grey")
				: TEXT("the dog's own materials may be drawn on a skeletal mesh: it is coloured, not the engine's grey"));
		}
		// whether its mesh and skeleton agree is said here, not failed: a bad import is then not shown (BuildRig), which is safe
		if (bFound)
		{
			FVector Up = FVector::CrossProduct(((Paws[0] + Paws[1]) - (Paws[2] + Paws[3])).GetSafeNormal(), ((Paws[0] + Paws[2]) - (Paws[1] + Paws[3])).GetSafeNormal()).GetSafeNormal();
			const FVector PawsAt = (Paws[0] + Paws[1] + Paws[2] + Paws[3]) * .25;
			if (FVector::DotProduct(Up, HeadAt - PawsAt) < 0) Up = -Up;
			const FBoxSphereBounds B = Body->GetBounds();
			const double Tall = 2.0 * (FMath::Abs(Up.X) * B.BoxExtent.X + FMath::Abs(Up.Y) * B.BoxExtent.Y + FMath::Abs(Up.Z) * B.BoxExtent.Z);
			FString Why;
			const bool bAgree = RigFramesAgree(FVector::DotProduct(PawsAt, Up), FVector::DotProduct(HeadAt, Up), FVector::DotProduct(B.Origin, Up) - .5 * Tall, Tall, Why);
			Lines.Add(FString::Printf(TEXT("ok animals: %s rig %s (%s)"), Animal, bAgree ? TEXT("mesh and skeleton in one frame") : TEXT("NOTE mesh and skeleton in different frames, not shown until imported again"), *Why));
		}
	}
	{
		FString Why;
		Check(RigFramesAgree(7, 255, 0, 319, Why) && !RigFramesAgree(7, 263, -14740, 38936, Why) && !RigFramesAgree(0, 0, 0, 0, Why),
			TEXT("a rig whose paws stand at the foot of its mesh is shown; one whose mesh lies in another frame from its bones is not"));
	}
	{
		bool bSpread = true, bLeft = false, bRight = false;
		for (int32 I = 0; I <= 20; ++I)
		{
			const FVector Looked = StareToward(FVector::ZeroVector, FVector(100, 0, 0), StareSpread, I / 20.f);
			const double Off = FMath::RadiansToDegrees(FMath::Atan2(Looked.Z, Looked.X));
			bSpread &= FMath::Abs(Off) <= StareSpread + 1e-6 && FMath::IsNearlyEqual(Looked.Size(), 100.0, 1e-6);
			bLeft |= Off < -10; bRight |= Off > 10;
		}
		Check(bSpread && bLeft && bRight, TEXT("a staring coyote looks off toward it, up to 25 degrees either way: a hint at the way, not a bearing"));
	}
	{
		bool bRange = true, bSame = true; TSet<int32> Minutes;
		for (int32 N = 0; N < 40; ++N)
		{
			const double Wait = CoyoteWait(470047, 3, N);
			bRange &= Wait >= 600 && Wait <= 1500; bSame &= Wait == CoyoteWait(470047, 3, N); Minutes.Add(int32(Wait / 60));
		}
		Check(bRange && bSame && Minutes.Num() > 5, TEXT("a coyote is out every ten to twenty-five minutes, drawn from the seed, not every couple of minutes"));
		bool bAside = true;
		for (int32 I = 0; I < 30; ++I)
		{
			const FVector Eye0(5, 1.6, 7), Look(0, 0, -1);
			const FVector At = CoyoteStart(Eye0, Look, (I % 2) ? 0.9f : 0.1f, (I % 7) / 6.f, (I % 5) / 4.f);
			const FVector Way = FleeDir(Eye0, At);
			const double Off = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Way, Look), -1.0, 1.0)));
			const double Dist = S47AnimalsLocal::Flat(Eye0, At);
			bAside &= Off >= 49.9 && Off <= 130.1 && Dist >= 17.9 && Dist <= 30.1;
		}
		Check(bAside, TEXT("on foot a coyote is first out off to one side, within the lamp's reach, never paraded across the middle of the view"));
	}
	bOk &= FS47AnimalWarn::SelfTest(Lines);
	return bOk;
}
