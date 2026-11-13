#pragma once
#include "CoreMinimal.h"

class US47Main;
struct FS47Rect;

// GAS-BAY-DOOR-20261113 (Tom, 13 November, after d65: "Verksteddør på bensinstasjonen må kunne åpnes og lukkes (som på SARO)").
// The service bay's roll-up door at the Caliche gas station, raised and lowered by hand, as the east wing's garage door at SARO
// rolls (FS47EastWing: the curtain shrinks toward its top into the hood, the walk's lock follows it, the state is saved).
//
// The door is not a part of its own in Tom's model (~/Assets/signal47-free/own/gas_station, gas_station.py "the bay: the roll-up
// door"): its two slat faces are in gas_station_Station with the rest, on the one material slot M_RollUp, which nothing else uses.
// So that slot is drawn invisible and a leaf of the same material is drawn in code over the opening, with its bottom bar and handle
// (a hair bigger than the baked ones, so shut it looks as it did) and a pull strap that hangs from the bar while it is up. The
// baked bottom bar, handle, hasp and padlock (M_Galv, M_SteelDark, M_Brass, shared with other parts) cannot be taken out without
// the model: raised, they stay at the foot of the opening. A model change (the door, bar, handle, hasp and padlock as a part of
// their own, "RollUp", as "Door" is) would take them with it.
//
// Walk: the bay's east wall (one of the station's blocks, prop:gasStation:*) is cut in three, the opening's piece is door:gasBay,
// shut while the door is less than .85 up; the old export's stoop in front (walk only) is opened and the dock in front of the
// door is a floor of its own (0.45 m over the apron: a step). The bay itself has always been walkable through the shop's door.
// State: in the Caliche access doors of the save (S47CalicheAccess, "gasBay"); a new game has it shut.
namespace S47GasBayDoor
{
	/** Every frame (S47CalicheAccess::Update): built once the station model is placed, then the door moves. */
	void Update(US47Main* Main);
	bool IsOpen();
	/** Raised (or on its way up). bInstant: put there at once, no sound (a load). */
	void SetOpen(bool bOpen, bool bInstant);
	bool Built();

	// ---------- the pure rules (self-tested) ----------
	/** The model's numbers, in the station's own metres (its pivot, gas_station.py: RZ0, RZ1, RY0, RY1 and BFX less the pivot). */
	inline constexpr double OpenZ0 = 16.2925, OpenZ1 = 19.8925, Bottom = 2.208, Top = 5.308, Face = -0.719;
	inline constexpr double Travel = 2.4;   // seconds each way by hand
	inline constexpr double WalkAt = .85;   // up this much, one walks under it
	/** How tall the leaf still hangs when it is K up (0 shut, 1 up): it rolls into the hood, a hand's breadth left. */
	double LeafHeight(double K);
	bool Blocks(double K);
	/** The bay's east wall cut round the opening (Z0..Z1): its south and north pieces and the door's. False: not across it. */
	bool SplitWall(const FS47Rect& Wall, double Z0, double Z1, FS47Rect& OutSouth, FS47Rect& OutDoor, FS47Rect& OutNorth);
	bool SelfTest(TArray<FString>& Lines);
}
