#pragma once
#include "CoreMinimal.h"

class US47Main;
class FJsonObject;

// FATIGUE-20261108: sleep and tiredness in Freeform, with no number on the screen. The hours awake count with the game's clock (time
// lost while taken counts too: you were not asleep); a rest at the station, a room at the motel or the cot out on the land takes them
// off, the cot least. Tired shows and has its cost: the edge of the view flickers dark now and then (SS47Hud paints it with the strain's
// edge), and near the end of the night the eyes close for a moment; the strain comes on faster (FS47Stress); the run is shorter
// (AS47Walker::RunScale). Coffee from the pot in the control room helps for a few hours and does not stand in for sleep: it takes off
// how tired you feel, not the hours. Held by FS47FreeformSession::Fatigue and saved with it. A developer run feels none of it unless F10
// sets it, so that the fixtures run as they did.
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

	/** How tired, 0 rested to 1 dead on your feet: what the others read (0 in a developer run unless F10 set it). */
	double Level() const;
	double AwakeHours() const { return Awake; }
	double Caffeine() const { return Caf; }
	/** The edge's darkness and how far in it reaches, for SS47Hud (0 when rested). */
	double EdgeAlpha() const;
	double EdgeReach() const;
	/** F10: this many hours awake now, felt in a developer run too (negative: as the game has it again). */
	void DevSet(double Hours);
	FString Describe() const;

	// ---------- the pure rules (tested) ----------
	/** Tiredness from the hours awake, the coffee in you and the hour (the small hours are the worst). */
	static double LevelOf(double AwakeHours, double Caffeine, double ClockOfDay);
	/** Hours awake a slept hour takes off: a motel bed best, the cot least. */
	static double SleepWorth(EBed Where);
	/** How much of the run is left (AS47Walker: the run is 1 + 0.6 times this as fast as the walk). */
	static double RunScaleOf(double Tired);
	/** How much faster the strain comes on (FS47Stress::Advance). */
	static double StressRateOf(double Tired);
	static double EdgeAlphaOf(double Tired, double T, double Droop);
	static double EdgeReachOf(double Tired, double Droop);
	static constexpr double CupCaffeine = 0.6, MostCaffeine = 1.5, CaffeineHalfLife = 3 * 3600.0, RestedAfterArrival = 2;
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	double Awake = RestedAfterArrival, Caf = 0, LastClock = -1, T = 0, Droop = 0, DroopAge = -1, DroopIn = 60;
	int32 Mug = 0; // the mug: 0 empty, 1 full, 2 half
	bool bDev = false;
	bool Live() const;
	void ShowMug() const;
};
