#include "S47Web.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/Engine.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "DynamicRHI.h"
#include "RenderTimer.h"
#include "S47Three.h"

namespace
{
	uint64 PoolId(int32 Set, int32 Index) { return (uint64(uint32(Set)) << 32) | uint32(Index); }
	void Cvar(const TCHAR* Name, int32 Value)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name)) V->Set(Value, ECVF_SetByGameSetting);
	}
}

float AS47Web::LightIntensity(const FString& Uuid) const
{
	const ULightComponent* L = LightByUuid(Uuid);
	return L ? L->Intensity / (L->IsA<UPointLightComponent>() ? 1.5f : 1.f) : 0.f;
}

void AS47Web::SetLightIntensity(const FString& Uuid, float Value)
{
	if (ULightComponent* L = LightByUuid(Uuid)) L->SetIntensity(Value * (L->IsA<UPointLightComponent>() ? 1.5f : 1.f));
}

void AS47Web::SuppressFloodLight(int32 Set, int32 Index)
{
	SuppressedPools.Add(PoolId(Set, Index));
}

void AS47Web::BuildNativeLighting()
{
	PoolRoots.SetNum(FloodSets.Num());
	for (const FS47WebNode& N : Nodes) for (const int32 MatId : N.Mats)
		if (const int32* Set = MatFloodSet.Find(MatId)) PoolRoots[*Set].AddUnique(N.Root);
	for (int32 S = 0; S < FloodSets.Num(); ++S)
	{
		const FFloodSet& F = FloodSets[S];
		for (int32 I = 0; I < F.W; ++I)
		{
			const bool bDown = (F.Key == TEXT("site") && I < 8) || (F.Key == TEXT("field") && I == 2) || (F.Key == TEXT("court") && I >= 10 && I <= 11) || (F.Key == TEXT("town") && I >= 1 && I <= 7);
			UPointLightComponent* L = bDown ? NewObject<USpotLightComponent>(this) : NewObject<UPointLightComponent>(this);
			if (USpotLightComponent* Spot = Cast<USpotLightComponent>(L))
			{
				Spot->SetInnerConeAngle(42.f); Spot->SetOuterConeAngle(78.f);
				Spot->SetWorldRotation(FRotator(-90, 0, 0));
			}
			L->SetupAttachment(GetRootComponent());
			L->SetMobility(EComponentMobility::Movable);
			L->SetIntensityUnits(ELightUnits::Candelas);
			L->SetIntensity(0);
			L->SetSourceRadius(F.Key == TEXT("annex") || F.Key == TEXT("diner") ? 12.f : 7.f);
			L->SetSoftSourceRadius(4.f);
			L->SetCastShadows(true);
			L->SetCastVolumetricShadow(true);
			L->SetVolumetricScatteringIntensity(0.6f);
			L->SetIndirectLightingIntensity(1.f);
			L->RegisterComponent();
			PoolLights.Add({ S, I, FMath::Max(F.Pos[I].A, 1.f), L });
		}
	}
	AExponentialHeightFog* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>();
	LightingFog = Fog->GetComponent();
	LightingFog->SetVolumetricFog(true);
	LightingFog->SetFogHeightFalloff(0.18f);
	LightingFog->SetVolumetricFogScatteringDistribution(0.35f);
	LightingFog->SetVolumetricFogAlbedo(FColor(210, 220, 235));
	LightingFog->SetVolumetricFogDistance(14000.f);
	LightingFog->SetFogMaxOpacity(0.92f);
	NightSky = NewObject<USkyLightComponent>(this);
	NightSky->SetupAttachment(GetRootComponent());
	NightSky->SetMobility(EComponentMobility::Movable);
	NightSky->CubemapResolution = 64;
	NightSky->SkyDistanceThreshold = 1500000.f;
	NightSky->SetIntensity(0.18f);
	NightSky->SetLightColor(FLinearColor(0.42f, 0.57f, 1.f));
	NightSky->RegisterComponent();
	DawnSun = NewObject<UDirectionalLightComponent>(this);
	DawnSun->SetupAttachment(GetRootComponent());
	DawnSun->SetMobility(EComponentMobility::Movable);
	DawnSun->SetLightColor(FLinearColor(1.f, 0.55f, 0.28f));
	DawnSun->SetIntensity(0);
	DawnSun->SetCastShadows(true);
	DawnSun->LightSourceAngle = 1.0f;
	DawnSun->RegisterComponent();
	SetLightingQuality(TEXT("high"));
	UpdateNativeFog();
	UE_LOG(LogTemp, Display, TEXT("S47: native lighting, %d lamp slots, Lumen (hardware when available), fixed exposure, no material floods or hemisphere"), PoolLights.Num());
}

void AS47Web::SetLightingQuality(const FString& Quality)
{
	LightingQuality = Quality == TEXT("low") ? 0 : Quality == TEXT("ultra") ? 2 : 1;
	Cvar(TEXT("r.Shadow.Virtual.OnePassProjection.MaxLightsPerPixel"), 32);
	Cvar(TEXT("r.ScreenPercentage"), LightingQuality < 2 ? 60 : 100);
	Cvar(TEXT("sg.ShadowQuality"), LightingQuality == 0 ? 1 : LightingQuality == 1 ? 2 : 3);
	Cvar(TEXT("sg.AntiAliasingQuality"), LightingQuality == 0 ? 1 : LightingQuality == 1 ? 2 : 3);
	// Non-Nanite export meshes benefit from fewer mip duplicates. Keep the coarse pages
	// and volumetric occlusion, but spend fewer SMRT samples on the desktop 60 fps profile.
	Cvar(TEXT("r.Shadow.Virtual.MarkPixelPagesMipModeLocal"), LightingQuality == 2 ? 0 : 2);
	Cvar(TEXT("r.Shadow.Virtual.SMRT.RayCountLocal"), LightingQuality == 2 ? 8 : LightingQuality == 1 ? 4 : 2);
	Cvar(TEXT("r.Shadow.Virtual.SMRT.RayCountDirectional"), LightingQuality == 2 ? 8 : LightingQuality == 1 ? 4 : 2);
	Cvar(TEXT("r.Shadow.Virtual.SMRT.SamplesPerRayLocal"), LightingQuality == 2 ? 4 : 2);
	Cvar(TEXT("r.Shadow.Virtual.SMRT.SamplesPerRayDirectional"), LightingQuality == 2 ? 4 : 2);
	Cvar(TEXT("sg.GlobalIlluminationQuality"), LightingQuality == 0 ? 1 : LightingQuality == 1 ? 2 : 3);
	Cvar(TEXT("sg.ReflectionQuality"), LightingQuality == 0 ? 1 : 2);
	Cvar(TEXT("r.VolumetricFog"), LightingQuality > 0);
	Cvar(TEXT("r.VolumetricFog.GridPixelSize"), LightingQuality == 2 ? 8 : 16);
	Cvar(TEXT("r.VolumetricFog.GridSizeZ"), LightingQuality == 2 ? 96 : 64);
	Cvar(TEXT("r.Shadow.Virtual.ResolutionLodBiasLocal"), LightingQuality == 2 ? 0 : 1);
	if (LightingPost)
	{
		FPostProcessSettings& P = LightingPost->Settings;
		P.bOverride_DynamicGlobalIlluminationMethod = true;
		P.DynamicGlobalIlluminationMethod = LightingQuality == 0 ? EDynamicGlobalIlluminationMethod::None : EDynamicGlobalIlluminationMethod::Lumen;
		P.bOverride_LumenSceneLightingQuality = true; P.LumenSceneLightingQuality = LightingQuality == 2 ? 1.f : 0.5f;
		P.bOverride_LumenFinalGatherQuality = true; P.LumenFinalGatherQuality = LightingQuality == 2 ? 1.f : 0.5f;
	}
	UE_LOG(LogTemp, Display, TEXT("S47: lighting quality %s, Lumen %s, volumetrics %s"), *Quality, LightingQuality ? TEXT("on") : TEXT("off"), LightingQuality ? TEXT("on") : TEXT("off"));
}

void AS47Web::SetSkyLighting(float Dawn, const FVector& SunDirection, float Sun, float Flash)
{
	SkyDawn = Dawn; SkySun = Sun; SkyFlash = Flash;
	if (DawnSun)
	{
		DawnSun->SetWorldRotation(S47Three::DirToUE(-SunDirection).Rotation());
		DawnSun->SetIntensity(Sun * 8.f);
	}
	if (NightSky) NightSky->SetIntensity(0.18f + Dawn * 1.8f);
}

void AS47Web::UpdateNativeFog()
{
	if (!LightingFog) return;
	// FogExp2 is not a physical extinction coefficient. Match its thin night air near eye level,
	// with a long view distance; local lamps and beams now scatter through the actual volume.
	LightingFog->SetFogDensity(FMath::Clamp(FogDensity * 2.f, 0.0003f, 0.012f));
	LightingFog->SetFogInscatteringColor(FogColor * 0.12f);
}

void AS47Web::UpdateNativeLighting()
{
	const FVector Camera = S47Three::ToThree(Eye);
	TArray<FVector> Occupied;
	for (FPoolLight& P : PoolLights)
	{
		UPointLightComponent* L = P.Light.Get();
		if (!L) continue;
		const FFloodSet& F = FloodSets[P.Set];
		const FLinearColor V = F.Pos[P.Slot], C = F.Col[P.Slot];
		const FVector Pos(V.R, V.G, V.B);
		const float Dist = FVector::Distance(Pos, Camera);
		bool bArea = PoolRoots[P.Set].IsEmpty();
		for (const int32 Root : PoolRoots[P.Set]) if (Nodes.IsValidIndex(Root) && Nodes[Root].bVis) bArea = true;
		bool bOn = bArea && V.A > 0.001f && F.Scale > 0 && Pos.Y > -900 && Dist < 180 && !SuppressedPools.Contains(PoolId(P.Set, P.Slot));
		// Shared pools are copies for materials in the adjacent area, never extra fixtures.
		for (const FVector& O : Occupied) if (Pos.Equals(O, 0.12)) { bOn = false; break; }
		if (bOn) Occupied.Add(Pos);
		L->SetVisibility(bOn);
		if (!bOn) continue;
		P.Reach = FMath::Max(P.Reach, V.A);
		L->SetWorldLocation(S47Three::ToUE(Pos));
		const float ColorGain = FMath::Max(1.f, FMath::Max3(C.R, C.G, C.B));
		L->SetLightColor(C / ColorGain);
		const bool bInside = F.Key == TEXT("annex") || F.Key == TEXT("lab") || F.Key == TEXT("hut") || F.Key == TEXT("cab") || F.Key == TEXT("motel");
		const float Gain = F.Key == TEXT("cab") ? 0.3f : bInside ? 12.f : F.Key == TEXT("diner") ? 18.f : 50.f;
		L->SetIntensity(V.A * F.Scale * Gain * ColorGain);
		const float Limit = F.Key == TEXT("cab") ? 2.f : F.Key == TEXT("hut") ? 5.f : bInside ? 7.f : F.Key == TEXT("diner") ? 8.f : 70.f;
		L->SetAttenuationRadius(FMath::Min(Limit, P.Reach * 1.6f + 8.f) * 100.f);
		// A light that reaches the eye retains its occlusion, including walls behind the camera.
		const float Reach = L->AttenuationRadius * 0.01f;
		L->SetCastShadows(LightingQuality > 0 || Dist < Reach + 5.f);
	}
	for (FLightRef& R : Lights) if (UDirectionalLightComponent* Moon = Cast<UDirectionalLightComponent>(R.L.Get()))
	{
		// Keep each area's original moon direction; the sun is driven by the living sky above.
		Moon->SetIntensity(R.Base > 0 ? 0.16f * (1.f - SkyDawn) + SkyFlash * 0.8f : 0.f);
	}
	if (NightSky && LightingRunTime > 2 && LightingRunTime - SkyCaptureTime > 2 && (!bSkyCaptured || FMath::Abs(CapturedDawn - SkyDawn) > 0.08f))
	{
		NightSky->RecaptureSky(); bSkyCaptured = true; CapturedDawn = SkyDawn; SkyCaptureTime = LightingRunTime;
	}
}

void AS47Web::SampleLightingPerformance(float Dt)
{
	LightingRunTime += Dt;
	if (!bLightingProfiled && LightingRunTime > 7 && FParse::Param(FCommandLine::Get(), TEXT("S47GpuProfile")))
	{
		bLightingProfiled = true;
		if (GEngine) GEngine->Exec(GetWorld(), TEXT("ProfileGPU"));
	}
	if (!FParse::Param(FCommandLine::Get(), TEXT("S47LightPerf")) || LightingRunTime < 5) return;
	PerfFrames.Add(Dt * 1000.f);
	PerfGame.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));
	PerfDraw.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime));
	PerfGpu.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));
	if (LightingRunTime - LightingLogTime < 1) return;
	LightingLogTime = LightingRunTime;
	auto Mean = [](const TArray<float>& V) { double Sum = 0; for (float X : V) Sum += X; return Sum / V.Num(); };
	TArray<float> Ordered = PerfFrames; Ordered.Sort();
	UE_LOG(LogTemp, Display, TEXT("S47: lightperf t=%.2f n=%d frame=%.3f p95=%.3f game=%.3f draw=%.3f gpu=%.3f ms quality=%d"), LightingRunTime, PerfFrames.Num(), Mean(PerfFrames), Ordered[FMath::Min(Ordered.Num() - 1, int32(Ordered.Num() * 0.95))], Mean(PerfGame), Mean(PerfDraw), Mean(PerfGpu), LightingQuality);
}
