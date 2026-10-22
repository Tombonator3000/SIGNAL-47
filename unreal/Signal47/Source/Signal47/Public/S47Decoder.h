#pragma once

#include "CoreMinimal.h"
#include "S47WebAudio.h"

class FS47SignalVoice;

// web/src/core/decoder.ts. The signal processor on the control room rack: it plays back the tape of the
// locked anomaly, either as it came in (RAW, slowed down if you like) or harmonized, where every
// pulse of the 4/7 group becomes a note with a fifth and an octave over it, the first of each group
// gets a bass note, a slow pad holds underneath and a long reverb carries it. It sounds close to
// music without being any. The player sets pitch, time and space. All synthesis. The sound comes
// from the rack, so it keeps playing as you walk around.
//
// (Played by the processor's desk, story/Decoder.ts (S47DecoderDesk), and by reel 047, story/reel047.ts,
// which plays the pulse group through it raw. The reverb is a ConvolverNode over a 3.6 s decaying
// noise in the web version; S47WebAudio has no convolver, so a feedback comb bank of the same length
// and darkening stands in for it.)

struct FS47DecoderParams
{
	FString Mode = TEXT("harmonized");   // 'raw' | 'harmonized'
	double Pitch = 0, Stretch = 2, Space = 0.6;
};
/** Partial<DecoderParams> */
struct FS47DecoderSet
{
	TOptional<FString> Mode;
	TOptional<double> Pitch, Stretch, Space;
};

class SIGNAL47_API FS47DecoderPlayer
{
public:
	FS47DecoderPlayer(const TSharedRef<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& Dest, const FVector& At, const TSharedPtr<FS47AudioBuffer>& InNoise);
	~FS47DecoderPlayer();

	bool bPlaying = false;
	double Ducked = 1;
	FS47DecoderParams Params;
	/** Which pulse sounds now: 0 to 3 the 4-group, 4 to 10 the 7-group, -1 between. */
	int32 Current = -1;

	void Start(const FS47DecoderSet& P = FS47DecoderSet());
	void Set(const FS47DecoderSet& P);
	void Stop();
	/** Every frame: schedule what is due, and say which pulse is sounding (for the displays). */
	void Update();
	/** (Unreal) Steps the playback back under a phone or a voice: 1 as set, 0.25 under them. */
	void Duck(double K);

private:
	TSharedRef<FS47AudioContext> Ctx;
	TSharedPtr<FS47AudioBuffer> Noise;
	TSharedPtr<FS47GainNode> Out, Dry, Wet;
	TSharedPtr<FS47AudioNode> Verb;
	TArray<TSharedPtr<FS47AudioNode>> VerbLoops;
	TSharedPtr<FS47SignalVoice> Raw;
	struct FPad { TSharedPtr<FS47GainNode> Gain; TArray<TSharedPtr<FS47OscillatorNode>> Oscs; TSharedPtr<FS47BiquadFilterNode> Filter; };
	TOptional<FPad> Pad;
	double T0 = 0;          // audio time of the start of the current cycle
	int32 Next = 0;         // index of the next pulse to schedule within the cycle (0..10)
	int32 Cycle = 0;
	double Started = 0;
	struct FMark { double At; int32 I; };
	TArray<FMark> Marks;

	void Apply(const FS47DecoderSet& P);
	void StopVoices();
	void ApplySpace();
	void StartPad();
	void TunePad();
	void Note(double At, double Deg, double Len, bool bFirst);
	TSharedPtr<FS47AudioNode> MakeVerb(double Secs);
};
