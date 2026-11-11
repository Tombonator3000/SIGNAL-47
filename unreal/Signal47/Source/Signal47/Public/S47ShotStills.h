#pragma once
#include "CoreMinimal.h"
#include "S47Shot.h"
class US47Main;

// DIRECTOR-MODE-01, stills (11 November). An author who writes scripted events as JSON (S47Script.h) cannot see the world. For one
// script this writes pictures and a text sheet, so that every shot and the trigger's volume can be seen without playing the night:
//
//   -S47Freeform -S47Director -S47ShotStills=<output dir> -S47ShotStillsScript=<script id, or an absolute path to a .json>
//   [-S47ShotStillsClock=<HH:MM>, default 22:00]
//
// Once the Freeform session is up, it switches director mode on (S47DirectorCam: the night held, no script, no dice), pushes the
// clock forward to the hour asked for (FS47FreeformSession::TestAdvance, as the developer tools do: never back), puts the director's
// camera exactly where each shot's pose is (S47Shot::PoseAt) at its first, middle and last frame (the first only for a shot that
// neither moves nor changes its lens) and takes shot<NN>-a/b/c.png with the director's drawing off; then trigger-top.png and
// trigger-side.png of the trigger's volume with the drawing on. Line traces give each shot a sight report (blocked, by what, how far;
// the eye's height over the ground; under the ground). sheet.md and sheet.json say it all; the run ends with
// "S47: shot stills COMPLETE PASS (<n> pictures)" or "S47: shot stills FAIL: <why>". Nothing of it is reachable without the flag.
namespace S47ShotStills
{
 // ---- pure parts (the self-test holds them) ----
 // "shot01-a.png": the shot's number among the shots (from 1) and its frame (0 a, 1 b, 2 c).
 SIGNAL47_API FString FileName(int32 ShotNumber,int32 Frame);
 // The seconds into the hold that get a picture: 0 for a shot that holds its pose (no move: S.bMove false), else 0, half and Secs.
 SIGNAL47_API TArray<double> FrameTimes(const FS47ShotSpec& S);
 // The height over the middle of a volume of this half-size (metres, the larger horizontal half-extent) from which a camera with
 // this vertical field of view (degrees) sees all of it and 30 % more round it; at least MinTopM.
 inline constexpr double MinTopM=40,Margin=1.3,BlockShare=.8,LowEyeM=.3;
 SIGNAL47_API double TopHeight(double HalfExtent,double FovDeg);
 // The eye Dist metres back along the view (yaw and pitch as the walker's) from the point C it looks at.
 SIGNAL47_API FVector EyeBack(const FVector& C,double Yaw,double Pitch,double Dist);

 // What a line trace found: Dist metres from where it began (below 0: nothing), and what.
 struct FHit { double Dist=-1; FString What; bool Blocked() const{return Dist>=0;} };
 // What is under and over one point.
 struct FGround
 {
  bool bOnLand=false;   // over the land of the area the player is in (S47FarmWorld::FloorRoad)
  double OverLand=0;    // metres over the land there (the range's hills included, which have no collision), below 0: under it
  FHit Below;           // the first thing straight below, within 500 m
  FHit Above;           // the first thing straight above, within 300 m (Dist: how far above)
  bool Under() const{return bOnLand&&OverLand<0;}
  // the clearance a picture sees: the land or what is below, whichever is nearer (no land and nothing below: a large number)
  double Clear() const;
 };
 struct FShotSheet
 {
  int32 Number=0,Action=0;   // among the shots (from 1); its index among the actions (from 0)
  double At=0,Secs=0,Fov=60,Fov2=60;
  bool bPlays=true;          // the script player would start it (an eye and a point, within S47Shot::Sane)
  FString Why;               // why not
  bool bMoves=false,bHasEye2=false;
  TArray<FString> Files;     // the pictures asked for
  TArray<bool> Written;      // each one that is on disk
  FVector Eye=FVector::ZeroVector,Eye2=FVector::ZeroVector,Target=FVector::ZeroVector; // scene metres
  double FromPlayer=-1;      // the first eye from the player's eye at the start, metres
  bool bTraced=false;
  double LookDist=0,LookDist2=0; // eye to the point it looks at; the second eye's
  FHit Look,Look2;           // the first thing on the line from the eye (the second eye) to the point
  FGround EyeG,Eye2G,TargetG;
  FHit Path;                 // something on the straight path from the first eye to the second (either way)
  TArray<FString> Warnings;
 };
 struct FTriggerSheet
 {
  bool bHas=false;
  FString Where;             // "point (x, z)" or "place <id>"
  FString Shape,Mode;
  double X=0,Z=0,Up=0;       // road-local middle
  double HalfX=0,HalfZ=0,Radius=0,Height=0,Extent=0;
  FVector Middle=FVector::ZeroVector; // scene metres, on the ground
  double GroundY=0,Hill=0;   // scene height of the land there; what a hill of the range adds to it (S47Relief::HillLift)
  bool bWalkable=false;      // within the land's rim and no hill too steep or too high to climb (S47Relief::HillStops)
  FHit OnTop;                // what a trace from 300 m above the middle lands on (Dist: that hit's height over the land)
  FString NearPlace;double NearPlaceM=-1;
  int32 NearShot=0;double NearShotM=-1;
  double TopM=0,SideM=0;
  TArray<FString> Files;TArray<bool> Written;
 };
 struct FSheet
 {
  FString Id,Name,Source,Clock,Lint;
  TArray<FShotSheet> Shots;
  FTriggerSheet Trigger;
 };
 // The warnings a shot's numbers call for (the look blocked before BlockShare of the way, an eye under the ground or less than
 // LowEyeM over it, the path between the eyes through something, the look-at under the ground, a shot the game would not play).
 SIGNAL47_API TArray<FString> Warnings(const FShotSheet& S);
 // sheet.md and sheet.json, from a filled sheet.
 SIGNAL47_API FString SheetText(const FSheet& S);
 SIGNAL47_API FString SheetJson(const FSheet& S);
 // Lines "PASS/FAIL STILLS: ..." (-S47SelfTest, from the HUD's).
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);

 // ---- the run ----
 // -S47ShotStills=<dir> is on the command line (read once).
 SIGNAL47_API bool Wanted();
 // Each frame from the developer events' contracts (FS47DevEvents::ControlTest) while Wanted: true once it is over (it has asked
 // the game to exit).
 SIGNAL47_API bool Step(US47Main* M,double Dt);
}
