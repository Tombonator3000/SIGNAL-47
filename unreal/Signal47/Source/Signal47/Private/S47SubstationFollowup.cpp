// Unreal-only physical follow-up. The historical T01 result is never rewritten by this cycle.
#include "S47Substation.h"
#include "S47Evening.h"
#include "S47FieldCamera.h"
#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Office.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Guid.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Followup, Log, All);

namespace
{
 constexpr double ControlSeconds = 3.0, ObserveSeconds = 3.5;
 const FLinearColor Green(.12f, 1.f, .2f), Amber(1.f, .54f, .08f), Dark(.012f, .018f, .015f);
 const FVector PanelAt(7886.4, .38, 527.10), ResponseAt(7886.05, .38, 527.10), AuxAt(7885.70, .38, 527.10);
 FString FieldStamp(double C) { return FString(C < 0 ? TEXT("13 APR 1986 / ") : TEXT("14 APR 1986 / ")) + S47Text::ClockText(S47Timeline::Day(C), true); }
 bool Time(double C) { return FMath::IsFinite(C); }
 S47J::FObj Point(const FVector& P)
 {
  auto O = S47J::New(); S47J::Set(O, TEXT("x"), P.X); S47J::Set(O, TEXT("y"), P.Y); S47J::Set(O, TEXT("z"), P.Z); return O;
 }
}

FVector FS47Substation::FollowupReferencePosition() { return FVector(7879, -1.35, 520.5); }

TArray<TPair<FString, FVector>> FS47Substation::FollowupFeatures() const
{
 // Pole A and both OUTER conductors are the actual native site geometry. The third phase stays.
 // The midpoint includes the authored .65 m sag. The two white sleeves identify a separate
 // low reference post outside the fence; the actual nearest power pole remains unchanged.
 return {{TEXT("culvert"), FVector(7882, -.65, 527.8)},
  {TEXT("white-band-1"), FVector(7880.35, -.090012, 527.65)}, {TEXT("white-band-2"), FVector(7880.35, .409988, 527.65)},
  {TEXT("nearest-pole"), FVector(7890, 7.859988, 558)},
  {TEXT("outer-conductor-1"), FVector(7888.96, 6.654994, 549.425)},
  {TEXT("outer-conductor-2"), FVector(7891.04, 6.654994, 549.425)},
  {TEXT("fence-corner"), Site(FVector(10, -7.2, 1.2))},
  {TEXT("marked-lamp"), Site(FVector(4.62, -9.01, 1.70))}};
}

TArray<FS47Obj> FS47Substation::FollowupOccluders() const
{
 TArray<FS47Obj> Out;
 for (const TCHAR* Name : {TEXT("Transformer"), TEXT("Buswork"), TEXT("Control"), TEXT("StatusLamp"), TEXT("Lever"),
  TEXT("Fence"), TEXT("Gate_L"), TEXT("Gate_R"), TEXT("Pad"), TEXT("Gravel"),
  TEXT("SitePole_A"), TEXT("SitePole_B"), TEXT("SitePole_C"), TEXT("SiteShed"), TEXT("SiteEquipment"), TEXT("SiteLines"), TEXT("SiteGround")})
 {
  const FS47Obj O = Root.Find(FString(TEXT("prop:substation")) + Name); if (O) Out.Add(O);
 }
 // These are only visible meshes made by FollowupProp. Target()'s invisible use proxies
 // are deliberately absent, so actual fixtures/markers/sleeves can occlude one another.
 for (const auto& O : FollowupVisuals) if (O && O.Shown()) Out.Add(O);
 return Out;
}

bool FS47Substation::FollowupAvailable() const
{
 return bReady && bFollowupBuilt && M && M->Story && M->Evening && M->Evening->Enabled() &&
  (M->Story->ChapterKey == TEXT("ch3") || M->Story->ChapterKey == TEXT("ch4")) &&
  M->Office && M->Office->HasFieldSheet() && M->Evening->Pack.Flag(TEXT("office_evidence_taken"));
}

bool FS47Substation::FollowupCameraContext() const
{
 return FollowupAvailable() && Here() && !M->Player->InAir() &&
  FVector2D(M->Player->Pos.X - FollowupReferencePosition().X, M->Player->Pos.Z - FollowupReferencePosition().Z).Size() < 12;
}

bool FS47Substation::FollowupRecordValid() const
{
 return !AttemptId.IsEmpty() && AttemptNumber > 0 && HasNormalResult() &&
  Time(AttemptAt) && Time(BaselineAt) && Time(BaselineObservedAt) &&
  BaselineAt == CompletedAt && BaselineObservedAt == ObservedAt && BaselineStamp == CompletedStamp &&
  BaselineObservedAt >= BaselineAt && AttemptAt >= BaselineObservedAt;
}

bool FS47Substation::ReferenceMatched() const
{
 return FollowupRecordValid() && bReferenceArmed && bReferenceMatched && Time(ReferenceAt) && ReferenceAt >= AttemptAt && ReferenceViewValid();
}

bool FS47Substation::ReferenceViewValid() const
{
 using namespace S47J;
 if (!ReferenceView || Str(ReferenceView, TEXT("attemptId")) != AttemptId || !Has(ReferenceView, TEXT("clock")) || Num(ReferenceView, TEXT("clock")) != ReferenceAt) return false;
 const auto P = Obj(ReferenceView, TEXT("pose")); const auto Eye = Obj(ReferenceView, TEXT("eye")); const auto Points = Obj(ReferenceView, TEXT("features"));
 const auto Rotation = Nums(ReferenceView,TEXT("cameraRotation"));
 if (Arr(ReferenceView,TEXT("cameraRotation")).Num() != 3 || Rotation.Num() != 3 || !Time(Rotation[0]) || !Time(Rotation[1]) || !Time(Rotation[2])) return false;
 const FRotator Camera(Rotation[0],Rotation[1],Rotation[2]);
 if (!P || !Eye || !Points || !Has(ReferenceView, TEXT("floorY")) || FMath::Abs(Num(ReferenceView, TEXT("floorY")) - FollowupReferencePosition().Y) > .45) return false;
 const FVector Position(Num(P,TEXT("x")), Num(P,TEXT("y")), Num(P,TEXT("z")));
 const FVector SourceEye(Num(Eye,TEXT("x")), Num(Eye,TEXT("y")), Num(Eye,TEXT("z")));
 const double Floor = Num(ReferenceView,TEXT("floorY"));
 if (Position.ContainsNaN() || SourceEye.ContainsNaN() || FVector2D(Position.X - FollowupReferencePosition().X, Position.Z - FollowupReferencePosition().Z).Size() > FollowupReferenceRadius() ||
  FVector2D(SourceEye.X - Position.X, SourceEye.Z - Position.Z).Size() > .25 || SourceEye.Y - Floor < 1.3 || SourceEye.Y - Floor > 1.95) return false;
 for (const auto& F : FollowupFeatures())
 {
  const auto Feature = Obj(Points, *F.Key); const auto World = Obj(Feature,TEXT("world")); const auto UV = Uv(Feature,TEXT("uv"));
  if (!World || !UV || !FVector(Num(World,TEXT("x")),Num(World,TEXT("y")),Num(World,TEXT("z"))).Equals(F.Value,.001) ||
   !FMath::IsFinite(UV->X) || !FMath::IsFinite(UV->Y) || UV->X < .02 || UV->X > .98 || UV->Y < .02 || UV->Y > .98) return false;
  const FVector Local = Camera.UnrotateVector(S47Three::ToUE(F.Value) - S47Three::ToUE(SourceEye));
  if (Local.ContainsNaN() || Local.X <= .001) return false;
  const double TanV = FMath::Tan(US47FieldCamera::PHOTO_FOV * PI / 360);
  const FVector2D Projected(((Local.Y / Local.X) / (TanV * US47FieldCamera::PHOTO_W / US47FieldCamera::PHOTO_H) + 1) / 2,
   (1 - (Local.Z / Local.X) / TanV) / 2);
  if ((Projected - *UV).Size() > .0025) return false;
 }
 return true;
}

bool FS47Substation::FollowupControlsValid() const
{
 return ReferenceMatched() && bIsolated && bIsolationResponseSeen && bBackupExcluded && bAuxiliaryPulseSeen && bIndicatorCompared && bLampObserved &&
  Time(IsolationAt) && Time(BackupAt) && Time(IndicatorAt) && Time(LampAt) &&
  IsolationAt >= ReferenceAt && BackupAt >= IsolationAt && IndicatorAt >= BackupAt && LampAt >= IndicatorAt &&
  LampViewSeconds >= ObserveSeconds && FMath::IsFinite(LampViewMin) && FMath::IsFinite(LampViewMax) && LampViewMax - LampViewMin >= .25f;
}

bool FS47Substation::FollowupVerified() const
{
 return M && M->Office && M->Office->HasFieldSheet() && FollowupControlsValid() && FollowupPhoto &&
  FollowupPhoto->Valid(AttemptId, FollowupReferencePosition(), FollowupReferenceRadius(), FollowupFeatures()) &&
  FollowupPhoto->Photo.Clock >= LampAt;
}

bool FS47Substation::CanCaptureFollowup(FString& Reason) const
{
 if (!FollowupCameraContext()) { Reason = TEXT("THE FOLLOW-UP CAMERA IS FOR CALICHE'S MARKED REFERENCE POSITION"); return false; }
 if (!ReferenceMatched()) { Reason = TEXT("MATCH THE CULVERT, TWO WHITE BANDS AND POLE TO DALE'S SHEET FIRST"); return false; }
 if (!FollowupControlsValid()) { Reason = TEXT("COMPLETE ISOLATION, AUXILIARY, INDICATOR AND VISIBLE-LAMP COMPARISONS FIRST"); return false; }
 if (bFollowupFiled) { Reason = TEXT("THIS FOLLOW-UP AND ITS ORIGINAL PHOTOGRAPH ARE ALREADY FILED"); return false; }
 Reason = TEXT("T02 / CONTROLLED COMPARISON READY"); return true;
}

bool FS47Substation::OnFollowupCapture(const FS47T02PhotoRecord& Photo)
{
 FString Why;
 if (!CanCaptureFollowup(Why) || !Photo.Valid(AttemptId, FollowupReferencePosition(), FollowupReferenceRadius(), FollowupFeatures()) ||
  Photo.Photo.Clock < LampAt || !M || !M->Story || FMath::Abs(Photo.Photo.Clock - M->Story->Clock()) > .25) return false;
 // A valid old exposure cannot be relabelled as a new photograph after moving the player.
 if (!M->Player || !Photo.SourcePosition.Equals(M->Player->Pos, .035) || !Photo.SourceEye.Equals(S47Three::ToThree(M->Player->EyeUE()), .035)) return false;
 FollowupPhoto = Photo; SyncFollowupFacts(); M->RequestAutosave();
 M->UI->Toast(TEXT("T02 instant-back exposed. Its time, original standing position and reference features are retained. File the card and photograph at SARO."), 5);
 UE_LOG(LogS47Followup, Display, TEXT("S47: T02 genuine exposure retained attempt=%s photo=%s clock=%.3f"), *AttemptId, *Photo.Photo.Id, Photo.Photo.Clock);
 return true;
}

FS47Obj FS47Substation::FollowupProp(const FString& Name, const FVector& At, const FVector& Size, const FLinearColor& Color, TWeakObjectPtr<UMaterialInstanceDynamic>* OutMid)
{
 const FS47WebNode* Parent = M && M->Web ? M->Web->Node(Root.I) : nullptr;
 UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
 UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 if (!Parent || !Parent->Comp.IsValid() || !Mesh || !Base) return {};
 UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(M->Web, *Name); C->SetMobility(EComponentMobility::Movable);
 C->SetStaticMesh(Mesh); C->SetupAttachment(Parent->Comp.Get()); C->SetWorldLocation(S47Three::ToUE(At)); C->SetWorldScale3D(S47Three::DirToUE(Size));
 C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore); C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
 UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Base, C); if (!D) return {};
 D->SetVectorParameterValue(TEXT("Color"), Color); D->SetScalarParameterValue(TEXT("Roughness"), .76f); D->SetVectorParameterValue(TEXT("EmissiveFactor"), FLinearColor::Black); C->SetMaterial(0, D);
 C->RegisterComponent(); if (OutMid) *OutMid = D;
 const FS47Obj Object = M->Web->ObjAt(M->Web->AdoptNode(C, Root.I, Name));
 if (Object) FollowupVisuals.Add(Object);
 return Object;
}

void FS47Substation::InstallFollowup()
{
 if (bFollowupBuilt || !bReady || !M || !M->Inter || !Root.Find(TEXT("prop:substationSitePole_A")) || !Root.Find(TEXT("prop:substationSiteLines"))) return;
 // A small real concrete culvert at the existing apron. The opening has sides/top and a dark back.
 const FLinearColor Concrete(.40f, .39f, .34f), White(.88f, .86f, .72f), Metal(.17f, .22f, .20f);
 FollowupProp(TEXT("T02CulvertTop"), FVector(7882, -.28, 527.9), FVector(1.7, .18, 1.0), Concrete);
 FollowupProp(TEXT("T02CulvertLeft"), FVector(7881.25, -.70, 527.9), FVector(.2, .72, 1.0), Concrete);
 FollowupProp(TEXT("T02CulvertRight"), FVector(7882.75, -.70, 527.9), FVector(.2, .72, 1.0), Concrete);
 FollowupProp(TEXT("T02CulvertInterior"), FVector(7882, -.67, 528.35), FVector(1.3, .64, .04), FLinearColor(.018f, .018f, .015f));
 for (const TCHAR* Area : {TEXT("road"),TEXT("diner")})
 {
  auto& Colliders = M->Web->CollidersMutable(Area);
  if (!Colliders.ContainsByPredicate([](const FS47Rect& B) { return B.Id == TEXT("substation:t02:culvert"); })) Colliders.Add({7881.15,7882.85,527.4,528.4,TEXT("substation:t02:culvert"),true});
  if (!Colliders.ContainsByPredicate([](const FS47Rect& B) { return B.Id == TEXT("substation:t02:band-post"); })) Colliders.Add({7880.26,7880.44,527.56,527.74,TEXT("substation:t02:band-post"),true});
 }
 // Both old pole-mounted bands were actually occluded by the fence/shed. This real local
 // reference post has its own small foot blocker and reaches below measured ground.
 const FS47Obj BandPost = FollowupProp(TEXT("T02BandReferencePost"), FVector(7880.35, -.485, 527.65), FVector(.10, 2.23, .10), Metal);
 const FS47Obj Band1 = FollowupProp(TEXT("T02PoleWhiteBand1"), FVector(7880.35, -.090012, 527.65), FVector(.22, .20, .22), White);
 const FS47Obj Band2 = FollowupProp(TEXT("T02PoleWhiteBand2"), FVector(7880.35, .409988, 527.65), FVector(.22, .20, .22), White);
 FVector FootMark = FollowupReferencePosition(); FootMark.Y = -1.329497906; // measured terrain: bottom 2 mm above its highest sampled corner
 FollowupProp(TEXT("T02ReferenceFootMark"), FootMark, FVector(.9, .025, .65), White);
 ReferenceMarker = FollowupProp(TEXT("T02ReferenceMarker"), FVector(7879, -.665939737, 522.1), FVector(.30, 1.40, .12), Metal);
 FollowupProp(TEXT("T02EnclosedFixture"), FVector(7886.05, .05, 527.30), FVector(1.22, 1.05, .28), Metal);
 FollowupPanel = FollowupProp(TEXT("T02PanelIndicator"), PanelAt, FVector(.20, .19, .08), Dark, &PanelMid);
 IsolationResponse = FollowupProp(TEXT("T02IsolationResponse"), ResponseAt, FVector(.20, .19, .08), Dark, &IsolationMid);
 AuxiliaryResponse = FollowupProp(TEXT("T02AuxiliaryResponse"), AuxAt, FVector(.20, .19, .08), Dark, &AuxiliaryMid);
 FS47Obj Handle = FollowupProp(TEXT("T02IsolationHandle"), FVector(7886.40, -.03, 527.05), FVector(.10, .32, .10), White);
 FS47Obj AuxHandle = FollowupProp(TEXT("T02AuxiliaryHandle"), FVector(7886.05, -.03, 527.05), FVector(.12, .12, .10), Amber);
 FS47Obj Record = FollowupProp(TEXT("T02ReportPlate"), FVector(7885.70, -.05, 527.04), FVector(.22, .24, .06), White);
 if (!BandPost || !Band1 || !Band2 || !ReferenceMarker || !FollowupPanel || !IsolationResponse || !AuxiliaryResponse || !Handle || !AuxHandle || !Record) return;
 if (const FS47WebNode* N = M->Web->Node(Handle.I)) { IsolationHandle = Cast<UStaticMeshComponent>(N->Comp.Get()); if (IsolationHandle.IsValid()) IsolationRest = IsolationHandle->GetRelativeTransform(); }
 if (const FS47WebNode* N = M->Web->Node(AuxHandle.I)) { AuxiliaryHandle = Cast<UStaticMeshComponent>(N->Comp.Get()); if (AuxiliaryHandle.IsValid()) AuxiliaryRest = AuxiliaryHandle->GetRelativeTransform(); }
 auto Add = [this](const TCHAR* Id, FS47Obj Object, TFunction<FString()> Label, TFunction<void()> Use, TFunction<FString()> Examine, double Range = 2.8)
 {
  FS47Interactable I; I.Id = Id; I.Object = Object; I.Range = Range; I.ExamineRange = 8; I.Label = MoveTemp(Label); I.Use = MoveTemp(Use); I.Examine = MoveTemp(Examine); M->Inter->Add(I);
 };
 Add(TEXT("substation:t02:reference"), ReferenceMarker,
  [this] { return FollowupAvailable() && Here() ? FString(bFollowupFiled && FiledResolution == TEXT("verified") ? TEXT("Read filed reference") : bFollowupFiled ? TEXT("Start a new comparison after unresolved filing") : AttemptId.IsEmpty() ? TEXT("Compare Dale's field sheet / begin follow-up") : TEXT("Reference position / restart unresolved comparison")) : FString(); },
  [this] { StartFollowup(); }, [] { return FString(TEXT("FIELD CHECKS / STANDING POSITION. Concrete culvert at the apron; two white bands on the separate metal reference post; the nearest power pole, its two outer conductors and the fence corner. Stay outside the fence. Raise the camera and match the whole reference.")); });
 Add(TEXT("substation:t02:isolation"), Handle,
  [this] { return FollowupAvailable() && Here() ? FString(FollowupRunning == 1 ? TEXT("Enclosed isolation test running") : bIsolated ? TEXT("Isolation confirmed / independent flag open") : TEXT("Run enclosed local isolation test")) : FString(); },
  [this] { StartIsolation(); }, [] { return FString(TEXT("ENCLOSED LOCAL TEST FIXTURE / ISOLATION. The left panel window and separate centre mechanical-response window must both reach OPEN. This controls only the marked fictional inspection circuit. No yard entry or exposed electrical work.")); });
 Add(TEXT("substation:t02:auxiliary"), AuxHandle,
  [this] { return FollowupAvailable() && Here() ? FString(FollowupRunning == 2 ? TEXT("Auxiliary test running / watch the right window") : bBackupExcluded ? TEXT("Auxiliary check complete / no local feed") : TEXT("Run separate auxiliary-feed test")) : FString(); },
  [this] { StartAuxiliary(); }, [] { return FString(TEXT("AUXILIARY TEST / RIGHT WINDOW. A separate local test pulse must show bright then return dark while the isolation response stays OPEN. It tests this ordinary backup explanation; it says nothing about the town supply.")); });
 Add(TEXT("substation:t02:indicator"), FollowupPanel,
  [this] { return FollowupAvailable() && Here() ? FString(TEXT("Compare panel indicator with independent isolation flag")) : FString(); },
  [this] { CompareIndicator(); }, [this] { return FString(bIsolated ? TEXT("PANEL: OPEN. The separate centre response window shows the physical isolation flag. Compare both with the auxiliary result before observing the marked lamp.") : TEXT("PANEL: NOT TESTED. The independent isolation flag has not completed a test.")); });
 Add(TEXT("substation:t02:record"), Record,
  [this] { return FollowupAvailable() && Here() ? FString(TEXT("Review follow-up / keep unresolved")) : FString(); },
  [this]
  {
   if (!FollowupAvailable() || !Here()) return;
   if (!ReferenceMatched()) { M->UI->Toast(TEXT("First identify Dale's standing position against the real reference features."), 4); return; }
   SyncFollowupFacts();
   if (const auto* Scene = M->Evening->Pack.CurrentScene())
   {
    if (Scene->Id.StartsWith(TEXT("T02_"))) M->Evening->ShowDialogue();
   }
   else if (M->Evening->Pack.Flag(TEXT("trafo_anomaly_checked")) || FollowupFiled()) M->UI->Document(FollowupDocument(true));
   else M->Evening->Trigger(TEXT("T02_CHECKS"));
  }, [] { return FString(TEXT("Keep an incomplete comparison unresolved. The STATION 01 field roll and Nora's route remain available. A verified disagreement also needs its own real reference photograph and return filing at SARO.")); });
 bFollowupBuilt = true;
 UE_LOG(LogS47Followup, Display, TEXT("S47: T02 physical fixture ready; reference=(7879,-1.35,520.5), separate band post / real pole A / outer conductors; T01 history preserved"));
}

void FS47Substation::StartFollowup()
{
 if (!FollowupAvailable() || !Here() || !M->UI) return;
 if (bFollowupFiled && FiledResolution == TEXT("verified")) { M->UI->Document(FollowupDocument(true)); return; }
 if (!HasNormalResult()) { M->UI->Toast(TEXT("No physical normal baseline is recorded. Read the journal, run the ordinary local service check and compare its actual lamp first. That new field baseline will be dated now."), 5); return; }
 if (M->Player && FVector2D(M->Player->Pos.X - FollowupReferencePosition().X, M->Player->Pos.Z - FollowupReferencePosition().Z).Size() > FollowupReferenceRadius())
 { M->UI->Toast(TEXT("Stand on the white apron mark beside the culvert before starting a new reference comparison."), 4); return; }
 M->Evening->Pack.ResetFollowupAttempt();
 // A second physical start makes a genuinely new attempt; no stale controls or picture survive it.
 ++AttemptNumber; AttemptId = FString::Printf(TEXT("T02-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
 AttemptAt = M->Story->Clock(); BaselineAt = CompletedAt; BaselineObservedAt = ObservedAt; BaselineStamp = CompletedStamp;
 bReferenceArmed = true; bReferenceMatched = bIsolated = bBackupExcluded = bIndicatorCompared = bLampObserved = bFollowupFiled = false;
 bIsolationResponseSeen = bAuxiliaryPulseSeen = false; ReferenceView.Reset();
 FiledResolution.Empty();
 ReferenceAt = IsolationAt = BackupAt = IndicatorAt = LampAt = FiledAt = 0;
 FollowupRunning = 0; FollowupElapsed = FollowupVisualTime = LampViewSeconds = 0; LampViewMin = TNumericLimits<float>::Max(); LampViewMax = 0; FollowupPhoto.Reset();
 SyncFollowupFacts(); Apply(); M->RequestAutosave();
 M->UI->Toast(TEXT("Match the culvert, both white bands on the reference post, the nearest power pole, its two outer conductors and fence corner. Use the camera from the standing mark; opening the sheet alone proves nothing."), 6);
 UE_LOG(LogS47Followup, Display, TEXT("S47: T02 physical comparison started attempt=%s baseline=%s"), *AttemptId, *BaselineStamp);
}

void FS47Substation::StartIsolation()
{
 if (!FollowupAvailable() || !Here() || !M->UI || FollowupRunning) return;
 if (!ReferenceMatched()) { M->UI->Toast(TEXT("Match the real reference view from the standing mark first."), 4); return; }
 if (bIsolated) { M->UI->Toast(TEXT("This attempt's enclosed isolation is already complete. The independent flag remains OPEN."), 4); return; }
 FollowupRunning = 1; FollowupElapsed = .001; Apply(); M->RequestAutosave();
 M->UI->Toast(TEXT("ENCLOSED ISOLATION / TEST RUNNING. Watch the panel window and independent centre response flag."), 4);
}

void FS47Substation::StartAuxiliary()
{
 if (!FollowupAvailable() || !Here() || !M->UI || FollowupRunning) return;
 if (!ReferenceMatched() || !bIsolated) { M->UI->Toast(TEXT("Complete the separate enclosed isolation test first."), 4); return; }
 if (bBackupExcluded) { M->UI->Toast(TEXT("Auxiliary test complete for this attempt. The right window is dark; it provides no local feed."), 4); return; }
 FollowupRunning = 2; FollowupElapsed = .001; Apply(); M->RequestAutosave();
 M->UI->Toast(TEXT("AUXILIARY / INDEPENDENT TEST. Watch the right window's test pulse and return to dark."), 4);
}

bool FS47Substation::FollowupVisible(const FVector& Point, double MaxDistance) const
{
 if (!Here() || !M->Player || !M->FieldCamera || !Root.Shown()) return false;
 const FVector Eye = S47Three::ToThree(M->Player->EyeUE()), D = Point - Eye;
 if (D.Size() > MaxDistance || D.Size() < .1 || FVector::DotProduct(M->Player->ForwardUE(), S47Three::DirToUE(D.GetSafeNormal())) < .62) return false;
 M->FieldCamera->Sync(); return M->FieldCamera->ClearLine(Point, FollowupOccluders());
}

void FS47Substation::CompareIndicator()
{
 if (!FollowupAvailable() || !Here() || !M->UI) return;
 if (!ReferenceMatched() || !bIsolated || !bBackupExcluded || FollowupRunning)
 { M->UI->Toast(TEXT("Complete both independent controls before comparing the panel with the separate isolation response."), 4); return; }
 UMaterialInstanceDynamic* P = PanelMid.Get(); UMaterialInstanceDynamic* I = IsolationMid.Get(); UMaterialInstanceDynamic* A = AuxiliaryMid.Get();
 if (!P || !I || !A || !P->K2_GetVectorParameterValue(TEXT("EmissiveFactor")).Equals(Green * 2.f, .001f) ||
  !I->K2_GetVectorParameterValue(TEXT("EmissiveFactor")).Equals(Green * 2.f, .001f) ||
  !A->K2_GetVectorParameterValue(TEXT("EmissiveFactor")).Equals(FLinearColor::Black, .001f) ||
  !FollowupVisible(PanelAt, 4) || !FollowupVisible(ResponseAt, 4) || !FollowupVisible(AuxAt, 4))
 { M->UI->Toast(TEXT("Keep the panel, independent centre flag and dark auxiliary window visible together."), 4); return; }
 if (!bIndicatorCompared) { bIndicatorCompared = true; IndicatorAt = M->Story->Clock(); M->RequestAutosave(); }
 SyncFollowupFacts();
 M->UI->Toast(TEXT("PANEL OPEN / independent flag OPEN / auxiliary dark. Now return to the reference mark and observe the separate inspection lamp's whole visible sequence."), 5);
}

void FS47Substation::ObserveFollowupLamp()
{
 if (!FollowupAvailable() || !Here() || !M->UI) return;
 if (!bIndicatorCompared) { M->UI->Toast(TEXT("The lamp is not the panel result. Complete and compare the enclosed isolation and auxiliary controls first."), 4); return; }
 M->UI->Toast(bLampObserved ? TEXT("The persistent local sequence is recorded for this attempt. Take its own real reference photograph, then keep the comparison or leave it unresolved.")
  : TEXT("Observe the marked lamp from Dale's standing mark for one full slow bright/dim sequence. Keep its actual lens unobstructed in view."), 5);
}

float FS47Substation::FollowupLampStrength() const
{
 if (!bIsolated) return 4.f;
 // Slow continuous variation, not a flashing effect. The enclosed local result remains OPEN.
 return 2.1f + .9f * FMath::Sin(float(FollowupVisualTime * PI / 2.8));
}

bool FS47Substation::FollowupLampRendered() const
{
 const float K = FollowupLampStrength(); const FLinearColor Warm(1.f, .62f, .18f);
 if (!bReady || !Root.Shown() || !Light.IsValid() || !Light->IsVisible() || FMath::Abs(Light->Intensity - 95.f * K / 4.f) > .1f || LampMids.IsEmpty()) return false;
 for (const auto& W : LampMids) if (!W.IsValid() || !W->K2_GetVectorParameterValue(TEXT("EmissiveFactor")).Equals(Warm * K, .003f)) return false;
 return true;
}

void FS47Substation::ApplyFollowup()
{
 if (!bFollowupBuilt) return;
 const bool Active = FollowupRecordValid();
 if (UMaterialInstanceDynamic* D = PanelMid.Get()) D->SetVectorParameterValue(TEXT("EmissiveFactor"), Active && bIsolated ? Green * 2.f : Active && FollowupRunning == 1 ? Amber * 2.f : FLinearColor::Black);
 if (UMaterialInstanceDynamic* D = IsolationMid.Get()) D->SetVectorParameterValue(TEXT("EmissiveFactor"), Active && bIsolated ? Green * 2.f : FLinearColor::Black);
 if (UMaterialInstanceDynamic* D = AuxiliaryMid.Get()) D->SetVectorParameterValue(TEXT("EmissiveFactor"), Active && FollowupRunning == 2 && FollowupElapsed > .5 && FollowupElapsed < 2.4 ? Amber * 3.f : FLinearColor::Black);
 if (UStaticMeshComponent* C = IsolationHandle.Get()) { FTransform T = IsolationRest; T.SetRotation(IsolationRest.GetRotation() * FQuat(FVector::ForwardVector, Active && (bIsolated || FollowupRunning == 1) ? -.72 : 0)); C->SetRelativeTransform(T); }
 if (UStaticMeshComponent* C = AuxiliaryHandle.Get()) { FTransform T = AuxiliaryRest; if (Active && FollowupRunning == 2) T.AddToTranslation(FVector(0, 3, 0)); C->SetRelativeTransform(T); }
}

void FS47Substation::StepFollowup(double Dt)
{
 if (!bFollowupBuilt) InstallFollowup();
 if (!M || !M->Story || M->Mode != US47Main::EMode::Play || (M->Flow && M->Flow->bStarting) || !FollowupRecordValid()) return;
 const bool Tick = !M->ModalOpen() && !M->Story->IsCinematic() && !M->bPausedByMenu;
 if (!Tick) return;
 FollowupVisualTime += FMath::Clamp(Dt, 0.0, .25);
 if (bReferenceArmed && !bReferenceMatched && FollowupCameraContext())
 {
  FString Why;
  if (FS47T02Photo::CheckView(M, Why))
  {
   ReferenceAt = M->Story->Clock(); ReferenceView = S47J::New();
   S47J::Set(ReferenceView,TEXT("attemptId"),AttemptId); S47J::Set(ReferenceView,TEXT("clock"),ReferenceAt);
   S47J::Set(ReferenceView,TEXT("pose"),Point(M->Player->Pos)); S47J::Set(ReferenceView,TEXT("eye"),Point(S47Three::ToThree(M->FieldCamera->PhotoLoc)));
   const FRotator Rotation = M->FieldCamera->PhotoRot;
   S47J::Set(ReferenceView,TEXT("cameraRotation"),TArray<double>{Rotation.Pitch,Rotation.Yaw,Rotation.Roll});
   S47J::Set(ReferenceView,TEXT("floorY"),M->Player->FloorY); S47J::Set(ReferenceView,TEXT("yaw"),M->Player->Yaw); S47J::Set(ReferenceView,TEXT("pitch"),M->Player->Pitch);
   auto Features = S47J::New();
   for (const auto& F : FollowupFeatures())
   {
    const auto UV = M->FieldCamera->Project(F.Value); auto Feature = S47J::New(); S47J::Set(Feature,TEXT("world"),Point(F.Value));
    S47J::Set(Feature,TEXT("uv"),TArray<double>{UV.U,UV.V}); S47J::Set(Features,*F.Key,Feature);
   }
   S47J::Set(ReferenceView,TEXT("features"),Features); bReferenceMatched = ReferenceViewValid();
   if (!bReferenceMatched) return;
   Commit(TEXT("T02_REFERENCE")); SyncFollowupFacts(); M->RequestAutosave();
   M->UI->Toast(TEXT("Reference matched against the actual culvert, banded reference post, power pole, outer conductors and fence corner. The enclosed test fixture is beside the ordinary service control."), 5);
  }
 }
 if (FollowupRunning)
 {
  FollowupElapsed += FMath::Clamp(Dt, 0.0, .25); Apply();
  const bool Visible = Here() && FollowupVisible(PanelAt, 4) && FollowupVisible(ResponseAt, 4) && FollowupVisible(AuxAt, 4);
  if (Visible && FollowupRunning == 2 && FollowupElapsed > .5 && FollowupElapsed < 2.4 && AuxiliaryMid.IsValid() &&
   AuxiliaryMid->K2_GetVectorParameterValue(TEXT("EmissiveFactor")).Equals(Amber * 3.f,.001f)) bAuxiliaryPulseSeen = true;
  // Completion is a current visible world result, not the act of pressing its button.
  if (FollowupElapsed >= ControlSeconds && Visible)
  {
   if (FollowupRunning == 2 && !bAuxiliaryPulseSeen)
   {
    FollowupElapsed = .001; M->UI->Toast(TEXT("No observed auxiliary pulse was recorded. The independent test repeats; watch its right-hand window."),4); return;
   }
   const int32 Which = FollowupRunning; FollowupRunning = 0; FollowupElapsed = 0;
   if (Which == 1) { bIsolated = true; IsolationAt = M->Story->Clock(); }
   else { bBackupExcluded = true; BackupAt = M->Story->Clock(); }
   Apply();
   if (Which == 1)
   {
    bIsolationResponseSeen = PanelMid.IsValid() && IsolationMid.IsValid() &&
     PanelMid->K2_GetVectorParameterValue(TEXT("EmissiveFactor")).Equals(Green * 2.f,.001f) && IsolationMid->K2_GetVectorParameterValue(TEXT("EmissiveFactor")).Equals(Green * 2.f,.001f);
    if (!bIsolationResponseSeen) { bIsolated = false; IsolationAt = 0; Apply(); return; }
   }
   else if (!AuxiliaryMid.IsValid() || !AuxiliaryMid->K2_GetVectorParameterValue(TEXT("EmissiveFactor")).Equals(FLinearColor::Black,.001f))
   { bBackupExcluded = false; BackupAt = 0; Apply(); return; }
   SyncFollowupFacts(); M->RequestAutosave();
   M->UI->Toast(Which == 1 ? TEXT("ENCLOSED ISOLATION COMPLETE / panel and independent response OPEN. The separate marked lamp still carries light.") : TEXT("AUXILIARY COMPLETE / its test pulse ended dark. The independent isolation flag remains OPEN. Compare the panel and flag physically."), 5);
   UE_LOG(LogS47Followup, Display, TEXT("S47: T02 physical control completed attempt=%s control=%s clock=%.3f"), *AttemptId, Which == 1 ? TEXT("enclosed-isolation") : TEXT("independent-auxiliary"), M->Story->Clock());
  }
 }
 if (bIndicatorCompared && !bLampObserved && FollowupCameraContext() &&
  FVector2D(M->Player->Pos.X - FollowupReferencePosition().X, M->Player->Pos.Z - FollowupReferencePosition().Z).Size() <= FollowupReferenceRadius() &&
  FollowupVisible(Site(FVector(4.62, -9.01, 1.70)), 15))
 {
  Apply();
  if (FollowupLampRendered())
  {
   LampViewSeconds += FMath::Clamp(Dt, 0.0, .25); LampViewMin = FMath::Min(LampViewMin, FollowupLampStrength()); LampViewMax = FMath::Max(LampViewMax, FollowupLampStrength());
   if (LampViewSeconds >= ObserveSeconds && LampViewMax - LampViewMin >= .25f)
   {
    bLampObserved = true; LampAt = M->Story->Clock(); SyncFollowupFacts(); M->RequestAutosave();
    M->UI->Toast(TEXT("Visible marked-lamp sequence observed under this attempt's isolation and auxiliary controls. Capture the whole matched reference view for its own T02 photograph."), 5);
   }
  }
 }
 else if (!bLampObserved) { LampViewSeconds = 0; LampViewMin = TNumericLimits<float>::Max(); LampViewMax = 0; }
}

void FS47Substation::SyncFollowupFacts()
{
 if (!M || !M->Evening) return;
 const bool Sheet = M->Office && M->Office->HasFieldSheet();
 const bool Base = Sheet && ReferenceMatched();
 auto& P = M->Evening->Pack;
 P.SetExternal(TEXT("ctx_trafo_isolation_verified"), Base && bIsolated && bIsolationResponseSeen && Time(IsolationAt) && IsolationAt >= ReferenceAt);
 P.SetExternal(TEXT("ctx_trafo_backup_excluded"), Base && bIsolated && bIsolationResponseSeen && bBackupExcluded && bAuxiliaryPulseSeen && Time(BackupAt) && BackupAt >= IsolationAt);
 P.SetExternal(TEXT("ctx_trafo_indicator_checked"), Base && bIsolated && bIsolationResponseSeen && bBackupExcluded && bAuxiliaryPulseSeen && bIndicatorCompared && Time(IndicatorAt) && IndicatorAt >= BackupAt);
 P.SetExternal(TEXT("ctx_trafo_lamp_persists"), Sheet && FollowupControlsValid());
 P.SetExternal(TEXT("ctx_trafo_photo_available"), Sheet && FollowupVerified());
 if (Base) Commit(TEXT("T02_REFERENCE"));
}

bool FS47Substation::TryFileFollowup()
{
 if (!M || !M->Player || !M->World || !M->Web || !M->Story || !M->Evening || !M->Office || !M->Office->HasFieldSheet() || M->Mode != US47Main::EMode::Play ||
  M->World->bDriving || M->World->Area != TEXT("saro") || !M->Web->Rect(TEXT("room.bounds")).Contains(M->Player->Pos) || !ReferenceMatched()) return false;
 if (bFollowupFiled) return false;
 // An unavailable current photograph can invalidate proof, never undo the actual historical filing.
 if (FiledHistory.ContainsByPredicate([this](const S47J::FObj& H) { return S47J::Str(H,TEXT("attemptId")) == AttemptId; })) return false;
 const bool Verified = FollowupVerified() && M->Evening->Pack.Flag(TEXT("trafo_anomaly_checked"));
 if (!Verified && !M->Evening->Pack.Flag(TEXT("field_followup_unresolved"))) return false;
 bFollowupFiled = true; FiledResolution = Verified ? TEXT("verified") : TEXT("unresolved"); FiledAt = M->Story->Clock();
 const FS47Doc D = FollowupDocument();
 auto History = S47J::New(); S47J::Set(History,TEXT("attemptId"),AttemptId); S47J::Set(History,TEXT("resolution"),FiledResolution); S47J::Set(History,TEXT("filedAt"),FiledAt);
 S47J::Set(History,TEXT("baselineStamp"),BaselineStamp); S47J::Set(History,TEXT("referenceAt"),ReferenceAt); S47J::Set(History,TEXT("photoId"),FollowupPhoto ? FollowupPhoto->Photo.Id : FString());
 S47J::Set(History,TEXT("cardTitle"),D.Title); S47J::Set(History,TEXT("cardStamp"),D.Stamp); S47J::Set(History,TEXT("cardPage"),D.Page); FiledHistory.Add(History);
 // Only NEW physical filing reissues the canonical card. Prior dated paper metadata stays in history.
 M->Story->AddDoc(D);
 M->Story->Note(Verified ? TEXT("Caliche Junction follow-up and its original reference photograph filed at SARO. Under the recorded local controls, the marked lamp and service result disagreed. Cause unknown.")
  : TEXT("Caliche Junction reference-bound follow-up filed UNRESOLVED at SARO. Missing checks and photograph limitations are retained; this gives no confirmed-anomaly credit."));
 M->RequestAutosave();
 UE_LOG(LogS47Followup, Display, TEXT("S47: T02 physical return filing attempt=%s resolution=%s photo=%s clock=%.3f"), *AttemptId, *FiledResolution, FollowupPhoto ? *FollowupPhoto->Photo.Id : TEXT("none"), FiledAt);
 return true;
}

FS47Doc FS47Substation::FiledFollowupDocument(bool bIncludePhoto) const
{
 FS47Doc D; if (FiledHistory.IsEmpty()) return D;
 const auto& H = FiledHistory.Last();
 D.Id = TEXT("EV_TRAFO_FOLLOWUP"); D.Title = S47J::Str(H,TEXT("cardTitle")); D.Kind = TEXT("hand");
 D.Stamp = S47J::Str(H,TEXT("cardStamp")); D.Page = S47J::Str(H,TEXT("cardPage")); D.Transcript = D.Page;
 if (bIncludePhoto)
 {
  const FString PhotoId = S47J::Str(H,TEXT("photoId"));
  if (FollowupPhoto && !PhotoId.IsEmpty() && PhotoId == FollowupPhoto->Photo.Id && S47J::Str(H,TEXT("attemptId")) == AttemptId &&
   FollowupPhoto->Valid(AttemptId,FollowupReferencePosition(),FollowupReferenceRadius(),FollowupFeatures())) D.Image = FollowupPhoto->Photo.Url;
  else
  {
   // An ephemeral viewing notice; the retained page, stamp and saved paper remain byte-for-byte historical.
   D.Title += PhotoId.IsEmpty() ? TEXT(" / NO ORIGINAL PHOTOGRAPH RECORDED") : TEXT(" / ORIGINAL PHOTOGRAPH UNAVAILABLE");
   D.Transcript += TEXT("\n\nNo current valid original image matches this filed card's attempt and photo ID. A new attempt cannot replace its photograph.");
  }
 }
 return D;
}

FS47Doc FS47Substation::FollowupDocument(bool bIncludePhoto) const
{
 FS47Doc D;
 if (!ReferenceMatched())
 {
  if (FiledHistory.IsEmpty()) return D;
  const auto& H = FiledHistory.Last(); D.Id = TEXT("EV_TRAFO_FOLLOWUP"); D.Title = S47J::Str(H,TEXT("cardTitle"),TEXT("Caliche Junction / historical filed card")); D.Kind = TEXT("hand");
  D.Stamp = S47J::Str(H,TEXT("cardStamp")); D.Page = TEXT("HISTORICAL PAPER / CURRENT FIELD EVIDENCE UNAVAILABLE\n\nThis retained writing cannot restore the current reference, controls or photograph. No verified credit is derived from it.\n\n") + S47J::Str(H,TEXT("cardPage")); D.Transcript = D.Page; return D;
 }
 D.Id = TEXT("EV_TRAFO_FOLLOWUP"); D.Title = TEXT("Caliche Junction / controlled follow-up"); D.Kind = TEXT("hand");
 D.Stamp = bFollowupFiled ? FiledResolution == TEXT("verified") ? TEXT("SARO / VERIFIED RETURN FILED") : TEXT("SARO / UNRESOLVED RETURN FILED") : TEXT("FIELD RECORD / RETURN FILING PENDING");
 // Permanent documents contain no redundant image URL. Only this ephemeral view can show it.
 if (bIncludePhoto && FollowupVerified()) D.Image = FollowupPhoto->Photo.Url;
 D.Page = TEXT("CALICHE JUNCTION / T02\n\nHistorical normal comparison: ") + BaselineStamp +
  TEXT("\nNew comparison: ") + FieldStamp(AttemptAt) + TEXT("\nReference matched: ") + FieldStamp(ReferenceAt) +
  (bIsolated && bIsolationResponseSeen ? TEXT("\nEnclosed local isolation: OPEN / ") + FieldStamp(IsolationAt) : FString(TEXT("\nEnclosed local isolation: NOT COMPLETED / no observed independent OPEN response."))) +
  (bBackupExcluded && bAuxiliaryPulseSeen ? TEXT("\nIndependent auxiliary test: no local feed / ") + FieldStamp(BackupAt) : FString(TEXT("\nIndependent auxiliary: NOT COMPLETED / its pulse and dark return were not both observed."))) +
  (bIndicatorCompared ? TEXT("\nPanel indication compared with separate isolation response: ") + FieldStamp(IndicatorAt) : FString(TEXT("\nPanel / independent response comparison: NOT COMPLETED."))) +
  (bLampObserved ? TEXT("\nMarked lamp's visible bright/dim sequence observed: ") + FieldStamp(LampAt) : FString(TEXT("\nMarked lamp sequence: NOT COMPLETED from the matched standing position."))) +
  (FollowupPhoto ? TEXT("\nOriginal T02 photograph exposed: ") + FieldStamp(FollowupPhoto->Photo.Clock) : FString(TEXT("\nNo T02 photograph exposed."))) +
  (!FollowupVerified() ? FString(TEXT("\nPHOTO UNAVAILABLE OR INVALID / current verification is unresolved. A missing image is not replaced by the written card.")) : FString()) +
  (bFollowupFiled ? TEXT("\nReturned and filed: ") + FieldStamp(FiledAt) : FString(TEXT("\nReturn filing pending."))) +
  TEXT("\n\nThe culvert, two white bands on the separate metal reference post, the nearest power pole, its two outer conductors and fence corner identify the standing position. The third phase is also present; these are local reference features, not survey markers A/B/C.") +
  (bFollowupFiled && FiledResolution == TEXT("unresolved") ? FString(TEXT("\n\nFiled UNRESOLVED by choice. This card does not confirm an anomaly, including when available checks allow further comparison."))
   : FollowupVerified() ? FString(TEXT("\n\nUnder these controls, the marked light and the independent local service result disagree. Cause unknown.")) : FString(TEXT("\n\nUNRESOLVED. The current controls and photograph do not support a verified disagreement."))) +
  TEXT(" This is not a finding about the town supply, SARO K3, STATION 01 or the source of the residual.");
 D.Transcript = D.Page; return D;
}

S47J::FObj FS47Substation::FollowupSnapshot() const
{
 auto O = S47J::New(); using namespace S47J;
 Set(O, TEXT("v"), 1); Set(O, TEXT("attemptId"), AttemptId); Set(O, TEXT("attemptNumber"), AttemptNumber); Set(O, TEXT("attemptAt"), AttemptAt);
 Set(O, TEXT("baselineAt"), BaselineAt); Set(O, TEXT("baselineObservedAt"), BaselineObservedAt); Set(O, TEXT("baselineStamp"), BaselineStamp);
 Set(O, TEXT("referenceArmed"), bReferenceArmed); Set(O, TEXT("referenceMatched"), bReferenceMatched); Set(O, TEXT("referenceAt"), ReferenceAt);
 Set(O, TEXT("referenceView"), Clone(ReferenceView));
 Set(O, TEXT("isolation"), bIsolated); Set(O, TEXT("isolationAt"), IsolationAt); Set(O, TEXT("isolationControl"), TEXT("enclosed-isolation"));
 Set(O, TEXT("isolationResponseSeen"), bIsolationResponseSeen);
 Set(O, TEXT("backup"), bBackupExcluded); Set(O, TEXT("backupAt"), BackupAt); Set(O, TEXT("backupControl"), TEXT("independent-auxiliary"));
 Set(O, TEXT("auxiliaryPulseSeen"), bAuxiliaryPulseSeen);
 Set(O, TEXT("indicator"), bIndicatorCompared); Set(O, TEXT("indicatorAt"), IndicatorAt); Set(O, TEXT("indicatorPair"), TEXT("panel-open/independent-isolation-open/aux-dark"));
 Set(O, TEXT("lamp"), bLampObserved); Set(O, TEXT("lampAt"), LampAt); Set(O, TEXT("lampViewSeconds"), LampViewSeconds);
 Set(O, TEXT("lampMin"), LampViewMin == TNumericLimits<float>::Max() ? 0.0 : double(LampViewMin)); Set(O, TEXT("lampMax"), double(LampViewMax));
 Set(O, TEXT("filed"), bFollowupFiled); Set(O, TEXT("filedAt"), FiledAt); Set(O, TEXT("running"), FollowupRunning); Set(O, TEXT("elapsed"), FollowupElapsed); Set(O, TEXT("visualTime"), FollowupVisualTime);
 Set(O, TEXT("filedResolution"), FiledResolution);
 TArray<TSharedPtr<FJsonValue>> History; for (const auto& H : FiledHistory) History.Add(MakeShared<FJsonValueObject>(Clone(H))); O->SetArrayField(TEXT("filedHistory"),History);
 Set(O, TEXT("referencePosition"), Point(FollowupReferencePosition()));
 Set(O, TEXT("photo"), FollowupPhoto ? FollowupPhoto->ToJson() : FObj()); return O;
}

void FS47Substation::BeginFollowup(const S47J::FObj& O)
{
 AttemptId.Empty(); BaselineStamp.Empty(); FiledResolution.Empty(); FiledHistory.Reset(); AttemptNumber = FollowupRunning = 0;
 bReferenceArmed = bReferenceMatched = bIsolated = bBackupExcluded = bIndicatorCompared = bLampObserved = bFollowupFiled = false;
 bIsolationResponseSeen = bAuxiliaryPulseSeen = false; ReferenceView.Reset();
 AttemptAt = BaselineAt = BaselineObservedAt = ReferenceAt = IsolationAt = BackupAt = IndicatorAt = LampAt = FiledAt = FollowupElapsed = FollowupVisualTime = LampViewSeconds = 0;
 LampViewMin = TNumericLimits<float>::Max(); LampViewMax = 0; FollowupPhoto.Reset();
 if (S47J::Int(O, TEXT("v")) != 1) return;
 using namespace S47J;
 for (const auto& V : Arr(O,TEXT("filedHistory")))
 {
  if (!V.IsValid() || V->Type != EJson::Object) continue;
  const auto H = V->AsObject(); const FString Resolution = Str(H,TEXT("resolution"));
  if (H && !Str(H,TEXT("attemptId")).IsEmpty() && (Resolution == TEXT("verified") || Resolution == TEXT("unresolved")) && Has(H,TEXT("filedAt")) && Time(Num(H,TEXT("filedAt")))) FiledHistory.Add(Clone(H));
 }
 AttemptId = Str(O, TEXT("attemptId")); AttemptNumber = Int(O, TEXT("attemptNumber")); AttemptAt = Num(O, TEXT("attemptAt"));
 BaselineAt = Num(O, TEXT("baselineAt")); BaselineObservedAt = Num(O, TEXT("baselineObservedAt")); BaselineStamp = Str(O, TEXT("baselineStamp"));
 if (!Has(O,TEXT("attemptAt")) || !Has(O,TEXT("baselineAt")) || !Has(O,TEXT("baselineObservedAt")) || !FollowupRecordValid()) { AttemptId.Empty(); AttemptNumber = 0; return; }
 bReferenceArmed = Bool(O, TEXT("referenceArmed")); bReferenceMatched = Bool(O, TEXT("referenceMatched")); ReferenceAt = Num(O, TEXT("referenceAt")); ReferenceView = Clone(Obj(O,TEXT("referenceView")));
 if (!ReferenceMatched()) bReferenceMatched = false;
 bIsolationResponseSeen = Bool(O,TEXT("isolationResponseSeen")); bAuxiliaryPulseSeen = Bool(O,TEXT("auxiliaryPulseSeen"));
 IsolationAt = Num(O, TEXT("isolationAt")); bIsolated = ReferenceMatched() && Bool(O, TEXT("isolation")) && bIsolationResponseSeen && Str(O, TEXT("isolationControl")) == TEXT("enclosed-isolation") && Has(O,TEXT("isolationAt")) && Time(IsolationAt) && IsolationAt >= ReferenceAt;
 BackupAt = Num(O, TEXT("backupAt")); bBackupExcluded = bIsolated && Bool(O, TEXT("backup")) && bAuxiliaryPulseSeen && Str(O, TEXT("backupControl")) == TEXT("independent-auxiliary") && Has(O,TEXT("backupAt")) && Time(BackupAt) && BackupAt >= IsolationAt;
 IndicatorAt = Num(O, TEXT("indicatorAt")); bIndicatorCompared = bBackupExcluded && Bool(O, TEXT("indicator")) && Str(O, TEXT("indicatorPair")) == TEXT("panel-open/independent-isolation-open/aux-dark") && Has(O,TEXT("indicatorAt")) && Time(IndicatorAt) && IndicatorAt >= BackupAt;
 LampAt = Num(O, TEXT("lampAt")); LampViewSeconds = Num(O, TEXT("lampViewSeconds")); LampViewMin = float(Num(O, TEXT("lampMin"))); LampViewMax = float(Num(O, TEXT("lampMax")));
 bLampObserved = bIndicatorCompared && Bool(O, TEXT("lamp")) && Has(O,TEXT("lampAt")) && Has(O,TEXT("lampViewSeconds")) && Has(O,TEXT("lampMin")) && Has(O,TEXT("lampMax")); if (!FollowupControlsValid()) { bLampObserved = false; LampViewSeconds = 0; LampViewMin = TNumericLimits<float>::Max(); LampViewMax = 0; }
 FollowupPhoto = FS47T02PhotoRecord::FromJson(Obj(O, TEXT("photo")));
 // Keep an unavailable photo object for its honest saved-file status; never grant proof for it.
 FiledAt = Num(O, TEXT("filedAt")); FiledResolution = Str(O,TEXT("filedResolution"));
 const bool ResolutionValid = FiledResolution == TEXT("unresolved") || (FiledResolution == TEXT("verified") && FollowupVerified() && FiledAt >= FollowupPhoto->Photo.Clock);
 const bool HistoryMatches = FiledHistory.ContainsByPredicate([&](const S47J::FObj& H) { return Str(H,TEXT("attemptId")) == AttemptId && Str(H,TEXT("resolution")) == FiledResolution && Num(H,TEXT("filedAt")) == FiledAt; });
 bFollowupFiled = Bool(O, TEXT("filed")) && ReferenceMatched() && Has(O,TEXT("filedAt")) && Time(FiledAt) && FiledAt >= ReferenceAt && ResolutionValid && HistoryMatches;
 if (!bFollowupFiled) FiledResolution.Empty();
 FollowupVisualTime = Num(O, TEXT("visualTime")); if (!Time(FollowupVisualTime) || FollowupVisualTime < 0) FollowupVisualTime = 0;
 const int32 Run = Int(O, TEXT("running")); const double Elapsed = Num(O, TEXT("elapsed"));
 if (((Run == 1 && ReferenceMatched() && !bIsolated) || (Run == 2 && bIsolated && !bBackupExcluded)) && Time(Elapsed) && Elapsed > 0 && Elapsed <= ControlSeconds)
 { FollowupRunning = Run; FollowupElapsed = Elapsed; }
}

TArray<FS47Task> FS47Substation::FollowupTasks() const
{
 if (!FollowupAvailable() && AttemptId.IsEmpty()) return {};
 const bool Decision = M && M->Evening && (M->Evening->Pack.Flag(TEXT("trafo_anomaly_checked")) || M->Evening->Pack.Flag(TEXT("field_followup_unresolved")));
 return {{TEXT("Retain a physically compared normal local inspection baseline"), HasNormalResult()},
  {TEXT("Match Dale's sheet at the culvert mark: banded reference post, nearest power pole, outer conductors and fence corner"), ReferenceMatched()},
  {TEXT("Complete and observe the enclosed local isolation response"), bIsolated && bIsolationResponseSeen},
  {TEXT("Observe the independent auxiliary test pulse and its dark return"), bBackupExcluded && bAuxiliaryPulseSeen},
  {TEXT("Compare the panel indication with the separate isolation response"), bIndicatorCompared},
  {TEXT("Observe the marked lamp's full slow sequence from the reference position"), bLampObserved},
  {TEXT("Expose a separate T02 instant-back from the matched view (required for a verified result)"), FollowupVerified()},
  {TEXT("Review the physical follow-up; keep missing evidence unresolved"), Decision},
  {TEXT("File this follow-up once in SARO's control-room shift folder; STATION 01 and Nora remain available"), FollowupFiled()}};
}

FString FS47Substation::FollowupObjective() const
{
 if (!Here() || !FollowupAvailable()) return FString();
 if (FollowupFiled()) return FiledResolution == TEXT("verified") ? TEXT("T02 // CONTROLLED FOLLOW-UP FILED. CONTINUE THE STATION 01 CASE") : TEXT("T02 // UNRESOLVED CARD FILED. THE MAIN CASE CONTINUES; A NEW PHYSICAL COMPARISON REMAINS OPTIONAL");
 if (!HasNormalResult()) return TEXT("CALICHE JUNCTION // READ THE JOURNAL, RUN THE ORDINARY CHECK AND COMPARE THE MARKED LAMP FOR A DATED BASELINE");
 if (!ReferenceMatched()) return TEXT("T02 // STAND ON THE WHITE MARK BY THE CULVERT AND MATCH DALE'S SHEET TO THE BANDS, POLE, OUTER CONDUCTORS AND FENCE CORNER");
 if (!bIsolated) return TEXT("T02 // RUN THE ENCLOSED ISOLATION CONTROL AND WATCH ITS INDEPENDENT RESPONSE");
 if (!bBackupExcluded) return TEXT("T02 // TEST THE SEPARATE AUXILIARY FEED; WATCH ITS PULSE RETURN DARK");
 if (!bIndicatorCompared) return TEXT("T02 // COMPARE THE PANEL WINDOW WITH THE INDEPENDENT ISOLATION FLAG");
 if (!bLampObserved) return TEXT("T02 // RETURN TO THE REFERENCE MARK AND OBSERVE THE MARKED LAMP'S SLOW SEQUENCE");
 if (!FollowupVerified()) return TEXT("T02 // EXPOSE ITS OWN REFERENCE PHOTOGRAPH, OR KEEP THE COMPARISON UNRESOLVED");
 return TEXT("T02 // REVIEW THE LOCAL COMPARISON, THEN FILE ITS CARD AND ORIGINAL PHOTOGRAPH AT SARO");
}

bool FS47Substation::FollowupSelfTest(TArray<FString>& Lines)
{
 // Pure save-contract fixtures. These do not claim world traversal, observed pixels or exposure.
 bool Ok = true; auto Check = [&](bool P, const TCHAR* N) { Lines.Add(FString(P ? TEXT("ok  substation follow-up: ") : TEXT("FAIL substation follow-up: ")) + N); Ok &= P; };
 FS47Substation S(nullptr); S.Begin(); S.bJournal = S.bComplete = S.bObserved = true; S.CompletedAt = -19800; S.ObservedAt = -19795; S.CompletedStamp = FieldStamp(S.CompletedAt);
 S.AttemptId = TEXT("T02-save-contract"); S.AttemptNumber = 1; S.AttemptAt = 10800; S.BaselineAt = S.CompletedAt; S.BaselineObservedAt = S.ObservedAt; S.BaselineStamp = S.CompletedStamp;
 FS47Substation R(nullptr); R.Begin(S.Save());
 Check(R.FollowupRecordValid() && R.CompletedAt == S.CompletedAt && R.CompletedStamp == S.CompletedStamp, TEXT("a separate later attempt roundtrips without rewriting the signed normal baseline"));
 S.bReferenceArmed = S.bReferenceMatched = true; S.ReferenceAt = 10801;
 Check(!S.ReferenceMatched(), TEXT("a reference bool without actual saved pose/features is rejected"));
 S.ReferenceView = S47J::New(); S47J::Set(S.ReferenceView,TEXT("attemptId"),S.AttemptId); S47J::Set(S.ReferenceView,TEXT("clock"),S.ReferenceAt);
 S47J::Set(S.ReferenceView,TEXT("pose"),Point(FVector(7879,0,520.5))); S47J::Set(S.ReferenceView,TEXT("eye"),Point(FVector(7879,.27,520.5))); S47J::Set(S.ReferenceView,TEXT("floorY"),-1.35);
 const FRotator Camera(2,65,0); const FVector Eye(7879,.27,520.5); const double TanV = FMath::Tan(US47FieldCamera::PHOTO_FOV * PI / 360);
 S47J::Set(S.ReferenceView,TEXT("cameraRotation"),TArray<double>{Camera.Pitch,Camera.Yaw,Camera.Roll});
 auto Features = S47J::New();
 for (const auto& F : S.FollowupFeatures())
 {
  const FVector L = Camera.UnrotateVector(S47Three::ToUE(F.Value) - S47Three::ToUE(Eye)); auto P = S47J::New(); S47J::Set(P,TEXT("world"),Point(F.Value));
  S47J::Set(P,TEXT("uv"),TArray<double>{((L.Y/L.X)/(TanV*US47FieldCamera::PHOTO_W/US47FieldCamera::PHOTO_H)+1)/2,(1-(L.Z/L.X)/TanV)/2}); S47J::Set(Features,*F.Key,P);
 }
 S47J::Set(S.ReferenceView,TEXT("features"),Features);
 S.bIsolated = S.bIsolationResponseSeen = S.bBackupExcluded = S.bAuxiliaryPulseSeen = S.bIndicatorCompared = S.bLampObserved = true;
 S.IsolationAt = 10804; S.BackupAt = 10807; S.IndicatorAt = 10808; S.LampAt = 10812; S.LampViewSeconds = 3.5; S.LampViewMin = 1.2f; S.LampViewMax = 3.f;
 R.Begin(S.Save()); Check(R.FollowupControlsValid() && !R.FollowupVerified(), TEXT("serialized controls alone never give a verified result without their own real photograph"));
 auto Bad = S.Save(); auto T = S47J::Obj(Bad,TEXT("t02")); S47J::Set(T,TEXT("baselineAt"),-19799); R.Begin(Bad); Check(R.AttemptId.IsEmpty(), TEXT("a follow-up cannot borrow a different current-case normal baseline"));
 Bad = S.Save(); T = S47J::Obj(Bad,TEXT("t02")); S47J::Set(T,TEXT("auxiliaryPulseSeen"),false); R.Begin(Bad); Check(!R.bBackupExcluded && !R.FollowupControlsValid(), TEXT("unobserved independent auxiliary pulse invalidates all later result credit"));
 Bad = S.Save(); T = S47J::Obj(Bad,TEXT("t02")); S47J::Set(S47J::Obj(S47J::Obj(T,TEXT("referenceView")),TEXT("pose")),TEXT("x"),7890.0); R.Begin(Bad); Check(!R.ReferenceMatched() && S.ReferenceMatched(), TEXT("a reference outside standing radius is refused without aliasing the live source view"));
 Bad = S.Save(); T = S47J::Obj(Bad,TEXT("t02")); S47J::Set(T,TEXT("indicatorAt"),10802.0); R.Begin(Bad); Check(!R.bIndicatorCompared && !R.FollowupControlsValid(), TEXT("a panel comparison dated before its independent tests is refused"));
 Bad = S.Save(); T = S47J::Obj(Bad,TEXT("t02")); auto View = S47J::Obj(T,TEXT("referenceView")); auto PointUV = S47J::Obj(S47J::Obj(View,TEXT("features")),TEXT("culvert"));
 S47J::Set(PointUV,TEXT("uv"),TArray<double>{.5,.5}); R.Begin(Bad); Check(!R.ReferenceMatched() && S.ReferenceMatched(), TEXT("saved framing UV reprojects correctly and an edited snapshot cannot mutate its source view"));
 S.bFollowupFiled = true; S.FiledResolution = TEXT("unresolved"); S.FiledAt = 10820;
 auto H = S47J::New(); S47J::Set(H,TEXT("attemptId"),S.AttemptId); S47J::Set(H,TEXT("resolution"),S.FiledResolution); S47J::Set(H,TEXT("filedAt"),S.FiledAt); S47J::Set(H,TEXT("cardPage"),TEXT("original unresolved paper")); S.FiledHistory.Add(H);
 R.Begin(S.Save()); Check(R.FollowupFiled() && R.FollowupFiledResolution() == TEXT("unresolved") && !R.FollowupVerified(), TEXT("a dated unresolved return remains unresolved without a photograph"));
 Check(R.FollowupDocument().Image.IsEmpty() && R.FollowupDocument().Page.Contains(TEXT("UNRESOLVED")), TEXT("permanent follow-up paper carries no redundant photo URL or false verified claim"));
 Bad = S.Save(); T = S47J::Obj(Bad,TEXT("t02")); S47J::Set(T,TEXT("filedResolution"),TEXT("verified")); R.Begin(Bad); Check(!R.FollowupFiled(), TEXT("changing an unresolved filing label cannot promote it to verified"));
 R.Begin(S.Save()); const FString Before = S47J::Write(R.FiledHistory[0]); R.Save(); Check(S47J::Write(R.FiledHistory[0]) == Before, TEXT("serialization does not rewrite a previously dated filed card"));
 R.Begin(); Check(R.AttemptId.IsEmpty() && R.FiledHistory.IsEmpty() && !R.FollowupFiled(), TEXT("a new case discards every previous attempt and filing-credit record"));
 return Ok;
}
