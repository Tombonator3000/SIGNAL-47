#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"

// Pure Freeform case logic (Docs/Design/Gameplay/FREEFORM_CASE_LOOP.md).
// Each case has one hidden, seeded cause. Player-chosen physical tests exclude
// explanations; a conclusion is accepted only when the tests support it.
// No world access, global RNG or deadlines.
class FS47FreeformCases
{
public:
 enum class ETest : uint8 { Generator, OffSource, Repeat };
 enum class ECause : uint8 { Feeder, LocalTransmitter, Sky };
 static constexpr int32 TestCount=3, CauseCount=3, CaseCount=1;
 static constexpr double OffSourceDegrees=.6, RepeatWindow=90*60.;
 struct FResult { bool bDone=false,bTicks=false; int32 Day=0; };
 struct FCase
 {
  bool bOpen=false,bConcluded=false;
  int32 OpenDay=0,Planned=-1,Conclusion=-1,ValidTests=0;
  double OpenClock=0; // seconds into the day of the retained original
  ECause Truth=ECause::Feeder;
  FString PlannedJob;
  FResult Results[TestCount];
 };
 void Reset(int32 Seed);
 void Load(const S47J::FObj& Saved,int32 Seed);
 S47J::FObj Save() const;
 bool Open(int32 Line,int32 Day,double DayClock);
 const FCase* Get(int32 Line) const { return Cases.IsValidIndex(Line)?&Cases[Line]:nullptr; }
 bool HasAccess(const FString& Id) const { return Access.Contains(Id); }
 bool Plan(int32 Line,ETest Test,const FString& JobId);
 void CancelPlan(int32 Line);
 // Records a finished test. Returns false if it was not the planned test.
 bool Record(int32 Line,ETest Test,int32 Day,bool bValid,bool bTicks,bool& bFirstValid);
 bool Excluded(int32 Line,ECause Cause) const;
 int32 Remaining(int32 Line) const;
 bool Conclude(int32 Line,ECause Cause,FString& Why);
 bool InRepeatWindow(int32 Line,int32 Day,double DayClock) const;
 static bool TicksFor(ECause Cause,ETest Test);
 static FString Title(int32 Line);
 static FString Question(int32 Line);
 static FString CauseName(ECause Cause);
 static FString TestName(ETest Test);
 static FString Prediction(ETest Test);
 static FString Instruction(ETest Test);
 static FString RecordsTelex(ECause Cause);
 static FString Clock(double DayClock);
 static bool SelfTest(TArray<FString>& Lines);
private:
 TArray<FCase> Cases;
 TSet<FString> Access;
 int32 SeedValue=0;
 static ECause Seeded(int32 Seed,int32 Line);
};
