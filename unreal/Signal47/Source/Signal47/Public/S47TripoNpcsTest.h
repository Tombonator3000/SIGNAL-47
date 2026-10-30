#pragma once

#include "CoreMinimal.h"

class UStaticMeshComponent;

// One opt-in smoke check of the actual imported mannequin components.
namespace S47TripoNpcsTest
{
 bool Check(const TArray<UStaticMeshComponent*>& Components, TArray<FString>& Errors);
}
