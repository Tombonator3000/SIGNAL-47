#pragma once

#include "CoreMinimal.h"

class AS47Web;

// Visual children only. The exported doors retain their pivots, proxies and state.
namespace S47SaroDoorDetail
{
 bool Build(AS47Web* World);
 bool Audit(AS47Web* World, TArray<FString>& Lines);
}
