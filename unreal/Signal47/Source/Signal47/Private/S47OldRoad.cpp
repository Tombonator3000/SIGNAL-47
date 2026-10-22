#include "S47OldRoad.h"

#include "S47OldRoadLayout.h"
#include "S47RoadTerrain.h"

#include <limits>

using namespace S47OldRoadLayout;

namespace
{
	constexpr double INF = std::numeric_limits<double>::infinity();
	FS47Surface S(ES47SurfaceKind Kind, double Grip, double Top, double Rough) { FS47Surface O; O.Kind = Kind; O.Grip = Grip; O.Top = Top; O.Rough = Rough; return O; }
	struct FSurf { FS47Surface Asphalt, Shoulder, Verge, Dirt; };
	const FSurf& SURF()
	{
		static const FSurf V = { S(ES47SurfaceKind::Asphalt, 1, 30, 0.08), S(ES47SurfaceKind::Gravel, 0.85, 22, 0.3), S(ES47SurfaceKind::Dirt, 0.6, 12, 0.6), S(ES47SurfaceKind::Dirt, 0.55, 10, 0.8) };
		return V;
	}
	/** The highway at the diner is open to the truck this far either way from mile 0. */
	constexpr double HWY_OPEN = 260;
	/** Road-local z of a local point, and local z of a road-local one (x: local = road-local + J.x). */
	double ToRoadZ(double Z) { return Z - J().Z + S47RoadShape::DINER().Z; }
	double FromRoadZ(double Rz) { return Rz + J().Z - S47RoadShape::DINER().Z; }
}

const TArray<int32>& FS47OldRoad::OLD_HEADLIGHTS() { static const TArray<int32> V = { 0, 1, 2 }; return V; }

FS47OldRoad::FS47OldRoad(const FVector& RoadOrigin)
{
	Origin = RoadOrigin + FVector(-J().X, 0, S47RoadShape::DINER().Z - J().Z);
	EndZone = { 1e9, 1e9, 1e9, 1e9 };   // never reached: the chapter ends the drive on C, not in a zone
	// ---------- world terms ----------
	auto W = [this](double X, double Z) { return FVector(Origin.X + X, Origin.Y + HeightAt(X, Z), Origin.Z + Z); };
	Start = { W(START().X, START().Z), START().Heading };
	// out of the diner's lot (diner x 11 to 16, World.ts parks it there), over the highway, into
	// the right lane (to the south of the centreline, the road going east)
	const double Lead[3][2] = { { -4.5, 4 }, { -1.5, 3 }, { 2.5, 2.4 } };
	for (const auto& R : Lead) Route.Add(W(J().X + R[0], FromRoadZ(S47RoadShape::DINER().Z + R[1])));
	for (double Sv = S0 + 10; Sv <= 300; Sv += Sv < S0 + 200 ? 8 : 20) { const FBeside B = Beside(Sv, 1.6); Route.Add(W(B.X, B.Z)); }
	Routes.Add(TEXT("park"), Route);
}

void FS47OldRoad::AddPlace(const TArray<FS47Box2>& InObstacles, const FS47Box2& Lot, double Y)
{
	for (const FS47Box2& B : InObstacles) Obstacles.Add(FS47Obstacle::Box(B.MinX, B.MaxX, B.MinZ, B.MaxZ));
	Lots.Add({ Lot, Y });
}

const FS47OldRoad::FLot* FS47OldRoad::LotAt(double X, double Z) const
{
	for (const FLot& L : Lots) if (L.Box.Contains(X, Z)) return &L;
	return nullptr;
}

FS47OldRoad::FWhere FS47OldRoad::Where(double X, double Z) const
{
	const FNearest N = Nearest(X - Origin.X, Z - Origin.Z);
	const double Sv = N.D == INF ? S0 : N.S;
	return { Sv, N.O, N.D, 8.15 + Sv / 1609.34 };
}

double FS47OldRoad::CrossY() const { return Origin.Y + Path()[I0()].Y; }

FVector FS47OldRoad::Lane(double Sv, double O) const
{
	const FBeside B = Beside(Sv, O);
	return FVector(B.X + Origin.X, Origin.Y + HeightAt(B.X, B.Z), B.Z + Origin.Z);
}

FS47OldRoad::FPose FS47OldRoad::PoseAt(double Mi) const
{
	const double Sv = (Mi - 8.15) * 1609.34;
	const FVector A = Lane(Sv), B = Lane(Sv + 4);
	return { A, FMath::Atan2(-(B.X - A.X), -(B.Z - A.Z)) };
}

double FS47OldRoad::PastC(double X, double Z) const { (void)X; return -(Z - Origin.Z - C_LINE.Z); }

FS47Surface FS47OldRoad::Surface(double X, double Z) const
{
	if (LotAt(X, Z)) return SURF().Shoulder;
	const double Lx = X - Origin.X, Lz = Z - Origin.Z, Ax = FMath::Abs(Lx - J().X);
	const S47RoadTerrain::FHwy& HWY = S47RoadTerrain::HWY;
	if (Ax <= HWY.Shoulder) return Ax <= HWY.Paved ? SURF().Asphalt : SURF().Shoulder;
	const FNearest N = Nearest(Lx, Lz);
	const double A = FMath::Abs(N.O);
	if (N.D == INF && Ax <= HWY.Margin) return SURF().Verge;
	if (N.D == INF) return SURF().Dirt;
	if (A <= HALF + 0.2) return SURF().Asphalt;
	if (A <= SHOULDER + 0.6) return SURF().Shoulder;
	return A <= 10 ? SURF().Verge : SURF().Dirt;
}

double FS47OldRoad::Height(double X, double Z) const
{
	if (const FLot* L = LotAt(X, Z)) return L->Y;
	return Origin.Y + HeightAt(X - Origin.X, Z - Origin.Z);
}

bool FS47OldRoad::Blocked(double X, double Z)
{
	const double Lx = X - Origin.X, Lz = Z - Origin.Z;
	if (FMath::Abs(Lx - J().X) < S47RoadTerrain::HWY.Fence - 2.5)
	{
		if (FMath::Abs(ToRoadZ(Lz) - S47RoadShape::DINER().Z) < HWY_OPEN) return false;
		bHitBack = true; return true;
	}
	const FNearest N = Nearest(Lx, Lz);
	if (N.D == INF || FMath::Abs(N.O) > FENCE - 2.5) return true;
	return N.S > S1 - 300;
}
