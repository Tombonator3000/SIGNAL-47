#include "S47Evening.h"
#include "S47Audio.h"
#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47RecordedDialogue.h"
#include "S47Story.h"
#include "S47Substation.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

// Opt-in live API/disk contract controls. D01 choices and dial values use real APIs;
// field/return locations are explicit fixtures, not a measured physical travel route.
// Journal, service and lamp actions must resolve real Inter rays before OnUse.
struct FS47Evening::FServiceTest
{
 bool bEnabled = false, bDone = false;
 int32 Stage = 0, Checks = 0, Loads = 0, Invalid = 0;
 double Elapsed = 0, Since = 0, Hold = 3, LastClock = 0;
 FString Root, NormalStamp;
 FVector Target = FVector::ZeroVector;
 S47J::FObj BeforeCalibration, Pending, Filed;
};

void FS47Evening::ControlServiceTest(double Dt)
{
 if (!ServiceTest)
 {
  ServiceTest = MakeShared<FServiceTest>();
  ServiceTest->bEnabled = FParse::Param(FCommandLine::Get(), TEXT("S47T01Contract"));
 }
 FServiceTest& T = *ServiceTest;
 if (!T.bEnabled || T.bDone) return;
 const auto Fail = [&T](const FString& Why)
 {
  T.bDone = true;
  UE_LOG(LogTemp, Error, TEXT("S47: T01 contract FAIL stage %d: %s"), T.Stage, *Why);
  FPlatformMisc::RequestExit(false);
 };
 if (!M || !M->Flow || !M->Story || !M->World || !M->Player || !M->Web || !M->UI || !M->Inter || !M->Substation)
 { Fail(TEXT("missing runtime dependency")); return; }
 if (!M->Flow->bDevRun || M->Flow->CaseId != 0)
 { Fail(TEXT("requires a developer run without S47Case; never operate on a real active case")); return; }
 T.Elapsed += Dt;
 if (T.Elapsed > 180) { Fail(TEXT("180 s control timeout")); return; }
 if (M->Flow->bStarting || M->World->bBusy) return;
 if (T.Stage == 31 && M->Story->Clock() + .001 < T.LastClock)
 { Fail(TEXT("ordinary survey/residual boundary moved the story clock backwards")); return; }
 T.LastClock = M->Story->Clock();
 if (T.Hold > 0) { T.Hold -= Dt; return; }
 const auto Stage = [&T](int32 Next, double Hold = .15)
 { T.Stage = Next; T.Since = T.Elapsed; T.Hold = Hold; };
 const auto Check = [&T, &Fail](bool Good, const FString& What)
 {
  ++T.Checks;
  if (!Good) { Fail(What); return false; }
  UE_LOG(LogTemp, Display, TEXT("S47: T01 contract PASS: %s"), *What);
  return true;
 };
 const auto Filed = [this] { return S47J::Bool(Save(), TEXT("serviceFiled")); };
 const auto Card = [this](const TCHAR* Id)
 {
  const FS47Doc* D = M->Story->Docs().FindByPredicate([Id](const FS47Doc& It) { return It.Id == Id; });
  return D ? D->Title + TEXT("\n") + D->Kind + TEXT("\n") + D->Stamp + TEXT("\n") + D->Page + TEXT("\n") + D->Transcript + TEXT("\n") + D->Image : FString();
 };
 const auto SavedCard = [](const S47J::FObj& State, const TCHAR* Id)
 {
  for (const auto& V : S47J::Arr(S47J::Obj(State, TEXT("routine")), TEXT("docs")))
  {
   const auto D = V->AsObject();
   if (D && S47J::Str(D, TEXT("id")) == Id)
    return S47J::Str(D, TEXT("title")) + TEXT("\n") + S47J::Str(D, TEXT("kind")) + TEXT("\n") + S47J::Str(D, TEXT("stamp")) + TEXT("\n") + S47J::Str(D, TEXT("page")) + TEXT("\n") + S47J::Str(D, TEXT("transcript")) + TEXT("\n") + S47J::Str(D, TEXT("image"));
  }
  return FString();
 };
 const auto NotesCount = [this] { return M->Story->Notes().Num(); };
 const auto ValidDials = [this]
 {
  auto& R = M->Story->Rx; const auto& P = R.Profile();
  R.Frequency = P.F; R.Gain = (P.Gain[0] + P.Gain[1]) * .5;
  R.Bandwidth = (P.Bw[0] + P.Bw[1]) * .5; R.Azimuth = (P.Az[0] + P.Az[1]) * .5;
  M->Story->ConsoleAzimuth();
 };
 const auto Load = [this, &T, &Fail, &Check, &Card, &SavedCard, &Filed](const FString& Name, const S47J::FObj& Source, bool WantPending, bool WantFiled, bool WantNormal)
 {
  if (!Source || S47J::Str(Source, TEXT("checkpoint")) != TEXT("routine"))
  { Fail(TEXT("disk fixture is not an actual routine GameState: ") + Name); return false; }
  const auto State = S47J::Clone(Source);
  const auto Pose = S47J::Obj(State, TEXT("pose"));
  const double Clock = S47J::Num(S47J::Obj(State, TEXT("routine")), TEXT("clock"));
  const FString Area = S47J::Str(State, TEXT("area")), TruckAt = S47J::Str(State, TEXT("truck"));
  const auto TruckPose = S47J::Obj(State, TEXT("truckPose")), Odo = S47J::Obj(State, TEXT("odo"));
  const FString Journal = SavedCard(State, TEXT("EV_TRAFO_CARD"));
  const FString Result = SavedCard(State, TEXT("EV_TRAFO_CHECK"));
  const auto Notes = S47J::Strs(S47J::Obj(State, TEXT("routine")), TEXT("notes"));
  FS47SaveStore Writer(T.Root / Name); Writer.Init();
  FS47SaveCard MetaCard; MetaCard.Chapter = TEXT("T01 API/disk contract"); MetaCard.Place = Area; MetaCard.Clock = S47Text::ClockText(Clock, true); MetaCard.Playtime = M->Playtime;
  FString Error; const auto Meta = Writer.Save(1, TEXT("manual"), 0, State, MetaCard, Error);
  if (!Meta) { Fail(TEXT("isolated disk write failed: ") + Name + TEXT(": ") + Error); return false; }
  FS47SaveStore Reader(T.Root / Name); Reader.Init(); const auto Disk = Reader.Load(Meta->Id);
  if (!Disk) { Fail(TEXT("fresh store could not read: ") + Name); return false; }
  FFileHelper::SaveStringToFile(S47J::Write(Disk, true), *(T.Root / (Name + TEXT(".loaded.json"))));
  US47Flow::FStart Restore; Restore.State = Disk; Restore.CaseId = 0; Restore.bDev = true; Restore.Playtime = M->Playtime;
  M->Flow->StartGame(Restore);
  ++T.Loads;
  const bool PoseOk = Pose && FMath::Abs(M->Player->Pos.X - S47J::Num(Pose, TEXT("x"))) < .02 &&
   FMath::Abs(M->Player->Pos.Z - S47J::Num(Pose, TEXT("z"))) < .02 &&
   FMath::Abs(M->Player->Yaw - S47J::Num(Pose, TEXT("yaw"))) < .001 && FMath::Abs(M->Player->Pitch - S47J::Num(Pose, TEXT("pitch"))) < .001;
  const auto NewTruck = M->World->TruckSave(), NewOdo = M->World->OdoSave();
  const auto NewTruckPose = S47J::Obj(NewTruck, TEXT("pose"));
  bool TruckOk = TruckPose && NewTruckPose && S47J::Str(NewTruck, TEXT("at")) == TruckAt;
  for (const TCHAR* K : {TEXT("x"), TEXT("y"), TEXT("z"), TEXT("h"), TEXT("p"), TEXT("r")})
   TruckOk &= FMath::Abs(S47J::Num(TruckPose, K) - S47J::Num(NewTruckPose, K)) < .001;
  bool OdoOk = Odo && NewOdo;
  for (const TCHAR* K : {TEXT("m"), TEXT("x"), TEXT("s")})
   OdoOk &= S47J::Has(Odo, K) == S47J::Has(NewOdo, K) && FMath::Abs(S47J::Num(Odo, K) - S47J::Num(NewOdo, K)) < .000001;
  return Check(M->Flow->CaseId == 0 && M->Flow->bDevRun && M->World->Area == Area && PoseOk && TruckOk && OdoOk &&
   M->Story->Clock() == Clock && M->Story->Notes() == Notes && Card(TEXT("EV_TRAFO_CARD")) == Journal && Card(TEXT("EV_TRAFO_CHECK")) == Result &&
   ServicePending() == WantPending && Filed() == WantFiled && M->Substation->HasNormalResult() == WantNormal,
   TEXT("fresh disk/full-Flow restore: ") + Name + TEXT(" preserves player/truck/odometer, time, papers and expected credit"));
 };
 const auto Pick = [this, &Fail](const TCHAR* Id, const FVector& Target)
 {
  const FVector Eye = M->Player->EyeUE(), At = S47Three::ToUE(Target);
  const FString Actual = M->Inter->Pick(Eye, (At - Eye).GetSafeNormal());
  if (Actual != Id || !M->Player->OnUse)
  { Fail(FString::Printf(TEXT("physical use ray for %s picked '%s' or has no OnUse"), Id, *Actual)); return false; }
  M->Player->OnUse(Actual); return true;
 };
 const auto AllowSurvey = [this, &Check, &ValidDials](const FString& Name)
 {
  if (!Check(!ServicePending() && M->Story->Phase() == ES47Phase::Shift && M->Story->Rx.Stage == 1, Name + TEXT(" has no retroactive service gate"))) return false;
  const double Clock = M->Story->Clock(); const int32 Notes = M->Story->Notes().Num();
  ValidDials(); M->Story->ConsoleAction();
  return Check(M->Story->Rx.Stage == 2 && M->Story->Phase() == ES47Phase::Survey && M->Story->Clock() == Clock && M->Story->Notes().Num() == Notes + 1,
   Name + TEXT(" permits the ordinary survey transition exactly once"));
 };

 if (T.Stage == 0)
 {
  FString UserDir; FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir);
  UserDir = FPaths::ConvertRelativePathToFull(UserDir); FPaths::CollapseRelativeDirectories(UserDir);
  const bool Safe = (UserDir.StartsWith(TEXT("/tmp/")) || UserDir.StartsWith(TEXT("/var/tmp/"))) &&
   FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()), UserDir);
  if (!Safe) { Fail(TEXT("requires an explicit isolated UserDir under /tmp or /var/tmp")); return; }
  if (FParse::Param(FCommandLine::Get(), TEXT("S47SubstationTest")) || FParse::Param(FCommandLine::Get(), TEXT("S47SubstationTestLoad")))
  { Fail(TEXT("do not combine two independent substation controllers")); return; }
  T.Root = FPaths::ProjectSavedDir() / TEXT("T01Contract") / FGuid::NewGuid().ToString(EGuidFormats::Digits);
  IFileManager::Get().MakeDirectory(*T.Root, true);
  if (!Check(Opening() && !Commute() && Pack.CurrentScene() && Pack.CurrentScene()->Id == TEXT("D01_HANDOVER") && !ServicePending() &&
    S47J::Int(Save(), TEXT("serviceRouteVersion")) == 1, TEXT("fresh night has no T01 gate before actual D01 completion; phone setup is a fixture"))) return;
  Stage(1); return;
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
   if (Reply.IsEmpty() || !Pack.Select(Reply)) { Fail(TEXT("unexpected D01 menu")); return; }
   M->UI->Close(true); bDialogueOpen = false; Changed(); ShowDialogue(); T.Hold = .1; return;
  }
  if (!Check(Pack.Flag(TEXT("handover_done")) && ServicePending() && M->Story->Phase() == ES47Phase::Shift && !Opening(), TEXT("new case receives T01 only after D01's real completion transaction"))) return;
  M->Story->Use(TEXT("rack")); Stage(2, 2.2); return;
 }
 if (T.Stage == 2)
 {
  if (!Check(M->Story->BankIn() && M->Story->Rx.Stage == 0, TEXT("ordinary rack power remains available"))) return;
  Use(TEXT("logbook"));
  if (!Check(ServicePending() && !Filed(), TEXT("folder filing cannot self-declare a missing physical normal result"))) return;
  T.BeforeCalibration = S47J::Clone(M->Flow->GameState());
  ValidDials(); M->Story->ConsoleAction();
  if (!Check(M->Story->Rx.Stage == 1 && M->Story->Phase() == ES47Phase::Shift && ServicePending(), TEXT("RX calibration 0 to 1 remains usable before T01"))) return;
  ValidDials(); const double Clock = M->Story->Clock(); const auto Notes = M->Story->Notes(); const auto Phase = M->Story->Phase(); const int32 Log = M->Story->Night().Log.Num();
  M->Story->ConsoleAction(); M->Story->ConsoleAction(); M->Story->ConsoleClosed();
  if (!Check(M->Story->Rx.Stage == 1 && M->Story->Phase() == Phase && M->Story->Clock() == Clock && M->Story->Notes() == Notes && M->Story->Night().Log.Num() == Log && ServicePending(),
    TEXT("RX1 survey refusal adds no note, power log, time jump or phase change, including repeated action"))) return;
  T.Pending = S47J::Clone(M->Flow->GameState());
  // Fork the actual earlier snapshot to cover a player doing T01 before calibration.
  // This is a deliberate save reload, not a claim of monotonic time across test cases.
  if (!Load(TEXT("fork-before-calibration"), T.BeforeCalibration, true, false, false)) return;
  if (!Check(M->Story->Rx.Stage == 0, TEXT("actual pre-calibration snapshot restores RX0 without changing the service assignment"))) return;
  M->World->PlaceTruck(TEXT("diner")); M->World->Enter(TEXT("diner")); M->Player->Place(7885, 525.8, UE_DOUBLE_PI); M->Player->Pitch = -.08;
  UE_LOG(LogTemp, Display, TEXT("S47: T01 contract field position is a labelled fixture; no road travel claim"));
  Stage(3, .3); return;
 }
 if (T.Stage == 3)
 {
  if (!Pick(TEXT("substation:service"), FS47Substation::Site(FVector(5.52, -9.10, 1.42)))) return;
  if (!Check(!M->Substation->HasJournal() && !M->Substation->HasNormalResult(), TEXT("physical service action cannot bypass the journal"))) return;
  if (!Pick(TEXT("substation:journal"), FS47Substation::Site(FVector(4.83, -8.99, .77)))) return;
  M->UI->Close(true);
  if (!Check(M->Substation->HasJournal() && !M->Substation->HasNormalResult(), TEXT("actual journal pick retains the complaint without claiming a result"))) return;
  if (!Load(TEXT("journal"), M->Flow->GameState(), true, false, false)) return;
  Stage(4, .35); return;
 }
 if (T.Stage == 4)
 {
  if (!Pick(TEXT("substation:service"), FS47Substation::Site(FVector(5.52, -9.10, 1.42)))) return;
  if (!Check(M->Inter->LabelOf(TEXT("substation:service")) == TEXT("Service check running") && !M->Substation->HasNormalResult(), TEXT("actual local check has started but is not yet evidence"))) return;
  if (!Load(TEXT("mid-check"), M->Flow->GameState(), true, false, false)) return;
  Stage(5, .35); return;
 }
 if (T.Stage == 5)
 {
  if (!Check(M->Inter->LabelOf(TEXT("substation:service")) == TEXT("Run local service check"), TEXT("Continue during an unfinished check is retryable rather than a fabricated completion"))) return;
  // Hold an existing authored scene through the physical result. This is an API
  // conversation-state fixture, not a claim that its recorded voice was played.
  if (!Check(Pack.Start(TEXT("D12_POWER_WALKIE")) && Pack.NextLine() == 0, TEXT("an existing authored scene can own Pack during the local check"))) return;
  if (!Pick(TEXT("substation:service"), FS47Substation::Site(FVector(5.52, -9.10, 1.42)))) return;
  Stage(6, .1); return;
 }
 if (T.Stage == 6)
 {
  if (M->Inter->LabelOf(TEXT("substation:service")) != TEXT("Service check complete / compare the lamp"))
  { if (T.Elapsed - T.Since > 8) Fail(TEXT("real local service check did not complete")); return; }
  if (!Check(!M->Substation->HasNormalResult(), TEXT("completed panel without physical lamp comparison is not a normal result"))) return;
  if (!Pick(TEXT("substation:lamp"), FS47Substation::Site(FVector(4.62, -9.01, 1.70)))) return;
  M->UI->Close(true);
  if (!Check(M->Substation->HasNormalResult() && !Pack.Flag(TEXT("trafo_normal_checked")) && ServicePending() && Pack.CurrentScene() &&
    Pack.CurrentScene()->Id == TEXT("D12_POWER_WALKIE") && Pack.NextLine() == 0, TEXT("real lamp result cannot overwrite another authored scene or its cursor"))) return;
  while (Pack.CurrentLine()) Pack.Advance();
  M->Substation->Step(0); const int32 Papers = Pack.Documents().Num(); M->Substation->Step(0);
  if (!Check(Pack.Flag(TEXT("trafo_normal_checked")) && !Pack.CurrentScene() && Pack.Documents().Num() == Papers && ServicePending(),
    TEXT("idle production sync acknowledges the genuine physical result once without filing it at SARO"))) return;
  T.NormalStamp = M->Substation->NormalCompletionStamp();
  if (!Load(TEXT("observed-at-site"), M->Flow->GameState(), true, false, true)) return;
  Stage(7, .35); return;
 }
 if (T.Stage == 7)
 {
  Use(TEXT("logbook"));
  if (!Check(ServicePending() && !Filed(), TEXT("filing API refuses a logbook ID used from the field"))) return;
  M->World->PlaceTruck(TEXT("saro")); M->World->Enter(TEXT("saro"));
  FS47Obj Book;
  if (const auto* I = M->Inter->Get(TEXT("logbook"))) Book = I->Object;
  else if (const auto* Item = M->Web->Item(TEXT("logbook"))) Book = M->Web->ObjAt(Item->Node);
  if (!Book.IsValid()) { Fail(TEXT("real shift-folder object is missing")); return; }
  const FBox Bounds = Book.WorldBox(); T.Target = Bounds.IsValid ? Bounds.GetCenter() : Book.WorldPosition();
  bool Placed = false;
  for (double Radius : {.9, 1.2, 1.5, 1.8}) for (int32 I = 0; I < 24 && !Placed; ++I)
  {
   const double A = I * UE_DOUBLE_PI / 12; const FVector P(T.Target.X + FMath::Cos(A) * Radius, 0, T.Target.Z + FMath::Sin(A) * Radius);
   if (!M->Web->Rect(TEXT("room.bounds")).Contains(P) || !M->Player->Walkable(P.X, P.Z)) continue;
   bool Blocked = false;
   for (const auto& C : M->Web->Colliders()) if (C.bEnabled && P.X + M->Player->Radius > C.MinX && P.X - M->Player->Radius < C.MaxX && P.Z + M->Player->Radius > C.MinZ && P.Z - M->Player->Radius < C.MaxZ) { Blocked = true; break; }
   if (!Blocked) { M->Player->Place(P.X, P.Z, 0); Placed = true; }
  }
  if (!Placed) { Fail(TEXT("no collider-clear folder fixture inside the actual control room")); return; }
  UE_LOG(LogTemp, Display, TEXT("S47: T01 contract return position is a labelled fixture; filing still requires a real folder ray"));
  Stage(8, .35); return;
 }
 if (T.Stage == 8)
 {
  const FString Result = Card(TEXT("EV_TRAFO_CHECK")); const int32 Before = NotesCount();
  if (!Pick(TEXT("logbook"), T.Target)) return; M->UI->Close(true);
  if (!Check(!ServicePending() && Filed() && NotesCount() == Before + 1 && Card(TEXT("EV_TRAFO_CHECK")) == Result && M->Substation->NormalCompletionStamp() == T.NormalStamp,
    TEXT("one real SARO folder filing opens the survey and preserves physical result/time"))) return;
  if (!Pick(TEXT("logbook"), T.Target)) return; M->UI->Close(true);
  if (!Check(NotesCount() == Before + 1, TEXT("reopening the same folder does not duplicate filing credit or require another acknowledgement"))) return;
  if (!Check(M->Story->Rx.Stage == 0, TEXT("physical T01 can be completed before local RX calibration"))) return;
  if (!Load(TEXT("filed-rx0"), M->Flow->GameState(), false, true, true)) return;
  ValidDials(); const double Clock = M->Story->Clock(); M->Story->ConsoleAction();
  if (!Check(M->Story->Rx.Stage == 1 && M->Story->Phase() == ES47Phase::Shift && M->Story->Clock() == Clock && !ServicePending(), TEXT("ordinary RX0 calibration remains available after a filed T01 restore"))) return;
  T.Filed = S47J::Clone(M->Flow->GameState());
  if (!Load(TEXT("filed-rx1"), T.Filed, false, true, true)) return;
  Stage(10, .3); return;
 }
 if (T.Stage == 10)
 {
  if (!AllowSurvey(TEXT("completed physical T01 + one filing"))) return;
  // Deliberately inconsistent snapshots test proof validation and restore ordering.
  // The positive source is the real result just observed and filed, never fake true flags.
  auto Bad = S47J::Clone(T.Filed); Bad->RemoveField(TEXT("substation"));
  if (!Load(TEXT("bad-missing-local-after-valid-case"), Bad, true, false, false)) return;
  Stage(11, .3); return;
 }
 if (T.Stage == 11)
 {
  if (!Load(TEXT("valid-local-after-invalid-case"), T.Filed, false, true, true)) return;
  Stage(12, .3); return;
 }
 if (T.Stage == 12)
 {
  auto Bad = S47J::Clone(T.Filed); const auto Local = S47J::Obj(Bad, TEXT("substation")); FString Name;
  if (T.Invalid == 0) { S47J::Set(Local, TEXT("v"), 999); Name = TEXT("bad-local-version"); }
  else if (T.Invalid == 1) { Local->RemoveField(TEXT("completedAt")); Name = TEXT("bad-missing-completion-time"); }
  else if (T.Invalid == 2) { S47J::Set(Local, TEXT("completedStamp"), FString()); Name = TEXT("bad-empty-completion-stamp"); }
  else if (T.Invalid == 3) { Local->RemoveField(TEXT("observedAt")); Name = TEXT("bad-missing-observation-time"); }
  else { S47J::Set(Local, TEXT("observed"), false); Name = TEXT("bad-unobserved-local"); }
  if (!Load(Name, Bad, true, false, false)) return;
  ++T.Invalid; Stage(T.Invalid < 5 ? 12 : 13, .3); return;
 }
 if (T.Stage == 13)
 {
  auto Bad = S47J::Clone(T.Filed); const auto E = S47J::Obj(Bad, TEXT("evening"));
  S47J::Set(S47J::Obj(S47J::Obj(E, TEXT("pack")), TEXT("flags")), TEXT("trafo_normal_checked"), false);
  if (!Load(TEXT("bad-filed-without-authored-result"), Bad, true, false, true)) return;
  Stage(20, .3); return;
 }
 if (T.Stage >= 20 && T.Stage <= 23)
 {
  auto Old = S47J::Clone(T.Pending); const FString Name = T.Stage == 20 ? TEXT("old-no-route-version") : T.Stage == 21 ? TEXT("old-unknown-route-version") : T.Stage == 22 ? TEXT("old-zero-route-version") : TEXT("past-opening-no-evening-state");
  if (T.Stage == 20) { const auto E = S47J::Obj(Old, TEXT("evening")); E->RemoveField(TEXT("serviceRouteVersion")); E->RemoveField(TEXT("serviceFiled")); }
  else if (T.Stage == 21 || T.Stage == 22) S47J::Set(S47J::Obj(Old, TEXT("evening")), TEXT("serviceRouteVersion"), T.Stage == 21 ? 999 : 0);
  else Old->RemoveField(TEXT("evening"));
  if (!Load(Name, Old, false, false, false) || !AllowSurvey(Name)) return;
  Stage(T.Stage + 1, .3); return;
 }
 if (T.Stage == 24)
 {
  US47Flow::FStart Legacy; Legacy.Jump = TEXT("shift"); Legacy.CaseId = 0; Legacy.bDev = true;
  M->Flow->StartGame(Legacy);
  if (!Check(!M->Story->bEveningTimeline && !ServicePending(), TEXT("actual legacy shift API start never gains T01"))) return;
  if (!Load(TEXT("valid-filed-after-legacy-case"), T.Filed, false, true, true)) return;
  Stage(30, .3); return;
 }
 if (T.Stage == 30)
 {
  // Forward-only boundary setup exercises the existing late-survey reservation.
  // This is not a claim about natural player pacing or elapsed story hours.
  M->Story->SetClock(FMath::Max(M->Story->Clock(), S47Timeline::Residual(true) - 6));
  if (!AllowSurvey(TEXT("restored valid T01 at the reserved residual boundary"))) return;
  T.LastClock = M->Story->Clock(); M->Story->ConsoleClosed();
  UE_LOG(LogTemp, Display, TEXT("S47: T01 contract waiting for the original real-time relay/dip/survey timers, no direct TimeSkip call"));
  Stage(31, .1); return;
 }
 if (T.Stage == 31 && M->Story->bResidualVisible)
 {
  if (!Check(M->Story->At(ES47Phase::Residual) && M->Story->Clock() >= S47Timeline::Residual(true) - .2 && !ServicePending() && Filed() && M->Substation->HasNormalResult(),
    TEXT("original relay/dip/residual route resumes without time reversal or lost T01 evidence"))) return;
  T.bDone = true;
  UE_LOG(LogTemp, Display, TEXT("S47: T01 contract PASS COMPLETE: %d API assertions, %d actual fresh-store/full-Flow disk restores; field/return placement are fixtures, full travel and human input UNVERIFIED; artifacts %s"), T.Checks, T.Loads, *T.Root);
  FPlatformMisc::RequestExit(false);
 }
}
