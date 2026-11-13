#include "S47AnimalWarn.h"
#include "S47Animals.h"
#include "S47EventScript.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47FreeformDirector.h"
#include "S47Happenings.h"
#include "S47NightRhythm.h"
#include "S47Rhythm.h"
#include "S47Taken.h"
#include "S47Walker.h"
#include "S47World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47AnimalWarn, Log, All);

bool FS47AnimalWarn::Warned(const FString& Id)
{
	return Id == TEXT("horizon_figure") || Id == TEXT("beam_figure") || Id == TEXT("dish_lights") || Id == TEXT("figure_near");
}

const TCHAR* FS47AnimalWarn::ScriptOf(EKind Kind)
{
	return Kind == EKind::Birds ? TEXT("animals_warn_birds") : Kind == EKind::Coyote ? TEXT("animals_warn_coyote") : TEXT("");
}

FS47AnimalWarn::EKind FS47AnimalWarn::First(int32 Seed, int32 Night, const FString& Id)
{
	return FS47NightRhythm::Roll(Seed, Night, GetTypeHash(Id) ^ 0x414e5731u) < .5f ? EKind::Birds : EKind::Coyote;
}

bool FS47AnimalWarn::Hold(const FString& Id, const FString& Variant, bool bOnFoot, FString& Why)
{
	FS47FreeformSession* F = M ? M->Freeform.Get() : nullptr;
	if (!F || !Warned(Id) || !M->Animals) return true;
	const int32 Now = FS47FreeformSession::NightOf(F->Clock());
	if (Now != Night)
	{
		if (Pending.Num()) UE_LOG(LogS47AnimalWarn, Display, TEXT("S47: animals warn: a new night, %d event(s) still waiting on their lead are dropped"), Pending.Num()); // N1110 (critic)
		Night = Now; bUsed = false; Asked.Reset(); Pending.Reset();
	}
	// Its lead is running: it waits for it, and when Due() hands it back it is let through at once.
	for (const FWait& W : Pending)
		if (W.Id == Id) { Why = TEXT("the animals first"); return false; }
	if (Asked.Contains(Id) || bUsed || !bOnFoot) return true;
	if (!F->RulesLive() || F->InTutorial()) return true;
	const EKind First0 = First(F->Seed(), Night, Id);
	// Away from where the event will be, which is not known yet: a random way, 90 m out. The animals go the other.
	const double Bearing = FMath::FRand() * 2 * UE_DOUBLE_PI;
	const FVector Eye(M->Player ? M->Player->Pos.X : 0, 0, M->Player ? M->Player->Pos.Z : 0);
	const FVector Threat = Eye + FVector(FMath::Cos(Bearing) * 90, 0, FMath::Sin(Bearing) * 90);
	for (const EKind Kind : { First0, Other(First0) })
	{
		FString Gate;
		if (!S47EventScript::Allows(M, ScriptOf(Kind), Gate)) continue;
		const FS47EventScript* S = S47EventScript::Find(ScriptOf(Kind));
		if (!S || !S->bEnabled) continue;
		FString Fail;
		if (!M->Animals->Warn(Kind == EKind::Birds, Threat, Lead, Fail)) { UE_LOG(LogS47AnimalWarn, Display, TEXT("S47: animals warn %s: no %s here: %s"), *Id, Kind == EKind::Birds ? TEXT("covey") : TEXT("coyote"), *Fail); continue; }
		// N1110 (critic): asked about only once the odds said yes and the warning really started; a roll that failed leaves it open.
		Asked.Add(Id);
		bUsed = true; ++Count;
		FWait W; W.Id = Id; W.Variant = Variant; W.At = M->T + Lead; Pending.Add(W);
		FS47EventCtx Ctx;
		S47EventScript::Play(M, ScriptOf(Kind), Ctx);
		UE_LOG(LogS47AnimalWarn, Display, TEXT("S47: animals warn before %s: %s, %.0f s ahead"), *Id, Kind == EKind::Birds ? TEXT("a covey goes up") : TEXT("a coyote stands and stares"), Lead);
		FS47Happenings::TellOther(M, TEXT("animal warning"), Id); // M1-MEASURE-20261113: out on foot, put near the player, and its script's thought says so
		Why = TEXT("the animals first");
		return false;
	}
	return true;
}

bool FS47AnimalWarn::WouldAsk(const FString& Id, int32 NightNo) const
{
	if (!Warned(Id)) return false;
	// A new night: Hold starts it afresh.
	if (NightNo != Night) return true;
	if (bUsed || Asked.Contains(Id)) return false;
	for (const FWait& W : Pending)
		if (W.Id == Id) return false;
	return true;
}

bool FS47AnimalWarn::StrayNight(int32 Seed, int32 NightNo)
{
	return FS47NightRhythm::Roll(Seed, NightNo, 0x53545259u) < 1.f / 3.f;
}

// N1110 (pace): on one night in three the animals warn with no event after them, at some quiet moment out on foot.
void FS47AnimalWarn::Stray()
{
	FS47FreeformSession* F = M->Freeform.Get();
	if (!F || !F->Active() || !F->RulesLive() || F->InTutorial() || !M->Animals || !M->World || !M->Player) return;
	const int32 Now = FS47FreeformSession::NightOf(F->Clock());
	if (Now != Night)
	{
		Night = Now; bUsed = false; Asked.Reset(); Pending.Reset();
		StrayAt = StrayNight(F->Seed(), Night) ? M->T + 180. + 600. * FMath::FRand() : -1.;
	}
	if (StrayAt < 0 || bUsed || M->T < StrayAt || Pending.Num()) return;
	StrayAt = M->T + 20.;  // asked again in twenty seconds if this is not the moment
	if (M->World->bDriving || (F->Taken && (F->Taken->Indoors() || F->Taken->Busy())) || F->Held() || M->ModalOpen() || M->OneAtATime()) return;
	// N1110 (pace, critic 2): not in the Director's rest, and not within five game minutes of a false alarm.
	FS47RhythmNow Rest;
	if (F->Director) F->Director->FillRhythm(Rest);
	if (!StrayClearOf(F->Clock(), F->Happenings ? F->Happenings->LastFalseAlarmAt() : -1e9, Rest.RestLeft)) return;
	const double Bearing = FMath::FRand() * 2 * UE_DOUBLE_PI;
	const FVector Eye(M->Player->Pos.X, 0, M->Player->Pos.Z);
	const FVector Threat = Eye + FVector(FMath::Cos(Bearing) * 90, 0, FMath::Sin(Bearing) * 90);
	// The script's odds roll (S47EventScript::Allows) is seeded by the night and the script: the same each time it is asked. So the
	// night's one kind is asked and no fallback, which makes a stray night's odds the script's own (.25), not two draws of it.
	const EKind First0 = First(F->Seed(), Night, TEXT("stray"));
	for (const EKind Kind : { First0 })
	{
		FString Gate, Fail;
		if (!S47EventScript::Allows(M, ScriptOf(Kind), Gate)) continue; // (hours or odds: the same answer all night, so asking again in twenty seconds costs nothing)
		if (!M->Animals->Warn(Kind == EKind::Birds, Threat, Lead, Fail)) continue;
		bUsed = true; ++Count; StrayAt = -1.;
		FS47EventCtx Ctx;
		S47EventScript::Play(M, ScriptOf(Kind), Ctx);
		UE_LOG(LogS47AnimalWarn, Display, TEXT("S47: animals warn: a false alarm on its own, no event follows (%s)"), Kind == EKind::Birds ? TEXT("a covey goes up") : TEXT("a coyote stands and stares"));
		FS47Happenings::TellOther(M, TEXT("animal warning"), TEXT("stray")); // M1-MEASURE-20261113
		return;
	}
}

bool FS47AnimalWarn::Due(FString& OutId, FString& OutVariant)
{
	if (!M) return false;
	Stray();
	for (int32 I = 0; I < Pending.Num(); ++I)
		if (M->T >= Pending[I].At)
		{
			OutId = Pending[I].Id; OutVariant = Pending[I].Variant;
			UE_LOG(LogS47AnimalWarn, Display, TEXT("S47: animals warn: the lead of %s is up, handed back to start (if it is refused now, it is dropped: see the Refused line)"), *OutId); // N1110 (critic)
			Pending.RemoveAt(I);
			return true;
		}
	return false;
}

void FS47AnimalWarn::Reset() { Pending.Reset(); Night = 0; bUsed = false; Asked.Reset(); StrayAt = -1.; }

bool FS47AnimalWarn::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	const auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s animal warning: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bCond; };
	Check(Warned(TEXT("beam_figure")) && Warned(TEXT("figure_near")) && Warned(TEXT("dish_lights")) && !Warned(TEXT("eyes_window")) && !Warned(TEXT("array_burst")) && !Warned(TEXT("hood_warm")),
		TEXT("the animals warn for what happens out in the open, not for the control room window or the array"));
	int32 Birds = 0;
	bool bSame = true;
	for (int32 N = 1; N <= 200; ++N)
	{
		const EKind K = First(47, N, TEXT("beam_figure"));
		bSame &= K == First(47, N, TEXT("beam_figure")) && K != EKind::None;
		Birds += K == EKind::Birds;
	}
	Check(bSame && Birds > 60 && Birds < 140, TEXT("which animal comes first is seeded, the same each time that night is asked, and both come"));
	Check(Other(EKind::Birds) == EKind::Coyote && Other(EKind::Coyote) == EKind::Birds, TEXT("the other animal is the fallback"));
	int32 Strays = 0;
	for (int32 N = 1; N <= 300; ++N) Strays += StrayNight(47, N) && StrayNight(47, N) == StrayNight(47, N);
	Check(Strays > 70 && Strays < 130, TEXT("on about one night in three the warning comes alone, with no event after it (seeded)"));
	Check(StrayClearOf(1000, 1000 - 301, 0) && !StrayClearOf(1000, 1000 - 299, 0) && !StrayClearOf(1000, -1e9, 1) && StrayClearOf(1000, -1e9, 0), TEXT("N1110 (pace, critic 2): the lone warning waits out the Director's rest and five game minutes after a false alarm"));
	Check(Lead >= 4 && Lead <= 10, TEXT("the event follows its warning by a few seconds"));
	FS47AnimalWarn Bare(nullptr);
	FString Why;
	Check(Bare.Hold(TEXT("beam_figure"), FString(), true, Why) && !Bare.Waiting() && Bare.Warnings() == 0, TEXT("without a game the event is never held"));
	{
		// M1-FLOOR-20261113: who may still be warned for tonight, so that the rhythm is asked before the animals go up.
		FS47AnimalWarn W(nullptr);
		W.Night = 5;
		const bool bFresh = W.WouldAsk(TEXT("horizon_figure"), 5) && !W.WouldAsk(TEXT("array_burst"), 5);
		FWait P; P.Id = TEXT("horizon_figure"); W.Pending.Add(P);
		const bool bWaiting = !W.WouldAsk(TEXT("horizon_figure"), 5) && W.WouldAsk(TEXT("horizon_figure"), 6);
		W.Pending.Reset(); W.bUsed = true;
		const bool bUsedUp = !W.WouldAsk(TEXT("beam_figure"), 5) && W.WouldAsk(TEXT("beam_figure"), 6);
		Check(bFresh && bWaiting && bUsedUp, TEXT("M1 floor: the animals may still go ahead only of a warned event not yet asked, none used and none waiting tonight; a new night starts afresh"));
	}
	// the two warning scripts: there, enabled, clean (the script lint also covers them), one thought and one finding each, nothing about the event
	for (const EKind K : { EKind::Birds, EKind::Coyote })
	{
		const FS47EventScript* S = S47EventScript::Find(ScriptOf(K));
		bool bGood = S != nullptr && S->bEnabled && S47EventScript::Lint(*S).Num() == 0;
		int32 Thoughts = 0, Findings = 0, Camera = 0;
		if (S) for (const FS47Beat& B : S->Beats) { Thoughts += B.Do == ES47Beat::Thought; Findings += B.Do == ES47Beat::Evidence; Camera += B.Do == ES47Beat::CameraSnap; }
		Check(bGood && Thoughts == 1 && Findings == 1 && Camera == 0 && S->Gates.HourFrom >= 0 && S->Gates.Odds > 0 && S->Gates.Odds <= .25 && S->Gates.DayFrom == 3 && S->Gates.Tier == ES47Allowed::Hint,
			K == EKind::Birds ? TEXT("animals_warn_birds: a thought, a finding, night hours, odds of at most a quarter, from the third night, no camera") : TEXT("animals_warn_coyote: a thought, a finding, night hours, odds of at most a quarter, from the third night, no camera"));
	}
	return bOk;
}
