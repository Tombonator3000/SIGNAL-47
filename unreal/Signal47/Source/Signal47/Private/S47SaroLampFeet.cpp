#include "S47SaroLampFeet.h"
#include "S47Web.h"
#include "S47Three.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace S47SaroLampFeet
{
namespace
{
 // The fourth yard lamp is supported by the truck pad and is deliberately absent.
 const FVector2D Feet[] = {{11.25,1.6},{11.25,-6},{7.75,-9.2}};
 const TCHAR* Names[] = {TEXT("saroLampFeet:concrete"),TEXT("saroLampFeet:plates"),TEXT("saroLampFeet:hardware")};
 constexpr double HalfWidth=.18, GroundY=-.62, BottomY=-.645, ConcreteTop=-.026, PlateTop=.002;
 constexpr double DeckClearance=.25-HalfWidth; // .07 m outside x=11 / x=8; also preserves walker-centre clearance.

 bool Fail(const TCHAR* Why)
 {
  UE_LOG(LogTemp,Warning,TEXT("S47: SARO lamp feet skipped: %s; original lamps retained"),Why);
  return false;
 }
 bool Under(AS47Web* W,int32 I,int32 Parent)
 {
  for (;I>=0;)
  {
   if(I==Parent)return true;
   const FS47WebNode* N=W->Node(I);if(!N)return false;I=N->Parent;
  }
  return false;
 }
 UStaticMeshComponent* Geometry(AS47Web* W,int32 Parent,int32 Geo)
 {
  UStaticMeshComponent* Found=nullptr;
  for(int32 I=0;I<W->PartCount();++I)
   if(const FS47WebNode* N=W->Node(I);N&&N->Geo==Geo&&Under(W,I,Parent))
   {
    if(Found)return nullptr;
    Found=Cast<UStaticMeshComponent>(N->Comp.Get());if(!Found)return nullptr;
   }
  return Found;
 }
 bool UnitShape(UStaticMesh* Mesh)
 {
  // Engine shapes are 100 cm across, centred, with cylinder axis Z. Validate, do not assume scale.
  return Mesh && Mesh->GetBounds().Origin.Equals(FVector::ZeroVector,.01) &&
   Mesh->GetBounds().BoxExtent.Equals(FVector(50),.01);
 }
 bool InstancedMaterial(UMaterialInterface* Material)
 {
  // Prepare this existing material's ISM permutation before cooking; never accept the default grid.
  const UMaterial* Base=Material?Material->GetMaterial():nullptr;
  return Base&&Base->bUsedWithInstancedStaticMeshes&&Material->CheckMaterialUsage_Concurrent(MATUSAGE_InstancedStaticMeshes);
 }
 bool Contact(UStaticMeshComponent* Ground,UStaticMeshComponent* Pole)
 {
  FCollisionQueryParams Query(SCENE_QUERY_STAT(S47SaroLampFeetContact),true);
  for(const FVector2D& P:Feet)
  {
   // Trace the original pole component upward into its actual bottom cap, not its group pivot.
   FHitResult Foot;
   if(!Pole->LineTraceComponent(Foot,S47Three::ToUE(P.X,-.20,P.Y),S47Three::ToUE(P.X,.06,P.Y),Query) ||
      FMath::Abs(S47Three::ToThree(Foot.ImpactPoint).Y)>.001)return false;
   for(const FVector2D& D:{FVector2D(0,0),FVector2D(-HalfWidth,-HalfWidth),FVector2D(-HalfWidth,HalfWidth),FVector2D(HalfWidth,-HalfWidth),FVector2D(HalfWidth,HalfWidth)})
   {
    FHitResult Land;
    // Direct g4 trace cannot accidentally hit a slab, new pedestal, railing or collision proxy.
    if(!Ground->LineTraceComponent(Land,S47Three::ToUE(P.X+D.X,.1,P.Y+D.Y),S47Three::ToUE(P.X+D.X,-1,P.Y+D.Y),Query) ||
       FMath::Abs(S47Three::ToThree(Land.ImpactPoint).Y-GroundY)>.0025)return false;
   }
  }
  return true;
 }
 void Add(UInstancedStaticMeshComponent* C,double X,double Y,double Z,double Width,double Height,double Depth)
 {
  C->AddInstance(FTransform(FQuat::Identity,S47Three::ToUE(X,Y,Z),FVector(Width,Depth,Height)),false);
 }
 bool Bounds(UInstancedStaticMeshComponent* C,int32 Instance,const FVector& Low,const FVector& High)
 {
  FTransform T;if(!C->GetInstanceTransform(Instance,T,true))return false;
  const FBoxSphereBounds B=C->GetStaticMesh()->GetBounds();
  const FBox Actual=FBox(B.Origin-B.BoxExtent,B.Origin+B.BoxExtent).TransformBy(T);
  return Actual.Min.Equals(S47Three::ToUE(Low),.05) && Actual.Max.Equals(S47Three::ToUE(High),.05);
 }
}

bool Build(AS47Web* W)
{
 if(!W||FParse::Param(FCommandLine::Get(),TEXT("S47SaroLampFeetOff"))||FParse::Param(FCommandLine::Get(),TEXT("S47PropsOff")))return false;
 const int32 Parent=W->Handle(TEXT("yard.group")),Ext=W->Handle(TEXT("ext.group"));
 const FS47WebNode* Yard=W->Node(Parent);
 if(!Yard||!Yard->Comp.IsValid()||!W->NodeWorldThree(Parent).Equals(FTransform::Identity,.0001))return Fail(TEXT("yard parent/transform differs"));
 USceneComponent* ParentComponent=Yard->Comp.Get();
 UStaticMeshComponent* Pole=Geometry(W,Parent,235),*Glass=Geometry(W,Parent,236),*Ground=Geometry(W,Ext,4);
 if(!Pole||!Glass||!Ground||!Pole->GetStaticMesh()||!Glass->GetStaticMesh()||
    Pole->GetStaticMesh()->GetNumTriangles(0)!=304||Glass->GetStaticMesh()->GetNumTriangles(0)!=48||
    !Pole->GetComponentTransform().Equals(FTransform::Identity,.001)||!Glass->GetComponentTransform().Equals(FTransform::Identity,.001))return Fail(TEXT("original pole/glass geometry differs"));
 if(!Contact(Ground,Pole))return Fail(TEXT("actual pole-bottom or terrain contact differs"));

 // Idempotence without touching an unexpected or partial old set.
 int32 Existing[3]={0,0,0};bool ValidExisting=true;
 const int32 Expected[3]={3,3,27};
 for(int32 I=0;I<W->PartCount();++I)
  if(const FS47WebNode* N=W->Node(I))for(int32 K=0;K<3;++K)if(N->Name==Names[K])
  {
   ++Existing[K];const auto* C=Cast<UInstancedStaticMeshComponent>(N->Comp.Get());
   ValidExisting&=C&&N->Parent==Parent&&C->GetInstanceCount()==Expected[K];
  }
 if(Existing[0]+Existing[1]+Existing[2])
  return Existing[0]==1&&Existing[1]==1&&Existing[2]==1&&ValidExisting ? true : Fail(TEXT("partial or changed existing set"));
 for(const FS47Rect& R:W->CollidersMutable(TEXT("saro")))if(R.Id.StartsWith(TEXT("saroLampFeet:")))return Fail(TEXT("orphaned foot collider"));

 UStaticMesh* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 UStaticMesh* Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 UMaterialInterface* Concrete=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SaroSite/Materials/M_SaroSite_Concrete.M_SaroSite_Concrete"));
 UMaterialInterface* Metal=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SubstationDetail/Materials/M_SubstationDetail_GalvanizedSteel.M_SubstationDetail_GalvanizedSteel"));
 if(!UnitShape(Cube)||!UnitShape(Cylinder)||!InstancedMaterial(Concrete)||!InstancedMaterial(Metal)||Concrete->GetBlendMode()!=BLEND_Opaque||Metal->GetBlendMode()!=BLEND_Opaque)return Fail(TEXT("existing shape/material or saved ISM permutation unavailable"));

 // Reuse the project's SubstationDetail baseplate/washer/bolt recipe with shared native shapes.
 // Round bolt heads are intentional here; no claim that these engine cylinders are hexagonal.
 UInstancedStaticMeshComponent* Created[3]={};
 auto Batch=[&](int32 K,UStaticMesh* Mesh,UMaterialInterface* Mat)
 {
  auto* C=NewObject<UInstancedStaticMeshComponent>(W);
  C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(ParentComponent);C->SetRelativeTransform(FTransform::Identity);
  C->SetStaticMesh(Mesh);C->SetMaterial(0,Mat);C->SetCastShadow(true);
  C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);
  C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);
  C->RegisterComponent();Created[K]=C;return C;
 };
 auto* Bases=Batch(0,Cube,Concrete);auto* Plates=Batch(1,Cube,Metal);auto* Hardware=Batch(2,Cylinder,Metal);
 for(const FVector2D& P:Feet)
 {
  Add(Bases,P.X,(BottomY+ConcreteTop)*.5,P.Y,2*HalfWidth,ConcreteTop-BottomY,2*HalfWidth);
  Add(Plates,P.X,(ConcreteTop+PlateTop)*.5,P.Y,.30,PlateTop-ConcreteTop,.30);
  Add(Hardware,P.X,.0435,P.Y,.21,.083,.21); // sleeve: .002..085, overlaps original .18 m pole
  for(double DX:{-.112,.112})for(double DZ:{-.112,.112})
  {
   Add(Hardware,P.X+DX,.004,P.Y+DZ,.05,.004,.05); // washer: plate top..006
   Add(Hardware,P.X+DX,.016,P.Y+DZ,.036,.020,.036); // round bolt head: .006..026
  }
 }
 bool Good=Bases->GetInstanceCount()==3&&Plates->GetInstanceCount()==3&&Hardware->GetInstanceCount()==27&&DeckClearance>=.069;
 for(int32 I=0;I<3;++I)
 {
  const FVector2D P=Feet[I];
  Good&=Bounds(Bases,I,{P.X-HalfWidth,BottomY,P.Y-HalfWidth},{P.X+HalfWidth,ConcreteTop,P.Y+HalfWidth});
  Good&=Bounds(Plates,I,{P.X-.15,ConcreteTop,P.Y-.15},{P.X+.15,PlateTop,P.Y+.15});
 }
 if(!Good)
 {
  for(auto* C:Created)if(C)C->DestroyComponent();
  return Fail(TEXT("new instance contact bounds differ"));
 }
 // Commit only after the original contacts and every new support bound are validated.
 for(int32 K=0;K<3;++K)W->AdoptNode(Created[K],Parent,Names[K]);
 for(int32 I=0;I<3;++I)
 {
  const FVector2D P=Feet[I];FS47Rect Foot;
  Foot.MinX=P.X-HalfWidth;Foot.MaxX=P.X+HalfWidth;Foot.MinZ=P.Y-HalfWidth;Foot.MaxZ=P.Y+HalfWidth;
  Foot.Id=FString::Printf(TEXT("saroLampFeet:%d"),I);W->CollidersMutable(TEXT("saro")).Add(Foot);
 }
 UE_LOG(LogTemp,Display,TEXT("S47: SARO lamp feet installed: 3 foundations, 3 shared batches, 33 instances; original bottom traces=3 terrain traces=15; support Y %.3f..%.3f, ground %.3f, deck clearance %.3f m; original pole/glass/flood/glow unchanged"),BottomY,PlateTop,GroundY,DeckClearance);
 return true;
}
}
