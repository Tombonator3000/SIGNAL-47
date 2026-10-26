#include "S47ExteriorFixtures.h"
#include "S47Web.h"
#include "S47Obj.h"
#include "S47Three.h"
#include "S47Main.h"
#include "S47SaroNight.h"
#include "S47SaroPower.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StaticMeshResources.h"
#include "RenderUtils.h"
#include "Misc/CommandLine.h"

namespace S47ExteriorFixtures
{
namespace
{
 const TCHAR* AssetNames[]={TEXT("SM_Exterior_WallPack"),TEXT("SM_Exterior_Canopy"),TEXT("SM_Exterior_SignWash")};
 const FVector EmitterAt[]={{0,-.037298783088,.18516261028},{0,-.105,0},{0,.117325901808,.057325901808}};
 const FVector LensNormal[]={{0,-.287347886,.957826285},{0,-1,0},{0,.707106781187,.707106781187}};
 const FVector2D LensSize[]={{.285,.18},{.51,.18},{.16,.068}};
 const FVector BoundsMin[]={{-.18,-.149400011,0},{-.30,-.099174999,-.12},{-.103174999,0,-.076942571}};
 const FVector BoundsMax[]={{.18,.148,.213571727},{.30,0,.12},{.103174999,.148085043,.088085048}};
 const int32 Triangles[][3]={{4428,1808,480},{2820,1292,508},{3356,1628,692}};
 bool Disabled(){return FParse::Param(FCommandLine::Get(),TEXT("S47ExteriorFixturesOff"));}
 UStaticMesh* Asset(EKind Kind)
 {
  const TCHAR* Name=AssetNames[int32(Kind)];
  auto* M=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Signal47/Art/ExteriorFixtures/%s.%s"),Name,Name));
  if(!M||!M->GetRenderData()||M->GetRenderData()->LODResources.Num()!=3||M->GetStaticMaterials().Num()!=5)return nullptr;
  for(int32 L=0;L<3;++L)if(M->GetNumTriangles(L)!=Triangles[int32(Kind)][L])return nullptr;
  const FVector Center=S47Three::ToUE((BoundsMin[int32(Kind)]+BoundsMax[int32(Kind)])*.5);
  const FVector Extent=S47Three::ToUE((BoundsMax[int32(Kind)]-BoundsMin[int32(Kind)])*.5);
  if(!M->GetBounds().Origin.Equals(Center,.1)||!M->GetBounds().BoxExtent.Equals(Extent,.1))return nullptr;
  const int32 Slot=M->GetMaterialIndex(TEXT("M_ExteriorFixtures_Lens"));
  FLinearColor Emission;
  if(Slot<0||!M->GetMaterial(Slot)||M->GetMaterial(Slot)->GetBlendMode()!=BLEND_Opaque||
     !M->GetMaterial(Slot)->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveFactor")),Emission))return nullptr;
  return M;
 }
 struct FPatch { UStaticMeshComponent* Component=nullptr;UStaticMesh* Cut=nullptr;int32 Removed=0; };
 // Immutable export boxes. Bounds are Three metres, measured from the original GLB.
 bool Patch(AS47Web* W,int32 Geo,int32 OriginalTris,const TArray<FBox>& Boxes,FPatch& Out)
 {
  UStaticMeshComponent* C=nullptr;
  for(int32 I=0;I<W->PartCount();++I)if(const auto* N=W->Node(I);N&&N->Geo==Geo)
  {
   if(C||!W->NodeWorldThree(I).Equals(FTransform::Identity,.0001))return false;
   C=Cast<UStaticMeshComponent>(N->Comp.Get());
  }
  UStaticMesh* Original=C?C->GetStaticMesh():nullptr;
  if(!Original||!Original->bAllowCPUAccess||!Original->GetRenderData()||Original->GetRenderData()->LODResources.Num()!=1)return false;
  const auto& Src=Original->GetRenderData()->LODResources[0];const auto& B=Src.VertexBuffers;
  if(Src.GetNumTriangles()!=OriginalTris||!B.PositionVertexBuffer.GetVertexData()||
     !B.StaticMeshVertexBuffer.GetTangentData()||!B.StaticMeshVertexBuffer.GetTexCoordData()||Src.IndexBuffer.GetIndexDataSize()==0)return false;
  TArray<uint32> Old,Keep;Src.IndexBuffer.GetCopy(Old);Keep.Reserve(Old.Num());
  TArray<int32> Counts;Counts.Init(0,Boxes.Num());TArray<FStaticMeshSection> Sections;
  for(const FStaticMeshSection& Section:Src.Sections)
  {
   FStaticMeshSection Copy=Section;Copy.FirstIndex=Keep.Num();Copy.NumTriangles=0;
   for(uint32 I=Section.FirstIndex;I<Section.FirstIndex+Section.NumTriangles*3;I+=3)
   {
    if(I+2>=uint32(Old.Num()))return false;
    const uint32 A=Old[I],Bb=Old[I+1],Cc=Old[I+2];
    if(FMath::Max3(A,Bb,Cc)>=B.PositionVertexBuffer.GetNumVertices())return false;
    int32 Box=-1;
    for(int32 J=0;J<Boxes.Num();++J)if(Boxes[J].ExpandBy(.0002).IsInsideOrOn(S47Three::ToThree(FVector(B.PositionVertexBuffer.VertexPosition(A))))&&
      Boxes[J].ExpandBy(.0002).IsInsideOrOn(S47Three::ToThree(FVector(B.PositionVertexBuffer.VertexPosition(Bb))))&&
      Boxes[J].ExpandBy(.0002).IsInsideOrOn(S47Three::ToThree(FVector(B.PositionVertexBuffer.VertexPosition(Cc))))){Box=J;break;}
    if(Box>=0){++Counts[Box];continue;}
    Keep.Append({A,Bb,Cc});++Copy.NumTriangles;
   }
   if(Copy.NumTriangles)Sections.Add(Copy);
  }
  for(int32 Count:Counts)if(Count!=12)return false;
  if(Keep.Num()+Boxes.Num()*36!=Old.Num())return false;
  // Same measured runtime render-data clone as S47SaroArrival's concrete trim. Keep the
  // original collision body, all retained vertices/normals/UVs/colours/slots and native RT.
  auto* Cut=NewObject<UStaticMesh>(W,NAME_None,RF_Transient);
  Cut->bAllowCPUAccess=true;Cut->NeverStream=true;Cut->bSupportRayTracing=true;
  Cut->GetStaticMaterials()=Original->GetStaticMaterials();Cut->SetBodySetup(Original->GetBodySetup());
  auto Data=MakeUnique<FStaticMeshRenderData>();Data->AllocateLODResources(1);
  Data->Bounds=Original->GetRenderData()->Bounds;Data->ScreenSize[0].Default=1;
  Data->NumInlinedLODs=1;Data->CurrentFirstLODIdx=0;Data->bReadyForStreaming=false;
  auto& Dst=Data->LODResources[0];Dst.Sections.Append(Sections);
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
  bool InitRT=Cut->bSupportRayTracing;
#if !WITH_EDITOR
  InitRT=InitRT&&IsRayTracingAllowed();
#endif
  if(InitRT)Data->InitializeRayTracingRepresentationFromRenderingLODs();
  Cut->SetRenderData(MoveTemp(Data));Cut->SetExtendedBounds(Original->GetBounds());
  Out={C,Cut,Boxes.Num()*12};return true;
 }
}

FFixture Make(AS47Web* W,const TCHAR* Name,EKind Kind,const FVector& Mount,const FQuat& Rotation,int32 Parent)
{
 FFixture F;if(!W||Disabled())return F;
 UStaticMesh* M=Asset(Kind);const auto* P=W->Node(Parent);if(!M||!P||!P->Comp.IsValid())return F;
 USceneComponent* ParentComponent=P->Comp.Get();
 auto* C=NewObject<UStaticMeshComponent>(W,*FString::Printf(TEXT("S47Fixture_%s"),Name));
 C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(ParentComponent);C->SetStaticMesh(M);
 C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);
 C->SetCastShadow(true);C->RegisterComponent();
 double Matrix[16];S47Three::Compose(Mount,Rotation,FVector::OneVector,Matrix);C->SetWorldTransform(S47Three::MatToUE(Matrix));
 const int32 Slot=M->GetMaterialIndex(TEXT("M_ExteriorFixtures_Lens"));
 auto* Lens=UMaterialInstanceDynamic::Create(M->GetMaterial(Slot),W);Lens->SetVectorParameterValue(TEXT("EmissiveFactor"),FLinearColor::Black);C->SetMaterial(Slot,Lens);
 W->AdoptNode(C,Parent,FString(TEXT("exteriorFixture:"))+Name);
 const FVector Normal=Rotation.RotateVector(LensNormal[int32(Kind)]).GetSafeNormal();
 F.Mesh=C;F.Lens=Lens;F.Normal=Normal;F.Aperture=Mount+Rotation.RotateVector(EmitterAt[int32(Kind)]);
 F.Width=LensSize[int32(Kind)].X*100;F.Height=LensSize[int32(Kind)].Y*100;
 return F;
}
void FFixture::Update(bool Shown,float Power,const FLinearColor& Color) const
{
 if(auto* C=Mesh.Get())C->SetVisibility(Shown,false);
 if(auto* D=Lens.Get())D->SetVectorParameterValue(TEXT("EmissiveFactor"),Color*(Shown?1.5f*FMath::Max(0.f,Power):0.f));
}
float SaroPower(AS47Web* W)
{
 const auto* M=US47Main::Get(W);
 return M&&M->SaroNight&&M->SaroNight->Power?float(FMath::Clamp(M->SaroNight->Power->Sodium,0.,1.)):1.f;
}
TMap<FName,FFixture> ReplaceLegacy(AS47Web* W)
{
 TMap<FName,FFixture> Result;if(!W||Disabled()||!Asset(EKind::WallPack)||!Asset(EKind::Canopy))return Result;
 const int32 Ext=W->Handle(TEXT("ext.group")),Annex=W->Handle(TEXT("annex.group")),Yard=W->Handle(TEXT("yard.group"));
 if(Ext<0||Annex<0||Yard<0)return Result;
 FPatch ExtPatch,AnnexPatch,YardPatch;
 if(!Patch(W,18,72,TArray<FBox>{FBox(FVector(-6.575,2.475,1.875),FVector(-6.225,2.725,2.125)),
    FBox(FVector(-6.575,2.475,-3.125),FVector(-6.225,2.725,-2.875)),
    FBox(FVector(20.725,2.675,5.675),FVector(21.075,2.925,5.925)),
    FBox(FVector(30.225,2.675,5.675),FVector(30.575,2.925,5.925))},ExtPatch)||
    !Patch(W,234,1896,TArray<FBox>{FBox(FVector(12.33,2.65,-2.1),FVector(12.63,2.85,-1.9))},YardPatch)||
    !Patch(W,314,84,TArray<FBox>{FBox(FVector(-2.175,2.275,10.795),FVector(-1.825,2.525,11.045)),
    FBox(FVector(-6.6,2.22,5.4),FVector(-6.3,2.28,6.))},AnnexPatch))
 {UE_LOG(LogTemp,Warning,TEXT("S47: exterior fixture legacy geometry differs or lacks CPU data; original housings retained"));return Result;}
 const FQuat West=S47Three::Euler(0,-UE_DOUBLE_PI/2,0),Front=FQuat::Identity;
 Result.Add(TEXT("WestSouth"),Make(W,TEXT("WestSouth"),EKind::WallPack,{-6.3,2.6,2},West,Ext));
 Result.Add(TEXT("WestNorth"),Make(W,TEXT("WestNorth"),EKind::WallPack,{-6.3,2.6,-3},West,Ext));
 Result.Add(TEXT("WingWest"),Make(W,TEXT("WingWest"),EKind::WallPack,{20.9,2.8,5.5},Front,Ext));
 // Move clear of the sign backing so the complete mounting plate bears on masonry.
 Result.Add(TEXT("WingEast"),Make(W,TEXT("WingEast"),EKind::WallPack,{30.8,2.8,5.5},Front,Ext));
 Result.Add(TEXT("LabDoor"),Make(W,TEXT("LabDoor"),EKind::WallPack,{12.6,2.58,-2},West,Yard));
 Result.Add(TEXT("AnnexSouth"),Make(W,TEXT("AnnexSouth"),EKind::WallPack,{-2,2.4,10.8},Front,Annex));
 Result.Add(TEXT("FireExit"),Make(W,TEXT("EntranceCanopy"),EKind::Canopy,{-7.05,2.4,5.7},Front,Annex));
 for(const auto& Pair:Result)if(!Pair.Value)
 {
  for(const auto& Built:Result)if(auto* C=Built.Value.Mesh.Get())C->DestroyComponent();
  return {};
 }
 for(const FPatch& P:{ExtPatch,YardPatch,AnnexPatch}){P.Cut->InitResources();P.Component->SetStaticMesh(P.Cut);}
 UE_LOG(LogTemp,Display,TEXT("S47: detailed exterior fixtures ready=7; exact old housings removed=84tri; retained triangles, collision, materials and RT preserved"));
 return Result;
}
}
