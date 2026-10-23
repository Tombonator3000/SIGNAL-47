#include "S47Legs.h"

#include "S47Geo.h"
#include "S47RoadTerrain.h"

#include <limits>

using namespace S47Geo;
using namespace S47RoadTerrain;

namespace
{
	FS47Surface S(ES47SurfaceKind Kind, double Grip, double Top, double Rough) { FS47Surface O; O.Kind = Kind; O.Grip = Grip; O.Top = Top; O.Rough = Rough; return O; }
	struct FSurf { FS47Surface Asphalt, Yard, Shoulder, Gravel, Dirt; };
	const FSurf& SURF()
	{
		static const FSurf V = { S(ES47SurfaceKind::Asphalt, 1, 30, 0.05), S(ES47SurfaceKind::Asphalt, 1, 14, 0.08), S(ES47SurfaceKind::Gravel, 0.85, 22, 0.3),
			S(ES47SurfaceKind::Gravel, 0.75, 18, 0.4), S(ES47SurfaceKind::Dirt, 0.55, 10, 0.8) };
		return V;
	}
	bool InBox(const FS47Box2& B, double X, double Z) { return X >= B.MinX && X <= B.MaxX && Z >= B.MinZ && Z <= B.MaxZ; }
	FS47Box2 Box(double MinX, double MaxX, double MinZ, double MaxZ) { return { MinX, MaxX, MinZ, MaxZ }; }
	FVector V3(double X, double Z, const FVector& O = FVector::ZeroVector) { return FVector(X + O.X, O.Y, Z + O.Z); }
	double Wrap(double A) { return FMath::Atan2(FMath::Sin(A), FMath::Cos(A)); }
	using FPts = TArray<FVector2D>;
	TArray<FVector> ToWorld(const FPts& P, const FVector& O) { TArray<FVector> Out; for (const FVector2D& Q : P) Out.Add(V3(Q.X, Q.Y, O)); return Out; }

	// ---------- SARO: the pad, the ramp, the service road, the highway ----------
	// Sierra Motor Court's lot (MotelFront.ts, in its own numbers before MOTEL_AT): the part north
	// of the office where the old driveway came in, and the strip in front of the rooms
	FS47Box2 Lot(double X0, double X1, double Z0, double Z1) { return Box(X0 + MOTEL_AT().X, X1 + MOTEL_AT().X, Z0 + MOTEL_AT().Z, Z1 + MOTEL_AT().Z); }
	const TArray<FS47Box2>& MOTEL_LOT() { static const TArray<FS47Box2> V = { Lot(-41, -27.0, 1.5, 21.6), Lot(-38.25, -27.0, 21.6, 70.8) }; return V; }
	const double SARO_ROAD_FROM = SARO_FROM - SARO_IN_ROAD.Z;   // site z where the road's own stretch begins (corridors.ts)
	struct FRamp { double MinX, MaxX, Top, Foot; };
	constexpr FRamp SARO_RAMP = { 6.6, 13.0, 12.4, 16.6 };
	// where the truck may go: the pad, the ramp, the service road and its turning area, the
	// junction and the highway; the rest of the site is walls, railings and other people's ground
	const TArray<FS47Box2>& SARO_WAY()
	{
		static const TArray<FS47Box2> V = []
		{
			using namespace S47Legs;
			TArray<FS47Box2> W = {
				SARO_PAD, Box(6.6, 13.0, 12.4, 16.8), Box(6.8, 12.8, 16.0, 21.5), Box(5.3, 14.3, 15.9, 20.1),
				Box(1.0, 12.8, 18.5, 27.6), Box(-21.5, 6.5, 21.4, 27.6), Box(-29.5, -17.0, 17.0, 34.0),
				Box(SARO_HWY.X - 6.2, SARO_HWY.X + 4.2, SARO_HWY.Z0, SARO_HWY.Z1),
				// south of the site, between the road's fences (as in the road area), and the motel's lot
				Box(SARO_HWY.X - HWY.Fence + 0.6, SARO_HWY.X + HWY.Fence - 0.6, 80, SARO_HWY.Z1),
			};
			W.Append(MOTEL_LOT());
			return W;
		}();
		return V;
	}
	bool OnSaroRoad(double X, double Z)
	{
		if (Z > 22 && Z < 27 && X > -20 && X < 5.8) return true;                    // the straight run west
		if (X > 7.3 && X < 12.3 && Z > 15.9 && Z < 20.5) return true;               // down from the ramp
		const double D = FMath::Sqrt(FMath::Square(X - 5.8) + FMath::Square(Z - 20.5));
		return X >= 5.8 && Z >= 20.5 && D > 1.5 && D < 6.5;                          // the bend
	}

	// ---------- STATION 01 ----------
	struct FTrackAt { double D, S; };
	FTrackAt TrackAt(const FPts& Pts, double X, double Z)
	{
		double Best = std::numeric_limits<double>::infinity(), Sv = 0, Acc = 0;
		for (int32 i = 0; i < Pts.Num() - 1; ++i)
		{
			const double Ax = Pts[i].X, Az = Pts[i].Y, Bx = Pts[i + 1].X, Bz = Pts[i + 1].Y, Dx = Bx - Ax, Dz = Bz - Az, L = FMath::Sqrt(Dx * Dx + Dz * Dz);
			const double T = FMath::Min(1.0, FMath::Max(0.0, ((X - Ax) * Dx + (Z - Az) * Dz) / (L * L)));
			const double D = FMath::Square(Ax + Dx * T - X) + FMath::Square(Az + Dz * T - Z);
			if (D < Best) { Best = D; Sv = Acc + L * T; }
			Acc += L;
		}
		return { FMath::Sqrt(Best), Sv };
	}

	// world/Station01.ts Terrain: flat around the station and along the road, rolling further out.
	// The grid is fine near the station and coarse towards the horizon; surface() returns the height
	// of the drawn triangles (the heights are a Float32Array there, floats here).
	struct FStationTerrain
	{
		TArray<double> Xs, Zs;
		TArray<float> H;
		static TArray<double> Axis()
		{
			TArray<double> V = { 0 };
			double X = 0, Step = 4;
			while (X < 2600) { if (X >= 60) Step *= 1.2; X += Step; V.Add(FMath::RoundToDouble(X * 100) / 100); }
			TArray<double> Out;
			for (int32 i = V.Num() - 1; i >= 1; --i) Out.Add(-V[i]);
			Out.Append(V);
			return Out;
		}
		static double Height(double X, double Z)
		{
			const FXZ R = StationToRoad(X, Z);
			const double Land = StationLand(R.X, R.Z);
			if (InRect(STATION_PATCH, R.X, R.Z, -2)) return Land - 0.7;
			const FRect Pad = STATION_PAD;
			const double Flat = FMath::Max(1 - S47RoadShape::Smooth(70, 130, FMath::Sqrt(X * X + FMath::Square(Z - 8))), 1 - S47RoadShape::Smooth(0, 8, Outside(Pad, X, Z)));
			return Land + (STATION_GROUND - Land) * Flat;
		}
		static int32 Find(const TArray<double>& A, double V)
		{
			int32 Lo = 0, Hi = A.Num() - 2;
			while (Lo < Hi) { const int32 M = (Lo + Hi + 1) >> 1; if (A[M] <= V) Lo = M; else Hi = M - 1; }
			return Lo;
		}
		FStationTerrain()
		{
			Xs = Axis(); Zs = Axis();
			const int32 Nx = Xs.Num(), Nz = Zs.Num();
			H.SetNumZeroed(Nx * Nz);
			for (int32 j = 0; j < Nz; ++j) for (int32 i = 0; i < Nx; ++i) H[j * Nx + i] = float(Height(Xs[i], Zs[j]));
		}
		double Surface(double X, double Z) const
		{
			const int32 I = Find(Xs, X), J = Find(Zs, Z), Nx = Xs.Num();
			const double Fx = FMath::Min(1.0, FMath::Max(0.0, (X - Xs[I]) / (Xs[I + 1] - Xs[I])));
			const double Fz = FMath::Min(1.0, FMath::Max(0.0, (Z - Zs[J]) / (Zs[J + 1] - Zs[J])));
			const double Ha = H[J * Nx + I], Hb = H[(J + 1) * Nx + I], Hc = H[(J + 1) * Nx + I + 1], Hd = H[J * Nx + I + 1];
			// cell corners a (i, j), b (i, j+1), c (i+1, j+1), d (i+1, j); triangles abd and bcd
			if (Fx + Fz <= 1) return Ha + (Hd - Ha) * Fx + (Hb - Ha) * Fz;
			return Hc + (Hb - Hc) * (1 - Fx) + (Hd - Hc) * (1 - Fz);
		}
	};
	const FStationTerrain& StationTerrain() { static const FStationTerrain T; return T; }

	TMap<FString, TArray<FVector>>& RoadWays() { static TMap<FString, TArray<FVector>> W; return W; }
}

namespace S47Legs
{
FVector ORIGINS(const FString& Leg)
{
	if (Leg == TEXT("road") || Leg == TEXT("diner") || Leg == TEXT("roswell")) return FVector(8000, 0, 0);
	if (Leg == TEXT("station01")) return FVector(0, 0, 8000);
	return FVector::ZeroVector;   // saro, motel
}

double SARO_TO_ROAD() { return MOTEL_RECT().MaxZ + 64; }
double ROAD_TO_SARO() { return SARO_TO_ROAD() - 10 + SARO_IN_ROAD.Z; }
const FS47Box2& MOTEL_PARK() { static const FS47Box2 B = Lot(-40.5, -28.5, 3, 19.5); return B; }

double StationGroundAt(double X, double Z)
{
	const FVector O = ORIGINS(TEXT("station01"));
	return O.Y + StationTerrain().Surface(X - O.X, Z - O.Z);
}

double SaroFloorAt(double X, double Z)
{
	// (the truck ramp's slope: GROUND at its foot, floor level at its top; the pad is floor level)
	constexpr double GROUND = -0.6;
	if (X >= SARO_RAMP.MinX && X <= SARO_RAMP.MaxX && Z >= SARO_RAMP.Top && Z <= SARO_RAMP.Foot + 0.1)
	{
		const double K = FMath::Clamp((Z - SARO_RAMP.Foot) / (SARO_RAMP.Top - SARO_RAMP.Foot), 0.0, 1.0);
		return GROUND * (1 - K);
	}
	return 0;
}

// ---------- the hand-overs ----------
// SARO and the road share the highway; the station's terms are the road's turned a quarter
// round. The truck changes from the road to the station 330 m out from the gate (the road's track
// at arc length 330 of 605), and back again 8 m farther out; both areas draw the same track there.
TOptional<FExit> ExitFrom(const FString& Leg, double X, double Z, double Heading)
{
	const FVector O = ORIGINS(Leg);
	const double Lx = X - O.X, Lz = Z - O.Z;
	const FVector Road = ORIGINS(TEXT("road")), Station = ORIGINS(TEXT("station01"));
	if (Leg == TEXT("saro"))
	{
		if (Lz > SARO_TO_ROAD() && FMath::Abs(Lx - SARO_HWY.X) < 14) return FExit{ TEXT("road"), Lx + SARO_IN_ROAD.X + Road.X, Lz + SARO_IN_ROAD.Z + Road.Z, Heading };
		return {};
	}
	if (Leg == TEXT("road"))
	{
		if (Lz < ROAD_TO_SARO() && FMath::Abs(Lx) < 14) return FExit{ TEXT("saro"), Lx - SARO_IN_ROAD.X, Lz - SARO_IN_ROAD.Z, Heading };
		const FNear T = TrackNearest(Lx, Lz);
		if (T.D < 14 && T.S >= HANDOFF)
		{
			const FXZ St = RoadToStation(Lx, Lz);
			return FExit{ TEXT("station01"), St.X + Station.X, St.Z + Station.Z, Wrap(Heading + STATION_TURN()) };
		}
		return {};
	}
	if (Leg == TEXT("station01"))
	{
		if (Lz < 150) return {};
		const FXZ R = StationToRoad(Lx, Lz);
		const FNear T = TrackNearest(R.X, R.Z);
		if (T.D < 14 && T.S < HANDOFF - 8) return FExit{ TEXT("road"), R.X + Road.X, R.Z + Road.Z, Wrap(Heading - STATION_TURN()) };
		return {};
	}
	return {};
}

// The autopilot's way on the road (world), towards the next area: the southbound lane and the
// survey track out, the track and the northbound lane back.
const TArray<FVector>& RoadRoute(const FString& Next, double Lz)
{
	// Evening commute starts south of town, beyond the old diner's exit route.
	// Stay in the northbound lane through Caliche instead of aiming diagonally across its buildings.
	const double DinerZ = S47RoadShape::DINER().Z;
	if (Lz > DinerZ + 20 && (Next == TEXT("saro") || Next == TEXT("diner")))
	{
		const FString TownKey = Next + TEXT(":north-through-town");
		if (const auto* Hit = RoadWays().Find(TownKey)) return *Hit;
		FPts TownPoints;
		const double End = Next == TEXT("saro") ? -160 : DinerZ + 25;
		for (double Z = S47RoadShape::TOWN().Z1 + 80; Z >= End; Z -= 10) TownPoints.Add({1.8,Z});
		if (Next == TEXT("diner")) TownPoints.Append({{1.8,DinerZ+23},{1.1,DinerZ+17},{-0.8,DinerZ+11},{-3.5,DinerZ+7},{-6,DinerZ+6},{-9,DinerZ+6}});
		return RoadWays().Add(TownKey,ToWorld(TownPoints,ORIGINS(TEXT("road"))));
	}
	// from the south (the diner, the truck any time): off the lot, into the northbound lane, north
	const bool bSouth = Lz > 540 && (Next == TEXT("station01") || Next == TEXT("saro"));
	const FString Key = bSouth ? Next + TEXT("S") : Next;
	if (const TArray<FVector>* Hit = RoadWays().Find(Key)) return *Hit;
	FPts Trk;
	const FTrack& T = Track();
	for (int32 i = 0; i < T.Pts.Num(); ++i) if (i % 2 == 0 && T.Pts[i].X < -6) Trk.Add(T.Pts[i]);
	const double DZ = S47RoadShape::DINER().Z;
	FPts Pts;
	if (bSouth)
	{
		Pts.Append({ { -9.0, DZ + 6 }, { -7.5, DZ + 5.4 }, { -5.6, DZ + 3.6 }, { -3.8, DZ }, { -1.0, DZ - 8 }, { 1.8, DZ - 20 } });
		for (double Z = DZ - 40; Z >= (Next == TEXT("saro") ? -160 : 530); Z -= 20) Pts.Add({ 1.8, Z });
		if (Next == TEXT("station01")) { Pts.Append({ { 0.4, 521 }, { -2.6, 517 }, { -4.2, 518.5 } }); Pts.Append(Trk); }
	}
	else if (Next == TEXT("station01"))
	{
		for (double Z = -140; Z <= 510; Z += 20) Pts.Add({ -1.8, Z });
		Pts.Append({ { -2.6, 513 }, { -4.2, 518.5 } });
		Pts.Append(Trk);
	}
	else if (Next == TEXT("diner"))
	{
		// south past the track, then right onto the diner's lot, south of the rig and the sign
		for (double Z = -140; Z <= DZ - 20; Z += 20) Pts.Add({ -1.8, Z });
		// (left facing the diner's door, clear of where the player gets out: diner x 11, z 6)
		Pts.Append({ { -2.0, DZ - 10 }, { -2.6, DZ - 4 }, { -3.8, DZ }, { -5.6, DZ + 3.6 }, { -7.5, DZ + 5.4 }, { -9.0, DZ + 6 } });
	}
	else if (Next == TEXT("saro"))
	{
		for (int32 i = Trk.Num() - 1; i >= 0; --i) Pts.Add(Trk[i]);
		Pts.Append({ { -4, 519.6 }, { -0.6, 517.4 }, { 1.6, 511 } });
		for (double Z = 495; Z >= -160; Z -= 20) Pts.Add({ 1.8, Z });
	}
	return RoadWays().Add(Key, ToWorld(Pts, ORIGINS(TEXT("road"))));
}

FString LegOf(const FString& P) { return P == TEXT("diner") ? FString(TEXT("road")) : P == TEXT("motel") ? FString(TEXT("saro")) : P; }

TArray<FString> LegsBetween(const FString& From, const FString& To)
{
	const TArray<FString> Chain = { TEXT("saro"), TEXT("road"), TEXT("station01") };
	const int32 A = Chain.IndexOfByKey(LegOf(From)), B = Chain.IndexOfByKey(LegOf(To));
	if (A < 0 || B < 0) return { From, To };
	TArray<FString> Out;
	if (A <= B) for (int32 i = A; i <= B; ++i) Out.Add(Chain[i]);
	else for (int32 i = A; i >= B; --i) Out.Add(Chain[i]);
	return Out;
}

FString BoundOn(const FString& Leg, double X, double Z, double Heading, bool bReversing)
{
	const FVector O = ORIGINS(Leg);
	const double Lx = X - O.X, Lz = Z - O.Z, K = bReversing ? -1 : 1;
	const double Fx = -FMath::Sin(Heading) * K, Fz = -FMath::Cos(Heading) * K;
	// the station's terms are the road's turned a quarter round: its -z is the track's way in
	if (Leg == TEXT("station01")) return Fz < -0.3 ? FString(TEXT("station01")) : FString();
	if (Leg == TEXT("saro")) return Lz > 40 && Fz < -0.5 ? FString(TEXT("saro")) : FString();
	if (Leg != TEXT("road")) return FString();
	const FNear T = TrackNearest(Lx, Lz);
	if (T.D < 10 && T.S > 12) return Fx < -0.3 ? FString(TEXT("station01")) : FString();   // the track runs west to the gate
	if (Fz < -0.5) return Lz < 500 ? FString(TEXT("saro")) : FString();
	if (Fz > 0.5) return Lz > 540 ? FString(TEXT("diner")) : FString();
	return FString();
}
}

// ---------- SaroDrive ----------

FS47SaroDrive::FS47SaroDrive(const TArray<FS47Box2>& Colliders, const TArray<FS47Obstacle>& Road)
{
	using namespace S47Legs;
	EndZone = Box(0, 0, 0, 0);    // arrivals are the trip's business (World.ts), not the controller's
	Park = SARO_PAD;
	MotelPark = MOTEL_PARK();
	// only what stands along the way matters to the truck
	for (const FS47Box2& C : Colliders)
	{
		const bool bNear = SARO_WAY().ContainsByPredicate([&](const FS47Box2& W) { return C.MaxX > W.MinX - 4 && C.MinX < W.MaxX + 4 && C.MaxZ > W.MinZ - 4 && C.MinZ < W.MaxZ + 4; });
		if (bNear) Obstacles.Add(FS47Obstacle::Box(C.MinX, C.MaxX, C.MinZ, C.MaxZ));
	}
	for (const FS47Obstacle& C : Road) Obstacles.Add(FS47Obstacle::Circle(C.X - SARO_IN_ROAD.X, C.Z - SARO_IN_ROAD.Z, C.R));
	// arcs of 6 m radius (the truck turns no tighter than about 5.3 m) round the posts at the foot
	// of the ramp and at the junction: centre, from and to angle (0 is +x, 90 is +z)
	auto Arc = [](double Cx, double Cz, double A0, double A1)
	{
		FPts P;
		for (int32 i = 0; i <= 9; ++i) { const double A = (A0 + (A1 - A0) * i / 9) * UE_DOUBLE_PI / 180; P.Add({ Cx + FMath::Cos(A) * 6, Cz + FMath::Sin(A) * 6 }); }
		return P;
	};
	const FPts Ramp = { { 9.0, 9 }, { 9.4, 11.5 }, { 10.2, 14 }, { 11.0, 16.2 }, { 11.3, 17.8 } };
	FPts Out = Ramp;
	Out.Append(Arc(5.3, 18.5, 0, 90));
	Out.Append({ { 1, 24.5 }, { -6, 24.5 }, { -13, 24.5 } });
	FPts A2 = Arc(-19.8, 30.5, -90, -180); A2.Pop();
	Out.Append(A2);
	Out.Append({ { -25.8, 31.5 }, { -25.8, 38 }, { -25.8, 46 } });
	for (double Z = 60; Z <= SARO_HWY.Z1 - 20; Z += 20) Out.Add({ -25.8, Z });
	FPts Back;
	for (double Z = SARO_HWY.Z1 - 20; Z >= 46; Z -= 20) Back.Add({ -22.2, Z });
	Back.Append({ { -22.2, 38 }, { -22.2, 31.5 } });
	Back.Append(Arc(-16.2, 30.5, 180, 270));
	Back.Append({ { -10, 24.5 }, { -3, 24.5 }, { 1, 24.5 } });
	Back.Append(Arc(5.3, 18.5, 90, 0));
	for (int32 i = Ramp.Num() - 2; i >= 1; --i) Back.Add(Ramp[i]);   // [...ramp].reverse().slice(1, -1)
	Back.Append({ { 9.2, 10.5 }, { 9.0, 8.4 } });
	// to the motel's lot: down the southbound lane and right, in front of the office; or, coming
	// from the south in the northbound lane, past the lot and left into it
	const FAt& A = MOTEL_AT();
	FPts Motel;
	for (const FVector2D& P : Out) if (P.Y < A.Z - 20) Motel.Add(P);
	Motel.Append({ { -25.8, A.Z - 14 }, { -26.6, A.Z - 6 }, { -28.3, A.Z - 0.5 }, { -30.8, A.Z + 4.5 }, { -33.6, A.Z + 8 }, { -37.3, A.Z + 10.2 } });
	FPts MotelS;
	for (double Z = SARO_TO_ROAD() + 10; Z > A.Z + 30; Z -= 20) MotelS.Add({ -22.2, Z });
	MotelS.Append({ { -22.2, A.Z + 22 }, { -22.2, A.Z + 12 }, { -22.8, A.Z + 6.5 }, { -25, A.Z + 4.2 }, { -28.3, A.Z + 5 }, { -31.2, A.Z + 7.6 }, { -34.4, A.Z + 9.5 }, { -37.3, A.Z + 10.2 } });
	const FVector O = ORIGINS(TEXT("saro"));
	Routes.Add(TEXT("road"), ToWorld(Out, O));
	Routes.Add(TEXT("park"), ToWorld(Back, O));
	Routes.Add(TEXT("motel"), ToWorld(Motel, O));
	Routes.Add(TEXT("motelS"), ToWorld(MotelS, O));
}

FS47Surface FS47SaroDrive::Surface(double X, double Z) const
{
	using namespace S47Legs;
	if (MOTEL_LOT().ContainsByPredicate([&](const FS47Box2& B) { return InBox(B, X, Z); }) && X < MOTEL_RECT().MaxX) return SURF().Yard;
	const double Ax = FMath::Abs(X - SARO_HWY.X);
	if (Z > SARO_ROAD_FROM) return Ax <= HWY.Paved ? SURF().Asphalt : Ax <= HWY.Shoulder ? SURF().Shoulder : SURF().Dirt;
	if (Ax <= SARO_HWY.Half + 0.6) return SURF().Asphalt;
	if (InBox(SARO_PAD, X, Z) || (X >= SARO_RAMP.MinX && X <= SARO_RAMP.MaxX && Z >= SARO_RAMP.Top && Z <= SARO_RAMP.Foot) || OnSaroRoad(X, Z)) return SURF().Yard;
	return SURF().Dirt;
}

double FS47SaroDrive::Height(double X, double Z) const
{
	using namespace S47Legs;
	if (InBox(SARO_PAD, X, Z) || (X >= SARO_RAMP.MinX && X <= SARO_RAMP.MaxX && Z >= SARO_RAMP.Top && Z <= SARO_RAMP.Foot)) return SaroFloorAt(X, Z);
	if (InRect(MOTEL_RECT(), X, Z)) return MOTEL_AT().Y;
	// south of the site's own highway, the road's land (corridors.ts)
	if (Z > SARO_ROAD_FROM) return SurfaceY(X + SARO_IN_ROAD.X, Z + SARO_IN_ROAD.Z);
	if (FMath::Abs(X - SARO_HWY.X) <= SARO_HWY.Half + 2) return -0.58;
	return -0.6;
}

bool FS47SaroDrive::Blocked(double X, double Z)
{
	return !SARO_WAY().ContainsByPredicate([&](const FS47Box2& W) { return InBox(W, X, Z); });
}

// ---------- StationDrive ----------

FS47StationDrive::FS47StationDrive(const TArray<FS47Box2>& Colliders, const TArray<FS47Obstacle>& Road)
	: O(S47Legs::ORIGINS(TEXT("station01")))
{
	EndZone = Box(0, 0, 0, 0);
	Park = Box(STATION_PAD.MinX + O.X, STATION_PAD.MaxX + O.X, STATION_PAD.MinZ + O.Z, STATION_PAD.MaxZ + O.Z);
	for (const FS47Box2& C : Colliders) if (C.MaxZ > O.Z + 20 || (C.MaxX > O.X - 30 && C.MinX < O.X - 8)) Obstacles.Add(FS47Obstacle::Box(C.MinX, C.MaxX, C.MinZ, C.MaxZ));
	for (const FS47Obstacle& C : Road) { const FXZ P = RoadToStation(C.X, C.Z); Obstacles.Add(FS47Obstacle::Circle(P.X + O.X, P.Z + O.Z, C.R)); }
	const FPts& T = STATION_TRACK();
	FPts Out = { { -19.25, 36.5 } };
	for (int32 i = 1; i < T.Num(); ++i) Out.Add(T[i]);
	FPts Back;
	for (int32 i = T.Num() - 1; i >= 0; --i) if (T[i].Y > 40) Back.Add(T[i]);
	Back.Append({ { -19.3, 39 }, { -19.25, 33 } });
	Routes.Add(TEXT("road"), ToWorld(Out, O));
	Routes.Add(TEXT("park"), ToWorld(Back, O));
}

FS47Surface FS47StationDrive::Surface(double X, double Z) const
{
	const double Lx = X - O.X, Lz = Z - O.Z;
	if (InRect(STATION_PAD, Lx, Lz) || TrackAt(STATION_TRACK(), Lx, Lz).D <= 2.6) return SURF().Gravel;
	return SURF().Dirt;
}

// on the road's stretch (corridors.ts) the road's own surface, as the station draws it
double FS47StationDrive::Height(double X, double Z) const
{
	const double Lx = X - O.X, Lz = Z - O.Z;
	const FXZ R = StationToRoad(Lx, Lz);
	if (InRect(STATION_PATCH, R.X, R.Z) || (TrackNearest(R.X, R.Z).D < 4 && Lz > STATION_PAD.MaxZ)) return O.Y + SurfaceY(R.X, R.Z) + STATION_DY() - StationBend(R.X, R.Z);
	return S47Legs::StationGroundAt(X, Z);
}

// the truck stays out of the compound and near the track
bool FS47StationDrive::Blocked(double X, double Z)
{
	const double Lx = X - O.X, Lz = Z - O.Z;
	if (Lz < 26.2) return true;
	return !InRect(STATION_PAD, Lx, Lz) && TrackAt(STATION_TRACK(), Lx, Lz).D > 14;
}
