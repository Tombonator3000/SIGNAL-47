#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;

// MOTEL-SLEEP-20261108, Freeform only: a night at the Sierra Motor Court instead of SARO. Behind a motel door the station's dangers do
// not come (FS47FreeformSession::WakeBefore), save the first big night, which wakes whoever sleeps anywhere on its night (PT-BIGNIGHT-
// 20261109, Tom: BigWakeAt); but a motel at three in the morning has its own too: the door handle tried, a car idling
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
	// MOTEL-PAY-20261109: "Take a room" at the desk (FS47Places): the rest, and AfterRest. A room is paid for until the morning it was
	// taken for; woken before it (the big night, the night's own, or up and about) and back at the desk, the same night's room is not
	// charged again. False: no rest (refused, or a room not paid and the budget short).
	bool TakeRoom(double Morning);
	// The room taken tonight is still paid for at Now (before the morning it was taken until).
	bool RoomPaid(double Now) const { return PaidFor(PaidUntil, Now); }
	static bool PaidFor(double PaidUntil, double Now) { return PaidUntil > 0 && FMath::IsFinite(Now) && Now < PaidUntil - 1; }
	// After the rest (FS47Places' desk): what the night was, its sound, its cost; a woken player may go back to sleep until Morning.
	// SleptFrom: the clock when the room was taken. A rest that did not cover the small hours (a nap from seven) has no night in it.
	// Until: the hour the rest was to end (SleepUntil); woken before it, it was the big night that woke them, and the night's own is
	// not told yet.
	void AfterRest(double Morning, double SleptFrom = -1, double Until = -1);
	// When the first big night wakes a sleeper in a room here: a few minutes before its hour (BigHour, hours after the evening's
	// midnight counted on from 24, as 26.1 for 02:06), or soon if that is past and it is not yet half past four. Midnight: that
	// midnight's clock; Roll 0 to 1. Negative: not before Target (the rest ends first), or too late in the night.
	static double BigWakeAt(double Midnight, double BigHour, double Now, double Target, float Roll);
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
	double PaidUntil = 0; // the morning (08:00) tonight's room was paid until; 0: none
	double StepIn = 0;
	EKind Tonight(int32 Night) const;
	bool CanSteal() const;
};
