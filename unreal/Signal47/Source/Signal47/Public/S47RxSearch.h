#pragma once
#include "CoreMinimal.h"

class US47Audio;
class FS47BufferSourceNode;
class FS47GainNode;
class FS47AudioBuffer;

// RX-SEARCH-SOUND-20261113 (-> M3, Tom 13 November: «Signal Sound Effect er en søke-lydeffekt til signalmottak som blir kraftigere og
// kraftigere, som jeg har laget, som kan mikses inn med eksisterende signallyder»): Tom's own search sound as a layer under the
// receiver's sounds, never in their place. On ACQUIRE it follows how close the reticle is to where the bearings cross and fades into
// the lock confirm; on TUNE it follows how close the tuning is to the carrier, holds a moment at the lock, then settles under the bed.
//
// The file is not monotonic over its 114 s: 14 s of growth (a faint air, then a ~1.07 s sonar pulse swelling from bright to deep) and
// then a steady 13 s pattern (measured: Tools/rxacquire/make_search.py). So it is staged as seven seamless loops, one per closeness band
// (search_0 the air far off ... search_6 the strong body), and the layer crossfades between the two neighbouring bands with equal power:
// the sound itself grows as the search closes in. The loops are Tom's and stay out of Git: staged by the recipe into the main
// checkout's git-ignored Content/Audio/Raw/media/rxsearch, read at first use; without them nothing plays and nothing fails.
//
// Through the SFX bus (the master volume and DuckAll hold it), at most -30 dBFS RMS at full: under the lock confirm and the speech.
// The constants are mirrored in the recipe's previews.
namespace S47RxSearch
{
	constexpr int32 Bands = 7;
	constexpr double LevelAcquire = .32, LevelTune = .26; // the layer at full (the body loop is -20 dBFS RMS in its file)
	constexpr double TauUp = .25, TauDown = .5, MaxSlew = 1.2; // closeness smoothing (s, s, closeness per second)
	constexpr double GainTau = .08, FadeTau = .12, LockFadeTau = .3;
	constexpr double Hold = 1.5, Settled = .35, SettleTau = 1.2; // TUNE: full for Hold s after the lock, then down to Settled
	constexpr double Duck = .3, Calm = .8;                      // under speech, the phone and a recording; the calm setting
	enum class EUse : uint8 { Off, Acquire, Tune };

	/** Equal-power weights of the seven bands for a closeness 0..1 (two neighbours at most; the squares sum to 1). */
	SIGNAL47_API void Weights(double Closeness, double (&Out)[Bands]);
	/** One frame of the held closeness toward Target: never past it, never more than MaxSlew * Dt (slower when calm). */
	SIGNAL47_API double Smooth(double Held, double Target, double Dt, bool bCalm);
	/** TUNE: the share of the layer this long after the carrier locked (0: not locked). */
	SIGNAL47_API double TuneSettle(double TunedFor);
	/** The layer's level for a frame: 0 when off or the ACQUIRE fix is held. */
	SIGNAL47_API double Level(EUse Use, bool bLocked, double TunedFor, bool bDuck, bool bCalm);
}

class SIGNAL47_API FS47RxSearch
{
public:
	struct FIn
	{
		S47RxSearch::EUse Use = S47RxSearch::EUse::Off;
		double Closeness = 0;   // ACQUIRE: FS47SkyAcquire::Closeness (1 while locking); TUNE: FS47Rx::Quality of the job's profile
		bool bLocked = false;   // ACQUIRE: the fix is held; TUNE: the carrier passes
		bool bDuck = false, bCalm = false;
	};
	/** The loops from Content/Audio/Raw/media/rxsearch, once (false: not there, the layer stays silent). */
	bool Load(US47Audio* Audio);
	/** Every frame. Use Off fades out (FadeTau) and lets the voices go. */
	void Frame(US47Audio* Audio, const FIn& In, double Dt);
	/** Stops at once (a short ramp): the session ending, the receiver monitor released with the view. */
	void Off(US47Audio* Audio);
	bool Loaded() const { return bLoaded; }
	int32 Voices() const;
	double Held() const { return HeldNear; }
	double Target(int32 Band) const { return Band >= 0 && Band < S47RxSearch::Bands ? Targets[Band] : 0; }
	/** Tests: these loops in place of the files (fewer than Bands: none, as when nothing is staged). */
	void UseLoops(const TArray<TSharedPtr<FS47AudioBuffer>>& InLoops);
	/** The mapping, and the graph on an unlocked US47Audio (US47Audio::ReceiverMonitorSelfTest makes one). */
	static bool SelfTest(US47Audio* A, TArray<FString>& Lines);

private:
	struct FVoice { TSharedPtr<FS47BufferSourceNode> Src; TSharedPtr<FS47GainNode> Gain; double QuietFor = 0; };
	TSharedPtr<FS47AudioBuffer> Loops[S47RxSearch::Bands];
	FVoice Voice[S47RxSearch::Bands];
	double Targets[S47RxSearch::Bands] = {};
	bool bTried = false, bLoaded = false;
	double HeldNear = 0, TunedFor = 0;
};
