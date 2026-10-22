// core/decoder.ts
#include "S47Decoder.h"

#include "S47SignalVoice.h"

namespace
{
	// the 4-group rises through a minor seventh chord, the 7-group falls back down
	const double DEG_A[4] = { 0, 3, 7, 10 };
	const double DEG_B[7] = { 12, 10, 7, 5, 3, 2, 0 };
	const double F0 = 220;
}

FS47DecoderPlayer::FS47DecoderPlayer(const TSharedRef<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& Dest, const FVector& At, const TSharedPtr<FS47AudioBuffer>& InNoise)
	: Ctx(InCtx), Noise(InNoise)
{
	FS47AudioContext::FBatch B(*Ctx);
	const TSharedRef<FS47PannerNode> Pan = Ctx->CreatePanner();
	Pan->PanningModel = TEXT("equalpower"); Pan->SetDistanceModel(TEXT("inverse")); Pan->RefDistance = 1.6; Pan->RolloffFactor = 1.1; Pan->MaxDistance = 40;
	Pan->PositionX.SetValue(At.X); Pan->PositionY.SetValue(At.Y); Pan->PositionZ.SetValue(At.Z);
	Pan->Connect(Dest);
	Out = Ctx->CreateGain(); Out->Gain.SetValue(0); Out->Connect(Pan);
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
		const TSharedRef<FS47BiquadFilterNode> Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(4200); Lp->Q.SetValue(0);
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
	if (!bPlaying) return;
	bPlaying = false;
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

void FS47DecoderPlayer::Note(double At, double Deg, double Len, bool bFirst)
{
	// one note of the harmonized playback: a soft bell (sine, octave and a slightly sharp twelfth),
	// the harmonizer's fifth and octave a few milliseconds late, a bass under the first note of a group
	const double F = F0 * FMath::Pow(2.0, (Deg + Params.Pitch) / 12);
	auto Voice = [&](double Freq, double Gain, double Delay, double Dur)
	{
		const double T = At + Delay;
		const TSharedRef<FS47GainNode> Env = Ctx->CreateGain(); Env->Gain.SetValueAtTime(0, T);
		Env->Gain.LinearRampToValueAtTime(Gain, T + FMath::Min(0.04, 0.01 * Params.Stretch));
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
		const TSharedRef<FS47GainNode> G = Ctx->CreateGain(); G->Gain.SetValueAtTime(0, T); G->Gain.LinearRampToValueAtTime(0.16, T + 0.02); G->Gain.ExponentialRampToValueAtTime(0.0008, T + 1.2 * Params.Stretch);
		O->Connect(G); G->Connect(Dry); G->Connect(Wet); O->Start(T); O->Stop(T + 1.3 * Params.Stretch);
	}
	if (Pad) Pad->Gain->Gain.SetTargetAtTime(bFirst ? 0.05 : 0.035, At, 0.4 * Params.Stretch);
}

void FS47DecoderPlayer::Duck(double K)
{
	if (!bPlaying || FMath::IsNearlyEqual(K, Ducked)) return;
	Ducked = K;
	Out->Gain.SetTargetAtTime(0.9 * K, Ctx->CurrentTime(), 0.3);
}

void FS47DecoderPlayer::Update()
{
	if (!bPlaying) return;
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
