#include "S47OfficeWindow.h"

#include "S47Three.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RenderUtils.h"
#include "StaticMeshResources.h"

namespace S47OfficeWindow
{
namespace
{
 constexpr double Left=-.55, Right=1.30, Bottom=1.08, Top=2.40;
 // Directly measured in the immutable exported g313/g342. The painted face
 // sits 2 mm in front of the concrete's inside face; all three must be opened.
 const double Planes[]={10.498,10.500,10.800};
 struct FVertex
 {
  FVector World; FVector3f Position, X, Y, Z; TArray<FVector2f> UV; FColor Color=FColor::White;
 };
 struct FPlan
 {
  UStaticMeshComponent* Component=nullptr; UStaticMesh* Source=nullptr; UStaticMesh* Cut=nullptr;
  TArray<FVertex> Added; TArray<uint32> Indices; TArray<FStaticMeshSection> Sections;
  int32 Counts[3]={0,0,0}; int32 BeforeTriangles=0;
 };
 struct FPart
 {
  TWeakObjectPtr<UStaticMeshComponent> Component; TWeakObjectPtr<UStaticMesh> Cut;
  TWeakObjectPtr<USceneComponent> Parent; FTransform Transform;
  TArray<TWeakObjectPtr<UMaterialInterface>> Materials;
  uint32 IndexCount=0, VertexCount=0;
 };
 struct FInstalled
 {
  TArray<FPart> Parts; TWeakObjectPtr<UStaticMeshComponent> Glass;
  TWeakObjectPtr<UMaterialInstanceDynamic> GlassMaterial;
  TWeakObjectPtr<USceneComponent> Parent; FTransform GlassTransform;
 };
 TMap<TWeakObjectPtr<AS47Web>,FInstalled> Installed;
 bool Fail(FString& Why,const FString& Message) { Why=Message; return false; }
 bool Near(double A,double B) { return FMath::Abs(A-B)<.0002; }
 bool SourceCorner(const FVector& P)
 { return (Near(P.X,-6.3)||Near(P.X,2.3)) && (Near(P.Y,0)||Near(P.Y,2.8)); }
 double Area(const TArray<FVertex>& P)
 {
  double A=0;
  for (int32 I=1;I+1<P.Num();++I) A+=FVector::CrossProduct(P[I].World-P[0].World,P[I+1].World-P[0].World).Size()*.5;
  return A;
 }
 FVertex Blend(const FVertex& A,const FVertex& B,double T)
 {
  FVertex R; R.World=FMath::Lerp(A.World,B.World,T);
  const float F=float(T);
  R.Position=FMath::Lerp(A.Position,B.Position,F);
  R.X=FMath::Lerp(A.X,B.X,F).GetSafeNormal(); R.Y=FMath::Lerp(A.Y,B.Y,F).GetSafeNormal(); R.Z=FMath::Lerp(A.Z,B.Z,F).GetSafeNormal();
  for (int32 I=0;I<A.UV.Num();++I) R.UV.Add(FMath::Lerp(A.UV[I],B.UV[I],F));
  auto Channel=[T](uint8 A0,uint8 B0) { return uint8(FMath::Clamp(FMath::RoundToInt(FMath::Lerp(double(A0),double(B0),T)),0,255)); };
  R.Color=FColor(Channel(A.Color.R,B.Color.R),Channel(A.Color.G,B.Color.G),Channel(A.Color.B,B.Color.B),Channel(A.Color.A,B.Color.A));
  return R;
 }
 // Bounded planar clipping, used only for the six measured rear-wall triangles.
 // Axis 0/1 denotes Three X/height. Each emitted polygon keeps original winding.
 TArray<FVertex> Clip(const TArray<FVertex>& P,int32 Axis,double Edge,bool Greater)
 {
  TArray<FVertex> Out; if (P.IsEmpty()) return Out;
  auto Distance=[&](const FVertex& V) { return (V.World[Axis]-Edge)*(Greater?1.:-1.); };
  FVertex A=P.Last(); double DA=Distance(A);
  for (const FVertex& B:P)
  {
   const double DB=Distance(B);
   if ((DA>=0)!=(DB>=0))
   { FVertex V=Blend(A,B,DA/(DA-DB)); V.World[Axis]=Edge; Out.Add(MoveTemp(V)); }
   if (DB>=0) Out.Add(B);
   A=B; DA=DB;
  }
  return Out;
 }
 TArray<FVertex> Hole(const TArray<FVertex>& P)
 { return Clip(Clip(Clip(Clip(P,0,Left,true),0,Right,false),1,Bottom,true),1,Top,false); }
 void Emit(FPlan& Plan,const TArray<FVertex>& P,uint32 OriginalVertices)
 {
  for (int32 I=1;I+1<P.Num();++I)
  {
   if (FVector::CrossProduct(P[I].World-P[0].World,P[I+1].World-P[0].World).SizeSquared()<1.e-16) continue;
   for (int32 K:{0,I,I+1})
   {
    FVertex V=P[K];
    // Snap the newly cut position through the double-precision world plane,
    // instead of letting float lerp leave a thin strip inside the aperture.
    V.Position=FVector3f(Plan.Component->GetComponentTransform().InverseTransformPosition(S47Three::ToUE(V.World)));
    Plan.Indices.Add(OriginalVertices+Plan.Added.Num()); Plan.Added.Add(MoveTemp(V));
   }
  }
 }
 bool Prepare(AS47Web* W,UStaticMeshComponent* Comp,int32 Geo,FPlan& Plan,FString& Why)
 {
  UStaticMesh* Source=Comp?Comp->GetStaticMesh():nullptr;
  if (!Source || !Source->bAllowCPUAccess || !Source->GetRenderData() || Source->GetRenderData()->LODResources.Num()!=1)
   return Fail(Why,FString::Printf(TEXT("g%d needs one original LOD with retained CPU buffers"),Geo));
  const auto& Src=Source->GetRenderData()->LODResources[0]; const auto& B=Src.VertexBuffers;
  const uint32 Count=B.PositionVertexBuffer.GetNumVertices(), Colors=B.ColorVertexBuffer.GetNumVertices();
  if (!Count || !B.PositionVertexBuffer.GetVertexData() || !B.StaticMeshVertexBuffer.GetTangentData() ||
      !B.StaticMeshVertexBuffer.GetTexCoordData() || !B.StaticMeshVertexBuffer.GetNumTexCoords() ||
      Src.IndexBuffer.GetIndexDataSize()==0 || (Colors && (Colors!=Count || !B.ColorVertexBuffer.GetVertexData())))
   return Fail(Why,FString::Printf(TEXT("g%d rear-wall CPU data missing: position=%d tangent=%d UV=%d color=%d indexBytes=%u"),Geo,
    B.PositionVertexBuffer.GetVertexData()!=nullptr,B.StaticMeshVertexBuffer.GetTangentData()!=nullptr,
    B.StaticMeshVertexBuffer.GetTexCoordData()!=nullptr,!Colors||B.ColorVertexBuffer.GetVertexData()!=nullptr,
    Src.IndexBuffer.GetIndexDataSize()));
  TArray<uint32> Old; Src.IndexBuffer.GetCopy(Old);
  if (Old.Num()%3) return Fail(Why,TEXT("rear-wall source has an incomplete triangle"));
  Plan.Component=Comp; Plan.Source=Source; Plan.BeforeTriangles=Old.Num()/3;
  auto Vertex=[&](uint32 Id)
  {
   FVertex V; V.Position=B.PositionVertexBuffer.VertexPosition(Id);
   V.World=S47Three::ToThree(Comp->GetComponentTransform().TransformPosition(FVector(V.Position)));
   V.X=FVector3f(B.StaticMeshVertexBuffer.VertexTangentX(Id)); V.Y=B.StaticMeshVertexBuffer.VertexTangentY(Id); V.Z=FVector3f(B.StaticMeshVertexBuffer.VertexTangentZ(Id));
   for (uint32 U=0;U<B.StaticMeshVertexBuffer.GetNumTexCoords();++U) V.UV.Add(B.StaticMeshVertexBuffer.GetVertexUV(Id,U));
   if (Colors) V.Color=B.ColorVertexBuffer.VertexColor(Id); return V;
  };
  double Removed[3]={0,0,0}; uint32 Read=0;
  for (const auto& Section:Src.Sections)
  {
   if (Section.FirstIndex!=Read) return Fail(Why,TEXT("rear-wall source section indices are not contiguous"));
   FStaticMeshSection Copy=Section; Copy.FirstIndex=Plan.Indices.Num(); Copy.NumTriangles=0;
   for (uint32 I=Section.FirstIndex;I<Section.FirstIndex+Section.NumTriangles*3;I+=3)
   {
    if (I+2>=uint32(Old.Num()) || FMath::Max3(Old[I],Old[I+1],Old[I+2])>=Count)
     return Fail(Why,TEXT("rear-wall source triangle index is out of bounds"));
    TArray<FVertex> P={Vertex(Old[I]),Vertex(Old[I+1]),Vertex(Old[I+2])};
    int32 Plane=-1;
    for (int32 K=0;K<3;++K) if (Near(P[0].World.Z,Planes[K])&&Near(P[1].World.Z,Planes[K])&&Near(P[2].World.Z,Planes[K])) Plane=K;
    const double OpeningArea=Plane>=0?Area(Hole(P)):0;
    const bool FullRearFace=Plane>=0 && SourceCorner(P[0].World)&&SourceCorner(P[1].World)&&SourceCorner(P[2].World) && FMath::IsNearlyEqual(Area(P),12.04,.001);
    if (FullRearFace)
    {
     if ((Geo==313 && Plane==0)||(Geo==342 && Plane!=0))
      return Fail(Why,TEXT("rear-wall material/plane identity differs; no geometry changed"));
     ++Plan.Counts[Plane]; Removed[Plane]+=OpeningArea;
    }
    if (OpeningArea>1.e-8)
    {
     if (!FullRearFace)
      return Fail(Why,TEXT("unexpected overlapping rear-wall triangle; no geometry changed"));
     // Four disjoint strips form triangle minus opening. No broad triangle is
     // simply discarded, so the adjoining archive and the exterior stay intact.
     Emit(Plan,Clip(P,0,Left,false),Count); P=Clip(P,0,Left,true);
     Emit(Plan,Clip(P,0,Right,true),Count); P=Clip(P,0,Right,false);
     Emit(Plan,Clip(P,1,Bottom,false),Count); P=Clip(P,1,Bottom,true);
     Emit(Plan,Clip(P,1,Top,true),Count);
    }
    else Plan.Indices.Append({Old[I],Old[I+1],Old[I+2]});
   }
   Read+=Section.NumTriangles*3; Copy.NumTriangles=(Plan.Indices.Num()-Copy.FirstIndex)/3;
   if (Copy.NumTriangles)
   {
    Copy.MinVertexIndex=MAX_uint32; Copy.MaxVertexIndex=0;
    for (uint32 I=Copy.FirstIndex;I<Copy.FirstIndex+Copy.NumTriangles*3;++I)
    { Copy.MinVertexIndex=FMath::Min(Copy.MinVertexIndex,Plan.Indices[I]); Copy.MaxVertexIndex=FMath::Max(Copy.MaxVertexIndex,Plan.Indices[I]); }
    Plan.Sections.Add(Copy);
   }
  }
  if (Read!=uint32(Old.Num())) return Fail(Why,TEXT("rear-wall source has unaccounted section indices"));
  for (int32 K=0;K<3;++K)
  {
   const int32 Wanted=Geo==313?(K==0?0:2):(K==0?2:0);
   if (Plan.Counts[K]!=Wanted || (Wanted && !FMath::IsNearlyEqual(Removed[K],(Right-Left)*(Top-Bottom),.0001)))
    return Fail(Why,FString::Printf(TEXT("g%d rear plane %.3f has %d triangles, expected %d, or opening area differs"),Geo,Planes[K],Plan.Counts[K],Wanted));
  }
  return true;
 }
 UStaticMesh* MakeCut(AS47Web* W,FPlan& Plan)
 {
  const auto& Src=Plan.Source->GetRenderData()->LODResources[0]; const auto& B=Src.VertexBuffers;
  const uint32 OldCount=B.PositionVertexBuffer.GetNumVertices(), Count=OldCount+Plan.Added.Num();
  UStaticMesh* Cut=NewObject<UStaticMesh>(W,NAME_None,RF_Transient);
  Cut->bAllowCPUAccess=true; Cut->NeverStream=true; Cut->GetStaticMaterials()=Plan.Source->GetStaticMaterials(); Cut->bSupportRayTracing=true;
  auto Data=MakeUnique<FStaticMeshRenderData>(); Data->AllocateLODResources(1);
  Data->Bounds=Plan.Source->GetRenderData()->Bounds; Data->ScreenSize[0].Default=1;
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
   const FVertex* V=I>=OldCount?&Plan.Added[I-OldCount]:nullptr;
   Dst.VertexBuffers.PositionVertexBuffer.VertexPosition(I)=V?V->Position:B.PositionVertexBuffer.VertexPosition(I);
   T.SetVertexTangents(I,V?V->X:FVector3f(B.StaticMeshVertexBuffer.VertexTangentX(I)),V?V->Y:B.StaticMeshVertexBuffer.VertexTangentY(I),V?V->Z:FVector3f(B.StaticMeshVertexBuffer.VertexTangentZ(I)));
   for (uint32 U=0;U<T.GetNumTexCoords();++U) T.SetVertexUV(I,U,V?V->UV[U]:B.StaticMeshVertexBuffer.GetVertexUV(I,U));
   if (Colors) Dst.VertexBuffers.ColorVertexBuffer.VertexColor(I)=V?V->Color:B.ColorVertexBuffer.VertexColor(I);
  }
  Dst.bHasColorVertexData=Colors; Dst.bBuffersInlined=true; Dst.bIsOptionalLOD=false; Dst.SourceMeshBounds=Data->Bounds; Dst.MaxDeviation=0;
  Dst.IndexBuffer.TrySetAllowCPUAccess(true);
  Dst.IndexBuffer.SetIndices(Plan.Indices,EIndexBufferStride::Force32Bit);
  bool RT=true;
#if !WITH_EDITOR
  RT=IsRayTracingAllowed();
#endif
  if (RT) Data->InitializeRayTracingRepresentationFromRenderingLODs();
  Cut->SetRenderData(MoveTemp(Data)); Cut->SetExtendedBounds(Plan.Source->GetBounds()); return Cut;
 }
}

bool Build(AS47Web* W)
{
 if (!W || !W->IsLoaded()) return false;
 if (Installed.Contains(W)) { TArray<FString> Lines; return Audit(W,Lines); }
 const FS47Obj Annex=W->Obj(TEXT("annex.group")), Root=Annex?Annex.Find(TEXT("operations:room")):FS47Obj();
 const auto* Parent=Root?W->Node(Root.I):nullptr;
 if (!Annex || !Parent || !Parent->Comp.IsValid() || !Parent->Comp->GetComponentTransform().Equals(FTransform::Identity,.001)) return false;
 USceneComponent* ParentComponent=Parent->Comp.Get();
 TArray<FPlan> Plans; FString Why;
 for (int32 Geo:{313,342})
 {
  UStaticMeshComponent* Found=nullptr; int32 Matches=0;
  for (int32 I=0;I<W->PartCount();++I)
  {
   const auto* N=W->Node(I); if (!N || N->Geo!=Geo) continue;
   int32 Ancestor=I;
   while (Ancestor>=0 && Ancestor!=Annex.I) { const auto* A=W->Node(Ancestor); Ancestor=A?A->Parent:-1; }
   if (Ancestor==Annex.I) { Found=Cast<UStaticMeshComponent>(N->Comp.Get()); ++Matches; }
  }
  FPlan Plan;
  if (Matches!=1 || !Prepare(W,Found,Geo,Plan,Why))
  { UE_LOG(LogTemp,Warning,TEXT("S47: Office window skipped before mutation: %s"),Matches!=1?TEXT("original Annex mesh identity differs"):*Why); return false; }
  Plans.Add(MoveTemp(Plan));
 }
 UStaticMesh* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
 UMaterialInterface* GlassBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Trans_2S.M_Web_Lit_Trans_2S"));
 if (!Plane || !Plane->GetBoundingBox().Min.Equals(FVector(-50,-50,0),.001) || !Plane->GetBoundingBox().Max.Equals(FVector(50,50,0),.001) ||
     !GlassBase || GlassBase->GetBlendMode()!=BLEND_Translucent || !GlassBase->IsTwoSided())
 { UE_LOG(LogTemp,Warning,TEXT("S47: Office window skipped before mutation: native translucent two-sided glass unavailable")); return false; }
 for (auto& Plan:Plans) Plan.Cut=MakeCut(W,Plan);
 // Every source face and replacement is prepared before touching either mesh.
 FInstalled Result; Result.Parent=ParentComponent;
 for (auto& Plan:Plans)
 {
  FPart Part; Part.Component=Plan.Component; Part.Cut=Plan.Cut; Part.Parent=Plan.Component->GetAttachParent(); Part.Transform=Plan.Component->GetComponentTransform();
  for (auto* Material:Plan.Component->GetMaterials()) Part.Materials.Add(Material);
  Part.IndexCount=Plan.Indices.Num(); Part.VertexCount=Plan.Cut->GetRenderData()->LODResources[0].VertexBuffers.PositionVertexBuffer.GetNumVertices();
  Plan.Cut->InitResources(); Plan.Component->SetStaticMesh(Plan.Cut);
  for (int32 I=0;I<Part.Materials.Num();++I) if (Part.Materials[I].IsValid()) Plan.Component->SetMaterial(I,Part.Materials[I].Get());
  Result.Parts.Add(Part);
  UE_LOG(LogTemp,Display,TEXT("S47: Office window rear source %s: %d to %d triangles; original plane faces %d/%d/%d; original remaining vertices, UVs, tangents, colors and material slots retained"),*Plan.Source->GetName(),Plan.BeforeTriangles,Plan.Indices.Num()/3,Plan.Counts[0],Plan.Counts[1],Plan.Counts[2]);
 }
 UMaterialInstanceDynamic* Material=UMaterialInstanceDynamic::Create(GlassBase,W);
 Material->SetVectorParameterValue(TEXT("Color"),FLinearColor(.64f,.70f,.67f)); Material->SetScalarParameterValue(TEXT("Opacity"),.035f);
 Material->SetScalarParameterValue(TEXT("Roughness"),.07f); Material->SetScalarParameterValue(TEXT("Metalness"),0); Material->SetScalarParameterValue(TEXT("Specular"),.5f);
 Material->SetScalarParameterValue(TEXT("UseMap"),0); Material->SetScalarParameterValue(TEXT("EmissivePower"),0); Material->SetVectorParameterValue(TEXT("Emissive"),FLinearColor::Black);
 UStaticMeshComponent* Glass=NewObject<UStaticMeshComponent>(W,TEXT("OperationsRearWindowGlass")); Glass->SetupAttachment(ParentComponent); Glass->SetMobility(EComponentMobility::Movable);
 Glass->SetStaticMesh(Plane); Glass->SetMaterial(0,Material); Glass->SetCollisionEnabled(ECollisionEnabled::NoCollision); Glass->SetCastShadow(false);
 Glass->SetRelativeTransform(FTransform(FRotationMatrix::MakeFromXY(FVector::XAxisVector,FVector::UpVector).ToQuat(),S47Three::ToUE(FVector((Left+Right)/2,(Bottom+Top)/2,10.65)),FVector(Right-Left,Top-Bottom,1)));
 Glass->RegisterComponent(); W->AdoptNode(Glass,Root.I,TEXT("office:rearWindowGlass"));
 Result.Glass=Glass; Result.GlassMaterial=Material; Result.GlassTransform=Glass->GetComponentTransform(); Installed.Add(W,Result);
 UE_LOG(LogTemp,Display,TEXT("S47: Office window opened x [-0.550, 1.300], height [1.080, 2.400], rear planes 10.498/10.500/10.800; no navigation or power change"));
 TArray<FString> Lines; const bool Good=Audit(W,Lines); for (const auto& Line:Lines) UE_LOG(LogTemp,Display,TEXT("S47: %s"),*Line); return Good;
}

bool Audit(AS47Web* W,TArray<FString>& Lines)
{
 const FInstalled* Found=Installed.Find(W);
 if (!Found) { Lines.Add(TEXT("Office window unavailable: bounded rear-wall patch was not installed")); return false; }
 bool Good=Found->Parts.Num()==2; int32 OpenFaces=0;
 for (const auto& Part:Found->Parts)
 {
  const auto* C=Part.Component.Get(); const auto* M=Part.Cut.Get();
  bool On=C && M && C->GetStaticMesh()==M && C->GetAttachParent()==Part.Parent.Get() && C->GetComponentTransform().Equals(Part.Transform,.001) &&
   M->bSupportRayTracing && M->bAllowCPUAccess && M->GetRenderData() && M->GetRenderData()->LODResources.Num()==1;
  if (On)
  {
   const auto& L=M->GetRenderData()->LODResources[0]; TArray<uint32> Indices; L.IndexBuffer.GetCopy(Indices);
   On=uint32(Indices.Num())==Part.IndexCount && L.VertexBuffers.PositionVertexBuffer.GetNumVertices()==Part.VertexCount && C->GetNumMaterials()==Part.Materials.Num();
   for (int32 I=0;I<Part.Materials.Num();++I) On &= C->GetMaterial(I)==Part.Materials[I].Get();
   for (int32 I=0;I+2<Indices.Num();I+=3)
   {
    TArray<FVertex> P;
    for (int32 K=0;K<3;++K)
    {
     if (Indices[I+K]>=Part.VertexCount) { On=false; break; }
     FVertex V; V.World=S47Three::ToThree(C->GetComponentTransform().TransformPosition(FVector(L.VertexBuffers.PositionVertexBuffer.VertexPosition(Indices[I+K])))); P.Add(V);
    }
    if (P.Num()!=3) break;
    for (double Z:Planes) if (Near(P[0].World.Z,Z)&&Near(P[1].World.Z,Z)&&Near(P[2].World.Z,Z) && Area(Hole(P))>1.e-7) ++OpenFaces;
   }
  }
  Good &= On;
 }
 const auto* Glass=Found->Glass.Get(); const auto* Material=Found->GlassMaterial.Get(); float Opacity=0,Emissive=1;
 const bool Pane=Glass && Material && Glass->GetAttachParent()==Found->Parent.Get() && Glass->GetComponentTransform().Equals(Found->GlassTransform,.001) &&
  Glass->GetMaterial(0)==Material && Material->GetBlendMode()==BLEND_Translucent && Material->IsTwoSided() &&
  Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Opacity")),Opacity) && FMath::IsNearlyEqual(Opacity,.035f) &&
  Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("EmissivePower")),Emissive) && FMath::IsNearlyZero(Emissive) &&
  Glass->GetCollisionEnabled()==ECollisionEnabled::NoCollision && !Glass->CastShadow;
 Good &= Pane && OpenFaces==0;
 Lines.Add(FString::Printf(TEXT("Office window rear cut %s: two actual rear components retain bindings/transforms; %d triangles cover opening; passive translucent pane %s; authored jambs/blind/daylight require separate visual inspection"),Good?TEXT("PASS"):TEXT("FAIL"),OpenFaces,Pane?TEXT("present"):TEXT("missing or mismatched")));
 return Good;
}
}
