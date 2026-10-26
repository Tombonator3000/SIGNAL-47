#include "S47Evening.h"
#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47RecordedDialogue.h"
#include "S47Story.h"
#include "S47Substation.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

// DRAFT: a separate native-route control, never run with S47T01Contract.
// D01/initial player placement and native trip target are explicit fixtures.
// After START there is no Place/Enter/MoveDriven/MoveParked or intermediate load.
// All steering is existing World.Autopilot; foot inputs mirror existing evening tests.
struct FS47Evening::FServiceDriveTest
{
 bool bEnabled = false, bDone = false, bStarted = false, bReturn = false;
 bool bOutSaro = false, bOutRoad = false, bBackRoad = false, bBackSaro = false;
 int32 Stage = 0, Waypoint = 0, Checks = 0;
 double Elapsed = 0, Since = 0, Hold = 3, LastClock = 0, OdoStart = 0;
 double OutMetres = 0, BackMetres = 0, WalkMetres = 0;
 FString Root, LastLeg, LastFootArea, LocalRecord;
 FVector LastTruck = FVector::ZeroVector, LastFoot = FVector::ZeroVector;
 FVector FieldTruck = FVector::ZeroVector, FieldEgress = FVector::ZeroVector;
 TArray<FVector> Path;
};

void FS47Evening::ControlServiceDriveTest(double Dt)
{
 if (!ServiceDriveTest)
 {
  ServiceDriveTest = MakeShared<FServiceDriveTest>();
  ServiceDriveTest->bEnabled = FParse::Param(FCommandLine::Get(), TEXT("S47T01DriveContract"));
 }
 auto& T = *ServiceDriveTest;
 if (!T.bEnabled || T.bDone) return;
 const auto Fail = [this, &T](const FString& Why)
 {
  T.bDone = true;
  if (M && M->Player) M->Player->TestMove.Reset();
  if (M && M->World) { M->World->Autopilot.Reset(); M->World->TestInput = nullptr; }
  UE_LOG(LogTemp, Error, TEXT("S47: T01 native route FAIL stage %d: %s"), T.Stage, *Why);
  FPlatformMisc::RequestExit(false);
 };
 if (!M || !M->Flow || !M->Story || !M->World || !M->Player || !M->Web || !M->Inter || !M->UI || !M->Substation)
 { Fail(TEXT("missing dependency")); return; }
 if (!M->Flow->bDevRun || M->Flow->CaseId != 0)
 { Fail(TEXT("requires developer case0, no S47Case")); return; }
 T.Elapsed += Dt;
 if (T.Elapsed > 900) { Fail(TEXT("900 s native route timeout")); return; }
 const auto Check = [&T, &Fail](bool Ok, const FString& Why)
 {
  ++T.Checks;
  if (!Ok) { Fail(Why); return false; }
  UE_LOG(LogTemp, Display, TEXT("S47: T01 native route PASS: %s"), *Why); return true;
 };
 const auto Stage = [&T](int32 Next, double Hold = .15)
 {
  T.Stage = Next; T.Since = T.Elapsed; T.Hold = Hold;
  UE_LOG(LogTemp, Display, TEXT("S47: T01 native route stage %d at %.1f s"), Next, T.Elapsed);
 };
 const auto Path = [&T](TArray<FVector> Points) { T.Path = MoveTemp(Points); T.Waypoint = 0; };
 const auto Walk = [this, &T, &Fail]()
 {
  if (T.Elapsed - T.Since > 90) { Fail(TEXT("physical waypoint walk blocked; no relocation fallback")); return false; }
  if (!T.Path.IsValidIndex(T.Waypoint)) { M->Player->TestMove.Reset(); return true; }
  const FVector D = T.Path[T.Waypoint] - M->Player->Pos;
  const double Far = FMath::Sqrt(D.X * D.X + D.Z * D.Z);
  if (Far < .15) { ++T.Waypoint; M->Player->TestMove.Reset(); return false; }
  M->Player->Yaw = FMath::Atan2(-D.X, -D.Z); M->Player->Pitch = -.06;
  M->Player->TestMove = FVector2D(0, -FMath::Min(1.0, Far * 2)); return false;
 };
 const auto ObjectTarget = [this](const TCHAR* Id)
 {
  FS47Obj O;
  if (const auto* I = M->Inter->Get(Id)) O = I->Object;
  else if (const auto* Item = M->Web->Item(Id)) O = M->Web->ObjAt(Item->Node);
  const FBox B = O.IsValid() ? O.WorldBox() : FBox(ForceInit);
  return B.IsValid ? B.GetCenter() : O.IsValid() ? O.WorldPosition() : FVector::ZeroVector;
 };
 const auto Pick = [this, &Fail](const TCHAR* Id, const FVector& At, bool bUse)
 {
  const FVector Eye = M->Player->EyeUE();
  const FString Actual = M->Inter->Pick(Eye, (S47Three::ToUE(At) - Eye).GetSafeNormal());
  if (Actual != Id || (bUse && !M->Player->OnUse))
  { Fail(FString::Printf(TEXT("actual ray wanted %s but got '%s'"), Id, *Actual)); return false; }
  if (bUse) M->Player->OnUse(Actual);
  return true;
 };
 const auto Persist = [this, &T, &Fail](const TCHAR* Name)
 {
  FS47SaveStore Writer(T.Root / Name); Writer.Init();
  FS47SaveCard Card; Card.Chapter = TEXT("T01 native route"); Card.Place = M->World->Area; Card.Clock = M->Story->ClockText(false); Card.Playtime = M->Playtime;
  FString Error; const auto Meta = Writer.Save(1, TEXT("manual"), 0, M->Flow->GameState(), Card, Error);
  if (!Meta) { Fail(TEXT("isolated route save failed: ") + Error); return false; }
  FS47SaveStore Reader(T.Root / Name); Reader.Init(); const auto Disk = Reader.Load(Meta->Id);
  if (!Disk || !FFileHelper::SaveStringToFile(S47J::Write(Disk, true), *(T.Root / (FString(Name) + TEXT(".disk.json")))))
  { Fail(TEXT("fresh store read/export failed")); return false; }
  return true;
 };

 if (T.bStarted)
 {
  if (M->Story->Clock() + .001 < T.LastClock) { Fail(TEXT("clock moved backwards during continuous route")); return; }
  T.LastClock = M->Story->Clock();
  if (M->World->bDriving)
  {
   const auto* Truck = M->World->CabTruck();
   if (!Truck) { Fail(TEXT("driving without native cab truck")); return; }
   if (M->World->Leg == T.LastLeg)
   {
    const double Moved = FVector::Dist(Truck->Position, T.LastTruck);
    if (Moved > FMath::Max(6.0, FMath::Abs(M->World->DrivingSpeed()) * Dt + 3)) { Fail(TEXT("non-native truck discontinuity inside a leg")); return; }
    (T.bReturn ? T.BackMetres : T.OutMetres) += Moved;
   }
   T.LastLeg = M->World->Leg; T.LastTruck = Truck->Position; T.LastFootArea.Empty();
   if (T.bReturn) { T.bBackRoad |= T.LastLeg == TEXT("road"); T.bBackSaro |= T.LastLeg == TEXT("saro"); }
   else { T.bOutSaro |= T.LastLeg == TEXT("saro"); T.bOutRoad |= T.LastLeg == TEXT("road"); }
  }
  else
  {
   T.LastLeg.Empty();
   if (!M->World->bBusy && M->World->Area == T.LastFootArea)
   {
    const double Moved = FVector2D::Distance(FVector2D(M->Player->Pos.X, M->Player->Pos.Z), FVector2D(T.LastFoot.X, T.LastFoot.Z));
    if (Moved > FMath::Max(3.0, Dt * 12)) { Fail(TEXT("walker discontinuity during the measured route")); return; }
    if (M->Player->TestMove && Moved > .001)
    {
     FHitResult Hit;
     const bool Ground = M->Web->GetWorld()->LineTraceSingleByChannel(Hit,
      S47Three::ToUE(FVector(M->Player->Pos.X, M->Player->FloorY + .3, M->Player->Pos.Z)),
      S47Three::ToUE(FVector(M->Player->Pos.X, M->Player->FloorY - 4, M->Player->Pos.Z)), ECC_Visibility);
     if (!Ground || FMath::Abs(S47Three::ToThree(Hit.ImpactPoint).Y - M->Player->FloorY) > .35)
     { Fail(TEXT("actual waypoint walk lost its rendered floor")); return; }
    }
    T.WalkMetres += Moved;
   }
   T.LastFootArea = M->World->Area; T.LastFoot = M->Player->Pos;
  }
 }
 if (M->Flow->bStarting || M->World->bBusy) return;
 if (T.Hold > 0) { T.Hold -= Dt; return; }

 if (T.Stage == 0)
 {
  FString UserDir; FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir);
  UserDir = FPaths::ConvertRelativePathToFull(UserDir); FPaths::CollapseRelativeDirectories(UserDir);
  if (!(UserDir.StartsWith(TEXT("/tmp/")) || UserDir.StartsWith(TEXT("/var/tmp/"))) ||
      !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()), UserDir))
  { Fail(TEXT("requires explicit isolated UserDir under /tmp or /var/tmp")); return; }
  for (const TCHAR* Flag : {TEXT("S47T01Contract"), TEXT("S47SubstationTest"), TEXT("S47SubstationTestLoad"), TEXT("S47EveningEntranceTest"), TEXT("S47SmokeTest")})
   if (FParse::Param(FCommandLine::Get(), Flag)) { Fail(TEXT("do not combine control drivers")); return; }
  FString Other;
  if (FParse::Value(FCommandLine::Get(), TEXT("S47EveningRoute="), Other) || FParse::Value(FCommandLine::Get(), TEXT("S47Drive="), Other))
  { Fail(TEXT("do not combine route/trip drivers")); return; }
  if (!Check(Opening() && !Commute() && Pack.CurrentScene() && Pack.CurrentScene()->Id == TEXT("D01_HANDOVER"), TEXT("phone/D01 start is an explicit fixture"))) return;
  T.Root = FPaths::ProjectSavedDir() / TEXT("T01NativeRoute") / FGuid::NewGuid().ToString(EGuidFormats::Digits);
  IFileManager::Get().MakeDirectory(*T.Root, true); Stage(1); return;
 }
 if (T.Stage == 1)
 {
  if (Pack.CurrentLine())
  {
   if (Voice) Voice->Stop(); M->UI->Close(true); bDialogueOpen = false;
   Pack.Advance(); Changed(); ShowDialogue(); T.Hold = .1; return;
  }
  if (const auto* S = Pack.CurrentScene())
  {
   const FString Reply = S->Id == TEXT("D01_HANDOVER") ? TEXT("D01_CH_NOFOOD") : S->Id == TEXT("D01_NOFOOD") ? TEXT("D01_CH_NOFOOD_BRIEF") : FString();
   if (Reply.IsEmpty() || !Pack.Select(Reply)) { Fail(TEXT("unexpected D01 fixture menu")); return; }
   M->UI->Close(true); bDialogueOpen = false; Changed(); ShowDialogue(); T.Hold = .1; return;
  }
  if (!Check(Pack.Flag(TEXT("handover_done")) && ServicePending() && M->Story->Phase() == ES47Phase::Shift && M->World->TruckAt == TEXT("saro") && !M->World->bCommuteVehicle,
    TEXT("D01 real transaction released service truck and new T01 assignment"))) return;
  // The single initial player placement is before START; no vehicle placement is made here.
  M->Player->Place(10.8, 8, 0); M->Player->Pitch = -.08; Stage(2, .35); return;
 }
 if (T.Stage == 2)
 {
  if (!Pick(TEXT("truck"), ObjectTarget(TEXT("truck")), false) || !Check(M->Inter->LabelOf(TEXT("truck")) == TEXT("Drive / Caliche Junction inspection"), TEXT("real service truck/range/label is available at the measured start"))) return;
  T.bStarted = true; T.LastClock = M->Story->Clock(); T.OdoStart = M->World->Odometer();
  UE_LOG(LogTemp, Display, TEXT("S47: T01 native route START: D01/player-start and station01 target are fixtures; existing native autopilot, physics/transfer/parking and actual foot uses thereafter; manual Roam/human input UNVERIFIED"));
  // Roam deliberately has no native target. Reuse existing station route and stop
  // along its real track; this is a labelled API target, not a new steering algorithm.
  M->World->Autopilot = 10; M->World->DriveOut(false); Stage(3, .6); return;
 }
 if (T.Stage == 3)
 {
  if (!M->World->bDriving) { Fail(TEXT("outbound native trip ended before Caliche stop")); return; }
  const auto* Truck = M->World->CabTruck();
  if (M->World->Leg == TEXT("road") && Truck->Position.X < 7950) M->World->Autopilot = 4;
  if (M->World->Leg != TEXT("road") || Truck->Position.X > 7900) return;
  if (!Check(Truck->Position.Z > 495 && Truck->Position.Z < 530 && T.bOutSaro && T.bOutRoad && T.OutMetres > 500,
    TEXT("native SARO/highway/track trip physically reached the marked-front approach"))) return;
  M->World->Autopilot = 0; Stage(4, .1); return;
 }
 if (T.Stage == 4)
 {
  if (!M->World->bDriving || T.Elapsed - T.Since > 15) { Fail(TEXT("native approach braking failed")); return; }
  if (FMath::Abs(M->World->DrivingSpeed()) > .1 || !M->World->CanGetOut()) return;
  T.FieldTruck = M->World->CabTruck()->Position;
  if (!Check(FVector2D::Distance(FVector2D(T.FieldTruck.X, T.FieldTruck.Z), FVector2D(7885, 525.8)) < 28 && M->World->GetOutHere(), TEXT("actual stopped truck permits normal GetOutHere near front"))) return;
  Stage(5, .65); return;
 }
 if (T.Stage == 5)
 {
  const auto Truck = M->World->TruckSave(), Pose = S47J::Obj(Truck, TEXT("pose"));
  if (!Check(!M->World->bDriving && M->World->TruckAt == TEXT("diner") && M->World->Area == TEXT("diner") && Pose &&
   FMath::Abs(S47J::Num(Pose, TEXT("x")) - T.FieldTruck.X) < .02 && FMath::Abs(S47J::Num(Pose, TEXT("z")) - T.FieldTruck.Z) < .02,
   TEXT("native Park retains actual Caliche truck position and physical door egress"))) return;
  T.FieldEgress = M->Player->Pos; Path({FVector(7885, 0, 520.8), FVector(7885, 0, 525.8)}); Stage(6); return;
 }
 if (T.Stage == 6)
 {
  if (!Walk()) return;
  if (!Pick(TEXT("substation:journal"), FS47Substation::Site(FVector(4.83, -8.99, .77)), true)) return;
  M->UI->Close(true);
  if (!Check(M->Substation->HasJournal() && !M->Substation->HasNormalResult(), TEXT("continuous physical front walk and actual journal use"))) return;
  if (!Pick(TEXT("substation:service"), FS47Substation::Site(FVector(5.52, -9.10, 1.42)), true)) return;
  Stage(7, .1); return;
 }
 if (T.Stage == 7)
 {
  if (M->Inter->LabelOf(TEXT("substation:service")) != TEXT("Service check complete / compare the lamp"))
  { if (T.Elapsed - T.Since > 8) Fail(TEXT("real local check did not complete")); return; }
  if (!Pick(TEXT("substation:lamp"), FS47Substation::Site(FVector(4.62, -9.01, 1.70)), true)) return;
  M->UI->Close(true);
  if (!Check(M->Substation->HasNormalResult() && Pack.Flag(TEXT("trafo_normal_checked")) && ServicePending(), TEXT("actual completed panel and lamp comparison before return"))) return;
  T.LocalRecord = S47J::Write(M->Substation->Save());
  if (!Persist(TEXT("actual-observed-route"))) return;
  Path({FVector(7885, 0, 520.8), T.FieldEgress}); Stage(8); return;
 }
 if (T.Stage == 8)
 {
  if (!Walk()) return;
  if (!Pick(TEXT("dinerTruck"), ObjectTarget(TEXT("dinerTruck")), false)) return;
  T.bReturn = true; M->World->Autopilot = 10; M->World->DriveBack(); Stage(9, .6); return;
 }
 if (T.Stage == 9)
 {
  if (M->World->bDriving) return;
  Stage(10, .65); return; // normal Park's delayed handover has to run
 }
 if (T.Stage == 10)
 {
  if (!Check(M->World->TruckAt == TEXT("saro") && M->World->Area == TEXT("saro") && M->World->TruckOnLot() &&
   T.bBackRoad && T.bBackSaro && T.BackMetres > 500 && M->World->Odometer() > T.OdoStart + (T.OutMetres + T.BackMetres) / 1609.344 - .05,
   TEXT("native reverse track/highway/SARO pad trip parked normally with real odometer gain"))) return;
  const auto& Body = M->Player->TruckBox;
  if (!Body || Body->Area != TEXT("saro")) { Fail(TEXT("native return did not publish the actual parked truck collider")); return; }
  // Native SARO parking faces north; DoorSpot may put us on either side.
  // Stay on that actual egress side until north of the truck before crossing.
  // West lane 7.35 also clears the retained pad post at x6.72..6.98,z11.47..11.73.
  const bool West = M->Player->Pos.X < Body->X;
  const double LaneX = West ? 7.35 : 11.5;
  const TArray<FVector> ExitPath = {FVector(LaneX, 0, M->Player->Pos.Z), FVector(LaneX, 0, 4),
   FVector(9.8, 0, 4), FVector(9.8, 0, 1.6), FVector(7.5, 0, 1.6)};
  FVector From = M->Player->Pos;
  for (const FVector& To : ExitPath)
  {
   const int32 Samples = FMath::Max(1, FMath::CeilToInt(FVector2D::Distance(FVector2D(From.X, From.Z), FVector2D(To.X, To.Z)) / .15));
   for (int32 Sample = 0; Sample <= Samples; ++Sample)
   {
    const FVector At = FMath::Lerp(From, To, double(Sample) / Samples);
    if (!M->Player->Walkable(At.X, At.Z) || M->Player->InTruck(At.X, At.Z))
    { Fail(TEXT("actual return egress side has no clear finite route around the parked truck")); return; }
   }
   From = To;
  }
  UE_LOG(LogTemp, Display, TEXT("S47: T01 native route returning on actual %s egress side: player %.3f %.3f, truck %.3f %.3f heading %.3f, lane %.2f"),
   West ? TEXT("west") : TEXT("east"), M->Player->Pos.X, M->Player->Pos.Z, Body->X, Body->Z, Body->Heading, LaneX);
  Path(ExitPath); Stage(11); return;
 }
 if (T.Stage == 11)
 {
  if (!Walk()) return;
  if (!M->Web->IsDoorOpen(TEXT("doorEast")) && !Pick(TEXT("doorEast"), ObjectTarget(TEXT("doorEast")), true)) return;
  Path({FVector(5.2, 0, 1.6), FVector(4.1, 0, 1), FVector(3.6, 0, 1.3)}); Stage(12, .8); return;
 }
 if (T.Stage == 12)
 {
  if (!Walk()) return;
  const int32 Notes = M->Story->Notes().Num();
  if (!Pick(TEXT("logbook"), ObjectTarget(TEXT("logbook")), true)) return;
  M->UI->Close(true);
  if (!Check(!ServicePending() && S47J::Bool(Save(), TEXT("serviceFiled")) && M->Story->Notes().Num() == Notes + 1 &&
   S47J::Write(M->Substation->Save()) == T.LocalRecord && M->Story->Phase() == ES47Phase::Shift && T.WalkMetres > 20,
   TEXT("normal return egress/doorway/room walk and one actual folder filing retain the same physical record"))) return;
  if (!Persist(TEXT("actual-filed-route"))) return;
  T.bDone = true;
  UE_LOG(LogTemp, Display, TEXT("S47: T01 native route PASS COMPLETE: %d checks, %.1f outbound m, %.1f return m, %.1f walked m; no post-START placement or intermediate load; target selection/D01/start are fixtures, manual Roam and human input UNVERIFIED; artifacts %s"), T.Checks, T.OutMetres, T.BackMetres, T.WalkMetres, *T.Root);
  FPlatformMisc::RequestExit(false);
 }
}
