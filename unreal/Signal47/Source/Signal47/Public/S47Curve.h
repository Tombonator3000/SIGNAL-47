#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Seen.h"

// web/src/story/curve.ts: one pressure for the whole night (FORSLAG.md, 20 October, item 1;
// HENDELSER.md, section 4: build up, peak, breathe out). The night's things ask this one curve
// before they start.
//
//   plain   from the moment chapter one lets the player out (02:16), ten minutes of plain night:
//           the crickets, the wind, and nothing else.
//   build   the signs pile up, slowly, then quicker: the crickets thin out one by one (Ambience
//           Thin), the flashlight stutters, the dash lights blink, the truck's radio slides under
//           its own static, a call far off out in the dark (core/calls.ts). The cardboard alien
//           turns up somewhere out of the view, in the build only. From half way, one of the
//           night's events (story/nightEvents.ts, the town's, the walk to the motel).
//   peak    one of the big ones: a glimpse at the edge of the view, the warning (the time away,
//           the dishes in the field), the steps on the archive roof, or a 47-second event (the
//           silence, the 1947 bulletin, the payphone, the motel's television), at most one of
//           those extra ones in a chapter. What the player actually got moves the curve (story/
//           seen.ts): a glimpse counts once it was looked at; one that stood there unseen does not,
//           and the peak goes on. Out where nothing can reach the player (indoors) the peak waits.
//   rest    8 to 12 minutes in which nothing can happen: no calls, no cutout, no glimpses, no
//           events. The crickets come back. Then the next build.
//
// Each cycle is a little stronger than the one before it, towards 05:29: a shorter build, the
// signs closer together, the night thinner. The curve counts the player's play time, not the
// night's clock. Nothing in the prologue, in a cinematic, on a call, in Nora's room, inside the
// diner in chapter five, or from chapter six. One thing at a time stays as it was: while one of the
// night's things is going, nothing else may start (main.ts, oneAtATime).
//
// Two of the 47-second events belong to a place and wait there for the player (the motel office's
// television, the payphone's first call in Caliche): where one of them is waiting now, the curve
// hurries (what is left of the build goes in at most HURRY seconds, indoors too) and the peak keeps
// itself for that event; staying by it (LINGER), the rest keeps only its first three minutes. The
// recording walking where the player was taken (story/recording.ts) waits at its place the same way
// (waitingScare). Chapter one's 47 is the story's (SARO dark, story/saroDark.ts) and chapter two's
// the archive's phone (story/archivePhone.ts): until they have come, no other 47 takes the chapter
// (keep). Earl's reel, reel 047, the face on the film, the payphone's second call, the office's
// crank telephone, the array put back on the schedule (again and again: one in a rest does not
// start the rest over), the truck on B: each lands the peak when it comes (FELT, SEEN).

/** What is kept with the case: { phase, t, len, cycle, long, ebb? }. */
struct FS47CurveIn
{
	/** The story's part of the night: 'ch1' to 'ch5' run the curve; the prologue and chapter six do not. */
	FString Chapter;
	/** Held still, and nothing may start: a cinematic, a panel or the notebook open, a call on the
	 *  line, Nora's room, inside the diner in chapter five. */
	bool bHeld = false;
	/** Out where the night can reach the player: on foot outdoors, or driving. */
	bool bExposed = false;
	/** One of the night's things is going: nothing else may start (one at a time). */
	bool bBusy = false;
	/** The peak's own thing is going: the rest after it counts from when it is over. */
	bool bPeaking = false;
	/** The night's clock, seconds since midnight. */
	double Clock = 0;
	/** One of the 47-second events that belong to a place is waiting there now. */
	bool bWaiting = false;
	/** A 47 of the story's may still come in this chapter: the chapter's one extra 47 is kept for it. */
	bool bKeep = false;
	/** The recording could walk within sight of the player now. */
	bool bWaitingScare = false;
};

class SIGNAL47_API FS47NightCurve
{
public:
	explicit FS47NightCurve(FS47SeenLog& InLog) : Log(InLog) {}

	/** 'off' | 'plain' | 'build' | 'peak' | 'rest' */
	FString Phase = TEXT("off");
	/** Seconds into the phase, and how long it runs (the build's length; the peak's wait). */
	double T = 0;
	double Len = 0;
	/** 0..1: how far up this cycle is (the build rises to 1, the peak holds it, the rest lets it go). */
	double Level = 0;
	/** How many peaks have come tonight, and how many builds have begun (since the case was opened). */
	int32 Cycle = 0;
	int32 Builds = 0;
	/** 0..1 over the night, 02:16 to 05:29: each cycle a little stronger. */
	double Strength = 0;
	/** The chapters that have had their one extra 47-second event. */
	TSet<FString> Long;
	/** The last peak that landed: what, at what clock. */
	struct FLanded { FString Kind, What; double Clock = 0; };
	TOptional<FLanded> Landed;
	/** The last rest came after a peak that never landed (shorter). */
	bool bEbb = false;
	/** The curve stands aside: everything as on its own timer before it, and no signs of its own
	 *  (the tests of a single thing, the developer menu). */
	bool bFree = false;
	/** A small sign is due now (main.ts gives one where it can: the flashlight, the dash, the radio). */
	bool bSign = false;
	/** A 47-second event is waiting where the player is, and the curve hurries for it. */
	bool bPulled = false;
	/** What the curve is hurrying for: a waiting 47 ('long') or the recording ('scare'). */
	FString PullKind = TEXT("long");
	/** 0..1 while the curve hurries in the build (the place shows it); 0 otherwise. */
	double Unrest() const { return bPulled && PullKind == TEXT("long") && Phase == TEXT("build") ? Level : 0; }
	/** How many of each started in this cycle (the calls, the cutout, the events). */
	struct FTook { int32 Call = 0, Cutout = 0, Event = 0; } Took;

	/** May a thing of this kind start now? 'sign' | 'call' | 'cutout' | 'post' | 'event' | 'long' |
	 *  'glimpse' | 'abduct' | 'scare' */
	bool May(const FString& Kind) const;
	/** May the cutout that is out and not yet found stay there? Only while the build is on. */
	bool CutoutKeep() const;
	/** How much sooner a thing that waits on a timer comes now (1: its own pace). 'call' | 'glimpse' | 'event' | 'cutout' */
	double Pace(const FString& Kind) const;
	/** How thin the crickets are, 0..1 (Ambience Thin). */
	double Thin() const;
	/** Seconds left of the phase. */
	int32 Left() const { return FMath::Max(0, int32(FMath::RoundHalfFromZero(Len - T))); }
	/** A thing has started (main.ts sees it): 'call' | 'cutout' | 'event'. */
	void Started(const FString& Kind);

	void Update(double Dt, const FS47CurveIn& S);

	/** What is kept with the case. */
	S47J::FObj Data() const;
	/** A save opened (its curve, or none: an older case goes on after a short quiet), or a new night. */
	void Restore(const S47J::FObj& Save, bool bStarted);

	// ---------- the developer menu and the tests ----------
	/** Go to a phase now (in the build: at this level). */
	bool Force(const FString& To, double InLevel = 0);
	/** Let this much of the night's play time go by at once, out in the night with nothing going. */
	void Advance(double Secs);
	/** One line for the overlay (the log here). */
	FString Text() const;

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47SeenLog& Log;
	double PullT = 0;
	FString Chapter;
	bool bKeep = false;
	bool bHeld = true;
	bool bBusy = false;
	double SignT = 0;
	int32 Read = 0;
	TArray<FS47SeenRef> Pending;
	FString Resume = TEXT("plain");
	struct FSave { FString Phase; double T = 0, Len = 0; bool bEbb = false; };
	TOptional<FSave> Restored;

	void Step(double Dt, bool bExposed, bool bInBusy, bool bPeaking);
	void Signs(double Dt, bool bExposed, bool bInBusy);
	void To(const FString& InPhase, TOptional<double> InLen = {});
	void ReadLog(double Clock);
	void Land(const FS47SeenEntry& E, double Clock);
	void Begin();
	void Stop();
};
