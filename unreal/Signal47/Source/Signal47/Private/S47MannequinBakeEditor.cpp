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
#include "Editor.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSourceData.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
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
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/MemoryWriter.h"
#include "StaticMeshCompiler.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshResources.h"
#include "SkeletalRenderPublic.h"
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
   // GetCPUSkinnedVertices must own its false->true transition for each LOD;
   // pre-enabling CPU skinning skips the render-state rebuild and keeps a stale cache.
   return Part;
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
  bool Audit(UStaticMesh* Mesh, const FJson& Expected = nullptr)
  {
   // The shader compiler registers with FAssetCompilingManager in UE5.8. Finish
   // it too; static mesh compilation alone does not prove a usable material.
   FAssetCompilingManager::Get().FinishAllCompilation();
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
    UMaterialInterface* Material = Slot.MaterialInterface; const FMaterialResource* Resource = Material ? Material->GetMaterialResource(GMaxRHIShaderPlatform) : nullptr;
    FJson State = Object(); State->SetStringField(TEXT("material"), GetPathNameSafe(Material));
    TArray<TSharedPtr<FJsonValue>> Errors; if (Resource) for (const FString& Message : Resource->GetCompileErrors()) Errors.Add(MakeShared<FJsonValueString>(Message));
    const bool Ready = Resource && Resource->GetGameThreadShaderMap() && Resource->IsGameThreadShaderMapComplete() && Errors.IsEmpty() && Material->GetMaterial() != UMaterial::GetDefaultMaterial(MD_Surface);
    State->SetArrayField(TEXT("errors"), Errors); State->SetBoolField(TEXT("shader_ready"), Ready); Compile.Add(Value(State)); ShadersReady &= Ready;
   }
   Report->SetArrayField(TEXT("actual_material_compile"), Compile);
   if (!Need(ShadersReady, TEXT("all actual material resources have complete shader maps, no compile errors or default-surface fallback"))) return false;
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
  Bake.Report->SetStringField(TEXT("status"), Passed ? TEXT("PASS") : TEXT("FAIL")); Bake.Report->SetStringField(TEXT("mode"), Bake.Mode);
  Bake.Report->SetStringField(TEXT("error"), Bake.Error); Bake.Report->SetNumberField(TEXT("checks"), Bake.Checks);
  Bake.Report->SetStringField(TEXT("evidence_limit"), TEXT("native authoring only; appearance, runtime adoption/schedule, input and performance require separate proof; no installation/release"));
  if (!Write(Bake.ReportPath, Bake.Report)) UE_LOG(LogS47MannequinBake, Error, TEXT("S47 Mannequin FAIL cannot write private receipt"));
  UE_LOG(LogS47MannequinBake, Display, TEXT("S47 Mannequin %s %s checks=%d report=%s"), Passed ? TEXT("PASS") : TEXT("FAIL"), *Bake.Mode, Bake.Checks, *Bake.ReportPath);
 }
 FAutoConsoleCommand BakeCommand(TEXT("S47.MannequinBake"), TEXT("Private editor-only mannequin pose/bake/verify; requires S47_MANNEQUIN_JOB JSON."), FConsoleCommandDelegate::CreateStatic(&Execute));
}
#endif
