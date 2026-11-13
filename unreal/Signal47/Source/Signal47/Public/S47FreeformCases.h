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
 // Repeat: Tom, 13 November: the repeat-in-the-same-window test is gone ("too poor for gameplay"). Its slot stays in the data so that
 // older saves keep their columns; it is never offered, never planned and never named (Offered), and a result kept in an older save
 // still counts as the evidence it was.
 enum class ECondition : uint8 { Generator, OffSource, Repeat, DoorsSealed, RelaySwap, FieldWork };
 // What the recording shows: the extra pulse group, or the receiver clock offset.
 enum class EObservable : uint8 { ExtraTicks, ClockDelta, FieldTrace };
 static constexpr int32 TestCount=3, CauseCount=3, CaseCount=9, MaxLeads=32;
 static constexpr int32 MysteryLine=3, LostLine=4, ClueCount=2, CallOutCharge=10;
 // M7, weeks three and four (S47PredecessorLate.cpp finds their traces out in the world): somebody in Caliche gone from their post
 // (FS47Townsfolk), the Sky Watch's printed minute (FS47Visitors, the cairn), and the ground the military raked (FS47Visitors).
 static constexpr int32 PostLine=5, MinuteLine=6, RakedLine=7, LateFirst=5;
 // LOOP-WHOLE-20261109, step 2: OUT ON THE LAND, split from the visitors' case (2): what the night leaves out on the land (FS47Happenings),
 // things moved at a place, the trip camera, a car on the road. Last, so that no older save is renumbered (Load moves its leads over).
 // An assessment whose controls use kit the player already has; its records may disagree, and then they rule nothing out (Excluded).
 // LandLeads: the land gathers a record most nights, so it keeps more; past any line's cap the oldest fold into a count that the
 // case board shows (FoldedLeads), never dropped without a word.
 static constexpr int32 LandLine=8, LandLeads=96, LandCamera=0, LandNeedle=1, LandLogger=2;
 static constexpr double OffSourceDegrees=.6, ClockOffset=3.2;
 // CASETEST-20261109: a field control's window (the visitors' gate and field post, the land's logger tape) still counts with up to
 // twenty minutes of it unwatched in all: a call, a drive, a short wait. More than that and it records nothing.
 static constexpr double UnwatchedAllowance=20*60.;
 static bool WindowKept(double Span,double Watched,double Unwatched){return Span>0&&Unwatched<=UnwatchedAllowance+.001&&Watched>=Span-UnwatchedAllowance-.01;}
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
  // D48-20261110 (D46-16): the explanations a matched reference sound ruled out (bit per cause) and the matches tried so far.
  int32 RefRuled=0,RefTries=0;
 // CASES-OPS-20261109: the first original's own day and clock (SetReference), or, for a case opened on a trace or an observation,
 // the day and clock it opened; the case board's SOURCE line. Saved only when it differs from the opening.
 int32 RefDay=0;double RefClock=0;
  FString PlannedJob;
  FResult Results[TestCount];
  TArray<FString> Leads; // retained observation ids, first-hand facts only
  int32 FoldedLeads=0; // leads past the cap, oldest first: counted and said on the board, each still a record in the notebook
  int32 LastLeadDay=0; // the newest lead's day: the land's trail goes cold from it, not from the opening (TrailCold)
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
 static int32 LeadCap(int32 Line){return Line==LandLine?LandLeads:MaxLeads;}
 // CASES-OPS-20261109: a finding the case file keeps for itself ("case1-test2", "case4-conclusion", "case4-reply"): a record of the
 // station's own business, not of anything seen. It goes in no field record, on no pin board card and under no SEEN AND HEARD.
 static bool IsCaseRecord(const FString& Key);
 // The first original's own clock (the reel deck's case), for the SOURCE line. Only while no test has a result.
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
 // D48-20261110 (D46-16, the receiver's known sounds): the player says the take sounds like a reference. FeederCause is the line's
 // explanation that reference stands for (-1: a natural sound or none). Only the first case (the repeated interval, extra ticks) takes
 // it. A correct match (the reference is the hidden cause, a man-made one) rules out the explanation that follows the sky; an unusual
 // take (the sky) matches nothing. At most RefTriesMax (two) matches a case, so that trying every sound is not a test.
 enum class EMatch : uint8 { Closed, NoMatch, Matched };
 static constexpr int32 RefTriesMax=2, SkyCause=2;
 EMatch MarkReference(int32 Line,int32 FeederCause);
 int32 RefTriesLeft(int32 Line) const;
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
 // Which reading a place shows: one place tells it straight, the other agrees or leans elsewhere (CHAIN-3-20261109). From the seed.
 int32 ClueReading(int32 Line,int32 Clue) const;
 FString ClueText(int32 Line,int32 Clue) const;
 bool TrailCold(int32 Line,int32 Day) const;
 bool ReplyDue(int32 Line,int32 Day) const;
 const FReplyDef* Reply(int32 Line) const; // null until an assessment is filed
 void MarkReplied(int32 Line);
 FString ConclusionText(int32 Line) const;
 static bool PresentFor(int32 Line,int32 Cause,int32 Test);
 // Empty when the current world implements this test; saved legacy results remain valid.
 static FString UnavailableReason(int32 Line,int32 Test,bool bSecondLogger=false);
 // False for a test withdrawn from the game (the repeat, Tom 13 November): not on the board, in no plan, sheet or note. A result
 // an older save kept for it is still listed where results are.
 static bool Offered(int32 Line,int32 Test);
 static FString Prediction(int32 Line,int32 Test);
 static FString PredictionFor(int32 Line,int32 Test,int32 Cause); // D48-20261110: one explanation's word for one test, as Prediction joins them
 static FString ResultText(int32 Line,bool bPresent);
 static FString TestResultText(int32 Line,int32 Test,bool bPresent);
 static FString Clock(double DayClock);
 // LOOP-WHOLE-20261109: the save loaded had fewer lines than now (from before OUT ON THE LAND): the session gives the land the records
 // it already holds (FS47FreeformSession::Begin). Not saved.
 bool FromBeforeLand() const { return bBeforeLand; }
 static bool SelfTest(TArray<FString>& Lines);
private:
 TArray<FCase> Cases;
 TSet<FString> Access;
 int32 CaseSeed=0;
 bool bBeforeLand=false;
 static int32 Seeded(int32 Seed,int32 Line);
 // The land: what the trip camera's own frames show is a world record, not drawn from the hidden cause; the cause follows the records
 // kept (to the next cause they all agree with, from the seed's own), and stays put when they disagree.
 void FollowLand();
 // A save from before the land had its own case (eight lines): the visitors' leads that belong out on the land move to it.
 void MoveLandLeads();
};
