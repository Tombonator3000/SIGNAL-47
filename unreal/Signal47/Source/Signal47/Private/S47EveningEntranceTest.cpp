#include "S47Evening.h"

#include "S47EveningVisual.h"
#include "S47Flow.h"
#include "S47Main.h"
#include "S47RecordedDialogue.h"
#include "S47Story.h"
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
 bool bEnabled = false, bDone = false, bVehicles = false, bThresholdRestored = false;
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
  EntranceTest->bEnabled = EntranceTest->bVehicles || FParse::Param(FCommandLine::Get(), TEXT("S47EveningEntrance"));
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
 if (T.Elapsed > 300) { Fail(TEXT("300 s control timeout")); return; }
 if (M->World->bBusy) return;
 if (T.Hold > 0) { T.Hold -= Dt; return; }
 const auto Stage = [&T](int32 Next, double Hold = 0) { T.Stage = Next; T.Since = T.Elapsed; T.Hold = Hold; };
 const auto Shot = [&T](const FString& Name)
 {
  if (T.Shots.IsEmpty()) return;
  const FString Path = T.Shots / (TEXT("entrance-") + Name + TEXT(".png"));
  FScreenshotRequest::RequestScreenshot(Path, true, false);
  UE_LOG(LogTemp, Display, TEXT("S47: entrance screenshot requested with HUD: %s"), *Path);
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
   Frame(*Service, FVector(7.4, 0, -12), FVector(1.9, 0.7, -3));
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
