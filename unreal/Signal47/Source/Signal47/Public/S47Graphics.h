#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Desktop graphics preferences live in the settings store, never in a story checkpoint.
// Each field is independently editable; legacy presets migrate without changing their look.
struct FS47GraphicsSettings
{
	int32 RenderScale = 60;          // 50..100 percent; interface remains at output resolution
	int32 Shadows = 2;               // 0 off, 1 low, 2 high, 3 ultra
	int32 IndirectLighting = 1;      // 0 off, 1 high, 2 ultra
	int32 Reflections = 2;           // 0 off, 1 screen space, 2 Lumen, 3 finer Lumen
	int32 Textures = 3;              // 0..3: streamed mip allowance and filtering, not a fixed VRAM pool
	int32 AntiAliasing = 2;          // 1..3, TSR quality; preserve temporal upsampling
	int32 Fog = 1;                   // 0 no volumetric fog, 1 standard, 2 high; distance haze remains
	int32 Clouds = 1;                // 0 photographic sky, 1 normal volumes, 2 finer volumes
	int32 Effects = 1;               // 0 reduced, 1 full; story triggers remain intact
	int32 Detail = 1;                // 0 reduced, 1 normal, 2 dense decorative detail
	bool bHardwareRayTracing = true; // prefer hardware for Lumen when supported; never toggles r.RayTracing

	static FS47GraphicsSettings ForPreset(const FString& Id);
	static FS47GraphicsSettings FromJson(const TSharedPtr<FJsonObject>& Object, const FString& LegacyQuality);
	TSharedPtr<FJsonObject> ToJson() const;
	void Normalize();
	FString PresetId() const;
	FString LegacyQuality() const;
	int32 ResolvedReflections() const; // GI off uses SSR; preserve the selected Lumen tier for GI on
	bool operator==(const FS47GraphicsSettings& Other) const;
	bool operator!=(const FS47GraphicsSettings& Other) const { return !(*this == Other); }
	static bool SelfTest(TArray<FString>& Out);
};
