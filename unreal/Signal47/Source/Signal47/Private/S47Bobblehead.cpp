#include "S47Bobblehead.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Bobblehead,Log,All);

// D45-20261110 (Tom, D45-29): see S47Bobblehead.h. The numbers below are a real dashboard bobblehead's: the head
// rings at about 2.4 times a second and keeps wobbling for a second or two; braking hard throws it a good
// 10 to 15 degrees towards the windshield; the road's knocks make it nod. It is read from the truck's pose (the
// drive's springs already carry the bumps, the squat, the dive and the lean), so nothing new is simulated.
namespace S47Bobble
{
 constexpr double Gravity=9.81;
 // the head on its spring: stiffness (2.4 Hz) and damping (it decays by e in about 0.7 s)
 constexpr double Stiff=227.,Damp=2.8;
 // tilt for each m/s^2 across the head; its nod for each m/s^2 up or down (the face is the heavy side)
 constexpr double Sway=.04,NodPerLift=.012;
 // the collar it hits, radians; what an acceleration may count at most (a hard knock, not a cut)
 constexpr double MaxTilt=.42,MaxAcc=40.;
 // the acceleration's smoothing (s); a frame this long or a jump this far is a cut or a hitch, not a shove
 constexpr double Smooth=.04,Hitch=.25,Jump=4.;
 // On the soft upper pad of the Jeep's dash (Tools/tripofleet/interior.py: its top at 1.47 m, from 0.30 to 0.09 m
 // in front of the cowl's edge; the compass stands on it at x -0.28): left of the middle, clear of the compass, the
 // steering wheel and the speedometer, a little below the driver's line to the road.
 const FVector DashAt(-.10,1.470,-.17);
 // The head's pivot on the body (the spring between the two neck bones), metres in the figure's frame: the same
 // numbers as Tools/alienbubblehead/prepare.py (EXPECT_PIVOT) and import_assets.py (PIVOT).
 const FVector HeadPivot(0,.0436,-.0017);
 const TCHAR* const Folder=TEXT("/Game/Licensed/Models/alienbubblehead/StaticMeshes/");

 // The damped spring's exact step (as S47Drive's body springs): stable through a hitch, the same at any frame rate.
 void Exact(double& X,double& V,double Target,double H)
 {
  const double D=Damp/2,W=FMath::Sqrt(Stiff-D*D),Decay=FMath::Exp(-D*H);
  const double Cos=FMath::Cos(W*H),SinOverW=FMath::Sin(W*H)/W,Off=X-Target,V0=V;
  X=Target+Decay*(Off*Cos+(V0+D*Off)*SinOverW);
  V=Decay*(V0*Cos-(D*V0+Stiff*Off)*SinOverW);
  // the collar: it stops there and comes back softly
  if(FMath::Abs(X)>MaxTilt){X=FMath::Sign(X)*MaxTilt;if(V*X>0)V=-.3*V;}
 }
 UStaticMesh* Mesh(const TCHAR* Name)
 {
  const FString Package=FString(Folder)+Name;
  if(!FPackageName::DoesPackageExist(Package))return nullptr;
  UStaticMesh* M=LoadObject<UStaticMesh>(nullptr,*(Package+TEXT(".")+Name));
  if(!M||M->GetStaticMaterials().IsEmpty())return nullptr;
  for(const FStaticMaterial& Slot:M->GetStaticMaterials())if(!Slot.MaterialInterface)return nullptr;
  return M;
 }
 UStaticMeshComponent* Part(AS47Web* W,UStaticMesh* M,USceneComponent* Parent,const FVector& P,const FQuat& Q)
 {
  auto* C=NewObject<UStaticMeshComponent>(W);C->SetStaticMesh(M);C->SetupAttachment(Parent);
  C->SetMobility(EComponentMobility::Movable);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  // (imported without Nanite; kept so whatever the asset says: its glTF material is not marked for Nanite, D44)
  C->SetForceDisableNanite(true);C->SetCastShadow(true);C->RegisterComponent();
  double E[16];S47Three::Compose(P,Q,FVector::OneVector,E);C->SetRelativeTransform(S47Three::MatToUE(E));
  return C;
 }
}

void FS47BobbleSpring::Step(double Dt,const FVector& Force)
{
 if(Dt<=0)return;
 const double H=FMath::Min(Dt,.1),Most=S47Bobble::MaxTilt;
 // a push towards the face tips the top that way; a push to +X leans it to +X (Lean is towards -X);
 // a lift (more than gravity pressing down) nods the heavy face down
 const double NodTo=FMath::Clamp(S47Bobble::Sway*Force.Z+S47Bobble::NodPerLift*(-Force.Y-S47Bobble::Gravity),-Most,Most);
 const double LeanTo=FMath::Clamp(-S47Bobble::Sway*Force.X,-Most,Most);
 S47Bobble::Exact(Nod,NodV,NodTo,H);
 S47Bobble::Exact(Lean,LeanV,LeanTo,H);
 Peak=FMath::Max(Peak,FMath::Max(FMath::Abs(Nod),FMath::Abs(Lean)));
}

void FS47BobbleSpring::Follow(double Dt,const FVector& Base,const FQuat& Figure)
{
 if(Dt<=0){Forget();return;}
 if(bLast&&(Dt>S47Bobble::Hitch||FVector::Dist(Base,Last)>S47Bobble::Jump))Forget();
 FVector A=FVector::ZeroVector;
 if(bLast)
 {
  const FVector V=(Base-Last)/Dt;
  if(bLastV)A=((V-LastV)/Dt).GetClampedToMaxSize(S47Bobble::MaxAcc);
  LastV=V;bLastV=true;
 }
 Last=Base;bLast=true;
 Acc+=(A-Acc)*(1-FMath::Exp(-Dt/S47Bobble::Smooth));
 // what the head feels: gravity less the base's acceleration, in the figure's own frame
 Step(Dt,Figure.UnrotateVector(FVector(0,-S47Bobble::Gravity,0)-Acc));
}

FQuat FS47BobbleSpring::Tilt() const
{
 return FQuat(FVector(1,0,0),Nod)*FQuat(FVector(0,0,1),Lean);
}

void FS47Bobblehead::Place(const FVector& Eye,FVector& OutAt,double& OutHeading)
{
 OutAt=S47Bobble::DashAt;
 // its face (+Z) towards the driver's eye, about Y only: it stands upright on the pad
 OutHeading=FMath::Atan2(Eye.X-OutAt.X,Eye.Z-OutAt.Z);
}

TUniquePtr<FS47Bobblehead> FS47Bobblehead::Install(AS47Web* W,const FS47Obj& Body,const FString& Path,const FVector& Eye)
{
 if(!W||!Body)return {};
 UStaticMesh* BodyAsset=S47Bobble::Mesh(TEXT("alienbubblehead_body"));
 UStaticMesh* HeadAsset=S47Bobble::Mesh(TEXT("alienbubblehead_head"));
 if(!BodyAsset||!HeadAsset)
 {
  UE_LOG(LogS47Bobblehead,Display,TEXT("S47: dashboard bobblehead not imported (Tools/alienbubblehead/import_assets.sh): %s as it was"),*Path);
  return {};
 }
 const FS47WebNode* N=W->Node(Body.I);if(!N||!N->Comp.IsValid())return {};
 auto B=MakeUnique<FS47Bobblehead>();
 Place(Eye,B->At,B->Heading);
 const FQuat Turn=S47Three::Euler(0,B->Heading,0);
 UStaticMeshComponent* FigureC=S47Bobble::Part(W,BodyAsset,N->Comp.Get(),B->At,Turn);
 const int32 FigureNode=W->AdoptNode(FigureC,Body.I,TEXT("bobblehead:")+Path+TEXT(":figure"));
 if(FigureNode<0){FigureC->DestroyComponent();return {};}
 // (AdoptNode reads the place only: the node's own frame set as it is drawn, so the head below it is placed right)
 B->Figure=W->ObjAt(FigureNode);B->Figure.SetPosition(B->At);B->Figure.SetQuaternion(Turn);
 UStaticMeshComponent* HeadC=S47Bobble::Part(W,HeadAsset,FigureC,S47Bobble::HeadPivot,FQuat::Identity);
 const int32 HeadNode=W->AdoptNode(HeadC,FigureNode,TEXT("bobblehead:")+Path+TEXT(":head"));
 if(HeadNode<0){HeadC->DestroyComponent();FigureC->DestroyComponent();B->Figure.SetVisible(false);return {};}
 B->Head=W->ObjAt(HeadNode);B->Head.SetPosition(S47Bobble::HeadPivot);B->Head.SetQuaternion(FQuat::Identity);
 B->FigureMesh=FigureC;B->HeadMesh=HeadC;
 UE_LOG(LogS47Bobblehead,Display,TEXT("S47: dashboard bobblehead on %s at (%.2f, %.3f, %.2f), turned %.0f degrees to the driver; head on its spring"),*Path,B->At.X,B->At.Y,B->At.Z,FMath::RadiansToDegrees(B->Heading));
 return B;
}

void FS47Bobblehead::Update(double Dt,const FVector& TruckPos,const FQuat& TruckQ)
{
 if(Dt<=0){Spring.Forget();return;}
 const FQuat Turn=TruckQ*S47Three::Euler(0,Heading,0);
 Spring.Follow(Dt,TruckQ.RotateVector(At)+TruckPos,Turn);
 Head.SetQuaternion(Spring.Tilt());
}

void FS47Bobblehead::Adopt(const FS47BobbleSpring& From)
{
 Spring=From;Spring.Forget();
 Head.SetQuaternion(Spring.Tilt());
}

void FS47Bobblehead::Settle()
{
 Spring.Settle();
 Head.SetQuaternion(FQuat::Identity);
}

bool FS47Bobblehead::Audit(TArray<FString>& Lines,bool bDrivingNow,bool bShownWanted) const
{
 bool Ok=true;auto Check=[&](bool bPass,const TCHAR* What){Ok&=bPass;Lines.Add(FString(bPass?TEXT("PASS truck bobblehead: "):TEXT("FAIL truck bobblehead: "))+What);};
 const UStaticMeshComponent* FigureC=FigureMesh.Get();const UStaticMeshComponent* HeadC=HeadMesh.Get();
 Check(FigureC&&HeadC&&Figure&&Head,TEXT("body and head installed as nodes of the truck"));
 if(!FigureC||!HeadC)return Ok;
 Check(FigureC->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&HeadC->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("no collision"));
 Check(HeadC->GetAttachParent()==FigureC&&Head.Position().Equals(S47Bobble::HeadPivot,1e-4),TEXT("the head sits on the body's neck pivot"));
 Check(Figure.Position().Equals(S47Bobble::DashAt,1e-4)&&FMath::Abs(Figure.Position().Y-1.47)<.002,TEXT("stands on the dash pad, left of the middle"));
 Check(Figure.Visible()==bShownWanted&&(!bShownWanted||Figure.Shown()==Figure.Parent().Shown()),bShownWanted?TEXT("shown with the truck's body (Freeform)"):TEXT("hidden outside Freeform (Story's cab as it was)"));
 if(bDrivingNow)Check(Spring.Peak>.002,TEXT("its head has moved while driving"));
 return Ok;
}

FString FS47Bobblehead::Describe() const
{
 return FString::Printf(TEXT("Bobblehead on the dash at (%.2f, %.2f, %.2f) m in the cab, turned %.0f deg to the driver; head nod %.1f, lean %.1f deg, most %.1f deg since parked; %s."),
  At.X,At.Y,At.Z,FMath::RadiansToDegrees(Heading),FMath::RadiansToDegrees(Spring.Nod),FMath::RadiansToDegrees(Spring.Lean),FMath::RadiansToDegrees(Spring.Peak),Figure.Shown()?TEXT("shown"):TEXT("hidden"));
}

// D45-20261110: the spring without a world (-S47SelfTest): still when still, the right way under braking, a turn and
// a knock, it rings and settles, the same at any frame rate, no kick from a cut, and it faces the driver on the pad.
bool FS47BobbleSpring::SelfTest(TArray<FString>& Lines)
{
 bool bOk=true;
 auto Check=[&](bool bPass,const FString& What){Lines.Add(FString(bPass?TEXT("ok   bobblehead: "):TEXT("FAIL bobblehead: "))+What);bOk&=bPass;};
 // the top of the head in the world, for a figure turned FigureQ
 auto TopOf=[](const FS47BobbleSpring& S,const FQuat& FigureQ){return FigureQ.RotateVector(S.Tilt().RotateVector(FVector(0,1,0)));};
 // a drive: Pos(t) gives the base's place; returns the spring after Seconds at Fps, with the extremes seen
 struct FRun{FS47BobbleSpring S;double MinNod=0,MaxNod=0,MinLean=0,MaxLean=0;int32 NodSigns=0;};
 auto Drive=[](TFunctionRef<FVector(double)> Pos,const FQuat& FigureQ,double Seconds,double Fps)
 {
  FRun R;const double Dt=1./Fps;double LastSign=0;
  for(double T=0;T<Seconds;T+=Dt)
  {
   R.S.Follow(Dt,Pos(T),FigureQ);
   R.MinNod=FMath::Min(R.MinNod,R.S.Nod);R.MaxNod=FMath::Max(R.MaxNod,R.S.Nod);
   R.MinLean=FMath::Min(R.MinLean,R.S.Lean);R.MaxLean=FMath::Max(R.MaxLean,R.S.Lean);
   const double Sign=FMath::Abs(R.S.Nod)>.004?FMath::Sign(R.S.Nod):0;
   if(Sign!=0&&Sign!=LastSign){if(LastSign!=0)++R.NodSigns;LastSign=Sign;}
  }
  return R;
 };
 const FQuat Facing=FQuat::Identity;   // the figure faces +Z: the truck's back, the driver
 {
  const FRun Still=Drive([](double){return FVector(3,1.5,-7);},Facing,5,60);
  Check(FMath::Abs(Still.S.Nod)<1e-6&&FMath::Abs(Still.S.Lean)<1e-6&&Still.S.Peak<1e-6,TEXT("still on level ground: upright"));
 }
 // braking: the truck goes forward (-Z) at 12 m/s and slows at 4 m/s^2 to a stop, then stands
 auto Brake=[](double T){const double Tb=FMath::Min(T,3.);return FVector(0,1.47,-(12*Tb-2*Tb*Tb));};
 {
  const FRun B=Drive(Brake,Facing,3,60);
  Check(B.MinNod<-.12&&B.MinNod>-S47Bobble::MaxTilt-1e-9,FString::Printf(TEXT("braking 4 m/s^2 throws the head towards the windshield (%.1f deg)"),FMath::RadiansToDegrees(B.MinNod)));
  Check(TopOf(B.S,Facing).Z<0,TEXT("under braking the top of the head is ahead of the base (-Z)"));
  const FRun After=Drive(Brake,Facing,8,60);
  Check(After.NodSigns>=2,FString::Printf(TEXT("after the stop it wobbles back and forth (%d swings)"),After.NodSigns));
  Check(FMath::Abs(After.S.Nod)<.01&&FMath::Abs(After.S.NodV)<.05,TEXT("and settles within five seconds"));
  const FRun Slow=Drive(Brake,Facing,3,30),Fast=Drive(Brake,Facing,3,144);
  Check(FMath::Abs(Slow.MinNod-Fast.MinNod)<.15*FMath::Abs(Fast.MinNod),FString::Printf(TEXT("the same at 30 and 144 frames a second (%.1f, %.1f deg)"),FMath::RadiansToDegrees(Slow.MinNod),FMath::RadiansToDegrees(Fast.MinNod)));
 }
 // a left turn: 10 m/s on a 25 m circle about (-25, 0): the pull is to the left (-X), the head leans out (+X)
 {
  auto Turn=[](double T){const double A=T*10/25;return FVector(-25+25*FMath::Cos(A),1.47,-25*FMath::Sin(A));};
  FS47BobbleSpring S;const double Dt=1./60;
  for(double T=0;T<2;T+=Dt)
  {
   const double A=T*10/25;
   S.Follow(Dt,Turn(T),FQuat(FVector(0,1,0),A));   // the truck turns with its path, the figure with it
  }
  Check(S.Lean<-.05,FString::Printf(TEXT("a left turn leans the head out to the right (%.1f deg)"),FMath::RadiansToDegrees(S.Lean)));
 }
 // turned on the dash (to the driver, about -21 degrees): braking still throws the top forward in the truck
 {
  const FQuat Turned=FQuat(FVector(0,1,0),-.36);
  const FRun B=Drive(Brake,Turned,1.5,60);
  Check(TopOf(B.S,Turned).Z<-.05,TEXT("turned towards the driver, braking still throws the head forward"));
 }
 // a knock: the base jumps 3 cm up and back down in 0.12 s at walking pace
 {
  auto Knock=[](double T){const double K=T>1&&T<1.12?FMath::Sin((T-1)/.12*UE_DOUBLE_PI)*.03:0;return FVector(0,1.47+K,-2*T);};
  const FRun R=Drive(Knock,Facing,2,60);
  Check(FMath::Max(R.MaxNod,-R.MinNod)>.004,FString::Printf(TEXT("a knock in the road makes it nod (%.2f deg)"),FMath::RadiansToDegrees(FMath::Max(R.MaxNod,-R.MinNod))));
 }
 // a cut: the truck is put 300 m away in one frame (another area) and a 2 s hitch: no kick
 {
  FS47BobbleSpring S;const double Dt=1./60;
  for(int32 I=0;I<60;++I)S.Follow(Dt,FVector(0,1.47,-10.*I*Dt),Facing);
  S.Follow(Dt,FVector(300,1.47,40),Facing);S.Follow(Dt,FVector(300,1.47,40-10*Dt),Facing);
  S.Follow(2.,FVector(300,1.47,0),Facing);S.Follow(Dt,FVector(300,1.47,-10*Dt),Facing);
  Check(S.Peak<.01,TEXT("a cut to another place or a long hitch gives no kick"));
 }
 // the hardest knock it may count stays on its collar
 {
  FS47BobbleSpring S;
  for(int32 I=0;I<120;++I)S.Step(1./60,FVector(0,-9.81,-300));
  Check(FMath::Abs(S.Nod)<=S47Bobble::MaxTilt+1e-9,TEXT("never past its collar"));
  S.Settle();S.Kick(3,0);
  for(int32 I=0;I<600;++I)S.Step(1./60,FVector(0,-9.81,0));
  Check(S.Peak>.05&&FMath::Abs(S.Nod)<.002,TEXT("a shove wobbles it, and it comes to rest"));
 }
 // on the Jeep's dash: on the pad, clear of the compass and the wheel, its face to the driver's eye
 {
  const FVector Eye(-.41,1.70,.65);FVector At;double Heading=0;
  FS47Bobblehead::Place(Eye,At,Heading);
  const FVector Face=FQuat(FVector(0,1,0),Heading).RotateVector(FVector(0,0,1));
  const FVector ToEye=FVector(Eye.X-At.X,0,Eye.Z-At.Z).GetSafeNormal();
  Check(FVector::DotProduct(Face,ToEye)>.999,TEXT("its face turned to the driver"));
  Check(At.X<0&&At.X>-.2&&At.Z>-.28&&At.Z<-.11&&FMath::IsNearlyEqual(At.Y,1.47,1e-6),TEXT("on the dash pad, left of the middle"));
  Check(FVector2D(At.X+.28,At.Z+.20).Size()>.12,TEXT("clear of the compass"));
  // the driver's line to the top of the head passes well clear of the steering wheel (hub, radius 0.185 m)
  const FVector Hub(-.41,1.29,.08),Top=At+S47Bobble::HeadPivot+FVector(0,.06,0);
  Check(FMath::PointDistToSegment(Hub,Eye,Top)>.185+.05,TEXT("seen from the seat over the steering wheel"));
 }
 return bOk;
}
