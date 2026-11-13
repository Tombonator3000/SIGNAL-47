#pragma once
#include "CoreMinimal.h"

class US47Main;
class USceneComponent;
class UStaticMesh;

// CHURCH-COLONIAL-20261113 (Tom, 13 November: "bytt til kolonikirka"). San Isidro in Caliche as a small Spanish colonial mission
// church, stood up from the bought Cosmos pack "Colonial Convent" (Tools/cosmosconvent; /Game/Licensed/CosmosConvent, Git-ignored)
// by the list in Config/Props/church_convent.json (Tools/cosmosconvent/layout.py writes it): a forecourt behind a low wall with a
// gate gap, a nave 12 by 9 m with the door under a bell gable, the altar and its screen, and a sacristy behind the altar wall with a
// back door 2 m from the cemetery gate.
//
// It takes the place of the white board church only when every mesh of the list is here; without the pack (or with -S47NoConvent)
// the old church stands as before, with its own walls, furniture and fixtures. Never both: S47CalicheAccess builds its room "Church"
// (the root, the floor, the front door with its use, sound and saved state, the room's lamp, the place for events) and leaves out
// the old shell, glass, interior and fixtures, their walls and furniture when this stands, and calls Build here instead.
//
// AGENTS.md, the rule of two routes: in at the front door or the back door; inside, round either end of the altar screen to the arch
// behind it, through the sacristy and out toward the cemetery; outside, round the church on both sides (the forecourt wall leaves a
// gap by each front corner). Out of sight: behind the altar screen, behind the sacristy's curtains, between the pews.
//
// The church's own metres: x east from the front wall's middle plane, up from the floor, z south of the axis.
namespace S47Convent
{
	struct FPiece { FString Id, Mesh; FVector At = FVector::ZeroVector; double Yaw = 0; FVector Scale = FVector::OneVector; bool bLand = false, bShadow = true; };
	struct FBoxPart { FString Id; FVector At = FVector::ZeroVector, Size = FVector::OneVector; uint32 Colour = 0x808080; bool bShadow = false, bLand = false; };
	struct FWordsL { FString Text; FVector At = FVector::ZeroVector; double Yaw = 180, Size = .14; bool bLand = false; }; // Yaw: the way the words face, Unreal degrees (180: west)
	struct FRectL { double X0 = 0, X1 = 0, Z0 = 0, Z1 = 0; FString Id; };
	struct FLightL { FString Id; FVector At = FVector::ZeroVector; float Lumens = 100; double Reach = 5; FLinearColor Colour = FLinearColor::White; bool bShadows = false, bNight = false; };
	struct FPlan
	{
		bool bRead = false; FString Error, MeshRoot, Room;
		double DoorWidth = 1.8, DoorHeight = 3.1; FVector2D Front = FVector2D::ZeroVector, Back = FVector2D(15, 0);
		double PadX0 = 0, PadX1 = 0, PadZ0 = 0, PadZ1 = 0, PadUnder = .02, PadBank = 3;
		TArray<FPiece> Pieces; TArray<FBoxPart> Boxes; TArray<FRectL> Walls, Blocks; TArray<FLightL> Lights; TArray<FWordsL> Words;
		FString AssetOf(const FPiece& P) const { return MeshRoot / P.Mesh + TEXT(".") + P.Mesh; }
	};
	/** Config/Props/church_convent.json, read once. */
	const FPlan& Plan();
	/** The list reads and every mesh it names is on disk (asked without a load), and -S47NoConvent is not given. */
	bool Present();
	/** S47CalicheAccess, while it builds the room "Church": the church at Root (the room's root, world Three metres, its floor there). */
	void Build(US47Main* Main, const FVector& Root, USceneComponent* Parent, int32 ParentNode, UStaticMesh* DoorLeaf);
	/** Every frame (S47CalicheAccess::Update): the back door's swing, the night lamps, the pieces on the land. */
	void Update(US47Main* Main);
	/** Built in this world. */
	bool Standing();
	bool BackOpen();
	/** bQuiet: put there at once, no sound (a load). */
	void SetBackOpen(bool bOpen, bool bQuiet);
	/** The road-local spot just outside the back door (the cemetery's path starts there): x of the wall's outer face, z of the axis. */
	FVector2D BackDoorRoad();

	// ---------- the pure rules (self-tested) ----------
	/** From the middle of the nave to the open ground outside, past the plan's walls and blocks, for a walker 0.3 m wide each side.
	 *  bFront, bBack: that door stands open. OutPocket: free floor inside the walls that cannot be reached from the nave. */
	bool WayOut(const FPlan& P, bool bFront, bool bBack, int32* OutPocket = nullptr);
	bool SelfTest(TArray<FString>& Lines);
}
