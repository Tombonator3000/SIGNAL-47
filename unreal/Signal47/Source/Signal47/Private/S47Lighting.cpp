#include "S47Web.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
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
#include "S47Sky.h"

namespace
{
	uint64 PoolId(int32 Set, int32 Index) { return (uint64(uint32(Set)) << 32) | uint32(Index); }
	// Exterior.ts's five sodium floods share their posts with ext.glow points 0..4.
	// The old y=3 coordinates describe material pools, not the 6.5 m fixture heads.
	constexpr int32 ArrayDishes[5] = { 2, 1, 0, 3, 4 }; // S-03, S-02, S-01, S-04, S-05
	constexpr float ArrayWidths[5] = { 46.f, 40.f, 34.f, 30.f, 28.f };
	constexpr float ArrayCandelas[5] = { 1800.f, 540.f, 500.f, 500.f, 500.f };
	constexpr float ArrayCones[5] = { 30.f, 44.f, 42.f, 44.f, 46.f };
	constexpr float ArrayRadii[5] = { 44.f, 32.f, 32.f, 32.f, 32.f };
	FVector DishBowlTarget(const AS47Web& W, int32 I)
	{
		const double Az = FMath::DegreesToRadians(W.DishAz(I)), El = FMath::DegreesToRadians(W.DishEl(I)), Scale = W.DishScale(I);
		// Dish.ts: elevation pivot at 7.8+4.6; the 9 m reflector's circular rim lies at
		// local y=1.1+9^2/(4*7.2). Transform its centre through the actual pointing pose.
		const FVector Normal(FMath::Cos(El) * FMath::Sin(Az), FMath::Sin(El), -FMath::Cos(El) * FMath::Cos(Az));
		return FVector(W.DishX(I), 12.4 * Scale, W.DishZ(I)) + Normal * ((1.1 + 81.0 / 28.8) * Scale);
	}
	FVector ArrayBowlTarget(const AS47Web& W, int32 Slot) { return DishBowlTarget(W, ArrayDishes[Slot]); }
	void Cvar(const TCHAR* Name, int32 Value)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name)) V->Set(Value, ECVF_SetByGameSetting);
	}
	// Diagnostic A/B uses the previous night fill with identical geometry and materials.
	bool PreviousNight() { return FParse::Param(FCommandLine::Get(), TEXT("S47PreviousNight")); }
	float NightSkyIntensity(float Dawn) { return FMath::Lerp(PreviousNight() ? 0.18f : 0.055f, 1.98f, FMath::Clamp(Dawn, 0.f, 1.f)); }
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

void AS47Web::BuildArrayLighting()
{
	for (FArrayLight& P : ArrayLights) if (P.Light.IsValid()) P.Light->DestroyComponent();
	ArrayLights.Reset();
	FS47GlowPoints* G = Glow(TEXT("ext.glow.points"));
	if (!G) return;
	// Exterior.ts's remaining six field posts have real heads but no material-flood slots.
	// Their exported glows supply source positions; each beam covers its nearest reflector.
	constexpr int32 Targets[6] = { 6, 5, 8, 9, 10, 11 }; // S-07, S-06, S-09, S-10, S-11, S-12
	constexpr float Cones[6] = { 38.f, 35.f, 42.f, 43.f, 43.f, 42.f };
	const FTransform Transform = NodeWorldThree(G->Node);
	for (int32 I = 0; I < UE_ARRAY_COUNT(Targets); ++I)
	{
		const int32 Point = I + 5, Dish = Targets[I];
		if (!G->Pos.IsValidIndex(Point) || Dish >= DishCount()) continue;
		const FVector At = Transform.TransformPosition(G->Pos[Point]);
		if (At.Y < -900) continue;
		USpotLightComponent* L = NewObject<USpotLightComponent>(this);
		L->SetupAttachment(GetRootComponent());
		L->SetMobility(EComponentMobility::Movable);
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(0);
		L->SetLightColor(S47Three::Hex(0xff9a45));
		L->SetWorldLocationAndRotation(S47Three::ToUE(At), S47Three::DirToUE(DishBowlTarget(*this, Dish) - At).Rotation());
		L->SetInnerConeAngle(Cones[I] * 0.68f); L->SetOuterConeAngle(Cones[I]);
		L->SetAttenuationRadius(3200.f);
		L->SetSourceRadius(18.f); L->SetSoftSourceRadius(12.f);
		L->SetCastShadows(true); L->SetCastVolumetricShadow(true);
		L->ContactShadowLengthInWS = true; L->ContactShadowLength = 20.f;
		L->ContactShadowCastingIntensity = 0.65f;
		L->SetVolumetricScatteringIntensity(0.6f);
		L->SetIndirectLightingIntensity(1.f);
		L->SetVisibility(false);
		L->RegisterComponent();
		ArrayLights.Add({ Dish, Point, At, L });
	}
	UE_LOG(LogTemp, Display, TEXT("S47: array lighting %d sources fitted to the exported field lamp heads"), ArrayLights.Num());
}

void AS47Web::UpdateArrayLighting()
{
	FS47GlowPoints* G = Glow(TEXT("ext.glow.points"));
	const int32 Site = FloodSet(TEXT("site"));
	if (!G || Site < 0) return;
	const FVector Camera = S47Three::ToThree(Eye);
	const FTransform Transform = NodeWorldThree(G->Node);
	const bool bArea = ObjAt(G->Node).Shown();
	const FLinearColor C = FloodCol(Site, 0);
	const float ColorGain = FMath::Max(1.f, FMath::Max3(C.R, C.G, C.B));
	const float K = FMath::Max(0.f, FloodPos(Site, 0).A / 46.f) * FMath::Max(0.f, FloodScaleOf(Site) / 0.07f);
	// ArrayBack hides the post glows even during its red flashes. The shared red pool state
	// overrides that ordinary off switch, while the cached source stays on the actual post.
	const bool bRed = C.R > C.G * 4.f && C.R > C.B * 8.f;
	for (FArrayLight& P : ArrayLights)
	{
		USpotLightComponent* L = P.Light.Get();
		if (!L || !G->Pos.IsValidIndex(P.Glow) || !G->Col.IsValidIndex(P.Glow)) continue;
		const float Dist = FVector::Distance(Camera, P.At);
		const FLinearColor GlowColor = G->Col[P.Glow];
		const bool bGlow = Transform.TransformPosition(G->Pos[P.Glow]).Y > -900 && FMath::Max3(GlowColor.R, GlowColor.G, GlowColor.B) > 0.0001f;
		const bool bOn = bArea && Dist < 200.f && K > 0.001f && (bGlow || bRed);
		if (L->IsVisible() != bOn) L->SetVisibility(bOn);
		if (!bOn) continue;
		L->SetWorldRotation(S47Three::DirToUE(DishBowlTarget(*this, P.Dish) - P.At).Rotation());
		L->SetLightColor(C / ColorGain);
		L->SetIntensity(480.f * K * ColorGain * (bRed ? 1.f : FMath::Max3(GlowColor.R, GlowColor.G, GlowColor.B)));
		L->SetCastShadows(LightingQuality > 0 || Dist < 37.f);
	}
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
			const bool bArray = F.Key == TEXT("site") && I < UE_ARRAY_COUNT(ArrayDishes) && DishCount() > ArrayDishes[I];
			const bool bDown = (F.Key == TEXT("site") && I < 8) || (F.Key == TEXT("field") && I == 2) || (F.Key == TEXT("court") && I >= 10 && I <= 11) || (F.Key == TEXT("town") && I >= 1 && I <= 7);
			UPointLightComponent* L = bDown ? NewObject<USpotLightComponent>(this) : NewObject<UPointLightComponent>(this);
			if (USpotLightComponent* Spot = Cast<USpotLightComponent>(L))
			{
				Spot->SetInnerConeAngle(bArray ? ArrayCones[I] * 0.68f : 42.f); Spot->SetOuterConeAngle(bArray ? ArrayCones[I] : 78.f);
				Spot->SetWorldRotation(FRotator(-90, 0, 0));
			}
			L->SetupAttachment(GetRootComponent());
			L->SetMobility(EComponentMobility::Movable);
			L->SetIntensityUnits(ELightUnits::Candelas);
			L->SetIntensity(0);
			const bool bCab = F.Key == TEXT("cab"), bDial = F.Key == TEXT("hut") && I == 0;
			L->SetSourceRadius(bArray ? 18.f : bCab || bDial ? 1.5f : F.Key == TEXT("annex") || F.Key == TEXT("diner") ? 12.f : 7.f);
			L->SetSoftSourceRadius(bArray ? 12.f : bCab || bDial ? 1.f : 4.f);
			// The dashboard and valve receiver emit over narrow instrument faces, not a bulb.
			if (bCab || bDial) L->SetSourceLength(bCab ? 24.f : 16.f);
			L->SetCastShadows(true);
			L->SetCastVolumetricShadow(true);
			L->ContactShadowLengthInWS = true; L->ContactShadowLength = bCab || bDial ? 6.f : 16.f;
			L->ContactShadowCastingIntensity = 0.75f;
			L->SetVolumetricScatteringIntensity(bCab || bDial ? 0.f : 0.6f);
			L->SetIndirectLightingIntensity(1.f);
			L->RegisterComponent();
			PoolLights.Add({ S, I, FMath::Max(F.Pos[I].A, 1.f), L });
		}
	}
	BuildFixtureLighting();
	BuildScreenLighting();
	BuildArrayLighting();
	BuildExteriorLighting();
	BuildLabLighting();
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
	NightSky->SetIntensity(NightSkyIntensity(0.f));
	NightSky->SetLightColor(FLinearColor(0.42f, 0.57f, 1.f));
	NightSky->RegisterComponent();
	// Several area moons can be visible together (road/diner). Give them stable distinct
	// priorities; hidden areas remain excluded by Enter(). The active sun takes precedence.
	int32 MoonPriority = 0;
	for (FLightRef& R : Lights) if (UDirectionalLightComponent* Moon = Cast<UDirectionalLightComponent>(R.L.Get()))
		Moon->SetForwardShadingPriority(++MoonPriority);
	DawnSun = NewObject<UDirectionalLightComponent>(this);
	DawnSun->SetupAttachment(GetRootComponent());
	DawnSun->SetMobility(EComponentMobility::Movable);
	DawnSun->SetLightColor(FLinearColor(1.f, 0.55f, 0.28f));
	DawnSun->SetIntensity(0);
	DawnSun->SetForwardShadingPriority(0);
	DawnSun->SetCastShadows(true);
	DawnSun->LightSourceAngle = 0.545f;
	DawnSun->LightSourceSoftAngle = 0.08f;
	DawnSun->ContactShadowLengthInWS = true; DawnSun->ContactShadowLength = 20.f;
	DawnSun->ContactShadowCastingIntensity = 0.65f;
	DawnSun->RegisterComponent();
	SetLightingQuality(TEXT("high"));
	UpdateNativeFog();
	UE_LOG(LogTemp, Display, TEXT("S47: native lighting, %d lamp slots, Lumen (hardware when available), fixed exposure, no material floods or hemisphere"), PoolLights.Num());
	UE_LOG(LogTemp, Display, TEXT("S47: night contrast previous=%d sky=%.3f moon=%.3f indirect=%.2f; direct lamp pools unchanged"), PreviousNight(), NightSkyIntensity(0.f), PreviousNight() ? 0.16f : 0.055f, PreviousNight() ? 1.f : 0.52f);
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
	Cvar(TEXT("r.ContactShadows"), LightingQuality > 0);
	Cvar(TEXT("r.Lumen.ScreenProbeGather.ShortRangeAO"), LightingQuality > 0);
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
		// Lumen owns local occlusion on High/Ultra. Low retains a small, inexpensive SSAO
		// radius so paper, cables and furniture keep their contact with the surface below.
		P.bOverride_LumenAmbientOcclusionIntensity = true; P.LumenAmbientOcclusionIntensity = 0.7f;
		P.bOverride_LumenSkylightLeaking = true; P.LumenSkylightLeaking = 0.f;
		P.bOverride_AmbientOcclusionIntensity = true; P.AmbientOcclusionIntensity = LightingQuality == 0 ? 0.55f : 0.f;
		P.bOverride_AmbientOcclusionRadius = true; P.AmbientOcclusionRadius = 35.f;
		P.bOverride_AmbientOcclusionRadiusInWS = true; P.AmbientOcclusionRadiusInWS = true;
		P.bOverride_AmbientOcclusionPower = true; P.AmbientOcclusionPower = 1.1f;
		P.bOverride_AmbientOcclusionQuality = true; P.AmbientOcclusionQuality = 40.f;
		P.bOverride_ReflectionMethod = true; P.ReflectionMethod = LightingQuality == 0 ? EReflectionMethod::ScreenSpace : EReflectionMethod::Lumen;
		P.bOverride_LumenReflectionQuality = true; P.LumenReflectionQuality = LightingQuality == 2 ? 1.f : 0.5f;
		P.bOverride_LumenReflectionsScreenTraces = true; P.LumenReflectionsScreenTraces = true;
		P.bOverride_LumenMaxReflectionBounces = true; P.LumenMaxReflectionBounces = 1;
		P.bOverride_LumenMaxRoughnessToTraceReflections = true; P.LumenMaxRoughnessToTraceReflections = 0.35f;
		// Surface cache reflections fit the 60 fps profile; hit lighting remains an Ultra option.
		P.bOverride_LumenRayLightingMode = true; P.LumenRayLightingMode = LightingQuality == 2 ? ELumenRayLightingModeOverride::HitLightingForReflections : ELumenRayLightingModeOverride::SurfaceCache;
	}
	UE_LOG(LogTemp, Display, TEXT("S47: lighting quality %s, Lumen %s, volumetrics %s"), *Quality, LightingQuality ? TEXT("on") : TEXT("off"), LightingQuality ? TEXT("on") : TEXT("off"));
}

void AS47Web::SetSkyLighting(float Daylight, const FVector& SunDirection, float Sun, float Flash, float Evening)
{
	SkyDawn = Daylight; SkySun = Sun; SkyFlash = Flash;
	const float Caliche = float(S47CalicheVisualWeight(S47Three::ToThree(Eye)));
	if (DawnSun)
	{
		DawnSun->SetWorldRotation(S47Three::DirToUE(-SunDirection).Rotation());
		// Forward translucency/water/fog get the live sun, or the area's moon at night.
		// Leaving the zero-intensity sun at highest priority would suppress moonlit fog.
		DawnSun->SetForwardShadingPriority(Sun > 0.f ? Lights.Num() + 1 : 0);
		// The existing fixed exposure is intentionally retained. These are calibrated game
		// light values, not a physical 120,000 lux daylight rig; avoid overexposing the cabin.
		DawnSun->SetIntensity(Sun * FMath::Lerp(8.f, 14.f, FMath::Clamp(Evening, 0.f, 1.f)));
		DawnSun->SetLightColor(Evening > 0.f
			? FMath::Lerp(FLinearColor(1.f, 0.36f, 0.14f), FLinearColor(1.f, 0.76f, 0.52f), FMath::Clamp(Evening, 0.f, 1.f))
			: FLinearColor(1.f, 0.55f, 0.28f));
		// The old dawn-only orange sun should not tint a whole noon street orange.
		const float HighSun = Caliche * FMath::Clamp(float((SunDirection.Y - 0.08) / 0.45), 0.f, 1.f);
		DawnSun->SetLightColor(FMath::Lerp(DawnSun->GetLightColor(), FLinearColor(1.f, 0.94f, 0.83f), HighSun));
	}
	if (NightSky)
	{
		const float D = FMath::Clamp(Daylight, 0.f, 1.f);
		// The concept's blue hour still has broad sky illumination after the disk sets.
		// Compensate the fixed-exposure cubemap at twilight; noon and full night keep
		// their endpoint values and all story clocks retain their original curves.
		const float LocalSky = FMath::Lerp(.72f, 1.98f, D) + 3.5f * 4.f * D * (1.f - D);
		NightSky->SetIntensity(FMath::Lerp(NightSkyIntensity(Daylight), LocalSky, Caliche));
		// A permanently blue tint doubles the blue in the captured dawn sky. Desaturate
		// only with the existing dawn curve, preserving the cold low-level night fill.
		const FLinearColor NightTint = FMath::Lerp(FLinearColor(0.42f, 0.57f, 1.f), FLinearColor(0.64f, 0.75f, 1.f), Caliche);
		NightSky->SetLightColor(FMath::Lerp(NightTint, FLinearColor(0.93f, 0.95f, 1.f), FMath::Clamp(Daylight, 0.f, 1.f)));
	}
}

void AS47Web::UpdateNativeFog()
{
	if (!LightingFog) return;
	// FogExp2 is not a physical extinction coefficient. Match its thin night air near eye level,
	// with a long view distance; local lamps and beams now scatter through the actual volume.
	const float Caliche = float(S47CalicheVisualWeight(S47Three::ToThree(Eye)));
	LightingFog->SetFogDensity(FMath::Clamp(FogDensity * 2.f + Caliche * .0012f, 0.0003f, 0.012f));
	LightingFog->SetFogInscatteringColor(FogColor * FMath::Lerp(.12f, .32f, Caliche));
	LightingFog->SetStartDistance(7000.f * Caliche);
	LightingFog->SetVolumetricFogDistance(FMath::Lerp(14000.f, 120000.f, Caliche));
}

void AS47Web::UpdateNativeLighting()
{
	// The clock can keep an identical fog colour for hours. Position-dependent aerial
	// perspective must still follow a drive into/out of Caliche during that interval.
	UpdateNativeFog();
	const FVector Camera = S47Three::ToThree(Eye);
	TArray<FVector> Occupied;
	for (FPoolLight& P : PoolLights)
	{
		UPointLightComponent* L = P.Light.Get();
		if (!L) continue;
		const FFloodSet& F = FloodSets[P.Set];
		const FLinearColor V = F.Pos[P.Slot], C = F.Col[P.Slot];
		const bool bArray = F.Key == TEXT("site") && P.Slot < UE_ARRAY_COUNT(ArrayDishes) && DishCount() > ArrayDishes[P.Slot];
		FVector Pos(V.R, V.G, V.B);
		if (bArray && Pos.Y > -900) Pos.Y = 6.5;
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
		if (bArray) L->SetWorldRotation(S47Three::DirToUE(ArrayBowlTarget(*this, P.Slot) - Pos).Rotation());
		const float ColorGain = FMath::Max(1.f, FMath::Max3(C.R, C.G, C.B));
		L->SetLightColor(C / ColorGain);
		const bool bInside = F.Key == TEXT("annex") || F.Key == TEXT("lab") || F.Key == TEXT("hut") || F.Key == TEXT("cab") || F.Key == TEXT("motel");
		const float Gain = F.Key == TEXT("cab") ? 0.3f : bInside ? 12.f : F.Key == TEXT("diner") ? 18.f : 50.f;
		// Keep the exact story dimming and red pulse state, but calibrate the real field floods
		// for their 12-25 m throw onto the reflector backs rather than a pool beneath the post.
		L->SetIntensity(bArray ? ArrayCandelas[P.Slot] * (V.A / ArrayWidths[P.Slot]) * (F.Scale / 0.07f) * ColorGain : V.A * F.Scale * Gain * ColorGain);
		const float Limit = F.Key == TEXT("cab") ? 2.f : F.Key == TEXT("hut") ? 5.f : bInside ? 7.f : F.Key == TEXT("diner") ? 8.f : 70.f;
		L->SetAttenuationRadius((bArray ? ArrayRadii[P.Slot] : FMath::Min(Limit, P.Reach * 1.6f + 8.f)) * 100.f);
		// A light that reaches the eye retains its occlusion, including walls behind the camera.
		const float Reach = L->AttenuationRadius * 0.01f;
		L->SetCastShadows(LightingQuality > 0 || Dist < Reach + 5.f);
	}
	UpdateFixtureLighting();
	UpdateScreenLighting();
	UpdateArrayLighting();
	UpdateExteriorLighting();
	UpdateLabLighting();
	// Lumen's direct-light surface-cache pass applies each light's IndirectLightingScale.
	// PostProcess.IndirectLightingIntensity only scales precomputed GI in this UE version.
	// Lower bounce at night without dimming the actual lamp, screen or flashlight pools;
	// return to the existing daylight response along the world's dawn curve.
	const float Indirect = PreviousNight() ? 1.f : FMath::Lerp(0.52f, 1.f, FMath::Clamp(SkyDawn, 0.f, 1.f));
	auto Bounce = [Indirect](ULightComponent* Light) { if (Light) Light->SetIndirectLightingIntensity(Indirect); };
	for (FPoolLight& P : PoolLights) Bounce(P.Light.Get());
	for (FFixtureLight& P : FixtureLights) Bounce(P.Light.Get());
	for (FScreenLight& P : ScreenLights) Bounce(P.Light.Get());
	for (FArrayLight& P : ArrayLights) Bounce(P.Light.Get());
	for (FExteriorLight& P : ExteriorLights) Bounce(P.Light.Get());
	for (FLabLight& P : LabLights) Bounce(P.Light.Get());
	if (NightSky) NightSky->SetIndirectLightingIntensity(Indirect);
	Bounce(DawnSun);
	for (FLightRef& R : Lights) if (UDirectionalLightComponent* Moon = Cast<UDirectionalLightComponent>(R.L.Get()))
	{
		// Keep each area's original moon direction; the sun is driven by the living sky above.
		const float LocalMoon = FMath::Lerp(PreviousNight() ? 0.16f : 0.055f, 0.24f, float(S47CalicheVisualWeight(S47Three::ToThree(Eye))));
		Moon->SetIntensity(R.Base > 0 ? LocalMoon * (1.f - SkyDawn) + SkyFlash * 0.8f : 0.f);
		if (!PreviousNight()) Moon->SetLightColor(FLinearColor(0.40f, 0.55f, 1.f));
		Bounce(Moon);
		Moon->SetLightSourceAngle(0.52f);
		Moon->SetLightSourceSoftAngle(0.08f);
	}
	const float CalicheSky = float(S47CalicheVisualWeight(S47Three::ToThree(Eye)));
	const bool bCalicheBoundary = (CalicheSky == 0.f && CapturedCaliche > 0.f) || (CalicheSky == 1.f && CapturedCaliche < 1.f);
	if (NightSky && LightingRunTime > 2 && LightingRunTime - SkyCaptureTime > 2 && (!bSkyCaptured || FMath::Abs(CapturedDawn - SkyDawn) > 0.08f || FMath::Abs(CapturedCaliche - CalicheSky) > 0.08f || bCalicheBoundary))
	{
		NightSky->RecaptureSky(); bSkyCaptured = true; CapturedDawn = SkyDawn; CapturedCaliche = CalicheSky; SkyCaptureTime = LightingRunTime;
		if (FParse::Param(FCommandLine::Get(), TEXT("S47LightLog"))) UE_LOG(LogTemp, Display, TEXT("S47: sky capture daylight=%.4f caliche=%.4f"), SkyDawn, CalicheSky);
	}
}

void AS47Web::SampleLightingPerformance(float Dt)
{
	LightingRunTime += Dt;
	ReviewLighting();
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
