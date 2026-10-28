#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"

class US47Main;
class FS47Horror;

// SC03 "The help that came" (revision 3): the report from the Caliche payphone, Mara Ortiz and
// wagon twenty-three at the fuel stop, the lift in the light with pale figures standing still at
// its edge, cover, the traces, the missing-person report and the deputy's question about the
// receiver. This class owns the world side (the wagon, Ortiz, the light, the figures, the thermos,
// the gravel marks, the deputy) and every world fact. FS47Horror owns calls, scenes and saving.
// Dialogue never produces a fact: each flag below is set by a physical event or a completed call.
class FS47HorrorSC03
{
public:
 enum class EPhase : uint8 { Off, Report, Wait, Arrival, Meet, Lift, Retreat, Calm, Deputy, Complete };
 struct FFacts
 {
  bool bDispatch = false, bAssigned = false, bIdentified = false, bObserved = false, bRetreat = false;
  bool bWagon = false, bThermos = false, bMarks = false, bMissing = false, bLocalOrder = false, bDeputyHere = false;
  FString DispatchStamp, TakenStamp, MissingStamp, OrderStamp;
 };

 explicit FS47HorrorSC03(US47Main* InM, FS47Horror* InHost);
 ~FS47HorrorSC03();

 void Begin(const S47J::FObj& Saved, bool bLive);
 void Start(); // after SC02's escape is presented: the report is owed
 void Stop();
 int32 Changes() const { return Version; }
 S47J::FObj Save() const;
 void Step(double Dt);
 bool Use(const FString& Id);

 EPhase Phase() const { return P; }
 bool Live() const { return P != EPhase::Off && P != EPhase::Complete; }
 bool Complete() const { return P == EPhase::Complete; }
 const FFacts& Facts() const { return F; }
 bool TracesChecked() const { return F.bWagon && F.bThermos && F.bMarks; }
 // The fuel stop holds the night's older events while the meeting, the lift and the retreat run.
 bool Busy() const;
 bool AtFuelStop(double Radius = 26) const;
 // The call FS47Horror should place when the payphone is used now; empty when none.
 TArray<FString> CallScenes() const;
 void CallFinished(const FString& SceneId);
 void ScenePresented(const FString& SceneId);
 FString Objective() const;
 TArray<TPair<FString, bool>> Tasks() const;
 // Ortiz's chest and the figures' heads in world (three) metres, for tests.
 FVector OrtizPoint() const;
 FVector WagonPoint() const { return Park; }
 TArray<FVector> FigureHeads() const;
 bool LightOn() const { return LightK > .05; }

 static const TCHAR* PhaseName(EPhase In);
 static bool ParsePhase(const FString& Text, EPhase& Out);
 static S47J::FObj Write(EPhase Phase, const FFacts& Facts);
 static bool Read(const S47J::FObj& O, EPhase& Phase, FFacts& Facts);
 static bool SelfTest(TArray<FString>& Lines);

private:
 struct FWorld;
 TUniquePtr<FWorld> W;
 US47Main* M = nullptr;
 FS47Horror* Host = nullptr;
 EPhase P = EPhase::Off;
 FFacts F;
 bool bBuilt = false, bBuildFailed = false;
 FVector Park = FVector::ZeroVector, Counter = FVector::ZeroVector, Approach = FVector::ZeroVector, DeputyFrom = FVector::ZeroVector, DeputyAt = FVector::ZeroVector;
 double WaitT = 0, ArriveT = 0, LiftDelay = 0, LiftT = 0, LookT = 0, HeldT = 0, Gone = 0, LightK = 0, FigureK = 0, TurnK = 0, DeputyT = 0, Clock = 0;
 bool bOrtizGone = false;
 int32 Version = 0;
 double GroundRef = 0;
 bool bGrounded = false;
 double Ground(double X, double Z) const;
 bool Build();
 void Apply();
 void SetPhase(EPhase Next);
 bool Sees(const FVector& Point, double Margin = .9) const;
 bool ClearLine(const FVector& From, const FVector& To) const;
 bool PlayerCovered() const;
 FVector Eye() const;
};
