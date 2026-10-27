#pragma once

#include "CoreMinimal.h"
#include "S47WebAudio.h"

// web/src/core/ambience.ts. The desert outside, low and never the same for long (Tom, 5 October:
// the sounds outside should vary a little, quietly, so it is not the same sound all the time).
//
//   wind      the recording, its level and pitch wandering, under a synthesised wind whose
//             colour wanders too, with gusts now and then and calm spells
//   crickets  the recording, faint, and a few single crickets around the listener, each
//             with its own pitch, place and pace. They chirp slower as the night cools
//             (about 13 C at midnight, 6 C at dawn) and fall silent towards dawn.
//   events    far off and rare: an owl, a dog at a ranch, a gust in the scrub (recordings);
//             a poorwill, coyotes, a freight train's horn on the line to Roswell, a truck on
//             the highway, and birds when the sky greys (made here).
//   mood      how much is going on: it wanders every minute or two, and now and then the
//             night goes almost quiet for a while.
//   breath    once, the first time out into the yard in chapter one (02:16; FORSLAG.md, 20
//             October, small change 1, "the yard holds its breath"): nothing at all, not the
//             wind, not a cricket; then the crickets come back one at a time and the wind under
//             them, over 47 s. It is the plain night's (story/curve.ts), not one of the night's
//             events: nothing waits for it, and the curve's thinning takes over where it ends.
//   still     the morning holds its breath (story/morningHolds.ts): everything out here stops at
//             once, birds and all, and back() lets it come again, a bird first.
//   window    in the diner at dawn the birds come in through the glass (0..1): above the walls'
//             low-pass, faint, the rest of the desert as it is indoors.
//
// Everything goes through one outdoor level and a low-pass: indoors the walls take the top
// off and most of the level, in the cab the engine covers it. Nothing here answers the player.
// Positions are three.js metres.

class SIGNAL47_API FS47Ambience
{
public:
	FS47Ambience(const TSharedRef<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& Dest, const TMap<FString, TSharedPtr<FS47AudioBuffer>>& InBuf,
		const TSharedPtr<FS47AudioBuffer>& InNoise, const TSharedPtr<FS47AudioBuffer>& InBrown);

	/** The birds through the diner's windows, 0..1 (main.ts: in the diner, indoors). */
	double Window = 0;
	/** How far the night has thinned out, 0..1 (story/curve.ts, its build): the crickets drop out one
	 *  by one and the far-off life comes less often, as if something were listening. Back at 0 they
	 *  come back the same way. */
	double Thin = 0;

	/** The night goes quiet for a few seconds, the crickets and the wind together, and comes back
	 *  slowly (core/calls.ts: before something out in the dark). tau and back: how fast it goes to
	 *  level and comes back (about a third of each in seconds: story/abduction.ts starts the
	 *  crickets again all at once). */
	void Hush(double Secs, double Level = 0.08, double Tau = 0.35, double BackTau = 2.2);
	/** The yard holds its breath (the first time out, chapter one): at once nothing at all; then the
	 *  crickets one at a time, the wind coming up under them, and the crickets' bed last, all back by
	 *  `secs`. Then the night as it was, thinned as the curve has it (`thin`). */
	void Breath(double Secs = 47);
	/** The morning holds its breath (story/morningHolds.ts): at once nothing at all out here, no
	 *  bird, no wind, nothing far off, until back(). */
	void Still();
	/** And it lets go: the desert back over a few seconds, and a bird `bird` seconds from now, the
	 *  first one; the others as before. */
	void Back(double Bird = 0.4);
	/** Held still now (tests). */
	bool IsStill() const { return bStilled; }
	/** How many birds have called so far (tests). */
	int32 Birds = 0;
	/** The breath is held now (tests): how far into it, and how many crickets are back. */
	struct FBreathing { double T = 0, Len = 0; int32 Back = 0, Of = 0; };
	TOptional<FBreathing> Breathing() const;
	/** Single crickets chirping now, near the listener (tests). */
	int32 CricketsNow() const;
	/** Circadian regression checks for the extended evening and restored daily clocks. */
	static bool SelfTest(TArray<FString>& Lines);

	/** Where the listener is: outside, indoors (muffled) or in the cab (under the engine). */
	void SetSpace(const FString& InSpace);
	/** Every frame while a night runs. clock: the night's clock in seconds (it may run past 24 h). */
	void Update(double Dt, const FVector& At, double Clock);
	/** A recording far off in a random direction: panned, quiet and dull with distance. */
	void Far(const FString& Name, const FVector& At, double Gain, double Rate = 1);

private:
	/** A value that wanders: now and then it picks a new target and an AudioParam drifts there. */
	struct FDrift
	{
		FDrift(FS47AudioParam& InParam, double InLo, double InHi, double Every0, double Every1, double Tau0, double Tau1);
		void Step(double Dt, double Now, double Scale = 1);
		double Next = 0, Value = 0;
		FS47AudioParam* Param = nullptr;
		double Lo = 0, Hi = 0, Every[2] = { 0, 0 }, Tau[2] = { 0, 0 };
	};
	/** One cricket in the grass: a pure tone in short pulses, three or four to a chirp. */
	struct FCricket
	{
		FCricket(const TSharedRef<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& Dest, const FVector& At);
		/** Chirps for the next half second, at `rate` chirps a second. */
		void Schedule(double Rate);
		void Fade(double Secs);
		TSharedRef<FS47AudioContext> Ctx;
		TSharedPtr<FS47OscillatorNode> Osc;
		TSharedPtr<FS47GainNode> Env, Vol;
		TSharedPtr<FS47PannerNode> Pan;
		double Next = 0;
		int32 Pulses = 3;
		double Pace = 1;
		FVector Pos = FVector::ZeroVector;
		double Level = 1;
		bool bDying = false;
		TSharedRef<bool> bGone = MakeShared<bool>(false);
	};

	TSharedRef<FS47AudioContext> Ctx;
	const TMap<FString, TSharedPtr<FS47AudioBuffer>>* Buf = nullptr;
	TSharedPtr<FS47AudioBuffer> Noise, Brown;
	TSharedPtr<FS47GainNode> Out, Quiet;
	TSharedPtr<FS47BiquadFilterNode> Lp;
	struct FWindRec { TSharedPtr<FS47BufferSourceNode> Src; TSharedPtr<FS47GainNode> Gain; TUniquePtr<FDrift> Level; double Rate = 1, RateNext = 0; };
	TOptional<FWindRec> WindRec;
	struct FWind { TSharedPtr<FS47GainNode> Gain; TSharedPtr<FS47BiquadFilterNode> Bp; TUniquePtr<FDrift> Level, Colour; } Wind;
	struct FBugRec { TSharedPtr<FS47BufferSourceNode> Src; TSharedPtr<FS47GainNode> Gain; TUniquePtr<FDrift> Level; };
	TOptional<FBugRec> BugRec;
	TArray<TSharedPtr<FCricket>> Crickets;
	// the breath (breath()): the wind and the crickets' bed each on a gate of their own, so the
	// single crickets can come back one by one over the quiet
	TSharedPtr<FS47GainNode> WindIn, BedIn;
	struct FHeld { double T = 0, Len = 0; TArray<double> Cues; int32 N = 0; };
	TOptional<FHeld> Held;
	// still() and the window: a gate after everything, and the birds on a bus of their own that also
	// goes past the walls' low-pass, through the glass
	TSharedPtr<FS47GainNode> Gate, BirdBus, Win;
	double WinK = -1;
	bool bStilled = false;
	struct FTimes { double Gust, Life, Poorwill, Coyote, Train, Truck, Birds, Mood, Bugs; } T;
	double Mood = 0.7;
	FString Space = TEXT("room");

	TSharedRef<FS47GainNode> Distant(const FVector& At, double D, double Gain, double Cut, TOptional<double> Bearing = {}, TSharedPtr<FS47AudioNode> Dest = nullptr);
	TSharedRef<FS47GainNode> Echo(const TSharedPtr<FS47AudioNode>& Into, double Delay = 0.22, double FbGain = 0.35);
	void Note(const TSharedPtr<FS47AudioNode>& Dest, double At, double Len, double F0, double F1, double Peak, double Harm = 0.12);
	void Gust();
	void Poorwill(const FVector& At);
	void Coyotes(const FVector& At);
	void Train(const FVector& At);
	void Truck();
	void Meadowlark(const FVector& At);
	void Lark(const FVector& At);
};
