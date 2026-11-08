#pragma once

#include "CoreMinimal.h"

class US47Main;

// M2 (Tom, 8 November: GEAR-FLICKER, SOUNDSCAPE, ANIMALS): what is out there near the player, read from the public state of the
// systems that really put it there (the Director's event, being taken, a visit at marks or out in the open, a door tried, a face at
// a window, the bodies out, the light over the highway, the night's own events) and nothing else. Nothing here is drawn or rolled:
// with no event on the ground there is no source, and so no flicker, no silence and no animal that runs.
//
// Each source has a place (three-space, in the area the player is in), how dangerous it is (0 to 1) and how far it carries. Three
// things read it, each in its own way, so that an attentive player learns to read them together:
//   FS47GearFlicker  the lamp, the walkie, the truck and the watch, by distance and danger (Level).
//   FS47Soundscape   the night's sounds stop seconds before it comes (Sense, onsets), and come back one at a time after.
//   FS47Animals      birds go up, a coyote stops and looks at it, the cattle walk off together, the dog stops (Sense, From).
// Freeform only. Gathered four times a second while the game runs; a menu or a paper holds it as it is.
struct FS47OmenSource
{
	FString Id;                      // "director:service_presence", "taken", "visit", "door", "peek", "body:3", "highway", "night:landing", "dev"
	FVector At = FVector::ZeroVector; // three-space, in the area the player is in now
	double Severity = 0;             // how dangerous, 0 to 1
	double Reach = 0;                // metres: nothing of it at or beyond this
	bool bComing = false;            // its first sign only: the body or the light is not here yet
	bool bResidual = false;          // what one left (marks, a ring): a faint touch on the gear, no silence, no animal
};

class SIGNAL47_API FS47Omen
{
public:
	explicit FS47Omen(US47Main* Main) : M(Main) {}

	/** Every frame after the Freeform session (game time). */
	void Step(double Dt);
	/** Out of Freeform, the title, a save opened: nothing is near. */
	void Reset();

	const TArray<FS47OmenSource>& Sources() const { return Now; }
	/** At the player's eyes, 0 to 1: the strongest of severity by distance. A source only coming counts for less. */
	double Level() const { return LevelNow; }
	/** What a creature or the night at this place feels, 0 to 1, out to Widen times each source's reach (and never less than
	 *  MinSense metres): animals and the crickets know before the lamp does. OutFrom: where the strongest is; bOutComing: it is
	 *  only coming. Residual sources are left out. */
	double SenseAt(const FVector& Where, double Widen = SenseWiden, FVector* OutFrom = nullptr, bool* bOutComing = nullptr) const;
	static constexpr double SenseWiden = 1.6, MinSense = 80;
	/** Seconds since something new came within its sense of the player; a large number when nothing has. */
	double SinceOnset() const { return OnsetAge; }
	int32 Onsets() const { return OnsetCount; }
	/** Where the last onset was, three-space. */
	FVector OnsetFrom() const { return OnsetAt; }

	/** F10: a made-up presence at this place for this long, as dangerous as Severity. bComing: its first six seconds are the
	 *  sign before it (the night goes quiet, the birds go up), then it is there. */
	void DevPresence(const FVector& Where, double Severity, double Seconds, bool bFirst);
	bool DevActive() const { return DevLeft > 0; }
	void DevClear() { DevLeft = 0; }

	// ---------- the pure rules (tested) ----------
	/** Full within a few metres, nothing at Reach, smooth between: a broad feeling, not a rangefinder. */
	static double Falloff(double Distance, double Span);
	/** How far a source this dangerous carries, for one that does not say (F10's): 25 m for a small thing, 240 m for the worst. */
	static double ReachFor(double Severity);
	/** The level at an eye from a list of sources. */
	static double LevelAt(const FVector& Eye, const TArray<FS47OmenSource>& In);
	static double SenseFrom(const FVector& Where, const TArray<FS47OmenSource>& In, double Widen, FVector* OutFrom, bool* bOutComing);
	/** A coming source counts this much of its severity at the eyes (the walkie hisses before the body is there). */
	static constexpr double ComingShare = 0.6;
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	TArray<FS47OmenSource> Now;
	TSet<FString> Sensed;
	double ReadIn = 0, LevelNow = 0, OnsetAge = 1e9;
	int32 OnsetCount = 0;
	FVector OnsetAt = FVector::ZeroVector;
	FVector DevAt = FVector::ZeroVector;
	double DevLeft = 0, DevSeverity = 0, DevAge = 0;
	bool bDevComing = false, bLog = false, bLogChecked = false;
	void Gather(TArray<FS47OmenSource>& Out) const;
};
