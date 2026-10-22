#include "S47Trouble.h"

#include "S47Drive.h"
#include "S47EngineSound.h"
#include "S47Truck.h"

namespace
{
	double Rnd(double A, double B) { return A + FMath::FRand() * (B - A); }
	double Random() { return FMath::FRand(); }
}

const TCHAR* S47TroubleName(ES47Trouble K)
{
	switch (K)
	{
	case ES47Trouble::Sputter: return TEXT("sputter");
	case ES47Trouble::Stall: return TEXT("stall");
	case ES47Trouble::Flicker: return TEXT("flicker");
	case ES47Trouble::Blink: return TEXT("blink");
	case ES47Trouble::DashOut: return TEXT("dashOut");
	default: return TEXT("");
	}
}

FS47TruckTrouble::FS47TruckTrouble() : Next(Rnd(0.5, 1.5)) {}

bool FS47TruckTrouble::WantsKey() const
{
	return A && A->Kind == ES47Trouble::Stall && A->bKey && !A->bHold && (A->Stage == EStage::Dead || A->Stage == EStage::Pause);
}

bool FS47TruckTrouble::Dead() const { return A && A->Kind == ES47Trouble::Stall && A->Stage != EStage::Catching; }

FString FS47TruckTrouble::Stage() const
{
	if (!A || A->Kind != ES47Trouble::Stall) return FString();
	switch (A->Stage)
	{
	case EStage::Dying: return TEXT("dying");
	case EStage::Dead: return TEXT("dead");
	case EStage::Pause: return TEXT("pause");
	case EStage::Cranking: return TEXT("cranking");
	case EStage::Catching: return TEXT("catching");
	default: return FString();
	}
}

int32 FS47TruckTrouble::Blinks() const
{
	return A && A->Kind == ES47Trouble::Blink && A->Count > 0 ? int32(FMath::Min(A->Count, FMath::Floor(A->T / A->Per) + 1)) : 0;
}

bool FS47TruckTrouble::BlinkDark() const
{
	return A && A->Kind == ES47Trouble::Blink && A->Count > 0 && A->T < A->Count * A->Per && FMath::Fmod(A->T, A->Per) < A->Per / 2;
}

bool FS47TruckTrouble::Start(ES47Trouble InKind, const FS47TroubleOpts& O)
{
	if (A && A->Kind == ES47Trouble::Stall && InKind != ES47Trouble::Stall) return false;
	const double Cnt = InKind == ES47Trouble::Blink ? FMath::Max(0.0, FMath::RoundToDouble(O.Count.Get(0))) : 0, Per = O.Per.Get(0.5);
	const double Len = O.Len ? *O.Len : (InKind == ES47Trouble::Sputter ? Rnd(1.2, 2.6) : InKind == ES47Trouble::Flicker ? Rnd(0.8, 2.4)
		: InKind == ES47Trouble::Blink ? (Cnt > 0 ? Cnt * Per + 0.05 : 0.55) : InKind == ES47Trouble::DashOut ? Rnd(2.5, 6) : 0);
	const double Need = !O.Tries ? (Random() < 0.55 ? 2 : 1) : FMath::Max(1.0, *O.Tries);
	FActive N;
	N.Kind = InKind; N.Len = Len; N.bHold = O.bHold; N.bKey = O.bKey; N.Need = Need; N.Count = Cnt; N.Per = Per; N.bSoft = O.bSoft;
	A = N;
	if (InKind == ES47Trouble::Stall) { A->Stage = O.bDead ? EStage::Dead : EStage::Dying; Turns = 0; if (O.bDead) A->Heard = EStage::Dying; }
	Count++;
	if (OnTrouble) OnTrouble(InKind);
	return true;
}

void FS47TruckTrouble::Release()
{
	if (A && A->Kind == ES47Trouble::Stall) { A->bHold = false; if (A->Stage == EStage::Dead) A->St = 0; }
}

void FS47TruckTrouble::CatchOn(double N) { if (A && A->Kind == ES47Trouble::Stall) A->Need = FMath::Max(1.0, N); }

bool FS47TruckTrouble::TurnKey()
{
	if (!WantsKey() || !A) return false;
	A->Stage = EStage::Cranking; A->St = 0; Turns++;
	return true;
}

void FS47TruckTrouble::Reset(FS47Truck* Truck, FS47DriveController* Drive)
{
	A.Reset(); Power = 1; Dim = 1;
	if (Drive) { Drive->bEngine = true; Drive->Power = 1; }
	if (Truck && bTouched) { Truck->SetLightLevel(1); Truck->SetDashLevel({}); Truck->SetWarnings({}); Truck->SetGaugePower(1); }
	bTouched = false;
}

void FS47TruckTrouble::HandOver(FS47Truck& Truck, FS47DriveController& Drive)
{
	if (!bTouched) return;
	Drive.bEngine = true; Drive.Power = 1;
	Truck.SetLightLevel(1); Truck.SetDashLevel({}); Truck.SetWarnings({}); Truck.SetGaugePower(1);
}

void FS47TruckTrouble::Step(double Dt, FS47DriveController& Drive, FS47Truck& Truck, FS47EngineSound* Sound, bool bMayRandom)
{
	// ---------- the random ones: only at speed, now and then ----------
	if (bRandom && bMayRandom && !A && Drive.bEngine && FMath::Abs(Drive.Speed) > 6)
	{
		Next -= Dt / Every;
		if (Next <= 0)
		{
			Next = Rnd(0.5, 1.5);
			const double R = Random();
			Start(R < 0.4 ? ES47Trouble::Flicker : R < 0.7 ? ES47Trouble::Sputter : R < 0.85 ? ES47Trouble::DashOut : ES47Trouble::Stall);
		}
	}
	if (!A)
	{
		Power = 1;
		// (nothing wrong with the truck itself, but its electrics held down from outside)
		if (Dim < 1) { bTouched = true; Truck.SetLightLevel(Dim); Truck.SetDashLevel(Dim); Truck.SetWarnings({}); return; }
		if (bTouched) Reset(&Truck, &Drive);
		return;
	}
	FActive& Act = *A;
	Act.T += Dt;
	bTouched = true;
	double Light = 1, Dash = 1, Gauges = 1;
	FS47Truck::FWarn Warn;
	// a pattern: hold a value for a short random time, then pick another
	auto Beat = [&](double Lo, double Hi, TFunctionRef<double()> Pick) { Act.Beat -= Dt; if (Act.Beat <= 0) { Act.Beat = Rnd(Lo, Hi); Act.V = Pick(); } return Act.V; };
	bool bDone = false;

	if (Act.Kind == ES47Trouble::Sputter)
	{
		// misfires: the pull drops out in bursts, each with a cough
		// (it begins with a miss: a misfire that never missed would be nothing)
		const double Was = Act.V;
		const double P = Beat(0.07, 0.24, [&] { return Act.T <= Dt + 1e-6 || Random() < 0.55 ? Rnd(0.05, 0.3) : 1.0; });
		Drive.Power = P;
		if (P < 0.5 && Was >= 0.5 && Sound) Sound->Cough(0.6 + Random() * 0.4);
		Dash = P < 0.5 ? 0.85 : 1;
		Warn.bVolts = P < 0.2;
		if (Act.T > Act.Len) { Drive.Power = 1; bDone = true; }
	}
	else if (Act.Kind == ES47Trouble::Flicker)
	{
		const double K = Beat(0.03, 0.16, [&] { return Random() < 0.5 ? Rnd(0, 0.25) : Rnd(0.7, 1); });
		Light = K; Dash = K;
		if (Act.T > Act.Len) bDone = true;
	}
	else if (Act.Kind == ES47Trouble::Blink && Act.Count > 0)
	{
		// counted: every light out for the dark half of each, evenly, and on again; no stutter (soft:
		// a smooth dip to under half, each the same, for slow fades)
		const double Ph = Act.T < Act.Count * Act.Per ? FMath::Fmod(Act.T, Act.Per) / Act.Per : 1;
		Light = Act.bSoft ? 1 - 0.58 * FMath::Square(FMath::Sin(UE_DOUBLE_PI * FMath::Min(1.0, Ph * 1.6))) : BlinkDark() ? 0.02 : 1;
		Dash = Light;
		if (Act.T > Act.Len) bDone = true;
	}
	else if (Act.Kind == ES47Trouble::Blink)
	{
		Light = Act.T < 0.14 ? 0.03 : Act.T < 0.42 ? Beat(0.03, 0.07, [&] { return Random() < 0.45 ? Rnd(0.15, 0.5) : 1.0; }) : 1;
		Dash = Light;
		if (Act.T > Act.Len) bDone = true;
	}
	else if (Act.Kind == ES47Trouble::DashOut)
	{
		// out at once with a stutter at both ends
		const double Edge = FMath::Min(Act.T, Act.Len - Act.T);
		Dash = Edge < 0.25 ? Beat(0.03, 0.08, [&] { return Random() < 0.5 ? 0.0 : 1.0; }) : 0;
		Gauges = 0;
		if (Act.T > Act.Len) bDone = true;
	}
	else
	{
		const FOut O = StallStep(Dt, Act, Drive, Sound);
		Light = O.Light; Dash = O.Dash; Warn.bOil = O.bOil; Warn.bVolts = O.bVolts;
		bDone = Act.Kind == ES47Trouble::None;
	}
	if (bDone) A.Reset();

	Power = A && A->Kind == ES47Trouble::Stall && A->Stage == EStage::Cranking ? 0.04 : A && (A->Kind == ES47Trouble::Flicker || A->Kind == ES47Trouble::Blink) ? Light : 1;
	Truck.SetLightLevel(Light * Dim);
	Truck.SetDashLevel(Dash * Dim);
	Truck.SetGaugePower(Gauges);
	// (the warning lamps go with every other light when the battery has gone)
	Truck.SetWarnings(Dim < 0.12 ? FS47Truck::FWarn() : Warn);
}

FS47TruckTrouble::FOut FS47TruckTrouble::StallStep(double Dt, FActive& Act, FS47DriveController& Drive, FS47EngineSound* Sound)
{
	Act.St += Dt;
	FOut O;
	auto Ign = [&O] { O.bOil = true; O.bVolts = true; };   // the key on and the engine stopped: oil pressure and charge lamps
	switch (Act.Stage)
	{
	case EStage::Dying:
		if (Act.Heard != EStage::Dying) { Act.Heard = EStage::Dying; if (Sound) Sound->Stall(); }
		Drive.bEngine = false;
		// the alternator stops: the lights sag onto the battery, with a stumble on the way
		O.Light = Act.St < 0.6 ? (Random() < 0.3 ? 0.4 : 0.8) : 0.72; O.Dash = 0.85;
		if (Act.St > 0.5) Ign();
		if (Act.St > 1.3) { Act.Stage = EStage::Dead; Act.St = 0; }
		break;
	case EStage::Dead:
		Act.Heard = EStage::Dead;
		Drive.bEngine = false;   // (a stall found dead never went through dying)
		O.Light = 0.72; O.Dash = 0.85; Ign();
		// the driver turns the key once the truck has slowed (or after a while), unless the story holds it
		if (!Act.bHold && !Act.bKey && (Act.St > 2.4 || (FMath::Abs(Drive.Speed) < 2 && Act.St > 1.2))) { Act.Stage = EStage::Cranking; Act.St = 0; }
		break;
	case EStage::Pause:
		O.Light = 0.72; O.Dash = 0.85; Ign();
		if (!Act.bKey && Act.St > 0.9) { Act.Stage = EStage::Cranking; Act.St = 0; }
		break;
	case EStage::Cranking:
	{
		if (Act.Heard != EStage::Cranking) { Act.Heard = EStage::Cranking; if (Sound) Sound->Crank(1.2); }
		// the starter pulls the battery down: the lights dip in time with the compression strokes
		const double K = 0.5 + 0.5 * FMath::Sin(Act.St * UE_DOUBLE_PI * 2 * 4.5);
		O.Light = 0.28 + 0.12 * K; O.Dash = 0.4 + 0.15 * K; Ign();
		if (Act.St > 1.15)
		{
			Act.Tries++;
			if (Act.bHold || Act.Tries < Act.Need) { Act.Stage = EStage::Pause; Act.St = 0; Act.Heard = EStage::Pause; }
			else { Act.Stage = EStage::Catching; Act.St = 0; Drive.bEngine = true; Drive.Power = 0.3; if (Sound) Sound->Restart(); }
		}
		break;
	}
	case EStage::Catching:
		// it fires, runs rough for a moment and settles; the lamps go out as the oil pressure comes up
		Drive.Power = FMath::Min(1.0, 0.3 + Act.St * 0.7);
		O.Light = FMath::Min(1.0, 0.75 + Act.St * 0.4); O.Dash = 1;
		if (Act.St < 0.35) Ign();
		if (Act.St > 1.0) { Drive.Power = 1; Act.Kind = ES47Trouble::None; }
		break;
	default:
		break;
	}
	return O;
}
