#include "S47PaleBody.h"
#include "S47Web.h"
#include "S47Three.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47PaleBody, Log, All);

namespace
{
 double PaleWrap(double A)
 {
  if (!FMath::IsFinite(A)) return 0;
  while (A > PI) A -= 2 * PI;
  while (A < -PI) A += 2 * PI;
  return A;
 }
}

bool FS47PaleBody::Build(AS47Web* W, const TCHAR* Group, const TCHAR* Prefix)
{
 if (bBuilt) return true;
 if (bFailed || !W || !W->IsLoaded()) return false;
 // "#root": on the web actor itself, outside the scene's groups, so no area hides it (the developer
 // events' body); its visibility is then only Apply's.
 const bool bLoose = FCString::Strcmp(Group, TEXT("#root")) == 0;
 const int32 GroupIndex = bLoose ? -1 : W->Handle(Group);
 const FS47WebNode* GroupNode = bLoose ? nullptr : W->Node(GroupIndex);
 USceneComponent* GroupComp = bLoose ? W->GetRootComponent() : GroupNode ? GroupNode->Comp.Get() : nullptr;
 auto Load = [](const TCHAR* Name) { return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Signal47/HorrorSC02/%s.%s"), Name, Name)); };
 UStaticMesh* TorsoMesh = Load(TEXT("SM_SC02_PaleTorso"));
 UStaticMesh* HeadMesh = Load(TEXT("SM_SC02_PaleHead"));
 UStaticMesh* UpperArmMesh = Load(TEXT("SM_SC02_PaleUpperArm"));
 UStaticMesh* LowerArmMesh = Load(TEXT("SM_SC02_PaleLowerArm"));
 UStaticMesh* ThighMesh = Load(TEXT("SM_SC02_PaleThigh"));
 UStaticMesh* ShinMesh = Load(TEXT("SM_SC02_PaleShin"));
 if (!GroupComp || !TorsoMesh || !HeadMesh || !UpperArmMesh || !LowerArmMesh || !ThighMesh || !ShinMesh)
 {
  bFailed = true;
  UE_LOG(LogS47PaleBody, Error, TEXT("S47: pale body %s unavailable (group %s or SC02 meshes missing)"), Prefix, Group);
  return false;
 }
 UBoxComponent* R = NewObject<UBoxComponent>(W, *FString::Printf(TEXT("%sRoot"), Prefix));
 R->SetMobility(EComponentMobility::Movable); R->SetupAttachment(GroupComp);
 R->SetCollisionEnabled(ECollisionEnabled::NoCollision); R->SetBoxExtent(FVector(1)); R->RegisterComponent();
 const int32 RootNode = bLoose ? -1 : W->AdoptNode(R, GroupIndex, FString::Printf(TEXT("%s:root"), Prefix));
 if (!bLoose) Root = W->ObjAt(RootNode);
 RootComp = R;
 // The joints of FS47HorrorThreat::Build (model_contract.json): the right side is the left mesh mirrored.
 auto Part = [&](const TCHAR* Name, UStaticMesh* Mesh, USceneComponent* Parent, int32 ParentNode, const FVector& At, const FVector& Scale, int32& Node)
 {
  UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, *FString::Printf(TEXT("%s%s"), Prefix, Name));
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(Parent); C->SetStaticMesh(Mesh);
  C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore);
  C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); C->SetCastShadow(true);
  C->SetRelativeLocation(At); C->SetRelativeScale3D(Scale); C->RegisterComponent();
  Node = bLoose ? -1 : W->AdoptNode(C, ParentNode, FString::Printf(TEXT("%s:%s"), Prefix, Name));
  All.Add(C);
  return C;
 };
 const FVector One(1, 1, 1), Mirror(1, -1, 1);
 int32 TorsoNode = -1, ArmLNode = -1, ArmRNode = -1, ThighLNode = -1, ThighRNode = -1, Leaf = -1;
 UStaticMeshComponent* T = Part(TEXT("Torso"), TorsoMesh, R, RootNode, FVector(0, 0, 102), One, TorsoNode); Torso = T;
 Head = Part(TEXT("Head"), HeadMesh, T, TorsoNode, FVector(7.2, 0, 68), One, Leaf);
 UStaticMeshComponent* AL = Part(TEXT("UpperArmL"), UpperArmMesh, T, TorsoNode, FVector(4.6, -21, 58), One, ArmLNode); ArmL = AL;
 ForeL = Part(TEXT("LowerArmL"), LowerArmMesh, AL, ArmLNode, FVector(0, 0, -36), One, Leaf);
 UStaticMeshComponent* AR = Part(TEXT("UpperArmR"), UpperArmMesh, T, TorsoNode, FVector(4.6, 21, 58), Mirror, ArmRNode); ArmR = AR;
 ForeR = Part(TEXT("LowerArmR"), LowerArmMesh, AR, ArmRNode, FVector(0, 0, -36), One, Leaf);
 UStaticMeshComponent* TL = Part(TEXT("ThighL"), ThighMesh, R, RootNode, FVector(0, -10, 100), One, ThighLNode); ThighL = TL;
 ShinL = Part(TEXT("ShinL"), ShinMesh, TL, ThighLNode, FVector(0, 0, -49), One, Leaf);
 UStaticMeshComponent* TR = Part(TEXT("ThighR"), ThighMesh, R, RootNode, FVector(0, 10, 100), Mirror, ThighRNode); ThighR = TR;
 ShinR = Part(TEXT("ShinR"), ShinMesh, TR, ThighRNode, FVector(0, 0, -49), One, Leaf);
 if (Creature.Build(W,R,FString(Prefix)+TEXT("TripoSkin")))
  for (const auto& Old:All) if (auto* C=Old.Get())
  { C->SetRenderInMainPass(false); C->SetRenderInDepthPass(false); C->SetVisibleInRayTracing(false); C->SetCastShadow(false); }
 bBuilt = true;
 Apply(false, FVector::ZeroVector, 0, 0, 0, 0, 0, EPose::Walk);
 UE_LOG(LogS47PaleBody, Display, TEXT("S47: pale body %s ready: 10 parts under %s"), Prefix, Group);
 return true;
}

bool FS47PaleBody::Owns(const UPrimitiveComponent* C) const
{
 return C && All.ContainsByPredicate([C](const TWeakObjectPtr<UStaticMeshComponent>& P) { return P.Get() == C; });
}

bool FS47PaleBody::Rendered() const
{
 if (Creature.Ready()) return bBuilt && Creature.Rendered();
 const UStaticMeshComponent* T = Torso.Get();
 return bBuilt && T && T->IsVisible() && T->WasRecentlyRendered(.25f);
}

void FS47PaleBody::Apply(bool bShow, const FVector& Feet, double BodyYaw, double HeadYaw, double Gait, double Phase, double Clock, EPose Pose)
{
 if (!bBuilt) return;
 if (Root) Root.SetVisible(bShow);
 else if (UBoxComponent* R = RootComp.Get()) R->SetVisibility(bShow, true);
 Creature.Apply(bShow,PaleWrap(HeadYaw-BodyYaw),Gait,Phase,Clock,Pose==EPose::Grab,Pose==EPose::Listen);
 if (bCollide != bShow)
 {
  // Hidden parts never block a sight, floor or pick trace.
  bCollide = bShow;
  for (const auto& C : All) if (UStaticMeshComponent* P = C.Get()) P->SetCollisionEnabled(bShow ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
 }
 if (!bShow) return;
 if (UBoxComponent* R = RootComp.Get()) R->SetWorldLocationAndRotation(S47Three::ToUE(Feet.X, Feet.Y, Feet.Z), FRotator(0, FMath::RadiansToDegrees(BodyYaw), 0));
 // FS47HorrorThreat::Apply's stiff joints: positive pitch swings a hanging limb forward (+X).
 const double A = FMath::Clamp(Gait, 0.0, 1.4), Sw = FMath::Sin(Phase), Cw = FMath::Cos(Phase);
 double AL = -22 * A * Sw, AR = 22 * A * Sw, Elbow = 10 + 12 * A, Lean = -(2 + 7 * A) + 1.2 * FMath::Sin(Clock * 1.3);
 if (Pose == EPose::Listen) Lean -= 3;
 if (Pose == EPose::Grab) { AL = AR = 78; Elbow = 8; Lean = -12; }
 auto Set = [](const TWeakObjectPtr<UStaticMeshComponent>& W, double Pitch, double Yaw)
 { if (UStaticMeshComponent* C = W.Get()) C->SetRelativeRotation(FRotator(Pitch, Yaw, 0)); };
 const double Target[JointCount] = {Lean, FMath::RadiansToDegrees(PaleWrap(HeadYaw - BodyYaw)), AL, Elbow, AR, Elbow,
  26 * A * Sw, -(6 + 34 * A * FMath::Max(0.0, Cw)), -26 * A * Sw, -(6 + 34 * A * FMath::Max(0.0, -Cw))};
 const double Rate = Pose == EPose::Chase ? 11.0 : 5.0 + 4.0 * A;
 for (int32 I = 0; I < JointCount; ++I)
 {
  const int32 Tick = FMath::FloorToInt(Clock * Rate + I * .37);
  if (Tick != JointTick[I] || Pose == EPose::Grab) { JointTick[I] = Tick; JointPose[I] = Target[I]; }
 }
 Set(Torso, JointPose[0], 0);
 Set(Head, -4 - .6 * JointPose[0], JointPose[1]);
 Set(ArmL, JointPose[2], 0); Set(ForeL, JointPose[3], 0);
 Set(ArmR, JointPose[4], 0); Set(ForeR, JointPose[5], 0);
 Set(ThighL, JointPose[6], 0); Set(ShinL, JointPose[7], 0);
 Set(ThighR, JointPose[8], 0); Set(ShinR, JointPose[9], 0);
}
