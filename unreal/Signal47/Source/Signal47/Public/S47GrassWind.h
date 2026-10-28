#pragma once
#include "CoreMinimal.h"
class UHierarchicalInstancedStaticMeshComponent;
namespace S47GrassWind
{
 // Only the two authored grass families: never enable WPO on the shared shrub/tree material.
 bool Apply(UHierarchicalInstancedStaticMeshComponent* Component, const FString& Family);
}
