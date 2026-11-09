#pragma once

#include "CoreMinimal.h"

class AS47Web;

// FENCES-INVISIBLE-20261108, the substation's realism: a fenced yard of the 1980s. Three strands of barbed wire on 45-degree arms
// outward from every fence post (the gate's leaves have none), and a second, smaller station-service transformer on its own
// concrete pad in the yard's free west strip, beside the main one. All drawn only: no collision (the yard is closed by the fence's
// collider in Config/Props/substation.json, now on the fence itself). The kit's site terms: Blender (x, y, z) is world three
// (7890 - x, -1.10 + z, 536.20 + y) (Config/Props/substation.json, S47Substation.cpp). -S47SubstationYardOff: as it was.
namespace S47SubstationYard
{
 SIGNAL47_API bool Build(AS47Web* W);
}
