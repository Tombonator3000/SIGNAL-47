#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47SC04.h"
class US47Main;
class UStaticMeshComponent;
class UTextRenderComponent;
struct FS47Task;
struct FS47Doc;

// Unreal addition (Tom 29.10.2026, Docs/Design/Story/TIDSLINJE_SC01_SC13_2026-10-29.md): the night's
// spine from SC01 to SC13 for new evening cases. Scenarios that exist are owned by their systems
// (Horror SC01 to SC03, the legacy prologue as SC05, chapters one to six as SC06, SC07, SC09, SC10,
// SC12 and SC13); the spine only reads them. The three that are not built yet (SC04, SC08, SC11)
// are marked placeholders at their place in the world: a labelled post, a card that says what will
// happen, one action and the clock jump to the next. Older saves never gain the spine.
class FS47Spine
{
public:
 enum EGate : uint8 { SC04, SC08, SC11, GateCount };
 struct FScenario { const TCHAR* Id; const TCHAR* Title; const TCHAR* Window; const TCHAR* Place; };
 static const TArray<FScenario>& Scenarios();
 static constexpr int32 Version = 1;
 // Clock at each placeholder's end, on the signed night axis (Monday begins at zero).
 static double GateDone(EGate G);
 static FVector GatePoint(EGate G);
 static FVector StandPoint(EGate G);
 static const TCHAR* GateItem(EGate G);

 explicit FS47Spine(US47Main* InM) : M(InM), Act04(InM) {}
 // SC04 is built (S47SC04.h); its gate is held while it runs instead of a placeholder card.
 const FS47SC04& Secure() const { return Act04; }
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
 // -S47SpineContract=sc08 (with -S47Jump=chapter2) or =sc11 (with -S47Jump=chapter4), -S47EveningTimeline.
 void ControlTest(double Dt);
private:
 US47Main* M = nullptr;
 bool bEnabled = false, bTestSC03 = false;
 int32 State[GateCount] = { 0, 0, 0 };
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Posts;
 TArray<TWeakObjectPtr<UTextRenderComponent>> Signs;
 bool bBuilt = false;
 FS47SC04 Act04;
 struct FTest { FString Mode; int32 Stage = 0, Checks = 0, Item = 0, Cand = 0, Hop = 0; double T = 0, Since = 0, Mark = 0, ClockMark = 0; bool bDone = false, bInit = false; TArray<FVector2D> Route; FString Miss; };
 // -S47SpineContract=sc04 (capture, then the lure: the short way), sc04hide (the shut records door) or sc04long
 // (out by the fire exit, round the north side, in by the service door), with -S47Jump=residual.
 void SC04Test(double Dt, TFunctionRef<void(const FString&)> Fail, TFunctionRef<bool(bool, const FString&)> Check);
 FTest Test;
 void Build();
 void ShowPosts();
 void Finish(EGate G);
};
