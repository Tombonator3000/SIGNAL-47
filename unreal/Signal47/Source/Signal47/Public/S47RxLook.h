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

	// ---- the glass's letters (round 2, 13 November: the first look in the engine) ----
	// Measured on the game's frame at 1600 x 900: the receiver's glass is 895 px across and 702 px high for 1099 by 824 units,
	// 0.81 px a unit across and 0.85 up. The next step's line (Mono 22) stands 19 px high there and reads; the frame's small
	// words (Mono 14 to 18) did not. Nothing the receiver's frame draws is smaller than the next step's line: fewer words
	// rather than smaller ones.
	constexpr int32 TextFloor = 22;
	// How wide a letter of the terminal's font stands on the glass. The font is four tenths of its size wide; the glass's
	// picture is drawn 1280 px across for 1200 units, and there a letter's advance is a whole number of pixels (so Mono 20 and
	// Mono 22 are equally wide, and Mono 18 is wider than four tenths of 18).
	constexpr float GlassScale = 1280.f / 1200.f;
	inline float GlyphWide(int32 Size) { return FMath::Max(1.f, FMath::RoundToFloat(float(Size) * .4f * GlassScale)) / GlassScale; }
	inline float TextWide(int32 Chars, int32 Size) { return float(Chars) * GlyphWide(Size); }

	// ---- the glass's numbers: none of them shows a 47 (the project's rule for every number the player reads) ----
	// True when a readout's digits, read in order with everything else left out, have a 4 followed by a 7: 47, 4.7, 14.7, 147,
	// 0.47, 14h47m. One number at a time: 54 beside 70 is not one.
	inline bool Shows47(const FString& S)
	{
		TCHAR Last = 0;
		for (int32 I = 0; I < S.Len(); ++I)
		{
			const TCHAR C = S[I];
			if (C < TEXT('0') || C > TEXT('9')) continue;
			if (Last == TEXT('4') && C == TEXT('7')) return true;
			Last = C;
		}
		return false;
	}
	// The whole number nearest Steps whose digits show no 47, by its digits: at the first 4 and 7 that stand together the 7
	// becomes an 8 with noughts after it, or a 6 with nines after it, whichever is nearer. A readout that would have shown a 47
	// stands one last-digit step off (more only inside a whole run of them: 47.0 to 47.9 become 46.9 or 48.0).
	inline int32 Near47(int32 Steps)
	{
		const int64 Whole = FMath::Abs(int64(Steps));
		int32 Digit[20];
		int32 Count = 0;
		for (int64 Left = Whole; Count == 0 || Left > 0; Left /= 10) Digit[Count++] = int32(Left % 10);
		for (int32 I = Count - 2; I >= 0; --I)
		{
			if (Digit[I + 1] != 4 || Digit[I] != 7) continue;
			int64 Unit = 1;
			for (int32 K = 0; K < I; ++K) Unit *= 10;
			const int64 Flat = Whole - Whole % Unit, Above = Flat + Unit, Below = Flat - 1;
			const int64 Nearest = Above - Whole <= Whole - Below ? Above : Below;
			return int32(Steps < 0 ? -Nearest : Nearest);
		}
		return Steps;
	}
	// A number as the glass shows it: so many decimals, a sign always or only below zero, filled on the left to a width (with
	// zeros after the sign when bZeros). Every number the receiver's frame draws is written here.
	inline FString Num(double Value, int32 Decimals = 0, int32 Width = 0, bool bSign = false, bool bZeros = false)
	{
		Decimals = FMath::Clamp(Decimals, 0, 6);
		double Scale = 1.;
		for (int32 I = 0; I < Decimals; ++I) Scale *= 10.;
		const int32 Steps = Near47(FMath::IsFinite(Value) ? int32(FMath::Clamp(FMath::RoundToDouble(Value * Scale), -2.0e9, 2.0e9)) : 0);
		FString Digits = FString::FromInt(FMath::Abs(Steps));
		while (Digits.Len() <= Decimals) Digits.InsertAt(0, TEXT('0'));
		if (Decimals > 0) Digits.InsertAt(Digits.Len() - Decimals, TEXT('.'));
		const int32 SignLen = Steps < 0 || bSign ? 1 : 0;
		FString Out;
		for (int32 I = SignLen + Digits.Len(); !bZeros && I < Width; ++I) Out.AppendChar(TEXT(' '));
		if (SignLen) Out.AppendChar(Steps < 0 ? TEXT('-') : TEXT('+'));
		for (int32 I = SignLen + Digits.Len(); bZeros && I < Width; ++I) Out.AppendChar(TEXT('0'));
		return Out + Digits;
	}
	// RX-LOOK gjennomgang 14 Nov: the take's seconds as every readout of the receiver counts them ("7 / 18 s"): the whole seconds
	// written (cut down, never rounded up, as the banner counts them), of the take's length.
	inline FString TakeCount(double Elapsed, double Seconds)
	{
		return Num(FMath::FloorToDouble(FMath::Max(0., FMath::IsFinite(Elapsed) ? Elapsed : 0.))) + TEXT(" / ") + Num(FMath::RoundToDouble(Seconds)) + TEXT(" s");
	}
	// The header's date and clock ("28-APR-86 23:47") as the glass shows it: the clock by the project's rule for a clock in player
	// text (S47Script::PlayerClock: never a minute or digits that read 47, nor 08:15 or 20:15; a minute on). The date is as given
	// (a day or a year of the game's calendar cannot read 47). Anything that does not end in HH:MM is given back as it is.
	SIGNAL47_API FString ClockLine(const FString& DateAndClock);
	// For a readout written in sixties (hours, minutes and seconds; degrees and minutes): the words of the count nearest Exact
	// that show no 47, a step up, a step down, two up, two down and on. Words writes a count as the glass shows it.
	template <typename TWords>
	inline FString Clean47(int32 Exact, TWords&& Words, int32 Reach = 400)
	{
		FString Shown = Words(Exact);
		for (int32 Away = 1; Away <= Reach && Shows47(Shown); ++Away) Shown = Words(Exact + (Away % 2 ? (Away + 1) / 2 : -(Away / 2)));
		return Shown;
	}

	// ---- the words on the glass (RX-LOOK gjennomgang 14 Nov) ----
	// The array's three arms by number, as the sky chart writes them by their fans and the tracking panel's DELAY rows (the game
	// has no other names for them; the letters A to C had a lone capital C, which the Freeform text rule bans).
	inline const TCHAR* ArmLabel(int32 Arm) { static const TCHAR* const Label[3] = { TEXT("1"), TEXT("2"), TEXT("3") }; return Label[FMath::Clamp(Arm, 0, 2)]; }
	// Empty when a line the receiver shows keeps to Freeform's text rule; otherwise what it touches: a number that reads 47 (the
	// point taken out: S47Script::SaysFortySeven), what S47Script::LintText finds (a lone capital C, 1947, Tomas, 8.15, the third
	// silhouette, how anyone was taken), and the words it lets by alone: «taken», «the cable».
	SIGNAL47_API FString TextFault(const FString& Shown);

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
		constexpr double Slot = 5.3;
		const double Index = FMath::FloorToDouble(T / Slot);
		if (T - Index * Slot > .11 || Hash01(uint32(Index) * 2654435761u) > .55) return 0.f;
		const float Band = float(Hash01(uint32(Index) * 40503u + 7u)) * 420.f;
		return FMath::Abs(Y - Band) < 46.f ? 1.f : 0.f;
	}
	// RX-LOOK gjennomgang 14 Nov: what is on the air makes the whole picture lose its hold and jump sideways (S47FreeformTerminal's
	// frames). Never with Calm flashes, and never more than three times a second (the project's flash rule): a jump can only
	// start at the beginning of a slot of TearSlot seconds and lasts a thirteenth of one, so two jumps stand at least a slot
	// apart. Jam is the interference 0..1 (under .15: none). Returns the sideways shift in units, 0 while there is none.
	constexpr double TearSlot = .35;
	inline float TearShift(double T, float Jam, bool bCalm)
	{
		if (bCalm || !FMath::IsFinite(T) || T < 0 || !(Jam >= .15f)) return 0.f;
		const double Slot = FMath::FloorToDouble(T / TearSlot);
		if (T - Slot * TearSlot >= 1. / 13.) return 0.f;
		const uint32 Key = uint32(int64(Slot) & 0xffffffff) * 2654435761u;
		if (Hash01(Key ^ 0x3c6ef372u) >= FMath::Min(1.f, FMath::Min(Jam, 1.f) * .4f * 13.f * float(TearSlot))) return 0.f;
		const float Shift = (float(Hash01(Key ^ 0xa54ff53au)) - .5f) * 36.f * FMath::Min(Jam, 1.f);
		return FMath::Abs(Shift) < .5f ? (Shift < 0 ? -.5f : .5f) : Shift;
	}
	// The same rule for the frame going dark for a moment on a bad beat (S47FreeformTerminal's glass): its veil's alpha, 0 while none.
	inline float TearDark(double T, float Jam, bool bCalm)
	{
		if (bCalm || !FMath::IsFinite(T) || T < 0 || !(Jam > .03f)) return 0.f;
		const double Slot = FMath::FloorToDouble(T / TearSlot);
		if (T - Slot * TearSlot >= 1. / 17.) return 0.f;
		const uint32 Key = uint32(int64(Slot) & 0xffffffff) * 40503u + 17u;
		const float J = FMath::Min(Jam, 1.f);
		return Hash01(Key) < FMath::Min(1.f, J * .35f * 17.f * float(TearSlot)) ? .25f + .4f * J : 0.f;
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
	// Round 2 (the first look in the engine: big square cells and one flat column): a row is drawn as it is on a real
	// waterfall, thin, its grain lying along it. The floor of a row is so many runs of even brightness, each a few hundredths
	// of the window long; Out is filled from the left edge to the right with each run's end (0..1) and level (0..1). Returns
	// how many.
	constexpr int32 FallRunsMost = 30;
	struct FFallRun { float End = 1.f, Level = 0.f; };
	inline int32 FallRuns(const FFallRow& Row, uint32 Seed, FFallRun* Out)
	{
		if (Row.Floor <= 0 || !Out) return 0;
		int32 Count = 0;
		float At = 0.f;
		while (At < 1.f && Count < FallRunsMost)
		{
			const uint32 Key = Seed ^ (Row.Index * 0x85ebca6bu) ^ (uint32(Count) * 0xc2b2ae35u);
			const float Noise = float(Hash01(Key));
			At += .024f + .068f * float(Hash01(Key ^ 0x27d4eb2fu));
			Out[Count].End = FMath::Min(At, 1.f);
			Out[Count].Level = FMath::Clamp(Row.Floor * (.3f + .7f * Noise * Noise), 0.f, 1.f);
			++Count;
		}
		Out[Count - 1].End = 1.f;
		return Count;
	}
	// The line in a row: a bright core and two wider, fainter steps about its place, so that it stands as a soft line and not
	// as a column. Step 0 is the core. Half its width as a share of the window, and its level 0..1 (a little different from
	// row to row, as the source's strength is). False: nothing of the line in this row.
	constexpr int32 FallLineSteps = 3;
	inline bool FallLine(const FFallRow& Row, uint32 Seed, int32 Step, float& HalfWide, float& Level)
	{
		HalfWide = 0.f; Level = 0.f;
		if (Step < 0 || Step >= FallLineSteps || Row.Floor <= 0 || Row.Strength < .02f || Row.Line < -.1f || Row.Line > 1.1f) return false;
		static constexpr float Wide[FallLineSteps] = { .008f, .022f, .05f }, Share[FallLineSteps] = { 1.f, .4f, .15f };
		HalfWide = Wide[Step];
		Level = FMath::Clamp(Row.Strength * Share[Step] * (.8f + .2f * float(Hash01(Seed ^ (Row.Index * 0x9e3779b9u) ^ 0x51ed27u))), 0.f, 1.f);
		return true;
	}
	// A tick of the take in its row: a short wide dash about the line.
	inline bool FallTick(const FFallRow& Row, float& HalfWide, float& Level)
	{
		HalfWide = 0.f; Level = 0.f;
		if (!Row.bTick || Row.Floor <= 0 || Row.Line < -.1f || Row.Line > 1.1f) return false;
		HalfWide = .11f; Level = FMath::Max(Row.Strength, .6f) * .85f;
		return true;
	}
	// The spectrum across the card's window (0..1 across, the card's line in the middle), 0..1 up: the TUNE page's zoom, small.
	// RX-LOOK gjennomgang 14 Nov: with Calm flashes the trace stands (its noise at one moment), as every other effect does.
	inline float FftLevel(const FScope& In, float U, double Now, bool bCalm)
	{
		if (!In.bPowered) return 0.f;
		const double T = bCalm || !FMath::IsFinite(Now) ? 0. : Now;
		const double X = U * 240.;
		const double Noise = (FMath::Sin(X * 1.3 + T * 5.) * 2. + FMath::Sin(X * 4.1 + T * 9.) * 3. + FMath::Sin(X * 12.9898 + T * 13.) * 1.5) * (In.Bandwidth / 35. + .2) / 220.;
		const double Sigma = FMath::Max(.014, In.FTol / (2. * HalfSpan(In.FTol)) * .8);
		const double Peak = (8. + 130. * FMath::Clamp(In.Strength, 0., 1.) * (.35 + .65 * In.Gain / 100.)) / 200. * FMath::Exp(-FMath::Square((U - .5) / Sigma));
		return float(FMath::Clamp(.18 + Noise + Peak, .02, .98));
	}
	// Where the dial stands in that window (0..1; outside it: off the scope).
	inline float FftTuned(const FScope& In) { return float(.5 + (In.Tuned - In.Card) / (2. * HalfSpan(In.FTol))); }
	// The in-phase and quadrature traces at a place along the strip (0..1), -1..1: noise when nothing is locked, two sines a
	// quarter turn apart when it is. The beat is slow on the card and quicker off it. Round 2: a locked carrier is not a drawn
	// sine: a little of the receiver's noise rides on it (RideShare of full scale: three slow waves that do not divide the
	// carrier's, each with its own phase from the job's seed, and the fine grain). The same for the same clock; with Calm
	// flashes the whole trace stands, noise and all.
	constexpr double RideShare = .13;
	inline void Quadrature(const FScope& In, float U, double T, bool bCalm, float& OutI, float& OutQ)
	{
		OutI = OutQ = 0.f;
		if (!In.bPowered) return;
		const double Lock = FMath::Clamp(In.Strength, 0., 1.);
		const double Cycles = 7. + 10. * FMath::Clamp(FMath::Abs(In.Tuned - In.Card) / FMath::Max(In.FTol, 1e-3), 0., 4.);
		const double Clock = bCalm || !FMath::IsFinite(T) ? 0. : T;
		const double Along = double(U) * 2. * UE_DOUBLE_PI, Turn = Cycles * Along - Clock * 1.1 * 2. * UE_DOUBLE_PI;
		const uint32 Key = In.Seed ^ (uint32(FMath::Clamp(U, 0.f, 1.f) * 4096.f) * 0x9e3779b9u) ^ (bCalm ? 0u : uint32(FMath::Max(Clock, 0.) * 14.) * 0x85ebca6bu);
		const double Rough = .07 + .5 * (1. - Lock);
		const double PhaseA = 2. * UE_DOUBLE_PI * Hash01(In.Seed ^ 0x1b873593u), PhaseB = 2. * UE_DOUBLE_PI * Hash01(In.Seed ^ 0xcc9e2d51u), PhaseC = 2. * UE_DOUBLE_PI * Hash01(In.Seed ^ 0xe6546b64u);
		const double RideI = .5 * FMath::Sin(Along * 23.3 + PhaseA + Clock * 2.3) + .3 * FMath::Sin(Along * 41.7 + PhaseB - Clock * 3.1) + .2 * FMath::Sin(Along * 67.1 + PhaseC + Clock * 1.7);
		const double RideQ = .5 * FMath::Sin(Along * 23.3 + PhaseB + Clock * 2.3) + .3 * FMath::Sin(Along * 41.7 + PhaseC - Clock * 3.1) + .2 * FMath::Sin(Along * 67.1 + PhaseA + Clock * 1.7);
		OutI = float(FMath::Clamp(.8 * Lock * FMath::Cos(Turn) + RideShare * Lock * RideI + Rough * (2. * Hash01(Key) - 1.), -1., 1.));
		OutQ = float(FMath::Clamp(.8 * Lock * FMath::Sin(Turn) + RideShare * Lock * RideQ + Rough * (2. * Hash01(Key ^ 0x5bd1e995u) - 1.), -1., 1.));
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
