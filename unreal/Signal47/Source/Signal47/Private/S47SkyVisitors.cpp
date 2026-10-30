#include "S47SkyVisitors.h"

#include "S47Evening.h"
#include "S47Chapter6.h"
#include "S47FieldCamera.h"
#include "S47Flow.h"
#include "S47Geo.h"
#include "S47Image.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/MaterialBillboardComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Base64.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SkyVisitors, Log, All);

namespace
{
 bool StrictBool(const S47J::FObj& O, const TCHAR* K, bool& Out)
 {
  const auto V = O.IsValid() ? O->TryGetField(K) : nullptr;
  return V.IsValid() && V->Type == EJson::Boolean && V->TryGetBool(Out);
 }
 bool StrictNumber(const S47J::FObj& O, const TCHAR* K, double& Out)
 {
  const auto V = O.IsValid() ? O->TryGetField(K) : nullptr;
  return V.IsValid() && V->Type == EJson::Number && V->TryGetNumber(Out) && FMath::IsFinite(Out);
 }
 bool TraceSkySight(US47Main* M, const FVector& Eye, const FVector& At, FString* Detail = nullptr)
 {
  if (!M || !M->Web || !M->Web->GetWorld()) { if (Detail) *Detail = TEXT("no live world"); return false; }
  FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SkyVisitorSight), true);
  const FVector From = S47Three::ToUE(Eye), End = S47Three::ToUE(At);
  for (int32 I = 0; I < 32; ++I)
  {
   FHitResult H;
   if (!M->Web->GetWorld()->LineTraceSingleByChannel(H, From, End, ECC_Visibility, Q))
   { if (Detail) *Detail = FString::Printf(TEXT("clear after %d ignored components"), I); return true; }
   const auto* C = H.GetComponent();
   if (!C) { if (Detail) *Detail = TEXT("blocking hit has no component"); return false; }
   const int32 NodeIndex = M->Web->NodeOf(C);
   const auto* Node = M->Web->Node(NodeIndex);
   bool Solid = C->IsVisible();
   if (Solid && M->Web->ItemOf(C) && Node && Node->Name.StartsWith(TEXT("proxy"))) Solid = false;
   const auto* Mat = C->GetMaterial(FMath::Max<int32>(0, H.ElementIndex));
   float Opacity = 1;
   if (Mat)
   {
    const auto Blend = Mat->GetBlendMode();
    Mat->GetScalarParameterValue(TEXT("Opacity"), Opacity);
    if (Blend == BLEND_Additive || (Blend == BLEND_Translucent && Opacity < .5f)) Solid = false;
   }
   if (Solid)
   {
    if (Detail) *Detail = FString::Printf(TEXT("actor=%s component=%s hiddenInGame=%d node=%d:%s material=%s blend=%d opacity=%.3f hit=%s distance=%.3fm ignored=%d"),
     *GetNameSafe(H.GetActor()), *C->GetPathName(), C->bHiddenInGame, NodeIndex, Node ? *Node->Name : TEXT("none"), *GetNameSafe(Mat),
     Mat ? int32(Mat->GetBlendMode()) : -1, Opacity, *S47Three::ToThree(H.ImpactPoint).ToString(), H.Distance / 100., I);
    return false;
   }
   Q.AddIgnoredComponent(C);
  }
  if (Detail) *Detail = TEXT("32 ignored components exhausted the sight trace");
  return false;
 }
}

double FS47SkyVisitorState::Duration(int32 S)
{
 const double Durations[5] = {80, 120, 150, 160, 240};
 return S >= 1 && S <= 5 ? Durations[S - 1] : 0;
}
int32 FS47SkyVisitorState::Count(int32 S) { const int32 N[5] = {1, 2, 4, 6, 9}; return S >= 1 && S <= 5 ? N[S - 1] : 0; }
int32 FS47SkyVisitorState::RequiredVisible(int32 S) { const int32 N[5] = {1, 2, 2, 3, 4}; return S >= 1 && S <= 5 ? N[S - 1] : MAX_int32; }
int32 FS47SkyVisitorState::ScheduledStage(double Clock, int32 Chapter, bool bHandoverDone)
{
 if (!FMath::IsFinite(Clock)) return 0;
 const double Night = S47Timeline::Night(Clock);
 // The evening has one light only, first due at 21:30. Observing's short clock
 // jumps past that time may start the opportunity, but cannot replace it with
 // the old 20:00/22:00 multi-light tracks. Later groups follow actual chapters.
 if (Night < 0) return bHandoverDone && Night >= S47Timeline::Sky01 ? 1 : 0;
 if (Chapter >= 5) return 5;
 if (Chapter >= 3) return 4;
 if (Chapter >= 2) return 3;
 if (Chapter >= 1) return 2;
 return bHandoverDone ? 1 : 0;
}

void FS47SkyVisitorState::Begin(const S47J::FObj& Saved, bool bNewNight)
{
 *this = FS47SkyVisitorState();
 if (!Saved.IsValid()) { bEnabled = bNewNight; return; }
 double Version = 0, Phase = 0, Time = 0;
 bool Enabled = false, Second = false;
 if (!StrictNumber(Saved, TEXT("v"), Version) || Version != 1 ||
  !StrictNumber(Saved, TEXT("stage"), Phase) || Phase < 0 || Phase > 5 || Phase != FMath::FloorToDouble(Phase) ||
  !StrictNumber(Saved, TEXT("elapsed"), Time) || Time < 0 || Time > Duration(int32(Phase)) ||
  !StrictBool(Saved, TEXT("enabled"), Enabled) || !StrictBool(Saved, TEXT("secondEarlyPass"), Second)) return;
 const auto Stages = S47J::Arr(Saved, TEXT("stages"));
 if (Stages.Num() != 5) return;
 FS47SkyVisitorState Restored;
 Restored.bEnabled = Enabled; Restored.Stage = int32(Phase); Restored.Elapsed = Time; Restored.bSecondEarlyPass = Second;
 for (int32 I = 0; I < 5; ++I)
 {
  if (!Stages[I].IsValid() || Stages[I]->Type != EJson::Object) return;
  const auto O = Stages[I]->AsObject();
  if (!StrictBool(O, TEXT("occurred"), Restored.Occurred[I]) ||
   !StrictBool(O, TEXT("observed"), Restored.Observed[I]) ||
   !StrictBool(O, TEXT("completed"), Restored.Completed[I])) return;
  if ((Restored.Observed[I] || Restored.Completed[I]) && !Restored.Occurred[I]) return;
  if (I + 1 > Restored.Stage && Restored.Occurred[I]) return;
 }
 if (Restored.Stage && (!Restored.Occurred[Restored.Stage - 1] ||
   (Restored.Completed[Restored.Stage - 1] != (Time >= Duration(Restored.Stage))))) return;
 if (Second && !Restored.Occurred[0]) return;
 *this = Restored; // Pending gaze dwell is deliberately not a saved observation.
}

bool FS47SkyVisitorState::Advance(int32 Requested, double Dt, bool bHeld)
{
 if (!bEnabled || bHeld || !FMath::IsFinite(Dt) || Dt <= 0) return false;
 Requested = FMath::Clamp(Requested, 0, 5);
 bool Changed = false;
 if (Requested > Stage)
 {
  // Optional sky opportunities can be missed. A later chapter never replays all
  // old tracks, nor marks them observed, after a clock jump or a time away.
  Stage = Requested; Elapsed = 0; Looking = 0; Occurred[Stage - 1] = true; Changed = true;
 }
 if (!Stage || Completed[Stage - 1]) return Changed;
 Elapsed = FMath::Min(Duration(Stage), Elapsed + Dt);
 if (Elapsed >= Duration(Stage)) { Completed[Stage - 1] = true; Looking = 0; Changed = true; }
 return Changed;
}
bool FS47SkyVisitorState::Observe(int32 Visible, double Dt)
{
 if (!bEnabled || Stage < 1 || Completed[Stage - 1] || Observed[Stage - 1] || !FMath::IsFinite(Dt) || Dt <= 0) return false;
 Looking = Visible >= RequiredVisible(Stage) ? Looking + Dt : 0;
 if (Looking < .65) return false;
 Observed[Stage - 1] = true; Looking = 0; return true;
}
bool FS47SkyVisitorState::OfferEarlyPass()
{
 if (!bEnabled || Stage != 1 || !Completed[0] || Observed[0] || bSecondEarlyPass) return false;
 bSecondEarlyPass = true; Completed[0] = false; Elapsed = Looking = 0; return true;
}
S47J::FObj FS47SkyVisitorState::Save() const
{
 const auto O = S47J::New();
 S47J::Set(O, TEXT("v"), 1); S47J::Set(O, TEXT("enabled"), bEnabled);
 S47J::Set(O, TEXT("stage"), Stage); S47J::Set(O, TEXT("elapsed"), Elapsed);
 S47J::Set(O, TEXT("secondEarlyPass"), bSecondEarlyPass);
 TArray<TSharedPtr<FJsonValue>> Stages;
 for (int32 I = 0; I < 5; ++I)
 {
  const auto P = S47J::New(); S47J::Set(P, TEXT("occurred"), Occurred[I]);
  S47J::Set(P, TEXT("observed"), Observed[I]); S47J::Set(P, TEXT("completed"), Completed[I]);
  Stages.Add(MakeShared<FJsonValueObject>(P));
 }
 O->SetArrayField(TEXT("stages"), Stages); return O;
}

FVector FS47SkyVisitorState::MapPosition(int32 S, int32 I, double Seconds)
{
 const double P = FMath::Clamp(Seconds / FMath::Max(1., Duration(S)), 0., 1.);
 // Road-map coordinates: the middle of this slow crossing is due north of
 // SARO's walk gate (site x=-1, z=-20). It is a distant light, never a figure.
 if (S == 1) return FVector(23 - 520 + 1040 * P, 330 + 22 * FMath::Sin(P * PI), -1180 + 80 * P);
 if (S == 2) return I == 0 ? FVector(-510 - 650 * P, 650 + 70 * P, 1410 + 360 * P)
  : FVector(-430 + 1100 * P, 760 + 70 * P, 1320 - 520 * P);
 if (S == 3)
 {
  static const FVector Starts[] = {{-760,800,-1000},{220,950,-1300},{-950,760,900},{900,900,1200}};
  const FVector A = Starts[FMath::Clamp(I, 0, 3)];
  return I % 2 ? A + FVector(650 * P, -40 * P, 310 * P) : A;
 }
 if (S == 4) return FVector(-700 + I * 105 + 800 * P, 670 + (I % 2) * 55, -1050 - I * 90 - 410 * P);
 const int32 Group = FMath::Clamp(I / 3, 0, 2), Member = I % 3;
 return FVector(-1000 + Group * 900 + Member * 75 + (Group - 1) * 440 * P,
  750 + Group * 80 + Member * 22, -850 - Member * 130 - 800 * P);
}

FS47SkyVisitors::FS47SkyVisitors(US47Main* InM) : M(InM)
{
 FParse::Value(FCommandLine::Get(), TEXT("S47SkyVisit="), TestStage);
 TestStage = FMath::Clamp(TestStage, 0, 5);
 bContract = FParse::Param(FCommandLine::Get(), TEXT("S47SkyVisitContract"));
 FParse::Value(FCommandLine::Get(), TEXT("S47SkyVisitProof="), ContractRoot);
}
FS47SkyVisitors::~FS47SkyVisitors()
{
 for (auto& W : Points) if (W.IsValid()) W->DestroyComponent();
}
void FS47SkyVisitors::Begin(const S47J::FObj& Saved, bool bNewNight)
{
 DevStage(0); // A previous F10 preview never controls the next real case or Continue.
 State.Begin(Saved, bNewNight); ObservationStep = 0; Hide();
 if (TestStage && !Saved.IsValid()) State.Begin(nullptr, true);
 if (State.bEnabled) Build();
}
void FS47SkyVisitors::DevStage(int32 Stage)
{
 Stage=FMath::Clamp(Stage,0,5);
 if (!Stage)
 {
  if (bDevOverride) { State=BeforeDev; TestStage=BeforeDevTestStage; bDevOverride=false; ObservationStep=0; Hide(); }
  return;
 }
 if (!M || !M->Flow || !M->Flow->bDevRun || M->Flow->CaseId!=0 || M->Mode!=US47Main::EMode::Play) return;
 if (!bDevOverride) { BeforeDev=State; BeforeDevTestStage=TestStage; bDevOverride=true; }
 // A preview is independent even when a lower stage is selected after a later one.
 State.Begin(nullptr,true); TestStage=Stage; ObservationStep=0; Hide();
}
void FS47SkyVisitors::Build()
{
 if (bBuilt || !M || !M->Web) return;
 auto* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebGlow.M_WebGlow"));
 if (!Glow) return;
 for (int32 I = 0; I < 9; ++I)
 {
  auto* B = NewObject<UMaterialBillboardComponent>(M->Web, *FString::Printf(TEXT("SkyVisitor%d"), I));
  B->SetupAttachment(M->Web->GetRootComponent()); B->SetMobility(EComponentMobility::Movable);
  auto* D = UMaterialInstanceDynamic::Create(Glow, M->Web);
  D->SetVectorParameterValue(TEXT("Color"), FLinearColor::Black);
  B->AddElement(D, nullptr, false, 50, 50, nullptr);
  B->SetCollisionEnabled(ECollisionEnabled::NoCollision); B->SetCastShadow(false);
  B->RegisterComponent(); B->SetVisibility(false); Points.Add(B);
 }
 bBuilt = true;
}
void FS47SkyVisitors::Hide()
{
 State.Looking = 0; ObservationStep = 0;
 for (auto& B : Points) if (B.IsValid()) B->SetVisibility(false);
}

FVector FS47SkyVisitors::WorldPosition(const FVector& P, const FString& Area)
{
 if (Area == TEXT("saro")) return P - FVector(S47Geo::SARO_IN_ROAD.X, 0, S47Geo::SARO_IN_ROAD.Z);
 if (Area == TEXT("station01"))
 {
  const auto Q = S47Geo::RoadToStation(P.X, P.Z); return FVector(Q.X, P.Y + S47Geo::STATION_DY(), Q.Z + 8000);
 }
 return P + FVector(8000, 0, 0); // road, diner and Roswell share the same geographic frame.
}
bool FS47SkyVisitors::ClearLine(const FVector& Eye, const FVector& At) const
{
 return TraceSkySight(M, Eye, At);
}
int32 FS47SkyVisitors::RequestedStage() const
{
 if (TestStage) return TestStage;
 if (!M || !M->Story || !M->Story->bEveningTimeline) return 0;
 const FString& C = M->Story->ChapterKey;
 const int32 Chapter = C.StartsWith(TEXT("ch")) ? FCString::Atoi(*C.Mid(2)) : 0;
 return FS47SkyVisitorState::ScheduledStage(M->Story->Clock(), Chapter,
  M->Evening && M->Evening->Pack.Flag(TEXT("handover_done")));
}
void FS47SkyVisitors::RecordObservation(int32 Visible)
{
 if (bDevOverride) { UE_LOG(LogS47SkyVisitors,Display,TEXT("S47: developer sky preview observed, no story note or saved observation")); return; }
 static const TCHAR* Notes[] = {
  TEXT("One pale light above the mesa."),
  TEXT("Two lights visible together in the sky."),
  TEXT("More than one light in the sky, in separate parts of the view."),
  TEXT("A row of lights above the field."),
  TEXT("Separate groups of lights over the horizon.")};
 FString Text = Notes[State.Stage - 1];
 if (State.Stage == 5 && State.Observed[0]) Text += TEXT(" More lights than the first one I saw.");
 if (M->Story) M->Story->Note(Text);
 UE_LOG(LogS47SkyVisitors, Display, TEXT("S47: SKY%02d observed: %d world lights in view with clear sight"), State.Stage, Visible);
 M->RequestAutosave();
}
void FS47SkyVisitors::Step(double Dt)
{
 ControlTest(Dt);
 if (!M || !M->Story || !M->Player || !M->World || !State.bEnabled || M->Mode != US47Main::EMode::Play)
 { Hide(); return; }
 const auto* Finale = Cast<US47Chapter6>(M->Story->Chapter(TEXT("ch6")));
 // The revised last road owns the horizon. Retain the lights' actual positions and phase;
 // neither a new optional crossing nor observation credit competes with THE EVENT.
 const bool Held = M->bPausedByMenu || M->ModalOpen() || M->Story->IsCinematic() || (Finale && Finale->RevisedFinale());
 if (State.Advance(RequestedStage(), Dt, Held) && !bDevOverride) M->RequestAutosave();
 // An early miss gets one further natural opportunity on the first field trip.
 // Loading, opening a door and simply walking outside never restart a track.
 if (!bDevOverride && !Held && M->Evening && M->Evening->ServicePending() && M->World->bDriving &&
  M->World->Area == TEXT("road") && State.OfferEarlyPass()) M->RequestAutosave();
 if (!State.Stage || State.Completed[State.Stage - 1] || M->World->Area == TEXT("room6")) { Hide(); return; }
 Build();
 if (!bBuilt || Points.Num() != 9) { Hide(); return; }
 const FS47Cam Cam = FS47Cam::Of(M->Player, 12000);
 FVector2D Size(1600, 900);
 if (M->Web && M->Web->GetWorld() && M->Web->GetWorld()->GetGameViewport()) M->Web->GetWorld()->GetGameViewport()->GetViewportSize(Size);
 const double Fade = FMath::Clamp(State.Elapsed / 3, 0., 1.) * FMath::Clamp((FS47SkyVisitorState::Duration(State.Stage) - State.Elapsed) / 8, 0., 1.);
 const int32 Count = FS47SkyVisitorState::Count(State.Stage);
 for (int32 I = 0; I < Points.Num(); ++I)
 {
  auto* B = Points[I].Get(); if (!B) continue;
  B->SetVisibility(I < Count && Fade > 0);
  if (I >= Count || B->Elements.IsEmpty()) continue;
  const FVector At = WorldPosition(FS47SkyVisitorState::MapPosition(State.Stage, I, State.Elapsed), M->World->Area);
  B->SetWorldLocation(S47Three::ToUE(At));
  // A soft six-pixel disc stays distinct from the existing star points. Its
  // world location and depth remain physical; only the distant glow keeps size.
  const float Half = float(6.4 * FMath::Tan(FMath::DegreesToRadians(Cam.Fov) / 2) * FVector::Dist(Cam.Pos, At) / FMath::Max(1., Size.Y) * 100);
  B->Elements[0].BaseSizeX = B->Elements[0].BaseSizeY = Half;
  if (auto* D = Cast<UMaterialInstanceDynamic>(B->Elements[0].Material))
   D->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(I % 3 == 2 ? 0xffdfb4 : 0xe3f2ff) * float(Fade * 4.0));
  B->MarkRenderStateDirty();
 }
 if (Held || Dt <= 0) { State.Looking = 0; ObservationStep = 0; return; }
 ObservationStep += Dt;
 if (ObservationStep < .12) return;
 const double Sample = FMath::Min(ObservationStep, .2); ObservationStep = 0;
 int32 Visible = 0;
 if (Fade >= .6)
  for (int32 I = 0; I < Count; ++I)
  {
   const auto* B = Points[I].Get();
   if (!B || !B->IsRegistered() || !B->IsVisible() || B->Elements.IsEmpty() || !B->Elements[0].Material) continue;
   const FVector At = WorldPosition(FS47SkyVisitorState::MapPosition(State.Stage, I, State.Elapsed), M->World->Area);
   const FVector P = Cam.Project(At);
   if (P.Z < 1 && FMath::Abs(P.X) < .94 && FMath::Abs(P.Y) < .94 && ClearLine(Cam.Pos, At)) ++Visible;
  }
 if (State.Observe(Visible, Sample)) RecordObservation(Visible);
}

bool FS47SkyVisitors::Check(bool Good, const FString& What)
{
 ++ContractChecks;
 UE_LOG(LogS47SkyVisitors, Display, TEXT("S47: sky contract %s: %s"), Good ? TEXT("PASS") : TEXT("FAIL"), *What);
 if (Good) return true;
 bContractDone = true;
 auto O = S47J::New(); S47J::Set(O,TEXT("status"),TEXT("FAIL")); S47J::Set(O,TEXT("reason"),What);
 S47J::Set(O,TEXT("phase"),ContractPhase); S47J::Set(O,TEXT("checks"),ContractChecks);
 if (!ContractRoot.IsEmpty()) FFileHelper::SaveStringToFile(S47J::Write(O,true), *(ContractRoot / TEXT("result.json")));
 FPlatformMisc::RequestExitWithStatus(false, 1); return false;
}
void FS47SkyVisitors::ControlTest(double Dt)
{
 if (!bContract || bContractDone || !M || !M->Flow || !M->World || !M->Player || !M->Story) return;
 ContractTime += Dt;
 if (ContractTime > 100) { Check(false,TEXT("world sight/save contract timed out")); return; }
 if (!ContractPhase)
 {
  FString User;
  FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
  if (ContractRoot.IsEmpty() || !ContractRoot.StartsWith(TEXT("/tmp/")) || !User.StartsWith(TEXT("/tmp/")) || M->Flow->CaseId != 0)
  { bContractDone = true; UE_LOG(LogS47SkyVisitors,Error,TEXT("S47: sky contract requires developer case zero and /tmp proof/UserDir")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
  IFileManager::Get().MakeDirectory(*ContractRoot,true);
  M->World->Enter(TEXT("saro")); M->Player->Place(-1,-18.5,0); M->Player->Pitch = -.5;
  State.Begin(nullptr,true); TestStage = 1; ContractPhase = 1; ContractSince = ContractTime;
  return;
 }
 const double Since = ContractTime - ContractSince;
 if (ContractPhase == 1)
 {
  if (Since < 4) return;
  if (!Check(State.Occurred[0] && !State.Observed[0],TEXT("early light moves in the world while looking down without journal credit"))) return;
  const FVector Target = WorldPosition(FS47SkyVisitorState::MapPosition(1,0,State.Elapsed),TEXT("saro"));
  if (!Check(!ClearLine(FVector(0,1.6,0),Target),TEXT("actual control-room roof or wall occludes the sky sight ray"))) return;
  HeldTime = State.Elapsed; M->bPausedByMenu = true; ContractPhase = 2; ContractSince = ContractTime; return;
 }
 if (ContractPhase == 2)
 {
  if (Since < .8) return;
  if (!Check(State.Elapsed == HeldTime && !State.Observed[0],TEXT("menu pause freezes actual world trajectory and observation"))) return;
  M->bPausedByMenu = false;
  const auto Full = M->Flow->GameState();
  if (!Check(FFileHelper::SaveStringToFile(S47J::Write(Full,true),*(ContractRoot/TEXT("midpass.json"))),TEXT("write real full-case snapshot before seeing the light"))) return;
  FString Bytes; FFileHelper::LoadFileToString(Bytes,*(ContractRoot/TEXT("midpass.json")));
  const auto Saved = S47J::Read(Bytes);
  if (!Check(Saved.IsValid(),TEXT("fresh reader parses the on-disk case"))) return;
  US47Flow::FStart Start; Start.State = Saved; Start.bDev = true; Start.CaseId = 0;
  M->Flow->StartGame(Start);
  if (!Check(State.Elapsed == HeldTime && State.Occurred[0] && !State.Observed[0],TEXT("full Flow restore retains midpass time and unseen status"))) return;
  M->World->Enter(TEXT("saro")); M->Player->Place(-1,-18.5,0);
  ContractPhase = 3; ContractSince = ContractTime;
 }
 // This test follows the physical targets using the ordinary Walker view. Production
 // code never changes the camera or follows player position with the light path.
 FVector Center = FVector::ZeroVector;
 const int32 N = FS47SkyVisitorState::Count(TestStage);
 const int32 AimN = TestStage == 3 ? 2 : N;
 for (int32 I = 0; I < AimN; ++I) Center += WorldPosition(FS47SkyVisitorState::MapPosition(TestStage,I,State.Elapsed),TEXT("saro"));
 Center /= FMath::Max(1,AimN);
 const FVector D = Center - S47Three::ToThree(M->Player->EyeUE());
 M->Player->Yaw = FMath::Atan2(-D.X,-D.Z); M->Player->Pitch = FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
 if (ContractPhase == 3 && Since >= 2)
 {
  // One diagnostic sample after the ordinary Walker has applied the test aim.
  // The close gate fixture deliberately catches the opaque crossrail. Give its
  // screenshot a frame before moving the positive camera fixture farther back.
  ContractPhase = 4;
  const FS47Cam Cam = FS47Cam::Of(M->Player,12000);
  const FVector At = WorldPosition(FS47SkyVisitorState::MapPosition(TestStage,0,State.Elapsed),M->World->Area);
  const FVector Projected = Cam.Project(At);
  FString Sight; const bool Clear = TraceSkySight(M,Cam.Pos,At,&Sight);
  const bool Modal = M->ModalOpen(), Cinematic = M->Story->IsCinematic();
  const auto* Point = Points.IsValidIndex(0) ? Points[0].Get() : nullptr;
  const bool PointVisible = Point && Point->IsVisible(), Registered = Point && Point->IsRegistered();
  const bool Material = Point && !Point->Elements.IsEmpty() && Point->Elements[0].Material;
  const FString ActualPoint = Point ? S47Three::ToThree(Point->GetComponentLocation()).ToString() : TEXT("none");
  UE_LOG(LogS47SkyVisitors,Display,TEXT("S47: sky phase3 diagnostic stage=%d requested=%d elapsed=%.6f looking=%.6f observed=%d completed=%d held=%d modal=%d menu=%d storyPaused=%d cinematic=%d driving=%d area=%s eye=%s forward=%s target=%s projection=%s actorRotation=%s yaw=%.6f pitch=%.6f built=%d pointVisible=%d registered=%d material=%d actualPoint=%s clear=%d trace=%s"),
   State.Stage,RequestedStage(),State.Elapsed,State.Looking,State.Stage > 0 && State.Observed[State.Stage-1],State.Stage > 0 && State.Completed[State.Stage-1],
   M->bPausedByMenu || Modal || Cinematic,Modal,M->bPausedByMenu,M->Story->IsPaused(),Cinematic,M->World->bDriving,*M->World->Area,
   *Cam.Pos.ToString(),*Cam.Forward.ToString(),*At.ToString(),*Projected.ToString(),*M->Player->GetActorRotation().ToString(),
   M->Player->Yaw,M->Player->Pitch,bBuilt,PointVisible,Registered,Material,*ActualPoint,Clear,*Sight);
  auto Diagnostic = S47J::New();
  S47J::Set(Diagnostic,TEXT("stage"),State.Stage); S47J::Set(Diagnostic,TEXT("elapsed"),State.Elapsed);
  S47J::Set(Diagnostic,TEXT("held"),M->bPausedByMenu || Modal || Cinematic); S47J::Set(Diagnostic,TEXT("modal"),Modal);
  S47J::Set(Diagnostic,TEXT("menuPaused"),M->bPausedByMenu); S47J::Set(Diagnostic,TEXT("storyPaused"),M->Story->IsPaused());
  S47J::Set(Diagnostic,TEXT("cinematic"),Cinematic); S47J::Set(Diagnostic,TEXT("area"),M->World->Area);
  S47J::Set(Diagnostic,TEXT("eye"),Cam.Pos.ToString()); S47J::Set(Diagnostic,TEXT("forward"),Cam.Forward.ToString());
  S47J::Set(Diagnostic,TEXT("target"),At.ToString()); S47J::Set(Diagnostic,TEXT("projection"),Projected.ToString());
  S47J::Set(Diagnostic,TEXT("pointVisible"),PointVisible); S47J::Set(Diagnostic,TEXT("registered"),Registered);
  S47J::Set(Diagnostic,TEXT("material"),Material); S47J::Set(Diagnostic,TEXT("actualPoint"),ActualPoint);
  S47J::Set(Diagnostic,TEXT("clearLine"),Clear); S47J::Set(Diagnostic,TEXT("trace"),Sight);
  S47J::Set(Diagnostic,TEXT("skyVisitors"),State.Save());
  FFileHelper::SaveStringToFile(S47J::Write(Diagnostic,true),*(ContractRoot/TEXT("phase3-diagnostic.json")));
  FScreenshotRequest::RequestScreenshot(ContractRoot/TEXT("phase3-diagnostic.png"),false,false);
  if (!Check(FVector2D(Cam.Pos.X+1,Cam.Pos.Z+18.5).Size() < .03,
   TEXT("near-gate negative fixture uses the actual Walker eye after the restore has settled"))) return;
  if (!Check(!Clear && !State.Observed[0],TEXT("actual opaque gate rail occludes the near-gate sky fixture without observation credit"))) return;
  return;
 }
 if (ContractPhase == 4)
 {
  if (Since < 2.15) return;
  if (!Check(M->Player->Walkable(-1,-15),TEXT("positive sky camera-position fixture is a walkable five-metre setback from the gate"))) return;
  M->Player->Place(-1,-15,0);
  UE_LOG(LogS47SkyVisitors,Display,TEXT("S47: sky positive sight camera-position fixture at x=-1 z=-15; authored light path and physical occlusion unchanged"));
  ContractPhase = 5; ContractSince = ContractTime;
  return; // The ordinary Walker tick applies this eye position and the subsequent aim.
 }
 if (State.Stage == TestStage && State.Observed[TestStage - 1] && Since > 4)
 {
  if (!Check(true,FString::Printf(TEXT("SKY%02d seen through actual projected and unoccluded sight"),TestStage))) return;
  if (TestStage == 1)
  {
   // Compare two real film captures at the same pose/seed. Only the world light
   // is hidden for the reference. Journal state is never used as pixel proof.
   if (!Check(M->FieldCamera != nullptr,TEXT("field camera exists for the optional gate photograph"))) return;
   M->FieldCamera->Sync();
   const FVector At = WorldPosition(FS47SkyVisitorState::MapPosition(1,0,State.Elapsed),TEXT("saro"));
   const FS47Projection P = M->FieldCamera->Project(At);
   if (!Check(P.bFront && P.U > .03 && P.U < .97 && P.V > .03 && P.V < .97,TEXT("SKY01 projects inside the actual field camera from the gate"))) return;
   FS47SnapOptions O; O.W = 960; O.Seed = 47;
   const FS47Snap With = M->FieldCamera->Snap(O);
   TArray<bool> Visible;
   for (auto& B : Points) { Visible.Add(B.IsValid() && B->IsVisible()); if (B.IsValid()) B->SetVisibility(false); }
   const FS47Snap Without = M->FieldCamera->Snap(O);
   for (int32 I = 0; I < Points.Num(); ++I) if (Points[I].IsValid()) Points[I]->SetVisibility(Visible[I]);
   if (!Check(With.bRendered && Without.bRendered && With.Px.Num() == Without.Px.Num(),TEXT("two native scene captures render for the photograph comparison"))) return;
   const int32 X = FMath::RoundToInt(P.U * With.W), Y = FMath::RoundToInt(P.V * With.H);
   int32 Pixels = 0, Peak = 0;
   for (int32 V = FMath::Max(0,Y-12); V <= FMath::Min(With.H-1,Y+12); ++V)
    for (int32 U = FMath::Max(0,X-12); U <= FMath::Min(With.W-1,X+12); ++U)
    {
     const FColor A = With.Px[V*With.W+U], B = Without.Px[V*With.W+U];
     const int32 Delta = int32(A.R)+A.G+A.B-int32(B.R)-B.G-B.B;
     Peak = FMath::Max(Peak,Delta); if (Delta >= 20) ++Pixels;
    }
   if (!Check(Pixels >= 2,FString::Printf(TEXT("SKY01 contributes actual photograph pixels near its projection (%d pixels, peak delta %d)"),Pixels,Peak))) return;
   const FString Url = S47Image::ToDataUrl(With.Px,With.W,With.H,false);
   int32 Comma = INDEX_NONE; TArray<uint8> Png;
   if (!Check(Url.FindChar(TEXT(','),Comma) && FBase64::Decode(Url.Mid(Comma+1),Png) &&
    FFileHelper::SaveArrayToFile(Png,*(ContractRoot/TEXT("sky01-field-photo.png"))),TEXT("preserve the native field-camera photograph as PNG"))) return;
  }
  FScreenshotRequest::RequestScreenshot(ContractRoot/FString::Printf(TEXT("sky%02d.png"),TestStage),false,false);
  const auto Snapshot = S47J::Clone(State.Save()); FS47SkyVisitorState Restored; Restored.Begin(Snapshot,false);
  if (!Check(Restored.Observed[TestStage-1] && Restored.Stage == TestStage && Restored.Elapsed == State.Elapsed,TEXT("phase position and actual observation survive serialization"))) return;
  if (TestStage < 5)
  {
   ++TestStage; ContractSince = ContractTime; return;
  }
  bContractDone = true;
  auto O = S47J::New(); S47J::Set(O,TEXT("status"),TEXT("PASS")); S47J::Set(O,TEXT("checks"),ContractChecks);
  S47J::Set(O,TEXT("elapsed"),ContractTime); S47J::Set(O,TEXT("skyVisitors"),State.Save());
  FFileHelper::SaveStringToFile(S47J::Write(O,true),*(ContractRoot/TEXT("result.json")));
  UE_LOG(LogS47SkyVisitors,Display,TEXT("S47: sky contract PASS complete (%d checks)"),ContractChecks);
  M->Story->After(1., [] { FPlatformMisc::RequestExit(false); });
 }
}

bool FS47SkyVisitors::SelfTest(TArray<FString>& Lines)
{
 bool Good = true;
 auto Check = [&](bool B, const TCHAR* Text) { Lines.Add(FString::Printf(TEXT("%s sky visitors: %s"), B ? TEXT("ok") : TEXT("FAIL"), Text)); Good &= B; };
 Check(FS47SkyVisitorState::ScheduledStage(21*3600+29*60,0,true) == 0 &&
  FS47SkyVisitorState::ScheduledStage(21*3600+30*60,0,true) == 1 &&
  FS47SkyVisitorState::ScheduledStage(-2*3600-30*60,0,true) == 1,
  TEXT("first evening light starts at 21:30 on either clock representation"));
 Check(FS47SkyVisitorState::ScheduledStage(18*3600,0,true) == 0 &&
  FS47SkyVisitorState::ScheduledStage(21*3600+30*60,0,false) == 0,
  TEXT("handover alone cannot start a light and unfinished handover has none"));
 Check(FS47SkyVisitorState::ScheduledStage(22*3600+5*60,0,true) == 1 &&
  FS47SkyVisitorState::ScheduledStage(22*3600+50*60,2,true) == 1,
  TEXT("Observing clock jumps retain the first single-light opportunity without early groups"));
 Check(FS47SkyVisitorState::ScheduledStage(2*3600,1,true) == 2 &&
  FS47SkyVisitorState::ScheduledStage(2*3600+15*60,2,true) == 3 &&
  FS47SkyVisitorState::ScheduledStage(3*3600,3,true) == 4 &&
  FS47SkyVisitorState::ScheduledStage(4*3600,5,true) == 5,
  TEXT("later world formations follow actual night chapters"));
 FS47SkyVisitorState S; S.Begin(nullptr, false);
 Check(!S.bEnabled && !S.Advance(5, 2, false), TEXT("legacy cases gain no retroactive sky sequence"));
 S.Begin(nullptr,true);
 S.Advance(FS47SkyVisitorState::ScheduledStage(20*3600+45*60,0,true),1,false);
 Check(S.Stage == 0 && !S.Occurred[0],TEXT("the 20:45 observing run is too early for SKY01"));
 S.Advance(FS47SkyVisitorState::ScheduledStage(22*3600+5*60,0,true),1,false);
 Check(S.Stage == 1 && S.Occurred[0] && !S.Observed[0] && !S.Occurred[1],
  TEXT("a short clock jump across 21:30 starts one optional unseen light, not a later formation"));
 S.Begin(nullptr, true); S.Advance(1, 5, false);
 Check(S.Stage == 1 && S.Occurred[0] && !S.Observed[0] && !S.Completed[0], TEXT("occurrence does not imply observation or completion"));
 const double Before = S.Elapsed; S.Advance(5, 20, true);
 Check(S.Stage == 1 && S.Elapsed == Before, TEXT("pause and modal cannot advance the global phase"));
 S.Observe(0, 20); Check(!S.Observed[0], TEXT("offscreen and occluded light gives no observation"));
 S.Observe(1, .4); S.Observe(0, .1); S.Observe(1, .4);
 Check(!S.Observed[0], TEXT("discontinuous glimpses do not manufacture a sustained view"));
 S.Looking = 0; const auto Early = S.Save(); FS47SkyVisitorState R; R.Begin(S47J::Clone(Early), false);
 Check(R.bEnabled && R.Stage == 1 && R.Elapsed == Before && !R.Observed[0], TEXT("Continue retains the exact route time without resetting the pass"));
 R.Advance(1, 100, false); Check(R.Completed[0] && !R.Observed[0] && R.OfferEarlyPass(), TEXT("missed first pass gets one field-trip opportunity"));
 R.Advance(1, 100, false); Check(!R.OfferEarlyPass(), TEXT("second early pass cannot become a doorway replay"));
 S.Looking = 0; Check(S.Observe(1, .7) && !S.Observe(1, 1), TEXT("actual sustained sight records exactly once"));
 S.Advance(4, 1, false); Check(S.Stage == 4 && !S.Occurred[1] && !S.Observed[2], TEXT("time loss does not queue or fabricate missed stages"));
 Check(!S.Completed[0], TEXT("interrupted earlier trajectory is not marked naturally complete"));
 Check(!S.Observe(2, 1) && S.Observe(3, .7), TEXT("formation claim needs multiple simultaneous visible lights"));
 S.Advance(1, 2, false); Check(S.Stage == 4, TEXT("old clock or chapter cannot rewind the phase"));
 R.Begin(S47J::Clone(S.Save()), false); Check(R.bEnabled && R.Stage == 4 && R.Observed[3], TEXT("observed phase survives a serialized restore"));
 auto Bad = S47J::Clone(S.Save()); Bad->SetBoolField(TEXT("stage"), true); R.Begin(Bad, true);
 Check(!R.bEnabled, TEXT("corrupt bool-to-number phase is rejected atomically"));
 Bad = S47J::Clone(S.Save()); S47J::Set(Bad, TEXT("elapsed"), -1.); R.Begin(Bad, true);
 Check(!R.bEnabled, TEXT("negative or invalid track time grants no sequence"));
 for (int32 P = 1; P <= 5; ++P)
 {
  const FVector A = FS47SkyVisitorState::MapPosition(P, 0, 0), B = FS47SkyVisitorState::MapPosition(P, 0, 40);
  Check(!A.Equals(B, .1) || P == 3, TEXT("overflight is a world trajectory independent of player movement"));
  Check(FS47SkyVisitorState::Count(P) >= FS47SkyVisitorState::RequiredVisible(P), TEXT("each authored sight threshold is physically attainable"));
 }
 const FVector P(55, 700, 1200), Road = WorldPosition(P, TEXT("road")), Site = WorldPosition(P, TEXT("saro"));
 Check(Road.Equals(P + FVector(8000,0,0), 1e-9) && (Site + FVector(24,0,-300)).Equals(P,1e-9), TEXT("SARO and road draw the same geographic path in their distinct world frames"));
 const FVector GateLight = WorldPosition(FS47SkyVisitorState::MapPosition(1,0,FS47SkyVisitorState::Duration(1)/2),TEXT("saro"));
 Check(FMath::Abs(GateLight.X+1) < 1e-9 && GateLight.Y > 300 && GateLight.Z < -600,
  TEXT("single world light crosses the northern sky above the SARO gate sector"));
 return Good;
}
