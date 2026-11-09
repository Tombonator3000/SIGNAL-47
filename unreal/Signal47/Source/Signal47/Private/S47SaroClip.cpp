#include "S47SaroClip.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "StaticMeshResources.h"
#if WITH_EDITOR
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#endif

namespace S47SaroClip
{
namespace
{
 // A wall: its box in three metres (x, y up, z), the openings cut through it, and its thin axis (0 x, 2 z).
 struct FSlab { const TCHAR* Name; FBox Box; TArray<FBox> Holes; int32 Thin = 0; };
 FBox B(double X0, double X1, double Y0, double Y1, double Z0, double Z1) { return FBox(FVector(X0, Y0, Z0), FVector(X1, Y1, Z1)); }
 // A hole through a wall along x (on a z wall: x range) or along z (on an x wall: z range), whatever its depth.
 FBox HoleX(double X0, double X1, double Y0, double Y1) { return B(X0, X1, Y0, Y1, -1e4, 1e4); }
 FBox HoleZ(double Z0, double Z1, double Y0, double Y1) { return B(-1e4, 1e4, Y0, Y1, Z0, Z1); }
 TArray<FSlab> Table()
 {
  TArray<FSlab> S;
  const auto Add = [&S](const TCHAR* Name, const FBox& Box, TArray<FBox> Holes = {})
  {
   FSlab W; W.Name = Name; W.Box = Box; W.Holes = MoveTemp(Holes);
   const FVector E = Box.GetSize(); W.Thin = E.X < E.Z ? 0 : 2; S.Add(MoveTemp(W));
  };
  // ---- the control room (web/src/world/ControlRoom.ts shell(): 0.3 m walls centred on x +-6.15 and z +-4.65, 3.2 m high) ----
  Add(TEXT("control north"), B(-6.3, 6.3, 0, 3.2, -4.8, -4.5), { HoleX(-5.4, 5.4, .95, 2.75) });
  Add(TEXT("control south"), B(-6.3, 6.3, 0, 3.2, 4.5, 4.8), { HoleX(-2.9, -1.9, 0, 2.12) });
  Add(TEXT("control west"), B(-6.3, -6.0, 0, 3.2, -4.8, 4.8));
  Add(TEXT("control east"), B(6.0, 6.3, 0, 3.2, -4.8, 4.8), { HoleZ(1.1, 2.1, 0, 2.12) });
  // ---- the annex (Annex.ts shell(): 2.8 m) ----
  Add(TEXT("annex west"), B(-6.3, -6.0, 0, 2.8, 4.8, 10.8), { HoleZ(5.22, 6.18, 0, 2.12) });
  // Dale's window (S47OfficeWindow.cpp) goes through it
  Add(TEXT("annex south"), B(-6.3, 2.3, 0, 2.8, 10.5, 10.8), { HoleX(-.55, 1.30, 1.08, 2.40) });
  Add(TEXT("annex east"), B(2.0, 2.3, 0, 2.8, 4.8, 10.8));
  // corridor / records partition with the records doorway; east of -0.94 the office's own partitions (S47Office.cpp)
  Add(TEXT("records partition"), B(-6.0, -.94, 0, 2.8, 6.6, 6.8), { HoleX(-4.4, -3.4, 0, 2.1) });
  Add(TEXT("records / office wall"), B(-1.0, -.82, 0, 2.8, 6.8, 10.5));
  // ---- Dale's office inside the annex (S47Office.cpp's colliders and boxes; the room is 0.5 m back from the corridor) ----
  Add(TEXT("office east"), B(1.77, 1.89, 0, 2.8, 7.3, 10.43));
  Add(TEXT("office rear lining"), B(-.82, 1.77, 0, 2.8, 10.31, 10.43), { HoleX(-.55, 1.30, 1.08, 2.40) });
  Add(TEXT("office front"), B(-.94, 2.0, 0, 2.8, 7.1, 7.3), { HoleX(-.08, 1.0, 0, 2.12) });
  // ---- the photo lab (ServiceYard.ts: 0.2 m walls inside X0 12.6, X1 19.2, Z0 -6, Z1 1; door in the west wall) ----
  Add(TEXT("lab west"), B(12.6, 12.8, 0, 3.0, -6.0, 1.0), { HoleZ(-1.7, -.7, 0, 2.1) });
  Add(TEXT("lab east"), B(19.0, 19.2, 0, 3.0, -6.0, 1.0));
  Add(TEXT("lab north"), B(12.6, 19.2, 0, 3.0, -6.0, -5.8));
  Add(TEXT("lab south"), B(12.6, 19.2, 0, 3.0, .8, 1.0));
  // ---- the east wing (S47EastWing.cpp: X0 20.6, X1 38.6, Z0 -5.5, Z1 5.5, T .25, H 3.6, partitions .16 thick to the ceiling 3.32) ----
  Add(TEXT("wing north"), B(20.6, 38.6, 0, 3.6, -5.5, -5.25), { HoleX(21.4, 23.0, 1.0, 2.25), HoleX(23.6, 25.2, 1.0, 2.25), HoleX(27.0, 28.6, 1.0, 2.25),
   HoleX(29.2, 30.8, 1.0, 2.25), HoleX(33.0, 34.6, 1.6, 2.4), HoleX(35.6, 37.2, 1.6, 2.4) });
  Add(TEXT("wing south"), B(20.6, 38.6, 0, 3.6, 5.25, 5.5), { HoleX(21.3, 22.4, 0, 2.15), HoleX(33.2, 37.6, 0, 3.0) });
  Add(TEXT("wing west"), B(20.6, 20.85, 0, 3.6, -5.5, 5.5));
  Add(TEXT("wing east"), B(38.35, 38.6, 0, 3.6, -5.5, 5.5), { HoleZ(1.2, 2.3, 0, 2.15) });
  Add(TEXT("wing lab partition"), B(25.92, 26.08, 0, 3.32, -5.25, 5.25), { HoleZ(2.4, 3.4, 0, 2.1) });
  Add(TEXT("wing garage partition"), B(31.52, 31.68, 0, 3.32, -5.25, 5.25), { HoleZ(-3.0, -2.0, 0, 2.1), HoleZ(2.9, 3.9, 0, 2.1) });
  Add(TEXT("wing store partition"), B(26.08, 31.52, 0, 3.32, .42, .58), { HoleX(28.3, 29.3, 0, 2.1) });
  return S;
 }
 // The buildings' region: what lies wholly outside it is not measured.
 const FBox Region = B(-7.5, 40.0, -1.0, 4.5, -7.5, 12.0);

 struct FCached { TArray<FVector3f> V; bool bExact = false; };
 TMap<const UStaticMesh*, FCached>& Cache() { static TMap<const UStaticMesh*, FCached> C; return C; }

 FString Describe(UStaticMeshComponent* C, AS47Web* W, int32 Instance)
 {
  FString S = C->GetName();
  if (W) { const int32 N = W->NodeOf(C); if (const FS47WebNode* Node = W->Node(N)) S += FString::Printf(TEXT(" [%s]"), Node->Name.IsEmpty() ? *FString::Printf(TEXT("N%d"), N) : *Node->Name); }
  if (Instance >= 0) S += FString::Printf(TEXT(" #%d"), Instance);
  if (const UStaticMesh* M = C->GetStaticMesh()) S += TEXT(" ") + M->GetName();
  return S;
 }
}

const TArray<FVector3f>& Vertices(UStaticMesh* Mesh, bool& bExact)
{
 FCached& C = Cache().FindOrAdd(Mesh);
 if (C.V.IsEmpty() && Mesh)
 {
  if (const FStaticMeshRenderData* RD = Mesh->GetRenderData(); RD && RD->LODResources.Num() > 0)
  {
   const FPositionVertexBuffer& P = RD->LODResources[0].VertexBuffers.PositionVertexBuffer;
   if (P.GetNumVertices() > 0 && P.GetVertexData())
   {
    C.V.Reserve(P.GetNumVertices());
    for (uint32 I = 0; I < P.GetNumVertices(); ++I) C.V.Add(P.VertexPosition(I));
    C.bExact = true;
   }
  }
#if WITH_EDITOR
  if (C.V.IsEmpty() && Mesh->GetNumSourceModels() > 0)
   if (const FMeshDescription* D = Mesh->GetMeshDescription(0))
   {
    const FStaticMeshConstAttributes A(*D);
    const auto Pos = A.GetVertexPositions();
    const FVector3f Scale = FVector3f(Mesh->GetSourceModel(0).BuildSettings.BuildScale3D);
    for (const FVertexID V : D->Vertices().GetElementIDs()) C.V.Add(Pos[V] * Scale);
    C.bExact = C.V.Num() > 0;
   }
#endif
  if (C.V.IsEmpty())
  {
   const FBox Bx = Mesh->GetBoundingBox();
   for (int32 K = 0; K < 8; ++K) C.V.Add(FVector3f(K & 1 ? Bx.Max.X : Bx.Min.X, K & 2 ? Bx.Max.Y : Bx.Min.Y, K & 4 ? Bx.Max.Z : Bx.Min.Z));
   C.bExact = false;
  }
 }
 bExact = C.bExact;
 return C.V;
}

void Walls(TArray<TPair<FString, FBox>>& Out)
{
 for (const FSlab& S : Table()) Out.Add({ S.Name, S.Box });
}

void Forget() { Cache().Reset(); }

TArray<FHit> Run(UWorld* World, double Tol, TArray<FString>& Lines, const FString& DumpDir)
{
 TArray<FHit> Hits;
 if (!World) { Lines.Add(TEXT("no world")); return Hits; }
 Cache().Reset();
 const TArray<FSlab> Slabs = Table();
 AS47Web* W = nullptr;
 if (TActorIterator<AS47Web> It(World); It) W = *It;
 int32 Measured = 0, Rough = 0, Verts = 0;
 TArray<float> Dump;
 TArray<TSharedPtr<FJsonValue>> DumpIndex;
 const bool bDump = !DumpDir.IsEmpty();
 // Find: the vertex's slab and depth (the nearer face along the slab's thin axis), or -1.
 const auto Inside = [&Slabs, Tol](const FVector& P, int32& Which)
 {
  for (int32 I = 0; I < Slabs.Num(); ++I)
  {
   const FSlab& S = Slabs[I];
   const int32 A = S.Thin, O = S.Thin == 0 ? 2 : 0;
   const double Lo = S.Box.Min[A], Hi = S.Box.Max[A];
   const double Depth = FMath::Min(P[A] - Lo, Hi - P[A]);
   if (Depth <= Tol) continue;
   // along the wall and up it: a little inside, so a perpendicular wall's corner or a floor never counts
   if (P[O] <= S.Box.Min[O] + .005 || P[O] >= S.Box.Max[O] - .005 || P.Y <= S.Box.Min.Y + .005 || P.Y >= S.Box.Max.Y - .005) continue;
   bool bHole = false;
   for (const FBox& H : S.Holes) if (H.ExpandBy(.005).IsInside(P)) { bHole = true; break; }
   if (bHole) continue;
   Which = I; return Depth;
  }
  return -1.;
 };
 TMap<FString, int32> HitIndex;
 for (TActorIterator<AActor> It(World); It; ++It)
 {
  TArray<UStaticMeshComponent*> Comps;
  It->GetComponents<UStaticMeshComponent>(Comps);
  for (UStaticMeshComponent* C : Comps)
  {
   UStaticMesh* Mesh = C ? C->GetStaticMesh() : nullptr;
   if (!Mesh || !C->IsRegistered() || !C->IsVisible() || C->bHiddenInGame || !C->bRenderInMainPass) continue;
   FBox Three(ForceInit); Three += S47Three::ToThree(C->Bounds.GetBox().Min); Three += S47Three::ToThree(C->Bounds.GetBox().Max);
   if (!Three.Intersect(Region)) continue;
   bool bExact = false;
   const TArray<FVector3f>& V = Vertices(Mesh, bExact);
   TArray<FTransform> Ts;
   int32 InstanceBase = -1;
   if (UInstancedStaticMeshComponent* I = Cast<UInstancedStaticMeshComponent>(C))
   {
    InstanceBase = 0;
    for (int32 K = 0; K < I->GetInstanceCount(); ++K) { FTransform T; if (I->GetInstanceTransform(K, T, true) && T.GetScale3D().GetMin() > .01) Ts.Add(T); else Ts.Add(FTransform(FQuat::Identity, FVector(1e9), FVector(0))); }
   }
   else Ts.Add(C->GetComponentTransform());
   ++Measured; Rough += !bExact;
   for (int32 K = 0; K < Ts.Num(); ++K)
   {
    if (Ts[K].GetScale3D().IsNearlyZero()) continue;
    const int32 DumpStart = Dump.Num() / 3;
    for (const FVector3f& L : V)
    {
     const FVector P = S47Three::ToThree(Ts[K].TransformPosition(FVector(L)));
     ++Verts;
     if (bDump && Region.IsInside(P)) { Dump.Add(P.X); Dump.Add(P.Y); Dump.Add(P.Z); }
     int32 Which = -1;
     const double D = Inside(P, Which);
     if (D <= 0) continue;
     const FString Thing = Describe(C, W, InstanceBase >= 0 ? K : -1);
     const FString Key = Thing + TEXT("|") + Slabs[Which].Name;
     int32* At = HitIndex.Find(Key);
     if (!At) { FHit H; H.Thing = Thing; H.Wall = Slabs[Which].Name; At = &HitIndex.Add(Key, Hits.Add(H)); }
     FHit& H = Hits[*At]; ++H.Count; H.Depth = FMath::Max(H.Depth, D); H.Where += P;
    }
    if (bDump && Dump.Num() / 3 > DumpStart)
    {
     auto O = MakeShared<FJsonObject>();
     O->SetStringField(TEXT("thing"), Describe(C, W, InstanceBase >= 0 ? K : -1));
     O->SetStringField(TEXT("mesh"), Mesh->GetPathName());
     O->SetStringField(TEXT("actor"), It->GetName());
     O->SetBoolField(TEXT("exact"), bExact);
     O->SetBoolField(TEXT("collides"), C->IsCollisionEnabled());
     O->SetNumberField(TEXT("first"), DumpStart);
     O->SetNumberField(TEXT("count"), Dump.Num() / 3 - DumpStart);
     DumpIndex.Add(MakeShared<FJsonValueObject>(O));
    }
   }
  }
 }
 Hits.Sort([](const FHit& A, const FHit& Bh) { return A.Depth > Bh.Depth; });
 Lines.Add(FString::Printf(TEXT("SARO walls: %d slabs; %d meshes measured (%d only by their bounds), %d vertices; %d things into a wall deeper than %.0f mm"),
  Slabs.Num(), Measured, Rough, Verts, Hits.Num(), Tol * 1000));
 for (const FHit& H : Hits)
  Lines.Add(FString::Printf(TEXT("  %.0f mm into %s: %s, %d vertices at x %.2f..%.2f y %.2f..%.2f z %.2f..%.2f"), H.Depth * 1000, *H.Wall, *H.Thing, H.Count,
   H.Where.Min.X, H.Where.Max.X, H.Where.Min.Y, H.Where.Max.Y, H.Where.Min.Z, H.Where.Max.Z));
 if (bDump)
 {
  IFileManager::Get().MakeDirectory(*DumpDir, true);
  TArray<uint8> Bytes; Bytes.Append(reinterpret_cast<const uint8*>(Dump.GetData()), Dump.Num() * sizeof(float));
  FFileHelper::SaveArrayToFile(Bytes, *(DumpDir / TEXT("saro-geo.bin")));
  auto Root = MakeShared<FJsonObject>(); Root->SetArrayField(TEXT("parts"), DumpIndex);
  TArray<TSharedPtr<FJsonValue>> WallList;
  for (const FSlab& S : Slabs)
  {
   auto O = MakeShared<FJsonObject>(); O->SetStringField(TEXT("name"), S.Name);
   O->SetArrayField(TEXT("min"), { MakeShared<FJsonValueNumber>(S.Box.Min.X), MakeShared<FJsonValueNumber>(S.Box.Min.Y), MakeShared<FJsonValueNumber>(S.Box.Min.Z) });
   O->SetArrayField(TEXT("max"), { MakeShared<FJsonValueNumber>(S.Box.Max.X), MakeShared<FJsonValueNumber>(S.Box.Max.Y), MakeShared<FJsonValueNumber>(S.Box.Max.Z) });
   TArray<TSharedPtr<FJsonValue>> Holes;
   for (const FBox& H : S.Holes)
    Holes.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{ MakeShared<FJsonValueNumber>(H.Min.X), MakeShared<FJsonValueNumber>(H.Min.Y), MakeShared<FJsonValueNumber>(H.Min.Z),
     MakeShared<FJsonValueNumber>(H.Max.X), MakeShared<FJsonValueNumber>(H.Max.Y), MakeShared<FJsonValueNumber>(H.Max.Z) }));
   O->SetArrayField(TEXT("holes"), Holes);
   WallList.Add(MakeShared<FJsonValueObject>(O));
  }
  Root->SetArrayField(TEXT("walls"), WallList);
  FString Text; const auto Writer = TJsonWriterFactory<>::Create(&Text); FJsonSerializer::Serialize(Root, Writer);
  FFileHelper::SaveStringToFile(Text, *(DumpDir / TEXT("saro-geo.json")));
  Lines.Add(FString::Printf(TEXT("SARO walls: %d vertices of %d parts written to %s"), Dump.Num() / 3, DumpIndex.Num(), *DumpDir));
 }
 Cache().Reset(); // (critic round 4: twelve million vertices were kept after the measure)
 return Hits;
}
}
