#include "S47RoadArea.h"

#include "S47RoadTerrain.h"

using namespace S47RoadTerrain;

namespace
{
	FS47Surface S(ES47SurfaceKind Kind, double Grip, double Top, double Rough) { FS47Surface O; O.Kind = Kind; O.Grip = Grip; O.Top = Top; O.Rough = Rough; return O; }
	struct FSurf { FS47Surface Asphalt, Shoulder, Gravel, Guard, Verge, Dirt; };
	const FSurf& SURF()
	{
		static const FSurf V = {
			S(ES47SurfaceKind::Asphalt, 1, 30, 0.05), S(ES47SurfaceKind::Gravel, 0.85, 22, 0.3), S(ES47SurfaceKind::Gravel, 0.75, 20, 0.4),
			S(ES47SurfaceKind::Gravel, 0.8, 20, 1), S(ES47SurfaceKind::Dirt, 0.6, 12, 0.6), S(ES47SurfaceKind::Dirt, 0.55, 10, 0.8),
		};
		return V;
	}
	// roadPlaces.ts trackRoute: every fourth of the track's points (the way in to the gate)
	TArray<FVector2D> TrackRoute()
	{
		const FTrack& T = Track();
		TArray<FVector2D> Out;
		for (int32 i = 0; i < T.Pts.Num(); i += 4) Out.Add(T.Pts[i]);
		return Out;
	}
}

FS47RoadArea::FS47RoadArea(const FVector& InOrigin)
	: Origin(InOrigin)
{
	// the headlights light the road's flood set in slots 0 to 2 (roadProps.HEADLIGHT_SLOTS); its
	// index among AS47Web's sets comes from the export (US47World)
	auto W = [this](double X, double Z) { return FVector(Origin.X + X, Origin.Y + SurfaceY(X, Z), Origin.Z + Z); };
	for (const FRoadStop& R : ROAD_STOPS()) Walks.Add({ R.MinX + Origin.X, R.MaxX + Origin.X, R.MinZ + Origin.Z, R.MaxZ + Origin.Z });
	Start = { W(START.X, 495), START.Heading };
	End = { W(END.X, END.Z), END.Heading };
	EndZone = { ENDZONE.MinX + Origin.X, ENDZONE.MaxX + Origin.X, ENDZONE.MinZ + Origin.Z, ENDZONE.MaxZ + Origin.Z };
	for (double Z = START.Z; Z <= 510; Z += 10) Route.Add(W(START.X, Z));
	Route.Add(W(-2.6, 513)); Route.Add(W(-4.2, 518.5));          // swing into the mouth of the track
	for (const FVector2D& P : TrackRoute()) if (P.X < -6) Route.Add(W(P.X, P.Y));
}

void FS47RoadArea::AddPlace(const TArray<FS47Box2>& InObstacles, const FS47Box2& Lot, double Y)
{
	for (const FS47Box2& B : InObstacles) Obstacles.Add(FS47Obstacle::Box(B.MinX, B.MaxX, B.MinZ, B.MaxZ));
	Lots.Add({ Lot, SURF().Gravel, Y });
}

const FS47RoadArea::FLot* FS47RoadArea::LotAt(double X, double Z) const
{
	for (const FLot& L : Lots) if (L.Box.Contains(X, Z)) return &L;
	return nullptr;
}

void FS47RoadArea::SetRoadClosed(bool bOn)
{
	if (bOn == bClosedOn) return;
	bClosedOn = bOn;
	// (the barricades' lamps are glow points of the road's own; the group carries the rest)
	ClosedGroup.SetVisible(bOn);
}

FS47Surface FS47RoadArea::Surface(double X, double Z) const
{
	if (const FLot* Lot = LotAt(X, Z)) return Lot->Surface;
	const double Lx = X - Origin.X, Lz = Z - Origin.Z, Ax = FMath::Abs(Lx);
	if (Lx > GUARD.X0 && Lx < GUARD.X1 && Lz > GUARD.Z0 && Lz < GUARD.Z1) return SURF().Guard;
	if (Ax <= HWY.Paved) return SURF().Asphalt;
	if (Ax <= HWY.Shoulder) return SURF().Shoulder;
	const double Od = OldNearest(Lx, Lz).D;
	if (Od <= OLD.Half + 0.2) return SURF().Asphalt;
	if (Od <= OLD.Shoulder + 0.6) return SURF().Shoulder;
	const FNear T = TrackNearest(Lx, Lz);
	if (T.D <= TrackHalf(T.S)) return SURF().Gravel;
	return Ax <= HWY.Margin || T.D <= TRK.Margin ? SURF().Verge : SURF().Dirt;
}

double FS47RoadArea::Height(double X, double Z) const
{
	if (const FLot* Lot = LotAt(X, Z)) return Lot->Y;
	return Origin.Y + SurfaceY(X - Origin.X, Z - Origin.Z);
}

void FS47RoadArea::Update(double Dt, double T, const FVector& TruckPos)
{
	(void)T;
	// (the reflectors' glints in the headlights are the road's glow points: drawn as they were exported)
	// the cattle guard rattles while the front axle is on it, from either side
	const double Lx = TruckPos.X - Origin.X, Lz = TruckPos.Z - Origin.Z;
	const bool bOn2 = Lx > GUARD.X0 - 1.8 && Lx < GUARD.X1 + 1.8 && Lz > GUARD.Z0 - 1.5 && Lz < GUARD.Z1 + 1.5;
	if (bOn2 && !bOnGuard && OnCattleGuard) OnCattleGuard(bHaveLast && Dt > 0 ? FVector::Dist(TruckPos, Last) / Dt : 0);
	bOnGuard = bOn2;
	Last = TruckPos; bHaveLast = true;
}
