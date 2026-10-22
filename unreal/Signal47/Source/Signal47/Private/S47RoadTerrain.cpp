#include "S47RoadTerrain.h"

#include <limits>

namespace S47RoadTerrain
{
namespace
{
	constexpr double INF = std::numeric_limits<double>::infinity();
	double Hypot(double A, double B) { return FMath::Sqrt(A * A + B * B); }

	// three's CatmullRomCurve3 (open, centripetal) and Curve's arc length table (200 divisions),
	// as the track is made with getSpacedPoints
	struct FCubicPoly
	{
		double C0 = 0, C1 = 0, C2 = 0, C3 = 0;
		void Init(double X0, double X1, double T0, double T1)
		{
			C0 = X0; C1 = T0; C2 = -3 * X0 + 3 * X1 - 2 * T0 - T1; C3 = 2 * X0 - 2 * X1 + T0 + T1;
		}
		void InitNonuniformCatmullRom(double X0, double X1, double X2, double X3, double Dt0, double Dt1, double Dt2)
		{
			// tangents when parameterized in [t1, t2], rescaled for [0, 1]
			double T1 = (X1 - X0) / Dt0 - (X2 - X0) / (Dt0 + Dt1) + (X2 - X1) / Dt1;
			double T2 = (X2 - X1) / Dt1 - (X3 - X1) / (Dt1 + Dt2) + (X3 - X2) / Dt2;
			T1 *= Dt1; T2 *= Dt1;
			Init(X1, X2, T1, T2);
		}
		double Calc(double T) const { const double T2 = T * T, T3 = T2 * T; return C0 + C1 * T + C2 * T2 + C3 * T3; }
	};

	struct FCatmullRom
	{
		TArray<FVector> Points;
		TArray<double> Lengths;

		FVector GetPoint(double T) const
		{
			const int32 L = Points.Num();
			const double P = (L - 1) * T;
			int32 IntPoint = int32(FMath::Floor(P));
			double Weight = P - IntPoint;
			if (Weight == 0 && IntPoint == L - 1) { IntPoint = L - 2; Weight = 1; }
			const FVector P0 = IntPoint > 0 ? Points[(IntPoint - 1) % L] : Points[0] - Points[1] + Points[0];   // extrapolate the first
			const FVector P1 = Points[IntPoint % L], P2 = Points[(IntPoint + 1) % L];
			const FVector P3 = IntPoint + 2 < L ? Points[(IntPoint + 2) % L] : Points[L - 1] - Points[L - 2] + Points[L - 1];   // and the last
			double Dt0 = FMath::Pow(FVector::DistSquared(P0, P1), 0.25);
			double Dt1 = FMath::Pow(FVector::DistSquared(P1, P2), 0.25);
			double Dt2 = FMath::Pow(FVector::DistSquared(P2, P3), 0.25);
			// safety check for repeated points
			if (Dt1 < 1e-4) Dt1 = 1.0;
			if (Dt0 < 1e-4) Dt0 = Dt1;
			if (Dt2 < 1e-4) Dt2 = Dt1;
			FCubicPoly Px, Py, Pz;
			Px.InitNonuniformCatmullRom(P0.X, P1.X, P2.X, P3.X, Dt0, Dt1, Dt2);
			Py.InitNonuniformCatmullRom(P0.Y, P1.Y, P2.Y, P3.Y, Dt0, Dt1, Dt2);
			Pz.InitNonuniformCatmullRom(P0.Z, P1.Z, P2.Z, P3.Z, Dt0, Dt1, Dt2);
			return FVector(Px.Calc(Weight), Py.Calc(Weight), Pz.Calc(Weight));
		}
		void MakeLengths(int32 Divisions = 200)
		{
			Lengths.Reset();
			FVector Last = GetPoint(0);
			double Sum = 0;
			Lengths.Add(0);
			for (int32 P = 1; P <= Divisions; ++P)
			{
				const FVector Current = GetPoint(double(P) / Divisions);
				Sum += FVector::Dist(Current, Last);
				Lengths.Add(Sum);
				Last = Current;
			}
		}
		double GetLength() const { return Lengths.Last(); }
		double GetUtoTmapping(double U) const
		{
			const int32 Il = Lengths.Num();
			const double Target = U * Lengths[Il - 1];
			int32 Low = 0, High = Il - 1, I = 0;
			while (Low <= High)
			{
				I = int32(FMath::Floor(Low + (High - Low) / 2.0));
				const double Comparison = Lengths[I] - Target;
				if (Comparison < 0) Low = I + 1;
				else if (Comparison > 0) High = I - 1;
				else { High = I; break; }
			}
			I = High;
			if (Lengths[I] == Target) return double(I) / (Il - 1);
			const double Before = Lengths[I], After = Lengths[I + 1];
			return (I + (Target - Before) / (After - Before)) / (Il - 1);
		}
		TArray<FVector> GetSpacedPoints(int32 Divisions) const
		{
			TArray<FVector> Out;
			for (int32 D = 0; D <= Divisions; ++D) Out.Add(GetPoint(GetUtoTmapping(double(D) / Divisions)));
			return Out;
		}
	};

	const double KEYS[][2] = { { -5, 520 }, { -24, 520 }, { -70, 518 }, { -150, 507 }, { -240, 496 }, { -330, 497 }, { -420, 508 }, { -500, 522 }, { -560, 530 }, { -606, 532 } };

	double Detail(double X, double Z) { return 0.16 * FMath::Sin(X * 0.13 + 1.7) * FMath::Sin(Z * 0.11 + 0.3) + 0.08 * FMath::Sin(X * 0.31 - Z * 0.23 + 0.9); }

	/** Height of the desert grid at a vertex: small bumps away from the roads, sunk under the road strips. */
	double GridH(double X, double Z)
	{
		const double Hd = FMath::Abs(X), Td = TrackNearest(X, Z).D, Od = OldNearest(X, Z).D;
		// no small bumps where the land meets another area's ground: north towards SARO, and by
		// the station's gate; none round the diner's and the motel's lots
		const double Meet = Smooth(-200, -140, Z) * Smooth(40, 90, Hypot(X - END.X, Z - END.Z)) * Smooth(40, 90, Hypot(X - DINER().FlatX, Z - DINER().Z))
			* Smooth(40, 90, Hypot(X - MOTEL().FlatX, Z - MOTEL().Z));
		double H = HLow(X, Z) + Detail(X, Z) * Smooth(0, 15, FMath::Min3(Hd - HWY.Margin, Td - TRK.Margin, Od - OLD.Margin)) * Meet;
		if (Hd < HWY.Flat || Td < TRK.Flat) H -= 0.45; // hidden under the road strips: no z-fighting
		return H;
	}

	// ---------- the desert grid: 10 m cells over the drive, wider cells out to the horizon ----------
	TArray<double> Axis(double C0, double C1, double Step, double Lo, double Hi)
	{
		TArray<double> Mid, Left, Right;
		for (double V = C0; V <= C1 + 1e-6; V += Step) Mid.Add(V);
		for (double S = Step, V = C0; V > Lo;) { S = FMath::Min(120.0, S * 1.25); V = FMath::Max(Lo, V - S); Left.Add(V); }
		for (double S = Step, V = C1; V < Hi;) { S = FMath::Min(120.0, S * 1.25); V = FMath::Min(Hi, V + S); Right.Add(V); }
		TArray<double> Out;
		for (int32 i = Left.Num() - 1; i >= 0; --i) Out.Add(Left[i]);
		Out.Append(Mid);
		Out.Append(Right);
		return Out;
	}
	const TArray<float>& GridHeights()
	{
		static const TArray<float> H = []
		{
			TArray<float> Out;
			const TArray<double>& Xs = GX();
			const TArray<double>& Zs = GZ();
			Out.SetNumZeroed(Xs.Num() * Zs.Num());
			for (int32 j = 0; j < Zs.Num(); ++j) for (int32 i = 0; i < Xs.Num(); ++i) Out[j * Xs.Num() + i] = float(GridH(Xs[i], Zs[j]));
			return Out;
		}();
		return H;
	}
	int32 Cell(const TArray<double>& A, double V)
	{
		int32 Lo = 0, Hi = A.Num() - 2;
		while (Lo < Hi) { const int32 M = (Lo + Hi + 1) >> 1; if (A[M] <= V) Lo = M; else Hi = M - 1; }
		return Lo;
	}

	/** What the old road lies on: the highway's verge by the highway, the drawn desert grid out beyond it. */
	double OldBase(double X, double Z)
	{
		const double K = Smooth(18, 26, X);
		return K <= 0 ? HLow(X, Z) : K >= 1 ? GridY(X, Z) : HLow(X, Z) + (GridY(X, Z) - HLow(X, Z)) * K;
	}
	double OldLift(double D) { return D <= OLD.Half ? 0.1 + 0.04 * (1 - FMath::Square(D / OLD.Half)) : 0.1 * (OLD.Shoulder - D) / (OLD.Shoulder - OLD.Half); }
	double Rut(double D, double E) { return RUT[2] * FMath::Max(0.0, 1 - FMath::Abs(D - RUT[0] * E) / (RUT[1] * E)); }
	double TrkLift(double D, double W) { return D <= W ? 0.03 + 0.02 * (1 - FMath::Square(D / W)) - Rut(D, W + TRK.Soft) : 0.03 * (W + TRK.Soft - D) / TRK.Soft; }
	double DropY(double D, double Flat, double Edge) { return D <= Flat ? 0 : -0.6 * (D - Flat) / (Edge - Flat); }
}

const TArray<FRoadStop>& ROAD_STOPS()
{
	static const TArray<FRoadStop> R = {
		{ TEXT("rsPasture"), TEXT("The pasture by the highway"), -160, -HWY.Fence + 0.5, 490, 640 },
		{ TEXT("rsHerdRoad"), TEXT("The highway south"), -HWY.Fence - 0.5, HWY.Fence - 0.15, 490, 640 },
		{ TEXT("rsDeputyCar"), TEXT("The highway north of Caliche"), -HWY.Flat, HWY.Flat, DEPUTY_CAR.Z - 40, DEPUTY_CAR.Z + 40 },
	};
	return R;
}

const FTrack& Track()
{
	static const FTrack T = []
	{
		FCatmullRom Curve;
		for (const auto& K : KEYS) Curve.Points.Add(FVector(K[0], 0, K[1]));
		Curve.MakeLengths();
		FTrack Out;
		for (const FVector& P : Curve.GetSpacedPoints(int32(FMath::CeilToDouble(Curve.GetLength() / 3)))) Out.Pts.Add(FVector2D(P.X, P.Z));
		Out.S.Add(0);
		for (int32 i = 1; i < Out.Pts.Num(); ++i) Out.S.Add(Out.S[i - 1] + FVector2D::Distance(Out.Pts[i], Out.Pts[i - 1]));
		Out.Length = Out.S.Last();
		return Out;
	}();
	return T;
}

double TrackHalf(double S) { return TRK.Half + 2.6 * (1 - Smooth(0, 15, S)); }

FNear TrackNearest(double X, double Z)
{
	FNear Near = { INF, 0 };
	if (X > 40 || X < -650 || Z < 450 || Z > 580) return Near;
	const FTrack& T = Track();
	const TArray<FVector2D>& P = T.Pts;
	for (int32 i = 0; i < P.Num() - 1; ++i)
	{
		const double Ax = P[i].X, Az = P[i].Y, Dx = P[i + 1].X - Ax, Dz = P[i + 1].Y - Az;
		const double L2 = Dx * Dx + Dz * Dz;
		const double Tt = FMath::Min(1.0, FMath::Max(0.0, ((X - Ax) * Dx + (Z - Az) * Dz) / L2));
		const double Ex = Ax + Dx * Tt - X, Ez = Az + Dz * Tt - Z, D = Ex * Ex + Ez * Ez;
		if (D < Near.D) { Near.D = D; Near.S = T.S[i] + FMath::Sqrt(L2) * Tt; }
	}
	Near.D = FMath::Sqrt(Near.D);
	return Near;
}

const TArray<FStubPt>& OldStub()
{
	// (oldRoadLayout.ts builds it backwards from C in the same 4 m steps)
	static const TArray<FStubPt> Pts = []
	{
		const double OLD_S0 = -FMath::Floor(CROSS_MI * MILE / 4) * 4;   // the old road's first point (oldRoadLayout.S0)
		TArray<FStubPt> Out;
		double X = 0, Z = DINER().Z;
		for (double S = 0; S <= OLD.Len; S += 4)
		{
			const double A = OldRoadBearing(CROSS_MI + (OLD_S0 + S) / MILE) * UE_DOUBLE_PI / 180;
			Out.Add({ X, Z, FMath::Sin(A), -FMath::Cos(A), S });
			const double B = OldRoadBearing(CROSS_MI + (OLD_S0 + S + 2) / MILE) * UE_DOUBLE_PI / 180;
			X += FMath::Sin(B) * 4; Z -= FMath::Cos(B) * 4;
		}
		return Out;
	}();
	return Pts;
}

FNear OldNearest(double X, double Z)
{
	FNear Near = { INF, 0 };
	if (X < -2 || X > OLD.Len + 30 || FMath::Abs(Z - DINER().Z) > 120) return Near;
	const TArray<FStubPt>& P = OldStub();
	for (int32 i = 0; i < P.Num() - 1; ++i)
	{
		const double Ax = P[i].X, Az = P[i].Z, Dx = P[i + 1].X - Ax, Dz = P[i + 1].Z - Az, L2 = Dx * Dx + Dz * Dz;
		const double Tr = ((X - Ax) * Dx + (Z - Az) * Dz) / L2;
		if (i == 0 && Tr < 0) continue;
		const double Tt = FMath::Min(1.0, FMath::Max(0.0, Tr)), Ex = Ax + Dx * Tt - X, Ez = Az + Dz * Tt - Z, D = Ex * Ex + Ez * Ez;
		if (D < Near.D) { Near.D = D; Near.S = P[i].S + 4 * Tt; }
	}
	Near.D = FMath::Sqrt(Near.D);
	return Near;
}

const TArray<double>& GX() { static const TArray<double> A = Axis(-800, 60, 10, -3000, 2300); return A; }
const TArray<double>& GZ() { static const TArray<double> A = Axis(-480, 2760, 10, -2700, 4400); return A; }

double GridY(double X, double Z)
{
	const TArray<double>& Xs = GX();
	const TArray<double>& Zs = GZ();
	const TArray<float>& H = GridHeights();
	const int32 I = Cell(Xs, X), J = Cell(Zs, Z), N = Xs.Num();
	const double U = FMath::Min(1.0, FMath::Max(0.0, (X - Xs[I]) / (Xs[I + 1] - Xs[I]))), V = FMath::Min(1.0, FMath::Max(0.0, (Z - Zs[J]) / (Zs[J + 1] - Zs[J])));
	const double A = H[J * N + I], B = H[J * N + I + 1], C = H[(J + 1) * N + I], D = H[(J + 1) * N + I + 1];
	return U + V <= 1 ? A + (B - A) * U + (C - A) * V : D + (C - D) * (1 - U) + (B - D) * (1 - V);
}

double SurfaceY(double X, double Z)
{
	const double Ax = FMath::Abs(X);
	if (Ax <= HWY.Shoulder) return HLow(X, Z) + HwyLift(Ax);
	const FNear T = TrackNearest(X, Z);
	const double W = TrackHalf(T.S);
	const FNear O = OldNearest(X, Z);
	if (T.D <= W + TRK.Soft) return HLow(X, Z) + TrkLift(T.D, W);
	if (O.D <= OLD.Shoulder) return OldBase(X, Z) + OldLift(O.D);
	double Y = GridY(X, Z);
	if (Ax <= HWY.Margin) Y = FMath::Max(Y, HLow(X, Z) + DropY(Ax, HWY.Flat, HWY.Margin));
	if (T.D <= TRK.Margin) Y = FMath::Max(Y, HLow(X, Z) + DropY(T.D, TRK.Flat, TRK.Margin));
	return Y;
}

double RanchZ(double X) { return 196 + (X - 15) * 0.012 + FMath::Sin(X * 0.021) * 1.2; }

bool OnRanchRoad(double X, double Z)
{
	return (X > 6 && X < RANCH_TO + 4 && FMath::Abs(Z - RanchZ(X)) < 3.2) || (X > 8 && X < 11 && FMath::Abs(Z - RanchZ(9.6) + 4.2) < 1);
}
}
