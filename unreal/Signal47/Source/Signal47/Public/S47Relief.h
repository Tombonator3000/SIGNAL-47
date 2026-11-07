#pragma once
#include "CoreMinimal.h"

// Freeform's rolling land: a height table baked by the map tools (Content/FreeformTerrain/relief.bin and relief.json), in
// road-local metres. Off is what SurfaceY's desert grid and SaroLand add to the ground. It is 0 until SetActive(true), and
// the world is always built with it off: STATION_DY, the diner, the motel and every placement read against the web version's
// numbers (S47Drive's self-test, S47CalicheNature) stay what they were. A missing or corrupt table means 0 everywhere.
namespace S47Relief
{
	/** Metres added to the ground at this road-local point: 0 while not active, off the table, or on a masked sample. */
	SIGNAL47_API double Off(double RoadX, double RoadZ);
	/** The same whether active or not, for what has to measure the table before it goes live. */
	SIGNAL47_API double Sample(double RoadX, double RoadZ);
	/** Turns it on only if the table is good; off is always honoured. */
	SIGNAL47_API void SetActive(bool bOn);
	SIGNAL47_API bool Active();
	/** Loads the table on the first call, once. False if it is missing or does not pass its checks. */
	SIGNAL47_API bool Ready();
	/** Where the table has to be 0 because nothing else follows it: SARO's and the station's patches of the road's land, the
	 *  site and array, the farm, the edges of the road strips. One line each with the largest value found. True if all hold. */
	SIGNAL47_API bool CheckZones(TArray<FString>& Lines);
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);

	/** How far Freeform's hills (SM_FF_Range, range_foot.bin) stand over the road's land at this road-local point, in metres, less than
	 *  0 where they lie under it. 0 while the relief is off or no table is installed; past the table's edge the hills go on, and it says
	 *  so with a large number. The range has no collision: this is what a hill is to whatever moves over the land. */
	SIGNAL47_API double HillAbove(double RoadX, double RoadZ);
	/** What the ground under a walker, a truck or a plant gains from the range: HillAbove from 0 up to where the table ends, 0 past its
	 *  edge. A fan is climbed, not a wall in front of a slope one can see. The range has no collision. Only SurfaceY's desert uses this. */
	SIGNAL47_API double HillLift(double RoadX, double RoadZ);
	/** A person's or a truck's footprint, as a square this far each way, is on what cannot be climbed: steeper than HillWalkSlope, or
	 *  more than HillWalkTopM up (US47World::GroundUnder finds the mesh only within 6 m under the floor). */
	SIGNAL47_API bool HillStops(double RoadX, double RoadZ, double Reach);
	inline constexpr double HillWalkSlope = .27, HillWalkTopM = 5;

	/** Off for as long as this lives, for a check that compares against fixed web numbers while a game may be running. */
	struct SIGNAL47_API FSuspend
	{
		FSuspend();
		~FSuspend();
		FSuspend(const FSuspend&) = delete;
		FSuspend& operator=(const FSuspend&) = delete;
	private:
		bool bWas;
	};
}
