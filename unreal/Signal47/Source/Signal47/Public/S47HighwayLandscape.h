#pragma once
#include "CoreMinimal.h"

// Rendering coordinates only. The ordinary drive areas retain their own floors/routes.
namespace S47HighwayLandscape
{
 bool Exterior(const FString& Area);
 FTransform AreaTransform(const FString& Area);
 FVector ToCanonical(const FString& Area,const FVector& SceneThree);
 bool SelfTest(TArray<FString>& Lines);
}
