#include "S47RxLook.h"
#include "S47Script.h"

// RX-LOOK-20261113 (-> M3, M6): see S47RxLook.h.
S47RxLook::FState S47RxLook::StateOf(const FStateIn& In)
{
	// The order is FS47FreeformSession::ReceiverNextAction's: the banner never says more than the receiver's own line does.
	auto Say = [](EState Id, const FString& Text, bool bAmber) { FState S; S.Id = Id; S.Text = Text; S.bAmber = bAmber; return S; };
	if (!In.bJob) return Say(EState::NoJob, TEXT("NO JOB"), false);
	if (!In.bPower) return Say(EState::NoPower, TEXT("NO POWER"), false);
	if (In.bNotUp && In.Stage <= 1) return Say(EState::NotUp, TEXT("SOURCE NOT UP"), false);
	if (In.Stage <= 0) return Say(EState::Point, TEXT("POINT THE ARRAY"), false);
	if (In.Stage == 1)
	{
		if (In.bBlocked) return Say(EState::Blocked, TEXT("ARRAY BLOCKED"), false);
		if (!In.bPointReady) return Say(EState::Slewing, TEXT("ARRAY SLEWING"), false);
		if (!In.bSettled) return Say(EState::Settling, TEXT("ARRAY SETTLING"), false);
		if (!In.bSky) return In.bSkyLocking ? Say(EState::TakingFix, TEXT("TAKING THE FIX"), true) : Say(EState::Searching, TEXT("SEARCHING"), false);
		if (!In.bTuned) return Say(EState::FixHeld, TEXT("FIX HELD"), true);
		return In.bAnomalyAhead ? Say(EState::Anomaly, TEXT("ANOMALY DETECTED"), true) : Say(EState::SourceLock, TEXT("SOURCE LOCK"), true);
	}
	if (In.Stage == 2)
	{
		if (In.bBlocked || !In.bTuned || !In.bPointReady || !In.bSettled) return Say(EState::RecordingHeld, TEXT("RECORDING HELD"), false);
		return Say(EState::Recording, FString::Printf(TEXT("RECORDING %s/%s s"), *Num(FMath::Clamp(int32(In.Elapsed), 0, 999)), *Num(FMath::Clamp(FMath::RoundToInt32(In.Seconds), 0, 999))), true);
	}
	if (In.Stage <= 4)
	{
		if (In.bReference) return Say(EState::Reference, FString::Printf(TEXT("REFERENCE %s/8 s"), *Num(FMath::Clamp(int32(In.ReferenceAt), 0, 8))), true);
		if (In.bPlainOnTape) return Say(EState::Anomaly, TEXT("ANOMALY DETECTED"), true);
		return Say(EState::OnTape, TEXT("TAKE ON TAPE"), false);
	}
	return Say(EState::Done, TEXT("TAKE REGISTERED"), false);
}

FString S47RxLook::ClockLine(const FString& DateAndClock)
{
	// RX-LOOK gjennomgang 14 Nov: the header's clock showed xx:47 one minute every hour
	const int32 Len = DateAndClock.Len();
	auto Digit = [&](int32 At) { return At >= 0 && At < Len && FChar::IsDigit(DateAndClock[At]); };
	if (Len < 5 || DateAndClock[Len - 3] != TEXT(':') || !Digit(Len - 5) || !Digit(Len - 4) || !Digit(Len - 2) || !Digit(Len - 1) || Digit(Len - 6)) return DateAndClock;
	const int32 Hours = FCString::Atoi(*DateAndClock.Mid(Len - 5, 2)), Minutes = FCString::Atoi(*DateAndClock.Mid(Len - 2, 2));
	return DateAndClock.Left(Len - 5) + S47Script::PlayerClock(double(Hours * 3600 + Minutes * 60));
}

FString S47RxLook::TextFault(const FString& Shown)
{
	if (S47Script::SaysFortySeven(Shown)) return TEXT("a number that reads 47");
	const FString Lint = S47Script::LintText(Shown);
	if (!Lint.IsEmpty()) return Lint;
	const FString Low = Shown.ToLower();
	if (Low.Contains(TEXT("the cable"))) return TEXT("the cable is Story's");
	for (int32 At = Low.Find(TEXT("taken")); At != INDEX_NONE; At = Low.Find(TEXT("taken"), ESearchCase::CaseSensitive, ESearchDir::FromStart, At + 1))
	{
		const bool bOpenBefore = At == 0 || !FChar::IsAlpha(Low[At - 1]), bOpenAfter = At + 5 >= Low.Len() || !FChar::IsAlpha(Low[At + 5]);
		if (bOpenBefore && bOpenAfter) return TEXT("the word taken is not Freeform's");
	}
	return FString();
}

TArray<float> S47RxLook::MovingPicture(const FS47RxLive& Live, double T)
{
	TArray<float> Out;
	const bool bCalm = Live.bCalm;
	for (int32 I = 0; I <= 48; ++I)
	{
		const float U = I / 48.f;
		float InPhase = 0, Quad = 0;
		Quadrature(Live.Scope, U, T, bCalm, InPhase, Quad);
		Out.Add(FftLevel(Live.Scope, U, T, bCalm)); Out.Add(InPhase); Out.Add(Quad);
		Out.Add(Jitter(T, float(I * 9), bCalm));
	}
	for (int32 Star = 0; Star < 24; ++Star) Out.Add(Twinkle(Star, T, bCalm));
	float Grow = 0, Alpha = 0;
	Out.Add(float(Sweep(T, bCalm))); Out.Add(Breath(T, 1.1, bCalm)); Out.Add(float(Typed(16, FMath::Fmod(T, 1.), bCalm)));
	Out.Add(Bloom(FMath::Fmod(T, 1.), bCalm)); Out.Add(Pulse(FMath::Fmod(T, 2.), 0, bCalm, Grow, Alpha) ? Grow + Alpha : -1.f);
	Out.Add(TearShift(T, 1.f, bCalm)); Out.Add(TearDark(T, 1.f, bCalm));
	return Out;
}

const FS47RxLive& FS47RxFeed::Now()
{
	if (Frame != GFrameCounter || !Read)
	{
		Frame = GFrameCounter;
		if (Read) Value = Read();
	}
	return Value;
}

bool S47RxLook::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bPass, const FString& What) { bOk &= bPass; Lines.Add(FString(bPass ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("receiver look: ") + What); };

	{ // the banner's words follow the state, and say no more than the receiver's own line
		FStateIn In;
		const bool bNone = StateOf(In).Id == EState::NoJob && !StateOf(In).bAmber;
		In.bJob = true; In.bPower = false; const bool bDark = StateOf(In).Id == EState::NoPower;
		In.bPower = true; In.Stage = 0; In.bNotUp = true; const bool bEarly = StateOf(In).Text == TEXT("SOURCE NOT UP");
		In.bNotUp = false; const bool bPoint = bEarly && StateOf(In).Text == TEXT("POINT THE ARRAY");
		In.Stage = 1; const bool bSlew = StateOf(In).Id == EState::Slewing;
		In.bBlocked = true; const bool bBlocked = StateOf(In).Id == EState::Blocked; In.bBlocked = false;
		In.bPointReady = true; const bool bSettle = StateOf(In).Id == EState::Settling;
		In.bSettled = true; const bool bSearch = StateOf(In).Text == TEXT("SEARCHING") && !StateOf(In).bAmber;
		In.bSkyLocking = true; const bool bTaking = StateOf(In).Id == EState::TakingFix;
		// tuned, and an unusual take ahead, but no fix yet: still the search (ReceiverReady waits for the fix, so the screens say nothing)
		In.bSkyLocking = false; In.bTuned = true; In.bAnomalyAhead = true; const bool bNotYet = StateOf(In).Id == EState::Searching;
		In.bSky = true; In.bTuned = false; const bool bFix = StateOf(In).Text == TEXT("FIX HELD") && StateOf(In).bAmber;
		In.bAnomalyAhead = true; const bool bUntuned = StateOf(In).Id == EState::FixHeld; // not tuned: nothing of the anomaly yet
		In.bTuned = true; const bool bOdd = StateOf(In).Text == TEXT("ANOMALY DETECTED");
		In.bAnomalyAhead = false; const bool bLock = StateOf(In).Text == TEXT("SOURCE LOCK") && StateOf(In).bAmber;
		In.Stage = 2; In.Elapsed = 7.6; In.Seconds = 18; const bool bRec = StateOf(In).Text == TEXT("RECORDING 7/18 s");
		In.bTuned = false; const bool bHeld = StateOf(In).Id == EState::RecordingHeld;
		In.bTuned = true; In.Stage = 4; const bool bTape = StateOf(In).Id == EState::OnTape;
		In.bPlainOnTape = true; const bool bStill = StateOf(In).Id == EState::Anomaly;
		In.bReference = true; In.ReferenceAt = 3.2; const bool bRef = StateOf(In).Text == TEXT("REFERENCE 3/8 s");
		In.Stage = 6; const bool bDone = StateOf(In).Id == EState::Done;
		Check(bNone && bDark && bPoint && bSlew && bBlocked && bSettle && bSearch && bTaking && bFix && bLock && bRec && bHeld && bTape && bRef && bDone,
			TEXT("the banner: NO JOB, NO POWER, POINT THE ARRAY, ARRAY SLEWING, SETTLING, SEARCHING, TAKING THE FIX, FIX HELD, SOURCE LOCK, RECORDING n/N s, REFERENCE n/8 s, each from its own state"));
		Check(bNotYet && bUntuned && bOdd && bStill, TEXT("the banner says ANOMALY DETECTED only where the receiver's own line does: fix held and tuned, or the take on tape before the reel deck has heard it"));
	}
	{ // RX-LOOK gjennomgang 14 Nov: with Calm flashes the receiver's moving picture is the same at every moment (the receiver test
		// compares two frames by it); without them it moves
		FS47RxLive Calm; Calm.bCalm = true; Calm.Scope.bPowered = true; Calm.Scope.Strength = .3; Calm.Scope.Tuned = Calm.Scope.Card + .01;
		FS47RxLive Moving = Calm; Moving.bCalm = false;
		const TArray<float> First = MovingPicture(Calm, 100.), Later = MovingPicture(Calm, 100.37), Much = MovingPicture(Calm, 2031.9);
		Check(First.Num() > 100 && First == Later && First == Much && MovingPicture(Moving, 100.) != MovingPicture(Moving, 100.37),
			FString::Printf(TEXT("with Calm flashes the receiver's moving picture (%d numbers: the spectrum, I and Q, the sweep, the stars, the slips, the jump, the breath, the letters) is the same frame after frame; without them it moves"), First.Num()));
	}
	{ // the line and its wavelength
		Check(FMath::Abs(WavelengthCm(1420.405) - 21.106) < .001 && FMath::Abs(WavelengthCm(1419.6) - 21.118) < .001 && WavelengthCm(0) == 0 && OnHydrogenLine(1420.38) && !OnHydrogenLine(1419.9) && FString(LineName(1419.6)) == TEXT("CONT"),
			TEXT("1420.405 MHz is the hydrogen line at 21.106 cm; the continuum cards are named as continuum"));
	}
	{ // the effects: gentle, and still when calm
		bool bStill = true, bMoves = false;
		float Grow = 0, Alpha = 0;
		for (int32 I = 0; I < 600; ++I)
		{
			const double T = 300. + I * .037;
			bStill &= Twinkle(I % 40, T, true) == 1.f && Sweep(T, true) < 0 && Bloom(I * .004, true) == 0.f && !Pulse(I * .004, I % 3, true, Grow, Alpha) && Breath(T, 1.1, true) == 1.f && Typed(16, I * .01, true) == 16 && Jitter(T, float(I % 400), true) == 0.f;
			bMoves |= Twinkle(3, T, false) < .95f;
		}
		Check(bStill && bMoves && Sweep(3.5, false) == .5, TEXT("with Calm flashes nothing twinkles, sweeps, blooms, pulses, types or jitters; without it they do"));
		// the lock's one pulse: up once and down once, never more than three changes of direction a second
		int32 Turns = 0, Dir = 0; float Was = 0, Top = 0, Step = 0;
		for (int32 I = 1; I <= 1200; ++I)
		{
			const float B = Bloom(I / 120. - .5, false);
			Top = FMath::Max(Top, B); Step = FMath::Max(Step, FMath::Abs(B - Was));
			const int32 D = B > Was ? 1 : B < Was ? -1 : Dir;
			if (Dir != 0 && D != 0 && D != Dir) ++Turns;
			if (D != 0) Dir = D;
			Was = B;
		}
		Check(Turns == 1 && Top > .98f && Top <= 1.f && Step < .05f && Bloom(-1., false) == 0.f && Bloom(BloomSeconds + .01, false) == 0.f,
			FString::Printf(TEXT("the picture's pulse at the lock is one soft swell of %.1f s (largest step %.3f at 120 Hz), not a flash"), BloomSeconds, Step));
		bool bRings = true; int32 Seen = 0;
		for (int32 Ring = 0; Ring < 3; ++Ring) for (int32 I = 0; I < 400; ++I)
		{
			if (!Pulse(I * .01, Ring, false, Grow, Alpha)) continue;
			++Seen; bRings &= Grow >= 0 && Grow <= 1 && Alpha >= 0 && Alpha <= 1;
		}
		bool bTyped = Typed(16, -1., false) == 16 && Typed(16, 0., false) == 1 && Typed(16, 5., false) == 16;
		for (int32 I = 1; I < 100; ++I) bTyped &= Typed(16, I * .01, false) >= Typed(16, (I - 1) * .01, false);
		int32 Slips = 0;
		for (int32 I = 0; I < 6000; ++I) Slips += Jitter(I * .01, 200.f, false) > 0 ? 1 : 0;
		Check(bRings && Seen > 300 && bTyped && Slips < 300 && Beam(2.f, 1.f) > Beam(2.f, 0.f) && Beam(2.f, 1.f) < 2.6f,
			FString::Printf(TEXT("three rings go out and fade, the banner's letters come one after the other, a band of the picture slips a unit for %d hundredths of a second in a minute, and a brighter beam is a little wider"), Slips));
	}
	{ // the scopes: the same picture for the same numbers, and the line where the dial puts it
		FScope On; On.bPowered = true; On.Tuned = On.Card = 1419.9; On.Strength = 1.; On.Seed = 4711u;
		FScope Off = On; Off.Strength = 0.;
		FScope Low = On; Low.Tuned = On.Card + .05; Low.Strength = .4;
		auto A0 = [](const FScope& In) { return FallRow(In, 0u, false); };
		bool bSame = true, bOther = false, bWhole = true;
		double LineAt = 0, Lit = 0, Dark = 0;
		int32 RunCount = 0, RunsLeast = FallRunsMost, RunsMost = 0;
		for (uint32 Row = 0; Row < 60; ++Row)
		{
			const FFallRow A = FallRow(On, Row, false), B = FallRow(Off, Row, false), C = FallRow(Low, Row, false);
			FFallRun RunsA[FallRunsMost], RunsAgain[FallRunsMost], RunsOther[FallRunsMost];
			const int32 CountA = FallRuns(A, On.Seed, RunsA), CountAgain = FallRuns(FallRow(On, Row, false), On.Seed, RunsAgain), CountOther = FallRuns(A, On.Seed + 1u, RunsOther);
			bSame &= CountA == CountAgain;
			bOther |= CountA != CountOther;
			// a row's runs stand end to end from the left edge to the right, each a little of the window long
			bWhole &= CountA >= 10 && CountA <= FallRunsMost && RunsA[CountA - 1].End == 1.f;
			float From = 0.f;
			for (int32 I = 0; I < CountA; ++I)
			{
				bSame &= CountA == CountAgain && RunsA[I].End == RunsAgain[I].End && RunsA[I].Level == RunsAgain[I].Level;
				bOther |= I < CountOther && RunsA[I].Level != RunsOther[I].Level;
				bWhole &= RunsA[I].End > From && RunsA[I].End - From < .12f && RunsA[I].Level > 0.f && RunsA[I].Level < .6f;
				From = RunsA[I].End;
			}
			RunCount += CountA; RunsLeast = FMath::Min(RunsLeast, CountA); RunsMost = FMath::Max(RunsMost, CountA);
			// the line: a bright core and two wider, fainter steps when the source is heard; nothing without it
			float Half[FallLineSteps], Level[FallLineSteps], NoHalf = 0, NoLevel = 0;
			bool bLine = true;
			for (int32 Step = 0; Step < FallLineSteps; ++Step) bLine &= FallLine(A, On.Seed, Step, Half[Step], Level[Step]);
			bWhole &= bLine && Half[0] < Half[1] && Half[1] < Half[2] && Half[2] < .06f && Level[0] > Level[1] && Level[1] > Level[2] && Level[0] > .75f && Level[2] > .05f;
			Lit += Level[0];
			Dark += FallLine(B, On.Seed, 0, NoHalf, NoLevel) ? 1. : 0.;
			LineAt += C.Line;
		}
		const FFallRow Tick = FallRow(On, 9u, true), Plain = FallRow(On, 9u, false);
		float TickHalf = 0, TickLevel = 0, PlainHalf = 0, PlainLevel = 0, EdgeHalf = 0, EdgeLevel = 0;
		const bool bTick = FallTick(Tick, TickHalf, TickLevel) && TickHalf > .08f && TickLevel > .5f && !FallTick(Plain, PlainHalf, PlainLevel);
		FFallRun Unlit[FallRunsMost];
		Check(bSame && bOther && bWhole && Lit > 45. && Dark == 0. && FMath::Abs(LineAt / 60. - (.5 - .05 / (2. * HalfSpan(On.FTol)))) < .02
			&& bTick && FallRuns(FallRow(FScope(), 3u, false), 1u, Unlit) == 0 && !FallLine(FallRow(FScope(), 3u, false), 1u, 0, EdgeHalf, EdgeLevel) && !FallLine(A0(On), On.Seed, FallLineSteps, EdgeHalf, EdgeLevel),
			FString::Printf(TEXT("the waterfall is the same for the same seed and row: a row is %d to %d runs of grain end to end (%.1f a row), the line a bright core with two fainter steps when the dial is on the card, off to the side when it is off, gone with no lock; a tick is a short wide dash"), RunsLeast, RunsMost, RunCount / 60.));
		// the spectrum: a peak over the floor at the card's line, none without the source; the dial's mark follows the dial
		const float PeakOn = FftLevel(On, .5f, 12.3, false), FloorOn = FftLevel(On, .1f, 12.3, false), PeakOff = FftLevel(Off, .5f, 12.3, false);
		// RX-LOOK gjennomgang 14 Nov: with Calm flashes the trace stands at every moment; without it, it moves
		bool bFftStill = true, bFftMoves = false;
		for (int32 I = 0; I <= 60; ++I) for (const double At : { 3.1, 47.9, 301.25 })
		{
			bFftStill &= FftLevel(On, I / 60.f, At, true) == FftLevel(On, I / 60.f, 12.3, true);
			bFftMoves |= FftLevel(On, I / 60.f, At, false) != FftLevel(On, I / 60.f, 12.3, false);
		}
		Check(PeakOn > FloorOn + .3f && PeakOff < FloorOn + .25f && FftLevel(On, .5f, 12.3, false) == PeakOn && FftLevel(FScope(), .5f, 1., false) == 0.f && FMath::Abs(FftTuned(On) - .5f) < 1e-6f && FftTuned(Low) > .5f && bFftStill && bFftMoves,
			TEXT("the small spectrum: a peak at the card's line when the source is heard, the floor without it, the dial's mark where the dial is; with Calm flashes the trace stands"));
		// RX-LOOK gjennomgang 14 Nov: the whole picture's jump and its dark beat on interference: none with Calm flashes, never more
		// than three a second in any second, and still there without it
		int32 MostTears = 0, MostDarks = 0, Tears = 0, Darks = 0;
		bool bCalmTear = true;
		for (const float Jam : { .2f, .5f, 1.f })
		{
			TArray<double> TearAt, DarkAt;
			float TearWas = 0.f, DarkWas = 0.f;
			for (int32 I = 0; I < 60 * 480; ++I)
			{
				const double T = 1000. + I / 480.;
				const float Tear = TearShift(T, Jam, false), Dark = TearDark(T, Jam, false);
				if (Tear != 0.f && Tear != TearWas) TearAt.Add(T);
				if (Dark != 0.f && DarkWas == 0.f) DarkAt.Add(T);
				TearWas = Tear; DarkWas = Dark;
				bCalmTear &= TearShift(T, Jam, true) == 0.f && TearDark(T, Jam, true) == 0.f;
			}
			for (int32 I = 0; I < TearAt.Num(); ++I) { int32 InSecond = 0; for (int32 K = I; K < TearAt.Num() && TearAt[K] < TearAt[I] + 1.; ++K) ++InSecond; MostTears = FMath::Max(MostTears, InSecond); }
			for (int32 I = 0; I < DarkAt.Num(); ++I) { int32 InSecond = 0; for (int32 K = I; K < DarkAt.Num() && DarkAt[K] < DarkAt[I] + 1.; ++K) ++InSecond; MostDarks = FMath::Max(MostDarks, InSecond); }
			Tears += TearAt.Num(); Darks += DarkAt.Num();
		}
		Check(bCalmTear && MostTears <= 3 && MostDarks <= 3 && Tears > 20 && Darks > 20 && TearShift(5., .1f, false) == 0.f,
			FString::Printf(TEXT("on interference the whole picture jumps (%d times in three minutes) and darkens (%d), at most %d and %d times in any second, never with Calm flashes"), Tears, Darks, MostTears, MostDarks));
		// I and Q: locked, two clean traces a quarter turn apart (the two squared sum to one size); unlocked, noise
		double Least = 1e9, Most = 0, Wild = 0; bool bQuad = true;
		for (int32 I = 0; I <= 200; ++I)
		{
			float Ti = 0, Tq = 0, Ni = 0, Nq = 0, Ai = 0, Aq = 0;
			Quadrature(On, I / 200.f, 33.3, false, Ti, Tq); Quadrature(On, I / 200.f, 33.3, false, Ai, Aq); Quadrature(Off, I / 200.f, 33.3, false, Ni, Nq);
			bQuad &= Ti == Ai && Tq == Aq && FMath::Abs(Ti) <= 1 && FMath::Abs(Tq) <= 1;
			const double Size = FMath::Sqrt(double(Ti) * Ti + double(Tq) * Tq);
			Least = FMath::Min(Least, Size); Most = FMath::Max(Most, Size); Wild = FMath::Max(Wild, double(FMath::Max(FMath::Abs(Ni), FMath::Abs(Nq))));
		}
		float CalmA = 0, CalmB = 0, CalmC = 0, CalmD = 0;
		Quadrature(On, .3f, 10., true, CalmA, CalmB); Quadrature(On, .3f, 99., true, CalmC, CalmD);
		// round 2: a locked carrier is not a drawn sine: a little noise rides on it, never much
		double Ride = 0;
		for (int32 I = 0; I <= 200; ++I)
		{
			float Ti = 0, Tq = 0;
			Quadrature(On, I / 200.f, 33.3, false, Ti, Tq);
			const double Turn = (7. * (I / 200.) - 33.3 * 1.1) * 2. * UE_DOUBLE_PI;
			Ride = FMath::Max(Ride, FMath::Abs(double(Ti) - .8 * FMath::Cos(Turn)));
		}
		// RX-LOOK gjennomgang 14 Nov: unlocked, the noise drifts: from one frame to the next (120 a second) no point of the trace moves
		// more than a little of full scale, while over a second it has moved
		double FrameStep = 0, SecondStep = 0;
		for (int32 Frame = 0; Frame < 240; ++Frame) for (int32 I = 0; I <= 60; ++I)
		{
			float Ai = 0, Aq = 0, Bi = 0, Bq = 0, Ci = 0, Cq = 0;
			const double T = 50. + Frame / 120.;
			Quadrature(Off, I / 60.f, T, false, Ai, Aq); Quadrature(Off, I / 60.f, T + 1. / 120., false, Bi, Bq); Quadrature(Off, I / 60.f, T + 1., false, Ci, Cq);
			FrameStep = FMath::Max(FrameStep, double(FMath::Max(FMath::Abs(Bi - Ai), FMath::Abs(Bq - Aq))));
			SecondStep = FMath::Max(SecondStep, double(FMath::Max(FMath::Abs(Ci - Ai), FMath::Abs(Cq - Aq))));
		}
		Check(FrameStep < .06 && SecondStep > .2, FString::Printf(TEXT("the unlocked I and Q noise drifts: at most %.3f of full scale from one frame to the next (it was drawn afresh 14 times a second), %.2f over a second"), FrameStep, SecondStep));
		Check(bQuad && Least > .5 && Most < 1.05 && Wild < .6 && Wild > .2 && CalmA == CalmC && CalmB == CalmD && Ride > .08 && Ride < RideShare + .08,
			FString::Printf(TEXT("I and Q: a circle when locked (%.2f to %.2f) with a little noise riding on the sines (%.2f of full scale at most), noise within %.2f when not, the same for the same clock, and standing with Calm flashes"), Least, Most, Ride, Wild));
	}
	{ // round 2: no number the frame draws shows a 47, and a clean number is written as it is
		bool bNone = true, bKept = true, bNear = true;
		int32 Swept = 0, Moved = 0;
		double Farthest = 0;
		for (int32 Decimals = 0; Decimals <= 3; ++Decimals)
		{
			const double Scale = Decimals == 0 ? 1. : Decimals == 1 ? 10. : Decimals == 2 ? 100. : 1000.;
			for (double V = -2600.; V <= 2600.; V += .0731)
			{
				const FString Shown = Num(V, Decimals), Signed = Num(V, Decimals, 9, true), Padded = Num(V, Decimals, 9, false, true);
				const double Back = FCString::Atod(*Shown);
				++Swept;
				bNone &= !Shows47(Shown) && !Shows47(Signed) && !Shows47(Padded) && Signed.Len() == 9 && Padded.Len() == 9;
				const double Off = FMath::Abs(Back - V);
				Farthest = FMath::Max(Farthest, Off);
				// a number with no 47 in it is the number rounded, and one with a 47 is the nearest that has none
				const int32 Whole = int32(FMath::RoundToDouble(V * Scale));
				if (Near47(Whole) == Whole) bKept &= Off <= .5 / Scale + 1e-9;
				else { ++Moved; bNear &= !Shows47(FString::FromInt(Near47(Whole))) && FMath::Abs(Near47(Whole) - Whole) >= 1; }
			}
		}
		for (int32 I = -99999; I <= 99999; ++I) { const int32 N = Near47(I); bNone &= !Shows47(FString::FromInt(N)) && FMath::Abs(N - I) <= 500 && (Shows47(FString::FromInt(I)) || N == I); }
		Check(bNone && bKept && bNear && Moved > 100 && Farthest <= 5.51
			&& Num(53.04, 1) == TEXT("53.0") && Num(7.2, 1, 5, false, true) == TEXT("007.2") && Num(3.2, 1, 5, true) == TEXT(" +3.2") && Num(-0.04, 1) == TEXT("0.0") && Num(5., 0, 2, false, true) == TEXT("05")
			&& Num(4.7, 1) == TEXT("4.8") && Num(47.) == TEXT("48") && Num(14.7, 1) == TEXT("14.8") && Num(.47, 2) == TEXT("0.48") && Num(147.) == TEXT("148") && Num(-4.7, 1, 6, true) == TEXT("  -4.8") && Num(47.5, 1) == TEXT("48.0") && Num(1419.647, 3) == TEXT("1419.648")
			&& Shows47(TEXT("14h47m02s")) && Shows47(TEXT("+4.7")) && !Shows47(TEXT("04h07m")) && !Shows47(TEXT("74")),
			FString::Printf(TEXT("no number the frame draws shows a 47: %d readouts swept from -2600 to 2600 at nought to three decimals, signed and padded, %d of them moved to the nearest number without one (never more than %.2f away), every other written as it is"), Swept * 3, Moved, Farthest));
		// RX-LOOK gjennomgang 14 Nov: the TUNE page's readouts as the page writes them (the frequency dial, gain, bandwidth, the carrier
		// lock, the dial's distance off the card, the take's seconds), swept over every value the dials and the take reach: none shows
		// a 47, and a readout that climbs never shows a value out of order (47 % is 48 % every time, after 46 % and before 49 %)
		{
			bool bDials = true, bOrder = true;
			int32 Dial = 0;
			double Was = -1e9;
			auto Climb = [&](const FString& Shown, double Back) { bDials &= !Shows47(Shown); bOrder &= Back >= Was - 1e-9; Was = Back; ++Dial; };
			for (int32 Step = 0; Step <= 1200; ++Step) { const FString S = Num(1419.5 + Step * .001, 3) + TEXT(" MHz"); Climb(S, FCString::Atod(*S)); }
			Was = -1e9; for (int32 V = 0; V <= 100; ++V) { const FString S = Num(double(V)); Climb(S, FCString::Atod(*S)); }
			Was = -1e9; for (int32 V = 1; V <= 100; ++V) { const FString S = Num(double(V)) + TEXT(" kHz"); Climb(S, FCString::Atod(*S)); }
			Was = -1e9; for (int32 V = 0; V <= 100; ++V) { const FString S = Num(double(V)) + TEXT(" % CLOSE"); Climb(S, FCString::Atod(*S)); }
			Was = -1e9; for (int32 Step = -400; Step <= 400; ++Step) { const FString S = Num(Step * .0005, 3, 0, true) + TEXT(" MHz"); Climb(S, FCString::Atod(*S)); }
			Was = -1e9; for (int32 Step = 0; Step <= 1800; ++Step) { const FString S = TakeCount(Step * .01, 18.); Climb(S, FCString::Atod(*S)); }
			Check(bDials && bOrder && Num(47.) == TEXT("48") && Num(46.) == TEXT("46") && TakeCount(7.9, 18.) == TEXT("7 / 18 s") && Num(1419.647, 3) + TEXT(" MHz") == TEXT("1419.648 MHz") && Num(-.047, 3, 0, true) == TEXT("-0.048"),
				FString::Printf(TEXT("the TUNE page's dials and readouts, %d values swept as the page writes them: none shows a 47, and none climbs out of order"), Dial));
		}
		int32 Counted = 0;
		const FString Sixties = Clean47(47, [&Counted](int32 N) { ++Counted; return FString::Printf(TEXT("%02dm"), N); });
		Check(Sixties == TEXT("48m") && Counted == 2 && Clean47(12, [](int32 N) { return FString::Printf(TEXT("%02dm"), N); }) == TEXT("12m"), TEXT("a readout in sixties that would show a 47 stands a step off; a clean one is written once, as it is"));
	}
	{ // RX-LOOK gjennomgang 14 Nov: the arms by number (the chart's labels by the fans, the tracking panel's DELAY rows), and the
		// text rule's own check, which every line the receiver shows is held to
		bool bArms = true;
		for (int32 Arm = 0; Arm < 3; ++Arm)
		{
			const FString Label = ArmLabel(Arm);
			bArms &= Label.Len() == 1 && TextFault(Label).IsEmpty() && TextFault(TEXT("DELAY ") + Label + TEXT(" +12.5 ns")).IsEmpty() && TextFault(TEXT("ARM ") + Label).IsEmpty() && (Arm == 0 || Label != ArmLabel(Arm - 1));
		}
		const bool bCatches = !TextFault(TEXT("DELAY C")).IsEmpty() && !TextFault(TEXT("ARM C")).IsEmpty() && !TextFault(TEXT("C")).IsEmpty() && !TextFault(TEXT("1419.647 MHz")).IsEmpty() && !TextFault(TEXT("+0.047")).IsEmpty()
			&& !TextFault(TEXT("23:47")).IsEmpty() && !TextFault(TEXT("THE SOURCE WAS TAKEN")).IsEmpty() && !TextFault(TEXT("CHECK THE CABLE")).IsEmpty() && !TextFault(TEXT("1947")).IsEmpty();
		const bool bLets = TextFault(TEXT("3C 286 / CARD")).IsEmpty() && TextFault(TEXT("TAKE ON TAPE")).IsEmpty() && TextFault(TEXT("RECORDING 7/18 s")).IsEmpty() && TextFault(TEXT("SC 104+70")).IsEmpty() && TextFault(TEXT("PSR B1133+16")).IsEmpty();
		Check(bArms && bCatches && bLets, TEXT("the arms are 1, 2 and 3 (DELAY 1 to 3, one letter each as A to C were); the text check catches a lone C, a 47, «taken» and «the cable», and lets the card names by"));
	}
	{ // round 2: the letters. Nothing under the next step's line, and a letter's width as the glass draws it
		Check(TextFloor == 22 && FMath::Abs(GlyphWide(22) - 8.4375f) < 1e-4f && GlyphWide(20) == GlyphWide(22) && GlyphWide(18) > 18 * .4f && FMath::Abs(TextWide(10, 54) - 215.625f) < 1e-3f && GlyphWide(1) > 0.f,
			FString::Printf(TEXT("the frame's smallest letters are the next step's (Mono %d, %.2f units wide on the glass: a whole number of the picture's pixels)"), TextFloor, GlyphWide(TextFloor)));
	}
	return bOk;
}
