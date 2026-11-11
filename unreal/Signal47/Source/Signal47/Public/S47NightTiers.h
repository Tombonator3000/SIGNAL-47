#pragma once
#include "CoreMinimal.h"

// TIERS-20261109 (Tom, 9 November, replacing "nothing alien before about two in the morning"): what the unexplained may be, by the
// clock and the week. One rule for every system that asks.
//   Hint     18 to 24, and by day: a normal job. Only what can be explained away: far lights, sounds, odd things on the radio, data that
//            does not agree, an eye's glint a long way off.
//   Glimpse  24 to 02: a silhouette at the edge of the lamp or far out on the flat, something crossing the road far ahead of the truck.
//            Never near, never contact, never taken.
//   Full     from 02: near, inside, taken.
// In the fourth week (night 22 on, the curve's last row) the edges creep an hour earlier: glimpses from 23:00, full from 01:00.
// An authored night (the big night, a director script) may start earlier when it says so on purpose ("early": true): Allows' bEarly.
// The tiers say the most a moment may hold; each system keeps its own hours inside them (their hours still end at six).
enum class ES47Allowed : uint8 { Hint, Glimpse, Full };

namespace S47NightTiers
{
 // Hours of the night, past midnight counted on past 24 (as FS47Happenings::NightHour counts them): noon to noon.
 inline constexpr double GlimpseFrom=24.,FullFrom=26.,LateGlimpseFrom=23.,LateFullFrom=25.;
 // The first night of the fourth week (director.json's curve row from day 22).
 inline constexpr int32 LateFromNight=22;
 inline double NightHour(double HourOfDay){const double H=FMath::Fmod(FMath::Fmod(HourOfDay,24.)+24.,24.);return H<12?H+24:H;}
 inline bool Late(int32 Night){return Night>=LateFromNight;}
 inline double GlimpseHour(int32 Night){return Late(Night)?LateGlimpseFrom:GlimpseFrom;}
 inline double FullHour(int32 Night){return Late(Night)?LateFullFrom:FullFrom;}
 // The clock hour (0 to 24) from which a night's full meetings may start: 2 for weeks one to three, 1 for the fourth.
 inline double FullClockHour(int32 Night){return FullHour(Night)-24.;}
 // What the hour of the day (0 to 24) holds on this night (FS47FreeformSession::NightOf).
 inline ES47Allowed At(double HourOfDay,int32 Night)
 {
  const double H=NightHour(HourOfDay);
  return H>=FullHour(Night)?ES47Allowed::Full:H>=GlimpseHour(Night)?ES47Allowed::Glimpse:ES47Allowed::Hint;
 }
 // From the session's world clock (seconds since the first day's midnight).
 inline ES47Allowed AtClock(double Clock,int32 Night){return At(FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.,Night);}
 // Whether something that needs Need may start now. bEarly: an authored exception, set on purpose.
 inline bool Allows(ES47Allowed Need,double HourOfDay,int32 Night,bool bEarly=false){return bEarly||uint8(At(HourOfDay,Night))>=uint8(Need);}
 // The clock hour the tier begins on this night ("02:00" for a full meeting in week one), for the panels and the lint.
 inline double FromClockHour(ES47Allowed Need,int32 Night)
 {return Need==ES47Allowed::Full?FullHour(Night)-24.:Need==ES47Allowed::Glimpse?FMath::Fmod(GlimpseHour(Night),24.):18.;}
 SIGNAL47_API const TCHAR* Name(ES47Allowed Tier);
 // "hint", "glimpse" or "full"; false for anything else.
 SIGNAL47_API bool FromName(const FString& Text,ES47Allowed& Out);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
