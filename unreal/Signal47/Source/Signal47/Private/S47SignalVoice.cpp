// web/src/core/signalVoice.ts
#include "S47SignalVoice.h"

S47SignalVoice::FPulse S47SignalVoice::PulseAt(double T)
{
	const double C = FMath::Fmod(FMath::Fmod(T, 5.2) + 5.2, 5.2);
	const int32 Count = C < 1.6 ? 4 : 7;
	const double Local = C < 1.6 ? C : C - 2.1;
	FPulse P;
	if (Local < 0) return P;
	P.bGroup = Local < Count * 0.22;
	P.bOn = P.bGroup && FMath::Fmod(Local, 0.22) < 0.11;
	return P;
}

TArray<float> S47SignalVoice::Drive(double K)
{
	const int32 N = 1024;
	TArray<float> Curve;
	Curve.SetNumUninitialized(N);
	for (int32 I = 0; I < N; ++I) { const double X = double(I) / (N - 1) * 2 - 1; Curve[I] = float(FMath::Tanh(K * X) / FMath::Tanh(K)); }
	return Curve;
}

FS47SignalVoice::FS47SignalVoice(const TSharedRef<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& Dest, const TSharedPtr<FS47AudioBuffer>& Noise, double InRate)
	: Ctx(InCtx), Rate(FMath::Clamp(InRate, 0.1, 4.0))
{
	FS47AudioContext::FBatch Batch(*Ctx);
	FS47AudioContext& C = *Ctx;
	Out = C.CreateGain(); Out->Gain.SetValue(0); Out->Connect(Dest);
	// a soft limiter so the thumps can be heavy without clipping the mix
	auto Comp = C.CreateDynamicsCompressor(); Comp->Threshold.SetValue(-16); Comp->Ratio.SetValue(5); Comp->Attack.SetValue(0.003); Comp->Release.SetValue(0.2);
	Comp->Connect(Out);
	Shaper = C.CreateWaveShaper(); Shaper->Curve = S47SignalVoice::Drive(2.6); Shaper->Oversample = TEXT("2x");

	// the array's hiss, which a slow tremolo turns into a pulsing interference as clarity rises
	auto N = C.CreateBufferSource(); N->Buffer = Noise; N->bLoop = true;
	auto Bp = C.CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(1700); Bp->Q.SetValue(0.55);
	StatGate = C.CreateGain(); StatGate->Gain.SetValue(1);
	Stat = C.CreateGain(); Stat->Gain.SetValue(0);
	N->Connect(Bp); Bp->Connect(StatGate); StatGate->Connect(Stat); Stat->Connect(Comp);
	auto Trem = C.CreateOscillator(); Trem->Frequency.SetValue(4.55);
	Rates(N, N->PlaybackRate, 1); Rates(Bp, Bp->Frequency, 1700); Rates(Trem, Trem->Frequency, 4.55);
	TremDepth = C.CreateGain(); TremDepth->Gain.SetValue(0); Trem->Connect(TremDepth); TremDepth->Connect(StatGate->Gain);

	// the whine: two detuned buzzing oscillators and a comb-filtered noise, through a
	// resonant band that wanders up and down, ring-modulated for the metal in it
	Whine = C.CreateGain(); Whine->Gain.SetValue(0);
	WhineBand = C.CreateBiquadFilter(); WhineBand->SetType(TEXT("bandpass")); WhineBand->Frequency.SetValue(1900); WhineBand->Q.SetValue(7);
	auto Ring = C.CreateGain(); Ring->Gain.SetValue(0.55);
	auto RingOsc = C.CreateOscillator(); RingOsc->Frequency.SetValue(37); auto RingDepth = C.CreateGain(); RingDepth->Gain.SetValue(0.45); RingOsc->Connect(RingDepth); RingDepth->Connect(Ring->Gain);
	auto O1 = C.CreateOscillator(); O1->SetType(TEXT("sawtooth")); O1->Frequency.SetValue(1460);
	auto O2 = C.CreateOscillator(); O2->SetType(TEXT("square")); O2->Frequency.SetValue(1473);
	auto Og = C.CreateGain(); Og->Gain.SetValue(0.18); O1->Connect(Og); O2->Connect(Og);
	auto Pitch = C.CreateOscillator(); Pitch->Frequency.SetValue(0.21); auto Pd = C.CreateGain(); Pd->Gain.SetValue(260);
	Pitch->Connect(Pd); Pd->Connect(O1->Frequency); Pd->Connect(O2->Frequency);
	auto Sweep = C.CreateOscillator(); Sweep->Frequency.SetValue(0.13); auto Sd = C.CreateGain(); Sd->Gain.SetValue(650);
	Sweep->Connect(Sd); Sd->Connect(WhineBand->Frequency);
	Rates(WhineBand.ToSharedRef(), WhineBand->Frequency, 1900); Rates(RingOsc, RingOsc->Frequency, 37); Rates(O1, O1->Frequency, 1460); Rates(O2, O2->Frequency, 1473);
	Rates(Pitch, Pitch->Frequency, 0.21); Rates(Pd, Pd->Gain, 260); Rates(Sweep, Sweep->Frequency, 0.13); Rates(Sd, Sd->Gain, 650);
	// metal scraped slowly: noise in a short comb whose length drifts
	auto Sn = C.CreateBufferSource(); Sn->Buffer = Noise; Sn->bLoop = true; Sn->PlaybackRate.SetValue(0.37);
	auto CombDelay = C.CreateDelay(0.05); CombDelay->DelayTime.SetValue(0.0027);
	auto Fb = C.CreateGain(); Fb->Gain.SetValue(0.86);
	auto Cd = C.CreateOscillator(); Cd->Frequency.SetValue(0.09); auto Cdd = C.CreateGain(); Cdd->Gain.SetValue(0.0009); Cd->Connect(Cdd); Cdd->Connect(CombDelay->DelayTime);
	auto Sg = C.CreateGain(); Sg->Gain.SetValue(0.22);
	Sn->Connect(CombDelay); CombDelay->Connect(Fb); Fb->Connect(CombDelay); CombDelay->Connect(Sg);
	Rates(Sn, Sn->PlaybackRate, 0.37); Rates(CombDelay, CombDelay->DelayTime, 0.0027, true); Rates(Cd, Cd->Frequency, 0.09); Rates(Cdd, Cdd->Gain, 0.0009, true);
	Og->Connect(WhineBand); Sg->Connect(WhineBand);
	WhineBand->Connect(Ring); Ring->Connect(Whine); Whine->Connect(Comp);
	Comb = Fb;

	// the thumps go through the drive, then the limiter
	ThumpBus = C.CreateGain(); ThumpBus->Gain.SetValue(1);
	ThumpBus->Connect(Shaper); Shaper->Connect(Comp);

	for (const TSharedPtr<FS47ScheduledSourceNode>& S : TArray<TSharedPtr<FS47ScheduledSourceNode>>{ N, Trem, RingOsc, O1, O2, Pitch, Sweep, Sn, Cd }) { S->Start(); Nodes.Add(S); }
}

void FS47SignalVoice::Rates(const TSharedRef<FS47AudioNode>& Node, FS47AudioParam& P, double Base, bool bTime)
{
	Rated.Add({ &P, Node, Base, bTime });
	P.SetValue(bTime ? Base / Rate : Base * Rate);
}

void FS47SignalVoice::SetRate(double InRate)
{
	InRate = FMath::Clamp(InRate, 0.1, 4.0);
	if (FMath::IsNearlyEqual(InRate, Rate)) return;
	Rate = InRate;
	FS47AudioContext::FBatch Batch(*Ctx);
	const double Now = Ctx->CurrentTime();
	for (const FRated& R : Rated) R.P->SetTargetAtTime(R.bTime ? R.Base / Rate : R.Base * Rate, Now, 0.25);
}

void FS47SignalVoice::Frame(double InClarity, double InLevel, double T)
{
	FS47AudioContext::FBatch Batch(*Ctx);
	const double Now = Ctx->CurrentTime();
	Clarity = InClarity;
	const S47SignalVoice::FPulse P = S47SignalVoice::PulseAt(T);
	Out->Gain.SetTargetAtTime(InLevel, Now, 0.12);
	// static: plain at first, then chopped into a pulse as the structure comes through
	Stat->Gain.SetTargetAtTime(0.05 * (1 - 0.65 * Clarity), Now, 0.1);
	TremDepth->Gain.SetTargetAtTime(0.75 * Clarity, Now, 0.2);
	// the whine grows with clarity and swells inside a pulse group
	Whine->Gain.SetTargetAtTime(0.05 * FMath::Pow(Clarity, 1.6) * (P.bGroup ? 1 : 0.55), Now, 0.08);
	if (P.bOn && !bLast && Clarity > 0.12) Thump(0.85 * Clarity * Clarity);
	bLast = P.bOn;
}

// One heavy pulse: a sine that falls from 118 to 41 Hz, a second one an octave up for
// the punch, and a dull clank of filtered noise, all through the drive.
void FS47SignalVoice::Thump(double Strength)
{
	FS47AudioContext& C = *Ctx;
	const double T = C.CurrentTime() + 0.005;
	// (Unreal: R, the tape's speed; 1 as the web version has it, slower and lower below 1)
	const double R = Rate;
	auto Env = C.CreateGain(); Env->Gain.SetValueAtTime(0, T);
	Env->Gain.LinearRampToValueAtTime(Strength, T + 0.006 / R); Env->Gain.ExponentialRampToValueAtTime(0.001, T + 0.34 / R);
	Env->Connect(ThumpBus);
	const double Parts[2][3] = { { 118, 41, 1 }, { 236, 82, 0.35 } };
	for (const auto& Part : Parts)
	{
		auto O = C.CreateOscillator(); O->Frequency.SetValueAtTime(Part[0] * R, T); O->Frequency.ExponentialRampToValueAtTime(Part[1] * R, T + 0.16 / R);
		auto Og = C.CreateGain(); Og->Gain.SetValue(Part[2]); O->Connect(Og); Og->Connect(Env);
		O->Start(T); O->Stop(T + 0.36 / R);
	}
	auto N = C.CreateBufferSource(); N->Buffer = NoiseBuf(); N->PlaybackRate.SetValue(R);
	auto Bp = C.CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(310 * R); Bp->Q.SetValue(2.2);
	auto Ng = C.CreateGain(); Ng->Gain.SetValueAtTime(0.5 * Strength, T); Ng->Gain.ExponentialRampToValueAtTime(0.001, T + 0.09 / R);
	N->Connect(Bp); Bp->Connect(Ng); Ng->Connect(ThumpBus);
	N->Start(T); N->Stop(T + 0.1 / R);
}

TSharedPtr<FS47AudioBuffer> FS47SignalVoice::NoiseBuf()
{
	if (Nb.IsValid()) return Nb;
	const FS47AudioContext& C = *Ctx;
	TSharedRef<FS47AudioBuffer> B = C.CreateBuffer(1, int32(C.SampleRate * 0.2), C.SampleRate);
	float* D = B->GetChannelData(0);
	for (int32 I = 0; I < B->Length(); ++I) D[I] = FMath::FRand() * 2 - 1;
	return Nb = B;
}

void FS47SignalVoice::Silence()
{
	Out->Gain.SetTargetAtTime(0, Ctx->CurrentTime(), 0.2);
	Clarity = 0;
}

void FS47SignalVoice::Stop()
{
	FS47AudioContext::FBatch Batch(*Ctx);
	Silence();
	TArray<TSharedPtr<FS47ScheduledSourceNode>> Old = MoveTemp(Nodes);
	Nodes.Reset();
	TSharedPtr<FS47GainNode> O = Out;
	TSharedPtr<FS47AudioNode> Loop = Comb;
	Ctx->SetTimeout(0.9, [Old, O, Loop]
	{
		for (const TSharedPtr<FS47ScheduledSourceNode>& S : Old) S->Stop();
		O->Disconnect();
		if (Loop.IsValid()) Loop->Disconnect();
	});
}
