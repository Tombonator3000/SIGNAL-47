#pragma once
#include "CoreMinimal.h"
#include "S47Script.h"
class US47Main;

// DIRECTOR-MODE-01, slice 2 (Tom 11 November: director mode, "like Zeus, Game Master, the Amnesia editor"). The director's free camera:
// in a developer's run (FS47ScriptPlayer::DevAllowed), in Freeform, on foot, the developer flies a free camera through the world (no
// collision, through walls) and places things into the script being edited in the script editor (S47ScriptEditor::Draft): a shot from
// where the camera is onto what the crosshair is on, a second eye for that shot, the trigger's point, and a preview from the top.
//
// The player's body is never moved: the walker's camera is held (its CameraOverride, as FS47ShotCamera holds it) and given back as it
// was. While director mode is on the night waits (Tom, 11 November): the Freeform session is held as by an open panel (its clock stands,
// FS47FreeformSession::Held), no script starts (FS47ScriptPlayer::Hold "director"), the Director's dice and OneAtATime wait, the night's
// rhythm lets no dangerous meeting begin (FS47Rhythm::Allow) and nobody is taken (FS47Taken). It is not entered while anything of that
// kind is under way, and it ends by itself when the session stops, the player is taken or a panel opens.
//
// Keys while flying: the walk keys (S47Keys forward, back, left, right) fly along the view, Jump up, Keep low down, Run fast; the mouse
// or the pad looks. 1 to 6, [ and ] are read straight off the keyboard (no rows in the key table: they mean something only here).
// Slice 4: one shot of the draft is the selected one, shared with the script editor (S47ScriptEditor::SelectedShot): [ and ] pick
// the one before or after, 2 gives the selected shot its second eye here, 5 puts its eye here and its look on the crosshair, 6
// deletes it. Marks (S47DirectorMarks.h): 0 picks the kind of the next mark, 7 drops one here, 8 drops one with a picture, 9 takes back
// the last one of this session.
namespace S47DirectorCam
{
 // The camera's pose: the eye in scene metres (three-space: x east, y up, z south), yaw and pitch in radians as the walker's.
 struct FFly { FVector Eye=FVector::ZeroVector; double Yaw=0, Pitch=0; };
 inline constexpr double FlyMps=6,FastMps=30,PitchLimit=1.5;
 // Pure: one step of flying. Move x right, z back (as the walker passes it to CameraOverride; longer than 1 is cut to 1), Rise -1..1
 // straight up or down, Mps metres a second, LookIn in radians as AS47Walker::Look takes it (the pitch held within PitchLimit).
 SIGNAL47_API FFly Step(const FFly& In,const FVector2D& Move,double Rise,const FVector2D& LookIn,double Mps,double Dt);
 // DIRECTOR-MODE-01, slice 4: the selected shot, as an index into a script's actions (INDEX_NONE: none). Pure, so the self-test
 // holds them. ShotOrNone: Sel itself when it is a shot action there, else none. ShotOrLast: that, or the last shot (none without
 // shots). CycleShot: the next shot after Sel (Dir 1) or before it (Dir -1), round the end; from no selection the first (Dir 1) or
 // the last (Dir -1). AfterRemove: Sel once action Removed has gone (After: the actions without it): an earlier one shifts it down,
 // a later one leaves it, itself moves it to the next shot, else the one before, else none. AfterSwap: Sel once actions A and B
 // have changed places. ShotNumber: the shot's place among the shots, from 1 (0: not a shot).
 SIGNAL47_API int32 ShotOrNone(const TArray<FS47ScriptAction>& Acts,int32 Sel);
 SIGNAL47_API int32 ShotOrLast(const TArray<FS47ScriptAction>& Acts,int32 Sel);
 SIGNAL47_API int32 CycleShot(const TArray<FS47ScriptAction>& Acts,int32 Sel,int32 Dir);
 SIGNAL47_API int32 AfterRemove(const TArray<FS47ScriptAction>& After,int32 Sel,int32 Removed);
 SIGNAL47_API int32 AfterSwap(int32 Sel,int32 A,int32 B);
 SIGNAL47_API int32 ShotNumber(const TArray<FS47ScriptAction>& Acts,int32 I);
 // The script as played from From seconds on its timeline (PREVIEW FROM HERE): the actions that start before From left out, the
 // rest From earlier. From 0 or less: the script as it is. Pure.
 SIGNAL47_API FS47Script FromTime(const FS47Script& S,double From);
 // DIRECTOR-MODE-01, marks: what the camera marks and their file, MarkToShot and MarkToTrigger are in S47DirectorMarks.h (the keys
 // 7, 8, 9 and 0 here call it; its "MARKS:" self-test lines are part of SelfTest below).
 // Lines "PASS/FAIL DIRECTOR-2: ..." for flying and "DIRECTOR-4: ..." for the selected shot (-S47SelfTest, from the HUD's).
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
 // Director mode is on: asked by everything that must wait while it is.
 SIGNAL47_API bool On(const US47Main* M);
 // On, or off again. False with Why: not entered (not a developer's run, not in Freeform play, not on foot, a panel open, the camera
 // held by something else, being taken, a visit or a hunt under way, a script playing, a big event holding the night). Says so on the
 // screen, except outside a developer's run.
 SIGNAL47_API bool Toggle(US47Main* M,FString& Why);
 // Off at once: the camera given back and every hold let go. Nothing on the screen (also from a destructor).
 SIGNAL47_API void Leave(US47Main* M);
 // Each frame (US47Main::Step, next to the developer events): the number keys, the text on the screen, and leaving by itself.
 SIGNAL47_API void Tick(US47Main* M,double Dt);
 // The F10 group "Director mode" (only in a developer's run): a line that turns it on. Added once; the menu outlives a session.
 SIGNAL47_API void AddDevLine(US47Main* M);
 // -S47Freeform -S47Director -S47DevEventsContract=/tmp/dir -S47DevEventsGroup="Director mode" (FS47DevEvents::ControlTest): enters
 // director mode, flies forward and up for two seconds through a test hook, checks the hold, the still player, the night held and a
 // shot placed at the camera, flies a little to the right and gives that shot its second eye (checking it is the selected shot), drops one mark into
 // <dir>/director-marks.json (never Saved/Director/marks.json) and reads it back (one mark, its camera point within 0.5 m), takes
 // director-mode-fly.png, leaves and checks everything is given back. True once it is over (it has asked the game to exit).
 SIGNAL47_API bool Contract(US47Main* M,double Dt);
}
