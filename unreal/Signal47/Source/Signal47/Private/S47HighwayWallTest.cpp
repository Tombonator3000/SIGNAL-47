#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47Flow.h"
#include "S47OpenLand.h"
#include "S47Relief.h"
#include "S47RoadTerrain.h"
#include "S47Startup.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47World.h"
#include "S47Blockers.h"
#include "S47FarmWorld.h"
#include "S47Three.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

// HWY-WALL-20261113, -S47HighwayWallTest (Tom, Freeform d67/d68: off the highway toward the Baca windmill, road x 170 z 720, the truck
// stopped against nothing). Two parts. (1) What holds the truck or the walker anywhere on the highway's two old fence lines (road x
// -21 and 21, z -1005..2905, every 2 m) and across them toward the places east and west: each drive obstacle of the legs (road, saro)
// and each enabled walk collider within 2 m, the area's Blocked, and the walker's own answer (WhyNotLand); hills and the rim are told
// apart. (2) A real drive: the truck put on the road's east shoulder at z 720 facing east, full throttle, steer held to the line, and
// how far east it gets in 40 s. Fails on any blocker on a fence line that is not a hill or the rim, or a drive that does not get past
// road x 150.
void FS47FreeformTest::StepHighwayWall(double Dt)
{
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>420){Finish(false,FString::Printf(TEXT("highway wall test exceeded the time bound at stage %d"),Stage));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->World)return;
 US47World& W=*M->World;
 if(Stage==0)
 {
  if(Age<4)return;M->UI->Close(true);
  if(!Check(S47OpenLand::Live(),TEXT("Freeform's open land is live")))return;
  // (1) the legs' obstacles and the walk's colliders near the lines
  struct FHit{FString Where;int32 N=0;double Z0=1e9,Z1=-1e9;};
  TMap<FString,FHit> Hits;
  auto Note=[&](const FString& What,double RoadZ){FHit& H=Hits.FindOrAdd(What);H.N++;H.Z0=FMath::Min(H.Z0,RoadZ);H.Z1=FMath::Max(H.Z1,RoadZ);};
  auto Probe=[&](double Rx,double Rz,double Near,const TCHAR* Line)
  {
   for(const TCHAR* LegC:{TEXT("road"),TEXT("saro")})
   {
    const FString Leg=LegC;
    // only where that leg is the one a truck or walker there is in
    const FString Owner=S47OpenLand::LegFor(TEXT("road"),Rx,Rz);
    if(Owner!=Leg&&!(Leg==TEXT("saro")&&S47OpenLand::InSaroCore(Rx,Rz,S47OpenLand::HandOverM)))continue;
    const FVector P=S47OpenLand::FromRoad(Leg,Rx,Rz);
    if(IS47DriveArea* A=W.LegArea(Leg))
    {
     for(int32 I=0;I<A->Obstacles.Num();++I)
     {
      const FS47Obstacle& O=A->Obstacles[I];
      const bool bNear=O.bCircle?FVector2D(P.X-O.X,P.Z-O.Z).Size()<O.R+Near:P.X>O.MinX-Near&&P.X<O.MaxX+Near&&P.Z>O.MinZ-Near&&P.Z<O.MaxZ+Near;
      if(!bNear)continue;
      double Ox=0,Oz=0;S47OpenLand::RoadOf(Leg,O.bCircle?O.X:O.MinX,O.bCircle?O.Z:O.MinZ,Ox,Oz);
      Note(O.bCircle?FString::Printf(TEXT("%s: drive %s obstacle %d circle r %.2f at road %.1f %.1f"),Line,*Leg,I,O.R,Ox,Oz)
       :FString::Printf(TEXT("%s: drive %s obstacle %d box %.2f wide %.2f deep from road %.1f %.1f"),Line,*Leg,I,O.MaxX-O.MinX,O.MaxZ-O.MinZ,Ox,Oz),Rz);
     }
     if(A->Blocked(P.X,P.Z))Note(FString::Printf(TEXT("%s: drive %s Blocked%s"),Line,*Leg,S47Relief::HillStops(Rx,Rz,6)?TEXT(" (a hill)"):!S47OpenLand::InRim(Rx,Rz,6)?TEXT(" (the rim)"):TEXT("")),Rz);
    }
    const FString Walk=Leg==TEXT("road")?TEXT("road"):TEXT("saro");
    for(const FS47Rect& C:M->Web->CollidersMutable(Walk))
    {
     if(!C.bEnabled||C.MinX>=1e8||C.Id==TEXT("truck")||C.Id==TEXT("personalCar"))continue;
     if(P.X>C.MinX-Near&&P.X<C.MaxX+Near&&P.Z>C.MinZ-Near&&P.Z<C.MaxZ+Near)
     {
      double Ox=0,Oz=0;S47OpenLand::RoadOf(Leg,C.MinX,C.MinZ,Ox,Oz);
      Note(FString::Printf(TEXT("%s: walk %s collider %s %.2f wide %.2f deep from road %.1f %.1f"),Line,*Walk,*C.Id,C.MaxX-C.MinX,C.MaxZ-C.MinZ,Ox,Oz),Rz);
     }
    }
   }
  };
  const double F=S47RoadTerrain::HWY.Fence;
  for(double Z=-1005;Z<=2905;Z+=2){Probe(F,Z,2,TEXT("east line"));Probe(-F,Z,2,TEXT("west line"));}
  // across them, toward the places on either side (the windmill, the farm, SARO's yard, the motel, the town, the pasture)
  for(double Z:{-800.,-500.,-300.,-120.,0.,150.,240.,400.,600.,720.,900.,1200.,1500.,1800.,2066.,2300.,2600.,2850.})
   for(double X=5;X<=60;X+=1){Probe(X,Z,1.2,TEXT("crossing east"));Probe(-X,Z,1.2,TEXT("crossing west"));}
  for(double X=5;X<=175;X+=1)Probe(X,720,1.6,TEXT("windmill line"));
  int32 Bad=0;
  for(const auto& P:Hits)
  {
   const bool bOk=P.Key.Contains(TEXT("(a hill)"))||P.Key.Contains(TEXT("(the rim)"));
   Bad+=bOk?0:1;
   UE_LOG(LogTemp,Display,TEXT("S47 hwy wall: %s %s: %d samples, road z %.0f..%.0f"),bOk?TEXT("ok"):TEXT("BLOCKER"),*P.Key,P.Value.N,P.Value.Z0,P.Value.Z1);
  }
  // (most are what stands there: SARO, the motel, the town's buildings on the lines; read the list)
  UE_LOG(LogTemp,Display,TEXT("S47 hwy wall: %d kinds of blocker near the lines"),Bad);
  // (1c) the truck's footprint swept off the highway on fans of lines (each side, every 100 m, straight out and 200 m either way),
  // the leg handed over as a drive hands it over: the first thing each line meets, with what it is
  {
   int32 Lines=0,Clear=0;
   for(double Z0=-900;Z0<=2800;Z0+=100)for(const double Side:{1.,-1.})for(const double Dz:{-200.,0.,200.})
   {
    ++Lines;
    const double X0=8*Side,Z00=Z0,X1=170*Side,Z1=Z0+Dz,Len=FVector2D(X1-X0,Z1-Z00).Size();
    const double Head=FMath::Atan2(-(X1-X0),-(Z1-Z00));
    FString Leg=S47OpenLand::LegFor(TEXT("road"),X0,Z00);FString Hit;
    for(double T=0;T<=Len&&Hit.IsEmpty();T+=.5)
    {
     const double Rx=X0+(X1-X0)*T/Len,Rz=Z00+(Z1-Z00)*T/Len;
     Leg=S47OpenLand::LegFor(Leg,Rx,Rz);
     IS47DriveArea* A=W.LegArea(Leg);if(!A)break;
     const FVector P=S47OpenLand::FromRoad(Leg,Rx,Rz);
     const double Hd=S47OpenLand::Turned(TEXT("road"),Leg,Head);
     if(A->Blocked(P.X,P.Z)){Hit=FString::Printf(TEXT("Blocked%s"),S47Relief::HillStops(Rx,Rz,3.2)?TEXT(" (a hill)"):!S47OpenLand::InRim(Rx,Rz,3.2)?TEXT(" (the rim)"):TEXT(" (NOT a hill)"));}
     else for(int32 I=0;I<A->Obstacles.Num()&&Hit.IsEmpty();++I)
     {
      TArray<FS47Obstacle> One;One.Add(A->Obstacles[I]);
      if(S47FarmWorld::TruckHits(One,P.X,P.Z,Hd,1.05,2.75,.05))
      {
       const FS47Obstacle& O=A->Obstacles[I];double Ox=0,Oz=0,Ox1=0,Oz1=0;
       S47OpenLand::RoadOf(Leg,O.bCircle?O.X:O.MinX,O.bCircle?O.Z:O.MinZ,Ox,Oz);S47OpenLand::RoadOf(Leg,O.bCircle?O.X:O.MaxX,O.bCircle?O.Z:O.MaxZ,Ox1,Oz1);
       Hit=O.bCircle?FString::Printf(TEXT("%s obstacle %d circle r %.2f at road %.1f %.1f"),*Leg,I,O.R,Ox,Oz):FString::Printf(TEXT("%s obstacle %d box road x %.1f..%.1f z %.1f..%.1f"),*Leg,I,Ox,Ox1,Oz,Oz1);
      }
     }
     if(!Hit.IsEmpty())UE_LOG(LogTemp,Display,TEXT("S47 hwy wall sweep: from road %.0f %.0f toward %.0f %.0f stopped at %.1f %.1f (%.0f m out) by %s"),X0,Z00,X1,Z1,Rx,Rz,T,*Hit);
    }
    Clear+=Hit.IsEmpty()?1:0;
   }
   UE_LOG(LogTemp,Display,TEXT("S47 hwy wall sweep: %d of %d lines off the highway clear to x 170"),Clear,Lines);
  }
  // the walker on the windmill line, on foot, as WhyNotLand has it, in the area it would be in
  Go(10,.2);return;
 }

 // (1b) what of it has nothing drawn by it (S47Blockers, as F10's Looks list): the player stood along the highway, every 300 m
 static const double Stops[]={-900,-600,-300,0,200,450,700,1000,1300,1600,1900,2200,2500,2800};
 if(Stage>=10&&Stage<10+2*int32(UE_ARRAY_COUNT(Stops)))
 {
  const int32 I=(Stage-10)/2;const double Z=Stops[I];
  if((Stage-10)%2==0)
  {
   const FString Area=S47OpenLand::LegFor(TEXT("road"),0,Z);
   const FVector Spot=S47OpenLand::FromRoad(Area,0,Z);
   if(W.bDriving)W.StopDriving();
   if(W.Area!=Area){W.PlaceTruck(Area);W.Enter(Area);}
   M->Player->Place(Spot.X,Spot.Z,0);M->Player->SettleOnFloor();
   Go(Stage+1,5.);return;
  }
  TArray<FString> Lines;
  const TArray<S47Blockers::FBlocker> Bare=S47Blockers::Run(M,.3,Lines);
  UE_LOG(LogTemp,Display,TEXT("S47 hwy wall unseen at road z %.0f (area %s): %s"),Z,*W.Area,Lines.Num()?*Lines[0]:TEXT(""));
  const FString Walk=W.Area==TEXT("diner")?FString(TEXT("road")):W.Area;
  for(const S47Blockers::FBlocker& B:Bare)
  {
   double X0=0,Z0=0,X1=0,Z1=0;S47OpenLand::RoadOf(Walk,B.MinX,B.MinZ,X0,Z0);S47OpenLand::RoadOf(Walk,B.MaxX,B.MaxZ,X1,Z1);
   const double F=S47RoadTerrain::HWY.Fence;
   const bool bLine=(X0<F+3&&X1>F-3)||(X0<-F+3&&X1>-F-3);
   UE_LOG(LogTemp,Display,TEXT("S47 hwy wall unseen%s: %s %s road x %.2f..%.2f z %.2f..%.2f"),bLine?TEXT(" ON A FENCE LINE"):TEXT(""),*B.Kind,*B.Id,X0,X1,Z0,Z1);
  }
  Go(Stage+1==10+2*int32(UE_ARRAY_COUNT(Stops))?5:Stage+1,.2);return;
 }
 // (1d) in the road's area: each round obstacle of the road's leg within 60 m of the highway, and what is drawn on it, how high over
 // the ground the drive stands on (a rock sunk under Freeform's ground, or one taken away, is a post nobody sees)
 if(Stage==5)
 {
  IS47DriveArea* A=W.LegArea(TEXT("road"));
  if(!A||(W.Area!=TEXT("road")&&W.Area!=TEXT("diner"))){UE_LOG(LogTemp,Display,TEXT("S47 hwy wall rocks: not in the road's area (%s)"),*W.Area);Go(1,.2);return;}
  struct FTop{FVector2D At;double Top;FString Name;};
  TArray<FTop> Tops;
  for(TActorIterator<AActor> It(M->Web->GetWorld());It;++It)
  {
   TArray<UStaticMeshComponent*> Comps;It->GetComponents<UStaticMeshComponent>(Comps);
   for(UStaticMeshComponent* C:Comps)
   {
    UStaticMesh* Mesh=C?C->GetStaticMesh():nullptr;
    if(!Mesh||!C->IsRegistered()||!C->IsVisible()||C->bHiddenInGame)continue;
    const FBox Local=Mesh->GetBoundingBox();
    auto Add=[&](const FTransform& T)
    {
     const FBox U=Local.TransformBy(T);FBox Th(ForceInit);Th+=S47Three::ToThree(U.Min);Th+=S47Three::ToThree(U.Max);
     if(Th.GetSize().X>6||Th.GetSize().Z>6)return;
     const double Rx=Th.GetCenter().X-8000;if(FMath::Abs(Rx)>70)return;
     Tops.Add({FVector2D(Th.GetCenter().X,Th.GetCenter().Z),Th.Max.Y,Mesh->GetName()+TEXT(" / ")+C->GetName()});
    };
    if(auto* I=Cast<UInstancedStaticMeshComponent>(C)){for(int32 K=0;K<I->GetInstanceCount();++K){FTransform T;if(I->GetInstanceTransform(K,T,true)&&T.GetScale3D().GetMin()>.01)Add(T);}}
    else Add(C->GetComponentTransform());
   }
  }
  int32 Bare=0,Sunk=0,Fine=0;
  for(int32 I=0;I<A->Obstacles.Num();++I)
  {
   const FS47Obstacle& O=A->Obstacles[I];
   if(!O.bCircle||FMath::Abs(O.X-8000)>60)continue;
   const double G=A->Height(O.X,O.Z);
   double Best=-1e9;FString What;
   for(const FTop& T:Tops)if(FVector2D::Distance(T.At,FVector2D(O.X,O.Z))<O.R+.35&&T.Top>Best){Best=T.Top;What=T.Name;}
   const bool bNone=What.IsEmpty(),bSunk=!bNone&&Best-G<.15;
   if(I==42||I==40||I==101)for(const FTop& T:Tops)if(FVector2D::Distance(T.At,FVector2D(O.X,O.Z))<O.R+1.5)UE_LOG(LogTemp,Display,TEXT("S47 hwy wall rocks: by obstacle %d: %s centre %.2f %.2f top %.2f over ground"),I,*T.Name,T.At.X-8000,T.At.Y,T.Top-G);
   (bNone?Bare:bSunk?Sunk:Fine)++;
   if(bNone||bSunk)UE_LOG(LogTemp,Display,TEXT("S47 hwy wall rocks: road obstacle %d r %.2f at road %.1f %.1f: %s"),I,O.R,O.X-8000,O.Z,bNone?TEXT("NOTHING drawn on it"):*FString::Printf(TEXT("%s, its top %.2f m over the drive's ground (SUNK)"),*What,Best-G));
  }
  UE_LOG(LogTemp,Display,TEXT("S47 hwy wall rocks: %d round obstacles within 60 m of the highway: %d drawn, %d sunk, %d with nothing"),Bare+Sunk+Fine,Fine,Sunk,Bare);
  // a look at the one Tom met (road 28.2 858.8), from 5 m off, at eye height
  M->Player->Place(8000+23.5,862.5,0);M->Player->SettleOnFloor();
  {const double Dx=28.2-23.5,Dz=858.8-862.5;M->Player->Yaw=FMath::Atan2(-Dx,-Dz);M->Player->Pitch=-.3;}
  Go(6,4.);return;
 }
 if(Stage==6){FString Dir;if(!FParse::Value(FCommandLine::Get(),TEXT("S47ShotDir="),Dir))Dir=Root;FScreenshotRequest::RequestScreenshot(Dir/TEXT("hwy-obstacle42.png"),false,false);Go(1,1.);return;}
 // (2) real drives, the truck steered through waypoints: Tom's own two runs off the highway (d68's log, 13 November 13:53 and 13:54:
 // reversing at road 26 1145 and at 22 866 by the east line) and the straight line to the windmill. Road-local; heading 0 is north.
 struct FRoute{const TCHAR* Name;double X,Z,Head;TArray<FVector2D> Way;};
 static const TArray<FRoute> Routes={
  {TEXT("Tom 13:54, south then east at z 866 to the windmill"),0,975,UE_DOUBLE_PI,{{23.3,864.4},{61.2,837.6},{112,791},{160,730}}},
  {TEXT("Tom 13:53, south then off east at z 1130"),1,1100,UE_DOUBLE_PI,{{8,1130},{26.2,1145.3},{60,1170}}},
  {TEXT("straight east at z 720 to the windmill"),12,720,-UE_DOUBLE_HALF_PI,{{150,720}}},
  {TEXT("west across the line at z 866"),-2,880,UE_DOUBLE_PI,{{-23,866},{-70,850}}},
 };
 static bool bHeldAny=false;static int32 Route=0,WayI=0;static double BestD=1e9,BestAt=0,NextLog=0;
 if(Stage==1)
 {
  if(Route>=Routes.Num()){Finish(!bHeldAny&&Checks>0&&!Evidence.ContainsByPredicate([](const TSharedPtr<FJsonValue>& V){return !V->AsObject()->GetBoolField(TEXT("pass"));}),TEXT("highway wall test done"));return;}
  const FRoute& R=Routes[Route];
  if(W.bDriving)W.StopDriving();
  FS47TripOpts O;O.AtPos=FVector(8000+R.X,S47RoadTerrain::SurfaceY(R.X,R.Z),R.Z);O.AtHeading=R.Head;O.Leg=TEXT("road");
  W.StartTrip(TEXT("road"),FString(),O);
  WayI=0;BestD=1e9;BestAt=Age;NextLog=0;
  W.TestInput=[this]
  {
   FS47DriveIn I;auto* D=M&&M->World?M->World->CabDrive():nullptr;if(!D||Route>=Routes.Num())return I;
   const FVector2D T=Routes[Route].Way[FMath::Min(WayI,Routes[Route].Way.Num()-1)];
   const double Dx=T.X+8000-D->Pos.X,Dz=T.Y-D->Pos.Z;
   const double Want=FMath::Atan2(-Dx,-Dz);
   I.Throttle=D->Speed<10?1:0;
   I.Steer=FMath::Clamp(FMath::UnwindRadians(D->Heading-Want)*2.,-1.,1.);
   return I;
  };
  UE_LOG(LogTemp,Display,TEXT("S47 hwy wall route %d: %s"),Route,R.Name);
  Go(2,1.);return;
 }
 if(Stage==2)
 {
  auto* D=W.CabDrive();
  if(!D){if(Age-At>15){Finish(false,TEXT("the truck was not driven"));}return;}
  const FRoute& R=Routes[Route];
  const double Rx=D->Pos.X-8000,Rz=D->Pos.Z;
  const FVector2D T=R.Way[WayI];const double Dist=FVector2D(T.X-Rx,T.Y-Rz).Size();
  if(Age>=NextLog){NextLog=Age+1;UE_LOG(LogTemp,Display,TEXT("S47 hwy wall drive %d: road %.1f %.1f speed %.1f heading %.2f leg %s, %.1f m to waypoint %d"),Route,Rx,Rz,D->Speed,D->Heading,*W.Area,Dist,WayI);}
  if(Dist<BestD-1){BestD=Dist;BestAt=Age;}
  if(Dist<5)
  {
   if(++WayI>=R.Way.Num()){W.TestInput=nullptr;Check(true,FString::Printf(TEXT("route %d (%s): the truck got through to road %.1f %.1f, nothing held it"),Route,R.Name,Rx,Rz));++Route;Go(1,.5);return;}
   BestD=1e9;BestAt=Age;return;
  }
  if(Age-BestAt>10){W.TestInput=nullptr;bHeldAny=true;UE_LOG(LogTemp,Display,TEXT("S47 Freeform FAIL stage2: route %d (%s): held at road %.1f %.1f, %.1f m short of waypoint %d"),Route,R.Name,Rx,Rz,Dist,WayI);++Route;Go(1,.5);return;}
 }
}
