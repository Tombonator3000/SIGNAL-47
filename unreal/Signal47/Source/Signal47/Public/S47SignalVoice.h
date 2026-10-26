#pragma once

#include "CoreMinimal.h"
#include "S47WebAudio.h"

// web/src/core/signalVoice.ts. The anomaly near 1420.405 MHz as you hear it while tuning in,
// after the signal in "Contact" (1997): the hiss of the array gives way to a pulsing, structured
// interference, then heavy low thumps like machinery cycling, with a metallic whine that rises
// and falls over them. The thumps keep this game's own rhythm, the 4/7 pulse group every 5.2 s
// (Signal.ts), not the film's prime numbers. All synthesis, no samples.
//
// clarity 0 is plain static; 1 is the locked signal. The receiver drives it every frame.

namespace S47SignalVoice
{
	// 4 pulses, a gap, 7 pulses, every 5.2 s; each pulse 0.11 s in a 0.22 s slot (as Rx.pulse).
	struct FPulse { bool bOn = false; bool bGroup = false; };
	SIGNAL47_API FPulse PulseAt(double T);
	// A soft-clipping curve: heavier `k`, more grit.
	SIGNAL47_API TArray<float> Drive(double K);
}

class SIGNAL47_API FS47SignalVoice
{
public:
	/** Rate: (Unreal only) the speed of a tape it plays from, 1 as received (S47Decoder.h). */
	FS47SignalVoice(const TSharedRef<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& Dest, const TSharedPtr<FS47AudioBuffer>& Noise, double InRate = 1);
	/** (Unreal only, on Tom's request: departs from the web version) The tape's speed: 0.5 plays it at
	 *  half speed, an octave down and twice as slow in every sweep, wobble and thump, as a slowed deck
	 *  does (the signal processor's RAW playback at TIME k plays at 1/k). It glides there. */
	void SetRate(double InRate);
	/** Called every frame: how clear the signal is (0..1), how loud overall, and the time. */
	void Frame(double InClarity, double InLevel, double T);
	/** Fade out (the receiver is off, the night is over); `stop` frees the nodes. */
	void Silence();
	void Stop();
	bool On() const { return Clarity > 0; }
	/** How clear it is right now (tests read this). */
	double Level() const { return Clarity; }

private:
	TSharedRef<FS47AudioContext> Ctx;
	TSharedPtr<FS47GainNode> Out, Stat, StatGate, Whine, TremDepth, ThumpBus;
	TSharedPtr<FS47BiquadFilterNode> WhineBand;
	TArray<TSharedPtr<FS47ScheduledSourceNode>> Nodes;
	bool bLast = false;
	double Clarity = 0;
	TSharedPtr<FS47WaveShaperNode> Shaper;
	TSharedPtr<FS47AudioBuffer> Nb;
	// (Unreal: the comb's feedback loop, broken when the voice stops so its nodes are freed)
	TSharedPtr<FS47AudioNode> Comb;
	// (Unreal only) every rate and pitch in it with its value at speed 1, and the delay times (which
	// scale the other way); the tape's speed
	struct FRated { FS47AudioParam* P = nullptr; TSharedPtr<FS47AudioNode> Keep; double Base = 0; bool bTime = false; };
	TArray<FRated> Rated;
	double Rate = 1;
	void Rates(const TSharedRef<FS47AudioNode>& Node, FS47AudioParam& P, double Base, bool bTime = false);

	void Thump(double Strength);
	TSharedPtr<FS47AudioBuffer> NoiseBuf();
};
