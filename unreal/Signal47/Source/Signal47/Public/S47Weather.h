#pragma once
#include "CoreMinimal.h"

// The weather of a Freeform day and the sky's almanac, in one place others can read (coordinator, 8 November: "a simple function
// others can use"). It follows the sky the game draws, never the other way round (coordinator, 8 November: the paper is to say what
// the player can see). What the sky does is the same every day: FS47Sky draws a broken layer by day, more of it in the morning
// (CloudCoverAt), breaking up after sundown to a few night clouds, dry lightning far off every night with its thunder (Update);
// S47Wind and FS47Ambience give a light wind with gusts now and then; no dust, no rain, no moon of its own (the night's moonlight is
// the same every night, S47Lighting). So a day here has the sky's clouds, a light wind, dry lightning after dark and no dust or rain;
// what changes is only the warmth, which nobody sees (FS47Ambience keeps the night's cooling to about 41 F at dawn, and the day's
// highs climb slowly through the spring). The almanac is the sky's own: sunrise 05:30 and sunset 18:27 (FS47Sky::SetClock) and
// Halley's comet, low in the south from the evening until a little after three (FS47Sky::SetCometClock). Since MOON-20261108 the sky
// has the real moon of the date in Freeform (Moon: its place, phase and light; FS47Sky draws it, AS47Web lights the land by it), and
// the paper's almanac for a date prints it (Almanac(Date)); Almanac() alone is the sun and the comet. When WEATHER-20261108 gives the
// sky days of its own (wind, dust, a shower), the sky should read Day() and Day() gets its variety back. Pure: no world and no
// clock, the same for a seed and a day whatever is loaded or waited through.
namespace S47Weather
{
 struct FDay
 {
  int32 HighF=78,LowF=41;     // degrees Fahrenheit
  int32 WindMph=8,GustMph=18; // the afternoon's wind; no gusts to speak of when 0
  float CloudAm=.6f,CloudPm=.5f,CloudNight=.12f; // the cover as FS47Sky draws it: morning, afternoon, night
  bool bDust=false;           // blowing dust: the sky draws none yet
  bool bShower=false;         // a shower: the sky draws none yet
  bool bLightning=true;       // dry lightning far off after dark: every night, as the sky has it
 };
 // The day's weather in this game (the seed is FS47FreeformSession::Seed()).
 SIGNAL47_API FDay Day(int32 Seed,int32 ShiftDay);
 // What a forecast written on IssuedOn said about ForDay: the day itself with a forecaster's errors in the warmth and the wind, the
 // more the further ahead. Never weather the sky does not have.
 SIGNAL47_API FDay Forecast(int32 Seed,int32 IssuedOn,int32 ForDay);
 // For a paper: "Mostly cloudy in the morning, partly cloudy in the afternoon, clearing after sundown. Light winds, gusty at times.
 // Dry lightning far off after dark. High 78, low 41." Short: "Partly cloudy. 78/41".
 SIGNAL47_API FString Words(const FDay& D,bool bShort=false);
 // The sky's own times (seconds after midnight).
 constexpr double SunriseClock=5*3600.+30*60.,SunsetClock=18*3600.+27*60.;
 // Halley's comet over the land: from 20:00 until 03:05, as the sky draws it.
 constexpr double CometRises=20*3600.,CometSets=3*3600.+5*60.;
 SIGNAL47_API FString ClockWords(double SecondsAfterMidnight); // "5:30 a.m."
 // The almanac line as the paper prints it: the sun and the comet, what the sky shows.
 SIGNAL47_API FString Almanac();
 // MOON-20261108: the same, with the moon the sky now draws on that date: its phase, when it is up, and the next full or new moon.
 SIGNAL47_API FString Almanac(const FDateTime& Date);
 // The moon: days since the last new moon at noon in the valley on that date (0 to about 29.5), its phase in words, and the date of
 // the next new moon (0), first quarter (1), full moon (2) or last quarter (3) on or after a date. For MOON-20261108; not printed.
 SIGNAL47_API double MoonAge(const FDateTime& Date);
 SIGNAL47_API FString MoonPhase(const FDateTime& Date);
 SIGNAL47_API FDateTime NextPhase(const FDateTime& From,int32 Quarter);
 // MOON-20261108: the moon in the valley's sky at a moment (the calendar date and the clock, seconds after midnight, local): its
 // age, the share of it lit (0 new to 1 full), and where it stands (degrees: azimuth from north through east, elevation). A plain
 // model good to a few degrees: it lags the sun by its age, on the ecliptic. Light: what its light is worth on the land against
 // the half moon the night was tuned for (1): about .4 with no moon up, 2.4 for a full moon high.
 // SunAz, SunEl: the sun by the same reckoning, for which side of the moon is lit.
 struct FMoon{double Age=0,Lit=0,Az=0,El=-90,SunAz=0,SunEl=-90;};
 SIGNAL47_API FMoon Moon(const FDateTime& Date,double Clock);
 SIGNAL47_API float MoonLight(const FMoon& Mo);
 // When it is up over the horizon, from a clock (seconds after midnight) on that date, searched minute by minute: rise and set; -1 none.
 SIGNAL47_API void MoonTimes(const FDateTime& Date,double& OutRise,double& OutSet);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
