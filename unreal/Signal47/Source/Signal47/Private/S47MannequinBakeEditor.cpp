// Local authoring only. Licensed source/derived assets stay in ignored MetaHumans.
// No runtime registration, person, animation graph, or waitress asset is changed.
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/Skeleton.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSourceData.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "MeshUtilities.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Modules/ModuleManager.h"
#include "PhysicsEngine/BodySetup.h"
#include "RawMesh.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/MemoryWriter.h"
#include "StaticMeshCompiler.h"
#include "StaticMeshResources.h"
#include "TwoBoneIK.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47MannequinBake, Log, All);
namespace S47MannequinBake
{
 using FJson = TSharedPtr<FJsonObject>;
 FJson Object() { return MakeShared<FJsonObject>(); }
 TSharedPtr<FJsonValue> Value(const FJson& Input) { return MakeShared<FJsonValueObject>(Input); }
 TArray<TSharedPtr<FJsonValue>> Vector(const FVector& Input)
 {
  return {MakeShared<FJsonValueNumber>(Input.X), MakeShared<FJsonValueNumber>(Input.Y), MakeShared<FJsonValueNumber>(Input.Z)};
 }
 FString Digest(TArray<uint8>& Bytes)
 {
  uint8 Hash[FSHA1::DigestSize]; FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Hash);
  return BytesToHex(Hash, UE_ARRAY_COUNT(Hash)).ToLower();
 }
 FString RawDigest(FRawMesh& Raw)
 {
  TArray<uint8> Bytes; FMemoryWriter Writer(Bytes); Writer << Raw; return Digest(Bytes);
 }
 bool Read(const FString& Path, FJson& Out)
 {
  FString Text;
  return FFileHelper::LoadFileToString(Text, *Path) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Out) && Out.IsValid();
 }
 bool Write(const FString& Path, const FJson& Input)
 {
  FString Text; FJsonSerializer::Serialize(Input.ToSharedRef(), TJsonWriterFactory<>::Create(&Text));
  return FFileHelper::SaveStringToFile(Text, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
 }
 struct FPose
 {
  const FReferenceSkeleton& Ref;
  TArray<FTransform> Local, Component;
  explicit FPose(const FReferenceSkeleton& InRef) : Ref(InRef), Local(InRef.GetRefBonePose()) { Rebuild(); }
  void Rebuild()
  {
   Component.SetNum(Local.Num());
   for (int32 Index = 0; Index < Local.Num(); ++Index)
   {
    const int32 Parent = Ref.GetParentIndex(Index);
    Component[Index] = Parent == INDEX_NONE ? Local[Index] : Local[Index] * Component[Parent];
   }
  }
  void Set(int32 Index, const FTransform& Transform)
  {
   const int32 Parent = Ref.GetParentIndex(Index);
   Local[Index] = Parent == INDEX_NONE ? Transform : Transform.GetRelativeTransform(Component[Parent]);
   Local[Index].NormalizeRotation(); Rebuild();
  }
  void Aim(int32 Index, int32 Child, const FVector& Direction)
  {
   FTransform Transform = Component[Index];
   const FVector OldDirection = (Component[Child].GetLocation() - Transform.GetLocation()).GetSafeNormal();
   Transform.SetRotation(FQuat::FindBetweenNormals(OldDirection, Direction.GetSafeNormal()) * Transform.GetRotation());
   Set(Index, Transform);
  }
  FString Hash() const
  {
   TArray<uint8> Bytes; FMemoryWriter Writer(Bytes);
   for (int32 Index = 0; Index < Local.Num(); ++Index)
   {
    FString Name = Ref.GetBoneName(Index).ToString(); int32 Parent = Ref.GetParentIndex(Index); FTransform Transform = Local[Index];
    Writer << Name << Parent << Transform;
   }
   return Digest(Bytes);
  }
 };

 struct FBake
 {
  FJson Job, Contract, Report = Object();
  FString Error, Mode, ReportPath;
  int32 Checks = 0;
  AActor* Actor = nullptr;
  USkeletalMesh* Body = nullptr;
  USkeletalMesh* Face = nullptr;
  USkeletalMesh* Clothes = nullptr;
  UStaticMesh* HairNear = nullptr;
  UStaticMesh* HairFar = nullptr;
  UStaticMesh* BrowsNear = nullptr;
  UStaticMesh* BrowsFar = nullptr;
  UMaterialInterface* HairMaterial = nullptr;
  UMaterialInterface* BrowMaterial = nullptr;
  ~FBake() { if (Actor) Actor->Destroy(); }
  bool Need(bool Test, const FString& Detail)
  {
   ++Checks;
   if (!Test)
   {
    Error = Detail; UE_LOG(LogS47MannequinBake, Error, TEXT("S47 Mannequin FAIL %s"), *Detail);
   }
   return Test;
  }
  template<typename T> T* Load(const FString& Path) { return LoadObject<T>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet); }
  double PoseNumber(const TCHAR* Key) const { return Contract->GetObjectField(TEXT("pose"))->GetNumberField(Key); }
  bool SameReference(const FTransform& First, const FTransform& Second) const
  {
   return First.GetLocation().Equals(Second.GetLocation(), PoseNumber(TEXT("max_shared_reference_error_cm"))) &&
    First.GetScale3D().Equals(Second.GetScale3D(), 0.00001) &&
    FMath::RadiansToDegrees(First.GetRotation().AngularDistance(Second.GetRotation())) <= PoseNumber(TEXT("max_shared_reference_angle_degrees"));
  }
  FJson SkeletalSource(USkeletalMesh* Mesh)
  {
   FJson Data = Object(); Data->SetStringField(TEXT("asset"), Mesh->GetPathName());
   Data->SetStringField(TEXT("reference_pose_sha1"), FPose(Mesh->GetRefSkeleton()).Hash());
   Data->SetNumberField(TEXT("bones"), Mesh->GetRefSkeleton().GetNum());
   TArray<TSharedPtr<FJsonValue>> LODs, Materials;
   for (const FSkeletalMeshLODRenderData& LOD : Mesh->GetResourceForRendering()->LODRenderData)
   {
    FJson Item = Object(); Item->SetNumberField(TEXT("vertices"), LOD.GetNumVertices());
    Item->SetNumberField(TEXT("triangles"), LOD.MultiSizeIndexContainer.GetIndexBuffer()->Num() / 3);
    Item->SetNumberField(TEXT("uv_channels"), LOD.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords());
    LODs.Add(Value(Item));
   }
   for (const FSkeletalMaterial& Material : Mesh->GetMaterials())
   {
    FJson Item = Object(); Item->SetStringField(TEXT("slot"), Material.MaterialSlotName.ToString());
    Item->SetStringField(TEXT("material"), GetPathNameSafe(Material.MaterialInterface)); Materials.Add(Value(Item));
   }
   Data->SetArrayField(TEXT("lods"), LODs); Data->SetArrayField(TEXT("materials"), Materials); return Data;
  }
  bool Preflight()
  {
   if (!Need(GIsEditor && GEditor && !GEditor->PlayWorld, TEXT("editor authoring world, no PIE or game"))) return false;
   const FJson Sources = Contract->GetObjectField(TEXT("sources"));
   Body = Load<USkeletalMesh>(Sources->GetStringField(TEXT("body")));
   Face = Load<USkeletalMesh>(Sources->GetStringField(TEXT("face")));
   Clothes = Load<USkeletalMesh>(Job->GetStringField(TEXT("clothes_asset")));
   HairNear = Load<UStaticMesh>(Sources->GetStringField(TEXT("hair_near")));
   HairFar = Load<UStaticMesh>(Sources->GetStringField(TEXT("hair_far")));
   BrowsNear = Load<UStaticMesh>(Sources->GetStringField(TEXT("brows_near")));
   BrowsFar = Load<UStaticMesh>(Sources->GetStringField(TEXT("brows_far")));
   HairMaterial = Load<UMaterialInterface>(Sources->GetStringField(TEXT("hair_material")));
   BrowMaterial = Load<UMaterialInterface>(Job->GetStringField(TEXT("brow_material")));
   if (!Need(Body && Face && Clothes && HairNear && HairFar && BrowsNear && BrowsFar && HairMaterial && BrowMaterial,
    TEXT("all existing female, cloth, actual hair/brow card meshes and surface materials available"))) return false;
   FAssetCompilingManager::Get().FinishAllCompilation();
   if (!Need(Body->GetResourceForRendering() && Face->GetResourceForRendering() && Clothes->GetResourceForRendering(), TEXT("native skeletal render data available"))) return false;
   const FString Output = Job->GetStringField(TEXT("output_asset"));
   if (!Need(Output.StartsWith(TEXT("/Game/MetaHumans/Mannequins/RadioWoman/")) && !Output.Contains(TEXT("..")), TEXT("private bounded output package"))) return false;
   if (!Need(Clothes->GetSkeleton() != Body->GetSkeleton() && Clothes->GetSkeleton() &&
    Clothes->GetSkeleton()->GetPathName().StartsWith(TEXT("/Game/MetaHumans/Mannequins/RadioWoman/")), TEXT("cloth importer used private skeleton copy"))) return false;
   FJson Inputs = Object(); Inputs->SetObjectField(TEXT("body"), SkeletalSource(Body)); Inputs->SetObjectField(TEXT("face"), SkeletalSource(Face));
   Inputs->SetObjectField(TEXT("clothes"), SkeletalSource(Clothes)); Report->SetObjectField(TEXT("skeletal_sources"), Inputs);
   return true;
  }
  bool PoseBody(FPose& Posed, FTransform& Seat)
  {
   const auto Bone = [&](const TCHAR* Name) { return Posed.Ref.FindBoneIndex(Name); };
   const TCHAR* Required[] = {TEXT("pelvis"),TEXT("thigh_l"),TEXT("thigh_r"),TEXT("calf_l"),TEXT("calf_r"),TEXT("foot_l"),TEXT("foot_r"),
    TEXT("upperarm_l"),TEXT("upperarm_r"),TEXT("lowerarm_l"),TEXT("lowerarm_r"),TEXT("hand_l"),TEXT("hand_r"),TEXT("head")};
   for (const TCHAR* Name : Required) if (!Need(Bone(Name) != INDEX_NONE, FString(TEXT("required native pose bone ")) + Name)) return false;
   const FPose Reference(Posed.Ref);
   FVector Left = Posed.Component[Bone(TEXT("thigh_l"))].GetLocation() - Posed.Component[Bone(TEXT("thigh_r"))].GetLocation(); Left.Z = 0;
   if (!Need(Left.Normalize(), TEXT("measured hip left axis"))) return false;
   const FVector Forward = (FVector::UpVector ^ Left).GetSafeNormal();
   const FVector Pelvis = Posed.Component[Bone(TEXT("pelvis"))].GetLocation();
   Seat = FTransform(FRotationMatrix::MakeFromXZ(Forward, FVector::UpVector).ToQuat(), Pelvis - FVector::UpVector * PoseNumber(TEXT("pelvis_above_seat_cm")));
   const double Drop = FMath::DegreesToRadians(PoseNumber(TEXT("thigh_drop_degrees")));
   TArray<TSharedPtr<FJsonValue>> IK;
   for (const bool IsLeft : {true, false})
   {
    const int32 Thigh = Bone(IsLeft ? TEXT("thigh_l") : TEXT("thigh_r"));
    const int32 Calf = Bone(IsLeft ? TEXT("calf_l") : TEXT("calf_r"));
    const int32 Foot = Bone(IsLeft ? TEXT("foot_l") : TEXT("foot_r"));
    if (!Need(Posed.Ref.GetParentIndex(Calf) == Thigh && Posed.Ref.GetParentIndex(Foot) == Calf, TEXT("native direct thigh/calf/foot chain"))) return false;
    const double ThighLength = FVector::Distance(Posed.Component[Thigh].GetLocation(), Posed.Component[Calf].GetLocation());
    Posed.Aim(Thigh, Calf, Forward * FMath::Cos(Drop) - FVector::UpVector * FMath::Sin(Drop) + Left * ((IsLeft ? 1 : -1) * PoseNumber(TEXT("knee_splay_cm")) / ThighLength));
    Posed.Aim(Calf, Foot, -FVector::UpVector);
    FTransform FlatFoot = Posed.Component[Foot]; FlatFoot.SetRotation(Reference.Component[Foot].GetRotation()); Posed.Set(Foot, FlatFoot);
    const int32 Upper = Bone(IsLeft ? TEXT("upperarm_l") : TEXT("upperarm_r"));
    const int32 Lower = Bone(IsLeft ? TEXT("lowerarm_l") : TEXT("lowerarm_r"));
    const int32 Hand = Bone(IsLeft ? TEXT("hand_l") : TEXT("hand_r"));
    if (!Need(Posed.Ref.GetParentIndex(Lower) == Upper && Posed.Ref.GetParentIndex(Hand) == Lower, TEXT("native direct upperarm/lowerarm/hand chain"))) return false;
    const double Side = IsLeft ? 1 : -1;
    const FVector Target = Pelvis + Forward * PoseNumber(TEXT("hand_forward_cm")) + Left * (Side * PoseNumber(TEXT("hand_side_cm"))) + FVector::UpVector * PoseNumber(TEXT("hand_above_pelvis_cm"));
    const FVector Elbow = Pelvis + Forward * PoseNumber(TEXT("elbow_forward_cm")) + Left * (Side * PoseNumber(TEXT("elbow_side_cm"))) + FVector::UpVector * PoseNumber(TEXT("elbow_above_pelvis_cm"));
    FTransform UpperPose = Posed.Component[Upper], LowerPose = Posed.Component[Lower], HandPose = Posed.Component[Hand];
    AnimationCore::SolveTwoBoneIK(UpperPose, LowerPose, HandPose, Elbow, Target, false, 1.0, 1.0);
    Posed.Set(Upper, UpperPose); Posed.Set(Lower, LowerPose); Posed.Set(Hand, HandPose);
    const double TargetError = FVector::Distance(Posed.Component[Hand].GetLocation(), Target);
    FJson Item = Object(); Item->SetStringField(TEXT("side"), IsLeft ? TEXT("left") : TEXT("right"));
    Item->SetArrayField(TEXT("target_native_cm"), Vector(Target)); Item->SetNumberField(TEXT("error_cm"), TargetError); IK.Add(Value(Item));
    if (!Need(TargetError <= PoseNumber(TEXT("max_hand_target_error_cm")), TEXT("non-stretched native hand IK reaches authored radio-side target"))) return false;
   }
   Report->SetArrayField(TEXT("hand_ik"), IK);
   Report->SetStringField(TEXT("posed_body_sha1"), Posed.Hash());
   FJson Landmarks = Object();
   for (const TCHAR* Name : Required) Landmarks->SetArrayField(Name, Vector(Seat.InverseTransformPosition(Posed.Component[Bone(Name)].GetLocation())));
   Report->SetObjectField(TEXT("seat_space_landmarks_cm"), Landmarks);
   Report->SetStringField(TEXT("output_axes"), TEXT("+X forward, +Y right, +Z up; origin seat"));
   return true;
  }
  bool Transfer(const FPose& Authoritative, USkeletalMesh* Mesh, bool IsClothes, FPose& Target)
  {
   const FPose BodyReference(Authoritative.Ref), TargetReference(Target.Ref);
   int32 Shared = 0; TArray<TSharedPtr<FJsonValue>> Mapping;
   for (int32 Index = 0; Index < Target.Local.Num(); ++Index)
   {
    const FName Name = Target.Ref.GetBoneName(Index); const int32 SourceIndex = Authoritative.Ref.FindBoneIndex(Name);
    if (SourceIndex == INDEX_NONE)
    {
     if (IsClothes && !Need(false, FString(TEXT("cloth bone missing from native body: ")) + Name.ToString())) return false;
     continue; // Facial-only branches keep their actual reference local transform.
    }
    if (!Need(SameReference(BodyReference.Component[SourceIndex], TargetReference.Component[Index]),
     FString(TEXT("shared native reference pose matches: ")) + Name.ToString() + TEXT(" in ") + Mesh->GetName())) return false;
    if (IsClothes)
    {
     const int32 SourceParent = Authoritative.Ref.GetParentIndex(SourceIndex), TargetParent = Target.Ref.GetParentIndex(Index);
     const FName SourceName = SourceParent == INDEX_NONE ? NAME_None : Authoritative.Ref.GetBoneName(SourceParent);
     const FName TargetName = TargetParent == INDEX_NONE ? NAME_None : Target.Ref.GetBoneName(TargetParent);
     if (!Need(SourceName == TargetName, FString(TEXT("cloth parent matches body: ")) + Name.ToString())) return false;
    }
    Target.Set(Index, Authoritative.Component[SourceIndex]); ++Shared; Mapping.Add(MakeShared<FJsonValueString>(Name.ToString()));
   }
   if (!Need(Shared > 0 && (IsClothes || Target.Ref.FindBoneIndex(TEXT("head")) != INDEX_NONE), TEXT("actual shared pose and face head present"))) return false;
   FJson Data = Object(); Data->SetNumberField(TEXT("shared_bones"), Shared); Data->SetArrayField(TEXT("names"), Mapping); Data->SetStringField(TEXT("pose_sha1"), Target.Hash());
   Report->SetObjectField(IsClothes ? TEXT("cloth_pose_transfer") : TEXT("face_pose_transfer"), Data); return true;
  }
  UPoseableMeshComponent* Component(USkeletalMesh* Mesh, const FPose& Posed, const TCHAR* Name)
  {
   UPoseableMeshComponent* Part = NewObject<UPoseableMeshComponent>(Actor, Name, RF_Transient);
   Part->SetSkinnedAssetAndUpdate(Mesh); Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   Part->SetComponentTickEnabled(false); Actor->AddInstanceComponent(Part); Part->RegisterComponent();
   Part->BoneSpaceTransforms = Posed.Local; Part->RefreshBoneTransforms();
   Part->SetCPUSkinningEnabled(true, true);
   return Part;
  }
  FJson StaticAudit(UStaticMesh* Mesh)
  {
   FJson Data = Object(); Data->SetStringField(TEXT("asset"), Mesh->GetPathName());
   Data->SetStringField(TEXT("class"), Mesh->GetClass()->GetName());
   Data->SetBoolField(TEXT("nanite"), Mesh->GetNaniteSettings().bEnabled); Data->SetBoolField(TEXT("ray_tracing"), Mesh->bSupportRayTracing);
   Data->SetBoolField(TEXT("body_no_collision"), Mesh->GetBodySetup() && Mesh->GetBodySetup()->DefaultInstance.GetCollisionEnabled() == ECollisionEnabled::NoCollision);
   Data->SetBoolField(TEXT("cpu_access"), Mesh->bAllowCPUAccess); Data->SetBoolField(TEXT("auto_screen_sizes"), Mesh->GetAutoComputeLODScreenSize());
   Data->SetArrayField(TEXT("bounds_min_cm"), Vector(Mesh->GetBoundingBox().Min)); Data->SetArrayField(TEXT("bounds_max_cm"), Vector(Mesh->GetBoundingBox().Max));
   TArray<TSharedPtr<FJsonValue>> LODs, Materials;
   for (int32 Index = 0; Index < Mesh->GetNumSourceModels(); ++Index)
   {
    FRawMesh Raw; const FStaticMeshSourceModel& Model = Mesh->GetSourceModel(Index); Model.LoadRawMesh(Raw);
    FJson Item = Object(); Item->SetNumberField(TEXT("index"), Index); Item->SetNumberField(TEXT("triangles"), Raw.WedgeIndices.Num() / 3);
    Item->SetNumberField(TEXT("vertices"), Raw.VertexPositions.Num()); Item->SetStringField(TEXT("raw_sha1"), RawDigest(Raw));
    Item->SetNumberField(TEXT("screen_size"), Model.ScreenSize.Default);
    Item->SetBoolField(TEXT("recompute_normals"), Model.BuildSettings.bRecomputeNormals); Item->SetBoolField(TEXT("recompute_tangents"), Model.BuildSettings.bRecomputeTangents);
    Item->SetBoolField(TEXT("generate_lightmap_uvs"), Model.BuildSettings.bGenerateLightmapUVs); Item->SetBoolField(TEXT("full_precision_uvs"), Model.BuildSettings.bUseFullPrecisionUVs);
    Item->SetBoolField(TEXT("high_precision_tangents"), Model.BuildSettings.bUseHighPrecisionTangentBasis);
    int32 UVs = 0; for (const auto& Channel : Raw.WedgeTexCoords) if (Channel.Num()) ++UVs; Item->SetNumberField(TEXT("uv_channels"), UVs);
    if (Mesh->GetRenderData() && Mesh->GetRenderData()->LODResources.IsValidIndex(Index))
    {
     const FStaticMeshLODResources& Render = Mesh->GetRenderData()->LODResources[Index];
     Item->SetNumberField(TEXT("render_vertices"), Render.GetNumVertices()); Item->SetNumberField(TEXT("render_triangles"), Render.GetNumTriangles());
     TArray<TSharedPtr<FJsonValue>> Sections;
     for (const FStaticMeshSection& Section : Render.Sections)
     {
      FJson Part = Object(); Part->SetNumberField(TEXT("material_index"), Section.MaterialIndex); Part->SetNumberField(TEXT("triangles"), Section.NumTriangles); Sections.Add(Value(Part));
     }
     Item->SetArrayField(TEXT("sections"), Sections);
    }
    LODs.Add(Value(Item));
   }
   for (const FStaticMaterial& Material : Mesh->GetStaticMaterials()) Materials.Add(MakeShared<FJsonValueString>(GetPathNameSafe(Material.MaterialInterface)));
   Data->SetArrayField(TEXT("lods"), LODs); Data->SetArrayField(TEXT("materials"), Materials); return Data;
  }
  bool Audit(UStaticMesh* Mesh, const FJson& Expected = nullptr)
  {
   const FJson Actual = StaticAudit(Mesh); Report->SetObjectField(TEXT("output"), Actual);
   const auto& Settings = Contract->GetArrayField(TEXT("lods")); const auto& LODs = Actual->GetArrayField(TEXT("lods"));
   if (!Need(LODs.Num() == Settings.Num() && Mesh->GetRenderData() && Mesh->GetRenderData()->LODResources.Num() == Settings.Num(), TEXT("three actual authored and render LODs"))) return false;
   int32 PreviousTriangles = MAX_int32;
   for (int32 Index = 0; Index < LODs.Num(); ++Index)
   {
    const FJson Item = LODs[Index]->AsObject(), Limit = Settings[Index]->AsObject(); const int32 Triangles = Item->GetIntegerField(TEXT("triangles"));
    if (!Need(Triangles > 0 && Triangles < PreviousTriangles && Triangles <= Limit->GetIntegerField(TEXT("max_triangles")), FString::Printf(TEXT("measured decreasing LOD %d triangle budget"), Index))) return false;
    if (!Need(Triangles == Item->GetIntegerField(TEXT("render_triangles")) && Item->GetIntegerField(TEXT("uv_channels")) > 0 &&
     !Item->GetBoolField(TEXT("recompute_normals")) && !Item->GetBoolField(TEXT("recompute_tangents")) && !Item->GetBoolField(TEXT("generate_lightmap_uvs")) &&
     Item->GetBoolField(TEXT("full_precision_uvs")) && Item->GetBoolField(TEXT("high_precision_tangents")) &&
     FMath::IsNearlyEqual(Item->GetNumberField(TEXT("screen_size")), Limit->GetNumberField(TEXT("screen_size")), 0.000001), TEXT("render triangles, authored UV/normal/tangent and screen policy retained"))) return false;
    if (Expected)
    {
     const FJson Old = Expected->GetArrayField(TEXT("lods"))[Index]->AsObject();
     if (!Need(Item->GetStringField(TEXT("raw_sha1")) == Old->GetStringField(TEXT("raw_sha1")) && Triangles == Old->GetIntegerField(TEXT("triangles")), TEXT("fresh saved asset exact raw geometry/UV/normal/tangent/color/material digest"))) return false;
    }
    PreviousTriangles = Triangles;
   }
   if (!Need(!Mesh->GetNaniteSettings().bEnabled && Mesh->bSupportRayTracing && Mesh->bAllowCPUAccess && !Mesh->GetAutoComputeLODScreenSize() &&
    Actual->GetBoolField(TEXT("body_no_collision")), TEXT("explicit conventional static LOD policy, NoCollision and RT fallback"))) return false;
   if (Expected)
   {
    const auto& Before = Expected->GetArrayField(TEXT("materials")); const auto& After = Actual->GetArrayField(TEXT("materials"));
    if (!Need(Before.Num() == After.Num(), TEXT("fresh saved material slot count"))) return false;
    for (int32 Index = 0; Index < Before.Num(); ++Index) if (!Need(Before[Index]->AsString() == After[Index]->AsString(), TEXT("fresh saved material pointer identity"))) return false;
   }
   return true;
  }
  bool Run()
  {
   if (!Preflight()) return false;
   if (Mode == TEXT("verify"))
   {
    FJson Previous; if (!Need(Read(Job->GetStringField(TEXT("bake_report")), Previous) && Previous->GetStringField(TEXT("status")) == TEXT("PASS"), TEXT("actual successful private bake receipt"))) return false;
    UStaticMesh* Saved = Load<UStaticMesh>(Job->GetStringField(TEXT("output_asset")));
    if (!Need(Saved != nullptr, TEXT("saved output really reloads as StaticMesh"))) return false;
    FAssetCompilingManager::Get().FinishAllCompilation();
    return Audit(Saved, Previous->GetObjectField(TEXT("output")));
   }
   FPose BodyPose(Body->GetRefSkeleton()), FacePose(Face->GetRefSkeleton()), ClothPose(Clothes->GetRefSkeleton()); FTransform Seat;
   if (!PoseBody(BodyPose, Seat) || !Transfer(BodyPose, Face, false, FacePose) || !Transfer(BodyPose, Clothes, true, ClothPose)) return false;
   if (Mode == TEXT("probe")) return true;
   if (!Need(Mode == TEXT("bake") && !FPackageName::DoesPackageExist(Job->GetStringField(TEXT("output_asset"))), TEXT("fresh bake destination; no replacement of any previous candidate"))) return false;
   if (!Need(FApp::CanEverRender(), TEXT("bake has a render context; commandlets require AllowCommandletRendering, not NullRHI"))) return false;
   UWorld* World = GEditor->GetEditorWorldContext().World();
   if (!Need(World != nullptr, TEXT("isolated editor authoring world"))) return false;
   FActorSpawnParameters Spawn; Spawn.ObjectFlags = RF_Transient; Actor = World->SpawnActor<AActor>(AActor::StaticClass(), Spawn);
   if (!Need(Actor != nullptr, TEXT("transient authoring actor"))) return false;
   TArray<UMeshComponent*> Parts;
   Parts.Add(Component(Body, BodyPose, TEXT("Body"))); Parts.Add(Component(Face, FacePose, TEXT("Face"))); Parts.Add(Component(Clothes, ClothPose, TEXT("Clothes")));
   for (int32 Index = 0; Index < 3; ++Index) if (!Need(CastChecked<USkinnedMeshComponent>(Parts[Index])->MeshObject != nullptr, TEXT("actual CPU skin mesh object registered for each posed part"))) return false;
   const int32 FaceHead = FacePose.Ref.FindBoneIndex(TEXT("head")); const FPose FaceReference(FacePose.Ref);
   const FTransform HeadDelta = FaceReference.Component[FaceHead].Inverse() * FacePose.Component[FaceHead];
   auto Cards = [&](const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Material)
   {
    UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Actor, Name, RF_Transient); Part->SetStaticMesh(Mesh);
    Part->SetWorldTransform(HeadDelta); Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    for (int32 Slot = 0; Slot < Mesh->GetStaticMaterials().Num(); ++Slot) Part->SetMaterial(Slot, Material);
    Part->SetComponentTickEnabled(false); Actor->AddInstanceComponent(Part); Part->RegisterComponent(); Parts.Add(Part); return Part;
   };
   UStaticMeshComponent* Hair = Cards(TEXT("HairCards"), HairNear, HairMaterial);
   UStaticMeshComponent* Brows = Cards(TEXT("BrowCards"), BrowsNear, BrowMaterial);
   IMeshUtilities& Utilities = FModuleManager::LoadModuleChecked<IMeshUtilities>(TEXT("MeshUtilities"));
   TArray<FRawMesh> RawLODs; TArray<FStaticMaterial> Materials; TArray<TSharedPtr<FJsonValue>> Provenance;
   const auto& LODSettings = Contract->GetArrayField(TEXT("lods"));
   for (int32 Index = 0; Index < LODSettings.Num(); ++Index)
   {
    const FJson Settings = LODSettings[Index]->AsObject(); const bool Far = Settings->GetStringField(TEXT("cards")) == TEXT("far");
    Hair->SetStaticMesh(Far ? HairFar : HairNear); Brows->SetStaticMesh(Far ? BrowsFar : BrowsNear);
    for (int32 Slot = 0; Slot < Hair->GetNumMaterials(); ++Slot) Hair->SetMaterial(Slot, HairMaterial);
    for (int32 Slot = 0; Slot < Brows->GetNumMaterials(); ++Slot) Brows->SetMaterial(Slot, BrowMaterial);
    const FString Scratch = FString::Printf(TEXT("/Temp/S47Mannequin_%s_%d"), *FGuid::NewGuid().ToString(EGuidFormats::Digits), Index);
    UStaticMesh* Converted = Utilities.ConvertMeshesToStaticMesh(Parts, Seat, Scratch);
    if (!Need(Converted != nullptr, TEXT("production IMeshUtilities CPU pose conversion succeeded"))) return false;
    FStaticMeshCompilingManager::Get().FinishAllCompilation();
    const int32 SourceLOD = Settings->GetIntegerField(TEXT("overall_source_lod"));
    if (!Need(SourceLOD >= 0 && SourceLOD < Converted->GetNumSourceModels(), TEXT("requested actual overall LOD exists"))) return false;
    if (Index == 0) Materials = Converted->GetStaticMaterials();
    if (!Need(Materials.Num() == Converted->GetStaticMaterials().Num(), TEXT("near/far card material table size stable"))) return false;
    for (int32 Slot = 0; Slot < Materials.Num(); ++Slot) if (!Need(Materials[Slot].MaterialInterface == Converted->GetStaticMaterials()[Slot].MaterialInterface && Materials[Slot].MaterialInterface, TEXT("all body/face/cloth/card material pointers retained across LODs"))) return false;
    FRawMesh& Raw = RawLODs.AddDefaulted_GetRef(); Converted->GetSourceModel(SourceLOD).LoadRawMesh(Raw);
    if (!Need(Raw.IsValid(), TEXT("actual converted corner attributes valid without repair"))) return false;
    FJson Item = Object(); Item->SetNumberField(TEXT("output_lod"), Index); Item->SetNumberField(TEXT("overall_source_lod"), SourceLOD);
    Item->SetStringField(TEXT("hair_asset"), Hair->GetStaticMesh()->GetPathName()); Item->SetStringField(TEXT("brow_asset"), Brows->GetStaticMesh()->GetPathName());
    Item->SetStringField(TEXT("converted_raw_sha1"), RawDigest(Raw)); Provenance.Add(Value(Item));
   }
   const FString OutputPath = Job->GetStringField(TEXT("output_asset")); UPackage* Package = CreatePackage(*OutputPath);
   UStaticMesh* Output = NewObject<UStaticMesh>(Package, *FPackageName::GetLongPackageAssetName(OutputPath), RF_Public | RF_Standalone);
   Output->InitResources(); Output->SetLightingGuid(); Output->GetStaticMaterials() = Materials;
   Output->SetAutoComputeLODScreenSize(false); Output->bAllowCPUAccess = true; Output->bSupportRayTracing = true; Output->GetNaniteSettings().bEnabled = false;
   for (int32 Index = 0; Index < RawLODs.Num(); ++Index)
   {
    FStaticMeshSourceModel& Model = Output->AddSourceModel();
    Model.BuildSettings.bRecomputeNormals = false; Model.BuildSettings.bRecomputeTangents = false;
    Model.BuildSettings.bRemoveDegenerates = false; Model.BuildSettings.bUseHighPrecisionTangentBasis = true;
    Model.BuildSettings.bUseFullPrecisionUVs = true; Model.BuildSettings.bGenerateLightmapUVs = false;
    Model.ReductionSettings.PercentTriangles = 1.0f; Model.ScreenSize.Default = LODSettings[Index]->AsObject()->GetNumberField(TEXT("screen_size"));
    Model.SaveRawMesh(RawLODs[Index]);
    TArray<int32> Sections;
    for (const int32 Material : RawLODs[Index].FaceMaterialIndices) Sections.AddUnique(Material);
    for (int32 Section = 0; Section < Sections.Num(); ++Section)
    {
     const int32 Compact = Sections[Section];
     const int32 Slot = RawLODs[Index].MaterialIndexToImportIndex.IsValidIndex(Compact) ? RawLODs[Index].MaterialIndexToImportIndex[Compact] : Compact;
     Output->GetSectionInfoMap().Set(Index, Section, FMeshSectionInfo(Slot));
    }
   }
   Output->GetOriginalSectionInfoMap().CopyFrom(Output->GetSectionInfoMap());
   Output->SetImportVersion(EImportStaticMeshVersion::LastVersion); Output->Build(false); Output->PostEditChange();
   FStaticMeshCompilingManager::Get().FinishAllCompilation();
   Output->CreateBodySetup(); Output->GetBodySetup()->DefaultInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);
   Report->SetArrayField(TEXT("conversion_provenance"), Provenance);
   if (!Audit(Output)) return false;
   const auto& SavedLODs = Report->GetObjectField(TEXT("output"))->GetArrayField(TEXT("lods"));
   for (int32 Index = 0; Index < SavedLODs.Num(); ++Index)
    if (!Need(SavedLODs[Index]->AsObject()->GetStringField(TEXT("raw_sha1")) == Provenance[Index]->AsObject()->GetStringField(TEXT("converted_raw_sha1")), TEXT("output retains exact converted positions, winding, corner UVs/normals/tangents/colors and material map"))) return false;
   FAssetRegistryModule::AssetCreated(Output); Output->MarkPackageDirty();
   FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
   return Need(UPackage::SavePackage(Package, Output, *FPackageName::LongPackageNameToFilename(OutputPath, FPackageName::GetAssetPackageExtension()), Args), TEXT("only new private static output saved"));
  }
 };
 void Execute()
 {
  FBake Bake; const FString JobPath = FPlatformMisc::GetEnvironmentVariable(TEXT("S47_MANNEQUIN_JOB"));
  if (!Read(JobPath, Bake.Job)) { UE_LOG(LogS47MannequinBake, Error, TEXT("S47 Mannequin FAIL missing private job JSON")); return; }
  Bake.ReportPath = Bake.Job->GetStringField(TEXT("native_report")); Bake.Mode = Bake.Job->GetStringField(TEXT("mode"));
  const bool ReadContract = Read(Bake.Job->GetStringField(TEXT("contract")), Bake.Contract);
  const bool Passed = ReadContract && Bake.Run();
  Bake.Report->SetStringField(TEXT("status"), Passed ? TEXT("PASS") : TEXT("FAIL")); Bake.Report->SetStringField(TEXT("mode"), Bake.Mode);
  Bake.Report->SetStringField(TEXT("error"), Bake.Error); Bake.Report->SetNumberField(TEXT("checks"), Bake.Checks);
  Bake.Report->SetStringField(TEXT("evidence_limit"), TEXT("native authoring only; appearance, runtime adoption/schedule, input and performance require separate proof; no installation/release"));
  if (!Write(Bake.ReportPath, Bake.Report)) UE_LOG(LogS47MannequinBake, Error, TEXT("S47 Mannequin FAIL cannot write private receipt"));
  UE_LOG(LogS47MannequinBake, Display, TEXT("S47 Mannequin %s %s checks=%d report=%s"), Passed ? TEXT("PASS") : TEXT("FAIL"), *Bake.Mode, Bake.Checks, *Bake.ReportPath);
 }
 FAutoConsoleCommand BakeCommand(TEXT("S47.MannequinBake"), TEXT("Private editor-only mannequin pose/bake/verify; requires S47_MANNEQUIN_JOB JSON."), FConsoleCommandDelegate::CreateStatic(&Execute));
}
#endif
