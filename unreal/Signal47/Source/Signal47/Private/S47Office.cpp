#include "S47Office.h"
#include "S47Evening.h"
#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47SaroInterior.h"
#include "S47SaroDoorDetail.h"
#include "S47OfficeOwned.h"
#include "S47OfficeWindow.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/BoxComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "StaticMeshResources.h"
#include "RenderUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Office, Log, All);

namespace
{
 constexpr double FrontOffset = .50;
 constexpr double HingeX = -.03, HingeZ = 6.70 + FrontOffset, LeafWidth = 1.0;
 // Reserve the actual inward quarter-turn and the two leaf end positions for
 // the player's capsule. The doorway restore strip has a different purpose.
 bool InDoorSweep(const FVector& P, double Radius)
 {
  const double X = P.X - HingeX, Z = P.Z - HingeZ;
  const double Margin = Radius + .075; // leaf, handle and a small body clearance
  if (X >= 0 && Z >= 0 && X * X + Z * Z <= FMath::Square(LeafWidth + Margin)) return true;
  const double ClosedX = FMath::Clamp(X, 0., LeafWidth);
  const double OpenZ = FMath::Clamp(Z, 0., LeafWidth);
  return FMath::Square(X - ClosedX) + Z * Z <= Margin * Margin ||
   X * X + FMath::Square(Z - OpenZ) <= Margin * Margin;
 }
 struct FPortalPatch
 {
  UStaticMeshComponent* Comp = nullptr;
  UStaticMesh* Cut = nullptr;
  int32 Partition = 0, Skirting = 0, Door = 0;
  bool bHide = false;
 };
 bool InBox(const FVector& P, double X0, double X1, double Y0, double Y1, double Z0, double Z1)
 { return P.X >= X0 && P.X <= X1 && P.Y >= Y0 && P.Y <= Y1 && P.Z >= Z0 && P.Z <= Z1; }
 bool PartitionTriangle(const FVector& A, const FVector& B, const FVector& C)
 {
  auto In = [](const FVector& P) { return InBox(P, -3.406, 2.006, -.006, 2.806, 6.597, 6.806); };
  return In(A) && In(B) && In(C) && FMath::Max3(A.X, B.X, C.X) - FMath::Min3(A.X, B.X, C.X) > 5.3;
 }
 bool SkirtingTriangle(const FVector& A, const FVector& B, const FVector& C)
 {
  auto In = [](const FVector& P) { return InBox(P, -3.406, 2.006, -.006, .106, 6.569, 6.601); };
  return In(A) && In(B) && In(C) && FMath::Max3(A.X, B.X, C.X) - FMath::Min3(A.X, B.X, C.X) > 5.3;
 }
 // ART-01: finished paper artwork from ArtSource (Tools/documentprops/surfaces.json).
 // The art is a separate plane above each existing paper; the paper keeps its object
 // name, interaction ID and document mapping. Without the texture the old paper stays.
 struct FDocumentArt { const TCHAR* Object; const TCHAR* Asset; double Width, Depth; };
 const FDocumentArt DocumentArt[] = {
  {TEXT("office:fieldFolder"), TEXT("T_Doc_DaleFolder"), .24, .32},
  {TEXT("office:workingIndex"), TEXT("T_Doc_DaleIndex"), .225, .30},
  {TEXT("office:clippings"), TEXT("T_Doc_DaleClippings"), .255, .34},
  {TEXT("office:foldedNote"), TEXT("T_Doc_DaleNote"), .15, .20},
  {TEXT("office:contactCard"), TEXT("T_Doc_DaleContact"), .16, .12},
  {TEXT("office:deskForms"), TEXT("T_Doc_ServiceForm"), .20, .30}
 };
 struct FResearchArt
 {
  const TCHAR* Name; const TCHAR* Texture; FVector At;
  double Width, Height, Roll, U0, V0, U1, V1;
 };
 const FResearchArt ResearchArt[] = {
#include "S47OfficeResearchData.inl"
 };
 // UV rows (u' = a*u + b*v + c) for /Engine/BasicShapes/Plane. Its measured corners give
 // u = (x + 50) / 100 and v = (y + 50) / 100 in UE cm. A reader facing +UE Y has -X on the
 // right, so the image runs u' = 1 - u, and its top row (v' = 0) lies at the far +Y edge.
 const FLinearColor DocumentMapU(-1, 0, 1, 0), DocumentMapV(0, -1, 1, 0);
 UTexture2D* DocumentTexture(const TCHAR* Asset)
 { return LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Signal47/Art/Documents/%s.%s"), Asset, Asset)); }
 bool OldDoorPoint(const FVector& P) { return InBox(P, -.04, .94, -.01, 2.061, 6.499, 6.60); }
 // The immutable Annex export merges the closed Operations door with unrelated geometry.
 // Retire only its exact leaf/handle/sign, wide partition/painted faces and low skirting.
 // The unchanged end caps lie outside the new aperture. Keep every other
 // vertex, UV, normal, material slot and ray tracing resource, as in SaroArrival's sign patch.
 bool PreparePortal(AS47Web* W, UStaticMeshComponent* Comp, FPortalPatch& Out)
 {
  UStaticMesh* Original = Comp ? Comp->GetStaticMesh() : nullptr;
  if (!Original || !Original->bAllowCPUAccess || !Original->GetRenderData() || Original->GetRenderData()->LODResources.Num() != 1) return false;
  const FStaticMeshLODResources& Src = Original->GetRenderData()->LODResources[0];
  const FStaticMeshVertexBuffers& Buffers = Src.VertexBuffers;
  if (!Buffers.PositionVertexBuffer.GetVertexData() || !Buffers.StaticMeshVertexBuffer.GetTangentData() || !Buffers.StaticMeshVertexBuffer.GetTexCoordData() || Src.IndexBuffer.GetIndexDataSize() == 0) return false;
  TArray<uint32> Old, Keep; Src.IndexBuffer.GetCopy(Old); Keep.Reserve(Old.Num());
  TArray<FStaticMeshSection> Sections; int32 Partition = 0, Skirting = 0, Door = 0;
  auto Point = [&](uint32 Idx) { return S47Three::ToThree(Comp->GetComponentTransform().TransformPosition(FVector(Buffers.PositionVertexBuffer.VertexPosition(Idx)))); };
  for (const FStaticMeshSection& Section : Src.Sections)
  {
   FStaticMeshSection Copy = Section; Copy.FirstIndex = Keep.Num(); Copy.NumTriangles = 0;
   for (uint32 Index = Section.FirstIndex; Index < Section.FirstIndex + Section.NumTriangles * 3; Index += 3)
   {
    if (Index + 2 >= uint32(Old.Num())) return false;
    const uint32 Aidx = Old[Index], Bidx = Old[Index + 1], Cidx = Old[Index + 2];
    if (FMath::Max3(Aidx, Bidx, Cidx) >= Buffers.PositionVertexBuffer.GetNumVertices()) return false;
    const FVector A = Point(Aidx), B = Point(Bidx), C = Point(Cidx);
    if (SkirtingTriangle(A, B, C)) { ++Skirting; continue; }
    if (PartitionTriangle(A, B, C)) { ++Partition; continue; }
    if (OldDoorPoint(A) && OldDoorPoint(B) && OldDoorPoint(C)) { ++Door; continue; }
    Keep.Append({Aidx, Bidx, Cidx}); ++Copy.NumTriangles;
   }
   if (Copy.NumTriangles) Sections.Add(Copy);
  }
  if (!Partition && !Skirting && !Door) return false;
  Out.Comp = Comp; Out.Partition = Partition; Out.Skirting = Skirting; Out.Door = Door;
  if (Keep.IsEmpty()) { Out.bHide = true; return true; }
  UStaticMesh* Cut = NewObject<UStaticMesh>(W, NAME_None, RF_Transient);
  Cut->bAllowCPUAccess = true; Cut->NeverStream = true; Cut->GetStaticMaterials() = Original->GetStaticMaterials(); Cut->bSupportRayTracing = true;
  auto Data = MakeUnique<FStaticMeshRenderData>(); Data->AllocateLODResources(1);
  Data->Bounds = Original->GetRenderData()->Bounds; Data->ScreenSize[0].Default = 1;
  Data->NumInlinedLODs = 1; Data->CurrentFirstLODIdx = 0; Data->bReadyForStreaming = false;
  FStaticMeshLODResources& Dst = Data->LODResources[0]; Dst.Sections.Append(Sections);
  Dst.VertexBuffers.PositionVertexBuffer.Init(Buffers.PositionVertexBuffer, true);
  auto& Tangents = Dst.VertexBuffers.StaticMeshVertexBuffer;
  Tangents.SetUseHighPrecisionTangentBasis(Buffers.StaticMeshVertexBuffer.GetUseHighPrecisionTangentBasis());
  Tangents.SetUseFullPrecisionUVs(Buffers.StaticMeshVertexBuffer.GetUseFullPrecisionUVs());
  Tangents.Init(Buffers.StaticMeshVertexBuffer.GetNumVertices(), Buffers.StaticMeshVertexBuffer.GetNumTexCoords(), true);
  for (uint32 Vertex = 0; Vertex < Buffers.StaticMeshVertexBuffer.GetNumVertices(); ++Vertex)
  {
   Tangents.SetVertexTangents(Vertex, FVector3f(Buffers.StaticMeshVertexBuffer.VertexTangentX(Vertex)), Buffers.StaticMeshVertexBuffer.VertexTangentY(Vertex), FVector3f(Buffers.StaticMeshVertexBuffer.VertexTangentZ(Vertex)));
   for (uint32 UV = 0; UV < Buffers.StaticMeshVertexBuffer.GetNumTexCoords(); ++UV) Tangents.SetVertexUV(Vertex, UV, Buffers.StaticMeshVertexBuffer.GetVertexUV(Vertex, UV));
  }
  const uint32 Colors = Buffers.ColorVertexBuffer.GetNumVertices();
  if (Colors)
  {
   if (!Buffers.ColorVertexBuffer.GetVertexData()) return false;
   Dst.VertexBuffers.ColorVertexBuffer.Init(Colors, true);
   for (uint32 Vertex = 0; Vertex < Colors; ++Vertex) Dst.VertexBuffers.ColorVertexBuffer.VertexColor(Vertex) = Buffers.ColorVertexBuffer.VertexColor(Vertex);
  }
  Dst.bHasColorVertexData = Colors > 0; Dst.bBuffersInlined = true; Dst.bIsOptionalLOD = false;
  Dst.SourceMeshBounds = Data->Bounds; Dst.MaxDeviation = 0;
  // The window clips this transient portal mesh next. Retain its actual index
  // array after RHI upload in cooked builds, as well as the asset-level flag.
  Dst.IndexBuffer.TrySetAllowCPUAccess(true);
  Dst.IndexBuffer.SetIndices(Keep, EIndexBufferStride::Force32Bit);
  bool bInitRayTracing = Cut->bSupportRayTracing;
#if !WITH_EDITOR
  bInitRayTracing = bInitRayTracing && IsRayTracingAllowed();
#endif
  if (bInitRayTracing) Data->InitializeRayTracingRepresentationFromRenderingLODs();
  Cut->SetRenderData(MoveTemp(Data)); Cut->SetExtendedBounds(Original->GetBounds()); Out.Cut = Cut; return true;
 }
 FTransform Transform(const FVector& At, double Yaw = 0)
 {
  double Matrix[16]; S47Three::Compose(At, S47Three::Euler(0, Yaw, 0), FVector::OneVector, Matrix); return S47Three::MatToUE(Matrix);
 }
}

struct FS47Office::FVisual
{
 FS47Obj Root, Leaf;
 TMap<FString, FS47Obj> Objects;
 TArray<TPair<FS47Obj, FVector>> DrawerParts;
 TArray<FS47Obj> KeyParts;
 TWeakObjectPtr<URectLightComponent> Light, TaskLight, DeskBounce;
 bool bRecoverLegacyPose = false;
 bool bMigrationFailed = false;
 bool bOwnedPreparing = false, bOwnedFootprint = false;
 double OwnedRetryWait = 0;
 // ART-01 sheets: paper object name, its art plane and the bound texture.
 struct FSheet { FString Paper; FS47Obj PaperObj, Art; TWeakObjectPtr<UTexture2D> Texture; };
 TArray<FSheet> Sheets;
 struct FReference { FString Name; FS47Obj Surface; TWeakObjectPtr<UTexture2D> Texture; };
 TArray<FReference> References;
};

FS47Office::FS47Office(US47Main* InM) : Visual(MakeUnique<FVisual>()), M(InM) {}
FS47Office::~FS47Office() = default;

void FS47Office::Init()
{
 if (bBuilt || !M || !M->Web || !M->Inter || !M->Web->IsLoaded()) return;
 bBuilt = true;
 AS47Web* W = M->Web; const int32 Parent = W->Handle(TEXT("annex.group"));
 const FS47WebNode* ParentNode = W->Node(Parent); USceneComponent* ParentComp = ParentNode ? ParentNode->Comp.Get() : nullptr;
 UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
 UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 auto Authored = [](const TCHAR* Name) { return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Signal47/Art/Office/%s.%s"), Name, Name)); };
 // Unreal art revision: use the existing authored PBR furniture instead of
 // another layer of details over the simple procedural desk and chair.
 UStaticMesh* Desk = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Free/Office/retro_office_props/StaticMeshes/retro_office_props_PI_TankerDesk.retro_office_props_PI_TankerDesk"));
 const TCHAR* ChairPackage = TEXT("/Game/Licensed/DaleRealism/SM_Dale_Chair_H31");
 UStaticMesh* Chair = FPackageName::DoesPackageExist(ChairPackage)
  ? LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Licensed/DaleRealism/SM_Dale_Chair_H31.SM_Dale_Chair_H31"))
  : LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Free/Office/retro_office_props/StaticMeshes/retro_office_props_PI_OfficeChair.retro_office_props_PI_OfficeChair"));
 UStaticMesh* Room = Authored(TEXT("SM_Office_Room"));
 UStaticMesh* Dressing = Authored(TEXT("SM_Office_Dressing"));
 UStaticMesh* Door = Authored(TEXT("SM_Office_Door"));
 UStaticMesh* Cabinet = Authored(TEXT("SM_Office_ServiceCabinet"));
 UStaticMesh* Drawer = Authored(TEXT("SM_Office_ServiceDrawer"));
 UStaticMesh* Spare = Authored(TEXT("SM_Office_SpareKey"));
 UStaticMesh* Lamp = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Free/Office/retro_office_props/StaticMeshes/retro_office_props_PI_DeskLamp.retro_office_props_PI_DeskLamp"));
 UStaticMesh* TV = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Kelvora/SM_Kelvora_TV.SM_Kelvora_TV"));
 if (!ParentComp || !Cube || !Base || !Desk || !Chair || !Lamp || !TV || !Room || !Dressing || !Door || !Cabinet || !Drawer || !Spare)
 { UE_LOG(LogS47Office, Error, TEXT("S47: office unavailable: existing annex or owned office assets absent; original partition retained")); return; }
 TArray<FPortalPatch> Patches; int32 PartitionCount = 0, SkirtingCount = 0, DoorCount = 0; const int32 OriginalNodes = W->PartCount();
 auto Under = [W, Parent](int32 Node)
 {
  for (; Node >= 0;) { if (Node == Parent) return true; const FS47WebNode* N = W->Node(Node); if (!N) break; Node = N->Parent; } return false;
 };
 for (int32 Node = 0; Node < OriginalNodes; ++Node)
 {
  const FS47WebNode* N = W->Node(Node); if (!N || !Under(Node) || N->Geo == 402 || N->Geo < 0) continue;
  UStaticMeshComponent* Comp = Cast<UStaticMeshComponent>(N->Comp.Get()); if (!Comp || !Comp->GetStaticMesh()) continue;
  // Bound the scan before requiring CPU data from any unrelated imported furniture.
  const FBox Box = W->ObjAt(Node).WorldBox();
  if (!Box.IsValid || Box.Max.X < -3.41 || Box.Min.X > 2.01 || Box.Max.Z < 6.49 || Box.Min.Z > 6.81) continue;
  FPortalPatch Patch; if (PreparePortal(W, Comp, Patch)) { PartitionCount += Patch.Partition; SkirtingCount += Patch.Skirting; DoorCount += Patch.Door; Patches.Add(Patch); }
 }
 if (PartitionCount != 10 || SkirtingCount != 8 || DoorCount != 26)
 { UE_LOG(LogS47Office, Error, TEXT("S47: office exact portal rejected partition=%d/10 skirting=%d/8 oldDoor=%d/26; original retained"), PartitionCount, SkirtingCount, DoorCount); return; }
 auto Material = [W, Base](uint32 Color, float Roughness = .8f, float Metalness = 0.f)
 {
  UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, W);
  Mat->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(Color)); Mat->SetScalarParameterValue(TEXT("NativeLighting"), 1);
  Mat->SetScalarParameterValue(TEXT("Roughness"), Roughness); Mat->SetScalarParameterValue(TEXT("Metalness"), Metalness); Mat->SetScalarParameterValue(TEXT("EmissivePower"), 0); return Mat;
 };
 UMaterialInterface* Wall = Material(0xa8a48c), *Green = Material(0x718073), *Wood = Material(0x56432c), *Steel = Material(0x4d5553, .6, .45), *Paper = Material(0xd5c9a6), *Kraft = Material(0x9c895e), *Ink = Material(0x182620);
 UBoxComponent* Group = NewObject<UBoxComponent>(W, TEXT("OperationsRoomRoot")); Group->SetMobility(EComponentMobility::Movable);
 Group->SetupAttachment(ParentComp); Group->SetCollisionEnabled(ECollisionEnabled::NoCollision); Group->SetBoxExtent(FVector(1)); Group->RegisterComponent();
 Visual->Root = W->ObjAt(W->AdoptNode(Group, Parent, TEXT("operations:room")));
 const int32 Root = Visual->Root.I;
 auto Mesh = [&](const FString& Name, UStaticMesh* Asset, const FVector& At, const FVector& Scale, UMaterialInterface* Mat, double Yaw = 0, int32 Attach = -1)
 {
  const int32 Owner = Attach >= 0 ? Attach : Root; const FS47WebNode* N = W->Node(Owner);
  UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, *Name); C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(N->Comp.Get()); C->SetStaticMesh(Asset);
  if (Mat) C->SetMaterial(0, Mat); C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore); C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
  C->RegisterComponent(); C->SetWorldTransform(Transform(At, Yaw)); C->SetWorldScale3D(S47Three::DirToUE(Scale));
  FS47Obj Obj = W->ObjAt(W->AdoptNode(C, Owner, Name)); Obj.SetScale(Scale); Obj.SetRotation(FVector(0, Yaw, 0)); return Obj;
 };
 auto Box = [&](const FString& Name, const FVector& At, const FVector& Size, UMaterialInterface* Mat) { return Mesh(Name, Cube, At, Size, Mat); };
 auto Label = [&](const FString& Name, const FString& Text, const FVector& At, double Yaw, float Size = 4.f, int32 Attach = -1)
 {
  const int32 Owner = Attach >= 0 ? Attach : Root; const FS47WebNode* N = W->Node(Owner);
  UTextRenderComponent* C = NewObject<UTextRenderComponent>(W, *Name); C->SetupAttachment(N->Comp.Get()); C->SetMobility(EComponentMobility::Movable);
  C->SetText(FText::FromString(Text)); C->SetWorldSize(Size); C->SetHorizontalAlignment(EHTA_Center); C->SetVerticalAlignment(EVRTA_TextCenter); C->SetTextRenderColor(FColor(30, 38, 30)); C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->RegisterComponent(); C->SetWorldLocation(S47Three::ToUE(At)); C->SetWorldRotation(FRotator(0, Yaw, 0));
  return W->ObjAt(W->AdoptNode(C, Owner, Name));
 };
 auto Collider = [W](const TCHAR* Id, double X0, double X1, double Z0, double Z1)
 { W->CollidersMutable(TEXT("saro")).Add({X0, X1, Z0, Z1, FString(Id), true}); };
 // Printed enamel wayfinding, on the measured corridor_dressing.SIGNS faces.
 Label(TEXT("corridor:controlSign"), TEXT("CONTROL ROOM"), FVector(-2.4,2.405,4.854), 90,3.3f);
 Label(TEXT("corridor:recordsSign"), TEXT("RECORDS"), FVector(-3.9,2.405,6.548), -90,3.3f);
 Label(TEXT("corridor:fireSign"), TEXT("FIRE EXIT"), FVector(-5.956,2.55,5.7), 0,2.7f);
 Label(TEXT("corridor:restroomsSign"), TEXT("RESTROOMS"), FVector(1.924,2.405,5.7), 180,3.1f);
 Label(TEXT("corridor:operationsSign"), TEXT("OPERATIONS"), FVector(.46,2.43,7.052), -90,3.3f);
 // Keep the records partition and its entrance in place. Recess only Operations
 // by 0.5m to form a real passing bay without squeezing the archive table route.
 Box(TEXT("office:partitionRecordsWest"), FVector(-2.17, 1.4, 6.70), FVector(2.46, 2.8, .2), Wall);
 Box(TEXT("office:partitionRecords"), FVector(-2.17, 1.4, 6.803), FVector(2.46, 2.8, .006), Green);
 Box(TEXT("office:partitionLeft"), FVector(-.51, 1.4, 6.70 + FrontOffset), FVector(.86, 2.8, .2), Wall);
 Box(TEXT("office:partitionRight"), FVector(1.5, 1.4, 6.70 + FrontOffset), FVector(1, 2.8, .2), Wall);
 Box(TEXT("office:partitionHeader"), FVector(.46, 2.46, 6.70 + FrontOffset), FVector(1.08, .68, .2), Wall);
 Box(TEXT("office:partitionSkirtingRecords"), FVector(-2.17, .05, 6.585), FVector(2.46, .1, .02), Wood);
 Box(TEXT("office:partitionSkirtingLeft"), FVector(-.51, .05, 6.585 + FrontOffset), FVector(.86, .1, .02), Wood);
 Box(TEXT("office:partitionSkirtingRight"), FVector(1.5, .05, 6.585 + FrontOffset), FVector(1, .1, .02), Wood);
 // Original Blender joinery, floor tiles, ceiling grid, door frame and PBR surfaces.
 Mesh(TEXT("office:authoredRoom"), Room, FVector::ZeroVector, FVector::OneVector, nullptr);
 Mesh(TEXT("office:authoredDressing"), Dressing, FVector::ZeroVector, FVector::OneVector, nullptr);
 Collider(TEXT("office:recordsPartition"), -3.4, -.94, 6.6, 6.8);
 Collider(TEXT("office:leftPartition"), -.94, -.08, 6.6 + FrontOffset, 6.8 + FrontOffset); Collider(TEXT("office:rightPartition"), 1, 2, 6.6 + FrontOffset, 6.8 + FrontOffset);
 Collider(TEXT("office:westWall"), -.94, -.82, 6.6, 10.43); Collider(TEXT("office:eastWall"), 1.77, 1.89, 6.6, 10.43); Collider(TEXT("office:rearWall"), -.94, 1.89, 10.31, 10.43);
 // The board and its pins project in front of the east wall at head height.
 // Include that authored envelope in the walking capsule instead of allowing
 // the old narrow desk-side slit to clip through the new research wall.
 Collider(TEXT("office:researchWall"), 1.63, 1.78, 8.11, 10.21);
 W->ZonesMutable(TEXT("saro")).Add({-.82, 1.77, 6.8 + FrontOffset, 10.31, TEXT("office:roomZone"), true});
 W->ZonesMutable(TEXT("saro")).Add({-.82, 1.77, 5.70, 6.60 + FrontOffset, TEXT("office:passingBayZone"), true});
 // Walker shrinks each rectangle by the capsule radius independently. Overlap both
 // corridor and room by more than two radii, so an open doorway has no centreline gap.
 W->ZonesMutable(TEXT("saro")).Add({-.08, 1, 5.70, 7.50 + FrontOffset, TEXT("office:thresholdZone"), false});
 Collider(TEXT("office:doorCollider"), HingeX, HingeX + LeafWidth, HingeZ - .035, HingeZ + .035);
 UBoxComponent* Hinge = NewObject<UBoxComponent>(W, TEXT("OperationsDoorHinge")); Hinge->SetMobility(EComponentMobility::Movable); Hinge->SetupAttachment(Group); Hinge->SetCollisionEnabled(ECollisionEnabled::NoCollision); Hinge->SetBoxExtent(FVector(1)); Hinge->RegisterComponent(); Hinge->SetWorldLocation(S47Three::ToUE(FVector(HingeX, 0, HingeZ)));
 Visual->Leaf = W->ObjAt(W->AdoptNode(Hinge, Root, TEXT("office:doorHinge")));
 FS47Obj DoorPanel = Mesh(TEXT("office:doorLeaf"), Door, FVector(.47, 1.047, HingeZ), FVector(1, 1, .045 / .055), nullptr, 0, Visual->Leaf.I);
 // The Blender leaf includes real lock cylinder, lever, hinges and kick plates.
 Label(TEXT("office:doorTitle"), TEXT("DALE / OPERATIONS"), FVector(.47, 1.662, HingeZ - .039), -90, 3.3f, Visual->Leaf.I);
 Label(TEXT("office:doorTitleInside"), TEXT("OPERATIONS"), FVector(.47, 1.662, HingeZ + .039), 90, 3.3f, Visual->Leaf.I);
 Box(TEXT("office:spareNotice"), FVector(-.29, 1.45, 6.585 + FrontOffset), FVector(.30, .31, .018), Paper);
 Label(TEXT("office:spareText"), TEXT("SPARE IN\nSERVICE DRAWER"), FVector(-.29, 1.45, 6.572 + FrontOffset), -90, 2.8f);
 // Own service cabinet at the workbench. It is unrelated to the archive's REEL047 drawer.
 Mesh(TEXT("office:serviceCabinet"), Cabinet, FVector(-.59, .525, 6.32 + FrontOffset), FVector::OneVector, nullptr);
 Collider(TEXT("office:serviceCabinetFoot"), -.94, -.26, 6.045 + FrontOffset, 6.60 + FrontOffset);
 FS47Obj DrawerFront = Mesh(TEXT("office:serviceDrawer"), Drawer, FVector(-.59, .85, 6.22 + FrontOffset), FVector::OneVector, nullptr);
 Visual->DrawerParts.Add({DrawerFront, DrawerFront.Position()});
 FS47Obj KeyBody = Mesh(TEXT("office:taggedSpare"), Spare, FVector(-.59, .805, 6.17 + FrontOffset), FVector::OneVector, nullptr);
 Visual->DrawerParts.Add({KeyBody, KeyBody.Position()}); Visual->KeyParts.Add(KeyBody);
 // The real paper inset on the plastic tag is the usable face. The bounds centre
 // of the entire cut key falls inside its open split ring and is not a surface.
 FS47Obj KeyTag = Box(TEXT("office:keyTagPaper"), FVector(-.513, .8086, 6.215 + FrontOffset), FVector(.053, .001, .025), Paper);
 Visual->DrawerParts.Add({KeyTag, KeyTag.Position()}); Visual->KeyParts.Add(KeyTag);
 Label(TEXT("office:drawerLabel"), TEXT("SERVICE / SPARE"), FVector(-.59, 1.14, 6.565 + FrontOffset), -90, 3.f);
 // The native desk's measured top is .790m. A 10mm height offset retains the
 // existing .800m work surface and all six independent document/use anchors.
 Mesh(TEXT("office:desk"), Desk, FVector(.37, .01, 9.73), FVector::OneVector, nullptr, PI);
 const FS47Obj ChairObj = Mesh(TEXT("office:chair"), Chair, FVector(1.15, 0, 8.73), FVector::OneVector, nullptr, .37);
 Mesh(TEXT("office:deskLamp"), Lamp, FVector(-.24, .79, 9.70), FVector::OneVector, nullptr);
 Collider(TEXT("office:deskFoot"), -.47, 1.21, 9.31, 10.16);
 // Both chair candidates are authored at metre scale. Walking follows the
 // actual rotated native envelope, including armrests and the caster base.
 const FBox ChairBounds = ChairObj.WorldBox();
 if (ChairBounds.IsValid)
  Collider(TEXT("office:chairFoot"), ChairBounds.Min.X, ChairBounds.Max.X, ChairBounds.Min.Z, ChairBounds.Max.Z);
 // ART-01: a paper with finished art takes the art's proportions around its centre. The
 // larger sheets moved back onto the desktop (front edge Three z 9.31); the note now lies
 // beside the cup instead of over the index, so neither printed face is hidden.
 TMap<FString, UTexture2D*> ArtTextures; TMap<FString, FVector2D> ArtSizes; TMap<FString, FVector> PaperTops;
 for (const FDocumentArt& Art : DocumentArt) if (UTexture2D* Tex = DocumentTexture(Art.Asset)) { ArtTextures.Add(Art.Object, Tex); ArtSizes.Add(Art.Object, FVector2D(Art.Width, Art.Depth)); }
 auto PaperBox = [&](const TCHAR* Name, const FVector& At, const FVector& Size, UMaterialInterface* Mat, double YawDegrees)
 {
  const FVector2D* Art = ArtSizes.Find(Name); const FVector Used = Art ? FVector(Art->X, Size.Y, Art->Y) : Size;
  PaperTops.Add(Name, At + FVector(0, Used.Y * .5, 0));
  return Mesh(Name, Cube, At, Used, Mat, FMath::DegreesToRadians(YawDegrees));
 };
 FS47Obj DeskPaper = PaperBox(TEXT("office:deskForms"), FVector(.39, .8044, 9.50), FVector(.40, .0008, .21), Paper, 2.5);
 FS47Obj Folder = PaperBox(TEXT("office:fieldFolder"), FVector(.05, .8055, 9.51), FVector(.31, .003, .31), Kraft, -4);
 FS47Obj Index = PaperBox(TEXT("office:workingIndex"), FVector(-.28, .8044, 9.52), FVector(.23, .0008, .29), Paper, 6);
 FS47Obj Clippings = PaperBox(TEXT("office:clippings"), FVector(.79, .8046, 9.73), FVector(.29, .0012, .34), Paper, -3);
 FS47Obj Note = PaperBox(TEXT("office:foldedNote"), FVector(1.02, .8043, 9.47), FVector(.20, .0006, .13), Paper, 5);
 FS47Obj Contact = PaperBox(TEXT("office:contactCard"), FVector(.76, .8042, 9.42), FVector(.16, .0004, .10), Paper, -7);
 if (UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
 {
  const TMap<FString, FS47Obj> Papers = {{TEXT("office:deskForms"), DeskPaper}, {TEXT("office:fieldFolder"), Folder}, {TEXT("office:workingIndex"), Index},
   {TEXT("office:clippings"), Clippings}, {TEXT("office:foldedNote"), Note}, {TEXT("office:contactCard"), Contact}};
  for (const FDocumentArt& Art : DocumentArt)
  {
   UTexture2D* Tex = ArtTextures.FindRef(Art.Object); const FS47Obj* PaperObj = Papers.Find(Art.Object);
   if (!Tex || !PaperObj) continue;
   UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, W);
   Mat->SetVectorParameterValue(TEXT("Color"), FLinearColor(.92f, .92f, .92f)); Mat->SetScalarParameterValue(TEXT("NativeLighting"), 1);
   Mat->SetScalarParameterValue(TEXT("Roughness"), .88f); Mat->SetScalarParameterValue(TEXT("Metalness"), 0); Mat->SetScalarParameterValue(TEXT("EmissivePower"), 0);
   Mat->SetScalarParameterValue(TEXT("UseMap"), 1); Mat->SetScalarParameterValue(TEXT("MapIsLinear"), 0); Mat->SetTextureParameterValue(TEXT("Map"), Tex);
   // Engine plane UVs (measured at import): the image's top faces the far edge of the desk
   // (+Three Z) and reads left to right for a reader standing in front of it.
   Mat->SetVectorParameterValue(TEXT("MapU"), DocumentMapU); Mat->SetVectorParameterValue(TEXT("MapV"), DocumentMapV);
   const FVector Top = PaperTops.FindChecked(Art.Object) + FVector(0, .0008, 0);
   FS47Obj Sheet = Mesh(FString(Art.Object) + TEXT(":art"), Plane, Top, FVector(Art.Width, 1, Art.Depth), Mat);
   Sheet.SetRotation(PaperObj->Rotation());
   if (const FS47WebNode* N = W->Node(Sheet.I)) if (UPrimitiveComponent* C = Cast<UPrimitiveComponent>(N->Comp.Get())) { C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCastShadow(false); }
   Visual->Sheets.Add({Art.Object, *PaperObj, Sheet, Tex});
  }
  // The research wall is decorative. Reusing a document's artwork never grants
  // that document, read status or a story fact. The six live papers stay separate.
  for (const FResearchArt& Art : ResearchArt)
  {
   UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, Art.Texture); if (!Tex) continue;
   UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, W);
   Mat->SetVectorParameterValue(TEXT("Color"), FLinearColor(.90f, .90f, .90f)); Mat->SetScalarParameterValue(TEXT("NativeLighting"), 1);
   Mat->SetScalarParameterValue(TEXT("Roughness"), .90f); Mat->SetScalarParameterValue(TEXT("Metalness"), 0); Mat->SetScalarParameterValue(TEXT("EmissivePower"), 0);
   Mat->SetScalarParameterValue(TEXT("UseMap"), 1); Mat->SetScalarParameterValue(TEXT("MapIsLinear"), 0); Mat->SetTextureParameterValue(TEXT("Map"), Tex);
   // On the east wall, original engine-plane U points up and V points right.
   Mat->SetVectorParameterValue(TEXT("MapU"), FLinearColor(0, float(Art.U1 - Art.U0), float(Art.U0), 0));
   Mat->SetVectorParameterValue(TEXT("MapV"), FLinearColor(float(Art.V0 - Art.V1), 0, float(Art.V1), 0));
   FS47Obj Surface = Mesh(FString(Art.Name) + TEXT(":art"), Plane, Art.At, FVector(Art.Height, 1, Art.Width), Mat);
   Surface.SetRotation(FVector(-FMath::DegreesToRadians(Art.Roll), 0, PI * .5));
   if (const FS47WebNode* N = W->Node(Surface.I)) if (UPrimitiveComponent* C = Cast<UPrimitiveComponent>(N->Comp.Get())) { C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCastShadow(false); }
   Visual->References.Add({Art.Name, Surface, Tex});
  }
 }
 auto HasArt = [this](const TCHAR* Paper) { return Visual->Sheets.ContainsByPredicate([Paper](const FVisual::FSheet& S) { return S.Paper == Paper; }); };
 // NP1 private notes and the separate array appendix remain real, reachable papers.
 // Their slim shelf fits below the authored bookcase without restoring the old cube cabinet.
 Box(TEXT("office:privateShelf"), FVector(-.66, .645, 8.75), FVector(.28, .035, .84), Wood);
 for (double Z : {8.43, 9.07})
 {
  Box(FString::Printf(TEXT("office:privateBracketBack%.2f"), Z), FVector(-.795, .55, Z), FVector(.018, .20, .025), Steel);
  Box(FString::Printf(TEXT("office:privateBracketArm%.2f"), Z), FVector(-.69, .62, Z), FVector(.21, .022, .025), Steel);
 }
 FS47Obj PrivateFolder = Box(TEXT("office:privateFolder"), FVector(-.64, .676, 8.98), FVector(.25, .025, .30), Kraft);
 FS47Obj ArraySheet = Box(TEXT("office:arrayServiceSheet"), FVector(-.65, .676, 8.48), FVector(.21, .012, .25), Paper);
 Collider(TEXT("office:privateCabinetFoot"), -.80, -.52, 8.33, 9.17);
 // Printed lines and labels lie on real paper; flat text faces up in Unreal.
 auto PaperText = [&](const FString& Name, const FString& Text, const FVector& At, float Size)
 {
  FS47Obj Obj = Label(Name, Text, At, 0, Size); const FS47WebNode* N = W->Node(Obj.I);
  if (N && N->Comp.IsValid()) N->Comp->SetWorldRotation(FRotator(90, 0, 0)); return Obj;
 };
 // The plain printed labels remain only where no finished art is bound.
 if (!HasArt(TEXT("office:fieldFolder"))) PaperText(TEXT("office:folderPrint"), TEXT("FIELD CHECKS\nTAKE A COPY"), FVector(.05, .834, 9.51), 2.4f);
 if (!HasArt(TEXT("office:workingIndex"))) PaperText(TEXT("office:indexPrint"), TEXT("WORKING INDEX"), FVector(-.28, .819, 9.52), 1.9f);
 if (!HasArt(TEXT("office:foldedNote"))) PaperText(TEXT("office:notePrint"), TEXT("D. / WORKING NOTE"), FVector(1.02, .823, 9.47), 1.6f);
 if (!HasArt(TEXT("office:contactCard"))) PaperText(TEXT("office:contactPrint"), TEXT("DALE / SERVICE"), FVector(.76, .823, 9.42), 1.4f);
 PaperText(TEXT("office:privatePrint"), TEXT("PRIVATE / NOTES"), FVector(-.64, .692, 8.98), 1.3f);
 PaperText(TEXT("office:arrayServicePrint"), TEXT("ARRAY SERVICE\nROW 1 / A + B"), FVector(-.65, .687, 8.48), 1.2f);
 // Ordinary service boards on the existing gravel row, not encounter-result props.
 // The 0.27 m walking radius has clear access from gate/spine and either A/B order.
 for (const auto& Mark : TArray<TPair<FString, double>>{{TEXT("A / ROW 1 WEST"), -8.}, {TEXT("REF / R1"), -1.}, {TEXT("B / ROW 1 EAST"), 18.}})
 {
  const FString Name = FString(TEXT("office:arrayBoard:")) + Mark.Key;
  Box(Name + TEXT(":post"), FVector(Mark.Value, -.04, -75.3), FVector(.035, 1.16, .035), Steel);
  Box(Name, FVector(Mark.Value, .37, -75.3), FVector(.66, .23, .018), Paper);
  Label(Name + TEXT(":label"), Mark.Key, FVector(Mark.Value, .37, -75.286), 90, 3.6f);
 }
 FS47Obj TVObj = Mesh(TEXT("office:fieldDeck"), TV, FVector(-.45, .73, 7.75), FVector(.7), nullptr);
 if (const FS47WebNode* N = W->Node(TVObj.I)) if (UStaticMeshComponent* C = Cast<UStaticMeshComponent>(N->Comp.Get()))
 {
  // The unproduced tape has no substitute image, moving sequence or pilot glow.
  if (C->GetNumMaterials() > 1) C->SetMaterial(1, Ink);
  if (C->GetNumMaterials() > 2) C->SetMaterial(2, Ink);
 }
 Collider(TEXT("office:deckFoot"), -.77, -.17, 7.43, 8.07);
 FS47Obj Deck = Box(TEXT("office:cassette"), FVector(-.43, .72, 8.08), FVector(.20, .025, .12), Ink);
 PaperText(TEXT("office:tapePrint"), TEXT("FIELD 01"), FVector(-.43, .739, 8.08), 1.8f);
 // Native fixture dimensions follow build_office.py. The emitter sits below the
 // two opal diffusers, faces down and retains the existing power/area driver.
 URectLightComponent* Light = NewObject<URectLightComponent>(W, TEXT("OperationsCeilingLight")); Light->SetMobility(EComponentMobility::Movable); Light->SetupAttachment(Group);
 Light->SetWorldLocation(S47Three::ToUE(FVector(.47, 2.594, 8.8))); Light->SetWorldRotation(FRotator(-90, 0, 0));
 Light->SetSourceWidth(115); Light->SetSourceHeight(32); Light->SetIntensityUnits(ELightUnits::Lumens); Light->SetIntensity(90); Light->SetLightColor(FLinearColor(1, .88f, .72f)); Light->SetAttenuationRadius(380); Light->SetCastShadows(true); Light->RegisterComponent(); Visual->Light = Light;
 // The native banker's lamp has a wide shade, 28cm above its foot. Keep its
 // small warm pool under the shade rather than illuminating the entire room.
 URectLightComponent* Task = NewObject<URectLightComponent>(W, TEXT("OperationsDeskLight")); Task->SetMobility(EComponentMobility::Movable); Task->SetupAttachment(Group);
 Task->SetWorldLocation(S47Three::ToUE(FVector(-.24, 1.030, 9.59))); Task->SetWorldRotation(FRotator(-90, 0, 0));
 Task->SetSourceWidth(55); Task->SetSourceHeight(4); Task->SetIntensityUnits(ELightUnits::Lumens); Task->SetIntensity(2);
 Task->SetLightColor(FLinearColor(1, .69f, .39f)); Task->SetAttenuationRadius(100); Task->SetCastShadows(true); Task->RegisterComponent(); Visual->TaskLight = Task;
 // Low has no GI. Approximate the first ceiling-fixture bounce off this real
 // desktop with 7% of its flux. It faces up, retains wall occlusion and is
 // disabled whenever Lumen supplies that reflection itself.
 URectLightComponent* Bounce = NewObject<URectLightComponent>(W, TEXT("OperationsDesktopBounce")); Bounce->SetMobility(EComponentMobility::Movable); Bounce->SetupAttachment(Group);
 Bounce->SetWorldLocation(S47Three::ToUE(FVector(.37, .84, 9.73))); Bounce->SetWorldRotation(FRotator(90, 0, 0));
 Bounce->SetSourceWidth(150); Bounce->SetSourceHeight(70); Bounce->SetIntensityUnits(ELightUnits::Lumens); Bounce->SetIntensity(6.3);
 Bounce->SetLightColor(FLinearColor(.82f, .76f, .61f)); Bounce->SetAttenuationRadius(260); Bounce->SetCastShadows(true); Bounce->RegisterComponent(); Visual->DeskBounce = Bounce;
 FS47Obj Depart = Box(TEXT("office:departureCard"), FVector(1.39, 1.45, 6.815 + FrontOffset), FVector(.55, .30, .015), Paper);
 Label(TEXT("office:departureText"), TEXT("SURVEY FILE\nSTATION 01"), FVector(1.39, 1.45, 6.829 + FrontOffset), 90, 3.8f);
 Visual->Objects.Add(TEXT("office:door"), DoorPanel); Visual->Objects.Add(TEXT("officeDoor"), DoorPanel);
 Visual->Objects.Add(TEXT("office:drawer"), DrawerFront); Visual->Objects.Add(TEXT("office:key"), KeyTag);
 Visual->Objects.Add(TEXT("office:desk"), DeskPaper); Visual->Objects.Add(TEXT("office:folder"), Folder); Visual->Objects.Add(TEXT("office:index"), Index);
 Visual->Objects.Add(TEXT("office:private"), PrivateFolder); Visual->Objects.Add(TEXT("office:arrayService"), ArraySheet);
 Visual->Objects.Add(TEXT("office:clippings"), Clippings); Visual->Objects.Add(TEXT("office:note"), Note); Visual->Objects.Add(TEXT("office:contact"), Contact);
 Visual->Objects.Add(TEXT("office:tape"), Deck); Visual->Objects.Add(TEXT("office:depart"), Depart);
 auto Bind = [&](const FString& Id)
 {
  FS47Interactable It; It.Id = Id; It.Object = Visual->Objects.FindChecked(Id); It.Range = 2.2;
  if (const FS47WebNode* N = W->Node(It.Object.I)) if (UPrimitiveComponent* C = Cast<UPrimitiveComponent>(N->Comp.Get())) C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
  It.Label = [this, Id]() -> FString
  {
   if (!Here() || !AccessAvailable()) return {};
   if (Id == TEXT("office:door") || Id == TEXT("officeDoor")) return !bKey ? TEXT("Operations office / locked") : !bUnlocked ? TEXT("Use the Operations spare") : bDoorTarget ? TEXT("Close Operations door") : TEXT("Open Operations door");
   if (Id == TEXT("office:drawer")) return !bDrawerTarget ? TEXT("Open service drawer") : !bKey ? TEXT("Take Operations spare key") : TEXT("Close service drawer");
   if (Id == TEXT("office:key")) return !bKey && DrawerK >= .995 ? TEXT("Take Operations spare key") : FString();
   if (!Contains(M->Player->Pos) || !WasUnlocked()) return {};
   if (Id == TEXT("office:desk")) return TEXT("Examine the working desk");
   if (Id == TEXT("office:folder")) return bFieldSheet ? TEXT("Read retained field sheet") : TEXT("Take the field sheet");
   if (Id == TEXT("office:index")) return TEXT("Read the working index");
   if (Id == TEXT("office:clippings")) return TEXT("Examine the local cuttings");
   if (Id == TEXT("office:note")) return TEXT("Read the folded note");
   if (Id == TEXT("office:contact")) return TEXT("Copy the field contact");
   if (Id == TEXT("office:private")) return TEXT("Browse Dale's private notes");
   if (Id == TEXT("office:arrayService")) return TEXT("Read Row One array service sheet");
   if (Id == TEXT("office:tape")) return TEXT("Inspect Service Road / Field 01");
   if (Id == TEXT("office:depart")) return WantsDeparture() ? TEXT("Leave for STATION 01") : FString();
   return {};
  };
  It.Use = [this, Id] { Use(Id); }; It.Examine = [this, Id]
  {
   if (Id == TEXT("office:door") || Id == TEXT("officeDoor")) return FString(TEXT("OPERATIONS. The spare is in the service drawer at the workbench. The office uses its own tagged key."));
   if (Id == TEXT("office:tape")) return FString(TEXT("SERVICE ROAD / FIELD 01. The deck is present, but this tape cannot be played. The paper beside the maintenance forms gives the camera position."));
   return FString(TEXT("An ordinary Operations working copy. Survey-station records keep their separate file."));
  }; M->Inter->Add(It);
 };
 // Replace the old inert officeDoor interaction, then give the physical leaf its new ID.
 Bind(TEXT("officeDoor"));
 for (const auto& Entry : Visual->Objects) if (Entry.Key != TEXT("officeDoor")) Bind(Entry.Key);
 // All guards and all replacement geometry are prepared before touching the old source.
 for (const FPortalPatch& Patch : Patches)
 {
  if (Patch.bHide)
  {
   for (int32 Node = 0; Node < OriginalNodes; ++Node) if (const FS47WebNode* N = W->Node(Node)) if (N->Comp.Get() == Patch.Comp) W->ObjAt(Node).SetVisible(false);
   Patch.Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  }
  else
  {
   const TArray<UMaterialInterface*> Materials = Patch.Comp->GetMaterials(); Patch.Cut->InitResources(); Patch.Comp->SetStaticMesh(Patch.Cut);
   for (int32 Slot = 0; Slot < Materials.Num(); ++Slot) if (Materials[Slot]) Patch.Comp->SetMaterial(Slot, Materials[Slot]);
  }
 }
 FS47Obj OldProxy = W->Obj(TEXT("annex.objs.officeDoor")); if (OldProxy) { OldProxy.SetVisible(false); if (const FS47WebNode* N = W->Node(OldProxy.I)) if (UPrimitiveComponent* C = Cast<UPrimitiveComponent>(N->Comp.Get())) C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
 bReady = true; S47SaroInterior::Build(W); S47SaroDoorDetail::Build(W);
 if (!S47OfficeWindow::Build(W))
  UE_LOG(LogS47Office, Warning, TEXT("S47: Operations rear window unavailable; native Room/Dressing and original CPU-retention update required"));
 const bool bOwnedReady = S47OfficeOwned::Build(W, &Visual->bOwnedPreparing);
 if (bOwnedReady || Visual->bOwnedPreparing)
 {
  // Capsule envelope includes the propped lid, which overhangs the body by 34mm.
  // Reserve it before Flow restores an old pose, including during texture warmup.
  Collider(TEXT("office:ownedOpenBox"), -.817, -.391, 8.537, 8.900);
  Visual->bOwnedFootprint = true;
 }
 Apply();
 UE_LOG(LogS47Office, Display, TEXT("S47: Operations ready physical portal=10 skirting=8 leaf/handle/sign=26; own spare/drawer; eight authored Blender/PBR assemblies; early physical access; field sheet independent of unavailable tape"));
}

void FS47Office::Begin(const S47J::FObj& Saved, bool bNewNight)
{
 Visual->bRecoverLegacyPose = Saved.IsValid() && S47J::Num(Saved, TEXT("interiorLayout"), 1) < 2;
 Visual->bMigrationFailed = false;
 bRequired = bNewNight; bKey = bUnlocked = bEverOpened = bEntered = false;
 bDoorInspected = bDoorTarget = bDrawerTarget = false;
 bDeskSeen = bIndexRead = bClippingsRead = bNoteRead = bContactRead = bFieldSheet = bDeparted = false;
 ReadDocuments.Reset();
 DoorK = DrawerK = FieldSheetAt = SyncWait = 0;
 double Version = 0;
 if (Saved.IsValid() && Saved->HasTypedField<EJson::Number>(TEXT("v")) && Saved->TryGetNumberField(TEXT("v"), Version) && Version == 1.)
 {
  // Only explicitly versioned new-night cases acquire this departure requirement.
  // Missing/unknown route versions preserve optional access to the room in old cases.
  double RouteVersion = 0;
  bRequired = Saved->HasTypedField<EJson::Number>(TEXT("routeVersion")) && Saved->TryGetNumberField(TEXT("routeVersion"), RouteVersion) && RouteVersion == 1. && S47J::Bool(Saved, TEXT("routeRequired"));
  bKey = S47J::Bool(Saved, TEXT("key")); bUnlocked = bKey && S47J::Bool(Saved, TEXT("unlocked"));
  bEverOpened = bUnlocked && S47J::Bool(Saved, TEXT("everOpened")); bEntered = bEverOpened && S47J::Bool(Saved, TEXT("entered"));
  bDoorInspected = S47J::Bool(Saved, TEXT("doorInspected")); bDoorTarget = bUnlocked && S47J::Bool(Saved, TEXT("doorOpen"));
  bDrawerTarget = S47J::Bool(Saved, TEXT("drawerOpen"));
  bDeskSeen = bEntered && S47J::Bool(Saved, TEXT("deskSeen")); bIndexRead = bEntered && S47J::Bool(Saved, TEXT("indexRead"));
  bClippingsRead = bEntered && S47J::Bool(Saved, TEXT("clippingsRead")); bNoteRead = bEntered && S47J::Bool(Saved, TEXT("noteRead")); bContactRead = bEntered && S47J::Bool(Saved, TEXT("contactRead"));
  const bool bHasAcquisitionTime = Saved->HasTypedField<EJson::Number>(TEXT("fieldSheetAt")) && Saved->TryGetNumberField(TEXT("fieldSheetAt"), FieldSheetAt) && FMath::IsFinite(FieldSheetAt);
  bFieldSheet = bEntered && S47J::Bool(Saved, TEXT("fieldSheet")) && bHasAcquisitionTime;
  bDeparted = bFieldSheet && S47J::Bool(Saved, TEXT("departed"));
  // Legacy office saves omit this optional list and therefore gain no read credit.
  // Reject the whole list when one entry is not a registered string; never coerce bools.
  const TArray<TSharedPtr<FJsonValue>>* Read = nullptr;
  if (bEntered && Saved->TryGetArrayField(TEXT("readDocuments"), Read))
  {
   bool Valid = true;
   for (const auto& Value : *Read)
   {
    FString Id;
    if (!Value.IsValid() || Value->Type != EJson::String || !Value->TryGetString(Id) || !LibraryDocumentIds().Contains(Id)) { Valid = false; break; }
    ReadDocuments.AddUnique(Id);
   }
   if (!Valid) ReadDocuments.Reset();
  }
 }
 Init();
 // Rebuild the saved opening before the host validates the restored walking pose.
 // A saved mid-open door resumes fully open; it never traps a valid room occupant.
 DoorK = bDoorTarget ? 1 : 0; DrawerK = bDrawerTarget ? 1 : 0;
 if (bDoorTarget) bEverOpened = true;
 Apply();
 RefileReadDocuments();
}

bool FS47Office::HasFieldSheet() const
{ return bReady && bKey && bUnlocked && bEverOpened && bEntered && bFieldSheet && FMath::IsFinite(FieldSheetAt); }

bool FS47Office::Contains(const FVector& P) const
{ return bReady && InBox(P, -.82, 1.77, -2, 4, 6.80 + FrontOffset, 10.31); }

bool FS47Office::InThreshold(const FVector& P, double Radius) const
{ return bReady && P.X >= -.08 - Radius && P.X <= 1 + Radius && P.Z >= 6.05 - Radius && P.Z <= 7.05 + FrontOffset + Radius; }

bool FS47Office::RestoreDoorwayAt(const FVector& P, double Radius)
{
 if (!WasUnlocked() || !InThreshold(P, Radius) || DoorOpen()) return false;
 // The host supplies the incoming saved pose before placing the player. Reopen only
 // this already unlocked doorway; retain entrance, paper, acquisition and departure facts.
 bDoorTarget = true; DoorK = 1; Apply();
 UE_LOG(LogS47Office, Display, TEXT("S47: restored unlocked office doorway at %.3f %.3f; saved pose retained"), P.X, P.Z);
 return true;
}

bool FS47Office::AccessAvailable() const
{
 // The tagged spare is a physical key at SARO, not an archive-chapter reward.
 // Permit the same real drawer/key/door/paper route during arrival and the evening;
 // STATION 01 departure remains guarded independently by WantsDeparture().
 return M && M->Story && M->Mode == US47Main::EMode::Play;
}

bool FS47Office::Here() const
{
 return bReady && M && M->World && M->Player && M->Story && M->UI && M->Mode == US47Main::EMode::Play &&
  M->World->Area == TEXT("saro") && !M->World->bDriving && !M->World->bBusy && !M->ModalOpen() && !M->Story->IsCinematic();
}

FS47Obj FS47Office::Object(const FString& Id) const
{ const FS47Obj* Obj = Visual ? Visual->Objects.Find(Id) : nullptr; return Obj ? *Obj : FS47Obj(); }

bool FS47Office::InReach(const FString& Id) const
{
 const FS47Obj Obj = Object(Id); if (!Obj || !M || !M->Player) return false;
 const FVector At = Obj.WorldPosition(); return FVector2D(M->Player->Pos.X - At.X, M->Player->Pos.Z - At.Z).Size() <= 2.2;
}

bool FS47Office::WantsDeparture() const
{ return !bDeparted && HasFieldSheet() && CanDepart && CanDepart(); }

void FS47Office::RefileReadDocuments()
{
 if (!bReady || !bEntered || !WasUnlocked() || !M || !M->Story || !M->Evening) return;
 // Opening saves have no chapter/routine document store. Rebuild only paper that
 // this office controller actually read or copied; do not advance D02 or open UI.
 TArray<TPair<bool, FString>> Read = {
  {bDeskSeen || bIndexRead || bFieldSheet, TEXT("EV_DALE_INDEX")},
  {bClippingsRead, TEXT("EV_DALE_CLIPPINGS")},
  {bNoteRead, TEXT("EV_DALE_NOTE")},
  {bContactRead, TEXT("EV_DALE_CONTACT")}
 };
 for (const FString& Id : ReadDocuments) if (DocumentRead(Id)) Read.Add({true, Id});
 for (const auto& Paper : Read)
 {
  if (!Paper.Key || M->Story->Docs().ContainsByPredicate([&](const FS47Doc& D) { return D.Id == Paper.Value; })) continue;
  if (const FS47EveningDocument* Source = M->Evening->Pack.FindDocument(Paper.Value))
  {
   FS47Doc Copy; Copy.Id = Source->Id; Copy.Title = Source->Title; Copy.Kind = Source->Kind; Copy.Stamp = Source->Stamp;
   Copy.Page = FString::Join(Source->Body, TEXT("\n\n")); Copy.Transcript = Copy.Page; M->Story->AddDoc(Copy);
  }
 }
}

bool FS47Office::OpenDocument(const FString& Id)
{
 if (!M || !M->Story || !M->UI) return false;
 const FS47Doc* Doc = M->Story->Docs().FindByPredicate([&](const FS47Doc& D) { return D.Id == Id; });
 const bool bLibrary = LibraryDocumentIds().Contains(Id);
 // New private pages always show their registered text, even when an older saved
 // notebook copy is blank/corrupt. The original seven papers retain historical copies.
 if (bLibrary && (!M->Evening || !M->Evening->Pack.FindDocument(Id))) return false;
 // Physical paper retrieval remains useful if an authored scene is currently occupied.
 // Filing this actual paper is independent of granting Pack scene/route facts.
 if ((!Doc || bLibrary) && M->Evening)
 {
  if (const FS47EveningDocument* Source = M->Evening->Pack.FindDocument(Id))
  {
   FS47Doc Copy; Copy.Id = Source->Id; Copy.Title = Source->Title; Copy.Kind = Source->Kind; Copy.Stamp = Source->Stamp;
   Copy.Page = FString::Join(Source->Body, TEXT("\n\n")); Copy.Transcript = Copy.Page; M->Story->AddDoc(Copy);
   Doc = M->Story->Docs().FindByPredicate([&](const FS47Doc& D) { return D.Id == Id; });
  }
 }
 if (Doc) { M->UI->Document(*Doc); return true; }
 M->UI->Toast(TEXT("The working sheet remains on the desk. The document source is unavailable."), 4);
 return false;
}


const TArray<FString>& FS47Office::LibraryDocumentIds()
{
 static const TArray<FString> Ids = {TEXT("EV_DALE_SKYLOG"), TEXT("EV_DALE_WITNESS"), TEXT("EV_DALE_EXPLANATIONS"),
  TEXT("EV_DALE_FRAME18"), TEXT("EV_DALE_TAPELOAN"), TEXT("EV_DALE_UNSENT"), TEXT("EV_DALE_ARRAY_SERVICE")};
 return Ids;
}

bool FS47Office::DocumentRead(const FString& Id) const
{ return bEntered && WasUnlocked() && LibraryDocumentIds().Contains(Id) && ReadDocuments.Contains(Id); }

void FS47Office::OpenLibrary()
{
 if (!M || !M->UI || !M->Evening) return;
 TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Text(TEXT("Personal copies. Choose a page to read; you can leave with the field sheet at any time.")));
 for (const FString& Id : LibraryDocumentIds())
 {
  if (Id == TEXT("EV_DALE_ARRAY_SERVICE")) continue;
  const auto* Source = M->Evening->Pack.FindDocument(Id); if (!Source) continue;
  Rows.Add(S47Kit::Card(Source->Title, DocumentRead(Id) ? TEXT("Read / retained in notebook") : TEXT("Unread"), [this, Id]
  {
   if (!M || !M->UI) return;
   M->UI->Close(); ReadLibraryDocument(Id);
  }));
 }
 M->UI->Open(S47Kit::Shell(M->UI, TEXT("DALE / PRIVATE NOTES"), S47Kit::Col(Rows), 920.f));
}

bool FS47Office::ReadLibraryDocument(const FString& Id)
{
 const FString ObjectId = Id == TEXT("EV_DALE_ARRAY_SERVICE") ? TEXT("office:arrayService") : TEXT("office:private");
 if (!LibraryDocumentIds().Contains(Id) || !Here() || !AccessAvailable() || !Contains(M->Player->Pos) ||
  !WasUnlocked() || !bEverOpened || !bEntered || !InReach(ObjectId)) return false;
 // Only opening the actual document gives read credit. Browsing, missing sources,
 // unavailable film and uncreated photographs cannot acknowledge this transaction.
 if (!OpenDocument(Id)) return false;
 ReadDocuments.AddUnique(Id); Changed(); return true;
}

void FS47Office::SyncFacts()
{
 if (!bReady || !M || !M->Evening || (M->Flow && M->Flow->bStarting)) return;
 auto Commit = [this](bool Fact, const TCHAR* Scene) { if (Fact) M->Evening->CommitWorldScene(Scene); };
 Commit(bDoorInspected, TEXT("D02_LOCKED_OFFICE")); Commit(bKey, TEXT("D02_SERVICE_DRAWER"));
 Commit(bEverOpened && WasUnlocked(), TEXT("D02_OPEN_OFFICE")); Commit(bDeskSeen, TEXT("D02_DESK_FIRST_LOOK"));
 Commit(bIndexRead, TEXT("D02_READ_FIELD_INDEX")); Commit(bClippingsRead, TEXT("D02_READ_CLIPPINGS"));
 Commit(bNoteRead, TEXT("D02_READ_DALE_NOTE")); Commit(bContactRead, TEXT("D02_SAVE_CONTACT"));
 Commit(HasFieldSheet(), TEXT("D02_TAKE_FIELD_FOLDER"));
 for (const FString& Id : ReadDocuments) if (DocumentRead(Id))
  M->Evening->CommitWorldScene(FString(TEXT("D02_READ_")) + Id.Mid(8));
 // There is no produced 72-second tape. Neither tape-start nor vhs_seen is acknowledged.
}

void FS47Office::Changed() { Apply(); SyncFacts(); if (M) M->RequestAutosave(); }

bool FS47Office::AuditDocumentArt(TArray<FString>& Lines) const
{
 bool bOk = bReady && Visual && Visual->Sheets.Num() == UE_ARRAY_COUNT(DocumentArt);
 Lines.Add(FString::Printf(TEXT("document art %s: %d/%d sheets bound"), bOk ? TEXT("PASS") : TEXT("FAIL"), Visual ? Visual->Sheets.Num() : 0, int32(UE_ARRAY_COUNT(DocumentArt))));
 if (!Visual) return false;
 bOk &= S47OfficeWindow::Audit(M->Web, Lines);
 const bool bReferences = Visual->References.Num() == UE_ARRAY_COUNT(ResearchArt); bOk &= bReferences;
 Lines.Add(FString::Printf(TEXT("research art %s: %d/%d decorative references; live document acquisition separate"), bReferences ? TEXT("PASS") : TEXT("FAIL"), Visual->References.Num(), int32(UE_ARRAY_COUNT(ResearchArt))));
 for (int32 I = 0; I < Visual->References.Num(); ++I)
 {
  const FVisual::FReference& Ref = Visual->References[I]; const FResearchArt& Art = ResearchArt[I];
  const FS47WebNode* Node = M->Web->Node(Ref.Surface.I);
  UPrimitiveComponent* Comp = Node ? Cast<UPrimitiveComponent>(Node->Comp.Get()) : nullptr;
  UMaterialInstanceDynamic* Mat = Comp ? Cast<UMaterialInstanceDynamic>(Comp->GetMaterial(0)) : nullptr;
  UTexture* Actual = nullptr; FLinearColor U, V; float UseMap = 0;
  UTexture2D* Tex = Ref.Texture.Get(); const double Angle = FMath::DegreesToRadians(Art.Roll);
  const FVector Up = Comp ? S47Three::ToThree(Comp->GetComponentTransform().GetUnitAxis(EAxis::X) * 100) : FVector::ZeroVector;
  const FVector Right = Comp ? S47Three::ToThree(Comp->GetComponentTransform().GetUnitAxis(EAxis::Y) * 100) : FVector::ZeroVector;
  const bool bMaterial = Mat && Mat->GetTextureParameterValue(FMaterialParameterInfo(TEXT("Map")), Actual) && Actual == Tex &&
   Mat->GetVectorParameterValue(FMaterialParameterInfo(TEXT("MapU")), U) && U.Equals(FLinearColor(0, float(Art.U1 - Art.U0), float(Art.U0), 0), .00001f) &&
   Mat->GetVectorParameterValue(FMaterialParameterInfo(TEXT("MapV")), V) && V.Equals(FLinearColor(float(Art.V0 - Art.V1), 0, float(Art.V1), 0), .00001f) &&
   Mat->GetScalarParameterValue(FMaterialParameterInfo(TEXT("UseMap")), UseMap) && FMath::IsNearlyEqual(UseMap, 1.f);
  const bool bRef = Tex && Tex->GetSizeX() >= 512 && Tex->GetSizeY() >= 512 && Tex->GetPathName() == Art.Texture &&
   Ref.Surface.WorldPosition().Equals(Art.At, .0001) && Up.Equals(FVector(0, FMath::Cos(Angle), -FMath::Sin(Angle)), .0001) &&
   Right.Equals(FVector(0, FMath::Sin(Angle), FMath::Cos(Angle)), .0001) && bMaterial && Comp->GetCollisionEnabled() == ECollisionEnabled::NoCollision;
  bOk &= bRef;
  Lines.Add(FString::Printf(TEXT("research actual binding %s %s: texture=%s %dx%d up=%s right=%s non-interactive plane"),
   bRef ? TEXT("PASS") : TEXT("FAIL"), *Ref.Name, *GetPathNameSafe(Actual), Tex ? Tex->GetSizeX() : 0, Tex ? Tex->GetSizeY() : 0, *Up.ToString(), *Right.ToString()));
 }
 for (const TCHAR* Id : {TEXT("office:desk"), TEXT("office:folder"), TEXT("office:index"), TEXT("office:clippings"), TEXT("office:note"), TEXT("office:contact")})
 { const bool bHave = Visual->Objects.Contains(Id) && Visual->Objects[Id].IsValid(); bOk &= bHave; if (!bHave) Lines.Add(FString(TEXT("document art FAIL: interaction paper missing ")) + Id); }
 for (int32 SheetIndex = 0; SheetIndex < Visual->Sheets.Num(); ++SheetIndex)
 {
  const FVisual::FSheet& Sheet = Visual->Sheets[SheetIndex];
  const FDocumentArt* Expected = SheetIndex < UE_ARRAY_COUNT(DocumentArt) ? &DocumentArt[SheetIndex] : nullptr;
  const UTexture2D* Tex = Sheet.Texture.Get();
  const FS47WebNode* ArtNode = M && M->Web ? M->Web->Node(Sheet.Art.I) : nullptr;
  const UStaticMeshComponent* ArtComp = ArtNode ? Cast<UStaticMeshComponent>(ArtNode->Comp.Get()) : nullptr;
  const UMaterialInstanceDynamic* ActualMaterial = ArtComp ? Cast<UMaterialInstanceDynamic>(ArtComp->GetMaterial(0)) : nullptr;
  UTexture* ActualTexture = nullptr;
  float UseMap = -1, MapIsLinear = -1, NativeLighting = -1, Roughness = -1, Metalness = -1, EmissivePower = -1;
  FLinearColor MapU = FLinearColor::Transparent, MapV = FLinearColor::Transparent;
  bool bParameters = ActualMaterial != nullptr;
  if (ActualMaterial)
  {
   bParameters &= ActualMaterial->GetTextureParameterValue(FMaterialParameterInfo(TEXT("Map")), ActualTexture);
   bParameters &= ActualMaterial->GetScalarParameterValue(FMaterialParameterInfo(TEXT("UseMap")), UseMap);
   bParameters &= ActualMaterial->GetScalarParameterValue(FMaterialParameterInfo(TEXT("MapIsLinear")), MapIsLinear);
   bParameters &= ActualMaterial->GetScalarParameterValue(FMaterialParameterInfo(TEXT("NativeLighting")), NativeLighting);
   bParameters &= ActualMaterial->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Roughness")), Roughness);
   bParameters &= ActualMaterial->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Metalness")), Metalness);
   bParameters &= ActualMaterial->GetScalarParameterValue(FMaterialParameterInfo(TEXT("EmissivePower")), EmissivePower);
   bParameters &= ActualMaterial->GetVectorParameterValue(FMaterialParameterInfo(TEXT("MapU")), MapU);
   bParameters &= ActualMaterial->GetVectorParameterValue(FMaterialParameterInfo(TEXT("MapV")), MapV);
  }
  const bool bMaterial = Expected && Sheet.Paper == Expected->Object && Tex && ActualTexture == Tex && bParameters
   && Tex->GetPathName() == FString::Printf(TEXT("/Game/Signal47/Art/Documents/%s.%s"), Expected->Asset, Expected->Asset)
   && ActualMaterial->Parent && ActualMaterial->Parent->GetPathName() == TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque")
   && FMath::IsNearlyEqual(UseMap, 1.f) && FMath::IsNearlyZero(MapIsLinear) && FMath::IsNearlyEqual(NativeLighting, 1.f)
   && FMath::IsNearlyEqual(Roughness, .88f) && FMath::IsNearlyZero(Metalness) && FMath::IsNearlyZero(EmissivePower)
   && MapU.Equals(DocumentMapU, .00001f) && MapV.Equals(DocumentMapV, .00001f);
  const bool bPlane = ArtComp && ArtComp->GetStaticMesh() && ArtComp->GetStaticMesh()->GetPathName() == TEXT("/Engine/BasicShapes/Plane.Plane")
   && ArtComp->GetNumMaterials() == 1 && ArtComp->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !ArtComp->CastShadow;
  const FBox ArtBox = Sheet.Art.IsValid() ? Sheet.Art.WorldBox() : FBox(ForceInit);
  const FBox PaperBox = Sheet.PaperObj.IsValid() ? Sheet.PaperObj.WorldBox() : FBox(ForceInit);
  const FS47WebNode* PaperNode = M->Web->Node(Sheet.PaperObj.I);
  const UPrimitiveComponent* PaperComp = PaperNode ? Cast<UPrimitiveComponent>(PaperNode->Comp.Get()) : nullptr;
  const bool bSizeAndPose = Expected && ArtComp && PaperComp &&
   ArtComp->GetComponentScale().Equals(S47Three::DirToUE(FVector(Expected->Width,1,Expected->Depth)),.00001) &&
   ArtComp->GetComponentQuat().Equals(PaperComp->GetComponentQuat(),.00001);
  const bool bOn = bMaterial && bPlane && Sheet.Art.IsValid() && Sheet.Art.Shown() && Tex && Tex->GetSizeX() >= 1024 && Tex->GetSizeY() >= 1024
   && ArtBox.IsValid && PaperBox.IsValid && ArtBox.Min.Y >= PaperBox.Max.Y - .0001 && ArtBox.Min.Y - PaperBox.Max.Y < .003
   && FMath::Abs(ArtBox.GetCenter().X - PaperBox.GetCenter().X) < .001 && FMath::Abs(ArtBox.GetCenter().Z - PaperBox.GetCenter().Z) < .001
   && bSizeAndPose;
  bOk &= bOn;
  Lines.Add(FString::Printf(TEXT("document art %s %s: %s %dx%d art=%.3fx%.3f m lift=%.4f m"), bOn ? TEXT("PASS") : TEXT("FAIL"), *Sheet.Paper, Tex ? *Tex->GetName() : TEXT("none"),
   Tex ? Tex->GetSizeX() : 0, Tex ? Tex->GetSizeY() : 0, ArtBox.IsValid ? ArtBox.GetSize().X : 0., ArtBox.IsValid ? ArtBox.GetSize().Z : 0., ArtBox.IsValid && PaperBox.IsValid ? ArtBox.Min.Y - PaperBox.Max.Y : -1.));
  Lines.Add(FString::Printf(TEXT("document actual material %s %s: slot0=%s Map=%s UseMap=%.4f MapIsLinear=%.4f NativeLighting=%.4f Roughness=%.4f Metalness=%.4f EmissivePower=%.4f MapU=%s MapV=%s planeNoCollision=%s; UV rows read from actual material, engine-plane source UVs measured by import"),
   bMaterial && bPlane ? TEXT("PASS") : TEXT("FAIL"), *Sheet.Paper, *GetPathNameSafe(ActualMaterial), *GetPathNameSafe(ActualTexture),
   UseMap, MapIsLinear, NativeLighting, Roughness, Metalness, EmissivePower, *MapU.ToString(), *MapV.ToString(), bPlane ? TEXT("yes") : TEXT("no")));
 }
 return bOk;
}

bool FS47Office::Use(const FString& Id)
{
 if (!Here() || !AccessAvailable() || !InReach(Id)) return false;
 if (Id == TEXT("office:drawer"))
 {
  if (!bDrawerTarget) { bDrawerTarget = true; Changed(); M->UI->Toast(TEXT("Fuses, pencils and a tagged brass spare. The drawer has to open before the key can be taken."), 3.5); return true; }
  if (DrawerK < .995) return true;
  if (!bKey) { bKey = true; Changed(); M->UI->Toast(TEXT("Operations spare key retained. This key belongs to the office latch."), 3.5); return true; }
  bDrawerTarget = false; Changed(); return true;
 }
 if (Id == TEXT("office:key"))
 {
  if (bKey || !bDrawerTarget || DrawerK < .995) return false;
  bKey = true; Changed(); M->UI->Toast(TEXT("Operations spare key retained."), 3); return true;
 }
 if (Id == TEXT("office:door") || Id == TEXT("officeDoor"))
 {
  if (!bKey) { bDoorInspected = true; Changed(); M->UI->Toast(TEXT("OPERATIONS / LOCKED. The paper label reads: SPARE IN SERVICE DRAWER."), 4); return true; }
  if (bDoorTarget && InDoorSweep(M->Player->Pos, M->Player->Radius)) { M->UI->Toast(TEXT("Step clear of the doorway before closing it."), 3); return true; }
  bUnlocked = true; bDoorTarget = !bDoorTarget; Changed(); return true;
 }
 if (!Contains(M->Player->Pos) || !WasUnlocked() || !bEverOpened) return false;
 bEntered = true;
 if (Id == TEXT("office:depart"))
 {
  if (!WantsDeparture()) return false;
  bDeparted = true; Changed(); // Save the explicit release before the captured chapter end callback.
  UE_LOG(LogS47Office, Display, TEXT("S47: office explicit departure released; actual field sheet at %.3f"), FieldSheetAt);
  if (OnDepart) OnDepart(); return true;
 }
 if (Id == TEXT("office:private")) { OpenLibrary(); return true; }
 if (Id == TEXT("office:arrayService")) return ReadLibraryDocument(TEXT("EV_DALE_ARRAY_SERVICE"));
 if (Id == TEXT("office:tape"))
 {
  M->UI->Toast(M->Story->bDark ? TEXT("The VHS deck has no power. The field sheet is beside the maintenance forms.") : TEXT("The tape will not play. The field sheet still gives the camera position."), 5);
  return true;
 }
 FString Doc;
 if (Id == TEXT("office:desk")) { bDeskSeen = true; Doc = TEXT("EV_DALE_INDEX"); }
 else if (Id == TEXT("office:folder"))
 {
  bDeskSeen = bIndexRead = true;
  if (!bFieldSheet) { bFieldSheet = true; FieldSheetAt = M->Story->Clock(); }
  Doc = TEXT("EV_DALE_INDEX");
 }
 else if (Id == TEXT("office:index")) { bIndexRead = true; Doc = TEXT("EV_DALE_INDEX"); }
 else if (Id == TEXT("office:clippings")) { bClippingsRead = true; Doc = TEXT("EV_DALE_CLIPPINGS"); }
 else if (Id == TEXT("office:note")) { bNoteRead = true; Doc = TEXT("EV_DALE_NOTE"); }
 else if (Id == TEXT("office:contact")) { bContactRead = true; Doc = TEXT("EV_DALE_CONTACT"); }
 else return false;
 Changed(); OpenDocument(Doc); M->RequestAutosave(); return true;
}

void FS47Office::Apply()
{
 if (!bReady || !M || !M->Web || !Visual) return;
 const bool Visible = M->World && M->World->Area == TEXT("saro") && M->Mode == US47Main::EMode::Play;
 Visual->Root.SetVisible(Visible);
 Visual->Leaf.SetRotation(FVector(0, -PI * .5 * DoorK, 0));
 for (auto& Part : Visual->DrawerParts) Part.Key.SetPosition(Part.Value + FVector(0, 0, -.24 * DrawerK));
 for (FS47Obj Part : Visual->KeyParts) Part.SetVisible(!bKey && DrawerK >= .995);
 for (const auto& Entry : Visual->Objects) if (const FS47WebNode* N = M->Web->Node(Entry.Value.I)) if (UPrimitiveComponent* C = Cast<UPrimitiveComponent>(N->Comp.Get()))
  C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, Entry.Value.Shown() ? ECR_Block : ECR_Ignore);
 // bPowered belongs to RX bank 3, not the building lighting circuit. The office
 // is lit during handover before the receiver is started, and goes dark with SARO.
 if (URectLightComponent* Light = Visual->Light.Get()) Light->SetVisibility(Visible && M->Story && !M->Story->bDark);
 if (URectLightComponent* Light = Visual->TaskLight.Get()) Light->SetVisibility(Visible && M->Story && !M->Story->bDark);
 if (URectLightComponent* Light = Visual->DeskBounce.Get()) Light->SetVisibility(Visible && M->Story && !M->Story->bDark && M->Web->GetGraphicsSettings().IndirectLighting == 0);
 for (FS47Rect& Rect : M->Web->ZonesMutable(TEXT("saro"))) if (Rect.Id == TEXT("office:thresholdZone")) Rect.bEnabled = DoorOpen();
 const double Angle = PI * .5 * DoorK, EndX = HingeX + LeafWidth * FMath::Cos(Angle), EndZ = HingeZ + LeafWidth * FMath::Sin(Angle);
 for (FS47Rect& Rect : M->Web->CollidersMutable(TEXT("saro"))) if (Rect.Id == TEXT("office:doorCollider"))
 { Rect.MinX = FMath::Min(HingeX, EndX) - .035; Rect.MaxX = FMath::Max(HingeX, EndX) + .035; Rect.MinZ = FMath::Min(HingeZ, EndZ) - .035; Rect.MaxZ = FMath::Max(HingeZ, EndZ) + .035; }
}

bool FS47Office::MigrateRestoredPose(FVector& P, double Radius)
{
 if (!Visual->bRecoverLegacyPose || !bReady || !M || !M->Player || !M->World || !M->Web) return false;
 Visual->bRecoverLegacyPose = false;
 const FVector Before = P;
 if (M->World->Area != TEXT("saro") || !InBox(Before, -1.1, 2.05, -2, 4, 6.0, 10.31)) return false;
 auto Clear = [&](double X, double Z)
 {
  if (!M->Player->Walkable(X, Z)) return false;
  for (const FS47Rect& C : M->Web->Colliders()) if (C.bEnabled && X + Radius > C.MinX && X - Radius < C.MaxX && Z + Radius > C.MinZ && Z - Radius < C.MaxZ) return false;
  return true;
 };
 if (Clear(P.X, P.Z)) return false;
 const bool bKeepInside = bEntered && WasUnlocked() && Before.Z >= 6.8;
 double Best = TNumericLimits<double>::Max(); FVector At = Before;
 for (int32 X = -10; X <= 30; ++X) for (int32 Z = 106; Z <= 196; ++Z)
 {
  const double PX = X * .05, PZ = Z * .05;
  if (bKeepInside && PZ < 6.8 + FrontOffset + Radius + .02) continue;
  if (!Clear(PX, PZ)) continue;
  const double Distance = FMath::Square(PX - Before.X) + FMath::Square(PZ - Before.Z);
  if (Distance < Best) { Best = Distance; At = FVector(PX, 0, PZ); }
 }
 if (Best == TNumericLimits<double>::Max())
 {
  // Do not mark a failed migration complete in a future save.
  Visual->bRecoverLegacyPose = true;
  Visual->bMigrationFailed = true;
  UE_LOG(LogS47Office, Error, TEXT("S47: legacy interior pose has no clear local recovery at %.3f %.3f"), Before.X, Before.Z);
  return false;
 }
 P = At;
 UE_LOG(LogS47Office, Display, TEXT("S47: legacy interior pose migrated %.3f %.3f -> %.3f %.3f before Flow placement; story/key/documents/view preserved"), Before.X, Before.Z, At.X, At.Z);
 return true;
}

void FS47Office::Step(double Dt)
{
 if (!bBuilt) Init(); if (!bReady) return;
 // A case without an incoming pose has nothing to migrate. Flow handles saved
 // poses before its walkability gate, while their original coordinates exist.
 if (Visual->bRecoverLegacyPose && !Visual->bMigrationFailed && M && M->Flow && !M->Flow->bStarting) Visual->bRecoverLegacyPose = false;
 if (!bDoorTarget && DoorK > 0 && M && M->Player && M->World && M->World->Area == TEXT("saro") && InDoorSweep(M->Player->Pos, M->Player->Radius))
 { bDoorTarget = true; if (M) M->RequestAutosave(); }
 const bool WasOpen = DoorOpen();
 const double SafeDt = FMath::Clamp(Dt, 0., .1);
 if (Visual->bOwnedPreparing && M && M->Web)
 {
  Visual->OwnedRetryWait -= SafeDt;
  if (Visual->OwnedRetryWait <= 0)
  {
   const bool bOwnedReady = S47OfficeOwned::Build(M->Web, &Visual->bOwnedPreparing);
   Visual->OwnedRetryWait = .25;
   if (!bOwnedReady && !Visual->bOwnedPreparing && Visual->bOwnedFootprint)
   {
    for (FS47Rect& Rect : M->Web->CollidersMutable(TEXT("saro"))) if (Rect.Id == TEXT("office:ownedOpenBox")) Rect.bEnabled = false;
    Visual->bOwnedFootprint = false; // A genuinely unavailable optional set retains the baseline.
   }
  }
 }
 DoorK = FMath::Clamp(DoorK + (bDoorTarget ? 1 : -1) * SafeDt * 1.8, 0., 1.);
 DrawerK = FMath::Clamp(DrawerK + (bDrawerTarget ? 1 : -1) * SafeDt * 3.0, 0., 1.);
 Apply();
 if (!WasOpen && DoorOpen()) { bEverOpened = true; Changed(); }
 if (!bEntered && WasUnlocked() && bEverOpened && Here() && Contains(M->Player->Pos)) { bEntered = true; Changed(); }
 SyncWait -= SafeDt;
 if (SyncWait <= 0) { SyncFacts(); SyncWait = .25; }
}

S47J::FObj FS47Office::Save() const
{
 auto O = S47J::New(); S47J::Set(O, TEXT("v"), 1); S47J::Set(O, TEXT("interiorLayout"), Visual->bRecoverLegacyPose ? 1 : 2); S47J::Set(O, TEXT("routeVersion"), 1); S47J::Set(O, TEXT("routeRequired"), bRequired);
 S47J::Set(O, TEXT("key"), bKey); S47J::Set(O, TEXT("unlocked"), WasUnlocked()); S47J::Set(O, TEXT("everOpened"), bEverOpened);
 S47J::Set(O, TEXT("entered"), bEntered); S47J::Set(O, TEXT("doorInspected"), bDoorInspected); S47J::Set(O, TEXT("doorOpen"), bDoorTarget); S47J::Set(O, TEXT("drawerOpen"), bDrawerTarget);
 S47J::Set(O, TEXT("deskSeen"), bDeskSeen); S47J::Set(O, TEXT("indexRead"), bIndexRead); S47J::Set(O, TEXT("clippingsRead"), bClippingsRead);
 S47J::Set(O, TEXT("noteRead"), bNoteRead); S47J::Set(O, TEXT("contactRead"), bContactRead); S47J::Set(O, TEXT("fieldSheet"), bFieldSheet);
 if (bFieldSheet && FMath::IsFinite(FieldSheetAt)) S47J::Set(O, TEXT("fieldSheetAt"), FieldSheetAt);
 TArray<TSharedPtr<FJsonValue>> Read; for (const FString& Id : ReadDocuments) if (DocumentRead(Id)) Read.Add(MakeShared<FJsonValueString>(Id));
 O->SetArrayField(TEXT("readDocuments"), Read);
 S47J::Set(O, TEXT("hasFieldSheet"), HasFieldSheet()); S47J::Set(O, TEXT("departed"), DepartureReleased()); return O;
}

bool FS47Office::SelfTest(TArray<FString>& Lines)
{
 bool Ok = true;
 auto Check = [&](bool Value, const TCHAR* Why) { Lines.Add(FString(Value ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("Office optional papers: ") + Why); Ok &= Value; };
 FS47Office A(nullptr), B(nullptr);
 A.bKey = A.bUnlocked = A.bEverOpened = A.bEntered = true;
 A.ReadDocuments = {TEXT("EV_DALE_SKYLOG")};
 const auto Actual = A.Save(); B.Begin(S47J::Clone(Actual));
 Check(B.DocumentRead(TEXT("EV_DALE_SKYLOG")) && !B.DocumentRead(TEXT("EV_DALE_WITNESS")), TEXT("one opened page restores without reading the collection"));
 auto Old = S47J::Clone(Actual); Old->RemoveField(TEXT("readDocuments")); B.Begin(Old);
 Check(!B.DocumentRead(TEXT("EV_DALE_SKYLOG")), TEXT("old office save gains no new optional read"));
 auto Locked = S47J::Clone(Actual); S47J::Set(Locked, TEXT("key"), false); B.Begin(Locked);
 Check(!B.DocumentRead(TEXT("EV_DALE_SKYLOG")), TEXT("read list without actual entry and own key is ignored"));
 auto Invalid = S47J::Clone(Actual); Invalid->SetArrayField(TEXT("readDocuments"), {MakeShared<FJsonValueString>(TEXT("EV_DALE_SKYLOG")), MakeShared<FJsonValueBoolean>(true)}); B.Begin(Invalid);
 Check(!B.DocumentRead(TEXT("EV_DALE_SKYLOG")), TEXT("mixed invalid read-list types give no partial credit"));
 auto Unknown = S47J::Clone(Actual); S47J::Set(Unknown, TEXT("readDocuments"), TArray<FString>{TEXT("VHS_DALE_FIELD_01")}); B.Begin(Unknown);
 Check(!B.DocumentRead(TEXT("VHS_DALE_FIELD_01")), TEXT("a film ID cannot be acknowledged as an opened paper"));
 Check(LibraryDocumentIds().Num() == 7 && !LibraryDocumentIds().Contains(TEXT("EV_DALE_INDEX")), TEXT("six optional texts and separate service appendix preserve T02 index identity"));
 return Ok;
}
