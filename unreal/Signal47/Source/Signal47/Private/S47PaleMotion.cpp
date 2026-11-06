#include "S47PaleMotion.h"
#include "S47TripoCreature.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

namespace
{
 double Ease(double V) { V=FMath::Clamp(V,0.,1.);return V*V*(3.-2.*V); }
 double Follow(double A,double B,double Dt,double Rate) { return FMath::Lerp(A,B,1.-FMath::Exp(-Rate*Dt)); }
}
FS47PaleContacts FS47PaleMotion::Apply(FS47TripoCreature& R,const FS47PalePose& In)
{
 FS47PaleContacts Out;
 auto* C=R.Mesh.Get();if(!C||R.Rest.Num()!=R.Parents.Num()||R.Rest.Num()!=R.Names.Num())return Out;
 const auto Bone=[&](const TCHAR* N){return R.Names.IndexOfByKey(FName(N));};
 for(const TCHAR* N:{TEXT("pelvis"),TEXT("spine_01"),TEXT("spine_03"),TEXT("head"),TEXT("thigh_l"),TEXT("calf_l"),TEXT("foot_l"),TEXT("thigh_r"),TEXT("calf_r"),TEXT("foot_r"),TEXT("upperarm_l"),TEXT("lowerarm_l"),TEXT("hand_l"),TEXT("upperarm_r"),TEXT("lowerarm_r"),TEXT("hand_r")})if(Bone(N)<0)return Out;
 Out.bValid=true;C->SetVisibility(In.bVisible);
 if(!In.bVisible){*this=FS47PaleMotion();return Out;}
 if(!FMath::IsFinite(In.Clock)||!FMath::IsFinite(In.HeadYaw)){Out.bValid=false;return Out;}
 const FTransform World=C->GetComponentTransform();const FVector Root=World.GetLocation();
 if(!World.GetScale3D().Equals(FVector::OneVector,.001)){Out.bValid=false;return Out;}
 const double Elapsed=bInit?In.Clock-LastClock:0;
 FVector Delta=bInit?Root-LastRoot:FVector::ZeroVector;Delta.Z=0;
 if(bInit&&(Elapsed<0||Elapsed>.5||Delta.Size()>150)) {*this=FS47PaleMotion();Delta=FVector::ZeroVector;}
 if(bInit&&Elapsed<=0)return Out; // held pose; no repeated footstep or independent timer
 const double Dt=bInit?FMath::Min(Elapsed,.1):0;
 const double Travel=Delta.Size();const bool Moving=Dt>0&&Travel>.01;
 LastClock=In.Clock;LastRoot=Root;
 Motion=Follow(Motion,Moving?1.:0.,Dt,Moving?10.:13.);
 Run=Follow(Run,In.Mode==ES47PaleMotion::Run?1.:0.,Dt,8.);
 HeadYaw=Follow(HeadYaw,FMath::Clamp(In.HeadYaw,-1.05,1.05),Dt,8.);
 // .7 s controller hold is retained: prepare, reach, then contact before its existing capture event.
 // Negative progress has no invented contact credit (only explicit preview/controller progress reaches).
 const double Grab=In.Mode==ES47PaleMotion::Grab&&In.GrabProgress>=0?FMath::Clamp(In.GrabProgress,0.,1.):0;
 const double WantReach=In.Mode==ES47PaleMotion::Grab?Ease((Grab-.18)/.42):0;
 Reach=Follow(Reach,WantReach,Dt,In.Mode==ES47PaleMotion::Grab?22.:13.);
 if(In.Mode==ES47PaleMotion::Grab&&In.GrabTargetUE.IsSet()&&!In.GrabTargetUE.GetValue().ContainsNaN())LastGrabTargetUE=In.GrabTargetUE;
 TArray<FTransform> P=R.Rest;
 for(int32 I=0;I<P.Num();++I)if(R.Parents[I]>=0)P[I]=P[I]*P[R.Parents[I]];
 const TArray<FTransform> Ref=P;
 const auto Below=[&](int32 I,int32 A){if(A<0)return false;for(int32 K=I;K>=0;K=R.Parents[K])if(K==A)return true;return false;};
 const auto Rotate=[&](int32 A,const FQuat& Q)
 {
  if(A<0)return;const FVector Pivot=P[A].GetLocation();
  for(int32 I=0;I<P.Num();++I)if(Below(I,A)){P[I].SetLocation(Pivot+Q.RotateVector(P[I].GetLocation()-Pivot));P[I].SetRotation((Q*P[I].GetRotation()).GetNormalized());}
 };
 const auto Aim=[&](int32 A,int32 B,const FVector& Target)
 {
  const FVector From=(P[B].GetLocation()-P[A].GetLocation()).GetSafeNormal(),To=(Target-P[A].GetLocation()).GetSafeNormal();
  if(!From.IsNearlyZero()&&!To.IsNearlyZero())Rotate(A,FQuat::FindBetweenNormals(From,To));
 };
 const auto Solve=[&](int32 A,int32 B,int32 End,const FVector& Target,const FVector& PreferredBend)
 {
  const double Upper=(Ref[B].GetLocation()-Ref[A].GetLocation()).Size(),Lower=(Ref[End].GetLocation()-Ref[B].GetLocation()).Size();
  if(Upper<1||Lower<1)return;
  const FVector Start=P[A].GetLocation(),To=Target-Start;
  const FVector Along=To.IsNearlyZero()?FVector(0,0,-1):To.GetSafeNormal();
  const double D=FMath::Clamp(To.Size(),FMath::Abs(Upper-Lower)+.01,Upper+Lower-.01);
  FVector Bend=(PreferredBend-Along*FVector::DotProduct(PreferredBend,Along)).GetSafeNormal();
  if(Bend.IsNearlyZero())Bend=FVector::CrossProduct(Along,FVector::XAxisVector).GetSafeNormal();
  if(Bend.IsNearlyZero())Bend=FVector::YAxisVector;
  const double K=(Upper*Upper-Lower*Lower+D*D)/(2*D);
  const FVector Knee=Start+Along*K+Bend*FMath::Sqrt(FMath::Max(0.,Upper*Upper-K*K));
  Aim(A,B,Knee);Aim(B,End,Start+Along*D);
 };
 const int32 Hip[2]={Bone(TEXT("thigh_l")),Bone(TEXT("thigh_r"))},Knee[2]={Bone(TEXT("calf_l")),Bone(TEXT("calf_r"))},Foot[2]={Bone(TEXT("foot_l")),Bone(TEXT("foot_r"))};
 const double Leg=(Ref[Knee[0]].GetLocation()-Ref[Hip[0]].GetLocation()).Size()+(Ref[Foot[0]].GetLocation()-Ref[Knee[0]].GetLocation()).Size();
 if(!FMath::IsFinite(Leg)||Leg<20||Leg>200){Out.bValid=false;return Out;}
 const bool Tall=In.Style==ES47PaleStyle::Tall;
 const double Cycle=Leg*(Tall?FMath::Lerp(1.15,1.3,Run):FMath::Lerp(.95,1.25,Run)),Duty=Tall?FMath::Lerp(.66,.48,Run):FMath::Lerp(.64,.48,Run);
 if(!bInit)Phase=Duty*.5; // initial stance never drags one planted foot a whole first cycle
 if(Moving)Phase+=Travel/Cycle;
 const double Wave=FMath::Sin(Phase*2*PI);
 const int32 Pelvis=Bone(TEXT("pelvis"));
 // Reference-derived bend reserve keeps planted legs reachable; dimensions are not Roswell constants.
 const double Lower=Leg*(.035+Motion*FMath::Lerp(.025,.055,Run));
 const double Bob=Motion*Leg*FMath::Lerp(.006,.018,Run)*(1.-FMath::Cos(Phase*4*PI));
 for(int32 I=0;I<P.Num();++I)if(Below(I,Pelvis))P[I].AddToTranslation(FVector(0,0,-Lower+Bob));
 const bool Listen=In.Mode==ES47PaleMotion::Listen;
 // SC02 retains a grab up to 1.05 m. A bounded visual spine bend uses the real target;
 // original arms, collision root and planted legs remain unchanged. Existing Reach also blends release.
 double FarGrabLean=0;
 if(Reach>.001&&LastGrabTargetUE.IsSet())
 {
  const FVector Chest=World.InverseTransformPosition(LastGrabTargetUE.GetValue());
  if(Chest.Y>0)FarGrabLean=16.*Ease((Chest.Size2D()-90.)/15.)*Reach;
 }
 Rotate(Bone(TEXT("spine_01")),FQuat(FVector::XAxisVector,FMath::DegreesToRadians(-(2.+Motion*FMath::Lerp(5.,15.,Run)+(Listen?4.:0.)+Reach*4.+FarGrabLean+(Tall?9.:0.)))));
 Rotate(Bone(TEXT("spine_03")),FQuat(FVector::YAxisVector,FMath::DegreesToRadians(.5*FMath::Sin(In.Clock*1.3)+Motion*Wave*FMath::Lerp(.6,2.,Run))));
 // Probe only visual foot targets. The controller still owns walkability, collision, position and floor.
 FCollisionQueryParams Query(SCENE_QUERY_STAT(S47PaleFootPlant),true);
 TArray<USceneComponent*> Children;
 if(USceneComponent* Parent=C->GetAttachParent()){Parent->GetChildrenComponents(true,Children);if(auto* Primitive=Cast<UPrimitiveComponent>(Parent))Query.AddIgnoredComponent(Primitive);}
 for(auto* Child:Children)if(auto* Primitive=Cast<UPrimitiveComponent>(Child))Query.AddIgnoredComponent(Primitive);
 Query.AddIgnoredComponent(C);
 const auto Grounded=[&](FVector At,double AnkleHeight)
 {
  const double Floor=Root.Z;At.Z=Floor+AnkleHeight;FHitResult Hit;
  if(C->GetWorld()&&C->GetWorld()->LineTraceSingleByChannel(Hit,FVector(At.X,At.Y,Floor+25),FVector(At.X,At.Y,Floor-35),ECC_Visibility,Query)&&FMath::Abs(Hit.ImpactPoint.Z-Floor)<=18)
   At.Z=Hit.ImpactPoint.Z+AnkleHeight;
  return At;
 };
 const FVector Direction=Moving?Delta.GetSafeNormal():World.TransformVectorNoScale(FVector::YAxisVector);
 for(int32 S=0;S<2;++S)
 {
  FFoot& F=Feet[S];const FVector RestFoot=Ref[Foot[S]].GetLocation();const FQuat RestQ=Ref[Foot[S]].GetRotation();
  const FVector Neutral=Grounded(World.TransformPosition(RestFoot),RestFoot.Z);
  const FQuat NeutralQ=(World.GetRotation()*RestQ).GetNormalized();
  if(!bInit){F.Anchor=F.Start=F.Target=F.Current=Neutral;F.AnchorQ=F.StartQ=F.TargetQ=F.CurrentQ=NeutralQ;}
  const double Now=Phase+S*.5,Local=FMath::Frac(Now);
  bool Landed=false;
  if(F.bSwing&&!F.bCorrection&&Moving&&Now>=F.SwingEnd)
  {F.Anchor=F.Target;F.AnchorQ=F.TargetQ;F.bSwing=false;Landed=true;}
  if(Moving&&!F.bSwing&&Local>=Duty)
  {F.Start=F.Current;F.StartQ=F.CurrentQ;F.TargetQ=NeutralQ;F.SwingStart=Now;F.SwingEnd=FMath::FloorToDouble(Now)+1.;
   // Include remaining root travel during swing; land half a stance ahead of the moving root.
   F.Target=Grounded(Neutral+Direction*(Cycle*(F.SwingEnd-Now+Duty*.5)),RestFoot.Z);F.bSwing=true;F.bCorrection=false;}
  // Complete a stopped swing, or one corrective step at a time during an actual root turn.
  if(!Moving&&F.bSwing&&!F.bCorrection)
  {F.Start=F.Current;F.Target=Neutral;F.StartQ=F.CurrentQ;F.TargetQ=NeutralQ;F.bCorrection=true;F.bCorrectionLift=false;F.CorrectionTime=0;}
  if(!Moving&&!F.bSwing&&!Feet[1-S].bSwing&&((F.Anchor-Neutral).Size()>Leg*.13||F.AnchorQ.AngularDistance(NeutralQ)>FMath::DegreesToRadians(12.)))
  {F.Start=F.Current;F.Target=Neutral;F.StartQ=F.CurrentQ;F.TargetQ=NeutralQ;F.bSwing=F.bCorrection=true;F.bCorrectionLift=true;F.CorrectionTime=0;}
  FVector At=F.Anchor;FQuat AtQ=F.AnchorQ;double U=0;
  if(F.bSwing)
  {
   if(F.bCorrection){F.CorrectionTime+=Dt;U=Ease(F.CorrectionTime/.24);}else U=Ease((Now-F.SwingStart)/FMath::Max(.001,F.SwingEnd-F.SwingStart));
   At=FMath::Lerp(F.Start,F.Target,U);At.Z+=FMath::Sin(U*PI)*Leg*(F.bCorrection?(F.bCorrectionLift?.035:0.):FMath::Lerp(.055,.12,Run));
   AtQ=FQuat::Slerp(F.StartQ,F.TargetQ,U).GetNormalized();
   if(F.bCorrection&&F.CorrectionTime>=.24){F.Anchor=F.Target;F.AnchorQ=F.TargetQ;F.bSwing=F.bCorrection=false;At=F.Anchor;AtQ=F.AnchorQ;Landed=true;}
  }
  F.Current=At;
  Solve(Hip[S],Knee[S],Foot[S],World.InverseTransformPosition(At),FVector::YAxisVector);
  if(F.bSwing&&!F.bCorrection)AtQ=(FQuat(World.TransformVectorNoScale(FVector::XAxisVector),FMath::Sin(U*PI)*FMath::Lerp(.12,.25,Run))*AtQ).GetNormalized();
  F.CurrentQ=AtQ;
  // A stance fixes the whole foot in world space, not only its ankle. Root yaw must not spin toes/heel.
  const FQuat Q=(World.GetRotation().Inverse()*AtQ).GetNormalized();
  Rotate(Foot[S],(Q*P[Foot[S]].GetRotation().Inverse()).GetNormalized());
  // Report actual posed ankle minus its rest sole offset, not an unreachable IK target.
  Out.AtUE[S]=World.TransformPosition(P[Foot[S]].GetLocation())-FVector::UpVector*RestFoot.Z;
  const double Error=(World.TransformPosition(P[Foot[S]].GetLocation())-At).Size();
  if(!F.bSwing)Out.MaxPlantErrorCm=FMath::Max(Out.MaxPlantErrorCm,Error);
  if(Landed&&Error<=2.){if(S==0)Out.bLeft=true;else Out.bRight=true;}
 }
 for(int32 S=0;S<2;++S)
 {
  const int32 Arm=Bone(S==0?TEXT("upperarm_l"):TEXT("upperarm_r")),Elbow=Bone(S==0?TEXT("lowerarm_l"):TEXT("lowerarm_r")),Hand=Bone(S==0?TEXT("hand_l"):TEXT("hand_r"));
  const double Upper=(Ref[Elbow].GetLocation()-Ref[Arm].GetLocation()).Size(),Fore=(Ref[Hand].GetLocation()-Ref[Elbow].GetLocation()).Size();
  const double Sign=FMath::Sign(Ref[Arm].GetLocation().X-Ref[Bone(TEXT("spine_03"))].GetLocation().X),Swing=(S==0?-Wave:Wave)*Motion;
  const double Prepare=In.Mode==ES47PaleMotion::Grab?Ease(Grab/.16)*(1.-Ease((Grab-.16)/.20)):0;
  Aim(Arm,Elbow,P[Arm].GetLocation()+FVector(Sign*.15, Swing*FMath::Lerp(.22,.55,Run)*(Tall?.3:1.)+Prepare*.25+(Tall?.1:0.),-1)*Upper);
  Aim(Elbow,Hand,P[Elbow].GetLocation()+FVector(Sign*.06,(Tall?.08:.18)+Run*(Tall?.25:.5)+Swing*(Tall?.05:.12),-1)*Fore);
  if(Reach>.001&&LastGrabTargetUE.IsSet())
  {
   const FVector Contact=World.InverseTransformPosition(LastGrabTargetUE.GetValue())+FVector(Sign*Leg*.10,0,0);
   const FVector Target=FMath::Lerp(P[Hand].GetLocation(),Contact,Reach);
   Solve(Arm,Elbow,Hand,Target,FVector(Sign,0,-.5).GetSafeNormal());
  }
 }
 if(Tall)
 {
  // The neck reaches forward and the head comes level again; every so often it slowly tips to one side.
  const double Seed=FMath::FloorToDouble(In.Clock/2.7);
  HeadTilt=Follow(HeadTilt,22.*(FMath::Frac(FMath::Sin(Seed*12.9898)*43758.5453)-.5),Dt,3.);
  Rotate(Bone(TEXT("neck_01")),FQuat(FVector::XAxisVector,FMath::DegreesToRadians(-8.)));
  Rotate(Bone(TEXT("head")),FQuat(FVector::XAxisVector,FMath::DegreesToRadians(15.)));
  Rotate(Bone(TEXT("head")),FQuat(FVector::YAxisVector,FMath::DegreesToRadians(HeadTilt)));
 }
 Rotate(Bone(TEXT("neck_01")),FQuat(FVector::UpVector,HeadYaw*.25));
 Rotate(Bone(TEXT("head")),FQuat(FVector::UpVector,HeadYaw*.75));
 Rotate(Bone(TEXT("head")),FQuat(FVector::YAxisVector,FMath::DegreesToRadians((Listen?4.:.7)*FMath::Sin(In.Clock*.65))));
 C->BoneSpaceTransforms.SetNum(P.Num());
 for(int32 I=0;I<P.Num();++I)C->BoneSpaceTransforms[I]=R.Parents[I]<0?P[I]:P[I].GetRelativeTransform(P[R.Parents[I]]);
 C->MarkRefreshTransformDirty();C->RefreshBoneTransforms();bInit=true;
 // Measure actual refreshed bone transforms. An ankle-only residual misses a rotating planted foot.
 for(int32 S=0;S<2;++S)
 {
  const FTransform Actual=C->GetBoneTransformByName(R.Names[Foot[S]],EBoneSpaces::WorldSpace);
  Out.RotationUE[S]=Actual.GetRotation().GetNormalized();
  if(Feet[S].bSwing)continue;
  Out.MaxPlantErrorCm=FMath::Max(Out.MaxPlantErrorCm,(Actual.GetLocation()-Feet[S].Anchor).Size());
  Out.MaxPlantAngleDeg=FMath::Max(Out.MaxPlantAngleDeg,FMath::RadiansToDegrees(Out.RotationUE[S].AngularDistance(Feet[S].AnchorQ)));
  const FTransform Planted(Feet[S].AnchorQ,Feet[S].Anchor,Ref[Foot[S]].GetScale3D());
  // Rig-relative toe/heel direction probes, not claims about authored mesh sole vertices.
  for(double Forward:{Leg*.15,-Leg*.08})
  {
   // Component-space centimetres must become bone-local units, including imported bone scale.
   const FVector Local=Ref[Foot[S]].InverseTransformVector(FVector(0,Forward,-Ref[Foot[S]].GetLocation().Z));
   Out.MaxSoleProbeErrorCm=FMath::Max(Out.MaxSoleProbeErrorCm,(Actual.TransformPosition(Local)-Planted.TransformPosition(Local)).Size());
  }
 }
 return Out;
}
