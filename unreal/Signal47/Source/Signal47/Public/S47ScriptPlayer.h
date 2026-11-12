#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Script.h"
#include "Misc/DateTime.h"
class US47Main;
class FS47FreeformSession;
class FS47EventBeats;
class FS47ShotCamera;

// DIRECTOR-TOOL-20261109: plays the scripted events (S47Script.h) in Freeform. It reads every script in Content/Director/scripted
// (and the developer's own copies, WriteDir), asks each enabled one every two seconds whether its trigger holds (S47Script::Ready,
// with the clock rule, the lock and the lint), and plays the first that does on its timeline. One script at a time, and nothing
// while another big event holds the night; while one plays, Busy says so to anyone who asks. What it does goes through the systems
// that exist, by their public calls: the bodies (FS47PaleBody), the sky's formations (FS47SkyVisitors), the sound (US47Audio), the
// walkie, the CB and 1420 AM (FS47FieldRadio, FS47Station), the array (AS47Web::PointDishes), the night's signs and the feeder
// (FS47Omen, FS47FreeformSession::SetPowerFault), the screen (US47UI), the findings (FS47FreeformSession::RecordEvidence) and the
// player's view. A menu or a paper holds the timeline where it is.
//
// Owned by the Freeform session (its Begin, Step, Stop and Save); see the integration order in the report. Saved: what has been
// played (once, nightly, cooldown), nothing of a script under way: a Continue starts the night without it.
class FS47ScriptPlayer
{
public:
 explicit FS47ScriptPlayer(US47Main* Main);
 ~FS47ScriptPlayer();
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;

 // ---- the library ----
 void Reload();
 const TArray<FS47Script>& Library() const{return Scripts;}
 // The first thing wrong with a script that is read but cannot play (its lint), by id; or a file that could not be read, by file.
 const TMap<FString,FString>& Refused() const{return Bad;}
 const FS47Script* Find(const FString& Id) const;
 // Lint, then write WriteDir()/<id>.json and read the library again. False with Why: nothing written.
 bool Store(const FS47Script& S,FString& Why);

 // ---- playing ----
 // At once, whatever its trigger and history (the editor's PREVIEW). Refused only by the lint, or by another script playing.
 // Why: also what would hold it back in play (before two, a big event). Nothing of it is counted as played.
 bool Preview(const FS47Script& S,FString& Why);
 void StopRun(const FString& Why=FString());
 bool Running() const{return bRun;}
 FString RunningId() const{return bRun?Run.S.Id:FString();}
 double RunAge() const{return bRun?Run.Age:0;}
 // Why a script would not start now (empty: it would).
 FString WhyNot(const FS47Script& S) const;
 int32 PlayedCount(const FString& Id) const{return Memory.Count.FindRef(Id);}
 // The moment, as a trigger reads it.
 FS47ScriptNow Now() const;
 // DIRECTOR-MODE-01 (slice 3): Base with what this script's trigger volume reads (FS47ScriptNow's bLooking, Held, bWasInside): its
 // look test now, and what its watch has seen so far. Base itself for a trigger without a volume (FS47ScriptTrigger::HasVolume).
 FS47ScriptNow NowFor(const FS47Script& S,const FS47ScriptNow& Base) const;
 // CRIT6-20261109: the weather and the moon a trigger reads, as the world shows them at Clock: the calendar day's (FS47WeatherLive,
 // the sky's moon), not the shift day's, whose morning is still "today" until 08:00. The night's storm is the evening's, the shift
 // day's (ShiftDay), held into the small hours. CalendarDate: that calendar day's date (FS47FreeformSession::DateOfDay).
 static int32 CalendarDayOf(double Clock){return int32(FMath::FloorToDouble(FMath::Max(0.,Clock)/86400.))+1;}
 static void SkyNow(int32 Seed,double Clock,int32 ShiftDay,const FDateTime& CalendarDate,FS47ScriptNow& N);
 // Where the player stands, or the ground they look at (within 400 m), road-local. False: no player on the land.
 bool HereRoad(FS47ScriptPoint& Out) const;
 bool LookRoad(FS47ScriptPoint& Out) const;
 // DIRECTOR-MODE-01 (S47DirectorCam.h): LookRoad's trace from any eye (scene metres; the view's yaw and pitch, radians), Up over the
 // ground and never below it; and a scene point as a road-local point with Up over the floor there as it is (below 0 under the ground:
 // the caller decides). False: not on the land of the area the player is in (indoors), or nothing within 400 m.
 static bool RayRoad(US47Main* InM,const FVector& Eye,double Yaw,double Pitch,FS47ScriptPoint& Out);
 static bool RoadOfWorld(US47Main* InM,const FVector& Three,FS47ScriptPoint& Out);
 // WorldOf for anyone: a road-local point in the area the player is in now, three-space, Up over its ground.
 static FVector WorldOfRoad(US47Main* InM,const FS47ScriptPoint& P);

 // ---- for everyone else ----
 // The session's player, or null outside Freeform.
 static FS47ScriptPlayer* Of(const US47Main* M);
 // A script is playing now: nothing else big should start. Asked by the night's rhythm (FS47Rhythm::ReadNow: "script" holds the
 // ground), the Director's RestBlock, the big night (StepBigDue), the gear (FS47Outfit) and saving (S47SaroSave::Danger); being
 // taken stops it (FS47Taken).
 static bool Busy(const US47Main* M);
 // Another big event holds the night: the Director's event, being taken, a night happening on the ground, the light over the
 // highway, or a lock someone holds (Hold). Why: which.
 static bool BigEvent(const US47Main* M,FString& Why);
 // The simple lock: while any holder has it, no script starts. Who: a name for the log ("finale", "test").
 static void Hold(const FString& Who,bool bOn);
 // The editor and its F10 lines are only there in a developer's run (or with -S47Director), never in a shipping build.
 static bool DevAllowed(const US47Main* M);
 // Where scripts are read from, in order (a later one with the same id wins), and where the editor writes: the project's Content
 // in the editor, the game's Saved/Director/scripted otherwise; -S47ScriptDir=<path> for either (the repository's own folder).
 static TArray<FString> ReadDirs();
 static FString WriteDir();
 // D46-20261110 (D46-11): the event scripts' runner (S47EventScript.h), stepped and stopped with this one.
 FS47EventBeats* EventBeats() const{return Beats.Get();}
private:
 US47Main* M=nullptr;
 FS47FreeformSession* Session() const;
 TArray<FS47Script> Scripts;
 TMap<FString,FString> Bad;
 FS47ScriptMemory Memory;
 double CheckIn=4;
 // DIRECTOR-MODE-01 (slice 3): each enabled volume trigger's watch, by script id: how long its mode's test has held and whether the
 // player has been inside. Not saved: a Continue starts every watch fresh, and so does any gap in the asking (a script playing, the
 // tutorial). SincePoll: the seconds since the last ask.
 struct FVolumeWatch{double Held=0;bool bHolding=false,bWasInside=false;};
 TMap<FString,FVolumeWatch> Watch;
 double SincePoll=0;
 FS47ScriptNow Watched(const FS47Script& S,const FS47ScriptNow& Base,double Gap);
 bool bLinesAdded=false;
 void AddDevLines();
 // The run.
 struct FRun
 {
  FS47Script S;
  double T=0,Age=0,Hold=0;
  int32 Next=0;
  bool bHoldLook=false,bPreview=false;
  FS47ScriptPoint HoldAt;
 } Run;
 bool bRun=false;
 bool Start(const FS47Script& S,bool bPreview,FString& Why);
 void Advance(double Dt);
 void Fire(const FS47ScriptAction& A);
 void Finish(const FString& Why);
 // What keeps going after an action has fired.
 struct FBodies;
 TUniquePtr<FBodies> Bodies;
 struct FFigure
 {
  int32 Body=0,Seg=1;
  TArray<FS47ScriptPoint> Path;
  double RoadX=0,RoadZ=0,Y=0,Yaw=0,Speed=1,Stand=6,Clock=0,Seen=0;
  bool bVanish=false,bLive=false,bGround=false;
 };
 TArray<FFigure> Figures;
 void StepFigure(FFigure& F,double Dt);
 struct FLook{FVector Target=FVector::ZeroVector;double Left=0,Total=1,FromYaw=0,FromPitch=0;bool bLive=false;} Look;
 void StepLook(double Dt);
 // bBigTurned: the big night had turned the array already when the targets were taken (CRIT6-20261109: one that turns it after
 // that is not undone when the hold ends).
 struct FDishHold{TArray<FVector2D> Targets;double Left=0;bool bLive=false,bBigTurned=false;} DishHold;
 // Put the antennas back where the script found them, unless the big night has turned them since, or is under way.
 void RestoreDishes();
 bool bSkyStarted=false,bOmenPosted=false;
 // How long the formation and the posted sign still last: the script is not over before they are.
 double SkyLeft=0,OmenLeft=0;
 bool EffectsLive() const;
 void ClearEffects();
 // A road-local point in the area the player is in now, three-space, on the ground and Up over it.
 FVector WorldOf(const FS47ScriptPoint& P) const;
 FVector EyeNow() const;
 // The player looks at this, within Degrees.
 bool LookingAt(const FVector& At,double Degrees) const;
 static TSet<FString>& Holders();
 TUniquePtr<FS47EventBeats> Beats;
 // SHOT-BEAT-01: the shot action's camera (S47Shot.h), made on the first shot; stopped with the run's other effects (ClearEffects).
 TSharedPtr<FS47ShotCamera> Shot;
};
