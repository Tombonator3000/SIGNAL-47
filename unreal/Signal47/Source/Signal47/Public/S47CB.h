#pragma once

#include "CoreMinimal.h"
#include "S47WebAudio.h"

class US47Audio;

// core/cb.ts: the CB radio under the dash of the truck (Tom's improvements, 16 October: the CB is
// modelled but did nothing). Someone calls the truck: the squelch opens with a burst of hiss, a
// voice mumbles like the grown-ups in Peanuts, as the AM radio's voices do (core/radio.ts, the same
// horn behind a mute that opens on every syllable), each line captioned, and the squelch closes
// with a click of hiss. Through a narrower, harder speaker than the AM set, from the dash, at the
// radios' own level (AudioSys.radioBus). A line with a recording of its own (N. Vega's, the
// character voices: US47Audio::CharacterBuffer) is played as it is.

/** CBLine: the words, the voice that mumbles them (core/radio.ts Voice: 'host' | 'woman' | 'man' |
 *  'roswell' | 'boy' | 'news' | 'old') and who said it, for the caption. */
struct FS47CBLine { FString Text, Who, By; };

namespace S47CB
{
	// core/radio.ts, what the CB takes from it (here until the radio is ported): PITCH, syllables(), secsOf()
	SIGNAL47_API double Pitch(const FString& Voice);
	struct FSyllable { double At = 0, D = 0, F = 1; };
	/** The syllables of a line said in Secs: when each starts, how long it lasts, its pitch (times the
	 *  voice's own). A word gets one to four; the pitch wanders, the first of a word is a little higher,
	 *  a sentence falls at its end and a question rises. The same every time for the same line. */
	SIGNAL47_API TArray<FSyllable> Syllables(const FString& Text, double Secs);
	SIGNAL47_API double SecsOf(const FString& Text);

	/** When the CB opens by itself on the old road (story/Chapter6.ts; FORSLAG.md, 20 October, punkt 6),
	 *  seconds from the squelch opening: nobody calls and nobody speaks. On the channel, small and a long
	 *  way off, an engine idling like the truck's own (its idle recording, a little slower and a long way
	 *  down; the field radio's made engine until that is in), which coughs twice and dies, as the engine
	 *  on Tomás' radio does (core/walkie.ts). The hiss goes with it and the channel stays open on nothing
	 *  for three seconds; then the squelch tail, and shut. CB_ENGINE */
	struct FEngineTimes { double In = 0.5, Up = 1.4, Cough[2] = { 5.6, 6.9 }, Die = 8.0, Dead = 9.3, Tail = 12.3, End = 12.8; };
	SIGNAL47_API const FEngineTimes& ENGINE();
	/** The CB_ENGINE sequence from T0 (context time) into Dest, on any context (the tests render it
	 *  offline): the CB's own speaker (band-limited, a little clipping), its squelch, the engine.
	 *  Returns stop(), which silences it at once. engineOnChannel */
	SIGNAL47_API TFunction<void()> EngineOnChannel(const TSharedRef<FS47AudioContext>& Ctx, const TSharedPtr<FS47AudioNode>& Dest, const TSharedPtr<FS47AudioBuffer>& Noise, const TSharedPtr<FS47AudioBuffer>& Idle, double T0);
}

// CB
class SIGNAL47_API FS47CB : public TSharedFromThis<FS47CB>
{
public:
	explicit FS47CB(US47Audio* InAudio);

	/** A call is going on. */
	bool bBusy = false;
	/** While the engine is on the channel, where it comes out (the tests measure it there). */
	TSharedPtr<FS47AudioNode> Out;

	/** Plays a call from the truck at At (three.js terms), or in the cab with the player (unset).
	 *  OnLine is called as each line begins (for the caption, with how long it is), OnEnd when the
	 *  squelch has closed. Returns how long it takes, in seconds (0 if there is no sound yet or a
	 *  call is going on). */
	double Call(const TArray<FS47CBLine>& Lines, const TOptional<FVector>& At, TFunction<void(const FS47CBLine&, double)> OnLine, TFunction<void()> OnEnd = nullptr);
	/** The channel opens by itself with an engine on it (CB_ENGINE, EngineOnChannel): in the cab, a
	 *  little to the passenger side where the set hangs, into Dest (story/Chapter6.ts: the game's own
	 *  sounds, so the radios' level does not take it away). Idle: the truck's idle recording, or null.
	 *  Busy until Stop() or the end. Returns how long it takes (0 if there is no sound yet or a call is
	 *  going on). */
	double Engine(const TSharedPtr<FS47AudioBuffer>& Idle, const TSharedPtr<FS47AudioNode>& Dest);
	/** At once: a new night, the title, a save opened. */
	void Stop() { if (StopAll) { const TFunction<void()> F = StopAll; F(); } }

	static bool SelfTest(TArray<FString>& Lines);

private:
	TWeakObjectPtr<US47Audio> Audio;
	TFunction<void()> StopAll;
	uint32 StopId = 0, Calls = 0; // which stop() is the current one (cb.ts compares the functions)
};
