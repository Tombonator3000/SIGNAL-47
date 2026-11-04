#include "S47SaroOverlays.h"
#include "S47Web.h"
#include "S47Three.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "RenderUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformTime.h"
namespace
{
struct FOverlayVertex { float X,Y,Z,NX,NY,NZ,U,V,R,G,B,A; };
#include "S47SaroOverlayGeometry.inl"
UStaticMesh* Finish(AS47Web* W,const FName Name,TUniquePtr<FStaticMeshRenderData> Data,UMaterialInterface* Material,const FBox& Bounds)
{
 auto* Mesh=NewObject<UStaticMesh>(W,Name,RF_Transient);
 Mesh->bAllowCPUAccess=true;Mesh->NeverStream=true;Mesh->bSupportRayTracing=false;
 FStaticMaterial Slot(Material);Slot.UVChannelData=FMeshUVChannelInfo(1.f);Mesh->GetStaticMaterials().Add(Slot);
 Data->Bounds=FBoxSphereBounds(Bounds);Data->CurrentFirstLODIdx=0;Data->bReadyForStreaming=false;
 Mesh->SetRenderData(MoveTemp(Data));Mesh->SetExtendedBounds(FBoxSphereBounds(Bounds));Mesh->InitResources();return Mesh;
}
void Section(FStaticMeshLODResources& L,uint32 Count)
{
 FStaticMeshSection S;S.MaterialIndex=0;S.FirstIndex=0;S.NumTriangles=Count/3;S.MinVertexIndex=0;S.MaxVertexIndex=Count-1;S.bEnableCollision=false;S.bCastShadow=false;L.Sections.Add(S);
 L.bHasColorVertexData=true;L.bBuffersInlined=true;L.bIsOptionalLOD=false;L.MaxDeviation=0;
}
UStaticMesh* RoomMesh(AS47Web* W,int32 Geo,UMaterialInterface* Material)
{
 const FOverlayVertex* V=Geo==220?Grime220:Grime377;
 const uint32 Count=Geo==220?UE_ARRAY_COUNT(Grime220):UE_ARRAY_COUNT(Grime377);
 auto Data=MakeUnique<FStaticMeshRenderData>();Data->AllocateLODResources(1);Data->ScreenSize[0].Default=1;Data->NumInlinedLODs=1;
 auto& L=Data->LODResources[0];L.VertexBuffers.PositionVertexBuffer.Init(Count,true);
 auto& T=L.VertexBuffers.StaticMeshVertexBuffer;T.SetUseFullPrecisionUVs(true);T.Init(Count,1,true);
 FBox Bounds(ForceInit);TArray<uint32> Indices;TArray<FColor> Colors;Indices.Reserve(Count);Colors.Reserve(Count);
 for(uint32 I=0;I<Count;++I)
 {
  const auto& P=V[I];const FVector At=S47Three::ToUE(FVector(P.X,P.Y,P.Z));Bounds+=At;
  L.VertexBuffers.PositionVertexBuffer.VertexPosition(I)=FVector3f(At);
  const FVector N(P.NX,P.NZ,P.NY);const FVector Tangent=FVector::CrossProduct(FMath::Abs(N.Z)>.9?FVector::RightVector:FVector::UpVector,N).GetSafeNormal();
  T.SetVertexTangents(I,FVector3f(Tangent),FVector3f(FVector::CrossProduct(N,Tangent)),FVector3f(N));
  // Exported glTF UVs and linear vertex colour, matching the native importer.
  T.SetVertexUV(I,0,FVector2f(P.U,P.V));Colors.Add(FLinearColor(P.R,P.G,P.B,P.A).ToFColor(false));
  // The three.js to UE axis swap already changes glTF CCW to UE clockwise.
  Indices.Add(I);
 }
 L.VertexBuffers.ColorVertexBuffer.InitFromColorArray(Colors);Section(L,Count);L.SourceMeshBounds=FBoxSphereBounds(Bounds);
 L.IndexBuffer.TrySetAllowCPUAccess(true);L.IndexBuffer.SetIndices(Indices,EIndexBufferStride::Force32Bit);
 return Finish(W,*FString::Printf(TEXT("SaroOverlayStable%d"),Geo),MoveTemp(Data),Material,Bounds);
}
UStaticMesh* PipeMesh(AS47Web* W,UStaticMesh* Source,UMaterialInterface* Material)
{
 if(!Source||!Source->bAllowCPUAccess||!Source->GetRenderData())return nullptr;
 const auto& Old=*Source->GetRenderData();if(Old.LODResources.Num()!=3)return nullptr;
 auto Data=MakeUnique<FStaticMeshRenderData>();Data->AllocateLODResources(3);Data->NumInlinedLODs=3;FBox Bounds(ForceInit);int32 Moved=0;
 for(int32 Level=0;Level<3;++Level)
 {
  const auto& O=Old.LODResources[Level];auto& L=Data->LODResources[Level];const uint32 Count=O.VertexBuffers.PositionVertexBuffer.GetNumVertices();
  if(Count==0||O.GetNumTriangles()!=1233||O.Sections.Num()!=1)return nullptr;
  Data->ScreenSize[Level]=Old.ScreenSize[Level];L.VertexBuffers.PositionVertexBuffer.Init(Count,true);
  auto& T=L.VertexBuffers.StaticMeshVertexBuffer;const auto& OT=O.VertexBuffers.StaticMeshVertexBuffer;T.SetUseFullPrecisionUVs(true);T.Init(Count,OT.GetNumTexCoords(),true);
  TArray<FColor> Colors;FBox LocalBounds(ForceInit);
  for(uint32 I=0;I<Count;++I)
  {
   FVector3f P=O.VertexBuffers.PositionVertexBuffer.VertexPosition(I);
   // Six roof leaders: their legacy wet marks were 112mm off the wall,
   // inside the pipe's 55..145mm volume. Put the marks on the wall instead.
   const bool South=FMath::Abs(P.Y-561.2f)<.02f,North=FMath::Abs(P.Y+561.2f)<.02f;
   const float X= P.X*.01f;bool Leader=false;
   for(float Centre:South?TArray<float>{20.75f,31.85f,38.4f}:TArray<float>{20.8f,29.6f,38.4f})Leader|=FMath::Abs(X-Centre)<.351f;
   if((South||North)&&Leader&&P.Z>=4.9f&&P.Z<=65.1f){P.Y=South?551.6f:-551.6f;++Moved;}
   L.VertexBuffers.PositionVertexBuffer.VertexPosition(I)=P;LocalBounds+=FVector(P);Bounds+=FVector(P);
   T.SetVertexTangents(I,OT.VertexTangentX(I),OT.VertexTangentY(I),OT.VertexTangentZ(I));
   for(uint32 UV=0;UV<OT.GetNumTexCoords();++UV)T.SetVertexUV(I,UV,OT.GetVertexUV(I,UV));
   Colors.Add(O.VertexBuffers.ColorVertexBuffer.GetNumVertices()==Count?O.VertexBuffers.ColorVertexBuffer.VertexColor(I):FColor::White);
  }
  L.VertexBuffers.ColorVertexBuffer.InitFromColorArray(Colors);TArray<uint32> Indices;O.IndexBuffer.GetCopy(Indices);L.IndexBuffer.TrySetAllowCPUAccess(true);L.IndexBuffer.SetIndices(Indices,EIndexBufferStride::Force32Bit);
  L.Sections=O.Sections;for(auto& S:L.Sections){S.bCastShadow=false;S.bEnableCollision=false;}L.bHasColorVertexData=true;L.bBuffersInlined=true;L.bIsOptionalLOD=false;L.SourceMeshBounds=FBoxSphereBounds(LocalBounds);L.MaxDeviation=O.MaxDeviation;
 }
 if(Moved==0)return nullptr;
 UE_LOG(LogTemp,Display,TEXT("S47: downpipe wet marks seated on wall, %d vertices across three LODs"),Moved);
 return Finish(W,TEXT("SaroOverlayStablePipes"),MoveTemp(Data),Material,Bounds);
}
}
void S47SaroOverlays::Update(AS47Web* W)
{
 if(!W||!W->IsLoaded()||FParse::Param(FCommandLine::Get(),TEXT("S47SaroOverlayOriginal")))return;
 struct FProgress { uint8 Done=0; double NextAttempt=0; };
 static TMap<TWeakObjectPtr<AS47Web>,FProgress> Progress;
 for(auto It=Progress.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
 auto& State=Progress.FindOrAdd(TWeakObjectPtr<AS47Web>(W));
 if(State.Done==7||FPlatformTime::Seconds()<State.NextAttempt)return;
 State.NextAttempt=FPlatformTime::Seconds()+.5;
 for(int32 I=0;I<W->PartCount();++I)if(const auto* N=W->Node(I))
 {
  const bool Room=(N->Geo==220&&N->Name==TEXT("clutter:control:dirt"))||(N->Geo==377&&N->Name==TEXT("clutter:annex:dirt"));
  if(!Room&&N->Geo!=455)continue;
  const uint8 Bit=N->Geo==220?1:N->Geo==377?2:4;if(State.Done&Bit)continue;
  auto* C=Cast<UStaticMeshComponent>(N->Comp.Get());if(!C||!C->GetStaticMesh()||!C->GetMaterial(0))continue;
  UStaticMesh* Old=C->GetStaticMesh();if(Old->GetName().StartsWith(TEXT("SaroOverlayStable"))){State.Done|=Bit;continue;}
  UStaticMesh* New=nullptr;
  if(Room&&Old->GetNumTriangles(0)==(N->Geo==220?176:284))New=RoomMesh(W,N->Geo,C->GetMaterial(0));
  else if(N->Geo==455&&Old->GetName()==TEXT("SM_EW_AccessStains"))New=PipeMesh(W,Old,C->GetMaterial(0));
  if(New){State.Done|=Bit;C->SetStaticMesh(New);C->SetCastShadow(false);UE_LOG(LogTemp,Display,TEXT("S47: stable SARO overlay geometry %d"),N->Geo);}
 }
}
