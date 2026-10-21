#pragma once

#include "CoreMinimal.h"

// Ported 1:1 from web/src/story/Signal.ts (itself ported from the Unity SignalProfile assets,
// SignalFeedback.Quality and the SignalConsole stages). Plain C++: no UObjects, no Slate.

struct SIGNAL47_API FS47Profile
{
	FString Stage;
	FString Action;
	FString Card;
	bool bCardIsTaped = false;
	double F = 0.0;
	double FTol = 0.0;
	double Gain[2] = { 0.0, 0.0 };
	double Bw[2] = { 0.0, 0.0 };
	double Az[2] = { 0.0, 0.0 };
	FString Note;
};

namespace S47Signal
{
	// The three stage profiles: CALIBRATION, INTERFERENCE, ANOMALY.
	SIGNAL47_API const TArray<FS47Profile>& Profiles();
	constexpr double FMin = 1419.5;
	constexpr double FMax = 1420.7;
}

class SIGNAL47_API FS47Rx
{
public:
	double Frequency = 1419.62;
	double Gain = 27;
	double Bandwidth = 82;
	double Azimuth = 18;
	int32 Stage = 0;          // index into Profiles(); 3 = pattern locked
	bool bSolved = false;     // direction solve done
	FString Status = TEXT("CALIBRATION REQUIRED");
	bool bPowered = false;

	// Profiles()[min(Stage, 2)].
	const FS47Profile& Profile() const;
	// 0..1, how close the dials are to the profile window (Gaussian falloff outside it).
	double Quality() const;
	double Quality(const FS47Profile& P) const;
	// True when every dial is inside the profile window.
	bool Pass() const;
	bool Pass(const FS47Profile& P) const;
	// 4 pulses, pause, 7 pulses. Returns a 0..1 envelope; 1 before stage 3.
	double Pulse(double T) const;
	void Advance();
	// Back to the initial values.
	void Reset();
};

// The spectrum trace as geometry, for a Slate painter: the trace polyline, the tuned marker x,
// the bandwidth band, the grid lines. Deterministic noise: `Rand` is a value in [0,1) the caller
// supplies per sample via a small generator so a test can be repeatable.
struct SIGNAL47_API FS47SpectrumGeometry
{
	TArray<FVector2f> Trace;
	float TunedX = 0;
	float BandX0 = 0;
	float BandX1 = 0;
	TArray<float> GridX;
	TArray<float> GridY;
	bool bPowered = false;
};

namespace S47Spectrum
{
	// Fills Out from the receiver state. When !Rx.bPowered only the grid is filled and
	// bPowered is false. The math is drawSpectrum() from Signal.ts, point for point.
	SIGNAL47_API void Build(const FS47Rx& Rx, double T, bool bResidualVisible, float W, float H,
		TFunctionRef<double()> Rand, FS47SpectrumGeometry& Out);
}

// Checks the port against values computed by the TypeScript original. Appends one line per
// check to Out ("PASS ..." or "FAIL ..."); returns true when every check passed.
SIGNAL47_API bool S47RxSelfTest(TArray<FString>& Out);
