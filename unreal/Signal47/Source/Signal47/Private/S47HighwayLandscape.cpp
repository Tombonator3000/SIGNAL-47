// Unreal-only continuous highway dressing. Story areas and their collision stay separate.
#include "S47Web.h"
#include "S47Geo.h"
#include "S47GroundCover.h"
#include "S47Legs.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47HighwayLandscape, Log, All);
namespace
{
 const FVector SaroToRoad(8000 + S47Geo::SARO_IN_ROAD.X, 0, S47Geo::SARO_IN_ROAD.Z);
 bool HighwayArea(const FString& A) { return A==TEXT("saro") || A==TEXT("road") || A==TEXT("diner"); }
 bool NaturalProp(const FString& N)
 {
  for (const TCHAR* Prefix : {TEXT("props:rocksWest"),TEXT("props:rocksEast"),TEXT("props:rocksSaro"),
   TEXT("props:sage"),TEXT("props:yucca"),TEXT("props:tumbleweed"),TEXT("prop:rockFace"),TEXT("prop:outcrop")})
   if (N.StartsWith(Prefix)) return true;
  return false;
 }
}

FVector AS47Web::HighwayMapEye(const FVector& ThreeEye) const
{
 return HighwayLandscapeReady() && CurrentArea==TEXT("saro") ? ThreeEye+SaroToRoad : ThreeEye;
}

void AS47Web::BuildHighwayLandscape()
{
 if (FParse::Param(FCommandLine::Get(),TEXT("S47LegacyHighway")) ||
     FParse::Param(FCommandLine::Get(),TEXT("S47PropsOff")) ||
     FParse::Param(FCommandLine::Get(),TEXT("S47LegacyCalicheStreet"))) return;
 int32 Backdrop=INDEX_NONE;
 for (int32 I=0; I<Nodes.Num(); ++I) if (Nodes[I].Name==TEXT("prop:streetGroundMesas"))
  if (const UStaticMeshComponent* C=Cast<UStaticMeshComponent>(Nodes[I].Comp.Get()); C && C->GetStaticMesh() && Nodes[I].bVis) Backdrop=I;
 const int32 RoadRoot=Handle(TEXT("world.legs.road.area.group"));
 if (Backdrop<0 || !Nodes.IsValidIndex(RoadRoot)) return; // Keep every original until the authored replacement is usable.
 USceneComponent* Shared=NewObject<USceneComponent>(this,TEXT("HighwayLandscape"));
 Shared->SetMobility(EComponentMobility::Movable);Shared->SetupAttachment(GetRootComponent());Shared->RegisterComponent();
 HighwayLandscapeRoot=Shared;
 const int32 Ext=Handle(TEXT("ext.group"));
 auto Under=[this](int32 I,int32 Parent) { for (;I>=0;I=Nodes[I].Parent) if(I==Parent)return true;return false; };
 for (int32 I=0; I<Nodes.Num(); ++I)
 {
  FS47WebNode& N=Nodes[I];
  const bool Saro=Under(I,Ext);
  const bool Keep=I==Backdrop || N.Name.StartsWith(TEXT("nature:")) || N.Name.StartsWith(TEXT("highway:")) ||
   N.Name.StartsWith(TEXT("prop:streetGroundNature")) || (Saro && NaturalProp(N.Name)) ||
   (Saro && (N.Name==TEXT("props:powerline") || N.Name==TEXT("props:powerline_wires") || N.Name==TEXT("props:spur") || N.Name==TEXT("props:fenceWest") || N.Name==TEXT("props:fenceEast")));
  if (!Keep || !N.bVis) continue;
  UPrimitiveComponent* C=Cast<UPrimitiveComponent>(N.Comp.Get());
  if (!C || C->bHiddenInGame) continue;
  FTransform Canonical=C->GetComponentTransform();
  if(Saro)Canonical.AddToTranslation(S47Three::ToUE(SaroToRoad));
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->AttachToComponent(Shared,FAttachmentTransformRules::KeepWorldTransform);C->SetRelativeTransform(Canonical);
  HighwaySharedNodes.Add(I);HighwayVisuals.Add(I,Canonical);
 }
 // The shared geology replaces the two conflicting baked silhouettes. Small local rock faces remain.
 for(int32 I=0;I<Nodes.Num();++I)
  if ((Nodes[I].Name==TEXT("mesas:near") && (Under(I,Ext) || Under(I,Handle(TEXT("world.legs.road.area.group"))))) ||
      (Under(I,Ext) && (Nodes[I].Name==TEXT("prop:cliffWest") || Nodes[I].Name==TEXT("prop:cliffEast"))))
   SetVisible(I,false);

 // A fixed 200 m overlap uses the existing road cover exclusions and height, independent of active area.
 // Low plants and contact gravel taper at its ends; they add neither floor traces nor blockers.
 const FS47FloraSpec Cover=S47GroundCover::RoadFlora({});
 const double Seam=S47Legs::ROAD_TO_SARO()+5;
 struct FKit { const TCHAR* Id; int32 Cull; double Density,Scale; };
 const FKit Kits[]={{TEXT("Grass_A"),11000,.45,.75},{TEXT("Grass_B"),11000,.3,.8},
  {TEXT("Sage_A"),22000,.12,.7},{TEXT("Caliche_A"),11000,.35,.8},{TEXT("Sandstone_A"),11000,.12,.65}};
 for(int32 Ki=0;Ki<UE_ARRAY_COUNT(Kits);++Ki)
 {
  const FKit& K=Kits[Ki];const FString Name=FString(TEXT("SM_CN_"))+K.Id;
  UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/CalicheStreet/NatureVegetation/")+Name+TEXT(".")+Name));
  if(!Mesh)continue;
  UHierarchicalInstancedStaticMeshComponent* C=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
  C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->SetCastShadow(Ki==2);C->SetCullDistances(int32(K.Cull*.82),K.Cull);C->bAutoRebuildTreeOnInstanceChanges=false;
  C->SetupAttachment(Shared);C->RegisterComponent();
  C->SetRelativeLocation(S47Three::ToUE(8000,0,Seam));
  FRandomStream Random(472610+Ki*719);
  for(double Z=Seam-100;Z<Seam+100;Z+=2.5)for(double X=-75;X<75;X+=2.5)
  {
   const double Px=X+Random.FRandRange(-1.1f,1.1f),Pz=Z+Random.FRandRange(-1.1f,1.1f);
   const double Fade=FMath::Clamp((100-FMath::Abs(Pz-Seam))/25,0.0,1.0);
   if(FMath::Abs(Px)<7.5 || Cover.Cover(Px,Pz)<=0 || Random.FRand()>K.Density*Fade)continue;
   const double H=Cover.Ground(Px,Pz),Scale=K.Scale*Random.FRandRange(.7f,1.3f);
   C->AddInstance(FTransform(FRotator(0,Random.FRandRange(0.f,360.f),0),S47Three::ToUE(Px,H,Pz-Seam),FVector(Scale)),false);
   ++HighwayBridgeInstances;
  }
  C->BuildTreeIfOutdated(false,true);
  const int32 I=AdoptNode(C,RoadRoot,FString(TEXT("highway:overlap:"))+K.Id);
  HighwaySharedNodes.Add(I);HighwayVisuals.Add(I,C->GetRelativeTransform());
 }
 UpdateHighwayLandscape();
 UE_LOG(LogS47HighwayLandscape,Display,TEXT("S47: continuous highway ready: %d shared components, %d overlap instances, 200m overlap, canonical delta %.0f,%.0f; gameplay floors unchanged"),
  HighwayVisuals.Num(),HighwayBridgeInstances,SaroToRoad.X,SaroToRoad.Z);
}

void AS47Web::UpdateHighwayLandscape()
{
 USceneComponent* Root=HighwayLandscapeRoot.Get();if(!Root)return;
 HighwayOffset=CurrentArea==TEXT("saro") ? -SaroToRoad : FVector::ZeroVector;
 Root->SetRelativeLocation(S47Three::ToUE(HighwayOffset));
 for(int32 I:HighwaySharedNodes)ApplyVisibility(I,true);
}

bool AS47Web::AuditHighwayLandscape(TArray<FString>& Lines) const
{
 bool Ok=true;
 auto Check=[&](bool Pass,const FString& Why){Lines.Add(FString::Printf(TEXT("%s highway %s"),Pass?TEXT("OK"):TEXT("FAIL"),*Why));Ok&=Pass;};
 Check(HighwayLandscapeReady() && HighwayVisuals.Num()>8,TEXT("shared landscape available"));
 Check(HighwayBridgeInstances>100,TEXT("200m native plant and gravel overlap"));
 for(const auto& Pair:HighwayVisuals)
 {
  const FS47WebNode* N=Node(Pair.Key);const UPrimitiveComponent* C=N?Cast<UPrimitiveComponent>(N->Comp.Get()):nullptr;
  FTransform Want=Pair.Value;Want.AddToTranslation(S47Three::ToUE(HighwayOffset));
  Check(C && C->GetComponentTransform().Equals(Want,.03),N?N->Name:TEXT("missing shared component"));
  Check(C && C->IsVisible()==(HighwayArea(CurrentArea)&&N->bVis),TEXT("component visibility follows continuous area"));
 }
 return Ok;
}
