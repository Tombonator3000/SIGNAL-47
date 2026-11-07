#pragma once

#include "CoreMinimal.h"

class US47Main;

// Fifteen years of night shifts: the rooms' lived-in dressing (T16-15). New floor, cable trays, racks, paper, coffee, ash and
// a jacket, as room-origin meshes under /Game/Lived that the room's own node carries, so nothing the game uses is touched. With no asset, the room as it was.
// -S47LivedOff shows the room as it was, for before and after from one build.
namespace S47Lived
{
 void Update(US47Main* Main,double Dt);
}
