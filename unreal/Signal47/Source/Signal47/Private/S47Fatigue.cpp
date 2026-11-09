#include "S47Fatigue.h"

#include "Dom/JsonObject.h"
#include "S47Audio.h"
#include "S47Freeform.h"
#include "S47Json.h"
#include "S47Main.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Fatigue, Log, All);

namespace S47FatigueLocal
{
	// A rest shorter than this is a sit-down, not a sleep: it counts as awake.
	constexpr double NapSeconds = 30 * 60;
	// A jump of the clock longer than this that is no rest and no time taken is no time anyone lived through (a developer's test).
	constexpr double LivedJump = 2 * 3600;
	// The mug's coffee level in the scene (AS47Web::SetCoffeeLevel), as the story pours it.
	constexpr double MugFull = 0.088, MugHalf = 0.05;
	double Smooth(double A, double B, double X) { return FMath::SmoothStep(A, B, X); }
}

double FS47Fatigue::LevelOf(double AwakeHours, double Caffeine, double ClockOfDay)
{
	using namespace S47FatigueLocal;
	// tired from about fifteen hours awake, worn out by twenty-seven
	const double Base = Smooth(15.0, 27.0, AwakeHours);
	// the small hours are the worst of it, once the day has been long
	const double H = FMath::Fmod(FMath::Fmod(ClockOfDay, 86400.0) + 86400.0, 86400.0) / 3600.0;
	const double Dip = 0.14 * Smooth(1.5, 3.5, H) * (1 - Smooth(5.0, 7.0, H)) * Smooth(8.0, 12.0, AwakeHours);
	// coffee takes off how tired you feel, up to about half of it
	return FMath::Clamp(Base + Dip - 0.45 * FMath::Clamp(Caffeine, 0.0, 1.0), 0.0, 1.0);
}

double FS47Fatigue::SleepWorth(EBed Where)
{
	return Where == EBed::Motel ? 3.0 : Where == EBed::Cot ? 1.7 : 2.5;
}

double FS47Fatigue::RunScaleOf(double Tired) { return FMath::Lerp(1.0, 0.45, S47FatigueLocal::Smooth(0.3, 1.0, Tired)); }
double FS47Fatigue::StressRateOf(double Tired) { return 1.0 + 1.2 * S47FatigueLocal::Smooth(0.25, 1.0, Tired); }

double FS47Fatigue::EdgeAlphaOf(double Tired, double InT, double InDroop)
{
	// a dark edge that comes and goes, never still, and the eyes closing for a moment near the end
	const double Base = 0.38 * S47FatigueLocal::Smooth(0.3, 1.0, Tired);
	const double Flicker = 0.65 + 0.35 * FMath::Sin(InT * 7.3) * FMath::Sin(InT * 2.9 + 1.7);
	return FMath::Clamp(Base * Flicker + 0.85 * FMath::Clamp(InDroop, 0.0, 1.0), 0.0, 0.95);
}

double FS47Fatigue::EdgeReachOf(double Tired, double InDroop)
{
	return Tired < 0.3 && InDroop <= 0 ? 0.0 : 0.1 + 0.14 * S47FatigueLocal::Smooth(0.3, 1.0, Tired) + 0.3 * FMath::Clamp(InDroop, 0.0, 1.0);
}

bool FS47Fatigue::Live() const
{
	const FS47FreeformSession* F = M && M->IsFreeform() ? M->Freeform.Get() : nullptr;
	return F && F->Active() && (F->RulesLive() || bDev);
}

double FS47Fatigue::Level() const
{
	if (!Live()) return 0;
	const double Clock = M->Freeform->Clock();
	return LevelOf(Awake, Caf, FMath::Fmod(Clock, 86400.0));
}

double FS47Fatigue::EdgeAlpha() const { return Live() ? EdgeAlphaOf(Level(), T, Droop) : 0.0; }
double FS47Fatigue::EdgeReach() const { return Live() ? EdgeReachOf(Level(), Droop) : 0.0; }

void FS47Fatigue::Begin(const TSharedPtr<FJsonObject>& Saved)
{
	// A new game: the player arrives in the morning, rested. A save before FATIGUE: the same.
	Awake = Saved.IsValid() ? FMath::Max(0.0, S47J::Num(Saved, TEXT("awake"), RestedAfterArrival)) : RestedAfterArrival;
	Caf = Saved.IsValid() ? FMath::Clamp(S47J::Num(Saved, TEXT("caffeine"), 0), 0.0, MostCaffeine) : 0.0;
	Mug = Saved.IsValid() ? FMath::Clamp(S47J::Int(Saved, TEXT("mug"), 0), 0, 2) : 0;
	LastClock = -1; Droop = 0; DroopAge = -1; DroopIn = 60; T = 0; bDev = false;
	ShowMug();
}

TSharedRef<FJsonObject> FS47Fatigue::Save() const
{
	TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
	O->SetNumberField(TEXT("awake"), FMath::RoundToDouble(Awake * 100) / 100);
	if (Caf > 0.005) O->SetNumberField(TEXT("caffeine"), FMath::RoundToDouble(Caf * 100) / 100);
	if (Mug > 0) O->SetNumberField(TEXT("mug"), Mug);
	return O;
}

void FS47Fatigue::Stop()
{
	if (M && M->Player) M->Player->RunScale = 1;
	Droop = 0; DroopAge = -1; bDev = false;
}

void FS47Fatigue::ShowMug() const
{
	if (!M || !M->Web) return;
	M->Web->SetCoffee(Mug > 0);
	if (Mug > 0) M->Web->SetCoffeeLevel(Mug == 1 ? S47FatigueLocal::MugFull : S47FatigueLocal::MugHalf);
}

void FS47Fatigue::Step(double Dt)
{
	using namespace S47FatigueLocal;
	const FS47FreeformSession* F = M && M->IsFreeform() ? M->Freeform.Get() : nullptr;
	if (!F || !F->Active()) return;
	// the hours awake go with the game's clock (a dig, a tow, a wait under two hours: lived through; the time taken: Lost)
	const double Clock = F->Clock();
	if (LastClock < 0) LastClock = Clock;
	const double Gap = Clock - LastClock;
	LastClock = Clock;
	if (Gap > 0)
	{
		if (Gap <= LivedJump) Awake += Gap / 3600.0;
		Caf *= FMath::Pow(0.5, Gap / CaffeineHalfLife);
	}
	if (!FMath::IsFinite(Dt) || Dt <= 0 || M->bPausedByMenu || M->ModalOpen()) return;
	T += Dt;
	const double Tired = Level();
	// near the end of a long night the eyes close for a moment now and then
	if (DroopAge >= 0)
	{
		DroopAge += Dt;
		Droop = FMath::Sin(FMath::Clamp(DroopAge / 0.9, 0.0, 1.0) * UE_DOUBLE_PI);
		if (DroopAge >= 0.9) { DroopAge = -1; Droop = 0; }
	}
	else if (Tired > 0.7 && M->Mode == US47Main::EMode::Play && (DroopIn -= Dt) <= 0)
	{
		DroopIn = FMath::FRandRange(35.f, 90.f) / Tired;
		DroopAge = 0;
	}
	if (M->Player) M->Player->RunScale = RunScaleOf(Tired);
}

void FS47Fatigue::Slept(double Seconds, EBed Where)
{
	using namespace S47FatigueLocal;
	if (!FMath::IsFinite(Seconds) || Seconds <= 0) return;
	// the session's clock has moved already: none of this is counted again as time awake
	if (M && M->Freeform) LastClock = M->Freeform->Clock();
	Caf *= FMath::Pow(0.5, Seconds / CaffeineHalfLife);
	if (Seconds < NapSeconds) { Awake += Seconds / 3600.0; return; }
	const double Was = Awake;
	Awake = FMath::Max(0.0, Awake - Seconds / 3600.0 * SleepWorth(Where));
	Droop = 0; DroopAge = -1; DroopIn = 60;
	UE_LOG(LogS47Fatigue, Display, TEXT("S47: fatigue: slept %.1f h (%d), awake %.1f h -> %.1f h"), Seconds / 3600.0, int32(Where), Was, Awake);
	// still tired after it: said, not numbered
	if (Live() && M->UI && Level() > 0.35)
		M->UI->Toast(Where == EBed::Cot ? TEXT("The cot's frame was in your back all night. Your eyes still sting.") : TEXT("Not enough sleep. Your eyes still sting."), 4);
}

void FS47Fatigue::Lost(double Seconds)
{
	if (!FMath::IsFinite(Seconds) || Seconds <= 0) return;
	if (M && M->Freeform) LastClock = M->Freeform->Clock();
	Awake += Seconds / 3600.0;
	Caf *= FMath::Pow(0.5, Seconds / CaffeineHalfLife);
}

bool FS47Fatigue::Use(const FString& Id)
{
	if (!M || !M->IsFreeform() || !M->Web || M->Web->Area() != TEXT("saro")) return false;
	if (Id == TEXT("coffeePot"))
	{
		if (Mug > 0) { if (M->UI) M->UI->Toast(TEXT("Your mug is still full. The hot plate ticks."), 2.6f); return true; }
		if (M->Audio) M->Audio->Pour(M->Web->PropLocation(TEXT("coffeePot")));
		Mug = 1; ShowMug();
		if (M->UI) M->UI->Toast(TEXT("You fill the SARO mug. It has been on the hot plate since the day shift."), 3);
		return true;
	}
	if (Id == TEXT("mug"))
	{
		if (Mug == 0) { if (M->UI) M->UI->Toast(TEXT("Empty. The coffee machine is in the corner."), 2.6f); return true; }
		Caf = FMath::Min(MostCaffeine, Caf + (Mug == 1 ? CupCaffeine : CupCaffeine * 0.5));
		Mug = Mug == 1 ? 2 : 0; ShowMug();
		if (M->Audio) M->Audio->Play(TEXT("thud_soft"), 0.25f, M->Web->PropLocation(TEXT("mug")));
		if (M->UI) M->UI->Toast(Caf > 1.2 ? TEXT("Another one. Your hands are not quite steady.") : Mug == 2 ? TEXT("Burnt. It helps.") : TEXT("The last of it, gone cold."), 2.8f);
		UE_LOG(LogS47Fatigue, Display, TEXT("S47: fatigue: coffee, caffeine %.2f"), Caf);
		return true;
	}
	return false;
}

FString FS47Fatigue::Label(const FString& Id) const
{
	if (!M || !M->IsFreeform() || !M->Web || M->Web->Area() != TEXT("saro")) return FString();
	if (Id == TEXT("coffeePot")) return Mug == 0 ? TEXT("Pour a cup of coffee") : TEXT("Coffee machine");
	if (Id == TEXT("mug")) return !M->Web->IsMugVisible() ? FString() : Mug > 0 ? FString(TEXT("Drink the coffee")) : FString(TEXT("SARO mug"));
	return FString();
}

void FS47Fatigue::DevSet(double Hours)
{
	if (Hours < 0) { bDev = false; return; }
	Awake = Hours; Caf = 0; bDev = true; DroopIn = 4;
	if (M && M->Freeform) LastClock = M->Freeform->Clock();
}

FString FS47Fatigue::Describe() const
{
	return FString::Printf(TEXT("awake %.1f h, coffee %.2f, tired %.2f (%s), run x%.2f, strain x%.2f"), Awake, Caf, Level(), Live() ? (bDev ? TEXT("F10") : TEXT("live")) : TEXT("off in a developer run"),
		RunScaleOf(Level()), StressRateOf(Level()));
}

bool FS47Fatigue::SelfTest(TArray<FString>& Lines)
{
	bool OK = true;
	const auto Check = [&](bool B, const TCHAR* Why) { OK &= B; Lines.Add(FString::Printf(TEXT("%s fatigue: %s"), B ? TEXT("PASS") : TEXT("FAIL"), Why)); };
	const double Noon = 12 * 3600.0, Four = 4 * 3600.0;
	Check(LevelOf(RestedAfterArrival, 0, Noon) == 0 && LevelOf(12, 0, Noon) == 0, TEXT("arrived rested, and a working day is not tiring yet"));
	Check(LevelOf(20, 0, Noon) > 0.2 && LevelOf(26, 0, Noon) > 0.85 && LevelOf(20, 0, Noon) < LevelOf(24, 0, Noon), TEXT("tired from about fifteen hours awake, worn out by twenty-seven"));
	Check(LevelOf(20, 0, Four) > LevelOf(20, 0, Noon) + 0.1 && LevelOf(6, 0, Four) == 0, TEXT("the small hours are the worst after a long day, nothing after a short one"));
	Check(LevelOf(22, 1, Noon) < LevelOf(22, 0, Noon) - 0.4 && LevelOf(30, 1.5, Noon) > 0.5, TEXT("coffee takes off about half of it and does not stand in for sleep"));
	Check(SleepWorth(EBed::Motel) > SleepWorth(EBed::Station) && SleepWorth(EBed::Station) > SleepWorth(EBed::Cot) && 8 * SleepWorth(EBed::Station) >= 20, TEXT("a night's sleep at the station undoes a long night; the motel bed best, the cot least"));
	Check(RunScaleOf(0) == 1 && RunScaleOf(1) < 0.5 && StressRateOf(0) == 1 && StressRateOf(1) > 2, TEXT("tired: a shorter run and a strain that comes on twice as fast"));
	bool bQuietRested = true, bShows = false;
	for (double Tm = 0; Tm < 20; Tm += 0.05)
	{
		bQuietRested &= EdgeAlphaOf(0, Tm, 0) == 0 && EdgeReachOf(0.1, 0) == 0;
		bShows |= EdgeAlphaOf(0.9, Tm, 0) > 0.25;
	}
	Check(bQuietRested && bShows && EdgeAlphaOf(0.9, 1, 1) > 0.8 && EdgeReachOf(0.9, 1) > EdgeReachOf(0.9, 0), TEXT("the edge: nothing when rested, a flicker when tired, dark for a moment when the eyes close"));
	{
		// a whole first night: arrived at eight, up all night, coffee at two, a morning's rest; nothing counts in a developer run's sense here
		FS47Fatigue Body(nullptr);
		Body.Begin(nullptr);
		const double At0 = Body.Awake;
		Body.Awake += 18; // to two in the morning
		const double Before = LevelOf(Body.Awake, Body.Caf, 2 * 3600.0);
		Body.Caf = FMath::Min(MostCaffeine, Body.Caf + CupCaffeine);
		const double After = LevelOf(Body.Awake, Body.Caf, 2 * 3600.0);
		Body.Slept(6 * 3600.0, EBed::Station);
		Check(At0 == RestedAfterArrival && After < Before && Body.Awake < 6 && Body.Caf < CupCaffeine * 0.3, TEXT("up all night, a cup at two helps; six hours' sleep at the station leaves you rested and the coffee gone"));
		FS47Fatigue Nap(nullptr); Nap.Begin(nullptr); Nap.Awake = 20; Nap.Slept(20 * 60.0, EBed::Station);
		Check(Nap.Awake > 20, TEXT("twenty minutes in the chair is no sleep"));
		const TSharedRef<FJsonObject> Saved = Body.Save();
		FS47Fatigue Again(nullptr); Again.Begin(Saved);
		Check(FMath::IsNearlyEqual(Again.Awake, Body.Awake, 0.01), TEXT("the hours awake are saved"));
	}
	return OK;
}
