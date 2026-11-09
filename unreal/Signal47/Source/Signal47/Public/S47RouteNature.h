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

 // PT-ROCKS-20261109, F10 "World fixes (T)": the one built last (the road's), its big rocks, and two switches for before and after
 // pictures. Nothing here is saved; the colliders stay those of the rocks as they now stand.
 static FS47RouteNature* Live();
 struct FRock { FString Family; FVector UE; double Side=0; int32 Cluster=-1; };
 // Every big rock (cliff, boulders, outcrop), where it stands (Unreal), which side of the road, and the composed group it is in.
 TArray<FRock> Rocks() const;
 // The rocks as they stood before 9 November (the authored turn, the old sink) or as now.
 void DevBefore(bool bBefore);
 bool DevIsBefore() const { return bDevBefore; }
 // The rocks in ray tracing (Lumen's hardware rays and their reduced proxies) or out of it.
 void DevRayTracing(bool bOn);
 // Which families have a measured cut (Tools/routenature/cut_faces.py), for the screen.
 FString CutNote() const;

private:
 TWeakObjectPtr<AS47Web> W;
 TArray<int32> Nodes;
 TArray<int32> Counts;
 struct FBlock { FString Area,Id; FVector4 Limits; };
 TArray<FBlock> Blocks;
 int32 RockColliders=0, RetiredInstances=0, SeamTriangles=0;
 struct FRockSet { FString Family; TWeakObjectPtr<class UInstancedStaticMeshComponent> C; TArray<FTransform> Now, Before; };
 TArray<FRockSet> RockSets;
 bool bDevBefore=false;
};
