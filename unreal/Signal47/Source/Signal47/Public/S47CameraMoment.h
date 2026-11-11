#pragma once
#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
class US47Main;
class AS47Walker;

// N1110 (N1110-3, Tom 10 November: "hendelser kan ha korte sekvenser der kameraet zoomer brått mot noe"). A camera moment: for a
// short while (0.6 to 2.5 s as a rule) the view snaps toward a point and closes in on it, holds, and eases back onto the player's
// own look. The player keeps walking and the mouse keeps turning the look underneath; only what is rendered is taken (AS47Walker::
// ViewLayer: the yaw, the pitch and a factor on the field of view), so nothing has to be given back but the layer itself.
//
// Asked for by the event scripts' camera_snap beat (S47EventScript.h; the events eyes_window, beam_figure and dish_lights) and by
// any code through S47EventScript::CameraSnap (the attention system's hook). Settings: "Camera moments" (FS47PauseOpts, US47Flow::
// bCameraMoments) off keeps the view the player's; "Head motion and bumps" off is the reduced motion: the zoom halved, no shake, a
// slower way in (Reduced). Not in the truck or with the camera held by something else (the walker's CameraOverride). One at a time.
struct FS47CameraSnap
{
 FVector At=FVector::ZeroVector; // what it looks at, scene metres (three-space: x east, y up, z south)
 double Fov=45;                  // the vertical field of view it closes to, degrees (the player's is 70 as made)
 double In=.25,Hold=1.2,Out=.9;  // seconds: the snap toward it, the hold on it, the ease back
 double Shake=0;                 // 0 to 1, the walker's own shake as the snap lands; 0 none
};

namespace S47CameraMoment
{
 // 0 to 1: eased in over In, 1 through Hold, eased out over Out; 0 before the start and from In+Hold+Out on. Pure.
 SIGNAL47_API double Curve(double T,double In,double Hold,double Out);
 // The field of view at K of the way from Fov0 to the snap's Fov (K from Curve).
 SIGNAL47_API double FovAt(double Fov0,double FovTo,double K);
 // The reduced motion's version: half the zoom (from Fov0), no shake, the way in half again as long. Pure.
 SIGNAL47_API FS47CameraSnap Reduced(const FS47CameraSnap& S,double Fov0);
 // The yaw and the pitch (three-space, as the walker's) that look from Eye at At. Pure.
 SIGNAL47_API void LookAt(const FVector& Eye,const FVector& At,double& Yaw,double& Pitch);
 // An angle into -pi..pi.
 SIGNAL47_API double Wrap(double A);
 // The limits the lint and Start hold a snap to.
 inline constexpr double FovMin=20,FovMax=90,LeastSeconds=.3,MostSeconds=5;
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}

class SIGNAL47_API FS47CameraMoment : public TSharedFromThis<FS47CameraMoment>
{
public:
 explicit FS47CameraMoment(US47Main* Main):M(Main){}
 ~FS47CameraMoment(){Stop();}
 FS47CameraMoment(const FS47CameraMoment&)=delete;
 FS47CameraMoment& operator=(const FS47CameraMoment&)=delete;
 // Takes the view for In+Hold+Out seconds. False with Why: no walker, the camera held by something else, "Camera moments" off,
 // one already on, or a snap outside the limits.
 bool Start(const FS47CameraSnap& S,FString& Why);
 // The view the player's again at once.
 void Stop();
 bool Live() const{return bLive;}
 double Age() const{return T;}
private:
 US47Main* M=nullptr;
 TWeakObjectPtr<AS47Walker> Hooked;
 FS47CameraSnap Snap;
 double T=0,Fov0=70;
 bool bLive=false,bShaken=false;
 bool Frame(double Dt,double& Yaw,double& Pitch,float& FovK);
};
