#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47EventScript.h"
class US47Main;
class FS47StoryScene;

// STORY-ON-FREEFORM-01, step 3 (Docs/Design/Story/STORY_PA_FREEFORM_2026-11-11.md): the story director's skeleton. NEW STORY is a
// Freeform session marked as Story (US47Flow::StoryNight); this sits on top of it and does nothing in any other session. It holds a
// short timeline of acts (Content/Director/Story/acts.json: id, title, from "HH:MM" of the night, optional needs, a list of story
// flags), the act the night is in, and the story's flags. It reads the Freeform clock; only a clock beat (step 4) moves it, forward. The
// beats that play inside the acts are step 4 onwards.
//
// The acts' clock is the night's hour as FS47Happenings::NightHour has it: 08:00 to 07:59 the next morning, as 8 to 32 (00:00 is 24).
// An act starts when the night's hour has reached its "from" and every flag it needs is set; acts only go forward, one after another.
// The save is {v:1, act:"<id>", flags:[...]}; an unknown version, or no save, starts at the first act. A missing or invalid acts.json
// leaves the director inactive with a log line.
//
// Step 4: the story's scripts (Content/Director/Story/beats/*.json) are event scripts (S47EventScript.h, the same grammar), read by this
// store only and checked by S47EventScript::LintStory, which lets the story-only beats (flag, start, clock, objective) and conditions
// (story_flag, no_story_flag) through and checks their text with LintStory. Freeform's folders never read them, and Freeform's lint
// refuses those beats and conditions by name. They run on this director's own FS47EventBeats (the runner Freeform's events use, its
// own instance), stepped in Tick; an act's "scripts" start when the act begins. While one runs (or waits to start, or a clock beat
// waits) Holds is true: the Freeform Director's dice, OneAtATime's visitors and new director scripts (FS47ScriptPlayer::Hold "story")
// wait. The save adds objective, scripts [{id, t, next}] (the runs under way) to {v:1, act, flags}; a save without them is read as none.
//
// Story's text has its own check (LintStory). It may carry the story's own spoilers, which Freeform's S47Script::LintText refuses;
// Freeform's check is never loosened for it, and story text never goes into Freeform's folders.
class FS47StoryDirector
{
public:
 struct FAct
 {
  FString Id;
  FString Title;
  double From=0;          // the night's hour (8 to 32) the act may start at
  TArray<FString> Needs;  // story flags that must all be set
  TArray<FString> Scripts;// step 4: story scripts started when the act begins
 };
 // The longest story text LintStory lets through.
 static constexpr int32 MaxStoryText=600;
 // A flag's or an act's id: 1 to MaxIdLen of a-z, 0-9 and _.
 static constexpr int32 MaxIdLen=48;

 explicit FS47StoryDirector(US47Main* Main);
 ~FS47StoryDirector();

 // Content/Director/Story/acts.json.
 static FString ActsPath();
 // "HH:MM" to the night's hour (before 08:00 counts as after midnight: 24 and over). False for anything else.
 static bool ParseFrom(const FString& Text,double& OutHour);
 // The hour of the night (8 to 32) of a Freeform clock.
 static double NightHourOf(double Clock);
 // The acts of an acts.json text, in order. False (with why) when any act is wrong: then none are used.
 static bool ParseActs(const FString& Text,TArray<FAct>& Out,FString& Why);
 static bool ValidId(const FString& Id);
 // Story's own text check: empty, too long, unprintable, or a name the game must not use. Empty when the text is fine.
 static TArray<FString> LintStory(const FString& Text);

 // ---- step 4: the story's scripts ----
 // Content/Director/Story/beats; every *.json in it, by name.
 static FString ScriptsDir();
 // Read and story-lint one script's text. False with why (the first thing wrong).
 static bool ReadScript(const FString& Text,FS47EventScript& Out,FString& Why);
 static void ReloadScripts();
 static const TArray<FS47EventScript>& StoryScripts();
 static const FS47EventScript* FindScript(const FString& Id);
 // Files that were not read or will not play, and why (logged as they are refused).
 static const TMap<FString,FString>& RefusedScripts();
 // The director of a Story night, or null: no director, not active, or not a Story night. Everything of step 4 asks this first.
 static FS47StoryDirector* Of(const US47Main* Main);
 // A story script runs, waits to start, or a clock beat waits: Freeform's dice hold. False without a Story night's director.
 static bool Holds(const US47Main* Main);
 // story_flag / no_story_flag. Without a Story night's director no story flag is set: story_flag false, no_story_flag true.
 static bool CondHolds(const US47Main* Main,const FS47ScriptCond& C);
 bool FlagCond(const FS47ScriptCond& C) const;
 // The story's line on the HUD's objective and the task list; empty without a Story night's director or with no line.
 static FString ObjectiveOf(const US47Main* Main);
 const FString& ObjectiveText() const{return ObjectiveLine;}
 // Seconds to push Clock forward to the night's hour AtLeast (8 to 32): 0 when tonight has passed it. Never back, never past the
 // night's end at 08:00.
 static double ClockPushFor(double Clock,double AtLeast);
 // The longest a clock beat waits for a closed panel and the player out of the truck, in real seconds; then it is left out.
 static constexpr double ClockWaitMost=5;
 // A story-only beat (flag, start, clock, objective), with its text filled in; called by the runner. What it did, for the log.
 FString Apply(const FS47Beat& B,const FString& Text);
 // Starts a story script on the next tick. False with why: not in the store, or already running or waiting.
 bool StartScript(const FString& Id,FString& Why);
 bool Busy() const;
 // Step 5: the story's telephone call is the player's to pick up (the OPS "phone"); true when it took the use.
 static bool UsePhone(const US47Main* Main);
 FS47StoryScene* SceneHost() const{return Scene.Get();}
 int32 RunningCount() const;
 // F10's group "Story (director)", shown on a Story night only (FS47DevEvents::AddStory).
 void AddDevLines();

 // STORY-DRIVE-20261112 (Tom 12 Nov: Story still begins in the car on the way to SARO): where a new Story night begins. In the truck
 // on the highway at the south end of Caliche, heading north for SARO: the old Story's drive start (US47World::BeginCommute: the road
 // leg, x 8001.8, z TOWN.Z1+35, heading 0), driven on Freeform's own trip and truck. The evening act's evening_drive takes it from there.
 struct FDrive{FString Leg,To;FVector At=FVector::ZeroVector;double Heading=0;};
 static FDrive DriveStart();
 // US47Flow::StartGame on a new Story night (not a Continue): starts that trip (US47World::StartTrip). False when it does not: the
 // world is busy or driving already, or -S47StoryAtSaro (the night then starts at SARO, as before).
 static bool BeginDrive(US47Main* Main);

 // Reads acts.json and takes the saved act and flags (Saved may be null: a new night at the first act).
 void Begin(const S47J::FObj& Saved);
 // Begin with the acts from this text in place of the file (Begin's own and the self-test's).
 void BeginWith(const FString& ActsText,const S47J::FObj& Saved);
 void Stop();
 bool Active() const{return bActive;}
 // Moves to the next act when its hour has come and its needs are set (as many as are due).
 void Tick(double Dt);
 // {v:1, act:"<id>", flags:[...], objective, scripts:[{id,t,next}]}; always with v:1, so a Story save stays a Story save.
 S47J::FObj Save() const;

 bool SetFlag(const FString& Flag);
 bool ClearFlag(const FString& Flag);
 bool HasFlag(const FString& Flag) const{return Flags.Contains(Flag);}
 TArray<FString> FlagList() const;
 // The act the night is in; null when inactive.
 const FAct* CurrentAct() const{return bActive&&ActList.IsValidIndex(ActIndex)?&ActList[ActIndex]:nullptr;}
 int32 CurrentIndex() const{return ActIndex;}
 const TArray<FAct>& Acts() const{return ActList;}
 FString StateText() const;

 // The self-test's clock in place of the Freeform session's (seconds, as FS47FreeformSession::Clock); below zero: the session's.
 void TestClock(double Clock){ClockOverride=Clock;}
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 TArray<FAct> ActList;
 int32 ActIndex=-1;
 TSet<FString> Flags;
 bool bActive=false;
 double ClockOverride=-1;
 bool ClockNow(double& Out) const;
 // True when the save had the act the night is in (the act's scripts have started already).
 bool Load(const S47J::FObj& Saved);
 // Step 4.
 TUniquePtr<FS47EventBeats> Runner;
 TUniquePtr<FS47StoryScene> Scene; // step 5: the scene beat's host (S47StoryScene.h)
 struct FPending{FString Id;double T=0;int32 Next=0;};
 TArray<FPending> Pending;     // started on the next tick: from an act, a start beat, F10 or a save
 FString ObjectiveLine;
 double ClockAt=-1,ClockWaited=0;
 bool bHeld=false;            // FS47ScriptPlayer::Hold("story") is on
 void QueueAct(int32 Index);
 void StartPending();
 void StepClock(double Dt);
 void SetHold(bool bOn);
};
