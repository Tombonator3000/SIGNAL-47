#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;

// MOTEL-SLEEP-20261108, Freeform only: a night at the Sierra Motor Court instead of SARO. Behind a motel door the station's dangers do
// not come (FS47FreeformSession::WakeBefore), but a motel at three in the morning has its own: the door handle tried, a car idling
// outside with its lights on the curtain, the phone on the nightstand, the truck gone through by morning, a party through the wall.
// Each night's is drawn from the game's seed. Some wake the player at the hour it happens (with its sound), and they may go back to
// sleep; what happened is kept as a field record. The motel is not marked on the place catalogue: its nights are its own, and the
// town's talk of lights over a place is not about a door handle (critics, round 3).
//
// SAVE-SARO-20261108: the log is filed at the station. A motel night is not saved: from the night until the player is back on SARO's
// ground no save is written (S47SaroSave::Unsaved asks Unfiled), and the departure from SARO afterwards saves as it always does.
class FS47MotelNight
{
public:
	explicit FS47MotelNight(US47Main* Main) : M(Main) {}
	void Begin(const S47J::FObj& Saved);
	void Step(double Dt);
	S47J::FObj Save() const;

	enum class EKind : uint8 { Quiet, Handle, Idling, Phone, Theft, Party };
	// What tonight is, from the seed; theft only when there is something in the truck to take (its spare wheel, a can of water: what
	// the truck carries, FS47TruckCare; never SARO's shelves, which are twenty kilometres off).
	static EKind KindOf(int32 Seed, int32 Night, bool bCanSteal);
	static bool Wakes(EKind K) { return K == EKind::Handle || K == EKind::Idling || K == EKind::Phone; }
	// When to sleep until, for a room taken now until Morning (08:00): the hour it wakes the player, or Morning.
	double SleepUntil(double Morning) const;
	// After the rest (FS47Places' desk): what the night was, its sound, its cost; a woken player may go back to sleep until Morning.
	// SleptFrom: the clock when the room was taken. A rest that did not cover the small hours (a nap from seven) has no night in it.
	void AfterRest(double Morning, double SleptFrom = -1);
	// The rule for that: some of the night's own hours (half past midnight to four) were slept through.
	static bool SleptTheNight(double SleptFrom, double Until, double Morning);
	// The motel night not yet filed: no save until back at SARO.
	bool Unfiled() const { return bUnfiled; }
	// F10: tonight is this kind, whatever the seed says.
	void DevKind(int32 Kind) { ForcedKind = Kind; }
	static bool SelfTest(TArray<FString>& Lines);
private:
	US47Main* M = nullptr;
	bool bUnfiled = false;
	int32 ForcedKind = -1;
	int32 LastNight = 0;
	double StepIn = 0;
	EKind Tonight(int32 Night) const;
	bool CanSteal() const;
};
