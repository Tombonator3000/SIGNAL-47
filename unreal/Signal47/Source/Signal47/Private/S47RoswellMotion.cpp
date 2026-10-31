#include "S47RoswellMotion.h"
#include "S47TripoCreature.h"
#include "Components/PoseableMeshComponent.h"

namespace
{
 double Ease(double X) { X=FMath::Clamp(X,0.,1.); return X*X*(3.-2.*X); }
 double Follow(double A,double B,double Dt,double Rate) { return FMath::Lerp(A,B,1.-FMath::Exp(-Dt*Rate)); }
}

void FS47RoswellMotion::Reset() { *this=FS47RoswellMotion(); }

FS47RoswellContacts FS47RoswellMotion::Apply(FS47TripoCreature& R,const FS47RoswellPose& In,double Dt)
{
 FS47RoswellContacts Out;
 auto* C=R.Mesh.Get(); if (!C || R.Rest.Num()!=R.Parents.Num() || R.Rest.Num()!=R.Names.Num()) return Out;
 C->SetVisibility(In.bVisible);
 if (!In.bVisible) { bInit=false; return Out; }
 if (In.bHeld) return Out;
 Dt=FMath::Clamp(Dt,0.,.1); Clock+=Dt;
 TArray<FTransform> P=R.Rest;
 for (int32 I=0;I<P.Num();++I) if (R.Parents[I]>=0) P[I]=P[I]*P[R.Parents[I]];
 const TArray<FTransform> Ref=P;
 const auto Bone=[&](const TCHAR* N){ return R.Names.IndexOfByKey(FName(N)); };
 const auto Below=[&](int32 I,int32 A)
 { if (A<0) return false; for (int32 K=I;K>=0;K=R.Parents[K]) if (K==A) return true; return false; };
 const auto Rotate=[&](int32 A,const FQuat& Q)
 {
  if (A<0) return;
  const FVector Pivot=P[A].GetLocation();
  for (int32 I=0;I<P.Num();++I) if (Below(I,A))
  { P[I].SetLocation(Pivot+Q.RotateVector(P[I].GetLocation()-Pivot)); P[I].SetRotation((Q*P[I].GetRotation()).GetNormalized()); }
 };
 const auto Aim=[&](int32 A,int32 B,const FVector& Target)
 {
  if (A<0 || B<0) return;
  const FVector From=(P[B].GetLocation()-P[A].GetLocation()).GetSafeNormal();
  const FVector To=(Target-P[A].GetLocation()).GetSafeNormal();
  if (!From.IsNearlyZero() && !To.IsNearlyZero()) Rotate(A,FQuat::FindBetweenNormals(From,To));
 };
 const int32 Hips=Bone(TEXT("pelvis"));
 const FTransform World=C->GetComponentTransform();
 const FVector Root=World.GetLocation();
 const bool Run=In.Mode==ES47RoswellMotion::Run;
 const bool Sneak=In.Mode==ES47RoswellMotion::Stalk || In.Mode==ES47RoswellMotion::Peek;
 const double Speed=FMath::Clamp(In.SpeedMps,0.,5.);
 // Different contact timing and stride geometry; time alone never walks the feet.
 const double CycleM=Run?1.25:(Sneak?.47:.72), Duty=Run?.49:(Sneak?.73:.62);
 double Travel=bInit?FMath::Max(0.,In.DistanceTravelled-Distance):0.;
 const double Actual=bInit?(Root-LastRoot).Size()/100.:0.;
 const bool Teleport=bInit && (Actual>1.5 || Travel>1.5 || In.DistanceTravelled<Distance);
 if (Teleport) { bInit=false; Travel=0.; }
 // Protect against a controller accumulating attempted travel while blocked.
 Travel=FMath::Min(Travel,Actual+.002);
 const bool Moving=Travel>.00001 && Speed>.025;
 Motion=Follow(Motion,Moving?1.:0.,Dt,Moving?10.:13.);
 const double WantDuck=FMath::Clamp(In.Crouch,0.,1.);
 Duck=Follow(Duck,WantDuck,Dt,7.);
 HeadYaw=Follow(HeadYaw,FMath::Clamp(In.HeadYaw,-1.1,1.1),Dt,8.);
 HeadPitch=Follow(HeadPitch,FMath::Clamp(In.HeadPitch,-.4,.4),Dt,6.);
 Reach=Follow(Reach,In.Mode==ES47RoswellMotion::Grab?1.:0.,Dt,9.);
 Distance=In.DistanceTravelled; LastRoot=Root;
 const double PreviousPhase=Phase;
 if (Moving) Phase+=Travel/CycleM;
 const double Wave=FMath::Sin(Phase*2.*PI);
 const double DuckCm=Duck*S47Roswell::DuckM*100.;
 const double Bob=Motion*(Run?1.4:(Sneak?.18:.7))*(1.-FMath::Cos(Phase*4.*PI));
 if (Hips>=0) for (int32 I=0;I<P.Num();++I) if (Below(I,Hips)) P[I].AddToTranslation(FVector(0,0,-DuckCm+Bob));
 // Spine breathing and a listening tilt remain much smaller than locomotion.
 const double Listen=In.Mode==ES47RoswellMotion::Listen?1.:0.;
 Rotate(Bone(TEXT("spine_01")),FQuat(FVector::XAxisVector,FMath::DegreesToRadians(-(2.+Motion*(Run?14.:(Sneak?10.:5.))+Listen*4.))));
 Rotate(Bone(TEXT("spine_03")),FQuat(FVector::YAxisVector,FMath::DegreesToRadians(.6*FMath::Sin(Clock*1.6)+Motion*Wave*(Run?2.:.7))));
 for (int32 S=0;S<2;++S)
 {
  const int32 Hip=Bone(S==0?TEXT("thigh_l"):TEXT("thigh_r"));
  const int32 Knee=Bone(S==0?TEXT("calf_l"):TEXT("calf_r"));
  const int32 Foot=Bone(S==0?TEXT("foot_l"):TEXT("foot_r"));
  if (Hip<0 || Knee<0 || Foot<0) continue;
  // Reference ankle includes its correct sole offset. The root is already on
  // the controller's supported surface, so roof/floor heights are not guessed.
  const FVector RestFoot=Ref[Foot].GetLocation();
  const FQuat RestQ=Ref[Foot].GetRotation();
  const double F=FMath::Frac(Phase+S*.5), OldF=FMath::Frac(PreviousPhase+S*.5);
  FFoot& Leg=Feet[S];
  if (!bInit)
  { Leg.Anchor=World.TransformPosition(RestFoot); Leg.Start=Leg.Target=Leg.Current=Leg.Anchor; Leg.bSwing=false; }
  const bool Lift=Moving && F>=Duty;
  const bool Landing=Moving && F<OldF && bInit;
  if (Landing)
  {
   // Endpoint is evaluated on this frame's supported surface, then fixed in
   // world space until the next lift. No foot slides during a stance.
   Leg.Anchor=Leg.Target;
   Leg.bSwing=false;
   if (S==0) Out.bLeft=true; else Out.bRight=true;
  }
  if (Lift && !Leg.bSwing)
  {
   Leg.Start=Leg.Anchor; Leg.bSwing=true;
   const double LeadCm=CycleM*100.*(1.-Duty*.5);
   Leg.Target=World.TransformPosition(RestFoot+FVector(0,LeadCm,0));
  }
  FVector FootWorld=Leg.Anchor;
  double SwingT=0.;
  if (Leg.bSwing)
  {
   // If stopped half-way through a step, finish lowering that foot softly;
   // never hang it in the air or emit repeated impacts while stationary.
   SwingT=Moving?Ease((F-Duty)/(1.-Duty)):1.;
   if (!Moving)
   { Leg.Target=World.TransformPosition(RestFoot); Leg.Anchor=FMath::Lerp(Leg.Current,Leg.Target,1.-FMath::Exp(-Dt*18.)); Leg.Start=Leg.Anchor; }
   else FootWorld=FMath::Lerp(Leg.Start,Leg.Target,SwingT);
   FootWorld.Z+=FMath::Sin(SwingT*PI)*(Run?13.:(Sneak?3.:6.));
   if (!Moving) { FootWorld=Leg.Anchor; if ((Leg.Anchor-Leg.Target).Size()<.2) Leg.bSwing=false; }
  }
  // Large turns in place need a small corrective plant instead of stretched
  // legs. The correction is silent and only happens while fully stationary.
  if (!Moving && !Leg.bSwing)
  {
   const FVector Neutral=World.TransformPosition(RestFoot);
   if ((FootWorld-Neutral).Size()>28.) { Leg.Start=FootWorld; Leg.Target=Neutral; Leg.bSwing=true; }
  }
  Leg.Current=FootWorld;
  Out.AtUE[S]=FootWorld-FVector::UpVector*RestFoot.Z;
  const FVector Target=World.InverseTransformPosition(FootWorld);
  const FVector HipAt=P[Hip].GetLocation();
  const double A=(Ref[Knee].GetLocation()-Ref[Hip].GetLocation()).Size();
  const double B=(Ref[Foot].GetLocation()-Ref[Knee].GetLocation()).Size();
  const FVector To=Target-HipAt;
  const double D=FMath::Clamp(To.Size(),FMath::Abs(A-B)+.01,A+B-.01);
  const FVector Along=To.IsNearlyZero()?FVector(0,0,-1):To.GetSafeNormal();
  FVector Bend=(FVector::YAxisVector-Along*FVector::DotProduct(Along,FVector::YAxisVector)).GetSafeNormal();
  if (Bend.IsNearlyZero()) Bend=FVector::XAxisVector;
  const double K=(A*A-B*B+D*D)/(2.*D);
  const FVector KneeAt=HipAt+Along*K+Bend*FMath::Sqrt(FMath::Max(0.,A*A-K*K));
  Aim(Hip,Knee,KneeAt); Aim(Knee,Foot,Target);
  // Toe-first stalking and faster heel clearance, preserving the original foot
  // basis rather than replacing its imported skin orientation with identity.
  const double Toe=Leg.bSwing?FMath::Sin(SwingT*PI)*(Sneak?-.22:.15):0.;
  const FQuat FootQ=FQuat(FVector::XAxisVector,Toe)*RestQ;
  Rotate(Foot,(FootQ*P[Foot].GetRotation().Inverse()).GetNormalized());
  const int32 Arm=Bone(S==0?TEXT("upperarm_l"):TEXT("upperarm_r"));
  const int32 Elbow=Bone(S==0?TEXT("lowerarm_l"):TEXT("lowerarm_r"));
  const int32 Hand=Bone(S==0?TEXT("hand_l"):TEXT("hand_r"));
  const double Sign=S==0?-1.:1., ArmSwing=(S==0?-Wave:Wave)*Motion;
  if (Arm>=0 && Elbow>=0)
  {
   const FVector Idle(Sign*4.,(Sneak?8.:2.)+ArmSwing*(Run?15.:(Sneak?2.:8.)),-26.);
   const FVector Grab(Sign*9.,28.,-7.);
   Aim(Arm,Elbow,P[Arm].GetLocation()+FMath::Lerp(Idle,Grab,Reach));
  }
  if (Elbow>=0 && Hand>=0)
   Aim(Elbow,Hand,P[Elbow].GetLocation()+FMath::Lerp(FVector(Sign*1.,(Sneak?15.:7.)+ArmSwing*5.,-21.),FVector(Sign*2.,24.,0),Reach));
  // Gently curl the retained finger chains; the source skin keeps its shape.
  for (const TCHAR* Digit:{TEXT("Index"),TEXT("Middle"),TEXT("Ring"),TEXT("Pinky"),TEXT("Thumb")})
  {
   const FString N=FString::Printf(TEXT("mixamorig:%sHand%s2"),S==0?TEXT("Left"):TEXT("Right"),Digit);
   Rotate(Bone(*N),FQuat(FVector::XAxisVector,.08+Duck*.13+Reach*.20));
  }
  Leg.Previous=F;
 }
 Rotate(Bone(TEXT("neck_01")),FQuat(FVector::UpVector,HeadYaw*.25));
 Rotate(Bone(TEXT("head")),FQuat(FVector::UpVector,HeadYaw*.75));
 Rotate(Bone(TEXT("head")),FQuat(FVector::XAxisVector,HeadPitch));
 Rotate(Bone(TEXT("head")),FQuat(FVector::YAxisVector,FMath::DegreesToRadians((Listen?5.:1.)*FMath::Sin(Clock*.65))));
 C->BoneSpaceTransforms.SetNum(P.Num());
 for (int32 I=0;I<P.Num();++I) C->BoneSpaceTransforms[I]=R.Parents[I]<0?P[I]:P[I].GetRelativeTransform(P[R.Parents[I]]);
 C->MarkRefreshTransformDirty(); C->RefreshBoneTransforms();
 Out.Energy=Moving?FMath::Clamp(.20+Speed*.22,.18,1.):0.;
 bInit=true;
 return Out;
}
