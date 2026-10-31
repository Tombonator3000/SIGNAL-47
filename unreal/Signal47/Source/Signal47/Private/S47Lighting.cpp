#include "S47Web.h"
#include "S47DevEvents.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/Engine.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "DynamicRHI.h"
#include "RHIStats.h"
#include "RenderTimer.h"
#include "S47Three.h"
#include "S47Sky.h"
#include "S47Wind.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

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
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			// User overrides must outrank renderer project settings (notably Lumen HWRT),
			// but still yield to explicit command-line/console experiments. Do not recreate
			// every component for an unchanged HWRT value while moving a different slider.
			if (V->GetInt() != Value || (V->GetFlags() & ECVF_SetByMask) < ECVF_SetByGameOverride)
				V->Set(Value, ECVF_SetByGameOverride);
		}
		else UE_LOG(LogTemp, Warning, TEXT("S47: graphics CVar missing: %s"), Name);
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
		L->SetCastShadows(Graphics.Shadows > 0 && (LightingQuality > 0 || Dist < 37.f));
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
	// Bounded experimental comparison only. Keep the approved atmosphere by default.
	LightingFog->FSSSSceneColorScatteringAmountScale = .07f;
	LightingFog->FSSSSceneColorScatteringAmountPower = 1.5f;
	LightingFog->FSSSSpreadScale = .3f;
	LightingFog->FSSSBlurControl = .15f;
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
	BuildClouds();
	SetLightingQuality(TEXT("high"));
	UpdateNativeFog();
	UE_LOG(LogTemp, Display, TEXT("S47: native lighting, %d lamp slots, Lumen (hardware when available), fixed exposure, no material floods or hemisphere"), PoolLights.Num());
	UE_LOG(LogTemp, Display, TEXT("S47: night contrast previous=%d sky=%.3f moon=%.3f indirect=%.2f; direct lamp pools unchanged"), PreviousNight(), NightSkyIntensity(0.f), PreviousNight() ? 0.16f : 0.055f, PreviousNight() ? 1.f : 0.52f);
}

void AS47Web::SetLightingQuality(const FString& Quality)
{
	ApplyGraphicsSettings(FS47GraphicsSettings::ForPreset(Quality));
}

const FPostProcessSettings* AS47Web::GetGraphicsPostProcess() const
{
	return LightingPost ? &LightingPost->Settings : nullptr;
}

TSharedPtr<FJsonObject> AS47Web::GraphicsReadback() const
{
	auto O = MakeShared<FJsonObject>();
	for (const TCHAR* Name : { TEXT("r.ScreenPercentage"), TEXT("sg.ShadowQuality"), TEXT("sg.AntiAliasingQuality"),
		TEXT("sg.GlobalIlluminationQuality"), TEXT("sg.ReflectionQuality"), TEXT("r.Lumen.HardwareRayTracing"),
		TEXT("r.RayTracing.ResidentGeometryMemoryPoolSizeInMB"),
		TEXT("r.ShadowQuality"), TEXT("r.ContactShadows"), TEXT("r.Lumen.ScreenProbeGather.ShortRangeAO"),
		TEXT("r.VolumetricFog"), TEXT("r.VolumetricFog.GridPixelSize"), TEXT("r.VolumetricFog.GridSizeZ"),
		TEXT("r.Streaming.MipBias"), TEXT("r.MaxAnisotropy"), TEXT("r.Streaming.PoolSize"),
		TEXT("r.Shadow.Virtual.SMRT.RayCountLocal"), TEXT("r.Shadow.Virtual.SMRT.RayCountDirectional"),
		TEXT("r.Shadow.Virtual.SMRT.SamplesPerRayLocal"), TEXT("r.Shadow.Virtual.SMRT.SamplesPerRayDirectional"),
		TEXT("r.Shadow.Virtual.MarkPixelPagesMipModeLocal"), TEXT("r.Shadow.Virtual.ResolutionLodBiasLocal"),
		TEXT("r.SSR.Quality"), TEXT("r.Lumen.Reflections.Allow"), TEXT("r.MaterialQualityLevel"),
		TEXT("r.PSOPrecache.Components"), TEXT("r.PSOPrecache.GlobalShaders") })
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name)) O->SetNumberField(Name, V->GetFloat());
	if (const FPostProcessSettings* P = GetGraphicsPostProcess())
	{
		O->SetNumberField(TEXT("giMethod"), int32(P->DynamicGlobalIlluminationMethod));
		O->SetNumberField(TEXT("reflectionMethod"), int32(P->ReflectionMethod));
		O->SetNumberField(TEXT("lumenSceneQuality"), P->LumenSceneLightingQuality);
		O->SetNumberField(TEXT("lumenGatherQuality"), P->LumenFinalGatherQuality);
		O->SetNumberField(TEXT("lumenReflectionQuality"), P->LumenReflectionQuality);
		O->SetNumberField(TEXT("lumenRayLightingMode"), int32(P->LumenRayLightingMode));
		O->SetNumberField(TEXT("aoIntensity"), P->AmbientOcclusionIntensity);
		O->SetNumberField(TEXT("ssrQuality"), P->ScreenSpaceReflectionQuality);
	}
	O->SetNumberField(TEXT("requestedReflectionTier"), Graphics.Reflections);
	O->SetNumberField(TEXT("resolvedReflectionTier"), Graphics.ResolvedReflections());
	O->SetBoolField(TEXT("reflectionFallback"), Graphics.Reflections != Graphics.ResolvedReflections());
	O->SetNumberField(TEXT("fogTier"), Graphics.Fog);
	O->SetNumberField(TEXT("cloudTier"), Graphics.Clouds);
	O->SetBoolField(TEXT("cloudVisible"), Clouds && Clouds->IsVisible());
	O->SetNumberField(TEXT("cloudSamples"), Clouds ? Clouds->ViewSampleCountScale : 0.f);
	O->SetBoolField(TEXT("skyRealtimeCapture"), NightSky && NightSky->bRealTimeCapture);
	return O;
}

void AS47Web::ApplyGraphicsSettings(const FS47GraphicsSettings& Settings)
{
	Graphics = Settings; Graphics.Normalize();
	LightingQuality = Graphics.Detail; // Existing local-lamp/decorative budgets keep their meaning.
	const int32 Shadow = Graphics.Shadows, GI = Graphics.IndirectLighting, Reflection = Graphics.ResolvedReflections();
	// Broad scalability groups come first: shadows also write volumetric-fog variables.
	// Independent controls below then restore the requested combinations. Effects/material
	// scalability and the project's PSO policy are deliberately never changed here.
	Cvar(TEXT("sg.ShadowQuality"), Shadow);
	Cvar(TEXT("sg.AntiAliasingQuality"), Graphics.AntiAliasing);
	Cvar(TEXT("sg.GlobalIlluminationQuality"), GI + 1);
	Cvar(TEXT("sg.ReflectionQuality"), FMath::Min(Reflection, 2));
	Cvar(TEXT("r.Shadow.Virtual.OnePassProjection.MaxLightsPerPixel"), 32);
	Cvar(TEXT("r.ScreenPercentage"), Graphics.RenderScale);
	Cvar(TEXT("r.Shadow.Virtual.MarkPixelPagesMipModeLocal"), Shadow == 3 ? 0 : 2);
	Cvar(TEXT("r.Shadow.Virtual.SMRT.RayCountLocal"), Shadow == 3 ? 8 : Shadow == 2 ? 4 : 2);
	Cvar(TEXT("r.Shadow.Virtual.SMRT.RayCountDirectional"), Shadow == 3 ? 8 : Shadow == 2 ? 4 : 2);
	Cvar(TEXT("r.Shadow.Virtual.SMRT.SamplesPerRayLocal"), Shadow == 3 ? 4 : 2);
	Cvar(TEXT("r.Shadow.Virtual.SMRT.SamplesPerRayDirectional"), Shadow == 3 ? 4 : 2);
	Cvar(TEXT("r.Shadow.Virtual.ResolutionLodBiasLocal"), Shadow == 3 ? 0 : 1);
	Cvar(TEXT("r.ContactShadows"), Shadow >= 2);
	Cvar(TEXT("r.Lumen.ScreenProbeGather.ShortRangeAO"), GI > 0);
	Cvar(TEXT("r.Lumen.HardwareRayTracing"), Graphics.bHardwareRayTracing ? 1 : 0);
	// UE5.8 RayTracingGeometryManager's real retention pool defaults to 400 MiB.
	// Its unchanged 20% resident warning needs modest headroom for this dense High scene.
	// Use the existing RHI's memory stats (Vulkan and D3D), never an external GPU probe.
	FTextureMemoryStats Memory;
	if (GDynamicRHI) RHIGetTextureMemoryStats(Memory);
	const bool HighHardware = Graphics.Detail >= 1 && GI > 0 && Graphics.bHardwareRayTracing;
	const int32 GeometryPool = HighHardware && Memory.DedicatedVideoMemory >= 8ll * 1024 * 1024 * 1024 ? 640 : 400;
	Cvar(TEXT("r.RayTracing.ResidentGeometryMemoryPoolSizeInMB"), GeometryPool);
	UE_LOG(LogTemp, Display, TEXT("S47: ray geometry retention pool %d MiB, dedicated VRAM %lld MiB, High HWRT %d; warning threshold unchanged"),
		GeometryPool, static_cast<long long>(Memory.DedicatedVideoMemory >= 0 ? Memory.DedicatedVideoMemory / (1024 * 1024) : -1), HighHardware ? 1 : 0);
	Cvar(TEXT("r.VolumetricFog"), Graphics.Fog > 0);
	Cvar(TEXT("r.VolumetricFog.GridPixelSize"), Graphics.Fog == 2 ? 8 : 16);
	Cvar(TEXT("r.VolumetricFog.GridSizeZ"), Graphics.Fog == 2 ? 96 : 64);
	// Avoid the engine's low texture preset (bias16 and a fixed small pool). Keep the
	// current pool policy, allowing modest per-texture reductions under memory pressure.
	static const int32 TextureBias[] = { 2, 1, 0, 0 }, Anisotropy[] = { 2, 4, 4, 8 };
	Cvar(TEXT("r.Streaming.MipBias"), TextureBias[Graphics.Textures]);
	Cvar(TEXT("r.MaxAnisotropy"), Anisotropy[Graphics.Textures]);
	if (LightingPost)
	{
		FPostProcessSettings& P = LightingPost->Settings;
		P.bOverride_DynamicGlobalIlluminationMethod = true;
		P.DynamicGlobalIlluminationMethod = GI == 0 ? EDynamicGlobalIlluminationMethod::None : EDynamicGlobalIlluminationMethod::Lumen;
		P.bOverride_LumenSceneLightingQuality = true; P.LumenSceneLightingQuality = GI == 2 ? 1.f : 0.5f;
		P.bOverride_LumenFinalGatherQuality = true; P.LumenFinalGatherQuality = GI == 2 ? 1.f : 0.5f;
		P.bOverride_LumenAmbientOcclusionIntensity = true; P.LumenAmbientOcclusionIntensity = 0.7f;
		P.bOverride_LumenSkylightLeaking = true; P.LumenSkylightLeaking = 0.f;
		P.bOverride_AmbientOcclusionIntensity = true; P.AmbientOcclusionIntensity = GI == 0 ? 0.55f : 0.f;
		P.bOverride_AmbientOcclusionRadius = true; P.AmbientOcclusionRadius = 35.f;
		P.bOverride_AmbientOcclusionRadiusInWS = true; P.AmbientOcclusionRadiusInWS = true;
		P.bOverride_AmbientOcclusionPower = true; P.AmbientOcclusionPower = 1.1f;
		P.bOverride_AmbientOcclusionQuality = true; P.AmbientOcclusionQuality = 40.f;
		P.bOverride_ReflectionMethod = true;
		P.ReflectionMethod = Reflection == 0 ? EReflectionMethod::None : Reflection == 1 ? EReflectionMethod::ScreenSpace : EReflectionMethod::Lumen;
		P.bOverride_LumenReflectionQuality = true; P.LumenReflectionQuality = Reflection == 3 ? 1.f : 0.5f;
		P.bOverride_LumenReflectionsScreenTraces = true; P.LumenReflectionsScreenTraces = true;
		P.bOverride_LumenMaxReflectionBounces = true; P.LumenMaxReflectionBounces = 1;
		P.bOverride_LumenMaxRoughnessToTraceReflections = true; P.LumenMaxRoughnessToTraceReflections = 0.35f;
		P.bOverride_LumenRayLightingMode = true; P.LumenRayLightingMode = Reflection == 3 ? ELumenRayLightingModeOverride::HitLightingForReflections : ELumenRayLightingModeOverride::SurfaceCache;
	}
	ApplyCloudQuality();
	if (Reflection != Graphics.Reflections)
		UE_LOG(LogTemp, Display, TEXT("S47: graphics reflections requested=%d resolved=screen-space because indirect lighting is off; Lumen preference retained"), Graphics.Reflections);
	UE_LOG(LogTemp, Display, TEXT("S47: lighting quality %s, Lumen %s, volumetrics %s"), *Graphics.LegacyQuality(), GI > 0 ? TEXT("on") : TEXT("off"), Graphics.Fog > 0 ? TEXT("on") : TEXT("off"));
	const auto Readback = GraphicsReadback();
	UE_LOG(LogTemp, Display, TEXT("S47: graphics %s actual scale=%.0f shadows=%.0f GI=%.0f reflection=%.0f HWRT=%.0f fog=%.0f texBias=%.0f anisotropy=%.0f clouds=%d samples=%.2f"),
		*Graphics.PresetId(), Readback->GetNumberField(TEXT("r.ScreenPercentage")), Readback->GetNumberField(TEXT("sg.ShadowQuality")),
		Readback->GetNumberField(TEXT("sg.GlobalIlluminationQuality")), Readback->GetNumberField(TEXT("sg.ReflectionQuality")),
		Readback->GetNumberField(TEXT("r.Lumen.HardwareRayTracing")), Readback->GetNumberField(TEXT("r.VolumetricFog")),
		Readback->GetNumberField(TEXT("r.Streaming.MipBias")), Readback->GetNumberField(TEXT("r.MaxAnisotropy")),
		Readback->GetBoolField(TEXT("cloudVisible")), Readback->GetNumberField(TEXT("cloudSamples")));
}

void AS47Web::SetFinaleGroundShade(float Shade)
{
	FinaleGroundShade = FMath::Clamp(Shade, 0.f, .82f);
	if (FinaleGroundShade <= 0) UpdateFinaleSkyCapture(); // Hide/Drop restores the exact original anchor now.
}

void AS47Web::UpdateFinaleSkyCapture()
{
	if (!NightSky) return;
	if (CurrentArea == TEXT("roswell") && FinaleGroundShade > 0)
	{
		if (!FinaleSkyCaptureRestore.IsSet())
		{
			FinaleSkyCaptureRestore = NightSky->GetComponentLocation();
			NightSky->SetWorldLocation(Eye); bSkyCaptured = false;
		}
		else if (FVector::DistSquared(NightSky->GetComponentLocation(), Eye) >= 25000. * 25000.)
		{
			NightSky->SetWorldLocation(Eye); bSkyCaptured = false;
		}
	}
	else if (FinaleSkyCaptureRestore.IsSet())
	{
		NightSky->SetWorldLocation(*FinaleSkyCaptureRestore);
		FinaleSkyCaptureRestore.Reset(); bSkyCaptured = false;
	}
}

void AS47Web::SetSkyLighting(float Daylight, const FVector& SunDirection, float Sun, float Flash, float Evening)
{
	SkyDawn = Daylight; SkySun = Sun; SkyFlash = Flash; SkyEvening = Evening;
	// Only the active Roswell view loses native ground fill; the living sky keeps its own curves.
	const float FinaleLight = CurrentArea == TEXT("roswell") ? 1.f - FinaleGroundShade : 1.f;
	const float Caliche = float(S47CalicheVisualWeight(HighwayMapEye(S47Three::ToThree(Eye))));
	UpdateCloudLight(SunDirection, Daylight);
	if (DawnSun)
	{
		// SKY.md, rendering only (FS47Sky::Sun keeps its story values): the dusk concept keeps a last
		// grazing light on west-facing cliffs a few minutes past sunset, never from below the ground.
		const float El = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(float(SunDirection.Y), -1.f, 1.f)));
		const float GroundSun = Evening > 0.f && El < -0.33f ? FMath::Max(Sun, 0.22f * FMath::SmoothStep(-2.8f, -0.33f, El)) : Sun;
		FVector GroundDir = SunDirection;
		if (El < 0.6f) GroundDir = FVector(SunDirection.X, 0, SunDirection.Z).GetSafeNormal() * FMath::Cos(FMath::DegreesToRadians(0.6f)) + FVector(0, FMath::Sin(FMath::DegreesToRadians(0.6f)), 0);
		DawnSun->SetWorldRotation(S47Three::DirToUE(-GroundDir).Rotation());
		// Forward translucency/water/fog get the live sun, or the area's moon at night.
		// Leaving the zero-intensity sun at highest priority would suppress moonlit fog.
		DawnSun->SetForwardShadingPriority(GroundSun > 0.f ? Lights.Num() + 1 : 0);
		// The existing fixed exposure is intentionally retained. These are calibrated game
		// light values, not a physical 120,000 lux daylight rig; avoid overexposing the cabin.
		DawnSun->SetIntensity(GroundSun * FMath::Lerp(8.f, 14.f, FMath::Clamp(Evening, 0.f, 1.f)) * FinaleLight);
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
		// SKY.md: the real-time capture now sees the cloud layer, whose dark bases replace bright open
		// sky. Keep the old calibrated ambient on the ground for the same clock; the cover still
		// shapes its direction and colour, and night (few clouds) stays as dark as before.
		static const float Boost = [] { float V = 2.4f; FParse::Value(FCommandLine::Get(), TEXT("S47SkyBoost="), V); return V; }();
		// This compensates visible native cloud bases. The cleared finale needs no
		// extra fill when its own banks replace that weather, regardless of capability.
		const float CloudFill = CloudsActive() ? 1.f + (Boost - 1.f) * FMath::Clamp(CloudCover / 0.55f, 0.f, 1.2f) * FMath::Clamp(Daylight * 4.f, 0.f, 1.f) * CloudFinaleWeatherWeight : 1.f;
		NightSky->SetIntensity(FMath::Lerp(NightSkyIntensity(Daylight), LocalSky, Caliche) * CloudFill * FinaleLight);
		// A permanently blue tint doubles the blue in the captured dawn sky. Desaturate
		// only with the existing dawn curve, preserving the cold low-level night fill.
		const FLinearColor NightTint = FMath::Lerp(FLinearColor(0.42f, 0.57f, 1.f), FLinearColor(0.64f, 0.75f, 1.f), Caliche);
		NightSky->SetLightColor(FMath::Lerp(NightTint, FLinearColor(0.93f, 0.95f, 1.f), FMath::Clamp(Daylight, 0.f, 1.f)));
	}
}

float AS47Web::RefreshFog()
{
	UpdateNativeFog();
	return LightingFog ? LightingFog->FogDensity : -1.f;
}

void AS47Web::UpdateNativeFog()
{
	if (!LightingFog) return;
	const bool bScattering = FParse::Param(FCommandLine::Get(), TEXT("S47FogScattering")) &&
		Graphics.Fog > 0 && CurrentArea == TEXT("saro") && SkyDawn < .65f;
	if (LightingFog->bEnableFSSS != bScattering)
	{
		LightingFog->bEnableFSSS = bScattering;
		LightingFog->MarkRenderStateDirty();
	}
	// FogExp2 is not a physical extinction coefficient. Match its thin night air near eye level,
	// with a long view distance; local lamps and beams now scatter through the actual volume.
	const float Caliche = float(S47CalicheVisualWeight(HighwayMapEye(S47Three::ToThree(Eye))));
	// a developer event's fog (S47DevEvents.h) holds over the night's own until it is set back
	LightingFog->SetFogDensity(GS47DevFogDensity >= 0.f ? GS47DevFogDensity : FMath::Clamp(FogDensity * 2.f + Caliche * .0012f, 0.0003f, 0.012f));
	// SKY.md: aerial perspective of the concept. Through late afternoon and dusk the far mesas go
	// dusty blue-violet, with a warm glow in the haze toward the low (or just set) sun.
	const float D = FMath::Clamp(SkyDawn, 0.f, 1.f);
	const float Dusk = FMath::Clamp(SkyEvening * 3.f, 0.f, 1.f) * FMath::Clamp(D * 3.f, 0.f, 1.f);
	const FLinearColor Blue = FLinearColor(.16f, .19f, .30f) * FMath::Lerp(.25f, 1.f, D);
	LightingFog->SetFogInscatteringColor(FMath::Lerp(FogColor, Blue, Dusk * .75f) * FMath::Lerp(.12f, .32f, Caliche));
	// A developer fog is a grey night mist, lit enough to be seen (the night's own inscatter is near black).
	if (GS47DevFogDensity >= 0.f) LightingFog->SetFogInscatteringColor(FLinearColor(.045f, .05f, .06f));
	LightingFog->SetDirectionalInscatteringExponent(6.f);
	LightingFog->SetDirectionalInscatteringStartDistance(3000.f);
	LightingFog->SetDirectionalInscatteringColor(FLinearColor(.55f, .26f, .10f) * (Dusk * FMath::Max(SkyGlow, FMath::Clamp(SkySun, 0.f, 1.f)) * FMath::Lerp(.15f, .45f, Caliche)));
	LightingFog->SetStartDistance(7000.f * Caliche);
	LightingFog->SetVolumetricFogDistance(FMath::Lerp(14000.f, 120000.f, Caliche));
}

void AS47Web::SyncFloodLights()
{
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
		L->SetCastShadows(Graphics.Shadows > 0 && (LightingQuality > 0 || Dist < Reach + 5.f));
	}
}

void AS47Web::UpdateNativeLighting()
{
	// The clock can keep an identical fog colour for hours. Position-dependent aerial
	// perspective must still follow a drive into/out of Caliche during that interval.
	UpdateNativeFog();
	SyncFloodLights();
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
	const float FinaleLight = CurrentArea == TEXT("roswell") ? 1.f - FinaleGroundShade : 1.f;
	for (FLightRef& R : Lights) if (UDirectionalLightComponent* Moon = Cast<UDirectionalLightComponent>(R.L.Get()))
	{
		// Keep each area's original moon direction; the sun is driven by the living sky above.
		const float LocalMoon = FMath::Lerp(PreviousNight() ? 0.16f : 0.055f, 0.24f, float(S47CalicheVisualWeight(HighwayMapEye(S47Three::ToThree(Eye)))));
		Moon->SetIntensity(R.Base > 0 ? (LocalMoon * (1.f - SkyDawn) + SkyFlash * 0.8f) * FinaleLight : 0.f);
		if (!PreviousNight()) Moon->SetLightColor(FLinearColor(0.40f, 0.55f, 1.f));
		Bounce(Moon);
		Moon->SetLightSourceAngle(0.52f);
		Moon->SetLightSourceSoftAngle(0.08f);
	}
	// Keep nearby C6 geometry inside the existing 15km exclusion. Recapture remains throttled below.
	UpdateFinaleSkyCapture();
	const float CalicheSky = float(S47CalicheVisualWeight(HighwayMapEye(S47Three::ToThree(Eye))));
	const bool bCalicheBoundary = (CalicheSky == 0.f && CapturedCaliche > 0.f) || (CalicheSky == 1.f && CapturedCaliche < 1.f);
	if (NightSky && !NightSky->bRealTimeCapture && LightingRunTime > 2 && LightingRunTime - SkyCaptureTime > 2 && (!bSkyCaptured || FMath::Abs(CapturedDawn - SkyDawn) > 0.08f || FMath::Abs(CapturedCaliche - CalicheSky) > 0.08f || bCalicheBoundary))
	{
		NightSky->RecaptureSky(); bSkyCaptured = true; CapturedDawn = SkyDawn; CapturedCaliche = CalicheSky; SkyCaptureTime = LightingRunTime;
		if (FParse::Param(FCommandLine::Get(), TEXT("S47LightLog"))) UE_LOG(LogTemp, Display, TEXT("S47: sky capture daylight=%.4f caliche=%.4f"), SkyDawn, CalicheSky);
	}
}

void AS47Web::SampleLightingPerformance(float Dt)
{
	LightingRunTime += Dt;
	ReviewLighting();
	FString AtmosphereDirectory;
	if (AtmosphereReviewShot < 2 && LightingRunTime > 8 + AtmosphereReviewShot * 2 &&
		FParse::Param(FCommandLine::Get(), TEXT("S47SmokeTest")) &&
		FParse::Value(FCommandLine::Get(), TEXT("S47AtmosphereReview="), AtmosphereDirectory))
	{
		IFileManager::Get().MakeDirectory(*AtmosphereDirectory, true);
		FScreenshotRequest::RequestScreenshot(FPaths::Combine(AtmosphereDirectory,
			FString::Printf(TEXT("wind-%d.png"), ++AtmosphereReviewShot)), false, false);
	}
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
	if (FParse::Param(FCommandLine::Get(), TEXT("S47AtmosphereLog")))
	{
		float WindTime = 0, WindAmp = 0;
		UMaterialParameterCollectionInstance* Collection = Env ? GetWorld()->GetParameterCollectionInstance(Env) : nullptr;
		const bool bWind = Collection && Collection->GetScalarParameterValue(TEXT("WindTime"), WindTime) &&
			Collection->GetScalarParameterValue(TEXT("WindAmp"), WindAmp);
		UE_LOG(LogTemp, Display, TEXT("S47: atmosphere MPCwind=%d time=%.3f amplitude_m=%.5f driver_m=%.5f FSSS=%d area=%s"),
			bWind, WindTime, WindAmp, S47Wind::Wind().Strength, LightingFog && LightingFog->bEnableFSSS, *CurrentArea);
	}
	auto Mean = [](const TArray<float>& V) { double Sum = 0; for (float X : V) Sum += X; return Sum / V.Num(); };
	TArray<float> Ordered = PerfFrames; Ordered.Sort();
	UE_LOG(LogTemp, Display, TEXT("S47: lightperf t=%.2f n=%d frame=%.3f p95=%.3f game=%.3f draw=%.3f gpu=%.3f ms quality=%d"), LightingRunTime, PerfFrames.Num(), Mean(PerfFrames), Ordered[FMath::Min(Ordered.Num() - 1, int32(Ordered.Num() * 0.95))], Mean(PerfGame), Mean(PerfDraw), Mean(PerfGpu), LightingQuality);
}
