#include "S47SaroLegacyCleanup.h"

#include "S47Three.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "RenderUtils.h"
#include "StaticMeshResources.h"
#include "Misc/CommandLine.h"

namespace S47SaroLegacyCleanup
{
namespace
{
 struct FState { TSet<FString> Applied; FString Failure; bool bBuilt=false; };
 TMap<TWeakObjectPtr<AS47Web>,FState> States;
 struct FVertex
 {
  FVector3f Position,X,Y,Z; TArray<FVector2f> UV; FColor Color=FColor::White;
 };
 struct FPlan
 {
  UStaticMeshComponent* Component=nullptr; UStaticMesh* Source=nullptr; UStaticMesh* Cut=nullptr;
  TArray<int32> Edits; TArray<uint32> Indices; TArray<FVertex> Added;
  TArray<FStaticMeshSection> Sections; TArray<UMaterialInterface*> Materials;
  FBox Bounds=FBox(ForceInit); int32 Before=0;
 };
 bool Fail(FString* Why,const FString& Message) { if (Why) *Why=Message; return false; }
 bool Under(AS47Web* W,int32 Node,int32 Root)
 {
  while (Node>=0 && Node!=Root) { const auto* N=W->Node(Node); Node=N?N->Parent:-1; }
  return Node==Root;
 }
 bool LegacySource(AS47Web* W,UStaticMesh* Mesh)
 {
  return Mesh && (Mesh->GetPathName().StartsWith(TEXT("/Game/Web/Geo/")) ||
   (Mesh->GetOuter()==W && Mesh->GetName().StartsWith(TEXT("S47LegacyCut"))));
 }
 bool ValidEdit(const FEdit& E,bool Native,FString* Why)
 {
  if (E.Id.IsEmpty() || !E.ThreeBounds.IsValid || E.ThreeBounds.Min.ContainsNaN() || E.ThreeBounds.Max.ContainsNaN() ||
      E.OffsetThree.ContainsNaN() || E.MinTriangles<1 || E.MaxTriangles<E.MinTriangles ||
      E.BoundsToleranceM<0 || E.BoundsToleranceM>.001 || (!E.bRemove && E.OffsetThree.IsNearlyZero()))
   return Fail(Why,TEXT("invalid bounded SARO edit: ")+E.Id);
  if (Native ? E.MaterialSlotNames.IsEmpty() : E.RootPath.IsEmpty() || E.GeometryIds.IsEmpty())
   return Fail(Why,TEXT("SARO edit needs explicit component slots or legacy root/geometries: ")+E.Id);
  return true;
 }
 bool InBox(const FVector& P,const FEdit& E)
 {
  const double T=E.BoundsToleranceM;
  return P.X>=E.ThreeBounds.Min.X-T && P.X<=E.ThreeBounds.Max.X+T &&
         P.Y>=E.ThreeBounds.Min.Y-T && P.Y<=E.ThreeBounds.Max.Y+T &&
         P.Z>=E.ThreeBounds.Min.Z-T && P.Z<=E.ThreeBounds.Max.Z+T;
 }
 bool Prepare(FPlan& Plan,const TArray<FEdit>& Edits,TArray<int32>& Counts,FString* Why)
 {
  UStaticMesh* Source=Plan.Source;
  if (!Source || Source->IsCompiling() || !Source->bAllowCPUAccess || !Source->GetRenderData() || Source->GetRenderData()->LODResources.Num()==0)
   return Fail(Why,TEXT("SARO edit needs ready retained CPU data: ")+(Source?Source->GetPathName():TEXT("missing mesh")));
  const auto& Src=Source->GetRenderData()->LODResources[0]; const auto& B=Src.VertexBuffers;
  const uint32 Vertices=B.PositionVertexBuffer.GetNumVertices(), Colors=B.ColorVertexBuffer.GetNumVertices();
  if (!Vertices || !B.PositionVertexBuffer.GetVertexData() || !B.StaticMeshVertexBuffer.GetTangentData() ||
      !B.StaticMeshVertexBuffer.GetTexCoordData() || !B.StaticMeshVertexBuffer.GetNumTexCoords() ||
      !Src.IndexBuffer.GetIndexDataSize() || (Colors && (Colors!=Vertices || !B.ColorVertexBuffer.GetVertexData())))
   return Fail(Why,TEXT("SARO source CPU buffers are incomplete: ")+Source->GetPathName());
  TArray<uint32> Old; Src.IndexBuffer.GetCopy(Old); Plan.Before=Old.Num()/3;
  if (Old.Num()%3) return Fail(Why,TEXT("SARO source has incomplete triangles"));
  const FTransform Transform=Plan.Component->GetComponentTransform();
  auto Vertex=[&](uint32 I)
  {
   FVertex V; V.Position=B.PositionVertexBuffer.VertexPosition(I);
   V.X=FVector3f(B.StaticMeshVertexBuffer.VertexTangentX(I)); V.Y=FVector3f(B.StaticMeshVertexBuffer.VertexTangentY(I)); V.Z=FVector3f(B.StaticMeshVertexBuffer.VertexTangentZ(I));
   for (uint32 U=0;U<B.StaticMeshVertexBuffer.GetNumTexCoords();++U) V.UV.Add(B.StaticMeshVertexBuffer.GetVertexUV(I,U));
   if (Colors) V.Color=B.ColorVertexBuffer.VertexColor(I); return V;
  };
  for (uint32 I=0;I<Vertices;++I) Plan.Bounds+=FVector(B.PositionVertexBuffer.VertexPosition(I));
  uint32 Read=0;
  for (const auto& Section:Src.Sections)
  {
   if (Section.FirstIndex!=Read || !Source->GetStaticMaterials().IsValidIndex(Section.MaterialIndex))
    return Fail(Why,TEXT("SARO source sections/material slots differ"));
   const FName Slot=Source->GetStaticMaterials()[Section.MaterialIndex].MaterialSlotName;
   FStaticMeshSection Next=Section; Next.FirstIndex=Plan.Indices.Num(); Next.NumTriangles=0;
   for (uint32 I=Section.FirstIndex;I<Section.FirstIndex+Section.NumTriangles*3;I+=3)
   {
    if (I+2>=uint32(Old.Num()) || FMath::Max3(Old[I],Old[I+1],Old[I+2])>=Vertices)
     return Fail(Why,TEXT("SARO source triangle index is out of bounds"));
    const FVector P[]={S47Three::ToThree(Transform.TransformPosition(FVector(B.PositionVertexBuffer.VertexPosition(Old[I])))),
                       S47Three::ToThree(Transform.TransformPosition(FVector(B.PositionVertexBuffer.VertexPosition(Old[I+1])))),
                       S47Three::ToThree(Transform.TransformPosition(FVector(B.PositionVertexBuffer.VertexPosition(Old[I+2]))))};
    int32 Selected=INDEX_NONE;
    for (int32 E:Plan.Edits)
    {
     const FEdit& Edit=Edits[E];
     if ((!Edit.MaterialSlotNames.IsEmpty() && !Edit.MaterialSlotNames.Contains(Slot)) ||
         !InBox(P[0],Edit) || !InBox(P[1],Edit) || !InBox(P[2],Edit)) continue;
     if (Edit.MaxVertexBrightness>=0)
     {
      if (!Colors) return Fail(Why,TEXT("SARO color selector has no retained colors: ")+Edit.Id);
      bool Bright=false;
      for (uint32 K=0;K<3;++K)
      { const FColor C=B.ColorVertexBuffer.VertexColor(Old[I+K]); Bright|=float(FMath::Max3(C.R,C.G,C.B))/255.f>Edit.MaxVertexBrightness; }
      if (Bright) continue;
     }
     if (Selected!=INDEX_NONE) return Fail(Why,TEXT("overlapping SARO edit selectors: ")+Edits[Selected].Id+TEXT(" / ")+Edit.Id);
     Selected=E;
    }
    if (Selected==INDEX_NONE) Plan.Indices.Append({Old[I],Old[I+1],Old[I+2]});
    else
    {
     const FEdit& Edit=Edits[Selected]; ++Counts[Selected];
     if (!Edit.bRemove) for (uint32 K=0;K<3;++K)
     {
      FVertex V=Vertex(Old[I+K]);
      V.Position=FVector3f(Transform.InverseTransformPosition(S47Three::ToUE(P[K]+Edit.OffsetThree)));
      Plan.Bounds+=FVector(V.Position); Plan.Indices.Add(Vertices+Plan.Added.Num()); Plan.Added.Add(MoveTemp(V));
     }
    }
   }
   Read+=Section.NumTriangles*3; Next.NumTriangles=(Plan.Indices.Num()-Next.FirstIndex)/3;
   if (Next.NumTriangles)
   {
    Next.MinVertexIndex=MAX_uint32; Next.MaxVertexIndex=0;
    for (uint32 I=Next.FirstIndex;I<Next.FirstIndex+Next.NumTriangles*3;++I)
    { Next.MinVertexIndex=FMath::Min(Next.MinVertexIndex,Plan.Indices[I]); Next.MaxVertexIndex=FMath::Max(Next.MaxVertexIndex,Plan.Indices[I]); }
    Plan.Sections.Add(Next);
   }
  }
  return Read==uint32(Old.Num()) || Fail(Why,TEXT("SARO source has unaccounted indices"));
 }
 UStaticMesh* MakeCut(AS47Web* W,FPlan& Plan)
 {
  const auto& Src=Plan.Source->GetRenderData()->LODResources[0]; const auto& B=Src.VertexBuffers;
  const uint32 Original=B.PositionVertexBuffer.GetNumVertices(), Count=Original+Plan.Added.Num();
  UStaticMesh* Cut=NewObject<UStaticMesh>(W,MakeUniqueObjectName(W,UStaticMesh::StaticClass(),FName(TEXT("S47LegacyCut"))),RF_Transient);
  Cut->bAllowCPUAccess=true; Cut->NeverStream=true; Cut->bSupportRayTracing=Plan.Source->bSupportRayTracing;
  Cut->GetStaticMaterials()=Plan.Source->GetStaticMaterials();
  // Visual replacement only. Retain the original cooked query/physics body;
  // walking rectangles and interaction proxies are not rebuilt by this helper.
  Cut->SetBodySetup(Plan.Source->GetBodySetup());
  auto Data=MakeUnique<FStaticMeshRenderData>(); Data->AllocateLODResources(1);
  Data->Bounds=FBoxSphereBounds(Plan.Bounds); Data->ScreenSize[0].Default=1;
  Data->NumInlinedLODs=1; Data->CurrentFirstLODIdx=0; Data->bReadyForStreaming=false;
  auto& Dst=Data->LODResources[0]; Dst.Sections.Append(Plan.Sections);
  Dst.VertexBuffers.PositionVertexBuffer.Init(Count,true);
  auto& T=Dst.VertexBuffers.StaticMeshVertexBuffer;
  T.SetUseHighPrecisionTangentBasis(B.StaticMeshVertexBuffer.GetUseHighPrecisionTangentBasis()); T.SetUseFullPrecisionUVs(B.StaticMeshVertexBuffer.GetUseFullPrecisionUVs());
  T.Init(Count,B.StaticMeshVertexBuffer.GetNumTexCoords(),true);
  const bool Colors=B.ColorVertexBuffer.GetNumVertices()>0;
  if (Colors) Dst.VertexBuffers.ColorVertexBuffer.Init(Count,true);
  for (uint32 I=0;I<Count;++I)
  {
   const FVertex* V=I>=Original?&Plan.Added[I-Original]:nullptr;
   Dst.VertexBuffers.PositionVertexBuffer.VertexPosition(I)=V?V->Position:B.PositionVertexBuffer.VertexPosition(I);
   T.SetVertexTangents(I,V?V->X:FVector3f(B.StaticMeshVertexBuffer.VertexTangentX(I)),V?V->Y:FVector3f(B.StaticMeshVertexBuffer.VertexTangentY(I)),V?V->Z:FVector3f(B.StaticMeshVertexBuffer.VertexTangentZ(I)));
   for (uint32 U=0;U<T.GetNumTexCoords();++U) T.SetVertexUV(I,U,V?V->UV[U]:B.StaticMeshVertexBuffer.GetVertexUV(I,U));
   if (Colors) Dst.VertexBuffers.ColorVertexBuffer.VertexColor(I)=V?V->Color:B.ColorVertexBuffer.VertexColor(I);
  }
  Dst.bHasColorVertexData=Colors; Dst.bBuffersInlined=true; Dst.bIsOptionalLOD=false; Dst.SourceMeshBounds=Data->Bounds; Dst.MaxDeviation=0;
  Dst.IndexBuffer.TrySetAllowCPUAccess(true); Dst.IndexBuffer.SetIndices(Plan.Indices,EIndexBufferStride::Force32Bit);
  bool RT=Cut->bSupportRayTracing;
#if !WITH_EDITOR
  RT&=IsRayTracingAllowed();
#endif
  if (RT && !Plan.Indices.IsEmpty()) Data->InitializeRayTracingRepresentationFromRenderingLODs();
  Cut->SetRenderData(MoveTemp(Data)); Cut->SetExtendedBounds(FBoxSphereBounds(Plan.Bounds)); return Cut;
 }
 bool Run(AS47Web* W,UStaticMeshComponent* Explicit,const TArray<FEdit>& Input,FString* Why)
 {
  if (!W || !W->IsLoaded()) return Fail(Why,TEXT("SARO world is not loaded"));
  FState& State=States.FindOrAdd(W); TArray<FEdit> Edits;
  for (const FEdit& E:Input) if (!State.Applied.Contains(E.Id))
  {
   if (!ValidEdit(E,Explicit!=nullptr,Why)) return false;
   if (Edits.ContainsByPredicate([&](const FEdit& Other){return Other.Id==E.Id;}))
    return Fail(Why,TEXT("duplicate SARO edit id: ")+E.Id);
   Edits.Add(E);
  }
  if (Edits.IsEmpty()) return true;
  TArray<FPlan> Plans;
  auto Add=[&](UStaticMeshComponent* Component,int32 E)
  {
   FPlan* P=Plans.FindByPredicate([&](const FPlan& Plan){return Plan.Component==Component;});
   if (!P)
   { FPlan Plan; Plan.Component=Component; Plan.Source=Component->GetStaticMesh(); Plan.Materials=Component->GetMaterials(); Plans.Add(MoveTemp(Plan)); P=&Plans.Last(); }
   P->Edits.AddUnique(E);
  };
  for (int32 E=0;E<Edits.Num();++E)
  {
   if (Explicit) Add(Explicit,E);
   else
   {
    const int32 Root=W->Handle(Edits[E].RootPath);
    if (Root<0) return Fail(Why,TEXT("SARO root absent: ")+Edits[E].RootPath);
    for (int32 I=0;I<W->PartCount();++I)
    {
     const auto* N=W->Node(I);
     if (!N || !Edits[E].GeometryIds.Contains(N->Geo) || !Under(W,I,Root)) continue;
     UStaticMeshComponent* Component=Cast<UStaticMeshComponent>(N->Comp.Get());
     if (!Component || !LegacySource(W,Component->GetStaticMesh())) return Fail(Why,TEXT("SARO selector met a nonlegacy render component: ")+Edits[E].Id);
     Add(Component,E);
    }
   }
  }
  TArray<int32> Counts; Counts.Init(0,Edits.Num());
  for (FPlan& Plan:Plans) if (!Prepare(Plan,Edits,Counts,Why)) return false;
  for (int32 E=0;E<Edits.Num();++E) if (Counts[E]<Edits[E].MinTriangles || Counts[E]>Edits[E].MaxTriangles)
   return Fail(Why,FString::Printf(TEXT("SARO edit %s selected %d triangles, expected %d..%d; no geometry changed"),*Edits[E].Id,Counts[E],Edits[E].MinTriangles,Edits[E].MaxTriangles));
  for (FPlan& Plan:Plans) Plan.Cut=MakeCut(W,Plan);
  for (FPlan& Plan:Plans)
  {
   const ECollisionEnabled::Type Collision=Plan.Component->GetCollisionEnabled();
   Plan.Cut->InitResources(); Plan.Component->SetStaticMesh(Plan.Cut);
   for (int32 I=0;I<Plan.Materials.Num();++I) if (Plan.Materials[I]) Plan.Component->SetMaterial(I,Plan.Materials[I]);
   Plan.Component->SetCollisionEnabled(Collision);
  }
  for (int32 E=0;E<Edits.Num();++E)
  {
   State.Applied.Add(Edits[E].Id);
   UE_LOG(LogTemp,Display,TEXT("S47: SARO legacy %s: %s %d triangles; original UV, colors, slots and query body retained"),*Edits[E].Id,Edits[E].bRemove?TEXT("removed"):TEXT("relocated"),Counts[E]);
  }
  return true;
 }
}

bool Apply(AS47Web* W,const TArray<FEdit>& Edits,FString* Why) { return Run(W,nullptr,Edits,Why); }
bool ApplyToComponent(AS47Web* W,UStaticMeshComponent* Component,const TArray<FEdit>& Edits,FString* Why)
{
 if (!Component || Component->GetOwner()!=W) return Fail(Why,TEXT("SARO explicit component is absent or belongs to another actor"));
 return Run(W,Component,Edits,Why);
}

bool Build(AS47Web* W)
{
 if (FParse::Param(FCommandLine::Get(),TEXT("S47Saro1986Off"))) return true;
 if (!W || !W->IsLoaded()) return false;
 if (States.FindOrAdd(W).bBuilt) return true;
 const int32 Annex=W->Handle(TEXT("annex.group")); if (Annex<0) return false;
 // The complete board/back and five original prints are distinct legacy nodes.
 // Move those nodes, including their original materials, rather than retexture.
 TArray<int32> Board;
 for (int32 Geo=354;Geo<=359;++Geo)
 {
  int32 Found=INDEX_NONE;
  for (int32 I=0;I<W->PartCount();++I) if (const auto* N=W->Node(I); N && N->Geo==Geo && Under(W,I,Annex))
  {
   auto* C=Cast<UStaticMeshComponent>(N->Comp.Get());
   if (Found!=INDEX_NONE || !C || !LegacySource(W,C->GetStaticMesh())) return false;
   Found=I;
  }
  if (Found==INDEX_NONE) return false; Board.Add(Found);
 }
 int32 Exit=INDEX_NONE;
 for (int32 I=0;I<W->PartCount();++I) if (const auto* N=W->Node(I); N && N->Geo==318 && Under(W,I,Annex))
 { if (Exit!=INDEX_NONE) return false; Exit=I; }
 if (Exit==INDEX_NONE) return false;
 TArray<FEdit> Edits;
 auto Add=[&](const TCHAR* Id,const TCHAR* Root,int32 Geo,const FVector& Min,const FVector& Max,int32 Count,const FVector& Offset=FVector::ZeroVector)
 {
  FEdit E; E.Id=Id; E.RootPath=Root; E.GeometryIds={Geo}; E.ThreeBounds=FBox(Min,Max);
  E.MinTriangles=E.MaxTriangles=Count; E.OffsetThree=Offset; E.bRemove=Offset.IsNearlyZero(); Edits.Add(MoveTemp(E));
 };
 Add(TEXT("decorative_hall_coat"),TEXT("annex.group"),376,{.50,.86,4.795},{1.10,1.83,5.08},214);
 Add(TEXT("decorative_control_cardigan"),TEXT("room.group"),219,{-.30,.39,-2.90},{.45,1.13,-2.45},140);
 Add(TEXT("quiet_poster_mount"),TEXT("annex.group"),376,{-4.63,1.24,4.79},{-4.17,1.86,4.825},26,{0,0,.028});
 Add(TEXT("aerial_frame_mount"),TEXT("annex.group"),376,{-4.034,1.400,4.80},{-3.466,1.800,4.833},50,{0,0,.032});
 Add(TEXT("fire_card_mount"),TEXT("annex.group"),376,{-5.335,1.35,4.80},{-5.165,1.556,4.816},26,{0,0,.028});
 Add(TEXT("safety_poster_mount"),TEXT("annex.group"),376,{-2.51,1.18,6.585},{-2.09,1.74,6.603},26,{0,0,-.017});
 Add(TEXT("control_switch_mount"),TEXT("annex.group"),376,{-3.119,1.189,4.80},{-3.041,1.311,4.813},12,{0,0,.032});
 FString Why;
 if (!Apply(W,Edits,&Why))
 {
  FState& State=States.FindOrAdd(W);
  if (State.Failure!=Why) { State.Failure=Why; UE_LOG(LogTemp,Warning,TEXT("S47: SARO legacy cleanup awaiting valid source: %s"),*Why); }
  return false;
 }
 auto Move=[&](int32 I,const FVector& Offset)
 {
  const auto* N=W->Node(I); const FTransform Parent=N->Parent>=0?W->NodeWorldThree(N->Parent):FTransform::Identity;
  const FVector Position=N->P+Parent.InverseTransformVector(Offset); W->SetPosition(I,Position);
 };
 for (int32 I:Board) Move(I,{0,0,.022});
 Move(Exit,{.006,0,0});
 States.FindOrAdd(W).bBuilt=true;
 UE_LOG(LogTemp,Display,TEXT("S47: SARO original board plus five prints mounted on paint, battery EXIT retained; decorative clothing removed, interactive jacket unchanged"));
 return true;
}
}
