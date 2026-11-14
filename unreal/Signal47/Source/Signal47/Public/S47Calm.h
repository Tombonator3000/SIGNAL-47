#pragma once
#include "CoreMinimal.h"
class US47Main;

// N1110 (rhythm, items 5 and 11 of the horror study): the night's breathing, as two small things the Director's events do not do.
//
// False alarms: once or twice an evening (before 01:30, the hint tier) something sounds like the start of an event and turns out
// ordinary: a cow leaning on the fence wire, the wind on a shed door, a jackrabbit in the brush, the refrigerator clunking (once a week
// at most), a pipe ticking, the printer's paper settling, a vent rattling (where each fits is its script's own indoors/outdoors gate). The night
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
	/** The refrigerator alarm is once a week at most: Day, and the day it last played (0: never) (pure). */
	static bool FridgeMay(int32 Day, int32 LastDay) { return LastDay <= 0 || Day >= LastDay + 7; }
	// N1110 (pace, critic 3): three variants of the relief, by the day (calm_after for days 1, 4, 7; calm_after_b for 2, 5, 8; calm_after_c for 3, 6, 9).
	static const TCHAR* ReliefScript(int32 Day = 1) { const int32 K = ((Day % 3) + 3) % 3; return K == 2 ? TEXT("calm_after_b") : K == 0 ? TEXT("calm_after_c") : TEXT("calm_after"); }

	// PLAY-31-34 (14 November, Tom's «Vurder»): a ceiling tube in the control room burns out of age in a false alarm's slot (script
	// tube_wear: its gates hold the rest, day 8 on and about one night in four). Pure: from week 2, none already out of age waiting for
	// the day shift, a real tube left to go, the player in the control room to see it, never on a Story night.
	static constexpr const TCHAR* TubeWearScript = TEXT("tube_wear");
	static constexpr const TCHAR* TubeDaleScript = TEXT("tube_wear_dale");
	static bool TubeWearMay(int32 Day, int32 WornNow, bool bTubeLeft, bool bInControlRoom, bool bStoryNight)
	{ return Day >= 8 && WornNow == 0 && bTubeLeft && bInControlRoom && !bStoryNight; }
	// Dale's word the evening after the day shift's new tube (FS47FreeformSession::TubeRemarkDue): 19:00 to 23:00 (night hours 19 to 23).
	static bool TubeRemarkHour(double NightHour) { return NightHour >= 19. && NightHour < 23.; }
	// REVIEW (14 Nov): Dale's word goes by the rules his other walkie lines keep (FS47Predecessor's hail): a set that hears him, no
	// hail or line of his already on the air, nothing near enough to break the set up, no light taking someone.
	static bool DaleMayRemark(bool bHearsDale, bool bHailing, bool bSpeaking, double Presence, bool bTaking)
	{ return bHearsDale && !bHailing && !bSpeaking && Presence <= .01 && !bTaking; }

	/** Once a frame from FS47Happenings::Step. */
	void Step(double Dt);
	/** A new game or area: nothing waits. */
	void Reset();

	/** N1110 (pace, critic 2): clock seconds of the last false alarm (-1e9: none yet). */
	double LastAlarmAt() const { return LastAlarm; }
	int32 AlarmsPlayed() const { return Played; }
	int32 ReliefPlayed() const { return Reliefs; }

	static bool SelfTest(TArray<FString>& Lines);

private:
	bool Quiet(double Clock) const;
	double LastAlarm = -1e9;       // clock seconds of the last false alarm
	int32 LastFridgeDay = 0;       // the day the refrigerator last played (0: never)
	int32 LastTubeNight = 0;       // PLAY-31-34: the night tube_wear was last asked (its odds are the night's)
	US47Main* M = nullptr;
	int32 Night = 0, Played = 0, Reliefs = 0;
	double NextTry = 0;            // real seconds (US47Main::T) before the next question
	double DangerEnd = -1e9;       // clock seconds when a danger last ended
	double ReliefAt = -1;          // clock seconds when its relief is due; negative: none waiting
	FName PrevBusy = NAME_None;
	bool bPrevDanger = false;
};
