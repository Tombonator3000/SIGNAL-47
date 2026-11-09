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

void Storm(double K, const FVector2D& Dir)
{
	const double S = FMath::Clamp(K, 0.0, 1.0);
	if (S <= 0 || Dir.SizeSquared() < 1e-8) return;
	FWind& W = Wind();
	// harder in the gusts as well as on the whole, so that the brush whips rather than leans
	W.Strength *= 1 + 2.5 * S * (0.85 + 0.15 * FMath::Sin(W.Time * 1.3));
	const FVector2D Blend = W.Dir * (1 - S) + Dir.GetSafeNormal() * S;
	if (Blend.SizeSquared() > 1e-8) W.Dir = Blend.GetSafeNormal();
}
}
