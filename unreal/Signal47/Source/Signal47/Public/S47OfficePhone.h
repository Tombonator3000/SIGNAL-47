#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"
#include "S47Seen.h"

class AS47Web;
class US47Audio;
class FS47AudioContext;
class FS47AudioBuffer;
class FS47AudioNode;
class FS47GainNode;
class FS47BufferSourceNode;
class FS47ScheduledSourceNode;

// web/src/story/officePhone.ts. The crank telephone in the office of Sierra Motor Court plays the player
// back (FORSLAG.md, 20 October, item 7). The oak wall set on the office's north wall is the far end of
// the old field line from the hut at STATION 01; Nora said so on the hut's telephone in chapter three
// ("This line still rings in the motel office"). In chapter four, before room 6 is over, the first time
// the player comes to the office on foot (in it, onto the motel's court, which every way to room 6
// crosses, or within earshot of the office's open door), it rings: its own bell, two small gongs and a
// clapper, in uneven bursts, the way a magneto rings when somebody turns the handle at the other end.
// Nobody is at the other end.
//
// Picked up, nobody speaks. Down a narrow line, with the carrier of the receiver the player left on in
// the hut under it, comes what the hut's telephone registered of the player out there, in order and
// compressed: steps on the hut's boards, the hut door on its hinges, the steps going out; then, not far
// off, a shutter, and another, as far apart as FRAME 03 and FRAME 04 were taken (their real capture
// clocks, chapter three's s.f3 and s.f4; a long wait is drawn in, see ShutterGap); the hut's bell right
// at the mouthpiece; steps under it, across the boards right up to the telephone; the handset lifted,
// very faint from its earpiece Nora's first two lines ("So you went out there." "This line still rings
// in the motel office...", her recordings when the game has both, else the captions), someone
// listening, and the handset put down. An imprint, as R5 has it: it gives back what was registered and
// answers nothing; only, now the player is on it, as T. Vega's correction is. It prepares P12 in room
// 6, and 05:29:47.
//
// A night without the two frames gets what the player did at the hut: the steps, the door, the call.
// One thing at a time: it rings only when nothing else of the night's is going (main.ts, oneAtATime),
// and nothing starts while it rings or is off the hook, nor for a few seconds after. The night's curve
// does not gate it (it is the story's, like the payphone's second call), but once it rings it lands the
// curve's peak (story/curve.ts, FELT). Blocked as the player comes, it waits while the other thing goes
// and rings when nothing has gone for a while (AFTER) if the player is still there; missed or hung up
// on, it rings again the next time the player comes to the office before room 6 is over. Heard to the
// end it is written down once, kept with the town's part of the case ('ev:office', 'office@<clock>')
// and on the case board, with threads to the two frames.

struct FS47OfficePhoneIn
{
	/** In play, no panel, no cinematic, and nothing else of the night's going (one at a time). */
	bool bOk = false;
	/** Chapter four, before room 6 is over. */
	bool bOpen = false;
	/** On foot in the office, on the motel's court, or near the office's door. */
	bool bNear = false;
	/** On foot in SARO's area within earshot of the office (the bell rings on while they are). */
	bool bHere = false;
	/** The player's feet. */
	FVector Pos = FVector::ZeroVector;
	double Clock = 0;
};

struct FS47OfficePhoneDeps
{
	TWeakObjectPtr<US47Audio> Audio;
	/** The motel's front (MotelFront): its office telephone's proxy and handset. */
	TWeakObjectPtr<AS47Web> Web;
	/** The town's part of the case (main.ts, `town`). */
	FS47TownSeen* Seen = nullptr;
	TFunction<TArray<FString>()> Notes;
	TFunction<void(const FString&)> Note;
	TFunction<void(const FString&, float)> Toast;
	/** FRAME 03 and FRAME 04 as chapter three has them: their capture clocks (unset: no frame). */
	TFunction<TPair<TOptional<double>, TOptional<double>>()> Frames;
	TFunction<void()> Save;
	/** The case board, drawn again. */
	TFunction<void()> Refresh;
};

namespace S47OfficeLine
{
	/** How far apart the two shutters come on the line, from how far apart FRAME 03 and FRAME 04 were
	 *  taken (seconds of the night's clock): the same up to REAL seconds; beyond that drawn in, the
	 *  longer the wait the longer still, but never past CAP. */
	SIGNAL47_API double ShutterGap(double Dt);

	struct FCue { double At = 0; FString Kind; TOptional<double> Gain, Dur; };
	struct FCap { double At = 0; FString Text; double Secs = 0; };
	struct FPlan
	{
		TArray<FCue> Cues; TArray<FCap> Caps;
		/** Seconds between the frames as they were taken, and between the shutters on the line (unset without the frames). */
		TOptional<double> Delta, Gap;
		/** When the two shutters come, seconds from the pick-up (none without the frames). */
		TArray<double> Shutters;
		double End = 0;
	};
	/** What comes down the line, seconds from the pick-up. */
	SIGNAL47_API FPlan PlanLine(TOptional<double> F3, TOptional<double> F4, double Voice = 1.4, double Voice2 = 4.4);
	/** The office's own bell, made once: two small brass gongs a little apart in pitch and a clapper
	 *  between them, struck in turn as fast as the generator at the other end is turned. */
	SIGNAL47_API TSharedRef<FS47AudioBuffer> CrankBell(const TSharedRef<FS47AudioContext>& Ctx);
}

class SIGNAL47_API FS47OfficePhone
{
public:
	explicit FS47OfficePhone(FS47OfficePhoneDeps InD);

	/** 'idle' | 'ringing' | 'line' */
	FString State = TEXT("idle");
	/** How the last time went: 'heard' | 'away' | 'unanswered', empty: null. */
	FString Result;
	/** What the last call had on it, and when it began (the line's clock), for the tests. */
	TOptional<S47OfficeLine::FPlan> Plan;
	double StartedAt = 0;
	/** Whether Nora's recording played on it (the tests), and the captions given so far. */
	bool bVoiced = false;
	TArray<FString> Shown;
	/** Rung tonight (since the case was opened): how many times. */
	int32 Rings = 0;
	TUniquePtr<FS47RuleBook> Book;

	/** Ringing, on the line, or just put down: nothing else of the night's starts (one at a time). */
	bool Busy() const { return State != TEXT("idle") || Tail > 0; }
	/** Heard to the end tonight. */
	bool Heard() const { return D.Seen && D.Seen->Contains(TEXT("ev:office")); }
	/** The line's own card on the board, once heard. */
	TArray<FS47RuleBook*> Books() { return Heard() ? TArray<FS47RuleBook*>{ Book.Get() } : TArray<FS47RuleBook*>(); }

	// ---------- the interaction (story/Chapter4.ts asks first) ----------
	FString Label() const;
	/** Use on the telephone: answer it, hang it up, or (heard) lift it and hear the carrier. False:
	 *  nothing of this one's to do, the chapter's own words for it. */
	bool Use();

	void Update(double Dt, const FS47OfficePhoneIn& S);
	/** Put down before the end (Use, or walked off the end of the cord). */
	void HangUp(const FString& Text = FString());
	/** A new night or a case opened: quiet, on its hook, the threads the case has. */
	void Reset();
	/** It rings now (the developer menu, the tests), wherever the player is. */
	bool RingNow();

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47OfficePhoneDeps D;
	double T = 0;
	bool bWas = false;
	bool bArmed = false;
	bool bWaited = false;
	double Calm = 0;
	bool bOpened = true;
	TOptional<FVector> Pos;
	double RingT = 0;
	double Tail = 0;
	int32 CapAt = 0;
	TSharedPtr<FS47AudioContext> Ctx;
	TSharedPtr<FS47GainNode> Bus;
	TArray<TSharedPtr<FS47ScheduledSourceNode>> Loops;
	TArray<TSharedPtr<FS47AudioNode>> Nodes;
	struct FBell { TSharedPtr<FS47BufferSourceNode> Src; TSharedPtr<FS47GainNode> Gain; };
	TOptional<FBell> Bell;
	TArray<FString> Links;
	double RealStart = 0;

	double LineTime() const;
	FVector PhoneAt() const;
	void Ring();
	void StopBell();
	void Unanswered(bool bHere);
	void Answer();
	void Schedule(const S47OfficeLine::FPlan& P, const TSharedPtr<FS47AudioBuffer>& Voice, const TSharedPtr<FS47AudioBuffer>& Voice2);
	void LineStep(double Clock);
	void StopLine(double Fade = 0.04);
	void PutBack();
	void Finish(double Clock);
	FString AnsweredAt() const;
	FString NoteText(double Clock, const S47OfficeLine::FPlan& P) const;
	void ListenAgain();
	FString When() const;
	TArray<FS47BoardCard> Cards() const;
	void KeepLinks();
	void Handset(TOptional<bool> Visible, TOptional<double> RotZ) const;
};
