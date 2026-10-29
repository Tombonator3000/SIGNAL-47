#include "S47Substation.h"
#include "S47Evening.h"
#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Substation, Log, All);

namespace
{
 const FVector Origin(7890, -1.10, 536.20);
 const FLinearColor Warm(1.f, .62f, .18f);
 constexpr double Duration = 3.0;
 constexpr float Weak = .12f, Strong = 4.f, Lumens = 95.f;
 const TCHAR* JournalId = TEXT("substation:journal");
 const TCHAR* ServiceId = TEXT("substation:service");
 const TCHAR* LampId = TEXT("substation:lamp");
 FString Stamp(double Clock) { return FString(Clock < 0 ? TEXT("13 APR 1986 / ") : TEXT("14 APR 1986 / ")) + S47Text::ClockText(S47Timeline::Day(Clock), true); }
}

FVector FS47Substation::Site(const FVector& B) { return Origin + FVector(-B.X, B.Z, B.Y); }
FS47Substation::FS47Substation(US47Main* InM) : M(InM)
{
 bDiskTest = FParse::Param(FCommandLine::Get(), TEXT("S47SubstationTestLoad"));
 bTest = bDiskTest || FParse::Param(FCommandLine::Get(), TEXT("S47SubstationTest"));
}

void FS47Substation::Begin(const S47J::FObj& Saved)
{
 bJournal = bComplete = bObserved = false; Running = CompletedAt = ObservedAt = 0; CompletedStamp.Empty(); ResetArc();
 if (S47J::Int(Saved, TEXT("v")) == 1)
 {
  bJournal = S47J::Bool(Saved, TEXT("journal"));
  CompletedAt = S47J::Num(Saved, TEXT("completedAt")); ObservedAt = S47J::Num(Saved, TEXT("observedAt"));
  CompletedStamp = S47J::Str(Saved, TEXT("completedStamp"));
  bComplete = bJournal && S47J::Bool(Saved, TEXT("complete")) && S47J::Has(Saved, TEXT("completedAt")) && FMath::IsFinite(CompletedAt) && !CompletedStamp.IsEmpty();
  bObserved = bComplete && S47J::Bool(Saved, TEXT("observed")) && S47J::Has(Saved, TEXT("observedAt")) && FMath::IsFinite(ObservedAt);
 }
 if (!bBuilt) Install();
 BeginFollowup(S47J::Obj(Saved, TEXT("t02")));
 Apply(); SyncFacts(); Refile();
}

bool FS47Substation::Here() const
{
 return bReady && M && M->World && M->Player && M->Story && M->Mode == US47Main::EMode::Play &&
  (M->World->Area == TEXT("road") || M->World->Area == TEXT("diner")) && !M->World->bDriving &&
  FVector2D(M->Player->Pos.X - 7885, M->Player->Pos.Z - 527.3).Size() < 12;
}

FS47Obj FS47Substation::Target(const FString& Name, const FVector& At, const FVector& Size)
{
 const FS47WebNode* N = M->Web->Node(Root.I);
 UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (!N || !N->Comp.IsValid() || !Mesh) return {};
 UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(M->Web, *Name);
 C->SetMobility(EComponentMobility::Movable); C->SetStaticMesh(Mesh);
 C->SetupAttachment(N->Comp.Get()); C->SetWorldLocation(S47Three::ToUE(At));
 C->SetWorldScale3D(S47Three::DirToUE(Size)); C->SetHiddenInGame(true); C->SetCastShadow(false);
 C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore);
 C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block); C->RegisterComponent();
 return M->Web->ObjAt(M->Web->AdoptNode(C, Root.I, Name));
}

bool FS47Substation::Install()
{
 if (!M || !M->Web || !M->Inter) return false;
 Root = M->Web->Obj(TEXT("world.legs.road.area.group"));
 if (!Root) return false;
 bBuilt = true; // Props has finished by HUD init; absent native assets stay inert without per-frame traversal.
 // Props owns placement, floors and blockers. No invisible interaction is installed for an absent kit.
 for (const TCHAR* Part : { TEXT("Transformer"), TEXT("Buswork"), TEXT("Control"), TEXT("StatusLamp"), TEXT("Lever"), TEXT("Fence"), TEXT("Gate_L"), TEXT("Gate_R"), TEXT("Pad"), TEXT("Gravel") })
  if (!Root.Find(FString(TEXT("prop:substation")) + Part)) return false;
 bBuilt = true;
 Lamp = Root.Find(TEXT("prop:substationStatusLamp")); Lever = Root.Find(TEXT("prop:substationLever"));
 if (const FS47WebNode* N = M->Web->Node(Lever.I)) if (UStaticMeshComponent* C = Cast<UStaticMeshComponent>(N->Comp.Get())) { LeverComp = C; LeverRest = C->GetRelativeTransform(); }
 const FS47WebNode* LN = M->Web->Node(Lamp.I);
 UStaticMeshComponent* LC = LN ? Cast<UStaticMeshComponent>(LN->Comp.Get()) : nullptr;
 if (!LC || LC->GetNumMaterials() == 0) return false;
 for (int32 I = 0; I < LC->GetNumMaterials(); ++I)
  if (UMaterialInstanceDynamic* D = LC->CreateDynamicMaterialInstance(I)) LampMids.Add(D);
 // The lens extends 3.9 cm toward native local +Y. Emit just beyond that glass,
 // through the weather hood, instead of illuminating through the cabinet back.
 // Keep the same lumen/state driver: T01 and T02 compare this actual lamp.
 USpotLightComponent* L = NewObject<USpotLightComponent>(M->Web, TEXT("SubstationInspectionLight"));
 L->SetMobility(EComponentMobility::Movable); L->SetupAttachment(LC);
 L->SetRelativeLocation(FVector(0, 4.9, 0));
 L->SetRelativeRotation(FVector(0, 1, -.25).Rotation());
 L->SetInnerConeAngle(45.f); L->SetOuterConeAngle(75.f); L->SetSourceRadius(2.f);
 L->SetIntensityUnits(ELightUnits::Lumens); L->SetLightColor(Warm); L->SetAttenuationRadius(400); L->SetCastShadows(true); L->RegisterComponent(); Light = L;
 const FS47Obj Card = Target(TEXT("substationJournalTarget"), Site(FVector(4.83, -8.99, .77)), FVector(.50, .30, .08));
 const FS47Obj Switch = Target(TEXT("substationServiceTarget"), Site(FVector(5.52, -9.04, 1.30)), FVector(.24, .60, .24));
 const FS47Obj Lens = Target(TEXT("substationLampTarget"), Site(FVector(4.62, -9.01, 1.70)), FVector(.28, .24, .10));
 if (!Card || !Switch || !Lens || LampMids.IsEmpty()) return false;
 bReady = true;
 FS47Interactable J; J.Id = JournalId; J.Object = Card; J.Range = 2.8;
 J.Label = [this] { return Here() ? FString(TEXT("Read service journal")) : FString(); };
 J.Use = [this] { ReadJournal(); }; J.Examine = [] { return FString(TEXT("CALICHE JUNCTION SUBSTATION / LOCAL INSPECTION CIRCUIT. A service journal at the accessible control front.")); }; M->Inter->Add(J);
 FS47Interactable S; S.Id = ServiceId; S.Object = Switch; S.Range = 2.8;
 S.Label = [this] { return !Here() ? FString() : Running > 0 ? FString(TEXT("Service check running")) : bComplete ? FString(TEXT("Service check complete / compare the lamp")) : FString(TEXT("Run local service check")); };
 S.Use = [this] { StartCheck(); }; S.Examine = [this] { return !bComplete ? FString(TEXT("INSPECTION CIRCUIT. The ordinary complaint is a weak exterior light. The enclosed service control does not operate SARO's receiver or the town supply.")) : FString(TEXT("PANEL: CHECK COMPLETE. Compare this result with the marked lamp before filing it.")); }; M->Inter->Add(S);
 FS47Interactable O; O.Id = LampId; O.Object = Lens; O.Range = 3.2; O.ExamineRange = 10;
 O.Label = [this] { return Here() ? FString(FollowupAvailable() && !AttemptId.IsEmpty() ? TEXT("Observe current follow-up lamp") : bObserved ? TEXT("Read completed inspection") : TEXT("Inspect marked lamp")) : FString(); };
 O.Use = [this] { Observe(); }; O.Examine = [this]
 {
  if (!AttemptId.IsEmpty() && FollowupRecordValid()) return FString(bIsolated ? TEXT("The marked lamp continues a slow bright/dim sequence while the enclosed panel and independent isolation flag show OPEN. The earlier normal result remains a dated baseline, not a report of this later light.") : TEXT("The marked lamp is lit. The new enclosed controls have not yet accounted for its supply. The historical normal comparison keeps its own date."));
  return FString(bComplete ? TEXT("The marked inspection lamp burns steadily. The panel's completed check can be compared with this actual response.") : TEXT("The marked inspection lamp is weak. Its local service control is on the cabinet below."));
 }; M->Inter->Add(O);
 UE_LOG(LogS47Substation, Display, TEXT("S47: substation ready root=(7890,-1.10,536.20) fenceFrontZ=529.00; inspection outside closed yard"));
 InstallArc();
 InstallFollowup();
 if (FCString::Strifind(FCommandLine::Get(), TEXT("S47SubstationGroundProbe=")))
 {
  FTimerHandle H; M->Web->GetWorldTimerManager().SetTimer(H, [this] { ProbeGround(); }, 4.f, false);
 }
 return true;
}

void FS47Substation::Apply()
{
 if (!bReady) return;
 const float K = (!AttemptId.IsEmpty() && FollowupRecordValid() ? FollowupLampStrength() : bComplete ? Strong : Running > 0 ? 0.f : Weak) * LampDip();
 for (const auto& W : LampMids) if (UMaterialInstanceDynamic* D = W.Get()) D->SetVectorParameterValue(TEXT("EmissiveFactor"), Warm * K);
 if (UPointLightComponent* L = Light.Get()) { L->SetIntensity(Lumens * K / Strong); L->SetVisibility(Root.Shown()); }
 if (UStaticMeshComponent* C = LeverComp.Get()) { FTransform T = LeverRest; T.SetRotation(LeverRest.GetRotation() * FQuat(FVector::ForwardVector, Running > 0 || bComplete ? -.65 : 0)); C->SetRelativeTransform(T); }
 ApplyFollowup();
}

bool FS47Substation::RenderedResult() const
{
 if (!bReady || !bComplete || LampMids.IsEmpty() || !Light.IsValid() || Light->Intensity < Lumens - .01f || !Root.Shown()) return false;
 for (const auto& W : LampMids)
 {
  UMaterialInstanceDynamic* D = W.Get();
  if (!D || !D->K2_GetVectorParameterValue(TEXT("EmissiveFactor")).Equals(Warm * Strong, .001f)) return false;
 }
 return true;
}

void FS47Substation::Commit(const FString& Scene)
{
 // These UI-only authored effects acknowledge the same physical paper/result already shown here.
 // A late/legacy case keeps this local evidence without forcing an authored phase or starting T02.
 if (!M || !M->Evening || !M->Evening->Enabled()) return;
 if (M->Flow && M->Flow->bStarting) return; // All current-case controllers must restore before authored acknowledgement.
 auto& P = M->Evening->Pack;
 if (!P.CanStart(Scene) || !P.Start(Scene)) return;
 while (P.CurrentLine()) P.Advance();
}

void FS47Substation::ReadJournal()
{
 if (!Here() || !M->UI) return;
 bJournal = true; Commit(TEXT("T01_SERVICE_LOG")); Refile(); M->RequestAutosave();
 const FS47Doc* D = M->Story->Docs().FindByPredicate([](const FS47Doc& It) { return It.Id == TEXT("EV_TRAFO_CARD"); });
 if (D) M->UI->Document(*D);
}

void FS47Substation::StartCheck()
{
 if (!Here() || !M->UI || Running > 0) return;
 if (M->Story && M->Story->NightSequence() && !M->Story->FeederOutage() && !bComplete)
 { M->UI->Toast(TEXT("The circuit is stable. No feeder fault is assigned; keep the service controls at normal."), 4); return; }
 if (M->Story && M->Story->NightSequence() && M->Evening && !M->Evening->FeederCallAnswered() && !bComplete)
 { M->UI->Toast(TEXT("The county dispatcher needs to confirm the feeder assignment. Answer the control-room telephone before operating the service check."), 5); return; }
 if (!AttemptId.IsEmpty() && FollowupRecordValid()) { M->UI->Toast(TEXT("The normal baseline keeps its original completion time. Use the separate enclosed follow-up and auxiliary controls for the current comparison."), 4); return; }
 if (!bJournal) { M->UI->Toast(TEXT("Read the service journal and match the inspection-circuit label first."), 3.5); return; }
 if (bComplete) { M->UI->Toast(TEXT("The panel check is complete. Compare the marked lamp, then retain the result."), 3.5); return; }
 Running = .001; Apply();
 M->UI->Toast(TEXT("LOCAL INSPECTION CIRCUIT / CHECK RUNNING. Watch the marked lamp."), 3.5);
 UE_LOG(LogS47Substation, Display, TEXT("S47: substation service started; marked lamp off during check"));
}

void FS47Substation::Observe()
{
 if (!Here() || !M->UI) return;
 if (FollowupAvailable() && !AttemptId.IsEmpty()) { ObserveFollowupLamp(); return; }
 if (!bComplete || !RenderedResult()) { M->UI->Toast(TEXT("No completed comparison to file. The marked lamp and the service journal are still available."), 3.5); return; }
 if (!bObserved)
 {
  bObserved = true; ObservedAt = M->Story->Clock(); SyncFacts(); Commit(TEXT("T01_NORMAL_RESULT")); Refile(); M->RequestAutosave();
  M->Story->Note(TEXT("Caliche Junction: the local inspection check and the marked lamp agreed. Completion recorded at ") + CompletedStamp + TEXT(". This result concerns that circuit at that time."));
  UE_LOG(LogS47Substation, Display, TEXT("S47: substation comparison recorded; completed=%.3f observed=%.3f; local circuit only"), CompletedAt, ObservedAt);
 }
 const FS47Doc* D = M->Story->Docs().FindByPredicate([](const FS47Doc& It) { return It.Id == TEXT("EV_TRAFO_CHECK"); });
 if (D) M->UI->Document(*D);
}

void FS47Substation::Refile()
{
 if (!M || !M->Story) return;
 auto Add = [this](const FS47Doc& D) { if (!M->Story->Docs().ContainsByPredicate([&D](const FS47Doc& E) { return E.Id == D.Id; })) M->Story->AddDoc(D); };
 if (bJournal)
 {
  FS47Doc D; D.Id = TEXT("EV_TRAFO_CARD"); D.Title = TEXT("Caliche Junction / service journal"); D.Kind = TEXT("typed"); D.Stamp = TEXT("CALICHE JUNCTION / INSPECTION FRONT");
  D.Page = TEXT("LOCAL INSPECTION CIRCUIT\n\nOpen complaint: weak exterior inspection light.\n\nMatch this journal to the labelled service front. Run the enclosed local check, then compare the completed panel result with the marked lamp before filing a result.\n\nThe receiver at SARO, K3 and the town supply are separate systems. No entry into the switching yard is required."); D.Transcript = D.Page; Add(D);
 }
 if (bObserved)
 {
  FS47Doc D; D.Id = TEXT("EV_TRAFO_CHECK"); D.Title = TEXT("Inspection card / completed local check"); D.Kind = TEXT("hand"); D.Stamp = TEXT("OPERATOR RECORD / CURRENT SHIFT");
  D.Page = TEXT("CALICHE JUNCTION / LOCAL INSPECTION CIRCUIT\n\nThe service panel completed its check. I compared the marked lamp with that result: it was burning steadily.\n\nActual completion: ") + CompletedStamp + TEXT("\nObserved: ") + Stamp(ObservedAt) + TEXT("\n\nThe ordinary local lamp complaint was resolved under this check. This records that circuit at that time, not the supply state of the town or observatory throughout the night."); D.Transcript = D.Page; Add(D);
 }
 // Chapter saves retain this measured paper in the controller's immutable filing history.
 // Reopening a case restores the original paper, never a new filing or current-attempt preview.
 const FS47Doc Filed = FiledFollowupDocument();
 if (!Filed.Id.IsEmpty()) Add(Filed);
}

void FS47Substation::SyncFacts()
{
 if (M && M->Evening)
 {
  M->Evening->Pack.SetExternal(TEXT("ctx_trafo_service_ok"), HasNormalResult());
  // If another authored conversation owned Pack during the physical action, acknowledge
  // the actual journal/result once it is idle. CanStart preserves phase/idempotency gates.
  if (bJournal) Commit(TEXT("T01_SERVICE_LOG"));
  if (HasNormalResult()) Commit(TEXT("T01_NORMAL_RESULT"));
  if (HasNormalResult()) Commit(TEXT("T02_BASELINE_RESULT"));
 }
 SyncFollowupFacts();
}

S47J::FObj FS47Substation::Save() const
{
 auto O = S47J::New(); S47J::Set(O, TEXT("v"), 1); S47J::Set(O, TEXT("journal"), bJournal);
 S47J::Set(O, TEXT("complete"), bComplete); S47J::Set(O, TEXT("observed"), bObserved);
 S47J::Set(O, TEXT("completedAt"), CompletedAt); S47J::Set(O, TEXT("observedAt"), ObservedAt); S47J::Set(O, TEXT("completedStamp"), CompletedStamp);
 S47J::Set(O, TEXT("t02"), FollowupSnapshot());
 return O;
}

void FS47Substation::Step(double Dt)
{
 if (!bBuilt) Install();
 if (Running > 0 && M && M->Story && M->Mode == US47Main::EMode::Play && !M->ModalOpen() && !M->Story->IsCinematic())
 {
  Running += Dt;
  if (Running >= Duration) { Running = 0; bComplete = true; CompletedAt = M->Story->Clock(); CompletedStamp = Stamp(CompletedAt); M->RequestAutosave(); UE_LOG(LogS47Substation, Display, TEXT("S47: substation service complete at %s; compare actual lamp"), *CompletedStamp); }
 }
 StepFollowup(Dt); StepArc(Dt); Apply(); SyncFacts(); TestStep(Dt);
}

void FS47Substation::TestStep(double Dt)
{
 if (!bTest || bTestDone || !M || !M->Player || !M->World || !M->Inter || !M->UI || !M->Story) return;
 TestTime += Dt; if (TestTime < 3) return;
 auto Fail = [this](const TCHAR* Why) { bTestDone = true; M->Player->TestMove.Reset(); UE_LOG(LogS47Substation, Error, TEXT("S47: substation test FAIL: %s"), Why); };
 if (!bReady) { Fail(TEXT("native kit/interactions missing")); return; }
 if (TestStage == 0)
 {
  if (bDiskTest)
  {
   FString UserDir;
   const bool HasDir = FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir);
   if (!HasDir || UserDir.IsEmpty() || !M->Flow || M->Flow->CaseId < 1 || M->Flow->CaseId > S47Saves::CASES ||
    !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir), FPaths::ConvertRelativePathToFull(UserDir)))
   { Fail(TEXT("disk test requires isolated -UserDir and -S47Case=1..3; save root must be inside that UserDir")); return; }
   for (const auto& Existing : M->Flow->Saves.List(M->Flow->CaseId))
    if (Existing.Kind == TEXT("manual") && Existing.Slot == 0) { Fail(TEXT("manual test slot already exists; use a fresh isolated profile")); return; }
  }
  // A road vehicle is left in PlaceOfLeg("road") == "diner": the existing foot area.
  // Walkable describes the union of zones only; actual colliders are applied by Walker::Resolve.
  M->World->Enter(TEXT("diner")); M->Player->Place(7885, 520.8, PI); M->Player->Pitch = -.08;
  if (!M->Player->Walkable(7885, 520.8)) { Fail(TEXT("service-track approach has no walkable ground")); return; }
  M->Player->TestMove = FVector2D(0, -1); TestDiskAt = TestTime; TestStage = 10; return;
 }
 if (TestStage == 10 || TestStage == 11)
 {
  FHitResult Hit; const FVector P = M->Player->Pos;
  const bool Ground = M->Web->GetWorld()->LineTraceSingleByChannel(Hit, S47Three::ToUE(FVector(P.X, M->Player->FloorY + .3, P.Z)), S47Three::ToUE(FVector(P.X, M->Player->FloorY - 4, P.Z)), ECC_Visibility);
  if (!Ground || FMath::Abs(S47Three::ToThree(Hit.ImpactPoint).Y - M->Player->FloorY) > .35) { Fail(TEXT("actual walking probe lost its visible floor")); return; }
  if (TestStage == 10)
  {
   if (TestTime - TestDiskAt < 2.6) return;
   M->Player->TestMove.Reset();
   if (FMath::Abs(P.X - 7885) > .03 || P.Z < 525.2 || P.Z > 526.8) { Fail(TEXT("normal walking failed to reach the service apron from the track")); return; }
   UE_LOG(LogS47Substation, Display, TEXT("S47: substation approach walk PASS: %.3f %.3f floor %.3f"), P.X, P.Z, M->Player->FloorY);
   M->Player->Place(7893, 527.3, PI); M->Player->TestMove = FVector2D(0, -1); TestDiskAt = TestTime; TestStage = 11; return;
  }
  if (TestTime - TestDiskAt < 1.2) return;
  M->Player->TestMove.Reset();
  if (FMath::Abs(P.X - 7893) > .03 || FMath::Abs(P.Z - (529 - M->Player->Radius)) > .03) { Fail(TEXT("normal walking did not stop at the closed yard fence")); return; }
  UE_LOG(LogS47Substation, Display, TEXT("S47: substation closed yard walk PASS: %.3f %.3f (front 529.000 minus radius)"), P.X, P.Z);
  M->Player->Place(7885, 525.8, PI); M->Player->Pitch = -.08; TestDiskAt = TestTime; TestStage = 12; return;
 }
 auto UsePicked = [this, &Fail](const TCHAR* Id, const FVector& B)
 {
  const FVector Eye = M->Player->EyeUE(), TargetUE = S47Three::ToUE(Site(B));
  const FString Picked = M->Inter->Pick(Eye, (TargetUE - Eye).GetSafeNormal());
  if (Picked != Id) { Fail(*(FString::Printf(TEXT("actual eye trace missed %s (picked '%s')"), Id, *Picked))); return false; }
  UE_LOG(LogS47Substation, Display, TEXT("S47: substation physical pick PASS: %s"), Id);
  M->Inter->UseId(Picked); return true;
 };
 if (TestStage == 12)
 {
  if (TestTime - TestDiskAt < .2) return; // let the ordinary walker place its real eye after Place
  if (!UsePicked(ServiceId, FVector(5.52, -9.10, 1.42))) return; if (Running > 0 || bComplete) { Fail(TEXT("service bypassed journal prerequisite")); return; }
  if (!UsePicked(JournalId, FVector(4.83, -8.99, .77))) return; M->UI->Close(true);
  if (!bJournal) { Fail(TEXT("journal interaction did not retain evidence")); return; }
  if (!UsePicked(ServiceId, FVector(5.52, -9.10, 1.42))) return; if (Running <= 0 || RenderedResult()) { Fail(TEXT("service did not start with lamp off")); return; }
  TestStage = 1; return;
 }
 if (TestStage == 1 && bComplete)
 {
  if (!RenderedResult()) { Fail(TEXT("actual lamp material/light do not show completed result")); return; }
  if (!UsePicked(LampId, FVector(4.62, -9.01, 1.70))) return; M->UI->Close(true);
  if (!bObserved) { Fail(TEXT("physical lamp comparison not recorded")); return; }
  const auto O = Save(); const FString Before = CompletedStamp; Begin(O);
  if (!bObserved || !bComplete || CompletedStamp != Before || !RenderedResult()) { Fail(TEXT("serialized state did not restore same result/time/light")); return; }
  if (bDiskTest)
  {
   if (!M->Flow->SaveBlock().IsEmpty()) { Fail(TEXT("normal save rules currently block this fixture")); return; }
   FString Error; const auto Meta = M->Flow->WriteSave(TEXT("manual"), 0, Error);
   if (!Meta) { Fail(*(TEXT("disk write: ") + Error)); return; }
   const auto Disk = M->Flow->Saves.Load(Meta->Id);
   if (!Disk || S47J::Str(Disk, TEXT("area")) != TEXT("diner") || !S47J::Bool(S47J::Obj(Disk, TEXT("substation")), TEXT("observed"))) { Fail(TEXT("disk file omitted normal road-foot area or observed result")); return; }
   TestStamp = CompletedStamp; TestCompletedAt = CompletedAt; TestPose = M->Player->Pos;
   TestDiskAt = TestTime; TestStage = 2;
   UE_LOG(LogS47Substation, Display, TEXT("S47: substation disk test saved %s under %s; opening through normal Flow"), *Meta->Id, *M->Flow->Saves.Dir);
   M->Flow->OpenSave(*Meta); return;
  }
  bTestDone = true;
  UE_LOG(LogS47Substation, Display, TEXT("S47: substation test PASS: journal+service+actualLamp+comparison+serialization+walkApproach+yardBlocked+physicalPicks; timestamp=%s"), *CompletedStamp);
 }
 if (TestStage == 2 && TestTime - TestDiskAt > .75)
 {
  if (!M->Flow || M->Flow->bStarting || M->World->Area != TEXT("diner") || !bObserved || !bComplete || CompletedStamp != TestStamp || CompletedAt != TestCompletedAt ||
   FVector2D(M->Player->Pos.X - TestPose.X, M->Player->Pos.Z - TestPose.Z).Size() > .1 || !RenderedResult())
  { Fail(TEXT("normal disk reload did not restore road pose, service time, observation and actual lamp")); return; }
  bTestDone = true;
  UE_LOG(LogS47Substation, Display, TEXT("S47: substation test PASS: journal+service+actualLamp+comparison+serialization+walkApproach+yardBlocked+physicalPicks+diskReloadRoadPose; timestamp=%s"), *CompletedStamp);
 }
 if (!bTestDone && TestTime > 20) Fail(TEXT("service/disk reload did not finish"));
}

bool FS47Substation::SelfTest(TArray<FString>& Lines)
{
 bool Ok = true; auto Check = [&](bool P, const TCHAR* N) { Lines.Add(FString(P ? TEXT("ok  substation: ") : TEXT("FAIL substation: ")) + N); Ok &= P; };
 Check(Site(FVector(0, -7.2, 0)).Equals(FVector(7890, -1.10, 529), .00001), TEXT("authored gate front maps to road world coordinates"));
 FS47Substation S(nullptr); S.Begin(); Check(!S.bObserved && !S.bComplete && !S.bJournal, TEXT("old or absent save starts with an inspectable ordinary complaint"));
 S.bJournal = S.bComplete = S.bObserved = true; S.CompletedAt = -19800; S.ObservedAt = -19795; S.CompletedStamp = Stamp(S.CompletedAt);
 FS47Substation R(nullptr); R.Begin(S.Save()); Check(R.bObserved && R.CompletedAt == -19800 && R.CompletedStamp == S.CompletedStamp, TEXT("roundtrip retains real signed completion time"));
 auto Bad = S.Save(); S47J::Set(Bad, TEXT("complete"), false); R.Begin(Bad); Check(!R.bObserved, TEXT("an observation cannot restore without a completed service check"));
 Check(Stamp(-19800).Contains(TEXT("13 APR 1986")) && Stamp(3600).Contains(TEXT("14 APR 1986")), TEXT("completion stamps follow the existing midnight axis"));
 Ok &= FollowupSelfTest(Lines);
 Ok &= ArcSelfTest(Lines);
 return Ok;
}
