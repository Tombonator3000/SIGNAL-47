#pragma once

#include "CoreMinimal.h"

// story/boardRules.ts and the data of ui/Board.ts: the rules of the case board, one book per
// chapter. A chapter writes down, as data, what it puts on the board (cards, and the lines on
// them that matter), its questions, and which threads hold: a rule ties two pins, and what they
// show together is a note. A note that belongs to a question goes into its slots by itself;
// when they are all filled, RECORD registers the finding with the chapter's own function. A rule
// marked wrong contradicts: the thread falls slack, the reply says why, and it counts against
// the question like a wrong answer did.

struct FS47Result { bool bOk = false; FString Text; };

// ---------- ui/Board.ts ----------
struct FS47BoardLine { FString Id, Text, Icon; };
struct FS47BoardCard
{
	FString Id, Stamp, Title;
	FString Kind;                 // 'paper' | 'photo' | 'map' | 'sealed' | 'finding' | 'thing' ('' none)
	bool bHave = false;
	FString Where;                // not collected yet: where it is
	FString Image;                // a data URL or an art name
	FString Text, ReadingText, Full;
	TArray<FS47BoardLine> Lines;  // the lines that matter, each with a pin; none: one pin
	FString Doc;                  // the paper in the journal it stands for
	FString Region;
	TOptional<double> X, Y, W;
};
struct FS47BoardQuestion
{
	FString Id, Stamp, Text;
	int32 Slots = 0;
	TArray<FString> Filled;
	FString Done;                 // the supported finding once recorded ('' while open)
	FString Elsewhere;
	FString Region;
	double X = 0, Y = 0;
	TOptional<double> W;
};
struct FS47BoardNote { FString Id, Text; FString FromA, FromB; FString Region; bool bHand = false; };
struct FS47BoardThread { FString A, B; bool bThin = false, bAuto = false; };
struct FS47BoardRegion { FString Id, Label; double X = 0, Y = 0, W = 0, H = 0; };

// ---------- story/boardRules.ts ----------
struct FS47BookNote
{
	FString Text; FString Q; bool bHand = false;
	/** A text that changes with the night (a TypeScript getter, `get text()`): asked each time; unset: Text. */
	TFunction<FString()> TextFn;
	FString Now() const { return TextFn ? TextFn() : Text; }
};
struct FS47BookRule
{
	FString A, B;
	FString Note;
	FString Text;
	FString Wrong;                // the question it counts against when it contradicts
	FString Group;                // alternative working hypotheses: only the last thread in a group stays
	TFunction<bool()> Restore;    // whether it restores a recorded finding from a save without threads
	TFunction<FString()> TextFn;  // a text that changes with the night (`get text()`); unset: Text
	FString Now() const { return TextFn ? TextFn() : Text; }
};
struct FS47BookQuestion
{
	FString Id, Stamp, Text;
	FString Doc;
	int32 Slots = 0;              // 0: a finding made somewhere else
	TArray<FString> Hints;
	TFunction<bool()> Open;
	TFunction<FString()> Done;    // '' while not done
	TFunction<FS47Result()> Record;
	FString Elsewhere;
	double X = 0, Y = 0;
	TOptional<double> W;
	TFunction<FString()> ElsewhereFn; // `get elsewhere()`: asked each time; unset: Elsewhere
	FString ElsewhereNow() const { return ElsewhereFn ? ElsewhereFn() : Elsewhere; }
};
struct FS47BookSpec
{
	FString Key;                                   // the board region: 'ch1', 'ch2'...
	TFunction<bool(const FString&)> Owns;          // the pins this chapter put on the board
	TMap<FString, FS47BookNote> Notes;
	TArray<FS47BookRule> Rules;
	TArray<FS47BookQuestion> Questions;
	TFunction<TArray<FString>&()> Links;           // where the chapter keeps its threads ('a|b', sorted)
	TFunction<TOptional<FS47Result>(const FString& Q, const FS47BookRule& Rule)> Wrong;
	TFunction<void()> Save;
	TFunction<TArray<FS47BoardCard>()> Cards;
	TFunction<TArray<TPair<FString, FString>>()> Auto;            // threads the story itself draws
	TFunction<FString(const FString& A, const FString& B)> QuestionOf;
	FString Empty;
	TFunction<bool(const FString&)> Legacy;
};

// What a chapter needs of the board: to lay it out, and to draw it again when its state changes
// while it is open (BoardHook).
struct FS47BoardHook
{
	TFunction<void(const FString& Place, const FString& Region)> Open;
	TFunction<void()> Refresh;
};

namespace S47Board
{
	SIGNAL47_API FString Pair(const FString& A, const FString& B);
	extern SIGNAL47_API const TCHAR* SAME;
	extern SIGNAL47_API const TCHAR* EMPTY;
}

class SIGNAL47_API FS47RuleBook
{
public:
	explicit FS47RuleBook(FS47BookSpec InSpec) : O(MoveTemp(InSpec)) {}

	const FString& Key() const { return O.Key; }
	const FS47BookRule* RuleOf(const FString& K) const;
	const FS47BookQuestion* Question(const FString& Id) const;
	// A thread from a to b, if it is this chapter's business; unset when it is not.
	TOptional<FS47Result> Connect(const FString& A, const FString& B, bool bSameCard);
	TArray<FString> Filled(const FString& Q) const;
	TArray<FS47BoardThread> Threads() const;
	TArray<FS47BoardNote> Notes() const;
	TArray<FS47BoardQuestion> Questions() const;
	TOptional<FS47Result> Record(const FString& Id);
	TOptional<FString> Hint(const FString& Id);
	void Backfill();
	TArray<FS47BoardCard> Cards() const { return O.Cards ? O.Cards() : TArray<FS47BoardCard>(); }
	bool Rests(const FString& Pin) const;
	bool Owns(const FString& Node) const { return O.Owns && O.Owns(Node); }

	// for the self test
	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47BookSpec O;
	TMap<FString, int32> HintSeen, MissSince;
	FString QuestionOf(const FString& A, const FString& B) const;
	FS47Result Miss(const FString& A, const FString& B, const FS47Result& R);
};
