#include "S47Blockers.h"
#include "S47Walker.h"
#include "S47Drive.h"
#include "S47Herd.h"
#include "S47Main.h"
#include "S47OpenLand.h"
#include "S47SaroClip.h"
#include "S47Three.h"
#include "S47TownLayer.h"
#include "S47Web.h"
#include "S47World.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

namespace S47Blockers
{
namespace
{
 constexpr double Cell = 8, BigSize = 4, Fine = 2;
 struct FDrawn { FBox Box; bool bBig = false; FString Name; };
 // A standing face of a big mesh (a wall, a fence, a post): its corners in three metres. Flat ground never stands.
 struct FFace { FVector3f A, B, C; };
 // Its triangles' corner indices: the render data's, or the editor's source; empty when neither is kept.
 void Indices(UStaticMesh* Mesh, TArray<uint32>& Out)
 {
  Out.Reset();
  if (const FStaticMeshRenderData* RD = Mesh->GetRenderData(); RD && RD->LODResources.Num() > 0 && RD->LODResources[0].IndexBuffer.GetNumIndices() > 0)
  {
   bool bExact = false;
   const int32 NV = S47SaroClip::Vertices(Mesh, bExact).Num();
   const uint32 RV = RD->LODResources[0].VertexBuffers.PositionVertexBuffer.GetNumVertices();
   if (bExact && uint32(NV) == RV) { RD->LODResources[0].IndexBuffer.GetCopy(Out); return; }
  }
 }
 // The triangle's shadow on the ground meets the square (separating axes: the square's two, the triangle's three).
 bool Meets(const FVector2D T[3], const FBox2D& Sq)
 {
  const FVector2D C[4] = { Sq.Min, { Sq.Max.X, Sq.Min.Y }, Sq.Max, { Sq.Min.X, Sq.Max.Y } };
  const auto Apart = [&](const FVector2D& Axis)
  {
   double A0 = 1e18, A1 = -1e18, B0 = 1e18, B1 = -1e18;
   for (int32 I = 0; I < 3; ++I) { const double D = FVector2D::DotProduct(T[I], Axis); A0 = FMath::Min(A0, D); A1 = FMath::Max(A1, D); }
   for (int32 I = 0; I < 4; ++I) { const double D = FVector2D::DotProduct(C[I], Axis); B0 = FMath::Min(B0, D); B1 = FMath::Max(B1, D); }
   return A1 < B0 || B1 < A0;
  };
  if (Apart({ 1, 0 }) || Apart({ 0, 1 })) return false;
  for (int32 I = 0; I < 3; ++I) { const FVector2D E = T[(I + 1) % 3] - T[I]; if (!E.IsNearlyZero() && Apart({ -E.Y, E.X })) return false; }
  return true;
 }
 FIntPoint CellOf(double X, double Z, double Size) { return FIntPoint(FMath::FloorToInt(X / Size), FMath::FloorToInt(Z / Size)); }
 // BLOCKERS-20261109: boxes that stand for something drawn which this measure cannot see from where it is taken. SARO's records room:
 // the room's furniture is drawn only while one is near it, and the measure stands in the yard; each box is the web export's own,
 // made by the code that builds the piece (web/src/world/Annex.ts, dressing.ts). Matched by the box itself: the list's indices move.
 struct FKnown { const TCHAR* Area; double MinX, MaxX, MinZ, MaxZ; const TCHAR* What; };
 const FKnown Known[] = {
  { TEXT("saro"), -5.27, -3.65, 9.84, 10.5, TEXT("records room: the three filing cabinets on the south wall (Annex.ts records room)") },
  { TEXT("saro"), -3.32, -1.38, 10.1, 10.5, TEXT("records room: the low bookcase with binders (Annex.ts)") },
  { TEXT("saro"), -1.8, -1.0, 7.5, 8.4, TEXT("records room: the card index cabinet on the east wall (Annex.ts)") },
  { TEXT("saro"), -3.9, -3.4, 9.05, 9.7, TEXT("records room: the folding chair at the table (Annex.ts)") },
  { TEXT("saro"), -4.7, -4.22, 9.48, 9.86, TEXT("records room: the middle cabinet's open drawer (dressing.ts)") },
 };
 const FKnown* KnownOf(const FString& Walk, const FBlocker& B)
 {
  for (const FKnown& K : Known)
   if (Walk == K.Area && FMath::Abs(B.MinX - K.MinX) < .02 && FMath::Abs(B.MaxX - K.MaxX) < .02 && FMath::Abs(B.MinZ - K.MinZ) < .02 && FMath::Abs(B.MaxZ - K.MaxZ) < .02) return &K;
  return nullptr;
 }
 // BLOCKERS-20261109: the others that are known by what they are rather than by their box. A cow's circle in the truck's list is
 // her own (S47Herd: it goes with her, within a step) and round for a long animal, so its edge by her flanks shows bare. A fence
 // line of the open land from SARO's z 100 south (the highway's and the pasture's) is stepped through in Freeform's open land
 // (US47World::OnLand), so it holds no one there. Empty: not known.
 FString KnownWhat(US47Main* M, const FString& Area, const FBlocker& B)
 {
  if (B.Kind == TEXT("drive") && B.bCircle && FMath::Abs((B.MaxX - B.MinX) * .5 - S47Herd::COW_R) < .001 && M->TownLayer)
  {
   const double X = (B.MinX + B.MaxX) * .5, Z = (B.MinZ + B.MaxZ) * .5;
   for (const TSharedPtr<FS47Herd>& H : M->TownLayer->Herds)
    if (H) for (int32 I = 0; I < H->Count(); ++I)
    {
     FVector At;
     if (H->CowAt(I, At) && FMath::Abs(At.X - X) < .01 && FMath::Abs(At.Z - Z) < .01)
      return FString::Printf(TEXT("cow %d's circle in the truck's list (%s herd, S47Herd COW_R %.2f m), where she is"), I, *H->Area, S47Herd::COW_R);
    }
  }
  if (B.Kind == TEXT("land") && Area == TEXT("saro") && S47OpenLand::Live() && B.MinZ >= 100 && (B.MinX == B.MaxX || B.MinZ == B.MaxZ))
   return TEXT("a fence line of MakeLands' Out, stepped through in Freeform's open land (US47World::OnLand); Story's fence (roadProps.ts) holds there");
  return FString();
 }
 // Whether a point of a blocker's edge has others of its kind all round it (5 cm each way): inside them, where nothing walks or drives
 // into it (the strips a big rock is held by, S47RouteNature, meet along their sides).
 bool Inside(const TArray<FBlocker>& All, const FBlocker& Self, const FVector2D& P)
 {
  constexpr double E = .05;
  const FVector2D Round[] = { { E, 0 }, { -E, 0 }, { 0, E }, { 0, -E }, { E, E }, { E, -E }, { -E, E }, { -E, -E } };
  for (const FVector2D& D : Round)
  {
   const FVector2D Q = P + D;
   bool bIn = false;
   for (const FBlocker& O : All)
   {
    if (O.Kind != Self.Kind) continue;
    if (O.bCircle)
    {
     const double R = (O.MaxX - O.MinX) * .5;
     bIn = FVector2D::DistSquared(Q, FVector2D((O.MinX + O.MaxX) * .5, (O.MinZ + O.MaxZ) * .5)) <= R * R;
    }
    else bIn = Q.X >= O.MinX && Q.X <= O.MaxX && Q.Y >= O.MinZ && Q.Y <= O.MaxZ;
    if (bIn) break;
   }
   if (!bIn) return false;
  }
  return true;
 }
 // A walk area's name for the area the player is in, and its drive leg's.
 FString WalkOf(const FString& Area) { return Area == TEXT("diner") ? FString(TEXT("road")) : Area; }
}

TArray<FBlocker> Run(US47Main* M, double Pad, TArray<FString>& Lines)
{
 S47SaroClip::Forget();
 constexpr double MeasureReach = 200; // metres from the player: what is drawn round one
 TArray<FBlocker> Out;
 if (!M || !M->Web || !M->World || !M->Web->GetWorld()) { Lines.Add(TEXT("no world")); return Out; }
 AS47Web* W = M->Web;
 UWorld* World = W->GetWorld();
 const FString Area = W->Area(), Walk = WalkOf(Area);
 // ---- what is drawn: small things by their boxes in a grid; big ones by their vertices, finer ----
 TArray<FDrawn> Drawn;
 TMap<FIntPoint, TArray<int32>> Grid;
 TArray<FFace> Faces;
 TMap<FIntPoint, TArray<int32>> FaceGrid;
 int32 Big = 0;
 for (TActorIterator<AActor> It(World); It; ++It)
 {
  TArray<UStaticMeshComponent*> Comps;
  It->GetComponents<UStaticMeshComponent>(Comps);
  for (UStaticMeshComponent* C : Comps)
  {
   UStaticMesh* Mesh = C ? C->GetStaticMesh() : nullptr;
   if (!Mesh || !C->IsRegistered() || !C->IsVisible() || C->bHiddenInGame || !C->bRenderInMainPass) continue;
   const FBox Local = Mesh->GetBoundingBox();
   TArray<FTransform> Ts;
   if (UInstancedStaticMeshComponent* I = Cast<UInstancedStaticMeshComponent>(C))
   {
    for (int32 K = 0; K < I->GetInstanceCount(); ++K) { FTransform T; if (I->GetInstanceTransform(K, T, true) && T.GetScale3D().GetMin() > .01) Ts.Add(T); }
   }
   else Ts.Add(C->GetComponentTransform());
   for (const FTransform& T : Ts)
   {
    const FBox U = Local.TransformBy(T);
    FBox Three(ForceInit); Three += S47Three::ToThree(U.Min); Three += S47Three::ToThree(U.Max);
    const bool bBig = Three.GetSize().X > BigSize || Three.GetSize().Z > BigSize;
    if (!bBig)
    {
     const int32 Index = Drawn.Add({ Three, false, C->GetName() });
     const FIntPoint A = CellOf(Three.Min.X, Three.Min.Z, Cell), B = CellOf(Three.Max.X, Three.Max.Z, Cell);
     for (int32 X = A.X; X <= B.X; ++X) for (int32 Z = A.Y; Z <= B.Y; ++Z) Grid.FindOrAdd(FIntPoint(X, Z)).Add(Index);
     continue;
    }
    ++Big;
    bool bExact = false;
    const TArray<FVector3f>& V = S47SaroClip::Vertices(Mesh, bExact);
    TArray<uint32> Ix; Indices(Mesh, Ix);
    if (Ix.IsEmpty())
    {
     // no triangles to read: its box stands for it, as a small thing's does
     const int32 Index = Drawn.Add({ Three, true, C->GetName() });
     const FIntPoint A = CellOf(Three.Min.X, Three.Min.Z, Cell), B = CellOf(Three.Max.X, Three.Max.Z, Cell);
     for (int32 X = A.X; X <= B.X; ++X) for (int32 Z = A.Y; Z <= B.Y; ++Z) Grid.FindOrAdd(FIntPoint(X, Z)).Add(Index);
     continue;
    }
    for (int32 K = 0; K + 2 < Ix.Num(); K += 3)
    {
     if (FMath::Max3(Ix[K], Ix[K + 1], Ix[K + 2]) >= uint32(V.Num())) break;
     FFace F;
     F.A = FVector3f(S47Three::ToThree(T.TransformPosition(FVector(V[Ix[K]]))));
     F.B = FVector3f(S47Three::ToThree(T.TransformPosition(FVector(V[Ix[K + 1]]))));
     F.C = FVector3f(S47Three::ToThree(T.TransformPosition(FVector(V[Ix[K + 2]]))));
     const FVector3f N = FVector3f::CrossProduct(F.B - F.A, F.C - F.A);
     const float Len = N.Size();
     if (Len < 1e-8f || FMath::Abs(N.Y) / Len > .7f) continue; // flat: ground, a floor, a roof's top
     const int32 Index = Faces.Add(F);
     const FIntPoint A = CellOf(FMath::Min3(F.A.X, F.B.X, F.C.X), FMath::Min3(F.A.Z, F.B.Z, F.C.Z), Fine);
     const FIntPoint B = CellOf(FMath::Max3(F.A.X, F.B.X, F.C.X), FMath::Max3(F.A.Z, F.B.Z, F.C.Z), Fine);
     if ((B.X - A.X + 1) * (B.Y - A.Y + 1) > 400) continue; // a face hundreds of metres long is a backdrop, not a wall here
     for (int32 X = A.X; X <= B.X; ++X) for (int32 Z = A.Y; Z <= B.Y; ++Z) FaceGrid.FindOrAdd(FIntPoint(X, Z)).Add(Index);
    }
   }
  }
 }
 // ---- the blockers ----
 TArray<FBlocker> All;
 int32 NWalk = 0, NDrive = 0, NLand = 0;
 // (critic round 4: a walk box with no Id was listed as «walk  (…)» and could not be traced: its place in the list stands in)
 { const TArray<FS47Rect>& Walks = W->CollidersMutable(Walk); for (int32 I = 0; I < Walks.Num(); ++I) { const FS47Rect& R = Walks[I]; if (R.bEnabled) { All.Add({ TEXT("walk"), R.Id.IsEmpty() ? FString::Printf(TEXT("%s#walk%d"), *Walk, I) : R.Id, R.MinX, R.MaxX, R.MinZ, R.MaxZ, false }); ++NWalk; } } }
 if (IS47DriveArea* Leg = M->World->LegArea(Walk))
  for (int32 I = 0; I < Leg->Obstacles.Num(); ++I)
  {
   const FS47Obstacle& O = Leg->Obstacles[I];
   FBlocker Bk; Bk.Kind = TEXT("drive"); Bk.Id = FString::Printf(TEXT("%s#%d"), *Walk, I); Bk.bCircle = O.bCircle;
   if (O.bCircle) { Bk.MinX = O.X - O.R; Bk.MaxX = O.X + O.R; Bk.MinZ = O.Z - O.R; Bk.MaxZ = O.Z + O.R; }
   else { Bk.MinX = O.MinX; Bk.MaxX = O.MaxX; Bk.MinZ = O.MinZ; Bk.MaxZ = O.MaxZ; }
   if (FMath::Abs(Bk.MinX) > 1e5 || FMath::Abs(Bk.MinZ) > 1e5) continue; // retired, parked far off
   // the same box as a walk collider (Freeform copies them over): once is enough
   bool bSame = false;
   for (const FBlocker& A : All) if (A.Kind == TEXT("walk") && FMath::Abs(A.MinX - Bk.MinX) < .01 && FMath::Abs(A.MaxX - Bk.MaxX) < .01 && FMath::Abs(A.MinZ - Bk.MinZ) < .01 && FMath::Abs(A.MaxZ - Bk.MaxZ) < .01) { bSame = true; break; }
   if (!bSame) { All.Add(Bk); ++NDrive; }
  }
 if (const TArray<FS47Box2>* Land = M->World->LandOut(Area))
  for (int32 I = 0; I < Land->Num(); ++I) { const FS47Box2& Bx = (*Land)[I]; All.Add({ TEXT("land"), FString::Printf(TEXT("%s#%d"), *Area, I), Bx.MinX, Bx.MaxX, Bx.MinZ, Bx.MaxZ, false }); ++NLand; }
 // ---- each against what is drawn ----
 int32 Away = 0, Seen = 0;
 TArray<FString> KnownLines;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47Blockers), true);
 for (const FBlocker& Bk : All)
 {
  const FBox2D Rect(FVector2D(Bk.MinX - Pad, Bk.MinZ - Pad), FVector2D(Bk.MaxX + Pad, Bk.MaxZ + Pad));
  const FVector2D Mid = Rect.GetCenter();
  // (critic round 4: boxes 1.6-2 km off on Main Street were judged from the substation, where little of the town is drawn)
  if (M->Player && FVector2D::Distance(Mid, FVector2D(M->Player->Pos.X, M->Player->Pos.Z)) > MeasureReach) { ++Away; continue; }
  // The ground (or roof) under its middle: the first visible thing straight down. None: another place's, out of sight now.
  FHitResult Hit; bool bGround = false;
  FCollisionQueryParams Each = Q;
  for (int32 Try = 0; Try < 8; ++Try)
  {
   if (!World->LineTraceSingleByChannel(Hit, S47Three::ToUE(Mid.X, 400, Mid.Y), S47Three::ToUE(Mid.X, -400, Mid.Y), ECC_Visibility, Each)) break;
   UPrimitiveComponent* P = Hit.GetComponent();
   if (P && P->IsVisible() && !P->bHiddenInGame) { bGround = true; break; }
   Each.AddIgnoredComponent(P);
  }
  if (!bGround) { ++Away; continue; }
  ++Seen;
  // Whatever is drawn standing within Pad of a point: a small thing taller than a decal, or a standing face of a big mesh.
  const auto DrawnAt = [&](double PX, double PZ)
  {
   const FBox2D Sq(FVector2D(PX - Pad, PZ - Pad), FVector2D(PX + Pad, PZ + Pad));
   const FIntPoint GA = CellOf(Sq.Min.X, Sq.Min.Y, Cell), GB = CellOf(Sq.Max.X, Sq.Max.Y, Cell);
   for (int32 GX = GA.X; GX <= GB.X; ++GX) for (int32 GZ = GA.Y; GZ <= GB.Y; ++GZ)
    if (const TArray<int32>* In = Grid.Find(FIntPoint(GX, GZ)))
     for (const int32 I : *In)
     {
      const FBox& D = Drawn[I].Box;
      if (D.Max.X < Sq.Min.X || D.Min.X > Sq.Max.X || D.Max.Z < Sq.Min.Y || D.Min.Z > Sq.Max.Y) continue;
      if (D.Max.Y - D.Min.Y < .15) continue; // a decal or a flat patch on the ground
     return true;
    }
   const FIntPoint A = CellOf(Sq.Min.X, Sq.Min.Y, Fine), B = CellOf(Sq.Max.X, Sq.Max.Y, Fine);
   for (int32 X = A.X; X <= B.X; ++X) for (int32 Z = A.Y; Z <= B.Y; ++Z)
    if (const TArray<int32>* In = FaceGrid.Find(FIntPoint(X, Z)))
     for (const int32 I : *In)
     {
      const FFace& F = Faces[I];
      const FVector2D T2[3] = { { F.A.X, F.A.Z }, { F.B.X, F.B.Z }, { F.C.X, F.C.Z } };
      if (Meets(T2, Sq)) return true;
     }
   return false;
  };
  // Round its edge every half metre (the edge is what one walks or drives into): the share with nothing drawn by it.
  TArray<FVector2D> Edge;
  if (Bk.bCircle)
  {
   const double R = (Bk.MaxX - Bk.MinX) / 2;
   const int32 N = FMath::Max(6, FMath::CeilToInt(2 * PI * R / .5));
   for (int32 I = 0; I < N; ++I) Edge.Add(FVector2D(Mid.X + R * FMath::Cos(2 * PI * I / N), Mid.Y + R * FMath::Sin(2 * PI * I / N)));
  }
  else
  {
   const FVector2D C[] = { { Bk.MinX, Bk.MinZ }, { Bk.MaxX, Bk.MinZ }, { Bk.MaxX, Bk.MaxZ }, { Bk.MinX, Bk.MaxZ } };
   for (int32 S = 0; S < 4; ++S)
   {
    const FVector2D A = C[S], B = C[(S + 1) % 4];
    const int32 N = FMath::Max(2, FMath::CeilToInt(FVector2D::Distance(A, B) / .5));
    for (int32 I = 0; I < N; ++I) Edge.Add(A + (B - A) * (double(I) / N));
   }
  }
  int32 Bare = 0, Open = Edge.Num();
  FBox2D BareBox(ForceInit);
  // (BLOCKERS-20261109: a point inside others of its kind is no edge one meets, and does not count)
  for (const FVector2D& P : Edge) if (!DrawnAt(P.X, P.Y)) { if (Inside(All, Bk, P)) --Open; else { ++Bare; BareBox += P; } }
  if (Open > 0 && Bare * 4 > Open)
  {
   if (const FKnown* K = KnownOf(Walk, Bk))
   {
    KnownLines.Add(FString::Printf(TEXT("  known: %s %s (%d%% bare here): %s"), *Bk.Kind, *Bk.Id, Bare * 100 / Open, K->What));
    continue;
   }
   if (const FString What = KnownWhat(M, Area, Bk); !What.IsEmpty())
   {
    KnownLines.Add(FString::Printf(TEXT("  known: %s %s (%d%% bare here): %s"), *Bk.Kind, *Bk.Id, Bare * 100 / Open, *What));
    continue;
   }
   FBlocker Found = Bk;
   Found.Id += FString::Printf(TEXT(" (%d%% of its edge bare, at x %.2f..%.2f z %.2f..%.2f)"), Bare * 100 / Open, BareBox.Min.X, BareBox.Max.X, BareBox.Min.Y, BareBox.Max.Y);
   Out.Add(Found);
  }
 }
 Out.Sort([](const FBlocker& A, const FBlocker& B) { return (A.MaxX - A.MinX) * (A.MaxZ - A.MinZ) > (B.MaxX - B.MinX) * (B.MaxZ - B.MinZ); });
 Lines.Add(FString::Printf(TEXT("Blockers in %s (walk %s): %d walk, %d drive, %d land; %d here, %d with no visible ground under them (other places); %d with nothing drawn within %.2f m, %d more known to stand for something drawn (%d small things, %d big meshes read)"),
  *Area, *Walk, NWalk, NDrive, NLand, Seen, Away, Out.Num(), Pad, KnownLines.Num(), Drawn.Num(), Big));
 for (const FBlocker& Bk : Out)
  Lines.Add(FString::Printf(TEXT("  %s %s: x %.2f..%.2f z %.2f..%.2f (%.2f x %.2f m%s)"), *Bk.Kind, *Bk.Id, Bk.MinX, Bk.MaxX, Bk.MinZ, Bk.MaxZ, Bk.MaxX - Bk.MinX, Bk.MaxZ - Bk.MinZ, Bk.bCircle ? TEXT(", a circle") : TEXT("")));
 Lines.Append(KnownLines);
 S47SaroClip::Forget();
 return Out;
}
}
