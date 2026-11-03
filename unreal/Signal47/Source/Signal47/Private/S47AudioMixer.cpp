// Unreal-only recording workshop, sharing the game's runtime graph and mixer.
#include "S47Audio.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SoundWorkshop, Log, All);

namespace
{
bool SafeName(const FString& S)
{
	if (S.IsEmpty() || S.Len() > 80) return false;
	for (const TCHAR C : S) if (!(C >= 'a' && C <= 'z') && !(C >= '0' && C <= '9') && C != '_') return false;
	return true;
}

bool SaveWave(const FString& Path, const TArray<float>& Samples)
{
	TArray<uint8> Bytes;
	auto U32 = [&Bytes](uint32 V) { for (int I = 0; I < 4; ++I) Bytes.Add(uint8(V >> (8 * I))); };
	auto U16 = [&Bytes](uint16 V) { Bytes.Add(uint8(V)); Bytes.Add(uint8(V >> 8)); };
	auto Tag = [&Bytes](const char* S) { for (int I = 0; I < 4; ++I) Bytes.Add(uint8(S[I])); };
	Tag("RIFF"); U32(36 + Samples.Num() * 2); Tag("WAVE");
	Tag("fmt "); U32(16); U16(1); U16(2); U32(48000); U32(192000); U16(4); U16(16);
	Tag("data"); U32(Samples.Num() * 2);
	for (float X : Samples) U16(uint16(int16(FMath::Clamp(X, -1.f, 1.f) * 32767.f)));
	return FFileHelper::SaveArrayToFile(Bytes, *Path);
}
}

bool US47Audio::LoadSoundBank(const FString& Path, TArray<FString>& Errors)
{
	if (!Ctx.IsValid()) { Errors.Add(TEXT("Audio context is not ready")); return false; }
	FString Text;
	TSharedPtr<FJsonObject> J;
	if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) || !J.IsValid())
	{ Errors.Add(TEXT("Cannot read sound bank JSON: ") + Path); return false; }
	double Version = 0;
	const TSharedPtr<FJsonObject>* Sources = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* Recipes = nullptr;
	if (!J->TryGetNumberField(TEXT("version"), Version) || Version != 1 || !J->TryGetObjectField(TEXT("sources"), Sources)
		|| !J->TryGetArrayField(TEXT("recipes"), Recipes) || (*Sources)->Values.Num() > 32 || Recipes->Num() > 32)
	{ Errors.Add(TEXT("Expected version 1, sources and recipes (maximum 32 each)")); return false; }
	TMap<FString, FS47MixAsset> NextBank;
	// Existing approved game recordings can be used without making another copy.
	for (const auto& B : Buf) NextBank.Add(B.Key, { B.Value, B.Key.Contains(TEXT("modem")) });
	const FString Dir = FPaths::ConvertRelativePathToFull(FPaths::GetPath(Path));
	for (const auto& Entry : (*Sources)->Values)
	{
		const FString Key(Entry.Key.ToView());
		const TSharedPtr<FJsonObject>* Source = nullptr;
		FString File;
		bool bModem = false;
		if (!SafeName(Key) || !Entry.Value->TryGetObject(Source) || !(*Source)->TryGetStringField(TEXT("file"), File)
			|| !(*Source)->TryGetBoolField(TEXT("modem_only"), bModem) || !FPaths::IsRelative(File)
			|| File.Contains(TEXT("..")) || !File.EndsWith(TEXT(".wav")))
		{ Errors.Add(TEXT("Invalid source entry: ") + Key); return false; }
		TArray<uint8> Bytes;
		if (IFileManager::Get().FileSize(*(Dir / File)) > 64 * 1024 * 1024 || !FFileHelper::LoadFileToArray(Bytes, *(Dir / File)))
		{ Errors.Add(TEXT("Cannot read source (maximum 64 MB): ") + Key); return false; }
		auto Buffer = S47Wa::DecodeWav(Bytes);
		if (!Buffer.IsValid() || Buffer->Length() <= 0)
		{ Errors.Add(TEXT("Invalid PCM WAVE source: ") + Key); return false; }
		NextBank.Add(Key, { Buffer, bModem || Key.Contains(TEXT("modem")) });
	}
	TMap<FString, FS47MixRecipe> NextRecipes;
	for (const auto& R : *Recipes)
	{
		const TSharedPtr<FJsonObject>* Obj = nullptr;
		FS47MixRecipe Recipe;
		if (!R->TryGetObject(Obj) || !FS47SoundMixer::ParseRecipe(*Obj, Recipe, Errors)) return false;
		if (!SafeName(Recipe.Name) || NextRecipes.Contains(Recipe.Name))
		{ Errors.Add(TEXT("Invalid or duplicate recipe name: ") + Recipe.Name); return false; }
		// Validate the entire bank before replacing any live settings. Destination is detached.
		auto Detached = Ctx->CreateGain();
		auto Test = FS47SoundMixer::Make(Ctx.ToSharedRef(), Detached, Recipe, NextBank, Errors);
		if (!Test.IsValid()) return false;
		Test->Stop(0);
		NextRecipes.Add(Recipe.Name, MoveTemp(Recipe));
	}
	SoundBank = MoveTemp(NextBank);
	SoundRecipes = MoveTemp(NextRecipes);
	return true;
}

bool US47Audio::PlaySoundMix(const FString& Key, const FString& Recipe, ES47SoundUse Use,
	TArray<FString>& Errors, TSharedPtr<FS47AudioNode> Destination)
{
	const FS47MixRecipe* R = SoundRecipes.Find(Recipe);
	if (!Ctx.IsValid() || !R || R->Use != Use)
	{ Errors.Add(TEXT("Recipe is missing or is reserved for another device: ") + Recipe); return false; }
	auto Mix = FS47SoundMixer::Make(Ctx.ToSharedRef(), Destination.IsValid() ? Destination : SfxBus, *R, SoundBank, Errors);
	if (!Mix.IsValid()) return false;
	StopSoundMix(Key);
	SoundMixes.Add(Key, Mix);
	return true;
}

void US47Audio::SoundMixFrame(const FString& Key, double Clarity, double Interference, double Activity, double T)
{
	if (auto* Mix = SoundMixes.Find(Key)) (*Mix)->Frame(Clarity, Interference, Activity, T);
}

void US47Audio::StopSoundMix(const FString& Key, float Fade)
{
	if (auto* Mix = SoundMixes.Find(Key)) { (*Mix)->Stop(Fade); SoundMixes.Remove(Key); }
}

bool US47Audio::RenderSoundWorkshop(const FString& BankPath)
{
	US47Audio* A = NewObject<US47Audio>(GetTransientPackage());
	A->AddToRoot();
	A->Unlock(false, false);
	TArray<FString> Errors;
	bool bOk = A->LoadSoundBank(BankPath, Errors);
	bOk &= FS47SoundMixer::SelfTest(Errors);
	const FString Output = FPaths::GetPath(FPaths::ConvertRelativePathToFull(BankPath)) / TEXT("previews");
	IFileManager::Get().MakeDirectory(*Output, true);
	TArray<TSharedPtr<FJsonValue>> Results;
	if (bOk) for (const auto& Entry : A->SoundRecipes)
	{
		// Fresh context for each recipe: no remaining tail from the previous sound.
		auto C = FS47AudioContext::Make(48000.f);
		auto Mix = FS47SoundMixer::Make(C, C->Destination, Entry.Value, A->SoundBank, Errors);
		if (!Mix.IsValid()) { bOk = false; C->Close(); break; }
		TArray<float> Samples;
		Samples.SetNumZeroed(48000 * 12 * 2);
		for (int32 Frame = 0; Frame < 48000 * 12; Frame += 800)
		{
			const double T = Frame / 48000.0;
			const double Q = Entry.Key == TEXT("array_search") ? FMath::Clamp(T / 8, 0.0, 1.0) : 0.9;
			Mix->Frame(Q, 1 - Q, T < 10.5 ? 1 : 0, T);
			if (Frame == 48000 * 11) Mix->Stop(0.1);
			C->Render(Samples.GetData() + Frame * 2, 800, 2);
			C->Tick(1.0 / 60);
		}
		double Sum = 0, Peak = 0;
		bool bFinite = true;
		for (float X : Samples) { bFinite &= FMath::IsFinite(X); Sum += X * X; Peak = FMath::Max(Peak, double(FMath::Abs(X))); }
		const double Rms = FMath::Sqrt(Sum / Samples.Num());
		const bool bValid = bFinite && Rms > 0.000001 && Peak < 1;
		const FString File = Entry.Key + TEXT(".wav");
		const bool bSaved = bValid && SaveWave(Output / File, Samples);
		bOk &= bSaved;
		auto Result = MakeShared<FJsonObject>();
		Result->SetStringField(TEXT("name"), Entry.Key); Result->SetStringField(TEXT("file"), File);
		Result->SetNumberField(TEXT("peak"), Peak); Result->SetNumberField(TEXT("rms"), Rms);
		Result->SetBoolField(TEXT("ok"), bSaved); Results.Add(MakeShared<FJsonValueObject>(Result));
		UE_LOG(LogS47SoundWorkshop, Display, TEXT("S47: sound workshop %s peak %.5f rms %.5f %s"), *Entry.Key, Peak, Rms, bSaved ? TEXT("PASS") : TEXT("FAIL"));
		Mix->Stop(0); C->Close();
	}
	for (const FString& Error : Errors) UE_LOG(LogS47SoundWorkshop, Display, TEXT("S47: sound workshop: %s"), *Error);
	auto Report = MakeShared<FJsonObject>();
	Report->SetBoolField(TEXT("ok"), bOk); Report->SetArrayField(TEXT("recipes"), Results);
	FString Text;
	FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Text));
	bOk &= FFileHelper::SaveStringToFile(Text, *(Output / TEXT("results.json")));
	A->Ctx->Close(); A->RemoveFromRoot();
	UE_LOG(LogS47SoundWorkshop, Display, TEXT("S47: sound workshop complete %s: %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), *Output);
	return bOk;
}
