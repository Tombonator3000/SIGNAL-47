// LAND-PLACES-WINDMILL-20261114 (→ M6, M1): see S47LandWindmill.h. Unreal's frame for a mill is its own ground: x east, y south,
// z up, centimetres (the meshes come in so: Blender's north is -y here), under a root that stands on the place.
#include "S47LandWindmill.h"
#include "S47Drive.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47Happenings.h"
#include "S47HighwayLandscape.h"
#include "S47Inter.h"
#include "S47Legs.h"
#include "S47Main.h"
#include "S47Places.h"
#include "S47Predecessor.h"
#include "S47PredecessorText.h"
#include "S47RoadTerrain.h"
#include "S47Soundscape.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Weather.h"
#include "S47Web.h"
#include "S47World.h"
#include "CollisionQueryParams.h"
#include "Components/DecalComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47LandWindmill, Log, All);

namespace S47LandWindmill
{
namespace S47LandWindmillLocal
{
 const TCHAR* const Folder=TEXT("/Game/LandPlaces/Windmill/SM_LP_Mill_");
 const TCHAR* const Names[]={TEXT("Tower"),TEXT("Pier"),TEXT("Rod"),TEXT("Head"),TEXT("Wheel"),TEXT("Housing"),TEXT("Lid"),TEXT("Tank"),TEXT("Float"),TEXT("Water"),TEXT("Page"),TEXT("VaneLabel"),TEXT("WireRun"),TEXT("WireKnot")};
 const TCHAR* const PlaceId=TEXT("windmill");
 const TCHAR* const PageDoc=TEXT("book_windmill");
 const TCHAR* const LidUse=TEXT("ff:mill:lid");
 const TCHAR* const PageUse=TEXT("ff:mill:page");
 const TCHAR* const BoxId=TEXT("landplace:windmill:");
 // The farm's (S47FarmWindmill: the tower over the old rods' feet, the tank beside it), road-local.
 const FVector2D FarmMill(146,214),FarmTank(150.7,210.2);
 constexpr double FarmTankR=2.3,FarmTankH=.72,LidOpenDeg=98,Away=1000000;

 struct FMill
 {
  bool bFarm=false,bBuilt=false,bShown=false,bSeated=false;
  TWeakObjectPtr<AS47Web> Web;
  TWeakObjectPtr<USceneComponent> Root;
  TWeakObjectPtr<UStaticMeshComponent> Tower,Rod,Head,Wheel,Housing,Lid,Tank,Float,Water,Page,Label,Run,KnotA,KnotB,LidWire,Pier[4];
  FVector Stand=FVector::ZeroVector;      // three metres, the area's frame: the place on its ground
  double PlaceX=0,PlaceZ=0;               // road-local
  double TowerUp=0,HouseUp=0,TankUp=0;    // each piece's own ground over the place's
  double PierGround[4]={0,0,0,0},TankGround[8]={0,0,0,0,0,0,0,0},TankLow=0;
  double HeadAz=0,WheelDeg=0,WheelRate=0,RodDeg=0,LidK=0,LidWant=0,Age=0;
  double Strain=99,KnockFor=0;bool bKnocking=false,bTraces=false,bReadBefore=false,bWiredBefore=false;
  FString Area;int32 AppliedOn=-1;
  TArray<FS47Obstacle> Drive;bool bDriveOn=false;
  double PlantsIn=0;int32 PlantLooks=0,PlantsGone=0;
  TWeakObjectPtr<UDecalComponent> Ground;TWeakObjectPtr<UMaterialInstanceDynamic> GroundMat;float GroundDay=-1.f;
 };
 FMill Baca,Farm;
 struct FDev{bool bOn=false,bLid=false,bPage=false,bWire=false;} Dev;

 bool Off(){static const bool bOff=FParse::Param(FCommandLine::Get(),TEXT("S47LandMillOff"));return bOff;}
 FString PackageOf(const TCHAR* Name){return FString(Folder)+Name;}
 UStaticMesh* MeshOf(const TCHAR* Name)
 {
  const FString Package=PackageOf(Name);
  if(!FPackageName::DoesPackageExist(Package))return nullptr;
  return LoadObject<UStaticMesh>(nullptr,*(Package+TEXT(".")+FPackageName::GetShortName(Package)),nullptr,LOAD_NoWarn|LOAD_Quiet);
 }
 int32 Present(){int32 N=0;for(const TCHAR* Name:Names)N+=FPackageName::DoesPackageExist(PackageOf(Name))?1:0;return N;}
 double HalfAt(double Y){return Foot+(.15-Foot)*(Y-PierTop)/(9.05-PierTop);}
 const double CornerX[4]={1,-1,-1,1},CornerZ[4]={-1,-1,1,1};   // NE, NW, SW, SE (z south)

 UStaticMeshComponent* Make(AS47Web* W,USceneComponent* Parent,const TCHAR* Mesh,const FString& Name,bool bSolid,bool bShadow=true)
 {
  UStaticMesh* Asset=MeshOf(Mesh);if(!Asset||!Parent)return nullptr;
  auto* C=NewObject<UStaticMeshComponent>(W,*Name);
  C->SetupAttachment(Parent);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Asset);C->SetCastShadow(bShadow);C->bCastContactShadow=false;
  C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->SetReceivesDecals(false);
  if(bSolid){C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);}
  else C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->RegisterComponent();
  return C;
 }
 void ForEach(FMill& Mill,TFunctionRef<void(UStaticMeshComponent*)> Fn)
 {
  for(const auto& P:{Mill.Tower,Mill.Rod,Mill.Head,Mill.Wheel,Mill.Housing,Mill.Lid,Mill.Tank,Mill.Float,Mill.Water,Mill.Page,Mill.Label,Mill.Run,Mill.KnotA,Mill.KnotB,Mill.LidWire,Mill.Pier[0],Mill.Pier[1],Mill.Pier[2],Mill.Pier[3]})
   if(auto* C=P.Get())Fn(C);
 }
 bool Build(FMill& Mill,AS47Web* W,const TCHAR* Tag)
 {
  auto* R=NewObject<USceneComponent>(W,*FString::Printf(TEXT("S47LandMill_%s"),Tag));
  R->SetupAttachment(W->GetRootComponent());R->SetMobility(EComponentMobility::Movable);R->RegisterComponent();
  Mill.Root=R;Mill.Web=W;
  const auto Name=[Tag](const TCHAR* Part){return FString::Printf(TEXT("S47LandMill_%s_%s"),Tag,Part);};
  Mill.Tower=Make(W,R,TEXT("Tower"),Name(TEXT("Tower")),true);
  for(int32 K=0;K<4;++K)Mill.Pier[K]=Make(W,R,TEXT("Pier"),Name(*FString::Printf(TEXT("Pier%d"),K)),true);
  Mill.Rod=Make(W,R,TEXT("Rod"),Name(TEXT("Rod")),false);
  Mill.Head=Make(W,R,TEXT("Head"),Name(TEXT("Head")),false);
  Mill.Wheel=Make(W,Mill.Head.Get(),TEXT("Wheel"),Name(TEXT("Wheel")),false);
  Mill.Housing=Make(W,R,TEXT("Housing"),Name(TEXT("Housing")),true);
  Mill.Lid=Make(W,Mill.Housing.Get(),TEXT("Lid"),Name(TEXT("Lid")),true);
  Mill.Tank=Make(W,R,TEXT("Tank"),Name(TEXT("Tank")),true);
  Mill.Float=Make(W,Mill.Tank.Get(),TEXT("Float"),Name(TEXT("Float")),false);
  Mill.Water=Make(W,Mill.Tank.Get(),TEXT("Water"),Name(TEXT("Water")),false,false);
  if(!Mill.bFarm)
  {
   Mill.Page=Make(W,Mill.Housing.Get(),TEXT("Page"),Name(TEXT("Page")),true,false);
   Mill.Label=Make(W,Mill.Head.Get(),TEXT("VaneLabel"),Name(TEXT("VaneLabel")),false,false);
   Mill.Run=Make(W,R,TEXT("WireRun"),Name(TEXT("WireRun")),false,false);
   Mill.KnotA=Make(W,Mill.Head.Get(),TEXT("WireKnot"),Name(TEXT("WireKnotA")),false,false);
   Mill.KnotB=Make(W,R,TEXT("WireKnot"),Name(TEXT("WireKnotB")),false,false);
   Mill.LidWire=Make(W,Mill.Housing.Get(),TEXT("WireKnot"),Name(TEXT("LidWire")),false,false);
  }
  if(!Mill.Tower.IsValid()||!Mill.Head.IsValid()||!Mill.Wheel.IsValid()||!Mill.Housing.IsValid()||!Mill.Lid.IsValid()||!Mill.Tank.IsValid())return false;
  // what never moves on its parent
  if(auto* C=Mill.Wheel.Get())C->SetRelativeLocation(FVector(0,HubOff*100,0));
  if(auto* C=Mill.Lid.Get())C->SetRelativeLocation(FVector(0,HingeZ*100,HingeY*100));
  if(auto* C=Mill.Float.Get())C->SetRelativeLocation(FVector((ValveX-TankX)*100,0,ValveY*100));
  if(auto* C=Mill.Page.Get())C->SetRelativeLocation(FVector(0,7.8,35.5));   // (the bag hangs on the pump's gland, where the lifted lid shows it)
  if(auto* C=Mill.LidWire.Get()){C->SetRelativeLocation(FVector(0,(HouseZ1+.028)*100,(HouseTop-.045)*100));C->SetRelativeRotation(FRotator(90,0,0));C->SetRelativeScale3D(FVector(.42));}
  if(auto* C=Mill.KnotA.Get()){C->SetRelativeLocation(FVector(0,-105,0));C->SetRelativeRotation(FRotator(0,0,90));C->SetRelativeScale3D(FVector(1.25));}
  Mill.bBuilt=true;
  return true;
 }
 // Where every piece sits, from the ground under it. Ground: metres over the place's own height at an offset on its ground.
 void Seat(FMill& Mill,TFunctionRef<double(double,double)> Ground,const FVector2D& TankAt,double TankScale,double TankTall,double TankYaw)
 {
  double G[4];
  for(int32 K=0;K<4;++K){G[K]=Ground(CornerX[K]*Foot,CornerZ[K]*Foot);Mill.PierGround[K]=G[K];}
  Mill.TowerUp=TowerSeat(G);
  // the housing on its own ground, never so far under the tower that the rod would show over the stand pipe
  Mill.HouseUp=FMath::Max(Ground(0,(HouseZ0+HouseZ1)*.5),Mill.TowerUp-.15);
  // the tank level on the highest ground under its rim; its wall goes TankDeep on down
  double Hi=-1e9,Lo=1e9;
  for(int32 K=0;K<8;++K){const double A=K*UE_DOUBLE_PI/4,Y=Ground(TankAt.X+FMath::Cos(A)*TankR*TankScale,TankAt.Y+FMath::Sin(A)*TankR*TankScale);Hi=FMath::Max(Hi,Y);Lo=FMath::Min(Lo,Y);Mill.TankGround[K]=Y;}
  Mill.TankUp=Hi-.02;Mill.TankLow=Lo;
  if(auto* C=Mill.Tower.Get())C->SetRelativeLocation(FVector(0,0,Mill.TowerUp*100));
  for(int32 K=0;K<4;++K)if(auto* C=Mill.Pier[K].Get()){C->SetRelativeLocation(FVector(CornerX[K]*Foot*100,CornerZ[K]*Foot*100,Mill.TowerUp*100));C->SetRelativeRotation(FRotator(0,90.*K,0));}
  if(auto* C=Mill.Housing.Get())C->SetRelativeLocation(FVector(0,0,Mill.HouseUp*100));
  if(auto* C=Mill.Tank.Get()){C->SetRelativeLocation(FVector(TankAt.X*100,TankAt.Y*100,Mill.TankUp*100));C->SetRelativeRotation(FRotator(0,TankYaw,0));C->SetRelativeScale3D(FVector(TankScale,TankScale,TankTall));}
 }
 // The bright new wire of the first days: a turn round the tail's spine, a run down to the nearest leg at the fourth girt, a turn there.
 void Wire(FMill& Mill,bool bOn)
 {
  for(const auto& P:{Mill.Run,Mill.KnotA,Mill.KnotB})if(auto* C=P.Get())C->SetHiddenInGame(!bOn);
  auto* Head=Mill.Head.Get();auto* Run=Mill.Run.Get();auto* Knot=Mill.KnotB.Get();
  if(!bOn||!Head||!Run||!Knot)return;
  const FVector A=Head->GetRelativeLocation()+Head->GetRelativeRotation().RotateVector(FVector(0,-105,-4));
  const double GirtY=6.85,H=HalfAt(GirtY)*100;
  FVector B=FVector::ZeroVector;double Best=-1e18;
  for(int32 K=0;K<4;++K){const FVector Corner(CornerX[K]*H,CornerZ[K]*H,(Mill.TowerUp+GirtY+.06)*100);const double Near=-FVector::DistSquared(Corner,A);if(Near>Best){Best=Near;B=Corner;}}
  const FVector D=B-A;const double L=D.Size();if(L<1)return;
  Run->SetRelativeLocation(A);Run->SetRelativeRotation(FRotationMatrix::MakeFromZ(D/L).Rotator());Run->SetRelativeScale3D(FVector(1.8,1.8,L/100.));
  Knot->SetRelativeLocation(B);Knot->SetRelativeScale3D(FVector(1.5));
 }
 void Pose(FMill& Mill,double Wind)
 {
  const double T=Mill.Strain;
  const double Kick=T<8?FMath::Exp(-T/1.3)*FMath::Sin(T*UE_DOUBLE_PI*2*1.1):0.;
  if(auto* C=Mill.Head.Get()){C->SetRelativeLocation(FVector(0,0,(Mill.TowerUp+HubY)*100));C->SetRelativeRotation(FRotator(0,HeadYaw(Mill.HeadAz)+Kick*.5,0));}
  if(auto* C=Mill.Wheel.Get())C->SetRelativeRotation(FQuat(FVector(0,1,0),FMath::DegreesToRadians(Mill.WheelDeg+Kick*1.6)));
  if(auto* C=Mill.Rod.Get())C->SetRelativeLocation(FVector(0,0,(Mill.TowerUp+RodLift(Mill.RodDeg))*100));
  const double Open=FMath::SmoothStep(0.,1.,Mill.LidK)*LidOpenDeg;
  if(auto* C=Mill.Lid.Get())C->SetRelativeRotation(FQuat(FVector(1,0,0),FMath::DegreesToRadians(Open)));
  if(auto* C=Mill.Float.Get())C->SetRelativeRotation(FQuat(FVector(0,1,0),FMath::DegreesToRadians(.5*FMath::Sin(Mill.Age*.9)+Wind*.5*FMath::Sin(Mill.Age*2.3))));
 }
 // The walk boxes of the road's list and the truck's obstacles, there while the model is drawn.
 void Solid(FMill& Mill,US47Main* M,bool bOn)
 {
  AS47Web* W=Mill.Web.Get();if(!W||Mill.bFarm)return;
  const TArray<FBox2D> Boxes=WalkBoxes(TankX,TankZ,TankR);
  for(const TCHAR* List:{TEXT("road"),TEXT("diner")})
  {
   TArray<FS47Rect>& Rows=W->CollidersMutable(List);
   for(int32 K=0;K<Boxes.Num();++K)
   {
    const FString Id=FString::Printf(TEXT("%s%d"),BoxId,K);
    FS47Rect* Row=Rows.FindByPredicate([&Id](const FS47Rect& R){return R.Id==Id;});
    if(!Row){if(!bOn)continue;FS47Rect New;New.Id=Id;Row=&Rows[Rows.Add(New)];}
    Row->MinX=Mill.Stand.X+Boxes[K].Min.X;Row->MaxX=Mill.Stand.X+Boxes[K].Max.X;Row->MinZ=Mill.Stand.Z+Boxes[K].Min.Y;Row->MaxZ=Mill.Stand.Z+Boxes[K].Max.Y;Row->bEnabled=bOn;
   }
  }
  IS47DriveArea* Leg=M&&M->World?M->World->LegArea(TEXT("road")):nullptr;if(!Leg)return;
  if(Mill.Drive.IsEmpty())
  {
   const double H=Foot+PierHalf+.03;
   Mill.Drive.Add(FS47Obstacle::Box(Mill.Stand.X-H,Mill.Stand.X+H,Mill.Stand.Z-H,Mill.Stand.Z+H));
   Mill.Drive.Add(FS47Obstacle::Circle(Mill.Stand.X+TankX,Mill.Stand.Z+TankZ,TankR+.06));
  }
  // (others keep places in the truck's list by index: never removed, only sent far off and brought back, as S47TownSites does)
  for(const FS47Obstacle& Mine:Mill.Drive)
  {
   FS47Obstacle* Found=Leg->Obstacles.FindByPredicate([&Mine](const FS47Obstacle& O)
   {
    if(O.bCircle!=Mine.bCircle)return false;
    if(O.bCircle)return FMath::IsNearlyEqual(O.Z,Mine.Z,1e-6)&&FMath::IsNearlyEqual(O.R,Mine.R,1e-6)&&(FMath::IsNearlyEqual(O.X,Mine.X,1e-6)||FMath::IsNearlyEqual(O.X,Mine.X+Away,1e-6));
    return FMath::IsNearlyEqual(O.MinZ,Mine.MinZ,1e-6)&&FMath::IsNearlyEqual(O.MaxZ,Mine.MaxZ,1e-6)&&(FMath::IsNearlyEqual(O.MinX,Mine.MinX,1e-6)||FMath::IsNearlyEqual(O.MinX,Mine.MinX+Away,1e-6));
   });
   const double Push=bOn?0.:Away;
   if(Found){if(Found->bCircle)Found->X=Mine.X+Push;else{Found->MinX=Mine.MinX+Push;Found->MaxX=Mine.MaxX+Push;}}
   else if(bOn)Leg->Obstacles.Add(Mine);
  }
  Mill.bDriveOn=bOn;
 }
 // The land scatters its plants and stones by cells that come in as the player nears: none is left standing in the tower's foot, through
 // the housing or out of the water. Looked for every second or two while the player is near, each instance drawn in to nothing.
 void ClearPlants(FMill& Mill,double Dt,bool bNear)
 {
  AS47Web* W=Mill.Web.Get();auto* Tower=Mill.Tower.Get();auto* Tank=Mill.Tank.Get();
  if(!W||!Tower||!Tank||!bNear||(Mill.PlantsIn-=Dt)>0)return;
  Mill.PlantsIn=1.5;++Mill.PlantLooks;   // (for as long as the player is near: a cell that comes in again brings its plants back)
  FBox Foot=Tower->Bounds.GetBox();Foot.Max.Z=Foot.Min.Z+250;Foot.Min.Z-=150;
  const FBox Round=Tank->Bounds.GetBox().ExpandBy(FVector(25,25,150));
  TArray<UInstancedStaticMeshComponent*> Comps;W->GetComponents<UInstancedStaticMeshComponent>(Comps);
  int32 Gone=0;
  for(UInstancedStaticMeshComponent* C:Comps)
  {
   if(!C||!C->GetStaticMesh()||!C->IsRegistered())continue;
   bool bTouched=false;
   for(const FBox& Box:{Foot,Round})for(const int32 I:C->GetInstancesOverlappingBox(Box,true))
   {
    FTransform T;if(!C->GetInstanceTransform(I,T,false)||T.GetScale3D().GetAbsMax()<=.001)continue;
    T.SetScale3D(FVector(.0001));C->UpdateInstanceTransform(I,T,false,false,true);bTouched=true;++Gone;
   }
   if(bTouched){C->MarkRenderStateDirty();if(auto* H=Cast<UHierarchicalInstancedStaticMeshComponent>(C))H->BuildTreeIfOutdated(false,true);}
  }
  if(Gone>0&&Mill.PlantsGone==0){UE_LOG(LogS47LandWindmill,Display,TEXT("S47: land windmill: the first %d scattered plants and stones under the %s mill drawn in"),Gone,Mill.bFarm?TEXT("farm's"):TEXT("Baca"));}
  Mill.PlantsGone+=Gone;
 }
 FS47Predecessor* Trail(US47Main* M){return M&&M->Freeform?M->Freeform->Predecessor.Get():nullptr;}
 bool PageRead(US47Main* M){const FS47Predecessor* P=Trail(M);return P&&P->Found(PageDoc);}
 bool PageDue(US47Main* M){const FS47Predecessor* P=Trail(M);return P&&P->BookDue(PageDoc);}
 bool PageThere(US47Main* M){return Dev.bOn?Dev.bPage:(PageRead(M)||PageDue(M));}
 bool Wired(US47Main* M){return Dev.bOn?Dev.bWire:(PageDue(M)&&!PageRead(M));}

 bool BuildBaca(US47Main* M)
 {
  AS47Web* W=M->Web.Get();
  const int32 RoadRoot=W->Handle(TEXT("world.legs.road.area.group"));if(RoadRoot<0||!IsValid(M->Inter))return false;
  FVector World;bool bOnGround=false;
  if(!M->Freeform->Places->Stand(PlaceId,World,bOnGround))return false;
  Baca=FMill();
  if(!Build(Baca,W,TEXT("Baca"))){UE_LOG(LogS47LandWindmill,Warning,TEXT("S47: land windmill: a mesh would not load; the place keeps its old shapes"));Baca.bBuilt=true;Baca.Root.Reset();return false;}
  // each piece a node of the road's land (it comes and goes with it), pointed at as the place is; the housing and its lid are their own
  const FString Use=FS47Places::UseId(PlaceId);
  ForEach(Baca,[&](UStaticMeshComponent* C)
  {
   const bool bLid=C==Baca.Housing.Get()||C==Baca.Lid.Get(),bPage=C==Baca.Page.Get();
   const FS47Obj O=W->ObjAt(W->AdoptNode(C,RoadRoot,FString::Printf(TEXT("landplace:%s"),*C->GetName())));
   if(O&&C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision)W->MarkUsable(O.I,bPage?FString(PageUse):bLid?FString(LidUse):Use);
  });
  {
   FS47Interactable I;I.Id=LidUse;I.Range=3;
   I.Label=[]{US47Main* Run=Baca.Web.IsValid()?US47Main::Get(Baca.Web.Get()):nullptr;if(!Baca.bShown||!Run)return FString();
    return Wired(Run)?FString(TEXT("Pump housing / undo the wire")):FString(Baca.LidWant>.5?TEXT("Pump housing / shut the lid"):TEXT("Pump housing / lift the lid"));};
   I.Use=[]{US47Main* Run=Baca.Web.IsValid()?US47Main::Get(Baca.Web.Get()):nullptr;if(!Baca.bShown||!Run)return;
    if(Wired(Run)&&!Dev.bOn){Baca.LidWant=1;if(FS47Predecessor* P=Trail(Run))P->ReadDoc(PageDoc);return;}
    Baca.LidWant=Baca.LidWant>.5?0.:1.;UE_LOG(LogS47LandWindmill,Display,TEXT("S47: land windmill: the pump housing's lid %s"),Baca.LidWant>.5?TEXT("lifted"):TEXT("shut"));};
   I.Examine=[]{US47Main* Run=Baca.Web.IsValid()?US47Main::Get(Baca.Web.Get()):nullptr;if(!Baca.bShown||!Run)return FString();
    return FString(TEXT("A knee-high box of corrugated sheet and angle iron over the pump, with a hinged lid."))+(Wired(Run)?TEXT(" A twist of bright new wire holds the hasp shut."):TEXT(""));};
   M->Inter->Add(I);
  }
  {
   FS47Interactable I;I.Id=PageUse;I.Range=2.4;
   I.Label=[]{US47Main* Run=Baca.Web.IsValid()?US47Main::Get(Baca.Web.Get()):nullptr;if(!Baca.bShown||!Run||Baca.LidK<.8||!PageThere(Run))return FString();
    return FString(PageRead(Run)?TEXT("Reyes's page / read it again"):TEXT("A page in a sandwich bag / read it"));};
   I.Use=[]{US47Main* Run=Baca.Web.IsValid()?US47Main::Get(Baca.Web.Get()):nullptr;if(!Baca.bShown||!Run||Baca.LidK<.8||!PageThere(Run))return;if(FS47Predecessor* P=Trail(Run))P->ReadDoc(PageDoc);};
   I.Examine=I.Label;M->Inter->Add(I);
  }
  // Trampled bare ground round the tower and the tank, where stock stood for years: the farm yard's worn-ground map (Tools/farmworld's
  // decals) laid on the land. Without that material the land is as it was.
  {
   const TCHAR* const Worn=TEXT("/Game/FarmBuildings/Materials/M_Farm_WornField");
   UMaterialInterface* Asset=FPackageName::DoesPackageExist(Worn)?LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/FarmBuildings/Materials/M_Farm_WornField.M_Farm_WornField")):nullptr;
   if(Asset)
   {
    auto* Mid=UMaterialInstanceDynamic::Create(Asset,W);Mid->SetScalarParameterValue(TEXT("Opacity"),.55f);Mid->SetForceMipLevelsToBeResident(true,true,-1.f,0,true);
    auto* D=NewObject<UDecalComponent>(W,TEXT("S47LandMill_Baca_Ground"));
    D->SetDecalMaterial(Mid);D->DecalSize=FVector(150,760,700);D->SetFadeScreenSize(0.f);D->SortOrder=-1;
    D->SetupAttachment(Baca.Root.Get());D->RegisterComponent();
    D->SetRelativeLocationAndRotation(FVector(170,30,0),FRotator(-90,0,90));
    Baca.Ground=D;Baca.GroundMat=Mid;
   }
  }
  Baca.bReadBefore=PageRead(M);
  UE_LOG(LogS47LandWindmill,Display,TEXT("S47: land windmill: the Baca mill drawn from /Game/LandPlaces/Windmill (%d of %d meshes)"),Present(),int32(UE_ARRAY_COUNT(Names)));
  return true;
 }
 void StepBaca(US47Main* M,double Dt)
 {
  const bool bLive=M->IsFreeform()&&M->Freeform&&M->Freeform->Active()&&M->Freeform->Places&&M->Web&&M->Web->IsLoaded();
  if(Baca.bBuilt&&Baca.Web.Get()!=M->Web.Get()){if(Baca.bDriveOn)Solid(Baca,M,false);Baca=FMill();}
  if(!bLive){if(Baca.bShown){Baca.bShown=false;Solid(Baca,M,false);}return;}
  if(!Baca.bBuilt&&!BuildBaca(M))return;
  if(!Baca.Root.IsValid())return;
  auto& F=*M->Freeform;
  FVector World;bool bOnGround=false;
  if(!F.Places->Stand(PlaceId,World,bOnGround))return;
  double Az=0;bool bTraces=false;F.Places->MillHead(Az,bTraces);
  if(!Baca.bShown||!World.Equals(Baca.Stand,.005)||bOnGround!=Baca.bSeated)
  {
   Baca.Stand=World;Baca.bSeated=bOnGround;F.Places->Spot(PlaceId,Baca.PlaceX,Baca.PlaceZ);
   const FVector Road=S47Legs::ORIGINS(TEXT("road"));
   const auto Ground=[&](double Lx,double Lz)
   {
    // as FS47Places::Seat: what the player will stand on once it can be measured, else the land as it is planned
    if(bOnGround&&M->Player&&M->Player->Floor){const double Y=M->Player->Floor(World.X+Lx,World.Z+Lz);if(FMath::IsFinite(Y))return Y-World.Y;}
    return Road.Y+S47RoadTerrain::SurfaceY(Baca.PlaceX+Lx,Baca.PlaceZ+Lz)-World.Y;
   };
   if(auto* R=Baca.Root.Get())R->SetWorldLocationAndRotation(S47Three::ToUE(World),FRotator::ZeroRotator);
   Seat(Baca,Ground,FVector2D(TankX,TankZ),1,1,0);
   Baca.bShown=true;Baca.Drive.Reset();Solid(Baca,M,true);
   UE_LOG(LogS47LandWindmill,Display,TEXT("S47: land windmill: stands at road %.1f %.1f, %s; the piers' ground %.2f %.2f %.2f %.2f m about the place's, the tower on %.2f, the housing on %.2f, the tank on %.2f"),
    Baca.PlaceX,Baca.PlaceZ,bOnGround?TEXT("on the measured ground"):TEXT("on the planned ground"),Baca.PierGround[0],Baca.PierGround[1],Baca.PierGround[2],Baca.PierGround[3],Baca.TowerUp,Baca.HouseUp,Baca.TankUp);
  }
  Baca.HeadAz=Az;Baca.bTraces=bTraces;
  // wired off, its brake on: it turns only while the night's knocking goes
  Baca.Age+=Dt;Baca.Strain+=Dt;
  if(Baca.bKnocking&&(Baca.KnockFor+=Dt)>KnockEvery*1.7)Baca.bKnocking=false;
  const double Want=Baca.bKnocking?180./KnockEvery:0.;
  Baca.WheelRate+=(Want-Baca.WheelRate)*FMath::Min(1.,Dt/.6);
  if(FMath::Abs(Baca.WheelRate)<1.&&!Baca.bKnocking)Baca.WheelRate=0;
  Baca.WheelDeg=FMath::Fmod(Baca.WheelDeg+Baca.WheelRate*Dt,360.);Baca.RodDeg=FMath::Fmod(Baca.RodDeg+Baca.WheelRate*Dt,360.);
  // the lid: the game's, or the pictures'
  const bool bRead=PageRead(M),bWired=Wired(M);
  if(bRead&&!Baca.bReadBefore&&Baca.bWiredBefore)Baca.LidWant=1;   // "Undo the wire" in the place's panel
  Baca.bReadBefore=bRead;Baca.bWiredBefore=bWired;
  if(Dev.bOn)Baca.LidWant=Dev.bLid?1:0;
  else if(bWired)Baca.LidWant=0;
  Baca.LidK=FMath::Clamp(Baca.LidK+FMath::Sign(Baca.LidWant-Baca.LidK)*Dt/.9,FMath::Min(Baca.LidK,Baca.LidWant),FMath::Max(Baca.LidK,Baca.LidWant));
  if(auto* C=Baca.Page.Get()){const bool bThere=PageThere(M);C->SetHiddenInGame(!bThere);C->SetCollisionEnabled(bThere&&Baca.LidK>.5?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);}
  if(auto* C=Baca.LidWire.Get())C->SetHiddenInGame(!bWired);
  Pose(Baca,S47Weather::Live().bOn?S47Weather::Live().Wind:0.);
  Wire(Baca,bTraces);
  ClearPlants(Baca,Dt,Baca.Tower.IsValid()&&Baca.Tower->IsVisible()&&M->Player&&FVector2D(M->Player->Pos.X-World.X,M->Player->Pos.Z-World.Z).Size()<160);
  if(auto* D=Baca.Ground.Get())
  {
   // there with the tower, and toned by the hour as the farm's worn ground is (S47FarmWorld ToneFarmDecals: the land's own day and night)
   const bool bSeen=Baca.Tower.IsValid()&&Baca.Tower->IsVisible();
   if(D->IsVisible()!=bSeen)D->SetVisibility(bSeen);
   const double Hour=FMath::Fmod(F.Clock(),86400.)/3600.;
   const float Day=float(FMath::Clamp(FMath::Min((Hour-4.72)/1.25,(19.25-Hour)/1.25),0.,1.));
   if(FMath::Abs(Day-Baca.GroundDay)>.01f)
   {
    Baca.GroundDay=Day;
    const FLinearColor Field(.8f,.74f,.68f,1.f),ByDay(.781f,.785f,.765f,1.f),Night(4.17f,3.28f,2.96f,1.f);
    if(auto* Mid=Baca.GroundMat.Get())Mid->SetVectorParameterValue(TEXT("Tint"),Field*ByDay*FMath::Lerp(Night,FLinearColor::White,Day));
   }
  }
 }
 void StepFarm(US47Main* M,double Dt)
 {
  if(!Farm.bBuilt||!Farm.bShown||!Farm.Root.IsValid())return;
  const S47Weather::FLive& Live=S47Weather::Live();
  const double Mph=Live.bOn?Live.Mph:8.;
  Farm.Age+=Dt;Farm.Strain+=Dt;
  // into the wind, slowly; the wheel takes a few seconds to follow a gust
  const double Want=FMath::RadiansToDegrees(FMath::Atan2(-Live.DirX,Live.DirZ));
  const double Turn=FMath::FindDeltaAngleDegrees(Farm.HeadAz,Want);
  Farm.HeadAz+=FMath::Clamp(Turn,-5.*Dt,5.*Dt)*(Mph>3?1.:0.);
  Farm.WheelRate+=(FreeRate(Mph)-Farm.WheelRate)*FMath::Min(1.,Dt/5.);
  Farm.WheelDeg=FMath::Fmod(Farm.WheelDeg+Farm.WheelRate*Dt,360.);Farm.RodDeg=Farm.WheelDeg;
  Pose(Farm,Live.bOn?Live.Wind:0.);
  ClearPlants(Farm,Dt,M->Player&&Farm.Root.IsValid()&&FVector::Dist2D(S47Three::ToUE(M->Player->Pos),Farm.Root->GetComponentLocation())<16000);
 }
}
using namespace S47LandWindmillLocal;

bool Available(){static const bool bAll=Present()==int32(UE_ARRAY_COUNT(Names));return bAll&&!Off();}

TArray<FBox2D> WalkBoxes(double TankAtX,double TankAtZ,double TankRadius)
{
 TArray<FBox2D> B;
 for(int32 K=0;K<4;++K){const FVector2D C(CornerX[K]*Foot,CornerZ[K]*Foot);B.Add(FBox2D(C-FVector2D(PierHalf),C+FVector2D(PierHalf)));}
 B.Add(FBox2D(FVector2D(HouseX0-.02,HouseZ0-.02),FVector2D(HouseX1+.02,HouseZ1+.02)));
 // a round tank as five boxes: none of them more than a hand outside the wall
 const double R=TankRadius+.02;
 for(const double A:{15.,30.,45.,60.,75.}){const double Rad=FMath::DegreesToRadians(A),X=R*FMath::Cos(Rad),Z=R*FMath::Sin(Rad);B.Add(FBox2D(FVector2D(TankAtX-X,TankAtZ-Z),FVector2D(TankAtX+X,TankAtZ+Z)));}
 return B;
}
bool Covers(double RoadX,double RoadZ)
{
 if(!Available())return false;
 const auto Under=[RoadX,RoadZ](const FVector2D& Mill,const FVector2D& Tank,double Radius)
 {
  return (FMath::Abs(RoadX-Mill.X)<Foot+.55&&FMath::Abs(RoadZ-Mill.Y)<Foot+.55)||FVector2D::Distance(FVector2D(RoadX,RoadZ),Tank)<Radius+.45;
 };
 const FS47Places::FDef* D=FS47Places::Def(PlaceId);
 return Under(FarmMill,FarmTank,FarmTankR)||(D&&Under(FVector2D(D->X,D->Z),FVector2D(D->X+TankX,D->Z+TankZ),TankR));
}
TArray<FBox2D> Plots()
{
 TArray<FBox2D> Out;if(!Available())return Out;
 const auto Add=[&Out](const FVector2D& Mill,const FVector2D& Tank,double Radius)
 {
  Out.Add(FBox2D(Mill-FVector2D(Foot+.45),Mill+FVector2D(Foot+.45)));
  Out.Add(FBox2D(Tank-FVector2D(Radius+.35),Tank+FVector2D(Radius+.35)));
 };
 Add(FarmMill,FarmTank,FarmTankR);
 if(const FS47Places::FDef* D=FS47Places::Def(PlaceId))Add(FVector2D(D->X,D->Z),FVector2D(D->X+TankX,D->Z+TankZ),TankR);
 return Out;
}
double FreeRate(double Mph){return FMath::Clamp((Mph-4.5)*14.,0.,250.);}
double RodLift(double WheelDeg){return Stroke*(1.-FMath::Cos(FMath::DegreesToRadians(WheelDeg)))*.5;}

void Knock(bool bUp)
{
 if(!Baca.bKnocking)UE_LOG(LogS47LandWindmill,Display,TEXT("S47: land windmill: the rod goes, and the wheel with it, with no wind"));
 Baca.bKnocking=true;Baca.KnockFor=0;Baca.RodDeg=bUp?180.:0.;
}
void KnockEnd(){if(Baca.bKnocking){UE_LOG(LogS47LandWindmill,Display,TEXT("S47: land windmill: it stops; the wheel stands"));}Baca.bKnocking=false;}
void Creak(const FVector2D& RoadSpot)
{
 FMill& Mill=FVector2D::Distance(RoadSpot,FarmMill)<FVector2D::Distance(RoadSpot,FVector2D(Baca.PlaceX,Baca.PlaceZ))?Farm:Baca;
 Mill.Strain=0;
}
void DevPose(bool bOn,bool bLidOpen,bool bPage,bool bWire)
{
 if(Dev.bOn&&!bOn)Baca.LidWant=0;   // back to the game's own: shut, as it was found
 Dev.bOn=bOn;Dev.bLid=bLidOpen;Dev.bPage=bPage;Dev.bWire=bWire;
}
double LidOpen(){return Baca.LidK;}
double WheelRateNow(){return Baca.WheelRate;}

void ReconcileFarm(AS47Web* W,bool bOn)
{
 if(!W||!Available())return;
 if(Farm.Web.Get()!=W){Farm=FMill();Farm.bFarm=true;Farm.Web=W;Farm.HeadAz=225;}
 if(!Farm.bBuilt)
 {
  if(!bOn)return;
  if(!Build(Farm,W,TEXT("Farm"))){Farm.bBuilt=true;Farm.Root.Reset();return;}
  const double BaseY=S47FarmWorld::FloorRoad(FarmMill.X,FarmMill.Y);
  Farm.PlaceX=FarmMill.X;Farm.PlaceZ=FarmMill.Y;Farm.Stand=FVector(FarmMill.X,BaseY,FarmMill.Y);
  const FVector2D TankAt=FarmTank-FarmMill;
  // the tank the farm has always had (2.3 m round, its truck circle): the model's, wider and a little taller, its fill line toward the tower
  Seat(Farm,[BaseY](double Lx,double Lz){return S47FarmWorld::FloorRoad(FarmMill.X+Lx,FarmMill.Y+Lz)-BaseY;},TankAt,FarmTankR/TankR,FarmTankH/TankH,FMath::RadiansToDegrees(FMath::Atan2(-TankAt.Y,-TankAt.X)));
  Pose(Farm,0);
  UE_LOG(LogS47LandWindmill,Display,TEXT("S47: land windmill: the farm's mill drawn from /Game/LandPlaces/Windmill at road %.1f %.1f, the tower on %.2f m about its foot"),FarmMill.X,FarmMill.Y,Farm.TowerUp);
 }
 USceneComponent* R=Farm.Root.Get();if(!R)return;
 const FString Area=W->Area();
 if(Area==Farm.Area&&Farm.AppliedOn==int32(bOn))return;
 Farm.Area=Area;Farm.AppliedOn=int32(bOn);
 R->SetWorldLocationAndRotation(S47Three::ToUE(S47FarmWorld::RoadToArea(Area,Farm.Stand)),FRotator(0,Area==TEXT("station01")?90:0,0));
 Farm.bShown=bOn&&S47HighwayLandscape::Exterior(Area);
 ForEach(Farm,[](UStaticMeshComponent* C){C->SetVisibility(Farm.bShown);C->SetHiddenInGame(!Farm.bShown);});
 for(const auto& P:{Farm.Tower,Farm.Housing,Farm.Lid,Farm.Tank,Farm.Pier[0],Farm.Pier[1],Farm.Pier[2],Farm.Pier[3]})if(auto* C=P.Get())C->SetCollisionEnabled(Farm.bShown?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
}

void Update(US47Main* M,double Dt)
{
 if(!M||!M->Web||!Available())return;
 StepBaca(M,Dt);
 StepFarm(M,Dt);
}

bool Audit(US47Main* M,TArray<FString>& Lines,bool bTrace)
{
 bool bOk=true;
 const auto Say=[&](bool bGood,const FString& What){Lines.Add(FString::Printf(TEXT("%s land windmill: %s"),bGood?TEXT("PASS"):TEXT("FAIL"),*What));bOk&=bGood;};
 if(!Available()){Lines.Add(FString::Printf(TEXT("PASS land windmill: %d of %d meshes%s: the old shapes are drawn"),Present(),int32(UE_ARRAY_COUNT(Names)),Off()?TEXT(", -S47LandMillOff"):TEXT("")));return true;}
 if(!Baca.bBuilt||!Baca.Root.IsValid()||!Baca.bShown){Lines.Add(TEXT("PASS land windmill: the Baca mill is not drawn yet (the place is not built, or not in Freeform)"));return true;}
 int32 Seen=0;ForEach(Baca,[&](UStaticMeshComponent* C){Seen+=C->IsVisible()&&!C->bHiddenInGame?1:0;});
 Say(Seen>=13,FString::Printf(TEXT("the model is placed at road %.1f %.1f (%d pieces drawn, %s)"),Baca.PlaceX,Baca.PlaceZ,Seen,Baca.bSeated?TEXT("on the measured ground"):TEXT("on the planned ground")));
 double Least=1e9,Most=-1e9;
 for(int32 K=0;K<4;++K){const double Out=Baca.TowerUp+PierTop-Baca.PierGround[K];Least=FMath::Min(Least,Out);Most=FMath::Max(Most,Out);}
 Say(Least>=PierTop-.001&&Most<=PierTop+PierDeep-.1,FString::Printf(TEXT("each pier stands on its own ground: %.2f to %.2f m of it out of the ground, none floating (it goes %.2f m down), none buried"),Least,Most,PierDeep));
 Say(Baca.TankUp-TankDeep<Baca.TankLow+.001,FString::Printf(TEXT("the tank's wall reaches the ground all round (level on %.2f, the lowest ground under its rim %.2f, the wall %.2f m on down)"),Baca.TankUp,Baca.TankLow,TankDeep));
 if(bTrace&&M&&M->Web&&M->Web->GetWorld())
 {
  // the drawn ground under each piece against the ground the model was seated on
  FCollisionQueryParams Q(SCENE_QUERY_STAT(S47LandMillSeat),true);
  ForEach(Baca,[&](UStaticMeshComponent* C){Q.AddIgnoredComponent(C);});
  Lines.Add(FString::Printf(TEXT("     land windmill: the place stands at height %.3f (%s)"),Baca.Stand.Y,Baca.bSeated?TEXT("measured"):TEXT("planned")));
  struct FProbe{const TCHAR* What;double X,Z,Seated;};
  TArray<FProbe> Probes;
  static const TCHAR* const PierNames[4]={TEXT("pier NE"),TEXT("pier NW"),TEXT("pier SW"),TEXT("pier SE")};
  for(int32 K=0;K<4;++K)Probes.Add({PierNames[K],CornerX[K]*(Foot+.45),CornerZ[K]*(Foot+.45),Baca.PierGround[K]});
  Probes.Add({TEXT("housing"),0,HouseZ1+.35,Baca.HouseUp});
  for(int32 K=0;K<8;++K){const double A=K*UE_DOUBLE_PI/4;Probes.Add({TEXT("tank rim"),TankX+FMath::Cos(A)*TankR,TankZ+FMath::Sin(A)*TankR,Baca.TankGround[K]});}
  double Worst=0;FString Where;int32 Hits=0;
  for(const FProbe& P:Probes)
  {
   // the first thing from above that is drawn (the land keeps unseen sheets of its older ground: they are passed through)
   const FVector Top=S47Three::ToUE(Baca.Stand+FVector(P.X,30,P.Z)),Bottom=S47Three::ToUE(Baca.Stand+FVector(P.X,-30,P.Z));
   FCollisionQueryParams Each=Q;FHitResult Hit;bool bFound=false;
   for(int32 Try=0;Try<16&&!bFound;++Try)
   {
    if(!M->Web->GetWorld()->LineTraceSingleByChannel(Hit,Top,Bottom,ECC_Visibility,Each))break;
    UPrimitiveComponent* C=Hit.GetComponent();if(!C)break;
    if(C->IsVisible()&&!C->bHiddenInGame&&!C->GetName().StartsWith(TEXT("S47Place_")))bFound=true;else Each.AddIgnoredComponent(C);   // (nor the place's own first-day traces)
   }
   if(!bFound)continue;
   ++Hits;
   const double Drawn=Hit.ImpactPoint.Z/100.-Baca.Stand.Y,Diff=Drawn-P.Seated;
   const double Walked=M->Player&&M->Player->Floor?M->Player->Floor(Baca.Stand.X+P.X,Baca.Stand.Z+P.Z)-Baca.Stand.Y:0.;
   Lines.Add(FString::Printf(TEXT("     land windmill: %s: drawn ground %.3f (%s), walked ground %.3f, seated on %.3f (%+.3f m)"),P.What,Drawn,*Hit.GetComponent()->GetName(),Walked,P.Seated,Diff));
   if(FMath::Abs(Diff)>Worst){Worst=FMath::Abs(Diff);Where=P.What;}
  }
  Say(Hits==Probes.Num()&&Worst<=.05,FString::Printf(TEXT("the ground each piece was seated on is the drawn ground within 5 cm: the most %.3f m (%s), %d of %d probes found ground"),Worst,*Where,Hits,Probes.Num()));
 }
 return bOk;
}

bool SelfTest(TArray<FString>& Lines)
{
 bool bOk=true;
 const auto Check=[&](bool bCond,const FString& What){Lines.Add(FString::Printf(TEXT("%s land windmill: %s"),bCond?TEXT("PASS"):TEXT("FAIL"),*What));bOk&=bCond;};
 const TArray<FBox2D> Boxes=WalkBoxes(TankX,TankZ,TankR);
 constexpr double Walker=.27;
 // 1. a walker goes between the legs and all the way round the housing
 double Narrow=1e9;
 for(int32 K=0;K<4;++K)
 {
  const FBox2D& P=Boxes[K];const FBox2D& H=Boxes[4];
  Narrow=FMath::Min(Narrow,FMath::Max(FMath::Max(H.Min.X-P.Max.X,P.Min.X-H.Max.X),FMath::Max(H.Min.Y-P.Max.Y,P.Min.Y-H.Max.Y)));
 }
 const double Between=2*(Foot-PierHalf);
 Check(Narrow>=2*Walker+.04&&Between>=2*Walker+.5,FString::Printf(TEXT("a walker (radius %.2f m) goes in between the legs (%.2f m clear) and past the pump housing to every pier (%.2f m at the narrowest)"),Walker,Between,Narrow));
 double TankGap=1e9;for(int32 K=5;K<Boxes.Num();++K)TankGap=FMath::Min(TankGap,Boxes[K].Min.X-Boxes[0].Max.X);
 Check(TankGap>=2*Walker+.3,FString::Printf(TEXT("and between the tower and the tank (%.2f m)"),TankGap));
 // 2. no box beyond what is drawn: the tank's five within a hand of its wall, each pier's inside its top, the housing's on its slab
 double Over=0;for(int32 K=5;K<Boxes.Num();++K)Over=FMath::Max(Over,FVector2D::Distance(Boxes[K].Max,FVector2D(TankX,TankZ))-TankR);
 Check(Over<=.06&&PierHalf<=.2&&Boxes[4].GetSize().X<=.92&&Boxes[4].GetSize().Y<=.77,FString::Printf(TEXT("no walk box stands out past what is drawn (the tank's corners %.3f m over its wall)"),Over));
 // 3. what else stands at the place keeps its ground: Reyes's tag, where F10 sets the player down, the Sky Watch van, the truck's two ways in
 const FVector2D Tag(6,4),Stand(0,5),Van(9,6);
 bool bFree=true;double Nearest=1e9;
 for(const FBox2D& B:Boxes)for(const FVector2D& P:{Tag,Stand,Van}){const double D=FMath::Sqrt(B.ComputeSquaredDistanceToPoint(P));Nearest=FMath::Min(Nearest,D);bFree&=D>=.6;}
 Check(bFree,FString::Printf(TEXT("Reyes's tag, F10's standing point and the van's place are clear of it (the nearest %.2f m)"),Nearest));
 const FS47Places::FDef* Def=FS47Places::Def(PlaceId);
 double RouteClear=1e9;
 if(Def)for(const FVector2D& End:{FVector2D(150,720),FVector2D(160,730)})RouteClear=FMath::Min(RouteClear,FVector2D::Distance(End,FVector2D(Def->X,Def->Z))-(TankX+TankR));
 Check(Def&&RouteClear>=5+2.75,FString::Printf(TEXT("the highway wall test's two routes end %.1f m clear of the tank and the tower (the truck is 2.75 m to its nose, 5 m to arrive)"),RouteClear));
 // 4. every hook is still there
 bool bMills=false;for(const FVector2D& S:FS47Soundscape::Mills())bMills|=Def&&S.Equals(FVector2D(Def->X,Def->Z),.01);
 const S47PredecessorText::FBookStep* Step=S47PredecessorText::BookStep(PageDoc);
 bool bTag=false;for(const S47PredecessorText::FTagDef& T:S47PredecessorText::Tags())bTag|=FString(T.Place)==PlaceId&&FVector2D(T.OffX,T.OffZ).Equals(Tag,.01);
 const FS47Happenings::FPlaceDef* Night=FS47Happenings::PlaceDef(TEXT("windmill_knock"));
 Check(Def&&FString(Def->Kind)==PlaceId&&Def->X==170&&Def->Z==720&&bMills&&Step&&FString(Step->Where)==PlaceId&&Step->Find&&FString(Step->Find).Contains(TEXT("housing"))&&bTag&&Night&&FString(Night->Place)==PlaceId
  &&FS47Places::Traced(PlaceId,1,1)==(FS47Places::ArcPlace(1)==PlaceId),
  TEXT("the hooks: the place at road 170 720, its creak, Reyes's tag and the page in the pump housing, the night's knocking, the first days' traces"));
 // 5. what moves
 Check(FreeRate(0)==0&&FreeRate(4)==0&&FreeRate(12)>60&&FreeRate(12)<FreeRate(25)&&FreeRate(80)<=250&&250./60.<8,TEXT("a free wheel stands in a calm, turns faster in more wind, and never so fast that eighteen blades strobe"));
 Check(FMath::IsNearlyZero(RodLift(0))&&FMath::IsNearlyEqual(RodLift(180),Stroke)&&RodLift(90)>0&&RodLift(90)<Stroke&&FMath::IsNearlyEqual(180./KnockEvery*KnockEvery*2,360.),TEXT("the rod: at the bottom on the knock, at the top on the lighter one, one stroke to a turn of the wheel"));
 Check(FMath::IsNearlyEqual(HeadYaw(180),0.)&&FMath::IsNearlyEqual(HeadYaw(0),-180.)&&FMath::IsNearlyEqual(HeadYaw(172),-8.),TEXT("the head: the model's wheel faces south, turned to the place's bearing as the old wheel was"));
 // 6. the seat on sloping ground
 const double Slope[4]={.10,.02,-.12,.31};
 const double Up=TowerSeat(Slope);
 bool bPiers=true;for(const double G:Slope)bPiers&=Up+PierTop-G>=PierTop&&Up-PierDeep<G;
 Check(Up==.31&&bPiers,TEXT("on sloping ground the tower stands level on the highest pier's ground, every pier out of its own ground and into it"));
 Check(true,FString::Printf(TEXT("%d of %d meshes of /Game/LandPlaces/Windmill are here%s: %s"),Present(),int32(UE_ARRAY_COUNT(Names)),Off()?TEXT(" (-S47LandMillOff)"):TEXT(""),
  Available()?TEXT("the model is placed at both windmills"):TEXT("the old shapes are shown (fallback)")));
 return bOk;
}
}
