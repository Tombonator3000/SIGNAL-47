#pragma once
#include "CoreMinimal.h"
#include "S47Weather.h"

class US47Main;
class FS47GainNode;
class UMaterialInstanceDynamic;

// WEATHER-20261108: the day's weather on the world, in Freeform. S47Weather::Day() is the truth (the paper prints it); each frame this
// reads what it is doing now (S47Weather::Now, for the calendar day and the clock of the session), eases toward it over a few seconds
// (a rest or a test that moves the clock takes it at once) and writes it to S47Weather::Live(), which the world reads:
//   the sky       FS47Sky's cloud gets the day's bias (US47WorldLife), and thunder far off on a shower afternoon (FS47Sky::DayThunder)
//   the air       the native fog thickens and takes the sand's colour or the haze's, the far land fades (AS47Web::UpdateNativeFog,
//                 US47WorldLife: the horizon haze); sand streams through the air by day and through the lamp and the headlights at
//                 night, a few drops fall in a shower (FS47NightLife)
//   the wind      the brush sways harder and turns with it (S47Wind::Storm); the ambience roars and hisses, the crickets go under,
//                 far-off sounds come less, drops tick in a shower (FS47Ambience)
//   the ground    sand drifts onto the dirt tracks, a crosswind pushes the truck at speed, wet dirt has less grip (FS47TruckCare);
//                 the wet is heard as the tyres' hiss on it, and the first slide on it says why (StepWet, M5 critic 3)
//   the ears      a storm covers footsteps and breathing from what listens (FS47RoswellGroup)
//   the night     a stormy night is a little busier (FS47Rhythm reads Day().NightStorm)
//   the lightning D46-20261110: the night's dry lightning (none, far off, or a storm on the horizon) and its thunder (FS47Sky::Update)
// Nothing makes the game unplayable: the thickest sand adds no more fog than a light mist (S47Weather::Live().FogAdd at most .03).
// A developer run has no weather unless F10 or -S47Weather asks, so that the fixtures' pictures keep their plain sky.
class SIGNAL47_API FS47WeatherLive
{
public:
	explicit FS47WeatherLive(US47Main* Main) : M(Main) {}

	/** Every frame of a Freeform game (game time). */
	void Step(double Dt);
	/** Out of Freeform, the title, a save opened: the world's weather off. */
	void Stop();

	/** F10: this weather now, for this long, over the day's own (None: the day's own again). A shower runs its own course on the
	 *  clock from ShowerHours after it set in (S47Weather::ShowerCourse): the rain, then the wet dirt drying over the hours after.
	 *  Lightning (D46-20261110): a storm on the horizon throwing dry lightning, where the night's own would stand. */
	enum class EDev : uint8 { None, Sand, Shower, Haze, Calm, Lightning };
	void Dev(EDev Kind, double Seconds, double ShowerHours = DevShowerHours);
	static constexpr double DevShowerHours = .5;
	EDev DevNow() const { return DevLeft > 0 ? DevWhat : EDev::None; }
	/** F10: the day's own weather on the world in a developer run too (as a real game has it). */
	void DevLive(bool bOn) { bForceLive = bOn; }
	bool DevLiveOn() const { return bForceLive; }
	/** What it is doing now, for F10. */
	FString Describe() const;
	/** The calendar day and the clock read last (F10). */
	int32 CalendarDay() const { return CalDay; }

	// ---------- the pure rules (tested) ----------
	/** How the world shows a moment's weather where the player is: Outdoors 1 out in the open (or in the cab), less indoors. */
	static void Shown(const S47Weather::FNow& Now, double Daylight, double Outdoors, S47Weather::FLive& Out);
	/** One step of easing from Cur toward Want (K 0 nothing, 1 all of it). */
	static void Ease(S47Weather::FNow& Cur, const S47Weather::FNow& Want, double K);
	/** What a developer's weather puts over the day's; a shower ShowerHours into its course. */
	static void Override(EDev Kind, S47Weather::FNow& Now, double ShowerHours = DevShowerHours);
	/** SHOWER-LOOK-20261109: how wet the ground shows (the ground's WetK, M_S47_FreeformGround): the slick dirt's own Wet (FS47TruckCare
	 *  takes the grip from it), so what is seen is what is driven on; 0 with no weather on the world. */
	static double GroundWet(const S47Weather::FLive& L) { return L.bOn ? FMath::Clamp(L.Wet, 0.0, 1.0) : 0.0; }
	static constexpr double MaxFog = .03;
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	S47Weather::FNow Cur;
	bool bHave = false, bCmdChecked = false, bCmd = false, bForceLive = false;
	double LastClock = -1, Outdoors = 1, FogApplied = -1, TintApplied = -1;
	EDev DevWhat = EDev::None;
	double DevLeft = 0, DevShowerH = DevShowerHours, DevShowerFrom = -1;
	int32 CalDay = 0, LoggedDay = 0;
	// the wet under the tyres: their hiss (a loop of noise on the sound effects, silent when dry) and the day the slide was explained
	TSharedPtr<FS47GainNode> Spray;
	int32 ToldWet = 0;
	void StepWet();
	// the wet on the ground's material (WetK), written when it changes; -1 none written
	double WetApplied = -1;
	TWeakObjectPtr<UMaterialInstanceDynamic> WetSeen;
	void ApplyGroundWet(double Wet);
	// M5 (13 November): the night's thought about being seen (S47Weather::NightThoughtKey): seconds out on foot in the dark, and the
	// night (noon to noon) it was last given on
	double OutFor = 0;
	int32 ThoughtNight = -100000;
	void StepNightThought(double Dt);
};
