#pragma once

#include "CoreMinimal.h"
#include "S47Audio.h"
#include "S47Seen.h"

// web/src/story/nightEvents.ts. More that can happen in the night, anywhere (Tom, 18 October: "we
// need more events and things that can happen in the game; more alive, more frightening"). The
// town has its own director (story/townEvents.ts); this one is for everywhere else. One thing at a
// time, a few minutes apart, at night only, and each in the notebook in the player's own words.
// Nothing chases and nothing answers (HENDELSER.md, the rules). The night's curve (story/curve.ts)
// says when: the small ones in the second half of its build, the 47-second ones (the bulletin, the
// silence) as a peak, one extra in a chapter, and none in the quiet after a peak.
//
//   steps     on foot outside: the player stops, and two or three steps go on behind them, on
//             the same ground, and stop. Up to three a night.
//   knocks    driving at night: three slow knocks on the cab roof, from above. Once.
//   radio     walking back to the parked truck: its radio is on, on 1090, though it was left
//             off. Once.
//   bulletin  driving with the radio on 1420: a news bulletin from July 1947 about a flying disc
//             on a ranch near Roswell, in the newsreader's voice, and then the Night Line again
//             as if nothing had been (HENDELSER.md, event 4). Once.
//   phone     outside near the SARO building: the control room's phone rings once, and stops.
//   door      outside at SARO, well away: the service door, shut, is found open. Once.
//   silence   on foot outside anywhere: the crickets, the wind and the footsteps stop for 47 s
//             (HENDELSER.md, event 3; core/roadside.ts does it, and on the walk to the motel too).

/** The bulletin's lines, in the newsreader's voice (radio.stations.caliche.cutIn). */
struct FS47BulletinLine { FString Text, Who, By; };
SIGNAL47_API const TArray<FS47BulletinLine>& S47Bulletin();

/** radio.ts ArchiveProgress: a recording a station is playing (the bulletin), by its id. */
#ifndef S47_ARCHIVE_PROGRESS
#define S47_ARCHIVE_PROGRESS
struct FS47ArchiveProgress { int32 Id = 0; FString State; double Elapsed = 0, Duration = 0; }; // state: 'loading' | 'playing' | 'complete' | 'cancelled' (also in S47Radio.h)
#endif

struct FS47NightIn
{
	/** At night, in play, nothing else holding the player (main.ts). */
	bool bOk = false;
	double Clock = 0;
	FS47Cam Cam;
	/** On foot: where, whether outside, and whether walking just now. */
	struct FFoot { FVector Pos = FVector::ZeroVector; bool bOutside = false, bMoving = false; };
	TOptional<FFoot> Foot;
	/** Driving at night (any road). */
	bool bDriving = false;
	/** The truck in this area while on foot, and whether its radio is on. */
	struct FTruck { FVector Pos = FVector::ZeroVector; bool bRadioOn = false; };
	TOptional<FTruck> Truck;
	/** Driving with the truck's radio on 1420 and on the air. */
	bool bRadio1420 = false;
	/** In SARO's area, outside: the building's phone and the service door. */
	struct FSaro { FVector Phone = FVector::ZeroVector, Door = FVector::ZeroVector; bool bDoorOpen = false; };
	TOptional<FSaro> Saro;
	/** The silence of core/roadside.ts can come now (not had in this case). */
	bool bSilenceFree = false;
	/** The night's curve lets one start now: the events, and the 47-second ones. Unset: they may. */
	TOptional<bool> May, MayLong;
	/** How much sooner the next comes than its own pace (the curve's build). */
	TOptional<double> Pace;
};

struct FS47NightDeps
{
	TWeakObjectPtr<US47Audio> Audio;
	TFunction<void(const FString&)> Note;
	FS47TownSeen* Seen = nullptr;
	/** The kind of ground under the player, for the steps behind. */
	TFunction<US47Audio::FStepPick()> Step;
	TFunction<void(const FString&)> SetRadio;
	TFunction<double()> Bulletin;
	/** Still in the cab, tuned to this recording, and able to hear it (unset: no such check). */
	TFunction<bool()> BulletinHeard;
	/** Actual station episode progress (unset: the bulletin's own length is counted down). */
	TFunction<TOptional<FS47ArchiveProgress>()> BulletinProgress;
	TFunction<void()> OpenDoor;
	TFunction<void()> Silence;
	TFunction<FString()> Place;
	/** The 47 s of silence are on (core/roadside.ts). */
	TFunction<bool()> Silent;
};

class SIGNAL47_API FS47NightEvents
{
public:
	explicit FS47NightEvents(FS47NightDeps InD);

	/** The last one, for the developer menu and the tests. */
	struct FLast { FString Id; double Clock = 0; };
	TOptional<FLast> Last;

	/** Something of this director's is going on (the town's director waits for it). */
	bool Busy() const;
	/** Something is playing out now (the steps, the knocks, the phone, the bulletin, the silence);
	 *  not the radio left on or the door left open, which wait to be come back to (one at a time). */
	bool Running() const;

	void Update(double Dt, const FS47NightIn& S);
	/** One now if it can be (the developer menu, the tests). */
	bool Now(const FString& Id, const FS47NightIn& S);
	/** Where the steps behind the player were, the last time (story/seen.ts). */
	FVector StepsAt() const { return StepAt; }
	/** A new night, the title, a save opened. */
	void Reset();

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47NightDeps D;
	double Wait = 0;
	double Walked = 0;
	double Still = 0;
	TArray<double> StepsLeft;
	FVector StepAt = FVector::ZeroVector;
	struct FPending { FString Id; double T = 0; TFunction<bool()> Check; TOptional<bool> Heard; TOptional<int32> ArchiveId; };
	TOptional<FPending> Pending;
	double RadioNear = 1e9;
	FVector DoorAt = FVector::ZeroVector;
	bool bDoorSeen = false;

	bool IsSilent() const { return D.Silent && D.Silent(); }
	int32 Count(const FString& Id) const;
	void Done(const FString& Id, double Clock);
	bool Can(const FString& Id, const FS47NightIn& S) const;
	static bool InView(const FS47Cam& Cam, const FVector& At);
	bool Start(const FString& Id, const FS47NightIn& S);
	void StepsBehind(double Dt, const FS47NightIn& S);
	void Finish(const FString& Id, const FS47NightIn& S);
};
