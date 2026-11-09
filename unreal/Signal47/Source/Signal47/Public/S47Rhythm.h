#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"

class US47Main;

// The rhythm of a Freeform night as data (Content/Director/director.json; Docs/Design/Director/DIRECTOR-FORSLAG.md, Tom 7 November): one set of
// rules for "may this start now, and if not, why". May has no dice and changes nothing. Each system keeps its own dice and what it
// does. Nothing asks May to decide yet unless its "enforce" says so, and until that is wired it only says what it would have done.
struct FS47Verdict { bool bMay=true; FName Why=NAME_None; FString Detail; double RetryClock=-1; };

enum class ES47RhythmLevel:uint8{Hint,Observation,Presence,Danger};
enum class ES47RhythmHours:uint8{Any,Night,Theirs,Window};

struct FS47RhythmEvent
{
 FName Id;
 FString System,Budget,DrawState;
 bool bGround=true,bEnforce=false,bFixedTable=false;
 ES47RhythmLevel Level=ES47RhythmLevel::Hint;
 ES47RhythmHours Hours=ES47RhythmHours::Any;
 double From=0,To=0,PerNight=-1;
 // draw.at: the hours of the night it is drawn in, 19 to 30 (past midnight over 24); -1: the system's own.
 double DrawFrom=-1,DrawTo=-1;
 int32 FromDay=1,CoolMin=-1,CoolMax=-1,MinNights=0;
 TArray<FString> Areas;
 TArray<FName> Needs;
};
// The build-up over the game (Tom's critics, 8 November: the chances were flat from the first night to the thirtieth): one row a week,
// from its day. What each level of a night event is worth that week (a factor on its per_night), how likely the night's one
// danger slot is (FS47Happenings), and how near things come (a factor on the distances they keep). The last row holds to the end.
struct FS47RhythmWeek{int32 FromDay=1;float Hint=1,Observation=1,Presence=1,Danger=.35f;double Near=1;};
struct FS47RhythmData
{
 // RestDanger: a pause after a danger, not the rest of the night (Tom's critics, 8 November: was 480 s, two hours of the clock).
 double Earliest=1,Latest=3,End=6,DevFixed=2,TickMin=240,TickMax=420,RestDanger=170,RestOther=45,RestTaken=1800;
 int32 MinorMin=2,MinorMax=4,InvestMin=0,InvestMax=2,SkyMin=0,SkyMax=1,DangerPerNight=-1; // -1: no limit
 TArray<FS47RhythmEvent> Events;
 TArray<FS47RhythmWeek> Weeks; // in order of FromDay; empty: every level at its own chance, everything as near as it is
 // The moon (MOON-20261108, globals.moon): what a night's levels and its danger are worth at a full moon and at a new one, the
 // nights between in proportion to the share of the moon lit. 1 and 1: the moon changes nothing. LitOf: the share lit on a shift
 // day's night (the game sets it; unset, a pure parse: no moon).
 float MoonFull=1,MoonNew=1;
 TFunction<double(int32)> LitOf;
 float MoonOn(int32 Day) const;
 // WEATHER-20261108: a night the wind holds into (S47Weather::Day().NightStorm, 0 to 1) is a little busier: what comes near and the
 // danger are worth up to StormNight times as much. StormOf: the game sets it; unset, no weather.
 float StormNight=1.15f;
 TFunction<double(int32)> StormOf;
 float StormOn(int32 Day) const;
 const FS47RhythmEvent* Find(FName Id) const{return Events.FindByPredicate([Id](const FS47RhythmEvent& E){return E.Id==Id;});}
 // This week's row with the night's moon in it (a copy).
 FS47RhythmWeek Week(int32 Day) const;
 float Worth(ES47RhythmLevel Level,int32 Day) const;
};

// Everything May looks at, read before the question. Pure data, so that the same answer can be had from a test.
struct FS47RhythmNow
{
 bool bFreeform=true,bTutorial=false,bTheirHours=false,bQuiet=false;
 // Something humorous came tonight (FS47Happenings): Tom, 8 November, never on the same night as a danger.
 bool bHumorTonight=false;
 int32 Day=1,MinorLeft=0,InvestLeft=0,SkyLeft=0,DangerTonight=0;
 double Clock=0,RestLeft=0;
 // Since the last start of anything, and how long the one after it waits (FS47Rhythm::Began).
 double SinceStart=1e9,StartGap=0;
 FString Area;
 FName Busy=NAME_None; // what holds the ground now
 TMap<FName,int32> LastDays;
 // draw state -> 1 drawn for tonight, 0 thrown and not drawn. Absent: not thrown yet.
 TMap<FString,int32> Draws;
};

class SIGNAL47_API FS47Rhythm
{
public:
 explicit FS47Rhythm(US47Main* Main):M(Main){}
 // From the packaged file, or what is embedded if that is missing or broken. An override only with -S47DirectorOverride.
 void Load();
 void LoadRules();
 // F10, in a test run: Saved/Director/director.override.json, asked for by hand. False: why not, in Why.
 bool LoadOverride(FString& Why);
 bool OverrideOn() const{return bOverride;}
 const FString& Source() const{return SourceName;}
 const FS47RhythmData& Data() const{return Rules;}
 static bool Parse(const FString& Json,FS47RhythmData& Out,FString& Why);
 // Days before an event comes again: from Min to Max (cooldown_days), always the same answer for one event's one last day. The
 // Director asks this too (FS47FreeformDirector::CoolFor), so that the panel and the night agree.
 static int32 CoolDays(const FString& Id,int32 Last,int32 Min=2,int32 Max=4);
 // Fixed order, so that the panel and the log say the same: off, tutorial, too_early_day, outside_hours, cooldown, slot_taken:<id>, rest,
 // budget, danger_night, humor_night, not_here, needs:<name>, not_drawn_tonight. Need answers for a name; unset: every need holds.
 FS47Verdict May(FName Id,const FS47RhythmNow& Now,TFunction<bool(FName)> Need=nullptr) const;
 // The world's answers, from what each system lets be read.
 FS47RhythmNow ReadNow() const;
 // A system's own question, at the moment it would start, after its dice. True when nothing stops it or its "enforce" is off; with it off,
 // what would have stopped it goes to the log. Fixtures on a developer run's fixed rules are not asked.
 // Within: the event that this one is part of (the car whose light it is), whose ground does not count against it either.
 bool Allow(FName Id,FName Within=NAME_None);
 // It has started, or is over: a danger is counted for the night, and the rest after it runs for everyone.
 // Every start is also the one common log of starts (Tom's critics, 8 November): after one, the next waits two to six minutes of the
 // clock, whatever system it is, so that nothing comes in a heap when a rest lets go. bMet false: a danger nobody met, the short rest.
 void Began(FName Id);
 void Ended(FName Id,bool bMet=true);
 // The clock of the last start of anything, for those that keep their own distance from it too (FS47Happenings' unease).
 double LastStart() const{return LastStartClock;}
 // The night's chance from the file, with the code's own number if the file does not have it. With a day, a night event's chance
 // is also worth what its level is worth that week (the curve), never above one.
 float PerNight(FName Id,float Otherwise,int32 Day=0) const;
 // The hours of the night it is drawn in (draw.at), 19 to 30; From and To untouched when the file has none.
 bool DrawHours(FName Id,double& From,double& To) const;
 bool NeedHolds(FName Name) const;
 // Watches who starts while the panel or a log is on, for the log: what started, and what the rules would have said just before it.
 void Observe(double Dt);
 void SetWatching(bool bOn);
 bool Watching() const{return bWatching;}
 FString PanelText(bool bFull) const;
 void LogPanel() const;
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 FS47RhythmData Rules;
 bool bOverride=false,bWatching=false,bWatchInit=false;
 FString SourceName;
 double Since=1;
 FS47RhythmNow Before;
 TMap<FName,bool> WasOn;
 // A night's tally for its line in the log, and what the panel has to say about the last few.
 struct FTally{int32 Night=0,Hint=0,Observation=0,Presence=0,Danger=0,WouldStop=0,Minor=0,Invest=0,Sky=0;double FirstClock=-1,LastClock=-1;};
 FTally Tally;
 TArray<FString> Recent;
 void Observed(FName Id,const FS47RhythmNow& At);
 TMap<FName,FName> ShadowSaid;
 void CloseNight(int32 NewNight);
 double LastStartClock=-1e9,StartGap=0;
};
