#include "S47SaroRooms.h"

#include "S47Main.h"
#include "S47Sky.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47WorldLife.h"
#include "Components/RectLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SaroRooms, Log, All);

namespace
{
 constexpr double FixtureX[] = {-3.7, .2};
 constexpr float FixtureLumens = 32.f;
 const TCHAR* FinishPackage = TEXT("/Game/Signal47/Art/SaroInteriors/SM_Saro_CorridorFinishes");
 const TCHAR* ControlPackage = TEXT("/Game/Signal47/Art/SaroInteriors/SM_Saro_ControlRoomFinishes");
 constexpr double WindowX[] = {-4.5, -2.7, -.9, .9, 2.7, 4.5};
 constexpr double ControlFixtureXZ[][2] = {{-3.5, -1.8}, {0, -1.8}, {0, 1.8}, {3.5, 1.8}};
 constexpr float WindowLumens = 40.f, ControlFixtureLumens = 36.f;
}

struct FS47SaroRooms::FState
{
 struct FBounce
 {
  TWeakObjectPtr<URectLightComponent> Driver, Surface;
 };
 TWeakObjectPtr<AS47Web> Web;
 TWeakObjectPtr<UStaticMeshComponent> Finish;
 int32 FinishNode = -1;
 FBounce Bounces[UE_ARRAY_COUNT(FixtureX)];
 TWeakObjectPtr<UStaticMeshComponent> ControlFinish;
 int32 ControlFinishNode = -1;
 TWeakObjectPtr<URectLightComponent> WindowSky[UE_ARRAY_COUNT(WindowX)];
 FBounce ControlBounces[UE_ARRAY_COUNT(ControlFixtureXZ)], WindowBounces[UE_ARRAY_COUNT(WindowX)];
 double RetryWait = 0;
 bool bMissingReported = false, bControlMissingReported = false;

 void Reset()
 {
  if (auto* C = Finish.Get()) C->DestroyComponent();
  Finish.Reset(); FinishNode = -1;
  if (auto* C = ControlFinish.Get()) C->DestroyComponent();
  ControlFinish.Reset(); ControlFinishNode = -1;
  for (FBounce& B : Bounces)
  {
   if (auto* L = B.Surface.Get()) L->DestroyComponent();
   B = FBounce();
  }
  for (auto& Light : WindowSky) { if (auto* L = Light.Get()) L->DestroyComponent(); Light.Reset(); }
  auto ResetBounces = [](auto& Parts)
  { for (FBounce& B : Parts) { if (auto* L = B.Surface.Get()) L->DestroyComponent(); B = FBounce(); } };
  ResetBounces(ControlBounces); ResetBounces(WindowBounces);
  RetryWait = 0; bMissingReported = false; bControlMissingReported = false;
 }
};

FS47SaroRooms::FS47SaroRooms(US47Main* InM) : State(MakeUnique<FState>()), M(InM)
{
 bOff = FParse::Param(FCommandLine::Get(), TEXT("S47SaroRoomsOff"));
 bControlOff = FParse::Param(FCommandLine::Get(), TEXT("S47SaroControlRoomOff"));
}
FS47SaroRooms::~FS47SaroRooms() { State->Reset(); }

void FS47SaroRooms::Update(US47Main* Main, double Dt)
{
 static TMap<TWeakObjectPtr<US47Main>, TSharedPtr<FS47SaroRooms>> Rooms;
 for (auto It = Rooms.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
 if (!Main) return;
 TSharedPtr<FS47SaroRooms>& Room = Rooms.FindOrAdd(TWeakObjectPtr<US47Main>(Main));
 if (!Room) Room = MakeShared<FS47SaroRooms>(Main);
 Room->Step(Dt);
}

void FS47SaroRooms::TryBuild()
{
 TryBuildCorridor();
 if (!bControlOff) TryBuildControlRoom();
}

void FS47SaroRooms::TryBuildCorridor()
{
 AS47Web* W = M->Web.Get();
 const FS47Obj Annex = W->Obj(TEXT("annex.group"));
 const FS47WebNode* Parent = Annex ? W->Node(Annex.I) : nullptr;
 if (!Parent || !Parent->Comp.IsValid() || !W->NodeWorldThree(Annex.I).Equals(FTransform::Identity, .0001)) return;
 USceneComponent* Attach = Parent->Comp.Get();
 if (!State->Finish.IsValid())
 {
  // Native finish assets are optional on a public source clone. Until import is
  // ready, the existing corridor stays visible and keeps all original colliders.
  if (FPackageName::DoesPackageExist(FinishPackage))
  {
   const FString Path = FString(FinishPackage) + TEXT(".SM_Saro_CorridorFinishes");
   UStaticMesh* Asset = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
   if (Asset && !Asset->IsCompiling())
   {
    auto* C = NewObject<UStaticMeshComponent>(W, TEXT("SaroCorridorFinishes"));
    C->SetupAttachment(Attach); C->SetMobility(EComponentMobility::Movable); C->SetStaticMesh(Asset);
    C->SetCollisionProfileName(TEXT("NoCollision")); C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetCollisionResponseToAllChannels(ECR_Ignore); C->SetGenerateOverlapEvents(false);
    C->SetCanEverAffectNavigation(false); C->SetCastShadow(true);
    W->AddInstanceComponent(C); C->RegisterComponent(); C->SetWorldTransform(FTransform::Identity);
    State->Finish = C; State->FinishNode = W->AdoptNode(C, Annex.I, TEXT("saro:rooms:corridorFinishes"));
    UE_LOG(LogS47SaroRooms, Display, TEXT("S47: SARO native corridor finishes placed; original walk, doors, fixtures and vending retained"));
   }
  }
  else if (!State->bMissingReported)
  {
   UE_LOG(LogS47SaroRooms, Display, TEXT("S47: SARO corridor finishes unavailable; existing room retained"));
   State->bMissingReported = true;
  }
 }
 // Rehouse can complete after the first Step. Bind only the two unique actual
 // sources at its measured poses and dimensions, never another room's lamp.
 TArray<URectLightComponent*> Lights; W->GetComponents(Lights);
 for (int32 I = 0; I < UE_ARRAY_COUNT(FixtureX); ++I)
 {
  FState::FBounce& B = State->Bounces[I];
  if (B.Driver.IsValid() && B.Surface.IsValid()) continue;
  URectLightComponent* Driver = nullptr; int32 Matches = 0;
  const FVector At = S47Three::ToUE(FVector(FixtureX[I], 2.696, 5.7));
  for (URectLightComponent* L : Lights)
   if (L && L->GetComponentLocation().Equals(At, .02) && FMath::IsNearlyEqual(L->SourceWidth, 124.f, .01f) && FMath::IsNearlyEqual(L->SourceHeight, 30.f, .01f))
   { Driver = L; ++Matches; }
  if (Matches != 1) continue;
  B.Driver = Driver;
  if (!B.Surface.IsValid())
  {
   auto* L = NewObject<URectLightComponent>(W, I == 0 ? TEXT("SaroCorridorBounceWest") : TEXT("SaroCorridorBounceEast"));
   L->SetupAttachment(Attach); L->SetMobility(EComponentMobility::Movable);
   L->SetWorldLocation(S47Three::ToUE(FVector(FixtureX[I], .075, 5.7)));
   L->SetWorldRotation(FRotationMatrix::MakeFromXY(FVector::UpVector, FVector::ForwardVector).Rotator());
   L->SetSourceWidth(160); L->SetSourceHeight(110); L->SetIntensityUnits(ELightUnits::Lumens); L->SetIntensity(0);
   L->SetAttenuationRadius(350); L->SetCastShadows(true); L->SetVolumetricScatteringIntensity(0);
   L->SetVisibility(false); W->AddInstanceComponent(L); L->RegisterComponent(); B.Surface = L;
   UE_LOG(LogS47SaroRooms, Display, TEXT("S47: SARO corridor Low reflection %d follows existing fixture; maximum %.2f lm"), I, FixtureLumens * .05f);
  }
 }
}

void FS47SaroRooms::TryBuildControlRoom()
{
 AS47Web* W = M->Web.Get();
 const FS47Obj Room = W->Obj(TEXT("room.group"));
 const FS47WebNode* Parent = Room ? W->Node(Room.I) : nullptr;
 if (!Parent || !Parent->Comp.IsValid() || !W->NodeWorldThree(Room.I).Equals(FTransform::Identity, .0001)) return;
 USceneComponent* Attach = Parent->Comp.Get();
 if (!State->ControlFinish.IsValid())
 {
  if (FPackageName::DoesPackageExist(ControlPackage))
  {
   const FString Path = FString(ControlPackage) + TEXT(".SM_Saro_ControlRoomFinishes");
   UStaticMesh* Asset = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
   if (Asset && !Asset->IsCompiling())
   {
    auto* C = NewObject<UStaticMeshComponent>(W, TEXT("SaroControlRoomFinishes"));
    C->SetupAttachment(Attach); C->SetMobility(EComponentMobility::Movable); C->SetStaticMesh(Asset);
    C->SetCollisionProfileName(TEXT("NoCollision")); C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetCollisionResponseToAllChannels(ECR_Ignore); C->SetGenerateOverlapEvents(false);
    C->SetCanEverAffectNavigation(false); C->SetCastShadow(true);
    W->AddInstanceComponent(C); C->RegisterComponent(); C->SetWorldTransform(FTransform::Identity);
    State->ControlFinish = C; State->ControlFinishNode = W->AdoptNode(C, Room.I, TEXT("saro:rooms:controlFinishes"));
    UE_LOG(LogS47SaroRooms, Display, TEXT("S47: SARO native control-room finishes placed; consoles, windows, doors and original lamp drivers retained"));
   }
  }
  else if (!State->bControlMissingReported)
  {
   UE_LOG(LogS47SaroRooms, Display, TEXT("S47: SARO control-room finishes unavailable; existing room retained"));
   State->bControlMissingReported = true;
  }
 }
 auto FloorReturn = [W, Attach](const FString& Name, const FVector& At, float Width, float Depth)
 {
  auto* L = NewObject<URectLightComponent>(W, *Name);
  L->SetupAttachment(Attach); L->SetMobility(EComponentMobility::Movable);
  L->SetWorldLocation(S47Three::ToUE(At));
  L->SetWorldRotation(FRotationMatrix::MakeFromXY(FVector::UpVector, FVector::ForwardVector).Rotator());
  L->SetSourceWidth(Width); L->SetSourceHeight(Depth); L->SetIntensityUnits(ELightUnits::Lumens); L->SetIntensity(0);
  L->SetAttenuationRadius(500); L->SetCastShadows(true); L->SetVolumetricScatteringIntensity(0);
  L->SetVisibility(false); W->AddInstanceComponent(L); L->RegisterComponent(); return L;
 };
 // S47WindowLight's two original SARO panes face OUT and carry electrical spill.
 // These six panes face IN, behind the original mullions/blinds, and carry only
 // the exterior sky. The vertical plane includes the rear ceiling without a seam.
 for (int32 I = 0; I < UE_ARRAY_COUNT(WindowX); ++I)
 {
  if (!State->WindowSky[I].IsValid())
  {
   auto* L = NewObject<URectLightComponent>(W, *FString::Printf(TEXT("SaroControlWindowSky%d"), I));
   L->SetupAttachment(Attach); L->SetMobility(EComponentMobility::Movable);
   L->SetWorldLocation(S47Three::ToUE(FVector(WindowX[I], 1.85, -4.535))); L->SetWorldRotation(FRotator(0, 90, 0));
   L->SetSourceWidth(168); L->SetSourceHeight(164); L->SetIntensityUnits(ELightUnits::Lumens); L->SetIntensity(0);
   L->SetAttenuationRadius(850); L->SetCastShadows(true); L->SetVolumetricScatteringIntensity(0);
   L->SetVisibility(false); W->AddInstanceComponent(L); L->RegisterComponent(); State->WindowSky[I] = L;
   UE_LOG(LogS47SaroRooms, Display, TEXT("S47: SARO control sky pane %d fitted to original window; daylight only, existing outward spill retained"), I);
  }
  FState::FBounce& B = State->WindowBounces[I]; B.Driver = State->WindowSky[I];
  if (!B.Surface.IsValid())
   B.Surface = FloorReturn(FString::Printf(TEXT("SaroControlWindowReturn%d"), I), FVector(WindowX[I], .075, -2.65), 155, 190);
 }
 TArray<URectLightComponent*> Lights; W->GetComponents(Lights);
 for (int32 I = 0; I < UE_ARRAY_COUNT(ControlFixtureXZ); ++I)
 {
  FState::FBounce& B = State->ControlBounces[I];
  if (B.Driver.IsValid() && B.Surface.IsValid()) continue;
  URectLightComponent* Driver = nullptr; int32 Matches = 0;
  const FVector At = S47Three::ToUE(FVector(ControlFixtureXZ[I][0], 2.910, ControlFixtureXZ[I][1]));
  for (URectLightComponent* L : Lights)
   if (L && L->GetComponentLocation().Equals(At, .02) && FMath::IsNearlyEqual(L->SourceWidth, 120.f, .01f) && FMath::IsNearlyEqual(L->SourceHeight, 26.f, .01f))
   { Driver = L; ++Matches; }
  if (Matches != 1) continue;
  B.Driver = Driver;
  if (!B.Surface.IsValid())
  {
   B.Surface = FloorReturn(FString::Printf(TEXT("SaroControlFixtureReturn%d"), I), FVector(ControlFixtureXZ[I][0], .075, ControlFixtureXZ[I][1]), 180, 170);
   UE_LOG(LogS47SaroRooms, Display, TEXT("S47: SARO control Low reflection %d follows original 36lm tube; maximum %.2f lm"), I, ControlFixtureLumens * .05f);
  }
 }
}

void FS47SaroRooms::Step(double Dt)
{
 if (bOff || !M || !M->Web || !M->Web->IsLoaded()) return;
 AS47Web* W = M->Web.Get();
 if (State->Web.Get() != W) { State->Reset(); State->Web = W; }
 State->RetryWait -= FMath::Max(0., Dt);
 if (State->RetryWait <= 0) { TryBuild(); State->RetryWait = 1.; }
 const FS47Obj AnnexInterior = W->Obj(TEXT("annex.interior"));
 const bool Visible = M->Mode == US47Main::EMode::Play && M->World && M->World->Area == TEXT("saro") && AnnexInterior.Shown();
 if (State->FinishNode >= 0) W->ObjAt(State->FinishNode).SetVisible(Visible);
 const bool Low = W->GetGraphicsSettings().IndirectLighting == 0;
 auto Reflect = [Low](auto& Parts, bool Shown, float Limit, const FLinearColor& Tint)
 {
  for (FState::FBounce& B : Parts) if (auto* L = B.Surface.Get())
  {
   const URectLightComponent* Driver = B.Driver.Get();
   const bool On = Shown && Low && Driver && Driver->IsVisible() && !Driver->bHiddenInGame;
   // Read the finished fixture state after WorldLife's driver update. The tiny
   // floor reflection preserves dimming, flicker and complete blackout exactly.
   L->SetIntensity(On ? FMath::Clamp(Driver->Intensity, 0.f, Limit) * .05f : 0.f);
   if (Driver) L->SetLightColor(Driver->GetLightColor() * Tint);
   L->SetVisibility(On && Driver->Intensity > .0001f);
  }
 };
 Reflect(State->Bounces, Visible, FixtureLumens, FLinearColor(.79f, .82f, .75f));
 const FS47Obj Room = W->Obj(TEXT("room.group"));
 const bool ControlVisible = !bControlOff && M->Mode == US47Main::EMode::Play && M->World && M->World->Area == TEXT("saro") && Room.Shown();
 if (State->ControlFinishNode >= 0) W->ObjAt(State->ControlFinishNode).SetVisible(ControlVisible);
 const FS47Sky* Sky = M->Life && M->Life->Sky.IsValid() ? M->Life->Sky.Get() : nullptr;
 const float Daylight = Sky ? float(FMath::Clamp(Sky->Daylight(), 0., 1.)) : 0.f;
 const FLinearColor SkyTint = FMath::Lerp(FLinearColor(.90f, .95f, 1), FLinearColor(1, .87f, .72f), Sky ? float(Sky->Evening) : 0.f);
 for (auto& Light : State->WindowSky) if (auto* L = Light.Get())
 {
  L->SetIntensity(ControlVisible ? WindowLumens * Daylight : 0.f); L->SetLightColor(SkyTint);
  L->SetVisibility(ControlVisible && Daylight > .001f);
 }
 Reflect(State->ControlBounces, ControlVisible, ControlFixtureLumens, FLinearColor(.88f, .77f, .61f));
 Reflect(State->WindowBounces, ControlVisible, WindowLumens, FLinearColor(.88f, .77f, .61f));
}
