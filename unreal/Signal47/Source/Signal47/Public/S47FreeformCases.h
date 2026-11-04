#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"

// Pure Freeform case logic (Docs/Design/Gameplay/FREEFORM_CASE_LOOP.md).
// Each case is data: three explanations, three physical tests and what each
// explanation predicts for each test. One hidden, seeded cause per case; a
// conclusion is accepted only when the player's own tests support it.
// No world access, global RNG or deadlines.
class FS47FreeformCases
{
public:
 // Physical precondition the session checks for a case test recording.
 // FieldWork is carried out and reported by the Freeform world (S47FreeformWorld/Director).
 enum class ECondition : uint8 { Generator, OffSource, Repeat, DoorsSealed, RelaySwap, FieldWork };
 // What the recording shows: the extra pulse group, or the receiver clock offset.
 enum class EObservable : uint8 { ExtraTicks, ClockDelta, FieldTrace };
 static constexpr int32 TestCount=3, CauseCount=3, CaseCount=3, MaxLeads=32;
 static constexpr double OffSourceDegrees=.6, RepeatWindow=90*60., ClockOffset=3.2;
 // Yes/No name what this test shows when the observable is present/absent; Access gates planning.
 struct FTestDef { const TCHAR* Name; ECondition Condition; const TCHAR* Instruction; const TCHAR* Yes=nullptr; const TCHAR* No=nullptr; const TCHAR* Access=nullptr; };
 struct FCaseDef
 {
  const TCHAR* Title; const TCHAR* Question; EObservable Observable;
  const TCHAR* Causes[CauseCount]; const TCHAR* Short[CauseCount];
  FTestDef Tests[TestCount];
  bool Present[CauseCount][TestCount]; // observable still present for [cause][test]
  const TCHAR* Telex[CauseCount]; const TCHAR* TelexTitle; const TCHAR* Access;
 };
 struct FResult { bool bDone=false,bPresent=false; int32 Day=0; int32 Hunch=-1; }; // Hunch: cause the player suspected when planning
 struct FCase
 {
  bool bOpen=false,bConcluded=false;
  int32 PlannedHunch=-1;
  int32 OpenDay=0,Planned=-1,Conclusion=-1,ValidTests=0,Truth=0;
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
 bool Plan(int32 Line,int32 Test,const FString& JobId,int32 Hunch=-1);
 // "You suspected X; it predicted Y; the result Z kept / ruled out your hunch." Empty without a hunch.
 static FString HunchText(int32 Line,int32 Test,int32 Hunch,bool bPresent);
 void CancelPlan(int32 Line);
 // Records a finished test. Returns false if it was not the planned test.
 bool Record(int32 Line,int32 Test,int32 Day,bool bValid,bool bPresent,bool& bFirstValid);
 bool Excluded(int32 Line,int32 Cause) const;
 int32 Remaining(int32 Line) const;
 int32 ValidTestsTotal() const;
 bool Conclude(int32 Line,int32 Cause,FString& Why);
 bool InRepeatWindow(int32 Line,int32 Day,double DayClock) const;
 static bool PresentFor(int32 Line,int32 Cause,int32 Test);
 static FString Prediction(int32 Line,int32 Test);
 static FString ResultText(int32 Line,bool bPresent);
 static FString TestResultText(int32 Line,int32 Test,bool bPresent);
 static FString Clock(double DayClock);
 static bool SelfTest(TArray<FString>& Lines);
private:
 TArray<FCase> Cases;
 TSet<FString> Access;
 static int32 Seeded(int32 Seed,int32 Line);
};
