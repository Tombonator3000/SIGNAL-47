#pragma once

#include "CoreMinimal.h"
#include "S47WebAudio.h"

// core/walkie.ts: Tomás' field radio from 1947, on the table in room 6 (KAPITLER.md, Room 6). It
// has had no battery since 1947 and it plays anyway, the same every time, like any reference: hiss,
// an engine far off that coughs twice and dies, the hiss gone, relay clicks in four and seven,
// somebody breathing close to the microphone, and the hiss again. It is THE EVENT on the Roswell
// road, an hour before it happens; nothing says so. Press to talk and the radio goes quiet (it is
// sending), let go and the clicks come back, four and seven. No voice (R5).
//
// Everything is made here and sent through a small speaker: high and low cut, a little clipping.
// The node graph is built on start and torn down on stop. The graphs are the web version's, line
// for line, on the game's own Web Audio (S47WebAudio.h).
namespace S47Walkie
{
	/** The relay's rhythm on the radio, four, a gap, seven (seconds from At); the key at 05:29 on the
	 *  line clicks the same (story/Chapter6.ts). RELAY */
	constexpr double RELAY_GAP = 0.32, RELAY_PAUSE = 0.9;
	SIGNAL47_API TArray<double> FourSeven(double At);
	/** One relay click at T into Dest (any context: the tests render it offline): a dry snap of noise
	 *  and the thump of the armature behind it. K scales it (1: as on the radio). relayClick */
	SIGNAL47_API void RelayClick(const TSharedRef<FS47AudioContext>& Ctx, const TSharedPtr<FS47AudioNode>& Dest, const TSharedPtr<FS47AudioBuffer>& Noise, double T, double K = 1);
	/** Somebody breathing close to the microphone, once, from At (1.3 s): in, and out. K scales it. breath */
	SIGNAL47_API void Breath(const TSharedRef<FS47AudioContext>& Ctx, const TSharedPtr<FS47AudioNode>& Dest, const TSharedPtr<FS47AudioBuffer>& Noise, double At, double K = 1);
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}

// WalkieSound
class SIGNAL47_API FS47WalkieSound
{
public:
	FS47WalkieSound(const TSharedRef<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& InDest, const TSharedPtr<FS47AudioBuffer>& InNoise)
		: Ctx(InCtx), Dest(InDest), Noise(InNoise) {}

	bool Active() const { return Out.IsValid(); }
	/** Switch on: the whole sequence. Returns its length in seconds; the hiss stays after it. */
	double Start();
	/** Press to talk: quiet while sending, then four and seven come back. Returns its length. */
	double PressToTalk();
	void Stop();

private:
	TSharedRef<FS47AudioContext> Ctx;
	TSharedPtr<FS47AudioNode> Dest;
	TSharedPtr<FS47AudioBuffer> Noise;
	TSharedPtr<FS47GainNode> Out, Hiss;
	TArray<TSharedPtr<FS47ScheduledSourceNode>> Srcs;
	TSharedPtr<FS47AudioNode> Speaker;

	void Build();
	TSharedRef<FS47BufferSourceNode> NoiseSrc();
	void Clicks(double At);
	void BreathAt(double At);
};
