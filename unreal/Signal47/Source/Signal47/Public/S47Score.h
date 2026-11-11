#pragma once

#include "CoreMinimal.h"
#include "S47WebAudio.h"

class US47Main;
class FS47MediaSource;
class FS47CosmicAmbient;

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
	bool HasAudio() const { return El.IsValid() || Outgoing.IsValid() || Cosmic.IsValid(); }
	int32 ActiveDecks() const { return int32(El.IsValid()) + int32(Outgoing.IsValid()); }
	FString PendingCue() const { return Pending; }

	// main.ts musicWant/musicHeld: the state already ported is read through the deps. The night's
	// other modules supply these exact properties when ported, rather than duplicating their state.
	FString MusicWant() const;
	bool MusicHeld() const;
	// Below 1 while the score stays audible under something the player is listening to.
	double MusicDuck() const;
	void SetDuck(double Level);
	// D45-20261110 (Tom, d45: the town's saxophone went on under the horror bed at the feed store). How far the score stands back, 0 to
	// 1, for the bought ground (Dread, FS47FieldRadio::DreadNow) and, in Freeform, for something the set hears coming (Heard). A
	// place's own piece (town, dawn, the motel, the drive) goes all but under, to about a twentieth; the adaptive cues keep a third.
	static double DreadDuck(bool bPlacePiece, double Dread, double Heard = 0);
	static bool PlaceCue(const FString& Cue);
	// The piece playing now, or the one still fading out under it, is a place's own.
	bool PlacePieceSounding() const;
	// D45-20261110: in Freeform the take's flood and lift hold the score; only the ground and the light are heard then.
	// Stage is FS47Taken::EStage as a number.
	static bool TakenHolds(bool bFree, int32 Stage);
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
	TSharedPtr<FS47CosmicAmbient> Cosmic;
	TSharedPtr<FS47MediaSource> El;
	TSharedPtr<FS47GainNode> Gain;
	TSharedPtr<FS47MediaSource> Outgoing;
	TSharedPtr<FS47GainNode> OutgoingGain;
	TMap<FString, FTrackTiming> Timings; // Empty until a track has verified timing metadata.
	FString RequestedMood, Candidate, Pending;
	FString OutgoingCue; // D45-20261110: the cue of the deck fading out, for PlacePieceSounding
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
