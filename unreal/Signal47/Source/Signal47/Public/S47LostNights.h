#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"
#include "S47Docs.h"

struct FS47TownSeen;

// web/src/story/lostNights.ts. The nights SARO printed a negative range are the nights Caliche lost
// time (FORSLAG.md, 20 October, item 11, and small tweak 4). The exceptions binder over the printer has
// three printouts from before tonight: 09/22/81 -34, 03/02/83 -36, 10/19/85 -38. The deputy's green pins
// are people who lost track of the time, three of them in town, on the same nights (story/caliche.ts):
// the boy who called the radio in September 1981, the man whose wife sits out at the laundromat, March
// 1983, and the attendant, October 19 last year. Nobody says so. On Night Shift's patch of the case board
// the binder lies with a pin on each printout and the green pins with a pin on each person, and a thread
// between a printout and its person holds. With all three nights tied, a question comes onto the cork
// that nothing answers: "-39 tonight. Who loses time tonight?" (After a time away, the answer is under it
// in the player's own hand, the minutes they lost, which they do not remember writing.)
//
// The 1981 night is also Earl's reel, the one call of twelve years he has never played (caliche.ts).
// Once the board has given the player the date, they can give it to him at the studio door before the
// station signs off at four, and he plays it (core/reelTape.ts): the boy says what the boy said on the
// Night Line tonight, word for word, and where Earl answered him tonight the tape has nothing on it.
//
// October 1985 has two more things: the carbon of the tow ticket, and the driver's watch, which has
// said two minutes to twelve ever since. He is at the counter in the Mesa Diner (story/Chapter5.ts:
// DriverExamine, DriverOpen, DriverChoices, DriverChoose are chapter five's driverMore).
//
// The threads are kept with Night Shift's part of the case (NightShiftState.ln); what the player has
// found with the town's ('ask:attendant:tow', 'ln:watch', 'ln:reel'; 'ln:85', 'ln:83', 'ln:81' once a
// night is tied, 'ln:q39' once the question is up).

/** ui/Panels.ts TalkLine: { who, text } (who empty: an aside). */
using FS47LostLine = TPair<FString, FString>;

/** core/reelTape.ts ReelTape, as this asks of it (its own port: Earl's deck). */
struct FS47LostTape
{
	TFunction<bool()> Busy;
	/** A cue as the tape plays: 'start', 'boy' (with the text), 'hole' (the i-th), 'click'. */
	struct FCue { FString Kind; FString Text; int32 I = 0; };
	TFunction<void(TFunction<void(const FCue&)>, TFunction<void(bool)>)> Play;
	TFunction<void()> Stop;
};

struct FS47LostNightsDeps
{
	/** The town's part of the case. */
	FS47TownSeen* Seen = nullptr;
	/** Where Night Shift keeps the threads (NightShiftState.ln). */
	TFunction<TArray<FString>&()> Links;
	/** Chapter two has begun (the book is on the board from then, once there is something for it). */
	TFunction<bool()> Open;
	/** The exceptions binder has been read (it is in the journal). */
	TFunction<bool()> Binder;
	TFunction<TArray<FString>()> Notes;
	TFunction<void(const FString&)> Note;
	TFunction<double()> Clock;
	TFunction<void(const FS47Doc&)> File;
	TFunction<void()> Save;
	TFunction<void()> Refresh;
	FS47LostTape Tape;
	/** Nothing else of the night's is going on now (one thing at a time). */
	TFunction<bool()> Quiet;
	/** The other voices stop before the reel (the narrators, a call, the CB). */
	TFunction<void()> HushVoices;
	/** The boy has called the Night Line tonight (core/radio.ts, Radio.boyCalled). */
	TFunction<void()> BoyCalled;
};

class SIGNAL47_API FS47LostNights
{
public:
	explicit FS47LostNights(FS47LostNightsDeps InD);

	TUniquePtr<FS47RuleBook> Book;

	/** The carbon of the tow ticket, from the attendant's spike by the register (caliche.ts). */
	static FS47Doc TowSlip();
	/** Reyes's notebook copy of the reel, once it has been heard to the end. */
	static FS47Doc ReelPaper(bool bHeard);

	/** The book's part of the board, from chapter two on: once the binder has been read, or once
	 *  somebody in Caliche has given a date. */
	TArray<FS47RuleBook*> Books();

	// ---------- Earl's reel (caliche.ts calls these from the studio door) ----------
	/** The date can be given to Earl now: the board has the 1981 night, and nothing else is on. */
	bool ReelReady() const;
	bool ReelBusy() const { return D.Tape.Busy && D.Tape.Busy(); }
	/** The reel, as lines under the conversation: the tape as it plays, then a page of Earl's. */
	void PlayReel(TFunction<void(const TArray<FS47LostLine>&)> Lines, TFunction<void(bool)> Done);
	void StopReel() { if (D.Tape.Stop) D.Tape.Stop(); }

	// ---------- the driver at the counter in the Mesa Diner (Chapter5.ts driverMore) ----------
	/** A closer look at him, any time the diner is there. */
	FString DriverExamine();
	/** When he is spoken to (chapter five): the watch, the first time. */
	TArray<FS47LostLine> DriverOpen();
	struct FChoice { FString Id, Label; };
	TArray<FChoice> DriverChoices(bool bOctober) const;
	TOptional<TArray<FS47LostLine>> DriverChoose(const FString& Id);

	/** After a case is opened: what the reel left, back in Papers. */
	void Restore();

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47LostNightsDeps D;
	/** The question has just come onto the cork with the thread just tied (its reply says so). */
	bool bJustAsked = false;
	TMap<FString, FS47BookNote> NotesT;
	FString NoteText(const FString& Id) const;
	FString Say(const FString& Note);
	TSet<FString> Nights() const;
	bool Trio() const;
	void Sync();
	FString Answer() const;
	TArray<FS47BoardCard> Cards() const;
	void SeeWatch();
	void Played(bool bHeard);
	bool Has(const FString& K) const;
};
