#pragma once
#include "CoreMinimal.h"
class US47Main;

// N1110 (rhythm, items 5 and 11 of the horror study): the night's breathing, as two small things the Director's events do not do.
//
// False alarms: once or twice an evening (before 01:30, the hint tier) something sounds like the start of an event and turns out
// ordinary: a cow leaning on the fence wire, the wind on a shed door, a jackrabbit in the brush, the refrigerator clunking. The night
// holds still first (the scripts' silence beat), then the thought gives it away. Scripts alarm_cow, alarm_shed_door, alarm_jackrabbit
// and alarm_fridge (Content/Director/Events) own the beats and their gates. Never in the first shift, never at the wheel, never while
// anything else is on, a rest runs, or a danger has just ended. They are not events of the rhythm: no budget, no finding, no map mark.
//
// Relief: eight to ten minutes of the clock after a danger has ended (the Director's own rest after a danger holds the night meanwhile)
// the script calm_after plays: the night sounds come back gently, a calm thought, the radio at 1420 AM finds an ordinary program if
// the player is indoors, and Dale checks in. Once per danger, and not after the long absence of being away from SARO.
//
// Nothing here is saved: a restart simply asks again (the slots follow the seed and the night).
class SIGNAL47_API FS47Calm
{
public:
	explicit FS47Calm(US47Main* Main) : M(Main) {}

	/** How many false alarms this night wants, 1 or 2 (pure, seeded). */
	static int32 AlarmsFor(int32 Seed, int32 Night);
	/** The night hour (19.5 to 25.5, so 24 is midnight) the Nth alarm (0 or 1) waits for (pure, seeded). */
	static double SlotHour(int32 Seed, int32 Night, int32 N);
	/** Clock seconds from a danger's end to the relief: 480 to 600 (pure, seeded). */
	static double ReliefDelay(int32 Seed, int32 Day);
	/** The false alarm scripts, in the order this night tries them (pure, seeded). */
	static TArray<FString> AlarmOrder(int32 Seed, int32 Night);
	/** Twenty clock minutes or more since the last false alarm (pure). */
	static bool GapOK(double LastAlarmClock, double Clock) { return Clock - LastAlarmClock >= 20 * 60.; }
	static const TCHAR* ReliefScript() { return TEXT("calm_after"); }

	/** Once a frame from FS47Happenings::Step. */
	void Step(double Dt);
	/** A new game or area: nothing waits. */
	void Reset();

	int32 AlarmsPlayed() const { return Played; }
	int32 ReliefPlayed() const { return Reliefs; }

	static bool SelfTest(TArray<FString>& Lines);

private:
	bool Quiet(double Clock) const;
	double LastAlarm = -1e9;       // clock seconds of the last false alarm
	US47Main* M = nullptr;
	int32 Night = 0, Played = 0, Reliefs = 0;
	double NextTry = 0;            // real seconds (US47Main::T) before the next question
	double DangerEnd = -1e9;       // clock seconds when a danger last ended
	double ReliefAt = -1;          // clock seconds when its relief is due; negative: none waiting
	FName PrevBusy = NAME_None;
	bool bPrevDanger = false;
};
