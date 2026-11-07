#pragma once
#include "CoreMinimal.h"

class AS47Web;

// Tom, 7 November: «må få motorveien til å passe bedre inn, med støv/jord mot vei, er kant mot naturen nå». Freeform only.
// A mesh decal along the highway, one draw for each stretch: a ribbon lying 2-3 cm over the asphalt, the shoulder and the verge, row for row
// on the exported strips (roadTerrain.ts highwayGeometry and vergeGeometry), so it draws only what it lies on: not the plants, not the truck.
// Its material is the ground's own graph (Tools/roadedge), so out on the land it is the same soil as under it and shows no edge,
// and in over the shoulder and the asphalt the same soil lies there. Pebbles and dry grass along the edge as cheap culled instances.
// Without /Game/RoadEdge, or with -S47RoadEdgeOff, nothing is built and the road stays as it was.
namespace S47RoadEdge
{
	/** Each reconcile: built once when the land is on; shown while it is. */
	void Reconcile(AS47Web* Web, bool bOn);
	void Report(const AS47Web* Web, TArray<FString>& Lines);
}
