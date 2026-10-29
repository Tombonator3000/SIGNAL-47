// HUM-01: preserve the existing lap radio and person owner; replace only old body geometry.
#include "S47RadioWoman.h"
#include "S47Web.h"
#include "S47Three.h"
#include "S47Json.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47World.h"
#include "S47Startup.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/SecureHash.h"
#include "RenderUtils.h"
#include "StaticMeshResources.h"
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47RadioWoman,Log,All);
namespace
{
 FString AssetPath=TEXT("/Game/MetaHumans/Mannequins/RadioWoman/BakeR12/SM_RadioWoman_Seated");
 FAutoConsoleVariableRef AssetCVar(TEXT("s47.RadioWoman.Asset"),AssetPath,TEXT("Private accepted bake candidate; evaluated when the world is built."),ECVF_Default);
 struct FExpected {int32 Node,Geo,Material,Triangles;};
 const FExpected Expected[]={{1481,1028,656,108},{1482,1029,657,156},{1483,1030,658,48},{1484,1031,641,36},{1485,1032,638,12},{1486,1033,659,100}};
 // Actual g1074 seat, measured with nine native traces; legacy body origin is higher.
 constexpr double MeasuredSeatFromLegacyCm=-6.559057316375288,MaxSeatFrameErrorCm=.25,MaxSoleGapCm=1.5;
 const FBox RadioBox(FVector(-.1,.06,-.36),FVector(.1,.18,-.30));
 const FBox ShoeBoxes[]={FBox(FVector(-.16,-.445,-.56),FVector(-.04,-.375,-.36)),FBox(FVector(.04,-.445,-.56),FVector(.16,-.375,-.36))};
 struct FOriginal
 {
  FExpected Id{};TWeakObjectPtr<UStaticMeshComponent> Component;TStrongObjectPtr<UStaticMesh> Mesh;
  TArray<UMaterialInterface*> Materials;FTransform Relative;
  FVector PositionThree=FVector::ZeroVector,ScaleThree=FVector::OneVector;
  FQuat QuaternionThree=FQuat::Identity;
  ECollisionEnabled::Type Collision=ECollisionEnabled::NoCollision;
  bool Hidden=false,Shadow=false,AuthoredVisible=false;
 };
 FString VertexHash(const FStaticMeshLODResources& L)
 {
  const auto& B=L.VertexBuffers;const auto& T=B.StaticMeshVertexBuffer;
  if(!B.PositionVertexBuffer.GetNumVertices()||B.PositionVertexBuffer.GetNumVertices()!=T.GetNumVertices()||!T.GetNumTexCoords()
   ||!B.PositionVertexBuffer.GetVertexData()||!T.GetTangentData()||!T.GetTexCoordData()
   ||(B.ColorVertexBuffer.GetNumVertices()&&!B.ColorVertexBuffer.GetVertexData()))return {};
  FSHA1 H;H.Update(static_cast<const uint8*>(B.PositionVertexBuffer.GetVertexData()),B.PositionVertexBuffer.GetNumVertices()*B.PositionVertexBuffer.GetStride());
  H.Update(static_cast<const uint8*>(T.GetTangentData()),T.GetTangentSize());H.Update(static_cast<const uint8*>(T.GetTexCoordData()),T.GetTexCoordSize());
  if(B.ColorVertexBuffer.GetNumVertices())H.Update(static_cast<const uint8*>(B.ColorVertexBuffer.GetVertexData()),B.ColorVertexBuffer.GetNumVertices()*B.ColorVertexBuffer.GetStride());
  H.Final();FSHAHash Out;H.GetHash(Out.Hash);return Out.ToString();
 }
 #include "S47RadioWomanNativePins.inl"
 bool Inside(const FVector& P,const FBox& Box){return Box.ExpandBy(.00002).IsInsideOrOn(P);}
 bool BoxFace(const FVector& A,const FVector& B,const FVector& C,const FBox& Box,int32& Face)
 {
  if(!Inside(A,Box)||!Inside(B,Box)||!Inside(C,Box))return false;
  int32 Matches=0;
  for(int32 Axis=0;Axis<3;++Axis)for(int32 Side=0;Side<2;++Side)
  {
   const double V=Side?Box.Max[Axis]:Box.Min[Axis];
   if(FMath::IsNearlyEqual(A[Axis],V,.00002)&&FMath::IsNearlyEqual(B[Axis],V,.00002)&&FMath::IsNearlyEqual(C[Axis],V,.00002)){Face=Axis*2+Side;++Matches;}
  }
  return Matches==1&&FVector::CrossProduct(B-A,C-A).SizeSquared()>1e-12;
 }
 UStaticMesh* RadioCut(AS47Web* W,UStaticMesh* Original,TArray<uint32>& Full,TArray<uint32>& Radio,FString& Packed)
 {
  Full.Reset();Radio.Reset();Packed.Empty();
  if(!Original||!Original->bAllowCPUAccess||!Original->GetRenderData()||Original->GetRenderData()->LODResources.Num()!=1)return nullptr;
  const auto& Src=Original->GetRenderData()->LODResources[0];Packed=VertexHash(Src);
  if(Src.GetNumTriangles()!=36||Packed.IsEmpty()||!Src.IndexBuffer.GetIndexDataSize())return nullptr;
  Src.IndexBuffer.GetCopy(Full);if(Full.Num()!=108)return nullptr;
  TArray<FStaticMeshSection> Sections;int32 Counts[3][6]={};uint32 Covered=0;
  for(const auto& S:Src.Sections)
  {
   if(S.FirstIndex!=Covered||S.MaterialIndex<0||S.MaterialIndex>=Original->GetStaticMaterials().Num())return nullptr;
   FStaticMeshSection Keep=S;Keep.FirstIndex=Radio.Num();Keep.NumTriangles=0;
   for(uint32 I=S.FirstIndex;I<S.FirstIndex+S.NumTriangles*3;I+=3)
   {
    if(I+2>=uint32(Full.Num())||FMath::Max3(Full[I],Full[I+1],Full[I+2])>=Src.VertexBuffers.PositionVertexBuffer.GetNumVertices())return nullptr;
    const FVector A=S47Three::ToThree(FVector(Src.VertexBuffers.PositionVertexBuffer.VertexPosition(Full[I])));
    const FVector B=S47Three::ToThree(FVector(Src.VertexBuffers.PositionVertexBuffer.VertexPosition(Full[I+1])));
    const FVector C=S47Three::ToThree(FVector(Src.VertexBuffers.PositionVertexBuffer.VertexPosition(Full[I+2])));
    int32 Box=INDEX_NONE,Face=INDEX_NONE,Found=0;
    for(int32 J=0;J<3;++J){int32 F=INDEX_NONE;if(BoxFace(A,B,C,J==0?RadioBox:ShoeBoxes[J-1],F)){Box=J;Face=F;++Found;}}
    if(Found!=1)return nullptr;
    ++Counts[Box][Face];if(Box==0){Radio.Append({Full[I],Full[I+1],Full[I+2]});++Keep.NumTriangles;}
   }
   Covered+=S.NumTriangles*3;if(Keep.NumTriangles)Sections.Add(Keep);
  }
  for(const auto& Box:Counts)for(int32 Count:Box)if(Count!=2)return nullptr;
  if(Covered!=108||Radio.Num()!=36)return nullptr;
  UStaticMesh* Copy=NewObject<UStaticMesh>(W,NAME_None,RF_Transient);
  Copy->bAllowCPUAccess=true;Copy->NeverStream=true;Copy->bSupportRayTracing=Original->bSupportRayTracing;
  Copy->GetStaticMaterials()=Original->GetStaticMaterials();Copy->SetLightMapCoordinateIndex(Original->GetLightMapCoordinateIndex());
  auto Data=MakeUnique<FStaticMeshRenderData>();Data->AllocateLODResources(1);
  const FBox Box(S47Three::ToUE(RadioBox.Min),S47Three::ToUE(RadioBox.Max));Data->Bounds=FBoxSphereBounds(Box);
  Data->ScreenSize[0].Default=1;Data->NumInlinedLODs=1;Data->CurrentFirstLODIdx=0;Data->bReadyForStreaming=false;
  auto& Dst=Data->LODResources[0];Dst.Sections.Append(Sections);
  const auto& B=Src.VertexBuffers;Dst.VertexBuffers.PositionVertexBuffer.Init(B.PositionVertexBuffer,true);
  auto& T=Dst.VertexBuffers.StaticMeshVertexBuffer;const auto& SourceT=B.StaticMeshVertexBuffer;
  T.SetUseHighPrecisionTangentBasis(SourceT.GetUseHighPrecisionTangentBasis());T.SetUseFullPrecisionUVs(SourceT.GetUseFullPrecisionUVs());
  T.Init(SourceT.GetNumVertices(),SourceT.GetNumTexCoords(),true);
  if(T.GetTangentSize()!=SourceT.GetTangentSize()||T.GetTexCoordSize()!=SourceT.GetTexCoordSize())return nullptr;
  FMemory::Memcpy(T.GetTangentData(),SourceT.GetTangentData(),T.GetTangentSize());FMemory::Memcpy(T.GetTexCoordData(),SourceT.GetTexCoordData(),T.GetTexCoordSize());
  if(B.ColorVertexBuffer.GetNumVertices())
  {
   auto& Colors=Dst.VertexBuffers.ColorVertexBuffer;Colors.Init(B.ColorVertexBuffer.GetNumVertices(),true);
   if(Colors.GetStride()!=B.ColorVertexBuffer.GetStride())return nullptr;
   FMemory::Memcpy(Colors.GetVertexData(),B.ColorVertexBuffer.GetVertexData(),Colors.GetNumVertices()*Colors.GetStride());
  }
  Dst.bHasColorVertexData=Src.bHasColorVertexData;Dst.bBuffersInlined=true;Dst.bIsOptionalLOD=false;Dst.SourceMeshBounds=Data->Bounds;
  Dst.IndexBuffer.SetIndices(Radio,EIndexBufferStride::Force32Bit);
  if(!Dst.IndexBuffer.TrySetAllowCPUAccess(true))return nullptr; // UE5.8: check only after SetIndices.
  bool RT=Copy->bSupportRayTracing;
#if !WITH_EDITOR
  RT=RT&&IsRayTracingAllowed();
#endif
  if(RT)Data->InitializeRayTracingRepresentationFromRenderingLODs();
  Copy->SetRenderData(MoveTemp(Data));Copy->SetExtendedBounds(FBoxSphereBounds(Box));
  TArray<uint32> Actual;Copy->GetRenderData()->LODResources[0].IndexBuffer.GetCopy(Actual);
  if(Actual!=Radio||VertexHash(Copy->GetRenderData()->LODResources[0])!=Packed)return nullptr;
  return Copy;
 }
 bool NativeReady(UStaticMesh* Mesh,TArray<int32>& LODs)
 {
  if(!Mesh||!Mesh->bAllowCPUAccess||!Mesh->bSupportRayTracing||Mesh->GetNaniteSettings().bEnabled||!Mesh->GetRenderData()
   ||!Mesh->GetRenderData()->IsInitialized()||Mesh->GetRenderData()->LODResources.Num()!=3||Mesh->GetStaticMaterials().IsEmpty())return false;
  int32 Previous=MAX_int32;const int32 Limits[]={150000,70000,40000};
  for(int32 I=0;I<3;++I)
  {
   const auto& L=Mesh->GetRenderData()->LODResources[I];const int32 N=L.GetNumTriangles();
   if(N<=0||N>=Previous||N>Limits[I]||!L.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords())return false;
   Previous=N;LODs.Add(N);
  }
  for(const auto& M:Mesh->GetStaticMaterials())if(!M.MaterialInterface)return false;
  const FBox B=Mesh->GetBoundingBox();
  return B.IsValid&&B.Min.Z< -15&&B.Min.Z> -80&&B.Max.Z>50&&B.Max.Z<135&&B.GetSize().X>30&&B.GetSize().X<140&&B.GetSize().Y>20&&B.GetSize().Y<140;
 }
 TArray<TSharedPtr<FJsonValue>> Vec(const FVector& P){return {MakeShared<FJsonValueNumber>(P.X),MakeShared<FJsonValueNumber>(P.Y),MakeShared<FJsonValueNumber>(P.Z)};}
 TSharedPtr<FJsonObject> Transform(const FTransform& T)
 {
  auto J=MakeShared<FJsonObject>();const FQuat Q=T.GetRotation();
  J->SetArrayField(TEXT("location_ue_cm"),Vec(T.GetLocation()));J->SetArrayField(TEXT("scale"),Vec(T.GetScale3D()));
  const TArray<TSharedPtr<FJsonValue>> Rotation={MakeShared<FJsonValueNumber>(Q.X),MakeShared<FJsonValueNumber>(Q.Y),MakeShared<FJsonValueNumber>(Q.Z),MakeShared<FJsonValueNumber>(Q.W)};
  J->SetArrayField(TEXT("rotation_xyzw"),Rotation);return J;
 }
 TSharedPtr<FJsonObject> ComponentIdentity(AS47Web* W,int32 Index)
 {
  auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("node"),Index);const auto* N=W?W->Node(Index):nullptr;
  auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;J->SetBoolField(TEXT("available"),C!=nullptr);if(!C)return J;
  J->SetNumberField(TEXT("parent"),N->Parent);J->SetNumberField(TEXT("geo"),N->Geo);J->SetStringField(TEXT("name"),N->Name);
  J->SetStringField(TEXT("component"),GetPathNameSafe(C));J->SetStringField(TEXT("mesh"),GetPathNameSafe(C->GetStaticMesh().Get()));
  J->SetBoolField(TEXT("bVisible"),C->IsVisible());J->SetBoolField(TEXT("node_bVis"),N->bVis);J->SetBoolField(TEXT("shown"),W->ObjAt(Index).Shown());
  J->SetBoolField(TEXT("hidden_in_game"),C->bHiddenInGame);J->SetBoolField(TEXT("registered"),C->IsRegistered());J->SetNumberField(TEXT("collision_enabled"),int32(C->GetCollisionEnabled()));
  J->SetObjectField(TEXT("transform"),Transform(C->GetComponentTransform()));J->SetObjectField(TEXT("relative_transform"),Transform(C->GetRelativeTransform()));J->SetStringField(TEXT("area"),W->Area());
  TArray<TSharedPtr<FJsonValue>> Ancestors;int32 Parent=N->Parent;
  for(int32 Depth=0;Depth<8&&Parent!=INDEX_NONE;++Depth)
  {
   const auto* A=W->Node(Parent);if(!A)break;auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("node"),Parent);Row->SetStringField(TEXT("name"),A->Name);Row->SetBoolField(TEXT("node_bVis"),A->bVis);
   if(A->Comp.IsValid()){Row->SetStringField(TEXT("component"),GetPathNameSafe(A->Comp.Get()));Row->SetBoolField(TEXT("bVisible"),A->Comp->IsVisible());Row->SetBoolField(TEXT("hidden_in_game"),A->Comp->bHiddenInGame);Row->SetObjectField(TEXT("transform"),Transform(A->Comp->GetComponentTransform()));}
   Ancestors.Add(MakeShared<FJsonValueObject>(Row));Parent=A->Parent;
  }
  J->SetArrayField(TEXT("ancestors"),Ancestors);return J;
 }
 TSharedPtr<FJsonObject> SurfaceIdentity(AS47Web* W,const FHitResult& Hit,UStaticMeshComponent* Shell)
 {
  auto J=MakeShared<FJsonObject>();auto* C=Hit.GetComponent();const int32 Index=W->NodeOf(C);
  J->SetStringField(TEXT("component"),GetPathNameSafe(C));J->SetNumberField(TEXT("node"),Index);J->SetNumberField(TEXT("face_index"),Hit.FaceIndex);
  J->SetBoolField(TEXT("bVisible"),C&&C->IsVisible());J->SetBoolField(TEXT("hidden_in_game"),C&&C->bHiddenInGame);J->SetStringField(TEXT("area_at_measurement"),W->Area());
  J->SetBoolField(TEXT("shown_at_measurement"),Index!=INDEX_NONE&&W->ObjAt(Index).Shown());J->SetBoolField(TEXT("same_laundry_shell"),C&&C==Shell);
  if(C)
  {
   J->SetArrayField(TEXT("hit_component_local_ue_cm"),Vec(C->GetComponentTransform().InverseTransformPosition(Hit.ImpactPoint)));
   J->SetArrayField(TEXT("normal_component_local"),Vec(C->GetComponentTransform().InverseTransformVectorNoScale(Hit.ImpactNormal).GetSafeNormal()));
   int32 Section=INDEX_NONE;auto* Material=Hit.FaceIndex>=0?C->GetMaterialFromCollisionFaceIndex(Hit.FaceIndex,Section):nullptr;
   J->SetNumberField(TEXT("material_section"),Section);J->SetStringField(TEXT("material"),GetPathNameSafe(Material));J->SetBoolField(TEXT("material_available"),Material!=nullptr);
   if(auto* MeshC=Cast<UStaticMeshComponent>(C))J->SetStringField(TEXT("mesh"),GetPathNameSafe(MeshC->GetStaticMesh().Get()));
  }
  FString Role=TEXT("external_surface_candidate_not_yet_support_acceptance");
  if(C&&C==Shell)
  {
   const FVector P=Shell->GetComponentTransform().InverseTransformPosition(Hit.ImpactPoint);
   const bool Bench=P.X>=19-.02&&P.X<=67+.02&&P.Y>=-165-.02&&P.Y<=25+.02&&P.Z>=-.02&&P.Z<=82+.02;
   Role=Bench?TEXT("bench_geometry_not_ground"):TEXT("other_laundry_shell_geometry");
  }
  if(!C)Role=TEXT("no_component");J->SetStringField(TEXT("role"),Role);return J;
 }
 TSharedPtr<FJsonObject> TraceData(AS47Web* W,UStaticMeshComponent* Shell,const FHitResult& Hit,bool Found,const FVector& Start,const FVector& End)
 {
  auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("hit"),Found);J->SetBoolField(TEXT("blocking_hit"),Hit.bBlockingHit);J->SetBoolField(TEXT("start_penetrating"),Hit.bStartPenetrating);
  J->SetArrayField(TEXT("trace_start_ue_cm"),Vec(Start));J->SetArrayField(TEXT("trace_end_ue_cm"),Vec(End));J->SetArrayField(TEXT("impact_ue_cm"),Vec(Hit.ImpactPoint));J->SetArrayField(TEXT("impact_normal_ue"),Vec(Hit.ImpactNormal));
  J->SetObjectField(TEXT("surface_identity"),SurfaceIdentity(W,Hit,Shell));return J;
 }
 TSharedPtr<FJsonObject> SoleGroundDiagnostics(AS47Web* W,UStaticMesh* Mesh,const FTransform& Placement,const FCollisionQueryParams& Query,UStaticMeshComponent* DiagnosticShell=nullptr)
 {
  auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("scope"),TEXT("Actual startup contact evidence: every unique lowest LOD0 Shoe/Trim vertex within0.15cm of its side minimum, at the measured seat and unchanged Group basis; all points must satisfy the strict contact gate before adoption."));
  if(DiagnosticShell)J->SetStringField(TEXT("scope"),TEXT("Read-only R11 diagnosis: R10 lowest sole geometry projected into the actually measured TB2 seat frame. Gaps are measurements, not contact/adoption acceptance; the entire Laundry Shell remains in the world query."));
  J->SetStringField(TEXT("source_asset"),GetPathNameSafe(Mesh));J->SetObjectField(TEXT("measured_seat_transform"),Transform(Placement));J->SetNumberField(TEXT("lowest_band_cm"),.15);
  auto* World=W?W->GetWorld():nullptr;J->SetBoolField(TEXT("world_available"),World!=nullptr);
  const bool Available=Mesh&&Mesh->bAllowCPUAccess&&Mesh->GetRenderData()&&Mesh->GetRenderData()->LODResources.Num()==3;
  J->SetBoolField(TEXT("geometry_available"),Available);if(!Available)return J;
  const auto& L=Mesh->GetRenderData()->LODResources[0];TArray<uint32> Indices;L.IndexBuffer.GetCopy(Indices);TSet<uint32> Candidates[2];double Low[2]={1e20,1e20};bool Valid=true;
  for(const auto& Section:L.Sections)
  {
   if(!Mesh->GetStaticMaterials().IsValidIndex(Section.MaterialIndex))continue;
   const auto* Material=Mesh->GetMaterial(Section.MaterialIndex);if(!Material||(Material->GetName()!=TEXT("M_RW_Shoe")&&Material->GetName()!=TEXT("M_RW_Trim")))continue;
   for(uint32 I=Section.FirstIndex;I<Section.FirstIndex+Section.NumTriangles*3;++I)
   {
    if(I>=uint32(Indices.Num())||Indices[I]>=L.VertexBuffers.PositionVertexBuffer.GetNumVertices()){Valid=false;break;}
    const FVector P(L.VertexBuffers.PositionVertexBuffer.VertexPosition(Indices[I]));if(P.Z>=-15)continue;
    const int32 Side=P.Y<0?0:1;Candidates[Side].Add(Indices[I]);Low[Side]=FMath::Min(Low[Side],P.Z);
   }
  }
  J->SetBoolField(TEXT("selection_indices_valid"),Valid);if(!Valid)return J;
  TArray<TSharedPtr<FJsonValue>> Feet;
  for(int32 Side=0;Side<2;++Side)
  {
   auto Foot=MakeShared<FJsonObject>();Foot->SetNumberField(TEXT("side"),Side);Foot->SetStringField(TEXT("native_side"),Side==0?TEXT("negative_Y"):TEXT("positive_Y"));Foot->SetNumberField(TEXT("candidate_vertices"),Candidates[Side].Num());
   TArray<FVector> Unique;TArray<uint32> UniqueIndices;TArray<double> Gaps,GroundHeights;TArray<TSharedPtr<FJsonValue>> Samples;FBox LocalBounds(ForceInit),WorldBounds(ForceInit);int32 UpwardHits=0;
   for(uint32 Index:Candidates[Side])
   {
    const FVector P(L.VertexBuffers.PositionVertexBuffer.VertexPosition(Index));
    if(P.Z<=Low[Side]+.15&&!Unique.ContainsByPredicate([&](const FVector& V){return V.Equals(P,.00001);})){Unique.Add(P);UniqueIndices.Add(Index);}
   }
   Foot->SetNumberField(TEXT("unique_lowest_sole_vertices"),Unique.Num());if(!Unique.IsEmpty())Foot->SetNumberField(TEXT("lowest_native_z_cm"),Low[Side]);
   for(int32 I=0;I<Unique.Num();++I)
   {
    const FVector Local=Unique[I],NativeWorld=Placement.TransformPosition(Local),Start=NativeWorld+FVector(0,0,50),End=NativeWorld-FVector(0,0,100);FHitResult Hit;
    const bool Found=World&&World->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Query);LocalBounds+=Local;WorldBounds+=NativeWorld;
    auto Sample=MakeShared<FJsonObject>();Sample->SetNumberField(TEXT("native_vertex_index"),UniqueIndices[I]);Sample->SetArrayField(TEXT("native_local_cm"),Vec(Local));Sample->SetArrayField(TEXT("native_world_ue_cm"),Vec(NativeWorld));
    Sample->SetArrayField(TEXT("trace_start_ue_cm"),Vec(Start));Sample->SetArrayField(TEXT("trace_end_ue_cm"),Vec(End));Sample->SetBoolField(TEXT("hit"),Found);Sample->SetBoolField(TEXT("blocking_hit"),Hit.bBlockingHit);
    Sample->SetArrayField(TEXT("impact_ue_cm"),Vec(Hit.ImpactPoint));Sample->SetArrayField(TEXT("impact_normal_ue"),Vec(Hit.ImpactNormal));Sample->SetStringField(TEXT("hit_component"),GetPathNameSafe(Hit.GetComponent()));Sample->SetNumberField(TEXT("hit_node"),W->NodeOf(Hit.GetComponent()));Sample->SetNumberField(TEXT("face_index"),Hit.FaceIndex);
    if(DiagnosticShell)Sample->SetObjectField(TEXT("surface_identity"),SurfaceIdentity(W,Hit,DiagnosticShell));
    Sample->SetBoolField(TEXT("upward_surface"),Found&&Hit.ImpactNormal.Z>.7);
    if(Found){const double Gap=NativeWorld.Z-Hit.ImpactPoint.Z;Sample->SetNumberField(TEXT("gap_cm"),Gap);Sample->SetNumberField(TEXT("measured_seat_above_impact_cm"),Placement.GetLocation().Z-Hit.ImpactPoint.Z);Gaps.Add(Gap);GroundHeights.Add(Hit.ImpactPoint.Z);if(Hit.ImpactNormal.Z>.7)++UpwardHits;}
    else Sample->SetField(TEXT("gap_cm"),MakeShared<FJsonValueNull>());
    Samples.Add(MakeShared<FJsonValueObject>(Sample));
   }
   Foot->SetArrayField(TEXT("samples"),Samples);Foot->SetNumberField(TEXT("ground_hits"),Gaps.Num());Foot->SetNumberField(TEXT("upward_ground_hits"),UpwardHits);
   if(LocalBounds.IsValid){Foot->SetArrayField(TEXT("sole_bounds_native_min_cm"),Vec(LocalBounds.Min));Foot->SetArrayField(TEXT("sole_bounds_native_max_cm"),Vec(LocalBounds.Max));Foot->SetArrayField(TEXT("sole_bounds_world_min_ue_cm"),Vec(WorldBounds.Min));Foot->SetArrayField(TEXT("sole_bounds_world_max_ue_cm"),Vec(WorldBounds.Max));}
   if(!Gaps.IsEmpty()){Gaps.Sort();GroundHeights.Sort();Foot->SetNumberField(TEXT("minimum_gap_cm"),Gaps[0]);Foot->SetNumberField(TEXT("maximum_gap_cm"),Gaps.Last());Foot->SetNumberField(TEXT("median_gap_cm"),Gaps[Gaps.Num()/2]);Foot->SetNumberField(TEXT("minimum_ground_ue_cm"),GroundHeights[0]);Foot->SetNumberField(TEXT("maximum_ground_ue_cm"),GroundHeights.Last());Foot->SetNumberField(TEXT("median_ground_ue_cm"),GroundHeights[GroundHeights.Num()/2]);}
   Feet.Add(MakeShared<FJsonValueObject>(Foot));
  }
  J->SetArrayField(TEXT("feet"),Feet);return J;
 }

 bool SoleGroundReady(const TSharedPtr<FJsonObject>& Data)
 {
  bool Available=false,Valid=false,World=false;const TArray<TSharedPtr<FJsonValue>>* Feet=nullptr;
  if(!Data||!Data->TryGetBoolField(TEXT("geometry_available"),Available)||!Available
   ||!Data->TryGetBoolField(TEXT("selection_indices_valid"),Valid)||!Valid
   ||!Data->TryGetBoolField(TEXT("world_available"),World)||!World
   ||!Data->TryGetArrayField(TEXT("feet"),Feet)||Feet->Num()!=2)return false;
  for(const auto& Value:*Feet)
  {
   const auto Foot=Value->AsObject();double Count=0,Hits=0,Up=0,Min=0,Max=0;
   if(!Foot||!Foot->TryGetNumberField(TEXT("unique_lowest_sole_vertices"),Count)||Count<3||Count>256
    ||!Foot->TryGetNumberField(TEXT("ground_hits"),Hits)||Hits!=Count
    ||!Foot->TryGetNumberField(TEXT("upward_ground_hits"),Up)||Up!=Count
    ||!Foot->TryGetNumberField(TEXT("minimum_gap_cm"),Min)||!FMath::IsFinite(Min)||Min< -MaxSoleGapCm
    ||!Foot->TryGetNumberField(TEXT("maximum_gap_cm"),Max)||!FMath::IsFinite(Max)||Max>MaxSoleGapCm)return false;
  }
  return true;
 }

}

struct FS47RadioWomanState
{
 TWeakObjectPtr<AS47Web> Web;TArray<FOriginal> Original;TStrongObjectPtr<UStaticMesh> NativeAsset;
 TWeakObjectPtr<UStaticMesh> Cut;TWeakObjectPtr<UStaticMeshComponent> Native;
 FString Why=TEXT("not attempted"),Asset,Packed;bool Active=false;int32 NativeNode=INDEX_NONE,SeatHits=0;
 FTransform Placement;FVector Seat=FVector::ZeroVector,Forward=FVector::ZeroVector,Right=FVector::ZeroVector;
 TSharedPtr<FJsonObject> BenchDiagnostics=MakeShared<FJsonObject>();
 bool BenchDiagnosticRequested=false,BenchDiagnosticPostRepair=false,BenchDiagnosticConflict=false;int32 DiagnosticBenchNode=INDEX_NONE,DiagnosticReadbacks=0;TSet<int32> DiagnosticHitNodes;
 TArray<uint32> FullIndices,RadioIndices;TArray<int32> LODs;
 bool Prepared=false,Pending=false;uint64 AreaEpoch=1,MeasuredEpoch=0,TownStepSerial=0,MeasuredTownStepSerial=0;
 int64 AttemptedEpoch=-1;FVector RadioDeltaUE=FVector::ZeroVector,RadioDeltaLocalThree=FVector::ZeroVector;
};

namespace
{
 // Private generated diagnostic proposal: no adoption, setters, scene hiding or asset saving.
 namespace S47RadioBenchPost
 {
  const FVector SourcePivot(0.42999999999999999,0.69999999999999996,0),TargetPivot(0.42999999999999999,0.69999999999999996,-0.487272127600062);
  const FVector RX(0.99992453754711386,-0.00017619361113445156,-0.012283654464442479),RY(-0.00017619361113445156,0.99958861410652189,-0.028680507394643031),RZ(0.012283654464442479,0.028680507394643031,0.99951315165363586);
  struct FSolid {const TCHAR* Name;FBox Box;};
  const FSolid Solids[]={
   {TEXT("Bench_cast_leg_front"),FBox(FVector(0.58999999999999997,-0.155,0),FVector(0.63,-0.10500000000000001,0.42999999999999999))},
   {TEXT("Bench_cast_leg_back"),FBox(FVector(0.21000000000000002,-0.155,0),FVector(0.25,-0.10500000000000001,0.81999999999999995))},
   {TEXT("Bench_seat_bearer"),FBox(FVector(0.21000000000000005,-0.155,0.40000000000000002),FVector(0.65000000000000002,-0.10500000000000001,0.44000000000000006))},
   {TEXT("Bench_foot"),FBox(FVector(0.19000000000000003,-0.16,0),FVector(0.67000000000000004,-0.10000000000000001,0.029999999999999999))},
   {TEXT("Bench_cast_leg_front"),FBox(FVector(0.58999999999999997,1.5049999999999999,0),FVector(0.63,1.5549999999999997,0.42999999999999999))},
   {TEXT("Bench_cast_leg_back"),FBox(FVector(0.21000000000000002,1.5049999999999999,0),FVector(0.25,1.5549999999999997,0.81999999999999995))},
   {TEXT("Bench_seat_bearer"),FBox(FVector(0.21000000000000005,1.5049999999999999,0.40000000000000002),FVector(0.65000000000000002,1.5549999999999997,0.44000000000000006))},
   {TEXT("Bench_foot"),FBox(FVector(0.19000000000000003,1.4999999999999998,0),FVector(0.67000000000000004,1.5599999999999998,0.029999999999999999))},
   {TEXT("Bench_seat_slat"),FBox(FVector(0.26000000000000001,-0.25,0.43999999999999995),FVector(0.33499999999999996,1.6499999999999999,0.46999999999999997))},
   {TEXT("Bench_seat_slat"),FBox(FVector(0.35999999999999999,-0.25,0.43999999999999995),FVector(0.43499999999999994,1.6499999999999999,0.46999999999999997))},
   {TEXT("Bench_seat_slat"),FBox(FVector(0.46000000000000008,-0.25,0.43999999999999995),FVector(0.53500000000000003,1.6499999999999999,0.46999999999999997))},
   {TEXT("Bench_seat_slat"),FBox(FVector(0.56000000000000005,-0.25,0.43999999999999995),FVector(0.63500000000000001,1.6499999999999999,0.46999999999999997))},
   {TEXT("Bench_back_slat"),FBox(FVector(0.23999999999999999,-0.25,0.56000000000000005),FVector(0.27000000000000002,1.6499999999999999,0.64500000000000002))},
   {TEXT("Bench_back_slat"),FBox(FVector(0.23999999999999999,-0.25,0.69999999999999996),FVector(0.27000000000000002,1.6499999999999999,0.78499999999999992))}
  };
  const FVector FootGrid[]={
   FVector(0.19,-0.16,0),
   FVector(0.19,-0.13,0),
   FVector(0.19,-0.10000000000000001,0),
   FVector(0.19,1.5,0),
   FVector(0.19,1.53,0),
   FVector(0.19,1.5600000000000001,0),
   FVector(0.23000000000000001,-0.16,0),
   FVector(0.23000000000000001,-0.13,0),
   FVector(0.23000000000000001,-0.10000000000000001,0),
   FVector(0.23000000000000001,1.5,0),
   FVector(0.23000000000000001,1.53,0),
   FVector(0.23000000000000001,1.5600000000000001,0),
   FVector(0.42999999999999999,-0.16,0),
   FVector(0.42999999999999999,-0.13,0),
   FVector(0.42999999999999999,-0.10000000000000001,0),
   FVector(0.42999999999999999,1.5,0),
   FVector(0.42999999999999999,1.53,0),
   FVector(0.42999999999999999,1.5600000000000001,0),
   FVector(0.60999999999999999,-0.16,0),
   FVector(0.60999999999999999,-0.13,0),
   FVector(0.60999999999999999,-0.10000000000000001,0),
   FVector(0.60999999999999999,1.5,0),
   FVector(0.60999999999999999,1.53,0),
   FVector(0.60999999999999999,1.5600000000000001,0),
   FVector(0.67000000000000004,-0.16,0),
   FVector(0.67000000000000004,-0.13,0),
   FVector(0.67000000000000004,-0.10000000000000001,0),
   FVector(0.67000000000000004,1.5,0),
   FVector(0.67000000000000004,1.53,0),
   FVector(0.67000000000000004,1.5600000000000001,0)
  };
  FVector Repair(const FVector& P){const auto V=P-SourcePivot;return TargetPivot+RX*V.X+RY*V.Y+RZ*V.Z;}
  FVector Unrepair(const FVector& P){const auto V=P-TargetPivot;return SourcePivot+FVector(FVector::DotProduct(RX,V),FVector::DotProduct(RY,V),FVector::DotProduct(RZ,V));}
  FVector BToCm(const FVector& P){return FVector(P.X*100,-P.Y*100,P.Z*100);}
  FVector CmToB(const FVector& P){return FVector(P.X/100,-P.Y/100,P.Z/100);}
  FVector WorldFromOld(const FTransform& Frame,const FVector& P){return Frame.TransformPosition(BToCm(Repair(P)));}
  FVector OldFromWorld(const FTransform& Frame,const FVector& P){return Unrepair(CmToB(Frame.InverseTransformPosition(P)));}
  int32 RootOf(AS47Web* W,int32 Index)
  {
   for(int32 D=0;D<32&&W;++D){const auto* N=W->Node(Index);if(!N)return INDEX_NONE;if(N->Parent==INDEX_NONE)return Index;Index=N->Parent;}return INDEX_NONE;
  }
  bool VisibleNow(AS47Web* W,UPrimitiveComponent* C,int32 Index)
  {
   const auto* N=W?W->Node(Index):nullptr;
   return N&&C&&N->Comp.Get()==C&&W->NodeOf(C)==Index&&C->IsRegistered()&&W->ObjAt(Index).Shown()
    &&C->IsVisible()&&!C->bHiddenInGame&&(!C->GetOwner()||!C->GetOwner()->IsHidden());
  }
  bool ActiveRoot(AS47Web* W,int32 Index)
  {
   const int32 Root=RootOf(W,Index);return W&&(W->Area()==TEXT("diner")||W->Area()==TEXT("road"))&&(Root==1240||Root==1431);
  }
  bool OldSeat(const FVector& P)
  {
   for(const auto& B:Solids)if(FString(B.Name)==TEXT("Bench_seat_slat")&&B.Box.ExpandBy(.0002).IsInsideOrOn(P)&&FMath::Abs(P.Z-B.Box.Max.Z)<=.0002)return true;
   return false;
  }
  FString BenchRole(const FVector& P)
  {
   for(const auto& B:Solids)if(B.Box.ExpandBy(.0002).IsInsideOrOn(P))return FString(TEXT("bench_not_ground:"))+B.Name;
   return TEXT("other_shell_surface_without_authored_floor_role");
  }
  TSharedPtr<FJsonObject> Identity(AS47Web* W,const FHitResult& H,UStaticMeshComponent* Shell)
  {
   auto J=SurfaceIdentity(W,H,Shell);auto* C=H.GetComponent();const int32 Index=W?W->NodeOf(C):INDEX_NONE;
   const int32 Root=RootOf(W,Index);J->SetNumberField(TEXT("actual_root_node"),Root);
   J->SetObjectField(TEXT("component_live"),ComponentIdentity(W,Index));J->SetBoolField(TEXT("registered_at_measurement"),C&&C->IsRegistered());
   const bool Visible=VisibleNow(W,C,Index);J->SetBoolField(TEXT("actually_shown_registered"),Visible);J->SetBoolField(TEXT("active_area_root"),ActiveRoot(W,Index));
   FString Role=TEXT("unclassified_component_not_support_acceptance");
   if(C==Shell&&Shell)
   {
    const FVector Old=OldFromWorld(Shell->GetComponentTransform(),H.ImpactPoint);
    J->SetArrayField(TEXT("hit_unrepaired_blender_m"),Vec(Old));Role=BenchRole(Old);
   }
   else if(const auto* N=W?W->Node(Index):nullptr)
   {
    const auto* M=Cast<UStaticMeshComponent>(C);const FString Path=M?GetPathNameSafe(M->GetStaticMesh().Get()):FString();
    if(!Visible)Role=TEXT("hidden_or_inactive_component_not_support");
    else if(N->Geo==873&&Index==1242&&Root==1240&&Path==TEXT("/Game/Web/Geo/geo_007/StaticMeshes/g873.g873"))Role=TEXT("measured_visible_active_road_desert");
    else if(N->Geo==1004&&Index==1453&&Root==1431&&Path==TEXT("/Game/TownBuildings/Trim/StaticMeshes/g1004.g1004"))Role=TEXT("measured_visible_retained_town_pavement");
    else Role=TEXT("other_visible_surface_without_verified_authored_support_role");
   }
   J->SetStringField(TEXT("role"),Role);J->SetBoolField(TEXT("contact_accepted"),false);return J;
  }
  TSharedPtr<FJsonObject> Record(AS47Web* W,UStaticMeshComponent* Shell,const FHitResult& H,bool Found,const FVector& Start,const FVector& End)
  {
   auto J=TraceData(W,Shell,H,Found,Start,End);J->SetObjectField(TEXT("surface_identity"),Identity(W,H,Shell));return J;
  }
  struct FProbe {TSharedPtr<FJsonObject> Json;FHitResult Top;bool Found=false,Complete=true;};
  FProbe WorldProbe(AS47Web* W,UStaticMeshComponent* Shell,const FVector& Start,const FVector& End,const FCollisionQueryParams& Base,TSet<int32>& Seen)
  {
   FProbe Out;Out.Json=MakeShared<FJsonObject>();FCollisionQueryParams Q=Base;TArray<TSharedPtr<FJsonValue>> Rows;bool Stopped=false;
   for(int32 Attempt=0;Attempt<32;++Attempt)
   {
    FHitResult H;const bool Found=W->GetWorld()->LineTraceSingleByChannel(H,Start,End,ECC_Visibility,Q);
    auto Row=Record(W,Shell,H,Found,Start,End);Row->SetNumberField(TEXT("attempt"),Attempt);
    auto* C=H.GetComponent();const int32 Index=W->NodeOf(C);if(Index!=INDEX_NONE)Seen.Add(Index);
    // A current, mapped hidden component can be excluded per query. Visible unknowns stop the ray.
    const auto* N=W->Node(Index);const bool Known=N&&C&&N->Comp.Get()==C&&C->IsRegistered();
    const bool Hidden=Known&&(!W->ObjAt(Index).Shown()||!C->IsVisible()||C->bHiddenInGame||(C->GetOwner()&&C->GetOwner()->IsHidden()));
    const bool Exclude=Found&&C!=Shell&&Hidden;
    Row->SetBoolField(TEXT("query_local_exclusion"),Exclude);Row->SetStringField(TEXT("exclusion_reason"),Exclude?TEXT("fresh node/ancestor/component hidden state; no global collision/visibility mutation"):TEXT("none"));
    Rows.Add(MakeShared<FJsonValueObject>(Row));
    if(Exclude){Q.AddIgnoredComponent(C);continue;}
    Out.Top=H;Out.Found=Found&&H.bBlockingHit&&C!=Shell&&VisibleNow(W,C,Index)&&ActiveRoot(W,Index)&&H.ImpactNormal.Z>.7;
    Stopped=true;break;
   }
   Out.Complete=Stopped;Out.Json->SetArrayField(TEXT("ordered_single_hit_queries"),Rows);Out.Json->SetBoolField(TEXT("bounded_query_complete"),Stopped);
   Out.Json->SetBoolField(TEXT("visible_upward_surface_candidate"),Out.Found);Out.Json->SetBoolField(TEXT("entire_laundry_shell_ignored"),false);
   Out.Json->SetBoolField(TEXT("contact_accepted"),false);return Out;
  }
  struct FProvider {int32 Index;UStaticMeshComponent* C;};
  TArray<FProvider> Providers(AS47Web* W,UStaticMeshComponent* Shell,const TSet<UPrimitiveComponent*>& Person)
  {
   TArray<FProvider> Out;
   for(int32 I=0;I<W->PartCount();++I)
   {
    const auto* N=W->Node(I);auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
    if(!C||C==Shell||Person.Contains(C)||!VisibleNow(W,C,I)||!ActiveRoot(W,I)||!CollisionEnabledHasQuery(C->GetCollisionEnabled())||C->GetCollisionResponseToChannel(ECC_Visibility)!=ECR_Block)continue;
    Out.Add({I,C});
   }
   return Out;
  }
  FProbe DirectProbe(AS47Web* W,UStaticMeshComponent* Shell,const TArray<FProvider>& Candidates,const FVector& Start,const FVector& End,const FCollisionQueryParams& Q,TSet<int32>& Seen)
  {
   FProbe Out;Out.Json=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;double Best=-1e30;
   for(const auto& P:Candidates)
   {
    const FBox B=P.C->Bounds.GetBox().ExpandBy(.02);
    if(!B.IsValid||Start.X<B.Min.X||Start.X>B.Max.X||Start.Y<B.Min.Y||Start.Y>B.Max.Y||FMath::Max(Start.Z,End.Z)<B.Min.Z||FMath::Min(Start.Z,End.Z)>B.Max.Z)continue;
    FHitResult H;const bool Found=P.C->LineTraceComponent(H,Start,End,Q);auto Row=Record(W,Shell,H,Found,Start,End);
    Row->SetNumberField(TEXT("requested_node"),P.Index);Row->SetStringField(TEXT("requested_component"),GetPathNameSafe(P.C));Rows.Add(MakeShared<FJsonValueObject>(Row));
    if(Found)Seen.Add(P.Index);
    // LineTraceComponent returns its hit via bool; observed native component hits have bBlockingHit=false.
    // Preserve that raw flag in Record. WorldProbe still requires its world blocking-hit contract.
    if(Found&&H.GetComponent()==P.C&&VisibleNow(W,P.C,P.Index)&&ActiveRoot(W,P.Index)&&H.ImpactNormal.Z>.7&&H.ImpactPoint.Z>Best){Best=H.ImpactPoint.Z;Out.Top=H;Out.Found=true;}
   }
   Out.Json->SetArrayField(TEXT("actual_component_queries"),Rows);Out.Json->SetBoolField(TEXT("visible_upward_surface_candidate"),Out.Found);
   Out.Json->SetStringField(TEXT("scope"),TEXT("Separate exact registered visible component queries; Shell remains in the world probe. Highest measured upward candidate is not support/contact acceptance."));
   if(Out.Found)Out.Json->SetObjectField(TEXT("highest_measured_visible_candidate"),Record(W,Shell,Out.Top,true,Start,End));
   Out.Json->SetBoolField(TEXT("contact_accepted"),false);return Out;
  }
  TSharedPtr<FJsonObject> Support(AS47Web* W,UStaticMeshComponent* Shell,const TArray<FProvider>& Candidates,const FVector& At,const FCollisionQueryParams& Q,TSet<int32>& Seen,bool& Complete,bool Near)
  {
   auto J=MakeShared<FJsonObject>();const FVector Start=At+FVector(0,0,50),End=At-FVector(0,0,Near?150:100);
   auto Raw=WorldProbe(W,Shell,Start,End,Q,Seen),Direct=DirectProbe(W,Shell,Candidates,Start,End,Q,Seen);
   J->SetArrayField(TEXT("measured_or_projected_point_ue_cm"),Vec(At));J->SetObjectField(TEXT("world_probe_with_conditional_hidden_exclusions"),Raw.Json);
   J->SetObjectField(TEXT("direct_visible_component_probes"),Direct.Json);J->SetBoolField(TEXT("visible_surface_identified"),Direct.Found);
   J->SetBoolField(TEXT("world_direct_visible_provider_agree"),Raw.Found&&Direct.Found&&Raw.Top.GetComponent()==Direct.Top.GetComponent()&&Raw.Top.ImpactPoint.Equals(Direct.Top.ImpactPoint,.02));
   if(Raw.Found)J->SetNumberField(TEXT("gap_to_filtered_world_candidate_cm"),At.Z-Raw.Top.ImpactPoint.Z);
   if(Direct.Found)J->SetNumberField(TEXT("gap_to_direct_visible_candidate_cm"),At.Z-Direct.Top.ImpactPoint.Z);
   if(Near)
   {
    const FVector A=At+FVector(0,0,1),B=At-FVector(0,0,1);auto Tight=DirectProbe(W,Shell,Candidates,A,B,Q,Seen);
    J->SetObjectField(TEXT("direct_near_contact_component_probes_no_skipped_gap"),Tight.Json);
    J->SetNumberField(TEXT("near_contact_half_interval_cm"),1.);if(Tight.Found)J->SetNumberField(TEXT("gap_to_direct_near_candidate_cm"),At.Z-Tight.Top.ImpactPoint.Z);
   }
   Complete=Raw.Complete&&Direct.Found;J->SetBoolField(TEXT("contact_accepted"),false);J->SetBoolField(TEXT("runtime_accepted"),false);return J;
  }
  bool Run(FS47RadioWomanState& S,UStaticMesh* OptionalProjectionMesh=nullptr)
  {
   S.BenchDiagnostics=MakeShared<FJsonObject>();auto& D=S.BenchDiagnostics;auto* W=S.Web.Get();S.DiagnosticHitNodes.Reset();
   D->SetNumberField(TEXT("schema_version"),2);D->SetStringField(TEXT("source_geometry_phase"),TEXT("POST_REPAIR_RIGID_SOURCE_FRAME_NATIVE_QUERY"));
   D->SetStringField(TEXT("geometry_proof_sha256"),TEXT("25dcfa0a41a8a9a5a8dc9db78e932a98c56401307746eef51391dbd56a17fd75"));
   D->SetBoolField(TEXT("read_only"),true);D->SetBoolField(TEXT("adoption_attempted"),false);D->SetBoolField(TEXT("runtime_accepted"),false);D->SetBoolField(TEXT("contact_accepted"),false);
   D->SetStringField(TEXT("scope"),TEXT("Source rigid transform only chooses and classifies actual native queries. No source coordinates substitute for hits. Current visible surfaces are measured independently; g1004 is not prescribed for shoe support. No setters/RadioCut/save/bake/adoption."));
   D->SetNumberField(TEXT("readback_serial"),++S.DiagnosticReadbacks);D->SetNumberField(TEXT("query_monotonic_seconds"),FPlatformTime::Seconds());
   TArray<TSharedPtr<FJsonValue>> Errors;bool Good=true;auto Check=[&](bool Pass,const TCHAR* Why){if(!Pass){Good=false;Errors.Add(MakeShared<FJsonValueString>(Why));}return Pass;};
   auto Finish=[&](){D->SetArrayField(TEXT("mismatches"),Errors);D->SetBoolField(TEXT("measurement_valid"),Good);D->SetStringField(TEXT("status"),Good?TEXT("MEASURED_NO_SUPPORT_ACCEPTANCE"):TEXT("REJECTED_MEASUREMENT"));S.Why=Good?TEXT("postrepair native measurements only; contact/adoption false"):TEXT("postrepair query rejected; original figure/radio retained");return Good;};
   if(!Check(!S.BenchDiagnosticConflict,TEXT("choose exactly one PRE_REPAIR or POST_REPAIR diagnostic flag"))||!Check(W&&W->GetWorld()&&S.Original.Num()==6,TEXT("actual Web/world and six captured originals required")))return Finish();
   const auto* Group=W->Node(1480);const auto* Parent=W->Node(1431);int32 Count=0;
   for(int32 I=0;I<W->PartCount();++I)if(const auto* N=W->Node(I))if(N->Name==TEXT("town-production:SM_Town_Laundry_Shell")){S.DiagnosticBenchNode=I;++Count;}
   const auto* N=W->Node(S.DiagnosticBenchNode);auto* Shell=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
   D->SetObjectField(TEXT("tb2_identity"),ComponentIdentity(W,S.DiagnosticBenchNode));D->SetObjectField(TEXT("legacy_g1074_identity"),ComponentIdentity(W,1562));D->SetObjectField(TEXT("legacy_g990_identity"),ComponentIdentity(W,1435));
   D->SetStringField(TEXT("area_at_query"),W->Area());D->SetNumberField(TEXT("named_shell_count"),Count);
   if(!Check(Count==1&&Shell&&Group&&Group->Comp.IsValid()&&Parent&&Parent->Comp.IsValid(),TEXT("unique native Shell and unchanged actual owner/root required")))return Finish();
   const FTransform Assembly(FQuat::Identity,FVector(-1500,213300,801.3043546669646));auto* Mesh=Shell->GetStaticMesh().Get();
   const bool MeshOK=Mesh&&Mesh->GetPathName()==TEXT("/Game/TownBuildings/SM_Town_Laundry_Shell.SM_Town_Laundry_Shell")&&Mesh->GetRenderData()&&Mesh->GetRenderData()->LODResources.Num()==3&&Mesh->GetNumTriangles(0)==9935&&Mesh->GetNumTriangles(1)==5463&&Mesh->GetNumTriangles(2)==2584;
   if(!Check(MeshOK&&N->Parent==1431&&Shell->GetAttachParent()==Parent->Comp.Get()&&Shell->GetRelativeTransform().Equals(Assembly,.001)&&Shell->GetComponentTransform().Equals(Assembly*Parent->Comp->GetComponentTransform(),.001)&&VisibleNow(W,Shell,S.DiagnosticBenchNode)&&ActiveRoot(W,S.DiagnosticBenchNode),TEXT("correct actual shown native Shell, canonical assembly and three unchanged LOD counts required")))return Finish();
   const FTransform Frame=Shell->GetComponentTransform();const FVector Up=Frame.TransformVectorNoScale(FVector(RZ.X,-RZ.Y,RZ.Z)).GetSafeNormal();
   if(!Check(!Up.ContainsNaN()&&Up.Z>.7,TEXT("actual assembly must preserve an upward nondegenerate repaired normal for vertical query extents")))return Finish();
   FCollisionQueryParams Q(SCENE_QUERY_STAT(S47RadioBenchPostRepair),true);Q.bReturnFaceIndex=true;TSet<UPrimitiveComponent*> Person;TArray<TSharedPtr<FJsonValue>> Ignored;
   for(const auto& P:S.Original)if(P.Component.IsValid()){Q.AddIgnoredComponent(P.Component.Get());Person.Add(P.Component.Get());Ignored.Add(MakeShared<FJsonValueString>(GetPathNameSafe(P.Component.Get())));}
   D->SetArrayField(TEXT("only_unconditionally_ignored_person_components"),Ignored);D->SetBoolField(TEXT("entire_laundry_shell_ignored"),false);const auto Candidates=Providers(W,Shell,Person);
   D->SetNumberField(TEXT("actually_registered_shown_queryable_provider_components"),Candidates.Num());D->SetArrayField(TEXT("expected_rigid_normal_world_query_only"),Vec(Up));
   TArray<TSharedPtr<FJsonValue>> Slats;int32 Slat=0,SeatSlats=0,BackSlats=0;
   for(const auto& B:Solids)if(FString(B.Name)==TEXT("Bench_seat_slat")||FString(B.Name)==TEXT("Bench_back_slat"))
   {
    auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("slat"),Slat++);Row->SetStringField(TEXT("source_solid_role"),B.Name);if(FString(B.Name)==TEXT("Bench_seat_slat"))++SeatSlats;else ++BackSlats;int32 Hits=0;TArray<TSharedPtr<FJsonValue>> Traces;
    for(double Y:{-.05,.70,1.45})
    {
     const double X=(B.Box.Min.X+B.Box.Max.X)*.5;const FVector A=WorldFromOld(Frame,FVector(X,Y,B.Box.Max.Z+.02)),Z=WorldFromOld(Frame,FVector(X,Y,B.Box.Max.Z-.05));FHitResult H;const bool Found=Shell->LineTraceComponent(H,A,Z,Q);
     const FVector Old=OldFromWorld(Frame,H.ImpactPoint);const bool Exact=Found&&H.GetComponent()==Shell&&FVector::DotProduct(H.ImpactNormal,Up)>.7&&FMath::Abs(Old.Z-B.Box.Max.Z)<=.0002&&B.Box.ExpandBy(.0002).IsInsideOrOn(Old);
     auto T=Record(W,Shell,H,Found,A,Z);T->SetBoolField(TEXT("actual_repaired_slat_top_match"),Exact);if(Found)T->SetArrayField(TEXT("hit_in_original_bench_frame_m"),Vec(Old));else T->SetField(TEXT("hit_in_original_bench_frame_m"),MakeShared<FJsonValueNull>());Traces.Add(MakeShared<FJsonValueObject>(T));if(Exact)++Hits;
    }
    Row->SetNumberField(TEXT("actual_top_hits"),Hits);Row->SetArrayField(TEXT("traces"),Traces);Slats.Add(MakeShared<FJsonValueObject>(Row));Check(Hits==3,TEXT("each repaired slat requires three actual native component top hits"));
   }
   D->SetArrayField(TEXT("native_slat_traces"),Slats);D->SetNumberField(TEXT("seat_slat_count"),SeatSlats);D->SetNumberField(TEXT("back_slat_count"),BackSlats);Check(SeatSlats==4&&BackSlats==2,TEXT("exact four seat slats and two back slats required"));
   const FVector Anchor=Group->Comp->GetComponentLocation(),Plane=WorldFromOld(Frame,FVector(.43,.70,.47));TArray<double> Heights;TArray<TSharedPtr<FJsonValue>> SeatRows;
   D->SetArrayField(TEXT("unchanged_original_anchor_ue_cm"),Vec(Anchor));
   for(double X:{-2.,0.,2.})for(double Y:{-2.,0.,2.})
   {
    const FVector At=Anchor+FVector(X,Y,0);const double QueryZ=At.Z+FVector::DotProduct(Up,Plane-At)/Up.Z;const FVector A(At.X,At.Y,QueryZ+20),Z(At.X,At.Y,QueryZ-20);FHitResult H;const bool Found=Shell->LineTraceComponent(H,A,Z,Q);
    const FVector Old=OldFromWorld(Frame,H.ImpactPoint);const bool Exact=Found&&H.GetComponent()==Shell&&FVector::DotProduct(H.ImpactNormal,Up)>.7&&OldSeat(Old);
    auto T=Record(W,Shell,H,Found,A,Z);T->SetNumberField(TEXT("source_plane_used_only_for_ray_extent_z_cm"),QueryZ);T->SetBoolField(TEXT("actual_repaired_seat_top_match"),Exact);SeatRows.Add(MakeShared<FJsonValueObject>(T));if(Exact)Heights.Add(H.ImpactPoint.Z);
   }
   D->SetArrayField(TEXT("candidate_seat_traces"),SeatRows);D->SetNumberField(TEXT("candidate_seat_actual_hits"),Heights.Num());const bool SeatOK=Check(Heights.Num()>=5,TEXT("unchanged anchorXY requires at least five of nine actual repaired seat hits"));
   if(SeatOK){Heights.Sort();S.Seat=FVector(Anchor.X,Anchor.Y,Heights[Heights.Num()/2]);D->SetArrayField(TEXT("measured_candidate_seat_ue_cm"),Vec(S.Seat));D->SetNumberField(TEXT("candidate_seat_spread_cm"),Heights.Last()-Heights[0]);Check(Heights.Last()-Heights[0]<=1.5,TEXT("measured seat spread exceeds unchanged1.5cm limit"));}
   TArray<TSharedPtr<FJsonValue>> Feet;int32 FootIndex=0;
   auto FootQuery=[&](const FVector& Old,bool Required)
   {
    const FVector A=WorldFromOld(Frame,Old-FVector(0,0,.05)),Z=WorldFromOld(Frame,Old+FVector(0,0,.05));FHitResult H;const bool Found=Shell->LineTraceComponent(H,A,Z,Q);const FVector Back=OldFromWorld(Frame,H.ImpactPoint);
    const bool Exact=Found&&H.GetComponent()==Shell&&FVector::DotProduct(H.ImpactNormal,Up)<-.7&&FMath::Abs(Back.Z)<=.0002&&BenchRole(Back).StartsWith(TEXT("bench_not_ground:"));
    auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("sample"),FootIndex++);Row->SetBoolField(TEXT("required_interior_foot_sample"),Required);Row->SetArrayField(TEXT("original_bench_frame_sample_m"),Vec(Old));Row->SetObjectField(TEXT("actual_bottom_trace"),Record(W,Shell,H,Found,A,Z));Row->SetBoolField(TEXT("actual_repaired_bottom_match"),Exact);
    if(Required)Check(Exact,TEXT("each of four actual repaired leg/foot-iron underside samples required"));
    if(Exact){bool Complete=false;Row->SetObjectField(TEXT("measured_visible_surface_probes"),Support(W,Shell,Candidates,H.ImpactPoint,Q,S.DiagnosticHitNodes,Complete,true));if(Required)Check(Complete,TEXT("identify actual visible component below bench; no g1004 identity is presumed"));}
    Feet.Add(MakeShared<FJsonValueObject>(Row));
   };
   for(double X:{.23,.61})for(double Y:{-.13,1.53})FootQuery(FVector(X,Y,0),true);
   for(const auto& P:FootGrid)FootQuery(P,false);
   D->SetArrayField(TEXT("bench_underside_and_visible_surface_traces"),Feet);D->SetStringField(TEXT("source_grid_edge_scope"),TEXT("30 source boundary/grid queries are logged, never replaced by expected hits. Four interior underside samples are required; edge misses remain explicit."));
   if(SeatOK)
   {
    // Optional explicit native query input only. It does not bind or accept contact/adoption.
    S.Asset=OptionalProjectionMesh?OptionalProjectionMesh->GetPathName():AssetPath;
    if(!OptionalProjectionMesh)FParse::Value(FCommandLine::Get(),TEXT("S47RadioWomanDiagnosticSoleAsset="),S.Asset);
    if(!Check(S.Asset.StartsWith(TEXT("/Game/MetaHumans/Mannequins/RadioWoman/"))&&!S.Asset.Contains(TEXT(".."))&&!S.Asset.Contains(TEXT(" ")),TEXT("explicit private CPU-readable sole projection asset path required")))return Finish();
    if(!S.Asset.Contains(TEXT("."))){FString Name;S.Asset.Split(TEXT("/"),nullptr,&Name,ESearchCase::CaseSensitive,ESearchDir::FromEnd);S.Asset+=TEXT(".")+Name;}
    if(OptionalProjectionMesh)S.NativeAsset.Reset(OptionalProjectionMesh);
    else if(!S.NativeAsset.IsValid()||S.NativeAsset.Get()->GetPathName()!=S.Asset)S.NativeAsset.Reset(LoadObject<UStaticMesh>(nullptr,*S.Asset,nullptr,LOAD_NoWarn|LOAD_Quiet));TArray<int32> LODs;
    D->SetStringField(TEXT("sole_projection_asset"),S.Asset);D->SetStringField(TEXT("sole_projection_role"),S.Asset.Contains(TEXT("/BakeR10/"))?TEXT("HISTORICAL_R10_GEOMETRY_PROJECTED_IN_MEASURED_POSTREPAIR_FRAME_ONLY"):TEXT("EXPLICIT_SELECTED_PRIVATE_BAKE_PROJECTED_ONLY_NOT_ADOPTED"));
    if(Check(S.NativeAsset.IsValid()&&S.NativeAsset.Get()->GetPathName()==S.Asset&&NativeReady(S.NativeAsset.Get(),LODs),TEXT("actual requested private three-LOD CPU geometry required, without adopting it")))
    {
     TArray<TSharedPtr<FJsonValue>> ActualLODs;for(int32 T:LODs)ActualLODs.Add(MakeShared<FJsonValueNumber>(T));D->SetArrayField(TEXT("actual_projection_lod_triangles"),ActualLODs);
     const auto Basis=Group->Comp->GetComponentTransform();const FVector Forward=Basis.TransformVectorNoScale(S47Three::DirToUE(FVector(0,0,-1))).GetSafeNormal(),Right=Basis.TransformVectorNoScale(S47Three::DirToUE(FVector(1,0,0))).GetSafeNormal();
     const FTransform Placement(FRotationMatrix::MakeFromXY(Forward,Right).ToQuat(),S.Seat);const auto& L=S.NativeAsset.Get()->GetRenderData()->LODResources[0];D->SetStringField(TEXT("actual_selected_sole_packed_vertex_sha1"),VertexHash(L));TArray<uint32> Indices;L.IndexBuffer.GetCopy(Indices);TSet<uint32> Picks[2];double Low[2]={1e20,1e20};bool Valid=true;
     for(const auto& Section:L.Sections)
     {
      if(!S.NativeAsset.Get()->GetStaticMaterials().IsValidIndex(Section.MaterialIndex)){Valid=false;continue;}
      const auto* Material=S.NativeAsset.Get()->GetMaterial(Section.MaterialIndex);if(!Material||(Material->GetName()!=TEXT("M_RW_Shoe")&&Material->GetName()!=TEXT("M_RW_Trim")))continue;
      for(uint32 I=Section.FirstIndex;I<Section.FirstIndex+Section.NumTriangles*3;++I){if(I>=uint32(Indices.Num())||Indices[I]>=L.VertexBuffers.PositionVertexBuffer.GetNumVertices()){Valid=false;break;}const FVector P(L.VertexBuffers.PositionVertexBuffer.VertexPosition(Indices[I]));if(P.Z>=-15)continue;const int32 Side=P.Y<0?0:1;Picks[Side].Add(Indices[I]);Low[Side]=FMath::Min(Low[Side],P.Z);}
     }
     Check(Valid,TEXT("actual sole section/index range must be valid"));TArray<TSharedPtr<FJsonValue>> SoleRows;
     if(Valid)for(int32 Side=0;Side<2;++Side)
     {
      TArray<FVector> Unique;TArray<uint32> VertexIndices;TArray<TSharedPtr<FJsonValue>> Samples;TArray<double> Gaps;
      for(uint32 I:Picks[Side]){const FVector P(L.VertexBuffers.PositionVertexBuffer.VertexPosition(I));if(P.Z<=Low[Side]+.15&&!Unique.ContainsByPredicate([&](const FVector& V){return V.Equals(P,.00001);})){Unique.Add(P);VertexIndices.Add(I);}}
      auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("side"),Side);Row->SetNumberField(TEXT("unique_lowest_sole_vertices"),Unique.Num());Check(Unique.Num()==48,TEXT("actual selected native sole geometry must contain48 unique lowest points per side"));
      for(int32 I=0;I<Unique.Num();++I)
      {
       const FVector At=Placement.TransformPosition(Unique[I]);bool Complete=false;auto P=Support(W,Shell,Candidates,At,Q,S.DiagnosticHitNodes,Complete,false);P->SetNumberField(TEXT("native_vertex_index"),VertexIndices[I]);P->SetArrayField(TEXT("native_local_cm"),Vec(Unique[I]));P->SetArrayField(TEXT("projected_world_ue_cm"),Vec(At));Check(Complete,TEXT("every sole query must identify an actual shown visible surface, never require pavement instead of road"));
       double Gap=0;if(P->TryGetNumberField(TEXT("gap_to_direct_visible_candidate_cm"),Gap))Gaps.Add(Gap);Samples.Add(MakeShared<FJsonValueObject>(P));
      }
      Row->SetArrayField(TEXT("samples"),Samples);Row->SetNumberField(TEXT("measured_visible_candidate_hits"),Gaps.Num());
      if(!Gaps.IsEmpty()){Gaps.Sort();Row->SetNumberField(TEXT("minimum_gap_cm"),Gaps[0]);Row->SetNumberField(TEXT("maximum_gap_cm"),Gaps.Last());Row->SetNumberField(TEXT("median_gap_cm"),Gaps[Gaps.Num()/2]);}
      Row->SetBoolField(TEXT("contact_accepted"),false);SoleRows.Add(MakeShared<FJsonValueObject>(Row));
     }
     D->SetArrayField(TEXT("selected_sole_projection"),SoleRows);D->SetObjectField(TEXT("measured_seat_projection_frame"),Transform(Placement));
    }
   }
   return Finish();
  }
 }


 // These helpers operate only on the six owned originals and our one adopted node.
 // No setters on owner1480/proxy1544, Shell, ground or original native assets.
 void OriginalFallback(FS47RadioWomanState& S,const FString& Why)
 {
  auto* W=S.Web.Get();if(!W)return;
  if(auto* C=S.Native.Get())
  {
   if(S.NativeNode!=INDEX_NONE)W->SetVisible(S.NativeNode,false);
   C->SetHiddenInGame(true,false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  }
  for(const auto& P:S.Original)if(auto* C=P.Component.Get())
  {
   if(C->GetStaticMesh()!=P.Mesh.Get())C->SetStaticMesh(P.Mesh.Get());
   for(int32 I=0;I<P.Materials.Num();++I)if(C->GetMaterial(I)!=P.Materials[I])C->SetMaterial(I,P.Materials[I]);
   const auto* N=W->Node(P.Id.Node);
   if(!N||!N->P.Equals(P.PositionThree,.00001)||!N->Q.Equals(P.QuaternionThree,.00001)||!N->S.Equals(P.ScaleThree,.00001)
    ||!C->GetRelativeTransform().Equals(P.Relative,.001))W->SetLocal(P.Id.Node,P.PositionThree,P.QuaternionThree,P.ScaleThree);
   if(C->GetCollisionEnabled()!=P.Collision)C->SetCollisionEnabled(P.Collision);
   if(C->bHiddenInGame!=P.Hidden)C->SetHiddenInGame(P.Hidden,false);if(C->CastShadow!=P.Shadow)C->SetCastShadow(P.Shadow);
   if(!N||N->bVis!=P.AuthoredVisible)W->SetVisible(P.Id.Node,P.AuthoredVisible);
  }
  S.Active=false;S.RadioDeltaUE=FVector::ZeroVector;S.RadioDeltaLocalThree=FVector::ZeroVector;
  S.Cut.Reset();S.Why=Why;
 }
 bool OriginalsMatchCaptured(const FS47RadioWomanState& S)
 {
  auto* W=S.Web.Get();if(!W||S.Original.Num()!=6)return false;
  for(const auto& P:S.Original)
  {
   const auto* N=W->Node(P.Id.Node);const auto* C=P.Component.Get();
   if(!N||N->Parent!=1480||N->Geo!=P.Id.Geo||N->Mats!=TArray<int32>{P.Id.Material}
    ||!N->P.Equals(P.PositionThree,.00001)||!N->Q.Equals(P.QuaternionThree,.00001)||!N->S.Equals(P.ScaleThree,.00001)
    ||N->bVis!=P.AuthoredVisible||!C||C->GetStaticMesh()!=P.Mesh.Get()||C->GetMaterials()!=P.Materials
    ||!C->GetRelativeTransform().Equals(P.Relative,.001)||C->GetCollisionEnabled()!=P.Collision
    ||C->bHiddenInGame!=P.Hidden||C->CastShadow!=P.Shadow)return false;
  }
  return true;
 }
 bool ActiveComponent(AS47Web* W,int32 Index,UStaticMeshComponent* C)
 {
  const auto* N=W?W->Node(Index):nullptr;
  return N&&C&&N->Comp.Get()==C&&W->NodeOf(C)==Index&&C->IsRegistered()
   &&W->ObjAt(Index).Shown()&&C->IsVisible()&&!C->bHiddenInGame;
 }
 bool CurrentShellStillEligible(const FS47RadioWomanState& S)
 {
  auto* W=S.Web.Get();const auto* N=W?W->Node(S.DiagnosticBenchNode):nullptr;
  auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
  const FTransform AssemblyExpected(FQuat::Identity,S47Three::ToUE(FVector(7985,8.013043546669646,2133)),FVector::OneVector);
  if(!(N&&N->Name==TEXT("town-production:SM_Town_Laundry_Shell")&&N->Parent==1431&&C&&C->GetStaticMesh()
   &&C->GetStaticMesh()->GetPathName()==TEXT("/Game/TownBuildings/SM_Town_Laundry_Shell.SM_Town_Laundry_Shell")
   &&C->GetComponentTransform().Equals(AssemblyExpected,.001)&&ActiveComponent(W,S.DiagnosticBenchNode,C)))return false;
  // These are the exact actual R2 contact providers; pavement is under detail:near1432.
  // Check their current flags/frame on every ordinary TownStep, without repeating rays.
  const int32 Nodes[]={1453,1242},Parents[]={1432,1240};
  const TCHAR* Paths[]={TEXT("/Game/TownBuildings/Trim/StaticMeshes/g1004.g1004"),TEXT("/Game/Web/Geo/geo_007/StaticMeshes/g873.g873")};
  const FTransform GroundFrame(FQuat::Identity,FVector(800000,0,0));
  for(int32 I=0;I<2;++I)
  {
   const auto* P=W->Node(Nodes[I]);const auto* Parent=W->Node(Parents[I]);auto* Surface=P?Cast<UStaticMeshComponent>(P->Comp.Get()):nullptr;
   if(!P||P->Parent!=Parents[I]||!Parent||!Parent->Comp.IsValid()||!Surface||!Surface->GetStaticMesh()
    ||Surface->GetStaticMesh()->GetPathName()!=Paths[I]||!ActiveComponent(W,Nodes[I],Surface)
    ||Surface->GetAttachParent()!=Parent->Comp.Get()||!Surface->GetRelativeTransform().Equals(FTransform::Identity,.001)
    ||!Surface->GetComponentTransform().Equals(GroundFrame,.001)||(Surface->GetOwner()&&Surface->GetOwner()->IsHidden())
    ||!CollisionEnabledHasQuery(Surface->GetCollisionEnabled())||Surface->GetCollisionResponseToChannel(ECC_Visibility)!=ECR_Block)return false;
  }
  return true;
 }
 struct FAcceptedPostrepairSite
 {
  FTransform Placement;FVector SeatUE=FVector::ZeroVector,Forward=FVector::ZeroVector,Right=FVector::ZeroVector;
  int32 SeatHits=0;TSharedPtr<FJsonObject> Diagnostics;
 };
 bool MeasureAcceptedPostrepairSite(FS47RadioWomanState& S,FAcceptedPostrepairSite& Out,FString& Why)
 {
  // All native payload/index/section/material pins come from the actual independent R12 reader.
  const TCHAR* AcceptedR12Object=TEXT("/Game/MetaHumans/Mannequins/RadioWoman/BakeR12/SM_RadioWoman_Seated.SM_RadioWoman_Seated");
  const FVector BoundMeasuredSeat(798557.,213275.,800.7192316799915); // actual postrepair R2
  const auto Reject=[&](const TCHAR* Reason){Why=Reason;return false;};
  UStaticMesh* Mesh=S.NativeAsset.Get();TArray<int32> LODs;
  if(!Mesh||Mesh->GetPathName()!=AcceptedR12Object||!NativeReady(Mesh,LODs))return Reject(TEXT("verified R12 asset not prepared"));
  if(!S47R12NativePins::Validate(Mesh,Why))return false;
  auto* W=S.Web.Get();const auto* Group=W?W->Node(1480):nullptr;
  if(!W||!Group||!Group->Comp.IsValid())return Reject(TEXT("actual original person frame missing"));
  // Runs only from the ordinary AfterTownStep hook, after visibility/schedule updates.
  // The shared sensor retains every raw hit, ignores only freshly hidden mapped blockers,
  // and leaves the entire Laundry Shell in world queries. No mesh/world setter runs here.
  if(!S47RadioBenchPost::Run(S,Mesh))return Reject(TEXT("live repaired-site measurement failed"));
  auto D=S.BenchDiagnostics;
  if(!D.IsValid()||!S47J::Bool(D,TEXT("measurement_valid"))||!S47J::Arr(D,TEXT("mismatches")).IsEmpty()
   ||S47J::Bool(D,TEXT("entire_laundry_shell_ignored"))||S47J::Int(D,TEXT("candidate_seat_actual_hits"))<5||!S.Seat.Equals(BoundMeasuredSeat,MaxSeatFrameErrorCm))return Reject(TEXT("live seat differs from measured R11 binding"));
  const auto Provider=[&](const TSharedPtr<FJsonObject>& Hit,int32 Node,const TCHAR* Path)
  {
   auto I=S47J::Obj(Hit,TEXT("surface_identity"));const auto Normal=S47J::Nums(Hit,TEXT("impact_normal_ue"));
   return Normal.Num()==3&&FMath::IsFinite(Normal[0])&&FMath::IsFinite(Normal[1])&&FMath::IsFinite(Normal[2])&&Normal[2]>.7
    &&!S47J::Bool(Hit,TEXT("start_penetrating"))&&S47J::Bool(Hit,TEXT("hit"))&&S47J::Int(I,TEXT("node"),INDEX_NONE)==Node&&S47J::Str(I,TEXT("mesh"))==Path
    &&S47J::Bool(I,TEXT("actually_shown_registered"))&&S47J::Bool(I,TEXT("active_area_root"))&&!S47J::Bool(I,TEXT("same_laundry_shell"));
  };
  const auto WorldChainComplete=[&](const TSharedPtr<FJsonObject>& Probe,bool OwnBenchMayBlock)
  {
   if(!S47J::Bool(Probe,TEXT("bounded_query_complete"))||S47J::Bool(Probe,TEXT("entire_laundry_shell_ignored")))return false;
   const auto Rows=S47J::Arr(Probe,TEXT("ordered_single_hit_queries"));if(Rows.IsEmpty()||Rows.Num()>32)return false;
   for(int32 I=0;I<Rows.Num();++I)
   {
    if(!Rows[I].IsValid()||Rows[I]->Type!=EJson::Object)return false;
    auto R=Rows[I]->AsObject(),Id=S47J::Obj(R,TEXT("surface_identity"));
    if(S47J::Bool(R,TEXT("query_local_exclusion")))
    {
     if(I==Rows.Num()-1||!S47J::Bool(R,TEXT("hit"))||S47J::Int(Id,TEXT("node"),INDEX_NONE)<0||!S47J::Bool(Id,TEXT("registered_at_measurement"))||S47J::Bool(Id,TEXT("same_laundry_shell"))||S47J::Bool(Id,TEXT("actually_shown_registered")))return false;
    }
    else if(I!=Rows.Num()-1)return false;
   }
   auto Last=Rows.Last()->AsObject(),Id=S47J::Obj(Last,TEXT("surface_identity"));
   return S47J::Bool(Last,TEXT("hit"))&&S47J::Bool(Last,TEXT("blocking_hit"))&&S47J::Bool(Id,TEXT("actually_shown_registered"))
    &&(OwnBenchMayBlock
      ? ((S47J::Bool(Id,TEXT("same_laundry_shell"))&&S47J::Int(Id,TEXT("node"),INDEX_NONE)==S.DiagnosticBenchNode&&S47J::Str(Id,TEXT("role")).StartsWith(TEXT("bench_not_ground:")))
         ||Provider(Last,1453,TEXT("/Game/TownBuildings/Trim/StaticMeshes/g1004.g1004")))
      : !S47J::Bool(Id,TEXT("same_laundry_shell")));
  };
  // Source rigid classification identifies actual four undersides. Narrow direct queries
  // prove their intersection with visible retained pavement; a world ray meets the bench
  // itself first and is never represented as unobstructed ground evidence.
  const auto Bench=S47J::Arr(D,TEXT("bench_underside_and_visible_surface_traces"));if(Bench.Num()!=34)return Reject(TEXT("complete native bench samples missing"));
  for(int32 I=0;I<4;++I)
  {
   if(!Bench[I].IsValid()||Bench[I]->Type!=EJson::Object)return Reject(TEXT("invalid bench observation row"));
   auto B=Bench[I]->AsObject(),P=S47J::Obj(B,TEXT("measured_visible_surface_probes"));
   auto Direct=S47J::Obj(S47J::Obj(P,TEXT("direct_visible_component_probes")),TEXT("highest_measured_visible_candidate"));
   auto Near=S47J::Obj(S47J::Obj(P,TEXT("direct_near_contact_component_probes_no_skipped_gap")),TEXT("highest_measured_visible_candidate"));
   double Gap=0,NearGap=0;
   if(!P.IsValid()||!S47J::Bool(B,TEXT("required_interior_foot_sample"))||!S47J::Bool(B,TEXT("actual_repaired_bottom_match"))
    ||!Provider(Direct,1453,TEXT("/Game/TownBuildings/Trim/StaticMeshes/g1004.g1004"))||!Provider(Near,1453,TEXT("/Game/TownBuildings/Trim/StaticMeshes/g1004.g1004"))
    ||!P->TryGetNumberField(TEXT("gap_to_direct_visible_candidate_cm"),Gap)||!FMath::IsFinite(Gap)||FMath::Abs(Gap)>.1
    ||!P->TryGetNumberField(TEXT("gap_to_direct_near_candidate_cm"),NearGap)||!FMath::IsFinite(NearGap)||FMath::Abs(NearGap-Gap)>.02
    ||!WorldChainComplete(S47J::Obj(P,TEXT("world_probe_with_conditional_hidden_exclusions")),true))return Reject(TEXT("native bench/pavement near contact is incomplete"));
  }
  const auto Feet=S47J::Arr(D,TEXT("selected_sole_projection"));if(Feet.Num()!=2)return Reject(TEXT("two actual R11 soles missing"));
  for(const auto& F:Feet)
  {
   if(!F.IsValid()||F->Type!=EJson::Object)return Reject(TEXT("invalid sole observation row"));
   auto Foot=F->AsObject();const auto Samples=S47J::Arr(Foot,TEXT("samples"));TSet<int32> Vertices;
   if(S47J::Int(Foot,TEXT("unique_lowest_sole_vertices"))!=48||S47J::Int(Foot,TEXT("measured_visible_candidate_hits"))!=48||Samples.Num()!=48)return Reject(TEXT("all 96 lowest actual R11 sole vertices are required"));
   for(const auto& V:Samples)
   {
    if(!V.IsValid()||V->Type!=EJson::Object)return Reject(TEXT("invalid sole sample row"));
    auto P=V->AsObject(),Raw=S47J::Obj(P,TEXT("world_probe_with_conditional_hidden_exclusions"));
    auto Direct=S47J::Obj(S47J::Obj(P,TEXT("direct_visible_component_probes")),TEXT("highest_measured_visible_candidate"));
    double Gap=0,WorldGap=0;const int32 Vertex=S47J::Int(P,TEXT("native_vertex_index"),INDEX_NONE);
    if(Vertex<0||Vertices.Contains(Vertex)||!S47J::Bool(P,TEXT("visible_surface_identified"))||!S47J::Bool(P,TEXT("world_direct_visible_provider_agree"))
     ||!Provider(Direct,1242,TEXT("/Game/Web/Geo/geo_007/StaticMeshes/g873.g873"))||!WorldChainComplete(Raw,false)
     ||!P->TryGetNumberField(TEXT("gap_to_direct_visible_candidate_cm"),Gap)||!FMath::IsFinite(Gap)||FMath::Abs(Gap)>MaxSoleGapCm
     ||!P->TryGetNumberField(TEXT("gap_to_filtered_world_candidate_cm"),WorldGap)||!FMath::IsFinite(WorldGap)||FMath::Abs(WorldGap-Gap)>.02)return Reject(TEXT("actual visible R11 shoe support/contact rejected"));
    Vertices.Add(Vertex);
   }
  }
  const FTransform Basis=Group->Comp->GetComponentTransform();
  Out.Forward=Basis.TransformVectorNoScale(S47Three::DirToUE(FVector(0,0,-1))).GetSafeNormal();
  Out.Right=Basis.TransformVectorNoScale(S47Three::DirToUE(FVector(1,0,0))).GetSafeNormal();
  Out.SeatUE=S.Seat;Out.SeatHits=S47J::Int(D,TEXT("candidate_seat_actual_hits"));Out.Placement=FTransform(FRotationMatrix::MakeFromXY(Out.Forward,Out.Right).ToQuat(),Out.SeatUE);Out.Diagnostics=D;
  D->SetNumberField(TEXT("area_epoch"),double(S.AreaEpoch));D->SetNumberField(TEXT("town_step_serial"),double(S.TownStepSerial));
  D->SetBoolField(TEXT("measured_seat_binding_valid"),true);D->SetBoolField(TEXT("all_lowest_soles_within_contact_limit"),true);
  D->SetStringField(TEXT("status"),TEXT("LIVE_R12_SITE_CONTACT_ACCEPTED_BEFORE_SWAP"));D->SetBoolField(TEXT("contact_accepted"),true);
  Why=TEXT("verified R11 pose and current visible native contacts");return true;
 }
 bool ApplyRadioTranslation(FS47RadioWomanState& S,const FVector& DeltaUE)
 {
  auto* W=S.Web.Get();const auto* Owner=W?W->Node(1480):nullptr;if(!Owner||!Owner->Comp.IsValid()||S.Original.Num()!=6)return false;
  const FTransform Frame=Owner->Comp->GetComponentTransform();
  const FVector LocalThree=S47Three::ToThree(Frame.InverseTransformVector(DeltaUE));
  // Absolute baseline + one common delta. Never AddOffset and never capture a shifted baseline.
  for(int32 I:{3,4})W->SetPosition(S.Original[I].Id.Node,S.Original[I].PositionThree+LocalThree);
  for(int32 I:{3,4})
  {
   const auto& P=S.Original[I];const auto* N=W->Node(P.Id.Node);const auto* C=P.Component.Get();
   const FVector BaselineWorld=Frame.TransformPosition(P.Relative.GetTranslation());
   if(!N||!C||!N->P.Equals(P.PositionThree+LocalThree,.00001)||!N->Q.Equals(P.QuaternionThree,.00001)||!N->S.Equals(P.ScaleThree,.00001)
    ||!C->GetRelativeTransform().GetRotation().Equals(P.Relative.GetRotation(),.00001)
    ||!C->GetRelativeScale3D().Equals(P.Relative.GetScale3D(),.00001)
    ||!C->GetComponentLocation().Equals(BaselineWorld+DeltaUE,.02)||C->GetMaterials()!=P.Materials)return false;
  }
  S.RadioDeltaUE=DeltaUE;S.RadioDeltaLocalThree=LocalThree;return true;
 }

 bool DiagnoseTB2Bench(FS47RadioWomanState& S)
 {
  if(S.BenchDiagnosticPostRepair)return S47RadioBenchPost::Run(S);
  S.BenchDiagnostics=MakeShared<FJsonObject>();auto& D=S.BenchDiagnostics;auto* W=S.Web.Get();S.DiagnosticHitNodes.Reset();
  D->SetNumberField(TEXT("schema_version"),1);D->SetBoolField(TEXT("read_only"),true);D->SetBoolField(TEXT("adoption_attempted"),false);D->SetBoolField(TEXT("runtime_accepted"),false);
  D->SetStringField(TEXT("source_geometry_phase"),TEXT("PRE_REPAIR_NATIVE_BASELINE; repaired Bench_* geometry must be independently calibrated and will fail these old exact bounds"));
  D->SetNumberField(TEXT("readback_serial"),++S.DiagnosticReadbacks);D->SetNumberField(TEXT("query_monotonic_seconds"),FPlatformTime::Seconds());
  D->SetStringField(TEXT("scope"),TEXT("Fresh explicit Readback queries only. Native TB2 bench/foot undersides and world support surfaces; unchanged R10 sole geometry is only projected, never adopted. No RadioCut, setters, save, bake or support acceptance."));
  TArray<TSharedPtr<FJsonValue>> Errors;bool Good=true;
  auto Check=[&](bool Pass,const TCHAR* Why){if(!Pass){Good=false;Errors.Add(MakeShared<FJsonValueString>(Why));}return Pass;};
  auto Finish=[&](){D->SetArrayField(TEXT("mismatches"),Errors);D->SetBoolField(TEXT("measurement_valid"),Good);D->SetStringField(TEXT("status"),Good?TEXT("MEASURED_NO_SUPPORT_ACCEPTANCE"):TEXT("REJECTED_MEASUREMENT"));S.Why=Good?TEXT("read-only native TB2 measurements; no adoption/support acceptance"):TEXT("read-only native TB2 measurement rejected; see mismatches");return Good;};
  D->SetNumberField(TEXT("captured_original_person_components"),S.Original.Num());
  if(!Check(W&&W->GetWorld()&&S.Original.Num()==6,TEXT("actual Web/world and all six unchanged original person bindings required")))return Finish();
  D->SetStringField(TEXT("area_at_query"),W->Area());D->SetObjectField(TEXT("legacy_g1074_identity"),ComponentIdentity(W,1562));
  const auto* Group=W->Node(1480);const auto* Parent=W->Node(1431);TArray<int32> Shells;
  for(int32 I=0;I<W->PartCount();++I){const auto* N=W->Node(I);if(N&&N->Name==TEXT("town-production:SM_Town_Laundry_Shell"))Shells.Add(I);}
  D->SetNumberField(TEXT("named_shell_count"),Shells.Num());S.DiagnosticBenchNode=Shells.IsEmpty()?INDEX_NONE:Shells[0];
  D->SetObjectField(TEXT("tb2_identity"),ComponentIdentity(W,S.DiagnosticBenchNode));
  const auto* ShellNode=W->Node(S.DiagnosticBenchNode);auto* Shell=ShellNode?Cast<UStaticMeshComponent>(ShellNode->Comp.Get()):nullptr;
  if(!Check(Shells.Num()==1&&Shell&&Group&&Group->Comp.IsValid()&&Parent&&Parent->Comp.IsValid(),TEXT("exactly one named TB2 Shell and original person/town root required")))return Finish();
  const FTransform ExpectedRelative(FQuat::Identity,FVector(-1500,213300,801.3043546669646));
  const FTransform ExpectedWorld=ExpectedRelative*Parent->Comp->GetComponentTransform();D->SetObjectField(TEXT("expected_assembly_relative_transform"),Transform(ExpectedRelative));D->SetObjectField(TEXT("expected_assembly_world_transform"),Transform(ExpectedWorld));
  D->SetArrayField(TEXT("assembly_site_three_m"),Vec(FVector(7985,8.013043546669646,2133)));
  auto* Mesh=Shell->GetStaticMesh().Get();const bool MeshOK=Mesh&&Mesh->GetPathName()==TEXT("/Game/TownBuildings/SM_Town_Laundry_Shell.SM_Town_Laundry_Shell")&&Mesh->GetRenderData()&&Mesh->GetRenderData()->LODResources.Num()==3&&Mesh->GetNumTriangles(0)==9935&&Mesh->GetNumTriangles(1)==5463&&Mesh->GetNumTriangles(2)==2584;
  TArray<TSharedPtr<FJsonValue>> ActualLODs;if(Mesh&&Mesh->GetRenderData())for(const auto& L:Mesh->GetRenderData()->LODResources)ActualLODs.Add(MakeShared<FJsonValueNumber>(L.GetNumTriangles()));D->SetArrayField(TEXT("actual_shell_lod_triangles"),ActualLODs);
  if(!Check(MeshOK&&ShellNode->Parent==1431&&Shell->GetAttachParent()==Parent->Comp.Get()&&Shell->GetRelativeTransform().Equals(ExpectedRelative,.001)&&Shell->GetComponentTransform().Equals(ExpectedWorld,.001),TEXT("exact native Shell path/three LODs/parent and canonical assembly transform mismatch")))return Finish();
  D->SetStringField(TEXT("reference_source_shell_glb_sha256"),TEXT("3c42f6ff154b09357c323500e6cc575cadfe71d55ffd6d58680abbe1b35f0be9"));
  const FTransform Frame=Shell->GetComponentTransform();FCollisionQueryParams Query(SCENE_QUERY_STAT(S47RadioWomanTB2Diagnostic),true);Query.bReturnFaceIndex=true;
  TArray<TSharedPtr<FJsonValue>> Ignored;for(const auto& P:S.Original)if(P.Component.IsValid()){Query.AddIgnoredComponent(P.Component.Get());Ignored.Add(MakeShared<FJsonValueString>(GetPathNameSafe(P.Component.Get())));}
  D->SetArrayField(TEXT("ignored_person_components"),Ignored);D->SetBoolField(TEXT("entire_laundry_shell_ignored"),false);
  const double Starts[]={26,36,46,56};TArray<TSharedPtr<FJsonValue>> Slats;
  for(int32 Slat=0;Slat<4;++Slat)
  {
   auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("slat"),Slat);Row->SetArrayField(TEXT("source_local_min_cm"),Vec(FVector(Starts[Slat],-165,44)));Row->SetArrayField(TEXT("source_local_max_cm"),Vec(FVector(Starts[Slat]+7.5,25,47)));
   int32 Hits=0;TArray<TSharedPtr<FJsonValue>> Traces;
   for(double Y:{-145.,-70.,5.})
   {
    const FVector Start=Frame.TransformPosition(FVector(Starts[Slat]+3.75,Y,65)),End=Frame.TransformPosition(FVector(Starts[Slat]+3.75,Y,35));FHitResult Hit;
    const bool Found=Shell->LineTraceComponent(Hit,Start,End,Query);const FVector Local=Frame.InverseTransformPosition(Hit.ImpactPoint);
    const bool Exact=Found&&Hit.GetComponent()==Shell&&Hit.ImpactNormal.Z>.7&&Local.X>=Starts[Slat]-.02&&Local.X<=Starts[Slat]+7.5+.02&&Local.Y>=-165-.02&&Local.Y<=25+.02&&FMath::Abs(Local.Z-47)<=.02;
    auto Trace=TraceData(W,Shell,Hit,Found,Start,End);Trace->SetBoolField(TEXT("matches_actual_slat_top"),Exact);Traces.Add(MakeShared<FJsonValueObject>(Trace));if(Exact)++Hits;
   }
   Row->SetNumberField(TEXT("actual_top_hits"),Hits);Row->SetArrayField(TEXT("traces"),Traces);Slats.Add(MakeShared<FJsonValueObject>(Row));Check(Hits==3,TEXT("physical native component traces must hit each authored slat top, not source-height substitution"));
  }
  D->SetArrayField(TEXT("native_slat_traces"),Slats);
  const FVector Anchor=Group->Comp->GetComponentLocation(),AnchorLocal=Frame.InverseTransformPosition(Anchor);D->SetArrayField(TEXT("original_anchor_ue_cm"),Vec(Anchor));
  TArray<double> Heights;TArray<TSharedPtr<FJsonValue>> SeatTraces;
  for(double X:{-2.,0.,2.})for(double Y:{-2.,0.,2.})
  {
   const FVector Start=Frame.TransformPosition(FVector(AnchorLocal.X+X,AnchorLocal.Y+Y,65)),End=Frame.TransformPosition(FVector(AnchorLocal.X+X,AnchorLocal.Y+Y,35));FHitResult Hit;
   const bool Found=Shell->LineTraceComponent(Hit,Start,End,Query);const FVector Local=Frame.InverseTransformPosition(Hit.ImpactPoint);bool InSlat=false;
   for(double X0:Starts)InSlat|=Local.X>=X0-.02&&Local.X<=X0+7.5+.02;
   const bool Exact=Found&&Hit.GetComponent()==Shell&&Hit.ImpactNormal.Z>.7&&InSlat&&Local.Y>=-165-.02&&Local.Y<=25+.02&&FMath::Abs(Local.Z-47)<=.02;
   auto Trace=TraceData(W,Shell,Hit,Found,Start,End);Trace->SetBoolField(TEXT("accepted_native_slat_top"),Exact);SeatTraces.Add(MakeShared<FJsonValueObject>(Trace));if(Exact)Heights.Add(Hit.ImpactPoint.Z);
  }
  D->SetArrayField(TEXT("candidate_seat_traces"),SeatTraces);D->SetNumberField(TEXT("candidate_seat_actual_hits"),Heights.Num());
  const bool SeatOK=Check(Heights.Num()>=5,TEXT("candidate frame requires five of nine actual native slat hits"));
  if(SeatOK){Heights.Sort();S.Seat=FVector(Anchor.X,Anchor.Y,Heights[Heights.Num()/2]);D->SetArrayField(TEXT("measured_candidate_seat_ue_cm"),Vec(S.Seat));D->SetNumberField(TEXT("candidate_seat_spread_cm"),Heights.Last()-Heights[0]);Check(Heights.Last()-Heights[0]<=1.5,TEXT("actual native candidate seat spread exceeds1.5cm"));}
  TArray<TSharedPtr<FJsonValue>> Foundations;
  for(double X:{23.,61.})for(double Y:{-153.,13.})
  {
   const FVector Start=Frame.TransformPosition(FVector(X,Y,-5)),End=Frame.TransformPosition(FVector(X,Y,5));FHitResult Bottom;
   const bool Found=Shell->LineTraceComponent(Bottom,Start,End,Query);const FVector Local=Frame.InverseTransformPosition(Bottom.ImpactPoint);
   const bool Exact=Found&&Bottom.GetComponent()==Shell&&Bottom.ImpactNormal.Z<-.7&&FMath::Abs(Local.Z)<=.02;
   auto Row=MakeShared<FJsonObject>();Row->SetArrayField(TEXT("source_foot_sample_local_cm"),Vec(FVector(X,Y,0)));Row->SetObjectField(TEXT("actual_foot_bottom_trace"),TraceData(W,Shell,Bottom,Found,Start,End));Row->SetBoolField(TEXT("actual_bottom_matches"),Exact);
   if(Check(Exact,TEXT("native underside trace must measure actual bench foot bottom at source-localZ0")))
   {
    const FVector GroundStart=Bottom.ImpactPoint-FVector(0,0,.1),GroundEnd=Bottom.ImpactPoint-FVector(0,0,150);FHitResult Support;
    const bool Hit=W->GetWorld()->LineTraceSingleByChannel(Support,GroundStart,GroundEnd,ECC_Visibility,Query);
    Row->SetObjectField(TEXT("world_surface_below_bottom"),TraceData(W,Shell,Support,Hit,GroundStart,GroundEnd));Row->SetNumberField(TEXT("unresolved_near_contact_interval_cm"),.1);
    Row->SetStringField(TEXT("support_scope"),TEXT("First real world surface0.1..150cm below actual foot underside; Laundry Shell is not ignored. Own bench geometry is not accepted as ground. Contact inside the skipped0.1cm interval remains unresolved."));
    if(Hit){Row->SetNumberField(TEXT("measured_bottom_above_surface_cm"),Bottom.ImpactPoint.Z-Support.ImpactPoint.Z);S.DiagnosticHitNodes.Add(W->NodeOf(Support.GetComponent()));}
    Check(Hit&&Support.ImpactNormal.Z>.7&&Support.GetComponent()!=Shell,TEXT("world surface below native bench foot must be measured and distinguished from bench itself"));
   }
   Foundations.Add(MakeShared<FJsonValueObject>(Row));
  }
  D->SetArrayField(TEXT("bench_foot_support_traces"),Foundations);
  if(SeatOK)
  {
   S.Asset=TEXT("/Game/MetaHumans/Mannequins/RadioWoman/BakeR10/SM_RadioWoman_Seated.SM_RadioWoman_Seated");if(!S.NativeAsset.IsValid())S.NativeAsset.Reset(LoadObject<UStaticMesh>(nullptr,*S.Asset,nullptr,LOAD_NoWarn|LOAD_Quiet));
   TArray<int32> LODs;if(Check(NativeReady(S.NativeAsset.Get(),LODs),TEXT("accepted private R10 CPU-readable three-LOD geometry required for actual sole projection")))
   {
    const FTransform Basis=Group->Comp->GetComponentTransform();const FVector Forward=Basis.TransformVectorNoScale(S47Three::DirToUE(FVector(0,0,-1))).GetSafeNormal(),Right=Basis.TransformVectorNoScale(S47Three::DirToUE(FVector(1,0,0))).GetSafeNormal();
    const FTransform Projection(FRotationMatrix::MakeFromXY(Forward,Right).ToQuat(),S.Seat);auto Soles=SoleGroundDiagnostics(W,S.NativeAsset.Get(),Projection,Query,Shell);D->SetObjectField(TEXT("R10_sole_projection"),Soles);
    const TArray<TSharedPtr<FJsonValue>>* Feet=nullptr;bool Complete=Soles->TryGetArrayField(TEXT("feet"),Feet)&&Feet->Num()==2;
    if(Complete)for(const auto& F:*Feet){const auto Foot=F->AsObject();Complete&=Foot->GetIntegerField(TEXT("unique_lowest_sole_vertices"))==48&&Foot->GetIntegerField(TEXT("ground_hits"))==48;for(const auto& V:Foot->GetArrayField(TEXT("samples")))S.DiagnosticHitNodes.Add(V->AsObject()->GetIntegerField(TEXT("hit_node")));}
    Check(Complete,TEXT("actual unchanged R10 projection requires all48 unique sole samples per foot and96 world hits; this does not assert contact"));
   }
  }
  return Finish();
 }
}

TSharedPtr<FS47RadioWoman> FS47RadioWoman::Build(AS47Web* W)
{
 auto Result=MakeShared<FS47RadioWoman>();Result->State=MakeShared<FS47RadioWomanState>();auto& S=*Result->State;S.Web=W;
 const bool PreRepairRequested=FParse::Param(FCommandLine::Get(),TEXT("S47RadioWomanBenchDiagnostic"));
 S.BenchDiagnosticPostRepair=FParse::Param(FCommandLine::Get(),TEXT("S47RadioWomanBenchPostRepairDiagnostic"));
 S.BenchDiagnosticRequested=PreRepairRequested||S.BenchDiagnosticPostRepair;S.BenchDiagnosticConflict=PreRepairRequested&&S.BenchDiagnosticPostRepair;
 auto& D=S.BenchDiagnostics;D->SetBoolField(TEXT("trace_attempted"),false);D->SetNumberField(TEXT("anchor_node"),1480);D->SetNumberField(TEXT("bench_node"),1562);
 D->SetNumberField(TEXT("required_surface_hits"),5);D->SetNumberField(TEXT("planned_samples"),9);D->SetNumberField(TEXT("expected_seat_from_legacy_anchor_cm"),MeasuredSeatFromLegacyCm);D->SetNumberField(TEXT("maximum_measured_seat_frame_error_cm"),MaxSeatFrameErrorCm);D->SetNumberField(TEXT("maximum_spread_cm"),1.5);
 auto Reject=[&](const TCHAR* Why){S.Why=Why;UE_LOG(LogS47RadioWoman,Warning,TEXT("S47: radio woman original fallback: %s; original group/radio/proxy retained"),Why);return Result;};
 if(!W)return Reject(TEXT("missing web"));
 const auto* Group=W->Node(1480);const auto* Proxy=W->Node(1544);
 if(Group&&Group->Comp.IsValid())
 {
  D->SetArrayField(TEXT("anchor_ue_cm"),Vec(Group->Comp->GetComponentLocation()));D->SetObjectField(TEXT("anchor_transform"),Transform(Group->Comp->GetComponentTransform()));
  D->SetStringField(TEXT("anchor_component"),GetPathNameSafe(Group->Comp.Get()));D->SetNumberField(TEXT("anchor_parent"),Group->Parent);
 }
 if(W->Handle(TEXT("world.town.woman"))!=1480||W->Handle(TEXT("world.town.proxies.woman"))!=1544||!Group||!Proxy||Group->Parent!=1431||Proxy->Parent!=1431
  ||!Group->Comp.IsValid()||!Proxy->Comp.IsValid()||!Group->P.Equals(FVector(-14.43,7.926916740199588,2132.75),.00001)
  ||!Group->S.Equals(FVector::OneVector,.00001)||!Group->Q.Equals(S47Three::Euler(0,-UE_DOUBLE_PI/2,0,TEXT("XYZ")),.00001)||Group->Children.Num()!=6)return Reject(TEXT("exact person/proxy/seat/basis"));
 for(const auto& E:Expected)
 {
  const auto* N=W->Node(E.Node);auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;UStaticMesh* M=C?C->GetStaticMesh().Get():nullptr;
  const FString Path=FString::Printf(TEXT("/Game/Web/Geo/geo_008/StaticMeshes/g%d.g%d"),E.Geo,E.Geo);
  if(!N||N->Parent!=1480||N->Geo!=E.Geo||N->Mats!=TArray<int32>{E.Material}||N->bInstanced||!N->bVis||!C||!M||M!=W->GeoMesh(E.Geo)||M->GetPathName()!=Path
   ||!M->GetRenderData()||M->GetRenderData()->LODResources.Num()!=1||M->GetNumTriangles(0)!=E.Triangles||!C->GetRelativeTransform().Equals(FTransform::Identity,.001)
   ||M->GetStaticMaterials().Num()!=1||C->GetNumMaterials()!=1||!C->GetMaterial(0)||!W->MatAsset(E.Material)||C->GetMaterial(0)->GetBaseMaterial()!=W->MatAsset(E.Material)->GetBaseMaterial())return Reject(TEXT("six exact original mesh/material/transform bindings"));
  FOriginal P;P.Id=E;P.Component=C;P.Mesh.Reset(M);P.Materials=C->GetMaterials();P.Relative=C->GetRelativeTransform();P.Collision=C->GetCollisionEnabled();P.Hidden=C->bHiddenInGame;P.Shadow=C->CastShadow;P.AuthoredVisible=N->bVis;
  P.PositionThree=N->P;P.QuaternionThree=N->Q;P.ScaleThree=N->S;S.Original.Add(MoveTemp(P));
 }
 if(S.BenchDiagnosticRequested){S.Why=TEXT("explicit read-only TB2 bench diagnosis; measurements run fresh on Readback, original figure/radio retained");return Result;}
 if(FParse::Param(FCommandLine::Get(),TEXT("S47RadioWomanOff"))||FParse::Param(FCommandLine::Get(),TEXT("S47PropsOff")))return Reject(TEXT("disabled by startup flag"));
 S.Asset=AssetPath;FParse::Value(FCommandLine::Get(),TEXT("S47RadioWomanAsset="),S.Asset);
 if(!S.Asset.StartsWith(TEXT("/Game/MetaHumans/Mannequins/RadioWoman/"))||S.Asset.Contains(TEXT(".."))||S.Asset.Contains(TEXT(" ")))return Reject(TEXT("private candidate asset path"));
 if(!S.Asset.Contains(TEXT("."))){FString Name;S.Asset.Split(TEXT("/"),nullptr,&Name,ESearchCase::CaseSensitive,ESearchDir::FromEnd);S.Asset+=TEXT(".")+Name;}
 S.NativeAsset.Reset(LoadObject<UStaticMesh>(nullptr,*S.Asset,nullptr,LOAD_NoWarn|LOAD_Quiet));
 if(!NativeReady(S.NativeAsset.Get(),S.LODs))return Reject(TEXT("complete private static three-LOD seated candidate unavailable or invalid"));
 S.Prepared=true;S.Pending=true;
 S.Why=TEXT("prepared original bindings and private candidate; wait ordinary Enter/TownStep and accepted postrepair contact binding");
 return Result;
}


void FS47RadioWoman::InvalidateAreaEpoch()
{
 if(!State.IsValid()||State->BenchDiagnosticRequested)return;auto& S=*State;++S.AreaEpoch;
 if(S.Active)OriginalFallback(S,TEXT("area epoch invalidated; originals restored pending real TownStep"));
 S.Pending=S.Prepared;S.AttemptedEpoch=-1;
}
void FS47RadioWoman::AfterTownStep()
{
 if(!State.IsValid())return;auto& S=*State;if(S.BenchDiagnosticRequested||!S.Prepared)return;
 ++S.TownStepSerial;auto* W=S.Web.Get();auto* M=W?US47Main::Get(W):nullptr;
 const bool Ready=W&&M&&M->Flow&&M->World&&S47Startup::Ready()&&!S47Startup::Blocking()
  &&M->Mode==US47Main::EMode::Play&&!M->Flow->bStarting&&!M->Flow->bRestoringNow&&!M->World->bBusy
  &&W->Area()==M->World->Area&&(W->Area()==TEXT("road")||W->Area()==TEXT("diner"));
 if(!Ready)
 {
  if(S.Active){OriginalFallback(S,TEXT("live context no longer eligible; complete originals restored"));S.Pending=true;S.AttemptedEpoch=-1;}
  return;
 }
 if(S.Active&&S.MeasuredEpoch==S.AreaEpoch)
 {
  if(!CurrentShellStillEligible(S)){OriginalFallback(S,TEXT("measured native support identity no longer active/shown"));S.Pending=true;S.AttemptedEpoch=-1;}
  return;
 }
 if(!S.Pending||S.AttemptedEpoch==int64(S.AreaEpoch))return;
 // Schedule has already run. Hidden owner/bench means wait; hidden collision is not support.
 if(!W->ObjAt(1480).Shown()||!W->ObjAt(1544).Shown())return;
 if(!OriginalsMatchCaptured(S)){OriginalFallback(S,TEXT("captured originals differ before live preflight"));S.Pending=false;return;}
 FAcceptedPostrepairSite Site;FString Why;S.AttemptedEpoch=int64(S.AreaEpoch);
 if(!MeasureAcceptedPostrepairSite(S,Site,Why)){OriginalFallback(S,Why);return;}
 // The binding implementation must supply actual traces and exact native/bake identities.
 bool SiteBound=false,SolesBound=false;
 if(!Site.Diagnostics.IsValid()||Site.SeatHits<5||!Site.Diagnostics->TryGetBoolField(TEXT("measured_seat_binding_valid"),SiteBound)||!SiteBound
  ||!Site.Diagnostics->TryGetBoolField(TEXT("all_lowest_soles_within_contact_limit"),SolesBound)||!SolesBound
  ||Site.SeatUE.ContainsNaN()||!Site.Placement.IsValid()||!Site.Placement.GetLocation().Equals(Site.SeatUE,.00001)
  ||!Site.Placement.GetScale3D().Equals(FVector::OneVector,.00001)
  ||!Site.Placement.GetRotation().Equals(FQuat::Identity,.00001)
  ||!Site.Forward.Equals(FVector::ForwardVector,.00001)||!Site.Right.Equals(FVector::RightVector,.00001))
 {OriginalFallback(S,TEXT("complete measured live site contract missing"));return;}
 S.Seat=Site.SeatUE;S.SeatHits=Site.SeatHits;S.Forward=Site.Forward;S.Right=Site.Right;S.Placement=Site.Placement;S.BenchDiagnostics=Site.Diagnostics;
 const auto* Group=W->Node(1480);if(!Group||!Group->Comp.IsValid())return;
 // Historical R10 frame is a reference for preserved torso/radio relation, never new support.
 const FVector ReferenceR10=Group->Comp->GetComponentLocation()+FVector(0,0,MeasuredSeatFromLegacyCm);
 const FVector RadioDelta=S.Seat-ReferenceR10;
 // The transient partition is created only now and bound this frame: no weak ref across waiting frames.
 UStaticMesh* Cut=RadioCut(W,S.Original[3].Mesh.Get(),S.FullIndices,S.RadioIndices,S.Packed);
 if(!Cut){OriginalFallback(S,TEXT("exact CPU radio partition failed after live preflight"));return;}
 Cut->InitResources();if(!Cut->GetRenderData()->IsInitialized()){OriginalFallback(S,TEXT("radio resources not initialized before binding"));return;}
 auto* C=S.Native.Get();
 if(!C)
 {
  C=NewObject<UStaticMeshComponent>(W,TEXT("RadioWomanStatic"));C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(S.NativeAsset.Get());
  C->SetupAttachment(Group->Comp.Get());C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);
  C->SetCastShadow(true);C->SetHiddenInGame(true,false);C->SetComponentTickEnabled(false);W->AddInstanceComponent(C);C->RegisterComponent();
  S.NativeNode=W->AdoptNode(C,1480,TEXT("mannequin:radio-woman"));S.Native=C;
  if(S.NativeNode==INDEX_NONE){C->DestroyComponent();OriginalFallback(S,TEXT("single native visibility adoption failed"));return;}
 }
 const auto* NN=W->Node(S.NativeNode);
 if(!NN||NN->Comp.Get()!=C||NN->Parent!=1480||NN->Name!=TEXT("mannequin:radio-woman")||C->GetStaticMesh()!=S.NativeAsset.Get())
 {OriginalFallback(S,TEXT("owned native node identity changed"));return;}
 // Flat world presentation basis is already required above. Keep native metadata and component together.
 const auto Owner=W->ObjAt(1480);
 W->SetLocal(S.NativeNode,Owner.WorldToLocal(S47Three::ToThree(S.Seat)),Owner.WorldQuaternion().Inverse(),FVector::OneVector);
 if(!C->GetComponentTransform().Equals(S.Placement,.02)||!ApplyRadioTranslation(S,RadioDelta))
 {OriginalFallback(S,TEXT("absolute native/radio transform transaction failed"));return;}
 auto* Housing=S.Original[3].Component.Get();Housing->SetStaticMesh(Cut);S.Cut=Cut;
 for(int32 I=0;I<S.Original[3].Materials.Num();++I)Housing->SetMaterial(I,S.Original[3].Materials[I]);Housing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 for(int32 I:{0,1,2,5}){auto* Old=S.Original[I].Component.Get();W->SetVisible(S.Original[I].Id.Node,false);Old->SetHiddenInGame(true,false);Old->SetCastShadow(false);Old->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
 W->SetVisible(S.NativeNode,true);C->SetHiddenInGame(false,false);S.Active=true;S.Pending=false;S.MeasuredEpoch=S.AreaEpoch;S.MeasuredTownStepSerial=S.TownStepSerial;
 S.Why=TEXT("live TownStep committed one native body and common absolute radio translation from captured baseline");
}

bool FS47RadioWoman::IsActive() const{return State.IsValid()&&State->Active;}
bool FS47RadioWoman::Audit(TArray<FString>& Lines) const
{
 bool Good=true;auto Check=[&](bool Pass,const TCHAR* Why){Lines.Add(FString::Printf(TEXT("%s radio woman %s"),Pass?TEXT("OK"):TEXT("FAIL"),Why));Good&=Pass;};
 Check(IsActive(),State.IsValid()?*State->Why:TEXT("missing install state"));if(!IsActive())return false;
 const auto& S=*State;auto* W=S.Web.Get();auto* C=S.Native.Get();Check(W&&C&&C->IsRegistered()&&C->GetStaticMesh()==S.NativeAsset.Get(),TEXT("one actual static mesh registered"));if(!W||!C)return false;
 const auto* Adopted=W->Node(S.NativeNode);Check(Adopted&&Adopted->Parent==1480&&Adopted->Name==TEXT("mannequin:radio-woman")&&W->NodeOf(C)==S.NativeNode,TEXT("native belongs to original person visibility owner"));
 Check(C->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!C->IsComponentTickEnabled()&&C->GetComponentTransform().Equals(S.Placement,.02),TEXT("static-only NoCollision and measured placement retained"));
 Check(S.MeasuredEpoch==S.AreaEpoch&&S.MeasuredTownStepSerial>0,TEXT("measurement belongs to current Enter epoch after real TownUpdate"));
  Check(S.BenchDiagnostics->GetBoolField(TEXT("measured_seat_binding_valid"))&&S.BenchDiagnostics->GetBoolField(TEXT("all_lowest_soles_within_contact_limit")),TEXT("measured bench binding and every lowest sole passed physical pre-adoption contact"));
 Check(W->Handle(TEXT("world.town.woman"))==1480&&W->Handle(TEXT("world.town.proxies.woman"))==1544&&W->IsVisible(1480)==W->IsVisible(1544),TEXT("original person/proxy ownership and schedule agree"));
 for(int32 I=0;I<S.Original.Num();++I)
 {
  const auto& P=S.Original[I];const auto* N=W->Node(P.Id.Node);auto* Old=P.Component.Get();const bool Mixed=I==3,Panel=I==4;
  FTransform ExpectedRelative=P.Relative;if(Mixed||Panel)ExpectedRelative.AddToTranslation(S47Three::ToUE(S.RadioDeltaLocalThree));
   const FVector ExpectedPosition=P.PositionThree+((Mixed||Panel)?S.RadioDeltaLocalThree:FVector::ZeroVector);
   Check(N&&N->Parent==1480&&N->Geo==P.Id.Geo&&N->Mats==TArray<int32>{P.Id.Material}&&N->P.Equals(ExpectedPosition,.00001)&&N->Q.Equals(P.QuaternionThree,.00001)&&N->S.Equals(P.ScaleThree,.00001)&&Old&&Old->GetRelativeTransform().Equals(ExpectedRelative,.001),TEXT("all original bindings retained; only both radio translations share one captured-baseline delta"));
  Check(Old&&Old->GetMaterials()==P.Materials,TEXT("actual original material pointers retained"));
  if(Panel)Check(Old&&Old->GetStaticMesh()==P.Mesh.Get()&&Old->GetCollisionEnabled()==P.Collision&&Old->bHiddenInGame==P.Hidden&&Old->CastShadow==P.Shadow&&N&&N->bVis==P.AuthoredVisible,TEXT("complete original metal radio panel retained"));
  else if(Mixed)Check(Old&&Old->GetStaticMesh()==S.Cut.Get()&&Old->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&N&&N->bVis&&Old->IsVisible()==C->IsVisible(),TEXT("radio housing retained without invisible shoe collision"));
  else Check(Old&&Old->GetStaticMesh()==P.Mesh.Get()&&Old->bHiddenInGame&&!Old->CastShadow&&Old->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&N&&!N->bVis,TEXT("only obsolete body leaf retired"));
 }
 auto* Cut=S.Cut.Get();TArray<uint32> Full,Radio;
 if(Cut&&Cut->GetRenderData())Cut->GetRenderData()->LODResources[0].IndexBuffer.GetCopy(Radio);
 const auto* Original=S.Original[3].Mesh.Get();if(Original&&Original->GetRenderData())Original->GetRenderData()->LODResources[0].IndexBuffer.GetCopy(Full);
 Check(Cut&&Original&&Cut->GetRenderData()&&Original->GetRenderData()&&Radio==S.RadioIndices&&Full==S.FullIndices&&VertexHash(Cut->GetRenderData()->LODResources[0])==S.Packed&&VertexHash(Original->GetRenderData()->LODResources[0])==S.Packed,TEXT("exact radio indices and untouched original packed geometry/UV/normal/color buffers"));
 const auto* Panel=S.Original[4].Component.Get();Check(Panel&&C->IsVisible()==Panel->IsVisible(),TEXT("native visibility follows unchanged radio through person/area schedule"));
 int32 Count=0;for(int32 I=0;I<W->PartCount();++I){const auto* N=W->Node(I);if(N&&N->Name==TEXT("mannequin:radio-woman"))++Count;}
 Check(Count==1,TEXT("exactly one adopted static mannequin after restore"));return Good;
}

TSharedPtr<FJsonObject> FS47RadioWoman::Readback() const
{
 if(State.IsValid()&&State->BenchDiagnosticRequested)DiagnoseTB2Bench(*State);
 auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("active"),IsActive());if(!State.IsValid())return J;const auto& S=*State;
 J->SetBoolField(TEXT("bench_diagnostic_requested"),S.BenchDiagnosticRequested);J->SetBoolField(TEXT("bench_postrepair_diagnostic_requested"),S.BenchDiagnosticPostRepair);
 J->SetBoolField(TEXT("preparation_complete"),S.Prepared);J->SetBoolField(TEXT("originals_captured"),S.Original.Num()==6);J->SetBoolField(TEXT("pending_live_measurement"),S.Pending);
 J->SetNumberField(TEXT("area_epoch"),double(S.AreaEpoch));J->SetNumberField(TEXT("measured_epoch"),double(S.MeasuredEpoch));J->SetNumberField(TEXT("town_step_serial"),double(S.TownStepSerial));
 J->SetBoolField(TEXT("scene_measurement_after_town_step"),S.Active&&S.MeasuredEpoch==S.AreaEpoch&&S.MeasuredTownStepSerial>0);
 J->SetArrayField(TEXT("radio_world_delta_ue_cm"),Vec(S.RadioDeltaUE));J->SetArrayField(TEXT("radio_local_delta_three_m"),Vec(S.RadioDeltaLocalThree));
 J->SetStringField(TEXT("reason"),S.Why);J->SetStringField(TEXT("asset"),S.Asset);J->SetNumberField(TEXT("native_node"),S.NativeNode);
 J->SetNumberField(TEXT("person_node"),1480);J->SetNumberField(TEXT("proxy_node"),1544);J->SetStringField(TEXT("interaction"),TEXT("radio_woman"));
 J->SetStringField(TEXT("schedule_owner"),TEXT("FS47Town::SetPeople / NightPerson 18:00..04:50"));J->SetStringField(TEXT("packed_vertex_sha1"),S.Packed);
 J->SetNumberField(TEXT("bench_surface_hits"),S.SeatHits);J->SetArrayField(TEXT("measured_seat_ue_cm"),Vec(S.Seat));
 J->SetObjectField(TEXT("bench_diagnostics"),S.BenchDiagnostics);
 if(S.BenchDiagnosticRequested)
 {
  auto* W=S.Web.Get();J->SetObjectField(TEXT("legacy_g1074_identity_live"),ComponentIdentity(W,1562));J->SetObjectField(TEXT("tb2_bench_identity_live"),ComponentIdentity(W,S.DiagnosticBenchNode));
  TArray<TSharedPtr<FJsonValue>> Supports;for(int32 Index:S.DiagnosticHitNodes)if(Index!=INDEX_NONE)Supports.Add(MakeShared<FJsonValueObject>(ComponentIdentity(W,Index)));J->SetArrayField(TEXT("support_components_live"),Supports);
 }
 if(auto* W=S.Web.Get())
 {
  J->SetObjectField(TEXT("tb2_bench_identity_live"),ComponentIdentity(W,S.DiagnosticBenchNode));
  TArray<TSharedPtr<FJsonValue>> RadioRows;const auto* Owner=W->Node(1480);
  if(Owner&&Owner->Comp.IsValid())for(int32 I:{3,4})if(S.Original.IsValidIndex(I))
  {
   const auto& P=S.Original[I];auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("node"),P.Id.Node);
   Row->SetArrayField(TEXT("captured_position_three_m"),Vec(P.PositionThree));
   if(const auto* N=W->Node(P.Id.Node))Row->SetArrayField(TEXT("actual_metadata_position_three_m"),Vec(N->P));
   if(auto* C=P.Component.Get())
   {
    Row->SetObjectField(TEXT("actual_relative_transform"),Transform(C->GetRelativeTransform()));
    Row->SetArrayField(TEXT("actual_world_delta_from_captured_ue_cm"),Vec(C->GetComponentLocation()-Owner->Comp->GetComponentTransform().TransformPosition(P.Relative.GetTranslation())));
   }
   RadioRows.Add(MakeShared<FJsonValueObject>(Row));
  }
  J->SetArrayField(TEXT("radio_parts_live"),RadioRows);
 }
 J->SetArrayField(TEXT("native_forward_ue"),Vec(S.Forward));J->SetArrayField(TEXT("native_right_ue"),Vec(S.Right));
 J->SetStringField(TEXT("limits"),TEXT("Runtime installation audit; root must verify actual seated coat/hand/radio/foot contact, all LOD views, interaction and disk/title/Continue. No human/FPS acceptance."));
 if(auto* C=S.Native.Get())
 {
  const FBox B=C->Bounds.GetBox();J->SetArrayField(TEXT("world_bounds_min_three_m"),Vec(S47Three::ToThree(B.Min)));J->SetArrayField(TEXT("world_bounds_max_three_m"),Vec(S47Three::ToThree(B.Max)));
  J->SetBoolField(TEXT("visible"),C->IsVisible());J->SetBoolField(TEXT("static_only"),!C->IsComponentTickEnabled());J->SetBoolField(TEXT("no_collision"),C->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
 }
 TArray<TSharedPtr<FJsonValue>> LODs;for(int32 N:S.LODs)LODs.Add(MakeShared<FJsonValueNumber>(N));J->SetArrayField(TEXT("native_lod_triangles"),LODs);
 J->SetNumberField(TEXT("radio_housing_triangles"),S.RadioIndices.Num()/3);J->SetNumberField(TEXT("radio_panel_triangles"),12);J->SetNumberField(TEXT("retired_body_triangles"),S.Active?436:0);
 bool OriginalIntact=S.Original.Num()==6;
 if(!S.Active)for(const auto& P:S.Original){auto* C=P.Component.Get();OriginalIntact&=C&&C->GetStaticMesh()==P.Mesh.Get()&&C->GetMaterials()==P.Materials&&C->GetRelativeTransform().Equals(P.Relative,.001)&&C->GetCollisionEnabled()==P.Collision&&C->bHiddenInGame==P.Hidden&&C->CastShadow==P.Shadow;}
 J->SetBoolField(TEXT("fallback_original_bindings_intact"),!S.Active&&OriginalIntact);return J;
}
