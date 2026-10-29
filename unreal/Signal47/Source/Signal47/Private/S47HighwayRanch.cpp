// Share only the existing ranch triangles across the ordinary road/SARO handoff.
#include "S47Web.h"
#include "S47Geo.h"
#include "S47Three.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/SecureHash.h"
#include "RenderUtils.h"
#include "StaticMeshResources.h"
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47HighwayRanch,Log,All);
namespace
{
 struct FExpected { int32 Node,Geo,Material,Total,Farm; };
 const FExpected Expected[]={{1301,910,590,204,12},{1302,911,589,3708,248},{1303,912,587,360,40}};
 const FVector RoadOrigin(8000,0,0);
 const FVector SaroToRoad(8000+S47Geo::SARO_IN_ROAD.X,0,S47Geo::SARO_IN_ROAD.Z);
 struct FSelection
 {
  TArray<uint32> Indices;
  TArray<FStaticMeshSection> Sections;
  FBox Bounds=FBox(ForceInit);
 };
 struct FPart
 {
  FExpected Source{};
  TStrongObjectPtr<UStaticMesh> Original;
  TWeakObjectPtr<UStaticMesh> RestMesh,FarmMesh;
  TWeakObjectPtr<UStaticMeshComponent> Rest,Farm;
  int32 FarmNode=INDEX_NONE;
  FTransform OriginalTransform;
  ECollisionEnabled::Type Collision=ECollisionEnabled::NoCollision;
  FCollisionResponseContainer Responses;
  bool Hidden=false,Shadow=false,RayVisible=false,AuthoredVisible=false;
  FSelection FarmSelection,RestSelection;
  FString VertexHash,OriginalHash,RestHash,FarmHash;
  TArray<uint32> RenderFlags;
 };
 TArray<uint32> RenderFlags(const UStaticMeshComponent* C)
 {
  if(!C)return {};
  return {uint32(C->bUseAsOccluder),uint32(C->bRenderInMainPass),uint32(C->bRenderInDepthPass),uint32(C->bReceivesDecals),
   uint32(C->bVisibleInReflectionCaptures),uint32(C->bVisibleInRealTimeSkyCaptures),uint32(C->bVisibleInRayTracing),
   uint32(C->CastShadow),uint32(C->bCastDynamicShadow),uint32(C->bCastStaticShadow),uint32(C->bCastContactShadow),
   uint32(C->bCastHiddenShadow),uint32(C->bCastShadowAsTwoSided),uint32(C->bEmissiveLightSource),
   uint32(C->bAffectDynamicIndirectLighting),uint32(C->bAffectIndirectLightingWhileHidden),uint32(C->bAffectDistanceFieldLighting),
   uint32(C->bRenderCustomDepth),uint32(C->CustomDepthStencilValue),uint32(C->CustomDepthStencilWriteMask),
   uint32(C->bOwnerNoSee),uint32(C->bOnlyOwnerSee),uint32(C->bUseAttachParentBound)};
 }
 void CopyRenderFlags(const UStaticMeshComponent* S,UStaticMeshComponent* D)
 {
  D->bUseAsOccluder=S->bUseAsOccluder;D->bRenderInMainPass=S->bRenderInMainPass;D->bRenderInDepthPass=S->bRenderInDepthPass;D->bReceivesDecals=S->bReceivesDecals;
  D->bVisibleInReflectionCaptures=S->bVisibleInReflectionCaptures;D->bVisibleInRealTimeSkyCaptures=S->bVisibleInRealTimeSkyCaptures;D->bVisibleInRayTracing=S->bVisibleInRayTracing;
  D->CastShadow=S->CastShadow;D->bCastDynamicShadow=S->bCastDynamicShadow;D->bCastStaticShadow=S->bCastStaticShadow;D->bCastContactShadow=S->bCastContactShadow;
  D->bCastHiddenShadow=S->bCastHiddenShadow;D->bCastShadowAsTwoSided=S->bCastShadowAsTwoSided;D->bEmissiveLightSource=S->bEmissiveLightSource;
  D->bAffectDynamicIndirectLighting=S->bAffectDynamicIndirectLighting;D->bAffectIndirectLightingWhileHidden=S->bAffectIndirectLightingWhileHidden;D->bAffectDistanceFieldLighting=S->bAffectDistanceFieldLighting;
  D->bRenderCustomDepth=S->bRenderCustomDepth;D->CustomDepthStencilValue=S->CustomDepthStencilValue;D->CustomDepthStencilWriteMask=S->CustomDepthStencilWriteMask;
  D->bOwnerNoSee=S->bOwnerNoSee;D->bOnlyOwnerSee=S->bOnlyOwnerSee;D->bUseAttachParentBound=S->bUseAttachParentBound;
  D->MinDrawDistance=S->MinDrawDistance;D->SetCullDistance(S->LDMaxDrawDistance);D->BoundsScale=S->BoundsScale;
 }
 bool InRanch(const FVector3f& Position)
 {
  const FVector P=S47Three::ToThree(FVector(Position));
  return P.X>=390&&P.X<=485&&P.Z>=140&&P.Z<=235;
 }
 void Bytes(FSHA1& Hash,const void* Data,uint32 Count)
 {
  if(Count)Hash.Update(static_cast<const uint8*>(Data),Count);
 }
 FString Finish(FSHA1& Hash)
 {
  Hash.Final();FSHAHash Result;Hash.GetHash(Result.Hash);return Result.ToString();
 }
 void String(FSHA1& Hash,const FString& Value)
 {
  const FTCHARToUTF8 Text(*Value);const int32 Count=Text.Length();Bytes(Hash,&Count,sizeof(Count));Bytes(Hash,Text.Get(),Count);
 }
 FString VertexHash(const FStaticMeshLODResources& L)
 {
  const auto& B=L.VertexBuffers;const auto& T=B.StaticMeshVertexBuffer;
  const uint32 PCount=B.PositionVertexBuffer.GetNumVertices(),TCount=T.GetNumVertices(),UVs=T.GetNumTexCoords(),Colors=B.ColorVertexBuffer.GetNumVertices();
  if(!PCount||PCount!=TCount||!UVs||!B.PositionVertexBuffer.GetVertexData()||!T.GetTangentData()||!T.GetTexCoordData()||(Colors&&!B.ColorVertexBuffer.GetVertexData()))return {};
  FSHA1 Hash;const uint32 Format[]={PCount,UVs,Colors,uint32(T.GetUseHighPrecisionTangentBasis()),uint32(T.GetUseFullPrecisionUVs())};Bytes(Hash,Format,sizeof(Format));
  Bytes(Hash,B.PositionVertexBuffer.GetVertexData(),PCount*B.PositionVertexBuffer.GetStride());
  Bytes(Hash,T.GetTangentData(),T.GetTangentSize());Bytes(Hash,T.GetTexCoordData(),T.GetTexCoordSize());
  Bytes(Hash,B.ColorVertexBuffer.GetVertexData(),Colors*B.ColorVertexBuffer.GetStride());return Finish(Hash);
 }
 void SectionHash(FSHA1& Hash,const FStaticMeshSection& S)
 {
  const uint32 Fields[]={uint32(S.MaterialIndex),S.FirstIndex,S.NumTriangles,S.MinVertexIndex,S.MaxVertexIndex,
   uint32(S.bEnableCollision),uint32(S.bCastShadow),uint32(S.bVisibleInRayTracing),uint32(S.bAffectDistanceFieldLighting),uint32(S.bForceOpaque)};
  Bytes(Hash,Fields,sizeof(Fields));
#if WITH_EDITORONLY_DATA
  Bytes(Hash,S.UVDensities,sizeof(S.UVDensities));Bytes(Hash,S.Weights,sizeof(S.Weights));
#endif
 }
 FString MeshHash(UStaticMesh* Mesh)
 {
  if(!Mesh||!Mesh->GetRenderData()||Mesh->GetRenderData()->LODResources.Num()!=1)return {};
  const auto& L=Mesh->GetRenderData()->LODResources[0];const FString Vertices=VertexHash(L);if(Vertices.IsEmpty())return {};
  FSHA1 Hash;String(Hash,Vertices);TArray<uint32> Indices;L.IndexBuffer.GetCopy(Indices);Bytes(Hash,Indices.GetData(),Indices.Num()*sizeof(uint32));
  for(const auto& Section:L.Sections)SectionHash(Hash,Section);
  for(const auto& Material:Mesh->GetStaticMaterials())
  {
   String(Hash,Material.MaterialSlotName.ToString());String(Hash,Material.ImportedMaterialSlotName.ToString());
   String(Hash,GetPathNameSafe(Material.MaterialInterface.Get()));String(Hash,GetPathNameSafe(Material.OverlayMaterialInterface.Get()));
  }
  const uint32 Flags[]={uint32(Mesh->bSupportRayTracing),uint32(Mesh->bAllowCPUAccess),uint32(Mesh->GetLightMapCoordinateIndex())};Bytes(Hash,Flags,sizeof(Flags));return Finish(Hash);
 }
 bool Select(UStaticMesh* Mesh,const FExpected& E,FSelection& Farm,FSelection& Rest)
 {
  if(!Mesh||!Mesh->bAllowCPUAccess||!Mesh->bSupportRayTracing||!Mesh->GetRenderData()||Mesh->GetRenderData()->LODResources.Num()!=1)return false;
  const auto& L=Mesh->GetRenderData()->LODResources[0];const auto& B=L.VertexBuffers;
  if(L.GetNumTriangles()!=E.Total||VertexHash(L).IsEmpty()||L.IndexBuffer.GetIndexDataSize()==0)return false;
  TArray<uint32> Indices;L.IndexBuffer.GetCopy(Indices);if(Indices.Num()!=E.Total*3)return false;
  uint32 Covered=0;
  for(const auto& Section:L.Sections)
  {
   if(Section.FirstIndex!=Covered||Section.MaterialIndex<0||Section.MaterialIndex>=Mesh->GetStaticMaterials().Num())return false;
   FStaticMeshSection F=Section,R=Section;F.FirstIndex=Farm.Indices.Num();R.FirstIndex=Rest.Indices.Num();F.NumTriangles=R.NumTriangles=0;
   for(uint32 I=Section.FirstIndex;I<Section.FirstIndex+Section.NumTriangles*3;I+=3)
   {
    if(I+2>=uint32(Indices.Num()))return false;
    const uint32 A=Indices[I],Bidx=Indices[I+1],Cidx=Indices[I+2];
    if(FMath::Max3(A,Bidx,Cidx)>=B.PositionVertexBuffer.GetNumVertices())return false;
    const bool Ia=InRanch(B.PositionVertexBuffer.VertexPosition(A)),Ib=InRanch(B.PositionVertexBuffer.VertexPosition(Bidx)),Ic=InRanch(B.PositionVertexBuffer.VertexPosition(Cidx));
    if((Ia||Ib||Ic)&&!(Ia&&Ib&&Ic))return false; // Never clip or guess across a changed source boundary.
    FSelection& Target=Ia?Farm:Rest;FStaticMeshSection& TargetSection=Ia?F:R;
    Target.Indices.Append({A,Bidx,Cidx});++TargetSection.NumTriangles;
    for(uint32 Index:{A,Bidx,Cidx})Target.Bounds+=FVector(B.PositionVertexBuffer.VertexPosition(Index));
   }
   Covered+=Section.NumTriangles*3;if(F.NumTriangles)Farm.Sections.Add(F);if(R.NumTriangles)Rest.Sections.Add(R);
  }
  return Covered==uint32(Indices.Num())&&Farm.Indices.Num()==E.Farm*3&&Rest.Indices.Num()==(E.Total-E.Farm)*3&&Farm.Bounds.IsValid&&Rest.Bounds.IsValid;
 }
 UStaticMesh* Clone(AS47Web* W,UStaticMesh* Original,const FSelection& Selected,bool Farm)
 {
  const auto& Src=Original->GetRenderData()->LODResources[0];const auto& B=Src.VertexBuffers;
  UStaticMesh* Copy=NewObject<UStaticMesh>(W,NAME_None,RF_Transient);
  Copy->bAllowCPUAccess=true;Copy->NeverStream=true;Copy->bSupportRayTracing=Original->bSupportRayTracing;
  Copy->GetStaticMaterials()=Original->GetStaticMaterials();Copy->SetLightMapCoordinateIndex(Original->GetLightMapCoordinateIndex());
  // The remainder's original component retains its existing cooked collision, including story traces.
  // The shared visual has no collision body. No source asset or BodySetup is changed.
  if(!Farm)Copy->SetBodySetup(Original->GetBodySetup());
  auto Data=MakeUnique<FStaticMeshRenderData>();Data->AllocateLODResources(1);
  Data->Bounds=Farm?FBoxSphereBounds(Selected.Bounds):Original->GetRenderData()->Bounds;
  Data->ScreenSize[0].Default=1;Data->NumInlinedLODs=1;Data->CurrentFirstLODIdx=0;Data->bReadyForStreaming=false;
  auto& Dst=Data->LODResources[0];Dst.Sections.Append(Selected.Sections);
  Dst.VertexBuffers.PositionVertexBuffer.Init(B.PositionVertexBuffer,true);
  auto& T=Dst.VertexBuffers.StaticMeshVertexBuffer;const auto& SourceT=B.StaticMeshVertexBuffer;
  T.SetUseHighPrecisionTangentBasis(SourceT.GetUseHighPrecisionTangentBasis());T.SetUseFullPrecisionUVs(SourceT.GetUseFullPrecisionUVs());
  T.Init(SourceT.GetNumVertices(),SourceT.GetNumTexCoords(),true);
  // Same allocation policy as MakeWirePatch, with packed bytes copied rather than requantized.
  if(T.GetTangentSize()!=SourceT.GetTangentSize()||T.GetTexCoordSize()!=SourceT.GetTexCoordSize())return nullptr;
  FMemory::Memcpy(T.GetTangentData(),SourceT.GetTangentData(),SourceT.GetTangentSize());
  FMemory::Memcpy(T.GetTexCoordData(),SourceT.GetTexCoordData(),SourceT.GetTexCoordSize());
  const uint32 Colors=B.ColorVertexBuffer.GetNumVertices();
  if(Colors)
  {
   auto& C=Dst.VertexBuffers.ColorVertexBuffer;C.Init(Colors,true);
   if(C.GetStride()!=B.ColorVertexBuffer.GetStride())return nullptr;
   FMemory::Memcpy(C.GetVertexData(),B.ColorVertexBuffer.GetVertexData(),Colors*C.GetStride());
  }
  Dst.bHasColorVertexData=Src.bHasColorVertexData;Dst.bBuffersInlined=true;Dst.bIsOptionalLOD=false;
  Dst.SourceMeshBounds=Data->Bounds;Dst.MaxDeviation=Src.MaxDeviation;
  if(!Dst.IndexBuffer.TrySetAllowCPUAccess(true))return nullptr;
  Dst.IndexBuffer.SetIndices(Selected.Indices,EIndexBufferStride::Force32Bit);
  // Solid ranch and remainder retain native ray tracing, including a correct selected-index BLAS.
  bool InitRT=Copy->bSupportRayTracing;
#if !WITH_EDITOR
  InitRT=InitRT&&IsRayTracingAllowed();
#endif
  if(InitRT)Data->InitializeRayTracingRepresentationFromRenderingLODs();
  const FBoxSphereBounds Bounds=Data->Bounds;Copy->SetRenderData(MoveTemp(Data));Copy->SetExtendedBounds(Bounds);
  return VertexHash(Copy->GetRenderData()->LODResources[0])==VertexHash(Src)?Copy:nullptr;
 }
 bool ExactIndices(UStaticMesh* Mesh,const FSelection& Want)
 {
  if(!Mesh||!Mesh->GetRenderData()||Mesh->GetRenderData()->LODResources.Num()!=1)return false;
  const auto& L=Mesh->GetRenderData()->LODResources[0];TArray<uint32> Actual;L.IndexBuffer.GetCopy(Actual);
  return Actual==Want.Indices&&L.GetNumTriangles()==Want.Indices.Num()/3;
 }
}

struct FS47HighwayRanch
{
 TArray<FPart> Parts;
};

void AS47Web::BuildHighwayRanch()
{
 if(HighwayRanch.IsValid())return;
 auto Reject=[](const TCHAR* Why){UE_LOG(LogS47HighwayRanch,Warning,TEXT("S47: highway ranch original fallback: %s; no original component changed"),Why);};
 for(const TCHAR* Flag:{TEXT("S47HighwayRanchOff"),TEXT("S47LegacyHighway"),TEXT("S47PropsOff"),TEXT("S47LegacyCalicheStreet")})
  if(FParse::Param(FCommandLine::Get(),Flag)){Reject(Flag);return;}
 const int32 RootIndex=Handle(TEXT("world.legs.road.area.group"));
 if(RootIndex!=1240||!Nodes.IsValidIndex(1289)||Nodes[1289].Parent!=RootIndex||!Nodes[RootIndex].Comp.IsValid()||HighwayLandscapeReady())
 {Reject(TEXT("original road root/parent or shared landscape order"));return;}
 bool Backdrop=false;
 for(const auto& N:Nodes)if(N.Name==TEXT("prop:streetGroundMesas")&&N.bVis)
  if(const auto* C=Cast<UStaticMeshComponent>(N.Comp.Get());C&&C->GetStaticMesh()&&!C->bHiddenInGame)Backdrop=true;
 if(!Backdrop){Reject(TEXT("continuous landscape backdrop unavailable"));return;}
 auto Ready=MakeShared<FS47HighwayRanch>();Ready->Parts.Reserve(3);
 for(const auto& E:Expected)
 {
  if(!Nodes.IsValidIndex(E.Node)){Reject(TEXT("source node missing"));return;}
  const auto& N=Nodes[E.Node];auto* C=Cast<UStaticMeshComponent>(N.Comp.Get());UStaticMesh* Mesh=C?C->GetStaticMesh().Get():nullptr;
  const FString Asset=FString::Printf(TEXT("/Game/Web/Geo/geo_007/StaticMeshes/g%d.g%d"),E.Geo,E.Geo);
  if(N.Parent!=1289||N.Geo!=E.Geo||N.Mats!=TArray<int32>{E.Material}||!N.bVis||N.bInstanced||!C||!Mesh||Mesh->GetPathName()!=Asset||C->bHiddenInGame||C->GetNumMaterials()==0
   ||!C->GetComponentTransform().Equals(FTransform(S47Three::ToUE(RoadOrigin)),.02))
  {Reject(TEXT("exact source parent/geometry/material/transform"));return;}
  bool SlotsReady=true;for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)SlotsReady&=C->GetMaterial(Slot)!=nullptr;
  if(!SlotsReady){Reject(TEXT("source material binding missing"));return;}
  FPart Part;Part.Source=E;Part.Original.Reset(Mesh);Part.Rest=C;Part.OriginalTransform=C->GetComponentTransform();
  Part.Collision=C->GetCollisionEnabled();Part.Responses=C->GetCollisionResponseToChannels();Part.Hidden=C->bHiddenInGame;
  Part.Shadow=C->CastShadow;Part.RayVisible=C->bVisibleInRayTracing;Part.AuthoredVisible=N.bVis;Part.RenderFlags=RenderFlags(C);
  if(!Select(Mesh,E,Part.FarmSelection,Part.RestSelection)){Reject(TEXT("CPU-ready exact 300/3972 partition required"));return;}
  Part.OriginalHash=MeshHash(Mesh);Part.VertexHash=VertexHash(Mesh->GetRenderData()->LODResources[0]);
  if(Part.OriginalHash.IsEmpty()){Reject(TEXT("source buffer fingerprint unavailable"));return;}
  UStaticMesh* Rest=Clone(this,Mesh,Part.RestSelection,false);UStaticMesh* Farm=Clone(this,Mesh,Part.FarmSelection,true);
  if(!Rest||!Farm||Rest->GetStaticMaterials()!=Mesh->GetStaticMaterials()||Farm->GetStaticMaterials()!=Mesh->GetStaticMaterials()
   ||!ExactIndices(Rest,Part.RestSelection)||!ExactIndices(Farm,Part.FarmSelection)||MeshHash(Mesh)!=Part.OriginalHash)
  {Reject(TEXT("exact packed buffer/index/slot preservation"));return;}
  Part.RestMesh=Rest;Part.FarmMesh=Farm;Part.RestHash=MeshHash(Rest);Part.FarmHash=MeshHash(Farm);Ready->Parts.Add(MoveTemp(Part));
 }
 // Complete all checks and prepare unregistered visuals before committing any original swap.
 for(auto& Part:Ready->Parts)
 {
  auto* Source=Part.Rest.Get();auto* C=NewObject<UStaticMeshComponent>(this);
  C->SetMobility(Source->Mobility);C->SetStaticMesh(Part.FarmMesh.Get());C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->SetCollisionResponseToAllChannels(ECR_Ignore);CopyRenderFlags(Source,C);
  for(int32 Slot=0;Slot<Source->GetNumMaterials();++Slot)C->SetMaterial(Slot,Source->GetMaterial(Slot));
  C->SetupAttachment(Nodes[RootIndex].Comp.Get());Part.Farm=C;
 }
 for(auto& Part:Ready->Parts)
 {
  Part.RestMesh->InitResources();Part.FarmMesh->InitResources();auto* Source=Part.Rest.Get();const auto Materials=Source->GetMaterials();
  Source->SetStaticMesh(Part.RestMesh.Get());for(int32 Slot=0;Slot<Materials.Num();++Slot)Source->SetMaterial(Slot,Materials[Slot]);
  auto* C=Part.Farm.Get();C->RegisterComponent();C->SetWorldTransform(Part.OriginalTransform);
  Part.FarmNode=AdoptNode(C,RootIndex,FString::Printf(TEXT("highway:ranch:g%d"),Part.Source.Geo));
  // Preserve normal material-producer updates on both sides of this index partition.
  Nodes[Part.FarmNode].Mats=Nodes[Part.Source.Node].Mats;
  UE_LOG(LogS47HighwayRanch,Display,TEXT("S47: highway ranch partition g%d original=%d shared=%d rest=%d vertexSHA1=%s originalSHA1=%s sharedSHA1=%s restSHA1=%s"),
   Part.Source.Geo,Part.Source.Total,Part.Source.Farm,Part.Source.Total-Part.Source.Farm,*Part.VertexHash,*Part.OriginalHash,*Part.FarmHash,*Part.RestHash);
 }
 HighwayRanch=MoveTemp(Ready);
 UE_LOG(LogS47HighwayRanch,Display,TEXT("S47: highway ranch ready: exactly300 shared+3972 retained triangles; original assets/buffers/slots intact, rest collision retained, native RT retained; awaiting canonical landscape sharing"));
}

bool AS47Web::AuditHighwayRanch(TArray<FString>& Lines) const
{
 bool Good=true;auto Check=[&](bool Pass,const FString& Why){Lines.Add(FString::Printf(TEXT("%s highway ranch %s"),Pass?TEXT("OK"):TEXT("FAIL"),*Why));Good&=Pass;};
 Check(HighwayRanch.IsValid()&&HighwayRanch->Parts.Num()==3&&HighwayLandscapeReady(),TEXT("complete three-part native partition and continuous landscape"));
 if(!HighwayRanch.IsValid())return false;
 int32 FarmTriangles=0,RestTriangles=0;
 for(const auto& Part:HighwayRanch->Parts)
 {
  const auto* Source=Node(Part.Source.Node);const auto* Shared=Node(Part.FarmNode);auto* Original=Part.Original.Get();auto* Rest=Part.Rest.Get();auto* Farm=Part.Farm.Get();
  auto* R=Part.RestMesh.Get();auto* F=Part.FarmMesh.Get();const FString Label=FString::Printf(TEXT("g%d"),Part.Source.Geo);
  Check(Original&&MeshHash(Original)==Part.OriginalHash,Label+TEXT(" original full index/section/packed-buffer/slot fingerprint unchanged ")+Part.OriginalHash);
  Check(R&&F&&MeshHash(R)==Part.RestHash&&MeshHash(F)==Part.FarmHash&&ExactIndices(R,Part.RestSelection)&&ExactIndices(F,Part.FarmSelection),Label+TEXT(" exact partition fingerprints/index order retained"));
  Check(R&&F&&Original&&R->GetStaticMaterials()==Original->GetStaticMaterials()&&F->GetStaticMaterials()==Original->GetStaticMaterials(),Label+TEXT(" original static material slots retained"));
  Check(R&&F&&R->bSupportRayTracing&&F->bSupportRayTracing,Label+TEXT(" solid native ray tracing policy retained"));
  Check(Rest&&R&&Rest->GetStaticMesh()==R&&Original&&R->GetBodySetup()==Original->GetBodySetup()&&Rest->GetCollisionEnabled()==Part.Collision&&Rest->GetCollisionResponseToChannels()==Part.Responses
   &&Rest->bHiddenInGame==Part.Hidden&&Rest->CastShadow==Part.Shadow&&Rest->bVisibleInRayTracing==Part.RayVisible&&Rest->GetComponentTransform().Equals(Part.OriginalTransform,.03),Label+TEXT(" original remainder component transform/collision/render flags retained"));
  Check(Source&&Source->Parent==1289&&Source->Geo==Part.Source.Geo&&Source->Mats==TArray<int32>{Part.Source.Material}&&Source->bVis==Part.AuthoredVisible,Label+TEXT(" source metadata/visibility producer retained"));
  Check(Farm&&F&&Farm->GetStaticMesh()==F&&Farm->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!Farm->bHiddenInGame&&Farm->IsRegistered()
   &&Shared&&Shared->Name==FString::Printf(TEXT("highway:ranch:g%d"),Part.Source.Geo)&&Shared->Parent==1240&&Shared->Mats==TArray<int32>{Part.Source.Material}&&HighwaySharedNodes.Contains(Part.FarmNode),Label+TEXT(" exactly the shared ranch is NoCollision in canonical landscape"));
  bool Materials=Rest&&Farm&&Rest->GetNumMaterials()==Farm->GetNumMaterials();
  if(Materials)for(int32 Slot=0;Slot<Rest->GetNumMaterials();++Slot)Materials&=Rest->GetMaterial(Slot)&&Rest->GetMaterial(Slot)==Farm->GetMaterial(Slot);
  Check(Materials,Label+TEXT(" actual original material producer bindings agree"));
  Check(RenderFlags(Rest)==Part.RenderFlags&&RenderFlags(Farm)==Part.RenderFlags&&Rest&&Farm&&Rest->MinDrawDistance==Farm->MinDrawDistance&&Rest->LDMaxDrawDistance==Farm->LDMaxDrawDistance&&Rest->BoundsScale==Farm->BoundsScale,Label+TEXT(" original occluder/depth/capture/decal/shadow/emissive/stencil and culling flags retained"));
  FTransform Want=Part.OriginalTransform;Want.AddToTranslation(S47Three::ToUE(CurrentArea==TEXT("saro")?-SaroToRoad:FVector::ZeroVector));
  const bool Visible=CurrentArea==TEXT("saro")||CurrentArea==TEXT("road")||CurrentArea==TEXT("diner");
  Check(Farm&&Farm->GetComponentTransform().Equals(Want,.03)&&Farm->IsVisible()==Visible,Label+TEXT(" canonical offset and active-area visibility retained"));
  Check(F&&F->GetBounds().GetBox().Equals(Part.FarmSelection.Bounds,.03),Label+TEXT(" selected ranch bounds, local coordinates unchanged"));
  if(F&&F->GetRenderData())FarmTriangles+=F->GetRenderData()->LODResources[0].GetNumTriangles();
  if(R&&R->GetRenderData())RestTriangles+=R->GetRenderData()->LODResources[0].GetNumTriangles();
 }
 Check(FarmTriangles==300&&RestTriangles==3972,TEXT("exactly 300 ranch +3972 remainder =4272 original triangles"));return Good;
}
