#include "S47GroundCover.h"
#include "S47SaroSite.h"

#include "S47Geo.h"
#include "S47Legs.h"
#include "S47OldRoadLayout.h"
#include "S47RoadShape.h"
#include "S47RoadTerrain.h"

using namespace S47RoadTerrain;
using S47Flora::Ragged;

namespace
{
	using FRect = FS47FloraRect;
	FRect R(double MinX, double MaxX, double MinZ, double MaxZ) { FRect O; O.MinX = MinX; O.MaxX = MaxX; O.MinZ = MinZ; O.MaxZ = MaxZ; return O; }
	const S47Geo::FXZ& SIR() { return S47Geo::SARO_IN_ROAD; }
	FRect ToRoadRect(const FRect& r) { return R(r.MinX + SIR().X, r.MaxX + SIR().X, r.MinZ + SIR().Z, r.MaxZ + SIR().Z); }
	bool Within(const FRect& r, double X, double Z, double M = 0) { return X >= r.MinX - M && X <= r.MaxX + M && Z >= r.MinZ - M && Z <= r.MaxZ + M; }
	double SegDist(double Px, double Pz, double Ax, double Az, double Bx, double Bz)
	{
		const double Dx = Bx - Ax, Dz = Bz - Az, L2 = Dx * Dx + Dz * Dz;
		const double T = L2 > 0 ? FMath::Min(1.0, FMath::Max(0.0, ((Px - Ax) * Dx + (Pz - Az) * Dz) / L2)) : 0;
		return FMath::Sqrt(FMath::Square(Px - Ax - Dx * T) + FMath::Square(Pz - Az - Dz * T));
	}
	double LineDist(double Px, double Pz, const TArray<FVector2D>& Pts)
	{
		double D = TNumericLimits<double>::Max();
		for (int32 i = 0; i < Pts.Num() - 1; ++i) D = FMath::Min(D, SegDist(Px, Pz, Pts[i].X, Pts[i].Y, Pts[i + 1].X, Pts[i + 1].Y));
		return D;
	}
	/** A way's edge: none on it (to `on`), thin just off it (to `thin`), thick out to `ditch`; j (ragged)
	 *  takes the thin band and the thick one further out here and there, by up to 1.2 m. */
	double Edge(double D, double On, double Thin, double Ditch, double Thick = 1.9, double J = 0) { return D < On ? 0 : D < Thin + 1.2 * J ? 0.3 : D < Ditch + 1.2 * J ? Thick : 1; }
	/** Along a fence: none in its line, thick either side. */
	double Fence(double D) { return D < 0.22 ? 0 : D < 1.9 ? 2.4 : 1; }

	// ---------- the road's land (road-local) ----------
	const FRect& MOTEL_ROAD() { static const FRect V = ToRoadRect(R(S47Geo::MOTEL_RECT().MinX, S47Geo::MOTEL_RECT().MaxX, S47Geo::MOTEL_RECT().MinZ, S47Geo::MOTEL_RECT().MaxZ)); return V; }
	// world/Crossing.ts SHOULDER (site), in the road's terms
	const FRect& SHOULDER_ROAD() { static const FRect V = ToRoadRect(R(-33.5, -15, 30, S47Geo::MOTEL_RECT().MaxZ + 4)); return V; }
	// the gaps in the highway's fences, as drive/roadProps.ts leaves them
	double DINER_GAP0() { return S47RoadShape::DINER().Z - 40; }
	double EAST_GAP0() { return S47RoadShape::DINER().Z - 18; }
	double MOTEL_GAP0() { return S47RoadShape::MOTEL().Z - 46; }
	double MOTEL_GAP1() { return S47RoadShape::MOTEL().Z + 46; }
	// the town's sidewalks and the gas station's apron (world/Town.ts, ground and gasStation)
	const TArray<FRect>& PAVED() { static const TArray<FRect> V = { R(-17.6, -14.2, 2016, 2040), R(-17.6, -14.2, 2090, 2250), R(16.6, 20.2, 2024, 2200), R(-17.6, -5.6, 2040, 2090) }; return V; }
	bool InTown(double X, double Z)
	{
		const auto& T = S47RoadShape::TOWN();
		return X > T.West - 8 && X < T.East + 8 && Z > S47RoadShape::DINER().Z - 20 && Z < T.Z1 + 20;
	}

	/** A seam's weeds only where the way's edge is near: on a way, there must be ground off every way
	 *  0.55 m one way or another. */
	double Seamed(double V, TFunctionRef<FString(double, double)> Ways, double X, double Z)
	{
		return V > 0 && (Ways(X, Z).IsEmpty() || Ways(X + 0.55, Z).IsEmpty() || Ways(X - 0.55, Z).IsEmpty() || Ways(X, Z + 0.55).IsEmpty() || Ways(X, Z - 0.55).IsEmpty()) ? V : 0;
	}

	double RoadSeamAt(double X, double Z)
	{
		using namespace S47RoadShape;
		if (X > -672 && X < -598 && Z > 500 && Z < 566) return 0;   // (the station's and the ranch's own)
		if (X > 395 && X < 485 && Z > 150 && Z < 235) return 0;
		const double Ax = FMath::Abs(X);
		// (none where a lot, the diner's or the gas station's apron opens on the highway: the truck turns in there)
		bool bMouthPaved = false;
		if (InTown(X, Z)) for (const FRect& Rc : PAVED()) bMouthPaved |= Within(Rc, X, Z, 1);
		const bool bMouth = Ax < 7.2 && (Within(MOTEL_ROAD(), X, Z, 1) || (X < 0 && X > DINER().X - 9 && FMath::Abs(Z - DINER().Z) < 18) || bMouthPaved);
		if (Z > HWY.Z0 && Z < HWY.Z1 && Ax < 6.3) return Ax > 5.5 && !bMouth ? 0.8 : 0;
		if (InTown(X, Z)) for (const FRect& Rc : PAVED()) if (Within(Rc, X, Z, 0.15))
		{
			const double Ex = FMath::Min(X - Rc.MinX, Rc.MaxX - X), Ez = FMath::Min(Z - Rc.MinZ, Rc.MaxZ - Z);
			return (Ex < 0.5 || Ez < 0.5) && !bMouth ? 0.9 : 0;
		}
		if (X > GUARD.X0 - 0.6 && X < GUARD.X1 + 0.6 && Z > GUARD.Z0 - 0.6 && Z < GUARD.Z1 + 0.6) return 0;
		const FNear T = TrackNearest(X, Z);
		if (T.D < 40) { const double W = TrackHalf(T.S); if (T.D < W + 0.5) return T.D > W - 0.5 ? 0.8 : 0; }
		const FNear O = OldNearest(X, Z);
		if (O.D < OLD.Shoulder + 0.4) return O.D > OLD.Shoulder - 0.45 ? 0.8 : 0;
		if (Within(MOTEL_ROAD(), X, Z, 0.3)) return Within(MOTEL_ROAD(), X, Z, -0.5) || bMouth ? 0 : 0.7;
		if (X > DINER().X - 9 && X < DINER().X + 17.5 && FMath::Abs(Z - DINER().Z) < 17) return X > DINER().X + 3 && FMath::Abs(Z - DINER().Z) > 15.5 && !bMouth ? 0.7 : 0;
		return 0;
	}

	// ---------- SARO (site terms) ----------
	// world/Grounds.ts: the floors (RAISED), the truck ramp, the steps, the service road, the worn path
	const TArray<FRect>& RAISED()
	{
		static const TArray<FRect> V = { R(-6.7, 6.7, -5.9, 5.9), R(-6.6, 2.6, 5.8, 11.1), R(-7.6, -6.3, 4.75, 6.65), R(6.3, 11.0, -0.6, 3.4), R(8.0, 11.0, -19.6, -0.6),
			R(11.0, 12.6, -13.4, -8.6), R(11.0, 12.6, -1.9, -0.6), R(6.6, 13.0, 3.4, 12.4), R(12.6, 19.2, -6.0, 1.0), R(20.2, 39.0, -5.9, 5.9) };
		return V;
	}
	struct FRamp { double MinX, MaxX, Top, Foot; };
	constexpr FRamp TRUCK_RAMP = { 6.6, 13.0, 12.4, 16.6 };
	struct FSteps { double X0, X1, Z0, Z1; };
	constexpr FSteps STEPS = { 7.1, 8.0, -12.6, -11.4 };
	constexpr double ROAD_W = 5.0;
	const TArray<FVector2D>& ROAD_LINE()
	{
		// Grounds.ts roadLine(): down the ramp's line to the south, a bend to the west, then straight to the highway
		static TArray<FVector2D> V;
		if (V.Num() == 0)
		{
			const double Cx = 5.8, Cz = 20.5, Rr = 4.0, RoadZ = Cz + Rr;
			V.Add(FVector2D(9.8, 16.2)); V.Add(FVector2D(9.8, Cz));
			for (int32 i = 1; i <= 10; ++i) { const double A = (i / 10.0) * UE_DOUBLE_PI / 2; V.Add(FVector2D(Cx + FMath::Cos(A) * Rr, Cz + FMath::Sin(A) * Rr)); }
			for (double X : { 0.0, -8.0, -16.0, -20.4 }) V.Add(FVector2D(X, RoadZ));
		}
		return V;
	}
	const TArray<FVector2D>& PATH_PTS()
	{
		static const TArray<FVector2D> V = { { -10.5, 5.7 }, { -9.8, 7.9 }, { -9.2, 10.4 }, { -8.1, 12.4 }, { -6.1, 13.5 }, { -2.0, 14.0 }, { 2.0, 14.3 }, { 4.6, 15.0 }, { 6.0, 16.4 }, { 6.9, 18.0 } };
		return V;
	}
	const TArray<FVector2D>& RANCH_PTS() { static const TArray<FVector2D> V = { { -18.6, 34.5 }, { 46.5, 34.5 }, { 46.5, -19.9 } }; return V; }
	// world/arrayLayout.ts
	constexpr double FENCE_Z = -20;
	struct FGateX { double X0, X1; };
	constexpr FGateX ARRAY_GATE = { -1.75, -0.25 };
	TArray<FRect>& FIELD_ZONES() { static TArray<FRect> V; return V; }
	FRect& FIELD_BOUNDS() { static FRect V = R(0, 0, 0, 0); return V; }
	bool OnField(double X, double Z, double M = 0)
	{
		if (!(Z < FENCE_Z)) return false;
		for (const FRect& Rc : FIELD_ZONES()) if (Within(Rc, X, Z, M)) return true;
		return false;
	}
	S47Geo::FRect ARRAY_FLAT() { const FRect& B = FIELD_BOUNDS(); return { B.MinX - 60, B.MaxX + 60, B.MinZ - 95, 0 }; }

	double SiteSeamAt(double X, double Z)
	{
		for (const FRect& B : RAISED()) if (Within(B, X, Z, 0.15)) return 0;
		if (X > TRUCK_RAMP.MinX - 0.2 && X < TRUCK_RAMP.MaxX + 0.2 && Z > TRUCK_RAMP.Top && Z < TRUCK_RAMP.Foot + 0.3) return 0;
		if (X > STEPS.X0 - 0.5 && X < STEPS.X1 + 0.2 && Z > STEPS.Z0 - 0.2 && Z < STEPS.Z1 + 0.2) return 0;
		const double Rd = LineDist(X, Z, ROAD_LINE());
		if (Rd < ROAD_W / 2 + 0.85) return Rd > ROAD_W / 2 + 0.3 ? 0.8 : 0;
		if (X > 5.2 && X < 14.4 && Z > 15.8 && Z < 20.2) return FMath::Min(FMath::Min(X - 5.2, 14.4 - X), FMath::Min(Z - 15.8, 20.2 - Z)) < 0.5 ? 0.8 : 0;
		const double Pd = LineDist(X, Z, PATH_PTS());
		if (Pd < 0.7) return Pd > 0.38 ? 0.7 : 0;
		if ((X > -20.2 && X < -10.4 && FMath::Abs(Z - 5.7) < 1.0) || (FMath::Abs(X + 1) < 0.5 && Z < -6 && Z > -19.6)) return 0;
		const double Hx = FMath::Abs(X + 24);
		if (Hx < 4.6) return Hx > 3.9 ? 0.8 : 0;
		if (Z < FENCE_Z + 0.5 && OnField(X, Z, 0.25)) return 0.8;
		return 0;
	}

	// ---------- STATION 01 (world/Station01.ts, station-local) ----------
	const TArray<TArray<FVector2D>>& PATHS()
	{
		static const TArray<TArray<FVector2D>> V = {
			{ { -22.3, 28.5 }, { -22.3, 24.6 }, { -19.8, 19.5 }, { -15.8, 12.8 }, { -11.8, 7.8 }, { -9.95, 5.7 } },
			{ { -9.95, 5.7 }, { -6.8, 4.2 }, { -3.2, 2.2 }, { 0.4, -0.6 }, { 2.0, -2.0 }, { 2.2, -3.4 }, { 3.0, -5.0 }, { 6.2, -7.0 }, { 9.4, -9.0 } },
			{ { 9.4, -9.0 }, { 13.6, -6.6 }, { 16.2, -4.8 }, { 14.0, 0.5 }, { 7.0, 7.5 }, { -2.5, 12.8 }, { -11.5, 16.8 }, { -17.6, 20.4 } },
			{ { 0.4, -0.6 }, { -1.6, -4.4 }, { -4.8, -8.0 }, { -6.9, -9.4 } },
		};
		return V;
	}
	// the cable's course on the ground, from the plinth to the pier (cablePath2D; CUT at 3.2, -1.3)
	const TArray<FVector2D>& CABLE()
	{
		static const TArray<FVector2D> V = { { -9.1, 5.0 }, { -8.2, 5.75 }, { -7.2, 5.95 }, { -6.2, 5.7 }, { -4.7, 5.8 }, { -3.8, 5.6 }, { -2.6, 5.0 }, { -1.4, 4.4 }, { -0.4, 3.6 }, { 0.6, 2.7 },
			{ 1.4, 1.7 }, { 2.1, 0.9 }, { 2.75, 0.1 }, { 3.05, -0.6 }, { 3.2, -1.3 }, { 3.22, -2.2 }, { 3.15, -3.0 }, { 3.1, -3.5 } };
		return V;
	}
}

namespace S47GroundCover
{
void SetField(const TArray<FS47FloraRect>& Zones)
{
	FIELD_ZONES() = Zones;
	FRect B = R(TNumericLimits<double>::Max(), -TNumericLimits<double>::Max(), TNumericLimits<double>::Max(), -TNumericLimits<double>::Max());
	for (const FRect& Z : Zones) { B.MinX = FMath::Min(B.MinX, Z.MinX); B.MaxX = FMath::Max(B.MaxX, Z.MaxX); B.MinZ = FMath::Min(B.MinZ, Z.MinZ); B.MaxZ = FMath::Max(B.MaxZ, Z.MaxZ); }
	FIELD_BOUNDS() = Zones.Num() ? B : R(0, 0, 0, 0);
}

double RoadCover(double X, double Z)
{
	using namespace S47RoadShape;
	const double Ax = FMath::Abs(X), J = Ragged(X, Z);
	double D = 1;
	if (Z > HWY.Z0 && Z < HWY.Z1)
	{
		if (Ax < 6.3) return 0;                                  // the asphalt and its gravel shoulders
		D = Ax < 6.9 + 1.4 * J ? 0.3 : Ax < 15.6 + 1.2 * J ? 1 : Ax < 18.6 ? 1.9 : 1;   // the verge, the ditch past it
		const bool bTown = InTown(X, Z);
		if (!bTown && Z > -1000 && Z < 2900)
		{
			const double Fx = FMath::Abs(Ax - HWY.Fence);
			const bool bGap = X > 0 ? Z > EAST_GAP0() && Z < TOWN().Z1 : (Z > MOTEL_GAP0() && Z < MOTEL_GAP1()) || (Z > GUARD.Z0 - 0.6 && Z < GUARD.Z1 + 0.6) || (Z > DINER_GAP0() && Z < TOWN().Z1);
			if (!bGap) D *= Fence(Fx);
		}
		if (bTown) for (const FRect& Rc : PAVED()) if (Within(Rc, X, Z, 0.15)) return 0;
	}
	// the survey track: gravel, its soft edges, its ditches
	const FNear T = TrackNearest(X, Z);
	if (T.D < 40) { const double W = TrackHalf(T.S); D = FMath::Min(D, 1.0) * Edge(T.D, W + 0.5, W + TRK.Soft, TRK.Margin + 0.6, 1.9, J); if (D == 0) return 0; }
	if (X > GUARD.X0 - 0.6 && X < GUARD.X1 + 0.6 && Z > GUARD.Z0 - 0.6 && Z < GUARD.Z1 + 0.6) return 0;   // the cattle guard
	if (OnRanchRoad(X, Z)) return 0;                           // the ranch's road east of the highway
	// the old road's first stretch across from the diner, and its barricades
	const FNear O = OldNearest(X, Z);
	if (O.D < 30) { D = FMath::Min(D, 1.0) * Edge(O.D, OLD.Shoulder + 0.4, OLD.Shoulder + 0.9, OLD.Margin + 0.5, 1.9, J); if (D == 0) return 0; }
	// the pasture south of the motel: grazed grass; the fences round it
	if (X < -HWY.Fence && X > PASTURE.West - 2 && Z > PASTURE.North - 2 && Z < PASTURE.South + 2)
	{
		D *= FMath::Min3(Fence(FMath::Abs(Z - PASTURE.North)), Fence(FMath::Abs(Z - PASTURE.South)), Fence(FMath::Abs(X - PASTURE.West))) * 1.25;
	}
	// Sierra Motor Court's lot, walk and rooms: none, thick round them
	if (Within(MOTEL_ROAD(), X, Z, 0.3)) return 0;
	if (Within(MOTEL_ROAD(), X, Z, 1.2 + 2 * J)) D *= 1.8;
	// the diner's building and lot, and the station and the ranch's yard far off
	if (X > DINER().X - 9 && X < DINER().X + 17.5 && FMath::Abs(Z - DINER().Z) < 17) return 0;
	if (X > -672 && X < -598 && Z > 500 && Z < 566) return 0;
	if (X > 395 && X < 485 && Z > 150 && Z < 235) return 0;
	return D;
}

FString RoadWays(double X, double Z)
{
	using namespace S47RoadShape;
	if (FMath::Abs(X) < 6 && Z > HWY.Z0 && Z < HWY.Z1) return TEXT("the highway");
	const FNear T = TrackNearest(X, Z);
	if (T.D < TrackHalf(T.S)) return TEXT("the survey track");
	if (OldNearest(X, Z).D < OLD.Half) return TEXT("the old road");
	if (OnRanchRoad(X, Z)) return TEXT("the ranch's road");
	if (Within(MOTEL_ROAD(), X, Z)) return TEXT("the motel's lot");
	if (InTown(X, Z)) for (const FRect& Rc : PAVED()) if (Within(Rc, X, Z)) return TEXT("a sidewalk in Caliche");
	if (X > DINER().X + 3 && X < DINER().X + 16.7 && FMath::Abs(Z - DINER().Z) < 16) return TEXT("the diner's lot");
	return FString();
}

double RoadSeam(double X, double Z) { return Seamed(RoadSeamAt(X, Z), [](double A, double B) { return RoadWays(A, B); }, X, Z); }

double RoadSpill(double X, double Z)
{
	if (Z > HWY.Z0 && Z < HWY.Z1 && FMath::Abs(X) < 7) return 4;
	const FNear T = TrackNearest(X, Z);
	if (T.D < 40 && T.D < TrackHalf(T.S) + 1.1) return 4;
	if (OldNearest(X, Z).D < OLD.Shoulder + 1) return 4;
	if (Within(MOTEL_ROAD(), X, Z, 0.9)) return 2;
	return 1;
}

double SiteCover(double X, double Z)
{
	for (const FRect& B : RAISED()) if (Within(B, X, Z, 0.15)) return 0;
	if (X > TRUCK_RAMP.MinX - 0.2 && X < TRUCK_RAMP.MaxX + 0.2 && Z > TRUCK_RAMP.Top && Z < TRUCK_RAMP.Foot + 0.3) return 0;
	if (X > STEPS.X0 - 0.5 && X < STEPS.X1 + 0.2 && Z > STEPS.Z0 - 0.2 && Z < STEPS.Z1 + 0.2) return 0;
	const double J = Ragged(X, Z);
	// the service road and its gravel, the gravel at the foot of the ramp, the worn path, the path
	// from the fire exit to the highway, the cable trench's covers
	const double Rd = LineDist(X, Z, ROAD_LINE());
	double D = Edge(Rd, ROAD_W / 2 + 0.85, ROAD_W / 2 + 1.2, ROAD_W / 2 + 3, 1.9, J);
	if (D == 0 || (X > 5.2 && X < 14.4 && Z > 15.8 && Z < 20.2)) return 0;
	D *= Edge(LineDist(X, Z, PATH_PTS()), 0.7, 0.9, 1.8, 1.4, J * 0.6);
	if (D == 0 || (X > -20.2 && X < -10.4 && FMath::Abs(Z - 5.7) < 1.0) || (FMath::Abs(X + 1) < 0.5 && Z < -6 && Z > -19.6)) return 0;
	// the highway north through the array, from the site's own south end
	const double Hx = FMath::Abs(X + 24);
	if (Hx < 4.6) return 0;
	if (Hx < 6) D *= 0.35; else if (Hx < 9) D *= 1.6;
	// the array's gravel: none on it, thin at its edge, thick a metre or two out
	if (Z < FENCE_Z + 0.5 && OnField(X, Z, 3))
	{
		if (OnField(X, Z, 0.25)) return 0;
		D *= OnField(X, Z, 0.5 + 1.3 * J) ? 0.35 : 1.8;
	}
	// the chain-link fence to the array (open at the highway and at the walk gate), the ranch fence
	if (X > -81 && X < 61)
	{
		const bool bGap = (X > -28.6 && X < -19.4) || (X > ARRAY_GATE.X0 - 0.3 && X < ARRAY_GATE.X1 + 0.3);
		if (!bGap) D *= Fence(FMath::Abs(Z - FENCE_Z));
	}
	D *= Fence(LineDist(X, Z, RANCH_PTS()));
	return D;
}

FString SiteWays(double X, double Z)
{
	for (const FRect& B : RAISED()) if (Within(B, X, Z)) return TEXT("a floor of the house");
	if (LineDist(X, Z, ROAD_LINE()) < ROAD_W / 2) return TEXT("the service road");
	if (LineDist(X, Z, PATH_PTS()) < 0.5) return TEXT("the worn path");
	if (FMath::Abs(X + 24) < 4) return TEXT("the highway");
	if (Z < FENCE_Z && OnField(X, Z)) return TEXT("the array's gravel");
	return FString();
}

double SiteSeam(double X, double Z) { return Seamed(SiteSeamAt(X, Z), [](double A, double B) { return SiteWays(A, B); }, X, Z); }

double SiteSpill(double X, double Z)
{
	if (LineDist(X, Z, ROAD_LINE()) < ROAD_W / 2 + 1.5) return 4;
	if (Z < FENCE_Z + 0.5 && OnField(X, Z, 0.85)) return 4;
	if (FMath::Abs(X + 24) < 5.4) return 2;
	return 1;
}

double SaroLand(double X, double Z)
{
	// Exterior.ts saroLand: flat at -0.62 within 300 m and over the array, rolling further out, and
	// south along the highway the road's land, kept out of the way under the road's own piece of it
	using S47RoadShape::Smooth;
	const double Far = FMath::Sqrt(X * X + Z * Z);
	double Y = Far > 300 ? -2 + FMath::Sin(X * 0.004 + Z * 0.003) * 6 * FMath::Min(1.0, (Far - 300) / 600) : -0.62;
	// the array is graded flat; the land's roll comes back over 40 m past it
	const double E = S47Geo::Outside(ARRAY_FLAT(), X, Z);
	if (E < 40) Y = -0.62 + (Y + 0.62) * Smooth(0, 40, E);
	const double Rx = X + SIR().X, Rz = Z + SIR().Z;
	const double W = Smooth(40, 100, Z) * (1 - Smooth(0, 160, S47Geo::Outside(S47Geo::SARO_PATCH, Rx, Rz)));
	if (W > 0) Y += (S47RoadShape::HLow(Rx, Rz) - (S47Geo::InRect(S47Geo::SARO_PATCH, Rx, Rz, -2) ? 0.7 : 0.05) - Y) * W;
	return Y;
}

TArray<FS47FloraRect> PostsOf(const TArray<FObstacle>& Obstacles, double Dx, double Dz)
{
	TArray<FS47FloraRect> Out;
	for (const FObstacle& O : Obstacles) if (O.bCircle) Out.Add(R(O.X - O.R - Dx, O.X + O.R - Dx, O.Z - O.R - Dz, O.Z + O.R - Dz));
	return Out;
}

FS47FloraSpec RoadFlora(const TArray<FS47FloraRect>& Posts)
{
	FS47FloraSpec S;
	S.Name = TEXT("road"); S.Seed = COVER_SEED;
	S.Ground = [](double X, double Z) { return SurfaceY(X, Z); };
	S.Cover = [](double X, double Z) { return RoadCover(X, Z); };
	S.Seam = [](double X, double Z) { return RoadSeam(X, Z); };
	S.Spill = [](double X, double Z) { return RoadSpill(X, Z); };
	S.Blocks = Posts;
	S.Walks = { SHOULDER_ROAD() };
	S.Ways = [](double X, double Z) { return RoadWays(X, Z); };
	return S;
}

FS47FloraSpec SaroFlora(const TArray<FS47FloraRect>& Colliders, const TArray<FS47FloraRect>& Zones)
{
	// in the road's terms, the site's rules north of site z 80 and the road's south of it, on the ground drawn there
	FS47FloraSpec S;
	S.Name = TEXT("saro"); S.Seed = COVER_SEED;
	S.Ground = [](double X, double Z)
	{
		if (Z >= -200 && S47Geo::InRect(S47Geo::SARO_PATCH, X, Z)) return SurfaceY(X, Z);
		const double Sx = X - SIR().X, Sz = Z - SIR().Z;
		double Y = SaroLand(Sx, Sz);
		if (Sz > 36 && FMath::Abs(X) < HWY.Flat) Y = FMath::Max(Y, S47RoadShape::HLow(X, Z));   // the verge of the road's stretch
		return Y;
	};
	S.Cover = [](double X, double Z) { const double Sx = X - SIR().X, Sz = Z - SIR().Z; return Sz < 80 ? SiteCover(Sx, Sz) : RoadCover(X, Z); };
	S.Seam = [](double X, double Z) { const double Sx = X - SIR().X, Sz = Z - SIR().Z; return Sz < 80 ? SiteSeam(Sx, Sz) : RoadSeam(X, Z); };
	S.Spill = [](double X, double Z) { const double Sx = X - SIR().X, Sz = Z - SIR().Z; return Sz < 80 ? SiteSpill(Sx, Sz) : RoadSpill(X, Z); };
	// (the site's own ground; south of it the road's rules, as the road area's) moved into the road's terms
	for (const FRect& C : Colliders) if (C.MinZ < 80) S.Blocks.Add(ToRoadRect(C));
	for (const FRect& Z : Zones) if (Z.MinZ < 80) S.Walks.Add(ToRoadRect(Z));
	// Unreal visitor paving: generate no procedural plants through the lot or its footpaths.
	// Flora is created after the arrival meshes, so a one-off mesh cleanup cannot enforce this.
	for (const FRect& P : { R(-18.3,-7.7,10.4,20.6), R(-16.5,-9.5,20.5,23.5),
		R(-8.3,5.9,11.65,12.75), R(4.85,5.95,12.2,17.3), R(5.4,11.5,16.75,17.85),
		R(-12.2,-11.0,5.7,10.5), R(-12.2,-9.4,5.1,6.3) })
		S.Blocks.Add(ToRoadRect(P));
	// Match the actual paved east forecourt and its service-road connector exactly.
	// This excludes grass, scrub and stones at generation, leaving the surrounding desert intact.
	using namespace S47SaroForecourt;
	S.Blocks.Add(ToRoadRect(R(X0,X1,Z0,Z1)));
	S.Blocks.Add(ToRoadRect(R(AccessX0,AccessX1,AccessZ0,AccessZ1)));
	S.Walks.Add(SHOULDER_ROAD());
	S.Ways = [](double X, double Z) { const double Sx = X - SIR().X, Sz = Z - SIR().Z; return Sz < 80 ? SiteWays(Sx, Sz) : RoadWays(X, Z); };
	return S;
}

bool StationClear(double X, double Z, double Rad)
{
	auto KeepRect = [X, Z](double X0, double X1, double Z0, double Z1, double M) { return X > X0 - M && X < X1 + M && Z > Z0 - M && Z < Z1 + M; };
	auto Poly = [X, Z](const TArray<FVector2D>& P) { return LineDist(X, Z, P); };
	auto Hyp = [X, Z](double Px, double Pz) { return FMath::Sqrt(FMath::Square(X - Px) + FMath::Square(Z - Pz)); };
	const FVector2D T(3, -4), SIGHT = FVector2D(1, -1).GetSafeNormal(), OFF = FVector2D(1, 1).GetSafeNormal();
	const FVector2D FOOT_A = T + SIGHT * 11, POST_A = T + SIGHT * 10 + OFF * 3, POST_B(-7.4, -10.0), LAMP(1.6, -2.6), POLE(-6.6, 7.0);
	if (KeepRect(-13.6, -8.4, 0.9, 5.6, Rad + 0.6)) return false;                      // hut and step
	if (KeepRect(-17.3, -15.1, -1.3, 0.5 + 1.7, Rad + 0.7)) return false;              // shed and drum
	if (KeepRect(-28.5, -11.8, 25.6, 41, Rad + 0.5)) return false;                     // pad and truck
	if (KeepRect(-22.0, -5.5, 4.8, 26.5, Rad) && Poly({ { -19.25, 26 }, { -18, 20 }, { -15, 13.5 }, { -12.5, 9.5 }, { -8, 7.5 } }) < 4.2 + Rad) return false;
	if (Hyp(T.X, T.Y) < 3.4 + Rad) return false;
	if (Hyp(LAMP.X, LAMP.Y) < 1.4 + Rad) return false;
	for (const FVector2D& P : { FOOT_A, POST_A, POST_B }) if (Hyp(P.X, P.Y) < 1.6 + Rad) return false;
	if (Hyp(POLE.X, POLE.Y) < 1.2 + Rad) return false;
	if (Hyp(7.45, -7.33) < 1.4 + Rad) return false;                                    // photo spot for A
	if (X > 6 && X < 60 && FMath::Abs(Z - T.Y) < 1.1 + Rad) return false;               // the C stakes
	if (Poly(S47Geo::STATION_TRACK()) < 3.4 + Rad) return false;
	{
		const S47Geo::FXZ Rr = S47Geo::StationToRoad(X, Z);
		if (S47Geo::InRect(S47Geo::STATION_PATCH, Rr.X, Rr.Z, -(Rad + 1))) return false;  // the road's own plants are there
	}
	for (const TArray<FVector2D>& P : PATHS()) if (Poly(P) < 1.0 + Rad) return false;
	if (Poly(CABLE()) < 0.9 + Rad) return false;
	const auto& F = S47Geo::STATION_FENCE;
	const bool bOnFence = ((FMath::Abs(Z - F.Z1) < 0.8 + Rad || FMath::Abs(Z - F.Z0) < 0.8 + Rad) && X > F.X0 - 1 && X < F.X1 + 1)
		|| ((FMath::Abs(X - F.X0) < 0.8 + Rad || FMath::Abs(X - F.X1) < 0.8 + Rad) && Z > F.Z0 - 1 && Z < F.Z1 + 1);
	return !bOnFence;
}

FS47FloraSpec StationFlora(const TArray<FS47FloraRect>& Colliders, const TArray<FS47FloraRect>& Zones, const FVector& Origin, const TArray<FS47FloraRect>& Posts)
{
	// a station rectangle (world) in the road's terms: local, then turned
	auto StationRect = [Origin](const FRect& Rw)
	{
		const S47Geo::FXZ A = S47Geo::StationToRoad(Rw.MinX - Origin.X, Rw.MinZ - Origin.Z), B = S47Geo::StationToRoad(Rw.MaxX - Origin.X, Rw.MaxZ - Origin.Z);
		return R(FMath::Min(A.X, B.X), FMath::Max(A.X, B.X), FMath::Min(A.Z, B.Z), FMath::Max(A.Z, B.Z));
	};
	FS47FloraSpec S;
	S.Name = TEXT("station01"); S.Seed = COVER_SEED;
	S.Ground = [Origin](double X, double Z)
	{
		if (S47Geo::InRect(S47Geo::STATION_PATCH, X, Z)) return SurfaceY(X, Z) - S47Geo::StationBend(X, Z);
		const S47Geo::FXZ St = S47Geo::RoadToStation(X, Z);
		return S47Legs::StationGroundAt(St.X + Origin.X, St.Z + Origin.Z) - Origin.Y - S47Geo::STATION_DY();
	};
	S.Cover = [](double X, double Z)
	{
		if (S47Geo::InRect(S47Geo::STATION_PATCH, X, Z)) return RoadCover(X, Z);
		const S47Geo::FXZ St = S47Geo::RoadToStation(X, Z);
		if (!StationClear(St.X, St.Z, 0.15)) return 0.0;
		// thick along the fence, the paths and the track (just out of their way), raggedly
		return StationClear(St.X, St.Z, 0.9 + 1.4 * Ragged(X, Z)) ? 1.0 : 1.9;
	};
	S.Seam = [](double X, double Z)
	{
		if (S47Geo::InRect(S47Geo::STATION_PATCH, X, Z)) return RoadSeam(X, Z);
		const S47Geo::FXZ St = S47Geo::RoadToStation(X, Z);
		return StationClear(St.X, St.Z, -0.25) ? 0.7 : 0.0;
	};
	S.Spill = [](double X, double Z)
	{
		if (S47Geo::InRect(S47Geo::STATION_PATCH, X, Z)) return RoadSpill(X, Z);
		const S47Geo::FXZ St = S47Geo::RoadToStation(X, Z);
		return StationClear(St.X, St.Z, 0.75) ? 1.0 : 2.0;   // (pebbles by the paths, the pad and the fence)
	};
	S.Blocks = Posts;
	for (const FRect& C : Colliders) S.Blocks.Add(StationRect(C));
	for (const FRect& Z : Zones) S.Walks.Add(StationRect(Z));
	S.Ways = [](double X, double Z)
	{
		if (S47Geo::InRect(S47Geo::STATION_PATCH, X, Z)) return RoadWays(X, Z);
		const S47Geo::FXZ St = S47Geo::RoadToStation(X, Z);
		return StationClear(St.X, St.Z, -0.3) ? FString() : FString(TEXT("a path, the pad or the fence at the station"));
	};
	return S;
}

FS47FloraSpec OldRoadFlora(const TArray<FS47FloraRect>& Posts)
{
	// drive/OldRoad.ts: the brush, the grass and the stones, thick in the ditches either side of the
	// shoulders and along the fence (open where Kessler's gate stands in it: oldRoadLandmarks.ts),
	// none on the line of C's stakes; by the highway and in the town the road's own rules
	using namespace S47OldRoadLayout;
	const double GateS = SAt(3.2);
	FS47FloraSpec S;
	S.Name = TEXT("oldroad"); S.Seed = COVER_SEED + 1;
	S.Ground = [](double X, double Z) { return HeightAt(X, Z); };
	S.Blocks = Posts;
	auto ToRoadZ = [](double Z) { return Z - J().Z + S47RoadShape::DINER().Z; };
	auto Near = [](double Rx, double Rz)
	{
		const auto& T = S47RoadShape::TOWN();
		return FMath::Abs(Rx) < 26 || (Rx > T.West - 8 && Rx < T.East + 8 && Rz > S47RoadShape::DINER().Z - 60 && Rz < T.Z1 + 20);
	};
	S.Cover = [ToRoadZ, Near, GateS](double X, double Z)
	{
		const double Rx = X - J().X, Rz = ToRoadZ(Z);
		if (Near(Rx, Rz)) return RoadCover(Rx, Rz);
		const FNearest N = Nearest(X, Z);
		if (!FMath::IsFinite(N.D)) return 1.0;
		if (N.D < SHOULDER + 0.35) return 0.0;
		if (FMath::Abs(Z - C_LINE.Z) < 1.2 && X > C_LINE.X0 && X < C_LINE.X1) return 0.0;
		const double Jr = Ragged(X, Z);   // (the edges ragged, groundCover.edge)
		double D = N.D < SHOULDER + 0.8 + 1.2 * Jr ? 0.3 : N.D < 6.6 + 1.2 * Jr ? 1 : N.D < 10.4 ? 1.9 : 1;   // the ditches either side (profile)
		const double Fo = FMath::Abs(FMath::Abs(N.O) - FENCE);
		const int32 Side = N.O < 0 ? -1 : 1;
		const bool bGap = Side == -1 && N.S > GateS - 3.85 && N.S < GateS + 3.85;
		if (N.S > S0 + 40 && N.S < 700 && !bGap) { if (Fo < 0.22) return 0.0; if (Fo < 1.9) D *= 2.4; }
		return D;
	};
	// the seam at the shoulder's outer edge, the gravel spilt past it
	S.Seam = [ToRoadZ, Near](double X, double Z)
	{
		const double Rx = X - J().X, Rz = ToRoadZ(Z);
		if (Near(Rx, Rz)) return RoadSeam(Rx, Rz);
		const FNearest N = Nearest(X, Z);
		if (!FMath::IsFinite(N.D) || (FMath::Abs(Z - C_LINE.Z) < 1.2 && X > C_LINE.X0 && X < C_LINE.X1)) return 0.0;
		return N.D > SHOULDER - 0.45 && N.D < SHOULDER + 0.35 ? 0.8 : 0.0;
	};
	S.Spill = [ToRoadZ, Near](double X, double Z)
	{
		const double Rx = X - J().X, Rz = ToRoadZ(Z);
		if (Near(Rx, Rz)) return RoadSpill(Rx, Rz);
		return Nearest(X, Z).D < SHOULDER + 1 ? 4.0 : 1.0;
	};
	S.Ways = [](double X, double Z) { return Nearest(X, Z).D < HALF ? FString(TEXT("the old road")) : FString(); };
	return S;
}
}
