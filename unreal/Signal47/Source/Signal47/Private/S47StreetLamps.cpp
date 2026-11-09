#include "S47StreetLamps.h"
#include "S47Three.h"
#include "S47Web.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
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
 UStaticMesh* Cut(AS47Web* W, UStaticMeshComponent* C, const TArray<FBox>& Boxes, TArray<int32>& Counts, TArray<double>* Bottoms = nullptr)
 {
  UStaticMesh* Original = C ? C->GetStaticMesh() : nullptr;
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
   L.Head = Head;
   L.Emitter = Slip + Q.RotateVector(EmitAt); L.Normal = Q.RotateVector(EmitNormal);
   const int32 Slot = HeadMesh->GetMaterialIndex(LensSlot);
   if (Slot >= 0 && Head->GetMaterial(Slot))
   {
    UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Head->GetMaterial(Slot), W);
    D->SetVectorParameterValue(TEXT("EmissiveFactor"), FLinearColor::Black); Head->SetMaterial(Slot, D); L.Lens = D;
   }
   else UE_LOG(LogS47StreetLamps, Warning, TEXT("S47: street lamp %s has no %s slot; its light stays where the old one was"), *L.Light, LensSlot);
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
  FBatch Batches[] = { { 678, 463, 12 }, { 676, 461, 6 }, { 685, 470, 2 } };
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
  Raise(W, Parent, TEXT("motel"), TEXT("CourtLot"), Feet, FQuat(FVector(0, 1, 0), PI) /* the arm west, as the old one */, {}, PoleMesh, ArmMesh, HeadMesh, Out, {}, OldHeight / PoleHeight);
  UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamps: the motel lot's two lamps are cobra heads on 6.6 m poles"));
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
 return bRoad || bTown || bYard || bMotel;
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
}
