// core/Flashlight.ts
#include "S47Flashlight.h"

#include "Components/SpotLightComponent.h"
#include "GameFramework/Actor.h"
#include "S47Three.h"

namespace
{
	// core/cookies.ts: the cone (the spot light's angle) and how much the light is raised with its cookie
	const double FLASH_ANGLE = 0.4, FLASH_GAIN = 1.6;
	// A reflector torch: 420 cd gives about 26 lux at four metres before surface response.
	constexpr double PEAK_CANDELAS = 420;
}

FS47Flashlight::FS47Flashlight(AActor* Owner)
{
	// a warm incandescent bulb behind a reflector: new SpotLight(0xffe6c0, 0, 30, FLASH_ANGLE, 0.12, FLASH_DECAY)
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
	L->SetIntensity(ThreeIntensity > 0 ? float(PEAK_CANDELAS * Level * K) : 0.f);
	// (three keeps the light at intensity 0 so no shader is rebuilt; Unreal's is simply not drawn while off)
	if (L->IsVisible() != (ThreeIntensity > 0)) L->SetVisibility(ThreeIntensity > 0);
	// held a little right of and below the eyes, pointing 10 m out where they look
	FVector P = Cam.Pos + Cam.Right * 0.22; P.Y -= 0.18;
	const FVector Target = Cam.Pos + Cam.Forward * 10;
	L->SetWorldLocationAndRotation(S47Three::ToUE(P), (S47Three::ToUE(Target) - S47Three::ToUE(P)).Rotation());
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
	return bOk;
}
