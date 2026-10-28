#include "S47Graphics.h"
#include "S47Json.h"

namespace
{
	// Reject wrong JSON types, non-finite/fractional values, and values outside the documented
	// domain. A broken field falls back independently to the legacy preset, not an unsafe cast.
	int32 ReadInt(const TSharedPtr<FJsonObject>& O, const TCHAR* Key, int32 Default, int32 Min, int32 Max)
	{
		double Value = 0;
		const TSharedPtr<FJsonValue> Field = O.IsValid() ? O->TryGetField(Key) : nullptr;
		if (!Field.IsValid() || Field->Type != EJson::Number || !Field->TryGetNumber(Value) || !FMath::IsFinite(Value) ||
			Value < Min || Value > Max || FMath::FloorToDouble(Value) != Value) return Default;
		return int32(Value);
	}
}

FS47GraphicsSettings FS47GraphicsSettings::ForPreset(const FString& Value)
{
	FS47GraphicsSettings S;
	const FString Id = Value.TrimStartAndEnd().ToLower();
	if (Id == TEXT("low") || Id == TEXT("verylow"))
	{
		S.Shadows = 1; S.IndirectLighting = 0; S.Reflections = 1; S.AntiAliasing = 1;
		S.Fog = 0; S.Clouds = 0; S.Effects = 0; S.Detail = 0;
		if (Id == TEXT("verylow"))
		{
			S.RenderScale = 50; S.Shadows = 0; S.Reflections = 0; S.Textures = 0; S.bHardwareRayTracing = false;
		}
	}
	else if (Id == TEXT("medium"))
	{
		// Keep image resolution while reducing indirect lighting and texture pressure first.
		S.IndirectLighting = 0; S.Reflections = 1; S.Textures = 1; S.Effects = 0; S.bHardwareRayTracing = false;
	}
	else if (Id == TEXT("ultra"))
	{
		S.RenderScale = 100; S.Shadows = 3; S.IndirectLighting = 2; S.Reflections = 3;
		S.AntiAliasing = 3; S.Fog = 2; S.Clouds = 2; S.Detail = 2;
	}
	return S;
}

void FS47GraphicsSettings::Normalize()
{
	RenderScale = FMath::Clamp(RenderScale, 50, 100);
	Shadows = FMath::Clamp(Shadows, 0, 3); IndirectLighting = FMath::Clamp(IndirectLighting, 0, 2);
	Reflections = FMath::Clamp(Reflections, 0, 3); Textures = FMath::Clamp(Textures, 0, 3);
	AntiAliasing = FMath::Clamp(AntiAliasing, 1, 3); Fog = FMath::Clamp(Fog, 0, 2);
	Clouds = FMath::Clamp(Clouds, 0, 2); Effects = FMath::Clamp(Effects, 0, 1); Detail = FMath::Clamp(Detail, 0, 2);
}

FS47GraphicsSettings FS47GraphicsSettings::FromJson(const TSharedPtr<FJsonObject>& O, const FString& LegacyQuality)
{
	FS47GraphicsSettings S = ForPreset(LegacyQuality);
	if (!O.IsValid() || ReadInt(O, TEXT("version"), -1, 1, 1) != 1) return S;
	S.RenderScale = ReadInt(O, TEXT("renderScale"), S.RenderScale, 50, 100);
	S.Shadows = ReadInt(O, TEXT("shadows"), S.Shadows, 0, 3);
	S.IndirectLighting = ReadInt(O, TEXT("indirectLighting"), S.IndirectLighting, 0, 2);
	S.Reflections = ReadInt(O, TEXT("reflections"), S.Reflections, 0, 3);
	S.Textures = ReadInt(O, TEXT("textures"), S.Textures, 0, 3);
	S.AntiAliasing = ReadInt(O, TEXT("antiAliasing"), S.AntiAliasing, 1, 3);
	S.Fog = ReadInt(O, TEXT("fog"), S.Fog, 0, 2);
	S.Clouds = ReadInt(O, TEXT("clouds"), S.Clouds, 0, 2);
	S.Effects = ReadInt(O, TEXT("effects"), S.Effects, 0, 1);
	S.Detail = ReadInt(O, TEXT("detail"), S.Detail, 0, 2);
	bool Hardware = S.bHardwareRayTracing;
	const TSharedPtr<FJsonValue> HardwareField = O->TryGetField(TEXT("hardwareRayTracing"));
	if (HardwareField.IsValid() && HardwareField->Type == EJson::Boolean && HardwareField->TryGetBool(Hardware)) S.bHardwareRayTracing = Hardware;
	return S;
}

TSharedPtr<FJsonObject> FS47GraphicsSettings::ToJson() const
{
	FS47GraphicsSettings S = *this; S.Normalize();
	auto O = S47J::New();
	S47J::Set(O, TEXT("version"), 1);
	S47J::Set(O, TEXT("renderScale"), S.RenderScale); S47J::Set(O, TEXT("shadows"), S.Shadows);
	S47J::Set(O, TEXT("indirectLighting"), S.IndirectLighting); S47J::Set(O, TEXT("reflections"), S.Reflections);
	S47J::Set(O, TEXT("textures"), S.Textures); S47J::Set(O, TEXT("antiAliasing"), S.AntiAliasing);
	S47J::Set(O, TEXT("fog"), S.Fog); S47J::Set(O, TEXT("clouds"), S.Clouds);
	S47J::Set(O, TEXT("effects"), S.Effects); S47J::Set(O, TEXT("detail"), S.Detail);
	S47J::Set(O, TEXT("hardwareRayTracing"), S.bHardwareRayTracing);
	return O;
}

bool FS47GraphicsSettings::operator==(const FS47GraphicsSettings& O) const
{
	return RenderScale == O.RenderScale && Shadows == O.Shadows && IndirectLighting == O.IndirectLighting &&
		Reflections == O.Reflections && Textures == O.Textures && AntiAliasing == O.AntiAliasing && Fog == O.Fog &&
		Clouds == O.Clouds && Effects == O.Effects && Detail == O.Detail && bHardwareRayTracing == O.bHardwareRayTracing;
}

FString FS47GraphicsSettings::PresetId() const
{
	for (const TCHAR* Id : { TEXT("verylow"), TEXT("low"), TEXT("medium"), TEXT("high"), TEXT("ultra") })
		if (*this == ForPreset(Id)) return Id;
	return TEXT("custom");
}

FString FS47GraphicsSettings::LegacyQuality() const
{
	return Detail == 0 ? TEXT("low") : Detail == 2 ? TEXT("ultra") : TEXT("high");
}

int32 FS47GraphicsSettings::ResolvedReflections() const
{
	return IndirectLighting == 0 && Reflections >= 2 ? 1 : Reflections;
}

bool FS47GraphicsSettings::SelfTest(TArray<FString>& Out)
{
	bool Ok = true;
	auto Check = [&](bool Pass, const TCHAR* Name) { Ok &= Pass; Out.Add(FString(Pass ? TEXT("ok  graphics: ") : TEXT("FAIL graphics: ")) + Name); };
	for (const TCHAR* Id : { TEXT("verylow"), TEXT("low"), TEXT("medium"), TEXT("high"), TEXT("ultra") })
	{
		const FS47GraphicsSettings S = ForPreset(Id);
		Check(S.PresetId() == Id && FromJson(S47J::Read(S47J::Write(S.ToJson())), TEXT("high")) == S, *FString::Printf(TEXT("%s actual JSON serialization round trip and preset identity"), Id));
	}
	const auto Low = ForPreset(TEXT("low")), High = ForPreset(TEXT("high")), Ultra = ForPreset(TEXT("ultra")), VeryLow = ForPreset(TEXT("verylow"));
	Check(Low.RenderScale == 60 && Low.Shadows == 1 && Low.IndirectLighting == 0 && Low.Reflections == 1 && Low.Textures == 3 && Low.Fog == 0 && Low.Clouds == 0, TEXT("legacy Low renderer settings retained"));
	Check(High == FS47GraphicsSettings() && Ultra.RenderScale == 100 && Ultra.Shadows == 3 && Ultra.IndirectLighting == 2 && Ultra.Reflections == 3 && Ultra.Textures == 3, TEXT("legacy High and Ultra renderer settings retained"));
	Check(VeryLow.RenderScale == 50 && VeryLow.Shadows == 0 && VeryLow.IndirectLighting == 0 && VeryLow.Reflections == 0 && VeryLow.Fog == 0 && VeryLow.Clouds == 0 && VeryLow.Effects == 0 && !VeryLow.bHardwareRayTracing, TEXT("Very Low reduces costly features without a story option"));
	Check(FromJson(nullptr, TEXT("ultra")) == Ultra && FromJson(nullptr, TEXT("low")) == Low && ForPreset(TEXT("invalid")) == High, TEXT("missing graphics object migrates old quality and invalid preset is safe High"));
	auto Broken = S47J::Read(TEXT("{\"version\":1,\"renderScale\":1e100,\"shadows\":\"off\",\"indirectLighting\":null,\"reflections\":1.5,\"textures\":true,\"antiAliasing\":0,\"fog\":-1,\"clouds\":99,\"effects\":[],\"detail\":{},\"hardwareRayTracing\":\"false\"}"));
	Check(FromJson(Broken, TEXT("high")) == High, TEXT("invalid types, fractions and extreme values fall back field by field"));
	S47J::Set(Broken, TEXT("renderScale"), 75); const auto Partial = FromJson(Broken, TEXT("high"));
	Check(Partial.RenderScale == 75 && Partial.Shadows == High.Shadows && Partial.PresetId() == TEXT("custom"), TEXT("valid field survives damaged siblings and becomes Custom"));
	S47J::Set(Broken, TEXT("version"), 2);
	Check(FromJson(Broken, TEXT("low")) == Low, TEXT("unknown schema versions preserve legacy fallback"));
	auto Limits = High; Limits.RenderScale = 999; Limits.Shadows = -1; Limits.Reflections = 9; Limits.AntiAliasing = 0; Limits.Normalize();
	Check(Limits.RenderScale == 100 && Limits.Shadows == 0 && Limits.Reflections == 3 && Limits.AntiAliasing == 1, TEXT("runtime values normalize before application"));
	Check(Low.LegacyQuality() == TEXT("low") && High.LegacyQuality() == TEXT("high") && Ultra.LegacyQuality() == TEXT("ultra"), TEXT("legacy detail consumers retain existing tiers"));
	auto ReflectionFallback = High; ReflectionFallback.IndirectLighting = 0;
	Check(ReflectionFallback.Reflections == 2 && ReflectionFallback.ResolvedReflections() == 1, TEXT("GI off resolves selected Lumen reflections to portable screen space"));
	ReflectionFallback.IndirectLighting = 1;
	Check(ReflectionFallback.ResolvedReflections() == 2 && ReflectionFallback == High, TEXT("GI restored recovers the preserved Lumen reflection preference"));
	return Ok;
}
