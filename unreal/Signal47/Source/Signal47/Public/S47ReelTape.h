#pragma once

#include "CoreMinimal.h"
#include "S47WebAudio.h"

class US47Audio;

// web/src/core/reelTape.ts. Earl's reel from 9/22/81, on the studio monitor at 1420 AM
// (story/lostNights.ts; FORSLAG.md, 20 October, item 11). A call taken off the telephone line on a
// night the station was off the air: the line's hiss and hum, and the boy who called the Night Line
// tonight, saying what he said tonight, word for word (the same recordings: core/radio.ts, the caller
// bank). Where Earl answered him tonight there is nothing on the tape at all, not even the hiss, for
// exactly as long as Earl talked (tonight's call as it went out: radio.boyCallTimes). The deck's motor
// and the reels go on through the holes; it is the tape that has nothing on it. At the end, a click
// on the line, as if a receiver were put down at the other end, and Earl stops the deck. All of it on
// the AudioContext's clock, and the lines for the conversation panel on the same clock, so a pause
// holds both. With the recorded callers turned off (or a clip that will not load), the boy is the
// caption alone, and the tape and its holes are the same.
//
// (The radio, core/radio.ts, is its own port: what the reel needs of it comes through the deps. Its
// caller bank loads short clips at once in the Unreal version, so there is no waiting for them here.
// The cues are stepped from the frame (Update) where the web version polls every 40 ms.)

/** 'deck' | 'start' | 'click' | 'end' | 'boy' (Text, Secs) | 'hole' (Secs, I) */
struct FS47TapeCue { double At = 0; FString Kind; FString Text; double Secs = 0; int32 I = 0; };
/** radio.boyCallTimes: the boy's lines and Earl's answers as tonight's call went out. */
struct FS47TapeLine { FString Text; bool bBoy = false; double Secs = 0, Gap = 0; };

namespace S47ReelTape
{
	/** Seconds from play() before the tape runs: Earl goes in and threads it on the deck. */
	inline constexpr double TAPE_BEFORE = 4.6;
}

struct FS47ReelTapeDeps
{
	TWeakObjectPtr<US47Audio> Audio;
	/** radio.boyCallTimes(radio) */
	TFunction<TArray<FS47TapeLine>()> BoyCallTimes;
	/** radio.callerVoice: the recorded callers are on. */
	TFunction<bool()> CallerVoice;
	/** The caller bank's recordings of the boy's lines (callerBank.resolve(text, 0, 'boy') and load):
	 *  one for each, or none. */
	TFunction<TArray<TSharedPtr<FS47AudioBuffer>>(const TArray<FString>&)> BoyVoices;
};

class SIGNAL47_API FS47ReelTape
{
public:
	explicit FS47ReelTape(FS47ReelTapeDeps InD);

	/** The reel is playing (one thing at a time: main.ts holds the music and the night for it). */
	bool bBusy = false;
	/** What the last play had on it, for the tests: whether the boy's own voice played, the clock it
	 *  started on (the AudioContext's, or the platform's seconds without sound), its cues. */
	bool bVoiced = false;
	double StartedAt = 0;
	TArray<FS47TapeCue> Cues;

	/** When each thing on the reel comes, in seconds from play(). */
	TArray<FS47TapeCue> Plan() const;
	/** Plays the reel. `Cue` gets each thing on it as it comes; `Done(true)` once the deck has been
	 *  stopped at the end, `Done(false)` if it was stopped before (Stop()). */
	void Play(TFunction<void(const FS47TapeCue&)> Cue, TFunction<void(bool)> Done);
	/** Stops the reel where it is (the conversation closed): not heard to the end. */
	void Stop();
	/** The cues due by now (window.setTimeout(step, 40) in the web version). */
	void Update();

private:
	FS47ReelTapeDeps D;
	int32 Gen = 0;
	int32 K = 0;
	double T0 = 0;
	bool bCtx = false;
	TFunction<void(const FS47TapeCue&)> OnCue;
	TFunction<void(bool)> OnDone;
	TArray<TSharedPtr<FS47ScheduledSourceNode>> Srcs;
	TArray<TSharedPtr<FS47AudioNode>> Nodes;
	TOptional<double> DuckedAt;
	TSharedRef<int32> Alive = MakeShared<int32>(0);

	double Now() const;
	void Build(const TSharedRef<FS47AudioContext>& Ctx, double InT0, const TArray<FS47TapeCue>& InCues, const TArray<TSharedPtr<FS47AudioBuffer>>* Voice);
	void Release(bool bNow = false);
};
