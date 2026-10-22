#include "S47Geo.h"

#include "S47RoadShape.h"

namespace S47Geo
{
using namespace S47RoadShape;

const FAt& MOTEL_AT() { static const FAt A = { -2.5, MOTEL().Y + 0.08, 650 }; return A; }

const FRect& MOTEL_RECT()
{
	static const FRect R = { -50 + MOTEL_AT().X, -27.7 + MOTEL_AT().X, 1.5 + MOTEL_AT().Z, 70.8 + MOTEL_AT().Z };
	return R;
}

const TArray<FVector2D>& STATION_TRACK()
{
	// It is the road's survey track from 186 m out from the highway to the gate, in the station's terms.
	static const TArray<FVector2D> T = {
		{ -19.25, 38.5 }, { -18.7, 46.4 }, { -18.3, 58.3 }, { -17.6, 70.3 }, { -16.5, 82.2 }, { -15.1, 94.1 }, { -13.5, 106 },
		{ -11.8, 117.9 }, { -10, 129.7 }, { -8.1, 141.5 }, { -6.1, 153.3 }, { -3.9, 165.1 }, { -1.7, 176.9 }, { 0.5, 188.7 }, { 2.5, 200.5 }, { 4.4, 212.3 }, { 6.2, 224.2 },
		{ 7.9, 236 }, { 9.6, 247.9 }, { 11.2, 259.8 }, { 12.7, 271.7 }, { 14, 283.6 }, { 15.1, 295.5 }, { 15.9, 307.5 }, { 16.6, 319.4 }, { 17.2, 331.4 }, { 17.5, 343.4 },
		{ 17.7, 355.3 }, { 17.6, 367.3 }, { 17.4, 379.3 }, { 16.9, 391.3 }, { 16.2, 403.2 }, { 15.1, 415.2 }, { 13.7, 427.1 }, { 12.2, 439 }, { 11.7, 441.9 },
	};
	return T;
}

double STATION_TURN() { return STATION_TRUCK.Heading - END.Heading; }
FXZ RoadToStation(double X, double Z) { return { STATION_TRUCK.X - (Z - END.Z), STATION_TRUCK.Z + (X - END.X) }; }
FXZ StationToRoad(double X, double Z) { return { END.X + (Z - STATION_TRUCK.Z), END.Z - (X - STATION_TRUCK.X) }; }
double STATION_DY() { static const double V = STATION_GROUND - HLow(END.X, END.Z); return V; }

bool InRect(const FRect& R, double X, double Z, double M) { return X >= R.MinX + M && X <= R.MaxX - M && Z >= R.MinZ + M && Z <= R.MaxZ - M; }

double Outside(const FRect& R, double X, double Z)
{
	const double Dx = FMath::Max3(R.MinX - X, 0.0, X - R.MaxX), Dz = FMath::Max3(R.MinZ - Z, 0.0, Z - R.MaxZ);
	return FMath::Sqrt(Dx * Dx + Dz * Dz);
}

double StationBend(double X, double Z)
{
	return (HLow(X, Z) - HLow(END.X, END.Z)) * (1 - Smooth(-600, -560, X)) * (1 - Smooth(60, 160, FMath::Sqrt(FMath::Square(X - END.X) + FMath::Square(Z - END.Z))));
}

double StationLand(double X, double Z) { return HLow(X, Z) + STATION_DY() - StationBend(X, Z); }
}
