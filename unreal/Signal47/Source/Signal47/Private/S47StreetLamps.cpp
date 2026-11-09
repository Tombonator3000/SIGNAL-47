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
 const FVector ArmOnPole(0, 7.42, 0);   // the arm's root on a pole standing on its foot
 const FVector ArmEnd(1.80, .45, 0);    // the arm's end, where the head's slipfitter takes it, from the arm's root
 const FVector EmitAt(.40, -.13, 0);    // the bowl's middle from the head's pivot (the slipfitter)
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
 UStaticMesh* Cut(AS47Web* W, UStaticMeshComponent* C, const TArray<FBox>& Boxes, TArray<int32>& Counts)
 {
  UStaticMesh* Original = C ? C->GetStaticMesh() : nullptr;
  Counts.Init(0, Boxes.Num());
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
    if (Box >= 0) { ++Counts[Box]; continue; }
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
 bool Down(UWorld* World, const FVector& From, double Depth, const TArray<UPrimitiveComponent*>& Ignore, double& Y, FString* What = nullptr)
 {
  FCollisionQueryParams Q(SCENE_QUERY_STAT(S47StreetLampFoot), true);
  for (UPrimitiveComponent* P : Ignore) if (P) Q.AddIgnoredComponent(P);
  for (int32 Try = 0; Try < 10; ++Try)
  {
   FHitResult Hit;
   if (!World->LineTraceSingleByChannel(Hit, S47Three::ToUE(From), S47Three::ToUE(From - FVector(0, Depth, 0)), ECC_Visibility, Q)) return false;
   UPrimitiveComponent* P = Hit.GetComponent();
   if (P && P->IsVisible() && !P->bHiddenInGame)
   {
    Y = S47Three::ToThree(Hit.ImpactPoint).Y;
    if (What) *What = P->GetName();
    return true;
   }
   Q.AddIgnoredComponent(P);
  }
  return false;
 }
 UStaticMeshComponent* Place(AS47Web* W, USceneComponent* Parent, int32 ParentNode, const FString& Name, UStaticMesh* Mesh, const FVector& At, const FQuat& Q)
 {
  auto* C = NewObject<UStaticMeshComponent>(W, *FString::Printf(TEXT("S47StreetLamp_%s"), *Name.Replace(TEXT(":"), TEXT("_"))));
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(Parent); C->SetStaticMesh(Mesh);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCollisionResponseToAllChannels(ECR_Ignore);
  C->SetCastShadow(true); C->RegisterComponent();
  double Matrix[16]; S47Three::Compose(At, Q, FVector::OneVector, Matrix); C->SetWorldTransform(S47Three::MatToUE(Matrix));
  W->AdoptNode(C, ParentNode, TEXT("streetLamp:") + Name);
  return C;
 }
 bool Disabled() { return FParse::Param(FCommandLine::Get(), TEXT("S47StreetLampsOff")) || FParse::Param(FCommandLine::Get(), TEXT("S47PropsOff")); }

 // ---- the highway west of SARO: Exterior.ts road(), eight poles at x -18.5 with their arms west over the road, in g26 ----
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
   if (!Down(World, FVector(-18.5, 1.5, Z), 4, Ignore, Y, &What))
   { UE_LOG(LogS47StreetLamps, Warning, TEXT("S47: street lamps: nothing under the highway pole at z %.0f; the old lamps stay"), Z); return false; }
   Feet.Add(FVector(-18.5, Y, Z));
   UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamp road z %.0f stands on %s at y %.3f"), Z, *What, Y);
  }
  Without->InitResources(); Batch->SetStaticMesh(Without);
  for (UPrimitiveComponent* C : Old) { C->SetVisibility(false); C->SetHiddenInGame(true); C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
  const FQuat West(FVector(0, 1, 0), PI); // the arm's +x turned to -x, over the road
  USceneComponent* ParentComp = P->Comp.Get();
  for (int32 I = 0; I < Feet.Num(); ++I)
  {
   FLamp L; L.Light = FString::Printf(TEXT("Highway%02d"), I + 1); L.Foot = Feet[I];
   if (Concrete) L.Under.Add(Concrete);
   const FVector Root = Feet[I] + West.RotateVector(ArmOnPole), Slip = Root + West.RotateVector(ArmEnd);
   L.Pole = Place(W, ParentComp, Parent, FString::Printf(TEXT("road:%d:pole"), I), PoleMesh, Feet[I], West);
   L.Arm = Place(W, ParentComp, Parent, FString::Printf(TEXT("road:%d:arm"), I), ArmMesh, Root, West);
   UStaticMeshComponent* Head = Place(W, ParentComp, Parent, FString::Printf(TEXT("road:%d:head"), I), HeadMesh, Slip, West);
   L.Head = Head;
   L.Emitter = Slip + West.RotateVector(EmitAt); L.Normal = West.RotateVector(EmitNormal);
   const int32 Slot = HeadMesh->GetMaterialIndex(LensSlot);
   if (Slot >= 0 && Head->GetMaterial(Slot))
   {
    UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Head->GetMaterial(Slot), W);
    D->SetVectorParameterValue(TEXT("EmissiveFactor"), FLinearColor::Black); Head->SetMaterial(Slot, D); L.Lens = D;
   }
   Out.Add(L);
  }
  UE_LOG(LogS47StreetLamps, Display, TEXT("S47: street lamps: the highway's eight lamps are cobra heads on davit arms; %s old triangles cut"), *FString::JoinBy(Counts, TEXT(","), [](int32 N) { return FString::FromInt(N); }));
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
 return bRoad;
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
 int32 Bad = 0, Measured = 0;
 TArray<FString> Each;
 for (const FLamp& L : *Lamps)
 {
  if (!L.Pole.IsValid()) continue;
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
 Lines.Add(FString::Printf(TEXT("Street lamps: %d measured, %d stand on what is under them within 2 cm, %d wrong"), Measured, Measured - Bad, Bad));
 Lines.Append(Each);
 return Bad;
}
}
