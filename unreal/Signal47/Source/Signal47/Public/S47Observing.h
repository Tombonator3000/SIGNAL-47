#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Rx.h"
class US47Main;
class FS47RecordedDialogue;
class FS47SaroWork;
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
 // New Story cases inspect the pulsar's extra ticks before proceeding to the archive.
 enum class ELoopStage : int8 { Off = -1, AwaitPulsar, Rack, ControlRunning, ControlPrint, Compare, Filed };
 struct FRun
 {
  const TCHAR* Id; const TCHAR* Source; const TCHAR* Task; const TCHAR* Data;
  double Slot, Minutes, Seconds, El; // night clock, game minutes the run takes, real seconds integrating, elevation
  FS47Profile P;
 };
 static const TArray<FRun>& Runs();
 static constexpr int32 Version = 1;

 explicit FS47Observing(US47Main* InM);
 ~FS47Observing();
 void Begin(const S47J::FObj& Saved, bool bNewCase);
 void Stop();
 void SetCharacterVoicesEnabled(bool bValue);
 bool DoorVoicePlaying() const;
 // SC04: the same door fragment again from the fire exit, word for word (no state, no hint).
 void ReplayDoorFragment(const FVector& At); // three.js metres, where the voice is heard
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
 ELoopStage LoopStage() const { return PulsarLoop; }
 double ControlElapsed() const { return ControlT; }
 const FString& LoopFeedback() const { return CompareFeedback; }
 bool ComparePulsar(const FString& Choice);
 // Unreal addition: the same physical stations now share a saved evening work order.
 bool WorkEnabled() const { return bWorkEnabled; }
 FS47SaroWork* Work() const { return ArrayWork.Get(); }
 bool PointScheduledRun();
 void OpenTracking();
 void OpenArrayStatus();
 void SelectStatusDish(int32 Index);
 int32 SelectedStatusDish() const { return SelectedDish; }
 bool AudioAction(const FString& Action);
 void SeekAudio(double Seconds);
 bool AudioReviewed() const { return bAudioReviewed; }
 bool AudioPlaying() const { return bAudioPlaying; }
 double AudioPosition() const { return AudioCursor[AudioSelected]; }
 int32 AudioSource() const { return AudioSelected; }
 double AudioMarkedStart() const { return MarkStart; }
 double AudioMarkedEnd() const { return MarkEnd; }
 FString AudioDisplay() const;
 double AudioLevel() const;
 bool PacketCollected() const { return bPacketCollected; }
 static FString WorkReference() { return TEXT("SARO-860413-02"); }
 static constexpr double ControlSeconds = 8;
 static FS47Doc PulsarControl();
 static FS47Doc PulsarComparison();
 // The chores between runs: the weekly generator test before the pulsar (0 none, 1 running,
 // 2 on generator power, 3 logged), the 20:30 filing in the records room before the hydrogen strip
 // (-1 legacy exemption, 0 not filed, 1 filed, 2 the reel turned) and the round to the array gate before Virgo A (0, 1 chained, 2 found open).
 int32 GeneratorStep() const { return GenStep; }
 int32 ArchiveStep() const { return Archive; }
 int32 YardStep() const { return Yard; }
 // After the last run: Dale's voice outside the visitor entrance (1 heard, 2 door opened on nobody).
 int32 DoorVoiceStep() const { return DoorVoice; }
 bool ChorePending() const;
 static FVector PanelPoint() { return FVector(5.93, 1.35, .65); }
 static FVector GatePoint() { return FVector(-1.0, 0, -20.0); }
 static FVector BinderPoint() { return FVector(-2.32, 1.0, 10.2); }
 static FS47Doc RunBinder();
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
 ELoopStage PulsarLoop = ELoopStage::Off;
 double ControlT = 0, ControlTick = 0;
 FString CompareFeedback;
 bool bWorkEnabled = false, bWorkOrderFiled = false, bAudioPanelOpen = false, bTelemetryPanelOpen = false, bWorkActive = false;
 bool bAudioPlaying = false, bAudioReviewed = false, bPacketCollected = false;
 int32 AudioSelected = 0, SelectedDish = 0;
 double AudioCursor[2] = {0, 0}, MarkStart = -1, MarkEnd = -1;
 double AudioCovered[2] = {0, 0};
 uint32 AudioCoverage[2] = {0, 0};
 FString AudioFeedback;
 TSharedPtr<FS47SaroWork> ArrayWork;
 void OpenAudioComparison();
 void StepAudio(double Dt);
 void CreditAudioRange(int32 Source, double Was, double Now);
 FString DishStatus(int32 Index) const;
 void CollectPacket();
 bool AudioAvailable() const;
 bool AudioCoverageComplete() const;
 void OpenWorkOrder();
 static FS47Doc WorkOrder();
 bool PulsarCheckActive() const;
 void SecurePulsarChart();
 void StartPulsarControl();
 void StepPulsarControl(double Dt);
 void CollectPulsarControl();
 void OpenPulsarComparison();
 FString PulsarObjective() const;
 int32 Run = 0, HintMask = 0, Ticks = 0, GenStep = 0, Archive = 0, Yard = 0, DoorVoice = 0;
 TSharedPtr<FS47RecordedDialogue> Voice;
 bool bReplay = false; // SC04's fragment owns the voice until it ends
 FVector ReplayAt = FVector::ZeroVector;
 void StepDoorVoice();
 double GenT = 0, FlickerT = -1;
 TArray<bool> HeldTubes;
 bool bPanelBuilt = false;
 void BuildPanel();
 void UseGenerator();
 void UseBinder();
 void StepChores(double Dt);
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
