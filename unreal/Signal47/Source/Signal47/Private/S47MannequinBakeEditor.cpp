// Local authoring only. Licensed source/derived assets stay in ignored MetaHumans.
// No runtime registration, person, animation graph, or waitress asset is changed.
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimInstance.h"
#include "AnimNodes/AnimNode_CopyPoseFromMesh.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "DynamicRHI.h"
#include "Editor.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSourceData.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "MaterialShared.h"
#include "MeshDescription.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Modules/ModuleManager.h"
#include "PhysicsEngine/BodySetup.h"
#include "RawMesh.h"
#include "RHI.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshModel.h"
#include "RenderingThread.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "StaticMeshCompiler.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshResources.h"
#include "StaticParameterSet.h"
#include "SkeletalRenderPublic.h"
#include "SkeletalMeshAttributes.h"
#include "ShaderCompiler.h"
#include "VertexFactory.h"
#include "TwoBoneIK.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

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
 FJson TransformData(const FTransform& Input)
 {
  FJson Data = Object(); const FQuat Rotation = Input.GetRotation();
  Data->SetArrayField(TEXT("translation_cm"), Vector(Input.GetLocation())); Data->SetArrayField(TEXT("scale"), Vector(Input.GetScale3D()));
  Data->SetArrayField(TEXT("rotation_quat_xyzw"), {MakeShared<FJsonValueNumber>(Rotation.X), MakeShared<FJsonValueNumber>(Rotation.Y),
   MakeShared<FJsonValueNumber>(Rotation.Z), MakeShared<FJsonValueNumber>(Rotation.W)}); return Data;
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
 // Wedge attributes and winding are semantic source data; vertex/triangle IDs and
 // smoothing masks may change during MeshDescription serialization. No numeric
 // quantization is used here. Tangent-Y is represented by its handedness, as UE does.
 FString CornerDigest(const FRawMesh& Raw)
 {
  TArray<FString> Triangles;
  for (int32 Triangle = 0; Triangle < Raw.FaceMaterialIndices.Num(); ++Triangle)
  {
   FString Best;
   for (int32 Start = 0; Start < 3; ++Start)
   {
    TArray<uint8> Bytes; FMemoryWriter Writer(Bytes); int32 Material = Raw.FaceMaterialIndices[Triangle]; Writer << Material;
    for (int32 Offset = 0; Offset < 3; ++Offset)
    {
     const int32 Wedge = Triangle * 3 + (Start + Offset) % 3;
     FVector3f Position = Raw.GetWedgePosition(Wedge), Tangent = Raw.WedgeTangentX[Wedge], Normal = Raw.WedgeTangentZ[Wedge];
     float Sign = ((Normal ^ Tangent) | Raw.WedgeTangentY[Wedge]) < 0 ? -1.0f : 1.0f;
     FColor Color = Raw.WedgeColors.IsValidIndex(Wedge) ? Raw.WedgeColors[Wedge] : FColor::White;
     Writer << Position << Tangent << Normal << Sign << Color;
     for (const auto& Channel : Raw.WedgeTexCoords)
     {
      bool Present = Channel.Num() != 0; Writer << Present;
      if (Present) { FVector2f UV = Channel[Wedge]; Writer << UV; }
     }
    }
    const FString Candidate = Digest(Bytes); if (Start == 0 || Candidate < Best) Best = Candidate;
   }
   Triangles.Add(Best);
  }
  Triangles.Sort(); TArray<uint8> Bytes; FMemoryWriter Writer(Bytes); Writer << Triangles; return Digest(Bytes);
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
  TArray<FVector> LeftSole, RightSole;
  TArray<FTransform> FaceReferenceCopyLocal;
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
  FJson CardSource(UStaticMesh* Mesh)
  {
   FJson Data = Object(); Data->SetStringField(TEXT("asset"), Mesh->GetPathName()); TArray<TSharedPtr<FJsonValue>> LODs;
   if (Mesh->GetRenderData()) for (const FStaticMeshLODResources& LOD : Mesh->GetRenderData()->LODResources)
   {
    FJson Row = Object(); Row->SetNumberField(TEXT("vertices"), LOD.GetNumVertices()); Row->SetNumberField(TEXT("triangles"), LOD.GetNumTriangles()); LODs.Add(Value(Row));
   }
   Data->SetArrayField(TEXT("lods"), LODs); Data->SetArrayField(TEXT("bounds_min_cm"), Vector(Mesh->GetBoundingBox().Min));
   Data->SetArrayField(TEXT("bounds_max_cm"), Vector(Mesh->GetBoundingBox().Max)); return Data;
  }
  // Pure native payload reads: no component, animation evaluation, CPU skinning,
  // render-context claim, reference-pose edit or source save in a NullRHI probe.
  bool ReferenceData(USkeletalMesh* Mesh, const FPose& Reference, FJson& Data, TArray<int32>& Direct)
  {
   Data = Object(); Data->SetStringField(TEXT("asset"), Mesh->GetPathName());
   Direct.Init(0, Reference.Local.Num()); TArray<FBox> Bounds; Bounds.Init(FBox(ForceInit), Direct.Num());
   TArray<TSharedPtr<FJsonValue>> LODs; TArray<TArray<int32>> PerLOD;
   bool Valid = true; int32 LODIndex = 0;
   for (const FSkeletalMeshLODRenderData& LOD : Mesh->GetResourceForRendering()->LODRenderData)
   {
    FJson Row = Object(); Row->SetNumberField(TEXT("lod"), LODIndex++); const FSkinWeightVertexBuffer* Weights = LOD.GetSkinWeightVertexBuffer();
    const auto& Positions = LOD.StaticVertexBuffers.PositionVertexBuffer;
    const bool Available = Weights && Weights->GetNumVertices() == LOD.GetNumVertices() && Weights->GetDataVertexBuffer()->GetWeightData()
     && (!Weights->GetVariableBonesPerVertex() || Weights->GetLookupVertexBuffer()->GetLookupData()) && Positions.GetVertexData();
    Row->SetBoolField(TEXT("cpu_payload_available"), Available); Row->SetNumberField(TEXT("vertices"), LOD.GetNumVertices());
    TArray<int32>& Hits = PerLOD.AddDefaulted_GetRef(); Hits.Init(0, Direct.Num());
    TArray<TSharedPtr<FJsonValue>> SectionRows;
    if (Available) for (const FSkelMeshRenderSection& Section : LOD.RenderSections)
    {
     FJson SectionRow = Object(); SectionRow->SetNumberField(TEXT("base_vertex"), Section.BaseVertexIndex);
     SectionRow->SetNumberField(TEXT("vertices"), Section.NumVertices); SectionRow->SetBoolField(TEXT("disabled"), Section.bDisabled);
     SectionRows.Add(Value(SectionRow));
     if (uint64(Section.BaseVertexIndex) + Section.NumVertices > LOD.GetNumVertices()) { Valid = false; continue; }
     for (uint32 Vertex = Section.BaseVertexIndex; Vertex < Section.BaseVertexIndex + Section.NumVertices; ++Vertex)
     {
      uint32 WeightOffset = 0, Count = 0; Weights->GetVertexInfluenceOffsetCount(Vertex, WeightOffset, Count);
      if (Count > Weights->GetMaxBoneInfluences()) { Valid = false; continue; }
      for (uint32 Influence = 0; Influence < Count; ++Influence)
      {
       if (!Weights->GetBoneWeight(Vertex, Influence)) continue;
       const uint32 LocalBone = Weights->GetBoneIndex(Vertex, Influence);
       if (LocalBone >= uint32(Section.BoneMap.Num())) { Valid = false; continue; }
       const int32 Bone = Section.BoneMap[LocalBone]; if (!Direct.IsValidIndex(Bone)) { Valid = false; continue; }
       ++Direct[Bone]; ++Hits[Bone]; Bounds[Bone] += FVector(Positions.VertexPosition(Vertex));
      }
     }
    }
    Valid &= Available; Row->SetArrayField(TEXT("sections"), SectionRows); LODs.Add(Value(Row));
   }
   TArray<bool> Affects; Affects.Init(false, Direct.Num()); for (int32 Index = 0; Index < Direct.Num(); ++Index) Affects[Index] = Direct[Index] != 0;
   for (int32 Index = Direct.Num() - 1; Index >= 0; --Index)
   { const int32 Parent = Reference.Ref.GetParentIndex(Index); if (Affects[Index] && Parent != INDEX_NONE) Affects[Parent] = true; }
   TArray<TSharedPtr<FJsonValue>> Bones;
   for (int32 Index = 0; Index < Direct.Num(); ++Index)
   {
    FJson Row = Object(); const int32 Parent = Reference.Ref.GetParentIndex(Index);
    Row->SetNumberField(TEXT("index"), Index); Row->SetStringField(TEXT("name"), Reference.Ref.GetBoneName(Index).ToString());
    Row->SetStringField(TEXT("parent"), Parent == INDEX_NONE ? TEXT("None") : Reference.Ref.GetBoneName(Parent).ToString());
    Row->SetObjectField(TEXT("reference_local"), TransformData(Reference.Local[Index])); Row->SetObjectField(TEXT("reference_component"), TransformData(Reference.Component[Index]));
    Row->SetNumberField(TEXT("nonzero_vertex_influences_all_lods"), Direct[Index]); Row->SetBoolField(TEXT("affects_weighted_descendants"), Affects[Index]);
    TArray<TSharedPtr<FJsonValue>> WeightedLODs; for (const TArray<int32>& Hits : PerLOD) WeightedLODs.Add(MakeShared<FJsonValueNumber>(Hits[Index]));
    Row->SetArrayField(TEXT("nonzero_vertex_influences_per_lod"), WeightedLODs);
    if (Bounds[Index].IsValid) { Row->SetArrayField(TEXT("weighted_reference_bounds_min_cm"), Vector(Bounds[Index].Min)); Row->SetArrayField(TEXT("weighted_reference_bounds_max_cm"), Vector(Bounds[Index].Max)); }
    Bones.Add(Value(Row));
   }
   Data->SetArrayField(TEXT("lods"), LODs); Data->SetArrayField(TEXT("bones"), Bones); Data->SetBoolField(TEXT("weight_scan_complete"), Valid); return Valid;
  }
  bool ReferenceDiagnostics(const FPose& BodyPose, const FPose& FacePose, const FPose& ClothPose)
  {
   const FPose BodyReference(BodyPose.Ref); FJson Evidence = Object(); Report->SetObjectField(TEXT("reference_diagnostics"), Evidence);
   FJson BodyData, FaceData, ClothData; TArray<int32> BodyWeights, FaceWeights, ClothWeights;
   const bool BodyValid = ReferenceData(Body, BodyReference, BodyData, BodyWeights);
   const bool FaceValid = ReferenceData(Face, FacePose, FaceData, FaceWeights);
   const bool ClothValid = ReferenceData(Clothes, ClothPose, ClothData, ClothWeights);
   Evidence->SetObjectField(TEXT("body"), BodyData); Evidence->SetObjectField(TEXT("face"), FaceData); Evidence->SetObjectField(TEXT("clothes"), ClothData);
   auto Shared = [&](const TCHAR* Key, const FPose& Target, const TArray<int32>& Weights)
   {
    TArray<TSharedPtr<FJsonValue>> Rows; int32 Mismatches = 0;
    for (int32 Index = 0; Index < Target.Local.Num(); ++Index)
    {
     const FName Name = Target.Ref.GetBoneName(Index); const int32 BodyIndex = BodyReference.Ref.FindBoneIndex(Name); if (BodyIndex == INDEX_NONE) continue;
     const FTransform& SourceRef = BodyReference.Component[BodyIndex]; const FTransform& TargetRef = Target.Component[Index];
     const bool Same = SameReference(SourceRef, TargetRef); Mismatches += Same ? 0 : 1; FJson Row = Object();
     Row->SetStringField(TEXT("bone"), Name.ToString()); Row->SetNumberField(TEXT("body_index"), BodyIndex); Row->SetNumberField(TEXT("target_index"), Index);
     Row->SetBoolField(TEXT("same_reference_with_original_limits"), Same);
     Row->SetNumberField(TEXT("translation_error_cm"), FVector::Distance(SourceRef.GetLocation(), TargetRef.GetLocation()));
     Row->SetNumberField(TEXT("rotation_error_degrees"), FMath::RadiansToDegrees(SourceRef.GetRotation().AngularDistance(TargetRef.GetRotation())));
     Row->SetNumberField(TEXT("scale_max_error"), (SourceRef.GetScale3D() - TargetRef.GetScale3D()).GetAbsMax());
     Row->SetObjectField(TEXT("target_ref_relative_to_body_ref"), TransformData(TargetRef.GetRelativeTransform(SourceRef)));
     Row->SetObjectField(TEXT("body_posed_component"), TransformData(BodyPose.Component[BodyIndex]));
     Row->SetNumberField(TEXT("target_direct_vertex_influences_all_lods"), Weights[Index]); Rows.Add(Value(Row));
    }
    FJson Data = Object(); Data->SetArrayField(TEXT("all_shared_bones"), Rows); Data->SetNumberField(TEXT("mismatches"), Mismatches); Evidence->SetObjectField(Key, Data);
   };
   Shared(TEXT("face_body"), FacePose, FaceWeights); Shared(TEXT("clothes_body"), ClothPose, ClothWeights);
   const TCHAR* FaceClassPath = TEXT("/Game/MetaHumans/Waitress/Anims/ABP_Face_Waitress.ABP_Face_Waitress_C");
   UClass* FaceClass = LoadClass<UAnimInstance>(nullptr, FaceClassPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
   UAnimInstance* Defaults = FaceClass ? FaceClass->GetDefaultObject<UAnimInstance>() : nullptr; TArray<TSharedPtr<FJsonValue>> Nodes;
   if (Defaults) for (TFieldIterator<FStructProperty> It(Defaults->GetClass()); It; ++It) if (It->Struct == FAnimNode_CopyPoseFromMesh::StaticStruct())
   {
    const auto* Node = It->ContainerPtrToValuePtr<FAnimNode_CopyPoseFromMesh>(Defaults); FJson Row = Object();
    Row->SetStringField(TEXT("property"), It->GetName()); Row->SetBoolField(TEXT("use_mesh_pose"), Node->bUseMeshPose);
    Row->SetStringField(TEXT("root_bone_to_copy"), Node->RootBoneToCopy.ToString()); Nodes.Add(Value(Row));
   }
   Evidence->SetStringField(TEXT("runtime_face_class"), FaceClassPath); Evidence->SetArrayField(TEXT("runtime_class_default_copy_pose_nodes"), Nodes);
   Evidence->SetStringField(TEXT("scope"), TEXT("read-only native reference transforms and actual nonzero skin weights/BoneMap; all render LODs, including disabled sections; class defaults, not evaluated runtime pose. S47Waitress sets Body source, attached-parent and curves on instances. Compatibility limits remain unchanged; no face-basis policy or geometry correction accepted here."));
   return Need(BodyValid && FaceValid && ClothValid && Nodes.Num() > 0, TEXT("complete native reference/weight diagnostics and existing waitress CopyPose graph available"));
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
   if (!Need(Clothes->GetLODNum() == 1, TEXT("authored clothes import has exactly its single source LOD0"))) return false;
   const FString Output = Job->GetStringField(TEXT("output_asset"));
   if (!Need(Output.StartsWith(TEXT("/Game/MetaHumans/Mannequins/RadioWoman/")) && !Output.Contains(TEXT("..")), TEXT("private bounded output package"))) return false;
   if (!Need(Clothes->GetSkeleton() != Body->GetSkeleton() && Clothes->GetSkeleton() &&
    Clothes->GetSkeleton()->GetPathName().StartsWith(TEXT("/Game/MetaHumans/Mannequins/RadioWoman/")), TEXT("cloth importer used private skeleton copy"))) return false;
   FJson Inputs = Object(); Inputs->SetObjectField(TEXT("body"), SkeletalSource(Body)); Inputs->SetObjectField(TEXT("face"), SkeletalSource(Face));
   Inputs->SetObjectField(TEXT("clothes"), SkeletalSource(Clothes)); Report->SetObjectField(TEXT("skeletal_sources"), Inputs);
   FJson Cards = Object(); Cards->SetObjectField(TEXT("hair_near"), CardSource(HairNear)); Cards->SetObjectField(TEXT("hair_far"), CardSource(HairFar));
   Cards->SetObjectField(TEXT("brows_near"), CardSource(BrowsNear)); Cards->SetObjectField(TEXT("brows_far"), CardSource(BrowsFar)); Report->SetObjectField(TEXT("card_sources"), Cards);
   return true;
  }
  FString InverseReferenceHash(const USkeletalMesh* Mesh) const
  {
   TArray<uint8> Bytes; FMemoryWriter Writer(Bytes); int32 Count = Mesh->GetRefBasesInvMatrix().Num(); Writer << Count;
   for (const FMatrix44f& Matrix : Mesh->GetRefBasesInvMatrix()) { FMatrix44f Copy = Matrix; Writer << Copy; }
   return Digest(Bytes);
  }
  bool FileState(const FString& Path, FJson& Data)
  {
   TArray<uint8> Bytes;
   if (!Need(FFileHelper::LoadFileToArray(Bytes, *Path) && Bytes.Num() > 0, TEXT("actual saved private file readable for SHA1: ") + Path)) return false;
   Data = Object(); Data->SetStringField(TEXT("path"), FPaths::ConvertRelativePathToFull(Path));
   Data->SetNumberField(TEXT("bytes"), Bytes.Num()); Data->SetStringField(TEXT("sha1"), Digest(Bytes)); return true;
  }
  bool DescriptionHash(FMeshDescription& Description, FString& Hash)
  {
   TArray<uint8> Bytes; FMemoryWriter Writer(Bytes, true); FObjectAndNameAsStringProxyArchive Archive(Writer, false);
   Description.Serialize(Archive); Hash = Digest(Bytes);
   return Need(!Archive.IsError(), TEXT("complete MeshDescription serialized without filtering any geometry or attribute"));
  }
  bool DescriptionState(FJson& Data)
  {
   Data = Object(); Report->SetObjectField(TEXT("clothes_bind_actual_description"), Data);
   const FString ImportedPin = TEXT("2144e1ffc0504791556fd705e21b37add599da6f"), BodyPin = TEXT("93838fa5259ddca476c5e523526483746f038133");
   FMeshDescription* Description = Clothes->GetMeshDescription(0);
   if (!Need(Description != nullptr, TEXT("actual imported clothes MeshDescription available"))) return false;
   FString FullHash; if (!DescriptionHash(*Description, FullHash)) return false;
   Data->SetStringField(TEXT("policy"), TEXT("canonical-body-bone-pose-only")); Data->SetStringField(TEXT("full_sha1"), FullHash);
   const FSkeletalMeshAttributesShared Attributes(*Description); const FReferenceSkeleton& BodyRef = Body->GetRefSkeleton();
   if (!Need(Attributes.HasBones() && Attributes.GetNumBones() == 342 && Attributes.HasBoneNameAttribute() &&
    Attributes.HasBoneParentIndexAttribute() && Attributes.HasBonePoseAttribute() && FPose(BodyRef).Hash() == BodyPin,
    TEXT("complete 342-bone Description metadata and exact pinned canonical Body reference"))) return false;
   const auto Names = Attributes.GetBoneNames(); const auto Parents = Attributes.GetBoneParentIndices(); const auto Poses = Attributes.GetBonePoses();
   if (!Need(Names.IsValid() && Parents.IsValid() && Poses.IsValid(), TEXT("actual typed Description bone-name/parent/pose attributes available"))) return false;
   TArray<FTransform> OriginalPoses; TArray<uint8> ReferenceBytes; FMemoryWriter ReferenceWriter(ReferenceBytes);
   for (int32 Index = 0; Index < 342; ++Index)
   {
    const FBoneID BoneID(Index);
    if (!Need(Attributes.IsBoneValid(BoneID) && Names.Get(BoneID) == BodyRef.GetBoneName(Index) && Parents.Get(BoneID) == BodyRef.GetParentIndex(Index),
     FString::Printf(TEXT("exact Description bone name/index/parent %d"), Index))) return false;
    FString Name = Names.Get(BoneID).ToString(); int32 Parent = Parents.Get(BoneID); FTransform Pose = Poses.Get(BoneID);
    OriginalPoses.Add(Pose); ReferenceWriter << Name << Parent << Pose;
   }
   const FString ReferenceHash = Digest(ReferenceBytes); Data->SetStringField(TEXT("reference_pose_sha1"), ReferenceHash); Data->SetNumberField(TEXT("bones"), 342);
   if (!Need(ReferenceHash == ImportedPin || ReferenceHash == BodyPin, TEXT("actual full Description bone metadata matches exact pinned import or canonical Body reference"))) return false;
   // This is a detached deep copy. Do not Register attributes, commit source
   // data, or edit the native asset. Only its existing 342 Bone::Pose values
   // are replaced. Whole-description hashes retain every other attribute.
   FMeshDescription Canonical(*Description); FString CopyBefore;
   if (!DescriptionHash(Canonical, CopyBefore) || !Need(CopyBefore == FullHash, TEXT("detached Description copy initially retains every serialized byte"))) return false;
   FSkeletalMeshAttributes CopyAttributes(Canonical); auto CopyPoses = CopyAttributes.GetBonePoses();
   for (int32 Index = 0; Index < 342; ++Index) CopyPoses.Set(FBoneID(Index), BodyRef.GetRefBonePose()[Index]);
   FString CanonicalHash; if (!DescriptionHash(Canonical, CanonicalHash)) return false;
   Data->SetStringField(TEXT("canonical_full_sha1"), CanonicalHash); Data->SetStringField(TEXT("canonical_reference_pose_sha1"), BodyPin);
   FString UntouchedDuringCopy;
   if (!DescriptionHash(*Description, UntouchedDuringCopy) || !Need(UntouchedDuringCopy == FullHash,
    TEXT("actual Description stays byte-identical while the detached copy holds canonical bone poses"))) return false;
   // Reverse exactly those replacements to prove the detached operation leaves
   // all non-pose data intact; rehash the real Description to reject aliasing.
   for (int32 Index = 0; Index < 342; ++Index) CopyPoses.Set(FBoneID(Index), OriginalPoses[Index]);
   FString RestoredHash, UntouchedHash;
   if (!DescriptionHash(Canonical, RestoredHash) || !DescriptionHash(*Description, UntouchedHash) ||
    !Need(RestoredHash == FullHash && UntouchedHash == FullHash, TEXT("only detached Bone::Pose replacements occurred; real Description and every other attribute remain exact"))) return false;
   Data->SetBoolField(TEXT("detached_roundtrip_preserved"), true); Data->SetBoolField(TEXT("actual_preserved_during_copy"), true);
   return Need(Mode != TEXT("verify-clothes-bind") || (ReferenceHash == BodyPin && FullHash == CanonicalHash),
    TEXT("fresh saved reader has actual canonical Body bone metadata and the entire canonical Description without mutating it"));
  }
  bool GeometryState(FJson& Data, FJson& DescriptionData)
  {
   FSkeletalMeshModel* Model = Clothes->GetImportedModel(); FSkeletalMeshRenderData* Render = Clothes->GetResourceForRendering();
   if (!Need(Model && Model->LODModels.Num() == 1 && Render && Render->LODRenderData.Num() == 1 &&
    Clothes->GetMorphTargets().IsEmpty() && Clothes->GetSkinWeightProfiles().IsEmpty() && Model->LODModels[0].SkinWeightProfiles.IsEmpty(),
    TEXT("bounded single-LOD clothes has complete import/render data without morphs or alternate skin-weight profiles"))) return false;
   Data = Object();
   // Use the engine's complete editor importer serialization, including soft
   // vertices, all influences, source sections/BoneMaps, maps and attributes.
   // Name/object paths, rather than process-local name indices, make the hash
   // comparable in the separate saved reader. No loading archive is used here.
   TArray<uint8> ImportedBytes; FMemoryWriter ImportedWriter(ImportedBytes, true);
   FObjectAndNameAsStringProxyArchive ImportedArchive(ImportedWriter, false); Model->Serialize(ImportedArchive, Clothes);
   // Model::Serialize does not contain the derived LOD vertex-attribute table.
   // Preserve it explicitly as well, in stable name order.
   TArray<FString> AttributeNames;
   for (const auto& Attribute : Model->LODModels[0].VertexAttributes) AttributeNames.Add(Attribute.Key.ToString());
   AttributeNames.Sort(); ImportedArchive << AttributeNames;
   for (const FString& Name : AttributeNames)
   {
    auto& Attribute = Model->LODModels[0].VertexAttributes.FindChecked(FName(*Name));
    uint8 Type = uint8(Attribute.DataType); ImportedArchive << Type << Attribute.ComponentCount << Attribute.Values;
   }
   if (!Need(!ImportedArchive.IsError(), TEXT("complete imported LOD model serialized for preservation"))) return false;
   Data->SetStringField(TEXT("imported_model_sha1"), Digest(ImportedBytes));
   if (!DescriptionState(DescriptionData)) return false;
   Data->SetStringField(TEXT("mesh_description_sha1"), DescriptionData->GetStringField(TEXT("canonical_full_sha1")));
   TArray<uint8> RenderBytes; FMemoryWriter RenderWriter(RenderBytes, true);
   FObjectAndNameAsStringProxyArchive RenderArchive(RenderWriter, false);
   const auto Buffer = [&](const void* Pointer, int64 Size)
   {
    RenderArchive << Size;
    if (!Need(Size >= 0 && (Size == 0 || Pointer), TEXT("native CPU payload present for complete buffer preservation"))) return false;
    if (Size) RenderArchive.Serialize(const_cast<void*>(Pointer), Size);
    return true;
   };
   FSkeletalMeshLODRenderData& LOD = Render->LODRenderData[0];
   const FPositionVertexBuffer& Positions = LOD.StaticVertexBuffers.PositionVertexBuffer;
   const FStaticMeshVertexBuffer& Vertices = LOD.StaticVertexBuffers.StaticMeshVertexBuffer;
   const FColorVertexBuffer& Colors = LOD.StaticVertexBuffers.ColorVertexBuffer;
   const FSkinWeightVertexBuffer& Weights = LOD.SkinWeightVertexBuffer;
   uint32 VertexCount = LOD.GetNumVertices(), UVCount = Vertices.GetNumTexCoords(), ColorCount = Colors.GetNumVertices();
   uint32 PositionStride = Positions.GetStride(), ColorStride = Colors.GetStride();
   bool FullUV = Vertices.GetUseFullPrecisionUVs(), HighTangents = Vertices.GetUseHighPrecisionTangentBasis();
   bool VariableWeights = Weights.GetVariableBonesPerVertex(); uint32 BoneBytes = Weights.GetBoneIndexByteSize(), WeightBytes = Weights.GetBoneWeightByteSize();
   uint32 WeightVertices = Weights.GetNumVertices(), MaxInfluences = Weights.GetMaxBoneInfluences();
   RenderArchive << VertexCount << UVCount << ColorCount << PositionStride << ColorStride << FullUV << HighTangents;
   RenderArchive << VariableWeights << BoneBytes << WeightBytes << WeightVertices << MaxInfluences;
   if (!Need(VertexCount > 0 && Positions.GetNumVertices() == VertexCount && Vertices.GetNumVertices() == VertexCount && WeightVertices == VertexCount,
    TEXT("all original render and weight vertices present"))) return false;
   if (!Buffer(Positions.GetVertexData(), int64(Positions.GetNumVertices()) * PositionStride) ||
    !Buffer(Vertices.GetTangentData(), Vertices.GetTangentSize()) || !Buffer(Vertices.GetTexCoordData(), Vertices.GetTexCoordSize()) ||
    !Buffer(Colors.GetVertexData(), int64(ColorCount) * ColorStride) ||
    !Buffer(Weights.GetDataVertexBuffer()->GetWeightData(), Weights.GetDataVertexBuffer()->GetVertexDataSize()) ||
    !Buffer(Weights.GetLookupVertexBuffer()->GetLookupData(), Weights.GetLookupVertexBuffer()->GetVertexDataSize())) return false;
   const FRawStaticIndexBuffer16or32Interface* Indices = LOD.MultiSizeIndexContainer.GetIndexBuffer();
   if (!Need(Indices && Indices->Num() > 0, TEXT("complete native index buffer available"))) return false;
   uint8 IndexBytes = LOD.MultiSizeIndexContainer.GetDataTypeSize(); int32 IndexCount = Indices->Num(); RenderArchive << IndexBytes << IndexCount;
   for (int32 Index = 0; Index < IndexCount; ++Index) { uint32 Vertex = Indices->Get(Index); RenderArchive << Vertex; }
   RenderArchive << LOD.RenderSections << LOD.ActiveBoneIndices << LOD.RequiredBones;
   int32 UnifiedCount = LOD.UnifiedBoneMap.Num(); RenderArchive << UnifiedCount;
   for (FBoneIndexType Bone : LOD.UnifiedBoneMap) RenderArchive << Bone;
   LOD.VertexAttributeBuffers.Serialize(RenderArchive);
   if (!Need(!RenderArchive.IsError(), TEXT("native render/weight/section payload serialized for preservation"))) return false;
   Data->SetStringField(TEXT("render_buffers_sha1"), Digest(RenderBytes)); Data->SetNumberField(TEXT("vertices"), VertexCount);
   Data->SetNumberField(TEXT("triangles"), IndexCount / 3); Data->SetNumberField(TEXT("sections"), LOD.RenderSections.Num());
   TArray<uint8> MaterialBytes; FMemoryWriter MaterialWriter(MaterialBytes, true);
   FObjectAndNameAsStringProxyArchive MaterialArchive(MaterialWriter, false);
   int32 MaterialCount = Clothes->GetMaterials().Num(); MaterialArchive << MaterialCount;
   for (FSkeletalMaterial& Material : Clothes->GetMaterials()) FSkeletalMaterial::StaticStruct()->SerializeItem(MaterialArchive, &Material, nullptr);
   FSkeletalMeshLODInfo* Info = Clothes->GetLODInfo(0);
   if (!Need(Info != nullptr, TEXT("original source LOD material map and settings available"))) return false;
   FSkeletalMeshLODInfo::StaticStruct()->SerializeItem(MaterialArchive, Info, nullptr);
   if (!Need(!MaterialArchive.IsError(), TEXT("original material slots and LOD map serialized for preservation"))) return false;
   Data->SetStringField(TEXT("materials_lod_info_sha1"), Digest(MaterialBytes));
   FString Skeleton = Clothes->GetSkeleton()->GetPathName(); Data->SetStringField(TEXT("skeleton"), Skeleton);
   TArray<uint8> PayloadBytes; FMemoryWriter PayloadWriter(PayloadBytes);
   for (const TCHAR* Key : {TEXT("imported_model_sha1"), TEXT("mesh_description_sha1"), TEXT("render_buffers_sha1"), TEXT("materials_lod_info_sha1")})
   { FString Hash = Data->GetStringField(Key); PayloadWriter << Hash; }
   PayloadWriter << Skeleton; Data->SetStringField(TEXT("payload_sha1"), Digest(PayloadBytes)); return true;
  }
  bool ClothesBinding(bool Verify)
  {
   const FString ImportedPin = TEXT("2144e1ffc0504791556fd705e21b37add599da6f"), BodyPin = TEXT("93838fa5259ddca476c5e523526483746f038133");
   const FJson* BindingContract = nullptr;
   if (!Need(Contract->TryGetObjectField(TEXT("clothes_bind"), BindingContract) &&
    (*BindingContract)->GetStringField(TEXT("policy")) == TEXT("canonical-native-body") &&
    (*BindingContract)->GetStringField(TEXT("imported_reference_pose_sha1")) == ImportedPin &&
    (*BindingContract)->GetStringField(TEXT("canonical_reference_pose_sha1")) == BodyPin,
    TEXT("explicit measured clothes-import and canonical native body reference pins"))) return false;
   const FString Asset = Job->GetStringField(TEXT("clothes_asset")), Skeleton = Job->GetStringField(TEXT("private_skeleton"));
   const FString Root = FPackageName::GetLongPackagePath(Job->GetStringField(TEXT("output_asset")));
   if (!Need(Root.StartsWith(TEXT("/Game/MetaHumans/Mannequins/RadioWoman/")) && !Root.Contains(TEXT("..")) &&
    FPackageName::GetLongPackagePath(Asset) == Root && FPackageName::GetLongPackagePath(Skeleton) == Root &&
    Clothes->GetOutermost()->GetName() == Asset && Clothes->GetSkeleton()->GetOutermost()->GetName() == Skeleton &&
    Clothes->GetOutermost() != Body->GetOutermost() && Clothes->GetOutermost() != Clothes->GetSkeleton()->GetOutermost(),
    TEXT("only this run's newly imported clothes package is a permitted bind/save target; private skeleton stays separate"))) return false;
   const FReferenceSkeleton& BodyRef = Body->GetRefSkeleton(); const FReferenceSkeleton& ClothRef = Clothes->GetRefSkeleton();
   if (!Need(BodyRef.GetNum() == 342 && BodyRef.GetRawBoneNum() == 342 && ClothRef.GetNum() == 342 && ClothRef.GetRawBoneNum() == 342,
    TEXT("exact measured native 342-bone body and cloth hierarchies"))) return false;
   for (int32 Index = 0; Index < 342; ++Index)
    if (!Need(ClothRef.GetBoneName(Index) == BodyRef.GetBoneName(Index) && ClothRef.GetParentIndex(Index) == BodyRef.GetParentIndex(Index),
     FString::Printf(TEXT("exact cloth/body bone name, index and parent %d"), Index))) return false;
   const FString BodyReference = FPose(BodyRef).Hash(), BodyInverse = InverseReferenceHash(Body);
   if (!Need(BodyReference == BodyPin && Body->GetRefBasesInvMatrix().Num() == 342, TEXT("unchanged measured canonical Body reference and complete inverse bind matrices"))) return false;
   FJson Before, DescriptionBefore; if (!GeometryState(Before, DescriptionBefore)) return false;
   const FString Filename = FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(Asset, FPackageName::GetAssetPackageExtension()));
   FJson FileBefore; if (!FileState(Filename, FileBefore)) return false;
   if (Verify)
   {
    // Keep actual fresh-process evidence outside the immutable bind child.
    // It must survive an early receipt mismatch, before the child is updated.
    Report->SetObjectField(TEXT("clothes_bind_actual_geometry_before"), Before);
    FJson Actual = Object(); Actual->SetStringField(TEXT("asset"), Asset); Actual->SetStringField(TEXT("object"), Clothes->GetPathName());
    Actual->SetStringField(TEXT("private_skeleton"), Skeleton); Actual->SetStringField(TEXT("reference_sha1"), FPose(ClothRef).Hash());
    Actual->SetStringField(TEXT("inverse_reference_sha1"), InverseReferenceHash(Clothes)); Actual->SetStringField(TEXT("body_reference_sha1"), BodyReference);
    Actual->SetStringField(TEXT("body_inverse_reference_sha1"), BodyInverse); Actual->SetNumberField(TEXT("bones"), ClothRef.GetNum());
    Actual->SetNumberField(TEXT("inverse_matrices"), Clothes->GetRefBasesInvMatrix().Num()); Actual->SetBoolField(TEXT("package_dirty"), Clothes->GetOutermost()->IsDirty());
    Actual->SetObjectField(TEXT("file"), FileBefore); Report->SetObjectField(TEXT("clothes_bind_actual_before"), Actual);
   }
   FJson Data = Object(); Report->SetObjectField(TEXT("clothes_bind"), Data);
   if (Verify)
   {
    const FString ReceiptPath = Job->GetStringField(TEXT("clothes_bind_report")); FJson Previous;
    if (!Need(FPaths::ConvertRelativePathToFull(ReceiptPath) != FPaths::ConvertRelativePathToFull(ReportPath) && Read(ReceiptPath, Previous) &&
     Previous->GetIntegerField(TEXT("schema_version")) == 1 && Previous->GetStringField(TEXT("status")) == TEXT("PASS") &&
     Previous->GetStringField(TEXT("mode")) == TEXT("bind-clothes") && Previous->GetNumberField(TEXT("pid")) > 0 &&
     Previous->GetNumberField(TEXT("pid")) != FPlatformProcess::GetCurrentProcessId(), TEXT("successful original bind receipt from a different native process"))) return false;
    const FJson Saved = Previous->GetObjectField(TEXT("clothes_bind")); Data->Values = Saved->Values;
    const FJson* ExpectedDescription = nullptr; const FJson* HistoricalBefore = nullptr; const FJson* HistoricalAfter = nullptr;
    if (!Need(Saved->TryGetObjectField(TEXT("description_saved_expected"), ExpectedDescription) &&
     Saved->TryGetObjectField(TEXT("description_before"), HistoricalBefore) && Saved->TryGetObjectField(TEXT("description_after"), HistoricalAfter) &&
     (*ExpectedDescription)->GetStringField(TEXT("policy")) == TEXT("canonical-body-bone-pose-only") &&
     (*ExpectedDescription)->GetIntegerField(TEXT("bones")) == 342 && (*ExpectedDescription)->GetStringField(TEXT("reference_pose_sha1")) == BodyPin &&
     (*HistoricalBefore)->GetStringField(TEXT("canonical_full_sha1")) == (*ExpectedDescription)->GetStringField(TEXT("full_sha1")) &&
     (*HistoricalAfter)->GetStringField(TEXT("canonical_full_sha1")) == (*ExpectedDescription)->GetStringField(TEXT("full_sha1")) &&
     (*HistoricalBefore)->GetBoolField(TEXT("detached_roundtrip_preserved")) && (*HistoricalAfter)->GetBoolField(TEXT("detached_roundtrip_preserved")) &&
     (*HistoricalBefore)->GetBoolField(TEXT("actual_preserved_during_copy")) && (*HistoricalAfter)->GetBoolField(TEXT("actual_preserved_during_copy")) &&
     (*ExpectedDescription)->GetStringField(TEXT("full_sha1")) == DescriptionBefore->GetStringField(TEXT("full_sha1")) &&
     DescriptionBefore->GetStringField(TEXT("reference_pose_sha1")) == BodyPin,
     TEXT("actual entire saved Description exactly matches the bind's proven canonical-only metadata copy and Body reference"))) return false;
    FJson Matches = Object(); const FJson SavedGeometry = Saved->GetObjectField(TEXT("geometry_before"));
    for (const auto& Entry : Before->Values)
    {
     const TSharedPtr<FJsonValue>* Expected = SavedGeometry->Values.Find(Entry.Key);
     Matches->SetBoolField(Entry.Key, Expected && FJsonValue::CompareEqual(**Expected, *Entry.Value));
    }
    Matches->SetBoolField(TEXT("asset"), Saved->GetStringField(TEXT("asset")) == Asset);
    Matches->SetBoolField(TEXT("object"), Saved->GetStringField(TEXT("object")) == Clothes->GetPathName());
    Matches->SetBoolField(TEXT("private_skeleton"), Saved->GetStringField(TEXT("private_skeleton")) == Skeleton);
    Matches->SetBoolField(TEXT("reference_after_sha1"), Saved->GetStringField(TEXT("reference_after_sha1")) == FPose(ClothRef).Hash());
    Matches->SetBoolField(TEXT("inverse_reference_after_sha1"), Saved->GetStringField(TEXT("inverse_reference_after_sha1")) == InverseReferenceHash(Clothes));
    Matches->SetBoolField(TEXT("file_after"), FJsonValue::CompareEqual(*Value(Saved->GetObjectField(TEXT("file_after"))), *Value(FileBefore)));
    Report->SetObjectField(TEXT("clothes_bind_verify_matches"), Matches);
    if (!Need(Saved->GetStringField(TEXT("asset")) == Asset && Saved->GetStringField(TEXT("object")) == Clothes->GetPathName() &&
     Saved->GetStringField(TEXT("private_skeleton")) == Skeleton && Saved->GetStringField(TEXT("policy")) == TEXT("canonical-native-body") &&
     Saved->GetStringField(TEXT("imported_reference_pose_sha1")) == ImportedPin && Saved->GetStringField(TEXT("canonical_reference_pose_sha1")) == BodyPin &&
     Saved->GetStringField(TEXT("reference_before_sha1")) == ImportedPin && Saved->GetBoolField(TEXT("preserved_payload")) &&
     Saved->GetStringField(TEXT("payload_before_sha1")) == Before->GetStringField(TEXT("payload_sha1")) &&
     FJsonValue::CompareEqual(*Value(Saved->GetObjectField(TEXT("geometry_before"))), *Value(Before)),
     TEXT("original bind receipt pins this exact private mesh and its complete unchanged imported/render payload"))) return false;
    Data->SetStringField(TEXT("reference_after_sha1"), FPose(ClothRef).Hash()); Data->SetStringField(TEXT("body_reference_sha1"), BodyReference);
    Data->SetStringField(TEXT("inverse_reference_after_sha1"), InverseReferenceHash(Clothes)); Data->SetStringField(TEXT("body_inverse_reference_sha1"), BodyInverse);
    Data->SetStringField(TEXT("payload_after_sha1"), Before->GetStringField(TEXT("payload_sha1"))); Data->SetObjectField(TEXT("geometry_after"), Before);
    Data->SetObjectField(TEXT("file_after"), FileBefore);
    if (!Need(FPose(ClothRef).Hash() == BodyPin && InverseReferenceHash(Clothes) == BodyInverse &&
     Clothes->GetRefBasesInvMatrix().Num() == 342 && FJsonValue::CompareEqual(*Value(Data), *Value(Saved)) && !Clothes->GetOutermost()->IsDirty(),
     TEXT("fresh saved cloth reference, inverse matrices, all payload fields and actual file SHA1 match original bind receipt without mutation"))) return false;
    FJson ReceiptFile; if (!FileState(ReceiptPath, ReceiptFile)) return false; Report->SetObjectField(TEXT("bind_receipt"), ReceiptFile);
    FJson FileAfter; return FileState(Filename, FileAfter) && Need(FJsonValue::CompareEqual(*Value(FileBefore), *Value(FileAfter)), TEXT("read-only bind verification leaves actual cloth package byte-identical"));
   }
   if (!Need(FPose(ClothRef).Hash() == ImportedPin && Clothes->GetRefBasesInvMatrix().Num() == 342,
    TEXT("only the exact measured FBX-import reference pose may be canonicalized once"))) return false;
   Data->SetStringField(TEXT("asset"), Asset); Data->SetStringField(TEXT("object"), Clothes->GetPathName()); Data->SetStringField(TEXT("private_skeleton"), Skeleton);
   Data->SetStringField(TEXT("policy"), TEXT("canonical-native-body")); Data->SetStringField(TEXT("imported_reference_pose_sha1"), ImportedPin);
   Data->SetStringField(TEXT("canonical_reference_pose_sha1"), BodyPin); Data->SetStringField(TEXT("reference_before_sha1"), FPose(ClothRef).Hash());
   Data->SetStringField(TEXT("inverse_reference_before_sha1"), InverseReferenceHash(Clothes)); Data->SetStringField(TEXT("body_reference_sha1"), BodyReference);
   Data->SetStringField(TEXT("body_inverse_reference_sha1"), BodyInverse); Data->SetStringField(TEXT("payload_before_sha1"), Before->GetStringField(TEXT("payload_sha1")));
   Data->SetObjectField(TEXT("geometry_before"), Before); Data->SetObjectField(TEXT("file_before"), FileBefore);
   Data->SetObjectField(TEXT("description_before"), DescriptionBefore);
   FJson ExpectedDescription = Object(); ExpectedDescription->SetStringField(TEXT("policy"), TEXT("canonical-body-bone-pose-only"));
   ExpectedDescription->SetNumberField(TEXT("bones"), 342); ExpectedDescription->SetStringField(TEXT("reference_pose_sha1"), BodyPin);
   ExpectedDescription->SetStringField(TEXT("full_sha1"), DescriptionBefore->GetStringField(TEXT("canonical_full_sha1")));
   Data->SetObjectField(TEXT("description_saved_expected"), ExpectedDescription);
   // SetRefSkeleton alone neither clears nor recomputes the inverse matrices.
   // CalculateInvRefMatrices deliberately does nothing while the count matches.
   Clothes->SetRefSkeleton(BodyRef); Clothes->GetRefBasesInvMatrix().Reset(); Clothes->CalculateInvRefMatrices();
   Data->SetStringField(TEXT("reference_after_sha1"), FPose(Clothes->GetRefSkeleton()).Hash());
   Data->SetStringField(TEXT("inverse_reference_after_sha1"), InverseReferenceHash(Clothes));
   if (!Need(FPose(Clothes->GetRefSkeleton()).Hash() == BodyPin && InverseReferenceHash(Clothes) == BodyInverse &&
    Clothes->GetRefBasesInvMatrix().Num() == 342, TEXT("native canonical reference and recomputed inverse bind matrices match Body exactly"))) return false;
   for (int32 Index = 0; Index < 342; ++Index)
    if (!Need(SameReference(Clothes->GetRefSkeleton().GetRefBonePose()[Index], BodyRef.GetRefBonePose()[Index]),
     FString::Printf(TEXT("strict unchanged cloth/body reference tolerance after canonical bind %d"), Index))) return false;
   FJson After, DescriptionAfter; if (!GeometryState(After, DescriptionAfter)) return false;
   Data->SetObjectField(TEXT("description_after"), DescriptionAfter);
   Data->SetObjectField(TEXT("geometry_after"), After); Data->SetStringField(TEXT("payload_after_sha1"), After->GetStringField(TEXT("payload_sha1")));
   if (!Need(FJsonValue::CompareEqual(*Value(Before), *Value(After)), TEXT("canonical bind changes no imported geometry, UVs, normals, tangents, weights, indices, BoneMaps, sections or materials"))) return false;
   Clothes->MarkPackageDirty(); FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
   if (!Need(UPackage::SavePackage(Clothes->GetOutermost(), Clothes, *Filename, Args), TEXT("only new private Clothes package saved after canonical bind"))) return false;
   FAssetCompilingManager::Get().FinishAllCompilation();
   if (!GeometryState(After, DescriptionAfter)) return false;
   Data->SetObjectField(TEXT("description_after"), DescriptionAfter);
   Data->SetObjectField(TEXT("geometry_after"), After); Data->SetStringField(TEXT("payload_after_sha1"), After->GetStringField(TEXT("payload_sha1")));
   if (!Need(FJsonValue::CompareEqual(*Value(Before), *Value(After)) && FPose(Clothes->GetRefSkeleton()).Hash() == BodyPin &&
    InverseReferenceHash(Clothes) == BodyInverse && FPose(BodyRef).Hash() == BodyReference && InverseReferenceHash(Body) == BodyInverse,
    TEXT("post-save cloth retains full payload and canonical reference; actual Body remains unchanged"))) return false;
   FJson FileAfter; if (!FileState(Filename, FileAfter)) return false;
   Data->SetObjectField(TEXT("file_after"), FileAfter); Data->SetObjectField(TEXT("geometry_after"), After);
   Data->SetStringField(TEXT("reference_after_sha1"), FPose(Clothes->GetRefSkeleton()).Hash()); Data->SetStringField(TEXT("inverse_reference_after_sha1"), InverseReferenceHash(Clothes));
   Data->SetStringField(TEXT("payload_after_sha1"), After->GetStringField(TEXT("payload_sha1"))); Data->SetBoolField(TEXT("preserved_payload"), true); return true;
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
    Posed.Set(Upper, UpperPose); Posed.Set(Lower, LowerPose);
    // SolveTwoBoneIK deliberately retains the end bone's old WORLD rotation.
    // Keep the native local hand transform instead, so wrist/fingers follow the
    // newly posed forearm. A radio grip must be authored/reviewed separately.
    const bool LocalHandRetained = Posed.Local[Hand].Equals(Reference.Local[Hand], 0.000001);
    const double TargetError = FVector::Distance(Posed.Component[Hand].GetLocation(), Target);
    FJson Item = Object(); Item->SetStringField(TEXT("side"), IsLeft ? TEXT("left") : TEXT("right"));
    Item->SetArrayField(TEXT("target_native_cm"), Vector(Target)); Item->SetNumberField(TEXT("error_cm"), TargetError); IK.Add(Value(Item));
    Item->SetBoolField(TEXT("reference_local_hand_retained"), LocalHandRetained);
    Item->SetStringField(TEXT("reference_local_hand"), Reference.Local[Hand].ToString()); Item->SetStringField(TEXT("actual_local_hand"), Posed.Local[Hand].ToString());
    Item->SetStringField(TEXT("actual_component_hand"), Posed.Component[Hand].ToString());
    if (!Need(LocalHandRetained && TargetError <= PoseNumber(TEXT("max_hand_target_error_cm")), TEXT("non-stretched hand target reached with native reference-local wrist orientation"))) return false;
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
   if (!IsClothes) return CopyFace(Authoritative, Target);
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
  bool CopyFace(const FPose& Authoritative, FPose& Target)
  {
   const FJson Diagnostic = Report->GetObjectField(TEXT("reference_diagnostics"));
   const auto& Nodes = Diagnostic->GetArrayField(TEXT("runtime_class_default_copy_pose_nodes"));
   if (!Need(Nodes.Num() == 1 && !Nodes[0]->AsObject()->GetBoolField(TEXT("use_mesh_pose")) &&
    Nodes[0]->AsObject()->GetStringField(TEXT("root_bone_to_copy")) == TEXT("None"), TEXT("existing waitress graph uses one unrestricted local-space CopyPose"))) return false;
   const FPose BodyReference(Authoritative.Ref), FaceReference(Target.Ref); FPose ReferenceCopy(Target.Ref);
   const auto& WeightRows = Diagnostic->GetObjectField(TEXT("face"))->GetArrayField(TEXT("bones"));
   if (!Need(WeightRows.Num() == Target.Local.Num(), TEXT("complete measured face weights correspond to native reference skeleton"))) return false;
   // R5 measured four different, unweighted lower-spine reference positions. The
   // longer Face spine_04 LOCAL transform compensates them, so all weighted CS
   // frames agree with Body. Copy the COMPLETE common parent chain, as the actual
   // ABP node does; skipping those ancestors would lose the spine_04 compensation.
   const TArray<FName> Bridge = {TEXT("pelvis"),TEXT("spine_01"),TEXT("spine_02"),TEXT("spine_03")};
   TArray<TSharedPtr<FJsonValue>> Mapping, Exceptions; int32 Unmapped = 0;
   for (int32 Index = 0; Index < Target.Local.Num(); ++Index)
   {
    const FName Name = Target.Ref.GetBoneName(Index); const int32 SourceIndex = Authoritative.Ref.FindBoneIndex(Name);
    if (SourceIndex == INDEX_NONE) { ++Unmapped; continue; }
    const int32 SourceParent = Authoritative.Ref.GetParentIndex(SourceIndex), TargetParent = Target.Ref.GetParentIndex(Index);
    const FName SourceName = SourceParent == INDEX_NONE ? NAME_None : Authoritative.Ref.GetBoneName(SourceParent);
    const FName TargetName = TargetParent == INDEX_NONE ? NAME_None : Target.Ref.GetBoneName(TargetParent);
    if (!Need(SourceName == TargetName, FString(TEXT("complete same-name CopyPose parent chain: ")) + Name.ToString())) return false;
    if (!SameReference(BodyReference.Component[SourceIndex], FaceReference.Component[Index]))
    {
     const FTransform& SourceRef = BodyReference.Component[SourceIndex]; const FTransform& FaceRef = FaceReference.Component[Index];
     if (!Need(Bridge.Contains(Name) && WeightRows[Index]->AsObject()->GetIntegerField(TEXT("nonzero_vertex_influences_all_lods")) == 0 &&
      SourceRef.GetScale3D().Equals(FaceRef.GetScale3D(), 0.00001) &&
      FMath::RadiansToDegrees(SourceRef.GetRotation().AngularDistance(FaceRef.GetRotation())) <= PoseNumber(TEXT("max_shared_reference_angle_degrees")) &&
      SameReference(SourceRef, Authoritative.Component[SourceIndex]), FString(TEXT("measured unweighted stationary lower-spine bind bridge only: ")) + Name.ToString())) return false;
     FJson Row = Object(); Row->SetStringField(TEXT("bone"), Name.ToString());
     Row->SetObjectField(TEXT("face_ref_relative_to_body_ref"), TransformData(FaceRef.GetRelativeTransform(SourceRef))); Exceptions.Add(Value(Row));
    }
    // Match FAnimNode_CopyPoseFromMesh::Evaluate_AnyThread, bUseMeshPose=false.
    Target.Local[Index] = SourceParent != INDEX_NONE && TargetParent != INDEX_NONE
     ? Authoritative.Component[SourceIndex].GetRelativeTransform(Authoritative.Component[SourceParent]) : Authoritative.Component[SourceIndex];
    ReferenceCopy.Local[Index] = SourceParent != INDEX_NONE && TargetParent != INDEX_NONE
     ? BodyReference.Component[SourceIndex].GetRelativeTransform(BodyReference.Component[SourceParent]) : BodyReference.Component[SourceIndex];
    Mapping.Add(MakeShared<FJsonValueString>(Name.ToString()));
   }
   Target.Rebuild(); ReferenceCopy.Rebuild(); int32 Weighted = 0;
   for (int32 Index = 0; Index < Target.Local.Num(); ++Index)
   {
    const FName Name = Target.Ref.GetBoneName(Index); const int32 SourceIndex = Authoritative.Ref.FindBoneIndex(Name);
    if (SourceIndex == INDEX_NONE)
    {
     if (!Need(Target.Local[Index].Equals(FaceReference.Local[Index], 0.000001), TEXT("unmapped facial branches retain exact native reference local transforms"))) return false;
    }
    else if (!Need(SameReference(Target.Component[Index], Authoritative.Component[SourceIndex]),
     FString(TEXT("local CopyPose equals complete mapped body component frame: ")) + Name.ToString())) return false;
    if (WeightRows[Index]->AsObject()->GetIntegerField(TEXT("nonzero_vertex_influences_all_lods")) > 0)
    {
     ++Weighted;
     if (!Need(SameReference(ReferenceCopy.Component[Index], FaceReference.Component[Index]),
      FString(TEXT("whole-chain reference copy preserves actual weighted face bind frame: ")) + Name.ToString())) return false;
    }
   }
   for (const TCHAR* Name : {TEXT("spine_04"),TEXT("spine_05"),TEXT("clavicle_l"),TEXT("clavicle_r"),TEXT("neck_01"),TEXT("neck_02"),TEXT("head")})
   {
    const int32 Index = Target.Ref.FindBoneIndex(Name), SourceIndex = Authoritative.Ref.FindBoneIndex(Name);
    if (!Need(Index != INDEX_NONE && SourceIndex != INDEX_NONE &&
     SameReference(ReferenceCopy.Component[Index], FaceReference.Component[Index]) &&
     SameReference(Target.Component[Index], Authoritative.Component[SourceIndex]), FString(TEXT("native shoulder/neck/head seam anchor retained: ")) + Name)) return false;
   }
   if (!Need(Exceptions.Num() == Bridge.Num() && Weighted > 0, TEXT("exact four measured bind bridges and actual weighted face frames checked"))) return false;
   const int32 Head = Target.Ref.FindBoneIndex(TEXT("head")); const FTransform Delta = FaceReference.Component[Head].Inverse() * Target.Component[Head];
   if (!Need(SameReference(FaceReference.Component[Head] * Delta, Target.Component[Head]), TEXT("rigid card head delta closes against the actual copied face head frame"))) return false;
   FaceReferenceCopyLocal = ReferenceCopy.Local;
   FJson Data = Object(); Data->SetStringField(TEXT("method"), TEXT("measured ABP_Face_Waitress local-space CopyPose; complete common parent chain, native local refs for unmapped facial branches"));
   Data->SetArrayField(TEXT("names"), Mapping); Data->SetArrayField(TEXT("compensated_unweighted_bind_bridges"), Exceptions);
   Data->SetNumberField(TEXT("shared_bones"), Mapping.Num()); Data->SetNumberField(TEXT("unmapped_reference_branches"), Unmapped); Data->SetNumberField(TEXT("weighted_bind_frames_checked"), Weighted);
   Data->SetObjectField(TEXT("native_face_reference_head"), TransformData(FaceReference.Component[Head]));
   Data->SetObjectField(TEXT("reference_copy_head"), TransformData(ReferenceCopy.Component[Head]));
   Data->SetObjectField(TEXT("posed_head"), TransformData(Target.Component[Head])); Data->SetObjectField(TEXT("rigid_cards_head_delta"), TransformData(Delta));
   Data->SetStringField(TEXT("pose_sha1"), Target.Hash()); Data->SetStringField(TEXT("reference_copy_pose_sha1"), ReferenceCopy.Hash());
   Data->SetStringField(TEXT("limit"), TEXT("native frame/CopyPose proof; real-RHI bake additionally requires all-vertex native face bind closure; visual seam/neutral-face review remains separate"));
   Report->SetObjectField(TEXT("face_pose_transfer"), Data); return true;
  }
  UPoseableMeshComponent* Component(USkeletalMesh* Mesh, const FPose& Posed, const TCHAR* Name)
  {
   UPoseableMeshComponent* Part = NewObject<UPoseableMeshComponent>(Actor, Name, RF_Transient);
   Part->SetSkinnedAssetAndUpdate(Mesh); Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   Part->SetComponentTickEnabled(false); Actor->AddInstanceComponent(Part); Part->RegisterComponent();
   Part->BoneSpaceTransforms = Posed.Local; Part->RefreshBoneTransforms();
   // GetCPUSkinnedVertices must own its false->true transition for each LOD;
   // pre-enabling CPU skinning skips the render-state rebuild and keeps a stale cache.
   return Part;
  }
  bool FaceBindGeometry()
  {
   FPose ReferenceCopy(Face->GetRefSkeleton());
   if (!Need(FaceReferenceCopyLocal.Num() == ReferenceCopy.Local.Num(), TEXT("accepted complete Face reference-copy pose available"))) return false;
   ReferenceCopy.Local = FaceReferenceCopyLocal; ReferenceCopy.Rebuild();
   UPoseableMeshComponent* Bind = Component(Face, ReferenceCopy, TEXT("FaceBindClosure")); TArray<int32> Used;
   TArray<TSharedPtr<FJsonValue>> Rows;
   for (const auto& Settings : Contract->GetArrayField(TEXT("lods")))
   {
    const int32 LODIndex = Settings->AsObject()->GetIntegerField(TEXT("overall_source_lod")); if (Used.Contains(LODIndex)) continue; Used.Add(LODIndex);
    if (!Need(LODIndex >= Bind->ComputeMinLOD() && LODIndex < Face->GetLODNum() && !Bind->GetCPUSkinningEnabled(), TEXT("fresh real-RHI Face bind-closure LOD read without min-LOD omission"))) return false;
    TArray<FFinalSkinVertex> Vertices; Bind->GetCPUSkinnedVertices(Vertices, LODIndex);
    const auto& Positions = Face->GetResourceForRendering()->LODRenderData[LODIndex].StaticVertexBuffers.PositionVertexBuffer;
    if (!Need(!Bind->GetCPUSkinningEnabled() && Vertices.Num() == int32(Positions.GetNumVertices()) && Vertices.Num() > 0,
     TEXT("native CPU helper returned every selected Face bind vertex and restored GPU state"))) return false;
    double Maximum = 0; int32 Worst = INDEX_NONE; bool Finite = true;
    for (int32 Index = 0; Index < Vertices.Num(); ++Index)
    {
     const FVector Native(Positions.VertexPosition(Index)), Actual(Vertices[Index].Position); Finite &= !Actual.ContainsNaN();
     const double Distance = FVector::Distance(Native, Actual); if (Distance > Maximum) { Maximum = Distance; Worst = Index; }
    }
    FJson Row = Object(); Row->SetNumberField(TEXT("lod"), LODIndex); Row->SetNumberField(TEXT("vertices"), Vertices.Num());
    Row->SetNumberField(TEXT("max_reference_position_error_cm"), Maximum); Row->SetNumberField(TEXT("worst_vertex"), Worst); Rows.Add(Value(Row));
    Report->SetArrayField(TEXT("native_face_bind_geometry"), Rows);
    if (!Need(Finite && Maximum <= PoseNumber(TEXT("max_shared_reference_error_cm")), TEXT("actual native CPU reference copy preserves every Face bind position within unchanged reference limit"))) return false;
   }
   Bind->DestroyComponent(); return true;
  }
  bool AppendNative(UMeshComponent* Part, int32 OverallLOD, int32 BaseSlot, const FTransform& Seat, FRawMesh& Raw,
   int32& UsedUVs, const FJson& Provenance, bool CaptureSoles)
  {
   const FMatrix Transform = Part->GetComponentTransform().ToMatrixWithScale() * Seat.ToMatrixWithScale().Inverse();
   const int32 BaseVertex = Raw.VertexPositions.Num(), BeforeFaces = Raw.FaceMaterialIndices.Num();
   TArray<TSharedPtr<FJsonValue>> Sections;
   auto Wedge = [&](int32 Vertex, const FVector3f& X, const FVector3f& Y, const FVector3f& Z, const FColor& Color, const FStaticMeshVertexBuffer& UVs)
   {
    Raw.WedgeIndices.Add(BaseVertex + Vertex);
    Raw.WedgeTangentX.Add((FVector4f)Transform.TransformVector((FVector)X));
    Raw.WedgeTangentY.Add((FVector4f)Transform.TransformVector((FVector)Y));
    Raw.WedgeTangentZ.Add((FVector4f)Transform.TransformVector((FVector)Z)); Raw.WedgeColors.Add(Color);
    const int32 Count = UVs.GetNumTexCoords(); UsedUVs = FMath::Max(UsedUVs, Count);
    for (int32 Channel = 0; Channel < MAX_MESH_TEXTURE_COORDS; ++Channel)
     Raw.WedgeTexCoords[Channel].Add(Channel < Count ? UVs.GetVertexUV(Vertex, Channel) : FVector2f::ZeroVector);
   };
   auto Section = [&](int32 NativeSlot, int32 ResolvedSlot, int32 Triangles)
   {
    FJson Item = Object(); Item->SetNumberField(TEXT("native_material_index"), NativeSlot); Item->SetNumberField(TEXT("resolved_source_slot"), ResolvedSlot);
    Item->SetNumberField(TEXT("output_slot"), BaseSlot + ResolvedSlot); Item->SetNumberField(TEXT("triangles"), Triangles);
    Item->SetStringField(TEXT("material"), GetPathNameSafe(Part->GetMaterial(ResolvedSlot))); Sections.Add(Value(Item));
    for (int32 Triangle = 0; Triangle < Triangles; ++Triangle) { Raw.FaceMaterialIndices.Add(BaseSlot + ResolvedSlot); Raw.FaceSmoothingMasks.Add(0); }
   };
   if (USkinnedMeshComponent* Skin = Cast<USkinnedMeshComponent>(Part))
   {
    const int32 LODIndex = FMath::Min(OverallLOD, Skin->GetNumLODs() - 1);
    if (!Need(LODIndex >= Skin->ComputeMinLOD(), TEXT("every posed source contributes to selected LOD; no min-LOD omission"))) return false;
    if (!Need(!Skin->GetCPUSkinningEnabled(), TEXT("native CPU helper begins from GPU state for a fresh selected-LOD cache"))) return false;
    TArray<FFinalSkinVertex> Vertices; Skin->GetCPUSkinnedVertices(Vertices, LODIndex);
    if (!Need(!Skin->GetCPUSkinningEnabled(), TEXT("native CPU helper restores GPU state after selected-LOD readback"))) return false;
    const FSkeletalMeshLODRenderData& LOD = Skin->GetSkinnedAsset()->GetResourceForRendering()->LODRenderData[LODIndex];
    if (!Need(Vertices.Num() > 0 && Vertices.Num() == int32(LOD.GetNumVertices()), TEXT("actual CPU-skinned source vertex count"))) return false;
    for (const FFinalSkinVertex& Vertex : Vertices) Raw.VertexPositions.Add((FVector4f)Transform.TransformPosition((FVector)Vertex.Position));
    const FRawStaticIndexBuffer16or32Interface& Indices = *LOD.MultiSizeIndexContainer.GetIndexBuffer();
    const auto& Buffers = LOD.StaticVertexBuffers;
    if (!Need(Buffers.ColorVertexBuffer.GetNumVertices() == 0 || Buffers.ColorVertexBuffer.GetNumVertices() == uint32(Vertices.Num()), TEXT("source skin colors are absent or complete"))) return false;
    const FSkeletalMeshLODInfo* Info = Skin->GetSkinnedAsset()->GetLODInfo(LODIndex);
    for (int32 SectionIndex = 0; SectionIndex < LOD.RenderSections.Num(); ++SectionIndex)
    {
     const FSkelMeshRenderSection& Source = LOD.RenderSections[SectionIndex]; int32 Slot = Source.MaterialIndex;
     if (Info->LODMaterialMap.IsValidIndex(SectionIndex) && Info->LODMaterialMap[SectionIndex] != INDEX_NONE) Slot = Info->LODMaterialMap[SectionIndex];
     if (!Need(Slot >= 0 && Slot < Part->GetNumMaterials() && Skin->IsMaterialSectionShown(Source.MaterialIndex, LODIndex) &&
      Source.BaseIndex + Source.NumTriangles * 3 <= uint32(Indices.Num()), TEXT("exact shown native skin section, original LOD material map and index range"))) return false;
     Section(Source.MaterialIndex, Slot, Source.NumTriangles);
     const bool IsShoe = CaptureSoles && (Part->GetMaterial(Slot)->GetName() == TEXT("M_RW_Shoe") || Part->GetMaterial(Slot)->GetName() == TEXT("M_RW_Trim"));
     for (uint32 Offset = 0; Offset < Source.NumTriangles * 3; ++Offset)
     {
      const uint32 Vertex = Indices.Get(Source.BaseIndex + Offset);
      if (Vertex >= uint32(Vertices.Num())) return Need(false, TEXT("skin source index outside vertex buffer"));
      const FFinalSkinVertex& Skinned = Vertices[Vertex];
      Wedge(Vertex, FVector3f(Skinned.TangentX.ToFVector()), Skinned.GetTangentY(), FVector3f(Skinned.TangentZ.ToFVector()),
       Buffers.ColorVertexBuffer.GetNumVertices() ? Buffers.ColorVertexBuffer.VertexColor(Vertex) : FColor::White, Buffers.StaticMeshVertexBuffer);
      if (IsShoe)
      {
       const FVector Point(Raw.VertexPositions[BaseVertex + Vertex]);
       // Collar/coat trim is excluded by its actual seat-relative height; soles use Trim, uppers Shoe.
       if (Point.Z < -10) (Point.Y < 0 ? LeftSole : RightSole).Add(Point);
      }
     }
    }
    Provenance->SetNumberField(TEXT("source_lod"), LODIndex); Provenance->SetStringField(TEXT("source_asset"), Skin->GetSkinnedAsset()->GetPathName());
   }
   else
   {
    UStaticMeshComponent* Static = CastChecked<UStaticMeshComponent>(Part); const FStaticMeshRenderData* Data = Static->GetStaticMesh()->GetRenderData();
    const int32 LODIndex = FMath::Min(OverallLOD, Data->LODResources.Num() - 1); const FStaticMeshLODResources& LOD = Data->LODResources[LODIndex];
    const auto& Buffers = LOD.VertexBuffers; const FIndexArrayView Indices = LOD.IndexBuffer.GetArrayView();
    if (!Need(LOD.GetNumVertices() > 0 && (Buffers.ColorVertexBuffer.GetNumVertices() == 0 || Buffers.ColorVertexBuffer.GetNumVertices() == uint32(LOD.GetNumVertices())), TEXT("actual complete static card vertex/color buffers"))) return false;
    for (int32 Vertex = 0; Vertex < LOD.GetNumVertices(); ++Vertex) Raw.VertexPositions.Add((FVector4f)Transform.TransformPosition((FVector)Buffers.PositionVertexBuffer.VertexPosition(Vertex)));
    for (const FStaticMeshSection& Source : LOD.Sections)
    {
     if (!Need(Source.MaterialIndex >= 0 && Source.MaterialIndex < Part->GetNumMaterials() && Source.FirstIndex + Source.NumTriangles * 3 <= uint32(Indices.Num()), TEXT("actual card section/index range and original material slot"))) return false;
     Section(Source.MaterialIndex, Source.MaterialIndex, Source.NumTriangles);
     for (uint32 Offset = 0; Offset < Source.NumTriangles * 3; ++Offset)
     {
      const uint32 Vertex = Indices[Source.FirstIndex + Offset]; if (Vertex >= uint32(LOD.GetNumVertices())) return Need(false, TEXT("card source index outside vertex buffer"));
      Wedge(Vertex, FVector3f(Buffers.StaticMeshVertexBuffer.VertexTangentX(Vertex)), Buffers.StaticMeshVertexBuffer.VertexTangentY(Vertex), FVector3f(Buffers.StaticMeshVertexBuffer.VertexTangentZ(Vertex)),
       Buffers.ColorVertexBuffer.GetNumVertices() ? Buffers.ColorVertexBuffer.VertexColor(Vertex) : FColor::White, Buffers.StaticMeshVertexBuffer);
     }
    }
    Provenance->SetNumberField(TEXT("source_lod"), LODIndex); Provenance->SetStringField(TEXT("source_asset"), Static->GetStaticMesh()->GetPathName());
   }
   Provenance->SetStringField(TEXT("part"), Part->GetName()); Provenance->SetArrayField(TEXT("sections"), Sections);
   Provenance->SetNumberField(TEXT("triangles"), Raw.FaceMaterialIndices.Num() - BeforeFaces);
   return Need(Raw.FaceMaterialIndices.Num() > BeforeFaces, TEXT("every actual source part contributes triangles"));
  }
  void StoreDescription(UStaticMesh* Output, int32 LODIndex, const FRawMesh& Raw)
  {
   // Build MeshDescription directly. SaveRawMesh's legacy conversion welds positions
   // and discards degenerate faces; that is unsuitable as an exact source boundary.
   FMeshDescription* Description = Output->CreateMeshDescription(LODIndex); FStaticMeshAttributes Attributes(*Description); Attributes.Register();
   auto Positions = Attributes.GetVertexPositions(); auto Normals = Attributes.GetVertexInstanceNormals(); auto Tangents = Attributes.GetVertexInstanceTangents();
   auto Signs = Attributes.GetVertexInstanceBinormalSigns(); auto Colors = Attributes.GetVertexInstanceColors(); auto UVs = Attributes.GetVertexInstanceUVs();
   auto SlotNames = Attributes.GetPolygonGroupMaterialSlotNames(); int32 UVCount = 0;
   for (const auto& Channel : Raw.WedgeTexCoords) if (Channel.Num()) ++UVCount; UVs.SetNumChannels(UVCount);
   TArray<FVertexID> Vertices; for (const FVector3f& Position : Raw.VertexPositions) { const FVertexID ID = Description->CreateVertex(); Positions[ID] = Position; Vertices.Add(ID); }
   TArray<int32> UsedSlots; for (const int32 Slot : Raw.FaceMaterialIndices) UsedSlots.AddUnique(Slot); UsedSlots.Sort();
   for (int32 SectionIndex = 0; SectionIndex < UsedSlots.Num(); ++SectionIndex)
   {
    const int32 Slot = UsedSlots[SectionIndex]; const FPolygonGroupID ID(Slot); Description->CreatePolygonGroupWithID(ID);
    SlotNames[ID] = Output->GetStaticMaterials()[Slot].ImportedMaterialSlotName; Output->GetSectionInfoMap().Set(LODIndex, SectionIndex, FMeshSectionInfo(Slot));
   }
   for (int32 Triangle = 0; Triangle < Raw.FaceMaterialIndices.Num(); ++Triangle)
   {
    FVertexInstanceID Corners[3];
    for (int32 Corner = 0; Corner < 3; ++Corner)
    {
     const int32 WedgeIndex = Triangle * 3 + Corner; const FVertexInstanceID ID = Description->CreateVertexInstance(Vertices[Raw.WedgeIndices[WedgeIndex]]); Corners[Corner] = ID;
     Normals[ID] = Raw.WedgeTangentZ[WedgeIndex]; Tangents[ID] = Raw.WedgeTangentX[WedgeIndex];
     Signs[ID] = ((Raw.WedgeTangentZ[WedgeIndex] ^ Raw.WedgeTangentX[WedgeIndex]) | Raw.WedgeTangentY[WedgeIndex]) < 0 ? -1.0f : 1.0f;
     Colors[ID] = FVector4f(FLinearColor::FromSRGBColor(Raw.WedgeColors[WedgeIndex]));
     for (int32 Channel = 0; Channel < UVCount; ++Channel) UVs.Set(ID, Channel, Raw.WedgeTexCoords[Channel][WedgeIndex]);
    }
    Description->CreateTriangle(FPolygonGroupID(Raw.FaceMaterialIndices[Triangle]), Corners);
   }
   Description->BuildIndexers(); Output->CommitMeshDescription(LODIndex);
  }
  bool SoleContacts()
  {
   FJson Contacts = Object();
   for (const bool IsLeft : {true, false})
   {
    const TArray<FVector>& Points = IsLeft ? LeftSole : RightSole;
    if (!Need(Points.Num() > 0, TEXT("actual low Shoe+Trim vertices for both sole contacts"))) return false;
    FVector Low = Points[0]; for (const FVector& Point : Points) if (Point.Z < Low.Z) Low = Point;
    TArray<FVector> Flat; for (const FVector& Point : Points) if (Point.Z <= Low.Z + 0.2) Flat.AddUnique(Point);
    FVector Heel = Flat[0], Toe = Flat[0], SideMin = Flat[0], SideMax = Flat[0];
    for (const FVector& Point : Flat) { if (Point.X < Heel.X) Heel = Point; if (Point.X > Toe.X) Toe = Point; if (Point.Y < SideMin.Y) SideMin = Point; if (Point.Y > SideMax.Y) SideMax = Point; }
    FJson Item = Object(); Item->SetNumberField(TEXT("min_z_cm"), Low.Z); Item->SetNumberField(TEXT("flat_unique_vertices"), Flat.Num());
    Item->SetArrayField(TEXT("lowest"), Vector(Low)); Item->SetArrayField(TEXT("heel"), Vector(Heel)); Item->SetArrayField(TEXT("toe"), Vector(Toe));
    Item->SetArrayField(TEXT("side_min"), Vector(SideMin)); Item->SetArrayField(TEXT("side_max"), Vector(SideMax));
    Contacts->SetObjectField(IsLeft ? TEXT("left") : TEXT("right"), Item);
   }
   Contacts->SetStringField(TEXT("source"), TEXT("actual posed source-LOD0 clothing vertices, M_RW_Trim soles plus M_RW_Shoe uppers, below seat-10cm; extrema within 2mm of lowest; no repositioning"));
   Report->SetObjectField(TEXT("sole_contacts_output_cm"), Contacts); return true;
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
    Item->SetBoolField(TEXT("raw_valid"), Raw.IsValid()); Item->SetStringField(TEXT("corner_sha1"), Raw.IsValid() ? CornerDigest(Raw) : TEXT("INVALID"));
    Item->SetNumberField(TEXT("screen_size"), Model.ScreenSize.Default);
    Item->SetBoolField(TEXT("recompute_normals"), Model.BuildSettings.bRecomputeNormals); Item->SetBoolField(TEXT("recompute_tangents"), Model.BuildSettings.bRecomputeTangents);
    Item->SetBoolField(TEXT("generate_lightmap_uvs"), Model.BuildSettings.bGenerateLightmapUVs); Item->SetBoolField(TEXT("full_precision_uvs"), Model.BuildSettings.bUseFullPrecisionUVs);
    Item->SetBoolField(TEXT("high_precision_tangents"), Model.BuildSettings.bUseHighPrecisionTangentBasis);
    int32 UVs = 0; for (const auto& Channel : Raw.WedgeTexCoords) if (Channel.Num()) ++UVs; Item->SetNumberField(TEXT("uv_channels"), UVs);
    if (Mesh->GetRenderData() && Mesh->GetRenderData()->LODResources.IsValidIndex(Index))
    {
     const FStaticMeshLODResources& Render = Mesh->GetRenderData()->LODResources[Index];
     Item->SetNumberField(TEXT("render_vertices"), Render.GetNumVertices()); Item->SetNumberField(TEXT("render_triangles"), Render.GetNumTriangles());
     Item->SetBoolField(TEXT("render_full_precision_uvs"), Render.VertexBuffers.StaticMeshVertexBuffer.GetUseFullPrecisionUVs());
     Item->SetBoolField(TEXT("render_high_precision_tangents"), Render.VertexBuffers.StaticMeshVertexBuffer.GetUseHighPrecisionTangentBasis());
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
  FJson MaterialState(UMaterialInterface* Material)
  {
   const FMaterialResource* Resource = Material ? Material->GetMaterialResource(GMaxRHIShaderPlatform) : nullptr;
   const UMaterialInstance* Instance = Cast<UMaterialInstance>(Material); const UMaterial* Base = Material ? Material->GetMaterial() : nullptr;
   FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
   const bool Complete = Resource && Resource->IsGameThreadShaderMapComplete();
   const bool Default = Base == UMaterial::GetDefaultMaterial(MD_Surface) || (Resource && Resource->IsDefaultMaterial());
   FJson State = Object(); State->SetStringField(TEXT("material"), GetPathNameSafe(Material));
   State->SetBoolField(TEXT("resource_exists"), Resource != nullptr); State->SetBoolField(TEXT("shadermap_exists"), Map != nullptr);
   State->SetBoolField(TEXT("complete"), Complete); State->SetBoolField(TEXT("default"), Default);
   State->SetStringField(TEXT("base"), GetPathNameSafe(Base)); State->SetStringField(TEXT("parent"), Instance ? GetPathNameSafe(Instance->Parent) : TEXT("None"));
   State->SetBoolField(TEXT("material_is_compiling"), Material && Material->IsCompiling()); State->SetBoolField(TEXT("base_is_compiling"), Base && Base->IsCompiling());
   State->SetStringField(TEXT("resource_material"), Resource ? GetPathNameSafe(Resource->GetMaterialInterface()) : TEXT("None"));
   State->SetStringField(TEXT("resource_base"), Resource ? GetPathNameSafe(Resource->GetMaterial()) : TEXT("None"));
   State->SetNumberField(TEXT("shader_platform"), int32(GMaxRHIShaderPlatform));
   State->SetNumberField(TEXT("usage_flags"), Material ? Material->GetUsageFlags() : 0); State->SetNumberField(TEXT("base_usage_flags"), Base ? Base->GetUsageFlags() : 0);
   State->SetBoolField(TEXT("used_with_skeletal_mesh"), Material && Material->GetUsageByFlag(MATUSAGE_SkeletalMesh));
   State->SetBoolField(TEXT("base_used_with_skeletal_mesh"), Base && Base->GetUsageByFlag(MATUSAGE_SkeletalMesh));
   State->SetBoolField(TEXT("package_dirty"), Material && Material->GetOutermost()->IsDirty());
   if (Instance)
   {
    const FStaticParameterSet Parameters = Instance->GetStaticParameters(); FJson Overrides = Object();
    State->SetBoolField(TEXT("has_static_permutation_resource"), Instance->bHasStaticPermutationResource);
    State->SetBoolField(TEXT("has_static_parameters"), Instance->HasStaticParameters()); State->SetBoolField(TEXT("static_parameters_empty"), Parameters.IsEmpty());
    State->SetNumberField(TEXT("static_switches"), Parameters.StaticSwitchParameters.Num());
    State->SetNumberField(TEXT("static_component_masks"), Parameters.EditorOnly.StaticComponentMaskParameters.Num());
    State->SetBoolField(TEXT("static_material_layers"), Parameters.bHasMaterialLayers); State->SetBoolField(TEXT("has_overridden_base_properties"), Instance->HasOverridenBaseProperties());
    for (TFieldIterator<FProperty> Field(FMaterialInstanceBasePropertyOverrides::StaticStruct()); Field; ++Field)
    {
     FString Text; Field->ExportTextItem_Direct(Text, Field->ContainerPtrToValuePtr<void>(&Instance->BasePropertyOverrides), nullptr, nullptr, PPF_None);
     Overrides->SetStringField(Field->GetName(), Text);
    }
    State->SetObjectField(TEXT("base_overrides"), Overrides);
   }
   if (Resource)
   {
    State->SetNumberField(TEXT("quality_level"), int32(Resource->GetQualityLevel()));
    FJson Flags = Object(); Flags->SetBoolField(TEXT("skeletal_mesh"), Resource->IsUsedWithSkeletalMesh());
    Flags->SetBoolField(TEXT("morph_targets"), Resource->IsUsedWithMorphTargets()); Flags->SetBoolField(TEXT("static_lighting"), Resource->IsUsedWithStaticLighting());
    Flags->SetBoolField(TEXT("instanced_static_meshes"), Resource->IsUsedWithInstancedStaticMeshes()); Flags->SetBoolField(TEXT("special_engine_material"), Resource->IsSpecialEngineMaterial());
    State->SetObjectField(TEXT("resource_should_cache_flags"), Flags);
   }
   if (Map)
   {
    const FMaterialShaderMapId& ID = Map->GetShaderMapId(); FJson MapID = Object(); MapID->SetStringField(TEXT("base_material_id"), ID.BaseMaterialId.ToString());
    MapID->SetNumberField(TEXT("quality_level"), int32(ID.QualityLevel)); MapID->SetNumberField(TEXT("feature_level"), int32(ID.FeatureLevel));
    MapID->SetNumberField(TEXT("usage"), int32(ID.Usage)); MapID->SetStringField(TEXT("base_property_overrides_hash"), LexToString(ID.BasePropertyOverridesHash));
    State->SetObjectField(TEXT("shader_map_id"), MapID); State->SetBoolField(TEXT("compilation_finalized"), Map->IsCompilationFinalized());
    State->SetBoolField(TEXT("compiled_successfully"), Map->CompiledSuccessfully()); State->SetNumberField(TEXT("compiling_id"), Map->GetCompilingId());
    const bool Valid = Map->IsValidForRendering(); State->SetBoolField(TEXT("valid_for_rendering"), Valid);
    if (Valid)
    {
     int32 Shaders = 0, Pipelines = 0; Map->CountNumShaders(Shaders, Pipelines); State->SetNumberField(TEXT("shader_count"), Shaders); State->SetNumberField(TEXT("pipeline_count"), Pipelines);
     State->SetStringField(TEXT("shader_content_hash"), LexToString(static_cast<const FXxHash64&>(Map->GetShaderContentHash()))); TArray<TSharedPtr<FJsonValue>> Factories;
     for (TLinkedList<FVertexFactoryType*>::TIterator It(FVertexFactoryType::GetTypeList()); It; ++It)
     {
      const FVertexFactoryType* Factory = *It; const FString Name = Factory->GetName();
      if (!Name.Contains(TEXT("LocalVertexFactory")) && !Name.Contains(TEXT("GPUSkin"))) continue;
      FJson FactoryState = Object(); FactoryState->SetStringField(TEXT("vertex_factory"), Name); const FMeshMaterialShaderMap* MeshMap = Map->GetMeshShaderMap(Factory);
      FactoryState->SetBoolField(TEXT("map_exists"), MeshMap != nullptr); FactoryState->SetNumberField(TEXT("shader_count"), MeshMap ? MeshMap->GetNumShaders() : 0);
      for (TLinkedList<FShaderType*>::TIterator ShaderIt(FShaderType::GetTypeList()); ShaderIt; ++ShaderIt)
      {
       const FShaderType* Shader = *ShaderIt; const FString ShaderName = Shader->GetName();
       if (ShaderName == TEXT("FDepthOnlyVS") || ShaderName == TEXT("FDepthOnlyPS") || ShaderName == TEXT("TBasePassVSFNoLightMapPolicy"))
        FactoryState->SetBoolField(ShaderName, Resource->ShouldCache(Shader, Factory));
      }
      Factories.Add(Value(FactoryState));
     }
     State->SetArrayField(TEXT("vertex_factories_material_should_cache"), Factories);
    }
   }
   TArray<TSharedPtr<FJsonValue>> Errors; if (Resource) for (const FString& Message : Resource->GetCompileErrors()) Errors.Add(MakeShared<FJsonValueString>(Message));
   State->SetArrayField(TEXT("errors"), Errors); State->SetBoolField(TEXT("shader_ready"), Resource && Map && Complete && Errors.IsEmpty() && Base && !Default); return State;
  }
  bool DiagnoseMaterials()
  {
   if (!Need(FApp::CanEverRender() && GDynamicRHI && RHIGetInterfaceType() != ERHIInterfaceType::Null && GShaderCompilingManager,
    TEXT("materials diagnosis requires actual RHI and shader compiler, never NullRHI"))) return false;
   const FString Root = Job->GetStringField(TEXT("asset_root")); TArray<UMaterialInterface*> Selected;
   if (!Need(Root.StartsWith(TEXT("/Game/MetaHumans/Mannequins/RadioWoman/")) && !Root.Contains(TEXT("..")) && Clothes->GetMaterials().Num() == 5,
    TEXT("exact five actual private clothes slots for this run"))) return false;
   for (const FSkeletalMaterial& Slot : Clothes->GetMaterials())
   {
    UMaterialInterface* Material = Slot.MaterialInterface; const UMaterialInstance* Instance = Cast<UMaterialInstance>(Material);
    if (!Need(Material && Instance && Instance->Parent && Instance->Parent->GetOutermost()->GetName() == Job->GetStringField(TEXT("cloth_master")) &&
     Material->GetOutermost()->GetName().StartsWith(Root + TEXT("/")) &&
     Contract->GetObjectField(TEXT("materials"))->HasField(Material->GetName()) && Slot.MaterialSlotName.ToString() == Material->GetName() && !Selected.Contains(Material),
     TEXT("each actual clothing MI and slot is distinct, authored and inside the correct private run"))) return false;
    Selected.Add(Material);
   }
   FJson Diagnostic = Object(); Report->SetObjectField(TEXT("materials"), Diagnostic); Diagnostic->SetBoolField(TEXT("explicit_transient_usage_replay"), true);
   Diagnostic->SetStringField(TEXT("scope"), TEXT("actual clothes slots; replay CheckMaterialUsage(SkeletalMesh) as recorded during R8 component registration; in-memory shader work only, no asset save, output mesh, pose or geometry mutation"));
   TArray<TSharedPtr<FJsonValue>> Loaded, Usage, After, Replay;
   Diagnostic->SetNumberField(TEXT("jobs_loaded_before"), GShaderCompilingManager->GetNumRemainingJobs());
   for (UMaterialInterface* Material : Selected) Loaded.Add(Value(MaterialState(Material)));
   Diagnostic->SetArrayField(TEXT("loaded_before"), Loaded);
   bool UsageAccepted = true;
   for (UMaterialInterface* Material : Selected)
   {
    FJson Request = Object(); Request->SetStringField(TEXT("material"), Material->GetPathName()); Request->SetBoolField(TEXT("needed_before"), !Material->GetUsageByFlag(MATUSAGE_SkeletalMesh));
    const bool Accepted = Material->CheckMaterialUsage(MATUSAGE_SkeletalMesh); UsageAccepted &= Accepted;
    Request->SetBoolField(TEXT("check_material_usage_returned"), Accepted); Replay.Add(Value(Request));
   }
   Diagnostic->SetBoolField(TEXT("all_usage_requests_succeeded"), UsageAccepted);
   Diagnostic->SetArrayField(TEXT("usage_replay"), Replay); Diagnostic->SetNumberField(TEXT("jobs_after_usage_before_ensure"), GShaderCompilingManager->GetNumRemainingJobs());
   for (UMaterialInterface* Material : Selected) Usage.Add(Value(MaterialState(Material))); Diagnostic->SetArrayField(TEXT("after_usage_before_ensure"), Usage);
   for (UMaterialInterface* Material : Selected) Material->EnsureIsComplete();
   GShaderCompilingManager->FinishAllCompilation(); GShaderCompilingManager->ProcessAsyncResults(false, true); FlushRenderingCommands();
   Diagnostic->SetNumberField(TEXT("jobs_after_ensure"), GShaderCompilingManager->GetNumRemainingJobs()); bool Ready = true;
   for (UMaterialInterface* Material : Selected) { FJson State = MaterialState(Material); Ready &= State->GetBoolField(TEXT("shader_ready")) && Material->GetUsageByFlag(MATUSAGE_SkeletalMesh); After.Add(Value(State)); }
   Diagnostic->SetArrayField(TEXT("after_ensure"), After); Diagnostic->SetBoolField(TEXT("assets_saved"), false);
   return Need(UsageAccepted && Ready, TEXT("all five actual private MI resources support the actual usage replay and are complete and nondefault after EnsureIsComplete"));
  }
  bool Audit(UStaticMesh* Mesh, const FJson& Expected = nullptr)
  {
   // Asset compilers finish in manager order. A mesh/usage update can enqueue
   // shaders after the shader manager's turn, so finish shaders explicitly
   // after every asset compiler and apply their results before render flush.
   FJson Barrier = Object(); Report->SetObjectField(TEXT("material_shader_barrier"), Barrier);
   Barrier->SetBoolField(TEXT("shader_manager_exists"), GShaderCompilingManager != nullptr);
   if (!Need(IsInGameThread() && FApp::CanEverRender() && GShaderCompilingManager,
    TEXT("actual RHI authoring audit with game-thread shader compiler available"))) return false;
   const double BarrierStart = FPlatformTime::Seconds();
   Barrier->SetNumberField(TEXT("jobs_before"), GShaderCompilingManager->GetNumRemainingJobs());
   FAssetCompilingManager::Get().FinishAllCompilation();
   Barrier->SetNumberField(TEXT("jobs_after_asset_compilers"), GShaderCompilingManager->GetNumRemainingJobs());
   TArray<TSharedPtr<FJsonValue>> BeforeEnsure;
   for (const FStaticMaterial& Slot : Mesh->GetStaticMaterials()) BeforeEnsure.Add(Value(MaterialState(Slot.MaterialInterface)));
   Report->SetArrayField(TEXT("actual_material_compile_before_ensure"), BeforeEnsure);
   // Usage changes can create a valid but incomplete shader map with None
   // precompilation and no queued jobs. Waiting alone cannot submit those.
   // EnsureIsComplete requests jobs for each actual selected MI/resource.
   for (const FStaticMaterial& Slot : Mesh->GetStaticMaterials()) if (Slot.MaterialInterface) Slot.MaterialInterface->EnsureIsComplete();
   Barrier->SetNumberField(TEXT("jobs_after_ensure"), GShaderCompilingManager->GetNumRemainingJobs());
   Barrier->SetBoolField(TEXT("actual_selected_slots_ensure_completed"), true);
   GShaderCompilingManager->FinishAllCompilation();
   GShaderCompilingManager->ProcessAsyncResults(false, true);
   FlushRenderingCommands();
   Barrier->SetNumberField(TEXT("jobs_after"), GShaderCompilingManager->GetNumRemainingJobs());
   Barrier->SetBoolField(TEXT("is_compiling_after"), GShaderCompilingManager->IsCompiling());
   Barrier->SetBoolField(TEXT("explicit_finish_all_completed"), true); Barrier->SetBoolField(TEXT("async_results_applied"), true);
   Barrier->SetBoolField(TEXT("render_commands_flushed"), true); Barrier->SetNumberField(TEXT("seconds"), FPlatformTime::Seconds() - BarrierStart);
   const FJson Actual = StaticAudit(Mesh); Report->SetObjectField(TEXT("output"), Actual);
   const auto& Settings = Contract->GetArrayField(TEXT("lods")); const auto& LODs = Actual->GetArrayField(TEXT("lods"));
   if (!Need(LODs.Num() == Settings.Num() && Mesh->GetRenderData() && Mesh->GetRenderData()->LODResources.Num() == Settings.Num(), TEXT("three actual authored and render LODs"))) return false;
   int32 PreviousTriangles = MAX_int32;
   for (int32 Index = 0; Index < LODs.Num(); ++Index)
   {
    const FJson Item = LODs[Index]->AsObject(), Limit = Settings[Index]->AsObject(); const int32 Triangles = Item->GetIntegerField(TEXT("triangles"));
    if (!Need(Triangles > 0 && Triangles < PreviousTriangles && Triangles <= Limit->GetIntegerField(TEXT("max_triangles")), FString::Printf(TEXT("measured decreasing LOD %d triangle budget"), Index))) return false;
    if (!Need(Item->GetBoolField(TEXT("raw_valid")) && Triangles == Item->GetIntegerField(TEXT("render_triangles")) && Item->GetIntegerField(TEXT("uv_channels")) > 0 &&
     !Item->GetBoolField(TEXT("recompute_normals")) && !Item->GetBoolField(TEXT("recompute_tangents")) && !Item->GetBoolField(TEXT("generate_lightmap_uvs")) &&
     Item->GetBoolField(TEXT("full_precision_uvs")) && Item->GetBoolField(TEXT("high_precision_tangents")) &&
     Item->GetBoolField(TEXT("render_full_precision_uvs")) && Item->GetBoolField(TEXT("render_high_precision_tangents")) &&
     FMath::IsNearlyEqual(Item->GetNumberField(TEXT("screen_size")), Limit->GetNumberField(TEXT("screen_size")), 0.000001), TEXT("render triangles, authored UV/normal/tangent and screen policy retained"))) return false;
    FRawMesh Raw; Mesh->GetSourceModel(Index).LoadRawMesh(Raw);
    TArray<int32> SourceCounts, RenderCounts; SourceCounts.Init(0, Mesh->GetStaticMaterials().Num()); RenderCounts = SourceCounts;
    for (const int32 Slot : Raw.FaceMaterialIndices)
    {
     if (Slot < 0 || Slot >= SourceCounts.Num()) return Need(false, TEXT("raw source material index outside original table"));
     ++SourceCounts[Slot];
    }
    const FStaticMeshLODResources& Render = Mesh->GetRenderData()->LODResources[Index];
    for (const FStaticMeshSection& Section : Render.Sections)
    {
     if (!Need(Section.MaterialIndex >= 0 && Section.MaterialIndex < RenderCounts.Num() &&
      Section.FirstIndex + Section.NumTriangles * 3 <= uint32(Render.IndexBuffer.GetNumIndices()), TEXT("actual render section uses valid original material/index range"))) return false;
     RenderCounts[Section.MaterialIndex] += Section.NumTriangles;
    }
    if (!Need(SourceCounts == RenderCounts, TEXT("actual render material bindings and per-slot triangles exactly match native source corners"))) return false;
    TArray<TSharedPtr<FJsonValue>> BindingCounts;
    for (int32 Slot = 0; Slot < SourceCounts.Num(); ++Slot)
    {
     FJson Binding = Object(); Binding->SetNumberField(TEXT("slot"), Slot); Binding->SetNumberField(TEXT("source_triangles"), SourceCounts[Slot]);
     Binding->SetNumberField(TEXT("render_triangles"), RenderCounts[Slot]); Binding->SetStringField(TEXT("material"), GetPathNameSafe(Mesh->GetStaticMaterials()[Slot].MaterialInterface));
     BindingCounts.Add(Value(Binding));
    }
    Item->SetArrayField(TEXT("actual_material_bindings"), BindingCounts);
    if (Expected)
    {
     const FJson Old = Expected->GetArrayField(TEXT("lods"))[Index]->AsObject();
     if (!Need(Item->GetStringField(TEXT("raw_sha1")) == Old->GetStringField(TEXT("raw_sha1")) && Item->GetStringField(TEXT("corner_sha1")) == Old->GetStringField(TEXT("corner_sha1")) && Triangles == Old->GetIntegerField(TEXT("triangles")), TEXT("fresh saved asset exact raw geometry/UV/normal/tangent/color/material digest"))) return false;
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
   TArray<TSharedPtr<FJsonValue>> Compile;
   bool ShadersReady = true;
   for (const FStaticMaterial& Slot : Mesh->GetStaticMaterials())
   {
    FJson State = MaterialState(Slot.MaterialInterface); ShadersReady &= State->GetBoolField(TEXT("shader_ready")); Compile.Add(Value(State));
   }
   Report->SetArrayField(TEXT("actual_material_compile"), Compile);
   if (!Need(ShadersReady, TEXT("all actual material resources have complete shader maps, no compile errors or default-surface fallback"))) return false;
   return true;
  }
  bool Run()
  {
   if (!Preflight()) return false;
   if (Mode == TEXT("bind-clothes") || Mode == TEXT("verify-clothes-bind")) return ClothesBinding(Mode == TEXT("verify-clothes-bind"));
   if (Mode == TEXT("materials")) return DiagnoseMaterials();
   if (Mode == TEXT("verify"))
   {
    FJson Previous; if (!Need(Read(Job->GetStringField(TEXT("bake_report")), Previous) && Previous->GetStringField(TEXT("status")) == TEXT("PASS"), TEXT("actual successful private bake receipt"))) return false;
    UStaticMesh* Saved = Load<UStaticMesh>(Job->GetStringField(TEXT("output_asset")));
    if (!Need(Saved != nullptr, TEXT("saved output really reloads as StaticMesh"))) return false;
    FAssetCompilingManager::Get().FinishAllCompilation();
    return Audit(Saved, Previous->GetObjectField(TEXT("output")));
   }
   FPose BodyPose(Body->GetRefSkeleton()), FacePose(Face->GetRefSkeleton()), ClothPose(Clothes->GetRefSkeleton()); FTransform Seat;
   if (!PoseBody(BodyPose, Seat) || !ReferenceDiagnostics(BodyPose, FacePose, ClothPose)
    || !Transfer(BodyPose, Face, false, FacePose) || !Transfer(BodyPose, Clothes, true, ClothPose)) return false;
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
   if (!FaceBindGeometry()) return false;
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
   // The local multi-component ConvertMeshesToStaticMesh compacts an already
   // accumulated slot map after each skinned part. Read its underlying native CPU
   // vertex/section APIs directly, preserving every original slot and LOD remap.
   TArray<FRawMesh> RawLODs; TArray<FStaticMaterial> Materials; TArray<int32> BaseSlots, SlotCounts; TArray<TSharedPtr<FJsonValue>> Provenance;
   for (int32 PartIndex = 0; PartIndex < Parts.Num(); ++PartIndex)
   {
    UMeshComponent* Part = Parts[PartIndex]; BaseSlots.Add(Materials.Num()); SlotCounts.Add(Part->GetNumMaterials());
    for (int32 Slot = 0; Slot < Part->GetNumMaterials(); ++Slot)
    {
     UMaterialInterface* Material = Part->GetMaterial(Slot); if (!Need(Material != nullptr, TEXT("actual material for every original source slot"))) return false;
     const FName Name(*FString::Printf(TEXT("Part%d_Slot%d"), PartIndex, Slot)); Materials.Add(FStaticMaterial(Material, Name, Name));
    }
   }
   const auto& LODSettings = Contract->GetArrayField(TEXT("lods"));
   for (int32 Index = 0; Index < LODSettings.Num(); ++Index)
   {
    const FJson Settings = LODSettings[Index]->AsObject(); const bool Far = Settings->GetStringField(TEXT("cards")) == TEXT("far");
    Hair->SetStaticMesh(Far ? HairFar : HairNear); Brows->SetStaticMesh(Far ? BrowsFar : BrowsNear);
    for (int32 Slot = 0; Slot < Hair->GetNumMaterials(); ++Slot) Hair->SetMaterial(Slot, HairMaterial);
    for (int32 Slot = 0; Slot < Brows->GetNumMaterials(); ++Slot) Brows->SetMaterial(Slot, BrowMaterial);
    const int32 SourceLOD = Settings->GetIntegerField(TEXT("overall_source_lod"));
    if (!Need(SourceLOD >= 0 && SourceLOD < Face->GetLODNum(), TEXT("requested actual overall face LOD exists"))) return false;
    FRawMesh& Raw = RawLODs.AddDefaulted_GetRef(); int32 UsedUVs = 0; TArray<TSharedPtr<FJsonValue>> Contributions;
    for (int32 PartIndex = 0; PartIndex < Parts.Num(); ++PartIndex)
    {
     UMeshComponent* Part = Parts[PartIndex]; if (!Need(Part->GetNumMaterials() == SlotCounts[PartIndex], TEXT("actual near/far source slot count stable"))) return false;
     for (int32 Slot = 0; Slot < SlotCounts[PartIndex]; ++Slot)
      if (!Need(Part->GetMaterial(Slot) == Materials[BaseSlots[PartIndex] + Slot].MaterialInterface, TEXT("original material identities remain fixed across LODs"))) return false;
     FJson Contribution = Object();
     if (!AppendNative(Part, SourceLOD, BaseSlots[PartIndex], Seat, Raw, UsedUVs, Contribution, Index == 0 && PartIndex == 2)) return false;
     Contributions.Add(Value(Contribution));
    }
    for (int32 Channel = UsedUVs; Channel < MAX_MESH_TEXTURE_COORDS; ++Channel) Raw.WedgeTexCoords[Channel].Empty();
    if (!Need(Raw.IsValid() && UsedUVs > 0 && UsedUVs <= MAX_MESH_TEXTURE_COORDS, TEXT("actual CPU-skinned/native corner attributes valid without material compaction"))) return false;
    bool Finite = true;
    for (const FVector3f& Position : Raw.VertexPositions) Finite &= !Position.ContainsNaN();
    for (const FVector3f& Normal : Raw.WedgeTangentZ) Finite &= !Normal.ContainsNaN() && Normal.SizeSquared() > 0.1f;
    for (const FVector3f& Tangent : Raw.WedgeTangentX) Finite &= !Tangent.ContainsNaN() && Tangent.SizeSquared() > 0.1f;
    for (const auto& Channel : Raw.WedgeTexCoords) for (const FVector2f& UV : Channel) Finite &= !UV.ContainsNaN();
    if (!Need(Finite, TEXT("finite native positions/UVs and nonzero authored tangent basis"))) return false;
    FJson Item = Object(); Item->SetNumberField(TEXT("output_lod"), Index); Item->SetNumberField(TEXT("overall_source_lod"), SourceLOD);
    Item->SetStringField(TEXT("hair_asset"), Hair->GetStaticMesh()->GetPathName()); Item->SetStringField(TEXT("brow_asset"), Brows->GetStaticMesh()->GetPathName());
    Item->SetArrayField(TEXT("parts"), Contributions); Item->SetStringField(TEXT("source_corner_sha1"), CornerDigest(Raw)); Provenance.Add(Value(Item));
   }
   if (!SoleContacts()) return false;
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
    StoreDescription(Output, Index, RawLODs[Index]);
   }
   Output->GetOriginalSectionInfoMap().CopyFrom(Output->GetSectionInfoMap());
   Output->SetImportVersion(EImportStaticMeshVersion::LastVersion); Output->Build(false); Output->PostEditChange();
   FStaticMeshCompilingManager::Get().FinishAllCompilation();
   Output->CreateBodySetup(); Output->GetBodySetup()->DefaultInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);
   Report->SetArrayField(TEXT("conversion_provenance"), Provenance);
   if (!Audit(Output)) return false;
   const auto& SavedLODs = Report->GetObjectField(TEXT("output"))->GetArrayField(TEXT("lods"));
   for (int32 Index = 0; Index < SavedLODs.Num(); ++Index)
    if (!Need(SavedLODs[Index]->AsObject()->GetStringField(TEXT("corner_sha1")) == Provenance[Index]->AsObject()->GetStringField(TEXT("source_corner_sha1")), TEXT("output retains exact native positions, winding, corner UVs/normals/tangent handedness/colors and original material map"))) return false;
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
  Bake.Report->SetNumberField(TEXT("schema_version"), 1); Bake.Report->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
  Bake.Report->SetStringField(TEXT("status"), Passed ? TEXT("PASS") : TEXT("FAIL")); Bake.Report->SetStringField(TEXT("mode"), Bake.Mode);
  Bake.Report->SetStringField(TEXT("error"), Bake.Error); Bake.Report->SetNumberField(TEXT("checks"), Bake.Checks);
  Bake.Report->SetStringField(TEXT("evidence_limit"), TEXT("native authoring only; appearance, runtime adoption/schedule, input and performance require separate proof; no installation/release"));
  if (!Write(Bake.ReportPath, Bake.Report)) UE_LOG(LogS47MannequinBake, Error, TEXT("S47 Mannequin FAIL cannot write private receipt"));
  UE_LOG(LogS47MannequinBake, Display, TEXT("S47 Mannequin %s %s checks=%d report=%s"), Passed ? TEXT("PASS") : TEXT("FAIL"), *Bake.Mode, Bake.Checks, *Bake.ReportPath);
 }
 FAutoConsoleCommand BakeCommand(TEXT("S47.MannequinBake"), TEXT("Private editor-only mannequin pose/bake/verify; requires S47_MANNEQUIN_JOB JSON."), FConsoleCommandDelegate::CreateStatic(&Execute));
}
#endif
