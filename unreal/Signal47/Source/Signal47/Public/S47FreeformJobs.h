#pragma once
#include "CoreMinimal.h"
#include "S47Rx.h"

// Pure, bounded daily observation cards. No world access, global RNG or deadlines.
class FS47FreeformJobs
{
public:
 enum class EControl : uint8 { PointingCross, PulsarTiming, InternalReference };
 struct FCard
 {
  int32 Day=1,Slot=0,Source=0;
  FS47Profile Receiver;
  double El=25,Seconds=12,SuggestedClock=19*3600.;
  EControl Control=EControl::PointingCross;
 };
 static constexpr int32 DailyCount=3;
 // Source override is meaningful only for slot 2: HI(2) or Virgo(3).
 static FCard Make(int32 Seed,int32 Day,int32 Slot,int32 Source=-1);
 static TArray<FCard> Generate(int32 Seed,int32 Day);
 static FString ControlText(EControl Control);
 static bool Valid(const FCard& Card);
 static bool SelfTest(TArray<FString>& Results);
};
