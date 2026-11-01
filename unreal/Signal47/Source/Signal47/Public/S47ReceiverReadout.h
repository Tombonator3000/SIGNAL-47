#pragma once
#include "CoreMinimal.h"
#include "S47Rx.h"
#include "Widgets/SWidget.h"

// Presentation only. Callbacks use the same receiver/profile and start guards
// as the owner; array availability is never substituted for received quality.
namespace S47ReceiverReadout
{
 struct FData
 {
  TFunction<FS47Rx()> GetReceiver;
  TFunction<FS47Profile()> GetProfile;
  TFunction<double()> GetQuality,GetProgress,GetArrayQuality;
  TFunction<bool()> GetReady,GetTrackingReady,GetRecording;
  TFunction<FString()> GetNextAction;
 };
 TSharedRef<SWidget> Build(const FData& Data);
}
