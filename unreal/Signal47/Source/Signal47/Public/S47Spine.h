#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47SC04.h"
#include "S47SC08.h"
class US47Main;
class UStaticMeshComponent;
class UTextRenderComponent;
struct FS47Task;
struct FS47Doc;

// Versioned night gates. SC04 owns the annex encounter; v2 new nights own SC08's original
// reel/reference/array route. Unqualified v1 saves retain their existing marked continuation.
// Chapter callbacks and every other scenario's owner remain authoritative.
class FS47Spine
{
public:
 enum EGate : uint8 { SC04, SC08, SC11, GateCount };
 struct FScenario { const TCHAR* Id; const TCHAR* Title; const TCHAR* Window; const TCHAR* Place; };
 static const TArray<FScenario>& Scenarios();
 static constexpr int32 Version = 2; // v1 stays legacy unless actual new SC07 custody qualifies.
 // Clock at each placeholder's end, on the signed night axis (Monday begins at zero).
 static double GateDone(EGate G);
 static FVector GatePoint(EGate G);
 static FVector StandPoint(EGate G);
 static const TCHAR* GateItem(EGate G);

 explicit FS47Spine(US47Main* InM) : M(InM), Act04(InM), Act08(InM) {}
 // SC04 is built (S47SC04.h); its gate is held while it runs instead of a placeholder card.
 const FS47SC04& Secure() const { return Act04; }
 FS47SC08* Array() { return bSC08New && State[SC08]>=1 ? &Act08 : nullptr; }
 const FS47SC08* Array() const { return bSC08New && State[SC08]>=1 ? &Act08 : nullptr; }
 bool ArrayBusy() const { return bEnabled && bSC08New && State[SC08]==1 && Act08.Busy(); }
 void Begin(const S47J::FObj& Saved, bool bNewCase);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Enabled() const { return bEnabled; }
 // 0 not reached, 1 pending (the placeholder waits), 2 done.
 int32 Gate(EGate G) const { return State[G]; }
 // The placeholder that holds the story now, or GateCount.
 EGate Pending() const;
 // Before the residual console work: SC04 holds the control room after SC03.
 bool HoldsResidual() const;
 // The chapter ends that continue only after their placeholder (S47Chapters.cpp).
 TFunction<void()> AfterSC08, AfterSC11;
 // Chapter two's and four's ends: true when the spine holds the continuation for a placeholder.
 bool HoldChapterEnd(EGate G);
 FString Label(const FString& Id) const;
 bool Use(const FString& Id);
 FString Objective() const;
 TArray<FS47Task> Tasks() const;
 // Index of the scenario the night is in (0 = SC01), for the notebook and the pause card.
 int32 Current() const;
 static FS47Doc Card(EGate G);
 static bool SelfTest(TArray<FString>& Lines);
 // Contract fixture: SC03's end without driving the whole of SC03.
 void TestOpenSC04() { bTestSC03 = true; }
 // SC08 additionally requires an actual SC07 PASS and -S47SC07Checkpoint; no predecessor fixture.
 void ControlTest(double Dt);
private:
 US47Main* M = nullptr;
 bool bEnabled = false, bTestSC03 = false, bSC08New = false;
 int32 State[GateCount] = { 0, 0, 0 };
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Posts;
 TArray<TWeakObjectPtr<UTextRenderComponent>> Signs;
 bool bBuilt = false;
 FS47SC04 Act04;
 FS47SC08 Act08;
 struct FTest { FString Mode; int32 Stage = 0, Checks = 0, Item = 0, Cand = 0, Hop = 0; double T = 0, Since = 0, Mark = 0, ClockMark = 0; bool bDone = false, bInit = false; TArray<FVector2D> Route; FString Miss; };
 FTest Test;
 void Build();
 void ShowPosts();
 void Finish(EGate G);
};
