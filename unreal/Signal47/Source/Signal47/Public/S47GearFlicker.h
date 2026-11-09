#pragma once

#include "CoreMinimal.h"

class US47Main;

// GEAR-FLICKER-20261108 (Tom, 8 November): the equipment fails near what is out there, an indicator with no HUD. Each piece follows
// what FS47Omen says that piece feels (LevelFor: how dangerous, by how near; now and then a piece misses it), roughly:
//
//   faint     the lamp's beam sags a little; the walkie and the truck's radio hiss now and then
//   nearer    the lamp blinks; the walkie crackles all the time, words of Dale's drop out; the truck's lights waver
//   close     the lamp goes out for moments; the walkie breaks up; the engine coughs; the watch and the dash clock jump
//   on you    the lamp is near enough dark; the clocks show nothing sensible
//
// The same gear also fails for ordinary reasons, as it does (TODO FLASHLIGHT-BATTERY-04: a flicker alone is no sure sign): a loose
// contact in the lamp on rough ground and at a run, a weak cell (FS47Flashlight), the walkie's hiss with distance from the mast,
// low ground and the wind, a far car on the channel, a bad tank that makes the engine miss under load, a headlight that stutters on
// a washboard road. A real source is stronger and touches several pieces at once; an ordinary one mostly one. What the player could
// notice is told to FS47Omen::Felt (the notebook's follow-up) and counted for the night's log, by what it was: the lamp on foot, the
// walkie and the truck's radio, the engine, the watch. Freeform only; Story is untouched. None of it costs battery.
class SIGNAL47_API FS47GearFlicker
{
public:
	explicit FS47GearFlicker(US47Main* Main) : M(Main) {}

	/** Every frame after FS47Omen, before the flashlight's own update. */
	void Step(double Dt);
	/** Everything the gear has touched is given back. */
	void Reset();

	/** The level the gear follows now, smoothed (0 to 1): the strongest of what its pieces feel. */
	double Level() const { return K; }
	/** What a clock shows: the real time, or for a moment a jumped one (the watch on the HUD, the truck's dash). */
	FString ClockSays(const FString& Real) const;
	/** A line from the walkie with words lost to the noise, as much as the walkie hears (none below "nearer"). */
	FString Broken(const FString& Line, int32 Seed) const;
	/** For the night's log: what the player could have noticed, each counted once a spell. */
	int32 ClockJumps() const { return Jumps; }
	int32 Coughs() const { return CoughCount; }
	/** The flashlight cut out in the player's hand, on foot, whatever the cause. */
	int32 Flickers() const { return FlickerCount; }
	/** The walkie broke up or the truck's radio lost its station. */
	int32 RadioDrifts() const { return DriftCount; }
	/** The truck's lights stuttered while it was driven. */
	int32 TruckFlickers() const { return TruckFlickerCount; }

	// ---------- the pure rules (tested): each rises with the level and is nothing below its own threshold ----------
	/** The share of the beam lost (0 to 0.55). */
	static double LampSag(double Level);
	/** The flashlight's flicker strength (FS47Flashlight::Flicker), 0 below the faint band. */
	static double LampFlicker(double Level);
	/** What the walkie and the AM sets hear of it, 0 to 1. */
	static double RadioBreak(double Level);
	/** Everything electric in the truck, 1 down to 0.35. */
	static double TruckDim(double Level);
	/** Seconds between: the truck's lights stutter, the engine coughs, the car radio drifts, a clock jumps. 0: never at this level. */
	static double TruckFlickerEvery(double Level);
	static double CoughEvery(double Level);
	static double DriftEvery(double Level);
	static double ClockJumpEvery(double Level);
	/** How many minutes a clock jumps at this level (a few, up to forty). */
	static int32 JumpMinutes(double Level, float Roll);
	/** The share of a walkie line's words lost. */
	static double WordLoss(double Level);
	/** "HH:MM" moved by some minutes, the day wrapped; anything else back as it was. */
	static FString ShiftClock(const FString& Real, int32 Minutes);

	// ---------- the ordinary causes (tested) ----------
	/** A loose contact: how much the lamp flickers for a contact this bad, at this pace on foot (m/s). Walking shakes it. */
	static double ContactFlicker(double Strength, double Pace);
	/** The walkie's own crackle with distance from SARO's mast (metres) and how far below the mast's ground it is (metres). */
	static double PathNoise(double MetresFromMast, double BelowMast);
	/** A tank of bad gas on this day: the engine misses now and then under load, all day. About one day in four. */
	static bool BadTank(int32 Seed, int32 OnDay);
	static bool SelfTest(TArray<FString>& Lines);

	/** The bands the comment above names, for the log and F10: 0 nothing, 1 faint, 2 nearer, 3 close, 4 on you. */
	static int32 Band(double Level);

private:
	US47Main* M = nullptr;
	// smoothed, for each piece: what it feels of something real
	double K = 0, KLamp = 0, KRadio = 0, KTruck = 0, KClock = 0;
	double TruckFlickerIn = 0, CoughIn = 0, DriftIn = 0, ClockIn = 0, ClockLeft = 0, TankCoughIn = 40, RoughIn = 150, OrdinaryDriftIn = 0;
	int32 ClockShift = 0, Jumps = 0, CoughCount = 0, FlickerCount = 0, DriftCount = 0, TruckFlickerCount = 0, LastBand = 0;
	// the lamp's cut-outs and the walkie's drop-outs, as their own counters had them, and how long since the last of each
	int32 LampCutsSeen = -1, RadioDropsSeen = -1;
	double SinceCut = 1e9, FeetSpeed = 0, LampOrdinary = 0;
	FVector LastFeet = FVector::ZeroVector;
	bool bScramble = false, bTouched = false, bLog = false, bLogChecked = false;
	void Apply(double Dt);
	void Release();
	void Count(double Dt);
	double Ease(double Cur, double Want, double Dt) const;
};
