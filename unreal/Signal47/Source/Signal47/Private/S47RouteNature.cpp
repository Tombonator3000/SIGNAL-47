// Original roadside composition using a private, bounded native nature kit.
#include "S47RouteNature.h"
#include "S47Web.h"
#include "S47Geo.h"
#include "S47GroundCover.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "StaticMeshResources.h"

namespace
{
 using namespace S47RoadTerrain;
 const FVector SiteToCanonical(8024,0,-300);
#include "S47RouteNatureSeam.inl"
 struct FFamily { const TCHAR* Name; int32 Cull; bool Rock; UStaticMesh* Mesh=nullptr; TArray<FTransform> Instances; };
 struct FCluster { double X,Z,Scale; int32 Seed; };
 // Road-local metres: four distinct outcrops on the SARO approach, two on the
 // middle highway and one south of Caliche. The road and its long sightline stay open.
 constexpr FCluster Clusters[]={{-43,-170,.78,4701},{39,-100,.62,4702},{-48,-12,.95,4703},{43,58,.72,4704},
  {-52,900,1.05,4705},{58,1320,.95,4706},{-48,2360,.72,4707}};
 struct FKeep { double X0,X1,Z0,Z1; };
 bool In(const FKeep& B,double X,double Z,double R)
 {return X>=B.X0-R&&X<=B.X1+R&&Z>=B.Z0-R&&Z<=B.Z1+R;}

 bool Clear(double X,double Z,double Radius,bool Rock,bool Gravel)
 {
  // The complete transformed footprint must clear the road, its 16.5m flat
  // verge (large rocks additionally clear the 20.4m driving boundary), and walks.
  const double Lane=Rock?20.4:Gravel?6.3:Z<100?10.0:7.5;
  if(FMath::Abs(X)<Lane+Radius)return false;
  if(Z<-264+Radius || (Z<100 && Z>80-Radius))return false;
  if(Z<100 && (X-Radius<-100 || X+Radius>100))return false;
  if(Z<100 && FMath::Abs(Z+200)<1.6+Radius)return false; // measured patch front; skirt owns it
  // Existing fences, including the original near-SARO prop rows; do not bury
  // their posts or let plants grow through wire. The native power pass runs first.
  if(Z>=-200-Radius&&Z<=2900+Radius&&FMath::Abs(FMath::Abs(X)-21)<.8+Radius)return false;
  if(Z>=-230-Radius&&Z<=340+Radius)
   for(double Fx:{-16.5,13.5})if(FMath::Abs(X-Fx)<.7+Radius)return false;
  if(Z<100)
  {
   if(FMath::Abs(X+13)<1.2+Radius)return false; // west power-line foundations
   if(FMath::Abs(Z+254)<1.4+Radius)return false; // SARO spur at site z46
  }
  const FNear Track=TrackNearest(X,Z);
  if(Track.D<TrackHalf(Track.S)+TRK.Soft+2+Radius)return false;
  if(OldNearest(X,Z).D<17+Radius || OnRanchRoad(X,Z))return false;
  // Story footprints and their views/approaches. These are deliberately wider
  // than the small meshes: cattle, wreck, motel, deputy, diner and all Caliche doors.
  constexpr FKeep Keep[]={ {-160,24,440,720}, {-40,-10,130,190}, {-38,5,335,430},
   {-18,22,1645,1740}, {-49,24,1965,2024}, {-48,56,2012,2310} };
  for(const FKeep& B:Keep)if(In(B,X,Z,Radius))return false;
  return true;
 }

 double Ground(double X,double Z)
 {
  // Same surface selection as SaroFlora/World::MakeLands. Do not extend the
  // Caliche-only authoring helper here: it omits this northern join and the motel.
  if(Z>=-200)return SurfaceY(X,Z);
  double Y=S47GroundCover::SaroLand(X-24,Z+300);
  if(Z>-264&&FMath::Abs(X)<HWY.Flat)Y=FMath::Max(Y,S47RoadShape::HLow(X,Z));
  return Y;
 }

 bool Place(FFamily& Family,double X,double Z,double Scale,double Yaw,const FFamily* RockFamilies=nullptr)
 {
  if(!Family.Mesh)return false;
  const FBox Bounds=Family.Mesh->GetBoundingBox();
  FTransform T(FRotator(0,Yaw,0),FVector::ZeroVector,FVector(Scale));
  const FBox Rotated=Bounds.TransformBy(T);
  const double Radius=FMath::Sqrt(FMath::Square(FMath::Max(FMath::Abs(Rotated.Min.X),FMath::Abs(Rotated.Max.X)))+
   FMath::Square(FMath::Max(FMath::Abs(Rotated.Min.Y),FMath::Abs(Rotated.Max.Y))))/100.;
  const bool Gravel=FString(Family.Name).StartsWith(TEXT("Pebble"));
  if(!FMath::IsFinite(Radius)||Radius<=0||Radius>18||!Clear(X,Z,Radius,Family.Rock,Gravel))return false;
  // Vegetation/debris belongs beside the buried rock base, never growing through
  // the opaque body. All four solid families are populated before each lee drift.
  if(RockFamilies)for(int32 I=0;I<4;++I)for(const auto& Rock:RockFamilies[I].Instances)
  {
   const FBox B=RockFamilies[I].Mesh->GetBoundingBox().TransformBy(Rock);
   if(8000+X>=B.Min.X/100.-Radius-.08&&8000+X<=B.Max.X/100.+Radius+.08&&
      Z>=B.Min.Y/100.-Radius-.08&&Z<=B.Max.Y/100.+Radius+.08)return false;
  }
  double Y=Ground(X,Z);
  // Lowest sampled support buries the irregular base, so a low corner cannot
  // float on the higher centre sample. Native pivots may lie below/above their base.
  for(double Dx:{Rotated.Min.X,Rotated.Max.X})for(double Dz:{Rotated.Min.Y,Rotated.Max.Y})
   Y=FMath::Min(Y,Ground(X+Dx/100.,Z+Dz/100.));
  const double Sink=Family.Rock?.10*Scale:Gravel?.006:.015;
  T.SetTranslation(S47Three::ToUE(8000+X,Y-Rotated.Min.Z/100.-Sink,Z));
  Family.Instances.Add(T);return true;
 }

 void AddBlock(AS47Web* W,const FBox& B,const FString& Area,const FString& Id)
 {
  FS47Rect R;R.MinX=B.Min.X/100.;R.MaxX=B.Max.X/100.;R.MinZ=B.Min.Y/100.;R.MaxZ=B.Max.Y/100.;R.Id=Id;
  W->CollidersMutable(Area).Add(R);
  // EnsureLegs replaces its provisional obstacles with DriveData. Publish both
  // lists before world creation, as the established Props::AddCollider does.
  const auto& Data=W->DriveData();const TSharedPtr<FJsonObject>* Legs=nullptr,*Leg=nullptr;
  const TArray<TSharedPtr<FJsonValue>>* Old=nullptr;
  if(!Data.IsValid()||!Data->TryGetObjectField(TEXT("legs"),Legs)||!(*Legs)->TryGetObjectField(Area,Leg)||!(*Leg)->TryGetArrayField(TEXT("obstacles"),Old))return;
  TArray<TSharedPtr<FJsonValue>> All=*Old;
  TArray<TSharedPtr<FJsonValue>> Box={MakeShared<FJsonValueNumber>(R.MinX),MakeShared<FJsonValueNumber>(R.MaxX),
   MakeShared<FJsonValueNumber>(R.MinZ),MakeShared<FJsonValueNumber>(R.MaxZ)};
  All.Add(MakeShared<FJsonValueArray>(Box));(*Leg)->SetArrayField(TEXT("obstacles"),All);
 }

 bool LegacyNature(const FString& Name)
 {
  for(const TCHAR* Prefix:{TEXT("props:rocksWest"),TEXT("props:rocksEast"),TEXT("props:sage"),TEXT("props:yucca"),TEXT("props:tumbleweed")})
   if(Name.StartsWith(Prefix))return true;
  return false;
 }

 int32 BuildSeam(AS47Web* W,int32 Parent)
 {
  // The original g4 ring was sunk below the corridor's rectangular front by
  // ~0.7m. These measured endpoints bridge only the existing non-walkable
  // front band; neither original ground mesh nor any height query is replaced.
  UMaterialInterface* Material=W->SharedMid(416);
  if(!Material)return 0;
  TArray<FVector3f> Positions,Normals;TArray<FVector2f> UVs;TArray<uint32> Indices;
  auto Strip=[&](const FSeamSample* Samples,int32 Num)
  {
   const uint32 Base=Positions.Num();
   for(int32 I=0;I<Num;++I)
   {
    const auto& S=Samples[I];const double J=FMath::Sin(S.X*.73)*FMath::Sin(S.X*.19);
    const double Z[]={S.Toe,98.65+.10*J,99.28+.09*J,99.98,100.48};
    const double Y[]={S.Low-.012,FMath::Lerp(S.Low,S.High,.24)+.035*J,FMath::Lerp(S.Low,S.High,.61)+.026*J,S.High+.012,S.High-.012};
    for(int32 K=0;K<5;++K)
    {
     Positions.Add(FVector3f(S47Three::ToUE(S.X,Y[K],Z[K]-100)));
     UVs.Add(FVector2f(S.X/5,(Z[K]-300)/5));
    }
   }
   for(int32 I=0;I+1<Num;++I)for(int32 K=0;K<4;++K)
   {
    const uint32 A=Base+I*5+K,B=A+5,C=A+1,D=B+1;
    Indices.Append({A,B,C,B,D,C});
   }
  };
  Strip(Seam0,UE_ARRAY_COUNT(Seam0));Strip(Seam1,UE_ARRAY_COUNT(Seam1));
  Normals.SetNumZeroed(Positions.Num());
  for(int32 I=0;I<Indices.Num();I+=3)
  {
   const uint32 A=Indices[I],B=Indices[I+1],C=Indices[I+2];
   const FVector3f N=FVector3f::CrossProduct(Positions[B]-Positions[A],Positions[C]-Positions[A]);
   Normals[A]+=N;Normals[B]+=N;Normals[C]+=N;
  }
  FBox Bounds(ForceInit);for(const auto& P:Positions)Bounds+=FVector(P);
  auto* Mesh=NewObject<UStaticMesh>(W,NAME_None,RF_Transient);
  Mesh->bAllowCPUAccess=true;Mesh->NeverStream=true;Mesh->bSupportRayTracing=false;
  FStaticMaterial StaticMaterial(Material);
  StaticMaterial.UVChannelData=FMeshUVChannelInfo(500.f); // UV0 repeats every 5m (500 Unreal units).
  Mesh->GetStaticMaterials().Add(StaticMaterial);
  auto Data=MakeUnique<FStaticMeshRenderData>();Data->AllocateLODResources(1);
  Data->Bounds=FBoxSphereBounds(Bounds);Data->ScreenSize[0].Default=1;Data->NumInlinedLODs=1;
  Data->CurrentFirstLODIdx=0;Data->bReadyForStreaming=false;
  auto& L=Data->LODResources[0];FStaticMeshSection Section;Section.MaterialIndex=0;Section.FirstIndex=0;
  Section.NumTriangles=Indices.Num()/3;Section.MinVertexIndex=0;Section.MaxVertexIndex=Positions.Num()-1;
  Section.bEnableCollision=false;Section.bCastShadow=false;L.Sections.Add(Section);
  L.VertexBuffers.PositionVertexBuffer.Init(Positions,true);
  auto& Tangents=L.VertexBuffers.StaticMeshVertexBuffer;Tangents.SetUseFullPrecisionUVs(true);Tangents.Init(Positions.Num(),1,true);
  L.VertexBuffers.ColorVertexBuffer.Init(Positions.Num(),true);
  for(int32 I=0;I<Positions.Num();++I)
  {
   const FVector3f N=Normals[I].GetSafeNormal();
   const FVector3f T=(FVector3f(1,0,0)-N*N.X).GetSafeNormal();
   Tangents.SetVertexTangents(I,T,FVector3f::CrossProduct(N,T),N);Tangents.SetVertexUV(I,0,UVs[I]);
   L.VertexBuffers.ColorVertexBuffer.VertexColor(I)=FColor(222,222,222,255);
  }
  L.bHasColorVertexData=true;L.bBuffersInlined=true;L.bIsOptionalLOD=false;L.SourceMeshBounds=Data->Bounds;
  L.MaxDeviation=0;L.IndexBuffer.SetIndices(Indices,EIndexBufferStride::Force32Bit);
  Mesh->SetRenderData(MoveTemp(Data));Mesh->SetExtendedBounds(FBoxSphereBounds(Bounds));Mesh->InitResources();
  auto* C=NewObject<UStaticMeshComponent>(W,TEXT("RouteNature_SaroSeam"));C->SetMobility(EComponentMobility::Movable);
  C->SetupAttachment(W->GetRootComponent());C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->SetCastShadow(false);C->SetVisibleInRayTracing(false);C->bAffectDistanceFieldLighting=false;
  C->RegisterComponent();C->SetWorldLocation(S47Three::ToUE(8000,0,-200));
  W->AdoptNode(C,Parent,TEXT("highway:routeNature:seam"));
  return Indices.Num()/3;
 }
}

TSharedPtr<FS47RouteNature> FS47RouteNature::Build(AS47Web* W)
{
 if(!W||FParse::Param(FCommandLine::Get(),TEXT("S47RouteNatureOff"))||FParse::Param(FCommandLine::Get(),TEXT("S47LegacyHighway"))||
  FParse::Param(FCommandLine::Get(),TEXT("S47PropsOff"))||FParse::Param(FCommandLine::Get(),TEXT("S47LegacyCalicheStreet")))return {};
 const int32 Parent=W->Handle(TEXT("world.legs.road.area.group"));
 const auto* P=W->Node(Parent);if(!P||!P->Comp.IsValid())return {};
 USceneComponent* ParentComponent=P->Comp.Get();
 bool Backdrop=false;
 for(int32 I=0;I<W->PartCount();++I)if(const auto* N=W->Node(I);N&&N->Name==TEXT("prop:streetGroundMesas")&&N->bVis)
  if(const auto* C=Cast<UStaticMeshComponent>(N->Comp.Get());C&&C->GetStaticMesh())Backdrop=true;
 if(!Backdrop)return {}; // shared highway will be unavailable; preserve the original scene
 const auto& Drive=W->DriveData();const TSharedPtr<FJsonObject>* Legs=nullptr,*Leg=nullptr;
 const TArray<TSharedPtr<FJsonValue>>* Obstacles=nullptr;
 for(const TCHAR* Area:{TEXT("saro"),TEXT("road")})
  if(!Drive.IsValid()||!Drive->TryGetObjectField(TEXT("legs"),Legs)||!(*Legs)->TryGetObjectField(Area,Leg)||!(*Leg)->TryGetArrayField(TEXT("obstacles"),Obstacles))return {};

 FFamily Families[]={{TEXT("Cliff_A"),55000,true},{TEXT("Boulder_A"),30000,true},{TEXT("Boulder_B"),40000,true},{TEXT("Outcrop_A"),30000,true},
  {TEXT("Pebble_A"),11000,false},{TEXT("Pebble_B"),11000,false},{TEXT("DryGrass_A"),8000,false},{TEXT("DryScrub_A"),10000,false}};
 for(FFamily& F:Families)
 {
  const FString Name=FString(TEXT("SM_RN_"))+F.Name;
  F.Mesh=LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Licensed/RouteNature/Meshes/")+Name+TEXT(".")+Name));
  bool Valid=F.Mesh&&F.Mesh->GetRenderData()&&F.Mesh->GetNumLODs()>0&&!F.Mesh->GetStaticMaterials().IsEmpty();
  if(Valid)for(const auto& Slot:F.Mesh->GetStaticMaterials())
   Valid&=Slot.MaterialInterface&&Slot.MaterialInterface->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes)&&Slot.MaterialInterface->CheckMaterialUsage_Concurrent(MATUSAGE_InstancedStaticMeshes);
  if(!Valid){UE_LOG(LogTemp,Warning,TEXT("S47: RouteNature optional kit unavailable (%s); legacy nature retained"),*Name);return {};}
 }
 // Authored silhouettes: rear cliff, asymmetric lower boulders, smaller outcrop
 // and mineral fragments. Yaw/scale deliberately vary by group rather than mirror.
 for(int32 I=0;I<UE_ARRAY_COUNT(Clusters);++I)
 {
  const auto& C=Clusters[I];const double Side=C.X<0?-1:1;FRandomStream R(C.Seed);
  if(Place(Families[0],C.X+Side*5.2,C.Z+2.2,C.Scale,18+I*47))
  {
   const FBox Cliff=Families[0].Mesh->GetBoundingBox().TransformBy(Families[0].Instances.Last());
   // Nest the descending rocks into the actual road-facing foot, accounting for
   // native pivots, scale and yaw. A small overlap joins their irregular surfaces.
   auto AtFoot=[&](FFamily& F,const FBox& Support,double Scale,double Yaw,double Along,double Overlap)
   {
    const FBox Local=F.Mesh->GetBoundingBox().TransformBy(FTransform(FRotator(0,Yaw,0),FVector::ZeroVector,FVector(Scale)));
    const double Edge=Side<0?Support.Max.X:Support.Min.X;
    const double Back=Side<0?Local.Min.X:Local.Max.X;
    const double X=(Edge+Side*Overlap*100-Back)/100.-8000;
    const double Z=(Support.GetCenter().Y-Local.GetCenter().Y)/100.+Along;
    return Place(F,X,Z,Scale,Yaw);
   };
   const bool Boulder=AtFoot(Families[2],Cliff,C.Scale*.95,133+I*31,-1.4*C.Scale,.65*C.Scale);
   const FBox Foot=Boulder?Families[2].Mesh->GetBoundingBox().TransformBy(Families[2].Instances.Last()):Cliff;
   AtFoot(Families[3],Foot,C.Scale*1.25,59-I*28,1.0*C.Scale,.35*C.Scale);
  }
  else
  {
   Place(Families[2],C.X-Side*1.7,C.Z-4.1,C.Scale*.95,133+I*31);
   Place(Families[3],C.X-Side*5.2,C.Z+2.7,C.Scale*1.25,59-I*28);
  }
  for(int32 K=0;K<9;++K)
  {
   const double A=R.FRandRange(0.f,2*PI),D=R.FRandRange(2.f,10.f);
   Place(Families[1],C.X+FMath::Cos(A)*D,C.Z+FMath::Sin(A)*D,C.Scale*R.FRandRange(.65f,1.5f),R.FRandRange(0.f,360.f));
  }
  for(int32 K=0;K<150;++K)
  {
   const int32 Kind=K%5<3?4+K%2:K%5==3?6:7;
   const double A=R.FRandRange(0.f,2*PI),D=FMath::Sqrt(R.FRand())*(Kind>=6?5.5:13);
   // Small plants form two sheltered pockets along the lower outcrop's feet;
   // mineral debris spreads farther down the toe. Avoid the actual rock bodies.
   const double X=C.X+(Kind>=6?-Side*3.5:0)+FMath::Cos(A)*D;
   const double Z=C.Z+(Kind>=6?(K%2?7.5:-8):0)+FMath::Sin(A)*D;
   Place(Families[Kind],X,Z,R.FRandRange(.7f,1.4f),R.FRandRange(0.f,360.f),Families);
  }
 }
 // Contact gravel and low drifts connect the four main compositions. Staggered
 // curved bands and patch gaps break the straight verge without moving its floor.
 FRandomStream R(47261037),GravelR(47261091);
 for(double Z=-260;Z<=76;Z+=1.6)for(double Side:{-1.,1.})
 {
  const double Drift=.5+.5*FMath::Sin(Z*.053+Side*1.7+FMath::Sin(Z*.017));
  // Preserve the existing plant stream exactly: the old gravel row consumed
  // one centre draw and three draws for each of its three stones.
  for(int32 K=0;K<10;++K)R.FRand();
  // Short asymmetric pockets, with broad bare gaps between the strongest drifts.
  // A separate stream keeps this gravel edit independent of all vegetation.
  if(Drift>.66&&GravelR.FRand()<.45f)
  {
   const double X=Side*(7.1+GravelR.FRandRange(0.f,2.2f));
   const double CentreZ=Z+GravelR.FRandRange(-1.4f,1.4f);
   const int32 Count=GravelR.RandRange(2,4);
   for(int32 K=0;K<Count;++K)
   {
    const double A=GravelR.FRandRange(0.f,2*PI),D=FMath::Sqrt(GravelR.FRand())*.95;
    Place(Families[4+GravelR.RandRange(0,1)],X+FMath::Cos(A)*D,CentreZ+FMath::Sin(A)*D,
     GravelR.FRandRange(.7f,1.3f),GravelR.FRandRange(0.f,360.f),Families);
   }
  }
  if(Drift>.34)
  {
   const double Px=Side*(11.2+2.4*FMath::Sin(Z*.039)+R.FRandRange(0.f,2.5f));
   Place(Families[6],Px,Z+R.FRandRange(-1.f,1.f),R.FRandRange(.65f,1.2f),R.FRandRange(0.f,360.f),Families);
   if(Drift>.8)Place(Families[7],Side*(24+3*FMath::Sin(Z*.031)),Z,R.FRandRange(.65f,1.f),R.FRandRange(0.f,360.f),Families);
  }
 }
 for(const auto& F:Families)if(F.Instances.IsEmpty())return {};
 auto Result=MakeShared<FS47RouteNature>();Result->W=W;
 auto Block=[&](const FBox& B,const TCHAR* Area,const FString& Id)
 {
  AddBlock(W,B,Area,Id);Result->Blocks.Add({Area,Id,FVector4(B.Min.X/100.,B.Max.X/100.,B.Min.Y/100.,B.Max.Y/100.)});
 };
 for(FFamily& F:Families)
 {
  auto* C=NewObject<UHierarchicalInstancedStaticMeshComponent>(W,*FString::Printf(TEXT("RouteNature_%s"),F.Name));
  C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(F.Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  if(FString(F.Name).StartsWith(TEXT("Pebble")))for(int32 Slot=0;Slot<F.Mesh->GetStaticMaterials().Num();++Slot)
   if(auto* Tint=UMaterialInstanceDynamic::Create(F.Mesh->GetStaticMaterials()[Slot].MaterialInterface,C))
   {
    Tint->SetVectorParameterValue(TEXT("TextureTint"),FLinearColor(.50f,.43f,.33f,1.f));
    C->SetMaterial(Slot,Tint);
   }
  C->SetCastShadow(F.Rock);C->SetVisibleInRayTracing(F.Rock);C->bAffectDistanceFieldLighting=F.Rock;
  C->SetCullDistances(FMath::RoundToInt(F.Cull*.82),F.Cull);C->bAutoRebuildTreeOnInstanceChanges=false;
  C->SetupAttachment(ParentComponent);C->RegisterComponent();C->SetWorldTransform(FTransform(S47Three::ToUE(8000,0,0)));
  for(int32 I=0;I<F.Instances.Num();++I)
  {
   const FTransform& T=F.Instances[I];C->AddInstance(T,true);
   if(F.Rock)
   {
    const FBox B=F.Mesh->GetBoundingBox().TransformBy(T);
    const FString Id=FString::Printf(TEXT("routeNature:%s:%d"),F.Name,I);
    Block(B,TEXT("road"),Id);
    const FVector Pos=S47Three::ToThree(T.GetLocation());
    if(Pos.Z<600)
    {
     const FVector Delta=S47Three::ToUE(SiteToCanonical);Block(FBox(B.Min-Delta,B.Max-Delta),TEXT("saro"),Id);
    }
    ++Result->RockColliders;
   }
  }
  C->BuildTreeIfOutdated(false,true);
  Result->Nodes.Add(W->AdoptNode(C,Parent,FString(TEXT("highway:routeNature:"))+F.Name));Result->Counts.Add(F.Instances.Num());
  UE_LOG(LogTemp,Display,TEXT("S47: RouteNature %s instances=%d nativeBounds_cm=%s LODs=%d cull=%dm shadowRT=%d"),
   F.Name,F.Instances.Num(),*F.Mesh->GetBoundingBox().ToString(),F.Mesh->GetNumLODs(),F.Cull/100,F.Rock);
 }
 Result->SeamTriangles=BuildSeam(W,Parent);
 // Only superseded old prop instances inside the new composed footprints retire.
 // Never hide entire merged terrain/fence nodes or the existing Caliche HISM kit.
 const int32 Ext=W->Handle(TEXT("ext.group"));
 for(int32 I=0;I<W->PartCount();++I)
 {
  const auto* N=W->Node(I);if(!N||!LegacyNature(N->Name))continue;
  auto* C=Cast<UInstancedStaticMeshComponent>(N->Comp.Get());if(!C)continue;
  bool Site=false;for(int32 J=I;J>=0;){const auto* A=W->Node(J);if(!A)break;if(J==Ext){Site=true;break;}J=A->Parent;}
  for(int32 K=C->GetInstanceCount()-1;K>=0;--K)
  {
   FTransform T;if(!C->GetInstanceTransform(K,T,true))continue;
   FVector At=S47Three::ToThree(T.GetLocation());if(Site)At+=SiteToCanonical;
   bool Replace=false;for(const auto& B:Clusters)if(FMath::Square(At.X-8000-B.X)+FMath::Square(At.Z-B.Z)<13*13){Replace=true;break;}
   if(Replace&&C->RemoveInstance(K))++Result->RetiredInstances;
  }
 }
 UE_LOG(LogTemp,Display,TEXT("S47: RouteNature ready: 7 composed groups, 8 native families, %d solid rock footprints, %d superseded instances, %d seam triangles; shared highway coordinates, original floors and routes retained"),Result->RockColliders,Result->RetiredInstances,Result->SeamTriangles);
 return Result;
}

bool FS47RouteNature::Audit(TArray<FString>& Lines)const
{
 if(!W.IsValid())return false;
 bool OK=Nodes.Num()==8&&Counts.Num()==8&&RockColliders>0&&SeamTriangles==552;
 for(int32 I=0;I<Nodes.Num();++I)
 {
  const auto* N=W->Node(Nodes[I]);const auto* C=N?Cast<UInstancedStaticMeshComponent>(N->Comp.Get()):nullptr;
  OK&=C&&Counts.IsValidIndex(I)&&C->GetInstanceCount()==Counts[I]&&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision;
 }
 for(const FBlock& B:Blocks)
 {
  OK&=W->CollidersMutable(B.Area).ContainsByPredicate([&](const FS47Rect& R){return R.Id==B.Id&&R.bEnabled&&
   FMath::IsNearlyEqual(R.MinX,B.Limits.X,.001)&&FMath::IsNearlyEqual(R.MaxX,B.Limits.Y,.001)&&
   FMath::IsNearlyEqual(R.MinZ,B.Limits.Z,.001)&&FMath::IsNearlyEqual(R.MaxZ,B.Limits.W,.001);});
  const auto& D=W->DriveData();const TSharedPtr<FJsonObject>* Legs=nullptr,*Leg=nullptr;const TArray<TSharedPtr<FJsonValue>>* Obs=nullptr;
  bool Driving=false;
  if(D.IsValid()&&D->TryGetObjectField(TEXT("legs"),Legs)&&(*Legs)->TryGetObjectField(B.Area,Leg)&&(*Leg)->TryGetArrayField(TEXT("obstacles"),Obs))
   for(const auto& O:*Obs)
   {
    const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(!O->TryGetArray(A)||A->Num()!=4)continue;
    Driving|=FMath::IsNearlyEqual((*A)[0]->AsNumber(),B.Limits.X,.001)&&FMath::IsNearlyEqual((*A)[1]->AsNumber(),B.Limits.Y,.001)&&
     FMath::IsNearlyEqual((*A)[2]->AsNumber(),B.Limits.Z,.001)&&FMath::IsNearlyEqual((*A)[3]->AsNumber(),B.Limits.W,.001);
   }
  OK&=Driving;
 }
 Lines.Add(FString::Printf(TEXT("RouteNature families=%d solidRockFootprints=%d retired=%d seamTriangles=%d %s"),Nodes.Num(),RockColliders,RetiredInstances,SeamTriangles,OK?TEXT("PASS"):TEXT("FAIL")));
 return OK;
}
