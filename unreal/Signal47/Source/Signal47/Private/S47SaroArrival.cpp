// Native SARO visitor lot and relocated monument sign. No new imported assets.
#include "S47SaroArrival.h"
#include "S47Web.h"
#include "S47Obj.h"
#include "S47Three.h"
#include "S47Night.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StaticMeshResources.h"
#include "RenderUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SaroArrival,Log,All);
namespace
{
 struct FSignPatch { UStaticMeshComponent* Comp=nullptr; UStaticMesh* Mesh=nullptr; int32 Removed=0,Kept=0; };
 bool OldSignPoint(const FVector3f& U)
 {
  const FVector P=S47Three::ToThree(FVector(U));
  return P.X>=-14.76&&P.X<=-14.24&&P.Y>=-.62&&P.Y<=.66&&P.Z>=18.99&&P.Z<=22.61;
 }
 bool OldFoot(double X0,double X1,double Z0,double Z1)
 {return FMath::Abs(X0+14.8)<.025&&FMath::Abs(X1+14.2)<.025&&FMath::Abs(Z0-19.05)<.025&&FMath::Abs(Z1-22.55)<.025;}
 const FVector NewSign(-7.3,-.6,15.5),SignDelta(7.2,0,-5.3);
 struct FBounds {double X0,X1,Y0,Y1,Z0,Z1;};
 // Exact remaining 19 original axis-aligned concrete boxes from immutable g435.
 const FBounds Physics[]={
 {7.7,8,-.6,-.155,-12.6,-11.4},{7.4,7.7,-.6,-.31,-12.6,-11.4},{7.1,7.4,-.6,-.465,-12.6,-11.4},
 {3.55,5.05,-.6,-.48,6.9,9.9},{-4.425,-4.175,-.6,-.3,17.2,18},{-3.025,-2.775,-.6,-.3,17.2,18},
 {-1.35,-.65,-.62,-.56,-6.9,-5.9},{-1.35,-.65,-.62,-.56,-7.95,-6.95},{-1.35,-.65,-.62,-.56,-9,-8},
 {-1.35,-.65,-.62,-.56,-10.05,-9.05},{-1.35,-.65,-.62,-.56,-11.1,-10.1},{-1.35,-.65,-.62,-.56,-12.15,-11.15},
 {-1.35,-.65,-.62,-.56,-13.2,-12.2},{-1.35,-.65,-.62,-.56,-14.25,-13.25},{-1.35,-.65,-.62,-.56,-15.3,-14.3},
 {-1.35,-.65,-.62,-.56,-16.35,-15.35},{-1.35,-.65,-.62,-.56,-17.4,-16.4},{-1.35,-.65,-.62,-.56,-18.45,-17.45},{-1.35,-.65,-.62,-.56,-19.5,-18.5}};
 bool KnownConcrete(const FVector3f& V)
 {
  const FVector P=S47Three::ToThree(FVector(V));if(OldSignPoint(V))return true;
  for(const FBounds& B:Physics)if(P.X>=B.X0-.001&&P.X<=B.X1+.001&&P.Y>=B.Y0-.001&&P.Y<=B.Y1+.001&&P.Z>=B.Z0-.001&&P.Z<=B.Z1+.001)return true;
  return false;
 }
 bool MakeSignPatch(AS47Web* W,UStaticMeshComponent* C,FSignPatch& Out)
 {
  UStaticMesh* Original=C?C->GetStaticMesh():nullptr;
  if(!Original||!Original->bAllowCPUAccess||!Original->GetRenderData()||Original->GetRenderData()->LODResources.Num()!=1)return false;
  const FStaticMeshLODResources& Src=Original->GetRenderData()->LODResources[0];
  const FStaticMeshVertexBuffers& B=Src.VertexBuffers;
  if(Src.GetNumTriangles()!=240||!B.PositionVertexBuffer.GetVertexData()||
     !B.StaticMeshVertexBuffer.GetTangentData()||!B.StaticMeshVertexBuffer.GetTexCoordData()||Src.IndexBuffer.GetIndexDataSize()==0)return false;
  for(uint32 I=0;I<B.PositionVertexBuffer.GetNumVertices();++I)if(!KnownConcrete(B.PositionVertexBuffer.VertexPosition(I)))return false;
  TArray<uint32> Old,Keep;Src.IndexBuffer.GetCopy(Old);Keep.Reserve(Old.Num());
  TArray<FStaticMeshSection> Sections;
  int32 Removed=0;
  for(const FStaticMeshSection& Section:Src.Sections)
  {
   FStaticMeshSection Copy=Section;Copy.FirstIndex=Keep.Num();Copy.NumTriangles=0;
   for(uint32 I=Section.FirstIndex;I<Section.FirstIndex+Section.NumTriangles*3;I+=3)
   {
    if(I+2>=uint32(Old.Num()))return false;
    const uint32 A=Old[I],Bidx=Old[I+1],Cidx=Old[I+2];
    if(FMath::Max3(A,Bidx,Cidx)>=B.PositionVertexBuffer.GetNumVertices())return false;
    if(OldSignPoint(B.PositionVertexBuffer.VertexPosition(A))&&OldSignPoint(B.PositionVertexBuffer.VertexPosition(Bidx))&&OldSignPoint(B.PositionVertexBuffer.VertexPosition(Cidx))) {++Removed;continue;}
    Keep.Append({A,Bidx,Cidx});++Copy.NumTriangles;
   }
   if(Copy.NumTriangles)Sections.Add(Copy);
  }
  if(Removed!=12||Keep.Num()!=228*3||Keep.Num()+Removed*3!=Old.Num())return false;
  // Preserve every other concrete triangle, normal, UV and slot; imported asset is immutable.
  // The imported asset itself is immutable and remains available for atomic fallback.
  UStaticMesh* Cut=NewObject<UStaticMesh>(W,NAME_None,RF_Transient);
  Cut->bAllowCPUAccess=true;Cut->NeverStream=true;Cut->GetStaticMaterials()=Original->GetStaticMaterials();
  // This concrete clone keeps native ray tracing with correctly initialised render geometry.
  Cut->bSupportRayTracing=true;
  auto Data=MakeUnique<FStaticMeshRenderData>();Data->AllocateLODResources(1);
  Data->Bounds=Original->GetRenderData()->Bounds;Data->ScreenSize[0].Default=1;
  Data->NumInlinedLODs=1;Data->CurrentFirstLODIdx=0;Data->bReadyForStreaming=false;
  FStaticMeshLODResources& Dst=Data->LODResources[0];Dst.Sections.Append(Sections);
  Dst.VertexBuffers.PositionVertexBuffer.Init(B.PositionVertexBuffer,true);
  auto& Tangents=Dst.VertexBuffers.StaticMeshVertexBuffer;
  Tangents.SetUseHighPrecisionTangentBasis(B.StaticMeshVertexBuffer.GetUseHighPrecisionTangentBasis());
  Tangents.SetUseFullPrecisionUVs(B.StaticMeshVertexBuffer.GetUseFullPrecisionUVs());
  Tangents.Init(B.StaticMeshVertexBuffer.GetNumVertices(),B.StaticMeshVertexBuffer.GetNumTexCoords(),true);
  for(uint32 I=0;I<B.StaticMeshVertexBuffer.GetNumVertices();++I)
  {
   Tangents.SetVertexTangents(I,FVector3f(B.StaticMeshVertexBuffer.VertexTangentX(I)),B.StaticMeshVertexBuffer.VertexTangentY(I),FVector3f(B.StaticMeshVertexBuffer.VertexTangentZ(I)));
   for(uint32 U=0;U<B.StaticMeshVertexBuffer.GetNumTexCoords();++U)Tangents.SetVertexUV(I,U,B.StaticMeshVertexBuffer.GetVertexUV(I,U));
  }
  const uint32 Colors=B.ColorVertexBuffer.GetNumVertices();
  if(Colors)
  {
   if(!B.ColorVertexBuffer.GetVertexData())return false;
   Dst.VertexBuffers.ColorVertexBuffer.Init(Colors,true);
   for(uint32 I=0;I<Colors;++I)Dst.VertexBuffers.ColorVertexBuffer.VertexColor(I)=B.ColorVertexBuffer.VertexColor(I);
  }
  Dst.bHasColorVertexData=Colors>0;Dst.bBuffersInlined=true;Dst.bIsOptionalLOD=false;
  Dst.SourceMeshBounds=Data->Bounds;Dst.MaxDeviation=0;Dst.IndexBuffer.SetIndices(Keep,EIndexBufferStride::Force32Bit);
  // Same runtime RT-proxy construction policy as UStaticMesh::BuildFromMeshDescriptions.
  // If enabled, InitResources builds its geometry from these indices without renderer overrides.
  bool bInitRayTracing=Cut->bSupportRayTracing;
#if !WITH_EDITOR
  bInitRayTracing=bInitRayTracing&&IsRayTracingAllowed();
#endif
  if(bInitRayTracing)Data->InitializeRayTracingRepresentationFromRenderingLODs();
  Cut->SetRenderData(MoveTemp(Data));Cut->SetExtendedBounds(Original->GetBounds());
  Out={C,Cut,Removed,Keep.Num()/3};return true;
 }

}

TSharedPtr<FS47SaroArrival> FS47SaroArrival::Build(AS47Web* W)
{
 if(!W)return {};
 const int32 Parent=W->Handle(TEXT("world.grounds.group")),Room=W->Handle(TEXT("room.group"));
 const FS47WebNode* G=W->Node(Parent);if(!G||!G->Comp.IsValid())return {};
 UStaticMesh* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 UMaterialInterface* Asphalt=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/MesaDiner/Materials/M_Mesa_Asphalt.M_Mesa_Asphalt"));
 UMaterialInterface* GlassBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/MesaDiner/Materials/M_Mesa_FacadeGlass.M_Mesa_FacadeGlass"));
 if(!Cube||!Base||!Asphalt||!GlassBase)return {};
 auto Under=[W](int32 I,int32 P){for(;I>=0;I=W->Node(I)->Parent)if(I==P)return true;return false;};
 UStaticMeshComponent* Concrete=nullptr,*Face=nullptr,*Pane=nullptr;int32 FaceNode=-1;
 for(int32 I=0;I<W->PartCount();++I)
 {
  const FS47WebNode* N=W->Node(I);if(!N)continue;
  UStaticMeshComponent* C=Cast<UStaticMeshComponent>(N->Comp.Get());
  if(Under(I,Parent)&&N->Geo==435)Concrete=C;
  if(Under(I,Parent)&&N->Geo==443){Face=C;FaceNode=I;}
  if(N->Parent==Room&&N->Geo==78)Pane=C;
 }
 FSignPatch Patch;
 if(!Concrete||!Face||!Face->GetStaticMesh()||!Face->GetMaterial(0)||!Pane||!MakeSignPatch(W,Concrete,Patch))
 {UE_LOG(LogS47SaroArrival,Error,TEXT("S47: SARO arrival rejected: exact g435/g443/window or CPU render data unavailable; complete original retained"));return {};}
 int32 WalkCount=0,DriveCount=0;
 for(const FS47Rect& R:W->CollidersMutable(TEXT("saro")))if(OldFoot(R.MinX,R.MaxX,R.MinZ,R.MaxZ))++WalkCount;
 const TSharedPtr<FJsonObject>& Drive=W->DriveData();const TSharedPtr<FJsonObject>* Legs=nullptr,*Leg=nullptr;const TArray<TSharedPtr<FJsonValue>>* Obs=nullptr;
 if(Drive.IsValid()&&Drive->TryGetObjectField(TEXT("legs"),Legs)&&(*Legs)->TryGetObjectField(TEXT("saro"),Leg))(*Leg)->TryGetArrayField(TEXT("obstacles"),Obs);
 if(Obs)for(const TSharedPtr<FJsonValue>& V:*Obs){const auto& A=V->AsArray();if(A.Num()==4&&OldFoot(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber(),A[3]->AsNumber()))++DriveCount;}
 bool Heights=true;for(double X:{-18.2,-12.8,-7.8})for(double Z:{10.5,14.3,20.5})Heights&=FMath::Abs(US47Night::SaroFloorAt(W,X,Z)+.6)<1e-8;
 if(WalkCount!=1||DriveCount!=1||!Heights){UE_LOG(LogS47SaroArrival,Error,TEXT("S47: SARO arrival rejected: sign collider counts %d/%d or existing flat floor mismatch"),WalkCount,DriveCount);return {};}
 TSharedPtr<FS47SaroArrival> S=MakeShared<FS47SaroArrival>();S->Web=W;S->OldConcrete=Concrete;S->OldSignFace=Face;S->RemovedTriangles=Patch.Removed;
 TArray<TWeakObjectPtr<UPrimitiveComponent>> Floors;
 auto Mat=[W,Base](uint32 Color,float Rough=.85f,float Metal=0.f)
 {
  UMaterialInstanceDynamic* M=UMaterialInstanceDynamic::Create(Base,W);M->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(Color));M->SetScalarParameterValue(TEXT("NativeLighting"),1);
  M->SetScalarParameterValue(TEXT("Roughness"),Rough);M->SetScalarParameterValue(TEXT("Metalness"),Metal);M->SetScalarParameterValue(TEXT("EmissivePower"),0);return M;
 };
 UMaterialInstanceDynamic* Kerb=Mat(0x8c8776,.92),*Paint=Mat(0xb6ad8a,.95),*Metal=Mat(0x777f79,.48,.7),*Cream=Mat(0xb4ac96,.91),*Gravel=Mat(0x8f8065,.98);
 auto Box=[&](const TCHAR* Name,const FVector& At,const FVector& Size,UMaterialInterface* Material,bool Floor=false)
 {
  UStaticMeshComponent* C=NewObject<UStaticMeshComponent>(W);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Cube);C->SetMaterial(0,Material);C->SetupAttachment(G->Comp.Get());C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCastShadow(!Floor);C->SetHiddenInGame(true);C->RegisterComponent();
  C->SetWorldLocation(S47Three::ToUE(At));C->SetWorldScale3D(S47Three::DirToUE(Size));
  const int32 I=W->AdoptNode(C,Parent,FString(TEXT("arrival:"))+Name);W->ObjAt(I).SetScale(Size);S->Parts.Add(C);if(Floor)Floors.Add(C);return C;
 };
 S->Parking=Box(TEXT("visitorLot"),FVector(-13,-.66,15.5),FVector(10.4,.12,10),Asphalt,true);
 Box(TEXT("entryApron"),FVector(-13,-.66,22.0),FVector(7,.12,3),Asphalt,true);
 for(double X:{-17.3,-14.3,-11.3,-8.3})Box(TEXT("bayLine"),FVector(X,-.597,14.3),FVector(.065,.004,5.5),Paint);
 for(double X:{-15.8,-12.8,-9.8})
 {
  Box(TEXT("bayEndLine"),FVector(X,-.597,11.55),FVector(3,.004,.065),Paint);
  Box(TEXT("wheelStop"),FVector(X,-.54,11.2),FVector(1.8,.12,.18),Kerb);
 }
 Box(TEXT("westKerb"),FVector(-18.25,-.57,15.5),FVector(.10,.18,10.10),Kerb);
 Box(TEXT("northKerb"),FVector(-13,-.57,10.45),FVector(10.6,.18,.10),Kerb);
 for(const FVector& Span:{FVector(10.5,11.6,0),FVector(12.8,20.5,0)})Box(TEXT("eastKerb"),FVector(-7.75,-.57,(Span.X+Span.Y)*.5),FVector(.10,.18,Span.Y-Span.X),Kerb);
 for(const FVector& Span:{FVector(-18.2,-16.5,0),FVector(-9.5,-7.8,0)})Box(TEXT("entryKerb"),FVector((Span.X+Span.Y)*.5,-.57,20.55),FVector(Span.Y-Span.X,.18,.10),Kerb);
 // Clear pedestrian strip, then skirt the annex and join the original truck ramp at its foot.
 Box(TEXT("pedestrianStrip"),FVector(-8.3,-.619,15.5),FVector(.85,.046,10),Gravel,true);
 Box(TEXT("annexApproach"),FVector(-1.2,-.619,12.2),FVector(14.2,.046,1.1),Kerb,true);
 Box(TEXT("rampApproachNorth"),FVector(5.4,-.619,14.75),FVector(1.1,.046,5.1),Kerb,true);
 Box(TEXT("rampApproachEast"),FVector(8.45,-.619,17.3),FVector(6.1,.046,1.1),Kerb,true);
 // Low shouldered monument stays wholly outside both driving and pedestrian corridors.
 Box(TEXT("signFoundation"),FVector(-7.3,-.59,15.5),FVector(.50,.18,3.50),Kerb);
 Box(TEXT("signMonument"),FVector(-7.3,.06,15.5),FVector(.40,1.32,3.40),Cream);
 Box(TEXT("signCap"),FVector(-7.3,.755,15.5),FVector(.50,.07,3.50),Kerb);
 for(double X:{-7.54,-7.06})
 {
  for(double Z:{13.91,17.09})Box(TEXT("signVerticalFrame"),FVector(X,.12,Z),FVector(.022,1.09,.035),Metal);
  for(double Y:{-.42,.66})Box(TEXT("signHorizontalFrame"),FVector(X,Y,15.5),FVector(.022,.035,3.21),Metal);
 }
 UStaticMeshComponent* Sign=NewObject<UStaticMeshComponent>(W);Sign->SetMobility(EComponentMobility::Movable);Sign->SetStaticMesh(Face->GetStaticMesh());Sign->SetMaterial(0,Face->GetMaterial(0));Sign->SetCollisionEnabled(ECollisionEnabled::NoCollision);Sign->SetCastShadow(false);Sign->SetHiddenInGame(true);Sign->SetupAttachment(G->Comp.Get());Sign->RegisterComponent();
 FTransform SignPose=Face->GetComponentTransform();SignPose.AddToTranslation(S47Three::ToUE(SignDelta));Sign->SetWorldTransform(SignPose);W->AdoptNode(Sign,Parent,TEXT("arrival:legibleSiteSign"));S->Parts.Add(Sign);
 // Existing glazing geometry keeps its exact aperture, with a restrained physically lit glass.
 UMaterialInstanceDynamic* Glass=UMaterialInstanceDynamic::Create(GlassBase,W);Glass->SetVectorParameterValue(TEXT("Color"),FLinearColor(.44,.52,.53,1));Glass->SetVectorParameterValue(TEXT("EmissiveFactor"),FLinearColor::Black);Glass->SetScalarParameterValue(TEXT("Opacity"),.045);Glass->SetScalarParameterValue(TEXT("Roughness"),.12);S->WindowGlass=Glass;
 for(int32 K=0;K<=6;++K)Box(TEXT("exteriorWindowBead"),FVector(-5.4+K*1.8,1.85,-4.685),FVector(.035,1.78,.04),Metal);
 for(double Y:{.945,2.755})Box(TEXT("windowDripFrame"),FVector(0,Y,-4.685),FVector(10.94,.055,.08),Metal);
 Box(TEXT("interiorWindowSill"),FVector(0,.938,-4.31),FVector(10.9,.035,.36),Cream);
 // Exact old concrete collision is rebuilt as axis-aligned floor boxes, excluding only the sign.
 for(const FBounds& B:Physics)
 {
  UBoxComponent* Q=NewObject<UBoxComponent>(W);Q->SetMobility(EComponentMobility::Movable);Q->SetBoxExtent(S47Three::DirToUE(FVector((B.X1-B.X0)*50,(B.Y1-B.Y0)*50,(B.Z1-B.Z0)*50)));Q->SetCollisionEnabled(ECollisionEnabled::NoCollision);Q->SetCollisionResponseToAllChannels(ECR_Ignore);Q->SetHiddenInGame(true);Q->SetupAttachment(G->Comp.Get());Q->RegisterComponent();Q->SetWorldLocation(S47Three::ToUE(FVector((B.X0+B.X1)*.5,(B.Y0+B.Y1)*.5,(B.Z0+B.Z1)*.5)));Floors.Add(Q);
 }
 // Commit only after exact source, both collision lists and all nine floor probes validated.
 Patch.Mesh->InitResources();Concrete->SetStaticMesh(Patch.Mesh);Concrete->SetCollisionEnabled(ECollisionEnabled::NoCollision);W->SetVisible(FaceNode,false);Face->SetCollisionEnabled(ECollisionEnabled::NoCollision);Pane->SetMaterial(0,Glass);
 for(FS47Rect& R:W->CollidersMutable(TEXT("saro")))if(OldFoot(R.MinX,R.MaxX,R.MinZ,R.MaxZ)){R.MinX=-7.55;R.MaxX=-7.05;R.MinZ=13.75;R.MaxZ=17.25;++S->WalkMoved;}
 TArray<TSharedPtr<FJsonValue>> NewObs=*Obs;
 for(TSharedPtr<FJsonValue>& V:NewObs){const auto& A=V->AsArray();if(A.Num()==4&&OldFoot(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber(),A[3]->AsNumber())){TArray<TSharedPtr<FJsonValue>> NewBox;for(double D:{-7.55,-7.05,13.75,17.25})NewBox.Add(MakeShared<FJsonValueNumber>(D));V=MakeShared<FJsonValueArray>(NewBox);++S->DriveMoved;}}
 (*Leg)->SetArrayField(TEXT("obstacles"),NewObs);
 S->FloorQueries=Floors;
 for(const auto& Weak:Floors)if(UPrimitiveComponent* Q=Weak.Get()){Q->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Q->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);}
 for(const auto& Weak:S->Parts)if(UPrimitiveComponent* C=Weak.Get())C->SetHiddenInGame(false);
 S->Update();UE_LOG(LogS47SaroArrival,Display,TEXT("S47: SARO arrival ready: planar lot top -0.600, personal bay -12.8/14.3, sign -7.3/15.5, exact concrete cut %d+%d, walk/drive moved %d/%d; original ramps and room apertures preserved"),Patch.Kept,Patch.Removed,S->WalkMoved,S->DriveMoved);return S;
}

void FS47SaroArrival::Update()
{
 AS47Web* W=Web.Get();if(!W)return;const bool Shown=W->Area()==TEXT("saro");
 bLastShown=Shown;
 for(const auto& Weak:FloorQueries)if(UPrimitiveComponent* C=Weak.Get())C->SetCollisionEnabled(Shown?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
 for(const auto& Weak:Parts)if(UPrimitiveComponent* C=Weak.Get())C->SetVisibility(Shown,false);
}

bool FS47SaroArrival::Audit(TArray<FString>& Lines)const
{
 bool Ok=true;auto Check=[&](bool Pass,const FString& Why){Lines.Add(FString::Printf(TEXT("%s SARO arrival %s"),Pass?TEXT("OK"):TEXT("FAIL"),*Why));Ok&=Pass;};
 Check(WalkMoved==1&&DriveMoved==1,TEXT("monument moved in walk and drive data atomically"));Check(RemovedTriangles==12,TEXT("exact old sign cube removed,228 concrete triangles preserved"));
 const UStaticMeshComponent* C=Parking.Get();Check(C&&FMath::Abs(C->Bounds.GetBox().Max.Z/100+.6)<.0001,TEXT("actual planar lot top -0.600 metres"));
 if(const AS47Web* W=Web.Get())for(double X:{-18.2,-12.8,-7.8})for(double Z:{10.5,14.3,20.5})Check(FMath::Abs(US47Night::SaroFloorAt(W,X,Z)+.6)<1e-8,TEXT("existing SARO floor equals new parking plane"));
 Check(WindowGlass.IsValid(),TEXT("restrained lit transparent window, no opaque/emissive backing"));return Ok;
}
