// HUM-01: preserve the existing lap radio and person owner; replace only old body geometry.
#include "S47RadioWoman.h"
#include "S47Web.h"
#include "S47Three.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
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
 FString AssetPath=TEXT("/Game/MetaHumans/Mannequins/RadioWoman/BakeR1/SM_RadioWoman_Seated");
 FAutoConsoleVariableRef AssetCVar(TEXT("s47.RadioWoman.Asset"),AssetPath,TEXT("Private accepted bake candidate; evaluated when the world is built."),ECVF_Default);
 struct FExpected {int32 Node,Geo,Material,Triangles;};
 const FExpected Expected[]={{1481,1028,656,108},{1482,1029,657,156},{1483,1030,658,48},{1484,1031,641,36},{1485,1032,638,12},{1486,1033,659,100}};
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
}

struct FS47RadioWomanState
{
 TWeakObjectPtr<AS47Web> Web;TArray<FOriginal> Original;TStrongObjectPtr<UStaticMesh> NativeAsset;
 TWeakObjectPtr<UStaticMesh> Cut;TWeakObjectPtr<UStaticMeshComponent> Native;
 FString Why=TEXT("not attempted"),Asset,Packed;bool Active=false;int32 NativeNode=INDEX_NONE,SeatHits=0;
 FTransform Placement;FVector Seat=FVector::ZeroVector,Forward=FVector::ZeroVector,Right=FVector::ZeroVector;
 TArray<uint32> FullIndices,RadioIndices;TArray<int32> LODs;
};

TSharedPtr<FS47RadioWoman> FS47RadioWoman::Build(AS47Web* W)
{
 auto Result=MakeShared<FS47RadioWoman>();Result->State=MakeShared<FS47RadioWomanState>();auto& S=*Result->State;S.Web=W;
 auto Reject=[&](const TCHAR* Why){S.Why=Why;UE_LOG(LogS47RadioWoman,Warning,TEXT("S47: radio woman original fallback: %s; original group/radio/proxy retained"),Why);return Result;};
 if(!W)return Reject(TEXT("missing web"));
 const auto* Group=W->Node(1480);const auto* Proxy=W->Node(1544);
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
 if(!BenchNode||BenchNode->Geo!=1074||BenchNode->Name!=TEXT("lic:garden_bench")||!Bench||!Bench->GetStaticMesh())return Reject(TEXT("actual laundromat bench unavailable"));
 const FVector Anchor=Group->Comp->GetComponentLocation();TArray<double> Heights;
 const FCollisionQueryParams Q(SCENE_QUERY_STAT(S47RadioWomanSeat),true);
 for(double X:{-2.,0.,2.})for(double Y:{-2.,0.,2.})
 {
  const FVector At=Anchor+FVector(X,Y,0);FHitResult Hit;
  if(Bench->LineTraceComponent(Hit,At+FVector(0,0,20),At-FVector(0,0,15),Q)&&Hit.ImpactNormal.Z>.7)Heights.Add(Hit.ImpactPoint.Z);
 }
 S.SeatHits=Heights.Num();if(Heights.Num()<5)return Reject(TEXT("actual bench seat requires five of nine physical surface samples"));
 Heights.Sort();S.Seat=FVector(Anchor.X,Anchor.Y,Heights[Heights.Num()/2]);
 if(FMath::Abs(S.Seat.Z-Anchor.Z)>2.||Heights.Last()-Heights[0]>1.5)return Reject(TEXT("measured bench seat differs from canonical seat anchor"));
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
 auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("active"),IsActive());if(!State.IsValid())return J;const auto& S=*State;
 J->SetStringField(TEXT("reason"),S.Why);J->SetStringField(TEXT("asset"),S.Asset);J->SetNumberField(TEXT("native_node"),S.NativeNode);
 J->SetNumberField(TEXT("person_node"),1480);J->SetNumberField(TEXT("proxy_node"),1544);J->SetStringField(TEXT("interaction"),TEXT("radio_woman"));
 J->SetStringField(TEXT("schedule_owner"),TEXT("FS47Town::SetPeople / NightPerson 18:00..04:50"));J->SetStringField(TEXT("packed_vertex_sha1"),S.Packed);
 J->SetNumberField(TEXT("bench_surface_hits"),S.SeatHits);J->SetArrayField(TEXT("measured_seat_ue_cm"),Vec(S.Seat));
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
