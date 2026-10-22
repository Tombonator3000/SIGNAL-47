// story/truckTaken.ts: taken in the truck
#include "S47TruckTaken.h"

#include "S47Abduction.h"
#include "S47Audio.h"
#include "S47Docs.h"
#include "S47Lines.h"
#include "S47Radio.h"
#include "S47Seen.h"
#include "S47Trouble.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47TruckTaken, Log, All);

namespace
{
	/** Within this far of B (m, the truck's middle) the truck is over it; this far off again before the
	 *  next pass counts (as story/truckOnB.ts). */
	const double ON = 6, REARM = 16;
	/** The truck slows to a stop under it (m/s²). */
	const double HELD = 3.2;
	/** The third turn catches, if begun before the beep. */
	const double CATCH = 3;
	/** Infinity: it never catches. */
	const double NEVER = TNumericLimits<double>::Max();
	/** Put down this far on from where it was taken (m), the nose the other way, in the southbound lane
	 *  of SARO's highway (site x; drive/legs.ts SARO_HWY: x -24, the southbound lane west of the
	 *  middle), and never onto the motel's lot. */
	const double ON_FROM = 300, LANE_X = -25.8, LAST_Z = 600;
	/** A warning in the truck comes at most this many times in a night. */
	const int32 TRIES = 2;
	/** Seconds nothing else of the night's starts after it. */
	const double TAIL = 8;
	/** The knocks on the roof: when, and how far apart (as story/nightEvents.ts has them). */
	const double KNOCK_AT = 41.2;
	const double KNOCKS[] = { 0, 0.95, 1.9 };
	double JsRound(double V) { return FMath::FloorToDouble(V + 0.5); }
	FString Clock(double S) { return S47Text::ClockText(S, false); }
	FS47TroubleOpts Stall(double Tries, bool bDead = false) { FS47TroubleOpts O; O.bKey = true; O.Tries = Tries; O.bDead = bDead; return O; }
}

FS47Receiver* FS47TruckTaken::Radio() const { return D.Radio ? D.Radio() : nullptr; }

bool FS47TruckTaken::Has(const FString& Prefix) const
{
	if (D.Seen) for (const FString& K : D.Seen->Keys) if (K.StartsWith(Prefix)) return true;
	return false;
}

bool FS47TruckTaken::Taken() const { return Has(TEXT("tt:taken@")); }

int32 FS47TruckTaken::Tries() const
{
	int32 N = 0;
	if (D.Seen) for (const FString& K : D.Seen->Keys) if (K.StartsWith(TEXT("tt:try@"))) N++;
	return N;
}

bool FS47TruckTaken::Ahead() const { return Phase == TEXT("idle") && bCould && bArmed && Pass != TEXT("ladder") && Dist.IsSet() && *Dist < REARM; }

void FS47TruckTaken::Update(double Dt, const FS47TruckTakenIn& S)
{
	FS47TruckTrouble* Tr = D.Trouble;
	// after the waking, whatever comes next: the frost goes off the glass slowly, quicker once the
	// engine runs and the heater with it; the headlights come on with the engine
	if (FrostK > 0) { FrostK = FMath::Max(0.0, FrostK - Dt / ((Tr && Tr->Kind() == ES47Trouble::Stall) || !S.bInCab ? 240 : 80)); if (D.Frost) D.Frost(FrostK); }
	if (bLightsOwed && Phase != TEXT("wake"))
	{
		if (!S.bInCab) bLightsOwed = false;
		else if (Tr && Tr->Kind() != ES47Trouble::Stall) { bLightsOwed = false; Tr->Dim = 1; if (D.Headlights) D.Headlights(true); }
	}
	const TOptional<double> Off = S.At ? TOptional<double>(FMath::Abs(S47Lines::Offset(TEXT('B'), S.At->X, S.At->Y))) : TOptional<double>();
	Dist = Off;
	if (!Off || *Off > REARM || !S.bTrip) { bArmed = true; Pass.Reset(); }
	bCould = S.bTrip && !bOff && !Taken() && Tries() < TRIES && S.bEnabled && S.bAllowed && (S.bMay || bForce) && !S.bDue;
	if (Phase == TEXT("idle"))
	{
		if (!S.bTrip || !Off || *Off >= ON || !bArmed) return;
		// the pass is decided as the truck comes onto B: a rung of the ladder that is due has it
		if (Pass.IsEmpty()) { Pass = S.bDue ? TEXT("ladder") : TEXT("free"); if (S.bDue) LadderPasses++; }
		if (Pass != TEXT("free") || bOff || Taken() || Tries() >= TRIES) return;
		if (S.South < 0.5 || FMath::Abs(S.Speed) < 1.5) return;
		if (!S.bOk || !S.bEnabled || !S.bAllowed || !(S.bMay || bForce)) return;
		bArmed = false; bForce = false;
		Start(S.Clock);
		return;
	}
	if (Phase == TEXT("warning")) { WarningStep(Dt, S); return; }
	if (Phase == TEXT("taken")) { TakenStep(); return; }
	if (Phase == TEXT("wake")) { WakeStep(Dt, S); return; }
	if (Phase == TEXT("tail")) { if ((T += Dt) >= TAIL) Phase = TEXT("idle"); return; }
}

void FS47TruckTaken::Start(double ClockNow)
{
	Phase = TEXT("warning"); T = 0; At = ClockNow; Free.Reset(); Ended.Reset();
	Turns = 0; bKnocked = false; bToldSilent = false; bToldDead = false; bGotOut = false; bWasOut = false; North = 0;
	FailedTold.Reset();
	bWokeHere = false; bOverSeen = false; bTakenIn = true;
	bKnocksOwed = !(D.Seen && D.Seen->Contains(TEXT("ne:knocks1")));
	if (D.Seen) D.Seen->Add(FString::Printf(TEXT("tt:try@%d"), int32(JsRound(ClockNow))));
	const TOptional<FS47TruckTakenDeps::FTruckAt> Tk = D.Truck ? D.Truck() : TOptional<FS47TruckTakenDeps::FTruckAt>();
	Plan.Reset();
	if (Tk) Plan = FPlan{ LANE_X, FMath::Min(LAST_Z, Tk->Z + ON_FROM) };
	// the radio slides off its station under the static first (it is on), then the bare carrier
	FS47Receiver* R = Radio();
	bRadioWas = R && R->bOn;
	if (R && !R->Drift(1.1, 1)) R->Carrier(0.5);
	// (the second time tonight, after the key got the truck away once, it never catches: they have a
	// better hold of it, and the way out is out of it)
	bKeyProof = Has(TEXT("tt:key@"));
	if (D.Trouble) D.Trouble->Start(ES47Trouble::Stall, Stall(bKeyProof ? NEVER : CATCH));
	if (D.Hold) D.Hold(HELD);
	if (D.Begin) D.Begin();
	if (D.Toast) D.Toast(bRadioWas ? TEXT("The radio slides off the station into a bare carrier. The engine dies.") : TEXT("The radio is off. A carrier comes out of it anyway. The engine dies."), 3.4f);
	UE_LOG(LogS47TruckTaken, Display, TEXT("S47: taken in the truck: the warning, try %d%s"), Tries(), bKeyProof ? TEXT(", the key never catches") : TEXT(""));
}

TOptional<FVector> FS47TruckTaken::Planned()
{
	const TOptional<FS47TruckTakenDeps::FTruckAt> Tk = D.Truck ? D.Truck() : TOptional<FS47TruckTakenDeps::FTruckAt>();
	if (Tk) Plan = FPlan{ LANE_X, FMath::Min(LAST_Z, Tk->Z + ON_FROM) };
	if (!Plan) return {};
	return FVector(Plan->X, D.Ground ? D.Ground(Plan->X, Plan->Z) : 0, Plan->Z);
}

void FS47TruckTaken::WarningStep(double Dt, const FS47TruckTakenIn& S)
{
	FS47TruckTrouble* Tr = D.Trouble;
	const double Tw = S.WarnT ? *S.WarnT : T;
	T += Dt;
	// it has passed (abduction.ts saw the truck let go, or Missing time turned off), or come
	if (S.Stage == TEXT("calm")) { Release(); End(Free.IsEmpty() ? FString(TEXT("off")) : Free, S); return; }
	if (S.Stage != TEXT("warning")) { Phase = TEXT("taken"); bTakenIn = S.bInCab; return; }
	if (!Free.IsEmpty()) return;
	const FS47Abduction::FBeat Beat = FS47Abduction::BEAT;
	if (S.bInCab)
	{
		if (Tr)
		{
			// (got back into it: as dead as it was, and three turns again before the beep)
			const double Can = Tw < Beat.Steps && !bKeyProof ? CATCH : NEVER;
			if (Tr->Kind() != ES47Trouble::Stall) Tr->Start(ES47Trouble::Stall, Stall(Can, true));
			if (bWasOut) { bWasOut = false; Tr->CatchOn(Can); Turns = Tr->Turns; FailedTold.Reset(); }
			// the third turn catches if it was begun before the beep; after it, never
			if (Tw >= Beat.Steps && Tr->Turns < CATCH) Tr->CatchOn(NEVER);
			if (D.Hold) D.Hold(HELD);
			// the electrics go down with the battery from the pressure, and out at :40, the radio with them
			Tr->Dim = Tw < Beat.Tone ? 1 : Tw < Beat.Dark ? 1 - 0.55 * (Tw - Beat.Tone) / (Beat.Dark - Beat.Tone) : 0;
			if (Tr->Turns > Turns)
			{
				Turns = Tr->Turns;
				if (Tw >= Beat.Steps && !bToldSilent) { bToldSilent = true; if (D.Toast) D.Toast(TEXT("The starter turns. There is no sound at all."), 3); }
			}
			const FString Stage = Tr->Stage();
			if (Stage == TEXT("pause") && Turns == 1 && !FailedTold.Contains(1)) { FailedTold.Add(1); if (D.Toast) D.Toast(TEXT("It turns over and will not catch."), 2.6f); }
			if (Stage == TEXT("pause") && Turns == 2 && !FailedTold.Contains(2) && Tw < Beat.Steps) { FailedTold.Add(2); if (D.Toast) D.Toast(TEXT("It turns over. Nearly."), 2.4f); }
			if (Stage == TEXT("pause") && Turns == 3 && !FailedTold.Contains(3) && Tw < Beat.Steps && bKeyProof) { FailedTold.Add(3); if (D.Toast) D.Toast(TEXT("The third turn. It does not catch this time."), 2.8f); }
			// it caught: they let go
			if (Stage == TEXT("catching")) { Free = TEXT("key"); Release(); UE_LOG(LogS47TruckTaken, Display, TEXT("S47: taken in the truck: the key caught")); return; }
		}
	}
	else
	{
		// (out of it: the engine is dead for whoever gets back in)
		if (!bGotOut && D.KeyStall) D.KeyStall();
		bGotOut = true; bWasOut = true;
		North = FMath::Max(North, S.North);
		if (S.Away >= RUN) { Free = TEXT("ran"); Release(); UE_LOG(LogS47TruckTaken, Display, TEXT("S47: taken in the truck: out and away")); return; }
	}
	// the radio: the bare carrier from the start to the dark (and none after)
	if (FS47Receiver* R = Radio())
	{
		if (Tw >= 0.9 && Tw < Beat.Dark) R->Carrier(0.5);
		if (Tw >= Beat.Dark) R->Carrier(0);
	}
	// :16, the truck's own sound gone (the engine's note, the tyres, the starter)
	if (D.Hush) D.Hush(Tw >= Beat.Steps);
	// :41, three knocks on the roof of the cab
	if (Tw >= KNOCK_AT && !bKnocked)
	{
		bKnocked = true;
		const TOptional<FVector> Roof = D.Roof ? D.Roof() : TOptional<FVector>();
		if (D.Audio) for (const double W : KNOCKS)
		{
			FS47Sound O; O.Gain = 1.1f; O.Rate = 0.62f; O.Lp = 900; O.When = float(W);
			if (Roof && !S.bInCab) O.At = *Roof;
			D.Audio->Sfx(TEXT("thudSoft"), O);
		}
	}
}

bool FS47TruckTaken::UseKey(TOptional<double> WarnT)
{
	if (Phase != TEXT("warning") || !Free.IsEmpty() || !WarnT || *WarnT < FS47Abduction::BEAT.Dark) return false;
	if (D.Audio) { FS47Sound O; O.Gain = 0.35f; O.Rate = 0.55f; D.Audio->Sfx(TEXT("click"), O); }
	if (!bToldDead) { bToldDead = true; if (D.Toast) D.Toast(TEXT("The key turns. Nothing. Not even the starter."), 3); }
	return true;
}

void FS47TruckTaken::Release()
{
	// the truck lets go: its engine, its lights, its sound and the radio its own again
	if (D.Hold) D.Hold(0);
	if (D.Hush) D.Hush(false);
	if (FS47TruckTrouble* Tr = D.Trouble)
	{
		Tr->Dim = 1;
		if (Tr->Kind() == ES47Trouble::Stall && Tr->Stage() != TEXT("catching")) Tr->CatchOn(1);
	}
	if (FS47Receiver* R = Radio()) R->Carrier(0);
	FailedTold.Reset();
}

void FS47TruckTaken::TakenStep()
{
	// the light and the taking: the truck dark and still (the abduction has the rest)
	if (D.Hold) D.Hold(HELD);
	if (D.Trouble) D.Trouble->Dim = 0;
	if (FS47Receiver* R = Radio()) R->Carrier(0);
}

// ---------- the waking (abduction.ts, AbductDeps.cab.wake) ----------

TOptional<FVector> FS47TruckTaken::Wake(double Lost, bool bInCab)
{
	(void)Lost;
	TOptional<FPlan> P = Plan;
	if (!P)
	{
		const TOptional<FS47TruckTakenDeps::FTruckAt> Tk = D.Truck ? D.Truck() : TOptional<FS47TruckTakenDeps::FTruckAt>();
		if (Tk) P = FPlan{ LANE_X, FMath::Min(LAST_Z, Tk->Z + ON_FROM) };
	}
	Phase = TEXT("wake"); T = 0; bTakenIn = bInCab; bWokeHere = true;
	if (D.Hold) D.Hold(0);
	if (D.Hush) D.Hush(false);
	if (!P) return {};
	const bool bMoved = bInCab ? (D.MoveDriven && D.MoveDriven(P->X, P->Z, 0)) : (D.MoveParked && D.MoveParked(P->X, P->Z, 0));
	if (!bMoved) return {};
	if (bInCab)
	{
		// the engine off and quiet, every light off: the key starts it, first time
		if (D.Trouble) { D.Trouble->Start(ES47Trouble::Stall, Stall(1, true)); D.Trouble->Dim = 0; }
		DashK = 0;
		if (D.Headlights) D.Headlights(false);
	}
	else { if (D.Trouble) D.Trouble->Dim = 1; if (D.KeyStall) D.KeyStall(); }
	bLightsOwed = bInCab;
	// the radio on, on 1090: the far station, coming and going in its static
	if (FS47Receiver* R = Radio()) { R->Set(true, TEXT("far")); R->Carrier(0); }
	// the odometer: 31.4 miles more than it read when the truck set out from SARO
	const double Now = D.Odometer ? D.Odometer() : 0;
	const TOptional<double> AtSaro = D.OdoAtSaro ? D.OdoAtSaro() : TOptional<double>();
	const double FromM = AtSaro ? *AtSaro : Now, More = FromM + MILES - Now;
	if (More > 0 && D.AddMiles) D.AddMiles(More);
	FrostK = 1; if (D.Frost) D.Frost(1);
	UE_LOG(LogS47TruckTaken, Display, TEXT("S47: taken in the truck: woken %s at %.1f %.1f"), bInCab ? TEXT("at the wheel") : TEXT("on the verge"), P->X, P->Z);
	return FVector(P->X, D.Ground ? D.Ground(P->X, P->Z) : 0, P->Z);
}

void FS47TruckTaken::Woke(const FString& A0, const FString& B, bool bInCab)
{
	(void)A0;
	// (the clock the warning came at, as the corner clock had it, not the light's)
	const bool K = bKnocksOwed;
	const FString A = Clock(At);
	const FString Knocks = K ? TEXT("Then three knocks on the roof of the cab, slow, from above") : TEXT("Then the three knocks on the roof of the cab again, slow, from above");
	if (D.Note)
	{
		D.Note(bInCab
			? FString::Printf(TEXT("%s. The turn-off to SARO, on B. The radio slid over to a bare carrier, the engine died under me and the headlights sank. %s At forty seconds every light in the truck went out. %s, and the light."), *A, Turns ? TEXT("The key would not catch.") : TEXT("I never turned the key."), *Knocks)
			: FString::Printf(TEXT("%s. The turn-off to SARO, on B. The engine died under me and the radio went to a bare carrier%s. I got out, and did not get far enough. %s, behind me, and the light."), *A, Turns ? TEXT(", and the key would not catch") : TEXT(""), *Knocks));
		D.Note(bInCab
			? FString::Printf(TEXT("%s. At the wheel, three hundred metres further on, in the southbound lane with the nose to the north. The engine off, every light off. The radio on, on 1090. I never have it on 1090."), *B)
			: FString::Printf(TEXT("%s. On my back on the verge. The truck beside me, three hundred metres on from where I left it, the nose to the north. Its radio on, on 1090."), *B));
		D.Note(FString::Printf(TEXT("%s. The odometer reads %s miles more than it did at SARO."), *B, TEXT("31.4")));
	}
	if (D.Seen) D.Seen->Add(FString::Printf(TEXT("tt:taken@%d"), int32(JsRound(At))));
}

FString FS47TruckTaken::BackText() const { return TEXT("At the wheel. The truck stands in the southbound lane, facing north. The engine is off. The radio is on, on 1090."); }

void FS47TruckTaken::WakeStep(double Dt, const FS47TruckTakenIn& S)
{
	T += Dt;
	// dark while one of them is there; then the instruments come on by themselves, OIL and VOLTS, and
	// the radio on 1090 with them
	if (S.bOver) bOverSeen = true;
	if (bTakenIn && S.bInCab)
	{
		const double Want = S.bOver || (S.Stage != TEXT("calm") && S.Stage != TEXT("back")) ? 0 : 1;
		DashK += (Want - DashK) * FMath::Min(1.0, Dt * 2.5);
		if (D.Trouble && D.Trouble->Kind() == ES47Trouble::Stall) D.Trouble->Dim = DashK > 0.98 ? 1 : DashK;
	}
	if (S.Stage == TEXT("calm") && T > 2)
	{
		if (bKnocksOwed && D.Seen && !D.Seen->Contains(TEXT("ne:knocks1"))) D.Seen->Add(TEXT("ne:knocks1"));
		Ended = TEXT("taken");
		if (D.Toast) D.Toast(TEXT("The odometer reads 31.4 miles more than it did at SARO."), 4.6f);
		if (D.Save) D.Save();
		Phase = TEXT("tail"); T = 0;
	}
}

// ---------- endings ----------

void FS47TruckTaken::End(const FString& How, const FS47TruckTakenIn& S)
{
	(void)S;
	const FString A = Clock(At);
	Ended = How;
	if (How == TEXT("key"))
	{
		if (D.Seen) D.Seen->Add(FString::Printf(TEXT("tt:key@%d"), int32(JsRound(At))));
		if (D.Toast) D.Toast(TEXT("It catches. The lights come up, and the radio."), 3.4f);
		if (D.Note) D.Note(FString::Printf(TEXT("%s. The turn-off to SARO, on B. The radio slid over to a bare carrier, the engine died under me and the headlights sank. The crickets had stopped. It caught on the third turn of the key, and it all came back at once: the engine, the lights, the radio."), *A));
	}
	else if (How == TEXT("ran"))
	{
		if (D.Seen) D.Seen->Add(FString::Printf(TEXT("tt:ran@%d"), int32(JsRound(At))));
		if (D.KeyStall) D.KeyStall();
		// (out and away after the knocks had come: heard, they were the night's, if it still owed them)
		if (bKnocked && bKnocksOwed && D.Seen && !D.Seen->Contains(TEXT("ne:knocks1"))) D.Seen->Add(TEXT("ne:knocks1"));
		const FString Knocks = bKnocked ? TEXT(" Behind me, three knocks on the roof of the cab, slow, from above.") : TEXT("");
		// (on the screen too, as the key's way out has its toast: the player is out in the dark with nothing else)
		if (D.Toast) D.Toast(TEXT("The crickets, all at once. Behind you the truck stands in the road with its lights out."), 4.2f);
		if (D.Note) D.Note(FString::Printf(TEXT("%s. The turn-off to SARO, on B. The engine died under me, the radio went to a bare carrier and the headlights sank%s. I got out and ran%s.%s When I looked back, the truck was standing in the road with its lights out. Then the crickets again."),
			*A, Turns ? TEXT(", and the key would not catch") : TEXT(""), North > RUN * 0.6 ? TEXT(", back up the shoulder towards SARO") : TEXT(""), *Knocks));
	}
	if (D.Save) D.Save();
	Phase = How == TEXT("off") ? TEXT("idle") : TEXT("tail"); T = 0;
	UE_LOG(LogS47TruckTaken, Display, TEXT("S47: taken in the truck: ended, %s"), *How);
}

void FS47TruckTaken::Reset()
{
	if (Phase == TEXT("warning") || Phase == TEXT("taken")) Release();
	if ((bWokeHere || FrostK > 0) && D.Frost) D.Frost(0);
	bLightsOwed = false;
	Phase = TEXT("idle"); T = 0; Free.Reset(); bArmed = false; Pass.Reset(); Dist.Reset(); Plan.Reset();
	bWokeHere = false; FrostK = 0; bForce = false; LadderPasses = 0; FailedTold.Reset();
}

FString FS47TruckTaken::Text() const
{
	return FString::Printf(TEXT("truck taken %s%s tries %d%s%s"), *Phase, Taken() ? TEXT(" had") : TEXT(""), Tries(),
		Dist ? *FString::Printf(TEXT(" %.0fm"), *Dist) : TEXT(""), Free.IsEmpty() ? TEXT("") : *(TEXT(" ") + Free));
}

bool FS47TruckTaken::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s truck taken: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	FS47TownSeen Seen;
	FS47TruckTrouble Trouble;
	TArray<FString> Notes;
	int32 Begun = 0;
	double Held = 0, Miles = 0;
	FS47TruckTakenDeps D;
	D.Trouble = &Trouble; D.Seen = &Seen;
	D.Begin = [&Begun] { Begun++; };
	D.Hold = [&Held](double K) { Held = K; };
	D.Truck = [] { FS47TruckTakenDeps::FTruckAt A; A.X = -22; A.Z = 30; A.H = UE_DOUBLE_PI; return TOptional<FS47TruckTakenDeps::FTruckAt>(A); };
	D.MoveDriven = [](double, double, double) { return true; };
	D.Odometer = [&Miles] { return Miles; };
	D.OdoAtSaro = [] { return TOptional<double>(2.0); };
	D.AddMiles = [&Miles](double M) { Miles += M; };
	D.Note = [&Notes](const FString& T) { Notes.Add(T); };
	FS47TruckTaken Tt(D);
	// over B southbound at speed, the curve's peak, the time away allowed: the warning
	FS47TruckTakenIn In;
	In.bOk = true; In.bMay = true; In.bAllowed = true; In.bTrip = true; In.Speed = 8; In.South = 1; In.Clock = 3 * 3600;
	const double Bz = S47Lines::HighwayZ(TEXT('B'));
	In.At = FVector2D(0, Bz - 120);
	Tt.Update(0.1, In);
	Check(Tt.Phase == TEXT("idle") && Tt.Dist && *Tt.Dist > 16, TEXT("off B, nothing"));
	In.At = FVector2D(0, Bz);
	Tt.Update(0.1, In);
	Check(Tt.Phase == TEXT("warning") && Begun == 1 && Held == 3.2 && Trouble.Kind() == ES47Trouble::Stall && Seen.Keys.Num() == 1, TEXT("the warning comes over B, the truck held, the engine dead"));
	// a rung of the truck on B due takes the pass
	FS47TruckTaken Lad(D);
	FS47TruckTakenIn L2 = In; L2.bDue = true; L2.At = FVector2D(0, Bz - 120);
	Lad.Update(0.1, L2); L2.At = FVector2D(0, Bz); Lad.Update(0.1, L2);
	Check(Lad.Phase == TEXT("idle") && Lad.LadderPasses == 1, TEXT("a rung that is due has the pass"));
	// the key after :40: nothing
	Check(!Tt.UseKey(39.0) && Tt.UseKey(40.5), TEXT("after :40 the key does nothing, not even the starter"));
	// the waking: 300 m on, the nose north, 31.4 miles on the odometer from SARO's reading
	const TOptional<FVector> W = Tt.Wake(1200, true);
	Check(W && FMath::IsNearlyEqual(W->X, -25.8) && FMath::IsNearlyEqual(W->Z, 330.0) && FMath::IsNearlyEqual(Miles, 33.4) && Tt.Phase == TEXT("wake"), TEXT("woken 300 m on, the odometer 31.4 miles over SARO's"));
	Tt.Woke(TEXT("03:00"), TEXT("03:25"), true);
	Check(Notes.Num() == 3 && Notes[2] == TEXT("03:25. The odometer reads 31.4 miles more than it did at SARO.") && Tt.Taken(), TEXT("the notes, and taken tonight"));
	return bOk;
}
