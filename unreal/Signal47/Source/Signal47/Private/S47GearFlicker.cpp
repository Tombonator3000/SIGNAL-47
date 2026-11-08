#include "S47GearFlicker.h"

#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "S47Audio.h"
#include "S47Drive.h"
#include "S47FarmWorld.h"
#include "S47FieldRadio.h"
#include "S47Flashlight.h"
#include "S47Freeform.h"
#include "S47Geo.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Omen.h"
#include "S47OpenLand.h"
#include "S47Radio.h"
#include "S47Trouble.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Gear, Log, All);

namespace S47GearLocal
{
	// The gear comes on quicker than it lets go: a light that passes leaves the lamp unsteady a moment after.
	constexpr double RiseTau = 0.5, FallTau = 1.6;
	// One spell of the lamp cutting out, or of the walkie breaking up, for the log and the notebook: what comes within this long of
	// the last is the same spell.
	constexpr double SpellGap = 6;
	const TCHAR* const From = TEXT("gear");
	const TCHAR* const BandNames[] = { TEXT("nothing"), TEXT("faint"), TEXT("nearer"), TEXT("close"), TEXT("on you") };
	uint32 Scramble(uint32 X) { X ^= X >> 16; X *= 0x7feb352du; X ^= X >> 15; X *= 0x846ca68bu; X ^= X >> 16; return X; }
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

double FS47GearFlicker::ContactFlicker(double Strength, double Pace)
{
	const double S = FMath::Clamp(Strength, 0.0, 1.0);
	if (S <= 0 || !FMath::IsFinite(Pace)) return 0;
	// standing still it hardly shows; walking it blinks; at a run it is as bad as something nearer
	const double Shake = FMath::Clamp(Pace / 3.5, 0.15, 1.0);
	return (0.2 + 0.4 * S) * Shake;
}

double FS47GearFlicker::PathNoise(double MetresFromMast, double BelowMast)
{
	if (!FMath::IsFinite(MetresFromMast)) return 0;
	const double Far = 0.24 * FMath::SmoothStep(900.0, 4500.0, FMath::Max(0.0, MetresFromMast));
	// in a wash or under a rise the set is in the mast's shadow: more of it, and some even near the station
	const double Low = FMath::IsFinite(BelowMast) ? 0.1 * FMath::SmoothStep(6.0, 25.0, BelowMast) * FMath::SmoothStep(250.0, 900.0, MetresFromMast) : 0.0;
	return FMath::Clamp(Far + Low, 0.0, 0.32);
}

bool FS47GearFlicker::BadTank(int32 Seed, int32 OnDay)
{
	const uint32 Hashed = S47GearLocal::Scramble(uint32(Seed) * 2654435761u ^ S47GearLocal::Scramble(uint32(OnDay) + 0x7a4cu));
	return (Hashed & 0xffffu) < 0x4000u;
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
	const double Loss = WordLoss(KRadio);
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
	if (M && M->FieldRadio) { M->FieldRadio->SetGearNoise(0); M->FieldRadio->SetOrdinaryNoise(0, 0); }
	if (M && M->World) M->World->Trouble.GearDim = 1;
	ClockLeft = 0; bScramble = false; bTouched = false; LastBand = 0;
}

void FS47GearFlicker::Reset()
{
	Release();
	K = KLamp = KRadio = KTruck = KClock = 0; TruckFlickerIn = CoughIn = DriftIn = ClockIn = OrdinaryDriftIn = 0; TankCoughIn = 40; RoughIn = 150;
	LampCutsSeen = RadioDropsSeen = -1; SinceCut = 1e9; FeetSpeed = 0; LastFeet = FVector::ZeroVector; LampOrdinary = 0;
}

double FS47GearFlicker::Ease(double Cur, double Want, double Dt) const
{
	const double Lag = Want > Cur ? S47GearLocal::RiseTau : S47GearLocal::FallTau;
	double Out = Cur + (Want - Cur) * (1.0 - FMath::Exp(-Dt / Lag));
	if (Want <= 0 && Out < 0.002) Out = 0;
	return FMath::Clamp(Out, 0.0, 1.0);
}

void FS47GearFlicker::Apply(double Dt)
{
	FS47Omen& Om = *M->Omen;
	const bool bDrive = M->World && M->World->bDriving;
	// The lamp: the beam sags, then blinks, then goes out for moments; and a loose contact blinks it on rough ground, at a walk.
	LampOrdinary = bDrive ? 0.0 : ContactFlicker(Om.Ordinary(ES47Ordinary::Contact), FeetSpeed);
	if (M->Night && M->Night->Flashlight)
	{
		FS47Flashlight& Lamp = *M->Night->Flashlight;
		Lamp.Flicker(FMath::Max(LampFlicker(KLamp), LampOrdinary), S47GearLocal::From);
		Lamp.Sag(LampSag(KLamp), S47GearLocal::From);
	}
	// The walkie, SARO's AM set and the receiver monitor share one interference voice (FS47FieldRadio). The walkie also hears
	// what any set does: distance from the mast and low ground (crackle), a spell when the squelch opens on nothing (hiss), a far
	// car keyed up on the channel (a burst), the wind.
	if (M->FieldRadio)
	{
		double Path = 0;
		if (M->Web && M->Player)
		{
			double Rx = 0, Rz = 0;
			if (S47OpenLand::RoadOf(M->Web->Area(), M->Player->Pos.X, M->Player->Pos.Z, Rx, Rz))
			{
				const double ToMast = FVector2D::Distance(FVector2D(Rx, Rz), FVector2D(S47Geo::SARO_IN_ROAD.X, S47Geo::SARO_IN_ROAD.Z));
				const double Below = S47FarmWorld::FloorRoad(S47Geo::SARO_IN_ROAD.X, S47Geo::SARO_IN_ROAD.Z) - S47FarmWorld::FloorRoad(Rx, Rz);
				Path = PathNoise(ToMast, Below);
			}
		}
		const double Hiss = 0.45 * Om.Ordinary(ES47Ordinary::Skip);
		const double Burst = FMath::Max(0.7 * Om.Ordinary(ES47Ordinary::Traffic), 0.25 * Om.Ordinary(ES47Ordinary::Gust));
		M->FieldRadio->SetGearNoise(RadioBreak(KRadio));
		M->FieldRadio->SetOrdinaryNoise(Hiss, FMath::Max(Path, Burst));
	}
	// The truck, while it is driven: its electrics sag, its lights stutter, and close by the engine coughs. A bad tank misses under
	// load all day; a washboard track makes a headlight stutter now and then.
	if (M->World)
	{
		FS47TruckTrouble& Tr = M->World->Trouble;
		Tr.GearDim = bDrive ? TruckDim(KTruck) : 1.0;
		const double FlickerGap = TruckFlickerEvery(KTruck), CoughGap = CoughEvery(KTruck);
		if (CoughGap > 0) CoughIn = FMath::Min(CoughIn, CoughGap);
		if (FlickerGap > 0) TruckFlickerIn = FMath::Min(TruckFlickerIn, FlickerGap);
		const FS47DriveController* Cab = bDrive ? M->World->CabDrive() : nullptr;
		if (bDrive && CoughGap > 0 && (CoughIn -= Dt) <= 0 && Tr.Kind() == ES47Trouble::None)
		{
			CoughIn = CoughGap * FMath::FRandRange(0.6f, 1.4f);
			FS47TroubleOpts Opts; Opts.Len = FMath::Lerp(0.5, 1.6, KTruck);
			if (Tr.Start(ES47Trouble::Sputter, Opts)) { ++CoughCount; Om.Felt(ES47OmenChannel::Truck, TEXT("the engine missed"), true, FVector::ZeroVector, false); }
		}
		if (bDrive && FlickerGap > 0 && (TruckFlickerIn -= Dt) <= 0 && Tr.Kind() == ES47Trouble::None)
		{
			TruckFlickerIn = FlickerGap * FMath::FRandRange(0.6f, 1.4f);
			FS47TroubleOpts Opts; Opts.Len = FMath::Lerp(0.25, 0.9, KTruck);
			if (Tr.Start(ES47Trouble::Flicker, Opts)) { ++TruckFlickerCount; Om.Felt(ES47OmenChannel::Truck, TEXT("the headlights stuttered"), true, FVector::ZeroVector, false); }
		}
		if (Cab && Cab->bHandling && M->Freeform && BadTank(M->Freeform->Seed(), M->Freeform->Day()) && Cab->Load > 0.45 && FMath::Abs(Cab->Speed) > 3)
		{
			if ((TankCoughIn -= Dt) <= 0 && Tr.Kind() == ES47Trouble::None)
			{
				TankCoughIn = FMath::FRandRange(50.f, 160.f);
				FS47TroubleOpts Opts; Opts.Len = FMath::FRandRange(0.4f, 0.9f);
				if (Tr.Start(ES47Trouble::Sputter, Opts)) { ++CoughCount; Om.Felt(ES47OmenChannel::Truck, TEXT("the engine missed"), false, FVector::ZeroVector, false); }
			}
		}
		if (Cab && Cab->bHandling && Cab->Surface.Kind == ES47SurfaceKind::Dirt && FMath::Abs(Cab->Speed) > 9)
		{
			if ((RoughIn -= Dt) <= 0 && Tr.Kind() == ES47Trouble::None)
			{
				RoughIn = FMath::FRandRange(120.f, 320.f);
				FS47TroubleOpts Opts; Opts.Len = FMath::FRandRange(0.15f, 0.4f);
				if (Tr.Start(ES47Trouble::Flicker, Opts)) { ++TruckFlickerCount; Om.Felt(ES47OmenChannel::Truck, TEXT("the headlights stuttered"), false, FVector::ZeroVector, false); }
			}
		}
	}
	// The truck's AM set, on: bursts of static, more and longer nearer; and the ordinary fade of a far station in low ground.
	if (M->CarRadio && M->CarRadio->Live())
	{
		const double DriftGap = DriftEvery(KRadio);
		if (DriftGap > 0) DriftIn = FMath::Min(DriftIn, DriftGap);
		if (DriftGap > 0 && (DriftIn -= Dt) <= 0)
		{
			DriftIn = DriftGap * FMath::FRandRange(0.6f, 1.4f);
			if (M->CarRadio->Drift(FMath::Lerp(0.5, 2.4, KRadio), FMath::Lerp(0.35, 1.0, KRadio)))
			{
				++DriftCount; Om.Felt(ES47OmenChannel::Radio, TEXT("the radio lost its station"), true, FVector::ZeroVector, false);
			}
		}
		const double Fade = FMath::Max(Om.Ordinary(ES47Ordinary::Skip), Om.Ordinary(ES47Ordinary::Traffic));
		if (Fade > 0 && (OrdinaryDriftIn -= Dt) <= 0)
		{
			OrdinaryDriftIn = FMath::FRandRange(4.f, 9.f);
			if (M->CarRadio->Drift(FMath::Lerp(0.6, 1.8, Fade), FMath::Lerp(0.3, 0.8, Fade)))
			{
				++DriftCount; Om.Felt(ES47OmenChannel::Radio, TEXT("the radio lost its station"), false, FVector::ZeroVector, false);
			}
		}
	}
	// The clocks: close by, the watch and the dash jump a few minutes for a moment; on top of it they show nothing sensible.
	const double JumpGap = ClockJumpEvery(KClock);
	if (ClockLeft > 0) ClockLeft -= Dt;
	else if (JumpGap > 0)
	{
		ClockIn = FMath::Min(ClockIn, JumpGap);
		if ((ClockIn -= Dt) <= 0)
		{
			ClockIn = JumpGap * FMath::FRandRange(0.6f, 1.4f);
			ClockLeft = FMath::FRandRange(0.35f, 1.1f);
			bScramble = KClock > 0.85 && FMath::FRand() < 0.5f;
			ClockShift = JumpMinutes(KClock, FMath::FRand());
			++Jumps;
			// the watch's own chirp, twice, as when it is set
			if (M->Audio) { M->Audio->Beep(4100.f, 0.022f, 0.018f); M->Audio->Beep(4100.f, 0.022f, 0.018f, 0.09f); }
			Om.Felt(ES47OmenChannel::Clock, TEXT("the watch jumped"), true, FVector::ZeroVector, false);
		}
	}
	else ClockLeft = 0;
}

void FS47GearFlicker::Count(double Dt)
{
	// The lamp cut out in the hand, on foot, with the lamp lit: one spell however many blinks it had, whatever made it.
	SinceCut += Dt;
	if (M->Night && M->Night->Flashlight)
	{
		const FS47Flashlight& Lamp = *M->Night->Flashlight;
		const int32 Cuts = Lamp.CutOuts();
		if (LampCutsSeen < 0 || Cuts < LampCutsSeen) LampCutsSeen = Cuts;
		if (Cuts > LampCutsSeen)
		{
			LampCutsSeen = Cuts;
			const bool bOnFoot = !(M->World && M->World->bDriving) && Lamp.bOn;
			if (bOnFoot && SinceCut > S47GearLocal::SpellGap)
			{
				++FlickerCount;
				const bool bReal = LampFlicker(KLamp) > 0 && LampFlicker(KLamp) >= LampOrdinary;
				if (M->Omen) M->Omen->Felt(ES47OmenChannel::Lamp, TEXT("the lamp cut out"), bReal, FVector::ZeroVector, false);
			}
			SinceCut = 0;
		}
	}
	// The walkie broke up: its own count of spells (FS47FieldRadio::Dropouts), the real and the ordinary alike.
	if (M->FieldRadio)
	{
		const int32 Drops = M->FieldRadio->Dropouts();
		if (RadioDropsSeen < 0 || Drops < RadioDropsSeen) RadioDropsSeen = Drops;
		if (Drops > RadioDropsSeen)
		{
			DriftCount += Drops - RadioDropsSeen;
			RadioDropsSeen = Drops;
			const bool bReal = M->FieldRadio->GearNoiseNow() >= M->FieldRadio->OrdinaryNoiseNow() && (M->FieldRadio->GearNoiseNow() > 0.05 || M->FieldRadio->PresenceLevel() > 0.04);
			if (M->Omen) M->Omen->Felt(ES47OmenChannel::Radio, TEXT("the walkie went to hiss"), bReal, FVector::ZeroVector, false);
		}
	}
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
	const FS47Omen& Om = *M->Omen;
	KLamp = Ease(KLamp, Om.LevelFor(ES47OmenChannel::Lamp), Dt);
	KRadio = Ease(KRadio, Om.LevelFor(ES47OmenChannel::Radio), Dt);
	KTruck = Ease(KTruck, Om.LevelFor(ES47OmenChannel::Truck), Dt);
	KClock = Ease(KClock, Om.LevelFor(ES47OmenChannel::Clock), Dt);
	K = FMath::Max(FMath::Max(KLamp, KRadio), FMath::Max(KTruck, KClock));
	if (M->Player)
	{
		const FVector Stand(M->Player->Pos.X, 0, M->Player->Pos.Z);
		FeetSpeed = LastFeet.IsZero() ? 0 : FMath::Lerp(FeetSpeed, FVector::Distance(LastFeet, Stand) / Dt, FMath::Min(1.0, Dt * 4));
		if (!FMath::IsFinite(FeetSpeed) || FeetSpeed > 40) FeetSpeed = 0;
		LastFeet = Stand;
	}
	Apply(Dt);
	Count(Dt);
	bTouched = true;
	const int32 NewBand = Band(K);
	if (NewBand != LastBand)
	{
		if (bLog)
		{
			UE_LOG(LogS47Gear, Display, TEXT("S47: gear %s (%.2f): lamp %.2f radio %.2f truck %.2f clock %.2f; coughs %d, lamp out %d, radio lost %d, jumps %d"),
				S47GearLocal::BandNames[NewBand], K, KLamp, KRadio, KTruck, KClock, CoughCount, FlickerCount, DriftCount, Jumps);
		}
		LastBand = NewBand;
	}
}

bool FS47GearFlicker::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s gear: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bCond; };
	Check(LampSag(0) == 0 && LampFlicker(0) == 0 && RadioBreak(0) == 0 && TruckDim(0) == 1 && TruckFlickerEvery(0) == 0 && CoughEvery(0) == 0
		&& DriftEvery(0) == 0 && ClockJumpEvery(0) == 0 && WordLoss(0) == 0 && Band(0) == 0, TEXT("with nothing real out there the gear answers to nothing real"));
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
	Check(Band(0.1) == 1 && Band(0.4) == 2 && Band(0.7) == 3 && Band(0.9) == 4, TEXT("four bands"));
	Check(ShiftClock(TEXT("23:50"), 15) == TEXT("00:05") && ShiftClock(TEXT("00:03"), -5) == TEXT("23:58") && ShiftClock(TEXT("02:14:30"), 7) == TEXT("02:21:30"), TEXT("a jumped clock wraps the day and keeps its seconds"));
	Check(ShiftClock(TEXT("LATE AFTERNOON"), 5) == TEXT("LATE AFTERNOON") && ShiftClock(FString(), 5).IsEmpty(), TEXT("what is not a clock is left alone"));
	int32 Backward = 0, Big = 0; bool bMoves = true;
	for (int32 I = 0; I < 100; ++I) { const int32 J = JumpMinutes(1.0, I / 100.f); Backward += J < 0 ? 1 : 0; Big = FMath::Max(Big, FMath::Abs(J)); bMoves &= J != 0; }
	Check(bMoves && Backward > 5 && Backward < 40 && Big <= 40 && FMath::Abs(JumpMinutes(0.6, 0.99f)) < Big, TEXT("a jump always moves: mostly forward, now and then back; bigger nearer"));
	// the ordinary causes: the same signs, from things that are nothing
	Check(ContactFlicker(0, 3) == 0 && ContactFlicker(1, 0) > 0 && ContactFlicker(1, 0) < ContactFlicker(1, 2) && ContactFlicker(1, 2) < ContactFlicker(1, 5)
		&& ContactFlicker(0.3, 3) < ContactFlicker(1, 3), TEXT("a loose contact blinks the lamp more the worse it is and the more it is shaken"));
	Check(ContactFlicker(1, 6) > LampFlicker(0.2) && ContactFlicker(1, 6) < LampFlicker(0.9), TEXT("at a run a bad contact looks like something not far off, never like something on top of you"));
	Check(PathNoise(100, 0) == 0 && PathNoise(3000, 0) > PathNoise(1500, 0) && PathNoise(8000, 0) <= 0.32 && PathNoise(800, 20) > PathNoise(800, 0) && PathNoise(800, 20) < RadioBreak(0.5),
		TEXT("the walkie crackles of itself far from the mast and in low ground, never as much as something close"));
	int32 Bad = 0; bool bKept = true;
	for (int32 OnDay = 1; OnDay <= 200; ++OnDay) { Bad += BadTank(470047, OnDay) ? 1 : 0; bKept &= BadTank(470047, OnDay) == BadTank(470047, OnDay); }
	Check(bKept && Bad > 30 && Bad < 75, TEXT("about one day in four the tank is bad, the same day every time"));
	FS47GearFlicker Probe(nullptr);
	Check(Probe.ClockSays(TEXT("23:41")) == TEXT("23:41") && Probe.Broken(TEXT("DALE: where are you, answer me"), 3) == TEXT("DALE: where are you, answer me"), TEXT("no level, no jump and no lost words"));
	Probe.KRadio = 1;
	const FString Said = TEXT("Get back inside the building and close the doors, do you hear me");
	const FString First = Probe.Broken(Said, 5), Second = Probe.Broken(Said, 5);
	Check(First == Second && First != Said && First.StartsWith(TEXT("Get")) && First.Contains(TEXT("...")), TEXT("close by, words drop out of the walkie, the same way each time for the same line"));
	Probe.Step(1); Probe.Reset();
	Check(Probe.Level() == 0 && Probe.ClockJumps() == 0 && Probe.Flickers() == 0 && Probe.RadioDrifts() == 0, TEXT("without a game it does nothing"));
	return bOk;
}
