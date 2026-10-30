#include "S47TripoNpcs.h"
#include "S47TripoNpcsTest.h"

#include "S47Main.h"
#include "S47Herd.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47TripoNpcs, Log, All);

namespace
{
 const TCHAR* Roles[] = { TEXT("dale"), TEXT("patron"), TEXT("waitress"), TEXT("nora"),
  TEXT("attendant"), TEXT("deputy"), TEXT("earl"), TEXT("radio_woman"), TEXT("souvenir_man"),
  TEXT("mara_ortiz"), TEXT("hereford_25"), TEXT("shadow_man"), TEXT("tall_one"), TEXT("crawler") };

 bool Shown(const USceneComponent* C)
 {
  if (!C || !C->IsVisible() || !C->GetOwner() || C->GetOwner()->IsHidden()) return false;
  // Query-only boxes are hidden in game by default, even while their mesh
  // children are drawn. Observe the original drawable parts, not that shim.
  if (C->IsA<UStaticMeshComponent>()) return !C->bHiddenInGame;
  TArray<USceneComponent*> Children; C->GetChildrenComponents(true, Children);
  for (const USceneComponent* Child : Children)
   if (Child->IsA<UStaticMeshComponent>() && Child->IsVisible() && !Child->bHiddenInGame) return true;
  return false;
 }

 // Only suppress the old rendering. Its visibility flags, transforms and pick
 // geometry remain available to the original story and interaction code.
 void Mask(USceneComponent* Root)
 {
  if (!Root) return;
  TArray<USceneComponent*> Parts; Root->GetChildrenComponents(true, Parts); Parts.Add(Root);
  for (USceneComponent* Part : Parts) if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(Part))
  { P->SetRenderInMainPass(false); P->SetRenderInDepthPass(false); P->SetVisibleInRayTracing(false); P->SetCastShadow(false); }
 }

 FTransform Pose(const FVector& Feet, const FQuat& Rotation)
 {
  double E[16]; S47Three::Compose(Feet, Rotation, FVector::OneVector, E);
  return S47Three::MatToUE(E);
 }
}

struct FS47TripoNpcs::FState
{
 struct FSlot
 {
  FString Role;
  TWeakObjectPtr<UStaticMeshComponent> Mesh;
  bool bBound = false;
 };
 TArray<FSlot> Slots;
 TWeakObjectPtr<UStaticMeshComponent> Radio;
};

FS47TripoNpcs::FS47TripoNpcs(US47Main* InM) : State(MakeUnique<FState>()), M(InM)
{ bOff = FParse::Param(FCommandLine::Get(), TEXT("S47TripoNpcsOff")); }
FS47TripoNpcs::~FS47TripoNpcs() = default;

UStaticMeshComponent* FS47TripoNpcs::Component(const FString& Role) const
{
 const FState::FSlot* Slot = State->Slots.FindByPredicate([&](const FState::FSlot& S) { return S.Role == Role; });
 return Slot ? Slot->Mesh.Get() : nullptr;
}
int32 FS47TripoNpcs::BoundCount() const
{ int32 N = 0; for (const auto& S : State->Slots) if (S.bBound && S.Mesh.IsValid()) ++N; return N; }

void FS47TripoNpcs::Build()
{
 AS47Web* W = M->Web.Get();
 auto Make = [W](UStaticMesh* Asset, const FString& Name)
 {
  auto* C = NewObject<UStaticMeshComponent>(W, *Name);
  W->AddInstanceComponent(C); C->SetMobility(EComponentMobility::Movable);
  C->SetupAttachment(W->GetRootComponent()); C->SetStaticMesh(Asset);
  C->SetCollisionProfileName(TEXT("NoCollision")); C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->SetCollisionResponseToAllChannels(ECR_Ignore); C->SetGenerateOverlapEvents(false);
  C->SetCanEverAffectNavigation(false); C->SetCastShadow(true);
  C->SetVisibility(false); C->RegisterComponent(); return C;
 };
 for (const TCHAR* Role : Roles)
 {
  const FString Name = FString(TEXT("SM_Tripo_")) + Role;
  const FString Path = TEXT("/Game/Licensed/TripoNPC/") + Name + TEXT(".") + Name;
  UStaticMesh* Asset = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
  FState::FSlot Slot; Slot.Role = Role;
  if (Asset)
  {
   UStaticMeshComponent* C = Make(Asset, FString(TEXT("TripoNPC_")) + Role);
   C->ComponentTags.Add(*FString::Printf(TEXT("S47Tripo:%s"), Role)); Slot.Mesh = C;
  }
  else UE_LOG(LogS47TripoNpcs, Warning, TEXT("S47: Tripo %s unavailable; original visual retained"), Role);
  State->Slots.Add(MoveTemp(Slot));
 }
 // The old bench figure and its radio were merged by material. Keep a real
 // radio in her lap when that composite figure is replaced.
 if (Component(TEXT("radio_woman")))
  if (auto* Asset = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Kelvora/SM_Kelvora_Radio.SM_Kelvora_Radio"), nullptr, LOAD_NoWarn | LOAD_Quiet))
  { State->Radio = Make(Asset, TEXT("TripoBenchRadio")); }
 bInitialized = true;
}

void FS47TripoNpcs::Update()
{
 AS47Web* W = M->Web.Get();
 FS47Town* Town = M->TownLayer && M->TownLayer->Town ? M->TownLayer->Town.Get() : nullptr;
 const FS47Obj TownGroup = W->Obj(TEXT("world.town.group"));
 const FS47Obj DinerInterior = W->Obj(TEXT("world.diner.interior"));
 auto Named = [W](const TCHAR* Name) { return FindObjectFast<USceneComponent>(W, FName(Name)); };
 auto NodeComponent = [W](const FS47Obj& O) -> USceneComponent*
 { const FS47WebNode* N = O ? W->Node(O.I) : nullptr; return N ? N->Comp.Get() : nullptr; };
 auto Place = [](FState::FSlot& Slot, const FVector& At, const FQuat& Q, bool bShow)
 {
  if (auto* C = Slot.Mesh.Get()) { C->SetWorldTransform(Pose(At, Q)); C->SetVisibility(bShow); Slot.bBound = true; }
 };
 auto OnObject = [&](FState::FSlot& Slot, const TCHAR* Path, const FVector& Offset, bool bFlip)
 {
  const FS47Obj O = W->Obj(Path); if (!O) return;
  Place(Slot, O.LocalToWorld(Offset), O.WorldQuaternion() * S47Three::Euler(0, bFlip ? PI : 0, 0), O.Shown());
  Mask(NodeComponent(O));
 };
 const bool bPlay = M->Mode == US47Main::EMode::Play;
 for (auto& Slot : State->Slots)
 {
  UStaticMeshComponent* C = Slot.Mesh.Get(); if (!C) continue;
  const FString& R = Slot.Role; Slot.bBound = false; C->SetVisibility(false);
  if (R == TEXT("patron")) OnObject(Slot, TEXT("world.diner.objs.driver"), FVector(0,.28,0), true);
  else if (R == TEXT("nora")) OnObject(Slot, TEXT("world.room6.objs.nora"), FVector::ZeroVector, true);
  else if (R == TEXT("radio_woman")) OnObject(Slot, TEXT("world.town.woman"), FVector(0,-.34,-.13), true);
  else if (R == TEXT("souvenir_man")) OnObject(Slot, TEXT("world.town.smoker"), FVector::ZeroVector, true);
  else if (R == TEXT("dale"))
  {
   USceneComponent* Room = Named(TEXT("OperationsRoomRoot"));
   if (Room) Place(Slot, FVector(1.07,.02,7.88), S47Three::Euler(0,PI,0), Shown(Room));
  }
  else if (R == TEXT("waitress"))
  {
   const FS47Obj Old = W->Obj(TEXT("world.diner.objs.waitress"));
   // A spare mannequin by the kitchen has no dialogue proxy. The working
   // MetaHuman remains the waitress at the counter.
   if (Old && DinerInterior) Place(Slot, Old.WorldPosition() + FVector(.03,0,-2.40),
    S47Three::Euler(0,PI/2,0), DinerInterior.Shown());
  }
  else if (R == TEXT("attendant"))
  {
   const FS47Obj Window = W->Obj(TEXT("world.town.proxies.gasWindow"));
   if (Window) Place(Slot, Window.WorldPosition() + FVector(-.50,-.90,0), S47Three::Euler(0,PI/2,0), TownGroup.Shown());
  }
  else if (R == TEXT("earl"))
  {
   const FS47Obj Door = W->Obj(TEXT("world.town.proxies.amDoor"));
   if (Door) Place(Slot, Door.WorldPosition() + FVector(-.58,-.45,0), S47Three::Euler(0,-PI/2,0),
    TownGroup.Shown() && M->Story && S47Timeline::EarlOnDuty(M->Story->Clock()));
  }
  else if (R == TEXT("deputy"))
  {
   USceneComponent* Body = Named(TEXT("SC03DeputyBody"));
   if (Body && Shown(Body))
   {
    FTransform T = Body->GetComponentTransform(); T.SetLocation(T.GetLocation() - FVector(0,0,90));
    T.SetScale3D(FVector::OneVector); T.ConcatenateRotation(FQuat(FVector::UpVector,-PI/2));
    C->SetWorldTransform(T); C->SetVisibility(true); Slot.bBound = true;
    Mask(Body); Mask(Named(TEXT("SC03DeputyHead"))); Mask(Named(TEXT("SC03DeputyHat")));
   }
   else
   {
    const FS47Obj Door = W->Obj(TEXT("world.town.proxies.sheriffDoor"));
    if (Door) Place(Slot, Door.WorldPosition() + FVector(.62,-1.05,0), S47Three::Euler(0,PI/2,0),
     TownGroup.Shown() && Town && Town->bDeputyIn);
   }
  }
  else if (R == TEXT("mara_ortiz"))
  {
   if (USceneComponent* Body = Named(TEXT("SC03OrtizCoat")))
   {
    FTransform T = Body->GetComponentTransform(); T.SetLocation(T.GetLocation() - FVector(0,0,118));
    T.SetScale3D(FVector::OneVector); T.ConcatenateRotation(FQuat(FVector::UpVector,-PI/2));
    C->SetWorldTransform(T); C->SetVisibility(Shown(Body)); Slot.bBound = true;
    Mask(Body); Mask(Named(TEXT("SC03OrtizHead"))); Mask(Named(TEXT("SC03OrtizLegs")));
   }
  }
  else if (R == TEXT("tall_one") || R == TEXT("crawler"))
  {
   USceneComponent* Root = Named(R == TEXT("tall_one") ? TEXT("SC03Figure0") : TEXT("SC03Figure1"));
   if (Root)
   {
    FTransform T = Root->GetComponentTransform(); T.SetScale3D(FVector::OneVector);
    T.ConcatenateRotation(FQuat(FVector::UpVector,-PI/2));
    C->SetWorldTransform(T); C->SetVisibility(Shown(Root)); Slot.bBound = true; Mask(Root);
   }
  }
  else if (R == TEXT("shadow_man"))
  {
   const FS47Obj O = S47SceneObject(W,TEXT("glimpse"));
   if (O) { Place(Slot,O.WorldPosition(),O.WorldQuaternion(),O.Shown()); Mask(NodeComponent(O)); }
  }
  else if (R == TEXT("hereford_25") && M->TownLayer)
  {
   // Keep the pasture's tag, missing-cow rule, eye sprites and query proxy.
   // Suppress just #25's old render instances; every other cow stays intact.
   for (int32 I=0; I<M->TownLayer->Herds.Num(); ++I)
   {
    const auto& H = M->TownLayer->Herds[I]; if (!H) continue;
    FVector HeadAt; if (!H->CowAt(S47Herd::MISSING,HeadAt)) continue;
    const FString Prefix = FString::Printf(TEXT("world.herds.%d"),I);
    FQuat Q = S47Three::Euler(0,H->bStill ? PI-26*PI/180 : PI+2.2,0);
    for (const TCHAR* Kind : {TEXT(".bodies"),TEXT(".heads")})
    {
     auto* Pool = Cast<UInstancedStaticMeshComponent>(NodeComponent(W->Obj(Prefix+Kind)));
     if (!Pool || Pool->GetInstanceCount()<=S47Herd::MISSING) continue;
     FTransform T; if (!Pool->GetInstanceTransform(S47Herd::MISSING,T,false)) continue;
     T.SetScale3D(FVector::ZeroVector); Pool->UpdateInstanceTransform(S47Herd::MISSING,T,false,true,false);
    }
    if (H->Group.Shown()) Place(Slot,HeadAt-FVector(0,1.1,0),Q,true);
   }
  }
  if (!bPlay) C->SetVisibility(false);
 }
 if (UStaticMeshComponent* Radio = State->Radio.Get())
 {
  UStaticMeshComponent* Woman = Component(TEXT("radio_woman"));
  const FS47Obj Old = W->Obj(TEXT("world.town.woman"));
  if (Woman && Old)
  {
   const FVector Centre = S47Three::ToThree(Woman->GetComponentLocation()) + FVector(.30,.70,0);
   FTransform T=Pose(Centre,Old.WorldQuaternion());
   const double Span=Radio->GetStaticMesh()->GetBoundingBox().GetSize().GetMax();
   T.SetScale3D(FVector(22./FMath::Max(Span,1.)));
   T.SetLocation(T.GetLocation() - T.TransformVector(Radio->GetStaticMesh()->GetBoundingBox().GetCenter()));
   Radio->SetWorldTransform(T); Radio->SetVisibility(Woman->IsVisible());
  }
 }
}

void FS47TripoNpcs::Step()
{
 if (bOff || !M || !M->Web || !M->Web->IsLoaded()) return;
 if (!bInitialized) Build();
 Update();
 if (!bChecked)
 {
  bChecked=true;
  int32 Loaded=0; TArray<UStaticMeshComponent*> Components;
  for (const auto& S : State->Slots) if (auto* C=S.Mesh.Get()) { ++Loaded; Components.Add(C); }
  UE_LOG(LogS47TripoNpcs,Display,TEXT("S47: Tripo mannequins %d/14 loaded; %d bound, %d waiting for ordinary scene anchors; original SC02 actor and MetaHuman retained"),
   Loaded,BoundCount(),Loaded-BoundCount());
  if (FParse::Param(FCommandLine::Get(),TEXT("S47TripoNpcsTest")))
  {
   TArray<FString> Errors; const bool bOk=S47TripoNpcsTest::Check(Components,Errors);
   for (const FString& Error : Errors) UE_LOG(LogS47TripoNpcs,Error,TEXT("S47 Tripo: %s"),*Error);
   UE_LOG(LogS47TripoNpcs,Display,TEXT("S47 Tripo mannequins quick check: %s; %d loaded, %d bound, %d deferred"),
    bOk?TEXT("PASS"):TEXT("FAIL"),Loaded,BoundCount(),Loaded-BoundCount());
  }
 }
}
