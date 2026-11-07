#include "S47CrabMotion.h"
#include "S47TripoCreature.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

namespace
{
 double Ease(double V) { V=FMath::Clamp(V,0.,1.);return V*V*(3.-2.*V); }
 double Follow(double A,double B,double Dt,double Rate) { return FMath::Lerp(A,B,1.-FMath::Exp(-Rate*Dt)); }
 double Rad(double Deg) { return FMath::DegreesToRadians(Deg); }
 // One limb at a time: left hand, right foot, right hand, left foot.
 const double StepOffset[4]={0.,.5,.75,.25};
}

FS47CrabContacts FS47CrabMotion::Apply(FS47TripoCreature& R,const FS47CrabPose& In)
{
 FS47CrabContacts Out;
 auto* C=R.Mesh.Get();if(!C||R.Rest.Num()!=R.Parents.Num()||R.Rest.Num()!=R.Names.Num())return Out;
 const auto Bone=[&](const TCHAR* N){return R.Names.IndexOfByKey(FName(N));};
 for(const TCHAR* N:{TEXT("pelvis"),TEXT("spine_01"),TEXT("spine_03"),TEXT("neck_01"),TEXT("head"),TEXT("thigh_l"),TEXT("calf_l"),TEXT("foot_l"),TEXT("thigh_r"),TEXT("calf_r"),TEXT("foot_r"),TEXT("upperarm_l"),TEXT("lowerarm_l"),TEXT("hand_l"),TEXT("upperarm_r"),TEXT("lowerarm_r"),TEXT("hand_r")})
  if(Bone(N)<0)return Out;
 Out.bValid=true;C->SetVisibility(In.bVisible,true);
 if(!In.bVisible){*this=FS47CrabMotion();return Out;}
 if(!FMath::IsFinite(In.Clock)||!FMath::IsFinite(In.HeadYaw)||!FMath::IsFinite(In.Fold)){Out.bValid=false;return Out;}
 const FTransform World=C->GetComponentTransform();const FVector Root=World.GetLocation();
 if(!World.GetScale3D().Equals(FVector::OneVector,.001)){Out.bValid=false;return Out;}
 const double Elapsed=bInit?In.Clock-LastClock:0;
 FVector Delta=bInit?Root-LastRoot:FVector::ZeroVector;Delta.Z=0;
 if(bInit&&(Elapsed<0||Elapsed>.5||Delta.Size()>150)){*this=FS47CrabMotion();Delta=FVector::ZeroVector;}
 if(bInit&&Elapsed<=0)return Out; // a held pose takes no step and no time
 const double Dt=bInit?FMath::Min(Elapsed,.1):0;
 const double Travel=Delta.Size();
 const double F=Ease(In.Fold);const bool bFolded=In.Fold>=.999;
 const bool Moving=bFolded&&Dt>0&&Travel>.01;
 LastClock=In.Clock;LastRoot=Root;
 Motion=Follow(Motion,Moving?1.:0.,Dt,Moving?10.:13.);
 Run=Follow(Run,In.Mode==ES47CrabMode::Run?1.:0.,Dt,8.);
 HeadYaw=Follow(HeadYaw,FMath::Clamp(In.HeadYaw,-1.05,1.05),Dt,9.);
 // Still most of the time, then the head snaps a little to one side.
 const double Seed=FMath::FloorToDouble(In.Clock/1.15);
 HeadTilt=Follow(HeadTilt,16.*(FMath::Frac(FMath::Sin(Seed*12.9898)*43758.5453)-.5)*(1.-Run*.6),Dt,18.);
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
  const FVector Mid=Start+Along*K+Bend*FMath::Sqrt(FMath::Max(0.,Upper*Upper-K*K));
  Aim(A,B,Mid);Aim(B,End,Start+Along*D);
 };
 const int32 Pelvis=Bone(TEXT("pelvis"));
 const int32 Root0[4]={Bone(TEXT("upperarm_l")),Bone(TEXT("upperarm_r")),Bone(TEXT("thigh_l")),Bone(TEXT("thigh_r"))};
 const int32 Mid0[4]={Bone(TEXT("lowerarm_l")),Bone(TEXT("lowerarm_r")),Bone(TEXT("calf_l")),Bone(TEXT("calf_r"))};
 const int32 End0[4]={Bone(TEXT("hand_l")),Bone(TEXT("hand_r")),Bone(TEXT("foot_l")),Bone(TEXT("foot_r"))};
 const double Leg=(Ref[Mid0[2]].GetLocation()-Ref[Root0[2]].GetLocation()).Size()+(Ref[End0[2]].GetLocation()-Ref[Mid0[2]].GetLocation()).Size();
 const double Arm=(Ref[Mid0[0]].GetLocation()-Ref[Root0[0]].GetLocation()).Size()+(Ref[End0[0]].GetLocation()-Ref[Mid0[0]].GetLocation()).Size();
 if(!FMath::IsFinite(Leg)||Leg<20||Leg>200||Arm<20||Arm>200){Out.bValid=false;return Out;}
 // The arms set the stride: a hand has to stay within reach of its shoulder from landing ahead of it to lifting behind it.
 // On the Pale One the arm is 69 cm against a leg of 114, so a stride measured from the leg can never be stood through.
 const double Stride=Arm*.76,Duty=FMath::Lerp(.68,.52,Run),Cycle=Stride/Duty;
 if(Moving)Phase+=Travel/Cycle;
 const double Wave=FMath::Sin(Phase*2*PI);

 // The body comes down and over, the chest ending parallel to the ground.
 Rotate(Pelvis,FQuat(FVector::XAxisVector,Rad(-34.*F)));
 Rotate(Bone(TEXT("spine_01")),FQuat(FVector::XAxisVector,Rad(-(30.+Run*5.)*F)));
 Rotate(Bone(TEXT("spine_03")),FQuat(FVector::XAxisVector,Rad(-16.*F)));
 // As low as it takes to leave the shoulders at three quarters of an arm above the hands, measured on this pose: the elbows
 // then stand out, and there is ground under the hand to plant on. Higher and the arms hang straight with nowhere to go.
 const double HandHeight=Leg*.04;
 const double Rise=P[Root0[0]].GetLocation().Z-P[Pelvis].GetLocation().Z;
 const double PelvisHeight=FMath::Clamp(HandHeight+Arm*.76-Rise,Leg*.22,Leg*FMath::Lerp(.50,.42,Run));
 const double Bob=Motion*Leg*.012*FMath::Sin(Phase*4*PI)+.5*FMath::Sin(In.Clock*1.1);
 const double Down=(Ref[Pelvis].GetLocation().Z-PelvisHeight-Bob)*F;
 for(int32 I=0;I<P.Num();++I)if(Below(I,Pelvis))P[I].AddToTranslation(FVector(0,0,-Down));
 // Hips roll and the shoulders swing against them with each step.
 Rotate(Pelvis,FQuat(FVector::YAxisVector,Rad(Motion*Wave*FMath::Lerp(3.,5.,Run))));
 Rotate(Bone(TEXT("spine_03")),FQuat(FVector::UpVector,Rad(Motion*Wave*FMath::Lerp(5.,9.,Run))));

 FCollisionQueryParams Query(SCENE_QUERY_STAT(S47CrabPlant),true);
 TArray<USceneComponent*> Children;
 if(USceneComponent* Parent=C->GetAttachParent()){Parent->GetChildrenComponents(true,Children);if(auto* Primitive=Cast<UPrimitiveComponent>(Parent))Query.AddIgnoredComponent(Primitive);}
 for(auto* Child:Children)if(auto* Primitive=Cast<UPrimitiveComponent>(Child))Query.AddIgnoredComponent(Primitive);
 Query.AddIgnoredComponent(C);
 const auto Grounded=[&](FVector At,double Height)
 {
  const double Floor=Root.Z;At.Z=Floor+Height;FHitResult Hit;
  if(C->GetWorld()&&C->GetWorld()->LineTraceSingleByChannel(Hit,FVector(At.X,At.Y,Floor+40),FVector(At.X,At.Y,Floor-50),ECC_Visibility,Query)&&FMath::Abs(Hit.ImpactPoint.Z-Floor)<45.)
   At.Z=Hit.ImpactPoint.Z+Height;
  return At;
 };
 const FVector ChestAt=P[Bone(TEXT("spine_03"))].GetLocation();
 const FVector HipAt=P[Pelvis].GetLocation();
 const FVector Direction=Moving?Delta.GetSafeNormal():World.TransformVectorNoScale(FVector::YAxisVector);
 for(int32 L=0;L<4;++L)
 {
  FLimb& Limb=Limbs[L];
  const bool bHand=L<2;
  const double Side=FMath::Sign(Ref[Root0[L]].GetLocation().X-Ref[Pelvis].GetLocation().X);
  const double Height=bHand?HandHeight:Ref[End0[L]].GetLocation().Z;
  // Where the limb belongs folded: hands wide and ahead of the shoulders, well inside their reach, feet wide and behind the hips.
  const FVector Spread=bHand
   ?FVector(Side*(FMath::Abs(P[Root0[L]].GetLocation().X-ChestAt.X)+Arm*.32),P[Root0[L]].GetLocation().Y+Arm*.12,Height)
   :FVector(Side*Leg*.56,HipAt.Y-Leg*.16,Height);
  // Standing: the feet where they are, the arms hanging.
  const FVector Upright=bHand
   ?P[Root0[L]].GetLocation()+FVector(Side*.14,.05,-1).GetSafeNormal()*(Arm*.97)
   :Ref[End0[L]].GetLocation();
  const FVector Neutral=Grounded(World.TransformPosition(Spread),Height);
  bool Landed=false;
  FVector At;
  if(!bFolded)
  {
   At=FMath::Lerp(World.TransformPosition(Upright),Neutral,F);
   Limb.Anchor=Limb.Start=Limb.Target=Limb.Current=Neutral;Limb.bSwing=Limb.bCorrection=false;
  }
  else
  {
   if(!bInit){Limb.Anchor=Limb.Start=Limb.Target=Limb.Current=Neutral;}
   const double Now=Phase+StepOffset[L],Local=FMath::Frac(Now);
   if(Limb.bSwing&&!Limb.bCorrection&&Moving&&Now>=Limb.SwingEnd){Limb.Anchor=Limb.Target;Limb.bSwing=false;Landed=true;}
   const double Length=bHand?Arm:Leg,Longest=Length*.96;
   const FVector RootAt=World.TransformPosition(P[Root0[L]].GetLocation());
   // A limb on the ground is only as long as it is. Past that it lifts, whatever the others are doing, and does not slide.
   const bool bPastReach=!Limb.bSwing&&(Limb.Anchor-RootAt).Size()>Longest&&(Limb.Anchor-Neutral).Size()>Length*.1;
   // Long before it would straighten, a limb the body has gone past takes its step, whatever the turn.
   const bool bFarBehind=Moving&&!Limb.bSwing&&(bPastReach||(Limb.Anchor-Neutral).Size()>(bHand?Arm*.5:Leg*.36));
   // Its turn in the walk, if the body has come past it: one that stepped out of turn may still stand well ahead.
   const bool bItsTurn=Local>=Duty&&FVector::DotProduct(Limb.Anchor-Neutral,Direction)<Stride*.2;
   if(Moving&&!Limb.bSwing&&(bItsTurn||bFarBehind))
   {
    const double ToCycleEnd=FMath::FloorToDouble(Now)+1.-Now;
    Limb.Start=Limb.Current;Limb.SwingStart=Now;Limb.SwingEnd=Local>=Duty&&ToCycleEnd>.1?Now+ToCycleEnd:Now+.22;
    // Half a stride ahead of where it belongs, but never further than it can reach from where the body will be by then.
    const double Ahead=Cycle*(Limb.SwingEnd-Now);
    const FVector Wanted=Neutral+Direction*(Ahead+Stride*.5),RootThen=RootAt+Direction*Ahead;
    const double Most=FMath::Sqrt(FMath::Max(0.,Longest*Longest-FMath::Square(RootThen.Z-Wanted.Z)));
    const FVector2D Off(Wanted.X-RootThen.X,Wanted.Y-RootThen.Y);
    const FVector2D Kept=Off.Size()>Most?Off.GetSafeNormal()*Most:Off;
    Limb.Target=Grounded(FVector(RootThen.X+Kept.X,RootThen.Y+Kept.Y,Wanted.Z),Height);Limb.bSwing=true;Limb.bCorrection=false;
   }
   if(!Moving&&Limb.bSwing&&!Limb.bCorrection){Limb.Start=Limb.Current;Limb.Target=Neutral;Limb.bCorrection=true;Limb.CorrectionTime=0;}
   // Standing or turning on the spot, a limb left behind catches up, one at a time.
   bool bOtherSwing=false;for(int32 K=0;K<4;++K)if(K!=L&&Limbs[K].bSwing)bOtherSwing=true;
   if(!Moving&&!Limb.bSwing&&(bPastReach||(!bOtherSwing&&(Limb.Anchor-Neutral).Size()>Length*.2)))
   {Limb.Start=Limb.Current;Limb.Target=Neutral;Limb.bSwing=Limb.bCorrection=true;Limb.CorrectionTime=0;}
   At=Limb.Anchor;
   if(Limb.bSwing)
   {
    double U;
    // A catching-up limb goes to where it belongs when it lands, not where that was when it lifted.
    if(Limb.bCorrection){Limb.Target=Neutral;Limb.CorrectionTime+=Dt;U=Ease(Limb.CorrectionTime/.26);}else U=Ease((Now-Limb.SwingStart)/FMath::Max(.001,Limb.SwingEnd-Limb.SwingStart));
    At=FMath::Lerp(Limb.Start,Limb.Target,U);At.Z+=FMath::Sin(U*PI)*Leg*(Limb.bCorrection?.10:FMath::Lerp(.20,.26,Run));
    if(Limb.bCorrection&&Limb.CorrectionTime>=.26){Limb.Anchor=Limb.Target;Limb.bSwing=Limb.bCorrection=false;At=Limb.Anchor;Landed=true;}
   }
  }
  Limb.Current=At;
  // Elbows and knees stand out and up, above the back.
  Solve(Root0[L],Mid0[L],End0[L],World.InverseTransformPosition(At),FMath::Lerp(FVector(Side,bHand?-.3:.5,-.6),FVector(Side,bHand?-.15:-.1,bHand?.75:1.),F).GetSafeNormal());
  if(!bHand)
  {
   // The sole lies flat, turned outward.
   const FQuat Q=(FQuat(FVector::UpVector,Rad(-Side*32.*F))*Ref[End0[L]].GetRotation()).GetNormalized();
   Rotate(End0[L],(Q*P[End0[L]].GetRotation().Inverse()).GetNormalized());
   Out.AtUE[L]=World.TransformPosition(P[End0[L]].GetLocation())-FVector::UpVector*Height;
  }
  else
  {
   const TCHAR* Sfx=L==0?TEXT("_l"):TEXT("_r");
   const int32 Middle=Bone(*(FString(TEXT("middle_01"))+Sfx)),Index=Bone(*(FString(TEXT("index_01"))+Sfx)),Pinky=Bone(*(FString(TEXT("pinky_01"))+Sfx));
   if(Middle>=0)
   {
    // Fingers forward and outward along the ground, the palm down.
    const FVector Want=FMath::Lerp(FVector(Side*.15,.15,-1),FVector(Side*.72,.68,-.05),F).GetSafeNormal();
    Rotate(End0[L],FQuat::FindBetweenNormals((P[Middle].GetLocation()-P[End0[L]].GetLocation()).GetSafeNormal(),Want));
    if(Index>=0&&Pinky>=0&&F>.01)
    {
     const FVector Across=P[Index].GetLocation()-P[Pinky].GetLocation();
     const FVector Back=FVector::CrossProduct(Want,Across).GetSafeNormal()*Side;
     const FVector Up=(FVector::UpVector-Want*FVector::DotProduct(FVector::UpVector,Want)).GetSafeNormal();
     if(!Back.IsNearlyZero()&&!Up.IsNearlyZero())
     {
      const double Angle=FMath::Atan2(FVector::DotProduct(FVector::CrossProduct(Back,Up),Want),FVector::DotProduct(Back,Up));
      Rotate(End0[L],FQuat(Want,Angle*F));
     }
    }
   }
   Out.AtUE[L]=World.TransformPosition(P[End0[L]].GetLocation())-FVector::UpVector*Height;
  }
  if(Landed){Out.bStep[L]=true;}
 }
 // The head comes back up, to look ahead along the ground.
 Rotate(Bone(TEXT("neck_01")),FQuat(FVector::XAxisVector,Rad(28.*F)));
 Rotate(Bone(TEXT("head")),FQuat(FVector::XAxisVector,Rad(46.*F)));
 Rotate(Bone(TEXT("neck_01")),FQuat(FVector::UpVector,HeadYaw*.3));
 Rotate(Bone(TEXT("head")),FQuat(FVector::UpVector,HeadYaw*.7));
 const FVector Face=P[Bone(TEXT("head"))].GetRotation().RotateVector(Ref[Bone(TEXT("head"))].GetRotation().Inverse().RotateVector(FVector::YAxisVector));
 Rotate(Bone(TEXT("head")),FQuat(Face.GetSafeNormal(),Rad(HeadTilt)*F));
 C->BoneSpaceTransforms.SetNum(P.Num());
 for(int32 I=0;I<P.Num();++I)C->BoneSpaceTransforms[I]=R.Parents[I]<0?P[I]:P[I].GetRelativeTransform(P[R.Parents[I]]);
 C->MarkRefreshTransformDirty();C->RefreshBoneTransforms();bInit=true;
 for(int32 L=0;L<4;++L)if(!Limbs[L].bSwing&&bFolded)
 {
  ++Out.PlantedLimbs;
  Out.MaxPlantErrorCm=FMath::Max(Out.MaxPlantErrorCm,(C->GetBoneTransformByName(R.Names[End0[L]],EBoneSpaces::WorldSpace).GetLocation()-Limbs[L].Anchor).Size());
 }
 return Out;
}
