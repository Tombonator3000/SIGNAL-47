#pragma once

#include "CoreMinimal.h"

class US47Main;

// SAVE-SARO-20261108 (Tom, 8 November 2026): "Lagring kun på SARO, men alltid autolagring når man drar ut fra SARO." Freeform only;
// the Story keeps its own rules.
//
// The log is filed at the station: a manual save is only offered inside the SARO building (the control room, where OPS's rest is
// taken, the annex and Dale's office, the photo lab and the east wing). Every time the player leaves (out of the building into the
// yard, setting off in a vehicle, or out of SARO's area into another), one autosave is written into the case's autosave slots, the
// same rotating three every other autosave uses, so nothing the player saved by hand is written over. Never in the middle of a
// visit, a taking or anything else out there: it waits a little for a safe moment, and is let go if none comes.
namespace S47SaroSave
{
	/** In the SARO building, on foot: the rooms a manual save is offered in. */
	SIGNAL47_API bool InStation(const US47Main* M);
	/** On SARO's own ground, in the saro scene's x and z: the yard, the pad, the east wing, the visitors' parking and the first rows
	 *  of the array (not the farm or the motel, which share the scene). A truck left here has arrived at the station. */
	SIGNAL47_API bool OnStationGround(double X, double Z);
	/** What the save menu says anywhere else in Freeform, with the save not offered. */
	SIGNAL47_API FString StationOnly();
	/** What no save can carry (a visit on the ground, one at a door or a window, the wrong voice on the CB): a save in the middle of
	 *  it would come back without it. Empty when there is none. Every Freeform save waits for it (US47Flow::FreeformSaveBlock). */
	SIGNAL47_API FString Unsaved(const US47Main* M);
	/** Why this is no moment to file a departure: anything out there now that is more than a light in the sky, saved or not (a taking,
	 *  a visit, bodies out, the light over the road, a presence or a danger holding the night's ground). Empty when it is safe. */
	SIGNAL47_API FString Danger(const US47Main* M);
}

// The rule for one departure, as plain state, so that the self test needs no world. US47Flow feeds it each frame and writes the save.
//
// Armed while the player is inside, and when they arrive (the truck parked at SARO). Out of the building on foot (Door), into a
// vehicle (Drive, from US47World::StartTrip before the trip begins) or on into another area (Area): one departure, and it is no
// longer armed until the player is back inside or arrives again. So the walk out to the truck, the drive off and the edge of the
// area are one departure, filed once, at the first of them. Never two departure saves within Spacing seconds of play: a step out
// and back at the door is not a second departure. One that cannot be written at once waits up to MaxWait seconds, while the player
// stays on foot in that area within MaxDrift metres; back inside before then, nobody left.
class SIGNAL47_API FS47SaroDeparture
{
public:
	enum class EHow : uint8 { None, Door, Drive, Area };
	struct FIn
	{
		bool bInside = false;      // S47SaroSave::InStation
		bool bDriving = false;
		bool bTruckAtSaro = false; // the vehicle stands parked at SARO (US47World::TruckAt)
		FString Area;              // the area loaded now
		FVector2D At = FVector2D::ZeroVector; // the player's x and z
		double Dt = 0;             // seconds of play since the last look (none while the menu holds the game)
	};
	static constexpr double Spacing = 60, MaxWait = 45, MaxDrift = 60;

	/** A new session or a Continue: nothing armed, nothing waiting, the first departure is never spaced out. */
	void Reset();
	/** Each frame: a new departure, or None. Ages the waiting one, and lets it go if the player is back inside. */
	EHow Look(const FIn& In);
	/** The player sets off in a vehicle now: a new departure if armed, else the one waiting (its last chance), else None. */
	EHow Drive(const FIn& In);
	bool Pending() const { return How != EHow::None; }
	EHow PendingHow() const { return How; }
	/** The waiting one has waited too long, or the player has gone on (driving, another area, too far). */
	bool Expired(const FIn& In) const;
	/** Written: the next one waits its Spacing. */
	void Filed();
	/** Not written: let go. */
	void Drop();
	/** F10: armed, the last departure long past. */
	void Arm() { bArmed = true; Since = Spacing; }
	bool Armed() const { return bArmed; }
	/** True once after a departure that came within Spacing of the last one and was not filed again. */
	bool TakeSpaced() { const bool b = bSpaced; bSpaced = false; return b; }
	static const TCHAR* Name(EHow H);
	static bool SelfTest(TArray<FString>& Lines);

private:
	bool bArmed = false, bHaveLast = false, bSpaced = false, bWasInside = false, bWasDriving = false;
	FString LastArea, FromArea;
	EHow How = EHow::None;
	double Age = 0, Since = 1e9;
	FVector2D From = FVector2D::ZeroVector;
	EHow Begin(EHow H, const FIn& In);
};
