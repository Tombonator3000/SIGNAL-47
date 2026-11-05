#include "S47RouteLampFeet.h"
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

namespace S47RouteLampFeet
{
namespace
{
 constexpr double Half=.18, Embed=.025;
 struct FFoot { FVector P; double Bottom=0,ConcreteTop=0; bool AddCollider=false; };
 bool Skip(const FString& Family,const TCHAR* Why)
 {
  UE_LOG(LogTemp,Warning,TEXT("S47: %s lamp feet skipped: %s; original lamps retained"),*Family,Why);return false;
 }
 UStaticMeshComponent* Part(AS47Web* W,int32 Index,int32 Geo,const TCHAR* Path)
 {
  const auto* N=W->Node(Index);auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
  const bool Good=N&&N->Geo==Geo&&C&&C->GetStaticMesh()&&C->GetStaticMesh()->GetPathName()==Path;
  if(!Good)UE_LOG(LogTemp,Warning,TEXT("S47: route foot component mismatch node=%d actualGeo=%d expectedGeo=%d expectedAsset=%s actualAsset=%s transform=%s"),Index,N?N->Geo:-1,Geo,Path,C&&C->GetStaticMesh()?*C->GetStaticMesh()->GetPathName():TEXT("none"),C?*C->GetComponentTransform().ToString():TEXT("none"));
  return Good?C:nullptr;
 }
 bool Shape(UStaticMesh* M)
 {return M&&M->GetBounds().Origin.Equals(FVector::ZeroVector,.01)&&M->GetBounds().BoxExtent.Equals(FVector(50),.01);}
 bool Material(UMaterialInterface* M)
 {
  const UMaterial* B=M?M->GetMaterial():nullptr;
  return B&&B->bUsedWithInstancedStaticMeshes&&M->CheckMaterialUsage_Concurrent(MATUSAGE_InstancedStaticMeshes)&&M->GetBlendMode()==BLEND_Opaque;
 }
 bool RigidAt(UStaticMeshComponent* C,const FVector& P)
 {
  const FTransform Expected(FQuat::Identity,S47Three::ToUE(P));
  const bool Good=C&&C->GetComponentTransform().Equals(Expected,.001);
  if(!Good)UE_LOG(LogTemp,Warning,TEXT("S47: route foot transform mismatch component=%s asset=%s actual=%s expected=%s"),C?*C->GetName():TEXT("none"),C&&C->GetStaticMesh()?*C->GetStaticMesh()->GetPathName():TEXT("none"),C?*C->GetComponentTransform().ToString():TEXT("none"),*Expected.ToString());
  return Good;
 }
 bool Contains(const FS47Rect& R,const FVector& P)
 {return R.MinX<=P.X-Half+1e-8&&R.MaxX>=P.X+Half-1e-8&&R.MinZ<=P.Z-Half+1e-8&&R.MaxZ>=P.Z+Half-1e-8;}
 bool Measure(AS47Web* W,UStaticMeshComponent* Pole,const TArray<UStaticMeshComponent*>& Surfaces,TArray<FFoot>& Feet,bool MotelNorth=false)
 {
  FCollisionQueryParams Q(SCENE_QUERY_STAT(S47RouteLampFeetContact),true);
  for(auto& F:Feet)
  {
   FHitResult Hit;
   // Up into the original bottom cap; the gap below it must be clear.
   const bool CapHit=Pole->LineTraceComponent(Hit,S47Three::ToUE(F.P.X,F.P.Y-.19,F.P.Z),S47Three::ToUE(F.P.X,F.P.Y+.015,F.P.Z),Q);
   if(!CapHit||FMath::Abs(S47Three::ToThree(Hit.ImpactPoint).Y-F.P.Y)>.001)
   {UE_LOG(LogTemp,Warning,TEXT("S47: route foot cap FAIL x=%.9f z=%.9f expectedY=%.9f hit=%d actualY=%.9f asset=%s"),F.P.X,F.P.Z,F.P.Y,CapHit,CapHit?S47Three::ToThree(Hit.ImpactPoint).Y:999.,*Pole->GetStaticMesh()->GetPathName());return false;}
   double Low=1e9,High=-1e9;
   // Direct queries against a small, named set of actual rendered ground components.
   // 9x9 over the complete square includes every corner, edge and the asphalt boundary.
   for(int32 IX=0;IX<=8;++IX)for(int32 IZ=0;IZ<=8;++IZ)
   {
    const double X=F.P.X-Half+2*Half*IX/8,Z=F.P.Z-Half+2*Half*IZ/8;
    bool Found=false;double Top=-1e9;UStaticMeshComponent* Winner=nullptr;
    for(auto* C:Surfaces)
     if(C->LineTraceComponent(Hit,S47Three::ToUE(X,F.P.Y+.03,Z),S47Three::ToUE(X,F.P.Y-.30,Z),Q))
     {Found=true;const double Y=S47Three::ToThree(Hit.ImpactPoint).Y;if(Y>Top){Top=Y;Winner=C;}}
    // Only the measured Z36 seam raises its inner verge over the original cap.
    // Preserve that cap; let this foundation surround the embedded pole bottom.
    const bool RaisedSeam=F.P.Equals(FVector(-18.5,-.6,36),1.e-8)&&Z>=36-1.e-8&&Winner&&
     Winner->GetStaticMesh()->GetPathName()==TEXT("/Game/Signal47/Art/SaroRoad/SM_SaroVergeSeam.SM_SaroVergeSeam");
    const double MaxTop=F.P.Y+(MotelNorth?.003:(RaisedSeam?.015:-.012));
    // All other feet/surfaces retain the existing ground-below-cap requirement.
    if(!Found||Top>MaxTop||Top<F.P.Y-.20)
    {UE_LOG(LogTemp,Warning,TEXT("S47: route foot support FAIL footX=%.9f footZ=%.9f grid=%d/%d x=%.9f z=%.9f found=%d top=%.9f pole=%.9f gap=%.9f winner=%s"),F.P.X,F.P.Z,IX,IZ,X,Z,Found,Top,F.P.Y,F.P.Y-Top,Winner&&Winner->GetStaticMesh()?*Winner->GetStaticMesh()->GetPathName():TEXT("none"));return false;}
    Low=FMath::Min(Low,Top);High=FMath::Max(High,Top);
   }
   // The original asphalt approach meets the replacement court at an 8cm step.
   // Require both measured elevations; do not accept a ground-only floating pier.
   if(MotelNorth&&(FMath::Abs(Low-(-2.184426280932433))>.003||FMath::Abs(High-F.P.Y)>.003))return false;
   double FoundationLow=Low;
   if(MotelNorth)
   {
    // CourtSurface FBX f5ede772: 25mm rounded north edge, not a flat slab.
    // The 9x9 grid touches triangle165 at Z667.91; the continuous edge at
    // Z667.9 reaches this lower top bound. Verify active contact just inside it.
    constexpr double EdgeBound=-2.203568382038064,EdgeZ=667.9001,EdgeExpected=-2.2031948089787035;
    {
     const double X=F.P.X;double Top=-1e9;UStaticMeshComponent* Winner=nullptr;
     for(auto* C:Surfaces)if(C->LineTraceComponent(Hit,S47Three::ToUE(X,F.P.Y+.03,EdgeZ),S47Three::ToUE(X,F.P.Y-.30,EdgeZ),Q))
     {const double Y=S47Three::ToThree(Hit.ImpactPoint).Y;if(Y>Top){Top=Y;Winner=C;}}
     if(!Winner||Winner->GetStaticMesh()->GetPathName()!=TEXT("/Game/MotelV2/SM_Sierra_CourtSurface.SM_Sierra_CourtSurface")||FMath::Abs(Top-EdgeExpected)>.003)
     {UE_LOG(LogTemp,Warning,TEXT("S47: motel north bevel contact differs x=%.9f y=%.9f expected=%.9f"),X,Top,EdgeExpected);return false;}
     FoundationLow=FMath::Min(FoundationLow,Top);
    }
    FoundationLow=FMath::Min(FoundationLow,EdgeBound);
    UE_LOG(LogTemp,Display,TEXT("S47: motel north measured grid low=%.9f source edge bound=%.9f foundation low=%.9f"),Low,EdgeBound,FoundationLow);
   }
   if(High-Low>.12){UE_LOG(LogTemp,Warning,TEXT("S47: route foot terrain variation FAIL x=%.9f z=%.9f low=%.9f high=%.9f"),F.P.X,F.P.Z,Low,High);return false;}
   F.Bottom=FoundationLow-Embed;F.ConcreteTop=FMath::Max(F.P.Y-.01,High+.01);
   // Reuse only existing narrow pole exclusions that cover the whole new support.
   bool Covered=false;for(const auto& R:W->CollidersMutable(TEXT("saro")))Covered|=(!MotelNorth||R.bEnabled)&&Contains(R,F.P)&&R.MaxX-R.MinX<=.501&&R.MaxZ-R.MinZ<=.501;
   if(MotelNorth&&!Covered)return false;
   F.AddCollider=!Covered;
   // Road foot west edge stays at least .52m east of the widest asphalt lane edge -19.2.
   if(!MotelNorth&&F.P.X-Half< -18.68-1e-8)return false;
   UE_LOG(LogTemp,Display,TEXT("S47: route lamp foot preflight %s x=%.3f z=%.3f pole=%.6f ground=%.6f..%.6f buriedBottom=%.6f concreteTop=%.6f existingExclusion=%d"),MotelNorth?TEXT("motel-north"):TEXT("road"),F.P.X,F.P.Z,F.P.Y,Low,High,F.Bottom,F.ConcreteTop,Covered);
  }
  return true;
 }
 void Add(UInstancedStaticMeshComponent* C,const FVector& P,double Width,double Height,double Depth)
 {C->AddInstance(FTransform(FQuat::Identity,S47Three::ToUE(P),FVector(Width,Depth,Height)),true);}
 bool Bounds(UInstancedStaticMeshComponent* C,int32 I,const FVector& Low,const FVector& High)
 {
  FTransform T;if(!C->GetInstanceTransform(I,T,true))return false;
  const auto B=C->GetStaticMesh()->GetBounds();const FBox A=FBox(B.Origin-B.BoxExtent,B.Origin+B.BoxExtent).TransformBy(T);
  return A.Min.Equals(S47Three::ToUE(Low),.05)&&A.Max.Equals(S47Three::ToUE(High),.05);
 }
 bool Install(AS47Web* W,int32 Parent,const FString& Family,UStaticMeshComponent* Pole,const TArray<UStaticMeshComponent*>& Surfaces,TArray<FFoot> Feet,bool MotelNorth=false)
 {
  const auto* N=W->Node(Parent);if(!N||!N->Comp.IsValid())return Skip(Family,TEXT("parent unavailable"));
  const FString Prefix=TEXT("routeLampFeet:")+Family+TEXT(":");
  // Build is called once. Never mix a second or partial set with these supports.
  for(int32 I=0;I<W->PartCount();++I)if(const auto* Existing=W->Node(I);Existing&&Existing->Name.StartsWith(Prefix))return Skip(Family,TEXT("existing set"));
  for(const auto& R:W->CollidersMutable(TEXT("saro")))if(R.Id.StartsWith(Prefix))return Skip(Family,TEXT("orphaned collider"));
  if(!Measure(W,Pole,Surfaces,Feet,MotelNorth))return Skip(Family,TEXT("original cap, actual support grid or old collision exclusion differs"));
  auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
  auto* Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
  auto* Concrete=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SaroSite/Materials/M_SaroSite_Concrete.M_SaroSite_Concrete"));
  auto* Metal=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SubstationDetail/Materials/M_SubstationDetail_GalvanizedSteel.M_SubstationDetail_GalvanizedSteel"));
  if(!Shape(Cube)||!Shape(Cylinder)||!Material(Concrete)||!Material(Metal))return Skip(Family,TEXT("shared shapes/material ISM permutations unavailable"));
  UInstancedStaticMeshComponent* Created[3]={};
  for(int32 K=0;K<3;++K)
  {
   auto* C=NewObject<UInstancedStaticMeshComponent>(W);Created[K]=C;
   C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(N->Comp.Get());
   C->SetStaticMesh(K==2?Cylinder:Cube);C->SetMaterial(0,K==0?Concrete:Metal);C->SetCastShadow(true);
   C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
   C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->RegisterComponent();
  }
  bool Good=true;
  for(int32 I=0;I<Feet.Num();++I)
  {
   const auto& F=Feet[I];const double X=F.P.X,Z=F.P.Z,Top=F.ConcreteTop,PlateTop=Top+.012;
   Add(Created[0],{X,(F.Bottom+Top)*.5,Z},.36,Top-F.Bottom,.36);
   Add(Created[1],{X,(Top+PlateTop)*.5,Z},.34,PlateTop-Top,.34);
   Add(Created[2],{X,PlateTop+.025,Z},.34,.05,.34);
   for(double DX:{-.145,.145})for(double DZ:{-.145,.145})
   {
    Add(Created[2],{X+DX,PlateTop+.002,Z+DZ},.035,.004,.035);
    Add(Created[2],{X+DX,PlateTop+.010,Z+DZ},.025,.012,.025);
   }
   Good&=Bounds(Created[0],I,{X-Half,F.Bottom,Z-Half},{X+Half,Top,Z+Half});
   Good&=Bounds(Created[1],I,{X-.17,Top,Z-.17},{X+.17,PlateTop,Z+.17});
   const double CollarHalf=.17;
   Good&=Bounds(Created[2],I*9,{X-CollarHalf,PlateTop,Z-CollarHalf},{X+CollarHalf,PlateTop+.05,Z+CollarHalf});
  }
  Good&=Created[0]->GetInstanceCount()==Feet.Num()&&Created[1]->GetInstanceCount()==Feet.Num()&&Created[2]->GetInstanceCount()==Feet.Num()*9;
  if(!Good){for(auto* C:Created)C->DestroyComponent();return Skip(Family,TEXT("new instance bounds differ"));}
  const TCHAR* Suffix[]={TEXT("concrete"),TEXT("plates"),TEXT("hardware")};
  for(int32 K=0;K<3;++K)W->AdoptNode(Created[K],Parent,Prefix+Suffix[K]);
  for(int32 I=0;I<Feet.Num();++I)if(Feet[I].AddCollider)
  {
   const FVector P=Feet[I].P;FS47Rect R;R.MinX=P.X-Half;R.MaxX=P.X+Half;R.MinZ=P.Z-Half;R.MaxZ=P.Z+Half;R.Id=Prefix+FString::FromInt(I);W->CollidersMutable(TEXT("saro")).Add(R);
  }
  UE_LOG(LogTemp,Display,TEXT("S47: %s lamp feet installed: %d foundations, %d instances; original poles, heads, light anchors and power retained"),*Family,Feet.Num(),Feet.Num()*11);return true;
 }
 bool Road(AS47Web* W)
 {
  auto* Pole=Part(W,35,26,TEXT("/Game/Web/Geo/geo_000/StaticMeshes/g26.g26"));
  auto* Land=Part(W,7,4,TEXT("/Game/MotelV2/Trim/g4.g4"));
  auto* Road=Part(W,954,664,TEXT("/Game/Signal47/Art/SaroRoad/SM_SaroRoadSeam.SM_SaroRoadSeam"));
  auto* Verge=Part(W,953,663,TEXT("/Game/Signal47/Art/SaroRoad/SM_SaroVergeSeam.SM_SaroVergeSeam"));
  if(!RigidAt(Pole,FVector::ZeroVector)||Pole->GetStaticMesh()->GetNumTriangles(0)!=992||!RigidAt(Land,FVector::ZeroVector)||
     !RigidAt(Road,{-24,0,300})||!RigidAt(Verge,{-24,0,300}))return Skip(TEXT("road"),TEXT("active original pole / composed road ground differs"));
  TArray<FFoot> Feet;for(double Z:{70.,36.,2.,-32.,-66.,-100.,-134.,-168.})Feet.Add(FFoot{{-18.5,-.6,Z}});
  return Install(W,W->Handle(TEXT("ext.group")),TEXT("road"),Pole,{Land,Road,Verge},Feet);
 }

 bool MotelNorth(AS47Web* W)
 {
  if(FParse::Param(FCommandLine::Get(),TEXT("S47MotelLampFootOff")))return false;
  auto* Pole=Part(W,678,463,TEXT("/Game/MotelV2/Trim/g463.g463"));
  auto* Land=Part(W,7,4,TEXT("/Game/MotelV2/Trim/g4.g4"));
  auto* Approach=Part(W,671,456,TEXT("/Game/MotelV2/Trim/g456.g456"));
  UStaticMeshComponent* Court=nullptr;
  for(int32 I=0;I<W->PartCount();++I)
  {
   const auto* N=W->Node(I);auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
   if(N&&N->Name==TEXT("motel-v2:SM_Sierra_CourtSurface")&&C&&C->GetStaticMesh()&&
      C->GetStaticMesh()->GetPathName()==TEXT("/Game/MotelV2/SM_Sierra_CourtSurface.SM_Sierra_CourtSurface"))Court=C;
  }
  if(!RigidAt(Pole,{-2.5,-2.098568384594018,650})||Pole->GetStaticMesh()->GetNumTriangles(0)!=80||
     !RigidAt(Approach,{-2.5,-2.098568384594018,650})||Approach->GetStaticMesh()->GetNumTriangles(0)!=30||
     !RigidAt(Land,FVector::ZeroVector)||!RigidAt(Court,{-45.9,-2.0985683853163195,674.9}))
   return Skip(TEXT("motel-north"),TEXT("active original pole / native court / terrain differs"));
  // Same original pole, light, flood slots, and 40cm exclusion. 36cm footing
  // bridges the retained approach/new-court step without moving either slab.
  TArray<FFoot> Feet;Feet.Add(FFoot{{-30.9,-2.098568384594018,668}});
  return Install(W,W->Handle(TEXT("world.motel.group")),TEXT("motel-north"),Pole,{Land,Approach,Court},Feet,true);
 }

}
bool Build(AS47Web* W)
{
 if(!W||FParse::Param(FCommandLine::Get(),TEXT("S47RouteLampFeetOff"))||FParse::Param(FCommandLine::Get(),TEXT("S47PropsOff")))return false;
 const bool RoadBuilt=Road(W);const bool MotelBuilt=MotelNorth(W);return RoadBuilt||MotelBuilt;
}
}
