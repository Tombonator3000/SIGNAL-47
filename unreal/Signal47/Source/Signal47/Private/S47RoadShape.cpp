#include "S47RoadShape.h"

namespace S47RoadShape
{
namespace
{
	double Base(double X, double Z)
	{
		return 1.1 * FMath::Sin(X * 0.0061 + 0.7) * FMath::Cos(Z * 0.0047 + 0.3)
			+ 0.7 * FMath::Sin((X + Z) * 0.0032 + 1.9) + 0.4 * FMath::Sin(Z * 0.011 - X * 0.004 + 0.4);
	}
	double B0() { static const double V = Base(START.X, START.Z); return V; }
	double Hypot(double A, double B) { return FMath::Sqrt(A * A + B * B); }

	double Raw(double X, double Z)
	{
		const double Dx = X + 250, Dz = Z - 300, A = FMath::Atan2(Dz, Dx);
		const double Hills = Smooth(1300, 2400, Hypot(Dx, Dz)) * (12 + 9 * FMath::Sin(A * 5 + 1.3) + 5 * FMath::Sin(A * 13));
		return Base(X, Z) - B0() - 1.3 * WashAt(X, Z) + Hills;
	}
}

double Smooth(double A, double B, double X)
{
	const double T = FMath::Min(1.0, FMath::Max(0.0, (X - A) / (B - A)));
	return T * T * (3 - 2 * T);
}

double Hash(double X, double Z)
{
	const double S = FMath::Sin(X * 12.9898 + Z * 78.233) * 43758.5453;
	return S - FMath::Floor(S);
}

double WashAt(double X, double Z)
{
	const double W = X + 330 + 0.25 * (Z - 500);
	return FMath::Exp(-W * W / 520) * Smooth(-150, 60, Z);
}

// (x and z sit on whole 0.15 m steps from where the diner stood before)
const FDinerAt& DINER() { static const FDinerAt D = { -20.05, 1999.95, -11.05, Raw(-11.05, 1999.95) }; return D; }
const FTownAt& TOWN() { static const FTownAt T = { DINER().Z + 18, 2300, -42, 48 }; return T; }
const FMotelAt& MOTEL() { static const FMotelAt M = { -14.75, 386, Raw(-14.75, 386) }; return M; }

double HLow(double X, double Z)
{
	const double R = Raw(X, Z), D = Hypot(X - DINER().FlatX, Z - DINER().Z), M = Hypot(X - MOTEL().FlatX, Z - MOTEL().Z);
	if (M < 110) return R + (MOTEL().Y - R) * (1 - Smooth(45, 110, M));
	return D > 110 ? R : R + (DINER().Y - R) * (1 - Smooth(45, 110, D));
}

double HwyLift(double Ax)
{
	return Ax <= HWY_W.Paved ? 0.06 + 0.06 * (1 - FMath::Square(Ax / HWY_W.Paved)) : Ax <= HWY_W.Shoulder ? 0.06 * (HWY_W.Shoulder - Ax) / (HWY_W.Shoulder - HWY_W.Paved) : 0;
}

double HighwayTop(double X, double Z) { return HLow(X, Z) + HwyLift(FMath::Abs(X)); }

double OldRoadBearing(double Mi)
{
	auto B = [Mi](double A, double C, double M0, double M1) { return A + (C - A) * Smooth(M0, M1, Mi); };
	if (Mi < 5.6)
	{
		// off the highway due east, round into the east-north-east, then long easy bends
		const double Lead = 63 + 27 * (1 - Smooth(0.02, 0.3, Mi)), M = Mi - 0.3;
		return Lead + (8 * FMath::Sin(M * 2.3) + 4 * FMath::Sin(M * 5.1 + 1)) * Smooth(0.3, 1.0, Mi) * (1 - Smooth(4.6, 5.5, Mi));
	}
	if (Mi < 6.75) return B(63, 45, 6.25, 6.75);
	if (Mi < 7.75) return B(45, 35, 7.25, 7.75);
	return B(35, 27, 8.5, 8.9);
}
}
