#include "S47Shot.h"
#include "S47CameraMoment.h"
#include "S47DevEvents.h"
#include "S47Flow.h"
#include "S47Main.h"
#include "S47Script.h"
#include "S47ScriptPlayer.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47World.h"
#include "HAL/PlatformMisc.h"
#include <limits>

DEFINE_LOG_CATEGORY_STATIC(LogS47Shot,Log,All);

// SHOT-BEAT-01: the directed camera shot (S47Shot.h): its pose, its curve, the reduced motion, and the hold on the walker's camera.

namespace S47ShotLocal
{
 // Smooth in and out (3k² - 2k³), as the camera moment's and the array shot's: no kick at either end.
 double Ease(double K){K=FMath::Clamp(K,0.,1.);return K*K*(3.-2.*K);}
 TWeakPtr<FS47ShotCamera>& Last(){static TWeakPtr<FS47ShotCamera> L;return L;}
 const TCHAR* const DevGroup=TEXT("Director shot");
 const TCHAR* const DevId=TEXT("dev_director_shot");
}

namespace S47Shot
{
FS47ArrayShot::FPose PoseAt(const FS47ShotSpec& S,double T)
{
 const double K=S.bMove&&S.Secs>0?S47ShotLocal::Ease(T/S.Secs):0.;
 FS47ArrayShot::FPose P;
 P.Eye=FMath::Lerp(S.EyeA,S.EyeB,K);
 P.Fov=FMath::Lerp(S.FovA,S.FovB,K);
 S47CameraMoment::LookAt(P.Eye,S.Target,P.Yaw,P.Pitch);
 P.Roll=0;
 return P;
}

double Weight(double T,double In,double Secs,double Out){return S47CameraMoment::Curve(T,In,Secs,Out);}

FS47ShotSpec Reduced(const FS47ShotSpec& S)
{
 FS47ShotSpec R=S;
 R.bMove=false;R.EyeB=R.EyeA;R.FovB=R.FovA;
 R.In=FMath::Max(S.In,ReducedFade);R.Out=FMath::Max(S.Out,ReducedFade);
 return R;
}

bool Sane(const FS47ShotSpec& S,FString& Why)
{
 // (written as what must hold, so that a NaN fails it)
 if(S.EyeA.ContainsNaN()||S.EyeB.ContainsNaN()||S.Target.ContainsNaN()){Why=TEXT("a point is not a number");return false;}
 if(!(S.Secs>0&&S.Secs<=MaxSecs)){Why=FString::Printf(TEXT("held %.1f s, not 0 to %.0f"),S.Secs,MaxSecs);return false;}
 if(!(S.FovA>=FovMin&&S.FovA<=FovMax&&S.FovB>=FovMin&&S.FovB<=FovMax)){Why=FString::Printf(TEXT("a lens of %.0f to %.0f degrees, not %.0f to %.0f"),S.FovA,S.FovB,FovMin,FovMax);return false;}
 if(!(S.In>=LeastFade&&S.In<=MostFade&&S.Out>=LeastFade&&S.Out<=MostFade)){Why=FString::Printf(TEXT("in %.2f s and out %.2f s, not %.1f to %.0f"),S.In,S.Out,LeastFade,MostFade);return false;}
 if(!(FVector::Dist(S.EyeA,S.Target)>=LeastReach)||(S.bMove&&!(FVector::Dist(S.EyeB,S.Target)>=LeastReach))){Why=TEXT("an eye on the point it looks at");return false;}
 if(S.bMove&&!(FVector::Dist(S.EyeA,S.EyeB)<=MostTravel)){Why=FString::Printf(TEXT("the eyes %.0f m apart, more than %.0f"),FVector::Dist(S.EyeA,S.EyeB),MostTravel);return false;}
 Why.Reset();
 return true;
}
}

TSharedPtr<FS47ShotCamera> FS47ShotCamera::LastStarted(){return S47ShotLocal::Last().Pin();}

bool FS47ShotCamera::Start(const FS47ShotSpec& S,FString& Why)
{
 using namespace S47Shot;
 AS47Walker* W=M?M->Player.Get():nullptr;
 if(!W){Why=TEXT("no walker");return false;}
 // (one over but not yet given back, Tick not having run since: given back now)
 if(!bLive&&Hooked.IsValid())Unhook();
 if(bLive){Why=TEXT("a shot is on");return false;}
 if(M->Flow&&!M->Flow->bCameraMoments){Why=TEXT("Camera moments is off");return false;}
 if(M->World&&M->World->bDriving){Why=TEXT("at the wheel");return false;}
 if(W->CameraOverride){Why=TEXT("the camera is held by something else");return false;}
 FS47ShotSpec Use=S;
 Use.Secs=FMath::Min(Use.Secs,MaxSecs);
 if(!Sane(Use,Why))return false;
 const bool bReduced=M->Flow&&!M->Flow->bHeadMotion;
 if(bReduced)Use=Reduced(Use);
 Spec=Use;Fov0=W->Fov;T=0;bLive=true;Hooked=W;
 // The walker's camera, held as the array shot holds it; whatever held it before (nothing, as Start asks) has it back after.
 Before=W->CameraOverride;
 TFunction<bool(double,const FVector2D&,const FVector2D&,FS47View&)> Prev=Before;
 const TWeakPtr<FS47ShotCamera> Wk=AsShared();
 W->CameraOverride=[Wk,Prev](double Dt,const FVector2D& Move,const FVector2D& LookIn,FS47View& Out)
 {
  if(const TSharedPtr<FS47ShotCamera> Self=Wk.Pin())if(Self->Frame(Dt,Out))return true;
  return Prev?Prev(Dt,Move,LookIn,Out):false;
 };
 S47ShotLocal::Last()=AsShared();
 UE_LOG(LogS47Shot,Display,TEXT("S47: director shot from %.1f %.1f %.1f%s at %.1f %.1f %.1f: %.0f to %.0f degrees, in %.2f s, held %.1f s, out %.2f s%s"),
  Spec.EyeA.X,Spec.EyeA.Y,Spec.EyeA.Z,Spec.bMove?*FString::Printf(TEXT(" to %.1f %.1f %.1f"),Spec.EyeB.X,Spec.EyeB.Y,Spec.EyeB.Z):TEXT(""),
  Spec.Target.X,Spec.Target.Y,Spec.Target.Z,Spec.FovA,Spec.FovB,Spec.In,Spec.Secs,Spec.Out,bReduced?TEXT(" (reduced motion)"):TEXT(""));
 return true;
}

void FS47ShotCamera::Stop()
{
 if(bLive)if(AS47Walker* W=Hooked.Get())W->Fov=float(Fov0);
 bLive=false;
 Unhook();
}

void FS47ShotCamera::Tick()
{
 if(!bLive&&Hooked.IsValid())Unhook();
}

void FS47ShotCamera::Unhook()
{
 if(AS47Walker* W=Hooked.Get())W->CameraOverride=Before;
 Hooked.Reset();
 Before=nullptr;
}

FS47ArrayShot::FPose FS47ShotCamera::LiveView() const
{
 // The walker's own view as its frame places it (without the bob: it stands still while the camera is held), as the array shot's.
 FS47ArrayShot::FPose P;
 if(const AS47Walker* W=Hooked.Get()){P.Eye=FVector(W->Pos.X,W->FloorY+W->Eye,W->Pos.Z);P.Yaw=W->Yaw;P.Pitch=W->Pitch;}
 P.Fov=Fov0;
 return P;
}

bool FS47ShotCamera::Frame(double Dt,FS47View& Out)
{
 AS47Walker* W=Hooked.Get();
 if(!bLive||!W)return false;
 T+=FMath::Max(0.,Dt);
 if(T>=Spec.In+Spec.Secs+Spec.Out)
 {
  // The walker's own from this frame on (Tick puts its camera back in its hands).
  bLive=false;
  W->Fov=float(Fov0);
  UE_LOG(LogS47Shot,Display,TEXT("S47: director shot over after %.1f s"),T);
  return false;
 }
 const double K=S47Shot::Weight(T,Spec.In,Spec.Secs,Spec.Out);
 const FS47ArrayShot::FPose P=FS47ArrayShot::Blend(LiveView(),S47Shot::PoseAt(Spec,T-Spec.In),K,0.);
 Out.Eye=P.Eye;Out.Yaw=P.Yaw;Out.Pitch=P.Pitch;Out.Roll=P.Roll;
 W->Fov=float(P.Fov);
 return true;
}

bool S47Shot::SelfTest(TArray<FString>& Lines)
{
 bool bAll=true;
 int32 N=0;
 const auto Check=[&](bool bOk,const FString& What){bAll&=bOk;Lines.Add(FString::Printf(TEXT("%s SHOT-%d: %s"),bOk?TEXT("PASS"):TEXT("FAIL"),++N,*What));};
 const auto Fwd=[](const FS47ArrayShot::FPose& P){return FVector(-FMath::Sin(P.Yaw)*FMath::Cos(P.Pitch),FMath::Sin(P.Pitch),-FMath::Cos(P.Yaw)*FMath::Cos(P.Pitch));};
 const auto Off=[&Fwd](const FS47ArrayShot::FPose& P,const FVector& At){return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Fwd(P),(At-P.Eye).GetSafeNormal()),-1.,1.)));};
 FS47ShotSpec S;
 S.EyeA=FVector(10,1.7,-20);S.EyeB=FVector(30,4,-10);S.Target=FVector(-6,9,-70);S.FovA=55;S.FovB=35;S.Secs=6;S.In=.6;S.Out=.8;S.bMove=true;
 // ---- the pose ----
 {
  const FS47ArrayShot::FPose A=PoseAt(S,0),Mid=PoseAt(S,3),B=PoseAt(S,6);
  Check(FVector::Dist(A.Eye,S.EyeA)<1e-9&&FMath::IsNearlyEqual(A.Fov,55.),TEXT("at the hold's start the first eye and lens"));
  Check(FVector::Dist(Mid.Eye,(S.EyeA+S.EyeB)*.5)<1e-9&&FMath::IsNearlyEqual(Mid.Fov,45.),TEXT("half way through the hold half way along, the lens half way"));
  Check(FVector::Dist(B.Eye,S.EyeB)<1e-9&&FMath::IsNearlyEqual(B.Fov,35.),TEXT("at the hold's end the second eye and lens"));
  Check(FVector::Dist(PoseAt(S,-2).Eye,S.EyeA)<1e-9&&FVector::Dist(PoseAt(S,9).Eye,S.EyeB)<1e-9,TEXT("before and after the hold the end poses"));
  const double OffA=Off(A,S.Target),OffM=Off(Mid,S.Target),OffB=Off(B,S.Target);
  Check(OffA<.01&&OffM<.01&&OffB<.01,FString::Printf(TEXT("always looking at the point (%.3f, %.3f, %.3f degrees off)"),OffA,OffM,OffB));
  double Yaw=0,Pitch=0;S47CameraMoment::LookAt(Mid.Eye,S.Target,Yaw,Pitch);
  Check(FMath::IsNearlyEqual(Mid.Yaw,Yaw,1e-9)&&FMath::IsNearlyEqual(Mid.Pitch,Pitch,1e-9)&&Mid.Roll==0,TEXT("the yaw and pitch are the camera moment's look-at, no roll"));
  const double Q1=FVector::Dist(PoseAt(S,.3).Eye,S.EyeA),Q2=FVector::Dist(PoseAt(S,.6).Eye,PoseAt(S,.3).Eye);
  Check(Q1<Q2,TEXT("the move eases away from the first pose (slow at first)"));
  FS47ShotSpec Still=S;Still.bMove=false;
  Check(FVector::Dist(PoseAt(Still,4).Eye,S.EyeA)<1e-9&&FMath::IsNearlyEqual(PoseAt(Still,4).Fov,55.),TEXT("without a move the first pose holds"));
 }
 // ---- the weight ----
 {
  bool bUp=true,bDown=true,bHeld=true;double Last=0;
  for(double X=0;X<=.6;X+=.01){const double K=Weight(X,.6,6,.8);bUp&=K>=Last-1e-12;Last=K;}
  for(double X=.6;X<6.6;X+=.05)bHeld&=Weight(X,.6,6,.8)==1;
  Last=1;for(double X=6.6;X<=7.4;X+=.01){const double K=Weight(X,.6,6,.8);bDown&=K<=Last+1e-12;Last=K;}
  Check(bUp&&bHeld&&bDown,TEXT("the weight only rises in, is 1 through the hold, only falls out"));
  Check(Weight(0,.6,6,.8)==0&&Weight(7.4,.6,6,.8)==0&&Weight(9,.6,6,.8)==0&&Weight(.01,.6,6,.8)<.01&&Weight(7.39,.6,6,.8)<.01,TEXT("no cut: nothing at the start and at the end, nearly nothing next to them"));
 }
 // ---- the reduced motion ----
 {
  const FS47ShotSpec R=Reduced(S);
  Check(!R.bMove&&FVector::Dist(PoseAt(R,6).Eye,S.EyeA)<1e-9&&FMath::IsNearlyEqual(PoseAt(R,6).Fov,55.)&&R.In>=ReducedFade&&R.Out>=ReducedFade&&R.Secs==S.Secs&&R.Target==S.Target,TEXT("reduced motion: no move, the first pose held, eased in and out over 0.6 s at least"));
  FS47ShotSpec Slow=S;Slow.In=2;Slow.Out=2.5;
  Check(Reduced(Slow).In==2&&Reduced(Slow).Out==2.5,TEXT("reduced motion keeps a slower ease as it is"));
 }
 // ---- the limits ----
 {
  FString Why;
  Check(Sane(S,Why)&&Why.IsEmpty(),TEXT("a good shot keeps to the limits"));
  const auto Refused=[&Why,&S](TFunctionRef<void(FS47ShotSpec&)> Spoil){FS47ShotSpec B=S;Spoil(B);return !Sane(B,Why)&&!Why.IsEmpty();};
  Check(Refused([](FS47ShotSpec& X){X.Secs=0;})&&Refused([](FS47ShotSpec& X){X.Secs=31;})&&Refused([](FS47ShotSpec& X){X.Secs=std::numeric_limits<double>::quiet_NaN();}),TEXT("held for nothing, past 30 s or for a NaN is refused"));
  Check(Refused([](FS47ShotSpec& X){X.FovA=15;})&&Refused([](FS47ShotSpec& X){X.FovB=95;}),TEXT("a lens outside 20 to 90 degrees is refused"));
  Check(Refused([](FS47ShotSpec& X){X.In=.1;})&&Refused([](FS47ShotSpec& X){X.Out=4;}),TEXT("an ease in or out outside 0.3 to 3 s is refused (no cut)"));
  Check(Refused([](FS47ShotSpec& X){X.EyeA=X.Target;})&&Refused([](FS47ShotSpec& X){X.EyeB=X.Target+FVector(.1,0,0);}),TEXT("an eye on the point it looks at is refused"));
  Check(Refused([](FS47ShotSpec& X){X.EyeB=X.EyeA+FVector(250,0,0);})&&Refused([](FS47ShotSpec& X){X.Target.X=std::numeric_limits<double>::quiet_NaN();}),TEXT("eyes 250 m apart, or a point that is not a number, are refused"));
  FS47ShotSpec Far=S;Far.bMove=false;Far.EyeB=Far.EyeA+FVector(500,0,0);
  Check(Sane(Far,Why),TEXT("without a move the second eye is not asked about"));
 }
 return bAll;
}

// ---- the F10 line and the in-game check ----

namespace S47ShotLocal
{
 // A shot by the player (road-local, so it works in whichever area they are): from over the right shoulder, moving slowly to the
 // left, onto a point 20 m ahead. Off, so it never plays by a trigger.
 FS47Script DevScript(const FS47ScriptPoint& Here,double Yaw)
 {
  FS47Script S;S.Id=DevId;S.Name=TEXT("Director shot (F10)");S.bEnabled=false;
  const FVector2D Fw(-FMath::Sin(Yaw),-FMath::Cos(Yaw)),Rt(FMath::Cos(Yaw),-FMath::Sin(Yaw));
  const auto At=[&Here](const FVector2D& D,double Up){FS47ScriptPoint P;P.X=Here.X+D.X;P.Z=Here.Z+D.Y;P.Up=Up;P.bSet=true;return P;};
  FS47ScriptAction A;A.Do=ES47ScriptAct::Shot;A.At=0;
  A.Path={At(Rt*3-Fw*2,2.4),At(-Rt*3-Fw*1,3.)};
  A.Point=At(Fw*20,1.2);
  A.Fov=50;A.Fov2=35;A.Secs=4;A.In=.6;A.Out=.8;
  S.Actions.Add(A);
  return S;
 }
}

void S47Shot::AddDevLine(US47Main* M)
{
 using namespace S47ShotLocal;
 if(!M||!M->DevEvents||!FS47ScriptPlayer::DevAllowed(M))return;
 if(M->DevEvents->Events().ContainsByPredicate([](const FS47DevEvents::FEvent& E){return E.Group==DevGroup;}))return;
 M->DevEvents->AddFreeform(DevGroup,TEXT("Preview a director shot"),TEXT("SHOT-BEAT-01: the camera from over your shoulder onto a point ahead, then back; you stay where you are"),[M]
 {
  FS47ScriptPlayer* P=FS47ScriptPlayer::Of(M);
  FS47ScriptPoint Here;
  FString Why;
  bool bOk=false;
  if(!P)Why=TEXT("no script player here");
  else if(!P->HereRoad(Here))Why=TEXT("no player on the land");
  else bOk=P->Preview(DevScript(Here,M->Player?M->Player->Yaw:0.),Why);
  UE_LOG(LogS47Shot,Display,TEXT("S47: developer event director shot: %s%s%s"),bOk?TEXT("previewed"):TEXT("not played"),Why.IsEmpty()?TEXT(""):TEXT(", "),*Why);
  if(M->UI)M->UI->Toast(bOk?FString(TEXT("Developer event: director shot.")):TEXT("Developer event: no shot: ")+Why,3);
 });
}

bool S47Shot::Contract(US47Main* M,double Dt)
{
 using namespace S47ShotLocal;
 struct FRun{int32 Stage=0;double T=0,At=0;FVector Pos=FVector::ZeroVector;bool bBefore=false,bDone=false;float Fov0=70;};
 static FRun R;
 if(R.bDone)return true;
 AS47Walker* W=M?M->Player.Get():nullptr;
 // (R is static: used here, not captured)
 const auto Finish=[](bool bGood,const FString& What)
 {
  R.bDone=true;
  if(bGood)UE_LOG(LogS47Shot,Display,TEXT("S47: director shot contract COMPLETE PASS"));
  else UE_LOG(LogS47Shot,Display,TEXT("S47: director shot contract FAIL %s"),*What);
  FPlatformMisc::RequestExitWithStatus(false,bGood?0:1);
  return true;
 };
 R.T+=FMath::Max(0.,Dt);
 FS47ScriptPlayer* P=FS47ScriptPlayer::Of(M);
 if(R.Stage==0)
 {
  // The night settled and the player on foot with the camera their own; at most a minute for it.
  const bool bReady=W&&P&&M->DevEvents&&(!M->World||!M->World->bDriving)&&!W->CameraOverride&&!P->Running();
  if(!bReady||R.T<6)
  {
   if(R.T>60)return Finish(false,!W||!P?FString(TEXT("no walker or no script player (not in Freeform?)")):W->CameraOverride?FString(TEXT("the camera is held by something else before the shot")):FString(TEXT("at the wheel, or a script is playing")));
   return false;
  }
  R.Pos=W->Pos;R.bBefore=bool(W->CameraOverride);R.Fov0=W->Fov;
  const FS47DevEvents::FEvent* E=M->DevEvents->Events().FindByPredicate([](const FS47DevEvents::FEvent& X){return X.Group==DevGroup;});
  if(!E||!E->Fn)return Finish(false,TEXT("no F10 line in the group Director shot (not a developer's run?)"));
  UE_LOG(LogS47Shot,Display,TEXT("S47: director shot contract: %s / %s"),*E->Group,*E->Label);
  E->Fn();
  if(!P->Running()||P->RunningId()!=DevId)return Finish(false,TEXT("the script player did not take the built-in shot (see the line above)"));
  R.Stage=1;R.At=R.T;
  return false;
 }
 if(!W||!P)return Finish(false,TEXT("the walker or the script player went"));
 const TSharedPtr<FS47ShotCamera> Shot=FS47ShotCamera::LastStarted();
 if(R.Stage==1)
 {
  // The shot starts on the script's first step; from then, half way through its hold.
  if(!Shot||!Shot->Live())
  {
   if(R.T-R.At>10)return Finish(false,TEXT("the shot never went live (a menu held the timeline, or it was skipped: see S47: script shot skipped)"));
   return false;
  }
  const FS47ShotSpec& S=Shot->Playing();
  if(Shot->Age()<S.In+S.Secs*.5)return false;
  if(!W->CameraOverride)return Finish(false,TEXT("(a) no CameraOverride mid-shot"));
  if(FVector::Dist(W->Pos,R.Pos)>1e-3)return Finish(false,FString::Printf(TEXT("(b) the player moved %.3f m"),FVector::Dist(W->Pos,R.Pos)));
  // The eye the walker's camera was put at (its actor, S47Walker's override branch), on the way from the first eye to the second.
  const FVector Eye=S47Three::ToThree(W->GetActorLocation());
  const double Miss=FMath::PointDistToSegment(Eye,S.EyeA,S.bMove?S.EyeB:S.EyeA);
  if(Miss>.25)return Finish(false,FString::Printf(TEXT("(c) the camera eye %.2f %.2f %.2f is %.2f m off the shot's"),Eye.X,Eye.Y,Eye.Z,Miss));
  UE_LOG(LogS47Shot,Display,TEXT("S47: director shot contract: mid-shot held, player still, eye %.2f m from the shot's"),Miss);
  R.Stage=2;R.At=R.T;
  return false;
 }
 // Over: the script done and the camera given back as it was.
 if((Shot&&Shot->Live())||P->Running())
 {
  if(R.T-R.At>20)return Finish(false,TEXT("(d) the shot did not end"));
  return false;
 }
 if(bool(W->CameraOverride)!=R.bBefore)return Finish(false,TEXT("(d) the CameraOverride is not back to what it was"));
 if(!FMath::IsNearlyEqual(W->Fov,R.Fov0,.01f))return Finish(false,FString::Printf(TEXT("(d) the field of view is %.2f, not %.2f"),W->Fov,R.Fov0));
 if(FVector::Dist(W->Pos,R.Pos)>1e-3)return Finish(false,TEXT("(b) the player moved after the shot"));
 return Finish(true,FString());
}
