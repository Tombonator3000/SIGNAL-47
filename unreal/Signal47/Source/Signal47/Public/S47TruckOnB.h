#pragma once

#include "CoreMinimal.h"

class FS47TruckTrouble;
class FS47Receiver;
struct FS47TownSeen;

// web/src/story/truckOnB.ts. Four, and not yet seven: the truck on B (FORSLAG.md, 20 October, item 8).
// A ladder towards 05:29 in the truck, for those who drive everywhere: the signs in the car (HENDELSER.md,
// section 5) as a count that goes up, on the one line the truck can cross. B crosses the highway at
// SARO's turn-off, 23 m south of the site (story/lines.ts, CROSSINGS).
//
//   four    from chapter two, the first time the truck goes over B: the radio drops to a bare carrier for
//           a second (switched off, it gives one anyway), the engine misses, and every light in the
//           truck goes out four times, evenly, a relay ticking in the dash with each.
//   seven   once room 6 is over, the next time over B: the engine dies, OIL and VOLTS, the radio on its
//           carrier. The key: the first turn does not catch, the second does, and then every light goes
//           out seven times. From the motel's lot it comes as the truck pulls out onto the highway.
//   compass the ball compass on the instrument hood reads 026 over B, from chapter two; on the old road
//           (chapter six) it slides towards 026 from mile 6, and is on it before the line.
//
// One thing at a time: never on top of another thing (then the crossing is missed and the next one tries
// again), and nothing starts while they run or for TAIL seconds after. Four is a strong sign; seven is a
// peak. No saving while it runs. Kept in the town's part of the case ('truckB:4', 'truckB:7', and a mark
// on B on the notebook's map).

struct FS47TruckOnBIn
{
	/** In play, no panel, no cinematic, and nothing else of the night's going (one at a time). */
	bool bOk = false;
	/** Driving on a trip, the story's or the player's own (not a free drive, not chapter six's road). */
	bool bTrip = false;
	/** The driven truck on the map (road-local metres), or unset where the area has no place on it. */
	TOptional<FVector2D> At;
	/** The truck's speed, m/s. */
	double Speed = 0;
	/** Chapters two to five: B has the compass, and the ladder may begin. */
	bool bOpen = false;
	/** Room 6 is over: the next rung is seven. */
	bool bAfter6 = false;
	double Clock = 0;
	/** On chapter six's old road before the line: miles from the diner; else unset. */
	TOptional<double> OldMi;
	/** The trip (World.tripNow): where it began and the metres driven so far; empty off a trip. */
	FString From;
	double Metres = 0;
	/** Settings, Flashing images: slow fades (the blinks dip softly instead of going out). */
	bool bCalm = false;
};

struct FS47TruckOnBDeps
{
	FS47TruckTrouble* Trouble = nullptr;
	/** The truck's radio (main.ts, carRadio). */
	TFunction<FS47Receiver*()> Radio;
	/** The town's part of the case (main.ts, townSeen). */
	FS47TownSeen* Seen = nullptr;
	TFunction<void(const FString&)> Note;
	TFunction<void(const FString&, float)> Toast;
	/** A relay in the dash, with each blink. */
	TFunction<void()> Tick;
	TFunction<void()> Save;
};

class SIGNAL47_API FS47TruckOnB
{
public:
	explicit FS47TruckOnB(FS47TruckOnBDeps InD) : D(MoveTemp(InD)) {}

	static constexpr double BEARING = 26;

	/** 'idle' | 'four' | 'stall' | 'seven' | 'tail' */
	FString Phase = TEXT("idle");
	/** Seconds into the phase. */
	double T = 0;
	/** Crossings of B that came while something else was going (the tests): missed, tried again. */
	int32 Missed = 0;
	/** Crossings of B counted (the tests). */
	int32 Crossings = 0;
	/** The blinks of the rung that is running, as the dash showed them (the tests). */
	int32 Blinked = 0;
	/** The truck's distance from B now (m), or unset off the map. */
	TOptional<double> Dist;
	/** The compass's pull now: towards 026, by k (Truck.compassPull), or unset. */
	struct FPull { double Deg = 0, K = 0; };
	TOptional<FPull> Pull;
	/** Stands aside: no rung begins (the compass still reads 026 over B). */
	bool bOff = false;

	/** Which rungs the night has had: four, seven. */
	bool Four() const;
	bool Seven() const;
	/** Something of it is going (one at a time). */
	bool Busy() const { return Phase != TEXT("idle"); }
	/** A rung is running: no saving. */
	bool Racing() const { return Phase == TEXT("four") || Phase == TEXT("stall") || Phase == TEXT("seven"); }
	/** The stall and its seven: the peak's own thing. */
	bool Peaking() const { return Phase == TEXT("stall") || Phase == TEXT("seven"); }
	/** A rung is due on the next pass over B as the night stands (story/truckTaken.ts). */
	bool Due(const FS47TruckOnBIn& S) const;

	void Update(double Dt, const FS47TruckOnBIn& S);
	/** A rung now ('four' | 'stall'), as the crossing begins one (the tests). */
	void BeginNow(const FString& Rung, const FS47TruckOnBIn& S, const FString& InWhere = TEXT("turnoff")) { Begin(Rung, S, InWhere); }
	/** A new night, or a save opened: nothing running (the rungs the night has had are in its keys). */
	void Reset();
	/** One line for the debug overlay. */
	FString Text() const;

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47TruckOnBDeps D;
	bool bArmed = false;
	bool bRadioWasOn = false;
	bool bFailed = false;
	/** Where a rung's script is (0 at its start), and when the stall caught. */
	int32 Step = 0;
	double CaughtAt = 0;
	/** The clock the rung began at (the note's time). */
	double At = 0;
	bool bToldOld = false;
	/** Where the seven came: over B at the turn-off, or pulling out of the motel. */
	FString Where = TEXT("turnoff");
	/** The motel's lot this drive: tried while something else was going (counted missed once). */
	bool bMotelMissed = false;
	/** Slow fades, as the rung began. */
	bool bCalm = false;

	FS47Receiver* Radio() const { return D.Radio ? D.Radio() : nullptr; }
	bool OutOfMotel(const FS47TruckOnBIn& S) const;
	void Begin(const FString& Rung, const FS47TruckOnBIn& S, const FString& InWhere);
	void Run(const FS47TruckOnBIn& S);
	void Done(const FString& Rung, double Clock);
	void Abandon(double Clock);
};
