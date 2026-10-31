#include "S47RoswellGroup.h"
#include "S47RoswellWorld.h"
#include "S47RoswellMotion.h"
#include "S47RoswellAudio.h"
#include "S47TripoCreature.h"
#include "S47Glimpses.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47Three.h"
#include "Components/PoseableMeshComponent.h"

struct FS47RoswellGroup::FImpl
{
 struct FIndividual
 {
  int32 Id=0,Contacts=0;
  bool bActive=false,bRoof=false,bNatural=false;
  TWeakObjectPtr<USceneComponent> Root;
  FS47TripoCreature OwnedBody; FS47TripoCreature* Body=nullptr;
  FS47RoswellBrain Brain; FS47RoswellMotion Motion; FS47RoswellAudio Foley;
  TArray<FS47RoswellBrain::FPoint> Points;
  double Refresh=0,Life=120;
 };
 US47Main* M=nullptr; FS47RoswellWorld World;
 TArray<TUniquePtr<FIndividual>> Actors;
 FVector LastPlayer=FVector::ZeroVector;
 bool bLastPlayer=false,bDev=false; FString Scene;
 double Wait=120+FMath::FRand()*180;
 int32 Serial=0,PlansThisFrame=0,DecisionTurn=0;
 void IgnoreBodies()
 {
  World.IgnoredComponents.Reset();
  for (const auto& A:Actors) if (A->Body && A->Body->Mesh.IsValid()) World.IgnoredComponents.Add(A->Body->Mesh.Get());
  if (M && M->Night && M->Night->Glimpses)
   M->Night->Glimpses->Figure().Traverse([this](FS47Obj O)
   { if (const auto* N=M->Web->Node(O.I)) if (auto* C=Cast<UPrimitiveComponent>(N->Comp.Get())) World.IgnoredComponents.Add(C); });
 }
 FIndividual* Slot()
 {
  for (auto& A:Actors) if (!A->bActive) return A.Get();
  if (Actors.Num()>=6 || !M || !M->Web) return nullptr;
  auto A=MakeUnique<FIndividual>();
  auto* Root=NewObject<USceneComponent>(M->Web.Get());
  M->Web->AddInstanceComponent(Root); Root->SetMobility(EComponentMobility::Movable);
  Root->SetupAttachment(M->Web->GetRootComponent()); Root->RegisterComponent(); A->Root=Root;
  if (!A->OwnedBody.Build(M->Web.Get(),Root,*FString::Printf(TEXT("RoswellGroupBody%d"),Actors.Num()),
   TEXT("/Game/Licensed/TripoNPC/Roswell/SK_Tripo_Roswell.SK_Tripo_Roswell")))
  { Root->DestroyComponent(); return nullptr; }
  A->Body=&A->OwnedBody; auto* Out=A.Get(); Actors.Add(MoveTemp(A)); IgnoreBodies(); return Out;
 }
 void Hide(FIndividual& A)
 {
  A.bActive=false; A.Foley.Reset(); A.Motion.Reset(); A.Points.Reset();
  if (A.Body && A.Body->Mesh.IsValid()) A.Body->Mesh->SetVisibility(false,true);
 }
 double Occupied(const FVector& At,const FIndividual* Except=nullptr) const
 {
  for (const auto& A:Actors) if (A.Get()!=Except && A->bActive && FVector::DistSquared(A->Brain.Pos,At)<.8*.8) return 1.;
  return 0.;
 }
};

FS47RoswellGroup::FS47RoswellGroup(US47Main* Main,USceneComponent* FirstRoot,FS47TripoCreature* FirstBody):P(MakeUnique<FImpl>())
{
 P->M=Main; P->World.Init(Main);
 auto A=MakeUnique<FImpl::FIndividual>(); A->Root=FirstRoot; A->Body=FirstBody; P->Actors.Add(MoveTemp(A)); P->IgnoreBodies();
}
FS47RoswellGroup::~FS47RoswellGroup()
{
 Clear();
 for (int32 I=1;I<P->Actors.Num();++I)
 { auto& A=*P->Actors[I]; if (A.Body && A.Body->Mesh.IsValid()) A.Body->Mesh->DestroyComponent(); if (A.Root.IsValid()) A.Root->DestroyComponent(); }
}
bool FS47RoswellGroup::Developer() const { return P->bDev; }
void FS47RoswellGroup::Clear()
{
 for (auto& A:P->Actors) P->Hide(*A);
 P->bDev=false; P->bLastPlayer=false; P->Wait=120+FMath::FRand()*180;
}
void FS47RoswellGroup::SetMode(FS47RoswellBrain::EMode Mode)
{
 for (auto& A:P->Actors) if (A->bActive && !A->bNatural) A->Brain.SetMode(Mode);
}
int32 FS47RoswellGroup::Spawn(int32 Count,FS47RoswellBrain::EMode Mode,bool bRoof,bool bNatural)
{
 auto* M=P->M;
 if (!M || !M->Player || !M->Web || !M->Night || !M->Night->Glimpses || !M->Night->Glimpses->HasNativeBody()) return 0;
 if (!bNatural) { Clear(); P->bDev=true; }
 P->Scene=M->Web->Area(); P->IgnoreBodies();
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 const auto Cam=M->Night->GlimpseIn(true).Cam;
 auto Places=P->World.Candidates(Feet,P->Scene,bRoof);
 // Dev ground placements are in front of the actual player; no wall/floor bypass.
 if (!bRoof && !bNatural)
 {
  Places.Reset();
  for (double D:{2.8,3.5,4.5,5.5,7.5}) for (double Turn:{0.,.3,-.3,.55,-.55,.8,-.8,1.,-1.,1.25,-1.25,1.5,-1.5})
  {
   const double Yaw=M->Player->Yaw+Turn;
   FVector At=Feet+FVector(-FMath::Sin(Yaw)*D,0,-FMath::Cos(Yaw)*D);
   const auto Floor=P->World.Floor(At); if (!Floor) continue; At.Y=*Floor;
   if (P->World.IsStandable(At) && P->World.ClearSight(Cam.Pos,At+FVector(0,1,0)))
    Places.Add({TEXT("developer"),At,false,false,false});
  }
 }
 if (bRoof) UE_LOG(LogTemp,Display,TEXT("S47: Roswell supported roof candidates=%d"),Places.Num());
 int32 Spawned=0;
 for (const auto& Place:Places)
 {
  if (Spawned>=FMath::Clamp(Count,1,6) || P->Occupied(Place.Pos)>=1) continue;
  if (bNatural)
  {
   const FVector To=Place.Pos+FVector(0,1.08,0)-Cam.Pos;
   if (To.Size()>26 || To.Size()<3) continue;
   // A newly arrived body must be behind actual cover or outside the player's view.
   if (FVector::DotProduct(Cam.Forward,To.GetSafeNormal())>.35 && P->World.ClearSight(Cam.Pos,Place.Pos+FVector(0,.9,0))) continue;
  }
  auto* A=P->Slot(); if (!A || !A->Body || !A->Body->Ready()) break;
  A->Id=++P->Serial; A->bActive=true; A->bRoof=bRoof; A->bNatural=bNatural;
  A->Contacts=0; A->Refresh=0; A->Life=90+FMath::FRand()*90;
  A->Motion.Reset(); A->Foley.Reset();
  A->Brain.Start(bNatural?FS47RoswellBrain::EMode::Curious:Mode,Place.Pos,
   FMath::Atan2(Feet.X-Place.Pos.X,Feet.Z-Place.Pos.Z),A->Id*491+317);
  ++Spawned; if (bNatural) ++NightCount;
 }
 return Spawned;
}
void FS47RoswellGroup::Update(double Dt,const FS47GlimpsesIn& Input)
{
 auto* M=P->M; if (!M || !M->Web || !M->Player) return;
 if (!P->Scene.IsEmpty() && M->Web->Area()!=P->Scene) Clear();
 P->Scene=M->Web->Area();
 const bool bHeld=M->Mode!=US47Main::EMode::Play || M->bPausedByMenu || (M->UI && M->UI->Modal()) ||
  Input.bHeld || (!P->bDev && !Input.bOk);
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 double Speed=!bHeld && P->bLastPlayer && Dt>0?FVector::Dist(Feet,P->LastPlayer)/Dt:0.;
 if (Speed>12) Speed=0; P->LastPlayer=Feet; P->bLastPlayer=true;
 int32 Active=0;
 for (const auto& A:P->Actors) Active+=A->bActive?1:0;
 const bool RoofBusy=M->Night && M->Night->Hooks.RoofStepsBusy && M->Night->Hooks.RoofStepsBusy();
 if (!bHeld && !P->bDev && Input.bOk && !Input.Scene.IsEmpty() && Active==0 && NightCount<8 && Input.May!=TOptional<bool>(false))
 {
  P->Wait-=FMath::Min(Dt,.25)*Input.Pace.Get(1.);
  if (P->Wait<=0)
  {
   const bool Roof=!RoofBusy && P->Scene==TEXT("saro") && FMath::FRand()<.3;
   const int32 N=FMath::Min(8-NightCount,FMath::FRand()<.25?2:1);
   if (!Spawn(N,FS47RoswellBrain::EMode::Curious,Roof,true) && Roof) Spawn(N,FS47RoswellBrain::EMode::Curious,false,true);
   P->Wait=30+FMath::FRand()*40;
  }
 }
 P->IgnoreBodies(); P->PlansThisFrame=0;
 const int32 First=P->Actors.IsEmpty()?0:P->DecisionTurn++%P->Actors.Num();
 for (int32 Index=0;Index<P->Actors.Num();++Index)
 {
  auto& A=*P->Actors[(First+Index)%P->Actors.Num()]; if (!A.bActive || !A.Root.IsValid() || !A.Body) continue;
  const bool Held=bHeld || (A.bNatural && A.bRoof && RoofBusy);
  if (!Held) A.Refresh-=FMath::Min(Dt,.25);
  if (A.Refresh<=0)
  {
   A.Points.Reset();
   for (const auto& Point:P->World.Candidates(A.Brain.Pos,P->Scene,A.bRoof))
    A.Points.Add({Point.Id,Point.Pos,Point.bCover,Point.bRoof,Point.bWindow});
   A.Refresh=1.1+(A.Id%5)*.19;
  }
  FS47RoswellBrain::FWorld W;
  W.Points=A.Points;
  bool PlannedThisActor=false;
  W.PlanningReady=[this,&PlannedThisActor]{return !PlannedThisActor && P->PlansThisFrame<2;};
  W.GroundedStep=[this](const FVector& From,const FVector& Desired){return P->World.GroundedStep(From,Desired);};
  W.FindPath=[this,&A,&PlannedThisActor](const FVector& From,const FVector& To)
  {
   if (PlannedThisActor || P->PlansThisFrame>=2) return TArray<FVector>();
   PlannedThisActor=true; ++P->PlansThisFrame;
   auto Route=P->World.Path(From,To);
   if (A.bRoof) UE_LOG(LogTemp,Display,TEXT("S47: Roswell roof route from=%s to=%s points=%d"),*From.ToCompactString(),*To.ToCompactString(),Route.Num());
   return Route;
  };
  W.CanStand=[this](const FVector& At){return P->World.IsStandable(At);};
  W.CanMove=[this](const FVector& From,const FVector& To){return P->World.MoveAllowed(From,To);};
  W.Sight=[this](const FVector& From,const FVector& To){return P->World.ClearSight(From,To);};
  W.Occupancy=[this,&A](const FVector& At){return P->Occupied(At,&A);};
  FS47RoswellBrain::FInput In; In.bHeld=Held; In.PlayerFeet=Feet; In.PlayerEye=Input.Cam.Pos;
  In.PlayerForward=Input.Cam.Forward; In.PlayerSpeed=Speed; In.bFlashlight=Input.bFlashlight;
  A.Brain.Tick(Dt,In,W);
  A.Root->SetWorldLocation(S47Three::ToUE(A.Brain.Pos));
  A.Root->SetWorldRotation(FRotationMatrix::MakeFromX(S47Three::DirToUE(FVector(FMath::Sin(A.Brain.Yaw),0,FMath::Cos(A.Brain.Yaw)))).Rotator());
  FS47RoswellPose Pose; Pose.bHeld=Held; Pose.SpeedMps=A.Brain.Velocity.Size();
  Pose.DistanceTravelled=A.Brain.DistanceTravelled; Pose.Crouch=A.Brain.Crouch;
  Pose.HeadYaw=-FMath::Clamp(FMath::UnwindRadians(A.Brain.HeadYaw-A.Brain.Yaw),-.65,.65);
  Pose.HeadPitch=FMath::Clamp(FMath::Atan2(A.Brain.LookTarget.Y-A.Brain.Eye().Y,
   FVector2D::Distance(FVector2D(A.Brain.LookTarget.X,A.Brain.LookTarget.Z),FVector2D(A.Brain.Pos.X,A.Brain.Pos.Z))),-.35,.35);
  using E=FS47RoswellBrain::EState;
  Pose.Mode=A.Brain.State==E::Contact?ES47RoswellMotion::Grab:
   Pose.SpeedMps>1.6?ES47RoswellMotion::Run:Pose.SpeedMps>.03?(Pose.Crouch>.2?ES47RoswellMotion::Stalk:ES47RoswellMotion::Walk):
   A.Brain.State==E::Peek?ES47RoswellMotion::Peek:A.Brain.State==E::Listen?ES47RoswellMotion::Listen:ES47RoswellMotion::Idle;
  const auto Contact=A.Motion.Apply(*A.Body,Pose,Dt);
  A.Contacts+=(Contact.bLeft?1:0)+(Contact.bRight?1:0);
  FS47RoswellFoley Surface; Surface.bHeld=Held; Surface.bRoof=A.bRoof; Surface.bMetal=false;
  Surface.Crouch=A.Brain.Crouch; Surface.bOccluded=!P->World.ClearSight(Input.Cam.Pos,A.Brain.Pos+FVector(0,.1,0));
  A.Foley.Step(M->Audio,Contact,Surface);
  if (A.Brain.bContact && M->UI) M->UI->Toast(TEXT("Roswell test: caught you. Move away, change behaviour or clear the group in F10."),3);
  if (!Held && A.bNatural)
  {
   A.Life-=FMath::Min(Dt,.25);
   if (A.Life<=0 && !A.Brain.bPlayerSeesMe && !P->World.ClearSight(Input.Cam.Pos,A.Brain.Eye())) P->Hide(A);
  }
  else if (!Held && A.Brain.bStandDone) P->Hide(A);
 }
 if (P->bDev && !P->Actors.ContainsByPredicate([](const auto& A){return A->bActive;})) P->bDev=false;
}
TArray<FS47RoswellGroup::FView> FS47RoswellGroup::Views() const
{
 TArray<FView> Out;
 for (const auto& A:P->Actors) if (A->bActive)
  Out.Add({A->Id,A->Brain.Mode,A->Brain.State,A->Brain.Pos,A->Brain.Eye(),A->Brain.DistanceTravelled,A->Brain.Age,
   A->Brain.Velocity.Size(),A->Brain.Crouch,A->bRoof,A->Body && A->Body->Rendered(),A->Contacts,A->Foley.Played()});
 return Out;
}
