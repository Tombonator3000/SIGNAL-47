#include "S47HorrorThreat.h"
#include "S47TripoCreature.h"
#include "S47ThreatNavigation.h"
#include "CollisionShape.h"

#include "S47Audio.h"
#include "S47Flow.h"
#include "S47Main.h"
#include "S47Seen.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Misc/ScopeExit.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47HorrorThreat, Log, All);

namespace
{
 constexpr double WarnTime = 1.6, WarnMin = .8, SeeTime = .35, SeenDwell = .35, LoseTime = 2.2, SearchTime = 9.0;
 constexpr double BodyRadius = .28;
 constexpr double CaptureReach = .9, CaptureHold = .7, WalkSpeed = 1.25, ChaseSpeed = 2.1; // walker 1.9, running 3.04
 constexpr double SightRange = 20, SightCos = .342, ViewRange = 40; // 70 degrees either side of the head
 constexpr double WalkReach = 11, RunReach = 20, EngineReach = 45, DriveLost = 20, DriveLoseTime = 2.0;
 constexpr double HeadRate = 2.6, BodyRate = 2.8, ReWarn = 6, WarnGap = 3, Stride = 1.5;
 // Closed yard: the fence collider of Config/Props/substation.json. The gates are part of it.
 constexpr double YardX0 = 7880, YardX1 = 7900, YardZ0 = 529, YardZ1 = 544.2;
 // Measured front colliders: substation.json control block, S47SubstationFollowup culvert and band post.
 struct FThreatBlock { double X0, X1, Z0, Z1; };
 const FThreatBlock FrontBlocks[] = {{7884.25, 7885.75, 527.26, 528.16}, {7881.15, 7882.85, 527.4, 528.4}, {7880.26, 7880.44, 527.56, 527.74}};
 enum : int32 { NFront, NTrack, NPost, NWestApron, NWestCorner, NWestMid, NShed, NNorthWest, NNorth, NNorthEast, NEastMid, NEastCorner, NEastApron, NWestTrack, NEastTrack, NSouth };
 bool ThreatNativeBounds(UPrimitiveComponent* Part, int32 Instance, FBox& Out)
 {
  if (!Part || !Part->IsQueryCollisionEnabled() || Part->GetCollisionResponseToChannel(ECC_Visibility) != ECR_Block) return false;
  if (const auto* Instances = Cast<UInstancedStaticMeshComponent>(Part))
  {
   FTransform Pose;
   if (!Instances->GetStaticMesh() || !Instances->GetInstanceTransform(Instance, Pose, true)) return false;
   Out = Instances->GetStaticMesh()->GetBoundingBox().TransformBy(Pose.ToMatrixWithScale());
  }
  else Out = Part->Bounds.GetBox();
  return Out.IsValid != 0;
 }
 double ThreatWrap(double A)
 {
  if (!FMath::IsFinite(A)) return 0;
  while (A > PI) A -= 2 * PI;
  while (A < -PI) A += 2 * PI;
  return A;
 }
 double ThreatYaw(const FVector2D& D) { return FMath::Atan2(D.Y, D.X); } // UE yaw: three x/z is UE x/y
 // Liang-Barsky: does segment A-B touch the rectangle?
 bool ThreatSegment(const FVector2D& A, const FVector2D& B, double X0, double X1, double Z0, double Z1)
 {
  double T0 = 0, T1 = 1; const FVector2D D = B - A;
  auto Clip = [&T0, &T1](double P, double Q)
  {
   if (FMath::Abs(P) < 1e-12) return Q >= 0;
   const double R = Q / P;
   if (P < 0) { if (R > T1) return false; if (R > T0) T0 = R; }
   else { if (R < T0) return false; if (R < T1) T1 = R; }
   return true;
  };
  return Clip(-D.X, A.X - X0) && Clip(D.X, X1 - A.X) && Clip(-D.Y, A.Y - Z0) && Clip(D.Y, Z1 - A.Y);
 }
}

struct FS47HorrorThreat::FParts
{
 FS47Obj Root;
 TWeakObjectPtr<UBoxComponent> RootComp;
 TWeakObjectPtr<UStaticMeshComponent> Torso, Head, ArmL, ForeL, ArmR, ForeR, ThighL, ShinL, ThighR, ShinR;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> All;
 FS47TripoCreature Creature;
 bool bCollide = true;
};

FS47HorrorThreat::FS47HorrorThreat(US47Main* InM) : Parts(MakeUnique<FParts>()), M(InM)
{
 NodeOk.Init(true, Nodes().Num()); EdgeOk.Init(true, Edges().Num());
}
FS47HorrorThreat::~FS47HorrorThreat() = default;

const TArray<FS47HorrorThreat::FNode>& FS47HorrorThreat::Nodes()
{
 static const TArray<FNode> N = {
  {TEXT("front"), FVector2D(7885.0, 525.8)},      // measured T01 standpoint
  {TEXT("track"), FVector2D(7885.0, 520.8)},      // measured service-track approach
  {TEXT("post"), FVector2D(7889.0, 519.0)},       // open crossing between the front and the truck
  {TEXT("westApron"), FVector2D(7880.6, 526.3)},  // south of the culvert and band post
  {TEXT("westCorner"), FVector2D(7878.5, 527.4)},
  {TEXT("westMid"), FVector2D(7878.5, 535.0)},
  {TEXT("shed"), FVector2D(7878.5, 541.1)},       // shed cover, local checkpoint
  {TEXT("northWest"), FVector2D(7878.5, 545.8)},
  {TEXT("north"), FVector2D(7890.0, 545.8)},
  {TEXT("northEast"), FVector2D(7901.5, 545.8)},
  {TEXT("eastMid"), FVector2D(7901.5, 535.0)},
  {TEXT("eastCorner"), FVector2D(7901.5, 527.4)},
  {TEXT("eastApron"), FVector2D(7894.0, 526.4)},
  {TEXT("westTrack"), FVector2D(7878.5, 519.0)},
  {TEXT("eastTrack"), FVector2D(7901.5, 519.0)},
  {TEXT("south"), FVector2D(7885.0, 512.0)}};
 return N;
}

const TArray<TPair<int32, int32>>& FS47HorrorThreat::Edges()
{
 static const TArray<TPair<int32, int32>> E = {
  {NFront, NTrack}, {NFront, NWestApron}, {NFront, NEastApron}, {NTrack, NPost}, {NTrack, NWestTrack}, {NTrack, NSouth},
  {NPost, NSouth}, {NPost, NEastTrack}, {NPost, NEastApron},
  {NWestApron, NWestCorner}, {NWestCorner, NWestMid}, {NWestMid, NShed}, {NShed, NNorthWest}, {NNorthWest, NNorth},
  {NNorth, NNorthEast}, {NNorthEast, NEastMid}, {NEastMid, NEastCorner}, {NEastCorner, NEastApron},
  {NWestCorner, NWestTrack}, {NWestTrack, NSouth}, {NEastCorner, NEastTrack}, {NEastTrack, NSouth}};
 return E;
}

const TCHAR* FS47HorrorThreat::StateName(EState In)
{
 switch (In)
 {
 case EState::Idle: return TEXT("idle");
 case EState::Warn: return TEXT("warn");
 case EState::Investigate: return TEXT("investigate");
 case EState::Search: return TEXT("search");
 case EState::Chase: return TEXT("chase");
 case EState::Capture: return TEXT("capture");
 case EState::Withdrawn: return TEXT("withdrawn");
 }
 return TEXT("idle");
}

bool FS47HorrorThreat::InYard(const FVector2D& P, double Pad)
{
 return P.X > YardX0 - Pad && P.X < YardX1 + Pad && P.Y > YardZ0 - Pad && P.Y < YardZ1 + Pad;
}

bool FS47HorrorThreat::Build()
{
 if (bBuilt) return true;
 if (bBuildFailed) return false;
 if (!M || !M->Web || !M->Web->IsLoaded()) return false;
 AS47Web* W = M->Web.Get();
 const FS47Obj Group = W->Obj(TEXT("world.legs.road.area.group"));
 const FS47WebNode* GroupNode = Group ? W->Node(Group.I) : nullptr;
 USceneComponent* GroupComp = GroupNode ? GroupNode->Comp.Get() : nullptr;
 auto Load = [](const TCHAR* Name) { return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Signal47/HorrorSC02/%s.%s"), Name, Name)); };
 UStaticMesh* TorsoMesh = Load(TEXT("SM_SC02_PaleTorso"));
 UStaticMesh* HeadMesh = Load(TEXT("SM_SC02_PaleHead"));
 UStaticMesh* UpperArmMesh = Load(TEXT("SM_SC02_PaleUpperArm"));
 UStaticMesh* LowerArmMesh = Load(TEXT("SM_SC02_PaleLowerArm"));
 UStaticMesh* ThighMesh = Load(TEXT("SM_SC02_PaleThigh"));
 UStaticMesh* ShinMesh = Load(TEXT("SM_SC02_PaleShin"));
 if (!GroupComp || !TorsoMesh || !HeadMesh || !UpperArmMesh || !LowerArmMesh || !ThighMesh || !ShinMesh)
 {
  bBuildFailed = true;
  UE_LOG(LogS47HorrorThreat, Error, TEXT("S47: SC02 pale body or Caliche road group unavailable; SC02 disabled, existing story continues"));
  return false;
 }
 UBoxComponent* RootComp = NewObject<UBoxComponent>(W, TEXT("SC02ThreatRoot"));
 RootComp->SetMobility(EComponentMobility::Movable); RootComp->SetupAttachment(GroupComp);
 RootComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); RootComp->SetBoxExtent(FVector(1)); RootComp->RegisterComponent();
 const int32 RootNode = W->AdoptNode(RootComp, Group.I, TEXT("sc02:threatRoot"));
 Parts->Root = W->ObjAt(RootNode); Parts->RootComp = RootComp;
 // Contract joints (cm, feet root, facing +X): hip 102, neck 170, shoulder (0,-+21,160), elbow -36, hip joint (0,-+10,100), knee -49.
 // The right side is the left mesh mirrored (scale Y -1); pitch about Y is unchanged by that mirror.
 auto Part = [&](const TCHAR* Name, UStaticMesh* Mesh, USceneComponent* Parent, int32 ParentNode, const FVector& At, const FVector& Scale, int32& Node)
 {
  UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, Name);
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(Parent); C->SetStaticMesh(Mesh);
  C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore);
  C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); C->SetCastShadow(true);
  C->SetRelativeLocation(At); C->SetRelativeScale3D(Scale); C->RegisterComponent();
  Node = W->AdoptNode(C, ParentNode, FString(TEXT("sc02:")) + Name);
  Parts->All.Add(C);
  return C;
 };
 const FVector One(1, 1, 1), Mirror(1, -1, 1);
 int32 TorsoNode = -1, ArmLNode = -1, ArmRNode = -1, ThighLNode = -1, ThighRNode = -1, Leaf = -1;
 UStaticMeshComponent* Torso = Part(TEXT("SC02PaleTorso"), TorsoMesh, RootComp, RootNode, FVector(0, 0, 102), One, TorsoNode);
 Parts->Torso = Torso;
 Parts->Head = Part(TEXT("SC02PaleHead"), HeadMesh, Torso, TorsoNode, FVector(7.2, 0, 68), One, Leaf); // neck sits forward of the hunched spine (model_contract.json)
 UStaticMeshComponent* ArmL = Part(TEXT("SC02PaleUpperArmL"), UpperArmMesh, Torso, TorsoNode, FVector(4.6, -21, 58), One, ArmLNode);
 Parts->ArmL = ArmL;
 Parts->ForeL = Part(TEXT("SC02PaleLowerArmL"), LowerArmMesh, ArmL, ArmLNode, FVector(0, 0, -36), One, Leaf);
 UStaticMeshComponent* ArmR = Part(TEXT("SC02PaleUpperArmR"), UpperArmMesh, Torso, TorsoNode, FVector(4.6, 21, 58), Mirror, ArmRNode);
 Parts->ArmR = ArmR;
 Parts->ForeR = Part(TEXT("SC02PaleLowerArmR"), LowerArmMesh, ArmR, ArmRNode, FVector(0, 0, -36), One, Leaf);
 UStaticMeshComponent* ThighL = Part(TEXT("SC02PaleThighL"), ThighMesh, RootComp, RootNode, FVector(0, -10, 100), One, ThighLNode);
 Parts->ThighL = ThighL;
 Parts->ShinL = Part(TEXT("SC02PaleShinL"), ShinMesh, ThighL, ThighLNode, FVector(0, 0, -49), One, Leaf);
 UStaticMeshComponent* ThighR = Part(TEXT("SC02PaleThighR"), ThighMesh, RootComp, RootNode, FVector(0, 10, 100), Mirror, ThighRNode);
 Parts->ThighR = ThighR;
 Parts->ShinR = Part(TEXT("SC02PaleShinR"), ShinMesh, ThighR, ThighRNode, FVector(0, 0, -49), One, Leaf);
 if (Parts->Creature.Build(W,RootComp,TEXT("SC02TripoSkin")))
  for (const auto& Old:Parts->All) if (auto* C=Old.Get())
  { C->SetRenderInMainPass(false); C->SetRenderInDepthPass(false); C->SetVisibleInRayTracing(false); C->SetCastShadow(false); }
 bBuilt = true; Apply(false);
 UE_LOG(LogS47HorrorThreat, Display, TEXT("S47: SC02 pale body ready: 10 parts from six original meshes under the Caliche road group"));
 return true;
}

void FS47HorrorThreat::RefreshNavigation()
{
 bWorld = M && M->Player && M->Web;
 if (!bWorld) return;
 Near.Reset(); NativeHints.Reset();
 const FVector2D C = SiteCentre();
 for (const FS47Rect& R : M->Web->Colliders())
  if (R.bEnabled && R.Id != TEXT("truck") && R.Id != TEXT("personalCar") && R.MaxX > C.X - 90 && R.MinX < C.X + 90 && R.MaxZ > C.Y - 90 && R.MinZ < C.Y + 90) Near.Add(R);
 // Conservative current bounds, rather than a parked vehicle's old exported rectangle.
 const auto Vehicle = [this](const TOptional<AS47Walker::FTruckBox>& Box, const TCHAR* Id)
 {
  if (!Box || Box->Area != M->Web->Area()) return;
  const double Co = FMath::Abs(FMath::Cos(Box->Heading)), Si = FMath::Abs(FMath::Sin(Box->Heading));
  const double X = Co * Box->HalfW + Si * Box->HalfL, Z = Si * Box->HalfW + Co * Box->HalfL;
  Near.Add({Box->X - X, Box->X + X, Box->Z - Z, Box->Z + Z, Id, true});
 };
 if (M->World && M->World->bDriving)
 {
  if (const FS47Truck* Cab = M->World->CabTruck())
  { AS47Walker::FTruckBox Box; Box.Area = M->Web->Area(); Box.X = Cab->Position.X; Box.Z = Cab->Position.Z; Box.Heading = Cab->Rotation.Y; Vehicle(Box, TEXT("threat:truck")); }
 }
 else Vehicle(M->Player->TruckBox, TEXT("threat:truck"));
 Vehicle(M->Player->PersonalCarBox, TEXT("threat:personalCar"));
 // Native AABBs may contain empty space around rotated or concave geometry. They supply
 // live corner candidates and a broad phase, never a hard rectangle enclosing the body.
 NativeObstacles.RemoveAll([](const FNativeObstacle& Obstacle)
 { FBox Box; return !ThreatNativeBounds(Obstacle.Part.Get(), Obstacle.Instance, Box); });
 for (const FNativeObstacle& Obstacle : NativeObstacles)
 {
  FBox Box;
  if (!ThreatNativeBounds(Obstacle.Part.Get(), Obstacle.Instance, Box)) continue;
  const FVector A = S47Three::ToThree(Box.Min), B = S47Three::ToThree(Box.Max);
  NativeHints.Add({FMath::Min(A.X, B.X), FMath::Max(A.X, B.X), FMath::Min(A.Z, B.Z), FMath::Max(A.Z, B.Z), TEXT("threat:nativeHint"), true});
 }
}

void FS47HorrorThreat::Validate()
{
 const int32 Count = Nodes().Num();
 NodeOk.Init(true, Count); EdgeOk.Init(true, Edges().Num());
 RefreshNavigation();
 for (int32 I = 0; I < Count; ++I) NodeOk[I] = Clear(Nodes()[I].At, Nodes()[I].At);
 for (int32 I = 0; I < Edges().Num(); ++I)
 {
  const auto& E = Edges()[I];
  EdgeOk[I] = NodeOk[E.Key] && NodeOk[E.Value] && Clear(Nodes()[E.Key].At, Nodes()[E.Value].At);
 }
 TOptional<FVector2D> Truck;
 if (bWorld && M->World) if (const FS47Truck* T = M->World->TruckHere()) Truck = FVector2D(T->Position.X, T->Position.Z);
 PostNode = NPost;
 for (const int32 I : {NPost, NSouth, NTrack}) if (NodeOk[I] && (!Truck || FVector2D::Distance(*Truck, Nodes()[I].At) >= 4.5)) { PostNode = I; break; }
 CheckNode = NFront;
 for (const int32 I : {NShed, NWestMid, NWestCorner, NFront}) if (NodeOk[I]) { CheckNode = I; break; }
 const FVector2D Away = Truck ? *Truck : Nodes()[NFront].At;
 RetreatNode = NNorth; double Far = -1;
 for (const int32 I : {NNorth, NNorthWest, NNorthEast}) if (NodeOk[I] && FVector2D::Distance(Away, Nodes()[I].At) > Far) { Far = FVector2D::Distance(Away, Nodes()[I].At); RetreatNode = I; }
 Y = bWorld ? M->Player->FloorY : -1.1;
 TArray<bool> West = NodeOk, East = NodeOk;
 for (const int32 I : {NNorthWest, NNorth, NNorthEast, NEastMid, NEastCorner, NEastApron, NEastTrack}) West[I] = false;
 for (const int32 I : {NWestMid, NWestCorner, NWestApron, NWestTrack, NFront, NTrack}) East[I] = false;
 int32 Ok = 0; for (const bool B : NodeOk) Ok += B ? 1 : 0;
 UE_LOG(LogS47HorrorThreat, Display, TEXT("S47: SC02 route graph %d/%d points walkable; route A west %s, route B east %s; post %s, checkpoint %s, %d site colliders"),
  Ok, Count, Route(CheckNode, NSouth, West, &EdgeOk).Num() ? TEXT("open") : TEXT("BLOCKED"), Route(CheckNode, NSouth, East, &EdgeOk).Num() ? TEXT("open") : TEXT("BLOCKED"),
  Nodes()[PostNode].Id, Nodes()[CheckNode].Id, Near.Num());
}

FVector2D FS47HorrorThreat::Checkpoint() const { return Nodes()[CheckNode].At; }
FString FS47HorrorThreat::CheckpointId() const { return Nodes()[CheckNode].Id; }

void FS47HorrorThreat::Start(bool bInSeen)
{
 bActive = true; bSeen = bInSeen; Validate(); Reset();
 UE_LOG(LogS47HorrorThreat, Display, TEXT("S47: SC02 meeting armed at %s; forewarning %s"), Nodes()[PostNode].Id, bSeen ? TEXT("already witnessed") : TEXT("not yet witnessed"));
}

void FS47HorrorThreat::Reset()
{
 bAppeared = bGone = bCaptureEvent = bChaseAfter = bBreath = bGrounded = bHavePlayer = false;
 S = EState::Idle; StateT = SeeAcc = SeenAcc = LostT = DriveLostT = Replan = Phase = Gait = NoiseCool = ChaseBreath = BlockedFor = 0;
 SinceSeen = SinceWarn = 1e9; SearchStep = 0; Path.Reset();
 Pos = Goal = LastKnown = Look = PathGoal = SearchAt = Nodes()[PostNode].At;
 BodyYaw = HeadYaw = -.5 * PI; // facing the track and SARO 07, away from the front
 Apply(false);
}

void FS47HorrorThreat::Stop() { bActive = false; Path.Reset(); Apply(false); }

void FS47HorrorThreat::Enter(EState Next) { S = Next; StateT = 0; LostT = 0; SearchStep = 0; Path.Reset(); Replan = BlockedFor = 0; }

void FS47HorrorThreat::Warn(const FVector2D& At, bool bChase)
{
 Look = At; bChaseAfter = bChase; SinceWarn = 0; bBreath = true; Enter(EState::Warn);
}

void FS47HorrorThreat::TurnBody(double Yaw, double Rate, double Dt)
{
 BodyYaw = ThreatWrap(BodyYaw + FMath::Clamp(ThreatWrap(Yaw - BodyYaw), -Rate * Dt, Rate * Dt));
}

bool FS47HorrorThreat::InCollider(const FVector2D& P, double Pad) const
{
 for (const FS47Rect& R : Near) if (R.bEnabled && P.X > R.MinX - Pad && P.X < R.MaxX + Pad && P.Y > R.MinZ - Pad && P.Y < R.MaxZ + Pad) return true;
 return false;
}

bool FS47HorrorThreat::Sampled(const FVector2D& A, const FVector2D& B) const
{
 if (!bWorld || !M || !M->Player) return true;
 const double L = FVector2D::Distance(A, B);
 const int32 Count = FMath::Max(1, FMath::CeilToInt(L / .25));
 // The start is where the body already stands; every later sample must be walkable ground.
 for (int32 I = 1; I <= Count; ++I)
 {
  const FVector2D P = A + (B - A) * (double(I) / Count);
  if (!M->Player->Walkable(P.X, P.Y) || InCollider(P, BodyRadius)) return false;
 }
 return true;
}

bool FS47HorrorThreat::Clear(const FVector2D& A, const FVector2D& B) const
{
 if (A.ContainsNaN() || B.ContainsNaN()) return false;
 if (ThreatSegment(A, B, YardX0 - BodyRadius, YardX1 + BodyRadius, YardZ0 - BodyRadius, YardZ1 + BodyRadius)) return false;
 for (const FThreatBlock& K : FrontBlocks)
  if (ThreatSegment(A, B, K.X0 - BodyRadius, K.X1 + BodyRadius, K.Z0 - BodyRadius, K.Z1 + BodyRadius)) return false;
 // Exact segment tests also catch narrow posts between the walkability samples.
 for (const FS47Rect& R : Near) if (R.bEnabled && ThreatSegment(A, B, R.MinX - BodyRadius, R.MaxX + BodyRadius, R.MinZ - BodyRadius, R.MaxZ + BodyRadius)) return false;
 return Sampled(A, B);
}

TArray<int32> FS47HorrorThreat::Route(int32 From, int32 To, const TArray<bool>& Allowed, const TArray<bool>* EdgeMask)
{
 const auto& N = Nodes(); const auto& E = Edges();
 if (!N.IsValidIndex(From) || !N.IsValidIndex(To) || Allowed.Num() != N.Num() || !Allowed[From] || !Allowed[To]) return {};
 TArray<double> Cost; Cost.Init(1e18, N.Num());
 TArray<int32> Prev; Prev.Init(INDEX_NONE, N.Num());
 TArray<bool> Done; Done.Init(false, N.Num());
 Cost[From] = 0;
 for (;;)
 {
  int32 U = INDEX_NONE;
  for (int32 I = 0; I < N.Num(); ++I) if (!Done[I] && Cost[I] < 1e17 && (U == INDEX_NONE || Cost[I] < Cost[U])) U = I;
  if (U == INDEX_NONE || U == To) break;
  Done[U] = true;
  for (int32 K = 0; K < E.Num(); ++K)
  {
   if (EdgeMask && (!EdgeMask->IsValidIndex(K) || !(*EdgeMask)[K])) continue;
   const int32 V = E[K].Key == U ? E[K].Value : E[K].Value == U ? E[K].Key : INDEX_NONE;
   if (V == INDEX_NONE || Done[V] || !Allowed[V]) continue;
   const double C = Cost[U] + FVector2D::Distance(N[U].At, N[V].At);
   if (C < Cost[V]) { Cost[V] = C; Prev[V] = U; }
  }
 }
 TArray<int32> Out;
 if (Cost[To] >= 1e17) return Out;
 for (int32 I = To; I != INDEX_NONE; I = Prev[I]) Out.Insert(I, 0);
 return Out;
}

TArray<FVector2D> FS47HorrorThreat::Plan(const FVector2D& From, const FVector2D& To) const
{
 TArray<FVector2D> Seeds;
 for (const FNode& N : Nodes()) Seeds.Add(N.At);
 const double CornerPad = BodyRadius + .02;
 for (const FS47Rect& H : NativeHints)
 {
  Seeds.Add(FVector2D(H.MinX - CornerPad, H.MinZ - CornerPad));
  Seeds.Add(FVector2D(H.MinX - CornerPad, H.MaxZ + CornerPad));
  Seeds.Add(FVector2D(H.MaxX + CornerPad, H.MinZ - CornerPad));
  Seeds.Add(FVector2D(H.MaxX + CornerPad, H.MaxZ + CornerPad));
 }
 TArray<FS47Rect> Blocks = Near;
 Blocks.Add({YardX0, YardX1, YardZ0, YardZ1, TEXT("threat:yard"), true});
 for (const FThreatBlock& B : FrontBlocks) Blocks.Add({B.X0, B.X1, B.Z0, B.Z1, TEXT("threat:front"), true});
 return FS47ThreatNavigation::FindPath(From, To, Seeds, Blocks, BodyRadius,
  [this](const FVector2D& A, const FVector2D& B)
  {
   if (!Clear(A, B)) return false;
   // Only edges near a learned native shape need an expensive world query. A point
   // inside its AABB remains valid when the actual capsule occupies empty space.
   for (const FS47Rect& H : NativeHints)
    if (H.bEnabled && ThreatSegment(A, B, H.MinX - BodyRadius, H.MaxX + BodyRadius, H.MinZ - BodyRadius, H.MaxZ + BodyRadius))
     return PhysicalSegment(A, B);
   return true;
  });
}

bool FS47HorrorThreat::PhysicalSegment(const FVector2D& From, const FVector2D& To, FHitResult* OutHit) const
{
 if (OutHit) *OutHit = FHitResult();
 if (From.ContainsNaN() || To.ContainsNaN() || !FMath::IsFinite(Y)) return false;
 if (!bWorld || !M || !M->Web || !M->Web->GetWorld()) return true;
 const double FromY = Ground(From, false), ToY = Ground(To, false);
 if (!FMath::IsFinite(FromY) || !FMath::IsFinite(ToY)) return false;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47ThreatSegment), true, M->Player);
 for (const auto& P : Parts->All) if (const UPrimitiveComponent* C = P.Get()) Q.AddIgnoredComponent(C);
 const FVector A = S47Three::ToUE(From.X, FromY + .99, From.Y);
 const FVector B = S47Three::ToUE(To.X, ToY + .99, To.Y);
 const FCollisionShape Capsule = FCollisionShape::MakeCapsule(float(BodyRadius * 100), 94.f);
 FHitResult Hit;
 if (!M->Web->GetWorld()->SweepSingleByChannel(Hit, A, B, FQuat::Identity, ECC_Visibility, Capsule, Q)) return true;
 // A bounded full step-up retry also handles low initial floor overlaps, whose
 // hit point/normal are not a reliable measure of step height. Tall barriers
 // still intersect the entire lifted capsule sweep (native probe covers both).
 {
  // Low step faces can have horizontal normals. Give them one lifted recheck;
  // a tall wall must still block that complete sweep. The same mesh may contain both
  // ground and walls, so neither the component nor its other instances are ignored.
  const FVector Lift(0, 0, 30);
  if (!M->Web->GetWorld()->SweepSingleByChannel(Hit, A + Lift, B + Lift, FQuat::Identity, ECC_Visibility, Capsule, Q)) return true;
 }
 if (OutHit) *OutHit = Hit;
 return false;
}

bool FS47HorrorThreat::PhysicalStep(const FVector2D& To)
{
 FHitResult Hit;
 if (PhysicalSegment(Pos, To, &Hit)) return true;
 // Planning is read-only. Only an actually attempted movement teaches a native shape.
 if (UPrimitiveComponent* Part = Hit.GetComponent())
 {
  const int32 Instance = Cast<UInstancedStaticMeshComponent>(Part) ? Hit.Item : INDEX_NONE;
  if (!NativeObstacles.ContainsByPredicate([Part, Instance](const FNativeObstacle& Obstacle) { return Obstacle.Part.Get() == Part && Obstacle.Instance == Instance; }))
  {
   NativeObstacles.Add({Part, Instance});
   FBox Bounds;
   if (ThreatNativeBounds(Part, Instance, Bounds))
   {
    const FVector Contact = S47Three::ToThree(Hit.ImpactPoint);
    UE_LOG(LogS47HorrorThreat, Display, TEXT("S47: native block detail foot %.3f ground %.3f contact %.3f normal %.3f %.3f %.3f initial %d depth %.3fcm traceZ %.3f..%.3f"),
     Y, Ground(Pos, false), Contact.Y, Hit.ImpactNormal.X, Hit.ImpactNormal.Y, Hit.ImpactNormal.Z, Hit.bStartPenetrating ? 1 : 0, Hit.PenetrationDepth, Hit.TraceStart.Z / 100, Hit.TraceEnd.Z / 100);
    const FVector A = S47Three::ToThree(Bounds.Min), B = S47Three::ToThree(Bounds.Max);
    UE_LOG(LogS47HorrorThreat, Display, TEXT("S47: native movement obstacle %s instance %d x %.2f..%.2f z %.2f..%.2f learned at %.2f %.2f; bounds are planning hints"),
     *Part->GetName(), Instance, FMath::Min(A.X, B.X), FMath::Max(A.X, B.X), FMath::Min(A.Z, B.Z), FMath::Max(A.Z, B.Z), Pos.X, Pos.Y);
   }
  }
 }
 Path.Reset(); Replan = 0;
 return false;
}

bool FS47HorrorThreat::ContactClear(const FVector2D& To) const
{
 // A hand/chest corridor, not a full body at the player's centre. Their legal wall
 // clearance is slightly smaller than ours; that must not make a wall-side player immune.
 if (ThreatSegment(Pos, To, YardX0, YardX1, YardZ0, YardZ1)) return false;
 for (const FThreatBlock& B : FrontBlocks) if (ThreatSegment(Pos, To, B.X0, B.X1, B.Z0, B.Z1)) return false;
 for (const FS47Rect& R : Near) if (R.bEnabled && ThreatSegment(Pos, To, R.MinX, R.MaxX, R.MinZ, R.MaxZ)) return false;
 if (!bWorld || !M || !M->Web || !M->Web->GetWorld()) return true;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47ThreatContact), true, M->Player);
 for (const auto& P : Parts->All) if (const UPrimitiveComponent* C = P.Get()) Q.AddIgnoredComponent(C);
 FHitResult Hit;
 const FVector A = S47Three::ToUE(Pos.X, Y + 1.15, Pos.Y);
 const FVector B = S47Three::ToUE(To.X, M->Player->FloorY + 1.15, To.Y);
 return !M->Web->GetWorld()->SweepSingleByChannel(Hit, A, B, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(10.f), Q);
}

bool FS47HorrorThreat::Move(const FVector2D& Requested, double Speed, double Dt, double StopDist, bool bVehicle)
{
 RefreshNavigation();
 FVector2D Target = Requested;
 if (!Clear(Target, Target))
 {
  // A cab centre, or a valid player just inside our wider clearance: approach a
  // tested free point. This is an explicit reachable goal, never a partial path.
  double Best = 1e18;
  for (const FS47Rect& R : Near)
  {
   if (!R.bEnabled || (bVehicle && R.Id != TEXT("threat:truck"))) continue;
   const double Margin = BodyRadius + .02;
   const FVector2D Anchor = bVehicle ? Pos : Requested;
   for (const FVector2D& At : {FVector2D(R.MinX - Margin, FMath::Clamp(Anchor.Y, R.MinZ - Margin, R.MaxZ + Margin)),
     FVector2D(R.MaxX + Margin, FMath::Clamp(Anchor.Y, R.MinZ - Margin, R.MaxZ + Margin)),
     FVector2D(FMath::Clamp(Anchor.X, R.MinX - Margin, R.MaxX + Margin), R.MinZ - Margin),
     FVector2D(FMath::Clamp(Anchor.X, R.MinX - Margin, R.MaxX + Margin), R.MaxZ + Margin)})
   {
    const double Cost = FVector2D::Distance(Pos, At);
    if (Cost < Best && (bVehicle || FVector2D::Distance(At, Requested) < CaptureReach * .75) && Clear(At, At) && !Plan(Pos, At).IsEmpty()) { Best = Cost; Target = At; }
   }
  }
  StopDist = bVehicle ? .15 : .05;
 }
 if (FVector2D::Distance(Pos, Target) <= StopDist && Clear(Pos, Target)) { BlockedFor = 0; return true; }
 Replan -= Dt;
 if (Path.IsEmpty() || FVector2D::Distance(PathGoal, Target) > .6 || Replan <= 0 || !Clear(Pos, Path[0]))
 { Path = Plan(Pos, Target); PathGoal = Target; Replan = .4; }
 if (Path.IsEmpty()) { BlockedFor += Dt; TurnBody(ThreatYaw(Target - Pos), BodyRate, Dt); return false; }
 // Drop a reached waypoint before steering, so a zero-distance first seed cannot stall us.
 while (Path.Num() > 1 && FVector2D::Distance(Pos, Path[0]) < .02) Path.RemoveAt(0);
 const bool bLast = Path.Num() == 1;
 const FVector2D D = Path[0] - Pos; const double L = D.Size();
 const double Room = bLast ? FMath::Max(0.0, L - StopDist) : L;
 const double Advance = FMath::Min(Room, Speed * Dt);
 if (L > 1e-4 && Advance > 1e-5)
 {
  const FVector2D Next = Pos + D * (Advance / L);
  if (!Clear(Pos, Next) || !PhysicalStep(Next)) { Path.Reset(); Replan = 0; BlockedFor += Dt; return false; }
  Pos = Next; BlockedFor = 0; TurnBody(ThreatYaw(D), BodyRate, Dt);
 }
 if (!bLast && L - Advance < .02) Path.RemoveAt(0);
 return FVector2D::Distance(Pos, Target) <= StopDist + 1e-3 && Clear(Pos, Target);
}

void FS47HorrorThreat::Think(const FSense& In, double Dt)
{
 if (!bActive || In.bHeld || !FMath::IsFinite(Dt) || Dt <= 0) return; // no saved-up approach
 Dt = FMath::Min(Dt, .1);
 if (!bAppeared)
 {
  if (!In.bCanAppear) return;
  bAppeared = true; Pos = Nodes()[PostNode].At; BodyYaw = HeadYaw = -.5 * PI; Enter(EState::Idle);
 }
 if (bGone) return;
 Clock += Dt; StateT += Dt; SinceWarn += Dt;
 const FVector2D Before = Pos;
 // Both sight facts are measured by the adapter; nothing here infers a view.
 SeeAcc = In.bSeesPlayer ? SeeAcc + Dt : 0;
 if (In.bSeesPlayer) { LastKnown = In.Player; SinceSeen = 0; } else SinceSeen += Dt;
 SeenAcc = S == EState::Warn && In.bPlayerSeesBody ? SeenAcc + Dt : 0;
 if (SeenAcc >= SeenDwell) bSeen = true;
 // The cab cannot be taken. Distance from the driven truck decides contact.
 if (In.bDriving && S != EState::Withdrawn)
 {
  DriveLostT = In.TruckDistance >= DriveLost ? DriveLostT + Dt : 0;
  if (DriveLostT >= DriveLoseTime)
  {
   double Far = -1;
   for (const int32 I : {NNorth, NNorthWest, NNorthEast}) if (NodeOk[I] && FVector2D::Distance(In.Player, Nodes()[I].At) > Far) { Far = FVector2D::Distance(In.Player, Nodes()[I].At); RetreatNode = I; }
   Enter(EState::Withdrawn);
  }
 }
 else DriveLostT = 0;
 double HeadGoal = BodyYaw;
 switch (S)
 {
 case EState::Idle:
  if (SeeAcc >= SeeTime) { Warn(In.Player, true); break; }
  if (In.Sound) { Warn(*In.Sound, false); break; }
  if (Move(Nodes()[PostNode].At, WalkSpeed * .8, Dt, .3)) TurnBody(-.5 * PI, 1.0, Dt);
  break;
 case EState::Warn:
  // The body stands still; the head turns first and the shoulders follow late.
  if (In.bSeesPlayer) Look = In.Player; else if (In.Sound && !bChaseAfter) Look = *In.Sound;
  HeadGoal = ThreatYaw(Look - Pos);
  if (StateT >= .6 && FMath::Abs(ThreatWrap(HeadGoal - BodyYaw)) > 1.3) TurnBody(HeadGoal, 1.2, Dt);
  if (bSeen && !In.bDriving && SeeAcc >= SeeTime && StateT >= WarnMin) { Enter(EState::Chase); break; }
  if (StateT >= WarnTime)
  {
   if (bSeen && (SinceSeen < 1 || In.bDriving)) { Enter(EState::Chase); break; }
   Goal = Look; Enter(EState::Investigate);
  }
  break;
 case EState::Investigate:
  if (SeeAcc >= SeeTime) { Warn(In.Player, true); break; }
  if (In.Sound && FVector2D::Distance(*In.Sound, Goal) > ReWarn && SinceWarn >= WarnGap) { Warn(*In.Sound, false); break; }
  if (In.Sound) Goal = *In.Sound;
  // A body whose forewarning was not witnessed never closes on the player.
  if (Move(Goal, WalkSpeed, Dt, bSeen ? .5 : 3.0)) { SearchAt = Goal; Enter(EState::Search); }
  else if (BlockedFor >= 2) { SearchAt = Pos; Enter(EState::Search); }
  break;
 case EState::Search:
  if (SeeAcc >= SeeTime) { Warn(In.Player, true); break; }
  if (In.Sound) { Warn(*In.Sound, false); break; }
  HeadGoal = BodyYaw + 1.1 * FMath::Sin(StateT * 1.4);
  if (SearchStep == 0 && StateT >= 3)
  {
   // Then the nearest route point to where contact ended, looked over again.
   int32 Best = INDEX_NONE; double Nearest = 1e9;
   for (int32 I = 0; I < Nodes().Num(); ++I)
   {
    const double D = FVector2D::Distance(Nodes()[I].At, SearchAt);
    if (NodeOk[I] && D > 1 && D < Nearest) { Nearest = D; Best = I; }
   }
   SearchStep = Best == INDEX_NONE ? 2 : 1;
   if (Best != INDEX_NONE) Goal = Nodes()[Best].At;
  }
  if (SearchStep == 1 && Move(Goal, WalkSpeed * .8, Dt, .3)) SearchStep = 2;
  if (StateT >= SearchTime) Enter(EState::Idle);
  break;
 case EState::Chase:
  if (In.bDriving) { HeadGoal = ThreatYaw(In.Player - Pos); Move(In.Player, ChaseSpeed, Dt, 2.2, true); break; }
  LostT = In.bSeesPlayer ? 0 : LostT + Dt;
  if (LostT >= LoseTime) { SearchAt = LastKnown; Enter(EState::Search); break; } // contact lost
  HeadGoal = ThreatYaw(LastKnown - Pos);
  Move(LastKnown, ChaseSpeed, Dt, .2);
  if (bSeen && In.bSeesPlayer && FVector2D::Distance(Pos, In.Player) <= CaptureReach && ContactClear(In.Player)) { Enter(EState::Capture); break; }
  if (BlockedFor >= 2) { SearchAt = Pos; Enter(EState::Search); break; }
  ChaseBreath -= Dt; if (ChaseBreath <= 0) { ChaseBreath = 2.2; bBreath = true; }
  break;
 case EState::Capture:
  if (In.bDriving) { Enter(EState::Chase); break; } // reached the cab first
  HeadGoal = ThreatYaw(In.Player - Pos);
  if (!In.bSeesPlayer || FVector2D::Distance(Pos, In.Player) > CaptureReach + .15 || !ContactClear(In.Player)) { Enter(EState::Chase); break; }
  if (StateT >= CaptureHold) bCaptureEvent = true;
  break;
 case EState::Withdrawn:
  // It leaves only out of the player's actual view.
  if ((Move(Nodes()[RetreatNode].At, WalkSpeed * 1.1, Dt, .4) || StateT >= 2.5) && !In.bPlayerSeesBody) bGone = true;
  break;
 }
 const double Moved = FVector2D::Distance(Before, Pos);
 Phase += Moved / Stride * 2 * PI;
 Gait += (FMath::Min(1.4, Moved / Dt / 1.3) - Gait) * FMath::Min(1.0, Dt * 6);
 // The head turns at a readable rate and never beyond the neck's reach.
 const double Want = BodyYaw + FMath::Clamp(ThreatWrap(HeadGoal - BodyYaw), -1.45, 1.45);
 HeadYaw = ThreatWrap(HeadYaw + FMath::Clamp(ThreatWrap(Want - HeadYaw), -HeadRate * Dt, HeadRate * Dt));
}

bool FS47HorrorThreat::InSite() const
{
 return M && M->World && (M->World->bDriving || M->World->Area == TEXT("road") || M->World->Area == TEXT("diner"));
}

bool FS47HorrorThreat::Held() const
{
 return !M || M->Mode != US47Main::EMode::Play || M->bPausedByMenu || M->ModalOpen() || !M->Story || M->Story->IsCinematic() ||
  !M->World || M->World->bBusy || !M->Player || (M->Flow && M->Flow->bStarting) || !InSite();
}

bool FS47HorrorThreat::ClearLine(const FVector& From, const FVector& To, bool bToBody) const
{
 // FS47HorrorVisual::ClearLine's opaque rule: visible solid surfaces stop sight; proxies,
 // additive and see-through translucent materials do not. The body never hides itself.
 if (!M || !M->Web || !M->Web->GetWorld()) return false;
 AS47Web* W = M->Web.Get();
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47ThreatSight), true);
 if (!bToBody) for (const auto& P : Parts->All) if (const UStaticMeshComponent* C = P.Get()) Q.AddIgnoredComponent(C);
 const FVector A = S47Three::ToUE(From), B = S47Three::ToUE(To);
 for (int32 I = 0; I < 24; ++I)
 {
  FHitResult H;
  if (!W->GetWorld()->LineTraceSingleByChannel(H, A, B, ECC_Visibility, Q)) return true;
  const auto* C = H.GetComponent(); if (!C) return false;
  if (bToBody && Parts->All.ContainsByPredicate([C](const TWeakObjectPtr<UStaticMeshComponent>& P) { return P.Get() == C; })) return true;
  bool Solid = C->IsVisible();
  const auto* Node = W->Node(W->NodeOf(C));
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

bool FS47HorrorThreat::SeesPlayer() const
{
 const FS47Cam Cam = FS47Cam::Of(M->Player);
 const FVector Eye = Head3();
 const FVector2D D(Cam.Pos.X - Eye.X, Cam.Pos.Z - Eye.Z);
 const double Dist = D.Size();
 if (Dist > SightRange) return false;
 if (Dist > .3 && FVector2D::DotProduct(FVector2D(FMath::Cos(HeadYaw), FMath::Sin(HeadYaw)), D * (1.0 / Dist)) < SightCos) return false;
 return ClearLine(Eye, Cam.Pos, false) || ClearLine(Eye, Cam.Pos - FVector(0, .55, 0), false);
}

bool FS47HorrorThreat::BodyInView() const
{
 if (!bAppeared || bGone || !M || !M->Player) return false;
 const UStaticMeshComponent* T = Parts->Torso.Get();
 if (Parts->Creature.Ready() ? !Parts->Creature.Rendered() : (!T || !T->IsRegistered() || !T->IsVisible() || !T->WasRecentlyRendered(.25f))) return false;
 const FS47Cam Cam = FS47Cam::Of(M->Player);
 for (const FVector& P : {Chest3(), Head3()})
 {
  if (FVector::Dist(Cam.Pos, P) > ViewRange) continue;
  const FVector N = Cam.Project(P);
  if (N.Z >= 1 || FMath::Abs(N.X) > .93 || FMath::Abs(N.Y) > .93) continue;
  if (ClearLine(Cam.Pos, P, true)) return true;
 }
 return false;
}

bool FS47HorrorThreat::PostHidden() const
{
 const FVector2D At = Nodes()[PostNode].At;
 const FS47Cam Cam = FS47Cam::Of(M->Player);
 if (FVector2D::Distance(FVector2D(Cam.Pos.X, Cam.Pos.Z), At) < 6) return false;
 for (const double H : {.3, 1.0, 1.9})
 {
  const FVector P(At.X, Y + H, At.Y);
  const FVector N = Cam.Project(P);
  if (N.Z < 1 && FMath::Abs(N.X) < 1.05 && FMath::Abs(N.Y) < 1.05 && ClearLine(Cam.Pos, P, false)) return false;
 }
 return true;
}

TOptional<FVector2D> FS47HorrorThreat::Heard(const FSense& In, double Dt)
{
 // Only real sources at their actual place: the player's own footsteps and SARO 07.
 if (In.bDriving)
 {
  bHavePlayer = false;
  if (In.TruckDistance <= EngineReach) return In.Player;
  return {};
 }
 const double Speed = bHavePlayer ? FVector2D::Distance(In.Player, LastPlayer) / FMath::Max(Dt, 1e-3) : 0;
 LastPlayer = In.Player; bHavePlayer = true; NoiseCool -= Dt;
 if (Speed < .4 || Speed > 12 || NoiseCool > 0) return {}; // still, or placed
 if (FVector2D::Distance(In.Player, Pos) > (Speed > 2.5 ? RunReach : WalkReach)) return {};
 NoiseCool = .5;
 return In.Player;
}

double FS47HorrorThreat::Ground(const FVector2D& P, bool bFree) const
{
 if (!M || !M->Web || !M->Web->GetWorld()) return Y;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47ThreatGround), true);
 for (const auto& C : Parts->All) if (const UStaticMeshComponent* Part = C.Get()) Q.AddIgnoredComponent(Part);
 const double Up = bFree ? 3.0 : 1.0, Down = bFree ? 6.0 : 3.0;
 FHitResult H;
 if (!M->Web->GetWorld()->LineTraceSingleByChannel(H, S47Three::ToUE(P.X, Y + Up, P.Y), S47Three::ToUE(P.X, Y - Down, P.Y), ECC_Visibility, Q)) return Y;
 const double G = S47Three::ToThree(H.ImpactPoint).Y;
 return bFree || FMath::Abs(G - Y) < .6 ? G : Y; // never step up onto the truck or a cabinet
}

void FS47HorrorThreat::Step(double Dt)
{
 if (!bActive) return;
 if (!bBuilt && !Build()) return;
 FSense In; In.bHeld = Held();
 if (!In.bHeld)
 {
  In.bDriving = M->World->bDriving;
  if (In.bDriving)
  {
   if (const FS47Truck* T = M->World->CabTruck()) { In.Player = FVector2D(T->Position.X, T->Position.Z); In.TruckDistance = FVector2D::Distance(In.Player, Pos); }
  }
  else In.Player = FVector2D(M->Player->Pos.X, M->Player->Pos.Z);
  if (!bAppeared) In.bCanAppear = !In.bDriving && PostHidden();
  else if (!bGone)
  {
   In.bSeesPlayer = !In.bDriving && SeesPlayer();
   In.bPlayerSeesBody = BodyInView();
   In.Sound = Heard(In, Dt);
  }
 }
 const bool bWas = bAppeared;
 const EState Was = S;
 const double PhaseBefore = Phase;
 Think(In, Dt);
 if (bAppeared && !bWas)
 {
  Y = Ground(Pos, true); bGrounded = true;
  UE_LOG(LogS47HorrorThreat, Display, TEXT("S47: SC02 pale body appeared unseen at %s (%.2f %.2f %.2f)"), Nodes()[PostNode].Id, Pos.X, Y, Pos.Y);
 }
 if (S != Was) UE_LOG(LogS47HorrorThreat, Display, TEXT("S47: SC02 body %s -> %s at %.2f %.2f; witnessed %d"), StateName(Was), StateName(S), Pos.X, Pos.Y, bSeen ? 1 : 0);
 if (!In.bHeld && bAppeared && !bGone) { Y = Ground(Pos, false); PlaySounds(PhaseBefore); }
 Apply(bAppeared && !bGone);
}

void FS47HorrorThreat::PlaySounds(double PhaseBefore)
{
 if (!M || !M->Audio) return;
 // Gravel steps and breath are placed at the body (existing step bank and breath synthesis).
 if (FMath::FloorToDouble(Phase / PI) != FMath::FloorToDouble(PhaseBefore / PI))
 {
  const auto Pick = M->Audio->StepPick(TEXT("dirt"));
  FS47Sound O; O.Gain = Pick.Gain * (S == EState::Chase ? 1.f : .7f); O.At = FVector(Pos.X, Y + .05, Pos.Y);
  M->Audio->Sfx(Pick.Name, O);
 }
 if (bBreath) { bBreath = false; M->Audio->Gasp(M->Audio->Spatial(Head3(), 2.f)); }
}

void FS47HorrorThreat::Apply(bool bShow)
{
 if (!bBuilt) return;
 Parts->Root.SetVisible(bShow);
 Parts->Creature.Apply(bShow,ThreatWrap(HeadYaw-BodyYaw),Gait,Phase,Clock,S==EState::Capture,S==EState::Warn);
 if (Parts->bCollide != bShow)
 {
  // Hidden parts never block a sight, floor or ground trace.
  Parts->bCollide = bShow;
  for (const auto& C : Parts->All) if (UStaticMeshComponent* P = C.Get()) P->SetCollisionEnabled(bShow ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
 }
 if (!bShow) return;
 if (UBoxComponent* R = Parts->RootComp.Get()) R->SetWorldLocationAndRotation(S47Three::ToUE(Pos.X, Y, Pos.Y), FRotator(0, FMath::RadiansToDegrees(BodyYaw), 0));
 // Stiff joints with a procedural gait. Positive pitch swings a hanging limb forward (+X).
 const double A = FMath::Clamp(Gait, 0.0, 1.4), Sw = FMath::Sin(Phase), Cw = FMath::Cos(Phase);
 double ArmL = -22 * A * Sw, ArmR = 22 * A * Sw, Elbow = 10 + 12 * A, Lean = -(2 + 7 * A) + 1.2 * FMath::Sin(Clock * 1.3);
 if (S == EState::Warn) Lean -= 3;
 if (S == EState::Capture) { ArmL = ArmR = 78; Elbow = 8; Lean = -12; }
 auto Set = [](const TWeakObjectPtr<UStaticMeshComponent>& W, double Pitch, double Yaw)
 { if (UStaticMeshComponent* C = W.Get()) C->SetRelativeRotation(FRotator(Pitch, Yaw, 0)); };
 // Deliberate stop-motion: every joint holds its pose and jumps to the gait target on its
 // own staggered tick, so the body never moves as one smooth mannequin. Clock only advances
 // in Think, so a held (paused) body also holds every joint. A grab snaps at once.
 const double Target[JointCount] = {Lean, FMath::RadiansToDegrees(ThreatWrap(HeadYaw - BodyYaw)), ArmL, Elbow, ArmR, Elbow,
  26 * A * Sw, -(6 + 34 * A * FMath::Max(0.0, Cw)), -26 * A * Sw, -(6 + 34 * A * FMath::Max(0.0, -Cw))};
 const double Rate = S == EState::Chase ? 11.0 : 5.0 + 4.0 * A;
 for (int32 I = 0; I < JointCount; ++I)
 {
  const int32 Tick = FMath::FloorToInt(Clock * Rate + I * .37);
  if (Tick != JointTick[I] || S == EState::Capture) { JointTick[I] = Tick; JointPose[I] = Target[I]; }
 }
 Set(Parts->Torso, JointPose[0], 0);
 Set(Parts->Head, -4 - .6 * JointPose[0], JointPose[1]); // the head lifts against the lean to keep looking ahead
 Set(Parts->ArmL, JointPose[2], 0); Set(Parts->ForeL, JointPose[3], 0);
 Set(Parts->ArmR, JointPose[4], 0); Set(Parts->ForeR, JointPose[5], 0);
 Set(Parts->ThighL, JointPose[6], 0); Set(Parts->ShinL, JointPose[7], 0);
 Set(Parts->ThighR, JointPose[8], 0); Set(Parts->ShinR, JointPose[9], 0);
}

bool FS47HorrorThreat::NativeSelfTest(TArray<FString>& Lines)
{
 bool Good = true;
 auto Check = [&Good, &Lines](bool Value, const TCHAR* What)
 {
  Good &= Value;
  Lines.Add(FString::Printf(TEXT("native navigation %s: %s"), Value ? TEXT("PASS") : TEXT("FAIL"), What));
  return Value;
 };
 UWorld* World = M && M->Web ? M->Web->GetWorld() : nullptr;
 if (!Check(World && M->Player, TEXT("loaded production UWorld and player are available for isolated native queries"))) return false;
 const FVector PlayerBefore = M->Player->Pos;
 const FVector2D BodyBefore = Pos;
 const EState StateBefore = S;
 const FVector Origin(0, 0, 100000); // test fixture only, one kilometre above world terrain
 const FVector MovedOrigin = Origin + FVector(0, 300, 0);
 const FVector OtherOrigin = Origin + FVector(1000, 1000, 0);
 const FCollisionShape Capsule = FCollisionShape::MakeCapsule(float(BodyRadius * 100), 94.f);
 FCollisionQueryParams Query(SCENE_QUERY_STAT(S47ThreatNativeProbe), true, M->Player);
 auto Sweep = [World, &Capsule, &Query](const FVector& Centre, FHitResult& Hit)
 {
  Hit = FHitResult();
  // Extent Z=75: capsule centre 24 cm above the box centre puts its bottom 5 cm above its floor.
  return World->SweepSingleByChannel(Hit, Centre + FVector(-300, 0, 24), Centre + FVector(300, 0, 24),
   FQuat::Identity, ECC_Visibility, Capsule, Query);
 };
 FHitResult Hit;
 if (!Check(!Sweep(Origin, Hit) && !Sweep(MovedOrigin, Hit), TEXT("native fixture corridors are clear before any probe components exist"))) return false;
 FActorSpawnParameters Spawn;
 Spawn.ObjectFlags |= RF_Transient;
 Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
 AActor* Fixture = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
 if (!Check(Fixture != nullptr, TEXT("transient native collision fixture actor was created"))) return false;
 ON_SCOPE_EXIT { if (IsValid(Fixture)) Fixture->Destroy(); };
 USceneComponent* Root = NewObject<USceneComponent>(Fixture, TEXT("ThreatProbeRoot"));
 Fixture->AddInstanceComponent(Root); Root->SetMobility(EComponentMobility::Movable);
 Fixture->SetRootComponent(Root); Root->RegisterComponent();
 UBoxComponent* Box = NewObject<UBoxComponent>(Fixture, TEXT("ThreatProbeBox"));
 Fixture->AddInstanceComponent(Box); Box->SetupAttachment(Root); Box->SetMobility(EComponentMobility::Movable);
 Box->SetBoxExtent(FVector(40, 70, 75)); Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 Box->SetCollisionObjectType(ECC_WorldDynamic); Box->SetCollisionResponseToAllChannels(ECR_Ignore);
 Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); Box->SetGenerateOverlapEvents(false);
 Box->RegisterComponent(); Box->SetWorldLocation(Origin); Box->UpdateBounds();
 if (!Check(Sweep(Origin, Hit) && Hit.GetComponent() == Box, TEXT("production-sized capsule sweep blocks on an actual query-only UBoxComponent"))) return false;
 FBox Bounds(ForceInit);
 if (!Check(ThreatNativeBounds(Box, INDEX_NONE, Bounds) && Bounds.GetCenter().Equals(Origin, .01) &&
  Bounds.GetExtent().Equals(FVector(40, 70, 75), .01), TEXT("native box bounds match its world transform and dimensions"))) return false;
 Box->SetWorldLocation(MovedOrigin); Box->UpdateBounds();
 if (!Check(!Sweep(Origin, Hit) && Sweep(MovedOrigin, Hit) && Hit.GetComponent() == Box,
  TEXT("moving the same native box frees its old corridor and blocks its new corridor"))) return false;
 if (!Check(ThreatNativeBounds(Box, INDEX_NONE, Bounds) && Bounds.GetCenter().Equals(MovedOrigin, .01),
  TEXT("discovered native box bounds follow the live moved component"))) return false;
 Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 if (!Check(!ThreatNativeBounds(Box, INDEX_NONE, Bounds) && !Sweep(MovedOrigin, Hit),
  TEXT("collision-disabled box has no navigation bounds and no native sweep hit"))) return false;
 Box->DestroyComponent();
 UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (!Check(Cube != nullptr, TEXT("engine Cube mesh is available for the two-instance HISM query fixture"))) return false;
 UHierarchicalInstancedStaticMeshComponent* Instances = NewObject<UHierarchicalInstancedStaticMeshComponent>(Fixture, TEXT("ThreatProbeHISM"));
 Fixture->AddInstanceComponent(Instances); Instances->SetupAttachment(Root); Instances->SetMobility(EComponentMobility::Movable);
 Instances->SetStaticMesh(Cube); Instances->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 Instances->SetCollisionObjectType(ECC_WorldDynamic); Instances->SetCollisionResponseToAllChannels(ECR_Ignore);
 Instances->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); Instances->SetGenerateOverlapEvents(false);
 Instances->bAutoRebuildTreeOnInstanceChanges = false; Instances->RegisterComponent();
 const FVector Scale(.8, 1.4, 1.5);
 const int32 First = Instances->AddInstance(FTransform(FQuat::Identity, Origin, Scale), true);
 const int32 Other = Instances->AddInstance(FTransform(FQuat::Identity, OtherOrigin, Scale), true);
 Instances->BuildTreeIfOutdated(false, true); Instances->UpdateBounds();
 if (!Check(First == 0 && Other == 1 && Instances->GetInstanceCount() == 2, TEXT("HISM fixture has two separately indexed cube instances"))) return false;
 if (!Check(Sweep(Origin, Hit) && Hit.GetComponent() == Instances && Hit.Item == First,
  TEXT("native capsule hit reports the first HISM instance through Hit.Item"))) return false;
 if (!Check(ThreatNativeBounds(Instances, Hit.Item, Bounds) && Bounds.GetCenter().Equals(Origin, .01) &&
  Bounds.GetExtent().Equals(FVector(40, 70, 75), .01), TEXT("HISM obstacle bounds describe only the hit instance, not the distant sibling"))) return false;
 if (!Check(Instances->Bounds.GetBox().GetSize().X > Bounds.GetSize().X + 500,
  TEXT("HISM fixture's aggregate bounds are deliberately much larger than the hit-instance bounds"))) return false;
 FBox OtherBounds(ForceInit);
 if (!Check(ThreatNativeBounds(Instances, Other, OtherBounds) && OtherBounds.GetCenter().Equals(OtherOrigin, .01),
  TEXT("second HISM instance keeps its own independent world bounds"))) return false;
 if (!Check(Instances->UpdateInstanceTransform(First, FTransform(FQuat::Identity, MovedOrigin, Scale), true, true, true),
  TEXT("the first HISM instance accepts a live world-space transform update"))) return false;
 Instances->BuildTreeIfOutdated(false, true); Instances->UpdateBounds();
 if (!Check(!Sweep(Origin, Hit) && Sweep(MovedOrigin, Hit) && Hit.GetComponent() == Instances && Hit.Item == First,
  TEXT("moving one HISM instance updates physical queries without leaving its old collider behind"))) return false;
 if (!Check(ThreatNativeBounds(Instances, Hit.Item, Bounds) && Bounds.GetCenter().Equals(MovedOrigin, .01),
  TEXT("discovered HISM instance bounds follow the updated instance transform"))) return false;
 FBox OtherAfter(ForceInit);
 if (!Check(ThreatNativeBounds(Instances, Other, OtherAfter) && OtherAfter.Min.Equals(OtherBounds.Min, .01) && OtherAfter.Max.Equals(OtherBounds.Max, .01),
  TEXT("moving the first HISM instance leaves the other instance's bounds unchanged"))) return false;
 if (!Check(!ThreatNativeBounds(Instances, INDEX_NONE, Bounds) && !ThreatNativeBounds(Instances, 2, Bounds),
  TEXT("invalid native instance indices do not fall back to aggregate HISM bounds"))) return false;
 Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 if (!Check(!ThreatNativeBounds(Instances, First, Bounds) && !Sweep(MovedOrigin, Hit),
  TEXT("collision-disabled HISM has no discovered-instance bounds and no sweep hit"))) return false;
 Instances->DestroyComponent();
 if (!Check(!Sweep(Origin, Hit) && !Sweep(MovedOrigin, Hit), TEXT("destroyed fixture components leave both native query corridors clear"))) return false;
 // Exercise the production ground/step adapter on a separate, unrendered threat. The
 // test floor's top is Origin.Z, so these probes never move the actual body or player.
 auto ProbeBox = [Fixture, Root](const TCHAR* Name, const FVector& Centre, const FVector& Extent)
 {
  UBoxComponent* Part = NewObject<UBoxComponent>(Fixture, Name);
  Fixture->AddInstanceComponent(Part); Part->SetupAttachment(Root); Part->SetMobility(EComponentMobility::Movable);
  Part->SetBoxExtent(Extent); Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  Part->SetCollisionObjectType(ECC_WorldDynamic); Part->SetCollisionResponseToAllChannels(ECR_Ignore);
  Part->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); Part->SetGenerateOverlapEvents(false);
  Part->RegisterComponent(); Part->SetWorldLocation(Centre); Part->UpdateBounds();
  return Part;
 };
 UBoxComponent* Floor = ProbeBox(TEXT("ThreatProbeStepFloor"), Origin - FVector(0, 0, 10), FVector(600, 600, 10));
 const FVector From3 = S47Three::ToThree(Origin + FVector(-300, 0, 0));
 const FVector To3 = S47Three::ToThree(Origin + FVector(300, 0, 0));
 const FVector2D From(From3.X, From3.Z), To(To3.X, To3.Z);
 FS47HorrorThreat Probe(M); Probe.bWorld = true; Probe.Y = Origin.Z / 100.0; Probe.Pos = From;
 if (!Check(FMath::Abs(Probe.Ground(From, false) - Probe.Y) < .001 && Probe.PhysicalSegment(From, To),
  TEXT("production PhysicalSegment reads the actual fixture floor and passes its unobstructed corridor"))) return false;
 UBoxComponent* Step = ProbeBox(TEXT("ThreatProbeLowStep"), Origin + FVector(0, 0, 5), FVector(40, 70, 5));
 if (!Check(Probe.PhysicalSegment(From, To) && Probe.PhysicalSegment(To, From),
  TEXT("production PhysicalSegment passes a 10 cm box step in both directions using its full lifted recheck"))) return false;
 Step->SetBoxExtent(FVector(40, 70, 20)); Step->SetWorldLocation(Origin + FVector(0, 0, 20)); Step->UpdateBounds();
 FHitResult StepHit;
 if (!Check(!Probe.PhysicalSegment(From, To, &StepHit) && StepHit.GetComponent() == Step,
  TEXT("production PhysicalSegment blocks a 40 cm barrier instead of treating every low face as traversable"))) return false;
 Step->SetBoxExtent(FVector(40, 70, 75)); Step->SetWorldLocation(Origin + FVector(0, 0, 75)); Step->UpdateBounds();
 if (!Check(!Probe.PhysicalSegment(From, To, &StepHit) && StepHit.GetComponent() == Step,
  TEXT("production PhysicalSegment still blocks a tall barrier after its low-step checks"))) return false;
 Check(Probe.Pos.Equals(From, 1e-9) && Probe.NativeObstacles.IsEmpty(),
  TEXT("PhysicalSegment probes do not move their test body or learn movement obstacles"));
 Step->DestroyComponent(); Floor->DestroyComponent();
 if (!Check(Probe.PhysicalSegment(From, To), TEXT("destroyed step fixtures leave the production segment clear"))) return false;
 Check(M->Player->Pos.Equals(PlayerBefore, 1e-9) && Pos.Equals(BodyBefore, 1e-9) && S == StateBefore,
  TEXT("native boundary probe neither moves the actual player/body nor changes the threat state"));
 return Good;
}

bool FS47HorrorThreat::SelfTest(TArray<FString>& Lines)
{
 bool Good = true;
 auto Check = [&](bool Value, const TCHAR* What) { Good &= Value; Lines.Add(FString::Printf(TEXT("SC02 threat %s %s"), Value ? TEXT("PASS") : TEXT("FAIL"), What)); };
 const auto& N = Nodes();
 bool Outside = true, EdgesClear = true;
 for (const FNode& Node : N) Outside &= !InYard(Node.At, 1.2);
 for (const auto& E : Edges())
 {
  const FVector2D A = N[E.Key].At, B = N[E.Value].At;
  EdgesClear &= !ThreatSegment(A, B, YardX0 - .4, YardX1 + .4, YardZ0 - .4, YardZ1 + .4);
  for (const FThreatBlock& K : FrontBlocks) EdgesClear &= !ThreatSegment(A, B, K.X0 - .3, K.X1 + .3, K.Z0 - .3, K.Z1 + .3);
 }
 Check(Outside, TEXT("every route point stands at least 1.2 m outside the closed yard"));
 Check(EdgesClear, TEXT("no route edge crosses the high-voltage yard, control cabinet, culvert or band post"));
 Check(InYard(FVector2D(7890, 536.2)) && !InYard(N[NFront].At), TEXT("yard test matches the fence collider and leaves the service front outside"));
 TArray<bool> All; All.Init(true, N.Num());
 TArray<bool> West = All, East = All, EastFront = All, Closed = All;
 for (const int32 I : {NNorthWest, NNorth, NNorthEast, NEastMid, NEastCorner, NEastApron, NEastTrack}) West[I] = false;
 for (const int32 I : {NWestMid, NWestCorner, NWestApron, NWestTrack, NFront, NTrack}) East[I] = false;
 for (const int32 I : {NWestMid, NWestCorner, NWestApron, NWestTrack, NTrack}) EastFront[I] = false;
 Closed[NWestMid] = Closed[NNorthWest] = false;
 Check(Route(NShed, NSouth, West).Num() > 0, TEXT("route A: shed cover to the south track along the west fence"));
 Check(Route(NShed, NSouth, East).Num() > 0, TEXT("route B: shed cover around the north and east fence to the south track"));
 Check(Route(NFront, NSouth, West).Num() > 0 && Route(NFront, NEastTrack, EastFront).Num() > 0, TEXT("the front reaches the truck side by either flank"));
 Check(Route(NShed, NSouth, Closed).Num() == 0, TEXT("with both shed-side fence lines closed no route crosses the yard"));

 FS47HorrorThreat T(nullptr); T.Start(false);
 Check(!T.Clear(N[NShed].At, N[NEastMid].At) && !T.Clear(N[NFront].At, N[NNorth].At), TEXT("a straight line through the yard is never a way for the body"));
 FSense In; const double Dt = .1;
 T.Think(In, Dt);
 Check(!T.Appeared(), TEXT("the body never appears while its post is in actual view"));
 In.bCanAppear = true; T.Think(In, Dt); In.bCanAppear = false;
 Check(T.Appeared() && T.State() == EState::Idle && T.Position().Equals(N[NPost].At, .001), TEXT("the body appears at its post between the front and the truck"));
 In.Player = N[NFront].At; In.Sound = N[NFront].At; T.Think(In, Dt); In.Sound.Reset();
 const FVector2D Stand = T.Position();
 Check(T.State() == EState::Warn, TEXT("a real heard source starts the forewarning"));
 for (int32 I = 0; I < 10; ++I) T.Think(In, Dt);
 Check(T.State() == EState::Warn && T.Position().Equals(Stand, .0001) && FMath::Abs(ThreatWrap(T.HeadYaw + .5 * PI)) > .5,
  TEXT("the body stands still and turns its head towards the sound before moving"));
 for (int32 I = 0; I < 8; ++I) T.Think(In, Dt);
 Check(T.State() == EState::Investigate, TEXT("after the readable turn it investigates the heard place"));
 In.bSeesPlayer = true; In.Player = T.Position() + FVector2D(.5, 0);
 bool Never = true;
 for (int32 I = 0; I < 60; ++I) { T.Think(In, Dt); Never &= T.State() != EState::Chase && T.State() != EState::Capture && !T.bCaptureEvent; }
 Check(Never && !T.Seen(), TEXT("an unwitnessed body may turn and approach but never chases or captures"));
 In.bPlayerSeesBody = true;
 for (int32 I = 0; I < 60 && !T.Seen(); ++I) T.Think(In, Dt);
 Check(T.Seen(), TEXT("a rendered, unobstructed view during the turn records the witnessed forewarning"));
 for (int32 I = 0; I < 60 && T.State() != EState::Chase; ++I) T.Think(In, Dt);
 Check(T.State() == EState::Chase, TEXT("a witnessed body that keeps the player in sight chases"));
 In.Player = T.Position() + FVector2D(.5, 0);
 for (int32 I = 0; I < 15 && !T.bCaptureEvent; ++I) T.Think(In, Dt);
 Check(T.ConsumeCapture() && !T.ConsumeCapture(), TEXT("contact within reach of a chasing, witnessed body is one capture event"));

 T.Reset(); In = FSense(); In.bCanAppear = true; T.Think(In, Dt); In.bCanAppear = false;
 In.Player = N[NFront].At; In.Sound = N[NFront].At; T.Think(In, Dt); In.Sound.Reset();
 const double HeldT = T.StateT, HeldHead = T.HeadYaw; const FVector2D HeldPos = T.Position();
 In.bHeld = true; for (int32 I = 0; I < 50; ++I) T.Think(In, Dt); In.bHeld = false;
 Check(T.Seen() && T.State() == EState::Warn && T.StateT == HeldT && T.HeadYaw == HeldHead && T.Position() == HeldPos,
  TEXT("pause or a modal panel freezes the body with no saved-up approach; a retry keeps the witnessed fact"));
 T.Enter(EState::Chase); In.Player = T.Position() + FVector2D(8, 0);
 for (int32 I = 0; I < 30 && T.State() == EState::Chase; ++I) T.Think(In, Dt);
 Check(T.State() == EState::Search, TEXT("broken sight for the measured time loses contact and starts a search"));
 for (int32 I = 0; I < 120 && T.State() == EState::Search; ++I) T.Think(In, Dt);
 Check(T.State() == EState::Idle, TEXT("a finished search returns the body to its post"));
 T.Enter(EState::Chase); In.bDriving = true; In.Player = T.Position() + FVector2D(3, 0); In.TruckDistance = 3;
 bool Cab = true;
 for (int32 I = 0; I < 50; ++I) { T.Think(In, Dt); Cab &= !T.bCaptureEvent && T.State() != EState::Capture && !T.Withdrawn(); }
 Check(Cab, TEXT("SARO 07's cab cannot be taken and a near truck keeps contact"));
 In.TruckDistance = 30;
 for (int32 I = 0; I < 25; ++I) T.Think(In, Dt);
 Check(T.Withdrawn() && !T.Gone(), TEXT("a clear drive beyond contact distance for the measured time withdraws the body"));
 In.bPlayerSeesBody = true; for (int32 I = 0; I < 40; ++I) T.Think(In, Dt);
 const bool Watched = !T.Gone();
 In.bPlayerSeesBody = false; T.Think(In, Dt);
 Check(Watched && T.Gone(), TEXT("a withdrawn body leaves only out of the player's view"));
 T.Stop();
 Check(!T.Active() && !T.Withdrawn(), TEXT("a stopped meeting holds no state that could start a later hunt"));
 // Exercise the movement adapter, not just the planner, with a newly placed obstacle.
 FS47HorrorThreat Moving(nullptr); Moving.Start(true); Moving.Pos = FVector2D(0, 0);
 const FVector2D Destination(8, 0);
 Moving.Move(Destination, 2, .1, .05);
 Moving.Near.Add({3, 5, -1, 1, TEXT("test:late-wall"), true});
 bool SafeSteps = true; double Detour = 0;
 for (int32 I = 0; I < 150; ++I)
 {
  const FVector2D BeforeStep = Moving.Pos;
  Moving.Move(Destination, 2, .1, .05);
  SafeSteps &= Moving.Clear(BeforeStep, Moving.Pos);
  Detour = FMath::Max(Detour, FMath::Abs(Moving.Pos.Y));
 }
 Check(SafeSteps && Detour > 1.28 && FVector2D::Distance(Moving.Pos, Destination) <= .051,
  TEXT("movement replans around a new wall with body clearance and reaches the far side"));
 FS47HorrorThreat Wall(nullptr); Wall.Start(true); Wall.Pos = FVector2D(0, 0); Wall.bAppeared = true; Wall.Enter(EState::Chase);
 Wall.Near.Add({.35, .40, -2, 2, TEXT("test:thin-wall"), true});
 FSense Contact; Contact.Player = FVector2D(.8, 0); Contact.bSeesPlayer = true;
 bool NoWallGrab = true;
 for (int32 I = 0; I < 12; ++I) { Wall.Think(Contact, .1); NoWallGrab &= Wall.State() != EState::Capture && !Wall.bCaptureEvent; }
 Check(NoWallGrab, TEXT("a nearby player behind a thin blocker is never grabbed through it"));
 FS47HorrorThreat WallSide(nullptr); WallSide.Start(true); WallSide.Pos = FVector2D(0, 0); WallSide.bAppeared = true; WallSide.Enter(EState::Chase);
 WallSide.Near.Add({1, 2, -4, 4, TEXT("test:wall-clearance"), true});
 FSense Beside; Beside.Player = FVector2D(.725, 0); Beside.bSeesPlayer = true;
 for (int32 I = 0; I < 15 && !WallSide.bCaptureEvent; ++I) WallSide.Think(Beside, .1);
 Check(WallSide.bCaptureEvent, TEXT("a legal player beside a wall remains reachable within hand contact distance"));

 Wall.Near.Reset(); Wall.Pos = FVector2D(0, 0); Wall.Enter(EState::Capture); Contact.bSeesPlayer = false;
 for (int32 I = 0; I < 8; ++I) Wall.Think(Contact, .1);
 Check(!Wall.bCaptureEvent && Wall.State() != EState::Capture, TEXT("lost physical contact cancels a grab before its hold completes"));
 return FS47ThreatNavigation::SelfTest(Lines) && Good;
}
