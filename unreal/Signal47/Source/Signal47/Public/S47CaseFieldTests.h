#pragma once
#include "CoreMinimal.h"
#include "S47Obj.h"
#include "S47Json.h"
class US47Main;
class USceneComponent;

// Physical Visitors controls. Field results need actual world ticks and observations;
// changing the clock never creates coverage, a crossing mark or a negative observation.
class FS47CaseFieldTests
{
public:
 explicit FS47CaseFieldTests(US47Main* InM):M(InM){}
 ~FS47CaseFieldTests();
 static constexpr int32 Line=2;
 void Load(const S47J::FObj& Saved);
 S47J::FObj Save() const;
 void Stop();
 void ResetPlan(); // Call on a newly selected/cancelled field plan, even if its job ID repeats.
 void Step(double Dt);
 void AdvanceUnobservedTo(double Clock); // Call before saving a rest/wait jump.
 bool Window(double& Start,double& End) const;
 bool Prepared() const;
 FString Objective() const;
 bool OwnsRoofWindow() const;
 bool Prepare(int32 Test); // Same guarded action as the physical ladder/strip interaction.
 bool InspectGate();
 static void PeakAfter(double Prepared,double& Start,double& End);
 static bool ComparableWind(double Reference,const FVector2D& Direction,double Strength,const FVector2D& Current);
 static bool SelfTest(TArray<FString>& Lines);
private:
 struct FRun
 {
  FString Job;
  int32 Test=-1,Impacts=0;
  double Prepared=-1,Start=0,End=0,LastClock=-1,Listened=0,Coverage=0,Gaps=0;
  double WindReference=0;
  FVector2D WindDirection=FVector2D(1,0),LastPlayer=FVector2D::ZeroVector;
  bool bGateCrossed=false,bGateTampered=false,bHavePlayer=false;
 } R;
 US47Main* M=nullptr;
 bool bBuilt=false,bActive=false;
 TSharedPtr<uint8> Life=MakeShared<uint8>(0);
 TArray<FS47Obj> Objects,LadderParts;
 TMap<int32,TWeakObjectPtr<USceneComponent>> Components;
 FS47Obj Ladder,Plate,Seal,SealBroken,Strip,StripBack,Ribbon,Tracks[6];
 bool SceneIntact() const;
 void ForgetScene();
 bool Build();
 void Present();
 void SyncPlan();
 void MonitorGateIntegrity();
 int32 PlannedTest() const;
 FString PlannedJob() const;
 bool Near(const FVector& At,double Range) const;
 void Changed(const FString& Message=FString());
 static double Overlap(double From,double To,double Start,double End);
};
