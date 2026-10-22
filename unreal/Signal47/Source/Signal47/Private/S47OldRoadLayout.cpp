#include "S47OldRoadLayout.h"

#include "S47RoadTerrain.h"

#include <limits>

namespace S47OldRoadLayout
{
using namespace S47RoadShape;

namespace
{
	constexpr double INF = std::numeric_limits<double>::infinity();
	FXZ GJ;   // J, set once the path is laid out (the module sets J.x and J.z as it builds it)

	// ---------- the land: long low swells, hills rising far out ----------
	double Own(double X, double Z)
	{
		const double R = FMath::Sqrt(FMath::Square(X + 800) + FMath::Square(Z - 800));
		const double Hills = Smooth(2600, 5200, R) * (22 + 14 * FMath::Sin(FMath::Atan2(Z, X) * 4 + 0.8));
		return 2.2 * FMath::Sin(X * 0.0021 + 0.4) * FMath::Cos(Z * 0.0017 + 1.1) + 1.4 * FMath::Sin((X - Z) * 0.0011 + 2.0)
			+ 0.6 * FMath::Sin(X * 0.0071 - Z * 0.0043) + Hills;
	}
	double TerrainAt(double X, double Z)
	{
		const double Rx = X - GJ.X, W = Smooth(250, 1100, FMath::Abs(Rx));
		if (W >= 1) return Own(X, Z);
		const double R = S47RoadTerrain::SurfaceY(Rx, Z - GJ.Z + DINER().Z);
		return R + (Own(X, Z) - R) * W;
	}
	double Detail(double X, double Z) { return 0.18 * FMath::Sin(X * 0.13 + 1.7) * FMath::Sin(Z * 0.11 + 0.3) + 0.09 * FMath::Sin(X * 0.31 - Z * 0.23 + 0.9); }

	// ---------- nearest point on the road, through a coarse grid of path segments ----------
	constexpr double CELL = 100;
	int64 Key(int64 Cx, int64 Cz) { return (Cx + 1000) * 4000 + (Cz + 1000); }
	const TMap<int64, TArray<int32>>& Cells()
	{
		static const TMap<int64, TArray<int32>> C = []
		{
			TMap<int64, TArray<int32>> Out;
			const TArray<FRoadPt>& P = Path();
			for (int32 i = 0; i < P.Num() - 1; ++i)
			{
				for (const FRoadPt* Q : { &P[i], &P[i + 1] })
				{
					const int64 K = Key(int64(FMath::Floor(Q->X / CELL)), int64(FMath::Floor(Q->Z / CELL)));
					if (TArray<int32>* L = Out.Find(K)) { if (L->Last() != i) L->Add(i); }
					else Out.Add(K, { i });
				}
			}
			return Out;
		}();
		return C;
	}
}

double Terrain(double X, double Z) { Path(); return TerrainAt(X, Z); }
double OwnLand(double X) { return Smooth(250, 1100, FMath::Abs(X - J().X)); }

const FXZ& J() { Path(); return GJ; }

const TArray<FRoadPt>& Path()
{
	static const TArray<FRoadPt> All = []
	{
		TArray<FRoadPt> Fwd, Back;
		auto At = [](double S, double X, double Z)
		{
			const double A = Bearing(MiAt(S)) * UE_DOUBLE_PI / 180;
			FRoadPt P; P.X = X; P.Z = Z; P.Tx = FMath::Sin(A); P.Tz = -FMath::Cos(A); P.S = S; P.Y = 0;
			return P;
		};
		FRoadPt P = At(0, 0, 0);
		Fwd.Add(P);
		for (double S = STEP; S <= S1; S += STEP)
		{
			const double A = Bearing(MiAt(S - STEP / 2)) * UE_DOUBLE_PI / 180;
			P = At(S, P.X + FMath::Sin(A) * STEP, P.Z - FMath::Cos(A) * STEP); Fwd.Add(P);
		}
		P = Fwd[0];
		for (double S = -STEP; S >= S0; S -= STEP)
		{
			const double A = Bearing(MiAt(S + STEP / 2)) * UE_DOUBLE_PI / 180;
			P = At(S, P.X - FMath::Sin(A) * STEP, P.Z + FMath::Cos(A) * STEP); Back.Add(P);
		}
		TArray<FRoadPt> Out;
		for (int32 i = Back.Num() - 1; i >= 0; --i) Out.Add(Back[i]);
		Out.Append(Fwd);
		GJ.X = Out[0].X; GJ.Z = Out[0].Z;
		// the road is graded: its height is the land under it, smoothed over about 120 m
		TArray<double> Raw;
		for (const FRoadPt& Q : Out) Raw.Add(TerrainAt(Q.X, Q.Z));
		const int32 W = 15;
		for (int32 i = 0; i < Out.Num(); ++i)
		{
			double Sum = 0; int32 N = 0;
			for (int32 K = -W; K <= W; ++K) { const int32 Jx = i + K; if (Jx >= 0 && Jx < Raw.Num()) { Sum += Raw[Jx]; N++; } }
			Out[i].Y = Sum / N;
		}
		return Out;
	}();
	return All;
}

int32 I0() { return int32(FMath::RoundToDouble(-S0 / STEP)); }

FRoadPt RoadAt(double S)
{
	const TArray<FRoadPt>& P = Path();
	const double F = FMath::Clamp((S - S0) / STEP, 0.0, P.Num() - 1.001);
	const int32 I = int32(FMath::Floor(F));
	const double K = F - I;
	const FRoadPt& A = P[I];
	const FRoadPt& B = P[I + 1];
	FRoadPt R;
	R.X = A.X + (B.X - A.X) * K; R.Z = A.Z + (B.Z - A.Z) * K; R.Tx = A.Tx + (B.Tx - A.Tx) * K; R.Tz = A.Tz + (B.Tz - A.Tz) * K; R.S = S; R.Y = A.Y + (B.Y - A.Y) * K;
	return R;
}

FBeside Beside(double S, double O)
{
	const FRoadPt P = RoadAt(S);
	return { P.X - P.Tz * O, P.Z + P.Tx * O, P };
}

FNearest Nearest(double X, double Z)
{
	FNearest Near = { INF, 0, 0, 0 };
	const TArray<FRoadPt>& P = Path();
	const TMap<int64, TArray<int32>>& C = Cells();
	const int64 Cx = int64(FMath::Floor(X / CELL)), Cz = int64(FMath::Floor(Z / CELL));
	double Best = INF;
	for (int64 Dx = -1; Dx <= 1; ++Dx) for (int64 Dz = -1; Dz <= 1; ++Dz)
	{
		const TArray<int32>* L = C.Find(Key(Cx + Dx, Cz + Dz));
		if (!L) continue;
		for (const int32 i : *L)
		{
			const FRoadPt& A = P[i];
			const FRoadPt& B = P[i + 1];
			const double Ex = B.X - A.X, Ez = B.Z - A.Z, L2 = Ex * Ex + Ez * Ez;
			const double Tr = ((X - A.X) * Ex + (Z - A.Z) * Ez) / L2;
			if (i == 0 && Tr < 0) continue;   // behind mile 0 is the highway and the diner, not this road
			const double T = FMath::Min(1.0, FMath::Max(0.0, Tr));
			const double Px = A.X + Ex * T - X, Pz = A.Z + Ez * T - Z, D2 = Px * Px + Pz * Pz;
			if (D2 < Best)
			{
				Best = D2;
				Near.S = A.S + STEP * T; Near.Y = A.Y + (B.Y - A.Y) * T;
				Near.O = (X - (A.X + Ex * T)) * -A.Tz + (Z - (A.Z + Ez * T)) * A.Tx;
			}
		}
	}
	if (Best < INF) Near.D = FMath::Sqrt(Best);
	return Near;
}

double Profile(double O)
{
	const double A = FMath::Abs(O);
	if (A <= HALF) return 0.1 + 0.04 * (1 - FMath::Square(A / HALF));
	if (A <= SHOULDER) return 0.1 - 0.06 * (A - HALF) / (SHOULDER - HALF);
	if (A <= 7) return 0.04 - 0.2 * Smooth(SHOULDER, 7, A);
	if (A <= 10) return -0.16 - 0.3 * Smooth(7, 9, A) + 0.3 * Smooth(9, 10, A);
	return -0.16;
}

double HeightAt(double X, double Z)
{
	const FNearest N = Nearest(X, Z);
	const double Land = Terrain(X, Z), Ax = FMath::Abs(X - J().X);
	const double Dl = Land + Detail(X, Z) * OwnLand(X) * (N.D == INF ? 1 : Smooth(10, 20, N.D));
	if (N.D == INF) return Dl;
	if (Ax <= S47RoadTerrain::HWY.Shoulder) return Land;
	const double Road = N.Y + Profile(N.D);
	double W = Smooth(16, 70, N.D);
	// off the strip, the old road's verges and ditches begin past the highway's own verge
	if (N.D > SHOULDER) W = FMath::Max(W, 1 - Smooth(S47RoadTerrain::HWY.Margin, S47RoadTerrain::HWY.Margin + 25, Ax));
	return Road + (Dl - Road) * W;
}

const FStart& START()
{
	static const FStart S = []
	{
		const double Sv = SAt(5.9);
		const FBeside B = Beside(Sv, 1.6);
		return FStart{ B.X, B.Z, -FMath::Atan2(B.P.Tx, -B.P.Tz), Sv };
	}();
	return S;
}
}
