#pragma once

#include "CoreMinimal.h"

class UWorld;
class UStaticMesh;

// SARO-CLIP-20261108: what goes into SARO's walls. The walls are a table here, taken from where the web version and the native
// buildings put them (ControlRoom.ts, Annex.ts, ServiceYard.ts's photo lab, S47Office's partitions, S47EastWing's constants), each a
// slab with its doors and windows cut out. Every drawn mesh in the buildings' region (the room dressing, the props, the furniture,
// whatever builder made it) is read vertex by vertex in world terms, and a vertex further into a slab than the tolerance counts.
// A wall's own faces lie on the slab's faces and never count. Nothing is moved or changed here; F10 "Looks (M6)" reads the list.
namespace S47SaroClip
{
 struct FHit
 {
  FString Thing;     // component, its web node if any, its mesh
  FString Wall;      // the slab's name
  int32 Count = 0;   // vertices inside
  double Depth = 0;  // the deepest, metres from the nearer face
  FBox Where = FBox(ForceInit); // the vertices inside, three metres
 };
 // Measure. Hits deeper than Tol (metres), the worst first. Lines: a line each, and a summary first. DumpDir: when not empty, every
 // measured mesh's vertices in world three metres go to saro-geo.bin with saro-geo.json beside it (for measuring again offline).
 SIGNAL47_API TArray<FHit> Run(UWorld* World, double Tol, TArray<FString>& Lines, const FString& DumpDir = FString());
 // The positions of a mesh's first LOD, in its own terms (cm): the render data when it keeps them, else the editor's source,
 // else the bounding box's corners (bExact false). Cached by mesh.
 SIGNAL47_API const TArray<FVector3f>& Vertices(UStaticMesh* Mesh, bool& bExact);
 // The slabs, for drawing or other checks: name, box in three metres.
 SIGNAL47_API void Walls(TArray<TPair<FString, FBox>>& Out);
}
