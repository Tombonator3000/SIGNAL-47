#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Rx.h"
class US47Main;
struct FS47Task;
struct FS47Doc;

// Unreal addition (Tom 28.10.2026): the evening's ordinary array work between the receiver
// reference and the 23:00 north-window round. Four scheduled observations of real 1986 sources
// visible east of the meridian from the site, each tuned on the existing console, integrated
// and reduced on paper. Small unexplained things ride along in the data; nothing is seen.
// New evening cases only. Older saves never gain a retroactive schedule.
class FS47Observing
{
public:
 enum class EStep : uint8 { Tune, Integrating, Data };
 struct FRun
 {
  const TCHAR* Id; const TCHAR* Source; const TCHAR* Task; const TCHAR* Data;
  double Slot, Minutes, Seconds, El; // night clock, game minutes the run takes, real seconds integrating, elevation
  FS47Profile P;
 };
 static const TArray<FRun>& Runs();
 static constexpr int32 Version = 1;

 explicit FS47Observing(US47Main* InM) : M(InM) {}
 void Begin(const S47J::FObj& Saved, bool bNewCase);
 void Stop() { bEnabled = false; }
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Enabled() const { return bEnabled; }
 // The schedule is open: after the receiver reference, before every run is filed.
 bool Active() const;
 bool Pending() const { return bEnabled && Run < Runs().Num(); }
 bool Done() const { return bEnabled && Run >= Runs().Num(); }
 int32 RunIndex() const { return Run; }
 EStep Phase() const { return StepNow; }
 double Integrated() const { return Elapsed; }
 int32 Hints() const { return HintMask; }
 // The console's card, action and window while a run waits to be tuned.
 const FS47Profile* ConsoleProfile() const;
 bool ConsoleAction();
 FString Label(const FString& Id) const;
 bool Use(const FString& Id);
 FString Objective() const;
 TArray<FS47Task> Tasks() const;
 static FS47Doc Schedule();
 static FS47Doc Chart(int32 Index);
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M = nullptr;
 bool bEnabled = false, bScheduleFiled = false;
 int32 Run = 0, HintMask = 0, Ticks = 0;
 EStep StepNow = EStep::Tune;
 double Elapsed = 0, Tick = 0;
 FString HeldStatus;
 bool AtConsoleStage() const;
 void Start();
 void Finish();
 void File();
 void Hint(int32 Bit);
 void Changed();
};
