#pragma once
#include "CoreMinimal.h"
#include "Math/RandomStream.h"

class US47Main;
class FJsonObject;

// FATIGUE-20261108: sleep and tiredness in Freeform, with no number on the screen. The hours awake count with the game's clock (time
// lost while taken counts too: you were not asleep); a rest at the station, a room at the motel or the cot out on the land takes them
// off, the cot least. Tired shows and has its cost: now and then the eyelids grow heavy for a few seconds (SS47Hud paints them from the
// top and the bottom of the view, apart from the strain's ring, which always reads), with a yawn; near the end of the night the eyes close
// for a moment inside such a spell; the strain comes on faster (FS47Stress); a tired run winds you sooner and eases off, heavy breathing
// with it (AS47Walker::RunScale), but it stays a run. Coffee from the pot in the control room, or the Mesa Diner's, helps for a few hours
// and does not stand in for sleep: it takes off how tired you feel, not the hours. Held by FS47FreeformSession::Fatigue and saved with it.
// A developer run feels none of it unless F10 sets it, so that the fixtures run as they did.
// M5 critic round 3 (9 November): sleep pays off the hours awake in proportion (the first hours of it most), so that the game's own rhythm
// (up through the evening and the small hours, a rest until morning) holds steady instead of running into a debt from the second evening;
// the bed says what kind of sleep it will be, and waking from a short one is heavy-eyed for a while, with a yawn.
class SIGNAL47_API FS47Fatigue
{
public:
	explicit FS47Fatigue(US47Main* Main) : M(Main) {}

	void Begin(const TSharedPtr<FJsonObject>& Saved);
	TSharedRef<FJsonObject> Save() const;
	void Stop();
	/** Every frame after the session (game time). */
	void Step(double Dt);
	/** A rest the session has just given (FS47FreeformSession::WaitUntil, after the clock moved): how long and where. */
	enum class EBed : uint8 { Station, Motel, Cot };
	void Slept(double Seconds, EBed Where);
	/** Time lost while taken (FS47FreeformSession::TakenAway, after the clock moved): awake, every hour of it. */
	void Lost(double Seconds);
	/** The coffee machine and the mug in the control room ("coffeePot", "mug"). */
	bool Use(const FString& Id);
	FString Label(const FString& Id) const;
	/** The Mesa's waitress pours a cup (FS47FreeformSession::Use, as her table opens): drunk, unless she is away. */
	void AtDiner();
	/** What a wait of this long here will be as sleep, in words and no numbers (FS47FreeformSession::WaitConsequences). */
	FString BedLine(double Seconds, EBed Where) const;

	/** How tired, 0 rested to 1 dead on your feet: what the others read (0 in a developer run unless F10 set it). */
	double Level() const;
	double AwakeHours() const { return Awake; }
	double Caffeine() const { return Caf; }
	/** The edge's faint darkness and how far in it reaches, for SS47Hud (0 when rested): always lighter than a moderate strain's. */
	double EdgeAlpha() const;
	double EdgeReach() const;
	/** How far the eyelids are down, 0 open to 1 shut, for SS47Hud: a heavy spell now and then, a moment shut near the end. */
	double Lids() const;
	/** How far off a winded breath can be heard just now, metres (0 most of the time), for FS47RoswellGroup's ears. */
	double HeardWithin() const { return HeardFor > 0 ? HeardReach : 0.0; }
	/** F10: this many hours awake now, felt in a developer run too (negative: as the game has it again). */
	void DevSet(double Hours);
	FString Describe() const;

	// ---------- the pure rules (tested) ----------
	/** Tiredness from the hours awake, the coffee in you and the hour (the small hours are the worst). */
	static double LevelOf(double AwakeHours, double Caffeine, double ClockOfDay);
	/** The hours awake left after SleptHours in this bed: sleep pays off a share of them, the first hours most; under half an hour is
	 *  no sleep, and a doze under a couple of hours counts for less. The motel bed best, the cot least. */
	static double AfterSleep(double AwakeHours, double SleptHours, EBed Where);
	/** How much of the run is left (AS47Walker: the run is 1 + 0.6 times this as fast as the walk), by how tired and how much wind is
	 *  left (1 fresh, 0 winded). Never under RunFloor: a tired run is still a run, heard as one, faster than a figure hunting. */
	static double RunScaleOf(double Tired, double Wind = 1);
	/** The wind a second of running takes, by how tired (none when rested). */
	static double WindDrainOf(double Tired);
	/** How much faster the strain comes on (FS47Stress::Advance). */
	static double StressRateOf(double Tired);
	/** The faint ring (alpha, reach) by how tired and how far into a heavy spell (0 to 1). */
	static double EdgeAlphaOf(double Tired, double Spell);
	static double EdgeReachOf(double Tired, double Spell);
	/** The lids by how tired, a heavy spell (0 to 1), the eyes closing (0 to 1) and the heaviness of waking (0 to 1). */
	static double LidsOf(double Tired, double Spell, double Droop, double Waking);
	/** Seconds from one heavy spell to the next (U a draw 0 to 1): never under MinSpellGap, rarer when only a little tired. */
	static double SpellGapOf(double Tired, double U);
	static constexpr double CupCaffeine = 0.6, MostCaffeine = 1.5, CaffeineHalfLife = 3 * 3600.0, RestedAfterArrival = 2;
	static constexpr double RunFloor = 0.9, MinSpellGap = 20, WakeSeconds = 22;
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	double Awake = RestedAfterArrival, Caf = 0, LastClock = -1, T = 0, Droop = 0, DroopAge = -1;
	// the heavy spells: seconds to the next, how far into this one (-1 none) and how long it lasts; whether the eyes close in it
	double SpellIn = 30, SpellAge = -1, SpellLen = 4, Spell = 0;
	bool bSpellDroop = false;
	int32 Spells = 0;
	// waking from a short sleep: seconds into it (-1 not), and a yawn due in this many seconds (-1 none)
	double WakeAge = -1, YawnIn = -1;
	// the run's wind (1 fresh), where the feet were, a winded breath due and how long it can still be heard, and how far
	double Wind = 1, BreathIn = 0, HeardFor = 0, HeardReach = 0, SinceRun = 1e9;
	FVector LastFeet = FVector::ZeroVector;
	bool bHaveFeet = false;
	double DinerAt = -1e9; // the clock when the Mesa's cup was last drunk
	int32 Mug = 0; // the mug: 0 empty, 1 full, 2 half
	bool bDev = false;
	FRandomStream Rng = FRandomStream(4711);
	bool Live() const;
	void ShowMug() const;
	/** The spells, the eyes closing and the waking: game time Dt at this tiredness (no world needed: tested). */
	void StepLids(double Dt, double Tired, bool bPlay);
	void Yawn(double K) const;
	void WindedBreath(double K) const;
};
