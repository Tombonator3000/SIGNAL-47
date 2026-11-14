#include "S47Calm.h"
#include "S47Audio.h"
#include "S47EventScript.h"
#include "S47FieldRadio.h"
#include "S47Freeform.h"
#include "S47FarmSite.h"
#include "S47FreeformDirector.h"
#include "S47Main.h"
#include "S47NightRhythm.h"
#include "S47Rhythm.h"
#include "S47Script.h"
#include "S47StoryDirector.h"
#include "S47Taken.h"
#include "S47Three.h"
#include "S47TubeFlicker.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Calm, Log, All);

namespace
{
	const TCHAR* const AlarmIds[] = { TEXT("alarm_cow"), TEXT("alarm_shed_door"), TEXT("alarm_jackrabbit"), TEXT("alarm_fridge"),
		TEXT("alarm_pipe_tick"), TEXT("alarm_printer_settle"), TEXT("alarm_vent_rattle") }; // N1110 (pace): three indoor ones besides the refrigerator
	// The night hour of a clock time: 19.5 to 25.5 is 19:30 to 01:30, so that midnight is 24.
	double NightHourOf(double Clock)
	{
		const double H = FMath::Fmod(FMath::Max(0., Clock), 86400.) / 3600.;
		return H < 12. ? H + 24. : H;
	}
}

int32 FS47Calm::AlarmsFor(int32 Seed, int32 Night)
{
	return FS47NightRhythm::Roll(Seed, Night, 0x46414c53u) < .45f ? 2 : 1;
}

double FS47Calm::SlotHour(int32 Seed, int32 Night, int32 N)
{
	const float R = FS47NightRhythm::Roll(Seed, Night, 0x534c4f54u + uint32(N));
	// the first between 20:00 and 23:00, the second between 23:30 and 01:15
	return N <= 0 ? 20. + 3. * R : 23.5 + 1.75 * R;
}

double FS47Calm::ReliefDelay(int32 Seed, int32 Day)
{
	return 480. + 120. * FS47NightRhythm::Roll(Seed, Day, 0x52454c46u);
}

TArray<FString> FS47Calm::AlarmOrder(int32 Seed, int32 Night)
{
	TArray<FString> Out;
	for (const TCHAR* Id : AlarmIds) Out.Add(Id);
	// a seeded rotation: the same each time that night is asked
	const int32 Turn = int32(FS47NightRhythm::Roll(Seed, Night, 0x4f524452u) * Out.Num()) % Out.Num();
	for (int32 I = 0; I < Turn; ++I) { const FString First = Out[0]; Out.RemoveAt(0); Out.Add(First); } // (a copy: Add may not take its own element)
	return Out;
}

void FS47Calm::Reset()
{
	Night = Played = 0;
	NextTry = 0;
	DangerEnd = -1e9;
	LastAlarm = -1e9;
	LastFridgeDay = 0;
	LastTubeNight = 0;
	ReliefAt = -1;
	PrevBusy = NAME_None;
	bPrevDanger = false;
}

bool FS47Calm::Quiet(double Clock) const
{
	// nothing within twenty minutes of the clock after a danger, and none waiting for its relief
	return Clock - DangerEnd >= 20 * 60. && ReliefAt < 0;
}

void FS47Calm::Step(double Dt)
{
	(void)Dt;
	FS47FreeformSession* F = M ? M->Freeform.Get() : nullptr;
	if (!F || !F->Active() || !F->Director || !M->Rhythm || !M->World || !F->RulesLive() || F->InTutorial()) return;
	const double Clock = F->Clock();
	const int32 Now = FS47FreeformSession::NightOf(Clock);
	if (Now != Night) { Night = Now; Played = 0; Reliefs = 0; ReliefAt = -1; }

	// A danger that has just ended: the relief waits eight to ten minutes of the clock. Taken is not one (its own long rest is the relief).
	const FS47RhythmNow N = M->Rhythm->ReadNow();
	if (N.Busy != PrevBusy)
	{
		if (bPrevDanger && N.Busy.IsNone() && PrevBusy != FName(TEXT("taken")))
		{
			DangerEnd = Clock;
			ReliefAt = Clock + ReliefDelay(F->Seed(), F->Day());
			UE_LOG(LogS47Calm, Display, TEXT("S47: calm: %s is over, relief in %.0f s of the clock"), *PrevBusy.ToString(), ReliefAt - Clock);
		}
		const FS47RhythmEvent* E = N.Busy.IsNone() ? nullptr : M->Rhythm->Data().Find(N.Busy);
		bPrevDanger = E && E->Level == ES47RhythmLevel::Danger;
		PrevBusy = N.Busy;
	}
	if (!N.Busy.IsNone() || N.RestLeft >= 1) return;
	const bool bDriving = M->World->bDriving;

	if (ReliefAt >= 0)
	{
		if (Clock < ReliefAt) return;
		if (bDriving) return;  // wait until the wheel is let go
		ReliefAt = -1;
		// N1110: by day (before 19:30 or after 05:30) the relief is dropped: a thought about the dark and a radio at night make no sense in daylight
		const double Rh = NightHourOf(Clock);
		if (Rh < 19.5 || Rh > 29.5) { UE_LOG(LogS47Calm, Display, TEXT("S47: calm: no relief in daylight (hour %.1f), %.0f s after the danger"), Rh, Clock - DangerEnd); return; }
		FS47EventCtx Ctx;
		if (S47EventScript::Play(M, ReliefScript(F->Day()), Ctx)) { ++Reliefs; UE_LOG(LogS47Calm, Display, TEXT("S47: calm: the night eases, day %d, %.0f s of the clock after the danger"), F->Day(), Clock - DangerEnd); }
		return;
	}

	// PLAY-31-34 (14 Nov): Dale's dry word, the evening after the day shift put in a new tube for one that burned out (once; on the
	// walkie when it can be heard; not on a Story night, not over a panel or anything the Director holds).
	const bool bStory = FS47StoryDirector::Of(M) != nullptr;
	if (!bStory && F->TubeRemarkDue(F->Day()) && !bDriving && TubeRemarkHour(NightHourOf(Clock)) && M->T >= NextTry
		&& M->FieldRadio && M->FieldRadio->HearsDale() && !F->Held() && !M->ModalOpen() && !M->OneAtATime())
	{
		NextTry = M->T + 5.;
		FS47EventCtx Ctx;
		if (S47EventScript::Play(M, TubeDaleScript, Ctx)) { F->TubeRemarkSaid(); UE_LOG(LogS47Calm, Display, TEXT("S47: calm: Dale on the new ceiling tube, day %d"), F->Day()); return; }
	}

	// False alarms: evening only, not the first shift, in the dark, nothing else near.
	if (F->Day() < 2 || bDriving || Played >= AlarmsFor(F->Seed(), Night)) return;
	if (M->T < NextTry) return;
	// N1110: not on top of a reader, a repair or any other open panel, nor a pause, a visit or anything the Director holds
	if (F->Held() || M->ModalOpen() || M->OneAtATime() || (F->Taken && F->Taken->Busy()) || (F->FarmSite && F->FarmSite->VisitWantsGround())) { NextTry = M->T + 5.; return; }
	if (!GapOK(LastAlarm, Clock)) return;  // N1110: two alarms are at least twenty minutes of the clock apart
	NextTry = M->T + 5.;
	const double H = NightHourOf(Clock);
	if (H < SlotHour(F->Seed(), Night, Played) || H > 25.5) return;
	if (N.DangerTonight > 0 || N.SinceStart < 600. || !Quiet(Clock)) return;
	// PLAY-31-34 (14 Nov): from week 2, now and then, the slot is a ceiling tube burning out over the player's head (tube_wear).
	{
		const TArray<int32> Next = F->NextTubes(1);
		const bool bLeft = Next.Num() > 0 && M->Web && Next[0] < M->Web->TubeCount(); // (the scene has tubes 0 to 3: 2 and 3 can go)
		const bool bHere = M->Player && US47Audio::InControlRoom(S47Three::ToThree(M->Player->EyeUE()));
		if (LastTubeNight != Night && TubeWearMay(F->Day(), F->WornTubeCount(), bLeft, bHere, bStory))
		{
			LastTubeNight = Night; // asked once a night: its odds are the night's
			FString Why;
			FS47EventCtx Ctx;
			if (S47EventScript::Allows(M, TubeWearScript, Why) && S47EventScript::Play(M, TubeWearScript, Ctx))
			{
				++Played;
				LastAlarm = Clock;
				F->Director->RestFor(90.);
				UE_LOG(LogS47Calm, Display, TEXT("S47: calm: a ceiling tube burns out, night %d, day %d"), Night, F->Day());
				return;
			}
		}
	}
	for (const FString& Id : AlarmOrder(F->Seed(), Night))
	{
		// N1110 (pace): where each fits is the script's own gate (indoors / outdoors); the refrigerator is once a week at most
		if (Id == TEXT("alarm_fridge") && !FridgeMay(F->Day(), LastFridgeDay)) continue;
		FString Why;
		if (!S47EventScript::Allows(M, Id, Why)) continue;
		FS47EventCtx Ctx;
		if (!S47EventScript::Play(M, Id, Ctx)) continue;
		++Played;
		LastAlarm = Clock;
		if (Id == TEXT("alarm_fridge")) LastFridgeDay = F->Day();
		F->Director->RestFor(90.); // N1110 (pace): a minute and a half of quiet after an alarm, not half a minute
		UE_LOG(LogS47Calm, Display, TEXT("S47: calm: false alarm %s, night %d, %d of %d"), *Id, Night, Played, AlarmsFor(F->Seed(), Night));
		return;
	}
	NextTry = M->T + 60.;  // nothing fits here; ask again in a minute
}

bool FS47Calm::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	const auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s calm: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bCond; };
	int32 Two = 0;
	bool bSlots = true, bSame = true, bRelief = true, bOrder = true;
	for (int32 Night = 1; Night <= 120; ++Night)
	{
		const int32 A = AlarmsFor(47, Night);
		Two += A == 2;
		bSame &= A == AlarmsFor(47, Night) && SlotHour(47, Night, 1) == SlotHour(47, Night, 1);
		bSlots &= A >= 1 && A <= 2 && SlotHour(47, Night, 0) >= 20. && SlotHour(47, Night, 0) <= 23. && SlotHour(47, Night, 1) >= 23.5 && SlotHour(47, Night, 1) <= 25.25;
		const double D = ReliefDelay(47, Night);
		bRelief &= D >= 480. && D <= 600.;
		const TArray<FString> O = AlarmOrder(47, Night);
		bOrder &= O.Num() == int32(UE_ARRAY_COUNT(AlarmIds)) && O == AlarmOrder(47, Night) && O.Contains(TEXT("alarm_cow")) && O.Contains(TEXT("alarm_fridge"));
	}
	Check(bSlots && bSame && Two > 25 && Two < 80, TEXT("one or two false alarms a night, seeded, the first in the early evening and the second near midnight"));
	Check(bRelief, TEXT("the relief comes eight to ten minutes of the clock after the danger"));
	Check(bOrder, TEXT("the order the alarms are tried in is seeded and holds all seven"));
	for (const TCHAR* Id : AlarmIds)
	{
		const FS47EventScript* S = S47EventScript::Find(Id);
		bool bGood = S && S->bEnabled && S47EventScript::Lint(*S).Num() == 0 && S->Beats.Num() >= 3;
		int32 Silence = 0, Thoughts = 0, Other = 0;
		double FirstSilence = 99, LastThought = -1, LastSound = -1;
		if (S) for (const FS47Beat& B : S->Beats)
		{
			if (B.Do == ES47Beat::Silence) { ++Silence; FirstSilence = FMath::Min(FirstSilence, B.At); }
			else if (B.Do == ES47Beat::Thought) { ++Thoughts; LastThought = FMath::Max(LastThought, B.At); }
			else if (B.Do == ES47Beat::Sound || B.Do == ES47Beat::Tone) LastSound = FMath::Max(LastSound, B.At);
			else Other++;  // no finding, map mark, camera, light or prop: nothing here is an event
		}
		Check(bGood && Silence == 1 && FirstSilence == 0 && Thoughts >= 1 && LastThought >= LastSound && Other == 0 && S->Gates.Tier == ES47Allowed::Hint && S->Gates.DayFrom >= 2 && S->Gates.HourFrom >= 0,
			Id);
	}
	// N1110 (pace, critic 3): three variants chosen by the day, each with its own thoughts and its own Dale line (no contractions)
	{
		TSet<FString> DaleLines, Thoughts3;
		bool bAllGood = true;
		for (int32 Day = 1; Day <= 3; ++Day)
		{
			const FString Id = ReliefScript(Day);
			const FS47EventScript* S = S47EventScript::Find(Id);
			bool bGood = S && S->bEnabled && S47EventScript::Lint(*S).Num() == 0 && S->Gates.DayFrom == 2 && S->Gates.Tier == ES47Allowed::Hint;
			bool bDale = false, bThought = false, bFinding = false, bContraction = false;
			int32 Thoughts = 0, Drive = 0, DriveInTruck = 0;
			if (S) for (const FS47Beat& B : S->Beats)
			{
				Thoughts += B.Do == ES47Beat::Thought;
				if (B.Do == ES47Beat::Thought) Thoughts3.Add(B.Text);
				if (B.Do == ES47Beat::Line && B.Who == TEXT("radio") && B.Text.Contains(TEXT("long drive home"))) { ++Drive; DriveInTruck += B.If.Num() == 1 && B.If[0].If == TEXT("in_truck"); }
				if (B.Do == ES47Beat::Line && B.Who == TEXT("dale")) { bDale = B.Text.EndsWith(TEXT("Over.")); DaleLines.Add(B.Text); bContraction = B.Text.Contains(TEXT("'")); }
				bThought |= B.Do == ES47Beat::Thought;
				bFinding |= B.Do == ES47Beat::Evidence || B.Do == ES47Beat::Map || B.Do == ES47Beat::CameraSnap || B.Do == ES47Beat::Silence;
			}
			bAllGood &= bGood && bDale && bThought && !bFinding && !bContraction && Thoughts == 3 && Drive == 1 && DriveInTruck == 1;
			if (!bGood || !bDale || bFinding || bContraction || Thoughts != 3 || Drive != 1 || DriveInTruck != 1) Lines.Add(FString::Printf(TEXT("  (%s is not right)"), *Id));
		}
		Check(bAllGood, TEXT("calm_after (three variants): night sounds, a calm thought for indoors, outdoors on foot and the truck, the long drive home only in the truck, Dale checks in without contractions; no finding, no silence, no camera"));
		Check(FString(ReliefScript(1)) != FString(ReliefScript(2)) && FString(ReliefScript(2)) != FString(ReliefScript(3)) && FString(ReliefScript(4)) == FString(ReliefScript(1)) && DaleLines.Num() == 3 && Thoughts3.Num() == 9, TEXT("calm_after: the variant turns with the day, and the three differ in all nine thoughts and the three Dale lines"));
	}
	{
		Check(FridgeMay(2, 0) && !FridgeMay(5, 2) && !FridgeMay(8, 2) && FridgeMay(9, 2), TEXT("the refrigerator clunks once a week at most"));
	}
	{
		// the gap between two alarms is the clock's, not the frame's: the same twenty minutes the danger rest uses
		Check(GapOK(-1e9, 0.) && !GapOK(1000., 1060.) && GapOK(1000., 1000. + 20 * 60.), TEXT("an alarm waits twenty clock minutes after the last one"));
	}
	{
		// PLAY-31-34 (14 Nov): the tube that burns out of age. Gated: never before week 2, never on a Story night, never two waiting for
		// the day shift, only with a tube left and the player in the control room; about one night in four. One spark (the flash
		// limit: three a second at most, one here; a fade with calm flashes), the night log's line, then Dale's word the next evening.
		const FS47EventScript* S = S47EventScript::Find(TubeWearScript);
		const FS47EventScript* D = S47EventScript::Find(TubeDaleScript);
		bool bScript = S && S->bEnabled && S47EventScript::Lint(*S).Num() == 0 && S->Gates.DayFrom >= 8 && S->Gates.Odds > 0 && S->Gates.Odds < .5 && S->Gates.Tier == ES47Allowed::Hint;
		int32 Tubes = 0, Silence = 0;
		if (S) for (const FS47Beat& B : S->Beats)
		{
			if (B.Do == ES47Beat::Tube) { ++Tubes; bScript &= B.Kind == TEXT("worn") && B.Next == 1 && B.Tube < 0; }
			Silence += B.Do == ES47Beat::Silence;
			bScript &= B.Do != ES47Beat::Evidence && B.Do != ES47Beat::Map && B.Do != ES47Beat::CameraSnap && B.Do != ES47Beat::Light;
			if (!B.Text.IsEmpty()) bScript &= S47Script::LintText(B.Text).IsEmpty() && !B.Text.Contains(TEXT("47"));
		}
		bool bDale = D && D->bEnabled && S47EventScript::Lint(*D).Num() == 0 && D->Beats.Num() == 1;
		if (D) for (const FS47Beat& B : D->Beats) bDale &= B.Do == ES47Beat::Line && B.Who == TEXT("dale") && B.Text.EndsWith(TEXT("Over.")) && !B.Text.Contains(TEXT("'")) && S47Script::LintText(B.Text).IsEmpty();
		Check(bScript && Tubes == 1 && Silence == 1, TEXT("tube_wear: one tube, burned out of age (kind worn), from day 8 on about one night in four; no finding, light, map or camera"));
		Check(bDale, TEXT("tube_wear_dale: one dry Dale line on the walkie, no contractions"));
		Check(!TubeWearMay(7, 0, true, true, false) && TubeWearMay(8, 0, true, true, false) && !TubeWearMay(1, 0, true, true, false) && !TubeWearMay(9, 1, true, true, false)
			&& !TubeWearMay(9, 0, false, true, false) && !TubeWearMay(9, 0, true, false, false) && !TubeWearMay(9, 0, true, true, true),
			TEXT("a tube burns out only from week 2 (never night 1), one at a time, with one left to go, in the player's sight, never on a Story night"));
		// REVIEW (14 Nov): the level curve against the lit tube it was (1 before T 0): a flash is a rise from under half to full, so a
		// tube that only holds full and goes out shows none; the dip before it, the flash's length, and never more than three a second
		int32 Flashes = 0; float Prev = 1.f; bool bCalm = true; float CalmPrev = 2; double FlashFrom = -1, FlashFor = 0, LastFlash = -9, MinGap = 9; float Dip = 1.f;
		for (double T = 0; T < 2.; T += .005)
		{
			const float L = FS47TubeFlicker::SparkLevel(T, false);
			if (L >= .9f && Prev < .5f) { ++Flashes; FlashFrom = T; MinGap = FMath::Min(MinGap, T - LastFlash); LastFlash = T; }
			if (L >= .9f && FlashFrom >= 0) FlashFor = T - FlashFrom + .005;
			if (T < .04) Dip = FMath::Min(Dip, L);
			Prev = L;
			const float C = FS47TubeFlicker::SparkLevel(T, true); bCalm &= C <= CalmPrev; CalmPrev = C;
		}
		Check(Flashes == 1 && MinGap >= 1. / 3. && Dip <= .15f && FlashFor > .06 && FlashFor < .1 && bCalm && FS47TubeFlicker::SparkLevel(FS47TubeFlicker::SparkSecs(false), false) == 0 && FS47TubeFlicker::SparkLevel(FS47TubeFlicker::SparkSecs(true), true) == 0,
			TEXT("the burned-out tube dips, then flashes once to full for about 80 ms against its own light (one flash, under the three a second limit), fades only with calm flashes, then stays dark"));
		Check(TubeRemarkHour(19.5) && TubeRemarkHour(22.9) && !TubeRemarkHour(18.9) && !TubeRemarkHour(23.5), TEXT("Dale's word on the new tube comes between 19:00 and 23:00"));
	}
	FS47Calm Bare(nullptr);
	Bare.Step(1.);
	Check(Bare.AlarmsPlayed() == 0 && Bare.ReliefPlayed() == 0, TEXT("without a game nothing plays"));
	return bOk;
}
