#pragma once

#include "CoreMinimal.h"

class US47Main;

// STRESS-20261108: the player character's strain, 0 to 1, with no number on the screen. It rises near danger (a dangerous event
// going on, a figure close by, the dark outside without a lamp, being taken, and a while after) and falls slowly where it is safe
// (indoors at SARO). Three things show it, and none of them while it is low: a darker edge (SS47Hud paints it), a view a little
// narrower (AS47Walker::FovScale) and heavier breathing (the walkie's breath, S47Walkie::Breath, through the sfx bus; OnBreath
// is the hook for a recorded breath later). Held by US47Main::Stress, stepped each frame after the flashlight.
struct FS47StressIn
{
	/** A dangerous event is going on near the player (the director's "not now", the farm's visit, the low pass overhead...). */
	bool bEvent = false;
	/** How near a real presence is, 0 to 1, and whether it is in view / hunting. */
	double Presence = 0;
	bool bSeen = false, bHunting = false;
	/** Outdoors in the dark (Darkness 0 at the edge of dusk to 1 in full night) with no lamp lit. */
	bool bDark = false;
	double Darkness = 1;
	/** Being taken now; and seconds since the last time (a large number: not lately). */
	bool bTaken = false;
	double SinceTaken = 1e9;
	/** Indoors at SARO: the stress falls faster here. */
	bool bSafe = false;
	/** The night has gone quiet round the player (FS47Soundscape: something is coming): a small unease of its own. */
	bool bHush = false;
};

class SIGNAL47_API FS47Stress
{
public:
	explicit FS47Stress(US47Main* Main) : M(Main) {}

	/** Every frame (game time: not while a menu or a paper holds the game). */
	void Step(double Dt);
	/** A new night, the title, a save opened: calm again. */
	void Reset();
	/** A start, at once (a covey going up from under the feet in the dark, FS47Animals): at least this much, falling as usual. */
	void Startle(double Jolt) { S = FMath::Max(S, FMath::Clamp(Jolt, 0.0, 1.0)); }

	/** 0 to 1. */
	double Level() const { return S; }
	/** The edge's darkness (alpha at the very edge, 0 when calm) and how far in it reaches (share of the screen, 0 when calm). */
	double Vignette() const { return VignetteAlpha(S); }
	double VignetteReach() const { return VignetteReachOf(S); }
	/** Called as each breath is drawn: the stress level and the breath's strength. Sound can be attached here later. */
	TFunction<void(double, double)> OnBreath;
	int32 Breaths() const { return BreathCount; }

	// ---------- the pure rules (tested) ----------
	/** Where the stress is heading with these inputs. The terms combine as chances do, so two worries are more than one, never over 1. */
	static double Target(const FS47StressIn& In);
	/** One step of Dt seconds from Cur toward Target: up quickly, down slowly (faster in a safe place). */
	static double Advance(double Cur, double Target, double Dt, bool bSafe);
	static double VignetteAlpha(double Strain);
	static double VignetteReachOf(double Strain);
	static double FovScaleOf(double Strain);
	/** Seconds between breaths (0: none at this level) and the breath's strength. */
	static double BreathGap(double Strain);
	static double BreathStrength(double Strain);

	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	double S = 0, ReadIn = 0, BreathIn = 2, SinceTaken = 1e9;
	bool bWasTaken = false, bLog = false, bLogChecked = false;
	int32 BreathCount = 0;
	FS47StressIn Inputs;
	FS47StressIn Read();
	void Breathe(double Dt);
};
