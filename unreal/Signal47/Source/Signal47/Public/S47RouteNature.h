#pragma once

#include "CoreMinimal.h"

class AS47Web;

// Unreal-only roadside composition. Coordinates, floors and route boundaries remain
// the existing road's; the shared highway root carries the visual copies between areas.
class SIGNAL47_API FS47RouteNature
{
public:
 static TSharedPtr<FS47RouteNature> Build(AS47Web* Web);
 bool Audit(TArray<FString>& Lines) const;

private:
 TWeakObjectPtr<AS47Web> W;
 TArray<int32> Nodes;
 TArray<int32> Counts;
 struct FBlock { FString Area,Id; FVector4 Limits; };
 TArray<FBlock> Blocks;
 int32 RockColliders=0, RetiredInstances=0, SeamTriangles=0;
};
