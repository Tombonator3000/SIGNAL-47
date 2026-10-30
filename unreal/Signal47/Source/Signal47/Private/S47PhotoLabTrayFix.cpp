#include "S47PhotoLabTrayFix.h"

#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/SecureHash.h"
#include "PhysicsEngine/BodySetup.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/MemoryWriter.h"
#include "StaticMeshResources.h"
#if WITH_EDITOR
#include "MeshDescription.h"
#endif

namespace
{
 const TCHAR* SourcePath=TEXT("/Game/Web/Geo/geo_002/StaticMeshes/g264.g264");
 const TCHAR* FixedPath=TEXT("/Game/Signal47/Art/PhotoLab/SM_PhotoLab_TrayInnerFix.SM_PhotoLab_TrayInnerFix");
 constexpr float OldTop=100.f, NewTop=100.1f, Tolerance=.002f;
 const float Centers[]={1400.f,1470.f,1540.f};
 int32 Tray(const FVector3f& P,float Top)
 {
  if(FMath::Abs(P.Z-Top)>Tolerance||P.Y < -557.f-Tolerance||P.Y > -523.f+Tolerance)return INDEX_NONE;
  for(int32 I=0;I<3;++I)if(FMath::Abs(P.X-Centers[I])<=22.f+Tolerance)return I;
  return INDEX_NONE;
 }
 FString Digest(const TArray<uint8>& Bytes) { return FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num()).ToString(); }
 bool Fail(FString& Why,const TCHAR* Message) { Why=Message;return false; }
 bool MaterialsSame(UStaticMesh* A,UStaticMesh* B)
 {
  const auto& X=A->GetStaticMaterials();const auto& Y=B->GetStaticMaterials();
  if(X.Num()!=1||Y.Num()!=X.Num())return false;
  for(int32 I=0;I<X.Num();++I)if(X[I].MaterialSlotName!=Y[I].MaterialSlotName||
   X[I].ImportedMaterialSlotName!=Y[I].ImportedMaterialSlotName||X[I].MaterialInterface!=Y[I].MaterialInterface)return false;
  return true;
 }
 bool BodySamePolicy(UStaticMesh* A,UStaticMesh* B)
 {
  const UBodySetup* X=A->GetBodySetup();const UBodySetup* Y=B->GetBodySetup();
  return X&&Y&&X->CollisionTraceFlag==Y->CollisionTraceFlag&&X->AggGeom.GetElementCount()==Y->AggGeom.GetElementCount()&&
   X->DefaultInstance.GetCollisionEnabled()==Y->DefaultInstance.GetCollisionEnabled()&&
   X->DefaultInstance.GetCollisionProfileName()==Y->DefaultInstance.GetCollisionProfileName();
 }
 bool RenderHash(UStaticMesh* Mesh,bool Normalize,FString& Hash,int32& Changed)
 {
  const FStaticMeshRenderData* Data=Mesh?Mesh->GetRenderData():nullptr;
  if(!Data||Data->LODResources.Num()!=1)return false;
  const FStaticMeshLODResources& L=Data->LODResources[0];const auto& V=L.VertexBuffers;
  if(!V.PositionVertexBuffer.GetVertexData()||!V.StaticMeshVertexBuffer.GetTangentData()||
   !V.StaticMeshVertexBuffer.GetTexCoordData()||L.IndexBuffer.GetIndexDataSize()==0)return false;
  Changed=0;TArray<FString> Corners;
  for(uint32 I=0;I<V.PositionVertexBuffer.GetNumVertices();++I)
  {
   FVector3f P=V.PositionVertexBuffer.VertexPosition(I);
   if(Tray(P,NewTop)!=INDEX_NONE){++Changed;if(Normalize)P.Z=OldTop;}
   TArray<uint8> Bytes;FMemoryWriter Ar(Bytes,true);Ar<<P;
   FVector3f X(V.StaticMeshVertexBuffer.VertexTangentX(I)),Y(V.StaticMeshVertexBuffer.VertexTangentY(I)),Z(V.StaticMeshVertexBuffer.VertexTangentZ(I));
   Ar<<X<<Y<<Z;
   for(uint32 UV=0;UV<V.StaticMeshVertexBuffer.GetNumTexCoords();++UV){FVector2f Q=V.StaticMeshVertexBuffer.GetVertexUV(I,UV);Ar<<Q;}
   FColor C=V.ColorVertexBuffer.GetNumVertices()?V.ColorVertexBuffer.VertexColor(I):FColor::White;Ar<<C;
   Corners.Add(BytesToHex(Bytes.GetData(),Bytes.Num()));
  }
  TArray<uint32> Indices;L.IndexBuffer.GetCopy(Indices);TArray<FString> Triangles;
  for(const FStaticMeshSection& S:L.Sections)for(uint32 I=S.FirstIndex;I<S.FirstIndex+S.NumTriangles*3;I+=3)
  {
   if(I+2>=uint32(Indices.Num())||Indices[I]>=uint32(Corners.Num())||Indices[I+1]>=uint32(Corners.Num())||Indices[I+2]>=uint32(Corners.Num()))return false;
   const FString A=Corners[Indices[I]],B=Corners[Indices[I+1]],C=Corners[Indices[I+2]];
   const FString Rotations[]={A+B+C,B+C+A,C+A+B};FString Ordered=Rotations[0];
   for(int32 K=1;K<3;++K)if(Rotations[K]<Ordered)Ordered=Rotations[K];
   Triangles.Add(FString::Printf(TEXT("%d:%d:%d:%d:%d:%d:"),S.MaterialIndex,S.bEnableCollision,S.bCastShadow,
    S.bVisibleInRayTracing,S.bAffectDistanceFieldLighting,S.bForceOpaque)+Ordered);
  }
  if(Triangles.Num()!=544)return false;
  Triangles.Sort();const FString Text=FString::Join(Triangles,TEXT("\n"));FTCHARToUTF8 Utf8(*Text);
  Hash=FSHA1::HashBuffer(Utf8.Get(),Utf8.Length()).ToString();return true;
 }
 bool AssetValid(UStaticMesh* Source,UStaticMesh* Fixed,FString& Why)
 {
  if(!Source||!Fixed||Source->GetPathName()!=SourcePath||Fixed->GetPathName()!=FixedPath||
   Source->GetNumLODs()!=1||Fixed->GetNumLODs()!=1||Source->GetNumTriangles(0)!=544||Fixed->GetNumTriangles(0)!=544)
   return Fail(Why,TEXT("exact source/clone type, path or 544-triangle single LOD differs"));
  const auto* Proof=Cast<US47PhotoLabTrayFixProof>(Fixed->GetAssetUserDataOfClass(US47PhotoLabTrayFixProof::StaticClass()));
  if(!Proof||Proof->SourceMesh!=Source||Proof->SourceTriangles!=544||Proof->ChangedVertices<12||Proof->ChangedRenderVertices<12||
   Proof->SourceDescriptionHash.IsEmpty()||Proof->SourceRenderHash.IsEmpty()||Proof->FixedRenderHash.IsEmpty()||
   !Source->GetBoundingBox().Min.Equals(Proof->SourceMin,.001)||!Source->GetBoundingBox().Max.Equals(Proof->SourceMax,.001)||
   !MaterialsSame(Source,Fixed)||!BodySamePolicy(Source,Fixed)||!Fixed->bAllowCPUAccess||
   Source->bSupportRayTracing!=Fixed->bSupportRayTracing||Source->GetNaniteSettings().bEnabled!=Fixed->GetNaniteSettings().bEnabled)
   return Fail(Why,TEXT("cooked proof, source bounds, slots, CPU retention, RT or collision policy differs"));
  FString Actual,Normalized;int32 Changed=0,Ignored=0;
  if(!RenderHash(Fixed,false,Actual,Changed)||!RenderHash(Fixed,true,Normalized,Ignored)||
   Actual!=Proof->FixedRenderHash||Normalized!=Proof->SourceRenderHash||Changed!=Proof->ChangedRenderVertices)
   return Fail(Why,TEXT("clone render triangles/UVs/normals/colors or selected positions differ from editor proof"));
  return true;
 }
#if WITH_EDITOR
 FString DescriptionHash(FMeshDescription Copy)
 { TArray<uint8> Bytes;FMemoryWriter Ar(Bytes,true);Copy.Serialize(Ar);return Digest(Bytes); }
 bool DescribeFix(UStaticMesh* Source,UStaticMesh* Fixed,bool Verify,TSharedPtr<FJsonObject> Report,FString& Why)
 {
  if(!Source||!Fixed||Source==Fixed||Source->GetPathName()!=SourcePath||Fixed->GetPathName()!=FixedPath||
   Source->GetNumLODs()!=1||Source->GetNumTriangles(0)!=544||!MaterialsSame(Source,Fixed))
   return Fail(Why,TEXT("source/duplicate identity, slots or topology mismatch before mutation"));
  FMeshDescription Original;
  if(!Source->CloneMeshDescription(0,Original)||Original.Triangles().Num()!=544)return Fail(Why,TEXT("original MeshDescription unavailable"));
  const FString OriginalHash=DescriptionHash(Original);
  FMeshDescription Expected=Original;auto Positions=Expected.GetVertexPositions();int32 Changed=0;TSet<FString> Corners[3];
  for(FVertexID Id:Expected.Vertices().GetElementIDs())
  {
   FVector3f P=Positions[Id];const int32 I=Tray(P,OldTop);if(I==INDEX_NONE)continue;
   if(FMath::Abs(FMath::Abs(P.X-Centers[I])-22.f)>Tolerance||
      FMath::Abs(FMath::Abs(P.Y+540.f)-17.f)>Tolerance)return Fail(Why,TEXT("selected plane contains unexpected non-corner vertices"));
   Corners[I].Add(FString::Printf(TEXT("%.3f,%.3f"),P.X,P.Y));P.Z=NewTop;Positions[Id]=P;++Changed;
  }
  if(Changed<12||Corners[0].Num()!=4||Corners[1].Num()!=4||Corners[2].Num()!=4)return Fail(Why,TEXT("exact twelve liner top corners not found"));
  FString SourceRender;int32 SourceRaised=0;
  if(!RenderHash(Source,false,SourceRender,SourceRaised)||SourceRaised)return Fail(Why,TEXT("original render buffer unavailable or already patched"));
  if(!Verify)
  {
   // APIs are native UE5.8.4 StaticMesh.h. Commit the copied description and use
   // the duplicate's established build settings, without touching the source.
   Fixed->CreateMeshDescription(0,Expected);Fixed->CommitMeshDescription(0);
   Fixed->bAllowCPUAccess=true;Fixed->NeverStream=true;TArray<FText> Errors;Fixed->Build(true,&Errors);
   if(Errors.Num())return Fail(Why,TEXT("native clone build reported errors"));
   if(!Source->GetBodySetup())return Fail(Why,TEXT("source collision body missing"));
   Fixed->SetBodySetup(DuplicateObject<UBodySetup>(Source->GetBodySetup(),Fixed));
  }
  FMeshDescription Actual;
  if(!Fixed->CloneMeshDescription(0,Actual)||DescriptionHash(Actual)!=DescriptionHash(Expected))
   return Fail(Why,TEXT("full clone MeshDescription differs beyond the exact selected vertex edit"));
  auto ActualPositions=Actual.GetVertexPositions();
  for(FVertexID Id:Actual.Vertices().GetElementIDs())if(Tray(ActualPositions[Id],NewTop)!=INDEX_NONE)
  {FVector3f P=ActualPositions[Id];P.Z=OldTop;ActualPositions[Id]=P;}
  if(DescriptionHash(Actual)!=OriginalHash)return Fail(Why,TEXT("normalized full attributes/UVs/normals/topology do not equal source"));
  FString FixedRender,Normalized;int32 ChangedRender=0,Ignored=0;
  if(!RenderHash(Fixed,false,FixedRender,ChangedRender)||!RenderHash(Fixed,true,Normalized,Ignored)||
   Normalized!=SourceRender||ChangedRender<12||!BodySamePolicy(Source,Fixed)||!MaterialsSame(Source,Fixed))
   return Fail(Why,TEXT("render triangles/UVs/normals/colors/materials or body policy not preserved"));
  if(Verify)
  {
   const auto* Proof=Cast<US47PhotoLabTrayFixProof>(Fixed->GetAssetUserDataOfClass(US47PhotoLabTrayFixProof::StaticClass()));
   if(!Proof||Proof->SourceDescriptionHash!=OriginalHash||Proof->ChangedVertices!=Changed)
    return Fail(Why,TEXT("saved proof does not bind the current full source description"));
  }
  if(!Verify)
  {
   Fixed->RemoveUserDataOfClass(US47PhotoLabTrayFixProof::StaticClass());
   auto* Proof=NewObject<US47PhotoLabTrayFixProof>(Fixed);Proof->SourceMesh=Source;
   Proof->SourceDescriptionHash=OriginalHash;Proof->SourceRenderHash=SourceRender;Proof->FixedRenderHash=FixedRender;
   Proof->SourceTriangles=544;Proof->ChangedVertices=Changed;Proof->ChangedRenderVertices=ChangedRender;
   Proof->SourceMin=Source->GetBoundingBox().Min;Proof->SourceMax=Source->GetBoundingBox().Max;Fixed->AddAssetUserData(Proof);Fixed->MarkPackageDirty();
  }
  FMeshDescription SourceAfter;
  if(!Source->CloneMeshDescription(0,SourceAfter)||DescriptionHash(SourceAfter)!=OriginalHash)
   return Fail(Why,TEXT("original full MeshDescription changed"));
  if(!AssetValid(Source,Fixed,Why))return false;
  Report->SetStringField(TEXT("source_description_sha1"),OriginalHash);Report->SetStringField(TEXT("normalized_render_sha1"),SourceRender);
  Report->SetStringField(TEXT("fixed_render_sha1"),FixedRender);Report->SetNumberField(TEXT("changed_source_vertices"),Changed);
  Report->SetNumberField(TEXT("changed_render_vertices"),ChangedRender);Report->SetNumberField(TEXT("triangles"),544);
  Report->SetBoolField(TEXT("full_normalized_description_equal"),true);Report->SetBoolField(TEXT("oriented_render_corners_equal"),true);
  Report->SetStringField(TEXT("collision"),TEXT("source body policy retained; runtime reuses original BodySetup for exact query geometry"));
  return true;
 }
#endif
 struct FInstall
 {
  TWeakObjectPtr<UStaticMeshComponent> Comp;TWeakObjectPtr<UStaticMesh> Source,Fixed;
  TWeakObjectPtr<USceneComponent> Parent;int32 ParentNode=-1;
  TArray<TWeakObjectPtr<UMaterialInterface>> Materials;FTransform Local;FString UseId;
  FCollisionResponseContainer Responses;ECollisionEnabled::Type Collision=ECollisionEnabled::NoCollision;bool bVis=false,bHidden=false,bOverlap=false;
 };
 TMap<TWeakObjectPtr<AS47Web>,FInstall> Installed;
}

FString US47PhotoLabTrayFixTools::Prepare(UStaticMesh* Source,UStaticMesh* Candidate,bool bVerifyOnly)
{
 auto Report=MakeShared<FJsonObject>();FString Why;bool Ok=false;
 Report->SetStringField(TEXT("mode"),bVerifyOnly?TEXT("verify_saved_only"):TEXT("prepare_clone"));
#if WITH_EDITOR
 Ok=DescribeFix(Source,Candidate,bVerifyOnly,Report,Why);
#else
 Why=TEXT("authoring bridge is editor-only");
#endif
 Report->SetStringField(TEXT("status"),Ok?TEXT("PASS"):TEXT("FAIL"));Report->SetStringField(TEXT("error"),Why);
 FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));return Json;
}

bool S47PhotoLabTrayFix::Build(AS47Web* W)
{
 if(!W||!W->IsLoaded()||FParse::Param(FCommandLine::Get(),TEXT("S47PhotoLabTrayFixOff"))||FParse::Param(FCommandLine::Get(),TEXT("S47PropsOff")))return false;
 if(Installed.Contains(W))return true;
 const FS47WebNode* N=W->Node(418);const FS47WebNode* Parent=W->Node(W->Handle(TEXT("yard.occluders[2]")));
 UStaticMeshComponent* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
 UStaticMesh* Source=C?C->GetStaticMesh():nullptr;UStaticMesh* Fixed=LoadObject<UStaticMesh>(nullptr,FixedPath);FString Why;
 if(!N||!Parent||N->Geo!=264||N->Mats.Num()!=1||N->Mats[0]!=161||N->Parent!=W->Handle(TEXT("yard.occluders[2]"))||
  !C||C->GetAttachParent()!=Parent->Comp.Get()||W->NodeOf(C)!=418||!AssetValid(Source,Fixed,Why)||!Source->GetBodySetup())
 {UE_LOG(LogTemp,Warning,TEXT("S47 tray fix skipped; original retained: %s"),*Why);return false;}
 const FS47Obj Paper=W->Obj(TEXT("yard.wetPrint"));
 if(!Paper||!Paper.WorldPosition().Equals(FVector(15.4,1.003,-5.4),.0001))return false;
 FInstall I;I.Comp=C;I.Source=Source;I.Fixed=Fixed;I.Parent=C->GetAttachParent();I.ParentNode=N->Parent;I.Local=C->GetRelativeTransform();I.UseId=W->UsableAt(C);
 I.Responses=C->GetCollisionResponseToChannels();I.Collision=C->GetCollisionEnabled();I.bVis=N->bVis;I.bHidden=C->bHiddenInGame;I.bOverlap=C->GetGenerateOverlapEvents();
 for(UMaterialInterface* M:C->GetMaterials())I.Materials.Add(M);
 // The same original component keeps its exact query shape/body, IDs and state.
 // Only its render mesh points at the cooked, independently proved native clone.
 Fixed->SetBodySetup(Source->GetBodySetup());C->SetStaticMesh(Fixed);
 for(int32 K=0;K<I.Materials.Num();++K)C->SetMaterial(K,I.Materials[K].Get());
 if(C->GetStaticMesh()!=Fixed||C->GetAttachParent()!=I.Parent.Get()||!C->GetRelativeTransform().Equals(I.Local,.0001)||
  C->GetCollisionEnabled()!=I.Collision||C->GetCollisionResponseToChannels()!=I.Responses||C->GetGenerateOverlapEvents()!=I.bOverlap||W->UsableAt(C)!=I.UseId)
 {
  C->SetStaticMesh(Source);for(int32 K=0;K<I.Materials.Num();++K)C->SetMaterial(K,I.Materials[K].Get());
  UE_LOG(LogTemp,Error,TEXT("S47 tray fix mesh swap did not retain component state; original restored"));return false;
 }
 Installed.Add(W,MoveTemp(I));UE_LOG(LogTemp,Display,TEXT("S47 tray fix installed: three original liner tops +1mm; wetPrint/other plastic unchanged"));return true;
}

bool S47PhotoLabTrayFix::Audit(AS47Web* W,TArray<FString>& Lines)
{
 const FInstall* I=W?Installed.Find(W):nullptr;const FS47WebNode* N=W?W->Node(418):nullptr;
 UStaticMeshComponent* C=I?I->Comp.Get():nullptr;FString Why;
 bool Ok=I&&N&&C&&W->NodeOf(C)==418&&N->Geo==264&&N->Mats.Num()==1&&N->Mats[0]==161&&N->Parent==I->ParentNode&&
  C->GetAttachParent()==I->Parent.Get()&&N->bVis==I->bVis&&C->GetStaticMesh()==I->Fixed.Get()&&C->IsVisible()==W->ObjAt(418).Shown()&&
  C->GetRelativeTransform().Equals(I->Local,.0001)&&C->bHiddenInGame==I->bHidden&&C->GetCollisionEnabled()==I->Collision&&
  C->GetCollisionResponseToChannels()==I->Responses&&C->GetGenerateOverlapEvents()==I->bOverlap&&W->UsableAt(C)==I->UseId&&
  AssetValid(I->Source.Get(),I->Fixed.Get(),Why)&&I->Fixed->GetBodySetup()==I->Source->GetBodySetup()&&C->GetNumMaterials()==I->Materials.Num();
 if(C&&I)for(int32 K=0;K<I->Materials.Num();++K)Ok&=C->GetMaterial(K)==I->Materials[K].Get();
 const FS47Obj Paper=W?W->Obj(TEXT("yard.wetPrint")):FS47Obj();Ok&=Paper&&Paper.WorldPosition().Equals(FVector(15.4,1.003,-5.4),.0001);
 Lines.Add(FString::Printf(TEXT("photolab tray separation %s: node418/g264 clone, full cooked render proof, original materials/transform/query body/use ID; liner1.001m paper1.003m; %s"),Ok?TEXT("PASS"):TEXT("FAIL"),*Why));
 return Ok;
}
