#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"
#include "S47Seen.h"

class US47Audio;
class US47Inter;
class FS47AudioContext;
class FS47AudioNode;
class FS47ScheduledSourceNode;

// web/src/story/roofSteps.ts. Light steps on the records room roof (FORSLAG.md, 20 October, "Valg for
// Tom", the extra choice). Tom: "Spenning og skummelt slår faste regler". This one breaks two rules of
// HENDELSER.md on purpose, once in the night: they make no sound, not even steps; and nothing in the
// world acts because the player is there. Here something walks over the player's head, and it knows
// where.
//
//   the steps   chapter two, once, while the player is in the records room at a quiet moment
//               (nothing open on the table, no call, the tape in the control room not playing, a
//               paper read, standing still a moment): light steps, closer together than anyone
//               walks, come across the annex roof from over the corridor's east end and stop right
//               over the player (up onto the roof unit when the player is at the table, under it).
//               While they come they go where the player goes, and the room's hum sinks a little.
//   the beat    nothing. A thin trickle of dust from the ceiling just in front of the player's face;
//               the room's hum drops out and the tired tube stops flickering; the lights stay on.
//               10.5 s of it.
//   once more   six or seven, quicker, further off: from where it now stands to the lip of the roof
//               at a corner (the south-west, or the south-east when that is the nearer), and gone.
//               The hum comes back two and a half seconds later and the tired tube stutters.
//   after       frost on the roof unit over the records room, and a crust of it on the lip at that
//               corner: seen from outside, with a closer look (the zoom), and a note in the player's
//               words, and another the first time the frost is looked at.
//
// The steps are heard through the slab: the bought ones (sfx/roof_steps.mp3, eight hits of a knife
// going through into a board: a dull thump and a dry crunch) or, without the private folder,
// Kenney's gravel steps (public, CC0) played quicker; low-passed, a thump in the slab under each,
// placed on the roof with the panner, quieter and duller the further off. A step on the roof unit
// over the table (where a player at the table has it stop) is on its sheet-metal top, 0.8 m higher,
// and the metal gives a short dull ring. Outside or in the control room the slab's filter is off and
// the walls do it. Kept in the night's seen set (main.ts `townSeen`, saved as `town`): `roof:steps`
// when they come, `roof:sw` / `roof:se` for the corner (the frost), `roof:frost` once the frost has
// been looked at. Captioned in time with them: once while they come, when they stop, when they go.
// Under each that comes over the room a few motes of dust shake out of the ceiling.
// (The panner is S47WebAudio's equal-power one: 'HRTF' is asked for, as the browser is, and pans so.)

struct FS47RoofIn
{
	/** Chapter two at the records stage or ready to call the key holder, nothing else of the night's
	 *  going (main.ts): the steps may start. */
	bool bOk = false;
	/** No paper or board open, no cinematic, no call on the line: quiet enough to hear a roof. */
	bool bQuiet = false;
	/** A call or a cinematic: what is left of the steps waits for it. */
	bool bHold = false;
	/** In the records room. */
	bool bInRoom = false;
	/** Indoors (the annex or the control room): heard through the slab. */
	bool bMuffled = false;
	/** The listener's feet. */
	FVector Pos = FVector::ZeroVector;
	double Clock = 0;
	/** A record has been read (the player has settled into the room). */
	bool bRead = false;
	/** The key holder is next (P05 recorded): the room's last quiet moment. */
	bool bLastQuiet = false;
	/** Still chapter two (if the chapter ends while they are on, what is left of them goes). */
	bool bChapter = false;
};

struct FS47RoofDeps
{
	TWeakObjectPtr<US47Audio> Audio;
	FS47TownSeen* Seen = nullptr;
	TFunction<void(const FString&)> Note;
	TFunction<void(const FString&, float)> Toast;
	/** The frost on the roof unit and the lip (Annex.setRoofFrost): 'sw' | 'se', empty: none. */
	TFunction<void(const FString&)> Frost;
	/** Dust from the ceiling where it stops (Annex.sift, a little in front of the player's face). */
	TFunction<void(double, double)> Dust;
	/** A few motes shaken loose under one step that comes over the rooms (Annex.sift, small). */
	TFunction<void(double, double, int32)> Puff;
	/** The tubes in the records room (Annex.holdTubes): 'steady' | 'stutter', empty: null. */
	TFunction<void(const FString&)> Tubes;
	/** Where the tired tube hangs, for the tick when the hum comes back. */
	FVector TubeAt = FVector::ZeroVector;
	TWeakObjectPtr<US47Inter> Inter;
	/** The hit areas for a closer look (Annex.frost.hit). */
	struct FHit { FS47Obj Unit, Sw, Se; };
	FHit Hit;
	/** The player is outside in SARO's grounds (a closer look at the roof makes sense only there). */
	TFunction<bool()> Outside;
	/** The night's clock. */
	TFunction<double()> Clock;
};

class SIGNAL47_API FS47RoofSteps
{
public:
	explicit FS47RoofSteps(FS47RoofDeps InD);

	/** 'idle' | 'come' | 'still' | 'off' | 'after' | 'done' */
	FString Phase = TEXT("idle");
	/** Every step as it was played (tests): when (the event's own clock), where, how far from the
	 *  listener's feet across the roof, and which burst. */
	struct FLog { double T = 0, X = 0, Z = 0, D = 0; int32 Burst = 1; };
	TArray<FLog> Log;
	/** 'sw' | 'se', empty: null. */
	FString Corner;
	/** The bought steps were there (or Kenney's stood in). */
	bool bBought = false;

	/** Where they make for and stop: the roof right over the player (SARO's terms; story/seen.ts). */
	FVector Overhead() const;
	/** Something of the steps is going on: the beat included (the music and the night's other events wait). */
	bool Busy() const { return Phase != TEXT("idle") && Phase != TEXT("done"); }

	/** A new night, a save opened: the frost as the seen set has it, nothing going. */
	void Reset();
	void Update(double Dt, const FS47RoofIn& S);
	/** Now, whatever the waiting (the developer menu, the tests). False when it cannot be. */
	bool Now(const FS47RoofIn& S);

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47RoofDeps D;
	struct FStep { double At = 0, X = 0, Z = 0; int32 Slot = 0; double Gain = 1; int32 Burst = 1; };
	double T = 0;
	double Room = 0, Quiet = 0, Still = 0;
	FVector Last = FVector(1e9, 0, 0);
	TArray<FStep> Queue;
	struct FCue { double At = 0; TFunction<void()> Fn; };
	TArray<FCue> Cues;
	struct FWalk { FVector2D Pos = FVector2D::ZeroVector; int32 N = 0; double Next = 0; int32 Settle = 0; bool bHesitated = false, bPlanned = false; };
	FWalk Walk;
	/** The first caption has been shown (while they come). */
	bool bTold = false;
	FVector2D Target = FVector2D::ZeroVector;
	bool bMuffled = true;
	TArray<TSharedPtr<FS47ScheduledSourceNode>> Sources;
	double HumBack = 0;
	int32 SlotLast = -1;
	double ClockAt = 0;

	void Cancel();
	bool Due(const FS47RoofIn& S) const;
	void Start(const FS47RoofIn& S);
	static FVector2D RoofOver(const FVector& P);
	void Plan();
	int32 Slot();
	void Stopped(double At, const FVector2D& Where);
	void Cue(double At, TFunction<void()> Fn);
	void FrostSeen();
	void Play(const FStep& St, double When);
	void Ring(const TSharedRef<FS47AudioContext>& Ctx, const TSharedPtr<FS47AudioNode>& Out, double T0, double K);
	void Track(const TSharedPtr<FS47ScheduledSourceNode>& N);
};
