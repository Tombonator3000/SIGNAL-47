// Unreal-only contact detail. Native meshes use site coordinates; placement is applied once.
#include "S47SubstationDetail.h"

#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshResources.h"
#include <cfloat>

namespace S47SubstationDetail
{
namespace
{
 struct FPart
 {
  const TCHAR* Name;
  int32 Triangles[3];
  const TCHAR* Materials[3];
  int32 MaterialCount;
  FVector CenterCm, ExtentCm;
  bool bShoulder;
 };
 struct FFloorTriangle { FVector A, B, C; };
 #include "S47SubstationDetailData.inl"

 const FString Prefix(TEXT("substationDetail:"));
 const FTransform SiteTransform(FRotator(0, 180, 0), FVector(789000, 53620, -110), FVector::OneVector);
 TWeakObjectPtr<AS47Web> ActiveWorld;
 TWeakObjectPtr<UStaticMeshComponent> ActiveShoulder;
 int32 ActiveShoulderNode = -1;

 bool HeightAt(double X, double Y, double& Z)
 {
  // Conservative bound only: a hit still requires containment in an authored triangle.
  if (!FMath::IsFinite(X) || !FMath::IsFinite(Y) || X < -11 || X > 11 || Y < -9 || Y > 9) return false;
  bool Found = false;
  double Highest = -DBL_MAX;
  for (const FFloorTriangle& T : ShoulderTriangles)
  {
   const double Den = (T.B.Y - T.C.Y) * (T.A.X - T.C.X) + (T.C.X - T.B.X) * (T.A.Y - T.C.Y);
   if (FMath::Abs(Den) < 1e-12) continue;
   const double U = ((T.B.Y - T.C.Y) * (X - T.C.X) + (T.C.X - T.B.X) * (Y - T.C.Y)) / Den;
   const double V = ((T.C.Y - T.A.Y) * (X - T.C.X) + (T.A.X - T.C.X) * (Y - T.C.Y)) / Den;
   const double W = 1 - U - V;
   if (U < -1e-9 || V < -1e-9 || W < -1e-9) continue;
   Highest = FMath::Max(Highest, U * T.A.Z + V * T.B.Z + W * T.C.Z);
   Found = true;
  }
  if (Found) Z = Highest;
  return Found;
 }

 bool ValidAsset(UStaticMesh* Mesh, const FPart& Part)
 {
  if (!Mesh || !Mesh->GetRenderData() || Mesh->GetRenderData()->LODResources.Num() != 3 ||
      Mesh->GetStaticMaterials().Num() != Part.MaterialCount) return false;
  for (int32 Lod = 0; Lod < 3; ++Lod) if (Mesh->GetNumTriangles(Lod) != Part.Triangles[Lod]) return false;
  TSet<FName> Names;
  for (const FStaticMaterial& Slot : Mesh->GetStaticMaterials())
  {
   bool Expected = false;
   for (int32 I = 0; I < Part.MaterialCount; ++I) if (Slot.MaterialSlotName == FName(Part.Materials[I])) Expected = true;
   FLinearColor Emissive;
   if (!Expected || Names.Contains(Slot.MaterialSlotName) || !Slot.MaterialInterface ||
       !Slot.MaterialInterface->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveFactor")), Emissive) ||
       FMath::Max3(FMath::Abs(Emissive.R), FMath::Abs(Emissive.G), FMath::Abs(Emissive.B)) > .0001f) return false;
   Names.Add(Slot.MaterialSlotName);
  }
  if (Part.bShoulder && (!Mesh->GetBodySetup() || Mesh->GetBodySetup()->CollisionTraceFlag != CTF_UseComplexAsSimple)) return false;
  const FBoxSphereBounds B = Mesh->GetBounds();
  return !B.Origin.ContainsNaN() && !B.BoxExtent.ContainsNaN() &&
   B.Origin.Equals(Part.CenterCm, .2) && B.BoxExtent.Equals(Part.ExtentCm, .2);
 }

 bool CheckShoulder(UStaticMeshComponent* C)
 {
  if (!C || !C->GetComponentTransform().Equals(SiteTransform, .001) ||
      C->GetCollisionEnabled() != ECollisionEnabled::QueryOnly ||
      C->GetCollisionResponseToChannel(ECC_Visibility) != ECR_Block ||
      C->GetCollisionResponseToChannel(ECC_GameTraceChannel1) != ECR_Ignore) return false;
  // Three cheap component traces bind the analytic walk surface to the cooked native triangles.
  const int32 ProbeIndices[] = {0, int32(UE_ARRAY_COUNT(ShoulderTriangles) / 2), int32(UE_ARRAY_COUNT(ShoulderTriangles) - 1)};
  for (int32 I : ProbeIndices)
  {
   const FFloorTriangle& T = ShoulderTriangles[I];
   const FVector Center = (T.A + T.B + T.C) / 3;
   double Z = 0;
   if (!HeightAt(Center.X, Center.Y, Z)) return false;
   const FVector At(100 * (7890 - Center.X), 100 * (536.2 + Center.Y), 100 * (-1.1 + Z));
   FHitResult Hit;
   FCollisionQueryParams Query(SCENE_QUERY_STAT(S47SubstationShoulder), true);
   if (!C->LineTraceComponent(Hit, At + FVector(0, 0, 50), At - FVector(0, 0, 50), Query) ||
       FMath::Abs(Hit.ImpactPoint.Z - At.Z) > .2) return false;
  }
  double Unused = 0;
  return !HeightAt(0, 0, Unused) && !HeightAt(20, 20, Unused);
 }
}

bool Build(AS47Web* W)
{
 // Never retain floor data from an old world, failed install or disabled detail pass.
 ActiveWorld.Reset(); ActiveShoulder.Reset(); ActiveShoulderNode = -1;
 if (!W || FParse::Param(FCommandLine::Get(), TEXT("S47SubstationDetailOff")) ||
     FParse::Param(FCommandLine::Get(), TEXT("S47PropsOff"))) return false;
 const int32 Parent = W->Handle(TEXT("world.legs.road.area.group"));
 const FS47WebNode* Road = W->Node(Parent);
 if (!Road || !Road->Comp.IsValid()) return false;
 const FS47Obj Root = W->ObjAt(Parent);
 for (const TCHAR* Name : {TEXT("Transformer"), TEXT("Buswork"), TEXT("Control"), TEXT("StatusLamp"), TEXT("Lever"), TEXT("Fence"), TEXT("Gate_L"), TEXT("Gate_R"), TEXT("Pad"), TEXT("Gravel")})
 {
  const FS47Obj O = Root.Find(FString(TEXT("prop:substation")) + Name);
  const FS47WebNode* N = W->Node(O.I);
  const UStaticMeshComponent* C = N ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
  if (!C || !C->GetStaticMesh() || C->GetStaticMesh()->GetName() != FString(TEXT("SM_Substation_")) + Name)
  {
   UE_LOG(LogTemp, Warning, TEXT("S47: substation detail skipped: original kit missing %s"), Name);
   return false;
  }
  if ((FString(Name) == TEXT("Transformer") || FString(Name) == TEXT("Gravel")) &&
      !C->GetComponentTransform().Equals(SiteTransform, .001)) return false;
 }

 TArray<UStaticMesh*> Meshes;
 TArray<UStaticMeshComponent*> Existing;
 int32 ExistingCount = 0, TotalTriangles = 0;
 for (const FPart& Part : Parts)
 {
  const FString Asset = FString::Printf(TEXT("/Game/SubstationDetail/%s.%s"), Part.Name, Part.Name);
  UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Asset);
  if (!ValidAsset(Mesh, Part))
  {
   UE_LOG(LogTemp, Warning, TEXT("S47: substation detail skipped: invalid native asset %s"), Part.Name);
   return false;
  }
  Meshes.Add(Mesh); TotalTriangles += Part.Triangles[0];
  UStaticMeshComponent* Found = nullptr;
  for (int32 I = 0; I < W->PartCount(); ++I)
   if (const FS47WebNode* N = W->Node(I); N && N->Name == Prefix + Part.Name)
   {
    if (Found) return false;
    Found = Cast<UStaticMeshComponent>(N->Comp.Get());
    if (!Found || N->Parent != Parent || Found->GetStaticMesh() != Mesh || !Found->GetComponentTransform().Equals(SiteTransform, .001)) return false;
    ++ExistingCount;
   }
  Existing.Add(Found);
 }
 if (ExistingCount && ExistingCount != UE_ARRAY_COUNT(Parts))
 {
  UE_LOG(LogTemp, Warning, TEXT("S47: substation detail skipped: partial existing installation"));
  return false;
 }
 TArray<UStaticMeshComponent*> Components;
 UStaticMeshComponent* Shoulder = nullptr;
 for (int32 I = 0; I < Meshes.Num(); ++I)
 {
  UStaticMeshComponent* C = Existing[I];
  if (!C)
  {
   C = NewObject<UStaticMeshComponent>(W, *FString::Printf(TEXT("S47SubstationDetail_%s"), Parts[I].Name));
   C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(Road->Comp.Get()); C->SetStaticMesh(Meshes[I]);
   C->SetCollisionEnabled(Parts[I].bShoulder ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
   C->SetCollisionResponseToAllChannels(ECR_Ignore);
   if (Parts[I].bShoulder) C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
   C->SetGenerateOverlapEvents(false); C->SetCanEverAffectNavigation(false); C->SetCastShadow(true);
   C->RegisterComponent(); C->SetWorldTransform(SiteTransform);
  }
  Components.Add(C);
  if (Parts[I].bShoulder) Shoulder = C;
 }
 if (!CheckShoulder(Shoulder))
 {
  if (!ExistingCount) for (UStaticMeshComponent* C : Components) C->DestroyComponent();
  UE_LOG(LogTemp, Warning, TEXT("S47: substation detail skipped: native shoulder/floor trace mismatch"));
  return false;
 }
 for (int32 I = 0; I < Components.Num(); ++I)
 {
  const int32 Node = ExistingCount ? W->NodeOf(Components[I]) : W->AdoptNode(Components[I], Parent, Prefix + Parts[I].Name);
  if (Parts[I].bShoulder) ActiveShoulderNode = Node;
 }
 ActiveWorld = W; ActiveShoulder = Shoulder;
 UE_LOG(LogTemp, Display, TEXT("S47: substation detail installed parts=%d triangles=%d; shoulder native traces=3 PASS, exact floor triangles=%d; original interactions retained"), Components.Num(), TotalTriangles, int32(UE_ARRAY_COUNT(ShoulderTriangles)));
 return true;
}

bool FloorAt(double WorldX, double WorldZ, double& WorldY)
{
 AS47Web* W = ActiveWorld.Get();
 UStaticMeshComponent* C = ActiveShoulder.Get();
 if (!W || !C || ActiveShoulderNode < 0 || !W->ObjAt(ActiveShoulderNode).Shown() ||
     !C->GetComponentTransform().Equals(SiteTransform, .001)) return false;
 double Bz = 0;
 if (!HeightAt(7890 - WorldX, WorldZ - 536.2, Bz)) return false;
 WorldY = -1.1 + Bz;
 return true;
}
}
