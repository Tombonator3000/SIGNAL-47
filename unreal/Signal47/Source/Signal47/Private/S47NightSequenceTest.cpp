#include "S47Evening.h"

#include "S47Album.h"
#include "S47Anomaly.h"
#include "S47FieldCamera.h"
#include "S47Flow.h"
#include "S47Horror.h"
#include "S47HorrorSC03.h"
#include "S47HorrorThreat.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Observing.h"
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

// Isolert API-/diskkontrakt for den nye nattrekkefølgen. D01 bruker den faktiske
// pakketransaksjonen. SC01-fakta og ett originalfoto kommer fra en uttrykkelig
// navngitt tidligere diskprøve. Observasjonene, surveytimere, telefonkvittering,
// strømtilstand og fysiske T01-rays kjøres her. Posisjonene og road-starten er
// merkede fixtures; dette er ingen måling av manuell kjøring eller full spilletid.
struct FS47Evening::FNightSequenceTest
{
 bool bEnabled = false, bDone = false, bSC03Probe = false;
 int32 Stage = 0, Checks = 0, Loads = 0, ObserveRun = -1;
 double Elapsed = 0, Since = 0, Hold = 3, BeforeDrive = 0, SC02Clock = 0, SC03Clock = 0, LockedClock = 0, PrintedClock = 0;
 FString Root, FixtureSource, EscapeSource, NormalStamp;
 S47J::FObj Fixture;
 TArray<double> DishAz, DishEl;
 FS47Substation::FFaultSnapshot PausedFault;
};

void FS47Evening::ControlNightSequenceTest(double Dt)
{
 if (!NightSequenceTest)
 {
  NightSequenceTest = MakeShared<FNightSequenceTest>();
  NightSequenceTest->bEnabled = FParse::Param(FCommandLine::Get(), TEXT("S47NightSequenceContract"));
 }
 FNightSequenceTest& T = *NightSequenceTest;
 if (!T.bEnabled || T.bDone) return;
 const auto Fail = [&T](const FString& Why)
 {
  T.bDone = true;
  UE_LOG(LogTemp, Error, TEXT("S47: night sequence contract FAIL stage %d: %s"), T.Stage, *Why);
  if (!T.Root.IsEmpty())
  {
   auto R = S47J::New(); S47J::Set(R, TEXT("status"), TEXT("FAIL")); S47J::Set(R, TEXT("stage"), T.Stage);
   S47J::Set(R, TEXT("reason"), Why); S47J::Set(R, TEXT("checks"), T.Checks); S47J::Set(R, TEXT("diskFlowRestores"), T.Loads);
   FFileHelper::SaveStringToFile(S47J::Write(R, true), *(T.Root / TEXT("result.json")));
  }
  FPlatformMisc::RequestExit(false);
 };
 if (!M || !M->Flow || !M->Story || !M->World || !M->Player || !M->Web || !M->UI || !M->Inter ||
     !M->Substation || !M->Horror || !M->Night || !M->Night->Album || !M->FieldCamera || !M->Anomaly || !M->Observing)
 { Fail(TEXT("mangler runtimeavhengighet")); return; }
 if (!M->Flow->bDevRun || M->Flow->CaseId != 0)
 { Fail(TEXT("krever utviklerkjøring uten S47Case; personlige saker skal aldri brukes")); return; }
 T.Elapsed += Dt;
 if (T.Elapsed > 360) { Fail(TEXT("360 sekunders kontrakttidsgrense")); return; }
 if (M->Flow->bStarting || M->World->bBusy) return;
 if (T.Hold > 0) { T.Hold -= Dt; return; }
 const auto Stage = [&T](int32 Next, double Hold = .15)
 { T.Stage = Next; T.Since = T.Elapsed; T.Hold = Hold; };
 const auto Check = [&T, &Fail](bool Good, const FString& What)
 {
  ++T.Checks;
  if (!Good) { Fail(What); return false; }
  UE_LOG(LogTemp, Display, TEXT("S47: night sequence contract PASS: %s"), *What); return true;
 };
 const auto Dials = [this](const FS47Profile& P)
 {
  auto& R = M->Story->Rx; R.Frequency = P.F; R.Gain = (P.Gain[0] + P.Gain[1]) * .5;
  R.Bandwidth = (P.Bw[0] + P.Bw[1]) * .5; R.Azimuth = (P.Az[0] + P.Az[1]) * .5;
  M->Story->ConsoleAzimuth();
 };
 const auto Pick = [this, &Fail](const TCHAR* Id, const FVector& Target)
 {
  const FVector Eye = M->Player->EyeUE(), At = S47Three::ToUE(Target);
  const FString Actual = M->Inter->Pick(Eye, (At - Eye).GetSafeNormal());
  if (Actual != Id || !M->Player->OnUse)
  { Fail(FString::Printf(TEXT("fysisk ray til %s valgte '%s', eller mangler OnUse"), Id, *Actual)); return false; }
  M->Player->OnUse(Actual); return true;
 };
 const auto Load = [this, &T, &Fail, &Check](const FString& Name)
 {
  const auto State = M->Flow->GameState(), Routine = S47J::Obj(State, TEXT("routine")), Pose = S47J::Obj(State, TEXT("pose"));
  if (!State || !Routine || !Pose || S47J::Str(State, TEXT("checkpoint")) != TEXT("nightService"))
  { Fail(TEXT("nytt nattcheckpoint mangler faktisk rutinetilstand/spillerpose: ") + Name); return false; }
  const double Clock = M->Story->Clock(); const auto Phase = M->Story->Phase();
  const bool Out = FeederOutage(), Called = FeederCallAnswered(), Drove = DriveTimeSkipped();
  const bool Normal = M->Substation->HasNormalResult(), K3 = M->Story->Night().bK3Open;
  const auto Notes = M->Story->Notes(); const int32 LogCount = M->Story->Night().Log.Num();
  const int32 RxStage = M->Story->Rx.Stage; const bool Residual = M->Story->bResidualVisible;
  const bool Solved = M->Story->Rx.bSolved, HasSolve = M->Story->Night().SolveAt.IsSet(), HasPrint = M->Story->Night().PrintoutAt.IsSet();
  const double SolveAt = HasSolve ? *M->Story->Night().SolveAt : 0, PrintAt = HasPrint ? *M->Story->Night().PrintoutAt : 0;
  auto Docs = S47J::New(); Docs->SetArrayField(TEXT("docs"), S47J::Arr(Routine, TEXT("docs"))); const FString Papers = S47J::Write(Docs);
  const FString LocalRecord = S47J::Write(M->Substation->Save());
  const FString Area = S47J::Str(State, TEXT("area"));
  const bool SavedPoseWalkableBefore = M->Player->Walkable(S47J::Num(Pose, TEXT("x")), S47J::Num(Pose, TEXT("z")));
  FS47SaveStore Writer(T.Root / Name); Writer.Init();
  FS47SaveCard Card; Card.Chapter = TEXT("Nattrekkefølge / isolert kontrakt"); Card.Place = Area;
  Card.Clock = M->Story->ClockText(true); Card.Playtime = M->Playtime;
  FString Error; const auto Meta = Writer.Save(1, TEXT("manual"), 0, State, Card, Error);
  if (!Meta) { Fail(TEXT("isolert disklagring feilet: ") + Error); return false; }
  FS47SaveStore Reader(T.Root / Name); Reader.Init(); const auto Disk = Reader.Load(Meta->Id);
  if (!Disk) { Fail(TEXT("ny leser kunne ikke lese nattlagringen: ") + Name); return false; }
  if (!FFileHelper::SaveStringToFile(S47J::Write(Disk, true), *(T.Root / (Name + TEXT(".loaded.json")))))
  { Fail(TEXT("kunne ikke skrive beviskopi av disklesingen")); return false; }
  US47Flow::FStart Restore; Restore.State = Disk; Restore.CaseId = 0; Restore.bDev = true; Restore.Playtime = M->Playtime;
  M->Flow->StartGame(Restore); ++T.Loads;
  const bool PoseOk = FMath::Abs(M->Player->Pos.X - S47J::Num(Pose, TEXT("x"))) < .02 &&
   FMath::Abs(M->Player->Pos.Z - S47J::Num(Pose, TEXT("z"))) < .02 &&
   FMath::Abs(M->Player->Yaw - S47J::Num(Pose, TEXT("yaw"))) < .001 &&
   FMath::Abs(M->Player->Pitch - S47J::Num(Pose, TEXT("pitch"))) < .001;
  auto AfterDocs = S47J::New(); AfterDocs->SetArrayField(TEXT("docs"), S47J::Arr(M->Story->SaveRoutine(), TEXT("docs")));
  const auto& Night = M->Story->Night();
  bool Good = true; auto Fields = S47J::New();
  const auto Field = [&Fields, &Good, &Name](const TCHAR* Key, bool Match)
  {
   S47J::Set(Fields, Key, Match); Good &= Match;
   if (!Match) UE_LOG(LogTemp, Error, TEXT("S47: night sequence restore mismatch %s: %s"), *Name, Key);
  };
  Field(TEXT("developerRun"), M->Flow->bDevRun);
  Field(TEXT("caseId"), M->Flow->CaseId == 0);
  Field(TEXT("eveningNightSequence"), NightSequence());
  Field(TEXT("storyNightSequence"), M->Story->NightSequence());
  Field(TEXT("area"), M->World->Area == Area);
  Field(TEXT("pose"), PoseOk);
  Field(TEXT("poseX"), FMath::Abs(M->Player->Pos.X - S47J::Num(Pose, TEXT("x"))) < .02);
  Field(TEXT("poseZ"), FMath::Abs(M->Player->Pos.Z - S47J::Num(Pose, TEXT("z"))) < .02);
  Field(TEXT("poseYaw"), FMath::Abs(M->Player->Yaw - S47J::Num(Pose, TEXT("yaw"))) < .001);
  Field(TEXT("posePitch"), FMath::Abs(M->Player->Pitch - S47J::Num(Pose, TEXT("pitch"))) < .001);
  Field(TEXT("clock"), M->Story->Clock() == Clock);
  Field(TEXT("phase"), M->Story->Phase() == Phase);
  Field(TEXT("rxStage"), M->Story->Rx.Stage == RxStage);
  Field(TEXT("residualVisible"), M->Story->bResidualVisible == Residual);
  Field(TEXT("k3Open"), Night.bK3Open == K3);
  Field(TEXT("rxSolved"), M->Story->Rx.bSolved == Solved);
  Field(TEXT("hasSolveAt"), Night.SolveAt.IsSet() == HasSolve);
  Field(TEXT("solveAt"), !HasSolve || (Night.SolveAt.IsSet() && *Night.SolveAt == SolveAt));
  Field(TEXT("hasPrintoutAt"), Night.PrintoutAt.IsSet() == HasPrint);
  Field(TEXT("printoutAt"), !HasPrint || (Night.PrintoutAt.IsSet() && *Night.PrintoutAt == PrintAt));
  Field(TEXT("documents"), S47J::Write(AfterDocs) == Papers);
  Field(TEXT("feederOutage"), FeederOutage() == Out);
  Field(TEXT("feederCallAnswered"), FeederCallAnswered() == Called);
  Field(TEXT("driveTimeSkipped"), DriveTimeSkipped() == Drove);
  Field(TEXT("dark"), M->Story->bDark == Out);
  Field(TEXT("hasNormalResult"), M->Substation->HasNormalResult() == Normal);
  Field(TEXT("substationRecord"), S47J::Write(M->Substation->Save()) == LocalRecord);
  Field(TEXT("notes"), M->Story->Notes() == Notes);
  Field(TEXT("logCount"), Night.Log.Num() == LogCount);
  if (!Good)
  {
   auto Details = S47J::New(), Runtime = S47J::New();
   S47J::Set(Details, TEXT("name"), Name); S47J::Set(Details, TEXT("checks"), Fields);
   S47J::Set(Details, TEXT("savedPoseWalkableBefore"), SavedPoseWalkableBefore);
   S47J::Set(Details, TEXT("savedPoseWalkableAfter"), M->Player->Walkable(S47J::Num(Pose, TEXT("x")), S47J::Num(Pose, TEXT("z"))));
   S47J::Set(Runtime, TEXT("developerRun"), M->Flow->bDevRun); S47J::Set(Runtime, TEXT("caseId"), M->Flow->CaseId);
   S47J::Set(Runtime, TEXT("phase"), int32(M->Story->Phase())); S47J::Set(Runtime, TEXT("expectedPhase"), int32(Phase));
   S47J::Set(Runtime, TEXT("dark"), M->Story->bDark); S47J::Set(Runtime, TEXT("expectedDark"), Out);
   S47J::Set(Runtime, TEXT("x"), M->Player->Pos.X); S47J::Set(Runtime, TEXT("z"), M->Player->Pos.Z);
   S47J::Set(Runtime, TEXT("yaw"), M->Player->Yaw); S47J::Set(Runtime, TEXT("pitch"), M->Player->Pitch);
   S47J::Set(Details, TEXT("actualRuntime"), Runtime);
   const auto Actual = M->Flow->GameState();
   const bool WroteExpected = FFileHelper::SaveStringToFile(S47J::Write(State, true), *(T.Root / (Name + TEXT(".expected.json"))));
   const bool WroteActual = FFileHelper::SaveStringToFile(S47J::Write(Actual, true), *(T.Root / (Name + TEXT(".actual.json"))));
   const bool WroteChecks = FFileHelper::SaveStringToFile(S47J::Write(Details, true), *(T.Root / (Name + TEXT(".checks.json"))));
   if (!(WroteExpected && WroteActual && WroteChecks)) { Fail(TEXT("kunne ikke skrive komplett expected/actual/checks-bevis ved restorefeil")); return false; }
  }
  return Check(Good,
   TEXT("ny diskleser/full Flow bevarer klokke, fase, RX-trinn/solve, papirer, K3, strøm, telefon, kjørehopp, pose og T01: ") + Name);
 };
 const auto RecordFault = [this, &T, &Fail](const FString& Name)
 {
  const auto S = M->Substation->FaultSnapshot(); auto O = S47J::New();
  S47J::Set(O, TEXT("requested"), S.bRequested); S47J::Set(O, TEXT("ready"), S.bReady); S47J::Set(O, TEXT("active"), S.bActive);
  S47J::Set(O, TEXT("arcVisible"), S.bArcVisible); S47J::Set(O, TEXT("lampsVisible"), S.bLampsVisible);
  S47J::Set(O, TEXT("materialAssigned"), S.bMaterialAssigned); S47J::Set(O, TEXT("safe"), S.bSafe); S47J::Set(O, TEXT("age"), S.Age);
  S47J::Set(O, TEXT("arcLumens"), S.ArcLumens); S47J::Set(O, TEXT("lampLumens"), S.LampLumens);
  S47J::Set(O, TEXT("arcIntensity"), S.ArcIntensity); S47J::Set(O, TEXT("lampIntensity"), S.LampIntensity);
  S47J::Set(O, TEXT("strikeBursts"), M->Substation->ArcCount());
  if (!FFileHelper::SaveStringToFile(S47J::Write(O, true), *(T.Root / (Name + TEXT(".json"))))) Fail(TEXT("kunne ikke skrive feillysbevis"));
 };
 const auto FaultOn = [this, &Check](const FString& Name)
 {
  const auto S = M->Substation->FaultSnapshot();
  return Check(S.bRequested && S.bReady && S.bActive && S.bArcVisible && S.bLampsVisible &&
   S.bMaterialAssigned && S.bSafe && S.ArcLumens > 0 && S.ArcIntensity > 0 && M->Substation->ArcCount() == 0,
   TEXT("fast feillys og rolig lysbue er aktive uten kraftig SC02-utladning: ") + Name);
 };
 const auto ThreatActive = [this] { return M->Horror->Threat && M->Horror->Threat->Active(); };

 if (T.Stage == 0)
 {
  FString UserDir; FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir);
  UserDir = FPaths::ConvertRelativePathToFull(UserDir); FPaths::CollapseRelativeDirectories(UserDir);
  if (!(UserDir.StartsWith(TEXT("/tmp/")) || UserDir.StartsWith(TEXT("/var/tmp/"))) ||
      !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()), UserDir))
  { Fail(TEXT("krever eksplisitt isolert UserDir under /tmp eller /var/tmp")); return; }
  FString OtherHorror;
  if (FParse::Value(FCommandLine::Get(), TEXT("S47HorrorContract="), OtherHorror) ||
      FParse::Param(FCommandLine::Get(), TEXT("S47T01Contract")) || FParse::Param(FCommandLine::Get(), TEXT("S47T01DriveContract")) ||
      FParse::Param(FCommandLine::Get(), TEXT("S47SubstationTest")) || FParse::Param(FCommandLine::Get(), TEXT("S47SubstationTestLoad")))
  { Fail(TEXT("skal ikke kombineres med en annen kontraktdriver")); return; }
  if (!FParse::Value(FCommandLine::Get(), TEXT("S47NightSequenceSC01="), T.FixtureSource) || T.FixtureSource.IsEmpty())
  { Fail(TEXT("krever navngitt faktisk SC01-diskfixture via S47NightSequenceSC01=/absolutt/fil.json")); return; }
  T.FixtureSource = FPaths::ConvertRelativePathToFull(T.FixtureSource);
  FString Text; if (!FFileHelper::LoadFileToString(Text, *T.FixtureSource)) { Fail(TEXT("kunne ikke lese SC01-fixture")); return; }
  T.Fixture = S47J::Read(Text);
  if (!T.Fixture || !S47J::Bool(S47J::Obj(T.Fixture, TEXT("horror")), TEXT("sc01Complete")) || !S47J::Obj(T.Fixture, TEXT("album")))
  { Fail(TEXT("navngitt kilde er ikke en fullført SC01-sak med originalalbum")); return; }
  T.Root = FPaths::ProjectSavedDir() / TEXT("NightSequenceContract") / FGuid::NewGuid().ToString(EGuidFormats::Digits);
  IFileManager::Get().MakeDirectory(*T.Root, true);
  if (!Check(Opening() && !Commute() && Pack.CurrentScene() && Pack.CurrentScene()->Id == TEXT("D01_HANDOVER") &&
      NightSequence() && S47J::Int(Save(), TEXT("serviceRouteVersion")) == 2 && !ServicePending() && !FeederOutage(),
      TEXT("ny route2 begynner uten trafotur før faktisk D01; telefonposisjon er oppstartsfixture"))) return;
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
   if (Reply.IsEmpty() || !Pack.Select(Reply)) { Fail(TEXT("uventet D01-valg")); return; }
   M->UI->Close(true); bDialogueOpen = false; Changed(); ShowDialogue(); T.Hold = .1; return;
  }
  if (!Check(Pack.Flag(TEXT("handover_done")) && !Opening() && !ServicePending() && !FeederOutage() &&
      !M->Substation->HasNormalResult() && M->Story->Phase() == ES47Phase::Shift,
      TEXT("faktisk D01 slipper kveldsskiftet uten tidlig T01-krav"))) return;
  M->Horror->Begin(nullptr, true); M->Observing->Begin(nullptr, true);
  M->Story->Use(TEXT("rack")); Stage(2, 2.3); return;
 }
 if (T.Stage == 2)
 {
  if (M->Horror->Pack.CurrentScene())
  {
   M->UI->Close(true); M->Horror->Pack.Advance(); T.Hold = .1; return;
  }
  M->UI->Close(true); Dials(S47Signal::Profiles()[0]); M->Story->ConsoleAction();
  if (!Check(M->Horror->Enabled() && !M->Horror->Complete() && M->Story->Rx.Stage == 1 && M->Horror->BaselineChecked(),
      TEXT("faktisk mottakerreferanse består mens SC01 fortsatt er ufullført"))) return;
  Stage(3); return;
 }
 if (T.Stage == 3)
 {
  auto& O = *M->Observing;
  if (O.Done()) { Stage(4); return; }
  if (!O.Active() || !FS47Observing::Runs().IsValidIndex(O.RunIndex())) { Fail(TEXT("observasjonsløypa ble ikke aktiv etter referansen")); return; }
  const auto& R = FS47Observing::Runs()[O.RunIndex()];
  if (O.Phase() == FS47Observing::EStep::Tune)
  {
   Dials(R.P); M->Story->ConsoleAction();
   if (!Check(O.Phase() == FS47Observing::EStep::Integrating && M->Story->Rx.Stage == 1,
       FString::Printf(TEXT("observasjon %d startet via beholdt ConsoleAction-krok"), O.RunIndex() + 1))) return;
   T.ObserveRun = O.RunIndex(); T.Hold = .2; return;
  }
  if (O.Phase() == FS47Observing::EStep::Integrating) return;
  if (!Check(O.RunIndex() == T.ObserveRun && O.Use(R.Data) && O.RunIndex() == T.ObserveRun + 1,
      FString::Printf(TEXT("observasjon %d arkivert via faktisk %s-API"), T.ObserveRun + 1, R.Data))) return;
  M->UI->Close(true); T.Hold = .2; return;
 }
 if (T.Stage == 4)
 {
  if (!Check(M->Observing->Done() && M->Story->Clock() >= -70 * 60 && !M->Substation->HasNormalResult(),
      TEXT("alle fire observasjoner er faktisk kjørt og arkivert innen kveldens siste datapunkt"))) return;
  Dials(S47Signal::Profiles()[1]); M->Story->ConsoleAction();
  if (!Check(M->Story->Rx.Stage == 1 && M->Story->Phase() == ES47Phase::Shift && !M->Horror->Complete(),
      TEXT("relékoblingen avvises fremdeles når SC01 ikke er fullført"))) return;
  const auto Horror = S47J::Clone(S47J::Obj(T.Fixture, TEXT("horror")));
  const FString Reference = S47J::Str(Horror, TEXT("referencePhoto"));
  auto Album = S47J::Clone(S47J::Obj(T.Fixture, TEXT("album"))); TArray<TSharedPtr<FJsonValue>> Original;
  for (const auto& V : S47J::Arr(Album, TEXT("shots")))
   if (const auto P = V->AsObject(); P && S47J::Str(P, TEXT("id")) == Reference) Original.Add(V);
  if (!Check(Original.Num() == 1, TEXT("SC01-fixture har ett identifisert originalt referansefoto; budsjettbilder importeres ikke"))) return;
  Album->SetArrayField(TEXT("shots"), Original); M->Night->Album->Restore(Album);
  M->Anomaly->Data.bEarlyCamera = true; M->FieldCamera->bHave = true;
  M->Horror->Begin(Horror, false); M->UI->Close(true);
  UE_LOG(LogTemp, Display, TEXT("S47: night sequence SC01 FIXTURE: copied completed facts and one retained original from %s; SC01 actions are not rerun"), *T.FixtureSource);
  if (!Check(M->Horror->Enabled() && M->Horror->Complete() && !M->Substation->HasNormalResult(),
      TEXT("uttrykkelig SC01-diskfixture gir kun SC01-credit, ingen T01-fullføring"))) return;
  Dials(S47Signal::Profiles()[1]); M->Story->ConsoleAction();
  if (!Check(M->Story->Rx.Stage == 2 && M->Story->Phase() == ES47Phase::Survey && !ServicePending(),
      TEXT("SC01 alene tillater relékobling uten T01 på ny route2"))) return;
  M->UI->Close(true); M->Story->ConsoleClosed(); Stage(5, .1); return;
 }
 if (T.Stage == 5)
 {
  if (!M->Story->bResidualVisible || M->Story->IsCinematic()) return;
  if (!Check(FeederOutage() && ServicePending() && M->Story->Night().bK3Open && M->Story->bDark && !M->Story->Rx.bPowered &&
      M->Story->Clock() >= S47Timeline::Residual(true) - .1 && M->Story->Clock() < 15 * 60 && !ThreatActive(),
      TEXT("vanlig ConsoleClosed/surveytimer gir 00:13-strømbrudd, åpen K3 og ingen tidlig kropp"))) return;
  M->UI->Close(true); M->Story->Use(TEXT("rack")); M->Story->Use(TEXT("crtCenter"));
  Dials(S47Signal::Profiles()[2]); M->Story->ConsoleAction();
  if (!Check(FeederOutage() && M->Story->Night().bK3Open && !M->Story->Rx.bPowered && M->Story->Rx.Stage == 2 && !M->ModalOpen(),
      TEXT("rack, død konsoll og direkte konsollhandling kan ikke omgå strømbruddet"))) return;
  T.DishAz.Reset(); T.DishEl.Reset();
  for (int32 I = 0; I < M->Web->DishCount(); ++I) { T.DishAz.Add(M->Web->DishAz(I)); T.DishEl.Add(M->Web->DishEl(I)); }
  M->Story->Rx.Azimuth += 73; M->Story->ConsoleAzimuth(); Stage(6, .5); return;
 }
 if (T.Stage == 6)
 {
  bool Still = T.DishAz.Num() == 27 && T.DishEl.Num() == 27;
  for (int32 I = 0; I < T.DishAz.Num(); ++I)
   Still &= FMath::IsNearlyEqual(T.DishAz[I], M->Web->DishAz(I), .0001) && FMath::IsNearlyEqual(T.DishEl[I], M->Web->DishEl(I), .0001);
  if (!Check(Still, TEXT("alle 27 antenner står stille etter en faktisk konsollazimuth-kommando mens matingen er ute"))) return;
  RecordFault(TEXT("outage-at-saro")); const auto Fault = M->Substation->FaultSnapshot();
  if (T.bDone || !Check(Fault.bRequested && Fault.bReady && !Fault.bActive && !Fault.bArcVisible &&
      !Fault.bLampsVisible && M->Substation->ArcCount() == 0,
      TEXT("trafostasjonens feil er aktiv i historien, men grafikken er skjult med road-området mens spilleren er ved SARO"))) return;
  M->Player->Place(4.2, 1.6, UE_DOUBLE_PI); M->Player->Pitch = -.08;
  if (!Check(M->Player->Walkable(4.2, 1.6), TEXT("telefonens eksplisitte SC01-startposisjon er faktisk walkable før diskprøven"))) return;
  UE_LOG(LogTemp, Display, TEXT("S47: night sequence phone position FIXTURE: validated walkable SC01 start at 4.2/1.6; actual phone ray and close callback follow"));
  if (!Load(TEXT("before-feeder-call"))) return;
  Stage(7, .2); return;
 }
 if (T.Stage == 7)
 {
  // Base room props are exported Web items, while authored additions use Inter registrations.
  FS47Obj Phone;
  if (const auto* Item = M->Inter->Get(TEXT("phone")); Item && Item->Object) Phone = Item->Object;
  else if (const auto* ExportedPhone = M->Web->Item(TEXT("phone"))) Phone = M->Web->ObjAt(ExportedPhone->Node);
  if (!Phone) { Fail(TEXT("telefonens fysiske mål mangler i både Inter og eksporten")); return; }
  const FBox B = Phone.WorldBox(); const FVector At = B.IsValid ? B.GetCenter() : Phone.WorldPosition();
  if (!Pick(TEXT("phone"), At)) return;
  if (!Check(M->ModalOpen() && !FeederCallAnswered() && FeederOutage(),
      TEXT("åpnet telefonsamtale kvitterer ikke matingen før faktisk dokumentlukking"))) return;
  M->UI->Close(true);
  if (!Check(FeederCallAnswered() && FeederOutage() && !DriveTimeSkipped(),
      TEXT("telefonens lukketransaksjon registrerer Caliche-oppdraget uten å gjenopprette strøm"))) return;
  if (!Load(TEXT("after-feeder-call"))) return;
  T.BeforeDrive = M->Story->Clock();
  M->World->Autopilot = 0; M->World->DriveOut(true);
  UE_LOG(LogTemp, Display, TEXT("S47: night sequence ROAD-START FIXTURE: DriveOut(true) places the live driving controller on road; no continuous SARO-to-Caliche or manual input claim"));
  Stage(8, .2); return;
 }
 if (T.Stage == 8)
 {
  if (!M->World->bDriving || M->World->Area != TEXT("road") || !DriveTimeSkipped())
  { if (T.Elapsed - T.Since > 10) Fail(TEXT("faktisk road-kjørekontroller utløste ikke 01:35-hopp")); return; }
  if (!Check(M->Story->Clock() >= 95 * 60 && M->Story->Clock() < 100 * 60 && M->Story->Clock() > T.BeforeDrive &&
      FeederOutage() && !ThreatActive(), TEXT("road-kjøring hopper frem til 01:35 med feilen og SC02-sperren bevart"))) return;
  if (!Load(TEXT("during-drive-safe-home"))) return;
  if (!Check(!M->World->bDriving && M->World->TruckAt == TEXT("saro") && M->World->Area == TEXT("saro") &&
      DriveTimeSkipped() && FeederOutage(), TEXT("Continue under kjøring bruker ordinær trygg hjemmeposisjon og beholder nattens tidshopp"))) return;
  M->World->PlaceTruck(TEXT("diner")); M->World->Enter(TEXT("diner")); M->Player->Place(7885, 525.8, UE_DOUBLE_PI); M->Player->Pitch = -.08;
  UE_LOG(LogTemp, Display, TEXT("S47: night sequence T01 field position FIXTURE: journal, enclosed check and lamp use actual interaction rays"));
  Stage(9, .3); return;
 }
 if (T.Stage == 9)
 {
  // On the new route the H02 briefing is presented at the junction, after the county call.
  if (M->Horror->Pack.CurrentScene()) { M->UI->Close(true); M->Horror->Pack.Advance(); T.Hold = .1; return; }
  RecordFault(TEXT("field-before-service")); if (T.bDone || !FaultOn(TEXT("field-before-service"))) return;
  if (!Pick(TEXT("substation:service"), FS47Substation::Site(FVector(5.52, -9.10, 1.42)))) return;
  if (!Check(!M->Substation->HasJournal() && !M->Substation->HasNormalResult() && FeederOutage(), TEXT("servicekontrollen krever fortsatt fysisk journal"))) return;
  if (!Pick(TEXT("substation:journal"), FS47Substation::Site(FVector(4.83, -8.99, .77)))) return;
  M->UI->Close(true);
  if (!Check(M->Substation->HasJournal() && !M->Substation->HasNormalResult() && !ThreatActive(), TEXT("faktisk journal gir ingen falsk T01- eller kroppscredit"))) return;
  if (!Load(TEXT("journal-at-night-site"))) return;
  Stage(10, .3); return;
 }
 if (T.Stage == 10)
 {
  if (M->Horror->Pack.CurrentScene()) { M->UI->Close(true); M->Horror->Pack.Advance(); T.Hold = .1; return; }
  if (!Pick(TEXT("substation:service"), FS47Substation::Site(FVector(5.52, -9.10, 1.42)))) return;
  if (!Check(M->Inter->LabelOf(TEXT("substation:service")) == TEXT("Service check running") && FeederOutage() && !ThreatActive(),
      TEXT("faktisk T01-kontroll starter mens feil og kraftig SC02-sperre består"))) return;
  RecordFault(TEXT("service-running")); if (T.bDone || !FaultOn(TEXT("service-running"))) return;
  FS47Doc Pause; Pause.Title = TEXT("Isolert modalpause / nattkontrakt"); Pause.Page = TEXT("Kontroll av frosset feillys og rolig lysbue.");
  M->UI->Document(Pause); Stage(11, .1); return;
 }
 if (T.Stage == 11)
 {
  T.PausedFault = M->Substation->FaultSnapshot(); Stage(12, .4); return;
 }
 if (T.Stage == 12)
 {
  const auto S = M->Substation->FaultSnapshot(); RecordFault(TEXT("modal-pause"));
  if (T.bDone || !Check(M->ModalOpen() && S.bActive && FMath::IsNearlyEqual(S.Age, T.PausedFault.Age, .0001) &&
      FMath::IsNearlyEqual(S.LampIntensity, T.PausedFault.LampIntensity, .001f) &&
      FMath::IsNearlyEqual(S.ArcLumens, T.PausedFault.ArcLumens, .001f) && M->Substation->ArcCount() == 0,
      TEXT("faktisk modalpause fryser feillysets alder/intensitet uten SC02-utladning"))) return;
  M->UI->Close(true); Stage(13, .3); return;
 }
 if (T.Stage == 13)
 {
  if (M->Inter->LabelOf(TEXT("substation:service")) != TEXT("Service check complete / compare the lamp"))
  { if (T.Elapsed - T.Since > 10) Fail(TEXT("T01-kontrollen ble ikke fullført")); return; }
  if (!Check(FeederOutage() && !M->Substation->HasNormalResult() && !ThreatActive(),
      TEXT("fullført panel alene slår ikke inn strøm eller SC02 før faktisk lampeavlesning"))) return;
  RecordFault(TEXT("panel-complete-before-lamp")); if (T.bDone || !FaultOn(TEXT("panel-complete-before-lamp"))) return;
  if (!Pick(TEXT("substation:lamp"), FS47Substation::Site(FVector(4.62, -9.01, 1.70)))) return;
  M->UI->Close(true);
  if (!Check(M->Substation->HasNormalResult(), TEXT("fysisk lampeavlesning gir faktisk validert T01-resultat"))) return;
  T.NormalStamp = M->Substation->NormalCompletionStamp(); Stage(14, .1); return;
 }
 if (T.Stage == 14)
 {
  if (FeederOutage()) { if (T.Elapsed - T.Since > 3) Fail(TEXT("faktisk T01-resultat gjenopprettet ikke matingen")); return; }
  if (!Check(!ServicePending() && !M->Story->bDark && !M->Story->Night().bK3Open && M->Story->Rx.bPowered &&
      M->Substation->HasNormalResult() && M->Substation->NormalCompletionStamp() == T.NormalStamp && !ThreatActive(),
      TEXT("T01 gjenoppretter faktisk SARO-strøm og K3 før SC02, uten returarkivering som strømport"))) return;
  RecordFault(TEXT("power-restored-local-fault-remains"));
  if (T.bDone || !FaultOn(TEXT("SARO-mating gjenopprettet; stedlig effekt beholdes"))) return;
  if (!Load(TEXT("after-t01-power-restored"))) return;
  Stage(15, .1); return;
 }
 if (T.Stage == 15)
 {
  if (M->Horror->Pack.CurrentScene())
  {
   if (!Check(!ThreatActive(), TEXT("kroppen venter mens T01-resultatets faktiske H02-presentasjon er åpen"))) return;
   M->UI->Close(true); M->Horror->Pack.Advance(); T.Hold = .1; return;
  }
  if (!ThreatActive()) { if (T.Elapsed - T.Since > 20) Fail(TEXT("SC02 startet ikke etter presentert nattlig T01")); return; }
  if (!Check(M->Horror->Pack.Flag(TEXT("hr_h02_recorded")) && !FeederOutage() && FeederCallAnswered() && DriveTimeSkipped() &&
      M->Story->Clock() >= 2 * 3600 && M->Story->Clock() < 2 * 3600 + 5 * 60 && M->Substation->NormalCompletionStamp() == T.NormalStamp,
      TEXT("SC02 starter først med presentert T01, kvittert oppdrag, faktisk road-hopp og klokke rundt 02:00"))) return;
  T.SC02Clock = M->Story->Clock();
  if (!FParse::Value(FCommandLine::Get(), TEXT("S47NightSequenceEscape="), T.EscapeSource) || T.EscapeSource.IsEmpty())
  { Stage(18, 0); return; }
  T.EscapeSource = FPaths::ConvertRelativePathToFull(T.EscapeSource);
  FString Text;
  if (!FFileHelper::LoadFileToString(Text, *T.EscapeSource)) { Fail(TEXT("kunne ikke lese navngitt SC02-fluktfixture")); return; }
  const auto Prior = S47J::Read(Text), Escaped = S47J::Clone(S47J::Obj(Prior, TEXT("horror")));
  const auto Meeting = S47J::Obj(Escaped, TEXT("sc02"));
  const auto Flags = S47J::Obj(S47J::Obj(Escaped, TEXT("pack")), TEXT("flags"));
  if (!Check(Escaped && S47J::Bool(Escaped, TEXT("sc01Complete")) && S47J::Str(Meeting, TEXT("phase")) == TEXT("escaped") &&
      S47J::Bool(Meeting, TEXT("threatSeen")) && !S47J::Str(Meeting, TEXT("escapeStamp")).IsEmpty() &&
      S47J::Bool(Flags, TEXT("hr_sc02_escape_presented")) && !S47J::Obj(Escaped, TEXT("sc03")) &&
      S47J::Str(Escaped, TEXT("referencePhoto")) == S47J::Str(M->Horror->Save(), TEXT("referencePhoto")),
      TEXT("navngitt SC02-diskkilde har faktisk kvittert flukt, samme originalreferanse og ingen SC03-credit"))) return;
  // En uttrykkelig revisjonsprobe av grensen, ikke en ny flukt: bare den tidligere
  // prøvens Horror-fakta settes inn. Ny Evening, T01, klokke og album beholdes.
  S47J::Set(Escaped, TEXT("contentRevision"), 3); M->UI->Close(true); M->Horror->Begin(Escaped, false);
  T.bSC03Probe = true;
  UE_LOG(LogTemp, Display, TEXT("S47: night sequence SC03 REVISION PROBE: actual escaped facts from %s relabelled revision 3; no new SC02 escape or SC03 content playthrough claimed"), *T.EscapeSource);
  if (!Check(M->Horror->SC02Escaped() && M->Horror->ContentRevision() == 3 && !FeederOutage() &&
      M->Story->Clock() == T.SC02Clock && M->Substation->NormalCompletionStamp() == T.NormalStamp,
      TEXT("revisjonsproben beholder den nye nattens klokke, strøm og faktisk nye T01-record"))) return;
  Stage(16, .15); return;
 }
 if (T.Stage == 16)
 {
  if (!M->Horror->SC03Live()) { if (T.Elapsed - T.Since > 5) Fail(TEXT("core StepSC03 startet ikke etter kvittert fluktfixture")); return; }
  if (!Check(M->Horror->SC03 && M->Horror->SC03->Phase() == FS47HorrorSC03::EPhase::Report &&
      M->Story->Clock() >= 2 * 3600 + 15 * 60 && M->Story->Clock() < 2 * 3600 + 20 * 60 &&
      !M->Horror->SC03->Facts().bDispatch && !M->Horror->SC03->Facts().bAssigned && !FeederOutage(),
      TEXT("faktisk StepSC03 starter rapportfasen rundt 02:15 uten oppfunnet dispatch eller tildeling"))) return;
  T.SC03Clock = M->Story->Clock(); const FString Facts = S47J::Write(S47J::Obj(M->Horror->Save(), TEXT("sc03")));
  if (!Load(TEXT("sc03-report-boundary"))) return;
  if (!Check(M->Horror->SC03Live() && M->Horror->SC03->Phase() == FS47HorrorSC03::EPhase::Report &&
      S47J::Write(S47J::Obj(M->Horror->Save(), TEXT("sc03"))) == Facts && M->Story->Clock() == T.SC03Clock &&
      !FeederOutage() && M->Story->BankIn() && M->Substation->NormalCompletionStamp() == T.NormalStamp,
      TEXT("SC03-grensens klokke, uendrede årsaksfakta og gjenopprettede mating overlever ny disk/full Flow"))) return;
  Stage(25, .15); return;
 }
 if (T.Stage == 25)
 {
  M->UI->Close(true); M->World->PlaceTruck(TEXT("saro")); M->World->Enter(TEXT("saro"));
  M->Player->Place(.05, -2.2, 0); M->Player->Pitch = -.08;
  UE_LOG(LogTemp, Display, TEXT("S47: night sequence SARO-RETURN POSITION FIXTURE: actual receiver tuning, direction solve and print timers follow; no continuous return drive claimed"));
  Dials(S47Signal::Profiles()[2]); M->Story->ConsoleAction();
  if (!Check(M->Story->Phase() == ES47Phase::Locked && M->Story->Rx.Stage == 3 && !M->Story->Rx.bSolved &&
      M->Story->Clock() >= T.SC03Clock && !FeederOutage() && M->Story->BankIn(),
      TEXT("faktisk residualtuning går til Locked etter 02:15 med gjenopprettet mating"))) return;
  T.LockedClock = M->Story->Clock();
  if (!Load(TEXT("after-residual-lock"))) return;
  if (!Check(M->Story->Phase() == ES47Phase::Locked && M->Story->Rx.Stage == 3 && M->Story->Clock() == T.LockedClock &&
      M->Story->Clock() >= T.SC03Clock, TEXT("Locked-Continue bevarer nattens faktiske klokke og RX3 uten retur til 00:13"))) return;
  Stage(26, .15); return;
 }
 if (T.Stage == 26)
 {
  M->UI->Close(true); M->Story->Use(TEXT("crtRight"));
  if (!Check(M->Story->Phase() == ES47Phase::Solving && !M->Story->Rx.bSolved && M->Story->Clock() >= T.LockedClock,
      TEXT("ordinær tracking-terminal starter faktisk direction solve fra gjenopptatt Locked"))) return;
  Stage(27, .15); return;
 }
 if (T.Stage == 27)
 {
  if (M->Story->Phase() != ES47Phase::Printed)
  { if (T.Elapsed - T.Since > 20) Fail(TEXT("ordinær solve/print nådde ikke Printed innen 20 sekunder")); return; }
  const auto Paper = M->Web->Obj(TEXT("room.printerPaper"));
  if (!Check(M->Story->Rx.Stage == 3 && M->Story->Rx.bSolved && M->Story->Night().SolveAt.IsSet() &&
      *M->Story->Night().SolveAt >= T.LockedClock && M->Story->Clock() >= T.SC03Clock &&
      Paper && Paper.Visible() && Paper.Scale().Y >= .999 && !M->Story->Night().PrintoutAt.IsSet(),
      TEXT("faktiske solve/print-timere gir Printed, tidsstemplet solve og helt utskrevet ark før arket er revet av"))) return;
  T.PrintedClock = M->Story->Clock(); const double SolveAt = *M->Story->Night().SolveAt;
  if (!Load(TEXT("after-direction-print"))) return;
  const auto RestoredPaper = M->Web->Obj(TEXT("room.printerPaper"));
  if (!Check(M->Story->Phase() == ES47Phase::Printed && M->Story->Rx.Stage == 3 && M->Story->Rx.bSolved &&
      M->Story->Night().SolveAt.IsSet() && *M->Story->Night().SolveAt == SolveAt &&
      M->Story->Clock() == T.PrintedClock && M->Story->Clock() >= T.SC03Clock &&
      RestoredPaper && RestoredPaper.Visible() && RestoredPaper.Scale().Y >= .999,
      TEXT("Printed-Continue bevarer faktisk klokke, RX solved, SolveAt, dokumenter og fysisk printerark"))) return;
  Stage(18, 0); return;
 }
 if (T.Stage == 18)
 {
  auto R = S47J::New(); S47J::Set(R, TEXT("status"), TEXT("PASS")); S47J::Set(R, TEXT("checks"), T.Checks);
  S47J::Set(R, TEXT("diskFlowRestores"), T.Loads); S47J::Set(R, TEXT("sc01FixtureSource"), T.FixtureSource);
  S47J::Set(R, TEXT("actualObservationRuns"), FS47Observing::Runs().Num()); S47J::Set(R, TEXT("t01Stamp"), T.NormalStamp);
  S47J::Set(R, TEXT("sc02Clock"), T.SC02Clock); S47J::Set(R, TEXT("sc03RevisionProbe"), T.bSC03Probe);
  if (T.bSC03Probe)
  {
   S47J::Set(R, TEXT("sc02EscapeFixtureSource"), T.EscapeSource); S47J::Set(R, TEXT("sc03BoundaryClock"), T.SC03Clock);
   S47J::Set(R, TEXT("residualLockedClock"), T.LockedClock); S47J::Set(R, TEXT("directionPrintedClock"), T.PrintedClock);
  }
  S47J::Set(R, TEXT("limits"), TEXT("D01 og observasjoner bruker produksjons-API, ikke menneskelig input. SC01-fakta og ett originalfoto er uttrykkelig diskfixture. Telefon-/felt-/returposisjoner og DriveOut(true) road-start er fixtures. Faktiske fysiske T01-rays, vanlige ConsoleClosed-timere, telefonens lukketransaksjon, aktiv road-kjørekontroller, gjenopprettet SARO-strøm med vedvarende stedlig feileffekt, modalpause og isolerte disk/full-Flow-restores kjøres. Når sc03RevisionProbe er true, prøves faktisk StepSC03-grense rundt 02:15 og dens diskrestore med navngitt tidligere kvittert flukt som uttrykkelig revisjon3-fixture; deretter kjøres faktisk residualtuning, direction solve/print og Locked-/Printed-diskrestore med bevart klokke og printerark. Helkjøring, fersk ordinær Continue-prosess, ny SC02-flukt, SC03-innhold, lydlytting, fysisk Xbox og fullnatt er ikke prøvd av denne kontrakten."));
  if (!FFileHelper::SaveStringToFile(S47J::Write(R, true), *(T.Root / TEXT("result.json")))) { Fail(TEXT("kunne ikke skrive sluttbevis")); return; }
  T.bDone = true;
  UE_LOG(LogTemp, Display, TEXT("S47: night sequence contract PASS COMPLETE: %d checks, %d fresh disk/full-Flow restores; SC01 and travel fixtures explicitly labelled; artifacts %s"), T.Checks, T.Loads, *T.Root);
  FPlatformMisc::RequestExit(false);
 }
}
