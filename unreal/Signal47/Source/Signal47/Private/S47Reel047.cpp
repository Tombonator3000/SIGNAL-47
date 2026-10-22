// story/reel047.ts
#include "S47Reel047.h"

#include "Brushes/SlateColorBrush.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/App.h"
#include "Rendering/DrawElements.h"
#include "RenderingThread.h"
#include "S47Audio.h"
#include "S47Decoder.h"
#include "Slate/WidgetRenderer.h"
#include "SS47Widgets.h"
#include "Widgets/SLeafWidget.h"

namespace
{
	const double PULSE_END = 13.8;
	// story/officePhone.ts
	const double BELL_LOOP = 7.2;
	const double BURSTS[3][2] = { { 0, 1.4 }, { 1.95, 2.6 }, { 3.1, 4.4 } };
}

const TArray<FS47ReelCue>& S47Reel047::REEL_PLAN()
{
	static const TArray<FS47ReelCue> P = {
		{ 0, TEXT("thread") },
		{ 2.2, TEXT("start") },
		{ 3.4, TEXT("pulse") },
		{ 14.2, TEXT("crank") },
		{ 17.6, TEXT("cut") },
		{ 18.6, TEXT("hole") },
		{ 18.6 + 47, TEXT("relay") },
		{ 18.6 + 47 + 0.9, TEXT("back") },
		{ 18.6 + 47 + 1.4, TEXT("range") },
		{ 18.6 + 47 + 3.2, TEXT("leader") },
		{ 18.6 + 47 + 5.2, TEXT("end") },
	};
	return P;
}

TSharedRef<FS47AudioBuffer> S47Reel047::CrankBell(const FS47AudioContext& Ctx)
{
	const int32 Sr = 16000, Len = FMath::RoundToInt(Sr * BELL_LOOP);   // (the top partial is near 6.1 kHz)
	const TSharedRef<FS47AudioBuffer> Buf = Ctx.CreateBuffer(1, Len, Sr);
	float* Out = Buf->GetChannelData(0);
	for (int32 I = 0; I < Len; ++I) Out[I] = 0;
	// [ratio, level, seconds to fall to 1/e]: a small hemispherical gong
	const double PARTS[5][3] = { { 1, 1, 0.5 }, { 2.04, 0.5, 0.32 }, { 2.83, 0.38, 0.24 }, { 4.16, 0.22, 0.15 }, { 5.43, 0.13, 0.1 } };
	struct FRes { double C1, C2, A, Y1 = 0, Y2 = 0; };
	TArray<FRes> Gongs[2];
	const double F0s[2] = { 1047, 1123 };
	for (int32 G = 0; G < 2; ++G) for (const auto& P : PARTS)
	{
		const double W = 2 * UE_DOUBLE_PI * F0s[G] * P[0] / Sr, R = FMath::Exp(-1 / (P[2] * Sr));
		Gongs[G].Add({ 2 * R * FMath::Cos(W), -R * R, P[1] * FMath::Sin(W) });
	}
	// where the clapper strikes, gong by gong (a little uneven: a hand on a crank)
	TArray<float> Hits; Hits.Init(0, Len * 2);
	int32 Side = 0; int64 Seed = 7;
	auto Rnd = [&Seed] { Seed = (Seed * 16807) % 2147483647; return double(Seed) / 2147483647; };
	for (const auto& B : BURSTS)
	{
		for (double T = B[0]; T < B[1];)
		{
			const double K = (T - B[0]) / (B[1] - B[0]), F = 14 + 6 * FMath::Sin(UE_DOUBLE_PI * FMath::Min(1.0, K * 1.15));
			const int32 I = FMath::RoundToInt(T * Sr);
			if (I < Len) Hits[I * 2 + Side] = float(0.75 + 0.25 * Rnd());
			Side ^= 1;
			T += 1 / (2 * F);
		}
	}
	double Peak = 0;
	for (int32 I = 0; I < Len; ++I)
	{
		double Y = 0;
		for (int32 G = 0; G < 2; ++G)
		{
			const double X = Hits[I * 2 + G];
			for (FRes& P : Gongs[G])
			{
				const double V = P.C1 * P.Y1 + P.C2 * P.Y2 + X * P.A;
				P.Y2 = P.Y1; P.Y1 = V; Y += V;
			}
			// the clapper's own tick on the metal
			if (X) for (int32 J = 0; J < 24 && I + J < Len; ++J) Out[I + J] += float((Rnd() * 2 - 1) * 0.35 * X * (1 - J / 24.0));
		}
		Out[I] += float(Y);
		Peak = FMath::Max(Peak, double(FMath::Abs(Out[I])));
	}
	if (Peak > 0) for (int32 I = 0; I < Len; ++I) Out[I] *= float(0.85 / Peak);
	return Buf;
}

// ---------- the processor's display ----------

class SS47Dsp : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Dsp) {}
	SLATE_END_ARGS()
	void Construct(const FArguments&) { White = MakeUnique<FSlateColorBrush>(FLinearColor::White); }
	FS47DspPicture P;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(256, 72); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
	{
		// the canvas's colours, linear: drawn without Slate's own gamma into an sRGB target, the target
		// holds the canvas's bytes, and the screen's material reads them back as three does (S47Yard's bus)
		auto Css = [](uint32 C, float A = 1.f) { FLinearColor L = FLinearColor::FromSRGBColor(FColor((C >> 16) & 255, (C >> 8) & 255, C & 255)); L.A = A; return L; };
		auto Box = [&](float X, float Y, float W, float H, const FLinearColor& C) { FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(X, Y))), White.Get(), ESlateDrawEffect::None, C); };
		// textBaseline 'top': the text's top at y
		auto Txt = [&](const FString& S, float X, float Y, const FLinearColor& C) { FSlateDrawElement::MakeText(Out, Layer + 1, G.ToPaintGeometry(FVector2f(250.f, 24.f), FSlateLayoutTransform(FVector2f(X, Y))), S, S47Style::Mono(20.f), ESlateDrawEffect::None, C); };
		Box(0, 0, 256, 72, Css(0x031008));
		Txt(TEXT("REEL 047  10-AUG-79"), 8, 4, Css(0x8cffa4));
		if (P.State == TEXT("RANGE"))
		{
			Txt(TEXT("SOURCE DIST  -32 LY"), 8, 26, Css(0xffd27a));
			Txt(P.bFinished ? FString(TEXT("END OF TAPE")) : P.Counter + TEXT("  DATA TRACK"), 8, 48, Css(0x8cffa4, 0.6f));
		}
		else
		{
			Txt(P.Counter + TEXT("  ") + P.State, 8, 26, Css(0x8cffa4));
			// the level on the tape: nothing at all in the hole
			const double Lvl = P.State == TEXT("NO SIGNAL") || P.State == TEXT("THREADING") ? 0 : P.State == TEXT("PULSE 4/7") ? 0.4 + 0.5 * FMath::Abs(FMath::Sin(P.T * 9)) : P.State == TEXT("RELAY") ? 1 : 0.18 + 0.06 * FMath::Sin(P.T * 23);
			for (int32 I = 0; I < 16; ++I) Box(10 + I * 14, 52, 10, 12, I / 16.0 < Lvl ? Css(0xb9ffc7) : Css(0x6edc96, 0.18f));
		}
		return Layer + 2;
	}

private:
	TUniquePtr<FSlateBrush> White;
};

FS47DspScreen::~FS47DspScreen()
{
	if (Renderer) { BeginCleanup(Renderer); Renderer = nullptr; }
	Widget.Reset();
}

void FS47DspScreen::Init(UTextureRenderTarget2D* InTarget)
{
	Target = InTarget;
	if (!InTarget || !FApp::CanEverRender()) return;
	Renderer = new FWidgetRenderer(false);
	Widget = SNew(SS47Dsp);
}

void FS47DspScreen::Draw(const FS47DspPicture& P)
{
	if (!Renderer || !Widget.IsValid() || !Target.IsValid() || !FSlateApplication::IsInitialized()) return;
	Widget->P = P;
	Renderer->DrawWidget(Target.Get(), Widget.ToSharedRef(), FVector2D(256, 72), 0.f);
}

// ---------- Reel047 ----------

FS47Reel047::FS47Reel047(FS47Reel047Deps InD) : D(MoveTemp(InD)) {}
FS47Reel047::~FS47Reel047() = default;

void FS47Reel047::Play(TFunction<void(const FS47ReelCue&)> Cue)
{
	Stop();
	bPlaying = true; bFinished = false; T = 0; K = 0; Heard.Reset(); Spin = 0; Gate = 0;
	Cb = MoveTemp(Cue);
	if (D.ShowScreen) D.ShowScreen(true);
	Build();
	Step(0);
}

void FS47Reel047::Stop()
{
	if (!bPlaying && !Tape.IsValid()) return;
	bPlaying = false; Cb = nullptr;
	Release(true);
}

void FS47Reel047::Reset()
{
	Stop(); bFinished = false; T = 0; Heard.Reset(); Gate = 0;
	// (its display and reels the processor's own again, story/Decoder.ts)
	if (D.ShowScreen) D.ShowScreen(false);
}

void FS47Reel047::Update(double Dt)
{
	if (!bPlaying) return;
	T += Dt;
	Step(Dt);
}

void FS47Reel047::Step(double Dt)
{
	US47Audio* A = D.Audio.Get();
	const TArray<FS47ReelCue>& Plan = S47Reel047::REEL_PLAN();
	while (K < Plan.Num() && Plan[K].At <= T)
	{
		const FS47ReelCue C = Plan[K++];
		Heard.Add(C.Kind);
		if (A && A->Ctx.IsValid() && Tape.IsValid()) Sound(C);
		if (Cb) Cb(C);
		if (C.Kind == TEXT("end")) { bPlaying = false; bFinished = true; Release(false); Draw(); return; }
	}
	if (Dec.IsValid()) { Dec->Update(); if (T >= PULSE_END) { Dec->Stop(); Dec.Reset(); } }
	// the reels go round from the start to the end, through the hole too
	if (T >= 2.2)
	{
		Spin += Dt * 2.4;
		for (int32 I = 0; I < D.Reels.Num(); ++I) D.Reels[I].SetRotX(Spin * (I ? 0.8 : 1.15));
	}
	if ((ScrT -= Dt) <= 0) { ScrT = 1.0 / 8; Draw(); }
}

// ---------- the sound ----------

void FS47Reel047::Build()
{
	// the tape's chain: what is on the tape comes through the deck's head and the processor's little
	// monitor, placed at the deck; the motor and the transport are the deck's own, not on the tape
	US47Audio* A = D.Audio.Get();
	if (!A || !A->Ctx.IsValid() || A->Ctx->State() == TEXT("closed") || !A->NoiseBuffer().IsValid()) return;
	const TSharedRef<FS47AudioContext> Ctx = A->Ctx.ToSharedRef();
	const FVector At = D.Deck ? D.Deck() : FVector::ZeroVector;
	const TSharedPtr<FS47AudioNode> Out = A->Spatial(At, 1.8f);
	if (!Out.IsValid()) return;
	FS47AudioContext::FBatch B(*Ctx);
	const TSharedRef<FS47BiquadFilterNode> Mon = Ctx->CreateBiquadFilter(); Mon->SetType(TEXT("peaking")); Mon->Frequency.SetValue(1700); Mon->Q.SetValue(0.8); Mon->Gain.SetValue(4); Mon->Connect(Out);
	const TSharedRef<FS47BiquadFilterNode> Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(5200); Lp->Connect(Mon);
	const TSharedRef<FS47GainNode> Tp = Ctx->CreateGain(); Tp->Gain.SetValue(0); Tp->Connect(Lp);
	Tape = Tp; Nodes.Append({ Out, Mon, Lp, Tp });
	// the hiss on the tape, from the start
	const TSharedRef<FS47BufferSourceNode> N = Ctx->CreateBufferSource(); N->Buffer = A->NoiseBuffer(); N->bLoop = true;
	const TSharedRef<FS47BiquadFilterNode> Hp = Ctx->CreateBiquadFilter(); Hp->SetType(TEXT("highpass")); Hp->Frequency.SetValue(1800);
	const TSharedRef<FS47GainNode> Ng = Ctx->CreateGain(); Ng->Gain.SetValue(0.045);
	N->Connect(Hp); Hp->Connect(Ng); Ng->Connect(Tp); N->Start();
	Srcs.Add(N); Nodes.Append({ Hp, Ng });
}

void FS47Reel047::Sound(const FS47ReelCue& C)
{
	US47Audio* A = D.Audio.Get();
	const TSharedRef<FS47AudioContext> Ctx = A->Ctx.ToSharedRef();
	FS47AudioContext::FBatch B(*Ctx);
	const TSharedPtr<FS47GainNode> Tp = Tape;
	const double Now = Ctx->CurrentTime();
	const FVector At = D.Deck ? D.Deck() : FVector::ZeroVector;
	auto Deck = [&](const FString& Name, double Gain, double Rate, double When = 0)
	{
		FS47Sound O; O.Gain = float(Gain); O.Rate = float(Rate); O.At = At; O.When = float(When);
		const FS47Play S = A->Sfx(Name, O);
		if (S) Srcs.Add(S.Src);
	};
	auto OnTape = [&](const FString& Name, double Gain, double Rate, double When = 0, double Lp = 0)
	{
		FS47Sound O; O.Gain = float(Gain); O.Rate = float(Rate); O.When = float(When); O.DestNode = Tp; O.Lp = float(Lp);
		const FS47Play S = A->Sfx(Name, O);
		if (S) Srcs.Add(S.Src);
	};
	if (C.Kind == TEXT("thread"))
	{
		// the reel seated on its spindle, the leader through the guides
		Deck(TEXT("click"), 0.4, 0.8); Deck(TEXT("click"), 0.3, 1.1, 0.5); Deck(TEXT("paper1"), 0.18, 2.2, 0.9); Deck(TEXT("click"), 0.35, 0.9, 1.6);
	}
	else if (C.Kind == TEXT("start"))
	{
		Deck(A->Or(TEXT("small_switch"), TEXT("switch")), 0.45, 0.8);
		Gate = 1;
		Tp->Gain.SetTargetAtTime(1, Now, 0.05);
		// the motor and the capstan
		const TSharedRef<FS47OscillatorNode> O = Ctx->CreateOscillator(); O->SetType(TEXT("sawtooth")); O->Frequency.SetValue(22);
		const TSharedRef<FS47BiquadFilterNode> Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(170);
		const TSharedRef<FS47GainNode> G = Ctx->CreateGain(); G->Gain.SetValue(0); O->Connect(Lp); Lp->Connect(G);
		const TSharedPtr<FS47AudioNode> Sp = A->Spatial(At, 1.4f); if (Sp.IsValid()) G->Connect(Sp);
		O->Start(); O->Frequency.SetTargetAtTime(38, Now, 0.15); G->Gain.SetTargetAtTime(0.028, Now, 0.12);
		Motor = FMotor{ O, G }; Srcs.Add(O); Nodes.Append({ Lp, G });
		if (Sp.IsValid()) Nodes.Add(Sp);
	}
	else if (C.Kind == TEXT("pulse"))
	{
		// the pulse group as it came in, raw: tape B's own sound, through the deck's monitor
		const TSharedPtr<FS47AudioBuffer> Noise = A->NoiseBuffer();
		if (!Noise.IsValid()) return;
		// (its own panner at the deck, straight to the effects: not through the tape's)
		Dec = MakeShared<FS47DecoderPlayer>(Ctx, A->SfxBus, At + FVector(-0.3, 1.1, 0), Noise);
		FS47DecoderSet P; P.Mode = TEXT("raw"); P.Stretch = 1; P.Pitch = 0; P.Space = 0.2;
		Dec->Start(P);
	}
	else if (C.Kind == TEXT("crank"))
	{
		// a field telephone rung by its handle, a little way off: two turns of the crank bell
		const TSharedRef<FS47BufferSourceNode> S = Ctx->CreateBufferSource(); S->Buffer = S47Reel047::CrankBell(*Ctx);
		const TSharedRef<FS47BiquadFilterNode> Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(1300); Bp->Q.SetValue(0.7);
		const TSharedRef<FS47GainNode> G = Ctx->CreateGain(); G->Gain.SetValue(0.3);
		S->Connect(Bp); Bp->Connect(G); G->Connect(Tp); S->Start(Now, 0, 3.0);
		// the handle's gears under it
		const TSharedRef<FS47OscillatorNode> O = Ctx->CreateOscillator(); O->SetType(TEXT("sawtooth")); O->Frequency.SetValueAtTime(70, Now);
		O->Frequency.LinearRampToValueAtTime(110, Now + 1.2); O->Frequency.LinearRampToValueAtTime(80, Now + 2.6);
		const TSharedRef<FS47GainNode> Og = Ctx->CreateGain(); Og->Gain.SetValueAtTime(0, Now); Og->Gain.LinearRampToValueAtTime(0.03, Now + 0.2); Og->Gain.SetValueAtTime(0.03, Now + 2.4); Og->Gain.LinearRampToValueAtTime(0, Now + 2.8);
		const TSharedRef<FS47BiquadFilterNode> Olp = Ctx->CreateBiquadFilter(); Olp->SetType(TEXT("lowpass")); Olp->Frequency.SetValue(900);
		O->Connect(Olp); Olp->Connect(Og); Og->Connect(Tp); O->Start(Now); O->Stop(Now + 2.9);
		Srcs.Append({ S, O }); Nodes.Append({ Bp, G, Og, Olp });
	}
	else if (C.Kind == TEXT("cut"))
	{
		// two blades closing on copper: a click, a short bright crunch, a ring
		OnTape(TEXT("click"), 0.7, 0.55);
		const TSharedRef<FS47BufferSourceNode> N = Ctx->CreateBufferSource(); N->Buffer = A->NoiseBuffer();
		const TSharedRef<FS47BiquadFilterNode> Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(3800); Bp->Q.SetValue(1.4);
		const TSharedRef<FS47GainNode> G = Ctx->CreateGain(); G->Gain.SetValueAtTime(0, Now + 0.03); G->Gain.LinearRampToValueAtTime(0.5, Now + 0.035); G->Gain.ExponentialRampToValueAtTime(0.001, Now + 0.12);
		N->Connect(Bp); Bp->Connect(G); G->Connect(Tp); N->Start(Now + 0.03); N->Stop(Now + 0.15);
		const TSharedRef<FS47OscillatorNode> R = Ctx->CreateOscillator(); R->Frequency.SetValue(2950);
		const TSharedRef<FS47GainNode> Rg = Ctx->CreateGain(); Rg->Gain.SetValueAtTime(0, Now + 0.04); Rg->Gain.LinearRampToValueAtTime(0.06, Now + 0.045); Rg->Gain.ExponentialRampToValueAtTime(0.0005, Now + 0.5);
		R->Connect(Rg); Rg->Connect(Tp); R->Start(Now + 0.04); R->Stop(Now + 0.55);
		OnTape(TEXT("thudSoft"), 0.3, 2.1, 0.05);
		Srcs.Append({ N, R }); Nodes.Append({ Bp, G, Rg });
	}
	else if (C.Kind == TEXT("hole"))
	{
		// nothing on the tape at all, for 47 seconds: the hiss too (the deck runs on); the night
		// outside goes with it, and comes back after
		Gate = 0;
		Tp->Gain.CancelScheduledValues(Now); Tp->Gain.SetValueAtTime(Tp->Gain.Value(), Now); Tp->Gain.SetTargetAtTime(0, Now, 0.006);
		if (D.Hush) D.Hush(S47Reel047::HOLE_LEN, 0.04, 0.4, 0.2);
	}
	else if (C.Kind == TEXT("relay"))
	{
		Gate = 1;
		Tp->Gain.CancelScheduledValues(Now); Tp->Gain.SetValueAtTime(0, Now); Tp->Gain.LinearRampToValueAtTime(1, Now + 0.004);
		OnTape(A->Or(TEXT("rack_switch"), TEXT("switch")), 1.0, 0.62);
		OnTape(TEXT("thudSoft"), 0.5, 1.4, 0.03);
		if (D.Hush) D.Hush(4, 0.3, 0.05, 2.2);
	}
	else if (C.Kind == TEXT("leader"))
	{
		// the end of the tape: the leader through the guides, then flapping on the take-up reel
		Gate = 0;
		Tp->Gain.SetTargetAtTime(0, Now, 0.05);
		for (int32 I = 0; I < 6; ++I) Deck(TEXT("click"), 0.22 - I * 0.025, 1.6, I * (0.16 + I * 0.05));
	}
	else if (C.Kind == TEXT("end"))
	{
		Deck(A->Or(TEXT("small_switch"), TEXT("switch")), 0.4, 0.72);
	}
	// ('back' and 'range': nothing more on the tape)
}

void FS47Reel047::Release(bool bNow)
{
	US47Audio* A = D.Audio.Get();
	const TSharedPtr<FS47AudioContext> Ctx = A ? A->Ctx : nullptr;
	if (Dec.IsValid()) { Dec->Stop(); Dec.Reset(); }
	if (Ctx.IsValid())
	{
		FS47AudioContext::FBatch B(*Ctx);
		if (Motor) { Motor->G->Gain.SetTargetAtTime(0, Ctx->CurrentTime(), bNow ? 0.05 : 0.3); Motor->O->Frequency.SetTargetAtTime(12, Ctx->CurrentTime(), 0.3); }
		if (Tape.IsValid()) Tape->Gain.SetTargetAtTime(0, Ctx->CurrentTime(), 0.03);
	}
	Motor.Reset();
	Tape.Reset();
	const TArray<TSharedPtr<FS47ScheduledSourceNode>> S = MoveTemp(Srcs);
	const TArray<TSharedPtr<FS47AudioNode>> N = MoveTemp(Nodes);
	Srcs.Reset(); Nodes.Reset();
	if (Ctx.IsValid())
	{
		Ctx->SetTimeout(bNow ? 0.12 : 1.8, [S, N]
		{
			for (const TSharedPtr<FS47ScheduledSourceNode>& X : S) { X->Stop(); X->Disconnect(); }
			for (const TSharedPtr<FS47AudioNode>& X : N) X->Disconnect();
		});
	}
}

// ---------- the processor's display ----------

FString FS47Reel047::State() const
{
	if (!bPlaying && !bFinished) return FString();
	const double RangeAt = S47Reel047::REEL_PLAN().FindByPredicate([](const FS47ReelCue& C) { return C.Kind == TEXT("range"); })->At;
	if (bFinished || T >= RangeAt) return TEXT("RANGE");
	if (T >= S47Reel047::HOLE_FROM + S47Reel047::HOLE_LEN) return TEXT("RELAY");
	if (T >= S47Reel047::HOLE_FROM) return TEXT("NO SIGNAL");
	if (T >= 3.4 && T < PULSE_END) return TEXT("PULSE 4/7");
	return T < 2.2 ? TEXT("THREADING") : TEXT("PLAY");
}

void FS47Reel047::Draw()
{
	if (!D.Screen) return;
	const int32 Secs = FMath::Max(0, FMath::FloorToInt(T - 2.2));
	FS47DspPicture P;
	P.State = State();
	P.Counter = FString::Printf(TEXT("%02d:%02d"), Secs / 60, Secs % 60);
	P.bFinished = bFinished;
	P.T = T;
	D.Screen->Draw(P);
}

bool FS47Reel047::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s reel 047: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	FS47Reel047 R(FS47Reel047Deps{});
	TArray<FString> Cues;
	R.Play([&Cues](const FS47ReelCue& C) { Cues.Add(C.Kind); });
	Check(R.bPlaying && Cues.Num() == 1 && Cues[0] == TEXT("thread") && R.State() == TEXT("THREADING"), TEXT("threaded first"));
	R.Update(4);
	Check(R.State() == TEXT("PULSE 4/7"), TEXT("the pulse group from 3.4 s"));
	R.Update(16);
	Check(R.State() == TEXT("NO SIGNAL") && Cues.Contains(TEXT("crank")) && Cues.Contains(TEXT("cut")) && Cues.Last() == TEXT("hole"), TEXT("the crank, the cut, and nothing"));
	R.Update(45);
	Check(R.State() == TEXT("NO SIGNAL"), TEXT("the hole is 47 s"));
	R.Update(1);
	Check(R.State() == TEXT("RELAY") && Cues.Last() == TEXT("relay"), TEXT("then the relay"));
	R.Update(10);
	Check(!R.bPlaying && R.bFinished && R.State() == TEXT("RANGE") && Cues.Num() == 11, TEXT("the range off the end of the tape, eleven things on it"));
	// the crank bell: 7.2 s at 16 kHz, peak at 0.85
	const TSharedRef<FS47AudioContext> Ctx = FS47AudioContext::Make(48000.f);
	const TSharedRef<FS47AudioBuffer> Bell = S47Reel047::CrankBell(*Ctx);
	double Peak = 0;
	for (int32 I = 0; I < Bell->Length(); ++I) Peak = FMath::Max(Peak, double(FMath::Abs(Bell->GetChannelData(0)[I])));
	Check(Bell->Length() == 115200 && Bell->SampleRate() == 16000 && FMath::IsNearlyEqual(Peak, 0.85, 1e-4), TEXT("the crank bell (story/officePhone.ts)"));
	return bOk;
}
