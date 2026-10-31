#pragma once

#include "CoreMinimal.h"

class AS47Web;
class UStaticMeshComponent;
class UStaticMesh;

// Narrow render edits for explicitly selected legacy SARO parts. Original
// assets, material bindings, UVs, collision and gameplay handles are retained.
namespace S47SaroLegacyCleanup
{
 // The exact authored source behind this helper's transient render copies.
 // Other meshes keep their own path; names alone never establish provenance.
 FString SourceAssetPath(const UStaticMesh* Mesh);
 struct FEdit
 {
  FString Id;
  FString RootPath;
  TArray<int32> GeometryIds;
  FBox ThreeBounds=FBox(ForceInit);
  FVector OffsetThree=FVector::ZeroVector;
  int32 MinTriangles=1;
  int32 MaxTriangles=MAX_int32;
  bool bRemove=true;
  TArray<FName> MaterialSlotNames;
  double BoundsToleranceM=.00005;
  // Optional upper bound on every vertex's RGB brightness (0..1). This lets
  // a dark bin skin be removed while bright paper in the same mesh remains.
  float MaxVertexBrightness=-1;
 };

 // Every selected triangle must have all three original vertices in the box.
 // RootPath and GeometryIds are mandatory; only original /Game/Web meshes or
 // this helper's previous transient copies can match. Id is once per world.
 bool Apply(AS47Web* World,const TArray<FEdit>& Edits,FString* Why=nullptr);

 // Direct component variant for an explicitly chosen native CPU duplicate.
 // No world search. Every edit must name at least one exact material slot.
 // RootPath/GeometryIds are unused here; the caller owns the selected component.
 bool ApplyToComponent(AS47Web* World,UStaticMeshComponent* Component,
                       const TArray<FEdit>& Edits,FString* Why=nullptr);

 // Original corridor prints/board move in front of the new paint; only the
 // decorative hall coat and middle-chair cardigan are removed. The interactive
 // room.objs.jacket and every chair remain outside these edits.
 bool Build(AS47Web* World);
}
