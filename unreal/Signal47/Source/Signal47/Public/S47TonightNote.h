#pragma once
#include "CoreMinimal.h"

// TONIGHT-NOTE-20261109 (Tom, 9 November: "Tonight-lappen i pausemenyen bør kanskje ha mer informasjon om hva man skal gjøre i
// løpet av natta"). The pause menu's TONIGHT card as the night supervisor's note: tonight's work in order, each line with where (the
// room as its sign has it) and when (a dated window), what is done ticked and struck, the next step marked; what else is open (the
// job cards, cases waiting on evidence, post not read); and what the clock allows now.
//
// Nothing of its own: it is made from what the task list already has (AS47Story::Tasks, in Freeform FS47FreeformSession::Tasks with
// FS47Predecessor, FS47Mail and FS47DayRound's TaskLine), the HUD's objective card (S47HudGauges::CardLine) and the case windows
// (FS47FreeformSession::CaseTestWindow). Nothing is saved. In Freeform every line keeps to the spoiler limits (S47Script::LintText):
// a line that does not is left off the note.
namespace S47TonightNote
{
 enum class EMark : uint8 { Open, Next, Done };
 struct FLine
 {
  EMark Mark = EMark::Open;
  FString When;  // "01:30-04:30"; empty: no window
  FString Where; // the room or the place as its sign has it, "PHOTO LAB"; empty: none said
  FString What;  // "Develop your exposed film at the wet bench."
 };
 struct FNote
 {
  TArray<FLine> Lines;  // at most MostLines, in the night's order: done, the next step, the rest, the dated windows by time
  int32 More = 0;       // open lines with no room on the note: "+N more in the notebook"
  TArray<FString> Also; // what else is open: "Job cards: ...", "Cases waiting on evidence: ...", "Post: ..."
  FString Clock;        // what the clock allows now: "Too early: the window opens at 01:30. Do a job or rest until then."
  bool IsEmpty() const { return Lines.IsEmpty() && Also.IsEmpty() && Clock.IsEmpty(); }
 };
 // A task as the list has it, and a dated window for one (Text: the task's own words; Start and End on the world clock).
 struct FItem { FString Text; bool bDone = false; };
 struct FWindow { FString Text; double Start = 0, End = 0; };
 struct FIn
 {
  TArray<FItem> Tasks;          // the task list, in its order
  FString Next;                 // the objective as the HUD's card has it ("PLACE // WHAT"), or the chapter's objective
  double Clock = -1;            // Freeform: the world clock; Story: seconds after midnight; negative: none
  bool bFreeform = false;
  TArray<FWindow> Windows;      // Freeform: the planned case tests' windows
  TArray<FString> WaitingCases; // Freeform: the titles of open cases with no test planned
  double EveningUp = -1;        // Freeform: when the next evening card's source is up (world clock); negative: none waiting
  bool bAntennasWait = false;   // Freeform: the station goal waits for daylight (FS47FreeformSession::GoalWaits)
 };
 constexpr int32 MostLines = 6, MostDone = 2, MostChars = 60;
 SIGNAL47_API FNote Build(const FIn& In);
 // The note as text: "[x] ", "[>] " or "[ ] " before each line, then the rest. For the log and the tests.
 SIGNAL47_API FString Plain(const FNote& Note);
 // One line as the card types it: "01:30-04:30  CONTROL ROOM: Return to listen under the fastened roof."
 SIGNAL47_API FString LineText(const FLine& Line);
 SIGNAL47_API FString HourMinute(double Clock);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
