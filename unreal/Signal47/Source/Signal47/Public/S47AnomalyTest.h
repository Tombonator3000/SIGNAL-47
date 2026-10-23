#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"

class US47Main;

// Persistent control harness: Flow.StartGame must not recreate this object.
// Uses rendered camera exposures and ordinary disk save/restore; fixture cuts and
// API actions are explicitly labelled, never reported as human keyboard input.
class FS47AnomalyTest
{
public:
 explicit FS47AnomalyTest(US47Main* InM) : M(InM) {}
 void Step(double Dt);
private:
 US47Main* M = nullptr;
 bool bInitialized = false, bDone = false;
 FString Route, Shots, Profile;
 int32 Stage = 0;
 double Elapsed = 0, StageElapsed = 0, Hold = 0;
 TArray<S47J::FObj> Events;
 TArray<FString> Requested;
 S47J::FObj EarlyState;
 void Fail(const FString& Why);
 void Next(int32 To, const FString& Action, double Wait = .7);
 void Event(const FString& Action);
 void Shot(const FString& Name);
 void Finish();
 bool RoundTrip(const FString& Name, const S47J::FObj& Override = nullptr);
 bool Photograph(const FString& ExpectedTarget);
 void Pose(const FVector& Feet, const FVector& LookAt);
 void Cut(const FString& Chapter);
};
