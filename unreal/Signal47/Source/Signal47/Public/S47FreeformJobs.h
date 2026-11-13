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
 // M7-SPREAD-20261113: slot 2's own survey for that day's week (days 1-7, 8-14, ...): HI(2) and Virgo(3) by turns, from the seed.
 static int32 WeekSource(int32 Seed,int32 Day);
 static TArray<FCard> Generate(int32 Seed,int32 Day);
 // LOOP-NIGHT-WATCH-20261113: a clock of day on the shift's clock, which runs from 08:00 to 08:00: the small hours after 24:00.
 static double ShiftClock(double DayClock){return DayClock<8*3600.?DayClock+86400.:DayClock;}
 // The third card from day 2 is the night watch: its suggested time is in the small hours, after midnight on its own shift day.
 static bool NightWatch(const FCard& Card){return Card.Slot==2&&Card.SuggestedClock<8*3600.;}
 static FString ControlText(EControl Control);
 static bool Valid(const FCard& Card);
 static bool SelfTest(TArray<FString>& Results);
};
