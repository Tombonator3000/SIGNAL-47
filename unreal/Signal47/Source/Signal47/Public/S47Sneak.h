#pragma once

#include "CoreMinimal.h"

// SNEAK-20261108 (Tom, 8 November): keep low. A key crouches the player (AS47Walker): slower, quieter steps and an eye some
// 60 cm lower. What looks and listens for the player takes a little less from that: the small ones see and hear a crouching
// player from less far (FS47RoswellBrain), a visit loses interest a little faster and comes on a little slower (FS47FarmSite),
// a thing at a shut door may give up on a room where nobody moves (FS47DoorVisit), and the light under a roof finds a chest
// that is under the sill a little later (FS47Taken). A small advantage, never invisibility: close by, in a lamp's beam or out
// in the open, it all still works as before. Lowness: 0 standing, 1 fully down; every number here is in one place.
//
// N1110 (HORROR-IDEAS-20261106-10 point 6): under a table. AS47Walker::Sneak() runs on past 1, to 2 for a player who has crept in under
// the control-room desk or the archive table (S47Hide.h): 1 to 2 is the share of the way from crouching to hidden. Hidden, what looks
// for the player sees them from a few metres (HiddenSight), hears the feet from about one (HiddenSteps), its interest grows slowly
// (HiddenHunt, a lamp still shows where they are) and the light looks for a chest nearly on the floor (HiddenChest). Never invisible:
// close by, it all still works. The approach and the light's pace do not change with it.
namespace S47Sneak
{
	constexpr double EyeDrop = 0.62;     // metres off the eye at 1.62
	constexpr double Pace = 0.5;         // times the walking speed
	constexpr double StepGain = 0.35;    // times a footstep's loudness
	constexpr double StepEvery = 0.6;    // metres between footsteps (0.8 standing)
	constexpr double SightLoss = 0.4;    // the small ones' 24 m of sight, less this share
	constexpr double HearingLoss = 0.6;  // their 9 m of hearing a walk, less this share
	constexpr double HuntLoss = 0.5;     // a visit's growing interest, less this share, while the lamp is off
	constexpr double ApproachLoss = 0.25; // a visitor's approach, less this share, while the lamp is off
	constexpr double ChestDrop = 0.5;    // the chest the light looks for, this much lower
	constexpr double TrackLoss = 0.2;    // the light's search pace, less this share
	constexpr double DoorGiveUp = 0.5;   // chance that the thing at a door goes, if nobody in the room has moved
	constexpr double DoorStillSeconds = 4.0; // ... for this long, low and dark, while it waits
	constexpr double HiddenSight = 3.0;  // under a table: seen from this far (m), in the lamp's light or not
	constexpr double HiddenSteps = 1.2;  // ... feet heard from this far (m)
	constexpr double HiddenHuntPace = 0.15; // ... a visit's interest grows at this share of the standing pace, lamp off
	constexpr double HiddenChest = 0.3;  // ... the chest the light looks for, this high (m)
	constexpr double HideEyeDrop = 0.45; // ... the eye this much lower still than crouching

	inline double QuietOf(double Lowness) { return FMath::IsFinite(Lowness) ? FMath::Clamp(Lowness, 0.0, 1.0) : 0.0; }
	/** 0 up to crouching, then 0 to 1 over the way in under a table (lowness 1 to 2). */
	inline double HiddenOf(double Lowness) { return FMath::IsFinite(Lowness) ? FMath::Clamp(Lowness - 1.0, 0.0, 1.0) : 0.0; }
	/** How far the small ones see the player: 24 m standing. */
	inline double SightReach(double Lowness) { return FMath::Lerp(24.0 * (1.0 - SightLoss * QuietOf(Lowness)), HiddenSight, HiddenOf(Lowness)); }
	/** How far they hear the player's feet at FeetSpeed m/s: 9 m walking, 18 running, nothing standing still. */
	inline double StepReach(double FeetSpeed, double Lowness)
	{
		if (!FMath::IsFinite(FeetSpeed) || FeetSpeed <= 0.35 || FeetSpeed >= 12.0) return 0.0;
		return FMath::Lerp((FeetSpeed > 2.5 ? 18.0 : 9.0) * (1.0 - HearingLoss * QuietOf(Lowness)), FMath::Min(HiddenSteps, (FeetSpeed > 2.5 ? 18.0 : 9.0) * (1.0 - HearingLoss)), HiddenOf(Lowness));
	}
	/** How fast a visit's interest grows, against 1 standing. A lamp on shows where the player is, low or not. */
	inline double HuntPace(double Lowness, bool bLampLitNow) { return bLampLitNow ? 1.0 : FMath::Lerp(1.0 - HuntLoss * QuietOf(Lowness), HiddenHuntPace, HiddenOf(Lowness)); }
	inline double ApproachPace(double Lowness, bool bLampLitNow) { return bLampLitNow ? 1.0 : 1.0 - ApproachLoss * QuietOf(Lowness); }
	inline double ChestHeight(double Lowness) { return FMath::Lerp(1.25 - ChestDrop * QuietOf(Lowness), HiddenChest, HiddenOf(Lowness)); }
	inline double TrackPace(double Lowness) { return 1.0 - TrackLoss * QuietOf(Lowness); }
	/** The thing at a shut door goes away unopened if the room has stayed low, dark and still long enough. Roll: 0 to 1. */
	inline bool DoorGoesAway(double StillTime, double DiceRoll) { return StillTime >= DoorStillSeconds && DiceRoll < DoorGiveUp; }
	/** Low, dark and still enough this frame to count towards it. */
	inline bool StillInRoom(double Lowness, bool bLampLitNow, double FeetSpeed) { return QuietOf(Lowness) > 0.6 && !bLampLitNow && FMath::IsFinite(FeetSpeed) && FeetSpeed < 0.4; }

	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
