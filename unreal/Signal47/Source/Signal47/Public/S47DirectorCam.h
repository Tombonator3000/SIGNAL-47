#pragma once
#include "CoreMinimal.h"
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
// or the pad looks. 1, 2, 3 and 4 are read straight off the keyboard (no rows in the key table: they mean something only here).
namespace S47DirectorCam
{
 // The camera's pose: the eye in scene metres (three-space: x east, y up, z south), yaw and pitch in radians as the walker's.
 struct FFly { FVector Eye=FVector::ZeroVector; double Yaw=0, Pitch=0; };
 inline constexpr double FlyMps=6,FastMps=30,PitchLimit=1.5;
 // Pure: one step of flying. Move x right, z back (as the walker passes it to CameraOverride; longer than 1 is cut to 1), Rise -1..1
 // straight up or down, Mps metres a second, LookIn in radians as AS47Walker::Look takes it (the pitch held within PitchLimit).
 SIGNAL47_API FFly Step(const FFly& In,const FVector2D& Move,double Rise,const FVector2D& LookIn,double Mps,double Dt);
 // Lines "PASS/FAIL DIRECTOR-2: ..." (-S47SelfTest, from the HUD's).
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
 // shot placed at the camera, takes director-mode-fly.png, leaves and checks everything is given back. True once it is over (it has
 // asked the game to exit).
 SIGNAL47_API bool Contract(US47Main* M,double Dt);
}
