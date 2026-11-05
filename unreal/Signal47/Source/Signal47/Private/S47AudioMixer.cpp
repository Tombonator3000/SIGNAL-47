// Unreal-only recording workshop, sharing the game's runtime graph and mixer.
#include "S47Audio.h"
#include "S47Score.h"
#include "S47FieldRadio.h"
#include "S47SignalVoice.h"
#include "S47ReceiverSound.h"
#include "S47Rx.h"
#include "S47Observing.h"
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
	if (Samples.IsEmpty()) return false;
	for (float X : Samples) if (!FMath::IsFinite(X) || FMath::Abs(X) > 1) return false;
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
	// A successful live reload must not keep playing layers from the old title bank.
	const bool bRestartTitle = bTitleTexture;
	if (bRestartTitle) TitleTexture(false);
	SoundBank = MoveTemp(NextBank);
	SoundRecipes = MoveTemp(NextRecipes);
	if (bRestartTitle) TitleTexture(true);
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

void US47Audio::PcTransfer(double Seconds)
{
	if (!Ctx || !FMath::IsFinite(Seconds) || Seconds <= 0) return;
	TArray<FString> Errors;
	if (!PlaySoundMix(TEXT("pc_transfer"),TEXT("pc_transfer"),ES47SoundUse::PcTransfer,Errors)) return;
	SoundMixFrame(TEXT("pc_transfer"),1,0,1,Ctx->CurrentTime());
	const uint64 Token = ++TransferSerial;
	TWeakObjectPtr<US47Audio> Self(this);
	Ctx->SetTimeout(FMath::Clamp(Seconds,0.2,27.0),[Self,Token]
	{
		if (Self.IsValid() && Self->TransferSerial == Token) Self->StopSoundMix(TEXT("pc_transfer"),0.08f);
	});
}

bool US47Audio::RenderSoundWorkshop(const FString& BankPath)
{
	US47Audio* A = NewObject<US47Audio>(GetTransientPackage());
	A->AddToRoot();
	A->Unlock(false, false);
	TArray<FString> Errors;
	bool bOk = A->LoadSoundBank(BankPath, Errors);
	bOk &= FS47SoundMixer::SelfTest(Errors);
	bOk &= TitleTextureSelfTest(Errors);
	bOk &= FS47ReceiverSound::SelfTest(Errors);
	bOk &= FS47Score::SelfTest(Errors);
	bOk &= FS47FieldRadio::SelfTest(Errors);
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
	if (bOk) bOk &= FS47Score::RenderTour(Output / TEXT("adaptive_music.wav"), Errors);
	if (bOk)
	{
		// Listen to the default title and anomaly, independently of the private bank.
		US47Audio* Plain = NewObject<US47Audio>(GetTransientPackage());
		Plain->AddToRoot(); Plain->Unlock(false, false); Plain->TitleTexture(true);
		TArray<float> Samples; Samples.SetNumZeroed(48000 * 12 * 2);
		for (int32 F=0; F<48000*12; F+=800)
		{
			Plain->TitleTextureFrame(Plain->Ctx->CurrentTime());
			if (F == 48000*10) Plain->TitleTexture(false);
			Plain->Ctx->Render(Samples.GetData()+F*2,800,2); Plain->Ctx->Tick(1.0/60);
		}
		bOk &= SaveWave(Output / TEXT("title_synthetic.wav"), Samples);
		Plain->Ctx->Close(); Plain->RemoveFromRoot();
		auto C = FS47AudioContext::Make(48000);
		FS47SignalVoice Voice(C,C->Destination,A->Noise);
		Samples.Init(0,48000*12*2);
		for (int32 F=0; F<48000*12; F+=800)
		{
			const double T = F/48000.0;
			Voice.Frame(FMath::Min(1.0,T/5),T < 10 ? 0.9 : 0,T);
			C->Render(Samples.GetData()+F*2,800,2); C->Tick(1.0/60);
		}
		bOk &= SaveWave(Output / TEXT("array_synthetic.wav"), Samples);
		Voice.Stop(); C->Close();
	}
	// Actual Freeform receiver graph, independent of the private recording bank.
	// Each source gets a fresh context: 0-2s search, 2-7s tuning/reception ramp,
	// 7-10s locked, then power off with two seconds to hear and check the release.
	for (int32 Source=0; Source<FS47Observing::Runs().Num(); ++Source)for(int32 Version:{1,2})
	{
		US47Audio* Receiver=NewObject<US47Audio>(GetTransientPackage());
		Receiver->AddToRoot();Receiver->Unlock(false,false);
		const FS47Profile& Profile=FS47Observing::Runs()[Source].P;
		const FString Id=FString::Printf(TEXT("freeform-source-%d"),Source);
		FS47Rx Rx;Rx.bPowered=true;Rx.Gain=(Profile.Gain[0]+Profile.Gain[1])*.5;
		Rx.Bandwidth=(Profile.Bw[0]+Profile.Bw[1])*.5;Rx.Azimuth=(Profile.Az[0]+Profile.Az[1])*.5;
		TArray<float> Samples;Samples.SetNumZeroed(48000*12*2);
		for (int32 F=0; F<48000*12; F+=800)
		{
			const double T=F/48000.;
			const double Tune=FMath::Clamp((T-2.)/5.,0.,1.);
			Rx.Frequency=Profile.F+Profile.FTol*4.*(1.-Tune);
			if(T<10)Receiver->MonitorReceiver(Rx,Profile,Id,.08+.92*Tune,Version);
			else if(F==48000*10)
			{
				Rx.bPowered=false;Receiver->MonitorReceiver(Rx,Profile,Id,1.,Version);
				Receiver->ReceiverMonitorOff(); // Repeated off must remain harmless.
			}
			Receiver->Ctx->Render(Samples.GetData()+F*2,800,2);Receiver->Ctx->Tick(1./60.);
		}
		bool Finite=true;double Peak=0,Energy=0,TailPeak=0;
		for(int32 I=0;I<Samples.Num();++I)
		{
			const double V=Samples[I];Finite&=FMath::IsFinite(V);Energy+=V*V;
			Peak=FMath::Max(Peak,FMath::Abs(V));
			if(I>=48000*23)TailPeak=FMath::Max(TailPeak,FMath::Abs(V)); // Last half second.
		}
		const double Rms=FMath::Sqrt(Energy/Samples.Num());
		const FString File=FString::Printf(TEXT("receiver_source_%d_v%d_search_tuned_off.wav"),Source,Version);
		const bool Valid=Finite&&Rms>1e-6&&Peak<.25&&TailPeak<1e-7;
		const bool Saved=Valid&&SaveWave(Output/File,Samples);bOk&=Saved;
		auto Result=MakeShared<FJsonObject>();Result->SetStringField(TEXT("name"),Id);
		Result->SetStringField(TEXT("source"),FS47Observing::Runs()[Source].Source);
  Result->SetNumberField(TEXT("texture_version"),Version);Result->SetBoolField(TEXT("human_listening_verified"),false);
		Result->SetStringField(TEXT("file"),File);Result->SetStringField(TEXT("graph"),TEXT("MonitorReceiver: search 0-2, tuning 2-7, locked 7-10, powered off 10-12 seconds"));
		Result->SetNumberField(TEXT("peak"),Peak);Result->SetNumberField(TEXT("rms"),Rms);
		Result->SetNumberField(TEXT("off_tail_peak"),TailPeak);Result->SetBoolField(TEXT("ok"),Saved);
		Results.Add(MakeShared<FJsonValueObject>(Result));
		UE_LOG(LogS47SoundWorkshop,Display,TEXT("S47: receiver workshop %s peak %.5f rms %.5f off %.9f %s"),*Id,Peak,Rms,TailPeak,Saved?TEXT("PASS"):TEXT("FAIL"));
		Receiver->Ctx->Close();Receiver->RemoveFromRoot();
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


void US47Audio::TitleTexture(bool bOn)
{
	if (!Ctx.IsValid() || bOn == bTitleTexture) return;
	if (!bOn)
	{
		bTitleTexture = false;
		StopSoundMix(TEXT("title_vhs"), 0.f);
		bTitlePrivateMix = false;
		if (TitleTape.Out.IsValid())
		{
			FS47AudioContext::FBatch Batch(*Ctx);
			const double Now = Ctx->CurrentTime();
			const auto Out = TitleTape.Out;
			const auto Sources = MoveTemp(TitleTape.Sources);
			Out->Gain.CancelAndHoldAtTime(Now);
			// A fresh title may open immediately: release the old tape now, not after
			// a fade which would overlap a newly-created graph on rapid re-entry.
			Out->Gain.SetValue(0);
			for (const auto& Source : Sources) { Source->Stop(Now); Source->Disconnect(); }
			Out->Disconnect();
			TitleTape = {};
		}
		return;
	}
	if (!Noise.IsValid() || !SfxBus.IsValid()) return;
	bTitleTexture = true;
	TArray<FString> Errors;
	if (SoundRecipes.Contains(TEXT("title_vhs")) &&
		PlaySoundMix(TEXT("title_vhs"), TEXT("title_vhs"), ES47SoundUse::Vhs, Errors))
	{
		bTitlePrivateMix = true;
		TitleTextureFrame(Ctx->CurrentTime());
		return;
	}
	// Original procedural fallback: continuous soft hiss, a faint hum and very slow wow.
	// No crackle recording or modem can be selected by this graph.
	FS47AudioContext::FBatch Batch(*Ctx);
	auto Out = Ctx->CreateGain(); Out->Gain.SetValue(0); Out->Connect(SfxBus);
	Out->Gain.SetTargetAtTime(1, Ctx->CurrentTime(), 0.7);
	auto Gate = Ctx->CreateGain(); Gate->Gain.SetValue(1); Gate->Connect(Out);
	auto Hiss = Ctx->CreateGain(); Hiss->Gain.SetValue(0.007);
	auto NoiseSrc = Ctx->CreateBufferSource(); NoiseSrc->Buffer = Noise; NoiseSrc->bLoop = true;
	auto Hp = Ctx->CreateBiquadFilter(); Hp->SetType(TEXT("highpass")); Hp->Frequency.SetValue(320); Hp->Q.SetValue(0.5);
	auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(4800); Lp->Q.SetValue(0.5);
	NoiseSrc->Connect(Hp); Hp->Connect(Lp); Lp->Connect(Hiss); Hiss->Connect(Gate);
	auto Hum = Ctx->CreateOscillator(); Hum->Frequency.SetValue(60);
	auto TitleHumGain = Ctx->CreateGain(); TitleHumGain->Gain.SetValue(0.0009); Hum->Connect(TitleHumGain); TitleHumGain->Connect(Gate);
	auto Wow = Ctx->CreateOscillator(); Wow->Frequency.SetValue(0.17);
	auto WowDepth = Ctx->CreateGain(); WowDepth->Gain.SetValue(0.18); Wow->Connect(WowDepth); WowDepth->Connect(Hum->Frequency);
	TitleTape.Out = Out; TitleTape.Gate = Gate; TitleTape.Hiss = Hiss;
	TitleTape.Sources = { NoiseSrc, Hum, Wow };
	for (const auto& Source : TitleTape.Sources) Source->Start();
}

void US47Audio::TitleTextureFrame(double T)
{
	if (!bTitleTexture || !Ctx.IsValid()) return;
	if (!FMath::IsFinite(T)) T = Ctx->CurrentTime();
	const double Cycle = FMath::Fmod(FMath::Max(0.0, T), 19.0);
	const double Tracking = Cycle > 15.8 && Cycle < 16.5 ? FMath::Square(FMath::Sin((Cycle - 15.8) / 0.7 * PI)) : 0;
	if (bTitlePrivateMix)
	{
		SoundMixFrame(TEXT("title_vhs"), 0.9, 0.05, Tracking, T);
		return;
	}
	if (!TitleTape.Gate.IsValid()) return;
	FS47AudioContext::FBatch Batch(*Ctx);
	const double Now = Ctx->CurrentTime();
	TitleTape.Gate->Gain.SetTargetAtTime(1 - 0.4 * Tracking, Now, 0.06);
	TitleTape.Hiss->Gain.SetTargetAtTime(0.007 + 0.004 * Tracking, Now, 0.06);
}

bool US47Audio::TitleTextureSelfTest(TArray<FString>& Lines)
{
	bool bAll = true;
	auto Check = [&](bool bOk, const TCHAR* Name) { bAll &= bOk; Lines.Add(FString::Printf(TEXT("%s title texture: %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), Name)); };
	US47Audio* A = NewObject<US47Audio>(GetTransientPackage()); A->AddToRoot(); A->Unlock(false, false);
	auto Run = [&](double Seconds, bool bFrame)
	{
		double Energy = 0, Peak = 0; bool bFinite = true;
		TArray<float> Samples; Samples.SetNumZeroed(1600);
		for (int32 I = 0; I < int32(Seconds * 60); ++I)
		{
			if (bFrame) A->TitleTextureFrame(A->Ctx->CurrentTime());
			A->Ctx->Render(Samples.GetData(), 800, 2); A->Ctx->Tick(1.0 / 60);
			for (float X : Samples) { bFinite &= FMath::IsFinite(X); Energy += X * X; Peak = FMath::Max(Peak, double(FMath::Abs(X))); }
		}
		return FVector(Energy, Peak, bFinite ? 1 : 0);
	};
	A->TitleTexture(true);
	const auto Out = A->TitleTape.Out;
	for (int32 I = 0; I < 30; ++I) A->TitleTexture(true);
	Check(A->bTitleTexture && A->TitleTape.Out == Out && A->TitleTape.Sources.Num() == 3, TEXT("repeated menu frames retain one tape graph"));
	const FVector Active = Run(2, true);
	Check(Active.Z == 1 && Active.X > 0.001 && Active.Y < 0.04, TEXT("default tape renders soft finite nonzero samples"));
	A->TitleTexture(false); A->TitleTexture(false); Run(2, false);
	const FVector After = Run(0.1, false);
	Check(!A->bTitleTexture && !A->TitleTape.Out.IsValid() && After.X == 0, TEXT("leaving menu releases loops and leaves silence"));
	for (int32 I = 0; I < 6; ++I)
	{
		A->TitleTexture(true); Run(0.2, true); const auto Previous = A->TitleTape.Out;
		A->TitleTexture(false);
		// Render() may already hold the remainder of one 128-sample quantum.
		float Pending[S47Quantum * 2]; A->Ctx->Render(Pending, S47Quantum, 2);
		Check(Previous.IsValid() && Run(1. / 60, false).X == 0,
			TEXT("rapid title exit has no fallback tail beyond the queued audio quantum"));
	}
	// The same lifecycle applies when an explicitly selected private bank provides the title tape.
	FS47MixRecipe Recipe; Recipe.Name = TEXT("title_vhs"); Recipe.Use = ES47SoundUse::Vhs;
	FS47MixLayer Layer; Layer.Source = TEXT("test_hiss"); Layer.GainDb = -42; Recipe.Layers.Add(Layer);
	A->SoundBank.Add(TEXT("test_hiss"), { A->Noise, false }); A->SoundRecipes.Add(Recipe.Name, Recipe);
	A->TitleTexture(true); Run(0.2, true);
	Check(A->bTitlePrivateMix && A->SoundMixes.Num() == 1, TEXT("selected private title recipe uses the common mixer"));
	A->TitleTexture(false);
	Check(Run(1. / 60, false).X == 0, TEXT("private title recipe has no audible tail on immediate exit"));
	Run(2, false);
	Check(A->SoundMixes.IsEmpty() && Run(0.1, false).X == 0, TEXT("private title recipe stops without residual playback"));
	A->Ctx->Close(); A->RemoveFromRoot(); return bAll;
}
