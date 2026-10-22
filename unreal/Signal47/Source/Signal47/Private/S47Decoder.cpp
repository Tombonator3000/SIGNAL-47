// core/decoder.ts
#include "S47Decoder.h"

#include "S47Reel047.h"
#include "S47SignalVoice.h"

namespace
{
	// the 4-group rises through a minor seventh chord, the 7-group falls back down
	const double DEG_A[4] = { 0, 3, 7, 10 };
	const double DEG_B[7] = { 12, 10, 7, 5, 3, 2, 0 };
	const double F0 = 220;
}

double FS47DecoderPlayer::FourDegree(int32 I) { return DEG_A[FMath::Clamp(I, 0, 3)]; }
double FS47DecoderPlayer::SevenDegree(int32 I) { return DEG_B[FMath::Clamp(I, 0, 6)]; }

// ---------- (Unreal) the level the VU meters read ----------

void FS47LevelMeter::Process(int64 Quantum)
{
	PullInputs(Quantum, In);
	Out = In;
	double S = 0;
	for (int32 C = 0; C < In.Channels; ++C) for (int32 J = 0; J < S47Quantum; ++J) S += double(In.D[C][J]) * In.D[C][J];
	Sum += S / FMath::Max(1, In.Channels);
	Frames += S47Quantum;
}

double FS47LevelMeter::Take()
{
	FScopeLock L(&Ctx->Lock);
	// (the audio thread renders in blocks, not once a frame: a frame that no block came through keeps
	// the last block's level, else the meters would read silence between the blocks)
	if (Frames > 0) Held = FMath::Sqrt(Sum / double(Frames));
	Sum = 0; Frames = 0;
	return Held;
}

FS47DecoderPlayer::FS47DecoderPlayer(const TSharedRef<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& Dest, const FVector& At, const TSharedPtr<FS47AudioBuffer>& InNoise)
	: Ctx(InCtx), Noise(InNoise)
{
	FS47AudioContext::FBatch B(*Ctx);
	const TSharedRef<FS47PannerNode> Pan = Ctx->CreatePanner();
	Pan->PanningModel = TEXT("equalpower"); Pan->SetDistanceModel(TEXT("inverse")); Pan->RefDistance = 1.6; Pan->RolloffFactor = 1.1; Pan->MaxDistance = 40;
	Pan->PositionX.SetValue(At.X); Pan->PositionY.SetValue(At.Y); Pan->PositionZ.SetValue(At.Z);
	Pan->Connect(Dest);
	// (Unreal: the meter between the output and the panner, for the VU unit; it passes everything on)
	Meter = MakeShared<FS47LevelMeter>(Ctx);
	Meter->Connect(Pan);
	Out = Ctx->CreateGain(); Out->Gain.SetValue(0); Out->Connect(Meter);
	Dry = Ctx->CreateGain(); Dry->Connect(Out);
	Wet = Ctx->CreateGain(); Verb = MakeVerb(3.6);
	Verb->Connect(Out);
}

FS47DecoderPlayer::~FS47DecoderPlayer()
{
	// (Unreal: the reverb's feedback loops hold each other; they are let go once its tail is out)
	const TArray<TSharedPtr<FS47AudioNode>> Loops = VerbLoops;
	Ctx->SetTimeout(5.0, [Loops] { for (const TSharedPtr<FS47AudioNode>& L : Loops) L->Disconnect(); });
}

TSharedPtr<FS47AudioNode> FS47DecoderPlayer::MakeVerb(double Secs)
{
	// impulse(ctx, 3.6): decaying noise, a little darker at the end. (Unreal: no ConvolverNode in
	// S47WebAudio: four feedback combs, low-passed in the loop, falling off over the same seconds, at
	// about the level the browser's normalized convolver gives a noise.)
	const TSharedRef<FS47GainNode> In = Ctx->CreateGain(), Sum = Ctx->CreateGain();
	Sum->Gain.SetValue(0.1);
	const double Delays[4] = { 0.0297, 0.0371, 0.0411, 0.0437 };
	const double T60 = Secs * 0.7;
	for (const double Dl : Delays)
	{
		const TSharedRef<FS47GainNode> Node = Ctx->CreateGain();
		const TSharedRef<FS47DelayNode> Dn = Ctx->CreateDelay(0.1); Dn->DelayTime.SetValue(Dl);
		// (the lowpass's Q is in dB, as the specification has it: at 0 dB its peak just under 4.2 kHz took
		// the loop's gain over one with the feedback, and the tail grew without end, minutes into a
		// playback or seconds into a key's tail, as the VU meters showed. At -6 dB it has no peak and the
		// loop stays under one at every frequency.)
		const TSharedRef<FS47BiquadFilterNode> Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(4200); Lp->Q.SetValue(-6);
		const TSharedRef<FS47GainNode> Fb = Ctx->CreateGain(); Fb->Gain.SetValue(FMath::Pow(10.0, -3 * Dl / T60));
		In->Connect(Node); Node->Connect(Dn); Dn->Connect(Lp); Lp->Connect(Fb); Fb->Connect(Node);
		Lp->Connect(Sum);
		VerbLoops.Add(Fb);
	}
	// (the output: the sum; its input: In)
	Wet->Connect(In);
	return Sum;
}

void FS47DecoderPlayer::Apply(const FS47DecoderSet& P)
{
	if (P.Mode) Params.Mode = *P.Mode;
	if (P.Pitch) Params.Pitch = *P.Pitch;
	if (P.Stretch) Params.Stretch = *P.Stretch;
	if (P.Space) Params.Space = *P.Space;
}

void FS47DecoderPlayer::Start(const FS47DecoderSet& P)
{
	Apply(P);
	StopVoices();
	FS47AudioContext::FBatch B(*Ctx);
	const double Now = Ctx->CurrentTime();
	bPlaying = true;
	bPhrase = false; KeyAt = -1; // (Unreal: the tape over the answer, if it was still playing by itself; a key's light)
	T0 = Now + 0.15; Next = 0; Cycle = 0; Started = Now; Marks.Reset();
	Out->Gain.CancelScheduledValues(Now);
	Ducked = 1;
	Out->Gain.SetTargetAtTime(0.9, Now, 0.2);
	ApplySpace();
	if (Params.Mode == TEXT("raw"))
	{
		Raw = MakeShared<FS47SignalVoice>(Ctx, Dry, Noise);
		Raw->Frame(1, 0.9, 0);
	}
	else StartPad();
}

void FS47DecoderPlayer::Set(const FS47DecoderSet& P)
{
	const FString Mode = Params.Mode;
	Apply(P);
	if (!bPlaying) return;
	if (P.Mode && *P.Mode != Mode) { Start(); return; }
	ApplySpace();
	if (Pad) TunePad();
}

void FS47DecoderPlayer::Stop()
{
	if (!bPlaying && !bPhrase) return;
	bPlaying = false;
	bPhrase = false; // (Unreal: the answer stopped with it; what is scheduled of it plays on silent)
	Current = -1;
	FS47AudioContext::FBatch B(*Ctx);
	Out->Gain.SetTargetAtTime(0, Ctx->CurrentTime(), 0.25);
	const TSharedPtr<FS47SignalVoice> R = Raw;
	const TOptional<FPad> P = Pad;
	Raw.Reset(); Pad.Reset();
	Ctx->SetTimeout(1.2, [R, P] { if (R.IsValid()) R->Stop(); if (P) for (const TSharedPtr<FS47OscillatorNode>& O : P->Oscs) O->Stop(); });
}

void FS47DecoderPlayer::StopVoices()
{
	if (Raw.IsValid()) { Raw->Stop(); Raw.Reset(); }
	if (Pad)
	{
		const FPad P = *Pad;
		Pad.Reset();
		FS47AudioContext::FBatch B(*Ctx);
		P.Gain->Gain.SetTargetAtTime(0, Ctx->CurrentTime(), 0.1);
		Ctx->SetTimeout(0.6, [P] { for (const TSharedPtr<FS47OscillatorNode>& O : P.Oscs) O->Stop(); });
	}
}

void FS47DecoderPlayer::ApplySpace()
{
	const double Now = Ctx->CurrentTime(), S = Params.Space;
	Wet->Gain.SetTargetAtTime(0.15 + 1.1 * S, Now, 0.1);
	Dry->Gain.SetTargetAtTime(1 - 0.45 * S, Now, 0.1);
}

void FS47DecoderPlayer::StartPad()
{
	// a slow pad under the harmonized playback: two detuned saws through a low filter
	FS47AudioContext::FBatch B(*Ctx);
	FPad P;
	P.Gain = Ctx->CreateGain(); P.Gain->Gain.SetValue(0);
	P.Filter = Ctx->CreateBiquadFilter(); P.Filter->SetType(TEXT("lowpass")); P.Filter->Frequency.SetValue(650); P.Filter->Q.SetValue(0.7);
	P.Filter->Connect(P.Gain); P.Gain->Connect(Dry); P.Gain->Connect(Wet);
	for (const double Cents : { 0.0, 7.0, -5.0 })
	{
		const TSharedRef<FS47OscillatorNode> O = Ctx->CreateOscillator(); O->SetType(TEXT("sawtooth")); O->Detune.SetValue(Cents); O->Connect(P.Filter); O->Start();
		P.Oscs.Add(O);
	}
	const TSharedRef<FS47OscillatorNode> Lfo = Ctx->CreateOscillator(); Lfo->Frequency.SetValue(0.07);
	const TSharedRef<FS47GainNode> Ld = Ctx->CreateGain(); Ld->Gain.SetValue(260);
	Lfo->Connect(Ld); Ld->Connect(P.Filter->Frequency); Lfo->Start();
	P.Oscs.Add(Lfo);
	Pad = P;
	TunePad();
}

void FS47DecoderPlayer::TunePad()
{
	const double F = F0 / 2 * FMath::Pow(2.0, Params.Pitch / 12);
	const double Now = Ctx->CurrentTime();
	for (int32 I = 0; I < 3 && I < Pad->Oscs.Num(); ++I) Pad->Oscs[I]->Frequency.SetTargetAtTime(I == 2 ? F * 1.5 : F, Now, 0.3);
}

void FS47DecoderPlayer::Note(double At, double Deg, double Len, bool bFirst, double Kx)
{
	// one note of the harmonized playback: a soft bell (sine, octave and a slightly sharp twelfth),
	// the harmonizer's fifth and octave a few milliseconds late, a bass under the first note of a group
	// (Unreal: Kx, the clock of the answer when it plays by itself; else the panel's TIME)
	const double Stretch = Kx > 0 ? Kx : Params.Stretch;
	const double F = F0 * FMath::Pow(2.0, (Deg + Params.Pitch) / 12);
	auto Voice = [&](double Freq, double Gain, double Delay, double Dur)
	{
		const double T = At + Delay;
		const TSharedRef<FS47GainNode> Env = Ctx->CreateGain(); Env->Gain.SetValueAtTime(0, T);
		Env->Gain.LinearRampToValueAtTime(Gain, T + FMath::Min(0.04, 0.01 * Stretch));
		Env->Gain.ExponentialRampToValueAtTime(0.0008, T + Dur);
		Env->Connect(Dry); Env->Connect(Wet);
		const struct { double Mul, G; const TCHAR* Type; } Parts[3] = { { 1, 1, TEXT("sine") }, { 2, 0.28, TEXT("triangle") }, { 3.004, 0.07, TEXT("sine") } };
		for (const auto& Pt : Parts)
		{
			const TSharedRef<FS47OscillatorNode> O = Ctx->CreateOscillator(); O->SetType(Pt.Type); O->Frequency.SetValue(Freq * Pt.Mul);
			const TSharedRef<FS47GainNode> Og = Ctx->CreateGain(); Og->Gain.SetValue(Pt.G); O->Connect(Og); Og->Connect(Env);
			O->Start(T); O->Stop(T + Dur + 0.05);
		}
	};
	const double Dur = 0.35 + Len * 2.2;
	Voice(F, 0.11, 0, Dur);
	Voice(F * FMath::Pow(2.0, 7.0 / 12), 0.045, 0.012, Dur * 0.9);
	Voice(F * 2, 0.03, 0.019, Dur * 0.8);
	if (bFirst)
	{
		const double T = At;
		const TSharedRef<FS47OscillatorNode> O = Ctx->CreateOscillator(); O->Frequency.SetValue(F0 / 4 * FMath::Pow(2.0, Params.Pitch / 12));
		const TSharedRef<FS47GainNode> G = Ctx->CreateGain(); G->Gain.SetValueAtTime(0, T); G->Gain.LinearRampToValueAtTime(0.16, T + 0.02); G->Gain.ExponentialRampToValueAtTime(0.0008, T + 1.2 * Stretch);
		O->Connect(G); G->Connect(Dry); G->Connect(Wet); O->Start(T); O->Stop(T + 1.3 * Stretch);
	}
	if (Pad) Pad->Gain->Gain.SetTargetAtTime(bFirst ? 0.05 : 0.035, At, 0.4 * Stretch);
}

void FS47DecoderPlayer::Duck(double K)
{
	if ((!bPlaying && !bPhrase) || FMath::IsNearlyEqual(K, Ducked)) return;
	Ducked = K;
	Out->Gain.SetTargetAtTime(0.9 * K, Ctx->CurrentTime(), 0.3);
}

// ---------- (Unreal only, on Tom's request) the ANSWER row's keys, and the answer by itself ----------

void FS47DecoderPlayer::Key(int32 I)
{
	FS47AudioContext::FBatch B(*Ctx);
	const double Now = Ctx->CurrentTime();
	I = FMath::Clamp(I, 0, 6);
	if (!bPlaying && !bPhrase)
	{
		// the output is shut while nothing plays: open for the note and its tail in the reverb, shut after
		Out->Gain.CancelScheduledValues(Now);
		Out->Gain.SetValueAtTime(Out->Gain.Value(), Now);
		Out->Gain.SetTargetAtTime(0.9, Now, 0.01);
		Out->Gain.SetTargetAtTime(0, Now + 0.35 + 0.11 * Params.Stretch * 2.2 + 3.6, 0.5);
		ApplySpace();
		// its bar on the displays while it sounds (the playback lights its own when it plays)
		KeyAt = Now + 0.02; KeyI = 4 + I;
	}
	Note(Now + 0.02, DEG_B[I], 0.11 * Params.Stretch, false);
}

void FS47DecoderPlayer::Phrase(const TArray<double>& Seven, const FS47AnswerTimes& Tm, const TSharedPtr<FS47AudioBuffer>& Voice)
{
	// whatever plays stops where it is (the pad and the raw voice let go), and the processor gives the
	// answer back by itself: the four, the gap, the seven as they were keyed, on a slower clock, with
	// the bass under the first of each group; the reverb opens up to all of it by the last note
	StopVoices();
	bPlaying = false; Current = -1; KeyAt = -1;
	FS47AudioContext::FBatch B(*Ctx);
	const double Now = Ctx->CurrentTime(), K = Tm.K;
	bPhrase = true; Marks.Reset(); Ducked = 1;
	PhraseEnd = Now + Tm.End(); PhraseK = K;
	Out->Gain.CancelScheduledValues(Now);
	Out->Gain.SetValueAtTime(Out->Gain.Value(), Now);
	Out->Gain.SetTargetAtTime(0.9, Now, 0.2);
	const double W0 = 0.15 + 1.1 * Params.Space, D0 = 1 - 0.45 * Params.Space;
	Wet->Gain.CancelScheduledValues(Now); Wet->Gain.SetValueAtTime(W0, Now); Wet->Gain.LinearRampToValueAtTime(1.6, Now + Tm.Last());
	Dry->Gain.CancelScheduledValues(Now); Dry->Gain.SetValueAtTime(D0, Now); Dry->Gain.LinearRampToValueAtTime(0.45, Now + Tm.Last());
	for (int32 I = 0; I < 4; ++I) { Note(Now + Tm.Four(I), DEG_A[I], 0.11 * K, I == 0, K); Marks.Add({ Now + Tm.Four(I), I }); }
	for (int32 J = 0; J < 7 && J < Seven.Num(); ++J) { Note(Now + Tm.Seven(J), Seven[J], 0.11 * K, J == 0, K); Marks.Add({ Now + Tm.Seven(J), 4 + J }); }
	// the room's reverb back as the panel has it once the wire comes (a small sound, close)
	Wet->Gain.SetValueAtTime(1.6, Now + Tm.Wire()); Wet->Gain.LinearRampToValueAtTime(W0 * 0.5, Now + Tm.Wire() + 0.8);
	Dry->Gain.SetValueAtTime(0.45, Now + Tm.Wire()); Dry->Gain.LinearRampToValueAtTime(1, Now + Tm.Wire() + 0.8);
	Wire(Now + Tm.Wire(), Seven, Tm, Voice);
	Out->Gain.SetTargetAtTime(0, Now + Tm.End(), 0.3);
}

void FS47DecoderPlayer::Wire(double At, const TArray<double>& Seven, const FS47AnswerTimes& Tm, const TSharedPtr<FS47AudioBuffer>& Voice)
{
	// TAPE A: a wire recording from 1947, small and close, the field line from C into the hut. Its hiss;
	// someone out on the line whistling the four and the seven as they were keyed; the seven coming back,
	// slow, as the processor has just played them, in the hut and close to its microphone (1947 took in
	// this night: HISTORIE.md); the field telephone rung by its handle
	// (the crank bell reel 047 has, S47Reel047.h); T. Vega's correction, the line's own recording; the
	// cutters, and then nothing on the wire at all, not even the hiss
	auto Off = [&Tm](double T) { return T - Tm.Wire(); };
	const double Crank = Off(Tm.Crank()), Cut = Off(Tm.Cut());
	const TSharedRef<FS47GainNode> Wg = Ctx->CreateGain(); Wg->Gain.SetValue(1);
	const TSharedRef<FS47BiquadFilterNode> Hp = Ctx->CreateBiquadFilter(); Hp->SetType(TEXT("highpass")); Hp->Frequency.SetValue(320);
	const TSharedRef<FS47BiquadFilterNode> Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(3400);
	Wg->Connect(Hp); Hp->Connect(Lp); Lp->Connect(Dry); Lp->Connect(Wet);
	// the cut: everything on the wire stops at once
	Wg->Gain.SetValueAtTime(1, At + Cut + 0.05); Wg->Gain.LinearRampToValueAtTime(0, At + Cut + 0.056);
	if (Noise.IsValid())
	{
		const TSharedRef<FS47BufferSourceNode> N = Ctx->CreateBufferSource(); N->Buffer = Noise; N->bLoop = true;
		const TSharedRef<FS47BiquadFilterNode> Nb = Ctx->CreateBiquadFilter(); Nb->SetType(TEXT("bandpass")); Nb->Frequency.SetValue(2200); Nb->Q.SetValue(0.6);
		const TSharedRef<FS47GainNode> Ng = Ctx->CreateGain(); Ng->Gain.SetValueAtTime(0, At); Ng->Gain.LinearRampToValueAtTime(0.06, At + 0.5);
		N->Connect(Nb); Nb->Connect(Ng); Ng->Connect(Wg); N->Start(At); N->Stop(At + Cut + 0.2);
		// the cutters through copper, close to the microphone: a click and a bright crunch, then the end
		const TSharedRef<FS47BufferSourceNode> C = Ctx->CreateBufferSource(); C->Buffer = Noise;
		const TSharedRef<FS47BiquadFilterNode> Cb = Ctx->CreateBiquadFilter(); Cb->SetType(TEXT("bandpass")); Cb->Frequency.SetValue(3800); Cb->Q.SetValue(1.4);
		const TSharedRef<FS47GainNode> Cg = Ctx->CreateGain(); Cg->Gain.SetValueAtTime(0, At + Cut - 0.04); Cg->Gain.LinearRampToValueAtTime(0.5, At + Cut - 0.035); Cg->Gain.ExponentialRampToValueAtTime(0.001, At + Cut + 0.04);
		C->Connect(Cb); Cb->Connect(Cg); Cg->Connect(Wg); C->Start(At + Cut - 0.04); C->Stop(At + Cut + 0.06);
	}
	// the whistle: a sine with a breath of vibrato and the wire's wow, each note pushed in and let go
	const double W0 = Off(Tm.Whistle(0)) - 0.05, W1 = Off(Tm.Whistle(10)) + 0.35;
	const TSharedRef<FS47OscillatorNode> Wo = Ctx->CreateOscillator(); Wo->SetType(TEXT("sine"));
	const TSharedRef<FS47OscillatorNode> Vib = Ctx->CreateOscillator(); Vib->Frequency.SetValue(5.3);
	const TSharedRef<FS47GainNode> Vd = Ctx->CreateGain(); Vd->Gain.SetValue(14); Vib->Connect(Vd); Vd->Connect(Wo->Detune);
	const TSharedRef<FS47OscillatorNode> Wow = Ctx->CreateOscillator(); Wow->Frequency.SetValue(0.7);
	const TSharedRef<FS47GainNode> Wd = Ctx->CreateGain(); Wd->Gain.SetValue(9); Wow->Connect(Wd); Wd->Connect(Wo->Detune);
	const TSharedRef<FS47GainNode> We = Ctx->CreateGain(); We->Gain.SetValue(0);
	Wo->Connect(We); We->Connect(Wg);
	for (int32 J = 0; J < 11; ++J)
	{
		const double Deg = J < 4 ? DEG_A[J] : Seven.IsValidIndex(J - 4) ? Seven[J - 4] : DEG_B[J - 4];
		const double T = At + Off(Tm.Whistle(J));
		Wo->Frequency.SetValueAtTime(F0 * 4 * FMath::Pow(2.0, (Deg + Params.Pitch) / 12), T);
		We->Gain.SetValueAtTime(0, T); We->Gain.LinearRampToValueAtTime(0.05, T + 0.04);
		We->Gain.SetValueAtTime(0.05, T + 0.22); We->Gain.LinearRampToValueAtTime(0, T + 0.27);
	}
	Wo->Start(At + W0); Wo->Stop(At + W1);
	Vib->Start(At + W0); Vib->Stop(At + W1);
	Wow->Start(At + W0); // (on through the seven back: stopped after them)
	// the seven back: the processor's bells as the wire took them down in the hut, an octave up, thin and
	// louder than the line, with the same wow in them (the wire's speed, not the notes')
	for (int32 J = 0; J < 7; ++J)
	{
		const double Deg = Seven.IsValidIndex(J) ? Seven[J] : DEG_B[J];
		const double T = At + Off(Tm.Back(J)), F = F0 * 2 * FMath::Pow(2.0, (Deg + Params.Pitch) / 12);
		const TSharedRef<FS47GainNode> Env = Ctx->CreateGain(); Env->Gain.SetValueAtTime(0, T);
		Env->Gain.LinearRampToValueAtTime(0.14, T + 0.03); Env->Gain.ExponentialRampToValueAtTime(0.0008, T + 1.6);
		Env->Connect(Wg);
		for (const double Mul : { 1.0, 1.5, 2.0 })
		{
			const TSharedRef<FS47OscillatorNode> O = Ctx->CreateOscillator(); O->Frequency.SetValue(F * Mul);
			const TSharedRef<FS47GainNode> G = Ctx->CreateGain(); G->Gain.SetValue(Mul == 1.0 ? 1.0 : 0.3);
			const TSharedRef<FS47GainNode> Wn = Ctx->CreateGain(); Wn->Gain.SetValue(6); Wow->Connect(Wn); Wn->Connect(O->Detune);
			O->Connect(G); G->Connect(Env); O->Start(T); O->Stop(T + 1.7);
		}
	}
	Wow->Stop(At + Off(Tm.Back(6)) + 1.8);
	// the field telephone, rung by its handle a little way off: two turns of the crank bell
	const TSharedRef<FS47BufferSourceNode> Bell = Ctx->CreateBufferSource(); Bell->Buffer = S47Reel047::CrankBell(*Ctx);
	const TSharedRef<FS47BiquadFilterNode> Bb = Ctx->CreateBiquadFilter(); Bb->SetType(TEXT("bandpass")); Bb->Frequency.SetValue(1300); Bb->Q.SetValue(0.7);
	const TSharedRef<FS47GainNode> Bg = Ctx->CreateGain(); Bg->Gain.SetValue(0.22);
	Bell->Connect(Bb); Bb->Connect(Bg); Bg->Connect(Wg); Bell->Start(At + Crank, 0, 3.0);
	// his correction, the line's own recording (the receiver in the hut has it; E10: the recorded voice
	// at 02:17:00, as the carrier ceased), far down the line; the cutters come in on its last word
	if (Voice.IsValid())
	{
		const TSharedRef<FS47BufferSourceNode> V = Ctx->CreateBufferSource(); V->Buffer = Voice;
		const TSharedRef<FS47GainNode> Vg = Ctx->CreateGain(); Vg->Gain.SetValue(0.55);
		V->Connect(Vg); Vg->Connect(Wg); V->Start(At + Off(Tm.Voice()));
	}
}

void FS47DecoderPlayer::Update()
{
	if (bPhrase)
	{
		// (Unreal) the answer playing by itself: which of its notes sounds now, and its end
		FS47AudioContext::FBatch B(*Ctx);
		const double Now = Ctx->CurrentTime(), K = PhraseK;
		while (Marks.Num() > 1 && Marks[1].At <= Now) Marks.RemoveAt(0);
		const FMark* M = Marks.Num() ? &Marks[0] : nullptr;
		Current = M && M->At <= Now && Now - M->At < 0.22 * K ? M->I : -1;
		if (Now >= PhraseEnd + 0.6) { bPhrase = false; Current = -1; ApplySpace(); }
		return;
	}
	if (!bPlaying)
	{
		// (Unreal) a key's note alone: its bar lit while it sounds
		if (KeyAt >= 0)
		{
			const double Now = Ctx->CurrentTime();
			Current = Now >= KeyAt && Now - KeyAt < 0.22 * Params.Stretch ? KeyI : -1;
			if (Now - KeyAt >= 0.22 * Params.Stretch) KeyAt = -1;
		}
		return;
	}
	FS47AudioContext::FBatch B(*Ctx);
	const double Now = Ctx->CurrentTime(), K = Params.Stretch;
	if (Raw.IsValid())
	{
		// the tape, slowed down: the same signal on a slower clock
		Raw->Frame(1, 0.9, (Now - Started) / K);
		const S47SignalVoice::FPulse P = S47SignalVoice::PulseAt((Now - Started) / K);
		Current = P.bGroup ? 0 : -1;
		return;
	}
	const double Period = 5.2 * K;
	while (true)
	{
		const int32 I = Next;
		const double Local = I < 4 ? I * 0.22 : 2.1 + (I - 4) * 0.22;
		const double At = T0 + Local * K;
		if (At > Now + 0.25) break;
		if (At >= Now - 0.05)
		{
			Note(At, I < 4 ? DEG_A[I] : DEG_B[I - 4], 0.11 * K, I == 0 || I == 4);
			Marks.Add({ At, I });
		}
		Next++;
		if (Next > 10) { Next = 0; Cycle++; T0 += Period; }
	}
	while (Marks.Num() > 1 && Marks[1].At <= Now) Marks.RemoveAt(0);
	const FMark* M = Marks.Num() ? &Marks[0] : nullptr;
	Current = M && M->At <= Now && Now - M->At < 0.22 * K ? M->I : -1;
}
