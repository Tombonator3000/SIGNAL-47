#include "S47Web.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialRenderProxy.h"
#include "MaterialShared.h"
#include "SceneInterface.h"
#include "RenderingThread.h"
#include <atomic>
#include "Misc/CommandLine.h"
#include "S47Three.h"

// SKY.md: one VolumetricCloud layer over every outdoor area, without a SkyAtmosphere. The living
// sky (S47Sky, the painted dome at 30 km) stays the sky; the clouds are composited in front of it.
// Coverage comes from the story clock (FS47Sky::SetClock), the cloud-only light gives the set sun's
// warm undersides after sunset and the moon's silver edges at night, and DawnSun (atmosphere light 0)
// lights them by day and casts their shadows on the desert.
namespace
{
	constexpr float LayerBottomKm = 2.9f, LayerHeightKm = 1.6f, PlanetKm = 6360.f;
	bool CloudsOff() { return FParse::Param(FCommandLine::Get(), TEXT("S47NoClouds")); }
	// Review-only tuning (-S47CloudExt=, -S47CloudGlow=, -S47CloudGlowEl=, -S47CloudAmb=, -S47CloudCover=).
	float Tune(const TCHAR* Key, float Default) { float V = Default; FParse::Value(FCommandLine::Get(), Key, V); return V; }
	FLinearColor Dir4(const FVector& V, double W) { return FLinearColor(float(V.X), float(V.Y), float(V.Z), float(W)); }
}

bool AS47Web::CloudsActive() const
{
	if (!Clouds || !CloudMid || LightingQuality == 0 || CloudsOff()) return false;
#if !WITH_EDITOR
	return true; // cooked shader maps are complete
#else
	// Editor -game: on-demand shader maps can still miss a cloud pass and the renderer then
	// asserts on the surface fallback. Clouds are a packaged-build feature unless asked for.
	static const bool bEditorClouds = FParse::Param(FCommandLine::Get(), TEXT("S47EditorClouds"));
	if (!bEditorClouds) return false;
	// The editor compiles shaders on demand; a volume material must never fall back to the
	// default (surface) material inside the cloud renderer. Cooked builds are ready at once.
	static uint64 ReadyFrom = 0; // frame of the last render-thread query
	static bool bReady = false, bReported = false;
	if (bReady) return true;
	UMaterial* Base = CloudMid->GetMaterial();
	if (!Base || !GetWorld()) return false;
	const EShaderPlatform Platform = GetWorld()->Scene ? GetWorld()->Scene->GetShaderPlatform() : GMaxRHIShaderPlatform;
	const FMaterialResource* R = Base->GetMaterialResource(Platform);
	static std::atomic<int32> RtState{ -1 };
	if (GFrameCounter % 120 == 0) UE_LOG(LogTemp, Display, TEXT("S47: cloud material wait: resource %d map %d complete %d compiling %d errors %d rt %d"),
		R != nullptr, R && R->GetGameThreadShaderMap(), R && R->GetGameThreadShaderMap() && R->IsGameThreadShaderMapComplete(), Base->IsCompilingOrHadCompileError(Platform),
		R ? R->GetCompileErrors().Num() : -1, RtState.load());
	if (R && R->GetGameThreadShaderMap() && R->IsGameThreadShaderMapComplete() && !Base->IsCompilingOrHadCompileError(Platform) && GFrameCounter > 600)
	{
		// The render thread receives the shader map later than the game thread: ask it directly.
		static std::atomic<int32> RenderReady{ 0 };
		if (RenderReady.load() > 0 && GFrameCounter > ReadyFrom + 10)
		{
			bReady = true;
			UE_LOG(LogTemp, Display, TEXT("S47: cloud material ready on the render thread, frame %llu"), static_cast<unsigned long long>(GFrameCounter));
			return true;
		}
		if (RenderReady.load() == 0 && GFrameCounter > ReadyFrom + 2)
		{
			ReadyFrom = GFrameCounter;
			const FMaterialRenderProxy* Proxy = CloudMid->GetRenderProxy();
			const ERHIFeatureLevel::Type Level = GetWorld()->GetFeatureLevel();
			ENQUEUE_RENDER_COMMAND(S47CloudReady)([Proxy, Level](FRHICommandListImmediate&)
			{
				const FMaterial* Material = Proxy ? Proxy->GetMaterialNoFallback(Level) : nullptr;
				RtState.store(!Material ? 0 : Material->GetMaterialDomain() != MD_Volume ? 1 : !Material->GetRenderingThreadShaderMap() ? 2 : 3);
				if (Material && Material->GetMaterialDomain() == MD_Volume && Material->GetRenderingThreadShaderMap()) RenderReady.store(1);
			});
		}
		return false;
	}
	// An editor shader map loaded from the DDC can lack the cloud passes; compile them once.
	static bool bRequested = false;
	if (!bRequested && R && !Base->IsCompilingOrHadCompileError(Platform)) { bRequested = true; Base->CacheShaders(EMaterialShaderPrecompileMode::Background); }
	if (!bReported && R && R->IsCompilationFinished() && R->GetCompileErrors().Num())
	{
		bReported = true;
		for (const FString& Error : R->GetCompileErrors()) UE_LOG(LogTemp, Error, TEXT("S47: cloud material: %s"), *Error);
	}
	return false;
#endif
}

void AS47Web::BuildClouds()
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Sky/M_S47_Clouds.M_S47_Clouds"));
	if (!Base)
	{
		UE_LOG(LogTemp, Warning, TEXT("S47: cloud material missing (Tools/sky/import.sh); painted sky clouds remain"));
		return;
	}
	Clouds = NewObject<UVolumetricCloudComponent>(this);
	Clouds->SetupAttachment(GetRootComponent());
	Clouds->SetLayerBottomAltitude(LayerBottomKm);
	Clouds->SetLayerHeight(LayerHeightKm);
	Clouds->SetPlanetRadius(PlanetKm);
	Clouds->SetTracingStartMaxDistance(350.f);
	// The dome is 30 km out; nothing behind it is visible, so no ray needs to go much further.
	Clouds->SetTracingMaxDistance(36.f);
	Clouds->SetGroundAlbedo(FColor(156, 128, 104)); // caliche and dry grass bounce under the bases
	Clouds->SetSkyLightCloudBottomOcclusion(0.3f);
	Clouds->SetStopTracingTransmittanceThreshold(0.01f);
	CloudMid = UMaterialInstanceDynamic::Create(Base, this);
	Clouds->SetMaterial(CloudMid);
	Clouds->RegisterComponent();

	// Atmosphere light 1 lights only the clouds: no lighting channel, no shadow, no bounce or fog.
	CloudLight = NewObject<UDirectionalLightComponent>(this);
	CloudLight->SetupAttachment(GetRootComponent());
	CloudLight->SetMobility(EComponentMobility::Movable);
	CloudLight->LightingChannels.bChannel0 = false;
	CloudLight->LightingChannels.bChannel1 = false;
	CloudLight->LightingChannels.bChannel2 = false;
	CloudLight->SetCastShadows(false);
	CloudLight->SetVolumetricScatteringIntensity(0.f);
	CloudLight->SetIndirectLightingIntensity(0.f);
	CloudLight->SetAffectTranslucentLighting(false);
	CloudLight->SetSpecularScale(0.f);
	CloudLight->SetForwardShadingPriority(-1);
	CloudLight->SetAtmosphereSunLight(true);
	CloudLight->SetAtmosphereSunLightIndex(1);
	CloudLight->bCastCloudShadows = false;
	CloudLight->bCastShadowsOnClouds = false;
	CloudLight->SetIntensity(0.f);
	CloudLight->RegisterComponent();

	if (DawnSun)
	{
		DawnSun->SetAtmosphereSunLight(true);
		DawnSun->SetAtmosphereSunLightIndex(0);
		DawnSun->bCastCloudShadows = true;
		DawnSun->CloudShadowStrength = 0.85f;
		DawnSun->CloudShadowOnSurfaceStrength = 1.f;
		DawnSun->CloudShadowExtent = 40.f;
		DawnSun->CloudShadowMapResolutionScale = 1.5f;
		DawnSun->MarkRenderStateDirty();
	}
	UE_LOG(LogTemp, Display, TEXT("S47: volumetric clouds %.1f-%.1f km, cloud-only light index 1, sun casts cloud shadows"), LayerBottomKm, LayerBottomKm + LayerHeightKm);
}

void AS47Web::ApplyCloudQuality()
{
	if (!Clouds) return;
	const bool bUltra = LightingQuality == 2;
	// High keeps the 60 fps profile: half the view/shadow samples, short shadow rays, cheap reflections.
	static const float Samples = Tune(TEXT("S47CloudSamples="), 0.f);
	Clouds->SetViewSampleCountScale(Samples > 0.f ? Samples : bUltra ? 1.f : 0.5f);
	Clouds->SetShadowViewSampleCountScale(Samples > 0.f ? Samples : bUltra ? 1.f : 0.5f);
	Clouds->SetReflectionViewSampleCountScale(bUltra ? 0.3f : 0.15f);
	Clouds->SetShadowReflectionViewSampleCountScale(0.15f);
	// Shadow rays only need to cross one cloud: long ones put their first step past the cloud and
	// light from below (the set sun) turned into speckle.
	static const float ShadowKm = Tune(TEXT("S47CloudShadowKm="), 0.f);
	Clouds->SetShadowTracingDistance(ShadowKm > 0.f ? ShadowKm : bUltra ? 2.f : 1.2f);
	Clouds->SetVisibility(CloudsActive());
	if (DawnSun && DawnSun->bCastCloudShadows != (LightingQuality > 0))
	{
		DawnSun->bCastCloudShadows = LightingQuality > 0;
		DawnSun->MarkRenderStateDirty();
	}
	// The ambient follows the clouds: real-time capture of the sky dome and the cloud layer.
	if (NightSky && NightSky->bRealTimeCapture != CloudsActive())
	{
		NightSky->SetRealTimeCapture(CloudsActive());
		bSkyCaptured = false;
	}
	UE_LOG(LogTemp, Display, TEXT("S47: clouds %s, view samples %.2f, sky capture %s"), CloudsActive() ? TEXT("on") : TEXT("off"),
		Clouds->ViewSampleCountScale, NightSky && NightSky->bRealTimeCapture ? TEXT("real-time") : TEXT("on change"));
}

void AS47Web::SetClouds(float Cover, float Cirrus, const FVector4& Clear0, const FVector4& Clear1, float Flash, const FVector& FlashDir, double Time)
{
	CloudCover = Cover; CloudCirrus = Cirrus;
	if (!Clouds || !CloudMid) return;
	const bool bOn = CloudsActive();
	if (Clouds->IsVisible() != bOn) Clouds->SetVisibility(bOn);
	if (NightSky && NightSky->bRealTimeCapture != bOn) { NightSky->SetRealTimeCapture(bOn); bSkyCaptured = false; }
	const double Dt = CloudTime < 0 ? 0.0 : FMath::Clamp(Time - CloudTime, 0.0, 0.5);
	CloudTime = Time;
	// About 9 m/s at altitude from the west-southwest (UE +X east, +Y south), and a slow change of shape.
	CloudWind.X -= 0.0085 * Dt;
	CloudWind.Y -= -0.0032 * Dt;
	CloudWind.Z += 0.0007 * Dt;
	if (!bOn) return;
	static const float CoverOverride = Tune(TEXT("S47CloudCover="), -1.f), Ext = Tune(TEXT("S47CloudExt="), 0.010f), AmbScale = Tune(TEXT("S47CloudAmb="), .55f);
	if (CoverOverride >= 0.f) Cover = CoverOverride;
	CloudMid->SetScalarParameterValue(TEXT("Ext"), Ext);
	// The sky's light on the clouds: cool blue by day, dusty violet-grey through dusk, nearly
	// nothing at night (the moon's edge light is the cloud light's job).
	const float D = FMath::Clamp(SkyDawn, 0.f, 1.f);
	FLinearColor Amb = FMath::Lerp(FLinearColor(.0035f, .0045f, .008f), FLinearColor(.40f, .45f, .56f), FMath::Pow(D, 1.4f));
	const float Dusk = FMath::Clamp(SkyEvening * 2.f, 0.f, 1.f) * (1.f - D) * 4.f * D;
	Amb *= FMath::Lerp(FLinearColor::White, FLinearColor(.95f, .78f, 1.05f), FMath::Clamp(Dusk, 0.f, 1.f));
	CloudMid->SetVectorParameterValue(TEXT("Ambient"), Amb * AmbScale);
	CloudMid->SetScalarParameterValue(TEXT("Cover"), Cover);
	CloudMid->SetScalarParameterValue(TEXT("Cirrus"), Cirrus);
	CloudMid->SetVectorParameterValue(TEXT("Wind"), FLinearColor(float(CloudWind.X), float(CloudWind.Y), float(CloudWind.Z), 0.f));
	CloudMid->SetVectorParameterValue(TEXT("Clear0"), Dir4(S47Three::DirToUE(FVector(Clear0.X, Clear0.Y, Clear0.Z)).GetSafeNormal(), Clear0.W));
	CloudMid->SetVectorParameterValue(TEXT("Clear1"), Dir4(S47Three::DirToUE(FVector(Clear1.X, Clear1.Y, Clear1.Z)).GetSafeNormal(), Clear1.W));
	CloudMid->SetScalarParameterValue(TEXT("Flash"), Flash);
	CloudMid->SetVectorParameterValue(TEXT("FlashDir"), Dir4(S47Three::DirToUE(FlashDir).GetSafeNormal(), 0));
	// No SkyAtmosphere: the cloud planet's top is the world origin. The story's areas lie up to
	// 80 km from it, where that sphere has dropped by d^2/2R; lift the layer back over the eye.
	const double Km = Eye.Size2D() / 100000.0;
	const float Bottom = LayerBottomKm + float(Km * Km / (2.0 * PlanetKm));
	if (FMath::Abs(Clouds->LayerBottomAltitude - Bottom) > 0.03f) Clouds->SetLayerBottomAltitude(Bottom);
}

void AS47Web::UpdateCloudLight(const FVector& SunDirection, float Daylight)
{
	if (!CloudLight) return;
	const float El = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(float(SunDirection.Y), -1.f, 1.f)));
	// The set (or rising) sun keeps lighting the cloud bases from below the horizon: warm orange
	// just after sunset, deep red toward the end of civil twilight, gone by about -8 degrees.
	SkyGlow = FMath::SmoothStep(-8.f, -0.8f, El) * (1.f - FMath::SmoothStep(-0.4f, 1.4f, El));
	const FLinearColor GlowColor = FMath::Lerp(FLinearColor(1.f, .20f, .07f), FLinearColor(1.f, .42f, .15f), FMath::SmoothStep(-6.f, -1.f, El));
	FVector MoonDir = FVector::ZeroVector;
	for (const FLightRef& R : Lights) if (const UDirectionalLightComponent* Moon = Cast<UDirectionalLightComponent>(R.L.Get()))
		if (R.Base > 0 && (MoonDir.IsZero() || Moon->IsVisible())) { MoonDir = Moon->GetForwardVector(); if (Moon->IsVisible()) break; }
	const float Night = 1.f - FMath::Clamp(Daylight / 0.2f, 0.f, 1.f);
	const float MoonI = MoonDir.IsZero() ? 0.f : 0.3f * Night;
	static const float GlowScale = Tune(TEXT("S47CloudGlow="), 1.8f), GlowEl = Tune(TEXT("S47CloudGlowEl="), 1.5f);
	const float GlowI = 5.f * SkyGlow * GlowScale;
	const float Total = GlowI + MoonI;
	if (!CloudsActive() || Total < 1e-4f)
	{
		if (CloudLight->Intensity != 0.f) CloudLight->SetIntensity(0.f);
		return;
	}
	// Art direction: the glow comes from just above the horizon at the sun's azimuth. A direction
	// under the cloud planet's horizon is shadowed by the planet per sample and turns into speckle
	// (no SkyAtmosphere here to soften it); level light gives the warm rims and lit edges instead.
	const FVector Flat = FVector(SunDirection.X, 0, SunDirection.Z).GetSafeNormal();
	const float Below = FMath::DegreesToRadians(GlowEl);
	const FVector GlowDir = S47Three::DirToUE(-(Flat * FMath::Cos(Below) + FVector(0, FMath::Sin(Below), 0))).GetSafeNormal();
	CloudLight->SetWorldRotation((GlowDir * GlowI + MoonDir * MoonI).GetSafeNormal().Rotation());
	CloudLight->SetLightColor((GlowColor * GlowI + FLinearColor(.62f, .72f, 1.f) * MoonI) / Total);
	CloudLight->SetIntensity(Total);
}
