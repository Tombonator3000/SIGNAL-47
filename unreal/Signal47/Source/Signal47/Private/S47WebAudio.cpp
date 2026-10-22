// The small Web Audio of S47WebAudio.h. Formulas and orders from the specification
// (https://webaudio.github.io/web-audio-api/); where the browser (Chromium) does something the
// specification leaves open, it says so.
#include "S47WebAudio.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47WebAudio, Log, All);

namespace
{
	constexpr double Pi = 3.14159265358979323846;
	double Exp2(double X) { return FMath::Pow(2.0, X); }
	double LinToDb(double X) { return X > 0.0 ? 20.0 * FMath::LogX(10.0, X) : -1000.0; }
	double DbToLin(double X) { return FMath::Pow(10.0, X / 20.0); }
	float Flush(double X) { return FMath::Abs(X) < 1e-25 ? 0.f : float(X); }
}

void FS47AudioBlock::Zero(int32 InChannels)
{
	Channels = InChannels;
	FMemory::Memzero(D, sizeof(D));
}

// ---------- AudioBuffer ----------

FS47AudioBuffer::FS47AudioBuffer(int32 InChannels, int32 InLength, float InSampleRate)
	: Frames(FMath::Max(0, InLength)), Rate(InSampleRate)
{
	Data.SetNum(FMath::Clamp(InChannels, 1, 32));
	for (TArray<float>& C : Data) C.SetNumZeroed(Frames);
}

// ---------- AudioParam ----------

FS47AudioParam::FS47AudioParam(FS47AudioNode& InOwner, double InDefault, double InMin, double InMax, bool bInARate)
	: DefaultValue(InDefault), MinValue(InMin), MaxValue(InMax), Owner(InOwner), bARate(bInARate), Intrinsic(InDefault)
{
	FMemory::Memzero(Values, sizeof(Values));
}

double FS47AudioParam::Value() const
{
	// The browser's getter gives the value the render thread computed last, so a script can
	// cancelScheduledValues(t) and then setValueAtTime(param.value, t) to hold where it is.
	FScopeLock L(&Owner.Ctx->Lock);
	const int64 Q = Owner.Ctx->Quantum;
	if (OverrideQ == Q) return OverrideV;
	if (Events.Num() == 0) return Clamp(Intrinsic);
	if (RenderedQ == Q) return LastIntrinsic;
	return Clamp(ValueAt(Owner.Ctx->CurrentTime())); // (not rendered lately: what the timeline says now)
}

void FS47AudioParam::SetValue(double V)
{
	FScopeLock L(&Owner.Ctx->Lock);
	if (!FMath::IsFinite(V)) return;
	if (Events.Num() == 0) Intrinsic = V;
	else SetValueAtTime(V, Owner.Ctx->CurrentTime());
	OverrideV = Clamp(V); OverrideQ = Owner.Ctx->Quantum;
}

void FS47AudioParam::Insert(FEvent E)
{
	FScopeLock L(&Owner.Ctx->Lock);
	if (!FMath::IsFinite(E.Time) || !FMath::IsFinite(E.V)) return; // the browser throws
	E.Time = FMath::Max(0.0, E.Time);
	int32 I = Events.Num();
	while (I > 0 && Events[I - 1].Time > E.Time) --I; // after the events at the same time
	if (I == 0)
	{
		// a ramp with nothing before it runs from the time it was called and the value then
		E.InsertT = Owner.Ctx->CurrentTime();
		E.InsertV = Intrinsic;
	}
	Events.Insert(MoveTemp(E), I);
	for (int32 J = I; J < Events.Num(); ++J) Events[J].bStart = false;
}

FS47AudioParam& FS47AudioParam::SetValueAtTime(double V, double T)
{
	FEvent E; E.Type = EEvent::SetValue; E.Time = T; E.V = V;
	Insert(MoveTemp(E));
	return *this;
}

FS47AudioParam& FS47AudioParam::LinearRampToValueAtTime(double V, double T)
{
	FEvent E; E.Type = EEvent::Linear; E.Time = T; E.V = V;
	Insert(MoveTemp(E));
	return *this;
}

FS47AudioParam& FS47AudioParam::ExponentialRampToValueAtTime(double V, double T)
{
	if (V == 0.0) return *this; // RangeError in the browser
	FEvent E; E.Type = EEvent::Exponential; E.Time = T; E.V = V;
	Insert(MoveTemp(E));
	return *this;
}

FS47AudioParam& FS47AudioParam::SetTargetAtTime(double Target, double T, double TimeConstant)
{
	FEvent E; E.Type = EEvent::Target; E.Time = T; E.V = Target; E.Tau = FMath::Max(0.0, TimeConstant);
	Insert(MoveTemp(E));
	return *this;
}

FS47AudioParam& FS47AudioParam::SetValueCurveAtTime(const TArray<float>& InValues, double T, double Duration)
{
	if (InValues.Num() < 2 || !(Duration > 0.0)) return *this; // the browser throws
	FEvent E; E.Type = EEvent::Curve; E.Time = T; E.Duration = Duration; E.Curve = InValues; E.V = InValues.Last();
	Insert(MoveTemp(E));
	return *this;
}

FS47AudioParam& FS47AudioParam::CancelScheduledValues(double T)
{
	FScopeLock L(&Owner.Ctx->Lock);
	OverrideV = Value(); OverrideQ = Owner.Ctx->Quantum; // (the getter keeps saying where it was)
	Events.RemoveAll([T](const FEvent& E) { return E.Time >= T; });
	return *this;
}

FS47AudioParam& FS47AudioParam::CancelAndHoldAtTime(double T)
{
	FScopeLock L(&Owner.Ctx->Lock);
	OverrideV = Value(); OverrideQ = Owner.Ctx->Quantum;
	const double V = ValueAt(T);
	const int32 K = CountAtOrBefore(T);
	if (K >= Events.Num())
	{
		// nothing after: a target or a curve still running stops where it is
		if (K > 0 && (Events[K - 1].Type == EEvent::Target || Events[K - 1].Type == EEvent::Curve)) SetValueAtTime(V, T);
		return *this;
	}
	const FEvent Next = Events[K];
	Events.SetNum(K);
	if (Next.Type == EEvent::Linear || Next.Type == EEvent::Exponential)
	{
		// the ramp that was under way ends here, on the value it had reached
		FEvent E = Next; E.Time = T; E.V = V; E.bStart = Next.bStart;
		Events.Add(MoveTemp(E));
	}
	else if (K > 0 && (Events[K - 1].Type == EEvent::Target || Events[K - 1].Type == EEvent::Curve)) SetValueAtTime(V, T);
	return *this;
}

int32 FS47AudioParam::CountAtOrBefore(double T) const
{
	int32 K = 0;
	while (K < Events.Num() && Events[K].Time <= T) ++K;
	return K;
}

double FS47AudioParam::EndValue(int32 I) const
{
	const FEvent& E = Events[I];
	return E.Type == EEvent::Curve ? double(E.Curve.Last()) : E.V;
}

double FS47AudioParam::TargetStart(int32 I) const
{
	// the value just before the target begins
	const FEvent& E = Events[I];
	if (!E.bStart)
	{
		E.StartV = Eval(I, E.Time);
		E.bStart = true;
	}
	return E.StartV;
}

void FS47AudioParam::RampStart(int32 K, double T, double& T0, double& V0) const
{
	const FEvent& N = Events[K];
	if (K == 0) { T0 = N.InsertT; V0 = N.InsertV; return; }
	const FEvent& P = Events[K - 1];
	if (P.Type == EEvent::Target)
	{
		// after a target, the ramp starts from where the target has got to when the ramp takes over
		if (!N.bStart)
		{
			N.StartT = T;
			N.StartV = P.Tau > 0.0 ? P.V + (TargetStart(K - 1) - P.V) * FMath::Exp(-(T - P.Time) / P.Tau) : P.V;
			N.bStart = true;
		}
		T0 = N.StartT; V0 = N.StartV;
		return;
	}
	T0 = P.Type == EEvent::Curve ? P.Time + P.Duration : P.Time;
	V0 = EndValue(K - 1);
}

double FS47AudioParam::Eval(int32 K, double T) const
{
	if (K < Events.Num() && (Events[K].Type == EEvent::Linear || Events[K].Type == EEvent::Exponential))
	{
		const FEvent& N = Events[K];
		double T0 = 0, V0 = 0;
		RampStart(K, T, T0, V0);
		if (N.Time <= T0) return N.V;
		const double F = FMath::Clamp((T - T0) / (N.Time - T0), 0.0, 1.0);
		if (N.Type == EEvent::Linear) return V0 + (N.V - V0) * F;
		if (V0 == 0.0 || V0 * N.V < 0.0) return V0; // no exponential through zero: it holds
		return V0 * FMath::Pow(N.V / V0, F);
	}
	if (K == 0) return Intrinsic;
	const FEvent& P = Events[K - 1];
	switch (P.Type)
	{
	case EEvent::Target:
		return P.Tau > 0.0 ? P.V + (TargetStart(K - 1) - P.V) * FMath::Exp(-(T - P.Time) / P.Tau) : P.V;
	case EEvent::Curve:
	{
		const int32 N = P.Curve.Num();
		if (T >= P.Time + P.Duration) return P.Curve.Last();
		const double X = (T - P.Time) * (N - 1) / P.Duration;
		const int32 C = FMath::Clamp(int32(X), 0, N - 2);
		return P.Curve[C] + (P.Curve[C + 1] - P.Curve[C]) * (X - C);
	}
	default:
		return P.V;
	}
}

double FS47AudioParam::ValueAt(double T) const
{
	if (Events.Num() == 0) return Intrinsic;
	return Eval(CountAtOrBefore(T), T);
}

void FS47AudioParam::Prune(double T)
{
	// events whose part is over: what comes after them no longer depends on them
	while (Events.Num() >= 2 && Events[1].Time <= T)
	{
		if (Events[1].Type == EEvent::Target) TargetStart(1);
		Events.RemoveAt(0, 1, EAllowShrinking::No);
	}
	if (Events.Num() == 1 && Events[0].Time <= T)
	{
		const FEvent& E = Events[0];
		bool bOver = true;
		if (E.Type == EEvent::Target) bOver = E.Tau <= 0.0 || (T - E.Time) > E.Tau * 30.0;
		else if (E.Type == EEvent::Curve) bOver = T >= E.Time + E.Duration;
		if (bOver)
		{
			Intrinsic = E.Type == EEvent::Target ? E.V : EndValue(0);
			Events.Reset();
		}
	}
}

const float* FS47AudioParam::Render(int64 Quantum, bool& bConst)
{
	if (RenderedQ != Quantum)
	{
		RenderedQ = Quantum;
		const FS47AudioContext& C = Owner.Ctx.Get();
		const double Sr = C.SampleRate, Dt = 1.0 / Sr, T0 = C.QuantumFrame / Sr;
		Prune(T0);
		bLastConst = true;
		if (Events.Num() == 0 || !bARate)
		{
			// k-rate: the value at the first frame of the quantum
			const float V = float(Clamp(Events.Num() ? ValueAt(T0) : Intrinsic));
			for (float& X : Values) X = V;
			LastIntrinsic = V;
		}
		else
		{
			bLastConst = false;
			int32 K = CountAtOrBefore(T0);
			int32 I = 0;
			while (I < S47Quantum)
			{
				const double T = T0 + I * Dt;
				while (K < Events.Num() && Events[K].Time <= T) ++K;
				// the frames before the next event share one formula
				int32 End = S47Quantum;
				if (K < Events.Num()) End = FMath::Clamp(int32(FMath::CeilToDouble((Events[K].Time - T0) * Sr - 1e-9)), I + 1, S47Quantum);
				const bool bRamp = K < Events.Num() && (Events[K].Type == EEvent::Linear || Events[K].Type == EEvent::Exponential);
				if (!bRamp && K > 0 && Events[K - 1].Type == EEvent::Target && Events[K - 1].Tau > 0.0)
				{
					// setTargetAtTime: v(t) = V1 + (V0 - V1) e^(-(t - T0) / tau), one step at a time
					const FEvent& P = Events[K - 1];
					double V = P.V + (TargetStart(K - 1) - P.V) * FMath::Exp(-(T - P.Time) / P.Tau);
					const double Kf = FMath::Exp(-Dt / P.Tau);
					for (int32 J = I; J < End; ++J) { Values[J] = float(Clamp(V)); V = P.V + (V - P.V) * Kf; }
				}
				else if (!bRamp && (K == 0 || Events[K - 1].Type != EEvent::Curve))
				{
					const float V = float(Clamp(Eval(K, T)));
					for (int32 J = I; J < End; ++J) Values[J] = V;
				}
				else for (int32 J = I; J < End; ++J) Values[J] = float(Clamp(Eval(K, T0 + J * Dt)));
				I = End;
			}
			LastIntrinsic = Values[S47Quantum - 1];
		}
		// the signals connected into the parameter are added to it (mixed down to mono)
		if (Inputs.Num())
		{
			bool bAny = false;
			for (int32 N = Inputs.Num() - 1; N >= 0; --N)
			{
				TSharedPtr<FS47AudioNode> In = Inputs[N].Pin();
				if (!In.IsValid()) { Inputs.RemoveAtSwap(N); continue; }
				const FS47AudioBlock& B = In->Pull(Quantum);
				bAny = true;
				if (B.Channels == 1) for (int32 J = 0; J < S47Quantum; ++J) Values[J] += B.D[0][J];
				else for (int32 J = 0; J < S47Quantum; ++J) Values[J] += 0.5f * (B.D[0][J] + B.D[1][J]);
			}
			if (bAny)
			{
				bLastConst = false;
				for (float& X : Values) X = float(Clamp(X));
			}
		}
	}
	bConst = bLastConst;
	return Values;
}

// ---------- AudioNode ----------

FS47AudioNode::FS47AudioNode(const TSharedRef<FS47AudioContext>& InCtx) : Ctx(InCtx)
{
	Out.Zero(1);
}

void FS47AudioNode::Connect(const TSharedPtr<FS47AudioNode>& To)
{
	if (!To.IsValid()) return;
	FScopeLock L(&Ctx->Lock);
	if (OutNodes.Contains(To)) return; // connecting twice is one connection
	OutNodes.Add(To);
	To->Inputs.Add(AsShared());
}

void FS47AudioNode::Connect(FS47AudioParam& To)
{
	FScopeLock L(&Ctx->Lock);
	for (const FParamLink& P : OutParams) if (P.Param == &To) return;
	OutParams.Add({ To.Owner.AsShared(), &To });
	To.Inputs.Add(AsShared());
}

void FS47AudioNode::Disconnect()
{
	FScopeLock L(&Ctx->Lock);
	const FS47AudioNode* Self = this;
	auto IsSelf = [Self](const TWeakPtr<FS47AudioNode>& W) { return !W.IsValid() || W.HasSameObject(Self); };
	for (const TSharedPtr<FS47AudioNode>& N : OutNodes) N->Inputs.RemoveAll(IsSelf);
	for (const FParamLink& P : OutParams) P.Param->Inputs.RemoveAll(IsSelf);
	// (the nodes this one fed may go now, on this thread, under the lock)
	OutNodes.Reset();
	OutParams.Reset();
}

const FS47AudioBlock& FS47AudioNode::Pull(int64 Quantum)
{
	if (Done != Quantum)
	{
		Done = Quantum; // (a loop that comes back here gets the last output: a loop needs a delay)
		Process(Quantum);
	}
	return Out;
}

void FS47AudioNode::PullInputs(int64 Quantum, FS47AudioBlock& In, int32 MaxChannels, int32 Explicit)
{
	TArray<TSharedPtr<FS47AudioNode>, TInlineAllocator<16>> Live;
	TArray<const FS47AudioBlock*, TInlineAllocator<16>> Blocks;
	int32 Most = 1;
	for (int32 N = Inputs.Num() - 1; N >= 0; --N)
	{
		TSharedPtr<FS47AudioNode> Node = Inputs[N].Pin();
		if (!Node.IsValid()) { Inputs.RemoveAtSwap(N); continue; }
		const FS47AudioBlock& B = Node->Pull(Quantum);
		Most = FMath::Max(Most, B.Channels);
		Live.Add(Node);
		Blocks.Add(&B);
	}
	// channelCountMode 'max' (clamped to MaxChannels) or 'explicit'; the speakers up- and down-mix
	const int32 Ch = Explicit > 0 ? Explicit : FMath::Clamp(Most, 1, MaxChannels);
	In.Zero(Ch);
	for (const FS47AudioBlock* B : Blocks)
	{
		if (B->Channels == Ch) { for (int32 C = 0; C < Ch; ++C) for (int32 J = 0; J < S47Quantum; ++J) In.D[C][J] += B->D[C][J]; }
		else if (B->Channels == 1) { for (int32 C = 0; C < Ch; ++C) for (int32 J = 0; J < S47Quantum; ++J) In.D[C][J] += B->D[0][J]; }
		else for (int32 J = 0; J < S47Quantum; ++J) In.D[0][J] += 0.5f * (B->D[0][J] + B->D[1][J]);
	}
}

// ---------- GainNode ----------

FS47GainNode::FS47GainNode(const TSharedRef<FS47AudioContext>& InCtx)
	: FS47AudioNode(InCtx), Gain(*this, 1.0, -FLT_MAX, FLT_MAX)
{
}

void FS47GainNode::Process(int64 Quantum)
{
	PullInputs(Quantum, In);
	bool bConst = true;
	const float* G = Gain.Render(Quantum, bConst);
	Out.Channels = In.Channels;
	for (int32 C = 0; C < In.Channels; ++C)
	{
		if (bConst) { const float K = G[0]; for (int32 J = 0; J < S47Quantum; ++J) Out.D[C][J] = In.D[C][J] * K; }
		else for (int32 J = 0; J < S47Quantum; ++J) Out.D[C][J] = In.D[C][J] * G[J];
	}
}

// ---------- AudioScheduledSourceNode ----------

void FS47ScheduledSourceNode::Start(double When)
{
	FScopeLock L(&Ctx->Lock);
	if (bStarted) return; // InvalidStateError in the browser
	bStarted = true;
	StartFrame = int64(FMath::CeilToDouble(FMath::Max(0.0, When) * Ctx->SampleRate - 1e-6));
	Ctx->Active.Add(StaticCastSharedRef<FS47ScheduledSourceNode>(AsShared()));
}

void FS47ScheduledSourceNode::Stop(double When)
{
	FScopeLock L(&Ctx->Lock);
	if (!bStarted || bEnded) return;
	StopFrame = int64(FMath::CeilToDouble(FMath::Max(0.0, When) * Ctx->SampleRate - 1e-6));
}

void FS47ScheduledSourceNode::Finish()
{
	bEnded = true;
}

// ---------- OscillatorNode ----------

FS47OscillatorNode::FS47OscillatorNode(const TSharedRef<FS47AudioContext>& InCtx)
	: FS47ScheduledSourceNode(InCtx), Frequency(*this, 440.0, -InCtx->SampleRate * 0.5, InCtx->SampleRate * 0.5), Detune(*this, 0.0, -153600.0, 153600.0)
{
}

void FS47OscillatorNode::SetType(const FString& Name)
{
	Type = S47Wa::OscType(Name);
}

namespace
{
	// PolyBLEP: the step of an ideal edge, smoothed over one sample either side
	double PolyBlep(double T, double Dt)
	{
		if (Dt <= 0.0) return 0.0;
		if (T < Dt) { T /= Dt; return T + T - T * T - 1.0; }
		if (T > 1.0 - Dt) { T = (T - 1.0) / Dt; return T * T + T + T + 1.0; }
		return 0.0;
	}
	double Frac(double X) { return X - FMath::FloorToDouble(X); }
}

double FS47OscillatorNode::Shape(ES47OscType InType, double P, double Dt)
{
	// the browser's phases: the sine and the sawtooth start at 0 going up, the square at +1,
	// the triangle at 0 going up to +1 at a quarter period
	// The browser's band-limited tables are scaled so their peak is 1: the square's and the
	// sawtooth's Gibbs overshoot (2 Si(pi) / pi = 1.17898) makes them that much quieter than +-1.
	constexpr double Gibbs = 1.0 / 1.1789797444721672;
	switch (InType)
	{
	case ES47OscType::Square: return Gibbs * ((P < 0.5 ? 1.0 : -1.0) + PolyBlep(P, Dt) - PolyBlep(Frac(P + 0.5), Dt));
	case ES47OscType::Sawtooth: { const double U = Frac(P + 0.5); return Gibbs * (2.0 * U - 1.0 - PolyBlep(U, Dt)); }
	case ES47OscType::Triangle: return P < 0.25 ? 4.0 * P : P < 0.75 ? 2.0 - 4.0 * P : 4.0 * P - 4.0;
	default: return FMath::Sin(2.0 * Pi * P);
	}
}

void FS47OscillatorNode::Process(int64 Quantum)
{
	Out.Zero(1);
	bool bFC = true, bDC = true;
	const float* F = Frequency.Render(Quantum, bFC);
	const float* Dn = Detune.Render(Quantum, bDC);
	const int64 F0 = Ctx->QuantumFrame;
	const double Sr = Ctx->SampleRate;
	for (int32 J = 0; J < S47Quantum; ++J)
	{
		const int64 Fr = F0 + J;
		if (!bStarted || Fr < StartFrame || Fr >= StopFrame) continue;
		double Hz = F[J];
		if (Dn[J] != 0.f) Hz *= Exp2(Dn[J] / 1200.0);
		const double Dt = Hz / Sr;
		Out.D[0][J] = float(Shape(Type, Phase, FMath::Abs(Dt)));
		Phase = Frac(Phase + Dt);
	}
	if (bStarted && F0 + S47Quantum >= StopFrame) Finish();
}

// ---------- AudioBufferSourceNode ----------

FS47BufferSourceNode::FS47BufferSourceNode(const TSharedRef<FS47AudioContext>& InCtx)
	: FS47ScheduledSourceNode(InCtx), PlaybackRate(*this, 1.0, -FLT_MAX, FLT_MAX, false), Detune(*this, 0.0, -FLT_MAX, FLT_MAX, false)
{
}

void FS47BufferSourceNode::Start(double When, double Offset, double Duration)
{
	FScopeLock L(&Ctx->Lock);
	if (bStarted) return;
	if (Buffer.IsValid())
	{
		const double Len = Buffer->Length();
		Pos = FMath::Clamp(FMath::Max(0.0, Offset) * Buffer->SampleRate(), 0.0, Len);
		if (bLoop && Len > 0 && Pos >= Len) Pos = FMath::Fmod(Pos, Len);
	}
	Limit = Duration;
	FS47ScheduledSourceNode::Start(When);
}

void FS47BufferSourceNode::Process(int64 Quantum)
{
	const int32 Ch = Buffer.IsValid() ? FMath::Min(2, Buffer->NumberOfChannels()) : 1;
	Out.Zero(Ch);
	bool bC = true;
	const double Rate = PlaybackRate.Render(Quantum, bC)[0] * Exp2(Detune.Render(Quantum, bC)[0] / 1200.0);
	const int64 F0 = Ctx->QuantumFrame;
	if (!bStarted || bEnded) return;
	if (!Buffer.IsValid() || Buffer->Length() == 0)
	{
		if (F0 + S47Quantum >= StopFrame) Finish();
		return;
	}
	const int32 Len = Buffer->Length();
	const double BR = Buffer->SampleRate(), Step = Rate * BR / Ctx->SampleRate;
	// loopStart and loopEnd count when they make a loop inside the buffer; else the whole of it
	double Ls = 0.0, Le = Len;
	if (bLoop && LoopEnd > 0.0 && LoopStart < LoopEnd)
	{
		Ls = FMath::Clamp(LoopStart * BR, 0.0, double(Len));
		Le = FMath::Clamp(LoopEnd * BR, 0.0, double(Len));
		if (Le <= Ls) { Ls = 0.0; Le = Len; }
	}
	for (int32 J = 0; J < S47Quantum; ++J)
	{
		const int64 Fr = F0 + J;
		if (Fr < StartFrame) continue;
		if (Fr >= StopFrame || (Limit >= 0.0 && Played >= Limit)) { Finish(); break; }
		if (bLoop)
		{
			const double Span = Le - Ls;
			if (Span > 0.0)
			{
				if (Pos >= Le) Pos = Ls + FMath::Fmod(Pos - Ls, Span);
				else if (Pos < 0.0) Pos = Le - FMath::Fmod(-Pos, Span);
			}
		}
		else if (Pos >= Len || Pos < 0.0) { Finish(); break; }
		const int32 I0 = FMath::Clamp(int32(Pos), 0, Len - 1);
		const double Fp = Pos - I0;
		int32 I1 = I0 + 1;
		if (I1 >= (bLoop ? int32(Le) : Len)) I1 = bLoop ? int32(Ls) : -1;
		for (int32 C = 0; C < Ch; ++C)
		{
			const float* D = Buffer->GetChannelData(C);
			const float S0 = D[I0], S1 = I1 >= 0 && I1 < Len ? D[I1] : 0.f;
			Out.D[C][J] = float(S0 + (S1 - S0) * Fp);
		}
		Pos += Step;
		Played += FMath::Abs(Step) / BR;
	}
	if (F0 + S47Quantum >= StopFrame) Finish();
}

// ---------- BiquadFilterNode ----------

FS47BiquadFilterNode::FS47BiquadFilterNode(const TSharedRef<FS47AudioContext>& InCtx)
	: FS47AudioNode(InCtx), Frequency(*this, 350.0, 0.0, InCtx->SampleRate * 0.5), Detune(*this, 0.0, -153600.0, 153600.0), Q(*this, 1.0, -FLT_MAX, FLT_MAX), Gain(*this, 0.0, -FLT_MAX, 1541.0)
{
}

void FS47BiquadFilterNode::SetType(const FString& Name)
{
	Type = S47Wa::BiquadType(Name);
}

void FS47BiquadFilterNode::Coefficients(ES47BiquadType InType, double SampleRate, double Freq, double InDetune, double InQ, double GainDb, double Out[5])
{
	const double Nyq = SampleRate * 0.5;
	double F0 = InDetune != 0.0 ? Freq * Exp2(InDetune / 1200.0) : Freq;
	F0 = FMath::Clamp(F0, 0.0, Nyq);
	const double A = FMath::Pow(10.0, GainDb / 40.0);
	auto Set = [Out](double B0, double B1, double B2, double A0, double A1, double A2)
	{
		Out[0] = B0 / A0; Out[1] = B1 / A0; Out[2] = B2 / A0; Out[3] = A1 / A0; Out[4] = A2 / A0;
	};
	// at 0 Hz and at the Nyquist frequency the formulas degenerate; Chromium's values there
	if (F0 <= 0.0 || F0 >= Nyq)
	{
		const bool bTop = F0 >= Nyq;
		double K = 1.0;
		switch (InType)
		{
		case ES47BiquadType::Lowpass: K = bTop ? 1.0 : 0.0; break;
		case ES47BiquadType::Highpass: K = bTop ? 0.0 : 1.0; break;
		case ES47BiquadType::Bandpass: K = 0.0; break;
		case ES47BiquadType::Lowshelf: K = bTop ? A * A : 1.0; break;
		case ES47BiquadType::Highshelf: K = bTop ? 1.0 : A * A; break;
		default: K = 1.0; break;
		}
		Set(K, 0, 0, 1, 0, 0);
		return;
	}
	const double W0 = 2.0 * Pi * F0 / SampleRate;
	const double Cw = FMath::Cos(W0), Sw = FMath::Sin(W0);
	const double AlphaQ = Sw / (2.0 * FMath::Max(InQ, 1e-4));
	const double AlphaQdB = Sw / (2.0 * FMath::Pow(10.0, InQ / 20.0));
	const double AlphaS = Sw / 2.0 * FMath::Sqrt((A + 1.0 / A) * (1.0 / 1.0 - 1.0) + 2.0); // S = 1
	const double SA = FMath::Sqrt(A);
	switch (InType)
	{
	case ES47BiquadType::Lowpass: Set((1 - Cw) / 2, 1 - Cw, (1 - Cw) / 2, 1 + AlphaQdB, -2 * Cw, 1 - AlphaQdB); break;
	case ES47BiquadType::Highpass: Set((1 + Cw) / 2, -(1 + Cw), (1 + Cw) / 2, 1 + AlphaQdB, -2 * Cw, 1 - AlphaQdB); break;
	case ES47BiquadType::Bandpass: Set(AlphaQ, 0, -AlphaQ, 1 + AlphaQ, -2 * Cw, 1 - AlphaQ); break;
	case ES47BiquadType::Notch: Set(1, -2 * Cw, 1, 1 + AlphaQ, -2 * Cw, 1 - AlphaQ); break;
	case ES47BiquadType::Allpass: Set(1 - AlphaQ, -2 * Cw, 1 + AlphaQ, 1 + AlphaQ, -2 * Cw, 1 - AlphaQ); break;
	case ES47BiquadType::Peaking: Set(1 + AlphaQ * A, -2 * Cw, 1 - AlphaQ * A, 1 + AlphaQ / A, -2 * Cw, 1 - AlphaQ / A); break;
	case ES47BiquadType::Lowshelf:
		Set(A * ((A + 1) - (A - 1) * Cw + 2 * AlphaS * SA), 2 * A * ((A - 1) - (A + 1) * Cw), A * ((A + 1) - (A - 1) * Cw - 2 * AlphaS * SA),
			(A + 1) + (A - 1) * Cw + 2 * AlphaS * SA, -2 * ((A - 1) + (A + 1) * Cw), (A + 1) + (A - 1) * Cw - 2 * AlphaS * SA);
		break;
	case ES47BiquadType::Highshelf:
		Set(A * ((A + 1) + (A - 1) * Cw + 2 * AlphaS * SA), -2 * A * ((A - 1) + (A + 1) * Cw), A * ((A + 1) + (A - 1) * Cw - 2 * AlphaS * SA),
			(A + 1) - (A - 1) * Cw + 2 * AlphaS * SA, 2 * ((A - 1) - (A + 1) * Cw), (A + 1) - (A - 1) * Cw - 2 * AlphaS * SA);
		break;
	}
}

void FS47BiquadFilterNode::Process(int64 Quantum)
{
	PullInputs(Quantum, In);
	const int32 Ch = In.Channels;
	if (Ch != LastChannels)
	{
		if (Ch == 2) { X1[1] = X1[0]; X2[1] = X2[0]; Y1[1] = Y1[0]; Y2[1] = Y2[0]; }
		LastChannels = Ch;
	}
	Out.Channels = Ch;
	bool bF = true, bD = true, bQ = true, bG = true;
	const float* F = Frequency.Render(Quantum, bF);
	const float* Dn = Detune.Render(Quantum, bD);
	const float* Qv = Q.Render(Quantum, bQ);
	const float* G = Gain.Render(Quantum, bG);
	// the coefficients once a quantum, or per frame while a parameter moves (as Chromium does)
	const bool bConst = bF && bD && bQ && bG;
	double K[5];
	if (bConst) Coefficients(Type, Ctx->SampleRate, F[0], Dn[0], Qv[0], G[0], K);
	for (int32 J = 0; J < S47Quantum; ++J)
	{
		if (!bConst) Coefficients(Type, Ctx->SampleRate, F[J], Dn[J], Qv[J], G[J], K);
		for (int32 C = 0; C < Ch; ++C)
		{
			const double X = In.D[C][J];
			const double Y = K[0] * X + K[1] * X1[C] + K[2] * X2[C] - K[3] * Y1[C] - K[4] * Y2[C];
			X2[C] = X1[C]; X1[C] = X;
			Y2[C] = Y1[C]; Y1[C] = Flush(Y);
			Out.D[C][J] = float(Y1[C]);
		}
	}
}

// ---------- PannerNode ----------

FS47PannerNode::FS47PannerNode(const TSharedRef<FS47AudioContext>& InCtx)
	: FS47AudioNode(InCtx), PositionX(*this, 0.0, -FLT_MAX, FLT_MAX), PositionY(*this, 0.0, -FLT_MAX, FLT_MAX), PositionZ(*this, 0.0, -FLT_MAX, FLT_MAX)
{
}

void FS47PannerNode::SetPosition(double X, double Y, double Z)
{
	FScopeLock L(&Ctx->Lock);
	PositionX.SetValue(X); PositionY.SetValue(Y); PositionZ.SetValue(Z);
}

void FS47PannerNode::SetDistanceModel(const FString& Name)
{
	DistanceModel = Name == TEXT("linear") ? ES47DistanceModel::Linear : Name == TEXT("exponential") ? ES47DistanceModel::Exponential : ES47DistanceModel::Inverse;
}

double FS47PannerNode::DistanceGain(ES47DistanceModel Model, double Distance, double Ref, double Max, double Rolloff)
{
	switch (Model)
	{
	case ES47DistanceModel::Linear:
	{
		if (Max <= Ref) return 1.0;
		const double D = FMath::Clamp(Distance, Ref, Max);
		return 1.0 - FMath::Clamp(Rolloff, 0.0, 1.0) * (D - Ref) / (Max - Ref);
	}
	case ES47DistanceModel::Exponential:
	{
		if (Ref <= 0.0) return 0.0;
		return FMath::Pow(FMath::Max(Distance, Ref) / Ref, -Rolloff);
	}
	default:
	{
		const double D = FMath::Max(Distance, Ref);
		const double Den = Ref + Rolloff * (D - Ref);
		return Den > 0.0 ? Ref / Den : 0.0;
	}
	}
}

double FS47PannerNode::Azimuth(const FVector& Source, const FVector& Listener, const FVector& Forward, const FVector& Up)
{
	// the specification's azimuth: source relative to the listener, projected on the listener's
	// horizontal plane, measured from straight ahead (+90 is to the right)
	FVector SL = Source - Listener;
	if (SL.IsNearlyZero(1e-9)) return 0.0;
	SL.Normalize();
	const FVector Fw = Forward.GetSafeNormal();
	const FVector Right = FVector::CrossProduct(Fw, Up).GetSafeNormal();
	const FVector UpN = FVector::CrossProduct(Right, Fw).GetSafeNormal();
	const double UpProj = FVector::DotProduct(SL, UpN);
	const FVector Proj = (SL - UpProj * UpN).GetSafeNormal();
	double Az = 180.0 / Pi * FMath::Acos(FMath::Clamp(FVector::DotProduct(Proj, Right), -1.0, 1.0));
	if (FVector::DotProduct(Proj, Fw) < 0.0) Az = 360.0 - Az;
	if (Az >= 0.0 && Az <= 270.0) Az = 90.0 - Az;
	else Az = 450.0 - Az;
	return Az;
}

void FS47PannerNode::EqualPower(double Az, int32 InChannels, double& GainL, double& GainR)
{
	Az = FMath::Clamp(Az, -180.0, 180.0);
	if (Az < -90.0) Az = -180.0 - Az;
	else if (Az > 90.0) Az = 180.0 - Az;
	const double X = InChannels == 1 ? (Az + 90.0) / 180.0 : Az <= 0.0 ? (Az + 90.0) / 90.0 : Az / 90.0;
	GainL = FMath::Cos(X * Pi / 2.0);
	GainR = FMath::Sin(X * Pi / 2.0);
}

void FS47PannerNode::Process(int64 Quantum)
{
	PullInputs(Quantum, In, 2);
	Out.Zero(2);
	const TSharedPtr<FS47AudioListener> Li = Ctx->Listener;
	if (!Li.IsValid()) return;
	bool bC = true;
	auto K = [Quantum, &bC](FS47AudioParam& P) { return double(P.Render(Quantum, bC)[0]); };
	const FVector L(K(Li->PositionX), K(Li->PositionY), K(Li->PositionZ));
	const FVector Fw(K(Li->ForwardX), K(Li->ForwardY), K(Li->ForwardZ));
	const FVector Up(K(Li->UpX), K(Li->UpY), K(Li->UpZ));
	const FVector S(K(PositionX), K(PositionY), K(PositionZ));
	const double Gd = DistanceGain(DistanceModel, FVector::Dist(S, L), RefDistance, MaxDistance, RolloffFactor);
	const double Az = Azimuth(S, L, Fw, Up);
	double GL = 0, GR = 0;
	EqualPower(Az, In.Channels, GL, GR);
	if (In.Channels == 1)
	{
		GL *= Gd; GR *= Gd;
		// the gains glide from the last quantum's, so a moving listener does not click
		if (LastL < 0.0) { LastL = GL; LastR = GR; }
		for (int32 J = 0; J < S47Quantum; ++J)
		{
			const double F = (J + 1) / double(S47Quantum);
			const double X = In.D[0][J];
			Out.D[0][J] = float(X * (LastL + (GL - LastL) * F));
			Out.D[1][J] = float(X * (LastR + (GR - LastR) * F));
		}
		LastL = GL; LastR = GR;
		return;
	}
	for (int32 J = 0; J < S47Quantum; ++J)
	{
		const double XL = In.D[0][J], XR = In.D[1][J];
		if (Az <= 0.0) { Out.D[0][J] = float((XL + XR * GL) * Gd); Out.D[1][J] = float(XR * GR * Gd); }
		else { Out.D[0][J] = float(XL * GL * Gd); Out.D[1][J] = float((XR + XL * GR) * Gd); }
	}
}

// ---------- StereoPannerNode ----------

FS47StereoPannerNode::FS47StereoPannerNode(const TSharedRef<FS47AudioContext>& InCtx)
	: FS47AudioNode(InCtx), Pan(*this, 0.0, -1.0, 1.0)
{
}

void FS47StereoPannerNode::Process(int64 Quantum)
{
	PullInputs(Quantum, In, 2);
	Out.Zero(2);
	bool bC = true;
	const float* P = Pan.Render(Quantum, bC);
	for (int32 J = 0; J < S47Quantum; ++J)
	{
		const double Pv = P[J];
		if (In.Channels == 1)
		{
			const double X = (Pv + 1.0) / 2.0;
			Out.D[0][J] = float(In.D[0][J] * FMath::Cos(X * Pi / 2.0));
			Out.D[1][J] = float(In.D[0][J] * FMath::Sin(X * Pi / 2.0));
		}
		else if (Pv <= 0.0)
		{
			const double X = Pv + 1.0;
			Out.D[0][J] = float(In.D[0][J] + In.D[1][J] * FMath::Cos(X * Pi / 2.0));
			Out.D[1][J] = float(In.D[1][J] * FMath::Sin(X * Pi / 2.0));
		}
		else
		{
			const double X = Pv;
			Out.D[0][J] = float(In.D[0][J] * FMath::Cos(X * Pi / 2.0));
			Out.D[1][J] = float(In.D[1][J] + In.D[0][J] * FMath::Sin(X * Pi / 2.0));
		}
	}
}

// ---------- DelayNode ----------

FS47DelayNode::FS47DelayNode(const TSharedRef<FS47AudioContext>& InCtx, double MaxDelayTime)
	: FS47AudioNode(InCtx), DelayTime(*this, 0.0, 0.0, FMath::Max(MaxDelayTime, 1e-5))
{
	const int32 N = int32(FMath::CeilToDouble(FMath::Max(MaxDelayTime, 1e-5) * InCtx->SampleRate)) + 2 * S47Quantum + 4;
	Ring[0].SetNumZeroed(N);
	Ring[1].SetNumZeroed(N);
	In.Zero(1);
}

const FS47AudioBlock& FS47DelayNode::Pull(int64 Quantum)
{
	if (Done == Quantum)
	{
		// back here while pulling its own input: it is in a loop (its delay is then at least a quantum)
		if (bBusy) bInCycle = true;
		return Out;
	}
	Done = Quantum;
	// the input first: a loop back into it gets this node's last quantum (Out, not yet replaced), as
	// in Chromium, where a loop through a delay is one quantum longer than the delay
	bBusy = true; PullInputs(Quantum, In); bBusy = false;
	WriteIn(Quantum);
	Read(Quantum);
	Write = (Write + S47Quantum) % Ring[0].Num();
	return Out;
}

void FS47DelayNode::WriteIn(int64 Quantum)
{
	const int32 N = Ring[0].Num();
	if (In.Channels == 2 && RingChannels == 1) { Ring[1] = Ring[0]; RingChannels = 2; }
	for (int32 C = 0; C < RingChannels; ++C)
	{
		const int32 Src = FMath::Min(C, In.Channels - 1);
		for (int32 J = 0; J < S47Quantum; ++J) Ring[C][(Write + J) % N] = In.D[Src][J];
	}
}

void FS47DelayNode::Read(int64 Quantum)
{
	const int32 N = Ring[0].Num();
	bool bC = true;
	const float* Dt = DelayTime.Render(Quantum, bC);
	Out.Zero(RingChannels);
	const double Sr = Ctx->SampleRate, Min = bInCycle ? double(S47Quantum) : 0.0;
	for (int32 J = 0; J < S47Quantum; ++J)
	{
		const double D = FMath::Clamp(Dt[J] * Sr, Min, double(N - 2 * S47Quantum));
		const double P = double(Write + J) - D;
		const double Fl = FMath::FloorToDouble(P);
		const double Fp = P - Fl;
		const int32 I0 = int32(((int64(Fl) % N) + N) % N), I1 = (I0 + 1) % N;
		for (int32 C = 0; C < RingChannels; ++C) Out.D[C][J] = float(Ring[C][I0] + (Ring[C][I1] - Ring[C][I0]) * Fp);
	}
}

// ---------- WaveShaperNode ----------

float FS47WaveShaperNode::Shape(const TArray<float>& InCurve, float X)
{
	const int32 N = InCurve.Num();
	if (N == 0) return X;
	if (N == 1) return InCurve[0];
	const double V = (N - 1) / 2.0 * (X + 1.0);
	if (V <= 0.0) return InCurve[0];
	if (V >= N - 1) return InCurve[N - 1];
	const int32 K = int32(V);
	const double F = V - K;
	return float((1.0 - F) * InCurve[K] + F * InCurve[K + 1]);
}

void FS47WaveShaperNode::Process(int64 Quantum)
{
	PullInputs(Quantum, In);
	Out.Channels = In.Channels;
	for (int32 C = 0; C < In.Channels; ++C) for (int32 J = 0; J < S47Quantum; ++J) Out.D[C][J] = Shape(Curve, In.D[C][J]);
}

// ---------- DynamicsCompressorNode ----------

FS47DynamicsCompressorNode::FS47DynamicsCompressorNode(const TSharedRef<FS47AudioContext>& InCtx)
	: FS47AudioNode(InCtx), Threshold(*this, -24.0, -100.0, 0.0, false), Knee(*this, 30.0, 0.0, 40.0, false), Ratio(*this, 12.0, 1.0, 20.0, false),
	Attack(*this, 0.003, 0.0, 1.0, false), Release(*this, 0.25, 0.0, 1.0, false)
{
	Pre[0].SetNumZeroed(1024); // Chromium's pre-delay buffers
	Pre[1].SetNumZeroed(1024);
}

namespace
{
	// Chromium's DynamicsCompressorKernel: an exponential knee matched to the ratio's slope
	struct FCompCurve
	{
		double LinThreshold = 0, K = 5, KneeThresholdDb = 0, KneeThreshold = 0, YKneeThresholdDb = 0, Slope = 1;
		double KneeCurve(double X) const { return X < LinThreshold ? X : LinThreshold + (1.0 - FMath::Exp(-K * (X - LinThreshold))) / K; }
		double SlopeAt(double X) const
		{
			if (X < LinThreshold) return 1.0;
			const double X2 = X * 1.001;
			const double XDb = LinToDb(X), X2Db = LinToDb(X2), YDb = LinToDb(KneeCurve(X)), Y2Db = LinToDb(KneeCurve(X2));
			return (Y2Db - YDb) / (X2Db - XDb);
		}
		FCompCurve(double ThresholdDb, double KneeDb, double Ratio)
		{
			LinThreshold = DbToLin(ThresholdDb);
			Slope = 1.0 / FMath::Max(Ratio, 1.0);
			// k at the slope: a geometric search, fifteen steps
			const double X = DbToLin(ThresholdDb + KneeDb);
			double MinK = 0.1, MaxK = 10000.0;
			K = 5.0;
			for (int32 I = 0; I < 15; ++I)
			{
				if (SlopeAt(X) < Slope) MaxK = K; else MinK = K;
				K = FMath::Sqrt(MinK * MaxK);
			}
			KneeThresholdDb = ThresholdDb + KneeDb;
			KneeThreshold = DbToLin(KneeThresholdDb);
			YKneeThresholdDb = LinToDb(KneeCurve(KneeThreshold));
		}
		double Saturate(double X) const
		{
			if (X < KneeThreshold) return KneeCurve(X);
			return DbToLin(YKneeThresholdDb + Slope * (LinToDb(X) - KneeThresholdDb));
		}
	};
}

double FS47DynamicsCompressorNode::Curve(double X, double ThresholdDb, double KneeDb, double InRatio)
{
	return FCompCurve(ThresholdDb, KneeDb, InRatio).Saturate(X);
}

double FS47DynamicsCompressorNode::Makeup(double ThresholdDb, double KneeDb, double InRatio)
{
	// Chromium: the full-range gain made up, to the power 0.6 ("empirical/perceptual tuning")
	const double Full = FCompCurve(ThresholdDb, KneeDb, InRatio).Saturate(1.0);
	return Full > 0.0 ? FMath::Pow(1.0 / Full, 0.6) : 1.0;
}

void FS47DynamicsCompressorNode::Process(int64 Quantum)
{
	// Chromium's DynamicsCompressorKernel::Process, with the node's fixed settings: pre-delay 6 ms,
	// post gain 0 dB, all wet, release zones 0.09, 0.16, 0.42 and 0.98. In floats, as there.
	PullInputs(Quantum, In, 2);
	const int32 Ch = In.Channels;
	Out.Channels = Ch;
	bool bC = true;
	const double Thr = Threshold.Render(Quantum, bC)[0], Kn = Knee.Render(Quantum, bC)[0], Ra = Ratio.Render(Quantum, bC)[0];
	const float AttackTime = FMath::Max(0.001f, Attack.Render(Quantum, bC)[0]), ReleaseTime = Release.Render(Quantum, bC)[0];
	const FCompCurve Cv(Thr, Kn, Ra);
	const float Sr = Ctx->SampleRate;
	constexpr float HalfPi = 1.57079632679489661923f;
	auto Db = [](float X) { return 20.f * FMath::LogX(10.f, X); };
	auto Lin = [](float X) { return FMath::Pow(10.f, 0.05f * X); };
	// make-up gain: the full range made up, to the power 0.6 ("empirical/perceptual tuning")
	const float FullRangeGain = float(Cv.Saturate(1.0));
	const float MasterLinearGain = Lin(0.f) * FMath::Pow(1.f / FullRangeGain, 0.6f);
	const float AttackFrames = AttackTime * Sr;
	const float ReleaseFrames = Sr * ReleaseTime;
	const float SatReleaseFrames = 0.0025f * Sr; // the detector's release
	// a smooth curve through the four release zones (x 0..3 for -15..0 dB of compression)
	const float Y1 = ReleaseFrames * 0.09f, Y2 = ReleaseFrames * 0.16f, Y3 = ReleaseFrames * 0.42f, Y4 = ReleaseFrames * 0.98f;
	const float KA = 0.9999999999999998f * Y1 + 1.8432219684323923e-16f * Y2 - 1.9373394351676423e-16f * Y3 + 8.824516011816245e-18f * Y4;
	const float KB = -1.5788320352845888f * Y1 + 2.3305837032074286f * Y2 - 0.9141194204840429f * Y3 + 0.1623677525612032f * Y4;
	const float KC = 0.5334142869106424f * Y1 - 1.272736789213631f * Y2 + 0.9258856042207512f * Y3 - 0.18656310191776226f * Y4;
	const float KD = 0.08783463138207234f * Y1 - 0.1694162967925622f * Y2 + 0.08588057951595272f * Y3 - 0.00429891410546283f * Y4;
	const float KE = -0.042416883008123074f * Y1 + 0.1115693827987602f * Y2 - 0.09764676325265872f * Y3 + 0.028494263462021576f * Y4;
	// the pre-delay (Chromium starts at 256 frames and moves to the node's 6 ms, clearing its buffers)
	const int32 PreDelayFrames = FMath::Min(int32(0.006f * Sr), 1023);
	if (LastPreDelayFrames != PreDelayFrames)
	{
		LastPreDelayFrames = PreDelayFrames;
		for (TArray<float>& P : Pre) FMemory::Memzero(P.GetData(), P.Num() * sizeof(float));
		PreRead = 0;
		PreWrite = PreDelayFrames;
	}
	const float MeteringReleaseK = float(1.0 - FMath::Exp(-1.0 / (Sr * 0.325)));
	int32 Frame = 0;
	for (int32 Division = 0; Division < S47Quantum / 32; ++Division)
	{
		if (!FMath::IsFinite(DetectorAverage)) DetectorAverage = 1;
		const float DesiredGain = DetectorAverage;
		// pre-warp so we get the desired gain after the sine warp below
		const float ScaledDesiredGain = FMath::Asin(DesiredGain) / HalfPi;
		float EnvelopeRate;
		const bool bReleasing = ScaledDesiredGain > CompressorGain;
		float CompressionDiffDb = Db(CompressorGain / ScaledDesiredGain);
		if (bReleasing)
		{
			// adaptive release: more compression releases faster
			MaxAttackDiffDb = -1;
			if (!FMath::IsFinite(CompressionDiffDb)) CompressionDiffDb = -1;
			float X = FMath::Clamp(CompressionDiffDb, -12.f, 0.f);
			X = 0.25f * (X + 12);
			const float X2 = X * X, X3 = X2 * X, X4 = X2 * X2;
			const float Frames = KA + KB * X + KC * X2 + KD * X3 + KE * X4;
			EnvelopeRate = Lin(5.f / Frames); // 5 dB spacing
		}
		else
		{
			// attack, at a rate from the largest compression difference seen so far
			if (!FMath::IsFinite(CompressionDiffDb)) CompressionDiffDb = 1;
			if (MaxAttackDiffDb == -1 || MaxAttackDiffDb < CompressionDiffDb) MaxAttackDiffDb = CompressionDiffDb;
			const float EffAttenDiffDb = FMath::Max(0.5f, MaxAttackDiffDb);
			EnvelopeRate = 1 - FMath::Pow(0.25f / EffAttenDiffDb, 1 / AttackFrames);
		}
		for (int32 L = 0; L < 32; ++L, ++Frame)
		{
			float CompressorInput = 0;
			for (int32 C = 0; C < Ch; ++C)
			{
				const float Undelayed = In.D[C][Frame];
				Pre[C][PreWrite] = Undelayed;
				CompressorInput = FMath::Max(CompressorInput, FMath::Abs(Undelayed));
			}
			const float AbsInput = CompressorInput;
			const float Shaped = float(Cv.Saturate(AbsInput));
			const float Attenuation = AbsInput <= 0.0001f ? 1.f : Shaped / AbsInput;
			const float AttenuationDb = FMath::Max(2.f, -Db(Attenuation));
			const float SatReleaseRate = Lin(AttenuationDb / SatReleaseFrames) - 1;
			const float Rate = Attenuation > DetectorAverage ? SatReleaseRate : 1.f;
			DetectorAverage += (Attenuation - DetectorAverage) * Rate;
			DetectorAverage = FMath::Min(1.f, DetectorAverage);
			if (!FMath::IsFinite(DetectorAverage)) DetectorAverage = 1;
			if (EnvelopeRate < 1) CompressorGain += (ScaledDesiredGain - CompressorGain) * EnvelopeRate;
			else CompressorGain = FMath::Min(1.f, CompressorGain * EnvelopeRate);
			// warp the gain to smooth the exponential's corners
			const float PostWarp = FMath::Sin(HalfPi * CompressorGain);
			const float TotalGain = MasterLinearGain * PostWarp;
			const float DbRealGain = 20.f * FMath::LogX(10.f, PostWarp);
			if (DbRealGain < MeteringGain) MeteringGain = DbRealGain;
			else MeteringGain += (DbRealGain - MeteringGain) * MeteringReleaseK;
			for (int32 C = 0; C < Ch; ++C) Out.D[C][Frame] = Pre[C][PreRead] * TotalGain;
			PreRead = (PreRead + 1) & 1023;
			PreWrite = (PreWrite + 1) & 1023;
		}
		if (FMath::Abs(DetectorAverage) < 1e-30f) DetectorAverage = 0;
		if (FMath::Abs(CompressorGain) < 1e-30f) CompressorGain = 0;
	}
	Reduction = MeteringGain;
}

// ---------- AudioListener ----------

FS47AudioListener::FS47AudioListener(const TSharedRef<FS47AudioContext>& InCtx)
	: FS47AudioNode(InCtx), PositionX(*this, 0, -FLT_MAX, FLT_MAX), PositionY(*this, 0, -FLT_MAX, FLT_MAX), PositionZ(*this, 0, -FLT_MAX, FLT_MAX),
	ForwardX(*this, 0, -FLT_MAX, FLT_MAX), ForwardY(*this, 0, -FLT_MAX, FLT_MAX), ForwardZ(*this, -1, -FLT_MAX, FLT_MAX),
	UpX(*this, 0, -FLT_MAX, FLT_MAX), UpY(*this, 1, -FLT_MAX, FLT_MAX), UpZ(*this, 0, -FLT_MAX, FLT_MAX)
{
}

// ---------- AudioContext ----------

TSharedRef<FS47AudioContext> FS47AudioContext::Make(float InSampleRate)
{
	TSharedRef<FS47AudioContext> C = MakeShared<FS47AudioContext>(InSampleRate);
	C->Destination = MakeShared<FS47AudioDestinationNode>(C);
	C->Listener = MakeShared<FS47AudioListener>(C);
	FMemory::Memzero(C->Q, sizeof(C->Q));
	return C;
}

TSharedRef<FS47GainNode> FS47AudioContext::CreateGain() { return MakeShared<FS47GainNode>(AsShared()); }
TSharedRef<FS47OscillatorNode> FS47AudioContext::CreateOscillator() { return MakeShared<FS47OscillatorNode>(AsShared()); }
TSharedRef<FS47BufferSourceNode> FS47AudioContext::CreateBufferSource() { return MakeShared<FS47BufferSourceNode>(AsShared()); }
TSharedRef<FS47BiquadFilterNode> FS47AudioContext::CreateBiquadFilter() { return MakeShared<FS47BiquadFilterNode>(AsShared()); }
TSharedRef<FS47PannerNode> FS47AudioContext::CreatePanner() { return MakeShared<FS47PannerNode>(AsShared()); }
TSharedRef<FS47StereoPannerNode> FS47AudioContext::CreateStereoPanner() { return MakeShared<FS47StereoPannerNode>(AsShared()); }
TSharedRef<FS47DelayNode> FS47AudioContext::CreateDelay(double MaxDelayTime) { return MakeShared<FS47DelayNode>(AsShared(), MaxDelayTime); }
TSharedRef<FS47WaveShaperNode> FS47AudioContext::CreateWaveShaper() { return MakeShared<FS47WaveShaperNode>(AsShared()); }
TSharedRef<FS47DynamicsCompressorNode> FS47AudioContext::CreateDynamicsCompressor() { return MakeShared<FS47DynamicsCompressorNode>(AsShared()); }
TSharedRef<FS47AudioBuffer> FS47AudioContext::CreateBuffer(int32 Channels, int32 Length, float Rate) const { return MakeShared<FS47AudioBuffer>(Channels, Length, Rate); }

void FS47AudioContext::Close()
{
	TArray<TSharedPtr<FS47ScheduledSourceNode>> OldActive, OldEnded;
	TSharedPtr<FS47AudioDestinationNode> OldDest;
	TSharedPtr<FS47AudioListener> OldListener;
	{
		FScopeLock L(&Lock);
		bClosed = true;
		OldActive = MoveTemp(Active); OldEnded = MoveTemp(Ended);
		Active.Reset(); Ended.Reset();
		// (the destination and the listener hold the context: let go of them so it can go)
		OldDest = MoveTemp(Destination); OldListener = MoveTemp(Listener);
		Destination.Reset(); Listener.Reset();
		if (OldDest.IsValid()) OldDest->Inputs.Reset();
	}
	Timeouts.Reset();
}

void FS47AudioContext::SetTimeout(double Seconds, TFunction<void()> Fn)
{
	Timeouts.Add({ Clock + FMath::Max(0.0, Seconds), MoveTemp(Fn) });
}

void FS47AudioContext::Tick(double DeltaSeconds)
{
	TArray<TSharedPtr<FS47ScheduledSourceNode>> Over;
	{
		FScopeLock L(&Lock);
		Swap(Over, Ended);
	}
	for (const TSharedPtr<FS47ScheduledSourceNode>& S : Over)
	{
		if (S->OnEnded) { TFunction<void()> Fn = S->OnEnded; Fn(); }
	}
	Over.Reset(); // (the nodes go here, on the game thread)
	Clock += FMath::Max(0.0, DeltaSeconds);
	TArray<FTimeout> Due;
	for (int32 I = 0; I < Timeouts.Num();)
	{
		if (Timeouts[I].At <= Clock) { Due.Add(MoveTemp(Timeouts[I])); Timeouts.RemoveAt(I); }
		else ++I;
	}
	Due.StableSort([](const FTimeout& A, const FTimeout& B) { return A.At < B.At; });
	for (FTimeout& T : Due) if (T.Fn) T.Fn();
}

void FS47AudioContext::RenderQuantum()
{
	FScopeLock L(&Lock);
	if (bSuspended || bClosed || !Destination.IsValid())
	{
		FMemory::Memzero(Q, sizeof(Q));
		return;
	}
	QuantumFrame = Frame.load();
	++Quantum;
	const FS47AudioBlock& B = Destination->Pull(Quantum);
	for (int32 C = 0; C < 2; ++C) FMemory::Memcpy(Q[C], B.D[FMath::Min(C, B.Channels - 1)], sizeof(float) * S47Quantum);
	// the sources that have played out leave the graph; their onended runs on the game thread
	const int64 End = QuantumFrame + S47Quantum;
	for (int32 I = Active.Num() - 1; I >= 0; --I)
	{
		FS47ScheduledSourceNode* S = Active[I].Get();
		if (!S->bEnded && S->StopFrame <= End) S->bEnded = true;
		if (S->bEnded) { Ended.Add(Active[I]); Active.RemoveAtSwap(I); }
	}
	Frame.store(End);
}

void FS47AudioContext::Render(float* Interleaved, int32 Frames, int32 NumChannels)
{
	int32 Done = 0;
	while (Done < Frames)
	{
		if (QPos >= S47Quantum) { RenderQuantum(); QPos = 0; }
		const int32 N = FMath::Min(S47Quantum - QPos, Frames - Done);
		for (int32 J = 0; J < N; ++J)
		{
			float* O = Interleaved + int64(Done + J) * NumChannels;
			const float L = Q[0][QPos + J], R = Q[1][QPos + J];
			if (NumChannels == 1) O[0] = 0.5f * (L + R);
			else
			{
				O[0] = L; O[1] = R;
				for (int32 C = 2; C < NumChannels; ++C) O[C] = 0.f;
			}
		}
		QPos += N;
		Done += N;
	}
}

// ---------- names, files ----------

ES47OscType S47Wa::OscType(const FString& Name)
{
	if (Name == TEXT("square")) return ES47OscType::Square;
	if (Name == TEXT("sawtooth")) return ES47OscType::Sawtooth;
	if (Name == TEXT("triangle")) return ES47OscType::Triangle;
	return ES47OscType::Sine;
}

ES47BiquadType S47Wa::BiquadType(const FString& Name)
{
	if (Name == TEXT("highpass")) return ES47BiquadType::Highpass;
	if (Name == TEXT("bandpass")) return ES47BiquadType::Bandpass;
	if (Name == TEXT("lowshelf")) return ES47BiquadType::Lowshelf;
	if (Name == TEXT("highshelf")) return ES47BiquadType::Highshelf;
	if (Name == TEXT("peaking")) return ES47BiquadType::Peaking;
	if (Name == TEXT("notch")) return ES47BiquadType::Notch;
	if (Name == TEXT("allpass")) return ES47BiquadType::Allpass;
	return ES47BiquadType::Lowpass;
}

TSharedPtr<FS47AudioBuffer> S47Wa::DecodeWav(const TArray<uint8>& Bytes)
{
	auto U16 = [&Bytes](int32 At) { return uint32(Bytes[At]) | (uint32(Bytes[At + 1]) << 8); };
	auto U32 = [&Bytes](int32 At) { return uint32(Bytes[At]) | (uint32(Bytes[At + 1]) << 8) | (uint32(Bytes[At + 2]) << 16) | (uint32(Bytes[At + 3]) << 24); };
	auto Tag = [&Bytes](int32 At, const char* T) { return Bytes[At] == T[0] && Bytes[At + 1] == T[1] && Bytes[At + 2] == T[2] && Bytes[At + 3] == T[3]; };
	if (Bytes.Num() < 12 || !Tag(0, "RIFF") || !Tag(8, "WAVE")) return nullptr;
	int32 Format = 0, Channels = 0, Bits = 0, DataAt = -1, DataSize = 0;
	uint32 Rate = 0;
	for (int32 At = 12; At + 8 <= Bytes.Num();)
	{
		const int32 Size = int32(U32(At + 4));
		if (Size < 0) break;
		if (Tag(At, "fmt ") && At + 8 + 16 <= Bytes.Num())
		{
			Format = int32(U16(At + 8)); Channels = int32(U16(At + 10)); Rate = U32(At + 12); Bits = int32(U16(At + 22));
			if (Format == 0xFFFE && Size >= 26 && At + 8 + 26 <= Bytes.Num()) Format = int32(U16(At + 8 + 24)); // WAVE_FORMAT_EXTENSIBLE
		}
		else if (Tag(At, "data")) { DataAt = At + 8; DataSize = FMath::Min(Size, Bytes.Num() - DataAt); break; }
		At += 8 + Size + (Size & 1);
	}
	if (DataAt < 0 || Channels <= 0 || Rate == 0 || (Format != 1 && Format != 3)) return nullptr;
	const int32 Bps = Bits / 8;
	if (Bps <= 0 || (Format == 3 && Bits != 32)) return nullptr;
	const int32 Frames = DataSize / (Bps * Channels);
	TSharedPtr<FS47AudioBuffer> B = MakeShared<FS47AudioBuffer>(Channels, Frames, float(Rate));
	const uint8* P = Bytes.GetData() + DataAt;
	for (int32 F = 0; F < Frames; ++F)
	{
		for (int32 C = 0; C < Channels; ++C)
		{
			const uint8* S = P + (int64(F) * Channels + C) * Bps;
			float V = 0.f;
			if (Format == 3) { float X; FMemory::Memcpy(&X, S, 4); V = X; }
			else if (Bits == 8) V = (int32(S[0]) - 128) / 128.f;
			else if (Bits == 16) V = int16(uint16(S[0]) | (uint16(S[1]) << 8)) / 32768.f;
			else if (Bits == 24) V = float(int32((uint32(S[0]) << 8) | (uint32(S[1]) << 16) | (uint32(S[2]) << 24)) >> 8) / 8388608.f;
			else if (Bits == 32) V = float(int32(uint32(S[0]) | (uint32(S[1]) << 8) | (uint32(S[2]) << 16) | (uint32(S[3]) << 24)) / 2147483648.0);
			B->GetChannelData(C)[F] = V;
		}
	}
	return B;
}

// ---------- self test ----------

bool S47Wa::SelfTest(TArray<FString>& Out)
{
	bool bAll = true;
	auto Check = [&](bool bOk, const FString& What)
	{
		bAll &= bOk;
		Out.Add(FString::Printf(TEXT("%s %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), *What));
	};
	auto Near = [&](double Got, double Want, double Tol, const FString& What)
	{
		Check(FMath::Abs(Got - Want) <= Tol, FString::Printf(TEXT("%s: got %.12g want %.12g"), *What, Got, Want));
	};

	// the biquad coefficients against the specification's formulas computed apart (48 kHz)
	{
		struct FCase { ES47BiquadType Type; double F, Detune, Q, G; double Want[5]; };
		const FCase Cases[] = {
			{ ES47BiquadType::Lowpass, 3000.0, 0.0, 1.0, 0.0, { 0.0325152883389638, 0.0650305766779276, 0.0325152883389638, -1.57856147662392, 0.708622629979775 } },
			{ ES47BiquadType::Highpass, 320.0, 0.0, 1.0, 0.0, { 0.981250440085909, -1.96250088017182, 0.981250440085909, -1.96163977915435, 0.96336198118929 } },
			{ ES47BiquadType::Lowpass, 3200.0, 0.0, 1.0, 0.0, { 0.036594446900892, 0.0731888938017839, 0.036594446900892, -1.54674074165136, 0.693118529254933 } },
			{ ES47BiquadType::Bandpass, 1800.0, 0.0, 0.6, 0.0, { 0.162856129533914, 0, -0.162856129533914, -1.62802703737302, 0.674287740932173 } },
			{ ES47BiquadType::Bandpass, 2600.0, 0.0, 0.5, 0.0, { 0.250266263756907, 0, -0.250266263756907, -1.4134602541086, 0.499467472486186 } },
			{ ES47BiquadType::Peaking, 1352.0, 0.0, 1.0, 8.0, { 1.0795536002078, -1.86516724405767, 0.815208865345417, -1.86516724405767, 0.894762465553221 } },
			{ ES47BiquadType::Lowpass, 800.0, 0.0, 6.5, 0.0, { 0.00267295315924232, 0.00534590631848463, 0.00267295315924232, -1.94104393462261, 0.951735747259575 } },
			{ ES47BiquadType::Notch, 1000.0, 0.0, 2.0, 0.0, { 0.968399621223586, -1.92022965643694, 0.968399621223586, -1.92022965643694, 0.936799242447173 } },
			{ ES47BiquadType::Allpass, 1000.0, 0.0, 0.7, 0.0, { 0.829436186216818, -1.81378510603597, 1, -1.81378510603597, 0.829436186216818 } },
			{ ES47BiquadType::Lowshelf, 200.0, 0.0, 1.0, 6.0, { 1.00644557785114, -1.96861235232003, 0.963120058272841, -1.96885010738573, 0.969327881058289 } },
			{ ES47BiquadType::Highshelf, 4000.0, 0.0, 1.0, -6.0, { 0.567828271113016, -0.65761955771448, 0.23823122398566, -1.38599185895162, 0.534431796335815 } },
			{ ES47BiquadType::Lowpass, 1000.0, 1200.0, 1.0, 0.0, { 0.0152752904785188, 0.0305505809570376, 0.0152752904785188, -1.73207986816539, 0.793181030079467 } },
		};
		const TCHAR* Names[] = { TEXT("lowpass"), TEXT("highpass"), TEXT("bandpass"), TEXT("lowshelf"), TEXT("highshelf"), TEXT("peaking"), TEXT("notch"), TEXT("allpass") };
		for (const FCase& C : Cases)
		{
			double K[5];
			FS47BiquadFilterNode::Coefficients(C.Type, 48000.0, C.F, C.Detune, C.Q, C.G, K);
			double Err = 0;
			for (int32 I = 0; I < 5; ++I) Err = FMath::Max(Err, FMath::Abs(K[I] - C.Want[I]));
			Check(Err < 1e-12, FString::Printf(TEXT("biquad %s %.0f Hz Q %.2f gain %.0f detune %.0f: coefficients as the formulas (max error %.3g)"), Names[int32(C.Type)], C.F, C.Q, C.G, C.Detune, Err));
		}
	}

	TSharedRef<FS47AudioContext> Ctx = FS47AudioContext::Make(48000.f);
	// (whole quanta, so the next render starts at the context's current time)
	auto RenderSeconds = [&Ctx](double Seconds)
	{
		const int32 N = int32(FMath::CeilToDouble(Seconds * Ctx->SampleRate / S47Quantum)) * S47Quantum;
		TArray<float> Buf; Buf.SetNumZeroed(N * 2);
		Ctx->Render(Buf.GetData(), N, 2);
		return Buf;
	};

	// automation: the values of the timeline at given times
	{
		TSharedRef<FS47GainNode> G = Ctx->CreateGain();
		FS47AudioParam& P = G->Gain;
		P.SetValueAtTime(0, 0.0).LinearRampToValueAtTime(1, 1.0).ExponentialRampToValueAtTime(0.001, 2.0).SetTargetAtTime(0.5, 2.0, 0.3);
		Near(P.ValueAt(0.25), 0.25, 1e-12, TEXT("linear ramp 0 -> 1 at a quarter"));
		Near(P.ValueAt(1.5), 0.0316227766016838, 1e-12, TEXT("exponential ramp 1 -> 0.001 half way"));
		Near(P.ValueAt(2.3), 0.31642815885545, 1e-9, TEXT("setTargetAtTime 0.001 -> 0.5 after one time constant"));
		Near(P.ValueAt(-1.0), 1.0, 1e-12, TEXT("before the first event: the default value"));
		P.CancelScheduledValues(1.5);
		Near(P.ValueAt(1.7), 1.0, 1e-12, TEXT("cancelScheduledValues: the cancelled ramp is gone, the value back where it began"));
		TSharedRef<FS47GainNode> H = Ctx->CreateGain();
		H->Gain.SetValueAtTime(0, 0.0).LinearRampToValueAtTime(1, 1.0);
		H->Gain.CancelAndHoldAtTime(0.4);
		Near(H->Gain.ValueAt(0.2), 0.2, 1e-12, TEXT("cancelAndHoldAtTime: the ramp before the cancel is as it was"));
		Near(H->Gain.ValueAt(0.9), 0.4, 1e-12, TEXT("cancelAndHoldAtTime: the value holds where the ramp had got to"));
		TSharedRef<FS47GainNode> Cv = Ctx->CreateGain();
		Cv->Gain.SetValueCurveAtTime({ 0.f, 1.f, 0.f }, 1.0, 2.0);
		Near(Cv->Gain.ValueAt(1.5), 0.5, 1e-9, TEXT("setValueCurveAtTime: between the points"));
		Near(Cv->Gain.ValueAt(5.0), 0.0, 1e-9, TEXT("setValueCurveAtTime: after the curve its last value"));
		TSharedRef<FS47GainNode> Tg = Ctx->CreateGain();
		Tg->Gain.SetValue(0.0);
		Tg->Gain.SetTargetAtTime(1.0, 0.0, 0.5).LinearRampToValueAtTime(0.0, 2.0);
		Near(Tg->Gain.ValueAt(1.0), 1.0 - FMath::Exp(-2.0), 1e-9, TEXT("a ramp after a target starts from the target's value where it takes over"));
		Near(Tg->Gain.ValueAt(2.0), 0.0, 1e-9, TEXT("... and reaches its own value"));
	}

	// the same timeline rendered frame by frame (a-rate), and an oscillator through a gain
	{
		TSharedRef<FS47OscillatorNode> O = Ctx->CreateOscillator();
		O->Frequency.SetValue(1000);
		TSharedRef<FS47GainNode> G = Ctx->CreateGain();
		G->Gain.SetValue(0);
		const double T0 = Ctx->CurrentTime();
		G->Gain.SetValueAtTime(0, T0).LinearRampToValueAtTime(1.0, T0 + 0.01);
		O->Connect(G); G->Connect(Ctx->Destination);
		O->Start(T0); O->Stop(T0 + 0.02);
		const TArray<float> B = RenderSeconds(0.03);
		double Err = 0;
		for (int32 J = 0; J < 960; ++J)
		{
			const double T = J / 48000.0, Env = FMath::Min(1.0, T / 0.01);
			Err = FMath::Max(Err, FMath::Abs(B[J * 2] - Env * FMath::Sin(2 * Pi * 1000.0 * T)));
		}
		Check(Err < 2e-5, FString::Printf(TEXT("sine 1000 Hz under a linear ramp, rendered: matches sin and the ramp (max error %.3g)"), Err));
		double After = 0;
		for (int32 J = 960; J < 1440; ++J) After = FMath::Max(After, double(FMath::Abs(B[J * 2])));
		Check(After == 0.0, TEXT("the oscillator is silent after stop()"));
		Check(O->Ended(), TEXT("the oscillator has ended"));
		bool bEnded = false;
		O->OnEnded = [&bEnded] { bEnded = true; };
		Ctx->Tick(0.0);
		Check(bEnded, TEXT("onended runs on the game thread's tick"));
		G->Disconnect();
	}

	// a low-pass at its cutoff with Q 1 (dB) passes a sine 1 dB up
	{
		TSharedRef<FS47OscillatorNode> O = Ctx->CreateOscillator();
		O->Frequency.SetValue(3000);
		TSharedRef<FS47BiquadFilterNode> F = Ctx->CreateBiquadFilter();
		F->Type = S47Wa::BiquadType(TEXT("lowpass")); F->Frequency.SetValue(3000);
		O->Connect(F); F->Connect(Ctx->Destination);
		O->Start();
		const TArray<float> B = RenderSeconds(0.3);
		double Sum = 0;
		for (int32 J = B.Num() / 2 - 1600; J < B.Num() / 2; ++J) Sum += double(B[J * 2]) * B[J * 2];
		Near(FMath::Sqrt(Sum / 1600.0) * FMath::Sqrt(2.0), 1.122018454301964, 1e-3, TEXT("lowpass 3000 Hz, a 3000 Hz sine comes through at +1 dB"));
		O->Stop(); F->Disconnect();
		RenderSeconds(0.01);
	}

	// the panner: where a source is heard from, and how much less of it far away
	{
		double L = 0, R = 0;
		const FVector Ear(0, 1.6, 0), Fw(0, 0, -1), Up(0, 1, 0);
		Near(FS47PannerNode::Azimuth(FVector(3, 1.6, 0), Ear, Fw, Up), 90.0, 1e-9, TEXT("azimuth of a source to the right: +90"));
		Near(FS47PannerNode::Azimuth(FVector(-3, 1.6, 0), Ear, Fw, Up), -90.0, 1e-9, TEXT("azimuth of a source to the left: -90"));
		Near(FS47PannerNode::Azimuth(FVector(0, 1.6, -3), Ear, Fw, Up), 0.0, 1e-9, TEXT("azimuth straight ahead: 0"));
		Near(FS47PannerNode::Azimuth(FVector(1, 1.6, -1), Ear, Fw, Up), 45.0, 1e-9, TEXT("azimuth ahead and to the right: 45"));
		FS47PannerNode::EqualPower(45.0, 1, L, R);
		Near(L, 0.38268343236509, 1e-12, TEXT("equal power at 45 degrees, left"));
		Near(R, 0.923879532511287, 1e-12, TEXT("equal power at 45 degrees, right"));
		FS47PannerNode::EqualPower(90.0, 1, L, R);
		Check(FMath::Abs(L) < 1e-12 && FMath::Abs(R - 1.0) < 1e-12, TEXT("equal power at 90 degrees: all right"));
		Near(FS47PannerNode::DistanceGain(ES47DistanceModel::Inverse, 5.0, 1.2, 60.0, 1.2), 0.208333333333333, 1e-12, TEXT("inverse distance, ref 1.2, rolloff 1.2, at 5 m"));
		Near(FS47PannerNode::DistanceGain(ES47DistanceModel::Inverse, 0.5, 1.2, 60.0, 1.2), 1.0, 1e-12, TEXT("inverse distance inside the ref distance: 1"));
		Near(FS47PannerNode::DistanceGain(ES47DistanceModel::Exponential, 4.0, 1.0, 60.0, 1.0), 0.25, 1e-12, TEXT("exponential distance at 4 m"));
		Near(FS47PannerNode::DistanceGain(ES47DistanceModel::Linear, 30.0, 1.0, 60.0, 1.0), 0.508474576271186, 1e-12, TEXT("linear distance at 30 m"));
	}

	// a delay puts an impulse 10 ms later; a delay in a feedback loop runs and dies away
	{
		TSharedRef<FS47AudioBuffer> Imp = Ctx->CreateBuffer(1, 4, 48000.f);
		Imp->GetChannelData(0)[0] = 1.f;
		TSharedRef<FS47BufferSourceNode> S = Ctx->CreateBufferSource();
		S->Buffer = Imp;
		TSharedRef<FS47DelayNode> D = Ctx->CreateDelay(1.0);
		D->DelayTime.SetValue(0.01);
		S->Connect(D); D->Connect(Ctx->Destination);
		const double T0 = Ctx->CurrentTime();
		S->Start(T0);
		const TArray<float> B = RenderSeconds(0.02);
		int32 At = -1;
		for (int32 J = 0; J < B.Num() / 2; ++J) if (FMath::Abs(B[J * 2]) > 0.5f) { At = J; break; }
		Check(At == 480, FString::Printf(TEXT("delay 0.01 s: the impulse comes out 480 frames later (at %d)"), At));
		D->Disconnect();

		TSharedRef<FS47BufferSourceNode> S2 = Ctx->CreateBufferSource();
		S2->Buffer = Imp;
		TSharedRef<FS47DelayNode> D2 = Ctx->CreateDelay(1.0);
		D2->DelayTime.SetValue(0.05);
		TSharedRef<FS47GainNode> Fb = Ctx->CreateGain();
		Fb->Gain.SetValue(0.5);
		S2->Connect(D2); D2->Connect(Fb); Fb->Connect(D2); D2->Connect(Ctx->Destination);
		S2->Start(Ctx->CurrentTime());
		const TArray<float> B2 = RenderSeconds(0.3);
		TArray<int32> Echoes;
		for (int32 J = 0; J < B2.Num() / 2; ++J) if (FMath::Abs(B2[J * 2]) > 0.05f) Echoes.Add(J);
		Check(Echoes.Num() == 5 && Echoes[0] == 2400 && Echoes[1] == 2400 + 2400 + S47Quantum && FMath::IsNearlyEqual(B2[Echoes[1] * 2], 0.5f, 1e-3f),
			FString::Printf(TEXT("a delay in a feedback loop: echoes halving, every 50 ms and one quantum as in Chromium (%d echoes at %d, %d; %.4f, %.4f)"), Echoes.Num(),
				Echoes.Num() > 0 ? Echoes[0] : -1, Echoes.Num() > 1 ? Echoes[1] : -1, Echoes.Num() > 0 ? B2[Echoes[0] * 2] : 0.f, Echoes.Num() > 1 ? B2[Echoes[1] * 2] : 0.f));
		D2->Disconnect(); Fb->Disconnect();
	}

	// a buffer played at twice its rate; a buffer at another sample rate
	{
		TSharedRef<FS47AudioBuffer> Ramp = Ctx->CreateBuffer(1, 1000, 24000.f);
		for (int32 J = 0; J < 1000; ++J) Ramp->GetChannelData(0)[J] = J / 1000.f;
		TSharedRef<FS47BufferSourceNode> S = Ctx->CreateBufferSource();
		S->Buffer = Ramp; S->PlaybackRate.SetValue(2.0);
		S->Connect(Ctx->Destination);
		S->Start(Ctx->CurrentTime());
		const TArray<float> B = RenderSeconds(0.05);
		Near(B[100 * 2], 0.1, 1e-6, TEXT("a 24 kHz buffer at rate 2 in a 48 kHz context: one buffer frame per output frame"));
		int32 Last = 0;
		for (int32 J = 0; J < B.Num() / 2; ++J) if (B[J * 2] != 0.f) Last = J;
		Check(Last == 999, FString::Printf(TEXT("... and it ends after its last frame (%d)"), Last));
	}

	// the wave shaper's curve, the compressor's curve
	{
		const TArray<float> Curve = { -1.f, 0.f, 1.f };
		Near(FS47WaveShaperNode::Shape(Curve, 0.5f), 0.5, 1e-6, TEXT("wave shaper: between curve points"));
		Near(FS47WaveShaperNode::Shape(Curve, 2.f), 1.0, 1e-6, TEXT("wave shaper: past the end, the last point"));
		Near(FS47DynamicsCompressorNode::Curve(DbToLin(-40.0), -16.0, 30.0, 5.0), DbToLin(-40.0), 1e-12, TEXT("compressor: below the threshold, untouched"));
		const double Over = LinToDb(FS47DynamicsCompressorNode::Curve(DbToLin(30.0), -16.0, 30.0, 5.0)) - LinToDb(FS47DynamicsCompressorNode::Curve(DbToLin(20.0), -16.0, 30.0, 5.0));
		Near(Over, 2.0, 1e-6, TEXT("compressor: past the knee, 10 dB in is 2 dB out at ratio 5"));
		Check(FS47DynamicsCompressorNode::Makeup(-16.0, 30.0, 5.0) > 1.0, TEXT("compressor: a make-up gain over 1"));
	}

	// a WAVE file
	{
		TArray<uint8> W;
		auto Put32 = [&W](uint32 V) { for (int32 I = 0; I < 4; ++I) W.Add(uint8(V >> (8 * I))); };
		auto Put16 = [&W](uint32 V) { W.Add(uint8(V)); W.Add(uint8(V >> 8)); };
		auto PutTag = [&W](const char* T) { for (int32 I = 0; I < 4; ++I) W.Add(uint8(T[I])); };
		PutTag("RIFF"); Put32(4 + 8 + 16 + 8 + 4 + 8 + 8); PutTag("WAVE");
		PutTag("fmt "); Put32(16); Put16(1); Put16(2); Put32(22050); Put32(22050 * 4); Put16(4); Put16(16);
		PutTag("LIST"); Put32(4); PutTag("INFO");
		PutTag("data"); Put32(8); Put16(16384); Put16(uint16(-16384)); Put16(32767); Put16(0);
		TSharedPtr<FS47AudioBuffer> B = DecodeWav(W);
		Check(B.IsValid() && B->NumberOfChannels() == 2 && B->Length() == 2 && B->SampleRate() == 22050.f, TEXT("WAVE: two channels, two frames, 22050 Hz"));
		if (B.IsValid()) Near(B->GetChannelData(1)[0], -0.5, 1e-6, TEXT("WAVE: the 16-bit samples as floats"));
	}

	Ctx->Close();
	return bAll;
}
