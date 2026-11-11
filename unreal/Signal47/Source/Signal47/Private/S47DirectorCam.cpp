#include "S47DirectorCam.h"
#include "CoreGlobals.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "S47Abduction.h"
#include "S47Attention.h"
#include "S47Away.h"
#include "S47DevEvents.h"
#include "S47DoorLoop.h"
#include "S47FarmSite.h"
#include "S47Freeform.h"
#include "S47IndoorDread.h"
#include "S47Keys.h"
#include "S47Main.h"
#include "S47Script.h"
#include "S47ScriptPlayer.h"
#include "S47Shot.h"
#include "S47Taken.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47World.h"
#include "SS47ScriptEditor.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47DirectorCam,Log,All);

// DIRECTOR-MODE-01, slice 2: the director's free camera (S47DirectorCam.h): one step of flying, the hold on the walker's camera, the
// night held meanwhile, and the number keys that place into the script editor's draft.

namespace S47DirectorCamLocal
{
 const TCHAR* const HoldName=TEXT("director");
 const TCHAR* const DevGroup=TEXT("Director mode");
 // One game at a time on this machine, as the editor's draft.
 struct FState
 {
  bool bOn=false;
  const US47Main* Owner=nullptr;
  TWeakObjectPtr<AS47Walker> Walker;
  // whatever held the walker's camera before (US47World's hook, which answers false on foot): put back on leaving
  TFunction<bool(double,const FVector2D&,const FVector2D&,FS47View&)> Before;
  S47DirectorCam::FFly Fly;
  uint64 HookFrame=0;  // the last frame the walker asked the hook (GFrameCounter)
  int32 Gen=0;         // counts up with every new text on the screen and with leaving: the text before goes
  FString Shown,Last;  // the text on the screen now; the last thing placed, or why nothing was
  TOptional<FVector> TestInput; // the contract's keys instead of the real ones: x right, y rise, z back
 };
 FState& St(){static FState S;return S;}

 FVector Forward(const S47DirectorCam::FFly& F){return FVector(-FMath::Sin(F.Yaw)*FMath::Cos(F.Pitch),FMath::Sin(F.Pitch),-FMath::Cos(F.Yaw)*FMath::Cos(F.Pitch));}
 FString KeyOf(const TCHAR* Action){const FString K=S47Keys::Label(Action);return K.IsEmpty()?FString(TEXT("-")):K;}

 // The walker's camera, each frame while director mode is on: the keys and the look fly it (the player stands where they stood).
 bool Frame(double Dt,const FVector2D& Move,const FVector2D& LookIn,FS47View& Out)
 {
  FState& S=St();
  AS47Walker* W=S.Walker.Get();
  if(!S.bOn||!W)return false;
  S.HookFrame=GFrameCounter;
  FVector2D Mv=Move;
  double Rise=0;
  bool bFast=false;
  if(S.TestInput.IsSet()){Mv=FVector2D(S.TestInput->X,S.TestInput->Z);Rise=S.TestInput->Y;}
  else if(W->IsActive())if(const APlayerController* PC=Cast<APlayerController>(W->GetController()))
  {
   // the walker's own keys as bound in Settings > Controls: Jump up, Keep low down, Run fast
   Rise=(S47Keys::Down(PC,TEXT("jump"))?1.:0.)-(S47Keys::Down(PC,TEXT("crouch"))?1.:0.);
   bFast=S47Keys::Down(PC,TEXT("run"));
  }
  S.Fly=S47DirectorCam::Step(S.Fly,Mv,Rise,LookIn,bFast?S47DirectorCam::FastMps:S47DirectorCam::FlyMps,Dt);
  Out.Eye=S.Fly.Eye;Out.Yaw=S.Fly.Yaw;Out.Pitch=S.Fly.Pitch;Out.Roll=0;
  return true;
 }

 // Why director mode may not begin now (empty: it may). A developer's run is asked by the caller.
 FString WhyNot(US47Main* M)
 {
  AS47Walker* W=M->Player.Get();
  FS47ScriptPlayer* P=FS47ScriptPlayer::Of(M);
  const FS47FreeformSession* F=M->Freeform.Get();
  if(M->Mode!=US47Main::EMode::Play||M->bPausedByMenu)return TEXT("not in play");
  if(!M->IsFreeform()||!F||!F->Active()||!P)return TEXT("Freeform only");
  if(!W||!W->IsActive())return TEXT("the player is not in your hands");
  if(M->World&&M->World->bDriving)return TEXT("at the wheel");
  if(M->ModalOpen())return TEXT("a panel is open");
  if(W->bCameraHeld)return TEXT("the camera is held by something else");
  // Nothing dangerous is cut off half way: director mode waits until it is over.
  if((F->Taken&&F->Taken->Busy())||(M->Away&&M->Away->Abduction&&M->Away->Abduction->Busy()))return TEXT("being taken");
  if(F->Attention&&F->Attention->Visiting())return TEXT("a visit at SARO is under way");
  if(F->DoorLoop&&F->DoorLoop->Active())return TEXT("the door loop is under way");
  if(F->FarmSite&&F->FarmSite->VisitWantsGround())return TEXT("a visit is out for the player");
  if(F->IndoorDread&&F->IndoorDread->DoorActive())return TEXT("a door visit is under way");
  if(P->Running())return TEXT("a script is playing: ")+P->RunningId();
  FString Big;
  if(FS47ScriptPlayer::BigEvent(M,Big))return TEXT("a big event holds the night: ")+Big;
  if(M->OneAtATime())return TEXT("something else holds the night");
  if(F->Held())return TEXT("the night is held");
  return FString();
 }

 // Why director mode must end now (empty: it goes on).
 FString WhyOff(US47Main* M)
 {
  const FState& S=St();
  if(!FS47ScriptPlayer::DevAllowed(M))return TEXT("not a developer's run");
  if(M->Mode!=US47Main::EMode::Play)return TEXT("out of play");
  if(!M->IsFreeform()||!M->Freeform||!M->Freeform->Active()||!FS47ScriptPlayer::Of(M))return TEXT("the session stopped");
  const AS47Walker* W=S.Walker.Get();
  if(!W||W!=M->Player.Get())return TEXT("the player went");
  if(M->ModalOpen()||M->bPausedByMenu)return TEXT("a panel opened");
  if((M->Freeform->Taken&&M->Freeform->Taken->Busy())||(M->Away&&M->Away->Abduction&&M->Away->Abduction->Busy()))return TEXT("taken");
  if(M->World&&M->World->bDriving)return TEXT("at the wheel");
  // (the walker asks its camera every frame, also while it is not in the player's hands: silence means another hook took it)
  if(GFrameCounter>S.HookFrame+30)return TEXT("something else took the camera");
  return FString();
 }

 // As the editor's NEW SCRIPT, when the editor has never been opened.
 void EnsureDraft()
 {
  if(S47ScriptEditor::HaveDraft())return;
  FS47Script N;
  N.Id=FString::Printf(TEXT("script_%s"),*FDateTime::Now().ToString(TEXT("%m%d_%H%M%S")));
  N.Name=TEXT("New script");
  N.bEnabled=false;
  N.Trigger.HourFrom=23;N.Trigger.HourTo=4;
  S47ScriptEditor::Draft()=N;S47ScriptEditor::HaveDraft()=true;S47ScriptEditor::Dirty()=true;
 }

 // The camera's eye as a shot's eye: on the land, 0 to 300 m over it (the lint's). False with Said: nothing to store.
 bool EyeHere(US47Main* M,FS47ScriptPoint& Eye,FString& Said)
 {
  const FState& S=St();
  if(!FS47ScriptPlayer::RoadOfWorld(M,S.Fly.Eye,Eye)){Said=TEXT("the camera is not over the land of this area (indoors?)");return false;}
  if(!(Eye.Up>=0&&Eye.Up<=300)){Said=FString::Printf(TEXT("the camera is %.1f m over the ground, not 0 to 300"),Eye.Up);return false;}
  Eye.Up=FMath::RoundToDouble(Eye.Up*100)/100;
  return true;
 }

 // Key 1: a shot at the end of the draft's timeline, from the camera onto what the crosshair is on (or a point 20 m ahead).
 bool PlaceShot(US47Main* M,FString& Said)
 {
  const FState& S=St();
  FS47ScriptPoint Eye,At;
  FString Why;
  if(!EyeHere(M,Eye,Why)){Said=TEXT("no shot: ")+Why;return false;}
  if(!FS47ScriptPlayer::RayRoad(M,S.Fly.Eye,S.Fly.Yaw,S.Fly.Pitch,At))
  {
   if(!FS47ScriptPlayer::RoadOfWorld(M,S.Fly.Eye+Forward(S.Fly)*20.,At)){Said=TEXT("no shot: nothing on the land ahead to look at");return false;}
   At.Up=FMath::Clamp(FMath::RoundToDouble(At.Up*10)/10,0.,300.);
  }
  if(!(FVector::Dist(FS47ScriptPlayer::WorldOfRoad(M,At),S.Fly.Eye)>=S47Shot::LeastReach)){Said=TEXT("no shot: the crosshair is on something right at the camera");return false;}
  EnsureDraft();
  FS47Script& D=S47ScriptEditor::Draft();
  FS47ScriptAction A;
  A.Do=ES47ScriptAct::Shot;
  A.At=D.Actions.Num()?FMath::RoundToDouble(D.Length()*10)/10:0.;
  A.Path.Add(Eye);
  A.Point=At;
  A.Fov=55;A.Secs=5;
  D.Actions.Add(A);
  S47ScriptEditor::Dirty()=true;
  Said=FString::Printf(TEXT("shot %d at %.1f s from (%.0f, %.0f) +%.1f m"),D.Actions.Num(),A.At,Eye.X,Eye.Z,Eye.Up);
  return true;
 }

 // Key 2: the last shot's second eye here, so that it moves.
 bool PlaceEye2(US47Main* M,FString& Said)
 {
  const FState& S=St();
  int32 I=INDEX_NONE;
  if(S47ScriptEditor::HaveDraft())for(int32 K=S47ScriptEditor::Draft().Actions.Num()-1;K>=0;--K)if(S47ScriptEditor::Draft().Actions[K].Do==ES47ScriptAct::Shot){I=K;break;}
  if(I==INDEX_NONE){Said=TEXT("no shot to move: place one with 1 first");return false;}
  FS47ScriptAction& A=S47ScriptEditor::Draft().Actions[I];
  if(A.Path.IsEmpty()){Said=FString::Printf(TEXT("shot %d has no first eye"),I+1);return false;}
  FS47ScriptPoint Eye;
  FString Why;
  if(!EyeHere(M,Eye,Why)){Said=TEXT("no move: ")+Why;return false;}
  const double Apart=FVector::Dist(FS47ScriptPlayer::WorldOfRoad(M,A.Path[0]),S.Fly.Eye);
  if(!(Apart<=S47Shot::MostTravel)){Said=FString::Printf(TEXT("no move: %.0f m from the first eye, more than %.0f"),Apart,S47Shot::MostTravel);return false;}
  if(A.Point.bSet&&!(FVector::Dist(FS47ScriptPlayer::WorldOfRoad(M,A.Point),S.Fly.Eye)>=S47Shot::LeastReach)){Said=TEXT("no move: on the point the shot looks at");return false;}
  if(A.Path.Num()<2)A.Path.Add(Eye);else A.Path[1]=Eye;
  S47ScriptEditor::Dirty()=true;
  Said=FString::Printf(TEXT("shot %d moves %.0f m to (%.0f, %.0f) +%.1f m"),I+1,Apart,Eye.X,Eye.Z,Eye.Up);
  return true;
 }

 // Key 3: the trigger's point on the ground under the crosshair (its radius as it was).
 bool PlaceTrigger(US47Main* M,FString& Said)
 {
  const FState& S=St();
  FS47ScriptPoint P;
  if(!FS47ScriptPlayer::RayRoad(M,S.Fly.Eye,S.Fly.Yaw,S.Fly.Pitch,P)){Said=TEXT("no trigger point: nothing on the land under the crosshair within 400 m");return false;}
  P.Up=0;
  EnsureDraft();
  FS47ScriptTrigger& T=S47ScriptEditor::Draft().Trigger;
  T.Place.Reset();
  T.Point=P;
  S47ScriptEditor::Dirty()=true;
  Said=FString::Printf(TEXT("the trigger's point (%.0f, %.0f), within %.0f m"),P.X,P.Z,T.Radius);
  return true;
 }

 // Key 4: out of director mode (the player has not moved), and the draft played from the top.
 void PreviewDraft(US47Main* M)
 {
  S47DirectorCam::Leave(M);
  FS47ScriptPlayer* P=FS47ScriptPlayer::Of(M);
  FString Why;
  const bool bOk=P&&S47ScriptEditor::HaveDraft()&&P->Preview(S47ScriptEditor::Draft(),Why);
  if(!bOk&&Why.IsEmpty())Why=P?TEXT("nothing to play yet"):TEXT("no script player here");
  UE_LOG(LogS47DirectorCam,Display,TEXT("S47: director mode preview: %s%s%s"),bOk?TEXT("played"):TEXT("not played"),Why.IsEmpty()?TEXT(""):TEXT(", "),*Why);
  if(M->UI)M->UI->Toast(bOk?TEXT("Preview: ")+S47ScriptEditor::Draft().Name+(Why.IsEmpty()?FString():TEXT(" (")+Why+TEXT(")")):TEXT("Director mode off; not played: ")+Why,4);
 }

 FString Text()
 {
  const FState& S=St();
  const FString Name=S47ScriptEditor::HaveDraft()?S47ScriptEditor::Draft().Name:FString(TEXT("a new script"));
  const FString Walk=KeyOf(TEXT("forward"))+KeyOf(TEXT("left"))+KeyOf(TEXT("back"))+KeyOf(TEXT("right"));
  return FString::Printf(TEXT("DIRECTOR MODE: %s. 1 a shot from here, 2 the last shot moves to here, 3 the trigger's point, 4 preview, %s back. %s fly, %s up, %s down, %s fast.%s%s"),
   *Name,*KeyOf(TEXT("director")),*Walk,*KeyOf(TEXT("jump")),*KeyOf(TEXT("crouch")),*KeyOf(TEXT("run")),S.Last.IsEmpty()?TEXT(""):TEXT(" Last: "),*S.Last);
 }

 // One steady block of text: a caption held on a clock that stands still while it is director mode's text now, and runs out at once
 // when it is not (a new text, or leaving). Put up again only when the words change: no blinking.
 void Show(US47Main* M)
 {
  FState& S=St();
  const FString Words=Text();
  if(Words==S.Shown||!M->UI)return;
  S.Shown=Words;
  const int32 Gen=++S.Gen;
  M->UI->Toast(Words,3600.f,[Gen]{const FState& X=St();return X.bOn&&X.Gen==Gen?0.:1e9;});
 }

 // DIRECTOR-MODE-01, the next slice: the trigger's volume, the shots' eyes and what they look at, drawn in the world while flying.
 // Steady lines, drawn again each frame (no blinking): the draft's trigger volume in amber (ground outline, and its top and corner
 // posts when it has a height), each shot's eye in cyan with a line to what it looks at, and its second eye in white.
 void Draw(US47Main* M)
 {
  const AS47Walker* Wk=St().Walker.Get();
  UWorld* World=Wk?Wk->GetWorld():nullptr;
  if(!M||!World||!S47ScriptEditor::HaveDraft())return;
  const FS47Script& D=S47ScriptEditor::Draft();
  const auto At=[M](const FS47ScriptPoint& P){return S47Three::ToUE(FS47ScriptPlayer::WorldOfRoad(M,P));};
  const FS47ScriptTrigger& T=D.Trigger;
  if(T.Point.bSet)
  {
   static FS47ScriptPoint Logged;
   if(Logged.X!=T.Point.X||Logged.Z!=T.Point.Z){Logged=T.Point;const FVector C=At(T.Point);UE_LOG(LogS47DirectorCam,Display,TEXT("S47: director mode draws the trigger at %.0f %.0f %.0f (UE cm), %d outline points"),C.X,C.Y,C.Z,S47Script::Outline(T,32).Num());}
   const FColor Amber(255,176,32);
   const TArray<FS47ScriptPoint> Ring=S47Script::Outline(T,32);
   const double Tall=T.Height>0?T.Height:2.; // (any height: a low fence, so the outline is seen over rough ground)
   const int32 Posts=T.Shape==ES47ScriptShape::Box?1:8;
   for(int32 I=0;I+1<Ring.Num();++I)
   {
    FS47ScriptPoint A=Ring[I],B=Ring[I+1];
    DrawDebugLine(World,At(A),At(B),Amber,false,-1.f,0,6.f);
    FS47ScriptPoint A2=A,B2=B;A2.Up+=Tall;B2.Up+=Tall;
    DrawDebugLine(World,At(A2),At(B2),Amber,false,-1.f,0,T.Height>0?6.f:2.f);
    if(I%Posts==0)DrawDebugLine(World,At(A),At(A2),Amber,false,-1.f,0,2.f);
   }
   DrawDebugPoint(World,At(T.Point),14.f,Amber,false,-1.f);
  }
  for(const FS47ScriptAction& A:D.Actions)
  {
   if(A.Do!=ES47ScriptAct::Shot||A.Path.IsEmpty()||!A.Path[0].bSet)continue;
   const FVector Eye=At(A.Path[0]);
   if(FVector::Dist(Eye,Wk->GetActorLocation())<150.)continue; // (the camera stands in it: its lines would cross the whole view)
   DrawDebugSphere(World,Eye,35.f,10,FColor::Cyan,false,-1.f,0,3.f);
   if(A.Point.bSet){DrawDebugLine(World,Eye,At(A.Point),FColor::Cyan,false,-1.f,0,3.f);DrawDebugPoint(World,At(A.Point),14.f,FColor::Cyan,false,-1.f);}
   if(A.Path.Num()>1&&A.Path[1].bSet){DrawDebugSphere(World,At(A.Path[1]),35.f,10,FColor::White,false,-1.f,0,3.f);DrawDebugLine(World,Eye,At(A.Path[1]),FColor::White,false,-1.f,0,2.f);}
  }
 }

 void Enter(US47Main* M)
 {
  FState& S=St();
  AS47Walker* W=M->Player.Get();
  S.bOn=true;S.Owner=M;S.Walker=W;S.HookFrame=GFrameCounter;S.Shown.Reset();S.Last.Reset();S.TestInput.Reset();
  // from the player's eye and view (the actor is the eye on foot)
  S.Fly.Eye=S47Three::ToThree(W->GetActorLocation());
  S.Fly.Yaw=W->Yaw;
  S.Fly.Pitch=FMath::Clamp(W->Pitch,-S47DirectorCam::PitchLimit,S47DirectorCam::PitchLimit);
  // The walker's camera, held as the director shot holds it; whatever held it before has it back after.
  S.Before=W->CameraOverride;
  TFunction<bool(double,const FVector2D&,const FVector2D&,FS47View&)> Prev=S.Before;
  W->CameraOverride=[Prev](double Dt,const FVector2D& Move,const FVector2D& LookIn,FS47View& Out)
  {
   if(Frame(Dt,Move,LookIn,Out))return true;
   return Prev?Prev(Dt,Move,LookIn,Out):false;
  };
  FS47ScriptPlayer::Hold(HoldName,true);
  EnsureDraft();
  UE_LOG(LogS47DirectorCam,Display,TEXT("S47: director mode on at %.1f %.1f %.1f, the player at %.1f %.1f, draft %s"),S.Fly.Eye.X,S.Fly.Eye.Y,S.Fly.Eye.Z,W->Pos.X,W->Pos.Z,*S47ScriptEditor::Draft().Id);
  Show(M);
 }
}

namespace S47DirectorCam
{
FFly Step(const FFly& In,const FVector2D& Move,double Rise,const FVector2D& LookIn,double Mps,double Dt)
{
 FFly O=In;
 // the look as the walker's own (AS47Walker::Look), the pitch held short of straight up and down
 O.Yaw=In.Yaw-LookIn.X;
 O.Pitch=FMath::Clamp(In.Pitch-LookIn.Y,-PitchLimit,PitchLimit);
 const FVector2D Mv=Move.SizeSquared()>1?Move.GetSafeNormal():Move;
 const double Up=FMath::Clamp(Rise,-1.,1.);
 const double Sy=FMath::Sin(O.Yaw),Cy=FMath::Cos(O.Yaw),Sp=FMath::Sin(O.Pitch),Cp=FMath::Cos(O.Pitch);
 // forward along the view, pitch and all; right level, as the walker steps (x right, z back)
 const FVector Fwd(-Sy*Cp,Sp,-Cy*Cp),Right(Cy,0,-Sy);
 O.Eye=In.Eye+(Right*Mv.X-Fwd*Mv.Y+FVector(0,Up,0))*Mps*FMath::Max(0.,Dt);
 return O;
}

bool SelfTest(TArray<FString>& Lines)
{
 bool bAll=true;
 const auto Check=[&](bool bOk,const FString& What){bAll&=bOk;Lines.Add(FString::Printf(TEXT("%s DIRECTOR-2: %s"),bOk?TEXT("PASS"):TEXT("FAIL"),*What));};
 FFly F;F.Eye=FVector(10,3,-20);F.Yaw=.7;F.Pitch=.3;
 const FVector Fwd(-FMath::Sin(F.Yaw)*FMath::Cos(F.Pitch),FMath::Sin(F.Pitch),-FMath::Cos(F.Yaw)*FMath::Cos(F.Pitch));
 const FVector2D No=FVector2D::ZeroVector;
 {
  const FFly A=Step(F,No,0,No,FlyMps,1);
  Check(A.Eye==F.Eye&&A.Yaw==F.Yaw&&A.Pitch==F.Pitch,TEXT("no keys and no look: the camera stays put"));
 }
 {
  const FVector Ahead=Step(F,FVector2D(0,-1),0,No,6,.5).Eye-F.Eye,Behind=Step(F,FVector2D(0,1),0,No,6,.5).Eye-F.Eye;
  Check(FVector::Dist(Ahead,Fwd*3)<1e-9,FString::Printf(TEXT("forward flies along the view, its pitch too (%.3f m of 3)"),Ahead.Size()));
  Check(FVector::Dist(Behind,-Fwd*3)<1e-9,TEXT("back flies the other way"));
 }
 {
  const FVector Side=Step(F,FVector2D(1,0),0,No,6,.5).Eye-F.Eye;
  Check(FMath::Abs(FVector::DotProduct(Side,Fwd))<1e-9&&FMath::Abs(Side.Y)<1e-12&&FMath::IsNearlyEqual(Side.Size(),3.,1e-9),TEXT("strafe is sideways and level, square to the view"));
  Check(FVector::Dist(Step(FFly(),FVector2D(1,0),0,No,1,1).Eye,FVector(1,0,0))<1e-12,TEXT("facing north (-z), right is east (+x), as the walker steps"));
 }
 {
  const FVector Rise=Step(F,No,1,No,6,.5).Eye-F.Eye,Sink=Step(F,No,-1,No,6,.5).Eye-F.Eye,Much=Step(F,No,5,No,6,.5).Eye-F.Eye;
  Check(FVector::Dist(Rise,FVector(0,3,0))<1e-12&&FVector::Dist(Sink,FVector(0,-3,0))<1e-12,TEXT("rise is straight up and down, whatever the view"));
  Check(FVector::Dist(Much,FVector(0,3,0))<1e-12,TEXT("rise is held to -1 to 1"));
 }
 {
  const FFly Up=Step(F,No,0,FVector2D(0,-10),6,.1),Down=Step(F,No,0,FVector2D(0,10),6,.1),Turn=Step(F,No,0,FVector2D(.2,.1),6,.1);
  Check(Up.Pitch==PitchLimit&&Down.Pitch==-PitchLimit,TEXT("looking far up or down stops at the pitch limit"));
  Check(FMath::IsNearlyEqual(Turn.Yaw,F.Yaw-.2,1e-12)&&FMath::IsNearlyEqual(Turn.Pitch,F.Pitch-.1,1e-12)&&Turn.Eye==F.Eye,TEXT("the look turns as the walker's (less the look), and moves nothing"));
 }
 {
  const double Quarter=(Step(F,FVector2D(0,-1),0,No,FlyMps,.25).Eye-F.Eye).Size(),Half=(Step(F,FVector2D(0,-1),0,No,FlyMps,.5).Eye-F.Eye).Size(),Fast=(Step(F,FVector2D(0,-1),0,No,FastMps,.5).Eye-F.Eye).Size();
  Check(FMath::IsNearlyEqual(Half,2*Quarter,1e-9)&&FMath::IsNearlyEqual(Fast,5*Half,1e-9)&&FMath::IsNearlyEqual(Half,3.,1e-9),TEXT("the distance scales with the step's time and the speed (6 and 30 m/s)"));
  Check(FMath::IsNearlyEqual((Step(F,FVector2D(1,-1),0,No,FlyMps,.5).Eye-F.Eye).Size(),3.,1e-9),TEXT("forward and sideways at once is no faster"));
  Check(Step(F,FVector2D(0,-1),1,No,FlyMps,-1).Eye==F.Eye,TEXT("a step back in time moves nothing"));
 }
 return bAll;
}

bool On(const US47Main* M)
{
 const S47DirectorCamLocal::FState& S=S47DirectorCamLocal::St();
 return M&&S.bOn&&S.Owner==M;
}

bool Toggle(US47Main* M,FString& Why)
{
 Why.Reset();
 if(!M)return false;
 if(On(M))
 {
  Leave(M);
  if(M->UI)M->UI->Toast(TEXT("Director mode off: the view is yours again."),2.5f);
  return true;
 }
 if(S47DirectorCamLocal::St().bOn)Leave(nullptr); // (one left on by a game that has gone)
 if(!FS47ScriptPlayer::DevAllowed(M)){Why=TEXT("not a developer's run");return false;}
 Why=S47DirectorCamLocal::WhyNot(M);
 if(!Why.IsEmpty())
 {
  UE_LOG(LogS47DirectorCam,Display,TEXT("S47: director mode not entered: %s"),*Why);
  if(M->UI)M->UI->Toast(TEXT("Director mode: not now, ")+Why+TEXT("."),3.5f);
  return false;
 }
 S47DirectorCamLocal::Enter(M);
 return true;
}

void Leave(US47Main* M)
{
 S47DirectorCamLocal::FState& S=S47DirectorCamLocal::St();
 if(!S.bOn||(M&&S.Owner!=M))return;
 S.bOn=false;++S.Gen;
 S.Shown.Reset();S.TestInput.Reset();
 if(AS47Walker* W=S.Walker.Get())W->CameraOverride=S.Before;
 S.Walker.Reset();S.Before=nullptr;S.Owner=nullptr;
 FS47ScriptPlayer::Hold(S47DirectorCamLocal::HoldName,false);
 UE_LOG(LogS47DirectorCam,Display,TEXT("S47: director mode off"));
}

void Tick(US47Main* M,double /*Dt*/)
{
 S47DirectorCamLocal::FState& S=S47DirectorCamLocal::St();
 if(!S.bOn||!M)return;
 if(S.Owner!=M){Leave(nullptr);return;} // (left on by a game that has gone)
 const FString Off=S47DirectorCamLocal::WhyOff(M);
 if(!Off.IsEmpty())
 {
  Leave(M);
  UE_LOG(LogS47DirectorCam,Display,TEXT("S47: director mode ended by itself: %s"),*Off);
  if(M->UI)M->UI->Toast(TEXT("Director mode off: ")+Off+TEXT("."),3.f);
  return;
 }
 // 1 to 4, straight off the keyboard (nothing else answers them; the walker neither walks nor picks while its camera is held)
 const AS47Walker* W=S.Walker.Get();
 const APlayerController* PC=W&&W->IsActive()?Cast<APlayerController>(W->GetController()):nullptr;
 if(PC&&!S.TestInput.IsSet())
 {
  if(PC->WasInputKeyJustPressed(EKeys::Four)){S47DirectorCamLocal::PreviewDraft(M);return;}
  FString Said;
  bool bAsked=true,bDone=false;
  if(PC->WasInputKeyJustPressed(EKeys::One))bDone=S47DirectorCamLocal::PlaceShot(M,Said);
  else if(PC->WasInputKeyJustPressed(EKeys::Two))bDone=S47DirectorCamLocal::PlaceEye2(M,Said);
  else if(PC->WasInputKeyJustPressed(EKeys::Three))bDone=S47DirectorCamLocal::PlaceTrigger(M,Said);
  else bAsked=false;
  if(bAsked)
  {
   S.Last=Said;
   UE_LOG(LogS47DirectorCam,Display,TEXT("S47: director mode %s: %s"),bDone?TEXT("placed"):TEXT("placed nothing"),*Said);
  }
 }
 S47DirectorCamLocal::Show(M);
 S47DirectorCamLocal::Draw(M);
}

void AddDevLine(US47Main* M)
{
 if(!M||!M->DevEvents||!FS47ScriptPlayer::DevAllowed(M))return;
 if(M->DevEvents->Events().ContainsByPredicate([](const FS47DevEvents::FEvent& E){return E.Group==S47DirectorCamLocal::DevGroup;}))return;
 M->DevEvents->AddFreeform(S47DirectorCamLocal::DevGroup,TEXT("Fly the director's camera"),
  FString::Printf(TEXT("DIRECTOR-MODE-01: a free camera through the world; 1 a shot, 2 its move, 3 the trigger's point, 4 preview; %s comes back. The night waits."),*S47DirectorCamLocal::KeyOf(TEXT("director"))),
  [M]{FString Why;Toggle(M,Why);});
}

bool Contract(US47Main* M,double Dt)
{
 struct FRun{int32 Stage=0;double T=0,At=0;FVector Pos=FVector::ZeroVector;bool bDone=false;};
 static FRun R;
 if(R.bDone)return true;
 AS47Walker* W=M?M->Player.Get():nullptr;
 // (R is static: used here, not captured)
 const auto Finish=[](bool bGood,const FString& What)
 {
  R.bDone=true;
  Leave(nullptr);
  if(bGood){UE_LOG(LogS47DirectorCam,Display,TEXT("S47: director mode contract COMPLETE PASS"));}
  else{UE_LOG(LogS47DirectorCam,Display,TEXT("S47: director mode contract FAIL %s"),*What);}
  FPlatformMisc::RequestExitWithStatus(false,bGood?0:1);
  return true;
 };
 // held by director mode, as the script player says it (its holders come first in BigEvent's answer)
 const auto HeldByUs=[M]{FString Big;return FS47ScriptPlayer::BigEvent(M,Big)&&Big.StartsWith(TEXT("held by"))&&Big.Contains(S47DirectorCamLocal::HoldName,ESearchCase::CaseSensitive);};
 R.T+=FMath::Max(0.,Dt);
 FS47ScriptPlayer* P=FS47ScriptPlayer::Of(M);
 if(R.Stage==0)
 {
  // The night settled, the player on foot with the camera their own and nothing under way; at most a minute for it.
  if(!W||!P||R.T<6)
  {
   if(R.T>60)return Finish(false,TEXT("no walker or no script player (not in Freeform?)"));
   return false;
  }
  if(!FS47ScriptPlayer::DevAllowed(M))return Finish(false,TEXT("not a developer's run (add -S47Director)"));
  const FString Not=S47DirectorCamLocal::WhyNot(M);
  if(!Not.IsEmpty())
  {
   if(R.T>60)return Finish(false,TEXT("not entered within a minute: ")+Not);
   return false;
  }
  R.Pos=W->Pos;
  FString Why;
  if(!Toggle(M,Why)||!On(M))return Finish(false,TEXT("not entered: ")+Why);
  // forward along the view and up, through the test hook (not the keys)
  S47DirectorCamLocal::St().TestInput=FVector(0,1,-1);
  R.Stage=1;R.At=R.T;
  return false;
 }
 if(!W||!P)return Finish(false,TEXT("the walker or the script player went"));
 if(R.Stage==1)
 {
  if(!On(M))return Finish(false,TEXT("director mode went off while flying (see S47: director mode ended by itself)"));
  if(R.T-R.At<2)return false;
  S47DirectorCamLocal::St().TestInput=FVector::ZeroVector; // held still from here
  const FVector Cam=S47Three::ToThree(W->GetActorLocation()),Body(W->Pos.X,W->FloorY+W->Eye,W->Pos.Z);
  if(!W->bCameraHeld)return Finish(false,TEXT("(a) the camera is not held"));
  if(FVector::Dist(W->Pos,R.Pos)>1e-3)return Finish(false,FString::Printf(TEXT("(b) the player moved %.3f m"),FVector::Dist(W->Pos,R.Pos)));
  if(!(FVector::Dist(Cam,Body)>5))return Finish(false,FString::Printf(TEXT("(c) the camera is only %.2f m from the player"),FVector::Dist(Cam,Body)));
  if(!HeldByUs())return Finish(false,TEXT("(d) the script player is not held by director mode"));
  if(!M->OneAtATime())return Finish(false,TEXT("(d) OneAtATime does not answer true"));
  if(!M->Freeform||!M->Freeform->Held())return Finish(false,TEXT("(d) the session is not held (the clock runs on)"));
  const int32 Before=S47ScriptEditor::HaveDraft()?S47ScriptEditor::Draft().Actions.Num():0;
  FString Said;
  if(!S47DirectorCamLocal::PlaceShot(M,Said))return Finish(false,TEXT("(e) ")+Said);
  const FS47Script& D=S47ScriptEditor::Draft();
  if(D.Actions.Num()!=Before+1||D.Actions.Last().Do!=ES47ScriptAct::Shot||D.Actions.Last().Path.IsEmpty())return Finish(false,TEXT("(e) the draft has no new shot action"));
  const double Miss=FVector::Dist(FS47ScriptPlayer::WorldOfRoad(M,D.Actions.Last().Path[0]),S47DirectorCamLocal::St().Fly.Eye);
  if(Miss>.5)return Finish(false,FString::Printf(TEXT("(e) the shot's eye is %.2f m off the camera"),Miss));
  S47DirectorCamLocal::St().Last=Said;
  // a trigger box where the shot looks, so the picture shows what the director sees drawn (the test's own draft, never saved)
  {FS47ScriptTrigger& Tr=S47ScriptEditor::Draft().Trigger;Tr.Place.Reset();Tr.Point=D.Actions.Last().Point;Tr.Shape=ES47ScriptShape::Box;Tr.HalfX=9;Tr.HalfZ=6;Tr.Yaw=.4;Tr.Height=4;}
  UE_LOG(LogS47DirectorCam,Display,TEXT("S47: director mode contract: held, player still, camera %.1f m away, night held, %s (eye %.3f m off)"),FVector::Dist(Cam,Body),*Said,Miss);
  // back and up from the shot's eye, so the picture shows the eye, its line and the trigger's volume drawn in the world
  S47DirectorCamLocal::St().TestInput=FVector(.4,.5,1);
  R.Stage=4;R.At=R.T;
  return false;
 }
 if(R.Stage==4)
 {
  if(R.T-R.At<1.5)return false;
  S47DirectorCamLocal::St().TestInput=FVector::ZeroVector;
  // a picture of what the director sees, next to the contract's other proof
  if(FString Dir;FParse::Value(FCommandLine::Get(),TEXT("S47DevEventsContract="),Dir,false)&&!Dir.IsEmpty())FScreenshotRequest::RequestScreenshot(Dir/TEXT("director-mode-fly.png"),false,false);
  R.Stage=2;R.At=R.T;
  return false;
 }
 if(R.Stage==2)
 {
  // (the picture is taken at the end of a frame: a moment for it)
  if(R.T-R.At<.5)return false;
  FString Why;
  if(!Toggle(M,Why)||On(M))return Finish(false,TEXT("(f) director mode did not go off"));
  R.Stage=3;R.At=R.T;
  return false;
 }
 // Given back: the walker's own camera at the player (its flag is a frame behind), and no hold left.
 if(R.T-R.At<.5)return false;
 if(W->bCameraHeld)return Finish(false,TEXT("(f) the camera is still held"));
 const FVector Cam=S47Three::ToThree(W->GetActorLocation());
 const double Off=FVector2D::Distance(FVector2D(Cam.X,Cam.Z),FVector2D(W->Pos.X,W->Pos.Z));
 if(Off>.5)return Finish(false,FString::Printf(TEXT("(f) the view is %.2f m from the player"),Off));
 if(HeldByUs())return Finish(false,TEXT("(f) the script player is still held by director mode"));
 if(On(M)||S47DirectorCamLocal::St().bOn)return Finish(false,TEXT("(f) director mode is still on"));
 if(FVector::Dist(W->Pos,R.Pos)>1e-3)return Finish(false,TEXT("(b) the player moved after director mode"));
 return Finish(true,FString());
}
}
