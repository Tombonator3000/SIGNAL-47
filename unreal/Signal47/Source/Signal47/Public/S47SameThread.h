#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"
#include "S47Seen.h"

class US47CaseBoard;
class SS47Board;

// web/src/story/sameThread.ts. A thread you do not remember (FORSLAG.md, 20 October, small change 8;
// Tom: "Spenning og skummelt slår faste regler"). After each time away (story/abduction.ts; taken in
// the truck too, story/truckTaken.ts, which wakes the same way), the case board has one more red thread
// than the player tied: the same yarn, between two cards they have, and on it a scrap of index card
// with one word in pencil, in their own hand: "same". The waking already has "a line in the notebook
// in my own hand. I do not remember writing it" (abduction.ts); this is the board's.
//
// Which two: one end is the exposure the player did not make (the photograph from above, `awayN`), the
// other the first of these the board has, never used before tonight and never a pin a question still
// open rests on (it would read as a hint), never a question card: his last field card, her
// correction, S-03's motor bus, the film that kept two, the printout, T. Vega's field book, the time
// away before this one (its photograph), and, before any of them have pins, the printout or the frames
// as cards. One per time away (at most three in a night). Kept with the case (`same:N`, `same:seen`).
// The first time the board is laid out with one on it, the board opens on it and says, in Reyes's
// words, "I don't remember drawing that.", and the notebook has it.
//
// (The book's notes and rules grow with the threads; FS47RuleBook keeps its spec by value, so the book
// is made again each time they change, and OnBook tells main's wiring the new one is there.)

namespace S47Same
{
	SIGNAL47_API extern const TCHAR* SAME_WORD;
	SIGNAL47_API extern const TCHAR* SAME_SAY;
	/** The other end, in the order it is looked for. */
	SIGNAL47_API const TArray<FString>& SAME_ENDS();
}

struct FS47SameDeps
{
	FS47TownSeen* Seen = nullptr;
	/** The case board: its pins, and whether a pin is one an open question rests on. */
	TFunction<TArray<TPair<FString, FString>>()> Pins;  // { pin, card }
	TFunction<bool(const FString&)> Rests;
	/** The night's clock. */
	TFunction<double()> Clock;
	TFunction<void(const FString&)> Note;
	TFunction<void()> Saw;
	TFunction<void()> Save;
};

class SIGNAL47_API FS47SameThread
{
public:
	explicit FS47SameThread(FS47SameDeps InD);

	/** The threads tonight: the time away it came with, and its two pins. */
	struct FThread { int32 N = 0; FString A, B; };
	TArray<FThread> Threads;
	TUniquePtr<FS47RuleBook> Book;
	/** The book has been made again (its notes and rules changed). */
	TFunction<void()> OnBook;

	/** Noticed on the board. */
	bool Noticed() const { return D.Seen && D.Seen->Contains(TEXT("same:seen")); }

	/** Every step: the count of times away tonight (story/abduction.ts, data.count). A new one, once
	 *  its photograph is on the board, gets its thread. */
	void Update(int32 Count);
	/** The board has been laid out: the first time with a thread on it, it opens on the thread. */
	void Opened(const TSharedRef<SS47Board>& View, TFunction<bool()> Connected);
	/** A night begun or a case opened: the threads as the case has them; the times away it has had
	 *  have had theirs (an older case's are not tied now). */
	void Reset(int32 Count);

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47SameDeps D;
	int32 UpTo = 0;
	TArray<FS47BookRule> Rules;
	TMap<FString, FS47BookNote> Notes;
	TArray<FString> Links;

	void Tie(int32 N);
	void Add(int32 N, const FString& A, const FString& B);
	void MakeBook();
	static FString EndName(const FString& Card);
};
