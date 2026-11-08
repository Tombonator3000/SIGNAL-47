#include "S47GearFlicker.h"

#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "S47Audio.h"
#include "S47FieldRadio.h"
#include "S47Flashlight.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Omen.h"
#include "S47Radio.h"
#include "S47Trouble.h"
#include "S47World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Gear, Log, All);

namespace S47GearLocal
{
	// The gear comes on quicker than it lets go: a light that passes leaves the lamp unsteady a moment after.
	constexpr double RiseTau = 0.5, FallTau = 1.6;
	const TCHAR* const From = TEXT("gear");
	const TCHAR* const BandNames[] = { TEXT("nothing"), TEXT("faint"), TEXT("nearer"), TEXT("close"), TEXT("on you") };
}

// ---------- the rules ----------

int32 FS47GearFlicker::Band(double Level)
{
	return Level < 0.06 ? 0 : Level < 0.3 ? 1 : Level < 0.6 ? 2 : Level < 0.85 ? 3 : 4;
}

double FS47GearFlicker::LampSag(double Level) { return 0.55 * FMath::SmoothStep(0.06, 0.95, FMath::Clamp(Level, 0.0, 1.0)); }

double FS47GearFlicker::LampFlicker(double Level)
{
	const double L = FMath::Clamp(Level, 0.0, 1.0);
	return L < 0.12 ? 0.0 : 0.15 + 0.8 * FMath::SmoothStep(0.12, 0.9, L);
}

double FS47GearFlicker::RadioBreak(double Level) { return FMath::SmoothStep(0.04, 0.85, FMath::Clamp(Level, 0.0, 1.0)); }

double FS47GearFlicker::TruckDim(double Level) { return 1.0 - 0.65 * FMath::SmoothStep(0.15, 0.95, FMath::Clamp(Level, 0.0, 1.0)); }

double FS47GearFlicker::TruckFlickerEvery(double Level)
{
	return Level < 0.3 ? 0.0 : FMath::Lerp(9.0, 1.6, FMath::SmoothStep(0.3, 1.0, FMath::Min(1.0, Level)));
}

double FS47GearFlicker::CoughEvery(double Level)
{
	return Level < 0.6 ? 0.0 : FMath::Lerp(12.0, 2.5, FMath::SmoothStep(0.6, 1.0, FMath::Min(1.0, Level)));
}

double FS47GearFlicker::DriftEvery(double Level)
{
	return Level < 0.06 ? 0.0 : FMath::Lerp(14.0, 1.5, FMath::SmoothStep(0.06, 1.0, FMath::Min(1.0, Level)));
}

double FS47GearFlicker::ClockJumpEvery(double Level)
{
	return Level < 0.6 ? 0.0 : FMath::Lerp(14.0, 2.5, FMath::SmoothStep(0.6, 1.0, FMath::Min(1.0, Level)));
}

int32 FS47GearFlicker::JumpMinutes(double Level, float Roll)
{
	const double Rv = FMath::Clamp(double(Roll), 0.0, 0.999);
	const int32 Mag = 1 + FMath::FloorToInt(Rv * FMath::Lerp(4.0, 39.0, FMath::Clamp(Level, 0.0, 1.0)));
	// Mostly forward: time that went by without them.
	return Rv < 0.2 ? -Mag : Mag;
}

double FS47GearFlicker::WordLoss(double Level)
{
	return Level < 0.3 ? 0.0 : 0.45 * FMath::SmoothStep(0.3, 1.0, FMath::Min(1.0, Level));
}

FString FS47GearFlicker::ShiftClock(const FString& Real, int32 Minutes)
{
	TArray<FString> Bits;
	Real.ParseIntoArray(Bits, TEXT(":"), false);
	if (Bits.Num() < 2 || Bits[0].Len() != 2 || Bits[1].Len() != 2 || !Bits[0].IsNumeric() || !Bits[1].IsNumeric()) return Real;
	const int32 Total = ((FCString::Atoi(*Bits[0]) * 60 + FCString::Atoi(*Bits[1]) + Minutes) % 1440 + 1440) % 1440;
	FString Out = FString::Printf(TEXT("%02d:%02d"), Total / 60, Total % 60);
	for (int32 I = 2; I < Bits.Num(); ++I) Out += TEXT(":") + Bits[I];
	return Out;
}

FString FS47GearFlicker::ClockSays(const FString& Real) const
{
	if (ClockLeft <= 0 || Real.IsEmpty()) return Real;
	if (bScramble) return TEXT("88:88");
	return ShiftClock(Real, ClockShift);
}

FString FS47GearFlicker::Broken(const FString& Line, int32 Seed) const
{
	const double Loss = WordLoss(K);
	if (Loss <= 0) return Line;
	TArray<FString> Words;
	Line.ParseIntoArrayWS(Words);
	if (Words.Num() < 3) return Line;
	FRandomStream Rng(Seed * 7919 + 47);
	FString Out = Words[0];
	bool bGap = false;
	for (int32 I = 1; I < Words.Num(); ++I)
	{
		if (Rng.FRand() < Loss) { if (!bGap) Out += TEXT(" ..."); bGap = true; continue; }
		Out += TEXT(" ") + Words[I];
		bGap = false;
	}
	return Out;
}

// ---------- in play ----------

void FS47GearFlicker::Release()
{
	if (M && M->Night && M->Night->Flashlight)
	{
		M->Night->Flashlight->Flicker(0, S47GearLocal::From);
		M->Night->Flashlight->Sag(0, S47GearLocal::From);
	}
	if (M && M->FieldRadio) M->FieldRadio->SetGearNoise(0);
	if (M && M->World) M->World->Trouble.GearDim = 1;
	ClockLeft = 0; bScramble = false; bTouched = false; LastBand = 0;
}

void FS47GearFlicker::Reset()
{
	Release();
	K = 0; TruckFlickerIn = CoughIn = DriftIn = ClockIn = 0;
}

void FS47GearFlicker::Apply(double Dt)
{
	// The lamp: the beam sags, then blinks, then goes out for moments. No battery is spent on it.
	if (M->Night && M->Night->Flashlight)
	{
		FS47Flashlight& Lamp = *M->Night->Flashlight;
		Lamp.Flicker(LampFlicker(K), S47GearLocal::From);
		Lamp.Sag(LampSag(K), S47GearLocal::From);
	}
	// The walkie, SARO's AM set and the receiver monitor share one interference voice (FS47FieldRadio).
	if (M->FieldRadio) M->FieldRadio->SetGearNoise(RadioBreak(K));
	// The truck, while it is driven: its electrics sag, its lights stutter, and close by the engine coughs.
	if (M->World)
	{
		FS47TruckTrouble& Tr = M->World->Trouble;
		const bool bDrive = M->World->bDriving;
		Tr.GearDim = bDrive ? TruckDim(K) : 1.0;
		const double FlickerGap = TruckFlickerEvery(K), CoughGap = CoughEvery(K);
		if (CoughGap > 0) CoughIn = FMath::Min(CoughIn, CoughGap);
		if (FlickerGap > 0) TruckFlickerIn = FMath::Min(TruckFlickerIn, FlickerGap);
		if (bDrive && CoughGap > 0 && (CoughIn -= Dt) <= 0 && Tr.Kind() == ES47Trouble::None)
		{
			CoughIn = CoughGap * FMath::FRandRange(0.6f, 1.4f);
			FS47TroubleOpts Opts; Opts.Len = FMath::Lerp(0.5, 1.6, K);
			if (Tr.Start(ES47Trouble::Sputter, Opts)) ++CoughCount;
		}
		if (bDrive && FlickerGap > 0 && (TruckFlickerIn -= Dt) <= 0 && Tr.Kind() == ES47Trouble::None)
		{
			TruckFlickerIn = FlickerGap * FMath::FRandRange(0.6f, 1.4f);
			FS47TroubleOpts Opts; Opts.Len = FMath::Lerp(0.25, 0.9, K);
			if (Tr.Start(ES47Trouble::Flicker, Opts)) ++FlickerCount;
		}
	}
	// The truck's AM set, on: bursts of static, more and longer nearer.
	const double DriftGap = DriftEvery(K);
	if (DriftGap > 0) DriftIn = FMath::Min(DriftIn, DriftGap);
	if (M->CarRadio && M->CarRadio->Live() && DriftGap > 0 && (DriftIn -= Dt) <= 0)
	{
		DriftIn = DriftGap * FMath::FRandRange(0.6f, 1.4f);
		if (M->CarRadio->Drift(FMath::Lerp(0.5, 2.4, K), FMath::Lerp(0.35, 1.0, K))) ++DriftCount;
	}
	// The clocks: close by, the watch and the dash jump a few minutes for a moment; on top of it they show nothing sensible.
	const double JumpGap = ClockJumpEvery(K);
	if (ClockLeft > 0) ClockLeft -= Dt;
	else if (JumpGap > 0)
	{
		ClockIn = FMath::Min(ClockIn, JumpGap);
		if ((ClockIn -= Dt) <= 0)
		{
			ClockIn = JumpGap * FMath::FRandRange(0.6f, 1.4f);
			ClockLeft = FMath::FRandRange(0.35f, 1.1f);
			bScramble = K > 0.85 && FMath::FRand() < 0.5f;
			ClockShift = JumpMinutes(K, FMath::FRand());
			++Jumps;
			// the watch's own chirp, twice, as when it is set
			if (M->Audio) { M->Audio->Beep(4100.f, 0.022f, 0.018f); M->Audio->Beep(4100.f, 0.022f, 0.018f, 0.09f); }
		}
	}
	else ClockLeft = 0;
}

void FS47GearFlicker::Step(double Dt)
{
	if (!bLogChecked) { bLogChecked = true; bLog = FParse::Param(FCommandLine::Get(), TEXT("S47GearLog")); }
	if (!M || !M->IsFreeform() || M->Mode != US47Main::EMode::Play || !M->Omen.IsValid())
	{
		if (bTouched || K > 0) Reset();
		return;
	}
	if (M->bPausedByMenu || M->ModalOpen() || !FMath::IsFinite(Dt) || Dt <= 0) return;
	const double Want = M->Omen->Level();
	const double Ease = Want > K ? S47GearLocal::RiseTau : S47GearLocal::FallTau;
	K += (Want - K) * (1.0 - FMath::Exp(-Dt / Ease));
	if (Want <= 0 && K < 0.002) K = 0;
	K = FMath::Clamp(K, 0.0, 1.0);
	if (K <= 0 && !bTouched) return;
	Apply(Dt);
	bTouched = K > 0;
	if (!bTouched) Release();
	const int32 NewBand = Band(K);
	if (NewBand != LastBand)
	{
		if (bLog)
		{
			UE_LOG(LogS47Gear, Display, TEXT("S47: gear %s (%.2f): lamp sag %.2f flicker %.2f, radio %.2f, truck %.2f, coughs %d, jumps %d"),
				S47GearLocal::BandNames[NewBand], K, LampSag(K), LampFlicker(K), RadioBreak(K), TruckDim(K), CoughCount, Jumps);
		}
		LastBand = NewBand;
	}
}

bool FS47GearFlicker::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s gear: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bCond; };
	Check(LampSag(0) == 0 && LampFlicker(0) == 0 && RadioBreak(0) == 0 && TruckDim(0) == 1 && TruckFlickerEvery(0) == 0 && CoughEvery(0) == 0
		&& DriftEvery(0) == 0 && ClockJumpEvery(0) == 0 && WordLoss(0) == 0 && Band(0) == 0, TEXT("with nothing out there the gear is as it always is"));
	Check(LampFlicker(0.1) == 0 && CoughEvery(0.5) == 0 && ClockJumpEvery(0.55) == 0 && TruckFlickerEvery(0.25) == 0 && WordLoss(0.25) == 0,
		TEXT("each failure keeps to its own band: the far ones never cough the engine or jump the clock"));
	bool bGrows = true;
	double PrevSag = 0, PrevFlick = 0, PrevRadio = 0, PrevDim = 1, LastCough = 1e9, LastJump = 1e9, LastDrift = 1e9; int32 PrevBand = 0;
	for (int32 I = 0; I <= 100; ++I)
	{
		const double Lv = I / 100.0;
		bGrows &= LampSag(Lv) >= PrevSag && LampFlicker(Lv) >= PrevFlick && RadioBreak(Lv) >= PrevRadio && TruckDim(Lv) <= PrevDim && Band(Lv) >= PrevBand;
		if (CoughEvery(Lv) > 0) { bGrows &= CoughEvery(Lv) <= LastCough; LastCough = CoughEvery(Lv); }
		if (ClockJumpEvery(Lv) > 0) { bGrows &= ClockJumpEvery(Lv) <= LastJump; LastJump = ClockJumpEvery(Lv); }
		if (DriftEvery(Lv) > 0) { bGrows &= DriftEvery(Lv) <= LastDrift; LastDrift = DriftEvery(Lv); }
		PrevSag = LampSag(Lv); PrevFlick = LampFlicker(Lv); PrevRadio = RadioBreak(Lv); PrevDim = TruckDim(Lv); PrevBand = Band(Lv);
	}
	Check(bGrows, TEXT("nearer and worse is always more: the lamp sags and blinks more, the radio breaks more, the truck dims more, everything comes oftener"));
	// (a hair of rounding: 0.15 + 0.8 is 0.9500000000000001 in doubles)
	constexpr double Eps = 1e-9;
	Check(LampSag(1) <= 0.55 + Eps && LampFlicker(1) <= 0.95 + Eps && TruckDim(1) >= 0.35 - Eps && CoughEvery(1) >= 2 && ClockJumpEvery(1) >= 2,
		TEXT("even on top of it there is something left to see by, and a moment between coughs"));
	Check(Band(0.1) == 1 && Band(0.4) == 2 && Band(0.7) == 3 && Band(0.9) == 4, TEXT("four bands a player can learn"));
	Check(ShiftClock(TEXT("23:50"), 15) == TEXT("00:05") && ShiftClock(TEXT("00:03"), -5) == TEXT("23:58") && ShiftClock(TEXT("02:14:30"), 7) == TEXT("02:21:30"), TEXT("a jumped clock wraps the day and keeps its seconds"));
	Check(ShiftClock(TEXT("LATE AFTERNOON"), 5) == TEXT("LATE AFTERNOON") && ShiftClock(FString(), 5).IsEmpty(), TEXT("what is not a clock is left alone"));
	int32 Backward = 0, Big = 0; bool bMoves = true;
	for (int32 I = 0; I < 100; ++I) { const int32 J = JumpMinutes(1.0, I / 100.f); Backward += J < 0 ? 1 : 0; Big = FMath::Max(Big, FMath::Abs(J)); bMoves &= J != 0; }
	Check(bMoves && Backward > 5 && Backward < 40 && Big <= 40 && FMath::Abs(JumpMinutes(0.6, 0.99f)) < Big, TEXT("a jump always moves: mostly forward, now and then back; bigger nearer"));
	FS47GearFlicker Probe(nullptr);
	Check(Probe.ClockSays(TEXT("23:41")) == TEXT("23:41") && Probe.Broken(TEXT("DALE: where are you, answer me"), 3) == TEXT("DALE: where are you, answer me"), TEXT("no level, no jump and no lost words"));
	Probe.K = 1;
	const FString Said = TEXT("Get back inside the building and close the doors, do you hear me");
	const FString First = Probe.Broken(Said, 5), Second = Probe.Broken(Said, 5);
	Check(First == Second && First != Said && First.StartsWith(TEXT("Get")) && First.Contains(TEXT("...")), TEXT("close by, words drop out of the walkie, the same way each time for the same line"));
	Probe.Step(1); Probe.Reset();
	Check(Probe.Level() == 0 && Probe.ClockJumps() == 0, TEXT("without a game it does nothing"));
	return bOk;
}
