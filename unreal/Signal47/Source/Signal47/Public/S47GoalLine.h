#pragma once
#include "CoreMinimal.h"

// N1110 (N1110-9, the horror study's "three goals in the book"): the top of the notebook's NOW page says, in three plain lines, what
// the player is working toward at three scales: tonight (the next job, or the one thing due), this week (the sheet that falls due,
// and the field allowance left) and the bigger question (the open case's question). Nothing of its own: it is made from what NOW
// already has (the note's next line, the weekly sheet, the case file) and nothing is saved. Freeform only. A line that does not keep to
// the spoiler limits (S47Script::LintText) is left off.
namespace S47GoalLine
{
 struct FIn
 {
  FString Next;          // the note's next step as one line (S47TonightNote::LineText), empty: nothing is due
  int32 ReviewDay = 0;   // the day the weekly sheet falls due, 0: none
  int32 SheetSoFar = 0;  // what the sheet comes to so far, dollars
  int32 AllowanceLeft = -1; // field allowance left this week, dollars, -1: not said
  FString CaseTitle;     // the open case, empty: none
  FString CaseQuestion;  // its question, as the case file words it
 };
 struct FGoals
 {
  FString Tonight, Week, Question;
  TArray<FString> Lines() const; // the three, in that order, the ones that are empty left out
 };
 // The first sentence (or two, while they fit Most), so that a long question is one plain line.
 SIGNAL47_API FString FirstSentence(const FString& Text, int32 Most = 110);
 SIGNAL47_API FGoals Make(const FIn& In);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
