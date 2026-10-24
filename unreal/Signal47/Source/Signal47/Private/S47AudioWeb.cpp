// web/src/core/Audio.ts (AudioSys) by its own names, in the file's order, on the Web Audio of
// S47WebAudio.h: the same graphs, the same numbers. Positions are three.js metres. Each function
// holds the context's lock while it builds its graph, as one JavaScript task would.
#include "S47Audio.h"

#include "Sound/SoundBase.h"

#include "S47Ambience.h"
#include "S47SignalVoice.h"

namespace
{
	using FBatch = FS47AudioContext::FBatch;
	constexpr double Pi = 3.14159265358979323846;

	/** A bought sound's level against the one it stands in for, at the same call: the files are cut
	 *  to match, but a click that would clip at the old level, or a crackle, is raised here. */
	float LicGain(const FString& Name)
	{
		static const TMap<FString, float> LIC_GAIN = { { TEXT("lic:rack_switch"), 1.6f }, { TEXT("lic:small_switch"), 2.2f }, { TEXT("lic:radio_crackle_long"), 4.2f }, { TEXT("lic:radio_crackle_short"), 3.f } };
		const float* G = LIC_GAIN.Find(Name);
		return G ? *G : 1.f;
	}
	/** The bought static against white noise, through the radios' band-pass: x5.8 to x6.4 for the same
	 *  RMS, x5 to x5.5 for the same loudness (it is brighter inside the band). */
	constexpr double STATIC_MAKEUP = 5.5;

	// What the player walks on, and how a step on it sounds: which recordings, how loud, the
	// pitch, and a low-pass to take the edge off. Quieter than before and never the same
	// recording twice running; the desert's dirt and gravel are Kenney's snow steps a little
	// deeper, which crunch like gravel.
	struct FSteps { TArray<FString> Names; float Gain, Rate, Lp; };
	TArray<FString> Five(const TCHAR* Kind) { TArray<FString> N; for (int32 I = 0; I < 5; ++I) N.Add(FString::Printf(TEXT("step%s%d"), Kind, I)); return N; }
	const FSteps& STEPS(const FString& Surface)
	{
		static const TMap<FString, FSteps> M = {
			{ TEXT("tile"), { Five(TEXT("Concrete")), 0.15f, 1.08f, 6000.f } },
			{ TEXT("concrete"), { Five(TEXT("Concrete")), 0.18f, 0.96f, 5000.f } },
			{ TEXT("dirt"), { Five(TEXT("Gravel")), 0.16f, 0.88f, 3800.f } },
			{ TEXT("wood"), { Five(TEXT("Wood")), 0.17f, 1.0f, 5000.f } },
			{ TEXT("carpet"), { Five(TEXT("Carpet")), 0.15f, 0.95f, 2200.f } },
		};
		const FSteps* S = M.Find(Surface);
		return S ? *S : M[TEXT("concrete")];
	}
	TSharedPtr<FS47AudioNode> Node(const TSharedPtr<FS47AudioNode>& A, const TSharedPtr<FS47AudioNode>& Or) { return A.IsValid() ? A : Or; }
}

bool US47Audio::BoughtSfx() const
{
	for (const TPair<FString, TSharedPtr<FS47AudioBuffer>>& B : Buf) if (B.Key.StartsWith(TEXT("lic:"))) return true;
	return false;
}

TSharedPtr<FS47AudioBuffer> US47Audio::CharacterBuffer(const FString& Voice, const FString& Text) const
{
	if (!bCharacterVoices) return nullptr;
	const TMap<FString, FString>* ByText = CharacterKeys.Find(Voice);
	const FString* Key = ByText ? ByText->Find(NormalizeReadingText(Text)) : nullptr;
	return Key ? Buf.FindRef(*Key) : nullptr;
}

void US47Audio::SetVolume(float V) { Volume = V; if (Master.IsValid()) Master->Gain.SetValue(V); }
void US47Audio::SetMusic(float V) { MusicLevel = V; if (Music.IsValid()) Music->Gain.SetValue(V); }
void US47Audio::SetRadio(float V) { RadioLevel = V; if (RadioBus.IsValid()) RadioBus->Gain.SetValue(V); }

void US47Audio::Listener(const FVector& P, const FVector& F, const FVector& U)
{
	if (!Ctx.IsValid() || !Ctx->Listener.IsValid()) return;
	FBatch Batch(*Ctx);
	FS47AudioListener& L = *Ctx->Listener;
	const double T = Ctx->CurrentTime();
	L.PositionX.SetTargetAtTime(P.X, T, 0.02); L.PositionY.SetTargetAtTime(P.Y, T, 0.02); L.PositionZ.SetTargetAtTime(P.Z, T, 0.02);
	L.ForwardX.SetTargetAtTime(F.X, T, 0.02); L.ForwardY.SetTargetAtTime(F.Y, T, 0.02); L.ForwardZ.SetTargetAtTime(F.Z, T, 0.02);
	L.UpX.SetTargetAtTime(U.X, T, 0.02); L.UpY.SetTargetAtTime(U.Y, T, 0.02); L.UpZ.SetTargetAtTime(U.Z, T, 0.02);
}

TSharedRef<FS47PannerNode> US47Audio::Panner(const FVector& Pos, double Ref, TSharedPtr<FS47AudioNode> Dest)
{
	auto P = Ctx->CreatePanner();
	P->PanningModel = TEXT("equalpower"); P->DistanceModel = ES47DistanceModel::Inverse; P->RefDistance = Ref; P->RolloffFactor = 1.2; P->MaxDistance = 60;
	P->SetPosition(Pos.X, Pos.Y, Pos.Z);
	P->Connect(Node(Dest, SfxBus));
	return P;
}

FS47Play US47Audio::Sfx(const FString& Name, const FS47Sound& O)
{
	const TSharedPtr<FS47AudioBuffer> B = Buf.FindRef(Name);
	if (!Ctx.IsValid() || !B.IsValid()) return {};
	FBatch Batch(*Ctx);
	auto Src = Ctx->CreateBufferSource(); Src->Buffer = B; Src->PlaybackRate.SetValue(O.Rate); Src->bLoop = O.bLoop;
	auto G = Ctx->CreateGain(); G->Gain.SetValue(O.Gain * LicGain(Name));
	TSharedPtr<FS47BiquadFilterNode> Filter;
	if (O.Lp > 0.f) { Filter = Ctx->CreateBiquadFilter(); Filter->SetType(TEXT("lowpass")); Filter->Frequency.SetValue(O.Lp); Src->Connect(Filter); Filter->Connect(G); }
	else Src->Connect(G);
	const TSharedPtr<FS47AudioNode> Dest = Node(O.DestNode, BusNamed(O.Dest));
	G->Connect(Dest.IsValid() ? Dest : O.At ? TSharedPtr<FS47AudioNode>(Panner(*O.At)) : TSharedPtr<FS47AudioNode>(SfxBus));
	Src->Start(Ctx->CurrentTime() + O.When, O.From, O.Dur ? *O.Dur : -1.0);
	return { Src, G, Filter };
}

FString US47Audio::Or(const FString& Name, const FString& Old) const
{
	const FString Key = TEXT("lic:") + Name;
	return Buf.Contains(Key) ? Key : Old;
}

TSharedPtr<FS47AudioBuffer> US47Audio::Lic(const FString& Name) const { return Buf.FindRef(TEXT("lic:") + Name); }

FS47Play US47Audio::LoopSfx(const FString& Key, const FString& Name, const FS47Sound& O)
{
	Stop(Key);
	FS47Sound L = O;
	L.bLoop = true;
	const FS47Play H = Sfx(Name, L);
	if (H) Loops.Add(Key, H);
	return H;
}

// ---------- Synth ----------

TSharedRef<FS47BufferSourceNode> US47Audio::NoiseSrc(bool bBrown, bool bLoop)
{
	auto S = Ctx->CreateBufferSource(); S->Buffer = bBrown ? Brown : Noise; S->bLoop = bLoop;
	return S;
}

TOptional<FS47StaticSrc> US47Audio::StaticSrc()
{
	// The static of an AM set at night, looping and already started: the bought hiss with its
	// random crackle when it is there (from a random point, so two radios are not in step),
	// otherwise white noise. `out` has the level the white noise had through the radios' band-pass.
	if (!Ctx.IsValid() || !Noise.IsValid()) return {};
	FBatch Batch(*Ctx);
	const TSharedPtr<FS47AudioBuffer> B = Lic(TEXT("radio_static"));
	auto Src = Ctx->CreateBufferSource(); Src->Buffer = B.IsValid() ? B : Noise; Src->bLoop = true;
	if (!B.IsValid()) { Src->Start(); return FS47StaticSrc{ Src, Src }; }
	auto G = Ctx->CreateGain(); G->Gain.SetValue(STATIC_MAKEUP); Src->Connect(G);
	Src->Start(0, FMath::FRand() * B->Duration());
	return FS47StaticSrc{ Src, G };
}

FS47Pressure US47Audio::Pressure()
{
	// The deep pressure of the time away (story/abduction.ts) and of THE EVENT (story/Chapter6.ts),
	// the same in both so it is known again: a bought low bed, looping, not placed, at gain 0 for
	// the caller to raise. Unset without the file (then the synth tone is all there is). stop(fade):
	// 0 cuts it at once (before the sound is suspended, so it is not there on waking).
	const TSharedPtr<FS47AudioBuffer> B = Lic(TEXT("pressure"));
	if (!Ctx.IsValid() || !B.IsValid()) return {};
	FBatch Batch(*Ctx);
	auto Src = Ctx->CreateBufferSource(); Src->Buffer = B; Src->bLoop = true;
	auto Gain = Ctx->CreateGain(); Gain->Gain.SetValue(0); Src->Connect(Gain); Gain->Connect(SfxBus); Src->Start();
	FS47Pressure P;
	P.Gain = Gain;
	const TSharedRef<FS47AudioContext> C = Ctx.ToSharedRef();
	TSharedPtr<FS47BufferSourceNode> S = Src;
	TSharedPtr<FS47GainNode> G = Gain;
	P.StopFn = [C, S, G](float Fade)
	{
		FBatch Inner(*C);
		const double T = C->CurrentTime();
		if (Fade <= 0) { S->Stop(); G->Disconnect(); return; }
		G->Gain.CancelScheduledValues(T); G->Gain.SetTargetAtTime(0, T, Fade); S->Stop(T + Fade * 6);
	};
	return P;
}

void US47Audio::StartRoomTone()
{
	if (!Ctx.IsValid() || RoomTone.IsValid()) return;
	FBatch Batch(*Ctx);
	auto G = Ctx->CreateGain(); G->Gain.SetValue(0.0);
	ToneHold = Ctx->CreateGain(); G->Connect(ToneHold); ToneHold->Connect(Amb);
	auto N = NoiseSrc(true); auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(420);
	auto Ng = Ctx->CreateGain(); Ng->Gain.SetValue(0.22); N->Connect(Lp); Lp->Connect(Ng); Ng->Connect(G); N->Start();
	auto HumO = Ctx->CreateOscillator(); HumO->Frequency.SetValue(60); auto Hg = Ctx->CreateGain(); Hg->Gain.SetValue(0.035); HumO->Connect(Hg); Hg->Connect(G); HumO->Start();
	auto Hum2 = Ctx->CreateOscillator(); Hum2->Frequency.SetValue(120); auto Hg2 = Ctx->CreateGain(); Hg2->Gain.SetValue(0.02); Hum2->Connect(Hg2); Hg2->Connect(G); Hum2->Start();
	G->Gain.SetTargetAtTime(0.5, Ctx->CurrentTime(), 1.5);
	RoomTone = G;
}

void US47Audio::HoldRoomTone(float Level, float Tau)
{
	// The room's hum held down (0) or let back (1), over where setSpace() keeps it (story/roofSteps.ts:
	// it drops out while something stands on the records room roof). tau: how fast.
	if (!Ctx.IsValid() || !ToneHold.IsValid()) return;
	FBatch Batch(*Ctx);
	// (from where it is now: a cancelled glide would otherwise jump back to where it began, a click)
	const double T = Ctx->CurrentTime();
	FS47AudioParam& G = ToneHold->Gain;
	G.CancelAndHoldAtTime(T);
	G.SetTargetAtTime(Level, T, Tau);
}

float US47Audio::RoomToneHeld() const { return ToneHold.IsValid() ? float(ToneHold->Gain.Value()) : 1.f; }

// Receiver tone: a sine whose loudness follows lock quality, plus band noise.
void US47Audio::StartCarrier()
{
	if (!Ctx.IsValid() || Carrier.Osc.IsValid()) return;
	FBatch Batch(*Ctx);
	auto Osc = Ctx->CreateOscillator(); Osc->SetType(TEXT("sine")); Osc->Frequency.SetValue(760);
	auto Gain = Ctx->CreateGain(); Gain->Gain.SetValue(0);
	Osc->Connect(Gain); Gain->Connect(SfxBus); Osc->Start();
	auto N = NoiseSrc(false); auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(1800); Bp->Q.SetValue(0.6);
	auto Stat = Ctx->CreateGain(); Stat->Gain.SetValue(0); N->Connect(Bp); Bp->Connect(Stat); Stat->Connect(SfxBus); N->Start();
	Carrier = { Osc, Gain, Stat };
}

/** The anomaly near 1420.405 as the receiver hears it: clarity 0 (static) to 1 (locked). */
void US47Audio::SignalFrame(float Clarity, float Level, double T)
{
	if (!Ctx.IsValid() || !Noise.IsValid()) return;
	FBatch Batch(*Ctx);
	if (!Signal.IsValid())
	{
		if (Clarity <= 0 || Level <= 0) return;
		Signal = MakeShared<FS47SignalVoice>(Ctx.ToSharedRef(), SfxBus, Noise);
	}
	Signal->Frame(Clarity, Level, T);
}

void US47Audio::SignalOff()
{
	if (Signal.IsValid()) Signal->Stop();
	Signal.Reset();
}

/** A point in the world that sounds can be sent to (core/walkie.ts). */
TSharedPtr<FS47AudioNode> US47Audio::Spatial(const FVector& At, float Ref, TSharedPtr<FS47AudioNode> Dest)
{
	if (!Ctx.IsValid()) return nullptr;
	FBatch Batch(*Ctx);
	return Panner(At, Ref, Dest);
}

// ---------- the radio on the filing cabinets ----------
// AM at night: a station a long way off that fades in and out of the static (skywave).
// The tune is made here, a slow three-chord waltz on a tinny little speaker; no
// recording, so no licence. amRadioJam() drops it into the hiss while the line is dead.
void US47Audio::AmRadioThree(TOptional<FVector> At)
{
	if (!Ctx.IsValid()) return;
	if (!At)
	{
		if (!Am.Out.IsValid()) return;
		FBatch Batch(*Ctx);
		const FAm R = Am;
		Am = FAm();
		R.Out->Gain.SetTargetAtTime(0, Ctx->CurrentTime(), 0.08);
		Ctx->SetTimeout(0.9, [R] { R.Stop(); });
		return;
	}
	if (Am.Out.IsValid()) return;
	// (the tune is written before the lock is taken, so the speaker does not wait for it)
	const float Sr = Ctx->SampleRate;
	const double Secs = 19.2;
	const TSharedRef<FS47AudioBuffer> Tune = Ctx->CreateBuffer(1, int32(FMath::FloorToDouble(Sr * Secs)), Sr);
	float* D = Tune->GetChannelData(0);
	const int32 Len = Tune->Length();
	// G, C, D, G in 3/4 at 100 bpm: bass on one, chord on two and three, a little melody over it
	auto Hz = [](double N) { return 440 * FMath::Pow(2.0, (N - 69) / 12); };
	struct FBar { int32 Bass; int32 Chord[3]; int32 Mel[3]; };
	const FBar Bars[4] = { { 43, { 59, 62, 67 }, { 71, 74, 71 } }, { 48, { 60, 64, 67 }, { 72, 71, 69 } }, { 50, { 62, 66, 69 }, { 69, 66, 62 } }, { 43, { 59, 62, 67 }, { 67, 0, 0 } } };
	const double Beat = 0.6;
	auto Tone = [D, Len, Sr](double F, double T0, double Dur, double Amp, double Harm)
	{
		const int32 I0 = int32(FMath::FloorToDouble(T0 * Sr)), N = int32(FMath::FloorToDouble(Dur * Sr));
		for (int32 I = 0; I < N && I0 + I < Len; ++I)
		{
			const double T = I / double(Sr), Env = FMath::Min(1.0, T / 0.01) * FMath::Exp(-T * 3.2);
			const double Ph = 2 * Pi * F * T;
			D[I0 + I] += float(Amp * Env * (FMath::Sin(Ph) + Harm * FMath::Sin(2 * Ph) + Harm * 0.5 * FMath::Sin(3 * Ph)));
		}
	};
	for (int32 Rep = 0; Rep < 2; ++Rep)
	{
		for (int32 Bi = 0; Bi < 4; ++Bi)
		{
			const FBar& Bar = Bars[Bi];
			const double T0 = (Rep * 4 + Bi) * 3 * Beat;
			Tone(Hz(Bar.Bass), T0, Beat * 1.2, 0.22, 0.4);
			for (const int32 K : { 1, 2 }) for (const int32 N : Bar.Chord) Tone(Hz(N), T0 + K * Beat, Beat * 0.8, 0.06, 0.6);
			for (int32 K = 0; K < 3; ++K) if (Bar.Mel[K]) Tone(Hz(Bar.Mel[K] + (Rep ? 0 : 0)), T0 + K * Beat, Beat * 1.4, 0.1, 0.3);
		}
	}
	FBatch Batch(*Ctx);
	auto Out = Ctx->CreateGain(); Out->Gain.SetValue(0);
	auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(1300); Bp->Q.SetValue(0.7);
	Out->Connect(Bp); Bp->Connect(Panner(*At, 0.8, RadioBus));
	auto Src = Ctx->CreateBufferSource(); Src->Buffer = Tune; Src->bLoop = true;
	auto MusicG = Ctx->CreateGain(); MusicG->Gain.SetValue(0.8);
	auto Fade = Ctx->CreateGain(); Fade->Gain.SetValue(0.6);
	auto Lfo = Ctx->CreateOscillator(); Lfo->Frequency.SetValue(0.07);
	auto Lg = Ctx->CreateGain(); Lg->Gain.SetValue(0.4); Lfo->Connect(Lg); Lg->Connect(Fade->Gain);
	Src->Connect(MusicG); MusicG->Connect(Fade); Fade->Connect(Out);
	const FS47StaticSrc N = StaticSrc().Get(FS47StaticSrc()); auto Hiss = Ctx->CreateGain(); Hiss->Gain.SetValue(0.18); N.Out->Connect(Hiss); Hiss->Connect(Out);
	Src->Start(); Lfo->Start();
	Out->Gain.SetTargetAtTime(0.11, Ctx->CurrentTime(), 0.3);
	TSharedPtr<FS47ScheduledSourceNode> S1 = Src, S2 = Lfo, S3 = N.Src;
	TSharedPtr<FS47GainNode> O = Out;
	Am.Out = Out; Am.MusicGain = MusicG; Am.Hiss = Hiss;
	Am.Stop = [S1, S2, S3, O] { for (const TSharedPtr<FS47ScheduledSourceNode>& X : { S1, S2, S3 }) if (X.IsValid()) X->Stop(); O->Disconnect(); };
}

void US47Audio::AmRadioJam(bool bOn)
{
	if (!Ctx.IsValid() || !Am.Out.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime();
	Am.MusicGain->Gain.SetTargetAtTime(bOn ? 0 : 0.8, T, bOn ? 0.4 : 1.2);
	Am.Hiss->Gain.SetTargetAtTime(bOn ? 0.5 : 0.18, T, 0.4);
}

void US47Audio::SetCarrier(float Level, float PitchOffset, float StaticLevel)
{
	if (!Carrier.Osc.IsValid() || !Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime();
	Carrier.Gain->Gain.SetTargetAtTime(Level, T, 0.015);
	Carrier.Osc->Frequency.SetTargetAtTime(760 * (1 + PitchOffset), T, 0.05);
	Carrier.Stat->Gain.SetTargetAtTime(StaticLevel, T, 0.08);
}

void US47Audio::Beep(float Freq, float Dur, float Gain, float When, const FString& Type)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime() + When;
	auto O = Ctx->CreateOscillator(); O->SetType(Type); O->Frequency.SetValue(Freq);
	auto G = Ctx->CreateGain(); G->Gain.SetValueAtTime(0, T); G->Gain.LinearRampToValueAtTime(Gain, T + 0.005); G->Gain.SetValueAtTime(Gain, T + Dur); G->Gain.LinearRampToValueAtTime(0, T + Dur + 0.02);
	auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(3000);
	O->Connect(Lp); Lp->Connect(G); G->Connect(SfxBus); O->Start(T); O->Stop(T + Dur + 0.05);
}

void US47Audio::Boom(float Strength, TSharedPtr<FS47AudioNode> Dest, float When)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime() + When;
	const TSharedPtr<FS47AudioNode> Out = Node(Dest, SfxBus);
	auto O = Ctx->CreateOscillator(); O->SetType(TEXT("sine"));
	O->Frequency.SetValueAtTime(70, T); O->Frequency.ExponentialRampToValueAtTime(24, T + 1.6);
	auto G = Ctx->CreateGain(); G->Gain.SetValueAtTime(0, T); G->Gain.LinearRampToValueAtTime(0.9 * Strength, T + 0.02); G->Gain.ExponentialRampToValueAtTime(0.001, T + 2.6);
	O->Connect(G); G->Connect(Out); O->Start(T); O->Stop(T + 2.8);
	auto N = NoiseSrc(true, false); auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValueAtTime(900, T); Lp->Frequency.ExponentialRampToValueAtTime(80, T + 2.2);
	auto Ng = Ctx->CreateGain(); Ng->Gain.SetValueAtTime(0, T); Ng->Gain.LinearRampToValueAtTime(1.4 * Strength, T + 0.015); Ng->Gain.ExponentialRampToValueAtTime(0.001, T + 2.8);
	N->Connect(Lp); Lp->Connect(Ng); Ng->Connect(Out); N->Start(T); N->Stop(T + 3);
}

// Thunder far away over the mesa: no crack, only rolls that come in slowly, darker and
// later the further off it is, after the far-thunder layers in Tom's Loincloth-Legends
// (thunderSyn with `far`), with a low sine swell under them.
/** Thunder that is rolling now falls silent at once (0) or is let back (1): story/morningHolds.ts. */
void US47Audio::HoldThunder(float Level)
{
	if (!Ctx.IsValid() || !ThunderBus.IsValid()) return;
	FBatch Batch(*Ctx);
	FS47AudioParam& G = ThunderBus->Gain;
	const double T = Ctx->CurrentTime();
	G.CancelScheduledValues(T); G.SetValueAtTime(G.Value(), T); G.SetTargetAtTime(Level, T, Level ? 0.3 : 0.02);
}

void US47Audio::Thunder(float Strength)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime();
	if (!ThunderBus.IsValid()) { ThunderBus = Ctx->CreateGain(); ThunderBus->Connect(Amb); }
	const TSharedPtr<FS47GainNode> Bus = ThunderBus;
	auto Roll = [this, T, &Bus](double At, double Dur, double F0, double F1, double V)
	{
		auto N = NoiseSrc(true, false); auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass"));
		Lp->Frequency.SetValueAtTime(F0, T + At); Lp->Frequency.ExponentialRampToValueAtTime(F1, T + At + Dur);
		auto G = Ctx->CreateGain(); G->Gain.SetValueAtTime(0, T + At); G->Gain.LinearRampToValueAtTime(V, T + At + Dur * 0.28); G->Gain.ExponentialRampToValueAtTime(0.001, T + At + Dur);
		N->Connect(Lp); Lp->Connect(G); G->Connect(Bus); N->Start(T + At); N->Stop(T + At + Dur + 0.1);
	};
	const double S = Strength;
	Roll(0, 3.4, 360, 55, 0.5 * S);
	Roll(0.8 + FMath::FRand() * 0.7, 2.6, 280, 45, 0.38 * S);
	if (FMath::FRand() < 0.7) Roll(2.1 + FMath::FRand() * 1.2, 3.2, 220, 38, 0.3 * S);
	auto O = Ctx->CreateOscillator(); O->Frequency.SetValueAtTime(44, T + 0.3); O->Frequency.ExponentialRampToValueAtTime(30, T + 2.8);
	auto Og = Ctx->CreateGain(); Og->Gain.SetValueAtTime(0, T + 0.3); Og->Gain.LinearRampToValueAtTime(0.28 * S, T + 1.0); Og->Gain.ExponentialRampToValueAtTime(0.001, T + 3.0);
	O->Connect(Og); Og->Connect(Bus); O->Start(T + 0.3); O->Stop(T + 3.1);
}

void US47Audio::Gasp(TSharedPtr<FS47AudioNode> Dest, float When)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime() + When;
	auto N = NoiseSrc(false, false); auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Q.SetValue(1.8);
	Bp->Frequency.SetValueAtTime(900, T); Bp->Frequency.LinearRampToValueAtTime(1600, T + 0.35);
	auto G = Ctx->CreateGain(); G->Gain.SetValueAtTime(0, T); G->Gain.LinearRampToValueAtTime(0.5, T + 0.06); G->Gain.ExponentialRampToValueAtTime(0.001, T + 0.45);
	N->Connect(Bp); Bp->Connect(G); G->Connect(Node(Dest, SfxBus)); N->Start(T); N->Stop(T + 0.5);
}

void US47Audio::PourThree(TOptional<FVector> At)
{
	if (!Ctx.IsValid()) return;
	// a recorded pour, the fill rising, when the bought one is there
	if (Lic(TEXT("pour")).IsValid()) { FS47Sound O; O.Gain = 0.33f; O.At = At; Sfx(TEXT("lic:pour"), O); return; }
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime();
	auto N = NoiseSrc(false, false); auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Q.SetValue(3);
	Bp->Frequency.SetValueAtTime(500, T); Bp->Frequency.LinearRampToValueAtTime(1300, T + 1.6);
	auto G = Ctx->CreateGain(); G->Gain.SetValueAtTime(0, T); G->Gain.LinearRampToValueAtTime(0.25, T + 0.1); G->Gain.SetValueAtTime(0.25, T + 1.4); G->Gain.LinearRampToValueAtTime(0, T + 1.7);
	N->Connect(Bp); Bp->Connect(G); G->Connect(At ? TSharedPtr<FS47AudioNode>(Panner(*At, 1)) : TSharedPtr<FS47AudioNode>(SfxBus)); N->Start(T); N->Stop(T + 1.8);
}

// A television with no station: hiss from the set's little speaker, where it stands.
void US47Audio::TvHiss(TOptional<FVector> At)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double Now = Ctx->CurrentTime();
	if (Tv.Src.IsValid()) { const FTv Old = Tv; Tv = FTv(); Old.Gain->Gain.SetTargetAtTime(0, Now, 0.05); Old.Src->Stop(Now + 0.3); }
	if (!At) return;
	auto N = NoiseSrc(false, true);
	auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(2600); Bp->Q.SetValue(0.5);
	auto Hp = Ctx->CreateBiquadFilter(); Hp->SetType(TEXT("highpass")); Hp->Frequency.SetValue(450);
	auto G = Ctx->CreateGain(); G->Gain.SetValue(0);
	G->Gain.SetTargetAtTime(0.09, Now, 0.15);
	N->Connect(Hp); Hp->Connect(Bp); Bp->Connect(G); G->Connect(Panner(*At, 0.9));
	N->Start();
	Tv = { N, G };
}

// The handset sound: band-limited like a 1980s phone line. (The voices go through it too: they
// are decoded into the context, so the line is the same two biquads as the browser's, not an
// audio component's own one-pole filters.)
FS47PhoneLine US47Audio::PhoneLine()
{
	FBatch Batch(*Ctx);
	auto Hp = Ctx->CreateBiquadFilter(); Hp->SetType(TEXT("highpass")); Hp->Frequency.SetValue(320);
	auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(3200);
	auto G = Ctx->CreateGain(); G->Gain.SetValue(1.0);
	Hp->Connect(Lp); Lp->Connect(G); G->Connect(SfxBus);
	FS47PhoneLine L;
	L.Input = Hp; L.Gain = G; L.Nodes = { Hp, Lp, G };
	return L;
}

double US47Audio::FutureCall()
{
	if (!Ctx.IsValid()) return 4.3;
	FBatch Batch(*Ctx);
	const FS47PhoneLine Line = PhoneLine();
	const double T = Ctx->CurrentTime();
	auto N = NoiseSrc(false); auto Sg = Ctx->CreateGain(); Sg->Gain.SetValue(0.05); N->Connect(Sg); Sg->Connect(Line.Input); N->Start(T); N->Stop(T + 4.3);
	auto H = NoiseSrc(true); auto Hg = Ctx->CreateGain(); Hg->Gain.SetValue(0.5); H->Connect(Hg); Hg->Connect(Line.Input); H->Start(T); H->Stop(T + 4.3);
	FS47Sound Printer; Printer.Gain = 0.15f; Printer.When = 0.4f; Printer.DestNode = Line.Input; Sfx(TEXT("printer"), Printer);
	Boom(0.8f, Line.Input, 2.0f);
	Gasp(Line.Input, 2.25f);
	FS47Sound Ceramic; Ceramic.Gain = 0.9f; Ceramic.When = 2.55f; Ceramic.DestNode = Line.Input; Sfx(TEXT("ceramic"), Ceramic);
	FS47Sound Click; Click.Gain = 0.8f; Click.When = 4.3f; Sfx(TEXT("click"), Click);
	return 4.3;
}

void US47Audio::StartMotors()
{
	if (!Ctx.IsValid() || Motor.Osc.IsValid()) return;
	FBatch Batch(*Ctx);
	auto Osc = Ctx->CreateOscillator(); Osc->SetType(TEXT("sawtooth")); Osc->Frequency.SetValue(46);
	auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(240);
	auto Gain = Ctx->CreateGain(); Gain->Gain.SetValue(0);
	Osc->Connect(Lp); Lp->Connect(Gain); Gain->Connect(Amb); Osc->Start();
	const double T = Ctx->CurrentTime();
	Gain->Gain.SetTargetAtTime(0.16, T, 0.8);
	Osc->Frequency.SetTargetAtTime(58, T, 2);
	Motor = { Gain, Osc };
}

void US47Audio::StopMotors()
{
	if (!Motor.Osc.IsValid() || !Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime();
	Motor.Gain->Gain.SetTargetAtTime(0, T, 0.6);
	Motor.Osc->Frequency.SetTargetAtTime(30, T, 0.8);
	Motor.Osc->Stop(T + 4);
	Motor = FMotor();
}

// An ordinary phone call on the handset: line hiss for `secs`, a click at each end.
void US47Audio::CallLine(float Secs)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const FS47PhoneLine Line = PhoneLine();
	const double T = Ctx->CurrentTime();
	auto N = NoiseSrc(false); auto G = Ctx->CreateGain(); G->Gain.SetValue(0.035);
	N->Connect(G); G->Connect(Line.Input); N->Start(T); N->Stop(T + Secs);
	FS47Sound A; A.Gain = 0.6f; Sfx(TEXT("click"), A);
	FS47Sound B; B.Gain = 0.5f; B.When = Secs; Sfx(TEXT("click"), B);
}

/** One prerecorded handset session. Captions and story events use the returned
 *  game-time timeline; every sound itself uses the pausable AudioContext clock. */
TOptional<FS47CallTimeline> US47Audio::PhoneCall(const TArray<FS47PhoneCue>& Lines, float Lead, float Gap)
{
	StopPhoneCalls();
	if ((!bKinskiCalls && !bCharacterVoices) || !Ctx.IsValid() || Ctx->State() == TEXT("closed") || !Noise.IsValid() || !Lines.Num()
		|| !Lines.ContainsByPredicate([](const FS47PhoneCue& L) { return !L.bAside; })) return {};
	FBatch Batch(*Ctx);
	TArray<TSharedPtr<FS47AudioBuffer>> Buffers;
	for (const FS47PhoneCue& L : Lines) Buffers.Add(LineBuffer(L.By, L.Text));
	// Validate the entire call before making any sound. Missing recordings fall
	// back to the existing caption/line flow, never a half-spoken conversation.
	for (int32 I = 0; I < Lines.Num(); ++I)
		if (!Lines[I].bAside && (!Buffers[I].IsValid() || !FMath::IsFinite(Buffers[I]->Duration()) || Buffers[I]->Duration() <= 0)) return {};
	auto Seconds = [](TOptional<double> Value, double Fallback) { return Value && FMath::IsFinite(*Value) ? FMath::Max(0.0, *Value) : Fallback; };
	const double LeadS = Seconds(double(Lead), 0.3), GapS = Seconds(double(Gap), 0.45);
	double At = LeadS;
	FS47CallTimeline TL;
	for (int32 I = 0; I < Lines.Num(); ++I)
	{
		const FS47PhoneCue& L = Lines[I];
		const double Duration = L.bAside ? Seconds(L.Pause ? TOptional<double>(*L.Pause) : TOptional<double>(), 1.2) : Buffers[I]->Duration();
		FS47CallCue Cue; Cue.At = float(At); Cue.Duration = float(Duration); Cue.Text = L.Text; Cue.By = L.By; Cue.bAside = L.bAside;
		TL.Cues.Add(Cue);
		At += Duration + (I < Lines.Num() - 1 ? GapS : 0);
	}
	const double Duration = At + 0.2, Started = Ctx->CurrentTime();
	if (OnPhoneCall) OnPhoneCall();
	TSharedPtr<FPhoneSession> Session = MakeShared<FPhoneSession>();
	PhoneSession = Session;
	const FS47PhoneLine Line = PhoneLine();
	// phoneLine's input is the high-pass, followed by low-pass and output gain.
	auto Speech = Ctx->CreateGain(); Speech->Gain.SetValue(0.95); Speech->Connect(Line.Input);
	Session->Nodes.Add(Speech); Session->Nodes.Append(Line.Nodes);
	for (int32 I = 0; I < Lines.Num(); ++I)
	{
		if (Lines[I].bAside) continue;
		auto Source = Ctx->CreateBufferSource(); Source->Buffer = Buffers[I];
		Source->Connect(Speech); Session->Sources.Add(Source); Source->Start(Started + TL.Cues[I].At);
	}
	auto Hiss = NoiseSrc(false); auto Level = Ctx->CreateGain(); Level->Gain.SetValue(0.025);
	Hiss->Connect(Level); Level->Connect(Line.Input);
	Session->Sources.Add(Hiss); Session->Nodes.Add(Level);
	TWeakObjectPtr<US47Audio> Self(this);
	TWeakPtr<FPhoneSession> Weak = Session;
	Hiss->OnEnded = [Self, Weak] { if (Self.IsValid() && Self->PhoneSession.IsValid() && Self->PhoneSession == Weak.Pin()) Self->StopPhoneCalls(); };
	Hiss->Start(Started); Hiss->Stop(Started + Duration);
	const double Clicks[2][2] = { { 0, 0.6 }, { At, 0.5 } };
	for (const auto& C : Clicks)
	{
		FS47Sound O; O.Gain = float(C[1]); O.When = float(C[0]); O.DestNode = Line.Input;
		const FS47Play Click = Sfx(TEXT("click"), O);
		if (Click) { Session->Sources.Add(Click.Src); Session->Nodes.Add(Click.Gain); }
	}
	TL.Duration = float(Duration);
	return TL;
}

/** The recording phoneCall() plays for a line on the telephone: the character's voice, else
 *  Kinski's when his calls are on; unset when the line has none (then it is the caption). */
TSharedPtr<FS47AudioBuffer> US47Audio::LineBuffer(const FString& By, const FString& Text) const
{
	const TSharedPtr<FS47AudioBuffer> Actor = By == TEXT("N. VEGA") ? CharacterBuffer(TEXT("bacall"), Text)
		: By == TEXT("THE LINE") ? CharacterBuffer(TEXT("price"), Text) : nullptr;
	if (Actor.IsValid()) return Actor;
	if (!bKinskiCalls) return nullptr;
	const FString* Key = PhoneKeys.Find(NormalizeReadingText(Text));
	return Key ? Buf.FindRef(*Key) : nullptr;
}

/** A hang-up, replacement, new night, save load or title: future sources stop too. */
void US47Audio::StopPhoneCalls()
{
	const TSharedPtr<FPhoneSession> Session = PhoneSession;
	if (!Session.IsValid()) return;
	PhoneSession.Reset();
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	for (const TSharedPtr<FS47ScheduledSourceNode>& Source : Session->Sources)
	{
		Source->OnEnded = nullptr;
		Source->Stop(); // (one that never started: nothing)
		Source->Disconnect();
	}
	for (const TSharedPtr<FS47AudioNode>& N : Session->Nodes) N->Disconnect();
}

// Touch-tone dialling. Returns how long it takes.
float US47Audio::Dial(const FString& Number)
{
	if (!Ctx.IsValid()) return 1.5f;
	FBatch Batch(*Ctx);
	static const int32 Rows[] = { 697, 770, 852, 941 }, Cols[] = { 1209, 1336, 1477 };
	const FString Keys = TEXT("123456789*0#");
	double When = 0.25;
	for (const TCHAR Ch : Number)
	{
		int32 K = INDEX_NONE;
		if (!Keys.FindChar(Ch, K)) { When += 0.2; continue; }
		const double T = Ctx->CurrentTime() + When;
		for (const int32 F : { Rows[K / 3], Cols[K % 3] })
		{
			auto O = Ctx->CreateOscillator(); O->Frequency.SetValue(F);
			auto G = Ctx->CreateGain();
			G->Gain.SetValueAtTime(0, T); G->Gain.LinearRampToValueAtTime(0.05, T + 0.01); G->Gain.SetValueAtTime(0.05, T + 0.09); G->Gain.LinearRampToValueAtTime(0, T + 0.1);
			O->Connect(G); G->Connect(SfxBus); O->Start(T); O->Stop(T + 0.12);
		}
		When += 0.16;
	}
	return float(When);
}

// North American ringback in the earpiece: 440 and 480 Hz together, on for 1.6 s.
float US47Audio::Ringback(int32 Times, float Start)
{
	if (!Ctx.IsValid()) return Start + Times * 3.2f;
	FBatch Batch(*Ctx);
	for (int32 I = 0; I < Times; ++I)
	{
		const double T = Ctx->CurrentTime() + Start + I * 3.2;
		for (const int32 F : { 440, 480 })
		{
			auto O = Ctx->CreateOscillator(); O->Frequency.SetValue(F);
			auto G = Ctx->CreateGain();
			G->Gain.SetValueAtTime(0, T); G->Gain.LinearRampToValueAtTime(0.04, T + 0.03); G->Gain.SetValueAtTime(0.04, T + 1.6); G->Gain.LinearRampToValueAtTime(0, T + 1.65);
			O->Connect(G); G->Connect(SfxBus); O->Start(T); O->Stop(T + 1.7);
		}
	}
	return Start + Times * 3.2f;
}

// Positional electrical hum (a transformer cabinet, a lamp ballast): mains and two harmonics.
void US47Audio::Hum(const FString& Key, const FVector& At, float Gain, float Base)
{
	if (!Ctx.IsValid() || Hums.Contains(Key)) return;
	FBatch Batch(*Ctx);
	HumGain.Add(Key, Gain);
	auto Out = Ctx->CreateGain(); Out->Gain.SetValue(0);
	auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(900);
	Out->Connect(Lp); Lp->Connect(Panner(At, 1.0));
	struct FPart { double Mul, G; const TCHAR* Type; };
	for (const FPart& P : { FPart{ 1, 1, TEXT("sine") }, FPart{ 2, 0.5, TEXT("sine") }, FPart{ 3, 0.12, TEXT("square") } })
	{
		auto O = Ctx->CreateOscillator(); O->SetType(P.Type); O->Frequency.SetValue(Base * P.Mul);
		auto Og = Ctx->CreateGain(); Og->Gain.SetValue(P.G); O->Connect(Og); Og->Connect(Out); O->Start();
	}
	Out->Gain.SetTargetAtTime(Gain * HumK, Ctx->CurrentTime(), 1.0);
	Hums.Add(Key, Out);
}

void US47Audio::StopHums()
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	for (const TPair<FString, TSharedPtr<FS47GainNode>>& H : Hums) H.Value->Gain.SetTargetAtTime(0, Ctx->CurrentTime(), 0.3);
	Hums.Empty(); HumGain.Empty();
}

/** Every hum at k of its level (story/saroDark.ts: they die with the power and come back). */
void US47Audio::HumLevel(float K, float Tau)
{
	HumK = K;
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	for (const TPair<FString, TSharedPtr<FS47GainNode>>& H : Hums)
	{
		const float* G = HumGain.Find(H.Key);
		H.Value->Gain.SetTargetAtTime((G ? *G : 0.05f) * K, Ctx->CurrentTime(), Tau);
	}
}

// ---------- SARO without power (story/saroDark.ts) ----------
/** Relay K3 tripping on the rack, as the prologue plays it, heard from where the listener is:
 *  through the wall it is duller and carries further than a click in the room would. */
void US47Audio::RelayClack(const FVector& At, bool bThrough)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const TSharedPtr<FS47AudioNode> Dest = Panner(At, bThrough ? 4 : 1.6);
	FS47Sound A; A.Gain = bThrough ? 0.62f : 1.0f; A.Rate = 0.62f; A.DestNode = Dest; A.Lp = bThrough ? 1500.f : 0.f;
	Sfx(Or(TEXT("rack_switch"), TEXT("switch")), A);
	FS47Sound B; B.Gain = bThrough ? 0.38f : 0.45f; B.Rate = 1.4f; B.DestNode = Dest; B.When = 0.03f; B.Lp = bThrough ? 700.f : 0.f;
	Sfx(TEXT("thudSoft"), B);
}

/** The building's main contactor dropping out (or pulling in): a heavy knock through the floor. */
void US47Audio::Contactor(const FVector& At, float Gain)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime();
	const TSharedPtr<FS47AudioNode> Dest = Panner(At, 6);
	FS47Sound A; A.Gain = 0.9f * Gain; A.Rate = 0.42f; A.DestNode = Dest; A.Lp = 900.f;
	Sfx(TEXT("thudSoft"), A);
	auto O = Ctx->CreateOscillator(); O->Frequency.SetValueAtTime(58, T); O->Frequency.ExponentialRampToValueAtTime(31, T + 0.5);
	auto G = Ctx->CreateGain(); G->Gain.SetValueAtTime(0, T); G->Gain.LinearRampToValueAtTime(0.32 * Gain, T + 0.012); G->Gain.ExponentialRampToValueAtTime(0.001, T + 0.7);
	O->Connect(G); G->Connect(Dest); O->Start(T); O->Stop(T + 0.75);
}

/** Everything electric running down at once: the transformers' hum sinking away, the fans after. */
void US47Audio::PowerDown(const FVector& At)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime();
	const TSharedPtr<FS47AudioNode> Dest = Panner(At, 8);
	auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValueAtTime(700, T); Lp->Frequency.ExponentialRampToValueAtTime(90, T + 2.6);
	auto G = Ctx->CreateGain(); G->Gain.SetValueAtTime(0.09, T); G->Gain.ExponentialRampToValueAtTime(0.001, T + 2.8);
	Lp->Connect(G); G->Connect(Dest);
	struct FPart { double F; const TCHAR* Type; double V; };
	for (const FPart& P : { FPart{ 120, TEXT("sawtooth"), 0.5 }, FPart{ 180, TEXT("sine"), 0.3 }, FPart{ 60, TEXT("sine"), 0.6 } })
	{
		auto O = Ctx->CreateOscillator(); O->SetType(P.Type); O->Frequency.SetValueAtTime(P.F, T); O->Frequency.ExponentialRampToValueAtTime(P.F * 0.28, T + 2.6);
		auto Og = Ctx->CreateGain(); Og->Gain.SetValue(P.V); O->Connect(Og); Og->Connect(Lp); O->Start(T); O->Stop(T + 2.9);
	}
}

/** And back: the hum rising out of nothing, and the ballasts' buzz as the tubes strike. */
void US47Audio::PowerUp(const FVector& At)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime();
	const TSharedPtr<FS47AudioNode> Dest = Panner(At, 8);
	auto G = Ctx->CreateGain(); G->Gain.SetValueAtTime(0, T); G->Gain.LinearRampToValueAtTime(0.06, T + 0.5); G->Gain.ExponentialRampToValueAtTime(0.001, T + 1.6);
	G->Connect(Dest);
	struct FPart { double F; const TCHAR* Type; double V; };
	for (const FPart& P : { FPart{ 120, TEXT("sawtooth"), 0.35 }, FPart{ 60, TEXT("sine"), 0.6 } })
	{
		auto O = Ctx->CreateOscillator(); O->SetType(P.Type); O->Frequency.SetValueAtTime(P.F * 0.4, T); O->Frequency.ExponentialRampToValueAtTime(P.F, T + 0.6);
		auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(500);
		auto Og = Ctx->CreateGain(); Og->Gain.SetValue(P.V); O->Connect(Lp); Lp->Connect(Og); Og->Connect(G); O->Start(T); O->Stop(T + 1.7);
	}
}

/** The teleprinter's motor running while it prints (unset: it stops). */
void US47Audio::TelexMotor(TOptional<FVector> At)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	if (!At)
	{
		const FTelex M = TelexRun;
		TelexRun = FTelex();
		if (M.G.IsValid()) { M.G->Gain.SetTargetAtTime(0, Ctx->CurrentTime(), 0.15); Ctx->SetTimeout(0.9, [M] { M.Stop(); }); }
		return;
	}
	if (TelexRun.G.IsValid()) return;
	const TSharedPtr<FS47AudioNode> Dest = Panner(*At, 2.4);
	auto G = Ctx->CreateGain(); G->Gain.SetValue(0); G->Connect(Dest);
	auto O = Ctx->CreateOscillator(); O->SetType(TEXT("sawtooth")); O->Frequency.SetValue(50);
	auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(260);
	auto Og = Ctx->CreateGain(); Og->Gain.SetValue(0.5); O->Connect(Lp); Lp->Connect(Og); Og->Connect(G);
	auto N = NoiseSrc(false); auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(1300); Bp->Q.SetValue(0.8);
	auto Ng = Ctx->CreateGain(); Ng->Gain.SetValue(0.05); N->Connect(Bp); Bp->Connect(Ng); Ng->Connect(G);
	O->Start(); N->Start();
	G->Gain.SetTargetAtTime(0.06, Ctx->CurrentTime(), 0.12);
	TSharedPtr<FS47OscillatorNode> Os = O;
	TSharedPtr<FS47BufferSourceNode> Ns = N;
	TSharedPtr<FS47GainNode> Gs = G;
	TelexRun.G = G;
	TelexRun.Stop = [Os, Ns, Gs, Dest] { Os->Stop(); Ns->Stop(); Gs->Disconnect(); Dest->Disconnect(); };
}

/** One character struck on the teleprinter's platen (KAPITLER.md: made here, rhythmic strikes and
 *  a motor); `cr` the carriage going back at the end of a line. */
void US47Audio::TypeBar(const FVector& At, bool bCr)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime();
	const TSharedPtr<FS47AudioNode> Dest = Panner(At, 2.4);
	auto N = NoiseSrc(false, false); auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(bCr ? 900 : 2300 + FMath::FRand() * 500); Bp->Q.SetValue(bCr ? 1.2 : 2.2);
	auto G = Ctx->CreateGain();
	const double Len = bCr ? 0.16 : 0.018;
	G->Gain.SetValueAtTime(0, T); G->Gain.LinearRampToValueAtTime(bCr ? 0.5 : 0.9, T + 0.002); G->Gain.ExponentialRampToValueAtTime(0.001, T + Len);
	N->Connect(Bp); Bp->Connect(G); G->Connect(Dest); N->Start(T); N->Stop(T + Len + 0.02);
	auto O = Ctx->CreateOscillator(); O->Frequency.SetValueAtTime(bCr ? 180 : 420, T); O->Frequency.ExponentialRampToValueAtTime(bCr ? 90 : 260, T + 0.03);
	auto Og = Ctx->CreateGain(); Og->Gain.SetValueAtTime(0, T); Og->Gain.LinearRampToValueAtTime(bCr ? 0.12 : 0.07, T + 0.002); Og->Gain.ExponentialRampToValueAtTime(0.001, T + (bCr ? 0.12 : 0.04));
	O->Connect(Og); Og->Connect(Dest); O->Start(T); O->Stop(T + 0.14);
	Ctx->SetTimeout(0.6, [Dest] { Dest->Disconnect(); });
}

// The valve receiver in the STATION 01 hut: a weak carrier that fades in and out, with
// static, from a small speaker. `voice` lays a voice-shaped band of noise under it for
// a few seconds (the fragment is captioned; there is no recorded actor).
void US47Audio::Radio(TOptional<FVector> At, float Level)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	if (!At)
	{
		if (!RadioNodes.Out.IsValid()) return;
		const FRadioNodes R = RadioNodes;
		RadioNodes = FRadioNodes();
		R.Out->Gain.SetTargetAtTime(0, Ctx->CurrentTime(), 0.15);
		Ctx->SetTimeout(1.2, [R] { R.Stop(); });
		return;
	}
	if (RadioNodes.Out.IsValid()) return;
	auto Out = Ctx->CreateGain(); Out->Gain.SetValue(0);
	auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(1100); Bp->Q.SetValue(0.6);
	Out->Connect(Bp); Bp->Connect(Panner(*At, 1.0));
	auto Osc = Ctx->CreateOscillator(); Osc->Frequency.SetValue(760);
	auto Og = Ctx->CreateGain(); Og->Gain.SetValue(0.45); Osc->Connect(Og); Og->Connect(Out);
	auto Lfo = Ctx->CreateOscillator(); Lfo->Frequency.SetValue(0.21);
	auto Lg = Ctx->CreateGain(); Lg->Gain.SetValue(0.3); Lfo->Connect(Lg); Lg->Connect(Og->Gain);
	const FS47StaticSrc N = StaticSrc().Get(FS47StaticSrc()); auto Ng = Ctx->CreateGain(); Ng->Gain.SetValue(0.3); N.Out->Connect(Ng); Ng->Connect(Out);
	// the voice: noise through a speech band, chopped at a syllable rate
	auto Vn = NoiseSrc(true); auto Vb = Ctx->CreateBiquadFilter(); Vb->SetType(TEXT("bandpass")); Vb->Frequency.SetValue(650); Vb->Q.SetValue(1.8);
	auto Voice = Ctx->CreateGain(); Voice->Gain.SetValue(0);
	auto Syl = Ctx->CreateOscillator(); Syl->SetType(TEXT("square")); Syl->Frequency.SetValue(4.3);
	auto Sg = Ctx->CreateGain(); Sg->Gain.SetValue(0.5); Syl->Connect(Sg);
	auto Chop = Ctx->CreateGain(); Chop->Gain.SetValue(0.5); Sg->Connect(Chop->Gain);
	Vn->Connect(Vb); Vb->Connect(Chop); Chop->Connect(Voice); Voice->Connect(Out);
	Osc->Start(); Lfo->Start(); Vn->Start(); Syl->Start();
	Out->Gain.SetTargetAtTime(Level, Ctx->CurrentTime(), 0.5);
	const TArray<TSharedPtr<FS47ScheduledSourceNode>> Sources = { Osc, Lfo, N.Src, Vn, Syl };
	TSharedPtr<FS47GainNode> O = Out;
	RadioNodes.Out = Out; RadioNodes.Voice = Voice;
	RadioNodes.Stop = [Sources, O] { for (const TSharedPtr<FS47ScheduledSourceNode>& X : Sources) if (X.IsValid()) X->Stop(); O->Disconnect(); };
}

void US47Audio::RadioVoice(float Secs, float When)
{
	if (!Ctx.IsValid() || !RadioNodes.Out.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime() + When;
	RadioNodes.Voice->Gain.SetTargetAtTime(2.2, T, 0.08);
	RadioNodes.Voice->Gain.SetTargetAtTime(0, T + Secs, 0.12);
}

// The radio in the truck's dash on the old road (chapter six): not placed in the world,
// a small speaker in front of the driver. 'talk' is the morning programme from Roswell,
// a mumbling horn too low to follow (the captions say what it says);
// 'carrier' is the bare carrier when the night takes the station; null fades it out.
// radioIn() is where the fragments of the night are played through the same speaker.
void US47Audio::CabRadio(const FString& Mode)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	if (Mode.IsEmpty())
	{
		if (!Cab.Out.IsValid()) return;
		const FCab R = Cab;
		Cab = FCab();
		R.Out->Gain.SetTargetAtTime(0, Ctx->CurrentTime(), 0.05);
		Ctx->SetTimeout(0.7, [R] { R.Stop(); });
		return;
	}
	if (!Cab.Out.IsValid())
	{
		auto Out = Ctx->CreateGain(); Out->Gain.SetValue(0);
		auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(1250); Bp->Q.SetValue(0.8);
		auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(3400);
		Out->Connect(Bp); Bp->Connect(Lp); Lp->Connect(SfxBus);
		// talk: the morning man in Roswell as a muted horn that mumbles like the grown-ups in
		// Peanuts (Tom, 16 October; core/radio.ts has the same voice): the mute opens and closes on
		// every syllable ("wah"), the pitch wanders, and there are pauses between phrases
		auto Horn = Ctx->CreateOscillator(); Horn->SetType(TEXT("sawtooth")); Horn->Frequency.SetValue(124);
		auto P1 = Ctx->CreateOscillator(); P1->Frequency.SetValue(0.73);
		auto P1g = Ctx->CreateGain(); P1g->Gain.SetValue(16); P1->Connect(P1g); P1g->Connect(Horn->Frequency);
		auto P2 = Ctx->CreateOscillator(); P2->Frequency.SetValue(1.9);
		auto P2g = Ctx->CreateGain(); P2g->Gain.SetValue(7); P2->Connect(P2g); P2g->Connect(Horn->Frequency);
		auto Mute = Ctx->CreateBiquadFilter(); Mute->SetType(TEXT("lowpass")); Mute->Frequency.SetValue(800); Mute->Q.SetValue(6.5);
		auto Syl = Ctx->CreateOscillator(); Syl->SetType(TEXT("triangle")); Syl->Frequency.SetValue(4.3);
		auto Sw = Ctx->CreateGain(); Sw->Gain.SetValue(520); Syl->Connect(Sw); Sw->Connect(Mute->Frequency);
		auto Sg = Ctx->CreateGain(); Sg->Gain.SetValue(0.5); Syl->Connect(Sg);
		auto Phr = Ctx->CreateOscillator(); Phr->SetType(TEXT("square")); Phr->Frequency.SetValue(0.31);
		auto Pg = Ctx->CreateGain(); Pg->Gain.SetValue(0.5); Phr->Connect(Pg);
		auto Chop = Ctx->CreateGain(); Chop->Gain.SetValue(0.5); Sg->Connect(Chop->Gain);
		auto Pause = Ctx->CreateGain(); Pause->Gain.SetValue(0.5); Pg->Connect(Pause->Gain);
		auto Talk = Ctx->CreateGain(); Talk->Gain.SetValue(0);
		Horn->Connect(Mute); Mute->Connect(Chop); Chop->Connect(Pause); Pause->Connect(Talk); Talk->Connect(Out);
		auto Car = Ctx->CreateGain(); Car->Gain.SetValue(0);
		auto Osc = Ctx->CreateOscillator(); Osc->Frequency.SetValue(760); Osc->Connect(Car); Car->Connect(Out);
		const FS47StaticSrc N = StaticSrc().Get(FS47StaticSrc());
		auto Hiss = Ctx->CreateGain(); Hiss->Gain.SetValue(0.25); N.Out->Connect(Hiss); Hiss->Connect(Out);
		// now and then the static swells over the talk and the talk sinks under it (Tom, 16 October):
		// three slow waves that only now and then add up past a threshold, so it comes irregularly
		auto Fz = Ctx->CreateGain(); Fz->Gain.SetValue(0);
		TArray<TSharedPtr<FS47ScheduledSourceNode>> Waves;
		const double WaveF[3] = { 0.071, 0.113, 0.29 }, WaveG[3] = { 0.45, 0.35, 0.25 };
		for (int32 I = 0; I < 3; ++I) { auto O = Ctx->CreateOscillator(); O->Frequency.SetValue(WaveF[I]); auto G = Ctx->CreateGain(); G->Gain.SetValue(WaveG[I]); O->Connect(G); G->Connect(Fz); Waves.Add(O); }
		auto Over = Ctx->CreateWaveShaper();
		TArray<float> Curve; Curve.SetNum(512);
		for (int32 I = 0; I < 512; ++I) { const double X = (I / 511.0) * 2.1 - 1.05; Curve[I] = float(FMath::Pow(FMath::Max(0.0, (X - 0.55) / 0.5), 1.4)); }
		Over->Curve = Curve;
		Fz->Connect(Over);
		auto Swell = Ctx->CreateGain(); Swell->Gain.SetValue(0); Over->Connect(Swell); Swell->Connect(Hiss->Gain);
		auto DuckAmt = Ctx->CreateGain(); DuckAmt->Gain.SetValue(-0.8); Over->Connect(DuckAmt);
		auto Duck = Ctx->CreateGain(); Duck->Gain.SetValue(1); DuckAmt->Connect(Duck->Gain);
		Talk->Disconnect(); Talk->Connect(Duck); Duck->Connect(Out);
		TArray<TSharedPtr<FS47ScheduledSourceNode>> Sources = { Horn, P1, P2, Syl, Phr, Osc };
		Sources.Append(Waves);
		for (const TSharedPtr<FS47ScheduledSourceNode>& X : Sources) X->Start();
		Sources.Add(N.Src);
		TSharedPtr<FS47GainNode> O = Out;
		Cab.Out = Out; Cab.Spk = Bp; Cab.Talk = Talk; Cab.Car = Car; Cab.Hiss = Hiss; Cab.Swell = Swell;
		Cab.Stop = [Sources, O] { for (const TSharedPtr<FS47ScheduledSourceNode>& X : Sources) if (X.IsValid()) X->Stop(); O->Disconnect(); };
	}
	const double T = Ctx->CurrentTime();
	const bool bTalk = Mode == TEXT("talk"), bCarrier = Mode == TEXT("carrier");
	Cab.Out->Gain.SetTargetAtTime(0.09, T, 0.2);
	Cab.Talk->Gain.SetTargetAtTime(bTalk ? 0.55 : 0, T, bTalk ? 0.4 : 0.01);
	Cab.Car->Gain.SetTargetAtTime(bCarrier ? 0.22 : 0, T, 0.01);
	Cab.Hiss->Gain.SetTargetAtTime(bTalk ? 0.22 : bCarrier ? 0.08 : 0.35, T, 0.02);
	Cab.Swell->Gain.SetTargetAtTime(bTalk ? 0.5 : 0, T, 0.05);   // the swells only under the morning talk, not in THE EVENT
}

/** The cab speaker's input (unset when the radio is off): sounds played here come out of it. */
TSharedPtr<FS47AudioNode> US47Audio::RadioIn() const { return Cab.Spk; }

/** The cab radio's level against its own (1), for a while: down under the CB when it opens by itself
 *  on the old road (story/Chapter6.ts). cabRadio() puts it back. */
void US47Audio::CabLevel(float K)
{
	if (!Ctx.IsValid() || !Cab.Out.IsValid()) return;
	Cab.Out->Gain.SetTargetAtTime(0.09 * K, Ctx->CurrentTime(), 0.35);
}

/** A voice on the cab radio, as noise in a speech band (no recorded actor; it is captioned).
 *  formant: lower for a man (about 520 Hz), higher for a woman (about 820 Hz). */
void US47Audio::CabVoice(float Secs, float Formant)
{
	if (!Ctx.IsValid() || !Cab.Out.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime();
	auto Vn = NoiseSrc(true, true); auto Vb = Ctx->CreateBiquadFilter(); Vb->SetType(TEXT("bandpass")); Vb->Frequency.SetValue(Formant); Vb->Q.SetValue(2.2);
	auto Vb2 = Ctx->CreateBiquadFilter(); Vb2->SetType(TEXT("peaking")); Vb2->Frequency.SetValue(Formant * 2.6); Vb2->Gain.SetValue(8);
	auto Syl = Ctx->CreateOscillator(); Syl->SetType(TEXT("triangle")); Syl->Frequency.SetValue(3.9 + (Formant > 700 ? 0.8 : 0));
	auto Sg = Ctx->CreateGain(); Sg->Gain.SetValue(0.55); Syl->Connect(Sg);
	auto Chop = Ctx->CreateGain(); Chop->Gain.SetValue(0.5); Sg->Connect(Chop->Gain);
	auto G = Ctx->CreateGain(); G->Gain.SetValue(0);
	Vn->Connect(Vb); Vb->Connect(Vb2); Vb2->Connect(Chop); Chop->Connect(G); G->Connect(Cab.Spk);
	G->Gain.SetTargetAtTime(3.2, T, 0.05); G->Gain.SetTargetAtTime(0, T + Secs, 0.08);
	Vn->Start(T); Syl->Start(T); Vn->Stop(T + Secs + 0.6); Syl->Stop(T + Secs + 0.6);
}

/** A cup set down on a saucer: two short bright rings. dest: where it plays (the cab radio). */
void US47Audio::CupDown(TSharedPtr<FS47AudioNode> Dest)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime();
	const TSharedPtr<FS47AudioNode> Out = Node(Dest, SfxBus);
	const double Rings[4][3] = { { 0, 2380, 0.22 }, { 0.012, 3710, 0.12 }, { 0.09, 2390, 0.07 }, { 0.1, 5120, 0.05 } };
	for (const auto& R : Rings)
	{
		const double Dt = R[0], F = R[1], A = R[2];
		auto O = Ctx->CreateOscillator(); O->Frequency.SetValue(F);
		auto G = Ctx->CreateGain(); G->Gain.SetValue(0);
		G->Gain.SetValueAtTime(0, T + Dt); G->Gain.LinearRampToValueAtTime(A, T + Dt + 0.002); G->Gain.ExponentialRampToValueAtTime(0.0005, T + Dt + 0.35);
		O->Connect(G); G->Connect(Out); O->Start(T + Dt); O->Stop(T + Dt + 0.4);
	}
	auto N = NoiseSrc(false, false); auto Ng = Ctx->CreateGain(); auto Hp = Ctx->CreateBiquadFilter(); Hp->SetType(TEXT("highpass")); Hp->Frequency.SetValue(2500);
	Ng->Gain.SetValueAtTime(0.18, T); Ng->Gain.ExponentialRampToValueAtTime(0.0005, T + 0.04);
	N->Connect(Hp); Hp->Connect(Ng); Ng->Connect(Out); N->Start(T); N->Stop(T + 0.06);
}

/** Total silence (THE EVENT): every sound and the ambience off; music is left alone. */
void US47Audio::Silence(bool bOn)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime();
	SfxBus->Gain.SetTargetAtTime(bOn ? 0 : 1, T, bOn ? 0.04 : 0.3);
	Amb->Gain.SetTargetAtTime(bOn ? 0 : 1, T, bOn ? 0.08 : 0.5);
}

// Where the listener is. Out in the yard the wind and the crickets take over from the
// room tone; the photo lab is small and closed, with its own ventilation hum.
void US47Audio::SetSpace(const FString& InSpace)
{
	Space = InSpace;
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime();
	if (RoomTone.IsValid()) RoomTone->Gain.SetTargetAtTime(Space == TEXT("room") ? 0.5 : Space == TEXT("lab") ? 0.3 : 0.1, T, 0.7);
	if (Night.IsValid()) Night->SetSpace(Space);
}

// The desert outside (core/ambience.ts): wind, crickets and far-off things, never quite
// the same, through one outdoor level that setSpace() turns down indoors and in the cab.
void US47Audio::StartNight()
{
	if (!Ctx.IsValid() || Night.IsValid()) return;
	Night = MakeShared<FS47Ambience>(Ctx.ToSharedRef(), Amb, Buf, Noise, Brown);
	Night->SetSpace(Space);
}

/** A step's recording and level for a surface, without playing it (story/nightEvents.ts). */
US47Audio::FStepPick US47Audio::StepPick(const FString& Surface) const
{
	const FSteps& S = STEPS(Surface);
	FStepPick P;
	P.Name = S.Names[FMath::Min(S.Names.Num() - 1, int32(FMath::FRand() * S.Names.Num()))];
	P.Gain = S.Gain;
	return P;
}

/** One step on a surface (Player.onStep). */
void US47Audio::Step(const FString& Surface)
{
	const FSteps& S = STEPS(Surface);
	const int32 Len = S.Names.Num();
	FString Name = S.Names[FMath::Min(Len - 1, int32(FMath::FRand() * Len))];
	if (Name == LastStep) Name = S.Names[(S.Names.IndexOfByKey(Name) + 1 + int32(FMath::FRand() * (Len - 1))) % Len];
	LastStep = Name;
	FS47Sound O; O.Gain = S.Gain * (0.8f + FMath::FRand() * 0.3f); O.Rate = S.Rate * (0.95f + FMath::FRand() * 0.1f); O.Lp = S.Lp;
	Sfx(Name, O);
}

/** Every frame while a night runs: the desert outside moves on (clock: the night's clock). */
void US47Audio::NightLife(float Dt, const FVector& At, double Clock) { if (Night.IsValid()) Night->Update(Dt, At, Clock); }
/** A recording far off in a random direction (an owl, a dog): through the desert outside. */
void US47Audio::Far(const FString& Name, const FVector& At, float Gain, float Rate) { if (Night.IsValid()) Night->Far(Name, At, Gain, Rate); }

void US47Audio::DuckAll(float Seconds)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime();
	SfxBus->Gain.SetTargetAtTime(0, T, 0.3); Amb->Gain.SetTargetAtTime(0, T, 0.6);
	TWeakObjectPtr<US47Audio> Self(this);
	Ctx->SetTimeout(Seconds, [Self]
	{
		if (!Self.IsValid() || !Self->Ctx.IsValid()) return;
		FBatch Later(*Self->Ctx);
		const double T2 = Self->Ctx->CurrentTime();
		Self->SfxBus->Gain.SetTargetAtTime(1, T2, 0.2); Self->Amb->Gain.SetTargetAtTime(1, T2, 0.5);
	});
}

// Rangefinder shutter: two dry clicks, then the film advance lever. The same shutter can be
// heard somewhere else, later (dest, when: the office line plays one back, story/officePhone.ts).
void US47Audio::Shutter(TSharedPtr<FS47AudioNode> Dest, float When)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime() + When;
	const TSharedPtr<FS47AudioNode> Out = Node(Dest, SfxBus);
	auto Click = [this, T, &Out](double W, double F, double G)
	{
		auto N = NoiseSrc(false, false); auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(F); Bp->Q.SetValue(2.5);
		auto Gn = Ctx->CreateGain(); Gn->Gain.SetValueAtTime(0, T + W); Gn->Gain.LinearRampToValueAtTime(G, T + W + 0.002); Gn->Gain.ExponentialRampToValueAtTime(0.001, T + W + 0.04);
		N->Connect(Bp); Bp->Connect(Gn); Gn->Connect(Out); N->Start(T + W); N->Stop(T + W + 0.06);
	};
	Click(0, 3400, 0.8); Click(0.05, 2200, 0.55);
	auto Osc = Ctx->CreateOscillator(); Osc->SetType(TEXT("sawtooth")); Osc->Frequency.SetValueAtTime(150, T + 0.16); Osc->Frequency.LinearRampToValueAtTime(95, T + 0.42);
	auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(1100);
	auto G = Ctx->CreateGain(); G->Gain.SetValueAtTime(0, T + 0.16); G->Gain.LinearRampToValueAtTime(0.07, T + 0.19); G->Gain.LinearRampToValueAtTime(0, T + 0.44);
	Osc->Connect(Lp); Lp->Connect(G); G->Connect(Out); Osc->Start(T + 0.16); Osc->Stop(T + 0.46);
}

// A small geared motor running for `secs` (the B-12 reference drive).
void US47Audio::Servo(float Secs, TOptional<FVector> At)
{
	if (!Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	const double T = Ctx->CurrentTime();
	auto O = Ctx->CreateOscillator(); O->SetType(TEXT("sawtooth")); O->Frequency.SetValueAtTime(210, T); O->Frequency.LinearRampToValueAtTime(260, T + Secs * 0.5); O->Frequency.LinearRampToValueAtTime(190, T + Secs);
	auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(700);
	auto G = Ctx->CreateGain(); G->Gain.SetValueAtTime(0, T); G->Gain.LinearRampToValueAtTime(0.18, T + 0.08); G->Gain.SetValueAtTime(0.18, T + Secs - 0.1); G->Gain.LinearRampToValueAtTime(0, T + Secs);
	O->Connect(Lp); Lp->Connect(G); G->Connect(At ? TSharedPtr<FS47AudioNode>(Panner(*At, 1.5)) : TSharedPtr<FS47AudioNode>(SfxBus)); O->Start(T); O->Stop(T + Secs + 0.05);
	FS47Sound S; S.Gain = 0.5f; S.When = Secs; S.At = At;
	Sfx(TEXT("switch"), S);
}

// Clicks in the 4 / 7 grouping of the signal.
void US47Audio::Signature(float Gain)
{
	if (!Ctx.IsValid()) return;
	TArray<double> Times;
	for (int32 I = 0; I < 4; ++I) Times.Add(I * 0.22);
	for (int32 I = 0; I < 7; ++I) Times.Add(1.4 + I * 0.22);
	for (const double W : Times) { FS47Sound O; O.Gain = Gain; O.When = float(W); O.Rate = 0.9f; Sfx(TEXT("click"), O); }
}

void US47Audio::Suspend(bool bOn) { if (Ctx.IsValid()) Ctx->Suspend(bOn); }

// ---------- drive/engineSound.ts ----------

void US47Audio::LoopLevel(const FString& Key, float Gain, float Rate, float LowPass)
{
	// the engine's recordings, mixed by rpm each frame (EngineSound): their gain and playback rate
	FS47Play* P = Loops.Find(Key);
	if (!P || !*P || !Ctx.IsValid()) return;
	FBatch Batch(*Ctx);
	if (P->Gain.IsValid()) P->Gain->Gain.SetValue(FMath::Max(0.f, Gain));
	P->Src->PlaybackRate.SetValue(FMath::Max(0.01f, Rate));
	if (P->Filter.IsValid() && LowPass > 0.f) P->Filter->Frequency.SetValue(LowPass);
}

TOptional<FS47LoopState> US47Audio::InspectLoop(const FString& Key) const
{
	const FS47Play* P = Loops.Find(Key);
	if (!P || !*P || !Ctx.IsValid()) return {};
	FBatch Batch(*Ctx);
	FS47LoopState State;
	State.bPlaying = P->Src->Started() && !P->Src->Ended();
	State.bLoop = P->Src->bLoop;
	State.Gain = P->Gain.IsValid() ? P->Gain->Gain.Value() : 0;
	State.Rate = P->Src->PlaybackRate.Value();
	State.LowPass = P->Filter.IsValid() ? P->Filter->Frequency.Value() : 0;
	State.Buffer = P->Src->Buffer;
	return State;
}

float US47Audio::Duration(const FString& Name)
{
	if (const TSharedPtr<FS47AudioBuffer>* B = Buf.Find(Name); B && B->IsValid()) return float((*B)->Duration());
	USoundBase* S = Find(Name);
	return S ? S->GetDuration() : 0.f;
}

// The truck's sounds made with Web Audio nodes in the web version (engineSound.ts): silent until
// they are built on the context here, as the other graphs in this file are.
static void NotYet(const TCHAR* What)
{
	static TSet<FString> Told;
	if (!Told.Contains(What)) { Told.Add(What); UE_LOG(LogTemp, Display, TEXT("S47: audio %s is not ported yet"), What); }
}
void US47Audio::EngineBus(float, float, float) {}
void US47Audio::EngineTyres(const FString&, float) { NotYet(TEXT("engine tyres")); }
void US47Audio::EngineCough(float) { NotYet(TEXT("engine cough")); }
void US47Audio::EngineCrank(float) { NotYet(TEXT("engine crank")); }
void US47Audio::EngineSolenoid(const TArray<float>&, float) { NotYet(TEXT("engine solenoid")); }
void US47Audio::EngineRattle(float) { NotYet(TEXT("engine rattle")); }
void US47Audio::EngineThump(float) { NotYet(TEXT("engine thump")); }
void US47Audio::EngineStop(float) {}
