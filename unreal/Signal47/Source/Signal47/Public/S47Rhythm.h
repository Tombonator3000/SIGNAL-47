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
 int32 FromDay=1,CoolMin=-1,CoolMax=-1,MinNights=0;
 TArray<FString> Areas;
 TArray<FName> Needs;
};
struct FS47RhythmData
{
 double Earliest=1,Latest=3,End=6,DevFixed=2,TickMin=240,TickMax=420,RestDanger=480,RestOther=45,RestTaken=1800;
 int32 MinorMin=2,MinorMax=4,InvestMin=0,InvestMax=2,SkyMin=0,SkyMax=1,DangerPerNight=-1; // -1: no limit
 TArray<FS47RhythmEvent> Events;
 const FS47RhythmEvent* Find(FName Id) const{return Events.FindByPredicate([Id](const FS47RhythmEvent& E){return E.Id==Id;});}
};

// Everything May looks at, read before the question. Pure data, so that the same answer can be had from a test.
struct FS47RhythmNow
{
 bool bFreeform=true,bTutorial=false,bTheirHours=false,bQuiet=false;
 int32 Day=1,MinorLeft=0,InvestLeft=0,SkyLeft=0,DangerTonight=0;
 double Clock=0,RestLeft=0;
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
 // budget, danger_night, not_here, needs:<name>, not_drawn_tonight. Need answers for a name; unset: every need holds.
 FS47Verdict May(FName Id,const FS47RhythmNow& Now,TFunction<bool(FName)> Need=nullptr) const;
 // The world's answers, from what each system lets be read.
 FS47RhythmNow ReadNow() const;
 // A system's own question, at the moment it would start, after its dice. True when nothing stops it or its "enforce" is off; with it off,
 // what would have stopped it goes to the log. Fixtures on a developer run's fixed rules are not asked.
 // Within: the event that this one is part of (the car whose light it is), whose ground does not count against it either.
 bool Allow(FName Id,FName Within=NAME_None);
 // It has started, or is over: a danger is counted for the night, and the rest after it runs for everyone.
 void Began(FName Id);
 void Ended(FName Id);
 // The night's chance from the file, with the code's own number if the file does not have it.
 float PerNight(FName Id,float Otherwise) const;
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
};
