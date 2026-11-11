#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Script.h"
class US47Main;
class UWorld;

// DIRECTOR-MODE-01, marks: a division of labour. Tom flies the director's free camera (S47DirectorCam.h) and marks spots by eye ("something
// could stand here", "this view is good"); an author who cannot see the world reads those marks from one file later and writes the event
// logic around them. While flying: 0 picks the kind of the next mark (place, view, path, avoid), 7 drops a mark, 8 drops one and takes a
// picture of the view next to the file, 9 takes back the last mark of this session (never one from an earlier session, never a picture).
//
// The file: <ProjectSavedDir>/Director/marks.json (with the game's -UserDir= it is in the user dir), {"v":1,"marks":[{...}]}, appended to
// by read, change, write to a temp file and move. A file that does not read as that is never overwritten: the marks of this session go
// to marks-<timestamp>.json beside it instead, and the toast and the log say so. No note is typed while flying (the walk keys fly):
// the kind is always stored, "note" only when a later tool adds one.
//
// One mark, as written (points road-local metres, x east, z south, up over the ground there; eye and yaw/pitch in the scene's frame of
// the area it was dropped in, three-space metres and radians as the walker's; fov the vertical field of view in degrees):
//   {"n":3,"kind":"view","session":"20261111-231502-4242","time":"2026-11-11 23:16:40","area":"saro","eye":[12.5,4.2,-30.1],
//    "yaw":0.71,"pitch":-0.12,"fov":70,"cam":{"x":-310.4,"z":1220.7,"up":3.1},"look":{"x":-325,"z":1204.2,"up":0,"dist":24.6},
//    "ahead":{"x":-323.6,"z":1205.6,"up":0.7},"indoors":"no","clock":"23:16","day":2,"weather":["clear","calm"],
//    "place":{"id":"farm","m":140.2},"draft":"script_1111_231200","picture":"mark-003.png"}
// "look" is left out when nothing is under the crosshair within 400 m, "cam" and "ahead" when the camera is not over the land of the
// area, "place" without a place list, "picture" for a 7.
struct FS47DirectorMark
{
 int32 N=0;                 // a running number over every session of the file, from 1
 FString Kind=TEXT("place"); // place (something stands or happens here), view (a good shot from here), path (the player walks
                             // through here), avoid (looks bad or broken here)
 FString Note;
 FString Session,Time,Area; // the game run that dropped it; the wall clock (local); the area the player was in (AS47Web::Area)
 FVector Eye=FVector::ZeroVector;
 double Yaw=0,Pitch=0,Fov=0;
 FS47ScriptPoint Cam;       // the camera, road-local (Up: its height over the ground)
 FS47ScriptPoint Look;      // under the crosshair (as FS47ScriptPlayer::RayRoad), LookDist metres from the camera
 double LookDist=-1;
 FS47ScriptPoint Ahead;     // 20 m along the view
 FString Indoors=TEXT("unknown"); // yes, no (US47World::Indoors at the camera), unknown
 FString Clock;int32 Day=0;TArray<FString> Weather; // the Freeform clock (HH:MM) and shift day, and the weather words of the scripts
 FString Place;double PlaceDist=-1; // the nearest place of S47EventPlaces and how far, road-local metres
 FString Draft;             // the id of the draft open in the script editor
 FString Picture;           // the picture's file name beside the marks file
};

namespace S47DirectorMarks
{
 // ---- pure ----
 SIGNAL47_API const TArray<FString>& Kinds();
 SIGNAL47_API FString NextKind(const FString& Kind);
 SIGNAL47_API S47J::FObj ToJson(const FS47DirectorMark& K);
 SIGNAL47_API FS47DirectorMark FromJson(const S47J::FObj& O);
 // What a file's text is: Empty (nothing, or only space: as no file), Good, or Bad (anything else: never overwritten).
 enum class ERead : uint8 { Empty, Good, Bad };
 SIGNAL47_API ERead Parse(const FString& Text,TArray<FS47DirectorMark>& Out);
 SIGNAL47_API FString Write(const TArray<FS47DirectorMark>& Marks);
 // The file to write to: Main, unless it read Bad, then Side.
 SIGNAL47_API FString TargetFile(const FString& Main,ERead Read,const FString& Side);
 // Adds K at the end with the next running number (one more than the highest there); returns that number.
 SIGNAL47_API int32 Append(TArray<FS47DirectorMark>& Marks,FS47DirectorMark K);
 // Removes the last mark of OfSession (OutN its number). False: none of this session's is there; nothing removed.
 SIGNAL47_API bool RemoveLast(TArray<FS47DirectorMark>& Marks,const FString& OfSession,int32& OutN);
 // A shot action from the mark: its eye at the mark's camera point, looking at the crosshair's point or else 20 m ahead, the mark's
 // lens, 5 s, at 0 s (the caller puts it on the timeline). No camera point: no eye (Path empty).
 SIGNAL47_API FS47ScriptAction MarkToShot(const FS47DirectorMark& K);
 // A trigger at the crosshair's point (on the ground), else under the camera: a circle of 8 m, entered at once; the rest default.
 SIGNAL47_API FS47ScriptTrigger MarkToTrigger(const FS47DirectorMark& K);
 // "view at (-310, 1221) +3.1 m, looking at (-325, 1204) 24.6 m, farm 140 m": the log's and the editor's line.
 SIGNAL47_API FString Line(const FS47DirectorMark& K);
 // Lines "PASS/FAIL MARKS: ..." (from S47DirectorCam::SelfTest).
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);

 // ---- the file ----
 // <ProjectSavedDir>/Director/marks.json, or the test's own (SetPathForTest; empty: back to the real one).
 SIGNAL47_API FString Path();
 SIGNAL47_API void SetPathForTest(const FString& File);
 // This game run's name in the marks ("<when it began>-<process id>").
 SIGNAL47_API FString Session();
 // Read a marks file. A missing file is Empty.
 SIGNAL47_API ERead ReadFile(const FString& File,TArray<FS47DirectorMark>& Out);
 // K given its number, session and time (and with bPicture its picture name), appended to the file, and with bPicture the picture asked
 // for; logged. Said: the line for the director's text. False: not written (Said says why).
 SIGNAL47_API bool Add(FS47DirectorMark K,bool bPicture,FString& Said);
 // Key 9: the last mark of this session taken out of the file (its picture stays). False with Said: nothing taken.
 SIGNAL47_API bool RemoveLastOfSession(FString& Said);
 // The marks of the file being written to now, read once and kept up to date by Add and RemoveLastOfSession.
 SIGNAL47_API const TArray<FS47DirectorMark>& Known();
 // While flying, each frame: each mark with a camera point a small magenta cross, a short line where it looked and its number; none
 // within 1.5 m of the camera (CamUE: the camera, Unreal centimetres); the 200 nearest at most.
 SIGNAL47_API void Draw(US47Main* M,UWorld* World,const FVector& CamUE);
}
