#include "S47CalicheGround.h"
#include "S47Three.h"
#include "S47Web.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47CalicheGround, Log, All);

namespace S47CalicheGround
{
namespace
{
 const TCHAR* const Prefix = TEXT("caliche-ground:");
 constexpr double FloatFrom = .03, StemUpTo = 1.5; // metres: a gap counts from 3 cm; a stem wall covers up to a metre and a half

 struct FBuilding { FString Name; UStaticMeshComponent* C = nullptr; int32 Parent = -1; };
 // A sample on the footprint: its place on the wall line in the shell's own terms, and which way is in.
 struct FSample { FVector Local = FVector::ZeroVector, In = FVector::ZeroVector; double Bottom = 0, Ground = 0; bool bGround = false; FString On; };

 bool Under(AS47Web* W, int32 Node, int32 Group)
 {
  for (int32 Up = Node, Guard = 0; Up >= 0 && Guard < 12; ++Guard)
  {
   if (Up == Group) return true;
   const FS47WebNode* P = W->Node(Up); Up = P ? P->Parent : -1;
  }
  return false;
 }
 bool IsBuilding(const FString& Name)
 {
  // the production buildings' shells (the motel's are retired by MotelV2 and stand elsewhere), and the homes' shells
  if (Name.StartsWith(TEXT("town-production:SM_Town_"))) return Name.EndsWith(TEXT("_Shell")) && !Name.Contains(TEXT("Motel"));
  if (Name.StartsWith(TEXT("caliche-homes:"))) return Name.EndsWith(TEXT("_Shell"));
  return false;
 }
 TArray<FBuilding> Buildings(AS47Web* W)
 {
  TArray<FBuilding> Out;
  const int32 Group = W->Handle(TEXT("world.town.group"));
  for (int32 I = 0; I < W->PartCount(); ++I)
  {
   const FS47WebNode* N = W->Node(I);
   if (!N || !IsBuilding(N->Name) || !Under(W, I, Group)) continue;
   auto* C = Cast<UStaticMeshComponent>(N->Comp.Get());
   if (!C || C->IsA<UInstancedStaticMeshComponent>() || !C->GetStaticMesh()) continue;
   const FVector Size = C->GetStaticMesh()->GetBoundingBox().GetSize() * C->GetComponentScale().GetAbs();
   if (Size.X < 250 || Size.Y < 250 || Size.Z < 200) continue; // a building: at least 2.5 x 2.5 m and 2 m high
   Out.Add({ N->Name, C, N->Parent });
  }
  return Out;
 }
 // Four corners and four edge middles of the shell's bounding footprint, 25 cm in from the wall line; the ground 30 cm out.
 TArray<FSample> Samples(const UStaticMeshComponent* C)
 {
  const FBox B = C->GetStaticMesh()->GetBoundingBox();
  const FVector Mid = B.GetCenter();
  TArray<FSample> Out;
  for (int32 K = 0; K < 8; ++K)
  {
   // 0..3 corners round the footprint, 4..7 the middles of the edges after each corner
   static const FVector2D Corner[4] = { FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1) };
   const FVector2D A = Corner[K % 4], Bb = Corner[(K + 1) % 4];
   const FVector2D T = K < 4 ? A : (A + Bb) * .5;
   FSample S;
   S.Local = FVector(FMath::Lerp(B.Min.X, B.Max.X, T.X), FMath::Lerp(B.Min.Y, B.Max.Y, T.Y), B.Min.Z);
   S.In = FVector(T.X == .5 ? 0. : FMath::Sign(Mid.X - S.Local.X), T.Y == .5 ? 0. : FMath::Sign(Mid.Y - S.Local.Y), 0);
   Out.Add(S);
  }
  return Out;
 }
 // The ground straight down from a point: the first thing that is not one of the buildings, a plant, a lamp or a stem wall.
 bool Ground(UWorld* World, const FVector& FromUE, double Depth, const TArray<FBuilding>& All, double& Z, FString& On)
 {
  FCollisionQueryParams Q(SCENE_QUERY_STAT(S47CalicheGround), true);
  for (const FBuilding& B : All) Q.AddIgnoredComponent(B.C);
  for (int32 Try = 0; Try < 16; ++Try)
  {
   FHitResult Hit;
   if (!World->LineTraceSingleByChannel(Hit, FromUE, FromUE - FVector(0, 0, Depth), ECC_Visibility, Q)) return false;
   UPrimitiveComponent* P = Hit.GetComponent();
   const FString Name = P ? P->GetName() : FString();
   const bool bSkip = !P || P->IsA<UInstancedStaticMeshComponent>() || Name.StartsWith(TEXT("S47StreetLamp")) || Name.Contains(TEXT("Glass"));
   if (!bSkip) { Z = Hit.ImpactPoint.Z; On = Name; return true; }
   Q.AddIgnoredComponent(P);
  }
  return false;
 }
 void MeasureOne(UWorld* World, const FBuilding& B, const TArray<FBuilding>& All, TArray<FSample>& Out)
 {
  Out = Samples(B.C);
  const FTransform T = B.C->GetComponentTransform();
  for (FSample& S : Out)
  {
   S.Bottom = T.TransformPosition(S.Local).Z;
   const FVector Outside = T.TransformPosition(S.Local - S.In * 30.);
   S.bGround = Ground(World, FVector(Outside.X, Outside.Y, S.Bottom + 300), 1300, All, S.Ground, S.On);
  }
 }
 double Gap(const FSample& S) { return S.bGround ? (S.Bottom - S.Ground) / 100 : 0; } // metres, positive: floating
 // One edge of the footprint at its wall. The bounding box is not the wall: an awning, a sign or a step can stand out past it
 // (sweep 1: a stem wall under the bounding edge stood out in front of the Feed & Seed and past its corner). Rays at 40 cm over the
 // shell's foot, from outside in, every 25 cm along the edge, against the shell alone: the wall is where most of them stop, and it
 // runs as far as they stop there. Then the ground just outside it at its two ends and middle, and the gap there.
 struct FWallEdge { bool bWall = false; double Face = 0, From = 0, To = 0, Foot = 0, Low = 0, Worst = -1e9; FVector Along = FVector::ZeroVector, Inward = FVector::ZeroVector, Start = FVector::ZeroVector; };
 FWallEdge WallEdge(UWorld* World, const FBuilding& B, const TArray<FBuilding>& All, const TArray<FSample>& S, int32 E)
 {
  FWallEdge W;
  const FSample& A = S[E]; const FSample& M = S[E + 4]; const FSample& Z = S[(E + 1) % 4];
  const FTransform T = B.C->GetComponentTransform();
  W.Along = Z.Local - A.Local; W.Inward = M.In; W.Start = A.Local;
  const double Span = W.Along.Size();
  FCollisionQueryParams Q(SCENE_QUERY_STAT(S47CalicheWall), true);
  TArray<TPair<double, double>> Hits; // along (0..1), depth in from the bounding edge (cm)
  for (double D = 15; D <= Span - 15; D += 25)
  {
   const FVector P = A.Local + W.Along * (D / Span) + FVector(0, 0, 40);
   FHitResult H;
   if (B.C->LineTraceComponent(H, T.TransformPosition(P - W.Inward * 150), T.TransformPosition(P + W.Inward * 250), Q))
    Hits.Add({ D / Span, FVector::DotProduct(T.InverseTransformPosition(H.ImpactPoint) - P, W.Inward) });
  }
  if (Hits.Num() < 3) return W;
  TArray<double> Depths; for (const auto& H : Hits) Depths.Add(H.Value); Depths.Sort();
  W.Face = Depths[Depths.Num() / 2];
  double From = 1, To = 0; int32 On = 0;
  for (const auto& H : Hits) if (FMath::Abs(H.Value - W.Face) < 20) { From = FMath::Min(From, H.Key); To = FMath::Max(To, H.Key); ++On; }
  if (On < 3 || W.Face > 150) return W;
  W.From = From; W.To = To; W.Foot = FMath::Max3(A.Bottom, M.Bottom, Z.Bottom);
  W.Low = 1e9; int32 Found = 0;
  for (const double U : { From, (From + To) / 2, To })
  {
   const FVector Out = T.TransformPosition(A.Local + W.Along * U + W.Inward * (W.Face - 12));
   double G = 0; FString What;
   if (!Ground(World, FVector(Out.X, Out.Y, W.Foot + 300), 1300, All, G, What)) continue;
   ++Found; W.Low = FMath::Min(W.Low, G); W.Worst = FMath::Max(W.Worst, (W.Foot - G) / 100);
  }
  W.bWall = Found >= 2;
  return W;
 }
}

void Build(AS47Web* W)
{
 if (!W || FParse::Param(FCommandLine::Get(), TEXT("S47CalicheGroundOff")) || FParse::Param(FCommandLine::Get(), TEXT("S47PropsOff"))) return;
 UWorld* World = W->GetWorld();
 UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
 UMaterialInterface* Concrete = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SaroSite/Materials/M_SaroSite_Concrete.M_SaroSite_Concrete"));
 if (!World || !Cube || !Concrete) return;
 const TArray<FBuilding> All = Buildings(W);
 int32 Walls = 0;
 for (const FBuilding& B : All)
 {
  TArray<FSample> S; MeasureOne(World, B, All, S);
  const FS47WebNode* Parent = W->Node(B.Parent);
  if (!Parent || !Parent->Comp.IsValid()) continue;
  const FTransform T = B.C->GetComponentTransform();
  for (int32 E = 0; E < 4; ++E)
  {
   if (FMath::Max3(Gap(S[E]), Gap(S[E + 4]), Gap(S[(E + 1) % 4])) < FloatFrom) continue;
   const FWallEdge WE = WallEdge(World, B, All, S, E);
   if (!WE.bWall) { UE_LOG(LogS47CalicheGround, Display, TEXT("S47: Caliche ground: %s edge %d: the wall is not found by rays; no stem wall"), *B.Name, E); continue; }
   if (WE.Worst < FloatFrom || WE.Worst > StemUpTo) continue;
   const FVector Along = WE.Along, Inward = WE.Inward; const double Span = Along.Size(), Face = WE.Face, From = WE.From, To = WE.To, Foot = WE.Foot, Low = WE.Low, Worst = WE.Worst;
   const double Thick = 20, Length = (To - From) * Span + 20; // (each end ray stands 25 cm short of the next; 10 cm more each way)
   const FVector CentreLocal = WE.Start + Along * ((From + To) / 2) + Inward * (Face + 2 + Thick / 2);
   const double Top = Foot + 5, Bottom = Low - 20;
   FVector Centre = T.TransformPosition(CentreLocal); Centre.Z = (Top + Bottom) / 2;
   const bool bAlongX = FMath::Abs(Along.X) > FMath::Abs(Along.Y);
   const FVector Scale(bAlongX ? Length / 100 : Thick / 100, bAlongX ? Thick / 100 : Length / 100, (Top - Bottom) / 100);
   const FString Name = FString::Printf(TEXT("%sstem:%s:%d"), Prefix, *B.Name.Replace(TEXT(":"), TEXT("_")), E);
   auto* C = NewObject<UStaticMeshComponent>(W, *FString::Printf(TEXT("S47CalicheStem_%d"), Walls));
   C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(Parent->Comp.Get()); C->SetStaticMesh(Cube); C->SetMaterial(0, Concrete);
   C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCastShadow(true); C->RegisterComponent();
   C->SetWorldTransform(FTransform(T.GetRotation(), Centre, Scale));
   W->AdoptNode(C, B.Parent, Name);
   ++Walls;
   UE_LOG(LogS47CalicheGround, Display, TEXT("S47: Caliche ground: %s edge %d: the wall %.0f cm in from the bounding edge along %.0f%%..%.0f%%, floats up to %.0f mm; a stem wall %.2f m long, %.2f m high"),
    *B.Name, E, Face, From * 100, To * 100, Worst * 1000, Length / 100, (Top - Bottom) / 100);
  }
 }
 UE_LOG(LogS47CalicheGround, Display, TEXT("S47: Caliche ground: %d buildings measured at build, %d stem walls set"), All.Num(), Walls);
}

int32 Measure(AS47Web* W, TArray<FString>& Lines, FVector* WorstAt)
{
 double WorstGap = FloatFrom;
 UWorld* World = W ? W->GetWorld() : nullptr;
 if (!World) { Lines.Add(TEXT("Caliche ground: no world")); return 0; }
 const TArray<FBuilding> All = Buildings(W);
 // the stem walls already set, by building
 TMap<FString, int32> Stems;
 for (int32 I = 0; I < W->PartCount(); ++I)
  if (const FS47WebNode* N = W->Node(I); N && N->Name.StartsWith(Prefix))
  {
   FString Rest = N->Name.Mid(FCString::Strlen(Prefix) + 5); // after "stem:"
   int32 Cut = INDEX_NONE; Rest.FindLastChar(TEXT(':'), Cut);
   ++Stems.FindOrAdd(Cut > 0 ? Rest.Left(Cut) : Rest);
  }
 int32 Floating = 0, Stemmed = 0;
 TArray<FString> Each;
 for (const FBuilding& B : All)
 {
  TArray<FSample> S; MeasureOne(World, B, All, S);
  double Worst = -1e9, Deepest = 1e9, GroundLo = 1e9, GroundHi = -1e9; int32 Float = 0, Found = 0;
  FString Gaps;
  for (const FSample& X : S)
  {
   if (!X.bGround) { Gaps += TEXT(" -"); continue; }
   ++Found; const double G = Gap(X);
   Worst = FMath::Max(Worst, G); Deepest = FMath::Min(Deepest, G); Float += G >= FloatFrom;
   GroundLo = FMath::Min(GroundLo, X.Ground / 100); GroundHi = FMath::Max(GroundHi, X.Ground / 100);
   Gaps += FString::Printf(TEXT(" %+.0f"), G * 1000);
   if (WorstAt && G >= WorstGap) { WorstGap = G; const FVector O = B.C->GetComponentTransform().TransformPosition(X.Local - X.In * 30.); *WorstAt = S47Three::ToThree(FVector(O.X, O.Y, X.Ground)); }
  }
  const int32 Stem = Stems.FindRef(B.Name.Replace(TEXT(":"), TEXT("_")));
  Stemmed += Stem > 0;
  // at the wall itself (the bounding corners can be an awning's or a step's): the worst gap along any edge's wall
  double AtWall = -1e9; FString Walls;
  for (int32 E = 0; E < 4; ++E)
  {
   const FWallEdge WE = WallEdge(World, B, All, S, E);
   Walls += WE.bWall ? FString::Printf(TEXT(" %+.0f"), WE.Worst * 1000) : FString(TEXT(" -"));
   if (WE.bWall) AtWall = FMath::Max(AtWall, WE.Worst);
  }
  // floating: a wall over 3 cm above the ground outside it, and no stem wall under that building
  const bool bBad = AtWall >= FloatFrom && Stem == 0;
  Float = AtWall >= FloatFrom ? 1 : 0;
  Floating += bBad;
  const FVector At = S47Three::ToThree(B.C->GetComponentLocation());
  Each.Add(FString::Printf(TEXT("  %s %s at x %.1f z %.1f: foot y %.2f; at each wall (mm, + floats):%s; at the bounding corners and edge middles:%s; worst %+.0f mm, deepest %+.0f mm; ground falls %.2f m across it; %d stem walls; ground under the first corner: %s"),
   bBad ? TEXT("FLOATS") : Float > 0 ? TEXT("stemmed") : TEXT("ok"), *B.Name, At.X, At.Z, S.Num() ? S[0].Bottom / 100 : 0., *Walls, *Gaps,
   Found ? Worst * 1000 : 0., Found ? Deepest * 1000 : 0., Found ? GroundHi - GroundLo : 0., Stem, S.Num() && S[0].bGround ? *S[0].On : TEXT("nothing")));
 }
 // M4's temporary boxes and the town's own are listed, never changed.
 TArray<FString> Boxes;
 for (const TCHAR* Walk : { TEXT("road"), TEXT("diner") })
  for (const FS47Rect& R : W->CollidersMutable(Walk))
   if (R.Id.StartsWith(TEXT("temp:")) || R.Id.StartsWith(TEXT("townuse:")))
    Boxes.Add(FString::Printf(TEXT("  listed (%s walk, not changed): %s x %.1f..%.1f z %.1f..%.1f %s"), Walk, *R.Id, R.MinX, R.MaxX, R.MinZ, R.MaxZ, R.bEnabled ? TEXT("on") : TEXT("off")));
 Lines.Add(FString::Printf(TEXT("Caliche ground: %d building shells measured, %d float with nothing under them, %d stand on stem walls; %d temp:/townuse: boxes listed (the export's merged batches are not measured per building)"),
  All.Num(), Floating, Stemmed, Boxes.Num()));
 Lines.Append(Each);
 Lines.Append(Boxes);
 return Floating;
}
}
