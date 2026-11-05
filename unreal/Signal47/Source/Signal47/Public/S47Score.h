#pragma once

#include "CoreMinimal.h"
#include "S47WebAudio.h"

class US47Main;
class FS47MediaSource;

// core/score.ts: the pieces, and what each place plays. Empty cue means TypeScript's null.
struct FS47ScoreTrack
{
	FString File, Title, By;
	double Gain = 0;
};
namespace S47Score
{
	extern SIGNAL47_API const TMap<FString, FS47ScoreTrack> TRACKS;
	extern SIGNAL47_API const TMap<FString, TArray<FString>> CUES;
	inline constexpr bool bScoreOn = true; // Unreal has files beside the game, unlike the web single-file build.
}

// core/score.ts: a quiet bed chosen by place, out of the way of voices, scenes and the radio.
class SIGNAL47_API FS47Score
{
public:
	FS47Score(US47Main* InM, const TSharedRef<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& InDest);
	~FS47Score();

	FString Playing, Track;
	void Update(double Dt, const FString& Want, bool bHeld);
	FString Pick(const FString& Cue) const;
	void Start(const FString& Cue, const FString& InTrack = FString());
	void Fade(double Secs, double Quiet);
	void Stop();
	void Pause(bool bOn);
	// Unreal adaptive score. Requests describe perceived play, never hidden enemy state.
	// Consumed by Update: callers repeat a request while its cause remains observable.
	void RequestMood(const FString& Cue, double Severity = 0);
	struct FTrackTiming { double Bpm = 0; int32 BeatsPerBar = 4, PhraseBars = 4; };
	bool SetTrackTiming(const FString& InTrack, double Bpm, int32 BeatsPerBar = 4, int32 PhraseBars = 4);
	int32 ActiveDecks() const { return int32(El.IsValid()) + int32(Outgoing.IsValid()); }
	FString PendingCue() const { return Pending; }

	// main.ts musicWant/musicHeld: the state already ported is read through the deps. The night's
	// other modules supply these exact properties when ported, rather than duplicating their state.
	FString MusicWant() const;
	bool MusicHeld() const;
	// Below 1 while the score stays audible under something the player is listening to.
	double MusicDuck() const;
	void SetDuck(double Level);
	struct FHeldHooks
	{
		TFunction<bool()> RoadsideSilent, MotelTvHeard, AbductionBusy, RecWalkBusy;
		TFunction<double()> FaceQuiet, NinthQuiet;
		TFunction<bool()> RoofStepsBusy, ReelTapeBusy, SaroDarkBusy, OfficePhoneBusy;
		TFunction<bool()> ArchiveBusy, ArrayBackBusy, TruckBRacing, MorningBusy;
	} Held;
	static bool SelfTest(TArray<FString>& Lines);
	static bool RenderTour(const FString& Path, TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	TSharedRef<FS47AudioContext> Ctx;
	TSharedPtr<FS47AudioNode> Dest;
	TSharedPtr<FS47GainNode> Bus; // every deck passes this, so a duck leaves deck fades alone
	TSharedPtr<FS47MediaSource> El;
	TSharedPtr<FS47GainNode> Gain;
	TSharedPtr<FS47MediaSource> Outgoing;
	TSharedPtr<FS47GainNode> OutgoingGain;
	TMap<FString, FTrackTiming> Timings; // Empty until a track has verified timing metadata.
	FString RequestedMood, Candidate, Pending;
	double RequestedSeverity = 0, CandidateSince = 0, PendingAt = 0;
	double ActionUntil = 0, RecoveryUntil = 0, OutgoingUntil = 0, RampUntil = 0, TargetGain = 0;
	double PauseAt = 0, PausedRampLeft = 0, PausedOutgoingLeft = 0, FadeQuiet = 4;
	bool bRecovering = false, bPaused = false, bHeldNow = false;
	uint64 Generation = 0;
	double Rest = 0, T = 0;
	TMap<FString, double> Last;
	bool bFading = false;
	TSharedRef<bool> bAlive = MakeShared<bool>(true);
	void Done(double Quiet);
	void RetireOutgoing();
	void Begin(const FString& Cue, const FString& InTrack, double FadeSeconds);
	double TransitionAt(bool bUrgent) const;
};
