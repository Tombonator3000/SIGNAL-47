#pragma once
#include "CoreMinimal.h"

// The weather of a Freeform day and the sky's almanac, in one place others can read (coordinator, 8 November: "a simple function
// others can use"; WEATHER-20261108 is to read the same Day() when the game gets weather of its own). The sky the game draws is the
// same every day (FS47Sky: a broken layer by day, clear and cold after dusk, dry lightning far off at night, a light wind), so a day
// here keeps to it: what changes is the wind and the dust it raises, the warmth, and now and then a shower that dries before it
// lands. Spring in the valley: windy afternoons, cold nights, next to no rain. The almanac is the sky's own: sunrise 05:30 and
// sunset 18:27 (FS47Sky::SetClock), and the moon's real phase on the date (the sky has no moon of its own yet; MOON-20261108 can
// read MoonAge). Pure: no world and no clock, the same for a seed and a day whatever is loaded or waited through.
namespace S47Weather
{
 struct FDay
 {
  int32 HighF=74,LowF=40;     // degrees Fahrenheit
  int32 WindMph=10,GustMph=0; // the afternoon's wind; no gusts to speak of when 0
  int32 WindFrom=225;         // degrees, the way it blows from
  float Cloud=.5f;            // the day's cover as FS47Sky draws it: a broken layer, .4 to .7
  bool bDust=false;           // blowing dust in the afternoon
  bool bShower=false;         // a few drops, most of it drying before it lands
  bool bLightning=true;       // dry lightning far off after dark
 };
 // The day's weather in this game (the seed is FS47FreeformSession::Seed()).
 SIGNAL47_API FDay Day(int32 Seed,int32 ShiftDay);
 // What a forecast written on IssuedOn said about ForDay: the day itself with a forecaster's errors, the more the further ahead.
 SIGNAL47_API FDay Forecast(int32 Seed,int32 IssuedOn,int32 ForDay);
 SIGNAL47_API FString Compass(int32 FromDegrees);
 // For a paper: "Windy, with blowing dust in the afternoon. Southwest 20 to 30, gusts to 45. High 78, low 41." Short: "Windy, dust. 78/41".
 SIGNAL47_API FString Words(const FDay& D,bool bShort=false);
 // The sky's own times (seconds after midnight).
 constexpr double SunriseClock=5*3600.+30*60.,SunsetClock=18*3600.+27*60.;
 SIGNAL47_API FString ClockWords(double SecondsAfterMidnight); // "5:30 a.m."
 // The moon: days since the last new moon at noon in the valley on that date (0 to about 29.5), its phase in words, and the date of
 // the next new moon (0), first quarter (1), full moon (2) or last quarter (3) on or after a date.
 SIGNAL47_API double MoonAge(const FDateTime& Date);
 SIGNAL47_API FString MoonPhase(const FDateTime& Date);
 SIGNAL47_API FDateTime NextPhase(const FDateTime& From,int32 Quarter);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
