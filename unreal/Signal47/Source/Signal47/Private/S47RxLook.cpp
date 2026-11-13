#include "S47RxLook.h"

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
		return Say(EState::Recording, FString::Printf(TEXT("RECORDING %d/%d s"), FMath::Clamp(int32(In.Elapsed), 0, 999), FMath::Clamp(FMath::RoundToInt32(In.Seconds), 0, 999)), true);
	}
	if (In.Stage <= 4)
	{
		if (In.bReference) return Say(EState::Reference, FString::Printf(TEXT("REFERENCE %d/8 s"), FMath::Clamp(int32(In.ReferenceAt), 0, 8)), true);
		if (In.bPlainOnTape) return Say(EState::Anomaly, TEXT("ANOMALY DETECTED"), true);
		return Say(EState::OnTape, TEXT("TAKE ON TAPE"), false);
	}
	return Say(EState::Done, TEXT("TAKE REGISTERED"), false);
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
		constexpr int32 Bins = 40;
		bool bSame = true, bOther = false; double Mid = 0, Side = 0, MidOff = 0, SideOff = 0, LineAt = 0;
		for (uint32 Row = 0; Row < 60; ++Row)
		{
			const FFallRow A = FallRow(On, Row, false), B = FallRow(Off, Row, false), C = FallRow(Low, Row, false);
			for (int32 Bin = 0; Bin < Bins; ++Bin)
			{
				bSame &= FallCell(A, On.Seed, Bin, Bins) == FallCell(FallRow(On, Row, false), On.Seed, Bin, Bins);
				bOther |= FallCell(A, On.Seed, Bin, Bins) != FallCell(A, On.Seed + 1u, Bin, Bins);
			}
			Mid += FallCell(A, On.Seed, Bins / 2, Bins) + FallCell(A, On.Seed, Bins / 2 - 1, Bins); Side += FallCell(A, On.Seed, 4, Bins) + FallCell(A, On.Seed, Bins - 5, Bins);
			MidOff += FallCell(B, On.Seed, Bins / 2, Bins) + FallCell(B, On.Seed, Bins / 2 - 1, Bins); SideOff += FallCell(B, On.Seed, 4, Bins) + FallCell(B, On.Seed, Bins - 5, Bins);
			LineAt += C.Line;
		}
		const FFallRow Tick = FallRow(On, 9u, true), Plain = FallRow(On, 9u, false);
		Check(bSame && bOther && Mid > Side * 2.5 && MidOff < SideOff * 1.6 && FMath::Abs(LineAt / 60. - (.5 - .05 / (2. * HalfSpan(On.FTol)))) < .02
			&& FallCell(Tick, On.Seed, Bins / 2 + 3, Bins) > FallCell(Plain, On.Seed, Bins / 2 + 3, Bins) + .2f && FallCell(FallRow(FScope(), 3u, false), 1u, 5, Bins) == 0.f,
			TEXT("the waterfall is the same for the same seed and row; the line stands bright in the middle when the dial is on the card, off to the side when it is off, gone with no lock; a tick is a short wide dash"));
		// the spectrum: a peak over the floor at the card's line, none without the source; the dial's mark follows the dial
		const float PeakOn = FftLevel(On, .5f, 12.3), FloorOn = FftLevel(On, .1f, 12.3), PeakOff = FftLevel(Off, .5f, 12.3);
		Check(PeakOn > FloorOn + .3f && PeakOff < FloorOn + .25f && FftLevel(On, .5f, 12.3) == PeakOn && FftLevel(FScope(), .5f, 1.) == 0.f && FMath::Abs(FftTuned(On) - .5f) < 1e-6f && FftTuned(Low) > .5f,
			TEXT("the small spectrum: a peak at the card's line when the source is heard, the floor without it, the dial's mark where the dial is"));
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
		Check(bQuad && Least > .68 && Most < .92 && Wild < .6 && Wild > .2 && CalmA == CalmC && CalmB == CalmD,
			FString::Printf(TEXT("I and Q: a steady circle when locked (%.2f to %.2f), noise within %.2f when not, the same for the same clock, and standing with Calm flashes"), Least, Most, Wild));
	}
	return bOk;
}
