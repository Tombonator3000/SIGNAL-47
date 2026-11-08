#pragma once

#include "CoreMinimal.h"

// The two small gauges in the HUD's bottom right corner (HUD-20261108), as plain functions so they can be tested without a
// screen: the flashlight's battery as five strokes, like the mark on an old battery (no numbers), and the radio line (channel
// or frequency and a small wave for the signal). SS47Hud reads the game and paints; the choices are made here.
namespace S47HudGauges
{
	constexpr int32 BatteryStrokes = 5;

	/** How many of the five strokes are lit: 0 only when empty, 1 for anything left, 5 for a full battery. */
	SIGNAL47_API int32 Strokes(double Level);
	/** The mark shows only while the lamp is on or the battery is low, and only where the lamp has a battery (Freeform). */
	SIGNAL47_API bool BatteryShown(bool bDrains, bool bHave, bool bOn, double Level, double Low);

	/** What the radio line says. The CB under the truck's dash: channel 19, 27.185 MHz. The carried walkie: channel 1, 49.830 MHz
	 *  (the game's own channel; no real station uses it). */
	SIGNAL47_API FString CbLine();
	SIGNAL47_API FString WalkieLine();
	/** The CB's signal while a call is on (steady, a little unsteady). T in seconds. */
	SIGNAL47_API double CbStrength(double T);

	/** One point of the wave, -1 to 1: X from 0 to 1 along the line, Phase moving, Strength 0 to 1 sets the swing. A weak signal is
	 *  small and ragged, a strong one is a clean wave. Zero at both ends. */
	SIGNAL47_API double Wave(double X, double Phase, double Strength);

	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
