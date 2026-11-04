#pragma once
#include "CoreMinimal.h"
class AS47Web;
// Distant New Mexico mesas: swaps the Caliche CN5 backdrop (prop:streetGroundMesas, per-slot palette) and the
// legacy exported vertex-coloured mesa meshes ("mesas", "mesas:near")
// onto /Game/Licensed/Backdrop/M_S47_MesaRock (world-aligned photo sandstone x vertex colour).
// Areas load lazily, so Step rescans now and then. Without the licensed material nothing changes.
namespace S47MesaRock
{
 void Step(AS47Web* Web,double Dt);
}
