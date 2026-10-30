#pragma once

#include "CoreMinimal.h"

class AS47Web;

// Unreal-only circulation correction. Original story handles and door indices remain intact.
namespace S47SaroInterior
{
 bool Build(AS47Web* World);
 bool Audit(AS47Web* World, TArray<FString>& Lines);
 // Semantic indoor space only. This does not grant entry into the locked office.
 bool ContainsPassingBay(AS47Web* World, const FVector& Position);
}
