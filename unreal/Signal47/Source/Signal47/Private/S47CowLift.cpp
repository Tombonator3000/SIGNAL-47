#include "S47CowLift.h"
#include <limits>
#include "S47Flow.h"
#include "S47Herd.h"
#include "S47Main.h"
#include "S47TownLayer.h"
#include "S47UfoPresentation.h"
#include "S47Web.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"

namespace
{
	double Ease(double A, double B, double X) { const double K = FMath::Clamp((X - A) / FMath::Max(1e-6, B - A), 0., 1.); return K * K * (3. - 2. * K); }
}

FS47CowLift::FS47CowLift(US47Main* InMain) : M(InMain) {}
FS47CowLift::~FS47CowLift() = default;

double FS47CowLift::HeightAt(double Sec) { return TopHeight * Ease(Arrive, Arrive + Rise, Sec); }
double FS47CowLift::SpinAt(double Sec) { return .35 * FMath::Clamp(Sec - Arrive, 0., Rise + Hold); }
double FS47CowLift::CraftAltitude(double Sec) { return Sec < GoneAt ? FMath::Lerp(70., CraftHigh, Ease(0, Arrive, Sec)) : FMath::Lerp(CraftHigh, 140., Ease(GoneAt, Total, Sec)); }
double FS47CowLift::BeamAt(double Sec) { return Ease(Arrive * .4, Arrive, Sec) * (1. - Ease(GoneAt, GoneAt + Leave * .4, Sec)); }
double FS47CowLift::OpacityAt(double Sec) { return Ease(0, 3, Sec) * (1. - Ease(Total - 4, Total, Sec)); }

bool FS47CowLift::MayBegin(double Hour, double SecondsToMorning, double Distance, bool bPlanned, bool bDirectorBusy)
{
	// After 02:00 is where the full encounters are; a plan that is not this night's own has to wait for its night.
	return bPlanned && !bDirectorBusy && FMath::IsFinite(Hour) && Hour >= MinHour && Hour < MaxHour
		&& SecondsToMorning > Total && SecondsToMorning <= 4 * 3600. && FMath::IsFinite(Distance) && Distance >= MinDistance && Distance <= MaxDistance;
}

void FS47CowLift::Begin(const S47J::FObj& Saved)
{
	Clear();
	if (!Saved.IsValid()) return;
	const int32 Id = S47J::Int(Saved, TEXT("cow"), -1);
	const double Sec = S47J::Num(Saved, TEXT("t"));
	// A stable id or none; a lift past the point she is gone is over, and nonsense is nothing.
	if (Id == S47Herd::MISSING && FMath::IsFinite(Sec) && Sec > 0 && Sec < GoneAt) { Cow = Id; T = Sec; }
}

S47J::FObj FS47CowLift::Save() const
{
	const auto O = S47J::New();
	if (Cow >= 0 && T > 0 && T < GoneAt) { S47J::Set(O, TEXT("cow"), Cow); S47J::Set(O, TEXT("t"), T); }
	return O;
}

void FS47CowLift::Clear()
{
	Cow = -1; T = 0; Tail = -1;
	if (Ufo) Ufo->Hide();
	const bool bLive = M && !IsEngineExitRequested() && !IsGarbageCollecting() && IsValid(M) && !M->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed) && IsValid(M->TownLayer.Get());
	if (bLive) for (const auto& H : M->TownLayer->Herds) if (H) H->SetLift(-1, 0, 0);
}

void FS47CowLift::Stop()
{
	// She is put down exactly where she stood; the saved seconds bring the pose back on Continue.
	const int32 KeepCow = Cow; const double KeepT = T;
	Clear(); Ufo.Reset();
	Cow = KeepCow; T = KeepT;
}

bool FS47CowLift::Visible() const
{
	return Cow >= 0 && HeightAt(T) >= VisibleHeight && OpacityAt(T) > .5 && BeamAt(T) > .5 && Ufo && Ufo->Presented();
}

const UPrimitiveComponent* FS47CowLift::Hull() const { return Ufo ? Ufo->SightTarget() : nullptr; }

void FS47CowLift::Apply(double Sec, int32 LiftedCow)
{
	if (!M || !M->TownLayer || !M->Web) return;
	const FString Here = M->Web->Area() == TEXT("diner") ? FString(TEXT("road")) : M->Web->Area();
	bool bShown = false;
	for (const auto& H : M->TownLayer->Herds)
	{
		if (!H) continue;
		const bool bHere = H->Area == Here, bUp = bHere && LiftedCow >= 0;
		H->SetLift(bUp ? LiftedCow : -1, bUp ? HeightAt(Sec) : 0., bUp ? SpinAt(Sec) : 0.);
		if (!bHere || bShown) continue;
		const FVector Ground = H->LiftGround();
		CraftPos = Ground + FVector(0, CraftAltitude(Sec), 0);
		CowPos = Ground + FVector(0, 1.1 + HeightAt(Sec), 0);
		if (!Ufo) Ufo = MakeUnique<FS47UfoPresentation>(M->Web.Get());
		FS47UfoPresentation::FFrame F;
		F.Position = CraftPos; F.GroundTarget = Ground; F.Forward = FVector(1, 0, 0); F.Seconds = Sec; F.Width = 9;
		F.Opacity = OpacityAt(Sec); F.Beam = BeamAt(Sec); F.NearWhite = .25 * BeamAt(Sec); F.bSlowBeam = true;
		F.bCalm = M->Flow && M->Flow->bCalmFlash;
		Ufo->Show(F); bShown = true;
	}
	if (!bShown && Ufo) Ufo->Hide();
}

FS47CowLift::EStep FS47CowLift::Step(double Dt, bool bMayStart, bool bWatching)
{
	if (!FMath::IsFinite(Dt) || Dt < 0) Dt = 0;
	Dt = FMath::Min(Dt, .25);
	if (Cow < 0 && Tail >= 0)
	{
		// She is gone; the light is still leaving.
		Tail += Dt;
		if (Tail >= Total) { Tail = -1; if (Ufo) Ufo->Hide(); return EStep::Idle; }
		Apply(Tail, -1);
		return EStep::Idle;
	}
	if (Cow < 0)
	{
		if (!bMayStart || !bWatching) return EStep::Idle;
		Cow = S47Herd::MISSING; T = 0;
		UE_LOG(LogTemp, Display, TEXT("S47: cow lift begins over the pasture"));
	}
	EStep Out = EStep::Lifting;
	if (bWatching) T += Dt; else T -= Dt * 1.5;
	if (T <= 0)
	{
		Clear(); Out = EStep::LetDown;
		UE_LOG(LogTemp, Display, TEXT("S47: cow lift: she is back on the grass, unlifted"));
		return Out;
	}
	if (T >= GoneAt)
	{
		// Gone: the herd is left whole but for her (FS47Aftermath sets her away); the craft and the beam leave on their own.
		Cow = -1; T = 0; Tail = GoneAt;
		Apply(Tail, -1);
		UE_LOG(LogTemp, Display, TEXT("S47: cow lift: she is gone"));
		return EStep::Done;
	}
	Apply(T, Cow);
	return Out;
}

bool FS47CowLift::SelfTest(TArray<FString>& Lines)
{
	bool OK = true;
	const auto Check = [&](bool B, const TCHAR* Why) { OK &= B; Lines.Add(FString::Printf(TEXT("%s cow lift: %s"), B ? TEXT("PASS") : TEXT("FAIL"), Why)); };
	Check(HeightAt(0) == 0 && HeightAt(Arrive) == 0 && FMath::IsNearlyEqual(HeightAt(GoneAt), TopHeight) && HeightAt(20) < HeightAt(30), TEXT("she rises slowly from the grass and no further than the top"));
	Check(BeamAt(0) == 0 && BeamAt(Arrive) > .99 && BeamAt(Total) == 0 && OpacityAt(0) == 0 && OpacityAt(20) == 1 && OpacityAt(Total) == 0, TEXT("the light comes, stands and leaves; nothing is left drawn at the end"));
	Check(CraftAltitude(0) > CraftAltitude(Arrive) && CraftAltitude(Total) > CraftAltitude(GoneAt), TEXT("the craft comes down to the pasture and goes up again"));
	// Reversible: the same seconds give the same pose, up or down.
	Check(HeightAt(25) == HeightAt(25) && SpinAt(Arrive) == 0 && SpinAt(30) > SpinAt(20), TEXT("the pose is a pure function of the seconds into it"));
	const double Night = 3.5, Morning = 3 * 3600.;
	Check(MayBegin(Night, Morning, 50, true, false), TEXT("after 02:00, planned for this morning, player at the right distance: it may begin"));
	Check(!MayBegin(1.0, Morning, 50, true, false) && !MayBegin(5.5, Morning, 50, true, false), TEXT("not before 02:00 and not in the grey of morning"));
	Check(!MayBegin(Night, Morning, 50, false, false) && !MayBegin(Night, 20 * 3600., 50, true, false), TEXT("no plan, or a plan for another night, gives no lift"));
	Check(!MayBegin(Night, Morning, 10, true, false) && !MayBegin(Night, Morning, 200, true, false), TEXT("not on top of her and not out of sight"));
	Check(!MayBegin(Night, Morning, 50, true, true), TEXT("never on top of another of the Director's events: one thing a night"));
	Check(!MayBegin(std::numeric_limits<double>::quiet_NaN(), Morning, 50, true, false) && !MayBegin(Night, Morning, std::numeric_limits<double>::quiet_NaN(), true, false), TEXT("nonsense in, no lift"));
	{
		FS47CowLift L(nullptr);
		Check(L.Save()->Values.Num() == 0 && !L.Active(), TEXT("a quiet night saves nothing"));
		const auto O = S47J::New(); S47J::Set(O, TEXT("cow"), 4); S47J::Set(O, TEXT("t"), 21.5);
		L.Begin(S47J::Read(S47J::Write(O)));
		Check(L.Active() && L.CowId() == S47Herd::MISSING && S47J::Write(L.Save()) == S47J::Write(O), TEXT("Continue brings the same cow back at the same height"));
		const auto Bad = S47J::New(); S47J::Set(Bad, TEXT("cow"), 2); S47J::Set(Bad, TEXT("t"), 5.);
		L.Begin(Bad);
		const auto Over = S47J::New(); S47J::Set(Over, TEXT("cow"), 4); S47J::Set(Over, TEXT("t"), GoneAt + 1);
		const bool bBadIgnored = !L.Active();
		L.Begin(Over);
		Check(bBadIgnored && !L.Active(), TEXT("another cow, or a lift already finished, is nothing"));
		// Stepping with no world: let down at once when nobody watches, never lifted without a herd.
		FS47CowLift S(nullptr); S.Begin(S47J::Read(S47J::Write(O)));
		Check(S.Step(.1, true, false) == EStep::Lifting && S.Seconds() < 21.5 && !S.Visible(), TEXT("walk away and she is let down again, a step at a time"));
		bool bDown = false; for (int32 I = 0; I < 400 && !bDown; ++I) bDown = S.Step(.25, true, false) == EStep::LetDown;
		Check(bDown && !S.Active() && S.Save()->Values.Num() == 0, TEXT("let down fully: nothing saved, nothing lifted"));
		FS47CowLift W(nullptr); EStep Last = EStep::Idle; int32 Steps = 0;
		while (Steps++ < 1000 && Last != EStep::Done) { const EStep R = W.Step(.25, true, true); if (R != EStep::Idle) Last = R; }
		Check(Last == EStep::Done && !W.Active() && W.Save()->Values.Num() == 0, TEXT("watched to the end she is gone once, and the save is clean"));
	}
	return OK;
}
