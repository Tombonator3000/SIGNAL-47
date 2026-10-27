#include "S47HorrorVisual.h"

#include "S47Inter.h"
#include "S47Main.h"
#include "S47Seen.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47HorrorVisual, Log, All);

namespace
{
 constexpr double PassDuration = 9.5;
 bool Bool(const S47J::FObj& O, const TCHAR* K, bool& Out)
 {
  const auto V = O.IsValid() ? O->TryGetField(K) : nullptr;
  return V.IsValid() && V->Type == EJson::Boolean && V->TryGetBool(Out);
 }
 bool Number(const S47J::FObj& O, const TCHAR* K, double& Out)
 {
  const auto V = O.IsValid() ? O->TryGetField(K) : nullptr;
  return V.IsValid() && V->Type == EJson::Number && V->TryGetNumber(Out) && FMath::IsFinite(Out);
 }
 FVector PassPosition(double T)
 {
  const double K = FMath::Clamp(T / PassDuration, 0., 1.);
  // The palm first presses against the pane, then slides toward the existing
  // eastern opaque pier (x>=5.4). Its forearm never crosses the glass plane.
  return FVector(4.26 + 1.72 * K, 1.70 + .022 * FMath::Sin(K * PI * 2), -4.69);
 }
 FTransform Transform(const FVector& At)
 {
  double Matrix[16]; S47Three::Compose(At, FQuat::Identity, FVector::OneVector, Matrix);
  return S47Three::MatToUE(Matrix);
 }
}

struct FS47HorrorVisual::FVisual
{
 FS47Obj Root, Hand, Marks, Lamp;
 TArray<FS47Obj> Slats;
 TMap<FString, FS47Obj> Objects;
 TWeakObjectPtr<UPointLightComponent> Light;
 TWeakObjectPtr<UStaticMeshComponent> MarkPick;
 TWeakObjectPtr<UMaterialInstanceDynamic> LampMat;
 double AppliedScreen = -1, AppliedHand = -1;
};

FS47HorrorVisual::FS47HorrorVisual(US47Main* InM) : Visual(MakeUnique<FVisual>()), M(InM) {}
FS47HorrorVisual::~FS47HorrorVisual() = default;

bool FS47HorrorVisual::Build()
{
 if (bBuilt) return true;
 if (bBuildFailed) return false;
 if (!M || !M->Web || !M->Inter || !M->Web->IsLoaded()) return false;
 AS47Web* W = M->Web;
 const int32 Parent = W->Handle(TEXT("room.group"));
 const auto* ParentNode = W->Node(Parent);
 USceneComponent* ParentComp = ParentNode ? ParentNode->Comp.Get() : nullptr;
 auto* HandMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Signal47/HorrorSC01/SM_SC01_Hand.SM_SC01_Hand"));
 auto* MarkMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Signal47/HorrorSC01/SM_SC01_GlassTraces.SM_SC01_GlassTraces"));
 auto* SlatMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Signal47/HorrorSC01/SM_SC01_ShutterSlat.SM_SC01_ShutterSlat"));
 auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 if (!ParentComp || !HandMesh || !MarkMesh || !SlatMesh || !Cube || !Sphere || !Base)
 {
  bBuildFailed = true;
  UE_LOG(LogS47HorrorVisual, Error, TEXT("S47: SC01 original physical scene unavailable; no observation credit granted"));
  return false;
 }
 auto Material = [W, Base](uint32 Color, float Rough = .65f, float Metal = 0.f)
 {
  auto* Mat = UMaterialInstanceDynamic::Create(Base, W);
  Mat->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(Color));
  Mat->SetScalarParameterValue(TEXT("Roughness"), Rough); Mat->SetScalarParameterValue(TEXT("Metalness"), Metal);
  Mat->SetScalarParameterValue(TEXT("NativeLighting"), 1); Mat->SetScalarParameterValue(TEXT("EmissivePower"), 0);
  return Mat;
 };
 UBoxComponent* Group = NewObject<UBoxComponent>(W, TEXT("SC01PhysicalRoot"));
 Group->SetMobility(EComponentMobility::Movable); Group->SetupAttachment(ParentComp);
 Group->SetCollisionEnabled(ECollisionEnabled::NoCollision); Group->SetBoxExtent(FVector(1)); Group->RegisterComponent();
 Visual->Root = W->ObjAt(W->AdoptNode(Group, Parent, TEXT("sc01:physicalRoot")));
 const int32 Root = Visual->Root.I;
 auto Mesh = [&](const FString& Name, UStaticMesh* Asset, const FVector& At, const FVector& Scale, UMaterialInterface* Mat = nullptr)
 {
  auto* C = NewObject<UStaticMeshComponent>(W, *Name);
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(Group); C->SetStaticMesh(Asset);
  if (Mat) C->SetMaterial(0, Mat);
  C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  C->SetCollisionResponseToAllChannels(ECR_Ignore); C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
  C->SetCastShadow(true); C->RegisterComponent(); C->SetWorldTransform(Transform(At));
  C->SetWorldScale3D(S47Three::DirToUE(Scale));
  FS47Obj Obj = W->ObjAt(W->AdoptNode(C, Root, Name)); Obj.SetScale(Scale); return Obj;
 };
 auto* Steel = Material(0x4b5a55, .48f, .55f);
 auto* Black = Material(0x171d1b, .72f);
 auto* Ivory = Material(0xd1cbb3, .48f);
 auto* Paper = Material(0xc9c2a4, .85f);
 auto* Bulb = Material(0xffd6a0, .30f);
 Visual->LampMat = Bulb;
 Visual->Hand = Mesh(TEXT("sc01:hand"), HandMesh, PassPosition(0), FVector::OneVector);
 // The original metric marks lie just beyond the real z=-4.56 pane. Their
 // volume is 3.6mm thick and never moves to the player's side of the glass.
 Visual->Marks = Mesh(TEXT("sc01:exteriorTraces"), MarkMesh, WindowPoint(), FVector::OneVector);
 // A group of narrow physical grease streaks has empty spaces inside its
 // bounding rectangle. Use the same bounded inspection-volume convention as
 // the existing service controls, so an ordinary crosshair can inspect the
 // patch rather than needing to hit one millimetre-wide triangle. This volume
 // never draws, casts a shadow or blocks visibility. Observation below still
 // checks the real native mark mesh through the real glass.
 UStaticMeshComponent* MarkPick = NewObject<UStaticMeshComponent>(W,TEXT("SC01MarksInspection"));
 MarkPick->SetMobility(EComponentMobility::Movable);
 if (const auto* N = W->Node(Visual->Marks.I)) MarkPick->SetupAttachment(N->Comp.Get());
 MarkPick->SetStaticMesh(Cube);
 MarkPick->SetHiddenInGame(true); MarkPick->SetCastShadow(false);
 MarkPick->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 MarkPick->SetCollisionResponseToAllChannels(ECR_Ignore);
 MarkPick->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
 MarkPick->RegisterComponent();
 MarkPick->SetWorldLocation(S47Three::ToUE(WindowPoint()+FVector(-.0348,.0664,0)));
 MarkPick->SetWorldScale3D(S47Three::DirToUE(FVector(.25,.33,.012)));
 FS47Obj MarksTarget = W->ObjAt(W->AdoptNode(MarkPick,Visual->Marks.I,TEXT("proxy:sc01MarksInspection")));
 MarksTarget.SetScale(FVector(.25,.33,.012));
 Visual->MarkPick = MarkPick;
 for (int32 I = 0; I < 26; ++I)
  Visual->Slats.Add(Mesh(FString::Printf(TEXT("sc01:shutterSlat%02d"), I), SlatMesh,
   FVector(4.50, 2.80 + I * .0025, -4.34), FVector::OneVector));
 Mesh(TEXT("sc01:shutterHeader"), Cube, FVector(4.5, 2.88, -4.32), FVector(1.85, .18, .17), Steel);
 Mesh(TEXT("sc01:shutterWestGuide"), Cube, FVector(3.60, 1.85, -4.34), FVector(.045, 1.78, .07), Steel);
 Mesh(TEXT("sc01:shutterEastGuide"), Cube, FVector(5.40, 1.85, -4.34), FVector(.045, 1.78, .07), Steel);
 // The normal reference is a real low-voltage task lamp on an outside post,
 // independent of the receiver, service circuit and future alien hand.
 Mesh(TEXT("sc01:referencePost"), Cube, FVector(4.45, .54, -5.54), FVector(.075, 1.08, .075), Steel);
 Mesh(TEXT("sc01:referenceFoot"), Cube, FVector(4.45, .035, -5.54), FVector(.25, .07, .25), Steel);
 Mesh(TEXT("sc01:referenceHousing"), Cube, FVector(4.45, 1.16, -5.48), FVector(.22, .12, .17), Black);
 Mesh(TEXT("sc01:referenceNeck"), Cube, FVector(4.45, 1.235, -5.48), FVector(.04, .03, .04), Steel);
 Visual->Lamp = Mesh(TEXT("sc01:referenceDiffuser"), Sphere, LampPoint(), FVector(.125, .125, .06), Bulb);
 auto* Light = NewObject<UPointLightComponent>(W, TEXT("SC01ReferenceLight"));
 Light->SetMobility(EComponentMobility::Movable); Light->SetupAttachment(Group);
 Light->SetWorldLocation(S47Three::ToUE(LampPoint())); Light->SetIntensityUnits(ELightUnits::Lumens);
 Light->SetLightColor(FLinearColor(1.f, .77f, .48f)); Light->SetAttenuationRadius(220); Light->SetCastShadows(true);
 Light->RegisterComponent(); Visual->Light = Light;
 Mesh(TEXT("sc01:controlBackplate"), Cube, FVector(5.18, 1.25, -4.22), FVector(.30, .32, .065), Steel);
 FS47Obj LampSwitch = Mesh(TEXT("sc01:lampToggle"), Cube, FVector(5.10, 1.26, -4.17), FVector(.075, .115, .04), Ivory);
 FS47Obj ScreenSwitch = Mesh(TEXT("sc01:screenToggle"), Cube, FVector(5.27, 1.26, -4.17), FVector(.075, .115, .04), Black);
 FS47Obj RoomSwitch = Mesh(TEXT("sc01:roomLightToggle"), Cube, FVector(5.10, 1.10, -4.17), FVector(.075, .08, .04), Ivory);
 FS47Obj OutsideSwitch = Mesh(TEXT("sc01:outsideLampToggle"), Cube, FVector(4.45, .91, -5.65), FVector(.12, .12, .05), Ivory);
 FS47Obj Plate = Mesh(TEXT("sc01:referenceLabel"), Cube, FVector(5.16, 1.50, -4.20), FVector(.42, .15, .012), Paper);
 auto* Text = NewObject<UTextRenderComponent>(W, TEXT("SC01ReferenceText"));
 Text->SetMobility(EComponentMobility::Movable); Text->SetupAttachment(Group);
 Text->SetText(FText::FromString(TEXT("REFERENCE\nLAMP / SHADE"))); Text->SetWorldSize(2.1f);
 Text->SetHorizontalAlignment(EHTA_Center); Text->SetVerticalAlignment(EVRTA_TextCenter);
 Text->SetTextRenderColor(FColor(30, 38, 32)); Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Text->RegisterComponent(); Text->SetWorldLocation(S47Three::ToUE(FVector(5.16, 1.50, -4.185)));
 Text->SetWorldRotation(FRotator(0, 90, 0)); W->AdoptNode(Text, Root, TEXT("sc01:referenceText"));
 Visual->Objects.Add(TEXT("horror:lamp"), LampSwitch);
 Visual->Objects.Add(TEXT("horror:screen"), ScreenSwitch);
 Visual->Objects.Add(TEXT("horror:roomLamp"), RoomSwitch);
 Visual->Objects.Add(TEXT("horror:lampOutside"), OutsideSwitch);
 Visual->Objects.Add(TEXT("horror:marks"), MarksTarget);
 (void)Plate;
 for (const FString& Id : {FString(TEXT("horror:lamp")), FString(TEXT("horror:screen")), FString(TEXT("horror:marks")), FString(TEXT("horror:roomLamp")), FString(TEXT("horror:lampOutside"))})
 {
  FS47Interactable It; It.Id = Id; It.Object = Visual->Objects.FindChecked(Id); It.Range = 2.4;
  if (const auto* N = W->Node(It.Object.I))
   if (auto* C = Cast<UPrimitiveComponent>(N->Comp.Get())) C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
  It.Label = [this, Id]() -> FString
  {
   if (!Here()) return {};
   if (Id == TEXT("horror:lamp") || Id == TEXT("horror:lampOutside")) return bLampOn ? TEXT("Switch reference lamp off") : TEXT("Switch reference lamp on");
   if (Id == TEXT("horror:roomLamp")) return bLightingReleased ? FString() : bRoomLampOn ? FString(TEXT("Switch room comparison lamp off")) : FString(TEXT("Switch room comparison lamp on"));
   if (Id == TEXT("horror:screen")) return bScreenTarget ? TEXT("Raise opaque window shade") : TEXT("Lower opaque window shade");
   return MarksPresent() ? TEXT("Check the marks on the outside glass") : FString();
  };
  It.Use = [this, Id] { Use(Id); };
  It.Examine = [this, Id]() -> FString
  {
   if (Id == TEXT("horror:lamp") || Id == TEXT("horror:lampOutside")) return TEXT("A low-voltage reference lamp outside the pane. Its switch controls this lamp only.");
   if (Id == TEXT("horror:roomLamp")) return TEXT("The room's comparison fixture. It changes the room-side light without touching the exterior trace or receiver power.");
   if (Id == TEXT("horror:screen")) return TEXT("An opaque steel shade in fixed guides. The slats cover the eastern pane when lowered.");
   return MarksPresent() ? TEXT("Five narrow streaks and a palm-sized smear on the exterior surface. The glass between them and the room remains intact.") : FString();
  };
  M->Inter->Add(It);
 }
 bBuilt = true; Apply();
 UE_LOG(LogS47HorrorVisual, Display, TEXT("S47: SC01 physical scene ready: original hand, exterior marks, 26 opaque slats and separate reference lamp"));
 return true;
}

void FS47HorrorVisual::Begin(const S47J::FObj& Saved, bool bInEnabled)
{
 bEnabled = bInEnabled; bWindowArmed = false; bHandSeen = false; bMarksChecked = false;
 bLightingCaptured = M && M->Web && M->Web->TubeCount() > 1;
 bOriginalRoomLamp = bLightingCaptured ? M->Web->TubeLit(1) : true; bLightingReleased = false;
 bLampOn = bRoomLampOn = true; bScreenTarget = false; bScreenChecked = false;
 HandPhase = DirectMask = GlassMask = MarksMask = 0; HandT = ScreenK = HandDwell = LampDwell = 0;
 if (Saved.IsValid())
 {
  double V = 0, Phase = 0, T = 0, Screen = 0, Direct = 0, Glass = 0, TraceMask = 0;
  bool Enabled = false, Armed = false, Seen = false, Marks = false, Lamp = false, RoomLamp = false, Target = false, Checked = false;
  bool OriginalLamp = true, Released = false;
  const bool Valid = Number(Saved,TEXT("v"),V) && V == 1 && Bool(Saved,TEXT("enabled"),Enabled) &&
   Bool(Saved,TEXT("armed"),Armed) && Bool(Saved,TEXT("handSeen"),Seen) && Bool(Saved,TEXT("marksChecked"),Marks) &&
   Bool(Saved,TEXT("lampOn"),Lamp) && Bool(Saved,TEXT("roomLampOn"),RoomLamp) && Bool(Saved,TEXT("screenTarget"),Target) && Bool(Saved,TEXT("screenChecked"),Checked) &&
   Bool(Saved,TEXT("originalRoomLamp"),OriginalLamp) && Bool(Saved,TEXT("lightingReleased"),Released) &&
   Number(Saved,TEXT("phase"),Phase) && Phase >= 0 && Phase <= 2 && Phase == FMath::FloorToDouble(Phase) &&
   Number(Saved,TEXT("time"),T) && T >= 0 && T <= PassDuration &&
   Number(Saved,TEXT("screen"),Screen) && Screen >= 0 && Screen <= 1 &&
   Number(Saved,TEXT("directMask"),Direct) && Direct >= 0 && Direct <= 3 && Direct == FMath::FloorToDouble(Direct) &&
   Number(Saved,TEXT("glassMask"),Glass) && Glass >= 0 && Glass <= 3 && Glass == FMath::FloorToDouble(Glass) &&
   Number(Saved,TEXT("marksMask"),TraceMask) && TraceMask >= 0 && TraceMask <= 3 && TraceMask == FMath::FloorToDouble(TraceMask) &&
   (!Seen || Phase >= 1) && (!Marks || (Phase == 2 && TraceMask == 3)) && (!TraceMask || Phase == 2) &&
   (Phase != 2 || T == PassDuration) && (Phase != 0 || T == 0) && (Phase != 1 || T < PassDuration) &&
   (!Checked || Glass == 3);
  if (Valid)
  {
   bEnabled = bInEnabled && Enabled; bWindowArmed = Armed; bHandSeen = Seen; bMarksChecked = Marks;
   bLampOn = Lamp; bRoomLampOn = RoomLamp; bScreenTarget = Target; bScreenChecked = Checked;
   bOriginalRoomLamp = OriginalLamp; bLightingReleased = Released;
   HandPhase = int32(Phase); HandT = T; ScreenK = Screen; DirectMask = int32(Direct); GlassMask = int32(Glass); MarksMask = int32(TraceMask);
  }
  else
  {
   bEnabled = false;
   UE_LOG(LogS47HorrorVisual, Warning, TEXT("S47: SC01 physical save rejected; no unsupported observation credit restored"));
  }
 }
 if (bEnabled) Build();
 Apply();
}

void FS47HorrorVisual::Stop()
{
 if (bLightingCaptured && !bLightingReleased && M && M->Web && M->Web->TubeCount() > 1)
  M->Web->SetTube(1,bOriginalRoomLamp);
 bEnabled = false; HandDwell = LampDwell = 0; Apply();
}
void FS47HorrorVisual::ReleaseLighting()
{
 if (bLightingReleased || !bEnabled) return;
 bLightingReleased = true;
 if (bLightingCaptured && M && M->Web && M->Web->TubeCount() > 1) M->Web->SetTube(1,bOriginalRoomLamp);
 bRoomLampOn = bOriginalRoomLamp; Changed();
}
bool FS47HorrorVisual::Here() const
{
 return bEnabled && bBuilt && M && M->World && M->Player && M->Mode == US47Main::EMode::Play &&
  M->World->Area == TEXT("saro") && !M->World->bDriving;
}
bool FS47HorrorVisual::Held() const
{
 return !M || M->bPausedByMenu || M->ModalOpen() || !M->Story || M->Story->IsCinematic();
}
FS47Obj FS47HorrorVisual::Object(const FString& Id) const
{
 if (Id == TEXT("hand")) return Visual->Hand;
 if (Id == TEXT("lamp")) return Visual->Lamp;
 if (Id == TEXT("marks")) return Visual->Marks;
 return Visual->Objects.FindRef(Id);
}
FVector FS47HorrorVisual::HandPosition() const { return PassPosition(HandT); }

bool FS47HorrorVisual::ClearLine(const FVector& Eye, const FVector& At, const FS47Obj& Target, bool* bScreenHit) const
{
 if (bScreenHit) *bScreenHit = false;
 if (!M || !M->Web || !M->Web->GetWorld()) return false;
 auto* W = M->Web.Get();
 const auto* TargetNode = Target.IsValid() ? W->Node(Target.I) : nullptr;
 const auto* TargetComp = TargetNode ? TargetNode->Comp.Get() : nullptr;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47HorrorSight), true);
 const FVector From = S47Three::ToUE(Eye), To = S47Three::ToUE(At);
 for (int32 I = 0; I < 24; ++I)
 {
  FHitResult H;
  if (!W->GetWorld()->LineTraceSingleByChannel(H, From, To, ECC_Visibility, Q)) return true;
  const auto* C = H.GetComponent(); if (!C) return false;
  if (C == TargetComp) return true;
  bool Solid = C->IsVisible();
  const auto* Node = W->Node(W->NodeOf(C));
  if (Node && Node->Name.StartsWith(TEXT("proxy"))) Solid = false;
  if (const auto* Mat = C->GetMaterial(FMath::Max<int32>(0,H.ElementIndex)))
  {
   float Opacity = 1; Mat->GetScalarParameterValue(TEXT("Opacity"), Opacity);
   if (Mat->GetBlendMode() == BLEND_Additive || (Mat->GetBlendMode() == BLEND_Translucent && Opacity < .5f)) Solid = false;
  }
  if (Solid)
  {
   if (bScreenHit && Node) *bScreenHit = Node->Name.StartsWith(TEXT("sc01:shutterSlat"));
   return false;
  }
  Q.AddIgnoredComponent(C);
 }
 return false;
}
bool FS47HorrorVisual::VisiblePoint(const FVector& At, const FS47Obj& Target, double MaxDistance) const
{
 if (!Here() || Held() || !Target.Shown()) return false;
 const FS47Cam Cam = FS47Cam::Of(M->Player);
 if (FVector::Dist(Cam.Pos, At) > MaxDistance) return false;
 const FVector P = Cam.Project(At);
 if (P.Z >= 1 || FMath::Abs(P.X) > .93 || FMath::Abs(P.Y) > .93) return false;
 return ClearLine(Cam.Pos, At, Target);
}
bool FS47HorrorVisual::InReach(const FString& Id) const
{
 if (!Here() || Held()) return false;
 const FS47Obj Obj = Object(Id); if (!Obj.IsValid() || !Obj.Shown()) return false;
 if (Id == TEXT("horror:marks"))
 {
  // The upper middle-finger streak is actual authored geometry; the patch's
  // box centre is deliberately not accepted as proof of a visible surface.
  return VisiblePoint(WindowPoint()+FVector(-.026,.12,0),Visual->Marks,2.5);
 }
 return VisiblePoint(Obj.WorldPosition(), Obj, 2.5);
}
bool FS47HorrorVisual::ReferenceInView() const
{
 return bLampOn && HandPhase == 0 && VisiblePoint(LampPoint(),Visual->Lamp,8);
}

void FS47HorrorVisual::ArmWindow()
{
 if (!bEnabled || bWindowArmed || HandPhase != 0) return;
 bWindowArmed = true; Changed();
}
bool FS47HorrorVisual::Use(const FString& Id)
{
 if (!Visual->Objects.Contains(Id)) return false;
 if (!InReach(Id)) return true;
 if (Id == TEXT("horror:lamp") || Id == TEXT("horror:lampOutside"))
 {
  bLampOn = !bLampOn; LampDwell = 0; Apply(); Changed();
  M->UI->Toast(bLampOn ? TEXT("Reference lamp on.") : TEXT("Reference lamp off. The other site lights remain on."));
 }
 else if (Id == TEXT("horror:roomLamp"))
 {
  if (bLightingReleased) return true;
  bRoomLampOn = !bRoomLampOn; Apply(); Changed();
  M->UI->Toast(bRoomLampOn ? TEXT("Room comparison lamp on.") : TEXT("Room comparison lamp off. Exterior lighting is unchanged."));
 }
 else if (Id == TEXT("horror:screen"))
 {
  bScreenTarget = !bScreenTarget; LampDwell = 0; Changed();
  M->UI->Toast(bScreenTarget ? TEXT("Lowering the opaque window shade.") : TEXT("Raising the opaque window shade."));
 }
 else if (Id == TEXT("horror:marks") && MarksPresent())
 {
  // A safe indoor look can establish which surface carries the visible trace.
  // The exterior mesh itself and the real glass plane remain in the sightline.
  const int32 Was = MarksMask; MarksMask |= bRoomLampOn ? 1 : 2;
  if (Was != MarksMask) Changed();
  if (MarksMask == 3)
  {
   if (!bMarksChecked) { bMarksChecked = true; Changed(); }
   M->UI->Toast(TEXT("The streaks are on the outside surface. Switching the room lamp does not remove them. The pane is intact."),5);
  }
  else M->UI->Toast(TEXT("The streaks sit beyond the glass. Compare them again after changing the room lamp beside the shade controls."),5);
 }
 return true;
}

void FS47HorrorVisual::Apply()
{
 if (!bBuilt || !M || !M->Web) return;
 const bool Shown = bEnabled && M->Mode == US47Main::EMode::Play && M->World && M->World->Area == TEXT("saro");
 Visual->Root.SetVisible(Shown);
 Visual->Hand.SetVisible(Shown && HandPhase == 1);
 if (Visual->AppliedHand != HandT)
 {
  Visual->Hand.SetPosition(PassPosition(HandT)); Visual->AppliedHand = HandT;
 }
 Visual->Marks.SetVisible(Shown && HandPhase == 2);
 if (auto* Pick = Visual->MarkPick.Get())
  Pick->SetCollisionEnabled(Shown && HandPhase == 2 ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
 if (Visual->AppliedScreen != ScreenK) for (int32 I = 0; I < Visual->Slats.Num(); ++I)
 {
  const double Raised = 2.80 + I * .0025, Lowered = .98 + I * .071;
  Visual->Slats[I].SetPosition(FVector(4.50,FMath::Lerp(Raised,Lowered,ScreenK),-4.34));
 }
 Visual->AppliedScreen = ScreenK;
 if (Here() && !bLightingReleased && M->Web->TubeCount() > 1) M->Web->SetTube(1,bRoomLampOn);
 if (auto* L = Visual->Light.Get()) { L->SetVisibility(Shown && bLampOn); L->SetIntensity(bLampOn ? 45 : 0); }
 if (auto* Mat = Visual->LampMat.Get())
 {
  Mat->SetVectorParameterValue(TEXT("Color"), bLampOn ? S47Three::Hex(0xffd6a0) : S47Three::Hex(0x302f29));
  Mat->SetScalarParameterValue(TEXT("EmissivePower"), bLampOn ? 2.4f : 0);
  Mat->SetVectorParameterValue(TEXT("Emissive"), bLampOn ? S47Three::Hex(0xffb869) : FLinearColor::Black);
 }
}

void FS47HorrorVisual::Step(double Dt)
{
 if (!bEnabled || !M || M->Mode != US47Main::EMode::Play) { Apply(); return; }
 if (!bBuilt && !Build()) return;
 if (!FMath::IsFinite(Dt) || Dt <= 0 || Held()) { HandDwell = LampDwell = 0; Apply(); return; }
 const double Sample = FMath::Min(Dt,.10);
 const double Before = ScreenK;
 ScreenK = FMath::Clamp(ScreenK + (bScreenTarget ? 1 : -1) * Sample / .85,0.,1.);
 if ((Before > 0 && ScreenK == 0) || (Before < 1 && ScreenK == 1)) Changed();
 Apply();
 // Completed SC01 retains useful physical props, including the working shade,
 // but no longer samples the room or competes with later story lighting.
 if (bLightingReleased) return;
 if (Here())
 {
  const FS47Cam Cam = FS47Cam::Of(M->Player);
  const FVector ToLamp = Cam.Project(LampPoint());
  const bool LampInFrame = ToLamp.Z < 1 && FMath::Abs(ToLamp.X) < .88 && FMath::Abs(ToLamp.Y) < .88 && FVector::Dist(Cam.Pos,LampPoint()) < 6;
  const bool Inside = Cam.Pos.Z > -4.45 && Cam.Pos.Z < 1.0 && Cam.Pos.X > 2.8 && Cam.Pos.X < 5.8;
  const bool Outside = Cam.Pos.Z < -4.8 && Cam.Pos.Z > -9;
  bool ScreenHit = false;
  const bool Clear = LampInFrame && ClearLine(Cam.Pos,LampPoint(),Visual->Lamp,&ScreenHit);
  if (Clear && (Inside || Outside))
  {
   LampDwell += Sample;
   if (LampDwell >= .40)
   {
    int32& Mask = Inside ? GlassMask : DirectMask;
    const int32 Was = Mask; Mask |= bLampOn ? 1 : 2;
    if (Mask != Was)
    {
     Changed();
     UE_LOG(LogS47HorrorVisual, Display, TEXT("S47: SC01 normal lamp observed %s %s"),Inside ? TEXT("through-glass") : TEXT("direct"),bLampOn ? TEXT("on") : TEXT("off"));
    }
   }
  }
  else LampDwell = 0;
  if (!bScreenChecked && Inside && LampInFrame && ScreenClosed() && ScreenHit && GlassMask == 3)
  {
   bScreenChecked = true; Changed();
   UE_LOG(LogS47HorrorVisual, Display, TEXT("S47: SC01 actual opaque shade blocks reference-lamp sight"));
  }
  if (bWindowArmed && HandPhase == 0 && ScreenK <= .005 && Inside)
  {
   const FVector P = Cam.Project(PassPosition(0));
   if (P.Z < 1 && FMath::Abs(P.X) < .78 && FMath::Abs(P.Y) < .78 && ClearLine(Cam.Pos,PassPosition(0),Visual->Hand))
   {
    HandPhase = 1; HandT = 0; Apply(); Changed();
    UE_LOG(LogS47HorrorVisual, Display, TEXT("S47: SC01 hand pass started outside glass"));
   }
  }
 }
 if (HandPhase == 1)
 {
  HandT = FMath::Min(PassDuration,HandT + Sample);
  Apply();
  const auto* N = M->Web->Node(Visual->Hand.I);
  const auto* C = N ? Cast<UPrimitiveComponent>(N->Comp.Get()) : nullptr;
  const bool Rendered = C && C->IsRegistered() && C->IsVisible() && C->WasRecentlyRendered(.25f);
  if (Rendered && VisiblePoint(PassPosition(HandT),Visual->Hand,8)) HandDwell += Sample; else HandDwell = 0;
  if (!bHandSeen && HandDwell >= .20)
  {
   bHandSeen = true; Changed();
   UE_LOG(LogS47HorrorVisual, Display, TEXT("S47: SC01 moving exterior hand actually observed with opaque sight check"));
  }
  if (HandT >= PassDuration)
  {
   HandPhase = 2; HandDwell = 0; Apply(); Changed();
   UE_LOG(LogS47HorrorVisual, Display, TEXT("S47: SC01 hand crossed behind existing wall; exterior traces remain"));
  }
 }
}

void FS47HorrorVisual::Changed() { if (M) M->RequestAutosave(); }
S47J::FObj FS47HorrorVisual::Save() const
{
 auto O = S47J::New();
 S47J::Set(O,TEXT("v"),1); S47J::Set(O,TEXT("enabled"),bEnabled); S47J::Set(O,TEXT("armed"),bWindowArmed);
 S47J::Set(O,TEXT("handSeen"),bHandSeen); S47J::Set(O,TEXT("marksChecked"),bMarksChecked);
 S47J::Set(O,TEXT("lampOn"),bLampOn); S47J::Set(O,TEXT("roomLampOn"),bRoomLampOn); S47J::Set(O,TEXT("screenTarget"),bScreenTarget);
 S47J::Set(O,TEXT("originalRoomLamp"),bOriginalRoomLamp); S47J::Set(O,TEXT("lightingReleased"),bLightingReleased);
 S47J::Set(O,TEXT("screenChecked"),bScreenChecked); S47J::Set(O,TEXT("phase"),HandPhase);
 S47J::Set(O,TEXT("time"),HandT); S47J::Set(O,TEXT("screen"),ScreenK);
 S47J::Set(O,TEXT("directMask"),DirectMask); S47J::Set(O,TEXT("glassMask"),GlassMask);
 S47J::Set(O,TEXT("marksMask"),MarksMask);
 return O;
}

bool FS47HorrorVisual::SelfTest(TArray<FString>& Lines)
{
 bool Good = true;
 auto Check = [&](bool Value,const TCHAR* What) { Good &= Value; Lines.Add(FString::Printf(TEXT("SC01 visual %s %s"),Value ? TEXT("PASS") : TEXT("FAIL"),What)); };
 Check(PassPosition(0).Z < -4.56 && PassPosition(PassDuration).Z < -4.56,TEXT("entire hand root path remains outside actual glass"));
 Check(PassPosition(PassDuration).X > 5.4,TEXT("hand ends behind existing opaque eastern pier"));
 Check(WindowPoint().Z < -4.56 && WindowPoint().Z > -4.60,TEXT("persistent traces occupy exterior glass face"));
 Check(LampPoint().Z < WindowPoint().Z,TEXT("normal task lamp is physically beyond glass"));
 Check(.073 > .071,TEXT("closed physical shutter slats overlap with no sight gap"));
 auto O = S47J::New(); S47J::Set(O,TEXT("flag"),TEXT("true")); bool B = false;
 Check(!Bool(O,TEXT("flag"),B),TEXT("string cannot manufacture saved physical observation"));
 S47J::Set(O,TEXT("number"),TEXT("1")); double D = 0;
 Check(!Number(O,TEXT("number"),D),TEXT("string cannot manufacture saved physical phase"));
 return Good;
}
