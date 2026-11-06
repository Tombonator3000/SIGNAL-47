#include "S47FarmHouse.h"
#include "S47RoadTerrain.h"
#include "S47Audio.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"

namespace
{
 // Plan v1 is drawn with the veranda at the bottom. It stands facing the yard, so plan x and y both run backwards.
 constexpr double OriginX=104,OriginZ=216,Width=12,Depth=9,VerandaDepth=2,WallHeight=2.6;
 constexpr double DoorHead=2.05,Sill=.9,WindowHead=2.1;
 struct FOpening { double From,To; bool bDoor; };
 struct FWall { const TCHAR* Name; double X0,Y0,X1,Y1; TArray<FOpening> Openings; };
 const TArray<FWall>& Walls()
 {
  static const TArray<FWall> Data={
   {TEXT("north"),0,0,12,.3,{{1.6,3.4,false},{5.5,6.5,true},{7.4,8.6,false},{10.5,11.3,false}}},
   {TEXT("south"),0,8.7,12,9,{{1.3,3.1,false},{5.5,6.5,true},{8,9.8,false}}},
   {TEXT("west"),0,.3,.3,8.7,{{5.8,7.1,false}}},
   {TEXT("east"),11.7,.3,12,8.7,{{6.2,7.5,false}}},
   {TEXT("hallW"),5.1,.3,5.25,8.7,{{1.8,2.7,true},{6.1,7,true}}},
   {TEXT("hallE"),6.75,.3,6.9,8.7,{{1.8,2.7,true},{6.1,7,true}}},
   {TEXT("midW"),.3,4.4,5.1,4.55,{}},
   {TEXT("midE"),6.9,4.4,11.7,4.55,{}},
   {TEXT("bath"),9.8,.3,9.95,4.4,{{2.6,3.4,true}}}};
  return Data;
 }
 double ToX(double PlanX){return OriginX-PlanX;}
 double ToZ(double PlanY){return OriginZ+Depth-PlanY;}
 // A rectangle in plan metres, as a centre and size in road-local metres.
 void Add(TArray<S47FarmHouse::FPiece>& Out,const FString& Name,double X0,double Y0,double X1,double Y1,double Bottom,double Top,S47FarmHouse::EMat Mat)
 {
  const double Floor=S47FarmHouse::PadY();
  Out.Add({Name,FVector((ToX(X0)+ToX(X1))*.5,Floor+(Bottom+Top)*.5,(ToZ(Y0)+ToZ(Y1))*.5),FVector(FMath::Abs(X1-X0),Top-Bottom,FMath::Abs(Y1-Y0)),Mat});
 }
}

namespace S47FarmHouse
{
 FVector Point(double PlanX,double PlanY){return FVector(ToX(PlanX),PadY(),ToZ(PlanY));}

 bool OnPad(double X,double Z,double Margin)
 {
  return X>=OriginX-Width-Margin&&X<=OriginX+Margin&&Z>=OriginZ-VerandaDepth-Margin&&Z<=OriginZ+Depth+Margin;
 }

 double PadY()
 {
  // A level floor just over the highest ground under it.
  static const double Y=[]
  {
   double High=-1e9;
   for(int32 I=0;I<=6;++I)for(int32 J=0;J<=6;++J)High=FMath::Max(High,S47RoadTerrain::SurfaceY(OriginX-Width*I/6.,OriginZ-VerandaDepth+(Depth+VerandaDepth)*J/6.));
   return High+.18;
  }();
  return Y;
 }

 TArray<FPiece> Pieces()
 {
  TArray<FPiece> Out;
  for(const FWall& W:Walls())
  {
   const bool bAlongX=W.X1-W.X0>W.Y1-W.Y0;
   const double Start=bAlongX?W.X0:W.Y0,End=bAlongX?W.X1:W.Y1;
   auto Run=[&](const FString& Name,double From,double To,double Bottom,double Top)
   {
    if(To-From<.001||Top-Bottom<.001)return;
    if(bAlongX)Add(Out,Name,From,W.Y0,To,W.Y1,Bottom,Top,EMat::Wall);else Add(Out,Name,W.X0,From,W.X1,To,Bottom,Top,EMat::Wall);
   };
   double At=Start;int32 Index=0;
   for(const FOpening& O:W.Openings)
   {
    const FString Base=FString::Printf(TEXT("farm:oldhouse:%s:%d"),W.Name,Index++);
    Run(Base+TEXT(":run"),At,O.From,0,WallHeight);
    if(!O.bDoor)Run(Base+TEXT(":sill"),O.From,O.To,0,Sill);
    Run(Base+TEXT(":head"),O.From,O.To,O.bDoor?DoorHead:WindowHead,WallHeight);
    At=O.To;
   }
   Run(FString::Printf(TEXT("farm:oldhouse:%s:end"),W.Name),At,End,0,WallHeight);
  }
  Add(Out,TEXT("farm:oldhouse:floor"),0,0,Width,Depth,-.3,0,EMat::Floor);
  Add(Out,TEXT("farm:oldhouse:ceiling"),0,0,Width,Depth,WallHeight,WallHeight+.12,EMat::Trim);
  Add(Out,TEXT("farm:oldhouse:roof"),-.4,-.4,Width+.4,Depth+.4,WallHeight+.12,WallHeight+.3,EMat::Roof);
  Add(Out,TEXT("farm:oldhouse:veranda"),0,Depth,Width,Depth+VerandaDepth,-.3,-.04,EMat::Floor);
  Add(Out,TEXT("farm:oldhouse:verandaRoof"),0,Depth,Width,Depth+VerandaDepth+.2,WallHeight-.14,WallHeight,EMat::Roof);
  for(const double X:{.3,4.,8.,11.7})Add(Out,FString::Printf(TEXT("farm:oldhouse:post%.0f"),X*10),X-.08,Depth+VerandaDepth-.16,X+.08,Depth+VerandaDepth,-.04,WallHeight-.14,EMat::Trim);
  return Out;
 }

 TArray<FBox2D> Solids()
 {
  TArray<FBox2D> Out;
  auto Box=[&](double X0,double Y0,double X1,double Y1)
  {
   const double A=ToX(X0),B=ToX(X1),C=ToZ(Y0),D=ToZ(Y1);
   Out.Add(FBox2D(FVector2D(FMath::Min(A,B),FMath::Min(C,D)),FVector2D(FMath::Max(A,B),FMath::Max(C,D))));
  };
  for(const FWall& W:Walls())
  {
   const bool bAlongX=W.X1-W.X0>W.Y1-W.Y0;
   double At=bAlongX?W.X0:W.Y0;const double End=bAlongX?W.X1:W.Y1;
   auto Run=[&](double From,double To){if(To-From<.001)return;if(bAlongX)Box(From,W.Y0,To,W.Y1);else Box(W.X0,From,W.X1,To);};
   for(const FOpening& O:W.Openings)if(O.bDoor){Run(At,O.From);At=O.To;}
   Run(At,End);
  }
  return Out;
 }

 TArray<FDoorPlan> DoorPlans()
 {
  TArray<FDoorPlan> Out;
  for(const FWall& W:Walls())
  {
   const bool bAlongX=W.X1-W.X0>W.Y1-W.Y0;int32 Index=0;
   for(const FOpening& O:W.Openings)if(O.bDoor)
   {
    const double Mid=bAlongX?(W.Y0+W.Y1)*.5:(W.X0+W.X1)*.5;
    Out.Add({FString::Printf(TEXT("%s%d"),W.Name,Index++),bAlongX?Point(O.From,Mid):Point(Mid,O.From),bAlongX?Point(O.To,Mid):Point(Mid,O.To)});
   }
  }
  return Out;
 }

 bool SelfTest(TArray<FString>& Lines)
 {
  // Straight through the hall from the front door to the back door, nothing in the way.
  bool bHall=true;const auto Solid=Solids();
  for(double Y=-1;Y<=10;Y+=.05)
  {
   const FVector P=Point(6,Y);
   for(const FBox2D& B:Solid)if(B.IsInside(FVector2D(P.X,P.Z)))bHall=false;
  }
  // Each room is reached from the hall through its door.
  bool bDoors=true;
  for(const double Y:{2.25,6.55})for(const double X:{5.17,6.82})
  {
   const FVector P=Point(X,Y);
   for(const FBox2D& B:Solid)if(B.IsInside(FVector2D(P.X,P.Z)))bDoors=false;
  }
  // Walls that are not doors stay solid, windows included.
  const auto Blocked=[&](double X,double Y){const FVector P=Point(X,Y);for(const FBox2D& B:Solid)if(B.IsInside(FVector2D(P.X,P.Z)))return true;return false;};
  const bool bClosed=Blocked(2.5,.15)&&Blocked(5.17,4)&&Blocked(.15,6.5);
  const bool bGood=bHall&&bDoors&&bClosed&&OnPad(Point(6,4).X,Point(6,4).Z)&&Pieces().Num()>30&&DoorPlans().Num()==7;
  Lines.Add(FString(bGood?TEXT("PASS "):TEXT("FAIL "))+TEXT("farmhouse plan: hall runs clear front to back, rooms open off it, walls and windows are solid"));
  return bGood;
 }
}

// ---------- the doors ----------
namespace
{
 const FVector SaroFrame(-24,0,300); // The house stands in the farm's part of the SARO area.
 constexpr double SwingDegrees=100,SwingSeconds=.55,LeafHeight=2.02,LeafThick=.05;
 const TCHAR* const Walk=TEXT("saro");
 FString RectId(const FString& Name){return TEXT("door:oldhouse:")+Name;}
}

int32 FS47FarmHouseDoors::IndexOf(const FString& Name) const{return Doors.IndexOfByPredicate([&](const FDoor& D){return D.Name==Name;});}

bool FS47FarmHouseDoors::Build()
{
 if(bBuilt)return true;
 if(!M||!IsValid(M->Web))return false;
 const int32 Saro=M->Web->Handle(TEXT("ext.group"));if(Saro<0)return false;
 auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Wood=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/TownBuildings/Materials/M_Town_DarkWood.M_Town_DarkWood"));if(!Cube)return false;
 for(const auto& Plan:S47FarmHouse::DoorPlans())
 {
  FDoor D;D.Name=Plan.Name;D.Id=TEXT("ff:olddoor:")+Plan.Name;
  D.Hinge=Plan.From+SaroFrame;const FVector Span=Plan.To-Plan.From;D.Width=Span.Size();D.Along=Span.GetSafeNormal();
  auto* C=NewObject<UStaticMeshComponent>(M->Web,*(TEXT("S47OldDoor")+Plan.Name));C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(M->Web->GetRootComponent());C->SetStaticMesh(Cube);if(Wood)C->SetMaterial(0,Wood);
  C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);C->RegisterComponent();
  D.Leaf=C;D.Object=M->Web->ObjAt(M->Web->AdoptNode(C,Saro,D.Id));
  C->SetWorldScale3D(FVector(D.Width,LeafThick,LeafHeight));
  const FVector End=D.Hinge+Span;
  const double Half=.09;
  D.Closed=FBox2D(FVector2D(FMath::Min(D.Hinge.X,End.X)-Half,FMath::Min(D.Hinge.Z,End.Z)-Half),FVector2D(FMath::Max(D.Hinge.X,End.X)+Half,FMath::Max(D.Hinge.Z,End.Z)+Half));
  FS47Rect R;R.MinX=R.MaxX=R.MinZ=R.MaxZ=1e9;R.Id=RectId(D.Name);M->Web->CollidersMutable(Walk).Add(R);
  Doors.Add(D);
 }
 bBuilt=true;return true;
}

void FS47FarmHouseDoors::SetCollider(const FDoor& D)
{
 // Nothing to tidy while the engine or the world is going away; the owner may already be gone.
 if(IsEngineExitRequested()||IsGarbageCollecting()||!Scene.IsValid())return;
 for(FS47Rect& R:Scene->CollidersMutable(Walk))if(R.Id==RectId(D.Name))
 {
  if(D.bOpen||!bActive)R.MinX=R.MaxX=R.MinZ=R.MaxZ=1e9;
  else{R.MinX=D.Closed.Min.X;R.MaxX=D.Closed.Max.X;R.MinZ=D.Closed.Min.Y;R.MaxZ=D.Closed.Max.Y;}
 }
}

void FS47FarmHouseDoors::Pose(FDoor& D)
{
 auto* C=D.Leaf.Get();if(!C)return;
 const double A=FMath::DegreesToRadians(SwingDegrees)*D.K;
 const FVector Dir(D.Along.X*FMath::Cos(A)-D.Along.Z*FMath::Sin(A),0,D.Along.X*FMath::Sin(A)+D.Along.Z*FMath::Cos(A));
 C->SetWorldLocationAndRotation(S47Three::ToUE(D.Hinge+Dir*(D.Width*.5)+FVector(0,LeafHeight*.5,0)),S47Three::DirToUE(Dir).Rotation());
}

void FS47FarmHouseDoors::Begin(const S47J::FObj& Saved)
{
 Stop();if(!M||!IsValid(M->Web))return;
 if(Scene.Get()!=M->Web){bBuilt=false;Doors.Reset();}
 Scene=M->Web;Life=MakeShared<uint8>(0);bActive=true;
 if(!Build())return;
 const TArray<FString> Open=Saved.IsValid()?S47J::Strs(Saved,TEXT("open")):TArray<FString>();
 const TWeakPtr<uint8> Guard=Life;
 for(int32 I=0;I<Doors.Num();++I)
 {
  FDoor& D=Doors[I];
  // Left as they were: an empty house's inner doors stand open, its outer doors are shut.
  D.bOpen=Saved.IsValid()&&Saved->HasField(TEXT("open"))?Open.Contains(D.Name):!D.Name.StartsWith(TEXT("north"))&&!D.Name.StartsWith(TEXT("south"));
  D.K=D.bOpen?1:0;
  if(auto* C=D.Leaf.Get()){C->SetVisibility(true);C->SetHiddenInGame(false);}
  Pose(D);SetCollider(D);
  if(IsValid(M->Inter))
  {
   FS47Interactable It;It.Id=D.Id;It.Object=D.Object;It.Range=2.6;
   It.Label=[this,Guard,I]{return Guard.IsValid()&&Doors.IsValidIndex(I)?FString(Doors[I].bOpen?TEXT("Close the door"):TEXT("Open the door")):FString();};
   It.Use=[this,Guard,I]{if(Guard.IsValid())Use(I);};
   It.Examine=It.Label;
   M->Inter->Add(It);M->Web->MarkUsable(D.Object.I,D.Id);
  }
 }
}

void FS47FarmHouseDoors::Stop()
{
 bActive=false;Life.Reset();
 for(FDoor& D:Doors){SetCollider(D);if(auto* C=D.Leaf.Get()){C->SetVisibility(false);C->SetHiddenInGame(true);}}
}

S47J::FObj FS47FarmHouseDoors::Save() const
{
 const auto O=S47J::New();TArray<FString> Open;for(const FDoor& D:Doors)if(D.bOpen)Open.Add(D.Name);
 S47J::Set(O,TEXT("open"),Open);return O;
}

bool FS47FarmHouseDoors::PlayerIn(const FDoor& D) const
{
 if(!M||!M->Player)return false;const double R=M->Player->Radius;
 return M->Player->Pos.X+R>D.Closed.Min.X&&M->Player->Pos.X-R<D.Closed.Max.X&&M->Player->Pos.Z+R>D.Closed.Min.Y&&M->Player->Pos.Z-R<D.Closed.Max.Y;
}

bool FS47FarmHouseDoors::Use(int32 Index)
{
 if(!bActive||!Doors.IsValidIndex(Index))return false;
 FDoor& D=Doors[Index];
 if(D.bOpen&&PlayerIn(D)){if(M&&M->UI)M->UI->Toast(TEXT("You are in the doorway."),2);return false;}
 D.bOpen=!D.bOpen;SetCollider(D);
 if(M&&M->Audio){FS47Sound S;S.At=D.Hinge+FVector(0,1,0);S.Gain=.7f;S.Rate=FMath::FRandRange(.85f,1.05f);M->Audio->Sfx(D.bOpen?TEXT("doorCreak0"):TEXT("doorClose"),S);}
 if(M)M->RequestAutosave();
 return true;
}

void FS47FarmHouseDoors::Step(double Dt)
{
 if(!bActive)return;
 for(FDoor& D:Doors)
 {
  const double Want=D.bOpen?1:0;if(D.K==Want)continue;
  D.K=FMath::Clamp(D.K+(Want>D.K?1:-1)*Dt/SwingSeconds,FMath::Min(D.K,Want),FMath::Max(D.K,Want));
  Pose(D);
 }
}
