#pragma once
#include "CoreMinimal.h"
#include "S47EveningPack.h"
#include "S47Json.h"
class US47Main;
class FS47EveningVisual;
class FS47MediaSource;
class FS47GainNode;
class FS47RecordedDialogue;

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
 bool Use(const FString& Id);
 bool Trigger(const FString& Scene);
 void Parked(const FString& Place);
 void ShowDialogue();
 void BootstrapTest(const FString& Mode);
 void ControlTest(double Dt);
 S47J::FObj Save() const;
 FS47EveningPack Pack;
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M = nullptr;
 FS47EveningPack RadioPack;
 TUniquePtr<FS47EveningVisual> Visual;
 TSharedPtr<FS47RecordedDialogue> Voice;
 TSharedPtr<FS47RecordedDialogue> RadioVoice;
 TSharedPtr<FS47MediaSource> Music;
 TSharedPtr<FS47GainNode> MusicGain;
 bool bResumeRequested = false, bBindings = false;
 bool bEnabled = false, bRing = false, bDialogueOpen = false, bRadioOn = true, bWorkStarted = false;
 bool bRadioRecorded = false;
 double RadioHold = 0, RadioRest = 0, MusicPosition = 0, RingAt = 0;
 FString SpokenLine, RadioSpokenLine, TestMode;
 bool bRouteInitialized = false, bRouteFailed = false, bRouteDone = false, bRouteArrived = false, bRoutePaidRestored = false, bRouteOrderShot = false;
 int32 RouteStage = 0, RouteWalkIndex = 0, RouteArrivalCount = 0;
 double RouteElapsed = 0, RouteHold = 0, RouteStageElapsed = 0, RouteDistance = 0, RouteLastClock = 0;
 FVector RouteLastPos = FVector::ZeroVector;
 FString RouteName, RouteShots, RouteLastLeg;
 TArray<FVector> RouteWalk;
 void Context();
 void Refile();
 void Changed();
 void AfterDialogue();
 void Answer();
 void StopRing();
 void RadioStep(double Dt);
 void Resume();
};
