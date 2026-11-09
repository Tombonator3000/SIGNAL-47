#include "S47Blockers.h"
#include "S47Drive.h"
#include "S47Main.h"
#include "S47SaroClip.h"
#include "S47Three.h"
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
 constexpr double Cell = 8, BigSize = 25, Fine = 2;
 struct FDrawn { FBox Box; bool bBig = false; FString Name; };
 FIntPoint CellOf(double X, double Z, double Size) { return FIntPoint(FMath::FloorToInt(X / Size), FMath::FloorToInt(Z / Size)); }
 // A walk area's name for the area the player is in, and its drive leg's.
 FString WalkOf(const FString& Area) { return Area == TEXT("diner") ? FString(TEXT("road")) : Area; }
}

TArray<FBlocker> Run(US47Main* M, double Pad, TArray<FString>& Lines)
{
 TArray<FBlocker> Out;
 if (!M || !M->Web || !M->World || !M->Web->GetWorld()) { Lines.Add(TEXT("no world")); return Out; }
 AS47Web* W = M->Web;
 UWorld* World = W->GetWorld();
 const FString Area = W->Area(), Walk = WalkOf(Area);
 // ---- what is drawn: small things by their boxes in a grid; big ones by their vertices, finer ----
 TArray<FDrawn> Drawn;
 TMap<FIntPoint, TArray<int32>> Grid;
 TMap<FIntPoint, TArray<FVector3f>> BigPoints;
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
    for (const FVector3f& L : S47SaroClip::Vertices(Mesh, bExact))
    {
     const FVector P = S47Three::ToThree(T.TransformPosition(FVector(L)));
     BigPoints.FindOrAdd(CellOf(P.X, P.Z, Fine)).Add(FVector3f(P));
    }
   }
  }
 }
 // ---- the blockers ----
 TArray<FBlocker> All;
 int32 NWalk = 0, NDrive = 0, NLand = 0;
 for (const FS47Rect& R : W->CollidersMutable(Walk)) if (R.bEnabled) { All.Add({ TEXT("walk"), R.Id, R.MinX, R.MaxX, R.MinZ, R.MaxZ, false }); ++NWalk; }
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
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47Blockers), true);
 for (const FBlocker& Bk : All)
 {
  const FBox2D Rect(FVector2D(Bk.MinX - Pad, Bk.MinZ - Pad), FVector2D(Bk.MaxX + Pad, Bk.MaxZ + Pad));
  const FVector2D Mid = Rect.GetCenter();
  // The ground (or roof) under its middle: the first visible thing straight down. None: another place's, out of sight now.
  FHitResult Hit; double GroundY = 0; bool bGround = false;
  FCollisionQueryParams Each = Q;
  for (int32 Try = 0; Try < 8; ++Try)
  {
   if (!World->LineTraceSingleByChannel(Hit, S47Three::ToUE(Mid.X, 400, Mid.Y), S47Three::ToUE(Mid.X, -400, Mid.Y), ECC_Visibility, Each)) break;
   UPrimitiveComponent* P = Hit.GetComponent();
   if (P && P->IsVisible() && !P->bHiddenInGame) { bGround = true; GroundY = S47Three::ToThree(Hit.ImpactPoint).Y; break; }
   Each.AddIgnoredComponent(P);
  }
  if (!bGround) { ++Away; continue; }
  ++Seen;
  // Whatever is drawn standing within Pad of a point (higher than a decal on the ground, lower than far overhead).
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
      if (D.Max.Y < GroundY + .15 || D.Min.Y > GroundY + 6) continue;
      return true;
     }
   const FIntPoint A = CellOf(Sq.Min.X, Sq.Min.Y, Fine), B = CellOf(Sq.Max.X, Sq.Max.Y, Fine);
   for (int32 X = A.X; X <= B.X; ++X) for (int32 Z = A.Y; Z <= B.Y; ++Z)
    if (const TArray<FVector3f>* In = BigPoints.Find(FIntPoint(X, Z)))
     for (const FVector3f& P : *In)
      if (P.X >= Sq.Min.X && P.X <= Sq.Max.X && P.Z >= Sq.Min.Y && P.Z <= Sq.Max.Y && P.Y > GroundY + .25 && P.Y < GroundY + 6) return true;
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
  int32 Bare = 0;
  FBox2D BareBox(ForceInit);
  for (const FVector2D& P : Edge) if (!DrawnAt(P.X, P.Y)) { ++Bare; BareBox += P; }
  if (Bare * 4 > Edge.Num())
  {
   FBlocker Found = Bk;
   Found.Id += FString::Printf(TEXT(" (%d%% of its edge bare, at x %.2f..%.2f z %.2f..%.2f)"), Bare * 100 / Edge.Num(), BareBox.Min.X, BareBox.Max.X, BareBox.Min.Y, BareBox.Max.Y);
   Out.Add(Found);
  }
 }
 Out.Sort([](const FBlocker& A, const FBlocker& B) { return (A.MaxX - A.MinX) * (A.MaxZ - A.MinZ) > (B.MaxX - B.MinX) * (B.MaxZ - B.MinZ); });
 Lines.Add(FString::Printf(TEXT("Blockers in %s (walk %s): %d walk, %d drive, %d land; %d here, %d with no visible ground under them (other places); %d with nothing drawn within %.2f m (%d small things, %d big meshes read)"),
  *Area, *Walk, NWalk, NDrive, NLand, Seen, Away, Out.Num(), Pad, Drawn.Num(), Big));
 for (const FBlocker& Bk : Out)
  Lines.Add(FString::Printf(TEXT("  %s %s: x %.2f..%.2f z %.2f..%.2f (%.2f x %.2f m%s)"), *Bk.Kind, *Bk.Id, Bk.MinX, Bk.MaxX, Bk.MinZ, Bk.MaxZ, Bk.MaxX - Bk.MinX, Bk.MaxZ - Bk.MinZ, Bk.bCircle ? TEXT(", a circle") : TEXT("")));
 return Out;
}
}
