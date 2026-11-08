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
 // Built once, then placed for the area and shown in Freeform only (as S47FarmWorld's parts).
 void Reconcile(AS47Web* W,bool bOn);
}
