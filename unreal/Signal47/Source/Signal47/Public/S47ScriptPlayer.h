#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Script.h"
class US47Main;
class FS47FreeformSession;

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
 // Where the player stands, or the ground they look at (within 400 m), road-local. False: no player on the land.
 bool HereRoad(FS47ScriptPoint& Out) const;
 bool LookRoad(FS47ScriptPoint& Out) const;

 // ---- for everyone else ----
 // The session's player, or null outside Freeform.
 static FS47ScriptPlayer* Of(const US47Main* M);
 // A script is playing now: nothing else big should start (FS47FreeformDirector and FS47Happenings may ask).
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
private:
 US47Main* M=nullptr;
 FS47FreeformSession* Session() const;
 TArray<FS47Script> Scripts;
 TMap<FString,FString> Bad;
 FS47ScriptMemory Memory;
 double CheckIn=4;
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
 struct FDishHold{TArray<FVector2D> Targets;double Left=0;bool bLive=false;} DishHold;
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
};
