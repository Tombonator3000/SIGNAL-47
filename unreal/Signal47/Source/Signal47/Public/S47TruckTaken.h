#pragma once

#include "CoreMinimal.h"

class FS47TruckTrouble;
class FS47Receiver;
class US47Audio;
struct FS47TownSeen;

// web/src/story/truckTaken.ts. Taken in the truck (FORSLAG.md, 20 October, item 10; HENDELSER.md,
// section 8: "alone, outdoors or in the car"). B crosses the highway at SARO's turn-off (story/lines.ts,
// CROSSINGS: 23 m south of the site): here, on B, southbound.
//
// When the night's curve is at its peak, the time away could come (story/abduction.ts, MayBegin) and
// nothing else of the night's is going, the truck going south over B gets the warning in the cab: the
// same 47 seconds as on foot, the corner clock in amber seconds, and the truck has the beats:
//
//   :00  the radio slides off its station under the static and comes out as the bare carrier; the
//        engine dies under the player and the headlights sink onto the battery, OIL and VOLTS; the
//        truck slows to a stop as if something had hold of it. "Turn the key."
//   :16  the beep, and the engine's note and the tyres drop out: no sound of the truck at all.
//   :31  the pressure; the headlights and the dash go down with the battery.
//   :40  every light in the truck out, the radio with them, with the click K3 made. The key does
//        nothing now: not even the starter.
//   :41  three knocks on the roof of the cab, slow, from above (if the night still owes them, these
//        are them).
//   :47  the light from above, through the windshield, and the taking (story/abduction.ts).
//
// The way out is the truck's: the key (the third turn catches, if it is begun before the beep at :16;
// the night's second warning in the truck, once the key has got it away, it never catches), or out
// (stopped, Get out with Q: out of the truck and RUN metres away from it before the light).
//
// Taken, the player wakes 17 to 47 minutes later at the wheel 300 m further on, in the southbound lane
// with the nose to the north: the engine off, every light off, the radio on 1090, frost on the
// windshield and on the asphalt round the truck; one of them is on the bonnet as the head comes up; then
// the instruments come on, and the odometer reads 31.4 miles more than it did at SARO.
//
// The truck on B (story/truckOnB.ts) has the same crossing: a rung of its ladder that is due takes the
// pass. One at a time otherwise. Chapters one to four, at night; not with Missing time turned off. No
// saving from the crossing to the waking. At most twice in a night, and taken once. Kept in the town's
// part of the case ('tt:try@', 'tt:taken@', 'tt:key@', 'tt:ran@') and with the times away.

struct FS47TruckTakenIn
{
	/** In play, no panel, no cinematic, chapters one to four, at night, nothing else of the night's
	 *  going, no trouble with the truck, no call on the line. */
	bool bOk = false;
	/** The night's curve is at its peak and lets the time away come (and is not standing aside). */
	bool bMay = false;
	/** The time away may come as far as it goes itself (abduction.ts, MayBegin). */
	bool bAllowed = false;
	/** Missing time on. */
	bool bEnabled = true;
	/** Driving on a trip (the story's or the player's own) in SARO's area. */
	bool bTrip = false;
	/** The driven truck on the map (road-local metres), or unset. */
	TOptional<FVector2D> At;
	double Speed = 0;
	/** How far the truck faces south (its forward's z on the map, -1 to 1). */
	double South = 0;
	/** A rung of the truck on B is due on a pass over B now (story/truckOnB.ts, Due). */
	bool bDue = false;
	/** The time away: its stage, and the warning's seconds (unset outside it). */
	FString Stage = TEXT("calm");
	TOptional<double> WarnT;
	/** At the wheel now (else on foot), how far from the truck the player is (m), and how far north of it (m). */
	bool bInCab = false;
	double Away = 0;
	double North = 0;
	double Clock = 0;
	/** Whether the time away has someone over the player on waking now (abduction.ts, OverPhase). */
	bool bOver = false;
};

struct FS47TruckTakenDeps
{
	FS47TruckTrouble* Trouble = nullptr;
	/** The truck's radio (main.ts, carRadio): made with the airwaves, so asked for. */
	TFunction<FS47Receiver*()> Radio;
	US47Audio* Audio = nullptr;
	/** The town's part of the case (main.ts, townSeen). */
	FS47TownSeen* Seen = nullptr;
	/** The warning begins in the cab (abduction.beginCab). */
	TFunction<void()> Begin;
	/** Something holds the truck back (World.holdTruck), its sound (World.hushTruck). */
	TFunction<void(double)> Hold;
	TFunction<void(bool)> Hush;
	/** The truck's roof in the world (the knocks), or unset. */
	TFunction<TOptional<FVector>()> Roof;
	/** The truck in SARO's area now: where (world) and its heading, or unset. */
	struct FTruckAt { double X = 0, Z = 0, H = 0; };
	TFunction<TOptional<FTruckAt>()> Truck;
	/** Put it down there: being driven (World.moveDriven), or standing (World.moveParked). */
	TFunction<bool(double, double, double)> MoveDriven, MoveParked;
	/** The headlights of the truck being driven, on or off; the frost on its windshield. */
	TFunction<void(bool)> Headlights;
	TFunction<void(double)> Frost;
	/** The odometer (miles since the shift began), what it read when the truck last set out from SARO,
	 *  and miles nobody drove put on it. */
	TFunction<double()> Odometer;
	TFunction<TOptional<double>()> OdoAtSaro;
	TFunction<void(double)> AddMiles;
	/** The next time the truck is got into, its engine is dead and wants the key. */
	TFunction<void()> KeyStall;
	TFunction<double(double, double)> Ground;
	TFunction<void(const FString&, float)> Toast;
	TFunction<void(const FString&)> Note;
	TFunction<void()> Save;
};

class SIGNAL47_API FS47TruckTaken
{
public:
	explicit FS47TruckTaken(FS47TruckTakenDeps InD) : D(MoveTemp(InD)) {}

	/** Out of the truck and this far from it (m), before the light: free. */
	static constexpr double RUN = 30;
	/** The miles on the odometer when the player wakes, over what it read at SARO. */
	static constexpr double MILES = 31.4;

	/** 'idle' | 'warning' | 'taken' | 'wake' | 'tail' */
	FString Phase = TEXT("idle");
	/** How the last one ended ('key' | 'ran' | 'off' | 'taken'), empty when none. */
	FString Ended;
	/** The truck has let go of the player in this warning ('key' | 'ran'), empty when not. */
	FString Free;
	/** Stands aside: never begins (the older tests that drive out of SARO). */
	bool bOff = false;
	/** The next pass over B southbound begins it, the curve or not (the tests, the developer menu). */
	bool bForce = false;
	/** The truck's distance from B now (m), or unset (the tests). */
	TOptional<double> Dist;
	/** Passes over B southbound that were the ladder's (the tests). */
	int32 LadderPasses = 0;
	/** The turns of the key in this warning. */
	int32 Turns = 0;
	/** This warning's key never catches (the night's second in the truck, after the key got away once). */
	bool bKeyProof = false;

	/** Taken in the truck tonight. */
	bool Taken() const;
	/** Warnings in the truck tonight. */
	int32 Tries() const;
	/** Something of it is going (one at a time). */
	bool Busy() const { return Phase != TEXT("idle"); }
	/** In the 47 seconds, the engine dead: the key and Get out are offered. */
	bool Warning() const { return Phase == TEXT("warning") && Free.IsEmpty(); }
	/** The truck is coming onto B on a trip or on it, and it could come on this pass. */
	bool Ahead() const;

	void Update(double Dt, const FS47TruckTakenIn& S);
	/** Begin now, the truck where it is (the developer menu; the pass over B calls it itself). */
	void Start(double Clock);
	/** Where the truck will be put down (world, on the ground), from where it stands when the light comes. */
	TOptional<FVector> Planned();
	/** The key in the cab (main.ts, Use): after :40 there is nothing in the battery to turn it with. */
	bool UseKey(TOptional<double> WarnT);
	/** The waking: the truck put down 300 m on, the nose north; the player at its wheel (bInCab) or not. */
	TOptional<FVector> Wake(double Lost, bool bInCab);
	/** The notes of it (after the time away's own three). */
	void Woke(const FString& A, const FString& B, bool bInCab);
	FString BackText() const;
	/** A new night, or a save opened, or the title: nothing running, the truck its own. */
	void Reset();
	/** One line for the debug overlay. */
	FString Text() const;

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47TruckTakenDeps D;
	double At = 0;
	bool bArmed = false;
	FString Pass; // 'ladder' | 'free' | empty
	double T = 0;
	bool bKnocked = false, bRadioWas = false, bToldSilent = false, bToldDead = false, bGotOut = false, bWasOut = false;
	double North = 0;
	struct FPlan { double X, Z; };
	TOptional<FPlan> Plan;
	/** The knocks were the night's own (it still owed them). */
	bool bKnocksOwed = false;
	bool bOverSeen = false;
	double FrostK = 0;
	double DashK = 1;
	bool bWokeHere = false;
	/** Where it was taken: at the wheel, or out of the truck. */
	bool bTakenIn = true;
	/** Woken at the wheel: the headlights come on when the engine does. */
	bool bLightsOwed = false;
	bool bCould = false;
	TSet<int32> FailedTold;

	FS47Receiver* Radio() const;
	void WarningStep(double Dt, const FS47TruckTakenIn& S);
	void Release();
	void TakenStep();
	void WakeStep(double Dt, const FS47TruckTakenIn& S);
	void End(const FString& How, const FS47TruckTakenIn& S);
	bool Has(const FString& Prefix) const;
};
