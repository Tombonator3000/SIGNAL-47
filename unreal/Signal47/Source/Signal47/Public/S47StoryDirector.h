#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;

// STORY-ON-FREEFORM-01, step 3 (Docs/Design/Story/STORY_PA_FREEFORM_2026-11-11.md): the story director's skeleton. NEW STORY is a
// Freeform session marked as Story (US47Flow::StoryNight); this sits on top of it and does nothing in any other session. It holds a
// short timeline of acts (Content/Director/Story/acts.json: id, title, from "HH:MM" of the night, optional needs, a list of story
// flags), the act the night is in, and the story's flags. It reads the Freeform clock and never moves it. No story content yet: the
// beats that play inside the acts are step 4 onwards.
//
// The acts' clock is the night's hour as FS47Happenings::NightHour has it: 08:00 to 07:59 the next morning, as 8 to 32 (00:00 is 24).
// An act starts when the night's hour has reached its "from" and every flag it needs is set; acts only go forward, one after another.
// The save is {v:1, act:"<id>", flags:[...]}; an unknown version, or no save, starts at the first act. A missing or invalid acts.json
// leaves the director inactive with a log line.
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
 };
 // The longest story text LintStory lets through.
 static constexpr int32 MaxStoryText=600;
 // A flag's or an act's id: 1 to MaxIdLen of a-z, 0-9 and _.
 static constexpr int32 MaxIdLen=48;

 explicit FS47StoryDirector(US47Main* Main):M(Main){}

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

 // Reads acts.json and takes the saved act and flags (Saved may be null: a new night at the first act).
 void Begin(const S47J::FObj& Saved);
 // Begin with the acts from this text in place of the file (Begin's own and the self-test's).
 void BeginWith(const FString& ActsText,const S47J::FObj& Saved);
 void Stop();
 bool Active() const{return bActive;}
 // Moves to the next act when its hour has come and its needs are set (as many as are due).
 void Tick(double Dt);
 // {v:1, act:"<id>", flags:[...]}; always with v:1, so a Story save stays a Story save.
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
 void Load(const S47J::FObj& Saved);
};
