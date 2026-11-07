#include "S47FarmWorld.h"
#include "S47FreeformTerrain.h"
#include "S47Relief.h"
#include "S47RoadEdge.h"
#include "Components/PointLightComponent.h"
#include "Components/MaterialBillboardComponent.h"
#include "S47FarmHouse.h"
#include "S47FarmBuildings.h"
#include "S47Freeform.h"
#include "S47Glow.h"
#include "S47Walker.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "S47Main.h"
#include "S47World.h"
#include "S47Web.h"
#include "S47Geo.h"
#include "S47RoadTerrain.h"
#include "S47RoadShape.h"
#include "StaticMeshResources.h"
#include "S47HighwayLandscape.h"
#include "S47Three.h"
#include "S47TownLayer.h"
#include "S47Herd.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Components/DecalComponent.h"
#include "Misc/PackageName.h"
namespace
{
 struct FPart {TWeakObjectPtr<UStaticMeshComponent> C;FVector Road,Size;};
 TWeakObjectPtr<AS47Web> Built,AppliedWorld;FString AppliedArea;bool AppliedOn=false;TArray<FPart> Parts;
 struct FRock {TWeakObjectPtr<UInstancedStaticMeshComponent> C;int32 Index=INDEX_NONE,Node=INDEX_NONE;FTransform Original,Clear;FBox Before=FBox(ForceInit),After=FBox(ForceInit);};
 TWeakObjectPtr<AS47Web> RockWorld;TArray<FRock> Rocks;bool RocksScanned=false,RocksOn=false,RocksSafe=false;
 FBox RoadBounds(AS47Web* W,UInstancedStaticMeshComponent* C,const FTransform& World)
 {
  const FBox B=C->GetStaticMesh()->GetBoundingBox();FBox Out(ForceInit);
  for(int32 I=0;I<8;++I){const FVector V(I&1?B.Max.X:B.Min.X,I&2?B.Max.Y:B.Min.Y,I&4?B.Max.Z:B.Min.Z);FVector P=W->HighwayMapEye(S47Three::ToThree(World.TransformPosition(V)));P.X-=8000;Out+=P;}return Out;
 }
 bool OverlapsLane(const FBox& B)
 {
  if(!B.IsValid||B.Max.X<6||B.Min.X>116)return false;
  const double X0=FMath::Max(6.,B.Min.X),X1=FMath::Min(116.,B.Max.X);double Low=1e30,High=-1e30;
  for(double X=X0;X<X1+.5;X+=.5){const double Z=S47RoadTerrain::RanchZ(FMath::Min(X,X1));Low=FMath::Min(Low,Z-3.4);High=FMath::Max(High,Z+3.4);}return B.Max.Z>=Low&&B.Min.Z<=High;
 }
 void ClearRoad(AS47Web* W,bool On)
 {
  if(RockWorld!=W){RockWorld=W;Rocks.Reset();RocksScanned=false;RocksOn=false;RocksSafe=false;}
  if(!RocksScanned)
  {
   if(!W->HighwayLandscapeReady())return;RocksSafe=true;int32 Families=0;
   for(const TCHAR* A:{TEXT("saro"),TEXT("road"),TEXT("diner")})for(const auto& C:W->CollidersMutable(A))
    if(C.Id.StartsWith(TEXT("props:rocksEast:"))||C.Id.StartsWith(TEXT("props:rocksWest:"))){RocksSafe=false;UE_LOG(LogTemp,Error,TEXT("S47 farm clearance unexpected authored rock collider %s in %s; retaining original instances"),*C.Id,A);}
   if(!RocksSafe){RocksScanned=true;return;}
   for(int32 Node=0;;++Node)
   {
    const auto* N=W->Node(Node);if(!N)break;if(!N->Name.StartsWith(TEXT("props:rocksEast_"))&&!N->Name.StartsWith(TEXT("props:rocksWest_")))continue;
    auto* C=Cast<UInstancedStaticMeshComponent>(N->Comp.Get());if(!C||!C->GetStaticMesh())continue;++Families;
    for(int32 I=0;I<C->GetInstanceCount();++I)
    {
     FTransform Local,World;if(!C->GetInstanceTransform(I,Local,false)||!C->GetInstanceTransform(I,World,true))continue;const FBox B=RoadBounds(W,C,World);if(!OverlapsLane(B))continue;
     FRock R;R.C=C;R.Index=I;R.Node=Node;R.Original=Local;R.Before=B;FVector Origin=W->HighwayMapEye(S47Three::ToThree(World.GetLocation()));Origin.X-=8000;
     const double DZ=S47RoadTerrain::RanchZ(B.GetCenter().X)-10.-B.GetExtent().Z-B.GetCenter().Z;
     const double DY=S47RoadTerrain::SurfaceY(Origin.X,Origin.Z+DZ)-S47RoadTerrain::SurfaceY(Origin.X,Origin.Z);
     const FVector Delta=S47FarmWorld::RoadToArea(W->Area(),Origin+FVector(0,DY,DZ))-S47FarmWorld::RoadToArea(W->Area(),Origin);
     FTransform ClearWorld=World;ClearWorld.AddToTranslation(S47Three::ToUE(Delta));R.Clear=ClearWorld.GetRelativeTransform(C->GetComponentTransform());R.After=RoadBounds(W,C,ClearWorld);
     if(OverlapsLane(R.After)){RocksSafe=false;UE_LOG(LogTemp,Error,TEXT("S47 farm clearance computed destination still overlaps node%d instance%d; retaining originals"),Node,I);continue;}
     UE_LOG(LogTemp,Display,TEXT("S47 farm clearance measured node%d name=%s mesh=%s instance%d collision=%d beforeRoad=%s afterRoad=%s transform=%s"),Node,*N->Name,*GetPathNameSafe(C->GetStaticMesh()),I,int32(C->GetCollisionEnabled()),*R.Before.ToString(),*R.After.ToString(),*Local.ToHumanReadableString());Rocks.Add(R);
    }
   }
   if(Families==0)return;RocksScanned=true;UE_LOG(LogTemp,Display,TEXT("S47 farm clearance measured %d intersecting instances in %d existing rock families; original matrices retained"),Rocks.Num(),Families);
  }
  if(!RocksSafe||RocksOn==On)return;
  for(const auto& R:Rocks)if(auto* C=R.C.Get())C->UpdateInstanceTransform(R.Index,On?R.Clear:R.Original,false,true,true);
  RocksOn=On;UE_LOG(LogTemp,Display,TEXT("S47 farm clearance mode=%s moved=%d; instance transforms updated; no authored drive/walk rock colliders"),On?TEXT("Freeform"):TEXT("Story"),Rocks.Num());
 }
 // Plants and stones scattered across the open land would stand through the old house's floor.
 struct FPadPlant{TWeakObjectPtr<UInstancedStaticMeshComponent> C;int32 Index=0;FTransform Original;};
 TArray<FPadPlant> PadPlants;TWeakObjectPtr<AS47Web> PadWorld;bool bPadScanned=false,bPadOn=false;
 void ClearHousePad(AS47Web* W,bool On)
 {
  if(PadWorld.Get()!=W){PadWorld=W;PadPlants.Reset();bPadScanned=false;bPadOn=false;}
  if(!bPadScanned)
  {
   if(!W->HighwayLandscapeReady())return;bPadScanned=true;
   TArray<UInstancedStaticMeshComponent*> Comps;W->GetComponents<UInstancedStaticMeshComponent>(Comps);
   for(auto* C:Comps)for(int32 I=0;I<C->GetInstanceCount();++I)
   {
    FTransform Local,World;if(!C->GetInstanceTransform(I,Local,false)||!C->GetInstanceTransform(I,World,true))continue;
    FVector Road=W->HighwayMapEye(S47Three::ToThree(World.GetLocation()));Road.X-=8000;
    if(!S47FarmHouse::OnPad(Road.X,Road.Z,1.2)&&!S47FarmBuildings::OnPlot(Road.X,Road.Z,1.2))continue;
    PadPlants.Add({C,I,Local});
   }
   UE_LOG(LogTemp,Display,TEXT("S47 farmhouse pad: %d scattered instances inside its footprint and the four building plots"),PadPlants.Num());
  }
  if(bPadOn==On)return;bPadOn=On;
  TSet<UInstancedStaticMeshComponent*> Touched;
  for(const auto& P:PadPlants)if(auto* C=P.C.Get())
  {
   FTransform T=P.Original;if(On)T.SetScale3D(FVector(.0001));
   C->UpdateInstanceTransform(P.Index,T,false,false,true);Touched.Add(C);
  }
  for(auto* C:Touched){C->MarkRenderStateDirty();if(auto* H=Cast<UHierarchicalInstancedStaticMeshComponent>(C))H->BuildTreeIfOutdated(false,true);}
 }
 // The farm stands where the road's piece of ground ends. In SARO's terms that piece (node 953) stops at road x 107.66; east
 // of it only SARO's ring (node 7) is left: cells some 30 m across, sunk .7 m under the piece up to x 118, and .79 to .25
 // under the surface everything here stands on. Hence the strip in the air, the shed on stilts and the walker over nothing.
 // Freeform gets the same desert grid on over the yard, in the ring's material; flat ground ends in a 1 m dive under the ring.
 struct FGroundGrid{int32 Rows=0,Cols=0;TArray<FVector> Pos,Normal;TArray<FVector2f> UV;TArray<FColor> Color;};
 UStaticMesh* GroundMesh(AS47Web* W,const FName Name,const FGroundGrid& G,UMaterialInterface* Material)
 {
  const uint32 Count=G.Pos.Num();if(G.Rows<2||G.Cols<2||Count!=uint32(G.Rows*G.Cols))return nullptr;
  auto Data=MakeUnique<FStaticMeshRenderData>();Data->AllocateLODResources(1);Data->ScreenSize[0].Default=1;Data->NumInlinedLODs=1;
  auto& L=Data->LODResources[0];L.VertexBuffers.PositionVertexBuffer.Init(Count,true);
  // A second UV of 0: the relief's material reads sediment and stone from UV1, and a mesh without one gives it UV0, tens of
  // metres over its scale: the whole yard read as the pale sand of an arroyo's bed. No sediment and no stone here.
  auto& T=L.VertexBuffers.StaticMeshVertexBuffer;T.SetUseFullPrecisionUVs(true);T.Init(Count,2,true);
  FBox Bounds(ForceInit);
  for(uint32 I=0;I<Count;++I)
  {
   const FVector At=S47Three::ToUE(G.Pos[I]);Bounds+=At;L.VertexBuffers.PositionVertexBuffer.VertexPosition(I)=FVector3f(At);
   const FVector N=S47Three::DirToUE(G.Normal[I]);const FVector Tangent=FVector::CrossProduct(FVector::RightVector,N).GetSafeNormal();
   T.SetVertexTangents(I,FVector3f(Tangent),FVector3f(FVector::CrossProduct(N,Tangent)),FVector3f(N));T.SetVertexUV(I,0,G.UV[I]);T.SetVertexUV(I,1,FVector2f(0.f,0.f));
  }
  L.VertexBuffers.ColorVertexBuffer.InitFromColorArray(G.Color);
  // Rows run along X and columns along Z. Three's up-facing order; its swap to UE already makes it UE's (S47SaroOverlays).
  TArray<uint32> Indices;Indices.Reserve((G.Rows-1)*(G.Cols-1)*6);
  for(int32 R=0;R+1<G.Rows;++R)for(int32 C=0;C+1<G.Cols;++C)
  {
   const uint32 A=R*G.Cols+C,Next=A+1,Over=A+G.Cols,Far=Over+1;
   Indices.Append({A,Next,Over,Over,Next,Far});
  }
  FStaticMeshSection S;S.MaterialIndex=0;S.FirstIndex=0;S.NumTriangles=Indices.Num()/3;S.MinVertexIndex=0;S.MaxVertexIndex=Count-1;S.bEnableCollision=false;S.bCastShadow=false;L.Sections.Add(S);
  L.bHasColorVertexData=true;L.bBuffersInlined=true;L.bIsOptionalLOD=false;L.MaxDeviation=0;L.SourceMeshBounds=FBoxSphereBounds(Bounds);
  L.IndexBuffer.TrySetAllowCPUAccess(true);L.IndexBuffer.SetIndices(Indices,EIndexBufferStride::Force32Bit);
  auto* Mesh=NewObject<UStaticMesh>(W,Name,RF_Transient);Mesh->bAllowCPUAccess=true;Mesh->NeverStream=true;Mesh->bSupportRayTracing=false;
  FStaticMaterial Slot(Material);Slot.UVChannelData=FMeshUVChannelInfo(1.f);Mesh->GetStaticMaterials().Add(Slot);
  Data->Bounds=FBoxSphereBounds(Bounds);Data->CurrentFirstLODIdx=0;Data->bReadyForStreaming=false;
  Mesh->SetRenderData(MoveTemp(Data));Mesh->SetExtendedBounds(FBoxSphereBounds(Bounds));Mesh->InitResources();return Mesh;
 }
 FVector LandNormal(double X,double Z,double H)
 {
  return FVector(S47RoadTerrain::SurfaceY(X-H,Z)-S47RoadTerrain::SurfaceY(X+H,Z),2*H,S47RoadTerrain::SurfaceY(X,Z-H)-S47RoadTerrain::SurfaceY(X,Z+H)).GetSafeNormal();
 }
 UStaticMeshComponent* NewGroundPart(AS47Web* W,UStaticMesh* Mesh,const TCHAR* Name)
 {
  auto* C=NewObject<UStaticMeshComponent>(W);C->SetupAttachment(W->GetRootComponent());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->RegisterComponent();W->AdoptNode(C,INDEX_NONE,FString(Name));return C;
 }
 constexpr double YardX0=107,YardX1=201,YardZ0=130,YardZ1=290,YardFlatX=163,YardFlatZ0=140,YardFlatZ1=280,YardDive=1;
 UStaticMesh* MakeYardGround(AS47Web* W,UMaterialInterface* Material)
 {
  TArray<double> Xs,Zs;
  for(const double X:S47RoadTerrain::GX())if(X>=YardX0&&X<=YardX1)Xs.Add(X);
  for(const double Z:S47RoadTerrain::GZ())if(Z>=YardZ0&&Z<=YardZ1)Zs.Add(Z);
  FGroundGrid G;G.Rows=Xs.Num();G.Cols=Zs.Num();
  for(const double X:Xs)for(const double Z:Zs)
  {
   // The first column is the road piece's own last column, so the two share every point.
   const bool bDive=X>YardX0+1&&(X>YardFlatX||Z<YardFlatZ0||Z>YardFlatZ1);
   G.Pos.Add(FVector(X,S47RoadTerrain::SurfaceY(X,Z)-(bDive?YardDive:0),Z));G.Normal.Add(LandNormal(X,Z,1));
   // Exterior.ts ground: the ring's own colour and UV in the site's terms, so the dive under it shows no join.
   const double Sx=X-24,Sz=Z+300,Shade=.81+.25*FMath::Sin(Sx*.013)*FMath::Cos(Sz*.011);
   G.UV.Add(FVector2f(Sx/5,Sz/5));const float Grey=float(Shade);G.Color.Add(FLinearColor(Grey,Grey,Grey,1.f).ToFColor(false));
  }
  return GroundMesh(W,TEXT("S47FarmYardGround"),G,Material);
 }
 // roadProps.ts ranchRoad, rowed every metre instead of every four so it follows the land with no daylight under it, and
 // ending at the yard: the last 8 m and the outer metre go over to the desert's own colour, as a track's edges do.
 constexpr double FarmRoadX0=6,FarmRoadX1=118,FarmRoadFade=8,FarmRoadHalf=2.5;
 UStaticMesh* MakeFarmRoad(AS47Web* W,UMaterialInterface* Material)
 {
  FGroundGrid G;G.Rows=int32(FarmRoadX1-FarmRoadX0)+1;G.Cols=5;
  for(int32 Row=0;Row<G.Rows;++Row)
  {
   const double X=FarmRoadX0+Row,Along=(S47RoadTerrain::RanchZ(X+1)-S47RoadTerrain::RanchZ(X-1))/2,Len=FMath::Sqrt(1+Along*Along);
   const double End=FMath::Clamp((X-(FarmRoadX1-FarmRoadFade))/FarmRoadFade,0.,1.);
   for(const double Out:{-FarmRoadHalf,-FarmRoadHalf/2,0.,FarmRoadHalf/2,FarmRoadHalf})
   {
    const double Px=X-Along/Len*Out,Pz=S47RoadTerrain::RanchZ(X)+Out/Len;const bool bEdge=FMath::Abs(Out)>=FarmRoadHalf;
    // 1.5 cm over the land, its edge .5 cm: the same rule as the ground under it, row for row.
    G.Pos.Add(FVector(Px,S47RoadTerrain::SurfaceY(Px,Pz)+(bEdge?.005:.015),Pz));G.Normal.Add(LandNormal(Px,Pz,.5));
    G.UV.Add(FVector2f(.5+Out/(2*FarmRoadHalf),X/8));
    // roadTerrain.ts tint and edgeColour; washAt is nothing out here.
    const double F=FMath::Max(End,bEdge?1.:Out!=0?.2:0.),N=.8+.12*FMath::Sin(Px*.013)*FMath::Cos(Pz*.011)+.16*(S47RoadShape::Hash(FMath::RoundToDouble(Px),FMath::RoundToDouble(Pz))-.5);
    G.Color.Add(FLinearColor(float(1+(N*.909-1)*F),float(1+(N*.98*.855-1)*F),float(1+(N*.95*.776-1)*F),1.f).ToFColor(false));
   }
  }
  return GroundMesh(W,TEXT("S47FarmRoadRibbon"),G,Material);
 }
 // Tom, 7 November: houses and land must blend into each other. Decals lie over whatever ground is there with real alpha, so the
 // road, the yard and the soil at the walls have no edge, and they follow the land by themselves (Tools/farmbuildings/ground_decals.py).
 // A decal's U runs along its local Z and V along Y (DeferredDecal.usf); the road's and the band's maps are alike on both sides.
 struct FFarmDecal{TWeakObjectPtr<UDecalComponent> C;FVector Road=FVector::ZeroVector;double Degrees=0;};
 struct FDecalTone{TWeakObjectPtr<UMaterialInstanceDynamic> Mid;FLinearColor Tint=FLinearColor::White,ByDay=FLinearColor::White;};
 TArray<FFarmDecal> FarmDecals;TArray<FDecalTone> DecalTones;TWeakObjectPtr<AS47Web> DecalWorld;bool bDecalsTried=false;float DecalDay=-1.f;
 constexpr double DecalRoadHalf=3.8,DecalBandHalf=.9,DecalMargin=.5; // Half widths of ground_decals.py's maps; how far past the land a box reaches.
 // The only numbers to retune from a capture: the maps' albedo is reckoned for the sun, sRGB 180,140,105 on the road.
 const FLinearColor DecalRoadTint(.62f,.56f,.5f),DecalBandTint(.6f,.62f,.66f),DecalFieldTint(.8f,.74f,.68f),DecalCornerTint(.5f,.46f,.44f);
 // Freeform's ground came down by day to one soil (S47MesaRock::ApplyFreeformGround: the sand layer by .781 .785 .765), so every decal follows by the same
 // factor, or it would stand bright on it. The ruts and the road are tinted from a capture at 11:18 (nature-yard): a rut read 175,114,79 on soil that read
 // 186,142,113, redder and darker by .87 .62 .47 (linear); against the soil of the day (that by .78) they come to .85 of it with the same hue by
 // .76 1.07 1.37 of what they were. The yard's, the door's, the wall band's and the corners' patches only follow the ground. The numbers to retune from a capture.
 const FLinearColor DecalSoilByDay(.781f,.785f,.765f,1.f),DecalRoadByDay(.76f,1.07f,1.37f,1.f);
 constexpr float DecalBandOpacity=.7f,DecalYardOpacity=.32f,DecalDoorOpacity=.6f,DecalCornerOpacity=.9f;
 void BuildFarmDecals(AS47Web* W,bool On)
 {
  if(DecalWorld.Get()!=W){DecalWorld=W;FarmDecals.Reset();DecalTones.Reset();bDecalsTried=false;DecalDay=-1.f;}
  if(!On||bDecalsTried||!S47FarmBuildings::Available())return;bDecalsTried=true;
  const auto Load=[](const TCHAR* Name)->UMaterialInterface*
  {
   const FString Package=FString::Printf(TEXT("/Game/FarmBuildings/Materials/%s"),Name);
   return FPackageName::DoesPackageExist(Package)?LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("%s.%s"),*Package,Name)):nullptr;
  };
  UMaterialInterface* RoadAsset=Load(TEXT("M_Farm_RoadDecal"));UMaterialInterface* EndAsset=Load(TEXT("M_Farm_RoadEnd"));UMaterialInterface* TrackAsset=Load(TEXT("M_Farm_TrackDecal"));
  UMaterialInterface* BandAsset=Load(TEXT("M_Farm_GroundBand"));UMaterialInterface* FieldAsset=Load(TEXT("M_Farm_WornField"));
  if(!RoadAsset||!EndAsset||!TrackAsset||!BandAsset||!FieldAsset){UE_LOG(LogTemp,Display,TEXT("S47: farm ground decals are not imported, keeping the road ribbon"));return;}
  const auto Tone=[&](UMaterialInterface* Asset,const FLinearColor& Tint,float Opacity,const FLinearColor& ByDay=DecalSoilByDay)->UMaterialInterface*
  {
   auto* Mid=UMaterialInstanceDynamic::Create(Asset,W);Mid->SetVectorParameterValue(TEXT("Tint"),Tint);Mid->SetScalarParameterValue(TEXT("Opacity"),Opacity);DecalTones.Add({Mid,Tint,ByDay});
   // The decals' low mips average the soil with the clear texels round it: a yard pale for the first seconds. All mips stay resident (as the ground's, S47MesaRock).
   Mid->SetForceMipLevelsToBeResident(true,true,-1.f,0,true);return Mid;
  };
  // From A to B along the map's U, centred between them. The box reaches just past the land under it, so a truck on the road is only dusty at the wheels.
  const auto Piece=[&](UMaterialInterface* Mat,double Ax,double Az,double Bx,double Bz,double HalfAcross)
  {
   const double Cx=(Ax+Bx)/2,Cz=(Az+Bz)/2,Dx=Bx-Ax,Dz=Bz-Az,Len=FMath::Max(.01,FMath::Sqrt(Dx*Dx+Dz*Dz)),Cy=S47RoadTerrain::SurfaceY(Cx,Cz);
   double Reach=0;
   for(int32 I=-4;I<=4;++I)for(int32 J=-2;J<=2;++J)
   {
    const double Along=I/8.,Across=J/4.;
    Reach=FMath::Max(Reach,FMath::Abs(S47RoadTerrain::SurfaceY(Cx+Dx*Along-Dz/Len*HalfAcross*Across,Cz+Dz*Along+Dx/Len*HalfAcross*Across)-Cy));
   }
   auto* D=NewObject<UDecalComponent>(W,*FString::Printf(TEXT("FarmGroundDecal_%03d"),FarmDecals.Num()));
   D->SetDecalMaterial(Mat);D->DecalSize=FVector((Reach+DecalMargin)*100,HalfAcross*100,Len*50);D->SetFadeScreenSize(0.f);D->SortOrder=FarmDecals.Num();
   D->SetupAttachment(W->GetRootComponent());D->RegisterComponent();
   FarmDecals.Add({D,FVector(Cx,Cy,Cz),FMath::RadiansToDegrees(FMath::Atan2(Dz,Dx))-90});
  };
  const auto Rz=[](double X){return S47RoadTerrain::RanchZ(X);};
  const auto Local=[](int32 Site,double X,double Y){return S47FarmBuildings::LocalToRoad(S47FarmBuildings::Sites()[Site],X,Y);};
  // Broad trampled ground first, so the road and the soil at the walls draw over it: SortOrder is the order they are made in.
  UMaterialInterface* Yard=Tone(FieldAsset,DecalFieldTint,DecalYardOpacity);UMaterialInterface* Door=Tone(FieldAsset,DecalFieldTint,DecalDoorOpacity);
  const auto Field=[&](UMaterialInterface* Mat,const FVector2D& At,const FVector2D& Along,double HalfAcross,double HalfAlong)
  {
   Piece(Mat,At.X-Along.X*HalfAlong,At.Y-Along.Y*HalfAlong,At.X+Along.X*HalfAlong,At.Y+Along.Y*HalfAlong,HalfAcross);
  };
  const FVector2D Over=(Local(0,1,0)-Local(0,0,0)).GetSafeNormal();
  Field(Yard,FVector2D(124,186),Over,16,19);                 // the yard between the barn, the sheds and the house
  Field(Door,Local(0,1.8,-9.6),Over,5,6.5);                  // in front of the barn's doors
  Field(Door,Local(1,-1,-6.6),Over,4,8.5);                   // in front of the house's porch
  Field(Door,Local(2,0,-5.4),Over,3.2,5.5);                  // the machine shed's open front
  Field(Door,Local(3,-1,-4),Over,2.4,2.8);                   // the feed shed's door
  // The road: 16 m to a map from the yard back to the highway, its first piece shorter.
  UMaterialInterface* RoadMat=Tone(RoadAsset,DecalRoadTint,1.f,DecalRoadByDay);
  Piece(RoadMat,6,Rz(6),14,Rz(14),DecalRoadHalf);
  for(double X=14;X<110;X+=16)Piece(RoadMat,X,Rz(X),X+16,Rz(X+16),DecalRoadHalf);
  // At x 110 the road goes over to two ruts by 118 and to nothing by 126. U=0 is the road's side.
  Piece(Tone(EndAsset,DecalRoadTint,1.f,DecalRoadByDay),110,Rz(110),126,Rz(126),DecalRoadHalf);
  // Two ruts on across the yard to the barn's doors.
  const FVector2D BarnDoor=Local(0,1.8,-6.5);
  Piece(Tone(TrackAsset,DecalRoadTint,1.f,DecalRoadByDay),114,Rz(114)-.6,BarnDoor.X,BarnDoor.Y,DecalRoadHalf);
  // The soil against the walls and in their corners.
  UMaterialInterface* Band=Tone(BandAsset,DecalBandTint,DecalBandOpacity);UMaterialInterface* Corner=Tone(FieldAsset,DecalCornerTint,DecalCornerOpacity);
  for(const S47FarmBuildings::FWallFoot& Foot:S47FarmBuildings::WallFeet())
  {
   const FVector2D A=Local(Foot.Site,Foot.X0,Foot.Y0),B=Local(Foot.Site,Foot.X1,Foot.Y1);
   const int32 Count=FMath::Max(1,FMath::RoundToInt(FVector2D::Distance(A,B)/4));
   for(int32 I=0;I<Count;++I){const FVector2D P0=FMath::Lerp(A,B,double(I)/Count),P1=FMath::Lerp(A,B,double(I+1)/Count);Piece(Band,P0.X,P0.Y,P1.X,P1.Y,DecalBandHalf);}
   Field(Corner,A,Over,1.1,1.1);
  }
  UE_LOG(LogTemp,Display,TEXT("S47: farm ground decals: %d laid, road ribbon retired"),FarmDecals.Num());
 }
 // The soil's tone follows the day (S47MesaRock::ApplyGround): at night the land is much redder than by day, and these would stand out dark on it.
 void ToneFarmDecals(const US47Main* M)
 {
  if(DecalTones.IsEmpty())return;
  const double Hour=M->Freeform?FMath::Fmod(M->Freeform->Clock(),86400.)/3600.:12.;
  const float Day=float(FMath::Clamp(FMath::Min((Hour-5.75)/1.25,(19.25-Hour)/1.25),0.,1.));
  if(FMath::Abs(Day-DecalDay)<.01f)return;DecalDay=Day;
  const FLinearColor Night(2.2f,1.4f,1.05f,1.f);
  for(const FDecalTone& Row:DecalTones)if(auto* Mid=Row.Mid.Get())Mid->SetVectorParameterValue(TEXT("Tint"),Row.Tint*FMath::Lerp(Night,Row.ByDay,Day));
 }
 TWeakObjectPtr<AS47Web> GroundWorld;TWeakObjectPtr<UStaticMeshComponent> YardGround,FarmRoad;bool bGroundTried=false;
 UMaterialInterface* NodeMaterial(const AS47Web* W,int32 Node)
 {
  const auto* N=W->Node(Node);const auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;return C?C->GetMaterial(0):nullptr;
 }
 void ReconcileGround(AS47Web* W,bool On)
 {
  if(GroundWorld.Get()!=W){GroundWorld=W;YardGround.Reset();FarmRoad.Reset();bGroundTried=false;}
  if(On&&!bGroundTried&&W->HighwayLandscapeReady())
  {
   UMaterialInterface* Ring=NodeMaterial(W,7);UMaterialInterface* Gravel=NodeMaterial(W,959);
   if(Ring&&Gravel)
   {
    bGroundTried=true;
    if(auto* Mesh=MakeYardGround(W,Ring))YardGround=NewGroundPart(W,Mesh,TEXT("farm:yardGround"));
    if(auto* Mesh=MakeFarmRoad(W,Gravel))FarmRoad=NewGroundPart(W,Mesh,TEXT("farm:road"));
    UE_LOG(LogTemp,Display,TEXT("S47: farm ground: yard %s, road %s"),YardGround.IsValid()?TEXT("built"):TEXT("FAILED"),FarmRoad.IsValid()?TEXT("built"):TEXT("FAILED"));
   }
  }
  const FString Area=W->Area();const bool bRoadArea=Area==TEXT("road")||Area==TEXT("diner");
  const auto Show=[&](UStaticMeshComponent* C,bool bHere,UMaterialInterface* Material)
  {
   if(!C)return;bHere&=On;
   if(C->IsVisible()!=bHere||C->bHiddenInGame==bHere){C->SetVisibility(bHere);C->SetHiddenInGame(!bHere);}
   if(!bHere)return;
   const FVector At=S47Three::ToUE(S47FarmWorld::RoadToArea(Area,FVector::ZeroVector));if(!C->GetComponentLocation().Equals(At,.01))C->SetWorldLocation(At);
   if(Material&&C->GetMaterial(0)!=Material)C->SetMaterial(0,Material);
  };
  // The road area draws this land itself; only SARO's terms lack it. The ring's and each strip's own material: their day tones follow.
  Show(YardGround.Get(),Area==TEXT("saro"),NodeMaterial(W,7));
  // With the decals the ribbon goes: an opaque plate can only ever end in an edge.
  Show(FarmRoad.Get(),(Area==TEXT("saro")||bRoadArea)&&FarmDecals.IsEmpty(),NodeMaterial(W,bRoadArea?1250:959));
 }
 // The exported strip lies 3.5 cm over the land at rows 4 m apart, -.9 to 6.7 cm between them, with a shadow under its edge.
 // FarmRoad replaces it in Freeform: sunk this far it stays out of sight, and the walking floor is the terrain's and does not move.
 TWeakObjectPtr<AS47Web> SunkWorld;FVector SunkFrom[2]={FVector::ZeroVector,FVector::ZeroVector};bool bSunk[2]={false,false};
 void SinkRanchRoad(AS47Web* W,bool On)
 {
  if(SunkWorld.Get()!=W){SunkWorld=W;bSunk[0]=bSunk[1]=false;}
  // 959 is the SARO corridor's strip, 1250 the road area's own: the farm may stand in either.
  const int32 Strips[2]={959,1250};
  for(int32 K=0;K<2;++K)
  {
   const auto* N=W->Node(Strips[K]);auto* C=N?N->Comp.Get():nullptr;if(!C||On==bSunk[K])continue;
   if(On){SunkFrom[K]=C->GetRelativeLocation();C->SetRelativeLocation(SunkFrom[K]+FVector(0,0,-9));}
   else C->SetRelativeLocation(SunkFrom[K]);
   bSunk[K]=On;
  }
 }
 // The original barn, house and sheds. Freeform only; each is a whole mesh turned on the spot, as the plan has it.
 struct FBuilding{TWeakObjectPtr<UStaticMeshComponent> C;FVector Road;double Degrees=0;};
 TArray<FBuilding> Buildings;TWeakObjectPtr<AS47Web> BuildingWorld;bool bBuildingsTried=false;
 TWeakObjectPtr<UMaterialInstanceDynamic> WindowMid;FLinearColor WindowLit=FLinearColor::Black;
 TWeakObjectPtr<UPointLightComponent> BarnDay;
 void BuildBuildings(AS47Web* W,bool On)
 {
  if(BuildingWorld.Get()!=W){BuildingWorld=W;Buildings.Reset();WindowMid.Reset();bBuildingsTried=false;}
  if(!On||bBuildingsTried||!S47FarmBuildings::Available())return;bBuildingsTried=true;
  for(const S47FarmBuildings::FSite& Site:S47FarmBuildings::Sites())
  {
   auto* Mesh=LoadObject<UStaticMesh>(nullptr,Site.Mesh);
   if(!Mesh){UE_LOG(LogTemp,Error,TEXT("S47: farm building %s did not load"),Site.Mesh);continue;}
   auto* C=NewObject<UStaticMeshComponent>(W);C->SetupAttachment(W->GetRootComponent());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);
   C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
   // The soil's decals reach past the walls; on the walls themselves they would smear. Those have their own dirt.
   C->SetReceivesDecals(false);
   C->SetCastShadow(true);C->RegisterComponent();W->AdoptNode(C,INDEX_NONE,FString(TEXT("farm:building:"))+Site.Name);
   Buildings.Add({C,S47FarmBuildings::Anchor(Site),FMath::RadiansToDegrees(-Site.Yaw)});
   // The daylight that gets in through the big doors and the gaps in the boards. Off after dark.
   if(FCString::Stristr(Site.Mesh,TEXT("Barn")))
   {
    auto* L=NewObject<UPointLightComponent>(W);L->SetupAttachment(C);L->SetMobility(EComponentMobility::Movable);L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(0);
    L->SetLightColor(FLinearColor(1.f,.93f,.82f));L->SetAttenuationRadius(1100.f);L->SetCastShadows(false);L->SetVolumetricScatteringIntensity(0.f);L->RegisterComponent();L->SetRelativeLocation(FVector(0,0,330));
    BarnDay=L;
   }
   const int32 Slot=C->GetMaterialIndex(TEXT("M_Farm_WindowLit"));
   if(Slot!=INDEX_NONE)if(auto* Mid=C->CreateDynamicMaterialInstance(Slot)){WindowMid=Mid;WindowLit=Mid->K2_GetVectorParameterValue(TEXT("EmissiveFactor"));}
  }
  UE_LOG(LogTemp,Display,TEXT("S47: farm buildings: %d of %d shown in Freeform, old boxes retired"),Buildings.Num(),S47FarmBuildings::Sites().Num());
 }
 // web/src ranchLights: the lit window is a glow sprite in the road, SARO and station corridors, in road-local
 // terms at the Story ranch. It stayed there, 310 m east of the Freeform farm. Follow the house, lit at night only.
 struct FWindowGlow{int32 Node=INDEX_NONE,Index=INDEX_NONE;FVector Story=FVector::ZeroVector;};
 TArray<FWindowGlow> WindowGlows;TWeakObjectPtr<AS47Web> WindowWorld;int32 WindowState=-1;
 // web/src ranchLights: the yard light's glow and halo, on a pole at road-local (432.9, 204) with the Story ranch.
 // The Freeform farm has no pole there, so the sprites are hidden (visibility, not scale: ExteriorLighting owns the scale).
 TArray<FWindowGlow> YardGlows;
 void ReconcileWindow(US47Main* M,AS47Web* W,bool On)
 {
  if(!S47FarmBuildings::Available())return;
  if(WindowWorld.Get()!=W)
  {
   WindowWorld=W;WindowGlows.Reset();YardGlows.Reset();WindowState=-1;
   for(const int32 Node:{991,1125,1312})if(const FS47GlowPoints* G=W->Glow(Node))for(int32 I=0;I<G->Num();++I)
   {
    if(FMath::Abs(G->Pos[I].X-424.05)<.05&&FMath::Abs(G->Pos[I].Z-193)<.05)WindowGlows.Add({Node,I,G->Pos[I]});
    else if(FMath::Abs(G->Pos[I].X-432.9)<.05&&FMath::Abs(G->Pos[I].Z-204)<.05)YardGlows.Add({Node,I,G->Pos[I]});
   }
   if(WindowGlows.Num()!=3)UE_LOG(LogTemp,Warning,TEXT("S47: farm window glow: found %d of 3 exported points"),WindowGlows.Num());
  }
  const double Hour=On&&M->Freeform?FMath::Fmod(M->Freeform->Clock(),86400.)/3600.:0;
  const int32 State=!On?0:(Hour>=18.75||Hour<6.25)?2:1; // Same night as S47FarmNight.
  if(State==WindowState)return;WindowState=State;
  for(const FWindowGlow& Row:WindowGlows)if(FS47GlowPoints* G=W->Glow(Row.Node))
  {
   G->SetPosition(Row.Index,On?S47FarmBuildings::EastWindow():Row.Story);
   G->SetOutputScale(Row.Index,State==1?0.f:1.f);
  }
  for(const FWindowGlow& Row:YardGlows)if(FS47GlowPoints* G=W->Glow(Row.Node))if(G->Bill.IsValidIndex(Row.Index)&&G->Bill[Row.Index].IsValid())G->Bill[Row.Index]->SetVisibility(!On);
  if(auto* Mid=WindowMid.Get())Mid->SetVectorParameterValue(TEXT("EmissiveFactor"),State==2?WindowLit:FLinearColor::Black);
  if(auto* L=BarnDay.Get())L->SetIntensity(State==1?1100.f:0.f);
 }
 // A save from before the buildings can stand where a wall is now. The walker never enters one by itself, and it
 // could not leave one either: walk it out to the nearest clear ground. No save field changes.
 void KeepPlayerOutOfWalls(US47Main* M,AS47Web* W)
 {
  if(!M->Player||M->World->bDriving||!S47FarmBuildings::Available())return;
  const FString Area=W->Area();if(Area!=TEXT("saro")&&Area!=TEXT("road")&&Area!=TEXT("diner"))return;
  const FVector Origin=S47FarmWorld::RoadToArea(Area,FVector::ZeroVector);const double X=M->Player->Pos.X-Origin.X,Z=M->Player->Pos.Z-Origin.Z,R=M->Player->Radius;
  if(!S47FarmBuildings::OnPlot(X,Z,1)||!S47FarmBuildings::Blocked(X,Z,R-.02))return;
  for(double D=.5;D<16;D+=.5)for(int32 I=0;I<16;++I)
  {
   const double A=2*UE_DOUBLE_PI*I/16,Px=X+FMath::Cos(A)*D,Pz=Z+FMath::Sin(A)*D;
   if(S47FarmBuildings::Blocked(Px,Pz,R+.05))continue;
   M->Player->Pos.X=Px+Origin.X;M->Player->Pos.Z=Pz+Origin.Z;
   UE_LOG(LogTemp,Warning,TEXT("S47: farm buildings: player stood in a wall at road %.2f %.2f, moved %.1f m out"),X,Z,D);
   return;
  }
 }
 void Build(AS47Web* W)
 {
  if(Built==W)return;Built=W;Parts.Reset();auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));if(!Cube)return;
  auto* Wood=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/TownBuildings/Materials/M_Town_DarkWood.M_Town_DarkWood"));auto* Metal=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/TownBuildings/Materials/M_Town_Metal.M_Town_Metal"));
  auto Box=[&](const FString& Name,FVector Road,FVector Size,UMaterialInterface* Material)
  {
   auto* C=NewObject<UStaticMeshComponent>(W);C->SetupAttachment(W->GetRootComponent());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Cube);if(Material)C->SetMaterial(0,Material);
   C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCastShadow(true);C->RegisterComponent();W->AdoptNode(C,INDEX_NONE,Name);Parts.Add({C,Road,Size});
  };
  // The road is the terrain's own ranch road. Flat gravel plates here stood in steps and overlapped it (Tom, 6 November).
  auto Fence=[&](double X0,double Z0,double X1,double Z1,int32 Id)
  {
   const double L=FVector2D(X1-X0,Z1-Z0).Size();const int32 Count=FMath::CeilToInt(L/4.);
   for(int32 I=0;I<=Count;++I){const double K=double(I)/Count,X=FMath::Lerp(X0,X1,K),Z=FMath::Lerp(Z0,Z1,K);Box(FString::Printf(TEXT("farm:fence:%d:post%d"),Id,I),FVector(X,S47FarmWorld::FloorRoad(X,Z)+.57,Z),FVector(.12,1.25,.12),Wood);}
   for(double H:{.45,.95}){const double X=(X0+X1)*.5,Z=(Z0+Z1)*.5;Box(FString::Printf(TEXT("farm:fence:%d:rail%.0f"),Id,H*100),FVector(X,S47FarmWorld::FloorRoad(X,Z)+H,Z),X0==X1?FVector(.05,.06,L):FVector(L,.06,.05),Metal);}
  };
  Fence(30,215,30,265,0);Fence(80,215,80,265,1);Fence(30,265,80,265,2);Fence(30,215,66,215,3);Fence(75,215,80,215,4);
  auto* Plaster=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/TownBuildings/Materials/M_Town_ChurchWhiteBoard.M_Town_ChurchWhiteBoard"));
  for(const auto& Piece:S47FarmHouse::Pieces())Box(Piece.Name,Piece.Centre,Piece.Size,Piece.Mat==S47FarmHouse::EMat::Wall?Plaster:Wood);
  const double Y=S47FarmWorld::FloorRoad(76,251);Box(TEXT("farm:trough:bottom"),{76,Y+.15,251},{2,.16,.9},Metal);
  for(double DX:{-1.,1.})Box(TEXT("farm:trough:end"),{76+DX,Y+.40,251},{.10,.6,.9},Metal);
  for(double DZ:{-.45,.45})Box(TEXT("farm:trough:side"),{76,Y+.40,251+DZ},{2,.6,.08},Metal);
 }
}
namespace S47FarmWorld
{
 bool Enabled(const US47Main* M){return M&&M->IsFreeform();}
 FVector HouseOffset(const US47Main* M){return Enabled(M)?FVector(-310,0,0):FVector::ZeroVector;}
 FVector HerdOffset(const US47Main* M){return Enabled(M)?FVector(92,0,-316):FVector::ZeroVector;}
 FVector RoadToArea(const FString& Area,const FVector& P)
 {
  if(Area==TEXT("saro"))return P+FVector(-24,0,300);
  if(Area==TEXT("station01")){const auto S=S47Geo::RoadToStation(P.X,P.Z);return FVector(S.X,P.Y+S47Geo::STATION_DY(),S.Z+8000);}
  return P+FVector(8000,0,0);
 }
 bool InRoadAccess(double X,double Z)
 {
  return (X>=6&&X<=116&&FMath::Abs(Z-S47RoadTerrain::RanchZ(X))<=3.1)||(X>=27&&X<=161&&Z>=150&&Z<=268);
 }
 double FloorRoad(double X,double Z)
 {
  double Y=0;if(S47FarmBuildings::FloorAt(X,Z,Y))return Y;
  return S47FarmHouse::OnPad(X,Z)?S47FarmHouse::PadY():S47RoadTerrain::SurfaceY(X,Z);
 }
 bool BuildingsShown(const US47Main* M)
 {
  if(!M||!M->Web||BuildingWorld.Get()!=M->Web.Get()||Buildings.Num()!=S47FarmBuildings::Sites().Num())return false;
  for(const auto& B:Buildings){const auto* C=B.C.Get();if(!C||!C->GetStaticMesh()||!C->IsVisible()||C->bHiddenInGame)return false;}
  return true;
 }
 int32 WindowGlowsAtHouse(const US47Main* M)
 {
  if(!M||!M->Web||WindowWorld.Get()!=M->Web.Get())return 0;int32 Count=0;
  for(const auto& Row:WindowGlows)if(const FS47GlowPoints* G=M->Web->Glow(Row.Node))Count+=G->Pos.IsValidIndex(Row.Index)&&FVector::Dist(G->Pos[Row.Index],S47FarmBuildings::EastWindow())<.01;
  return Count;
 }
 bool HouseVisible(const US47Main* M)
 {
  if(BuildingsShown(M))return true;
  if(!M||!M->Web)return false;int32 Count=0;for(int32 I=0;;++I){const auto* N=M->Web->Node(I);if(!N)break;if(N->Name.StartsWith(TEXT("highway:ranch:g")))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))if(C->IsVisible()&&!C->bHiddenInGame)++Count;}return Count==3;
 }
 FVector HousePhotoPoint(const US47Main* M)
 {
  if(!M||!M->Web)return FVector::ZeroVector;FBox Bounds(ForceInit);
  // Between the house and the barn: the yard, not a point inside a wall. Without them, the middle of the old group.
  if(BuildingsShown(M)){for(int32 I=0;I<2;++I)Bounds+=Buildings[I].C->Bounds.GetBox();return S47Three::ToThree(Bounds.GetCenter());}
  for(int32 I=0;;++I){const auto* N=M->Web->Node(I);if(!N)break;if(N->Name.StartsWith(TEXT("highway:ranch:g")))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))Bounds+=C->Bounds.GetBox();}return Bounds.IsValid?S47Three::ToThree(Bounds.GetCenter()):FVector::ZeroVector;
 }
 void Reconcile(US47Main* M)
 {
  if(!M||!M->Web||!M->World)return;auto* W=M->Web.Get();S47FreeformTerrain::Reconcile(M);S47RoadEdge::Reconcile(W,Enabled(M));ReconcileRanch(M);const bool On=Enabled(M)&&GateReady(M);M->World->ReconcileFarm(On);Build(W);BuildBuildings(W,On);BuildFarmDecals(W,On);ClearRoad(W,On);ClearHousePad(W,On);SinkRanchRoad(W,On);ReconcileGround(W,On);ReconcileWindow(M,W,On);if(On)ToneFarmDecals(M);
  if(On)KeepPlayerOutOfWalls(M,W);
  if(On)M->World->KeepParkedOutOfFarmWalls();
  const bool Apply=AppliedWorld!=W||AppliedArea!=W->Area()||AppliedOn!=On;
  if(Apply)for(auto& P:Parts)if(auto* C=P.C.Get())
  {
   C->SetWorldLocation(S47Three::ToUE(RoadToArea(W->Area(),P.Road)));C->SetWorldRotation(W->Area()==TEXT("station01")?FRotator(0,90,0):FRotator::ZeroRotator);
   C->SetWorldScale3D(FVector(P.Size.X,P.Size.Z,P.Size.Y));C->SetVisibility(On&&S47HighwayLandscape::Exterior(W->Area()));C->SetHiddenInGame(!On);C->SetCollisionEnabled(On?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
  }
  if(Apply)for(const auto& B:Buildings)if(auto* C=B.C.Get())
  {
   C->SetWorldLocation(S47Three::ToUE(RoadToArea(W->Area(),B.Road)));C->SetWorldRotation(FRotator(0,B.Degrees+(W->Area()==TEXT("station01")?90:0),0));
   C->SetVisibility(On&&S47HighwayLandscape::Exterior(W->Area()));C->SetHiddenInGame(!On);C->SetCollisionEnabled(On?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
  }
  if(Apply)for(const auto& D:FarmDecals)if(auto* C=D.C.Get())
  {
   C->SetWorldLocationAndRotation(S47Three::ToUE(RoadToArea(W->Area(),D.Road)),FRotator(-90,D.Degrees+(W->Area()==TEXT("station01")?90:0),90));
   C->SetVisibility(On&&S47HighwayLandscape::Exterior(W->Area()));C->SetHiddenInGame(!On);
  }
  AppliedWorld=W;AppliedArea=W->Area();AppliedOn=On;
  if(M->TownLayer)for(const auto& H:M->TownLayer->Herds)if(H){H->ReconcileMode();if(Enabled(M)){H->Set(false,-1,false);H->Update(M->T);}}
 }
 int32 MovedRockCount(const US47Main* M){return M&&RockWorld==M->Web.Get()&&RocksOn?Rocks.Num():0;}
 bool RoadClearance(US47Main* M,TArray<FString>& Lines)
 {
  if(!M||!M->Web)return false;auto* W=M->Web.Get();bool Good=RockWorld==W&&RocksScanned&&RocksSafe&&RocksOn==Enabled(M);int32 Checked=0;
  for(int32 Node=0;;++Node){const auto* N=W->Node(Node);if(!N)break;if(!N->Name.StartsWith(TEXT("props:rocksEast_"))&&!N->Name.StartsWith(TEXT("props:rocksWest_")))continue;if(auto* C=Cast<UInstancedStaticMeshComponent>(N->Comp.Get()))if(C->GetStaticMesh())for(int32 I=0;I<C->GetInstanceCount();++I){FTransform T;if(C->GetInstanceTransform(I,T,true)){++Checked;if(Enabled(M)&&OverlapsLane(RoadBounds(W,C,T)))Good=false;}}}
  for(const auto& R:Rocks){auto* C=R.C.Get();FTransform T;if(!C||!C->GetInstanceTransform(R.Index,T,false)||!T.Equals(Enabled(M)?R.Clear:R.Original,.00001))Good=false;}
  Lines.Add(FString::Printf(TEXT("%s farm road clearance actual instance bounds checked=%d relocated=%d mode=%s"),Good?TEXT("PASS"):TEXT("FAIL"),Checked,Rocks.Num(),Enabled(M)?TEXT("Freeform"):TEXT("Story")));return Good;
 }
 bool Ready(US47Main* M){TArray<FString> Clearance;return RoadClearance(M,Clearance)&&Enabled(M)&&GateReady(M)&&HouseVisible(M)&&Built==M->Web.Get()&&Parts.Num()>30;}
 bool Audit(US47Main* M,TArray<FString>& Lines)
 {
  const bool Good=M&&GateReady(M)&&RoadClearance(M,Lines)&&(!Enabled(M)||Ready(M));Lines.Add(FString(Good?TEXT("PASS "):TEXT("FAIL "))+TEXT("farm exact gate partitions, native housegroup and mode-derived physical access"));return Good;
 }
 bool TruckHits(const TArray<FS47Obstacle>& Obstacles,double X,double Z,double Heading,double HalfWidth,double HalfLength,double Shrink)
 {
  const double Hw=HalfWidth-Shrink,Hl=HalfLength-Shrink*2,C=FMath::Cos(Heading),Sn=FMath::Sin(Heading);
  for(const FS47Obstacle& O:Obstacles)
  {
   if(O.bCircle)
   {
    const double Dx=O.X-X,Dz=O.Z-Z;if(Dx*Dx+Dz*Dz>FMath::Square(O.R+FMath::Sqrt(Hw*Hw+Hl*Hl)))continue;
    const double Lx=Dx*C-Dz*Sn,Lz=Dx*Sn+Dz*C,Qx=Lx-FMath::Clamp(Lx,-Hw,Hw),Qz=Lz-FMath::Clamp(Lz,-Hl,Hl);
    if(Qx*Qx+Qz*Qz<O.R*O.R)return true;
    continue;
   }
   const double Bx=(O.MinX+O.MaxX)/2,Bz=(O.MinZ+O.MaxZ)/2,Hx=(O.MaxX-O.MinX)/2,Hz=(O.MaxZ-O.MinZ)/2,Ac=FMath::Abs(C),As=FMath::Abs(Sn),Dx=Bx-X,Dz=Bz-Z;
   if(FMath::Abs(Dx)>Hx+Hw*Ac+Hl*As||FMath::Abs(Dz)>Hz+Hw*As+Hl*Ac)continue;
   if(FMath::Abs(Dx*C-Dz*Sn)>Hw+Hx*Ac+Hz*As||FMath::Abs(Dx*Sn+Dz*C)>Hl+Hx*As+Hz*Ac)continue;
   return true;
  }
  return false;
 }
 bool NearestClearTruckSpot(const TArray<FS47Obstacle>& Obstacles,double X,double Z,double Heading,double HalfWidth,double HalfLength,double& OutX,double& OutZ)
 {
  OutX=X;OutZ=Z;
  if(!TruckHits(Obstacles,X,Z,Heading,HalfWidth,HalfLength,.05))return true; // a legal park stays where it is
  for(double D=.5;D<=30;D+=.5)for(int32 I=0;I<24;++I)
  {
   const double A=2*UE_DOUBLE_PI*I/24,Px=X+FMath::Cos(A)*D,Pz=Z+FMath::Sin(A)*D;
   if(TruckHits(Obstacles,Px,Pz,Heading,HalfWidth+.15,HalfLength+.15,0))continue;
   OutX=Px;OutZ=Pz;return true;
  }
  return false;
 }
 bool SelfTest(TArray<FString>& Lines)
 {
  const FBox Edge(FVector(10,0,S47RoadTerrain::RanchZ(11)+3.3),FVector(12,2,S47RoadTerrain::RanchZ(11)+6));
  const FBox Verge(FVector(10,0,S47RoadTerrain::RanchZ(11)-20),FVector(12,2,S47RoadTerrain::RanchZ(11)-10));
  S47FarmHouse::SelfTest(Lines);S47FarmBuildings::SelfTest(Lines);
  const bool Good=OverlapsLane(Edge)&&!OverlapsLane(Verge)&&InRoadAccess(21,S47RoadTerrain::RanchZ(21))&&InRoadAccess(96,196.8)&&InRoadAccess(55,240)&&!InRoadAccess(200,196)&&RoadToArea(TEXT("saro"),{21,0,196}).Equals({-3,0,496},.00001);
  Lines.Add(FString(Good?TEXT("PASS "):TEXT("FAIL "))+TEXT("Alderete road, yard, pasture and scene coordinates remain bounded"));
  // A save from before the buildings can park the truck inside a wall: it must come out on clear ground, and a truck on the road must stay.
  bool Moved=true;
  if(S47FarmBuildings::Available())
  {
   TArray<FS47Obstacle> Walls;for(const FBox2D& B:S47FarmBuildings::Obstacles())Walls.Add(FS47Obstacle::Box(B.Min.X,B.Max.X,B.Min.Y,B.Max.Y));
   const double Hw=1.05,Hl=2.75;const TArray<FBox2D>& Each=S47FarmBuildings::Obstacles();
   // Across the walls themselves: a building's middle is open floor where a truck is legal.
   for(int32 I=0;I<Each.Num();I+=FMath::Max(1,Each.Num()/12))for(const double H:{0.,.8,1.57})
   {
    const FVector2D P=Each[I].GetCenter();
    double Ox=0,Oz=0;const bool Hit=TruckHits(Walls,P.X,P.Y,H,Hw,Hl,.05);
    const bool Found=NearestClearTruckSpot(Walls,P.X,P.Y,H,Hw,Hl,Ox,Oz);
    Moved&=Hit&&Found&&!TruckHits(Walls,Ox,Oz,H,Hw,Hl,.05)&&FVector2D::Distance(P,FVector2D(Ox,Oz))<30;
   }
   double Kx=0,Kz=0;const double RoadZ=S47RoadTerrain::RanchZ(60);
   Moved&=NearestClearTruckSpot(Walls,60,RoadZ,0,Hw,Hl,Kx,Kz)&&Kx==60&&Kz==RoadZ;
  }
  Lines.Add(FString(Moved?TEXT("PASS "):TEXT("FAIL "))+TEXT("farm buildings: a truck parked in a wall by an old save is moved to the nearest clear ground, one on the road is left alone"));
  const bool Relief=S47Relief::SelfTest(Lines);
  return Good&&Moved&&Relief;
 }
}
