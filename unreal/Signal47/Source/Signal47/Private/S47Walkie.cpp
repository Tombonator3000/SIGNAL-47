// core/walkie.ts
#include "S47Walkie.h"

TArray<double> S47Walkie::FourSeven(double At)
{
	TArray<double> Times;
	for (int32 I = 0; I < 4; ++I) Times.Add(At + I * RELAY_GAP);
	for (int32 I = 0; I < 7; ++I) Times.Add(At + 4 * RELAY_GAP + RELAY_PAUSE + I * RELAY_GAP);
	return Times;
}

void S47Walkie::RelayClick(const TSharedRef<FS47AudioContext>& Ctx, const TSharedPtr<FS47AudioNode>& Dest, const TSharedPtr<FS47AudioBuffer>& Noise, double T, double K)
{
	FS47AudioContext::FBatch B(*Ctx);
	auto N = Ctx->CreateBufferSource(); N->Buffer = Noise;
	auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(2600); Bp->Q.SetValue(1.4);
	auto G = Ctx->CreateGain(); G->Gain.SetValueAtTime(0, T); G->Gain.LinearRampToValueAtTime(0.9 * K, T + 0.002); G->Gain.ExponentialRampToValueAtTime(0.001, T + 0.03);
	N->Connect(Bp); Bp->Connect(G); G->Connect(Dest);
	N->Start(T, FMath::FRand()); N->Stop(T + 0.05);
	// and the thump of the armature behind it
	auto O = Ctx->CreateOscillator(); O->Frequency.SetValueAtTime(150, T); O->Frequency.ExponentialRampToValueAtTime(60, T + 0.04);
	auto Og = Ctx->CreateGain(); Og->Gain.SetValueAtTime(0.5 * K, T); Og->Gain.ExponentialRampToValueAtTime(0.001, T + 0.05);
	O->Connect(Og); Og->Connect(Dest); O->Start(T); O->Stop(T + 0.06);
}

void S47Walkie::Breath(const TSharedRef<FS47AudioContext>& Ctx, const TSharedPtr<FS47AudioNode>& Dest, const TSharedPtr<FS47AudioBuffer>& Noise, double At, double K)
{
	FS47AudioContext::FBatch B(*Ctx);
	auto N = Ctx->CreateBufferSource(); N->Buffer = Noise;
	auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Q.SetValue(0.9);
	Bp->Frequency.SetValueAtTime(650, At); Bp->Frequency.LinearRampToValueAtTime(900, At + 0.8); Bp->Frequency.LinearRampToValueAtTime(560, At + 1.3);
	auto G = Ctx->CreateGain();
	FS47AudioParam& V = G->Gain;
	V.SetValueAtTime(0, At); V.LinearRampToValueAtTime(0.32 * K, At + 0.55);   // in
	V.LinearRampToValueAtTime(0.06 * K, At + 0.75); V.LinearRampToValueAtTime(0.26 * K, At + 0.95); // out
	V.LinearRampToValueAtTime(0, At + 1.3);
	N->Connect(Bp); Bp->Connect(G); G->Connect(Dest);
	N->Start(At, FMath::FRand()); N->Stop(At + 1.4);
}

// ---------- WalkieSound ----------

void FS47WalkieSound::Build()
{
	FS47AudioContext::FBatch B(*Ctx);
	auto O = Ctx->CreateGain(); O->Gain.SetValue(0.9);
	auto Hp = Ctx->CreateBiquadFilter(); Hp->SetType(TEXT("highpass")); Hp->Frequency.SetValue(420);
	auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(2900);
	auto Sh = Ctx->CreateWaveShaper();
	TArray<float> Curve;
	Curve.SetNum(256);
	for (int32 I = 0; I < 256; ++I) { const double X = I / 127.5 - 1; Curve[I] = float(FMath::Tanh(X * 2.2) / FMath::Tanh(2.2)); }
	Sh->Curve = Curve;
	Hp->Connect(Lp); Lp->Connect(Sh); Sh->Connect(O); O->Connect(Dest);
	Out = O; Speaker = Hp;
	// the hiss runs all the time the radio is on; the sequence opens and shuts it
	auto N = NoiseSrc();
	auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(2000); Bp->Q.SetValue(0.6);
	auto H = Ctx->CreateGain(); H->Gain.SetValue(0);
	N->Connect(Bp); Bp->Connect(H); H->Connect(Hp);
	N->Start();
	Hiss = H;
}

TSharedRef<FS47BufferSourceNode> FS47WalkieSound::NoiseSrc()
{
	auto S = Ctx->CreateBufferSource(); S->Buffer = Noise; S->bLoop = true; Srcs.Add(S); return S;
}

double FS47WalkieSound::Start()
{
	Stop();
	FS47AudioContext::FBatch B(*Ctx);
	Build();
	const double T0 = Ctx->CurrentTime();
	FS47AudioParam& H = Hiss->Gain;
	const TSharedPtr<FS47AudioNode> Sp = Speaker;
	H.SetValueAtTime(0, T0); H.LinearRampToValueAtTime(0.22, T0 + 2.6);
	H.SetValueAtTime(0.22, T0 + 6.95); H.LinearRampToValueAtTime(0, T0 + 7.1);   // gone with the engine
	H.SetValueAtTime(0, T0 + 16.4); H.LinearRampToValueAtTime(0.13, T0 + 17.2);

	// the engine: a low saw chopped at the firing rate, through a muffled exhaust
	auto Osc = Ctx->CreateOscillator(); Osc->SetType(TEXT("sawtooth"));
	auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(520);
	auto Fire = Ctx->CreateOscillator(); Fire->SetType(TEXT("square")); Fire->Frequency.SetValue(17);
	auto Fg = Ctx->CreateGain(); Fg->Gain.SetValue(0.45); Fire->Connect(Fg);
	auto Chop = Ctx->CreateGain(); Chop->Gain.SetValue(0.55); Fg->Connect(Chop->Gain);
	auto Eng = Ctx->CreateGain(); Eng->Gain.SetValue(0);
	Osc->Connect(Lp); Lp->Connect(Chop); Chop->Connect(Eng); Eng->Connect(Sp);
	FS47AudioParam& F = Osc->Frequency;
	FS47AudioParam& G = Eng->Gain;
	FS47AudioParam& Ff = Fire->Frequency;
	F.SetValueAtTime(34, T0); G.SetValueAtTime(0, T0 + 3.0); G.LinearRampToValueAtTime(0.42, T0 + 3.7);
	for (const double C : { 5.0, 5.9 })                  // two coughs
	{
		G.SetValueAtTime(0.42, T0 + C); G.LinearRampToValueAtTime(0.04, T0 + C + 0.08); G.LinearRampToValueAtTime(0.42, T0 + C + 0.3);
		F.SetValueAtTime(34, T0 + C); F.LinearRampToValueAtTime(24, T0 + C + 0.12); F.LinearRampToValueAtTime(34, T0 + C + 0.35);
		Ff.SetValueAtTime(17, T0 + C); Ff.LinearRampToValueAtTime(11, T0 + C + 0.12); Ff.LinearRampToValueAtTime(17, T0 + C + 0.35);
	}
	F.SetValueAtTime(34, T0 + 6.4); F.LinearRampToValueAtTime(15, T0 + 7.0);  // and it dies
	Ff.SetValueAtTime(17, T0 + 6.4); Ff.LinearRampToValueAtTime(6, T0 + 7.0);
	G.SetValueAtTime(0.42, T0 + 6.4); G.LinearRampToValueAtTime(0, T0 + 7.05);
	Osc->Start(T0); Fire->Start(T0); Osc->Stop(T0 + 7.2); Fire->Stop(T0 + 7.2);
	Srcs.Add(Osc); Srcs.Add(Fire);

	// two seconds of nothing, then the relay: four, a gap, seven
	Clicks(T0 + 9.0);
	// somebody breathing, close to the microphone, twice
	BreathAt(T0 + 13.7); BreathAt(T0 + 15.1);
	return 17.2;
}

void FS47WalkieSound::Clicks(double At)
{
	if (!Speaker.IsValid()) return;
	for (const double T : S47Walkie::FourSeven(At)) S47Walkie::RelayClick(Ctx, Speaker, Noise, T);
}

void FS47WalkieSound::BreathAt(double At)
{
	if (!Speaker.IsValid()) return;
	S47Walkie::Breath(Ctx, Speaker, Noise, At);
}

double FS47WalkieSound::PressToTalk()
{
	if (!Out.IsValid() || !Hiss.IsValid()) return 0;
	FS47AudioContext::FBatch B(*Ctx);
	const double T0 = Ctx->CurrentTime();
	FS47AudioParam& H = Hiss->Gain;
	H.CancelScheduledValues(T0); H.SetValueAtTime(H.Value(), T0); H.LinearRampToValueAtTime(0, T0 + 0.05);
	Clicks(T0 + 2.3);
	H.SetValueAtTime(0, T0 + 7.0); H.LinearRampToValueAtTime(0.13, T0 + 7.6);
	return 7.6;
}

void FS47WalkieSound::Stop()
{
	const TSharedPtr<FS47GainNode> O = Out;
	if (!O.IsValid()) return;
	FS47AudioContext::FBatch B(*Ctx);
	const double T = Ctx->CurrentTime();
	O->Gain.SetTargetAtTime(0, T, 0.03);
	TArray<TSharedPtr<FS47ScheduledSourceNode>> Old = MoveTemp(Srcs);
	Srcs.Reset();
	Ctx->SetTimeout(0.3, [Old, O]
	{
		for (const TSharedPtr<FS47ScheduledSourceNode>& S : Old) S->Stop(); // (one already stopped: nothing)
		O->Disconnect();
	});
	Out.Reset(); Hiss.Reset(); Speaker.Reset();
}

// ---------- self test ----------

bool S47Walkie::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s walkie: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	const TArray<double> T = FourSeven(9.0);
	Check(T.Num() == 11 && FMath::IsNearlyEqual(T[3], 9.96) && FMath::IsNearlyEqual(T[4], 9.0 + 1.28 + 0.9) && FMath::IsNearlyEqual(T[10], 9.0 + 1.28 + 0.9 + 6 * 0.32), TEXT("four, a gap, seven"));
	// the sequence on a context of its own, rendered offline: the hiss and the engine, then silence
	// with the engine gone, then the clicks
	TSharedRef<FS47AudioContext> Ctx = FS47AudioContext::Make(48000.f);
	TSharedRef<FS47AudioBuffer> Noise = Ctx->CreateBuffer(1, 48000 * 2, 48000.f);
	FRandomStream R(47);
	for (int32 I = 0; I < Noise->Length(); ++I) Noise->GetChannelData(0)[I] = R.FRand() * 2 - 1;
	FS47WalkieSound W(Ctx, Ctx->Destination, Noise);
	Check(W.Start() == 17.2 && W.Active(), TEXT("switched on: 17.2 s"));
	auto Rms = [&](double From, double To)
	{
		TArray<float> Buf;
		const int32 Frames = int32((To - From) * 48000);
		Buf.SetNumZeroed(Frames * 2);
		Ctx->Render(Buf.GetData(), Frames, 2);
		double S = 0;
		for (float V : Buf) S += V * V;
		return FMath::Sqrt(S / FMath::Max(1, Buf.Num()));
	};
	const double Hiss = Rms(0, 2.5), Engine = Rms(2.5, 7.4), Gone = Rms(7.4, 8.9), Clicks = Rms(8.9, 13.0);
	Check(Engine > Gone * 4 && Hiss > Gone * 2 && Clicks > Gone * 2, FString::Printf(TEXT("hiss %.4f, engine %.4f, silence %.4f, clicks %.4f"), Hiss, Engine, Gone, Clicks));
	Check(W.PressToTalk() == 7.6, TEXT("press to talk: 7.6 s"));
	W.Stop();
	Check(!W.Active() && W.PressToTalk() == 0, TEXT("off: nothing to press"));
	Ctx->Close();
	return bOk;
}
