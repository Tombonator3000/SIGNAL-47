#include "S47FarmWorldTest.h"
#include "S47FarmWorld.h"
#include "S47FarmBuildings.h"
#include "S47Truck.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Inter.h"
#include "S47Story.h"
#include "S47Startup.h"
#include "S47TownLayer.h"
#include "S47Herd.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47UI.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"

namespace
{
 struct FRun
 {
  bool Started=false,Done=false,PathDone=false,Reverse=false,HaveLast=false,CargoProbe=false,TruckInWall=false;
  int32 Stage=0,Checks=0,Cursor=0,Restores=0,Bumps=0;
  double Began=0,At=0,LastAt=0,ProgressAt=0,MaxSpeed=3,DriveMetres=0,WalkMetres=0;
  FVector Last=FVector::ZeroVector,Progress=FVector::ZeroVector,Park=FVector::ZeroVector,SavedFoot=FVector::ZeroVector,EgressRoad=FVector::ZeroVector;
  FString Root,PendingPhoto;
  double PhotoAt=0;
  TArray<FVector> Path;
  S47J::FObj RetainedFreeform;
  TArray<TSharedPtr<FJsonValue>> Evidence,Trace;
  TArray<FString> Photos;
 } S;
 FVector Road(const FVector& P,const FString& Area)
 {return Area==TEXT("saro")?P+FVector(24,0,-300):P-FVector(8000,0,0);}
 FVector Here(US47Main* M,const FVector& P){return S47FarmWorld::RoadToArea(M->World->Area,P);}
 S47J::FObj Vec(const FVector& P)
 {auto O=S47J::New();S47J::Set(O,TEXT("x"),P.X);S47J::Set(O,TEXT("y"),P.Y);S47J::Set(O,TEXT("z"),P.Z);return O;}
 void Finish(US47Main* M,bool Good,const FString& Why)
 {
  if(S.Done)return;S.Done=true;
  if(M&&M->Player)M->Player->TestMove.Reset();
  if(M&&M->World){M->World->TestInput=nullptr;M->World->Autopilot.Reset();}
  if(Good)for(const FString& N:S.Photos)
  {TArray<uint8> B;if(!FFileHelper::LoadFileToArray(B,*(S.Root/(N+TEXT(".png"))))||B.Num()<1024||B[0]!=137||B[1]!=80||B[2]!=78||B[3]!=71)Good=false;}
  auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("checks"),S.Checks);S47J::Set(O,TEXT("stage"),S.Stage);
  S47J::Set(O,TEXT("seconds"),FPlatformTime::Seconds()-S.Began);S47J::Set(O,TEXT("driveMetres"),S.DriveMetres);S47J::Set(O,TEXT("walkMetres"),S.WalkMetres);S47J::Set(O,TEXT("nativeBumps"),S.Bumps);S47J::Set(O,TEXT("diskFlowRestores"),S.Restores);S47J::Set(O,TEXT("assetsOnly"),FParse::Param(FCommandLine::Get(),TEXT("S47FarmWorldAssetsOnly")));
  S47J::Set(O,TEXT("method"),TEXT("One explicit initial parked service-Jeep pose on the existing main road at road-local (1.8,230), plus a serialized loaded relay parcel fixture. Ordinary Roam/Drive TestInput, GetOutHere, Walker TestMove, actual cargo eye-ray Pick/OnUse, disk/full Flow Continue. Later Story/Freeform comparisons are separate declared camera fixtures outside the measured physical journey."));
  S47J::Set(O,TEXT("limits"),TEXT("Assets-only opt-in is a distinct short render/retention smoke and skips every route/cargo/Continue/mode-switch assertion. Isolated developer case0 only; purchase/source and initial cargo loading excluded. API controls are not human input. No teleport recovery inside the measured route. Story comparison uses a fresh genuine nightService Story start, followed by the retained Freeform state; no conversion of a Freeform save into Story. PNG existence is required; visual acceptance requires review. Bound 360 seconds; no whole-game regression."));
  S47J::Set(O,TEXT("cargoFixtureOnly"),S.CargoProbe);
  if(FParse::Param(FCommandLine::Get(),TEXT("S47FarmWorldAssetsOnly")))S47J::Set(O,TEXT("method"),TEXT("Fresh isolated Freeform resource/render fixture; native mesh/cow/collision/floor/road-clearance audits and a staged camera capture. Optional S47FarmClearanceCycle starts a genuine Story case, verifies original rock matrices, then restores Freeform and checks the clear road again. No drive/walk journey or cargo claim."));
  if(S.CargoProbe)S47J::Set(O,TEXT("method"),TEXT("Explicit isolated cargo diagnostic fixture: load a previous temporary farm save, replace only its player pose with an approach behind the real parked truck, then actual eye-ray Pick/OnUse. No driving, walking journey or whole-route claim."));
  O->SetArrayField(TEXT("evidence"),S.Evidence);O->SetArrayField(TEXT("route"),S.Trace);
  if(!S.Root.IsEmpty()&&!FFileHelper::SaveStringToFile(S47J::Write(O,true),*(S.Root/TEXT("result.json"))))Good=false;
  UE_LOG(LogTemp,Display,TEXT("S47 FarmWorld %s: %s, drive %.2fm walk %.2fm bumps%d"),Good?TEXT("PASS"):TEXT("FAIL"),*Why,S.DriveMetres,S.WalkMetres,S.Bumps);
  FPlatformMisc::RequestExitWithStatus(false,Good?0:1);
 }
 bool Check(US47Main* M,bool Good,const FString& Why)
 {
  ++S.Checks;auto O=S47J::New();S47J::Set(O,TEXT("check"),Why);S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("stage"),S.Stage);S.Evidence.Add(MakeShared<FJsonValueObject>(O));
  UE_LOG(LogTemp,Display,TEXT("S47 FarmWorld %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),*Why);if(!Good)Finish(M,false,Why);return Good;
 }
 void Go(int32 Next){S.Stage=Next;S.At=FPlatformTime::Seconds();S.ProgressAt=S.At;S.HaveLast=false;}
 void Aim(US47Main* M,const FVector& Target)
 {
  if(M->World->bDriving)
  {
   auto* D=M->World->CabDrive();auto* T=M->World->CabTruck();if(!D||!T)return;
   const FVector V=S47Three::Euler(T->Rotation.X,T->Rotation.Y,T->Rotation.Z,TEXT("YXZ")).UnrotateVector(Target-D->CamPos);
   D->SetLook(FMath::Clamp(FMath::Atan2(-V.X,-V.Z),-1.85,1.85),FMath::Clamp(FMath::Atan2(V.Y,FVector2D(V.X,V.Z).Size()),-.85,.55));
  }
  else{const FVector D=Target-S47Three::ToThree(M->Player->EyeUE());M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());}
 }
 void Photo(const TCHAR* Name)
 {S.PendingPhoto=Name;S.PhotoAt=FPlatformTime::Seconds();}
 void Brake(US47Main* M)
 {
  const TWeakObjectPtr<US47World> World=M->World;M->World->Autopilot.Reset();
  M->World->TestInput=[World]{FS47DriveIn I;if(World.IsValid()){const double V=World->DrivingSpeed();I.Throttle=V>.025?-1:V<-.025?1:0;}return I;};
 }
 void Follow(US47Main* M,TArray<FVector> P,double Speed,bool Back=false)
 {
  S.Path=MoveTemp(P);S.Cursor=0;S.PathDone=false;S.MaxSpeed=Speed;S.Reverse=Back;S.ProgressAt=FPlatformTime::Seconds();S.Progress=Road(M->World->CabDrive()->Pos,M->World->Area);
  const TWeakObjectPtr<US47World> World=M->World;
  M->World->TestInput=[World]
  {
   FS47DriveIn I;auto* W=World.Get();auto* D=W?W->CabDrive():nullptr;if(!D||S.Path.IsEmpty())return I;
   const FVector P=Road(D->Pos,W->Area);
   while(S.Cursor<S.Path.Num()-1&&FVector2D(S.Path[S.Cursor].X-P.X,S.Path[S.Cursor].Z-P.Z).Size()<.85)++S.Cursor;
   const double Final=FVector2D(S.Path.Last().X-P.X,S.Path.Last().Z-P.Z).Size();
   if(Final<.35){I.Throttle=D->Speed>.025?-1:D->Speed<-.025?1:0;S.PathDone=FMath::Abs(D->Speed)<.07;return I;}
   FVector Aim=S.Path[S.Cursor],From=P;double Look=2.5;
   for(int32 K=S.Cursor;K<S.Path.Num();++K){const FVector V=S.Path[K]-From;const double L=FVector2D(V.X,V.Z).Size();if(L>=Look){Aim=From+V*(Look/L);break;}Look-=L;Aim=S.Path[K];From=S.Path[K];}
   const FVector V=Aim-P;const double Right=V.X*FMath::Cos(D->Heading)-V.Z*FMath::Sin(D->Heading);
   I.Steer=D->SteerFor(2*Right/FMath::Max(.1,V.X*V.X+V.Z*V.Z));
   const double Wanted=FMath::Min(S.MaxSpeed,FMath::Max(.18,Final*.8))*(S.Reverse?-1:1);I.Throttle=FMath::Clamp((Wanted-D->Speed)*.8,-1.,1.);return I;
  };
 }
 bool Motion(US47Main* M,double Now)
 {
  const bool Driving=M->World->bDriving;const FVector Current=Driving?M->World->CabDrive()->Pos:M->Player->Pos;
  if(S.HaveLast)
  {
   const double D=FVector2D(Current.X-S.Last.X,Current.Z-S.Last.Z).Size();
   if(D>FMath::Max(1.5,(Now-S.LastAt)*12+.3)){Finish(M,false,TEXT("measured route position jumped; no pose fallback is allowed"));return false;}
   if(Driving)S.DriveMetres+=D;else S.WalkMetres+=D;
  }
  S.Last=Current;S.LastAt=Now;S.HaveLast=true;
  const FVector R=Road(Current,M->World->Area);
  if(FVector2D(R.X-S.Progress.X,R.Z-S.Progress.Z).Size()>.2){S.Progress=R;S.ProgressAt=Now;}
  if(Now-S.ProgressAt>14){Finish(M,false,TEXT("actual route stalled for14 seconds without positional progress"));return false;}
  auto O=S47J::New();S47J::Set(O,TEXT("stage"),S.Stage);S47J::Set(O,TEXT("seconds"),Now-S.Began);S47J::Set(O,TEXT("roadPosition"),Vec(R));S47J::Set(O,TEXT("driving"),Driving);S47J::Set(O,TEXT("floor"),Driving?Current.Y:M->Player->FloorY);S47J::Set(O,TEXT("area"),M->World->Area);S.Trace.Add(MakeShared<FJsonValueObject>(O));return true;
 }
 void WalkPath(TArray<FVector> P){S.Path=MoveTemp(P);S.Cursor=0;S.ProgressAt=FPlatformTime::Seconds();S.Progress=S.Last;}
 bool Walk(US47Main* M)
 {
  if(S.Cursor>=S.Path.Num()){M->Player->TestMove.Reset();return true;}
  const FVector Target=Here(M,S.Path[S.Cursor]);const FVector V=Target-M->Player->Pos;const double D=FVector2D(V.X,V.Z).Size();
  if(D<.16){++S.Cursor;M->Player->TestMove.Reset();return false;}
  M->Player->Yaw=FMath::Atan2(-V.X,-V.Z);M->Player->Pitch=-.08;M->Player->TestMove=FVector2D(0,-FMath::Min(1.,D/.4));return false;
 }
 bool Audit(US47Main* M,bool Freeform)
 {
  TArray<FString> Lines;const bool House=M->Web->AuditHighwayRanch(Lines);auto O=S47J::New();S47J::Set(O,TEXT("mode"),Freeform?TEXT("freeform"):TEXT("story"));S47J::Set(O,TEXT("actualHouseAudit"),House);S47J::Set(O,TEXT("houseLines"),FString::Join(Lines,TEXT("\n")));
  TArray<TSharedPtr<FJsonValue>> Cows;bool Herd=false;
  if(M->TownLayer)for(const auto& H:M->TownLayer->Herds)if(H&&H->Area==TEXT("saro"))
  {
   Herd=H->Count()==9&&H->Group.Shown()&&(!Freeform||H->Missing==-1);
   S47J::Set(O,TEXT("herdMiddleRoad"),Vec(Road(H->Middle(),TEXT("saro"))));S47J::Set(O,TEXT("missingCow"),H->Missing);
   const auto Body=M->Web->Obj(TEXT("world.herds.0.bodies"));const auto* Node=Body?M->Web->Node(Body.I):nullptr;const auto* Mesh=Node?Cast<UInstancedStaticMeshComponent>(Node->Comp.Get()):nullptr;
   Herd&=Mesh&&Mesh->GetStaticMesh()&&Mesh->IsVisible()&&Body.Shown()&&Mesh->GetInstanceCount()==9;
   for(int32 I=0;I<H->Count();++I)
   {
    if(!Freeform&&I==H->Missing)continue;
    FVector P;const bool Found=H->CowAt(I,P);const FVector R=Road(P,TEXT("saro"));auto C=S47J::New();S47J::Set(C,TEXT("cow"),I);S47J::Set(C,TEXT("position"),Vec(R));
    if(Mesh)
    {
     FTransform T;if(Mesh->GetInstanceTransform(I,T,true))
     {
      const FVector BodyRoot=S47Three::ToThree(T.GetLocation());S47J::Set(C,TEXT("actualBodyRootThree"),Vec(BodyRoot));
      Herd&=Found&&FVector::Dist(BodyRoot,P-FVector(0,1.1,0))<1.; // CowAt is the collider, "within a step of the cow"; the herd wanders since 6 November.
      const double Floor=S47RoadTerrain::SurfaceY(R.X,R.Z);S47J::Set(C,TEXT("expectedTerrainFloor"),Floor);Herd&=FMath::Abs(BodyRoot.Y-Floor)<.08;
     }
     else Herd=false;
    }
    Herd&=Found&&(Freeform?(R.X>25&&R.X<90&&R.Z>205&&R.Z<280):(R.X<0&&R.Z>500));Cows.Add(MakeShared<FJsonValueObject>(C));
   }
  }
  O->SetArrayField(TEXT("nativeCows"),Cows);S47J::Set(O,TEXT("cowAudit"),Herd);
  bool CollisionCentres=true;
  if(M->TownLayer)for(const auto& H:M->TownLayer->Herds)if(H)
  {
   const FVector Origin=S47FarmWorld::RoadToArea(H->Area,FVector::ZeroVector);
   for(int32 I=0;I<H->Count();++I)
   {
    FVector At;if(H->CowAt(I,At))CollisionCentres&=M->World->HasHerdObstacle(H->Area,At,S47Herd::COW_R);
    else {const auto Circle=H->Obstacle(FVector2D(Origin.X,Origin.Z));CollisionCentres&=M->World->HasHerdObstacle(H->Area,FVector(Circle.X,0,Circle.Z),0);}
   }
  }
  S47J::Set(O,TEXT("bothActualDriveHerdCirclesMatch"),CollisionCentres);
  TArray<TSharedPtr<FJsonValue>> Meshes;
  for(int32 I=0;I<M->Web->PartCount();++I)
  {
   const auto* N=M->Web->Node(I);if(!N||(!N->Name.StartsWith(TEXT("farm:"))&&!N->Name.StartsWith(TEXT("highway:ranch"))))continue;
   const auto* C=Cast<UStaticMeshComponent>(N->Comp.Get());if(!C||!C->GetStaticMesh())continue;
   auto R=S47J::New();S47J::Set(R,TEXT("name"),N->Name);S47J::Set(R,TEXT("mesh"),C->GetStaticMesh()->GetPathName());S47J::Set(R,TEXT("visible"),C->IsVisible()&&M->Web->ObjAt(I).Shown());S47J::Set(R,TEXT("triangles"),C->GetStaticMesh()->GetNumTriangles(0));S47J::Set(R,TEXT("centreUE"),Vec(C->Bounds.Origin));S47J::Set(R,TEXT("extentUE"),Vec(C->Bounds.BoxExtent));Meshes.Add(MakeShared<FJsonValueObject>(R));
  }
  O->SetArrayField(TEXT("actualFarmMeshes"),Meshes);
  TArray<FString> ClearanceLines;const bool RoadOkay=S47FarmWorld::RoadClearance(M,ClearanceLines);
  S47J::Set(O,TEXT("roadClearance"),RoadOkay);S47J::Set(O,TEXT("clearanceLines"),ClearanceLines);S47J::Set(O,TEXT("movedRockInstances"),S47FarmWorld::MovedRockCount(M));
  FHitResult Ground;const FVector At=Here(M,FVector(25,S47FarmWorld::FloorRoad(25,196.7),196.7));FCollisionQueryParams Params(SCENE_QUERY_STAT(FarmWorldGround),true);Params.AddIgnoredActor(M->Player.Get());
  const bool Hit=M->Web->GetWorld()->LineTraceSingleByChannel(Ground,S47Three::ToUE(At+FVector(0,6,0)),S47Three::ToUE(At-FVector(0,6,0)),ECC_Visibility,Params);
  S47J::Set(O,TEXT("gateAccessNativeFloorHit"),Hit);if(Hit){S47J::Set(O,TEXT("gateAccessFloorThree"),Vec(S47Three::ToThree(Ground.ImpactPoint)));if(const auto* C=Ground.GetComponent())S47J::Set(O,TEXT("gateAccessFloorComponent"),C->GetName());}
  S.Evidence.Add(MakeShared<FJsonValueObject>(O));
  const bool FloorOkay=!Freeform||(Hit&&FMath::Abs(S47Three::ToThree(Ground.ImpactPoint).Y-At.Y)<.15);
  return Check(M,House&&Herd&&CollisionCentres&&FloorOkay&&RoadOkay&&S47FarmWorld::GateReady(M)&&S47FarmWorld::Enabled(M)==Freeform,TEXT("actual house topology, exact gate, visible cows, drive collisions, floor and clear farm road match selected mode"));
 }
 bool SaveReload(US47Main* M,const FString& Name)
 {
  const auto Save=M->Flow->GameState();const FString File=S.Root/(Name+TEXT(".json"));FString Text;
  if(!Check(M,FFileHelper::SaveStringToFile(S47J::Write(Save),*File)&&FFileHelper::LoadFileToString(Text,*File),TEXT("full farm save written and freshly read: ")+Name))return false;
  S.RetainedFreeform=S47J::Read(Text);if(!Check(M,bool(S.RetainedFreeform),TEXT("full farm save JSON parsed")))return false;
  US47Flow::FStart Start;Start.bDev=true;Start.State=S.RetainedFreeform;M->Flow->StartGame(Start);++S.Restores;return true;
 }
}
namespace S47FarmWorldTest
{
 void Update(AS47Web* W)
 {
  if(!FParse::Param(FCommandLine::Get(),TEXT("S47FarmWorldTest"))||S.Done)return;
  US47Main* M=W?US47Main::Get(W):nullptr;if(!M)return;
  const double Now=FPlatformTime::Seconds();
  if(S.Started&&Now-S.Began>360){Finish(M,false,TEXT("bounded360-second farm route timeout"));return;}
  if(!M->Flow||!M->World||!M->Player||!M->Inter||M->Flow->bStarting||S47Startup::Blocking()||M->World->bBusy)return;
  if(!S.Started)
  {
   FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
   if(!User.StartsWith(TEXT("/tmp/"))||!FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),FPaths::ConvertRelativePathToFull(User))||!M->IsFreeform()||!M->Freeform||!M->Freeform->Active()||!M->Flow->bDevRun||M->Flow->CaseId!=0){Finish(M,false,TEXT("explicit isolated Freeform developer case0 required"));return;}
   S.Started=true;S.Began=Now;S.Root=FPaths::ProjectSavedDir()/TEXT("FarmWorldTest");IFileManager::Get().MakeDirectory(*S.Root,true);Go(0);return;
  }
  if(Now-S.Began>360){Finish(M,false,TEXT("bounded360-second farm route timeout"));return;}
  if(!S.PendingPhoto.IsEmpty())
  {
   // Render the settled native camera before the next stage changes pose/mode.
   if(Now-S.PhotoAt<.5)return;
   S.Photos.Add(S.PendingPhoto);FScreenshotRequest::RequestScreenshot(S.Root/(S.PendingPhoto+TEXT(".png")),true,false);S.PendingPhoto.Empty();S.At=Now;return;
  }
  if(Now-S.At<.6)return;
  if(S.Stage==0)
  {
   if(!S47FarmWorld::Ready(M)){if(Now-S.At>20)Finish(M,false,TEXT("native farm geometry did not become ready"));return;}
   M->UI->Close(true);M->Freeform->Director->Stop();
   FString CargoFixture;
   if(FParse::Value(FCommandLine::Get(),TEXT("S47FarmCargoFixture="),CargoFixture))
   {
    FString Text;if(!CargoFixture.StartsWith(TEXT("/tmp/"))||!FFileHelper::LoadFileToString(Text,*CargoFixture)){Finish(M,false,TEXT("temporary cargo fixture required"));return;}
    auto Save=S47J::Read(Text);auto Truck=S47J::Obj(Save,TEXT("truckPose"));if(!Truck){Finish(M,false,TEXT("fixture lacks parked truck"));return;}
    const double H=S47J::Num(Truck,TEXT("h"));auto Pose=S47J::New();
    S47J::Set(Pose,TEXT("x"),S47J::Num(Truck,TEXT("x"))+FMath::Sin(H)*3.4);
    S47J::Set(Pose,TEXT("z"),S47J::Num(Truck,TEXT("z"))+FMath::Cos(H)*3.4);S47J::Set(Pose,TEXT("yaw"),H);S47J::Set(Save,TEXT("pose"),Pose);
    S.CargoProbe=true;US47Flow::FStart Start;Start.bDev=true;Start.State=Save;M->Flow->StartGame(Start);Go(13);return;
   }
   if(FParse::Param(FCommandLine::Get(),TEXT("S47FarmWorldAssetsOnly")))
   {
    if(!Audit(M,true))return;
    S.RetainedFreeform=M->Flow->GameState();
    const bool Pasture=FParse::Param(FCommandLine::Get(),TEXT("S47FarmPastureView"));
    const FVector P=S47FarmWorld::RoadToArea(TEXT("saro"),Pasture?FVector(86,0,260):FVector(16.5,0,S47RoadTerrain::RanchZ(16.5)));M->Player->Place(P.X,P.Z,0);
    Go(91);return;
   }
   auto Save=M->Flow->GameState();auto F=S47J::Obj(Save,TEXT("freeform")),Field=S47J::Obj(F,TEXT("field"));
   if(!Check(M,bool(Field),TEXT("initial fixture retains ordinary Freeform field state")))return;
   S47J::Set(Field,TEXT("payload"),3);S47J::Set(Field,TEXT("item"),TEXT("relay"));S47J::Set(Field,TEXT("parcelLoaded"),true);Field->RemoveField(TEXT("carry"));Field->RemoveField(TEXT("contents")); // An empty list would be read as an empty parcel.
   // -S47FarmTruckInWall: the old save parked the service truck across a wall of the new barn (the barn's middle is open floor).
   const bool InWall=FParse::Param(FCommandLine::Get(),TEXT("S47FarmTruckInWall"));S.TruckInWall=InWall;
   const FVector2D WallAt=S47FarmBuildings::Obstacles().Num()?S47FarmBuildings::Obstacles()[0].GetCenter():FVector2D(135.5,170);
   const FVector Barn(WallAt.X,S47RoadTerrain::SurfaceY(WallAt.X,WallAt.Y),WallAt.Y);
   const FVector P=S47FarmWorld::RoadToArea(TEXT("saro"),InWall?Barn:FVector(1.8,S47RoadTerrain::SurfaceY(1.8,230),230));
   auto T=S47J::New();S47J::Set(T,TEXT("at"),TEXT("saro"));S47J::Set(T,TEXT("x"),P.X);S47J::Set(T,TEXT("y"),P.Y);S47J::Set(T,TEXT("z"),P.Z);S47J::Set(T,TEXT("h"),0.);S47J::Set(T,TEXT("p"),0.);S47J::Set(T,TEXT("r"),0.);
   S47J::Set(Save,TEXT("truck"),TEXT("saro"));S47J::Set(Save,TEXT("truckPose"),T);
   auto Pose=S47J::New();S47J::Set(Pose,TEXT("x"),P.X-2.);S47J::Set(Pose,TEXT("z"),P.Z);S47J::Set(Pose,TEXT("yaw"),0.);S47J::Set(Save,TEXT("pose"),Pose);
   FFileHelper::SaveStringToFile(S47J::Write(Save,true),*(S.Root/TEXT("initial-road-and-loaded-cargo-fixture.json")));
   US47Flow::FStart Start;Start.bDev=true;Start.State=Save;M->Flow->StartGame(Start);Go(1);return;
  }
  if(S.Stage==90)
  {
   if(FParse::Param(FCommandLine::Get(),TEXT("S47FarmClearanceCycle"))){US47Flow::FStart Story;Story.bDev=true;Story.Jump=TEXT("nightService");M->Flow->StartGame(Story);Go(92);return;}
   Finish(M,true,TEXT("native farm asset smoke only; continuous route and cargo/Continue were not executed"));return;
  }
  if(S.Stage==92)
  {
   if(!Audit(M,false))return;US47Flow::FStart Free;Free.bDev=true;Free.State=S.RetainedFreeform;M->Flow->StartGame(Free);++S.Restores;Go(93);return;
  }
  if(S.Stage==93){if(!Audit(M,true))return;Finish(M,true,TEXT("resource/render and exact rock clearance Story/Freeform cycle complete; full journey excluded"));return;}
  if(S.Stage==91)
  {
   const bool Pasture=FParse::Param(FCommandLine::Get(),TEXT("S47FarmPastureView"));
   Aim(M,Here(M,Pasture?FVector(55,1.1,240):FVector(108,1.5,192)));
   Photo(Pasture?TEXT("assets-only-native-pasture"):TEXT("assets-only-native-farm"));Go(90);return;
  }
  if(S.Stage==1)
  {
   if(!Audit(M,true))return;
   if(S.TruckInWall)
   {
    const auto* Truck=M->World->SaroTruck();const FVector Origin=S47FarmWorld::RoadToArea(TEXT("saro"),FVector::ZeroVector);
    TArray<FS47Obstacle> Walls;for(const FBox2D& Wall:S47FarmBuildings::Obstacles())Walls.Add(FS47Obstacle::Box(Wall.Min.X+Origin.X,Wall.Max.X+Origin.X,Wall.Min.Y+Origin.Z,Wall.Max.Y+Origin.Z));
    const FVector Barn=S47FarmWorld::RoadToArea(TEXT("saro"),S47FarmBuildings::BarnPoint(0,0));
    if(!Check(M,Truck&&!S47FarmWorld::TruckHits(Walls,Truck->Position.X,Truck->Position.Z,Truck->Rotation.Y,Truck->HalfWidth(),Truck->HalfLength(),.05)&&FVector2D::Distance(FVector2D(Truck->Position.X,Truck->Position.Z),FVector2D(Barn.X,Barn.Z))>.4,TEXT("service truck restored inside a barn wall was moved to clear ground")))return;
    Finish(M,true,TEXT("service truck restored inside a barn wall was moved to clear ground; the route below starts from the road"));return;
   }
   if(!Check(M,M->Freeform->Field->ParcelStage()==3&&M->Freeform->Field->ParcelItem()==TEXT("relay"),TEXT("declared initial loaded relay fixture restored without stock/reward injection")))return;
   Aim(M,Here(M,FVector(21,1,S47RoadTerrain::RanchZ(21))));Photo(TEXT("01-entrance-freeform"));Go(2);return;
  }
  if(S.Stage==2){M->World->Roam(TEXT("saro"));Go(3);return;}
  if(S.Stage==3)
  {
   if(!M->World->bDriving){if(Now-S.At>12)Finish(M,false,TEXT("ordinary Roam did not board the parked Jeep"));return;}
   if(!Check(M,M->World->CabTruck()->TripoJeep()&&!M->World->bCommuteVehicle,TEXT("own service Jeep uses production controller, not a borrowed commute vehicle")))return;
   const auto Previous=M->World->CabDrive()->OnBump;M->World->CabDrive()->OnBump=[Previous](double V){++S.Bumps;if(Previous)Previous(V);};
   TArray<FVector> Path={FVector(1.8,0,214),FVector(1.8,0,208)};const double Z0=S47RoadTerrain::RanchZ(12),Cz=Z0+6;
   Path.Add(FVector(1.8,0,Cz));for(int32 I=1;I<=18;++I){const double A=PI+I*PI/36;Path.Add(FVector(7.8+6*FMath::Cos(A),0,Cz+6*FMath::Sin(A)));}
   Path.Append({FVector(12,0,196),FVector(28,0,196.7)});Follow(M,Path,3);Go(4);return;
  }
  if(S.Stage==4)
  {
   if(!Motion(M,Now)||!S.PathDone)return;Brake(M);
   if(!Check(M,Road(M->World->CabDrive()->Pos,M->World->Area).X>25,TEXT("real Jeep footprint crossed the physically open Alderete gate")))return;
   Aim(M,Here(M,FVector(108,1.5,192)));Photo(TEXT("02-through-gate"));Go(5);return;
  }
  if(S.Stage==5){Follow(M,{FVector(55,0,197.6),FVector(80,0,198),FVector(96,0,196.8)},4);Go(6);return;}
  if(S.Stage==6)
  {
   if(!Motion(M,Now)||!S.PathDone)return;Brake(M);Aim(M,Here(M,FVector(108,1.5,192)));Photo(TEXT("03-farm-house"));Go(7);return;
  }
  if(S.Stage==7)
  {
   // Drive.Pos is a horizontal origin (Y=0); use the real sprung body pose
   // which Park and TruckSave preserve on the local terrain.
   S.Park=M->World->CabTruck()->Position;
   if(!Check(M,M->World->GetOutHere(),TEXT("production stopped Jeep egress parks at the farm, without StopDriving home restoration")))return;
   Go(8);return;
  }
  if(S.Stage==8)
  {
   if(!Check(M,!M->World->bDriving&&FVector::Dist(M->World->ServicePosition(),S.Park)<.1,TEXT("actual own parked vehicle remains at the farm")))return;
   S.EgressRoad=Road(M->Player->Pos,M->World->Area);WalkPath({FVector(79,0,203),FVector(70,0,212),FVector(70,0,220),FVector(55,0,240)});Go(9);return;
  }
  if(S.Stage==9)
  {
   if(!Motion(M,Now)||!Walk(M))return;if(!Audit(M,true))return;
   S.SavedFoot=M->Player->Pos;Aim(M,Here(M,FVector(65,.3,222.5)));Photo(TEXT("04-pasture"));Go(10);return;
  }
  if(S.Stage==10){if(!SaveReload(M,TEXT("farm-pasture-with-cargo")))return;Go(11);return;}
  if(S.Stage==11)
  {
   M->Freeform->Director->Stop();
   if(!Check(M,M->IsFreeform()&&FVector::Dist(M->Player->Pos,S.SavedFoot)<.05&&FVector::Dist(M->World->ServicePosition(),S.Park)<.1&&M->Freeform->Field->ParcelStage()==3&&M->Freeform->Field->ParcelItem()==TEXT("relay"),TEXT("disk/full Flow Continue retains pasture pose, actual parked Jeep and loaded parcel")))return;
   Photo(TEXT("05-continued-pasture"));WalkPath({FVector(70,0,220),FVector(70,0,212),FVector(79,0,203),FVector(91,0,200)});Go(12);return;
  }
  if(S.Stage==12)
  {
   if(!Motion(M,Now)||!Walk(M))return;
   const FVector P=Road(M->Freeform->Field->InteractionPoint(TEXT("ff:cargo")),M->World->Area);WalkPath({P+FVector(-.8,0,1.4)});Go(13);return;
  }
  if(S.Stage==13)
  {
   if(!S.CargoProbe&&(!Motion(M,Now)||!Walk(M)))return;Aim(M,M->Freeform->Field->InteractionPoint(TEXT("ff:cargo")));Photo(TEXT("cargo-native-approach"));Go(14);return;
  }
  if(S.Stage==14||S.Stage==15)
  {
   const FString Id=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
   // A visible loaded parcel sits over the rack and is itself a valid pickup.
   const bool Target=Id==TEXT("ff:cargo")||(S.Stage==14&&Id==TEXT("ff:parcel"));
   if(!Check(M,Target&&bool(M->Player->OnUse),TEXT("actual native eye ray selects parked cargo or its loaded parcel: ")+Id))return;
   M->Player->OnUse(Id);
   if(!Check(M,M->Freeform->Field->ParcelStage()==(S.Stage==14?2:3),S.Stage==14?TEXT("production cargo Use unloads restored parcel"):TEXT("production cargo Use reloads the same parcel without delivery/reward")))return;
   if(S.Stage==14){Go(15);return;}if(S.CargoProbe){Finish(M,true,TEXT("cargo-only fixture completed actual unload/reload; full route excluded"));return;}M->World->Roam(TEXT("saro"));Go(16);return;
  }
  if(S.Stage==16)
  {
   if(!M->World->bDriving){if(Now-S.At>12)Finish(M,false,TEXT("farm Jeep reboarding failed"));return;}
   const auto Previous=M->World->CabDrive()->OnBump;M->World->CabDrive()->OnBump=[Previous](double V){++S.Bumps;if(Previous)Previous(V);};
   TArray<FVector> Back={FVector(80,0,198),FVector(55,0,197.6),FVector(28,0,196.7),FVector(12,0,196)};
   const double Z0=S47RoadTerrain::RanchZ(12),Cz=Z0+6;
   for(int32 I=18;I>=0;--I){const double A=PI+I*PI/36;Back.Add(FVector(7.8+6*FMath::Cos(A),0,Cz+6*FMath::Sin(A)));}
   Back.Append({FVector(1.8,0,214),FVector(1.8,0,230)});Follow(M,Back,2,true);Go(17);return;
  }
  if(S.Stage==17)
  {
   if(!Motion(M,Now)||!S.PathDone)return;Brake(M);
   if(!Check(M,S.DriveMetres>180&&S.WalkMetres>90&&M->Freeform->Field->ParcelStage()==3,TEXT("continuous own-Jeep return reached main road with real walking and retained cargo")))return;
   if(!Check(M,M->World->GetOutHere(),TEXT("ordinary egress ends measured physical journey on main road")))return;Go(18);return;
  }
  if(S.Stage==18)
  {
   if(!SaveReload(M,TEXT("returned-freeform")))return;Go(19);return;
  }
  if(S.Stage==19)
  {
   Aim(M,Here(M,FVector(21,1,S47RoadTerrain::RanchZ(21))));Photo(TEXT("06-return-entrance"));Go(20);return;
  }
  if(S.Stage==20)
  {US47Flow::FStart Story;Story.bDev=true;Story.Jump=TEXT("nightService");M->Flow->StartGame(Story);Go(21);return;}
  if(S.Stage==21)
  {
   // Separate mode-comparison camera fixture, outside the completed route.
   if(!Check(M,!M->IsFreeform()&&!S47FarmWorld::Enabled(M),TEXT("genuine fresh Story start disables the Freeform farm")))return;
   if(!Audit(M,false))return;
   const FVector P=S47FarmWorld::RoadToArea(TEXT("saro"),FVector(16.5,0,S47RoadTerrain::RanchZ(16.5)));M->Player->Place(P.X,P.Z,0);Aim(M,Here(M,FVector(21,1,S47RoadTerrain::RanchZ(21))));Go(22);return;
  }
  if(S.Stage==22)
  {Photo(TEXT("07-story-entrance-negative"));Go(221);return;}
  if(S.Stage==221)
  {
   M->Player->Yaw=-PI/2;M->Player->Pitch=-.05;M->Player->TestMove=FVector2D(0,-1);
   const FVector P=Road(M->Player->Pos,M->World->Area);
   if(P.X+M->Player->Radius>21.05){M->Player->TestMove.Reset();Finish(M,false,TEXT("Story Walker footprint crossed the original closed Alderete gate"));return;}
   if(Now-S.At<4)return;
   M->Player->TestMove.Reset();
   if(!Check(M,P.X>19&&P.X+M->Player->Radius<=21.05,TEXT("actual positive Walker input meets the original closed Story gate without crossing its plane")))return;
   Go(23);return;
  }
  if(S.Stage==23)
  {US47Flow::FStart Free;Free.bDev=true;Free.State=S.RetainedFreeform;M->Flow->StartGame(Free);++S.Restores;Go(24);return;}
  if(S.Stage==24)
  {
   if(!Audit(M,true))return;
   if(!Check(M,M->Freeform->Field->ParcelStage()==3&&M->Freeform->Field->ParcelItem()==TEXT("relay"),TEXT("Story-to-Freeform restoration retains original cargo")))return;
   const FVector P=S47FarmWorld::RoadToArea(TEXT("saro"),FVector(16.5,0,S47RoadTerrain::RanchZ(16.5)));M->Player->Place(P.X,P.Z,0);Aim(M,Here(M,FVector(21,1,S47RoadTerrain::RanchZ(21))));Go(25);return;
  }
  if(S.Stage==25){Photo(TEXT("08-freeform-entrance-restored"));Go(26);return;}
  if(S.Stage==26)Finish(M,true,TEXT("native farm drive/walk/cargo/Continue and Story mode restoration complete; route PNGs require visual review"));
 }
}
