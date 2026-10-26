#include "S47Evening.h"

#include "S47EveningVisual.h"
#include "S47Flow.h"
#include "S47Main.h"
#include "S47Inter.h"
#include "S47RecordedDialogue.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

// Opt-in integration control. Setup/phone choices use the API, all portal crossings use
// the normal walker's movement and collision resolver, including after full-world loads.
struct FS47Evening::FEntranceTest
{
 bool bEnabled = false, bDone = false, bVehicles = false, bThresholdRestored = false, bParking = false, bSawWaitingService = false, bSavedStop = false;
 int32 Condition = 0, Stage = 0, Waypoint = 0, Passes = 0, CloseRefusals = 0, OutsideToggles = 0;
 double Elapsed = 0, Since = 0, Hold = 3;
 FString Shots;
 TArray<FVector> Path;
 FVector Park;
 double Heading = 0;
};

void FS47Evening::ControlEntranceTest(double Dt)
{
 if (!EntranceTest)
 {
  EntranceTest = MakeShared<FEntranceTest>();
  EntranceTest->bVehicles = FParse::Param(FCommandLine::Get(), TEXT("S47EveningVehicles"));
  EntranceTest->bParking = FParse::Param(FCommandLine::Get(), TEXT("S47SaroParking"));
  EntranceTest->bSavedStop = FParse::Param(FCommandLine::Get(), TEXT("S47ParkingSavedStop"));
  EntranceTest->bEnabled = EntranceTest->bParking || EntranceTest->bVehicles || FParse::Param(FCommandLine::Get(), TEXT("S47EveningEntrance"));
  if (!EntranceTest->bEnabled) return;
  FParse::Value(FCommandLine::Get(), TEXT("S47EveningShots="), EntranceTest->Shots);
  if (!EntranceTest->Shots.IsEmpty()) IFileManager::Get().MakeDirectory(*EntranceTest->Shots, true);
 }
 FEntranceTest& T = *EntranceTest;
 if (!T.bEnabled || T.bDone) return;
 const auto Fail = [this, &T](const FString& Why)
 {
  T.bDone = true;
  if (M && M->Player) M->Player->TestMove.Reset();
  const FVector P = M && M->Player ? M->Player->Pos : FVector::ZeroVector;
  UE_LOG(LogTemp, Error, TEXT("S47: evening entrance FAIL condition %d stage %d waypoint %d at %.3f %.3f: %s"), T.Condition, T.Stage, T.Waypoint, P.X, P.Z, *Why);
  if (M && M->Web && M->Player)
  {
   const double R = M->Player->Radius;
   for (const FS47Rect& C : M->Web->Colliders())
    if (P.X + R > C.MinX && P.X - R < C.MaxX && P.Z + R > C.MinZ && P.Z - R < C.MaxZ)
     UE_LOG(LogTemp, Error, TEXT("S47: entrance overlapping collider '%s' enabled %d [%.3f, %.3f] [%.3f, %.3f]"), *C.Id, C.bEnabled, C.MinX, C.MaxX, C.MinZ, C.MaxZ);
  }
  FPlatformMisc::RequestExit(false);
 };
 if (!M || !M->Player || !M->World || !M->Story || !M->Flow || !M->Web || !Visual) { Fail(TEXT("runtime dependency missing")); return; }
 if (!M->Flow->bDevRun || M->Flow->CaseId != 0) { Fail(TEXT("requires an isolated developer run without S47Case")); return; }
 T.Elapsed += Dt;
 if (T.Elapsed > (T.bParking?90:300)) { Fail(TEXT("control timeout")); return; }
 if (M->World->bBusy) return;
 if (T.Hold > 0) { T.Hold -= Dt; return; }
 const auto Stage = [&T](int32 Next, double Hold = 0) { T.Stage = Next; T.Since = T.Elapsed; T.Hold = Hold; };
 const auto Shot = [this, &T](const FString& Name)
 {
  if (T.Shots.IsEmpty()) return;
  const FString Path = T.Shots / (TEXT("entrance-") + Name + TEXT(".png"));
  // The walker's camera consumes Yaw on its next tick. Stage holds keep the same
  // physical pose and heading until this delayed capture sees that camera update.
  M->Story->After(0.1, [Path]
  {
   FScreenshotRequest::RequestScreenshot(Path, true, false);
   UE_LOG(LogTemp, Display, TEXT("S47: entrance screenshot requested with HUD: %s"), *Path);
  });
 };
 const auto Walk = [this, &T]()
 {
  if (!T.Path.IsValidIndex(T.Waypoint)) { M->Player->TestMove.Reset(); return true; }
  const FVector D = T.Path[T.Waypoint] - M->Player->Pos;
  const double Distance = FMath::Sqrt(D.X * D.X + D.Z * D.Z);
  if (Distance < 0.08) { ++T.Waypoint; M->Player->TestMove.Reset(); return false; }
  M->Player->Yaw = FMath::Atan2(-D.X, -D.Z); M->Player->Pitch = -0.06;
  M->Player->TestMove = FVector2D(0, -FMath::Min(1.0, Distance * 2));
  return false;
 };
 if (T.bParking)
 {
  // Only the starting fixture is placed. Travel, stop detection and arrival use normal gameplay.
  const auto AimDoor=[this,&Fail](const FString& Id)
  {
   const FS47WebItem* Item=M->Web->Item(Id);
   const FS47Obj Obj=Item?M->Web->ObjAt(Item->Node):FS47Obj();
   const FBox Bounds=Obj.WorldBox();
   if(!Obj || !Bounds.IsValid){Fail(TEXT("missing physical entrance target: ")+Id);return false;}
   const FVector D=Bounds.GetCenter()-S47Three::ToThree(M->Player->EyeUE());
   M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);
   M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
   return true;
  };
  const auto UseAimedDoor=[this,&Fail](const FString& Id)
  {
   if(!M->Inter || !M->Player->OnUse){Fail(TEXT("ordinary entrance interaction unavailable"));return false;}
   // Use the actual settled camera ray and normal gameplay callback. Pick enforces
   // the existing reach and usable mesh; no direct door ID dispatch bypasses it.
   const FString Picked=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
   if(Picked!=Id){Fail(FString::Printf(TEXT("aimed entrance ray expected %s, picked '%s'"),*Id,*Picked));return false;}
   if(M->Web->IsDoorOpen(Id)){Fail(TEXT("entrance fixture unexpectedly already open: ")+Id);return false;}
   M->Player->OnUse(Picked);
   if(!M->Web->IsDoorOpen(Id)){Fail(TEXT("normal aimed entrance use did not open: ")+Id);return false;}
   UE_LOG(LogTemp,Display,TEXT("S47: SARO parking PASS actual camera Pick and Player OnUse opened %s"),*Id);
   return true;
  };
  const auto CheckPark=[this,&T]()
  {
   const auto* Service=M->World->SaroTruck();const auto& Box=M->Player->PersonalCarBox;
   return !Commute() && Pack.Flag(TEXT("intro_arrived")) && !Pack.Flag(TEXT("food_bought")) && !M->World->bDriving
    && Visual->PersonalVisible() && Visual->ParkedPosition().Equals(T.Park,.015)
    && FMath::IsNearlyEqual(Visual->ParkedHeading(),T.Heading,.001)
    && Box && FMath::IsNearlyEqual(Box->X,T.Park.X,.015) && FMath::IsNearlyEqual(Box->Z,T.Park.Z,.015)
    && Service && !Service->IsPersonalVehicle() && Service->Group.Shown() && Service->Body.Shown() && Service->Shell.Shown()
    && Service->Position.Equals(FVector(8.9,0,8),.01) && FMath::IsNearlyEqual(Service->Rotation.Y,UE_DOUBLE_PI,.001)
    && !Visual->ServiceWaitingVisible();
  };
  if(T.Stage==0)
  {
   if(!Commute()){Fail(TEXT("S47SaroParking requires a fresh -S47Evening=commute"));return;}
   M->World->StopDriving();M->World->bCommuteVehicle=true;
   FS47TripOpts O;O.Leg=TEXT("saro");O.AtPos=FVector(-22.2,-.62,32);O.AtHeading=0;
   double BayX=0;
   const bool BayFixture=FParse::Value(FCommandLine::Get(),TEXT("S47VisitorBay="),BayX);
   if(BayFixture)
   {
    double ParkZ=14.3;FParse::Value(FCommandLine::Get(),TEXT("S47VisitorZ="),ParkZ);
    O.AtPos=FVector(BayX,-.6,ParkZ);
    O.AtHeading=FParse::Param(FCommandLine::Get(),TEXT("S47ReverseParking"))?UE_DOUBLE_PI:0;
    FParse::Value(FCommandLine::Get(),TEXT("S47VisitorHeading="),O.AtHeading);
    if(!FS47EveningVisual::FitsArrivalArea(*O.AtPos,O.AtHeading)){Fail(TEXT("invalid visitor area fixture"));return;}
   }
   if(T.bSavedStop)
   {
    if(!BayFixture){Fail(TEXT("saved-stop recovery requires explicit visitor-area pose"));return;}
    auto Pose=S47J::New();S47J::Set(Pose,TEXT("x"),O.AtPos->X);S47J::Set(Pose,TEXT("y"),O.AtPos->Y);S47J::Set(Pose,TEXT("z"),O.AtPos->Z);S47J::Set(Pose,TEXT("h"),O.AtHeading);S47J::Set(Pose,TEXT("at"),TEXT("saro"));
    M->World->PlaceTruck(TEXT("saro"),Pose);M->World->Enter(TEXT("saro"));M->Player->Place(-19.5,21.5,0);
    T.Park=*O.AtPos;T.Heading=O.AtHeading;Stage(20,.5);return;
   }
   M->World->Autopilot=BayFixture?TOptional<double>():TOptional<double>(3.5);
   M->World->StartTrip(TEXT("saro"),TEXT("saro"),O);
   UE_LOG(LogTemp,Display,TEXT("S47: SARO parking START ordinary controller/TripStep, fixture=%s x=%.2f z=%.2f heading=%.3f; no food purchased; human input UNVERIFIED"),BayFixture?TEXT("stationary visitor area"):TEXT("entrance approach"),O.AtPos->X,O.AtPos->Z,O.AtHeading);
   Stage(1,BayFixture?.05:1);return;
  }
  if(T.Stage==20)
  {
   // Reproduce an older save made on foot after the strict bay gate rejected the stop.
   // Setup places the car; recovery must happen only through real disk/full-world load.
   T.bSawWaitingService=Visual->ServiceWaitingVisible();
   const auto* Car=M->World->TruckHere();
   if(!Commute() || Pack.Flag(TEXT("intro_arrived")) || M->World->bDriving || !Car || !Car->Position.Equals(T.Park,.01) || !T.bSawWaitingService){Fail(TEXT("legacy pending-arrival fixture did not retain uncommitted parked car"));return;}
   const FString Dir=FPaths::ProjectSavedDir()/TEXT("SaroPendingArrivalControl");
   FS47SaveStore Writer(Dir);Writer.Init();FS47SaveCard Card;Card.Chapter=TEXT("Pending visitor arrival");Card.Place=TEXT("SARO");Card.Clock=M->Story->ClockText(false);
   FString Error;const auto Meta=Writer.Save(1,TEXT("manual"),0,M->Flow->GameState(),Card,Error);
   if(!Meta){Fail(TEXT("pending arrival disk write: ")+Error);return;}
   FS47SaveStore Reader(Dir);Reader.Init();const auto Saved=Reader.Load(Meta->Id);
   if(!Saved){Fail(TEXT("pending arrival fresh disk read failed"));return;}
   if(!T.Shots.IsEmpty())FFileHelper::SaveStringToFile(S47J::Write(Saved,true),*(T.Shots/TEXT("saro-pending-arrival.disk-save.json")));
   US47Flow::FStart Restore;Restore.State=Saved;Restore.CaseId=0;Restore.bDev=true;Restore.Playtime=M->Playtime;
   M->Flow->StartGame(Restore);
   if(Commute() || !Pack.Flag(TEXT("intro_arrived")) || Pack.Flag(TEXT("handover_done")) || !Visual->ParkedPosition().Equals(T.Park,.01) || !FMath::IsNearlyEqual(Visual->ParkedHeading(),T.Heading,.001)){Fail(TEXT("legacy stationary save did not recover arrival with exact pose and handover still pending"));return;}
   UE_LOG(LogTemp,Display,TEXT("S47: SARO parking PASS pending stationary save recovered through fresh disk/full-world load without moving car or completing phone"));
   Stage(7,.55);return;
  }
  if(T.Stage==1 || T.Stage==7)
  {
   if(T.Stage==1 && M->World->bDriving)
   {
    T.bSawWaitingService|=Visual->ServiceWaitingVisible();
    if(!Visual->PersonalVisible()||!Visual->ServiceWaitingVisible()){Fail(TEXT("moving personal car or stable waiting service pickup missing"));return;}
    return;
   }
   if(T.Stage==1)
   {
    // Park releases the cab immediately; its normal OnParked story transaction follows
    // the 0.4 s fade. bBusy is not the indicator for that existing asynchronous handover.
    UE_LOG(LogTemp,Display,TEXT("S47: SARO parking awaiting normal 0.4 s OnParked handover after actual TripStep stop"));
    Stage(7,.55);return;
   }
   if(M->World->bDriving || Commute() || !Pack.Flag(TEXT("intro_arrived")) || !T.bSawWaitingService){Fail(TEXT("ordinary TripStep failed to commit arrival with waiting service visible after normal handover"));return;}
   UE_LOG(LogTemp,Display,TEXT("S47: SARO parking PASS %s committed arrival after %.3f s"),T.bSavedStop?TEXT("pending stationary save recovery"):TEXT("normal delayed OnParked"),T.Elapsed-T.Since);
   if(T.bSavedStop && (!Visual->ParkedPosition().Equals(T.Park,.01) || !FMath::IsNearlyEqual(Visual->ParkedHeading(),T.Heading,.001))){Fail(TEXT("recovered stationary save changed original car pose"));return;}
   T.Park=Visual->ParkedPosition();T.Heading=Visual->ParkedHeading();
   if(!FS47EveningVisual::FitsArrivalArea(T.Park,T.Heading)||!CheckPark()){Fail(TEXT("arrival body/collider or distinct service pose invalid"));return;}
   UE_LOG(LogTemp,Display,TEXT("S47: SARO parking PASS actual arrival pos %.6f %.6f %.6f heading %.6f floor %.6f; waiting service persisted and original service restored"),T.Park.X,T.Park.Y,T.Park.Z,T.Heading,M->Player->FloorY);
   // Ordinary GetOut supplies the initial foot pose. Clear the actual car, then
   // join the north pedestrian opening at x=-11.6 and its short path to the ramp.
   // The old fixed (-19.5,21.5) detour grazed the existing bollard at (-19,21.1).
   const double C=FMath::Abs(FMath::Cos(T.Heading)),S=FMath::Abs(FMath::Sin(T.Heading));
   const double HalfX=FS47EveningVisual::PersonalHalfWidth()*C+FS47EveningVisual::PersonalHalfLength()*S;
   const double HalfZ=FS47EveningVisual::PersonalHalfWidth()*S+FS47EveningVisual::PersonalHalfLength()*C;
   FVector Egress=M->Player->Pos;T.Path.Reset();
   if(Egress.X>T.Park.X-HalfX-.27 && Egress.X<T.Park.X+HalfX+.27 && Egress.Z>T.Park.Z-HalfZ-.27 && Egress.Z<T.Park.Z+HalfZ+.27)
   {
    const double Cos=FMath::Cos(T.Heading),Sin=FMath::Sin(T.Heading);
    const FVector Offset=Egress-T.Park;const double Side=Offset.X*Cos-Offset.Z*Sin<0?-1:1;
    Egress=T.Park+FVector(Side*4*Cos-.3*Sin,0,-Side*4*Sin-.3*Cos);T.Path.Add(Egress);
   }
   if(Egress.X>T.Park.X-HalfX-.27)
   {
    const double GoZ=Egress.Z<T.Park.Z-HalfZ ? T.Park.Z-HalfZ-.8 : T.Park.Z+HalfZ+.8;
    T.Path.Add(FVector(Egress.X,0,GoZ));Egress.Z=GoZ;
   }
   const double ClearX=FMath::Min(Egress.X,T.Park.X-HalfX-.8);
   const double NorthZ=FMath::Min(10.9,T.Park.Z-HalfZ-.8);
   T.Path.Append({FVector(ClearX,0,Egress.Z),FVector(ClearX,0,NorthZ),FVector(-11.6,0,NorthZ),FVector(-11.6,0,5.7),FVector(-9,0,5.7),FVector(-7.55,0,5.7)});
   UE_LOG(LogTemp,Display,TEXT("S47: SARO parking physical route clears car at x=%.3f z=%.3f, then uses north visitor path x=-11.6; original car and Walker pose retained"),ClearX,NorthZ);
   T.Waypoint=0;Stage(5);return;
  }
  if(T.Stage==5)
  {
   if(!CheckPark()){Fail(TEXT("walking to the entrance changed a parked car"));return;}
   if(!Walk())return;
   if(!M->Inter || !M->Inter->LabelOf(TEXT("exitDoor")).Contains(TEXT("visitor entrance"))){Fail(TEXT("west ramp door is not registered as visitor entrance"));return;}
   if(!AimDoor(TEXT("exitDoor")))return;
   Shot(TEXT("saro-walk-entrance"));Stage(8,1);return;
  }
  if(T.Stage==8)
  {
   if(!UseAimedDoor(TEXT("exitDoor")))return;
   T.Path={FVector(-6.5,0,5.7),FVector(-5.2,0,5.7),FVector(-2.4,0,5.7)};T.Waypoint=0;Stage(9,.8);return;
  }
  if(T.Stage==9)
  {
   if(!CheckPark()){Fail(TEXT("entry through the visitor ramp changed the parked car"));return;}
   if(!Walk())return;
   if(!AimDoor(TEXT("doorSouth")))return;
   Stage(12,.35);return;
  }
  if(T.Stage==12)
  {
   if(!UseAimedDoor(TEXT("doorSouth")))return;
   T.Path={FVector(-2.4,0,5.2),FVector(-2.4,0,4.1),FVector(-2.4,0,3.5)};T.Waypoint=0;Stage(10,.8);return;
  }
  if(T.Stage==10)
  {
   if(!Walk())return;
   if(!CheckPark() || Pack.Flag(TEXT("handover_done")) || !M->Web->Rect(TEXT("room.bounds")).Contains(M->Player->Pos)){Fail(TEXT("physical visitor entrance route failed to reach control room before handover"));return;}
   UE_LOG(LogTemp,Display,TEXT("S47: SARO parking PASS actual egress/west ramp/visitor entrance/corridor/control-room walker route %.3f %.3f floor %.3f, no teleport or Walkable-only assertion"),M->Player->Pos.X,M->Player->Pos.Z,M->Player->FloorY);
   Shot(TEXT("saro-entry-control-room"));
   T.Path={FVector(-2.4,0,4.1),FVector(-2.4,0,5.7),FVector(-5.2,0,5.7),FVector(-6.5,0,5.7),FVector(-7.55,0,5.7),FVector(-9,0,5.7),FVector(-11.3,0,5.7)};T.Waypoint=0;Stage(11,1);return;
  }
  if(T.Stage==11)
  {
   if(!Walk())return;
   if(!CheckPark() || !M->Web->IsDoorOpen(TEXT("exitDoor"))){Fail(TEXT("return through visitor entrance failed"));return;}
   UE_LOG(LogTemp,Display,TEXT("S47: SARO parking PASS bidirectional visitor entrance and ramp movement before handover"));Stage(6);return;
  }
  if(T.Stage==6)
  {
   M->Player->Place(-18,21,-.65);M->Player->Pitch=-.10;Shot(TEXT("saro-actual-parking"));Stage(2,1);return;
  }
  if(T.Stage==2)
  {
   const FString Dir=FPaths::ProjectSavedDir()/TEXT("SaroParkingControl");
   FS47SaveStore Writer(Dir);Writer.Init();FS47SaveCard Card;Card.Chapter=TEXT("SARO parking control");Card.Place=TEXT("SARO");Card.Clock=M->Story->ClockText(false);
   FString Error;const auto Meta=Writer.Save(1,TEXT("manual"),0,M->Flow->GameState(),Card,Error);
   if(!Meta){Fail(TEXT("parking disk write: ")+Error);return;}
   FS47SaveStore Reader(Dir);Reader.Init();const auto Saved=Reader.Load(Meta->Id);
   if(!Saved){Fail(TEXT("parking fresh disk read failed"));return;}
   if(!T.Shots.IsEmpty())FFileHelper::SaveStringToFile(S47J::Write(Saved,true),*(T.Shots/TEXT("saro-parking.disk-save.json")));
   US47Flow::FStart Restore;Restore.State=Saved;Restore.CaseId=0;Restore.bDev=true;Restore.Playtime=M->Playtime;
   M->Flow->StartGame(Restore);Stage(3,1);return;
  }
  if(T.Stage==3)
  {
   if(!CheckPark()){Fail(TEXT("fresh disk/world reload changed either car or personal collider"));return;}
   Shot(TEXT("saro-disk-parking"));Stage(4,1);return;
  }
  if(T.Stage==4)
  {
   T.bDone=true;
   UE_LOG(LogTemp,Display,TEXT("S47: SARO parking PASS COMPLETE %s, whole visitor area regardless paint/heading, camera-ray entrance interaction and actual bidirectional west-entrance walk, waiting white service retained, distinct blue personal car after arrival and fresh disk/full-world reload; no phone shortcut or human-input claim"),T.bSavedStop?TEXT("pending stationary save recovery"):TEXT("ordinary drive/controller/TripStep"));
   FPlatformMisc::RequestExit(false);return;
  }
  return;
 }
 if (T.bVehicles)
 {
  const auto Frame = [this](const FS47Truck& Car, const FVector& At, const FVector& Target)
  {
   const FVector P = Car.LocalToWorld(At), Look = Car.LocalToWorld(Target);
   M->Player->Place(P.X, P.Z, FMath::Atan2(P.X - Look.X, P.Z - Look.Z));
   M->Player->FloorY = Car.Position.Y;
   const FVector D = Look - FVector(P.X, Car.Position.Y + M->Player->Eye, P.Z);
   M->Player->Pitch = FMath::Atan2(D.Y, FMath::Sqrt(D.X * D.X + D.Z * D.Z));
  };
  if (T.Stage == 0)
  {
   FS47Truck* Car = M->World->CabTruck();
   if (!Commute() || !M->World->bDriving || !Car || !Car->IsPersonalVehicle() || Car->Group.Shown() || !Visual->PersonalVisible() || !Visual->PersonalDoorVisible() || Visual->PersonalDoorAngle() != 0 || !Car->DriverEye.Equals(FS47EveningVisual::PersonalDriverEye(), 0.001))
   { Fail(TEXT("driven wagon must show its own body, closed leaf and driver eye while the entire SARO carrier group is hidden")); return; }
   Shot(TEXT("vehicles-driver-seat"));
   UE_LOG(LogTemp, Display, TEXT("S47: evening vehicles PASS driving: personal mesh and closed door visible, complete SARO group hidden, own driver eye %.3f %.3f %.3f; actual runtime seat capture"), Car->DriverEye.X, Car->DriverEye.Y, Car->DriverEye.Z);
   Stage(100, 1); return;
  }
  if (T.Stage == 100)
  {
   if (FMath::Abs(M->World->DrivingSpeed()) > 0.05 || !M->World->GetOutHere()) { Fail(TEXT("stationary wagon could not use ordinary GetOutHere")); return; }
   Stage(101, 1); return;
  }
  if (T.Stage == 101 || T.Stage == 102)
  {
   FS47Truck* Car = M->World->TruckHere();
   if (!Commute() || M->World->bDriving || !Car || !Car->IsPersonalVehicle() || Car->Group.Shown() || !Visual->PersonalVisible() || !Visual->PersonalDoorVisible() || Visual->PersonalDoorAngle() < 0.5)
   { Fail(TEXT("parked wagon must keep every SARO carrier part hidden and show its own open driver door")); return; }
   if (T.Stage == 101) Frame(*Car, FVector(-4.3, 0, -4.6), FVector(0, 0.8, 0));
   else Frame(*Car, FVector(-2.7, 0, -0.5), FVector(-0.65, 0.85, -0.45));
   Stage(T.Stage == 101 ? 103 : 104, 1); return;
  }
  if (T.Stage == 103)
  {
   Shot(TEXT("vehicles-wagon-outside")); Stage(102, 0.5); return;
  }
  if (T.Stage == 104)
  {
   Shot(TEXT("vehicles-wagon-door-open"));
   UE_LOG(LogTemp, Display, TEXT("S47: evening vehicles PASS parked wagon: own open leaf and body visible, complete SARO carrier group hidden; exterior camera setups after actual GetOutHere"));
   Stage(105, 0.5); return;
  }
  if (T.Stage == 105)
  {
   BootstrapTest(TEXT("parking")); Stage(106, 1); return;
  }
  if (T.Stage == 106)
  {
   FS47Truck* Service = M->World->SaroTruck();
   const auto& Box = M->Player->TruckBox;
   if (Commute() || !Pack.Flag(TEXT("intro_arrived")) || !Service || Service->IsPersonalVehicle() || !Service->Group.Shown() || !Service->Body.Shown() || !Service->Shell.Shown() || Service->Cab.Shown() || !Service->Position.Equals(FVector(8.9, 0, 8), 0.01) || FMath::Abs(Service->Rotation.X) > 0.001 || FMath::Abs(Service->Rotation.Z) > 0.001 || !Box || Box->Area != TEXT("saro") || FMath::Abs(Box->X - Service->Position.X) > 0.01 || FMath::Abs(Box->Z - Service->Position.Z) > 0.01)
   { Fail(TEXT("SARO 07 did not regain its real body/cab mode, level original pad or matching service collider")); return; }
   if (!Visual->PersonalVisible() || !Visual->PersonalDoorVisible() || !M->Player->PersonalCarBox || FMath::Abs(M->Player->PersonalCarBox->X - Visual->ParkedPosition().X) > 0.01 || FMath::Abs(M->Player->PersonalCarBox->Z - Visual->ParkedPosition().Z) > 0.01)
   { Fail(TEXT("SARO visitor wagon body, leaf or collider missing/mismatched")); return; }
   Frame(*Service, FVector(-4.4, 0, -4.5), FVector(0, 0.9, 0));
   UE_LOG(LogTemp, Display, TEXT("S47: evening vehicles PASS SARO 07 restored: body/shell visible, cab interior hidden on foot, original pad %.3f %.3f %.3f pitch/roll %.6f %.6f, matching truck collider; separate wagon collider matches visible visitor bay"), Service->Position.X, Service->Position.Y, Service->Position.Z, Service->Rotation.X, Service->Rotation.Z);
   Stage(107, 1); return;
  }
  if (T.Stage == 107) { Shot(TEXT("vehicles-service-truck")); Stage(108, 1); return; }
  if (T.Stage == 108)
  {
   FS47Truck* Service = M->World->SaroTruck();
   if (!Service) { Fail(TEXT("service truck missing")); return; }
   // South of both bays, facing the separate private wagon and the complete service truck.
   Frame(*Service, FVector(14, 0, -16), FVector(10.5, 0.7, -3));
   Stage(109, 1); return;
  }
  if (T.Stage == 109) { Shot(TEXT("vehicles-saro-parking")); Stage(110, 1); return; }
  if (T.Stage == 110)
  {
   if (!T.Shots.IsEmpty())
    for (const TCHAR* Name : {TEXT("driver-seat"), TEXT("wagon-outside"), TEXT("wagon-door-open"), TEXT("service-truck"), TEXT("saro-parking")})
    {
     const FString Path = T.Shots / (FString(TEXT("entrance-vehicles-")) + Name + TEXT(".png"));
     if (IFileManager::Get().FileSize(*Path) < 1000) { Fail(TEXT("vehicle screenshot missing/empty: ") + Path); return; }
    }
   T.bDone = true;
   UE_LOG(LogTemp, Display, TEXT("S47: evening vehicles PASS COMPLETE: coherent personal body, own open door and driver seat, whole SARO carrier hidden while driving/on foot, level service truck on original pad, matching separate colliders, five runtime captures; parking/camera setups use API, human input UNVERIFIED"));
   FPlatformMisc::RequestExit(false); return;
  }
  return;
 }
 if (T.Stage == 0)
 {
  if (!Pack.Flag(TEXT("intro_arrived")) || Commute() || M->World->Area != TEXT("saro") || M->World->bDriving) { Fail(TEXT("start with -S47Evening=parking")); return; }
  T.Park = Visual->ParkedPosition(); T.Heading = Visual->ParkedHeading();
  M->Player->Place(7.5, 1.6, UE_DOUBLE_PI / 2);
  UE_LOG(LogTemp, Display, TEXT("S47: evening entrance START: 8 conditions, before/after phone, parked/moved wagon, live/fresh disk and full-world load; physical bidirectional movement, three portal lanes; human input UNVERIFIED"));
  Stage(1); return;
 }
 if (T.Stage == 1)
 {
  const bool Moved = (T.Condition % 2) != 0;
  Visual->ParkPersonal(Moved ? FVector(-12, -0.62, 14) : T.Park, T.Heading);
  M->Web->SetDoorOpen(TEXT("doorEast"), true);
  if (T.Condition % 4 >= 2)
  {
   const FString StoreDir = FPaths::ProjectSavedDir() / TEXT("EveningEntranceControls") / FString::FromInt(T.Condition);
   FS47SaveStore Writer(StoreDir); Writer.Init(); FS47SaveCard Card;
   Card.Chapter = TEXT("Opening entrance collision control"); Card.Place = TEXT("SARO"); Card.Clock = M->Story->ClockText(false);
   FString Error; const FVector Before = M->Player->Pos; const double Clock = M->Story->Clock();
   const auto Meta = Writer.Save(1, TEXT("manual"), 0, M->Flow->GameState(), Card, Error);
   if (!Meta) { Fail(TEXT("disk save: ") + Error); return; }
   FS47SaveStore Reader(StoreDir); Reader.Init(); const auto Saved = Reader.Load(Meta->Id);
   if (!Saved) { Fail(TEXT("fresh store failed to read disk save")); return; }
   if (!T.Shots.IsEmpty()) FFileHelper::SaveStringToFile(S47J::Write(Saved, true), *(T.Shots / FString::Printf(TEXT("entrance-%d.disk-save.json"), T.Condition)));
   US47Flow::FStart Restore; Restore.State = Saved; Restore.CaseId = 0; Restore.bDev = true; Restore.Playtime = M->Playtime;
   M->Flow->StartGame(Restore);
   if (!M->Player->Pos.Equals(Before, 0.015) || M->Story->Clock() != Clock || Pack.Flag(TEXT("handover_done")) != (T.Condition >= 4) || !M->Web->IsDoorOpen(TEXT("doorEast")))
   {
    UE_LOG(LogTemp, Error, TEXT("S47: entrance normal restore diagnostics condition %d: expected pose %.9f %.9f %.9f, actual %.9f %.9f %.9f; expected clock %.9f, actual %.9f; expected handover %d, actual %d; east open %d, actual-pose walkable %d"), T.Condition,
     Before.X, Before.Y, Before.Z, M->Player->Pos.X, M->Player->Pos.Y, M->Player->Pos.Z, Clock, M->Story->Clock(), T.Condition >= 4, Pack.Flag(TEXT("handover_done")), M->Web->IsDoorOpen(TEXT("doorEast")), M->Player->Walkable(M->Player->Pos.X, M->Player->Pos.Z));
    Fail(TEXT("disk/world restore changed pose, clock, handover or open door")); return;
   }
   UE_LOG(LogTemp, Display, TEXT("S47: evening entrance PASS fresh FS47SaveStore disk load and full Flow.StartGame restore, condition %d"), T.Condition);
  }
  Stage(2, 1); return;
 }
 if (T.Stage == 2)
 {
  const FVector Expected = (T.Condition % 2) ? FVector(-12, -0.62, 14) : T.Park;
  if (!Visual->ParkedPosition().Equals(Expected, 0.01)) { Fail(TEXT("wagon pose did not retain its parked/moved condition")); return; }
  if (!M->Web->ToggleDoor(TEXT("doorEast")) || M->Web->IsDoorOpen(TEXT("doorEast")) || !M->Web->ToggleDoor(TEXT("doorEast")) || !M->Web->IsDoorOpen(TEXT("doorEast")))
  { Fail(TEXT("ordinary close/reopen must succeed with the player clear of the entrance at x7.5")); return; }
  ++T.OutsideToggles;
  UE_LOG(LogTemp, Display, TEXT("S47: evening entrance PASS ordinary door close/reopen outside its threshold, condition %d"), T.Condition);
  T.Path.Reset(); T.Waypoint = 0;
  // The opened east leaf occupies z1.08..1.18: include its 0.27 m player clearance.
  for (double Z : {1.48, 1.60, 1.78})
  {
   T.Path.Add(FVector(7.5, 0, Z)); T.Path.Add(FVector(6.1, 0, Z)); T.Path.Add(FVector(5.2, 0, Z)); T.Path.Add(FVector(4.2, 0, Z));
   T.Path.Add(FVector(5.2, 0, Z)); T.Path.Add(FVector(6.1, 0, Z)); T.Path.Add(FVector(7.5, 0, Z));
  }
  UE_LOG(LogTemp, Display, TEXT("S47: evening entrance condition %d begin: %s phone, wagon %s, %s load"), T.Condition, T.Condition < 4 ? TEXT("before") : TEXT("after"), (T.Condition % 2) ? TEXT("moved") : TEXT("parked"), (T.Condition % 4) >= 2 ? TEXT("after disk/world") : TEXT("live before"));
  Stage(3); return;
 }
 if (T.Stage == 3)
 {
  if (T.Elapsed - T.Since > 45) { Fail(TEXT("physical bidirectional entrance walking blocked")); return; }
  const int32 Previous = T.Waypoint;
  if (!Walk())
  {
   if (T.Waypoint != Previous && (Previous % 7 == 1 || Previous % 7 == 5))
   {
    // This is reached by real walker movement, so ToggleDoor reads the actual eye.
    if (M->Web->ToggleDoor(TEXT("doorEast")) || !M->Web->IsDoorOpen(TEXT("doorEast")) || !M->Player->Walkable(M->Player->Pos.X, M->Player->Pos.Z))
    { Fail(TEXT("manual close in the occupied threshold must be refused without disabling its only walk zone")); return; }
    ++T.CloseRefusals;
    UE_LOG(LogTemp, Display, TEXT("S47: evening entrance PASS refused manual close in occupied threshold at %.3f %.3f, condition %d"), M->Player->Pos.X, M->Player->Pos.Z, T.Condition);
    if (!T.bThresholdRestored && T.Condition == 0)
    {
     const FVector Before = M->Player->Pos; const double Clock = M->Story->Clock(); const auto DoorsBefore = M->Web->DoorStates();
     M->Player->TestMove.Reset();
     // Reproduce the previously possible invalid save through a forced API setup.
     // The normal manual close above has already been proved to refuse this state.
     M->Web->SetDoorOpen(TEXT("doorEast"), false);
     if (M->Player->Walkable(Before.X, Before.Z)) { Fail(TEXT("forced legacy threshold setup did not reproduce the missing walk zone")); return; }
     const FString StoreDir = FPaths::ProjectSavedDir() / TEXT("EveningEntranceControls") / TEXT("legacy-closed-threshold");
     FS47SaveStore Writer(StoreDir); Writer.Init(); FS47SaveCard Card;
     Card.Chapter = TEXT("Legacy closed entrance recovery control"); Card.Place = TEXT("SARO threshold"); Card.Clock = M->Story->ClockText(false);
     FString Error; const auto Meta = Writer.Save(1, TEXT("manual"), 0, M->Flow->GameState(), Card, Error);
     if (!Meta) { Fail(TEXT("legacy closed-threshold disk save: ") + Error); return; }
     FS47SaveStore Reader(StoreDir); Reader.Init(); const auto Saved = Reader.Load(Meta->Id);
     if (!Saved) { Fail(TEXT("fresh store failed to read the legacy closed-threshold save")); return; }
     if (!T.Shots.IsEmpty()) FFileHelper::SaveStringToFile(S47J::Write(Saved, true), *(T.Shots / TEXT("entrance-legacy-closed-threshold.disk-save.json")));
     US47Flow::FStart Restore; Restore.State = Saved; Restore.CaseId = 0; Restore.bDev = true; Restore.Playtime = M->Playtime;
     M->Flow->StartGame(Restore);
     if (!M->Player->Pos.Equals(Before, 0.015) || M->Story->Clock() != Clock || !M->Web->IsDoorOpen(TEXT("doorEast")) || !M->Player->Walkable(Before.X, Before.Z) || Pack.Flag(TEXT("handover_done")))
     {
      UE_LOG(LogTemp, Error, TEXT("S47: entrance legacy restore diagnostics: expected pose %.9f %.9f %.9f, actual %.9f %.9f %.9f; expected clock %.9f, actual %.9f; east open %d, expected-pose walkable %d, actual-pose walkable %d, handover %d, opening %d, area %s, enabled %d"),
       Before.X, Before.Y, Before.Z, M->Player->Pos.X, M->Player->Pos.Y, M->Player->Pos.Z,
       Clock, M->Story->Clock(), M->Web->IsDoorOpen(TEXT("doorEast")), M->Player->Walkable(Before.X, Before.Z), M->Player->Walkable(M->Player->Pos.X, M->Player->Pos.Z), Pack.Flag(TEXT("handover_done")), Opening(), *M->World->Area, Enabled());
      Fail(TEXT("legacy closed-threshold restore must reopen only the occupied door and preserve pose, clock and pre-phone state")); return;
     }
     const auto DoorsAfter = M->Web->DoorStates();
     for (const auto& Pair : DoorsBefore)
      if (Pair.Key != TEXT("east") && (!DoorsAfter.Contains(Pair.Key) || DoorsAfter.FindRef(Pair.Key) != Pair.Value))
      { Fail(TEXT("legacy threshold recovery changed an unrelated door: ") + Pair.Key); return; }
     T.bThresholdRestored = true; T.Hold = 0.5;
     UE_LOG(LogTemp, Display, TEXT("S47: evening entrance PASS reproduced old closed-threshold non-walkable state, saved to disk, loaded through fresh store/full world: occupied doorway reopened, exact pose/clock retained and walkability recovered at %.3f %.3f"), Before.X, Before.Z);
    }
   }
   if (T.Waypoint != Previous && (Previous % 7 == 3 || Previous % 7 == 6))
   {
    ++T.Passes;
    UE_LOG(LogTemp, Display, TEXT("S47: evening entrance PASS crossing %d condition %d %s at %.3f %.3f"), T.Passes, T.Condition, Previous % 7 == 3 ? TEXT("in") : TEXT("out"), M->Player->Pos.X, M->Player->Pos.Z);
    if (Previous == 3 || Previous == 6)
    {
     // Keep the actual walked position and face the passage for both evidence views.
     M->Player->Yaw = FMath::Atan2(M->Player->Pos.X - 6.1, 0.0); M->Player->Pitch = -0.06;
     Shot(FString::Printf(TEXT("%d-%s"), T.Condition, Previous == 3 ? TEXT("inside") : TEXT("outside"))); T.Hold = 0.3;
    }
   }
   return;
  }
  ++T.Condition;
  if (T.Condition == 4)
  {
   M->Player->Place(4.1, 1.0, 0); Answer(); Stage(4, 0.2); return;
  }
  if (T.Condition < 8) { Stage(1); return; }
  Visual->ParkPersonal(T.Park, T.Heading); M->Player->TestMove.Reset(); T.bDone = true;
  if (T.Passes != 48 || T.CloseRefusals != 48 || T.OutsideToggles != 8 || !T.bThresholdRestored) { Fail(TEXT("missing physical crossings, occupied-close refusals, outside toggles or legacy threshold recovery")); return; }
  UE_LOG(LogTemp, Display, TEXT("S47: evening entrance PASS COMPLETE: 48 physical crossings and occupied-close refusals, 8 clear-threshold close/reopens, 8 conditions before/after phone with parked/moved wagon and fresh disk/full-world load, legacy closed-threshold save recovered without pose/clock change; human input UNVERIFIED"));
  FPlatformMisc::RequestExit(false); return;
 }
 if (T.Stage == 4)
 {
  if (T.Elapsed - T.Since > 30) { Fail(TEXT("phone API control timed out")); return; }
  if (Voice) Voice->Stop(); M->UI->Close(true); bDialogueOpen = false;
  if (Pack.CurrentLine()) { Pack.Advance(); Changed(); T.Hold = 0.02; return; }
  if (const auto* Scene = Pack.CurrentScene())
  {
   const FString Id = Scene->Id == TEXT("D01_HANDOVER") ? TEXT("D01_CH_NOFOOD") : Scene->Id == TEXT("D01_NOFOOD") ? TEXT("D01_CH_NOFOOD_BRIEF") : FString();
   if (Id.IsEmpty() || !Pack.Select(Id)) { Fail(TEXT("unexpected phone choice control")); return; }
   Changed(); T.Hold = 0.02; return;
  }
  AfterDialogue();
  if (!Pack.Flag(TEXT("handover_done")) || Opening() || M->Story->bOpeningHeld) { Fail(TEXT("phone did not complete handover")); return; }
  M->Player->Place(7.5, 1.6, UE_DOUBLE_PI / 2); Stage(1, 0.3); return;
 }
}
