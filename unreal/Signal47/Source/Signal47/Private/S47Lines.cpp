// story/lines.ts
#include "S47Lines.h"

#include "S47Geo.h"
#include "S47RoadShape.h"
#include "S47RoadTerrain.h"

namespace
{
	using S47Lines::FMapPt;

	// world/stationLayout.ts (station-local): the transit's pier, and the old stakes along C from it
	constexpr double STATION_TRANSIT_X = 3, STATION_TRANSIT_Z = -4;
	const FMapPt STATION_STAKES_C(1, 0);
	// world/stationLayout.ts STATION_NINTH: the middle of the ninth cow where she lies
	constexpr double STATION_NINTH_X = -5.6, STATION_NINTH_Z = -8.55;
	// world/ServiceYard.ts YARD.vane: the centre of the B-12 vane board (site terms)
	constexpr double YARD_VANE_X = 9.9, YARD_VANE_Z = -18.35;
	// C on the old Roswell road (its area's local terms): the line z 0, the road on C within reach of it
	constexpr double C_OLD_Z = 0, C_OLD_REACH = 300;
	// C at STATION 01: within this of the transit the ground is C's, where the three lines meet
	constexpr double C_STATION_OWN = 80;

	FMapPt Unit(double X, double Z) { const double L = FMath::Sqrt(X * X + Z * Z); return FMapPt(X / L, Z / L); }
	FMapPt Site(double X, double Z) { return FMapPt(X + S47Geo::SARO_IN_ROAD.X, Z + S47Geo::SARO_IN_ROAD.Z); }
	FMapPt StationToRoad(double X, double Z) { const S47Geo::FXZ R = S47Geo::StationToRoad(X, Z); return FMapPt(R.X, R.Z); }
}

namespace S47Lines
{
	FMapPt TRANSIT() { static const FMapPt P = StationToRoad(STATION_TRANSIT_X, STATION_TRANSIT_Z); return P; }
	FMapPt B12() { return Site(YARD_VANE_X, YARD_VANE_Z); }

	const TMap<FString, FMapPt>& PLACES()
	{
		static const TMap<FString, FMapPt> P = []
		{
			const S47Geo::FAt& Motel = S47Geo::MOTEL_AT();
			TMap<FString, FMapPt> M = {
				{ TEXT("phone"), FMapPt(-15.5, 2064.6) }, { TEXT("phone2"), FMapPt(-15.5, 2064.6) }, { TEXT("repeat"), FMapPt(-13.8, 2071) },
				{ TEXT("lamps"), FMapPt(-7, 2146) }, { TEXT("boy"), FMapPt(32, 2036) }, { TEXT("regular"), FMapPt(S47RoadShape::DINER().X - 4.8, S47RoadShape::DINER().Z) },
				{ TEXT("cutout"), FMapPt(-6.4, 1450) }, { TEXT("car"), FMapPt(S47RoadTerrain::DEPUTY_CAR.X, S47RoadTerrain::DEPUTY_CAR.Z) },
				{ TEXT("lights"), FMapPt(-210, 30) }, { TEXT("cows"), FMapPt(S47RoadShape::HERD.X, S47RoadShape::HERD.Z) }, { TEXT("cows2"), FMapPt(S47RoadShape::HERD.X, S47RoadShape::HERD.Z) },
				{ TEXT("tv"), Site(-41.1 + Motel.X, 24.1 + Motel.Z) },
				{ TEXT("truck7m"), FMapPt(0, Site(0, 11.25 + Motel.Z).Y) },
				{ TEXT("ninth"), StationToRoad(STATION_NINTH_X, STATION_NINTH_Z) },
			};
			return M;
		}();
		// (the truck on B: the lights went out where B crosses the highway, story/truckOnB.ts; set once B is known)
		static const TMap<FString, FMapPt> Full = []
		{
			TMap<FString, FMapPt> M = P;
			M.Add(TEXT("truck7"), FMapPt(0, HighwayZ(TEXT('B'))));
			M.Add(TEXT("truck4"), FMapPt(0, HighwayZ(TEXT('B'))));
			return M;
		}();
		return Full;
	}

	const FStraight& LINES(TCHAR Id)
	{
		static const FStraight A = [] { const FMapPt T = TRANSIT(); return FStraight{ T, Unit(-15.5 - T.X, 2064.6 - T.Y), TEXT("Caliche"), 300 }; }();
		static const FStraight B = [] { const FMapPt T = TRANSIT(), V = B12(); return FStraight{ T, Unit(V.X - T.X, V.Y - T.Y), TEXT("B-12 at SARO"), 520 }; }();
		return Id == TEXT('A') ? A : B;
	}

	double Offset(TCHAR Id, double X, double Z)
	{
		const FStraight& L = LINES(Id);
		return (X - L.Through.X) * -L.Dir.Y + (Z - L.Through.Y) * L.Dir.X;
	}
	double DistTo(TCHAR Id, double X, double Z) { return FMath::Abs(Offset(Id, X, Z)); }
	double AlongFrom(TCHAR Id, double X, double Z)
	{
		const FStraight& L = LINES(Id);
		return (X - L.Through.X) * L.Dir.X + (Z - L.Through.Y) * L.Dir.Y;
	}
	FMapPt PointAt(TCHAR Id, double S)
	{
		const FStraight& L = LINES(Id);
		return FMapPt(L.Through.X + L.Dir.X * S, L.Through.Y + L.Dir.Y * S);
	}
	double HighwayZ(TCHAR Id)
	{
		const FStraight& L = LINES(Id);
		return L.Through.Y + (-L.Through.X / L.Dir.X) * L.Dir.Y;
	}

	// ---------- the areas' own terms ----------
	TOptional<FMapPt> ToMap(const FString& Area, double X, double Z)
	{
		if (Area == TEXT("saro")) return Site(X, Z);
		if (Area == TEXT("road") || Area == TEXT("diner")) return FMapPt(X, Z);
		if (Area == TEXT("station01")) return StationToRoad(X, Z);
		if (Area == TEXT("room6")) return Site(-41.07 + S47Geo::MOTEL_AT().X, 55 + S47Geo::MOTEL_AT().Z);
		return {};
	}
	TOptional<FMapPt> FromMap(const FString& Area, double X, double Z)
	{
		if (Area == TEXT("saro")) return FMapPt(X - S47Geo::SARO_IN_ROAD.X, Z - S47Geo::SARO_IN_ROAD.Z);
		if (Area == TEXT("road") || Area == TEXT("diner")) return FMapPt(X, Z);
		if (Area == TEXT("station01")) { const S47Geo::FXZ R = S47Geo::RoadToStation(X, Z); return FMapPt(R.X, R.Z); }
		return {};
	}
	FMapPt DirIn(const FString& Area, const FMapPt& D) { return Area == TEXT("station01") ? FMapPt(-D.Y, D.X) : D; }

	TOptional<FOnLine> LineAt(const FString& Area, double X, double Z)
	{
		if (Area == TEXT("station01"))
		{
			const FMapPt C = STATION_STAKES_C;
			if (FMath::Sqrt(FMath::Square(X - STATION_TRANSIT_X) + FMath::Square(Z - STATION_TRANSIT_Z)) <= C_STATION_OWN)
				return FOnLine{ TEXT("C"), FMath::Abs((X - STATION_TRANSIT_X) * -C.Y + (Z - STATION_TRANSIT_Z) * C.X), C };
		}
		if (Area == TEXT("roswell"))
		{
			const double D = FMath::Abs(Z - C_OLD_Z);
			return D <= C_OLD_REACH ? TOptional<FOnLine>(FOnLine{ TEXT("C"), D, FMapPt(1, 0) }) : TOptional<FOnLine>();
		}
		const TOptional<FMapPt> M = ToMap(Area, X, Z);
		if (!M) return {};
		TOptional<FOnLine> Best;
		for (const TCHAR Id : { TEXT('A'), TEXT('B') })
		{
			const double D = DistTo(Id, M->X, M->Y);
			if (D <= LINES(Id).Reach && (!Best || D < Best->Dist)) Best = FOnLine{ FString::Chr(Id), D, DirIn(Area, LINES(Id).Dir) };
		}
		return Best;
	}

	FString LineOfMark(const FString& Kind)
	{
		const FMapPt* P = PLACES().Find(Kind);
		const TOptional<FOnLine> L = P ? LineAt(TEXT("road"), P->X, P->Y) : TOptional<FOnLine>();
		return L ? L->Line : FString(TEXT("A"));
	}

	bool SelfTest(TArray<FString>& Lines)
	{
		bool bOk = true;
		auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s lines: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
		auto Near = [](double A, double B, double E = 1e-6) { return FMath::Abs(A - B) < E; };
		// (the values lines.ts gives in node)
		Check(Near(TRANSIT().X, -638.4) && Near(TRANSIT().Y, 509.75) && Near(B12().X, 33.9) && Near(B12().Y, -318.35), TEXT("the transit and B-12 on the map"));
		Check(Near(LINES('A').Dir.X, 0.3718847105514692, 1e-12) && Near(LINES('B').Dir.Y, -0.776357696511818, 1e-12), TEXT("A and B run from the transit"));
		Check(Near(HighwayZ('A'), 2103.2902793385774, 1e-8) && Near(HighwayZ('B'), -276.5939535921465, 1e-8), TEXT("where A and B cross the highway"));
		Check(Near(PLACES()[TEXT("tv")].X, -19.6) && Near(PLACES()[TEXT("tv")].Y, 374.1) && Near(PLACES()[TEXT("ninth")].X, -642.95) && Near(PLACES()[TEXT("ninth")].Y, 518.35)
			&& Near(PLACES()[TEXT("truck7m")].Y, 361.25) && Near(PLACES()[TEXT("regular")].X, -24.85), TEXT("the marked places"));
		const TOptional<FOnLine> Yard = LineAt(TEXT("saro"), 10, -10), Town = LineAt(TEXT("road"), -15, 2064), St = LineAt(TEXT("station01"), 50, 60), Old = LineAt(TEXT("roswell"), 100, 120), Room = LineAt(TEXT("room6"), 0, 0);
		Check(Yard && Yard->Line == TEXT("B") && Near(Yard->Dist, 5.3405787508090725, 1e-8), TEXT("the yard is on B"));
		Check(Town && Town->Line == TEXT("A") && Near(Town->Dist, 0.6872702886849993, 1e-8), TEXT("the payphone is on A"));
		Check(St && St->Line == TEXT("C") && Near(St->Dist, 64) && Old && Old->Line == TEXT("C") && Near(Old->Dist, 120), TEXT("the station and the old road are C's"));
		Check(!LineAt(TEXT("roswell"), 0, 400) && !LineAt(TEXT("road"), 400, 900), TEXT("between the lines"));
		Check(Room && Room->Line == TEXT("B") && Near(Room->Dist, 414.4102863532116, 1e-8), TEXT("room 6 is on B"));
		const TOptional<FMapPt> Back = FromMap(TEXT("station01"), -400, 600);
		Check(Back && Near(Back->X, -87.25, 1e-6) && Near(Back->Y, 234.4, 1e-6), TEXT("the station's terms from the map"));
		Check(LineOfMark(TEXT("lights")) == TEXT("B") && LineOfMark(TEXT("cutout")) == TEXT("A") && LineOfMark(TEXT("ninth")) == TEXT("B") && LineOfMark(TEXT("zzz")) == TEXT("A"), TEXT("the marks' lines"));
		return bOk;
	}
}
