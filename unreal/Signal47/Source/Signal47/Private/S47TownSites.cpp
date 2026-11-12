#include "S47TownSites.h"
#include "S47Breather.h"
#include "S47EventPlaces.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47Visitors.h"
#include "Components/PointLightComponent.h"
#include "S47Legs.h"
#include "S47Main.h"
#include "S47OpenLand.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47TownSites, Log, All);

namespace S47TownSitesLocal
{
	// ---- THE TABLE: where each temporary box stands. Move one by changing its row. ----
	// X, Z: the middle of the site, road-local metres (x east, z south; the highway runs down x = 0, Caliche's street is z 2018 to
	// 2300). Yaw: degrees the whole site is turned, clockwise seen from above with north up (0: as drawn below, square to the
	// highway). The reasons are in the report of 9 November and in the comments under each site below.
	struct FSpot { const TCHAR* Id; double X, Z, Yaw; };
	const FSpot Spots[] = {
		{ TEXT("trailers"), -72, 1905, 0 },  // west of the highway, just north of the Mesa Diner: the first thing coming in from SARO
		{ TEXT("school"), 48, 2245, 0 },     // east side at the south end of town, behind the power line, facing the highway
		{ TEXT("grange"), -40, 2272, 0 },    // west side at the south end, past the last house, across from the school
		{ TEXT("baca"), 820, 1350, 0 },      // NEIGHBOR-20261108: the old rancher's place, alone on the flat under the east rim
		// N1110 (Caliche history): the Breather's tour entrance, a landmark and not a box: west of the town's fence (x -42), behind the
		// laundromat and the Plaza, facing the street. Its mouth looks east, its gate is 40 m from Main Street's west walk.
		{ TEXT("breather"), -62, 2146, 0 },
	};
	// Pale grey, as a model maker's grey box is, with the parts a shade apart so the shapes read.
	constexpr uint32 Grey = 0xb4b0a6, GreyDark = 0x8e8a82, GreyLight = 0xc9c5bb, Board = 0x2b2a27, Steel = 0x8b8e8a, CarA = 0x6f4a3c, CarB = 0x4d5866;
	const TCHAR* const CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* const CylPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* const LitPath = TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque");
	constexpr double StripM = 1.5;

	S47TownSites::FPart Box(double X, double Z, double W, double D, double H, uint32 Colour, bool bSolid = true)
	{
		S47TownSites::FPart P; P.X = X; P.Z = Z; P.W = W; P.D = D; P.H = H; P.Colour = Colour; P.bSolid = bSolid; return P;
	}
	// The shapes on each site's own ground, before it is turned: x east, z south from the middle given in Spots.
	void Shape(S47TownSites::FSite& S)
	{
		using namespace S47TownSites;
		if (S.Id == TEXT("trailers"))
		{
			// A highway trailer court of the sixties: a gravel lane in from the highway, four single-wides each side of it end on to
			// the lane (14 by 60 to 70 feet, a little over 3 m to the roof on their skirting), two cars, and the court's own water
			// tower at the far end. The lane itself is open ground (the place for lights over it, and marks).
			S.Sign = TEXT("TRAILER PARK (TEMP)"); S.Title = TEXT("the trailer park by the highway"); S.MapName = TEXT("Trailer Park (temp)");
			S.Traits = ES47PlaceTrait::Open | ES47PlaceTrait::People | ES47PlaceTrait::Road | ES47PlaceTrait::Lot;
			S.Radius = 40; S.Up = 22; S.NameSide = -1;
			S.PlaceX = 12; S.PlaceZ = 0; S.EntryX = 58; S.EntryZ = 0;
			const double Xs[] = { 38, 20, 2, -16 }, Lengths[] = { 18.3, 20.1, 16.5, 19.2 };
			for (int32 I = 0; I < 4; ++I)
			{
				const uint32 C = I % 2 ? GreyLight : Grey;
				S.Parts.Add(Box(Xs[I], -6 - Lengths[I] * .5, 4.3, Lengths[I], 3.3, C));   // north of the lane
				S.Parts.Add(Box(Xs[I], 6 + Lengths[(I + 1) % 4] * .5, 4.3, Lengths[(I + 1) % 4], 3.3, I % 2 ? Grey : GreyLight)); // south
			}
			S.Parts.Add(Box(29, -11, 1.8, 4.6, 1.45, CarA));
			S.Parts.Add(Box(11, 11, 1.8, 4.6, 1.45, CarB));
			// The water tower: four legs, a tank on them, a ladder's worth of height. Only the legs' square stops anyone.
			for (const double Lx : { -2.2, 2.2 }) for (const double Lz : { -2.2, 2.2 }) S.Parts.Add(Box(-40 + Lx, Lz, .3, .3, 14, Steel, false));
			{ FPart Base = Box(-40, 0, 4.8, 4.8, 1, Steel); Base.H = .2; S.Parts.Add(Base); } // the legs' footing, for the colliders and the sheet
			{ FPart Tank = Box(-40, 0, 6.4, 6.4, 5, GreyLight, false); Tank.Lift = 14; Tank.bRound = true; S.Parts.Add(Tank); }
			// The sign by the highway: two posts and a board, end on to the road so both lanes read it.
			S.Parts.Add(Box(58, -1.3, .14, .14, 2.8, GreyDark, false));
			S.Parts.Add(Box(58, 1.3, .14, .14, 2.8, GreyDark, false));
			{ FPart B = Box(58, 0, .1, 3.2, 1.1, Board, false); B.Lift = 1.6; S.Parts.Add(B); }
			S.Words.Add({ TEXT("TRAILER PARK\n(TEMP)"), 58.07, 0, 2.15, 90, .4 });
			S.Words.Add({ TEXT("TRAILER PARK\n(TEMP)"), 57.93, 0, 2.15, 270, .4 });
			// And big on the tank, toward the highway, where a driver sees it first.
			S.Words.Add({ TEXT("TRAILER PARK\n(TEMP)"), -36.75, 0, 16.5, 90, .75 });
		}
		else if (S.Id == TEXT("school"))
		{
			// A one-storey school of the fifties: a long classroom wing along the highway with its front door in the middle, and the
			// gymnasium behind it, taller, as the consolidated rural schools were built. A flagpole on the front lawn.
			S.Sign = TEXT("CALICHE SCHOOL (TEMP)"); S.Title = TEXT("the school at the south end of town"); S.MapName = TEXT("School (temp)");
			S.Traits = ES47PlaceTrait::Town | ES47PlaceTrait::Open | ES47PlaceTrait::Lot;
			S.Radius = 30; S.Up = 26; S.NameSide = 1;
			S.PlaceX = -26; S.PlaceZ = 0; S.EntryX = -18.5; S.EntryZ = 0;
			S.Parts.Add(Box(-12.5, 0, 11, 60, 4.2, Grey));          // the classroom wing: x 30 to 41, z 2215 to 2275
			S.Parts.Add(Box(5, -2, 24, 30, 8, GreyDark));            // the gym: x 41 to 65
			{ FPart Pole = Box(-25, 0, .14, .14, 9, Steel, false); Pole.bRound = true; Pole.bKeep = true; S.Parts.Add(Pole); }   // the flagpole: 4 m before the model's front steps (stage 2)
			S.Words.Add({ S.Sign, -18.06, 0, 3.1, 270, .9 });
			S.Words.Add({ TEXT("GYM (TEMP)"), -6.94, -2, 6.4, 270, .8 });
			// SCHOOL-V2-20261112 stage 2: the real school (Docs/Design/World/CalicheSchool_V2, ~/Assets/signal47-free/own/caliche_school),
			// 28 x 24 m, a basement under its north half. Its front door is in the middle of its south wall: turned a quarter clockwise it
			// faces the highway, the front steps at x 25.5 and the flagpole before them. The mesh's pivot is the middle of its base, 3.6 m
			// under the ground floor; the yard lies 0.6 m under the floor (the spec's Z_TERRAIN), so the pivot goes 3 m into the ground.
			S.Model = TEXT("/Game/Free/Town/caliche_school/StaticMeshes/caliche_school.caliche_school");
			S.ModelX = -8; S.ModelZ = 0; S.ModelYaw = 90; S.ModelLift = 3.0;
		}
		else if (S.Id == TEXT("baca"))
		{
			// An old homestead a long drive out: a flat-roofed adobe house, a shed, a stock tank, nothing else for a mile. Eloy Baca's
			// (S47Visitors): he has watched the sky from this porch for forty years, and is not always right about what he saw.
			S.Sign = TEXT("BACA PLACE (TEMP)"); S.Title = TEXT("the old Baca place under the east rim"); S.MapName = TEXT("Baca Place (temp)");
			S.Traits = ES47PlaceTrait::Farm | ES47PlaceTrait::Open | ES47PlaceTrait::Far | ES47PlaceTrait::People;
			S.Radius = 25; S.Up = 24; S.NameSide = 1;
			S.PlaceX = -8; S.PlaceZ = 12; S.EntryX = -8; S.EntryZ = 4.6;
			S.Parts.Add(Box(-8, 0, 10, 8, 3.6, Grey));               // the house, its porch side to the south
			S.Parts.Add(Box(10, -2, 7, 11, 4.2, GreyDark));          // the shed
			{ FPart Tank = Box(4, 13, 5, 5, 1.2, Steel); Tank.bRound = true; S.Parts.Add(Tank); }
			S.Words.Add({ S.Sign, -8, 4.06, 2.6, 180, .55 });
		}
		else if (S.Id == TEXT("breather"))
		{
			// N1110 (Caliche history, approved by Tom 10 November; S47Breather.h): the cave's tour entrance as the Trujillos keep it. An
			// outcrop of the hard caliche rock with a dark opening in its east face, a poured concrete surround and landing with a pipe
			// rail, a chain-link fence from the rock round the front with a padlocked gate, a sign on two posts, a bronze plaque on a
			// stone, and the stucco ticket hut. The rock and the fence stop the walker; nobody goes down tonight.
			S.Sign = TEXT("THE BREATHER"); S.Title = TEXT("the Breather, the cave west of Main Street"); S.MapName = TEXT("The Breather");
			S.Traits = ES47PlaceTrait::Town | ES47PlaceTrait::Open;
			S.Radius = 14; S.Up = 20; S.NameSide = -1; S.bLandmark = true;
			S.PlaceX = 4.5; S.PlaceZ = .3; S.EntryX = 6; S.EntryZ = .3;
			constexpr uint32 Rock = 0x8c7258, RockDark = 0x76604b, RockLight = 0x9e8468, Concrete = 0xaaa499, Dark = 0x050505, Mesh = 0x6f736f,
				Stucco = 0xd9c8a5, Roof = 0x7a4b33, Ply = 0x8a6a45, Green = 0x2f4a36, Bronze = 0x6b5530, Brass = 0xa08c48, Post = 0x4a3b2c, Card = 0xe6dfc8;
			const TCHAR* const Gate = TEXT("ff:breather:gate");
			const auto Add = [&S](double X, double Z, double W, double D, double H, uint32 Colour, bool bSolid, double Lift, const TCHAR* Use, bool bRound = false)
			{
				FPart P = Box(X, Z, W, D, H, Colour, bSolid); P.Lift = Lift; P.bRound = bRound; if (Use) P.Use = Use; S.Parts.Add(P);
			};
			// The outcrop: three blocks of rock in a ragged line, and a cap.
			Add(-4.5, .3, 8, 10, 3.6, Rock, true, 0, Gate);
			Add(-3.2, -6.2, 6, 3.6, 2.5, RockDark, true, 0, Gate);
			Add(-3.8, 6.8, 5.4, 3.4, 2.1, RockLight, true, 0, Gate);
			Add(-5.5, -.6, 5, 6.5, 1.5, RockDark, false, 3.3, nullptr);
			// The mouth in its concrete surround, and the landing in front of it with its rail.
			Add(-.47, .3, .1, 2.0, 1.85, Dark, false, 0, Gate);
			Add(-.38, .3, .26, 2.6, .28, Concrete, false, 1.85, nullptr);
			for (const double Jz : { -.85, 1.45 }) Add(-.38, Jz, .26, .3, 1.85, Concrete, false, 0, nullptr);
			Add(1.4, .3, 3.6, 3.0, .14, Concrete, false, 0, nullptr);
			for (const double Rz : { -1.1, 1.7 })
			{
				for (const double Px : { .4, 2.8 }) Add(Px, Rz, .06, .06, 1.0, Steel, false, 0, nullptr, true);
				Add(1.6, Rz, 2.5, .05, .05, Steel, false, .95, nullptr);
			}
			// The fence at x 3.6 with its gate between z -0.9 and 1.5, and back to the rock at each end. The mesh panels and the gate's
			// top rail stop the walker.
			for (const double Fz : { -6.6, -3.75, -.9, 1.5, 4.35, 7.2 }) Add(3.6, Fz, .07, .07, 2.0, Steel, false, 0, nullptr, true);
			for (const double Cz : { -3.75, 4.35 }) { Add(3.6, Cz, .02, 5.7, 1.75, Mesh, true, .1, Gate); Add(3.6, Cz, .05, 5.7, .05, Steel, false, 1.85, nullptr); }
			Add(1.65, -6.6, 3.9, .02, 1.75, Mesh, true, .1, Gate);
			Add(1.2, 7.2, 4.8, .02, 1.75, Mesh, true, .1, Gate);
			Add(3.6, .3, .05, 2.4, .05, Steel, true, 1.75, Gate);
			Add(3.6, .3, .05, 2.4, .05, Steel, false, .12, nullptr);
			Add(3.6, .3, .05, .05, 1.8, Steel, false, 0, nullptr, true);
			Add(3.64, -.35, .02, .7, .45, Card, false, 1.0, Gate);
			Add(3.65, .3, .03, .14, .34, Steel, false, .85, Gate);
			Add(3.68, .3, .05, .07, .09, Brass, false, .82, Gate);
			S.Words.Add({ TEXT("CLOSED FOR\nTHE SEASON"), 3.66, -.35, 1.22, 90, .085 });
			// The sign, on two posts north of the gate, toward the street.
			const TCHAR* const SignUse = TEXT("ff:breather:sign");
			for (const double Sz : { -4.5, -1.7 }) Add(4.9, Sz, .1, .1, 2.6, Post, true, 0, SignUse);
			Add(4.9, -3.1, .08, 3.2, 1.3, Green, false, 1.2, SignUse);
			S.Words.Add({ TEXT("THE BREATHER\nCAVE TOURS\nCLOSED FOR THE SEASON"), 4.95, -3.1, 1.85, 90, .19, true });
			S.Words.Add({ TEXT("THE BREATHER\nCAVE TOURS\nCLOSED FOR THE SEASON"), 4.85, -3.1, 1.85, 270, .19, true });
			// The plaque, on a stone south of the gate.
			Add(4.4, 2.6, .7, .6, .9, RockLight, true, 0, TEXT("ff:breather:plaque"));
			Add(4.77, 2.6, .03, .5, .36, Bronze, false, .33, TEXT("ff:breather:plaque"));
			S.Words.Add({ TEXT("THE BREATHER\n1923"), 4.80, 2.6, .53, 90, .06, true });
			// The ticket hut: stucco, a flat roof, its window boarded for the winter, its door on the north side.
			const TCHAR* const Hut = TEXT("ff:breather:hut");
			Add(6.5, 9.5, 3.0, 3.2, 2.7, Stucco, true, 0, Hut);
			Add(6.5, 9.5, 3.5, 3.7, .22, Roof, false, 2.7, nullptr);
			Add(8.03, 9.2, .06, 1.1, .7, Ply, false, 1.0, Hut);
			Add(6.0, 7.87, .9, .06, 2.0, 0x5a4632, false, 0, Hut);
			S.Words.Add({ TEXT("TICKETS"), 8.07, 9.2, 2.1, 90, .2 });
		}
		else if (S.Id == TEXT("grange"))
		{
			// A Grange hall: one big room for dances, meetings and bingo, gable end to the highway, a gravel lot in front.
			S.Sign = TEXT("GRANGE HALL (TEMP)"); S.Title = TEXT("the Grange hall"); S.MapName = TEXT("Grange Hall (temp)");
			S.Traits = ES47PlaceTrait::Town | ES47PlaceTrait::Road | ES47PlaceTrait::Lot | ES47PlaceTrait::Open;
			S.Radius = 16; S.Up = 26; S.NameSide = -1;
			S.PlaceX = 22; S.PlaceZ = 0; S.EntryX = 14.5; S.EntryZ = 0;
			S.Parts.Add(Box(0, 0, 28, 15, 6, Grey));                 // x -54 to -26, z 2264.5 to 2279.5
			S.Words.Add({ S.Sign, 14.06, 0, 4.2, 90, .9 });
		}
	}
	FVector2D Turn(double X, double Z, double YawDeg)
	{
		const double A = FMath::DegreesToRadians(YawDeg), C = FMath::Cos(A), S = FMath::Sin(A);
		return FVector2D(X * C - Z * S, X * S + Z * C);
	}
	bool Square(double YawDeg) { const double R = FMath::Fmod(FMath::Abs(YawDeg), 90.0); return R < 1e-6 || R > 90 - 1e-6; }
}

const TArray<S47TownSites::FSite>& S47TownSites::Sites()
{
	using namespace S47TownSitesLocal;
	static const TArray<FSite> All = []
	{
		TArray<FSite> Out;
		for (const FSpot& Spot : Spots)
		{
			FSite S; S.Id = Spot.Id; S.X = Spot.X; S.Z = Spot.Z; S.Yaw = Spot.Yaw;
			Shape(S);
			Out.Add(MoveTemp(S));
		}
		return Out;
	}();
	return All;
}

const S47TownSites::FSite* S47TownSites::Find(const FString& Id)
{
	return Sites().FindByPredicate([&](const FSite& S) { return S.Id == Id; });
}

FVector2D S47TownSites::ToRoad(const FSite& S, double LocalX, double LocalZ)
{
	return FVector2D(S.X, S.Z) + S47TownSitesLocal::Turn(LocalX, LocalZ, S.Yaw);
}

TArray<FBox2D> S47TownSites::Footprint(const FSite& S, const FPart& P)
{
	using namespace S47TownSitesLocal;
	TArray<FVector2D> C;
	for (const FVector2D& K : { FVector2D(-1, -1), FVector2D(1, -1), FVector2D(1, 1), FVector2D(-1, 1) })
		C.Add(ToRoad(S, P.X + K.X * P.W * .5, P.Z + K.Y * P.D * .5));
	FBox2D All(ForceInit);
	for (const FVector2D& V : C) All += V;
	if (Square(S.Yaw)) return { All };
	// Strips across x: in each, the z the turned rectangle reaches (at the strip's two edges, and at any corner inside it).
	TArray<FBox2D> Out;
	const auto ZAt = [&C](double X, double& Lo, double& Hi)
	{
		for (int32 I = 0; I < 4; ++I)
		{
			const FVector2D& A = C[I]; const FVector2D& B = C[(I + 1) % 4];
			if ((X < FMath::Min(A.X, B.X) - 1e-9) || (X > FMath::Max(A.X, B.X) + 1e-9) || FMath::IsNearlyEqual(A.X, B.X)) continue;
			const double Z = A.Y + (B.Y - A.Y) * (X - A.X) / (B.X - A.X);
			Lo = FMath::Min(Lo, Z); Hi = FMath::Max(Hi, Z);
		}
	};
	for (double X0 = All.Min.X; X0 < All.Max.X - 1e-6; X0 += StripM)
	{
		const double X1 = FMath::Min(All.Max.X, X0 + StripM);
		double Lo = TNumericLimits<double>::Max(), Hi = -TNumericLimits<double>::Max();
		ZAt(X0, Lo, Hi); ZAt(X1, Lo, Hi);
		for (const FVector2D& V : C) if (V.X >= X0 && V.X <= X1) { Lo = FMath::Min(Lo, V.Y); Hi = FMath::Max(Hi, V.Y); }
		if (Lo < Hi) Out.Add(FBox2D(FVector2D(X0, Lo), FVector2D(X1, Hi)));
	}
	return Out;
}

TArray<FBox2D> S47TownSites::SolidFootprints()
{
	TArray<FBox2D> Out;
	for (const FSite& S : Sites()) for (const FPart& P : S.Parts) if (P.bSolid) Out.Append(Footprint(S, P));
	return Out;
}

TArray<FS47EventPlace> S47TownSites::EventPlaces()
{
	TArray<FS47EventPlace> Out;
	for (const FSite& S : Sites())
	{
		if (S.bLandmark) continue;
		FS47EventPlace P; P.Id = S.Id; P.Title = S.Title; P.Region = S.Id == TEXT("baca") ? TEXT("land") : TEXT("caliche");
		const FVector2D At = ToRoad(S, S.PlaceX, S.PlaceZ), In = ToRoad(S, S.EntryX, S.EntryZ);
		P.X = At.X; P.Z = At.Y; P.EntryX = In.X; P.EntryZ = In.Y; P.Traits = S.Traits; P.Radius = S.Radius; P.Up = S.Up;
		Out.Add(P);
	}
	return Out;
}

bool S47TownSites::SelfTest(TArray<FString>& Lines)
{
	using namespace S47TownSitesLocal;
	bool bAll = true;
	const auto Check = [&](bool bOk, const FString& What) { bAll &= bOk; Lines.Add(FString(bOk ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("temp sites: ") + What); };
	Check(Sites().Num() == 5 && Find(TEXT("trailers")) && Find(TEXT("school")) && Find(TEXT("grange")) && Find(TEXT("baca")) && Find(TEXT("breather")), TEXT("the trailer park, the school, the Grange hall, the Baca place and the Breather are in the table"));
	bool bNamed = true, bTitled = true, bInRim = true, bOffRoad = true, bWords = true;
	int32 Trailers = 0, Cars = 0;
	for (const FSite& S : Sites())
	{
		// (N1110: a landmark is there for good, and says so nowhere.)
		bNamed &= S.bLandmark ? !S.Sign.Contains(TEXT("TEMP")) && !S.MapName.Contains(TEXT("temp")) && !S.Words.IsEmpty() : S.Sign.EndsWith(TEXT("(TEMP)")) && S.MapName.EndsWith(TEXT("(temp)")) && !S.Words.IsEmpty();
		bTitled &= !S.Title.IsEmpty() && FChar::IsLower(S.Title[0]) && S.Title.StartsWith(TEXT("the "));
		for (const FWords& W : S.Words) bWords &= S.bLandmark ? !W.Text.Contains(TEXT("TEMP")) : W.Text.Contains(TEXT("(TEMP)"));
		for (const FPart& P : S.Parts)
		{
			if (!P.bSolid) continue;
			for (const FBox2D& B : Footprint(S, P))
			{
				bInRim &= S47OpenLand::InRim(B.Min.X, B.Min.Y, 50) && S47OpenLand::InRim(B.Max.X, B.Max.Y, 50);
				// Clear of the highway's paving and shoulder and the power line's poles on its east side (road x 15).
				bOffRoad &= B.Min.X > 8 || B.Max.X < -8;
				bOffRoad &= !(B.Min.X < 15.6 && B.Max.X > 14.4 && B.Min.Y < 2300 && B.Max.Y > 1640);
			}
			if (S.Id == TEXT("trailers") && P.W > 4 && P.W < 4.5 && P.D > 15) ++Trailers;
			if (S.Id == TEXT("trailers") && P.H < 1.6 && P.D > 4 && P.D < 5) ++Cars;
		}
	}
	Check(bNamed && bWords, TEXT("each box says TEMP on its board or wall, and (temp) on the sheet; the Breather does not"));
	Check(bTitled, TEXT("each title reads in a radio line (lower case, with its article)"));
	Check(bInRim && bOffRoad, TEXT("every box stands on the land, off the highway and clear of the power line"));
	Check(Trailers >= 6 && Trailers <= 10 && Cars >= 2, FString::Printf(TEXT("the trailer park has %d trailers and %d cars"), Trailers, Cars));
	// Nothing on top of what is there already: Caliche's buildings as the sheet has them (SS47FieldMap's TownBlocks), the diner and
	// its lot, the two town signs, and the boxes not on each other.
	static const double Built[][4] = {
		{ -27, -17, 2066, 2078 }, { -27, -17.4, 2078, 2088 }, { -26, -16, 2100, 2112 }, { -25, -15, 2126, 2140 }, { -29, -15, 2152, 2172 },
		{ -31, -15, 2190, 2214 }, { 20, 30, 2030, 2042 }, { 20.5, 29.5, 2058, 2070 }, { 21, 35, 2091, 2100 }, { 24, 33, 2128, 2138 },
		{ 24.5, 32, 2152, 2161 }, { 25, 34, 2180, 2190 }, { -28, -19, 2236, 2246 }, { -28.05, -4, 1984, 2016 }, { 18, 61, 2012, 2061 },
		{ -10.2, -7, 1879.8, 1880.2 }, { 7, 10.2, 2334.8, 2335.2 } };
	bool bClear = true; FString Hit;
	TArray<TPair<FString, FBox2D>> Mine;
	for (const FSite& S : Sites()) for (const FPart& P : S.Parts) for (const FBox2D& B : Footprint(S, P)) Mine.Add({ S.Id, B });
	for (const auto& A : Mine)
	{
		for (const auto& K : Built) if (A.Value.Intersect(FBox2D(FVector2D(K[0], K[2]), FVector2D(K[1], K[3])))) { bClear = false; Hit = A.Key; }
		for (const auto& B : Mine) if (A.Key != B.Key && A.Value.ExpandBy(-.01).Intersect(B.Value)) { bClear = false; Hit = A.Key + TEXT("/") + B.Key; }
	}
	Check(bClear, bClear ? FString(TEXT("no box stands on a building, the diner's lot, a town sign or another site")) : TEXT("overlap at ") + Hit);
	// The strips cover a turned box and do not reach far past it.
	{
		FSite T; T.X = 0; T.Z = 0; T.Yaw = 30; FPart P = Box(0, 0, 4.3, 18, 3, Grey);
		const TArray<FBox2D> Strips = Footprint(T, P);
		double Area = 0; for (const FBox2D& B : Strips) Area += B.GetArea();
		bool bCovers = true;
		for (double U = -2.1; U <= 2.1; U += .7) for (double V = -8.9; V <= 8.9; V += 1.1)
		{
			const FVector2D Q = ToRoad(T, U, V);
			bCovers &= Strips.ContainsByPredicate([&Q](const FBox2D& B) { return B.ExpandBy(1e-6).IsInside(Q); });
		}
		FSite Square0 = Sites()[0]; Square0.Yaw = 0;
		Check(bCovers && Area >= 4.3 * 18 && Area < 4.3 * 18 * 1.6 && Footprint(Square0, Square0.Parts[0]).Num() == 1, FString::Printf(TEXT("a box turned 30 degrees is covered by %d strips (%.0f m2 for 77), a square one by one box"), Strips.Num(), Area));
	}
	// On the catalogue, with what an event needs of them.
	const TArray<FS47EventPlace> Places = EventPlaces();
	int32 Boxes = 0; for (const FSite& S : Sites()) Boxes += S.bLandmark ? 0 : 1;
	bool bPlaces = Places.Num() == Boxes && !Places.ContainsByPredicate([](const FS47EventPlace& P) { return P.Id == TEXT("breather"); });
	for (const FS47EventPlace& P : Places) bPlaces &= P.HasAny(ES47PlaceTrait::Open | ES47PlaceTrait::Lot) && !P.Has(ES47PlaceTrait::Door) && !P.Has(ES47PlaceTrait::Window) && S47OpenLand::InRim(P.X, P.Z);
	Check(bPlaces, TEXT("each is a place for events: open ground or a lot, and no door or window it does not have"));
	// Turned by its row: the trailer park turned a quarter round keeps its middle and its entrance swings to the south.
	{
		FSite T = Sites()[0]; T.Yaw = 90;
		const FVector2D In = ToRoad(T, T.EntryX, T.EntryZ);
		Check(FMath::IsNearlyEqual(In.X, T.X, 1e-6) && FMath::IsNearlyEqual(In.Y, T.Z + T.EntryX, 1e-6), TEXT("a row's Yaw turns the whole site about its middle, clockwise"));
	}
	// N1110: the Breather's pieces carry its four uses, and its rock and fence close the mouth off all round.
	if (const FSite* B = Find(TEXT("breather")))
	{
		TSet<FString> Uses; for (const FPart& P : B->Parts) if (!P.Use.IsEmpty()) Uses.Add(P.Use);
		const FVector2D Mouth = ToRoad(*B, -.47, .3), Out = ToRoad(*B, 4.5, .3);
		bool bClosed = true;
		// Every way out of the mouth to the gate's side crosses something solid: the lines from it to points round the front.
		for (double Z = -9; Z <= 10; Z += .5)
		{
			const FVector2D To = ToRoad(*B, 6.0, Z);
			bool bCross = false;
			for (const FPart& P : B->Parts) if (P.bSolid) for (const FBox2D& Fp : Footprint(*B, P))
				for (double U = .02; U <= 1 && !bCross; U += .002) if (Fp.ExpandBy(.01).IsInside(Mouth + (To - Mouth) * U) && !Fp.IsInside(Mouth)) bCross = true;
			bClosed &= bCross;
		}
		Check(Uses.Num() == 4 && Uses.Contains(TEXT("ff:breather:gate")) && bClosed && FVector2D::Distance(Mouth, Out) < 6,
			FString::Printf(TEXT("the Breather: %d uses on its pieces (gate, sign, plaque, hut), and no way from the mouth out to the street but through the fence"), Uses.Num()));
	}
	bAll &= S47Breather::SelfTest(Lines);
	return bAll;
}

// ---------- in the world ----------

double FS47TownSites::Ground(double RoadX, double RoadZ, bool bWithFloor) const
{
	// The walker's own floor where the player is on the road's land; otherwise the land's shape, as FS47Places seats its props.
	if (bWithFloor && M && M->Player && M->Player->Floor && M->Web && (M->Web->Area() == TEXT("road") || M->Web->Area() == TEXT("diner")))
	{
		const FVector Road = S47Legs::ORIGINS(TEXT("road"));
		const double F = M->Player->Floor(Road.X + RoadX, Road.Z + RoadZ);
		if (FMath::IsFinite(F)) return F;
	}
	return S47Legs::ORIGINS(TEXT("road")).Y + S47RoadTerrain::SurfaceY(RoadX, RoadZ);
}

bool FS47TownSites::Build()
{
	using namespace S47TownSitesLocal;
	using namespace S47TownSites;
	if (bBuilt) return true;
	if (!M || !IsValid(M->Web.Get())) return false;
	AS47Web* W = M->Web.Get();
	const int32 Root = W->Handle(TEXT("world.legs.road.area.group"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, CubePath);
	UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, CylPath);
	UMaterialInterface* Lit = LoadObject<UMaterialInterface>(nullptr, LitPath);
	if (Root < 0 || !Cube || !Cyl || !Lit) return false;
	TMap<uint32, UMaterialInstanceDynamic*> Mats;
	const auto Mat = [&](uint32 Hex)
	{
		if (UMaterialInstanceDynamic** Had = Mats.Find(Hex)) return *Had;
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Lit, W);
		D->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(Hex)); D->SetScalarParameterValue(TEXT("Roughness"), .9f); D->SetScalarParameterValue(TEXT("NativeLighting"), 1);
		return Mats.Add(Hex, D);
	};
	int32 Models = 0;
	for (const FSite& S : Sites())
	{
		// The site's real model, when its asset is there (SCHOOL-V2 stage 2); the stand-in boxes and their words go unseen then.
		UStaticMesh* Model = S.Model.IsEmpty() ? nullptr : LoadObject<UStaticMesh>(nullptr, *S.Model);
		if (!S.Model.IsEmpty() && !Model) { UE_LOG(LogS47TownSites, Display, TEXT("S47: temp sites: %s has no model at %s (Tools/freemodels/import_free.sh): its boxes stand"), *S.Id, *S.Model); }
		if (Model)
		{
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, *FString::Printf(TEXT("S47Site_%s_Model"), *S.Id));
			C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(W->GetRootComponent());
			C->SetStaticMesh(Model); C->SetCastShadow(true);
			// As the boxes: sight, camera and use rays stop at its walls (complex collision from the import); the walker and the truck are
			// still held by the stand-in boxes' footprints until the walls get colliders of their own (stage 3).
			C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore);
			C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
			C->RegisterComponent();
			FBuiltPart B; B.Site = S.Id; B.bModel = true; B.Comp = C; Parts.Add(B);
			++Models;
		}
		for (int32 I = 0; I < S.Parts.Num(); ++I)
		{
			const FPart& P = S.Parts[I];
			const FString Name = FString::Printf(TEXT("S47TempSite_%s_%d"), *S.Id, I);
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, *Name);
			C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(W->GetRootComponent());
			C->SetStaticMesh(P.bRound ? Cyl : Cube); C->SetMaterial(0, Mat(P.Colour)); C->SetCastShadow(true);
			// The view, the camera's sight and a use ray stop at a wall; the walker and the truck are held by the colliders below.
			C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore);
			C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
			C->RegisterComponent();
			FBuiltPart B; B.Site = S.Id; B.Part = I; B.bByModel = Model && !P.bKeep; B.Comp = C; Parts.Add(B);
		}
		for (int32 I = 0; I < S.Words.Num(); ++I)
		{
			const FWords& Wd = S.Words[I];
			UTextRenderComponent* T = NewObject<UTextRenderComponent>(W, *FString::Printf(TEXT("S47TempSite_%s_Words%d"), *S.Id, I));
			T->SetMobility(EComponentMobility::Movable); T->SetupAttachment(W->GetRootComponent());
			T->SetText(FText::FromString(Wd.Text)); T->SetWorldSize(float(Wd.Size * 100.0));
			T->SetHorizontalAlignment(EHTA_Center); T->SetVerticalAlignment(EVRTA_TextCenter);
			// White on the dark board; near black on a grey wall.
			const bool bOnBoard = Wd.bLight || (S.Id == TEXT("trailers") && Wd.Size < .6);
			T->SetTextRenderColor(bOnBoard ? FColor(236, 232, 214) : FColor(32, 30, 28));
			T->SetCollisionEnabled(ECollisionEnabled::NoCollision); T->SetCastShadow(false);
			T->RegisterComponent();
			FBuiltPart B; B.Site = S.Id; B.bWords = true; B.Words = I; B.bByModel = Model && !Wd.bKeep; B.Comp = T; Parts.Add(B);
		}
	}
	for (const FSite& S : Sites()) Seat(S.Id, false);
	ShowAll(true);
	// Into the road's area, so that they come and go with it as the town does. A part with a use is what a look picks for it (N1110).
	UseNodes.Reset();
	for (const FBuiltPart& B : Parts)
	{
		UPrimitiveComponent* C = Cast<UPrimitiveComponent>(B.Comp.Get());
		if (!C) continue;
		const int32 Node = W->AdoptNode(C, Root, C->GetName());
		const FSite* Owner = B.bWords || Node < 0 ? nullptr : Find(B.Site);
		const FString Use = Owner && Owner->Parts.IsValidIndex(B.Part) ? Owner->Parts[B.Part].Use : FString();
		if (Use.IsEmpty()) continue;
		W->MarkUsable(Node, Use);
		if (!UseNodes.Contains(Use)) UseNodes.Add(Use, Node);
	}
	bBuilt = true;
	UE_LOG(LogS47TownSites, Display, TEXT("S47: temp sites built: %d sites, %d parts and boards, %d real models"), Sites().Num(), Parts.Num(), Models);
	return true;
}

void FS47TownSites::Seat(const FString& Site, bool bWithFloor)
{
	using namespace S47TownSites;
	const FSite* S = Find(Site);
	if (!S) return;
	const FVector Road = S47Legs::ORIGINS(TEXT("road"));
	// Each box sits as deep as its lowest corner and reaches its height over its highest one: no gap under it on a slope.
	TArray<double> Bottom, Top;
	for (const FPart& P : S->Parts)
	{
		double Lo = TNumericLimits<double>::Max(), Hi = -TNumericLimits<double>::Max();
		for (const FVector2D& K : { FVector2D(0, 0), FVector2D(-1, -1), FVector2D(1, -1), FVector2D(1, 1), FVector2D(-1, 1) })
		{
			const FVector2D At = ToRoad(*S, P.X + K.X * P.W * .5, P.Z + K.Y * P.D * .5);
			const double G = Ground(At.X, At.Y, bWithFloor);
			Lo = FMath::Min(Lo, G); Hi = FMath::Max(Hi, G);
		}
		Bottom.Add(Lo); Top.Add(Hi);
	}
	for (const FBuiltPart& B : Parts)
	{
		if (B.Site != Site) continue;
		USceneComponent* C = B.Comp.Get();
		if (!C) continue;
		if (B.bModel)
		{
			// The real model stands on the highest ground under its footprint (its floor 0.6 m over the yard at the front door, the
			// foundation showing more where the land falls away), its pivot ModelLift under that.
			const UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(C);
			const FBox Bounds = SMC && SMC->GetStaticMesh() ? SMC->GetStaticMesh()->GetBoundingBox() : FBox(FVector(-100), FVector(100));
			const double Yaw = FMath::DegreesToRadians(S->ModelYaw);
			double Hi = -TNumericLimits<double>::Max();
			for (const FVector2D& K : { FVector2D(0, 0), FVector2D(Bounds.Min.X, Bounds.Min.Y), FVector2D(Bounds.Max.X, Bounds.Min.Y), FVector2D(Bounds.Max.X, Bounds.Max.Y), FVector2D(Bounds.Min.X, Bounds.Max.Y) })
			{
				// a corner of the mesh (Unreal centimetres, x east, y south) turned by the model's own yaw, in the site's metres
				const double LX = (K.X * FMath::Cos(Yaw) - K.Y * FMath::Sin(Yaw)) * .01, LZ = (K.X * FMath::Sin(Yaw) + K.Y * FMath::Cos(Yaw)) * .01;
				const FVector2D At = ToRoad(*S, S->ModelX + LX, S->ModelZ + LZ);
				Hi = FMath::Max(Hi, Ground(At.X, At.Y, bWithFloor));
			}
			const FVector2D At = ToRoad(*S, S->ModelX, S->ModelZ);
			C->SetWorldLocationAndRotation(S47Three::ToUE(FVector(Road.X + At.X, Hi - S->ModelLift, Road.Z + At.Y)), FRotator(0, S->Yaw + S->ModelYaw, 0));
		}
		else if (!B.bWords)
		{
			const FPart& P = S->Parts[B.Part];
			const FVector2D At = ToRoad(*S, P.X, P.Z);
			// Standing parts go into the ground to the lowest corner; a raised one (the tank, the board) hangs over the highest.
			const double Y0 = P.Lift > 0 ? Top[B.Part] + P.Lift : Bottom[B.Part] - .3;
			const double Y1 = (P.Lift > 0 ? Top[B.Part] + P.Lift : Top[B.Part]) + P.H;
			C->SetWorldLocationAndRotation(S47Three::ToUE(FVector(Road.X + At.X, (Y0 + Y1) * .5, Road.Z + At.Y)), FRotator(0, S->Yaw, 0));
			C->SetWorldScale3D(FVector(P.W, P.D, Y1 - Y0));
		}
		else
		{
			const FWords& Wd = S->Words[B.Words];
			const FVector2D At = ToRoad(*S, Wd.X, Wd.Z);
			// The ground under the words' own wall: the highest of the part they are on, so a sign on a slope still reads at its height.
			double G = Ground(At.X, At.Y, bWithFloor);
			for (int32 I = 0; I < S->Parts.Num(); ++I)
			{
				const FPart& P = S->Parts[I];
				if (FMath::Abs(Wd.X - P.X) <= P.W * .5 + .2 && FMath::Abs(Wd.Z - P.Z) <= P.D * .5 + .2 && P.Lift <= 0) { G = Top[I]; break; }
			}
			// TextRender faces its own +X: the compass bearing it faces, as Unreal yaw (east 0, south 90), turned with the site.
			C->SetWorldLocationAndRotation(S47Three::ToUE(FVector(Road.X + At.X, G + Wd.Up, Road.Z + At.Y)), FRotator(0, Wd.Facing - 90 + S->Yaw, 0));
		}
	}
	if (bWithFloor) SeatedSites.Add(Site);
}

int32 FS47TownSites::ColliderCount() const
{
	if (!M || !M->Web) return 0;
	int32 N = 0;
	for (const FS47Rect& R : M->Web->CollidersMutable(TEXT("road"))) N += R.Id.StartsWith(TEXT("temp:")) && R.bEnabled ? 1 : 0;
	return N;
}

bool FS47TownSites::Seated(const FString& Id) const { return SeatedSites.Contains(Id); }

void FS47TownSites::SetColliders(bool bOn)
{
	using namespace S47TownSites;
	if (!M || !M->Web) return;
	TArray<FS47Rect>& Walk = M->Web->CollidersMutable(TEXT("road"));
	const FVector Road = S47Legs::ORIGINS(TEXT("road"));
	bool bHave = false;
	for (FS47Rect& R : Walk) if (R.Id.StartsWith(TEXT("temp:"))) { R.bEnabled = bOn; bHave = true; }
	if (bHave || !bOn) return;
	// The walker's list (the diner's area walks the road's): world terms, the road's land at x + 8000.
	for (const FSite& S : Sites()) for (int32 I = 0; I < S.Parts.Num(); ++I)
	{
		if (!S.Parts[I].bSolid) continue;
		int32 K = 0;
		for (const FBox2D& B : Footprint(S, S.Parts[I]))
		{
			FS47Rect R; R.MinX = Road.X + B.Min.X; R.MaxX = Road.X + B.Max.X; R.MinZ = Road.Z + B.Min.Y; R.MaxZ = Road.Z + B.Max.Y;
			R.Id = FString::Printf(TEXT("temp:%s:%d:%d"), *S.Id, I, K++); R.bEnabled = true;
			Walk.Add(R);
		}
	}
}

void FS47TownSites::SetDrive(bool bOn)
{
	using namespace S47TownSites;
	if (!M || !M->World) return;
	IS47DriveArea* Area = M->World->LegArea(TEXT("road"));
	if (!Area) return;
	const FVector Road = S47Legs::ORIGINS(TEXT("road"));
	if (Obstacles.IsEmpty())
		for (const FBox2D& B : SolidFootprints()) Obstacles.Add(FS47Obstacle::Box(Road.X + B.Min.X, Road.X + B.Max.X, Road.Z + B.Min.Y, Road.Z + B.Max.Y));
	// Others keep places in the list by index (the farm's controlled spans): never removed, only sent far off and brought back.
	constexpr double Away = 1000000;
	for (const FS47Obstacle& Mine : Obstacles)
	{
		FS47Obstacle* Found = Area->Obstacles.FindByPredicate([&Mine](const FS47Obstacle& O)
		{
			return !O.bCircle && FMath::IsNearlyEqual(O.MinZ, Mine.MinZ, 1e-6) && FMath::IsNearlyEqual(O.MaxZ, Mine.MaxZ, 1e-6) &&
				(FMath::IsNearlyEqual(O.MinX, Mine.MinX, 1e-6) || FMath::IsNearlyEqual(O.MinX, Mine.MinX + Away, 1e-6));
		});
		if (Found) { Found->MinX = Mine.MinX + (bOn ? 0 : Away); Found->MaxX = Mine.MaxX + (bOn ? 0 : Away); }
		else if (bOn) Area->Obstacles.Add(Mine);
	}
	bDriveAdded = bOn;
}

void FS47TownSites::HidePlants(bool bHide)
{
	if (!bHide)
	{
		TSet<UInstancedStaticMeshComponent*> Touched;
		for (const FHidden& H : Hidden) if (UInstancedStaticMeshComponent* C = H.Comp.Get()) { C->UpdateInstanceTransform(H.Index, H.Was, true, false, true); Touched.Add(C); }
		for (UInstancedStaticMeshComponent* C : Touched) { C->MarkRenderStateDirty(); if (auto* H = Cast<UHierarchicalInstancedStaticMeshComponent>(C)) H->BuildTreeIfOutdated(false, true); }
		Hidden.Reset();
		return;
	}
	if (!Hidden.IsEmpty() || !M || !M->Web) return;
	// Caliche's desert plants and the route's (S47CalicheNature, S47RouteNature) where a box stands, a metre round it: a juniper
	// would otherwise stand up through a trailer's roof. Shrunk to nothing, never removed, so that every count they keep is as it was.
	TArray<FBox2D> Under;
	for (const FBox2D& B : S47TownSites::SolidFootprints()) Under.Add(B.ExpandBy(1.0));
	// (N1110: and the Breather's landing and surround, inside its fence.)
	for (const S47TownSites::FSite& S : S47TownSites::Sites()) if (S.bLandmark)
		for (const S47TownSites::FPart& P : S.Parts) if (!P.bSolid && P.Lift <= 0) for (const FBox2D& B : S47TownSites::Footprint(S, P)) Under.Add(B.ExpandBy(.5));
	const FVector Road = S47Legs::ORIGINS(TEXT("road"));
	TArray<UInstancedStaticMeshComponent*> All;
	M->Web->GetComponents<UInstancedStaticMeshComponent>(All);
	int32 Kinds = 0, Seen = 0;
	FVector2D First(0, 0);
	for (UInstancedStaticMeshComponent* C : All)
	{
		const FString Name = C ? C->GetName() : FString();
		if (!C || !(Name.StartsWith(TEXT("Nature_")) || Name.StartsWith(TEXT("RouteNature_")))) continue;
		++Kinds; Seen += C->GetInstanceCount();
		if (Kinds == 1 && C->GetInstanceCount() > 0) { FTransform T0; C->GetInstanceTransform(0, T0, true); First = FVector2D(T0.GetLocation().X / 100.0 - Road.X, T0.GetLocation().Y / 100.0 - Road.Z); }
		bool bAny = false;
		for (int32 I = 0; I < C->GetInstanceCount(); ++I)
		{
			FTransform T;
			if (!C->GetInstanceTransform(I, T, true)) continue;
			const FVector L = T.GetLocation();
			const FVector2D At(L.X / 100.0 - Road.X, L.Y / 100.0 - Road.Z);
			if (!Under.ContainsByPredicate([&At](const FBox2D& B) { return B.IsInside(At); })) continue;
			FHidden H; H.Comp = C; H.Index = I; H.Was = T; Hidden.Add(H);
			FTransform Gone = T; Gone.SetScale3D(FVector(.0001));
			C->UpdateInstanceTransform(I, Gone, true, false, true);
			bAny = true;
		}
		if (bAny) { C->MarkRenderStateDirty(); if (auto* H = Cast<UHierarchicalInstancedStaticMeshComponent>(C)) H->BuildTreeIfOutdated(false, true); }
	}
	UE_LOG(LogS47TownSites, Display, TEXT("S47: temp sites: %d plants under the boxes hidden (%d plant kinds, %d plants looked at, %d components; the first at road %.0f %.0f)"), Hidden.Num(), Kinds, Seen, All.Num(), First.X, First.Y);
}

void FS47TownSites::ShowAll(bool bShow)
{
	for (const FBuiltPart& B : Parts)
		if (UPrimitiveComponent* C = Cast<UPrimitiveComponent>(B.Comp.Get()))
		{
			// A stand-in the site's real model replaces stays unseen (and does not stop a ray) for good.
			const bool bOn = bShow && !B.bByModel;
			C->SetHiddenInGame(!bOn);
			if (!B.bWords) C->SetCollisionEnabled(bOn ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		}
}

void FS47TownSites::Begin()
{
	if (!M || !M->IsFreeform() || !IsValid(M->Web.Get())) return;
	// A new world (a load that built the scene again): everything is made afresh in it.
	if (Scene.Get() != M->Web.Get()) { Parts.Reset(); Hidden.Reset(); SeatedSites.Reset(); Obstacles.Reset(); UseNodes.Reset(); bBuilt = false; bDriveAdded = false; }
	Scene = M->Web;
	if (!Build()) return;
	if (!BreatherPart) BreatherPart = MakeShared<FS47Breather>(M);
	BreatherPart->Begin(M->Web.Get(), UseNodes);
	ShowAll(true); SetColliders(true); HidePlants(true);
	bActive = true; SeatIn = 0;
	SetDrive(true);
}

void FS47TownSites::Stop()
{
	if (BreatherPart) BreatherPart->Stop();
	if (!bActive) return;
	bActive = false;
	if (!IsValid(Scene.Get())) return;
	ShowAll(false); SetColliders(false); HidePlants(false); SetDrive(false);
	for (const FLamp& L : Lamps) if (UPointLightComponent* C = L.L.Get()) C->SetVisibility(false);
}

void FS47TownSites::BuildLamps()
{
	using namespace S47TownSites;
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!IsValid(W) || !Lamps.IsEmpty()) return;
	const auto Add = [&](const FString& Site, double LocalX, double LocalZ, double Up, float Lumens, double Reach, double From, double To)
	{
		const FSite* S = Find(Site);
		if (!S) return;
		const FVector2D At = ToRoad(*S, LocalX, LocalZ);
		UPointLightComponent* C = NewObject<UPointLightComponent>(W, *FString::Printf(TEXT("S47TempLamp_%s_%d"), *Site, Lamps.Num()));
		C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(W->GetRootComponent());
		C->SetIntensityUnits(ELightUnits::Lumens); C->SetIntensity(Lumens); C->SetAttenuationRadius(float(Reach * 100.)); C->SetLightColor(FLinearColor(1.f, .78f, .52f)); C->SetCastShadows(false);
		C->RegisterComponent(); C->SetVisibility(false);
		C->SetWorldLocation(S47Three::ToUE(FVector(8000 + At.X, S47FarmWorld::FloorRoad(At.X, At.Y) + Up, At.Y)));
		FLamp L; L.Site = Site; L.L = C; L.From = From; L.To = To; Lamps.Add(L);
	};
	// The trailers' porch lights at the lane ends (north row, and two of the south row), until midnight; the court's yard light under
	// the tank and the school's over its front door, all night.
	for (const double X : { 38.0, 20.0, 2.0, -16.0 }) Add(TEXT("trailers"), X + 1.4, -6.4, 2.4, 260.f, 7, 19, 24);
	for (const double X : { 38.0, 2.0 }) Add(TEXT("trailers"), X - 1.4, 6.4, 2.4, 260.f, 7, 19, 24);
	Add(TEXT("trailers"), -40, 3.6, 13.2, 900.f, 26, 19, 31);
	Add(TEXT("school"), -18.8, 0, 3.6, 700.f, 16, 19, 31);
}

void FS47TownSites::StepLamps()
{
	Lamps.RemoveAll([](const FLamp& L) { return !L.L.IsValid(); }); // a new scene: made again
	if (Lamps.IsEmpty()) BuildLamps();
	if (!M || !M->Freeform || !M->Web) return;
	const FS47FreeformSession& F = *M->Freeform;
	const FString Area = M->Web->Area();
	const bool bHere = Area == TEXT("road") || Area == TEXT("diner");
	double H = FMath::Fmod(F.Clock(), 86400.) / 3600.;
	if (H < 8) H += 24; // the night's hours, 19 to 31
	const int32 Night = FS47FreeformSession::NightOf(F.Clock());
	for (const FLamp& L : Lamps)
	{
		UPointLightComponent* C = L.L.Get();
		if (!C) continue;
		// Out where something big came tonight: nobody switches it on again.
		const bool bDark = F.EventPlaces && F.EventPlaces->LastNight(L.Site) == Night && FS47Visitors::Big(F.EventPlaces->LastKind(L.Site));
		C->SetVisibility(bHere && !bDark && H >= L.From && H < L.To);
	}
}

void FS47TownSites::Step(double Dt)
{
	if (!bActive) { if (M && M->IsFreeform() && IsValid(M->Web.Get())) Begin(); return; }
	if (!M || M->Web.Get() != Scene.Get()) { Begin(); return; }
	// The truck's list is made once the world has its legs, which may be after the session began.
	if (!bDriveAdded) SetDrive(true);
	if (BreatherPart) BreatherPart->Step(Dt); // (N1110: every frame: the draft's level follows the player)
	if (Hidden.IsEmpty() && PlantTries < 6 && (PlantIn -= Dt) <= 0) { PlantIn = 5; ++PlantTries; HidePlants(true); }
	SeatIn -= Dt;
	if (SeatIn > 0 || !M->Player || !M->Web) return;
	SeatIn = 1;
	StepLamps();
	const FString Area = M->Web->Area();
	if (Area != TEXT("road") && Area != TEXT("diner")) return;
	double RX = 0, RZ = 0;
	if (!S47OpenLand::RoadOf(Area, M->Player->Pos.X, M->Player->Pos.Z, RX, RZ)) return;
	// On the ground the walker really stands on, once the player is near enough for it to be the land's own floor.
	for (const S47TownSites::FSite& S : S47TownSites::Sites())
		if (!SeatedSites.Contains(S.Id) && FVector2D::Distance(FVector2D(RX, RZ), FVector2D(S.X, S.Z)) < 400) Seat(S.Id, true);
}
