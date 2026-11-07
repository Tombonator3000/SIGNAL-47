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
 static constexpr int32 TestCount=3, CauseCount=3, CaseCount=4, MaxLeads=32;
 static constexpr int32 MysteryLine=3, ClueCount=2, CallOutCharge=10;
 static constexpr double OffSourceDegrees=.6, RepeatWindow=90*60., ClockOffset=3.2;
 // Yes/No name what this test shows when the observable is present/absent; Access gates planning.
 struct FTestDef { const TCHAR* Name; ECondition Condition; const TCHAR* Instruction; const TCHAR* Yes=nullptr; const TCHAR* No=nullptr; const TCHAR* Access=nullptr; };
 // What a filed assessment brings on next morning. Spare and CallOut never run the player into debt; FeederFault is the
 // ordinary feeder fault, with its ordinary repair and its fuse on credit.
 enum class EReply : uint8 { None, Spare, CallOut, FeederFault };
 struct FReplyDef { const TCHAR* Text; EReply Effect=EReply::None; };
 // One place to look. Read holds what can be found there, one reading leaning to each cause. A reading is never proof of
 // the cause it leans to, and which one shows is decided by ClueReading.
 struct FClueDef { const TCHAR* Finding; const TCHAR* Place; const TCHAR* Read[CauseCount]; };
 struct FMysteryDef
 {
  int32 FreshDays; // controls can be planned this many days after the day it opened; filing is always open
  const TCHAR* ReplyTitle;
  FClueDef Clues[ClueCount];
  FReplyDef Replies[CauseCount][CauseCount]; // [what was filed][what it was]
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
  double OpenClock=0; // seconds into the day of the retained original
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
 const FCase* Get(int32 Line) const { return Cases.IsValidIndex(Line)?&Cases[Line]:nullptr; }
 bool HasAccess(const FString& Id) const { return Access.Contains(Id); }
 bool Plan(int32 Line,int32 Test,const FString& JobId);
 bool SetHunch(int32 Line,int32 Cause);
 void CancelPlan(int32 Line);
 // Records a finished test. Returns false if it was not the planned test.
 bool Record(int32 Line,int32 Test,int32 Day,bool bValid,bool bPresent,bool& bFirstValid);
 bool Excluded(int32 Line,int32 Cause) const;
 // Lost time or a room lit up from outside: the visitors' case can no longer be the wind. Never leaves a case with nothing.
 void RuleOutWind(int32 Line);
 FString QuestionText(int32 Line) const;
 int32 Remaining(int32 Line) const;
 int32 ValidTestsTotal() const;
 bool Conclude(int32 Line,int32 Cause,FString& Why,int32 Day=0);
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
 static FString UnavailableReason(int32 Line,int32 Test);
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
