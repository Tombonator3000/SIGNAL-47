#include "S47ArrayFence.h"
#include "S47Night.h"
#include "S47StreetLamps.h"
#include "S47Three.h"
#include "S47Web.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47ArrayFence, Log, All);

// D45-20261110 (D45-14): see S47ArrayFence.h. Everything here is in SARO's terms (three metres, y up), which are the world's: the build
// stops, changing nothing, unless ext.group and the pole batch stand at the origin (as S47StreetLamps requires of the batch too).
namespace S47ArrayFence
{
namespace
{
 constexpr double FenceZ = -20;                  // arrayLayout.ts FENCE_Z
 constexpr double GateX0 = -1.75, GateX1 = -.25; // arrayLayout.ts GATE: the walk gate's posts
 struct FRun { double X0, X1; };
 // Exterior.ts fence(): west of the highway, from the highway to the gate, from the gate east
 const FRun Runs[] = { { -80, -28.6 }, { -19.4, GateX0 }, { GateX1, 60 } };
 constexpr int32 RunCount = UE_ARRAY_COUNT(Runs);
 // The top arms lean south, out of the array towards the yard and the highway, the side one comes from (a security fence's outriggers
 // face out). The kit's arm leans to its own -y, so each part is turned half round about the vertical: its length then runs west.
 constexpr double Yaw = UE_DOUBLE_PI;
 constexpr double Direction = -1; // cos(Yaw): a panel runs this way along x from its post
 const TCHAR* const KitPath = TEXT("/Game/Licensed/MilitaryBarriers/Meshes/");

 UStaticMesh* Kit(const TCHAR* Name)
 {
  const FString Package = FString(KitPath) + Name;
  if (!FPackageName::DoesPackageExist(Package)) return nullptr;
  return LoadObject<UStaticMesh>(nullptr, *(Package + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
 }
 FString Bounds(const UStaticMesh* M)
 {
  if (!M) return TEXT("(missing)");
  const FBox B = M->GetBoundingBox();
  return FString::Printf(TEXT("x %.1f..%.1f y %.1f..%.1f z %.1f..%.1f cm"), B.Min.X, B.Max.X, B.Min.Y, B.Max.Y, B.Min.Z, B.Max.Z);
 }
 // ---- the kit's frame, as Tools/militarybarriers/verification.json measured it (centimetres): a part's length along +x from the post at
 // its pivot (the post's centre, on its foot, z 0), the top arm leaning to -y. Nothing is placed from a part that does not keep it. ----
 // The bare post (WiredFence_EndCap_c, 7.8 cm square, 2.8 m with its arm).
 bool PostFrame(const UStaticMesh* M)
 {
  if (!M) return false;
  const FBox B = M->GetBoundingBox();
  return FMath::Abs(B.Min.X + B.Max.X) < 1 && B.Max.X > 1 && B.Max.X < 10 && FMath::Abs(B.Min.Z) < 1.5 && B.Max.Z > 150 && B.Min.Y < -5;
 }
 // A panel (WiredFence_b): its post at the pivot and 2.5 m of mesh to where the next panel's post stands (Half: half the post's width).
 bool PanelFrame(const UStaticMesh* M, double Half)
 {
  if (!M) return false;
  const FBox B = M->GetBoundingBox();
  return FMath::Abs(B.Min.X + Half) < 1.5 && B.Max.X > 150 && B.Max.X < 400 && FMath::Abs(B.Min.Z) < 1.5 && B.Max.Z > 150 && B.Max.Z < 400
   && B.Min.Y < -5 && FMath::Abs(B.Max.Y - Half) < 1.5;
 }
 // A barbed strand set (Wire_a, _b): along +x from the pivot, standing up from it, centred on its line.
 bool StrandFrame(const UStaticMesh* M)
 {
  if (!M) return false;
  const FBox B = M->GetBoundingBox();
  return B.Max.X > 100 && FMath::Abs(B.Min.Z) < 1.5 && B.Max.Z > 10 && B.Max.Z < 80 && FMath::Abs(B.Min.Y + B.Max.Y) < 1.5;
 }
 // A gate leaf (Gate_R, Gate_L): over 1.5 m wide, standing on its foot.
 bool GateFrame(const UStaticMesh* M)
 {
  if (!M) return false;
  const FBox B = M->GetBoundingBox();
  return B.GetSize().X > 150 && FMath::Abs(B.Min.Z) < 1.5 && B.Max.Z > 150;
 }
 // Every slot's material compiled for instancing (import_fence.py sets it on MM_Basic_PBR): a cooked instanced draw would otherwise fall
 // back to the default material (as S47CalicheNature requires of its scatter).
 bool Instancing(const UStaticMesh* M)
 {
  if (!M || M->GetStaticMaterials().IsEmpty()) return false;
  for (const FStaticMaterial& Slot : M->GetStaticMaterials())
  {
   const UMaterialInterface* Mat = Slot.MaterialInterface;
   if (!Mat || !Mat->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes) || !Mat->CheckMaterialUsage_Concurrent(MATUSAGE_InstancedStaticMeshes)) return false;
  }
  return true;
 }
 // A part's box in three terms (metres, y up): S47Three swaps two axes and scales, so the corners stay the corners.
 FBox ThreeBox(const UStaticMesh* M)
 {
  const FBox B = M->GetBoundingBox();
  return FBox(S47Three::ToThree(B.Min), S47Three::ToThree(B.Max));
 }
 // A part in SARO's (or its parent node's) three terms: turned by Turn about the vertical, tilted by Tilt about its own length first.
 FTransform Part(const FVector& At, double Turn, const FVector& Scale, double Tilt = 0)
 {
  double Matrix[16];
  S47Three::Compose(At, FQuat(FVector(0, 1, 0), Turn) * FQuat(FVector(1, 0, 0), Tilt), Scale, Matrix);
  return S47Three::MatToUE(Matrix);
 }
 // Exterior.ts fence(): a 5 cm post every 7 m from x -80 to 60, but none in the highway's opening or by the gate, and a 6 cm one each
 // side of the highway, 2.6 and 2.7 m tall round y 0.6, all in the pole batch. Each old post whole, in the batch's own terms.
 TArray<FBox> OldPosts()
 {
  const auto Gap = [](double X) { return (X > -28.6 && X < -19.4) || (X > GateX0 - .8 && X < GateX1 + .8); };
  TArray<FBox> Boxes;
  const auto Add = [&Boxes](double X) { Boxes.Add(FBox(FVector(X - .12, -.8, FenceZ - .12), FVector(X + .12, 2.1, FenceZ + .12))); };
  for (int32 K = 0; K <= 20; ++K) if (!Gap(-80. + 7 * K)) Add(-80. + 7 * K);
  Add(-28.6); Add(-19.4);
  return Boxes;
 }
 // The first thing straight down from From (three world) at or under MaxY, but none of Ignore; bShownOnly: only what is drawn now.
 bool Down(UWorld* World, const FVector& From, double Depth, const TArray<const UPrimitiveComponent*>& Ignore, bool bShownOnly, double MaxY, double& Y)
 {
  FCollisionQueryParams Q(SCENE_QUERY_STAT(S47ArrayFenceFoot), true);
  for (const UPrimitiveComponent* P : Ignore) if (P) Q.AddIgnoredComponent(P);
  for (int32 Try = 0; Try < 10; ++Try)
  {
   FHitResult Hit;
   if (!World->LineTraceSingleByChannel(Hit, S47Three::ToUE(From), S47Three::ToUE(From - FVector(0, Depth, 0)), ECC_Visibility, Q)) return false;
   const UPrimitiveComponent* P = Hit.GetComponent();
   if (!P) return false;
   const double At = S47Three::ToThree(Hit.ImpactPoint).Y;
   if ((!bShownOnly || (P->IsVisible() && !P->bHiddenInGame)) && At <= MaxY) { Y = At; return true; }
   Q.AddIgnoredComponent(P);
  }
  return false;
 }
 // The ground under (X, Z): within 30 cm of SARO's walking floor (Grounds.ts floorAt: the array graded flat at -0.6, its ground mesh some
 // 2 cm under it), what is there (drawn now or not, unless bShownOnly: at build an area away is hidden); else that floor. bFound: measured.
 double Ground(AS47Web* W, double X, double Z, const TArray<const UPrimitiveComponent*>& Ignore, bool bShownOnly, bool* bFound = nullptr)
 {
  const double Floor = US47Night::SaroFloorAt(W, X, Z);
  double Y = Floor;
  UWorld* World = W->GetWorld();
  const bool bHit = World && Down(World, FVector(X, Floor + 1.5, Z), 3, Ignore, bShownOnly, Floor + .1, Y) && FMath::Abs(Y - Floor) <= .1; // D45: .3 let one panel stand 30 cm up on something else
  if (bFound) *bFound = bHit;
  return bHit ? Y : Floor;
 }
 bool Shown(const UPrimitiveComponent* C) { return C && C->IsVisible() && !C->bHiddenInGame; }
 // An export part retired: hidden at node level (so no area's visibility pass brings it back), its draw and collision off.
 void Retire(AS47Web* W, int32 Node)
 {
  W->SetVisible(Node, false);
  const FS47WebNode* N = W->Node(Node);
  if (UPrimitiveComponent* C = N ? Cast<UPrimitiveComponent>(N->Comp.Get()) : nullptr)
  { C->SetVisibility(false, false); C->SetHiddenInGame(true, false); C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
 }
 bool Retired(const AS47Web* W, int32 Node)
 {
  const FS47WebNode* N = W->Node(Node);
  const UPrimitiveComponent* C = N ? Cast<UPrimitiveComponent>(N->Comp.Get()) : nullptr;
  return N && !N->bVis && C && !C->IsVisible() && C->bHiddenInGame && C->GetCollisionEnabled() == ECollisionEnabled::NoCollision;
 }

 struct FState
 {
  TArray<int32> Planes;    // the export's three unlit planes, a run each
  TArray<int32> OldLeaf;   // the gate leaf's old chain-link plane and pipe frame (world.field.gateHinge's children)
  int32 Hinge = INDEX_NONE;
  TWeakObjectPtr<UStaticMeshComponent> Batch; // the pole batch the old posts were cut from
  TArray<FBox> PostBoxes;
  TArray<int32> Cut;       // the triangles taken per old post
  TWeakObjectPtr<UInstancedStaticMeshComponent> Panels, Posts, Strands;
  TWeakObjectPtr<UStaticMeshComponent> Gate;
  FTransform GateRelative;
  TArray<int32> Counts;    // panels per run
  TArray<double> Stretches;
  int32 StrandCount = 0;
 };
 TMap<TWeakObjectPtr<AS47Web>, FState>& All() { static TMap<TWeakObjectPtr<AS47Web>, FState> A; return A; }
}

void Layout(double Run, double Module, int32& Count, double& Stretch)
{
 Count = Module > 0 && Run > 0 ? FMath::Max(1, FMath::RoundToInt(Run / Module)) : 0;
 Stretch = Count > 0 ? Run / (Count * Module) : 1;
}

bool Build(AS47Web* W)
{
 if (!W || FParse::Param(FCommandLine::Get(), TEXT("S47ArrayFenceOld")) || FParse::Param(FCommandLine::Get(), TEXT("S47PropsOff"))) return false;
 if (All().Contains(W)) return true; // built once per world: the old planes and posts are gone by now
 UStaticMesh* PanelMesh = Kit(TEXT("SM_MilitaryBase_WiredFence_b"));
 UStaticMesh* PostMesh = Kit(TEXT("SM_MilitaryBase_WiredFence_EndCap_c"));
 if (!PanelMesh || !PostMesh)
 { UE_LOG(LogS47ArrayFence, Display, TEXT("S47: array fence: the Military Barriers kit is not imported (%s); the old fence stays"), KitPath); return false; }
 const double Half = PostMesh->GetBoundingBox().Max.X; // centimetres: half the post's width
 if (!PostFrame(PostMesh) || !PanelFrame(PanelMesh, Half))
 {
  UE_LOG(LogS47ArrayFence, Warning, TEXT("S47: array fence: the kit's frame differs from verification.json (post %s, panel %s); the old fence stays"), *Bounds(PostMesh), *Bounds(PanelMesh));
  return false;
 }
 if (!Instancing(PanelMesh) || !Instancing(PostMesh))
 { UE_LOG(LogS47ArrayFence, Warning, TEXT("S47: array fence: the kit's materials are not compiled for instancing (import_fence.py sets it on MM_Basic_PBR); the old fence stays")); return false; }
 UStaticMesh* WireMesh = nullptr;
 if (!FParse::Param(FCommandLine::Get(), TEXT("S47ArrayFenceNoWire")))
 {
  for (const TCHAR* Name : { TEXT("SM_MilitaryBase_Wire_b"), TEXT("SM_MilitaryBase_Wire_a") })
   if (UStaticMesh* Candidate = Kit(Name); !WireMesh && StrandFrame(Candidate) && Instancing(Candidate)) WireMesh = Candidate;
  if (!WireMesh) UE_LOG(LogS47ArrayFence, Warning, TEXT("S47: array fence: no barbed strand in the kit keeps its frame (%s); the fence goes up without it"), *Bounds(Kit(TEXT("SM_MilitaryBase_Wire_b"))));
 }
 // The gate leaf hinges on its west post and swings north. Gate_R (hinged at its +x end) turned half round has its arm south, as the
 // fence's; Gate_L unturned is the same leaf with its arm north, if Gate_R is missing.
 UStaticMesh* GateMesh = Kit(TEXT("SM_MilitaryBase_WiredFence_Gate_R"));
 double GateTurn = Yaw;
 if (!GateFrame(GateMesh)) { GateMesh = Kit(TEXT("SM_MilitaryBase_WiredFence_Gate_L")); GateTurn = 0; }
 if (!GateFrame(GateMesh)) GateMesh = nullptr;

 // ---- the export as it was built: ext.group and the pole batch at the origin, the three planes, the gate's hinge ----
 const int32 Ext = W->Handle(TEXT("ext.group"));
 const FS47WebNode* ExtNode = W->Node(Ext);
 USceneComponent* ExtComp = ExtNode ? ExtNode->Comp.Get() : nullptr;
 if (!ExtComp || !ExtComp->GetComponentTransform().Equals(FTransform::Identity, .001) || !W->NodeWorldThree(Ext).Equals(FTransform::Identity, .001))
 { UE_LOG(LogS47ArrayFence, Warning, TEXT("S47: array fence: ext.group is missing or not at the origin; the old fence stays")); return false; }
 UStaticMeshComponent* Batch = nullptr;
 int32 Batches = 0;
 for (int32 I = 0; I < W->PartCount(); ++I)
  if (const FS47WebNode* N = W->Node(I); N && N->Geo == 26)
   if (UStaticMeshComponent* C = Cast<UStaticMeshComponent>(N->Comp.Get())) { Batch = C; ++Batches; }
 if (Batches != 1 || !Batch->GetComponentTransform().Equals(FTransform::Identity, .001))
 { UE_LOG(LogS47ArrayFence, Warning, TEXT("S47: array fence: the pole batch (g26) is not the one batch at the origin (%d found); the old fence stays"), Batches); return false; }
 // The planes by what they are, never by their index: under ext.group, at y 0.6 on the fence's line, each as wide as its run and centred
 // on it, two triangles (Exterior.ts plane()).
 TArray<int32> Planes; Planes.Init(INDEX_NONE, RunCount);
 bool bTwice = false;
 for (int32 I = 0; I < W->PartCount(); ++I)
 {
  const FS47WebNode* N = W->Node(I);
  if (!N || N->Parent != Ext || N->bInstanced) continue;
  const UStaticMeshComponent* C = Cast<UStaticMeshComponent>(N->Comp.Get());
  const UStaticMesh* Plane = C ? C->GetStaticMesh().Get() : nullptr;
  if (!Plane || Plane->GetNumTriangles(0) > 4 || FMath::Abs(N->P.Z - FenceZ) > .01 || FMath::Abs(N->P.Y - .6) > .05) continue;
  const double Wide = Plane->GetBoundingBox().GetSize().X / 100;
  for (int32 R = 0; R < RunCount; ++R)
   if (FMath::Abs(N->P.X - (Runs[R].X0 + Runs[R].X1) / 2) < .05 && FMath::Abs(Wide - (Runs[R].X1 - Runs[R].X0)) < .05)
   { bTwice |= Planes[R] != INDEX_NONE; Planes[R] = I; }
 }
 if (bTwice || Planes.Contains(INDEX_NONE))
 {
  UE_LOG(LogS47ArrayFence, Warning, TEXT("S47: array fence: the export's three fence planes were not each found once (%s); the old fence stays"),
   *FString::JoinBy(Planes, TEXT(","), [](int32 I) { return FString::FromInt(I); }));
  return false;
 }
 // The gate leaf's parts (Field.ts gate(): merged by material under the hinge): the chain-link plane (two triangles across the leaf),
 // the pipe frame (across it too), and the latch and its chain (a few centimetres at the far side), which stay.
 const int32 Hinge = W->Handle(TEXT("world.field.gateHinge"));
 USceneComponent* HingeComp = W->Node(Hinge) ? W->Node(Hinge)->Comp.Get() : nullptr;
 TArray<int32> OldLeaf;
 if (GateMesh && HingeComp)
 {
  const TArray<int32> Children = W->Node(Hinge)->Children; // a copy: adopting nodes below grows the node list
  int32 Leaves = 0;
  for (const int32 I : Children)
  {
   const FS47WebNode* N = W->Node(I);
   const UStaticMeshComponent* C = N ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
   const UStaticMesh* Part0 = C ? C->GetStaticMesh().Get() : nullptr;
   if (!Part0 || Part0->GetBoundingBox().GetSize().X < 120) continue; // the latch and chain
   OldLeaf.Add(I); Leaves += Part0->GetNumTriangles(0) <= 4;
  }
  if (Leaves != 1 || OldLeaf.Num() > 2)
  {
   UE_LOG(LogS47ArrayFence, Warning, TEXT("S47: array fence: the gate leaf's chain-link was not found once under its hinge (%d planes, %d parts); the old leaf stays"), Leaves, OldLeaf.Num());
   GateMesh = nullptr; OldLeaf.Reset();
  }
 }
 else if (GateMesh) { UE_LOG(LogS47ArrayFence, Warning, TEXT("S47: array fence: no world.field.gateHinge; the old leaf stays")); GateMesh = nullptr; }

 // ---- the old posts out of the pole batch, with the street lamps' cut (at least 12 triangles each, or nothing changes) ----
 const TArray<FBox> PostBoxes = OldPosts();
 TArray<int32> Cut;
 UStaticMesh* Without = S47StreetLamps::CutBatch(W, Batch, PostBoxes, Cut);
 bool bAll = Without != nullptr;
 for (const int32 K : Cut) bAll &= K >= 12;
 if (!bAll)
 {
  UE_LOG(LogS47ArrayFence, Warning, TEXT("S47: array fence: the old posts were not all found in g26 (%s; CPU data %d); the old fence stays"),
   *FString::JoinBy(Cut, TEXT(","), [](int32 K) { return FString::FromInt(K); }), Batch->GetStaticMesh() && Batch->GetStaticMesh()->bAllowCPUAccess ? 1 : 0);
  return false;
 }

 // ---- where everything goes, measured before anything changes ----
 TArray<const UPrimitiveComponent*> Ignore = { Batch };
 for (const int32 I : Planes) if (const FS47WebNode* N = W->Node(I)) Ignore.Add(Cast<UPrimitiveComponent>(N->Comp.Get()));
 for (const int32 I : OldLeaf) if (const FS47WebNode* N = W->Node(I)) Ignore.Add(Cast<UPrimitiveComponent>(N->Comp.Get()));
 const FBox PanelBox = ThreeBox(PanelMesh);
 const double Module = PanelBox.Max.X; // metres from a panel's post to the next panel's
 const double Top = PanelBox.Max.Y;     // the arm's tip over the foot
 TArray<FTransform> PanelAt, PostAt, StrandAt;
 TArray<int32> Counts; TArray<double> Stretches;
 int32 Measured = 0, Floor = 0;
 const auto Foot = [&](double X)
 {
  bool bFound = false;
  const double Y = Ground(W, X, FenceZ, Ignore, false, &bFound);
  ++(bFound ? Measured : Floor);
  return Y;
 };
 for (const FRun& Run : Runs)
 {
  // the first panel's post at the run's end the panels run away from, the closing post at the other
  const double Length = Run.X1 - Run.X0, Middle = (Run.X0 + Run.X1) / 2, Start = Middle - Direction * Length / 2, End = Middle + Direction * Length / 2;
  int32 Each = 0; double Stretch = 1;
  Layout(Length, Module, Each, Stretch);
  Counts.Add(Each); Stretches.Add(Stretch);
  if (FMath::Abs(Stretch - 1) > .1) UE_LOG(LogS47ArrayFence, Warning, TEXT("S47: array fence: the run x %.2f..%.2f stretches its %d panels by %.3f"), Run.X0, Run.X1, Each, Stretch);
  // each panel's post on its own place, the panel seated on the ground at its middle, stretched along its length only
  for (int32 K = 0; K < Each; ++K)
  {
   const double PostX = Start + Direction * K * Module * Stretch;
   const double Y = Foot(PostX + Direction * Module * Stretch / 2);
   PanelAt.Add(Part(FVector(PostX, Y - PanelBox.Min.Y, FenceZ), Yaw, FVector(Stretch, 1, 1)));
  }
  // a bare post closing the run where the last panel ends (by the highway's opening, the gate's east post, the fence's west end)
  PostAt.Add(Part(FVector(End, Foot(End), FenceZ), Yaw, FVector::OneVector));
  if (!WireMesh) continue;
  // the strands from each post's top along the arm to its tip, laid in pieces of the strand's own length along the run
  const FBox WireBox = ThreeBox(WireMesh);
  const double Piece = WireBox.Max.X, Rise = WireBox.Max.Y;
  const double Lean = -PanelBox.Min.Z - WireBox.Max.Z; // the arm's tip out from the post's line, less the strands' half width
  const double Tilt = FMath::Asin(FMath::Clamp(Lean / Rise, 0., .7));
  int32 Pieces = 0; double WireStretch = 1;
  Layout(Length, Piece, Pieces, WireStretch);
  for (int32 K = 0; K < Pieces; ++K)
  {
   const double WireX = Start + Direction * K * Piece * WireStretch;
   const double Y = Foot(WireX + Direction * Piece * WireStretch / 2);
   StrandAt.Add(Part(FVector(WireX, Y + Top - Rise * FMath::Cos(Tilt), FenceZ), Yaw, FVector(WireStretch, 1, 1), -Tilt));
  }
 }
 // the gate leaf in the hinge's own terms: across the 1.44 m the old leaf filled (Field.ts: 3 cm in from each post), 5 cm off the ground
 FVector GateAt = FVector::ZeroVector, GateScale = FVector::OneVector;
 if (GateMesh)
 {
  const FBox Leaf = ThreeBox(GateMesh);
  const double Wide = GateX1 - GateX0 - .06;
  GateScale = FVector(Wide / Leaf.GetSize().X, 1, 1);
  const FBox Turned = Leaf.TransformBy(FTransform(FQuat(FVector(0, 1, 0), GateTurn), FVector::ZeroVector, GateScale));
  const FVector Under = W->NodeWorldThree(Hinge).InverseTransformPosition(FVector((GateX0 + GateX1) / 2, Foot((GateX0 + GateX1) / 2), FenceZ));
  GateAt = FVector(.03 - Turned.Min.X, Under.Y + .05 - Turned.Min.Y, 0);
 }

 // ---- commit: the batch without the old posts, the old planes and leaf hidden, the kit in their place ----
 Without->InitResources(); Batch->SetStaticMesh(Without);
 for (const int32 I : Planes) Retire(W, I);
 const auto Instanced = [W, Ext, ExtComp](const TCHAR* Label, UStaticMesh* Shape, const TArray<FTransform>& Where)
 {
  auto* C = NewObject<UInstancedStaticMeshComponent>(W, *FString::Printf(TEXT("S47ArrayFence_%s"), Label));
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(ExtComp); C->SetStaticMesh(Shape);
  // the walk and the truck keep the fence's 2D lines (S47World); nothing here blocks a trace or a step
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCollisionResponseToAllChannels(ECR_Ignore);
  // masked, two-sided chain-link: its shadow, but no contact shadow and no distance field (a solid wall to Lumen)
  C->SetCastShadow(true); C->SetCastContactShadow(false); C->SetAffectDistanceFieldLighting(false);
  C->RegisterComponent();
  for (const FTransform& T : Where) C->AddInstance(T, false);
  W->AdoptNode(C, Ext, FString(TEXT("arrayFence:")) + Label);
  return C;
 };
 FState& S = All().FindOrAdd(W);
 S.Planes = Planes; S.Batch = Batch; S.PostBoxes = PostBoxes; S.Cut = Cut; S.Counts = Counts; S.Stretches = Stretches;
 S.Panels = Instanced(TEXT("panels"), PanelMesh, PanelAt);
 S.Posts = Instanced(TEXT("posts"), PostMesh, PostAt);
 if (WireMesh && StrandAt.Num()) { S.Strands = Instanced(TEXT("strands"), WireMesh, StrandAt); S.StrandCount = StrandAt.Num(); }
 if (GateMesh)
 {
  for (const int32 I : OldLeaf) Retire(W, I);
  auto* G = NewObject<UStaticMeshComponent>(W, TEXT("S47ArrayFence_Gate"));
  G->SetMobility(EComponentMobility::Movable); G->SetupAttachment(HingeComp); G->SetStaticMesh(GateMesh);
  G->SetCollisionEnabled(ECollisionEnabled::NoCollision); G->SetCollisionResponseToAllChannels(ECR_Ignore);
  G->SetCastShadow(true); G->SetCastContactShadow(false); G->SetAffectDistanceFieldLighting(false);
  G->RegisterComponent();
  // a child of the hinge node: the door's swing (AS47Web::ApplyDoor moves the hinge) carries it, and it is shown with the leaf
  const int32 Node = W->AdoptNode(G, Hinge, TEXT("arrayFence:gate"));
  if (Node >= 0) W->SetLocal(Node, GateAt, FQuat(FVector(0, 1, 0), GateTurn), GateScale);
  S.Gate = G; S.GateRelative = G->GetRelativeTransform(); S.Hinge = Hinge; S.OldLeaf = OldLeaf;
 }
 FString Runs3;
 for (int32 R = 0; R < RunCount; ++R) Runs3 += FString::Printf(TEXT("%s%d x %.3f"), R ? TEXT(", ") : TEXT(""), Counts[R], Stretches[R]);
 UE_LOG(LogS47ArrayFence, Display, TEXT("S47: array fence: the wired fence stands: %d panels of %.2f m (%s), %d closing posts, %d strand pieces, gate %s; %s old post triangles cut from g26, the three planes hidden; %d feet on the ground measured, %d on SARO's floor"),
  PanelAt.Num(), Module, *Runs3, PostAt.Num(), S.StrandCount, GateMesh ? *GateMesh->GetName() : TEXT("(the old leaf)"),
  *FString::JoinBy(Cut, TEXT(","), [](int32 K) { return FString::FromInt(K); }), Measured, Floor);
 return true;
}

int32 Audit(AS47Web* W, TArray<FString>& Lines)
{
 const FState* S = W ? All().Find(W) : nullptr;
 if (!S) { Lines.Add(TEXT("SKIP array fence: not built (the kit not imported, -S47ArrayFenceOld, or the export differs); the old fence stands")); return 0; }
 int32 Bad = 0;
 TArray<FString> Each;
 const auto Note = [&Bad, &Each](bool bGood, const FString& What) { Bad += !bGood; Each.Add(FString::Printf(TEXT("  %s %s"), bGood ? TEXT("ok") : TEXT("WRONG"), *What)); };
 // the old planes and leaf: hidden at node level, not drawn, no collision
 int32 Hidden = 0;
 for (const int32 I : S->Planes) Hidden += Retired(W, I);
 Note(Hidden == RunCount && S->Planes.Num() == RunCount, FString::Printf(TEXT("old fence planes hidden: %d of %d"), Hidden, RunCount));
 // the old posts: every one cut at build (12 triangles or more) and none left in the batch now
 TArray<int32> Left;
 const bool bRead = S47StreetLamps::CountBatch(S->Batch.Get(), S->PostBoxes, Left);
 int32 Remaining = 0, Taken = 0;
 for (const int32 K : Left) Remaining += K;
 for (const int32 K : S->Cut) Taken += K >= 12;
 // (a batch without CPU data, as in an editor run, cannot be read back: the cut at build is the proof then)
 Note((!bRead || Remaining == 0) && Taken == S->PostBoxes.Num(), FString::Printf(TEXT("old posts cut: %d of %d, %s triangles left in the pole batch"), Taken, S->PostBoxes.Num(),
  bRead ? *FString::FromInt(Remaining) : TEXT("(unread: no CPU data)")));
 // the panels per run and their stretch
 const UInstancedStaticMeshComponent* Panels = S->Panels.Get();
 int32 Expected = 0, Stretched = 0;
 for (int32 R = 0; R < S->Counts.Num(); ++R) { Expected += S->Counts[R]; Stretched += FMath::Abs(S->Stretches[R] - 1) > .1; }
 Note(Panels && Panels->GetInstanceCount() == Expected && Stretched == 0 && Panels->GetCollisionEnabled() == ECollisionEnabled::NoCollision,
  FString::Printf(TEXT("panels: %d drawn of %d in %d runs, %d runs stretched over 10 %%, no collision"), Panels ? Panels->GetInstanceCount() : 0, Expected, S->Counts.Num(), Stretched));
 const UInstancedStaticMeshComponent* Posts = S->Posts.Get();
 Note(Posts && Posts->GetInstanceCount() == RunCount, FString::Printf(TEXT("closing posts: %d of %d"), Posts ? Posts->GetInstanceCount() : 0, RunCount));
 const UInstancedStaticMeshComponent* Strands = S->Strands.Get();
 Note(S->StrandCount == 0 || (Strands && Strands->GetInstanceCount() == S->StrandCount), FString::Printf(TEXT("barbed strand pieces: %d of %d"), Strands ? Strands->GetInstanceCount() : 0, S->StrandCount));
 // every panel's foot against the ground drawn under its middle, where the fence is drawn now
 FString Feet = TEXT("not measured (the fence is not drawn here)");
 if (Panels && Shown(Panels) && Panels->GetStaticMesh())
 {
  const FBox Local = Panels->GetStaticMesh()->GetBoundingBox();
  const TArray<const UPrimitiveComponent*> Ignore = { S->Batch.Get(), Panels, Posts, Strands };
  int32 Off = 0, Lost = 0; double Worst = 0;
  for (int32 K = 0; K < Panels->GetInstanceCount(); ++K)
  {
   FTransform T;
   if (!Panels->GetInstanceTransform(K, T, true)) { ++Lost; continue; }
   const FBox U = Local.TransformBy(T);
   const FVector Low = S47Three::ToThree(U.Min), Mid = S47Three::ToThree(U.GetCenter());
   bool bFound = false;
   const double Y = Ground(W, Mid.X, FenceZ, Ignore, true, &bFound);
   if (!bFound) { ++Lost; continue; }
   Worst = FMath::Max(Worst, FMath::Abs(Low.Y - Y)); Off += FMath::Abs(Low.Y - Y) > .03;
  }
  Feet = FString::Printf(TEXT("%d panels more than 3 cm off the ground drawn under them (worst %.0f mm), %d with nothing drawn under them"), Off, Worst * 1000, Lost);
  Note(Off == 0 && Lost == 0, TEXT("feet: ") + Feet);
 }
 else Each.Add(TEXT("  -- feet: ") + Feet);
 // the gate leaf: a child of the hinge, where it was put on it, swinging with it
 FString Gate = TEXT("the old leaf (no kit gate)");
 if (S->Hinge != INDEX_NONE)
 {
  const UStaticMeshComponent* G = S->Gate.Get();
  const FS47WebNode* H = W->Node(S->Hinge);
  const USceneComponent* HingeComp = H ? H->Comp.Get() : nullptr;
  int32 Old = 0;
  for (const int32 I : S->OldLeaf) Old += Retired(W, I);
  const bool bChild = G && HingeComp && G->GetAttachParent() == HingeComp;
  const bool bPut = bChild && G->GetRelativeTransform().Equals(S->GateRelative, .01);
  const bool bFollows = bChild && G->GetComponentTransform().Equals(S->GateRelative * HingeComp->GetComponentTransform(), .5);
  Gate = FString::Printf(TEXT("%s: %s the hinge, %s, %s; old leaf parts hidden %d of %d (the latch and chain stay)"), G ? *G->GetStaticMesh()->GetName() : TEXT("MISSING"),
   bChild ? TEXT("a child of") : TEXT("NOT a child of"), bPut ? TEXT("where it was put") : TEXT("MOVED on it"), bFollows ? TEXT("swinging with it") : TEXT("NOT following it"), Old, S->OldLeaf.Num());
  Note(bChild && bPut && bFollows && Old == S->OldLeaf.Num() && G->GetCollisionEnabled() == ECollisionEnabled::NoCollision, TEXT("gate: ") + Gate);
 }
 Lines.Add(FString::Printf(TEXT("Array fence: %d panels in %d runs, %d closing posts, %d strand pieces, gate %s; old planes hidden %d/%d, old posts cut %d/%d; %d wrong"),
  Panels ? Panels->GetInstanceCount() : 0, S->Counts.Num(), Posts ? Posts->GetInstanceCount() : 0, Strands ? Strands->GetInstanceCount() : 0,
  S->Gate.IsValid() ? TEXT("the kit's leaf") : TEXT("the old leaf"), Hidden, RunCount, Taken, S->PostBoxes.Num(), Bad));
 Lines.Append(Each);
 return Bad;
}

bool SelfTest(TArray<FString>& Lines)
{
 bool bOk = true;
 const auto Check = [&Lines, &bOk](bool bGood, const FString& What) { Lines.Add(FString::Printf(TEXT("%s array fence: %s"), bGood ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bGood; };
 int32 Count = 0; double Stretch = 0;
 Layout(51.4, 2.5, Count, Stretch);
 Check(Count == 21 && FMath::IsNearlyEqual(Stretch, 51.4 / 52.5, 1e-9), FString::Printf(TEXT("51.4 m of 2.5 m panels: %d, stretched %.4f (21, 0.9790)"), Count, Stretch));
 Layout(1, 2.5, Count, Stretch);
 Check(Count == 1 && FMath::IsNearlyEqual(Stretch, .4, 1e-9), TEXT("a run shorter than a panel takes one, squeezed to fit"));
 Layout(10, 0, Count, Stretch);
 Check(Count == 0 && FMath::IsNearlyEqual(Stretch, 1.), TEXT("no panel length, no panels"));
 // the three runs at every panel length from 2 to 3.6 m: the whole run covered, each panel within 10 % of its modelled length
 int32 Worse = 0; double Worst = 0;
 for (int32 Cm = 200; Cm <= 360; Cm += 10)
  for (const FRun& Run : Runs)
  {
   const double Length = Run.X1 - Run.X0, Module = Cm / 100.;
   Layout(Length, Module, Count, Stretch);
   Worse += !(Count >= 1 && FMath::Abs(Count * Module * Stretch - Length) < 1e-6 && FMath::Abs(Stretch - 1) <= .1);
   Worst = FMath::Max(Worst, FMath::Abs(Stretch - 1));
  }
 Check(Worse == 0, FString::Printf(TEXT("the three runs at panel lengths 2.0 to 3.6 m: %d layouts off, the most any panel stretches %.1f %%"), Worse, Worst * 100));
 const TArray<FBox> Boxes = OldPosts();
 bool bApart = true;
 for (const FBox& B : Boxes) bApart &= !(B.GetCenter().X > -28.6 + .01 && B.GetCenter().X < -19.4 - .01) && !(B.GetCenter().X > GateX0 - .8 && B.GetCenter().X < GateX1 + .8);
 Check(Boxes.Num() == 22 && bApart, FString::Printf(TEXT("%d old posts to cut (20 every 7 m and the two by the highway), none in the highway's opening or by the gate"), Boxes.Num()));
 UStaticMesh* PanelMesh = Kit(TEXT("SM_MilitaryBase_WiredFence_b"));
 UStaticMesh* PostMesh = Kit(TEXT("SM_MilitaryBase_WiredFence_EndCap_c"));
 if (!PanelMesh || !PostMesh) { Lines.Add(TEXT("array fence: the Military Barriers kit is not imported; its frame skipped (the old fence stands)")); return bOk; }
 const double Half = PostMesh->GetBoundingBox().Max.X;
 Check(PostFrame(PostMesh) && PanelFrame(PanelMesh, Half), FString::Printf(TEXT("the kit's frame: post %s, panel %s"), *Bounds(PostMesh), *Bounds(PanelMesh)));
 Check(Instancing(PanelMesh) && Instancing(PostMesh), TEXT("the panel's and post's materials are compiled for instancing"));
 const double Module = ThreeBox(PanelMesh).Max.X;
 FString Each; int32 Off = 0;
 for (const FRun& Run : Runs)
 {
  Layout(Run.X1 - Run.X0, Module, Count, Stretch);
  Off += Count < 1 || FMath::Abs(Stretch - 1) > .1;
  Each += FString::Printf(TEXT(" %d x %.3f"), Count, Stretch);
 }
 Check(Off == 0, FString::Printf(TEXT("the kit's %.3f m panel along the three runs:%s"), Module, *Each));
 return bOk;
}
}
