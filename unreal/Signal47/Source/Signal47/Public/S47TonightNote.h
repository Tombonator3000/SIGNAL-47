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
  // PT-NOTEBOOK-20261109: what the line is, for the notebook's NOW, which shows the same lines (Gather)
  FString Key;    // the same thing on the note and on NOW, and in what is remembered done tonight
  FString Source; // the task's own words as the list has them: what FOLLOW and the details look up (US47Flow::NotebookExtras)
  uint8 Rank = 1; // D45-20261110: 0 do first, 1 tonight, 2 when you can (RankOf); Freeform's note and NOW are ordered by it
 };
 struct FNote
 {
  TArray<FLine> Lines;  // at most MostLines, in the night's order: done, the next step, the rest, the dated windows by time
  int32 More = 0;       // open lines with no room on the note: "+N more in the notebook"
  TArray<FString> Also; // what else is open: "Job cards: ...", "Cases waiting on evidence: ...", "Post: ..."
  TArray<FString> AlsoKeys; // beside each of Also: "cards", "cases" or "post"
  FString Clock;        // what the clock allows now: "Too early: the window opens at 01:30. Do a job or rest until then."
  bool IsEmpty() const { return Lines.IsEmpty() && Also.IsEmpty() && Clock.IsEmpty(); }
 };
 // A task as the list has it, and a dated window for one (Text: the task's own words; Start and End on the world clock).
 struct FItem { FString Text; bool bDone = false; };
 struct FDone;
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
  struct FDone* Done = nullptr; // Freeform: what is remembered done tonight (Tonight()); none: only what the list says now
  uint64 Game = 0;              // which game it is (the case and its seed): another one forgets what was remembered
 };
 // PT-NOTEBOOK-20261109: the lines done tonight, kept struck through until 08:00 even when the list lets them go (the feeder looked at,
 // the post read, a fault mended). In memory only: a game loaded again starts the night's memory afresh.
 struct FDone
 {
  TMap<FString, FLine> Lines; // by key, as the line last read when it was done
  TArray<FString> Order;      // the keys in the order they were done
  int32 Night = 0;            // the night they belong to (08:00 to 08:00)
  double Last = -1;           // the clock when it was last looked at: a clock gone back (a load) forgets
  uint64 Game = 0;            // the game it was remembered in (FIn::Game)
 };
 SIGNAL47_API FDone& Tonight();
 // D45-20261110 (Tom: the field guide should open on a plain list of what to do, by importance): how much a line matters, from the
 // task's own words. 0 DO FIRST: the station or the truck cannot work (the feeder, an empty tank, a dish's relay, the truck stuck or
 // flat), or its window is open now. 2 WHEN YOU CAN: the long goals, the round, the post, a case waiting for a test. 1 TONIGHT: the
 // rest, everything with a clock on it (the bearing slip, a report, WORTH A LOOK, the case windows, the photo lab, the jobs).
 SIGNAL47_API uint8 RankOf(const FString& Source, bool bWindowOpen);
 // D45-20261110: the lines of NOW the notebook has shown, so that one added since is marked NEW and the notebook opens on NOW for it.
 // In memory only, as Tonight(); another game (FIn::Game) starts afresh, with what it has at first counted as seen.
 // bPlain: the notebook's own key was pressed (no page asked for, as the map's key or a test asks for one): only then may it open on
 // NOW instead of the page used last; the notebook takes it back as it opens.
 struct FSeen { TSet<FString> Keys; uint64 Game = 0; bool bBegun = false, bPlain = false; };
 SIGNAL47_API FSeen& Seen();
 constexpr int32 MostLines = 6, MostDone = 2, MostChars = 60;
 // PT-NOTEBOOK-20261109: the night's lines, one source for the pause menu's note and the notebook's NOW. bAll: no limit (NOW): every
 // line done tonight, every open line, and More 0; otherwise the note's six lines and the rest counted in More, which NOW has all of.
 SIGNAL47_API FNote Gather(const FIn& In, bool bAll = false);
 SIGNAL47_API FNote Build(const FIn& In);
 // The note as text: "[x] ", "[>] " or "[ ] " before each line, then the rest. For the log and the tests.
 SIGNAL47_API FString Plain(const FNote& Note);
 // One line as the card types it: "01:30-04:30  CONTROL ROOM: Return to listen under the fastened roof."
 SIGNAL47_API FString LineText(const FLine& Line);
 SIGNAL47_API FString HourMinute(double Clock);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}

// PT-NOTEBOOK-20261109: a line of Freeform's NOW in the notebook (FS47NbExtras::Now), as the pause menu's note has it (S47TonightNote::Gather
// with no limit). Here, where the notebook's extras (S47UI.h) can see it.
struct FS47NbNowLine
{
 FString Group;                   // "next", "clock" (by the clock), "todo", "also", "done"
 FString Text, When, Where, What; // the line as the note writes it, and its parts
 FString Key;                     // the note's own key for it; under ALSO OPEN "cards", "cases", "post" or a work item's id
 FString Why;                     // the task's own words, said in full on the right page
 FString Item;                    // the work item with its details, hints and FOLLOW (FS47NbWorkItem::Id); empty: none
 FString Fold;                    // "cards": the job cards open under it; "cases": the cases waiting on evidence
 bool bDone = false;
 // D45-20261110: Group is now "next", "first" (DO FIRST), "tonight", "later" (WHEN YOU CAN), "done"; "clock", "todo" and "also" still
 // read as before. Rank as the note's line has it; bNew: not shown on NOW before (S47TonightNote::Seen); bSummary: a line that sums
 // up others (the job cards, the cases, the post), a dash and no box.
 uint8 Rank = 1;
 bool bNew = false, bSummary = false;
};
