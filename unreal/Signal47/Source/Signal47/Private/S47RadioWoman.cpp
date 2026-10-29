// HUM-01: preserve the existing lap radio and person owner; replace only old body geometry.
#include "S47RadioWoman.h"
#include "S47Web.h"
#include "S47Three.h"
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
 FString AssetPath=TEXT("/Game/MetaHumans/Mannequins/RadioWoman/BakeR10/SM_RadioWoman_Seated");
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
 bool BenchDiagnosticRequested=false;int32 DiagnosticBenchNode=INDEX_NONE,DiagnosticReadbacks=0;TSet<int32> DiagnosticHitNodes;
 TArray<uint32> FullIndices,RadioIndices;TArray<int32> LODs;
};

namespace
{
 bool DiagnoseTB2Bench(FS47RadioWomanState& S)
 {
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
 S.BenchDiagnosticRequested=FParse::Param(FCommandLine::Get(),TEXT("S47RadioWomanBenchDiagnostic"));
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
  FOriginal P;P.Id=E;P.Component=C;P.Mesh.Reset(M);P.Materials=C->GetMaterials();P.Relative=C->GetRelativeTransform();P.Collision=C->GetCollisionEnabled();P.Hidden=C->bHiddenInGame;P.Shadow=C->CastShadow;P.AuthoredVisible=N->bVis;S.Original.Add(MoveTemp(P));
 }
 if(S.BenchDiagnosticRequested){S.Why=TEXT("explicit read-only TB2 bench diagnosis; measurements run fresh on Readback, original figure/radio retained");return Result;}
 if(FParse::Param(FCommandLine::Get(),TEXT("S47RadioWomanOff"))||FParse::Param(FCommandLine::Get(),TEXT("S47PropsOff")))return Reject(TEXT("disabled by startup flag"));
 S.Asset=AssetPath;FParse::Value(FCommandLine::Get(),TEXT("S47RadioWomanAsset="),S.Asset);
 if(!S.Asset.StartsWith(TEXT("/Game/MetaHumans/Mannequins/RadioWoman/"))||S.Asset.Contains(TEXT(".."))||S.Asset.Contains(TEXT(" ")))return Reject(TEXT("private candidate asset path"));
 if(!S.Asset.Contains(TEXT("."))){FString Name;S.Asset.Split(TEXT("/"),nullptr,&Name,ESearchCase::CaseSensitive,ESearchDir::FromEnd);S.Asset+=TEXT(".")+Name;}
 S.NativeAsset.Reset(LoadObject<UStaticMesh>(nullptr,*S.Asset,nullptr,LOAD_NoWarn|LOAD_Quiet));
 if(!NativeReady(S.NativeAsset.Get(),S.LODs))return Reject(TEXT("complete private static three-LOD seated candidate unavailable or invalid"));
 UStaticMesh* Cut=RadioCut(W,S.Original[3].Mesh.Get(),S.FullIndices,S.RadioIndices,S.Packed);
 if(!Cut)return Reject(TEXT("g1031 CPU-ready exact12-radio/24-shoe triangle partition required"));
 S.Cut=Cut;
 // Measure the seat against the actual existing purchased bench, not the old origin alone.
 const auto* BenchNode=W->Node(1562);auto* Bench=BenchNode?Cast<UStaticMeshComponent>(BenchNode->Comp.Get()):nullptr;
 if(BenchNode){D->SetNumberField(TEXT("bench_geo"),BenchNode->Geo);D->SetNumberField(TEXT("bench_parent"),BenchNode->Parent);D->SetStringField(TEXT("bench_name"),BenchNode->Name);}
 if(Bench)
 {
  D->SetStringField(TEXT("bench_component"),GetPathNameSafe(Bench));D->SetStringField(TEXT("bench_mesh"),GetPathNameSafe(Bench->GetStaticMesh().Get()));
  D->SetObjectField(TEXT("bench_transform"),Transform(Bench->GetComponentTransform()));D->SetObjectField(TEXT("bench_relative_transform"),Transform(Bench->GetRelativeTransform()));
  D->SetArrayField(TEXT("bench_bounds_min_ue_cm"),Vec(Bench->Bounds.GetBox().Min));D->SetArrayField(TEXT("bench_bounds_max_ue_cm"),Vec(Bench->Bounds.GetBox().Max));
  D->SetNumberField(TEXT("bench_collision_enabled"),int32(Bench->GetCollisionEnabled()));D->SetBoolField(TEXT("bench_visible"),Bench->IsVisible());
 }
 if(!BenchNode||BenchNode->Geo!=1074||BenchNode->Name!=TEXT("lic:garden_bench")||!Bench||!Bench->GetStaticMesh()
  ||Bench->GetStaticMesh()!=W->GeoMesh(1074)||Bench->GetStaticMesh()->GetPathName()!=TEXT("/Game/Web/Geo/geo_008/StaticMeshes/g1074.g1074"))return Reject(TEXT("actual laundromat bench unavailable"));
 const FVector Anchor=Group->Comp->GetComponentLocation();TArray<double> Heights;
 const FCollisionQueryParams Q(SCENE_QUERY_STAT(S47RadioWomanSeat),true);
 D->SetBoolField(TEXT("trace_attempted"),true);TArray<TSharedPtr<FJsonValue>> Samples;
 for(double X:{-2.,0.,2.})for(double Y:{-2.,0.,2.})
 {
  const FVector At=Anchor+FVector(X,Y,0);FHitResult Hit;
  const FVector Start=At+FVector(0,0,20),End=At-FVector(0,0,15);
  const bool Found=Bench->LineTraceComponent(Hit,Start,End,Q),Accepted=Found&&Hit.ImpactNormal.Z>.7;
  if(Accepted)Heights.Add(Hit.ImpactPoint.Z);
  auto Sample=MakeShared<FJsonObject>();Sample->SetNumberField(TEXT("sample_index"),Samples.Num());Sample->SetArrayField(TEXT("sample_ue_cm"),Vec(At));
  Sample->SetArrayField(TEXT("trace_start_ue_cm"),Vec(Start));Sample->SetArrayField(TEXT("trace_end_ue_cm"),Vec(End));Sample->SetBoolField(TEXT("hit"),Found);
  Sample->SetBoolField(TEXT("blocking_hit"),Hit.bBlockingHit);Sample->SetBoolField(TEXT("start_penetrating"),Hit.bStartPenetrating);Sample->SetBoolField(TEXT("accepted_surface"),Accepted);
  Sample->SetArrayField(TEXT("impact_ue_cm"),Vec(Hit.ImpactPoint));Sample->SetArrayField(TEXT("impact_normal_ue"),Vec(Hit.ImpactNormal));Sample->SetNumberField(TEXT("face_index"),Hit.FaceIndex);
  Sample->SetStringField(TEXT("requested_bench_component"),GetPathNameSafe(Bench));Sample->SetStringField(TEXT("hit_component"),GetPathNameSafe(Hit.GetComponent()));Sample->SetNumberField(TEXT("hit_node"),W->NodeOf(Hit.GetComponent()));
  Samples.Add(MakeShared<FJsonValueObject>(Sample));
 }
 D->SetArrayField(TEXT("samples"),Samples);D->SetNumberField(TEXT("surface_hits"),Heights.Num());
 if(!Heights.IsEmpty())
 {
  auto Sorted=Heights;Sorted.Sort();const double Median=Sorted[Sorted.Num()/2],Spread=Sorted.Last()-Sorted[0];
  D->SetNumberField(TEXT("minimum_ue_cm"),Sorted[0]);D->SetNumberField(TEXT("maximum_ue_cm"),Sorted.Last());D->SetNumberField(TEXT("median_ue_cm"),Median);
  D->SetNumberField(TEXT("spread_cm"),Spread);D->SetNumberField(TEXT("median_minus_anchor_cm"),Median-Anchor.Z);D->SetBoolField(TEXT("anchor_within_2cm"),FMath::Abs(Median-Anchor.Z)<=2.);D->SetBoolField(TEXT("spread_within_1_5cm"),Spread<=1.5);
 }
 // Diagnostic world rays only: their results never choose or validate placement.
 auto Ground=MakeShared<FJsonObject>();D->SetObjectField(TEXT("ground_diagnostics"),Ground);
 auto* World=W->GetWorld();Ground->SetBoolField(TEXT("world_available"),World!=nullptr);Ground->SetStringField(TEXT("scope"),TEXT("Diagnostic only; group-basis front70/85cm and side+-16cm, original six person components and bench ignored; no placement or acceptance effect."));
 Ground->SetStringField(TEXT("channel"),TEXT("ECC_Visibility"));TArray<TSharedPtr<FJsonValue>> GroundSamples,Ignored;
 FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(S47RadioWomanSeatGround),true);GroundQuery.AddIgnoredComponent(Bench);Ignored.Add(MakeShared<FJsonValueString>(GetPathNameSafe(Bench)));
 for(const auto& P:S.Original)if(P.Component.IsValid()){GroundQuery.AddIgnoredComponent(P.Component.Get());Ignored.Add(MakeShared<FJsonValueString>(GetPathNameSafe(P.Component.Get())));}
 Ground->SetArrayField(TEXT("ignored_components"),Ignored);
 const FTransform GroupWorld=Group->Comp->GetComponentTransform();
 const FVector GroundForward=GroupWorld.TransformVectorNoScale(S47Three::DirToUE(FVector(0,0,-1))).GetSafeNormal();
 const FVector GroundRight=GroupWorld.TransformVectorNoScale(S47Three::DirToUE(FVector(1,0,0))).GetSafeNormal();
 Ground->SetArrayField(TEXT("forward_ue"),Vec(GroundForward));Ground->SetArrayField(TEXT("right_ue"),Vec(GroundRight));
 if(World)for(double Front:{70.,85.})for(double Side:{-16.,16.})
 {
  const FVector At=Anchor+GroundForward*Front+GroundRight*Side,Start=At+FVector(0,0,50),End=At-FVector(0,0,150);FHitResult Hit;
  const bool Found=World->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,GroundQuery);
  auto Sample=MakeShared<FJsonObject>();Sample->SetNumberField(TEXT("front_cm"),Front);Sample->SetNumberField(TEXT("side_cm"),Side);
  Sample->SetArrayField(TEXT("trace_start_ue_cm"),Vec(Start));Sample->SetArrayField(TEXT("trace_end_ue_cm"),Vec(End));Sample->SetBoolField(TEXT("hit"),Found);Sample->SetBoolField(TEXT("blocking_hit"),Hit.bBlockingHit);
  Sample->SetArrayField(TEXT("impact_ue_cm"),Vec(Hit.ImpactPoint));Sample->SetArrayField(TEXT("impact_normal_ue"),Vec(Hit.ImpactNormal));Sample->SetStringField(TEXT("hit_component"),GetPathNameSafe(Hit.GetComponent()));Sample->SetNumberField(TEXT("hit_node"),W->NodeOf(Hit.GetComponent()));Sample->SetNumberField(TEXT("face_index"),Hit.FaceIndex);
  if(Found)Sample->SetNumberField(TEXT("anchor_above_impact_cm"),Anchor.Z-Hit.ImpactPoint.Z);
  GroundSamples.Add(MakeShared<FJsonValueObject>(Sample));
 }
 Ground->SetArrayField(TEXT("samples"),GroundSamples);
 S.SeatHits=Heights.Num();if(Heights.Num()<5)return Reject(TEXT("actual bench seat requires five of nine physical surface samples"));
 Heights.Sort();S.Seat=FVector(Anchor.X,Anchor.Y,Heights[Heights.Num()/2]);
 D->SetObjectField(TEXT("native_sole_ground_diagnostics"),SoleGroundDiagnostics(W,S.NativeAsset.Get(),FTransform(FRotationMatrix::MakeFromXY(GroundForward,GroundRight).ToQuat(),S.Seat),GroundQuery));
 const double SeatFrameError=FMath::Abs((S.Seat.Z-Anchor.Z)-MeasuredSeatFromLegacyCm);
 D->SetNumberField(TEXT("actual_measured_seat_frame_error_cm"),SeatFrameError);
 D->SetBoolField(TEXT("measured_seat_binding_valid"),SeatFrameError<=MaxSeatFrameErrorCm);
 if(SeatFrameError>MaxSeatFrameErrorCm||Heights.Last()-Heights[0]>1.5)return Reject(TEXT("actual bench seat differs from the measured g1074 seat binding"));
 const auto SoleData=D->GetObjectField(TEXT("native_sole_ground_diagnostics"));const bool SolesReady=SoleGroundReady(SoleData);
 D->SetBoolField(TEXT("all_lowest_soles_within_contact_limit"),SolesReady);D->SetNumberField(TEXT("maximum_sole_ground_gap_cm"),MaxSoleGapCm);
 if(!SolesReady)return Reject(TEXT("every actual lowest sole vertex must reach physical ground within1.5cm before adoption"));
 S.Forward=Group->Comp->GetComponentTransform().TransformVectorNoScale(S47Three::DirToUE(FVector(0,0,-1))).GetSafeNormal();
 S.Right=Group->Comp->GetComponentTransform().TransformVectorNoScale(S47Three::DirToUE(FVector(1,0,0))).GetSafeNormal();
 if(!S.Forward.Equals(FVector::ForwardVector,.00001)||!S.Right.Equals(FVector::RightVector,.00001))return Reject(TEXT("canonical road-facing orientation"));
 S.Placement=FTransform(FRotationMatrix::MakeFromXY(S.Forward,S.Right).ToQuat(),S.Seat);
 Cut->InitResources();if(!Cut->GetRenderData()->IsInitialized())return Reject(TEXT("radio render resources before component binding"));
 auto* C=NewObject<UStaticMeshComponent>(W,TEXT("RadioWomanStatic"));C->SetMobility(EComponentMobility::Movable);
 C->SetStaticMesh(S.NativeAsset.Get());C->SetupAttachment(Group->Comp.Get());C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);
 C->SetCastShadow(true);C->SetHiddenInGame(true,false);C->SetComponentTickEnabled(false);W->AddInstanceComponent(C);C->RegisterComponent();C->SetWorldTransform(S.Placement);
 S.NativeNode=W->AdoptNode(C,1480,TEXT("mannequin:radio-woman"));S.Native=C;
 if(S.NativeNode==INDEX_NONE){C->DestroyComponent();return Reject(TEXT("native visibility adoption"));}
 // All source/CPU/native/seat checks and resource initialization completed before this commit.
 auto* Housing=S.Original[3].Component.Get();Housing->SetStaticMesh(Cut);
 for(int32 I=0;I<S.Original[3].Materials.Num();++I)Housing->SetMaterial(I,S.Original[3].Materials[I]);
 Housing->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Original BodySetup also contains the removed shoes.
 for(int32 I:{0,1,2,5}){auto* Old=S.Original[I].Component.Get();W->SetVisible(S.Original[I].Id.Node,false);Old->SetHiddenInGame(true,false);Old->SetCastShadow(false);Old->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
 C->SetHiddenInGame(false,false);S.Active=true;S.Why=TEXT("installed: exact radio preserved, one static adopted mannequin");
 UE_LOG(LogS47RadioWoman,Display,TEXT("S47: radio woman ready: asset=%s node=%d radio12+metal12 retained body436 retired nativeLODs=%d/%d/%d seatHits=%d packedSHA1=%s; seated visual/contact acceptance remains required"),*S.Asset,S.NativeNode,S.LODs[0],S.LODs[1],S.LODs[2],S.SeatHits,*S.Packed);
 return Result;
}

bool FS47RadioWoman::IsActive() const{return State.IsValid()&&State->Active;}
bool FS47RadioWoman::Audit(TArray<FString>& Lines) const
{
 bool Good=true;auto Check=[&](bool Pass,const TCHAR* Why){Lines.Add(FString::Printf(TEXT("%s radio woman %s"),Pass?TEXT("OK"):TEXT("FAIL"),Why));Good&=Pass;};
 Check(IsActive(),State.IsValid()?*State->Why:TEXT("missing install state"));if(!IsActive())return false;
 const auto& S=*State;auto* W=S.Web.Get();auto* C=S.Native.Get();Check(W&&C&&C->IsRegistered()&&C->GetStaticMesh()==S.NativeAsset.Get(),TEXT("one actual static mesh registered"));if(!W||!C)return false;
 const auto* Adopted=W->Node(S.NativeNode);Check(Adopted&&Adopted->Parent==1480&&Adopted->Name==TEXT("mannequin:radio-woman")&&W->NodeOf(C)==S.NativeNode,TEXT("native belongs to original person visibility owner"));
 Check(C->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!C->IsComponentTickEnabled()&&C->GetComponentTransform().Equals(S.Placement,.02),TEXT("static-only NoCollision and measured placement retained"));
 Check(S.BenchDiagnostics->GetBoolField(TEXT("measured_seat_binding_valid"))&&S.BenchDiagnostics->GetBoolField(TEXT("all_lowest_soles_within_contact_limit")),TEXT("measured bench binding and every lowest sole passed physical pre-adoption contact"));
 Check(W->Handle(TEXT("world.town.woman"))==1480&&W->Handle(TEXT("world.town.proxies.woman"))==1544&&W->IsVisible(1480)==W->IsVisible(1544),TEXT("original person/proxy ownership and schedule agree"));
 for(int32 I=0;I<S.Original.Num();++I)
 {
  const auto& P=S.Original[I];const auto* N=W->Node(P.Id.Node);auto* Old=P.Component.Get();const bool Mixed=I==3,Panel=I==4;
  Check(N&&N->Parent==1480&&N->Geo==P.Id.Geo&&N->Mats==TArray<int32>{P.Id.Material}&&Old&&Old->GetRelativeTransform().Equals(P.Relative,.001),TEXT("original legacy metadata/transform retained"));
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
 J->SetBoolField(TEXT("bench_diagnostic_requested"),S.BenchDiagnosticRequested);
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
