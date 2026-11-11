#include "S47StreetLamps.h"
#include "S47Night.h"
#include "S47SaroPower.h"
#include "S47Three.h"
#include "S47Web.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/Package.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "RenderUtils.h"
#include "StaticMeshResources.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47StreetLamps, Log, All);

namespace S47StreetLamps
{
namespace
{
 // ---- unreal/ArtSource/StreetLamps/model_contract.json: three metres, each mesh in its own terms, y up, the arm along +x ----
 constexpr double PoleHeight = 8.0;
 const FVector ArmOnPole(0, 7.6, 0);    // the arm's root on a pole standing on its foot (assembly_three_m.arm_origin)
 const FVector ArmEnd(1.80, .45, 0);    // the arm's end, where the head's slipfitter takes it, from the arm's root (head_pivot - arm_origin)
 const FVector EmitAt(.42, -.173, 0);   // 8 mm under the bowl's lowest point, from the head's pivot (the slipfitter mouth)
 const FVector EmitNormal(0, -1, 0);
 constexpr float LensWidth = 50, LensDepth = 28; // centimetres
 const TCHAR* const LensSlot = TEXT("M_StreetLamp_Lens");

 struct FLamp
 {
  FString Light;          // the exterior light it carries (S47ExteriorLighting's name)
  FVector Foot = FVector::ZeroVector; // the pole's base, three world metres
  TWeakObjectPtr<UStaticMeshComponent> Pole, Arm, Head;
  TWeakObjectPtr<UMaterialInstanceDynamic> Lens;
  FVector Emitter = FVector::ZeroVector, Normal = FVector(0, -1, 0);
  TArray<TWeakObjectPtr<UPrimitiveComponent>> Under; // what it was set on (the footing), for the measure
  FGlow Glow;                              // STREETLAMP-GLOW: the lens on every LOD and the halo under the bowl
  TWeakObjectPtr<USpotLightComponent> Spot; // the exterior light's spot, as S47ExteriorLighting last lit it (for the measure)
 };
 TMap<TWeakObjectPtr<AS47Web>, TArray<FLamp>>& All() { static TMap<TWeakObjectPtr<AS47Web>, TArray<FLamp>> A; return A; }

 UStaticMesh* Asset(const TCHAR* Name)
 {
  return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Signal47/Art/StreetLamps/%s.%s"), Name, Name));
 }
 UStaticMeshComponent* NodeMesh(AS47Web* W, int32 Index, int32 Geo)
 {
  const FS47WebNode* N = W->Node(Index);
  return N && N->Geo == Geo ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
 }
 // An export batch with every triangle wholly inside one of the boxes (the mesh's own terms, three metres) taken out: the same runtime
 // render-data clone as S47ExteriorFixtures' patch, keeping every other triangle, vertex, UV, colour, slot and the collision body.
 UStaticMesh* Cut(AS47Web* W, UStaticMeshComponent* C, const TArray<FBox>& Boxes, TArray<int32>& Counts, TArray<double>* Bottoms = nullptr, UStaticMesh* Source = nullptr)
 {
  // Source: the same batch as an asset with CPU data, when what the component draws has none (the motel lot's lens batch, sweep 3)
  UStaticMesh* Original = Source ? Source : C ? static_cast<UStaticMesh*>(C->GetStaticMesh()) : nullptr;
  Counts.Init(0, Boxes.Num());
  if (Bottoms) Bottoms->Init(1e9, Boxes.Num());
  if (!Original || !Original->bAllowCPUAccess || !Original->GetRenderData() || Original->GetRenderData()->LODResources.Num() != 1) return nullptr;
  const auto& Src = Original->GetRenderData()->LODResources[0]; const auto& B = Src.VertexBuffers;
  if (!B.PositionVertexBuffer.GetVertexData() || !B.StaticMeshVertexBuffer.GetTangentData() || !B.StaticMeshVertexBuffer.GetTexCoordData() || Src.IndexBuffer.GetIndexDataSize() == 0) return nullptr;
  TArray<uint32> Old, Keep; Src.IndexBuffer.GetCopy(Old); Keep.Reserve(Old.Num());
  TArray<FStaticMeshSection> Sections;
  const auto In = [&](uint32 V, const FBox& Bx) { return Bx.IsInsideOrOn(S47Three::ToThree(FVector(B.PositionVertexBuffer.VertexPosition(V)))); };
  for (const FStaticMeshSection& Section : Src.Sections)
  {
   FStaticMeshSection Copy = Section; Copy.FirstIndex = Keep.Num(); Copy.NumTriangles = 0;
   for (uint32 I = Section.FirstIndex; I < Section.FirstIndex + Section.NumTriangles * 3; I += 3)
   {
    if (I + 2 >= uint32(Old.Num())) return nullptr;
    const uint32 A = Old[I], Bb = Old[I + 1], Cc = Old[I + 2];
    if (FMath::Max3(A, Bb, Cc) >= B.PositionVertexBuffer.GetNumVertices()) return nullptr;
    int32 Box = -1;
    for (int32 J = 0; J < Boxes.Num(); ++J) if (In(A, Boxes[J]) && In(Bb, Boxes[J]) && In(Cc, Boxes[J])) { Box = J; break; }
    if (Box >= 0)
    {
     ++Counts[Box];
     if (Bottoms) for (const uint32 V : { A, Bb, Cc }) (*Bottoms)[Box] = FMath::Min((*Bottoms)[Box], S47Three::ToThree(FVector(B.PositionVertexBuffer.VertexPosition(V))).Y);
     continue;
    }
    Keep.Append({ A, Bb, Cc }); ++Copy.NumTriangles;
   }
   if (Copy.NumTriangles) Sections.Add(Copy);
  }
  auto* Out = NewObject<UStaticMesh>(W, NAME_None, RF_Transient);
  Out->bAllowCPUAccess = true; Out->NeverStream = true; Out->bSupportRayTracing = true;
  Out->GetStaticMaterials() = Original->GetStaticMaterials(); Out->SetBodySetup(Original->GetBodySetup());
  auto Data = MakeUnique<FStaticMeshRenderData>(); Data->AllocateLODResources(1);
  Data->Bounds = Original->GetRenderData()->Bounds; Data->ScreenSize[0].Default = 1;
  Data->NumInlinedLODs = 1; Data->CurrentFirstLODIdx = 0; Data->bReadyForStreaming = false;
  auto& Dst = Data->LODResources[0]; Dst.Sections.Append(Sections);
  Dst.VertexBuffers.PositionVertexBuffer.Init(B.PositionVertexBuffer, true);
  auto& Tangents = Dst.VertexBuffers.StaticMeshVertexBuffer;
  Tangents.SetUseHighPrecisionTangentBasis(B.StaticMeshVertexBuffer.GetUseHighPrecisionTangentBasis());
  Tangents.SetUseFullPrecisionUVs(B.StaticMeshVertexBuffer.GetUseFullPrecisionUVs());
  Tangents.Init(B.StaticMeshVertexBuffer.GetNumVertices(), B.StaticMeshVertexBuffer.GetNumTexCoords(), true);
  for (uint32 I = 0; I < B.StaticMeshVertexBuffer.GetNumVertices(); ++I)
  {
   Tangents.SetVertexTangents(I, FVector3f(B.StaticMeshVertexBuffer.VertexTangentX(I)), B.StaticMeshVertexBuffer.VertexTangentY(I), FVector3f(B.StaticMeshVertexBuffer.VertexTangentZ(I)));
   for (uint32 U = 0; U < B.StaticMeshVertexBuffer.GetNumTexCoords(); ++U) Tangents.SetVertexUV(I, U, B.StaticMeshVertexBuffer.GetVertexUV(I, U));
  }
  const uint32 Colors = B.ColorVertexBuffer.GetNumVertices();
  if (Colors)
  {
   if (!B.ColorVertexBuffer.GetVertexData()) return nullptr;
   Dst.VertexBuffers.ColorVertexBuffer.Init(Colors, true);
   for (uint32 I = 0; I < Colors; ++I) Dst.VertexBuffers.ColorVertexBuffer.VertexColor(I) = B.ColorVertexBuffer.VertexColor(I);
  }
  Dst.bHasColorVertexData = Colors > 0; Dst.bBuffersInlined = true; Dst.bIsOptionalLOD = false;
  // D45-20261110: keep the cut copy's indices on the CPU (as S47MotelV2's cut does). In a packaged game the RHI upload discarded them,
  // so Field(), cutting the array's posts from Highway()'s copy of g26, read no triangles and the eleven field heads were never built.
  Dst.IndexBuffer.TrySetAllowCPUAccess(true);
  Dst.SourceMeshBounds = Data->Bounds; Dst.MaxDeviation = 0; Dst.IndexBuffer.SetIndices(Keep, EIndexBufferStride::Force32Bit);
  bool InitRT = Out->bSupportRayTracing;
#if !WITH_EDITOR
  InitRT = InitRT && IsRayTracingAllowed();
#endif
  if (InitRT) Data->InitializeRayTracingRepresentationFromRenderingLODs();
  Out->SetRenderData(MoveTemp(Data)); Out->SetExtendedBounds(Original->GetBounds());
  return Out;
 }
 // The first visible thing straight down from From, but none of Ignore.
 // bAnyShown false: whatever is there, drawn now or not (at build an area away is hidden, and its ground is no less there).
 // MaxY: anything higher is passed through (sweep 3: the highway's feet met a surface at y -0.06, half a metre over the footings).
 bool Down(UWorld* World, const FVector& From, double Depth, const TArray<UPrimitiveComponent*>& Ignore, double& Y, FString* What = nullptr, bool bShownOnly = true, double MaxY = 1e9)
 {
  FCollisionQueryParams Q(SCENE_QUERY_STAT(S47StreetLampFoot), true);
  for (UPrimitiveComponent* P : Ignore) if (P) Q.AddIgnoredComponent(P);
  for (int32 Try = 0; Try < 10; ++Try)
  {
   FHitResult Hit;
   if (!World->LineTraceSingleByChannel(Hit, S47Three::ToUE(From), S47Three::ToUE(From - FVector(0, Depth, 0)), ECC_Visibility, Q)) return false;
   UPrimitiveComponent* P = Hit.GetComponent();
   if (P && (!bShownOnly || (P->IsVisible() && !P->bHiddenInGame)) && S47Three::ToThree(Hit.ImpactPoint).Y <= MaxY)
   {
    Y = S47Three::ToThree(Hit.ImpactPoint).Y;
    if (What) *What = P->GetName();
    return true;
   }
   Q.AddIgnoredComponent(P);
  }
  return false;
 }
 UStaticMeshComponent* Place(AS47Web* W, USceneComponent* Parent, int32 ParentNode, const FString& Name, UStaticMesh* Mesh, const FVector& At, const FQuat& Q, const FVector& Scale = FVector::OneVector)
 {
  auto* C = NewObject<UStaticMeshComponent>(W, *FString::Printf(TEXT("S47StreetLamp_%s"), *Name.Replace(TEXT(":"), TEXT("_"))));
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(Parent); C->SetStaticMesh(Mesh);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCollisionResponseToAllChannels(ECR_Ignore);
  // (M6 sweep 4: the arm's, head's and wood arm's LOD1/2 had come in turned 90 degrees about the pole; imported again on 9 November
  // through import_recipe.import_lod_same_frame, every LOD within 0.4 cm of LOD0's frame, so no LOD is forced)
  C->SetCastShadow(true); C->RegisterComponent();
  // STREETLAMP-GLOW: the LOD1/2 slots draw what LOD0's slot of the same name draws (they hold WorldGridMaterial in the asset)
  MatchLodSlots(C);
  double Matrix[16]; S47Three::Compose(At, Q, Scale, Matrix); C->SetWorldTransform(S47Three::MatToUE(Matrix));
  // AdoptNode keeps only where it is; the turn goes into the node too, or the next time the node is set the arm swings back to +x
  // (sweep 3: the heads stood over the road, the arms pointed the other way).
  const int32 Node = W->AdoptNode(C, ParentNode, TEXT("streetLamp:") + Name);
  if (Node >= 0) W->ObjAt(Node).SetQuaternion(W->NodeWorldThree(ParentNode).GetRotation().Inverse() * Q);
  if (Node >= 0 && !Scale.Equals(FVector::OneVector)) W->SetScale(Node, Scale);
  return C;
 }
 bool Disabled() { return FParse::Param(FCommandLine::Get(), TEXT("S47StreetLampsOff")) || FParse::Param(FCommandLine::Get(), TEXT("S47PropsOff")); }

 // ---- the highway west of SARO: Exterior.ts road(), eight poles at x -18.5 with their arms west over the road, in g26 ----
 // A pole on each foot, its arm turned by Q, the head on the arm's end, the lens a material of its own for the light's feed.
 void Raise(AS47Web* W, int32 ParentNode, const TCHAR* Family, const TCHAR* LightPrefix, const TArray<FVector>& Feet, const FQuat& Turn,
  const TArray<UPrimitiveComponent*>& Under, UStaticMesh* PoleMesh, UStaticMesh* ArmMesh, UStaticMesh* HeadMesh, TArray<FLamp>& Out,
  const TArray<FQuat>& Each = {}, double Height = 1)
 {
  // Each: a turn per lamp instead of Turn. Height: the pole's height as a share of the model's eight metres (the arm sits as far down).
  const FS47WebNode* P = W->Node(ParentNode);
  USceneComponent* ParentComp = P ? P->Comp.Get() : nullptr;
  if (!ParentComp) return;
  for (int32 I = 0; I < Feet.Num(); ++I)
  {
   FLamp L; L.Light = FString::Printf(TEXT("%s%02d"), LightPrefix, I + 1); L.Foot = Feet[I];
   for (UPrimitiveComponent* U : Under) L.Under.Add(U);
   const FQuat Q = Each.IsValidIndex(I) ? Each[I] : Turn;
   const FVector Root = Feet[I] + Q.RotateVector(ArmOnPole * FVector(1, Height, 1)), Slip = Root + Q.RotateVector(ArmEnd);
   L.Pole = Place(W, ParentComp, ParentNode, FString::Printf(TEXT("%s:%d:pole"), Family, I), PoleMesh, Feet[I], Q, FVector(1, Height, 1));
   L.Arm = Place(W, ParentComp, ParentNode, FString::Printf(TEXT("%s:%d:arm"), Family, I), ArmMesh, Root, Q);
   UStaticMeshComponent* Head = Place(W, ParentComp, ParentNode, FString::Printf(TEXT("%s:%d:head"), Family, I), HeadMesh, Slip, Q);
   // D45-20261110: the head casts no shadow, so its bowl and housing never shade its own spot (pole and arm keep theirs).
   Head->SetCastShadow(false);
   L.Head = Head;
   L.Emitter = Slip + Q.RotateVector(EmitAt); L.Normal = Q.RotateVector(EmitNormal);
   // STREETLAMP-GLOW: one dark lens material on every LOD's lens slot (it was on LOD0's alone) and the halo under the bowl
   L.Glow = Dress(W, Head); L.Lens = L.Glow.Lens;
   if (!L.Lens.IsValid()) UE_LOG(LogS47StreetLamps, Warning, TEXT("S47: street lamp %s has no %s slot; its light stays where the old one was"), *L.Light, LensSlot);
   Out.Add(L);
  }
 }

 bool Highway(AS47Web* W, UStaticMesh* PoleMesh, UStaticMesh* ArmMesh, UStaticMesh* HeadMesh, TArray<FLamp>& Out)
 {
  UStaticMeshComponent* Batch = NodeMesh(W, 35, 26);
  const int32 Parent = W->Handle(TEXT("ext.group"));
  const FS47WebNode* P = W->Node(Parent);
  if (!Batch || !P || !P->Comp.IsValid() || !Batch->GetComponentTransform().Equals(FTransform::Identity, .001))
  { UE_LOG(LogS47StreetLamps, Warning, TEXT("S47: street lamps: the highway's pole batch (node 35, g26) or ext.group differs; the old lamps stay")); return false; }
  static const double Zs[] = { 70., 36., 2., -32., -66., -100., -134., -168. };
  // Each old pole and its arm whole: the cylinder from -0.6 to 7.4 round x -18.5, the arm out to x -20.1 at 7.3 (Exterior.ts).
  TArray<FBox> Boxes;
  for (const double Z : Zs) Boxes.Add(FBox(FVector(-20.55, -.65, Z - .32), FVector(-18.2, 8.2, Z + .32)));
  TArray<int32> Counts;
  UStaticMesh* Without = Cut(W, Batch, Boxes, Counts);
  bool bAll = Without != nullptr;
  for (const int32 N : Counts) bAll &= N >= 12;
  if (!bAll)
  {
   UE_LOG(LogS47StreetLamps, Warning, TEXT("S47: street lamps: the highway's old poles were not all found in g26 (%s); the old lamps stay"), *FString::JoinBy(Counts, TEXT(","), [](int32 N) { return FString::FromInt(N); }));
   return false;
  }
  // The footings (S47RouteLampFeet): the new pole has its own base plate and anchor bolts, so the old sleeve and plates go;
  // the concrete stays, and the pole stands on its top. The cobra heads it gave the old arms go too.
  TArray<UPrimitiveComponent*> Old;
  UPrimitiveComponent* Concrete = nullptr;
  for (int32 I = 0; I < W->PartCount(); ++I)
  {
   const FS47WebNode* N = W->Node(I);
   UPrimitiveComponent* C = N ? Cast<UPrimitiveComponent>(N->Comp.Get()) : nullptr;
   if (!C) continue;
   if (N->Name == TEXT("routeLampFeet:road:concrete")) Concrete = C;
   if (N->Name == TEXT("routeLampFeet:road:plates") || N->Name == TEXT("routeLampFeet:road:hardware") || N->Name.StartsWith(TEXT("routeLampHead:road:"))) Old.Add(C);
  }
  UWorld* World = W->GetWorld();
  TArray<UPrimitiveComponent*> Ignore = Old; Ignore.Add(Batch);
  TArray<FVector> Feet;
  for (const double Z : Zs)
  {
   double Y = 0; FString What;
   // The footings' tops lie near -0.61 and the ground at -0.62 (S47RouteLampFeet measured within 12 cm): nothing over -0.3 is a foot.
   if (!Down(World, FVector(-18.5, 1.5, Z), 4, Ignore, Y, &What, false, -.3))
   { UE_LOG(LogS47StreetLamps, Warning, TEXT("S47: street lamps: nothing under the highway pole at z %.0f; the old lamps stay"), Z); return false; }
   Feet.Add(FVector(-18.5, Y, Z));
   UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamp road z %.0f stands on %s at y %.3f"), Z, *What, Y);
  }
  Without->InitResources(); Batch->SetStaticMesh(Without);
  for (UPrimitiveComponent* C : Old) { C->SetVisibility(false); C->SetHiddenInGame(true); C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
  TArray<UPrimitiveComponent*> Under; if (Concrete) Under.Add(Concrete);
  Raise(W, Parent, TEXT("road"), TEXT("Highway"), Feet, FQuat(FVector(0, 1, 0), PI) /* the arm's +x turned to -x, over the road */, Under, PoleMesh, ArmMesh, HeadMesh, Out);
  UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamps: the highway's eight lamps are cobra heads on davit arms; %s old triangles cut"), *FString::JoinBy(Counts, TEXT(","), [](int32 N) { return FString::FromInt(N); }));
  return true;
 }

 // ---- Caliche's Main Street: Town.ts streetLamps(), seven poles at x -7 (town terms) with their arms east over the street to the
 // head at -3.9, in one of the town's merged batches (which one is found by where the triangles are). Each gets a concrete footing,
 // as the web version had none. The lens quads Town.ts lit (world.town.runtime.panels.lamps.*) are hidden: the bowl is the lamp now.
 bool Caliche(AS47Web* W, UStaticMesh* PoleMesh, UStaticMesh* ArmMesh, UStaticMesh* HeadMesh, TArray<FLamp>& Out)
 {
  static const double Zs[] = { 2030, 2092, 2122, 2150, 2184, 2222, 2262 };
  constexpr double LampX = -7, OriginX = 8000;
  const int32 Group = W->Handle(TEXT("world.town.group"));
  const FS47WebNode* G = W->Node(Group);
  if (!G || !G->Comp.IsValid()) { UE_LOG(LogS47StreetLamps, Warning, TEXT("S47: street lamps: no world.town.group; Caliche's lamps stay")); return false; }
  // Pole, rod arm and box head of each, whole (the batch's own terms are the town's: the group stands at x 8000).
  TArray<FBox> Boxes;
  for (const double Z : Zs) Boxes.Add(FBox(FVector(LampX - .35, -10, Z - .3), FVector(-3.3, 25, Z + .3)));
  UStaticMeshComponent* Batch = nullptr; UStaticMesh* Without = nullptr; TArray<int32> Counts; TArray<double> Bases;
  for (int32 I = 0; I < W->PartCount() && !Batch; ++I)
  {
   const FS47WebNode* N = W->Node(I);
   if (!N || N->Geo < 0 || N->bInstanced || !N->Name.IsEmpty()) continue;
   int32 Up = N->Parent; bool bTown = false;
   for (int32 Guard = 0; Up >= 0 && Guard < 8 && !bTown; ++Guard) { bTown = Up == Group; const FS47WebNode* P = W->Node(Up); Up = P ? P->Parent : -1; }
   UStaticMeshComponent* C = bTown ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
   if (!C || !C->GetComponentTransform().Equals(FTransform(FQuat::Identity, S47Three::ToUE(OriginX, 0, 0)), .001)) continue;
   TArray<int32> Got; TArray<double> Low;
   UStaticMesh* Cut0 = Cut(W, C, Boxes, Got, &Low);
   bool bAll = Cut0 != nullptr;
   for (const int32 K : Got) bAll &= K >= 12;
   if (bAll) { Batch = C; Without = Cut0; Counts = Got; Bases = Low; }
  }
  if (!Batch) { UE_LOG(LogS47StreetLamps, Warning, TEXT("S47: street lamps: no town batch holds all seven of Caliche's old lamps; they stay")); return false; }
  // The ground under each, beside the pole (straight down would meet the old pole's collision, which the cut leaves): the sidewalk,
  // from half a metre over the old pole's base (where Town.ts put it, on its own ground) down.
  UWorld* World = W->GetWorld();
  TArray<FVector> Feet;
  for (int32 I = 0; I < UE_ARRAY_COUNT(Zs); ++I)
  {
   const double Z = Zs[I], Base = Bases[I];
   TArray<double> Ys;
   for (const FVector2D& D : { FVector2D(-.4, 0), FVector2D(0, .4), FVector2D(0, -.4) })
   {
    double Y = 0;
    if (Down(World, FVector(OriginX + LampX + D.X, Base + .5, Z + D.Y), 3, {}, Y, nullptr, false)) Ys.Add(Y);
   }
   Ys.Sort();
   const double Y = Ys.Num() ? Ys[Ys.Num() / 2] : Base;
   Feet.Add(FVector(OriginX + LampX, Y, Z));
   UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamp caliche z %.0f: old base y %.3f, the sidewalk beside it at %.3f (%d of 3 found)"), Z, Base, Y, Ys.Num());
  }
  Without->InitResources(); Batch->SetStaticMesh(Without);
  for (const FString& Path : W->PathsUnder(TEXT("world.town.runtime.panels.lamps.")))
   if (const FS47WebNode* Q = W->Node(W->Obj(Path).I))
    if (UPrimitiveComponent* Quad = Cast<UPrimitiveComponent>(Q->Comp.Get())) Quad->SetHiddenInGame(true);
  // A footing each: concrete, 0.45 m square, 30 cm into the ground and 8 cm out of it; the pole stands on its top.
  UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
  UMaterialInterface* Concrete = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SaroSite/Materials/M_SaroSite_Concrete.M_SaroSite_Concrete"));
  TArray<UPrimitiveComponent*> Under;
  if (Cube && Concrete)
  {
   auto* F = NewObject<UInstancedStaticMeshComponent>(W, TEXT("S47StreetLamp_CalicheFootings"));
   F->SetMobility(EComponentMobility::Movable); F->SetupAttachment(G->Comp.Get()); F->SetStaticMesh(Cube); F->SetMaterial(0, Concrete);
   F->SetCollisionEnabled(ECollisionEnabled::QueryOnly); F->SetCollisionResponseToAllChannels(ECR_Ignore); F->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
   F->SetCastShadow(true); F->RegisterComponent();
   for (FVector& Foot : Feet)
   {
    F->AddInstance(FTransform(FQuat::Identity, S47Three::ToUE(Foot.X, Foot.Y - .11, Foot.Z), FVector(.45, .45, .38)), true);
    Foot.Y += .08;
   }
   W->AdoptNode(F, Group, TEXT("streetLamp:caliche:footings"));
   Under.Add(F);
  }
  Raise(W, Group, TEXT("caliche"), TEXT("Caliche"), Feet, FQuat::Identity /* the arm east, over the street */, Under, PoleMesh, ArmMesh, HeadMesh, Out);
  UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamps: Caliche's seven lamps are cobra heads on footings; batch %s, %s old triangles cut"), *Batch->GetName(), *FString::JoinBy(Counts, TEXT(","), [](int32 N) { return FString::FromInt(N); }));
  return true;
 }
}

namespace
{
 // ---- SARO's service yard: ServiceYard.ts, four props.lampPost(4.4, 0.95) along the walk and the truck pad, merged into the yard's
 // static batches: their pole, arm and box head in the pole material's batch (g235, node 387), their bulb faces alone in the bulb
 // material's (g236, node 388; S47ExteriorLighting: only those four faces use it). Each becomes a cobra head on a davit arm, its pole
 // cut to the old post's 4.4 m (the arm 4.18 m up), turned the old arm's way. The pole batch needs CPU data (Tools/streetlamps/
 // prepare_cpu.py); without it the old posts stay.
 bool Yard(AS47Web* W, UStaticMesh* PoleMesh, UStaticMesh* ArmMesh, UStaticMesh* HeadMesh, TArray<FLamp>& Out)
 {
  if (FParse::Param(FCommandLine::Get(), TEXT("S47YardLampsOld"))) return false;
  struct FPost { double X, Z, Turn; };
  static const FPost Posts[] = { { 11.25, 1.6, PI }, { 11.25, -6.0, PI }, { 7.75, -9.2, 0 }, { 6.85, 11.6, 0 } };
  constexpr double OldHeight = 4.4;
  UStaticMeshComponent* Batch = NodeMesh(W, 387, 235);
  UStaticMeshComponent* Bulbs = NodeMesh(W, 388, 236);
  const int32 Parent = W->Handle(TEXT("yard.group"));
  if (!Batch || !Bulbs || !W->Node(Parent) || !Batch->GetComponentTransform().Equals(FTransform::Identity, .001))
  { UE_LOG(LogS47StreetLamps, Warning, TEXT("S47: street lamps: the yard's lamp-post batch (node 387, g235) or its bulbs (388, g236) differ; the old posts stay")); return false; }
  // Each post whole: the pole round its foot, the arm and head out to 0.95 m and a little past, up to 4.6 m.
  TArray<FBox> Boxes;
  for (const FPost& P : Posts)
  {
   const double Out0 = P.Turn > 1 ? -1.25 : -.2, Out1 = P.Turn > 1 ? .2 : 1.25;
   Boxes.Add(FBox(FVector(P.X + Out0, -.2, P.Z - .25), FVector(P.X + Out1, OldHeight + .2, P.Z + .25)));
  }
  TArray<int32> Counts;
  UStaticMesh* Without = Cut(W, Batch, Boxes, Counts);
  bool bAll = Without != nullptr;
  for (const int32 N : Counts) bAll &= N >= 12;
  if (!bAll)
  {
   UE_LOG(LogS47StreetLamps, Warning, TEXT("S47: street lamps: the yard's old posts were not all found in g235 (%s; CPU data: %d); the old posts stay"),
    *FString::JoinBy(Counts, TEXT(","), [](int32 N) { return FString::FromInt(N); }), Batch->GetStaticMesh() && Batch->GetStaticMesh()->bAllowCPUAccess ? 1 : 0);
   return false;
  }
  UWorld* World = W->GetWorld();
  TArray<FVector> Feet; TArray<FQuat> Turns;
  for (const FPost& P : Posts)
  {
   double Y = 0; FString What;
   if (!Down(World, FVector(P.X, 1.5, P.Z), 4, { Batch, Bulbs }, Y, &What, false)) Y = 0;
   Feet.Add(FVector(P.X, Y, P.Z)); Turns.Add(FQuat(FVector(0, 1, 0), P.Turn));
   UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamp yard x %.2f z %.2f stands on %s at y %.3f"), P.X, P.Z, What.IsEmpty() ? TEXT("(nothing found, y 0)") : *What, Y);
  }
  Without->InitResources(); Batch->SetStaticMesh(Without);
  Bulbs->SetVisibility(false); Bulbs->SetHiddenInGame(true); Bulbs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Raise(W, Parent, TEXT("yard"), TEXT("Yard"), Feet, FQuat::Identity, {}, PoleMesh, ArmMesh, HeadMesh, Out, Turns, OldHeight / PoleHeight);
  UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamps: the yard's four posts are cobra heads on 4.4 m poles; %s old triangles cut"), *FString::JoinBy(Counts, TEXT(","), [](int32 N) { return FString::FromInt(N); }));
  return true;
 }
}

namespace
{
 // ---- the motel's lot: MotelFront.ts lotLamps(), two 6.6 m poles at x -28.4 (court terms) with a 1.3 m arm west and a box head, at
 // z 18 and 47. MotelV2 kept them in its trims: the pole and arm in the metal batch (node 678, now /Game/MotelV2/Trim/g463), the head
 // in the black one (676, g461), the lens in the export's lamps batch (685, g470, with the porch lamps). Each becomes a cobra head on a
 // pole cut to 6.6 m, its arm west as the old one. The three batches need CPU data (prepare_motel_cpu.py for the trims, which are in
 // Git; prepare_cpu.py for g470); without it the old lamps stay.
 bool MotelLot(AS47Web* W, UStaticMesh* PoleMesh, UStaticMesh* ArmMesh, UStaticMesh* HeadMesh, TArray<FLamp>& Out)
 {
  if (FParse::Param(FCommandLine::Get(), TEXT("S47MotelLampsOld"))) return false;
  static const double Zs[] = { 18., 47. };
  constexpr double PoleX = -28.4, OldHeight = 6.6;
  const int32 Parent = W->Handle(TEXT("world.motel.group"));
  struct FBatch { int32 Node, Geo, Least; UStaticMeshComponent* C = nullptr; UStaticMesh* Without = nullptr; TArray<int32> Counts; };
  // (sweep 2: node 685 had no CPU data at runtime and no lens was found in it; the lens may as well be in 687, g472, the same lamps
  // material, which TownBuildings retires. The lenses are cut where they are found and the lamps go on without them otherwise.)
  FBatch Batches[] = { { 678, 463, 12 }, { 676, 461, 6 } };
  FBatch Lenses[] = { { 685, 470, 2 }, { 687, 472, 2 } };
  if (!W->Node(Parent)) return false;
  TArray<FBox> Boxes;
  for (const double Z : Zs) Boxes.Add(FBox(FVector(-29.9, -.3, Z - .3), FVector(-28.15, OldHeight + .15, Z + .3)));
  for (FBatch& B : Batches)
  {
   B.C = NodeMesh(W, B.Node, B.Geo);
   B.Without = B.C ? Cut(W, B.C, Boxes, B.Counts) : nullptr;
   bool bAll = B.Without != nullptr;
   for (const int32 N : B.Counts) bAll &= N >= B.Least;
   if (!bAll)
   {
    UE_LOG(LogS47StreetLamps, Warning, TEXT("S47: street lamps: the motel lot's old lamps were not all found in node %d g%d (%s; CPU data %d); they stay"), B.Node, B.Geo,
     *FString::JoinBy(B.Counts, TEXT(","), [](int32 N) { return FString::FromInt(N); }), B.C && B.C->GetStaticMesh() && B.C->GetStaticMesh()->bAllowCPUAccess ? 1 : 0);
    return false;
   }
  }
  TArray<FBatch*> Lens;
  for (FBatch& B : Lenses)
  {
   B.C = NodeMesh(W, B.Node, B.Geo);
   UStaticMesh* Drawn = B.C ? B.C->GetStaticMesh() : nullptr;
   // what is drawn may be a copy without CPU data (sweep 3: g470 had it as an asset, the component's mesh did not); then the asset
   // itself, if it has the data and the same triangles
   UStaticMesh* Source = nullptr;
   if (Drawn && !Drawn->bAllowCPUAccess)
   {
    const FString Path = FString::Printf(TEXT("/Game/Web/Geo/geo_%03d/StaticMeshes/g%d.g%d"), B.Geo / 120, B.Geo, B.Geo);
    UStaticMesh* Asset = LoadObject<UStaticMesh>(nullptr, *Path);
    if (Asset && Asset != Drawn && Asset->bAllowCPUAccess && Asset->GetNumTriangles(0) == Drawn->GetNumTriangles(0)) Source = Asset;
    UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamps: node %d draws %s (%d triangles, no CPU data); the asset %s: %s"), B.Node, *GetPathNameSafe(Drawn), Drawn->GetNumTriangles(0), *Path,
     Source ? TEXT("same triangles with CPU data, cut from it") : Asset ? TEXT("not usable (no CPU data or other triangles)") : TEXT("missing"));
   }
   B.Without = B.C ? Cut(W, B.C, Boxes, B.Counts, nullptr, Source) : nullptr;
   bool bAll = B.Without != nullptr;
   for (const int32 N : B.Counts) bAll &= N >= B.Least;
   UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamps: motel lot lenses in node %d g%d: %s (drawn %s, CPU data %d, shown %d)"), B.Node, B.Geo,
    *FString::JoinBy(B.Counts, TEXT(","), [](int32 N) { return FString::FromInt(N); }), *GetPathNameSafe(Drawn), Drawn && Drawn->bAllowCPUAccess ? 1 : 0, B.C && B.C->IsVisible() ? 1 : 0);
   if (bAll) Lens.Add(&B);
  }
  UWorld* World = W->GetWorld();
  const FTransform Court = W->NodeWorldThree(Parent);
  TArray<FVector> Feet;
  TArray<UPrimitiveComponent*> Ignore; for (const FBatch& B : Batches) Ignore.Add(B.C);
  for (const double Z : Zs)
  {
   const FVector At = Court.TransformPosition(FVector(PoleX, 0, Z));
   double Y = At.Y; FString What;
   if (!Down(World, At + FVector(0, 1.5, 0), 4, Ignore, Y, &What, false)) What = TEXT("(nothing; the court's level)");
   Feet.Add(FVector(At.X, Y, At.Z));
   UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamp motel lot z %.0f stands on %s at y %.3f"), Z, *What, Y);
  }
  for (FBatch& B : Batches) { B.Without->InitResources(); B.C->SetStaticMesh(B.Without); }
  for (FBatch* B : Lens) { B->Without->InitResources(); B->C->SetStaticMesh(B->Without); }
  // Sweep 5: node 685 draws TownBuildings' room trim /Game/TownBuildings/RoomTrim/StaticMeshes/g470, 24 triangles: the two lot lamps'
  // lenses and nothing else (12 each, as lotLamps() makes them), with no CPU data. With both lamps replaced it goes whole.
  if (Lens.IsEmpty())
   if (UStaticMeshComponent* Trim = NodeMesh(W, 685, 470))
    if (UStaticMesh* Mesh = Trim->GetStaticMesh(); Mesh && Mesh->GetPathName().Contains(TEXT("/RoomTrim/")) && Mesh->GetNumTriangles(0) == 24)
    {
     W->SetVisible(685, false); // the node itself, so an area's visibility pass keeps it hidden
     Trim->SetVisibility(false); Trim->SetHiddenInGame(true); Trim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
     UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamps: the motel lot's two old lenses (%s, 24 triangles) hidden whole"), *Mesh->GetPathName());
    }
  Raise(W, Parent, TEXT("motel"), TEXT("CourtLot"), Feet, FQuat(FVector(0, 1, 0), PI) /* the arm west, as the old one */, {}, PoleMesh, ArmMesh, HeadMesh, Out, {}, OldHeight / PoleHeight);
  UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamps: the motel lot's two lamps are cobra heads on 6.6 m poles"));
  return true;
 }
}

namespace
{
 // ---- ARRAY-LAMPS: the array field's eleven posts (Exterior.ts array(), arrayLayout.ts LAMPS), in the highway's pole batch ----
 struct FFieldPost { double X, Z, Height; int32 Dish; };
 // Where each stands, how tall the old post was (it went from -0.6 to 6.4 or 5.4; its glow 0.5 under the top), and the antenna its
 // light falls on (S47Lighting: ArrayDishes for the five floods, BuildArrayLighting's Targets for the six): S-03, S-02, S-01, S-04,
 // S-05, then S-07, S-06, S-09, S-10, S-11, S-12. Each is 11 to 24 m from that antenna's axis; the head, 2.2 m out along the arm, stays
 // at least 1.5 m clear of a reflector's rim at the head's height, however the antenna turns.
 const FFieldPost FieldPosts[] = { { 30, -30, 7, 2 }, { -10, -42, 7, 1 }, { -40, -56, 7, 0 }, { 12, -78, 7, 3 }, { 48, -86, 7, 4 },
  { -70, -94, 6, 6 }, { -33, -100, 6, 5 }, { 60, -146, 6, 8 }, { -52, -158, 6, 9 }, { -4, -190, 6, 10 }, { 94, -110, 6, 11 } };
 constexpr int32 FieldFloods = 5; // the first five carry the site's floods
 // A cobra head throws down and out (its peak some 65 degrees from straight down, cut off above the horizontal by the housing): the
 // spot's axis 40 degrees out from straight down, towards the antenna, its 62 degree cone up to the horizontal.
 constexpr double FieldAimFromDown = 40;
 const TCHAR* const FieldPrefix = TEXT("Array");
 // Each old post whole: the cylinder 0.16 m round at its foot, from -0.6 to its top (the batch's own terms are the site's).
 FBox FieldBox(const FFieldPost& P) { return FBox(FVector(P.X - .3, -.7, P.Z - .3), FVector(P.X + .3, P.Height - .5, P.Z + .3)); }
 // The glow over a post's top (ext.glow.points: 0.5 under the top, or at y -999 while ArrayBack has it out), by where it is.
 int32 FieldGlow(AS47Web* W, const FS47GlowPoints* G, const FFieldPost& P)
 {
  if (!G) return INDEX_NONE;
  const FTransform T = W->NodeWorldThree(G->Node);
  for (int32 I = 0; I < G->Num(); ++I)
  {
   const FVector At = T.TransformPosition(G->Pos[I]);
   if (FMath::Abs(At.X - P.X) < .05 && FMath::Abs(At.Z - P.Z) < .05 && (FMath::Abs(At.Y - (P.Height - .5)) < .05 || At.Y < -900)) return I;
  }
  return INDEX_NONE;
 }
 // Triangles of a batch wholly inside each box, from its CPU data only (never read without bAllowCPUAccess). False: not readable.
 bool CountIn(const UStaticMeshComponent* C, const TArray<FBox>& Boxes, TArray<int32>& Counts)
 {
  Counts.Init(0, Boxes.Num());
  const UStaticMesh* Mesh = C ? C->GetStaticMesh().Get() : nullptr;
  if (!Mesh || !Mesh->bAllowCPUAccess || !Mesh->GetRenderData() || Mesh->GetRenderData()->LODResources.Num() < 1) return false;
  const auto& Src = Mesh->GetRenderData()->LODResources[0]; const auto& B = Src.VertexBuffers.PositionVertexBuffer;
  if (!B.GetVertexData() || Src.IndexBuffer.GetIndexDataSize() == 0) return false;
  TArray<uint32> Index; Src.IndexBuffer.GetCopy(Index);
  for (int32 I = 0; I + 2 < Index.Num(); I += 3)
  {
   if (FMath::Max3(Index[I], Index[I + 1], Index[I + 2]) >= B.GetNumVertices()) return false;
   for (int32 J = 0; J < Boxes.Num(); ++J)
   {
    bool bIn = true;
    for (int32 K = 0; K < 3 && bIn; ++K) bIn = Boxes[J].IsInsideOrOn(S47Three::ToThree(FVector(B.VertexPosition(Index[I + K]))));
    if (bIn) { ++Counts[J]; break; }
   }
  }
  return true;
 }
 bool Field(AS47Web* W, UStaticMesh* PoleMesh, UStaticMesh* ArmMesh, UStaticMesh* HeadMesh, TArray<FLamp>& Out)
 {
  if (FParse::Param(FCommandLine::Get(), TEXT("S47ArrayLampsOld"))) return false;
  // after Highway, which cut its own poles from the same batch: what is drawn now is that copy, with CPU data
  UStaticMeshComponent* Batch = NodeMesh(W, 35, 26);
  const int32 Parent = W->Handle(TEXT("ext.group"));
  const FS47WebNode* P = W->Node(Parent);
  if (!Batch || !P || !P->Comp.IsValid() || !Batch->GetComponentTransform().Equals(FTransform::Identity, .001))
  { UE_LOG(LogS47StreetLamps, Warning, TEXT("S47: street lamps: the array field's pole batch (node 35, g26) or ext.group differs; the old posts stay")); return false; }
  TArray<FBox> Boxes;
  for (const FFieldPost& F : FieldPosts) Boxes.Add(FieldBox(F));
  TArray<int32> Counts;
  UStaticMesh* Without = Cut(W, Batch, Boxes, Counts);
  bool bAll = Without != nullptr;
  for (const int32 N : Counts) bAll &= N >= 12;
  if (!bAll)
  {
   UE_LOG(LogS47StreetLamps, Warning, TEXT("S47: street lamps: the array field's old posts were not all found in g26 (%s; CPU data %d); the old posts stay"),
    *FString::JoinBy(Counts, TEXT(","), [](int32 N) { return FString::FromInt(N); }), Batch->GetStaticMesh() && Batch->GetStaticMesh()->bAllowCPUAccess ? 1 : 0);
   return false;
  }
  // Each foot on the ground that is drawn there, the arm to its antenna. SARO's walking floor (Grounds.ts floorAt: the array graded
  // flat at -0.6) is where to look; the ground mesh under it lies 2 cm lower (N7 at -0.62, as under the highway's footings), and a
  // pole set on the floor stood 20 mm over it (sweep of 9 November: all eleven WRONG in MeasureFeet). The old posts' collision stays
  // in the batch, so the batch is passed through; nothing more than 30 cm from the floor counts as ground.
  UWorld* World = W->GetWorld();
  TArray<FVector> Feet[2]; TArray<FQuat> Turns[2];
  for (int32 I = 0; I < UE_ARRAY_COUNT(FieldPosts); ++I)
  {
   const FFieldPost& F = FieldPosts[I];
   const double Floor = US47Night::SaroFloorAt(W, F.X, F.Z);
   double Y = Floor, Hit = 0; FString What;
   if (World && Down(World, FVector(F.X, Floor + 1.5, F.Z), 3, { Batch }, Hit, &What, false, Floor + .3) && FMath::Abs(Hit - Floor) <= .3) Y = Hit;
   else What = TEXT("(nothing drawn found: SARO's floor)");
   UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamp array x %.0f z %.0f stands on %s at y %.3f (SARO's floor %.3f)"), F.X, F.Z, *What, Y, Floor);
   FVector2D To(-1, 0);
   if (F.Dish < W->DishCount()) To = FVector2D(W->DishX(F.Dish) - F.X, W->DishZ(F.Dish) - F.Z).GetSafeNormal();
   if (To.IsNearlyZero()) To = FVector2D(-1, 0);
   // the arm's +x turned to To about the pole (y up: a turn by a takes +x to (cos a, 0, -sin a))
   const int32 Kind = I < FieldFloods ? 0 : 1;
   Feet[Kind].Add(FVector(F.X, Y, F.Z)); Turns[Kind].Add(FQuat(FVector(0, 1, 0), FMath::Atan2(-To.Y, To.X)));
   UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamp array x %.0f z %.0f: a %.0f m pole on the ground at y %.3f, its arm to dish %d (%.2f, %.2f)"), F.X, F.Z, F.Height, Y, F.Dish, To.X, To.Y);
  }
  Without->InitResources(); Batch->SetStaticMesh(Without);
  // The glow over each old top goes: the bowl and its halo are the lamp now (its colour and place stay, for ArrayBack and SARO's power).
  int32 Hidden = 0;
  if (FS47GlowPoints* G = W->Glow(TEXT("ext.glow.points")))
   for (const FFieldPost& F : FieldPosts)
    if (const int32 I = FieldGlow(W, G, F); I != INDEX_NONE) { G->SetSize(I, 0.f); ++Hidden; }
  Raise(W, Parent, TEXT("array"), TEXT("ArrayFlood"), Feet[0], FQuat::Identity, {}, PoleMesh, ArmMesh, HeadMesh, Out, Turns[0], 7. / PoleHeight);
  Raise(W, Parent, TEXT("arrayPost"), TEXT("ArrayPost"), Feet[1], FQuat::Identity, {}, PoleMesh, ArmMesh, HeadMesh, Out, Turns[1], 6. / PoleHeight);
  UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamps: the array field's eleven posts are cobra heads on 7 and 6 m poles; %s old triangles cut, %d old glows hidden"),
   *FString::JoinBy(Counts, TEXT(","), [](int32 N) { return FString::FromInt(N); }), Hidden);
  return true;
 }
}

FLinearColor Sodium() { return S47Three::Hex(0xffa552); }

bool Build(AS47Web* W)
{
 if (!W || Disabled()) return false;
 UStaticMesh* PoleMesh = Asset(TEXT("SM_StreetLamp_Pole"));
 UStaticMesh* ArmMesh = Asset(TEXT("SM_StreetLamp_Arm"));
 UStaticMesh* HeadMesh = Asset(TEXT("SM_StreetLamp_Head"));
 if (!PoleMesh || !ArmMesh || !HeadMesh)
 {
  UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamps: models not imported (/Game/Signal47/Art/StreetLamps); the old lamps stay"));
  return false;
 }
 if (FMath::Abs(PoleMesh->GetBoundingBox().Max.Z / 100 - PoleHeight) > .15)
  UE_LOG(LogS47StreetLamps, Warning, TEXT("S47: street lamps: the pole is %.2f m, the contract says %.2f"), PoleMesh->GetBoundingBox().Max.Z / 100, PoleHeight);
 TArray<FLamp>& Lamps = All().FindOrAdd(W);
 Lamps.Reset();
 const bool bRoad = Highway(W, PoleMesh, ArmMesh, HeadMesh, Lamps);
 const bool bTown = Caliche(W, PoleMesh, ArmMesh, HeadMesh, Lamps);
 const bool bYard = Yard(W, PoleMesh, ArmMesh, HeadMesh, Lamps);
 const bool bMotel = MotelLot(W, PoleMesh, ArmMesh, HeadMesh, Lamps);
 const bool bField = Field(W, PoleMesh, ArmMesh, HeadMesh, Lamps); // ARRAY-LAMPS (after Highway: the same batch)
 return bRoad || bTown || bYard || bMotel || bField;
}

TMap<FName, S47ExteriorFixtures::FFixture> Fixtures(AS47Web* W)
{
 TMap<FName, S47ExteriorFixtures::FFixture> Out;
 if (const TArray<FLamp>* Lamps = All().Find(W))
  for (const FLamp& L : *Lamps)
  {
   S47ExteriorFixtures::FFixture F;
   F.Mesh = L.Head; F.Lens = L.Lens; F.Aperture = L.Emitter; F.Normal = L.Normal; F.Width = LensWidth; F.Height = LensDepth; F.bStreetLamp = true;
   if (F) Out.Add(FName(*L.Light), F);
  }
 return Out;
}

int32 MeasureFeet(AS47Web* W, TArray<FString>& Lines)
{
 const TArray<FLamp>* Lamps = W ? All().Find(W) : nullptr;
 if (!Lamps || Lamps->IsEmpty()) { Lines.Add(TEXT("Street lamps: none of the new lamps is built (no models, or -S47StreetLampsOff)")); return 0; }
 UWorld* World = W->GetWorld();
 int32 Bad = 0, Measured = 0, Away = 0;
 TArray<FString> Each;
 for (const FLamp& L : *Lamps)
 {
  if (!L.Pole.IsValid()) continue;
  // Only the lamps of the place one is in are drawn, and only what is drawn can be measured against.
  if (!L.Pole->IsVisible() || L.Pole->bHiddenInGame) { ++Away; continue; }
  TArray<UPrimitiveComponent*> Own = { L.Pole.Get(), L.Arm.Get(), L.Head.Get() };
  // the pole's base as drawn now (a relief or a mover may have shifted it since the build)
  const FVector Base = S47Three::ToThree(L.Pole->GetComponentLocation());
  double Support = 0, Ground = 0; FString On, Earth;
  const bool bSupport = Down(World, Base + FVector(0, .5, 0), 3.5, Own, Support, &On);
  TArray<UPrimitiveComponent*> Bare = Own;
  for (const auto& U : L.Under) if (U.IsValid()) Bare.Add(U.Get());
  for (int32 I = 0; I < W->PartCount(); ++I) if (const FS47WebNode* N = W->Node(I); N && N->Name.StartsWith(TEXT("routeLampFeet:"))) Bare.Add(Cast<UPrimitiveComponent>(N->Comp.Get()));
  const bool bGround = Down(World, Base + FVector(0, .5, 0), 3.5, Bare, Ground, &Earth);
  const double Gap = bSupport ? Base.Y - Support : 99;
  const bool bOk = bSupport && FMath::Abs(Gap) <= .02;
  Bad += !bOk; ++Measured;
  Each.Add(FString::Printf(TEXT("  %s %s at x %.2f z %.2f: base y %.3f; on %s at %.3f (gap %+.0f mm); ground %s at %.3f (footing %.0f mm over it)"), bOk ? TEXT("ok") : TEXT("WRONG"), *L.Light,
   Base.X, Base.Z, Base.Y, bSupport ? *On : TEXT("nothing"), Support, Gap * 1000, bGround ? *Earth : TEXT("nothing"), Ground, bGround && bSupport ? (Support - Ground) * 1000 : 0.));
 }
 Lines.Add(FString::Printf(TEXT("Street lamps: %d measured here, %d stand on what is under them within 2 cm, %d wrong (%d in other places, not drawn now)"), Measured, Measured - Bad, Bad, Away));
 Lines.Append(Each);
 return Bad;
}

// ---- STREETLAMP-GLOW ----
namespace
{
 TAutoConsoleVariable<float> CVarHalo(TEXT("s47.lamps.Halo"), 1.f, TEXT("STREETLAMP-GLOW: the cobra heads' halo under the bowl, times (0: none)."));
 TAutoConsoleVariable<float> CVarHaloSize(TEXT("s47.lamps.HaloSize"), 1.1f, TEXT("STREETLAMP-GLOW: the cobra heads' halo, its width in metres."));
 // The halo's centre shows this share of the lens's emissive: a faint veil round a bright bowl, not a ball (critic round 4 found
 // orange clouds beside the heads; M_WebGlow falls from 1 at the centre to 0.55 a quarter out and 0 at the rim).
 constexpr float HaloShare = .04f;
 // The halo's centre 6 cm under the emitter, so the bowl over it does not cut it as one looks up from the road.
 const FVector HaloBelow(0, -.06, 0);
 constexpr float Dark = 1e-4f;
 // Far off (Tom 9 November: the turn-off's lamp was a faint dot until one stood under it) the bowl is under a pixel: the halo keeps
 // at least this angular width (0.26 degrees, some three pixels across a 1080p view, so 13 m wide three kilometres out) and its
 // centre comes up from the faint veil near the lamp to most of the bowl's own brightness, between 60 and 600 m. A sodium lamp
 // reads for kilometres across the desert at night.
 constexpr double HaloMinAngle = .0045, HaloNear = 60, HaloFar = 600;
 constexpr float HaloFarShare = .35f;
 bool CameraAt(const UWorld* World, FVector& Out)
 {
  const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
  if (!PC || !PC->PlayerCameraManager) return false;
  Out = PC->PlayerCameraManager->GetCameraLocation();
  return true;
 }
 // A slot's name without the _N a duplicate may be given.
 FString SlotKey(const FStaticMaterial& Slot)
 {
  FString S = (Slot.MaterialSlotName.IsNone() ? Slot.ImportedMaterialSlotName : Slot.MaterialSlotName).ToString();
  int32 Under = INDEX_NONE;
  if (S.FindLastChar(TEXT('_'), Under) && Under > 0 && Under + 1 < S.Len())
  {
   bool bDigits = true;
   for (int32 I = Under + 1; I < S.Len(); ++I) bDigits &= FChar::IsDigit(S[I]);
   if (bDigits) S.LeftInline(Under);
  }
  return S;
 }
 bool Shown(const UPrimitiveComponent* C) { return C && C->IsVisible() && !C->bHiddenInGame; }
 float Peak(const FLinearColor& C) { return FMath::Max3(C.R, C.G, C.B); }
}

FVector EmitterLocal() { return EmitAt; }
FVector SpotLocal() { return EmitAt + EmitNormal * SpotBelow; }

int32 MatchLodSlots(UStaticMeshComponent* Part)
{
 const UStaticMesh* Mesh = Part ? Part->GetStaticMesh().Get() : nullptr;
 if (!Mesh) return 0;
 const TArray<FStaticMaterial>& Slots = Mesh->GetStaticMaterials();
 TMap<FString, int32> First;
 int32 Set = 0;
 for (int32 I = 0; I < Slots.Num(); ++I)
 {
  const FString Key = SlotKey(Slots[I]);
  if (Key.IsEmpty() || Key == TEXT("None")) continue;
  if (const int32* J = First.Find(Key))
  {
   UMaterialInterface* Want = Part->GetMaterial(*J);
   if (Want && Part->GetMaterial(I) != Want) { Part->SetMaterial(I, Want); ++Set; }
  }
  else First.Add(Key, I);
 }
 return Set;
}

FGlow Dress(UObject* Outer, UStaticMeshComponent* Head)
{
 FGlow G; G.Head = Head;
 const UStaticMesh* Mesh = Head ? Head->GetStaticMesh().Get() : nullptr;
 if (!Mesh) return G;
 if (!Outer) Outer = Head;
 const TArray<FStaticMaterial>& Slots = Mesh->GetStaticMaterials();
 TArray<int32> Lens;
 for (int32 I = 0; I < Slots.Num(); ++I) if (SlotKey(Slots[I]) == LensSlot) Lens.Add(I);
 if (UMaterialInterface* Base = Lens.Num() ? Head->GetMaterial(Lens[0]) : nullptr)
 {
  UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Base, Outer);
  D->SetVectorParameterValue(TEXT("EmissiveFactor"), FLinearColor::Black);
  for (const int32 I : Lens) Head->SetMaterial(I, D);
  G.Lens = D; G.Emissive = FLinearColor::Black;
 }
 MatchLodSlots(Head);
 // The halo: the glow points' own soft spot (M_WebGlow: additive, unlit, Color x Brightness), camera-facing, on the head.
 if (UMaterialInterface* GlowBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebGlow.M_WebGlow")))
 {
  UMaterialBillboardComponent* B = NewObject<UMaterialBillboardComponent>(Outer);
  B->SetupAttachment(Head); B->SetMobility(EComponentMobility::Movable);
  B->SetRelativeLocation(S47Three::ToUE(EmitAt + HaloBelow)); // the head's own terms (Compose/MatToUE conjugate with ToUE)
  UMaterialInstanceDynamic* H = UMaterialInstanceDynamic::Create(GlowBase, Outer);
  H->SetVectorParameterValue(TEXT("Color"), FLinearColor::Black); H->SetScalarParameterValue(TEXT("Brightness"), 1.f);
  G.HaloSize = FMath::Max(.05f, CVarHaloSize.GetValueOnAnyThread());
  B->AddElement(H, nullptr, false, G.HaloSize * 50.f, G.HaloSize * 50.f, nullptr); // half its width each way, centimetres
  B->SetCollisionEnabled(ECollisionEnabled::NoCollision); B->SetCastShadow(false);
  B->bVisibleInRayTracing = false; B->bVisibleInReflectionCaptures = false; B->bVisibleInRealTimeSkyCaptures = false;
  B->SetVisibility(false);
  if (Head->IsRegistered()) B->RegisterComponent();
  G.Halo = B; G.HaloMid = H;
 }
 return G;
}

void Shine(FGlow& G, const FLinearColor& Emissive)
{
 if (UMaterialInstanceDynamic* D = G.Lens.Get(); D && !G.Emissive.Equals(Emissive, Dark)) D->SetVectorParameterValue(TEXT("EmissiveFactor"), Emissive);
 G.Emissive = Emissive;
 G.bLit = Peak(Emissive) > Dark;
 UMaterialBillboardComponent* B = G.Halo.Get();
 if (!B) return;
 const float Scale = FMath::Max(0.f, CVarHalo.GetValueOnAnyThread());
 const bool bHalo = G.bLit && Scale > 0 && Shown(G.Head.Get());
 if (bHalo)
 {
  // how far the eye is (metres; none known, as in the self-test: near)
  double Dist = 0; FVector Cam;
  if (CameraAt(B->GetWorld(), Cam)) Dist = FVector::Dist(Cam, B->GetComponentLocation()) / 100.0;
  const float Far = float(FMath::SmoothStep(HaloNear, HaloFar, Dist));
  if (UMaterialInstanceDynamic* H = G.HaloMid.Get())
  {
   const FLinearColor Want = FLinearColor(Emissive.R, Emissive.G, Emissive.B, 1.f) * (FMath::Lerp(HaloShare, HaloFarShare, Far) * Scale);
   if (!H->K2_GetVectorParameterValue(TEXT("Color")).Equals(Want, Dark)) H->SetVectorParameterValue(TEXT("Color"), FLinearColor(Want.R, Want.G, Want.B, 1.f));
  }
  const float Size = FMath::Max3(.05f, CVarHaloSize.GetValueOnAnyThread(), float(Dist * HaloMinAngle));
  if (B->Elements.Num() && FMath::Abs(Size - G.HaloSize) > FMath::Max(.001f, Size * .02f))
  {
   B->Elements[0].BaseSizeX = B->Elements[0].BaseSizeY = Size * 50.f; G.HaloSize = Size;
   B->UpdateBounds(); B->MarkRenderStateDirty();
  }
 }
 if (B->GetVisibleFlag() != bHalo) B->SetVisibility(bHalo);
}

bool Shine(AS47Web* W, const UMaterialInstanceDynamic* Lens, const FLinearColor& Emissive, USpotLightComponent* Light)
{
 TArray<FLamp>* Lamps = Lens ? All().Find(W) : nullptr;
 if (!Lamps) return false;
 for (FLamp& L : *Lamps)
  if (L.Glow.Lens.Get() == Lens)
  {
   L.Spot = Light;
   Shine(L.Glow, Emissive);
   return true;
  }
 return false;
}

int32 MeasureGlow(AS47Web* W, TArray<FString>& Lines, int32& Lit)
{
 Lit = 0;
 const TArray<FLamp>* Lamps = W ? All().Find(W) : nullptr;
 if (!Lamps || Lamps->IsEmpty()) { Lines.Add(TEXT("Street lamp glow: none of the new lamps is built (no models, or -S47StreetLampsOff)")); return 0; }
 const bool bHaloWanted = CVarHalo.GetValueOnAnyThread() > 0;
 UWorld* World = W->GetWorld();
 int32 Bad = 0, Measured = 0, Away = 0, Dim = 0;
 TArray<FString> Each;
 for (const FLamp& L : *Lamps)
 {
  const UStaticMeshComponent* Head = L.Head.Get();
  if (!Head) continue;
  if (!Shown(Head)) { ++Away; continue; }
  ++Measured;
  const USpotLightComponent* S = L.Spot.Get();
  const FTransform T = Head->GetComponentTransform();
  const FVector Emit = T.TransformPosition(S47Three::ToUE(EmitAt));
  const FBox Box = Head->Bounds.GetBox().ExpandBy(FVector(5, 5, 5 + SpotBelow * 100), FVector(5.0)); // D45-20261110: the spot is SpotBelow under the bowl
  const bool bIn = S && Box.IsInside(S->GetComponentLocation());
  const double Off = S ? FVector::Dist(S->GetComponentLocation(), Emit) : 1e6; // centimetres
  // straight down is -Z in Unreal; the spot's tilt from it
  const double Tilt = S ? FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(-S->GetForwardVector().Z, -1.0, 1.0))) : 180;
  const bool bSpot = S && S->IsVisible() && S->Intensity > 0;
  const bool bHalo = L.Glow.Halo.IsValid() && L.Glow.Halo->GetVisibleFlag();
  Lit += L.Glow.bLit;
  // D45-20261110: the lux the spot gives the ground straight under it: candelas x the cone's falloff x the radius window over the
  // drop squared (Unreal's own spot and radius falloff), so a dark pool under a lit bowl shows here.
  double Lux = -1, Drop = 0;
  if (S && World)
  {
   const FVector From = S47Three::ToThree(S->GetComponentLocation());
   const TArray<UPrimitiveComponent*> Own = { L.Pole.Get(), L.Arm.Get(), L.Head.Get() };
   double Ground = 0;
   if (Down(World, From, 15, Own, Ground))
   {
    Drop = FMath::Max(.1, From.Y - Ground);
    const double CosIn = FMath::Cos(FMath::DegreesToRadians(double(S->InnerConeAngle))), CosOut = FMath::Cos(FMath::DegreesToRadians(double(S->OuterConeAngle)));
    const double Falloff = FMath::Square(FMath::Clamp((FMath::Cos(FMath::DegreesToRadians(Tilt)) - CosOut) / FMath::Max(1e-4, CosIn - CosOut), 0.0, 1.0));
    const double Window = FMath::Square(FMath::Clamp(1.0 - FMath::Square(FMath::Square(Drop * 100.0 / FMath::Max(1.0, double(S->AttenuationRadius)))), 0.0, 1.0));
    Lux = bSpot && S->IntensityUnits == ELightUnits::Candelas ? double(S->Intensity) * Falloff * Window / (Drop * Drop) : 0.0;
   }
  }
  Dim += bSpot && L.Glow.bLit && Lux >= 0 && Lux < 2;
  // D45-20261110: the highway's heads are the county's (S47SaroPower.h): none may stand where SARO's power would dim it.
  const bool bFeed = !L.Light.StartsWith(TEXT("Highway")) || !FS47SaroPower::SaroAt(L.Emitter.X, L.Emitter.Z);
  const bool bOk = bIn && Off <= SpotBelow * 100 + 3 && Tilt <= 45 && (!bSpot || L.Glow.bLit) && bHalo == (L.Glow.bLit && bHaloWanted && L.Glow.Halo.IsValid()) && bFeed;
  Bad += !bOk;
  Each.Add(FString::Printf(TEXT("  %s %s: spot %s the head's bounds, %.0f mm from the emitter, %.0f degrees off straight down; spot %s, lens %s (%.2f), halo %s; %s%s"),
   bOk ? TEXT("ok") : TEXT("WRONG"), *L.Light, bIn ? TEXT("inside") : S ? TEXT("OUTSIDE") : TEXT("(none yet)"), Off * 10, Tilt,
   bSpot ? TEXT("on") : TEXT("off"), L.Glow.bLit ? TEXT("lit") : TEXT("dark"), Peak(L.Glow.Emissive), bHalo ? TEXT("shown") : L.Glow.Halo.IsValid() ? TEXT("hidden") : TEXT("(none: no M_WebGlow)"),
   Lux < 0 ? TEXT("no ground found under it") : *FString::Printf(TEXT("%.0f cd, the ground %.1f m under it gets %.1f lux"), S ? S->Intensity : 0.f, Drop, Lux),
   bFeed ? TEXT("") : TEXT("; ON SARO'S POWER")));
 }
 Lines.Add(FString::Printf(TEXT("Street lamp glow: %d measured here, %d lit, %d wrong, %d lit bowls over a pool under 2 lux (%d in other places, not drawn now)"), Measured, Lit, Bad, Dim, Away));
 Lines.Append(Each);
 return Bad;
}

// ---- ARRAY-LAMPS ----
namespace
{
 FLamp* FieldLampAt(AS47Web* W, double X, double Z)
 {
  TArray<FLamp>* Lamps = W ? All().Find(W) : nullptr;
  if (!Lamps) return nullptr;
  for (FLamp& L : *Lamps)
   if (L.Light.StartsWith(FieldPrefix) && FMath::Square(L.Foot.X - X) + FMath::Square(L.Foot.Z - Z) < 9.0) return &L;
  return nullptr;
 }
 FVector FieldSpot(const FLamp& L) { return L.Emitter + L.Normal * SpotBelow; } // as S47ExteriorLighting puts a cobra head's spot
}

bool FieldHead(AS47Web* W, double X, double Z, FVector& Spot, FVector& Aim)
{
 const FLamp* L = FieldLampAt(W, X, Z);
 if (!L || !L->Head.IsValid()) return false;
 Spot = FieldSpot(*L);
 // out along the arm (the head's emitter from the pole, flat), then down
 FVector Out = L->Emitter - L->Foot; Out.Y = 0;
 Out = Out.GetSafeNormal();
 if (Out.IsNearlyZero()) Out = FVector(-1, 0, 0);
 const double A = FMath::DegreesToRadians(FieldAimFromDown);
 Aim = Spot + (Out * FMath::Sin(A) + FVector(0, -1, 0) * FMath::Cos(A)) * 4.0;
 return true;
}

bool ShineField(AS47Web* W, double X, double Z, const FLinearColor& Emissive, USpotLightComponent* Light)
{
 FLamp* L = FieldLampAt(W, X, Z);
 if (!L || !L->Glow) return false;
 L->Spot = Light;
 FLinearColor E = Emissive;
 // lit exactly when its spot is (MeasureGlow): at the very foot of a feed's ramp a spot that is on keeps the bowl just lit
 if (Light && Light->GetVisibleFlag() && Light->Intensity > 0 && Peak(E) <= Dark * 2) E = Sodium() * (Dark * 4);
 Shine(L->Glow, E);
 return true;
}

int32 MeasureField(AS47Web* W, TArray<FString>& Lines)
{
 if (!W) { Lines.Add(TEXT("Array field lamps: no world")); return 1; }
 int32 Bad = 0, Built = 0;
 TArray<FString> Each;
 // the old posts in the pole batch as it is drawn now
 TArray<FBox> Boxes;
 for (const FFieldPost& F : FieldPosts) Boxes.Add(FieldBox(F));
 TArray<int32> Counts;
 const bool bRead = CountIn(NodeMesh(W, 35, 26), Boxes, Counts);
 int32 Left = 0;
 for (const int32 N : Counts) Left += N;
 // the old glows over the posts' tops
 const FS47GlowPoints* G = W->Glow(TEXT("ext.glow.points"));
 for (int32 I = 0; I < UE_ARRAY_COUNT(FieldPosts); ++I)
 {
  const FFieldPost& F = FieldPosts[I];
  const int32 Gi = FieldGlow(W, G, F);
  const bool bGlow = G && Gi != INDEX_NONE && G->Size.IsValidIndex(Gi) && G->Size[Gi] > .001f;
  const FLamp* L = FieldLampAt(W, F.X, F.Z);
  const UStaticMeshComponent* Head = L ? L->Head.Get() : nullptr;
  Built += Head != nullptr;
  bool bOk = !bGlow && bRead && Counts.IsValidIndex(I) && Counts[I] == 0 && Head && L->Pole.IsValid() && L->Arm.IsValid();
  FString What = FString::Printf(TEXT("old post %s, old glow %s"), !bRead ? TEXT("unread (no CPU data)") : Counts[I] ? *FString::Printf(TEXT("%d triangles LEFT"), Counts[I]) : TEXT("gone"),
   bGlow ? TEXT("SHOWN") : Gi == INDEX_NONE ? TEXT("not found") : TEXT("hidden"));
  if (Head)
  {
   // the spot's place inside the head's bounds, under the bowl (not down the pole), and the lens and halo there
   const FBox Box = Head->Bounds.GetBox();
   const FVector Spot = S47Three::ToUE(FieldSpot(*L));
   const bool bIn = Box.ExpandBy(FVector(5, 5, 5 + SpotBelow * 100), FVector(5.0)).IsInside(Spot) && Spot.Z < Box.Min.Z + 5.0; // (D45-20261110: SpotBelow under the bowl)
   const USpotLightComponent* S = L->Spot.Get();
   const double Off = S ? FVector::Dist(S->GetComponentLocation(), Spot) : 0; // centimetres
   const bool bLens = L->Glow.Lens.IsValid(), bHalo = L->Glow.Halo.IsValid();
   bOk &= bIn && bLens && bHalo && Off <= 6;
   What += FString::Printf(TEXT("; head, lens %s, halo %s, the spot's place %s the head under the bowl, its light %s"), bLens ? TEXT("yes") : TEXT("NONE"), bHalo ? TEXT("yes") : TEXT("NONE (no M_WebGlow)"),
    bIn ? TEXT("in") : TEXT("OUTSIDE"), S ? *FString::Printf(TEXT("%.0f mm from it (%s)"), Off * 10, S->GetVisibleFlag() ? TEXT("on") : TEXT("off")) : TEXT("not bound yet"));
  }
  else What += TEXT("; NO HEAD");
  Bad += !bOk;
  Each.Add(FString::Printf(TEXT("  %s post %d at x %.0f z %.0f (%.0f m): %s"), bOk ? TEXT("ok") : TEXT("WRONG"), I + 1, F.X, F.Z, F.Height, *What));
 }
 Lines.Add(FString::Printf(TEXT("Array field lamps: %d of %d are cobra heads, %s old post triangles left, %d wrong"), Built, int32(UE_ARRAY_COUNT(FieldPosts)),
  bRead ? *FString::FromInt(Left) : TEXT("(unread)"), Bad));
 Lines.Append(Each);
 return Bad;
}

bool SelfTest(TArray<FString>& Lines)
{
 UStaticMesh* HeadMesh = Asset(TEXT("SM_StreetLamp_Head"));
 if (!HeadMesh) { Lines.Add(TEXT("street lamp glow: the models are not imported; skipped")); return true; }
 bool bOk = true;
 const auto Check = [&](bool bGood, const FString& What) { Lines.Add(FString::Printf(TEXT("%s street lamp glow: %s"), bGood ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bGood; };
 UObject* Outer = GetTransientPackage();
 const UMaterialInterface* Grid = UMaterial::GetDefaultMaterial(MD_Surface);
 // every part: no slot left on the engine's default (WorldGridMaterial) once the LOD slots are matched
 for (const TCHAR* Name : { TEXT("SM_StreetLamp_Head"), TEXT("SM_StreetLamp_Arm"), TEXT("SM_StreetLamp_Pole"), TEXT("SM_StreetLamp_WoodArm") })
 {
  UStaticMesh* Mesh = Asset(Name);
  if (!Mesh) { Check(false, FString::Printf(TEXT("%s missing beside the head"), Name)); continue; }
  UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Outer);
  C->SetStaticMesh(Mesh);
  const int32 Set = MatchLodSlots(C);
  int32 Grey = 0;
  for (int32 I = 0; I < C->GetNumMaterials(); ++I) { const UMaterialInterface* M = C->GetMaterial(I); Grey += !M || M == Grid || M->GetName() == TEXT("WorldGridMaterial"); }
  Check(Grey == 0, FString::Printf(TEXT("%s: %d slots, %d matched to LOD0's of the same name, %d left on WorldGridMaterial"), Name, C->GetNumMaterials(), Set, Grey));
 }
 UStaticMeshComponent* Head = NewObject<UStaticMeshComponent>(Outer);
 Head->SetStaticMesh(HeadMesh);
 FGlow G = Dress(Outer, Head);
 Check(G.Lens.IsValid(), TEXT("the head has a lens slot and a dynamic lens material"));
 // every LOD that is drawn has a lens section, and each lens slot draws the one dynamic material
 TArray<int32> Lens;
 for (int32 I = 0; I < HeadMesh->GetStaticMaterials().Num(); ++I) if (SlotKey(HeadMesh->GetStaticMaterials()[I]) == LensSlot) Lens.Add(I);
 int32 OnLens = 0;
 for (const int32 I : Lens) OnLens += G.Lens.IsValid() && Head->GetMaterial(I) == G.Lens.Get();
 Check(Lens.Num() > 0 && OnLens == Lens.Num(), FString::Printf(TEXT("%d of %d lens slots draw the dynamic lens"), OnLens, Lens.Num()));
 if (const FStaticMeshRenderData* R = HeadMesh->GetRenderData())
 {
  FString Each; int32 Without = 0;
  for (int32 Lod = 0; Lod < R->LODResources.Num(); ++Lod)
  {
   int32 Sections = 0;
   for (const FStaticMeshSection& Sec : R->LODResources[Lod].Sections) Sections += Lens.Contains(Sec.MaterialIndex) && Sec.NumTriangles > 0;
   // the far LOD (2) may drop the bowl's lens: the halo stands for it out there (S47StreetLamps::Shine)
   Without += Sections == 0 && Lod < 2;
   Each += FString::Printf(TEXT(" LOD%d %d"), Lod, Sections);
  }
  Check(R->LODResources.Num() > 0 && Without == 0, FString::Printf(TEXT("every near LOD draws its bowl with a lens slot, the far one has the halo (lens sections:%s)"), *Each));
 }
 // the emitter and the spot inside the head's bounds, under the bowl's footprint, the bowl facing down
 const FBox Box = HeadMesh->GetBoundingBox();
 const FVector Emit = S47Three::ToUE(EmitterLocal()), Spot = S47Three::ToUE(SpotLocal());
 const auto UnderLens = [](const FVector& P) { return FMath::Abs(P.X - EmitAt.X) <= LensWidth / 200. && FMath::Abs(P.Z - EmitAt.Z) <= LensDepth / 200.; };
 Check(Box.ExpandBy(2.0).IsInside(Emit) && UnderLens(EmitterLocal()) && EmitNormal.Y < -.99, FString::Printf(TEXT("the emitter %s lies in the head's bounds %s (2 cm slack) under the lens"), *Emit.ToString(), *Box.ToString()));
 // D45-20261110: the spot hangs SpotBelow (10 cm) under the emitter, clear of the bowl: 5 cm slack round the head, 15 cm below it
 Check(Box.ExpandBy(FVector(5, 5, 5 + SpotBelow * 100), FVector(5.0)).IsInside(Spot) && UnderLens(SpotLocal()) && Spot.Z < Box.Min.Z + 5.0 && Spot.Z < Emit.Z - SpotBelow * 100 + .5,
  FString::Printf(TEXT("the spot %s lies %.0f mm under the emitter, under the bowl, not down the pole"), *Spot.ToString(), (Emit.Z - Spot.Z) * 10));
 // D45-20261110: the highway's eight heads hang over the road at x -20.7, on the county's line: SARO's power never dims them
 const FQuat Over(FVector(0, 1, 0), PI);
 const double RoadX = -18.5 + Over.RotateVector(ArmOnPole + ArmEnd + EmitAt).X;
 bool bCounty = true;
 for (const double Z : { 70., 2., -168. }) bCounty &= !FS47SaroPower::SaroAt(RoadX, Z);
 Check(bCounty, FString::Printf(TEXT("the highway's heads at x %.2f are the county's, not on SARO's power"), RoadX));
 // on and off: the lens takes what it is given, the halo shows with it and goes with it
 const FLinearColor On = Sodium() * 6.f;
 Shine(G, On);
 const bool bHaloOn = !G.Halo.IsValid() || G.Halo->GetVisibleFlag() == (CVarHalo.GetValueOnAnyThread() > 0);
 Check(G.Lens.IsValid() && G.Lens->K2_GetVectorParameterValue(TEXT("EmissiveFactor")).Equals(On, .001f) && G.bLit && bHaloOn,
  FString::Printf(TEXT("lit: the lens glows sodium (%s) and the halo shows%s"), *On.ToString(), G.Halo.IsValid() ? TEXT("") : TEXT(" (no M_WebGlow here: no halo)")));
 Shine(G, FLinearColor::Black);
 Check(G.Lens.IsValid() && Peak(G.Lens->K2_GetVectorParameterValue(TEXT("EmissiveFactor"))) <= Dark && !G.bLit && (!G.Halo.IsValid() || !G.Halo->GetVisibleFlag()),
  TEXT("dark: the lens is black and the halo hidden"));
 Head->SetVisibility(false);
 Shine(G, On);
 Check(!G.Halo.IsValid() || !G.Halo->GetVisibleFlag(), TEXT("a hidden head shows no halo, lit or not"));
 return bOk;
}

// D45-20261110 (D45-14): the cut and the count for S47ArrayFence, which takes the array fence's old posts out of the same batch.
UStaticMesh* CutBatch(AS47Web* W, UStaticMeshComponent* Batch, const TArray<FBox>& Boxes, TArray<int32>& Counts)
{
 Counts.Init(0, Boxes.Num());
 return W && Batch ? Cut(W, Batch, Boxes, Counts) : nullptr;
}

bool CountBatch(const UStaticMeshComponent* Batch, const TArray<FBox>& Boxes, TArray<int32>& Counts) { return CountIn(Batch, Boxes, Counts); }
}
