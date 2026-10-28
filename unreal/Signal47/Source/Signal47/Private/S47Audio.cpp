// US47Audio: the context, the recordings, the prologue's names and the self test. The graphs of
// core/Audio.ts are in S47AudioWeb.cpp.
#include "S47Audio.h"
#include "S47Ambience.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "S47Main.h"
#include "S47SignalVoice.h"
#include "S47Synth.h"
#include "S47Three.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Audio, Log, All);

// ---------- preload() and unlock() ----------

void US47Audio::Init(UWorld* InWorld)
{
	World = InWorld;
	Unlock(true);
	FString TourPath;
	if (FParse::Value(FCommandLine::Get(), TEXT("S47AudioTour="), TourPath)) Tour(TourPath);
	FString CompareDir;
	if (FParse::Value(FCommandLine::Get(), TEXT("S47AudioCompare="), CompareDir)) Compare(CompareDir);
}

void US47Audio::Unlock(bool bWithSynth, bool bDecode)
{
	if (Ctx.IsValid()) return;
	Ctx = FS47AudioContext::Make(48000.f);
	{
		FS47AudioContext::FBatch Batch(*Ctx);
		Master = Ctx->CreateGain(); Master->Gain.SetValue(Volume); Master->Connect(Ctx->Destination);
		SfxBus = Ctx->CreateGain(); SfxBus->Connect(Master);
		Music = Ctx->CreateGain(); Music->Gain.SetValue(MusicLevel); Music->Connect(Master);
		Amb = Ctx->CreateGain(); Amb->Gain.SetValue(1); Amb->Connect(Master);
		RadioBus = Ctx->CreateGain(); RadioBus->Gain.SetValue(RadioLevel); RadioBus->Connect(Master);
		Noise = MakeNoise(false); Brown = MakeNoise(true);
	}
	if (bDecode) DecodeAll();
	bReady = true;
	if (!bWithSynth || !World.IsValid()) return;
	// the speaker: one synth that renders the context (S47Synth.h)
	FActorSpawnParameters P;
	P.ObjectFlags |= RF_Transient;
	SynthHost = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, P);
	if (!SynthHost) return;
	Synth = NewObject<US47Synth>(SynthHost, TEXT("S47Synth"));
	Synth->Ctx = Ctx;
	SynthHost->SetRootComponent(Synth);
	Synth->RegisterComponent();
	Synth->Start();
	UE_LOG(LogS47Audio, Display, TEXT("S47: audio context %.0f Hz, %d recordings"), Ctx->SampleRate, Buf.Num());
}

TSharedPtr<FS47AudioBuffer> US47Audio::MakeNoise(bool bBrown) const
{
	const int32 Len = int32(Ctx->SampleRate * 3);
	TSharedRef<FS47AudioBuffer> B = Ctx->CreateBuffer(1, Len, Ctx->SampleRate);
	float* D = B->GetChannelData(0);
	double Last = 0;
	for (int32 I = 0; I < Len; ++I)
	{
		const double W = FMath::FRand() * 2 - 1;
		if (bBrown) { Last = (Last + 0.02 * W) / 1.02; D[I] = float(Last * 3.5); }
		else D[I] = float(W);
	}
	return B;
}

void US47Audio::DecodeAll()
{
	// Scripts/import-audio.sh writes the recordings Audio.ts decodes as WAVE files, with an index:
	// sources (web name -> file), phone (text -> phone:<id>), character (voice -> text -> character:<id>).
	// The bought ones come second, so a file name finds them first (as the old lookup did).
	const FString Dirs[2] = { FPaths::ProjectContentDir() / TEXT("Audio/Raw"), FPaths::ProjectContentDir() / TEXT("Licensed/Raw") };
	for (const FString& Dir : Dirs)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *(Dir / TEXT("index.json"))))
		{
			UE_LOG(LogS47Audio, Warning, TEXT("S47: no %s/index.json (run Scripts/import-audio.sh); those recordings are silent"), *Dir);
			continue;
		}
		TSharedPtr<FJsonObject> J;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) || !J.IsValid()) continue;
		const TSharedPtr<FJsonObject>* Sources = nullptr;
		if (J->TryGetObjectField(TEXT("sources"), Sources))
		{
			for (const auto& S : (*Sources)->Values)
			{
				const FString Key(S.Key.ToView());
				const FString File = S.Value->AsString();
				TArray<uint8> Bytes;
				TSharedPtr<FS47AudioBuffer> B;
				if (FFileHelper::LoadFileToArray(Bytes, *(Dir / File))) B = S47Wa::DecodeWav(Bytes);
				if (!B.IsValid()) { UE_LOG(LogS47Audio, Warning, TEXT("S47: audio load failed %s (%s)"), *Key, *File); continue; }
				Buf.Add(Key, B);
				ByFile.Add(FPaths::GetBaseFilename(File), Key);
			}
		}
		const TSharedPtr<FJsonObject>* Phone = nullptr;
		if (J->TryGetObjectField(TEXT("phone"), Phone))
			for (const auto& P : (*Phone)->Values) PhoneKeys.Add(FString(P.Key.ToView()), P.Value->AsString());
		const TSharedPtr<FJsonObject>* Character = nullptr;
		if (J->TryGetObjectField(TEXT("character"), Character))
		{
			for (const auto& V : (*Character)->Values)
			{
				const TSharedPtr<FJsonObject>* ByText = nullptr;
				if (!V.Value->TryGetObject(ByText)) continue;
				TMap<FString, FString>& M = CharacterKeys.FindOrAdd(FString(V.Key.ToView()));
				for (const auto& T : (*ByText)->Values) M.Add(FString(T.Key.ToView()), T.Value->AsString());
			}
		}
	}
}

void US47Audio::BeginDestroy()
{
	if (Ctx.IsValid()) Ctx->Close();
	Super::BeginDestroy();
}

void US47Audio::Tick(float DeltaSeconds)
{
	if (!Ctx.IsValid()) return;
	Ctx->Tick(DeltaSeconds); // onended, setTimeout
	// main.ts: audio.listener(camera) every frame. The camera is the player's eye; Unreal to
	// three.js is the same swap of two axes as the other way (S47Three), so DirToUE turns back too.
	if (!World.IsValid()) return;
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager) return;
	const FRotator R = PC->PlayerCameraManager->GetCameraRotation();
	Listener(S47Three::ToThree(PC->PlayerCameraManager->GetCameraLocation()), S47Three::DirToUE(R.Vector()), S47Three::DirToUE(R.RotateVector(FVector::UpVector)));
}

// ---------- the prologue's names: Unreal positions, file names ----------

USoundBase* US47Audio::Find(const FString& Name)
{
	if (TObjectPtr<USoundBase>* C = Cache.Find(Name)) return C->Get();
	static const TCHAR* Folders[] = { TEXT("/Game/Licensed/Sfx"), TEXT("/Game/Audio/Sfx"), TEXT("/Game/Audio/Synth"), TEXT("/Game/Audio/Music"), TEXT("/Game/Audio/Radio") };
	for (const TCHAR* F : Folders)
	{
		if (USoundBase* S = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("%s/%s.%s"), F, *Name, *Name)))
		{
			Cache.Add(Name, S);
			return S;
		}
	}
	if (!Missing.Contains(Name))
	{
		Missing.Add(Name);
		UE_LOG(LogS47Audio, Warning, TEXT("S47: no sound named %s (run Scripts/import-audio.sh)"), *Name);
	}
	return nullptr;
}

void US47Audio::Play(const FString& Name, float Gain, const FVector& At, float Rate)
{
	// a recording the web version has goes through its graph, placed where it is (three.js terms)
	if (const FString* Key = ByFile.Find(Name))
	{
		FS47Sound O; O.Gain = Gain; O.Rate = Rate;
		if (!At.IsNearlyZero()) O.At = S47Three::ToThree(At);
		Sfx(*Key, O);
		return;
	}
	USoundBase* S = Find(Name);
	if (!S || !World.IsValid()) return;
	if (At.IsNearlyZero()) UGameplayStatics::PlaySound2D(World.Get(), S, Gain, Rate);
	else UGameplayStatics::PlaySoundAtLocation(World.Get(), S, At, Gain, Rate);
}

UAudioComponent* US47Audio::LoopComponent(const FString& Key, const FString& Name, float Gain, const FVector& At, bool b2D)
{
	Stop(Key, 0.f);
	USoundBase* S = Find(Name);
	if (!S || !World.IsValid()) return nullptr;
	UAudioComponent* C = b2D ? UGameplayStatics::SpawnSound2D(World.Get(), S, Gain, 1.f, 0.f, nullptr, true, false)
		: UGameplayStatics::SpawnSoundAtLocation(World.Get(), S, At, FRotator::ZeroRotator, Gain, 1.f, 0.f, nullptr, nullptr, false);
	if (C)
	{
		C->bAutoDestroy = false;
		AssetLoops.Add(Key, C);
	}
	return C;
}

void US47Audio::Loop(const FString& Key, const FString& Name, float Gain, const FVector& At)
{
	if (const FString* K = ByFile.Find(Name))
	{
		FS47Sound O; O.Gain = Gain;
		if (!At.IsNearlyZero()) O.At = S47Three::ToThree(At);
		LoopSfx(Key, *K, O);
		return;
	}
	LoopComponent(Key, Name, Gain, At, At.IsNearlyZero());
}

void US47Audio::Stop(const FString& Key, float Fade)
{
	// stop(key, fade): the loop fades out and its source stops a little later
	if (FS47Play* H = Loops.Find(Key))
	{
		if (!Ctx.IsValid()) return;
		FS47AudioContext::FBatch Batch(*Ctx);
		const double T = Ctx->CurrentTime();
		H->Gain->Gain.SetTargetAtTime(0, T, Fade);
		H->Src->Stop(T + Fade * 6);
		Loops.Remove(Key);
		return;
	}
	if (TObjectPtr<UAudioComponent>* C = AssetLoops.Find(Key))
	{
		if (C->Get())
		{
			if (Fade > 0.f) (*C)->FadeOut(Fade, 0.f); else (*C)->Stop();
		}
		AssetLoops.Remove(Key);
	}
}

void US47Audio::Pour(const FVector& At)
{
	PourThree(At.IsNearlyZero() ? TOptional<FVector>() : TOptional<FVector>(S47Three::ToThree(At)));
}

void US47Audio::AmRadio(bool bOn, const FVector& At)
{
	AmRadioThree(bOn ? TOptional<FVector>(S47Three::ToThree(At)) : TOptional<FVector>());
}

void US47Audio::SignalFrame(float Clarity, float Level)
{
	// the prologue's update(dt, t) passes main.ts's t
	const US47Main* M = US47Main::Get(this);
	SignalFrame(Clarity, Level, M ? M->T : 0.0);
}

void US47Audio::Spatial(const FString& Name, const FVector& At, float Ref)
{
	FS47Sound O;
	O.DestNode = Spatial(At, Ref);
	Sfx(Name, O);
}

void US47Audio::Pressure(bool bOn)
{
	if (bOn)
	{
		if (PressureBed) return;
		PressureBed = Pressure();
		if (PressureBed && Ctx.IsValid()) PressureBed.Gain->Gain.SetTargetAtTime(1, Ctx->CurrentTime(), 1.0);
	}
	else if (PressureBed)
	{
		PressureBed.Stop(0.6f);
		PressureBed = FS47Pressure();
	}
}

void US47Audio::Gasp(float When) { Gasp(TSharedPtr<FS47AudioNode>(), When); }
void US47Audio::CupDown(bool bThroughRadio) { CupDown(bThroughRadio ? RadioIn() : TSharedPtr<FS47AudioNode>()); }
void US47Audio::Shutter(const FString& Dest, float When) { Shutter(BusNamed(Dest), When); }

TSharedPtr<FS47AudioNode> US47Audio::BusNamed(const FString& Name) const
{
	if (Name == TEXT("radio")) return RadioIn();
	if (Name == TEXT("music")) return Music;
	if (Name == TEXT("amb")) return Amb;
	if (Name == TEXT("radioBus")) return RadioBus;
	if (Name == TEXT("master")) return Master;
	if (Name == TEXT("sfx")) return SfxBus;
	return nullptr;
}

FString US47Audio::NormalizeReadingText(const FString& Text)
{
	// text.replace(/\s+/g, ' ').trim()
	FString Out;
	Out.Reserve(Text.Len());
	bool bGap = false;
	for (const TCHAR C : Text)
	{
		if (FChar::IsWhitespace(C) || C == 0x00A0 || C == 0xFEFF) { bGap = true; continue; }
		if (bGap && Out.Len()) Out.AppendChar(TEXT(' '));
		bGap = false;
		Out.AppendChar(C);
	}
	return Out;
}

// ---------- self test ----------

bool US47Audio::SelfTest(TArray<FString>& Out)
{
	bool bAll = S47Wa::SelfTest(Out);
	bAll &= FS47Ambience::SelfTest(Out);
	auto Check = [&](bool bOk, const FString& What)
	{
		bAll &= bOk;
		Out.Add(FString::Printf(TEXT("%s %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), *What));
	};
	auto Near = [&](double Got, double Want, double Tol, const FString& What)
	{
		Check(FMath::Abs(Got - Want) <= Tol, FString::Printf(TEXT("%s: got %.12g want %.12g"), *What, Got, Want));
	};

	// signalVoice.ts: the 4/7 pulse group, the drive curve
	{
		using namespace S47SignalVoice;
		Check(PulseAt(0.05).bOn && PulseAt(0.05).bGroup, TEXT("pulseAt(0.05): on, in the group of four"));
		Check(!PulseAt(0.15).bOn && PulseAt(0.15).bGroup, TEXT("pulseAt(0.15): off between pulses, still in the group"));
		Check(!PulseAt(1.7).bOn && !PulseAt(1.7).bGroup, TEXT("pulseAt(1.7): the gap before the seven"));
		Check(PulseAt(2.15).bOn, TEXT("pulseAt(2.15): the first of the seven"));
		Check(!PulseAt(3.7).bGroup, TEXT("pulseAt(3.7): after the seven"));
		Check(PulseAt(5.25).bOn && PulseAt(-5.15).bOn, TEXT("pulseAt wraps every 5.2 s, also before 0"));
		const TArray<float> D = Drive(2.6);
		Check(D.Num() == 1024 && FMath::Abs(D[0] + 1.f) < 1e-6f && FMath::Abs(D[1023] - 1.f) < 1e-6f, TEXT("drive(2.6): 1024 points from -1 to 1"));
	}
	Check(NormalizeReadingText(TEXT("  Sierra\n  Motor \t Court. ")) == TEXT("Sierra Motor Court."), TEXT("normalizeReadingText collapses and trims the white space"));

	// an AudioSys with no speaker and made-up recordings
	US47Audio* A = NewObject<US47Audio>(GetTransientPackage());
	A->Unlock(false, false);
	FS47AudioContext& C = *A->Ctx;
	auto Rec = [&C](double Secs) { return TSharedPtr<FS47AudioBuffer>(C.CreateBuffer(1, int32(Secs * 24000), 24000.f)); };
	A->Buf.Add(TEXT("phone:a"), Rec(2.0));
	A->Buf.Add(TEXT("phone:b"), Rec(3.0));
	A->Buf.Add(TEXT("character:c"), Rec(1.5));
	A->Buf.Add(TEXT("click"), Rec(0.1));
	A->PhoneKeys.Add(TEXT("Sierra Motor Court."), TEXT("phone:a"));
	A->PhoneKeys.Add(TEXT("Station One. Nobody has asked me for that key in a long time."), TEXT("phone:b"));
	A->CharacterKeys.FindOrAdd(TEXT("bacall")).Add(TEXT("So you went out there."), TEXT("character:c"));
	A->bKinskiCalls = true; A->bCharacterVoices = true;
	{
		TArray<FS47PhoneCue> Nora;
		Nora.Add({ TEXT("Sierra Motor Court."), TEXT("N. VEGA") });
		FS47PhoneCue Aside; Aside.Text = TEXT("You give your name and SARO, and read her the survey number from the sleeve."); Aside.bAside = true; Aside.Pause = 3.4f;
		Nora.Add(Aside);
		Nora.Add({ TEXT("Station One.  Nobody has asked me for that key in a long time."), TEXT("N. VEGA") });
		int32 Calls = 0;
		A->OnPhoneCall = [&Calls] { ++Calls; };
		const TOptional<FS47CallTimeline> TL = A->PhoneCall(Nora);
		Check(TL.IsSet() && TL->Cues.Num() == 3, TEXT("phoneCall: a timeline when every spoken line has a recording"));
		if (TL.IsSet() && TL->Cues.Num() == 3)
		{
			Near(TL->Cues[0].At, 0.3, 1e-6, TEXT("phoneCall: the first line after the lead"));
			Near(TL->Cues[1].At, 2.75, 1e-5, TEXT("phoneCall: the aside after the line and the gap"));
			Near(TL->Cues[1].Duration, 3.4, 1e-6, TEXT("phoneCall: the aside lasts its pause"));
			Near(TL->Cues[2].At, 6.6, 1e-5, TEXT("phoneCall: the last line"));
			Near(TL->Duration, 9.8, 1e-5, TEXT("phoneCall: the call ends 0.2 s after the last line"));
		}
		Check(A->PhonePlaying() && Calls == 1, TEXT("phoneCall: the session plays, onPhoneCall told"));
		A->StopPhoneCalls();
		Check(!A->PhonePlaying(), TEXT("stopPhoneCalls ends the session"));
		Check(A->PhoneCall({ { TEXT("So you went out there."), TEXT("N. VEGA") } }).IsSet(), TEXT("phoneCall: N. VEGA in her own voice"));
		A->bCharacterVoices = false;
		Check(!A->PhoneCall({ { TEXT("So you went out there."), TEXT("N. VEGA") } }).IsSet(), TEXT("phoneCall: her voice off and no Kinski line: unset (captions)"));
		A->bCharacterVoices = true;
		Check(!A->PhoneCall({ { TEXT("Sierra Motor Court."), TEXT("N. VEGA") }, { TEXT("A line nobody recorded."), TEXT("N. VEGA") } }).IsSet(), TEXT("phoneCall: one line without a recording: unset, nothing half-spoken"));
		Check(!A->PhonePlaying(), TEXT("... and no session"));
		A->bKinskiCalls = false; A->bCharacterVoices = false;
		Check(!A->PhoneCall(Nora).IsSet(), TEXT("phoneCall: both voices off: unset"));
		A->bKinskiCalls = true; A->bCharacterVoices = true;
		A->StopPhoneCalls();
	}
	Check(A->StepPick(TEXT("dirt")).Name.StartsWith(TEXT("stepGravel")) && FMath::IsNearlyEqual(A->StepPick(TEXT("dirt")).Gain, 0.16f), TEXT("stepPick: dirt is the gravel steps at 0.16"));
	{
		// render: a beep scheduled 50 ms ahead, then the room tone held down
		const int32 Q = S47Quantum;
		auto RenderSeconds = [&C, Q](double Seconds)
		{
			const int32 N = int32(FMath::CeilToDouble(Seconds * C.SampleRate / Q)) * Q;
			TArray<float> B; B.SetNumZeroed(N * 2);
			C.Render(B.GetData(), N, 2);
			return B;
		};
		RenderSeconds(0.2); // (the clicks of the calls)
		A->Beep(1000, 0.08f, 0.12f, 0.05f, TEXT("sine"));
		const TArray<float> B = RenderSeconds(0.3);
		double Before = 0, During = 0, After = 0;
		for (int32 J = 0; J < 2300; ++J) Before = FMath::Max(Before, double(FMath::Abs(B[J * 2])));
		for (int32 J = 3000; J < 6000; ++J) During = FMath::Max(During, double(FMath::Abs(B[J * 2])));
		for (int32 J = 8500; J < B.Num() / 2; ++J) After = FMath::Max(After, double(FMath::Abs(B[J * 2])));
		Check(Before < 1e-6, FString::Printf(TEXT("beep: silent before its time (%.3g)"), Before));
		// 0.12 through the 3 kHz low-pass (+0.55 dB at 1 kHz) and the master volume 0.8
		Near(During, 0.12 * 1.0654701354713518 * 0.8, 3e-3, TEXT("beep: its level while it sounds"));
		Check(After < 1e-3, FString::Printf(TEXT("beep: silent after it (%.3g)"), After));
		A->StartRoomTone();
		RenderSeconds(0.5);
		A->HoldRoomTone(0.2f, 0.04f);
		RenderSeconds(0.2);
		Near(A->RoomToneHeld(), 0.205390357599268, 2e-3, TEXT("holdRoomTone(0.2, 0.04): after 0.2 s"));
	}
	Near(A->Dial(TEXT("555-0147")), 1.57, 1e-5, TEXT("dial: 0.25 s, 0.16 s a digit, 0.2 s for the dash"));
	Near(A->Ringback(3, 0.5f), 10.1, 1e-5, TEXT("ringback: 3.2 s a ring"));
	A->Hum(TEXT("test"), FVector(0, 1, -2));
	A->HumLevel(0.3f);
	Near(A->HumHeld(), 0.3, 1e-6, TEXT("humLevel: where the hums are held"));
	A->StopHums();
	A->Ctx->Close();
	return bAll;
}
