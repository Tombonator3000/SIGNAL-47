#pragma once
#include "CoreMinimal.h"

// CALICHE-GROUND-20261112: ground that fits buildings. A site (S47TownSites, later any Caliche building) says where the road's land
// is to be level (a pad at a height, with a soft bank down or up to the land round it) and where it is to be open down to a floor (an
// areaway, a light well, the building's own hole). Road-local metres, heights in SurfaceY's terms.
// Who follows it: S47RoadTerrain::SurfaceY (the walker, the truck, props seated on the land, plants, the herd, the map: everything that
// asks the land's height) while Freeform's relief is on, and S47RoadTerrain::NaturalY is the land without it. The drawn land and its
// collision are baked meshes (Nanite: nothing to move at run time), so the same edits are cut into SM_FF_Ground_Road by
// Tools/freeformterrain/site_ground.py from the list the game writes (-S47GroundEditsDump), and Content/FreeformTerrain/site_ground.json
// says what the installed mesh holds. Baked(Id) is true when that is what the site registered: only then may a site leave its pad and
// banks to the land itself. With the relief off (Story's flat land, the web numbers' checks) nothing here changes any height.
namespace S47GroundEdits
{
	/** A rectangle on the land: its middle, half its size each way, turned Yaw degrees (clockwise from above, as a site is). */
	struct FRect { double X = 0, Z = 0, HalfX = 0, HalfZ = 0, Yaw = 0; };
	/** Level at Y inside R; over Bank metres outside it the land goes smoothly from Y to what it was. */
	struct FPad { FRect R; double Y = 0, Bank = 4; };
	/** No land inside R: whatever asks stands on Y (the floor down there). The baked mesh has a hole. */
	struct FCut { FRect R; double Y = 0; };

	/** A site's edits, replacing what it gave before. An empty pair takes them away. */
	SIGNAL47_API void Set(const FString& Id, const TArray<FPad>& Pads, const TArray<FCut>& Cuts);
	/** CHURCH: ground a site keeps bare of plants without moving the land (a graveyard on the slope as it lies). Only Clears follows it:
	 *  no height changes, nothing for the mesh tool, not among ClearBoxes. An empty list takes it away. */
	SIGNAL47_API void SetBare(const FString& Id, const TArray<FRect>& Bare);
	/** The land's height Y at a point with every edit on it (S47RoadTerrain::SurfaceY calls this on the desert's own branch). */
	SIGNAL47_API double Apply(double X, double Z, double Y);
	/** On a pad, a bank or a cut, or within Margin of one: no plant, rock or tuft belongs here. False while the relief is off. */
	SIGNAL47_API bool Clears(double X, double Z, double Margin = 0);
	/** The boxes round every pad with its bank and every cut (road-local), for whoever hides instances by box. */
	SIGNAL47_API TArray<FBox2D> ClearBoxes();
	/** The installed ground mesh holds exactly what this site registered (2 cm), and Freeform's land is the one in use. */
	SIGNAL47_API bool Baked(const FString& Id);
	/** Everything registered, as the JSON the mesh tool reads. */
	SIGNAL47_API FString Json();
	/** Distance from a point to a rectangle, 0 inside (for tests). */
	SIGNAL47_API double Outside(const FRect& R, double X, double Z);
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
