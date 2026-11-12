#include "S47Fatigue.h"
#include <limits>

#include "Dom/JsonObject.h"
#include "S47Audio.h"
#include "S47Evening.h"
#include "S47Freeform.h"
#include "S47Json.h"
#include "S47Main.h"
#include "S47RoswellBrain.h"
#include "S47Sneak.h"
#include "S47Stress.h"
#include "S47Thoughts.h"
#include "S47Townsfolk.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Walkie.h"
#include "S47Web.h"
#include "S47WebAudio.h"
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
	// How fast sleep pays off the hours awake (hours of effective sleep for each e-fold): the motel bed best, the cot least. Tuned so that
	// the game's own rhythm, up from eight in the morning to half past three in the night and asleep until eight, holds steady at about
	// two hours awake on waking (a little tired at two in the morning, tired by the end of the night, fine in the evening).
	constexpr double TauStation = 1.7, TauMotel = 1.4, TauCot = 2.6;
	// How long the heavy spells last (seconds), how far the lids come down in one, and how long a winded run takes to get its wind back.
	constexpr double SpellLenMin = 3, SpellLenMax = 6, WindBackSeconds = 15, WindEmptySeconds = 25;
	double Smooth(double A, double B, double X) { return FMath::SmoothStep(A, B, X); }
	double TiredK(double Tired) { return Smooth(0.3, 1.0, Tired); }
}

double FS47Fatigue::LevelOf(double AwakeHours, double Caffeine, double ClockOfDay)
{
	using namespace S47FatigueLocal;
	// tired from about sixteen hours awake, worn out by twenty-eight
	const double Base = Smooth(16.0, 28.0, AwakeHours);
	// the small hours are the worst of it, once the day has been long
	const double H = FMath::Fmod(FMath::Fmod(ClockOfDay, 86400.0) + 86400.0, 86400.0) / 3600.0;
	const double Dip = 0.14 * Smooth(1.5, 3.5, H) * (1 - Smooth(5.0, 7.0, H)) * Smooth(8.0, 12.0, AwakeHours);
	// coffee takes off how tired you feel, up to about half of it
	return FMath::Clamp(Base + Dip - 0.45 * FMath::Clamp(Caffeine, 0.0, 1.0), 0.0, 1.0);
}

double FS47Fatigue::AfterSleep(double AwakeHours, double SleptHours, EBed Where, double CaffeineAtBed)
{
	using namespace S47FatigueLocal;
	const double Was = FMath::Max(0.0, FMath::IsFinite(AwakeHours) ? AwakeHours : 0.0);
	if (!FMath::IsFinite(SleptHours) || SleptHours <= 0) return Was;
	if (SleptHours < NapSeconds / 3600.0) return Was + SleptHours;
	// the first half hour is getting to sleep; a doze under a couple of hours is light sleep and counts for less
	const double Quality = FMath::Lerp(0.45, 1.0, Smooth(0.5, 3.0, SleptHours));
	const double Deep = (SleptHours - 0.5) * Quality;
	// M7 (C): wired (more than WiredCaffeine at lying down, the cap 1.5 at most), the sleep is up to half as thin again
	const double Wired = 1.0 + FMath::Clamp((FMath::IsFinite(CaffeineAtBed) ? CaffeineAtBed : 0.0) - WiredCaffeine, 0.0, MostCaffeine - WiredCaffeine);
	const double Tau = (Where == EBed::Motel ? TauMotel : Where == EBed::Cot ? TauCot : TauStation) * Wired;
	return Was * FMath::Exp(-Deep / Tau);
}

double FS47Fatigue::CupStrengthOf(double CupsBefore)
{
	// D46-20261110: diminishing returns within a day
	return FMath::Max(0.2, FMath::Pow(0.7, FMath::Clamp(FMath::IsFinite(CupsBefore) ? CupsBefore : 0.0, 0.0, 20.0)));
}

double FS47Fatigue::WalkScaleOf(double Tired) { return 1.0 - 0.12 * S47FatigueLocal::Smooth(0.5, 1.0, Tired); }

FString FS47Fatigue::KitLineOf(double Tired, double CupsToday, double Caffeine)
{
	if (Tired >= 0.45) return TEXT("Tired");
	const int32 N = FMath::RoundToInt(CupsToday);
	if (N >= 1 && Caffeine > 0.25) return FString::Printf(TEXT("Wide awake, coffee %d cup%s"), N, N == 1 ? TEXT("") : TEXT("s"));
	return FString();
}

FString FS47Fatigue::KitLine() const { return Live() ? KitLineOf(Level(), Cups, Caf) : FString(); }

void FS47Fatigue::RollDay()
{
	if (!M || !M->Freeform) return;
	const int32 Day = CoffeeDayOf(M->Freeform->Clock());
	if (Day != CupDay) { CupDay = Day; Cups = 0; }
}

void FS47Fatigue::TiredThought()
{
	// D46-20261110: now and then, in a heavy spell, the player's own thought (one through the queue; never twice in four minutes)
	// UI-20261112 (Tom: more occasions): its lines are S47Thoughts' pools now, "tired" and, from ExhaustedFrom, "exhausted", under
	// the thoughts' own cooldown as well (one in 90 real seconds), so a tired thought never comes on top of another.
	if (!M || !M->UI || T - ThoughtAt < 240) return;
	if (S47Thoughts::Think(M, S47Thoughts::TiredKey(Level()))) { ThoughtAt = T; ++ThoughtNo; }
}

double FS47Fatigue::RunScaleOf(double Tired, double InWind)
{
	// tired, the run winds you: it eases off as the wind goes, a little even when fresh, never under the floor
	const double K = S47FatigueLocal::TiredK(Tired), W = FMath::Clamp(FMath::IsFinite(InWind) ? InWind : 1.0, 0.0, 1.0);
	return 1.0 - (1.0 - RunFloor) * K * (0.35 + 0.65 * (1.0 - W));
}
double FS47Fatigue::WindDrainOf(double Tired) { return S47FatigueLocal::TiredK(Tired) / S47FatigueLocal::WindEmptySeconds; }
double FS47Fatigue::StressRateOf(double Tired) { return 1.0 + 1.2 * S47FatigueLocal::Smooth(0.25, 1.0, Tired); }

double FS47Fatigue::EdgeAlphaOf(double Tired, double InSpell)
{
	// a faint shade at the rim, a little more in a heavy spell: lighter than a moderate strain's edge, so that the strain still reads
	const double K = S47FatigueLocal::TiredK(Tired);
	return FMath::Clamp(K * (0.08 + 0.10 * FMath::Clamp(InSpell, 0.0, 1.0)), 0.0, 0.2);
}

double FS47Fatigue::EdgeReachOf(double Tired, double InSpell)
{
	return Tired < 0.3 ? 0.0 : 0.1 + 0.08 * S47FatigueLocal::TiredK(Tired) + 0.06 * FMath::Clamp(InSpell, 0.0, 1.0);
}

double FS47Fatigue::LidsOf(double Tired, double InSpell, double InDroop, double Waking)
{
	// heavy lids in a spell (more the more tired), shut for a moment when the eyes close, heavy for a while after a short sleep
	const double Heavy = FMath::Lerp(0.12, 0.30, S47FatigueLocal::TiredK(Tired)) * FMath::Clamp(InSpell, 0.0, 1.0) * (Tired >= 0.3 ? 1.0 : 0.0);
	return FMath::Clamp(FMath::Max3(Heavy, 0.95 * FMath::Clamp(InDroop, 0.0, 1.0), 0.34 * FMath::Clamp(Waking, 0.0, 1.0)), 0.0, 0.97);
}

double FS47Fatigue::SpellGapOf(double Tired, double U)
{
	const double X = FMath::Clamp(FMath::IsFinite(U) ? U : 0.5, 0.0, 1.0);
	return FMath::Max(MinSpellGap, FMath::Lerp(FMath::Lerp(45.0, 95.0, X), FMath::Lerp(MinSpellGap, 50.0, X), S47FatigueLocal::TiredK(Tired)));
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

double FS47Fatigue::EdgeAlpha() const { return Live() ? EdgeAlphaOf(Level(), Spell) : 0.0; }
double FS47Fatigue::EdgeReach() const { return Live() ? EdgeReachOf(Level(), Spell) : 0.0; }

double FS47Fatigue::Lids() const
{
	if (!Live()) return 0.0;
	// waking: heavy at first and easing, two slow blinks in it
	double Waking = 0, Shut = Droop;
	if (WakeAge >= 0)
	{
		const auto Blink = [this](double At) { const double X = (WakeAge - At) / 1.3; return X > 0 && X < 1 ? FMath::Sin(X * UE_DOUBLE_PI) : 0.0; };
		Waking = 1 - S47FatigueLocal::Smooth(0, WakeSeconds, WakeAge);
		Shut = FMath::Max(Shut, 0.8 * FMath::Max(Blink(4), Blink(11)));
	}
	return LidsOf(Level(), Spell, Shut, Waking);
}

void FS47Fatigue::Begin(const TSharedPtr<FJsonObject>& Saved)
{
	// A new game: the player arrives in the morning, rested. A save before FATIGUE: the same.
	Awake = Saved.IsValid() ? FMath::Max(0.0, S47J::Num(Saved, TEXT("awake"), RestedAfterArrival)) : RestedAfterArrival;
	Caf = Saved.IsValid() ? FMath::Clamp(S47J::Num(Saved, TEXT("caffeine"), 0), 0.0, MostCaffeine) : 0.0;
	Mug = Saved.IsValid() ? FMath::Clamp(S47J::Int(Saved, TEXT("mug"), 0), 0, 2) : 0;
	Cups = Saved.IsValid() ? FMath::Clamp(S47J::Num(Saved, TEXT("cups"), 0), 0.0, 20.0) : 0.0;
	CupDay = Saved.IsValid() ? S47J::Int(Saved, TEXT("cupDay"), -100000) : -100000; ThoughtAt = -1e9; ThoughtNo = 0; // D46-20261110
	LastClock = -1; Droop = 0; DroopAge = -1; T = 0; bDev = false;
	SpellIn = 30; SpellAge = -1; Spell = 0; bSpellDroop = false; WakeAge = -1; YawnIn = -1;
	Wind = 1; BreathIn = 0; HeardFor = 0; SinceRun = 1e9; bHaveFeet = false; DinerAt = -1e9;
	ShowMug();
}

TSharedRef<FJsonObject> FS47Fatigue::Save() const
{
	TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
	O->SetNumberField(TEXT("awake"), FMath::RoundToDouble(Awake * 100) / 100);
	if (Caf > 0.005) O->SetNumberField(TEXT("caffeine"), FMath::RoundToDouble(Caf * 100) / 100);
	if (Mug > 0) O->SetNumberField(TEXT("mug"), Mug);
	if (Cups > 0) { O->SetNumberField(TEXT("cups"), Cups); O->SetNumberField(TEXT("cupDay"), CupDay); } // D46-20261110
	return O;
}

void FS47Fatigue::Stop()
{
	if (M && M->Player) M->Player->RunScale = 1;
	if (M && M->Player) M->Player->TiredWalk = 1;
	Droop = 0; DroopAge = -1; bDev = false; Spell = 0; SpellAge = -1; WakeAge = -1; YawnIn = -1; Wind = 1; HeardFor = 0; bHaveFeet = false;
}

void FS47Fatigue::ShowMug() const
{
	if (!M || !M->Web) return;
	M->Web->SetCoffee(Mug > 0);
	if (Mug > 0) M->Web->SetCoffeeLevel(Mug == 1 ? S47FatigueLocal::MugFull : S47FatigueLocal::MugHalf);
}

void FS47Fatigue::StepLids(double Dt, double Tired, bool bPlay)
{
	using namespace S47FatigueLocal;
	if (!FMath::IsFinite(Dt) || Dt <= 0) return;
	if (WakeAge >= 0 && (WakeAge += Dt) > WakeSeconds) WakeAge = -1;
	// the eyes shut for a moment (only inside a heavy spell, at its heaviest)
	if (DroopAge >= 0)
	{
		DroopAge += Dt;
		Droop = FMath::Sin(FMath::Clamp(DroopAge / 0.9, 0.0, 1.0) * UE_DOUBLE_PI);
		if (DroopAge >= 0.9) { DroopAge = -1; Droop = 0; }
	}
	// a heavy spell now and then: the lids come down slowly over a few seconds and lift again; nothing between them
	if (SpellAge >= 0)
	{
		SpellAge += Dt;
		const double X = FMath::Clamp(SpellAge / SpellLen, 0.0, 1.0);
		Spell = FMath::Square(FMath::Sin(X * UE_DOUBLE_PI));
		if (bSpellDroop && X >= 0.5) { bSpellDroop = false; DroopAge = 0; }
		if (X >= 1) { SpellAge = -1; Spell = 0; }
		return;
	}
	if (Tired < 0.3 || !bPlay) { SpellIn = FMath::Max(SpellIn, 8.0); return; }
	if ((SpellIn -= Dt) > 0) return;
	SpellIn = SpellGapOf(Tired, Rng.FRand());
	SpellAge = 0; Spell = 0; SpellLen = Rng.FRandRange(float(SpellLenMin), float(SpellLenMax));
	bSpellDroop = Tired > 0.7 && Rng.FRand() < 0.8 * Smooth(0.7, 1.0, Tired);
	// every other spell with a yawn, once it is more than a little
	if (Tired > 0.45 && (++Spells % 2) == 1) YawnIn = 0.4;
}

void FS47Fatigue::Yawn(double K) const
{
	// a yawn, close: a long breath in through the open mouth, a low voiced sigh at the top of it, and the breath out
	US47Audio* Audio = M ? M->Audio.Get() : nullptr;
	if (!Audio || !Audio->Ctx || !Audio->SfxBus || !Audio->NoiseBuffer()) return;
	const TSharedRef<FS47AudioContext> Ctx = Audio->Ctx.ToSharedRef();
	FS47AudioContext::FBatch Batch(*Ctx);
	const double At = Ctx->CurrentTime() + 0.05;
	{
		auto N = Ctx->CreateBufferSource(); N->Buffer = Audio->NoiseBuffer();
		auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Q.SetValue(0.8);
		Bp->Frequency.SetValueAtTime(650, At); Bp->Frequency.LinearRampToValueAtTime(1050, At + 1.5); Bp->Frequency.LinearRampToValueAtTime(480, At + 3.6);
		auto G = Ctx->CreateGain(); FS47AudioParam& V = G->Gain;
		V.SetValueAtTime(0, At); V.LinearRampToValueAtTime(0.20 * K, At + 1.4); V.LinearRampToValueAtTime(0.05 * K, At + 2.0);
		V.LinearRampToValueAtTime(0.16 * K, At + 2.4); V.LinearRampToValueAtTime(0, At + 3.7);
		N->Connect(Bp); Bp->Connect(G); G->Connect(Audio->SfxBus);
		N->Start(At, FMath::FRand()); N->Stop(At + 3.8);
	}
	{
		auto O = Ctx->CreateOscillator(); O->SetType(TEXT("sawtooth"));
		O->Frequency.SetValueAtTime(205, At + 1.2); O->Frequency.LinearRampToValueAtTime(150, At + 2.7);
		auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(700); Lp->Q.SetValue(0.7);
		auto G = Ctx->CreateGain(); FS47AudioParam& V = G->Gain;
		V.SetValueAtTime(0, At + 1.2); V.LinearRampToValueAtTime(0.035 * K, At + 1.6); V.LinearRampToValueAtTime(0.025 * K, At + 2.3); V.LinearRampToValueAtTime(0, At + 2.8);
		O->Connect(Lp); Lp->Connect(G); G->Connect(Audio->SfxBus);
		O->Start(At + 1.2); O->Stop(At + 2.9);
	}
}

void FS47Fatigue::WindedBreath(double K) const
{
	US47Audio* Audio = M ? M->Audio.Get() : nullptr;
	if (!Audio || !Audio->Ctx || !Audio->SfxBus || !Audio->NoiseBuffer()) return;
	const TSharedRef<FS47AudioContext> Ctx = Audio->Ctx.ToSharedRef();
	FS47AudioContext::FBatch Batch(*Ctx);
	S47Walkie::Breath(Ctx, Audio->SfxBus, Audio->NoiseBuffer(), Ctx->CurrentTime(), K);
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
	const bool bPlay = M->Mode == US47Main::EMode::Play && Live();
	const bool bWasSpell = SpellAge >= 0;
	StepLids(Dt, Tired, bPlay);
	if (!bWasSpell && SpellAge >= 0 && bPlay && Tired > 0.55) TiredThought(); // D46-20261110
	if (YawnIn >= 0 && (YawnIn -= Dt) < 0) { YawnIn = -1; if (bPlay) Yawn(FMath::Lerp(0.6, 1.0, TiredK(Tired))); }
	// the run: tired, it winds you; out of wind it eases off and the breath comes hard (heard a little way off), but it stays a run
	if (HeardFor > 0) HeardFor -= Dt;
	bool bRunning = false;
	if (M->Player)
	{
		const FVector Feet = M->Player->Pos;
		const bool bCab = M->World && M->World->bDriving;
		if (bHaveFeet && !bCab)
		{
			const double Pace = FVector2D::Distance(FVector2D(Feet.X, Feet.Z), FVector2D(LastFeet.X, LastFeet.Z)) / Dt;
			bRunning = Pace > M->Player->Speed * 1.25 && Pace < 12.0;
		}
		LastFeet = Feet; bHaveFeet = !bCab;
	}
	SinceRun = bRunning ? 0.0 : SinceRun + Dt;
	if (bRunning) Wind = FMath::Max(0.0, Wind - Dt * WindDrainOf(Tired));
	else Wind = FMath::Min(1.0, Wind + Dt / WindBackSeconds);
	if (bPlay && Wind < 0.5 && SinceRun < 2.5 && (BreathIn -= Dt) <= 0)
	{
		const double Out = 1.0 - Wind / 0.5;
		BreathIn = FMath::Lerp(1.7, 1.05, Out) * Rng.FRandRange(0.9f, 1.1f);
		WindedBreath(FMath::Lerp(0.45, 0.85, Out));
		HeardFor = 1.2; HeardReach = FMath::Lerp(1.5, 3.0, Out);
	}
	if (M->Player) { M->Player->RunScale = RunScaleOf(Tired, Wind); M->Player->TiredWalk = Live() ? WalkScaleOf(Tired) : 1.0; }
}

void FS47Fatigue::Slept(double Seconds, EBed Where)
{
	using namespace S47FatigueLocal;
	if (!FMath::IsFinite(Seconds) || Seconds <= 0) return;
	// the session's clock has moved already: none of this is counted again as time awake
	if (M && M->Freeform) LastClock = M->Freeform->Clock();
	const double CafAtBed = Caf;
	Caf *= FMath::Pow(0.5, Seconds / CaffeineHalfLife);
	if (Seconds < NapSeconds) { Awake += Seconds / 3600.0; return; }
	const double Was = Awake;
	Awake = AfterSleep(Awake, Seconds / 3600.0, Where, CafAtBed);
	Cups = 0; // D46-20261110: a real sleep starts the day's cups over
	const bool bWired = CafAtBed > WiredCaffeine;
	Droop = 0; DroopAge = -1; Spell = 0; SpellAge = -1; SpellIn = 40; Wind = 1;
	UE_LOG(LogS47Fatigue, Display, TEXT("S47: fatigue: slept %.1f h (%d), awake %.1f h -> %.1f h"), Seconds / 3600.0, int32(Where), Was, Awake);
	// waking from a short sleep: heavy-eyed for a while and a yawn, and a line that says why; no number
	if (Awake > 3.0)
	{
		WakeAge = 0; YawnIn = 2.5;
		if (Live() && M->UI)
			M->UI->Toast(bWired ? TEXT("You lay a long time with the coffee loud in you. Your eyes still sting.")
				: Where == EBed::Cot ? TEXT("The cot's frame dug into your back. Your eyes still sting.")
				: Awake > 6.5 ? TEXT("Not enough sleep. Your eyes still sting, and the night will be long.")
				: TEXT("A short sleep. Your eyelids are heavy. Enough for the evening, but the small hours will drag."), 5);
	}
}

void FS47Fatigue::Lost(double Seconds)
{
	if (!FMath::IsFinite(Seconds) || Seconds <= 0) return;
	if (M && M->Freeform) LastClock = M->Freeform->Clock();
	Awake += Seconds / 3600.0;
	Caf *= FMath::Pow(0.5, Seconds / CaffeineHalfLife);
}

void FS47Fatigue::Desk(double Hours)
{
	if (!FMath::IsFinite(Hours) || Hours <= 0) return;
	Awake += FMath::Min(Hours, 2.0);
	UE_LOG(LogS47Fatigue, Display, TEXT("S47: fatigue: desk work %.2f h, awake %.1f h"), Hours, Awake);
}

FString FS47Fatigue::BedLine(double Seconds, EBed Where) const
{
	const double Hours = FMath::Max(0.0, FMath::IsFinite(Seconds) ? Seconds : 0.0) / 3600.0;
	const TCHAR* Bed = Where == EBed::Motel ? TEXT("In the motel bed") : Where == EBed::Cot ? TEXT("On the cot in the truck bed") : TEXT("In the chair in the control room, the receiver ticking");
	if (Hours < S47FatigueLocal::NapSeconds / 3600.0) return TEXT("SLEEP / None. You sit with your eyes open.");
	const double Left = AfterSleep(Awake, Hours, Where, Caf);
	if (Caf > WiredCaffeine && Hours >= 2.0) return FString::Printf(TEXT("SLEEP / %s: the coffee is still loud in you. It will be a thin sleep."), Bed);
	if (Hours < 2.0) return FString::Printf(TEXT("SLEEP / %s: a doze, not a sleep. It takes the edge off."), Bed);
	if (Left <= 3.0) return FString::Printf(TEXT("SLEEP / %s: a real sleep. Enough for the evening and the night after."), Bed);
	if (Left <= 6.5) return FString::Printf(TEXT("SLEEP / %s: a short sleep. Enough for the evening; the small hours will drag."), Bed);
	return FString::Printf(TEXT("SLEEP / %s: not much of a sleep. You will be tired before the night is out."), Bed);
}

void FS47Fatigue::AtDiner()
{
	if (!M || !M->IsFreeform() || !M->Freeform) return;
	if (M->Evening && M->Evening->Commute()) return; // the evening's own supper scene
	if (M->Freeform->Townsfolk && M->Freeform->Townsfolk->Away(TEXT("waitress"))) return; // nobody pours it
	const double Clock = M->Freeform->Clock();
	// the pot is left in reach: one cup a visit, and a visit is a half hour or so
	if (Clock - DinerAt < 30 * 60) return;
	DinerAt = Clock;
	RollDay();
	Caf = FMath::Min(MostCaffeine, Caf + CupCaffeine * CupStrengthOf(Cups)); Cups += 1; // D46-20261110
	if (M->Audio) M->Audio->Pour(FVector::ZeroVector);
	UE_LOG(LogS47Fatigue, Display, TEXT("S47: fatigue: the Mesa's coffee, caffeine %.2f"), Caf);
}

bool FS47Fatigue::Use(const FString& Id)
{
	if (!M || !M->IsFreeform() || !M->Web || M->Web->Area() != TEXT("saro")) return false;
	if (Id == TEXT("coffeePot"))
	{
		if (Mug > 0) { if (M->UI) M->UI->Toast(TEXT("There is still coffee in your mug. The hot plate ticks."), 2.6f); return true; }
		if (M->Audio) M->Audio->Pour(M->Web->PropLocation(TEXT("coffeePot")));
		Mug = 1; ShowMug();
		if (M->UI) M->UI->Toast(TEXT("You fill the SARO mug. The coffee has been on the hot plate since the day shift."), 3);
		return true;
	}
	if (Id == TEXT("mug"))
	{
		if (Mug == 0) { if (M->UI) M->UI->Toast(TEXT("Empty. The coffee machine is in the corner."), 2.6f); return true; }
		RollDay(); // D46-20261110: diminishing returns within a day
		const double Part = Mug == 1 ? 1.0 : 0.5;
		Caf = FMath::Min(MostCaffeine, Caf + CupCaffeine * Part * CupStrengthOf(Cups)); Cups += Part;
		Mug = Mug == 1 ? 2 : 0; ShowMug();
		if (M->Audio) M->Audio->Play(TEXT("thud_soft"), 0.25f, M->Web->PropLocation(TEXT("mug")));
		if (M->UI) M->UI->Toast(Caf > WiredCaffeine ? TEXT("Another one. Your hands are not quite steady, and sleep will be slow to come.") : Mug == 2 ? TEXT("Burnt. It helps.") : TEXT("The last of it, gone cold."), 2.8f);
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
	Awake = Hours; Caf = 0; bDev = true; SpellIn = 3;
	if (M && M->Freeform) LastClock = M->Freeform->Clock();
}

FString FS47Fatigue::Describe() const
{
	return FString::Printf(TEXT("awake %.1f h, coffee %.2f, tired %.2f (%s), wind %.2f, run x%.2f, strain x%.2f"), Awake, Caf, Level(), Live() ? (bDev ? TEXT("F10") : TEXT("live")) : TEXT("off in a developer run"),
		Wind, RunScaleOf(Level(), Wind), StressRateOf(Level()));
}

bool FS47Fatigue::SelfTest(TArray<FString>& Lines)
{
	bool OK = true;
	const auto Check = [&](bool B, const FString& Why) { OK &= B; Lines.Add(FString::Printf(TEXT("%s fatigue: %s"), B ? TEXT("PASS") : TEXT("FAIL"), *Why)); };
	const double Noon = 12 * 3600.0, Four = 4 * 3600.0;
	Check(LevelOf(RestedAfterArrival, 0, Noon) == 0 && LevelOf(12, 0, Noon) == 0, TEXT("arrived rested, and a working day is not tiring yet"));
	Check(LevelOf(20, 0, Noon) > 0.2 && LevelOf(26, 0, Noon) > 0.85 && LevelOf(20, 0, Noon) < LevelOf(24, 0, Noon), TEXT("tired from about sixteen hours awake, worn out by twenty-eight"));
	Check(LevelOf(20, 0, Four) > LevelOf(20, 0, Noon) + 0.1 && LevelOf(6, 0, Four) == 0, TEXT("the small hours are the worst after a long day, nothing after a short one"));
	Check(LevelOf(22, 1, Noon) < LevelOf(22, 0, Noon) - 0.4 && LevelOf(30, 1.5, Noon) > 0.5, TEXT("coffee takes off about half of it and does not stand in for sleep"));
	Check(AfterSleep(20, 8, EBed::Motel) < AfterSleep(20, 8, EBed::Station) && AfterSleep(20, 8, EBed::Station) < AfterSleep(20, 8, EBed::Cot) && AfterSleep(20, 8, EBed::Station) < 0.5,
		TEXT("a night's sleep at the station undoes a long day; the motel bed best, the cot least"));
	Check(AfterSleep(20, 0.4, EBed::Station) > 20 && AfterSleep(20, 1, EBed::Station) > 15 && AfterSleep(20, 1, EBed::Station) < 20 && AfterSleep(20, 3, EBed::Station) < 8,
		TEXT("twenty minutes in the chair is no sleep, an hour's doze takes the edge off, three hours a good part of it"));
	{
		// FATIGUE-20261109 (M5 critic 3): three ordinary days. Up at eight, the evening's work, the small hours (nothing comes before about
		// two), REST UNTIL MORNING at half past three: four and a half hours. It must hold steady, not run into a debt from the second evening.
		double Aw = RestedAfterArrival, Evening = 0, Midnight = 0, Two = 0, Late = 0, Wake1 = 0, Wake3 = 0;
		FString Curve;
		for (int32 Day = 1; Day <= 3; ++Day)
		{
			Evening = FMath::Max(Evening, LevelOf(Aw + 11, 0, 19 * 3600.0)); // 19:00
			Midnight = FMath::Max(Midnight, LevelOf(Aw + 15, 0, 23 * 3600.0)); // 23:00
			Two = FMath::Max(Two, LevelOf(Aw + 18, 0, 2 * 3600.0)); // 02:00
			Late = FMath::Max(Late, LevelOf(Aw + 19.5, 0, 3.5 * 3600.0)); // 03:30
			Curve += FString::Printf(TEXT(" day %d: 19h %.2f, 23h %.2f, 02h %.2f, 03:30 %.2f;"), Day, LevelOf(Aw + 11, 0, 19 * 3600.0), LevelOf(Aw + 15, 0, 23 * 3600.0), LevelOf(Aw + 18, 0, 2 * 3600.0), LevelOf(Aw + 19.5, 0, 3.5 * 3600.0));
			Aw = AfterSleep(Aw + 19.5, 4.5, EBed::Station);
			if (Day == 1) Wake1 = Aw;
			if (Day == 3) Wake3 = Aw;
		}
		Check(Evening < 0.05 && Midnight < 0.15 && Two > 0.2 && Two < 0.55 && Late < 0.7 && FMath::Abs(Wake3 - Wake1) < 0.5,
			TEXT("three ordinary days (up through the small hours, asleep 03:30 to 08:00) hold steady: fine in the evening, a little tired at two, tired by the end;") + Curve);
		// the same days with the cot out on the land, or up until five: tired sooner, but it settles rather than running away
		double Cot = RestedAfterArrival, Five = RestedAfterArrival;
		for (int32 Day = 1; Day <= 6; ++Day) { Cot = AfterSleep(Cot + 19.5, 4.5, EBed::Cot); Five = AfterSleep(Five + 21, 3, EBed::Station); }
		const double CotThen = Cot, FiveThen = Five;
		Cot = AfterSleep(Cot + 19.5, 4.5, EBed::Cot); Five = AfterSleep(Five + 21, 3, EBed::Station);
		Check(Cot > Wake3 && Five > Wake3 && FMath::Abs(Cot - CotThen) < 0.2 && FMath::Abs(Five - FiveThen) < 0.2 && LevelOf(Five + 11, 0, 19 * 3600.0) < 0.2,
			TEXT("a cot every night, or up until five every night: more tired, never a debt that grows without end, still fit at the start of the evening"));
	}
	{
		// D46-20261110: coffee in a day: each cup gives less, the walk slows a little when tired, the notebook says it in a few words, a real sleep starts over
		Check(CupStrengthOf(0) == 1 && CupStrengthOf(1) < CupStrengthOf(0) && CupStrengthOf(3) < CupStrengthOf(1) && CupStrengthOf(50) >= 0.2 && CupStrengthOf(std::numeric_limits<double>::quiet_NaN()) == 1,
			TEXT("D46: each cup in a day gives less than the one before, never under a fifth"));
		Check(WalkScaleOf(0.3) == 1 && WalkScaleOf(1) < 1 && WalkScaleOf(1) >= 0.85, TEXT("D46: a tired walk is a little slower, nothing until properly tired"));
		Check(KitLineOf(0.7, 0, 0) == TEXT("Tired") && KitLineOf(0.1, 2, 0.9) == TEXT("Wide awake, coffee 2 cups") && KitLineOf(0.1, 1, 0.5) == TEXT("Wide awake, coffee 1 cup") && KitLineOf(0.1, 0, 0).IsEmpty() && KitLineOf(0.1, 2, 0.05).IsEmpty(),
			TEXT("D46: the notebook line is Tired, or Wide awake with the cups, or nothing"));
		FS47Fatigue Day(nullptr); Day.Begin(nullptr); Day.Cups = 3; Day.Awake = 20; Day.Slept(5 * 3600.0, EBed::Station); const bool bReset = Day.Cups == 0;
		const TSharedRef<FJsonObject> Kept = Day.Save(); Day.Cups = 2; Day.CupDay = 4; const TSharedRef<FJsonObject> Held = Day.Save(); FS47Fatigue Back(nullptr); Back.Begin(Held);
		Check(bReset && !Kept->HasField(TEXT("cups")) && Back.Cups == 2 && Back.CupDay == 4,
			TEXT("D46: a real sleep starts the cups over; the cups are saved only when there are some"));
		Check(CoffeeDayOf(7 * 3600.0) != CoffeeDayOf(5 * 3600.0),
			TEXT("D46: the coffee day turns at six"));
	}
	{
		// the run: tired and winded it eases off, but it stays a run (heard as one) and faster than a figure that sees you and hunts
		const double Walk = GetDefault<AS47Walker>() ? GetDefault<AS47Walker>()->Speed : 1.9;
		double Slowest = 1e9;
		for (double Tr = 0; Tr <= 1.0001; Tr += 0.05) for (double W = 0; W <= 1.0001; W += 0.1) Slowest = FMath::Min(Slowest, Walk * (1 + 0.6 * RunScaleOf(Tr, W)));
		Check(Slowest > FS47RoswellBrain::HuntSeenSpeed + 0.1 && S47Sneak::StepReach(Slowest, 0) == S47Sneak::StepReach(Walk * 1.6, 0) && S47Sneak::StepReach(Slowest, 0) > S47Sneak::StepReach(Walk, 0),
			FString::Printf(TEXT("a worn-out, winded run is %.2f m/s: still faster than a hunting figure's %.2f and still heard as a run"), Slowest, FS47RoswellBrain::HuntSeenSpeed));
		Check(RunScaleOf(0, 0) == 1 && RunScaleOf(1, 0) < RunScaleOf(1, 1) && RunScaleOf(1, 1) < 1 && WindDrainOf(0) == 0 && WindDrainOf(1) > WindDrainOf(0.6) && StressRateOf(0) == 1 && StressRateOf(1) > 2,
			TEXT("tired: the run winds you and eases off (rested, never), and the strain comes on twice as fast"));
	}
	{
		// the edge: nothing when rested; tired, a faint rim always lighter than a moderate strain's, so that the strain reads over it
		bool bQuietRested = EdgeAlphaOf(0, 1) == 0 && EdgeReachOf(0.1, 1) == 0 && LidsOf(0.1, 1, 0, 0) == 0;
		Check(bQuietRested && EdgeAlphaOf(1, 1) < FS47Stress::VignetteAlpha(0.5) && EdgeAlphaOf(1, 1) > 0 && LidsOf(0.9, 1, 0, 0) > 0.2 && LidsOf(0.9, 1, 1, 0) > 0.9 && LidsOf(0, 0, 0, 1) > 0.3,
			TEXT("the edge: nothing when rested; tired, heavy lids in a spell and a rim lighter than the strain's; shut a moment when the eyes close; heavy on waking"));
		// ten minutes worn out: the heavy spells come seldom (never closer than MinSpellGap) and slowly, no flicker between them
		FS47Fatigue Body(nullptr); Body.Begin(nullptr);
		const double Dt = 1 / 60.0, Minutes = 10;
		int32 Events = 0, Turns = 0; double Prev = 0, PrevD = 0, Fastest = 0; bool bDown = false;
		for (double Tm = 0; Tm < Minutes * 60; Tm += Dt)
		{
			Body.StepLids(Dt, 0.95, true);
			const double L = LidsOf(0.95, Body.Spell, Body.Droop, 0);
			if (!bDown && L > 0.05) { bDown = true; ++Events; }
			if (bDown && L < 0.01) bDown = false;
			const double D = L - Prev;
			if (FMath::Abs(D) > 1e-5 && FMath::Abs(PrevD) > 1e-5 && (D > 0) != (PrevD > 0)) ++Turns;
			if (FMath::Abs(D) > 1e-5) PrevD = D;
			if (Body.Droop <= 0) Fastest = FMath::Max(Fastest, FMath::Abs(D) / Dt);
			Prev = L;
		}
		const int32 Most = int32(Minutes * 60 / MinSpellGap) + 1;
		Check(Events >= 3 && Events <= Most && Turns <= Events * 4 + 2 && Fastest < 0.4,
			FString::Printf(TEXT("ten minutes worn out: %d heavy spells (at most %d), each a slow close and open (%d turns, lids at most %.2f a second), no flicker"), Events, Most, Turns, Fastest));
		FS47Fatigue Fresh(nullptr); Fresh.Begin(nullptr); int32 Any = 0;
		for (double Tm = 0; Tm < 300; Tm += Dt) { Fresh.StepLids(Dt, 0.1, true); Any += Fresh.Spell > 0 ? 1 : 0; }
		Check(Any == 0, TEXT("rested, no spells at all"));
	}
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
		Check(At0 == RestedAfterArrival && After < Before && Body.Awake < 3 && Body.Caf < CupCaffeine * 0.3, TEXT("up all night, a cup at two helps; six hours' sleep at the station leaves you rested and the coffee gone"));
		FS47Fatigue Nap(nullptr); Nap.Begin(nullptr); Nap.Awake = 20; Nap.Slept(20 * 60.0, EBed::Station);
		Check(Nap.Awake > 20, TEXT("twenty minutes in the chair is no sleep"));
		FS47Fatigue Short(nullptr); Short.Begin(nullptr); Short.Awake = 22; Short.Slept(2.5 * 3600.0, EBed::Station);
		Check(Short.WakeAge == 0 && Short.YawnIn > 0 && Body.WakeAge < 0, TEXT("waking from a short sleep is heavy-eyed, with a yawn; from a real one it is not"));
		// M7 (C): coffee has its price in the sleep: wired at lying down, the same hours in the same chair pay off less, and the bed says so
		FS47Fatigue Wired(nullptr); Wired.Begin(nullptr); Wired.Awake = 21.5; Wired.Caf = 1.4;
		FS47Fatigue Calm(nullptr); Calm.Begin(nullptr); Calm.Awake = 21.5; Calm.Caf = 0.6;
		const FString WiredLine = Wired.BedLine(4.5 * 3600, EBed::Station);
		Wired.Slept(4.5 * 3600.0, EBed::Station); Calm.Slept(4.5 * 3600.0, EBed::Station);
		Check(AfterSleep(21.5, 4.5, EBed::Station, 1.5) > AfterSleep(21.5, 4.5, EBed::Station, 0) + 1.5 && AfterSleep(21.5, 4.5, EBed::Station, 0.9) == AfterSleep(21.5, 4.5, EBed::Station, 0)
			&& Wired.Awake > Calm.Awake + 1 && Wired.WakeAge == 0 && WiredLine.Contains(TEXT("thin sleep")),
			FString::Printf(TEXT("M7: more than one cup's worth of coffee at lying down is a thin sleep (%.1f h awake after it, %.1f h without) and the bed says so; one cup costs nothing"), Wired.Awake, Calm.Awake));
		FS47Fatigue Bed(nullptr); Bed.Begin(nullptr); Bed.Awake = 21.5;
		Check(Bed.BedLine(4.5 * 3600, EBed::Station).Contains(TEXT("real sleep")) && Bed.BedLine(3 * 3600, EBed::Station).Contains(TEXT("short sleep")) && Bed.BedLine(3600, EBed::Station).Contains(TEXT("doze"))
			&& !Bed.BedLine(4.5 * 3600, EBed::Station).Contains(TEXT("4")), TEXT("the bed says what kind of sleep it will be, in words"));
		const TSharedRef<FJsonObject> Saved = Body.Save();
		FS47Fatigue Again(nullptr); Again.Begin(Saved);
		Check(FMath::IsNearlyEqual(Again.Awake, Body.Awake, 0.01), TEXT("the hours awake are saved"));
	}
	return OK;
}
