#pragma once

#include "CoreMinimal.h"
#include "S47Web.h"

// A small visibility graph in the world's metre-based x/z plane. It does not move an actor.
// Every returned segment clears the enabled rectangles by Clearance and passes CanTraverse.
// The adapter still validates each actual movement step against the current native world.
class SIGNAL47_API FS47ThreatNavigation
{
public:
 // The path omits From and ends exactly at To. Empty means no complete route was found;
 // there is no straight-line fallback or partial path presented as a reached target.
 // Seeds describe useful passages; additional candidates come from expanded obstacle corners.
 // Work is bounded to 96 graph points and a local detour horizon, so an empty result is not
 // proof that no route exists anywhere in the world. CanTraverse may be directional.
 static TArray<FVector2D> FindPath(const FVector2D& From, const FVector2D& To,
  const TArray<FVector2D>& Seeds, const TArray<FS47Rect>& Blocks, double Clearance,
  TFunctionRef<bool(const FVector2D&, const FVector2D&)> CanTraverse);

 static bool SelfTest(TArray<FString>& Lines);
};
