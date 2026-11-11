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
  S47ScriptEditor::SelectShot(INDEX_NONE);
 }

 bool IsShotAt(const TArray<FS47ScriptAction>& Acts,int32 I){return Acts.IsValidIndex(I)&&Acts[I].Do==ES47ScriptAct::Shot;}
 int32 ShotCount(const TArray<FS47ScriptAction>& Acts){int32 N=0;for(const FS47ScriptAction& X:Acts)N+=X.Do==ES47ScriptAct::Shot?1:0;return N;}
 // "shot 2 of 3" for the selected one, by its place among the shots.
 FString ShotName(int32 I)
 {
  if(!S47ScriptEditor::HaveDraft())return FString(TEXT("no shot"));
  const TArray<FS47ScriptAction>& Acts=S47ScriptEditor::Draft().Actions;
  return IsShotAt(Acts,I)?FString::Printf(TEXT("shot %d of %d"),S47DirectorCam::ShotNumber(Acts,I),ShotCount(Acts)):FString(TEXT("no shot"));
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

 // What the crosshair is on (or a point 20 m ahead), as what a shot looks at. False with Said: nothing fit to look at.
 bool AimHere(US47Main* M,FS47ScriptPoint& At,FString& Said)
 {
  const FState& S=St();
  if(!FS47ScriptPlayer::RayRoad(M,S.Fly.Eye,S.Fly.Yaw,S.Fly.Pitch,At))
  {
   if(!FS47ScriptPlayer::RoadOfWorld(M,S.Fly.Eye+Forward(S.Fly)*20.,At)){Said=TEXT("nothing on the land ahead to look at");return false;}
   At.Up=FMath::Clamp(FMath::RoundToDouble(At.Up*10)/10,0.,300.);
  }
  if(!(FVector::Dist(FS47ScriptPlayer::WorldOfRoad(M,At),S.Fly.Eye)>=S47Shot::LeastReach)){Said=TEXT("the crosshair is on something right at the camera");return false;}
  return true;
 }

 // Key 1: a shot at the end of the draft's timeline, from the camera onto what the crosshair is on (or a point 20 m ahead). It is
 // the selected shot from then on.
 bool PlaceShot(US47Main* M,FString& Said)
 {
  FS47ScriptPoint Eye,At;
  FString Why;
  if(!EyeHere(M,Eye,Why)){Said=TEXT("no shot: ")+Why;return false;}
  if(!AimHere(M,At,Why)){Said=TEXT("no shot: ")+Why;return false;}
  EnsureDraft();
  FS47Script& D=S47ScriptEditor::Draft();
  FS47ScriptAction A;
  A.Do=ES47ScriptAct::Shot;
  A.At=D.Actions.Num()?FMath::RoundToDouble(D.Length()*10)/10:0.;
  A.Path.Add(Eye);
  A.Point=At;
  A.Fov=55;A.Secs=5;
  D.Actions.Add(A);
  S47ScriptEditor::SelectShot(D.Actions.Num()-1);
  S47ScriptEditor::Dirty()=true;
  Said=FString::Printf(TEXT("%s at %.1f s from (%.0f, %.0f) +%.1f m"),*ShotName(D.Actions.Num()-1),A.At,Eye.X,Eye.Z,Eye.Up);
  return true;
 }

 // Key 2: the selected shot's second eye here, so that it moves (no shot selected: the last one, which is selected then).
 bool PlaceEye2(US47Main* M,FString& Said)
 {
  const FState& S=St();
  const int32 I=S47ScriptEditor::HaveDraft()?S47DirectorCam::ShotOrLast(S47ScriptEditor::Draft().Actions,S47ScriptEditor::SelectedShot()):INDEX_NONE;
  if(I==INDEX_NONE){Said=TEXT("no shot to move: place one with 1 first");return false;}
  S47ScriptEditor::SelectShot(I);
  FS47ScriptAction& A=S47ScriptEditor::Draft().Actions[I];
  if(A.Path.IsEmpty()){Said=FString::Printf(TEXT("%s has no first eye"),*ShotName(I));return false;}
  FS47ScriptPoint Eye;
  FString Why;
  if(!EyeHere(M,Eye,Why)){Said=TEXT("no move: ")+Why;return false;}
  const double Apart=FVector::Dist(FS47ScriptPlayer::WorldOfRoad(M,A.Path[0]),S.Fly.Eye);
  if(!(Apart<=S47Shot::MostTravel)){Said=FString::Printf(TEXT("no move: %.0f m from the first eye, more than %.0f"),Apart,S47Shot::MostTravel);return false;}
  if(A.Point.bSet&&!(FVector::Dist(FS47ScriptPlayer::WorldOfRoad(M,A.Point),S.Fly.Eye)>=S47Shot::LeastReach)){Said=TEXT("no move: on the point the shot looks at");return false;}
  if(A.Path.Num()<2)A.Path.Add(Eye);else A.Path[1]=Eye;
  S47ScriptEditor::Dirty()=true;
  Said=FString::Printf(TEXT("%s moves %.0f m to (%.0f, %.0f) +%.1f m"),*ShotName(I),Apart,Eye.X,Eye.Z,Eye.Up);
  return true;
 }

 // Key 5: the selected shot shot again from here: its eye at the camera, looking at what the crosshair is on (its second eye, its
 // lens and its timing as they were).
 bool ReshootHere(US47Main* M,FString& Said)
 {
  const FState& S=St();
  const int32 I=S47ScriptEditor::SelectedShot();
  if(I==INDEX_NONE){Said=TEXT("no shot selected: [ or ] picks one, 1 places one");return false;}
  FS47ScriptPoint Eye,At;
  FString Why;
  if(!EyeHere(M,Eye,Why)){Said=TEXT("not moved: ")+Why;return false;}
  if(!AimHere(M,At,Why)){Said=TEXT("not moved: ")+Why;return false;}
  FS47ScriptAction& A=S47ScriptEditor::Draft().Actions[I];
  if(A.Path.Num()>1&&A.Path[1].bSet)
  {
   const double Apart=FVector::Dist(FS47ScriptPlayer::WorldOfRoad(M,A.Path[1]),S.Fly.Eye);
   if(!(Apart<=S47Shot::MostTravel)){Said=FString::Printf(TEXT("not moved: %.0f m from its second eye, more than %.0f"),Apart,S47Shot::MostTravel);return false;}
   if(!(FVector::Dist(FS47ScriptPlayer::WorldOfRoad(M,A.Path[1]),FS47ScriptPlayer::WorldOfRoad(M,At))>=S47Shot::LeastReach)){Said=TEXT("not moved: its second eye would be on the point it looks at");return false;}
  }
  if(A.Path.IsEmpty())A.Path.Add(Eye);else A.Path[0]=Eye;
  A.Point=At;
  S47ScriptEditor::Dirty()=true;
  Said=FString::Printf(TEXT("%s now from (%.0f, %.0f) +%.1f m onto the crosshair"),*ShotName(I),Eye.X,Eye.Z,Eye.Up);
  return true;
 }

 // Key 6: the selected shot deleted; the next shot is selected (or the one before).
 bool DeleteShot(FString& Said)
 {
  const int32 I=S47ScriptEditor::SelectedShot();
  if(I==INDEX_NONE){Said=TEXT("no shot selected to delete");return false;}
  const FString Was=ShotName(I);
  TArray<FS47ScriptAction>& Acts=S47ScriptEditor::Draft().Actions;
  Acts.RemoveAt(I);
  S47ScriptEditor::SelectShot(S47DirectorCam::AfterRemove(Acts,I,I));
  S47ScriptEditor::Dirty()=true;
  Said=Was+TEXT(" deleted; selected now: ")+ShotName(S47ScriptEditor::SelectedShot());
  return true;
 }

 // [ and ]: the shot before or after the selected one, round the end.
 bool PickShot(int32 Dir,FString& Said)
 {
  if(!S47ScriptEditor::HaveDraft()){Said=TEXT("no shots yet: 1 places one");return false;}
  const int32 I=S47DirectorCam::CycleShot(S47ScriptEditor::Draft().Actions,S47ScriptEditor::SelectedShot(),Dir);
  if(I==INDEX_NONE){Said=TEXT("no shots yet: 1 places one");return false;}
  S47ScriptEditor::SelectShot(I);
  const FS47ScriptAction& A=S47ScriptEditor::Draft().Actions[I];
  Said=FString::Printf(TEXT("selected %s, at %.1f s for %.1f s%s"),*ShotName(I),A.At,A.Secs,A.Path.Num()>1?TEXT(", moves"):TEXT(""));
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
  const int32 Sel=S47ScriptEditor::SelectedShot();
  const FString Picked=Sel==INDEX_NONE?FString(TEXT("no shot selected")):ShotName(Sel)+TEXT(" selected");
  return FString::Printf(TEXT("DIRECTOR MODE: %s, %s. 1 a new shot from here, [ ] pick a shot, 2 it moves to here, 5 its eye here looking at the crosshair, 6 delete it, 3 the trigger's point, 4 preview, %s back. %s fly, %s up, %s down, %s fast.%s%s"),
   *Name,*Picked,*KeyOf(TEXT("director")),*Walk,*KeyOf(TEXT("jump")),*KeyOf(TEXT("crouch")),*KeyOf(TEXT("run")),S.Last.IsEmpty()?TEXT(""):TEXT(" Last: "),*S.Last);
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
  // Slice 4: each shot numbered at its eye (its place among the shots), the selected one yellow and thicker, and a moving shot's
  // path as the camera will fly it (S47Shot::PoseAt, eased), not a straight line. Whatever is within 1.5 m of the camera is left
  // out: the camera stands in it, and its lines would cross the whole view.
  const FVector Cam=Wk->GetActorLocation();
  const auto Far=[&Cam](const FVector& P){return FVector::Dist(P,Cam)>=150.;};
  const int32 Sel=S47ScriptEditor::SelectedShot();
  int32 Number=0;
  for(int32 I=0;I<D.Actions.Num();++I)
  {
   const FS47ScriptAction& A=D.Actions[I];
   if(A.Do!=ES47ScriptAct::Shot)continue;
   ++Number;
   if(A.Path.IsEmpty()||!A.Path[0].bSet)continue;
   const bool bSel=I==Sel;
   const FColor Tint=bSel?FColor::Yellow:FColor::Cyan,Tint2=bSel?FColor::Yellow:FColor::White;
   const float Thick=bSel?6.f:3.f;
   const FVector Eye=At(A.Path[0]);
   const bool bEyeFar=Far(Eye);
   if(bEyeFar)
   {
    DrawDebugSphere(World,Eye,bSel?45.f:35.f,10,Tint,false,-1.f,0,Thick);
    DrawDebugString(World,Eye+FVector(0,0,70),FString::Printf(TEXT("%s%d"),bSel?TEXT("> "):TEXT(""),Number),nullptr,Tint,0.f,true,bSel?1.6f:1.2f);
    if(A.Point.bSet)DrawDebugLine(World,Eye,At(A.Point),Tint,false,-1.f,0,Thick);
   }
   if(A.Point.bSet)DrawDebugPoint(World,At(A.Point),bSel?20.f:14.f,Tint,false,-1.f);
   if(A.Path.Num()<2||!A.Path[1].bSet)continue;
   const FVector Eye2=At(A.Path[1]);
   if(Far(Eye2))
   {
    DrawDebugSphere(World,Eye2,bSel?45.f:35.f,10,Tint2,false,-1.f,0,Thick);
    DrawDebugString(World,Eye2+FVector(0,0,70),FString::Printf(TEXT("%d end"),Number),nullptr,Tint2,0.f,true,bSel?1.4f:1.f);
   }
   FS47ShotSpec Spec;
   Spec.EyeA=FS47ScriptPlayer::WorldOfRoad(M,A.Path[0]);Spec.EyeB=FS47ScriptPlayer::WorldOfRoad(M,A.Path[1]);
   Spec.Target=A.Point.bSet?FS47ScriptPlayer::WorldOfRoad(M,A.Point):Spec.EyeB;
   Spec.FovA=A.Fov;Spec.FovB=A.Fov2>0?A.Fov2:A.Fov;
   Spec.Secs=A.Secs>0?A.Secs:1.;Spec.In=A.In;Spec.Out=A.Out;Spec.bMove=true;
   constexpr int32 Samples=12;
   FVector Prev=S47Three::ToUE(S47Shot::PoseAt(Spec,0).Eye);
   for(int32 K=1;K<=Samples;++K)
   {
    const FVector Next=S47Three::ToUE(S47Shot::PoseAt(Spec,Spec.Secs*K/Samples).Eye);
    if(Far(Prev)&&Far(Next))
    {
     DrawDebugLine(World,Prev,Next,Tint2,false,-1.f,0,bSel?4.f:2.f);
     if(K<Samples)DrawDebugPoint(World,Next,bSel?10.f:7.f,Tint2,false,-1.f);
    }
    Prev=Next;
   }
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

int32 ShotOrNone(const TArray<FS47ScriptAction>& Acts,int32 Sel){return S47DirectorCamLocal::IsShotAt(Acts,Sel)?Sel:INDEX_NONE;}

int32 ShotOrLast(const TArray<FS47ScriptAction>& Acts,int32 Sel)
{
 if(S47DirectorCamLocal::IsShotAt(Acts,Sel))return Sel;
 for(int32 K=Acts.Num()-1;K>=0;--K)if(Acts[K].Do==ES47ScriptAct::Shot)return K;
 return INDEX_NONE;
}

int32 CycleShot(const TArray<FS47ScriptAction>& Acts,int32 Sel,int32 Dir)
{
 TArray<int32> Shots;
 for(int32 K=0;K<Acts.Num();++K)if(Acts[K].Do==ES47ScriptAct::Shot)Shots.Add(K);
 if(Shots.IsEmpty())return INDEX_NONE;
 const int32 Here=Shots.IndexOfByKey(Sel);
 if(Here==INDEX_NONE)return Dir<0?Shots.Last():Shots[0];
 const int32 N=Shots.Num(),By=Dir<0?-1:Dir>0?1:0;
 return Shots[((Here+By)%N+N)%N];
}

int32 AfterRemove(const TArray<FS47ScriptAction>& After,int32 Sel,int32 Removed)
{
 if(Sel<0)return INDEX_NONE;
 if(Removed<0||Removed>Sel)return ShotOrNone(After,Sel);
 if(Removed<Sel)return ShotOrNone(After,Sel-1);
 // the selected one itself: the next shot, else the one before
 for(int32 K=Removed;K<After.Num();++K)if(After[K].Do==ES47ScriptAct::Shot)return K;
 for(int32 K=FMath::Min(Removed,After.Num())-1;K>=0;--K)if(After[K].Do==ES47ScriptAct::Shot)return K;
 return INDEX_NONE;
}

int32 AfterSwap(int32 Sel,int32 A,int32 B){return Sel==A?B:Sel==B?A:Sel;}

int32 ShotNumber(const TArray<FS47ScriptAction>& Acts,int32 I)
{
 if(!S47DirectorCamLocal::IsShotAt(Acts,I))return 0;
 int32 N=0;
 for(int32 K=0;K<=I;++K)N+=Acts[K].Do==ES47ScriptAct::Shot?1:0;
 return N;
}

FS47Script FromTime(const FS47Script& S,double From)
{
 FS47Script O=S;
 if(!(From>0))return O;
 O.Actions.Reset();
 for(const FS47ScriptAction& X:S.Actions)if(X.At>=From-1e-6)
 {
  FS47ScriptAction Y=X;
  Y.At=FMath::Max(0.,FMath::RoundToDouble((X.At-From)*1000)/1000);
  O.Actions.Add(Y);
 }
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
 // DIRECTOR-MODE-01, slice 4: the selected shot, shared by the editor and the camera.
 const auto Check4=[&](bool bOk,const FString& What){bAll&=bOk;Lines.Add(FString::Printf(TEXT("%s DIRECTOR-4: %s"),bOk?TEXT("PASS"):TEXT("FAIL"),*What));};
 const auto Act=[](ES47ScriptAct Do,double When){FS47ScriptAction X;X.Do=Do;X.At=When;return X;};
 // shots at indices 0, 2 and 4 (0, 5 and 9 s); a toast at 1, a wait at 3
 const TArray<FS47ScriptAction> Acts={Act(ES47ScriptAct::Shot,0),Act(ES47ScriptAct::Toast,1),Act(ES47ScriptAct::Shot,5),Act(ES47ScriptAct::Wait,7),Act(ES47ScriptAct::Shot,9)};
 {
  Check4(CycleShot(Acts,0,1)==2&&CycleShot(Acts,2,1)==4&&CycleShot(Acts,4,1)==0,TEXT("] goes shot by shot over the other actions and wraps at the end"));
  Check4(CycleShot(Acts,0,-1)==4&&CycleShot(Acts,4,-1)==2,TEXT("[ goes back and wraps at the start"));
  Check4(CycleShot(Acts,INDEX_NONE,1)==0&&CycleShot(Acts,INDEX_NONE,-1)==4&&CycleShot(Acts,1,1)==0&&CycleShot(Acts,99,-1)==4,TEXT("from no shot: ] the first, [ the last"));
  const TArray<FS47ScriptAction> NoShots={Act(ES47ScriptAct::Toast,0)};
  Check4(CycleShot(NoShots,0,1)==INDEX_NONE&&CycleShot(TArray<FS47ScriptAction>(),INDEX_NONE,1)==INDEX_NONE,TEXT("without shots nothing is selected"));
 }
 {
  Check4(ShotOrNone(Acts,2)==2&&ShotOrNone(Acts,1)==INDEX_NONE&&ShotOrNone(Acts,5)==INDEX_NONE&&ShotOrNone(Acts,-3)==INDEX_NONE&&ShotOrNone(Acts,INDEX_NONE)==INDEX_NONE,TEXT("an index past the end, below 0 or on another action is no shot"));
  Check4(ShotOrLast(Acts,2)==2&&ShotOrLast(Acts,1)==4&&ShotOrLast(Acts,INDEX_NONE)==4&&ShotOrLast(TArray<FS47ScriptAction>(),0)==INDEX_NONE,TEXT("2 without a selection works on the last shot"));
  Check4(ShotNumber(Acts,0)==1&&ShotNumber(Acts,2)==2&&ShotNumber(Acts,4)==3&&ShotNumber(Acts,1)==0&&ShotNumber(Acts,7)==0,TEXT("shots are numbered among the shots"));
 }
 {
  const auto Without=[&Acts](int32 Gone){TArray<FS47ScriptAction> R=Acts;R.RemoveAt(Gone);return R;};
  Check4(AfterRemove(Without(1),4,1)==3&&AfterRemove(Without(1),2,1)==1,TEXT("deleting an earlier action shifts the selection with its shot"));
  Check4(AfterRemove(Without(3),0,3)==0&&AfterRemove(Without(3),2,3)==2,TEXT("deleting a later action leaves the selection"));
  Check4(AfterRemove(Without(2),2,2)==3,TEXT("deleting the selected shot selects the next shot"));
  Check4(AfterRemove(Without(4),4,4)==2,TEXT("deleting the selected last shot selects the one before"));
  TArray<FS47ScriptAction> One={Act(ES47ScriptAct::Toast,0),Act(ES47ScriptAct::Shot,1)};
  One.RemoveAt(1);
  Check4(AfterRemove(One,1,1)==INDEX_NONE&&AfterRemove(Without(1),INDEX_NONE,1)==INDEX_NONE,TEXT("deleting the only shot leaves none selected, and none stays none"));
  Check4(AfterSwap(2,2,1)==1&&AfterSwap(1,2,1)==2&&AfterSwap(4,2,1)==4,TEXT("UP and DOWN take the selection with the shot"));
 }
 {
  FS47Script Sc;Sc.Actions=Acts;
  const FS47Script From5=FromTime(Sc,5),From0=FromTime(Sc,0);
  Check4(From5.Actions.Num()==3&&From5.Actions[0].Do==ES47ScriptAct::Shot&&From5.Actions[0].At==0&&From5.Actions[1].At==2&&From5.Actions[2].At==4,TEXT("preview from a shot leaves out what comes before it and starts the shot at 0"));
  Check4(From0.Actions.Num()==Acts.Num()&&From0.Actions[4].At==9,TEXT("preview from 0 is the whole script as it is"));
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
 // 1 to 6, [ and ], straight off the keyboard (nothing else answers them; the walker neither walks nor picks while its camera is held)
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
  else if(PC->WasInputKeyJustPressed(EKeys::Five))bDone=S47DirectorCamLocal::ReshootHere(M,Said);
  else if(PC->WasInputKeyJustPressed(EKeys::Six))bDone=S47DirectorCamLocal::DeleteShot(Said);
  else if(PC->WasInputKeyJustPressed(EKeys::LeftBracket))bDone=S47DirectorCamLocal::PickShot(-1,Said);
  else if(PC->WasInputKeyJustPressed(EKeys::RightBracket))bDone=S47DirectorCamLocal::PickShot(1,Said);
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
  FString::Printf(TEXT("DIRECTOR-MODE-01: a free camera through the world; 1 a shot, [ ] pick a shot, 2 its move, 5 its eye here, 6 delete it, 3 the trigger's point, 4 preview; %s comes back. The night waits."),*S47DirectorCamLocal::KeyOf(TEXT("director"))),
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
  // Slice 4: a little to the right for a second, for the shot's second eye
  S47DirectorCamLocal::St().TestInput=FVector(1,0,0);
  R.Stage=5;R.At=R.T;
  return false;
 }
 if(R.Stage==5)
 {
  if(!On(M))return Finish(false,TEXT("director mode went off while flying to the second eye"));
  if(R.T-R.At<1)return false;
  S47DirectorCamLocal::St().TestInput=FVector::ZeroVector;
  FString Said;
  if(!S47DirectorCamLocal::PlaceEye2(M,Said))return Finish(false,TEXT("(g) ")+Said);
  const FS47Script& D=S47ScriptEditor::Draft();
  if(D.Actions.IsEmpty()||D.Actions.Last().Do!=ES47ScriptAct::Shot||D.Actions.Last().Path.Num()!=2)return Finish(false,TEXT("(g) the last shot has no second eye"));
  if(S47ScriptEditor::SelectedShot()!=D.Actions.Num()-1)return Finish(false,FString::Printf(TEXT("(g) the selected shot is action %d, not the last one (%d)"),S47ScriptEditor::SelectedShot()+1,D.Actions.Num()));
  S47DirectorCamLocal::St().Last=Said;
  UE_LOG(LogS47DirectorCam,Display,TEXT("S47: director mode contract: %s, selected action %d"),*Said,S47ScriptEditor::SelectedShot()+1);
  // back and up, so the picture shows the eyes, the path between them, the line to the look and the trigger's volume in the world
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
