#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47NightTiers.h"
#include "S47Script.h"
class US47Main;
struct FS47CameraSnap;
class FS47CameraMoment;

// D46-20261110 (D46-11, Tom 10 November: "mer detaljerte manus for alle hendelser, så man mer kan styre hva som skal skje i en
// hendelse"). An event script is one JSON file per event, Content/Director/Events/<id>.json, that says what happens inside an
// event the game already has: its beats, in order, at seconds from its start or when a condition holds, and the gates that may
// hold it back. Guide: Docs/Active/Guides/EVENT_SCRIPTS.md.
//
// What stays where it was: director.json (and S47RhythmDefaults.inl) says which events there are, when, how often and how near
// (the rhythm); the event's own code says whether it can start where the player is and decides what only it can know (the place,
// the bearing, the truck). The script then says what the player hears, sees and keeps, through the functions the events used
// before (US47Audio, US47UI, FS47FieldRadio, FS47Omen, FS47Places, AS47Web, FS47FreeformSession::RecordEvidence). No new mechanics.
//
// The code gives the script words ({az}, {clock}) and named points ("truck") in an FS47EventCtx; a word the code does not give is
// left as it is written. Every wired event has its script built in (S47EventScriptDefaults.inl, the same text as the file in
// Content), so a missing or broken file never leaves the event without its beats: a file that is read and passes the lint wins.
// Read from, in order: the built-in copies, Content/Director/Events, Saved/Director/Events, -S47EventDir=<path> (a later one wins).
//
// Beats (S47EventScript::BeatNames; the guide has every field):
//   sound    a recording (sound), placed (pos, point) or through a set (who: radio, cb, walkie); gain, rate, lp, delay, stop
//   tone     a plain tone (hz, secs, gain, wave), repeat times every seconds, after delay
//   printer  the rack printer runs (gain, delay, stop); text: its line on the screen
//   light    kind town (Caliche's street lamps go out one by one), lamp (the flashlight flickers for secs), gear (the lamp, the
//            walkie, the truck and the screens answer a sign of strength for secs, through FS47Omen)
//   tv       the hiss of a set (pos, point); with strength and secs the screens break up as for gear
//   line     who dale, voice (the walkie), cb, radio (1420 AM); text
//   thought  the player's own thought (US47UI::Thought); toast: a plain line on the screen; both text and secs
//   evidence key and text, a finding (kept only in play and in a developer's run)
//   map      place: the place is heard of, on the map (FS47Places::Hear)
//   prop     node (an AS47Web key, "room.objs.mug"): moved by [x, y, z] metres, or to, with "to": true. Not saved.
//   show     node shown; hide: node hidden. Not saved.
//   shake    the view shakes a little (strength 0 to 1, the walker's own)
//   silence  N1110 (SOUNDSCAPE): the night's crickets and wind stop at once for secs (level: what is left, 0.04 as made) and come back slowly;
//            put it a few seconds before the sight, as the warning. Heard on the night's own bed only (FS47Ambience::Hush).
//   tube     N1110 (PLAY-31-34): tube N (2 or 3) of the control room's ceiling tubes sparks and dies for good in this game (saved, kept through
//            Continue and a change of area; the working light, tubes 0 and 1, is never touched). Not kept in an F10 run.
//   camera_snap  N1110: the view snaps toward pos/point and closes in to fov degrees over in seconds, holds hold seconds and eases
//            back over out (FS47CameraMoment); shake 0 to 1 as it lands; scare: a bought hit with it (soft, hard, braam, swell).
// Any beat: "if": conditions that must hold when its time comes (else it is left out); "wait": conditions it waits for, at most
// "max" seconds (else it is left out). Conditions are the scripted events' (S47Script::CondKinds), and in_saro, area.
// Gates (all optional): from_day, to_day, hours [from, to] (clock hours, from > to past midnight), tier (hint, glimpse, full: the
// least the night's tier must allow at that hour, S47NightTiers), early, areas (the scene's area names), odds (0 to 1, the game's
// seed, the same each night it is asked), if (conditions). Asked by the night's happenings (FS47Happenings::StartEvent) before the
// rhythm, never for an F10 start.

enum class ES47Beat : uint8 { Sound, Tone, Printer, Light, Tv, Line, Thought, Toast, Evidence, Map, Prop, Show, Hide, Shake, CameraSnap, Silence, Tube, Count };

struct FS47Beat
{
 ES47Beat Do=ES47Beat::Toast;
 double At=0;                 // seconds from the event's start
 // Name: the recording, the node or the place; Kind: light's kind, tone's wave; Who: the set; Key: the finding; Point: a named point.
 FString Name,Kind,Who,Text,Key,Point;
 TOptional<FVector> Pos;      // scene metres in the area the player is in (three-space: x east, y up, z south)
 FVector By=FVector::ZeroVector;
 bool bTo=false;
 double Gain=1,Rate=1,Lp=0,Delay=0,Stop=0,Secs=0,Hz=1200,Every=0,Strength=.5,Max=0;
 double Fov=45,In=.25,Hold=1.2,Out=.9; // N1110: camera_snap's degrees and seconds (its shake is Strength, its scare is Kind)
 int32 Repeat=1;
 int32 Tube=-1;                 // N1110: the tube beat's index into the control room's ceiling tubes (2 to 7)
 int32 Next=0;                  // N1110: or, instead of a tube, how many of the next still-working ones go out (1 to 6): {"beat":"tube","next":2}
 TArray<FS47ScriptCond> If,Wait;
};

struct FS47EventGates
{
 int32 DayFrom=0,DayTo=0;
 double HourFrom=-1,HourTo=-1;
 bool bTier=false,bEarly=false;
 ES47Allowed Tier=ES47Allowed::Hint;
 TArray<FString> Areas;
 double Odds=1;
 TArray<FS47ScriptCond> If;
 bool Any() const{return DayFrom>0||DayTo>0||HourFrom>=0||bTier||Areas.Num()>0||Odds<1||If.Num()>0;}
};

struct FS47EventScript
{
 static constexpr int32 Version=1;
 FString Id,Name,Notes;
 bool bEnabled=true;          // off: the event plays its built-in beats, as if the file were not there
 FS47EventGates Gates;
 TArray<FS47Beat> Beats;      // in the order of At (the same At keeps the order written)
 FString Source;              // "built in" or the file; not written
};

// What an event's code knows as it starts: the words for the texts and the points in the scene.
struct FS47EventCtx
{
 TMap<FString,FString> Words;
 TMap<FString,FVector> Points;
 bool bKeep=true;             // what it records is kept (an F10 run in a real game keeps nothing)
};

namespace S47EventScript
{
 SIGNAL47_API const TCHAR* BeatName(ES47Beat B);
 SIGNAL47_API bool BeatFrom(const FString& Name,ES47Beat& Out);
 SIGNAL47_API const TArray<FString>& BeatNames();
 // False with Why when it is not an event script of this version, or a beat or a condition is not known (by name).
 SIGNAL47_API bool Read(const FString& Text,FS47EventScript& Out,FString& Why);
 // Everything wrong with it (its words, its numbers, what a beat needs). Warnings: what is not wrong but will not do what it says
 // (a word the event does not give, an id no event's code plays: it runs from F10 only).
 SIGNAL47_API TArray<FString> Lint(const FS47EventScript& S,TArray<FString>* Warnings=nullptr);
 // {word} by the words; unknown ones are left as written.
 SIGNAL47_API FString Fill(const FString& Text,const TMap<FString,FString>& Words);
 // Pure: whether the gates hold. Roll: this game's 0 to 1 for this event and night (odds); Area: the scene's area.
 SIGNAL47_API bool GatesHold(const FS47EventGates& G,const FS47ScriptNow& Now,const FString& Area,double Roll,FString& Why,TFunction<bool(const FS47ScriptCond&)> Extra=nullptr);
 // The events whose code plays its script, and the words and points each gives (besides clock, day and night).
 SIGNAL47_API const TArray<FString>& Wired();
 SIGNAL47_API const TArray<FString>& WordsOf(const FString& Id);
 SIGNAL47_API const TArray<FString>& PointsOf(const FString& Id);
 // The built-in copies, id and JSON text.
 SIGNAL47_API const TArray<TPair<FString,FString>>& BuiltIn();

 // ---- the library ----
 SIGNAL47_API void Reload();
 SIGNAL47_API TArray<FString> ReadDirs();
 SIGNAL47_API const TArray<FS47EventScript>& Library();
 SIGNAL47_API const FS47EventScript* Find(const FString& Id);
 // Files that could not be read or will not play, and why (the built-in copy, or the earlier file, plays instead).
 SIGNAL47_API const TMap<FString,FString>& Refused();

 // ---- in play ----
 // Asked by an event's code before the rhythm: false with Why when the script's gates hold it back. No script, no gates: true.
 SIGNAL47_API bool Allows(US47Main* M,const FString& Id,FString& Why);
 // Plays the event's script now (its first beats at once, before this returns). False: no script (switched off, or none), or no
 // runner here (not Freeform): the caller has nothing else to play.
 SIGNAL47_API bool Play(US47Main* M,const FString& Id,const FS47EventCtx& Ctx);
 // N1110: a camera moment outside any script (the attention system, a visit at the door): the same as the camera_snap beat, with
 // the same settings. False with Why: no runner here, or FS47CameraMoment::Start's reasons.
 SIGNAL47_API bool CameraSnap(US47Main* M,const FS47CameraSnap& Snap,FString& Why);

 // HUD's self-test (S47EventScriptTest.cpp).
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}

// Plays the beats. Owned by the session's script player (FS47ScriptPlayer, which steps it); a menu or a paper holds it where it is.
// Nothing of a run is saved: what a beat records is (a finding, a place heard of), what it shows for a while is not.
class SIGNAL47_API FS47EventBeats
{
public:
 explicit FS47EventBeats(US47Main* Main):M(Main){}
 bool Start(const FS47EventScript& S,const FS47EventCtx& Ctx);
 void Step(double Dt);
 void Stop();
 int32 Live() const{return Runs.Num();}
 // F10: a line per script (and one to read them again), in any Freeform night; added once each.
 void AddDevLines();
 // F10: this script now, with sample words where the event's code would give them. Why: what it did, or what stopped it.
 bool DevRun(const FString& Id,FString& Why);
 // N1110: the camera moment (S47CameraMoment.h), the beat's and the hook's; made when first asked for, stopped with the runs.
 bool Snap(const FS47CameraSnap& S,FString& Why);
private:
 US47Main* M=nullptr;
 struct FRun{FS47EventScript S;FS47EventCtx Ctx;double T=0,Waited=0;int32 Next=0;};
 TArray<FRun> Runs;
 TSharedPtr<FS47CameraMoment> Camera;
 // The flashlight's flicker, given back after its seconds.
 double LampLeft=0;
 TSet<FString> LinesFor;
 bool Advance(FRun& R,double Dt);
 void Fire(const FS47Beat& B,const FRun& R);
 bool Holds(const TArray<FS47ScriptCond>& Conds) const;
 bool PosOf(const FS47Beat& B,const FRun& R,FVector& Out) const;
};
