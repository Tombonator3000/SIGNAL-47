#pragma once
#include "CoreMinimal.h"
#include "S47Audio.h"
class US47Main;
class AS47Web;

// N1110 (Caliche history, approved by Tom 10 November: Docs/Lore/CALICHE_HISTORIE.md, "the Breather"). Freeform only. The cave
// under Caliche, found by a well driller in 1923 and shown to tourists by the Trujillo family since 1931, has its tour entrance at
// the west edge of town, behind Main Street: an outcrop of rock with a dark opening in it, a concrete landing and a pipe rail, a
// chain-link fence with a padlocked gate, a stucco ticket hut and a sign, THE BREATHER / CAVE TOURS / CLOSED FOR THE SEASON. The
// pieces stand in S47TownSites' table (the "breather" row: a landmark, not a temporary box and not a place for events); this is
// what they do:
//
//   the draft    a steady cold draft out of the mouth, slow in and out (a made loop of low noise, about twelve seconds a breath,
//                through the audio graph like any placed sound): louder near the mouth, stronger at night, gone past 40 m.
//   the words    the sign, the plaque by the gate, the hut's window card (the leaflet at the gas station tells the rest).
//   the record   looking at the gate or the plaque the first time keeps a field record, "cave_breather".
//   the night    after 02:00 on a full-tier night, rarely (the script's odds, and never twice in one week), standing at the gate:
//                the breath stops for a few seconds and the night's own sound with it, then both come back. Nothing else. The
//                beats are Content/Director/Events/breather_hush.json; how long the draft holds is its silence beat's secs.
namespace S47Breather
{
	// Road-local metres (x east, z south): the dark of the mouth, and where the player stands at the gate.
	FVector2D Mouth();
	FVector2D Gate();
	constexpr double GateReach = 2.4;      // metres from Gate() that count as standing at it
	constexpr double Breath = 12.0;        // seconds a breath, in and out
	constexpr double LoopSecs = 24.0;      // the loop: two breaths, a little different from each other
	constexpr double Gone = 40.0;          // metres from the mouth past which nothing is heard
	const TCHAR* const HushScript = TEXT("breather_hush");
	const TCHAR* const Finding = TEXT("cave_breather");

	// The draft's loop at a sample rate: mono, LoopSecs long, seamless end to start, peak about .6.
	SIGNAL47_API TArray<float> MakeDraft(float Hz);
	// How loud the draft is set, before the panner's own fall-off: by the distance from the mouth and the hour of the day (0 to 24).
	SIGNAL47_API double Level(double Metres, double Hour);
	// How much of the breath is left during a hush: 1 before it and after, 0 while it holds (Since: seconds since it began).
	SIGNAL47_API double Held(double Since, double Secs);
	// The week of a Freeform night (1, 2, ...): nights 1 to 7 are week 0.
	SIGNAL47_API int32 WeekOf(int32 Night);
	// The flag the town keeps (FS47TownUse::Seen, saved with it) for the week's one hush.
	SIGNAL47_API FString WeekFlag(int32 Week);
	// How long the script says the draft holds (its silence beat's secs), or 5 s.
	SIGNAL47_API double HushSecs();
	// Starts the loop in an audio (made into its buffers the first time); unset At: not placed. The low-pass is set so Level can open it.
	SIGNAL47_API FS47Play StartLoop(US47Audio* A, const FString& Key, TOptional<FVector> AtThree, float Gain);
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}

class FS47Breather
{
public:
	explicit FS47Breather(US47Main* Main) : M(Main) {}
	// The pieces are built (S47TownSites): Nodes, each use's id to the node a look picks.
	void Begin(AS47Web* W, const TMap<FString, int32>& Nodes);
	void Step(double Dt);
	void Stop();
	bool Hushing() const { return HushAt >= 0; }
private:
	US47Main* M = nullptr;
	// The uses outlive a session (the world keeps them): they hold this, and do nothing while bLive is false.
	TSharedPtr<uint8> Alive = MakeShared<uint8>(0);
	bool bLive = false, bSound = false;
	double AtGate = 0, HushAt = -1, HushSecsNow = 5;
	int32 AskedNight = 0;
	void Record();
	void StepSound(double Dt, double Metres, double Hour, bool bHere);
	void StepHush(double Dt, bool bAtGate, double Hour);
};
