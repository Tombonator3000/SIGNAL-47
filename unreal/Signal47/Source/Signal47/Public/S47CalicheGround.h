#pragma once

#include "CoreMinimal.h"

class AS47Web;

// CALICHE-REAL-20261108, first part: do Caliche's buildings sit in the ground? Each native building shell under the town (the
// production buildings, the homes) is measured at its four footprint corners and four edge middles: the shell's bounding bottom there (its
// wall foot) against the ground 30 cm outside it (the first thing straight down that is not a building, a plant or a lamp). Positive: the corner floats.
// Where a wall floats by 3 cm to 1.5 m, a concrete stem wall (grunnmur) is set under that edge, inside the wall line, from under the
// ground up to the wall's foot, as a building on a slope has. The export's merged batches (several buildings in one mesh) are not
// measured per building; M4's temp: boxes and S47TownSites' parts are listed, never changed. -S47CalicheGroundOff: no stem walls.
namespace S47CalicheGround
{
 // After the town, the homes and the land under them are built.
 SIGNAL47_API void Build(AS47Web* W);
 // The measure as it stands now. Lines: a summary first, then a line a building and the listed boxes. Returns floating buildings.
 // WorstAt: the worst floating sample's ground point outside the wall (three metres), if any floats.
 SIGNAL47_API int32 Measure(AS47Web* W, TArray<FString>& Lines, FVector* WorstAt = nullptr);
}
