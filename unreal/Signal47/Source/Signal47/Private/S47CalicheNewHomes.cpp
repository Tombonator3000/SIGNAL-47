#include "S47CalicheNewHomes.h"
#include "S47CalicheAccess.h"
#include "S47Drive.h"
#include "S47EventPlaces.h"
#include "S47Main.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47TownDay.h"
#include "S47TownSites.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47NewHomes, Log, All);

namespace S47CalicheNewHomesLocal
{
 using FHome = S47CalicheNewHomes::FHome;
 constexpr double RoadX = 8000;
 // ---- THE TABLE (road-local metres). Picked against the export's walk colliders and drive obstacles, the street lamps (x -7), the
 // power line (x 15, its wires 13 to 17), the town's fence (x -42 to 48, z 2018 to 2300), the temporary boxes (S47TownSites) and the
 // places for events (S47EventPlaces); the gaps a small town's lots have (four to ten metres to the next building). The footprints
 // of H05 to H07 are their rooms' in Content/CalicheAccess/assembly.json (SelfTest holds the two together).
 const TArray<FHome> Table = {
  // between the church and the empty house, across from the substation: H02's model in cream with blue trim
  { TEXT("H05"), TEXT("mesa_waitress"), false, 24.5, 32.0, 2108.5, 2117.5, -1, 2115.9, 2111.7, 32.0, 39.0, 18.7, 2111.0, 32.6, 2107.8 },
  // south of the souvenir man's, before the school's ground: H01's model in cream
  { TEXT("H06"), TEXT("earl"), false, 24.0, 33.0, 2199.0, 2209.0, -1, 2207.4, 2202.2, 33.0, 40.0, 18.7, 2202.4, 33.6, 2198.3 },
  // between the feed store and the cook's house: H04's model in tan with sage trim
  { TEXT("H07"), TEXT("bench_woman"), false, -28.0, -19.0, 2219.0, 2229.0, 1, 2227.4, 2222.2, -35.0, -28.0, -17.3, 2222.4, -28.6, 2218.3 },
  // past the cook's house, end on to the street: the trailer
  { TEXT("H08"), TEXT("regular"), true, -34.9, -27.1, 2253.1, 2255.9, 1, 2254.5, 2254.5, 0.0, 0.0, -17.3, 2257.6, -26.4, 2257.4 },
 };
 // The cars that are props (Config/Props/town.json): what they stand on, for the plants under them and the self-test.
 struct FCar { const TCHAR* Id; double X0, X1, Z0, Z1; };
 const FCar Cars[] = { { TEXT("earlPickup"), 24.55, 29.45, 2193.55, 2195.65 }, { TEXT("trailerJunker"), -40.6, -37.4, 2257.8, 2259.4 } };
 const TCHAR* const Motorhome = TEXT("/Game/Licensed/TripoNew/SM_Tripo_Motorhome.SM_Tripo_Motorhome"), * const MotorhomePackage = TEXT("/Game/Licensed/TripoNew/SM_Tripo_Motorhome");

 struct FWindow { FString Id; TWeakObjectPtr<UMaterialInstanceDynamic> Pane; TWeakObjectPtr<UPointLightComponent> Spill; double Lumens = 0; float Last = -1; };
 struct FState
 {
  TWeakObjectPtr<AS47Web> Web;
  bool bBuilt = false, bPlants = false;
  int32 PlantTries = 0, PlantsHidden = 0;
  bool bMotorhome = false;
  TArray<FWindow> Windows;
  TWeakObjectPtr<UPointLightComponent> TrailerLamp;
  TArray<FS47Obstacle> Obstacles;
  const IS47DriveArea* DriveFor = nullptr;
  TArray<FBox2D> Under;      // road-local: where plants go
  FBox2D Trailer = FBox2D(ForceInit); // road-local, as built
 };
 FState State;

 FBox2D Box(double X0, double X1, double Z0, double Z1) { return FBox2D(FVector2D(FMath::Min(X0, X1), FMath::Min(Z0, Z1)), FVector2D(FMath::Max(X0, X1), FMath::Max(Z0, Z1))); }
 FBox2D Foot(const FHome& H) { return Box(H.X0, H.X1, H.Z0, H.Z1); }
 bool HasYard(const FHome& H) { return H.YardX1 > H.YardX0; }
 FBox2D Yard(const FHome& H) { return Box(H.YardX0, H.YardX1, H.Z0, H.Z1); }
 bool Meets(const FBox2D& A, const FBox2D& B, double Pad) { return A.Min.X < B.Max.X + Pad && A.Max.X > B.Min.X - Pad && A.Min.Y < B.Max.Y + Pad && A.Max.Y > B.Min.Y - Pad; }
 // The yard's fence: three runs from the house's back wall out and round, a gate in the last one by the house. Road-local.
 struct FRun { FVector2D A, B; };
 TArray<FRun> Runs(const FHome& H)
 {
  TArray<FRun> Out; if (!HasYard(H)) return Out;
  const double Back = H.Front < 0 ? H.YardX0 : H.YardX1, Far = H.Front < 0 ? H.YardX1 : H.YardX0, Out1 = Far > Back ? 1. : -1.;
  Out.Add({ FVector2D(Back, H.Z0), FVector2D(Far, H.Z0) });
  Out.Add({ FVector2D(Far, H.Z0), FVector2D(Far, H.Z1) });
  // (the gate: a metre and a bit, half a metre out from the house)
  Out.Add({ FVector2D(Far, H.Z1), FVector2D(Back + Out1 * 1.6, H.Z1) });
  Out.Add({ FVector2D(Back + Out1 * .45, H.Z1), FVector2D(Back, H.Z1) });
  return Out;
 }
 FBox2D RunBox(const FRun& R, double Half) { return Box(R.A.X, R.B.X, R.A.Y, R.B.Y).ExpandBy(FVector2D(Half, Half)); }
 double Ground(AS47Web* W, double X, double Z)
 {
  FVector O(RoadX, 0, 0);
  if (W) if (const FS47Obj G = W->Obj(TEXT("world.town.group"))) O = G.WorldPosition();
  return O.Y + S47RoadTerrain::SurfaceY(X + RoadX - O.X, Z - O.Z);
 }
 FS47Rect Rect(const FBox2D& B, const FString& Id) { FS47Rect R; R.MinX = RoadX + B.Min.X; R.MaxX = RoadX + B.Max.X; R.MinZ = B.Min.Y; R.MaxZ = B.Max.Y; R.Id = Id; return R; }
 uint32 Hash(uint32 A, uint32 B) { uint32 H = A * 0x9e3779b1u ^ (B + 0x7f4a7c15u) * 0x85ebca6bu; H ^= H >> 15; H *= 0x2c1b3c6du; H ^= H >> 12; return H; }
 double Unit(uint32 A, uint32 B) { return double(Hash(A, B) & 0xffffff) / double(0xffffff); }

 // ---- drawing: instanced boxes and cylinders in the web's own lit material, one batch a colour ----
 struct FKit
 {
  AS47Web* W = nullptr; int32 Parent = -1; USceneComponent* ParentComp = nullptr;
  UStaticMesh* Cube = nullptr; UStaticMesh* Cyl = nullptr; UMaterialInterface* Lit = nullptr; UMaterialInterface* Unlit = nullptr;
  TMap<FString, UInstancedStaticMeshComponent*> Batches;
  UMaterialInstanceDynamic* Colour(uint32 Hex, float Rough)
  {
   UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Lit, W);
   Mid->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(Hex)); Mid->SetScalarParameterValue(TEXT("Roughness"), Rough);
   return Mid;
  }
  UInstancedStaticMeshComponent* Batch(const FString& Key, bool bRound, uint32 Hex, float Rough)
  {
   if (UInstancedStaticMeshComponent** Have = Batches.Find(Key)) return *Have;
   auto* C = NewObject<UInstancedStaticMeshComponent>(W);
   C->SetMobility(EComponentMobility::Movable); C->SetStaticMesh(bRound ? Cyl : Cube); C->SetMaterial(0, Colour(Hex, Rough));
   C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCastShadow(true); C->SetCullDistances(16000, 20000);
   C->RegisterComponent(); C->AttachToComponent(ParentComp, FAttachmentTransformRules::KeepWorldTransform);
   C->SetWorldTransform(FTransform(S47Three::ToUE(RoadX, 0, 2180)));
   W->AdoptNode(C, Parent, TEXT("newhomes:") + Key);
   Batches.Add(Key, C); return C;
  }
  // A box: its middle (road-local x, height, z), its size across x, up and across z, turned Yaw degrees about the vertical.
  void Add(const FString& Key, uint32 Hex, float Rough, const FVector& At, const FVector& Size, double Yaw = 0, double Pitch = 0, bool bRound = false)
  {
   Batch(Key, bRound, Hex, Rough)->AddInstance(FTransform(FRotator(Pitch, Yaw, 0), S47Three::ToUE(At.X + RoadX, At.Y, At.Z), FVector(Size.X, Size.Z, Size.Y)), true);
  }
  UStaticMeshComponent* Glow(const FVector& At, const FVector& Size, UMaterialInstanceDynamic* Mid, const FString& Name)
  {
   auto* C = NewObject<UStaticMeshComponent>(W);
   C->SetupAttachment(ParentComp); C->SetMobility(EComponentMobility::Movable); C->SetStaticMesh(Cube); C->SetMaterial(0, Mid);
   C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCastShadow(false); C->RegisterComponent();
   C->SetWorldLocation(S47Three::ToUE(At.X + RoadX, At.Y, At.Z)); C->SetWorldScale3D(FVector(Size.X, Size.Z, Size.Y));
   W->AdoptNode(C, Parent, TEXT("newhomes:") + Name);
   return C;
  }
  UMaterialInstanceDynamic* Pane()
  {
   UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Unlit, W);
   Mid->SetVectorParameterValue(TEXT("Color"), FLinearColor(.045f, .04f, .035f)); Mid->SetScalarParameterValue(TEXT("UseMap"), 0.f);
   Mid->SetScalarParameterValue(TEXT("UseInstanceColor"), 0.f); Mid->SetScalarParameterValue(TEXT("Untonemapped"), 0.f);
   return Mid;
  }
  UPointLightComponent* Light(const FVector& At, double Radius, bool bShadows)
  {
   auto* L = NewObject<UPointLightComponent>(W);
   L->SetupAttachment(ParentComp); L->SetMobility(EComponentMobility::Movable); L->SetIntensityUnits(ELightUnits::Lumens); L->SetIntensity(0);
   L->SetLightColor(FLinearColor(1, .8f, .56f)); L->SetAttenuationRadius(float(Radius * 100)); L->SetCastShadows(bShadows); L->RegisterComponent();
   L->SetWorldLocation(S47Three::ToUE(At.X + RoadX, At.Y, At.Z));
   return L;
  }
 };
 constexpr uint32 Latilla = 0x8a7860, LatillaDark = 0x6b5c48, PostDark = 0x4a3c2e, Galv = 0x9a9d98, GalvDark = 0x7c807c, FlagRed = 0xa3322a;

 // The mailbox at the street's edge (the old houses have theirs at x 18.7 and -17.3): a post, the rounded box on it, its flag.
 void Mailbox(FKit& K, double X, double Z, double Toward)
 {
  const double G = Ground(K.W, X, Z);
  K.Add(TEXT("post"), PostDark, .9f, FVector(X, G + .55, Z), FVector(.09, 1.1, .09));
  K.Add(TEXT("galv"), Galv, .45f, FVector(X, G + 1.155, Z), FVector(.48, .11, .2));
  // (a cylinder lying along x: its height is its length)
  K.Add(TEXT("galvRound"), Galv, .45f, FVector(X, G + 1.21, Z), FVector(.2, .48, .2), 0, 90, true);
  K.Add(TEXT("flag"), FlagRed, .7f, FVector(X - Toward * .08, G + 1.3, Z + .115), FVector(.04, .2, .015));
 }
 // A galvanized garbage can with its lid.
 void Can(FKit& K, double X, double Z, uint32 Seed)
 {
  const double G = Ground(K.W, X, Z);
  K.Add(TEXT("galvRound"), Galv, .45f, FVector(X, G + .39, Z), FVector(.56, .78, .56), 0, 0, true);
  K.Add(TEXT("galvDarkRound"), GalvDark, .5f, FVector(X, G + .805, Z), FVector(.6, .05, .6), 0, 0, true);
  K.Add(TEXT("galvDark"), GalvDark, .5f, FVector(X, G + .85, Z), FVector(.2, .04, .04), Unit(Seed, 3) * 180);
 }
 // A coyote fence: latillas side by side, a little uneven, on two rails.
 void Fence(FKit& K, const FRun& R, uint32 Seed)
 {
  const FVector2D D = R.B - R.A; const double L = D.Size(); if (L < .2) return;
  const FVector2D U = D / L;
  const int32 N = FMath::Max(2, FMath::CeilToInt(L / .1));
  for (int32 I = 0; I <= N; ++I)
  {
   const FVector2D P = R.A + U * (L * I / N) + FVector2D(U.Y, -U.X) * ((Unit(Seed, I * 5) - .5) * .03);
   const double G = Ground(K.W, P.X, P.Y), H = 1.32 + Unit(Seed, I * 5 + 1) * .36, T = .05 + Unit(Seed, I * 5 + 2) * .03;
   K.Add(Unit(Seed, I * 5 + 3) < .45 ? TEXT("latillaDark") : TEXT("latilla"), Unit(Seed, I * 5 + 3) < .45 ? LatillaDark : Latilla, .95f,
    FVector(P.X, G + H * .5 - .05, P.Y), FVector(T, H, T), Unit(Seed, I * 5 + 4) * 90);
  }
  const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
  const FVector2D Mid = (R.A + R.B) * .5; const double G = (Ground(K.W, R.A.X, R.A.Y) + Ground(K.W, R.B.X, R.B.Y)) * .5;
  for (const double Up : { .35, 1.05 }) K.Add(TEXT("post"), PostDark, .9f, FVector(Mid.X, G + Up, Mid.Y), FVector(L, .05, .07), Yaw);
 }
 void AddWalk(AS47Web* W, const FBox2D& B, const FString& Id)
 {
  for (const TCHAR* Area : { TEXT("road"), TEXT("diner") }) W->CollidersMutable(Area).Add(Rect(B, Id));
 }

 // The trailer: Tom's motorhome where it is imported, set down on the land end on to the street; else a single-wide of boxes.
 void BuildTrailer(FKit& K, const FHome& H, FState& S)
 {
  const FBox2D F = Foot(H);
  double G = -1e9; for (const double X : { F.Min.X, F.Max.X }) for (const double Z : { F.Min.Y, F.Max.Y }) G = FMath::Max(G, Ground(K.W, X, Z));
  const FVector2D Mid = F.GetCenter();
  UStaticMesh* Mesh = FPackageName::DoesPackageExist(MotorhomePackage) ? LoadObject<UStaticMesh>(nullptr, Motorhome, nullptr, LOAD_NoWarn | LOAD_Quiet) : nullptr;
  double Top = G + 2.6, Len = F.GetSize().X, Wid = F.GetSize().Y;
  if (Mesh)
  {
   const FBox B = Mesh->GetBoundingBox(); const FVector Size = B.GetSize();
   const bool bAlongX = Size.X >= Size.Y;
   const double L = FMath::Max(Size.X, Size.Y) / 100, Wd = FMath::Min(Size.X, Size.Y) / 100;
   const double Scale = FMath::Min3(1., F.GetSize().X / FMath::Max(L, .1), F.GetSize().Y / FMath::Max(Wd, .1));
   const FRotator Rot(0, bAlongX ? 0 : 90, 0);
   const FVector Off = Rot.RotateVector(FVector(B.GetCenter().X, B.GetCenter().Y, 0)) * Scale;
   auto* C = NewObject<UStaticMeshComponent>(K.W);
   C->SetupAttachment(K.ParentComp); C->SetMobility(EComponentMobility::Movable); C->SetStaticMesh(Mesh);
   C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore); C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
   C->SetCastShadow(true); C->RegisterComponent();
   // (sunk two centimetres: its tyres in the dirt, not over it)
   C->SetWorldLocationAndRotation(FVector((Mid.X + RoadX) * 100 - Off.X, Mid.Y * 100 - Off.Y, (G - .02) * 100 - B.Min.Z * Scale), Rot);
   C->SetWorldScale3D(FVector(Scale));
   K.W->AdoptNode(C, K.Parent, TEXT("newhomes:trailer"));
   Len = L * Scale; Wid = Wd * Scale; Top = G - .02 + Size.Z / 100 * Scale; S.bMotorhome = true;
  }
  else
  {
   // A sixties single-wide on blocks: skirting, the body, a brown band, the roof's cap, its door and steps on the south side.
   const double Body0 = G + .45, Body1 = G + 2.75;
   K.Add(TEXT("skirt"), 0x8c8577, .95f, FVector(Mid.X, G + .2, Mid.Y), FVector(Len - .2, .5, Wid - .2));
   K.Add(TEXT("trailer"), 0xd8ccb0, .7f, FVector(Mid.X, (Body0 + Body1) * .5, Mid.Y), FVector(Len, Body1 - Body0, Wid));
   K.Add(TEXT("band"), 0x6b4a32, .7f, FVector(Mid.X, Body0 + 1.0, Mid.Y), FVector(Len + .02, .22, Wid + .02));
   K.Add(TEXT("cap"), 0xb9b2a4, .6f, FVector(Mid.X, Body1 + .03, Mid.Y), FVector(Len + .04, .06, Wid + .04));
   K.Add(TEXT("band"), 0x6b4a32, .7f, FVector(F.Max.X - 1.3, Body0 + .95, F.Max.Y + .015), FVector(.8, 1.9, .03));
   // (two timber steps up to the door: the lower one further out)
   K.Add(TEXT("post"), PostDark, .9f, FVector(F.Max.X - 1.3, G + .1, F.Max.Y + .4), FVector(1.0, .2, .8));
   K.Add(TEXT("post"), PostDark, .9f, FVector(F.Max.X - 1.3, G + .2, F.Max.Y + .2), FVector(1.0, .4, .4));
   Top = Body1;
  }
  S.Trailer = Box(Mid.X - Len * .5, Mid.X + Len * .5, Mid.Y - Wid * .5, Mid.Y + Wid * .5);
  AddWalk(K.W, S.Trailer, TEXT("newhomes:H08:trailer"));
  S.Obstacles.Add(FS47Obstacle::Box(RoadX + S.Trailer.Min.X, RoadX + S.Trailer.Max.X, S.Trailer.Min.Y, S.Trailer.Max.Y));
  // Its window toward the south, in the living end (away from the street), glowing while he is home.
  {
   FWindow Win; Win.Id = H.Id; UMaterialInstanceDynamic* Mid2 = K.Pane(); Win.Pane = Mid2;
   const double WinY = FMath::Lerp(G, Top, .62), WinX = S.Trailer.Min.X + Len * .3;
   K.Glow(FVector(WinX, WinY, S.Trailer.Max.Y + .015), FVector(.9, .5, .02), Mid2, TEXT("H08:window"));
   if (!S.bMotorhome) K.Glow(FVector(S.Trailer.Min.X + Len * .62, WinY, S.Trailer.Max.Y + .015), FVector(.7, .5, .02), Mid2, TEXT("H08:window2"));
   Win.Spill = K.Light(FVector(WinX, WinY, S.Trailer.Max.Y + .7), 5, false); Win.Lumens = 90;
   S.Windows.Add(Win);
  }
  // A yard light on a post by the street end: the home's lamp (S47CalicheAccess::AddLamp, so S47TownDay lights it while he is in).
  {
   const double PX = S.Trailer.Max.X + .7, PZ = S.Trailer.Max.Y + .5, PG = Ground(K.W, PX, PZ);
   K.Add(TEXT("post"), PostDark, .9f, FVector(PX, PG + 1.3, PZ), FVector(.12, 2.6, .12));
   K.Add(TEXT("post"), PostDark, .9f, FVector(PX - .2, PG + 2.52, PZ), FVector(.42, .05, .05));
   FWindow Bulb; Bulb.Id = H.Id; UMaterialInstanceDynamic* BulbMid = K.Pane(); Bulb.Pane = BulbMid;
   K.Glow(FVector(PX - .38, PG + 2.43, PZ), FVector(.08, .1, .08), BulbMid, TEXT("H08:bulb"));
   S.Windows.Add(Bulb);
   UPointLightComponent* L = K.Light(FVector(PX - .38, PG + 2.36, PZ), 9, true);
   S.TrailerLamp = L;
   AddWalk(K.W, Box(PX - .15, PX + .15, PZ - .15, PZ + .15), TEXT("newhomes:H08:post"));
   S.Obstacles.Add(FS47Obstacle::Circle(RoadX + PX, PZ, .2));
  }
 }

 // Caliche's plants (S47CalicheNature, S47RouteNature) under a house, a yard, the trailer or a car: shrunk to nothing, never removed.
 void HidePlants(AS47Web* W, FState& S)
 {
  if (S.bPlants || !W) return;
  TArray<UInstancedStaticMeshComponent*> All; W->GetComponents<UInstancedStaticMeshComponent>(All);
  int32 Kinds = 0;
  for (UInstancedStaticMeshComponent* C : All)
  {
   const FString Name = C ? C->GetName() : FString();
   if (!C || !(Name.StartsWith(TEXT("Nature_")) || Name.StartsWith(TEXT("RouteNature_")))) continue;
   ++Kinds; bool bAny = false;
   for (int32 I = 0; I < C->GetInstanceCount(); ++I)
   {
    FTransform T; if (!C->GetInstanceTransform(I, T, true)) continue;
    const FVector2D At(T.GetLocation().X / 100. - RoadX, T.GetLocation().Y / 100.);
    if (!S.Under.ContainsByPredicate([&At](const FBox2D& B) { return B.IsInside(At); })) continue;
    if (T.GetScale3D().GetMax() < .001) continue;
    T.SetScale3D(FVector(.0001)); C->UpdateInstanceTransform(I, T, true, false, true); bAny = true; ++S.PlantsHidden;
   }
   if (bAny) { C->MarkRenderStateDirty(); if (auto* Tree = Cast<UHierarchicalInstancedStaticMeshComponent>(C)) Tree->BuildTreeIfOutdated(false, true); }
  }
  // The town's plants are built with the scene; a few tries in case they come after.
  S.bPlants = Kinds > 0 || ++S.PlantTries > 30;
  if (S.bPlants) UE_LOG(LogS47NewHomes, Display, TEXT("S47: new homes: %d plants under them shrunk away (%d plant batches)"), S.PlantsHidden, Kinds);
 }
}

const TArray<S47CalicheNewHomes::FHome>& S47CalicheNewHomes::Homes() { return S47CalicheNewHomesLocal::Table; }

void S47CalicheNewHomes::Build(US47Main* M)
{
 using namespace S47CalicheNewHomesLocal;
 if (FParse::Param(FCommandLine::Get(), TEXT("S47CalicheNewHomesOff")) || !M || !M->Web) return;
 AS47Web* W = M->Web;
 FState& S = State;
 // The same scene again (the rooms made afresh): its lamp goes back on the list, nothing is built twice.
 if (S.Web.Get() == W && S.bBuilt) { if (UPointLightComponent* L = S.TrailerLamp.Get()) S47CalicheAccess::AddLamp(TEXT("H08"), L, 260); return; }
 S = FState(); S.Web = W;
 FKit K; K.W = W; K.Parent = W->Handle(TEXT("world.town.group"));
 const auto* PN = W->Node(K.Parent);
 K.ParentComp = PN ? PN->Comp.Get() : nullptr;
 K.Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
 K.Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 K.Lit = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 K.Unlit = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Unlit_Opaque.M_Web_Unlit_Opaque"));
 if (!K.ParentComp || !K.Cube || !K.Cyl || !K.Lit || !K.Unlit) { UE_LOG(LogS47NewHomes, Warning, TEXT("S47: new homes: no town group or basic shapes; only their rooms")); return; }
 for (auto& R : W->CollidersMutable(TEXT("road"))) if (R.Id.StartsWith(TEXT("newhomes:"))) R.bEnabled = false;
 for (auto& R : W->CollidersMutable(TEXT("diner"))) if (R.Id.StartsWith(TEXT("newhomes:"))) R.bEnabled = false;
 int32 Houses = 0;
 for (int32 Index = 0; Index < Table.Num(); ++Index)
 {
  const FHome& H = Table[Index]; const uint32 Seed = FCrc::StrCrc32(H.Id);
  if (H.bTrailer) BuildTrailer(K, H, S);
  else
  {
   // Its room: built by S47CalicheAccess from the assembly (left out there if a part did not load: then nothing here either).
   FVector Floor;
   if (!S47CalicheAccess::RoomMiddle(H.Id, Floor)) { UE_LOG(LogS47NewHomes, Warning, TEXT("S47: new homes: %s has no room; left out"), H.Id); continue; }
   ++Houses;
   S.Obstacles.Add(FS47Obstacle::Box(RoadX + H.X0, RoadX + H.X1, H.Z0, H.Z1));
   // The front window (Tools/calichehomes: 1.5 by 1.15 m, its middle 1.5 m up): a pane inside the opening, a little light outside.
   const double FX = H.Front > 0 ? H.X1 : H.X0;
   FWindow Win; Win.Id = H.Id; UMaterialInstanceDynamic* Mid = K.Pane(); Win.Pane = Mid;
   K.Glow(FVector(FX - H.Front * .09, Floor.Y + 1.5, H.WindowZ), FVector(.02, 1.17, 1.52), Mid, FString(H.Id) + TEXT(":window"));
   Win.Spill = K.Light(FVector(FX + H.Front * .7, Floor.Y + 1.4, H.WindowZ), 5, false); Win.Lumens = 110;
   S.Windows.Add(Win);
   for (const FRun& R : Runs(H))
   {
    Fence(K, R, Seed + uint32(S.Obstacles.Num()) * 131u);
    const FBox2D B = RunBox(R, .08);
    AddWalk(W, B, FString::Printf(TEXT("newhomes:%s:fence"), H.Id));
    S.Obstacles.Add(FS47Obstacle::Box(RoadX + B.Min.X, RoadX + B.Max.X, B.Min.Y, B.Max.Y));
   }
  }
  Mailbox(K, H.MailX, H.MailZ, H.MailX > 0 ? -1. : 1.);
  AddWalk(W, Box(H.MailX - .12, H.MailX + .12, H.MailZ - .12, H.MailZ + .12), FString::Printf(TEXT("newhomes:%s:mailbox"), H.Id));
  S.Obstacles.Add(FS47Obstacle::Circle(RoadX + H.MailX, H.MailZ, .2));
  Can(K, H.CanX, H.CanZ, Seed);
  AddWalk(W, Box(H.CanX - .3, H.CanX + .3, H.CanZ - .3, H.CanZ + .3), FString::Printf(TEXT("newhomes:%s:can"), H.Id));
  S.Obstacles.Add(FS47Obstacle::Circle(RoadX + H.CanX, H.CanZ, .35));
  S.Under.Add(Foot(H).ExpandBy(1.0));
  if (HasYard(H)) S.Under.Add(Yard(H).ExpandBy(.6));
  S.Under.Add(Box(H.MailX - .5, H.MailX + .5, H.MailZ - .5, H.MailZ + .5));
  S.Under.Add(Box(H.CanX - .6, H.CanX + .6, H.CanZ - .6, H.CanZ + .6));
 }
 for (const FCar& C : Cars) S.Under.Add(Box(C.X0, C.X1, C.Z0, C.Z1).ExpandBy(.5));
 if (UPointLightComponent* L = S.TrailerLamp.Get()) S47CalicheAccess::AddLamp(TEXT("H08"), L, 260);
 for (auto& Pair : K.Batches) if (Pair.Value) Pair.Value->MarkRenderStateDirty();
 S.bBuilt = true;
 HidePlants(W, S);
 UE_LOG(LogS47NewHomes, Display, TEXT("S47: new homes: %d houses dressed (yard, mailbox, can, window), the trailer as %s; %d obstacles for the truck"),
  Houses, S.bMotorhome ? TEXT("the Tripo motorhome") : TEXT("a single-wide of boxes (no motorhome imported)"), S.Obstacles.Num());
}

void S47CalicheNewHomes::Update(US47Main* M)
{
 using namespace S47CalicheNewHomesLocal;
 FState& S = State;
 if (!S.bBuilt || !M || S.Web.Get() != M->Web.Get()) return;
 HidePlants(M->Web, S);
 // The truck: once the road's leg is made (US47World::EnsureLegs), and again for a new one. Never removed (others keep places by index).
 if (M->World)
  if (IS47DriveArea* Area = M->World->LegArea(TEXT("road")); Area && Area != S.DriveFor)
  {
   for (const FS47Obstacle& Mine : S.Obstacles)
    if (!Area->Obstacles.ContainsByPredicate([&Mine](const FS47Obstacle& O) { return O.bCircle == Mine.bCircle && FMath::IsNearlyEqual(O.MinX, Mine.MinX, 1e-6) && FMath::IsNearlyEqual(O.MaxZ, Mine.MaxZ, 1e-6) && FMath::IsNearlyEqual(O.X, Mine.X, 1e-6) && FMath::IsNearlyEqual(O.Z, Mine.Z, 1e-6); }))
     Area->Obstacles.Add(Mine);
   S.DriveFor = Area;
  }
 // The windows: lit while the home's lamp is (somebody home and awake), against the dark only.
 const float Mask = M->Web->ExteriorDaylightMask(M->Web->FloodSet(TEXT("town")), 1);
 for (FWindow& Win : S.Windows)
 {
  const float K = float(FMath::Clamp(S47CalicheAccess::LampLevel(Win.Id), 0., 1.5)) * Mask;
  if (FMath::Abs(K - Win.Last) < .01f) continue;
  Win.Last = K;
  if (UMaterialInstanceDynamic* Mid = Win.Pane.Get()) Mid->SetVectorParameterValue(TEXT("Color"), FLinearColor(.045f, .04f, .035f) + FLinearColor(1.f, .7f, .4f) * (1.5f * K));
  if (UPointLightComponent* L = Win.Spill.Get()) L->SetIntensity(float(Win.Lumens * K));
 }
}

int32 S47CalicheNewHomes::Measure(US47Main* M, TArray<FString>& Lines)
{
 using namespace S47CalicheNewHomesLocal;
 if (!M || !M->Web) { Lines.Add(TEXT("FAIL new homes: no world")); return 1; }
 int32 Bad = 0; TArray<FString> Each;
 const auto Note = [&](bool bOk, const FString& What) { Bad += !bOk; Each.Add(FString::Printf(TEXT("%s %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), *What)); };
 for (const FHome& H : Table)
 {
  const bool bBuilt = H.bTrailer ? State.bBuilt && State.Trailer.bIsValid : S47CalicheAccess::HasDoor(H.Id);
  // Clear of the walk's colliders that are not its own (its walls and door, its fence, mailbox and can).
  FString In;
  for (const TCHAR* Area : { TEXT("road"), TEXT("diner") }) for (const FS47Rect& R : M->Web->CollidersMutable(Area))
  {
   if (!R.bEnabled || R.Id.Contains(H.Id) || R.MaxX - R.MinX > 60 || R.MaxZ - R.MinZ > 60) continue;
   const FBox2D C = Box(R.MinX - RoadX, R.MaxX - RoadX, R.MinZ, R.MaxZ);
   if (Meets(C, Foot(H).ExpandBy(-.05), 0) || (HasYard(H) && Meets(C, Yard(H).ExpandBy(-.15), 0))) { In = R.Id.IsEmpty() ? TEXT("(unnamed)") : R.Id; break; }
  }
  const FString Spot = S47TownDay::SpotNow(H.Who);
  const bool bHome = S47TownDay::Active() && S47TownDay::HomeAwake(Spot);
  const double Level = S47CalicheAccess::LampLevel(H.Id);
  const bool bLampOk = !S47TownDay::Active() || (Level > .5) == bHome;
  Note(bBuilt && In.IsEmpty() && bLampOk, FString::Printf(TEXT("%s (%s, %s): %s%s; %s, lamp %.2f"), H.Id, H.Who, H.bTrailer ? TEXT("trailer") : TEXT("house"),
   bBuilt ? TEXT("built") : TEXT("NOT BUILT"), In.IsEmpty() ? TEXT("") : *(TEXT(", in ") + In), Spot.IsEmpty() ? TEXT("no day running") : *Spot, Level));
 }
 Lines.Add(FString::Printf(TEXT("%s new homes: %d homes, %d windows, %d plants shrunk away, the trailer %s; %d wrong"), Bad ? TEXT("FAIL") : TEXT("PASS"),
  Table.Num(), State.Windows.Num(), State.PlantsHidden, State.bMotorhome ? TEXT("the motorhome") : TEXT("of boxes"), Bad));
 Lines.Append(Each);
 return Bad;
}

bool S47CalicheNewHomes::SelfTest(TArray<FString>& Lines)
{
 using namespace S47CalicheNewHomesLocal;
 bool bOk = true;
 const auto Check = [&](bool B, const FString& Why) { bOk &= B; Lines.Add(FString::Printf(TEXT("%s caliche homes: %s"), B ? TEXT("PASS") : TEXT("FAIL"), *Why)); };
 // Everything that stands: the new homes with their yards, mailboxes, cans and the props' cars; the buildings in the assembly.
 struct FThing { FString Id; FBox2D B; };
 TArray<FThing> Mine, Others;
 for (const FHome& H : Table)
 {
  Mine.Add({ H.Id, Foot(H) });
  if (HasYard(H)) Mine.Add({ FString(H.Id) + TEXT(" yard"), Yard(H) });
  Mine.Add({ FString(H.Id) + TEXT(" mailbox"), Box(H.MailX - .12, H.MailX + .12, H.MailZ - .12, H.MailZ + .12) });
  Mine.Add({ FString(H.Id) + TEXT(" can"), Box(H.CanX - .3, H.CanX + .3, H.CanZ - .3, H.CanZ + .3) });
 }
 for (const FCar& C : Cars) Mine.Add({ C.Id, Box(C.X0, C.X1, C.Z0, C.Z1) });
 const auto OwnerOf = [](const FString& Id) { return Id.Left(3); };
 // The rooms' assembly: H05 to H07 there, seated, with the footprints this table has.
 TMap<FString, FBox2D> Houses;
 {
  FString Text; TSharedPtr<FJsonObject> J; int32 Agree = 0;
  if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("CalicheAccess/assembly.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) && J)
   for (const auto& V : J->GetArrayField(TEXT("buildings")))
   {
    const auto B = V->AsObject(); const FString Id = B->GetStringField(TEXT("id"));
    const auto& Root = B->GetArrayField(TEXT("root")); const auto& Fp = B->GetArrayField(TEXT("footprint"));
    const double RX = Root[0]->AsNumber() - RoadX, RZ = Root[2]->AsNumber();
    const FBox2D F = Box(RX + Fp[0]->AsNumber(), RX + Fp[1]->AsNumber(), RZ - Fp[3]->AsNumber(), RZ - Fp[2]->AsNumber());
    Houses.Add(Id, F);
    if (const FHome* H = Table.FindByPredicate([&Id](const FHome& X) { return Id == X.Id; }))
    {
     bool bSeat = false; B->TryGetBoolField(TEXT("ground_seat"), bSeat);
     Agree += bSeat && F.Min.Equals(Foot(*H).Min, .01) && F.Max.Equals(Foot(*H).Max, .01);
    }
    else Others.Add({ Id, F });
   }
  Check(Agree == 3, FString::Printf(TEXT("H05 to H07 are rooms in Content/CalicheAccess/assembly.json, seated on the land, on this table's footprints (%d of 3)"), Agree));
 }
 for (const FBox2D& B : S47TownSites::SolidFootprints()) Others.Add({ TEXT("a temporary box (S47TownSites)"), B });
 // Not in the road, on the power line, by a street lamp, outside the fence.
 {
  constexpr double LampZ[] = { 2030, 2092, 2122, 2150, 2184, 2222, 2262 };
  FString Why;
  for (const FThing& T : Mine)
  {
   const bool bMail = T.Id.EndsWith(TEXT("mailbox"));
   const double Clear = bMail ? 16.5 : 18.0; // (the mailboxes at the street's edge, as the old houses' are)
   if (T.B.Max.X > -Clear && T.B.Min.X < Clear) Why = T.Id + TEXT(" by the highway");
   if (T.B.Min.X < 17.5 && T.B.Max.X > 12.5) Why = T.Id + TEXT(" under the power line");
   for (const double Z : LampZ) if (Meets(T.B, Box(-7.4, -6.6, Z - .4, Z + .4), 1.0)) Why = T.Id + TEXT(" at a street lamp");
   if (T.B.Min.X < -41 || T.B.Max.X > 47 || T.B.Min.Y < 2019 || T.B.Max.Y > 2299) Why = T.Id + TEXT(" outside the town's fence");
  }
  Check(Why.IsEmpty(), TEXT("no house, yard, mailbox, can or car in the road, under the power line, at a street lamp or outside the town's fence") + (Why.IsEmpty() ? FString() : TEXT(": ") + Why));
 }
 // Not in another house, nor in each other.
 {
  FString Why;
  for (const FThing& T : Mine)
  {
   for (const FThing& O : Others) if (Meets(T.B, O.B, 2.0)) Why = T.Id + TEXT(" at ") + O.Id;
   for (const FThing& O : Mine) if (&O != &T && OwnerOf(O.Id) != OwnerOf(T.Id) && Meets(T.B, O.B, 1.5)) Why = T.Id + TEXT(" at ") + O.Id;
  }
  Check(Why.IsEmpty() && Others.Num() >= 10, FString::Printf(TEXT("no new home in another house or building (%d of them) or a temporary box, two metres apart"), Others.Num()) + (Why.IsEmpty() ? FString() : TEXT(": ") + Why));
 }
 // Not on a place for the night's events (where lights hang, the way in) other than its own.
 {
  FS47EventPlaces Places(nullptr); FString Why; int32 Own = 0;
  for (const FS47EventPlace& P : Places.All())
  {
   const FString Of = P.Id.ToUpper();
   if (Table.ContainsByPredicate([&Of](const FHome& H) { return Of == H.Id; }))
   { ++Own; if (P.Title.Contains(TEXT("empty")) || !P.Has(ES47PlaceTrait::People) || !P.Has(ES47PlaceTrait::Town)) Why = P.Id + TEXT(" is not a lived-in town place"); continue; }
   for (const FThing& T : Mine) for (const FVector2D& At : { FVector2D(P.X, P.Z), FVector2D(P.EntryX, P.EntryZ) })
    if (T.B.ExpandBy(.5).IsInside(At)) Why = T.Id + TEXT(" on ") + P.Id;
  }
  Check(Why.IsEmpty() && Own == Table.Num(), FString::Printf(TEXT("h05 to h08 on the event places, lived in (%d), and no home on another place's spot"), Own) + (Why.IsEmpty() ? FString() : TEXT(": ") + Why));
 }
 // Not in anything the export stands there (its walk's colliders and the road leg's obstacles), where the export can be read.
 {
  FString Text; TSharedPtr<FJsonObject> J;
  if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("WebData/manifest.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) || !J)
   Lines.Add(TEXT("SKIP caliche homes: Content/WebData/manifest.json not read (the export's colliders)"));
  else
  {
   TArray<FBox2D> Export;
   const TSharedPtr<FJsonObject>* Walks = nullptr; const TSharedPtr<FJsonObject>* Road = nullptr; const TArray<TSharedPtr<FJsonValue>>* Cs = nullptr;
   if (J->TryGetObjectField(TEXT("walks"), Walks) && (*Walks)->TryGetObjectField(TEXT("road"), Road) && (*Road)->TryGetArrayField(TEXT("colliders"), Cs))
    for (const auto& V : *Cs) { const auto O = V->AsObject(); Export.Add(Box(O->GetNumberField(TEXT("minX")) - RoadX, O->GetNumberField(TEXT("maxX")) - RoadX, O->GetNumberField(TEXT("minZ")), O->GetNumberField(TEXT("maxZ")))); }
   const TSharedPtr<FJsonObject>* Drive = nullptr; const TSharedPtr<FJsonObject>* Legs = nullptr; const TSharedPtr<FJsonObject>* Leg = nullptr; const TArray<TSharedPtr<FJsonValue>>* Os = nullptr;
   if (J->TryGetObjectField(TEXT("drive"), Drive) && (*Drive)->TryGetObjectField(TEXT("legs"), Legs) && (*Legs)->TryGetObjectField(TEXT("road"), Leg) && (*Leg)->TryGetArrayField(TEXT("obstacles"), Os))
    for (const auto& V : *Os)
    {
     const TArray<TSharedPtr<FJsonValue>>& A = V->AsArray();
     if (A.Num() == 4) Export.Add(Box(A[0]->AsNumber() - RoadX, A[1]->AsNumber() - RoadX, A[2]->AsNumber(), A[3]->AsNumber()));
     else if (A.Num() == 3) Export.Add(Box(A[0]->AsNumber() - RoadX - A[2]->AsNumber(), A[0]->AsNumber() - RoadX + A[2]->AsNumber(), A[1]->AsNumber() - A[2]->AsNumber(), A[1]->AsNumber() + A[2]->AsNumber()));
    }
   FString Why;
   for (const FThing& T : Mine) for (const FBox2D& E : Export) if (Meets(T.B, E, .3)) Why = FString::Printf(TEXT("%s in the export's %.1f-%.1f x %.1f-%.1f"), *T.Id, E.Min.X, E.Max.X, E.Min.Y, E.Max.Y);
   Check(Why.IsEmpty() && Export.Num() > 100, FString::Printf(TEXT("nothing of the new homes in the export's %d walk colliders and road obstacles"), Export.Num()) + (Why.IsEmpty() ? FString() : TEXT(": ") + Why));
  }
 }
 // Every one of the town's people has a home to sleep in, and sleeps in it every day; those who work by day are home at 02:00.
 {
  for (const FHome& H : Table) if (H.bTrailer) Houses.Add(H.Id, Foot(H));
  const int32 Seeds[] = { 1, 47, -9, 2024 };
  FString Why; TMap<FString, FString> Owner;
  for (const FString& Who : S47TownDay::People())
  {
   const FString Home = S47TownDay::HomeOf(Who);
   if (Who == TEXT("attendant"))
   {
    // (the night man lives at his station: its back room)
    for (int32 Seed : Seeds) for (int32 Day = 0; Day < 7; ++Day)
     if (S47TownDay::PlaceOf(S47TownDay::SpotIn(S47TownDay::Plan(Seed, Day, Day % 7, Who), 2.)) != TEXT("gas")) Why = TEXT("the attendant not at his station at 02:00");
    continue;
   }
   const FBox2D* F = Houses.Find(Home);
   if (Home.IsEmpty() || !F) { Why = Who + TEXT(" has no house"); continue; }
   if (const FString* Other = Owner.Find(Home)) Why = Home + TEXT(" is both ") + *Other + TEXT("'s and ") + Who + TEXT("'s");
   Owner.Add(Home, Who);
   const bool bDayWork = Who == TEXT("deputy") || Who == TEXT("cook") || Who == TEXT("souvenir_man") || Who == TEXT("regular");
   for (int32 Seed : Seeds) for (int32 Day = 0; Day < 7; ++Day)
   {
    const TArray<S47TownDay::FRow> Rows = S47TownDay::Plan(Seed, Day, Day % 7, Who);
    double Slept = 0;
    for (const S47TownDay::FRow& R : Rows) if (R.Spot.StartsWith(TEXT("sleep.")))
    {
     const S47TownDay::FSpot* Sp = S47TownDay::Spot(R.Spot);
     if (!Sp || !F->ExpandBy(.01).IsInside(FVector2D(Sp->X - RoadX, Sp->Z))) Why = Who + TEXT(" sleeps outside ") + Home;
     Slept += R.To - R.From;
    }
    if (Slept < 5) Why = FString::Printf(TEXT("%s sleeps %.1f h at %s"), *Who, Slept, *Home);
    if (bDayWork && S47TownDay::PlaceOf(S47TownDay::SpotIn(Rows, 2.)) != TEXT("home")) Why = Who + TEXT(" not at home at 02:00");
   }
  }
  for (const FHome& H : Table) if (!Owner.Contains(H.Id)) Why = FString(H.Id) + TEXT(" with nobody living in it");
  Check(Why.IsEmpty() && Owner.Num() >= 7, FString::Printf(TEXT("every one of the town's people has a home to sleep in (%d homes lived in, the attendant at his station) and sleeps there every day; the day workers are home at 02:00"), Owner.Num())
   + (Why.IsEmpty() ? FString() : TEXT(": ") + Why));
 }
 return bOk;
}
