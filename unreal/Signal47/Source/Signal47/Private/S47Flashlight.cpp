// core/Flashlight.ts
#include "S47Flashlight.h"

#include "CollisionQueryParams.h"
#include "Components/SpotLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "GameFramework/Actor.h"
#include "S47Three.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Flashlight, Log, All);

namespace
{
	// core/cookies.ts: the cone (the spot light's angle) and how much the light is raised with its cookie
	const double FLASH_ANGLE = 0.4, FLASH_GAIN = 1.6;
	// A reflector torch: 420 cd gives about 26 lux at four metres before surface response.
	constexpr double PEAK_CANDELAS = 420;
	constexpr double NEAR_METRES = 3.5, MIN_NEAR_GAIN = .0025;

	// Perceptual near-field compensation for the fixed-exposure picture, not an electrical effect.
	double NearGain(double Distance)
	{
		if (!FMath::IsFinite(Distance) || Distance < 0 || Distance >= NEAR_METRES) return 1;
		return FMath::Clamp(FMath::Square(Distance / NEAR_METRES), MIN_NEAR_GAIN, 1.0);
	}

	double FilterGain(double Current, double Target, double Dt)
	{
		if (!FMath::IsFinite(Dt) || Dt <= 0) return Current;
		const double Value = Target + (Current - Target) * FMath::Exp(-12.0 * Dt);
		return FMath::IsNearlyEqual(Value, Target, .00001) ? Target : Value;
	}

	// Hidden old areas and collision helpers remain traceable for other systems.
	// Ignore those components individually, never Owner (which owns the whole Web world).
	double BeamSurface(UWorld* World, const FVector& From, const FVector& Direction,
		double RangeCm, UPrimitiveComponent*& Surface)
	{
		Surface = nullptr;
		if (!World) return -1;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(S47FlashlightNear), true);
		const FVector To = From + Direction * RangeCm;
		for (int32 Try = 0; Try < 24; ++Try)
		{
			FHitResult Hit;
			if (!World->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Query)) return -1;
			UPrimitiveComponent* C = Hit.GetComponent();
			bool Visible = C && C->IsVisible() && !C->bHiddenInGame && C->bRenderInMainPass
				&& (!C->GetOwner() || !C->GetOwner()->IsHidden());
			if (Visible)
			{
				if (const UMaterialInterface* Material = C->GetMaterial(FMath::Max<int32>(0, Hit.ElementIndex)))
				{
					float Opacity = 1.f;
					Material->GetScalarParameterValue(FName(TEXT("Opacity")), Opacity);
					if (Material->GetBlendMode() == BLEND_Additive
						|| (Material->GetBlendMode() == BLEND_Translucent && Opacity < .5f)) Visible = false;
				}
			}
			if (Visible) { Surface = C; return Hit.Distance / 100.0; }
			if (!C) return -1;
			Query.AddIgnoredComponent(C);
		}
		return -1;
	}
}

FS47Flashlight::FS47Flashlight(AActor* Owner)
{
	// a warm incandescent bulb behind a reflector: new SpotLight(0xffe6c0, 0, 30, FLASH_ANGLE, 0.12, FLASH_DECAY)
	bLogNear = FParse::Param(FCommandLine::Get(), TEXT("S47FlashlightLog"));
	if (!Owner) return;
	USpotLightComponent* L = NewObject<USpotLightComponent>(Owner, TEXT("S47Flashlight"));
	L->SetupAttachment(Owner->GetRootComponent());
	L->SetMobility(EComponentMobility::Movable);
	L->SetIntensityUnits(ELightUnits::Candelas);
	L->SetIntensity(0.f);
	L->SetLightColor(S47Three::Hex(0xffe6c0));
	L->SetAttenuationRadius(30.f * 100.f);
	L->SetOuterConeAngle(float(FMath::RadiansToDegrees(FLASH_ANGLE)));
	L->SetInnerConeAngle(float(FMath::RadiansToDegrees(FLASH_ANGLE * (1 - 0.12)))); // penumbra 0.12
	L->SetSourceRadius(1.4f);
	L->SetSoftSourceRadius(2.5f);
	L->SetCastShadows(true);
	L->SetCastVolumetricShadow(true);
	// Small foreground cables and handles need a short trace beside the shadow map.
	L->ContactShadowLengthInWS = true; L->ContactShadowLength = 8.f;
	L->ContactShadowCastingIntensity = 0.75f;
	L->SetVolumetricScatteringIntensity(0.8f);
	L->RegisterComponent();
	Spot = L;
}

bool FS47Flashlight::Toggle()
{
	if (!bHave) return false;
	bOn = !bOn;
	return true;
}

void FS47Flashlight::Flicker(double K, const FString& From)
{
	Flicks.Add(From, FMath::Clamp(K, 0.0, 1.0));
	double M = 0;
	for (const TPair<FString, double>& V : Flicks) M = FMath::Max(M, V.Value);
	Flick = M;
}

void FS47Flashlight::Update(double Dt, double T, const FS47Cam& Cam, bool bOff)
{
	const double Want = bHave && bOn && !bOff ? 1 : 0;
	Level += (Want - Level) * FMath::Min(1.0, Dt * 14);
	// a weak contact now and then when something is near: a dip, sometimes out for a moment
	double K = 1;
	if (Flick > 0)
	{
		Dark -= Dt;
		if (Dark <= 0 && FMath::FRand() < Flick * Dt * 1.2) Dark = 0.05 + FMath::FRand() * 0.25 * Flick;
		K = Dark > 0 ? 0.08 : 1 - Flick * 0.25 * (0.5 + 0.5 * FMath::Sin(T * 37) * FMath::Sin(T * 11.3));
	}
	ThreeIntensity = Level < 0.01 ? 0 : 40 * FLASH_GAIN * Level * K;
	USpotLightComponent* L = Spot.Get();
	if (!L) return;
	// (three keeps the light at intensity 0 so no shader is rebuilt; Unreal's is simply not drawn while off)
	if (L->IsVisible() != (ThreeIntensity > 0)) L->SetVisibility(ThreeIntensity > 0);
	// held a little right of and below the eyes, pointing 10 m out where they look
	FVector P = Cam.Pos + Cam.Right * 0.22; P.Y -= 0.18;
	const FVector Target = Cam.Pos + Cam.Forward * 10;
	const FVector From = S47Three::ToUE(P), Direction = (S47Three::ToUE(Target) - From).GetSafeNormal();
	L->SetWorldLocationAndRotation(From, Direction.Rotation());
	UPrimitiveComponent* Surface = nullptr;
	const double HitMetres = ThreeIntensity > 0 ? BeamSurface(L->GetWorld(), From, Direction, L->AttenuationRadius, Surface) : -1;
	const double TargetGain = NearGain(HitMetres);
	if (ThreeIntensity <= 0) { NativeNearGain = 1; bNativeNearReady = false; }
	else if (!bNativeNearReady) { NativeNearGain = TargetGain; bNativeNearReady = true; }
	else NativeNearGain = FilterGain(NativeNearGain, TargetGain, Dt);
	L->SetIntensity(ThreeIntensity > 0 ? float(PEAK_CANDELAS * Level * K * NativeNearGain) : 0.f);
	if (bLogNear)
	{
		const double Now = FPlatformTime::Seconds();
		if (Now >= NextNearLog)
		{
			NextNearLog = Now + .25;
			UE_LOG(LogS47Flashlight, Display, TEXT("S47: flashlight near hit_m=%.3f target_gain=%.5f gain=%.5f spot_cd=%.3f three=%.3f surface=%s"),
				HitMetres, TargetGain, NativeNearGain, L->Intensity, ThreeIntensity, *GetNameSafe(Surface));
		}
	}
}

bool FS47Flashlight::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s flashlight: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	FS47Flashlight F(nullptr);
	FS47Cam Cam;
	Check(!F.Toggle() && !F.bOn, TEXT("none before the truck: F does nothing"));
	F.bHave = true;
	Check(F.Toggle() && F.bOn, TEXT("F switches it on"));
	for (int32 I = 0; I < 60; ++I) F.Update(1.0 / 60, I / 60.0, Cam, false);
	Check(FMath::IsNearlyEqual(F.Intensity(), 64, 0.01), TEXT("on: 40 times the cookie's 1.6"));
	for (int32 I = 0; I < 60; ++I) F.Update(1.0 / 60, I / 60.0, Cam, true);
	Check(F.Intensity() == 0, TEXT("off in the truck"));
	F.Flicker(0.3, TEXT("glimpse")); F.Flicker(0.85, TEXT("curve")); F.Flicker(0, TEXT("curve"));
	Check(FMath::IsNearlyEqual(F.Flick, 0.3), TEXT("the strongest flicker counts, each by its source"));
	Check(NearGain(-1) == 1 && NearGain(3.5) == 1 && NearGain(8) == 1, TEXT("no surface or ordinary range retains the full native beam"));
	bool bNearLux = true;
	for (double D : { .5, 1.0, 2.0 }) bNearLux &= FMath::IsNearlyEqual(PEAK_CANDELAS * NearGain(D) / (D * D), PEAK_CANDELAS / (NEAR_METRES * NEAR_METRES), .00001);
	Check(bNearLux, TEXT("close surfaces target about 34 lux without altering the story intensity"));
	Check(NearGain(0) == MIN_NEAR_GAIN && NearGain(.5) < NearGain(1) && NearGain(1) < NearGain(3.5), TEXT("near compensation stays positive and rises continuously to full range"));
	const double Close = NearGain(1), First = FilterGain(1, Close, 1.0 / 60);
	Check(First > Close && First < 1 && FilterGain(Close, 1, 1.0 / 60) > Close && FilterGain(1, Close, 0) == 1, TEXT("temporal filter changes smoothly without overshoot or a zero-time jump"));
	double At30 = 1, At60 = 1;
	for (int32 I = 0; I < 30; ++I) At30 = FilterGain(At30, Close, 1.0 / 30);
	for (int32 I = 0; I < 60; ++I) At60 = FilterGain(At60, Close, 1.0 / 60);
	Check(FMath::IsNearlyEqual(At30, At60, .00001) && FMath::IsNearlyEqual(At60, Close, .00001), TEXT("near filtering converges equally at 30 and 60 fps"));
	return bOk;
}
