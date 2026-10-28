// The Mesa Diner waitress as one MetaHuman, when the licensed local assets exist (never in Git;
// rebuilt by Tools/metahuman). Without them the original figure stays, so a clean checkout plays.
#include "S47Person.h"
#include "S47Evening.h"
#include "S47Main.h"
#include "S47Obj.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "Animation/AnimSequence.h"
#include "AnimNodes/AnimNode_CopyPoseFromMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "GroomAsset.h"
#include "GroomBindingAsset.h"
#include "GroomComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Waitress, Log, All);

namespace S47Waitress
{
 // Her parts as MetaHuman Creator assembled them. The assembled Blueprint is not used: its graph
 // sets up Live Link, which is editor tooling and does not cook.
 const TCHAR* BodyPath = TEXT("/Game/MetaHumans/MHC_Waitress/Body/SKM_MHC_Waitress_BodyMesh.SKM_MHC_Waitress_BodyMesh");
 const TCHAR* FacePath = TEXT("/Game/MetaHumans/MHC_Waitress/Face/SKM_MHC_Waitress_FaceMesh.SKM_MHC_Waitress_FaceMesh");
 // A copy of Common/Face/ABP_Face (copies pose and curves from Body) beside her animations, so the
 // cook finds it: nothing else references the original once the Blueprint is gone.
 const TCHAR* FaceAnimPath = TEXT("/Game/MetaHumans/Waitress/Anims/ABP_Face_Waitress.ABP_Face_Waitress_C");
 struct FGroomPart { const TCHAR* Name; const TCHAR* Groom; const TCHAR* Binding; };
 const FGroomPart Grooms[] = {
  {TEXT("Hair"), TEXT("/Game/MetaHumans/MHC_Waitress/Grooms/Hair_M_Layered.Hair_M_Layered"), TEXT("/Game/MetaHumans/MHC_Waitress/Grooms/Hair_M_Layered_Binding.Hair_M_Layered_Binding")},
  {TEXT("Eyebrows"), TEXT("/Game/MetaHumans/MHC_Waitress/Grooms/Eyebrows_M_SlightArch.Eyebrows_M_SlightArch"), TEXT("/Game/MetaHumans/MHC_Waitress/Grooms/Eyebrows_M_SlightArch_Binding.Eyebrows_M_SlightArch_Binding")},
  {TEXT("Eyelashes"), TEXT("/Game/MetaHumans/MHC_Waitress/Grooms/Eyelashes_L_ThickCurl.Eyelashes_L_ThickCurl"), TEXT("/Game/MetaHumans/MHC_Waitress/Grooms/Eyelashes_L_ThickCurl_Binding.Eyelashes_L_ThickCurl_Binding")},
 };
 const TCHAR* IdlePath = TEXT("/Game/MetaHumans/Waitress/Anims/AS_Waitress_Idle.AS_Waitress_Idle");
 const TCHAR* TakePath = TEXT("/Game/MetaHumans/Waitress/FaceAnims/AS_{take}.AS_{take}");
 const TCHAR* UniformPath = TEXT("/Game/MetaHumans/Waitress/Uniform/SKM_Waitress_Uniform.SKM_Waitress_Uniform");

 template <class T> T* Load(const TCHAR* Path) { return LoadObject<T>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet); }

 // The same parts the assembled Blueprint has: Body at the root, Face on Body, grooms on Face.
 AActor* Assemble(UWorld* World, const FVector& Feet)
 {
  USkeletalMesh* BodyMesh = Load<USkeletalMesh>(BodyPath);
  USkeletalMesh* FaceMesh = Load<USkeletalMesh>(FacePath);
  UClass* FaceAnim = LoadClass<UAnimInstance>(nullptr, FaceAnimPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
  if (!BodyMesh || !FaceMesh || !FaceAnim)
  {
   if (BodyMesh || FaceMesh || FaceAnim) // partly installed: say what is missing
    UE_LOG(LogS47Waitress, Warning, TEXT("S47: waitress parts missing: body %d face %d face anim %d"), !BodyMesh, !FaceMesh, !FaceAnim);
   return nullptr;
  }
  FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  AActor* A = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(Feet), Params);
  if (!A) return nullptr;
  USceneComponent* Root = NewObject<USceneComponent>(A, TEXT("Root"));
  A->SetRootComponent(Root); A->AddInstanceComponent(Root); Root->RegisterComponent();
  Root->SetWorldLocation(Feet);
  auto Mesh = [A](const TCHAR* Name, USkeletalMesh* Asset, USceneComponent* Parent)
  {
   USkeletalMeshComponent* C = NewObject<USkeletalMeshComponent>(A, Name);
   C->SetSkeletalMeshAsset(Asset);
   C->SetupAttachment(Parent);
   C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   A->AddInstanceComponent(C); C->RegisterComponent();
   return C;
  };
  USkeletalMeshComponent* Body = Mesh(TEXT("Body"), BodyMesh, Root);
  USkeletalMeshComponent* Face = Mesh(TEXT("Face"), FaceMesh, Body);
  Face->SetAnimInstanceClass(FaceAnim);
  Face->AddTickPrerequisiteComponent(Body);
  // The stock node defaults to copying bones only. Our CTRL_expressions curves live on
  // Body's native proxy, so explicitly connect them to the MetaHuman face's RigLogic input.
  int32 CopyNodes = 0;
  UAnimInstance* FaceInstance = Face->GetAnimInstance();
  if (FaceInstance) for (TFieldIterator<FStructProperty> It(FaceInstance->GetClass()); It; ++It)
   if (It->Struct == FAnimNode_CopyPoseFromMesh::StaticStruct())
   {
    FAnimNode_CopyPoseFromMesh* Node = It->ContainerPtrToValuePtr<FAnimNode_CopyPoseFromMesh>(FaceInstance);
    Node->SourceMeshComponent = Body;
    Node->bUseAttachedParent = true;
    Node->bCopyCurves = true;
    ++CopyNodes;
   }
  if (!CopyNodes)
  {
   UE_LOG(LogS47Waitress, Warning, TEXT("S47: waitress face has no CopyPose node; original figure kept"));
   A->Destroy(); return nullptr;
  }
  UE_LOG(LogS47Waitress, Display, TEXT("S47: waitress face pose/curve links %d, postprocess %s"), CopyNodes, *GetNameSafe(Face->GetPostProcessInstance()));
  // A short bob does not need simulated strands. Keep the authored card geometry and
  // coverage atlas, with an ordinary masked surface material that also cooks for Linux.
  // Groom's MI_Hair_Cards expects its special vertex factory and is transparent on a static mesh.
  UStaticMesh* HairMesh = Load<UStaticMesh>(TEXT("/Game/MetaHumans/MHC_Waitress/Grooms/Hair_M_Layered_CardsMesh_Group0_LOD1.Hair_M_Layered_CardsMesh_Group0_LOD1"));
  UMaterialInterface* HairMaterial = Load<UMaterialInterface>(TEXT("/Game/MetaHumans/Waitress/Uniform/M_Waitress_HairCards.M_Waitress_HairCards"));
  const FReferenceSkeleton& Ref = FaceMesh->GetRefSkeleton();
  const int32 Head = Ref.FindBoneIndex(TEXT("head"));
  if (!HairMesh || !HairMaterial || Head == INDEX_NONE) { A->Destroy(); return nullptr; }
  FTransform HeadRef = FTransform::Identity;
  for (int32 Bone = Head; Bone != INDEX_NONE; Bone = Ref.GetParentIndex(Bone)) HeadRef = HeadRef * Ref.GetRefBonePose()[Bone];
  UStaticMeshComponent* Hair = NewObject<UStaticMeshComponent>(A, TEXT("HairCards"));
  Hair->SetStaticMesh(HairMesh);
  for (int32 Slot = 0; Slot < HairMesh->GetStaticMaterials().Num(); ++Slot) Hair->SetMaterial(Slot, HairMaterial);
  Hair->SetupAttachment(Face, TEXT("head"));
  Hair->SetRelativeTransform(HeadRef.Inverse()); // imported cards are in face reference component space
  Hair->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  A->AddInstanceComponent(Hair); Hair->RegisterComponent();
  UE_LOG(LogS47Waitress, Display, TEXT("S47: waitress authored rigid hair cards %s, headRef %s"), *GetNameSafe(HairMesh), *HeadRef.ToString());
  for (const FGroomPart& G : Grooms)
  {
   if (FCString::Strcmp(G.Name, TEXT("Hair")) == 0) continue;
   UGroomAsset* Groom = Load<UGroomAsset>(G.Groom);
   if (!Groom) continue;
   UGroomComponent* C = NewObject<UGroomComponent>(A, G.Name);
   C->SimulationSettings.bOverrideSettings = true;
   C->SimulationSettings.SolverSettings.bEnableSimulation = false;
   C->SetupAttachment(Face);
   C->SetGroomAsset(Groom, Load<UGroomBindingAsset>(G.Binding), false);
   // This is one conversational NPC. Use her authored cards rather than a strand simulation;
   // explicit LOD selection also avoids choosing an unbuilt groom LOD without LODSync.
   int32 CardLOD = MAX_int32;
   // SourceDescription::GetMesh() intentionally returns nullptr in a packaged build.
   // Select the actual cooked card data, not that editor-only convenience function.
   for (const FHairGroupPlatformData& Group : Groom->GetHairGroupsPlatformData())
    for (int32 LOD = 0; LOD < Group.Cards.LODs.Num(); ++LOD)
     if (Group.Cards.IsValid(LOD)) CardLOD = FMath::Min(CardLOD, LOD);
   if (CardLOD != MAX_int32) { C->SetUseCards(true); C->SetForcedLOD(CardLOD); }
   A->AddInstanceComponent(C); C->RegisterComponent();
   UE_LOG(LogS47Waitress, Display, TEXT("S47: waitress groom %s loaded=%s cards=%d forcedLOD=%d groups=%u"), G.Name, *GetNameSafe(Groom), C->bUseCards, C->GetForcedLOD(), C->GetGroupCount());
  }
  return A;
 }

 // The 1986 uniform replaces MetaHuman Creator's default garment; it follows the body (Leader Pose).
 bool Dress(AActor* A, USkeletalMeshComponent* Body)
 {
  USkeletalMesh* Uniform = Load<USkeletalMesh>(UniformPath);
  if (!Uniform) return false;
  USkeletalMeshComponent* Worn = NewObject<USkeletalMeshComponent>(A, TEXT("Uniform"));
  Worn->SetSkeletalMeshAsset(Uniform);
  Worn->SetupAttachment(Body);
  Worn->SetLeaderPoseComponent(Body);
  Worn->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  A->AddInstanceComponent(Worn); Worn->RegisterComponent();
  return true;
 }
 const FName Tag = TEXT("S47Waitress");
 constexpr double AttentionRange = 650; // cm from her eyes: inside the diner, not through the window

 // Turn the spawned actor so her measured facing (from the hips) points along Facing.
 void Face(AActor* A, USkeletalMeshComponent* Body, const FVector& Facing)
 {
  FVector Left = Body->GetSocketLocation(TEXT("thigh_l")) - Body->GetSocketLocation(TEXT("thigh_r")); Left.Z = 0;
  if (Left.IsNearlyZero() || Facing.IsNearlyZero()) return;
  const FVector Forward = FVector::UpVector ^ Left.GetSafeNormal();
  const double Delta = FMath::RadiansToDegrees(FMath::Atan2((Forward ^ Facing.GetSafeNormal2D()).Z, Forward | Facing.GetSafeNormal2D()));
  A->AddActorWorldRotation(FRotator(0, Delta, 0));
 }

 void Attach(AS47Web* W)
 {
  UWorld* World = W ? W->GetWorld() : nullptr;
  if (!World || FParse::Param(FCommandLine::Get(), TEXT("S47NoMetaHuman"))) return;
  for (TActorIterator<AActor> It(World); It; ++It) if (It->ActorHasTag(Tag)) It->Destroy(); // a rebuilt diner gets one waitress
  FS47Obj Old = W->Obj(TEXT("world.diner.objs.waitress")), OldHead = W->Obj(TEXT("world.diner.objs.waitressHead"));
  const FS47Obj Counter = W->Obj(TEXT("world.diner.proxies.counter"));
  if (!Old || !Counter) return;
  const FVector Feet = S47Three::ToUE(Old.WorldPosition());
  AActor* A = Assemble(World, Feet);
  if (!A) { UE_LOG(LogS47Waitress, Display, TEXT("S47: waitress MetaHuman not installed; original figure kept")); return; }
  A->Tags.Add(Tag);
  US47PersonComponent* P = NewObject<US47PersonComponent>(A, TEXT("S47Person"));
  A->AddInstanceComponent(P); P->RegisterComponent();
  // Body idle provides breathing; the native driver adds quiet gaze and blinks. Do not load
  // the discarded face idle that referenced an absent editor skeleton.
  UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, IdlePath, nullptr, LOAD_NoWarn | LOAD_Quiet);
  if (!Idle || !P->Setup(Idle, nullptr))
  { A->Destroy(); return; }
  // She stands where the old figure stood, facing across the counter.
  FVector Facing = S47Three::ToUE(Counter.WorldPosition()) - Feet; Facing.Z = 0;
  Face(A, P->Body, Facing);
  // Without the uniform she would stand in MetaHuman's default T-shirt: keep the old figure instead.
  if (!Dress(A, P->Body)) { UE_LOG(LogS47Waitress, Display, TEXT("S47: waitress uniform not installed; original figure kept")); A->Destroy(); return; }
  Old.SetVisible(false); if (OldHead) OldHead.SetVisible(false);
  const TWeakObjectPtr<AS47Web> Web(W);
  const FS47Obj Room = Old.Parent();
  P->TalkPath = TakePath;
  P->Speech = [Web](FString& Clip, double& Seconds)
  {
   US47Main* M = Web.IsValid() ? US47Main::Get(Web.Get()) : nullptr;
   return M && M->Evening && M->Evening->PhysicalSpeech(Clip, Seconds);
  };
  P->Listener = [Web, Feet]() -> TOptional<FVector>
  {
   US47Main* M = Web.IsValid() ? US47Main::Get(Web.Get()) : nullptr;
   if (!M || !M->Player || Web->Area() != TEXT("diner")) return {};
   const FVector Eye = M->Player->EyeUE();
   return FVector::Dist(Eye, Feet + FVector(0, 0, 155)) < AttentionRange ? TOptional<FVector>(Eye) : TOptional<FVector>();
  };
  // Canon (Looks): she looks at the window each time a truck might pass, and at her counter.
  for (const TCHAR* Id : {TEXT("world.diner.proxies.window"), TEXT("world.diner.proxies.counter"), TEXT("world.diner.proxies.coffee")})
   if (const FS47Obj O = W->Obj(Id)) P->IdleSpots.Add(S47Three::ToUE(O.WorldPosition()));
  // Follow the diner's own visibility (closed variants, other areas).
  P->Present = [Web, Room]() { return !Web.IsValid() || !Room || Room.Shown(); };
  UE_LOG(LogS47Waitress, Display, TEXT("S47: waitress MetaHuman at %s yaw %.1f; %s"), *Feet.ToString(), A->GetActorRotation().Yaw, *P->Describe());
 }
}
