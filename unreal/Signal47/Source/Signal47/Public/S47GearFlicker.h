#pragma once

#include "CoreMinimal.h"

class US47Main;

// GEAR-FLICKER-20261108 (Tom, 8 November): the equipment fails near what is out there, an indicator with no HUD. It follows one
// level only (FS47Omen::Level: how dangerous, by how near), the same way every time, so that a player can learn to read it:
//
//   faint     the lamp's beam sags a little; the walkie and the truck's radio hiss now and then
//   nearer    the lamp blinks; the walkie crackles all the time, words of Dale's drop out; the truck's lights waver
//   close     the lamp goes out for moments; the walkie breaks up; the engine coughs; the watch and the dash clock jump
//   on you    the lamp is near enough dark; the clocks show nothing sensible
//
// Nothing here happens without a source: no source, no flicker. It costs no battery (FLASHLIGHT-BATTERY-04: weakness from the
// cell is slow and steady, this comes and goes with distance and comes with the radio's noise). Freeform only; Story is untouched.
class SIGNAL47_API FS47GearFlicker
{
public:
	explicit FS47GearFlicker(US47Main* Main) : M(Main) {}

	/** Every frame after FS47Omen, before the flashlight's own update. */
	void Step(double Dt);
	/** Everything the gear has touched is given back. */
	void Reset();

	/** The level the gear follows now, smoothed (0 to 1). */
	double Level() const { return K; }
	/** What a clock shows: the real time, or for a moment a jumped one (the watch on the HUD, the truck's dash). */
	FString ClockSays(const FString& Real) const;
	/** A line from the walkie with words lost to the noise, as much as the level says (none below "nearer"). */
	FString Broken(const FString& Line, int32 Seed) const;
	int32 ClockJumps() const { return Jumps; }
	int32 Coughs() const { return CoughCount; }
	int32 Flickers() const { return FlickerCount; }
	int32 RadioDrifts() const { return DriftCount; }

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
	static bool SelfTest(TArray<FString>& Lines);

	/** The bands the comment above names, for the log and F10: 0 nothing, 1 faint, 2 nearer, 3 close, 4 on you. */
	static int32 Band(double Level);

private:
	US47Main* M = nullptr;
	double K = 0;
	double TruckFlickerIn = 0, CoughIn = 0, DriftIn = 0, ClockIn = 0, ClockLeft = 0;
	int32 ClockShift = 0, Jumps = 0, CoughCount = 0, FlickerCount = 0, DriftCount = 0, LastBand = 0;
	bool bScramble = false, bTouched = false, bLog = false, bLogChecked = false;
	void Apply(double Dt);
	void Release();
};
