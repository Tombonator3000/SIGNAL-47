#include "S47RxSearch.h"
#include "S47Audio.h"
#include "S47WebAudio.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include <limits>

// RX-SEARCH-SOUND-20261113: the rule (pure) and the seven loops' voices. See S47RxSearch.h; the loops are made and measured by
// Tools/rxacquire/make_search.py.

void S47RxSearch::Weights(double Closeness, double (&Out)[Bands])
{
	for (double& W : Out) W = 0;
	const double X = (FMath::IsFinite(Closeness) ? FMath::Clamp(Closeness, 0., 1.) : 0.) * (Bands - 1);
	const int32 Lo = FMath::Min(FMath::FloorToInt32(X), Bands - 2);
	const double F = X - Lo;
	Out[Lo] = FMath::Cos(F * HALF_PI);
	Out[Lo + 1] = FMath::Sin(F * HALF_PI);
}

double S47RxSearch::Smooth(double Held, double Target, double Dt, bool bCalm)
{
	const double From = FMath::IsFinite(Held) ? FMath::Clamp(Held, 0., 1.) : 0.;
	if (!FMath::IsFinite(Target) || !FMath::IsFinite(Dt) || Dt <= 0) return From;
	const double To = FMath::Clamp(Target, 0., 1.);
	const double Tau = (To > From ? TauUp : TauDown) * (bCalm ? 1.5 : 1.);
	const double Step = (To - From) * (1. - FMath::Exp(-Dt / Tau));
	const double Limit = MaxSlew * Dt * (bCalm ? 1. / 1.5 : 1.);
	return From + FMath::Clamp(Step, -Limit, Limit);
}

double S47RxSearch::TuneSettle(double TunedFor)
{
	if (!FMath::IsFinite(TunedFor) || TunedFor < Hold) return 1.;
	return Settled + (1. - Settled) * FMath::Exp(-(TunedFor - Hold) / SettleTau);
}

double S47RxSearch::Level(EUse Use, bool bLocked, double TunedFor, bool bDuck, bool bCalm)
{
	double L = 0;
	if (Use == EUse::Acquire) L = bLocked ? 0. : LevelAcquire;
	else if (Use == EUse::Tune) L = LevelTune * (bLocked ? TuneSettle(TunedFor) : 1.);
	return L * (bDuck ? Duck : 1.) * (bCalm ? Calm : 1.);
}

bool FS47RxSearch::Load(US47Audio* Audio)
{
	if (bTried) return bLoaded;
	if (!Audio || !Audio->Ctx.IsValid()) return false;
	bTried = true;
	// Staged by Tools/rxacquire/make_search.py (git-ignored; Audio/Raw is staged with the build: DefaultGame.ini). All seven or none.
	const FString Folder = FPaths::ProjectContentDir() / TEXT("Audio/Raw/media/rxsearch");
	int32 Got = 0;
	for (int32 K = 0; K < S47RxSearch::Bands; ++K)
	{
		TArray<uint8> Bytes;
		if (FFileHelper::LoadFileToArray(Bytes, *(Folder / FString::Printf(TEXT("search_%d.wav"), K)), FILEREAD_Silent)) Loops[K] = S47Wa::DecodeWav(Bytes);
		if (Loops[K].IsValid() && Loops[K]->Length() > 0) ++Got;
	}
	bLoaded = Got == S47RxSearch::Bands;
	if (!bLoaded) for (TSharedPtr<FS47AudioBuffer>& L : Loops) L.Reset();
	UE_LOG(LogTemp, Display, TEXT("S47: receiver search layer: %s (%d of %d loops in Content/Audio/Raw/media/rxsearch)"), bLoaded ? TEXT("on") : TEXT("left out"), Got, int32(S47RxSearch::Bands));
	return bLoaded;
}

int32 FS47RxSearch::Voices() const
{
	int32 N = 0;
	for (const FVoice& V : Voice) N += V.Src.IsValid() ? 1 : 0;
	return N;
}

void FS47RxSearch::Frame(US47Audio* Audio, const FIn& In, double Dt)
{
	using namespace S47RxSearch;
	if (!Audio || !Audio->Ctx.IsValid() || !Audio->SfxBus.IsValid()) return;
	const bool bOn = In.Use != EUse::Off;
	if (bOn && !bTried) Load(Audio);
	if (!bLoaded) return; // no loops: nothing was started, nothing to do
	const double Step = FMath::IsFinite(Dt) ? FMath::Clamp(Dt, 0., .25) : 0.;
	const double Near = FMath::IsFinite(In.Closeness) ? FMath::Clamp(In.Closeness, 0., 1.) : HeldNear;
	// A fresh start begins where the search is (fading in there), not at the far end
	if (bOn && Voices() == 0) HeldNear = Near;
	else if (bOn) HeldNear = Smooth(HeldNear, Near, Step, In.bCalm);
	TunedFor = In.Use == EUse::Tune && In.bLocked ? TunedFor + Step : 0.;
	const double L = Level(In.Use, In.bLocked, TunedFor, In.bDuck, In.bCalm);
	double W[Bands];
	Weights(HeldNear, W);
	const double Tau = !bOn ? FadeTau : In.Use == EUse::Acquire && In.bLocked ? LockFadeTau : GainTau;
	FS47AudioContext::FBatch Batch(*Audio->Ctx);
	const double T = Audio->Ctx->CurrentTime();
	for (int32 K = 0; K < Bands; ++K)
	{
		Targets[K] = L * W[K];
		FVoice& V = Voice[K];
		if (Targets[K] > 1e-4 && !V.Src.IsValid() && Loops[K].IsValid())
		{
			V.Src = Audio->Ctx->CreateBufferSource();
			V.Src->Buffer = Loops[K];
			V.Src->bLoop = true;
			V.Gain = Audio->Ctx->CreateGain();
			V.Gain->Gain.SetValue(0);
			V.Src->Connect(V.Gain);
			V.Gain->Connect(Audio->SfxBus);
			V.Src->Start(T);
			V.QuietFor = 0;
		}
		if (!V.Src.IsValid()) continue;
		V.Gain->Gain.SetTargetAtTime(Targets[K], T, Tau);
		V.QuietFor = Targets[K] > 1e-4 ? 0. : V.QuietFor + Step;
		// let a voice go once its fade has run out (seven time constants: under -60 dB)
		if (V.QuietFor > 7. * Tau + .1)
		{
			V.Gain->Gain.CancelAndHoldAtTime(T);
			V.Gain->Gain.LinearRampToValueAtTime(0, T + .03);
			V.Src->Stop(T + .05);
			V = FVoice();
		}
	}
}

void FS47RxSearch::Off(US47Audio* Audio)
{
	for (double& X : Targets) X = 0;
	TunedFor = 0;
	if (!Audio || !Audio->Ctx.IsValid() || Voices() == 0) { for (FVoice& V : Voice) V = FVoice(); return; }
	FS47AudioContext::FBatch Batch(*Audio->Ctx);
	const double T = Audio->Ctx->CurrentTime();
	for (FVoice& V : Voice)
	{
		if (!V.Src.IsValid()) continue;
		V.Gain->Gain.CancelAndHoldAtTime(T);
		V.Gain->Gain.LinearRampToValueAtTime(0, T + .06);
		V.Src->Stop(T + .07);
		V = FVoice();
	}
}

void FS47RxSearch::UseLoops(const TArray<TSharedPtr<FS47AudioBuffer>>& InLoops)
{
	bTried = true;
	bLoaded = InLoops.Num() == S47RxSearch::Bands;
	for (int32 K = 0; K < S47RxSearch::Bands; ++K) { if (bLoaded) Loops[K] = InLoops[K]; else Loops[K].Reset(); }
}

bool FS47RxSearch::SelfTest(US47Audio* A, TArray<FString>& Lines)
{
	using namespace S47RxSearch;
	bool Good = true;
	auto Check = [&](bool Ok, const TCHAR* Name) { Good &= Ok; Lines.Add(FString::Printf(TEXT("%s RX-SEARCH-SOUND-20261113: %s"), Ok ? TEXT("PASS") : TEXT("FAIL"), Name)); };
	// the mapping: far is the air, near the body, between two neighbours at equal power
	const double Nan = std::numeric_limits<double>::quiet_NaN();
	double W[Bands];
	bool Ends = true, Power = true, Pair = true, Rising = true;
	Weights(0., W); Ends &= W[0] > .999;
	Weights(1., W); Ends &= W[Bands - 1] > .999;
	Weights(-3., W); Ends &= W[0] > .999;
	Weights(TNumericLimits<double>::Max(), W); Ends &= W[Bands - 1] > .999;
	Weights(Nan, W); Ends &= W[0] > .999;
	double Centre = 0, Before = -1;
	for (int32 I = 0; I <= 120; ++I)
	{
		Weights(I / 120., W);
		double Sq = 0, Mean = 0; int32 Live = 0;
		for (int32 K = 0; K < Bands; ++K) { Sq += W[K] * W[K]; Mean += K * W[K] * W[K]; Live += W[K] > 1e-9 ? 1 : 0; }
		Power &= FMath::Abs(Sq - 1.) < 1e-9; Pair &= Live <= 2;
		Centre = Mean; Rising &= Centre >= Before - 1e-9; Before = Centre;
	}
	Check(Ends && Power && Pair && Rising, TEXT("closeness to band: far plays the quiet start, near the strong body, at most two neighbours at equal power, rising with closeness"));
	// smoothing: bounded, toward the target, never past it, slower when calm; nonsense holds
	bool Bounded = true;
	double H = 0;
	for (int32 I = 0; I < 240; ++I)
	{
		const double Was = H, Goal = I < 120 ? 1. : 0.;
		H = Smooth(H, Goal, 1. / 60., false);
		Bounded &= FMath::Abs(H - Was) <= MaxSlew / 60. + 1e-12 && H >= 0 && H <= 1 && (Goal > Was ? H <= Goal && H >= Was : H >= Goal && H <= Was);
	}
	Bounded &= Smooth(.4, Nan, 1. / 60., false) == .4 && Smooth(.4, 1., -1., false) == .4 && Smooth(.4, 1., Nan, false) == .4;
	Bounded &= Smooth(0, 1, .1, true) < Smooth(0, 1, .1, false);
	Check(Bounded, TEXT("closeness smoothing is bounded per frame, never overshoots, holds on nonsense, slower when calm"));
	// level: off silent, the ACQUIRE fix held silent, TUNE full then settled, ducked and calm lower
	const bool Levels = Level(EUse::Off, false, 0, false, false) == 0 && Level(EUse::Acquire, true, 0, false, false) == 0
		&& Level(EUse::Acquire, false, 0, false, false) == LevelAcquire && Level(EUse::Tune, true, Hold - .01, false, false) == LevelTune
		&& FMath::Abs(Level(EUse::Tune, true, 60., false, false) - LevelTune * Settled) < 1e-6 && TuneSettle(Hold + 1.) < 1. && TuneSettle(Hold + 1.) > Settled
		&& Level(EUse::Acquire, false, 0, true, false) < LevelAcquire * .5 && Level(EUse::Acquire, false, 0, false, true) < LevelAcquire;
	Check(Levels, TEXT("level: silent off and on a held fix; TUNE full at the lock, then settled under the bed; ducked under speech and playback; lower when calm"));
	if (!A || !A->Ctx.IsValid()) { Check(false, TEXT("an audio context for the graph checks")); return Good; }
	auto Render = [&A](double Secs)
	{
		const int32 N = FMath::RoundToInt32(Secs * 48000.);
		TArray<float> PCM; PCM.SetNumZeroed(N * 2); A->Ctx->Render(PCM.GetData(), N, 2);
		double E = 0; for (int32 I = N; I < PCM.Num(); ++I) E += double(PCM[I]) * PCM[I];
		return FMath::Sqrt(E / FMath::Max(1, N));
	};
	auto Run = [&](FS47RxSearch& S, const FIn& In, double Secs) { for (double T = 0; T < Secs; T += 1. / 30.) { S.Frame(A, In, 1. / 30.); Render(1. / 30.); } };
	FIn On; On.Use = EUse::Acquire; On.Closeness = .2;
	{
		// no files: no voice, no sound, no error (a folder that is not there)
		FS47RxSearch None; None.UseLoops({});
		Run(None, On, .5);
		Check(!None.Loaded() && None.Voices() == 0 && Render(.1) < 1e-6, TEXT("without the staged loops nothing plays and nothing fails"));
	}
	TArray<TSharedPtr<FS47AudioBuffer>> Fake;
	for (int32 K = 0; K < Bands; ++K)
	{
		TSharedRef<FS47AudioBuffer> B = A->Ctx->CreateBuffer(1, 48000, 48000.f);
		float* D = B->GetChannelData(0);
		for (int32 I = 0; I < 48000; ++I) D[I] = float(.5 * FMath::Sin(2. * PI * (60. + 20. * K) * I / 48000.));
		Fake.Add(B);
	}
	FS47RxSearch S; S.UseLoops(Fake);
	Run(S, On, .6);
	const double Far = Render(.2);
	Check(S.Voices() >= 1 && S.Voices() <= 2 && Far > 1e-3, TEXT("on ACQUIRE, far off: the near-start loops play"));
	On.Closeness = 1.;
	Run(S, On, 2.5);
	const double NearRms = Render(.2);
	Check(S.Target(Bands - 1) > .99 * LevelAcquire && S.Held() > .99 && NearRms > 1e-3, TEXT("closing in: the layer moves to the strong body"));
	FIn Lock = On; Lock.bLocked = true;
	Run(S, Lock, 3.);
	Check(S.Voices() == 0 && Render(.2) < 1e-5, TEXT("the held fix fades the layer out under the lock confirm and lets its voices go"));
	Run(S, On, 1.);
	Check(S.Voices() >= 1 && S.Held() > .99, TEXT("coming back to a search starts where the search is, not at the far end"));
	Run(S, FIn(), 2.);
	Check(S.Voices() == 0 && Render(.2) < 1e-5, TEXT("terminal closed (off): faded to silence and released"));
	FIn Tune; Tune.Use = EUse::Tune; Tune.Closeness = 1.; Tune.bLocked = true;
	Run(S, Tune, .5);
	const double Full = S.Target(Bands - 1);
	Run(S, Tune, 8.);
	Check(Full > .99 * LevelTune && S.Target(Bands - 1) < LevelTune * (Settled + .05) && S.Target(Bands - 1) > 0 && Render(.2) > 1e-4, TEXT("TUNE: full at the lock, then settled under the carrier bed"));
	S.Off(A);
	Check(S.Voices() == 0 && Render(.2) < 1e-5, TEXT("Off: stopped at once"));
	return Good;
}
