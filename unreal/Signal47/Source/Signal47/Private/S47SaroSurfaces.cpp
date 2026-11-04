#include "S47SaroSurfaces.h"
#include "S47Web.h"
#include "S47Three.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "StaticMeshResources.h"
#include "RenderUtils.h"
namespace
{
#include "S47SaroSurfaceGeometry.inl"
}
UStaticMesh* S47SaroSurfaces::Mesh(AS47Web* W,UMaterialInterface* Material,bool Concrete)
{
 if(!W||!Material)return nullptr;
 const FVector2D* Points=Concrete?ConcreteVertices:AsphaltVertices;
 const uint32 Count=Concrete?UE_ARRAY_COUNT(ConcreteVertices):UE_ARRAY_COUNT(AsphaltVertices);
 auto* Mesh=NewObject<UStaticMesh>(W,NAME_None,RF_Transient);
 Mesh->bAllowCPUAccess=true;Mesh->NeverStream=true;Mesh->bSupportRayTracing=true;
 FStaticMaterial Slot(Material);Slot.UVChannelData=FMeshUVChannelInfo(360.f);Mesh->GetStaticMaterials().Add(Slot);
 auto Data=MakeUnique<FStaticMeshRenderData>();Data->AllocateLODResources(1);
 Data->ScreenSize[0].Default=1;Data->NumInlinedLODs=1;Data->CurrentFirstLODIdx=0;Data->bReadyForStreaming=false;
 auto& L=Data->LODResources[0];L.VertexBuffers.PositionVertexBuffer.Init(Count,true);
 auto& T=L.VertexBuffers.StaticMeshVertexBuffer;T.SetUseFullPrecisionUVs(true);T.Init(Count,1,true);
 FBox Bounds(ForceInit);TArray<uint32> Indices;Indices.Reserve(Count);
 for(uint32 I=0;I<Count;++I)
 {
  const FVector P=S47Three::ToUE(FVector(Points[I].X,Height,Points[I].Y));Bounds+=P;
  L.VertexBuffers.PositionVertexBuffer.VertexPosition(I)=FVector3f(P);
  T.SetVertexTangents(I,FVector3f(1,0,0),FVector3f(0,1,0),FVector3f(0,0,1));
  T.SetVertexUV(I,0,FVector2f(P.X/360.,P.Y/360.));
  // UE front faces use clockwise winding; keep the explicit upward normals.
  Indices.Add(I-I%3+(2-I%3));
 }
 Data->Bounds=FBoxSphereBounds(Bounds);FStaticMeshSection Section;
 Section.MaterialIndex=0;Section.FirstIndex=0;Section.NumTriangles=Count/3;Section.MinVertexIndex=0;Section.MaxVertexIndex=Count-1;
 Section.bEnableCollision=false;Section.bCastShadow=false;L.Sections.Add(Section);
 L.bHasColorVertexData=false;L.bBuffersInlined=true;L.bIsOptionalLOD=false;L.SourceMeshBounds=Data->Bounds;L.MaxDeviation=0;
 L.IndexBuffer.TrySetAllowCPUAccess(true);L.IndexBuffer.SetIndices(Indices,EIndexBufferStride::Force32Bit);
 bool RT=true;
#if !WITH_EDITOR
 RT=IsRayTracingAllowed();
#endif
 if(RT)Data->InitializeRayTracingRepresentationFromRenderingLODs();
 Mesh->SetRenderData(MoveTemp(Data));Mesh->SetExtendedBounds(FBoxSphereBounds(Bounds));Mesh->InitResources();return Mesh;
}
