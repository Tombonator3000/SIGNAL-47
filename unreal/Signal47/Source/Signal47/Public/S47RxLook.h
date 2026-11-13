#pragma once
#include "CoreMinimal.h"

// RX-LOOK-20261113 (-> M3, M6; Tom 13 November: «Få receiver til grafisk å se mer ut som bilde nr. to. Masse FX.»): what the
// receiver PC's new frame shows, as plain C++ (no Slate, no world): the state banner's words, the line a frequency belongs to and
// its wavelength, the small scopes' pictures (waterfall, spectrum, I and Q) as functions of the receiver's own numbers, and the
// tube's effects as functions of time. SS47RxLook and SS47SkyChart draw them; FS47FreeformSession::RxLive fills the numbers in.
//
// Nothing here decides anything. Every picture is a function of what the session already knows, the same for the same numbers
// (a seed and a clock), and every effect stands still with Calm flashes chosen.
namespace S47RxLook
{
	// ---- the state banner ----
	enum class EState : uint8 { NoJob, NoPower, NotUp, Point, Blocked, Slewing, Settling, Searching, TakingFix, FixHeld, SourceLock, Anomaly, Recording, RecordingHeld, Reference, OnTape, Done };
	// The job's stage as FS47FreeformSession::EJobStage counts it (0 new, 1 pointed, 2 recording, 3 at the printer, 4 recorded, 5 and on: registered).
	struct FStateIn
	{
		bool bJob = false, bPower = true, bBlocked = false, bPointReady = false, bSettled = false;
		bool bNotUp = false;                     // the card's source is not up yet this evening (nothing can be pointed)
		bool bSky = false, bSkyLocking = false;  // the fix held on the sky chart; its lock sequence running
		bool bTuned = false;                     // the carrier meter at 100 %
		bool bAnomalyAhead = false;              // tuned and ready, the take would be an unusual one (the screens already say ANOMALY DETECTED)
		bool bPlainOnTape = false;               // an unusual take on tape that the reel deck has not heard yet (the screens still say it)
		bool bReference = false;                 // its 8-second reference is being recorded
		int32 Stage = 0;
		double Elapsed = 0, Seconds = 18, ReferenceAt = 0;
	};
	struct FState { EState Id = EState::NoJob; FString Text; bool bAmber = false; };
	SIGNAL47_API FState StateOf(const FStateIn& In);

	// ---- the line and its wavelength ----
	constexpr double HydrogenMhz = 1420.40575;
	inline double WavelengthCm(double Mhz) { return Mhz > 1. ? 29979.2458 / Mhz : 0.; }
	inline bool OnHydrogenLine(double Mhz) { return FMath::Abs(Mhz - HydrogenMhz) <= .2; }
	inline const TCHAR* LineName(double Mhz) { return OnHydrogenLine(Mhz) ? TEXT("H I") : TEXT("CONT"); }

	// ---- the tube's effects: all of them stand still with Calm flashes ----
	constexpr double SweepSeconds = 7.;   // the search sweep once round
	constexpr double BloomSeconds = .9;   // the picture's one soft pulse at the lock
	constexpr double PulseSeconds = 1.6;  // a ring going out from the fix
	constexpr double TypeRate = 38.;      // the banner's letters a second
	inline double Hash01(uint32 V) { V ^= V >> 16; V *= 0x7feb352du; V ^= V >> 15; V *= 0x846ca68bu; V ^= V >> 16; return double(V & 0xffffffu) / double(0x1000000); }
	// A star's slow shimmer, .72 to 1.
	inline float Twinkle(int32 Index, double T, bool bCalm)
	{
		if (bCalm || !FMath::IsFinite(T)) return 1.f;
		const double Rate = .35 + .5 * Hash01(uint32(Index) * 3u + 1u), Phase = Hash01(uint32(Index) * 3u + 2u);
		return float(.86 + .14 * FMath::Sin((T * Rate + Phase) * 2. * UE_DOUBLE_PI));
	}
	// The search sweep: turns of the whole circle, 0..1; below zero there is none.
	inline double Sweep(double T, bool bCalm) { return bCalm || !FMath::IsFinite(T) ? -1. : FMath::Fmod(FMath::Max(T, 0.) / SweepSeconds, 1.); }
	// The one soft pulse of the whole picture at the lock moment, 0..1 (up once and down once).
	inline float Bloom(double Since, bool bCalm)
	{
		if (bCalm || !(Since >= 0) || Since >= BloomSeconds) return 0.f;
		return float(FMath::Square(FMath::Sin(UE_DOUBLE_PI * Since / BloomSeconds)));
	}
	// The rings going out from the fix after the lock, one after the other. False: that ring is not on the glass.
	inline bool Pulse(double Since, int32 Ring, bool bCalm, float& Grow, float& Alpha)
	{
		const double At = Since - .25 * Ring;
		Grow = 0.f; Alpha = 0.f;
		if (bCalm || !(At >= 0) || At >= PulseSeconds) return false;
		const double U = At / PulseSeconds;
		Grow = float(1. - FMath::Square(1. - U)); Alpha = float(FMath::Square(1. - U));
		return true;
	}
	// A slow breath for a small mark, Low to 1.
	inline float Breath(double T, double Hz, bool bCalm, float Low = .72f)
	{
		if (bCalm || !FMath::IsFinite(T)) return 1.f;
		return Low + (1.f - Low) * float(.5 + .5 * FMath::Sin(T * Hz * 2. * UE_DOUBLE_PI));
	}
	// How many of a line's letters have been written this long after it changed.
	inline int32 Typed(int32 Len, double Since, bool bCalm) { return bCalm || !(Since >= 0) ? Len : FMath::Clamp(int32(Since * TypeRate) + 1, 0, Len); }
	// Now and then a band of the picture steps a unit sideways for a tenth of a second: 0 or 1 for a height on the glass.
	inline float Jitter(double T, float Y, bool bCalm)
	{
		if (bCalm || !FMath::IsFinite(T) || T < 0) return 0.f;
		constexpr double Slot = 4.7;
		const double Index = FMath::FloorToDouble(T / Slot);
		if (T - Index * Slot > .11 || Hash01(uint32(Index) * 2654435761u) > .55) return 0.f;
		const float Band = float(Hash01(uint32(Index) * 40503u + 7u)) * 420.f;
		return FMath::Abs(Y - Band) < 46.f ? 1.f : 0.f;
	}
	// The beam is a little wider where it is brighter.
	inline float Beam(float Width, float Bright) { return Width * (.8f + .45f * FMath::Clamp(Bright, 0.f, 1.f)); }

	// ---- the small scopes ----
	// The receiver as the scopes read it. Strength is what of the source reaches the tube: the carrier quality for the card, the
	// array on target, the fix on the sky.
	struct FScope
	{
		bool bPowered = false;
		double Tuned = 1420., Card = 1420., FTol = .02, Gain = 50., Bandwidth = 50.;
		double Strength = 0.;
		uint32 Seed = 47u;
	};
	// Half the scopes' window (MHz): the TUNE page's zoom window.
	inline double HalfSpan(double FTol) { return FMath::Max(.04, FTol * 5.); }
	// One row of the waterfall, as it was when it was written. The window is centred on the dial, so the card's line stands off by
	// what the dial is off; not locked, it wanders a little. bTick: the take has an extra tick in this row.
	struct FFallRow { float Line = -1.f, Strength = 0.f, Floor = 0.f; uint32 Index = 0u; bool bTick = false; };
	inline FFallRow FallRow(const FScope& In, uint32 Index, bool bTick)
	{
		FFallRow Row; Row.Index = Index; Row.bTick = bTick;
		if (!In.bPowered) return Row;
		const double Lock = FMath::Clamp(In.Strength, 0., 1.);
		Row.Line = float(.5 + (In.Card - In.Tuned) / (2. * HalfSpan(In.FTol)) + (1. - Lock) * .014 * (2. * Hash01(In.Seed ^ (Index * 0x9e3779b9u)) - 1.));
		Row.Strength = float(Lock);
		Row.Floor = float(.14 + .2 * FMath::Clamp(In.Bandwidth / 100., 0., 1.) + .1 * FMath::Clamp(In.Gain / 100., 0., 1.));
		return Row;
	}
	// The brightness of one cell of a row, 0..1.
	inline float FallCell(const FFallRow& Row, uint32 Seed, int32 Bin, int32 Bins)
	{
		if (Bins <= 0 || Row.Floor <= 0) return 0.f;
		const float Noise = float(Hash01(Seed ^ (Row.Index * 0x85ebca6bu) ^ (uint32(Bin) * 0xc2b2ae35u)));
		float V = Row.Floor * (.3f + .7f * Noise * Noise);
		if (Row.Line > -.2f && Row.Line < 1.2f)
		{
			const float Off = ((Bin + .5f) / Bins - Row.Line) * Bins;
			V += Row.Strength * .95f * FMath::Exp(-Off * Off / 1.1f);
			if (Row.bTick) V += FMath::Max(Row.Strength, .6f) * .85f * FMath::Exp(-Off * Off / 16.f);
		}
		return FMath::Clamp(V, 0.f, 1.f);
	}
	// The spectrum across the card's window (0..1 across, the card's line in the middle), 0..1 up: the TUNE page's zoom, small.
	inline float FftLevel(const FScope& In, float U, double T)
	{
		if (!In.bPowered) return 0.f;
		const double X = U * 240.;
		const double Noise = (FMath::Sin(X * 1.3 + T * 5.) * 2. + FMath::Sin(X * 4.1 + T * 9.) * 3. + FMath::Sin(X * 12.9898 + T * 13.) * 1.5) * (In.Bandwidth / 35. + .2) / 220.;
		const double Sigma = FMath::Max(.014, In.FTol / (2. * HalfSpan(In.FTol)) * .8);
		const double Peak = (8. + 130. * FMath::Clamp(In.Strength, 0., 1.) * (.35 + .65 * In.Gain / 100.)) / 200. * FMath::Exp(-FMath::Square((U - .5) / Sigma));
		return float(FMath::Clamp(.18 + Noise + Peak, .02, .98));
	}
	// Where the dial stands in that window (0..1; outside it: off the scope).
	inline float FftTuned(const FScope& In) { return float(.5 + (In.Tuned - In.Card) / (2. * HalfSpan(In.FTol))); }
	// The in-phase and quadrature traces at a place along the strip (0..1), -1..1: noise when nothing is locked, two clean sines a
	// quarter turn apart when it is. The beat is slow on the card and quicker off it.
	inline void Quadrature(const FScope& In, float U, double T, bool bCalm, float& OutI, float& OutQ)
	{
		OutI = OutQ = 0.f;
		if (!In.bPowered) return;
		const double Lock = FMath::Clamp(In.Strength, 0., 1.);
		const double Cycles = 7. + 10. * FMath::Clamp(FMath::Abs(In.Tuned - In.Card) / FMath::Max(In.FTol, 1e-3), 0., 4.);
		const double Turn = (Cycles * U - (bCalm ? 0. : T * 1.1)) * 2. * UE_DOUBLE_PI;
		const uint32 Key = In.Seed ^ (uint32(FMath::Clamp(U, 0.f, 1.f) * 4096.f) * 0x9e3779b9u) ^ (bCalm ? 0u : uint32(FMath::Max(T, 0.) * 14.) * 0x85ebca6bu);
		const double Rough = .07 + .5 * (1. - Lock);
		OutI = float(FMath::Clamp(.8 * Lock * FMath::Cos(Turn) + Rough * (2. * Hash01(Key) - 1.), -1., 1.));
		OutQ = float(FMath::Clamp(.8 * Lock * FMath::Sin(Turn) + Rough * (2. * Hash01(Key ^ 0x5bd1e995u) - 1.), -1., 1.));
	}

	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}

// What the receiver's frame shows this frame: every number the session's own (FS47FreeformSession::RxLive).
struct FS47RxLive
{
	// the header
	FString Clock;                 // the terminal's date and clock, as its header always had them
	FString Term;                  // "V4.2  TERM 3  OPR NIGHT"
	bool bPointing = false;        // the array answers: its mean pointing follows
	double El = 0., Az = 0.;
	// the banner
	S47RxLook::FState State;
	int32 Dishes = 0;              // antennas on remote, of 27
	double Rate = 0.;              // the measurement rate, 0..1
	FString RateWord;              // FULL, DEGRADED, WORN or BLOCKED, as the idle picture says it
	bool bJob = false;
	FString Source;                // the card's source
	double CardMhz = 0.;
	// the scopes
	S47RxLook::FScope Scope;
	bool bRecording = false;       // the take is being written now
	double Elapsed = 0., Seconds = 18.;
	TArray<double> Ticks;          // the take's extra ticks (seconds), empty for an ordinary take
	bool bCalm = false;
	double LstDeg = 0.;            // the station's sidereal time, from the game's clock
};

// One reading a frame for every part of the frame that asks.
class SIGNAL47_API FS47RxFeed
{
public:
	explicit FS47RxFeed(TFunction<FS47RxLive()> InRead) : Read(MoveTemp(InRead)) {}
	const FS47RxLive& Now();
private:
	TFunction<FS47RxLive()> Read;
	FS47RxLive Value;
	uint64 Frame = ~0ull;
};
