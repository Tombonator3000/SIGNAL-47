// -S47AudioTour=<file.wav>: every sound of core/Audio.ts in turn, rendered offline (no speaker)
// into a WAVE file, with each part's peak and RMS in the log. A check that every graph sounds,
// stays finite and is about as loud as the web version's, and something to listen to.
#include "S47Audio.h"

#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "S47Ambience.h"
#include "S47Decoder.h"
#include "S47TvProgramme.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47AudioTour, Log, All);

void US47Audio::Tour(const FString& Path)
{
	US47Audio* A = NewObject<US47Audio>(GetTransientPackage());
	A->AddToRoot();
	A->Unlock(false, true);
	FS47AudioContext& C = *A->Ctx;
	A->Listener(FVector(0, 1.62, 0), FVector(0, 0, -1), FVector(0, 1, 0));
	TArray<float> Wav;
	struct FPart { FString Name; int32 From = 0; };
	TArray<FPart> Parts;
	double Clock = 3 * 3600; // the night's clock for the desert
	TSharedPtr<FS47DecoderPlayer> Dec; // (the processor's parts: updated as the game does, every frame)
	// render Seconds of sound, ticking the game side (timeouts, onended, the night) at 60 Hz
	auto Run = [&](double Seconds, bool bNight)
	{
		const int32 Step = 800; // 1/60 s at 48 kHz
		for (int32 Done = 0; Done < int32(Seconds * C.SampleRate); Done += Step)
		{
			const int32 At = Wav.Num();
			Wav.AddZeroed(Step * 2);
			C.Render(Wav.GetData() + At, Step, 2);
			C.Tick(1.0 / 60);
			if (Dec.IsValid()) Dec->Update();
			if (bNight) { A->NightLife(1.f / 60, FVector(0, 0, 0), Clock); Clock += 1.0 / 60; }
		}
	};
	auto Part = [&](const TCHAR* Name) { Parts.Add({ Name, Wav.Num() / 2 }); };
	const FVector Right(2, 1.6, 0), Left(-3, 1.2, -1), Ahead(0, 1.2, -4);

	const bool bAll = !FParse::Param(FCommandLine::Get(), TEXT("S47AudioTourDecoder"));
	if (bAll)
	{
		Part(TEXT("beep square 1200 / sine 52 drone")); A->Beep(); A->Beep(1760, 0.18f, 0.08f, 0.3f); A->Beep(52, 2.8f, 0.09f, 0.6f, TEXT("sine")); Run(3.6, false);
		Part(TEXT("shutter")); A->Shutter(); Run(0.8, false);
		Part(TEXT("servo 1.8 s, right")); A->Servo(1.8f, Right); Run(2.6, false);
		Part(TEXT("signature 4/7 clicks")); A->Signature(0.45f); Run(3.2, false);
		Part(TEXT("dial and ringback")); { const float D = A->Dial(TEXT("555-0147")); A->Ringback(2, D); } Run(8.5, false);
		Part(TEXT("call line 3 s")); A->CallLine(3); Run(3.5, false);
		Part(TEXT("phone call (Nora, recorded)"));
		{
			TArray<FS47PhoneCue> Lines;
			Lines.Add({ TEXT("Sierra Motor Court."), TEXT("N. VEGA") });
			FS47PhoneCue Aside; Aside.Text = TEXT("You give your name and SARO, and read her the survey number from the sleeve."); Aside.bAside = true; Aside.Pause = 1.0f;
			Lines.Add(Aside);
			Lines.Add({ TEXT("Station One. Nobody has asked me for that key in a long time."), TEXT("N. VEGA") });
			const TOptional<FS47CallTimeline> TL = A->PhoneCall(Lines);
			UE_LOG(LogS47AudioTour, Display, TEXT("S47: tour: phone call %s"), TL ? *FString::Printf(TEXT("%.2f s, %d cues"), TL->Duration, TL->Cues.Num()) : TEXT("unset (no recordings: captions)"));
			Run(TL ? TL->Duration + 0.5 : 1.0, false);
		}
		Part(TEXT("phone call (Ward, Kinski)"));
		{
			TArray<FS47PhoneCue> Lines;
			Lines.Add({ TEXT("That is enough to hold the morning series. It is not enough to tell us why it happened."), TEXT("WARD") });
			const TOptional<FS47CallTimeline> TL = A->PhoneCall(Lines);
			Run(TL ? TL->Duration + 0.5 : 1.0, false);
		}
		Part(TEXT("the call from the future")); Run(A->FutureCall() + 0.6, false);
		Part(TEXT("boom 1.25")); A->Boom(1.25f); Run(3.2, false);
		Part(TEXT("thunder")); A->Thunder(0.8f); Run(6.0, false);
		Part(TEXT("relay clack in the room, through the wall")); A->RelayClack(Right, false); Run(0.8, false); A->RelayClack(Left, true); Run(0.8, false);
		Part(TEXT("contactor, power down, power up")); A->Contactor(Ahead); Run(1.0, false); A->PowerDown(Ahead); Run(3.0, false); A->PowerUp(Ahead); Run(2.0, false);
		Part(TEXT("telex motor and type bars"));
		A->TelexMotor(Left);
		for (int32 I = 0; I < 24; ++I) { A->TypeBar(Left, I == 23); Run(0.11, false); }
		A->TelexMotor({}); Run(1.2, false);
		Part(TEXT("station radio with a voice")); A->Radio(Ahead); Run(1.5, false); A->RadioVoice(2.5f); Run(3.5, false); A->Radio({}); Run(1.4, false);
		Part(TEXT("AM radio, jammed, back")); A->AmRadioThree(Right); Run(5.0, false); A->AmRadioJam(true); Run(3.0, false); A->AmRadioJam(false); Run(3.0, false); A->AmRadioThree({}); Run(1.0, false);
		Part(TEXT("cab radio: talk, voices, cup, carrier, hiss"));
		A->CabRadio(TEXT("talk")); Run(4.0, false); A->CabVoice(2.4f, 520); Run(3.0, false); A->CabVoice(2.0f, 820); Run(2.6, false);
		A->CupDown(A->RadioIn()); Run(0.6, false);
		{ FS47Sound O; O.Gain = 1.2f; O.Dest = TEXT("radio"); const FS47Play H = A->Sfx(TEXT("phoneRing"), O); if (H) H.Src->Stop(C.CurrentTime() + 2.6); } Run(3.0, false);
		A->CabRadio(TEXT("carrier")); Run(2.0, false); A->CabRadio(TEXT("hiss")); Run(2.0, false); A->CabLevel(0.3f); Run(1.0, false); A->CabRadio(TEXT("")); Run(1.0, false);
		Part(TEXT("receiver carrier"));
		A->StartCarrier();
		for (int32 I = 0; I <= 120; ++I) { const float Q = I / 120.f; A->SetCarrier(0.06f * Q * Q, (Q - 1) * 0.03f, 0.03f * (1 - Q) + 0.008f); Run(1.0 / 30, false); }
		A->SetCarrier(0, 0, 0); Run(0.5, false);
		Part(TEXT("the anomaly: static to the locked signal"));
		{
			double T = 0;
			for (int32 I = 0; I < 60 * 14; ++I) { A->SignalFrame(FMath::Min(1.0, 0.02 + I / (60.0 * 9)), 0.9f, T); T += 1.0 / 60; Run(1.0 / 60, false); }
			A->SignalOff(); Run(1.2, false);
		}
		Part(TEXT("motors")); A->StartMotors(); Run(4.0, false); A->StopMotors(); Run(3.0, false);
		Part(TEXT("hums")); A->Hum(TEXT("s03"), Right, 0.07f, 60); A->Hum(TEXT("safelight"), Left, 0.025f, 120); Run(4.0, false); A->HumLevel(0); Run(1.0, false); A->HumLevel(1); Run(1.5, false); A->StopHums(); Run(1.5, false);
		Part(TEXT("pour (bought or made), gasp, cup")); A->PourThree(Ahead); Run(2.2, false); A->Gasp(); Run(0.8, false); A->CupDown(); Run(0.6, false);
		Part(TEXT("steps on each surface"));
		for (const TCHAR* S : { TEXT("tile"), TEXT("concrete"), TEXT("dirt"), TEXT("wood"), TEXT("carpet") }) for (int32 I = 0; I < 4; ++I) { A->Step(S); Run(0.5, false); }
		Part(TEXT("tv hiss")); A->TvHiss(Left); Run(2.0, false); A->TvHiss({}); Run(0.5, false);
		{
			// the late broadcast's sound (Scripts/prepare-tv.py, when staged): a set's speaker 2 m away at the
			// office set's level, then through the wall, then stepping back under a voice (VoicesDuck)
			FS47TvProgramme Tv;
			if (Tv.HasSound())
			{
				const FVector Set(0.6, 1.2, -1.9);
				Part(TEXT("tv broadcast at 2 m")); Tv.Time = 30.0; Tv.Update(0, A, Set, 0.32, false);
				UE_LOG(LogS47AudioTour, Display, TEXT("S47: tour: tv broadcast from %.3f s of the programme, %.4f s into the tour"), 30.0, C.CurrentTime());
				Run(8.0, false);
				Part(TEXT("tv broadcast through the wall")); Tv.Update(0, A, Set, 0.32, true); Run(4.0, false);
				Part(TEXT("tv broadcast under a voice")); Tv.Update(0, A, Set, 0.32 * 0.25, false); Run(4.0, false);
				UE_LOG(LogS47AudioTour, Display, TEXT("S47: tour: tv broadcast %s"), *Tv.Describe());
				Tv.Silence(); Run(0.5, false);
			}
		}
		Part(TEXT("room tone, held down and back")); A->StartRoomTone(); Run(4.0, false); A->HoldRoomTone(0, 0.09f); Run(1.5, false); A->HoldRoomTone(1, 0.35f); Run(2.0, false);
		Part(TEXT("the night in the yard at 03:00"));
		A->StartNight(); A->SetSpace(TEXT("yard")); Run(25.0, true);
		A->Far(TEXT("owl"), FVector::ZeroVector, 0.26f); A->Far(TEXT("dog0"), FVector::ZeroVector, 0.18f); Run(4.0, true);
		A->Night->Hush(3, 0.08); Run(6.0, true);
		Part(TEXT("the yard holds its breath (47 s, shortened to 12)")); A->Night->Breath(12); Run(14.0, true);
		Part(TEXT("indoors, the desert through the wall")); A->SetSpace(TEXT("room")); Run(6.0, true);
		Part(TEXT("dawn, birds")); Clock = 5.6 * 3600; A->SetSpace(TEXT("yard")); Run(20.0, true);
		Part(TEXT("silence, back; duck all")); A->Silence(true); Run(1.5, true); A->Silence(false); Run(2.0, true); A->DuckAll(1.5f); Run(4.0, true);
		{ FS47Pressure P = A->Pressure(); if (P) { Part(TEXT("pressure bed (bought)")); P.Gain->Gain.SetTargetAtTime(1.0, C.CurrentTime(), 1.0); Run(5.0, false); P.Stop(0.5f); Run(3.0, false); } }
	}
	// the signal processor (core/decoder.ts, S47Decoder.h) at the panel's settings: RAW and HARMONIZED
	// at the default TIME (2) and at 1 and 4, then the answer with TAPE A; its bands logged below.
	// -S47AudioTourDecoder renders only these.
	// Each twice: as the web version sounds (FS47DecoderPlayer::bWebSound), then as this one does.
	auto Processor = [&](const FString& Name, const TCHAR* Mode, double Stretch, double Secs)
	{
		Part(*Name);
		Dec = MakeShared<FS47DecoderPlayer>(A->Ctx.ToSharedRef(), A->SfxBus, FVector(0, 1.2, -1.6), A->NoiseBuffer());
		FS47DecoderSet P; P.Mode = FString(Mode); P.Stretch = Stretch;
		Dec->Start(P);
		Run(Secs, false);
		Dec->Stop(); Run(1.5, false);
		Dec.Reset();
	};
	for (const bool bWeb : { true, false })
	{
		FS47DecoderPlayer::bWebSound = bWeb;
		const FString Was = bWeb ? TEXT(" [web]") : TEXT("");
		Processor(TEXT("processor RAW, time 2 (default)") + Was, TEXT("raw"), 2, 12);
		Processor(TEXT("processor RAW, time 1") + Was, TEXT("raw"), 1, 12);
		Processor(TEXT("processor RAW, time 4") + Was, TEXT("raw"), 4, 12);
		Processor(TEXT("processor HARMONIZED, time 2 (default)") + Was, TEXT("harmonized"), 2, 12);
		Processor(TEXT("processor HARMONIZED, time 1") + Was, TEXT("harmonized"), 1, 12);
		Processor(TEXT("processor HARMONIZED, time 4") + Was, TEXT("harmonized"), 4, 12);
		Part(*(TEXT("processor the answer and TAPE A") + Was));
		Dec = MakeShared<FS47DecoderPlayer>(A->Ctx.ToSharedRef(), A->SfxBus, FVector(0, 1.2, -1.6), A->NoiseBuffer());
		TArray<double> Seven;
		for (const int32 K : { 7, 5, 3, 2, 1, 4, 6 }) Seven.Add(FS47DecoderPlayer::SevenDegree(K - 1));
		FS47AnswerTimes Tm; Tm.K = 3;
		Dec->Phrase(Seven, Tm);
		Run(Tm.End() + 1.0, false);
		Dec.Reset();
	}
	FS47DecoderPlayer::bWebSound = false;
	Parts.Add({ TEXT("end"), Wav.Num() / 2 });

	// where a part's energy sits: the share of it in each band (a Hann-windowed FFT over 4096 frames
	// of the mono sum), the spectral centroid, and the share above 3 kHz
	auto Bands = [&Wav, &C](int32 F0, int32 F1) -> FString
	{
		const int32 N = 4096;
		const double Edges[] = { 0, 150, 300, 600, 1200, 2400, 3000, 4800, 9600, 24000 };
		constexpr int32 NB = UE_ARRAY_COUNT(Edges) - 1;
		double E[NB] = {}, Total = 0, Moment = 0, Above3k = 0;
		TArray<double> Re, Im;
		for (int32 W0 = F0; W0 + N <= F1; W0 += N)
		{
			Re.Init(0.0, N); Im.Init(0.0, N);
			for (int32 J = 0; J < N; ++J) Re[J] = 0.5 * (Wav[(W0 + J) * 2] + Wav[(W0 + J) * 2 + 1]) * (0.5 - 0.5 * FMath::Cos(2 * PI * J / N));
			// in place, radix 2
			for (int32 J = 1, K = 0; J < N; ++J)
			{
				int32 Bit = N >> 1;
				for (; K & Bit; Bit >>= 1) K ^= Bit;
				K ^= Bit;
				if (J < K) { Swap(Re[J], Re[K]); Swap(Im[J], Im[K]); }
			}
			for (int32 Len = 2; Len <= N; Len <<= 1)
			{
				const double Ang = -2 * PI / Len;
				for (int32 J = 0; J < N; J += Len) for (int32 K = 0; K < Len / 2; ++K)
				{
					const double Wr = FMath::Cos(Ang * K), Wi = FMath::Sin(Ang * K);
					const double Ur = Re[J + K], Ui = Im[J + K];
					const double Vr = Re[J + K + Len / 2] * Wr - Im[J + K + Len / 2] * Wi, Vi = Re[J + K + Len / 2] * Wi + Im[J + K + Len / 2] * Wr;
					Re[J + K] = Ur + Vr; Im[J + K] = Ui + Vi; Re[J + K + Len / 2] = Ur - Vr; Im[J + K + Len / 2] = Ui - Vi;
				}
			}
			for (int32 K = 1; K < N / 2; ++K)
			{
				const double F = K * C.SampleRate / N, P = Re[K] * Re[K] + Im[K] * Im[K];
				Total += P; Moment += P * F;
				if (F >= 3000) Above3k += P;
				for (int32 B = 0; B < NB; ++B) if (F >= Edges[B] && F < Edges[B + 1]) { E[B] += P; break; }
			}
		}
		FString Out = TEXT("bands");
		for (int32 B = 0; B < NB; ++B) Out += FString::Printf(TEXT(" %g-%gk %4.1f%%"), Edges[B] / 1000, Edges[B + 1] / 1000, Total > 0 ? 100 * E[B] / Total : 0.0);
		Out += FString::Printf(TEXT("  centroid %.0f Hz, above 3 kHz %.2f%% (%.1f dB under the whole)"), Total > 0 ? Moment / Total : 0.0, Total > 0 ? 100 * Above3k / Total : 0.0,
			Total > 0 && Above3k > 0 ? -10 * FMath::LogX(10.0, Above3k / Total) : 0.0);
		return Out;
	};
	// the parts' levels
	bool bFinite = true;
	for (const float X : Wav) if (!FMath::IsFinite(X)) { bFinite = false; break; }
	for (int32 I = 0; I + 1 < Parts.Num(); ++I)
	{
		double Peak = 0, Sum = 0;
		const int32 F0 = Parts[I].From, F1 = Parts[I + 1].From;
		for (int32 F = F0; F < F1; ++F) for (int32 Ch = 0; Ch < 2; ++Ch) { const double X = Wav[F * 2 + Ch]; Peak = FMath::Max(Peak, FMath::Abs(X)); Sum += X * X; }
		const double Rms = F1 > F0 ? FMath::Sqrt(Sum / (2.0 * (F1 - F0))) : 0;
		UE_LOG(LogS47AudioTour, Display, TEXT("S47: tour %5.1f s  peak %6.3f  rms %6.4f  %s"), F0 / double(C.SampleRate), Peak, Rms, *Parts[I].Name);
		if (Parts[I].Name.StartsWith(TEXT("processor"))) UE_LOG(LogS47AudioTour, Display, TEXT("S47: tour        %s"), *Bands(F0, F1));
	}
	// a 16-bit stereo WAVE file
	TArray<uint8> Out;
	const int32 Frames = Wav.Num() / 2, Rate = int32(C.SampleRate);
	auto Put32 = [&Out](uint32 V) { for (int32 I = 0; I < 4; ++I) Out.Add(uint8(V >> (8 * I))); };
	auto Put16 = [&Out](uint32 V) { Out.Add(uint8(V)); Out.Add(uint8(V >> 8)); };
	auto Tag = [&Out](const char* T) { for (int32 I = 0; I < 4; ++I) Out.Add(uint8(T[I])); };
	Tag("RIFF"); Put32(36 + Frames * 4); Tag("WAVE");
	Tag("fmt "); Put32(16); Put16(1); Put16(2); Put32(Rate); Put32(Rate * 4); Put16(4); Put16(16);
	Tag("data"); Put32(Frames * 4);
	Out.Reserve(Out.Num() + Frames * 4);
	for (const float X : Wav) Put16(uint32(uint16(int16(FMath::Clamp(FMath::IsFinite(X) ? X : 0.f, -1.f, 1.f) * 32767.f))));
	const bool bSaved = FFileHelper::SaveArrayToFile(Out, *Path);
	UE_LOG(LogS47AudioTour, Display, TEXT("S47: audio tour %s: %.1f s, %d recordings, %s, written to %s"), bFinite ? TEXT("OK") : TEXT("FAILED (not finite)"),
		Frames / double(Rate), A->Buf.Num(), bFinite ? TEXT("all samples finite") : TEXT("NaN or Inf in the output"), bSaved ? *Path : TEXT("(not written)"));
	A->Ctx->Close();
	A->RemoveFromRoot();
}

// -S47AudioCompare=<dir>: a set of graphs whose input is known (oscillators, ramps, a seeded noise),
// rendered offline into <dir>/<case>.f32 (float, stereo, 2 s at 48 kHz), to hold against the same
// graphs rendered by a browser's OfflineAudioContext (Tools/webaudio-compare).
namespace
{
	TSharedRef<FS47AudioBuffer> LcgNoise(FS47AudioContext& Ctx, double Seconds, uint32 Seed)
	{
		TSharedRef<FS47AudioBuffer> B = Ctx.CreateBuffer(1, int32(Ctx.SampleRate * Seconds), Ctx.SampleRate);
		float* D = B->GetChannelData(0);
		uint32 X = Seed;
		for (int32 I = 0; I < B->Length(); ++I) { X = X * 1664525u + 1013904223u; D[I] = float(X / 4294967296.0 * 2 - 1); }
		return B;
	}
}

void US47Audio::Compare(const FString& Dir)
{
	TMap<FString, TFunction<void(FS47AudioContext&)>> Cases;
	Cases.Add(TEXT("envelope"), [](FS47AudioContext& C)
	{
		auto O = C.CreateOscillator(); O->Frequency.SetValue(440);
		auto G = C.CreateGain(); G->Gain.SetValue(0);
		G->Gain.SetValueAtTime(0, 0.1); G->Gain.LinearRampToValueAtTime(0.8, 0.3); G->Gain.ExponentialRampToValueAtTime(0.01, 1.0); G->Gain.SetTargetAtTime(0.5, 1.2, 0.1);
		G->Gain.SetValueAtTime(0.5, 1.6); G->Gain.LinearRampToValueAtTime(0, 1.9);
		O->Connect(G); G->Connect(C.Destination); O->Start(0.05); O->Stop(1.95);
	});
	Cases.Add(TEXT("beep"), [](FS47AudioContext& C)
	{
		const double T = 0.1, Dur = 0.5, Gain = 0.3;
		auto O = C.CreateOscillator(); O->SetType(TEXT("square")); O->Frequency.SetValue(1200);
		auto G = C.CreateGain(); G->Gain.SetValueAtTime(0, T); G->Gain.LinearRampToValueAtTime(Gain, T + 0.005); G->Gain.SetValueAtTime(Gain, T + Dur); G->Gain.LinearRampToValueAtTime(0, T + Dur + 0.02);
		auto Lp = C.CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(3000);
		O->Connect(Lp); Lp->Connect(G); G->Connect(C.Destination); O->Start(T); O->Stop(T + Dur + 0.05);
	});
	Cases.Add(TEXT("sweep"), [](FS47AudioContext& C)
	{
		auto O = C.CreateOscillator(); O->SetType(TEXT("sawtooth")); O->Frequency.SetValue(110);
		auto Lp = C.CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Q.SetValue(6); Lp->Frequency.SetValueAtTime(4000, 0); Lp->Frequency.ExponentialRampToValueAtTime(200, 1.9);
		auto G = C.CreateGain(); G->Gain.SetValue(0.3);
		O->Connect(Lp); Lp->Connect(G); G->Connect(C.Destination); O->Start(0);
	});
	Cases.Add(TEXT("panner"), [](FS47AudioContext& C)
	{
		auto O = C.CreateOscillator(); O->Frequency.SetValue(500);
		auto P = C.CreatePanner(); P->DistanceModel = ES47DistanceModel::Inverse; P->RefDistance = 1.2; P->RolloffFactor = 1.2; P->MaxDistance = 60;
		P->SetPosition(2, 0, -1);
		O->Connect(P); P->Connect(C.Destination); O->Start(0);
	});
	Cases.Add(TEXT("compressor"), [](FS47AudioContext& C)
	{
		auto O = C.CreateOscillator(); O->Frequency.SetValue(200);
		auto G = C.CreateGain(); G->Gain.SetValueAtTime(0.1, 0); G->Gain.SetValueAtTime(1.0, 0.5); G->Gain.SetValueAtTime(0.3, 1.0); G->Gain.SetValueAtTime(0.02, 1.5);
		auto Comp = C.CreateDynamicsCompressor(); Comp->Threshold.SetValue(-16); Comp->Ratio.SetValue(5); Comp->Attack.SetValue(0.003); Comp->Release.SetValue(0.2);
		O->Connect(G); G->Connect(Comp); Comp->Connect(C.Destination); O->Start(0);
	});
	for (const TCHAR* Name : { TEXT("shaper"), TEXT("shaper2x") })
	{
		Cases.Add(Name, [](FS47AudioContext& C)
		{
			auto O = C.CreateOscillator(); O->Frequency.SetValue(300);
			auto G = C.CreateGain(); G->Gain.SetValue(0.8);
			auto S = C.CreateWaveShaper();
			const int32 N = 1024;
			for (int32 I = 0; I < N; ++I) { const double X = double(I) / (N - 1) * 2 - 1; S->Curve.Add(float(FMath::Tanh(2.6 * X) / FMath::Tanh(2.6))); }
			O->Connect(G); G->Connect(S); S->Connect(C.Destination); O->Start(0);
		});
	}
	Cases.Add(TEXT("comb"), [](FS47AudioContext& C)
	{
		auto B = C.CreateBuffer(1, 8, 48000.f); B->GetChannelData(0)[0] = 1;
		auto Src = C.CreateBufferSource(); Src->Buffer = B;
		auto D = C.CreateDelay(0.05); D->DelayTime.SetValue(0.0027);
		auto Fb = C.CreateGain(); Fb->Gain.SetValue(0.86);
		Src->Connect(D); D->Connect(Fb); Fb->Connect(D); D->Connect(C.Destination); Src->Start(0.01);
	});
	Cases.Add(TEXT("horn"), [](FS47AudioContext& C)
	{
		auto Horn = C.CreateOscillator(); Horn->SetType(TEXT("sawtooth")); Horn->Frequency.SetValue(124);
		auto P1 = C.CreateOscillator(); P1->Frequency.SetValue(0.73);
		auto P1g = C.CreateGain(); P1g->Gain.SetValue(16); P1->Connect(P1g); P1g->Connect(Horn->Frequency);
		auto Mute = C.CreateBiquadFilter(); Mute->SetType(TEXT("lowpass")); Mute->Frequency.SetValue(800); Mute->Q.SetValue(6.5);
		auto Syl = C.CreateOscillator(); Syl->SetType(TEXT("triangle")); Syl->Frequency.SetValue(4.3);
		auto Sw = C.CreateGain(); Sw->Gain.SetValue(520); Syl->Connect(Sw); Sw->Connect(Mute->Frequency);
		auto Sg = C.CreateGain(); Sg->Gain.SetValue(0.5); Syl->Connect(Sg);
		auto Chop = C.CreateGain(); Chop->Gain.SetValue(0.5); Sg->Connect(Chop->Gain);
		Horn->Connect(Mute); Mute->Connect(Chop); Chop->Connect(C.Destination);
		Horn->Start(0); P1->Start(0); Syl->Start(0);
	});
	Cases.Add(TEXT("noisebands"), [](FS47AudioContext& C)
	{
		auto N = C.CreateBufferSource(); N->Buffer = LcgNoise(C, 2, 47);
		auto Bp = C.CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(1800); Bp->Q.SetValue(0.6);
		auto Pk = C.CreateBiquadFilter(); Pk->SetType(TEXT("peaking")); Pk->Frequency.SetValue(1352); Pk->Gain.SetValue(8);
		auto Hp = C.CreateBiquadFilter(); Hp->SetType(TEXT("highpass")); Hp->Frequency.SetValue(320);
		N->Connect(Bp); Bp->Connect(Pk); Pk->Connect(Hp); Hp->Connect(C.Destination); N->Start(0);
	});
	Cases.Add(TEXT("playback"), [](FS47AudioContext& C)
	{
		auto B = C.CreateBuffer(1, 24000, 24000.f); float* D = B->GetChannelData(0);
		for (int32 I = 0; I < 24000; ++I) D[I] = float(FMath::Sin(2 * 3.14159265358979323846 * 300 * I / 24000.0) * (I / 24000.0));
		auto S = C.CreateBufferSource(); S->Buffer = B; S->PlaybackRate.SetValue(0.62); S->bLoop = true;
		auto Lp = C.CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(1500);
		S->Connect(Lp); Lp->Connect(C.Destination); S->Start(0.02, 0.1);
	});
	Cases.Add(TEXT("stereopan"), [](FS47AudioContext& C)
	{
		auto O = C.CreateOscillator(); O->Frequency.SetValue(330);
		auto P = C.CreateStereoPanner(); P->Pan.SetValueAtTime(-0.9, 0); P->Pan.LinearRampToValueAtTime(0.9, 1.9);
		O->Connect(P); P->Connect(C.Destination); O->Start(0);
	});
	Cases.Add(TEXT("cancel"), [](FS47AudioContext& C)
	{
		auto O = C.CreateOscillator(); O->Frequency.SetValue(220);
		auto G = C.CreateGain(); G->Gain.SetValue(0);
		G->Gain.SetValueAtTime(0, 0); G->Gain.LinearRampToValueAtTime(1, 1.0); G->Gain.CancelAndHoldAtTime(0.6); G->Gain.SetTargetAtTime(0.1, 0.6, 0.2);
		O->Connect(G); G->Connect(C.Destination); O->Start(0);
	});
	for (const TPair<FString, TFunction<void(FS47AudioContext&)>>& Case : Cases)
	{
		TSharedRef<FS47AudioContext> C = FS47AudioContext::Make(48000.f);
		{
			FS47AudioContext::FBatch Batch(*C);
			Case.Value(*C);
		}
		TArray<float> Out;
		Out.SetNumZeroed(96000 * 2);
		C->Render(Out.GetData(), 96000, 2);
		C->Close();
		TArray<uint8> Bytes;
		Bytes.Append(reinterpret_cast<const uint8*>(Out.GetData()), Out.Num() * sizeof(float));
		FFileHelper::SaveArrayToFile(Bytes, *(Dir / Case.Key + TEXT(".f32")));
	}
	UE_LOG(LogS47AudioTour, Display, TEXT("S47: audio compare: %d cases written to %s"), Cases.Num(), *Dir);
}
