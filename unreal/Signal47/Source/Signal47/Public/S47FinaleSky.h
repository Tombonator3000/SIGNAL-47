#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"

class US47Main;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// Original SC13 art. Chapter6 owns time, activation and disposal; this object never advances
// a clock, starts a timer or moves the camera. Missing/unverified art retains the original ending.
class SIGNAL47_API FS47FinaleSky
{
public:
 explicit FS47FinaleSky(US47Main* InMain);
 ~FS47FinaleSky();
 bool Prepare();
 void Step(double Mile, const FString& Stage, double EventSeconds, double PullSeconds);
 void Hide();
 bool Ready() const { return bReady; }
 S47J::FObj Readback() const;
 static S47J::FObj InspectAssets(); // same actual LOD/material reader used by the fresh editor process
private:
 TWeakObjectPtr<US47Main> Main;
 TWeakObjectPtr<UStaticMeshComponent> Craft;
 TWeakObjectPtr<UStaticMeshComponent> Banks[2];
 TWeakObjectPtr<UMaterialInstanceDynamic> BankMids[2];
 S47J::FObj Binding, ActualAssets;
 bool bReady = false, bShown = false, bLow = false;
 double Phase = 0, Growth = 0, Reveal = 0;
 FVector Crossing = FVector::ZeroVector, ForwardUE = FVector::ForwardVector, RightUE = FVector::RightVector;
 FString Failure;
};
