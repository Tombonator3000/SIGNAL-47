#include "S47HudGauges.h"
#include <limits>

namespace
{
	// a steady little pseudo-random number from 0 to 1 for a whole number (no state, the same every time)
	double Hash01(int32 N)
	{
		uint32 X = uint32(N) * 2654435761u;
		X ^= X >> 15; X *= 2246822519u; X ^= X >> 13;
		return double(X & 0xffffu) / 65535.0;
	}
}

int32 S47HudGauges::Strokes(double Level)
{
	if (!FMath::IsFinite(Level) || Level <= 0) return 0;
	return FMath::Clamp(FMath::CeilToInt(Level * BatteryStrokes - 1e-9), 1, BatteryStrokes);
}

bool S47HudGauges::BatteryShown(bool bDrains, bool bHave, bool bOn, double Level, double Low)
{
	return bDrains && bHave && (bOn || Level < Low);
}

FString S47HudGauges::CbLine() { return TEXT("CB 19  27.185"); }
FString S47HudGauges::WalkieLine(int32 Channel, int32 Mode)
{
	const int32 Ch = FMath::Clamp(Channel, 1, 3);
	const FString Line = FString::Printf(TEXT("WALKIE %d  %.3f"), Ch, 49.830 + 0.015 * (Ch - 1));
	return Mode == 1 ? Line + TEXT("  TX") : Mode == 2 ? Line + TEXT("  CALL") : Line;
}

FLinearColor S47HudGauges::WalkieTint() { return FLinearColor(0.58f, 0.86f, 0.64f, 1.f); }

double S47HudGauges::CbStrength(double T)
{
	return FMath::Clamp(0.82 + 0.1 * FMath::Sin(T * 1.7) + 0.05 * FMath::Sin(T * 4.3), 0.0, 1.0);
}

double S47HudGauges::Wave(double X, double Phase, double Strength)
{
	const double K = FMath::Clamp(Strength, 0.0, 1.0);
	const double Ends = FMath::Sin(UE_DOUBLE_PI * FMath::Clamp(X, 0.0, 1.0));
	const double Clean = FMath::Sin(X * UE_DOUBLE_PI * 2 * 2.5 - Phase);
	// the weaker the signal, the more the line is torn: a different jitter every tenth of the phase
	const double Torn = (Hash01(FMath::FloorToInt(X * 18) + 131 * FMath::FloorToInt(Phase * 2.5)) - 0.5) * 2;
	return FMath::Clamp((Clean * (0.25 + 0.75 * K) + Torn * (1 - K) * 0.6) * Ends * (0.2 + 0.8 * K), -1.0, 1.0);
}

bool S47HudGauges::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s hud gauges: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bCond; };
	Check(Strokes(1) == 5 && Strokes(0.81) == 5 && Strokes(0.8) == 4 && Strokes(0.21) == 2 && Strokes(0.2) == 1 && Strokes(0.0001) == 1 && Strokes(0) == 0,
		TEXT("five strokes: full shows five, anything left shows one, empty none"));
	Check(Strokes(-1) == 0 && Strokes(2) == 5 && Strokes(std::numeric_limits<double>::quiet_NaN()) == 0, TEXT("a level out of range or not a number still gives a stroke count in 0 to 5"));
	bool bSteps = true; int32 Last = 0;
	for (int32 I = 0; I <= 100; ++I) { const int32 N = Strokes(I / 100.0); bSteps &= N >= Last; Last = N; }
	Check(bSteps, TEXT("the strokes only ever grow with the level"));
	Check(BatteryShown(true, true, true, 1.0, 0.2) && BatteryShown(true, true, false, 0.1, 0.2) && !BatteryShown(true, true, false, 0.8, 0.2),
		TEXT("shown while the lamp is on, or when it is low; hidden when it is off and well charged"));
	Check(!BatteryShown(false, true, true, 0.1, 0.2) && !BatteryShown(true, false, true, 0.1, 0.2), TEXT("not shown in the Story, nor before the lamp is taken"));
	Check(CbLine().Contains(TEXT("19")) && WalkieLine().Contains(TEXT("49.830")) && !CbLine().IsEmpty(), TEXT("the radio line names the channel and the frequency"));
	Check(WalkieLine(2).Contains(TEXT("WALKIE 2")) && WalkieLine(2).Contains(TEXT("49.845")) && WalkieLine(3).Contains(TEXT("49.860")) && WalkieLine(7) == WalkieLine(3)
		&& WalkieLine(1, 1).EndsWith(TEXT("TX")) && WalkieLine(1, 2).EndsWith(TEXT("CALL")) && !WalkieLine(1).Contains(TEXT("CB")) && !CbLine().Contains(TEXT("WALKIE")),
		TEXT("the walkie's line says its channel and whether it is keyed or calling, and never reads as the CB's"));
	Check(WalkieTint().G > WalkieTint().R && WalkieTint().G > WalkieTint().B, TEXT("the walkie has a tint of its own, apart from the CB's amber"));
	bool bCb = true;
	for (int32 I = 0; I < 200; ++I) { const double S = CbStrength(I * 0.37); bCb &= S > 0.6 && S <= 1.0; }
	Check(bCb, TEXT("the CB's signal during a call stays strong and steady"));
	bool bEnds = true, bBounded = true;
	for (double K : { 0.0, 0.3, 1.0 })
		for (int32 P = 0; P < 40; ++P)
		{
			bEnds &= FMath::Abs(Wave(0, P * 0.3, K)) < 1e-9 && FMath::Abs(Wave(1, P * 0.3, K)) < 1e-9;
			for (int32 I = 0; I <= 24; ++I) { const double V = Wave(I / 24.0, P * 0.3, K); bBounded &= V >= -1.0 && V <= 1.0; }
		}
	Check(bEnds && bBounded, TEXT("the wave starts and ends on the line and stays within it"));
	auto Swing = [](double K) { double M = 0; for (int32 P = 0; P < 40; ++P) for (int32 I = 0; I <= 48; ++I) M = FMath::Max(M, FMath::Abs(Wave(I / 48.0, P * 0.3, K))); return M; };
	Check(Swing(1.0) > Swing(0.5) && Swing(0.5) > Swing(0.1) && Swing(1.0) > 0.9, TEXT("a stronger signal swings wider"));
	return bOk;
}
