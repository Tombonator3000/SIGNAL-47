#include "S47StationPhone.h"
#include "S47Web.h"
#include "S47Three.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "StaticMeshResources.h"
#include "RenderUtils.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
 struct FInstalledCut {TWeakObjectPtr<UStaticMeshComponent> Component;TStrongObjectPtr<UStaticMesh> Source;TWeakObjectPtr<UStaticMesh> Copy;TArray<TWeakObjectPtr<UMaterialInterface>> Materials;int32 Node=INDEX_NONE,Removed=0;};
 struct FState {TWeakObjectPtr<AS47Web> World;TWeakObjectPtr<UStaticMeshComponent> Model;bool Attempted=false,Installed=false;int32 Removed=0;TArray<FInstalledCut> Cuts;};
 FState State;
 struct FCut {UStaticMeshComponent* Component=nullptr;UStaticMesh* Source=nullptr;UStaticMesh* Copy=nullptr;int32 Node=INDEX_NONE,Removed=0;bool Whole=false;};
 bool InPhone(const FVector& P)
 {return P.X>=7983.96&&P.X<=7984.85&&P.Y>=4.48&&P.Y<=6.90&&P.Z>=2064.10&&P.Z<=2065.10;}
 UStaticMesh* Remainder(AS47Web* W,UStaticMesh* Source,const TArray<uint32>& Indices,const TArray<FStaticMeshSection>& Sections)
 {
  const auto& Src=Source->GetRenderData()->LODResources[0];const auto& B=Src.VertexBuffers;const auto& T=B.StaticMeshVertexBuffer;
  if(!B.PositionVertexBuffer.GetVertexData()||!T.GetTangentData()||!T.GetTexCoordData())return nullptr;
  auto* Copy=NewObject<UStaticMesh>(W,NAME_None,RF_Transient);Copy->bAllowCPUAccess=true;Copy->NeverStream=true;Copy->bSupportRayTracing=Source->bSupportRayTracing;
  Copy->GetStaticMaterials()=Source->GetStaticMaterials();Copy->SetBodySetup(Source->GetBodySetup());Copy->SetLightMapCoordinateIndex(Source->GetLightMapCoordinateIndex());
  auto Data=MakeUnique<FStaticMeshRenderData>();Data->AllocateLODResources(1);Data->Bounds=Source->GetRenderData()->Bounds;Data->ScreenSize[0].Default=1;Data->NumInlinedLODs=1;Data->CurrentFirstLODIdx=0;Data->bReadyForStreaming=false;
  auto& D=Data->LODResources[0];D.Sections.Append(Sections);D.VertexBuffers.PositionVertexBuffer.Init(B.PositionVertexBuffer,true);
  auto& V=D.VertexBuffers.StaticMeshVertexBuffer;V.SetUseHighPrecisionTangentBasis(T.GetUseHighPrecisionTangentBasis());V.SetUseFullPrecisionUVs(T.GetUseFullPrecisionUVs());V.Init(T.GetNumVertices(),T.GetNumTexCoords(),true);
  if(V.GetTangentSize()!=T.GetTangentSize()||V.GetTexCoordSize()!=T.GetTexCoordSize())return nullptr;
  FMemory::Memcpy(V.GetTangentData(),T.GetTangentData(),T.GetTangentSize());FMemory::Memcpy(V.GetTexCoordData(),T.GetTexCoordData(),T.GetTexCoordSize());
  const uint32 Colors=B.ColorVertexBuffer.GetNumVertices();if(Colors){auto& C=D.VertexBuffers.ColorVertexBuffer;C.Init(Colors,true);if(C.GetStride()!=B.ColorVertexBuffer.GetStride())return nullptr;FMemory::Memcpy(C.GetVertexData(),B.ColorVertexBuffer.GetVertexData(),Colors*C.GetStride());}
  D.bHasColorVertexData=Src.bHasColorVertexData;D.bBuffersInlined=true;D.bIsOptionalLOD=false;D.SourceMeshBounds=Data->Bounds;D.MaxDeviation=Src.MaxDeviation;
  D.IndexBuffer.SetIndices(Indices,EIndexBufferStride::Force32Bit);if(!D.IndexBuffer.TrySetAllowCPUAccess(true))return nullptr;
  bool RT=Copy->bSupportRayTracing;
#if !WITH_EDITOR
  RT=RT&&IsRayTracingAllowed();
#endif
  if(RT)Data->InitializeRayTracingRepresentationFromRenderingLODs();
  const FBoxSphereBounds Bounds=Data->Bounds;Copy->SetRenderData(MoveTemp(Data));Copy->SetExtendedBounds(Bounds);return Copy;
 }
}
namespace S47StationPhone
{
 void Register(AS47Web* W,UStaticMeshComponent* C){State={};State.World=W;State.Model=C;if(C)C->SetHiddenInGame(true,false);}
 bool IsVerifiedRemainder(const AS47Web* W,const UStaticMeshComponent* C,const UStaticMesh* Source)
 {
  if(State.World!=W||!State.Installed||!C||!Source)return false;
  for(const auto& Cut:State.Cuts)if(Cut.Component.Get()==C&&Cut.Source.Get()==Source)
  {
   const auto* Node=W->Node(Cut.Node);
   if(!Node||Node->Comp.Get()!=C||Node->Geo!=Cut.Node-449||Cut.Node<1448||Cut.Node>1450)return false;
   const auto* Copy=Cut.Copy.Get();
   if(!Copy||C->GetStaticMesh()!=Copy||Copy->GetNumLODs()!=1||Source->GetNumLODs()!=1||Copy->GetNumTriangles(0)!=Source->GetNumTriangles(0)-Cut.Removed||C->GetNumMaterials()!=Cut.Materials.Num()||Copy->GetStaticMaterials()!=Source->GetStaticMaterials())return false;
   for(int32 I=0;I<Cut.Materials.Num();++I)if(C->GetMaterial(I)!=Cut.Materials[I].Get())return false;
   const FString Path=Source->GetPathName();
   return (Path==TEXT("/Game/CalicheHomes/Trim/g999.g999")&&Source->GetNumTriangles(0)==1356&&Cut.Removed==24)
    ||(Path==TEXT("/Game/CalicheHomes/Trim/g1000.g1000")&&Source->GetNumTriangles(0)==108&&Cut.Removed==24)
    ||(Path==TEXT("/Game/CalicheHomes/Trim/g1001.g1001")&&Source->GetNumTriangles(0)==4&&Cut.Removed==2);
  }
  return false;
 }
 bool Ready(const AS47Web* W)
 {
  if(State.World!=W||!State.Installed||!State.Model.IsValid()||State.Model->bHiddenInGame||State.Cuts.Num()!=3)return false;
  for(const auto& Cut:State.Cuts)if(!IsVerifiedRemainder(W,Cut.Component.Get(),Cut.Source.Get()))return false;
  return true;
 }
 int32 RemovedTriangles(const AS47Web* W){return State.World==W?State.Removed:0;}
 void Update(AS47Web* W)
 {
  if(State.World!=W||State.Attempted||!State.Model.IsValid())return;State.Attempted=true;
  const int32 Town=W->Handle(TEXT("world.town.group"));if(Town<0)return;TArray<FCut> Cuts;int32 Total=0;
  // First Props tick follows Web BeginPlay: TownBuildings -> CalicheHomes -> CinemaPosters -> MotelV2.
  // CalicheHomes is the last writer of these three nodes; its Update only retires other nodes.
  // Never touch proxies, native
  // replacement props or unrelated surfaces sharing the old town materials.
  for(int32 I=0;;++I)
  {
   const auto* N=W->Node(I);if(!N)break;if(I<1448||I>1450||N->Geo!=I-449)continue;if((N->Geo!=999&&N->Geo!=1000&&N->Geo!=1001)||N->bInstanced||N->Name.Contains(TEXT("hit:")))continue;
   bool Under=false;for(int32 P=N->Parent;P>=0;){if(P==Town){Under=true;break;}const auto* Parent=W->Node(P);if(!Parent)break;P=Parent->Parent;}if(!Under)continue;
   auto* C=Cast<UStaticMeshComponent>(N->Comp.Get());UStaticMesh* Mesh=C?C->GetStaticMesh():nullptr;
   UE_LOG(LogTemp,Display,TEXT("S47 station phone source node%d g%d mesh=%s hidden=%d lods=%d cpu=%d bounds=%s transform=%s"),I,N->Geo,*GetPathNameSafe(Mesh),C?int32(C->bHiddenInGame):-1,Mesh?Mesh->GetNumLODs():-1,Mesh?int32(Mesh->bAllowCPUAccess):-1,C?*C->Bounds.GetBox().ToString():TEXT("none"),C?*C->GetComponentTransform().ToHumanReadableString():TEXT("none"));
   const int32 ExpectedSourceTris=N->Geo==999?1356:N->Geo==1000?108:4;
   const FString ExpectedPath=FString::Printf(TEXT("/Game/CalicheHomes/Trim/g%d.g%d"),N->Geo,N->Geo);
   if(!Mesh||C->bHiddenInGame||Mesh->GetPathName()!=ExpectedPath||!Mesh->GetRenderData()||Mesh->GetNumLODs()!=1||Mesh->GetNumTriangles(0)!=ExpectedSourceTris||!Mesh->bAllowCPUAccess)continue;
   const auto& L=Mesh->GetRenderData()->LODResources[0];const auto& P=L.VertexBuffers.PositionVertexBuffer;
   UE_LOG(LogTemp,Display,TEXT("S47 station phone buffers g%d vertices=%u positions=%d indexBytes=%d triangles=%u"),N->Geo,P.GetNumVertices(),P.GetVertexData()!=nullptr,L.IndexBuffer.GetIndexDataSize(),L.GetNumTriangles());
   if(!P.GetVertexData()||L.IndexBuffer.GetIndexDataSize()==0)continue;TArray<uint32> Old,Keep;L.IndexBuffer.GetCopy(Old);TArray<FStaticMeshSection> Sections;int32 Removed=0;
   for(const auto& Section:L.Sections)
   {
    auto S=Section;S.FirstIndex=Keep.Num();S.NumTriangles=0;
    for(uint32 K=Section.FirstIndex;K+2<Section.FirstIndex+Section.NumTriangles*3;K+=3)
    {
     bool Inside=true;for(int32 Corner=0;Corner<3;++Corner){const uint32 V=Old[K+Corner];if(V>=P.GetNumVertices())return;Inside&=InPhone(S47Three::ToThree(C->GetComponentTransform().TransformPosition(FVector(P.VertexPosition(V)))));}
     if(Inside){++Removed;continue;}Keep.Append({Old[K],Old[K+1],Old[K+2]});++S.NumTriangles;
    }
    if(S.NumTriangles)Sections.Add(S);
   }
   UE_LOG(LogTemp,Display,TEXT("S47 station phone measured g%d removed=%d expected=%d"),N->Geo,Removed,N->Geo==1001?2:24);
   if(Removed!=(N->Geo==1001?2:24))return;FCut Cut;Cut.Component=C;Cut.Node=I;Cut.Source=Mesh;Cut.Removed=Removed;Cut.Whole=Keep.IsEmpty();
   if(!Cut.Whole){Cut.Copy=Remainder(W,Mesh,Keep,Sections);if(!Cut.Copy)return;}Cuts.Add(Cut);Total+=Removed;
  }
  if(Total!=50||Cuts.Num()!=3){UE_LOG(LogTemp,Warning,TEXT("S47 station phone retained original: unexpected precise cut %d triangles"),Total);return;}
  for(const auto& Cut:Cuts)
  {
   auto* C=Cut.Component;if(Cut.Whole){C->SetHiddenInGame(true,false);C->SetCastShadow(false);}
   else
   {
    auto Materials=C->GetMaterials();Cut.Copy->InitResources();C->SetStaticMesh(Cut.Copy);for(int32 K=0;K<Materials.Num();++K)C->SetMaterial(K,Materials[K]);
    // Component's UPROPERTY owns the clone. Retain its detached source for the strict composed audit.
    FInstalledCut Installed;Installed.Component=C;Installed.Node=Cut.Node;Installed.Source.Reset(Cut.Source);Installed.Copy=Cut.Copy;Installed.Removed=Cut.Removed;
    for(auto* Material:Materials)Installed.Materials.Add(Material);State.Cuts.Add(MoveTemp(Installed));
   }
  }
  // The existing call proxy/phone collider/audio anchor stay at exactly the same spot.
  State.Model->SetHiddenInGame(false,false);State.Removed=Total;State.Installed=true;
  UE_LOG(LogTemp,Display,TEXT("S47 station phone: existing blue model installed at original call anchor; %d legacy triangles removed, proxies untouched"),Total);
 }
}
