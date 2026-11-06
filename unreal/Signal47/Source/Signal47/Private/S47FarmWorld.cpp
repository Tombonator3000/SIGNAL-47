#include "S47FarmWorld.h"
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
#include "S47HighwayLandscape.h"
#include "S47Three.h"
#include "S47TownLayer.h"
#include "S47Herd.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
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
 // The farm road's strip lies 3 to 8 cm over the land with a visible edge, which reads as floating. Sunk this
 // far its edge meets the ground instead; the walking floor is the terrain's and does not move.
 TWeakObjectPtr<AS47Web> SunkWorld;FVector SunkFrom=FVector::ZeroVector;bool bSunk=false;
 void SinkRanchRoad(AS47Web* W,bool On)
 {
  if(SunkWorld.Get()!=W){SunkWorld=W;bSunk=false;}
  const auto* N=W->Node(959);auto* C=N?N->Comp.Get():nullptr;if(!C)return;
  if(On==bSunk)return;
  if(On){SunkFrom=C->GetRelativeLocation();C->SetRelativeLocation(SunkFrom+FVector(0,0,-9));}
  else C->SetRelativeLocation(SunkFrom);
  bSunk=On;
 }
 // The original barn, house and sheds. Freeform only; each is a whole mesh turned on the spot, as the plan has it.
 struct FBuilding{TWeakObjectPtr<UStaticMeshComponent> C;FVector Road;double Degrees=0;};
 TArray<FBuilding> Buildings;TWeakObjectPtr<AS47Web> BuildingWorld;bool bBuildingsTried=false;
 TWeakObjectPtr<UMaterialInstanceDynamic> WindowMid;FLinearColor WindowLit=FLinearColor::Black;
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
   C->SetCastShadow(true);C->RegisterComponent();W->AdoptNode(C,INDEX_NONE,FString(TEXT("farm:building:"))+Site.Name);
   Buildings.Add({C,S47FarmBuildings::Anchor(Site),FMath::RadiansToDegrees(-Site.Yaw)});
   const int32 Slot=C->GetMaterialIndex(TEXT("M_Farm_WindowLit"));
   if(Slot!=INDEX_NONE)if(auto* Mid=C->CreateDynamicMaterialInstance(Slot)){WindowMid=Mid;WindowLit=Mid->K2_GetVectorParameterValue(TEXT("EmissiveFactor"));}
  }
  UE_LOG(LogTemp,Display,TEXT("S47: farm buildings: %d of %d shown in Freeform, old boxes retired"),Buildings.Num(),S47FarmBuildings::Sites().Num());
 }
 // web/src ranchLights: the lit window is a glow sprite in the road, SARO and station corridors, in road-local
 // terms at the Story ranch. It stayed there, 310 m east of the Freeform farm. Follow the house, lit at night only.
 struct FWindowGlow{int32 Node=INDEX_NONE,Index=INDEX_NONE;FVector Story=FVector::ZeroVector;};
 TArray<FWindowGlow> WindowGlows;TWeakObjectPtr<AS47Web> WindowWorld;int32 WindowState=-1;
 void ReconcileWindow(US47Main* M,AS47Web* W,bool On)
 {
  if(!S47FarmBuildings::Available())return;
  if(WindowWorld.Get()!=W)
  {
   WindowWorld=W;WindowGlows.Reset();WindowState=-1;
   for(const int32 Node:{991,1125,1312})if(const FS47GlowPoints* G=W->Glow(Node))for(int32 I=0;I<G->Num();++I)
    if(FMath::Abs(G->Pos[I].X-424.05)<.05&&FMath::Abs(G->Pos[I].Z-193)<.05)WindowGlows.Add({Node,I,G->Pos[I]});
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
  if(auto* Mid=WindowMid.Get())Mid->SetVectorParameterValue(TEXT("EmissiveFactor"),State==2?WindowLit:FLinearColor::Black);
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
  if(!M||!M->Web||!M->World)return;auto* W=M->Web.Get();ReconcileRanch(M);const bool On=Enabled(M)&&GateReady(M);M->World->ReconcileFarm(On);Build(W);BuildBuildings(W,On);ClearRoad(W,On);ClearHousePad(W,On);SinkRanchRoad(W,On);ReconcileWindow(M,W,On);
  if(On)KeepPlayerOutOfWalls(M,W);
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
 bool SelfTest(TArray<FString>& Lines)
 {
  const FBox Edge(FVector(10,0,S47RoadTerrain::RanchZ(11)+3.3),FVector(12,2,S47RoadTerrain::RanchZ(11)+6));
  const FBox Verge(FVector(10,0,S47RoadTerrain::RanchZ(11)-20),FVector(12,2,S47RoadTerrain::RanchZ(11)-10));
  S47FarmHouse::SelfTest(Lines);S47FarmBuildings::SelfTest(Lines);
  const bool Good=OverlapsLane(Edge)&&!OverlapsLane(Verge)&&InRoadAccess(21,S47RoadTerrain::RanchZ(21))&&InRoadAccess(96,196.8)&&InRoadAccess(55,240)&&!InRoadAccess(200,196)&&RoadToArea(TEXT("saro"),{21,0,196}).Equals({-3,0,496},.00001);
  Lines.Add(FString(Good?TEXT("PASS "):TEXT("FAIL "))+TEXT("Alderete road, yard, pasture and scene coordinates remain bounded"));return Good;
 }
}
