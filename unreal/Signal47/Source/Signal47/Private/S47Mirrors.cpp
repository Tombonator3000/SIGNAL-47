#include "S47Mirrors.h"

#include "Components/SceneCaptureComponent2D.h"
#include "CoreGlobals.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "S47Three.h"
#include "S47Truck.h"
#include "S47Web.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Mirrors, Log, All);

using namespace S47Three;

namespace
{
	// ======================================================================================
	// The mirrors, for root to adjust. Truck-local three.js metres: x right (the driver sits
	// at -x), y up, -z forward (+z is behind the car). The Jeep's parts hang at zero under the
	// truck's Body, so a native UEcm point (X, Y, Z) in the part's frame is three (X, Z, Y) / 100
	// (S47Three::ToThree; the driver's eye UEcm (-41, 65, 170) is three (-.41, 1.70, .65)).
	//
	// Centre, Normal, Up  the model's glass (jeep_model, candidate-r16a anchors.json mirrors.glass,
	//          converted): where the capture stands and the plane it reflects in. The normal faces
	//          the driver and the rear. Used whether the glass is the model's or the fallback's.
	// FallbackCentre, FallbackNormal  our own thin plane when the Jeep has no MI_JeepMirrorGlass_*
	//          slot (an older import): over interior.py's straight door-mirror faces (Blender
	//          (+-.89, .120, 1.665)); zero normal: the model's normal (the inside mirror, which
	//          then also gets a black back and a stalk to the windshield rail).
	// Size     the fallback glass, width and height (the model's glass is 14.6 x 20.5 / 16.1 x 4.35 cm).
	// Pixels   the render target (its aspect matches the glass: the door mirrors stand upright).
	// Fov      horizontal degrees of the capture (a flat mirror's true field is narrower; this
	//          is a little wider, as a driver gets by moving the head).
	// bHideCar the Jeep's own meshes are not drawn in this one (the inside mirror would only show
	//          the cab and the seats; the door mirrors keep the car's flank, as a real one does).
	// ======================================================================================
	struct FSpec
	{
		const TCHAR* Key;  // MI_JeepMirrorGlass_<Key>
		FVector Centre, Normal, Up;
		FVector FallbackCentre, FallbackNormal;
		FVector2D Size;
		FIntPoint Pixels;
		float Fov;
		bool bHideCar;
	};
	const FSpec SPECS[FS47Mirrors::Count] = {
		{ TEXT("L"), FVector(-.876064, 1.665, -.121253), FVector(.2749, 0, .9615), FVector(0, 1, 0),
		  FVector(-.89, 1.665, -.117), FVector(0, 0, 1), FVector2D(.140, .198), FIntPoint(160, 224), 30.f, false },
		{ TEXT("R"), FVector(.864905, 1.665, -.125946), FVector(-.495, 0, .8689), FVector(0, 1, 0),
		  FVector(.89, 1.665, -.117), FVector(0, 0, 1), FVector2D(.140, .198), FIntPoint(160, 224), 30.f, false },
		{ TEXT("Rear"), FVector(-.0004, 1.91479, .176112), FVector(-.3337, -.175, .9263), FVector(-.0593, .9846, .1646),
		  FVector(0, 1.915, .150), FVector::ZeroVector, FVector2D(.161, .0435), FIntPoint(384, 104), 40.f, true },
	};
	/** The inside mirror's stalk goes up to here (the windshield's upper rail, truck-local). */
	const FVector REAR_STALK_TOP(0, 1.995, .095);

	constexpr double MIRROR_HZ = 20;        // each mirror captured at most this often
	constexpr double VIEW_COS = .2;         // only mirrors within ~78 degrees of the look are captured
	constexpr double CAMERA_OUT = .02;      // the capture stands this far in front of the glass (m)
	constexpr float NEAR_CLIP_CM = 4.f;
	constexpr float LOD_FACTOR = 2.f;       // coarser LODs in the mirrors
	constexpr bool SHADOWS = false;         // dynamic shadows in the mirrors (off: cheaper)
	constexpr float LIVE_GAIN = .82f;       // the glass's reflectance on the live picture
	constexpr bool FLIP_U = true, FLIP_V = false; // the picture on the glass: U = 1 - U (a mirror)
	const FLinearColor OFF_COLOR(.10f, .11f, .12f); // the fallback glass with the mirrors off
	constexpr float OFF_ROUGHNESS = .06f, OFF_METALNESS = 1.f;
	const FLinearColor BACKING_COLOR(.018f, .018f, .02f);

	bool GEnabled = true;

	/** A plane facing N, in three's Euler XYZ: its up stays upright (no roll). */
	FQuat Facing(const FVector& N)
	{
		return Euler(FMath::Atan2(-N.Y, N.Z), FMath::Asin(FMath::Clamp(N.X, -1.0, 1.0)), 0);
	}
	/** Which mirror a slot is for: "...JeepMirrorGlass_L" -> 0, "_R" -> 1, "_Rear" -> 2, else -1. */
	int32 KeyOf(const FString& Name)
	{
		static const FString Tag = TEXT("JeepMirrorGlass_");
		const int32 At = Name.Find(Tag, ESearchCase::IgnoreCase);
		if (At == INDEX_NONE) return -1;
		FString Key;
		for (int32 I = At + Tag.Len(); I < Name.Len() && FChar::IsAlpha(Name[I]); ++I) Key.AppendChar(Name[I]);
		for (int32 I = 0; I < FS47Mirrors::Count; ++I) if (Key.Equals(SPECS[I].Key, ESearchCase::IgnoreCase)) return I;
		return -1;
	}
	int32 SlotKey(const UStaticMeshComponent* C, int32 Slot)
	{
		if (const UStaticMesh* Mesh = C->GetStaticMesh())
		{
			const TArray<FStaticMaterial>& Mats = Mesh->GetStaticMaterials();
			if (Mats.IsValidIndex(Slot))
			{
				const int32 K = KeyOf(Mats[Slot].MaterialSlotName.ToString());
				if (K >= 0) return K;
				const int32 Imported = KeyOf(Mats[Slot].ImportedMaterialSlotName.ToString());
				if (Imported >= 0) return Imported;
				if (Mats[Slot].MaterialInterface) { const int32 Asset = KeyOf(Mats[Slot].MaterialInterface->GetName()); if (Asset >= 0) return Asset; }
			}
		}
		const UMaterialInterface* Now = C->GetMaterial(Slot);
		return Now ? KeyOf(Now->GetName()) : -1;
	}
	/** A part of our own under one of the truck's nodes (no collision, no shadow), adopted by the web
	 *  world so it is shown and hidden with the truck. */
	UStaticMeshComponent* MakePart(AS47Web* W, int32 Parent, UStaticMesh* Mesh, UMaterialInterface* Mat, const FVector& P, const FQuat& Q, const FVector& S, const FString& Name)
	{
		const FS47WebNode* N = W ? W->Node(Parent) : nullptr;
		if (!N || !N->Comp.IsValid() || !Mesh) return nullptr;
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W);
		C->SetStaticMesh(Mesh);
		C->SetupAttachment(N->Comp.Get());
		C->SetMobility(EComponentMobility::Movable);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->bAffectDistanceFieldLighting = false;
		if (Mat) C->SetMaterial(0, Mat);
		C->RegisterComponent();
		double E[16];
		Compose(P, Q, S, E);
		C->SetRelativeTransform(MatToUE(E));
		if (W->AdoptNode(C, Parent, Name) < 0) { C->DestroyComponent(); return nullptr; }
		return C;
	}
}

// ---------- the switch ----------

void S47Mirrors::SetEnabled(bool bOn) { GEnabled = bOn; }
bool S47Mirrors::Enabled() { return GEnabled; }

// ---------- install ----------

TUniquePtr<FS47Mirrors> FS47Mirrors::Install(AS47Web* W, FS47Truck& T, const TArray<TWeakObjectPtr<UStaticMeshComponent>>& Parts)
{
	if (!W || !T.Body || !T.TripoJeep() || FParse::Param(FCommandLine::Get(), TEXT("S47NoMirrors"))) return {};
	UMaterialInterface* Unlit = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Unlit_Opaque.M_Web_Unlit_Opaque"));
	UMaterialInterface* Lit = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
	if (!Unlit || !Lit) { UE_LOG(LogS47Mirrors, Warning, TEXT("S47: mirrors unavailable (web masters missing); %s keeps its plain mirror faces"), *T.Path); return {}; }
	TUniquePtr<FS47Mirrors> R(new FS47Mirrors());
	R->Web = W;
	for (const TWeakObjectPtr<UStaticMeshComponent>& P : Parts) if (UStaticMeshComponent* C = P.Get()) R->CarParts.Add(C);
	for (int32 I = 0; I < Count; ++I) R->Aim[I] = SPECS[I].Normal.GetSafeNormal();

	// the model's own glass, if the Jeep has it (agent jeep_model: MI_JeepMirrorGlass_L, _R, _Rear)
	for (const TWeakObjectPtr<UStaticMeshComponent>& P : Parts)
	{
		UStaticMeshComponent* C = P.Get();
		if (!C) continue;
		for (int32 Slot = 0; Slot < C->GetNumMaterials(); ++Slot)
		{
			const int32 K = SlotKey(C, Slot);
			if (K < 0 || R->Glass[K].Mesh.IsValid()) continue;
			FGlass& G = R->Glass[K];
			G.Mesh = C; G.Slot = Slot; G.bModel = true; G.Off.Reset(C->GetMaterial(Slot));
		}
	}

	// otherwise a thin plane of our own over the mirror (and for the inside mirror a backing and a stalk)
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInstanceDynamic* Dark = UMaterialInstanceDynamic::Create(Lit, W);
	Dark->SetVectorParameterValue(TEXT("Color"), OFF_COLOR);
	Dark->SetScalarParameterValue(TEXT("Roughness"), OFF_ROUGHNESS);
	Dark->SetScalarParameterValue(TEXT("Metalness"), OFF_METALNESS);
	Dark->SetScalarParameterValue(TEXT("NativeLighting"), 1.f);
	UMaterialInstanceDynamic* Backing = UMaterialInstanceDynamic::Create(Lit, W);
	Backing->SetVectorParameterValue(TEXT("Color"), BACKING_COLOR);
	Backing->SetScalarParameterValue(TEXT("Roughness"), .55f);
	Backing->SetScalarParameterValue(TEXT("Metalness"), 0.f);
	Backing->SetScalarParameterValue(TEXT("NativeLighting"), 1.f);
	for (int32 I = 0; I < Count; ++I)
	{
		FGlass& G = R->Glass[I];
		if (G.bModel) continue;
		const FSpec& S = SPECS[I];
		const FVector N = S.FallbackNormal.IsNearlyZero() ? R->Aim[I] : S.FallbackNormal.GetSafeNormal();
		const FQuat Q = Facing(N);
		// (the engine's plane lies flat: stood up by a quarter turn about x, as FS47Group::Plane does)
		UStaticMeshComponent* C = MakePart(W, T.Body.I, Plane, Dark, S.FallbackCentre, Q * Euler(UE_DOUBLE_HALF_PI, 0, 0), FVector(S.Size.X, 1, S.Size.Y),
			FString::Printf(TEXT("mirror:%s:%s"), *T.Path, S.Key));
		if (!C) continue;
		G.Mesh = C; G.Slot = 0; G.Off.Reset(Dark);
		R->Made.Add(C);
		if (S.bHideCar && Cube)
		{
			// a black plastic back and a short stalk up to the rail: the inside mirror has no model
			const double Depth = .012, Rim = .014;
			const FVector Back = S.FallbackCentre - N * (Depth / 2 + .0005);
			if (UStaticMeshComponent* B = MakePart(W, T.Body.I, Cube, Backing, Back, Q, FVector(S.Size.X + Rim, S.Size.Y + Rim, Depth), FString::Printf(TEXT("mirror:%s:%s:back"), *T.Path, S.Key)))
				R->Made.Add(B);
			const FVector From = Back + Q.RotateVector(FVector(0, S.Size.Y / 2 + Rim / 2, 0)), Dir = REAR_STALK_TOP - From;
			const double Len = Dir.Size();
			if (Len > .005)
				if (UStaticMeshComponent* B = MakePart(W, T.Body.I, Cube, Backing, (From + REAR_STALK_TOP) / 2, FQuat::FindBetweenNormals(FVector(0, 1, 0), Dir / Len), FVector(.012, Len, .012),
					FString::Printf(TEXT("mirror:%s:%s:stalk"), *T.Path, S.Key)))
					R->Made.Add(B);
		}
	}
	for (const TWeakObjectPtr<UStaticMeshComponent>& C : R->Made) R->Hidden.Add(C.Get());

	// the live picture: the web world's unlit master with the capture as its (linear) map, mirrored
	for (int32 I = 0; I < Count; ++I)
	{
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Unlit, W);
		D->SetVectorParameterValue(TEXT("Color"), FLinearColor(LIVE_GAIN, LIVE_GAIN, LIVE_GAIN, 1.f));
		D->SetScalarParameterValue(TEXT("UseMap"), 1.f);
		D->SetScalarParameterValue(TEXT("MapIsLinear"), 1.f);
		D->SetVectorParameterValue(TEXT("MapU"), FLIP_U ? FLinearColor(-1, 0, 1, 0) : FLinearColor(1, 0, 0, 0));
		D->SetVectorParameterValue(TEXT("MapV"), FLIP_V ? FLinearColor(0, -1, 1, 0) : FLinearColor(0, 1, 0, 0));
		R->Glass[I].Live.Reset(D);
	}
	R->MakeCaptures(W, true);
	for (int32 I = 0; I < Count; ++I) R->ShowGlass(I);

	// not fed by the world for a couple of frames (on foot, a panel open, another area): idle and dark
	const TWeakObjectPtr<AS47Web> WeakWeb = W;
	FS47Mirrors* Self = R.Get();
	R->Tick = FWorldDelegates::OnWorldPostActorTick.AddLambda([Self, WeakWeb](UWorld* World, ELevelTick, float)
	{
		if (!Self->bLive || !WeakWeb.IsValid() || WeakWeb->GetWorld() != World) return;
		if (GFrameCounter > Self->FedFrame + 2) Self->SetLive(false);
	});
	UE_LOG(LogS47Mirrors, Display, TEXT("S47: Jeep mirrors on %s: left %s, right %s, rear %s; live while driving with Vehicle mirrors on"), *T.Path,
		R->Glass[0].bModel ? TEXT("model glass") : TEXT("fallback plane"), R->Glass[1].bModel ? TEXT("model glass") : TEXT("fallback plane"), R->Glass[2].bModel ? TEXT("model glass") : TEXT("fallback plane"));
	return R;
}

FS47Mirrors::~FS47Mirrors()
{
	if (Tick.IsValid()) FWorldDelegates::OnWorldPostActorTick.Remove(Tick);
	for (FGlass& G : Glass)
	{
		if (USceneCaptureComponent2D* C = G.Capture.Get()) if (C->IsRegistered()) C->DestroyComponent();
		G.Capture.Reset(); G.Target.Reset(); G.Live.Reset(); G.Off.Reset();
	}
}

void FS47Mirrors::MakeCaptures(UObject* Outer, bool bRegister)
{
	AS47Web* W = Web.Get();
	for (int32 I = 0; I < Count; ++I)
	{
		const FSpec& S = SPECS[I];
		USceneCaptureComponent2D* C = NewObject<USceneCaptureComponent2D>(Outer);
		C->SetAutoActivate(false);
		C->bCaptureEveryFrame = false;
		C->bCaptureOnMovement = false;
		C->bAlwaysPersistRenderingState = false; // no view state: no history, pre-exposure 1 (linear radiance)
		C->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDRNoAlpha; // before post: the main view tone-maps it once
		C->FOVAngle = S.Fov;
		C->bOverride_CustomNearClippingPlane = true;
		C->CustomNearClippingPlane = NEAR_CLIP_CM;
		C->LODDistanceFactor = LOD_FACTOR;
		C->bUseRayTracingIfEnabled = false;
		// the expensive parts off: Lumen, reflections, volumetric fog, particles, AO, post effects
		C->PostProcessBlendWeight = 1.f;
		C->PostProcessSettings.bOverride_DynamicGlobalIlluminationMethod = true;
		C->PostProcessSettings.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::None;
		C->PostProcessSettings.bOverride_ReflectionMethod = true;
		C->PostProcessSettings.ReflectionMethod = EReflectionMethod::None;
		FEngineShowFlags& F = C->ShowFlags;
		F.SetLumenGlobalIllumination(false); F.SetLumenReflections(false); F.SetScreenSpaceReflections(false);
		F.SetVolumetricFog(false); F.SetCloud(false); F.SetParticles(false);
		F.SetAmbientOcclusion(false); F.SetDistanceFieldAO(false); F.SetContactShadows(false); F.SetDynamicShadows(SHADOWS);
		F.SetMotionBlur(false); F.SetBloom(false); F.SetEyeAdaptation(false); F.SetTemporalAA(false);
		for (const TWeakObjectPtr<UPrimitiveComponent>& H : Hidden) if (H.IsValid()) C->HideComponent(H.Get());
		if (S.bHideCar) for (const TWeakObjectPtr<UPrimitiveComponent>& H : CarParts) if (H.IsValid()) C->HideComponent(H.Get());
		if (bRegister && W)
		{
			C->SetupAttachment(W->GetRootComponent());
			C->RegisterComponent();
		}
		C->SetActive(false);
		Glass[I].Capture.Reset(C);
	}
}

void FS47Mirrors::EnsureTarget(int32 I)
{
	FGlass& G = Glass[I];
	if (G.Target) return;
	UObject* Outer = Web.IsValid() ? static_cast<UObject*>(Web.Get()) : GetTransientPackage();
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(Outer);
	RT->ClearColor = FLinearColor::Black;
	RT->InitCustomFormat(SPECS[I].Pixels.X, SPECS[I].Pixels.Y, PF_FloatRGBA, true);
	RT->UpdateResourceImmediate(true);
	G.Target.Reset(RT);
	if (G.Capture) G.Capture->TextureTarget = RT;
	if (G.Live) G.Live->SetTextureParameterValue(TEXT("MapLinear"), RT);
}

void FS47Mirrors::SetLive(bool bOn)
{
	if (bLive == bOn) return;
	bLive = bOn;
	for (int32 I = 0; I < Count; ++I)
	{
		FGlass& G = Glass[I];
		if (bOn) EnsureTarget(I);
		G.bImage = false; G.Last = -1;
		if (G.Capture) G.Capture->SetActive(bOn);
		ShowGlass(I);
	}
}

void FS47Mirrors::ShowGlass(int32 I)
{
	FGlass& G = Glass[I];
	UStaticMeshComponent* C = G.Mesh.Get();
	// (the dark glass until the first picture is in: never a black frame on going live)
	UMaterialInterface* Want = bLive && G.bImage && G.Live ? static_cast<UMaterialInterface*>(G.Live.Get()) : G.Off.Get();
	if (C && Want && C->GetMaterial(G.Slot) != Want) C->SetMaterial(G.Slot, Want);
}

bool FS47Mirrors::CaptureActive() const
{
	for (const FGlass& G : Glass) if (G.Capture && G.Capture->IsActive()) return true;
	return false;
}

UTextureRenderTarget2D* FS47Mirrors::Target(int32 I) const { return I >= 0 && I < Count ? Glass[I].Target.Get() : nullptr; }
bool FS47Mirrors::UsesModelGlass(int32 I) const { return I >= 0 && I < Count && Glass[I].bModel; }

// ---------- once a frame ----------

void FS47Mirrors::Step(const FS47Truck& T, bool bAtWheel, const FVector& EyeThree, const FQuat& LookThree)
{
	const bool bWant = S47Mirrors::Enabled() && bAtWheel && T.bDriving && !T.IsPersonalVehicle() && T.Group.Shown();
	StepAt(bWant, T.Group.WorldQuaternion(), T.Group.WorldPosition(), EyeThree, LookThree, FPlatformTime::Seconds());
}

void FS47Mirrors::StepAt(bool bWant, const FQuat& CarQ, const FVector& CarP, const FVector& Eye, const FQuat& Look, double Now)
{
	FedFrame = GFrameCounter;
	SetLive(bWant);
	if (!bLive) return;
	// One mirror this frame: the one longest without a picture, if it is due and in the view.
	const FVector Fwd = Look.RotateVector(FVector(0, 0, -1));
	int32 Best = -1;
	double BestAge = -1;
	FVector BestAt = FVector::ZeroVector, BestIn = FVector::ZeroVector, BestN = FVector::ZeroVector;
	for (int32 I = 0; I < Count; ++I)
	{
		const FVector At = CarP + CarQ.RotateVector(SPECS[I].Centre);
		FVector In = At - Eye;
		const double Dist = In.Size();
		if (Dist < .05) continue;
		In /= Dist;
		const FVector N = CarQ.RotateVector(Aim[I]);
		if (FVector::DotProduct(In, N) > -.05 || FVector::DotProduct(In, Fwd) < VIEW_COS) continue; // behind the glass, or not looked at
		const double Age = Glass[I].Last < 0 ? 1e9 : Now - Glass[I].Last;
		if (Age < 1.0 / MIRROR_HZ - 1e-4 || Age <= BestAge) continue;
		Best = I; BestAge = Age; BestAt = At; BestIn = In; BestN = N;
	}
	if (Best < 0) return;
	FGlass& G = Glass[Best];
	// the eye's ray off the glass; the picture upright with the car
	const FVector Out = (BestIn - 2 * FVector::DotProduct(BestIn, BestN) * BestN).GetSafeNormal();
	FVector Up = CarQ.RotateVector(SPECS[Best].Up);
	Up = (Up - FVector::DotProduct(Up, Out) * Out).GetSafeNormal();
	if (Up.IsNearlyZero()) Up = FVector(0, 1, 0);
	if (USceneCaptureComponent2D* C = G.Capture.Get())
	{
		if (C->IsRegistered())
		{
			C->SetWorldLocationAndRotation(ToUE(BestAt + Out * CAMERA_OUT), FRotationMatrix::MakeFromXZ(DirToUE(Out), DirToUE(Up)).Rotator());
			// drawn with the main view this frame, not on its own
			C->CaptureSceneDeferred();
		}
	}
	G.Last = Now;
	G.bImage = true;
	++CaptureCount;
	ShowGlass(Best);
}

// ---------- self test ----------

bool FS47Mirrors::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool B, const TCHAR* S) { bOk &= B; Lines.Add(FString(B ? TEXT("ok mirrors: ") : TEXT("FAIL mirrors: ")) + S); };
	const bool bWas = S47Mirrors::Enabled();
	const FVector EyeLocal(-.41, 1.70, .65); // the Jeep's eye (S47TripoFleet)
	FS47Mirrors Mi;
	for (int32 I = 0; I < Count; ++I) Mi.Aim[I] = SPECS[I].Normal.GetSafeNormal();
	Mi.MakeCaptures(GetTransientPackage(), false);
	// the aim: from the Jeep's eye each mirror shows what its table entry asks for
	bool bAim = true;
	for (int32 I = 0; I < Count; ++I)
	{
		const FVector In = (SPECS[I].Centre - EyeLocal).GetSafeNormal(), N = Mi.Aim[I];
		const FVector Out = In - 2 * FVector::DotProduct(In, N) * N;
		bAim &= N.Z > 0 && FVector::DotProduct(In, N) < -.5 && Out.Z > .95; // within ~18 degrees of straight back
	}
	Check(bAim, TEXT("from the Jeep's eye each authored glass looks back behind the car"));
	const FQuat Car = FQuat::Identity, Ahead = FQuat::Identity, Behind(FVector(0, 1, 0), UE_DOUBLE_PI);
	const FVector At = FVector(5, 0, -3), Eye = At + EyeLocal;
	auto Feed = [&](bool bAtWheel, const FQuat& Look, double Now) { Mi.StepAt(S47Mirrors::Enabled() && bAtWheel, Car, At, Eye, Look, Now); };

	S47Mirrors::SetEnabled(false);
	for (int32 F = 0; F < 4; ++F) Feed(true, Ahead, 10 + F * .1);
	Check(Mi.Captures() == 0 && !Mi.Live() && !Mi.CaptureActive(), TEXT("switch off: nothing captured at the wheel, captures inactive"));
	S47Mirrors::SetEnabled(true);
	for (int32 F = 0; F < 4; ++F) Feed(false, Ahead, 11 + F * .1);
	Check(Mi.Captures() == 0 && !Mi.Live() && !Mi.CaptureActive(), TEXT("out of the Jeep: nothing captured, captures inactive"));
	for (int32 F = 0; F < 3; ++F) Feed(true, Ahead, 20 + F * .1);
	bool bTargets = true;
	for (int32 I = 0; I < Count; ++I)
	{
		UTextureRenderTarget2D* RT = Mi.Target(I);
		bTargets &= RT && RT->SizeX == SPECS[I].Pixels.X && RT->SizeY == SPECS[I].Pixels.Y && Mi.Glass[I].Capture && Mi.Glass[I].Capture->TextureTarget.Get() == RT && Mi.Glass[I].bImage;
	}
	Check(Mi.Live() && Mi.CaptureActive() && bTargets, TEXT("switch on at the wheel: every mirror has its render target and a picture"));
	Check(Mi.Captures() == 3, TEXT("one mirror a frame: three frames capture the three mirrors in turn"));
	const int32 Before = Mi.Captures();
	for (int32 F = 0; F < 4; ++F) Feed(true, Ahead, 30 + F * .01);
	Check(Mi.Captures() - Before == 3, TEXT("each mirror at most 20 times a second"));
	const int32 Back = Mi.Captures();
	for (int32 F = 0; F < 3; ++F) Feed(true, Behind, 40 + F * .1);
	Check(Mi.Captures() == Back, TEXT("mirrors out of the view are not captured"));
	Feed(false, Ahead, 50);
	Check(!Mi.Live() && !Mi.CaptureActive() && !Mi.Glass[0].bImage, TEXT("out of the Jeep again: captures inactive, the glass dark"));
	S47Mirrors::SetEnabled(true);
	Feed(true, Ahead, 60);
	S47Mirrors::SetEnabled(false);
	Feed(true, Ahead, 61);
	Check(!Mi.Live() && !Mi.CaptureActive(), TEXT("switching off while driving stops the captures at once"));
	Check(KeyOf(TEXT("MI_JeepMirrorGlass_L")) == 0 && KeyOf(TEXT("mi_jeepmirrorglass_r")) == 1 && KeyOf(TEXT("MI_JeepMirrorGlass_Rear_Inst")) == 2 && KeyOf(TEXT("M_JeepInterior_MirrorSilver")) == -1,
		TEXT("the model's glass slots are recognised by name (MI_JeepMirrorGlass_L, _R, _Rear)"));
	S47Mirrors::SetEnabled(bWas);
	return bOk;
}
