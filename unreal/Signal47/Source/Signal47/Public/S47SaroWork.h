#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
class US47Main;

// Unreal Story extension: real pointing, a five-point cross and the S-03 evening service.
// Owned by Observing; it releases all array holds before later story scenes take over.
class FS47SaroWork
{
public:
 enum class EService : uint8 { NotDue, Fault, Local, OpenLoose, OpenSeated, Closed, TestingOut, TestingBack, Tested, Complete };
 enum class EMotion : uint8 { None, Slewing, Ready, Scanning, Complete };
 explicit FS47SaroWork(US47Main* InM) : M(InM) {}
 ~FS47SaroWork();
 void Begin(const S47J::FObj& Saved, bool bEnable);
 void Stop();
 bool Enabled() const { return bEnabled; }
 S47J::FObj Save() const;
 void Step(double Dt, bool bPause);
 void SetContext(int32 Run, bool bServiceDue);
 bool CommandObservation(int32 Run, double Az, double El, bool bCross);
 bool HasCommand() const { return Motion != EMotion::None; }
 int32 CommandedRun() const { return CommandRun; }
 bool Ready() const { return bEnabled && !ServiceRequired() && (Motion == EMotion::Ready || Motion == EMotion::Complete) && OnTarget(); }
 bool CanMeasure() const;
 void BeginScan();
 bool ScanComplete() const { return Motion == EMotion::Complete; }
 bool ServiceRequired() const { return bEnabled && Service != EService::NotDue && Service != EService::Complete; }
 bool ServiceComplete() const { return Service == EService::Complete; }
 EService ServiceStage() const { return Service; }
 EMotion MotionStage() const { return Motion; }
 int32 CrossPoint() const { return CrossIndex; }
 bool Use(const FString& Id);
 FString Label(const FString& Id) const;
 FString Objective() const;
 FString StateText() const;
 TArray<FString> StatusRows() const;
 FVector ServicePoint() const;
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M = nullptr;
 TSharedPtr<uint8> Life = MakeShared<uint8>(0);
 bool bEnabled = false, bBuilt = false, bRestore = false, bMotorSound = false, bCross = false;
 int32 ContextRun = 0, CommandRun = -1, CrossIndex = 0;
 EService Service = EService::NotDue;
 EMotion Motion = EMotion::None;
 double TargetAz = 0, TargetEl = 0, Settle = 0, Sample = 0, ServiceAz = 0, ServiceEl = 0;
 TArray<double> SavedAngles;
 TArray<FString> Commands;
 FS47Obj Panel, Lid, Connector, Selector;
 int32 S03() const;
 bool OnTarget(bool bOnlyS03 = false) const;
 void ApplyRestore();
 void PointCross();
 void BuildPanel();
 void DrawPanel();
 void Changed(const FString& Text = FString());
 void Sound(bool bMoving);
};
