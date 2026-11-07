#include "S47CreatureClips.h"
#include "S47TripoCreature.h"
#include "S47PaleMotion.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Misc/Paths.h"

namespace
{
 double ClipEase(double V) { V=FMath::Clamp(V,0.,1.);return V*V*(3.-2.*V); }
 double ClipFollow(double A,double B,double Dt,double Rate) { return FMath::Lerp(A,B,1.-FMath::Exp(-Rate*Dt)); }
 constexpr double FadeSeconds=.3;
 const TCHAR* const ClipFile[int32(ES47Clip::Num)]={TEXT("idle"),TEXT("walk"),TEXT("climb"),TEXT("jump_down"),TEXT("angry"),TEXT("frustrated"),TEXT("sneak"),TEXT("listen"),TEXT("run"),TEXT("fall"),TEXT("afraid"),TEXT("observe"),TEXT("look_around"),TEXT("relax"),TEXT("shh"),TEXT("sit"),TEXT("jump"),TEXT("freeze")};
}

TSharedPtr<FS47ClipBank> FS47ClipBank::For(const USkeletalMesh* Mesh,const FS47TripoCreature& Creature,const TCHAR* Prefix)
{
 const USkeleton* Skeleton=Mesh?Mesh->GetSkeleton():nullptr;
 if(!Skeleton)return nullptr;
 // One per mesh for as long as anything wears it.
 static TMap<const USkeletalMesh*,TWeakPtr<FS47ClipBank>> Live;
 if(const TWeakPtr<FS47ClipBank>* Found=Live.Find(Mesh))if(TSharedPtr<FS47ClipBank> Same=Found->Pin())return Same;
 TSharedPtr<FS47ClipBank> Bank=MakeShared<FS47ClipBank>();
 Bank->Rest=Creature.Rest;Bank->Parents=Creature.Parents;Bank->Names=Creature.Names;
 const FReferenceSkeleton& Ref=Skeleton->GetReferenceSkeleton();
 for(const FName& Name:Creature.Names)Bank->SkeletonIndex.Add(Ref.FindBoneIndex(Name));
 Bank->PelvisBone=Creature.Names.IndexOfByKey(FName(TEXT("pelvis")));
 if(Bank->PelvisBone<0)return nullptr;
 {
  FTransform At=FTransform::Identity;
  for(int32 B=Bank->PelvisBone;B>=0;B=Creature.Parents[B])At=At*Creature.Rest[B];
  Bank->RestPelvis=At.GetLocation();
 }
 int32 Count=0;
 FString Missing;
 for(int32 I=0;I<int32(ES47Clip::Num);++I)
 {
  const FString Path=FString::Printf(TEXT("%s%s.%s"),Prefix,ClipFile[I],*(FPaths::GetBaseFilename(Prefix)+ClipFile[I]));
  UAnimSequence* Seq=LoadObject<UAnimSequence>(nullptr,*Path,nullptr,LOAD_NoWarn|LOAD_Quiet);
  if(!Seq||Seq->GetSkeleton()!=Skeleton||Seq->GetPlayLength()<=.01f){Missing+=FString(ClipFile[I])+TEXT(" ");continue;}
  FClip& Clip=Bank->Clips[I];
  Clip.Seq=TStrongObjectPtr<UAnimSequence>(Seq);Clip.Length=Seq->GetPlayLength();
  Clip.Frames=FMath::Max(2,FMath::RoundToInt(Clip.Length*Rate)+1);
  Clip.Rows.SetNumUninitialized(Clip.Frames*Bank->Rest.Num());
  Clip.Filled.Init(false,Clip.Frames);
  ++Count;
 }
 if(!Bank->Has(Bank->Standing())||!Bank->Has(ES47Clip::Walk))
 {
  UE_LOG(LogTemp,Display,TEXT("S47: creature clips: no walk and nothing to stand in under %s, the body keeps its procedural motions (%d found)"),Prefix,Count);
  return nullptr;
 }
 for(const ES47Clip Gait:{ES47Clip::Walk,ES47Clip::Sneak,ES47Clip::Run})Bank->ReadGait(Gait);
 Live.Add(Mesh,Bank);
 UE_LOG(LogTemp,Display,TEXT("S47: creature clips: %d under %s, walk %.2f m/s, sneak %.2f m/s, run %.2f m/s, not found: %s"),Count,Prefix,Bank->GaitSpeedMps(ES47Clip::Walk),Bank->GaitSpeedMps(ES47Clip::Sneak),Bank->GaitSpeedMps(ES47Clip::Run),*Missing);
 return Bank;
}

bool FS47ClipBank::Has(ES47Clip Clip) const{return Clip<ES47Clip::Num&&Clips[int32(Clip)].Seq.IsValid();}
double FS47ClipBank::Seconds(ES47Clip Clip) const{return Has(Clip)?Clips[int32(Clip)].Length:0.;}

const FS47ClipBank::FRow* FS47ClipBank::RowsAt(FClip& Clip,int32 Frame)
{
 Frame=FMath::Clamp(Frame,0,Clip.Frames-1);
 const int32 Bones=Rest.Num();
 FRow* Rows=Clip.Rows.GetData()+Frame*Bones;
 if(!Clip.Filled[Frame])
 {
  const FAnimExtractContext At(FMath::Min(double(Frame)/Rate,Clip.Length),false,FDeltaTimeRecord(),false);
  for(int32 B=0;B<Bones;++B)
  {
   // A track the compression dropped equals the rest pose; it writes nothing, so start from that.
   FTransform Atom=Rest[B];
   if(SkeletonIndex[B]!=INDEX_NONE)Clip.Seq->GetBoneTransform(Atom,FSkeletonPoseBoneIndex(SkeletonIndex[B]),At,false);
   Rows[B].Q=FQuat4f(Atom.GetRotation().GetNormalized());Rows[B].T=FVector3f(Atom.GetTranslation());
  }
  Clip.Filled[Frame]=true;
 }
 return Rows;
}

void FS47ClipBank::Sample(ES47Clip Clip,double Time,TArray<FTransform>& Out)
{
 const int32 Bones=Rest.Num();
 Out.SetNum(Bones);
 if(!Has(Clip)){Out=Rest;return;}
 FClip& C=Clips[int32(Clip)];
 const double Position=FMath::Clamp(Time,0.,C.Length)*Rate;
 const int32 A=FMath::FloorToInt(Position);
 const float Alpha=float(Position-A);
 const FRow* RowA=RowsAt(C,A);
 const FRow* RowB=RowsAt(C,A+1);
 for(int32 B=0;B<Bones;++B)
 {
  // The skin's root holds the file's unit and frame, and a clip's does not: it stands where the skin has it.
  if(Parents[B]<0){Out[B]=Rest[B];continue;}
  const FQuat4f Q=FQuat4f::FastLerp(RowA[B].Q,RowB[B].Q,Alpha).GetNormalized();
  const FVector3f T=FMath::Lerp(RowA[B].T,RowB[B].T,Alpha);
  Out[B]=FTransform(FQuat(Q),FVector(T));
 }
}

FVector FS47ClipBank::BoneAt(FClip& Clip,int32 Frame,int32 Bone)
{
 const FRow* Rows=RowsAt(Clip,Frame);
 FTransform World=FTransform::Identity;
 for(int32 B=Bone;B>=0;B=Parents[B])World=World*(Parents[B]<0?Rest[B]:FTransform(FQuat(Rows[B].Q),FVector(Rows[B].T)));
 return World.GetLocation();
}

FVector FS47ClipBank::Travel(ES47Clip Clip)
{
 if(!Has(Clip)||PelvisBone<0)return FVector::ZeroVector;
 FClip& C=Clips[int32(Clip)];
 FVector D=BoneAt(C,C.Frames-1,PelvisBone)-BoneAt(C,0,PelvisBone);
 D.Z=0;
 return D;
}

FVector FS47ClipBank::StartOffset(ES47Clip Clip)
{
 if(!Has(Clip)||PelvisBone<0)return FVector::ZeroVector;
 FVector D=BoneAt(Clips[int32(Clip)],0,PelvisBone)-RestPelvis;
 D.Z=0;
 return D;
}

void FS47ClipBank::ReadGait(ES47Clip Which)
{
 if(!Has(Which))return;
 FClip& C=Clips[int32(Which)];
 C.Cycle=Travel(Which).Size();
 const double Speed=C.Cycle/FMath::Max(.01,C.Length);
 const int32 FootBone[2]={Names.IndexOfByKey(FName(TEXT("foot_l"))),Names.IndexOfByKey(FName(TEXT("foot_r")))};
 for(int32 S=0;S<2;++S)
 {
  C.Stance[S].Init(false,C.Frames);
  if(FootBone[S]<0)continue;
  // The clip goes across the ground, so a foot on the ground is what stands still in the clip's own space and is low.
  TArray<FVector> At;At.Reserve(C.Frames);
  double Lowest=DBL_MAX;
  for(int32 F=0;F<C.Frames;++F){At.Add(BoneAt(C,F,FootBone[S]));Lowest=FMath::Min(Lowest,At.Last().Z);}
  for(int32 F=0;F<C.Frames;++F)
  {
   const FVector Move=At[FMath::Min(F+1,C.Frames-1)]-At[FMath::Max(F-1,0)];
   const double FootSpeed=FVector(Move.X,Move.Y,0).Size()*Rate/2.;
   C.Stance[S][F]=FootSpeed<Speed*.3&&At[F].Z<Lowest+C.Cycle*.04;
  }
 }
}

double FS47ClipBank::GaitCycleCm(ES47Clip Clip) const{return Has(Clip)?Clips[int32(Clip)].Cycle:0.;}
double FS47ClipBank::GaitSpeedMps(ES47Clip Clip) const{return Has(Clip)?Clips[int32(Clip)].Cycle/FMath::Max(.01,Clips[int32(Clip)].Length)/100.:0.;}

bool FS47ClipBank::GaitStance(ES47Clip Clip,int32 Foot,double U) const
{
 if(!Has(Clip)||Foot<0||Foot>1)return false;
 const TArray<bool>& Row=Clips[int32(Clip)].Stance[Foot];
 if(Row.IsEmpty())return false;
 return Row[FMath::Clamp(FMath::FloorToInt(FMath::Frac(U)*(Row.Num()-1)),0,Row.Num()-1)];
}

void FS47ClipMotion::FadeFromShown(const UPoseableMeshComponent* Mesh)
{
 if(Mesh){FadeFrom=Mesh->BoneSpaceTransforms;FadeLeft=FadeSeconds;}
}

FVector FS47ClipMotion::ActRootVelocity(FS47ClipBank& Bank) const
{
 if(!CarriesRoot(Now)||bActDone||!Bank.Has(Now))return FVector::ZeroVector;
 return Bank.Travel(Now)/FMath::Max(.01,Bank.Seconds(Now));
}

void FS47ClipMotion::Apply(FS47TripoCreature& R,FS47ClipBank& Bank,const FS47ClipPose& In,FS47PaleContacts& Out)
{
 auto* C=R.Mesh.Get();
 if(!C||R.Rest.Num()!=R.Parents.Num()||R.Rest.Num()!=R.Names.Num())return;
 const auto Bone=[&](const TCHAR* N){return R.Names.IndexOfByKey(FName(N));};
 const int32 Hip[2]={Bone(TEXT("thigh_l")),Bone(TEXT("thigh_r"))},Knee[2]={Bone(TEXT("calf_l")),Bone(TEXT("calf_r"))},Ankle[2]={Bone(TEXT("foot_l")),Bone(TEXT("foot_r"))};
 const int32 Pelvis=Bone(TEXT("pelvis")),HeadBone=Bone(TEXT("head")),Neck=Bone(TEXT("neck_01"));
 for(const int32 Needed:{Hip[0],Hip[1],Knee[0],Knee[1],Ankle[0],Ankle[1],Pelvis,HeadBone})if(Needed<0)return;
 Out.bValid=true;
 C->SetVisibility(In.bVisible,true);
 if(!In.bVisible){*this=FS47ClipMotion();return;}
 if(!FMath::IsFinite(In.Clock)||!FMath::IsFinite(In.HeadYaw)){Out.bValid=false;return;}
 const FTransform World=C->GetComponentTransform();const FVector Root=World.GetLocation();
 const double Elapsed=bInit?In.Clock-LastClock:0;
 FVector Delta=bInit?Root-LastRoot:FVector::ZeroVector;Delta.Z=0;
 if(bInit&&(Elapsed<0||Elapsed>.5||Delta.Size()>150)){*this=FS47ClipMotion();Delta=FVector::ZeroVector;}
 if(bInit&&Elapsed<=0)return; // a held pose takes no step and no time
 const double Dt=bInit?FMath::Min(Elapsed,.1):0;
 const double Moved=Delta.Size();
 StillFor=Dt>0&&Moved/Dt>4.?0.:StillFor+Dt;
 const bool bWalking=bInit&&StillFor<.15;
 LastClock=In.Clock;LastRoot=Root;

 // What to play, faded from what the last frame showed.
 const ES47Clip Stand=In.Stand<ES47Clip::Num&&Bank.Has(In.Stand)?In.Stand:Bank.Standing();
 const ES47Clip Gait=In.Move<ES47Clip::Num&&Bank.Has(In.Move)?In.Move:ES47Clip::Walk;
 const ES47Clip Want=In.Act<ES47Clip::Num&&Bank.Has(In.Act)?In.Act:bWalking?Gait:Stand;
 if(Want!=Now)
 {
  if(bInit&&C->BoneSpaceTransforms.Num()==R.Rest.Num()){FadeFrom=C->BoneSpaceTransforms;FadeLeft=FadeSeconds;}
  Now=Want;ClipTime=0;bActDone=false;
 }
 const bool bGait=Bank.IsGait(Now);
 const double Length=Bank.Seconds(Now);
 const FVector Forward=World.TransformVectorNoScale(FVector::YAxisVector);
 double Time=0;
 if(bGait)
 {
  // The phase is how far the root has gone, in whichever direction it has gone, so a slower walk is a slower clip.
  const double Way=FVector::DotProduct(Delta,Forward)>=0?1.:-1.;
  GaitU=FMath::Frac(GaitU+Way*Moved/FMath::Max(1.,Bank.GaitCycleCm(Now)));
  Time=GaitU*Length;
 }
 else
 {
  ClipTime+=Dt;
  if(Now==Stand||Now==ES47Clip::Frustrated)Time=FMath::Fmod(ClipTime,FMath::Max(.01,Length));
  else{Time=FMath::Min(ClipTime,Length);bActDone=ClipTime>=Length;}
 }
 // The angry clip stamps forward across the ground, and its acts here keep the body on the spot: it stands, and only its
 // back, arms and head are angry.
 const bool bUpperOnly=Now==ES47Clip::Angry;
 const ES47Clip Base=bUpperOnly?Stand:Now;
 const double BaseLength=Bank.Seconds(Base);
 const double BaseTime=bUpperOnly?FMath::Fmod(ClipTime,FMath::Max(.01,BaseLength)):Time;
 Bank.Sample(Base,BaseTime,Local);
 const int32 Spine=Bone(TEXT("spine_01"));
 if(bUpperOnly&&Spine>=0)
 {
  TArray<FTransform> Upper;
  Bank.Sample(Now,Time,Upper);
  for(int32 I=0;I<Local.Num();++I)for(int32 Up=I;Up>=0;Up=R.Parents[Up])if(Up==Spine){Local[I]=Upper[I];break;}
 }
 // Into component space, with the clip's travel taken out in a straight line: root and pose together give the clip's own path.
 TArray<FTransform> P=Local;
 for(int32 I=0;I<P.Num();++I)if(R.Parents[I]>=0)P[I]=P[I]*P[R.Parents[I]];
 const auto Below=[&](int32 I,int32 A){if(A<0)return false;for(int32 K=I;K>=0;K=R.Parents[K])if(K==A)return true;return false;};
 const FVector Progress=Bank.StartOffset(Base)+(BaseLength>0?Bank.Travel(Base)*(BaseTime/BaseLength):FVector::ZeroVector);
 for(int32 I=0;I<P.Num();++I)if(Below(I,Pelvis))P[I].AddToTranslation(-Progress);

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

 // Feet: on the ground in the walk they stay where they landed, whatever the root does, and let go softly.
 FCollisionQueryParams Query(SCENE_QUERY_STAT(S47ClipFootPlant),true);
 TArray<USceneComponent*> Children;
 if(USceneComponent* Parent=C->GetAttachParent()){Parent->GetChildrenComponents(true,Children);if(auto* Primitive=Cast<UPrimitiveComponent>(Parent))Query.AddIgnoredComponent(Primitive);}
 for(auto* Child:Children)if(auto* Primitive=Cast<UPrimitiveComponent>(Child))Query.AddIgnoredComponent(Primitive);
 Query.AddIgnoredComponent(C);
 const auto Grounded=[&](FVector At)
 {
  FHitResult Hit;
  if(C->GetWorld()&&C->GetWorld()->LineTraceSingleByChannel(Hit,FVector(At.X,At.Y,Root.Z+25),FVector(At.X,At.Y,Root.Z-35),ECC_Visibility,Query)&&FMath::Abs(Hit.ImpactPoint.Z-Root.Z)<=18)
   At.Z+=Hit.ImpactPoint.Z-Root.Z;
  return At;
 };
 const FQuat BodyQ=World.GetRotation();
 for(int32 S=0;S<2;++S)
 {
  const FVector Clip=World.TransformPosition(P[Ankle[S]].GetLocation());
  const FQuat ClipQ=P[Ankle[S]].GetRotation();
  // Lengths as this pose has them, in component centimetres: a bone's own offset is in the root's scale.
  const double Upper=(P[Knee[S]].GetLocation()-P[Hip[S]].GetLocation()).Size(),Lower=(P[Ankle[S]].GetLocation()-P[Knee[S]].GetLocation()).Size();
  FPlant& Foot=Plants[S];
  FVector Target=Clip;
  FQuat TargetQ=ClipQ;
  bool bBend=false;
  if(Base==Stand)
  {
   // Standing: a foot stays where it is while the body turns and the clip sways, and steps when it has been left behind,
   // one at a time. One coming down from the walk keeps its place; the one in the air fades to the clip's.
   const FQuat WantQ=(BodyQ*ClipQ).GetNormalized();
   if(!Foot.bPlanted){Foot.bPlanted=true;Foot.Anchor=Clip+Foot.Slip;Foot.AnchorQ=WantQ;Foot.StepTime=-1;}
   else if(!Foot.bHasTurn)Foot.AnchorQ=WantQ;
   Foot.bHasTurn=true;
   Foot.Ramp=1;
   const bool bOtherStep=Plants[1-S].StepTime>=0;
   if(Foot.StepTime<0&&!bOtherStep&&((Foot.Anchor-Clip).Size()>(Upper+Lower)*.13||Foot.AnchorQ.AngularDistance(WantQ)>FMath::DegreesToRadians(14.)))
   {Foot.StepTime=0;Foot.StepFrom=Foot.Anchor;Foot.StepFromQ=Foot.AnchorQ;}
   if(Foot.StepTime>=0)
   {
    Foot.StepTime+=Dt;
    const double U=ClipEase(Foot.StepTime/.26);
    const FVector Landing=Grounded(Clip);
    Target=FMath::Lerp(Foot.StepFrom,Landing,U);Target.Z+=FMath::Sin(U*PI)*(Upper+Lower)*.035;
    TargetQ=FQuat::Slerp(Foot.StepFromQ,WantQ,U);
    if(Foot.StepTime>=.26){Foot.Anchor=Landing;Foot.AnchorQ=WantQ;Foot.StepTime=-1;Target=Landing;TargetQ=WantQ;if(S==0)Out.bLeft=true;else Out.bRight=true;}
   }
   else{Target=Foot.Anchor;TargetQ=Foot.AnchorQ;}
   TargetQ=(BodyQ.Inverse()*TargetQ).GetNormalized();
   Foot.Slip=Target-Clip;
   bBend=true;
  }
  else if(bGait&&bWalking&&Bank.GaitStance(Now,S,GaitU))
  {
   if(!Foot.bPlanted||Foot.StepTime>=0)
   {
    Foot.bPlanted=true;Foot.StepTime=-1;Foot.Anchor=Grounded(Clip);Foot.Ramp=0;
    if(S==0)Out.bLeft=true;else Out.bRight=true;
   }
   Foot.bHasTurn=false;
   Foot.Ramp=FMath::Min(1.,Foot.Ramp+Dt/.08);
   Target=FMath::Lerp(Clip+Foot.Slip,Foot.Anchor,ClipEase(Foot.Ramp));
   // Too far behind to stay down: it lets go and catches up with the clip.
   if((Foot.Anchor-World.TransformPosition(P[Hip[S]].GetLocation())).Size()>(Upper+Lower)*1.02){Foot.bPlanted=false;Target=Clip+Foot.Slip;}
   Foot.Slip=Target-Clip;
   bBend=true;
  }
  else
  {
   Foot.bPlanted=false;Foot.StepTime=-1;
   Foot.Slip*=FMath::Exp(-Dt/.12);
   Target=Clip+Foot.Slip;
   bBend=Foot.Slip.SizeSquared()>.01;
  }
  Out.AtUE[S]=Target;
  if(!bBend)continue;
  const FVector TargetCS=World.InverseTransformPosition(Target);
  const FVector Start=P[Hip[S]].GetLocation(),ToFoot=TargetCS-Start;
  const FVector Along=ToFoot.IsNearlyZero()?FVector(0,0,-1):ToFoot.GetSafeNormal();
  const double Reach=FMath::Clamp(ToFoot.Size(),FMath::Abs(Upper-Lower)+.01,Upper+Lower-.01);
  const FVector Middle=(P[Hip[S]].GetLocation()+P[Ankle[S]].GetLocation())*.5;
  const FVector Prefer=(P[Knee[S]].GetLocation()-Middle).GetSafeNormal()+FVector::YAxisVector*.25;
  FVector Bend=(Prefer-Along*FVector::DotProduct(Prefer,Along)).GetSafeNormal();
  if(Bend.IsNearlyZero())Bend=FVector::YAxisVector;
  const double Along0=(Upper*Upper-Lower*Lower+Reach*Reach)/(2*Reach);
  const FVector Bent=Start+Along*Along0+Bend*FMath::Sqrt(FMath::Max(0.,Upper*Upper-Along0*Along0));
  Aim(Hip[S],Knee[S],Bent);Aim(Knee[S],Ankle[S],Start+Along*Reach);
  // The whole foot as asked, not as the leg left it.
  Rotate(Ankle[S],(TargetQ*P[Ankle[S]].GetRotation().Inverse()).GetNormalized());
 }

 // The head keeps turning to what it looks at, less while an act has its own idea of that; every so often it slowly tips.
 const bool bAct=In.Act<ES47Clip::Num;
 LookYaw=ClipFollow(LookYaw,FMath::Clamp(In.HeadYaw,-1.05,1.05),Dt,8.);
 LookPitch=ClipFollow(LookPitch,FMath::Clamp(In.HeadPitch,-.4,.4),Dt,6.);
 LookWeight=ClipFollow(LookWeight,bAct?.35:1.,Dt,6.);
 const double TipSeed=FMath::FloorToDouble(In.Clock/2.7);
 LookTilt=ClipFollow(LookTilt,14.*(FMath::Frac(FMath::Sin(TipSeed*12.9898)*43758.5453)-.5),Dt,3.);
 if(Neck>=0)Rotate(Neck,FQuat(FVector::UpVector,LookYaw*LookWeight*.25));
 Rotate(HeadBone,FQuat(FVector::UpVector,LookYaw*LookWeight*(Neck>=0?.75:1.)));
 Rotate(HeadBone,FQuat(FVector::XAxisVector,LookPitch*LookWeight));
 Rotate(HeadBone,FQuat(FVector::YAxisVector,FMath::DegreesToRadians(LookTilt*LookWeight)));

 C->BoneSpaceTransforms.SetNum(P.Num());
 for(int32 I=0;I<P.Num();++I)C->BoneSpaceTransforms[I]=R.Parents[I]<0?P[I]:P[I].GetRelativeTransform(P[R.Parents[I]]);
 if(FadeLeft>0&&FadeFrom.Num()==P.Num())
 {
  FadeLeft=FMath::Max(0.,FadeLeft-Dt);
  const float Weight=float(1.-ClipEase(1.-FadeLeft/FadeSeconds));
  for(int32 I=0;I<P.Num();++I)
  {
   FTransform& To=C->BoneSpaceTransforms[I];const FTransform& From=FadeFrom[I];
   To.SetRotation(FQuat::Slerp(To.GetRotation(),From.GetRotation(),Weight).GetNormalized());
   To.SetTranslation(FMath::Lerp(To.GetTranslation(),From.GetTranslation(),double(Weight)));
  }
 }
 C->MarkRefreshTransformDirty();C->RefreshBoneTransforms();bInit=true;
 if(FadeLeft<=0)for(int32 S=0;S<2;++S)if(Plants[S].bPlanted&&Plants[S].Ramp>=1.&&Plants[S].StepTime<0)
  Out.MaxPlantErrorCm=FMath::Max(Out.MaxPlantErrorCm,(C->GetBoneTransformByName(R.Names[Ankle[S]],EBoneSpaces::WorldSpace).GetLocation()-Plants[S].Anchor).Size());
 Out.RotationUE[0]=Out.RotationUE[1]=BodyQ;
}
