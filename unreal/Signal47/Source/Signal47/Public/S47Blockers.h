#pragma once

#include "CoreMinimal.h"

class US47Main;

// FENCES-INVISIBLE-20261108: what stops the player or the truck where nothing is drawn. In the area the player is in now: every enabled
// walk collider, every drive obstacle of the area's leg and every box the open land keeps out (US47World::LandOut). Each is matched
// against what is drawn there: a visible mesh (or an instance of one) whose box meets the blocker padded by Pad, or, for a mesh bigger
// than a building (ground, roads, merged export batches), one of its vertices standing over the ground inside it. What is left has
// nothing to show for itself. Nothing is changed here; F10 "Looks (M6)" lists them.
namespace S47Blockers
{
 struct FBlocker
 {
  FString Kind;  // walk, drive or land
  FString Id;    // the collider's id, or the obstacle's index
  double MinX = 0, MaxX = 0, MinZ = 0, MaxZ = 0; // three metres, world terms
  bool bCircle = false;
 };
 // The invisible ones, worst (largest) first. Lines: a summary and a line each. Skipped: blockers with no visible ground under them
 // (another area's, out of sight now).
 SIGNAL47_API TArray<FBlocker> Run(US47Main* M, double Pad, TArray<FString>& Lines);
}
