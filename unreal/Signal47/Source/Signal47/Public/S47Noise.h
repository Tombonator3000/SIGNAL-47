#pragma once

#include "CoreMinimal.h"

// PHYSICS-20261108: sounds in the world that something out there can hear, other than the player's own feet, breath and walkie
// (those the listeners read where they are made). A thrown stone, a can that falls, a crate set down hard: each is posted here
// where it happened, and the listeners read the loudest fresh one: the small ones (FS47RoswellGroup, through FS47RoswellBrain's
// ears: it goes to look where it heard it, which is how a stone draws one away), and a visit at a place (FS47FarmSite, through
// FS47VisitAttention: it stops and turns to the sound). Loudness is as FS47RoswellBrain::FNoise has it: heard out to ten metres
// times this, never more than twice that. Three-space metres. Nothing here is saved; a noise lasts a moment.
namespace S47Noise
{
	struct FNoise
	{
		FVector At = FVector::ZeroVector;
		double Loudness = 0;
		double Time = 0;   // FPlatformTime::Seconds when it was made
		uint32 Serial = 0; // counted up for each one, so a reader can take each once
		FString What;      // "stone on dirt", "crate on concrete": for the log and F10
	};
	/** A sound at At, as loud as this (0 to 2). Quieter than 0.05 is nothing anyone hears. */
	SIGNAL47_API void Make(const FVector& At, double Loudness, const FString& What);
	/** The loudest one made in the last Within seconds, if any. */
	SIGNAL47_API TOptional<FNoise> Latest(double Within = 0.5);
	/** The one with the highest serial (the last made), fresh or not. */
	SIGNAL47_API TOptional<FNoise> Last();
	SIGNAL47_API int32 Count();
	SIGNAL47_API void Clear();
	/** How far one is heard on a still night, metres (FS47RoswellBrain::Heard: ten times the loudness, at most twenty). */
	inline double Reach(double Loudness) { return FMath::Clamp(Loudness, 0.0, 2.0) * 10.0; }
}
