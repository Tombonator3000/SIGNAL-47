#include "S47TripoNpcs.h"
#include "S47TripoNpcsTest.h"

#include "S47Main.h"
#include "S47Night.h"
#include "S47Glimpses.h"
#include "S47Herd.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47Town.h"
#include "S47TownDay.h"
#include "S47TownLayer.h"
#include "S47TripoCreature.h"
#include "S47CreatureClips.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
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

 // NPC-ANIM: the hips' height over the feet (cm, the skin's component space) at the start of a rigged figure's clip.
 double HipsCm(FS47TripoCreature& Rig, ES47Clip Clip)
 {
  if (!Rig.Clips || !Rig.Clips->Has(Clip)) return 0;
  TArray<FTransform> Local; Rig.Clips->Sample(Clip, 0, Local);
  const int32 Pelvis = Rig.Names.IndexOfByKey(FName(TEXT("pelvis")));
  if (Pelvis < 0 || Local.Num() != Rig.Parents.Num()) return 0;
  FTransform At = FTransform::Identity;
  for (int32 B = Pelvis; B >= 0; B = Rig.Parents[B]) At = At * Local[B];
  return At.GetLocation().Z;
 }
 // The static seated bakes' hips over their feet (seat_z_m, unreal-static-2026-10-30/manifest.json): their places are set by these.
 double BakedSeatM(const FString& Role)
 {
  return Role == TEXT("patron") ? .5928 : Role == TEXT("nora") ? .5451 : Role == TEXT("earl") ? .5741 : Role == TEXT("radio_woman") ? .5437 : 0.;
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
  // NPC-ANIM: the same figure on its own rig with clips, drawn instead of the bake where this build has it (Licensed/TripoNPC/Rigged).
  TSharedPtr<FS47TripoCreature> Rig;
  TWeakObjectPtr<USceneComponent> RigRoot;
  bool bSeated = false; // the figure is a seated one (roles.json): it sits wherever it is put
  double SeatLowerCm = 0; // a seated one: how far the rig goes down to have its hips where the bake had them
 };
 TArray<FSlot> Slots;
 TWeakObjectPtr<UStaticMeshComponent> Radio;
 struct FCow { int32 Herd=0, Index=0; TWeakObjectPtr<UStaticMeshComponent> Mesh; };
 TArray<FCow> Cows;
 bool bCowsBuilt=false;
};

FS47TripoNpcs::FS47TripoNpcs(US47Main* InM) : State(MakeUnique<FState>()), M(InM)
{ bOff = FParse::Param(FCommandLine::Get(), TEXT("S47TripoNpcsOff")); }
FS47TripoNpcs::~FS47TripoNpcs()
{
 // The day's walk goes back to the bake's steps for anyone this one put on a rig.
 if (State) for (const auto& Slot : State->Slots) if (Slot.Rig) S47TownDay::SetRigged(Slot.Role, false, false);
}

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
  // NPC-ANIM (Tom, 9 November: "Kan animasjoner fra andre figurer overføres til for eksempel byfolket, så de ikke sklir over
  // bakken?"). Every one of these figures came out of Tripo rigged; Tools/triponpcs/prepare_rigged_townsfolk.py keeps that rig and
  // gives it Tripo's clips from a human-proportioned body. Where this build has the result, the figure stands, walks and sits on
  // it (FS47ClipMotion: its steps go by the distance it really goes, its feet stay where they land); where it has not, or with
  // -S47RiggedNpcsOff, the static bake above as before. Placement, presence and story stay the bake's (Update).
  if (Slot.Mesh.IsValid() && !FParse::Param(FCommandLine::Get(), TEXT("S47RiggedNpcsOff")))
  {
   const FString Skin = FString::Printf(TEXT("/Game/Licensed/TripoNPC/Rigged/SK_Tripo_%s.SK_Tripo_%s"), Role, Role);
   if (USkeletalMesh* SkinAsset = LoadObject<USkeletalMesh>(nullptr, *Skin, nullptr, LOAD_NoWarn | LOAD_Quiet))
   {
    auto* RigRoot = NewObject<USceneComponent>(W, *FString::Printf(TEXT("TripoNPCRigRoot_%s"), Role));
    W->AddInstanceComponent(RigRoot); RigRoot->SetMobility(EComponentMobility::Movable);
    RigRoot->SetupAttachment(W->GetRootComponent()); RigRoot->RegisterComponent();
    TSharedPtr<FS47TripoCreature> Rig = MakeShared<FS47TripoCreature>();
    if (Rig->Build(W, RigRoot, FString::Printf(TEXT("TripoNPCRig_%s"), Role), *Skin))
    {
     // ANIM-TRY-20261112: -S47Free100=walk,idle (ClipFile names) wears those Free100 clips, retargeted by Tools/animtry/retarget.py,
     // where this build has them; every other clip, and every clip without the flag, is the Tripo one as before.
     static const TArray<FString> Free100 = [] { FString V; TArray<FString> Out; if (FParse::Value(FCommandLine::Get(), TEXT("S47Free100="), V, false)) V.ParseIntoArray(Out, TEXT(",")); return Out; }();
     const FString Prefer = FString::Printf(TEXT("/Game/Licensed/Free100/Retargeted/%s/A_F100_%s_"), Role, Role);
     Rig->Clips = FS47ClipBank::For(SkinAsset, *Rig, *FString::Printf(TEXT("/Game/Licensed/TripoNPC/Rigged/A_Tripo_%s_"), Role), Free100.Num() ? *Prefer : nullptr, &Free100);
    }
    if (Rig->Ready() && Rig->Clips)
    {
     // Both the bake and the rig face +Y with their feet at the pivot under the pelvis: the rig takes the bake's transform as it is.
     Rig->Mesh->SetRelativeRotation(FRotator::ZeroRotator);
     Slot.Rig = Rig; Slot.RigRoot = RigRoot;
     for (const TCHAR* Seated : { TEXT("patron"), TEXT("nora"), TEXT("earl"), TEXT("radio_woman") }) Slot.bSeated |= FCString::Strcmp(Role, Seated) == 0;
     const double SitHips = HipsCm(*Rig, ES47Clip::Sit), StandHips = HipsCm(*Rig, ES47Clip::Idle);
     S47TownDay::SetRigged(Role, true, Rig->Clips->Has(ES47Clip::Sit), SitHips / 100., StandHips / 100.);
     // A seated figure's place was set for its bake's hips (the seat), which sit lower than the clip's: the hips go where they
     // were, the feet a few centimetres into the floor in front of the seat, at most 6.
     if (Slot.bSeated && SitHips > 0 && BakedSeatM(Role) > 0) Slot.SeatLowerCm = FMath::Clamp(SitHips - 100. * BakedSeatM(Role), 0., 6.);
     UE_LOG(LogS47TripoNpcs, Display, TEXT("S47: Tripo %s on its own rig: walk %.2f m/s a cycle of %.0f cm, sit clip %s, hips %.0f cm standing and %.0f seated, %s"), Role,
      Rig->Clips->GaitSpeedMps(ES47Clip::Walk), Rig->Clips->GaitCycleCm(ES47Clip::Walk), Rig->Clips->Has(ES47Clip::Sit) ? TEXT("yes") : TEXT("no"),
      StandHips, SitHips, Slot.bSeated ? *FString::Printf(TEXT("always seated, %.1f cm down onto the bake's seat"), Slot.SeatLowerCm) : TEXT("seated where its day has it"));
    }
    else
    {
     if (UPoseableMeshComponent* Body = Rig->Mesh.Get()) Body->DestroyComponent();
     RigRoot->DestroyComponent();
     UE_LOG(LogS47TripoNpcs, Display, TEXT("S47: Tripo %s rigged skin found without its clips; the static bake is worn"), Role);
    }
   }
  }
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
 // NPC-DAY (S47TownDay, Freeform): where the day has them, walking, at work, at home (not drawn); else where they stood here.
 auto Daily = [&Place](FState::FSlot& Slot, const FVector& At, const FQuat& Q, bool bArea) -> bool
 {
  const FVector Ahead = Q.RotateVector(FVector(0, 0, 1));
  FVector Feet; double Yaw = 0; bool bShow = false;
  if (!S47TownDay::Pose(Slot.Role, At, FMath::Atan2(Ahead.X, Ahead.Z), Feet, Yaw, bShow)) return false;
  Place(Slot, Feet, S47Three::Euler(0, Yaw, 0), bArea && bShow);
  return true;
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
  else if (R == TEXT("radio_woman")) OnObject(Slot, TEXT("world.town.woman"), FVector(0,.12,.02), true);
  else if (R == TEXT("souvenir_man"))
  {
   const FS47Obj O = W->Obj(TEXT("world.town.smoker"));
   if (O)
   {
    const FVector At = O.LocalToWorld(FVector::ZeroVector); const FQuat Q = O.WorldQuaternion() * S47Three::Euler(0, PI, 0);
    Mask(NodeComponent(O));
    if (!Daily(Slot, At, Q, TownGroup.Shown())) Place(Slot, At, Q, O.Shown());
   }
  }
  else if (R == TEXT("dale"))
  {
   // NPC-DAY-FOLLOW-20261109: Dale is a voice on the phone and the walkie and is never met (DECISIONS); in Freeform his mannequin
   // is not stood in his office. Story keeps it as it was (a decision for Tom: TODO NPC-DAY-FOLLOW-20261109).
   USceneComponent* Room = Named(TEXT("OperationsRoomRoot"));
   if (Room) Place(Slot, FVector(1.07,.02,7.88), S47Three::Euler(0,PI,0), Shown(Room) && !M->IsFreeform());
  }
  else if (R == TEXT("waitress"))
  {
   const FS47Obj Old = W->Obj(TEXT("world.diner.objs.waitress"));
   // A spare mannequin by the kitchen has no dialogue proxy. The working
   // MetaHuman remains the waitress at the counter.
   if (Old && DinerInterior)
   {
    const FVector At = Old.WorldPosition() + FVector(.03,0,-2.40); const FQuat Q = S47Three::Euler(0,PI/2,0);
    if (!Daily(Slot, At, Q, DinerInterior.Shown())) Place(Slot, At, Q, DinerInterior.Shown());
   }
  }
  else if (R == TEXT("attendant"))
  {
   const FS47Obj Window = W->Obj(TEXT("world.town.proxies.gasWindow"));
   const FS47Obj Door = W->Obj(TEXT("world.town.proxies.gasDoor"));
   if (Window && Door)
   {
    // The night-window box starts at the counter, .65m above the floor.
    // Use the adjacent 2.20m doorway's bottom as the shared interior floor.
    const FVector At=Window.WorldPosition();
    const FVector Feet(At.X-.50,Door.WorldPosition().Y-1.10+.02,At.Z); const FQuat Q=S47Three::Euler(0,PI/2,0);
    if (!Daily(Slot,Feet,Q,TownGroup.Shown())) Place(Slot,Feet,Q,TownGroup.Shown());
   }
  }
  else if (R == TEXT("earl"))
  {
   const FS47Obj Door = W->Obj(TEXT("world.town.proxies.amDoor"));
   if (Door)
   {
    // Authored studio chair: Blender(1.55,2.8,.51), shared origin(0,5,0).
    // Door is a 2.2m-high proxy centred 1.10m above the real floor, not a feet anchor.
    // The seated bake's pelvis height comes from its private preparation manifest.
    const double Floor=Door.WorldPosition().Y-1.10;
    // NPC-DAY-FOLLOW-20261109: in a running Freeform day, by his own plan (S47TownDay::Present: midnight to four, never swapped
    // in the player's sight); otherwise the Night Line's hours as before.
    bool bOnAir=M->Story && S47Timeline::EarlOnDuty(M->Story->Clock());
    if (bool bDay=false; S47TownDay::Present(TEXT("earl"),bDay)) bOnAir=bDay;
    Place(Slot,FVector(8022.8,Floor+.585-.5741413869501052,2037.55),S47Three::Euler(0,-PI/2,0),TownGroup.Shown() && bOnAir);
   }
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
    if (Door)
    {
     const FVector At = Door.WorldPosition() + FVector(.62,-1.05,0); const FQuat Q = S47Three::Euler(0,PI/2,0);
     // (the diner is the town's area too: seen at lunch from the street or inside)
     if (!Daily(Slot, At, Q, TownGroup.Shown() || DinerInterior.Shown())) Place(Slot, At, Q, TownGroup.Shown() && Town && Town->bDeputyIn);
    }
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
   if (FParse::Param(FCommandLine::Get(),TEXT("S47RoswellOff"))) continue;
   // The little Roswell is owned by Glimpses, with its own skin/peek pose.
   // Retain the old shadow model only when that separate asset is unavailable.
   if (M->Night && M->Night->Glimpses && M->Night->Glimpses->HasNativeBody()) continue;
   const FS47Obj O = S47SceneObject(W,TEXT("glimpse"));
   if (O) { Place(Slot,O.WorldPosition(),O.WorldQuaternion(),O.Shown()); Mask(NodeComponent(O)); }
  }
  else if (R == TEXT("hereford_25") && M->TownLayer)
  {
   // Use the user's actual Hereford throughout the herd. The original pools
   // remain the pose/query source; hiding their draw must never zero transforms.
   if (!State->bCowsBuilt && !M->TownLayer->Herds.IsEmpty())
   {
    State->bCowsBuilt=true;
    for (int32 H=0; H<M->TownLayer->Herds.Num(); ++H)
     if (const auto& Herd=M->TownLayer->Herds[H]) for (int32 I=0; I<Herd->Count(); ++I)
     {
      auto* Cow=NewObject<UStaticMeshComponent>(W,*FString::Printf(TEXT("TripoHereford_%d_%d"),H,I));
      W->AddInstanceComponent(Cow); Cow->SetMobility(EComponentMobility::Movable); Cow->SetupAttachment(W->GetRootComponent());
      Cow->SetStaticMesh(C->GetStaticMesh()); Cow->SetCollisionProfileName(TEXT("NoCollision"));
      Cow->SetGenerateOverlapEvents(false); Cow->SetCanEverAffectNavigation(false); Cow->SetCastShadow(true);
      Cow->SetVisibility(false); Cow->RegisterComponent(); State->Cows.Add({H,I,Cow});
     }
   }
   bool bRepresentative=false;
   for (int32 I=0; I<M->TownLayer->Herds.Num(); ++I)
   {
    const auto& H = M->TownLayer->Herds[I]; if (!H) continue;
    const FString Prefix = FString::Printf(TEXT("world.herds.%d"),I);
    auto* Bodies=Cast<UInstancedStaticMeshComponent>(NodeComponent(W->Obj(Prefix+TEXT(".bodies"))));
    if (!Bodies) continue;
    for (const TCHAR* Kind : {TEXT(".bodies"),TEXT(".heads")})
     Mask(NodeComponent(W->Obj(Prefix+Kind)));
    for (const auto& Cow:State->Cows) if (Cow.Herd==I) if (auto* Draw=Cow.Mesh.Get())
    {
     FTransform T;
     const bool bAlive=Cow.Index!=H->Missing && Bodies->GetInstanceTransform(Cow.Index,T,true);
     if (bAlive)
     {
      // Prepared Tripo faces +Z in game space, the herd matrices face -Z.
      T.ConcatenateRotation(FQuat(FVector::UpVector,PI)); Draw->SetWorldTransform(T);
      if (Cow.Index==S47Herd::MISSING && H->Group.Shown()) { C->SetWorldTransform(T); bRepresentative=true; }
     }
     Draw->SetVisibility(bPlay && H->Group.Shown() && bAlive);
    }
   }
   Slot.bBound=bRepresentative;
   // The registered role component remains the roster/query representative;
   // its pooled counterpart supplies the visible #25 just like its eight peers.
   C->SetVisibility(false);
  }
  if (!bPlay) C->SetVisibility(false);
 }
 // NPC-ANIM: a rigged figure is drawn where its bake would be, in its place: standing, walking (FS47ClipMotion reads the walk off
 // how far the root really goes, so the feet do not glide) or seated where the day has it seated (S47TownDay::SitNow; Pose gives
 // it the plain path, S47TownDay::SetRigged). The bake keeps its visibility flag, which the roster, the radio and the quick check
 // read, but is not drawn.
 const double Clock = W->GetWorld() ? W->GetWorld()->GetTimeSeconds() : 0.;
 for (auto& Slot : State->Slots)
 {
  UStaticMeshComponent* C = Slot.Mesh.Get(); USceneComponent* RigRoot = Slot.RigRoot.Get();
  if (!C || !RigRoot || !Slot.Rig || !Slot.Rig->Ready() || !Slot.Rig->Clips) continue;
  C->SetHiddenInGame(true);
  FTransform T = C->GetComponentTransform(); T.SetScale3D(FVector::OneVector);
  if (Slot.bSeated) T.AddToTranslation(FVector(0, 0, -Slot.SeatLowerCm));
  RigRoot->SetWorldTransform(T);
  FS47ClipPose Clip; Clip.bVisible = C->IsVisible() && Slot.bBound; Clip.Clock = Clock; Clip.Move = ES47Clip::Walk;
  double Sit = 0;
  const bool bSit = Slot.bSeated || (S47TownDay::SitNow(Slot.Role, Sit) && Sit > .5);
  if (bSit && Slot.Rig->Clips->Has(ES47Clip::Sit)) Clip.Stand = ES47Clip::Sit;
  Slot.Rig->PlayClips(Clip);
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
