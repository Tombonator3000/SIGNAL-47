#include "S47SaroFixes.h"

#include "S47FreeformTerrain.h"
#include "S47Main.h"
#include "S47SaroLegacyCleanup.h"
#include "S47SaroSurfaces.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47World.h"
#include "CollisionQueryParams.h"
#include "Components/BoxComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "RenderUtils.h"
#include "StaticMeshResources.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SaroFixes, Log, All);

// Tom's test of d36, 9 November 2026 (his pictures): the ramp to the entrance flat and pale (PT-RAMP), the walks flat concrete strips
// that look laid on the desert (PT-PATHS), the stop sign by SARO (PT-STOP), the hall's floating black bar and dead restroom door
// (PT-HALL), no fitting where Dale's light comes from (PT-DALE-LIGHT), and a dead switch plate beside each live one (PT-SWITCH).
// The service cabinet, its sign and label are moved and removed where they are built (S47Office.cpp); the records room is its own
// file (S47RecordsDressing.cpp). Everything here is measured on the web export (manifest of 22 October; geo ids below) and the
// authored office room (ArtSource/Office); a part whose source differs keeps the old and says why.
namespace S47SaroFixes
{
namespace
{
 using S47SaroLegacyCleanup::FEdit;

 // Three metres, the export's terms at SARO. ---- PT-PATHS: the concrete walks, as Tools/sarosurfaces/build_geometry.py lays them ----
 struct FRect { double X0, X1, Z0, Z1; };
 const FRect Walks[] = { { -20, -9.4, 5.1, 6.3 }, { -12.2, -11, 6.3, 10.5 }, { -8.725, -7.8, 10.5, 20.5 }, { -8.3, 5.95, 11.65, 12.75 },
  { 4.85, 5.95, 12.75, 17.7 }, { 5.95, 7.3, 16.6, 17.7 }, { 20.1, 39.8, 5.9, 6.5 } };
 // Where an edge meets these there is no edge to dress: the lots' asphalt (the same file), the entrance ramp and its landing
 // (geo 426), the east wing's apron, and the highway west of x -20 (S47RoadEdge dresses that).
 const FRect NoEdge[] = { { -18.2, -7.8, 10.5, 20.5 }, { -16.5, -9.5, 20.5, 22 }, { 20.1, 39.8, 5.9, 18 }, { 10, 39.8, 18, 23 },
  { -10.6, -6.3, 4.75, 6.65 }, { -60, -19.95, -60, 80 } };
 constexpr double GroundY = -.62;          // the site's ground (g4, S47SaroLampFeet), under the walks' -.576 finish
 constexpr double SkirtBottom = -.665;
 constexpr double TileMetres = 6;          // M_CS_WornConcrete is painted for six metres a tile (calichestreet build_ground.py)
 // The road edge's rule (Tools/roadedge, EDGE_HLSL) is drawn for the highway: asphalt to 4.8 m from the middle, its fade out to
 // 16.3 m. A walk's edge is the asphalt's edge here, at a scale that brings the far fade 2.5 m out and the tongues of dust in
 // under a metre onto the concrete.
 constexpr double EdgeOut = 2.5, EdgeScale = (16.5 - 4.8) / EdgeOut;

 // ---- PT-RAMP: the export's entrance ramp and landing, geo 426 (node 627) ----
 constexpr int32 RampNode = 627, RampGeo = 426;
 // ---- PT-STOP: Grounds.ts signs(): the octagon (node 633, geo 430), its pole in the pole batch (node 639, geo 436), its walk and
 // drive box at x -19, z 21.1 ----
 constexpr int32 StopPlaneNode = 633, StopPlaneGeo = 430, StopPoleGeo = 436;
 bool StopFoot(double X0, double X1, double Z0, double Z1)
 { return FMath::Abs(X0 + 19.15) < .02 && FMath::Abs(X1 + 18.85) < .02 && FMath::Abs(Z0 - 20.95) < .02 && FMath::Abs(Z1 - 21.25) < .02; }
 // ---- PT-HALL: Annex.ts's frame over the old Operations door (x -.08..0.98, y 2.065..2.135, z 6.54..6.60) stayed when the door
 // went back 0.5 m into the bay (S47Office): a black bar in the air before the bay. Its batch (node 531, geo 350) holds only that and
 // the restrooms' header, which the authored corridor (corridor_dressing.py, Restroom_steel_header) already has. ----
 constexpr int32 HeaderNode = 531, HeaderGeo = 350;

 struct FState
 {
  TWeakObjectPtr<AS47Web> Web;
  double Retry = 0;
  bool bStop = false, bRamp = false, bWalks = false, bHeader = false, bPlates = false, bLight = false, bRestroom = false;
  TMap<FString, FString> Why;
  TArray<TWeakObjectPtr<UPrimitiveComponent>> Outdoor, Indoor;
  TArray<int32> Hidden;
  TWeakObjectPtr<UMaterialInstanceDynamic> Edge, Glow;
  TWeakObjectPtr<URectLightComponent> OfficeLight;
  TWeakObjectPtr<UStaticMeshComponent> Lens, Body;
  int32 EdgeVertices = 0, Joints = 0, StopCut = 0, Plates = 0;
  bool bTripoFixture = false;
 };
 TMap<TWeakObjectPtr<US47Main>, FState> States;

 void Say(FState& S, const FString& Part, const FString& Why)
 {
  FString& Old = S.Why.FindOrAdd(Part);
  if (Old != Why) { Old = Why; UE_LOG(LogS47SaroFixes, Display, TEXT("S47: SARO fixes %s: %s"), *Part, *Why); }
 }

 UMaterialInterface* Material(const TCHAR* Package, const TCHAR* Name)
 {
  if (!FPackageName::DoesPackageExist(Package)) return nullptr;
  return LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("%s.%s"), Package, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
 }
 UMaterialInterface* Concrete() { return Material(TEXT("/Game/CalicheStreet/Ground/Materials/M_CS_WornConcrete"), TEXT("M_CS_WornConcrete")); }
 UMaterialInstanceDynamic* Flat(AS47Web* W, uint32 Color, float Rough)
 {
  UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
  if (!Base) return nullptr;
  UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, W);
  M->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(Color)); M->SetScalarParameterValue(TEXT("NativeLighting"), 1);
  M->SetScalarParameterValue(TEXT("Roughness"), Rough); M->SetScalarParameterValue(TEXT("Metalness"), 0); M->SetScalarParameterValue(TEXT("EmissivePower"), 0);
  return M;
 }

 // ---- a mesh made here: three-metre vertices with their normal, tangent, UV and colour; triangles turned to face their normal ----
 struct FVert { FVector P, N, T; FVector2f UV; FColor C = FColor::White; };
 struct FMeshData
 {
  TArray<FVert> V; TArray<uint32> I;
  uint32 Add(const FVert& X) { return uint32(V.Add(X)); }
  void Tri(uint32 A, uint32 B, uint32 C)
  {
   // Front faces in UE are those whose three-metre turn is counter-clockwise about the normal (S47RoadEdge EdgeAddGrid).
   const FVector Turn = FVector::CrossProduct(V[B].P - V[A].P, V[C].P - V[A].P);
   if (FVector::DotProduct(Turn, V[A].N + V[B].N + V[C].N) < 0) I.Append({ A, C, B }); else I.Append({ A, B, C });
  }
  void Quad(uint32 A, uint32 B, uint32 C, uint32 D) { Tri(A, B, C); Tri(A, C, D); }
  // A flat quad with UVs from its own metres: U along Along, V along Up x N, both over Tile metres.
  void Face(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& N, const FVector& Along, double Tile, FColor Col = FColor::White)
  {
   const FVector Across = FVector::CrossProduct(N, Along).GetSafeNormal();
   const auto Make = [&](const FVector& P) { FVert X; X.P = P; X.N = N; X.T = Along; X.UV = FVector2f(float(FVector::DotProduct(P, Along) / Tile), float(FVector::DotProduct(P, Across) / Tile)); X.C = Col; return Add(X); };
   const uint32 Ia = Make(A), Ib = Make(B), Ic = Make(C), Id = Make(D);
   Quad(Ia, Ib, Ic, Id);
  }
  void Triangle(const FVector& A, const FVector& B, const FVector& C, const FVector& N, const FVector& Along, double Tile)
  {
   const FVector Across = FVector::CrossProduct(N, Along).GetSafeNormal();
   const auto Make = [&](const FVector& P) { FVert X; X.P = P; X.N = N; X.T = Along; X.UV = FVector2f(float(FVector::DotProduct(P, Along) / Tile), float(FVector::DotProduct(P, Across) / Tile)); return Add(X); };
   const uint32 Ia = Make(A), Ib = Make(B), Ic = Make(C);
   Tri(Ia, Ib, Ic);
  }
 };
 UStaticMesh* Build(AS47Web* W, const FMeshData& D, UMaterialInterface* Mat, bool bShadow, int32 UVs = 1)
 {
  const uint32 Count = D.V.Num();
  if (!W || !Mat || !Count || D.I.IsEmpty()) return nullptr;
  auto* Mesh = NewObject<UStaticMesh>(W, NAME_None, RF_Transient);
  Mesh->bAllowCPUAccess = true; Mesh->NeverStream = true; Mesh->bSupportRayTracing = true;
  FStaticMaterial Slot(Mat); Slot.UVChannelData = FMeshUVChannelInfo(100.f); Mesh->GetStaticMaterials().Add(Slot);
  auto Data = MakeUnique<FStaticMeshRenderData>(); Data->AllocateLODResources(1);
  Data->ScreenSize[0].Default = 1; Data->NumInlinedLODs = 1; Data->CurrentFirstLODIdx = 0; Data->bReadyForStreaming = false;
  FStaticMeshLODResources& L = Data->LODResources[0];
  L.VertexBuffers.PositionVertexBuffer.Init(Count, true);
  auto& T = L.VertexBuffers.StaticMeshVertexBuffer; T.SetUseFullPrecisionUVs(true); T.Init(Count, UVs, true);
  TArray<FColor> Colors; Colors.Reserve(Count);
  FBox Bounds(ForceInit);
  for (uint32 K = 0; K < Count; ++K)
  {
   const FVert& X = D.V[K];
   const FVector At = S47Three::ToUE(X.P); Bounds += At;
   L.VertexBuffers.PositionVertexBuffer.VertexPosition(K) = FVector3f(At);
   const FVector Nz = S47Three::DirToUE(X.N).GetSafeNormal(), Tx = S47Three::DirToUE(X.T).GetSafeNormal();
   T.SetVertexTangents(K, FVector3f(Tx), FVector3f(FVector::CrossProduct(Nz, Tx)), FVector3f(Nz));
   T.SetVertexUV(K, 0, X.UV);
   for (int32 U = 1; U < UVs; ++U) T.SetVertexUV(K, U, FVector2f(0.f, 0.f));
   Colors.Add(X.C);
  }
  L.VertexBuffers.ColorVertexBuffer.InitFromColorArray(Colors);
  FStaticMeshSection Section;
  Section.MaterialIndex = 0; Section.FirstIndex = 0; Section.NumTriangles = D.I.Num() / 3; Section.MinVertexIndex = 0; Section.MaxVertexIndex = Count - 1;
  Section.bEnableCollision = false; Section.bCastShadow = bShadow; L.Sections.Add(Section);
  L.bHasColorVertexData = true; L.bBuffersInlined = true; L.bIsOptionalLOD = false; L.MaxDeviation = 0;
  L.SourceMeshBounds = FBoxSphereBounds(Bounds); Data->Bounds = FBoxSphereBounds(Bounds);
  L.IndexBuffer.TrySetAllowCPUAccess(true); L.IndexBuffer.SetIndices(D.I, EIndexBufferStride::Force32Bit);
  bool bRT = true;
#if !WITH_EDITOR
  bRT = IsRayTracingAllowed();
#endif
  if (bRT) Data->InitializeRayTracingRepresentationFromRenderingLODs();
  Mesh->SetRenderData(MoveTemp(Data)); Mesh->SetExtendedBounds(FBoxSphereBounds(Bounds)); Mesh->InitResources();
  return Mesh;
 }
 // At and Scale in three's terms; the node is given both (AdoptNode keeps only where it is: S47StreetLamps Place).
 UStaticMeshComponent* Place(FState& S, AS47Web* W, int32 Parent, const FString& Name, UStaticMesh* Mesh, UMaterialInterface* Mat, bool bShadow, bool bOutdoor,
  const FVector& At = FVector::ZeroVector, const FVector& Scale = FVector::OneVector)
 {
  const FS47WebNode* P = W->Node(Parent);
  if (!P || !P->Comp.IsValid() || !Mesh) return nullptr;
  auto* C = NewObject<UStaticMeshComponent>(W, MakeUniqueObjectName(W, UStaticMeshComponent::StaticClass(), FName(*(TEXT("S47SaroFix_") + Name.Replace(TEXT(":"), TEXT("_"))))));
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(P->Comp.Get()); C->SetStaticMesh(Mesh);
  if (Mat) C->SetMaterial(0, Mat);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCollisionResponseToAllChannels(ECR_Ignore); C->SetCastShadow(bShadow);
  C->RegisterComponent(); C->SetWorldLocation(S47Three::ToUE(At)); C->SetWorldScale3D(S47Three::DirToUE(Scale));
  const int32 Node = W->AdoptNode(C, Parent, TEXT("sarofix:") + Name);
  if (Node >= 0 && !Scale.Equals(FVector::OneVector)) W->ObjAt(Node).SetScale(Scale);
  (bOutdoor ? S.Outdoor : S.Indoor).Add(C);
  return C;
 }
 bool Inside(const FRect& R, double X, double Z, double Pad = 0) { return X >= R.X0 - Pad && X <= R.X1 + Pad && Z >= R.Z0 - Pad && Z <= R.Z1 + Pad; }
 bool Exposed(double X, double Z)
 {
  for (const FRect& R : Walks) if (Inside(R, X, Z, .01)) return false;
  for (const FRect& R : NoEdge) if (Inside(R, X, Z)) return false;
  return true;
 }
 // The visible ground straight under (X, Z): the first drawn mesh that is not a walking or query box, from a metre up.
 double Ground(UWorld* World, double X, double Z)
 {
  if (!World) return GroundY;
  FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SaroFixGround), true);
  for (int32 Try = 0; Try < 12; ++Try)
  {
   FHitResult Hit;
   if (!World->LineTraceSingleByChannel(Hit, S47Three::ToUE(X, .4, Z), S47Three::ToUE(X, -1.2, Z), ECC_Visibility, Q)) return GroundY;
   UPrimitiveComponent* P = Hit.GetComponent();
   const double Y = S47Three::ToThree(Hit.ImpactPoint).Y;
   if (P && P->IsA<UStaticMeshComponent>() && P->IsVisible() && Y > GroundY - .12 && Y < GroundY + .08) return Y;
   if (!P) return GroundY;
   Q.AddIgnoredComponent(P);
  }
  return GroundY;
 }

 // ---- PT-STOP ----
 bool BuildStop(FState& S, AS47Web* W)
 {
  const FS47WebNode* Plane = W->Node(StopPlaneNode);
  if (!Plane || Plane->Geo != StopPlaneGeo || !Plane->Comp.IsValid()) { Say(S, TEXT("stop sign"), TEXT("the export's octagon (node 633, geo 430) differs; the sign stays")); return true; }
  FEdit Pole; Pole.Id = TEXT("pt-stop-pole"); Pole.RootPath = TEXT("world.grounds.group"); Pole.GeometryIds = { StopPoleGeo };
  Pole.ThreeBounds = FBox(FVector(-19.06, -.66, 21.04), FVector(-18.94, 1.66, 21.16)); Pole.MinTriangles = 12; Pole.MaxTriangles = 48;
  FString Why;
  if (!S47SaroLegacyCleanup::Apply(W, { Pole }, &Why)) { Say(S, TEXT("stop sign"), TEXT("waiting: ") + Why + TEXT(" (Tools/sarofixes/prepare_cpu.py keeps g436's CPU data)")); return false; }
  W->SetVisible(StopPlaneNode, false);
  if (auto* C = Cast<UPrimitiveComponent>(Plane->Comp.Get())) { C->SetVisibility(false); C->SetHiddenInGame(true); C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
  S.Hidden.Add(StopPlaneNode);
  // The pole's foot in the walking and the driving lists goes with it.
  const int32 Walk = W->CollidersMutable(TEXT("saro")).RemoveAll([](const FS47Rect& R) { return StopFoot(R.MinX, R.MaxX, R.MinZ, R.MaxZ); });
  int32 Drive = 0;
  const TSharedPtr<FJsonObject>& Data = W->DriveData(); const TSharedPtr<FJsonObject>* Legs = nullptr, *Leg = nullptr; const TArray<TSharedPtr<FJsonValue>>* Obs = nullptr;
  if (Data.IsValid() && Data->TryGetObjectField(TEXT("legs"), Legs) && (*Legs)->TryGetObjectField(TEXT("saro"), Leg) && (*Leg)->TryGetArrayField(TEXT("obstacles"), Obs))
  {
   TArray<TSharedPtr<FJsonValue>> Kept;
   for (const TSharedPtr<FJsonValue>& V : *Obs)
   {
    const TArray<TSharedPtr<FJsonValue>>& A = V->AsArray();
    if (A.Num() == 4 && StopFoot(A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber(), A[3]->AsNumber())) { ++Drive; continue; }
    Kept.Add(V);
   }
   (*Leg)->SetArrayField(TEXT("obstacles"), Kept);
  }
  S.StopCut = 1;
  Say(S, TEXT("stop sign"), FString::Printf(TEXT("gone: octagon hidden, pole cut from g436, walk feet %d and drive boxes %d removed"), Walk, Drive));
  return true;
 }

 // ---- PT-HALL: the floating header ----
 bool BuildHeader(FState& S, AS47Web* W)
 {
  const FS47WebNode* N = W->Node(HeaderNode);
  if (!N || N->Geo != HeaderGeo || !N->Comp.IsValid() || !W->Obj(TEXT("annex.group")).Find(TEXT("office:authoredRoom")))
  { Say(S, TEXT("hall header"), TEXT("node 531 (geo 350) or the authored corridor differs; the old header stays")); return true; }
  W->SetVisible(HeaderNode, false);
  if (auto* C = Cast<UPrimitiveComponent>(N->Comp.Get())) { C->SetVisibility(false); C->SetHiddenInGame(true); C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
  S.Hidden.Add(HeaderNode);
  Say(S, TEXT("hall header"), TEXT("the old Operations door header (x -.08..0.98, y 2.07..2.14, z 6.54..6.60) hidden with the export's duplicate restroom header"));
  return true;
 }

 // ---- PT-SWITCH: the dead plates beside the records and control switches (the corridor's is removed by S47SaroLegacyCleanup) ----
 bool BuildPlates(FState& S, AS47Web* W)
 {
  TArray<FEdit> Edits;
  auto Plate = [&](const TCHAR* Id, const TCHAR* Root, int32 Geo, const FVector& Min, const FVector& Max)
  { FEdit E; E.Id = Id; E.RootPath = Root; E.GeometryIds = { Geo }; E.ThreeBounds = FBox(Min, Max); E.MinTriangles = E.MaxTriangles = 12; Edits.Add(E); };
  // dressing.ts: c.plate(-4.62, 1.25, 6.8, 0) on the records side, c.plate(-1.72, 1.25, 4.498, PI) on the control room's.
  Plate(TEXT("pt-switch-records-plate"), TEXT("annex.group"), 376, FVector(-4.659, 1.189, 6.794), FVector(-4.581, 1.311, 6.813));
  Plate(TEXT("pt-switch-control-plate"), TEXT("room.group"), 219, FVector(-1.759, 1.189, 4.485), FVector(-1.681, 1.311, 4.505));
  FString Why;
  if (!S47SaroLegacyCleanup::Apply(W, Edits, &Why)) { Say(S, TEXT("switch plates"), TEXT("waiting: ") + Why); return false; }
  S.Plates = 2;
  Say(S, TEXT("switch plates"), TEXT("the dead plates by the records and control switches removed (the corridor's by the legacy cleanup)"));
  return true;
 }

 // ---- PT-RAMP ----
 bool BuildRamp(FState& S, AS47Web* W)
 {
  const FS47WebNode* N = W->Node(RampNode);
  UMaterialInterface* Mat = Concrete();
  if (!N || N->Geo != RampGeo || !N->Comp.IsValid()) { Say(S, TEXT("ramp"), TEXT("node 627 (geo 426) differs; the old ramp stays")); return true; }
  if (!Mat) { Say(S, TEXT("ramp"), TEXT("no M_CS_WornConcrete; the old ramp stays")); return true; }
  const int32 Parent = W->Handle(TEXT("world.grounds.group"));
  if (Parent < 0) return false;
  // The same solid as the export's: the landing x -7.6..-6.3, z 4.75..6.65, 0.62 m high, and the run down west to x -10.6 over
  // z 5.05..6.35. Concrete in metre-true UVs, a broom finish across the run, worn paler where feet go.
  FMeshData D;
  const double Y0 = -.62, Y1 = 0;
  const FVector Up(0, 1, 0), East(1, 0, 0), South(0, 0, 1);
  D.Face(FVector(-7.6, Y1, 4.75), FVector(-6.3, Y1, 4.75), FVector(-6.3, Y1, 6.65), FVector(-7.6, Y1, 6.65), Up, East, TileMetres);
  D.Face(FVector(-7.6, Y0, 4.75), FVector(-6.3, Y0, 4.75), FVector(-6.3, Y1, 4.75), FVector(-7.6, Y1, 4.75), FVector(0, 0, -1), East, TileMetres);
  D.Face(FVector(-7.6, Y0, 6.65), FVector(-6.3, Y0, 6.65), FVector(-6.3, Y1, 6.65), FVector(-7.6, Y1, 6.65), South, East, TileMetres);
  D.Face(FVector(-7.6, Y0, 4.75), FVector(-7.6, Y0, 5.05), FVector(-7.6, Y1, 5.05), FVector(-7.6, Y1, 4.75), FVector(-1, 0, 0), South, TileMetres);
  D.Face(FVector(-7.6, Y0, 6.35), FVector(-7.6, Y0, 6.65), FVector(-7.6, Y1, 6.65), FVector(-7.6, Y1, 6.35), FVector(-1, 0, 0), South, TileMetres);
  const FVector Run = FVector(3.0, .62, 0).GetSafeNormal(), Slope = FVector::CrossProduct(FVector(0, 0, 1), Run).GetSafeNormal() * -1.;
  const FVector SlopeUp = FVector::DotProduct(Slope, Up) > 0 ? Slope : -Slope;
  D.Face(FVector(-10.6, Y0, 5.05), FVector(-7.6, Y1, 5.05), FVector(-7.6, Y1, 6.35), FVector(-10.6, Y0, 6.35), SlopeUp, Run, TileMetres);
  D.Triangle(FVector(-10.6, Y0, 5.05), FVector(-7.6, Y0, 5.05), FVector(-7.6, Y1, 5.05), FVector(0, 0, -1), East, TileMetres);
  D.Triangle(FVector(-10.6, Y0, 6.35), FVector(-7.6, Y0, 6.35), FVector(-7.6, Y1, 6.35), South, East, TileMetres);
  UStaticMesh* Mesh = Build(W, D, Mat, true);
  UStaticMeshComponent* C = Mesh ? Place(S, W, Parent, TEXT("ramp"), Mesh, Mat, true, true) : nullptr;
  if (!C) { Say(S, TEXT("ramp"), TEXT("the new ramp mesh could not be made; the old ramp stays")); return true; }
  // Broom grooves across the run, 12 cm apart, and the nosing where the run meets the landing: thin dark strips just over the face.
  if (UMaterialInstanceDynamic* Groove = Flat(W, 0x7d786d, .97))
  {
   FMeshData G;
   for (double X = -10.45; X < -7.65; X += .12)
   {
    const double T = (X + 10.6) / 3.0, Y = Y0 + T * (Y1 - Y0) + .0025;
    const double T2 = (X + .006 + 10.6) / 3.0, Ye = Y0 + T2 * (Y1 - Y0) + .0025;
    G.Face(FVector(X, Y, 5.10), FVector(X + .006, Ye, 5.10), FVector(X + .006, Ye, 6.30), FVector(X, Y, 6.30), SlopeUp, Run, 1);
   }
   G.Face(FVector(-7.62, Y1 + .002, 5.05), FVector(-7.56, Y1 + .002, 5.05), FVector(-7.56, Y1 + .002, 6.35), FVector(-7.62, Y1 + .002, 6.35), Up, East, 1);
   if (UStaticMesh* GM = Build(W, G, Groove, false)) Place(S, W, Parent, TEXT("rampGrooves"), GM, Groove, false, true);
  }
  // The export's ramp keeps its collision for the walk; only its pale picture goes.
  if (auto* Old = Cast<UPrimitiveComponent>(N->Comp.Get())) { Old->SetVisibility(false); Old->SetCastShadow(false); }
  S.Hidden.Add(RampNode);
  Say(S, TEXT("ramp"), TEXT("worn concrete ramp and landing with broom grooves over the export's (its collision kept)"));
  return true;
 }

 // ---- PT-PATHS ----
 bool BuildWalks(FState& S, AS47Web* W)
 {
  UMaterialInterface* Mat = Concrete();
  const int32 Parent = W->Handle(TEXT("world.grounds.group"));
  if (!Mat || Parent < 0) { Say(S, TEXT("walks"), TEXT("no M_CS_WornConcrete or no grounds group; the walks stay")); return true; }
  // The arrival's one walk mesh (S47SaroArrival, node "arrival:continuousConcreteWalk") keeps its geometry and its place in the
  // arrival's audit; it gets the worn concrete instead of the flat kerb colour.
  UStaticMeshComponent* WalkPaving = nullptr;
  for (int32 K = 0; K < W->PartCount() && !WalkPaving; ++K)
   if (const FS47WebNode* N = W->Node(K)) if (N->Name == TEXT("arrival:continuousConcreteWalk")) WalkPaving = Cast<UStaticMeshComponent>(N->Comp.Get());
  if (!WalkPaving) { Say(S, TEXT("walks"), TEXT("waiting for the arrival's walk")); return false; }
  WalkPaving->SetMaterial(0, Mat);
  constexpr double Top = S47SaroSurfaces::Height;
  // Sides: each slab shows its 4.4 cm edge over the ground (-.62) and goes on under it.
  FMeshData Sides, Cuts;
  int32 JointCount = 0;
  for (const FRect& R : Walks)
  {
   const FVector A(R.X0, 0, R.Z0), B(R.X1, 0, R.Z0), C(R.X1, 0, R.Z1), D(R.X0, 0, R.Z1);
   const auto Side = [&](const FVector& P, const FVector& Q, const FVector& N)
   {
    const FVector Along = (Q - P).GetSafeNormal();
    Sides.Face(FVector(P.X, SkirtBottom, P.Z), FVector(Q.X, SkirtBottom, Q.Z), FVector(Q.X, Top, Q.Z), FVector(P.X, Top, P.Z), N, Along, TileMetres);
   };
   Side(A, B, FVector(0, 0, -1)); Side(B, C, FVector(1, 0, 0)); Side(C, D, FVector(0, 0, 1)); Side(D, A, FVector(-1, 0, 0));
   // Tooled control joints every 1.5 m across the walk, 9 mm wide, as a 1986 sidewalk is poured.
   const bool bAlongX = R.X1 - R.X0 >= R.Z1 - R.Z0;
   const double L0 = bAlongX ? R.X0 : R.Z0, L1 = bAlongX ? R.X1 : R.Z1;
   for (double L = L0 + 1.5; L < L1 - .4; L += 1.5)
   {
    const double Y = Top + .0012, H = .0045;
    if (bAlongX) Cuts.Face(FVector(L - H, Y, R.Z0 + .02), FVector(L + H, Y, R.Z0 + .02), FVector(L + H, Y, R.Z1 - .02), FVector(L - H, Y, R.Z1 - .02), FVector(0, 1, 0), FVector(1, 0, 0), 1);
    else Cuts.Face(FVector(R.X0 + .02, Y, L - H), FVector(R.X1 - .02, Y, L - H), FVector(R.X1 - .02, Y, L + H), FVector(R.X0 + .02, Y, L + H), FVector(0, 1, 0), FVector(1, 0, 0), 1);
    ++JointCount;
   }
  }
  if (UStaticMesh* M = Build(W, Sides, Mat, true)) Place(S, W, Parent, TEXT("walkSides"), M, Mat, true, true);
  if (UMaterialInstanceDynamic* Dark = Flat(W, 0x4a463e, .97)) if (UStaticMesh* M = Build(W, Cuts, Dark, false)) Place(S, W, Parent, TEXT("walkJoints"), M, Dark, false, true);
  S.Joints = JointCount;
  // The edge: the road edge's own ground-copy decal (Tools/roadedge), so the soil over the concrete's edge and the land past it are
  // the ground's own soil, with dust in tongues onto the slab, a lip of it over the edge and its fade a couple of metres out.
  UMaterialInterface* EdgeMat = Material(TEXT("/Game/RoadEdge/M_S47_RoadEdge"), TEXT("M_S47_RoadEdge"));
  UMaterialInstanceDynamic* GroundMid = S47FreeformTerrain::GroundMid(W);
  if (!EdgeMat || !GroundMid) { Say(S, TEXT("walks"), TEXT("worn concrete, sides and joints; no edge (no road-edge material or no Freeform ground)")); return true; }
  UMaterialInstanceDynamic* Edge = UMaterialInstanceDynamic::Create(EdgeMat, W);
  Edge->SetForceMipLevelsToBeResident(true, true, -1.f, 0, true);
  Edge->CopyParameterOverrides(GroundMid);
  S.Edge = Edge;
  UWorld* World = W->GetWorld();
  FMeshData E;
  const double Cols[] = { -1, -.5, -.15, 0, .07, .2, .45, .8, 1.3, 1.9, EdgeOut };
  for (int32 RI = 0; RI + 1 < UE_ARRAY_COUNT(Walks); ++RI) // not the wing's apron strip, which lies between wall and forecourt
  {
   const FRect& R = Walks[RI];
   const double In = FMath::Min(.55, .5 * FMath::Min(R.X1 - R.X0, R.Z1 - R.Z0) - .02);
   struct FEdgeLine { FVector From, Dir, Out; double Len; };
   const FEdgeLine Lines[] = {
    { FVector(R.X0, 0, R.Z0), FVector(1, 0, 0), FVector(0, 0, -1), R.X1 - R.X0 }, { FVector(R.X1, 0, R.Z0), FVector(0, 0, 1), FVector(1, 0, 0), R.Z1 - R.Z0 },
    { FVector(R.X0, 0, R.Z1), FVector(1, 0, 0), FVector(0, 0, 1), R.X1 - R.X0 }, { FVector(R.X0, 0, R.Z0), FVector(0, 0, 1), FVector(-1, 0, 0), R.Z1 - R.Z0 } };
   for (const FEdgeLine& Line : Lines)
   {
    const int32 Rows = FMath::Max(2, FMath::CeilToInt(Line.Len / .5) + 1);
    const uint32 Base = E.V.Num();
    bool bAny = false;
    for (int32 Row = 0; Row < Rows; ++Row)
    {
     const double Along = Line.Len * Row / (Rows - 1);
     const FVector At = Line.From + Line.Dir * Along;
     const FVector Probe = At + Line.Out * .3;
     const bool bKeep = Exposed(Probe.X, Probe.Z);
     bAny |= bKeep;
     const double World1D = FVector::DotProduct(At, Line.Dir);
     for (const double Col : Cols)
     {
      const double Dd = Col < 0 ? Col * In : Col;
      const FVector P = At + Line.Out * Dd;
      double Y = Top + .003;
      if (Dd > 0)
      {
       const double G = Ground(World, P.X, P.Z) + .03;
       Y = Dd >= .07 ? G : FMath::Lerp(Top + .003, G, Dd / .07);
      }
      FVert X; X.P = FVector(P.X, Y, P.Z); X.N = FVector(0, 1, 0); X.T = Line.Dir;
      X.UV = FVector2f(float(4.8 + Dd * EdgeScale), float(World1D * EdgeScale));
      X.C = FLinearColor(.81f, .81f, .81f, bKeep ? 1.f : 0.f).ToFColor(false);
      E.Add(X);
     }
    }
    if (!bAny) { E.V.SetNum(Base); continue; }
    const uint32 NC = UE_ARRAY_COUNT(Cols);
    for (int32 Row = 0; Row + 1 < Rows; ++Row) for (uint32 Col = 0; Col + 1 < NC; ++Col)
    {
     const uint32 A = Base + Row * NC + Col;
     E.Quad(A, A + 1, A + NC + 1, A + NC);
    }
   }
  }
  S.EdgeVertices = E.V.Num();
  if (UStaticMesh* M = Build(W, E, Edge, false, 2)) Place(S, W, Parent, TEXT("walkEdge"), M, Edge, false, true);
  Say(S, TEXT("walks"), FString::Printf(TEXT("worn concrete with %d tooled joints, 4 cm slab sides, and the ground's own dust over their edges (%d vertices)"), JointCount, S.EdgeVertices));
  return true;
 }

 // ---- PT-HALL: the restroom door is locked, with Dale's note on it ----
 bool BuildRestroom(FState& S, AS47Web* W)
 {
  const int32 Parent = W->Handle(TEXT("annex.group"));
  UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
  UMaterialInstanceDynamic* Paper = Flat(W, 0xd9d2b8, .95), *Tape = Flat(W, 0xc8bf9a, .6);
  const FS47WebNode* P = W->Node(Parent);
  if (!Cube || !Paper || !Tape || !P || !P->Comp.IsValid()) return false;
  // On the leaf (Annex.ts: x 1.96..2.0, z 5.22..6.18), under its RESTROOMS plate (1.62 m): a sheet torn from a pad, a little askew.
  const FVector At(1.957, 1.33, 5.66);
  if (!Place(S, W, Parent, TEXT("restroomNote"), Cube, Paper, false, false, At, FVector(.002, .22, .17))) return false;
  for (const double Dz : { -.07, .07 }) Place(S, W, Parent, TEXT("restroomTape"), Cube, Tape, false, false, At + FVector(-.0012, .108, Dz), FVector(.001, .03, .045));
  auto* Text = NewObject<UTextRenderComponent>(W);
  Text->SetupAttachment(P->Comp.Get()); Text->SetMobility(EComponentMobility::Movable);
  Text->SetText(FText::FromString(TEXT("OUT OF ORDER\nPLUMBER CALLED\nDON'T USE - D.")));
  Text->SetWorldSize(1.9f); Text->SetHorizontalAlignment(EHTA_Center); Text->SetVerticalAlignment(EVRTA_TextCenter);
  Text->SetTextRenderColor(FColor(28, 34, 74)); Text->SetCollisionEnabled(ECollisionEnabled::NoCollision); Text->RegisterComponent();
  Text->SetWorldLocation(S47Three::ToUE(At + FVector(-.0025, 0, 0))); Text->SetWorldRotation(FRotator(0, 180, 0));
  W->AdoptNode(Text, Parent, TEXT("sarofix:restroomNoteText"));
  S.Indoor.Add(Text);
  Say(S, TEXT("restroom"), TEXT("locked, with a taped note under the RESTROOMS plate"));
  return true;
 }

 // ---- PT-DALE-LIGHT ----
 UStaticMesh* Licensed(const TCHAR* Name)
 {
  const FString Package = FString(TEXT("/Game/Licensed/Saro1986/")) + Name;
  if (!FPackageName::DoesPackageExist(Package)) return nullptr;
  return LoadObject<UStaticMesh>(nullptr, *(Package + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
 }
 bool BuildLight(FState& S, AS47Web* W)
 {
  const FS47Obj Annex = W->Obj(TEXT("annex.group"));
  const FS47Obj RoomObj = Annex ? Annex.Find(TEXT("office:authoredRoom")) : FS47Obj();
  const FS47WebNode* RN = W->Node(RoomObj.I);
  UStaticMeshComponent* Room = RN ? Cast<UStaticMeshComponent>(RN->Comp.Get()) : nullptr;
  URectLightComponent* Light = nullptr;
  {
   TArray<URectLightComponent*> Lights; W->GetComponents(Lights);
   for (URectLightComponent* L : Lights) if (L && L->ComponentHasTag(TEXT("S47RoomCeiling:office")) && L->GetName().StartsWith(TEXT("OperationsCeilingLight"))) Light = L;
  }
  if (!Room || !Light) { Say(S, TEXT("Dale's light"), TEXT("waiting for the office")); return false; }
  S.OfficeLight = Light;
  UStaticMesh* BodyMesh = Licensed(TEXT("SM_Saro_FluorescentBody"));
  UStaticMesh* LensMesh = Licensed(TEXT("SM_Saro_FluorescentLens"));
  // With the corridor's own fitting at hand (private, /Game/Licensed/Saro1986), Dale's ceiling gets one of the same: the authored
  // backplate and two opal strips (build_office.py, at x .47 z 8.8) go, once the period pass has given the room its CPU copy.
  const bool bPeriod = FPackageName::DoesPackageExist(TEXT("/Game/Signal47/Art/Saro1986/SM_Saro1986_OfficeRoomCpu")) &&
   !FParse::Param(FCommandLine::Get(), TEXT("S47Saro1986Off")) && !FParse::Param(FCommandLine::Get(), TEXT("S47PeriodFixturesOff"));
  if (BodyMesh && LensMesh && bPeriod)
  {
   if (BodyMesh->IsCompiling() || LensMesh->IsCompiling() ||
       S47SaroLegacyCleanup::SourceAssetPath(Room->GetStaticMesh()) != TEXT("/Game/Signal47/Art/Saro1986/SM_Saro1986_OfficeRoomCpu.SM_Saro1986_OfficeRoomCpu"))
   { Say(S, TEXT("Dale's light"), TEXT("waiting for the period pass's office room copy")); return false; }
   FEdit Cut; Cut.Id = TEXT("pt-dale-fixture"); Cut.ThreeBounds = FBox(FVector(.255, 2.585, 8.17), FVector(.685, 2.722, 9.43)); Cut.MinTriangles = 12; Cut.MaxTriangles = 6000;
   Cut.MaterialSlotNames = { FName(TEXT("M_Office_CabinetEnamel")), FName(TEXT("M_Office_Diffuser")) };
   FString Why;
   if (S47SaroLegacyCleanup::ApplyToComponent(W, Room, { Cut }, &Why))
   {
    // The corridor's fitting is 1.30 x 0.36 x 0.08 m along x (S47PeriodFixtures); Dale's runs along z, 1.22 m, under the tiles' 2.7175.
    const FQuat Turn(FVector(0, 1, 0), UE_DOUBLE_PI / 2);
    const FVector Top(.47, 2.735, 8.8), Scale(1.22 / 1.30, 1, 1);
    for (int32 K = 0; K < 2; ++K)
    {
     auto* C = NewObject<UStaticMeshComponent>(W, MakeUniqueObjectName(W, UStaticMeshComponent::StaticClass(), FName(K ? TEXT("S47SaroFix_DaleFixtureLens") : TEXT("S47SaroFix_DaleFixtureBody"))));
     C->SetupAttachment(RN->Comp.Get()); C->SetMobility(EComponentMobility::Movable); C->SetStaticMesh(K ? LensMesh : BodyMesh);
     C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCastShadow(K == 0); C->RegisterComponent();
     double Matrix[16]; S47Three::Compose(Top, Turn, Scale, Matrix); C->SetWorldTransform(S47Three::MatToUE(Matrix));
     const int32 Node = W->AdoptNode(C, RoomObj.I, K ? TEXT("sarofix:daleFixtureLens") : TEXT("sarofix:daleFixtureBody"));
     if (Node >= 0) W->ObjAt(Node).SetQuaternion(W->NodeWorldThree(RoomObj.I).GetRotation().Inverse() * Turn);
     S.Indoor.Add(C);
     if (K) { S.Lens = C; if (UMaterialInterface* LM = C->GetMaterial(0)) { UMaterialInstanceDynamic* G = UMaterialInstanceDynamic::Create(LM, W); C->SetMaterial(0, G); S.Glow = G; } }
     else S.Body = C;
    }
    // The emitter goes to the lens, 1 cm under the fitting's 8 cm body (its top 1.75 cm up in the tiles).
    Light->SetWorldLocation(S47Three::ToUE(FVector(.47, 2.735 - .09, 8.8)));
    S.bTripoFixture = true;
    Say(S, TEXT("Dale's light"), TEXT("the corridor's 1986 fluorescent fitting with its lens in Dale's ceiling, the light in it"));
    return true;
   }
   Say(S, TEXT("Dale's light"), TEXT("the authored fitting stays (") + Why + TEXT("); its opal strips glow with the light instead"));
  }
  // Otherwise the authored fitting stays, and its two opal strips glow with the light that comes from them.
  const int32 Slot = Room->GetMaterialIndex(TEXT("M_Office_Diffuser"));
  UMaterialInterface* Old = Slot >= 0 ? Room->GetMaterial(Slot) : nullptr;
  if (!Old) { Say(S, TEXT("Dale's light"), TEXT("no M_Office_Diffuser slot on the office room")); return true; }
  UMaterialInstanceDynamic* G = Cast<UMaterialInstanceDynamic>(Old);
  if (!G) { G = UMaterialInstanceDynamic::Create(Old, W); Room->SetMaterial(Slot, G); }
  S.Glow = G;
  if (!S.bTripoFixture) Say(S, TEXT("Dale's light"), TEXT("the authored fitting's opal strips glow with the office light"));
  return true;
 }

 void Show(FState& S, US47Main* M, AS47Web* W)
 {
  const bool bSaro = M->Mode == US47Main::EMode::Play && W->Area() == TEXT("saro");
  const bool bInside = bSaro && W->Obj(TEXT("annex.interior")).Shown();
  for (const auto& Weak : S.Outdoor) if (UPrimitiveComponent* C = Weak.Get()) C->SetVisibility(bSaro);
  for (const auto& Weak : S.Indoor) if (UPrimitiveComponent* C = Weak.Get()) C->SetVisibility(bInside);
  for (const int32 Node : S.Hidden) if (const FS47WebNode* N = W->Node(Node)) if (auto* C = Cast<UPrimitiveComponent>(N->Comp.Get())) if (C->IsVisible()) C->SetVisibility(false);
  if (UMaterialInstanceDynamic* G = S.Glow.Get())
  {
   const URectLightComponent* L = S.OfficeLight.Get();
   // The office light runs 32 lm by day to 46 lm at night (S47Office); the corridor's tubes glow 1.4 at full.
   const float K = L && L->IsVisible() ? FMath::Clamp(L->Intensity / 32.f, 0.f, 1.5f) : 0.f;
   G->SetVectorParameterValue(TEXT("EmissiveFactor"), FLinearColor(1.f, .95f, .86f) * (1.4f * K));
  }
  if (UMaterialInstanceDynamic* Edge = S.Edge.Get())
   if (const UMaterialInstanceDynamic* GroundMid = S47FreeformTerrain::GroundMid(W))
   {
    for (const TCHAR* Name : { TEXT("GroundTint"), TEXT("CrustTint"), TEXT("GravelTint") })
    {
     FLinearColor Want, Have;
     if (GroundMid->GetVectorParameterValue(Name, Want) && (!Edge->GetVectorParameterValue(Name, Have) || !Have.Equals(Want, .001f))) Edge->SetVectorParameterValue(Name, Want);
    }
    for (const TCHAR* Name : { TEXT("GravelAmount"), TEXT("CrustAmount"), TEXT("WetK") })
    {
     float Want = 0, Have = 0;
     if (GroundMid->GetScalarParameterValue(Name, Want) && (!Edge->GetScalarParameterValue(Name, Have) || !FMath::IsNearlyEqual(Have, Want, .001f))) Edge->SetScalarParameterValue(Name, Want);
    }
   }
 }
}

bool Before()
{
 static const bool bBefore = FParse::Param(FCommandLine::Get(), TEXT("S47SaroFixesBefore"));
 return bBefore;
}

void Update(US47Main* M, double Dt)
{
 if (!M || Before()) return;
 for (auto It = States.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
 AS47Web* W = M->Web.Get();
 if (!W || !W->IsLoaded()) return;
 FState& S = States.FindOrAdd(TWeakObjectPtr<US47Main>(M));
 if (S.Web.Get() != W) { S = FState(); S.Web = W; }
 S.Retry -= Dt;
 if (S.Retry <= 0 && !(S.bStop && S.bRamp && S.bWalks && S.bHeader && S.bPlates && S.bLight && S.bRestroom))
 {
  S.Retry = 2;
  if (!S.bHeader) S.bHeader = BuildHeader(S, W);
  if (!S.bPlates) S.bPlates = BuildPlates(S, W);
  if (!S.bRestroom) S.bRestroom = BuildRestroom(S, W);
  if (!S.bLight) S.bLight = BuildLight(S, W);
  if (!S.bRamp) S.bRamp = BuildRamp(S, W);
  if (!S.bWalks) S.bWalks = BuildWalks(S, W);
  if (!S.bStop) S.bStop = BuildStop(S, W);
 }
 Show(S, M, W);
}

void Report(US47Main* M, TArray<FString>& Lines)
{
 if (Before()) { Lines.Add(TEXT("SARO fixes: off (-S47SaroFixesBefore)")); return; }
 const FState* S = States.Find(TWeakObjectPtr<US47Main>(M));
 if (!S) { Lines.Add(TEXT("SARO fixes: not started")); return; }
 for (const auto& Pair : S->Why) Lines.Add(FString::Printf(TEXT("SARO fixes %s: %s"), *Pair.Key, *Pair.Value));
}
}
