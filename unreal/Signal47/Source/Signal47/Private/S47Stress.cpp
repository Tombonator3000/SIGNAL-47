#include "S47Stress.h"

#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "S47Audio.h"
#include "S47FieldCamera.h"
#include "S47Fatigue.h"
#include "S47FieldRadio.h"
#include "S47Flashlight.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Omen.h"
#include "S47Soundscape.h"
#include "S47Sky.h"
#include "S47Story.h"
#include "S47Taken.h"
#include "S47Walkie.h"
#include "S47Walker.h"
#include "S47World.h"
#include "S47WorldLife.h"
#include <limits>

DEFINE_LOG_CATEGORY_STATIC(LogS47Stress, Log, All);

namespace S47StressLocal
{
	// how fast it rises (the time constant, seconds) and how long it takes to fall from full to nothing
	constexpr double RiseSeconds = 3.5, FallSafeSeconds = 90, FallSeconds = 360;
	// after being taken: how long the aftermath lasts, and how strong it is at first. AFTER-TAKEN-20261109 (Tom, d37: "some sounds
	// hang on in a loop when you wake up"): it was 0.75, over the breathing's mark, so the hard breath went on every few seconds for
	// minutes after waking (tired, up to eight). Now they come to dazed, under the breathing: a faint edge that fades, and quiet.
	constexpr double AfterTakenSeconds = 420, AfterTakenLevel = 0.3;
	// nothing shows below this; the edge and the narrower view come in above it
	constexpr double EdgeFrom = 0.15, FovFrom = 0.35, BreathFrom = 0.4;
	// the dark and the hush together stay under the breathing: 1 - (1 - DarkMost) * (1 - HushWorry) < BreathFrom
	constexpr double DarkLeast = 0.12, DarkMost = 0.26, HushWorry = 0.15;
}

double FS47Stress::Target(const FS47StressIn& In)
{
	double Calm = 1; // the chance that none of the worries holds
	auto Worry = [&Calm](double T) { Calm *= 1.0 - FMath::Clamp(T, 0.0, 1.0); };
	// something dangerous near, as near as it is: full on top of it, nothing at its reach (FS47Omen's own falloff)
	if (In.Danger > 0) Worry(0.9 * FMath::Clamp(In.Danger, 0.0, 1.0));
	const double Near = FMath::Clamp(In.Presence, 0.0, 1.0);
	if (Near > 0) Worry(0.95 * Near * (In.bHunting ? 1.0 : In.bSeen ? 0.85 : 0.6));
	if (In.bDark && !In.bSafe) Worry(FMath::Lerp(S47StressLocal::DarkLeast, S47StressLocal::DarkMost, FMath::Clamp(In.Darkness, 0.0, 1.0)));
	// the silence (SOUNDSCAPE-20261108): felt a little, even with nothing to see; a gust does it too
	if (In.bHush) Worry(S47StressLocal::HushWorry);
	if (In.bTaken) Worry(1.0);
	// AFTER-TAKEN-20261109: the aftermath is a floor, not one more worry on top: with the dark and the hush (kept under the breathing
	// on their own) it would have breathed again for no new reason
	const double After = !In.bTaken && In.SinceTaken < S47StressLocal::AfterTakenSeconds
		? S47StressLocal::AfterTakenLevel * (1.0 - FMath::Max(0.0, In.SinceTaken) / S47StressLocal::AfterTakenSeconds) : 0.0;
	return FMath::Max(1.0 - Calm, After);
}

double FS47Stress::Advance(double Cur, double To, double Dt, bool bSafe, double Tired)
{
	if (!FMath::IsFinite(Dt) || Dt <= 0) return Cur;
	// tired, it comes on faster and leaves more slowly (FATIGUE-20261108)
	const double Rate = FS47Fatigue::StressRateOf(FMath::Clamp(FMath::IsFinite(Tired) ? Tired : 0.0, 0.0, 1.0));
	if (To > Cur) return FMath::Min(To, Cur + (To - Cur) * (1.0 - FMath::Exp(-Dt * Rate / S47StressLocal::RiseSeconds)));
	return FMath::Max(To, Cur - Dt / (Rate * (bSafe ? S47StressLocal::FallSafeSeconds : S47StressLocal::FallSeconds)));
}

double FS47Stress::VignetteAlpha(double Strain)
{
	return 0.72 * FMath::SmoothStep(S47StressLocal::EdgeFrom, 1.0, Strain);
}

double FS47Stress::VignetteReachOf(double Strain)
{
	return Strain <= S47StressLocal::EdgeFrom ? 0.0 : FMath::Lerp(0.12, 0.34, FMath::SmoothStep(S47StressLocal::EdgeFrom, 1.0, Strain));
}

double FS47Stress::FovScaleOf(double Strain)
{
	return 1.0 - 0.10 * FMath::SmoothStep(S47StressLocal::FovFrom, 1.0, Strain);
}

double FS47Stress::BreathGap(double Strain)
{
	return Strain < S47StressLocal::BreathFrom ? 0.0 : FMath::Lerp(6.5, 1.6, FMath::SmoothStep(S47StressLocal::BreathFrom, 1.0, Strain));
}

double FS47Stress::BreathStrength(double Strain)
{
	return Strain < S47StressLocal::BreathFrom ? 0.0 : 0.12 + 0.5 * FMath::SmoothStep(S47StressLocal::BreathFrom, 1.0, Strain);
}

double FS47Stress::OnWaking(double Strain)
{
	return FMath::Clamp(FMath::IsFinite(Strain) ? FMath::Min(Strain, S47StressLocal::AfterTakenLevel) : 0.0, 0.0, 1.0);
}

double FS47Stress::BreathReach(double Strain)
{
	// near panic only: up to three and a half metres, in the open (FS47RoswellBrain takes less through a wall)
	return Strain < 0.6 ? 0.0 : FMath::Lerp(1.5, 3.5, FMath::SmoothStep(0.6, 1.0, Strain));
}

void FS47Stress::Reset()
{
	S = 0; ReadIn = 0; BreathIn = 2; SinceTaken = 1e9; SinceBreath = 1e9; bWasTaken = false; Inputs = FS47StressIn();
	if (M && M->Player) M->Player->FovScale = 1.f;
}

FS47StressIn FS47Stress::Read()
{
	FS47StressIn R;
	if (!M) return R;
	// something dangerous near the player, by how near (FS47Omen: real sources only, at the eyes); not that something is going on
	// somewhere in the same area (the Director's rest block knew no distance)
	if (M->IsFreeform() && M->Omen.IsValid()) R.Danger = M->Omen->Level();
	if (M->FieldRadio)
	{
		R.Presence = M->FieldRadio->PresenceLevel();
		R.bSeen = M->FieldRadio->PresenceSeen();
		R.bHunting = M->FieldRadio->PresenceHunting();
	}
	const bool bAbducted = (M->Night && M->Night->Hooks.AbductionBusy && M->Night->Hooks.AbductionBusy())
		|| (M->IsFreeform() && M->Freeform && M->Freeform->Taken && M->Freeform->Taken->Busy());
	R.bTaken = bAbducted;
	// the aftermath is counted here, from the moment it ends; and the strain comes down to it at once, not over minutes
	if (bAbducted) { bWasTaken = true; SinceTaken = 0; }
	else if (bWasTaken) { bWasTaken = false; SinceTaken = 0; S = OnWaking(S); BreathIn = 1.5; }
	// gone (sound and picture): nothing is breathing where the player can hear it
	R.bAway = M->IsFreeform() && M->Freeform && M->Freeform->Taken && M->Freeform->Taken->Stage() == FS47Taken::EStage::Away;
	R.SinceTaken = SinceTaken;
	const bool bOutdoors = M->Space == TEXT("yard") && !(M->World && M->World->bDriving);
	const double Daylight = M->Life && M->Life->Sky.IsValid() ? M->Life->Sky->Daylight() : 1.0;
	const bool bLamp = M->Night && M->Night->Flashlight.IsValid() && M->Night->Flashlight->Intensity() > 1.0;
	if (bOutdoors && Daylight < 0.25 && !bLamp) { R.bDark = true; R.Darkness = 1.0 - Daylight / 0.25; }
	const FString Area = M->World ? M->World->Area : FString(TEXT("saro"));
	R.bSafe = M->Space == TEXT("room") && Area == TEXT("saro");
	R.bHush = M->IsFreeform() && M->Soundscape.IsValid() && M->Soundscape->Hushed();
	R.Tired = M->IsFreeform() && M->Freeform && M->Freeform->Fatigue ? M->Freeform->Fatigue->Level() : 0.0;
	return R;
}

void FS47Stress::Step(double Dt)
{
	if (!M) return;
	if (!bLogChecked) { bLogChecked = true; bLog = FParse::Param(FCommandLine::Get(), TEXT("S47StressLog")); }
	if (M->Mode != US47Main::EMode::Play || !M->Player)
	{
		if (S > 0 || ReadIn != 0) Reset();
		return;
	}
	// the strain is game time: a menu, a paper or the notebook holds it as it is
	const bool bHeld = M->bPausedByMenu || M->ModalOpen();
	if (!bHeld && FMath::IsFinite(Dt) && Dt > 0)
	{
		if (SinceTaken < 1e8) SinceTaken += Dt;
		if (SinceBreath < 1e8) SinceBreath += Dt;
		ReadIn -= Dt;
		if (ReadIn <= 0) { ReadIn = 0.25; Inputs = Read(); }
		else { Inputs.SinceTaken = SinceTaken; }
		const double Was = S;
		S = FMath::Clamp(Advance(S, Target(Inputs), Dt, Inputs.bSafe && Inputs.Danger <= 0.02 && Inputs.Presence <= 0, Inputs.Tired), 0.0, 1.0);
		if (bLog && (int32(Was * 10) != int32(S * 10)))
		{
			UE_LOG(LogS47Stress, Display, TEXT("S47: stress %.2f (danger %.2f, presence %.2f, dark %d, hush %d, taken %d, safe %d)"), S, Inputs.Danger, Inputs.Presence, Inputs.bDark, Inputs.bHush, Inputs.bTaken, Inputs.bSafe);
		}
		Breathe(Dt);
	}
	// the narrower view: not through the field camera's own lens
	const bool bRaised = M->FieldCamera && M->FieldCamera->bRaised;
	M->Player->FovScale = bRaised ? 1.f : float(FovScaleOf(S));
}

void FS47Stress::Breathe(double Dt)
{
	const double Gap = Inputs.bAway ? 0.0 : BreathGap(S);
	if (Gap <= 0) { BreathIn = 1.5; return; } // (the first breath comes a moment after it passes the mark)
	BreathIn -= Dt;
	if (BreathIn > 0) return;
	BreathIn = Gap * FMath::FRandRange(0.85f, 1.15f);
	const double K = BreathStrength(S);
	++BreathCount; SinceBreath = 0;
	if (bLog)
	{
		UE_LOG(LogS47Stress, Display, TEXT("S47: stress breath %d at %.2f (strength %.2f)"), BreathCount, S, K);
	}
	if (OnBreath) OnBreath(S, K);
	// the breathing already in the game (the walkie's, close to the microphone), into the sound effects, as chapter six's own is
	US47Audio* Audio = M->Audio.Get();
	if (Audio && Audio->Ctx && Audio->SfxBus && Audio->NoiseBuffer())
	{
		const TSharedRef<FS47AudioContext> Ctx = Audio->Ctx.ToSharedRef();
		FS47AudioContext::FBatch Batch(*Ctx);
		S47Walkie::Breath(Ctx, Audio->SfxBus, Audio->NoiseBuffer(), Ctx->CurrentTime(), K);
	}
}

bool FS47Stress::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s stress: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bCond; };
	FS47StressIn In;
	Check(Target(In) == 0, TEXT("a calm place has no strain to head for"));
	In.Danger = 1; const double Event = Target(In);
	In = FS47StressIn(); In.Presence = 1; In.bHunting = true; const double Hunter = Target(In);
	In.bHunting = false; In.bSeen = true; const double Seen = Target(In);
	In.bSeen = false; const double Heard = Target(In);
	Check(Event > 0.8 && Hunter > Seen && Seen > Heard && Heard > 0.5 && Hunter < 1, TEXT("something dangerous or a figure close by raises it; a hunting one more than a seen one, a seen one more than a felt one"));
	In = FS47StressIn(); In.Danger = 0.15; const double FarOff = Target(In);
	In.Danger = 0; const double Nothing = Target(In);
	Check(FarOff < S47StressLocal::BreathFrom && Nothing == 0 && Event > S47StressLocal::BreathFrom, TEXT("something dangerous far off is felt a little, never as hard breath; only near is"));
	In = FS47StressIn(); In.Presence = 0.5; const double Half = Target(In);
	In.Presence = 0.2; const double Far = Target(In);
	Check(Far < Half && Far > 0, TEXT("a figure further off strains less"));
	In = FS47StressIn(); In.bDark = true; In.Darkness = 1; const double Dark = Target(In);
	In.Darkness = 0.1; const double Dusk = Target(In);
	Check(Dark > Dusk && Dusk > 0.1 && Dark < S47StressLocal::BreathFrom && BreathGap(Dark) == 0, TEXT("the dark outside without a lamp is a small unease, under the breathing"));
	In.bSafe = true; Check(Target(In) == 0, TEXT("indoors at SARO the dark is no worry"));
	In = FS47StressIn(); In.bHush = true; const double Hush = Target(In);
	Check(Hush > 0.1 && Hush < Dark && Hush < Event, TEXT("the night going quiet is a small unease, less than the dark or a danger"));
	In.bDark = true; In.Darkness = 1;
	Check(Target(In) < S47StressLocal::BreathFrom && BreathGap(Target(In)) == 0, TEXT("in the dark with the crickets stopped it is uneasy, but the breathing waits for something near"));
	Check(BreathReach(0.5) == 0 && BreathReach(0.6) > 0 && BreathReach(1) <= 3.5 && BreathReach(1) > BreathReach(0.7), TEXT("hard breathing near panic is heard a few metres off, no more"));
	In = FS47StressIn(); In.Danger = 1; In.Presence = 0.8; In.bDark = true; In.bTaken = true;
	Check(Target(In) <= 1.0 && Target(In) >= Event, TEXT("worries add up as chances do and never pass 1"));
	In = FS47StressIn(); In.bTaken = true; Check(Target(In) == 1.0, TEXT("being taken is the most there is"));
	In = FS47StressIn(); In.SinceTaken = 0; const double Fresh = Target(In);
	In.SinceTaken = 200; const double Later = Target(In);
	In.SinceTaken = 1e6; const double Gone = Target(In);
	Check(Fresh > Later && Later > 0 && Gone == 0 && Fresh < S47StressLocal::BreathFrom && Fresh > S47StressLocal::EdgeFrom, TEXT("after being taken it lingers as a faint edge, then fades out by itself"));
	// AFTER-TAKEN-20261109: from the moment they come to, no breath is heard again from the taking: from full strain, tired out,
	// in the open and away from SARO, ten minutes of the aftermath at a quarter second a step
	{
		bool bQuiet = true, bEdge = false;
		double Now = OnWaking(1.0);
		for (int32 I = 0; I < 2400; ++I)
		{
			FS47StressIn After; After.SinceTaken = I * 0.25; After.Tired = 1.0; After.bDark = true; After.Darkness = 1.0; After.bHush = I < 160;
			Now = Advance(Now, Target(After), 0.25, false, After.Tired);
			bQuiet &= BreathGap(Now) == 0 && BreathStrength(Now) == 0 && BreathReach(Now) == 0;
			bEdge |= I < 40 && VignetteAlpha(Now) > 0;
		}
		Check(bQuiet && bEdge && OnWaking(0.1) == 0.1 && OnWaking(std::numeric_limits<double>::quiet_NaN()) == 0, TEXT("after waking from being taken no breathing goes on, only a faint edge that fades"));
		// the old aftermath, for the measure: 0.75 at first breathed on for minutes
		Check(BreathGap(0.75) > 0, TEXT("the measure catches the old aftermath's breathing"));
	}
	// the way up and the way down
	double Up = 0;
	for (int32 I = 0; I < 40; ++I) Up = Advance(Up, 1.0, 0.1, false);
	Check(Up > 0.6 && Up < 1.0, TEXT("it rises within a few seconds, and never past where it heads"));
	Check(Advance(0.9, 0.0, 1.0, true) < 0.9 && Advance(0.9, 0.0, 1.0, true) > 0.85, TEXT("it falls slowly, not at once"));
	Check(Advance(0.9, 0.0, 10.0, true) < Advance(0.9, 0.0, 10.0, false), TEXT("it falls faster in a safe place"));
	Check(Advance(0.2, 0.5, 0, false) == 0.2 && Advance(0.5, 0.0, -1, true) == 0.5 && Advance(0.5, 0.3, 1000, false) == 0.3, TEXT("no time, no change; it never falls below where it heads"));
	Check(Advance(0.1, 1.0, 1.0, false, 1) > Advance(0.1, 1.0, 1.0, false, 0) + 0.1 && Advance(0.9, 0.0, 10.0, true, 1) > Advance(0.9, 0.0, 10.0, true, 0) && Advance(0.1, 1.0, 1.0, false, 0) == Advance(0.1, 1.0, 1.0, false),
		TEXT("tired (FATIGUE-20261108): the strain comes on faster and goes more slowly; rested, as before"));
	double Down = 1; int32 Steps = 0;
	while (Down > 0.05 && Steps < 100000) { Down = Advance(Down, 0.0, 0.5, true); ++Steps; }
	Check(Steps * 0.5 > 60 && Steps * 0.5 < 120, TEXT("from full to calm in about a minute and a half indoors at SARO"));
	// what shows
	Check(VignetteAlpha(0) == 0 && VignetteAlpha(0.14) == 0 && VignetteReachOf(0.1) == 0 && FovScaleOf(0.3) == 1.0 && BreathGap(0.39) == 0 && BreathStrength(0.1) == 0,
		TEXT("nothing shows in calm stretches: no edge, no narrower view, no breathing"));
	bool bGrows = true; double LA = 0, LR = 0, LF = 1;
	for (int32 I = 0; I <= 100; ++I)
	{
		const double L = I / 100.0;
		bGrows &= VignetteAlpha(L) >= LA && VignetteReachOf(L) >= LR && FovScaleOf(L) <= LF;
		LA = VignetteAlpha(L); LR = VignetteReachOf(L); LF = FovScaleOf(L);
	}
	Check(bGrows && VignetteAlpha(1) > 0.6 && VignetteAlpha(1) < 0.8 && VignetteReachOf(1) < 0.4 && FovScaleOf(1) >= 0.89 && FovScaleOf(1) < 0.91,
		TEXT("the edge darkens and reaches further, and the view narrows by a tenth at most, as the strain grows"));
	Check(BreathGap(1) < BreathGap(0.5) && BreathGap(1) > 1.0 && BreathStrength(1) > BreathStrength(0.5) && BreathStrength(1) < 1.0, TEXT("the breathing comes faster and heavier as the strain grows"));
	// the object, with no game behind it: nothing happens and nothing breaks
	FS47Stress Quiet(nullptr); Quiet.Step(1.0); Quiet.Reset();
	Check(Quiet.Level() == 0 && Quiet.Vignette() == 0 && Quiet.Breaths() == 0 && Quiet.HeardWithin() == 0, TEXT("without a game it stays calm"));
	return bOk;
}
