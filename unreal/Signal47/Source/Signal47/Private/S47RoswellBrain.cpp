#include "S47RoswellBrain.h"
#include "S47TripoCreature.h"
#include "S47Sneak.h"

namespace
{
 double Distance(const FVector& A,const FVector& B)
 { return FVector2D::Distance(FVector2D(A.X,A.Z),FVector2D(B.X,B.Z)); }
 bool Finite(const FVector& P)
 { return FMath::IsFinite(P.X) && FMath::IsFinite(P.Y) && FMath::IsFinite(P.Z); }
 double Wrap(double A) { return FMath::UnwindRadians(A); }
 FVector Forward(double Yaw) { return FVector(FMath::Sin(Yaw),0,FMath::Cos(Yaw)); }
 double Facing(const FVector& From,const FVector& To) { return FMath::Atan2(To.X-From.X,To.Z-From.Z); }
}

const TCHAR* FS47RoswellBrain::ModeName(EMode Value)
{
 switch (Value) { case EMode::Stand:return TEXT("stand"); case EMode::Hunt:return TEXT("hunt"); default:return TEXT("curious"); }
}

const TCHAR* FS47RoswellBrain::StateName(EState Value)
{
 switch (Value)
 {
  case EState::Idle:return TEXT("idle"); case EState::Listen:return TEXT("listen");
  case EState::Stalk:return TEXT("stalk"); case EState::Peek:return TEXT("peek");
  case EState::Hide:return TEXT("hide"); case EState::Flee:return TEXT("flee");
  case EState::Hunt:return TEXT("hunt"); case EState::Search:return TEXT("search");
  case EState::Contact:return TEXT("contact"); default:return TEXT("unknown");
 }
}

void FS47RoswellBrain::Start(EMode InMode,const FVector& At,double FacingYaw,int32 Seed)
{
 Random.Initialize(Seed);
 Pos=Finite(At)?At:FVector::ZeroVector; Yaw=HeadYaw=FMath::IsFinite(FacingYaw)?FacingYaw:0.;
 Velocity=FVector::ZeroVector; Age=StateAge=MemoryAge=Noticed=DistanceTravelled=0;
 ContactCool=0; HeardAge=1e6; LastListenAt=-10.; Crouch=DesiredCrouch=DesiredSpeed=0;
 bHasPlayerMemory=bSeesPlayer=bPlayerSeesMe=bContact=bStandDone=false;
 bTargetCover=bTargetRoof=bTargetWindow=false; bAtGoal=true;
 LastKnownPlayer=Goal=Pos; LastPlayerEye=LookTarget=Eye()+Forward(Yaw)*3.;
 Path.Reset(); NextPoint=ChoiceCursor=0; bPlanningDeferred=false; TargetId.Reset(); Mode=InMode; State=EState::Idle;
 DecisionAt=Random.FRandRange(1.0f,2.3f); RepathAt=0;
 NoiseAt=Pos; NoiseAge=1e6; bLurePending=bGoLook=false; LookTry=0;
}

void FS47RoswellBrain::SetMode(EMode InMode)
{
 if (Mode==InMode) return;
 const bool KeepMemory=InMode==EMode::Hunt && bHasPlayerMemory && MemoryAge<12.;
 Mode=InMode; Path.Reset(); NextPoint=0; bAtGoal=true; Velocity=FVector::ZeroVector;
 bContact=bStandDone=false; bHasPlayerMemory=KeepMemory; if (!KeepMemory) MemoryAge=0; Noticed=0;
 bTargetCover=bTargetRoof=bTargetWindow=false; TargetId.Reset();
 LastListenAt=Age-10.; HeardAge=1e6; NoiseAge=1e6; bLurePending=bGoLook=false; LookTry=0;
 State=EState::Idle; StateAge=0; ChoiceCursor=0; bPlanningDeferred=false; DecisionAt=Age+1.; RepathAt=Age;
}

void FS47RoswellBrain::Enter(EState NewState)
{
 if (State==NewState) return;
 State=NewState; StateAge=0;
 if (NewState==EState::Idle) DecisionAt=Age+Random.FRandRange(1.4f,3.2f);
 if (NewState==EState::Listen) LastListenAt=Age;
 if (NewState==EState::Peek) DecisionAt=Age+Random.FRandRange(2.8f,3.6f);
 if (NewState==EState::Flee) bAtGoal=false;
}

bool FS47RoswellBrain::Seen(const FInput& In,const FWorld& World) const
{
 if (!In.bPlayerValid || !Finite(In.PlayerFeet) || !Finite(In.PlayerEye) || !World.Sight) return false;
 const FVector D=In.PlayerEye-Eye();
 // 24 m, less for a player keeping low (SNEAK-20261108); the lower eye is also more often behind something.
 const double SeesFar=S47Sneak::SightReach(In.PlayerQuiet);
 if (D.SizeSquared()>SeesFar*SeesFar) return false;
 if (D.SizeSquared()<1e-6) return World.Sight(Eye(),In.PlayerEye);
 // The eyes follow the same limited head turn as the skin, not a 360 degree sensor.
 const double ViewYaw=Yaw+FMath::Clamp(Wrap(HeadYaw-Yaw),-.65,.65);
 return FVector::DotProduct(Forward(ViewYaw),D.GetSafeNormal())>.34 && World.Sight(Eye(),In.PlayerEye);
}

bool FS47RoswellBrain::Watched(const FInput& In,const FWorld& World) const
{
 if (!In.bPlayerValid || !Finite(In.PlayerEye) || !Finite(In.PlayerForward) || !World.Sight) return false;
 const FVector D=Eye()-In.PlayerEye;
 return D.SizeSquared()<32.*32. && D.SizeSquared()>1e-6 &&
  FVector::DotProduct(In.PlayerForward.GetSafeNormal(),D.GetSafeNormal())>.819 && World.Sight(In.PlayerEye,Eye());
}

TOptional<FVector> FS47RoswellBrain::Heard(const FInput& In,const FWorld& World) const
{
 if (In.Noise && (!Finite(In.Noise->At) || !FMath::IsFinite(In.Noise->Loudness) || In.Noise->Loudness<=0)) return {};
 // the wind covers what it can (WEATHER-20261108)
 const double Scale=FMath::IsFinite(In.HearScale)?FMath::Clamp(In.HearScale,.3,1.):1.;
 TOptional<FVector> Best; double BestMargin=0;
 // PHYSICS-20261108: of the noise and the feet, the one it hears best where it stands, not the one that carries furthest: a stone
 // landing beside it is not lost to footsteps that carry nine metres from twelve metres off.
 const auto Try=[&](const FVector& At,double Reach)
 {
  Reach*=Scale;
  const double D=FVector::Dist(Eye(),At);
  if (Reach<=0 || D>Reach || FMath::Abs(At.Y-Pos.Y)>6.) return;
  // A roof/wall muffles sound instead of granting a perfect through-wall view. Sound needs no clear line to a point on the ground:
  // measured to 0.3 m over where it was made, and round a kerb or a rise in the ground to a metre over it; a wall stops both.
  if (D>Reach*.42 && (!World.Sight || (!World.Sight(Eye(),At+FVector(0,.3,0)) && !World.Sight(Eye(),At+FVector(0,1.,0))))) return;
  if (!Best || Reach-D>BestMargin) { Best=At; BestMargin=Reach-D; }
 };
 if (In.Noise) Try(In.Noise->At,FMath::Clamp(In.Noise->Loudness,0.,2.)*10.);
 // The feet as well: 9 m walking, 18 running; a crouching walk carries much less far (S47Sneak.h).
 // Stationary input cannot generate footsteps; the adapter supplies measured speed.
 if (In.bPlayerValid && Finite(In.PlayerFeet)) Try(In.PlayerFeet,S47Sneak::StepReach(In.PlayerSpeed,In.PlayerQuiet));
 return Best;
}

bool FS47RoswellBrain::GoLook(const FVector& At,const FWorld& World)
{
 // To where it was heard, or as near to it as there is ground to stand on: the spot, then a little short of it on the way.
 const double Far=Distance(Pos,At);
 const FVector Back=FVector(Pos.X-At.X,0,Pos.Z-At.Z).GetSafeNormal();
 // STONE-LURE-20261109: where it got to is kept (LookTry): one route a tick, so a spot without one gives way to the next on the
 // following tick. Asked for from the spot again each tick, a stone by a wall had it plan the spot, fail, and wait for the budget.
 static constexpr double Shorts[]={0.,.8,1.6,2.4};
 for (; LookTry<int32(UE_ARRAY_COUNT(Shorts)); ++LookTry)
 {
  const double Short=Shorts[LookTry];
  if (Short>0 && Short>Far-.3) break;
  FVector To=At+Back*Short; To.Y=FMath::Lerp(At.Y,Pos.Y,Far>.1?Short/Far:0.);
  if (World.GroundedStep) { const auto Grounded=World.GroundedStep(To,To); if (!Grounded.IsSet() || !Finite(Grounded.GetValue())) continue; To=Grounded.GetValue(); }
  if (Plan(To,TEXT("heard-sound"),World)) { bTargetCover=bTargetRoof=bTargetWindow=false; return true; }
  if (bPlanningDeferred) return false; // this same place again on its next turn
 }
 return false;
}

bool FS47RoswellBrain::Plan(const FVector& To,const FString& Id,const FWorld& World)
{
 bPlanningDeferred=false;
 bAtGoal=false;
 if (!Finite(To) || !World.CanStand || !World.CanMove || !World.CanStand(To)) return false;
 if (Distance(Pos,To)<.08 && FMath::Abs(Pos.Y-To.Y)<.15)
 { Goal=To; TargetId=Id; Path.Reset(); NextPoint=0; bAtGoal=true; return true; }
 TArray<FVector> Proposed;
 if (World.PlanningReady && !World.PlanningReady()) { bPlanningDeferred=true; return false; }
 if (World.FindPath) Proposed=World.FindPath(Pos,To);
 else if (World.CanMove(Pos,To)) Proposed={To};
 if (Proposed.IsEmpty() || Proposed.Num()>96 || FVector::Dist(Proposed.Last(),To)>.15) return false;
 FVector From=Pos;
 for (const FVector& P:Proposed)
 {
  if (!Finite(P) || !World.CanStand(P) || !World.CanMove(From,P)) return false;
  From=P;
 }
 Goal=To; TargetId=Id; Path=MoveTemp(Proposed); NextPoint=0; bAtGoal=false;
 return true;
}

bool FS47RoswellBrain::Choose(const FInput&,const FWorld& World,bool bFlee,bool bPreferCover)
{
 bPlanningDeferred=false;
 if (World.PlanningReady && !World.PlanningReady()) { bPlanningDeferred=true; return false; }
 struct FCandidate { FPoint Point; double Score; };
 TArray<FCandidate> Candidates;
 // Only a perceived location affects decisions. Unseen PlayerFeet never leaks
 // into scoring, a hunt target or the cover on the other side of a wall.
 const FVector Known=bHasPlayerMemory?LastKnownPlayer:Pos+Forward(Yaw)*5.;
 const FVector KnownEye=bHasPlayerMemory?LastPlayerEye:Eye()+Forward(Yaw)*5.;
 const int32 Limit=FMath::Min(World.Points.Num(),48);
 for (int32 I=0; I<Limit; ++I)
 {
  const FPoint& P=World.Points[I];
  const double D=Distance(Pos,P.Pos);
  if (!Finite(P.Pos) || D<.6 || D>28. || FMath::Abs(P.Pos.Y-Pos.Y)>.65 || !World.CanStand || !World.CanStand(P.Pos)) continue;
  const double Occupancy=World.Occupancy?FMath::Max(0.,World.Occupancy(P.Pos)):0.;
  if (Occupancy>=1.) continue;
  const bool Concealed=World.Sight && !World.Sight(KnownEye,P.Pos+FVector(0,P.bWindow?.65:.78,0));
  double Score=Random.FRandRange(0.f,1.4f)-D*.08-Occupancy*7.;
  if (P.Id==TargetId) Score-=4.;
  if (bFlee)
   Score+=Distance(P.Pos,Known)*.42+(Concealed?7.:0.)+(P.bCover?3.:0.);
  else
  {
   Score+=(bPreferCover && P.bCover?3.:0.)+(Concealed?1.5:0.)+(P.bWindow?1.6:0.);
   if (bHasPlayerMemory) Score-=FMath::Abs(Distance(P.Pos,Known)-7.)*.19;
   if (bHasPlayerMemory && Distance(P.Pos,Known)<3.) Score-=8.;
   if (D<.5) Score-=3.;
  }
  Candidates.Add({P,Score});
 }
 Candidates.Sort([](const FCandidate& A,const FCandidate& B) { return A.Score>B.Score; });
 // If authored cover is unreachable, try bounded real ground directions away
 // from the last perceived player. Rotate through these too, rather than using
 // a whole frame's route budget retrying the same inaccessible top-ranked cover.
 if (bFlee)
 {
  const double Away=Facing(Known,Pos);
  for (double Turn:{0.,.65,-.65,1.2,-1.2,2.0,-2.0})
  {
   const FVector P=Pos+Forward(Away+Turn)*5.;
   if (World.Occupancy && World.Occupancy(P)>=1.) continue;
   if (!World.CanStand || !World.CanStand(P)) continue;
   Candidates.Add({FPoint{FString::Printf(TEXT("escape-%d"),Candidates.Num()),P,false,false,false},-100.});
  }
 }
 if (Candidates.IsEmpty()) return false;
 const FPoint& P=Candidates[ChoiceCursor%Candidates.Num()].Point;
 const bool bPlanned=Plan(P.Pos,P.Id,World);
 // An actual rejection advances to another candidate. A budget deferral must
 // retry this candidate once the group scheduler grants this individual a turn.
 if (!bPlanningDeferred) ChoiceCursor=(ChoiceCursor+1)%Candidates.Num();
 if (!bPlanned) return false;
 bTargetCover=P.bCover; bTargetRoof=P.bRoof; bTargetWindow=P.bWindow;
 return true;
}

void FS47RoswellBrain::Move(double Dt,const FWorld& World)
{
 const double OldSpeed=Velocity.Size(); Velocity=FVector::ZeroVector;
 if (DesiredSpeed<=0 || NextPoint>=Path.Num()) return;
 while (NextPoint<Path.Num() && FVector::Dist(Pos,Path[NextPoint])<.035) ++NextPoint;
 if (NextPoint>=Path.Num()) { bAtGoal=true; Path.Reset(); NextPoint=0; return; }
 const FVector D=Path[NextPoint]-Pos;
 // Unhurried, it goes in steps and stops between them.
 const bool bSneaking=DesiredSpeed<1.3&&(State==EState::Stalk||State==EState::Search||Crouch>.2);
 const double Pulse=!bSneaking?1.:bSteadySneak?(FMath::Fmod(Age,8.3)<6.5?.9:.02):S47Roswell::StalkPulse(Age);
 const double Speed=FMath::Min(DesiredSpeed*Pulse,OldSpeed+Dt*3.8);

 FVector To=Pos+D.GetSafeNormal()*FMath::Min(D.Size(),Speed*Dt);
 bool bGrounded=true;
 if (World.GroundedStep)
 {
  const auto Grounded=World.GroundedStep(Pos,To);
  bGrounded=Grounded.IsSet() && Finite(Grounded.GetValue()) && Distance(To,Grounded.GetValue())<.01;
  if (bGrounded) To=Grounded.GetValue();
 }
 if (!bGrounded || !World.CanMove || !World.CanStand || !World.CanStand(To) || !World.CanMove(Pos,To) ||
  (World.Occupancy && World.Occupancy(To)>=1.))
 {
  // A door can close after planning. Stop here; never snap to the route's end
  // or report that a blocked destination was reached.
  Path.Reset(); NextPoint=0; bAtGoal=false; RepathAt=Age+.65;
  if (State==EState::Hunt) Enter(EState::Search);
  else if (State==EState::Stalk) { Enter(EState::Listen); DecisionAt=Age+1.; }
  return;
 }
 const FVector Before=Pos; Pos=To;
 const double Travel=FVector::Dist(Pos,Before); DistanceTravelled+=Travel;
 Velocity=(Pos-Before)/FMath::Max(Dt,1e-6);
 if (Travel>1e-6)
 {
  const double TargetYaw=Facing(Before,Pos);
  Yaw=Wrap(Yaw+FMath::Clamp(Wrap(TargetYaw-Yaw),-Dt*3.3,Dt*3.3));
 }
 if (FVector::Dist(Pos,Path[NextPoint])<.035)
 {
  ++NextPoint;
  if (NextPoint>=Path.Num()) { Path.Reset(); NextPoint=0; bAtGoal=true; }
 }
}

void FS47RoswellBrain::Tick(double Dt,const FInput& In,const FWorld& World)
{
 bContact=false; bPlanningDeferred=false;
 if (In.bHeld || !FMath::IsFinite(Dt) || Dt<=0) return;
 // Bounded physical steps prevent long frames from jumping through a doorway.
 double Left=FMath::Min(Dt,.25);
 while (Left>1e-8) { const double StepDt=FMath::Min(Left,.05); Step(StepDt,In,World); Left-=StepDt; }
}

void FS47RoswellBrain::Step(double Dt,const FInput& In,const FWorld& World)
{
 Age+=Dt; StateAge+=Dt; MemoryAge+=Dt; HeardAge+=Dt; NoiseAge+=Dt; ContactCool=FMath::Max(0.,ContactCool-Dt);
 bSeesPlayer=Seen(In,World); bPlayerSeesMe=Watched(In,World);
 const auto Sound=Heard(In,World);
 // PHYSICS-20261108: a sound from somewhere the player is not (a stone, a can), as against their feet, their breath or the walkie.
 const bool bLure=Sound && In.Noise && Sound->Equals(In.Noise->At) && (!In.bPlayerValid || !Finite(In.PlayerFeet) || Distance(*Sound,In.PlayerFeet)>2.);
 if (bLure)
 {
  // The same sound heard again the next tick (S47Noise keeps one a third of a second) goes on from the place GoLook got to.
  if (NoiseAge>1.5 || Distance(NoiseAt,*Sound)>1.) LookTry=0;
  NoiseAt=*Sound; NoiseAge=0; bLurePending=bSeesPlayer; bGoLook=Mode==EMode::Curious && !(TargetId==TEXT("heard-sound") && !bAtGoal && Distance(Goal,NoiseAt)<2.);
 }
 if (bSeesPlayer)
 {
  LastKnownPlayer=In.PlayerFeet; LastPlayerEye=In.PlayerEye;
  bHasPlayerMemory=true; MemoryAge=0;
 }
 else if (Sound)
 {
  LastKnownPlayer=*Sound; LastPlayerEye=*Sound+FVector(0,1.5,0);
  bHasPlayerMemory=true; MemoryAge=0; HeardAge=0; bLurePending=false;
 }
 else if (bLurePending && NoiseAge<3.)
 {
  // Heard while it still had the player in view: once it does not, the newer sound wins over what it saw before.
  LastKnownPlayer=NoiseAt; LastPlayerEye=NoiseAt+FVector(0,1.5,0);
  bHasPlayerMemory=true; MemoryAge=NoiseAge; bLurePending=false;
 }
 if (NoiseAge>=3.) bLurePending=bGoLook=false;
 if (MemoryAge>(Mode==EMode::Hunt?16.:12.)) bHasPlayerMemory=false;
 if (bHasPlayerMemory) LookTarget=LastPlayerEye;
 else LookTarget=Eye()+Forward(Yaw+.35*FMath::Sin(Age*.63))*4.;
 // A curious one turns its head to a fresh sound, whatever it was watching.
 if (Mode==EMode::Curious && NoiseAge<1.5) LookTarget=NoiseAt+FVector(0,.3,0);
 HeadYaw=Facing(Eye(),LookTarget);

 DesiredSpeed=DesiredCrouch=0;
 // Held by a lamp or a sound for a moment, from the visit: whatever it was doing waits, in either mode.
 if (In.HoldLook.IsSet() && Finite(*In.HoldLook) && Mode!=EMode::Stand)
 {
  LookTarget=*In.HoldLook; HeadYaw=Facing(Eye(),LookTarget);
  if (In.bHoldHeard) { LastKnownPlayer=*In.HoldLook; LastPlayerEye=*In.HoldLook+FVector(0,1.5,0); bHasPlayerMemory=true; MemoryAge=0; HeardAge=0; }
  Path.Reset(); NextPoint=0; bAtGoal=true; Velocity=FVector::ZeroVector;
  Enter(EState::Listen); DecisionAt=Age+1.; RepathAt=Age;
  Yaw=Wrap(Yaw+FMath::Clamp(Wrap(Facing(Pos,LookTarget)-Yaw),-Dt*2.4,Dt*2.4));
  Crouch+=FMath::Clamp(.22-Crouch,-Dt*.9,Dt*.9);
  if (In.HoldBack>0)
  {
   const FVector Away=FVector(Pos.X-LookTarget.X,0,Pos.Z-LookTarget.Z).GetSafeNormal();
   // Straight back from it, or off to either side when something stands behind.
   for (const double Turn : {0.,.7,-.7,1.4,-1.4})
   {
    FVector To=Pos+Away.RotateAngleAxisRad(Turn,FVector(0,1,0))*In.HoldBack*Dt;
    bool bStep=true;
    if (World.GroundedStep) { const auto Grounded=World.GroundedStep(Pos,To); bStep=Grounded.IsSet()&&Finite(Grounded.GetValue()); if (bStep) To=Grounded.GetValue(); }
    const bool bStand=bStep&&World.CanStand&&World.CanStand(To),bMove=bStand&&World.CanMove&&World.CanMove(Pos,To),bFree=bMove&&!(World.Occupancy&&World.Occupancy(To)>=1.);
    if (bFree)
    {
     const FVector Before=Pos; Pos=To; DistanceTravelled+=FVector::Dist(Pos,Before);
     Velocity=(Pos-Before)/FMath::Max(Dt,1e-6);
     break;
    }
   }
  }
  return;
 }
 if (Mode==EMode::Stand)
 {
  Velocity=FVector::ZeroVector; Path.Reset(); NextPoint=0; bAtGoal=true;
  bStandDone=StateAge>=8.; Crouch=FMath::Max(0.,Crouch-Dt*2.);
  return;
 }
 bStandDone=false;
 // Hunt is explicit (developer control or a warned Freeform event), never an escalation of ordinary curiosity.
 if (Mode==EMode::Hunt)
 {
  const bool Contact=In.bCanContact && bSeesPlayer && Distance(Pos,In.PlayerFeet)<.92 &&
   FMath::Abs(Pos.Y-In.PlayerFeet.Y)<.65 && World.Sight && World.Sight(Eye(),In.PlayerEye) &&
   World.CanMove && World.CanMove(Pos,In.PlayerFeet);
  if (Contact)
  {
   Enter(EState::Contact); Path.Reset(); NextPoint=0; bAtGoal=true;
   if (ContactCool<=0) { bContact=true; ContactCool=4.; }
  }
  else if (bHasPlayerMemory)
  {
   Enter(bSeesPlayer && (!Path.IsEmpty() || Age>=RepathAt)?EState::Hunt:EState::Search);
   DesiredSpeed=bSeesPlayer?HuntSeenSpeed:HuntSearchSpeed; DesiredCrouch=bSeesPlayer?.08:.24;
   if (Age>=RepathAt && (Path.IsEmpty() || FVector::Dist(Goal,LastKnownPlayer)>.55))
   {
    const bool Planned=Plan(LastKnownPlayer,TEXT("last-perceived-player"),World);
    RepathAt=Age+(Planned?.5:(bPlanningDeferred?.05:.35));
   }
   if (!bSeesPlayer && bAtGoal && StateAge>2.5)
   { bHasPlayerMemory=false; Path.Reset(); Enter(EState::Listen); DecisionAt=Age+2.; }
  }
  else
  {
   if (State==EState::Hunt || State==EState::Search || State==EState::Contact) Enter(EState::Listen);
   if (Age>=DecisionAt)
   {
    if (Choose(In,World,false,true)) { Enter(EState::Stalk); DecisionAt=Age+3.; }
    else { Enter(EState::Listen); DecisionAt=Age+(bPlanningDeferred?.05:.25); }
   }
   if (State==EState::Stalk) DesiredSpeed=.8;
  }
 }
 else
 {
  // Observation takes a readable moment. Being seen from behind alone does not
  // magically tell the alien where the player is; light or a perceived sound can.
  const bool Alarm=bPlayerSeesMe && (bSeesPlayer || HeardAge<2. || In.bFlashlight);
  Noticed=Alarm?Noticed+Dt*(In.bFlashlight?1.5:1.):FMath::Max(0.,Noticed-Dt*.5);
  const bool TooNear=bSeesPlayer && Distance(Pos,LastKnownPlayer)<3.;
  if (Age>1.4 && (Noticed>.65 || TooNear) && State!=EState::Flee && State!=EState::Hide)
  {
   Enter(EState::Flee); Path.Reset(); NextPoint=0; bAtGoal=false;
   Choose(In,World,true,true); RepathAt=Age+(bPlanningDeferred?.05:.25);
  }
  // PHYSICS-20261108: a stone off to the side draws it: it goes, slowly, to look where it landed, from whatever it was doing.
  // Not out of hiding or flight; it stays put there.
  if (bGoLook && State!=EState::Flee && State!=EState::Hide)
  {
   if (GoLook(NoiseAt,World)) { Enter(EState::Stalk); bGoLook=false; }
   else if (!bPlanningDeferred) bGoLook=false;
  }
  else if (State==EState::Flee || State==EState::Hide) bGoLook=false;
  switch (State)
  {
   case EState::Flee:
    DesiredSpeed=2.25; DesiredCrouch=.3;
    if (bAtGoal) { Enter(EState::Hide); DesiredSpeed=0; DecisionAt=Age+Random.FRandRange(2.5f,4.5f); }
    else if (Path.IsEmpty() && Age>=RepathAt)
    { Choose(In,World,true,true); RepathAt=Age+(bPlanningDeferred?.05:.25); }
    break;
   case EState::Hide:
    DesiredCrouch=1.;
    if (Age>=DecisionAt && !bPlayerSeesMe) { Enter(EState::Idle); Noticed=0; }
    else if (bPlayerSeesMe && StateAge>1.2 && Age>=RepathAt)
    { Enter(EState::Flee); Choose(In,World,true,true); RepathAt=Age+(bPlanningDeferred?.05:.25); }
    break;
   case EState::Peek:
    DesiredCrouch=bTargetWindow?FMath::Clamp(1.-StateAge/1.1,0.,1.):.18;
    if (Age>=DecisionAt) { Enter(EState::Hide); DecisionAt=Age+2.5; }
    break;
   case EState::Stalk:
    DesiredSpeed=bHasPlayerMemory?.65:.9; DesiredCrouch=bTargetRoof?.58:.35;
    if (bAtGoal)
    {
     Enter((bTargetCover || bTargetWindow)?EState::Peek:EState::Listen);
     if (State==EState::Listen) DecisionAt=Age+Random.FRandRange(1.8f,3.2f);
    }
    else if (Path.IsEmpty() && Age>=RepathAt) { Enter(EState::Listen); DecisionAt=Age+1.; }
    break;
   case EState::Listen:
    DesiredCrouch=.22;
    if (Age>=DecisionAt) Enter(EState::Idle);
    break;
   default:
    if (Sound && HeardAge<.1 && StateAge>.7 && Age-LastListenAt>6.)
    { Enter(EState::Listen); DecisionAt=Age+Random.FRandRange(.9f,1.5f); }
    else if (Age>=DecisionAt)
    {
     if (Choose(In,World,false,bHasPlayerMemory)) Enter(EState::Stalk);
     else DecisionAt=Age+(bPlanningDeferred?.05:.25);
    }
    break;
  }
 }
 Crouch+=FMath::Clamp(DesiredCrouch-Crouch,-Dt*.9,Dt*.9);
 Move(Dt,World);
}

bool FS47RoswellBrain::SelfTest(TArray<FString>& Lines)
{
 bool bOk=true;
 const auto Check=[&](bool Pass,const TCHAR* Why)
 { Lines.Add(FString::Printf(TEXT("%s Roswell brain: %s"),Pass?TEXT("ok"):TEXT("FAIL"),Why)); bOk&=Pass; };
 FWorld World;
 World.CanStand=[](const FVector& P) { return Finite(P); };
 World.CanMove=[](const FVector&,const FVector&) { return true; };
 World.FindPath=[](const FVector&,const FVector& To) { return TArray<FVector>{To}; };
 World.Sight=[](const FVector&,const FVector&) { return true; };
 World.Points={{TEXT("west-cover"),FVector(-5,0,4),true,false,false},
               {TEXT("east-window"),FVector(5,0,4),true,false,true}};
 FInput In; In.PlayerFeet=FVector(0,0,8); In.PlayerEye=FVector(0,1.62,8); In.PlayerForward=FVector(0,0,-1);
 FS47RoswellBrain B; B.Start(EMode::Curious,FVector::ZeroVector,0,47);
 for (int32 I=0; I<12; ++I) B.Tick(.1,In,World);
 Check(B.bSeesPlayer && B.bHasPlayerMemory && B.LastKnownPlayer.Equals(In.PlayerFeet),TEXT("visible player creates a real last-seen memory"));
 In.bHeld=true; const double AgeBefore=B.Age, MemoryBefore=B.MemoryAge;
 const FVector PosBefore=B.Pos; B.Tick(10,In,World);
 Check(B.Age==AgeBefore && B.MemoryAge==MemoryBefore && B.Pos.Equals(PosBefore),TEXT("pause freezes movement, decisions and memory age"));
 In.bHeld=false; World.Sight=[](const FVector&,const FVector&) { return false; };
 const FVector KnownBefore=B.LastKnownPlayer; In.PlayerFeet=FVector(20,0,20); In.PlayerEye=In.PlayerFeet+FVector(0,1.62,0);
 B.Tick(.1,In,World);
 Check(!B.bSeesPlayer && B.LastKnownPlayer.Equals(KnownBefore),TEXT("unseen silent player cannot update last-seen position"));
 FS47RoswellBrain Listener; Listener.Start(EMode::Curious,FVector::ZeroVector,PI,17);
 In.PlayerFeet=FVector(0,0,3); In.PlayerEye=In.PlayerFeet+FVector(0,1.62,0); In.PlayerSpeed=0;
 Listener.Tick(.1,In,World);
 Check(!Listener.bHasPlayerMemory,TEXT("stationary player does not create imagined footsteps"));
 In.PlayerSpeed=1.3; Listener.Tick(.1,In,World);
 Check(Listener.bHasPlayerMemory && !Listener.bSeesPlayer,TEXT("nearby real movement can be heard muffled through cover"));
 {
  FS47RoswellBrain Windy; Windy.Start(EMode::Curious,FVector::ZeroVector,PI,17);
  FInput Gale=In; Gale.HearScale=.5;
  Windy.Tick(.1,Gale,World);
  Check(!Windy.bHasPlayerMemory,TEXT("a strong wind covers the same steps through cover (WEATHER-20261108)"));
 }
 In.PlayerSpeed=0; In.Noise=FNoise{FVector(0,0,40),1.}; Listener.Tick(.1,In,World);
 Check(Listener.LastKnownPlayer.Equals(FVector(0,0,3)),TEXT("inaudible distant source cannot retarget an individual"));
 In.Noise.Reset(); In.PlayerFeet=FVector(0,0,5); In.PlayerEye=In.PlayerFeet+FVector(0,1.62,0);
 World.Sight=[](const FVector&,const FVector&) { return true; };
 FS47RoswellBrain Shy; Shy.Start(EMode::Curious,FVector::ZeroVector,0,28);
 for (int32 I=0; I<24; ++I) Shy.Tick(.1,In,World);
 Check((Shy.State==EState::Flee || Shy.State==EState::Hide) && Shy.Mode==EMode::Curious && !Shy.bContact,
  TEXT("a watched ordinary Roswell flees or hides instead of becoming a hunter"));
 const FVector Remembered=Shy.LastKnownPlayer; Shy.SetMode(EMode::Hunt);
 World.Sight=[](const FVector&,const FVector&) { return false; }; Shy.Tick(.1,In,World);
 Check(Shy.bHasPlayerMemory && Shy.LastKnownPlayer.Equals(Remembered),TEXT("explicit hunt keeps a genuinely perceived recent target after the observer turns away"));
 World.Sight=[](const FVector&,const FVector&) { return true; };
 FS47RoswellBrain Hunter; Hunter.Start(EMode::Hunt,FVector::ZeroVector,0,8);
 In.PlayerFeet=FVector(0,0,8); In.PlayerEye=In.PlayerFeet+FVector(0,1.62,0);
 Hunter.Tick(.1,In,World); const FVector HuntGoal=Hunter.LastKnownPlayer;
 Check(Hunter.State==EState::Hunt && Hunter.Velocity.Size()>0,TEXT("explicit hunt mode pursues a visible target"));
 World.Sight=[](const FVector&,const FVector&) { return false; };
 In.PlayerFeet=FVector(12,0,12); In.PlayerEye=In.PlayerFeet+FVector(0,1.62,0); Hunter.Tick(.1,In,World);
 Check(Hunter.State==EState::Search && Hunter.LastKnownPlayer.Equals(HuntGoal),TEXT("lost sight searches the last perceived point, not the hidden player"));
 World.CanMove=[](const FVector&,const FVector&) { return false; };
 const FVector AtDoor=Hunter.Pos; Hunter.Tick(.1,In,World);
 Check(Hunter.Pos.Equals(AtDoor) && Hunter.Velocity.IsNearlyZero() && !Hunter.bAtGoal,
  TEXT("a newly closed obstacle stops an already planned movement without reaching its goal"));
 FS47RoswellBrain Stander; Stander.Start(EMode::Stand,FVector(2,0,2),0,1);
 for (int32 I=0; I<82; ++I) Stander.Tick(.1,In,World);
 Check(Stander.bStandDone && Stander.Pos.Equals(FVector(2,0,2)),TEXT("stand mode lasts eight seconds without teleporting or starting a chase"));
 World.CanMove=[](const FVector&,const FVector&) { return true; };
 World.FindPath=[](const FVector&,const FVector&) { return TArray<FVector>(); };
 FS47RoswellBrain Trapped; Trapped.Start(EMode::Hunt,FVector::ZeroVector,0,2);
 In.PlayerFeet=FVector(0,0,8); In.PlayerEye=In.PlayerFeet+FVector(0,1.62,0);
 World.Sight=[](const FVector&,const FVector&) { return true; };
 for (int32 I=0; I<20; ++I) Trapped.Tick(.1,In,World);
 Check(Trapped.Pos.IsNearlyZero() && Trapped.DistanceTravelled==0 && !Trapped.bContact,
  TEXT("an empty route never falls back to walking through a blocked world"));
 World.FindPath=[](const FVector&,const FVector& To) { return TArray<FVector>{To}; };
 FS47RoswellBrain Touch; Touch.Start(EMode::Hunt,FVector::ZeroVector,0,3);
 In.PlayerFeet=FVector(0,0,.7); In.PlayerEye=In.PlayerFeet+FVector(0,1.62,0); Touch.Tick(.1,In,World);
 Check(Touch.bContact && Touch.State==EState::Contact && Touch.Velocity.IsNearlyZero(),TEXT("developer contact requires visible nearby player and stops at the player"));
 Touch.Tick(.1,In,World);
 Check(!Touch.bContact,TEXT("contact cue is a cooldown-protected pulse, not a repeated frame event"));
 In.bCanContact=false;
 FS47RoswellBrain Driver;Driver.Start(EMode::Hunt,FVector::ZeroVector,0,6);Driver.Tick(.1,In,World);
 Check(Driver.bSeesPlayer&&!Driver.bContact&&Driver.State!=EState::Contact,
  TEXT("visible driver remains observable but cannot be grabbed through a cab"));
 In.bCanContact=true;
 World.CanMove=[](const FVector&,const FVector&){return false;};
 FS47RoswellBrain BehindGlass;BehindGlass.Start(EMode::Hunt,FVector::ZeroVector,0,5);BehindGlass.Tick(.1,In,World);
 Check(BehindGlass.bSeesPlayer&&!BehindGlass.bContact&&BehindGlass.State!=EState::Contact,
  TEXT("transparent sight alone cannot grab a nearby player through a physical barrier"));
 World.CanMove=[](const FVector&,const FVector&){return true;};
 FInput NoPlayer; NoPlayer.bPlayerValid=false;
 World.Occupancy=[](const FVector& P) { return P.X<0?1.:0.; };
 FS47RoswellBrain Reserved; Reserved.Start(EMode::Curious,FVector::ZeroVector,0,57);
 for (int32 I=0; I<40; ++I) Reserved.Tick(.1,NoPlayer,World);
 Check(Reserved.TargetId==TEXT("east-window") && Reserved.Pos.X>0,
  TEXT("an individual excludes another alien's reserved cover instead of stacking there"));
 World.Occupancy={}; World.Sight=[](const FVector&,const FVector&) { return false; };
 In.PlayerFeet=FVector(0,0,3); In.PlayerEye=In.PlayerFeet+FVector(0,1.62,0); In.PlayerSpeed=1.3;
 FS47RoswellBrain Lurker; Lurker.Start(EMode::Curious,FVector::ZeroVector,PI,19);
 for (int32 I=0; I<60; ++I) Lurker.Tick(.1,In,World);
 Check(Lurker.DistanceTravelled>.3 && Lurker.Mode==EMode::Curious && !Lurker.bContact,
  TEXT("continuous audible footsteps allow cautious stalking after listening, not endless listening"));
 World.Sight=[](const FVector&,const FVector&) { return true; };
 World.FindPath=[](const FVector&,const FVector& To) { return TArray<FVector>{FVector(3,0,0),To}; };
 World.CanMove=[](const FVector& From,const FVector& To)
  { return !(From.X<2.9 && To.Z>1.); };
 In.PlayerSpeed=0; In.PlayerFeet=FVector(0,0,8); In.PlayerEye=In.PlayerFeet+FVector(0,1.62,0);
 FS47RoswellBrain Detour; Detour.Start(EMode::Hunt,FVector::ZeroVector,0,6);
 for (int32 I=0; I<10; ++I) Detour.Tick(.1,In,World);
 Check(Detour.Pos.X>.5 && FMath::Abs(Detour.Pos.Z)<.035 && Detour.DistanceTravelled<3.,
  TEXT("a complete detour follows its first waypoint without cutting or teleporting to the player"));
 int32 Budget=0,Calls=0;
 World.CanMove=[](const FVector&,const FVector&) { return true; };
 World.Sight=[](const FVector&,const FVector&) { return false; };
 World.Points={{TEXT("already-here"),FVector::ZeroVector,true,false,false},
  {TEXT("blocked-cover"),FVector(-3,0,0),true,false,false},{TEXT("reachable"),FVector(3,0,0),false,false,false}};
 World.PlanningReady=[&] { return Budget>0; };
 World.FindPath=[&](const FVector&,const FVector& To)
 { --Budget; ++Calls; return To.X<0?TArray<FVector>():TArray<FVector>{To}; };
 FS47RoswellBrain Fair; Fair.Start(EMode::Curious,FVector::ZeroVector,0,91);
 Check(!Fair.Choose(NoPlayer,World,false,true) && Fair.bPlanningDeferred && Calls==0,
  TEXT("an exhausted shared budget defers planning without consuming a candidate"));
 Budget=1;
 Check(!Fair.Choose(NoPlayer,World,false,true) && !Fair.bPlanningDeferred && Calls==1,
  TEXT("one rejected cover consumes only one actual route attempt"));
 Budget=1;
 Check(Fair.Choose(NoPlayer,World,false,true) && Calls==2 && Fair.TargetId==TEXT("reachable") && !Fair.bAtGoal,
  TEXT("next granted turn explores reachable ground instead of retrying blocked cover or selecting itself"));
 World.PlanningReady={};
 World.FindPath=[](const FVector&,const FVector& To) { return TArray<FVector>{To}; };
 const auto Height=[](double X) { return X>.4?.12:0.; };
 World.CanStand=[&](const FVector& P) { return Finite(P) && FMath::Abs(P.Y-Height(P.X))<.001; };
 World.GroundedStep=[&](const FVector&,const FVector& Desired)->TOptional<FVector>
 { return FVector(Desired.X,Height(Desired.X),Desired.Z); };
 FS47RoswellBrain Ramp; Ramp.Start(EMode::Curious,FVector::ZeroVector,0,92);
 Ramp.Plan(FVector(2,.12,0),TEXT("raised-ground"),World); Ramp.DesiredSpeed=1.;
 for (int32 I=0; I<20; ++I) Ramp.Move(.05,World);
 Check(Ramp.Pos.X>.5 && FMath::Abs(Ramp.Pos.Y-.12)<.001 && Ramp.DistanceTravelled>.5,
  TEXT("short grounded steps follow actual raised terrain instead of an unsupported linear Y interpolation"));
 const FVector SupportedAt=Ramp.Pos;
 World.GroundedStep=[](const FVector&,const FVector&)->TOptional<FVector> { return {}; };
 Ramp.Move(.05,World);
 Check(Ramp.Pos.Equals(SupportedAt) && Ramp.Velocity.IsNearlyZero() && !Ramp.bAtGoal,
  TEXT("a rejected grounded step stops movement without snapping across unsupported ground"));
 {
  // Hold from a visit: stops, faces, eases back; no mode change, and letting go leaves it as it was.
  FWorld Open; Open.CanStand=[](const FVector& P){return Finite(P);};Open.CanMove=[](const FVector&,const FVector&){return true;};
  Open.FindPath=[](const FVector&,const FVector& To){return TArray<FVector>{To};};Open.Sight=[](const FVector&,const FVector&){return true;};
  FInput Lit; Lit.PlayerFeet=FVector(0,0,10); Lit.PlayerEye=FVector(0,1.62,10); Lit.PlayerForward=FVector(0,0,-1); Lit.bFlashlight=true;
  Lit.HoldLook=Lit.PlayerEye; Lit.HoldBack=.5;
  FS47RoswellBrain Held; Held.Start(EMode::Hunt,FVector::ZeroVector,0,4);
  for (int32 I=0; I<20; ++I) Held.Tick(.1,Lit,Open);
  Check(Held.Pos.Z<-.5 && Held.Pos.Z>-1.5 && Held.Mode==EMode::Hunt && !Held.bContact && Held.State==EState::Listen,
   TEXT("a hold stops even a hunter and eases it back from the lamp, slowly, without changing its mode"));
  FInput Noisy=Lit; Noisy.bFlashlight=false; Noisy.HoldBack=0; Noisy.HoldLook=FVector(9,0,0); Noisy.bHoldHeard=true;
  FS47RoswellBrain Turner; Turner.Start(EMode::Curious,FVector::ZeroVector,0,5);
  for (int32 I=0; I<10; ++I) Turner.Tick(.1,Noisy,Open);
  Check(Turner.Pos.IsNearlyZero() && Turner.LastKnownPlayer.Equals(FVector(9,0,0)) && Turner.Mode==EMode::Curious && FMath::Abs(Wrap(Turner.Yaw-Facing(FVector::ZeroVector,FVector(9,0,0))))<.2,
   TEXT("a heard sound turns it to that place and that is what it then knows"));
 }
 {
  // PHYSICS-20261108, the F10 lure, curious (the hunt's is in FS47Handling's test): a small one that has had the player in view,
  // the player low and looking away twelve metres off, a stone landing a few metres to its side and behind it.
  FWorld Yard; Yard.CanStand=[](const FVector& P){return Finite(P);};Yard.CanMove=[](const FVector&,const FVector&){return true;};
  Yard.FindPath=[](const FVector&,const FVector& To){return TArray<FVector>{To};};Yard.Sight=[](const FVector&,const FVector&){return true;};
  Yard.Points={{TEXT("west-cover"),FVector(-5,0,4),true,false,false}};
  FInput Low; Low.PlayerFeet=FVector(0,0,10); Low.PlayerEye=FVector(0,1.1,10); Low.PlayerForward=FVector(0,0,1); Low.PlayerQuiet=1;
  const FVector StoneAt(3,0,-1.5);
  // Watched first in the open, then the stone (loudness .8: a stone on dirt, heard to eight metres), then five seconds in After.
  const auto Lure=[&](FS47RoswellBrain& Small,double Loud,const FWorld& After,bool& bKnewPlayer,double& OutBefore)
  {
   for (int32 I=0; I<12; ++I) Small.Tick(.1,Low,Yard);
   bKnewPlayer=Small.bHasPlayerMemory && Small.LastKnownPlayer.Equals(Low.PlayerFeet);
   FInput Thrown=Low; Thrown.Noise=FNoise{StoneAt,Loud};
   Small.Tick(.1,Thrown,After);
   OutBefore=Distance(Small.Pos,StoneAt);
   for (int32 I=0; I<50; ++I) Small.Tick(.1,Low,After);
  };
  bool bKnew=false; double Before=0;
  // The player gone out of its sight as the stone lands: what it hears now wins over what it saw.
  FWorld Ducked=Yard; Ducked.Sight=[Hide=Low.PlayerEye](const FVector&,const FVector& To){return Distance(To,Hide)>1.;};
  FS47RoswellBrain Hidden; Hidden.Start(EMode::Curious,FVector::ZeroVector,0,61);
  Lure(Hidden,.8,Ducked,bKnew,Before);
  Check(bKnew && Hidden.Mode==EMode::Curious && Hidden.LastKnownPlayer.Equals(StoneAt) && Distance(Hidden.Pos,StoneAt)<Before-1. && Hidden.State!=EState::Flee,
   TEXT("curious: a stone heard with the player out of sight wins over the old sighting, and it goes slowly to look there"));
  // Still in view as it lands: it turns to the sound and goes to look, and once the player is behind it the stone is what it knows.
  FS47RoswellBrain Watching; Watching.Start(EMode::Curious,FVector::ZeroVector,0,61);
  Lure(Watching,.8,Yard,bKnew,Before);
  Check(bKnew && !Watching.bSeesPlayer && Watching.LastKnownPlayer.Equals(StoneAt) && Distance(Watching.Pos,StoneAt)<Before-1. && Distance(Watching.Pos,Low.PlayerFeet)>8. && Watching.Mode==EMode::Curious,
   TEXT("curious: a stone landing while it watches the player draws it off, and when it no longer sees the player it takes them to be there"));
  // Too faint to carry to where it stands: nothing changes.
  FS47RoswellBrain Far; Far.Start(EMode::Curious,FVector::ZeroVector,0,61);
  Lure(Far,.2,Yard,bKnew,Before);
  Check(bKnew && Far.NoiseAge>1e5 && Far.TargetId!=TEXT("heard-sound") && !Far.LastKnownPlayer.Equals(StoneAt),TEXT("curious: a stone too faint to reach it changes nothing"));
 }
 {
  // STONE-LURE-20261109: the F10 line "Draw a small one off with a stone" at its own distances (the run of 9 November, 03:23): the
  // player low, the stone thrown three metres to the small one's side and a metre and a half beyond it, first down loud (a stone on
  // dirt at 6.6 m/s: .8, heard to eight metres), a softer bounce 1.6 m on (.37), at rest 2.9 m on from where it first hit; the view
  // turned away as it is thrown, and looked at 3.6 s after it first hit. Where it first hit is by the walk zones' wall: no route
  // ends on that spot itself, one does half a metre off. One route a tick, as FS47RoswellGroup grants each of them.
  FVector Wall(1e6,0,1e6);
  bool bRouted=false;
  FWorld Lot; Lot.CanStand=[](const FVector& P){return Finite(P);}; Lot.CanMove=[](const FVector&,const FVector&){return true;};
  Lot.Sight=[](const FVector&,const FVector&){return true;};
  Lot.PlanningReady=[&bRouted]{return !bRouted;};
  Lot.FindPath=[&](const FVector&,const FVector& To){ bRouted=true; return Distance(To,Wall)<.5?TArray<FVector>():TArray<FVector>{To}; };
  const auto Turn=[&](FS47RoswellBrain& Small,const FInput& I){ bRouted=false; Small.Tick(.1,I,Lot); };
  FInput Low; Low.PlayerFeet=FVector::ZeroVector; Low.PlayerEye=FVector(0,1.1,0); Low.PlayerQuiet=1;
  struct FLureRun { FVector Was=FVector::ZeroVector, First=FVector::ZeroVector, Second=FVector::ZeroVector, Rest=FVector::ZeroVector; bool bBolted=false; };
  // Before seconds looking where Look points, .4 s aimed at where the stone will come down, thrown (and the view turned away,
  // unless bStare), .9 s in the air, the two hits, then on to F10's look.
  const auto Run=[&](FS47RoswellBrain& Small,double Before,const FVector& Look,bool bStare)
  {
   FLureRun R;
   FInput Pre=Low; Pre.PlayerForward=Look.GetSafeNormal();
   for (int32 I=0; I<FMath::RoundToInt(Before*10.); ++I) Turn(Small,Pre);
   const FVector ToBody=FVector(Small.Pos.X,0,Small.Pos.Z).GetSafeNormal(), Side(-ToBody.Z,0,ToBody.X);
   R.First=Small.Pos+Side*3.+ToBody*1.5; R.First.Y=0;
   const FVector On=FVector(R.First.X,0,R.First.Z).GetSafeNormal();
   R.Second=R.First+On*1.6; R.Rest=R.First+On*2.9; Wall=R.First;
   FInput Aim=Low; Aim.PlayerForward=(R.First+FVector(0,.1,0)-Low.PlayerEye).GetSafeNormal();
   for (int32 I=0; I<4; ++I) Turn(Small,Aim);
   R.Was=Small.Pos;
   FInput After=bStare?Pre:Aim; if (!bStare) After.PlayerForward=FVector(-Aim.PlayerForward.X,Aim.PlayerForward.Y,-Aim.PlayerForward.Z);
   for (int32 I=0; I<9; ++I) { Turn(Small,After); R.bBolted|=Small.State==EState::Flee || Small.State==EState::Hide; }
   FInput Hit=After; Hit.Noise=FNoise{R.First,.8};
   for (int32 I=0; I<4; ++I) Turn(Small,Hit);
   Hit.Noise=FNoise{R.Second,.37};
   for (int32 I=0; I<3; ++I) Turn(Small,Hit);
   for (int32 I=0; I<29; ++I) Turn(Small,After);
   return R;
  };
  // As F10 has it now: the player gone back to seven metres from it, low and looking away, then turning only to aim.
  FS47RoswellBrain Drawn; Drawn.Start(EMode::Curious,FVector(0,0,-7),0,47);
  const FLureRun D=Run(Drawn,1.5,FVector(0,0,1),false);
  const double Belief=FMath::Min(Distance(Drawn.LastKnownPlayer,D.First),Distance(Drawn.LastKnownPlayer,D.Rest));
  Check(!D.bBolted && Drawn.Mode==EMode::Curious && Drawn.TargetId==TEXT("heard-sound") && Belief<2. && Distance(Drawn.Pos,D.First)<Distance(D.Was,D.First)-.5 &&
   Drawn.State!=EState::Flee && Drawn.State!=EState::Hide,
   TEXT("F10 lure at its distances: seven metres off and looking away, a stone by a wall it cannot stand at draws it to a step short of there, and it takes you to be there"));
  // As F10 ran it: come up 2.8 m in front of the view and looked at for 2.5 s. It bolts first, and a stone does not draw it back.
  FS47RoswellBrain Near; Near.Start(EMode::Curious,FVector(0,0,-2.8),0,47);
  const FLureRun N=Run(Near,2.5,FVector(0,0,-1),false);
  Check(N.bBolted && Near.TargetId!=TEXT("heard-sound") && Near.Mode==EMode::Curious && !Near.bContact,
   TEXT("F10 lure as it ran: one stood over and watched from 2.8 m bolts, and the stone after does not draw it"));
  // Standing in front of it five metres off and looking at it all along: no lure works on it.
  FS47RoswellBrain Stared; Stared.Start(EMode::Curious,FVector(0,0,-5),0,47);
  const FLureRun S=Run(Stared,1.5,FVector(0,0,-1),true);
  Check(S.bBolted && Stared.TargetId!=TEXT("heard-sound") && Stared.Mode==EMode::Curious && !Stared.bContact,
   TEXT("a stone thrown while the player stands in front of it and watches it draws nothing: it bolts"));
 }
 return bOk;
}


// Freeform owns these facts; Story/dev groups retain their existing ephemeral life.
S47J::FObj FS47RoswellBrain::Save() const
{
 auto O=S47J::New(); S47J::Set(O,TEXT("v"),1);
 S47J::Set(O,TEXT("mode"),int32(Mode)); S47J::Set(O,TEXT("state"),int32(State));
 S47J::Set(O,TEXT("random"),Random.GetCurrentSeed());
 auto V=[&](const TCHAR* K,const FVector& P){S47J::Set(O,K,TArray<double>{P.X,P.Y,P.Z});};
 V(TEXT("pos"),Pos); V(TEXT("velocity"),Velocity); V(TEXT("lastKnown"),LastKnownPlayer); V(TEXT("look"),LookTarget); V(TEXT("goal"),Goal); V(TEXT("lastEye"),LastPlayerEye);
 S47J::Set(O,TEXT("values"),TArray<double>{Yaw,HeadYaw,Crouch,DistanceTravelled,Age,StateAge,MemoryAge,Noticed,DecisionAt,RepathAt,ContactCool,HeardAge,LastListenAt,DesiredCrouch,DesiredSpeed});
 S47J::Set(O,TEXT("memory"),bHasPlayerMemory); S47J::Set(O,TEXT("contact"),bContact); S47J::Set(O,TEXT("standDone"),bStandDone); S47J::Set(O,TEXT("atGoal"),bAtGoal);
 S47J::Set(O,TEXT("targetCover"),bTargetCover); S47J::Set(O,TEXT("targetRoof"),bTargetRoof); S47J::Set(O,TEXT("targetWindow"),bTargetWindow);
 S47J::Set(O,TEXT("target"),TargetId); S47J::Set(O,TEXT("next"),NextPoint); S47J::Set(O,TEXT("choice"),ChoiceCursor);
 TArray<double> R; for(const auto& P:Path)R.Append({P.X,P.Y,P.Z}); S47J::Set(O,TEXT("route"),R);return O;
}
bool FS47RoswellBrain::Restore(const S47J::FObj& O)
{
 if(!O || S47J::Num(O,TEXT("v"))!=1)return false;
 const auto Values=S47J::Nums(O,TEXT("values")); const auto R=S47J::Nums(O,TEXT("route"));
 const int32 Mo=S47J::Int(O,TEXT("mode"),-1),St=S47J::Int(O,TEXT("state"),-1);
 if(Values.Num()!=15 || R.Num()%3 || R.Num()>3072 || Mo<0 || Mo>2 || St<0 || St>8)return false;
 for(double X:Values)if(!FMath::IsFinite(X))return false;for(double X:R)if(!FMath::IsFinite(X))return false;
 auto V=[&](const TCHAR* K,FVector& P){const auto A=S47J::Nums(O,K);if(A.Num()!=3)return false;for(double X:A)if(!FMath::IsFinite(X)||FMath::Abs(X)>1e6)return false;P=FVector(A[0],A[1],A[2]);return true;};
 if(!V(TEXT("pos"),Pos)||!V(TEXT("velocity"),Velocity)||!V(TEXT("lastKnown"),LastKnownPlayer)||!V(TEXT("look"),LookTarget)||!V(TEXT("goal"),Goal)||!V(TEXT("lastEye"),LastPlayerEye))return false;
 Mode=EMode(Mo); State=EState(St);Random.Initialize(S47J::Int(O,TEXT("random"),47));
 double* Targets[]={&Yaw,&HeadYaw,&Crouch,&DistanceTravelled,&Age,&StateAge,&MemoryAge,&Noticed,&DecisionAt,&RepathAt,&ContactCool,&HeardAge,&LastListenAt,&DesiredCrouch,&DesiredSpeed};
 for(int32 I=0;I<15;++I)*Targets[I]=Values[I];
 bHasPlayerMemory=S47J::Bool(O,TEXT("memory"));bContact=S47J::Bool(O,TEXT("contact"));bStandDone=S47J::Bool(O,TEXT("standDone"));bAtGoal=S47J::Bool(O,TEXT("atGoal"));
 bTargetCover=S47J::Bool(O,TEXT("targetCover"));bTargetRoof=S47J::Bool(O,TEXT("targetRoof"));bTargetWindow=S47J::Bool(O,TEXT("targetWindow"));
 TargetId=S47J::Str(O,TEXT("target"));Path.Reset();for(int32 I=0;I<R.Num();I+=3)Path.Add(FVector(R[I],R[I+1],R[I+2]));NextPoint=FMath::Clamp(S47J::Int(O,TEXT("next")),0,Path.Num());ChoiceCursor=S47J::Int(O,TEXT("choice"));
 bPlanningDeferred=false;bPlayerSeesMe=bSeesPlayer=false;NoiseAge=1e6;bLurePending=bGoLook=false;LookTry=0;return true;
}
