// web/src/core/radioVoice.ts, in the source's order.
#include "S47RadioVoice.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	constexpr int32 MAX_BUFFERS = 24;
	constexpr int64 MAX_BYTES = 24 * 1024 * 1024;
	FString Str(const TSharedPtr<FJsonObject>& O, const TCHAR* K) { FString S; if (O) O->TryGetStringField(K, S); return S; }
	const TArray<TSharedPtr<FJsonValue>>& Arr(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		static const TArray<TSharedPtr<FJsonValue>> Empty;
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		return O && O->TryGetArrayField(K, A) && A ? *A : Empty;
	}
	FString Key(const FString& Voice, const FString& Text) { return Voice + TEXT("\x1f") + Text; } // JS uses NUL; FString keys use a separator outside the role/text alphabet.
}

FString FS47RadioVoice::Clean(const FString& Text)
{
	FString S; bool bSpace = false;
	for (TCHAR C : Text)
	{
		if (FChar::IsWhitespace(C)) { bSpace = !S.IsEmpty(); continue; }
		if (bSpace) S.AppendChar(TEXT(' '));
		S.AppendChar(C); bSpace = false;
	}
	return S;
}

FS47RadioVoice::FS47RadioVoice(const FString& InKind) : Kind(InKind)
{
	const FString Bank = Kind == TEXT("ensemble") ? TEXT("ensemble") : Kind == TEXT("earl") ? TEXT("artbell-town") : Kind == TEXT("caller") ? TEXT("radiocast") : TEXT("artbell");
	Root = FPaths::ProjectContentDir() / TEXT("Audio/Raw/voice") / Bank;
	FString Text; TSharedPtr<FJsonObject> Voices;
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Audio/Raw/voices.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Voices) || !Voices) return;
	const TSharedPtr<FJsonObject>* B = nullptr;
	if (!Voices->TryGetObjectField(Bank, B) || !B || !*B) return;
	const TSharedPtr<FJsonObject>* CatPtr = nullptr;
	if (!(*B)->TryGetObjectField(TEXT("catalog"), CatPtr) || !CatPtr || !*CatPtr) return;
	const TSharedPtr<FJsonObject> Cat = *CatPtr;
	for (const auto& V : Arr(Cat, TEXT("clips")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject(); if (!O) continue;
		FS47RadioClip C; C.Id = Str(O, TEXT("id")); C.Text = Str(O, TEXT("text")); C.File = Str(O, TEXT("file")); C.Voice = Str(O, TEXT("voice")); O->TryGetNumberField(TEXT("duration"), C.Duration);
		ById.Add(C.Id, Clips.Num()); ByText.Add(Kind == TEXT("caller") || Kind == TEXT("ensemble") ? Key(C.Voice, Clean(C.Text)) : Clean(C.Text), Clips.Num()); Clips.Add(MoveTemp(C));
	}
	for (const auto& V : Arr(Cat, TEXT("templates")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject(); if (!O) continue;
		FTemplate T; T.Prefix = Str(O, TEXT("prefix")); T.Suffix = Str(O, TEXT("suffix")); T.Before = Str(O, TEXT("before")); T.After = Str(O, TEXT("after")); O->TryGetBoolField(TEXT("capitalize"), T.bCapitalize); Templates.Add(MoveTemp(T));
	}
	const TSharedPtr<FJsonObject>* Time = nullptr;
	if (Cat->TryGetObjectField(TEXT("timeBank"), Time) && Time && *Time)
	{
		auto Parts = [](const TArray<TSharedPtr<FJsonValue>>& Values, TArray<FTimePart>& Into)
		{
			for (const auto& V : Values) { const auto O = V->AsObject(); if (!O) continue; FTimePart P; double N = 0; O->TryGetNumberField(TEXT("value"), N); P.Value = int32(N); P.Id = Str(O, TEXT("id")); P.Text = Str(O, TEXT("text")); Into.Add(MoveTemp(P)); }
		};
		Parts(Arr(*Time, TEXT("hours")), Hours); Parts(Arr(*Time, TEXT("minutes")), Minutes);
	}
}

TOptional<TArray<FS47RadioClip>> FS47RadioVoice::Resolve(const FString& Text, double Clock, const FString& Voice) const
{
	if (Kind == TEXT("ensemble") || Kind == TEXT("earl") || Kind == TEXT("caller"))
	{
		const bool bRole = Kind != TEXT("earl");
		const int32* I = bRole && Voice.IsEmpty() ? nullptr : ByText.Find(bRole ? Key(Voice, Clean(Text)) : Clean(Text));
		if (I) { const TArray<FS47RadioClip> Found = { Clips[*I] }; if (Valid(Found)) return Found; }
		return {};
	}
	if (const int32* I = ByText.Find(Clean(Text)))
	{
		const TArray<FS47RadioClip> Found = { Clips[*I] }; return Valid(Found) ? TOptional<TArray<FS47RadioClip>>(Found) : TOptional<TArray<FS47RadioClip>>();
	}
	if (!FMath::IsFinite(Clock)) return {};
	const double Day = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
	const int32 H12 = int32(FMath::FloorToDouble(Day / 3600)) % 12, H = H12 ? H12 : 12, M = int32(FMath::FloorToDouble(Day / 60)) % 60;
	const FTimePart* Hour = Hours.FindByPredicate([H](const FTimePart& P) { return P.Value == H; });
	const FTimePart* Minute = Minutes.FindByPredicate([M](const FTimePart& P) { return P.Value == M; });
	if (!Hour || !Minute) return {};
	const FString Time = Hour->Text + (M > 0 && M < 10 ? TEXT("-") : TEXT(" ")) + Minute->Text;
	for (const FTemplate& T : Templates)
	{
		FString Spoken = Time; if (T.bCapitalize && Spoken.Len()) Spoken[0] = FChar::ToUpper(Spoken[0]);
		if (Clean(T.Prefix + Spoken + T.Suffix) != Clean(Text)) continue;
		TArray<FS47RadioClip> Found;
		for (const FString& Id : TArray<FString>{ T.Before, Hour->Id, Minute->Id, T.After })
		{
			if (Id.IsEmpty()) continue;
			const int32* I = ById.Find(Id); if (!I) return {};
			Found.Add(Clips[*I]);
		}
		return Valid(Found) ? TOptional<TArray<FS47RadioClip>>(Found) : TOptional<TArray<FS47RadioClip>>();
	}
	return {};
}

bool FS47RadioVoice::Valid(const TArray<FS47RadioClip>& List) const
{
	for (const FS47RadioClip& C : List) if (!(C.Duration > 0) || !FMath::IsFinite(C.Duration) || !IFileManager::Get().FileExists(*Url(C))) return false;
	return true;
}

FString FS47RadioVoice::Url(const FS47RadioClip& Clip) const { return Root / FPaths::ChangeExtension(Clip.File, TEXT("wav")); }

bool FS47RadioVoice::Load(const TArray<FS47RadioClip>& List, TMap<FString, TSharedPtr<FS47AudioBuffer>>& Result, const TFunction<bool()>& IsAborted)
{
	TMap<FString, TSharedPtr<FS47AudioBuffer>> Loaded;
	for (const FS47RadioClip& Clip : List)
	{
		if (IsAborted && IsAborted()) return false;
		TSharedPtr<FS47AudioBuffer> Buffer = Loaded.FindRef(Clip.Id); if (!Buffer.IsValid()) Buffer = Buffers.FindRef(Clip.Id);
		if (!Buffer.IsValid())
		{
			TArray<uint8> Encoded; if (!FFileHelper::LoadFileToArray(Encoded, *Url(Clip))) return false;
			if (IsAborted && IsAborted()) return false;
			Buffer = S47Wa::DecodeWav(Encoded);
			if (IsAborted && IsAborted()) return false;
			if (!Buffer.IsValid() || !(Buffer->Duration() > 0) || !FMath::IsFinite(Buffer->Duration())) return false;
			Buffers.Add(Clip.Id, Buffer); Order.Add(Clip.Id); Bytes += int64(Buffer->Length()) * Buffer->NumberOfChannels() * 4; Trim();
		}
		else if (Buffers.Contains(Clip.Id)) { Order.Remove(Clip.Id); Order.Add(Clip.Id); }
		Loaded.Add(Clip.Id, Buffer);
	}
	Result = MoveTemp(Loaded);
	return true;
}

void FS47RadioVoice::Trim()
{
	while (Buffers.Num() > MAX_BUFFERS || Bytes > MAX_BYTES)
	{
		if (Order.Num() == 0) break;
		const FString Id = Order[0]; Order.RemoveAt(0);
		const auto B = Buffers.FindRef(Id); if (B.IsValid()) Bytes -= int64(B->Length()) * B->NumberOfChannels() * 4;
		Buffers.Remove(Id);
	}
}

bool FS47RadioVoice::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&Lines, &bOk](bool b, const TCHAR* T) { Lines.Add(FString::Printf(TEXT("%s radioVoice: %s"), b ? TEXT("PASS") : TEXT("FAIL"), T)); bOk &= b; };
	Check(Clean(TEXT(" \tThe\n night  line. \r")) == TEXT("The night line."), TEXT("exact whitespace normalization"));
	FS47RadioVoice V; V.Clips.Reset(); V.ById.Reset(); V.ByText.Reset(); V.Templates.Reset(); V.Hours.Reset(); V.Minutes.Reset();
	V.Root = FPaths::ProjectSavedDir(); const FString File = TEXT("S47RadioVoiceSelfTest.wav"), Path = V.Root / File;
	TArray<uint8> Wav; Wav.SetNumZeroed(46); FMemory::Memcpy(Wav.GetData(), "RIFF", 4); FMemory::Memcpy(Wav.GetData() + 8, "WAVEfmt ", 8); FMemory::Memcpy(Wav.GetData() + 36, "data", 4);
	auto Set16 = [&Wav](int32 At, uint16 N) { Wav[At] = uint8(N); Wav[At + 1] = uint8(N >> 8); }; auto Set32 = [&Set16](int32 At, uint32 N) { Set16(At, uint16(N)); Set16(At + 2, uint16(N >> 16)); };
	Set32(4, 38); Set32(16, 16); Set16(20, 1); Set16(22, 1); Set32(24, 48000); Set32(28, 96000); Set16(32, 2); Set16(34, 16); Set32(40, 2);
	if (!FFileHelper::SaveArrayToFile(Wav, *Path)) { Check(false, TEXT("test recording written")); return false; }
	for (int32 I = 0; I < 25; ++I) { FS47RadioClip C; C.Id = FString::FromInt(I); C.Text = C.Id; C.Duration = 1; C.File = File; V.ById.Add(C.Id, I); V.Clips.Add(C); }
	V.Hours.Add({ 12, TEXT("0"), TEXT("twelve") }); V.Minutes.Add({ 5, TEXT("1"), TEXT("oh five") }); V.Templates.Add({ TEXT("It is "), TEXT("."), true, TEXT("2"), TEXT("3") });
	const auto R = V.Resolve(TEXT("It is Twelve-oh five."), -86100);
	Check(R.IsSet() && R->Num() == 4 && (*R)[0].Id == TEXT("2") && (*R)[1].Id == TEXT("0") && (*R)[2].Id == TEXT("1") && (*R)[3].Id == TEXT("3"), TEXT("authored time parts, negative clock, oh hyphen and order"));
	Check(!V.Resolve(TEXT("It is twelve oh five."), 300).IsSet(), TEXT("unknown words do not silently disappear"));
	V.Kind = TEXT("caller"); V.ByText.Add(Key(TEXT("boy"), TEXT("Hello.")), 0);
	Check(V.Resolve(TEXT("Hello."), 0, TEXT("boy")).IsSet() && !V.Resolve(TEXT("Hello."), 0, TEXT("woman")).IsSet(), TEXT("caller role remains part of identity"));
	TMap<FString, TSharedPtr<FS47AudioBuffer>> Out;
	Check(V.Load(V.Clips, Out) && Out.Num() == 25 && V.BufferCount() == 24 && !V.Buffers.Contains(TEXT("0")) && Out.FindRef(TEXT("0")).IsValid(), TEXT("LRU capped at 24 while active plan keeps evicted clips"));
	Check(!V.Load({ V.Clips[0] }, Out, [] { return true; }) && V.BufferCount() == 24, TEXT("aborted plan does not load or start audio"));
	IFileManager::Get().Delete(*Path);
	return bOk;
}
