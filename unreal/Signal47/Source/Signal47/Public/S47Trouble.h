#pragma once

#include "CoreMinimal.h"

struct FS47Truck;
class FS47DriveController;
class FS47EngineSound;

// drive/trouble.ts: trouble with the truck (Tom, 16 October): an engine that misfires or dies on
// the road, and instrument lights and headlights that flicker or go out. Made for the story's
// events and for strange things that happen at random on a night drive. US47World steps it every
// frame of a trip with the truck that is being driven, and it only touches the truck while
// something is wrong; afterwards the lights, the instruments and the engine are given back.
//
//   sputter  the engine misfires for a second or two: the pull comes and goes, it coughs.
//   stall    the engine dies: the truck coasts, OIL and VOLTS light up, the headlights sag on
//            the battery. Then the key: the starter turns it over (the lights dip with it), it
//            may take two tries, and it catches. hold: it stays dead until Release(); key: the
//            player turns the key (E) instead of the driver doing it; tries: the turn it catches
//            on (2: the first turn fails; infinite: it never catches).
//   flicker  a bad contact: dash and headlights stutter for a moment.
//   blink    the same contact, once; or with a count, every light out and back evenly that many
//            times, a blink every `per` seconds.
//   dashOut  the instrument lights go dark; the fuel and temperature needles fall back.
//
// Random trouble is off until the story turns it on (bRandom).
enum class ES47Trouble : uint8 { None, Sputter, Stall, Flicker, Blink, DashOut };
SIGNAL47_API const TCHAR* S47TroubleName(ES47Trouble K);

struct FS47TroubleOpts
{
	TOptional<double> Len;
	bool bHold = false, bKey = false;
	/** stall: the turn of the key it catches on (1 or 2 at random by default; infinite: never). */
	TOptional<double> Tries;
	/** blink: this many even blinks, every light out and back (a count, not a bad contact). */
	TOptional<double> Count;
	/** blink with a count: seconds from one blink to the next (half of it dark). */
	TOptional<double> Per;
	/** blink with a count: soft, the lights dip and come back smoothly (slow fades). */
	bool bSoft = false;
	/** stall: already dead when it begins (no dying, no sound of it). */
	bool bDead = false;
};

class SIGNAL47_API FS47TruckTrouble
{
public:
	FS47TruckTrouble();
	/** Trouble at random on drives (the story or the developer menu turns it on). */
	bool bRandom = false;
	/** About how many seconds of driving between random troubles. */
	double Every = 300;
	/** Each trouble as it starts (the story, tests). */
	TFunction<void(ES47Trouble)> OnTrouble;
	/** How many troubles have started, and how many times the key has been turned in this stall. */
	int32 Count = 0;
	int32 Turns = 0;
	/** Everything electric in the truck, 0..1, on top of whatever the trouble does (story/truckTaken.ts). */
	double Dim = 1;

	/** What is wrong now (None when nothing). */
	ES47Trouble Kind() const { return A ? A->Kind : ES47Trouble::None; }
	/** The engine is dead and waits for the player to turn the key (Use). */
	bool WantsKey() const;
	/** What the radio gets, 0..1: nothing while the starter turns, a stutter with the lights. */
	double RadioLevel() const { return Power * Dim; }
	/** The engine is dead (or being started). */
	bool Dead() const;
	/** What a stall is doing now ('dying', 'dead', 'pause', 'cranking', 'catching'), or empty. */
	FString Stage() const;
	/** A counted blink: how many of its blinks have begun so far. */
	int32 Blinks() const;
	/** A counted blink: every light is out right now. */
	bool BlinkDark() const;

	/** Start a trouble now. A running one is replaced, except a stall, which only Release() or the key ends. */
	bool Start(ES47Trouble InKind, const FS47TroubleOpts& O = FS47TroubleOpts());
	/** A held stall may be started again (the story lets go of it). */
	void Release();
	/** The turn of the key this stall catches on, from now. */
	void CatchOn(double N);
	/** The player turns the key (when WantsKey). */
	bool TurnKey();
	/** Everything as it was: the engine runs, the lights and the instruments are the truck's own. */
	void Reset(FS47Truck* Truck, FS47DriveController* Drive);
	/** The truck goes on in another area: the copy left behind gets its own lights and engine back. */
	void HandOver(FS47Truck& Truck, FS47DriveController& Drive);
	/** Every frame of a trip, after the drive has moved. bMayRandom: whether random trouble may come now. */
	void Step(double Dt, FS47DriveController& Drive, FS47Truck& Truck, FS47EngineSound* Sound, bool bMayRandom);

private:
	enum class EStage : uint8 { None, Dying, Dead, Pause, Cranking, Catching };
	struct FActive
	{
		ES47Trouble Kind = ES47Trouble::None;
		double T = 0, Len = 0;
		EStage Stage = EStage::None;
		double St = 0;
		bool bHold = false, bKey = false;
		double Tries = 0, Need = 1;
		double Count = 0, Per = 0.5;
		bool bSoft = false;
		EStage Heard = EStage::None;
		double Beat = 0, V = 1;
	};
	TOptional<FActive> A;
	double Next;
	bool bTouched = false;   // the truck has been changed and must be given back
	double Power = 1;
	struct FOut { double Light = 1, Dash = 1; bool bOil = false, bVolts = false; };
	FOut StallStep(double Dt, FActive& Act, FS47DriveController& Drive, FS47EngineSound* Sound);
};
