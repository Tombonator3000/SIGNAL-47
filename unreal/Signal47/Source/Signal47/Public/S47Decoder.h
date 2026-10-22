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

// (Unreal only: what the VU unit on the processor reads, S47ConsoleKit.h. The web version draws no
// meter. A pass-through node, where an AnalyserNode would sit: it sums the squares of what goes
// through it on the audio thread, and the game thread takes the level and clears it, under the
// context's lock.)
class SIGNAL47_API FS47LevelMeter : public FS47AudioNode
{
public:
	explicit FS47LevelMeter(const TSharedRef<FS47AudioContext>& InCtx) : FS47AudioNode(InCtx) {}
	/** The RMS of what went through since the last call, and cleared (the last one again if nothing
	 *  went through since: the audio comes in blocks, not once a frame). */
	double Take();
protected:
	virtual void Process(int64 Quantum) override;
private:
	FS47AudioBlock In;
	double Sum = 0, Held = 0;
	int64 Frames = 0;
};

// (Unreal only, on Tom's request: the tones' hidden answer, S47DecoderDesk.h.) The answer's timeline,
// in seconds from the seventh key: a beat, the four rising, the gap, the player's seven, on a clock K
// times slower than the tape's (slower than the panel's TIME); then the wire, TAPE A from 1947: its
// hiss, someone on the line whistling the four and then the same seven, the seven coming back as the
// processor has just played them (in the hut, close), a field telephone's crank, T. Vega's correction
// (the line's recording, VoiceSecs long; silence if there is none), cutters, and nothing at all.
struct FS47AnswerTimes
{
	double K = 3, Lead = 0.6, VoiceSecs = 3.6;
	static double KFor(double Stretch) { return FMath::Clamp(Stretch * 1.5, 3.0, 4.5); }
	double Four(int32 I) const { return Lead + I * 0.22 * K; }
	double Seven(int32 J) const { return Lead + (2.1 + J * 0.22) * K; }
	double Last() const { return Seven(6); }
	double Wire() const { return Last() + 2.4; }
	/** The whistle on the wire: 0 to 3 the four, 4 to 10 the seven, a breath between. */
	double Whistle(int32 J) const { return Wire() + 1.2 + J * 0.3 + (J >= 4 ? 0.3 : 0.0); }
	/** The seven coming back, on the answer's clock. */
	double Back(int32 J) const { return Whistle(10) + 1.2 + J * 0.22 * K; }
	double Crank() const { return Back(6) + 1.8; }
	double Voice() const { return Crank() + 3.2; }
	double Cut() const { return Voice() + VoiceSecs + 0.25; }
	double End() const { return Cut() + 2.0; }
};

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
	/** (Unreal only) The answer playing by itself (Phrase); and the degrees of the 4-group and of the
	 *  7-group as the harmonized playback sounds them (DEG_A, DEG_B). */
	bool bPhrase = false;
	static double FourDegree(int32 I);
	static double SevenDegree(int32 I);

	void Start(const FS47DecoderSet& P = FS47DecoderSet());
	void Set(const FS47DecoderSet& P);
	void Stop();
	/** Every frame: schedule what is due, and say which pulse is sounding (for the displays). */
	void Update();
	/** (Unreal) Steps the playback back under a phone or a voice: 1 as set, 0.25 under them. */
	void Duck(double K);
	/** (Unreal) The level going out of the processor since the last call (RMS), for the VU meters. */
	double TakeLevel() const { return Meter.IsValid() ? Meter->Take() : 0.0; }
	/** (Unreal only) The I-th note of the 7-group (0 the first and highest) as the harmonized playback
	 *  sounds it, now, with the panel's pitch, time and space (the ANSWER row's keys): over the playback
	 *  if it plays; alone if not, and then its bar lit on the displays while it sounds (Current). */
	void Key(int32 I);
	/** (Unreal only) The answer by itself (FS47AnswerTimes): whatever plays stops; the four, then the
	 *  seven as given (degrees), harmonized, slower, the reverb opening up; then the wire from 1947,
	 *  with T. Vega's correction on it when the line's recording is there (Voice). */
	void Phrase(const TArray<double>& Seven, const FS47AnswerTimes& Tm, const TSharedPtr<FS47AudioBuffer>& Voice = nullptr);

private:
	TSharedRef<FS47AudioContext> Ctx;
	TSharedPtr<FS47AudioBuffer> Noise;
	TSharedPtr<FS47GainNode> Out, Dry, Wet;
	TSharedPtr<FS47LevelMeter> Meter;
	double PhraseEnd = 0, PhraseK = 3;
	double KeyAt = -1; int32 KeyI = -1;   // (Unreal) the last key's note, lit while it sounds
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
	void Note(double At, double Deg, double Len, bool bFirst, double Kx = -1);
	void Wire(double At, const TArray<double>& Seven, const FS47AnswerTimes& Tm, const TSharedPtr<FS47AudioBuffer>& Voice);
	TSharedPtr<FS47AudioNode> MakeVerb(double Secs);
};
