#include "S47UfoPresentation.h"

#include "S47SkyVisitors.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const TCHAR* SaucerPath = TEXT("/Game/Licensed/TripoFleet/ufo_saucer/SM_TripoFleet_Saucer.SM_TripoFleet_Saucer");
	const FLinearColor Red(1.f, .055f, .025f);
	const FLinearColor WarmWhite(1.f, .82f, .76f);
	const FLinearColor NearWhite(1.f, .84f, .93f);
	constexpr int32 DustCount = 12;
	constexpr int32 HazeCount = 7;
	bool FiniteVector(const FVector& P)
	{
		return !P.ContainsNaN() && FMath::Abs(P.X) < 1e6 && FMath::Abs(P.Y) < 1e6 && FMath::Abs(P.Z) < 1e6;
	}
	double Bounded(double V, double Lo, double Hi, double Default)
	{
		return FMath::IsFinite(V) ? FMath::Clamp(V, Lo, Hi) : Default;
	}
	TArray<double> XYZ(const FVector& V) { return { V.X, V.Y, V.Z }; }
}

struct FS47UfoPresentation::FVisual
{
	TWeakObjectPtr<AS47Web> Web;
	TWeakObjectPtr<UStaticMeshComponent> Hull;
	TWeakObjectPtr<UStaticMeshComponent> Lamps[3], Halos[3], Core, CoreHalo;
	TWeakObjectPtr<UMaterialInstanceDynamic> LampMids[3], HaloMids[3], CoreMid, CoreHaloMid;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> Dust, Haze;
	TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> DustMids, HazeMids;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> Meshes;
	TWeakObjectPtr<USpotLightComponent> Beam;
	TWeakObjectPtr<UPointLightComponent> RimLight, NearLight;
	FFrame Frame;
	FVector NativeCentre = FVector::ZeroVector;
	double NativeWidth = 100, NativeHalfHeight = 3;
	bool bNative = false, bReady = false, bShown = false, bLow = false;
	int32 ShownDust = 0, ShownHaze = 0;
	double AppliedBeam = 0, AppliedScatter = 0;

	bool CanTouch() const
	{
		auto* W = Web.Get();
		return !IsEngineExitRequested() && !IsGarbageCollecting() && IsValid(W)
			&& !W->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed) && !W->IsActorBeingDestroyed()
			&& IsValid(W->GetWorld()) && !W->GetWorld()->bIsTearingDown;
	}
	void Hide()
	{
		bShown = false; ShownDust = ShownHaze = 0; AppliedBeam = AppliedScatter = 0;
		if (!CanTouch()) return;
		for (auto& C : Meshes) if (C.IsValid()) C->SetVisibility(false, false);
		for (ULightComponent* L : { static_cast<ULightComponent*>(Beam.Get()), static_cast<ULightComponent*>(RimLight.Get()), static_cast<ULightComponent*>(NearLight.Get()) })
			if (IsValid(L)) { L->SetIntensity(0); L->SetVisibility(false); }
	}
	void Drop()
	{
		Hide();
		if (CanTouch())
		{
			for (auto& C : Meshes) if (C.IsValid()) C->DestroyComponent();
			if (Beam.IsValid()) Beam->DestroyComponent();
			if (RimLight.IsValid()) RimLight->DestroyComponent();
			if (NearLight.IsValid()) NearLight->DestroyComponent();
		}
		Meshes.Reset(); Dust.Reset(); DustMids.Reset(); Haze.Reset(); HazeMids.Reset();
		Hull.Reset(); Core.Reset(); CoreHalo.Reset(); CoreMid.Reset(); CoreHaloMid.Reset();
		for (int32 I = 0; I < 3; ++I) { Lamps[I].Reset(); Halos[I].Reset(); LampMids[I].Reset(); HaloMids[I].Reset(); }
		Beam.Reset(); RimLight.Reset(); NearLight.Reset(); bReady = false;
	}
	UMaterialInstanceDynamic* Material(UMaterialInterface* Base, const FLinearColor& Color, bool bGlow)
	{
		auto* Mid = UMaterialInstanceDynamic::Create(Base, Web.Get());
		Mid->SetVectorParameterValue(TEXT("Color"), Color);
		if (bGlow) Mid->SetScalarParameterValue(TEXT("Brightness"), 0.f);
		else
		{
			Mid->SetScalarParameterValue(TEXT("Roughness"), .62f);
			Mid->SetScalarParameterValue(TEXT("Metalness"), .35f);
			Mid->SetScalarParameterValue(TEXT("NativeLighting"), 1.f);
			Mid->SetScalarParameterValue(TEXT("EmissivePower"), 0.f);
		}
		return Mid;
	}
	UStaticMeshComponent* Mesh(const TCHAR* Name, UStaticMesh* Asset, UMaterialInterface* Mat = nullptr)
	{
		auto* W = Web.Get();
		auto* C = NewObject<UStaticMeshComponent>(W, MakeUniqueObjectName(W, UStaticMeshComponent::StaticClass(), FName(Name)));
		C->SetupAttachment(W->GetRootComponent()); C->SetMobility(EComponentMobility::Movable); C->SetStaticMesh(Asset);
		if (Mat) C->SetMaterial(0, Mat);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetGenerateOverlapEvents(false);
		C->SetCanEverAffectNavigation(false); C->SetCastShadow(false); C->bAffectDistanceFieldLighting = false;
		C->bVisibleInReflectionCaptures = false; C->bVisibleInRealTimeSkyCaptures = false;
		C->SetVisibility(false); C->RegisterComponent(); Meshes.Add(C); return C;
	}
	void Billboard(UStaticMeshComponent* C, const FVector& AtUE, double WidthM, const FVector& EyeUE)
	{
		if (!C) return;
		const FVector ToEye = (EyeUE - AtUE).GetSafeNormal();
		C->SetWorldLocation(AtUE);
		C->SetWorldRotation(FRotationMatrix::MakeFromZ(ToEye.IsNearlyZero() ? FVector::ForwardVector : ToEye).Rotator());
		// Engine plane is one metre wide, with its normal along local Z.
		C->SetWorldScale3D(FVector(WidthM, WidthM, 1));
		C->SetVisibility(true);
	}
};

FS47UfoPresentation::FS47UfoPresentation(AS47Web* InWeb) : Visual(MakeUnique<FVisual>()) { Visual->Web = InWeb; }
FS47UfoPresentation::~FS47UfoPresentation() { Visual->Drop(); }

bool FS47UfoPresentation::Prepare()
{
	auto& V = *Visual;
	if (!V.CanTouch() || !V.Web->IsLoaded()) return false;
	if (V.bReady && V.Hull.IsValid() && V.Beam.IsValid()) return true;
	auto* Lit = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
	auto* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebGlow.M_WebGlow"));
	auto* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	auto* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	auto* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (!Lit || !Glow || !Sphere || !Plane || !Cylinder) return false;
	V.Drop();
	UStaticMesh* Native = nullptr;
	if (FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(SaucerPath))))
		Native = LoadObject<UStaticMesh>(nullptr, SaucerPath, nullptr, LOAD_NoWarn);
	V.bNative = Native != nullptr;
	V.Hull = V.Mesh(TEXT("S47UfoHull"), Native ? Native : Cylinder,
		Native ? nullptr : V.Material(Lit, FLinearColor(.024f, .029f, .035f), false));
	const auto Bounds = V.Hull->GetStaticMesh()->GetBounds();
	V.NativeCentre = FVector(Bounds.Origin);
	V.NativeWidth = FMath::Max(1., double(FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y)) * 2.);
	V.NativeHalfHeight = double(Bounds.BoxExtent.Z);
	for (int32 I = 0; I < 3; ++I)
	{
		auto* M = V.Material(Lit, Red, false); V.LampMids[I] = M;
		V.Lamps[I] = V.Mesh(TEXT("S47UfoLamp"), Sphere, M);
		auto* H = V.Material(Glow, Red, true); V.HaloMids[I] = H;
		V.Halos[I] = V.Mesh(TEXT("S47UfoHalo"), Plane, H);
	}
	auto* CoreM = V.Material(Lit, NearWhite, false); V.CoreMid = CoreM;
	V.Core = V.Mesh(TEXT("S47UfoUndersideCore"), Sphere, CoreM);
	auto* CoreH = V.Material(Glow, NearWhite, true); V.CoreHaloMid = CoreH;
	V.CoreHalo = V.Mesh(TEXT("S47UfoCoreHalo"), Plane, CoreH);
	for (int32 I = 0; I < DustCount; ++I)
	{
		auto* M = V.Material(Glow, WarmWhite, true); V.DustMids.Add(M);
		V.Dust.Add(V.Mesh(TEXT("S47UfoDust"), Plane, M));
	}
	for (int32 I = 0; I < HazeCount; ++I)
	{
		auto* M = V.Material(Glow, NearWhite, true); V.HazeMids.Add(M);
		V.Haze.Add(V.Mesh(TEXT("S47UfoAirLight"), Plane, M));
	}
	auto* W = V.Web.Get();
	auto* Beam = NewObject<USpotLightComponent>(W, MakeUniqueObjectName(W, USpotLightComponent::StaticClass(), FName(TEXT("S47UfoGroundLight"))));
	Beam->SetupAttachment(W->GetRootComponent()); Beam->SetMobility(EComponentMobility::Movable);
	Beam->SetIntensityUnits(ELightUnits::Candelas); Beam->SetUseInverseSquaredFalloff(true);
	Beam->SetIntensity(0); Beam->SetCastShadows(true); Beam->SetCastVolumetricShadow(false);
	Beam->SetIndirectLightingIntensity(0); Beam->SetVolumetricScatteringIntensity(0);
	Beam->SetSourceRadius(18); Beam->SetSoftSourceRadius(30); Beam->SetSpecularScale(.35f);
	Beam->SetVisibility(false); Beam->RegisterComponent(); V.Beam = Beam;
	for (int32 I = 0; I < 2; ++I)
	{
		auto* L = NewObject<UPointLightComponent>(W, MakeUniqueObjectName(W, UPointLightComponent::StaticClass(), FName(TEXT("S47UfoLocalLight"))));
		L->SetupAttachment(W->GetRootComponent()); L->SetMobility(EComponentMobility::Movable);
		L->SetIntensityUnits(ELightUnits::Candelas); L->SetUseInverseSquaredFalloff(true);
		L->SetIntensity(0); L->SetAttenuationRadius(900); L->SetSourceRadius(30); L->SetSoftSourceRadius(50);
		L->SetCastShadows(false); L->SetCastVolumetricShadow(false); L->SetIndirectLightingIntensity(0);
		L->SetVolumetricScatteringIntensity(0); L->SetSpecularScale(.25f);
		L->SetVisibility(false); L->RegisterComponent();
		if (I == 0) V.RimLight = L; else V.NearLight = L;
	}
	V.bReady = true;
	return true;
}

void FS47UfoPresentation::Show(const FFrame& Input)
{
	auto& V = *Visual;
	if (!FiniteVector(Input.Position) || !FiniteVector(Input.GroundTarget) || !FiniteVector(Input.Forward)) { Hide(); return; }
	V.Frame = Input;
	auto& F = V.Frame;
	F.Seconds = Bounded(F.Seconds, 0, 1e9, 0); F.Width = Bounded(F.Width, 2, 60, 8);
	F.Opacity = Bounded(F.Opacity, 0, 1, 0); F.Beam = Bounded(F.Beam, 0, 1, 0); F.NearWhite = Bounded(F.NearWhite, 0, 1, 0);
	F.Forward.Y = 0; F.Forward = F.Forward.GetSafeNormal(); if (F.Forward.IsNearlyZero()) F.Forward = FVector(0, 0, -1);
	if (F.Opacity <= .001 || !Prepare()) { Hide(); return; }
	V.bShown = true; V.bLow = V.Web->GetGraphicsSettings().Effects == 0;
	const FVector PositionUE = S47Three::ToUE(F.Position), TargetUE = S47Three::ToUE(F.GroundTarget);
	const FQuat Rotation = FRotationMatrix::MakeFromXZ(S47Three::DirToUE(F.Forward), FVector::UpVector).ToQuat();
	const FVector Right = FVector(-F.Forward.Z, 0, F.Forward.X);
	const double Scale = F.Width * 100 / V.NativeWidth;
	const FVector HullScale = V.bNative ? FVector(Scale) : FVector(Scale, Scale, Scale * .06);
	const FVector HullOffset = V.NativeCentre * HullScale;
	V.Hull->SetWorldLocation(PositionUE - Rotation.RotateVector(HullOffset)); V.Hull->SetWorldRotation(Rotation);
	V.Hull->SetWorldScale3D(HullScale); V.Hull->SetVisibility(F.bHull);
	const double Drop = V.bNative ? FMath::Max(.16, V.NativeHalfHeight * Scale * .01 + .06) : F.Width * .04;
	FVector EyeUE = PositionUE + FVector(1500, 0, -1000);
	if (auto* PC = V.Web->GetWorld()->GetFirstPlayerController()) if (PC->PlayerCameraManager) EyeUE = PC->PlayerCameraManager->GetCameraLocation();
	const FVector Offsets[3] = { Right * (-F.Width * .31) + F.Forward * (F.Width * .16),
		Right * (F.Width * .31) + F.Forward * (F.Width * .16), F.Forward * (-F.Width * .32) };
	for (int32 I = 0; I < 3; ++I)
	{
		const FVector At = S47Three::ToUE(F.Position + Offsets[I] - FVector(0, Drop, 0));
		double Pulse = F.bCalm ? 1. : .89 + .11 * FMath::Sin(F.Seconds * .8 + I * 2.1);
		const FLinearColor Base = I == 1 ? FMath::Lerp(Red, WarmWhite, .3f) : Red;
		FLinearColor Color = FMath::Lerp(Base, NearWhite, float(F.NearWhite));
		// UFO-TWINKLE-20261110: the sky lights' own glint (FS47SkyGlint, 9 Nov) at a quarter of its pace, per lamp: bigger core and halo,
		// slow size and colour drift. Calm flashes damp it to a fifth; F10's plain comparison draws the old lamps.
		double LampK = 1., HaloK = 1.;
		if (!FS47SkyGlint::Plain())
		{
			double Swell = 1.;
			const FVector Gain = FS47SkyGlint::Gains(F.Seconds * .25, 100 + I, FMath::DegreesToRadians(20.), &Swell);
			const double Damp = F.bCalm ? .2 : 1., Mean = (Gain.X + Gain.Y + Gain.Z) / 3.;
			Pulse *= 1. + Damp * (Mean - 1.);
			Color.R *= float(1. + Damp * (Gain.X / Mean - 1.)); Color.G *= float(1. + Damp * (Gain.Y / Mean - 1.)); Color.B *= float(1. + Damp * (Gain.Z / Mean - 1.));
			const double Drift = 1. + Damp * (Swell - 1.);
			LampK = FS47SkyGlint::SizeScale * Drift; HaloK = FS47SkyGlint::SizeScale * 1.25 * Drift;
		}
		V.Lamps[I]->SetWorldLocation(At); V.Lamps[I]->SetWorldScale3D(FVector(F.Width * .035 * LampK)); V.Lamps[I]->SetVisibility(true);
		V.LampMids[I]->SetVectorParameterValue(TEXT("Color"), Color);
		V.LampMids[I]->SetScalarParameterValue(TEXT("EmissivePower"), float(F.Opacity * Pulse * (7 + 7 * F.NearWhite)));
		V.Billboard(V.Halos[I].Get(), At, F.Width * (.19 + .09 * F.NearWhite) * HaloK, EyeUE);
		V.HaloMids[I]->SetVectorParameterValue(TEXT("Color"), Color);
		V.HaloMids[I]->SetScalarParameterValue(TEXT("Brightness"), float(F.Opacity * Pulse * (.65 + F.NearWhite)));
	}
	const FVector CoreAt = PositionUE - FVector(0, 0, (Drop + .08) * 100);
	const double CoreK = F.Opacity * FMath::Max(F.Beam, F.NearWhite);
	V.Core->SetWorldLocation(CoreAt); V.Core->SetWorldScale3D(FVector(F.Width * .075, F.Width * .075, .08));
	V.Core->SetVisibility(CoreK > .001); V.CoreMid->SetScalarParameterValue(TEXT("EmissivePower"), float(CoreK * 16));
	V.Billboard(V.CoreHalo.Get(), CoreAt, F.Width * .6, EyeUE);
	V.CoreHalo->SetVisibility(CoreK > .001); V.CoreHaloMid->SetScalarParameterValue(TEXT("Brightness"), float(CoreK * 1.6));

	// The ground is lit by an actual shadowed source. The inverse-square compensation
	// holds the requested patch readable at different altitudes, with a finite ceiling.
	const double BeamDistance = FMath::Max(2., FVector::Distance(CoreAt, TargetUE) * .01);
	const double Radius = F.Width * (.45 + .2 * F.NearWhite);
	const float Outer = float(FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(Radius, BeamDistance)), 4., 55.));
	const FLinearColor BeamColor = FMath::Lerp(Red, NearWhite, float(F.NearWhite));
	const FVector Aim = (TargetUE - CoreAt).GetSafeNormal();
	V.Beam->SetWorldLocation(CoreAt); V.Beam->SetWorldRotation((Aim.IsNearlyZero() ? FVector::DownVector : Aim).Rotation());
	V.Beam->SetInnerConeAngle(Outer * .58f); V.Beam->SetOuterConeAngle(Outer);
	V.Beam->SetAttenuationRadius(float((BeamDistance + Radius * 2) * 100));
	V.AppliedBeam = F.Opacity * F.Beam * FMath::Min(180000., BeamDistance * BeamDistance * (8. + 24. * F.NearWhite));
	V.AppliedScatter = F.bSlowBeam && !V.bLow && V.Web->GetGraphicsSettings().Fog > 0 ? .65 : 0;
	V.Beam->SetIntensity(float(V.AppliedBeam)); V.Beam->SetLightColor(BeamColor);
	V.Beam->SetVolumetricScatteringIntensity(float(V.AppliedScatter)); V.Beam->SetVisibility(V.AppliedBeam > .001);
	V.RimLight->SetWorldLocation(CoreAt); V.RimLight->SetLightColor(BeamColor);
	V.RimLight->SetAttenuationRadius(float(F.Width * 180));
	V.RimLight->SetIntensity(float(F.Opacity * (22 + 34 * F.NearWhite))); V.RimLight->SetVisibility(F.bHull);
	// This fill stays at the physical source, never at the camera or behind cabin walls.
	V.NearLight->SetWorldLocation(CoreAt); V.NearLight->SetLightColor(NearWhite);
	V.NearLight->SetAttenuationRadius(float(F.Width * 280));
	V.NearLight->SetIntensity(float(CoreK * 110)); V.NearLight->SetVisibility(CoreK > .001);

	V.ShownDust = F.Beam > .05 ? (V.bLow ? 3 : DustCount) : 0;
	const FLinearColor DustColor = FMath::Lerp(FLinearColor(1.f, .24f, .11f), NearWhite, float(F.NearWhite));
	for (int32 I = 0; I < V.Dust.Num(); ++I)
	{
		auto* C = V.Dust[I].Get(); if (!C) continue;
		if (I >= V.ShownDust) { C->SetVisibility(false); continue; }
		const double Cycle = FMath::Frac(F.Seconds * .12 + I * .61803398875);
		const double Angle = I * 2.399963 + F.Seconds * .11;
		const double Spread = Radius * (.18 + .5 * FMath::Frac(I * .381966));
		const FVector At = F.GroundTarget + FVector(FMath::Cos(Angle) * Spread, .15 + Cycle * (F.bSlowBeam ? 4.5 : 1.7), FMath::Sin(Angle) * Spread);
		V.Billboard(C, S47Three::ToUE(At), .13 + .2 * FMath::Frac(I * .73), EyeUE);
		V.DustMids[I]->SetVectorParameterValue(TEXT("Color"), DustColor);
		V.DustMids[I]->SetScalarParameterValue(TEXT("Brightness"), float(F.Opacity * F.Beam * .16 * FMath::Sin(Cycle * PI)));
	}
	// A few bounded, depth-tested patches of air retain a visible shaft on fast
	// passes without feeding a rapidly moving light into temporal volumetric fog.
	V.ShownHaze = F.Beam > .05 ? (V.bLow ? 3 : HazeCount) : 0;
	for (int32 I = 0; I < V.Haze.Num(); ++I)
	{
		auto* C = V.Haze[I].Get(); if (!C) continue;
		if (I >= V.ShownHaze) { C->SetVisibility(false); continue; }
		const double T = (I + .65) / (V.ShownHaze + .65);
		const FVector At = FMath::Lerp(CoreAt, TargetUE, T);
		V.Billboard(C, At, FMath::Lerp(.5, Radius * 1.6, T), EyeUE);
		V.HazeMids[I]->SetVectorParameterValue(TEXT("Color"), BeamColor);
		V.HazeMids[I]->SetScalarParameterValue(TEXT("Brightness"), float(F.Opacity * F.Beam * .055));
	}
}

void FS47UfoPresentation::Hide() { Visual->Hide(); }
bool FS47UfoPresentation::Presented() const
{
	const auto& V = *Visual;
	if (!V.CanTouch() || !V.bShown || V.Web->IsHidden()) return false;
	const auto* C = V.Lamps[0].Get();
	return IsValid(C) && C->IsRegistered() && C->IsVisible() && !C->bHiddenInGame;
}
const UPrimitiveComponent* FS47UfoPresentation::SightTarget() const
{
	if (!Presented()) return nullptr;
	return Visual->Frame.bHull ? Visual->Hull.Get() : Visual->Lamps[0].Get();
}
TArray<UStaticMeshComponent*> FS47UfoPresentation::PhotoParts() const
{
	TArray<UStaticMeshComponent*> Out;
	if (!Presented()) return Out;
	auto Add = [&](UStaticMeshComponent* C)
	{
		if (IsValid(C) && C->IsRegistered() && C->IsVisible() && !C->bHiddenInGame) Out.Add(C);
	};
	Add(Visual->Hull.Get());
	for (const auto& L : Visual->Lamps) Add(L.Get());
	return Out;
}
S47J::FObj FS47UfoPresentation::Readback() const
{
	const auto& V = *Visual;
	auto O = S47J::New();
	S47J::Set(O, TEXT("ready"), V.bReady); S47J::Set(O, TEXT("presented"), Presented());
	S47J::Set(O, TEXT("nativeSaucer"), V.bNative); S47J::Set(O, TEXT("low"), V.bLow);
	S47J::Set(O, TEXT("position"), XYZ(V.Frame.Position)); S47J::Set(O, TEXT("groundTarget"), XYZ(V.Frame.GroundTarget));
	S47J::Set(O, TEXT("seconds"), V.Frame.Seconds); S47J::Set(O, TEXT("widthM"), V.Frame.Width);
	S47J::Set(O, TEXT("opacity"), V.Frame.Opacity); S47J::Set(O, TEXT("nearWhite"), V.Frame.NearWhite);
	S47J::Set(O, TEXT("calm"), V.Frame.bCalm); S47J::Set(O, TEXT("beamCandela"), V.AppliedBeam);
	S47J::Set(O, TEXT("volumetricScattering"), V.AppliedScatter); S47J::Set(O, TEXT("dustCount"), V.ShownDust);
	S47J::Set(O, TEXT("hazeCount"), V.ShownHaze);
	S47J::Set(O, TEXT("componentCount"), V.Meshes.Num() + int32(V.Beam.IsValid()) + int32(V.RimLight.IsValid()) + int32(V.NearLight.IsValid()));
	return O;
}
