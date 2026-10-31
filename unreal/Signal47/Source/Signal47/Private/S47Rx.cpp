#include "S47Rx.h"

// ---------------------------------------------------------------------------------------------
// Profiles (web/src/story/Signal.ts PROFILES, every number and string as in the original)

namespace S47Signal
{
	const TArray<FS47Profile>& Profiles()
	{
		static const TArray<FS47Profile> List = []()
		{
			TArray<FS47Profile> P;
			P.SetNum(3);

			P[0].Stage = TEXT("CALIBRATION");
			P[0].Action = TEXT("LOG CALIBRATION");
			P[0].bCardIsTaped = true;
			P[0].Card = TEXT("TAPED REF CARD  /  CAL 1419.900 MHz  /  AZ 042  /  GAIN 55 +-10  /  BW 48 +-14");
			P[0].F = 1419.9; P[0].FTol = 0.025;
			P[0].Gain[0] = 45; P[0].Gain[1] = 65;
			P[0].Bw[0] = 34; P[0].Bw[1] = 62;
			P[0].Az[0] = 35; P[0].Az[1] = 49;
			P[0].Note = TEXT("Receiver calibrated on the 1419.900 reference.");

			P[1].Stage = TEXT("INTERFERENCE");
			P[1].Action = TEXT("NOTCH INTERFERENCE");
			P[1].bCardIsTaped = false;
			P[1].Card = TEXT("LOCAL SPIKE  /  centre carrier near 1420.110 MHz. Narrow BW below 22 kHz while keeping nominal gain.");
			P[1].F = 1420.11; P[1].FTol = 0.03;
			P[1].Gain[0] = 40; P[1].Gain[1] = 72;
			P[1].Bw[0] = 4; P[1].Bw[1] = 22;
			P[1].Az[0] = 0; P[1].Az[1] = 180;
			P[1].Note = TEXT("Local carrier at 1420.110 notched. Highway relay again.");

			P[2].Stage = TEXT("ANOMALY");
			P[2].Action = TEXT("ISOLATE PATTERN");
			P[2].bCardIsTaped = false;
			P[2].Card = TEXT("UNLOGGED RESIDUAL  /  weak peak near 1420.4 MHz. Raise gain, narrow bandwidth, then search array azimuth by hand.");
			P[2].F = 1420.405; P[2].FTol = 0.008;
			P[2].Gain[0] = 78; P[2].Gain[1] = 100;
			P[2].Bw[0] = 4; P[2].Bw[1] = 14;
			P[2].Az[0] = 79; P[2].Az[1] = 87;
			P[2].Note = TEXT("Narrowband carrier isolated under the hydrogen line.");
			return P;
		}();
		return List;
	}
}

// ---------------------------------------------------------------------------------------------
// FS47Rx

const FS47Profile& FS47Rx::Profile() const
{
	const TArray<FS47Profile>& P = S47Signal::Profiles();
	return P[FMath::Min(Stage, P.Num() - 1)];
}

double FS47Rx::Quality() const
{
	return Quality(Profile());
}

double FS47Rx::Quality(const FS47Profile& P) const
{
	// rd(v, r) = max(r[0] - v, 0, v - r[1]): distance outside the window, 0 inside.
	auto Rd = [](double V, const double R[2]) { return FMath::Max3(R[0] - V, 0.0, V - R[1]); };
	const double F = FMath::Max(0.0, FMath::Abs(Frequency - P.F) - P.FTol) / FMath::Max(P.FTol * 4, 0.025);
	const double G = Rd(Gain, P.Gain) / 22;
	const double B = Rd(Bandwidth, P.Bw) / 25;
	const double A = Rd(Azimuth, P.Az) / 25;
	return FMath::Exp(-(F * F + G * G + B * B + A * A));
}

bool FS47Rx::Pass() const
{
	return Pass(Profile());
}

bool FS47Rx::Pass(const FS47Profile& P) const
{
	// Unlike the original strict windows, accept the lock promised by the visible meter.
	return S47Signal::CarrierPercent(Quality(P)) == 100;
}

double FS47Rx::Pulse(double T) const
{
	if (Stage < 3) return 1;
	const double C = FMath::Fmod(T, 5.2);
	const int32 Count = C < 1.6 ? 4 : 7;
	const double Local = C < 1.6 ? C : C - 2.1;
	if (Local < 0) return 0.06;
	const double Slot = 0.22;
	return (Local < Count * Slot && FMath::Fmod(Local, Slot) < 0.11) ? 1 : 0.06;
}

void FS47Rx::Advance()
{
	if (Stage == 0)
	{
		Stage = 1; Status = TEXT("LOCAL CARRIER DRIFT");
		Frequency = 1419.98; Gain = 55; Bandwidth = 58; Azimuth = 42;
	}
	else if (Stage == 1)
	{
		Stage = 2; Status = TEXT("SURVEY SWEEP ACTIVE  /  H-LINE  /  UNTIL 06:00");
	}
	else if (Stage == 2)
	{
		Stage = 3; Status = TEXT("PATTERN LOCK  /  PULSE GROUP 4 / 7");
		Frequency = 1420.405; Gain = 82; Bandwidth = 12; Azimuth = 83;
	}
}

void FS47Rx::Reset()
{
	*this = FS47Rx();
}

// ---------------------------------------------------------------------------------------------
// Spectrum geometry (drawSpectrum in Signal.ts, without the canvas)

namespace S47Spectrum
{
	void Build(const FS47Rx& Rx, double T, bool bResidualVisible, float W, float H,
		TFunctionRef<double()> Rand, FS47SpectrumGeometry& Out)
	{
		Out.Trace.Reset();
		Out.GridX.Reset();
		Out.GridY.Reset();
		Out.TunedX = Out.BandX0 = Out.BandX1 = 0;
		Out.bPowered = false;

		const double w = W, h = H;
		// Same accumulating loops as the original so the line count matches.
		for (double x = 0; x <= w; x += w / 8) Out.GridX.Add((float)x);
		for (double y = h * 0.15; y < h; y += h * 0.17) Out.GridY.Add((float)y);
		if (!Rx.bPowered) return;
		Out.bPowered = true;

		auto Fx = [w](double f) { return (f - S47Signal::FMin) / (S47Signal::FMax - S47Signal::FMin) * w; };
		const double Tuned = Fx(Rx.Frequency);
		const double Half = FMath::Max(2.0, (Rx.Bandwidth / 1000) / (S47Signal::FMax - S47Signal::FMin) * w * 0.5);
		Out.TunedX = (float)Tuned;
		Out.BandX0 = (float)(Tuned - Half);
		Out.BandX1 = (float)(Tuned + Half);

		const FS47Profile& P = Rx.Profile();
		const bool bShowPeak = Rx.Stage != 2 || bResidualVisible;
		const double Q = Rx.Quality();
		const double Amp = (12 + 110 * Q * Rx.Gain / 100) * (h / 220) * Rx.Pulse(T);
		const double PeakX = Fx(P.F);
		const double HydrogenX = Fx(1420.406);

		Out.Trace.Reserve((int32)(w / 2) + 2);
		for (double x = 0; x <= w; x += 2)
		{
			const double Noise = (FMath::Sin(x * 0.17 + T * 4) * 2 + FMath::Sin(x * 0.71 + T * 7) * 3 + (Rand() - 0.5) * 3)
				* (Rx.Bandwidth / 35 + 0.2) * (h / 220);
			double y = h * 0.84 - Noise;
			if (bShowPeak) y -= Amp * FMath::Exp(-FMath::Square((x - PeakX) / (7 * w / 512)));
			// the hydrogen line itself is always a soft broad hump
			y -= 14 * (h / 220) * FMath::Exp(-FMath::Square((x - HydrogenX) / (40 * w / 512)));
			Out.Trace.Add(FVector2f((float)x, (float)y));
		}
	}
}

// ---------------------------------------------------------------------------------------------
// Self test. Expected values come from running Signal.ts in node (esbuild bundle); see the
// expected-value table in the port notes. Quality to 1e-9, pulse exact, geometry to 1e-3
// (the trace is stored as float).

bool S47RxSelfTest(TArray<FString>& Out)
{
	bool bAll = true;
	auto Check = [&](bool bOk, const FString& What)
	{
		bAll &= bOk;
		Out.Add(FString::Printf(TEXT("%s %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), *What));
	};
	auto Near = [&](double Got, double Want, double Tol, const FString& What)
	{
		Check(FMath::Abs(Got - Want) <= Tol, FString::Printf(TEXT("%s: got %.12g want %.12g"), *What, Got, Want));
	};

	// Profiles
	{
		const TArray<FS47Profile>& P = S47Signal::Profiles();
		Check(P.Num() == 3, TEXT("3 profiles"));
		Check(P[0].Stage == TEXT("CALIBRATION") && P[0].bCardIsTaped && P[0].F == 1419.9 && P[0].FTol == 0.025, TEXT("profile 0 CALIBRATION"));
		Check(P[1].Stage == TEXT("INTERFERENCE") && !P[1].bCardIsTaped && P[1].F == 1420.11 && P[1].Az[1] == 180, TEXT("profile 1 INTERFERENCE"));
		Check(P[2].Stage == TEXT("ANOMALY") && P[2].F == 1420.405 && P[2].FTol == 0.008 && P[2].Gain[0] == 78, TEXT("profile 2 ANOMALY"));
	}

	// Quality / pass
	FS47Rx Rx;
	Near(Rx.Quality(), 0.00025498100963120723, 1e-9, TEXT("quality at defaults"));
	Check(!Rx.Pass(), TEXT("pass false at defaults"));
	Rx.Frequency = 1419.9; Rx.Gain = 55; Rx.Bandwidth = 48; Rx.Azimuth = 42;
	Near(Rx.Quality(), 1.0, 1e-9, TEXT("quality at calibration card"));
	Check(Rx.Pass(), TEXT("pass true at calibration card"));
	Rx.Gain = 70;
	Check(!Rx.Pass(), TEXT("pass false with gain 70 (> 65)"));
	Near(Rx.Quality(), 0.94965844314957926, 1e-9, TEXT("quality with gain 70"));

	// The visible 99/100 boundary must agree with acceptance for every profile,
	// including scheduled observations which supply their own profile.
	{
		Check(S47Signal::CarrierPercent(.99499) == 99 && S47Signal::CarrierPercent(.99501) == 100,
			TEXT("carrier rounding boundary is shared by display and acceptance"));
		for (const FS47Profile& P : S47Signal::Profiles())
		{
			FS47Rx R;
			R.Gain = (P.Gain[0] + P.Gain[1]) / 2;
			R.Bandwidth = (P.Bw[0] + P.Bw[1]) / 2;
			R.Azimuth = (P.Az[0] + P.Az[1]) / 2;
			for (double Q : { .99499, .99501 })
			{
				R.Frequency = P.F + P.FTol + FMath::Max(P.FTol * 4, .025) * FMath::Sqrt(-FMath::Loge(Q));
				const bool FullMeter = S47Signal::CarrierPercent(R.Quality(P)) == 100;
				Check(FullMeter == (Q > .995) && R.Pass(P) == FullMeter,
					FString::Printf(TEXT("%s accepts exactly the displayed 100 near frequency boundary %.5f"), *P.Stage, Q));
			}
			R.Frequency = P.F;
			R.Gain = P.Gain[1] + .8; R.Bandwidth = P.Bw[1] + .8; R.Azimuth = P.Az[1] + .8;
			Check(R.Pass(P) && S47Signal::CarrierPercent(R.Quality(P)) == 100,
				FString::Printf(TEXT("%s accepts rounded full carrier with several slightly imperfect dials"), *P.Stage));
			R.Gain = P.Gain[1] + 6;
			Check(!R.Pass(P) && S47Signal::CarrierPercent(R.Quality(P)) < 100,
				FString::Printf(TEXT("%s still rejects visibly incomplete tuning"), *P.Stage));
		}
	}


	// A saved lock stage cannot fabricate full carrier after its dials drift.
	{
		FS47Rx R;
		R.Stage = 3;
		Check(S47Signal::CarrierPercent(R.Quality()) < 100 && !R.Pass(),
			TEXT("stage 3 uses actual profile quality rather than forcing 100 percent"));
	}

	// Pulse
	{
		const double Ts[] = { 0.05, 0.15, 0.5, 1.7, 2.2, 3.0, 5.1 };
		const double Stage3[] = { 1, 0.06, 1, 0.06, 1, 1, 0.06 };
		FS47Rx R;
		R.Stage = 3;
		for (int32 i = 0; i < 7; i++)
			Check(R.Pulse(Ts[i]) == Stage3[i], FString::Printf(TEXT("pulse(%.2f) stage 3 == %.2f"), Ts[i], Stage3[i]));
		R.Stage = 0;
		for (int32 i = 0; i < 7; i++)
			Check(R.Pulse(Ts[i]) == 1.0, FString::Printf(TEXT("pulse(%.2f) stage 0 == 1"), Ts[i]));
	}

	// Advance
	{
		FS47Rx R;
		Check(R.Status == TEXT("CALIBRATION REQUIRED") && R.Stage == 0, TEXT("initial status"));
		R.Advance();
		Check(R.Stage == 1 && R.Status == TEXT("LOCAL CARRIER DRIFT")
			&& R.Frequency == 1419.98 && R.Gain == 55 && R.Bandwidth == 58 && R.Azimuth == 42, TEXT("advance 1: LOCAL CARRIER DRIFT, 1419.98/55/58/42"));
		Near(R.Quality(), 0.062784663667834376, 1e-9, TEXT("quality after advance 1"));
		Check(!R.Pass(), TEXT("pass false after advance 1"));
		R.Advance();
		Check(R.Stage == 2 && R.Status == TEXT("SURVEY SWEEP ACTIVE  /  H-LINE  /  UNTIL 06:00")
			&& R.Frequency == 1419.98 && R.Gain == 55 && R.Bandwidth == 58 && R.Azimuth == 42, TEXT("advance 2: SURVEY SWEEP ACTIVE, dials unchanged"));
		Near(R.Quality(), 3.0180182019789699e-77, 1e-9, TEXT("quality after advance 2"));
		R.Advance();
		Check(R.Stage == 3 && R.Status == TEXT("PATTERN LOCK  /  PULSE GROUP 4 / 7")
			&& R.Frequency == 1420.405 && R.Gain == 82 && R.Bandwidth == 12 && R.Azimuth == 83, TEXT("advance 3: PATTERN LOCK, 1420.405/82/12/83"));
		Near(R.Quality(), 1.0, 1e-9, TEXT("quality after advance 3"));
		Check(R.Pass(), TEXT("pass true after advance 3"));
		R.Advance();
		Check(R.Stage == 3 && R.Status == TEXT("PATTERN LOCK  /  PULSE GROUP 4 / 7"), TEXT("advance 4: no change"));
		R.Reset();
		Check(R.Stage == 0 && R.Status == TEXT("CALIBRATION REQUIRED") && R.Frequency == 1419.62 && R.Gain == 27
			&& R.Bandwidth == 82 && R.Azimuth == 18 && !R.bPowered && !R.bSolved, TEXT("reset to initial values"));
	}

	// Spectrum geometry, W=512 H=384 T=1.0, Rand = 0.5, stage 0, powered, default dials
	{
		FS47Rx R;
		FS47SpectrumGeometry G;
		auto Half = []() { return 0.5; };
		S47Spectrum::Build(R, 1.0, false, 512, 384, Half, G);
		Check(!G.bPowered && G.Trace.Num() == 0 && G.GridX.Num() == 9 && G.GridY.Num() == 5, TEXT("unpowered: grid only (9 x, 5 y)"));

		R.bPowered = true;
		S47Spectrum::Build(R, 1.0, false, 512, 384, Half, G);
		Check(G.bPowered, TEXT("powered flag"));
		Check(G.Trace.Num() == 257, FString::Printf(TEXT("trace has 257 points (got %d)"), G.Trace.Num()));
		Check(G.GridX.Num() == 9 && G.GridX[1] == 64.f && G.GridX[8] == 512.f, TEXT("grid x every 64 px, 0..512"));
		Check(G.GridY.Num() == 5, TEXT("grid y 5 lines"));
		Near(G.GridY[0], 57.6, 1e-3, TEXT("grid y[0] = h*0.15"));
		Near(G.GridY[4], 318.72, 1e-3, TEXT("grid y[4]"));
		Near(G.TunedX, 51.199999999951494, 1e-3, TEXT("tuned x"));
		Near(G.BandX0, 33.706666666618823, 1e-3, TEXT("band x0"));
		Near(G.BandX1, 68.693333333284158, 1e-3, TEXT("band x1"));
		if (G.Trace.Num() == 257)
		{
			Check(G.Trace[0].X == 0.f && G.Trace[128].X == 256.f && G.Trace[256].X == 512.f, TEXT("trace x at 0, 256, 512"));
			Near(G.Trace[0].Y, 320.53005742413603, 1e-3, TEXT("trace[0].y"));
			Near(G.Trace[128].Y, 322.50419420064856, 1e-3, TEXT("trace[128].y"));
			Near(G.Trace[256].Y, 324.45927221662072, 1e-3, TEXT("trace[256].y"));
		}

		// Stage 2 hides the peak unless the residual is visible: the two traces differ at the peak.
		FS47Rx R2 = R;
		R2.Stage = 2;
		FS47SpectrumGeometry Hidden, Shown;
		S47Spectrum::Build(R2, 1.0, false, 512, 384, Half, Hidden);
		S47Spectrum::Build(R2, 1.0, true, 512, 384, Half, Shown);
		const int32 PeakIdx = FMath::RoundToInt32((1420.405 - S47Signal::FMin) / (S47Signal::FMax - S47Signal::FMin) * 512 / 2);
		Check(Hidden.Trace.Num() == 257 && Shown.Trace.Num() == 257 && Shown.Trace[PeakIdx].Y < Hidden.Trace[PeakIdx].Y - 1.f,
			TEXT("stage 2: peak only with residualVisible"));
	}

	Out.Add(bAll ? TEXT("S47Rx self test: all passed") : TEXT("S47Rx self test: FAILURES"));
	return bAll;
}
