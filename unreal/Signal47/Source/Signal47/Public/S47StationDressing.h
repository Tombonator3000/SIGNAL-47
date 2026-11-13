#pragma once

#include "CoreMinimal.h"

class US47Main;
class UStaticMeshComponent;

// GAS-STATION-LIFT-01 (Tom, 11 November: lift the Caliche gas station): bought Cosmos props in the station's shop, a register, a
// coffee machine, a fire extinguisher, a bin, boxes, a vending machine, a vent and the like, to make the room denser and more real.
// No new gameplay: they only stand there.
//
// The meshes are bought (Tools/cosmosshop): they lie under /Game/Licensed/CosmosShop, which Git ignores, and are loaded only if
// present. Without them the shop is exactly as before, with one log line saying how many are missing. Where each stands is data:
// Config/Props/gas_cosmos.json (the shop's town metres for the bottom centre of each piece, the station model's origin, yaw, pitch,
// roll, scale, an optional walk block), read when the
// station is built, so placements are tuned without a rebuild (S47Props leaves the file to this class).
//
// The pieces hang under the station's own mesh (AS47Props::PlacedMesh("gasStation")) in its model terms, the way the canopy's
// insects do, so they move with it. Shown only in Freeform (Story uses the Freeform world too); a piece marked "collide" adds a walk
// block (the road area's colliders, as a prop's "collide" does) while shown.
class SIGNAL47_API FS47StationDressing
{
public:
	explicit FS47StationDressing(US47Main* Main) : M(Main) {}

	/** Every frame. Builds the pieces the first time the station is there in Freeform; shows them only in Freeform. */
	void Step();

	/** One placement as the file has it. */
	struct FPiece
	{
		FString Id, Asset;
		FString Model; // PROPS-REUSE-20261113: the shared catalogue id the asset came from ("model"), empty for a path ("asset")
		FVector Town = FVector::ZeroVector; // three metres in the town's terms (x, y up, z), where the piece's pivot goes
		double Yaw = 0, Pitch = 0, Roll = 0, Scale = 1;
		bool bCollide = false, bPivot = false;
		FString AnchorOf; // another piece whose bounds place this one (a drawer with its register)
		// PROPS-REUSE-20261113: the old low-poly stand-in this piece replaces, as a part of the station model of its own
		// (/Game/Free/Town/gas_station/...): shown only while the piece's model is not here. bWaitForStandin: the piece waits until
		// that part exists (until then the stand-in is baked into the station mesh and the two would overlap).
		FString Standin;
		bool bWaitForStandin = false;
	};
	/** Config/Props/gas_cosmos.json read: the station model's origin in town metres and the pieces. False when it does not read. */
	static bool ReadFile(FVector& Origin, TArray<FPiece>& Out, FString* Error = nullptr);
	/** The file reads; every piece has an id, a licensed asset path, a sane scale and stands inside the shop off the game's card patch
	 *  and the attendant's place; and how many of the assets are here (passes with and without them). */
	static bool SelfTest(TArray<FString>& Lines);

	/** PROPS-REUSE-20261113: what stands for a piece: its real model, or its old stand-in (never both). */
	struct FUse { bool bModel = false, bStandin = false; };
	static FUse Decide(bool bModelHere, bool bHasStandin, bool bStandinHere, bool bWait);

	int32 Shown() const { return Comps.Num(); }

private:
	US47Main* M = nullptr;
	TWeakObjectPtr<UStaticMeshComponent> BuiltFor;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> Comps;
	bool bVisible = false;
	int32 Builds = 0;
	void Build(UStaticMeshComponent* Station);
	void SetVisible(bool bOn);
};
