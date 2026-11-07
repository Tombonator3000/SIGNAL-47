#include "S47TripoCreature.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Web.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"
#include "Misc/CommandLine.h"

namespace
{
 TArray<TWeakObjectPtr<UStaticMeshComponent>> AllGlints;
 // Rest position of a dot in the head bone's frame, the bone-frame directions of the body's sideways and up axes, and a
 // seed shared by both eyes of one creature so they look the same way together.
 struct FGlintRest { FVector Base,Side,Up; uint32 Seed; };
 TMap<TWeakObjectPtr<UStaticMeshComponent>,FGlintRest> GlintRest;
 uint32 NextGlintSeed=1;
 // A repeatable 0..1 number from a seed and a time slot.
 double Rand01(uint32 Seed,int64 Slot,uint32 Salt){return double((HashCombineFast(HashCombineFast(Seed,GetTypeHash(Slot)),Salt*2654435761u)>>8)&0xFFFFu)/65536.;}
}
bool FS47TripoCreature::bTestGlintsAlways=false;
void FS47TripoCreature::StepEyeGlints(const FVector& EyeUE, double Daylight)
{
 // A dot of a fixed size falls under one pixel past a few metres, so it holds a small width on screen. It belongs
 // to the dark and to near distances: gone by day and fully out at 50 m, fading from 30 m.
 static constexpr double PerMetre=.0018,FadeFrom=30.,GoneAt=50.,Glow=3.;
 const double Dark=bTestGlintsAlways?1.:1.-FMath::SmoothStep(.02,.25,Daylight);
 AllGlints.RemoveAll([](const TWeakObjectPtr<UStaticMeshComponent>& G){return !G.IsValid();});
 for(auto It=GlintRest.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
 for(const auto& Weak:AllGlints)if(auto* G=Weak.Get())
 {
  const double Metres=FVector::Dist(EyeUE,G->GetComponentLocation())/100.;
  const double Level=Dark*(1.-FMath::SmoothStep(FadeFrom,GoneAt,Metres));
  const bool bShow=Level>.01;
  if(G->bHiddenInGame==bShow)G->SetHiddenInGame(!bShow);
  if(!bShow||!G->IsVisible())continue;
  double Size=FMath::Max(EyeSize,Metres*PerMetre);
  // A gaze: now and then a sudden small jump of both dots inside the eye, and a rare quick blink. Not needed past 12 m.
  if(const FGlintRest* R=GlintRest.Find(G))
  {
   FVector At=R->Base;
   if(Metres<12.&&G->GetWorld())
   {
    const double T=G->GetWorld()->GetTimeSeconds();
    const int64 Look=int64(FMath::FloorToDouble(T/(.7+1.3*Rand01(R->Seed,0,1))));
    const double Reach=.35*(1.-FMath::SmoothStep(8.,12.,Metres));
    At+=(R->Side*(Rand01(R->Seed,Look,2)*2.-1.)+R->Up*(Rand01(R->Seed,Look,3)*2.-1.))*Reach;
    const double Gap=7.,InGap=FMath::Fmod(T,Gap);const int64 Round=int64(FMath::FloorToDouble(T/Gap));
    if(Rand01(R->Seed,Round,4)<.7&&InGap>Gap*.15+Rand01(R->Seed,Round,5)*Gap*.7&&InGap<Gap*.15+Rand01(R->Seed,Round,5)*Gap*.7+.13)Size*=.08;
   }
   if(!At.Equals(G->GetRelativeLocation(),KINDA_SMALL_NUMBER))G->SetRelativeLocation(At);
  }
  G->SetWorldScale3D(FVector(Size));
  if(auto* Mat=Cast<UMaterialInstanceDynamic>(G->GetMaterial(0)))Mat->SetVectorParameterValue(TEXT("Emissive"),FLinearColor(1,1,1)*float(Glow*Level));
 }
}

int32 FS47TripoCreature::ShownEyeGlints()
{
 int32 N=0;
 for(const auto& Weak:AllGlints)if(auto* G=Weak.Get())if(G->IsVisible()&&!G->bHiddenInGame)++N;
 return N;
}

// A pinprick of light in each big black eye: it reads in the dark before the rest of the face does. Children of the
// head bone, so they turn with it and hide with the body.
void FS47TripoCreature::AddEyeGlints(AS47Web* W, UPoseableMeshComponent* C)
{
 auto* Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 if(!Sphere||!Base)return;
 const FTransform Head=C->GetBoneTransform(C->GetBoneIndex(TEXT("head")),FTransform::Identity);
 const uint32 Seed=NextGlintSeed++*7919u;
 for(int32 I=0;I<2;++I)
 {
  auto* Mat=UMaterialInstanceDynamic::Create(Base,W);Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor::Black);Mat->SetVectorParameterValue(TEXT("Emissive"),FLinearColor(1,1,1)*3.f);Mat->SetScalarParameterValue(TEXT("EmissivePower"),1);
  auto* G=NewObject<UStaticMeshComponent>(W,*FString::Printf(TEXT("%sGlint%d"),*C->GetName(),I));
  G->SetMobility(EComponentMobility::Movable);G->SetupAttachment(C,TEXT("head"));G->SetStaticMesh(Sphere);G->SetMaterial(0,Mat);
  G->SetCollisionEnabled(ECollisionEnabled::NoCollision);G->SetCastShadow(false);G->SetVisibility(false);
  // A point seen, not a lamp: it gives no light to what is around it.
  G->bAffectDynamicIndirectLighting=false;G->bAffectDistanceFieldLighting=false;G->SetVisibleInRayTracing(false);
  G->RegisterComponent();
  // Component space (the body faces +Y, Z up) into the head bone's own frame.
  const FVector Target=Head.GetLocation()+FVector(I?EyeAt.X:-EyeAt.X,EyeAt.Y,EyeAt.Z);
  G->SetRelativeLocation(Head.InverseTransformPosition(Target));
  G->SetWorldScale3D(FVector(EyeSize));
  Glints.Add(G);AllGlints.Add(G);
  GlintRest.Add(G,{G->GetRelativeLocation(),Head.InverseTransformVector(FVector(1,0,0)),Head.InverseTransformVector(FVector(0,0,1)),Seed});
 }
}

bool FS47TripoCreature::Build(AS47Web* W, USceneComponent* Parent, const FString& Name, const TCHAR* AssetPath)
{
 if (Ready()) return true;
 if (!W || !Parent || FParse::Param(FCommandLine::Get(), TEXT("S47TripoNpcsOff"))) return false;
 auto* Asset = LoadObject<USkeletalMesh>(nullptr, AssetPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
 // Licensed lies outside Git: without the new body, the old one in its place.
 if (!Asset)
 {
  const TCHAR* const Fallback=FCString::Stristr(AssetPath,TEXT("SK_Tripo_PaleOne"))||FCString::Stristr(AssetPath,TEXT("SK_Tripo_TallOne"))?S47KindBody::Old:FCString::Stristr(AssetPath,TEXT("SK_Tripo_Roswell2"))?S47KindBody::OldRoswell:nullptr;
  if(Fallback)
  {
   static TSet<FString> Said;
   if(!Said.Contains(FString(AssetPath))){Said.Add(FString(AssetPath));UE_LOG(LogTemp,Display,TEXT("S47: %s is not in this build, wearing %s"),AssetPath,Fallback);}
   Asset = LoadObject<USkeletalMesh>(nullptr, Fallback, nullptr, LOAD_NoWarn | LOAD_Quiet);
  }
 }
 if (!Asset) return false;
 const FString Worn = Asset->GetName();
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
 if (Worn==TEXT("SK_Tripo_Roswell")) AddEyeGlints(W,C);
 else if (Worn==TEXT("SK_Tripo_PaleOne")||Worn==TEXT("SK_Tripo_TallOne")||Worn==TEXT("SK_Tripo_Roswell2"))
 {
  const bool bPale=Worn==TEXT("SK_Tripo_PaleOne"),bTall=Worn==TEXT("SK_Tripo_TallOne");
  EyeAt=bPale?S47KindBody::PaleOneEyeAt:bTall?S47KindBody::TallOneEyeAt:S47KindBody::RoswellEyeAt;AddEyeGlints(W,C);
  // The climb and the jump take the bones far from the root.
  C->BoundsScale=2.f;
  Clips=FS47ClipBank::For(Asset,*this,bPale?S47KindBody::PaleOneClips:bTall?S47KindBody::TallOneClips:S47KindBody::RoswellClips);
 }
 UE_LOG(LogTemp,Display,TEXT("S47: animated Tripo body %s ready, %d original skin bones, %s"), *Name, Rest.Num(), AssetPath);
 return true;
}

void FS47TripoCreature::ApplyCrab(const FS47CrabPose& Pose)
{
 if(!Mesh.IsValid())return;
 if(!CrabMotion)CrabMotion=MakeShared<FS47CrabMotion>();
 CrabContacts=CrabMotion->Apply(*this,Pose);
}

void FS47TripoCreature::Peek(bool bShow, double Exposure, double HeadTurn, double Clock)
{
 auto* C=Mesh.Get(); if (!C) return;
 C->SetVisibility(bShow,true); if (!bShow) return;
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
  const int32 Chest=Bone(TEXT("spine_03"));
  const double Outward=Arm>=0 && Chest>=0?FMath::Sign(Pose[Arm].GetLocation().X-Pose[Chest].GetLocation().X):1.;
  if (Arm>=0 && Elbow>=0) Aim(Arm,Elbow,Pose[Arm].GetLocation()+FVector(Outward*12.,3.,-26.));
  if (Elbow>=0 && Hand>=0) Aim(Elbow,Hand,Pose[Elbow].GetLocation()+FVector(Outward*4.,7.+Duck*.35,-20.));
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
 C->SetVisibility(bShow,true); if (!bShow) return;
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

void FS47TripoCreature::PlayClips(const FS47ClipPose& Pose)
{
 if(!Clips)return;
 if(!ClipMotion)ClipMotion=MakeShared<FS47ClipMotion>();
 Contacts=FS47PaleContacts();
 ClipMotion->Apply(*this,*Clips,Pose,Contacts);
}

void FS47TripoCreature::ApplyPale(const FS47PalePose& Pose)
{
 auto* C=Mesh.Get();
 const bool bRunClip=Clips.IsValid()&&Pose.Mode==ES47PaleMotion::Run&&Clips->Has(ES47Clip::Run);
 const bool bClip=Clips.IsValid()&&Pose.bVisible&&(Pose.Mode!=ES47PaleMotion::Run||bRunClip)&&Pose.Mode!=ES47PaleMotion::Grab;
 if(bClip)
 {
  if(!ClipMotion)ClipMotion=MakeShared<FS47ClipMotion>();
  if(bOnProcedural){bOnProcedural=false;PaleMotion.Reset();ClipMotion->FadeFromShown(C);}
  // A body with a run of its own goes at this mode's pace (under 2 m/s) on its walk: its run was made for 6 m/s.
  FS47ClipPose Clip;Clip.Clock=Pose.Clock;Clip.HeadYaw=Pose.HeadYaw;Clip.Act=Pose.Act;Clip.Stand=Pose.Stand;
  PlayClips(Clip);
  if(Contacts.bValid)return;
 }
 else if(Clips.IsValid()&&C)
 {
  if(!Pose.bVisible){bOnProcedural=false;ProcFadeLeft=0;if(ClipMotion)*ClipMotion=FS47ClipMotion();}
  // Out of the clips and into a run or a grab: from the pose it was in.
  else if(!bOnProcedural)BeginProcFade(Pose.Clock);
 }
 if(!PaleMotion)PaleMotion=MakeShared<FS47PaleMotion>();
 Contacts=PaleMotion->Apply(*this,Pose);
 if(!Contacts.bValid)Apply(Pose.bVisible,Pose.HeadYaw,Pose.LegacyGait,Pose.LegacyPhase,Pose.Clock,
  Pose.Mode==ES47PaleMotion::Grab,Pose.Mode==ES47PaleMotion::Listen);
 if(Pose.bVisible)StepProcFade(Pose.Clock);
}

void FS47TripoCreature::BeginProcFade(double Clock)
{
 auto* C=Mesh.Get();if(!C)return;
 bOnProcedural=true;ProcFadeFrom=C->BoneSpaceTransforms;ProcFadeLeft=.3;ProcFadeClock=Clock;
}

void FS47TripoCreature::StepProcFade(double Clock)
{
 auto* C=Mesh.Get();
 if(ProcFadeLeft<=0||!C||ProcFadeFrom.Num()!=C->BoneSpaceTransforms.Num())return;
 ProcFadeLeft=FMath::Max(0.,ProcFadeLeft-FMath::Clamp(Clock-ProcFadeClock,0.,.1));ProcFadeClock=Clock;
 const double Keep=ProcFadeLeft/.3;
 for(int32 I=0;I<ProcFadeFrom.Num();++I)
 {
  FTransform& To=C->BoneSpaceTransforms[I];const FTransform& From=ProcFadeFrom[I];
  To.SetRotation(FQuat::Slerp(To.GetRotation(),From.GetRotation(),Keep).GetNormalized());
  To.SetTranslation(FMath::Lerp(To.GetTranslation(),From.GetTranslation(),Keep));
 }
 C->MarkRefreshTransformDirty();C->RefreshBoneTransforms();
}
