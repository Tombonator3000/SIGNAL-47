#pragma once

#include "CoreMinimal.h"

class AS47Web;

// Optional private owned props. Native binding checks do not certify bake or visual quality.
namespace S47OfficeOwned
{
 bool Build(AS47Web* World);
 bool Audit(AS47Web* World, TArray<FString>& Lines);
}
