#include "S47ThreatNavigation.h"

namespace
{
 constexpr int32 MaxPoints = 96, MaxSeeds = 32, MaxSeedChecks = 64, MaxCornerChecks = 256;
 constexpr double InfiniteCost = 1e100;

 bool NavFinite(const FVector2D& P) { return FMath::IsFinite(P.X) && FMath::IsFinite(P.Y); }

 bool NavValid(const FS47Rect& B)
 {
  // A line or point collider is valid: the body clearance gives it physical width.
  return !B.bEnabled || (FMath::IsFinite(B.MinX) && FMath::IsFinite(B.MaxX) &&
   FMath::IsFinite(B.MinZ) && FMath::IsFinite(B.MaxZ) && B.MinX <= B.MaxX && B.MinZ <= B.MaxZ);
 }

 bool NavContains(const FS47Rect& B, const FVector2D& P, double Pad)
 {
  return B.bEnabled && P.X >= B.MinX - Pad && P.X <= B.MaxX + Pad &&
   P.Y >= B.MinZ - Pad && P.Y <= B.MaxZ + Pad;
 }

 bool NavPointClear(const FVector2D& P, const TArray<FS47Rect>& Blocks, double Pad)
 {
  if (!NavFinite(P)) return false;
  for (const FS47Rect& B : Blocks) if (NavContains(B, P, Pad)) return false;
  return true;
 }

 bool NavTouches(const FVector2D& A, const FVector2D& B, const FS47Rect& R, double Pad)
 {
  // Liang-Barsky, including tangency: a body must stay outside the expanded rectangle.
  if (!R.bEnabled) return false;
  double Lo = 0, Hi = 1;
  const FVector2D D = B - A;
  auto Clip = [&Lo, &Hi](double P, double Q)
  {
   if (FMath::Abs(P) < 1e-12) return Q >= 0;
   const double T = Q / P;
   if (P < 0) { if (T > Hi) return false; Lo = FMath::Max(Lo, T); }
   else { if (T < Lo) return false; Hi = FMath::Min(Hi, T); }
   return true;
  };
  return Clip(-D.X, A.X - (R.MinX - Pad)) && Clip(D.X, R.MaxX + Pad - A.X) &&
   Clip(-D.Y, A.Y - (R.MinZ - Pad)) && Clip(D.Y, R.MaxZ + Pad - A.Y);
 }

 bool NavClear(const FVector2D& A, const FVector2D& B, const TArray<FS47Rect>& Blocks, double Pad)
 {
  for (const FS47Rect& R : Blocks) if (NavTouches(A, B, R, Pad)) return false;
  return true;
 }

 struct FNavCandidate { FVector2D At; double Score; };
 bool NavEarlier(const FNavCandidate& A, const FNavCandidate& B)
 {
  if (A.Score != B.Score) return A.Score < B.Score;
  return A.At.X != B.At.X ? A.At.X < B.At.X : A.At.Y < B.At.Y;
 }
}

TArray<FVector2D> FS47ThreatNavigation::FindPath(const FVector2D& From, const FVector2D& To,
 const TArray<FVector2D>& Seeds, const TArray<FS47Rect>& Blocks, double Clearance,
 TFunctionRef<bool(const FVector2D&, const FVector2D&)> CanTraverse)
{
 if (!NavFinite(From) || !NavFinite(To) || !FMath::IsFinite(Clearance) || Clearance < 0) return {};
 for (const FS47Rect& B : Blocks) if (!NavValid(B)) return {};
 if (!NavPointClear(From, Blocks, Clearance) || !NavPointClear(To, Blocks, Clearance)) return {};
 if (!CanTraverse(From, From) || !CanTraverse(To, To)) return {};
 if (NavClear(From, To, Blocks, Clearance) && CanTraverse(From, To)) return {To};

 const double Direct = FVector2D::Distance(From, To);
 if (!FMath::IsFinite(Direct)) return {};
 // Nearby measured passages survive long, thin blockers. Faraway background geometry cannot
 // produce an unbounded number of expensive walkability queries.
 const double Horizon = Direct + 2 * FMath::Max(32.0, Direct);
 TArray<FNavCandidate> SeedCandidates, Corners;
 auto Candidate = [&From, &To, Horizon](TArray<FNavCandidate>& Into, const FVector2D& P)
 {
  if (!NavFinite(P)) return;
  const double Score = FVector2D::Distance(From, P) + FVector2D::Distance(P, To);
  if (FMath::IsFinite(Score) && Score <= Horizon) Into.Add({P, Score});
 };
 for (const FVector2D& P : Seeds) Candidate(SeedCandidates, P);
 const double Pad = Clearance + .02;
 for (const FS47Rect& B : Blocks)
 {
  if (!B.bEnabled) continue;
  Candidate(Corners, FVector2D(B.MinX - Pad, B.MinZ - Pad));
  Candidate(Corners, FVector2D(B.MinX - Pad, B.MaxZ + Pad));
  Candidate(Corners, FVector2D(B.MaxX + Pad, B.MinZ - Pad));
  Candidate(Corners, FVector2D(B.MaxX + Pad, B.MaxZ + Pad));
 }
 SeedCandidates.Sort(NavEarlier); Corners.Sort(NavEarlier);
 TArray<FVector2D> Points = {From, To};
 auto Add = [&Points, &Blocks, Clearance, &CanTraverse](const FVector2D& P)
 {
  for (const FVector2D& Existing : Points) if (Existing.Equals(P, 1e-4)) return false;
  if (!NavPointClear(P, Blocks, Clearance) || !CanTraverse(P, P)) return false;
  Points.Add(P);
  return true;
 };
 int32 SeedCount = 0;
 for (int32 I = 0; I < FMath::Min(MaxSeedChecks, SeedCandidates.Num()) && SeedCount < MaxSeeds; ++I)
  if (Add(SeedCandidates[I].At)) ++SeedCount;
 for (int32 I = 0; I < FMath::Min(MaxCornerChecks, Corners.Num()) && Points.Num() < MaxPoints; ++I) Add(Corners[I].At);

 const int32 Count = Points.Num();
 TArray<double> Cost; Cost.Init(InfiniteCost, Count); Cost[0] = 0;
 TArray<int32> Previous; Previous.Init(INDEX_NONE, Count);
 TArray<bool> Done; Done.Init(false, Count);
 // A directed cache preserves slope/step rules in adapters whose A->B differs from B->A.
 TArray<int8> Edge; Edge.Init(-1, Count * Count);
 auto CanUse = [&Points, &Edge, &Blocks, Clearance, Count, &CanTraverse](int32 A, int32 B)
 {
  int8& Cached = Edge[A * Count + B];
  if (Cached < 0) Cached = NavClear(Points[A], Points[B], Blocks, Clearance) && CanTraverse(Points[A], Points[B]) ? 1 : 0;
  return Cached == 1;
 };
 for (;;)
 {
  int32 U = INDEX_NONE;
  for (int32 I = 0; I < Count; ++I)
   if (!Done[I] && Cost[I] < InfiniteCost && (U == INDEX_NONE || Cost[I] < Cost[U])) U = I;
  if (U == INDEX_NONE || U == 1) break;
  Done[U] = true;
  for (int32 V = 1; V < Count; ++V)
  {
   if (Done[V] || V == U) continue;
   const double NextCost = Cost[U] + FVector2D::Distance(Points[U], Points[V]);
   if (NextCost >= Cost[V] || !CanUse(U, V)) continue;
   Cost[V] = NextCost; Previous[V] = U;
  }
 }
 if (Previous[1] == INDEX_NONE) return {};
 TArray<FVector2D> Result;
 for (int32 I = 1, Steps = 0; I != 0 && Steps < Count; ++Steps)
 {
  if (!Points.IsValidIndex(I)) return {};
  Result.Insert(Points[I], 0); I = Previous[I];
  if (I == 0) return Result;
 }
 return {}; // no unverified partial route if the predecessor chain is inconsistent
}

bool FS47ThreatNavigation::SelfTest(TArray<FString>& Lines)
{
 bool Good = true;
 auto Check = [&Good, &Lines](bool Pass, const TCHAR* Why)
 { Lines.Add(FString::Printf(TEXT("%s threat navigation: %s"), Pass ? TEXT("ok") : TEXT("FAIL"), Why)); Good &= Pass; };
 auto Box = [](double X0, double X1, double Z0, double Z1)
 { FS47Rect R; R.MinX = X0; R.MaxX = X1; R.MinZ = Z0; R.MaxZ = Z1; return R; };
 auto Domain = [](const FVector2D& A, const FVector2D& B)
 { return NavFinite(A) && NavFinite(B) && FMath::Abs(A.X) <= 8 && FMath::Abs(A.Y) <= 8 && FMath::Abs(B.X) <= 8 && FMath::Abs(B.Y) <= 8; };
 // Independent dense occupancy sampling checks the produced route, not its predecessor tree.
 auto Safe = [](const FVector2D& Start, const FVector2D& Goal, const TArray<FVector2D>& Path, const TArray<FS47Rect>& Blocks, double Pad)
 {
  if (Path.IsEmpty() || !Path.Last().Equals(Goal, 1e-6)) return false;
  FVector2D A = Start;
  for (const FVector2D& B : Path)
  {
   const int32 Samples = FMath::Max(1, FMath::CeilToInt(FVector2D::Distance(A, B) / .025));
   for (int32 I = 0; I <= Samples; ++I)
   {
    const FVector2D P = A + (B - A) * (double(I) / Samples);
    if (FMath::Abs(P.X) > 8 || FMath::Abs(P.Y) > 8) return false;
    for (const FS47Rect& R : Blocks)
     if (R.bEnabled && P.X >= R.MinX - Pad && P.X <= R.MaxX + Pad && P.Y >= R.MinZ - Pad && P.Y <= R.MaxZ + Pad) return false;
   }
   A = B;
  }
  return true;
 };
 auto Length = [](const FVector2D& Start, const TArray<FVector2D>& Path)
 { double Sum = 0; FVector2D P = Start; for (const FVector2D& Q : Path) { Sum += FVector2D::Distance(P, Q); P = Q; } return Sum; };

 const FVector2D Start(-4, 0), Goal(4, 0);
 const TArray<FVector2D> NoSeeds;
 TArray<FS47Rect> Blocks = {Box(-1, 1, -2, 2)};
 const TArray<FVector2D> Around = FindPath(Start, Goal, NoSeeds, Blocks, .3, Domain);
 Check(Around.Num() >= 3 && Safe(Start, Goal, Around, Blocks, .3) && Length(Start, Around) > 8.5,
  TEXT("a rectangle requires a real, body-clear corner detour rather than a straight fallback"));
 Check(Around == FindPath(Start, Goal, NoSeeds, Blocks, .3, Domain), TEXT("equal-cost detours are deterministic"));

 Blocks = {Box(-3, -2, -3, 3), Box(2, 3, -3, 3), Box(-3, 3, -3, -2)};
 const FVector2D InsideU(0, 0), BelowU(0, -5);
 const TArray<FVector2D> OutOfU = FindPath(InsideU, BelowU, NoSeeds, Blocks, .3, Domain);
 bool LeftByMouth = false; for (const FVector2D& P : OutOfU) LeftByMouth |= P.Y > 3.3;
 Check(LeftByMouth && Safe(InsideU, BelowU, OutOfU, Blocks, .3),
  TEXT("a U-shaped obstacle is escaped through its mouth before going around its closed side"));

 Blocks = {Box(-2.5, -2, -2.5, 2.5), Box(2, 2.5, -2.5, 2.5), Box(-2.5, 2.5, -2.5, -2), Box(-2.5, 2.5, 2, 2.5)};
 Check(FindPath(FVector2D(-5, 0), FVector2D(0, 0), NoSeeds, Blocks, .2, Domain).IsEmpty(),
  TEXT("an enclosed but individually free goal is reported unreachable without crossing its walls"));
 Check(FindPath(FVector2D(-5, 0), FVector2D(2.25, 0), NoSeeds, Blocks, .2, Domain).IsEmpty(),
  TEXT("a goal inside a collider is not returned as an apparent partial success"));

 Blocks.Reset();
 const TArray<FVector2D> Open = FindPath(Start, Goal, NoSeeds, Blocks, .3, Domain);
 Blocks.Add(Box(-.25, .25, -3, 3));
 const TArray<FVector2D> Changed = FindPath(Start, Goal, NoSeeds, Blocks, .3, Domain);
 Check(Open.Num() == 1 && !Safe(Start, Goal, Open, Blocks, .3) && Safe(Start, Goal, Changed, Blocks, .3) && Changed.Num() >= 3,
  TEXT("a newly added obstacle invalidates the old direct route and produces a fresh detour"));
 Blocks[0] = Box(-.25, .25, -9, 9);
 Check(FindPath(Start, Goal, NoSeeds, Blocks, .3, Domain).IsEmpty(),
  TEXT("a barrier spanning all traversable ground stays blocked instead of teleporting across it"));

 Blocks = {Box(-2, 2, .45, 9), Box(-2, 2, -9, -.45)};
 const TArray<FVector2D> Narrow = FindPath(Start, Goal, NoSeeds, Blocks, .3, Domain);
 Check(Safe(Start, Goal, Narrow, Blocks, .3) && FindPath(Start, Goal, NoSeeds, Blocks, .5, Domain).IsEmpty(),
  TEXT("a passage wider than the body is usable and closes when its clearance is insufficient"));

 FS47Rect Thin = Box(0, 0, -3, 3);
 Blocks = {Thin};
 const TArray<FVector2D> ThinPath = FindPath(Start, Goal, NoSeeds, Blocks, .3, Domain);
 Check(ThinPath.Num() >= 3 && Safe(Start, Goal, ThinPath, Blocks, .3), TEXT("a zero-width enabled blocker still has body clearance"));
 Blocks[0].bEnabled = false;
 Check(FindPath(Start, Goal, NoSeeds, Blocks, .3, Domain).Num() == 1, TEXT("disabled obstacles do not leave stale blocked edges"));
 Blocks[0].bEnabled = true; Blocks[0].MinX = 1; Blocks[0].MaxX = -1;
 Check(FindPath(Start, Goal, NoSeeds, Blocks, .3, Domain).IsEmpty(), TEXT("malformed enabled bounds fail closed"));

 const FS47Rect Native = Box(-1, 1, -2, 2);
 auto NativeClear = [&Native, &Domain](const FVector2D& A, const FVector2D& B)
 { return Domain(A, B) && !NavTouches(A, B, Native, .3); };
 const TArray<FVector2D> Passages = {FVector2D(-1.5, -2.5), FVector2D(1.5, -2.5)};
 Blocks.Reset();
 const TArray<FVector2D> ViaSeeds = FindPath(Start, Goal, Passages, Blocks, .3, NativeClear);
 const TArray<FS47Rect> NativeBlocks = {Native};
 Check(ViaSeeds.Num() >= 3 && Safe(Start, Goal, ViaSeeds, NativeBlocks, .3),
  TEXT("measured passage seeds obey additional world-adapter collision checks on every edge"));
 return Good;
}
