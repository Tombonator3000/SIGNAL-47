#include "S47Backdrop.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "S47Sky.h"

namespace
{
	// THREE.MathUtils.smoothstep(x, min, max)
	double Smoothstep(double X, double Min, double Max)
	{
		if (X <= Min) return 0;
		if (X >= Max) return 1;
		const double T = (X - Min) / (Max - Min);
		return T * T * (3 - 2 * T);
	}
	FLinearColor Lerp(const FLinearColor& A, const FLinearColor& B, double K) { return A + (B - A) * float(K); }
}

namespace S47Backdrop
{
double HorizonDip(double H) { return FMath::Atan(FMath::Max(0.0, H) / 20000) * 180 / UE_DOUBLE_PI; }

FEdge& HorizonEdge()
{
	static FEdge E;
	return E;
}
}

void FS47Backdrop::Update(FS47Sky& Sky, const FLinearColor& InFog, double Density, bool bOldRoad, double H, const TArray<FS47SkyPlace>& Places, const FVector& Cam, double Yaw)
{
	Sky.BandVisible = !bHold;
	const double Dawn = Sky.Dawn, Day = Smoothstep(Dawn, 0.05, 0.6);
	Fog = InFog;
	Rim = bOldRoad ? 1 : 0;
	Dip = S47Backdrop::HorizonDip(H);
	// a little of the night's air on them (dark against the night sky), more of it as the day comes;
	// on the old road the air is the dawn sky's own haze, and less of that keeps them dark against it
	BodyK = bOldRoad ? 0.2 - 0.1 * Day : 0.2 + 0.2 * Day;
	// the land where it ends, as far as it is seen from here: about a kilometre out from a man's eye
	// or the truck's (the rest is a line under it), many from the air
	const double Far = 900 + 40 * FMath::Max(0.0, H);
	S47Backdrop::FEdge& E = S47Backdrop::HorizonEdge();
	E.Fog = 1 - FMath::Exp(-FMath::Square(Density * Far));
	E.Land = Lerp(FLinearColor(0.009f, 0.009f, 0.012f), FLinearColor(0.024f, 0.017f, 0.013f), Day);
	// the mesa the sun comes up behind: dark, against the light at dawn
	Mesa = Lerp(FLinearColor(0.0045f, 0.0055f, 0.0085f), FLinearColor(0.02f, 0.019f, 0.026f), Day);
	// the glows: brightest a few hundred metres out, gone right inside the place (its own lamps are
	// all round then) and far off; wider the nearer it is; the night's only
	const double Night = 1 - Smoothstep(Dawn, 0.15, 0.6), C = FMath::Cos(-Yaw), S = FMath::Sin(-Yaw);
	for (int32 i = 0; i < S47Backdrop::GLOWS; ++i)
	{
		FVector4& G = Sky.Glow[i];
		if (!Places.IsValidIndex(i) || Night <= 0) { G = FVector4(1, 0, 0, 1); continue; }
		const FS47SkyPlace& P = Places[i];
		const double Dx = P.X - Cam.X, Dz = P.Z - Cam.Z;
		double Dist = FMath::Sqrt(Dx * Dx + Dz * Dz);
		if (Dist == 0) Dist = 1;
		// into the sky group's own terms (it is turned by yaw)
		const double Lx = (Dx * C + Dz * S) / Dist, Lz = (-Dx * S + Dz * C) / Dist;
		const double K = P.K * Night * Smoothstep(Dist, P.R * 0.6, P.R * 2.2) / (1 + FMath::Square(Dist / 2600));
		G = FVector4(Lx, Lz, K, FMath::Max(0.06, FMath::Atan(P.R / Dist) * 1.3));
	}
	if (UMaterialInstanceDynamic* D = Sky.Band())
	{
		D->SetScalarParameterValue(TEXT("Gain"), float(Gain));
		D->SetScalarParameterValue(TEXT("Rim"), float(Rim));
		D->SetScalarParameterValue(TEXT("BodyK"), float(BodyK));
		D->SetScalarParameterValue(TEXT("Dip"), float(Dip));
		D->SetVectorParameterValue(TEXT("Fog"), Fog);
		D->SetVectorParameterValue(TEXT("Mesa"), Mesa);
		D->SetVectorParameterValue(TEXT("Land"), E.Land);
		D->SetScalarParameterValue(TEXT("EdgeFog"), float(E.Fog));
	}
}
