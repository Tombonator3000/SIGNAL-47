#pragma once

#include "CoreMinimal.h"

class AS47Web;

// Optional private owned props. Native binding checks do not certify bake or visual quality.
namespace S47OfficeOwned
{
 // Preparing means no current structural mismatch and pending native resources;
 // async render checks finish on the ready retry. Audit always requires ready maps.
 bool Build(AS47Web* World, bool* bPreparing = nullptr);
 bool Audit(AS47Web* World, TArray<FString>& Lines);
}
