// Unreal-only recording workshop, sharing the game's runtime graph and mixer.
#include "S47Audio.h"
#include "S47Score.h"
#include "S47FieldRadio.h"
#include "S47SignalVoice.h"
#include "S47ReceiverSound.h"
#include "S47Rx.h"
#include "S47Observing.h"
#include "S47Web.h"
#include "S47Three.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SoundWorkshop, Log, All);

namespace
{
// T16-09: out of the control room this long with the modem going, and its sound is stopped (US47Audio::StepTransferWalls).
constexpr double TransferLeaveSeconds = 1.5;

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

void US47Audio::PcTransfer(double Seconds, TOptional<FVector> At)
{
	if (!Ctx || !FMath::IsFinite(Seconds) || Seconds <= 0) return;
	TArray<FString> Errors;
	TSharedPtr<FS47AudioNode> Dest;
	TransferWalls.Reset(); TransferLp.Reset(); bTransferHere = false; TransferOutSince = -1;
	{
		// T16-09: the modem is the OPS PC's, in the room. Mix -> dull filter -> walls gain -> panner (distance) at the PC.
		// Without a place (no OPS PC loaded) the same walls, straight to the bus: it still stays in the room.
		FS47AudioContext::FBatch Batch(*Ctx);
		TransferLp = Ctx->CreateBiquadFilter(); TransferLp->SetType(TEXT("lowpass")); TransferLp->Frequency.SetValue(18000);
		TransferWalls = Ctx->CreateGain(); TransferWalls->Gain.SetValue(1);
		TransferLp->Connect(TransferWalls);
		if (At.IsSet()) TransferWalls->Connect(Panner(At.GetValue(), 2.5)); else TransferWalls->Connect(SfxBus);
		Dest = TransferLp;
		bTransferHere = true;
	}
	if (!PlaySoundMix(TEXT("pc_transfer"),TEXT("pc_transfer"),ES47SoundUse::PcTransfer,Errors,Dest)) { TransferWalls.Reset(); TransferLp.Reset(); bTransferHere = false; return; }
	SoundMixFrame(TEXT("pc_transfer"),1,0,1,Ctx->CurrentTime());
	const uint64 Token = ++TransferSerial;
	TWeakObjectPtr<US47Audio> Self(this);
	Ctx->SetTimeout(FMath::Clamp(Seconds,0.2,27.0),[Self,Token]
	{
		if (Self.IsValid() && Self->TransferSerial == Token) { Self->StopSoundMix(TEXT("pc_transfer"),0.08f); Self->bTransferHere = false; }
	});
}

void US47Audio::PcTransferAtOps(AS47Web* Web, double Seconds)
{
	TOptional<FVector> At;
	if (Web && Web->IsLoaded())
		if (const FS47WebItem* Item = Web->Item(TEXT("opsTerminal")); (Item && (Item->bBox || Web->ObjAt(Item->Node))) || Web->Handle(TEXT("room.objs.opsTerminal")) >= 0)
		{
			// (PropLocation falls back to the room's node when the export has no interactable for it)
			const FVector P = S47Three::ToThree(Web->PropLocation(TEXT("opsTerminal")));
			if (!P.ContainsNaN()) At = P;
		}
	PcTransfer(Seconds, At);
}

bool US47Audio::InControlRoom(const FVector& Eye)
{
	// Under its ceiling (3 m; the floor is at 0): on the roof above it (the ladder, T16-11, the roof at 3.6) the player is out of it.
	return Eye.X > -6.f && Eye.X < 6.f && Eye.Z > -4.5f && Eye.Z < 4.5f && Eye.Y < 3.f;
}

// Control room bounds (ControlRoom.ts: x +-6, z +-4.5, walls .3 thick). Out of it the modem comes through a wall and a door:
// a quarter of the level and nothing above a few hundred hertz. Eased, so a step through the door is not a click.
// T16-09, second round (Tom: the modem sound follows you out after a job is sent): out of the room for TransferLeaveSeconds it
// fades away and stops, as a door shut on it would; the transfer itself (the job) is not touched, only its sound.
void US47Audio::StepTransferWalls(const FVector& Eye)
{
	if (!bTransferHere || !Ctx.IsValid() || !TransferWalls.IsValid() || !TransferLp.IsValid()) return;
	const bool bIn = InControlRoom(Eye);
	FS47AudioContext::FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime();
	TransferWalls->Gain.SetTargetAtTime(bIn ? 1.0 : 0.25, T, 0.12);
	TransferLp->Frequency.SetTargetAtTime(bIn ? 18000.0 : 450.0, T, 0.12);
	if (bIn) { TransferOutSince = -1; return; }
	if (TransferOutSince < 0) TransferOutSince = T;
	if (T - TransferOutSince < TransferLeaveSeconds) return;
	TransferWalls->Gain.SetTargetAtTime(0.0, T, 0.15);
	StopSoundMix(TEXT("pc_transfer"), 0.6f);
	++TransferSerial; // (its timeout has nothing left to stop)
	bTransferHere = false;
}

// T16-09: the modem's walls, offline. A tone stands in for the recording (the bank is not loaded in a self test).
bool US47Audio::TransferSelfTest(TArray<FString>& Lines)
{
	bool Ok = true;
	const auto Check = [&](bool B, const TCHAR* Why) { Ok &= B; Lines.Add(FString::Printf(TEXT("%s modem: %s"), B ? TEXT("PASS") : TEXT("FAIL"), Why)); };
	Check(InControlRoom(FVector(0, 1.6, -3.0)) && InControlRoom(FVector(-5.5, 1.6, 4.0)) && !InControlRoom(FVector(-2.4, 1.6, 5.6)) && !InControlRoom(FVector(8.9, 1.6, 8.0))
		&& !InControlRoom(FVector(-1.5, 3.6 + 1.6, -1.0)),
		TEXT("the control room is the room the OPS PC is in; the corridor, the yard and the roof above it are not"));
	US47Audio* A = NewObject<US47Audio>(GetTransientPackage()); A->AddToRoot();
	constexpr int32 Rate = 48000;
	A->Ctx = FS47AudioContext::Make(Rate); A->bReady = true;
	A->Master = A->Ctx->CreateGain(); A->Master->Gain.SetValue(1); A->Master->Connect(A->Ctx->Destination);
	A->SfxBus = A->Ctx->CreateGain(); A->SfxBus->Connect(A->Master);
	{
		FS47AudioContext::FBatch Batch(*A->Ctx);
		A->TransferLp = A->Ctx->CreateBiquadFilter(); A->TransferLp->SetType(TEXT("lowpass")); A->TransferLp->Frequency.SetValue(18000);
		A->TransferWalls = A->Ctx->CreateGain(); A->TransferWalls->Gain.SetValue(1);
		A->TransferLp->Connect(A->TransferWalls); A->TransferWalls->Connect(A->SfxBus);
		auto Tone = A->Ctx->CreateOscillator(); Tone->Frequency.SetValue(1200); Tone->Connect(A->TransferLp); Tone->Start(0);
		A->bTransferHere = true; A->TransferOutSince = -1;
	}
	TArray<float> Samples; Samples.SetNumZeroed(Rate / 10 * 2);
	auto Run = [&](const FVector& Eye, double Seconds)
	{
		double Top = 0;
		for (double T = 0; T < Seconds - 1e-6; T += .1)
		{
			A->StepTransferWalls(Eye);
			A->Ctx->Render(Samples.GetData(), Rate / 10, 2); A->Ctx->Tick(.1);
			Top = 0; for (float V : Samples) Top = FMath::Max(Top, FMath::Abs(double(V)));
		}
		return Top; // the last tenth of a second
	};
	const FVector Desk(0, 1.6, -3.0), Corridor(-2.4, 1.6, 5.6);
	const double In = Run(Desk, .6);
	const double OutSoon = Run(Corridor, .5);
	Check(In > .3 && OutSoon < In * .4 && A->bTransferHere, TEXT("stepping out, the modem drops behind the wall at once"));
	Run(Desk, .3);
	Check(A->bTransferHere && A->TransferOutSince < 0, TEXT("back in the room within a moment, it is there again"));
	const double Gone = Run(Corridor, TransferLeaveSeconds + 1.2);
	Check(!A->bTransferHere && Gone < .002, TEXT("out of the room, it fades and stops instead of following the player"));
	{
		// Up the ladder onto the roof right above the desk: out of the room as well, and it stops there too.
		FS47AudioContext::FBatch Batch(*A->Ctx);
		A->TransferLp->Frequency.SetValue(18000); A->TransferWalls->Gain.SetValue(1);
		A->bTransferHere = true; A->TransferOutSince = -1;
	}
	const double Again = Run(Desk, .6);
	const double Roof = Run(FVector(-1.5, 3.6 + 1.6, -1.0), TransferLeaveSeconds + 1.2);
	Check(Again > .3 && !A->bTransferHere && Roof < .002, TEXT("on the roof above the control room it stops as well"));
	A->Ctx->Close(); A->RemoveFromRoot();
	return Ok;
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
