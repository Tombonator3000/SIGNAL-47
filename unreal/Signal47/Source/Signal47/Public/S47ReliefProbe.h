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

	/** How far one gets (T16-01, Tom 7 November 2026: "kommer bare litt rundt gården"): from a road-local start in an area, straight
	 *  out one of eight ways (0 north, clockwise), on foot or at the wheel, with the game's own walker and a controller on the leg's
	 *  real area, until something stops it, and what. bRound: what stands in the way (a building, a post, a box) is gone round, as a
	 *  person would, and named; only the land itself ends the line. bNatural: it ended at a hill's foot or the rim's fence. */
	struct FReach
	{
		FString Line, Why, EndArea;
		bool bNatural = false;
		double Metres = 0, EndX = 0, EndZ = 0, Dx = 0, Dz = 0;
		int32 Crossed = 0, Round = 0, NoGround = 0;
	};
	SIGNAL47_API FReach Reach(US47Main* Main, const TCHAR* Name, const FString& Area, double RoadX, double RoadZ, int32 Dir, bool bDrive, bool bRound);
	SIGNAL47_API const TCHAR* ReachDir(int32 Dir);
}
