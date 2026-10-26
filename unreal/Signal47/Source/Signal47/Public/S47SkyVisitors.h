#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Seen.h"

class US47Main;
class UMaterialBillboardComponent;

// Unreal addition: optional world-space overflights, independent of the comet,
// meteors, star hole and the old three lights along B. No mission or photo quota.
struct FS47SkyVisitorState
{
 bool bEnabled = false;
 int32 Stage = 0;
 double Elapsed = 0, Looking = 0;
 bool Occurred[5] = {}, Observed[5] = {}, Completed[5] = {};
 bool bSecondEarlyPass = false;
 void Begin(const S47J::FObj& Saved, bool bNewNight);
 bool Advance(int32 RequestedStage, double Dt, bool bHeld);
 bool Observe(int32 Visible, double Dt);
 bool OfferEarlyPass();
 S47J::FObj Save() const;
 static double Duration(int32 Stage);
 static int32 Count(int32 Stage);
 static int32 RequiredVisible(int32 Stage);
 static FVector MapPosition(int32 Stage, int32 Light, double Seconds);
};

class FS47SkyVisitors
{
public:
 explicit FS47SkyVisitors(US47Main* InM);
 ~FS47SkyVisitors();
 void Begin(const S47J::FObj& Saved = nullptr, bool bNewNight = false);
 void Step(double Dt);
 S47J::FObj Save() const { return State.Save(); }
 const FS47SkyVisitorState& Data() const { return State; }
 static FVector WorldPosition(const FVector& MapPoint, const FString& Area);
 // Export meshes use a dedicated visibility trace. Transparent glass and hidden
 // area copies do not conceal a light; an opaque roof, wall or mesa does.
 bool ClearLine(const FVector& Eye, const FVector& At) const;
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M = nullptr;
 FS47SkyVisitorState State;
 TArray<TWeakObjectPtr<UMaterialBillboardComponent>> Points;
 bool bBuilt = false;
 double ObservationStep = 0;
 int32 TestStage = 0;
 bool bContract = false, bContractDone = false;
 int32 ContractPhase = 0, ContractChecks = 0;
 double ContractTime = 0, ContractSince = 0, HeldTime = 0;
 FString ContractRoot;
 void Build();
 void Hide();
 int32 RequestedStage() const;
 void RecordObservation(int32 Visible);
 void ControlTest(double Dt);
 bool Check(bool Good, const FString& What);
};
