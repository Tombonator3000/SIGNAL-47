#include "S47HorrorSC03.h"

#include "S47Audio.h"
#include "S47Horror.h"
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
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47HorrorSC03, Log, All);

namespace
{
 // Narrative pacing in played seconds. None of these moves the story clock.
 constexpr double WaitFor = 25;      // in view of the counter before wagon 23 pulls in
 constexpr double DriveIn = 7;       // the wagon's last stretch from the road to the pumps
 constexpr double LiftAfter = 3.5;   // after the identification scene closes
 constexpr double Rise = 2.2, Hang = 1.6, SeeFor = .6, FadeLight = 1.8;
 constexpr double DeputyWalk = 9;
 const TCHAR* WAGON = TEXT("/Game/Free/Vehicles/fairheaven_sw84/StaticMeshes/fairheaven_sw84_body.fairheaven_sw84_body");
 const TCHAR* WAGON_DOOR = TEXT("/Game/Free/Vehicles/fairheaven_sw84/StaticMeshes/fairheaven_sw84_door_driver.fairheaven_sw84_door_driver");
 double Yaw(const FVector& From, const FVector& To) { return FMath::RadiansToDegrees(FMath::Atan2(To.Z - From.Z, To.X - From.X)); } // UE yaw, three x/z
 bool Flag(const S47J::FObj& O, const TCHAR* K, bool& Out)
 {
  const auto* V = O ? O->Values.Find(K) : nullptr;
  if (!V) { Out = false; return true; }
  if (!V->IsValid() || (*V)->Type != EJson::Boolean) return false;
  Out = (*V)->AsBool(); return true;
 }
}

struct FS47HorrorSC03::FWorld
{
 TWeakObjectPtr<UBoxComponent> Root;
 TWeakObjectPtr<UStaticMeshComponent> Wagon, Door, Thermos, OrtizBody, OrtizHead, OrtizLegs, DeputyBody, DeputyHead, DeputyHat;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Marks, Bar;
 TArray<TWeakObjectPtr<UTextRenderComponent>> Labels;
 TWeakObjectPtr<USpotLightComponent> Beam;
 TWeakObjectPtr<UPointLightComponent> Glow, Amber;
 TWeakObjectPtr<UStaticMeshComponent> Disc;
 TWeakObjectPtr<UMaterialInstanceDynamic> DiscMat;
 struct FFigure { TWeakObjectPtr<USceneComponent> Root, Head; FVector At; };
 TArray<FFigure> Figures;
 TArray<TWeakObjectPtr<UPrimitiveComponent>> All;
 TMap<FString, FS47Obj> Objects;
 FVector OrtizAt = FVector::ZeroVector, DeputyPos = FVector::ZeroVector;
 double Lift = 0;
};

FS47HorrorSC03::FS47HorrorSC03(US47Main* InM, FS47Horror* InHost) : W(MakeUnique<FWorld>()), M(InM), Host(InHost) {}
FS47HorrorSC03::~FS47HorrorSC03() = default;

const TCHAR* FS47HorrorSC03::PhaseName(EPhase In)
{
 switch (In)
 {
 case EPhase::Off: return TEXT("off");
 case EPhase::Report: return TEXT("report");
 case EPhase::Wait: return TEXT("wait");
 case EPhase::Arrival: return TEXT("arrival");
 case EPhase::Meet: return TEXT("meet");
 case EPhase::Lift: return TEXT("lift");
 case EPhase::Retreat: return TEXT("retreat");
 case EPhase::Calm: return TEXT("calm");
 case EPhase::Deputy: return TEXT("deputy");
 case EPhase::Complete: return TEXT("complete");
 }
 return TEXT("off");
}
bool FS47HorrorSC03::ParsePhase(const FString& Text, EPhase& Out)
{
 for (const EPhase C : {EPhase::Off, EPhase::Report, EPhase::Wait, EPhase::Arrival, EPhase::Meet, EPhase::Lift, EPhase::Retreat, EPhase::Calm, EPhase::Deputy, EPhase::Complete})
  if (Text == PhaseName(C)) { Out = C; return true; }
 return false;
}
S47J::FObj FS47HorrorSC03::Write(EPhase Phase, const FFacts& Facts)
{
 auto O = S47J::New(); S47J::Set(O, TEXT("v"), 1); S47J::Set(O, TEXT("phase"), PhaseName(Phase));
 S47J::Set(O, TEXT("dispatch"), Facts.bDispatch); S47J::Set(O, TEXT("assigned"), Facts.bAssigned); S47J::Set(O, TEXT("identified"), Facts.bIdentified);
 S47J::Set(O, TEXT("observed"), Facts.bObserved); S47J::Set(O, TEXT("retreat"), Facts.bRetreat);
 S47J::Set(O, TEXT("wagon"), Facts.bWagon); S47J::Set(O, TEXT("thermos"), Facts.bThermos); S47J::Set(O, TEXT("marks"), Facts.bMarks);
 S47J::Set(O, TEXT("missing"), Facts.bMissing); S47J::Set(O, TEXT("localOrder"), Facts.bLocalOrder); S47J::Set(O, TEXT("deputyHere"), Facts.bDeputyHere);
 S47J::Set(O, TEXT("dispatchStamp"), Facts.DispatchStamp); S47J::Set(O, TEXT("takenStamp"), Facts.TakenStamp);
 S47J::Set(O, TEXT("missingStamp"), Facts.MissingStamp); S47J::Set(O, TEXT("orderStamp"), Facts.OrderStamp);
 return O;
}
bool FS47HorrorSC03::Read(const S47J::FObj& O, EPhase& Phase, FFacts& Facts)
{
 Phase = EPhase::Off; Facts = FFacts();
 if (!O) return true; // absent: a case that never reached SC03
 const auto* V = O->Values.Find(TEXT("v"));
 if (!V || !V->IsValid() || (*V)->Type != EJson::Number || (*V)->AsNumber() != 1) return false;
 const auto* PV = O->Values.Find(TEXT("phase"));
 if (!PV || !PV->IsValid() || (*PV)->Type != EJson::String || !ParsePhase((*PV)->AsString(), Phase)) return false;
 bool Ok = Flag(O, TEXT("dispatch"), Facts.bDispatch) && Flag(O, TEXT("assigned"), Facts.bAssigned) && Flag(O, TEXT("identified"), Facts.bIdentified) &&
  Flag(O, TEXT("observed"), Facts.bObserved) && Flag(O, TEXT("retreat"), Facts.bRetreat) && Flag(O, TEXT("wagon"), Facts.bWagon) &&
  Flag(O, TEXT("thermos"), Facts.bThermos) && Flag(O, TEXT("marks"), Facts.bMarks) && Flag(O, TEXT("missing"), Facts.bMissing) &&
  Flag(O, TEXT("localOrder"), Facts.bLocalOrder) && Flag(O, TEXT("deputyHere"), Facts.bDeputyHere);
 if (!Ok) { Phase = EPhase::Off; Facts = FFacts(); return false; }
 Facts.DispatchStamp = S47J::Str(O, TEXT("dispatchStamp")); Facts.TakenStamp = S47J::Str(O, TEXT("takenStamp"));
 Facts.MissingStamp = S47J::Str(O, TEXT("missingStamp")); Facts.OrderStamp = S47J::Str(O, TEXT("orderStamp"));
 // Facts are causal: a later fact without its cause is rejected rather than invented.
 const bool Causal = (!Facts.bAssigned || Facts.bDispatch) && (!Facts.bIdentified || Facts.bAssigned) && (!Facts.bObserved || Facts.bIdentified) &&
  (!Facts.bRetreat || Facts.bObserved) && (!(Facts.bWagon || Facts.bThermos || Facts.bMarks) || Facts.bRetreat) && (!Facts.bMissing || Facts.bRetreat) &&
  (!Facts.bLocalOrder || Facts.bMissing) && (!Facts.bDeputyHere || Facts.bMissing);
 if (!Causal) { Phase = EPhase::Off; Facts = FFacts(); return false; }
 return true;
}

void FS47HorrorSC03::Begin(const S47J::FObj& Saved, bool bLive)
{
 Stop();
 EPhase Phase; FFacts Facts;
 if (!Read(Saved, Phase, Facts)) UE_LOG(LogS47HorrorSC03, Warning, TEXT("S47: SC03 save rejected; no SC03 facts restored"));
 P = bLive ? Phase : EPhase::Off; F = bLive ? Facts : FFacts();
 // Re-derive the phase from the facts so a half sequence resumes at its safe point.
 if (P == EPhase::Arrival) P = EPhase::Meet;                       // the wagon is parked on restore
 if (P == EPhase::Meet && F.bIdentified) P = EPhase::Lift;          // the scene was shown; the lift is owed
 if (P == EPhase::Lift && F.bObserved) P = EPhase::Retreat;         // she is gone; cover is owed
 if (P == EPhase::Retreat && F.bRetreat) P = EPhase::Calm;
 if (P == EPhase::Calm && F.bMissing && F.bLocalOrder) P = EPhase::Deputy;
 WaitT = ArriveT = LiftT = LookT = HeldT = Gone = DeputyT = 0; LiftDelay = 0; bOrtizGone = F.bObserved;
 LightK = 0; FigureK = (P == EPhase::Retreat) ? 1 : 0; TurnK = FigureK;
 if (Live() && !Build()) UE_LOG(LogS47HorrorSC03, Warning, TEXT("S47: SC03 world unavailable; facts kept, world waits"));
 Apply();
}

void FS47HorrorSC03::Start()
{
 if (P != EPhase::Off) return;
 F = FFacts(); SetPhase(EPhase::Report); Build(); Apply();
}

void FS47HorrorSC03::Stop()
{
 if (W && W->Root.IsValid()) W->Root->SetVisibility(false, true);
}

S47J::FObj FS47HorrorSC03::Save() const
{
 // A lift in progress is saved as owed (Meet with the identification done), never as half a lift.
 EPhase S = P;
 if (S == EPhase::Arrival) S = EPhase::Meet;
 return Write(S, F);
}

FVector FS47HorrorSC03::Eye() const
{
 if (!M || !M->Player) return FVector::ZeroVector;
 return FVector(M->Player->Pos.X, M->Player->FloorY + M->Player->Eye, M->Player->Pos.Z);
}

bool FS47HorrorSC03::AtFuelStop(double Radius) const
{
 if (!M || !M->World || !M->Player || Counter.IsZero()) return false;
 if ((M->World->Area != TEXT("diner") && M->World->Area != TEXT("road")) || M->World->bDriving) return false;
 return FVector2D(M->Player->Pos.X - Counter.X, M->Player->Pos.Z - Counter.Z).Size() < Radius;
}

bool FS47HorrorSC03::Busy() const
{
 return (P == EPhase::Arrival || P == EPhase::Meet || P == EPhase::Lift || P == EPhase::Retreat) && M && M->World &&
  (M->World->Area == TEXT("diner") || M->World->Area == TEXT("road"));
}

double FS47HorrorSC03::Ground(double X, double Z) const
{
 // Hidden exported/old-road components can still block queries. Settle only on
 // a shown, registered surface in the fuel stop's currently loaded area.
 if (!M || !M->Web || !M->Web->GetWorld() || !M->World ||
  (M->World->Area != TEXT("diner") && M->World->Area != TEXT("road"))) return TNumericLimits<double>::Lowest();
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SC03Ground), true);
 for (const auto& C : W->All) if (C.IsValid()) Q.AddIgnoredComponent(C.Get());
 const double From = GroundRef + 1.0; // below the canopy, at the night window
 for (int32 Attempt = 0; Attempt < 32; ++Attempt)
 {
  FHitResult H;
  if (!M->Web->GetWorld()->LineTraceSingleByChannel(H, S47Three::ToUE(X, From, Z), S47Three::ToUE(X, From - 12, Z), ECC_Visibility, Q)) break;
  const auto* C = H.GetComponent(); const int32 Id = C ? M->Web->NodeOf(C) : -1;
  const auto* N = M->Web->Node(Id);
  if (!C || !C->IsRegistered() || !N || N->Comp.Get() != C) break;
  bool Shown = C->IsVisible() && !C->bHiddenInGame && C->GetOwner() && !C->GetOwner()->IsHidden();
  int32 Parent = Id, Depth = 0;
  for (; Parent >= 0 && Depth < 128; ++Depth)
  {
   const auto* A = M->Web->Node(Parent); if (!A || !A->Comp.IsValid()) return TNumericLimits<double>::Lowest();
   // A hidden attachment helper does not hide a child unless visibility was propagated.
   Shown &= A->bVis; Parent = A->Parent;
  }
  if (Parent >= 0) break;
  if (!Shown) { Q.AddIgnoredComponent(C); continue; }
  if (H.bBlockingHit && H.ImpactNormal.Z > .5) return H.ImpactPoint.Z / 100.0;
  break; // a shown wall, underside or unknown blocker must not be bypassed
 }
 return TNumericLimits<double>::Lowest(); // nothing loaded under it yet
}

bool FS47HorrorSC03::Build()
{
 if (bBuilt) return true;
 if (bBuildFailed || !M || !M->Web || !M->Inter || !M->Web->IsLoaded()) return false;
 AS47Web* Web = M->Web.Get();
 const FS47Interactable* GasWindow = M->Inter->Get(TEXT("town:gasWindow"));
 const FS47Interactable* Pumps = M->Inter->Get(TEXT("town:pumps"));
 const FS47Interactable* Sheriff = M->Inter->Get(TEXT("town:sheriffDoor"));
 FS47Obj Group = Web->Obj(TEXT("world.town.group"));
 if (!Group) Group = Web->Obj(TEXT("world.diner.group"));
 const FS47WebNode* GroupNode = Group ? Web->Node(Group.I) : nullptr;
 USceneComponent* GroupComp = GroupNode ? GroupNode->Comp.Get() : nullptr;
 auto Load = [](const TCHAR* Path) { return LoadObject<UStaticMesh>(nullptr, Path); };
 auto LoadPale = [](const TCHAR* Name) { return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Signal47/HorrorSC02/%s.%s"), Name, Name)); };
 UStaticMesh* WagonMesh = Load(WAGON); UStaticMesh* DoorMesh = Load(WAGON_DOOR);
 UStaticMesh* Cube = Load(TEXT("/Engine/BasicShapes/Cube.Cube")); UStaticMesh* Cyl = Load(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 UStaticMesh* Sphere = Load(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 UStaticMesh* Torso = LoadPale(TEXT("SM_SC02_PaleTorso")); UStaticMesh* Head = LoadPale(TEXT("SM_SC02_PaleHead"));
 UStaticMesh* UpperArm = LoadPale(TEXT("SM_SC02_PaleUpperArm")); UStaticMesh* LowerArm = LoadPale(TEXT("SM_SC02_PaleLowerArm"));
 UStaticMesh* Thigh = LoadPale(TEXT("SM_SC02_PaleThigh")); UStaticMesh* Shin = LoadPale(TEXT("SM_SC02_PaleShin"));
 auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 auto* GlowBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebGlow.M_WebGlow"));
 if (!GasWindow || !Pumps || !GroupComp || !WagonMesh || !DoorMesh || !Cube || !Cyl || !Sphere || !Torso || !Head || !UpperArm || !LowerArm || !Thigh || !Shin || !Base)
 {
  bBuildFailed = true;
  UE_LOG(LogS47HorrorSC03, Error, TEXT("S47: SC03 fuel stop, wagon or pale figures unavailable; SC03 world not built"));
  return false;
 }
 Counter = GasWindow->Object.WorldPosition(); GroundRef = Counter.Y;
 const FVector PumpAt = Pumps->Object.WorldPosition();
 // Wagon 23 stops beyond the pumps, nose towards the road, in plain view of the night window.
 const FVector Away = FVector(PumpAt.X - Counter.X, 0, PumpAt.Z - Counter.Z).GetSafeNormal();
 Park = PumpAt + Away * 5.5 + FVector(Away.Z, 0, -Away.X) * 2.0;
 // The former pivot was inside the south canopy post. Move the complete scene
 // 2.95 m sideways onto the preserved apron; approach, people and props follow Park.
 Park += FVector(-Away.Z, 0, Away.X) * 2.95;
 Approach = Park + Away * 34;
 DeputyFrom = Sheriff ? Sheriff->Object.WorldPosition() : Counter + FVector(0, 0, 25);
 if (true)
 {
  Park.Y = Approach.Y = DeputyFrom.Y = GroundRef - 1.4; // settled by Apply once the apron is loaded
 }
 DeputyAt = Counter + Away * 3.2 + FVector(-Away.Z, 0, Away.X) * 2.5; DeputyAt.Y = GroundRef - 1.4;

 UBoxComponent* Root = NewObject<UBoxComponent>(Web, TEXT("SC03Root"));
 Root->SetMobility(EComponentMobility::Movable); Root->SetupAttachment(GroupComp);
 Root->SetCollisionEnabled(ECollisionEnabled::NoCollision); Root->SetBoxExtent(FVector(1)); Root->RegisterComponent();
 Root->SetWorldLocation(S47Three::ToUE(Park));
 const int32 RootNode = Web->AdoptNode(Root, Group.I, TEXT("sc03:root"));
 W->Root = Root;
 auto Mat = [Web, Base](uint32 Color, float Rough, float Emissive = 0)
 {
  auto* Mid = UMaterialInstanceDynamic::Create(Base, Web);
  Mid->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(Color)); Mid->SetScalarParameterValue(TEXT("Roughness"), Rough);
  Mid->SetScalarParameterValue(TEXT("Metalness"), 0); Mid->SetScalarParameterValue(TEXT("NativeLighting"), 1);
  Mid->SetScalarParameterValue(TEXT("EmissivePower"), Emissive); return Mid;
 };
 auto Mesh = [&](const TCHAR* Name, UStaticMesh* Asset, USceneComponent* Parent, UMaterialInterface* M0, bool bPick)
 {
  auto* C = NewObject<UStaticMeshComponent>(Web, Name);
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(Parent); C->SetStaticMesh(Asset);
  if (M0) C->SetMaterial(0, M0);
  C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore);
  C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
  if (bPick) C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
  C->SetCastShadow(true); C->RegisterComponent(); W->All.Add(C);
  return C;
 };
 auto* White = Mat(0xd9d7cf, .55f); auto* Coat = Mat(0x2f4a6a, .8f); auto* Denim = Mat(0x2a3140, .85f); auto* Skin = Mat(0xb08468, .7f);
 auto* Steel = Mat(0x9aa3a6, .35f); auto* Tape = Mat(0xe8d9a0, .6f); auto* Dark = Mat(0x2b2620, .95f); auto* Khaki = Mat(0x8a7a52, .8f);
 auto* Amber = Mat(0xffa21c, .4f, 4.f); auto* Hat = Mat(0x5a4a32, .8f);

 W->Wagon = Mesh(TEXT("SC03Wagon23"), WagonMesh, Root, nullptr, true);
 W->Door = Mesh(TEXT("SC03Wagon23Door"), DoorMesh, Root, nullptr, true);
 // An amber bar on the roof and the district number on both front doors set wagon twenty-three
 // apart from Reyes's own station wagon of the same make.
 for (int32 I = 0; I < 3; ++I)
 {
  auto* B = Mesh(*FString::Printf(TEXT("SC03Wagon23Bar%d"), I), Cube, Root, I == 1 ? (UMaterialInterface*)Steel : Amber, false);
  W->Bar.Add(B);
 }
 for (int32 Side = 0; Side < 2; ++Side)
 {
  auto* T = NewObject<UTextRenderComponent>(Web, *FString::Printf(TEXT("SC03Wagon23Label%d"), Side));
  T->SetMobility(EComponentMobility::Movable); T->SetupAttachment(Root);
  T->SetText(FText::FromString(TEXT("DISTRICT\nMAINTENANCE  23"))); T->SetWorldSize(9.f);
  T->SetHorizontalAlignment(EHTA_Center); T->SetVerticalAlignment(EVRTA_TextCenter); T->SetTextRenderColor(FColor(28, 54, 92));
  T->SetCollisionEnabled(ECollisionEnabled::NoCollision); T->RegisterComponent(); W->Labels.Add(T);
 }
 auto* AmberLight = NewObject<UPointLightComponent>(Web, TEXT("SC03Wagon23Amber"));
 AmberLight->SetMobility(EComponentMobility::Movable); AmberLight->SetupAttachment(Root); AmberLight->SetLightColor(FLinearColor(1.f, .55f, .1f));
 AmberLight->SetIntensityUnits(ELightUnits::Lumens); AmberLight->SetIntensity(0); AmberLight->SetAttenuationRadius(900); AmberLight->SetCastShadows(false);
 AmberLight->RegisterComponent(); W->Amber = AmberLight;

 // Mara Ortiz: work coat, jeans, short dark hair. Built from basic shapes like the night's other people.
 W->OrtizLegs = Mesh(TEXT("SC03OrtizLegs"), Cyl, Root, Denim, true);
 W->OrtizBody = Mesh(TEXT("SC03OrtizCoat"), Cyl, Root, Coat, true);
 W->OrtizHead = Mesh(TEXT("SC03OrtizHead"), Sphere, Root, Skin, true);
 W->Thermos = Mesh(TEXT("SC03Thermos"), Cyl, Root, Steel, true);
 auto* ThermosTape = Mesh(TEXT("SC03ThermosTape"), Cyl, W->Thermos.Get(), Tape, false);
 ThermosTape->SetRelativeScale3D(FVector(1.04, 1.04, .22)); ThermosTape->SetRelativeLocation(FVector(0, 0, 8));
 for (int32 I = 0; I < 5; ++I) W->Marks.Add(Mesh(*FString::Printf(TEXT("SC03GravelMark%d"), I), Cube, Root, Dark, I == 0));

 // The light: a cold beam from far above, its source a glare disc, and a fill on the ground.
 auto* Beam = NewObject<USpotLightComponent>(Web, TEXT("SC03LiftBeam"));
 Beam->SetMobility(EComponentMobility::Movable); Beam->SetupAttachment(Root); Beam->SetLightColor(FLinearColor(.78f, .9f, 1.f));
 Beam->SetIntensityUnits(ELightUnits::Lumens); Beam->SetIntensity(0); Beam->SetAttenuationRadius(6000); Beam->SetInnerConeAngle(5); Beam->SetOuterConeAngle(9);
 Beam->SetCastShadows(true); Beam->SetVolumetricScatteringIntensity(4.f); Beam->RegisterComponent(); W->Beam = Beam;
 auto* Glow = NewObject<UPointLightComponent>(Web, TEXT("SC03LiftGlow"));
 Glow->SetMobility(EComponentMobility::Movable); Glow->SetupAttachment(Root); Glow->SetLightColor(FLinearColor(.8f, .92f, 1.f));
 Glow->SetIntensityUnits(ELightUnits::Lumens); Glow->SetIntensity(0); Glow->SetAttenuationRadius(1500); Glow->SetCastShadows(false); Glow->RegisterComponent(); W->Glow = Glow;
 if (GlowBase)
 {
  auto* DiscMat = UMaterialInstanceDynamic::Create(GlowBase, Web);
  DiscMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(.85f, .95f, 1.f)); DiscMat->SetScalarParameterValue(TEXT("Opacity"), 0);
  auto* Disc = NewObject<UStaticMeshComponent>(Web, TEXT("SC03LiftSource"));
  Disc->SetMobility(EComponentMobility::Movable); Disc->SetupAttachment(Root); Disc->SetStaticMesh(Sphere); Disc->SetMaterial(0, DiscMat);
  Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision); Disc->SetCastShadow(false); Disc->RegisterComponent();
  W->Disc = Disc; W->DiscMat = DiscMat;
 }

 // Pale figures standing still at the edge of the light: the SC02 body, posed rigidly, no rule of its own.
 const FVector Side(-Away.Z, 0, Away.X);
 const FVector Spots[3] = {Park + Away * 11.5 + Side * 4.0, Park + Away * 13.0 - Side * 3.0, Park - Side * 10.5 + Away * 3.0};
 for (int32 I = 0; I < 3; ++I)
 {
  FWorld::FFigure Fig; Fig.At = Spots[I];
  Fig.At.Y = GroundRef - 1.4;
  auto* FR = NewObject<UBoxComponent>(Web, *FString::Printf(TEXT("SC03Figure%d"), I));
  FR->SetMobility(EComponentMobility::Movable); FR->SetupAttachment(Root); FR->SetCollisionEnabled(ECollisionEnabled::NoCollision); FR->SetBoxExtent(FVector(1)); FR->RegisterComponent();
  auto Part = [&](const TCHAR* N, UStaticMesh* Asset, USceneComponent* Parent, const FVector& At, bool bMirror)
  {
   auto* C = Mesh(*FString::Printf(TEXT("SC03Figure%d%s"), I, N), Asset, Parent, nullptr, false);
   C->SetRelativeLocation(At); if (bMirror) C->SetRelativeScale3D(FVector(1, -1, 1)); return C;
  };
  auto* T0 = Part(TEXT("Torso"), Torso, FR, FVector(0, 0, 102), false); T0->SetRelativeRotation(FRotator(-9, 0, 0));
  Fig.Head = Part(TEXT("Head"), Head, T0, FVector(7.2, 0, 68), false);
  Part(TEXT("ForeL"), LowerArm, Part(TEXT("ArmL"), UpperArm, T0, FVector(4.6, -21, 58), false), FVector(0, 0, -36), false);
  Part(TEXT("ForeR"), LowerArm, Part(TEXT("ArmR"), UpperArm, T0, FVector(4.6, 21, 58), true), FVector(0, 0, -36), false);
  Part(TEXT("ShinL"), Shin, Part(TEXT("ThighL"), Thigh, FR, FVector(0, -10, 100), false), FVector(0, 0, -49), false);
  Part(TEXT("ShinR"), Shin, Part(TEXT("ThighR"), Thigh, FR, FVector(0, 10, 100), true), FVector(0, 0, -49), false);
  Fig.Root = FR; W->Figures.Add(Fig);
 }

 // The deputy: tan uniform and hat, out of the substation office when the missing report is made.
 W->DeputyBody = Mesh(TEXT("SC03DeputyBody"), Cyl, Root, Khaki, true);
 W->DeputyHead = Mesh(TEXT("SC03DeputyHead"), Sphere, Root, Skin, true);
 W->DeputyHat = Mesh(TEXT("SC03DeputyHat"), Cyl, Root, Hat, false);

 for (const TPair<FString, UStaticMeshComponent*>& Pick : {TPair<FString, UStaticMeshComponent*>(TEXT("horror:ortiz"), W->OrtizBody.Get()),
  TPair<FString, UStaticMeshComponent*>(TEXT("horror:wagon23"), W->Wagon.Get()), TPair<FString, UStaticMeshComponent*>(TEXT("horror:thermos"), W->Thermos.Get()),
  TPair<FString, UStaticMeshComponent*>(TEXT("horror:gravelMarks"), W->Marks[0].Get()), TPair<FString, UStaticMeshComponent*>(TEXT("horror:deputy"), W->DeputyBody.Get())})
 {
  const int32 Node = Web->AdoptNode(Pick.Value, RootNode, FString(TEXT("sc03:")) + Pick.Key.RightChop(7));
  W->Objects.Add(Pick.Key, Web->ObjAt(Node));
  FS47Interactable It; It.Id = Pick.Key; It.Object = W->Objects[Pick.Key]; It.Range = Pick.Key == TEXT("horror:wagon23") ? 3.4 : 2.6;
  const FString Id = Pick.Key;
  It.Label = [this, Id]() -> FString
  {
   if (!Live()) return FString();
   if (Id == TEXT("horror:ortiz")) return P == EPhase::Meet && !F.bIdentified ? FString(TEXT("Meet Mara Ortiz")) : FString();
   if (P != EPhase::Calm && P != EPhase::Deputy) return FString();
   if (Id == TEXT("horror:wagon23")) return F.bWagon ? FString() : FString(TEXT("Look into wagon twenty-three"));
   if (Id == TEXT("horror:thermos")) return F.bThermos ? FString() : FString(TEXT("Look at the thermos"));
   if (Id == TEXT("horror:gravelMarks")) return F.bMarks ? FString() : FString(TEXT("Look at the marks in the gravel"));
   if (Id == TEXT("horror:deputy")) return P == EPhase::Deputy && F.bDeputyHere ? FString(TEXT("Talk to the deputy")) : FString();
   return FString();
  };
  It.Use = [this, Id] { if (Host) Host->Use(Id); };
  It.Examine = [this, Id]() -> FString
  {
   if (Id == TEXT("horror:wagon23")) return bOrtizGone ? TEXT("DISTRICT MAINTENANCE 23. The driver's door stands open. Her clipboard is on the seat.") : TEXT("A white district wagon, number twenty-three, amber bar on the roof.");
   if (Id == TEXT("horror:thermos")) return TEXT("A steel thermos. A strip of tape on it: M. ORTIZ.");
   if (Id == TEXT("horror:gravelMarks")) return TEXT("Narrow pressed marks in the gravel. They start beside the wagon and do not go anywhere.");
   if (Id == TEXT("horror:ortiz")) return TEXT("A woman in a district work coat, card clipped to the pocket.");
   if (Id == TEXT("horror:deputy")) return TEXT("The county deputy, notebook out.");
   return FString();
  };
  M->Inter->Add(It);
 }
 for (const FS47Rect& Z : Web->Zones())
  if (Z.bEnabled && FVector2D(FMath::Clamp(Counter.X, Z.MinX, Z.MaxX) - Counter.X, FMath::Clamp(Counter.Z, Z.MinZ, Z.MaxZ) - Counter.Z).Size() < 12)
   UE_LOG(LogS47HorrorSC03, Display, TEXT("S47: SC03 zone near the counter: %s x %.1f..%.1f z %.1f..%.1f"), *Z.Id, Z.MinX, Z.MaxX, Z.MinZ, Z.MaxZ);
 bBuilt = true;
 UE_LOG(LogS47HorrorSC03, Display, TEXT("S47: SC03 fuel stop built: counter %.1f %.1f, wagon park %.1f %.1f, three still figures"), Counter.X, Counter.Z, Park.X, Park.Z);
 return true;
}

FVector FS47HorrorSC03::OrtizPoint() const { return W->OrtizAt + FVector(0, 1.25 + W->Lift, 0); }
TArray<FVector> FS47HorrorSC03::FigureHeads() const
{
 TArray<FVector> Out; for (const auto& Fig : W->Figures) Out.Add(Fig.At + FVector(0, 1.75, 0)); return Out;
}

void FS47HorrorSC03::Apply()
{
 if (!bBuilt || !W->Root.IsValid()) return;
 const bool bShow = Live() || P == EPhase::Complete;
 W->Root->SetVisibility(bShow, true);
 if (!bShow) return;
 if (!bGrounded)
 {
  // Caliche may still be unloaded when SC03 is built at SARO: settle everything once the apron is there.
  const double Y = Ground(Park.X, Park.Z);
  if (Y > -1e6)
  {
   Park.Y = Y; const double A = Ground(Approach.X, Approach.Z); Approach.Y = A > -1e6 ? A : Y;
   const double D0 = Ground(DeputyFrom.X, DeputyFrom.Z); DeputyFrom.Y = D0 > -1e6 ? D0 : Y; DeputyAt.Y = Y;
   for (auto& Fig : W->Figures) { const double G = Ground(Fig.At.X, Fig.At.Z); Fig.At.Y = G > -1e6 ? G : Y; }
   bGrounded = true;
   UE_LOG(LogS47HorrorSC03, Display, TEXT("S47: SC03 apron grounded at %.2f"), Y);
  }
 }
 const FVector Away = (Approach - Park).GetSafeNormal();
 const double Heading = FMath::Atan2(-Away.X, -Away.Z); // nose towards the road
 // Wagon: approaching, then parked; hidden before Ortiz is on her way.
 const bool bWagon = P >= EPhase::Arrival;
 FVector WagonAt = Park;
 if (P == EPhase::Arrival) { const double K = FMath::SmoothStep(0., 1., FMath::Clamp(ArriveT / DriveIn, 0., 1.)); WagonAt = FMath::Lerp(Approach, Park, K); }
 const FQuat Car = S47Three::Euler(0, Heading, 0);
 auto PoseCar = [&](UStaticMeshComponent* C, const FVector& At, const FQuat& Q)
 {
  if (!C) return; double E[16]; S47Three::Compose(At, Q, FVector::OneVector, E);
  C->SetWorldTransform(S47Three::MatToUE(E)); C->AddLocalRotation(FRotator(0, -90, 0));
 };
 PoseCar(W->Wagon.Get(), WagonAt, Car);
 const double DoorAngle = bOrtizGone ? 1.0 : (P == EPhase::Meet || P == EPhase::Lift) ? .35 : 0;
 {
  const FVector Hinge(-0.73, 0.72, -0.86); const FQuat Leaf = S47Three::Euler(0, -DoorAngle, 0);
  PoseCar(W->Door.Get(), WagonAt + Car.RotateVector(Hinge - Leaf.RotateVector(Hinge)), Car * Leaf);
 }
 if (W->Wagon.IsValid()) W->Wagon->SetVisibility(bWagon); if (W->Door.IsValid()) W->Door->SetVisibility(bWagon);
 // The parked wagon is solid to walk into (its turned body's bounding box on the apron).
 if (M->Web)
 {
  auto& Cs = M->Web->CollidersMutable(TEXT("diner"));
  int32 Index = Cs.IndexOfByPredicate([](const FS47Rect& R) { return R.Id == TEXT("sc03:wagon23"); });
  if (Index == INDEX_NONE) { FS47Rect R; R.Id = TEXT("sc03:wagon23"); Index = Cs.Add(R); }
  const double C = FMath::Abs(FMath::Cos(Heading)), S = FMath::Abs(FMath::Sin(Heading)), HalfW = .95, HalfL = 2.55;
  const double Ex = C * HalfW + S * HalfL, Ez = S * HalfW + C * HalfL;
  Cs[Index].MinX = WagonAt.X - Ex; Cs[Index].MaxX = WagonAt.X + Ex; Cs[Index].MinZ = WagonAt.Z - Ez; Cs[Index].MaxZ = WagonAt.Z + Ez;
  Cs[Index].bEnabled = bWagon && P != EPhase::Arrival;
 }
 // Keep the fixtures at their body-relative heights after removing the extra body lift.
 const FVector Up(0, 1, 0), Right = Car.RotateVector(FVector(1, 0, 0)), Fwd = Car.RotateVector(FVector(0, 0, -1));
 for (int32 I = 0; I < W->Bar.Num(); ++I) if (auto* B = W->Bar[I].Get())
 {
  B->SetWorldLocation(S47Three::ToUE(WagonAt + Up * 1.52 + Right * ((I - 1) * .42) + Fwd * .25));
  B->SetWorldRotation(FRotator(0, -FMath::RadiansToDegrees(Heading) - 90, 0)); B->SetWorldScale3D(FVector(.36, .12, .09)); B->SetVisibility(bWagon);
 }
 for (int32 I = 0; I < W->Labels.Num(); ++I) if (auto* T = W->Labels[I].Get())
 {
  const double S = I == 0 ? 1 : -1;
  T->SetWorldLocation(S47Three::ToUE(WagonAt + Up * .55 + Right * (S * .93) + Fwd * .6));
  T->SetWorldRotation(FRotator(0, -FMath::RadiansToDegrees(Heading) + (S > 0 ? 0 : 180), 0)); T->SetVisibility(bWagon);
 }
 if (W->Amber.IsValid())
 {
  W->Amber->SetWorldLocation(S47Three::ToUE(WagonAt + Up * 1.7));
  const bool On = bWagon && !bOrtizGone && P != EPhase::Lift; W->Amber->SetIntensity(On ? 1800.f * (FMath::Fmod(Clock, 1.0) < .5 ? 1.f : .25f) : 0.f);
 }
 // Ortiz beside the driver's door.
 W->OrtizAt = WagonAt + Right * -1.6 + Fwd * .4; { const double G = Ground(W->OrtizAt.X, W->OrtizAt.Z); W->OrtizAt.Y = G > -1e6 ? G : Park.Y; }
 const bool bOrtiz = (P == EPhase::Meet || P == EPhase::Lift) && !bOrtizGone;
 const FVector O0 = W->OrtizAt + FVector(0, W->Lift, 0);
 const double Face = M->Player ? Yaw(W->OrtizAt, M->Player->Pos) : 0;
 auto Place = [](UStaticMeshComponent* C, const FVector& At, const FVector& Scale, double YawDeg, bool bOn)
 { if (!C) return; C->SetWorldLocation(S47Three::ToUE(At)); C->SetWorldRotation(FRotator(0, YawDeg, 0)); C->SetWorldScale3D(Scale); C->SetVisibility(bOn); };
 Place(W->OrtizLegs.Get(), O0 + FVector(0, .45, 0), FVector(.30, .22, .90), Face, bOrtiz);
 Place(W->OrtizBody.Get(), O0 + FVector(0, 1.18, 0), FVector(.42, .30, .62), Face, bOrtiz);
 Place(W->OrtizHead.Get(), O0 + FVector(0, 1.62, 0), FVector(.21, .23, .25), Face, bOrtiz);
 // The thermos rolls out once she is gone; before that it is in her hand.
 const FVector ThermosAt = bOrtizGone ? W->OrtizAt + Right * -.9 + Fwd * .7 + FVector(0, .05, 0) : O0 + FVector(.28, .92, 0);
 Place(W->Thermos.Get(), ThermosAt, FVector(.08, .08, .26), Face, bOrtiz || bOrtizGone);
 if (bOrtizGone && W->Thermos.IsValid()) W->Thermos->SetWorldRotation(FRotator(0, Face, 90));
 for (int32 I = 0; I < W->Marks.Num(); ++I) Place(W->Marks[I].Get(), W->OrtizAt + Right * (-.25 + I * .13) + Fwd * (.15 * I) + FVector(0, .01, 0), FVector(.05, .4, .012), Face + I * 7, bOrtizGone);
 // The light.
 const FVector Source = W->OrtizAt + FVector(0, 38, 0);
 if (W->Beam.IsValid()) { W->Beam->SetWorldLocation(S47Three::ToUE(Source)); W->Beam->SetWorldRotation(FRotator(-90, 0, 0)); W->Beam->SetIntensity(LightK * 2.6e6f); }
 if (W->Glow.IsValid()) { W->Glow->SetWorldLocation(S47Three::ToUE(W->OrtizAt + FVector(0, 3, 0))); W->Glow->SetIntensity(LightK * 9000.f); }
 if (W->Disc.IsValid()) { W->Disc->SetWorldLocation(S47Three::ToUE(Source)); W->Disc->SetWorldScale3D(FVector(9, 9, 1.2)); W->Disc->SetVisibility(LightK > .01); }
 if (W->DiscMat.IsValid()) W->DiscMat->SetScalarParameterValue(TEXT("Opacity"), LightK);
 // Figures: still, facing the wagon; one turns its head towards Reyes.
 const bool bFigures = FigureK > .01 && P == EPhase::Retreat;
 for (int32 I = 0; I < W->Figures.Num(); ++I)
 {
  auto& Fig = W->Figures[I];
  if (!Fig.Root.IsValid()) continue;
  Fig.Root->SetVisibility(bFigures, true);
  Fig.Root->SetWorldLocation(S47Three::ToUE(Fig.At)); Fig.Root->SetWorldRotation(FRotator(0, Yaw(Fig.At, W->OrtizAt), 0));
  if (I == 0 && Fig.Head.IsValid() && M->Player)
  {
   const double ToPlayer = FRotator::NormalizeAxis(Yaw(Fig.At, M->Player->Pos) - Yaw(Fig.At, W->OrtizAt));
   Fig.Head->SetRelativeRotation(FRotator(0, FMath::Clamp(ToPlayer, -80., 80.) * TurnK, 0));
  }
 }
 // The deputy walks from the substation office to the pumps once the missing report is in.
 const bool bDeputy = P == EPhase::Deputy;
 if (bDeputy)
 {
  const double K = F.bDeputyHere ? 1 : FMath::Clamp(DeputyT / DeputyWalk, 0., 1.);
  W->DeputyPos = FMath::Lerp(DeputyFrom, DeputyAt, K); { const double G = Ground(W->DeputyPos.X, W->DeputyPos.Z); W->DeputyPos.Y = G > -1e6 ? G : Park.Y; }
  const double DFace = K >= 1 && M->Player ? Yaw(W->DeputyPos, M->Player->Pos) : Yaw(DeputyFrom, DeputyAt);
  const double Bob = K < 1 ? FMath::Abs(FMath::Sin(DeputyT * 6)) * .03 : 0;
  Place(W->DeputyBody.Get(), W->DeputyPos + FVector(0, .9 + Bob, 0), FVector(.40, .30, .90), DFace, true);
  Place(W->DeputyHead.Get(), W->DeputyPos + FVector(0, 1.66 + Bob, 0), FVector(.21, .23, .25), DFace, true);
  Place(W->DeputyHat.Get(), W->DeputyPos + FVector(0, 1.8 + Bob, 0), FVector(.42, .42, .05), DFace, true);
 }
 else for (auto* C : {W->DeputyBody.Get(), W->DeputyHead.Get(), W->DeputyHat.Get()}) if (C) C->SetVisibility(false);
}

bool FS47HorrorSC03::ClearLine(const FVector& From, const FVector& To) const
{
 if (!M || !M->Web || !M->Web->GetWorld()) return false;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SC03Sight), true);
 for (const auto& C : W->All) if (C.IsValid()) Q.AddIgnoredComponent(C.Get());
 const FVector A = S47Three::ToUE(From), B = S47Three::ToUE(To);
 for (int32 I = 0; I < 24; ++I)
 {
  FHitResult H;
  if (!M->Web->GetWorld()->LineTraceSingleByChannel(H, A, B, ECC_Visibility, Q)) return true;
  const UPrimitiveComponent* C = H.GetComponent(); if (!C) return false;
  bool Solid = C->IsVisible();
  const auto* Node = M->Web->Node(M->Web->NodeOf(C));
  if (Node && Node->Name.StartsWith(TEXT("proxy"))) Solid = false;
  if (const auto* Mat = C->GetMaterial(FMath::Max<int32>(0, H.ElementIndex)))
  {
   float Opacity = 1; Mat->GetScalarParameterValue(TEXT("Opacity"), Opacity);
   if (Mat->GetBlendMode() == BLEND_Additive || (Mat->GetBlendMode() == BLEND_Translucent && Opacity < .5f)) Solid = false;
  }
  if (Solid) return false;
  Q.AddIgnoredComponent(C);
 }
 return false;
}

bool FS47HorrorSC03::Sees(const FVector& Point, double Margin) const
{
 if (!M || !M->Player) return false;
 const FS47Cam Cam = FS47Cam::Of(M->Player);
 if (FVector::Dist(Cam.Pos, Point) > 90) return false;
 const FVector N = Cam.Project(Point);
 if (N.Z >= 1 || FMath::Abs(N.X) > Margin || FMath::Abs(N.Y) > Margin) return false;
 return ClearLine(Cam.Pos, Point);
}

bool FS47HorrorSC03::PlayerCovered() const
{
 // Inside SARO 07's cab, or with solid cover between Reyes and every still figure and the light.
 if (!M || !M->World || !M->Player) return false;
 if (M->World->bDriving) return true;
 // Inside the fuel station's walk zone the building itself is the cover.
 for (const FS47Rect& Z : M->Web->Zones())
  if (Z.bEnabled && (Z.Id.Contains(TEXT("gas")) || Z.Id.Contains(TEXT("station"))) && Z.MinX <= M->Player->Pos.X && M->Player->Pos.X <= Z.MaxX && Z.MinZ <= M->Player->Pos.Z && M->Player->Pos.Z <= Z.MaxZ)
   return true;
 const FVector E = Eye();
 for (const FVector& H : FigureHeads()) if (ClearLine(H, E)) return false;
 return !ClearLine(W->OrtizAt + FVector(0, 3, 0), E);
}

void FS47HorrorSC03::SetPhase(EPhase Next)
{
 if (Next == P) return;
 UE_LOG(LogS47HorrorSC03, Display, TEXT("S47: SC03 %s -> %s at %s"), PhaseName(P), PhaseName(Next), M && M->Story ? *M->Story->ClockText(true) : TEXT("?"));
 P = Next; ++Version;
}

void FS47HorrorSC03::Step(double Dt)
{
 if (!Live() || !M || !M->World || !M->Player) return;
 if (!bBuilt && !Build()) return;
 Clock += Dt;
 const bool bHeld = M->bPausedByMenu || M->ModalOpen() || M->World->bBusy;
 if (!bHeld) switch (P)
 {
 case EPhase::Wait:
  // Wagon 23 comes while Reyes is in view of the night window, never while away from the stop.
  if (AtFuelStop(16)) WaitT += Dt;
  if (WaitT >= WaitFor) { SetPhase(EPhase::Arrival); ArriveT = 0; }
  break;
 case EPhase::Arrival:
  ArriveT += Dt;
  if (ArriveT >= DriveIn) SetPhase(EPhase::Meet);
  break;
 case EPhase::Lift:
  if (!F.bIdentified) break;
  LiftDelay += Dt;
  if (LiftDelay < LiftAfter) break;
  LiftT += Dt;
  LightK = FMath::Min(1., LightK + Dt * 2.5);
  W->Lift = FMath::SmoothStep(0., 1., FMath::Clamp(LiftT / Rise, 0., 1.)) * 2.4;
  FigureK = FMath::Min(1., FigureK + Dt);
  // She hangs in the light until Reyes has actually seen it; the camera is never turned.
  if (Sees(OrtizPoint(), .85)) LookT += Dt;
  if (LiftT >= Rise) HeldT += Dt;
  if (LookT >= SeeFor && HeldT >= Hang)
  {
   F.bObserved = true; bOrtizGone = true; ++Version; F.TakenStamp = M->Story ? M->Story->ClockText(true) : FString();
   if (M->Audio) M->Audio->Gasp();
   SetPhase(EPhase::Retreat); Gone = 0;
   M->Story->Note(TEXT("Caliche fuel stop: a light struck wagon twenty-three. Mara Ortiz rose clear of the ground with nothing under her and was gone. Pale figures stood still at the edge of the light."));
  }
  break;
 case EPhase::Retreat:
  Gone += Dt;
  LightK = FMath::Max(0., 1 - Gone / FadeLight);
  TurnK = FMath::Min(1., TurnK + Dt / 1.5);
  if (Gone > 1.0 && PlayerCovered())
  {
   F.bRetreat = true; ++Version; FigureK = 0; SetPhase(EPhase::Calm);
   UE_LOG(LogS47HorrorSC03, Display, TEXT("S47: SC03 safe retreat %s"), M->World->bDriving ? TEXT("in SARO 07") : TEXT("behind solid cover"));
  }
  break;
 case EPhase::Deputy:
  if (!F.bDeputyHere)
  {
   DeputyT += Dt;
   if (DeputyT >= DeputyWalk) { F.bDeputyHere = true; ++Version; UE_LOG(LogS47HorrorSC03, Display, TEXT("S47: SC03 deputy at the fuel stop")); }
  }
  break;
 default: break;
 }
 Apply();
}

bool FS47HorrorSC03::Use(const FString& Id)
{
 if (!Live()) return false;
 if (Id == TEXT("horror:ortiz") && P == EPhase::Meet && !F.bIdentified) { F.bIdentified = true; SetPhase(EPhase::Lift); LiftDelay = LiftT = LookT = HeldT = 0; return true; }
 if (P != EPhase::Calm && P != EPhase::Deputy) return false;
 if (Id == TEXT("horror:wagon23") && !F.bWagon) { F.bWagon = true; ++Version; M->UI->Toast(TEXT("The driver's door is open. Her clipboard on the seat, the work order for SARO on top. The keys are still in it."), 5); return true; }
 if (Id == TEXT("horror:thermos") && !F.bThermos) { F.bThermos = true; ++Version; M->UI->Toast(TEXT("A steel thermos against the kerb, still warm. Tape on it: M. ORTIZ. You leave it where it lies."), 5); return true; }
 if (Id == TEXT("horror:gravelMarks") && !F.bMarks) { F.bMarks = true; ++Version; M->UI->Toast(TEXT("Narrow pressed marks beside the wagon. No footprints lead away from them."), 5); return true; }
 if (Id == TEXT("horror:deputy") && P == EPhase::Deputy && F.bDeputyHere) return true; // FS47Horror presents the question
 return false;
}

TArray<FString> FS47HorrorSC03::CallScenes() const
{
 if (P == EPhase::Report && !F.bDispatch) return {TEXT("SC03_DISPATCH_RECEIPT"), TEXT("SC03_RELIEF_CONTACT")};
 if (P == EPhase::Calm && !F.bMissing) return {TEXT("SC03_MISSING_RECEIPT"), TEXT("SC03_LOCAL_ORDER_HOLD")};
 return {};
}

void FS47HorrorSC03::CallFinished(const FString& SceneId)
{
 const FString Now = M && M->Story ? M->Story->ClockText(true) : FString(); ++Version;
 if (SceneId == TEXT("SC03_DISPATCH_RECEIPT")) { F.bDispatch = F.bAssigned = true; F.DispatchStamp = Now; }
 else if (SceneId == TEXT("SC03_RELIEF_CONTACT") && F.bDispatch) { SetPhase(EPhase::Wait); WaitT = 0; }
 else if (SceneId == TEXT("SC03_MISSING_RECEIPT") && F.bRetreat) { F.bMissing = true; F.MissingStamp = Now; }
 else if (SceneId == TEXT("SC03_LOCAL_ORDER_HOLD") && F.bMissing) { F.bLocalOrder = true; F.OrderStamp = Now; SetPhase(EPhase::Deputy); DeputyT = 0; }
}

void FS47HorrorSC03::ScenePresented(const FString& SceneId)
{
 if (SceneId == TEXT("SC03_RECEIVER_QUESTION") && P == EPhase::Deputy)
 {
  SetPhase(EPhase::Complete);
  if (M && M->Story && M->Story->bEveningTimeline) M->Story->AdvanceClockTo(2 * 3600 + 44 * 60, TEXT("SC03 deputy report complete"));
  Apply();
 }
}

FString FS47HorrorSC03::Objective() const
{
 switch (P)
 {
 case EPhase::Report: return TEXT("CALICHE // REPORT THE ENCOUNTER FROM THE PAYPHONE AT THE FUEL STOP");
 case EPhase::Wait: return TEXT("FUEL STOP // WAIT IN VIEW OF THE NIGHT WINDOW FOR WAGON TWENTY-THREE");
 case EPhase::Arrival: return TEXT("FUEL STOP // A WAGON IS PULLING IN");
 case EPhase::Meet: return TEXT("FUEL STOP // MEET MARA ORTIZ BY HER WAGON");
 case EPhase::Lift: return F.bIdentified ? TEXT("FUEL STOP // TELL ORTIZ WHAT YOU SAW") : TEXT("FUEL STOP // MEET MARA ORTIZ BY HER WAGON");
 case EPhase::Retreat: return TEXT("FUEL STOP // GET BEHIND SOLID COVER: INSIDE THE STATION OR IN YOUR TRUCK");
 case EPhase::Calm: return F.bMissing ? TEXT("FUEL STOP // STAY WITH THE PHONE") : TEXT("FUEL STOP // REPORT ORTIZ MISSING FROM THE PAYPHONE. HER THINGS STAY WHERE THEY LIE");
 case EPhase::Deputy: return F.bDeputyHere ? TEXT("FUEL STOP // TALK TO THE DEPUTY") : TEXT("FUEL STOP // THE DEPUTY IS COMING FROM THE SUBSTATION OFFICE");
 default: return FString();
 }
}

TArray<TPair<FString, bool>> FS47HorrorSC03::Tasks() const
{
 return {{TEXT("Report the encounter and get a callback"), F.bDispatch},
  {TEXT("Meet the relief technician at the fuel stop"), F.bIdentified},
  {TEXT("Get behind solid cover"), F.bRetreat},
  {TEXT("Report Ortiz missing"), F.bMissing},
  {TEXT("Optional: look at the wagon, the thermos and the marks"), TracesChecked()},
  {TEXT("Answer the deputy"), P == EPhase::Complete}};
}

bool FS47HorrorSC03::SelfTest(TArray<FString>& Lines)
{
 bool Good = true;
 auto Check = [&](bool V, const TCHAR* What) { Good &= V; Lines.Add(FString::Printf(TEXT("SC03 %s %s"), V ? TEXT("PASS") : TEXT("FAIL"), What)); };
 FFacts A; A.bDispatch = A.bAssigned = A.bIdentified = A.bObserved = A.bRetreat = A.bThermos = true; A.TakenStamp = TEXT("13 APR 1986 / 20:58:10");
 EPhase P0; FFacts B;
 Check(Read(Write(EPhase::Calm, A), P0, B) && P0 == EPhase::Calm && B.bRetreat && B.bThermos && !B.bWagon && B.TakenStamp == A.TakenStamp, TEXT("save round trip keeps phase, facts and stamps"));
 FFacts Bad = A; Bad.bObserved = false;
 Check(!Read(Write(EPhase::Calm, Bad), P0, B) && P0 == EPhase::Off && !B.bRetreat, TEXT("a retreat without an observed lift is rejected, not invented"));
 auto Str = Write(EPhase::Report, FFacts()); S47J::Set(Str, TEXT("dispatch"), TEXT("true"));
 Check(!Read(Str, P0, B) && !B.bDispatch, TEXT("a string is never a fact"));
 auto Future = Write(EPhase::Report, FFacts()); S47J::Set(Future, TEXT("v"), 2);
 Check(!Read(Future, P0, B), TEXT("unknown save version rejected"));
 Check(Read(nullptr, P0, B) && P0 == EPhase::Off, TEXT("absent field is a case before SC03"));
 FS47HorrorSC03 S(nullptr, nullptr); S.P = EPhase::Report;
 Check(S.CallScenes().Num() == 2 && S.CallScenes()[0] == TEXT("SC03_DISPATCH_RECEIPT"), TEXT("payphone in report places dispatch then Ward"));
 S.CallFinished(TEXT("SC03_RELIEF_CONTACT")); Check(S.P == EPhase::Report, TEXT("Ward's contact without the dispatch readback does not start the wait"));
 S.CallFinished(TEXT("SC03_DISPATCH_RECEIPT")); S.CallFinished(TEXT("SC03_RELIEF_CONTACT"));
 Check(S.F.bDispatch && S.F.bAssigned && S.P == EPhase::Wait, TEXT("completed dispatch and Ward calls start the wait"));
 S.P = EPhase::Calm; Check(S.CallScenes().Num() == 2 && S.CallScenes()[0] == TEXT("SC03_MISSING_RECEIPT"), TEXT("after cover the payphone places the missing report"));
 S.CallFinished(TEXT("SC03_MISSING_RECEIPT")); Check(!S.F.bMissing, TEXT("no missing report without a real retreat"));
 S.F.bIdentified = S.F.bObserved = S.F.bRetreat = true; S.CallFinished(TEXT("SC03_MISSING_RECEIPT")); S.CallFinished(TEXT("SC03_LOCAL_ORDER_HOLD"));
 Check(S.F.bMissing && S.F.bLocalOrder && S.P == EPhase::Deputy, TEXT("missing report and Ward's hold bring the deputy"));
 S.ScenePresented(TEXT("SC03_RECEIVER_QUESTION")); Check(S.P == EPhase::Complete && !S.Live(), TEXT("the deputy's question completes SC03"));
 return Good;
}
