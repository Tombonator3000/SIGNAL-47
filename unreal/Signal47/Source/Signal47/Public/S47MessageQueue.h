#pragma once
#include "CoreMinimal.h"

// MSG-ONE-20261109 (Tom, d37: «Når det kommer beskjeder, må det bare være én om gangen. Nå kom Dale og CB-radiotekst samtidig, så hele
// skjermen ble fylt med tekst.»): every line that goes on the screen goes through one queue (SS47Hud::Toast, US47UI::Toast and Alert).
// Two lanes. The speech lane is the caption at the foot: one speaker at a time (Dale on the walkie, the CB, the 1420 and the car radio,
// the telephone), the rest waiting their turn by priority: danger first, then Dale, then the CB and the radio. The hint lane is the
// plain toast without a speaker, also one at a time; while a caption is up it stands small and dimmed in a corner, never over it.
// A line already on screen or waiting is not added again. The next line of the speaker on screen (a CB call's second line) takes
// the place of the one before at once, once that has been up MinShow, so that the words keep with the voice. A line waits at most MaxWait seconds of play, and goes when it no longer
// holds (Valid: the CB once the player is out of the truck); a line timed by its sound (Clock) goes when that sound is over, and when
// it is shown late it is shown only for what is left of its sound. What a line says is never changed, only when it is shown.
// Pure: the HUD gives it the time; no engine, no widgets (the self test runs it).
class SIGNAL47_API FS47MessageQueue
{
public:
	enum class EPri : uint8 { Danger = 0, Dale = 1, Radio = 2, Hint = 3 };
	enum class ELane : uint8 { Speech = 0, Hint = 1 };
	static constexpr double MaxWait = 60.;   // seconds of play a line may wait before it is let go
	static constexpr double Gap = .35;       // between one line and the next in a lane: the old one fades out first
	static constexpr double MinShow = 1.2;   // a line timed by its sound with less than this left is let go, not flashed up

	struct FItem
	{
		int32 Id = 0;
		FString Text;
		float Secs = 3.2f;
		EPri Pri = EPri::Hint;
		TFunction<double()> Clock;   // the line's own clock (a recording's): it runs out on that
		TFunction<bool()> Valid;     // unset: always
		double Arrived = 0;          // the queue's time it came in (the order among equals)
		double Age = 0;              // seconds of play it has waited
		double SoundEnd = -1;        // on Clock: when its sound is over (-1: no clock)
		double Until = 0;            // shown: when it goes, on Clock if it has one, else on the queue's time
		double ShownAt = -1;         // shown: the queue's time it came up
		ELane Lane() const { return LaneOf(Pri); }
	};

	// What kind of line it is from its words: a speaker head ("WALKIE 1, DALE: ", "CB: ", "RADIO, a boy: ", "N. VEGA: ", "YOU: ") is
	// speech, Dale's above the rest; "COUNTER / " (the Geiger counter) is danger; anything else is a hint. bSound: it came with its
	// sound's clock, so it is speech whatever its head.
	static EPri Classify(const FString& Text, bool bSound = false);
	static ELane LaneOf(EPri P) { return P == EPri::Hint ? ELane::Hint : ELane::Speech; }
	// The speaker head of a caption, or empty: S47Game::SplitCaption's rule, but only the heads somebody speaks under.
	static FString Speaker(const FString& Text);

	// A line in. Returns its id (the one it was merged into, if it was already there).
	int32 Push(const FString& Text, float Secs, EPri Pri, TFunction<double()> Clock = {}, TFunction<bool()> Valid = {});
	// Now: the HUD's time (it always runs); PlayDt: how much of it was play (no pause, no panel), the time lines wait by.
	// True when what either lane shows has changed.
	bool Step(double Now, double PlayDt);
	// The line a lane shows now, or none.
	const FItem* Showing(ELane L) const { return Current[int32(L)].IsSet() ? &Current[int32(L)].GetValue() : nullptr; }
	int32 Waiting(ELane L) const;
	int32 Waiting() const { return Queue.Num(); }
	void Clear();
	// A line of the speech lane that must not start yet although the lane is free (Dale's voice still on the set for anyone else's).
	TFunction<bool(const FItem&)> Hold;
	int32 Dropped = 0, Merged = 0;
	static bool SelfTest(TArray<FString>& Lines);

private:
	TArray<FItem> Queue;
	TOptional<FItem> Current[2];
	double FreeAt[2] = { 0, 0 };
	double T = 0;
	int32 NextId = 1;
	bool bChanged = false;
	bool Over(const FItem& I) const;
	bool Stale(const FItem& I) const;
};

// TRUCK-HINT-20261109 (Tom, d37: «Hver gang man går inn i bilen, kommer det hjelpetekst som sier at man må kjøre til bensinstasjonen
// osv.»): the Freeform line about the Caliche gas station and GARAGE RECEIVING comes the first time in a run, and after that only when
// a situation it is about comes up that was not there the last time the player got in (the tank empty, a parcel on the rack, a fault
// waiting for a part), once for each. A situation that has passed can come again. The rest of the time it is in NOW/TONIGHT.
namespace S47TruckHint
{
	struct FState { bool bIntro = false; TSet<FString> Shown; };
	// Getting in now with these situations: whether the line is said. Shown becomes what holds now.
	SIGNAL47_API bool Due(FState& S, const TArray<FString>& Now);
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
