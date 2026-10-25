#pragma once
#include "CoreMinimal.h"
#include "S47EveningPack.h"
#include "S47Json.h"
class US47Main;
class FS47EveningVisual;
class FS47Takeaway;
class FS47MediaSource;
class FS47GainNode;
class FS47RecordedDialogue;
struct FS47Task;

// Runtime host for the authored pack. Later chapter work supplies real world result gates.
class FS47Evening
{
public:
 explicit FS47Evening(US47Main* InM);
 ~FS47Evening();
 void Begin(const S47J::FObj& Saved = nullptr, bool bPastOpening = false);
 void Stop();
 void Step(double Dt);
 bool Opening() const { return bEnabled && !Pack.Flag(TEXT("handover_done")); }
 bool Commute() const { return Opening() && !Pack.Flag(TEXT("intro_arrived")); }
 bool Enabled() const { return bEnabled; }
 bool MealBusy() const;
 // New nights complete the local inspection before launching the unattended survey.
 // Existing saves and developer chapter starts never gain a retroactive assignment.
 bool ServicePending() const;
 FString ServiceObjective() const;
 TArray<FS47Task> ServiceTasks() const;
 // Flow calls after the new case's local controller and routine have both restored.
 void ValidateServiceRestore();
 FVector PersonalCarPosition() const;
 double PersonalCarHeading() const;
 bool Use(const FString& Id);
 bool Trigger(const FString& Scene);
 void Parked(const FString& Place);
 void ShowDialogue();
 void BootstrapTest(const FString& Mode);
 void ControlTest(double Dt);
 void ControlEntranceTest(double Dt);
 void ControlTakeawayTest(double Dt);
 void ControlServiceTest(double Dt);
 void ControlServiceDriveTest(double Dt);
 S47J::FObj Save() const;
 FS47EveningPack Pack;
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M = nullptr;
 struct FEntranceTest;
 TSharedPtr<FEntranceTest> EntranceTest;
 struct FTakeawayTest;
 struct FServiceTest;
 struct FServiceDriveTest;
 TSharedPtr<FTakeawayTest> TakeawayTest;
 TSharedPtr<FServiceTest> ServiceTest;
 TSharedPtr<FServiceDriveTest> ServiceDriveTest;
 FS47EveningPack RadioPack;
 TUniquePtr<FS47EveningVisual> Visual;
 TUniquePtr<FS47Takeaway> Takeaway;
 TSharedPtr<FS47RecordedDialogue> Voice;
 TSharedPtr<FS47RecordedDialogue> RadioVoice;
 TSharedPtr<FS47MediaSource> Music;
 TSharedPtr<FS47GainNode> MusicGain;
 bool bResumeRequested = false, bBindings = false;
 bool bServiceRoute = false, bServiceFiled = false;
 bool AtShiftFolder() const;
 bool bEnabled = false, bRing = false, bDialogueOpen = false, bRadioOn = true, bWorkStarted = false;
 bool bRadioRecorded = false;
 bool bRadioCaptionShown = false;
 TSharedPtr<bool> RadioCaptionAlive;
 double VoiceStartedAt = 0, RadioStartedAt = 0;
 double RadioHold = 0, RadioRest = 0, MusicPosition = 0, RingAt = 0;
 FString SpokenLine, RadioSpokenLine, TestMode;
 bool bRouteInitialized = false, bRouteFailed = false, bRouteDone = false, bRouteArrived = false, bRoutePaidRestored = false, bRouteOrderShot = false;
 int32 RouteStage = 0, RouteWalkIndex = 0, RouteArrivalCount = 0;
 double RouteElapsed = 0, RouteHold = 0, RouteStageElapsed = 0, RouteDistance = 0, RouteLastClock = 0;
 FVector RouteLastPos = FVector::ZeroVector;
 FString RouteName, RouteShots, RouteLastLeg;
 TArray<FVector> RouteWalk;
 bool bRouteAudio = false, bRouteAudioPauseChecked = false, bRouteAudioRadioInterrupted = false, bRouteAudioRadioLevels = false;
 FString RouteAudioObservedLine;
 TSet<FString> RouteAudioPlayedLines;
 double RouteAudioLineStart = 0, RouteAudioPauseAt = -1, RouteAudioPauseClock = 0;
 void Context();
 void Refile();
 void Changed();
 void AfterDialogue();
 void Answer();
 void StopRing();
 void RadioStep(double Dt);
 void Resume();
};
