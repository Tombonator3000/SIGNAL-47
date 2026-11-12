#pragma once
#include "CoreMinimal.h"
#include "S47WebAudio.h"

class US47Audio;

// Optional local TV footage (Scripts/prepare-tv.py): the frame pack in WebData/tv and its sound in
// Audio/Raw/tv, staged as loose files and never committed (the footage's rights are not cleared).
// One of these per set: its clock, its last decoded frame (one JPEG when the frame changes; no
// UObject per frame, no platform video decoder) and the set's little speaker, a looping buffer source
// in the game's Web Audio (S47WebAudio). While the sound plays the picture follows the sound's clock,
// so the two stay in step; when it is silent (the set off, or the picture taken over) the picture's
// clock goes on with the game's time, and the sound starts again where the picture is.
class SIGNAL47_API FS47TvProgramme
{
public:
	~FS47TvProgramme();
	bool Available() const;  // the frame pack is staged (without it the sets keep their test card or snow)
	bool HasSound() const;   // and its sound beside it, as long as the pictures
	double Length() const;   // the loop: every clip in turn, in seconds
	double Fps() const;      // the pictures a second (the sources' own)
	double Time = 0;         // the picture's clock, seconds into the loop

	/** Each frame. Level is the speaker's gain: 0 is silence (the sound fades and stops). At is where
	 *  the speaker is (three.js metres); bThrough: heard through a wall (duller and quieter). */
	void Update(double Dt, US47Audio* A, const FVector& At, double Level, bool bThrough);
	void Silence();
	bool Playing() const { return Src.IsValid(); }
	int32 FrameIndex() const;
	/** The frame at Time, 4:3, into Width x Height of a screen whose shape is Aspect (width / height):
	 *  black bars at the sides or above and below when the screen is not 4:3. */
	bool Frame(TArray<FColor>& Out, int32 Width, int32 Height, double Aspect = 4.0 / 3.0);
	/** The last decoded frame's mean brightness, 0..1 (what the set throws on the room). */
	double Brightness() const { return Lum; }
	FString Describe() const;

	/** A 4:3 picture Sw x Sh into Width x Height of a screen shaped Aspect: averaged, bars in black. */
	static void Fit(const TArray<FColor>& Src, int32 Sw, int32 Sh, TArray<FColor>& Out, int32 Width, int32 Height, double Aspect);

	/** TV-SCHEDULE-20261109 (Tom: the clip on the other sets too, in Freeform and in Story): the late station's hours, one schedule
	 *  for every set in the world. The programme goes out from half past eleven at night to half past three in the morning; the rest
	 *  of the day a set shows its own test card or snow. Clock: seconds after midnight (any day; wrapped). Without the frame pack
	 *  (Available false) a set keeps its card or snow at every hour. */
	static constexpr double ON_AIR = 23.5 * 3600, OFF_AIR = 3.5 * 3600;
	static bool OnAir(double Clock);
	/** -S47TvOnAir, -S47MotelTvFilm or a -S47MotelTv=programme... test: the programme at any hour (the tests' pictures). */
	static bool ForcedOnAir();
	static bool SelfTest(TArray<FString>& Lines);

	/** TV-CHANNEL-20261110 (Tom: every set on the one channel; at the end of the broadcast day the flag and the anthem, then the
	 *  test card or snow). One channel for every set in the world, stepped once a frame (S47Main). In Freeform the broadcast day
	 *  runs from six in the morning to two at night; when it ends in front of the player the station signs off: the flag and the
	 *  anthem (made here: the tune is two centuries old, the playing is a synthesiser's), SIGN_OFF real seconds, then the card for
	 *  a while, then snow until the card comes back a quarter of an hour before sign-on. The story keeps the late station's hours
	 *  (ON_AIR, OFF_AIR) and its snow. The programme's own clock (ChannelTime) runs in real seconds: a set tuned in later shows
	 *  the moment the others show. */
	enum class EAir : uint8 { Programme, SignOff, Card, Snow };
	static constexpr double DAY_ON = 6 * 3600, DAY_OFF = 2 * 3600, SIGN_OFF = 82, CARD_AFTER = 150;
	struct FChannel { EAir Air = EAir::Snow; double Into = -1, Time = 0, Clock = -1; bool bAsked = false; };
	static void Advance(FChannel& C, double Clock, double Dt, bool bDay, bool bForced);
	static void ChannelStep(double Clock, double Dt, bool bDay);
	static EAir Air();
	static bool Staged();         // the local frame pack is there (without it a set shows the card in the programme's hours)
	static double AirInto();      // real seconds into the sign-off (or the card after it)
	static double ChannelTime();  // the programme's clock, real seconds
	static void SignOffNow();     // F10: the sign-off at any hour
	/** Something on the air near a set (0 to 1), asked for by F10 for a few seconds; the sets take the greater of this and what
	 *  they feel themselves. */
	static void JamFor(double Level, double Seconds);
	static double JamAsked();
	/** The feed this set shows: the programme (the local frame pack), or the station's sign-off. Changing it moves the clock to At. */
	void Tune(bool bSignOff, double At);
	bool SignOff() const { return bSign; }
	/** The sign-off's picture, 320 x 240, T seconds in: the flag in a slow wind, up from black and down to it. */
	static void Flag(TArray<FColor>& Out, double T);
	static TSharedPtr<FS47AudioBuffer> Anthem();

private:
	bool bSign = false;
	int32 LastFrame = -1;
	TArray<FColor> Pixels;
	double Lum = 0;
	// the speaker: the graph's source and the nodes that change, and the sound's clock
	TSharedPtr<FS47AudioContext> Ctx;
	TSharedPtr<FS47BufferSourceNode> Src;
	TSharedPtr<FS47GainNode> Gain;
	TSharedPtr<FS47BiquadFilterNode> Wall;
	double From = 0, StartedAt = 0, Heard = 0, Stall = 0, Level = 0;
	bool bThrough = false;
	// (the self test's own loop and sound)
	double TestLength = 0;
	TSharedPtr<FS47AudioBuffer> TestSound;
	TSharedPtr<FS47AudioBuffer> SoundBuffer() const;
	void Step(double Dt, const TSharedPtr<FS47AudioContext>& InCtx, TFunctionRef<TSharedPtr<FS47AudioNode>()> Out, double InLevel, bool bInThrough);
	void Start(const TSharedPtr<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& Out);
	double Wrap(double Seconds) const;
};
