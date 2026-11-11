#pragma once

#include "CoreMinimal.h"

class AS47Web;

// D45-20261110 (D45-14, Tom in d45: the fence at SARO is to be the better wired fence from Cosmos, MilitaryBarriers): the chain-link
// fence between SARO's windows side and the array (Exterior.ts fence(), FENCE_Z -20) is built from the licensed Military Barriers kit
// (Tools/militarybarriers: migrated to UE 5.8 into Content/Licensed/MilitaryBarriers, which Git ignores). The export's three unlit
// planes go (found by their parent, place and size, never by their index) and stay hidden through every area change (node level); the
// old 5 cm posts are cut out of the pole batch (node 35, g26) with the street lamps' cut. In their place: the kit's panels, a post every
// 2.5 m (the panel's own), a bare post closing each run, three barbed strands along the top arms (no razor spiral: an observatory, not
// a base), all lit PBR (SARO's power does not dim them as it dimmed ext.paint's unlit planes), seated on the ground measured under each
// panel, without collision (the walk and the truck keep the 2D colliders, S47World). The walk gate's leaf (world.field.gateHinge) draws
// the kit's gate leaf, a child of the hinge, so it swings with the story; the latch and its chain stay. With the kit not imported, or
// not as Tools/militarybarriers/verification.json measured it, nothing changes. -S47ArrayFenceOld (or -S47PropsOff): the old fence.
namespace S47ArrayFence
{
 // After S47StreetLamps::Build (both cut the same pole batch). True: the new fence stands.
 SIGNAL47_API bool Build(AS47Web* W);
 // Panels of Module metres along a run of Run metres: Count of them (at least one), each stretched along its length by Stretch (1: as
 // modelled), so that Count x Module x Stretch is the run.
 SIGNAL47_API void Layout(double Run, double Module, int32& Count, double& Stretch);
 // -S47SelfTest: the layout for the three runs (counts, stretch within 10 %) at panel lengths 2 to 3.6 m and at the kit's own when it
 // is imported, and the kit's frame as the runtime needs it.
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
 // The fence as built: the old planes and gate leaf hidden, every old post cut, the panels per run, every panel's foot within 3 cm of
 // the ground drawn under it (when it is drawn here), the gate leaf a child of the hinge and where it was put on it. Lines: a summary
 // first, then a line a thing. Returns how many are wrong (0 and a SKIP line when the fence was not built).
 SIGNAL47_API int32 Audit(AS47Web* W, TArray<FString>& Lines);
}
