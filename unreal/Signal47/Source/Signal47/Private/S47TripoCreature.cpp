#include "S47TripoCreature.h"
#include "S47Web.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"
#include "Misc/CommandLine.h"

bool FS47TripoCreature::Build(AS47Web* W, USceneComponent* Parent, const FString& Name, const TCHAR* AssetPath)
{
 if (Ready()) return true;
 if (!W || !Parent || FParse::Param(FCommandLine::Get(), TEXT("S47TripoNpcsOff"))) return false;
 auto* Asset = LoadObject<USkeletalMesh>(nullptr, AssetPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
 if (!Asset) return false;
 const FReferenceSkeleton& Skeleton = Asset->GetRefSkeleton();
 for (const TCHAR* Bone : {TEXT("pelvis"), TEXT("head"), TEXT("upperarm_l"), TEXT("upperarm_r"), TEXT("thigh_l"), TEXT("thigh_r")})
  if (Skeleton.FindBoneIndex(Bone) == INDEX_NONE) return false;
 Rest = Skeleton.GetRefBonePose(); Parents.Reset(); Names.Reset();
 for (int32 I=0; I<Rest.Num(); ++I) { Parents.Add(Skeleton.GetParentIndex(I)); Names.Add(Skeleton.GetBoneName(I)); }
 auto* C = NewObject<UPoseableMeshComponent>(W, *Name);
 W->AddInstanceComponent(C); C->SetMobility(EComponentMobility::Movable);
 C->SetupAttachment(Parent); C->SetSkinnedAssetAndUpdate(Asset);
 // Prepared FBX faces +Y, whereas the threat's feet-root faces +X.
 C->SetRelativeRotation(FRotator(0,-90,0));
 C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetGenerateOverlapEvents(false);
 C->SetCanEverAffectNavigation(false); C->SetCastShadow(true); C->BoundsScale=1.35f;
 C->SetVisibility(false); C->RegisterComponent(); Mesh=C;
 UE_LOG(LogTemp,Display,TEXT("S47: animated Tripo body %s ready, %d original skin bones, %s"), *Name, Rest.Num(), AssetPath);
 return true;
}

void FS47TripoCreature::Peek(bool bShow, double Exposure, double HeadTurn, double Clock)
{
 auto* C=Mesh.Get(); if (!C) return;
 C->SetVisibility(bShow); if (!bShow) return;
 TArray<FTransform> Pose=Rest;
 for (int32 I=0; I<Pose.Num(); ++I) if (Parents[I]>=0) Pose[I]=Pose[I]*Pose[Parents[I]];
 const auto Descends=[&](int32 Child,int32 Root)
 { for (int32 P=Child; P>=0; P=Parents[P]) if (P==Root) return true; return false; };
 const auto Rotate=[&](int32 Bone,const FQuat& Q)
 {
  if (Bone<0) return;
  const FVector Pivot=Pose[Bone].GetLocation();
  for (int32 I=0; I<Pose.Num(); ++I) if (Descends(I,Bone))
  {
   Pose[I].SetLocation(Pivot+Q.RotateVector(Pose[I].GetLocation()-Pivot));
   Pose[I].SetRotation((Q*Pose[I].GetRotation()).GetNormalized());
  }
 };
 const auto Bone=[&](const TCHAR* Name) { return Names.IndexOfByKey(FName(Name)); };
 const auto Aim=[&](int32 A,int32 B,const FVector& Target)
 {
  if (A>=0 && B>=0) Rotate(A,FQuat::FindBetweenNormals(
   (Pose[B].GetLocation()-Pose[A].GetLocation()).GetSafeNormal(),(Target-Pose[A].GetLocation()).GetSafeNormal()));
 };
 const int32 Pelvis=Bone(TEXT("pelvis"));
 const double Duck=(1.-FMath::Clamp(Exposure,0.,1.))*S47Roswell::DuckM*100.;
 // Save the feet before lowering the pelvis. Two-bone legs then bend towards
 // +Y, with the original foot contact and orientation rather than floor sinking.
 for (int32 Side=0; Side<2; ++Side)
 {
  const int32 Hip=Bone(Side==0?TEXT("thigh_l"):TEXT("thigh_r"));
  const int32 Knee=Bone(Side==0?TEXT("calf_l"):TEXT("calf_r"));
  const int32 Foot=Bone(Side==0?TEXT("foot_l"):TEXT("foot_r"));
  if (Hip<0 || Knee<0 || Foot<0) continue;
  const FVector Target=Pose[Foot].GetLocation(); const FQuat FootQ=Pose[Foot].GetRotation();
  const double Upper=(Pose[Knee].GetLocation()-Pose[Hip].GetLocation()).Size();
  const double Lower=(Pose[Foot].GetLocation()-Pose[Knee].GetLocation()).Size();
  for (int32 I=0; I<Pose.Num(); ++I) if (Descends(I,Hip)) Pose[I].AddToTranslation(FVector(0,0,-Duck));
  const FVector H=Pose[Hip].GetLocation(), To=Target-H;
  const double Dist=FMath::Clamp(To.Size(),FMath::Abs(Upper-Lower)+.001,Upper+Lower-.001);
  const FVector Along=To.GetSafeNormal();
  const FVector Bend=(FVector::YAxisVector-Along*FVector::DotProduct(Along,FVector::YAxisVector)).GetSafeNormal();
  const double A=(Upper*Upper-Lower*Lower+Dist*Dist)/(2.*Dist);
  const FVector K=H+Along*A+Bend*FMath::Sqrt(FMath::Max(0.,Upper*Upper-A*A));
  Aim(Hip,Knee,K); Aim(Knee,Foot,Target);
  Rotate(Foot,(FootQ*Pose[Foot].GetRotation().Inverse()).GetNormalized());
 }
 // Lower the upper body exactly as far as its hips. The leg transforms above
 // are already in the crouched component-space pose, and must not move twice.
 if (Pelvis>=0) for (int32 I=0; I<Pose.Num(); ++I) if (Descends(I,Pelvis))
 {
  const int32 L=Bone(TEXT("thigh_l")), R=Bone(TEXT("thigh_r"));
  if (!Descends(I,L) && !Descends(I,R)) Pose[I].AddToTranslation(FVector(0,0,-Duck));
 }
 for (int32 Side=0; Side<2; ++Side)
 {
  const int32 Arm=Bone(Side==0?TEXT("upperarm_l"):TEXT("upperarm_r"));
  const int32 Elbow=Bone(Side==0?TEXT("lowerarm_l"):TEXT("lowerarm_r"));
  const int32 Hand=Bone(Side==0?TEXT("hand_l"):TEXT("hand_r"));
  if (Arm>=0 && Elbow>=0) Aim(Arm,Elbow,Pose[Arm].GetLocation()+FVector(Side==0?-4.:4.,3.,-30.));
  if (Elbow>=0 && Hand>=0) Aim(Elbow,Hand,Pose[Elbow].GetLocation()+FVector(0,7.+Duck*.35,-20.));
 }
 Rotate(Bone(TEXT("head")),FQuat(FVector::UpVector,FMath::Clamp(HeadTurn,-.65,.65)));
 Rotate(Bone(TEXT("head")),FQuat(FVector::YAxisVector,FMath::DegreesToRadians(3.*FMath::Sin(Clock*.7))));
 C->BoneSpaceTransforms.SetNum(Pose.Num());
 for (int32 I=0; I<Pose.Num(); ++I) C->BoneSpaceTransforms[I]=Parents[I]<0?Pose[I]:Pose[I].GetRelativeTransform(Pose[Parents[I]]);
 C->MarkRefreshTransformDirty(); C->RefreshBoneTransforms();
}

bool FS47TripoCreature::Rendered() const
{
 const auto* C=Mesh.Get(); return C && C->IsRegistered() && C->IsVisible() && C->WasRecentlyRendered(.25f);
}

void FS47TripoCreature::Apply(bool bShow, double HeadTurn, double Gait, double Phase, double Clock, bool bGrab, bool bListen)
{
 auto* C=Mesh.Get(); if (!C) return;
 C->SetVisibility(bShow); if (!bShow) return;
 TArray<FTransform> Pose=Rest;
 for (int32 I=0; I<Pose.Num(); ++I) if (Parents[I]>=0) Pose[I]=Pose[I]*Pose[Parents[I]];
 const auto Descends=[&](int32 Child,int32 Root)
 { for (int32 P=Child; P>=0; P=Parents[P]) if (P==Root) return true; return false; };
 const auto Rotate=[&](int32 Bone,const FQuat& Q)
 {
  if (Bone<0) return;
  const FVector Pivot=Pose[Bone].GetLocation();
  for (int32 I=Bone; I<Pose.Num(); ++I) if (Descends(I,Bone))
  {
   Pose[I].SetLocation(Pivot+Q.RotateVector(Pose[I].GetLocation()-Pivot));
   Pose[I].SetRotation((Q*Pose[I].GetRotation()).GetNormalized());
  }
 };
 const auto Aim=[&](const TCHAR* Start,const TCHAR* End,const FVector& Target)
 {
  const int32 A=Names.IndexOfByKey(FName(Start)), B=Names.IndexOfByKey(FName(End));
  if (A>=0 && B>=0) Rotate(A,FQuat::FindBetweenNormals((Pose[B].GetLocation()-Pose[A].GetLocation()).GetSafeNormal(),Target.GetSafeNormal()));
 };
 const double A=FMath::Clamp(Gait,0.,1.4), Swing=FMath::Sin(Phase);
 // Component-space directions follow the imported +Y forward, Z-up skeleton.
 // Skin weights deform the continuous original mesh, rather than moving rigid parts.
 Rotate(Names.IndexOfByKey(FName(TEXT("spine_01"))),FQuat(FVector::XAxisVector,FMath::DegreesToRadians(-(5.+5.*A+(bListen?4.:0.)))));
 for (int32 Side=0; Side<2; ++Side)
 {
  const bool Left=Side==0; const double Sign=Left?-1.:1., Stride=Left?Swing:-Swing;
  Aim(Left?TEXT("thigh_l"):TEXT("thigh_r"),Left?TEXT("calf_l"):TEXT("calf_r"),FVector(Sign*.025,Stride*.43*A,-1));
  Aim(Left?TEXT("calf_l"):TEXT("calf_r"),Left?TEXT("foot_l"):TEXT("foot_r"),FVector(0,-FMath::Max(0.,-Stride)*.55*A,-1));
  Aim(Left?TEXT("upperarm_l"):TEXT("upperarm_r"),Left?TEXT("lowerarm_l"):TEXT("lowerarm_r"),
   bGrab?FVector(Sign*.12,1,-.12):FVector(Sign*.13,-Stride*.32*A,-1));
  Aim(Left?TEXT("lowerarm_l"):TEXT("lowerarm_r"),Left?TEXT("hand_l"):TEXT("hand_r"),
   bGrab?FVector(Sign*.05,1,-.02):FVector(Sign*.025,.15-Stride*.18*A,-1));
 }
 Rotate(Names.IndexOfByKey(FName(TEXT("head"))),FQuat(FVector::UpVector,FMath::Clamp(HeadTurn,-1.15,1.15)));
 const double Bob=A*.7*(1.-FMath::Cos(Phase*2))+.18*FMath::Sin(Clock*1.3);
 for (FTransform& Bone:Pose) Bone.AddToTranslation(FVector(0,0,Bob));
 C->BoneSpaceTransforms.SetNum(Pose.Num());
 for (int32 I=0; I<Pose.Num(); ++I) C->BoneSpaceTransforms[I]=Parents[I]<0?Pose[I]:Pose[I].GetRelativeTransform(Pose[Parents[I]]);
 C->MarkRefreshTransformDirty(); C->RefreshBoneTransforms();
}
