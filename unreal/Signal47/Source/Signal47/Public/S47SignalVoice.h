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
	FS47SignalVoice(const TSharedRef<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& Dest, const TSharedPtr<FS47AudioBuffer>& Noise);
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

	void Thump(double Strength);
	TSharedPtr<FS47AudioBuffer> NoiseBuf();
};
