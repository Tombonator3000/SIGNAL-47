#pragma once
#include "CoreMinimal.h"

class AS47Web;

// Freeform's farm windmill and stock tank (critic 8 November: four smooth grey rods and a plain grey drum). A steel lattice
// tower with girts and crossed braces, a platform, a wheel of pitched blades with its tail, the pump rod down the middle,
// and a round tank of rusted corrugated tin with water in it. Built from the engine's cube and the farm buildings' own
// materials; Story keeps the old ones. Road-local metres, as S47FarmWorld.
namespace S47FarmWindmill
{
 // The cube and the farm's steel and tin are there. Without them the old windmill and tank stay.
 bool Available();
 // In the Story ranch's own metres (before Freeform's 310 m west): the old windmill's and tank's triangles, which go.
 bool Retired(const FVector& StoryThree);
 // Road-local footprints of the tower's feet, for the truck's obstacles (FENCES-INVISIBLE-20261108: the old tank's box stayed
 // where nothing stands any more).
 TArray<FBox2D> Footprints();
 // BLOCKERS-20261109: the round tank's, road-local: its middle (x, z) and radius. A square box round it stood out at the corners
 // (F10 "Looks (M6)": saro#346, 30 % of its edge bare); the truck's list has it as a circle.
 FVector3d TankRound();
 // Built once, then placed for the area and shown in Freeform only (as S47FarmWorld's parts).
 void Reconcile(AS47Web* W,bool bOn);
}
