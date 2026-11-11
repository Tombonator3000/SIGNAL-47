#pragma once
#include "CoreMinimal.h"
class US47Main;
// WORLD-04 (N1110): SARO walls gone before Rest and back right after, cause not pinned down by reading. This writes what the
// office room, its corridor finishes and the records partition are (mesh, triangles, visibility flags, render state, bounds)
// before and after every Rest, and F10 "SARO wall check" writes the same on demand. A change across Rest is logged as a warning.
namespace S47SaroWallWatch
{
 void Mark(US47Main* Main, const TCHAR* Phase);   // snapshot; after a "before" mark, an "after" mark compares to it
 FString Check(US47Main* Main);                   // F10: snapshot now, the verdict as one line
 bool SelfTest(FString& Why);                     // the comparison, on made-up snapshots
}
