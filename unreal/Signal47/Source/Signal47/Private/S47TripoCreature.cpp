#include "S47TripoCreature.h"
#include "S47Web.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"
#include "Misc/CommandLine.h"

bool FS47TripoCreature::Build(AS47Web* W, USceneComponent* Parent, const FString& Name)
{
 if (Ready()) return true;
 if (!W || !Parent || FParse::Param(FCommandLine::Get(), TEXT("S47TripoNpcsOff"))) return false;
 auto* Asset = LoadObject<USkeletalMesh>(nullptr,
  TEXT("/Game/Licensed/TripoNPC/Animated/SK_Tripo_Pale.SK_Tripo_Pale"), nullptr, LOAD_NoWarn | LOAD_Quiet);
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
 UE_LOG(LogTemp,Display,TEXT("S47: animated Tripo pale body %s ready, %d original skin bones"), *Name, Rest.Num());
 return true;
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
