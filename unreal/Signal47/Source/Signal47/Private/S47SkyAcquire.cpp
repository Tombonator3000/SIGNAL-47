#include "S47SkyAcquire.h"
#include "S47FreeformJobs.h"
#include "S47Observing.h"

// RX-ACQUIRE-SKY-20261113 (-> M3): see S47SkyAcquire.h. Everything here is deterministic from the job's seed and the clock the
// owner steps it with: no global random numbers, no world.
namespace
{
	uint32 S47SkyMix(uint32 V) { V ^= V >> 16; V *= 0x7feb352du; V ^= V >> 15; V *= 0x846ca68bu; return V ^ (V >> 16); }
	double S47SkyUnit(uint32 V) { return double(S47SkyMix(V) & 0xffffffu) / double(0x1000000); }
	// The three arms' bands on the chart: their directions sixty degrees apart (a triangle round the crossing), each arm's own
	// antennas fanned a little so that a band reads as a bundle; how long each band takes to sweep; and how tight the triangle
	// has to be before an arm's band goes solid (outer arm first, the last one only close in).
	const double S47SkyArmDeg[FS47SkyAcquire::ArmCount] = { 25., 85., 145. };
	const double S47SkyFanDeg = 7.;
	const double S47SkyArmPeriod[FS47SkyAcquire::ArmCount] = { 2.3, 2.9, 3.4 };
	const double S47SkyArmGate[FS47SkyAcquire::ArmCount] = { 5.5, 3.2, 1.6 };
	// Which side of the crossing each arm's band stands off to: the three normals 120 degrees apart, so the crossing lies inside
	// the triangle the bands make (its incentre while the bands stand off alike).
	const double S47SkyArmSide[FS47SkyAcquire::ArmCount] = { 1., -1., 1. };
	const double S47SkySweepShare = .28;  // of the spread: how much a band swings while it sweeps
	const double S47SkyMagnet = 5.;       // the pull inside CaptureRadius, per second at the crossing
	const double S47SkyFollow = 6.;       // the reticle after the pointer, per second
	const double S47SkyLight = 299792458.;
}

const TArray<FS47SkyAcquire::FSource>& FS47SkyAcquire::Catalogue()
{
	static const TArray<FSource> List = []
	{
		TArray<FSource> Out;
		const TArray<FS47Observing::FRun>& RunList = FS47Observing::Runs();
		for (int32 I = 0; I < RunList.Num(); ++I)
		{
			FSource S; S.Name = FString(RunList[I].Source).ToUpper(); S.At = FVector2D((RunList[I].P.Az[0] + RunList[I].P.Az[1]) * .5, RunList[I].El); S.bMajor = true; S.Run = I;
			Out.Add(S);
		}
		// The minor field: a fixed handful of entries, clear of the card sources and of each other, named by their place.
		const int32 Want = RunList.Num() + 11;
		uint32 H = 0x53415243u;
		for (int32 Tries = 0; Out.Num() < Want && Tries < 600; ++Tries)
		{
			H = S47SkyMix(H + 0x9e3779b9u); const double A = AzMin + 4 + (AzMax - AzMin - 8) * S47SkyUnit(H);
			H = S47SkyMix(H + 0x9e3779b9u); const double E = ElMin + 3 + (ElMax - ElMin - 6) * S47SkyUnit(H);
			const FVector2D P(FMath::RoundToDouble(A), FMath::RoundToDouble(E));
			bool bClear = true;
			for (const FSource& O : Out) bClear &= FVector2D::Distance(O.At, P) >= (O.bMajor ? 12. : 8.);
			if (!bClear) continue;
			FSource S; S.At = P; S.Name = FString::Printf(TEXT("SC %03d+%02d"), int32(P.X), int32(P.Y));
			Out.Add(S);
		}
		return Out;
	}();
	return List;
}

FVector2D FS47SkyAcquire::CataloguePlace(int32 InRun)
{
	for (const FSource& S : Catalogue()) if (S.bMajor && S.Run == InRun) return S.At;
	return FVector2D((AzMin + AzMax) * .5, (ElMin + ElMax) * .5);
}

FVector2D FS47SkyAcquire::ClampToChart(FVector2D P, double Margin)
{
	if (!FMath::IsFinite(P.X) || !FMath::IsFinite(P.Y)) return FVector2D((AzMin + AzMax) * .5, (ElMin + ElMax) * .5);
	return FVector2D(FMath::Clamp(P.X, AzMin + Margin, AzMax - Margin), FMath::Clamp(P.Y, ElMin + Margin, ElMax - Margin));
}

void FS47SkyAcquire::HourAngleDec(FVector2D AzEl, double& HaHours, double& DecDeg)
{
	const double A = FMath::DegreesToRadians(AzEl.X), E = FMath::DegreesToRadians(AzEl.Y), Phi = FMath::DegreesToRadians(Latitude);
	const double SinDec = FMath::Clamp(FMath::Sin(Phi) * FMath::Sin(E) + FMath::Cos(Phi) * FMath::Cos(E) * FMath::Cos(A), -1., 1.);
	const double Dec = FMath::Asin(SinDec);
	// azimuth from north through east: a source east of the meridian has a negative hour angle
	const double Ha = FMath::Atan2(-FMath::Sin(A) * FMath::Cos(E), FMath::Sin(E) * FMath::Cos(Phi) - FMath::Cos(E) * FMath::Sin(Phi) * FMath::Cos(A));
	HaHours = FMath::RadiansToDegrees(Ha) / 15.; DecDeg = FMath::RadiansToDegrees(Dec);
}

double FS47SkyAcquire::DelayNs(FVector2D Ground, FVector2D AzEl)
{
	const double A = FMath::DegreesToRadians(AzEl.X), E = FMath::DegreesToRadians(AzEl.Y);
	return (Ground.X * FMath::Sin(A) + Ground.Y * FMath::Cos(A)) * FMath::Cos(E) / S47SkyLight * 1e9;
}

double FS47SkyAcquire::Hash(int32 I, int32 Salt) const
{
	return S47SkyUnit(Seed ^ S47SkyMix(uint32(I) + 0x4a4f4253u) ^ S47SkyMix(uint32(Salt) + 0x5341524fu));
}

void FS47SkyAcquire::Begin(uint32 InSeed, FVector2D InCard, int32 InRun, bool bInOdd)
{
	Seed = InSeed; Card = ClampToChart(InCard); Run = InRun; bOdd = bInOdd;
	Phase = EPhase::NoBearings; Time = SearchT = Hold = LockT = NextSweep = 0; MovedAt = -10; Rings = 0; EventArm = -1;
	bSteady = bBearings = bPointer = bHint = bPlaced = false; SteerDir = FVector2D::ZeroVector; Mean = FixAt = Card;
	for (bool& bSolid : bArm) bSolid = false;
	// Unusual: the crossing stands outside the card source's box, on the side that keeps it on the chart.
	const FVector2D Listed = Run >= 0 ? CataloguePlace(Run) : Card;
	const double Angle = 2 * PI * Hash(0, 91), Reach = BoxRadius + FMath::Lerp(OddOffMin, OddOffMax, Hash(1, 92));
	FVector2D Off(FMath::Cos(Angle) * Reach, FMath::Sin(Angle) * Reach);
	const FVector2D Tried = Listed + Off;
	if (Tried.X < AzMin + 2 || Tried.X > AzMax - 2) Off.X = -Off.X;
	if (Tried.Y < ElMin + 2 || Tried.Y > ElMax - 2) Off.Y = -Off.Y;
	OddAt = ClampToChart(Listed + Off, 1.5);
	Array.Reset();
	Place();
}

void FS47SkyAcquire::Place()
{
	// The reticle starts well away from the crossing: the far side of the chart, a little off its middle line.
	const FVector2D C = bOdd ? OddAt : Card;
	const double Az = C.X < (AzMin + AzMax) * .5 ? AzMax - 14 - 10 * Hash(2, 93) : AzMin + 14 + 10 * Hash(2, 93);
	const double El = C.Y < (ElMin + ElMax) * .5 ? ElMax - 9 - 6 * Hash(3, 94) : ElMin + 9 + 6 * Hash(3, 94);
	Reticle = ClampToChart(FVector2D(Az, El), 1.);
}

void FS47SkyAcquire::SetArray(const TArray<FDish>& InDishes, bool bInSteady, bool bInBearings)
{
	Array = InDishes; bSteady = bInSteady; bBearings = bInBearings;
	FVector2D Sum = FVector2D::ZeroVector; int32 N = 0;
	for (const FDish& D : Array) if (D.bOn && FMath::IsFinite(D.Point.X) && FMath::IsFinite(D.Point.Y)) { Sum += D.Point; ++N; }
	Mean = N ? Sum / double(N) : Card;
}

void FS47SkyAcquire::Nudge(FVector2D Degrees)
{
	if (Phase == EPhase::Locking || Phase == EPhase::Locked || !FMath::IsFinite(Degrees.X) || !FMath::IsFinite(Degrees.Y)) return;
	Reticle = ClampToChart(Reticle + Degrees); bPointer = false; MovedAt = Time;
}

int32 FS47SkyAcquire::OnCount() const
{
	int32 N = 0;
	for (const FDish& D : Array) N += D.bOn ? 1 : 0;
	return N;
}

FVector2D FS47SkyAcquire::Cross() const
{
	if (bOdd) return OddAt;
	// A fix that is held stays where it was taken, whatever the array does afterwards (the calibration cross, a release).
	if (Phase == EPhase::Locked) return FixAt;
	// A steady array is on the card; a moving one is wherever its antennas point just now.
	return ClampToChart(bSteady || Array.IsEmpty() ? Card : Mean);
}

double FS47SkyAcquire::SpreadAt(double D, double Rest)
{
	const double X = FMath::Clamp(D / FarRange, 0., 1.);
	return Rest + (FarSpread - Rest) * FMath::Pow(X, .75);
}

double FS47SkyAcquire::Spread() const
{
	if (Phase == EPhase::Locked) return Residual();
	// An array still on its way gives bearings that will not hold: the triangle stays open however near the reticle is.
	return SpreadAt(Distance(), Residual()) + (bSteady ? 0. : 2.5);
}

FS47SkyAcquire::FLine FS47SkyAcquire::Line(int32 Dish) const
{
	FLine L;
	if (!Array.IsValidIndex(Dish) || !bBearings) return L;
	const FDish& D = Array[Dish];
	L.Arm = ArmOf(Dish); L.bOn = D.bOn;
	const int32 PerArm = FMath::Max(1, DishCount / ArmCount), InArm = Dish - L.Arm * PerArm;
	const double Fan = PerArm > 1 ? (double(InArm) - (PerArm - 1) * .5) / ((PerArm - 1) * .5) : 0.;
	const double Angle = FMath::DegreesToRadians(S47SkyArmDeg[L.Arm] + S47SkyFanDeg * Fan);
	L.Dir = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
	const FVector2D Normal(-L.Dir.Y, L.Dir.X);
	const double Wide = Spread();
	// Each antenna keeps its own place in its arm's band; the band swings while it sweeps and stops as it closes.
	const double Share = .72 + .28 * Hash(Dish, 11);
	const double Swing = S47SkySweepShare * FMath::Sin(2 * PI * Time / S47SkyArmPeriod[L.Arm] + .35 * InArm + 2 * PI * Hash(L.Arm, 12));
	// An antenna off the array's mean pointing (still turning, or held elsewhere) stands off by that much.
	const double Own = bOdd || bSteady || Phase == EPhase::Locked ? 0. : FVector2D::DotProduct(D.Point - Mean, Normal);
	L.Off = S47SkyArmSide[L.Arm] * Wide * (Share + Swing) + FMath::Clamp(Own, -40., 40.);
	L.Through = Cross() + Normal * L.Off;
	L.bSolid = bArm[L.Arm];
	return L;
}

bool FS47SkyAcquire::Triangle(FVector2D Corner[ArmCount]) const
{
	if (!HasBearings()) return false;
	FVector2D At[ArmCount], Along[ArmCount];
	for (int32 A = 0; A < ArmCount; ++A)
	{
		double Sum = 0; int32 N = 0;
		for (int32 I = 0; I < Array.Num(); ++I) { if (ArmOf(I) != A || !Array[I].bOn) continue; Sum += Line(I).Off; ++N; }
		if (!N) return false;
		const double Angle = FMath::DegreesToRadians(S47SkyArmDeg[A]);
		Along[A] = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
		At[A] = Cross() + FVector2D(-Along[A].Y, Along[A].X) * (Sum / N);
	}
	for (int32 A = 0; A < ArmCount; ++A)
	{
		const int32 B = (A + 1) % ArmCount;
		const double Den = FVector2D::CrossProduct(Along[A], Along[B]);
		if (FMath::Abs(Den) < 1e-6) return false;
		Corner[A] = At[A] + Along[A] * (FVector2D::CrossProduct(At[B] - At[A], Along[B]) / Den);
	}
	return true;
}

double FS47SkyAcquire::MeanOff() const
{
	double Sum = 0; int32 N = 0;
	for (int32 I = 0; I < Array.Num(); ++I) { const FLine L = Line(I); if (L.bOn) { Sum += FMath::Abs(L.Off); ++N; } }
	return N ? Sum / N : 0.;
}

bool FS47SkyAcquire::ClipSegment(FVector2D& A, FVector2D& B)
{
	// Liang-Barsky against the chart's window
	const FVector2D D = B - A; double T0 = 0, T1 = 1;
	const double Edge[4][2] = { { -D.X, A.X - AzMin }, { D.X, AzMax - A.X }, { -D.Y, A.Y - ElMin }, { D.Y, ElMax - A.Y } };
	for (const auto& E : Edge)
	{
		if (FMath::Abs(E[0]) < 1e-12) { if (E[1] < 0) return false; continue; }
		const double R = E[1] / E[0];
		if (E[0] < 0) { if (R > T1) return false; T0 = FMath::Max(T0, R); }
		else { if (R < T0) return false; T1 = FMath::Min(T1, R); }
	}
	const FVector2D From = A;
	A = From + D * T0; B = From + D * T1;
	return T1 > T0;
}

int32 FS47SkyAcquire::InCount() const
{
	int32 N = 0;
	for (int32 I = 0; I < Array.Num(); ++I) { const FLine L = Line(I); N += L.bOn && FMath::Abs(L.Off) <= ClosedWithin ? 1 : 0; }
	return N;
}

uint32 FS47SkyAcquire::UpdateArms()
{
	uint32 Ev = 0;
	const double Wide = Spread();
	for (int32 A = 0; A < ArmCount; ++A)
	{
		// Unusual: the last arm's gate is below what the bearings can ever reach, so its band never goes solid.
		const double Gate = bOdd && A == ArmCount - 1 ? OddResidual * .5 : S47SkyArmGate[A];
		const bool bWant = bSteady && (bArm[A] ? Wide <= Gate * 1.25 : Wide <= Gate);
		if (bWant != bArm[A]) { bArm[A] = bWant; EventArm = A; Ev |= bWant ? EvLine : EvLineLost; }
	}
	return Ev;
}

uint32 FS47SkyAcquire::Step(double Dt)
{
	if (!FMath::IsFinite(Dt) || Dt <= 0) return 0;
	Dt = FMath::Min(Dt, .1); Time += Dt;
	if (Phase == EPhase::Locked) return 0;
	uint32 Ev = 0;
	const bool bHave = HasBearings();
	if (!bHave)
	{
		if (Phase == EPhase::Locking) Ev |= EvBlurt;
		Phase = EPhase::NoBearings; Hold = LockT = 0; Rings = 0;
		for (bool& bSolid : bArm) bSolid = false;
	}
	else if (Phase == EPhase::NoBearings) { Phase = EPhase::Search; if (!bPlaced) { Place(); bPlaced = true; } }
	if (Phase == EPhase::Locking)
	{
		if (!bSteady) { Phase = EPhase::Search; Hold = LockT = 0; Rings = 0; Ev |= EvBlurt; }
		else
		{
			const FVector2D C = Cross();
			Reticle += (C - Reticle) * FMath::Min(1., Dt * 8.);
			LockT += Dt;
			while (Rings < ArmCount && LockT >= LockSeconds * (Rings + 1) / (ArmCount + 1)) { ++Rings; Ev |= EvRing; }
			Ev |= UpdateArms();
			if (LockT >= LockSeconds) { FixAt = C; Phase = EPhase::Locked; Reticle = C; Rings = ArmCount; Ev |= EvLocked; }
		}
		return Ev;
	}
	// The reticle: after the pointer, by the keys, eased in close to the crossing, and carried by the assist once it has begun.
	FVector2D V = FVector2D::ZeroVector;
	if (bPointer) V += ((Pointer - Reticle) * S47SkyFollow).GetClampedToMaxSize(SlewRate);
	V += SteerDir * KeyRate;
	const bool bHand = V.SizeSquared() > .25;
	if (Phase == EPhase::Search && bSteady)
	{
		const FVector2D C = Cross(); const FVector2D To = C - Reticle; const double D = To.Size();
		if (D > 1e-6)
		{
			if (D < CaptureRadius) V += To * (S47SkyMagnet * (1. - D / CaptureRadius));
			const double Assist = AssistLevel();
			if (Assist > 0) V += To / D * FMath::Min(AssistSpeed * Assist, D * 4.);
		}
	}
	if (V.SizeSquared() > .25) MovedAt = Time;
	Reticle = ClampToChart(Reticle + V * Dt);
	if (Phase != EPhase::Search) return Ev;
	Ev |= UpdateArms();
	if (bSteady)
	{
		SearchT += Dt;
		if (!bHint && SearchT >= HintAfter) { bHint = true; Ev |= EvHint; }
		if (Distance() <= LockRadius) { Hold += Dt; if (Hold >= HoldSeconds) { Phase = EPhase::Locking; LockT = 0; Rings = 0; Hold = HoldSeconds; Ev |= EvLockStart; return Ev; } }
		else Hold = 0;
	}
	else Hold = 0;
	// The search tick: slow far off, a fast chatter close in, and only while the reticle moves.
	if (Time - MovedAt < .35 && (bHand || AssistLevel() > 0 || Distance() < CaptureRadius) && Time >= NextSweep)
	{
		Ev |= EvSweep; NextSweep = Time + FMath::Lerp(.42, .09, FMath::Pow(Closeness(), 3.));
	}
	return Ev;
}

bool FS47SkyAcquire::Solve()
{
	if (Phase == EPhase::Locked) return true;
	if (!HasBearings() || !bSteady) return false;
	ForceLocked();
	return true;
}

void FS47SkyAcquire::ForceLocked()
{
	FixAt = bOdd ? OddAt : ClampToChart(Card);
	Phase = EPhase::Locked; bSteady = true; bHint = true; Hold = HoldSeconds; LockT = LockSeconds; Rings = ArmCount; bPointer = false; SteerDir = FVector2D::ZeroVector;
	Reticle = FixAt;
	for (int32 A = 0; A < ArmCount; ++A) bArm[A] = !(bOdd && A == ArmCount - 1);
}

double FS47SkyAcquire::SignalToNoise() const
{
	// What a whole array makes of the source: every antenna in adds to it. The unusual kind wavers.
	const double Base = 8.5 + 10. * FMath::LogX(10., double(FMath::Max(1, OnCount())));
	return bOdd ? Base - 2.2 + 1.4 * FMath::Sin(Time * 2.1) : Base;
}

double FS47SkyAcquire::ArmDelayNs(int32 Arm) const
{
	FVector2D Centre = FVector2D::ZeroVector; int32 N = 0;
	for (const FDish& D : Array) { Centre += D.Ground; ++N; }
	if (N) Centre /= double(N);
	FVector2D Far = FVector2D::ZeroVector; double Most = -1;
	for (int32 I = 0; I < Array.Num(); ++I)
	{
		if (ArmOf(I) != Arm || !Array[I].bOn) continue;
		const FVector2D G = Array[I].Ground - Centre;
		if (G.SizeSquared() > Most) { Most = G.SizeSquared(); Far = G; }
	}
	return DelayNs(Far, Cross());
}

const FS47SkyAcquire::FSource* FS47SkyAcquire::NearestCatalogued(FVector2D At, bool bMajorOnly) const
{
	const FSource* Best = nullptr; double BestD = 1e9;
	for (const FSource& S : Catalogue())
	{
		if (bMajorOnly && !S.bMajor) continue;
		const double D = FVector2D::Distance(S.At, At);
		if (D < BestD) { BestD = D; Best = &S; }
	}
	return Best;
}

double FS47SkyAcquire::CatalogueOffset() const
{
	const FSource* S = NearestCatalogued(Cross(), true);
	return S ? FVector2D::Distance(S->At, Cross()) : 0.;
}

bool FS47SkyAcquire::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bPass, const FString& What) { bOk &= bPass; Lines.Add(FString(bPass ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("sky acquire: ") + What); };
	const double Tick = 1. / 60.;
	// An array of 27 on the card, standing as a Y of three arms (the self-test has no world to ask).
	auto ArrayOn = [](FVector2D At, int32 Off = 0)
	{
		TArray<FDish> Out;
		for (int32 I = 0; I < DishCount; ++I)
		{
			FDish D; D.bOn = I >= Off; D.Point = At;
			const double Arm = FMath::DegreesToRadians(120. * ArmOf(I)), Reach = 40. * (1 + I % (DishCount / ArmCount));
			D.Ground = FVector2D(FMath::Sin(Arm) * Reach, FMath::Cos(Arm) * Reach);
			Out.Add(D);
		}
		return Out;
	};
	const TArray<FS47Observing::FRun>& RunList = FS47Observing::Runs();

	{ // the catalogue and the cards
		const TArray<FSource>& Cat = Catalogue();
		int32 Majors = 0; bool bInside = true, bApart = true, bNamed = true;
		for (int32 I = 0; I < Cat.Num(); ++I)
		{
			Majors += Cat[I].bMajor ? 1 : 0; bNamed &= !Cat[I].Name.IsEmpty();
			bInside &= Cat[I].At.X >= AzMin + 3 && Cat[I].At.X <= AzMax - 3 && Cat[I].At.Y >= ElMin + 3 && Cat[I].At.Y <= ElMax - 3;
			for (int32 K = I + 1; K < Cat.Num(); ++K) bApart &= FVector2D::Distance(Cat[I].At, Cat[K].At) >= 8.;
		}
		Check(Majors == RunList.Num() && Cat.Num() >= Majors + 6 && bInside && bApart && bNamed,
			FString::Printf(TEXT("the catalogue lists the %d card sources and %d minor entries, all on the chart and apart"), Majors, Cat.Num() - Majors));
		bool bCards = true; double Worst = 0;
		for (int32 CardSeed : { 0, 47, 470047, -1 }) for (int32 Day = 1; Day <= 21; ++Day) for (int32 Slot = 0; Slot < FS47FreeformJobs::DailyCount; ++Slot) for (int32 Src : { -1, 2, 3 })
		{
			const FS47FreeformJobs::FCard C = FS47FreeformJobs::Make(CardSeed, Day, Slot, Src);
			const FVector2D At((C.Receiver.Az[0] + C.Receiver.Az[1]) * .5, C.El);
			const double Off = FVector2D::Distance(At, CataloguePlace(C.Source));
			Worst = FMath::Max(Worst, Off);
			bCards &= Off < BoxRadius && ClampToChart(At, BoxRadius).Equals(At, 1e-9);
		}
		Check(bCards, FString::Printf(TEXT("every generated card lies on the chart inside its source's box (farthest %.2f of %.1f degrees)"), Worst, BoxRadius));
	}
	{ // convergence geometry
		bool bMono = true; double Was = SpreadAt(FarRange * 1.5, 0);
		for (double D = FarRange; D >= 0; D -= .5) { const double S = SpreadAt(D, 0); bMono &= S <= Was + 1e-12; Was = S; }
		Check(bMono && FMath::IsNearlyEqual(SpreadAt(FarRange, 0), FarSpread) && SpreadAt(0, 0) == 0 && FMath::IsNearlyEqual(SpreadAt(0, OddResidual), OddResidual) && SpreadAt(LockRadius, 0) < 1.2,
			TEXT("the bearings stand off less the nearer the reticle: the whole spread far away, none on the crossing"));
		FS47SkyAcquire A; const FVector2D Card0(112, 54);
		A.Begin(4711, Card0, 1, false); A.SetArray(ArrayOn(Card0), true, true); A.Step(Tick);
		const double FarOff = A.Distance(); double FarMost = 0, FarLeast = 1e9;
		for (int32 I = 0; I < DishCount; ++I) { const FLine L = A.Line(I); FarMost = FMath::Max(FarMost, FMath::Abs(L.Off)); FarLeast = FMath::Min(FarLeast, FMath::Abs(L.Off)); }
		A.ForceLocked(); double NearMost = 0; bool bThrough = true;
		for (int32 I = 0; I < DishCount; ++I)
		{
			const FLine L = A.Line(I); NearMost = FMath::Max(NearMost, FMath::Abs(L.Off));
			const FVector2D Normal(-L.Dir.Y, L.Dir.X);
			bThrough &= FMath::Abs(FVector2D::DotProduct(A.Cross() - L.Through, Normal)) < 1e-9 && FMath::IsNearlyEqual(L.Dir.Size(), 1.);
		}
		Check(FarOff > 30 && FarLeast > 2 && FarMost < FarSpread * 1.3 && NearMost < 1e-9 && bThrough && A.Cross().Equals(Card0, 1e-9) && A.InCount() == DishCount,
			FString::Printf(TEXT("27 bearings stand %.1f to %.1f degrees off with the reticle %.0f away, and all pass through the card's place once locked"), FarLeast, FarMost, FarOff));
		// three bands sixty degrees apart: any two arms' middle lines cross, so there is a triangle to close
		const FLine L0 = A.Line(4), L1 = A.Line(13), L2 = A.Line(22);
		const double C01 = FMath::Abs(FVector2D::CrossProduct(L0.Dir, L1.Dir)), C12 = FMath::Abs(FVector2D::CrossProduct(L1.Dir, L2.Dir)), C02 = FMath::Abs(FVector2D::CrossProduct(L0.Dir, L2.Dir));
		Check(L0.Arm == 0 && L1.Arm == 1 && L2.Arm == 2 && C01 > .8 && C12 > .8 && C02 > .8, TEXT("the three arms' bands cross each other at sixty degrees"));
		// the triangle: round the crossing while the bands stand off, a point once locked; and the chart's own clipping
		FS47SkyAcquire Tri; Tri.Begin(4711, Card0, 1, false); Tri.SetArray(ArrayOn(Card0), true, true); Tri.Step(Tick);
		FVector2D Corner[ArmCount]; const bool bTri = Tri.Triangle(Corner);
		auto Side = [](FVector2D P, FVector2D Q, FVector2D R) { return FVector2D::CrossProduct(Q - P, R - P); };
		const FVector2D X = Tri.Cross();
		const double S0 = bTri ? Side(Corner[0], Corner[1], X) : 0., S1 = bTri ? Side(Corner[1], Corner[2], X) : 0., S2 = bTri ? Side(Corner[2], Corner[0], X) : 0.;
		const bool bInside = bTri && ((S0 > 0 && S1 > 0 && S2 > 0) || (S0 < 0 && S1 < 0 && S2 < 0));
		const double Open = bTri ? FVector2D::Distance(Corner[0], Corner[1]) : 0.;
		Tri.ForceLocked(); const bool bShut = Tri.Triangle(Corner) && FVector2D::Distance(Corner[0], X) < 1e-6 && FVector2D::Distance(Corner[1], X) < 1e-6 && FVector2D::Distance(Corner[2], X) < 1e-6 && Tri.MeanOff() < 1e-9;
		Check(bInside && Open > 10 && bShut, FString::Printf(TEXT("the bands make a triangle with the crossing inside it (%.0f degrees a side far away) that shuts to a point on the lock"), Open));
		FVector2D CutA(AzMin - 30, 40), CutB(AzMax + 30, 40), OutA(AzMin - 30, ElMax + 5), OutB(AzMax + 30, ElMax + 9), InA(60, 30), InB(70, 35);
		const bool bCut = ClipSegment(CutA, CutB) && FMath::IsNearlyEqual(CutA.X, AzMin) && FMath::IsNearlyEqual(CutB.X, AzMax) && FMath::IsNearlyEqual(CutA.Y, 40.) && FMath::IsNearlyEqual(CutB.Y, 40.);
		Check(bCut && !ClipSegment(OutA, OutB) && ClipSegment(InA, InB) && InA.Equals(FVector2D(60, 30), 1e-9) && InB.Equals(FVector2D(70, 35), 1e-9), TEXT("a bearing is drawn only where it lies on the chart"));
		// antennas off remote give no bearing; the rest still close
		FS47SkyAcquire Few; Few.Begin(4711, Card0, 1, false); Few.SetArray(ArrayOn(Card0, 3), true, true); Few.Step(Tick);
		Check(Few.OnCount() == 24 && !Few.Line(0).bOn && Few.Line(3).bOn && Few.Solve() && Few.InCount() == 24 && Few.SignalToNoise() < A.SignalToNoise(), TEXT("three antennas off remote: 24 bearings, a weaker fix, still a fix"));
	}
	{ // the lock rule
		FS47SkyAcquire A; const FVector2D Card0(70, 25);
		A.Begin(99, Card0, 0, false); A.SetArray(ArrayOn(Card0), true, true); A.Step(Tick);
		const bool bStartsFar = A.State() == EPhase::Search && A.Distance() > 30;
		A.Nudge(A.Cross() - A.ReticleAt());
		uint32 Seen = 0; double T = 0, StartAt = -1, DoneAt = -1; int32 RingCount = 0;
		for (; T < 10 && !A.Locked(); T += Tick)
		{
			A.SetArray(ArrayOn(Card0), true, true); const uint32 Ev = A.Step(Tick); Seen |= Ev; RingCount += (Ev & EvRing) ? 1 : 0;
			if ((Ev & EvLockStart) && StartAt < 0) StartAt = T;
			if (Ev & EvLocked) DoneAt = T;
		}
		Check(bStartsFar && A.Locked() && FMath::Abs(StartAt - HoldSeconds) < .1 && FMath::Abs(DoneAt - StartAt - LockSeconds) < .1 && RingCount == ArmCount && (Seen & EvBlurt) == 0 && A.ArmSolid(0) && A.ArmSolid(1) && A.ArmSolid(2),
			FString::Printf(TEXT("on the crossing the lock starts after %.1f s and holds after %.1f s more, three rings, all three bands solid"), StartAt, DoneAt - StartAt));
		// an array still turning never locks, and one that loses its pointing during the sequence breaks it off
		FS47SkyAcquire B; B.Begin(99, Card0, 0, false); B.SetArray(ArrayOn(Card0), false, true); B.Step(Tick); B.Nudge(B.Cross() - B.ReticleAt());
		bool bNever = true;
		for (T = 0; T < 8; T += Tick) { B.SetArray(ArrayOn(Card0), false, true); B.Step(Tick); bNever &= B.State() == EPhase::Search; }
		Check(bNever && !B.Solve(), TEXT("no lock, and no way through, while the array is still turning"));
		uint32 Broke = 0;
		for (T = 0; T < 3 && B.State() != EPhase::Locking; T += Tick) { B.SetArray(ArrayOn(Card0), true, true); B.Step(Tick); }
		const bool bWasLocking = B.State() == EPhase::Locking;
		for (T = 0; T < .5; T += Tick) { B.SetArray(ArrayOn(Card0), false, true); Broke |= B.Step(Tick); }
		Check(bWasLocking && B.State() == EPhase::Search && (Broke & EvBlurt) != 0 && B.LockProgress() == 0, TEXT("pointing lost during the lock sequence: back to the search, with the error tone"));
		// not pointed at all: nothing to find, nothing to lock
		FS47SkyAcquire N; N.Begin(99, Card0, 0, false); N.SetArray(ArrayOn(Card0), false, false);
		for (T = 0; T < 30; T += Tick) N.Step(Tick);
		Check(N.State() == EPhase::NoBearings && !N.HasBearings() && !N.Line(5).bOn && N.SearchSeconds() == 0 && !N.Solve(), TEXT("an array that has not been pointed gives no bearings and starts no clock"));
	}
	{ // the hint and the assist: a chart nobody touches still locks, and nothing moves it before the assist has begun
		bool bAll = true; double Slowest = 0, FirstStir = 1e9;
		for (int32 R = 0; R < RunList.Num(); ++R)
		{
			FS47SkyAcquire A; const FVector2D Spot = CataloguePlace(R);
			A.Begin(1000 + R, Spot, R, false); A.SetArray(ArrayOn(Spot), true, true); A.Step(Tick);
			const FVector2D From = A.ReticleAt(); double T = 0, HintAt = -1, Stirred = -1;
			for (; T < 60 && !A.Locked(); T += Tick)
			{
				A.SetArray(ArrayOn(Spot), true, true); const uint32 Ev = A.Step(Tick);
				if ((Ev & EvHint) && HintAt < 0) HintAt = T;
				if (Stirred < 0 && !A.ReticleAt().Equals(From, 1e-6)) Stirred = T;
			}
			bAll &= A.Locked() && FMath::Abs(HintAt - HintAfter) < .1 && Stirred >= AssistAfter - .1;
			Slowest = FMath::Max(Slowest, T); FirstStir = FMath::Min(FirstStir, Stirred);
		}
		Check(bAll && Slowest < 34, FString::Printf(TEXT("untouched: the hint at %.0f s, the assist moves the reticle from %.1f s, and the fix is held within %.1f s"), HintAfter, FirstStir, Slowest));
	}
	{ // by hand: the pointer, the keys and the step keys
		auto ByPointer = [&](FVector2D Miss, double Limit, int32* SweepTicks = nullptr, int32* LineLocks = nullptr)
		{
			FS47SkyAcquire A; const FVector2D Spot(146, 64);
			A.Begin(7, Spot, 3, false); double T = 0;
			for (; T < Limit && !A.Locked(); T += Tick)
			{
				A.SetArray(ArrayOn(Spot), true, true); A.PointAt(Spot + Miss); const uint32 Ev = A.Step(Tick);
				if (SweepTicks) *SweepTicks += (Ev & EvSweep) ? 1 : 0;
				if (LineLocks) *LineLocks += (Ev & EvLine) ? 1 : 0;
			}
			return A.Locked() ? T : -1.;
		};
		int32 SweepTicks = 0, LineLocks = 0;
		const double OnIt = ByPointer(FVector2D(0, 0), 20, &SweepTicks, &LineLocks), Near = ByPointer(FVector2D(1.5, -1.2), 20), Wide = ByPointer(FVector2D(4.5, 3.), AssistAfter - 1);
		Check(OnIt > 0 && OnIt < 10 && Near > 0 && Near < 11 && Wide < 0,
			FString::Printf(TEXT("the pointer on the crossing holds the fix in %.1f s, a pointer 1.9 degrees off is eased in (%.1f s), one 5.4 degrees off is not"), OnIt, Near));
		Check(SweepTicks >= 4 && LineLocks == ArmCount, FString::Printf(TEXT("on the way in the search ticks %d times and each arm's band locks once (%d)"), SweepTicks, LineLocks));
		FS47SkyAcquire K; const FVector2D Spot(90, 48);
		K.Begin(8, Spot, 2, false); double T = 0;
		for (; T < 20 && !K.Locked(); T += Tick)
		{
			K.SetArray(ArrayOn(Spot), true, true);
			K.Steer(K.Distance() > 2.5 ? (K.Cross() - K.ReticleAt()).GetSafeNormal() : FVector2D::ZeroVector); K.Step(Tick);
		}
		const double ByKeys = K.Locked() ? T : -1.;
		FS47SkyAcquire S; S.Begin(9, Spot, 2, false); S.SetArray(ArrayOn(Spot), true, true); S.Step(Tick);
		int32 Presses = 0;
		for (; Presses < 60; ++Presses)
		{
			const FVector2D To = S.Cross() - S.ReticleAt();
			if (FMath::Abs(To.X) > 2.5) S.Nudge(FVector2D(To.X > 0 ? 5 : -5, 0));
			else if (FMath::Abs(To.Y) > 2.5) S.Nudge(FVector2D(0, To.Y > 0 ? 5 : -5));
			else break;
		}
		for (T = 0; T < 10 && !S.Locked(); T += Tick) { S.SetArray(ArrayOn(Spot), true, true); S.Step(Tick); }
		Check(ByKeys > 0 && ByKeys < 12 && S.Locked() && Presses < 40 && T < AssistAfter,
			FString::Printf(TEXT("the keys hold the fix in %.1f s; %d five-degree steps land near enough to be eased in (%.1f s more)"), ByKeys, Presses, T));
		// at rest and far away nothing ticks
		FS47SkyAcquire Q; Q.Begin(10, Spot, 2, false); int32 Idle = 0;
		for (T = 0; T < AssistAfter - 1; T += Tick) { Q.SetArray(ArrayOn(Spot), true, true); Idle += (Q.Step(Tick) & EvSweep) ? 1 : 0; }
		Check(Idle == 0, TEXT("a reticle at rest far from the crossing is quiet"));
	}
	{ // the unusual variant
		bool bPlaces = true; double Least = 1e9, Most = 0;
		for (int32 R = 0; R < RunList.Num(); ++R) for (uint32 S = 1; S <= 60; ++S)
		{
			FS47SkyAcquire A; const FVector2D Spot = CataloguePlace(R) + FVector2D(double(S % 3) - 1., double(S % 2) - .5) * 2.;
			A.Begin(S * 7919u, Spot, R, true); A.SetArray(ArrayOn(Spot), true, true);
			const FVector2D C = A.Cross();
			for (const FSource& Listed : Catalogue()) if (Listed.bMajor) { const double Off = FVector2D::Distance(C, Listed.At); Least = FMath::Min(Least, Off); bPlaces &= Off >= BoxRadius + OddOffMin - 1e-6; }
			Most = FMath::Max(Most, FVector2D::Distance(C, CataloguePlace(R)));
			bPlaces &= ClampToChart(C, 1.4).Equals(C, 1e-9) && A.CatalogueOffset() > BoxRadius;
		}
		Check(bPlaces && Most <= BoxRadius + OddOffMax + 1e-6, FString::Printf(TEXT("unusual: the bearings cross off the catalogue, %.1f to %.1f degrees from the card's source and outside every box"), Least, Most));
		FS47SkyAcquire A, Plain; const FVector2D Spot(112, 54);
		A.Begin(31, Spot, 1, true); Plain.Begin(31, Spot, 1, false); Plain.SetArray(ArrayOn(Spot), true, true); Plain.Step(Tick);
		int32 LineLocks = 0; double T = 0; uint32 Seen = 0;
		for (; T < 20 && !A.Locked(); T += Tick) { A.SetArray(ArrayOn(Spot), true, true); A.PointAt(A.Cross()); const uint32 Ev = A.Step(Tick); Seen |= Ev; LineLocks += (Ev & EvLine) ? 1 : 0; }
		double Tightest = 1e9, Loosest = 0, SnLow = 1e9, SnHigh = -1e9;
		for (int32 Frame = 0; Frame < 600; ++Frame)
		{
			A.Step(Tick); // (locked: only the clock moves, so the triangle keeps breathing)
			for (int32 I = 0; I < DishCount; ++I) { const double Off = FMath::Abs(A.Line(I).Off); Tightest = FMath::Min(Tightest, Off); Loosest = FMath::Max(Loosest, Off); }
			SnLow = FMath::Min(SnLow, A.SignalToNoise()); SnHigh = FMath::Max(SnHigh, A.SignalToNoise());
		}
		Check(A.Locked() && (Seen & EvLocked) != 0 && T < 12, FString::Printf(TEXT("unusual: the fix is still held (%.1f s), never a blocker"), T));
		Check(LineLocks == ArmCount - 1 && A.ArmSolid(0) && A.ArmSolid(1) && !A.ArmSolid(ArmCount - 1) && Tightest > .15 && Loosest < .8 && Loosest - Tightest > .2,
			FString::Printf(TEXT("unusual: the bearings will not quite close (%.2f to %.2f degrees off, breathing) and the last arm's band never goes solid"), Tightest, Loosest));
		Check(SnHigh < Plain.SignalToNoise() && SnHigh - SnLow > 1. && A.Residual() > 0 && Plain.Residual() == 0 && Plain.CatalogueOffset() < BoxRadius,
			TEXT("unusual: a weaker, wavering signal-to-noise and a residual; the ordinary card has neither and lies inside its box"));
	}
	{ // the way through without a hand on it, and the same chart for the same job
		FS47SkyAcquire A; const FVector2D Spot(70, 25);
		A.Begin(5, Spot, 0, false);
		const bool bEarly = A.Solve();
		A.SetArray(ArrayOn(Spot), true, true);
		const bool bSolved = A.Solve();
		Check(!bEarly && bSolved && A.Locked() && A.ReticleAt().Equals(A.Cross(), 1e-9) && A.LockProgress() == 1. && A.RingsClosed() == ArmCount && A.Step(Tick) == 0,
			TEXT("the programmatic path: no fix before the array is told, the fix at once on a steady array, and nothing more to step"));
		// a fix that is held stays where it was taken while the array moves on (the calibration cross, a release to the zenith)
		A.SetArray(ArrayOn(FVector2D(0, 90)), false, true); A.Step(Tick);
		Check(A.Locked() && A.Cross().Equals(Spot, 1e-9) && A.MeanOff() < 1e-9 && A.InCount() == DishCount, TEXT("a held fix stays where it was taken while the array moves on"));
		FS47SkyAcquire B, C;
		B.Begin(77, Spot, 0, false); C.Begin(77, Spot, 0, false); bool bSame = true;
		for (int32 Frame = 0; Frame < 900; ++Frame)
		{
			B.SetArray(ArrayOn(Spot), true, true); C.SetArray(ArrayOn(Spot), true, true);
			B.PointAt(FVector2D(100, 40)); C.PointAt(FVector2D(100, 40));
			bSame &= B.Step(Tick) == C.Step(Tick) && B.ReticleAt().Equals(C.ReticleAt(), 0.) && B.Line(7).Off == C.Line(7).Off;
		}
		Check(bSame, TEXT("the same job gives the same chart, step for step"));
	}
	{ // the readouts
		double Ha = 9, Dec = 0;
		HourAngleDec(FVector2D(123, 90), Ha, Dec);
		const bool bZenith = FMath::Abs(Dec - Latitude) < 1e-6 && FMath::Abs(Ha) < 1e-6;
		HourAngleDec(FVector2D(180, 40), Ha, Dec);
		const bool bSouth = FMath::Abs(Dec - (40 - (90 - Latitude))) < 1e-6 && FMath::Abs(Ha) < 1e-6;
		HourAngleDec(FVector2D(90, 30), Ha, Dec);
		const bool bEast = Ha < -1 && Ha > -12;
		Check(bZenith && bSouth && bEast, TEXT("hour angle and declination: the zenith at the station's latitude, the meridian at zero, the east before it"));
		const double Along = DelayNs(FVector2D(0, 300), FVector2D(0, 0)), Up = DelayNs(FVector2D(0, 300), FVector2D(0, 90)), Across = DelayNs(FVector2D(0, 300), FVector2D(90, 0));
		FS47SkyAcquire A; A.Begin(5, FVector2D(70, 25), 0, false); A.SetArray(ArrayOn(FVector2D(70, 25)), true, true);
		bool bDelays = true;
		for (int32 Arm = 0; Arm < ArmCount; ++Arm) bDelays &= FMath::IsFinite(A.ArmDelayNs(Arm)) && FMath::Abs(A.ArmDelayNs(Arm)) < 400. / .299792458 + 1;
		Check(FMath::Abs(Along - 1000.69) < .1 && FMath::Abs(Up) < 1e-6 && FMath::Abs(Across) < 1e-6 && bDelays, TEXT("baseline delay: 300 m along the line of sight is 1000.7 ns, none across it or straight up"));
	}
	return bOk;
}
