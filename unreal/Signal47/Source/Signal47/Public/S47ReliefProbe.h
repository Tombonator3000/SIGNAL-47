#pragma once
#include "CoreMinimal.h"

class US47Main;

// -S47MenuPointerRelief: Freeform's rolling land from an ordinary game started through the menu. Reads the table's zones,
// measures the relief meshes under the floor at thousands of points on live ground, then walks three short walks and drives
// one short off-road trip with a get-out in the -S47GetOut manner, then takes the same five views at 11:00, in the low sun and
// at night (Saved/AlbumTest/relief-*.png). PASS only if the relief is really on, every walk and the trip saw live ground,
// Void is 0, the floor is under 0.15 m off the meshes and the zones hold. Frame times per view go in the log.
namespace S47ReliefProbe
{
	void Begin(US47Main* Main);
	/** A step of the test's own ticker; false when it has finished and asked the game to exit. */
	bool Step(US47Main* Main, float Dt);
}
