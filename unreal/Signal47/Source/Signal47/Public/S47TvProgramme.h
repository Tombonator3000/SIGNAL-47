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
	static bool SelfTest(TArray<FString>& Lines);

private:
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
