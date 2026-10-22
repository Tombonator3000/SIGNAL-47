#include "S47Wind.h"

namespace S47Wind
{
FWind& Wind()
{
	static FWind W;
	return W;
}

void WindAt(double T)
{
	FWind& W = Wind();
	W.Time = T;
	W.Strength = 0.07 * (0.75 + 0.2 * FMath::Sin(T * 0.11) + 0.15 * FMath::Sin(T * 0.37 + 1.3));
	const double A = 0.64 + 0.18 * FMath::Sin(T * 0.023);
	W.Dir = FVector2D(FMath::Cos(A), FMath::Sin(A));
}
}
