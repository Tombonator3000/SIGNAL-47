#pragma once

#include "CoreMinimal.h"
class US47Main;
class AS47Web;
class FJsonObject;

// Exact authored material/mesh bindings; never changes geometry or quest glass.
namespace S47Lookdev
{
 void Update(US47Main* Main, double Dt);
 TSharedPtr<FJsonObject> Readback(const AS47Web* Web);
}
