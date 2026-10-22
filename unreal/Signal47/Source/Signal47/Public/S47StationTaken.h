#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "S47StationTaken.generated.h"

class US47Main;
class US47Station;

// story/stationTaken.ts: at STATION 01 everyone goes (FORSLAG.md, 20 October, item 9 A; Tom:
// "Spenning og skummelt slår faste regler"). A player who has not been taken by the time N. Vega
// has called the field telephone in the hut gets the warning on the way out from it, the 47
// seconds of it, and this one cannot be got away from. The two ways out are closed, both in the
// world and both fairly: a step or two out of the door, the hut door swings shut behind the player
// by itself, latches, and the light inside goes out; and the truck by the gate, whose keys are
// always in it, has none in it. At :40, with the flashlight, the lamps outside go out too.
//
// On waking: the door stands open into the room again, the light is on, the lamps outside are on,
// and the truck's keys are lying in the frost by the player's hand.
//
// main.ts begins it on the time away (story/abduction.ts, stationDue) and steps it with the time
// away's stage; that system is not in the Unreal version yet, so nothing begins it here: chapter
// three has its hooks (taken: shut, tryDoor, truck) and the truck at the station asks it for its keys.
UCLASS()
class SIGNAL47_API US47StationTaken : public UObject
{
	GENERATED_BODY()

public:
	// StationTakenDeps: site, audio, toast, note, clockText (through M and its host)
	void Init(US47Main* InM, TFunction<US47Station*()> InSite);

	/** The door shuts this long after the warning begins, in this long; the light inside goes out after it. */
	static constexpr double DOOR_AT = 0.35, DOOR_T = 1.15, LIGHT_AT = 2.0;
	/** The player is this far out of the hut door before it begins (metres). */
	static constexpr double OUT_OF_DOOR = 2.4;
	/** The lost time's 17 minutes, a minute's room, and the warning's 47 s and the light's 3 s before
	 *  chapter three's fixed point (04:35): the last clock it may begin at. */
	static constexpr double LAST = 4 * 3600 + 35 * 60 - (17 * 60 + 60 + 47 + 3);
	/** story/abduction.ts BEAT.dark: the warning's second the lamps go out with the flashlight */
	static constexpr double BEAT_DARK = 40;

	/** idle: nothing (or over); shut: the warning there is under way, the door shut, the keys gone. */
	FString State = TEXT("idle");
	/** The keys are not in the truck (the truck at the station says so). */
	bool bKeysGone = false;
	/** How it went the last time (the tests): 'taken' or 'passed' ('' null). */
	FString Last;

	/** The hut door is shut (the hut says so when it is tried). */
	bool HutShut() const { return State == TEXT("shut") && DoorK > 0.8; }

	/** The forced warning has begun here (main.ts has started it on the time away). */
	void Begin(double Clock);

	/** StationIn: the time away's stage, how far into the warning (seconds; unset outside it), the clock. */
	struct FIn { FString Stage; TOptional<double> WarnT; double Clock = 0; };
	void Update(double Dt, const FIn& S);

	/** The truck by the gate while the keys are gone. */
	void TryTruck();
	/** The hut door while it is shut. */
	void TryDoor();
	/** How the truck at the station is described while the keys are gone ('' null: as always). */
	FString TruckExamine() const { return bKeysGone ? FString(TEXT("SARO 07, by the gate where you left it. The ignition is empty.")) : FString(); }

	/** A night put back or left in the middle: the station as it always is. */
	void Reset();

	static bool SelfTest(TArray<FString>& Lines);

private:
	UPROPERTY() TObjectPtr<US47Main> M;
	TFunction<US47Station*()> Site;
	double T = 0;
	double DoorK = 0;
	bool bLit = true;
	bool bLamps = true;
	/** Taken: the keys and the note wait until the player is up again. */
	bool bPending = false;
	int32 TriedTruck = 0;
	int32 TriedDoor = 0;
	double From = 0;
	FString WasStage = TEXT("calm");
	TArray<FString> TestSaid; // toasts and notes without a host (the self test)

	TOptional<FVector> DoorAt(US47Station* S) const;
	void PutBack();
	void Toast(const FString& Text, float Secs = 3.2f);
	void Note(const FString& Text);
	void Play(const FString& Name, float Gain, float Rate, float When, const TOptional<FVector>& At);
};
