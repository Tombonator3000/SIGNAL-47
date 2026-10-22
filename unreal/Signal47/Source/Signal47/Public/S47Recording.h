#pragma once

#include "CoreMinimal.h"
#include "S47Seen.h"

class FS47JacketWalker;

// web/src/story/recording.ts. The recording (FORSLAG.md, 20 October, item 9 B; HENDELSER.md, section 8:
// "they take a recording of you, and every recording stays"). Where the player was taken, someone in a
// SARO jacket stands for the rest of the night (story/abduction.ts, record). The game keeps the player's
// last 47 seconds there, as they were lived: where they went, every quarter of a second, and which way
// they looked (AbductSave.spots[].path). Once, later, when the player is back within sight of the
// place and the night is building or at its peak, the one in the jacket walks them: from where the
// player stood when the crickets stopped, along the same way, stopping where they stopped, running
// the same stumbling run, turning to look where they looked, and standing still at the end for the
// three seconds the light took. Then it is not there, and it starts over where it began. R5 on the
// player's own body: it repeats what was registered and does not answer. Seen from a distance only
// (it is not there within 45 m, as the one standing is not), silent, and now and then the tape it
// is on drops a few frames and catches up.
//
// One at a time (main.ts, oneAtATime): it waits for anything else of the night's, and nothing
// starts while it walks. The night's curve (story/curve.ts) lets it come in a build from half way
// or at a peak, never in a rest; where one waits for the player within sight, the curve hurries
// for it the way it does for the motel's television. Written in the night's log (story/seen.ts)
// as 'walk 1' (the first time away's), and it lands the peak once it is looked at: looked at when it
// can be made out, its band lit by the flashlight or the headlights, or within 60 m (a dark shape on
// dark ground at 150 m has not been seen, however the eyes are turned). Far off, what carries is the
// tape: in a beam it gives the light back as a glint a few pixels across on any screen, which moves
// as the one wearing it moves; zoomed, it is someone in a SARO jacket. Kept with the case (the spot's
// `walked`): it never walks again, after Continue either; the one standing stays. Only a walk the
// player went away from before ever making it out (drove on, say) comes again, once, another time
// they are back within sight of the place.
//
// The one in the jacket is made at run time in the web version (story/abduction.ts records it where the
// player was taken, a recording.ts Walker), so it is built here the same way: recording.ts's Walker is
// FS47JacketWalker (S47Figures.h, with the other figures the web version builds in code), one port
// for both: the time away makes one for each time away (FS47Abduction::Record) and gives it to the
// walk in its recordings, and the walk poses it. (Walker.baked, the one-draw copy story/theThird.ts
// uses, is chapter six's: S47TheThird.cpp.)

namespace S47Recording
{
	/** The path is sampled this often (seconds), x, z and the yaw of the view each time. */
	inline constexpr double PATH_DT = 0.25;
	/** It walks no nearer than this (as the one standing: story/abduction.ts) and comes when the player
	 *  is between these from the place (metres). */
	inline constexpr double NEAR = 45, FROM_D = 55, TO_D = 180, GIVE_UP = 450;
	/** How long a path runs (seconds). */
	SIGNAL47_API double PathLen(const TArray<double>& P);
	/** Where on the path at t seconds (smoothed between the samples), and the yaw of the view then. */
	SIGNAL47_API double PathAt(const TArray<double>& P, double T, FVector& Out);
	/** How much ground the path covers (metres). */
	SIGNAL47_API double PathMoved(const TArray<double>& P);
}

/** One of the times away, as the walk sees it (story/abduction.ts, recording). */
struct FS47Rec
{
	int32 N = 0;
	FString Scene;
	FVector Take = FVector::ZeroVector;
	/** x, z and yaw every PATH_DT seconds; empty: none kept. */
	TArray<double> Path;
	TOptional<double> Walked;
	double Clock = 0;
	/** The one standing for this time away (recording.ts Walker: S47Figures.h), the time away's. */
	TSharedPtr<FS47JacketWalker> Fig;
	/** Kept with the case: it has walked (the night's clock then), or not after all (unset). */
	TFunction<void(TOptional<double>)> MarkWalked;
};

struct FS47RecordingDeps
{
	/** The recordings there are (one for each time away). */
	TFunction<TArray<TSharedPtr<FS47Rec>>()> Recs;
	/** The walk has the one standing for this time away (held) or gives it back (the one standing
	 *  comes back where it stands, once that place is out of the view). */
	TFunction<void(int32, bool)> Hold;
	TFunction<double(double, double)> Ground;
	TFunction<FS47Cam()> Camera;
	/** Write it down: the notebook, the case. */
	TFunction<void(const FString&)> Note;
	TFunction<FString(double)> ClockText;
	TFunction<void()> Save;
	/** How much a light from near the eye falls on a point (the band gives it back), 0 to 1. */
	TFunction<double(const FVector&)> Shine;
};

struct FS47RecordingIn
{
	/** Chapters three to five, playing, no panel, no cinematic. */
	bool bOk = false;
	/** The night's curve lets a scare come, and nothing else of the night's is going. */
	bool bMay = false;
	/** The area the player is in (rings and recordings belong to one), and where they are. */
	FString Scene;
	FVector Pos = FVector::ZeroVector;
	/** Out where it could be seen: on foot outdoors, or in the truck (not inside a building). */
	bool bOut = false;
	double Clock = 0;
};

class SIGNAL47_API FS47RecordingWalk
{
public:
	explicit FS47RecordingWalk(FS47RecordingDeps InD);

	/** 'idle' | 'walk' | 'stand' | 'gone' */
	FString Phase = TEXT("idle");
	/** Which time away is walking (1 to 3), or 0. */
	int32 N = 0;
	/** How it ended the last time: 'near' (walked up to it), 'left' (the player went away) or 'done'. */
	FString Ended;
	/** How many times it has started over, and the seconds it was looked at. */
	int32 Loops = 0;
	double Looked = 0;
	/** A recording that could walk is within sight of the player now (the curve hurries for it). */
	bool bWaiting = false;

	bool Busy() const { return Phase != TEXT("idle"); }
	/** Where it is now, at the head, while it is there and can be made out (the log watches this: a
	 *  dark shape on dark ground at 150 m has not been seen, however the eyes are turned). */
	TOptional<FVector> At() const;
	/** Where it is, at the head, whether or not it can be made out (unset while it is not walking). */
	TOptional<FVector> Spot() const { return Rec.IsValid() ? TOptional<FVector>(Head) : TOptional<FVector>(); }
	/** The one that would walk if it could, from where the player is now. */
	TSharedPtr<FS47Rec> Candidate(const FS47RecordingIn& S) const;

	void Update(double Dt, const FS47RecordingIn& S);
	/** It walks now (the curve and one-at-a-time have said so; the developer menu, the tests). */
	void Begin(const TSharedPtr<FS47Rec>& R, double Clock);
	/** A night put back or left (nothing of it walks on; the one standing stands). */
	void Reset();

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47RecordingDeps D;
	TSharedPtr<FS47Rec> Rec;
	double T = 0, St = 0, Shown = 0, Hitch = 0, NextHitch = 8, PhaseStep = 0, YawNow = 0;
	FVector Prev = FVector::ZeroVector, V = FVector::ZeroVector, W = FVector::ZeroVector, Head = FVector::ZeroVector;
	bool bNoted = false, bSawRestart = false;
	/** It can be made out from where the player is now (lit, or near enough). */
	bool bClear = false;
	/** How many times each time away has begun to walk tonight (n). */
	TMap<int32, int32> Tries;

	void Place(double Dt, FS47Rec& R);
	void End(const FString& Why);
	FS47Cam Cam() const { return D.Camera ? D.Camera() : FS47Cam(); }
};
