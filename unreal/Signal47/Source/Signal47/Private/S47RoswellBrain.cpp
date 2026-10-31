#include "S47RoswellBrain.h"

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
 Path.Reset(); NextPoint=0; TargetId.Reset(); Mode=InMode; State=EState::Idle;
 DecisionAt=Random.FRandRange(1.0f,2.3f); RepathAt=0;
}

void FS47RoswellBrain::SetMode(EMode InMode)
{
 if (Mode==InMode) return;
 const bool KeepMemory=InMode==EMode::Hunt && bHasPlayerMemory && MemoryAge<12.;
 Mode=InMode; Path.Reset(); NextPoint=0; bAtGoal=true; Velocity=FVector::ZeroVector;
 bContact=bStandDone=false; bHasPlayerMemory=KeepMemory; if (!KeepMemory) MemoryAge=0; Noticed=0;
 bTargetCover=bTargetRoof=bTargetWindow=false; TargetId.Reset();
 LastListenAt=Age-10.; HeardAge=1e6;
 State=EState::Idle; StateAge=0; DecisionAt=Age+1.; RepathAt=Age;
}

void FS47RoswellBrain::Enter(EState NewState)
{
 if (State==NewState) return;
 State=NewState; StateAge=0;
 if (NewState==EState::Idle) DecisionAt=Age+Random.FRandRange(1.4f,3.2f);
 if (NewState==EState::Listen) LastListenAt=Age;
 if (NewState==EState::Peek) DecisionAt=Age+Random.FRandRange(2.8f,3.6f);
}

bool FS47RoswellBrain::Seen(const FInput& In,const FWorld& World) const
{
 if (!In.bPlayerValid || !Finite(In.PlayerFeet) || !Finite(In.PlayerEye) || !World.Sight) return false;
 const FVector D=In.PlayerEye-Eye();
 if (D.SizeSquared()>24.*24.) return false;
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
 TOptional<FNoise> Source=In.Noise;
 double Reach=0;
 if (Source)
 {
  if (!Finite(Source->At) || !FMath::IsFinite(Source->Loudness) || Source->Loudness<=0) return {};
  Reach=FMath::Clamp(Source->Loudness,0.,2.)*10.;
 }
 else if (In.bPlayerValid && Finite(In.PlayerFeet) && In.PlayerSpeed>.35 && In.PlayerSpeed<12.)
 {
  Source=FNoise{In.PlayerFeet,1.}; Reach=In.PlayerSpeed>2.5?18.:9.;
 }
 if (!Source) return {};
 // A roof/wall muffles sound instead of granting a perfect through-wall view.
 // Stationary input cannot generate footsteps; the adapter supplies measured speed.
 if (!World.Sight || !World.Sight(Eye(),Source->At+FVector(0,.3,0))) Reach*=.42;
 if (FVector::Dist(Eye(),Source->At)>Reach || FMath::Abs(Source->At.Y-Pos.Y)>6.) return {};
 return Source->At;
}

bool FS47RoswellBrain::Plan(const FVector& To,const FString& Id,const FWorld& World)
{
 if (!Finite(To) || !World.CanStand || !World.CanMove || !World.CanStand(To)) return false;
 if (Distance(Pos,To)<.08 && FMath::Abs(Pos.Y-To.Y)<.15)
 { Goal=To; TargetId=Id; Path.Reset(); NextPoint=0; bAtGoal=true; return true; }
 bAtGoal=false;
 TArray<FVector> Proposed;
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
  if (!Finite(P.Pos) || D>28. || FMath::Abs(P.Pos.Y-Pos.Y)>.65 || !World.CanStand || !World.CanStand(P.Pos)) continue;
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
 for (int32 I=0; I<FMath::Min(Candidates.Num(),10); ++I)
 {
  const FPoint& P=Candidates[I].Point;
  if (!Plan(P.Pos,P.Id,World)) continue;
  bTargetCover=P.bCover; bTargetRoof=P.bRoof; bTargetWindow=P.bWindow;
  return true;
 }
 // If authored cover is unreachable, try bounded real ground directions away
 // from the last perceived player. Every fallback still needs a complete route.
 if (bFlee)
 {
  const double Away=Facing(Known,Pos);
  for (double Turn:{0.,.65,-.65,1.2,-1.2,2.0,-2.0})
  {
   const FVector P=Pos+Forward(Away+Turn)*5.;
   if (World.Occupancy && World.Occupancy(P)>=1.) continue;
   if (Plan(P,TEXT("escape"),World))
   { bTargetCover=bTargetRoof=bTargetWindow=false; return true; }
  }
 }
 return false;
}

void FS47RoswellBrain::Move(double Dt,const FWorld& World)
{
 const double OldSpeed=Velocity.Size(); Velocity=FVector::ZeroVector;
 if (DesiredSpeed<=0 || NextPoint>=Path.Num()) return;
 while (NextPoint<Path.Num() && FVector::Dist(Pos,Path[NextPoint])<.035) ++NextPoint;
 if (NextPoint>=Path.Num()) { bAtGoal=true; Path.Reset(); NextPoint=0; return; }
 const FVector D=Path[NextPoint]-Pos;
 const double Speed=FMath::Min(DesiredSpeed,OldSpeed+Dt*3.8);
 const FVector To=Pos+D.GetSafeNormal()*FMath::Min(D.Size(),Speed*Dt);
 if (!World.CanMove || !World.CanStand || !World.CanStand(To) || !World.CanMove(Pos,To) ||
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
 bContact=false;
 if (In.bHeld || !FMath::IsFinite(Dt) || Dt<=0) return;
 // Bounded physical steps prevent long frames from jumping through a doorway.
 double Left=FMath::Min(Dt,.25);
 while (Left>1e-8) { const double StepDt=FMath::Min(Left,.05); Step(StepDt,In,World); Left-=StepDt; }
}

void FS47RoswellBrain::Step(double Dt,const FInput& In,const FWorld& World)
{
 Age+=Dt; StateAge+=Dt; MemoryAge+=Dt; HeardAge+=Dt; ContactCool=FMath::Max(0.,ContactCool-Dt);
 bSeesPlayer=Seen(In,World); bPlayerSeesMe=Watched(In,World);
 const auto Sound=Heard(In,World);
 if (bSeesPlayer)
 {
  LastKnownPlayer=In.PlayerFeet; LastPlayerEye=In.PlayerEye;
  bHasPlayerMemory=true; MemoryAge=0;
 }
 else if (Sound)
 {
  LastKnownPlayer=*Sound; LastPlayerEye=*Sound+FVector(0,1.5,0);
  bHasPlayerMemory=true; MemoryAge=0; HeardAge=0;
 }
 if (MemoryAge>(Mode==EMode::Hunt?16.:12.)) bHasPlayerMemory=false;
 if (bHasPlayerMemory) LookTarget=LastPlayerEye;
 else LookTarget=Eye()+Forward(Yaw+.35*FMath::Sin(Age*.63))*4.;
 HeadYaw=Facing(Eye(),LookTarget);

 DesiredSpeed=DesiredCrouch=0;
 if (Mode==EMode::Stand)
 {
  Velocity=FVector::ZeroVector; Path.Reset(); NextPoint=0; bAtGoal=true;
  bStandDone=StateAge>=8.; Crouch=FMath::Max(0.,Crouch-Dt*2.);
  return;
 }
 bStandDone=false;
 // Chase is a developer mode, never an escalation of ordinary curiosity.
 if (Mode==EMode::Hunt)
 {
  const bool Contact=bSeesPlayer && Distance(Pos,In.PlayerFeet)<.92 &&
   FMath::Abs(Pos.Y-In.PlayerFeet.Y)<.65 && World.Sight && World.Sight(Eye(),In.PlayerEye);
  if (Contact)
  {
   Enter(EState::Contact); Path.Reset(); NextPoint=0; bAtGoal=true;
   if (ContactCool<=0) { bContact=true; ContactCool=4.; }
  }
  else if (bHasPlayerMemory)
  {
   Enter(bSeesPlayer && (!Path.IsEmpty() || Age>=RepathAt)?EState::Hunt:EState::Search);
   DesiredSpeed=bSeesPlayer?2.8:1.15; DesiredCrouch=bSeesPlayer?.08:.24;
   if (Age>=RepathAt && (Path.IsEmpty() || FVector::Dist(Goal,LastKnownPlayer)>.55))
   {
    const bool Planned=Plan(LastKnownPlayer,TEXT("last-perceived-player"),World);
    RepathAt=Age+(Planned?.5:1.2);
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
    else { Enter(EState::Listen); DecisionAt=Age+2.; }
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
   Choose(In,World,true,true); RepathAt=Age+1.3;
  }
  switch (State)
  {
   case EState::Flee:
    DesiredSpeed=2.25; DesiredCrouch=.3;
    if (bAtGoal) { Enter(EState::Hide); DesiredSpeed=0; DecisionAt=Age+Random.FRandRange(2.5f,4.5f); }
    else if (Path.IsEmpty() && Age>=RepathAt)
    { Choose(In,World,true,true); RepathAt=Age+1.3; }
    break;
   case EState::Hide:
    DesiredCrouch=1.;
    if (Age>=DecisionAt && !bPlayerSeesMe) { Enter(EState::Idle); Noticed=0; }
    else if (bPlayerSeesMe && StateAge>1.2 && Age>=RepathAt)
    { Enter(EState::Flee); Choose(In,World,true,true); RepathAt=Age+1.3; }
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
     else DecisionAt=Age+Random.FRandRange(1.4f,2.8f);
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
 return bOk;
}
