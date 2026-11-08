#include "S47Omen.h"

#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "S47DuskOrb.h"
#include "S47FarmSite.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47Glimpses.h"
#include "S47Happenings.h"
#include "S47HighwayUfo.h"
#include "S47IndoorDread.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47RoswellBrain.h"
#include "S47RoswellGroup.h"
#include "S47Taken.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Omen, Log, All);

namespace S47OmenLocal
{
	// Beyond this from the player a source is taken to be in some other area's terms (a site left on other ground), not near.
	constexpr double FarCut = 1500;
	// Only something at least this dangerous is an onset: the small things on a roof do not hush a whole night.
	constexpr double OnsetSeverity = 0.35;

	double Horizontal(const FVector& From, const FVector& To)
	{
		// Up counts for half: a light high over a place is felt as over it, not as far off.
		const double Dx = From.X - To.X, Dz = From.Z - To.Z, Dy = (From.Y - To.Y) * 0.5;
		return FMath::Sqrt(Dx * Dx + Dz * Dz + Dy * Dy);
	}
}

double FS47Omen::Falloff(double Distance, double Span)
{
	if (!FMath::IsFinite(Distance) || !FMath::IsFinite(Span) || Span <= 0) return 0;
	const double Core = FMath::Min(4.0, Span * 0.1);
	const double Share = FMath::Clamp((Span - FMath::Max(0.0, Distance)) / FMath::Max(0.001, Span - Core), 0.0, 1.0);
	return Share * Share * (3.0 - 2.0 * Share);
}

double FS47Omen::ReachFor(double Severity)
{
	const double Sv = FMath::Clamp(Severity, 0.0, 1.0);
	return 25.0 + 215.0 * Sv * Sv;
}

double FS47Omen::LevelAt(const FVector& Eye, const TArray<FS47OmenSource>& In)
{
	double Level = 0;
	for (const FS47OmenSource& Src : In)
	{
		if (Src.At.ContainsNaN()) continue;
		const double Sev = FMath::Clamp(Src.Severity, 0.0, 1.0) * (Src.bComing ? ComingShare : 1.0);
		Level = FMath::Max(Level, Sev * Falloff(FVector::Distance(Eye, Src.At), Src.Reach));
	}
	return FMath::Clamp(Level, 0.0, 1.0);
}

double FS47Omen::SenseFrom(const FVector& Where, const TArray<FS47OmenSource>& In, double Widen, FVector* OutFrom, bool* bOutComing)
{
	double Best = 0;
	for (const FS47OmenSource& Src : In)
	{
		if (Src.bResidual || Src.At.ContainsNaN()) continue;
		const double Span = FMath::Max(MinSense, Src.Reach * FMath::Max(1.0, Widen));
		const double Felt = FMath::Clamp(Src.Severity, 0.0, 1.0) * Falloff(S47OmenLocal::Horizontal(Where, Src.At), Span);
		if (Felt > Best)
		{
			Best = Felt;
			if (OutFrom) *OutFrom = Src.At;
			if (bOutComing) *bOutComing = Src.bComing;
		}
	}
	return Best;
}

double FS47Omen::SenseAt(const FVector& Where, double Widen, FVector* OutFrom, bool* bOutComing) const
{
	return SenseFrom(Where, Now, Widen, OutFrom, bOutComing);
}

void FS47Omen::DevPresence(const FVector& Where, double Severity, double Seconds, bool bFirst)
{
	DevAt = Where; DevSeverity = FMath::Clamp(Severity, 0.0, 1.0); DevLeft = FMath::Max(0.0, Seconds); DevAge = 0; bDevComing = bFirst;
	ReadIn = 0;
}

void FS47Omen::Reset()
{
	Now.Reset(); Sensed.Reset(); ReadIn = 0; LevelNow = 0; OnsetAge = 1e9; DevLeft = 0; DevAge = 0;
}

void FS47Omen::Gather(TArray<FS47OmenSource>& Out) const
{
	if (!M || !M->IsFreeform() || !M->Freeform || !M->Player || !M->Web) return;
	const FS47FreeformSession& Fs = *M->Freeform;
	const FVector Eye = S47Three::ToThree(M->Player->EyeUE());
	auto Add = [&Out, &Eye](const FString& Name, const FVector& Where, double Sev, double Span, bool bFirst, bool bLeft)
	{
		if (Where.ContainsNaN() || FVector::Distance(Where, Eye) > S47OmenLocal::FarCut || Sev <= 0) return;
		FS47OmenSource Src; Src.Id = Name; Src.At = Where; Src.Severity = Sev; Src.Reach = Span; Src.bComing = bFirst; Src.bResidual = bLeft;
		Out.Add(Src);
	};
	// The Director's event, when it is on the ground the player is on.
	if (Fs.Director)
	{
		const FS47FreeformDirector& Dr = *Fs.Director;
		const FString& Ev = Dr.ActiveEvent();
		if (!Ev.IsEmpty() && Dr.AnchorHere())
		{
			const FVector EvAt = Dr.EventAnchor();
			const FString EvId = TEXT("director:") + Ev;
			const int32 EvStage = Dr.EventStage();
			// the figures: six seconds of nothing at the anchor, then a body (S47FreeformDirector Stage 0 to 1)
			if (Ev == TEXT("service_presence")) Add(EvId, EvAt + FVector(0, 1, 0), 0.9, 70, EvStage == 0, false);
			else if (Ev == TEXT("observer") || Ev == TEXT("feeder_presence") || Ev == TEXT("array_presence")) Add(EvId, EvAt + FVector(0, 1, 0), 0.65, 55, EvStage == 0, false);
			// the craft low over the station: nearest at thirty seconds
			else if (Ev == TEXT("close_pass")) Add(EvId, Dr.ClosePassNow(), 1.0, 260, Dr.EventAge() < 18, false);
			else if (Ev == TEXT("farm_lights") || Ev == TEXT("town_lights") || Ev == TEXT("response_lights")) Add(EvId, EvAt + FVector(0, 20, 0), 0.6, 160, false, false);
			else if (Ev == TEXT("flyover")) Add(EvId, EvAt + FVector(0, 40, 0), 0.4, 200, false, false);
			else if (Ev == TEXT("dusk_orb"))
			{
				const FS47DuskOrb& Orbs = Dr.DuskOrbState();
				for (int32 I = 0; I < Orbs.VisitLights() && I < FS47DuskOrb::Count; ++I)
					if (FS47DuskOrb::Presence(Dr.EventAge(), I, Orbs.VisitSeed(), Orbs.VisitSeconds()) > 0.5)
						Add(FString::Printf(TEXT("orb:%d"), I), Dr.OrbAt(I), 0.45, 45, false, false);
			}
			// something on the roof: a body, but a small thing
			else if (Ev == TEXT("roof_steps")) Add(EvId, EvAt, 0.25, 20, false, false);
		}
	}
	// Being taken: the steps over the roof first, then the light; in it, everything at once.
	if (Fs.Taken && Fs.Taken->Busy())
	{
		const FS47Taken::EStage TakenStage = Fs.Taken->Stage();
		const TOptional<FVector> TakenAt = Fs.Taken->RadioAt();
		if (TakenAt.IsSet()) Add(TEXT("taken"), TakenAt.GetValue(), 1.0, 140, TakenStage == FS47Taken::EStage::Dread, false);
		else Add(TEXT("taken"), Eye + FVector(0, 6, 0), 1.0, 140, false, false);
	}
	// A visit at marks, or out on open ground: the quiet before it, its warning, then the body.
	if (Fs.FarmSite)
	{
		const FS47FarmSite& Farm = *Fs.FarmSite;
		const double Sev = Farm.VisitKind() == 2 ? 0.95 : 0.85;
		if (Farm.VisitWantsGround())
		{
			const TOptional<FVector> Body = Farm.VisitBodyEye();
			const TOptional<FVector> WarnAt = Farm.VisitWarningAt();
			const FVector Where = Body.IsSet() ? Body.GetValue() : WarnAt.IsSet() ? WarnAt.GetValue() : Farm.Centre() + FVector(0, 1, 0);
			Add(TEXT("visit"), Where, Body.IsSet() && Farm.VisitHuntClock() > 0 ? 1.0 : Sev, 70, !Body.IsSet(), false);
		}
		else if (Farm.VisitHushed()) Add(TEXT("visit"), Farm.Centre() + FVector(0, 1, 0), Sev, 70, true, false);
		// marks still standing, with nothing at them: the ground itself, faintly, close by
		else if (Farm.Armed()) Add(TEXT("marks"), Farm.Centre() + FVector(0, 0.5, 0), 0.12, 16, false, true);
	}
	// Indoors: something tries the door; a head at the window.
	if (Fs.IndoorDread)
	{
		const TOptional<FVector> PeekAt = Fs.IndoorDread->PeekEye();
		if (PeekAt.IsSet()) Add(TEXT("peek"), PeekAt.GetValue(), 0.45, 22, false, false);
		FS47DoorVisit& Dv = Fs.IndoorDread->Door();
		if (Dv.Active())
		{
			const TOptional<FVector> Body = Dv.BodyEye();
			Add(TEXT("door"), Body.IsSet() ? Body.GetValue() : Dv.DoorMiddle(), 0.6, 24, Dv.StageNow() == FS47DoorVisit::EStage::Trying, false);
		}
	}
	// Bodies out, whoever sent them: the Director's figures, a visit's, the developer's.
	if (M->Night && M->Night->Glimpses)
	{
		for (const FS47RoswellGroup::FView& View : M->Night->Glimpses->Individuals())
		{
			const bool bHunt = View.State == FS47RoswellBrain::EState::Hunt || View.State == FS47RoswellBrain::EState::Contact || View.State == FS47RoswellBrain::EState::Search;
			Add(FString::Printf(TEXT("body:%d"), View.Id), View.Eye, bHunt ? 1.0 : 0.6, 60, false, false);
		}
	}
	// The light over the highway, on the road's own ground.
	if (M->HighwayUfo && (M->Web->Area() == TEXT("road") || M->Web->Area() == TEXT("diner")))
	{
		const FString UfoStage = M->HighwayUfo->StageName();
		if (UfoStage != TEXT("idle") && UfoStage != TEXT("done")) Add(TEXT("highway"), M->HighwayUfo->UfoPosition(), 1.0, 260, UfoStage == TEXT("approach"), false);
	}
	// The night's own events (S47Happenings), read only: the ones with something real out there.
	if (Fs.Happenings)
	{
		const FS47Happenings& Haps = *Fs.Happenings;
		const FString& OnGround = Haps.Ground();
		FVector HapAt = FVector::ZeroVector;
		if (!OnGround.IsEmpty() && Haps.LiveAt(HapAt))
		{
			double Sev = 0, Span = 0;
			if (OnGround == TEXT("car_dies")) { Sev = 1.0; Span = 220; }
			else if (OnGround == TEXT("landing")) { Sev = 0.85; Span = 320; }
			else if (OnGround == TEXT("car_light")) { Sev = 0.9; Span = 90; }
			else if (OnGround == TEXT("hitchhiker")) { Sev = 0.7; Span = 45; }
			else if (OnGround == TEXT("horizon_figure")) { Sev = 0.5; Span = 200; }
			if (Sev > 0) Add(TEXT("night:") + OnGround, HapAt, Sev, Span, false, false);
		}
		FVector RingAt = FVector::ZeroVector;
		if (Haps.SiteHere(TEXT("landing"), RingAt)) Add(TEXT("ring"), RingAt + FVector(0, 0.5, 0), 0.15, 26, false, true);
	}
	// F10's.
	if (DevLeft > 0) Add(TEXT("dev"), DevAt, DevSeverity, ReachFor(DevSeverity), bDevComing && DevAge < 6, false);
}

void FS47Omen::Step(double Dt)
{
	if (!bLogChecked) { bLogChecked = true; bLog = FParse::Param(FCommandLine::Get(), TEXT("S47OmenLog")); }
	if (!M || !M->IsFreeform() || M->Mode != US47Main::EMode::Play || !M->Player)
	{
		if (!Now.IsEmpty() || LevelNow > 0) Reset();
		return;
	}
	if (M->bPausedByMenu || M->ModalOpen() || !FMath::IsFinite(Dt) || Dt <= 0) return;
	if (OnsetAge < 1e8) OnsetAge += Dt;
	if (DevLeft > 0) { DevLeft = FMath::Max(0.0, DevLeft - Dt); DevAge += Dt; }
	ReadIn -= Dt;
	if (ReadIn > 0) return;
	ReadIn = 0.25;
	TArray<FS47OmenSource> Fresh;
	Gather(Fresh);
	Now = MoveTemp(Fresh);
	const FVector Eye = S47Three::ToThree(M->Player->EyeUE());
	const double Was = LevelNow;
	LevelNow = LevelAt(Eye, Now);
	// An onset: something real comes within the night's own sense of the player that was not there at the last look.
	TSet<FString> Here;
	for (const FS47OmenSource& Src : Now)
	{
		if (Src.bResidual || Src.Severity < S47OmenLocal::OnsetSeverity) continue;
		const double Span = FMath::Max(MinSense, Src.Reach * SenseWiden);
		if (S47OmenLocal::Horizontal(Eye, Src.At) >= Span) continue;
		Here.Add(Src.Id);
		if (!Sensed.Contains(Src.Id))
		{
			OnsetAge = 0; ++OnsetCount; OnsetAt = Src.At;
			if (bLog)
			{
				UE_LOG(LogS47Omen, Display, TEXT("S47: omen onset %s, severity %.2f, %.0f m, coming %d"), *Src.Id, Src.Severity, FVector::Distance(Eye, Src.At), Src.bComing ? 1 : 0);
			}
		}
	}
	Sensed = MoveTemp(Here);
	if (bLog && int32(Was * 10) != int32(LevelNow * 10))
	{
		UE_LOG(LogS47Omen, Display, TEXT("S47: omen level %.2f from %d sources"), LevelNow, Now.Num());
	}
}

bool FS47Omen::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s omen: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bCond; };
	Check(Falloff(0, 60) == 1 && Falloff(2, 60) == 1 && Falloff(60, 60) == 0 && Falloff(90, 60) == 0, TEXT("full close by, nothing at its reach or beyond"));
	bool bDown = true; double Prev = 1;
	for (double Dist = 0; Dist <= 80; Dist += 0.5) { const double Felt = Falloff(Dist, 60); bDown &= Felt <= Prev + 1e-9; Prev = Felt; }
	Check(bDown && Falloff(30, 60) > 0.3 && Falloff(30, 60) < 0.7, TEXT("it falls smoothly with distance and never rises again"));
	Check(Falloff(10, 0) == 0 && Falloff(TNumericLimits<double>::Max(), 60) == 0 && Falloff(-5, 60) == 1, TEXT("no reach, no feeling; a bad distance is never more than full"));
	Check(ReachFor(0.1) < ReachFor(0.5) && ReachFor(0.5) < ReachFor(1) && ReachFor(1) > 200 && ReachFor(0) >= 25, TEXT("a worse thing carries further"));
	TArray<FS47OmenSource> Empty;
	Check(LevelAt(FVector::ZeroVector, Empty) == 0 && SenseFrom(FVector::ZeroVector, Empty, SenseWiden, nullptr, nullptr) == 0, TEXT("nothing out there, nothing felt"));
	TArray<FS47OmenSource> Two;
	{ FS47OmenSource Src; Src.Id = TEXT("near"); Src.At = FVector(20, 0, 0); Src.Severity = 0.5; Src.Reach = 60; Two.Add(Src); }
	{ FS47OmenSource Src; Src.Id = TEXT("bad"); Src.At = FVector(0, 0, 100); Src.Severity = 1; Src.Reach = 240; Two.Add(Src); }
	const double Lv = LevelAt(FVector::ZeroVector, Two);
	Check(Lv >= 0.5 * Falloff(20, 60) && Lv >= Falloff(100, 240) && Lv <= 1, TEXT("the strongest counts, not the sum"));
	TArray<FS47OmenSource> One;
	{ FS47OmenSource Src; Src.At = FVector(10, 0, 0); Src.Severity = 0.8; Src.Reach = 60; One.Add(Src); }
	const double Full = LevelAt(FVector::ZeroVector, One);
	One[0].bComing = true;
	const double Coming = LevelAt(FVector::ZeroVector, One);
	Check(Coming < Full && Coming > 0, TEXT("a source only coming is felt at the eyes, but less"));
	FVector FromAt = FVector::ZeroVector; bool bFirst = false;
	Check(SenseFrom(FVector::ZeroVector, One, SenseWiden, &FromAt, &bFirst) > Coming && bFirst && FromAt.Equals(FVector(10, 0, 0)), TEXT("the animals and the night feel it fully, and know where it is"));
	One[0].bComing = false; One[0].At = FVector(80, 0, 0);
	Check(LevelAt(FVector::ZeroVector, One) == 0 && SenseFrom(FVector::ZeroVector, One, SenseWiden, nullptr, nullptr) > 0, TEXT("past the lamp's reach the animals still feel it"));
	One[0].bResidual = true;
	Check(SenseFrom(FVector::ZeroVector, One, SenseWiden, nullptr, nullptr) == 0, TEXT("what was left behind is never felt by the animals"));
	TArray<FS47OmenSource> Above;
	{ FS47OmenSource Src; Src.At = FVector(0, 60, 0); Src.Severity = 1; Src.Reach = 120; Above.Add(Src); }
	Check(SenseFrom(FVector::ZeroVector, Above, 1, nullptr, nullptr) > SenseFrom(FVector(60, 0, 0), Above, 1, nullptr, nullptr), TEXT("a light high over a place is felt most under it"));
	FS47Omen Bare(nullptr); Bare.Step(1); Bare.Reset();
	Check(Bare.Level() == 0 && Bare.Sources().IsEmpty() && Bare.Onsets() == 0, TEXT("without a game nothing is out there"));
	return bOk;
}
