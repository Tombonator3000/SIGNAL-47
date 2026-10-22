// core/reelTape.ts
#include "S47ReelTape.h"

#include "HAL/PlatformTime.h"
#include "S47Ambience.h"
#include "S47Audio.h"

using namespace S47ReelTape;

namespace
{
	/** Hiss on the tape before the boy's first word. */
	const double LEAD = 1.5;
}

FS47ReelTape::FS47ReelTape(FS47ReelTapeDeps InD) : D(MoveTemp(InD)) {}

TArray<FS47TapeCue> FS47ReelTape::Plan() const
{
	TArray<FS47TapeCue> Out = { { TAPE_BEFORE - 0.55, TEXT("deck") }, { TAPE_BEFORE, TEXT("start") } };
	double At = TAPE_BEFORE + LEAD;
	int32 Hole = 0;
	for (const FS47TapeLine& L : D.BoyCallTimes ? D.BoyCallTimes() : TArray<FS47TapeLine>())
	{
		if (L.bBoy) Out.Add({ At, TEXT("boy"), L.Text, L.Secs });
		else Out.Add({ At, TEXT("hole"), FString(), L.Secs, Hole++ });
		At += L.Secs + L.Gap;
	}
	// after the last word: the line a moment more, the click at the other end, and the deck stopped
	Out.Add({ At + 0.5, TEXT("click") });
	Out.Add({ At + 2.4, TEXT("end") });
	return Out;
}

double FS47ReelTape::Now() const
{
	US47Audio* A = D.Audio.Get();
	return bCtx && A && A->Ctx.IsValid() ? A->Ctx->CurrentTime() : FPlatformTime::Seconds();
}

void FS47ReelTape::Play(TFunction<void(const FS47TapeCue&)> Cue, TFunction<void(bool)> Done)
{
	Stop();
	const int32 G = ++Gen;
	bBusy = true; bVoiced = false;
	const TArray<FS47TapeCue> Cs = Plan();
	// (onDone: if (gen === this.gen || !complete) o.done(complete))
	OnDone = [this, G, Done](bool bComplete) { if ((G == Gen || !bComplete) && Done) Done(bComplete); };
	OnCue = MoveTemp(Cue);
	US47Audio* A = D.Audio.Get();
	const TSharedPtr<FS47AudioContext> Ctx = A && A->Ctx.IsValid() && A->Ctx->State() == TEXT("running") ? A->Ctx : nullptr;
	bCtx = Ctx.IsValid();
	T0 = Now();
	StartedAt = T0; Cues = Cs; K = 0;
	if (Ctx.IsValid())
	{
		// the boy's recordings first (they are small; in the Unreal version they load at once)
		TArray<FString> Texts;
		for (const FS47TapeCue& C : Cs) if (C.Kind == TEXT("boy")) Texts.Add(C.Text);
		TArray<TSharedPtr<FS47AudioBuffer>> Voice;
		if (D.CallerVoice && D.CallerVoice() && D.BoyVoices) Voice = D.BoyVoices(Texts);
		bVoiced = Voice.Num() == Texts.Num() && Texts.Num() > 0 && !Voice.ContainsByPredicate([](const TSharedPtr<FS47AudioBuffer>& B) { return !B.IsValid(); });
		Build(Ctx.ToSharedRef(), T0, Cs, bVoiced ? &Voice : nullptr);
	}
	Update();
}

void FS47ReelTape::Update()
{
	if (!bBusy) return;
	const int32 G = Gen;
	const double T = Now() - T0;
	US47Audio* A = D.Audio.Get();
	while (K < Cues.Num() && Cues[K].At <= T)
	{
		const FS47TapeCue C = Cues[K++];
		// in a hole the night outside goes with it, all but the deck; then back to as it was
		if (C.Kind == TEXT("hole") && bCtx && A && A->Ctx.IsValid())
		{
			const double End = Cues.Last().At;
			if (A->Night.IsValid()) A->Night->Hush(C.Secs, 0.03, 0.015, 0.2);
			TWeakPtr<int32> W = Alive;
			TWeakObjectPtr<US47Audio> Au = A;
			const double After = FMath::Max(0.5, End - C.At - C.Secs + 2);
			A->Ctx->SetTimeout(C.Secs, [this, W, Au, G, After] { if (W.IsValid() && G == Gen && Au.IsValid() && Au->Night.IsValid()) Au->Night->Hush(After, 0.22, 0.05, 2.5); });
		}
		if (OnCue) OnCue(C);
		if (G != Gen) return;   // (stopped from the cue)
		if (C.Kind == TEXT("end"))
		{
			Release(); bBusy = false;
			const TFunction<void(bool)> Dn = MoveTemp(OnDone); OnDone = nullptr;
			if (Dn) Dn(true);
			return;
		}
	}
}

void FS47ReelTape::Stop()
{
	const bool bWas = bBusy;
	++Gen;
	Release(true);
	bBusy = false;
	const TFunction<void(bool)> Dn = MoveTemp(OnDone); OnDone = nullptr;
	if (bWas && Dn) Dn(false);
}

// ---------- the sound ----------

void FS47ReelTape::Build(const TSharedRef<FS47AudioContext>& Ctx, double InT0, const TArray<FS47TapeCue>& InCues, const TArray<TSharedPtr<FS47AudioBuffer>>* Voice)
{
	US47Audio* A = D.Audio.Get();
	FS47AudioContext::FBatch B(*Ctx);
	auto At = [InT0](double S) { return InT0 + S; };
	auto Add = [this](const auto& N) { Nodes.Add(N); return N; };
	const double End = InCues.FindByPredicate([](const FS47TapeCue& C) { return C.Kind == TEXT("end"); })->At, Start = TAPE_BEFORE;
	// the radios and the night held down while it plays (the monitor is not loud)
	DuckedAt = Ctx->CurrentTime();
	if (A->RadioBus.IsValid()) A->RadioBus->Gain.SetTargetAtTime(A->RadioLevel * 0.12, Ctx->CurrentTime(), 0.4);
	if (A->Night.IsValid()) A->Night->Hush(End + 2, 0.22, 0.6, 2.5);
	// the studio's monitor: what is on the tape comes through the band of the telephone line it was
	// taken off, and a small speaker's honk
	const TSharedRef<FS47GainNode> Out = Add(Ctx->CreateGain()); Out->Gain.SetValue(0.85); Out->Connect(A->SfxBus);
	const TSharedRef<FS47BiquadFilterNode> Mon = Add(Ctx->CreateBiquadFilter()); Mon->SetType(TEXT("peaking")); Mon->Frequency.SetValue(1500); Mon->Q.SetValue(0.9); Mon->Gain.SetValue(5); Mon->Connect(Out);
	const TSharedRef<FS47BiquadFilterNode> Lp = Add(Ctx->CreateBiquadFilter()); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(3300); Lp->Connect(Mon);
	const TSharedRef<FS47BiquadFilterNode> Hp = Add(Ctx->CreateBiquadFilter()); Hp->SetType(TEXT("highpass")); Hp->Frequency.SetValue(300); Hp->Connect(Lp);
	// the tape: everything on it goes through here, and where Earl answered tonight it is nothing
	const TSharedRef<FS47GainNode> Tape = Add(Ctx->CreateGain()); Tape->Gain.SetValue(0); Tape->Connect(Hp);
	FS47AudioParam& G = Tape->Gain;
	G.SetValueAtTime(0, At(Start)); G.LinearRampToValueAtTime(1, At(Start + 0.25));
	for (const FS47TapeCue& C : InCues)
	{
		if (C.Kind != TEXT("hole")) continue;
		G.SetTargetAtTime(0, At(C.At), 0.006);
		G.SetTargetAtTime(1, At(C.At + C.Secs), 0.006);
	}
	G.SetTargetAtTime(0, At(End - 0.05), 0.02);
	if (const TSharedPtr<FS47AudioBuffer> Noise = A->NoiseBuffer())
	{
		const TSharedRef<FS47BufferSourceNode> N = Ctx->CreateBufferSource(); N->Buffer = Noise; N->bLoop = true;
		const TSharedRef<FS47GainNode> Ng = Add(Ctx->CreateGain()); Ng->Gain.SetValue(0.06); N->Connect(Ng); Ng->Connect(Tape);
		N->Start(At(Start)); N->Stop(At(End + 0.2)); Srcs.Add(N);
	}
	// the line's hum, faint, on the tape too
	const TSharedRef<FS47OscillatorNode> Hum = Ctx->CreateOscillator(); Hum->Frequency.SetValue(60);
	const TSharedRef<FS47GainNode> Hg = Add(Ctx->CreateGain()); Hg->Gain.SetValue(0.01); Hum->Connect(Hg); Hg->Connect(Tape);
	Hum->Start(At(Start)); Hum->Stop(At(End + 0.2)); Srcs.Add(Hum);
	// the boy, with the little waver an old deck gives a voice
	if (Voice)
	{
		const TSharedRef<FS47GainNode> Vg = Add(Ctx->CreateGain()); Vg->Gain.SetValue(1.15); Vg->Connect(Tape);
		int32 I = 0;
		for (const FS47TapeCue& C : InCues)
		{
			if (C.Kind != TEXT("boy")) continue;
			const TSharedRef<FS47BufferSourceNode> Src = Ctx->CreateBufferSource(); Src->Buffer = (*Voice)[I++];
			const TSharedRef<FS47OscillatorNode> Wow = Ctx->CreateOscillator(); Wow->Frequency.SetValue(0.55);
			const TSharedRef<FS47GainNode> Wg = Add(Ctx->CreateGain()); Wg->Gain.SetValue(0.004); Wow->Connect(Wg); Wg->Connect(Src->PlaybackRate);
			Src->Connect(Vg); Src->Start(At(C.At)); Wow->Start(At(C.At)); Wow->Stop(At(C.At + C.Secs + 0.5));
			Srcs.Add(Src); Srcs.Add(Wow);
		}
	}
	// the click on the line, on the tape
	const FS47TapeCue* Click = InCues.FindByPredicate([](const FS47TapeCue& C) { return C.Kind == TEXT("click"); });
	{
		FS47Sound O; O.Gain = 0.55f; O.Rate = 0.8f; O.When = float(FMath::Max(0.0, At(Click->At) - Ctx->CurrentTime())); O.DestNode = Tape;
		const FS47Play Cl = A->Sfx(TEXT("click"), O);
		if (Cl) { Srcs.Add(Cl.Src); Nodes.Add(Cl.Gain); }
	}
	// the deck itself, in the studio, not on the tape: the transport key, the motor and the reels
	// going round (they go on through the holes), and the key again at the end
	const FString Sw = A->Or(TEXT("small_switch"), TEXT("switch"));
	const double Keys[2][3] = { { Start - 0.55, 0.42, 0.8 }, { End - 0.1, 0.38, 0.72 } };
	for (const auto& Kk : Keys)
	{
		FS47Sound O; O.Gain = float(Kk[1]); O.Rate = float(Kk[2]); O.When = float(FMath::Max(0.0, At(Kk[0]) - Ctx->CurrentTime()));
		const FS47Play S = A->Sfx(Sw, O);
		if (S) { Srcs.Add(S.Src); Nodes.Add(S.Gain); }
	}
	const TSharedRef<FS47OscillatorNode> Motor = Ctx->CreateOscillator(); Motor->SetType(TEXT("sawtooth")); Motor->Frequency.SetValue(22);
	const TSharedRef<FS47BiquadFilterNode> Ml = Add(Ctx->CreateBiquadFilter()); Ml->SetType(TEXT("lowpass")); Ml->Frequency.SetValue(170);
	const TSharedRef<FS47GainNode> Mg = Add(Ctx->CreateGain()); Mg->Gain.SetValue(0);
	Motor->Connect(Ml); Ml->Connect(Mg); Mg->Connect(A->SfxBus);
	Mg->Gain.SetValueAtTime(0, At(Start - 0.5)); Mg->Gain.LinearRampToValueAtTime(0.022, At(Start - 0.1));
	Motor->Frequency.SetValueAtTime(22, At(Start - 0.5)); Motor->Frequency.LinearRampToValueAtTime(38, At(Start - 0.1));
	Mg->Gain.SetTargetAtTime(0, At(End), 0.25); Motor->Frequency.SetTargetAtTime(12, At(End), 0.3);
	Motor->Start(At(Start - 0.5)); Motor->Stop(At(End + 1.6)); Srcs.Add(Motor);
	// the reels' flanges, a soft tick each time round
	for (double T = Start + 0.4; T < End; T += 1.35)
	{
		FS47Sound O; O.Gain = 0.03f; O.Rate = 2.4f; O.Lp = 1800; O.When = float(FMath::Max(0.0, At(T) - Ctx->CurrentTime()));
		const FS47Play R = A->Sfx(TEXT("click"), O);
		if (R) { Srcs.Add(R.Src); Nodes.Add(R.Gain); }
	}
}

void FS47ReelTape::Release(bool bNow)
{
	// lets go of the sound; `now`: at once (stopped early), else the tails ring out by themselves
	US47Audio* A = D.Audio.Get();
	const TSharedPtr<FS47AudioContext> Ctx = A ? A->Ctx : nullptr;
	if (Ctx.IsValid() && DuckedAt)
	{
		FS47AudioContext::FBatch B(*Ctx);
		if (A->RadioBus.IsValid()) A->RadioBus->Gain.SetTargetAtTime(A->RadioLevel, Ctx->CurrentTime(), 0.6);
		if (bNow && A->Night.IsValid()) A->Night->Hush(0.1, 1, 0.3, 1.5);
	}
	DuckedAt.Reset();
	const TArray<TSharedPtr<FS47ScheduledSourceNode>> S = MoveTemp(Srcs);
	const TArray<TSharedPtr<FS47AudioNode>> N = MoveTemp(Nodes);
	Srcs.Reset(); Nodes.Reset();
	auto Go = [S, N]
	{
		for (const TSharedPtr<FS47ScheduledSourceNode>& X : S) { X->OnEnded = nullptr; X->Stop(); X->Disconnect(); }
		for (const TSharedPtr<FS47AudioNode>& X : N) X->Disconnect();
	};
	if (bNow || !Ctx.IsValid()) { if (Ctx.IsValid()) { FS47AudioContext::FBatch B(*Ctx); Go(); } else Go(); }
	else Ctx->SetTimeout(2.0, Go);
}
