#pragma once
#include "CoreMinimal.h"

// RX-ACQUIRE-SKY-20261113 (-> M3, Tom 13 November: "ACQUIRE og TUNE er nesten identisk"): the receiver's ACQUIRE page as a search of
// the sky. After TRACKING has put the array on the card's bearing, the receiver has to find where on its chart the antennas agree:
// each antenna gives one bearing line, the lines of an arm stand together as a band, and the three bands make a triangle round the
// place the array hears. The operator steers a reticle (the receiver's phase centre) toward it; the nearer the reticle, the tighter
// the triangle; with the reticle on the crossing for a moment a lock sequence runs and the fix is held. TUNE is the frequency search
// that follows.
//
// This is the whole rule, plain C++ (no Slate, no world): the chart's window, the catalogue drawn on it, where the bearings cross,
// how far each stands off, the lock rule, the hint and the assist, the unusual variant, and the way through without a hand on it
// (Solve). SS47SkyChart draws it; FS47FreeformSession owns one and tells it what the array does (S47FreeformSky.cpp).
//
// Short and never in the way: about 15 to 30 seconds for a normal card, a hint after HintAfter seconds, and from AssistAfter the
// reticle is eased toward the crossing by itself, so that a chart nobody touches still locks.
//
// The unusual variant (a take that will carry extra ticks): the bearings cross at a point the catalogue does not list, outside the
// card source's box, and never quite close (a small triangle stays, breathing; the third arm never goes solid). Nothing says so in
// words: the readouts carry the same fields with other numbers.
class SIGNAL47_API FS47SkyAcquire
{
public:
	// The chart's window, degrees of azimuth and elevation. Drawn to one scale on both axes; distances are plain degrees on it.
	static constexpr double AzMin = 50, AzMax = 166, ElMin = 18, ElMax = 70;
	static constexpr int32 DishCount = 27, ArmCount = 3;
	static constexpr double LockRadius = 1.6;     // the reticle this near the crossing: the fix can be taken
	static constexpr double CaptureRadius = 6.;   // inside it the reticle is eased in at once
	static constexpr double FarSpread = 11.;      // how far a bearing stands off the crossing with the reticle far away
	static constexpr double FarRange = 45.;       // "far away"
	static constexpr double HoldSeconds = .7;     // on the crossing this long before the lock sequence starts
	static constexpr double LockSeconds = 3.4;    // rings closing, readouts counting
	static constexpr double HintAfter = 6., AssistAfter = 11., AssistFull = 24.; // seconds of searching with a steady array
	static constexpr double AssistSpeed = 16.;    // degrees a second at full assist
	static constexpr double SlewRate = 26.;       // the reticle after the pointer, degrees a second at most
	static constexpr double KeyRate = 24.;        // the reticle by keys, degrees a second
	static constexpr double BoxRadius = 3.;       // the catalogue's box round a card source (every card lies inside its own)
	static constexpr double OddResidual = .55;    // unusual: the bearings never come nearer than this
	static constexpr double OddOffMin = 1.4, OddOffMax = 2.8; // unusual: the crossing stands this far outside the box
	static constexpr double ClosedWithin = 1.;    // a bearing this near the crossing counts as in
	static constexpr double Latitude = 34.08;     // the station's, for the hour angle and declination on the readout

	enum class EPhase : uint8 { NoBearings, Search, Locking, Locked };
	// What Step says happened, for the sounds.
	enum EEvent : uint32 { EvSweep = 1, EvLine = 2, EvLockStart = 4, EvRing = 8, EvLocked = 16, EvBlurt = 32, EvHint = 64, EvLineLost = 128 };

	struct FSource { FString Name; FVector2D At = FVector2D::ZeroVector; bool bMajor = false; int32 Run = -1; };
	// One antenna as the session tells it: on remote or not, where it points (az, el) and where it stands (metres east, north).
	struct FDish { bool bOn = true; FVector2D Point = FVector2D::ZeroVector; FVector2D Ground = FVector2D::ZeroVector; };
	// One bearing as the chart draws it: a point it passes through, its direction, and how far it stands off the crossing.
	struct FLine { bool bOn = false, bSolid = false; int32 Arm = 0; FVector2D Through = FVector2D::ZeroVector, Dir = FVector2D(1, 0); double Off = 0; };

	// The catalogue: the four sources the cards observe (FS47Observing::Runs, at their catalogued place) and a fixed field of
	// minor entries named by their place ("SC 104+41"). The same chart every night.
	static const TArray<FSource>& Catalogue();
	static FVector2D CataloguePlace(int32 Run);
	static FVector2D ClampToChart(FVector2D P, double Margin = 0);
	static int32 ArmOf(int32 Dish) { return FMath::Clamp(Dish * ArmCount / DishCount, 0, ArmCount - 1); }
	// Hour angle (hours, west positive) and declination (degrees) of a direction, at the station's latitude.
	static void HourAngleDec(FVector2D AzEl, double& HaHours, double& DecDeg);
	// Geometric delay (nanoseconds) of a ground offset (metres east, north) toward a direction.
	static double DelayNs(FVector2D Ground, FVector2D AzEl);

	// A job's search starts: the card's own place, which catalogued source it is (-1: none), and whether it is the unusual kind.
	void Begin(uint32 InSeed, FVector2D InCard, int32 InRun, bool bInOdd);
	// Every frame: the antennas, whether the array is on target and settled, and whether it has been pointed at all.
	void SetArray(const TArray<FDish>& InDishes, bool bInSteady, bool bInBearings);
	// The unusual kind can still change while nothing is locked (the session asks every frame); kept from the lock on.
	void SetOdd(bool bInOdd) { if (Phase == EPhase::NoBearings || Phase == EPhase::Search) bOdd = bInOdd; }
	void PointAt(FVector2D AzEl) { Pointer = ClampToChart(AzEl); bPointer = true; }
	void ClearPointer() { bPointer = false; }
	void Steer(FVector2D Unit) { SteerDir = Unit.GetClampedToMaxSize(1.); }
	void Nudge(FVector2D Degrees);
	// One step; returns EEvent bits. Dt is clamped to a tenth of a second.
	uint32 Step(double Dt);
	// The way through without a hand on it (bots, tests, a script): the fix at once, if the array is steady. False otherwise.
	bool Solve();
	// The fix as already held (a job acquired before, or past recording).
	void ForceLocked();

	EPhase State() const { return Phase; }
	bool Locked() const { return Phase == EPhase::Locked; }
	bool Odd() const { return bOdd; }
	bool Steady() const { return bSteady; }
	bool HasBearings() const { return bBearings && OnCount() > 0; }
	FVector2D ReticleAt() const { return Reticle; }
	FVector2D PointerAt() const { return Pointer; }
	bool HasPointer() const { return bPointer; }
	FVector2D CardAt() const { return Card; }
	int32 CardRun() const { return Run; }
	// Where the bearings close.
	FVector2D Cross() const;
	double Distance() const { return FVector2D::Distance(Reticle, Cross()); }
	double Closeness() const { return 1. - FMath::Clamp(Distance() / FarRange, 0., 1.); }
	double Residual() const { return bOdd ? OddResidual : 0.; }
	// How far the bands stand off the crossing now (degrees): FarSpread far away, Residual on it.
	double Spread() const;
	static double SpreadAt(double Distance, double Residual);
	FLine Line(int32 Dish) const;
	// The triangle the three arms' bands make round the crossing (each band's middle line); false with an arm wholly off remote.
	bool Triangle(FVector2D Corner[ArmCount]) const;
	// How far the bearings stand off the crossing just now, on average (degrees).
	double MeanOff() const;
	// A segment cut to the chart's window; false when none of it is on the chart.
	static bool ClipSegment(FVector2D& A, FVector2D& B);
	bool ArmSolid(int32 Arm) const { return Arm >= 0 && Arm < ArmCount && bArm[Arm]; }
	int32 OnCount() const;
	int32 InCount() const;                    // bearings within ClosedWithin of the crossing
	double SearchSeconds() const { return SearchT; }
	bool HintShown() const { return bHint && Phase == EPhase::Search; }
	double AssistLevel() const { return FMath::Clamp((SearchT - AssistAfter) / (AssistFull - AssistAfter), 0., 1.); }
	double HoldProgress() const { return FMath::Clamp(Hold / HoldSeconds, 0., 1.); }
	double LockProgress() const { return Phase == EPhase::Locked ? 1. : Phase == EPhase::Locking ? FMath::Clamp(LockT / LockSeconds, 0., 1.) : 0.; }
	int32 RingsClosed() const { return Phase == EPhase::Locked ? ArmCount : Rings; }
	int32 LastArm() const { return EventArm; }  // the arm EvLine or EvLineLost was about
	double Clock() const { return Time; }
	// The readouts: what they come to once locked (the chart counts up to them over the lock sequence).
	double SignalToNoise() const;              // dB
	double ArmDelayNs(int32 Arm) const;        // the arm's longest baseline toward the crossing
	double CatalogueOffset() const;            // degrees from the crossing to the nearest catalogued major source
	const FSource* NearestCatalogued(FVector2D At, bool bMajorOnly) const;

	static bool SelfTest(TArray<FString>& Lines);

private:
	uint32 Seed = 47;
	FVector2D Card = FVector2D(90, 45), Reticle = FVector2D(90, 45), Pointer = FVector2D::ZeroVector, SteerDir = FVector2D::ZeroVector, Mean = FVector2D(90, 45), OddAt = FVector2D(90, 45), FixAt = FVector2D(90, 45);
	int32 Run = -1;
	bool bOdd = false, bSteady = false, bBearings = false, bPointer = false, bHint = false, bPlaced = false;
	bool bArm[ArmCount] = { false, false, false };
	TArray<FDish> Array;
	EPhase Phase = EPhase::NoBearings;
	double Time = 0, SearchT = 0, Hold = 0, LockT = 0, NextSweep = 0, MovedAt = -10;
	int32 Rings = 0, EventArm = -1;
	double Hash(int32 I, int32 Salt) const;
	void Place();
	uint32 UpdateArms();
};
