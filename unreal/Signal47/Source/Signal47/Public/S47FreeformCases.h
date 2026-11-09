#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"

// Pure Freeform case logic (Docs/Design/Gameplay/FREEFORM_CASE_LOOP.md).
// Each case is data: three explanations, three physical tests and what each
// explanation predicts for each test. One hidden, seeded cause per case; a
// conclusion is accepted only when the player's own tests support it.
// A line with a FMysteryDef differs in one thing: it takes an assessment, which may be filed on any evidence and may be
// wrong. Its hidden cause also decides what each place shows, and next morning's reply follows the cause, not the filing.
// No world access, global RNG or deadlines.
class FS47FreeformCases
{
public:
 // Physical precondition the session checks for a case test recording.
 // FieldWork is carried out and reported by the Freeform world (S47FreeformWorld/Director).
 enum class ECondition : uint8 { Generator, OffSource, Repeat, DoorsSealed, RelaySwap, FieldWork };
 // What the recording shows: the extra pulse group, or the receiver clock offset.
 enum class EObservable : uint8 { ExtraTicks, ClockDelta, FieldTrace };
 static constexpr int32 TestCount=3, CauseCount=3, CaseCount=8, MaxLeads=32;
 static constexpr int32 MysteryLine=3, LostLine=4, ClueCount=2, CallOutCharge=10;
 // M7, weeks three and four (S47PredecessorLate.cpp finds their traces out in the world): somebody in Caliche gone from their post
 // (FS47Townsfolk), the Sky Watch's printed minute (FS47Visitors, the cairn), and the ground the military raked (FS47Visitors).
 static constexpr int32 PostLine=5, MinuteLine=6, RakedLine=7, LateFirst=5;
 static constexpr double OffSourceDegrees=.6, RepeatWindow=90*60., ClockOffset=3.2;
 // Yes/No name what this test shows when the observable is present/absent; Access gates planning.
 struct FTestDef { const TCHAR* Name; ECondition Condition; const TCHAR* Instruction; const TCHAR* Yes=nullptr; const TCHAR* No=nullptr; const TCHAR* Access=nullptr; };
 // What a filed assessment brings on next morning. Spare and CallOut never run the player into debt; FeederFault is the
 // ordinary feeder fault, with its ordinary repair and its fuse on credit.
 enum class EReply : uint8 { None, Spare, CallOut, FeederFault };
 struct FReplyDef { const TCHAR* Text; EReply Effect=EReply::None; };
 // One place to look. Read holds what can be found there, one reading leaning to each cause. A reading is never proof of
 // the cause it leans to, and which one shows is decided by ClueReading.
 // Hint: what the case board says of a place not yet looked at (null: the board's own words for the first two mysteries).
 struct FClueDef { const TCHAR* Finding; const TCHAR* Place; const TCHAR* Read[CauseCount]; const TCHAR* Hint=nullptr; };
 struct FMysteryDef
 {
  int32 FreshDays; // controls can be planned this many days after the day it opened; filing is always open
  const TCHAR* ReplyTitle;
  FClueDef Clues[ClueCount];
  FReplyDef Replies[CauseCount][CauseCount]; // [what was filed][what it was]
  // One trace is what opened the question, and a filing wants a second, or a control: the same minute opens on one of its own; the
  // lost hours open on a taking, and what the player wakes to is not looked into either (critic round 5).
  bool bOpensOnTrace=false;
 };
 struct FCaseDef
 {
  const TCHAR* Title; const TCHAR* Question; EObservable Observable;
  const TCHAR* Causes[CauseCount]; const TCHAR* Short[CauseCount];
  FTestDef Tests[TestCount];
  bool Present[CauseCount][TestCount]; // observable still present for [cause][test]
  const TCHAR* Telex[CauseCount]; const TCHAR* TelexTitle; const TCHAR* Access;
  const FMysteryDef* Mystery=nullptr;
 };
 struct FResult { bool bDone=false,bPresent=false; int32 Day=0; };
 struct FCase
 {
  bool bOpen=false,bConcluded=false;
  int32 OpenDay=0,Planned=-1,Conclusion=-1,ValidTests=0,Truth=0,Hunch=-1; // optional player note, never evidence
  int32 ConcludeDay=0;bool bReplied=false; // mystery lines only
  bool bBeyondWind=false; // the visitors' case after a taking or a flooded room; follows from the findings, never saved
  double OpenClock=0; // seconds into the day the case opened
 // CASES-OPS-20261109: what the repeat window is measured from: the first original's own day and clock (SetReference), or, for a
 // case opened on a trace or an observation, the day and clock it opened. Saved only when it differs from the opening.
 int32 RefDay=0;double RefClock=0;
  FString PlannedJob;
  FResult Results[TestCount];
  TArray<FString> Leads; // retained observation ids, first-hand facts only
 };
 static const FCaseDef& Def(int32 Line);
 static int32 LineForFinding(const FString& FindingId);
 void Reset(int32 Seed);
 void Load(const S47J::FObj& Saved,int32 Seed);
 S47J::FObj Save() const;
 bool Open(int32 Line,int32 Day,double DayClock);
 // Adds a retained observation as a lead; opens the case on its first lead. Idempotent.
 bool AddLead(int32 Line,const FString& ObservationId,int32 Day,double DayClock);
 static int32 LineForObservation(const FString& Event);
 // CASES-OPS-20261109: a finding the case file keeps for itself ("case1-test2", "case4-conclusion", "case4-reply"): a record of the
 // station's own business, not of anything seen. It goes in no field record, on no pin board card and under no SEEN AND HEARD.
 static bool IsCaseRecord(const FString& Key);
 // The first original's own clock (the reel deck's case): the repeat window is reckoned from it. Only while no test has a result.
 void SetReference(int32 Line,int32 Day,double DayClock);
 const FCase* Get(int32 Line) const { return Cases.IsValidIndex(Line)?&Cases[Line]:nullptr; }
 bool HasAccess(const FString& Id) const { return Access.Contains(Id); }
 // Refused once one explanation is left (CASES-OPS-20261109): the case is then concluded or filed, not tested.
 bool Plan(int32 Line,int32 Test,const FString& JobId);
 bool SetHunch(int32 Line,int32 Cause);
 void CancelPlan(int32 Line);
 // Records a finished test. Returns false if it was not the planned test, or the case is already concluded.
 bool Record(int32 Line,int32 Test,int32 Day,bool bValid,bool bPresent,bool& bFirstValid);
 bool Excluded(int32 Line,int32 Cause) const;
 // Lost time or a room lit up from outside: the visitors' case can no longer be the wind. Never leaves a case with nothing.
 // CASES-OPS-20261109: the hidden cause moves off the wind only before any result is retained, or later only to a cause every
 // retained result agrees with; with none, it stays the wind, which the retained tests then leave as the one explanation.
 void RuleOutWind(int32 Line);
 FString QuestionText(int32 Line) const;
 int32 Remaining(int32 Line) const;
 int32 ValidTestsTotal() const;
 bool Conclude(int32 Line,int32 Cause,FString& Why,int32 Day=0);
 // M7: what stands behind a line, for Dr. Ward's assessment (FS47Predecessor::Count). A tested line always has its tests. A mystery
 // line, which takes a filing on any evidence, has something behind it only with a control recorded, or with a trace kept beyond
 // the one that opened it: the other place looked at too. Kept: which of the line's places are in the findings.
 bool Supported(int32 Line,const bool Kept[ClueCount]) const;
 // Which reading a place shows: one place tells it straight, the other leans elsewhere. Both follow from the seed.
 int32 ClueReading(int32 Line,int32 Clue) const;
 FString ClueText(int32 Line,int32 Clue) const;
 bool TrailCold(int32 Line,int32 Day) const;
 bool ReplyDue(int32 Line,int32 Day) const;
 const FReplyDef* Reply(int32 Line) const; // null until an assessment is filed
 void MarkReplied(int32 Line);
 FString ConclusionText(int32 Line) const;
 bool InRepeatWindow(int32 Line,int32 Day,double DayClock) const;
 static bool PresentFor(int32 Line,int32 Cause,int32 Test);
 // Empty when the current world implements this test; saved legacy results remain valid.
 static FString UnavailableReason(int32 Line,int32 Test,bool bSecondLogger=false);
 static FString Prediction(int32 Line,int32 Test);
 static FString ResultText(int32 Line,bool bPresent);
 static FString TestResultText(int32 Line,int32 Test,bool bPresent);
 static FString Clock(double DayClock);
 static bool SelfTest(TArray<FString>& Lines);
private:
 TArray<FCase> Cases;
 TSet<FString> Access;
 int32 CaseSeed=0;
 static int32 Seeded(int32 Seed,int32 Line);
};
