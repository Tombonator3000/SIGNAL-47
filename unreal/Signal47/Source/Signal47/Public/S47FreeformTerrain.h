#pragma once
#include "CoreMinimal.h"

class AS47Web;
class US47Main;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;

// Freeform's rolling land in the scene: the two relief meshes on the road's and SARO's ground (nodes 1242 and 7), and
// everything with a fixed height lifted to stand on them (S47Relief.h has the numbers). Story never sees any of it. If the
// table, the meshes or the material are not installed the land stays flat, with one line in the log.
// The hills too: SM_FF_Range on the mesas' own component (prop:streetGroundMesas) while the relief is on. Without it, or
// with -S47RangeOff, the mesas stay; Tools/freeformterrain/build_range.py has how it is made.
namespace S47FreeformTerrain
{
	/** Follows the session's mode; does nothing while the answer stays the same, so it can be asked every frame.
	 *  Call it before anything that places things on the ground for the new mode. True while the relief is on. */
	bool Reconcile(US47Main* Main);
	bool IsOn(const AS47Web* Web);
	/** One of the two relief meshes now on this world's ground (S47SaroSeam's and S47RadioWoman's checks accept it). */
	bool IsRelief(const AS47Web* Web, const UStaticMesh* Mesh);
	/** CALICHE-GROUND: a site gave the land new pads or cuts (S47GroundEdits): the old soil tiles over them go unseen. */
	void SitesChanged(AS47Web* Web);
	const TCHAR* RoadMeshPath();
	/** The ground's or the range's material is ours for the time being: S47MesaRock leaves this component alone. */
	bool HoldsMaterial(const AS47Web* Web, const UStaticMeshComponent* Component);
	/** The relief meshes' material while it is on the ground, for S47MesaRock's day tone. */
	UMaterialInstanceDynamic* GroundMid(const AS47Web* Web);
	/** The range's material while it stands, for the same day tone at its foot. Null with the mesas. */
	UMaterialInstanceDynamic* RangeMid(const AS47Web* Web);
	UMaterialInstanceDynamic* CorridorMid(const AS47Web* Web);
	/** What moved and what could not, one line each. */
	void Report(const AS47Web* Web, TArray<FString>& Lines);
}
