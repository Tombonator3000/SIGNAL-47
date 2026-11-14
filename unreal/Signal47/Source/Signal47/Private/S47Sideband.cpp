// SIDEBAND-20261113: the colour bus (WORN, BROKEN, WIDE, FAR), FLARES' two voices and its rules, and the drivers' pure mapping.
// Every chain and number is the offline prototypes' (Docs/Design/Audio/SIDEBAND_20261113.md), which Tom heard on 13 November.
// The session that reads the game is S47SidebandSession.cpp; the tests are S47SidebandTest.cpp.
#include "S47Sideband.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Sideband, Log, All);

namespace S47SbImpl
{
	constexpr double SbPi = 3.14159265358979323846;
	constexpr double LoopQuantum = double(S47Quantum) / 48000.; // a loop through a delay is this much longer than its delayTime
	constexpr double PushTau = .08;                         // how fast the graph follows a band (the bands glide by themselves)
	constexpr double StingerFront = .003;                   // SIDEBAND-20261113: the stinger's front, the prototype's 3 ms raised cosine
	double SbDb(double X) { return FMath::Pow(10., X / 20.); }
	double SbSweep(double F0, double F1, double I) { return F0 * FMath::Pow(F1 / F0, FMath::Clamp(I, 0., 1.)); }
	double SbSat(double X) { return FMath::Clamp(X, 0., 1.); }
	double SbSmooth(double A, double B, double X) { const double U = SbSat((X - A) / (B - A)); return U * U * (3. - 2. * U); }

	// One pass of a biquad over a signal (for the levels measured once when the noise is made).
	void SbFilter(ES47BiquadType Type, double Hz, double Q, TArray<float>& X)
	{
		double K[5]; FS47BiquadFilterNode::Coefficients(Type, 48000., Hz, 0., Q, 0., K);
		double X1 = 0, X2 = 0, Y1 = 0, Y2 = 0;
		for (float& V : X)
		{
			const double In = V, Y = K[0] * In + K[1] * X1 + K[2] * X2 - K[3] * Y1 - K[4] * Y2;
			X2 = X1; X1 = In; Y2 = Y1; Y1 = Y; V = float(Y);
		}
	}
	double SbRms(const TArray<float>& X, int32 From = 2000)
	{
		double S = 0; int32 N = 0;
		for (int32 I = From; I < X.Num(); ++I) { S += double(X[I]) * X[I]; ++N; }
		return FMath::Sqrt(S / FMath::Max(1, N));
	}
	double SbPeak(const TArray<float>& X)
	{
		double P = 0;
		for (float V : X) P = FMath::Max(P, FMath::Abs(double(V)));
		return P;
	}
	TArray<float> SbChannel(const FS47AudioBuffer& B, int32 C = 0)
	{
		TArray<float> Out; Out.Append(B.GetChannelData(C), B.Length());
		return Out;
	}

	// The buffers the colours need, the same every run.
	// hiss: 4 s of white noise, two channels, RMS 0.1
	TSharedRef<FS47AudioBuffer> HissBuffer()
	{
		auto B = MakeShared<FS47AudioBuffer>(2, 4 * 48000, 48000.f);
		FRandomStream R(4701);
		const float K = float(.1 * FMath::Sqrt(3.)); // uniform -1..1 has an RMS of 1/sqrt(3)
		for (int32 C = 0; C < 2; ++C) { float* D = B->GetChannelData(C); for (int32 I = 0; I < B->Length(); ++I) D[I] = R.FRandRange(-1.f, 1.f) * K; }
		return B;
	}
	// crackle: 7.3 s, about 2.5 single-frame ticks a second of random size and sign, nearly the same in both channels
	TSharedRef<FS47AudioBuffer> CrackleBuffer()
	{
		const int32 N = int32(7.3 * 48000);
		auto B = MakeShared<FS47AudioBuffer>(2, N, 48000.f);
		FRandomStream R(4702);
		float* L = B->GetChannelData(0); float* Rt = B->GetChannelData(1);
		bool bFirst = true;
		for (int32 I = 2400; I < N - 2400;)
		{
			I += int32(-FMath::Loge(FMath::Max(1e-6f, R.FRand())) / 2.5 * 48000.);
			if (I >= N - 2400) break;
			const float U = bFirst ? 1.f : R.FRandRange(.2f, 1.f);
			const float A = U * U * (R.FRand() < .5f ? -1.f : 1.f);
			L[I] = A; Rt[I] = A * R.FRandRange(.5f, 1.f);
			bFirst = false; I += 480;
		}
		return B;
	}
	// drift: 16.4 s of noise under 1.5 Hz, peak 1, kept at 100 Hz (the source plays it in step): the irregular part of the wow
	TSharedRef<FS47AudioBuffer> DriftBuffer()
	{
		const int32 N = 1640;
		auto B = MakeShared<FS47AudioBuffer>(1, N, 100.f);
		FRandomStream R(4703);
		float* D = B->GetChannelData(0);
		double Top = 0;
		TArray<double> X; X.SetNumZeroed(N);
		for (int32 K = 2; K <= 24; ++K) // whole turns in the loop, 0.12 to 1.46 Hz: it meets itself
		{
			const double A = R.FRandRange(.3f, 1.f), Ph = R.FRandRange(0.f, float(2. * SbPi));
			for (int32 I = 0; I < N; ++I) X[I] += A * FMath::Sin(2. * SbPi * K * I / N + Ph);
		}
		for (double V : X) Top = FMath::Max(Top, FMath::Abs(V));
		for (int32 I = 0; I < N; ++I) D[I] = float(X[I] / FMath::Max(Top, 1e-9));
		return B;
	}
	// the gentle BROKEN's dips, as (gate - 1): 13.7 s, zero with five raised-cosine notches 110 to 220 ms wide, four of them not
	// reaching the bottom; kept at 1 kHz
	TSharedRef<FS47AudioBuffer> DipBuffer()
	{
		const int32 Rate = 1000, N = 13700;
		auto B = MakeShared<FS47AudioBuffer>(1, N, float(Rate));
		float* D = B->GetChannelData(0);
		const double Dips[5][3] = { { 1.30, .14, 0. }, { 4.10, .22, .25 }, { 6.25, .11, .40 }, { 9.40, .18, .10 }, { 11.85, .13, .30 } };
		for (const double* Dip : Dips)
		{
			const int32 Wd = int32(Dip[1] * Rate), S = int32(Dip[0] * Rate) - Wd / 2;
			for (int32 I = 0; I < Wd; ++I) D[S + I] = float(-(1. - Dip[2]) * (.5 - .5 * FMath::Cos(2. * SbPi * I / (Wd - 1))));
		}
		return B;
	}
	TArray<float> SbCurve(TFunctionRef<double(double)> Fn)
	{
		TArray<float> C; C.SetNumUninitialized(4097);
		for (int32 I = 0; I < 4097; ++I) C[I] = float(Fn(-1. + 2. * I / 4096.));
		return C;
	}
	TSharedPtr<FS47AudioBuffer> LoadWav(const FString& Name)
	{
		TArray<uint8> Bytes;
		const FString Path = FPaths::ProjectContentDir() / TEXT("Audio/Raw/media/sideband") / (Name + TEXT(".wav"));
		if (!IFileManager::Get().FileExists(*Path) || !FFileHelper::LoadFileToArray(Bytes, *Path)) return nullptr;
		return S47Wa::DecodeWav(Bytes);
	}
}

// ---------------------------------------------------------------- names, tables
namespace S47Sideband
{
	EMode ModeFrom(const FString& Id)
	{
		return Id.Equals(TEXT("off"), ESearchCase::IgnoreCase) ? EMode::Off : Id.Equals(TEXT("soft"), ESearchCase::IgnoreCase) ? EMode::Soft : EMode::On;
	}
	const TCHAR* ReactsId(EMode In) { return In == EMode::Off ? TEXT("off") : In == EMode::Soft ? TEXT("soft") : TEXT("on"); }
	const TCHAR* StingersId(EMode In) { return In == EMode::Off ? TEXT("off") : In == EMode::Soft ? TEXT("soft") : TEXT("full"); }
	const TCHAR* TierName(ETier In) { return In == ETier::Full ? TEXT("full") : In == ETier::Glimpse ? TEXT("glimpse") : In == ETier::Hint ? TEXT("hint") : TEXT("none"); }

	// The palette (Tools/sideband/build_flares.py bakes each from the bought bundle; names are ours). Trim: what makes "the tier's
	// level" mean the same loudness for each after the tier's lowpass and FAR's band.
	const TArray<FStinger>& Stingers()
	{
		static const TArray<FStinger> All = {
			{ TEXT("full_braam_dsharp"), ETier::Full, 0. }, { TEXT("full_cluster_cold"), ETier::Full, 0. },
			{ TEXT("full_glass_low"), ETier::Full, 0. }, { TEXT("full_cluster_c"), ETier::Full, 0. },
			{ TEXT("glimpse_glass_swell"), ETier::Glimpse, -3.5 }, { TEXT("glimpse_struck_tone"), ETier::Glimpse, 4. },
			{ TEXT("glimpse_far_horn"), ETier::Glimpse, 5. },
			{ TEXT("hint_low_note"), ETier::Hint, 3. }, { TEXT("hint_low_swell"), ETier::Hint, 0. },
		};
		return All;
	}
	const TArray<const TCHAR*>& Beds()
	{
		static const TArray<const TCHAR*> All = { TEXT("bed_low_breath"), TEXT("bed_still"), TEXT("bed_cold_tone") };
		return All;
	}

	FTierMix TierMix(ETier Tier, bool bSoft, double TrimDb)
	{
		//                 level   lowpass  direct  send into FAR's room
		FTierMix M = Tier == ETier::Full ? FTierMix{ 9.5, 7500., 1., .22 } : Tier == ETier::Glimpse ? FTierMix{ -2., 2600., .12, .9 } : FTierMix{ -7., 240., 1., 0. };
		// soft: never louder than the score at its loudest 400 ms, and a darker top (the slower front is the voice's ramp)
		if (bSoft) { M.LevelDb = FMath::Min(M.LevelDb, 0.); M.Lowpass = FMath::Min(M.Lowpass, 3000.); }
		M.LevelDb += TrimDb;
		return M;
	}

	void BedFor(double Near, double& LevelDb, double& Lowpass)
	{
		const double N = S47SbImpl::SbSat(Near);
		LevelDb = -10. + 14. * N;
		Lowpass = S47SbImpl::SbSweep(200., 1000., N);
	}

	double FarTrimDb(const FString& File)
	{
		// Measured offline on each staged piece (Tools/sideband/measure_far.py: FAR at full about 5 LU under the dry piece).
		static const TMap<FString, double> Trim = {
#include "S47SidebandTrim.inl"
		};
		const double* Found = Trim.Find(FPaths::GetBaseFilename(File));
		return Found ? *Found : -6.5;
	}

	// ---------------------------------------------------------------- the drivers
	FWant Drive(const FDrive& In)
	{
		using namespace S47SbImpl;
		FWant W;
		if (!In.bFreeform || In.Reacts == EMode::Off) return W;
		const bool bSeen = In.bSeen || In.bHunt;
		const double Unease = FMath::Max(SbSat(In.Omen), SbSat((In.Stress - .25) / .75));
		// Indoors at SARO with nothing about and nothing just over: the routine, dry.
		W.bRoutine = In.bSaroIndoors && In.Approach < .05 && In.Omen < .05 && In.Presence < .04 && !bSeen && In.Gear < .05 && !In.bPowerOut
			&& In.AfterEncounter <= 0 && In.AfterTaken <= 0 && !In.bHiding && In.HintNudge <= 0;
		const double CapBroken = In.Tier == ETier::Full ? BrokenFullCap : In.Tier == ETier::Glimpse ? BrokenGlimpseCap : BrokenHintCap;
		if (!W.bRoutine)
		{
			// WORN: tired, late (nothing before 02:00, all of it by 03:30, gone again by six), uneasy; a nudge after a deniable hint
			const double Late = SbSat((In.NightHour - 26.) / 1.5) * (1. - SbSat((In.NightHour - 29.) / 1.));
			W.Bands.Worn = FMath::Min(WornCap, .40 * SbSat(In.Fatigue) + .25 * Late + .20 * Unease + .15 * SbSat(In.HintNudge));
			// BROKEN: how near it is, and the gear failing. Never above a small cap before midnight.
			const double Near = FMath::Max(SbSat(In.Presence), .5 * SbSat(In.Approach));
			double B = FMath::Max(.6 * SbSmooth(.1, .9, Near), .45 * SbSat(In.Gear));
			if (In.bPowerOut) B = FMath::Max(B, .25);
			if (In.bHunt) B = FMath::Max(B, BrokenFullCap);
			// The omen silence: while the night has dropped and nothing has been seen, the silence is the tell. Hold back.
			if (In.Hush > .5 && !bSeen) B = FMath::Min(B, HushBrokenCap);
			W.Bands.Broken = FMath::Min(B, CapBroken);
			// WIDE: on foot on open land, more in wind
			W.Bands.Wide = In.bOutdoors ? FMath::Min(WideCap, SbSat(In.Open) * (.35 + .45 * SbSat(In.Wind))) : 0.;
			// FAR: after a meeting, hiding, after lost time
			W.Bands.Far = SbSat(FMath::Max3(SbSat(In.AfterEncounter), In.bHiding ? .55 : 0., .8 * SbSat(In.AfterTaken)));
		}
		// Tension for the stems: what the player can perceive, capped by the tier.
		double Tn = FMath::Max3(.35 * SbSat(In.Approach), In.Presence > .04 ? .4 + .3 * SbSat(In.Presence) : 0., .3 * SbSat(In.Omen));
		if (In.bSeen) Tn = FMath::Max(Tn, .7);
		if (In.bHunt) Tn = 1.;
		W.Tension = FMath::Min(Tn, In.Tier == ETier::Full ? 1. : In.Tier == ETier::Glimpse ? .6 : .3);
		// The bed: none in a hint, capped in a glimpse, and held down under an unseen hush.
		W.BedCap = In.Tier == ETier::Full ? 1. : In.Tier == ETier::Glimpse ? GlimpseBedCap : 0.;
		if (In.Hush > .5 && !bSeen) W.BedCap = FMath::Min(W.BedCap, HushBedCap);
		if (In.Reacts == EMode::Soft)
		{
			W.Bands.Worn = FMath::Min(W.Bands.Worn, SoftWorn); W.Bands.Broken = FMath::Min(W.Bands.Broken, SoftBroken);
			W.Bands.Wide = FMath::Min(W.Bands.Wide, SoftWide); W.Bands.Far = FMath::Min(W.Bands.Far, SoftFar);
			W.BedCap = FMath::Min(W.BedCap, .5);
		}
		return W;
	}

	double FGlide::Step(double Dt, double Want)
	{
		Want = FMath::IsFinite(Want) ? S47SbImpl::SbSat(Want) : 0.;
		Dt = FMath::IsFinite(Dt) ? FMath::Clamp(Dt, 0., .25) : 0.;
		if (Want > Now) Now = FMath::Min(Want, Now + Dt / FMath::Max(.05, Up));
		else Now = FMath::Max(Want, Now - Dt / FMath::Max(.05, Down));
		return Now;
	}

	// ---------------------------------------------------------------- FLARES' rules
	int32 FRules::Begin(double T, ETier InTier)
	{
		(void)T;
		bEvent = true; bSpent = false; Tier = InTier; Bed = -1;
		if (Tier == ETier::Glimpse || Tier == ETier::Full)
		{
			// a different bed than the last event's
			TArray<int32, TInlineAllocator<4>> Pool;
			for (int32 I = 0; I < Beds().Num(); ++I) if (I != LastBed) Pool.Add(I);
			Bed = Pool[Rng.RandHelper(Pool.Num())];
			LastBed = Bed;
		}
		return Bed;
	}

	int32 FRules::Moment(double T, bool bSpeech, bool& bSoft, FString& Why, TFunctionRef<bool(int32)> Have)
	{
		bSoft = false;
		if (!bEvent || Tier == ETier::None) { Why = TEXT("no event, or no tier at this hour"); return -1; }
		if (bSpent) { Why = TEXT("this event has had its moment"); return -1; }
		bSpent = true; // one chance per event, used or not: nothing is queued
		const int32 Got = Decide(T, bSpeech, bSoft, Why, Have);
		if (Tier == ETier::Full) FullHistory.Add(Got >= 0);
		return Got;
	}

	int32 FRules::Decide(double T, bool bSpeech, bool& bSoft, FString& Why, TFunctionRef<bool(int32)> Have)
	{
		if (Setting == EMode::Off) { Why = TEXT("setting: stingers off"); return -1; }
		const double Cool = LastTier == ETier::Full ? CooldownFull : LastTier == ETier::Glimpse ? CooldownGlimpse : CooldownHint;
		if (T - LastTime < Cool) { Why = FString::Printf(TEXT("cooldown: %.0f s since a %s stinger (needs %.0f)"), T - LastTime, TierName(LastTier), Cool); return -1; }
		if (Tier == ETier::Full)
		{
			if (T - LastFullTime < FullGap) { Why = FString::Printf(TEXT("cooldown: %.0f s since the last full stinger (needs %.0f)"), T - LastFullTime, FullGap); return -1; }
			bool bRun = FullHistory.Num() >= FullRun;
			for (int32 I = 0; bRun && I < FullRun; ++I) bRun = FullHistory[FullHistory.Num() - 1 - I];
			if (bRun) { Why = TEXT("the two full events before this had one: this one is the duck alone"); return -1; }
		}
		bSoft = Setting == EMode::Soft;
		if (bSpeech)
		{
			if (Tier != ETier::Full) { Why = TEXT("a voice or a signal is sounding: no hint or glimpse stinger over it"); return -1; }
			bSoft = true;
		}
		// never the same twice in a row: not the last one played in any tier, nor the last of this tier
		TArray<int32, TInlineAllocator<8>> Pool, Loose;
		const TArray<FStinger>& All = Stingers();
		for (int32 I = 0; I < All.Num(); ++I)
		{
			if (All[I].Tier != Tier || !Have(I) || I == LastStinger) continue;
			Loose.Add(I);
			if (I != LastOfTier[int32(Tier)]) Pool.Add(I);
		}
		const TArray<int32, TInlineAllocator<8>>& From = Pool.Num() ? Pool : Loose;
		if (From.IsEmpty()) { Why = TEXT("no stinger of this tier is in this build"); return -1; }
		const int32 Pick = From[Rng.RandHelper(From.Num())];
		LastStinger = Pick; LastOfTier[int32(Tier)] = Pick; LastTime = T; LastTier = Tier;
		if (Tier == ETier::Full) LastFullTime = T;
		Why = TEXT("ok");
		return Pick;
	}
}

// ---------------------------------------------------------------- the tap
void FS47SidebandTap::Arm(double Seconds)
{
	FS47AudioContext::FBatch B(*Ctx);
	Want = int32(FMath::Clamp(Seconds, 1., 600.) * Ctx->SampleRate) * 2;
	Kept.Reset(Want + 2 * S47Quantum); // (all of it now, on the game thread: the audio thread only fills it)
	bArmed = true;
}
bool FS47SidebandTap::Full() const { FS47AudioContext::FBatch B(*Ctx); return bArmed && Kept.Num() >= Want; }
TArray<float> FS47SidebandTap::Take()
{
	FS47AudioContext::FBatch B(*Ctx);
	bArmed = false;
	TArray<float> Got = MoveTemp(Kept);
	Kept.Reset();
	return Got;
}
void FS47SidebandTap::Process(int64 Quantum)
{
	PullInputs(Quantum, Out, 2);
	if (!bArmed || Kept.Num() >= Want) return;
	const int32 R = Out.Channels > 1 ? 1 : 0;
	for (int32 J = 0; J < S47Quantum; ++J) { Kept.Add(Out.D[0][J]); Kept.Add(Out.D[R][J]); }
}

// ---------------------------------------------------------------- the bus
FS47SidebandBus::FS47SidebandBus(const TSharedRef<FS47AudioContext>& InCtx) : Ctx(InCtx)
{
	FS47AudioContext::FBatch B(*Ctx);
	Head = Ctx->CreateGain(); Moment = Ctx->CreateGain(); Thru = Ctx->CreateGain();
	Out = MakeShared<FS47SidebandTap>(Ctx);
	Head->Connect(Moment); Moment->Connect(Thru); Thru->Connect(Out);
}

FS47SidebandBus::~FS47SidebandBus()
{
	FS47AudioContext::FBatch B(*Ctx);
	StopBed(); EndStinger();
	for (const TSharedPtr<FS47ScheduledSourceNode>& S : Sources) if (S) { S->Stop(); S->Disconnect(); }
	for (const TSharedPtr<FS47AudioNode>& N : Keep) if (N) N->Disconnect();
	if (Head) Head->Disconnect();
	if (Moment) Moment->Disconnect();
	if (Thru) Thru->Disconnect();
	if (Out) Out->Disconnect();
}

TSharedRef<FS47GainNode> FS47SidebandBus::G(double V)
{
	auto N = Add(Ctx->CreateGain()); N->Gain.SetValue(V);
	return N;
}
TSharedRef<FS47BiquadFilterNode> FS47SidebandBus::Bq(const TCHAR* Type, double Hz, double Q, double GainDb)
{
	auto N = Add(Ctx->CreateBiquadFilter()); N->SetType(Type);
	N->Frequency.SetValue(Hz); N->Q.SetValue(Q); N->Gain.SetValue(GainDb);
	return N;
}
TSharedRef<FS47OscillatorNode> FS47SidebandBus::Osc(double Hz, const TCHAR* Type)
{
	auto N = Add(Ctx->CreateOscillator()); N->SetType(Type); N->Frequency.SetValue(Hz);
	N->Start(0); Sources.Add(N);
	return N;
}
TSharedRef<FS47BufferSourceNode> FS47SidebandBus::Loop(const TSharedPtr<FS47AudioBuffer>& Buffer, double Offset)
{
	auto N = Add(Ctx->CreateBufferSource()); N->Buffer = Buffer; N->bLoop = true;
	N->Start(0, Offset); Sources.Add(N);
	return N;
}
// A two-channel signal as one channel: a Gain fed with ones, the signal into its gain (what goes into a parameter is mixed down).
TSharedRef<FS47GainNode> FS47SidebandBus::Mono(const TSharedPtr<FS47AudioNode>& From)
{
	auto N = G(0);
	Ones->Connect(N);
	From->Connect(N->Gain);
	return N;
}

// The noise the colours need and its levels, measured once: hiss -47 dBFS RMS at full after its two filters, crackle peaking
// at -36 dBFS, the static's band set to an RMS of 1. No lock is held here.
void FS47SidebandBus::Prepare()
{
	using namespace S47SbImpl;
	if (BufHiss.IsValid()) return;
	BufHiss = HissBuffer(); BufCrackle = CrackleBuffer(); BufDrift = DriftBuffer(); BufDips = DipBuffer();
	{
		TArray<float> X = SbChannel(*BufHiss);
		SbFilter(ES47BiquadType::Highpass, 1200., -3., X); SbFilter(ES47BiquadType::Lowpass, 8000., -3., X);
		KHiss = SbDb(-47.) / FMath::Max(1e-6, SbRms(X));
	}
	{
		TArray<float> X = SbChannel(*BufCrackle);
		SbFilter(ES47BiquadType::Bandpass, 2800., .9, X);
		KCrackle = SbDb(-36.) / FMath::Max(1e-6, SbPeak(X));
	}
	{
		TArray<float> X = SbChannel(*BufHiss);
		SbFilter(ES47BiquadType::Bandpass, 2400., .6, X);
		StaticK = 1. / FMath::Max(1e-6, SbRms(X));
	}
}

void FS47SidebandBus::Build()
{
	using namespace S47SbImpl;
	if (bBuilt) return;
	bBuilt = true;
	Prepare(); // (the noise is made and measured before the lock is taken: the audio thread waits only for the nodes)
	FS47AudioContext::FBatch B(*Ctx);
	OnesBuf = MakeShared<FS47AudioBuffer>(1, S47Quantum, Ctx->SampleRate);
	for (int32 I = 0; I < S47Quantum; ++I) OnesBuf->GetChannelData(0)[I] = 1.f;
	Ones = G(1); Loop(OnesBuf)->Connect(Ones);
	const TSharedPtr<FS47AudioBuffer> Hiss = BufHiss;
	// The chain works at the staged level of the score's files: the shapers' numbers are for that.
	Pre = G(1. / Level);
	BuildWorn(Pre, Hiss);
	BuildBroken(Hiss);
	BuildWide();
	BuildFar();
	ChainOut = G(0);
	auto Post = G(Level);
	FarOut->Connect(Post); Post->Connect(ChainOut);
	for (TArray<FAim>* Aims : { &WornAims, &BrokenAims, &WideAims, &FarAims }) for (FAim& A : *Aims) A.P->SetValue(A.Fn(0.));
	// the gentle hit's own hole: a 160 ms raised-cosine dip to a fifth
	HoleCurve.SetNumUninitialized(65);
	for (int32 I = 0; I < 65; ++I) HoleCurve[I] = float(1. - .8 * FMath::Square(FMath::Sin(SbPi * I / 64.)));
	UE_LOG(LogS47Sideband, Display, TEXT("S47: SIDEBAND chain built, %d nodes"), NodeCount);
}

// WORN: Delay 12 ms (moved by three oscillators and a drift buffer) -> [dry | +8 dB -> tape curve -> -8 dB] -> lowpass 18 k -> 2.6 k
// -> highpass 20 -> 70 Hz -> level (wobbling) ; hiss and crackle beside it.
void FS47SidebandBus::BuildWorn(const TSharedPtr<FS47AudioNode>& In, const TSharedPtr<FS47AudioBuffer>& Hiss)
{
	using namespace S47SbImpl;
	auto Delay = Add(Ctx->CreateDelay(.05)); Delay->DelayTime.SetValue(Latency);
	In->Connect(Delay);
	// a sine of depth D at f Hz in delayTime bends pitch by 2 pi f D: 0.35 %, 0.15 %, 0.10 %
	auto Mod = G(0);
	const double Wow[3][2] = { { .41, .0035 }, { .93, .0015 }, { 6.7, .0010 } };
	for (const double* W : Wow) { auto Depth = G(W[1] / (2. * SbPi * W[0])); Osc(W[0])->Connect(Depth); Depth->Connect(Mod); }
	{ auto Depth = G(.00035); Loop(BufDrift)->Connect(Depth); Depth->Connect(Mod); }
	Mod->Connect(Delay->DelayTime);
	WornAims.Add({ &Mod->Gain, [](double W) { return W; } });
	auto Dry = G(1), SatIn = G(SbDb(8.)), SatOut = G(0), Sum = G(1);
	auto Tape = Add(Ctx->CreateWaveShaper()); Tape->Curve = SbCurve([](double X) { return FMath::Tanh(1.6 * X) / 1.6; });
	Delay->Connect(Dry); Dry->Connect(Sum);
	Delay->Connect(SatIn); SatIn->Connect(Tape); Tape->Connect(SatOut); SatOut->Connect(Sum);
	WornAims.Add({ &Dry->Gain, [](double W) { return 1. - .8 * W; } });
	WornAims.Add({ &SatOut->Gain, [](double W) { return SbDb(-8.) * .8 * W; } });
	auto Lp = Bq(TEXT("lowpass"), 18000., -3.), Hp = Bq(TEXT("highpass"), 20., -3.);
	WornAims.Add({ &Lp->Frequency, [](double W) { return SbSweep(18000., 2600., W); } });
	WornAims.Add({ &Hp->Frequency, [](double W) { return SbSweep(20., 70., W); } });
	auto Lvl = G(1), Wobble = G(0);
	Osc(.31)->Connect(Wobble); Wobble->Connect(Lvl->Gain);
	WornAims.Add({ &Lvl->Gain, [](double W) { return SbDb(1.4 * W); } });
	WornAims.Add({ &Wobble->Gain, [](double W) { return .10 * W * SbDb(1.4 * W); } });
	WornOut = G(1);
	Sum->Connect(Lp); Lp->Connect(Hp); Hp->Connect(Lvl); Lvl->Connect(WornOut);
	// hiss: -47 dBFS RMS at full after its two filters; crackle: peaks -36 dBFS at full. Both measured here, once.
	WornNoise = G(bNoise ? 1. : 0.); WornNoise->Connect(WornOut);
	{
		const double K = KHiss;
		auto Hp2 = Bq(TEXT("highpass"), 1200., -3.), Lp2 = Bq(TEXT("lowpass"), 8000., -3.); auto Gn = G(0);
		Loop(Hiss)->Connect(Hp2); Hp2->Connect(Lp2); Lp2->Connect(Gn); Gn->Connect(WornNoise);
		WornAims.Add({ &Gn->Gain, [K](double W) { return K * FMath::Pow(W, 1.5); } });
	}
	{
		const TSharedPtr<FS47AudioBuffer> Crackle = BufCrackle;
		const double K = KCrackle;
		auto Bp = Bq(TEXT("bandpass"), 2800., .9); auto Gn = G(0);
		Loop(Crackle)->Connect(Bp); Bp->Connect(Gn); Gn->Connect(WornNoise);
		WornAims.Add({ &Gn->Gain, [K](double W) { return K * W * W; } });
	}
}

// BROKEN, the gentle one: highpass 20 -> 320 -> [clean | drive -> uneven clip -> (as it is | 13 steps) -> ring by a sawtooth ->
// highpass 60 -> level] -> dips (a buffer into the gain) -> two lowpasses 18 k -> 2.8 k -> peak at 1.5 k ; static beside it.
void FS47SidebandBus::BuildBroken(const TSharedPtr<FS47AudioBuffer>& Hiss)
{
	using namespace S47SbImpl;
	auto Hp = Bq(TEXT("highpass"), 20., 0.);
	WornOut->Connect(Hp);
	BrokenAims.Add({ &Hp->Frequency, [](double V) { return SbSweep(20., 320., V); } });
	auto Clean = G(1), Sum = G(1);
	Hp->Connect(Clean); Clean->Connect(Sum);
	BrokenAims.Add({ &Clean->Gain, [](double V) { return 1. - FMath::Pow(V, .7); } });
	HitDrive = G(1);
	auto Drive = G(SbDb(4.));
	auto Clip = Add(Ctx->CreateWaveShaper());
	Clip->Curve = SbCurve([](double X) { return X >= 0 ? FMath::Tanh(2.5 * X) / FMath::Tanh(2.5) : .75 * FMath::Tanh(3.3 * X) / FMath::Tanh(3.3); });
	Hp->Connect(HitDrive); HitDrive->Connect(Drive); Drive->Connect(Clip);
	BrokenAims.Add({ &Drive->Gain, [](double V) { return SbDb(4. + 14. * V); } });
	// c: how much of the stepped copy (from b 0.25, all of it by 0.85)
	const auto Steps = [](double V) { return SbSat((V - .25) / .6); };
	auto Smooth1 = G(1), StepG = G(0), Ring = G(1), RingDepth = G(0);
	auto Stepper = Add(Ctx->CreateWaveShaper()); Stepper->Curve = SbCurve([](double X) { return FMath::RoundToDouble(X * 6.) / 6.; });
	Clip->Connect(Smooth1); Smooth1->Connect(Ring);
	Clip->Connect(Stepper); Stepper->Connect(StepG); StepG->Connect(Ring);
	BrokenAims.Add({ &Smooth1->Gain, [Steps](double V) { return 1. - Steps(V); } });
	BrokenAims.Add({ &StepG->Gain, [Steps](double V) { return Steps(V); } });
	// the sawtooth into the gain is ring modulation: copies of the music round 3.7 kHz, as a sample-rate reducer's images
	auto Saw = Osc(9000., TEXT("sawtooth"));
	Saw->Connect(RingDepth); RingDepth->Connect(Ring->Gain);
	BrokenAims.Add({ &Saw->Frequency, [](double V) { return SbSweep(9000., 3700., V); } });
	BrokenAims.Add({ &RingDepth->Gain, [Steps](double V) { return .5 * Steps(V); } });
	auto DirtyHp = Bq(TEXT("highpass"), 60., -3.); auto Dirty = G(0);
	HitLevel = G(1);
	Ring->Connect(DirtyHp); DirtyHp->Connect(Dirty); Dirty->Connect(HitLevel); HitLevel->Connect(Sum);
	BrokenAims.Add({ &Dirty->Gain, [](double V) { return FMath::Pow(V, .7) * SbDb(-8.5 - 13. * V); } });
	// the dips: from b 0.5, no deeper after 0.8, never more than 0.55 of the way down; the same buffer lifts the static in them
	const auto Depth = [](double V) { return .55 * SbSat((V - .5) / .3); };
	const TSharedPtr<FS47AudioBuffer> Dips = BufDips;
	auto DipSrc = Loop(Dips);
	auto Gate = G(1), GateDepth = G(0);
	DipSrc->Connect(GateDepth); GateDepth->Connect(Gate->Gain);
	BrokenAims.Add({ &GateDepth->Gain, [Depth](double V) { return Depth(V); } });
	HitHole = G(1);
	auto Lp1 = Bq(TEXT("lowpass"), 18000., 4.), Lp2 = Bq(TEXT("lowpass"), 18000., -3.), Pk = Bq(TEXT("peaking"), 1500., 1., 0.);
	Sum->Connect(Gate); Gate->Connect(HitHole); HitHole->Connect(Lp1); Lp1->Connect(Lp2); Lp2->Connect(Pk);
	BrokenAims.Add({ &Lp1->Frequency, [](double V) { return SbSweep(18000., 2800., V); } });
	BrokenAims.Add({ &Lp2->Frequency, [](double V) { return SbSweep(18000., 2800., V); } });
	BrokenAims.Add({ &Pk->Gain, [](double V) { return 7. * V; } });
	BrokenOut = G(1); Pk->Connect(BrokenOut);
	// static: the hiss through a band at 2.4 kHz, set to an RMS of 1 here; -46 dB at full, -38 dB in a dip, -24 dB in the hit
	const double K = StaticK;
	auto Bp = Bq(TEXT("bandpass"), 2400., .6); auto Static = G(0), Fill = G(0);
	BrokenNoise = G(bNoise ? 1. : 0.); BrokenNoise->Connect(BrokenOut);
	Loop(Hiss, 12345. / 48000.)->Connect(Bp); Bp->Connect(Static); Static->Connect(BrokenNoise);
	DipSrc->Connect(Fill); Fill->Connect(Static->Gain);
	BrokenAims.Add({ &Static->Gain, [K](double V) { return K * SbDb(-46.) * V * V; } });
	BrokenAims.Add({ &Fill->Gain, [K, Depth](double V) { return -K * SbDb(-38.) * Depth(V); } });
	HitStatic = G(0); Bp->Connect(HitStatic); HitStatic->Connect(BrokenNoise);
	// FLARES' bed and stingers join here: after BROKEN (its highpass and dips would gut them), before WIDE and FAR
	FlareSum = G(1); BrokenOut->Connect(FlareSum);
}

// WIDE: [dry | two short moving delays -> highpass 250 -> a StereoPanner each, swaying] -> a peak at 900 Hz that wanders.
void FS47SidebandBus::BuildWide()
{
	using namespace S47SbImpl;
	auto Dry = G(1), Sum = G(1);
	FlareSum->Connect(Dry); Dry->Connect(Sum);
	WideAims.Add({ &Dry->Gain, [](double A) { return 1. - .15 * A; } });
	auto Sway = G(.4); Osc(.12)->Connect(Sway);
	const double Voice[2][4] = { { .018, .0035, .19, -.55 }, { .027, .0045, .27, .55 } };
	for (const double* V : Voice)
	{
		auto Delay = Add(Ctx->CreateDelay(.1)); Delay->DelayTime.SetValue(V[0]);
		auto Depth = G(V[1]); Osc(V[2])->Connect(Depth); Depth->Connect(Delay->DelayTime);
		auto Hp = Bq(TEXT("highpass"), 250., -3.);
		auto Pan = Add(Ctx->CreateStereoPanner()); Pan->Pan.SetValue(V[3]); Sway->Connect(Pan->Pan);
		auto Gn = G(0);
		FlareSum->Connect(Delay); Delay->Connect(Hp); Hp->Connect(Pan); Pan->Connect(Gn); Gn->Connect(Sum);
		WideAims.Add({ &Gn->Gain, [](double A) { return .6 * A; } });
	}
	auto Pk = Bq(TEXT("peaking"), 900., .9, 0.);
	auto Cents = G(1680.); Osc(.083)->Connect(Cents); Cents->Connect(Pk->Detune);
	WideAims.Add({ &Pk->Gain, [](double A) { return 6. * A; } });
	WideOut = G(1);
	Sum->Connect(Pk); Pk->Connect(WideOut);
	WideAims.Add({ &WideOut->Gain, [](double A) { return SbDb(-2.8 * A); } });
}

// FAR: what is left dry (lowpass 18 k -> 1.5 k, drifting from side to side) | one channel -> highpass 200 -> lowpass 3 k ->
// a slow ping-pong and, after 30 ms and two diffusers, an eight-line network; all of it lowpassed and behind one return.
void FS47SidebandBus::BuildFar()
{
	using namespace S47SbImpl;
	FarOut = G(1);
	auto Dry = G(1); auto Lp = Bq(TEXT("lowpass"), 18000., -3.);
	auto Pan = Add(Ctx->CreateStereoPanner()); auto PanDepth = G(0);
	Osc(.07)->Connect(PanDepth); PanDepth->Connect(Pan->Pan);
	WideOut->Connect(Dry); Dry->Connect(Lp); Lp->Connect(Pan); Pan->Connect(FarOut);
	FarAims.Add({ &Dry->Gain, [](double F) { return 1. - .9 * F * F; } });
	FarAims.Add({ &Lp->Frequency, [](double F) { return SbSweep(18000., 1500., F); } });
	FarAims.Add({ &PanDepth->Gain, [](double F) { return .6 * F; } });
	// the send: one channel, the piece's own trim on it (the room is linear, so a trim here is a trim on everything wet)
	auto Send = G(0);
	Mono(WideOut)->Connect(Send);
	FarAims.Add({ &Send->Gain, [this](double F) { return F * S47SbImpl::SbDb(TrimDb); } });
	RoomIn = G(1); Send->Connect(RoomIn); // (a stinger's send comes in here too, untrimmed)
	auto SHp = Bq(TEXT("highpass"), 200., -3.), SLp = Bq(TEXT("lowpass"), 3000., -3.);
	RoomIn->Connect(SHp); SHp->Connect(SLp);
	Return = G(bHeld ? 0. : 1.);
	// the ping-pong: send -> A 0.56 s -> B 0.56 s -> lowpass 1.6 k -> 0.55 -> back into A (the loop's quantum taken off B)
	auto AIn = G(1);
	auto PA = Add(Ctx->CreateDelay(1.)), PB = Add(Ctx->CreateDelay(1.));
	PA->DelayTime.SetValue(.56); PB->DelayTime.SetValue(.56 - LoopQuantum);
	auto PLp = Bq(TEXT("lowpass"), 1600., -3.); auto PFb = G(.55);
	SLp->Connect(AIn); AIn->Connect(PA); PA->Connect(PB); PB->Connect(PLp); PLp->Connect(PFb); PFb->Connect(AIn);
	auto PanA = Add(Ctx->CreateStereoPanner()), PanB = Add(Ctx->CreateStereoPanner());
	PanA->Pan.SetValue(-.8); PanB->Pan.SetValue(.8);
	auto PWet = G(.5);
	PA->Connect(PanA); PB->Connect(PanB); PanA->Connect(PWet); PanB->Connect(PWet); PWet->Connect(Return);
	// into the room: the send and 0.6 of both echoes, 30 ms late, through two diffusers
	auto ToRoom = G(.6); PA->Connect(ToRoom); PB->Connect(ToRoom);
	auto PreDelay = Add(Ctx->CreateDelay(.1)); PreDelay->DelayTime.SetValue(.030);
	SLp->Connect(PreDelay); ToRoom->Connect(PreDelay);
	TSharedPtr<FS47AudioNode> R = PreDelay;
	for (const double Secs : { .0079, .0127 })
	{
		// v = x + g v(n - M); y = -g v + v(n - M). The loop's quantum is taken off the delay; the forward tap is connected first
		// so that the sum is pulled before the delay and both taps see the same M (an exact all-pass: the impulse test holds it).
		auto Sum = G(1), Fb = G(.62), Fwd = G(-.62), ApOut = G(1);
		auto Line = Add(Ctx->CreateDelay(.05)); Line->DelayTime.SetValue(Secs - LoopQuantum);
		R->Connect(Sum); Sum->Connect(Line); Line->Connect(Fb); Fb->Connect(Sum);
		Line->Connect(ApOut); Sum->Connect(Fwd); Fwd->Connect(ApOut);
		R = ApOut;
	}
	// eight lines; each back into itself through its share of a 7 s decay and a highshelf (the highs die faster), all through
	// one shared Gain of -2/8 fed by every line (a Householder reflection: lossless before the gains)
	const double Lines[8] = { .0613, .0739, .0891, .1017, .1273, .1439, .1631, .1877 };
	auto House = G(-2. / 8.);
	auto Left = G(1), Right = G(1);
	for (int32 I = 0; I < 8; ++I)
	{
		const double D = Lines[I];
		auto In = G((I % 2 ? -1. : 1.) / FMath::Sqrt(8.)); auto LineIn = G(1);
		auto Line = Add(Ctx->CreateDelay(.25)); Line->DelayTime.SetValue(D - LoopQuantum);
		auto Back = G(FMath::Pow(10., -3. * D / 7.));
		auto Shelf = Bq(TEXT("highshelf"), 1800., 0., -14. * D);
		R->Connect(In); In->Connect(LineIn); LineIn->Connect(Line);
		Line->Connect(Back); Line->Connect(House); House->Connect(Back);
		Back->Connect(Shelf); Shelf->Connect(LineIn);
		if (I == 2 || I == 6) { auto Depth = G(.0004); Osc(I == 2 ? .11 : .14)->Connect(Depth); Depth->Connect(Line->DelayTime); }
		// lines 0 2 4 6 (+ - + -) to the left, 1 3 5 7 (+ - + -) to the right
		auto Sign = G((I / 2) % 2 ? -1. : 1.);
		Line->Connect(Sign); Sign->Connect(I % 2 ? Right : Left);
	}
	auto PanL = Add(Ctx->CreateStereoPanner()), PanR = Add(Ctx->CreateStereoPanner());
	PanL->Pan.SetValue(-.8); PanR->Pan.SetValue(.8);
	auto RLp = Bq(TEXT("lowpass"), 4500., -3.); auto RGain = G(.5);
	Left->Connect(PanL); Right->Connect(PanR); PanL->Connect(RLp); PanR->Connect(RLp); RLp->Connect(RGain); RGain->Connect(Return);
	// (Return joins FarOut only while the room is wanted: Room())
}

void FS47SidebandBus::Engage()
{
	Build();
	FS47AudioContext::FBatch B(*Ctx);
	const double At = Ctx->CurrentTime();
	Moment->Connect(Pre); ChainOut->Connect(Out);
	// 30 ms for the chain's own delay to fill, then 60 ms across
	for (int32 I = 0; I < 2; ++I)
	{
		FS47AudioParam& P = I ? Thru->Gain : ChainOut->Gain;
		const double V0 = P.ValueAt(At);
		P.CancelScheduledValues(At); P.SetValueAtTime(V0, At); P.SetValueAtTime(V0, At + .03); P.LinearRampToValueAtTime(I ? 0. : 1., At + .09);
	}
	bEngaged = true; IdleFor = 0; LetGoAt = -1; EngagedAt = At;
	Pushed = { -1, -1, -1, -1 };
}

bool FS47SidebandBus::Warm() const { return bEngaged && LetGoAt < 0 && Ctx->CurrentTime() >= EngagedAt + .09; }
void FS47SidebandBus::Wake() { if (!bEngaged || LetGoAt >= 0) Engage(); }

void FS47SidebandBus::LetGo()
{
	if (!bEngaged || LetGoAt >= 0) return;
	FS47AudioContext::FBatch B(*Ctx);
	const double At = Ctx->CurrentTime();
	for (int32 I = 0; I < 2; ++I)
	{
		FS47AudioParam& P = I ? Thru->Gain : ChainOut->Gain;
		const double V0 = P.ValueAt(At);
		P.CancelScheduledValues(At); P.SetValueAtTime(V0, At); P.LinearRampToValueAtTime(I ? 1. : 0., At + .06);
	}
	LetGoAt = At + .12;
}

void FS47SidebandBus::Room(bool bOn)
{
	if (bOn == bRoom || !bBuilt) return;
	FS47AudioContext::FBatch B(*Ctx);
	if (bOn) Return->Connect(FarOut); else Return->Disconnect(FarOut);
	bRoom = bOn; RoomIdleFor = 0;
}

void FS47SidebandBus::Push(TArray<FAim>& Aims, double V, double At)
{
	for (FAim& A : Aims)
	{
		const double V0 = A.P->ValueAt(At);
		A.P->CancelScheduledValues(At); A.P->SetValueAtTime(V0, At); A.P->SetTargetAtTime(A.Fn(V), At, S47SbImpl::PushTau);
	}
}

void FS47SidebandBus::Set(const FS47SidebandBands& In)
{
	const auto Clean = [](double V) { return FMath::IsFinite(V) ? FMath::Clamp(V, 0., 1.) : 0.; };
	Now = { Clean(In.Worn), Clean(In.Broken), Clean(In.Wide), Clean(In.Far) };
}

void FS47SidebandBus::Step(double Dt)
{
	Dt = FMath::IsFinite(Dt) ? FMath::Clamp(Dt, 0., .5) : 0.;
	const double At = Ctx->CurrentTime();
	if (StingSrc && At > StingEnd) EndStinger();
	if (BedSrc && !bBedOn) { BedOffFor += Dt; if (BedOffFor > 12.) StopBed(); }
	const bool bWant = Now.Any() || BedSrc.IsValid() || StingSrc.IsValid() || At < RoomUntil || At < HitUntil;
	if (bWant)
	{
		IdleFor = 0;
		if (!bEngaged || LetGoAt >= 0) Engage();
	}
	else if (bEngaged && LetGoAt < 0)
	{
		IdleFor += Dt;
		if (IdleFor > Tail) LetGo();
	}
	if (!bEngaged) return;
	FS47AudioContext::FBatch B(*Ctx);
	if (LetGoAt >= 0 && At >= LetGoAt)
	{
		// across: nothing pulls the chain now, and its sources stand still
		ChainOut->Disconnect(Out); Moment->Disconnect(Pre);
		bEngaged = false; LetGoAt = -1; Room(false);
		return;
	}
	const auto Moved = [](double A, double V) { return FMath::Abs(A - V) > .002 || (A <= 0 && V > 0); };
	if (Moved(Now.Worn, Pushed.Worn)) { Push(WornAims, Now.Worn, At); Pushed.Worn = Now.Worn; }
	if (At >= HitUntil && Moved(Now.Broken, Pushed.Broken)) { Push(BrokenAims, Now.Broken, At); Pushed.Broken = Now.Broken; }
	if (Moved(Now.Wide, Pushed.Wide)) { Push(WideAims, Now.Wide, At); Pushed.Wide = Now.Wide; }
	if (Moved(Now.Far, Pushed.Far)) { Push(FarAims, Now.Far, At); Pushed.Far = Now.Far; }
	// the room: there while FAR is up or a stinger rings in it, and for its tail after
	if (Now.Far > 0 || At < RoomUntil) { RoomIdleFor = 0; Room(true); }
	else if (bRoom) { RoomIdleFor += Dt; if (RoomIdleFor > Tail) Room(false); }
}

void FS47SidebandBus::StandBack(double InLevel)
{
	const double V = FMath::IsFinite(InLevel) ? FMath::Clamp(InLevel, 0., 1.) : 1.;
	if (FMath::Abs(V - StandNow) < 1e-4) return;
	StandNow = V;
	FS47AudioContext::FBatch B(*Ctx);
	Head->Gain.SetTargetAtTime(V, Ctx->CurrentTime(), .25);
}

void FS47SidebandBus::NoiseOpen(bool bOpen)
{
	if (bOpen == bNoise) return;
	bNoise = bOpen;
	if (!bBuilt) return;
	FS47AudioContext::FBatch B(*Ctx);
	const double At = Ctx->CurrentTime();
	WornNoise->Gain.SetTargetAtTime(bOpen ? 1. : 0., At, bOpen ? .6 : .12);
	BrokenNoise->Gain.SetTargetAtTime(bOpen ? 1. : 0., At, bOpen ? .6 : .12);
}

void FS47SidebandBus::Hold(bool bInHeld)
{
	if (bInHeld == bHeld) return;
	bHeld = bInHeld;
	if (!bBuilt) return;
	FS47AudioContext::FBatch B(*Ctx);
	Return->Gain.SetTargetAtTime(bHeld ? 0. : 1., Ctx->CurrentTime(), bHeld ? .1 : .5);
}

void FS47SidebandBus::FarTrim(double InDb)
{
	InDb = FMath::IsFinite(InDb) ? FMath::Clamp(InDb, -24., 6.) : -6.5;
	if (FMath::Abs(InDb - TrimDb) < .05) return;
	TrimDb = InDb;
	Pushed.Far = -1; // the send's level is the trim's too
}

void FS47SidebandBus::Hit(double When, bool bBurst)
{
	using namespace S47SbImpl;
	if (!bEngaged || LetGoAt >= 0) Engage();
	FS47AudioContext::FBatch B(*Ctx);
	const double T0 = FMath::Max(When, Ctx->CurrentTime());
	// the push: every one of BROKEN's parameters to where it stands at full in 8 ms, held a quarter second, then back (tau 0.25)
	for (FAim& A : BrokenAims)
	{
		const double V0 = A.P->ValueAt(T0);
		A.P->CancelScheduledValues(T0); A.P->SetValueAtTime(V0, T0);
		A.P->LinearRampToValueAtTime(A.Fn(1.), T0 + .008);
		A.P->SetTargetAtTime(A.Fn(Now.Broken), T0 + .25, .25);
	}
	HitUntil = T0 + 1.25; Pushed.Broken = -1;
	const auto Burst = [T0](FS47AudioParam& P, double Rest, double Top, double Rise, double Stay, double Tau)
	{
		P.CancelScheduledValues(T0); P.SetValueAtTime(Rest, T0); P.LinearRampToValueAtTime(Top, T0 + Rise); P.SetTargetAtTime(Rest, T0 + Stay, Tau);
	};
	if (bBurst)
	{
		Burst(HitDrive->Gain, 1., SbDb(10.), .003, .11, .04);
		Burst(HitLevel->Gain, 1., SbDb(3.), .003, .11, .04);
	}
	Burst(HitStatic->Gain, 0., StaticK * SbDb(-24.), .003, .05, .07);
	HitHole->Gain.CancelScheduledValues(T0); HitHole->Gain.SetValueAtTime(1., T0);
	HitHole->Gain.SetValueCurveAtTime(HoleCurve, T0 + .11, .16);
}

namespace S47SbImpl
{
	// SIDEBAND-20261113 (d69: the duck was -10.4/-9.9 dB at 2-4/4-6 s against the prototype's -12.6/-16.5): the prototype's curves
	// (colour2/s47dsp.py env: a smoothstep glide between points). The score's Gain glides in dB (a linear ramp in gain came back
	// from -36 dB to -6 dB in the first half of the four seconds); the bed's in gain, as the prototype's "on" curve is.
	TArray<float> SbGlide(double From, double To, bool bInDb, int32 Points = 129)
	{
		TArray<float> C; C.SetNumUninitialized(Points);
		for (int32 I = 0; I < Points; ++I)
		{
			double U = double(I) / (Points - 1); U = U * U * (3. - 2. * U);
			C[I] = float(bInDb ? SbDb(From + (To - From) * U) : From + (To - From) * U);
		}
		return C;
	}
}

double S47Sideband::DuckDbAt(double T)
{
	// the prototype's duck_points: 0.2 s of music under the hit, down 36 dB in 0.1 s, held 3 s, back over 4.2 s (dB, smoothstep)
	const auto Glide = [](double U) { U = FMath::Clamp(U, 0., 1.); return U * U * (3. - 2. * U); };
	if (T < .2) return 0;
	if (T < .3) return -36. * Glide((T - .2) / .1);
	if (T < 3.3) return -36.;
	return -36. * (1. - Glide((T - 3.3) / 4.2));
}

void FS47SidebandBus::MomentDuck(double When)
{
	using namespace S47SbImpl;
	FS47AudioContext::FBatch B(*Ctx);
	const double T0 = FMath::Max(When, Ctx->CurrentTime());
	FS47AudioParam& P = Moment->Gain;
	const double Low = SbDb(-36.);
	P.CancelScheduledValues(T0); P.SetValueAtTime(P.ValueAt(T0), T0);
	P.SetValueAtTime(1., T0 + .2); P.SetValueCurveAtTime(SbGlide(0., -36., true, 33), T0 + .2, .1);
	P.SetValueAtTime(Low, T0 + 3.3); P.SetValueCurveAtTime(SbGlide(-36., 0., true), T0 + 3.3, 4.2);
}

// ---------------------------------------------------------------- FLARES' voices
bool FS47SidebandBus::LoadFlares()
{
	if (!bFlaresTried)
	{
		bFlaresTried = true;
		const TArray<S47Sideband::FStinger>& All = S47Sideband::Stingers();
		StingerBuf.SetNum(All.Num()); BedBuf.SetNum(S47Sideband::Beds().Num());
		for (int32 I = 0; I < All.Num(); ++I) StingerBuf[I] = S47SbImpl::LoadWav(All[I].Name);
		for (int32 I = 0; I < BedBuf.Num(); ++I) BedBuf[I] = S47SbImpl::LoadWav(S47Sideband::Beds()[I]);
		UE_LOG(LogS47Sideband, Display, TEXT("S47: FLARES has %d of %d baked sounds (Content/Audio/Raw/media/sideband)"), FlaresLoaded(), All.Num() + BedBuf.Num());
	}
	return FlaresLoaded() > 0;
}

int32 FS47SidebandBus::FlaresLoaded() const
{
	int32 N = 0;
	for (const auto& B : StingerBuf) N += B.IsValid();
	for (const auto& B : BedBuf) N += B.IsValid();
	return N;
}

void FS47SidebandBus::TestFlares(const TSharedPtr<FS47AudioBuffer>& InStinger, const TSharedPtr<FS47AudioBuffer>& InBed)
{
	bFlaresTried = true;
	StingerBuf.Init(InStinger, S47Sideband::Stingers().Num());
	BedBuf.Init(InBed, S47Sideband::Beds().Num());
}

// bed: BufferSource(loop) -> highpass 40 -> -7 dB at 230 Hz (where the score's own pads sit) -> lowpass 200 -> 1000 Hz with
// nearness -> -8 dB at BROKEN's own 1.5 kHz -> Gain (nearness, and the event's fade) -> the moment's duck -> after BROKEN.
void FS47SidebandBus::Bed(int32 Index, double Near, bool bOn)
{
	using namespace S47SbImpl;
	Near = FMath::IsFinite(Near) ? SbSat(Near) : 0.;
	if (!bOn || !HasBed(Index))
	{
		if (BedSrc && bBedOn)
		{
			FS47AudioContext::FBatch B(*Ctx);
			const double At = Ctx->CurrentTime();
			BedGain->Gain.CancelScheduledValues(At); BedGain->Gain.SetTargetAtTime(0., At, 3.); // out over 8 to 10 s
			bBedOn = false; BedOffFor = 0;
		}
		return;
	}
	if (BedSrc && BedIndex != Index) StopBed(true); // REVIEW-20261113: the old bed fades out, no click
	if (!bEngaged || LetGoAt >= 0) Engage();
	double LevelDb = 0, Cut = 0; S47Sideband::BedFor(Near, LevelDb, Cut);
	FS47AudioContext::FBatch B(*Ctx);
	const double At = Ctx->CurrentTime();
	if (!BedSrc)
	{
		const TSharedPtr<FS47AudioBuffer>& Buf = BedBuf[Index];
		BedSrc = Ctx->CreateBufferSource(); BedSrc->Buffer = Buf; BedSrc->bLoop = true;
		auto Hp = Ctx->CreateBiquadFilter(); Hp->SetType(TEXT("highpass")); Hp->Frequency.SetValue(40.); Hp->Q.SetValue(-3.);
		auto Mud = Ctx->CreateBiquadFilter(); Mud->SetType(TEXT("peaking")); Mud->Frequency.SetValue(230.); Mud->Q.SetValue(.7); Mud->Gain.SetValue(-7.);
		BedLp = Ctx->CreateBiquadFilter(); BedLp->SetType(TEXT("lowpass")); BedLp->Frequency.SetValue(Cut); BedLp->Q.SetValue(-3.);
		auto Notch = Ctx->CreateBiquadFilter(); Notch->SetType(TEXT("peaking")); Notch->Frequency.SetValue(1500.); Notch->Q.SetValue(1.); Notch->Gain.SetValue(-8.);
		BedGain = Ctx->CreateGain(); BedGain->Gain.SetValue(0.);
		BedDuck = Ctx->CreateGain();
		BedSrc->Connect(Hp); Hp->Connect(Mud); Mud->Connect(BedLp); BedLp->Connect(Notch); Notch->Connect(BedGain); BedGain->Connect(BedDuck); BedDuck->Connect(FlareSum);
		// not from the top each time
		BedSrc->Start(At, FMath::FRandRange(0., FMath::Max(0., Buf->Duration() - .1)));
		BedIndex = Index; BedNow = -1; bBedOn = false;
	}
	if (!bBedOn || FMath::Abs(Near - BedNow) > .01)
	{
		// in over about 4 s; nearness itself glides 2 to 4 s
		BedGain->Gain.CancelScheduledValues(At); BedGain->Gain.SetTargetAtTime(SbDb(LevelDb), At, bBedOn ? .9 : 1.3);
		BedLp->Frequency.CancelScheduledValues(At); BedLp->Frequency.SetTargetAtTime(Cut, At, .9);
		BedNow = Near; bBedOn = true;
	}
}

void FS47SidebandBus::BedMoment(double When)
{
	if (!BedSrc) return;
	FS47AudioContext::FBatch B(*Ctx);
	const double T0 = FMath::Max(When, Ctx->CurrentTime());
	FS47AudioParam& P = BedDuck->Gain;
	P.CancelScheduledValues(T0); P.SetValueAtTime(P.ValueAt(T0), T0);
	// SIDEBAND-20261113: the prototype's "on" curve after the moment (make_flares.py: 1 at +0.2, 0.1 at +0.6, held to +3.3, 0.55 at
	// +6.0, out by +13), each leg a smoothstep glide; the old 0.55 at +7.5 in straight lines was the engine's own
	P.SetValueAtTime(1., T0 + .2); P.SetValueCurveAtTime(S47SbImpl::SbGlide(1., .1, false, 33), T0 + .2, .4);
	P.SetValueAtTime(.1, T0 + 3.3); P.SetValueCurveAtTime(S47SbImpl::SbGlide(.1, .55, false, 65), T0 + 3.3, 2.7);
	P.SetValueCurveAtTime(S47SbImpl::SbGlide(.55, 0., false), T0 + 6., 7.);
}

void FS47SidebandBus::StopBed(bool bFade)
{
	if (!BedSrc) return;
	FS47AudioContext::FBatch B(*Ctx);
	if (bFade && BedDuck)
	{
		const double At = Ctx->CurrentTime();
		FS47AudioParam& P = BedDuck->Gain;
		P.CancelScheduledValues(At); P.SetValueAtTime(P.ValueAt(At), At); P.LinearRampToValueAtTime(0., At + .05);
		Ctx->SetTimeout(.07, [Src = BedSrc, Duck = BedDuck] { Src->Stop(); Src->Disconnect(); if (Duck) Duck->Disconnect(); });
		BedSrc.Reset(); BedGain.Reset(); BedDuck.Reset(); BedLp.Reset();
		BedIndex = -1; bBedOn = false; BedNow = 0;
		return;
	}
	BedSrc->Stop(); BedSrc->Disconnect();
	if (BedDuck) BedDuck->Disconnect();
	BedSrc.Reset(); BedGain.Reset(); BedDuck.Reset(); BedLp.Reset();
	BedIndex = -1; bBedOn = false; BedNow = 0;
}

// stinger: BufferSource -> Gain (the tier's level and the stinger's trim) -> two lowpasses (the tier's) -> [direct -> after BROKEN
// | one channel -> send -> FAR's room, after FAR's own send]
bool FS47SidebandBus::Stinger(int32 Index, S47Sideband::ETier Tier, bool bSoft, double When)
{
	using namespace S47SbImpl;
	if (!HasStinger(Index) || StingSrc) return false;
	if (!bEngaged || LetGoAt >= 0) Engage();
	const S47Sideband::FTierMix M = S47Sideband::TierMix(Tier, bSoft, S47Sideband::Stingers()[Index].TrimDb);
	FS47AudioContext::FBatch B(*Ctx);
	const double T0 = FMath::Max(When, Ctx->CurrentTime());
	const TSharedPtr<FS47AudioBuffer>& Buf = StingerBuf[Index];
	StingSrc = Ctx->CreateBufferSource(); StingSrc->Buffer = Buf;
	StingGain = Ctx->CreateGain();
	// the front: the prototype's raised cosine (s47flares.py shape(): sin² over 3 ms; the soft mode's over 120 ms). The gain used to
	// stand at its level from the first sample, so a buffer that does not start at zero struck (click ratio 11.7 against 7.0).
	StingGain->Gain.SetValueAtTime(0., T0);
	{
		TArray<float> Front; Front.SetNumUninitialized(33);
		for (int32 I = 0; I < 33; ++I) Front[I] = float(SbDb(M.LevelDb) * FMath::Square(FMath::Sin(SbPi * .5 * I / 32.)));
		StingGain->Gain.SetValueCurveAtTime(Front, T0, bSoft ? .12 : StingerFront);
	}
	auto Lp1 = Ctx->CreateBiquadFilter(), Lp2 = Ctx->CreateBiquadFilter();
	for (const TSharedRef<FS47BiquadFilterNode>& Lp : { Lp1, Lp2 }) { Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(M.Lowpass); Lp->Q.SetValue(-3.); }
	auto Direct = Ctx->CreateGain(); Direct->Gain.SetValue(M.Direct);
	StingSrc->Connect(StingGain); StingGain->Connect(Lp1); Lp1->Connect(Lp2); Lp2->Connect(Direct); Direct->Connect(FlareSum);
	StingNodes = { Lp1, Lp2, Direct };
	if (M.Send > 0)
	{
		auto MonoG = Ctx->CreateGain(); MonoG->Gain.SetValue(0.);
		Ones->Connect(MonoG); Lp2->Connect(MonoG->Gain);
		auto Send = Ctx->CreateGain(); Send->Gain.SetValue(M.Send);
		MonoG->Connect(Send); Send->Connect(RoomIn);
		StingNodes.Add(MonoG); StingNodes.Add(Send);
		RoomUntil = FMath::Max(RoomUntil, T0 + Buf->Duration() + 8.); // the room stays for its tail
	}
	StingSrc->Start(T0);
	StingEnd = T0 + Buf->Duration() + .1;
	return true;
}

bool FS47SidebandBus::StingerSounding() const { return StingSrc.IsValid(); }

void FS47SidebandBus::EndStinger()
{
	if (!StingSrc) return;
	FS47AudioContext::FBatch B(*Ctx);
	StingSrc->Stop(); StingSrc->Disconnect();
	if (StingGain) StingGain->Disconnect();
	for (const TSharedPtr<FS47AudioNode>& N : StingNodes) if (N) N->Disconnect();
	// (the mono fold's Gain is fed by Ones: let go of that link too)
	if (Ones) for (const TSharedPtr<FS47AudioNode>& N : StingNodes) Ones->Disconnect(N);
	StingNodes.Reset(); StingSrc.Reset(); StingGain.Reset();
}
