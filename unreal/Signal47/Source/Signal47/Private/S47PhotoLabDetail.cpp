// Unreal-only photolab detailing. The exported meshes already contain their world placement.
#include "S47PhotoLabDetail.h"

#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "StaticMeshResources.h"

namespace S47PhotoLabDetail
{
namespace
{
 struct FPart { const TCHAR* Name; int32 Triangles, Materials; FVector CenterCm, ExtentCm; };
 // Measured source contract, ArtSource/PhotoLab/model_contract.json. Update together if rebuilt.
 const FPart Parts[] = {
  { TEXT("SM_PhotoLab_WetCabinetDetail"), 3960, 4, FVector(1519.999981, -509.749985, 47.125000), FVector(210.999966, 3.649998, 46.625000) },
  { TEXT("SM_PhotoLab_RinseSink"), 2632, 5, FVector(1796.807003, -548.199940, 61.119579), FVector(53.207111, 28.699970, 60.819579) },
  { TEXT("SM_PhotoLab_EnlargerDetail"), 3260, 5, FVector(1700.000000, -540.625000, 144.174999), FVector(23.700142, 23.624992, 45.975000) },
  { TEXT("SM_PhotoLab_ProcessDetails"), 1380, 4, FVector(1534.125042, -547.125006, 103.250000), FVector(159.525013, 27.374983, 9.750000) },
  { TEXT("SM_PhotoLab_StorageDetail"), 3840, 7, FVector(1301.475000, -430.000007, 107.499996), FVector(17.724943, 119.999993, 77.499995) },
  { TEXT("SM_PhotoLab_ArchiveDetail"), 1896, 4, FVector(1856.350040, -309.499997, 57.749999), FVector(38.650036, 144.499999, 54.750001) },
  { TEXT("SM_PhotoLab_RecordsDetail"), 1620, 3, FVector(1699.999952, 41.449999, 50.799999), FVector(132.000017, 36.049999, 50.699999) },
  { TEXT("SM_PhotoLab_SafelightHousing"), 1276, 4, FVector(1555.318975, -407.694173, 257.972455), FVector(73.618937, 182.194161, 22.672462) },
  { TEXT("SM_PhotoLab_Ventilation"), 1748, 4, FVector(1859.874916, -573.365378, 187.000000), FVector(29.375076, 5.384612, 61.000001) },
  { TEXT("SM_PhotoLab_UtilityDetail"), 2004, 5, FVector(1600.000048, -443.474996, 138.250006), FVector(330.000067, 135.724986, 137.250006) }
 };
 const FString Prefix(TEXT("photoLabDetail:"));

 bool ValidAsset(UStaticMesh* Mesh, const FPart& Part)
 {
  if (!Mesh || !Mesh->GetRenderData() || Mesh->GetRenderData()->LODResources.Num() != 3) return false;
  const int32 T0 = Mesh->GetNumTriangles(0), T1 = Mesh->GetNumTriangles(1), T2 = Mesh->GetNumTriangles(2);
  if (T0 != Part.Triangles || T0 <= T1 || T1 <= T2 || T2 <= 0) return false;
  if (Mesh->GetStaticMaterials().Num() != Part.Materials) return false;
  for (const FStaticMaterial& Slot : Mesh->GetStaticMaterials())
  {
   FLinearColor Emissive;
   if (!Slot.MaterialInterface || !Slot.MaterialSlotName.ToString().StartsWith(TEXT("M_PhotoLab_")) ||
       !Slot.MaterialInterface->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveFactor")), Emissive) ||
       FMath::Max3(FMath::Abs(Emissive.R), FMath::Abs(Emissive.G), FMath::Abs(Emissive.B)) > .0001f) return false;
  }

  const FBoxSphereBounds Bounds = Mesh->GetBounds();
  // Source bounds are converted from Blender metres to UE centimetres at import, once.
  return !Bounds.Origin.ContainsNaN() && !Bounds.BoxExtent.ContainsNaN() &&
   Bounds.Origin.Equals(Part.CenterCm, .35) && Bounds.BoxExtent.Equals(Part.ExtentCm, .35);
 }
}

bool Build(AS47Web* W)
{
 if (!W || FParse::Param(FCommandLine::Get(), TEXT("S47PhotoLabDetailOff")) ||
     FParse::Param(FCommandLine::Get(), TEXT("S47PropsOff"))) return false;

 const int32 Parent = W->Handle(TEXT("yard.occluders[2]"));
 const FS47WebNode* Lab = W->Node(Parent);
 if (!Lab || !Lab->Comp.IsValid() || Lab->Parent != W->Handle(TEXT("yard.group")) ||
     !W->NodeWorldThree(Parent).Equals(FTransform::Identity, .0001))
 {
  UE_LOG(LogTemp, Warning, TEXT("S47: photolab detail skipped: original lab parent differs"));
  return false;
 }
 USceneComponent* ParentComponent = Lab->Comp.Get();

 // A second invocation must not duplicate fittings. A partial old installation is left alone.
 int32 Existing = 0;
 for (const FPart& Part : Parts)
  for (int32 I = 0; I < W->PartCount(); ++I)
   if (const FS47WebNode* Node = W->Node(I); Node && Node->Name == Prefix + Part.Name) ++Existing;
 if (Existing)
 {
  if (Existing != UE_ARRAY_COUNT(Parts))
   UE_LOG(LogTemp, Warning, TEXT("S47: photolab detail skipped: partial existing set (%d)"), Existing);
  return Existing == UE_ARRAY_COUNT(Parts);
 }

 // Resolve every asset before adding anything. Missing optional detail never hides the old lab.
 TArray<UStaticMesh*> Meshes;
 int32 TotalTriangles = 0;
 for (const FPart& Part : Parts)
 {
  const FString Path = FString::Printf(TEXT("/Game/Signal47/Art/PhotoLab/%s.%s"), Part.Name, Part.Name);
  UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
  if (!ValidAsset(Mesh, Part))
  {
   UE_LOG(LogTemp, Warning, TEXT("S47: photolab detail skipped: invalid asset %s"), Part.Name);
   return false;
  }
  TotalTriangles += Mesh->GetNumTriangles(0);
  Meshes.Add(Mesh);
 }
 if (TotalTriangles > 80000)
 {
  UE_LOG(LogTemp, Warning, TEXT("S47: photolab detail skipped: authored triangle budget exceeded (%d)"), TotalTriangles);
  return false;
 }

 for (int32 I = 0; I < Meshes.Num(); ++I)
 {
  UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W,
   *FString::Printf(TEXT("S47PhotoLabDetail_%s"), Parts[I].Name));
  C->SetMobility(EComponentMobility::Movable);
  C->SetupAttachment(ParentComponent);
  C->SetStaticMesh(Meshes[I]);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->SetCollisionResponseToAllChannels(ECR_Ignore);
  C->SetGenerateOverlapEvents(false);
  C->SetCanEverAffectNavigation(false);
  C->SetCastShadow(true);
  C->RegisterComponent();
  // The project's FBX import already maps (Blender x,-game z,height) into UE centimetres.
  // Applying the lab's centre again would move all ten parts out of the room.
  C->SetWorldTransform(FTransform::Identity);
  W->AdoptNode(C, Parent, Prefix + Parts[I].Name);
 }
 UE_LOG(LogTemp, Display, TEXT("S47: photolab detail installed parts=%d triangles=%d; original collision, interactions and light controls retained"), Meshes.Num(), TotalTriangles);
 return true;
}
}
