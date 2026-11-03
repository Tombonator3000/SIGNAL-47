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
 static int32 ScheduledStage(double Clock, int32 Chapter, bool bHandoverDone);
 static FVector MapPosition(int32 Stage, int32 Light, double Seconds);
};

// A separate Freeform opportunity: one seeded draw per workday, never a replay
// queue. Absolute session time makes unseen passages and sleep expire naturally.
struct FS47FreeformSkyState
{
 int32 Day=0,Pattern=0,Seed=4701986;
 double Start=0,Elapsed=0,Looking=0;
 bool bChosen=false;
 TArray<FString> ObservedPlaces;
 static constexpr double Duration=240.;
 void Begin(const S47J::FObj& Saved,int32 NewSeed);
 bool Advance(int32 CurrentDay,double Clock);
 bool Active() const { return bChosen&&Elapsed>=0&&Elapsed<Duration; }
 int32 Count() const { return Pattern==2&&Elapsed>100?2:1; }
 FVector Position(int32 Light) const;
 S47J::FObj Save() const;
};

class FS47SkyVisitors
{
public:
 explicit FS47SkyVisitors(US47Main* InM);
 ~FS47SkyVisitors();
 void Begin(const S47J::FObj& Saved = nullptr, bool bNewNight = false);
 void Step(double Dt);
 void BeginFreeform(const S47J::FObj& Saved);
 void StopFreeform();
 S47J::FObj SaveFreeform() const { return FreeformState.Save(); }
 S47J::FObj Save() const { return State.Save(); }
 const FS47SkyVisitorState& Data() const { return State; }
 static FVector WorldPosition(const FVector& MapPoint, const FString& Area);
 // Export meshes use a dedicated visibility trace. Transparent glass and hidden
 // area copies do not conceal a light; an opaque roof, wall or mesa does.
 bool ClearLine(const FVector& Eye, const FVector& At) const;
 static bool SelfTest(TArray<FString>& Lines);
 // F10 is an ephemeral visual preview, separate from the CLI natural-state contract.
 // Repeating a stage restarts only the preview; zero returns to the natural sky.
 void DevStage(int32 Stage);
private:
 US47Main* M = nullptr;
 FS47SkyVisitorState State;
 FS47FreeformSkyState FreeformState;
 bool bFreeform=false;
 void StepFreeform(double Dt);
 FS47SkyVisitorState Preview;
 FString PreviewArea;
 TArray<TWeakObjectPtr<UMaterialBillboardComponent>> Points;
 bool bBuilt = false;
 double ObservationStep = 0;
 int32 TestStage = 0;
 bool bContract = false, bContractDone = false;
 int32 ContractPhase = 0, ContractChecks = 0;
 double ContractTime = 0, ContractSince = 0, HeldTime = 0;
 FString ContractRoot;
 void Build();
 const FS47SkyVisitorState& ActiveState() const { return Preview.bEnabled ? Preview : State; }
 void HidePoints();
 void ClearPreview();
 void Hide();
 int32 RequestedStage() const;
 void RecordObservation(int32 Visible);
 void ControlTest(double Dt);
 bool Check(bool Good, const FString& What);
};
