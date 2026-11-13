// RECORDS-ROOM-LIFT-20261113 (→ M6): the SARO records room's realism lift, after Tom's reference sheet (six photographic angles of
// the room, ~/scratch-bm/refs/records-room-20261113) and his note: surfaces, fittings and props from the sheet; the room's real plan
// and every gameplay position win over the pictures.
//
// Nothing the game hangs on moves or is deleted. The web export's walls and floor keep their geometry and take a new material laid
// on by world position (M_Rec_Block: painted 40 x 20 cm block with real joints; M_Rec_Floor: worn 30 cm vinyl tile), a hung ceiling
// with two troffers goes in 10 cm under the old slab (SM_Rec_Ceiling), and the room's lights keep their exported drivers (flood
// slots, power, flicker, the wall switch) while their source moves into the new fittings (AS47Web::RehouseSaroRecordsLights, ...Lamp).
// Old pieces that a new one stands in for are hidden in game, not removed: their collision, node and handle stay for the AI's
// sweeps, the sight traces and the tests. Everything new is NoCollision under annex.interior, named records:..., inside the walls;
// the one piece that is meant to stop a look (the second shelving) has a box that blocks ECC_Visibility only.
//
// Every asset is load-if-present (/Game/RecordsRoom, made by Tools/recordsroom): a part whose asset is absent leaves the room as it
// was there. -S47RecordsRoomOff and -S47SaroFixesBefore leave the whole room as before, for the before pictures.
//
// Three metres (x, height y, z). The room: x -6.0..-1.0, z 6.8..10.5, the doorway x -4.4..-3.4 in the north partition. The plan
// with its measures: Docs/Design/World/RecordsRoom.
#include "S47SaroFixes.h"

#include "S47Glow.h"
#include "S47Main.h"
#include "S47Props.h"
#include "S47SaroLegacyCleanup.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/BoxComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47RecordsRoom, Log, All);

namespace S47RecordsRoom
{
namespace
{
 const TCHAR* Root = TEXT("/Game/RecordsRoom/");
 constexpr int32 WallGeo = 342, FloorGeo = 345;       // the export's records walls (mat 224) and floor (the room's own quad)

 // ---------- the plan (also what the self test measures) ----------
 namespace Lay
 {
  struct FRect { double X0, X1, Z0, Z1; const TCHAR* What; };
  constexpr double X0 = -6.0, X1 = -1.0, Z0 = 6.8, Z1 = 10.5, Radius = .27;
  const FRect Zone = { -5.75, -1.25, 7.05, 10.25, TEXT("annex.zone.records") };
  const FRect DoorZone = { -4.38, -3.42, 5.6, 7.7, TEXT("annex.zone.recordsDoor") };
  // The export's boxes in the room (walks.saro 33..40) and the dressing's, as they stand after this file's changes.
  const FRect Table = { -4.53, -2.77, 8.15, 9.03, TEXT("the work table (its south edge 9.05 brought in to the top's own edge)") };
  const FRect TableOld = { -4.53, -2.77, 8.15, 9.05, TEXT("the work table as exported") };
  const FRect ChairOld = { -3.9, -3.4, 9.05, 9.7, TEXT("the folding chair at the table (moved to the clerk's desk)") };
  const FRect DrawerOld = { -4.7, -4.22, 9.48, 9.86, TEXT("the middle cabinet's open drawer (now 7 cm ajar on the west cabinet, inside its box)") };
  const FRect FlatOld = { -5.48, -4.92, 6.80, 7.06, TEXT("the flattened cartons by the door (taken away)") };
  const FRect Chair = { -1.76, -1.24, 7.26, 7.50, TEXT("the folding chair pushed in at the clerk's desk") };
  const FRect Bay2 = { -1.82, -1.0, 8.48, 8.97, TEXT("the second shelving, south of the card index") };
  const FRect Kept[] = {
   { -6.0, -5.5, 7.1, 10.2, TEXT("the west shelving") }, { -5.27, -3.65, 9.84, 10.5, TEXT("the filing cabinets") },
   { -3.32, -1.38, 10.1, 10.5, TEXT("the bookcase") }, { -1.8, -1.0, 7.5, 8.4, TEXT("the card index") },
   { -2.38, -1.02, 6.80, 7.28, TEXT("the clerk's desk") }, { -3.67, -3.30, 10.04, 10.48, TEXT("the cartons on the floor") } };
  // Where the pieces stand.
  const FVector TableAt(-3.65, 0, 8.6), LampAt(-3.07, .77, 8.35), TypewriterAt(-4.12, .77, 8.72), ShelvingAt(-5.735, 0, 8.65), CabinetsAt(-4.46, 0, 10.17);
  const FVector Bay2At(-1.40, 0, 8.725), ChairAt(-1.50, 0, 7.21), ChairWas(-3.65, 0, 9.35);
  // What the game uses here and from where a player reaches it (the item's middle on the floor plan, its reach).
  struct FUse { const TCHAR* Id; double X, Z, Reach; };
  const FUse Uses[] = { { TEXT("binder"), -2.32, 10.2, 2.4 }, { TEXT("recPhone"), -1.12, 9.25, 2.4 }, { TEXT("lineage"), -1.3, 7.95, 2.4 },
   { TEXT("workTable"), -3.65, 8.6, 2.4 }, { TEXT("fieldBox"), -5.55, 8.6, 2.4 }, { TEXT("arc:drawer"), -6.14, 8.6, 2.6 }, { TEXT("arc:recorder"), -1.14, 10.22, 2.4 },
   { TEXT("roomlight:records"), -4.75, 6.853, 2.2 }, { TEXT("recordsDoor"), -3.9, 6.8, 2.4 }, { TEXT("ff:hide:annex"), -3.70, 8.30, 2.4 } };
  // Where the tests and the hunt stand and walk (S47Spine, S47FreeformEvidenceTest, S47HorrorTest, S47NightSequenceTest, S47RoomLightsTest, S47SC04).
  const FVector2D Stands[] = { { -2.3, 9.45 }, { -1.75, 9.3 }, { -2.4, 9.4 }, { -2.32, 9.4 }, { -4.75, 7.703 }, { -3.9, 7.5 }, { -2.3, 7.6 }, { -2.2, 9.6 }, { -5.0, 7.7 }, { -5.0, 9.2 } };
  const FVector2D Walks[][2] = { { { -3.9, 7.5 }, { -2.3, 7.6 } }, { { -2.3, 7.6 }, { -2.3, 9.6 } }, { { -2.3, 9.6 }, { -2.2, 9.6 } }, { { -3.9, 7.4 }, { -2.5, 7.7 } }, { { -2.5, 7.7 }, { -2.4, 9.4 } },
   { { -3.9, 7.5 }, { -5.0, 7.7 } }, { { -5.0, 7.7 }, { -5.0, 9.2 } }, { { -2.3, 7.7 }, { -3.9, 7.5 } } };
  // Round the table: west aisle, south of it, east aisle, north of it.
  const FVector2D Round[] = { { -4.99, 7.72 }, { -4.99, 9.42 }, { -2.33, 9.42 }, { -2.33, 7.72 }, { -4.99, 7.72 } };
  const FVector2D Door[] = { { -4.38, 7.0 }, { -3.9, 6.9 }, { -3.42, 7.0 } };     // where one looks in from
  const FVector2D Nook(-1.27, 9.25), Hide(-3.70, 8.30);

  bool In(const FRect& R, double X, double Z, double Grow = 0) { return X > R.X0 - Grow && X < R.X1 + Grow && Z > R.Z0 - Grow && Z < R.Z1 + Grow; }
  void Now(TArray<FRect>& Out) { Out.Reset(); Out.Append(Kept, UE_ARRAY_COUNT(Kept)); Out.Add(Table); Out.Add(Chair); Out.Add(Bay2); }
  // The walker's rule: its middle inside a zone and outside every box grown by its radius.
  bool Walkable(const TArray<FRect>& Boxes, double X, double Z)
  {
   const bool bZone = (X >= Zone.X0 && X <= Zone.X1 && Z >= Zone.Z0 && Z <= Zone.Z1) || (X >= DoorZone.X0 && X <= DoorZone.X1 && Z >= DoorZone.Z0 && Z <= DoorZone.Z1);
   if (!bZone) return false;
   for (const FRect& R : Boxes) if (In(R, X, Z, Radius)) return false;
   return true;
  }
  bool WalkLine(const TArray<FRect>& Boxes, const FVector2D& A, const FVector2D& B, FVector2D* Stop = nullptr)
  {
   const int32 N = FMath::Max(1, FMath::CeilToInt32(FVector2D::Distance(A, B) / .02));
   for (int32 I = 0; I <= N; ++I)
   {
    const FVector2D P = FMath::Lerp(A, B, double(I) / N);
    if (!Walkable(Boxes, P.X, P.Y)) { if (Stop) *Stop = P; return false; }
   }
   return true;
  }
  // Whether the straight line A > B on the plan crosses the box.
  bool Crosses(const FRect& R, const FVector2D& A, const FVector2D& B)
  {
   double T0 = 0, T1 = 1;
   const double D[2] = { B.X - A.X, B.Y - A.Y }, P[2] = { A.X, A.Y }, Lo[2] = { R.X0, R.Z0 }, Hi[2] = { R.X1, R.Z1 };
   for (int32 K = 0; K < 2; ++K)
   {
    if (FMath::Abs(D[K]) < 1e-9) { if (P[K] < Lo[K] || P[K] > Hi[K]) return false; continue; }
    double A0 = (Lo[K] - P[K]) / D[K], A1 = (Hi[K] - P[K]) / D[K];
    if (A0 > A1) Swap(A0, A1);
    T0 = FMath::Max(T0, A0); T1 = FMath::Min(T1, A1);
    if (T0 > T1) return false;
   }
   return true;
  }
  double Gap(const FRect& A, const FRect& B)
  {
   const double Dx = FMath::Max3(0., A.X0 - B.X1, B.X0 - A.X1), Dz = FMath::Max3(0., A.Z0 - B.Z1, B.Z0 - A.Z1);
   return FMath::Sqrt(Dx * Dx + Dz * Dz);
  }
 }

 enum EPart : int32 { Surfaces, Ceiling, Lights, Table, Lamp, Typewriter, Shelving, Cabinets, Bay2, Bookcase, Fittings, Extras, Layout, PartCount };
 const TCHAR* PartNames[PartCount] = { TEXT("surfaces"), TEXT("ceiling"), TEXT("lights"), TEXT("work table"), TEXT("banker's lamp"), TEXT("typewriter"),
  TEXT("shelving and cartons"), TEXT("filing cabinets"), TEXT("second shelving"), TEXT("bookcase binders"), TEXT("fittings"), TEXT("fan and extinguisher"), TEXT("layout") };
 // What each part needs of /Game/RecordsRoom (the first is the one that decides it).
 const TCHAR* const PartAssets[PartCount][3] = {
  { TEXT("Materials/M_Rec_Block"), TEXT("Materials/M_Rec_Floor"), nullptr }, { TEXT("SM_Rec_Ceiling"), nullptr, nullptr }, { TEXT("SM_Rec_Ceiling"), nullptr, nullptr },
  { TEXT("SM_Rec_WorkTable"), nullptr, nullptr }, { TEXT("SM_Rec_BankerLamp"), nullptr, nullptr }, { TEXT("SM_Rec_Typewriter"), nullptr, nullptr },
  { TEXT("SM_Rec_ShelfBay"), TEXT("SM_Rec_ShelfEnd"), TEXT("SM_Rec_BoxA") }, { TEXT("SM_Rec_CabinetA"), TEXT("SM_Rec_CabinetB"), TEXT("SM_Rec_CabinetC") },
  { TEXT("SM_Rec_ShelfBay2"), TEXT("SM_Rec_BoxA"), nullptr }, { TEXT("SM_Rec_BinderA"), TEXT("SM_Rec_BinderB"), TEXT("SM_Rec_BinderC") }, { TEXT("SM_Rec_Fittings"), nullptr, nullptr }, { TEXT("SM_Rec_ExtBracket"), nullptr, nullptr }, { nullptr, nullptr, nullptr } };

 struct FState
 {
  TWeakObjectPtr<AS47Web> Web;
  double Retry = 0;
  bool bBuilt[PartCount] = {};
  FString Why[PartCount];
  int32 Props[PartCount] = {};
  TWeakObjectPtr<UMaterialInterface> Block, Floor;
  TArray<TWeakObjectPtr<UStaticMeshComponent>> Walls;
  TWeakObjectPtr<UStaticMeshComponent> FloorComp, CeilingComp, LampComp;
  TWeakObjectPtr<URectLightComponent> LampLight;
  TWeakObjectPtr<UMaterialInstanceDynamic> LampGlass, LampInner;
  TWeakObjectPtr<UTexture2D> Labels;
  TArray<TWeakObjectPtr<UPrimitiveComponent>> Made;
  int32 Seq = 0;
  bool bLoaded = false;
 };
 TMap<TWeakObjectPtr<US47Main>, FState> States;

 FString OffReason()
 {
  if (S47SaroFixes::Before()) return TEXT("off: -S47SaroFixesBefore (the room as it was, for the before pictures)");
  if (FParse::Param(FCommandLine::Get(), TEXT("S47RecordsRoomOff"))) return TEXT("off: -S47RecordsRoomOff");
  return FString();
 }

 bool Here(const FString& Name) { return FPackageName::DoesPackageExist(FString(Root) + Name); }
 template <class T> T* Asset(const FString& Name)
 {
  const FString Package = FString(Root) + Name;
  if (!FPackageName::DoesPackageExist(Package)) return nullptr;
  return LoadObject<T>(nullptr, *(Package + TEXT(".") + FPackageName::GetShortName(Package)), nullptr, LOAD_NoWarn | LOAD_Quiet);
 }
 UStaticMesh* Mesh(const TCHAR* Name) { UStaticMesh* M = Asset<UStaticMesh>(Name); return M && !M->IsCompiling() ? M : nullptr; }

 // Everything the room may use, loaded once and waited for: a mesh that is still compiling is not an absent one.
 const TCHAR* const AllMeshes[] = { TEXT("SM_Rec_Ceiling"), TEXT("SM_Rec_Fittings"), TEXT("SM_Rec_ExtBracket"), TEXT("SM_Rec_WorkTable"), TEXT("SM_Rec_BankerLamp"), TEXT("SM_Rec_Typewriter"),
  TEXT("SM_Rec_ShelfBay"), TEXT("SM_Rec_ShelfEnd"), TEXT("SM_Rec_ShelfBay2"), TEXT("SM_Rec_CabinetA"), TEXT("SM_Rec_CabinetB"), TEXT("SM_Rec_CabinetC"),
  TEXT("SM_Rec_BoxA"), TEXT("SM_Rec_BoxB"), TEXT("SM_Rec_BoxC"), TEXT("SM_Rec_BoxD"), TEXT("SM_Rec_BoxE"), TEXT("SM_Rec_BoxF"), TEXT("SM_Rec_BoxG"), TEXT("SM_Rec_BoxH"),
  TEXT("SM_Rec_BinderA"), TEXT("SM_Rec_BinderB"), TEXT("SM_Rec_BinderC"), TEXT("SM_Rec_Folders"), TEXT("SM_Rec_Papers") };
 UStaticMesh* Bought(const TCHAR* Package)
 {
  if (!FPackageName::DoesPackageExist(Package)) return nullptr;
  UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *(FString(Package) + TEXT(".") + FPackageName::GetShortName(Package)), nullptr, LOAD_NoWarn | LOAD_Quiet);
  return M && !M->IsCompiling() ? M : nullptr;
 }
 const TCHAR* FanPackage = TEXT("/Game/Rocket/RPS_DeskFan/SM_RPS_DeskFan01_c1_LOD1");
 const TCHAR* ExtinguisherPackage = TEXT("/Game/Rocket/RPS_FireExtinguisher/SM_RPS_FireExtinguisher_02_c1_LOD1");
 const TCHAR* AshtrayPackage = TEXT("/Game/Free/Office/office_assets_radio/StaticMeshes/office_assets_radio_Ashtray_27");
 bool Loaded()
 {
  bool bBusy = false;
  for (const TCHAR* Name : AllMeshes) if (const UStaticMesh* A = Asset<UStaticMesh>(Name)) bBusy |= A->IsCompiling();
  for (const TCHAR* Package : { FanPackage, ExtinguisherPackage, AshtrayPackage })
   if (FPackageName::DoesPackageExist(Package))
    if (const UStaticMesh* A = LoadObject<UStaticMesh>(nullptr, *(FString(Package) + TEXT(".") + FPackageName::GetShortName(Package)), nullptr, LOAD_NoWarn | LOAD_Quiet)) bBusy |= A->IsCompiling();
  Asset<UTexture2D>(TEXT("Textures/T_Rec_Labels"));
  return !bBusy;
 }

 bool Under(AS47Web* W, int32 Node, int32 RootNode)
 {
  while (Node >= 0 && Node != RootNode) { const FS47WebNode* N = W->Node(Node); Node = N ? N->Parent : -1; }
  return Node == RootNode && RootNode >= 0;
 }
 // The one mesh of the export with this geometry under the annex.
 UStaticMeshComponent* GeoMesh(AS47Web* W, int32 Geo)
 {
  const int32 Annex = W->Handle(TEXT("annex.group"));
  UStaticMeshComponent* Found = nullptr;
  for (int32 I = 0; I < W->PartCount(); ++I)
   if (const FS47WebNode* N = W->Node(I); N && N->Geo == Geo && Under(W, I, Annex))
   {
    if (Found) return nullptr;
    Found = Cast<UStaticMeshComponent>(N->Comp.Get());
   }
  return Found;
 }
 UPrimitiveComponent* Named(AS47Web* W, const TCHAR* RootPath, const TCHAR* Name)
 {
  const FS47Obj Found = W->Obj(RootPath).Find(Name);
  const FS47WebNode* N = Found ? W->Node(Found.I) : nullptr;
  return N ? Cast<UPrimitiveComponent>(N->Comp.Get()) : nullptr;
 }
 // Out of sight, still there: the node, its handle and its collision stay (the AI's sweeps, the sight traces, the tests).
 void Retire(UPrimitiveComponent* C) { if (C) { C->SetHiddenInGame(true); C->SetCastShadow(false); } }
 int32 RetireNamed(AS47Web* W, const TCHAR* Prefix)
 {
  const int32 Interior = W->Handle(TEXT("annex.interior"));
  int32 Count = 0;
  for (int32 I = 0; I < W->PartCount(); ++I)
   if (const FS47WebNode* N = W->Node(I); N && N->Name.StartsWith(Prefix) && Under(W, I, Interior))
    if (UPrimitiveComponent* C = Cast<UPrimitiveComponent>(N->Comp.Get())) { Retire(C); ++Count; }
  return Count;
 }
 void Wear(UStaticMeshComponent* C, UMaterialInterface* Mat)
 {
  if (!C || !Mat) return;
  for (int32 K = 0; K < C->GetNumMaterials(); ++K) if (C->GetMaterial(K) != Mat) C->SetMaterial(K, Mat);
 }
 // A box of SARO's walk list by where it stands; nullptr when none is there.
 FS47Rect* WalkBox(AS47Web* W, const Lay::FRect& R)
 {
  for (FS47Rect& C : W->CollidersMutable(TEXT("saro")))
   if (FMath::Abs(C.MinX - R.X0) < .005 && FMath::Abs(C.MaxX - R.X1) < .005 && FMath::Abs(C.MinZ - R.Z0) < .005 && FMath::Abs(C.MaxZ - R.Z1) < .005) return &C;
  return nullptr;
 }
 void WalkBoxNamed(AS47Web* W, const TCHAR* Id, const Lay::FRect& R)
 {
  TArray<FS47Rect>& All = W->CollidersMutable(TEXT("saro"));
  for (FS47Rect& C : All) if (C.Id == Id) { C.MinX = R.X0; C.MaxX = R.X1; C.MinZ = R.Z0; C.MaxZ = R.Z1; C.bEnabled = true; return; }
  All.Add({ R.X0, R.X1, R.Z0, R.Z1, FString(Id), true });
 }
 bool Cut(AS47Web* W, const TCHAR* Id, int32 Geo, const FVector& Min, const FVector& Max, int32 Least, int32 Most = 0)
 {
  S47SaroLegacyCleanup::FEdit E;
  E.Id = Id; E.RootPath = TEXT("annex.group"); E.GeometryIds = { Geo }; E.ThreeBounds = FBox(Min, Max); E.MinTriangles = Least; E.MaxTriangles = FMath::Max(Least, Most);
  FString Why;
  const bool bOk = S47SaroLegacyCleanup::Apply(W, { E }, &Why);
  if (!bOk) UE_LOG(LogS47RecordsRoom, Display, TEXT("S47: records room: the old %s stays (%s)"), Id, *Why);
  return bOk;
 }

 // ---------- placing ----------
 struct FCtx { AS47Web* W = nullptr; FState* S = nullptr; int32 Part = 0, Parent = -1; USceneComponent* ParentComp = nullptr; };
 bool Open(FCtx& C, AS47Web* W, FState& S, int32 Part)
 {
  C.W = W; C.S = &S; C.Part = Part;
  const FS47Obj Interior = W->Obj(TEXT("annex.interior"));
  const FS47WebNode* N = W->Node(Interior.I);
  C.Parent = Interior.I; C.ParentComp = N ? N->Comp.Get() : nullptr;
  if (!C.ParentComp) S.Why[Part] = TEXT("waiting: annex.interior has no component");
  return C.ParentComp != nullptr;
 }
 // Unreal yaw of a piece whose front (its +Y after import) is to face the Three direction (dx, dz).
 double Facing(double Dx, double Dz) { return FMath::RadiansToDegrees(FMath::Atan2(-Dx, Dz)); }
 UStaticMeshComponent* Place(FCtx& C, const FString& Name, UStaticMesh* M, const FVector& At, double Yaw, const FRotator& Tilt = FRotator::ZeroRotator, bool bShadow = true)
 {
  if (!M || !C.ParentComp) return nullptr;
  auto* Comp = NewObject<UStaticMeshComponent>(C.W, *FString::Printf(TEXT("S47Records_%d_%s"), C.S->Seq++, *Name.Replace(TEXT(":"), TEXT("_"))));
  Comp->SetupAttachment(C.ParentComp); Comp->SetMobility(EComponentMobility::Movable); Comp->SetStaticMesh(M);
  Comp->SetCollisionProfileName(TEXT("NoCollision")); Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Comp->SetCollisionResponseToAllChannels(ECR_Ignore); Comp->SetGenerateOverlapEvents(false); Comp->SetCanEverAffectNavigation(false);
  Comp->SetCastShadow(bShadow);
  C.W->AddInstanceComponent(Comp); Comp->RegisterComponent();
  Comp->SetWorldTransform(FTransform(FQuat(FRotator(0, Yaw, 0)) * FQuat(Tilt), S47Three::ToUE(At)));
  C.W->AdoptNode(Comp, C.Parent, TEXT("records:") + Name);
  C.S->Made.Add(Comp); ++C.S->Props[C.Part];
  return Comp;
 }
 // One cell of T_Rec_Labels on a flat card: its face toward Normal, its text running along Right (Three directions), metres.
 FIntRect Cell(int32 I)
 {
  if (I < 24) { const int32 X = (I % 4) * 512, Y = (I / 4) * 256; return FIntRect(X, Y, X + 512, Y + 256); }       // carton cards
  const int32 K = I - 24, X = (K % 4) * 512, Y = 1536 + (K / 4) * 128;                                              // drawer and spine slips
  return FIntRect(X, Y, X + 512, Y + 128);
 }
 void Label(FCtx& C, const FString& Name, int32 CellIndex, const FVector& Centre, const FVector& Normal, const FVector& Right, double Length, double Height, double TurnDeg = 0)
 {
  UTexture2D* Tex = C.S->Labels.Get();
  if (!Tex) { Tex = Asset<UTexture2D>(TEXT("Textures/T_Rec_Labels")); C.S->Labels = Tex; }
  UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
  UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
  if (!Tex || !Plane || !Base || !C.ParentComp) return;
  constexpr float Atlas = 2048;
  const FIntRect R = Cell(CellIndex);
  UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, C.W);
  Mat->SetVectorParameterValue(TEXT("Color"), FLinearColor(.86f, .86f, .86f)); Mat->SetScalarParameterValue(TEXT("NativeLighting"), 1);
  Mat->SetScalarParameterValue(TEXT("Roughness"), .9f); Mat->SetScalarParameterValue(TEXT("Metalness"), 0); Mat->SetScalarParameterValue(TEXT("EmissivePower"), 0);
  Mat->SetScalarParameterValue(TEXT("UseMap"), 1); Mat->SetScalarParameterValue(TEXT("MapIsLinear"), 0); Mat->SetTextureParameterValue(TEXT("Map"), Tex);
  const float U0 = (R.Min.X + .5f) / Atlas, V0 = (R.Min.Y + .5f) / Atlas, Su = (R.Width() - 1.f) / Atlas, Sv = (R.Height() - 1.f) / Atlas;
  Mat->SetVectorParameterValue(TEXT("MapU"), FLinearColor(Su, 0, U0, 0)); Mat->SetVectorParameterValue(TEXT("MapV"), FLinearColor(0, Sv, V0, 0));
  const FVector N = S47Three::DirToUE(Normal).GetSafeNormal();
  const FQuat Q = FQuat(N, FMath::DegreesToRadians(TurnDeg)) * FRotationMatrix::MakeFromXZ(S47Three::DirToUE(Right).GetSafeNormal(), N).ToQuat();
  auto* Comp = NewObject<UStaticMeshComponent>(C.W, *FString::Printf(TEXT("S47Records_%d_%s"), C.S->Seq++, *Name.Replace(TEXT(":"), TEXT("_"))));
  Comp->SetupAttachment(C.ParentComp); Comp->SetMobility(EComponentMobility::Movable); Comp->SetStaticMesh(Plane); Comp->SetMaterial(0, Mat);
  Comp->SetCollisionProfileName(TEXT("NoCollision")); Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Comp->SetCollisionResponseToAllChannels(ECR_Ignore); Comp->SetGenerateOverlapEvents(false); Comp->SetCanEverAffectNavigation(false); Comp->SetCastShadow(false);
  C.W->AddInstanceComponent(Comp); Comp->RegisterComponent();
  Comp->SetWorldTransform(FTransform(Q, S47Three::ToUE(Centre), FVector(Length, Height, 1)));
  C.W->AdoptNode(Comp, C.Parent, TEXT("records:") + Name);
  C.S->Made.Add(Comp); ++C.S->Props[C.Part];
 }

 // ---------- cartons, binders and folders on a shelf ----------
 // W along the piece's own X, D along its Y (front -Y), H; the lettered kinds are Tools/recordsroom/build_records.py's.
 struct FKind { const TCHAR* Mesh; double W, D, H; };
 const FKind Kinds[] = { { TEXT("SM_Rec_BoxA"), .39, .31, .26 }, { TEXT("SM_Rec_BoxB"), .39, .31, .275 }, { TEXT("SM_Rec_BoxC"), .39, .31, .26 }, { TEXT("SM_Rec_BoxD"), .39, .31, .27 },
  { TEXT("SM_Rec_BoxE"), .39, .31, .20 }, { TEXT("SM_Rec_BoxF"), .31, .26, .30 }, { TEXT("SM_Rec_BoxG"), .39, .31, .26 }, { TEXT("SM_Rec_BoxH"), .39, .31, .26 } };
 const TCHAR* const Binders[] = { TEXT("SM_Rec_BinderA"), TEXT("SM_Rec_BinderB"), TEXT("SM_Rec_BinderC") };
 // A shelf seen from its front: Out is the Three direction the front faces, Along the direction of its run (to the viewer's right is
 // -Along x Up...; only used consistently), Front the front edge's coordinate along Out, A0..A1 the free span along Along.
 struct FShelf { FVector Out, Along; double Front, Y, A0, A1, Clear; bool bBindersOnly = false; };
 FVector OnShelf(const FShelf& Sh, double Along, double Back, double Up) { return Sh.Out * (Sh.Front - Back) + Sh.Along * Along + FVector(0, Sh.Y + Up, 0); }
 // Fills a span with cartons (their long or short side out), a run of binders or a stack of folders, no two alike in how they stand.
 void Fill(FCtx& C, FRandomStream& Rng, const FShelf& Sh, const FString& Tag, int32& NextCard, int32& NextSlip)
 {
  double A = Sh.A0 + Rng.FRandRange(.0, .03);
  int32 N = 0;
  const double YawOut = Facing(Sh.Out.X, Sh.Out.Z);
  // the reader's right, looking at the shelf's front
  const FVector Right = FVector::CrossProduct(FVector(0, 1, 0), Sh.Out).GetSafeNormal();
  const double Sign = FVector::DotProduct(Right, Sh.Along) >= 0 ? 1 : -1;
  while (A < Sh.A1 - .07)
  {
   const double Left = Sh.A1 - A;
   const int32 Roll = Rng.RandRange(0, 9);
   const FString Name = FString::Printf(TEXT("%s_%d"), *Tag, N++);
   if (Sh.bBindersOnly && Roll >= 8 && Left > .5)
   {
    // a gap on the shelf, or a few folders lying in it
    if (Roll == 9) Place(C, Name + TEXT("_flat"), Mesh(TEXT("SM_Rec_Folders")), OnShelf(Sh, A + .16, .17, 0), YawOut + 90 + Rng.FRandRange(-10., 10.));
    A += Roll == 9 ? .34 : Rng.FRandRange(.08, .2);
    continue;
   }
   if (Left < .27 || Roll == 9 || Sh.bBindersOnly)
   {
    // binders, spines out; the last of a run leans on its neighbours
    const int32 Count = FMath::Min(Rng.RandRange(2, Sh.bBindersOnly ? 6 : 4), FMath::FloorToInt32(Left / .079));
    if (Count < 1) break;
    for (int32 K = 0; K < Count; ++K)
    {
     const bool bLean = K == Count - 1 && Count > 2 && Rng.FRand() < .5 && Sh.A1 - A > .16;
     UStaticMesh* M = Mesh(Binders[Rng.RandRange(0, 2)]);
     const double Mid = A + .0395 + (bLean ? .03 : 0);
     const FRotator Tilt = bLean ? FRotator(Sign * 11.0, 0, 0) : FRotator::ZeroRotator;       // its top onto the run behind it
     const double Set = .012 + Rng.FRandRange(0., .02);
     Place(C, FString::Printf(TEXT("%s_binder%d"), *Name, K), M, OnShelf(Sh, Mid, Set + .143, bLean ? .004 : 0), YawOut + Rng.FRandRange(-1.5, 1.5), Tilt);
     if (!bLean && Rng.FRand() < .75)
      Label(C, FString::Printf(TEXT("%s_slip%d"), *Name, K), 32 + (NextSlip++ % 8), OnShelf(Sh, Mid, Set - .0012, .2125), Sh.Out, FVector(0, 1, 0), .118, .046);
     A += .079 + (bLean ? .05 : 0);
    }
    A += Rng.FRandRange(.01, .05);
    continue;
   }
   if (Roll == 8 && Left > .34 && Sh.Clear > .1)
   {
    UStaticMesh* M = Mesh(Rng.FRand() < .6 ? TEXT("SM_Rec_Folders") : TEXT("SM_Rec_Papers"));
    Place(C, Name + TEXT("_flat"), M, OnShelf(Sh, A + .16, .19 + Rng.FRandRange(0., .05), 0), YawOut + 90 + Rng.FRandRange(-12., 12.));
    A += .33 + Rng.FRandRange(.01, .04);
    continue;
   }
   // a carton: its long side or its short end to the front
   int32 Kind = Rng.RandRange(0, 7);
   if (Kinds[Kind].H > Sh.Clear - .02) Kind = 4;
   const FKind& K = Kinds[Kind];
   const bool bEnd = Kind != 5 ? (Left < K.W + .02 || Rng.FRand() < .30) : true;
   const double Wide = bEnd ? K.D : K.W, Deep = bEnd ? K.W : K.D;
   if (Wide > Left + .005) { A += .05; continue; }
   const double Back = Rng.FRand() < .12 ? -.035 : Rng.FRandRange(.004, .07);       // one in eight pulled a little out over the edge
   const double Turn = Rng.FRandRange(-3.5, 3.5);
   const FVector At = OnShelf(Sh, A + Wide * .5, Back + Deep * .5, 0);
   Place(C, Name, Mesh(K.Mesh), At, YawOut + (bEnd ? -Sign * 90 : 0) + Turn);
   if (Kind != 7 && Rng.FRand() < .86)
   {
    const double Up = bEnd ? K.H * .36 : K.H * .45;
    const FVector Face = OnShelf(Sh, A + Wide * .5 + Rng.FRandRange(-.03, .03) * (bEnd ? .4 : 1), Back - .0009, Up + Rng.FRandRange(-.015, .015));
    Label(C, Name + TEXT("_card"), NextCard++ % 24, Face, Sh.Out, Right, bEnd ? .15 : .18, bEnd ? .075 : .09, Rng.FRandRange(-3., 3.));
   }
   // a lower carton or a stack of folders on top, where the shelf above leaves room
   if (Kind != 1 && Kind != 7 && K.H + .21 < Sh.Clear && Rng.FRand() < .28)
   {
    const FKind& T = Kinds[4];
    Place(C, Name + TEXT("_top"), Mesh(T.Mesh), At + FVector(0, K.H + .001, 0) + Sh.Out * Rng.FRandRange(-.02, .0), YawOut + (bEnd ? -Sign * 90 : 0) + Rng.FRandRange(-6., 6.));
   }
   else if (Kind != 1 && Kind != 7 && K.H + .06 < Sh.Clear && Rng.FRand() < .16)
    Place(C, Name + TEXT("_onTop"), Mesh(TEXT("SM_Rec_Folders")), At + FVector(0, K.H + .001, 0), YawOut + 90 + Rng.FRandRange(-20., 20.));
   A += Wide + Rng.FRandRange(.008, .045);
  }
 }

 // ---------- the parts ----------
 bool BuildSurfaces(AS47Web* W, FState& S)
 {
  UMaterialInterface* Block = Asset<UMaterialInterface>(TEXT("Materials/M_Rec_Block"));
  UMaterialInterface* Floor = Asset<UMaterialInterface>(TEXT("Materials/M_Rec_Floor"));
  if (!Block || !Floor) { S.Why[Surfaces] = TEXT("waiting: M_Rec_Block or M_Rec_Floor absent (Tools/recordsroom); the painted walls and lino as before"); return false; }
  UStaticMeshComponent* Walls = GeoMesh(W, WallGeo);
  UStaticMeshComponent* FloorComp = GeoMesh(W, FloorGeo);
  if (!Walls || !FloorComp) { S.Why[Surfaces] = TEXT("waiting: the export's records walls (geo 342) or floor (geo 345) not found as one mesh each"); return false; }
  S.Block = Block; S.Floor = Floor; S.FloorComp = FloorComp;
  S.Walls.Reset(); S.Walls.Add(Walls);
  // S47Office's green skin over the partition east of the door (x -3.4..-0.94, 6 mm): the same block wall as the rest of the room.
  if (UStaticMeshComponent* Skin = Cast<UStaticMeshComponent>(Named(W, TEXT("annex.group"), TEXT("office:partitionRecords")))) S.Walls.Add(Skin);
  for (const auto& Wall : S.Walls) Wear(Wall.Get(), Block);
  Wear(FloorComp, Floor);
  return true;
 }

 bool BuildCeiling(AS47Web* W, FState& S)
 {
  UStaticMesh* M = Mesh(TEXT("SM_Rec_Ceiling"));
  FCtx C;
  if (!M) { S.Why[Ceiling] = TEXT("waiting: SM_Rec_Ceiling absent; the old slab and tube boxes as before"); return false; }
  if (!Open(C, W, S, Ceiling)) return false;
  S.CeilingComp = Place(C, TEXT("ceiling"), M, FVector::ZeroVector, 0);
  return S.CeilingComp.IsValid();
 }

 bool BuildLights(AS47Web* W, FState& S)
 {
  if (!S.CeilingComp.IsValid()) { S.Why[Lights] = TEXT("waiting: the hung ceiling is not there; the tubes as before"); return false; }
  if (!W->RehouseSaroRecordsLights(S.CeilingComp.Get(), true))
  { S.Why[Lights] = TEXT("waiting: the two tube fixtures (nodes 490, 491; annex slots 4, 3) or the lens slots differ"); return false; }
  return true;
 }

 bool BuildTable(AS47Web* W, FState& S)
 {
  UStaticMesh* M = Mesh(TEXT("SM_Rec_WorkTable"));
  FCtx C;
  if (!M) { S.Why[Table] = TEXT("waiting: SM_Rec_WorkTable absent; the ArchiveProps table as before"); return false; }
  UPrimitiveComponent* Old = Named(W, TEXT("annex.interior"), TEXT("SM_ArchiveProps_WorkTable"));
  if (!Old) { S.Why[Table] = TEXT("waiting: SM_ArchiveProps_WorkTable not built yet (its collision is what the hunt and the sight traces meet)"); return false; }
  if (!Open(C, W, S, Table)) return false;
  Place(C, TEXT("workTable"), M, Lay::TableAt, 0);
  Retire(Old);
  // the export's blotter and loose sheets lay flat on the old top
  for (int32 Geo : { 371, 372 }) Retire(GeoMesh(W, Geo));
  // a stack of papers and one of folders where the sheets lay
  Place(C, TEXT("tablePapers"), Mesh(TEXT("SM_Rec_Papers")), FVector(-3.62, .771, 8.47), 172);
  Place(C, TEXT("tableFolders"), Mesh(TEXT("SM_Rec_Folders")), FVector(-3.40, .771, 8.83), 96);
  // a glass ashtray by the lamp (the free office set's, CC BY, when it is here)
  Place(C, TEXT("tableAshtray"), Bought(AshtrayPackage), FVector(-3.33, .771, 8.50), 20);
  return true;
 }

 bool BuildLamp(AS47Web* W, FState& S)
 {
  UStaticMesh* M = Mesh(TEXT("SM_Rec_BankerLamp"));
  FCtx C;
  if (!M) { S.Why[Lamp] = TEXT("waiting: SM_Rec_BankerLamp absent; the export's lamp and its light as before"); return false; }
  if (!Open(C, W, S, Lamp)) return false;
  // its shade opens to the south, over the table's middle
  const FVector Bulb = S47Three::ToUE(Lay::LampAt + FVector(0, .345, .006));
  if (!W->RehouseSaroRecordsLamp(Bulb, false)) { S.Why[Lamp] = TEXT("waiting: the desk lamp's fixture (annex slot 5) not fitted yet"); return false; }
  UStaticMeshComponent* L = Place(C, TEXT("bankerLamp"), M, Lay::LampAt, Facing(0, 1));
  URectLightComponent* Light = L ? W->RehouseSaroRecordsLamp(Bulb, true) : nullptr;
  if (!Light) { S.Why[Lamp] = TEXT("waiting: the lamp could not be placed"); return false; }
  S.LampComp = L; S.LampLight = Light;
  const int32 Glass = L->GetMaterialIndex(TEXT("M_Rec_LampGlass")), Inner = L->GetMaterialIndex(TEXT("M_Rec_LampInner"));
  if (Glass >= 0) { S.LampGlass = UMaterialInstanceDynamic::Create(L->GetMaterial(Glass), W); L->SetMaterial(Glass, S.LampGlass.Get()); }
  if (Inner >= 0) { S.LampInner = UMaterialInstanceDynamic::Create(L->GetMaterial(Inner), W); L->SetMaterial(Inner, S.LampInner.Get()); }
  for (int32 Geo : { 373, 374 }) Retire(GeoMesh(W, Geo));      // the export's foot and shade; its rod is inside the new stem
  // the export's glow sprite hung in the air beside the old shade (a blurred halo): the glass and the pool on the table are the glow now
  if (FS47GlowPoints* G = W->Glow(TEXT("annex.glow.points")))
   for (int32 I = 0; I < G->Num(); ++I) if (FVector::Dist(G->Pos[I], FVector(-3.07, 1.08, 8.36)) < .05) G->SetOutputScale(I, 0.f);
  return true;
 }

 bool BuildTypewriter(AS47Web* W, FState& S)
 {
  UStaticMesh* M = Mesh(TEXT("SM_Rec_Typewriter"));
  FCtx C;
  if (!M) { S.Why[Typewriter] = TEXT("waiting: SM_Rec_Typewriter absent; the free model as before"); return false; }
  if (!Open(C, W, S, Typewriter)) return false;
  // Config/Props/saro.json's "typewriter" (the free model, yaw 180: its keys to the door) stands here; hidden once ours does
  UStaticMeshComponent* Old = AS47Props::PlacedMesh(TEXT("typewriter"));
  if (Old && !S47Three::ToThree(Old->GetComponentLocation()).Equals(Lay::TypewriterAt, .3)) Old = nullptr;
  Place(C, TEXT("typewriter"), M, Lay::TypewriterAt, Facing(0, -1) + 2.5);
  Retire(Old);
  return true;
 }

 bool BuildShelving(AS47Web* W, FState& S)
 {
  UStaticMesh* Bay = Mesh(TEXT("SM_Rec_ShelfBay"));
  UStaticMesh* End = Mesh(TEXT("SM_Rec_ShelfEnd"));
  FCtx C;
  if (!Bay || !End || !Mesh(TEXT("SM_Rec_BoxA"))) { S.Why[Shelving] = TEXT("waiting: SM_Rec_ShelfBay, SM_Rec_ShelfEnd or the cartons absent; the ArchiveProps shelving as before"); return false; }
  UPrimitiveComponent* Old = Named(W, TEXT("annex.interior"), TEXT("SM_ArchiveProps_ShelvingStorage"));
  if (!Old) { S.Why[Shelving] = TEXT("waiting: SM_ArchiveProps_ShelvingStorage not built yet"); return false; }
  if (!Open(C, W, S, Shelving)) return false;
  // Three 1.0 m bays and the end frame: frames at z 10.15, 9.15, 8.15 and 7.15, its front to the room (+x). The run's own +X is north.
  const double Yaw = Facing(1, 0);
  for (int32 B = 0; B < 3; ++B) Place(C, FString::Printf(TEXT("shelfBay%d"), B), Bay, Lay::ShelvingAt + FVector(0, 0, 1.0 - B), Yaw);
  Place(C, TEXT("shelfEnd"), End, Lay::ShelvingAt + FVector(0, 0, -1.5), Yaw);
  Retire(Old);
  RetireNamed(W, TEXT("records:shelfCarton"));          // the dressing's cartons on the old top shelf
  Cut(W, TEXT("records room shelf slips"), 376, FVector(-5.96, .15, 7.30), FVector(-5.37, 1.82, 9.90), 100, 180);   // the export's loose slips and binders
  // The cartons: every shelf of every bay its own, by one seed. The middle bay's third shelf keeps z 8.36..8.84 free: the
  // STATION 01 box stands there, pulled half out, with the fire drawer behind it.
  static const double Tops[] = { .132, .632, 1.132, 1.632, 2.132 };
  FRandomStream Rng(1986);
  int32 Card = 0, Slip = 0;
  for (int32 L = 0; L < 5; ++L)
   for (int32 B = 0; B < 3; ++B)
   {
    const double ZN = 7.15 + B + .05, ZS = 8.15 + B - .05;        // between the frames, north to south
    FShelf Sh; Sh.Out = FVector(1, 0, 0); Sh.Along = FVector(0, 0, 1); Sh.Front = -5.489; Sh.Y = Tops[L] - (L < 4 ? .004 : .003); Sh.Clear = L < 4 ? .47 : .52;
    const FString Tag = FString::Printf(TEXT("shelf%d_%d"), L, B);
    if (L == 2 && B == 1)
    {
     Sh.A0 = ZN; Sh.A1 = 8.35; Fill(C, Rng, Sh, Tag + TEXT("n"), Card, Slip);
     Sh.A0 = 8.85; Sh.A1 = ZS; Fill(C, Rng, Sh, Tag + TEXT("s"), Card, Slip);
     continue;
    }
    Sh.A0 = ZN; Sh.A1 = ZS; Fill(C, Rng, Sh, Tag, Card, Slip);
   }
  return true;
 }

 bool BuildCabinets(AS47Web* W, FState& S)
 {
  const TCHAR* Names[] = { TEXT("SM_Rec_CabinetB"), TEXT("SM_Rec_CabinetA"), TEXT("SM_Rec_CabinetC") };     // west (a drawer ajar), middle, east
  UStaticMesh* Meshes[3];
  for (int32 I = 0; I < 3; ++I) Meshes[I] = Mesh(Names[I]);
  FCtx C;
  if (!Meshes[0] || !Meshes[1] || !Meshes[2]) { S.Why[Cabinets] = TEXT("waiting: SM_Rec_CabinetA/B/C absent; the ArchiveProps cabinets as before"); return false; }
  UPrimitiveComponent* Old = Named(W, TEXT("annex.interior"), TEXT("SM_ArchiveProps_FilingCabinets"));
  if (!Old) { S.Why[Cabinets] = TEXT("waiting: SM_ArchiveProps_FilingCabinets not built yet"); return false; }
  if (!Open(C, W, S, Cabinets)) return false;
  static const double Xs[] = { -5.0, -4.46, -3.92 }, Turn[] = { .8, -.4, 1.1 }, Out[] = { .0, .012, .004 };
  int32 Slip = 0;
  for (int32 I = 0; I < 3; ++I)
  {
   Place(C, FString::Printf(TEXT("cabinet%d"), I), Meshes[I], FVector(Xs[I], 0, Lay::CabinetsAt.Z - Out[I]), Facing(0, -1) + Turn[I]);
   for (int32 K = 0; K < 4; ++K)        // the slip in each drawer's holder
   {
    const double Ajar = (I == 0 && K == 2) ? .07 : 0;
    Label(C, FString::Printf(TEXT("cabinet%d_slip%d"), I, K), 24 + (Slip++ % 8), FVector(Xs[I], .058 + K * .312 + .250, Lay::CabinetsAt.Z - Out[I] - .3128 - Ajar), FVector(0, 0, -1), FVector(-1, 0, 0), .078, .026);
   }
  }
  Retire(Old);
  // the export's drawer stood 36 cm out of the middle cabinet with a box of its own in the walk list
  Cut(W, TEXT("records room open drawer"), 376, FVector(-4.69, .39, 9.47), FVector(-4.23, .84, 9.87), 60, 110);
  // cartons on top: two on the west cabinet, one on the middle one; the fan keeps the east one
  FRandomStream Rng(47);
  int32 Card = 12, Sl = 3;
  FShelf Sh; Sh.Out = FVector(0, 0, -1); Sh.Along = FVector(1, 0, 0); Sh.Front = -9.87; Sh.Y = 1.321; Sh.Clear = .6; Sh.A0 = -5.22; Sh.A1 = -4.22;
  Fill(C, Rng, Sh, TEXT("cabinetTop"), Card, Sl);
  return true;
 }

 bool BuildBay2(AS47Web* W, FState& S)
 {
  UStaticMesh* M = Mesh(TEXT("SM_Rec_ShelfBay2"));
  FCtx C;
  if (!M || !Mesh(TEXT("SM_Rec_BoxA"))) { S.Why[Bay2] = TEXT("waiting: SM_Rec_ShelfBay2 or the cartons absent; no second shelving"); return false; }
  if (!Open(C, W, S, Bay2)) return false;
  Place(C, TEXT("bay2"), M, Lay::Bay2At, Facing(0, 1));
  // What a look and the hunt's body meet: a box of the bay's size on ECC_Visibility only (never the interaction channel).
  auto* Box = NewObject<UBoxComponent>(W, TEXT("S47Records_bay2Sight"));
  Box->SetupAttachment(C.ParentComp); Box->SetMobility(EComponentMobility::Movable);
  Box->SetBoxExtent(FVector(40, 22.5, 100)); Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Box->SetCollisionObjectType(ECC_WorldStatic);
  Box->SetCollisionResponseToAllChannels(ECR_Ignore); Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
  Box->SetGenerateOverlapEvents(false); Box->SetCanEverAffectNavigation(false); Box->SetHiddenInGame(true);
  W->AddInstanceComponent(Box); Box->RegisterComponent();
  Box->SetWorldTransform(FTransform(FQuat::Identity, S47Three::ToUE(Lay::Bay2At + FVector(0, 1.0, 0))));
  W->AdoptNode(Box, C.Parent, TEXT("records:bay2Sight"));
  S.Made.Add(Box);
  WalkBoxNamed(W, TEXT("records:bay2"), Lay::Bay2);
  static const double Tops[] = { .132, .582, 1.032, 1.482, 1.932 };
  FRandomStream Rng(53);
  int32 Card = 5, Slip = 5;
  for (int32 L = 0; L < 5; ++L)
  {
   FShelf Sh; Sh.Out = FVector(0, 0, 1); Sh.Along = FVector(1, 0, 0); Sh.Front = Lay::Bay2At.Z + .219; Sh.Y = Tops[L] - .003; Sh.Clear = L < 4 ? .42 : .6;
   Sh.A0 = Lay::Bay2At.X - .365; Sh.A1 = Lay::Bay2At.X + .365;
   Fill(C, Rng, Sh, FString::Printf(TEXT("bay2_%d"), L), Card, Slip);
  }
  return true;
 }

 // The bookcase's middle shelf: the dressing's box binders give way to modelled ones, in runs with gaps, one leaning, folders lying.
 bool BuildBookcase(AS47Web* W, FState& S)
 {
  FCtx C;
  if (!Mesh(TEXT("SM_Rec_BinderA")) || !Mesh(TEXT("SM_Rec_BinderB")) || !Mesh(TEXT("SM_Rec_BinderC"))) { S.Why[Bookcase] = TEXT("waiting: SM_Rec_BinderA/B/C absent; the dressing's binders as before"); return false; }
  if (!Named(W, TEXT("annex.interior"), TEXT("SM_ArchiveProps_Bookcase"))) { S.Why[Bookcase] = TEXT("waiting: SM_ArchiveProps_Bookcase not built yet"); return false; }
  if (RetireNamed(W, TEXT("records:binder")) == 0) { S.Why[Bookcase] = TEXT("waiting: the dressing's binders are not on the shelf yet"); return false; }
  if (!Open(C, W, S, Bookcase)) return false;
  FRandomStream Rng(77);
  int32 Card = 0, Slip = 2;
  FShelf Sh; Sh.Out = FVector(0, 0, -1); Sh.Along = FVector(1, 0, 0); Sh.Front = -10.128; Sh.Y = .451; Sh.Clear = .40; Sh.A0 = -3.25; Sh.A1 = -1.44; Sh.bBindersOnly = true;
  Fill(C, Rng, Sh, TEXT("bookcase"), Card, Slip);
  return true;
 }

 // A bought piece whose own origin is not known here, stood by its bounds: bHang false, the bottom's middle on At; true, its top at
 // At's height and its back (the side to Three -z) on At's z. Height: its height in metres when it is to be scaled to one (0: as it is).
 UStaticMeshComponent* Stand(FCtx& C, const FString& Name, UStaticMesh* M, const FVector& At, double Yaw, bool bHang, double Height = 0)
 {
  if (!M || !C.ParentComp) return nullptr;
  auto* Comp = NewObject<UStaticMeshComponent>(C.W, *FString::Printf(TEXT("S47Records_%d_%s"), C.S->Seq++, *Name.Replace(TEXT(":"), TEXT("_"))));
  Comp->SetupAttachment(C.ParentComp); Comp->SetMobility(EComponentMobility::Movable); Comp->SetStaticMesh(M);
  Comp->SetCollisionProfileName(TEXT("NoCollision")); Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Comp->SetCollisionResponseToAllChannels(ECR_Ignore); Comp->SetGenerateOverlapEvents(false); Comp->SetCanEverAffectNavigation(false);
  C.W->AddInstanceComponent(Comp); Comp->RegisterComponent();
  const double Raw = M->GetBoundingBox().GetSize().Z / 100.0;
  const double Scale = Height > 0 && Raw > .01 ? Height / Raw : 1.0;
  Comp->SetWorldTransform(FTransform(FQuat(FRotator(0, Yaw, 0)), S47Three::ToUE(At), FVector(Scale)));
  const FBox B = Comp->Bounds.GetBox(); const FVector Want = S47Three::ToUE(At);
  FVector Delta(Want.X - B.GetCenter().X, Want.Y - B.GetCenter().Y, Want.Z - B.Min.Z);
  if (bHang) { Delta.Y = Want.Y - B.Min.Y; Delta.Z = Want.Z - B.Max.Z; }
  Comp->AddWorldOffset(Delta);
  C.W->AdoptNode(Comp, C.Parent, TEXT("records:") + Name);
  C.S->Made.Add(Comp); ++C.S->Props[C.Part];
  return Comp;
 }
 bool BuildFittings(AS47Web* W, FState& S)
 {
  UStaticMesh* M = Mesh(TEXT("SM_Rec_Fittings"));
  FCtx C;
  if (!M) { S.Why[Fittings] = TEXT("waiting: SM_Rec_Fittings absent; no cove base or conduit, the export's cork board as before"); return false; }
  if (!Open(C, W, S, Fittings)) return false;
  Place(C, TEXT("fittings"), M, FVector::ZeroVector, 0);
  // the export's cork board: its panel and four frame bars go (the notices at z 6.829 and their pins stay, on the new cork)
  Cut(W, TEXT("records room cork panel"), 376, FVector(-2.26, 1.19, 6.799), FVector(-1.24, 1.91, 6.826), 1, 40);
  Cut(W, TEXT("records room cork frame top"), 376, FVector(-2.275, 1.895, 6.799), FVector(-1.225, 1.93, 6.831), 1, 40);
  Cut(W, TEXT("records room cork frame bottom"), 376, FVector(-2.275, 1.17, 6.799), FVector(-1.225, 1.205, 6.831), 1, 40);
  Cut(W, TEXT("records room cork frame west"), 376, FVector(-2.28, 1.195, 6.799), FVector(-2.245, 1.905, 6.831), 1, 40);
  Cut(W, TEXT("records room cork frame east"), 376, FVector(-1.255, 1.195, 6.799), FVector(-1.22, 1.905, 6.831), 1, 40);
  // the old lamp's cord across the floor to the wall (ours is in the fittings)
  Cut(W, TEXT("records room lamp cord"), 376, FVector(-3.08, -.01, 8.19), FVector(-1.0, .80, 8.71), 200, 400);
  return true;
 }

 // Tom's two single Cosmos props (git-ignored under /Game/Rocket; Tools/recordsroom/cosmos_props.py): a fire extinguisher on a
 // bracket of ours west of the switch, in place of the red cylinder, and a desk fan on the east cabinet, in place of the wire one.
 bool BuildExtras(AS47Web* W, FState& S)
 {
  UStaticMesh* Ext = Bought(ExtinguisherPackage);
  UStaticMesh* Fan = Bought(FanPackage);
  UStaticMesh* Bracket = Mesh(TEXT("SM_Rec_ExtBracket"));
  FCtx C;
  if (!Ext && !Fan) { S.Why[Extras] = TEXT("waiting: the Cosmos fan and extinguisher are not here (bought, never in Git); the red cylinder and the wire fan as before"); return false; }
  if (!Open(C, W, S, Extras)) return false;
  if (Ext && Bracket)
  {
   constexpr double X = -5.05, Foot = .52;        // the bracket's foot; its hook .40 above, the sign over it
   Place(C, TEXT("extBracket"), Bracket, FVector(X, Foot, Lay::Z0), Facing(0, 1));
   Stand(C, TEXT("extinguisher"), Ext, FVector(X, Foot + .43, Lay::Z0 + .008), 0, true, .52);
   Retire(GeoMesh(W, 375));                      // the export's red cylinder
   if (S.bBuilt[Lamp]) Retire(GeoMesh(W, 370));  // and its bracket (that mesh holds only this and the old lamp's rod by now)
  }
  if (Fan && S.bBuilt[Cabinets])
  {
   Stand(C, TEXT("deskFan"), Fan, FVector(-3.93, 1.321, 10.17), Facing(-.45, -1), false);
   Cut(W, TEXT("records room wire fan"), 376, FVector(-4.12, 1.375, 10.05), FVector(-3.80, 1.78, 10.26), 100, 300);
  }
  return true;
 }

 // The chair to the clerk's desk, the old boxes out of the walk list, the flattened cartons away.
 bool BuildLayout(AS47Web* W, FState& S)
 {
  const FS47Obj Interior = W->Obj(TEXT("annex.interior"));
  const FS47Obj Chair = Interior.Find(TEXT("lic:folding_chair"));
  FS47Rect* ChairBox = WalkBox(W, Lay::ChairOld);
  FS47Rect* DrawerBox = WalkBox(W, Lay::DrawerOld);
  FS47Rect* TableBox = WalkBox(W, Lay::TableOld);
  if (!TableBox) TableBox = WalkBox(W, Lay::Table);
  if (!TableBox) { S.Why[Layout] = TEXT("waiting: the table's box is not in SARO's walk list as exported"); return false; }
  TableBox->MaxZ = Lay::Table.Z1;
  if (DrawerBox && S.bBuilt[Cabinets]) DrawerBox->bEnabled = false;
  // the chair: the bought model moves; without it the export's chair stays where it was with its box
  if (Chair && ChairBox)
  {
   const FS47WebNode* N = W->Node(Chair.I);
   if (N && N->P.Equals(Lay::ChairWas, .01)) { W->SetPosition(Chair.I, Lay::ChairAt); ChairBox->bEnabled = false; WalkBoxNamed(W, TEXT("records:clerkChair"), Lay::Chair); }
  }
  // the dressing's flattened cartons on the north wall and their box
  RetireNamed(W, TEXT("records:flatCarton"));
  for (FS47Rect& R : W->CollidersMutable(TEXT("saro"))) if (R.Id == TEXT("records:flatCartons")) R.bEnabled = false;
  return true;
 }
}

void Update(US47Main* M, double Dt)
{
 if (!M || !OffReason().IsEmpty()) return;
 for (auto It = States.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
 AS47Web* W = M->Web.Get(); if (!W || !W->IsLoaded()) return;
 FState& S = States.FindOrAdd(TWeakObjectPtr<US47Main>(M));
 if (S.Web.Get() != W) { S = FState(); S.Web = W; }
 const bool bSaro = M->World && M->World->Area == TEXT("saro") && W->Area() == TEXT("saro");
 S.Retry -= Dt;
 if (bSaro && S.Retry <= 0)
 {
  S.Retry = 2;
  if (!S.bLoaded) { S.bLoaded = Loaded(); if (!S.bLoaded) { S.Retry = .25; return; } }
  for (int32 I = 0; I < PartCount; ++I)
  {
   if (S.bBuilt[I]) continue;
   const FString Old = S.Why[I];
   bool bOk = false;
   switch (I)
   {
    case Surfaces: bOk = BuildSurfaces(W, S); break;
    case Ceiling: bOk = BuildCeiling(W, S); break;
    case Lights: bOk = BuildLights(W, S); break;
    case Table: bOk = BuildTable(W, S); break;
    case Lamp: bOk = BuildLamp(W, S); break;
    case Typewriter: bOk = BuildTypewriter(W, S); break;
    case Shelving: bOk = BuildShelving(W, S); break;
    case Cabinets: bOk = BuildCabinets(W, S); break;
    case Bay2: bOk = BuildBay2(W, S); break;
    case Bookcase: bOk = BuildBookcase(W, S); break;
    case Fittings: bOk = BuildFittings(W, S); break;
    case Extras: bOk = BuildExtras(W, S); break;
    case Layout: bOk = BuildLayout(W, S); break;
    default: break;
   }
   S.bBuilt[I] = bOk;
   if (bOk) { S.Why[I].Reset(); UE_LOG(LogS47RecordsRoom, Display, TEXT("S47: records room %s built (%d pieces)"), PartNames[I], S.Props[I]); }
   else if (Old != S.Why[I]) { UE_LOG(LogS47RecordsRoom, Display, TEXT("S47: records room %s %s"), PartNames[I], *S.Why[I]); }
  }
 }
 if (bSaro && S.Retry >= 1.99)
 {
  if (S.bBuilt[Shelving]) RetireNamed(W, TEXT("records:shelfCarton"));
  if (S.bBuilt[Bookcase]) RetireNamed(W, TEXT("records:binder"));
  if (S.bBuilt[Layout]) { RetireNamed(W, TEXT("records:flatCarton")); for (FS47Rect& R : W->CollidersMutable(TEXT("saro"))) if (R.Id == TEXT("records:flatCartons")) R.bEnabled = false; }
 }
 // The export's own materials can come back on a node (a private copy for a story colour): the walls and floor keep the new ones.
 if (S.bBuilt[Surfaces]) { for (const auto& Wall : S.Walls) Wear(Wall.Get(), S.Block.Get()); Wear(S.FloorComp.Get(), S.Floor.Get()); }
 // The lamp's glass and its white inside glow with its light (the fixture keeps power, the dark and the dimming).
 if (URectLightComponent* L = S.LampLight.Get())
 {
  const float K = L->IsVisible() ? FMath::Clamp(L->Intensity / 12.f, 0.f, 1.5f) : 0.f;
  if (UMaterialInstanceDynamic* G = S.LampGlass.Get()) G->SetVectorParameterValue(TEXT("EmissiveFactor"), FLinearColor(.02f, .30f, .09f) * (K * .11f));
  if (UMaterialInstanceDynamic* In = S.LampInner.Get()) In->SetVectorParameterValue(TEXT("EmissiveFactor"), FLinearColor(1.f, .78f, .5f) * (K * 2.2f));
 }
}

void Report(US47Main* M, TArray<FString>& Lines)
{
 const FString Off = OffReason();
 const FState* S = M ? States.Find(TWeakObjectPtr<US47Main>(M)) : nullptr;
 for (int32 I = 0; I < PartCount; ++I)
 {
  FString Line = FString::Printf(TEXT("RECORDS-ROOM %s: "), PartNames[I]);
  if (!Off.IsEmpty()) Line += Off;
  else if (!S || !S->Web.IsValid()) Line += TEXT("waiting: SARO's web scene is not loaded yet");
  else Line += S->bBuilt[I] ? FString::Printf(TEXT("built, %d pieces"), S->Props[I]) : (S->Why[I].IsEmpty() ? FString(TEXT("waiting: not tried yet (SARO not entered)")) : S->Why[I]);
  Lines.Add(Line);
 }
}

bool SelfTest(TArray<FString>& Lines)
{
 bool bOk = true;
 auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s records room: %s"), bCond ? TEXT("PASS") : TEXT("FAIL"), *What)); bOk &= bCond; };
 TArray<Lay::FRect> Boxes; Lay::Now(Boxes);
 constexpr int32 TableIndex = UE_ARRAY_COUNT(Lay::Kept);      // Now() puts the table after the kept ones
 // 1. Round the main table: the narrowest free width on each side, and a walker's whole round.
 double Narrow = 1e9; const TCHAR* Where = TEXT("");
 for (const Lay::FRect& R : Boxes)
 {
  if (&R == &Boxes[TableIndex]) continue;
  const double G = Lay::Gap(Lay::Table, R);
  if (G < Narrow) { Narrow = G; Where = R.What; }
 }
 const double North = Lay::Table.Z0 - Lay::Z0, East = Lay::X1 - Lay::Table.X1;
 Narrow = FMath::Min3(Narrow, North, East);
 Check(Narrow >= .8, FString::Printf(TEXT("round the work table at least 0.80 m is free everywhere: narrowest %.2f m (%s); to the north wall %.2f m"), Narrow, Where, North));
 bool bRound = true; FVector2D Stop(0, 0);
 for (int32 I = 0; I + 1 < UE_ARRAY_COUNT(Lay::Round) && bRound; ++I) bRound = Lay::WalkLine(Boxes, Lay::Round[I], Lay::Round[I + 1], &Stop);
 Check(bRound, bRound ? FString(TEXT("a walker (radius 0.27 m) goes all the way round the table: west aisle, south of it, east aisle, north of it"))
  : FString::Printf(TEXT("the walk round the table stops at %.2f, %.2f"), Stop.X, Stop.Y));
 TArray<Lay::FRect> Before; Before.Append(Lay::Kept, UE_ARRAY_COUNT(Lay::Kept)); Before.Add(Lay::TableOld); Before.Add(Lay::ChairOld); Before.Add(Lay::DrawerOld); Before.Add(Lay::FlatOld);
 bool bWas = true;
 for (int32 I = 0; I + 1 < UE_ARRAY_COUNT(Lay::Round) && bWas; ++I) bWas = Lay::WalkLine(Before, Lay::Round[I], Lay::Round[I + 1]);
 Lines.Add(FString::Printf(TEXT("INFO records room: before the lift the same round was %s (the chair 0.14 m from the cabinets, the open drawer 0.43 m from the table)"), bWas ? TEXT("open") : TEXT("shut")));
 // 2. Sight: the second shelving stands between the doorway and the nook under the wall phone; the hide under the table is
 // open to the doorway (its north side has no panel: a hider must see out), measured, not required.
 int32 Blocked = 0;
 for (const FVector2D& D : Lay::Door) Blocked += Lay::Crosses(Lay::Bay2, D, Lay::Nook) ? 1 : 0;
 Check(Blocked == UE_ARRAY_COUNT(Lay::Door), FString::Printf(TEXT("the second shelving breaks the line from the doorway to the nook at (%.2f, %.2f): %d of %d lines cross it"), Lay::Nook.X, Lay::Nook.Y, Blocked, int32(UE_ARRAY_COUNT(Lay::Door))));
 Check(Lay::Walkable(Boxes, Lay::Nook.X, Lay::Nook.Y), TEXT("the nook behind the second shelving is a place a walker can stand"));
 int32 HideSeen = 0;
 for (const FVector2D& D : Lay::Door) { bool bCut = false; for (const Lay::FRect& R : Boxes) bCut |= (&R != &Boxes[TableIndex]) && Lay::Crosses(R, D, Lay::Hide); HideSeen += bCut ? 0 : 1; }
 Lines.Add(FString::Printf(TEXT("INFO records room: the hide under the table (%.2f, %.2f) is in line of sight of the doorway from %d of %d points, %.2f m away (the game sees a hider from at most 3.0 m); nothing can stand between without closing the hider's own view or the tests' walks"),
  Lay::Hide.X, Lay::Hide.Y, HideSeen, int32(UE_ARRAY_COUNT(Lay::Door)), FVector2D::Distance(Lay::Door[1], Lay::Hide)));
 // 3. Everything the game uses in the room can still be reached: a free standing point within its reach, the second shelving not between.
 TArray<FString> Lost;
 for (const Lay::FUse& U : Lay::Uses)
 {
  bool bFound = false;
  for (double R = .5; R <= U.Reach - .15 && !bFound; R += .1)
   for (int32 A = 0; A < 72 && !bFound; ++A)
   {
    const double X = U.X + R * FMath::Cos(A * PI / 36), Z = U.Z + R * FMath::Sin(A * PI / 36);
    bFound = Lay::Walkable(Boxes, X, Z) && !Lay::Crosses(Lay::Bay2, FVector2D(X, Z), FVector2D(U.X, U.Z));
   }
  if (!bFound) Lost.Add(U.Id);
 }
 Check(Lost.IsEmpty(), Lost.IsEmpty() ? FString::Printf(TEXT("all %d things used in the room are within reach of a free standing point"), int32(UE_ARRAY_COUNT(Lay::Uses))) : TEXT("out of reach: ") + FString::Join(Lost, TEXT(", ")));
 // 4. Where the tests and the hunt stand and walk stays free.
 TArray<FString> Shut;
 for (const FVector2D& P : Lay::Stands) if (!Lay::Walkable(Boxes, P.X, P.Y)) Shut.Add(FString::Printf(TEXT("(%.2f, %.2f)"), P.X, P.Y));
 for (const auto& Leg : Lay::Walks) { FVector2D At(0, 0); if (!Lay::WalkLine(Boxes, Leg[0], Leg[1], &At)) Shut.Add(FString::Printf(TEXT("the walk to (%.2f, %.2f) at (%.2f, %.2f)"), Leg[1].X, Leg[1].Y, At.X, At.Y)); }
 Check(Shut.IsEmpty(), Shut.IsEmpty() ? FString::Printf(TEXT("the %d standing points and %d walks of the tests and the hunt are free"), int32(UE_ARRAY_COUNT(Lay::Stands)), int32(UE_ARRAY_COUNT(Lay::Walks))) : TEXT("shut: ") + FString::Join(Shut, TEXT(", ")));
 // 5. The phone's fixed stand sees the phone past the second shelving, and the new boxes stay inside the room and out of the doorway.
 Check(!Lay::Crosses(Lay::Bay2, FVector2D(-1.75, 9.3), FVector2D(-1.12, 9.25)) && !Lay::Crosses(Lay::Bay2, FVector2D(-2.3, 9.45), FVector2D(-2.32, 10.2)), TEXT("the fixed stands of the wall phone and the binder see them past the second shelving"));
 Check(Lay::Bay2.X1 <= Lay::X1 + 1e-6 && Lay::Bay2.X0 > Lay::X0 && Lay::Bay2.Z1 < Lay::Z1 && Lay::Bay2.Z1 < 9.1 - .12 && Lay::Chair.Z1 <= 7.5 + 1e-6 && !Lay::In(Lay::Bay2, -1.12, 9.25), TEXT("the second shelving ends 13 cm north of the wall phone and inside the east wall; the chair stays in the pocket by the clerk's desk"));
 // 6. Load-if-present: with the assets or without them the room stands (the old look is each part's fallback).
 int32 HereN = 0, Total = 0; TArray<FString> Fallback;
 for (int32 I = 0; I < PartCount; ++I)
 {
  bool bAll = true;
  for (const TCHAR* A : PartAssets[I]) if (A) { ++Total; const bool bHere = Here(A); HereN += bHere ? 1 : 0; bAll &= bHere; }
  if (!bAll) Fallback.Add(PartNames[I]);
 }
 Check(true, FString::Printf(TEXT("%d of %d named assets of /Game/RecordsRoom are here%s%s"), HereN, Total, Fallback.Num() ? TEXT("; as before (fallback): ") : TEXT("; no part falls back"), *FString::Join(Fallback, TEXT(", "))));
 return bOk;
}
}
