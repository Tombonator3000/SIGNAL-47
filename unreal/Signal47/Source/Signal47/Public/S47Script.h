#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47NightTiers.h"

// DIRECTOR-TOOL-20261109 (Tom, 9 November: "en regissør-mulighet i spillet så jeg/vi kan lage skriptede hendelser"). A scripted event
// is one JSON file in Content/Director/scripted/<id>.json: when it may start (its trigger) and what then happens on a timeline of
// seconds from its start (its actions). This file is the data and the rules only: reading and writing the JSON, whether a trigger
// holds for a given moment (FS47ScriptNow, filled from the world by FS47ScriptPlayer or by hand in the tests), and the lint that
// keeps a script inside Freeform's limits. Nothing here touches the world.
//
// The fixed rules, the same as for the night's own events (DECISIONS, Freeform):
//   - the night's tiers (S47NightTiers, Tom 9 November): a script with a figure is a full meeting and starts from two in the morning (one
//     in the fourth week), a glimpse ("tier": "glimpse", every figure vanishing when looked at) from midnight (23:00 in the fourth week),
//     both until AlienUntil; "early": true, set on purpose, lets an authored one start from AlienEarlyFrom. A script without a figure
//     keeps the tier its file sets (CRIT6-20261109), and one with a formation in the sky is a glimpse at least; with neither it is a
//     hint, at any hour its trigger allows;
//   - never while another big event holds the night (the Director's event, being taken, a night happening on the ground, the light
//     over the highway, another script): FS47ScriptNow::bBigEvent;
//   - the spoiler limits: never the 47 seconds, the 1947 photograph, C, the cable, Tomás, the third silhouette, how anyone was taken,
//     or 8.15. LintText refuses any text that says them; a script with such text is neither saved nor played.
//
// Points are road-local metres (x east, z south), the land's one frame (S47OpenLand, S47EventPlaces), so a script works whichever
// area the player is in. Up is metres above the ground there.

enum class ES47ScriptAct : uint8 { Figure, Sky, Sound, Dish, Fault, Line, Toast, Note, Evidence, Look, Wait, Shot, Count };
enum class ES47ScriptRepeat : uint8 { Once, Nightly, Always };
// DIRECTOR-MODE-01 (slice 3): a trigger's volume, as the HPL editor's script areas: its ground shape, and what the player does with it.
enum class ES47ScriptShape : uint8 { Circle, Box };
enum class ES47ScriptMode : uint8 { Enter, Look, Leave };

struct FS47ScriptPoint
{
 double X=0,Z=0,Up=0;
 bool bSet=false;
 FVector2D Road() const{return FVector2D(X,Z);}
};

struct FS47ScriptCond
{
 // finding, no_finding (Arg: a finding id, "*" at the end matches a prefix); case_open, case_closed, case_none (Num: the case line);
 // weather (Arg: clear, cloudy, windy, calm, dust, shower, storm); moon_min, moon_max (Num: the lit fraction 0 to 1); moon_up;
 // indoors, outdoors, in_truck, on_foot; near, far (Arg: a place id from S47EventPlaces, Num: metres); story_flag, no_story_flag (Arg:
 // the flag; DIRECTOR-MODE-01: without a Story night's director no story flag is set).
 FString If,Arg;
 double Num=0;
};

struct FS47ScriptTrigger
{
 int32 DayFrom=0,DayTo=0;     // the shift day, inclusive; 0: any
 int32 NightFrom=0,NightTo=0; // the night (FS47FreeformSession::NightOf), inclusive; 0: any
 double HourFrom=-1,HourTo=-1; // clock hours; From > To runs past midnight; negative: any time
 FString Place;               // an S47EventPlaces id; empty: none
 FS47ScriptPoint Point;       // or a point of its own (bSet)
 double Radius=40;            // metres from the place or the point
 TArray<FS47ScriptCond> Conds;
 // DIRECTOR-MODE-01 (slice 3): the volume around the place or the point. The defaults are a circle of Radius entered at any height,
 // at once: what every trigger did before, and what a file without these fields reads as (they are written only when not these).
 ES47ScriptShape Shape=ES47ScriptShape::Circle;
 double HalfX=5,HalfZ=5;      // box: half its size, metres, along its own x and z
 double Yaw=0;                // box: radians about up, turning its own x from east towards south (road-local)
 double Height=0;             // 0: any height; above 0 the player's height over the ground is within [Point.Up, Point.Up+Height]
 ES47ScriptMode Mode=ES47ScriptMode::Enter; // enter: inside it; look: its middle in view; leave: was inside, now outside
 double Degrees=12,LookRange=60; // look: within Degrees of the middle of the view, from at most LookRange metres
 double Dwell=0;              // seconds the mode's test must have held, continuously, before it starts
 bool HasWhere() const{return !Place.IsEmpty()||Point.bSet;}
 // Anything of the volume that is not the plain circle entered at once: the player watches it more closely (FS47ScriptPlayer).
 bool HasVolume() const{return Shape!=ES47ScriptShape::Circle||Height!=0||Mode!=ES47ScriptMode::Enter||Dwell!=0;}
};

struct FS47ScriptAction
{
 ES47ScriptAct Do=ES47ScriptAct::Toast;
 double At=0;                 // seconds from the script's start
 // What each kind reads (the rest is ignored and not written):
 //   figure    Kind body (pale, tall, crab); Path; Speed m/s; Secs it stands at the end; bFlag vanishes when looked at
 //   sky       Kind form (v, line, triangle, circle, scatter, high_track); Who pace (slow, fast, out)
 //   sound     Text the recording's name (thudSoft, lic:door_heavy ...); Who where (here, placed, radio, cb, walkie); Point; Gain
 //   dish      Az, El degrees; Slew; Secs held before the array goes back (0: it stays); Index one antenna (0 to 26), -1 all
 //   fault     Kind (omen, power, contact); Gain severity 0 to 1; Point (unset: the player); Secs
 //   line      Who (dale, radio, cb, voice); Text
 //   toast     Text; Secs
 //   note      Title; Text
 //   evidence  Kind the finding id; Text
 //   look      Point; Secs the turn takes
 //   wait      Secs; bFlag: until the player looks at Point instead, at most Secs
 //   shot      SHOT-BEAT-01 (S47Shot.h): Path[0] the eye, Path[1] a second eye it moves to (optional); Point what it looks at; Fov
 //             degrees, Fov2 at the end (0: the same); Secs held, In and Out the ease from and back onto the player's view
 FString Kind,Who,Text,Title;
 TArray<FS47ScriptPoint> Path;
 FS47ScriptPoint Point;
 double Secs=0,Speed=1,Gain=1,Az=26,El=40,Slew=9;
 double Fov=60,Fov2=0,In=.6,Out=.8; // the shot's (SHOT-BEAT-01); not written for any other kind
 bool bFlag=false;
 int32 Index=-1;
};

struct FS47Script
{
 static constexpr int32 Version=1;
 FString Id,Name;
 bool bEnabled=false;
 ES47ScriptRepeat Repeat=ES47ScriptRepeat::Once;
 double CooldownMin=60;       // Always: minutes of the game's clock before it can come again
 ES47Allowed Tier=ES47Allowed::Full; // what its figures are: full, or a glimpse (every figure vanishes when looked at); no figure: a hint
 bool bTierSet=false;         // the file says "tier" (CRIT6-20261109): then it holds for a script without a figure too
 bool bEarly=false;           // an authored exception: its figures may start before their tier's hour, from AlienEarlyFrom
 FS47ScriptTrigger Trigger;
 TArray<FS47ScriptAction> Actions;
 FString File;                // where it was read from; not written
 bool HasFigure() const;
 bool HasSky() const;
 // What it needs of the night's tier: its Tier with a figure or when the file sets one, else a hint; a formation in the sky (Sky)
 // a glimpse at least (CRIT6-20261109).
 ES47Allowed Need() const;
 double Length() const;       // the last action's start plus what it holds for, seconds
};

// What a trigger is asked against. Filled from the game (FS47ScriptPlayer::Now) or by a test.
struct FS47ScriptNow
{
 int32 Day=1,Night=1;
 double Clock=8*3600.;         // the session's world clock (seconds since the first day's midnight)
 bool bHasRoad=false;FVector2D Road=FVector2D::ZeroVector; // the player, road-local
 bool bIndoors=false,bInTruck=false;
 TArray<FString> Weather;      // the words that hold now: clear or cloudy, windy or calm, dust, shower, storm
 double MoonLit=.5;bool bMoonUp=false;
 bool bBigEvent=false;FString BigWhy;
 TFunction<bool(const FString&)> HasFinding;
 TFunction<int32(int32)> CaseState; // 0 none, 1 open, 2 closed
 TFunction<bool(const FString&,FVector2D&,double&)> PlaceAt; // id: road-local middle and radius
 // DIRECTOR-MODE-01 (slice 3): what a trigger volume reads. FloorUp is filled by Now; the rest is this one script's, filled by the
 // player for a script whose trigger HasVolume (FS47ScriptPlayer::NowFor). The defaults are what every trigger read before.
 double FloorUp=0;             // the player's height over the ground where they stand (as a point's Up)
 bool bLooking=false;          // look: the volume's middle is within its Degrees of the middle of the view
 double Held=0;                // seconds the mode's test has held, continuously, up to now
 bool bWasInside=false;        // leave: the player has been inside the volume since it was last watched from the start
 TFunction<bool(const FString&)> HasStoryFlag; // story_flag, no_story_flag; unset: no Story night's director (no flag is set)
 double Hour() const{return FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;}
};

// What has been played, saved with the session.
struct FS47ScriptMemory
{
 TMap<FString,int32> Count,LastNight;
 TMap<FString,double> LastClock;
 void Played(const FString& Id,int32 Night,double Clock){Count.FindOrAdd(Id)++;LastNight.Add(Id,Night);LastClock.Add(Id,Clock);}
 S47J::FObj Save() const;
 void Load(const S47J::FObj& O);
};

namespace S47Script
{
 // Until first light: a script with a figure starts from its tier's hour (S47NightTiers::FromClockHour) until AlienUntil, or, set
 // early on purpose, from AlienEarlyFrom. AlienFrom: the full tier's hour in the first three weeks, for the texts.
 constexpr double AlienFrom=2.,AlienUntil=6.,AlienEarlyFrom=19.;
 // The clock hour this script's figures may start from on that night.
 SIGNAL47_API double FigureFrom(const FS47Script& S,int32 Night);
 SIGNAL47_API const TCHAR* ActName(ES47ScriptAct A);
 SIGNAL47_API ES47ScriptAct ActFrom(const FString& Name);
 SIGNAL47_API const TCHAR* RepeatName(ES47ScriptRepeat R);
 SIGNAL47_API ES47ScriptRepeat RepeatFrom(const FString& Name);
 // The choices each kind offers, for the editor's menus and the lint.
 SIGNAL47_API const TArray<FString>& Bodies();
 SIGNAL47_API const TArray<FString>& Forms();
 SIGNAL47_API const TArray<FString>& Paces();
 SIGNAL47_API const TArray<FString>& SoundWhere();
 SIGNAL47_API const TArray<FString>& FaultKinds();
 SIGNAL47_API const TArray<FString>& Voices();
 SIGNAL47_API const TArray<FString>& CondKinds();
 SIGNAL47_API const TArray<FString>& WeatherWords();
 // A few recordings the game has, for the editor's sound menu (any name the game's sound knows works).
 SIGNAL47_API const TArray<FString>& SoundPresets();

 SIGNAL47_API S47J::FObj ToJson(const FS47Script& S);
 // False with Why when it is not a script of this version; what it does not know is left out.
 SIGNAL47_API bool FromJson(const S47J::FObj& O,FS47Script& Out,FString& Why);
 SIGNAL47_API FString Write(const FS47Script& S);
 SIGNAL47_API bool Read(const FString& Text,FS47Script& Out,FString& Why);
 // "23:30", "23.5" or "23" as hours; negative when it is none of them.
 SIGNAL47_API double ParseHour(const FString& Text);
 SIGNAL47_API FString HourText(double Hour);
 // A file name's id: lower case, a to z, 0 to 9 and _, at most 48.
 SIGNAL47_API FString CleanId(const FString& Text);

 // Empty when the words keep to Freeform's limits; otherwise what they touch, for the editor ("the 47 seconds are Story's").
 SIGNAL47_API FString LintText(const FString& Text);
 // Everything wrong with a script: its words (LintText), and fields it cannot play with. Warnings: what will hold it back but is
 // not wrong (a figure in hours that never reach two in the morning).
 SIGNAL47_API TArray<FString> Lint(const FS47Script& S,TArray<FString>* Warnings=nullptr);

 // Whether the clock hour is in [From, To), From > To running past midnight.
 SIGNAL47_API bool InHours(double Hour,double From,double To);
 // Whether this script may start now. Why: what holds it back. bPreview leaves out the trigger and the history (the editor's
 // play-now), never the lint; the clock rule and the lock are then reported in Why but do not stop it.
 SIGNAL47_API bool Ready(const FS47Script& S,const FS47ScriptNow& Now,const FS47ScriptMemory& Mem,FString& Why,bool bPreview=false);
 SIGNAL47_API bool CondHolds(const FS47ScriptCond& C,const FS47ScriptNow& Now);

 // DIRECTOR-MODE-01 (slice 3): the trigger volume. Pure.
 // Whether P (road-local, P.Up the player's height over the ground) is inside T's volume around T.Point: the circle (Radius) or the
 // turned box (HalfX, HalfZ, Yaw), and with Height above 0 the height band [Point.Up, Point.Up+Height] (HeightSlack either side).
 inline constexpr double HeightSlack=.3;
 SIGNAL47_API bool Inside(const FS47ScriptTrigger& T,const FS47ScriptPoint& P);
 // The volume's outline on the ground around T.Point, road-local at T.Point.Up, closed: Segments points (at least 4, a box at least
 // 5), the last the first again. A circle's points are at Radius; a box has its four corners and the rest along its edges. For the
 // in-world drawing.
 SIGNAL47_API TArray<FS47ScriptPoint> Outline(const FS47ScriptTrigger& T,int32 Segments=24);
 // The player is inside the volume now (around its place's middle for a place); false with no player on the land or no place.
 SIGNAL47_API bool InVolume(const FS47ScriptTrigger& T,const FS47ScriptNow& Now);
 // The mode's test now, without the dwell: enter inside it; look within LookRange of its middle with bLooking; leave bWasInside and
 // outside it. Why: what does not hold. True for a trigger with no place and no point.
 SIGNAL47_API bool ModeHolds(const FS47ScriptTrigger& T,const FS47ScriptNow& Now,FString* Why=nullptr);

 // HUD's self-test: the JSON both ways, the triggers, the night's tiers and the lint (S47ScriptTest.cpp).
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
