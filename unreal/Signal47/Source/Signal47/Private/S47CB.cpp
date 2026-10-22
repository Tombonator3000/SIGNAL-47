// core/cb.ts (and from core/radio.ts the syllables the voices mumble in)
#include "S47CB.h"

#include "S47Audio.h"

namespace
{
	constexpr double LEAD = 0.45, GAP = 0.55;
	/** The speaker's clipping: narrow and overdriven. SHAPE */
	const TArray<float>& SHAPE()
	{
		static const TArray<float> C = []
		{
			TArray<float> A;
			A.SetNum(256);
			for (int32 I = 0; I < 256; ++I) { const double X = I / 127.5 - 1; A[I] = float(FMath::Tanh(X * 2.4) / FMath::Tanh(2.4)); }
			return A;
		}();
		return C;
	}
	/** The engine on the channel's level (a few dB over the cab at 55 mph: tools/f_last_mile.py measures
	 *  it), how much slower it runs than the truck's own idle, the recording's make-up, and the hiss under it. */
	constexpr double CB_LEVEL = 0.6, CB_RATE = 0.9, CB_REC = 3.2, HISS = 0.035;

	// ---------- core/radio.ts ----------
	/** A number from text, the same every time (a line mumbles the same way each time it is said). seed */
	uint32 Seed(const FString& Text)
	{
		uint32 H = 2166136261u;
		for (const TCHAR C : Text) H = (H ^ uint32(C)) * 16777619u; // Math.imul(h ^ text.charCodeAt(i), 16777619)
		return H;
	}
	/** rng(n): mulberry32, in 32-bit arithmetic as JavaScript's Math.imul and >>> do it */
	struct FRng
	{
		uint32 N;
		double operator()()
		{
			N = N + 0x6d2b79f5u;
			uint32 T = (N ^ (N >> 15)) * (1u | N);
			T ^= T + (T ^ (T >> 7)) * (61u | T);
			return double(T ^ (T >> 14)) / 4294967296.0;
		}
	};
	bool IsSpace(TCHAR C) { return FChar::IsWhitespace(C); }
	// text.split(/\s+/)
	TArray<FString> SplitWs(const FString& Text)
	{
		TArray<FString> Out;
		FString Cur;
		bool bInWs = false;
		for (const TCHAR C : Text)
		{
			if (IsSpace(C)) { if (!bInWs) { Out.Add(Cur); Cur.Reset(); bInWs = true; } }
			else { Cur.AppendChar(C); bInWs = false; }
		}
		Out.Add(Cur);
		return Out;
	}
	// /[,.;:!?]$/
	bool EndsPause(const FString& W) { return W.Len() > 0 && FString(TEXT(",.;:!?")).Contains(FString::Chr(W[W.Len() - 1])); }
	// Math.round for the non-negative numbers it is given here
	int32 JsRound(double X) { return int32(FMath::FloorToDouble(X + 0.5)); }
}

double S47CB::Pitch(const FString& Voice)
{
	// PITCH: Record<Voice, number>
	if (Voice == TEXT("host")) return 116;
	if (Voice == TEXT("woman")) return 212;
	if (Voice == TEXT("man")) return 138;
	if (Voice == TEXT("roswell")) return 126;
	if (Voice == TEXT("boy")) return 262;
	if (Voice == TEXT("news")) return 150;
	if (Voice == TEXT("old")) return 104;
	return 116;
}

TArray<S47CB::FSyllable> S47CB::Syllables(const FString& Text, double Secs)
{
	FRng R{ Seed(Text) };
	TArray<FString> Words;
	for (const FString& W : SplitWs(Text)) if (!W.IsEmpty()) Words.Add(W); // .filter(Boolean)
	TArray<int32> Per;
	int32 Pauses = 0, Total = 0;
	for (const FString& W : Words)
	{
		int32 Letters = 0;
		for (const TCHAR C : W) if ((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z')) ++Letters; // w.replace(/[^a-z]/gi, '').length
		Per.Add(FMath::Clamp(JsRound(Letters / 3.0), 1, 4));
		Total += Per.Last();
		if (EndsPause(W)) ++Pauses;
	}
	const double D = FMath::Max(0.12, (Secs - Pauses * 0.16) / FMath::Max(1, Total));
	TArray<FSyllable> Out;
	double At = 0, F = 1;
	for (int32 I = 0; I < Words.Num(); ++I)
	{
		const FString& W = Words[I];
		for (int32 K = 0; K < Per[I]; ++K)
		{
			F = FMath::Clamp(F + (R() - 0.5) * 0.22, 0.86, 1.22);
			const bool bEnd = I == Words.Num() - 1 && K == Per[I] - 1;
			const double Pitch = bEnd ? (W.EndsWith(TEXT("?")) ? 1.25 : 0.8) : F * (K == 0 ? 1.07 : 1);
			FSyllable S;
			S.At = At; S.D = D * (0.7 + R() * 0.25); S.F = Pitch;
			Out.Add(S);
			At += D;
		}
		if (EndsPause(W)) At += 0.16;
	}
	return Out;
}

double S47CB::SecsOf(const FString& Text)
{
	return FMath::Clamp(1.1 + SplitWs(Text).Num() * 0.32, 2.4, 7.5);
}

const S47CB::FEngineTimes& S47CB::ENGINE()
{
	static const FEngineTimes E;
	return E;
}

TFunction<void()> S47CB::EngineOnChannel(const TSharedRef<FS47AudioContext>& Ctx, const TSharedPtr<FS47AudioNode>& Dest, const TSharedPtr<FS47AudioBuffer>& Noise, const TSharedPtr<FS47AudioBuffer>& Idle, double T0)
{
	const FEngineTimes& E = ENGINE();
	FS47AudioContext::FBatch B(*Ctx);
	TArray<TSharedPtr<FS47ScheduledSourceNode>> Srcs;
	// the speaker: as for a call, with a little more of the low middle left in for an engine
	auto Out = Ctx->CreateGain(); Out->Gain.SetValue(0.85);
	auto Hp = Ctx->CreateBiquadFilter(); Hp->SetType(TEXT("highpass")); Hp->Frequency.SetValue(300); Hp->Q.SetValue(0.7);
	auto Pk = Ctx->CreateBiquadFilter(); Pk->SetType(TEXT("peaking")); Pk->Frequency.SetValue(1500); Pk->Q.SetValue(1.1); Pk->Gain.SetValue(5);
	auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(2900);
	auto Sh = Ctx->CreateWaveShaper(); Sh->Curve = SHAPE();
	Out->Connect(Hp); Hp->Connect(Pk); Pk->Connect(Lp); Lp->Connect(Sh); Sh->Connect(Dest);
	// the squelch: a burst as it opens, the hiss low under the engine and gone with it (an open carrier
	// with nothing on it), and the tail as the carrier drops
	auto N = Ctx->CreateBufferSource(); N->Buffer = Noise; N->bLoop = true; Srcs.Add(N);
	auto Nh = Ctx->CreateBiquadFilter(); Nh->SetType(TEXT("highpass")); Nh->Frequency.SetValue(700);
	auto Sq = Ctx->CreateGain(); Sq->Gain.SetValue(0);
	N->Connect(Nh); Nh->Connect(Sq); Sq->Connect(Out);
	FS47AudioParam& S = Sq->Gain;
	S.SetValueAtTime(0, T0); S.LinearRampToValueAtTime(0.55, T0 + 0.02); S.SetTargetAtTime(HISS, T0 + 0.16, 0.06);
	S.SetValueAtTime(HISS, T0 + E.Die + 0.3); S.LinearRampToValueAtTime(0, T0 + E.Dead);
	S.SetValueAtTime(0, T0 + E.Tail); S.LinearRampToValueAtTime(0.5, T0 + E.Tail + 0.03); S.SetTargetAtTime(0, T0 + E.Tail + 0.2, 0.03);
	N->Start(T0); N->Stop(T0 + E.End + 0.2);
	// the engine
	auto Eng = Ctx->CreateGain(); Eng->Gain.SetValue(0); Eng->Connect(Out);
	FS47AudioParam& G = Eng->Gain;
	const double L = CB_LEVEL;
	G.SetValueAtTime(0, T0 + E.In); G.LinearRampToValueAtTime(L, T0 + E.Up);
	for (const double C : E.Cough)
	{
		G.SetValueAtTime(L, T0 + C); G.LinearRampToValueAtTime(L * 0.08, T0 + C + 0.08); G.LinearRampToValueAtTime(L * 1.1, T0 + C + 0.24); G.LinearRampToValueAtTime(L, T0 + C + 0.4);
	}
	// it dies as the truck's does (drive/engineSound.ts, stall): chugs, and a wind-down
	G.SetValueAtTime(L, T0 + E.Die);
	const double Chugs[6][2] = { { 0.08, 1.25 }, { 0.3, 0.45 }, { 0.42, 0.85 }, { 0.72, 0.25 }, { 0.84, 0.45 }, { 1.3, 0 } };
	for (const auto& Ch : Chugs) G.LinearRampToValueAtTime(L * Ch[1], T0 + E.Die + Ch[0]);
	const double StopAt = T0 + E.Dead + 0.1;
	if (Idle.IsValid())
	{
		auto Src = Ctx->CreateBufferSource(); Src->Buffer = Idle; Src->bLoop = true; Srcs.Add(Src);
		auto Rg = Ctx->CreateGain(); Rg->Gain.SetValue(CB_REC); Src->Connect(Rg); Rg->Connect(Eng);
		FS47AudioParam& R = Src->PlaybackRate;
		R.SetValueAtTime(CB_RATE, T0);
		for (const double C : E.Cough) { R.SetValueAtTime(CB_RATE, T0 + C); R.LinearRampToValueAtTime(CB_RATE * 0.72, T0 + C + 0.12); R.LinearRampToValueAtTime(CB_RATE, T0 + C + 0.38); }
		R.SetValueAtTime(CB_RATE, T0 + E.Die); R.ExponentialRampToValueAtTime(0.22, T0 + E.Die + 1.25);
		Src->Start(T0 + E.In, FMath::Fmod(T0 * 3.7, FMath::Max(0.1, Idle->Duration() - 0.1))); Src->Stop(StopAt);
	}
	else
	{
		// (the field radio's engine: a low saw beating at the firing rate, through a muffled exhaust; the beat
		// is round here, so on the channel's band it is a lope and not a train of clicks)
		auto Osc = Ctx->CreateOscillator(); Osc->SetType(TEXT("sawtooth"));
		auto Elp = Ctx->CreateBiquadFilter(); Elp->SetType(TEXT("lowpass")); Elp->Frequency.SetValue(520);
		auto Elp2 = Ctx->CreateBiquadFilter(); Elp2->SetType(TEXT("lowpass")); Elp2->Frequency.SetValue(520); Elp->Connect(Elp2);
		auto Fire = Ctx->CreateOscillator(); Fire->SetType(TEXT("sine")); Fire->Frequency.SetValue(17);
		auto Fg = Ctx->CreateGain(); Fg->Gain.SetValue(0.45); Fire->Connect(Fg);
		auto Chop = Ctx->CreateGain(); Chop->Gain.SetValue(0.55); Fg->Connect(Chop->Gain);
		auto Lift = Ctx->CreateGain(); Lift->Gain.SetValue(2.2);
		Osc->Connect(Elp); Elp2->Connect(Lift); Lift->Connect(Chop); Chop->Connect(Eng);
		FS47AudioParam& F = Osc->Frequency;
		FS47AudioParam& Ff = Fire->Frequency;
		F.SetValueAtTime(31, T0); Ff.SetValueAtTime(15.5, T0);
		for (const double C : E.Cough)
		{
			F.SetValueAtTime(31, T0 + C); F.LinearRampToValueAtTime(22, T0 + C + 0.12); F.LinearRampToValueAtTime(31, T0 + C + 0.38);
			Ff.SetValueAtTime(15.5, T0 + C); Ff.LinearRampToValueAtTime(10, T0 + C + 0.12); Ff.LinearRampToValueAtTime(15.5, T0 + C + 0.38);
		}
		F.SetValueAtTime(31, T0 + E.Die); F.ExponentialRampToValueAtTime(8, T0 + E.Die + 1.25);
		Ff.SetValueAtTime(15.5, T0 + E.Die); Ff.ExponentialRampToValueAtTime(4, T0 + E.Die + 1.25);
		for (const TSharedRef<FS47OscillatorNode>& O : { Osc, Fire }) { O->Start(T0 + E.In); O->Stop(StopAt); Srcs.Add(O); }
	}
	// a dull pop in the exhaust with each cough
	for (const double C : E.Cough)
	{
		auto P = Ctx->CreateBufferSource(); P->Buffer = Noise; Srcs.Add(P);
		auto Pb = Ctx->CreateBiquadFilter(); Pb->SetType(TEXT("bandpass")); Pb->Frequency.SetValue(420); Pb->Q.SetValue(1.4);
		auto Pg = Ctx->CreateGain(); Pg->Gain.SetValueAtTime(0, T0 + C + 0.02); Pg->Gain.LinearRampToValueAtTime(0.5, T0 + C + 0.03); Pg->Gain.ExponentialRampToValueAtTime(0.001, T0 + C + 0.18);
		P->Connect(Pb); Pb->Connect(Pg); Pg->Connect(Eng);
		P->Start(T0 + C + 0.02, FMath::Fmod(C * 0.37, FMath::Max(0.1, (Noise.IsValid() ? Noise->Duration() : 1.0) - 0.3))); P->Stop(T0 + C + 0.22);
	}
	TSharedRef<FS47AudioContext> C = Ctx;
	return [C, Srcs, Out]
	{
		FS47AudioContext::FBatch Bt(*C);
		for (const TSharedPtr<FS47ScheduledSourceNode>& X : Srcs) X->Stop(); // (stopped: nothing)
		Out->Disconnect();
	};
}

// ---------- CB ----------

FS47CB::FS47CB(US47Audio* InAudio) : Audio(InAudio) {}

double FS47CB::Call(const TArray<FS47CBLine>& Lines, const TOptional<FVector>& At, TFunction<void(const FS47CBLine&, double)> OnLine, TFunction<void()> OnEnd)
{
	US47Audio* A = Audio.Get();
	if (!A || !A->Ctx.IsValid()) return 0;
	const TSharedRef<FS47AudioContext> Ctx = A->Ctx.ToSharedRef();
	const TSharedPtr<FS47AudioBuffer> Noise = A->NoiseBuffer();
	if (!Noise.IsValid() || bBusy || !Lines.Num()) return 0;
	A->StopPhoneCalls();
	if (A->OnPhoneCall) A->OnPhoneCall();
	bBusy = true;
	FS47AudioContext::FBatch B(*Ctx);
	TArray<TSharedPtr<FS47AudioBuffer>> Selected;
	bool bRecorded = true;
	for (const FS47CBLine& L : Lines)
	{
		Selected.Add(L.By == TEXT("N. VEGA") && L.Who == TEXT("woman") ? A->CharacterBuffer(TEXT("bacall"), L.Text) : nullptr);
		const TSharedPtr<FS47AudioBuffer>& Buf = Selected.Last();
		if (!(Buf.IsValid() && FMath::IsFinite(Buf->Duration()) && Buf->Duration() > 0)) bRecorded = false;
	}
	TArray<TSharedPtr<FS47BufferSourceNode>> Sources;
	const double T0 = Ctx->CurrentTime() + 0.05;
	// the speaker: narrow, overdriven, from the dash
	auto Out0 = Ctx->CreateGain(); Out0->Gain.SetValue(0.85);
	auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(1500); Bp->Q.SetValue(1.1);
	auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(2900);
	auto Sh = Ctx->CreateWaveShaper(); Sh->Curve = SHAPE();
	const TSharedPtr<FS47AudioNode> RadioBus = A->RadioBus;
	const TSharedPtr<FS47AudioNode> Pan = At ? A->Spatial(FVector(At->X, At->Y + 1.1, At->Z), 1.0f, RadioBus) : RadioBus;
	if (!Pan.IsValid()) { bBusy = false; return 0; }
	Out0->Connect(Bp); Bp->Connect(Lp); Lp->Connect(Sh); Sh->Connect(Pan);
	// the voice
	auto Horn = Ctx->CreateOscillator(); Horn->SetType(TEXT("sawtooth"));
	auto Vib = Ctx->CreateOscillator(); Vib->Frequency.SetValue(5.8);
	auto Vd = Ctx->CreateGain(); Vd->Gain.SetValue(2.6); Vib->Connect(Vd); Vd->Connect(Horn->Frequency);
	auto Mute = Ctx->CreateBiquadFilter(); Mute->SetType(TEXT("lowpass")); Mute->Frequency.SetValue(400); Mute->Q.SetValue(6.5);
	auto Voice = Ctx->CreateGain(); Voice->Gain.SetValue(0);
	Horn->Connect(Mute); Mute->Connect(Voice); Voice->Connect(Out0);
	// the squelch: hiss that bursts open, sits low under the voice, and bursts again at the end
	auto N = Ctx->CreateBufferSource(); N->Buffer = Noise; N->bLoop = true;
	auto Hp = Ctx->CreateBiquadFilter(); Hp->SetType(TEXT("highpass")); Hp->Frequency.SetValue(700);
	auto Sq = Ctx->CreateGain(); Sq->Gain.SetValue(0);
	N->Connect(Hp); Hp->Connect(Sq); Sq->Connect(Out0);
	// the lines, one after the other
	FS47AudioParam& Fq = Horn->Frequency;
	FS47AudioParam& Mf = Mute->Frequency;
	FS47AudioParam& G = Voice->Gain;
	double At0 = LEAD;
	TArray<double> Starts, Durations;
	for (int32 I = 0; I < Lines.Num(); ++I)
	{
		const FS47CBLine& L = Lines[I];
		const double Secs = bRecorded ? Selected[I]->Duration() : S47CB::SecsOf(L.Text), Base = S47CB::Pitch(L.Who);
		Durations.Add(Secs);
		Starts.Add(At0);
		if (bRecorded)
		{
			auto Source = Ctx->CreateBufferSource(); Source->Buffer = Selected[I]; Source->Connect(Out0);
			Source->Start(T0 + At0); Sources.Add(Source); At0 += Secs + GAP; continue;
		}
		Fq.SetValueAtTime(Base, T0 + At0);
		for (const S47CB::FSyllable& Y : S47CB::Syllables(L.Text, Secs))
		{
			const double Ts = T0 + At0 + Y.At, F = Base * Y.F;
			Fq.SetValueAtTime(F, Ts); Fq.LinearRampToValueAtTime(F * 0.94, Ts + Y.D);
			Mf.SetValueAtTime(330, Ts); Mf.LinearRampToValueAtTime(1300, Ts + Y.D * 0.3); Mf.LinearRampToValueAtTime(420, Ts + Y.D);
			G.SetValueAtTime(0, Ts); G.LinearRampToValueAtTime(0.5, Ts + 0.025); G.SetValueAtTime(0.5, Ts + Y.D * 0.7); G.LinearRampToValueAtTime(0, Ts + Y.D);
		}
		At0 += Secs + GAP;
	}
	const double End = At0;
	FS47AudioParam& S = Sq->Gain;
	S.SetValueAtTime(0, T0); S.LinearRampToValueAtTime(0.55, T0 + 0.02); S.SetTargetAtTime(0.035, T0 + 0.16, 0.06);
	S.SetValueAtTime(0.035, T0 + End); S.LinearRampToValueAtTime(0.5, T0 + End + 0.03); S.SetTargetAtTime(0, T0 + End + 0.2, 0.03);
	Horn->Start(T0); Vib->Start(T0); N->Start(T0);
	// Silent source-end markers use the same pausable clock as the speaker.
	// No wall timer can caption the next sentence or finish the call during Pause.
	TSharedRef<TArray<TSharedPtr<FS47BufferSourceNode>>> All = MakeShared<TArray<TSharedPtr<FS47BufferSourceNode>>>(Sources);
	auto Marker = [&Ctx, Out0, All, T0](double Seconds, TFunction<void()> Callback)
	{
		auto Source = Ctx->CreateBufferSource(); Source->Buffer = Ctx->CreateBuffer(1, 1, Ctx->SampleRate);
		Source->Connect(Out0);
		TWeakPtr<FS47BufferSourceNode> W = Source;
		Source->OnEnded = [W, Callback] { if (const TSharedPtr<FS47BufferSourceNode> P = W.Pin()) P->Disconnect(); Callback(); };
		All->Add(Source); Source->Start(T0 + Seconds);
	};
	const TWeakPtr<FS47CB> Self = AsShared();
	const uint32 Id = ++Calls;
	const TSharedPtr<FS47OscillatorNode> HornP = Horn, VibP = Vib;
	const TSharedPtr<FS47BufferSourceNode> NP = N;
	const bool bOwnPan = Pan != RadioBus;
	TSharedRef<FS47AudioContext> C = Ctx;
	TFunction<void()> StopFn = [Self, Id, C, All, HornP, VibP, NP, Out0, Pan, bOwnPan]
	{
		FS47AudioContext::FBatch Bt(*C);
		for (const TSharedPtr<FS47BufferSourceNode>& Source : *All) { Source->OnEnded = nullptr; Source->Stop(); Source->Disconnect(); }
		for (const TSharedPtr<FS47ScheduledSourceNode>& X : TArray<TSharedPtr<FS47ScheduledSourceNode>>{ HornP, VibP, NP }) X->Stop();
		Out0->Disconnect();
		if (bOwnPan) Pan->Disconnect();
		if (const TSharedPtr<FS47CB> P = Self.Pin(); P.IsValid() && P->StopId == Id) { P->bBusy = false; P->StopAll = nullptr; }
	};
	for (int32 I = 0; I < Lines.Num(); ++I)
	{
		const FS47CBLine L = Lines[I];
		const double D = Durations[I];
		Marker(Starts[I], [OnLine, L, D] { if (OnLine) OnLine(L, D); });
	}
	StopAll = StopFn; StopId = Id;
	Marker(End + 0.8, [StopFn, OnEnd] { StopFn(); if (OnEnd) OnEnd(); });
	return End + 0.5;
}

double FS47CB::Engine(const TSharedPtr<FS47AudioBuffer>& Idle, const TSharedPtr<FS47AudioNode>& Dest)
{
	US47Audio* A = Audio.Get();
	if (!A || !A->Ctx.IsValid()) return 0;
	const TSharedRef<FS47AudioContext> Ctx = A->Ctx.ToSharedRef();
	const TSharedPtr<FS47AudioBuffer> Noise = A->NoiseBuffer();
	if (!Noise.IsValid() || bBusy) return 0;
	bBusy = true;
	FS47AudioContext::FBatch B(*Ctx);
	auto Pan = Ctx->CreateStereoPanner(); Pan->Pan.SetValue(0.35); Pan->Connect(Dest);
	const double T0 = Ctx->CurrentTime() + 0.05;
	const TFunction<void()> H = S47CB::EngineOnChannel(Ctx, Pan, Noise, Idle, T0);
	// (and gone by itself at the end, on the same clock, if nothing has stopped it first)
	auto EndSrc = Ctx->CreateBufferSource(); EndSrc->Buffer = Ctx->CreateBuffer(1, 1, Ctx->SampleRate); EndSrc->Connect(Pan);
	const TWeakPtr<FS47CB> Self = AsShared();
	const uint32 Id = ++Calls;
	const TSharedPtr<FS47BufferSourceNode> EndP = EndSrc;
	const TSharedPtr<FS47StereoPannerNode> PanP = Pan;
	TSharedRef<FS47AudioContext> C = Ctx;
	TFunction<void()> StopFn = [Self, Id, C, EndP, H, PanP]
	{
		FS47AudioContext::FBatch Bt(*C);
		EndP->OnEnded = nullptr; EndP->Stop();
		H();
		PanP->Disconnect();
		if (const TSharedPtr<FS47CB> P = Self.Pin(); P.IsValid() && P->StopId == Id) { P->bBusy = false; P->StopAll = nullptr; P->Out.Reset(); }
	};
	EndSrc->OnEnded = [Self, Id, StopFn] { if (const TSharedPtr<FS47CB> P = Self.Pin(); P.IsValid() && P->StopId == Id && P->StopAll) StopFn(); };
	EndSrc->Start(T0 + S47CB::ENGINE().End + 0.4);
	StopAll = StopFn; StopId = Id; Out = Pan;
	return S47CB::ENGINE().End;
}

// ---------- self test ----------

bool FS47CB::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s cb: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// core/radio.ts: the numbers the browser gives for these lines (node, the same functions)
	Check(FMath::IsNearlyEqual(S47CB::SecsOf(TEXT("SARO seven, this is Sierra. You hear me?")), 3.66) && S47CB::SecsOf(TEXT("Hi")) == 2.4 && S47CB::SecsOf(FString::ChrN(40, 'a').Replace(TEXT("a"), TEXT("a "))) == 7.5, TEXT("secsOf"));
	const TArray<S47CB::FSyllable> Y = S47CB::Syllables(TEXT("SARO seven, this is Sierra. You hear me?"), 3.66);
	// words: SARO(1) seven,(2) this(1) is(1) Sierra.(2) You(1) hear(1) me?(1) = 10 syllables, three pauses
	Check(Y.Num() == 10 && FMath::IsNearlyEqual(Y[1].At, (3.66 - 3 * 0.16) / 10) && Y.Last().F == 1.25 && FMath::IsNearlyEqual(Y[3].At - Y[2].At, 0.16 + (3.66 - 0.48) / 10), TEXT("syllables: count, pauses and the rising question"));
	FRng R{ Seed(TEXT("abc")) };
	const double R0 = R();
	Check(Seed(TEXT("abc")) == 440920331u && FMath::IsNearlyEqual(R0, 0.5166419988963753, 1e-12), FString::Printf(TEXT("the line's own random numbers (seed %u, first %.17g)"), Seed(TEXT("abc")), R0));
	Check(S47CB::Pitch(TEXT("woman")) == 212 && S47CB::Pitch(TEXT("old")) == 104, TEXT("PITCH"));
	// the engine on the channel, rendered offline: it is there, then the channel is quiet before its tail
	TSharedRef<FS47AudioContext> Ctx = FS47AudioContext::Make(48000.f);
	TSharedRef<FS47AudioBuffer> Noise = Ctx->CreateBuffer(1, 48000 * 2, 48000.f);
	FRandomStream Rs(47);
	for (int32 I = 0; I < Noise->Length(); ++I) Noise->GetChannelData(0)[I] = Rs.FRand() * 2 - 1;
	const TFunction<void()> Halt = S47CB::EngineOnChannel(Ctx, Ctx->Destination, Noise, nullptr, 0.0);
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
	Rms(0, 2.0);
	const double Up = Rms(2.0, 5.0);
	Rms(5.0, 9.6);
	const double Dead = Rms(9.6, 12.2), Tail = Rms(12.2, 12.6);
	Check(Up > Dead * 5 && Tail > Dead * 2, FString::Printf(TEXT("the engine on the channel %.4f, dead %.5f, the tail %.4f"), Up, Dead, Tail));
	Halt();
	Ctx->Close();
	return bOk;
}
