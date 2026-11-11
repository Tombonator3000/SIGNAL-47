#pragma once
#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "S47ArrayShot.h"
class US47Main;
class AS47Walker;
struct FS47View;

// SHOT-BEAT-01 (Tom 11 November, director mode: "shot beat in Story og Freeform"). A directed camera shot: for some seconds the
// camera is taken to an eye of its own and looks at a point through a lens of its own, if asked moving slowly from a first pose to a
// second one, and is then given back. The player is never moved: the walker's camera is held (its CameraOverride, as FS47ArrayShot
// holds it, and through the same pose and blend), the player stands where they stood and looks where they looked, and the view eases
// in from the live one and back onto it, never a cut. No shake, no flashing.
//
// Asked for by the scripted events' shot action (S47Script.h, ES47ScriptAct::Shot, played by FS47ScriptPlayer), so it works in
// Freeform and in Story (a Freeform session). Settings: "Camera moments" (US47Flow::bCameraMoments) off: no shot; "Head motion and
// bumps" off is the reduced motion: no move (the first pose held) and a slower way in and out (Reduced). Not at the wheel, not while
// the camera is held by something else, one at a time.
struct FS47ShotSpec
{
 FVector EyeA=FVector::ZeroVector,EyeB=FVector::ZeroVector,Target=FVector::ZeroVector; // scene metres (three-space: x east, y up, z south)
 double FovA=60,FovB=60;     // the vertical field of view at the first and the second pose, degrees
 double Secs=6,In=.6,Out=.8; // seconds: the hold, the ease in from the live view, the ease back onto it
 bool bMove=false;           // EyeA to EyeB and FovA to FovB over the hold; false: the first pose held
};

namespace S47Shot
{
 // The limits Start and the scripts' lint hold a shot to (S47Script::Lint keeps the same numbers).
 inline constexpr double MaxSecs=30,FovMin=20,FovMax=90,LeastFade=.3,MostFade=3,ReducedFade=.6,LeastReach=.5,MostTravel=200;
 // The pose at T seconds into the hold (0: the hold begins, Secs: it ends; clamped outside): eased EyeA to EyeB and FovA to FovB
 // when bMove, always looking at Target (S47CameraMoment::LookAt). Pure.
 SIGNAL47_API FS47ArrayShot::FPose PoseAt(const FS47ShotSpec& S,double T);
 // 0 to 1, the shot's share against the live view at T seconds from the start: eased in over In, 1 through Secs, eased out over
 // Out (S47CameraMoment::Curve). Pure.
 SIGNAL47_API double Weight(double T,double In,double Secs,double Out);
 // The reduced motion's version: no move (the first pose held), In and Out at least ReducedFade. Pure.
 SIGNAL47_API FS47ShotSpec Reduced(const FS47ShotSpec& S);
 // Whether a shot keeps to the limits; Why: the first thing that does not.
 SIGNAL47_API bool Sane(const FS47ShotSpec& S,FString& Why);
 // Lines "PASS/FAIL SHOT-n: ..." (-S47SelfTest, from the HUD's).
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
 // The F10 group "Director shot" (only in a developer's run, as the script player's own lines): a built-in shot by the player,
 // previewed through the script player. Added once; the menu outlives a session.
 SIGNAL47_API void AddDevLine(US47Main* M);
 // -S47DevEventsContract=/tmp/dir -S47DevEventsGroup="Director shot" (FS47DevEvents::ControlTest): runs that line and checks the
 // camera is held mid-shot, the player has not moved, the eye is the shot's in the hold and the camera is given back as it was.
 // True once it is over (it has asked the game to exit).
 SIGNAL47_API bool Contract(US47Main* M,double Dt);
}

class SIGNAL47_API FS47ShotCamera : public TSharedFromThis<FS47ShotCamera>
{
public:
 explicit FS47ShotCamera(US47Main* InM):M(InM){}
 ~FS47ShotCamera(){Stop();}
 FS47ShotCamera(const FS47ShotCamera&)=delete;
 FS47ShotCamera& operator=(const FS47ShotCamera&)=delete;
 // Takes the camera for In+Secs+Out seconds (Secs capped at MaxSecs). False with Why: no walker, "Camera moments" off, at the wheel,
 // the camera held by something else, one already live, or a shot outside the limits.
 bool Start(const FS47ShotSpec& S,FString& Why);
 // At once, the camera as it was before the shot (its override and its field of view), also mid-shot.
 void Stop();
 // Each frame from outside the walker's own frame (FS47ScriptPlayer::Step): gives the walker's camera back once a shot is over (not
 // from inside the override, which is still running then).
 void Tick();
 bool Live() const{return bLive;}
 double Age() const{return T;}
 // The shot as it plays (after the cap and the reduced motion).
 const FS47ShotSpec& Playing() const{return Spec;}
 // The last one started, for the in-game check (S47Shot::Contract).
 static TSharedPtr<FS47ShotCamera> LastStarted();
private:
 US47Main* M=nullptr;
 TWeakObjectPtr<AS47Walker> Hooked;
 TFunction<bool(double,const FVector2D&,const FVector2D&,FS47View&)> Before;
 FS47ShotSpec Spec;
 double T=0,Fov0=70;
 bool bLive=false;
 bool Frame(double Dt,FS47View& Out);
 FS47ArrayShot::FPose LiveView() const;
 void Unhook();
};
