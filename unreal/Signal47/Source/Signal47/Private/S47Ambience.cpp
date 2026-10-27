// web/src/core/ambience.ts
#include "S47Ambience.h"

namespace
{
	double Rnd(double A, double B) { return A + FMath::FRand() * (B - A); }
	template <typename T> const T& Pick(const TArray<T>& A) { return A[FMath::Min(A.Num() - 1, int32(FMath::FRand() * A.Num()))]; }
	constexpr double Pi = 3.14159265358979323846;
	struct FTimeOfDay { double Hour, Temp, Dawn, Night; };
	FTimeOfDay TimeOfDay(double Clock)
	{
		const double Hour = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0) / 3600;
		// Unreal's extended evening starts before 21:00. The old modulo treated
		// 17:20 as twenty hours into a freezing night, then jumped warm at 21:00.
		// Keep the established 21:00-to-dawn cooling, with a continuous evening.
		const double CoolingHours = Hour < 12 ? Hour + 3 : FMath::Max(Hour - 21, 0.0);
		const double Dawn = FMath::Clamp((Hour - 4.9) / 0.6, 0.0, 1.0) * (Hour < 12 ? 1 : 0);
		const double Evening = FMath::Clamp(Hour - 18, 0.0, 1.0);
		const double Night = Hour < 12 ? 1 - Dawn : Evening * Evening * (3 - 2 * Evening);
		return { Hour, 16 - 1.2 * CoolingHours, Dawn, Night };
	}
}

// ---------- Drift ----------

FS47Ambience::FDrift::FDrift(FS47AudioParam& InParam, double InLo, double InHi, double Every0, double Every1, double Tau0, double Tau1)
	: Param(&InParam), Lo(InLo), Hi(InHi)
{
	Every[0] = Every0; Every[1] = Every1; Tau[0] = Tau0; Tau[1] = Tau1;
	Value = Rnd(Lo, Hi); Param->SetValue(Value);
}

void FS47Ambience::FDrift::Step(double Dt, double Now, double Scale)
{
	Next -= Dt;
	if (Next > 0) return;
	Next = Rnd(Every[0], Every[1]);
	Value = Rnd(Lo, Hi) * Scale;
	Param->SetTargetAtTime(Value, Now, Rnd(Tau[0], Tau[1]));
}

// ---------- Cricket ----------

FS47Ambience::FCricket::FCricket(const TSharedRef<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& Dest, const FVector& At)
	: Ctx(InCtx)
{
	Pulses = 3 + int32(FMath::FRand() * 2); Pace = Rnd(0.85, 1.15); Level = Rnd(0.5, 1);
	Osc = Ctx->CreateOscillator(); Osc->Frequency.SetValue(Rnd(4200, 5200));
	Env = Ctx->CreateGain(); Env->Gain.SetValue(0);
	Vol = Ctx->CreateGain(); Vol->Gain.SetValue(0);
	Pan = Ctx->CreatePanner();
	Pan->PanningModel = TEXT("equalpower"); Pan->DistanceModel = ES47DistanceModel::Inverse; Pan->RefDistance = 6; Pan->RolloffFactor = 1; Pan->MaxDistance = 80;
	const double A = FMath::FRand() * Pi * 2, D = Rnd(5, 22);
	Pos = FVector(At.X + FMath::Cos(A) * D, At.Y, At.Z + FMath::Sin(A) * D);
	Pan->SetPosition(Pos.X, Pos.Y, Pos.Z);
	Osc->Connect(Env); Env->Connect(Vol); Vol->Connect(Pan); Pan->Connect(Dest);
	Osc->Start();
	Next = Ctx->CurrentTime() + Rnd(0.2, 2);
	Vol->Gain.SetTargetAtTime(0.022 * Level, Ctx->CurrentTime(), Rnd(3, 8));
}

void FS47Ambience::FCricket::Schedule(double Rate)
{
	const double Now = Ctx->CurrentTime();
	FS47AudioParam& G = Env->Gain;
	if (Next < Now) Next = Now + 0.05;
	while (!bDying && Next < Now + 0.5)
	{
		for (int32 K = 0; K < Pulses; ++K)
		{
			const double At = Next + K * 0.034;
			G.SetValueAtTime(0, At); G.LinearRampToValueAtTime(1, At + 0.005); G.SetValueAtTime(1, At + 0.015); G.LinearRampToValueAtTime(0, At + 0.022);
		}
		// now and then a pause, as if it had heard something
		Next += FMath::FRand() < 0.04 ? Rnd(2, 6) : (1 / FMath::Max(0.15, Rate * Pace)) * Rnd(0.92, 1.08);
	}
}

void FS47Ambience::FCricket::Fade(double Secs)
{
	bDying = true;
	const double Now = Ctx->CurrentTime();
	Vol->Gain.SetTargetAtTime(0, Now, Secs / 3);
	TSharedPtr<FS47OscillatorNode> O = Osc;
	TSharedPtr<FS47PannerNode> P = Pan;
	TSharedRef<bool> Gone = bGone;
	Ctx->SetTimeout(Secs + 0.5, [O, P, Gone] { O->Stop(); P->Disconnect(); *Gone = true; });
}

// ---------- Ambience ----------

FS47Ambience::FS47Ambience(const TSharedRef<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& Dest, const TMap<FString, TSharedPtr<FS47AudioBuffer>>& InBuf,
	const TSharedPtr<FS47AudioBuffer>& InNoise, const TSharedPtr<FS47AudioBuffer>& InBrown)
	: Ctx(InCtx), Buf(&InBuf), Noise(InNoise), Brown(InBrown)
{
	T = { Rnd(15, 40), Rnd(20, 50), Rnd(60, 180), Rnd(150, 420), Rnd(240, 900), Rnd(120, 400), Rnd(10, 40), Rnd(60, 150), 0 };
	FS47AudioContext::FBatch Batch(*Ctx);
	FS47AudioContext& C = *Ctx;
	Out = C.CreateGain(); Out->Gain.SetValue(0);
	Lp = C.CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(900);
	Quiet = C.CreateGain(); Quiet->Gain.SetValue(1);
	Gate = C.CreateGain(); Gate->Gain.SetValue(1);
	Out->Connect(Quiet); Quiet->Connect(Lp); Lp->Connect(Gate); Gate->Connect(Dest);
	BirdBus = C.CreateGain(); BirdBus->Connect(Out);
	auto Glass = C.CreateBiquadFilter(); Glass->SetType(TEXT("lowpass")); Glass->Frequency.SetValue(3800);
	Win = C.CreateGain(); Win->Gain.SetValue(0);
	BirdBus->Connect(Win); Win->Connect(Glass); Glass->Connect(Gate);
	WindIn = C.CreateGain(); WindIn->Connect(Out);
	BedIn = C.CreateGain(); BedIn->Connect(Out);
	const double Now = C.CurrentTime();
	// the wind as recorded
	if (const TSharedPtr<FS47AudioBuffer>* W = Buf->Find(TEXT("wind")))
	{
		auto Src = C.CreateBufferSource(); Src->Buffer = *W; Src->bLoop = true;
		auto Gain = C.CreateGain(); Src->Connect(Gain); Gain->Connect(WindIn); Src->Start(Now, Rnd(0, (*W)->Duration()));
		FWindRec R; R.Src = Src; R.Gain = Gain; R.Level = MakeUnique<FDrift>(Gain->Gain, 0.14, 0.4, 8, 26, 3, 7); R.Rate = 1; R.RateNext = 0;
		WindRec = MoveTemp(R);
	}
	// a wind made of noise under it, its colour wandering
	auto N = C.CreateBufferSource(); N->Buffer = Brown; N->bLoop = true;
	auto Bp = C.CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Q.SetValue(0.7); Bp->Frequency.SetValue(420);
	auto Wg = C.CreateGain(); N->Connect(Bp); Bp->Connect(Wg); Wg->Connect(WindIn); N->Start(Now, Rnd(0, 2));
	Wind.Gain = Wg; Wind.Bp = Bp; Wind.Level = MakeUnique<FDrift>(Wg->Gain, 0.02, 0.16, 6, 20, 2.5, 6); Wind.Colour = MakeUnique<FDrift>(Bp->Frequency, 260, 720, 5, 16, 2, 5);
	// the crickets as recorded: faint, under the single ones
	if (const TSharedPtr<FS47AudioBuffer>* Cr = Buf->Find(TEXT("crickets")))
	{
		auto Src = C.CreateBufferSource(); Src->Buffer = *Cr; Src->bLoop = true;
		auto Gain = C.CreateGain(); Src->Connect(Gain); Gain->Connect(BedIn); Src->Start(Now, Rnd(0, (*Cr)->Duration()));
		FBugRec R; R.Src = Src; R.Gain = Gain; R.Level = MakeUnique<FDrift>(Gain->Gain, 0.02, 0.07, 10, 30, 3, 8);
		BugRec = MoveTemp(R);
	}
}

void FS47Ambience::Hush(double Secs, double Level, double Tau, double BackTau)
{
	FS47AudioContext::FBatch Batch(*Ctx);
	FS47AudioParam& G = Quiet->Gain;
	const double Now = Ctx->CurrentTime();
	G.CancelScheduledValues(Now); G.SetTargetAtTime(Level, Now, Tau); G.SetTargetAtTime(1, Now + Secs, BackTau);
}

void FS47Ambience::Breath(double Secs)
{
	FS47AudioContext::FBatch Batch(*Ctx);
	const double Now = Ctx->CurrentTime();
	for (const TSharedPtr<FCricket>& C : Crickets) if (!C->bDying) C->Fade(0.4);
	const TPair<FS47GainNode*, double> Gates[2] = { { WindIn.Get(), 0.3 }, { BedIn.Get(), 0.6 } };
	for (const TPair<FS47GainNode*, double>& G : Gates)
	{
		G.Key->Gain.CancelScheduledValues(Now); G.Key->Gain.SetValueAtTime(0, Now);
		G.Key->Gain.SetValueAtTime(0, Now + Secs * G.Value); G.Key->Gain.LinearRampToValueAtTime(1, Now + Secs);
	}
	// when each single cricket starts again: the first after a long beat, the others closer
	const int32 N = FMath::Max(2, int32(FMath::RoundHalfFromZero(FMath::Clamp(1 - Thin, 0.4, 1.0) * 4)));
	FHeld H;
	for (int32 I = 0; I < N; ++I) H.Cues.Add(Secs * (0.15 + 0.68 * FMath::Pow(double(I) / FMath::Max(1, N - 1), 0.85)) + Rnd(-1.2, 1.2));
	H.Len = Secs;
	Held = MoveTemp(H);
	// nothing far off comes in it either
	for (double* K : { &T.Life, &T.Poorwill, &T.Coyote, &T.Truck, &T.Gust }) *K = FMath::Max(*K, Secs + Rnd(5, 25));
}

void FS47Ambience::Still()
{
	FS47AudioContext::FBatch Batch(*Ctx);
	FS47AudioParam& G = Gate->Gain;
	const double Now = Ctx->CurrentTime();
	G.CancelScheduledValues(Now); G.SetValueAtTime(G.Value(), Now); G.SetTargetAtTime(0, Now, 0.015);
	bStilled = true;
}

void FS47Ambience::Back(double Bird)
{
	FS47AudioContext::FBatch Batch(*Ctx);
	FS47AudioParam& G = Gate->Gain;
	const double Now = Ctx->CurrentTime();
	G.CancelScheduledValues(Now); G.SetValueAtTime(G.Value(), Now); G.SetTargetAtTime(1, Now, 1.4);
	bStilled = false;
	T.Birds = Bird;
	for (double* K : { &T.Life, &T.Poorwill, &T.Coyote, &T.Truck, &T.Gust }) *K = FMath::Max(*K, Rnd(20, 50));
}

TOptional<FS47Ambience::FBreathing> FS47Ambience::Breathing() const
{
	if (!Held) return {};
	FBreathing B; B.T = Held->T; B.Len = Held->Len; B.Back = Held->N; B.Of = Held->Cues.Num();
	return B;
}

int32 FS47Ambience::CricketsNow() const
{
	int32 N = 0;
	for (const TSharedPtr<FCricket>& C : Crickets) if (!C->bDying) ++N;
	return N;
}

bool FS47Ambience::SelfTest(TArray<FString>& Lines)
{
	bool bAll = true;
	auto Check = [&](bool bPass, const TCHAR* Why)
	{
		Lines.Add(FString::Printf(TEXT("%s ambience %s"), bPass ? TEXT("OK") : TEXT("FAIL"), Why));
		bAll &= bPass;
	};
	const FTimeOfDay Afternoon = TimeOfDay((17 + 20.0 / 60) * 3600);
	Check(Afternoon.Temp > 6.3 && Afternoon.Night == 0, TEXT("17:20 is a warm afternoon without nocturnal insects"));
	const FTimeOfDay Dusk = TimeOfDay(18.5 * 3600);
	Check(Dusk.Night > 0 && Dusk.Night < 1, TEXT("insects fade in during dusk instead of waiting for 21:00"));
	Check(TimeOfDay(19 * 3600).Night == 1, TEXT("night ambience is available after dusk"));
	const FTimeOfDay Before = TimeOfDay(21 * 3600 - .01), After = TimeOfDay(21 * 3600 + .01);
	Check(FMath::Abs(Before.Temp - After.Temp) < .001 && Before.Night == After.Night,
		TEXT("temperature and nocturnal envelope stay continuous across 21:00"));
	bool bNightUnchanged = true;
	for (double Hour = 21; Hour <= 29.5; Hour += .25)
	{
		const FTimeOfDay P = TimeOfDay(Hour * 3600);
		const double ExpectedDawn = Hour < 24 ? 0 : FMath::Clamp((Hour - 24 - 4.9) / .6, 0.0, 1.0);
		bNightUnchanged &= FMath::IsNearlyEqual(P.Temp, 16 - 1.2 * (Hour - 21), 1e-9) &&
			FMath::IsNearlyEqual(P.Dawn, ExpectedDawn, 1e-9) && FMath::IsNearlyEqual(P.Night, 1 - ExpectedDawn, 1e-9);
	}
	Check(bNightUnchanged, TEXT("established full-night cooling and dawn envelope are preserved"));
	const FTimeOfDay Yesterday = TimeOfDay(-2 * 3600), Today = TimeOfDay(22 * 3600), Tomorrow = TimeOfDay(46 * 3600);
	Check(Yesterday.Temp == Today.Temp && Today.Temp == Tomorrow.Temp && Yesterday.Night == Today.Night && Today.Night == Tomorrow.Night,
		TEXT("negative and next-day restored clocks use the same daily phase"));
	Check(FMath::IsNearlyZero(TimeOfDay(5.5 * 3600).Night, 1e-9) && TimeOfDay(12 * 3600).Night == 0,
		TEXT("the nocturnal envelope is silent after dawn and at noon"));
	return bAll;
}

void FS47Ambience::SetSpace(const FString& InSpace)
{
	FS47AudioContext::FBatch Batch(*Ctx);
	Space = InSpace;
	const double Now = Ctx->CurrentTime();
	// indoors the desert is barely there (Tom, 16 October): about -26 dB and only what comes
	// through a wall, the low part of the wind; the crickets are gone. The cab and the photo
	// lab are closed too.
	const double Level = Space == TEXT("yard") ? 1 : Space == TEXT("room") ? 0.05 : 0.04;
	const double Cut = Space == TEXT("yard") ? 9000 : Space == TEXT("room") ? 420 : 380;
	Out->Gain.SetTargetAtTime(Level, Now, 0.8);
	Lp->Frequency.SetTargetAtTime(Cut, Now, 0.5);
}

void FS47Ambience::Update(double Dt, const FVector& At, double Clock)
{
	FS47AudioContext::FBatch Batch(*Ctx);
	const double Now = Ctx->CurrentTime();
	const double Wk = Space == TEXT("room") ? FMath::Clamp(Window, 0.0, 1.0) : 0;
	if (Wk != WinK)
	{
		// (in through the door at dawn: the first of them soon)
		if (Wk > 0 && WinK <= 0) T.Birds = FMath::Min(T.Birds, Rnd(2, 6));
		WinK = Wk; Win->Gain.SetTargetAtTime(0.2 * Wk, Now, 0.6);
	}
	const FTimeOfDay Day = TimeOfDay(Clock);
	const double Hour = Day.Hour, Temp = Day.Temp, Dawn = Day.Dawn;
	// ---------- how much is going on ----------
	if ((T.Mood -= Dt) <= 0)
	{
		T.Mood = Rnd(60, 180);
		Mood = FMath::FRand() < 0.15 ? 0.08 : Rnd(0.35, 1);          // now and then the night goes quiet
	}
	const double M = Mood;
	// ---------- wind ----------
	if (WindRec)
	{
		WindRec->Level->Step(Dt, Now, 0.4 + 0.6 * M);
		if ((WindRec->RateNext -= Dt) <= 0)
		{
			WindRec->RateNext = Rnd(10, 30);
			WindRec->Src->PlaybackRate.SetTargetAtTime(Rnd(0.88, 1.06), Now, 6);
		}
	}
	Wind.Level->Step(Dt, Now, 0.3 + 0.7 * M);
	Wind.Colour->Step(Dt, Now);
	if ((T.Gust -= Dt) <= 0) { T.Gust = Rnd(14, 55) / (0.4 + M); Gust(); }
	// ---------- crickets: fewer and slower as it cools, gone by dawn ----------
	const double Active = FMath::Clamp((Temp - 6.3) / 5, 0.0, 1.0) * Day.Night * (1 - FMath::Clamp(Thin, 0.0, 1.0));
	const double Rate = FMath::Max(0.2, (Temp - 4) / 7.5);
	if (BugRec) BugRec->Level->Step(Dt, Now, Active * (0.5 + 0.5 * M));
	if (Held)
	{
		// the breath: one cricket at each cue, close by; the night's own count takes over at the end
		Held->T += Dt;
		while (Held->N < Held->Cues.Num() && Held->T >= Held->Cues[Held->N])
		{
			TSharedPtr<FCricket> C = MakeShared<FCricket>(Ctx, Out, At);
			C->Vol->Gain.CancelScheduledValues(Now); C->Vol->Gain.SetTargetAtTime(0.022 * C->Level, Now, 0.6);
			Crickets.Add(C); Held->N++;
		}
		if (Held->T >= Held->Len) { Held.Reset(); T.Bugs = Rnd(6, 12); }
	}
	else if ((T.Bugs -= Dt) <= 0)
	{
		T.Bugs = Rnd(4, 12);
		const int32 Want = int32(FMath::RoundHalfFromZero(Active * (1 + 4 * M)));
		TArray<TSharedPtr<FCricket>> Live = Crickets.FilterByPredicate([](const TSharedPtr<FCricket>& C) { return !C->bDying; });
		for (const TSharedPtr<FCricket>& C : Live) if (FVector::Dist(C->Pos, At) > 32) C->Fade(4);  // left behind
		TArray<TSharedPtr<FCricket>> Near = Live.FilterByPredicate([&At](const TSharedPtr<FCricket>& C) { return FVector::Dist(C->Pos, At) <= 32; });
		if (Near.Num() < Want) Crickets.Add(MakeShared<FCricket>(Ctx, Out, At));
		else if (Near.Num() > Want && Near.Num()) Pick(Near)->Fade(Rnd(4, 12));
		Crickets.RemoveAll([](const TSharedPtr<FCricket>& C) { return *C->bGone; });
	}
	for (const TSharedPtr<FCricket>& C : Crickets) C->Schedule(Rate);
	// ---------- far off ----------
	if (Space == TEXT("lab") || bStilled) return;                     // the engine covers it; held still, nothing
	const double Busy = (0.3 + M) * (1 - 0.7 * FMath::Clamp(Thin, 0.0, 1.0));
	if ((T.Life -= Dt) <= 0)
	{
		T.Life = Rnd(25, 70) / Busy;
		const double R = FMath::FRand();
		const FString Name = R < 0.35 ? TEXT("owl") : R < 0.6 ? Pick(TArray<FString>{ TEXT("dog0"), TEXT("dog1") }) : TEXT("gust");
		if (Name != TEXT("owl") || Day.Night >= 0.4) Far(Name, At, Name == TEXT("gust") ? 0.3 : Name == TEXT("owl") ? 0.26 : 0.18, Name == TEXT("owl") ? Rnd(0.9, 1.05) : Rnd(0.95, 1.05));
	}
	if ((T.Poorwill -= Dt) <= 0)
	{
		// they call most in the grey before dawn
		const double Keen = Hour >= 3.5 && Hour < 5.3 ? 1 : 0.35;
		T.Poorwill = Rnd(90, 260) / (Keen * Busy);
		if (Temp > 5 && Day.Night > 0.1) Poorwill(At);
	}
	if ((T.Coyote -= Dt) <= 0)
	{
		T.Coyote = Rnd(240, 600) / Busy;
		if (M > 0.2) Coyotes(At);
	}
	if ((T.Train -= Dt) <= 0) { T.Train = Rnd(700, 1500); Train(At); }
	if ((T.Truck -= Dt) <= 0) { T.Truck = Rnd(200, 520) / Busy; Truck(); }
	if (Dawn > 0 && (T.Birds -= Dt) <= 0)
	{
		// (through the diner's glass the dawn is more of them: it is all there is to hear of outside)
		T.Birds = Rnd(12, 40) / (0.3 + Dawn) / (1 + 4 * Wk);
		Birds++;
		if (FMath::FRand() < 0.55) Meadowlark(At); else Lark(At);
	}
}

// ---------- helpers ----------

TSharedRef<FS47GainNode> FS47Ambience::Distant(const FVector& At, double D, double Gain, double Cut, TOptional<double> Bearing, TSharedPtr<FS47AudioNode> Dest)
{
	// A panner some way off in a random direction (or a given bearing), heard at about `gain`.
	FS47AudioContext& C = *Ctx;
	const double A = Bearing ? *Bearing : FMath::FRand() * Pi * 2;
	auto P = C.CreatePanner();
	P->PanningModel = TEXT("equalpower"); P->DistanceModel = ES47DistanceModel::Inverse; P->RefDistance = D; P->RolloffFactor = 1; P->MaxDistance = D * 4;
	const double X = At.X + FMath::Cos(A) * D, Y = At.Y + 2, Z = At.Z + FMath::Sin(A) * D;
	P->SetPosition(X, Y, Z);
	auto L = C.CreateBiquadFilter(); L->SetType(TEXT("lowpass")); L->Frequency.SetValue(Cut);
	auto G = C.CreateGain(); G->Gain.SetValue(Gain);
	G->Connect(L); L->Connect(P); P->Connect(Dest.IsValid() ? Dest : TSharedPtr<FS47AudioNode>(Out));
	Ctx->SetTimeout(40, [P] { P->Disconnect(); });
	return G;
}

TSharedRef<FS47GainNode> FS47Ambience::Echo(const TSharedPtr<FS47AudioNode>& Into, double Delay, double FbGain)
{
	// A short echo off the mesas, for things a long way away.
	FS47AudioContext& C = *Ctx;
	auto D = C.CreateDelay(1); D->DelayTime.SetValue(Delay);
	auto F = C.CreateGain(); F->Gain.SetValue(FbGain);
	auto L = C.CreateBiquadFilter(); L->SetType(TEXT("lowpass")); L->Frequency.SetValue(900);
	auto Inp = C.CreateGain();
	Inp->Connect(Into); Inp->Connect(D); D->Connect(L); L->Connect(F); F->Connect(D); L->Connect(Into);
	Ctx->SetTimeout(30, [Inp, D] { Inp->Disconnect(); D->Disconnect(); });
	return Inp;
}

void FS47Ambience::Far(const FString& Name, const FVector& At, double Gain, double Rate)
{
	const TSharedPtr<FS47AudioBuffer>* B = Buf->Find(Name);
	if (!B || !B->IsValid()) return;
	FS47AudioContext::FBatch Batch(*Ctx);
	const double D = Rnd(30, 60);
	auto G = Distant(At, D, Gain, 2600 - D * 20);
	auto Src = Ctx->CreateBufferSource(); Src->Buffer = *B; Src->PlaybackRate.SetValue(Rate);
	Src->Connect(G); Src->Start();
}

void FS47Ambience::Note(const TSharedPtr<FS47AudioNode>& Dest, double At, double Len, double F0, double F1, double Peak, double Harm)
{
	// A whistled note: a sine (and a little of its octave) gliding from f0 to f1.
	FS47AudioContext& C = *Ctx;
	auto O = C.CreateOscillator(); O->Frequency.SetValueAtTime(F0, At); O->Frequency.ExponentialRampToValueAtTime(F1, At + Len);
	auto O2 = C.CreateOscillator(); O2->Frequency.SetValueAtTime(F0 * 2, At); O2->Frequency.ExponentialRampToValueAtTime(F1 * 2, At + Len);
	auto H = C.CreateGain(); H->Gain.SetValue(Harm);
	auto G = C.CreateGain(); G->Gain.SetValueAtTime(0, At); G->Gain.LinearRampToValueAtTime(Peak, At + FMath::Min(0.03, Len * 0.3));
	G->Gain.SetValueAtTime(Peak, At + Len * 0.7); G->Gain.LinearRampToValueAtTime(0, At + Len);
	O->Connect(G); O2->Connect(H); H->Connect(G); G->Connect(Dest);
	O->Start(At); O2->Start(At); O->Stop(At + Len + 0.02); O2->Stop(At + Len + 0.02);
}

// ---------- the wind gusts ----------

void FS47Ambience::Gust()
{
	const double Now = Ctx->CurrentTime();
	FWind& W = Wind;
	const double Up = Rnd(1.2, 3), Hold = Rnd(0.5, 2.5), Down = Rnd(3, 7), Peak = Rnd(0.12, 0.3);
	FS47AudioParam& G = W.Gain->Gain;
	G.CancelScheduledValues(Now); G.SetValueAtTime(G.Value(), Now);
	G.LinearRampToValueAtTime(Peak, Now + Up); G.SetValueAtTime(Peak, Now + Up + Hold);
	G.SetTargetAtTime(W.Level->Value, Now + Up + Hold, Down / 3);
	FS47AudioParam& F = W.Bp->Frequency;
	F.CancelScheduledValues(Now); F.SetValueAtTime(F.Value(), Now);
	F.LinearRampToValueAtTime(Rnd(800, 1300), Now + Up);
	F.SetTargetAtTime(W.Colour->Value, Now + Up + Hold, Down / 3);
	W.Level->Next = Up + Hold + Down; W.Colour->Next = Up + Hold + Down;
}

// ---------- made here ----------

void FS47Ambience::Poorwill(const FVector& At)
{
	// The common poorwill: "poor-will", a soft low note and a rising one, three to eight times.
	auto G = Distant(At, Rnd(40, 110), 1, 3200);
	const double T0 = Ctx->CurrentTime() + 0.1;
	const int32 N = 3 + int32(FMath::FRand() * 6);
	const double F = Rnd(0.95, 1.06);
	for (int32 I = 0; I < N; ++I)
	{
		const double At2 = T0 + I * Rnd(1.05, 1.3);
		Note(G, At2, 0.11, 1900 * F, 1820 * F, 0.03);
		Note(G, At2 + 0.17, 0.2, 1980 * F, 2650 * F, 0.04);
	}
}

void FS47Ambience::Coyotes(const FVector& At)
{
	// Coyotes a long way off: a few voices, yips and a howl or two, with the mesas answering.
	FS47AudioContext& C = *Ctx;
	const double D = Rnd(250, 700);
	auto Out2 = Echo(Distant(At, D, 1, 1700), Rnd(0.18, 0.3), 0.32);
	const double T0 = C.CurrentTime() + 0.2;
	const int32 Voices = 2 + int32(FMath::FRand() * 3);
	for (int32 V = 0; V < Voices; ++V)
	{
		auto O = C.CreateOscillator(); O->SetType(TEXT("sawtooth"));
		auto Bp = C.CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(Rnd(900, 1300)); Bp->Q.SetValue(1.4);
		auto G = C.CreateGain(); G->Gain.SetValue(0);
		O->Connect(Bp); Bp->Connect(G); G->Connect(Out2);
		auto Vib = C.CreateOscillator(); Vib->Frequency.SetValue(Rnd(5, 6.5)); auto Vg = C.CreateGain(); Vg->Gain.SetValue(0);
		Vib->Connect(Vg); Vg->Connect(O->Frequency); Vib->Start(T0); O->Start(T0);
		double Tt = T0 + Rnd(0, 1.2);
		const double End = T0 + Rnd(4, 7);
		const double K = Rnd(0.9, 1.15);
		while (Tt < End)
		{
			if (FMath::FRand() < 0.3)
			{
				// a howl
				const double Len = Rnd(0.9, 1.7);
				O->Frequency.SetValueAtTime(430 * K, Tt); O->Frequency.ExponentialRampToValueAtTime(880 * K, Tt + 0.3);
				O->Frequency.SetValueAtTime(880 * K, Tt + Len - 0.45); O->Frequency.ExponentialRampToValueAtTime(520 * K, Tt + Len);
				Vg->Gain.SetValueAtTime(0, Tt + 0.3); Vg->Gain.LinearRampToValueAtTime(22, Tt + 0.5); Vg->Gain.SetValueAtTime(22, Tt + Len - 0.4); Vg->Gain.LinearRampToValueAtTime(0, Tt + Len);
				G->Gain.SetValueAtTime(0, Tt); G->Gain.LinearRampToValueAtTime(0.022, Tt + 0.25); G->Gain.SetValueAtTime(0.022, Tt + Len - 0.3); G->Gain.LinearRampToValueAtTime(0, Tt + Len);
				Tt += Len + Rnd(0.1, 0.5);
			}
			else
			{
				// a run of yips
				for (int32 Y = 0, N = 2 + int32(FMath::FRand() * 4); Y < N; ++Y)
				{
					const double Len = Rnd(0.06, 0.11);
					O->Frequency.SetValueAtTime(700 * K, Tt); O->Frequency.ExponentialRampToValueAtTime(1250 * K, Tt + Len * 0.5); O->Frequency.ExponentialRampToValueAtTime(820 * K, Tt + Len);
					G->Gain.SetValueAtTime(0, Tt); G->Gain.LinearRampToValueAtTime(0.02, Tt + 0.012); G->Gain.LinearRampToValueAtTime(0, Tt + Len);
					Tt += Len + Rnd(0.05, 0.16);
				}
				Tt += Rnd(0.1, 0.4);
			}
		}
		O->Stop(End + 2); Vib->Stop(End + 2);
	}
}

void FS47Ambience::Train(const FVector& At)
{
	// A freight train's horn on the line to Roswell, miles off: long, long, short, long.
	FS47AudioContext& C = *Ctx;
	auto Out2 = Echo(Distant(At, 1500, 1, 700, Rnd(0, Pi * 2)), 0.32, 0.45);
	double Tt = C.CurrentTime() + 0.3;
	for (const double Len : { 1.8, 1.8, 0.7, 2.8 })
	{
		auto G = C.CreateGain(); G->Gain.SetValueAtTime(0, Tt); G->Gain.LinearRampToValueAtTime(0.012, Tt + 0.18); G->Gain.SetValueAtTime(0.012, Tt + Len - 0.3); G->Gain.LinearRampToValueAtTime(0, Tt + Len);
		G->Connect(Out2);
		for (const double F : { 311.0, 370.0, 494.0 })
		{
			auto O = C.CreateOscillator(); O->SetType(TEXT("sawtooth")); O->Frequency.SetValue(F * Rnd(0.995, 1.005));
			O->Connect(G); O->Start(Tt); O->Stop(Tt + Len + 0.05);
		}
		Tt += Len + (Len < 1 ? 0.35 : 0.55);
	}
	// and the rumble of the train itself, rising and going
	auto N = C.CreateBufferSource(); N->Buffer = Brown; N->bLoop = true;
	auto L = C.CreateBiquadFilter(); L->SetType(TEXT("lowpass")); L->Frequency.SetValue(110);
	auto G = C.CreateGain();
	const double T0 = C.CurrentTime();
	G->Gain.SetValueAtTime(0, T0); G->Gain.LinearRampToValueAtTime(0.05, T0 + 8); G->Gain.LinearRampToValueAtTime(0, T0 + 24);
	N->Connect(L); L->Connect(G); G->Connect(Out2); N->Start(T0); N->Stop(T0 + 25);
}

void FS47Ambience::Truck()
{
	// A truck on the highway: tyres and a diesel coming up out of nothing and going away.
	FS47AudioContext& C = *Ctx;
	const double T0 = C.CurrentTime(), Len = Rnd(14, 22), Mid = T0 + Len * 0.5;
	auto Pan = C.CreateStereoPanner();
	const double Dir = FMath::FRand() < 0.5 ? -1 : 1;
	Pan->Pan.SetValueAtTime(-0.9 * Dir, T0); Pan->Pan.LinearRampToValueAtTime(0.9 * Dir, T0 + Len);
	auto L = C.CreateBiquadFilter(); L->SetType(TEXT("lowpass")); L->Frequency.SetValue(900);
	Pan->Connect(L); L->Connect(Out);
	auto Env = [&](const TSharedRef<FS47GainNode>& G, double Peak) { G->Gain.SetValueAtTime(0, T0); G->Gain.LinearRampToValueAtTime(Peak, Mid); G->Gain.LinearRampToValueAtTime(0, T0 + Len); G->Connect(Pan); };
	auto N = C.CreateBufferSource(); N->Buffer = Noise; N->bLoop = true;
	auto Bp = C.CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(700); Bp->Q.SetValue(0.6);
	auto Ng = C.CreateGain(); N->Connect(Bp); Bp->Connect(Ng); Env(Ng, 0.018);
	auto O = C.CreateOscillator(); O->SetType(TEXT("sawtooth"));
	O->Frequency.SetValueAtTime(78, T0); O->Frequency.SetValueAtTime(78, Mid - 1); O->Frequency.LinearRampToValueAtTime(66, Mid + 1);  // the Doppler drop
	auto Ol = C.CreateBiquadFilter(); Ol->SetType(TEXT("lowpass")); Ol->Frequency.SetValue(240);
	auto Og = C.CreateGain(); O->Connect(Ol); Ol->Connect(Og); Env(Og, 0.025);
	N->Start(T0); O->Start(T0); N->Stop(T0 + Len + 0.1); O->Stop(T0 + Len + 0.1);
	Ctx->SetTimeout(Len + 1, [Pan] { Pan->Disconnect(); });
}

void FS47Ambience::Meadowlark(const FVector& At)
{
	// A meadowlark when the sky greys: a few clear slurred whistles, then a quick jumble.
	auto G = Distant(At, Rnd(40, 90), 1, 5000, {}, BirdBus);
	double Tt = Ctx->CurrentTime() + 0.1;
	const double F = Rnd(0.93, 1.07);
	const double Parts[3][3] = { { 0.28, 2650, 2400 }, { 0.22, 2950, 2700 }, { 0.12, 2150, 2350 } };
	for (const auto& P : Parts) { Note(G, Tt, P[0], P[1] * F, P[2] * F, 0.022, 0.2); Tt += P[0] + 0.04; }
	for (int32 I = 0, N = 4 + int32(FMath::FRand() * 4); I < N; ++I) { const double A = Rnd(1900, 3300) * F; Note(G, Tt, 0.06, A, A * Rnd(0.85, 1.15), 0.016, 0.25); Tt += 0.075; }
}

void FS47Ambience::Lark(const FVector& At)
{
	// Horned larks: high, quick, tinkling notes from the open ground.
	auto G = Distant(At, Rnd(30, 70), 1, 7000, {}, BirdBus);
	double Tt = Ctx->CurrentTime() + 0.1;
	for (int32 I = 0, N = 10 + int32(FMath::FRand() * 12); I < N; ++I) { const double A = Rnd(3600, 6000); Note(G, Tt, Rnd(0.03, 0.05), A, A * Rnd(0.9, 1.12), 0.009, 0.05); Tt += Rnd(0.05, 0.13); }
}
