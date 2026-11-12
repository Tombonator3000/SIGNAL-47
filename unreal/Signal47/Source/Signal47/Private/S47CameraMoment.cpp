#include "S47CameraMoment.h"
#include "S47Flow.h"
#include "S47Main.h"
#include "S47Sneak.h"
#include "S47Walker.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47CameraMoment,Log,All);

// N1110 (N1110-3): the camera moment (S47CameraMoment.h): the curve, the look-at, and the layer on the walker's view.

namespace S47CameraMoment
{
namespace
{
 // Smooth in and out (3k² - 2k³): no kick at either end, the snap still quick in the middle.
 double Ease(double K){K=FMath::Clamp(K,0.,1.);return K*K*(3.-2.*K);}
 bool Sane(const FS47CameraSnap& S)
 {
  const double Whole=S.In+S.Hold+S.Out;
  return FMath::IsFinite(Whole)&&S.In>0&&S.Hold>=0&&S.Out>0&&Whole>=LeastSeconds&&Whole<=MostSeconds&&S.Fov>=FovMin&&S.Fov<=FovMax&&S.Shake>=0&&S.Shake<=1&&!S.At.ContainsNaN();
 }
}

double Curve(double T,double In,double Hold,double Out)
{
 if(!FMath::IsFinite(T)||T<=0)return 0;
 if(T<In)return Ease(T/FMath::Max(In,1e-6));
 if(T<In+Hold)return 1;
 if(T<In+Hold+Out)return 1-Ease((T-In-Hold)/FMath::Max(Out,1e-6));
 return 0;
}

double FovAt(double Fov0,double FovTo,double K){return FMath::Lerp(Fov0,FovTo,FMath::Clamp(K,0.,1.));}

FS47CameraSnap Reduced(const FS47CameraSnap& S,double Fov0)
{
 FS47CameraSnap R=S;
 R.Fov=Fov0-(Fov0-S.Fov)*.5;
 R.Shake=0;
 R.In=S.In*1.5;
 return R;
}

double Wrap(double A){return FMath::Atan2(FMath::Sin(A),FMath::Cos(A));}

void LookAt(const FVector& Eye,const FVector& At,double& Yaw,double& Pitch)
{
 // The walker's forward in three's space is (-sin yaw cos pitch, sin pitch, -cos yaw cos pitch) (S47Three::CameraRotation).
 const FVector D=(At-Eye).GetSafeNormal();
 if(D.IsNearlyZero())return;
 Pitch=FMath::Asin(FMath::Clamp(D.Y,-1.,1.));
 Yaw=FMath::Atan2(-D.X,-D.Z);
}
}

bool FS47CameraMoment::Start(const FS47CameraSnap& S,FString& Why)
{
 using namespace S47CameraMoment;
 AS47Walker* W=M?M->Player.Get():nullptr;
 if(!W){Why=TEXT("no walker");return false;}
 if(M->Flow&&!M->Flow->bCameraMoments){Why=TEXT("Camera moments is off");return false;}
 // (one left live while the cab or a screen held the camera, or with its layer taken off the walker, is over)
 if(bLive&&(W->bCameraHeld||!W->ViewLayer))Stop();
 if(W->bCameraHeld){Why=TEXT("the camera is held by something else");return false;}
 if(bLive){Why=TEXT("one is on");return false;}
 if(!Sane(S)){Why=TEXT("outside the limits");return false;}
 Fov0=W->Fov;
 Snap=M->Flow&&!M->Flow->bHeadMotion?Reduced(S,Fov0):S;
 T=0;bShaken=false;bLive=true;Hooked=W;
 const TWeakPtr<FS47CameraMoment> Wk=AsShared();
 W->ViewLayer=[Wk](double Dt,double& Yaw,double& Pitch,float& FovK)
 {
  const TSharedPtr<FS47CameraMoment> Self=Wk.Pin();
  return Self.IsValid()&&Self->Frame(Dt,Yaw,Pitch,FovK);
 };
 UE_LOG(LogS47CameraMoment,Display,TEXT("S47: camera moment toward %.1f %.1f %.1f: to %.0f degrees in %.2f s, held %.2f s, back in %.2f s%s"),S.At.X,S.At.Y,S.At.Z,Snap.Fov,Snap.In,Snap.Hold,Snap.Out,Snap.Fov!=S.Fov?TEXT(" (reduced motion)"):TEXT(""));
 return true;
}

void FS47CameraMoment::Stop()
{
 if(AS47Walker* W=Hooked.Get())W->ViewLayer=nullptr;
 Hooked.Reset();
 bLive=false;
}

bool FS47CameraMoment::Frame(double Dt,double& Yaw,double& Pitch,float& FovK)
{
 using namespace S47CameraMoment;
 AS47Walker* W=Hooked.Get();
 if(!bLive||!W)return false;
 T+=FMath::Max(0.,Dt);
 if(T>=Snap.In+Snap.Hold+Snap.Out){Stop();return false;}
 const double K=Curve(T,Snap.In,Snap.Hold,Snap.Out);
 // From the eye where it is (without the bob: it is the look that moves, not the head), to the point.
 const FVector Eye(W->Pos.X,W->FloorY+W->Lift+W->Eye-S47Sneak::EyeDrop*W->CrouchK,W->Pos.Z);
 double ToYaw=Yaw,ToPitch=Pitch;
 LookAt(Eye,Snap.At,ToYaw,ToPitch);
 Yaw=Yaw+Wrap(ToYaw-Yaw)*K;
 Pitch=FMath::Lerp(Pitch,ToPitch,K);
 FovK=float(FovAt(Fov0,Snap.Fov,K)/FMath::Max(1.,Fov0));
 // The shake as the snap lands, once (the walker leaves it out with head motion off).
 if(!bShaken&&T>=Snap.In){bShaken=true;if(Snap.Shake>0)W->Shake=FMath::Max(W->Shake,Snap.Shake);}
 return true;
}

bool S47CameraMoment::SelfTest(TArray<FString>& Lines)
{
 bool bAll=true;
 const auto Check=[&](bool bOk,const FString& What){bAll&=bOk;Lines.Add(FString(bOk?TEXT("PASS "):TEXT("FAIL "))+TEXT("camera moment: ")+What);};
 // ---- the curve ----
 Check(Curve(0,.25,1.2,.9)==0&&Curve(-1,.25,1.2,.9)==0&&Curve(2.35,.25,1.2,.9)==0&&Curve(9,.25,1.2,.9)==0,TEXT("nothing before the start and from the end on"));
 Check(FMath::IsNearlyEqual(Curve(.125,.25,1.2,.9),.5)&&Curve(.25,.25,1.2,.9)==1&&Curve(1,.25,1.2,.9)==1&&Curve(1.45,.25,1.2,.9)==1,TEXT("half way in at half the snap, all the way through the hold"));
 Check(FMath::IsNearlyEqual(Curve(1.9,.25,1.2,.9),.5)&&Curve(2.3,.25,1.2,.9)<.02&&Curve(2.3,.25,1.2,.9)>0,TEXT("half way back at half the ease out, nearly nothing just before the end"));
 {
  bool bUp=true,bDown=true;double Last=0;
  for(double T=0;T<=.25;T+=.01){const double K=Curve(T,.25,1.2,.9);bUp&=K>=Last;Last=K;}
  Last=1;for(double T=1.45;T<=2.35;T+=.01){const double K=Curve(T,.25,1.2,.9);bDown&=K<=Last;Last=K;}
  Check(bUp&&bDown,TEXT("in only rises, out only falls"));
  const double A=Curve(.01,.25,1.2,.9),B=Curve(.02,.25,1.2,.9);
  Check(A<.01&&B-A<.02,TEXT("no kick at the start: it eases"));
 }
 Check(Curve(.5,0,1,1)==1&&Curve(.5,1,0,1)<1&&Curve(1.5,1,0,1)<1,TEXT("no way in: on it at once; no hold: never quite still"));
 // ---- the field of view ----
 Check(FovAt(70,45,0)==70&&FovAt(70,45,1)==45&&FMath::IsNearlyEqual(FovAt(70,45,.5),57.5)&&FovAt(70,45,2)==45,TEXT("the zoom goes from the player's field of view to the snap's"));
 // ---- reduced motion ----
 {
  FS47CameraSnap S;S.Fov=40;S.Shake=.4;S.In=.2;
  const FS47CameraSnap R=Reduced(S,70);
  Check(FMath::IsNearlyEqual(R.Fov,55.)&&R.Shake==0&&FMath::IsNearlyEqual(R.In,.3)&&R.Hold==S.Hold&&R.Out==S.Out,TEXT("reduced motion: half the zoom, no shake, a slower way in, the same hold and way back"));
 }
 // ---- the look-at ----
 {
  double Yaw=9,Pitch=9;
  LookAt(FVector(0,1.6,0),FVector(0,1.6,-10),Yaw,Pitch);
  Check(FMath::IsNearlyEqual(Yaw,0.,1e-9)&&FMath::IsNearlyEqual(Pitch,0.,1e-9),TEXT("straight ahead (north, -z) is yaw 0, level"));
  LookAt(FVector(0,1.6,0),FVector(-10,1.6,0),Yaw,Pitch);
  Check(FMath::IsNearlyEqual(Yaw,UE_DOUBLE_PI/2,1e-9),TEXT("west (-x) is yaw +90 degrees, as the walker turns"));
  LookAt(FVector(0,1.6,0),FVector(0,1.6+10,-10),Yaw,Pitch);
  Check(FMath::IsNearlyEqual(Pitch,UE_DOUBLE_PI/4,1e-9)&&FMath::IsNearlyEqual(Yaw,0.,1e-9),TEXT("up at 45 degrees is pitch +45"));
  Yaw=1;Pitch=.2;LookAt(FVector(1,1,1),FVector(1,1,1),Yaw,Pitch);
  Check(Yaw==1&&Pitch==.2,TEXT("looking at the eye itself changes nothing"));
  Check(FMath::IsNearlyEqual(Wrap(UE_DOUBLE_PI+.1),-UE_DOUBLE_PI+.1,1e-9)&&FMath::IsNearlyEqual(Wrap(-.3),-.3,1e-9),TEXT("the turn goes the short way round"));
 }
 return bAll;
}
