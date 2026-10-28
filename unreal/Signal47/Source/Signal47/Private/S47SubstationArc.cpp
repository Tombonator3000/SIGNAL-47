#include "S47Substation.h"
#include "S47Flow.h"
#include "S47Fx.h"
#include "S47Horror.h"
#include "S47HorrorThreat.h"
#include "S47Main.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47Story.h"
#include "S47World.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstanceController.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SubstationArc, Log, All);

// The feeder outage has a quiet sustained arc and fixed fault beacons until T01 restores it.
// SC02 keeps its separate short strike with sparks when the pale body first appears at the post.
// Runtime only: Story owns the saved outage, and Horror/T01 own the strike's existing gate.
namespace
{
 const TCHAR* ArcSystem = TEXT("/Game/Signal47/FX/NS_S47_ArcSparks.NS_S47_ArcSparks");
 const double InsulatorX[3] = {-1.04, 0, 1.04}; // Tools/substation/geometry.py: bus supports at (x, 3.25, 7.69)
 constexpr double InsulatorY = 3.25, InsulatorZ = 7.69, GravelTop = -.08;
 constexpr float ArcPeakLumens = 1800.f, CalmPeak = .45f;
 constexpr float FaultLampLumens = 120.f;
 constexpr int32 MaxSparks = 40; // Tools/fx/fx_contract.json maxSparks
 const FLinearColor ArcColor(.36f, .64f, 1.f), SparkColor(1.f, .74f, .42f, 1.f);
 const FLinearColor FaultLampColor(1.f, .24f, .035f);
 FVector Insulator(int32 I) { return FS47Substation::Site(FVector(InsulatorX[FMath::Clamp(I, 0, 2)], InsulatorY, InsulatorZ)); }
 float FaultArcLevel(bool bLow, bool bCalm) { return bCalm ? .065f : bLow ? .08f : .10f; }
 bool SafeMesh(const UStaticMeshComponent* C)
 {
  return !C || (!C->bVisibleInRayTracing && !C->CastShadow && !C->bAffectDistanceFieldLighting &&
   !C->bAffectDynamicIndirectLighting && !C->CanEverAffectNavigation() && C->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
 }
 bool SafeLight(const UPointLightComponent* L) { return !L || (!L->CastShadows && L->IndirectLightingIntensity == 0.f); }
 // Gravel top under the portal beam, relative to the insulator: the sparks' analytic bounce plane (cm, UE Z).
 double GroundZ() { return (FS47Substation::Site(FVector(0, InsulatorY, GravelTop)).Y - Insulator(1).Y) * 100.0; }
 int32 Nearest(const FVector2D& Body)
 {
  int32 Best = 1; double D = 1e18;
  for (int32 I = 0; I < 3; ++I) { const FVector P = Insulator(I); const double E = FVector2D::Distance(FVector2D(P.X, P.Z), Body); if (E < D) { D = E; Best = I; } }
  return Best;
 }
}

bool FS47Substation::ArcAllowed(const FArcGate& G)
{
 // Once per meeting, on the frame the body first appears unseen. T01 (no result yet or the check
 // running) and the later T02 cycle never show it; a witnessed forewarning or a spent arc never repeat it.
 return G.bMeeting && G.bThreatActive && G.bAppeared && !G.bWasAppeared && !G.bSeen && G.bNormalResult && !G.bChecking && !G.bFollowup && !G.bSpent;
}

float FS47Substation::ArcLevel(double T, bool bCalm)
{
 if (!(T >= 0) || T >= ArcDuration) return 0.f;
 // Calm: one soft rise and fall, no flicker.
 if (bCalm) return CalmPeak * float(T < .25 ? FMath::SmoothStep(0.0, .25, T) : 1.0 - FMath::SmoothStep(.25, ArcDuration, T));
 // Three authored strikes in the short event, then a dying glow. Calm uses no strikes.
 auto Strike = [T](double At, double W) { return T >= At ? FMath::Exp(-(T - At) / W) : 0.0; };
 return float(FMath::Clamp(FMath::Max3(Strike(0, .045), .7 * Strike(.09, .035), .85 * Strike(.21, .06)) + .12 * (1 - T / ArcDuration), 0.0, 1.0));
}

float FS47Substation::ArcDip(double T, bool bCalm)
{
 // The marked lamp sags with the arc's draw; exactly 1 outside it, so RenderedResult stays exact.
 const float L = ArcLevel(T, bCalm);
 return L <= 0.f ? 1.f : 1.f - .8f * (bCalm ? L / CalmPeak : L);
}

int32 FS47Substation::ArcSparks(bool bLow, bool bCalm) { return FMath::Clamp(FMath::RoundToInt(MaxSparks * (bLow ? .5 : 1.0) * (bCalm ? .5 : 1.0)), 1, MaxSparks); }

float FS47Substation::ArcLumens() const { return ArcReady() ? ArcPeakLumens * ArcLevel(ArcT, bArcCalm) : 0.f; }

bool FS47Substation::ArcReady() const
{
 return !S47Fx::Off() && Sparks.IsValid() && ArcLight.IsValid() && ArcMesh.IsValid() && ArcMid.IsValid();
}

bool FS47Substation::ArcSafe() const
{
 return (!Sparks.IsValid() || S47Fx::Safe(Sparks.Get())) &&
  SafeMesh(ArcMesh.Get()) && SafeLight(ArcLight.Get()) && SafeMesh(FaultArcMesh.Get()) &&
  SafeMesh(FaultLampMesh.Get()) && SafeMesh(FaultLampMounts[0].Get()) && SafeMesh(FaultLampMounts[1].Get()) && SafeLight(FaultArcLight.Get()) &&
  SafeLight(FaultLampLights[0].Get()) && SafeLight(FaultLampLights[1].Get());
}

FS47Substation::FArcSnapshot FS47Substation::ArcSnapshot() const
{
 FArcSnapshot S; S.bReady = ArcReady(); S.bSafe = ArcSafe(); S.Age = ArcT; S.Bursts = ArcFired; S.SparkCount = ArcSparkCount;
 if (const UNiagaraComponent* C = Sparks.Get())
 {
  S.bNiagaraActive = C->IsActive(); S.bNiagaraPaused = C->IsPaused(); S.Origin = C->GetComponentLocation();
  if (const auto Controller = C->GetSystemInstanceController(); Controller && Controller->IsValid()) S.NiagaraAge = Controller->GetAge();
 }
 if (const UInstancedStaticMeshComponent* C = ArcMesh.Get())
 {
  S.Instances = C->GetInstanceCount(); S.bMeshVisible = C->IsVisible(); S.bMaterialAssigned = C->GetMaterial(0) == ArcMid.Get();
 }
 if (UMaterialInstanceDynamic* D = ArcMid.Get())
 {
  S.MaterialIntensity = D->K2_GetScalarParameterValue(TEXT("ArcIntensity")); S.MaterialColor = D->K2_GetVectorParameterValue(TEXT("ArcColor"));
 }
 if (const UPointLightComponent* L = ArcLight.Get()) { S.FlashLumens = L->Intensity; S.bFlashVisible = L->IsVisible(); }
 if (const UPointLightComponent* L = Light.Get()) S.LampLumens = L->Intensity;
 if (!LampMids.IsEmpty()) if (UMaterialInstanceDynamic* D = LampMids[0].Get()) S.LampEmissive = D->K2_GetVectorParameterValue(TEXT("EmissiveFactor"));
 return S;
}

float FS47Substation::FaultLampLevel(double T, bool bCalm)
{
 // A slow fixed warning, not random electrical flicker. Calm flash holds one dim level.
 if (bCalm) return .35f;
 if (!FMath::IsFinite(T) || T < 0) return .10f;
 const double Phase = FMath::Fmod(T, 4.0);
 const double Pulse = Phase < .8 ? FMath::SmoothStep(0.0, .8, Phase) :
  Phase < 1.4 ? 1.0 : 1.0 - FMath::SmoothStep(1.4, 2.2, Phase);
 return .10f + .90f * float(Pulse);
}

bool FS47Substation::FaultReady() const
{
 return ArcReady() && FaultArcMesh.IsValid() && FaultLampMesh.IsValid() && FaultLampMounts[0].IsValid() && FaultLampMounts[1].IsValid() &&
  FaultArcMid.IsValid() && FaultLampMid.IsValid() && FaultArcLight.IsValid() &&
  FaultLampLights[0].IsValid() && FaultLampLights[1].IsValid();
}

FS47Substation::FFaultSnapshot FS47Substation::FaultSnapshot() const
{
 FFaultSnapshot S; S.bRequested = M && M->Story && M->Story->FeederOutage();
 S.bReady = FaultReady(); S.bActive = bFaultShown; S.bSafe = ArcSafe(); S.Age = FaultTime;
 if (const UInstancedStaticMeshComponent* C = FaultArcMesh.Get())
 { S.bArcVisible = C->IsVisible(); S.ArcInstances = C->GetInstanceCount(); S.Origin = C->GetComponentLocation(); }
 if (const UInstancedStaticMeshComponent* C = FaultLampMesh.Get())
 { S.bLampsVisible = C->IsVisible(); S.LampInstances = C->GetInstanceCount(); }
 S.bMaterialAssigned = FaultArcMesh.IsValid() && FaultLampMesh.IsValid() &&
  FaultArcMesh->GetMaterial(0) == FaultArcMid.Get() && FaultLampMesh->GetMaterial(0) == FaultLampMid.Get();
 if (UMaterialInstanceDynamic* D = FaultArcMid.Get()) S.ArcIntensity = D->K2_GetScalarParameterValue(TEXT("ArcIntensity"));
 if (UMaterialInstanceDynamic* D = FaultLampMid.Get()) S.LampIntensity = D->K2_GetScalarParameterValue(TEXT("ArcIntensity"));
 if (const UPointLightComponent* L = FaultArcLight.Get()) S.ArcLumens = L->Intensity;
 for (const auto& W : FaultLampLights) if (const UPointLightComponent* L = W.Get()) S.LampLumens += L->Intensity;
 return S;
}

void FS47Substation::InstallFault(USceneComponent* Parent, UStaticMesh* Cylinder, UMaterialInterface* Material)
{
 UMaterialInterface* Steel = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Substation/Materials/M_Substation_DarkSteel.M_Substation_DarkSteel"));
 if (!Steel) { UE_LOG(LogS47SubstationArc, Warning, TEXT("S47: substation fault beacon mounts absent; sustained fault effect inert")); return; }
 auto Mid = [&](const FLinearColor& Color)
 {
  UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Material, M->Web.Get());
  D->SetVectorParameterValue(TEXT("ArcColor"), Color); D->SetScalarParameterValue(TEXT("ArcIntensity"), 0.f); return D;
 };
 FaultArcMid = Mid(ArcColor); FaultLampMid = Mid(FaultLampColor);
 auto Mesh = [&](const TCHAR* Name, UMaterialInterface* Mat)
 {
  UInstancedStaticMeshComponent* C = NewObject<UInstancedStaticMeshComponent>(M->Web.Get(), Name);
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(Parent); C->SetStaticMesh(Cylinder); C->SetMaterial(0, Mat);
  C->SetCastShadow(false); C->SetVisibleInRayTracing(false); C->bAffectDistanceFieldLighting = false; C->bAffectDynamicIndirectLighting = false;
  C->SetReceivesDecals(false); C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCanEverAffectNavigation(false);
  C->SetVisibility(false); C->RegisterComponent(); C->SetWorldTransform(FTransform::Identity); return C;
 };
 UInstancedStaticMeshComponent* Filament = Mesh(TEXT("SubstationFaultArc"), FaultArcMid.Get()); FaultArcMesh = Filament;
 const FVector At = S47Three::ToUE(Insulator(1)); Filament->SetWorldLocation(At);
 // One stable bow along the porcelain. No timed shape changes, branches, particles or strikes.
 FVector Previous = FVector::ZeroVector;
 for (int32 I = 1; I <= 12; ++I)
 {
  const double T = double(I) / 12, S = FMath::Sin(PI * T);
  const FVector Next(-32 * S, -25 * S, -75 * T), Delta = Next - Previous;
  Filament->AddInstance(FTransform(FQuat::FindBetweenNormals(FVector::UpVector, Delta.GetSafeNormal()),
   (Previous + Next) * .5, FVector(.7 / 50, .7 / 50, Delta.Size() / 100)));
  Previous = Next;
 }
 UInstancedStaticMeshComponent* Lenses = Mesh(TEXT("SubstationFaultBeacons"), FaultLampMid.Get()); FaultLampMesh = Lenses;
 auto Light = [&](const TCHAR* Name, const FVector& P, const FLinearColor& Color, float Radius, float Source)
 {
  UPointLightComponent* L = NewObject<UPointLightComponent>(M->Web.Get(), Name);
  L->SetMobility(EComponentMobility::Movable); L->SetupAttachment(Parent); L->SetIntensityUnits(ELightUnits::Lumens);
  L->SetIntensity(0); L->SetLightColor(Color); L->SetAttenuationRadius(Radius); L->SetSourceRadius(Source);
  L->SetCastShadows(false); L->SetIndirectLightingIntensity(0); L->SetVolumetricScatteringIntensity(0);
  L->SetVisibility(false); L->RegisterComponent(); L->SetWorldLocation(P); return L;
 };
 FaultArcLight = Light(TEXT("SubstationFaultArcLight"), At + FVector(-32, -40, -35), ArcColor, 1400, 8);
 // Both lenses are bolted to the front of the existing portal's outer steel legs, 6.43 m high.
 // Their fixed faces sit above the transformer for the approach and the enclosed service.
 const FQuat Face = FQuat::FindBetweenNormals(FVector::UpVector, FVector(0, -1, 0));
 for (int32 I = 0; I < 2; ++I)
 {
  const double X = I == 0 ? -3.45 : 3.45;
  const FVector P = S47Three::ToUE(Site(FVector(X, 3.05, 6.43)));
  // Use ordinary components for the original steel material, which has no instancing shader usage.
  UStaticMeshComponent* Mount = NewObject<UStaticMeshComponent>(M->Web.Get(), I == 0 ? TEXT("SubstationFaultMountLeft") : TEXT("SubstationFaultMountRight"));
  Mount->SetMobility(EComponentMobility::Movable); Mount->SetupAttachment(Parent); Mount->SetStaticMesh(Cylinder); Mount->SetMaterial(0, Steel);
  Mount->SetCastShadow(false); Mount->SetVisibleInRayTracing(false); Mount->bAffectDistanceFieldLighting = false; Mount->bAffectDynamicIndirectLighting = false;
  Mount->SetReceivesDecals(false); Mount->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mount->SetCanEverAffectNavigation(false);
  Mount->SetVisibility(false); Mount->RegisterComponent();
  Mount->SetWorldTransform(FTransform(Face, S47Three::ToUE(Site(FVector(X, 3.11, 6.43))), FVector(.26, .26, .08)));
  FaultLampMounts[I] = Mount; S47Fx::Precache(Mount);
  Lenses->AddInstance(FTransform(Face, P, FVector(.20, .20, .04)));
  FaultLampLights[I] = Light(I == 0 ? TEXT("SubstationFaultBeaconLeft") : TEXT("SubstationFaultBeaconRight"),
   P + FVector(0, -6, 0), FaultLampColor, 900, 10);
 }
 S47Fx::Precache(Filament); S47Fx::Precache(Lenses);
 UE_LOG(LogS47SubstationArc, Display, TEXT("S47: sustained feeder fault ready: stable 12-segment arc, two mounted amber beacons; safe=%d"), ArcSafe());
}

void FS47Substation::ClearFault()
{
 bFaultShown = false;
 for (const auto& W : {FaultArcMesh, FaultLampMesh}) if (UInstancedStaticMeshComponent* C = W.Get()) C->SetVisibility(false);
 for (const auto& W : FaultLampMounts) if (UStaticMeshComponent* C = W.Get()) C->SetVisibility(false);
 for (const auto& W : {FaultArcMid, FaultLampMid}) if (UMaterialInstanceDynamic* D = W.Get()) D->SetScalarParameterValue(TEXT("ArcIntensity"), 0.f);
 for (const auto& W : {FaultArcLight, FaultLampLights[0], FaultLampLights[1]})
  if (UPointLightComponent* L = W.Get()) { L->SetIntensity(0); L->SetVisibility(false); }
}

void FS47Substation::StepFault(double Dt)
{
 const bool bOutage = M && M->Story && M->Story->FeederOutage();
 const bool bLoading = !M || !M->World || M->World->bBusy || (M->Flow && M->Flow->bStarting);
 const bool bShow = bOutage && bReady && FaultReady() && M->Mode == US47Main::EMode::Play && Root.Shown() && !bLoading;
 if (!bShow) { ClearFault(); return; }
 const UWorld* World = M->Web ? M->Web->GetWorld() : nullptr;
 const bool bHeld = M->bPausedByMenu || M->ModalOpen() || M->Story->IsCinematic() || (World && World->IsPaused());
 if (!bHeld && FMath::IsFinite(Dt) && Dt > 0) FaultTime += FMath::Min(Dt, .1);
 const bool bCalm = M->Flow && M->Flow->bCalmFlash, bLow = M->Flow && M->Flow->Graphics.Effects == 0;
 const float Arc = FaultArcLevel(bLow, bCalm), LampLevel = FaultLampLevel(FaultTime, bCalm);
 FaultArcMesh->SetVisibility(true); FaultLampMesh->SetVisibility(true);
 for (const auto& W : FaultLampMounts) if (UStaticMeshComponent* C = W.Get()) C->SetVisibility(true);
 FaultArcMid->SetScalarParameterValue(TEXT("ArcIntensity"), 18.f * Arc);
 FaultLampMid->SetScalarParameterValue(TEXT("ArcIntensity"), 9.f * LampLevel);
 FaultArcLight->SetIntensity(ArcPeakLumens * Arc); FaultArcLight->SetVisibility(true);
 for (const auto& W : FaultLampLights) if (UPointLightComponent* L = W.Get()) { L->SetIntensity(FaultLampLumens * LampLevel); L->SetVisibility(true); }
 if (!bFaultShown) UE_LOG(LogS47SubstationArc, Display, TEXT("S47: sustained feeder fault shown until T01 restores supply; %s, arc %.0f lm, no sparks"), bCalm ? TEXT("calm steady beacons") : TEXT("slow 4 s beacons"), ArcPeakLumens * Arc);
 bFaultShown = true;
}

void FS47Substation::InstallArc()
{
 if (S47Fx::Off()) return;
 const FS47WebNode* N = M && M->Web ? M->Web->Node(Root.I) : nullptr;
 USceneComponent* Parent = N ? N->Comp.Get() : nullptr;
 if (!Parent) return;
 UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Signal47/FX/M_S47_FxArc.M_S47_FxArc"));
 if (!Cylinder || !Material) { UE_LOG(LogS47SubstationArc, Warning, TEXT("S47: substation arc material/mesh absent; entire effect inert")); return; }
 UNiagaraComponent* C = S47Fx::Make(M->Web.Get(), Parent, ArcSystem, TEXT("SubstationArcSparks"));
 if (!C) return; // A missing system disables geometry, light and marked-lamp dip together.
 const FVector At = S47Three::ToUE(Insulator(1));
 C->SetWorldLocation(At); C->SetWorldRotation(FRotator::ZeroRotator); C->SetWorldScale3D(FVector::OneVector); Sparks = C;
 // Desired age prevents particle drift while document panels pause the gameplay clock.
 C->SetAgeUpdateMode(ENiagaraAgeUpdateMode::DesiredAge); C->SetLockDesiredAgeDeltaTimeToSeekDelta(false); C->SetCanRenderWhileSeeking(true); C->SetDesiredAge(0);
 UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Material, M->Web.Get()); ArcMid = D;
 D->SetVectorParameterValue(TEXT("ArcColor"), ArcColor); D->SetScalarParameterValue(TEXT("ArcIntensity"), 0.f);
 UInstancedStaticMeshComponent* Mesh = NewObject<UInstancedStaticMeshComponent>(M->Web.Get(), TEXT("SubstationArcFilaments"));
 Mesh->SetMobility(EComponentMobility::Movable); Mesh->SetupAttachment(Parent); Mesh->SetStaticMesh(Cylinder); Mesh->SetMaterial(0, D);
 Mesh->SetCastShadow(false); Mesh->SetVisibleInRayTracing(false); Mesh->bAffectDistanceFieldLighting = false; Mesh->bAffectDynamicIndirectLighting = false;
 Mesh->SetReceivesDecals(false); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetCanEverAffectNavigation(false);
 Mesh->SetVisibility(false); Mesh->RegisterComponent(); Mesh->SetWorldTransform(FTransform(FQuat::Identity, At)); ArcMesh = Mesh;
 S47Fx::Precache(Mesh);
 UPointLightComponent* L = NewObject<UPointLightComponent>(M->Web.Get(), TEXT("SubstationArcLight"));
 L->SetMobility(EComponentMobility::Movable); L->SetupAttachment(Parent);
 L->SetIntensityUnits(ELightUnits::Lumens); L->SetIntensity(0); L->SetLightColor(ArcColor); L->SetAttenuationRadius(1800); L->SetSourceRadius(8);
 L->SetCastShadows(false); L->SetIndirectLightingIntensity(0); L->SetVolumetricScatteringIntensity(.5f); L->SetVisibility(false); L->RegisterComponent(); L->SetWorldLocation(At); ArcLight = L;
 InstallFault(Parent, Cylinder, Material);
 UE_LOG(LogS47SubstationArc, Display, TEXT("S47: substation arc ready: porcelain top 7.69 to mount 6.94 m, gravel %.0f cm, native filaments+Niagara, safe=%d"), GroundZ(), ArcSafe());
}

void FS47Substation::CancelArc()
{
 ArcT = -1; ArcShape = -1;
 if (UNiagaraComponent* C = Sparks.Get()) { C->SetPaused(false); C->DeactivateImmediate(); C->SetDesiredAge(0); }
 if (UPointLightComponent* L = ArcLight.Get()) { L->SetIntensity(0); L->SetVisibility(false); }
 if (UInstancedStaticMeshComponent* C = ArcMesh.Get()) C->SetVisibility(false);
 if (UMaterialInstanceDynamic* D = ArcMid.Get()) D->SetScalarParameterValue(TEXT("ArcIntensity"), 0.f);
}

void FS47Substation::ResetArc()
{
 CancelArc(); bArcSpent = false; bArcCalm = false; bArcWasAppeared = false;
 ClearFault(); FaultTime = 0;
 // Substation Begin precedes Horror Begin. Never inspect the previous case's Threat here.
 // Continue inside a meeting is deliberately quiet, including a pre-witness checkpoint.
 // A save before the meeting still gets its first appearance. No new saved effect field.
 if (M && M->Flow && M->Flow->bStarting)
 {
  const auto H = S47J::Obj(M->Flow->Carry, TEXT("horror"));
  const FString Phase = S47J::Str(S47J::Obj(H, TEXT("sc02")), TEXT("phase"));
  bArcSpent = Phase == TEXT("encounter") || Phase == TEXT("escaped") || Phase == TEXT("unwitnessed");
 }
}

void FS47Substation::ShapeArc(int32 Shape)
{
 UInstancedStaticMeshComponent* C = ArcMesh.Get(); if (!C || ArcShape == Shape) return;
 ArcShape = Shape; C->ClearInstances();
 // Engine cylinder is 100 cm high, radius 50 cm. Positions here are native local cm.
 auto Segment = [C](const FVector& A, const FVector& B, double Radius)
 {
  const FVector Delta = B - A;
  C->AddInstance(FTransform(FQuat::FindBetweenNormals(FVector::UpVector, Delta.GetSafeNormal()), (A + B) * .5, FVector(Radius / 50, Radius / 50, Delta.Size() / 100)));
 };
 FRandomStream R(7919 + ArcSite * 101 + Shape * 37);
 TArray<FVector> Points; constexpr int32 Count = 12;
 for (int32 I = 0; I <= Count; ++I)
 {
  const double T = double(I) / Count, S = FMath::Sin(PI * T);
  // A lateral bow stays readable beside the porcelain from the service track;
  // a purely forward bow projected onto the brightly lit insulator in the live view.
  Points.Add(FVector(-32 * S + (I == 0 || I == Count ? 0 : R.FRandRange(-2.2f, 2.2f) * S),
   -25 * S + (I == 0 || I == Count ? 0 : R.FRandRange(-1.8f, 1.8f) * S), -75 * T));
  if (I) Segment(Points[I - 1], Points[I], .6);
 }
 for (int32 I : {4, 8})
 {
  const FVector A = Points[I], B = A + FVector(I == 4 ? 9 : -10, -6, -7), End = B + FVector(I == 4 ? 3 : -4, 2, -9);
  Segment(A, B, .22); Segment(B, End, .15);
 }
}

void FS47Substation::FireArc(int32 At, const TCHAR* Why)
{
 if (!ArcReady()) return;
 bArcSpent = true; ArcT = 0; ArcSite = At; ++ArcFired;
 bArcCalm = M && M->Flow && M->Flow->bCalmFlash;
 const bool bLow = M && M->Flow && M->Flow->Graphics.Effects == 0;
 const FVector P = S47Three::ToUE(Insulator(At));
 ArcSparkCount = ArcSparks(bLow, bArcCalm);
 ArcLight->SetWorldLocation(P + FVector(-32, -40, -35));
 ArcMesh->SetWorldTransform(FTransform(FQuat::Identity, P)); ArcShape = -1; ShapeArc(0);
 UNiagaraComponent* C = Sparks.Get();
 C->SetWorldLocation(P); C->SetWorldRotation(FRotator::ZeroRotator); C->SetWorldScale3D(FVector::OneVector);
 C->SetVariableInt(TEXT("User.SparkCount"), ArcSparkCount);
 C->SetVariableLinearColor(TEXT("User.SparkColor"), bArcCalm ? FLinearColor(SparkColor.R, SparkColor.G, SparkColor.B, .6f) : SparkColor);
 C->SetVariableVec3(TEXT("User.Direction"), FVector(0, -.45, -1));
 C->SetVariableFloat(TEXT("User.GroundZ"), float(GroundZ()));
 C->SetRandomSeedOffset(ArcFired * 7919 + At); C->SetPaused(false); C->SetDesiredAge(0); C->Activate(true);
 UE_LOG(LogS47SubstationArc, Display, TEXT("S47: substation arc %d at support %d (%s): sparks %d, %s"), ArcFired, At, Why, ArcSparkCount, bArcCalm ? TEXT("calm single pulse") : TEXT("three strikes"));
}

void FS47Substation::StepArc(double Dt)
{
 StepFault(Dt); // Independent of the SC02 meeting: visible before and during the T01 check.
 const FS47HorrorThreat* Th = M && M->Horror ? M->Horror->Threat.Get() : nullptr;
 const bool bMeeting = M && M->Horror && M->Horror->SC02Meeting();
 const bool bLoading = !M || !M->World || M->World->bBusy || (M->Flow && M->Flow->bStarting);
 const bool bContext = bReady && ArcReady() && M && M->Mode == US47Main::EMode::Play && Root.Shown() && bMeeting && !bLoading && Running <= 0 && AttemptId.IsEmpty();
 if (!bContext)
 {
  if (ArcT >= 0) CancelArc();
  bArcWasAppeared = Th && Th->Appeared();
  return;
 }
 const UWorld* World = M->Web ? M->Web->GetWorld() : nullptr;
 const bool bHeld = M->bPausedByMenu || M->ModalOpen() || !M->Story || M->Story->IsCinematic() || (World && World->IsPaused());
 if (UNiagaraComponent* C = Sparks.Get()) C->SetPaused(bHeld);
 if (bHeld) return; // Preserve both age and edge until the same gameplay frame can proceed.
 FArcGate G; G.bMeeting = bMeeting; G.bThreatActive = Th && Th->Active(); G.bAppeared = Th && Th->Appeared(); G.bWasAppeared = bArcWasAppeared; G.bSeen = Th && Th->Seen();
 G.bNormalResult = HasNormalResult(); G.bChecking = Running > 0; G.bFollowup = !AttemptId.IsEmpty(); G.bSpent = bArcSpent;
 bArcWasAppeared = G.bAppeared;
 if (Th && ArcAllowed(G)) FireArc(Nearest(Th->Position()), TEXT("SC02 pale body appeared unseen"));
 if (ArcT >= 0)
 {
  if (!G.bThreatActive || !G.bAppeared) { CancelArc(); return; } // Retry/withdrawal: spent survives.
  if (FMath::IsFinite(Dt) && Dt > 0) ArcT += FMath::Min(Dt, .1);
  if (ArcT >= ArcDuration + ArcTailDuration) { CancelArc(); return; }
  if (UNiagaraComponent* C = Sparks.Get()) C->SetDesiredAge(float(ArcT));
  if (ArcT < ArcDuration) ShapeArc(bArcCalm ? 0 : FMath::FloorToInt(ArcT / .065));
 }
 const float Level = ArcLevel(ArcT, bArcCalm);
 if (UPointLightComponent* L = ArcLight.Get()) { L->SetIntensity(ArcPeakLumens * Level); L->SetVisibility(Level > 0); }
 if (UInstancedStaticMeshComponent* C = ArcMesh.Get()) C->SetVisibility(Level > 0);
 if (UMaterialInstanceDynamic* D = ArcMid.Get()) D->SetScalarParameterValue(TEXT("ArcIntensity"), 18.f * Level);
}

bool FS47Substation::ArcSelfTest(TArray<FString>& Lines)
{
 bool Ok = true; auto Check = [&](bool P, const TCHAR* N) { Lines.Add(FString(P ? TEXT("ok  substation arc: ") : TEXT("FAIL substation arc: ")) + N); Ok &= P; };
 FArcGate Meet; Meet.bMeeting = Meet.bThreatActive = Meet.bAppeared = Meet.bNormalResult = true;
 Check(ArcAllowed(Meet), TEXT("fires when the SC02 body first appears unseen after the secured T01 result"));
 auto Without = [&Meet](TFunctionRef<void(FArcGate&)> F) { FArcGate G = Meet; F(G); return ArcAllowed(G); };
 Check(!Without([](FArcGate& G) { G.bNormalResult = false; }) && !Without([](FArcGate& G) { G.bChecking = true; }), TEXT("never during T01: no result yet, or the service check running"));
 Check(!Without([](FArcGate& G) { G.bFollowup = true; }), TEXT("never in the later T02 follow-up cycle"));
 Check(!Without([](FArcGate& G) { G.bSeen = true; }), TEXT("a witnessed forewarning (capture, Continue mid-meeting) never repeats it"));
 Check(!Without([](FArcGate& G) { G.bSpent = true; }), TEXT("once per meeting: a capture or yard retry re-appearance does not fire again"));
 Check(!Without([](FArcGate& G) { G.bWasAppeared = true; }) && !Without([](FArcGate& G) { G.bAppeared = false; }) && !Without([](FArcGate& G) { G.bThreatActive = false; }) && !Without([](FArcGate& G) { G.bMeeting = false; }),
  TEXT("only on the appearance edge of an active meeting (no legacy, no idle threat)"));
 auto Peaks = [](bool bCalm, float& Top)
 {
  int32 N = 0; float Prev = 0, Cur = ArcLevel(0, bCalm); Top = 0;
  for (int32 I = 1; I <= 1400; ++I) { const float Next = ArcLevel(I / 1000.0, bCalm); if (Cur > Prev && Cur >= Next && Cur > .05f) ++N; Top = FMath::Max(Top, Cur); Prev = Cur; Cur = Next; }
  return N;
 };
 float TopCalm = 0, TopNormal = 0;
 const int32 CalmPeaks = Peaks(true, TopCalm), NormalPeaks = Peaks(false, TopNormal);
 Check(CalmPeaks == 1 && TopCalm <= .5f, TEXT("calm flash: one soft pulse, no flicker, at most half the light"));
 Check(NormalPeaks >= 2 && NormalPeaks <= 3 && TopNormal >= .99f, TEXT("normal arc: exactly three authored strikes in the 1.2 s envelope"));
 Check(ArcLevel(-.01, false) == 0 && ArcLevel(ArcDuration, false) == 0 && ArcLevel(ArcDuration, true) == 0, TEXT("envelope is deterministic and zero outside its 1.2 s"));
 Check(ArcDip(-1, false) == 1.f && ArcDip(ArcDuration + 1, true) == 1.f && ArcDip(0, false) >= .19f && ArcDip(.6, true) < 1.f, TEXT("the lamp dips only during the arc and is exactly normal outside it"));
 Check(ArcSparks(false, false) == MaxSparks && ArcSparks(true, false) == MaxSparks / 2 && ArcSparks(true, true) < ArcSparks(true, false) && ArcSparks(false, true) <= MaxSparks / 2, TEXT("40 sparks, 20 on Low, fewer with calm flash"));
 Check(FaultArcLevel(false, false) > 0 && FaultArcLevel(false, false) <= .1f &&
  FaultArcLevel(true, false) > 0 && FaultArcLevel(true, false) <= FaultArcLevel(false, false) &&
  FaultArcLevel(true, true) <= FaultArcLevel(true, false), TEXT("sustained fault stays weak and visible on Low, further reduced by calm flash"));
 Check(FaultLampLevel(0, false) == FaultLampLevel(4, false) && FaultLampLevel(1, false) > .99f && FaultLampLevel(3, false) < .11f &&
  FaultLampLevel(.4, false) > FaultLampLevel(.1, false) && FaultLampLevel(1.8, false) > FaultLampLevel(2.1, false),
  TEXT("fixed fault beacons use a slow four-second rise, hold and fade, with no random flicker"));
 Check(FaultLampLevel(0, true) == FaultLampLevel(1, true) && FaultLampLevel(1, true) == FaultLampLevel(30, true) &&
  FaultLampLevel(0, true) > 0 && FaultLampLevel(0, true) < .5f,
  TEXT("calm flash holds the fixed beacons at a steady dim level"));
 Check(FMath::IsNearlyEqual(GroundZ(), -777.0, .5) && Nearest(FS47HorrorThreat::Nodes()[2].At) == 2 && FS47HorrorThreat::InYard(FVector2D(Insulator(0).X, Insulator(0).Z)) && FS47HorrorThreat::InYard(FVector2D(Insulator(2).X, Insulator(2).Z)),
  TEXT("insulators inside the closed yard, bounce plane on the gravel 7.77 m below, the post's nearest support picked"));
 return Ok;
}
