// story/truckOnB.ts: the truck on B
#include "S47TruckOnB.h"

#include "S47Docs.h"
#include "S47Lines.h"
#include "S47Radio.h"
#include "S47Seen.h"
#include "S47Trouble.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47TruckOnB, Log, All);

namespace
{
	/** Within this far of B (metres, the truck's middle) the truck is over it; it must be this far off
	 *  again before the next time counts. The turn out of the service road passes 3.7 to 5 m from it. */
	const double ON = 6, REARM = 16;
	/** Over B the compass reads 026 within NEAR metres, and lets go by FAR. */
	const double NEAR = 5, FAR = 16;
	/** Seconds nothing else of the night's may start after a rung (one at a time). */
	const double TAIL = 8;
	/** From the motel's lot: the truck out on the highway (|map x| under OUT_X, the asphalt), this many
	 *  metres into the drive and at speed; past GIVE_UP metres not on this drive. */
	const double OUT_X = 4.5, OUT_M = 10, GIVE_UP = 220;
	/** The old road: the compass leaves the road's heading at mile 6 and is on 026 by 7.6. */
	const double OLD_FROM = 6.0, OLD_TO = 7.6;
	double Smooth(double A, double B, double X) { const double T = FMath::Min(1.0, FMath::Max(0.0, (X - A) / (B - A))); return T * T * (3 - 2 * T); }
	double JsRound(double V) { return FMath::FloorToDouble(V + 0.5); }
}

bool FS47TruckOnB::Four() const { return D.Seen && D.Seen->Contains(TEXT("truckB:4")); }
bool FS47TruckOnB::Seven() const { return D.Seen && D.Seen->Contains(TEXT("truckB:7")); }
bool FS47TruckOnB::Due(const FS47TruckOnBIn& S) const { return S.bOpen && !bOff && !Seven() && (S.bAfter6 || !Four()); }

void FS47TruckOnB::Update(double Dt, const FS47TruckOnBIn& S)
{
	const TOptional<double> Off = S.At ? TOptional<double>(FMath::Abs(S47Lines::Offset(TEXT('B'), S.At->X, S.At->Y))) : TOptional<double>();
	Dist = Off;
	if (!Off || *Off > REARM || !S.bTrip) bArmed = !Off || *Off > REARM;
	// ---------- a rung that is running ----------
	if (Racing() && !S.bTrip) Abandon(S.Clock);
	const bool bTrouble = D.Trouble && D.Trouble->Kind() != ES47Trouble::None;
	if (Phase != TEXT("idle")) { T += Dt; Run(S); }
	else if (S.bTrip && S.bOpen && bArmed && Off && *Off < ON && FMath::Abs(S.Speed) > 1.5)
	{
		// ---------- over B ----------
		bArmed = false;
		Crossings++;
		const FString Rung = Seven() || bOff ? FString() : S.bAfter6 ? FString(TEXT("stall")) : Four() ? FString() : FString(TEXT("four"));
		if (!Rung.IsEmpty())
		{
			if (!S.bOk || bTrouble) { Missed++; UE_LOG(LogS47TruckOnB, Display, TEXT("S47: the truck on B: a crossing missed, something else going")); }
			else Begin(Rung, S, TEXT("turnoff"));
		}
	}
	else if (S.bTrip && S.bOpen && S.bAfter6 && !Seven() && !bOff && OutOfMotel(S))
	{
		// ---------- after room 6, out of the motel's lot onto the highway ----------
		if (!S.bOk || bTrouble) { if (!bMotelMissed) { bMotelMissed = true; Missed++; } }
		else Begin(TEXT("stall"), S, TEXT("motel"));
	}
	if (!S.bTrip) bMotelMissed = false;
	// ---------- the compass ----------
	double K = 0;
	if (Phase == TEXT("four") || Phase == TEXT("stall") || Phase == TEXT("seven") || (Phase == TEXT("tail") && T < 2.5)) K = 1;
	else if (S.OldMi) K = Smooth(OLD_FROM, OLD_TO, *S.OldMi);
	else if (S.bOpen && Off) K = 1 - Smooth(NEAR, FAR, *Off);
	if (K > 0) Pull = FPull{ BEARING, K }; else Pull.Reset();
	if (S.OldMi && K > 0.97 && !bToldOld && S.bOk)
	{
		bToldOld = true;
		if (D.Toast) D.Toast(TEXT("The compass on the dash has come round to 026. The road has not."), 4.4f);
	}
}

bool FS47TruckOnB::OutOfMotel(const FS47TruckOnBIn& S) const
{
	// a drive from the motel's lot, out on the highway and still on B's ground (the seven's other way in)
	return S.From == TEXT("motel") && S.At && S.Metres > OUT_M && S.Metres < GIVE_UP && FMath::Abs(S.At->X) < OUT_X
		&& FMath::Abs(S.Speed) > 3 && S47Lines::DistTo(TEXT('B'), S.At->X, S.At->Y) <= S47Lines::LINES(TEXT('B')).Reach;
}

void FS47TruckOnB::Begin(const FString& Rung, const FS47TruckOnBIn& S, const FString& InWhere)
{
	const double Clock = S.Clock;
	Phase = Rung; T = 0; Blinked = 0; bFailed = false; Step = 0; CaughtAt = 0; At = Clock; Where = InWhere;
	bCalm = S.bCalm;
	FS47Receiver* R = Radio();
	bRadioWasOn = R && R->bOn;
	if (Rung == TEXT("four"))
	{
		if (R) R->Carrier(1.1);
		if (D.Seen) D.Seen->Add(FString::Printf(TEXT("mark:truck4@%d"), int32(JsRound(Clock))));
		if (D.Toast) D.Toast(bRadioWasOn ? TEXT("The radio drops to a bare carrier. The engine misses.") : TEXT("The radio is off. A carrier comes out of it anyway. The engine misses."), 2.6f);
	}
	else
	{
		FS47TroubleOpts O; O.bKey = true; O.Tries = 2;
		if (D.Trouble) D.Trouble->Start(ES47Trouble::Stall, O);
		if (R) R->Carrier(3);
		if (D.Seen) D.Seen->Add(FString::Printf(TEXT("mark:%s@%d"), Where == TEXT("motel") ? TEXT("truck7m") : TEXT("truck7"), int32(JsRound(Clock))));
		if (D.Toast) D.Toast(TEXT("The engine dies. OIL and VOLTS. The radio goes to a bare carrier."), 3.4f);
	}
	UE_LOG(LogS47TruckOnB, Display, TEXT("S47: the truck on B: %s (%s)"), *Rung, *Where);
}

void FS47TruckOnB::Run(const FS47TruckOnBIn& S)
{
	FS47TruckTrouble* Tr = D.Trouble;
	FS47Receiver* R = Radio();
	if (!Tr) return;
	// a relay ticks in the dash with each blink, as every light goes out
	const int32 N = Tr->Blinks();
	if (Tr->Kind() == ES47Trouble::Blink && N > Blinked) { Blinked = N; if (D.Tick) D.Tick(); }
	if (Phase == TEXT("four"))
	{
		// the carrier (begin), the engine missing under it, then the count
		if (T >= 0.12 && Step == 0) { Step = 1; FS47TroubleOpts O; O.Len = 0.85; Tr->Start(ES47Trouble::Sputter, O); }
		if (T >= 1.15 && Step == 1) { Step = 2; FS47TroubleOpts O; O.Count = 4; O.bSoft = bCalm; Tr->Start(ES47Trouble::Blink, O); }
		if (Step == 2 && Tr->Kind() != ES47Trouble::Blink) Done(TEXT("four"), S.Clock);
	}
	else if (Phase == TEXT("stall"))
	{
		// the carrier holds while it is dead (the starter takes the radio down with the lights), and
		// through the seven after it
		if (R) R->Carrier(0.5);
		if (Tr->Stage() == TEXT("pause") && Tr->Turns == 1 && !bFailed) { bFailed = true; if (D.Toast) D.Toast(TEXT("It turns over and will not catch."), 2.6f); }
		// it caught and settled: a breath, then seven
		if (Tr->Kind() == ES47Trouble::None && Step == 0) { Step = 1; CaughtAt = T; }
		if (Step == 1 && T - CaughtAt >= 0.9) { Phase = TEXT("seven"); T = 0; Step = 0; FS47TroubleOpts O; O.Count = 7; O.bSoft = bCalm; Tr->Start(ES47Trouble::Blink, O); }
	}
	else if (Phase == TEXT("seven"))
	{
		if (R) R->Carrier(0.3);
		if (Tr->Kind() != ES47Trouble::Blink) { if (R) R->Carrier(0); Done(TEXT("seven"), S.Clock); }
	}
	else if (Phase == TEXT("tail"))
	{
		if (T >= TAIL) Phase = TEXT("idle");
	}
}

void FS47TruckOnB::Done(const FString& Rung, double Clock)
{
	const FString AtT = S47Text::ClockText(At ? At : Clock, false);
	if (Rung == TEXT("four"))
	{
		if (D.Seen) D.Seen->Add(TEXT("truckB:4"));
		if (D.Toast) D.Toast(TEXT("Four. Every light in the truck, four times. The compass on the dash reads 026."), 4.6f);
		if (D.Note) D.Note(FString::Printf(TEXT("%s. The turn-off to SARO, on B. %s, the engine missed, and every light in the truck went out four times. Evenly, like counting. The compass on the dash swung round to 026 and held it."),
			*AtT, bRadioWasOn ? TEXT("The radio went to a bare carrier") : TEXT("The radio was off and gave a carrier anyway")));
	}
	else
	{
		if (D.Seen) D.Seen->Add(TEXT("truckB:7"));
		if (D.Toast) D.Toast(TEXT("Seven."), 3);
		const FString Place = Where == TEXT("motel") ? FString(TEXT("Out of Sierra Motor Court onto the highway")) : FString::Printf(TEXT("The turn-off to SARO, on B%s"), Four() ? TEXT(", again") : TEXT(""));
		if (D.Note) D.Note(FString::Printf(TEXT("%s. %s. The engine died under me, OIL and VOLTS lit, the radio on a carrier. It caught on the second turn of the key. Then every light in the truck went out seven times.%s"),
			*AtT, *Place, Four() ? TEXT(" Four, the last time.") : TEXT("")));
	}
	if (D.Save) D.Save();
	Phase = TEXT("tail"); T = 0; Step = 0;
	UE_LOG(LogS47TruckOnB, Display, TEXT("S47: the truck on B: %s done"), *Rung);
}

void FS47TruckOnB::Abandon(double Clock)
{
	// the drive ended in the middle of a rung (the truck left, the night put away): what was seen of it
	// stands, so it is not given again
	const FString Rung = Phase == TEXT("four") ? TEXT("four") : TEXT("seven");
	if (FS47Receiver* R = Radio()) R->Carrier(0);
	Done(Rung, Clock);
	Phase = TEXT("idle");
}

void FS47TruckOnB::Reset()
{
	Phase = TEXT("idle"); T = 0; Blinked = 0; Step = 0;
	bArmed = false; Pull.Reset(); Dist.Reset(); bToldOld = false; Where = TEXT("turnoff"); bMotelMissed = false;
	Missed = 0; Crossings = 0;
}

FString FS47TruckOnB::Text() const
{
	return FString::Printf(TEXT("truck on B %s%s%s%s%s"), *Phase, Four() ? TEXT(" 4") : TEXT(""), Seven() ? TEXT(" 7") : TEXT(""),
		Dist ? *FString::Printf(TEXT(" %.0fm"), *Dist) : TEXT(""), Pull ? *FString::Printf(TEXT(" 026 x%.2f"), Pull->K) : TEXT(""));
}

bool FS47TruckOnB::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s truck on B: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	FS47TownSeen Seen;
	FS47TruckTrouble Trouble;
	TArray<FString> Notes;
	int32 Ticks = 0;
	FS47TruckOnBDeps D;
	D.Trouble = &Trouble; D.Seen = &Seen;
	D.Note = [&Notes](const FString& T) { Notes.Add(T); };
	D.Tick = [&Ticks] { Ticks++; };
	FS47TruckOnB B(D);
	const double Bz = S47Lines::HighwayZ(TEXT('B'));
	FS47TruckOnBIn In;
	In.bOk = true; In.bTrip = true; In.bOpen = true; In.Speed = 8; In.Clock = 3 * 3600;
	In.At = FVector2D(0, Bz - 120);
	B.Update(0.1, In);
	Check(B.Phase == TEXT("idle") && !B.Pull, TEXT("away from B, nothing, and the compass free"));
	In.At = FVector2D(0, Bz);
	B.Update(0.1, In);
	Check(B.Phase == TEXT("four") && B.Pull && B.Pull->Deg == 26 && Seen.Contains(FString::Printf(TEXT("mark:truck4@%d"), 3 * 3600)), TEXT("over B from chapter two: four, the compass on 026"));
	// the next crossing that counts is not before the truck has been off it again
	B.Update(0.1, In);
	Check(B.Crossings == 1, TEXT("one crossing until the truck has been away from B"));
	// a rung that is due takes the pass for the truck taken (story/truckTaken.ts)
	FS47TownSeen Seen2;
	FS47TruckOnBDeps D2 = D; D2.Seen = &Seen2;
	FS47TruckOnB C(D2);
	FS47TruckOnBIn After6 = In; After6.bAfter6 = true;
	Check(C.Due(In) && C.Due(After6), TEXT("the four is due from chapter two, the seven after room 6"));
	Seen2.Add(TEXT("truckB:4"));
	Check(!C.Due(In) && C.Due(After6), TEXT("four had: only the seven, after room 6"));
	Seen2.Add(TEXT("truckB:7"));
	Check(!C.Due(After6), TEXT("seven had: nothing more"));
	// the old road: the compass on 026 from mile 6 to 7.6
	FS47TruckOnB Old(D2);
	FS47TruckOnBIn Road; Road.bOk = true; Road.OldMi = 7.7;
	Old.Update(0.1, Road);
	Check(Old.Pull && Old.Pull->K > 0.97, TEXT("on the old road the compass comes round to 026"));
	return bOk;
}
