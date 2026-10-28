#include "S47GrassWind.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
namespace S47GrassWind
{
 bool Apply(UHierarchicalInstancedStaticMeshComponent* C, const FString& Family)
 {
  if (!C || (Family != TEXT("Grass_A") && Family != TEXT("Grass_B")) ||
      FParse::Param(FCommandLine::Get(), TEXT("S47LegacyGrassWind"))) return false;
  UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/CalicheStreet/NatureVegetation/Materials/M_CN_GrassWind.M_CN_GrassWind"));
  if (!M || !M->CheckMaterialUsage_Concurrent(MATUSAGE_InstancedStaticMeshes))
  { UE_LOG(LogTemp, Warning, TEXT("S47: grass wind missing, original material retained: %s"), *Family); return false; }
  for (int32 Slot = 0; Slot < C->GetStaticMesh()->GetStaticMaterials().Num(); ++Slot) C->SetMaterial(Slot, M);
  C->SetBoundsScale(1.1f);
  UE_LOG(LogTemp, Display, TEXT("S47: grass-only wind %s, fixed roots, <=2.2cm tip, fade20-70m; existing shadow/RT policy"), *Family);
  return true;
 }
}
