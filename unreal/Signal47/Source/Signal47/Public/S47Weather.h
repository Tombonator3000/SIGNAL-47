#pragma once
#include "CoreMinimal.h"
#include "Math/Color.h"

// The weather of a Freeform day and the sky's almanac, in one place others can read (coordinator, 8 November: "a simple function
// others can use"). WEATHER-20261108: Day() is the truth. The paper prints it (S47Ledger reads Forecast and Words), and what the
// player sees, hears and drives through comes from the same day by Now(): FS47WeatherLive puts it on the sky's cloud, the fog, the
// wind in the brush, the ambience and the truck each frame, through Live(). What a spring here has (New Mexico, April and May):
//   - wind most afternoons, and on some days a strong south-west wind from about one o'clock with blowing sand, the far land gone
//     brown, easing in the evening (some of it may hold into the night: NightStorm, which the night's rhythm reads);
//   - now and then a stray shower late in the day, most of it drying before it reaches the ground (virga): a few drops, thunder
//     far off; the dirt is slick for a few hours after;
//   - haze some mornings, always the morning after a shower.
// Dry lightning far off every night is the sky's own (FS47Sky::Update). The almanac is the sky's: sunrise 05:30 and sunset 18:27
// (FS47Sky::SetClock), Halley's comet low in the south from the evening until a little after three (FS47Sky::SetCometClock) and,
// since MOON-20261108, the real moon of the date (Moon, Almanac(Date)); Almanac() alone is the sun and the comet. Pure: no world and
// no clock, the same for a seed and a day whatever is loaded or waited through. The paper keeps the word dust out (its own test):
// a dust day is "blowing sand".
namespace S47Weather
{
 struct FDay
 {
  int32 HighF=78,LowF=41;     // degrees Fahrenheit
  int32 WindMph=8,GustMph=18; // the afternoon's wind; no gusts to speak of when 0
  float WindFrom=225.f;       // degrees, the way it blows from (FS47Omen::WindFrom: the gusts come from there)
  float CloudAm=.6f,CloudPm=.5f,CloudNight=.12f; // the cover as FS47Sky draws it with the day's weather: morning, afternoon, night
  bool bDust=false;           // a strong wind with blowing sand in the afternoon
  float DustK=0;              // how thick it gets, .45 to 1
  float DustFrom=13.5f,DustTo=19.f; // its hours (clock hours of the day)
  float NightStorm=0;         // how much of the wind holds into the night, 0 to 1
  bool bShower=false;         // a stray shower late in the day, mostly virga, a few drops
  float ShowerAt=17.f;        // when (clock hour)
  bool bThunder=false;        // thunder with it, far off
  bool bHaze=false;           // haze in the morning
  float HazeK=0;              // how thick, .5 to 1
  bool bLightning=true;       // dry lightning far off after dark: every night, as the sky has it
  bool Windy() const{return WindMph>=18;}
 };
 // The day's weather in this game (the seed is FS47FreeformSession::Seed(); the day is the shift day, whose daytime is that date's).
 SIGNAL47_API FDay Day(int32 Seed,int32 ShiftDay);
 // What a forecast written on IssuedOn said about ForDay: the day itself with a forecaster's errors in the warmth and the wind, the
 // more the further ahead; two days out or more a shower can be missed. On the day itself it is the day's weather.
 SIGNAL47_API FDay Forecast(int32 Seed,int32 IssuedOn,int32 ForDay);
 // For a paper: "Mostly cloudy in the morning, partly cloudy in the afternoon, clearing after sundown. Light winds, gusty at times.
 // Dry lightning far off after dark. High 78, low 41." Short: "Partly cloudy. 78/41". A dust day says blowing sand, a shower day a
 // stray shower, a hazy morning haze early.
 SIGNAL47_API FString Words(const FDay& D,bool bShort=false);
 // What the weather is doing at a moment, from the day (and the night before, for the small hours). Wind: how strong beyond the light
 // breeze the brush always has, 0 to 1 (1 about 34 mph); Mph: the wind now; Dust, Rain, Haze 0 to 1; Wet: how slick the dirt is
 // after a shower; CloudBias: what the day adds to the sky's own cover (FS47Sky::CloudCoverAt); Storm: the wind held into the night;
 // DirX, DirZ: which way it blows, three-space (x east, z south), away from FDay::WindFrom.
 // ShowerH (SHOWER-LOOK-20261109): hours since the day's shower set in (FDay::ShowerAt), NoShower on a day without one; the sky
 // walks the shower's cloud past on it (FS47Sky::CellPlace), upwind before and downwind after.
 constexpr double NoShower=-99;
 struct FNow{double Wind=0,Mph=8,Dust=0,Rain=0,Haze=0,Wet=0,CloudBias=0,Storm=0,DirX=.6,DirZ=-.8,ShowerH=NoShower;bool bThunder=false;};
 SIGNAL47_API FNow Now(int32 Seed,int32 CalendarDay,double ClockOfDay);
 // A shower's own course, Hours after it set in: the rain (about three quarters of an hour at its fullest), the wet dirt (slick from a
 // third of an hour in, drying from an hour and a half, dry by four), thunder far off with it if bThunder, and what its cloud adds to
 // the sky. Now() has a shower day's from this, and F10's shower runs the same course on the clock.
 SIGNAL47_API void ShowerCourse(double Hours,bool bThunder,double& OutRain,double& OutWet,double& OutCloud,bool& bOutThunder);
 SIGNAL47_API double CloudBiasAt(const FDay& D,double ClockOfDay);
 // What is on the world now: FS47WeatherLive writes it each frame in Freeform; off elsewhere, where the sky is the Story's own.
 // FogAdd: density added to the native fog where the player is (less indoors); Tint, TintK: the colour the fog takes and how much
 // of it; HorizonK: how much more the far land fades.
 struct FLive
 {
  bool bOn=false;
  double Wind=0,Mph=8,Dust=0,Rain=0,Haze=0,Wet=0,CloudBias=0,Storm=0,DirX=.6,DirZ=-.8,ShowerH=NoShower;
  double FogAdd=0,TintK=0,HorizonK=0;
  FLinearColor Tint=FLinearColor(.5f,.38f,.24f);
 };
 SIGNAL47_API FLive& Live();
 // For a paper's "last week" (S47Ledger still prints its own, "no rain"): the highs and lows of the days From to To-1, and whether it
 // rained or blew sand.
 SIGNAL47_API FString LastWeek(int32 Seed,int32 FromDay,int32 ToDay);
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
