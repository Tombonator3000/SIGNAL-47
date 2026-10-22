#pragma once

#include "CoreMinimal.h"
#include "S47Seen.h"

class US47Audio;

// web/src/core/calls.ts. Out in the dark (Tom, 16 October: alien sounds now and then when the
// player walks about with the flashlight). The light shows where you are. On foot, outside, at
// night, with the flashlight on, something sounds out in the dark now and then: first the crickets
// and the wind go quiet, the flashlight stutters, and then a call, far off (35 to 110 m) and never
// in front where the beam goes, but behind or off to one side. Nothing comes closer and nothing
// answers (R5, HENDELSER.md). The more the light is used, the sooner the next one comes. They are
// one of the signs of the night's curve (story/curve.ts): in its build and its peak, one or two in
// a cycle, and never in the quiet after a peak.
//
// The sounds are Freesound originals from the credits of They Are Here, CC0 and CC BY
// (THIRD_PARTY_NOTICES.md): a voice with clicks in it, something on a radio, calls of animals that
// are not from here and a singing, a rush of air, and now and then a long dark horn across the
// desert. With the bought sounds there is one more: a single note that swells out of nowhere.

struct FS47CallsWhen
{
	/** On foot, outside, at night, the flashlight on, no scene holding the player. */
	bool bOk = false;
	FS47Cam Cam;
	/** The flashlight's stutter, 0..1 (Flashlight.flicker). */
	TFunction<void(double)> Flicker;
	/** The night goes quiet for a few seconds (Ambience.hush). */
	TFunction<void(double)> Hush;
	/** The night's curve lets one come now; unset, it may. */
	TOptional<bool> May;
	/** How much sooner the next comes than its own pace (the curve's build: 1 and up). */
	TOptional<double> Pace;
};

class SIGNAL47_API FS47NightCalls
{
public:
	explicit FS47NightCalls(US47Audio* InAudio);

	/** Seconds of walking with the light on before the next. */
	double Next = 0;
	/** How many so far (tests, the developer menu). */
	int32 Count = 0;
	/** The last one: which, where, how far and how far round from where the player looked (tests). */
	struct FLast { FString Name; FVector At = FVector::ZeroVector; double Dist = 0, Off = 0; };
	TOptional<FLast> Last;

	/** One now (the developer menu, tests): the hush, then the call in a moment. */
	void Now() { Next = 0; bForced = true; }
	void Update(double Dt, const FS47CallsWhen& W);

	static bool SelfTest(TArray<FString>& Lines);

private:
	TWeakObjectPtr<US47Audio> Audio;
	double Lead = 0;             // seconds from the hush to the call
	double Stutter = 0;          // the flashlight's stutter, fading after the call
	FString Before;
	/** Asked for now (the developer menu, tests): the curve is not asked. */
	bool bForced = false;

	void Call(const FS47Cam& Cam);
};
