#include "S47DevWaves.h"
#include "S47Abduction.h"
#include "S47Attention.h"
#include "S47Audio.h"
#include "S47Away.h"
#include "S47DevEvents.h"
#include "S47DevFreeform.h"
#include "S47DoorLoop.h"
#include "S47FarmSite.h"
#include "S47FieldCamera.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Gamepad.h"
#include "S47Handling.h"
#include "S47IndoorDread.h"
#include "S47Main.h"
#include "S47PaleBody.h"
#include "S47RoswellMotion.h"
#include "S47ScriptPlayer.h"
#include "S47Taken.h"
#include "S47Theme.h"
#include "S47Three.h"
#include "S47TripoCreature.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47WeekCase.h"
#include "S47World.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47DevWaves,Log,All);

// DEV TOY (Tom, 13 November 2026): the F10 wave shooter at SARO. See S47DevWaves.h. Nothing here is part of the game: it is started
// only from the F10 line, holds the night while it runs (FS47DevWaves::On, asked where the director's camera is asked) and gives
// the player back as they stood. It uses the aliens' own bodies and motion (FS47PaleBody, FS47RoswellMotion) and sounds the game has.

// One alien body of the shooter: built once on the web actor's root (as the F10 "Eye glint" bodies are), out or put away.
struct FS47DevWaveBody
{
 FS47PaleBody Body;
 FS47RoswellMotion Motion;
 S47DevWaves::EKind Kind=S47DevWaves::EKind::Pale;
 bool bOut=false;
 FVector At=FVector::ZeroVector; // the feet, scene metres
 // Fall: below 0 on its feet, else seconds since it was shot down.
 double Yaw=0,Clock=0,Travelled=0,Speed=0,TouchWait=0,Stagger=0,Fall=-1,Blocked=0,Steer=0,SteerLeft=0;
 int32 Hp=1;
};

// What the screen reads: written by the shooter each step, read by the widget (which never touches the shooter itself).
struct FS47DevWaveHud
{
 FString Top,Health,Ammo,Banner;
};

namespace S47DevWavesLocal
{
 const TCHAR* const HoldName=TEXT("devwaves");
 const TCHAR* const MenuGroup=TEXT("Wave shooter (dev toy)");
 // The SARO yard, as F10's "Go: SARO yard".
 constexpr double YardX=7,YardZ=19;
 constexpr double FallSeconds=.45,LieSeconds=.9;
 constexpr int32 MostBodies=14;

 FS47DevWaves*& Live(){static FS47DevWaves* P=nullptr;return P;}

 // A run nobody plays, or one that tries every F10 line by itself: the shooter is never started there.
 bool NobodyPlays()
 {
  const TCHAR* Cmd=FCommandLine::Get();
  FString Dir;
  return FApp::IsUnattended()||FParse::Param(Cmd,TEXT("unattended"))||FParse::Param(Cmd,TEXT("S47SmokeTest"))||FParse::Param(Cmd,TEXT("S47SelfTest"))
   ||FParse::Value(Cmd,TEXT("S47DevSweep="),Dir)||FParse::Value(Cmd,TEXT("S47DevEventsContract="),Dir)||FParse::Value(Cmd,TEXT("S47DevProof="),Dir)||FParse::Value(Cmd,TEXT("S47ShotStills="),Dir);
 }

 const TCHAR* KindName(S47DevWaves::EKind K)
 {
  switch(K)
  {
  case S47DevWaves::EKind::Small:return TEXT("small Roswell");
  case S47DevWaves::EKind::Tall:return TEXT("Tall One");
  case S47DevWaves::EKind::Crab:return TEXT("crab");
  default:return TEXT("Pale One");
  }
 }

 // The shooter's screen, in the look of the F10 panel: terminal type, amber on the night. Crosshair, the wave line, health, ammo and
 // the banner between waves. It takes no clicks.
 class SS47DevWavesHud:public SCompoundWidget
 {
 public:
  SLATE_BEGIN_ARGS(SS47DevWavesHud){}
   SLATE_ARGUMENT(TSharedPtr<FS47DevWaveHud>,Source)
  SLATE_END_ARGS()
  void Construct(const FArguments& InArgs)
  {
   S47_USING_THEME
   Source=InArgs._Source;
   SetVisibility(EVisibility::HitTestInvisible);
   const TSharedPtr<FS47DevWaveHud> Src=Source;
   const auto Line=[Src](FString FS47DevWaveHud::*Field){return [Src,Field]{return Src?FText::FromString((*Src).*Field):FText::GetEmpty();};};
   ChildSlot
   [
    SNew(SOverlay)
    +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
    [
     SNew(STextBlock).Font(Term(30)).ColorAndOpacity(Amber(.9f)).Text(FText::FromString(TEXT("+")))
    ]
    +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(Pad(0,16,0,0))
    [
     SNew(SBorder).BorderImage(Box(Night(.72f))).Padding(Pad(14,8,14,8))
     [
      SNew(STextBlock).Font(Term(22)).ColorAndOpacity(Amber(.95f)).Justification(ETextJustify::Center).Text_Lambda(Line(&FS47DevWaveHud::Top))
     ]
    ]
    +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(Pad(24,0,0,24))
    [
     SNew(SBorder).BorderImage(Box(Night(.72f))).Padding(Pad(14,8,14,8))
     [
      SNew(STextBlock).Font(Term(26)).ColorAndOpacity(Phos(.95f)).Text_Lambda(Line(&FS47DevWaveHud::Health))
     ]
    ]
    +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(Pad(0,0,24,24))
    [
     SNew(SBorder).BorderImage(Box(Night(.72f))).Padding(Pad(14,8,14,8))
     [
      SNew(STextBlock).Font(Term(26)).ColorAndOpacity(Amber(.95f)).Text_Lambda(Line(&FS47DevWaveHud::Ammo))
     ]
    ]
    +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(Pad(0,0,0,260))
    [
     SNew(SBorder).BorderImage(Box(Night(.8f))).Padding(Pad(26,14,26,14))
     .Visibility_Lambda([Src]{return Src&&!Src->Banner.IsEmpty()?EVisibility::HitTestInvisible:EVisibility::Collapsed;})
     [
      SNew(STextBlock).Font(Heading(40,2)).ColorAndOpacity(Paper(.97f)).Justification(ETextJustify::Center).Text_Lambda(Line(&FS47DevWaveHud::Banner))
     ]
    ]
   ];
  }
 private:
  TSharedPtr<FS47DevWaveHud> Source;
 };
}

FS47DevWaves::FS47DevWaves(US47Main* InM):M(InM){}

FS47DevWaves::~FS47DevWaves()
{
 // The game itself is going: nothing is given back, only let go.
 if(S47DevWavesLocal::Live()==this)
 {
  S47DevWavesLocal::Live()=nullptr;
  FS47ScriptPlayer::Hold(S47DevWavesLocal::HoldName,false);
 }
 if(Hud&&!IsEngineExitRequested()&&GEngine&&GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(Hud.ToSharedRef());
 Hud.Reset();
}

bool FS47DevWaves::On(const US47Main* InM)
{
 const FS47DevWaves* W=S47DevWavesLocal::Live();
 return InM&&W&&W->M==InM&&W->Running();
}

void FS47DevWaves::Say(const FString& Text,float Seconds) const
{
 if(M&&M->UI)M->UI->Toast(Text,Seconds);
 UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: %s"),*Text);
}

void FS47DevWaves::AddEntries(FS47DevEvents& E)
{
 E.AddFreeform(S47DevWavesLocal::MenuGroup,TEXT("Wave shooter: start"),
  TEXT("DEV TOY, not part of the game. Puts you in the SARO yard with a gun; aliens come in waves. Mouse fires, R reloads. The night waits, nothing is saved, and you are put back where you stood when it stops or you go down."),
  [this]{Start();});
 E.AddFreeform(S47DevWavesLocal::MenuGroup,TEXT("Wave shooter: stop"),
  TEXT("DEV TOY. Ends the shooter at once: aliens and gun gone, you back where you stood, the night running again."),
  [this]
  {
   if(Running())Stop(TEXT("stopped from F10"));
   else Say(TEXT("Wave shooter is not running."),3);
  });
}

// Why it may not begin now (empty: it may). As the director's camera asks: nothing of the night is cut off half way.
FString FS47DevWaves::WhyNot() const
{
 if(S47DevWavesLocal::NobodyPlays())return TEXT("a run nobody plays");
 if(!M||M->Mode!=US47Main::EMode::Play||M->bPausedByMenu)return TEXT("not in play");
 const FS47FreeformSession* F=M->Freeform.Get();
 if(!M->IsFreeform()||!F||!F->Active())return TEXT("Freeform only");
 const AS47Walker* W=M->Player.Get();
 if(!W||!W->IsActive()||!M->Web||!M->Web->IsLoaded()||!M->World)return TEXT("the player is not in your hands");
 if(M->World->bDriving)return TEXT("at the wheel");
 if(M->Web->Area()!=TEXT("saro"))return TEXT("only at SARO (F10, Time and place, Go: SARO yard)");
 if(M->ModalOpen())return TEXT("a panel is open");
 if(W->bCameraHeld)return TEXT("the camera is held by something else");
 if(M->FieldCamera&&M->FieldCamera->bRaised)return TEXT("the field camera is raised");
 if((F->Handling&&F->Handling->Holding())||(F->Field&&F->Field->ParcelStage()==2))return TEXT("your hands are full: put it down first");
 if((F->Taken&&F->Taken->Busy())||(M->Away&&M->Away->Abduction&&M->Away->Abduction->Busy()))return TEXT("being taken");
 if(F->Attention&&F->Attention->Visiting())return TEXT("a visit at SARO is under way");
 if(F->DoorLoop&&F->DoorLoop->Active())return TEXT("the door loop is under way");
 if(F->FarmSite&&F->FarmSite->VisitWantsGround())return TEXT("a visit is out for the player");
 if(F->IndoorDread&&F->IndoorDread->DoorActive())return TEXT("a door visit is under way");
 if(FS47WeekCase::Holds(M))return TEXT("the week's case is on the ground");
 if(const FS47ScriptPlayer* P=FS47ScriptPlayer::Of(M);P&&P->Running())return TEXT("a script is playing");
 FString Big;
 if(FS47ScriptPlayer::BigEvent(M,Big))return TEXT("a big event holds the night: ")+Big;
 if(M->DevEvents&&M->DevEvents->FreeformDev()&&M->DevEvents->FreeformDev()->Natural())return TEXT("Natural rhythm is on: turn it off first");
 if(M->OneAtATime())return TEXT("something else holds the night");
 if(F->Held())return TEXT("the night is held");
 return FString();
}

// Why it must end now (empty: it goes on).
FString FS47DevWaves::WhyOff() const
{
 if(!M||M->Mode!=US47Main::EMode::Play)return TEXT("out of play");
 if(!M->IsFreeform()||!M->Freeform||!M->Freeform->Active())return TEXT("the session stopped");
 const AS47Walker* W=Shooter.Get();
 if(!W||W!=M->Player.Get()||!M->Web||!M->Web->IsLoaded())return TEXT("the player went");
 if(M->Web->Area()!=StoodArea)return TEXT("left SARO");
 if(M->World&&M->World->bDriving)return TEXT("at the wheel");
 if((M->Freeform->Taken&&M->Freeform->Taken->Busy())||(M->Away&&M->Away->Abduction&&M->Away->Abduction->Busy()))return TEXT("taken");
 return FString();
}

S47DevWaves::FStand FS47DevWaves::ReadStand() const
{
 S47DevWaves::FStand S;
 if(const AS47Walker* W=M?M->Player.Get():nullptr)
 {
  S.Pos=W->Pos;S.FloorY=W->FloorY;S.Yaw=W->Yaw;S.Pitch=W->Pitch;S.bCrouch=W->bCrouch;
 }
 return S;
}

void FS47DevWaves::WriteStand(const S47DevWaves::FStand& S) const
{
 AS47Walker* W=Shooter.Get();
 if(!W)return;
 // (Place reads the floor from near the floor it has: the floor it stood on first)
 W->FloorY=S.FloorY;
 W->Place(S.Pos.X,S.Pos.Z,S.Yaw);
 W->Pitch=S.Pitch;
 if(S.bCrouch)W->SetCrouch(true);
}

void FS47DevWaves::Start()
{
 using namespace S47DevWaves;
 if(Running()){Say(TEXT("Wave shooter is already running. F10, Wave shooter: stop ends it."),3);return;}
 const FString Why=WhyNot();
 if(!Why.IsEmpty()){Say(TEXT("Wave shooter not started: ")+Why+TEXT("."));return;}
 // Whatever another F10 line left out goes first (its bodies, its walks).
 if(M->DevEvents&&M->DevEvents->FreeformDev())M->DevEvents->FreeformDev()->ClearRunning();
 AS47Walker* W=M->Player.Get();
 Shooter=W;
 Stood=ReadStand();StoodArea=M->Web->Area();StoodClock=M->Freeform->Clock();
 // Use is put away while it runs: nothing is picked up, opened or driven, so hands, pockets and doors are as they were after.
 StoodUse=W->OnUse;
 W->OnUse=[](const FString&){};
 W->Place(S47DevWavesLocal::YardX,S47DevWavesLocal::YardZ,UE_DOUBLE_PI);
 FS47ScriptPlayer::Hold(S47DevWavesLocal::HoldName,true);
 S47DevWavesLocal::Live()=this;
 Gun=FGun();
 Round.Begin();
 SpawnSerial=0;SpawnWait=0;SpawnDry=0;Kick=TracerLeft=MuzzleLeft=0;LastFlash=-10;Age=0;HurtSaid=-10;
 for(bool& bNo:bNoKind)bNo=false;
 BuildGun();
 ShowHud(true);
 WriteHud();
 UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter on, the player from %.1f %.1f (%s) at clock %.0f"),Stood.Pos.X,Stood.Pos.Z,*StoodArea,StoodClock);
 if(M->UI)M->UI->Toast(TEXT("DEV TOY: wave shooter. Mouse fires, R reloads. F10, Wave shooter: stop ends it. The night waits; nothing is saved."),6);
}

void FS47DevWaves::Stop(const TCHAR* Why)
{
 if(!Running())return;
 const int32 Reached=Round.Wave,Points=Round.Score,Down=Round.Kills;
 Round=S47DevWaves::FRound();
 if(S47DevWavesLocal::Live()==this)S47DevWavesLocal::Live()=nullptr;
 FS47ScriptPlayer::Hold(S47DevWavesLocal::HoldName,false);
 for(const TSharedPtr<FS47DevWaveBody>& B:Bodies)if(B&&B->Body.RootComp.IsValid()){Show(*B,false,0);B->bOut=false;}
 Bodies.RemoveAll([](const TSharedPtr<FS47DevWaveBody>& B){return !B||!B->Body.RootComp.IsValid();});
 DropGun();
 ShowHud(false);
 bool bBack=false;
 if(AS47Walker* W=Shooter.Get())
 {
  W->OnUse=StoodUse;
  // Only in the place it was taken from: a point of one area means nothing in another.
  if(M&&M->Web&&M->Web->IsLoaded()&&M->Web->Area()==StoodArea){WriteStand(Stood);bBack=true;}
 }
 StoodUse=nullptr;Shooter.Reset();
 const double ClockNow=M&&M->Freeform?M->Freeform->Clock():StoodClock;
 if(FMath::Abs(ClockNow-StoodClock)>.5){UE_LOG(LogS47DevWaves,Warning,TEXT("S47: dev wave shooter: the clock moved %.1f s while it ran (it should stand)"),ClockNow-StoodClock);}
 UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter off (%s): wave %d, %d down, score %d, the player %s"),Why?Why:TEXT("-"),Reached,Down,Points,bBack?TEXT("put back"):TEXT("left where they are"));
 if(M&&M->UI&&M->Mode==US47Main::EMode::Play)
  M->UI->Toast(FString::Printf(TEXT("Wave shooter off (%s). Wave reached: %d, score %d.%s"),Why?Why:TEXT("-"),Reached,Points,bBack?TEXT(" You are back where you stood."):TEXT("")),5);
}

// ---------- the aliens ----------

void FS47DevWaves::Show(FS47DevWaveBody& B,bool bShow,double Dt) const
{
 using namespace S47DevWaves;
 if(!B.Body.bBuilt||!B.Body.RootComp.IsValid())return;
 const AS47Walker* W=Shooter.Get();
 const double Face=W?FMath::Atan2(W->Pos.Z-B.At.Z,W->Pos.X-B.At.X):B.Yaw;
 const bool bMoving=B.Speed>.05;
 if(B.Kind==EKind::Crab){B.Body.ApplyCrab(bShow,B.At,B.Yaw,Face,B.Clock,1.,true);return;}
 if(B.Kind==EKind::Small)
 {
  // As the F10 "Eye glint" stands a small Roswell: its root placed, then its own motion.
  UBoxComponent* Root=B.Body.RootComp.Get();
  Root->SetVisibility(bShow,true);
  if(!bShow)return;
  Root->SetWorldLocationAndRotation(S47Three::ToUE(B.At),FRotator(0,FMath::RadiansToDegrees(B.Yaw),0));
  FS47RoswellPose P;
  P.Mode=B.Speed>1.6?ES47RoswellMotion::Run:bMoving?ES47RoswellMotion::Walk:ES47RoswellMotion::Idle;
  P.SpeedMps=B.Speed;P.DistanceTravelled=B.Travelled;
  B.Motion.Apply(B.Body.Creature,P,FMath::Min(Dt,.1));
  return;
 }
 // The Pale One and the Tall One, as F10's gait lines walk them: the chase when moving, listening when it stands.
 B.Body.Apply(bShow,B.At,B.Yaw,Face,bMoving?1.4:0.,B.Clock*3,B.Clock,bMoving?FS47PaleBody::EPose::Chase:FS47PaleBody::EPose::Listen);
}

FS47DevWaveBody* FS47DevWaves::Take(S47DevWaves::EKind Want)
{
 using namespace S47DevWaves;
 if(!M||!M->Web||!M->Web->IsLoaded())return nullptr;
 // A body whose parts went with their world is forgotten.
 Bodies.RemoveAll([](const TSharedPtr<FS47DevWaveBody>& B){return !B||(!B->bOut&&!B->Body.RootComp.IsValid());});
 if(bNoKind[int32(Want)])Want=EKind::Pale;
 for(const TSharedPtr<FS47DevWaveBody>& B:Bodies)if(!B->bOut&&B->Kind==Want)return B.Get();
 if(Bodies.Num()<S47DevWavesLocal::MostBodies)
 {
  TSharedPtr<FS47DevWaveBody> N=MakeShared<FS47DevWaveBody>();
  N->Kind=Want;
  N->Body.CreatureAsset=Want==EKind::Small?S47KindBody::Roswell:Want==EKind::Tall?S47KindBody::Tall:Want==EKind::Crab?S47KindBody::Crab:S47KindBody::Pale;
  if(Want==EKind::Tall)N->Body.Style=ES47PaleStyle::Tall;
  if(N->Body.Build(M->Web.Get(),TEXT("#root"),*FString::Printf(TEXT("DevWaves%d_"),++BodySerial)))
  {
   if(Want!=EKind::Pale&&!N->Body.Creature.Ready())
   {
    // No skinned model for this kind in this build: it comes as the pale stand-in, and so do the rest of its kind.
    bNoKind[int32(Want)]=true;N->Kind=EKind::Pale;
    UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: no skinned body for the %s here; the pale body stands in"),S47DevWavesLocal::KindName(Want));
   }
   Bodies.Add(N);
   return N.Get();
  }
  UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: a %s body could not be built"),S47DevWavesLocal::KindName(Want));
  if(Want!=EKind::Pale)bNoKind[int32(Want)]=true;
 }
 // None of that kind free: any body put away will do.
 for(const TSharedPtr<FS47DevWaveBody>& B:Bodies)if(!B->bOut)return B.Get();
 return nullptr;
}

// One more out of sight: on the ground round the player, behind something or behind their back when there is such a place.
bool FS47DevWaves::Spawn()
{
 using namespace S47DevWaves;
 AS47Walker* W=Shooter.Get();
 UWorld* World=M&&M->Web?M->Web->GetWorld():nullptr;
 if(!W||!World)return false;
 const FVector Feet(W->Pos.X,W->FloorY,W->Pos.Z);
 const FVector Eye=S47Three::ToThree(W->EyeUE());
 const FVector2D Ahead(-FMath::Sin(W->Yaw),-FMath::Cos(W->Yaw));
 TOptional<FVector> Best;
 double BestScore=-1;
 const double Turn=FMath::FRand()*2*PI;
 for(int32 I=0;I<12;++I)for(const double Metres:{34.,26.,42.})
 {
  const double Ang=Turn+I*(2*PI/12);
  const FVector2D C(Feet.X+FMath::Cos(Ang)*Metres,Feet.Z+FMath::Sin(Ang)*Metres);
  FCollisionQueryParams Under(SCENE_QUERY_STAT(S47DevWavesSpawnGround),true,W);
  FHitResult Ground;
  if(!World->LineTraceSingleByChannel(Ground,S47Three::ToUE(C.X,Feet.Y+10.,C.Y),S47Three::ToUE(C.X,Feet.Y-10.,C.Y),ECC_Visibility,Under))continue;
  const double G=S47Three::ToThree(Ground.ImpactPoint).Y;
  if(G-Feet.Y>2.5)continue; // a roof, not the ground
  FCollisionQueryParams Sight(SCENE_QUERY_STAT(S47DevWavesSpawnSight),true,W);
  FHitResult Block;
  const bool bHidden=World->LineTraceSingleByChannel(Block,S47Three::ToUE(Eye),S47Three::ToUE(C.X,G+1.,C.Y),ECC_Visibility,Sight);
  const bool bBehind=FVector2D::DotProduct(Ahead,(C-FVector2D(Feet.X,Feet.Z)).GetSafeNormal())<.2;
  const double Score=(bHidden?4.:0.)+(bBehind?2.:0.)+(W->Walkable(C.X,C.Y)?1.:0.)+FMath::FRand()*.9;
  if(Score>BestScore){BestScore=Score;Best=FVector(C.X,G,C.Y);}
 }
 if(!Best)return false;
 const EKind Want=KindOf(Round.Wave,SpawnSerial);
 FS47DevWaveBody* B=Take(Want);
 if(!B)return false;
 ++SpawnSerial;
 B->bOut=true;B->At=*Best;B->Yaw=FMath::Atan2(Feet.Z-B->At.Z,Feet.X-B->At.X);
 B->Clock=FMath::FRand()*3;B->Travelled=0;B->Speed=0;B->TouchWait=0;B->Stagger=0;B->Fall=-1;B->Blocked=0;B->Steer=0;B->SteerLeft=0;
 B->Hp=HealthOf(B->Kind);
 B->Motion.Reset();
 Show(*B,true,0);
 return true;
}

bool FS47DevWaves::PathClear(const FS47DevWaveBody& B,double Heading) const
{
 using namespace S47DevWaves;
 const AS47Walker* W=Shooter.Get();
 UWorld* World=M&&M->Web?M->Web->GetWorld():nullptr;
 if(!World)return true;
 FCollisionQueryParams Probe(SCENE_QUERY_STAT(S47DevWavesProbe),true,W);
 for(const auto& Part:B.Body.All)if(const auto* Own=Part.Get())Probe.AddIgnoredComponent(Own);
 const FVector From=B.At+FVector(0,FMath::Min(.9,HeightOf(B.Kind)*.42),0);
 const FVector To=From+FVector(FMath::Cos(Heading),0,FMath::Sin(Heading))*(RadiusOf(B.Kind)+.6);
 FHitResult Hit;
 return !World->LineTraceSingleByChannel(Hit,S47Three::ToUE(From),S47Three::ToUE(To),ECC_Visibility,Probe);
}

// Simple chase on the ground: towards the player, round what stands in the way, and through it after a while (a dev toy: none is
// left stuck behind a wall). At touch range it hurts.
void FS47DevWaves::StepBody(FS47DevWaveBody& B,double Dt)
{
 using namespace S47DevWaves;
 AS47Walker* W=Shooter.Get();
 UWorld* World=M&&M->Web?M->Web->GetWorld():nullptr;
 UBoxComponent* Root=B.Body.RootComp.Get();
 if(!Root||!W||!World)
 {
  if(B.Fall<0)Round.Lost();
  B.bOut=false;
  return;
 }
 B.Clock+=Dt;
 if(B.Fall>=0)
 {
  // Shot down: it tips over backwards from its feet, lies a moment and is put away.
  B.Fall+=Dt;
  if(B.Fall>=S47DevWavesLocal::FallSeconds+S47DevWavesLocal::LieSeconds){Show(B,false,0);B.bOut=false;return;}
  const double K=FMath::Min(1.,B.Fall/S47DevWavesLocal::FallSeconds);
  Root->SetWorldRotation(FRotator(82.*K*K,FMath::RadiansToDegrees(B.Yaw),0));
  return;
 }
 const FVector Feet(W->Pos.X,W->FloorY,W->Pos.Z);
 const FVector2D To(Feet.X-B.At.X,Feet.Z-B.At.Z);
 const double Dist=To.Size(),Face=FMath::Atan2(To.Y,To.X);
 B.TouchWait=FMath::Max(0.,B.TouchWait-Dt);
 B.Stagger=FMath::Max(0.,B.Stagger-Dt);
 const bool bFight=Round.Phase==EPhase::Fight;
 double Pace=0,Head=Face;
 if(bFight&&B.Stagger<=0&&Dist>TouchReach*.8)
 {
  B.SteerLeft-=Dt;
  if(B.SteerLeft<=0)B.Steer=0;
  bool bClear=false;
  for(const double Try:{B.Steer,0.,.75,-.75,1.5,-1.5})
  {
   if(!PathClear(B,Face+Try))continue;
   if(Try!=0&&Try!=B.Steer){B.Steer=Try;B.SteerLeft=.6;}
   Head=Face+Try;bClear=true;
   break;
  }
  if(bClear)B.Blocked=0;else B.Blocked+=Dt;
  if(bClear||B.Blocked>1.2)Pace=WaveSpeed(Round.Wave)*PaceOf(B.Kind);
 }
 B.Yaw+=FMath::UnwindRadians(Head-B.Yaw)*FMath::Min(1.,Dt*7.);
 const double Moved=FMath::Min(Pace*Dt,FMath::Max(0.,Dist-TouchReach*.7));
 B.At.X+=FMath::Cos(Head)*Moved;B.At.Z+=FMath::Sin(Head)*Moved;
 B.Travelled+=Moved;B.Speed=Dt>0?Moved/Dt:0.;
 // The ground under it, not its own stand-in parts.
 {
  FCollisionQueryParams Under(SCENE_QUERY_STAT(S47DevWavesFloor),true,W);
  for(const auto& Part:B.Body.All)if(const auto* Own=Part.Get())Under.AddIgnoredComponent(Own);
  FHitResult Floor;
  if(World->LineTraceSingleByChannel(Floor,S47Three::ToUE(B.At.X,B.At.Y+1.,B.At.Z),S47Three::ToUE(B.At.X,B.At.Y-3.,B.At.Z),ECC_Visibility,Under))
   B.At.Y=FMath::FInterpTo(B.At.Y,S47Three::ToThree(Floor.ImpactPoint).Y,Dt,12.);
 }
 if(bFight&&B.Stagger<=0&&B.TouchWait<=0&&Dist<=TouchReach&&FMath::Abs(B.At.Y-Feet.Y)<2.)
 {
  B.TouchWait=TouchGap;
  Round.Touched();
  if(M->Audio)
  {
   M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.9f,.Rate=.8f});
   if(Age-HurtSaid>1.6){HurtSaid=Age;M->Audio->Gasp();}
  }
  if(Round.Phase==EPhase::Dead)
  {
   UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: the player went down in wave %d, score %d"),Round.Wave,Round.Score);
   if(M->Audio)M->Audio->Sfx(TEXT("alienRadio"),FS47Sound{.Gain=.5f,.Rate=.7f,.Dur=2.5f});
  }
 }
 Show(B,true,Dt);
}

// ---------- the gun ----------

void FS47DevWaves::BuildGun()
{
 DropGun();
 AS47Walker* W=Shooter.Get();
 AS47Web* Web=M?M->Web.Get():nullptr;
 UStaticMesh* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 UMaterialInterface* Lit=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 if(!W||!Web||!Cube||!W->GetRootComponent())return;
 // The walker's root is the eye and turns with the look: the gun hangs under it, low and to the right.
 USceneComponent* Holder=NewObject<USceneComponent>(W);
 Holder->SetupAttachment(W->GetRootComponent());Holder->SetMobility(EComponentMobility::Movable);Holder->RegisterComponent();
 GunRoot=Holder;GunParts.Add(Holder);
 const auto Tint=[Lit,W](const FLinearColor& Shade,float Rough)
 {
  UMaterialInstanceDynamic* Mat=Lit?UMaterialInstanceDynamic::Create(Lit,W):nullptr;
  if(Mat){Mat->SetVectorParameterValue(TEXT("Color"),Shade);Mat->SetScalarParameterValue(TEXT("Roughness"),Rough);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);}
  return Mat;
 };
 UMaterialInstanceDynamic* Steel=Tint(FLinearColor(.035f,.038f,.045f),.42f);
 UMaterialInstanceDynamic* Grip=Tint(FLinearColor(.07f,.045f,.03f),.8f);
 // A box is one metre: the scale is the size in metres. Places in centimetres from the eye (x ahead, y right, z up).
 const auto Part=[this,Holder,W,Cube](const FVector& AtCm,const FVector& SizeM,UMaterialInstanceDynamic* Mat)
 {
  UStaticMeshComponent* C=NewObject<UStaticMeshComponent>(W);
  C->SetupAttachment(Holder);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Cube);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);
  if(Mat)C->SetMaterial(0,Mat);
  C->SetRelativeLocation(AtCm);C->SetRelativeScale3D(SizeM);
  C->RegisterComponent();
  GunParts.Add(C);
  return C;
 };
 Part(FVector(40,17,-15),FVector(.30,.05,.085),Steel);        // the receiver
 Barrel=Part(FVector(64,17,-12.5),FVector(.26,.026,.026),Steel); // the barrel
 Part(FVector(31,17,-23),FVector(.045,.04,.12),Grip);          // the grip
 Part(FVector(42,17,-22),FVector(.035,.03,.10),Steel);         // the magazine
 Part(FVector(27,17,-13),FVector(.12,.035,.06),Grip);          // the stock's stub
 // A steady lamp under the barrel, so that what comes can be seen at night. Steady: it never flashes.
 {
  USpotLightComponent* L=NewObject<USpotLightComponent>(W);
  L->SetupAttachment(Holder);L->SetMobility(EComponentMobility::Movable);
  L->SetIntensityUnits(ELightUnits::Candelas);L->SetIntensity(420.f);
  L->SetLightColor(S47Three::Hex(0xffe6c0));
  L->SetAttenuationRadius(45.f*100.f);L->SetOuterConeAngle(21.f);L->SetInnerConeAngle(9.f);
  L->SetCastShadows(true);
  L->SetRelativeLocation(FVector(70,17,-15));
  L->RegisterComponent();
  GunParts.Add(L);
 }
 // The muzzle's light: small and local (3.5 m), on for a twentieth of a second, never more than three a second, never with Calm flashes.
 {
  UPointLightComponent* L=NewObject<UPointLightComponent>(W);
  L->SetupAttachment(Holder);L->SetMobility(EComponentMobility::Movable);
  L->SetIntensityUnits(ELightUnits::Candelas);L->SetIntensity(28.f);
  L->SetLightColor(S47Three::Hex(0xffc070));
  L->SetAttenuationRadius(350.f);L->SetCastShadows(false);
  L->SetRelativeLocation(FVector(82,17,-12.5));
  L->SetVisibility(false);
  L->RegisterComponent();
  Muzzle=L;GunParts.Add(L);
 }
 // The tracer: one thin box in the world, laid along the shot for a moment.
 if(Web->GetRootComponent())
 {
  UStaticMeshComponent* C=NewObject<UStaticMeshComponent>(Web);
  C->SetupAttachment(Web->GetRootComponent());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Cube);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);
  if(UMaterialInstanceDynamic* Mat=Lit?UMaterialInstanceDynamic::Create(Lit,Web):nullptr)
  {
   Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor::Black);
   Mat->SetVectorParameterValue(TEXT("Emissive"),S47Three::Hex(0xffb052));
   Mat->SetScalarParameterValue(TEXT("EmissivePower"),3.f);
   C->SetMaterial(0,Mat);TracerMat=Mat;
  }
  C->SetVisibility(false);
  C->RegisterComponent();
  Tracer=C;GunParts.Add(C);
 }
}

void FS47DevWaves::DropGun()
{
 // (the children first: the holder is the first one in)
 for(int32 I=GunParts.Num()-1;I>=0;--I)if(USceneComponent* C=GunParts[I].Get())C->DestroyComponent();
 GunParts.Reset();
 GunRoot.Reset();Barrel.Reset();Tracer.Reset();Muzzle.Reset();TracerMat.Reset();
}

void FS47DevWaves::PoseGun(double Dt)
{
 Kick=FMath::Max(0.,Kick-Dt*7.);
 TracerLeft-=Dt;MuzzleLeft-=Dt;
 if(UStaticMeshComponent* T=Tracer.Get())if(TracerLeft<=0&&T->IsVisible())T->SetVisibility(false);
 if(UPointLightComponent* L=Muzzle.Get())if(MuzzleLeft<=0&&L->IsVisible())L->SetVisibility(false);
 if(USceneComponent* Holder=GunRoot.Get())
 {
  // Back with the shot, down and tilted while the magazine is changed, down when the player has gone down.
  const double Low=Round.Phase==S47DevWaves::EPhase::Dead?1.:Gun.Reload>0?FMath::Sin(PI*FMath::Clamp(1.-Gun.Reload/S47DevWaves::ReloadSeconds,0.,1.)):0.;
  Holder->SetRelativeLocationAndRotation(FVector(-5.*Kick,0,-13.*Low),FRotator(3.5*Kick-24.*Low,0,0));
 }
}

void FS47DevWaves::Reload()
{
 if(!Gun.StartReload())return;
 if(M&&M->Audio)M->Audio->Sfx(TEXT("rack_switch"),FS47Sound{.Gain=.7f,.Rate=.8f});
}

// A hitscan shot along the view: the nearest alien on the line, unless something of the world stands before it.
void FS47DevWaves::Fire(bool bPulled)
{
 using namespace S47DevWaves;
 AS47Walker* W=Shooter.Get();
 UWorld* World=M&&M->Web?M->Web->GetWorld():nullptr;
 if(!W||!World)return;
 if(!Gun.Fire())
 {
  // Empty: the trigger clicks, and the magazine is changed.
  if(bPulled&&Gun.Mag<=0&&Gun.Reload<=0){if(M->Audio)M->Audio->Sfx(TEXT("click"),FS47Sound{.Gain=.8f});Reload();}
  return;
 }
 const FVector Eye=S47Three::ToThree(W->EyeUE());
 const FVector AimUE=W->ForwardUE();
 const FVector Dir=FVector(AimUE.X,AimUE.Z,AimUE.Y).GetSafeNormal();
 FCollisionQueryParams Line(SCENE_QUERY_STAT(S47DevWavesShot),true,W);
 for(const TSharedPtr<FS47DevWaveBody>& B:Bodies)for(const auto& Part:B->Body.All)if(const auto* Own=Part.Get())Line.AddIgnoredComponent(Own);
 FHitResult Wall;
 double Reach=Range;
 if(World->LineTraceSingleByChannel(Wall,S47Three::ToUE(Eye),S47Three::ToUE(Eye+Dir*Range),ECC_Visibility,Line))Reach=Wall.Distance/100.;
 FS47DevWaveBody* Struck=nullptr;
 double StruckAt=Reach+.3;
 for(const TSharedPtr<FS47DevWaveBody>& B:Bodies)
 {
  if(!B->bOut||B->Fall>=0)continue;
  const double Along=RayBody(Eye,Dir,B->At,HeightOf(B->Kind),RadiusOf(B->Kind));
  if(Along>=0&&Along<StruckAt){StruckAt=Along;Struck=B.Get();}
 }
 const FVector End=Eye+Dir*(Struck?StruckAt:Reach);
 Kick=1;
 const bool bCalm=M->Flow&&M->Flow->bCalmFlash;
 if(M->Audio)
 {
  M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.95f,.Rate=2.3f});
  M->Audio->Sfx(TEXT("metalTap"),FS47Sound{.Gain=.5f,.Rate=.6f});
 }
 // The tracer: a thin dim line from the muzzle to where the shot ended. A line, not a light.
 if(UStaticMeshComponent* T=Tracer.Get())
 {
  const UStaticMeshComponent* Tube=Barrel.Get();
  const FVector MuzzleUE=Tube?Tube->GetComponentLocation()+Tube->GetForwardVector()*16.:W->EyeUE();
  const FVector EndUE=S47Three::ToUE(End);
  const double LenCm=FVector::Dist(MuzzleUE,EndUE);
  if(LenCm>60.)
  {
   T->SetWorldLocationAndRotation((MuzzleUE+EndUE)*.5,FRotationMatrix::MakeFromX(EndUE-MuzzleUE).Rotator());
   T->SetWorldScale3D(FVector(LenCm/100.,.012,.012));
   if(UMaterialInstanceDynamic* Mat=TracerMat.Get())Mat->SetScalarParameterValue(TEXT("EmissivePower"),bCalm?1.f:3.f);
   T->SetVisibility(true);
   TracerLeft=.05;
  }
 }
 if(FlashMay(Age,LastFlash,bCalm))if(UPointLightComponent* L=Muzzle.Get())
 {
  LastFlash=Age;MuzzleLeft=.05;
  L->SetVisibility(true);
 }
 if(!Struck)return;
 --Struck->Hp;
 const FVector AtHead=Struck->At+FVector(0,HeightOf(Struck->Kind)*.7,0);
 if(Struck->Hp<=0)
 {
  Struck->Fall=0;Struck->Speed=0;
  Round.Killed(Struck->Kind);
  if(M->Audio)
  {
   M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.8f,.Rate=.6f,.At=Struck->At});
   M->Audio->Sfx(TEXT("alienRadio"),FS47Sound{.Gain=.4f,.Rate=1.5f,.At=AtHead,.Dur=.4f});
  }
  // (the wave was cleared by this one: the next is announced)
  if(Round.Phase==EPhase::Banner)
  {
   SpawnSerial=0;SpawnWait=0;
   if(M->Audio)M->Audio->Sfx(TEXT("switch"),FS47Sound{.Gain=.7f});
   UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: wave %d cleared, score %d, health %d"),Round.Wave-1,Round.Score,Round.Health);
  }
 }
 else
 {
  Struck->Stagger=.35;
  if(M->Audio)M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.6f,.Rate=1.4f,.At=AtHead});
 }
}

// ---------- the screen ----------

void FS47DevWaves::ShowHud(bool bShow)
{
 if(!GEngine||!GEngine->GameViewport)return;
 if(bShow&&!Hud)
 {
  if(!HudText)HudText=MakeShared<FS47DevWaveHud>();
  Hud=SNew(S47DevWavesLocal::SS47DevWavesHud).Source(HudText);
  GEngine->GameViewport->AddViewportWidgetContent(Hud.ToSharedRef(),27);
 }
 else if(!bShow&&Hud)
 {
  GEngine->GameViewport->RemoveViewportWidgetContent(Hud.ToSharedRef());
  Hud.Reset();
 }
}

void FS47DevWaves::WriteHud()
{
 using namespace S47DevWaves;
 if(!HudText)return;
 HudText->Top=FString::Printf(TEXT("DEV TOY / WAVE SHOOTER      WAVE %d      ALIENS LEFT %d      SCORE %d"),Round.Wave,Round.Left(),Round.Score);
 HudText->Health=FString::Printf(TEXT("HEALTH %d"),Round.Health);
 HudText->Ammo=Gun.Reload>0?FString(TEXT("RELOADING")):Gun.Mag<=0?FString(TEXT("EMPTY  -  R")):FString::Printf(TEXT("AMMO %d / %d"),Gun.Mag,MagSize);
 if(Round.Phase==EPhase::Banner)HudText->Banner=Round.Wave>1?FString::Printf(TEXT("WAVE %d CLEARED\nWAVE %d"),Round.Wave-1,Round.Wave):FString(TEXT("WAVE 1"));
 else if(Round.Phase==EPhase::Dead)HudText->Banner=FString::Printf(TEXT("YOU WENT DOWN\nWAVE REACHED: %d      SCORE: %d"),Round.Wave,Round.Score);
 else HudText->Banner.Reset();
}

// ---------- each frame ----------

void FS47DevWaves::Step(double Dt)
{
 using namespace S47DevWaves;
 if(!Running())return;
 const FString Off=WhyOff();
 if(!Off.IsEmpty()){Stop(*Off);return;}
 // A menu or a panel holds it, as it holds the night.
 if(M->bPausedByMenu||M->ModalOpen()||!FMath::IsFinite(Dt)||Dt<=0)return;
 const double D=FMath::Min(Dt,.1);
 Age+=D;
 if(Round.Step(D)){Stop(TEXT("you went down"));return;}
 AS47Walker* W=Shooter.Get();
 // The trigger and the reload, read straight off the mouse, the keyboard and the pad (no rows in the key table: they mean something only here).
 if(Round.Phase!=EPhase::Dead&&W->IsActive())if(const APlayerController* PC=Cast<APlayerController>(W->GetController()))
 {
  if(PC->WasInputKeyJustPressed(EKeys::R)||S47Gamepad::Pressed(EKeys::Gamepad_FaceButton_Left))Reload();
  const bool bPulled=PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)||S47Gamepad::Pressed(EKeys::Gamepad_RightTrigger);
  if(bPulled||PC->IsInputKeyDown(EKeys::LeftMouseButton)||S47Gamepad::Down(EKeys::Gamepad_RightTrigger))Fire(bPulled);
 }
 if(Gun.Step(D)&&M->Audio)M->Audio->Sfx(TEXT("switch"),FS47Sound{.Gain=.7f,.Rate=1.2f});
 // The wave comes out one at a time, never more at once than the wave allows.
 SpawnWait-=D;
 if(Round.MaySpawn()&&SpawnWait<=0)
 {
  if(Spawn()){Round.Spawned();SpawnWait=WaveGap(Round.Wave);SpawnDry=0;}
  else
  {
   SpawnWait=.5;SpawnDry+=.5;
   if(SpawnDry>8&&Round.Out==0){Stop(TEXT("no alien body or free ground here"));return;}
  }
 }
 for(const TSharedPtr<FS47DevWaveBody>& B:Bodies)if(B->bOut)StepBody(*B,D);
 PoseGun(D);
 WriteHud();
}

// ---------- the rules, held by -S47SelfTest ----------

bool FS47DevWaves::SelfTest(TArray<FString>& Lines)
{
 using namespace S47DevWaves;
 bool bOk=true;
 const auto Check=[&](bool bGood,const FString& What){Lines.Add(FString::Printf(TEXT("%s devWaves: %s"),bGood?TEXT("ok"):TEXT("FAIL"),*What));bOk&=bGood;};
 {
  bool bGrows=true,bCapped=true;
  for(int32 N=1;N<60;++N)
  {
   bGrows&=WaveSize(N+1)>=WaveSize(N)&&WaveSpeed(N+1)>=WaveSpeed(N)&&WaveAtOnce(N+1)>=WaveAtOnce(N)&&WaveGap(N+1)<=WaveGap(N);
   bCapped&=WaveSize(N)<=MostInWave&&WaveAtOnce(N)<=MostAtOnce&&WaveSpeed(N)<=4.2&&WaveGap(N)>=.45;
  }
  Check(bGrows&&WaveSize(2)>WaveSize(1)&&WaveSpeed(2)>WaveSpeed(1),FString::Printf(TEXT("each wave is at least as big and as fast as the one before (wave 1: %d at %.2f m/s, wave 5: %d at %.2f m/s)"),WaveSize(1),WaveSpeed(1),WaveSize(5),WaveSpeed(5)));
  Check(bCapped&&WaveSize(0)==WaveSize(1)&&WaveSize(-3)==WaveSize(1),TEXT("size, speed and how many at once stop at their caps; a wave below 1 is wave 1"));
  Check(WaveSpeed(1)*PaceOf(EKind::Crab)<3.04,TEXT("in wave 1 even the fastest kind is slower than the player's run"));
 }
 {
  bool bSmall=true,bPale=false,bCrab=false,bTall=false;
  for(int32 I=0;I<WaveSize(1);++I)bSmall&=KindOf(1,I)==EKind::Small;
  for(int32 I=0;I<WaveSize(4);++I){const EKind K=KindOf(4,I);bPale|=K==EKind::Pale;bCrab|=K==EKind::Crab;bTall|=K==EKind::Tall;}
  Check(bSmall&&bPale&&bCrab&&bTall,TEXT("wave 1 is small Roswells only; by wave 4 the Pale One, the crab and the Tall One come too"));
  Check(HealthOf(EKind::Tall)>HealthOf(EKind::Pale)&&HealthOf(EKind::Pale)>HealthOf(EKind::Small)&&ScoreOf(EKind::Tall)>ScoreOf(EKind::Small),TEXT("the bigger the body, the more shots it takes and the more it scores"));
 }
 {
  FGun G;
  int32 Shots=0;
  for(int32 I=0;I<200&&G.Mag>0;++I){if(G.Fire())++Shots;G.Step(ShotGap);}
  Check(Shots==MagSize&&G.Mag==0&&!G.Fire(),TEXT("a magazine is twelve shots, and an empty gun does not fire"));
  FGun Quick;
  const bool bFirst=Quick.Fire(),bAtOnce=Quick.Fire();
  Quick.Step(ShotGap*.5);
  const bool bHalf=Quick.Fire();
  Quick.Step(ShotGap*.5);
  Check(bFirst&&!bAtOnce&&!bHalf&&Quick.Fire(),TEXT("a shot waits out the gap after the one before"));
  FGun Full;
  Check(!Full.StartReload(),TEXT("a full magazine is not changed"));
  const bool bBegan=G.StartReload(),bTwice=G.StartReload();
  const bool bEarly=G.Step(ReloadSeconds*.5);
  const bool bDuring=G.Fire();
  const bool bDone=G.Step(ReloadSeconds*.5+.001);
  Check(bBegan&&!bTwice&&!bEarly&&!bDuring&&bDone&&G.Mag==MagSize&&G.Fire(),TEXT("a reload takes its time, nothing fires during it, and the magazine is full after"));
 }
 {
  Check(Hurt(100,12)==88&&Hurt(5,12)==0&&Hurt(40,-9)==40&&Hurt(0,12)==0,TEXT("a touch takes its damage, never below nothing, and nothing heals by a negative one"));
  FRound R;
  R.Begin();
  const bool bWaits=R.Phase==EPhase::Banner&&!R.MaySpawn();
  R.Step(BannerSeconds+.01);
  bool bFight=R.Phase==EPhase::Fight&&R.MaySpawn()&&R.Left()==WaveSize(1);
  int32 Most=0;
  while(R.MaySpawn()){R.Spawned();Most=FMath::Max(Most,R.Out);}
  bFight&=Most==WaveAtOnce(1)&&!R.MaySpawn();
  Check(bWaits&&bFight,TEXT("a round begins with a banner, then wave 1 comes out, never more at once than the wave allows"));
  for(int32 I=0;I<3;++I)R.Touched();
  const int32 Before=R.Health;
  int32 Guard=0;
  while(R.Wave==1&&Guard++<200){if(R.Out==0)R.Spawned();R.Killed(EKind::Small);}
  Check(R.Wave==2&&R.Phase==EPhase::Banner&&R.Kills==WaveSize(1)&&R.ToCome==WaveSize(2)&&R.Out==0&&R.Health==FMath::Min(StartHealth,Before+WaveHeal)&&R.Score==WaveSize(1)*ScoreOf(EKind::Small),
   TEXT("the last one of a wave down: the next wave is announced, some health comes back, the score counts each kill"));
  R.Touched();
  Check(R.Health==FMath::Min(StartHealth,Before+WaveHeal),TEXT("nothing hurts during the banner between waves"));
  R.Step(BannerSeconds+.01);
  int32 Touches=0;
  while(R.Phase==EPhase::Fight&&Touches<100){R.Touched();++Touches;}
  const bool bCard=R.Phase==EPhase::Dead&&R.Health==0&&!R.Step(CardSeconds*.5);
  Check(bCard&&R.Step(CardSeconds*.5+.01)&&R.Wave==2,TEXT("at no health the round is over: the card says the wave reached, stays its time, then the shooter stops"));
  FRound L;
  L.Begin();L.Step(BannerSeconds+.01);L.Spawned();
  const int32 Was=L.Left();
  L.Lost();
  Check(L.Left()==Was&&L.Out==0&&L.Kills==0,TEXT("a body that went with its world is not a kill: it comes again"));
 }
 {
  // The trigger held for ten seconds at sixty frames a second: the muzzle's light, in any one second.
  FGun G;
  TArray<double> Flashes;
  double Last=-10;
  int32 CalmFlashes=0;
  for(int32 Frame=0;Frame<600;++Frame)
  {
   const double Now=Frame/60.;
   if(G.Mag<=0)G.Mag=MagSize;
   if(G.Fire())
   {
    if(FlashMay(Now,Last,false)){Last=Now;Flashes.Add(Now);}
    if(FlashMay(Now,-10,true))++CalmFlashes;
   }
   G.Step(1/60.);
  }
  int32 Worst=0;
  for(int32 I=0;I<Flashes.Num();++I){int32 N=0;for(int32 K=I;K<Flashes.Num()&&Flashes[K]<Flashes[I]+1.;++K)++N;Worst=FMath::Max(Worst,N);}
  Check(Flashes.Num()>10&&Worst<=3,FString::Printf(TEXT("the muzzle's light comes at most three times in any second with the trigger held (%d at most)"),Worst));
  Check(CalmFlashes==0,TEXT("with Calm flashes chosen the muzzle gives no light at all"));
 }
 {
  const FVector Eye(0,1.62,0),North(0,0,-1);
  const double Hit=RayBody(Eye,(FVector(0,.7,-10)-Eye).GetSafeNormal(),FVector(0,0,-10),1.2,.32);
  Check(Hit>9.6&&Hit<10.1,FString::Printf(TEXT("a shot aimed at the chest of a small one ten metres off strikes it (at %.2f m)"),Hit));
  Check(RayBody(Eye,(FVector(.8,.7,-10)-Eye).GetSafeNormal(),FVector(0,0,-10),1.2,.32)<0,TEXT("a shot past its side misses"));
  Check(RayBody(Eye,North,FVector(0,0,-10),1.2,.32)<0,TEXT("a level shot from the eye goes over a small one's head"));
  Check(RayBody(Eye,North,FVector(0,0,10),2.,.4)<0,TEXT("one behind the player is not struck"));
  Check(RayBody(Eye,FVector(0,.05,-1).GetSafeNormal(),FVector(0,0,-10),2.5,.4)>0,TEXT("a shot a little high still strikes a tall one"));
 }
 {
  // What is given back is what was kept, whatever happened in between.
  FStand Kept;
  Kept.Pos=FVector(-3.25,0,-29);Kept.FloorY=.4;Kept.Yaw=1.1;Kept.Pitch=-.2;Kept.bCrouch=true;
  FStand Cur=Kept;
  Cur.Pos=FVector(7,0,19);Cur.FloorY=0;Cur.Yaw=UE_DOUBLE_PI;Cur.Pitch=.5;Cur.bCrouch=false;
  const bool bMoved=!(Cur==Kept);
  Cur=Kept;
  Check(bMoved&&Cur==Kept&&Cur.Pos==FVector(-3.25,0,-29)&&Cur.bCrouch,TEXT("stopping gives back the place, the floor, the look and the crouch the player had"));
  FRound Fresh;
  Check(Fresh.Phase==EPhase::Off&&!Fresh.MaySpawn(),TEXT("a round not begun is off: nothing comes out, and nothing is held"));
 }
 return bOk;
}
