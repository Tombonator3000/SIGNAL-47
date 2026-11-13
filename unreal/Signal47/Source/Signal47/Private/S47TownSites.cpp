#include "S47TownSites.h"
#include "S47PropCatalogue.h"
#include "S47GroundEdits.h"
#include "S47FreeformTerrain.h"
#include "S47Breather.h"
#include "S47EventPlaces.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47Visitors.h"
#include "Components/PointLightComponent.h"
#include "Components/DecalComponent.h"
#include "S47Legs.h"
#include "S47Main.h"
#include "S47OpenLand.h"
#include "S47Props.h"
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
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include <limits>

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
		// CHURCH (Tom, 13 November: "lag kirkegården bak også"): San Isidro's graveyard, a landmark and not a box: the plot behind the
		// church (its rear wall is road x 35.1, z 2090.9 to 2100.1), east of the town's lots, north of the waitress's yard (z 2108.5).
		{ TEXT("cemetery"), 49, 2095.5, 0 },
	};
	// Pale grey, as a model maker's grey box is, with the parts a shade apart so the shapes read.
	constexpr uint32 Grey = 0xb4b0a6, GreyDark = 0x8e8a82, GreyLight = 0xc9c5bb, Board = 0x2b2a27, Steel = 0x8b8e8a, CarA = 0x6f4a3c, CarB = 0x4d5866;
	const TCHAR* const CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* const CylPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* const LitPath = TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque");
	constexpr double StripM = 1.5;

	TMap<FString, TArray<S47TownSites::FSite::FDress>> DressOf; // each site's bought pieces as last read (Build)
	S47TownSites::FPart Box(double X, double Z, double W, double D, double H, uint32 Colour, bool bSolid = true)
	{
		S47TownSites::FPart P; P.X = X; P.Z = Z; P.W = W; P.D = D; P.H = H; P.Colour = Colour; P.bSolid = bSolid; return P;
	}
	// SCHOOL-V2 stage 3: the school's inside from its plan (~/Assets/signal47-free/own/caliche_school/src/school_plan.py: centimetres, x east,
	// y north, the south-west corner at 0,0; the model turned a quarter clockwise on the site, so the plan's north is the site's +x and the
	// plan's east the site's +z (south)). Walls with the doors as gaps, the floors and the stair, the lights.
	void SchoolInside(S47TownSites::FSite& S)
	{
		using namespace S47TownSites;
		// plan centimetres to the site's metres: py 1200 at ModelX, px 1400 at ModelZ
		const auto PX = [&S](double Py) { return S.ModelX + (Py - 1200) * .01; };
		const auto PZ = [&S](double Px) { return S.ModelZ + (Px - 1400) * .01; };
		constexpr double Door = 1.1, Wide = 2.0, T = .2;
		// a wall along the plan's y (at px, py from A to B) with gaps at the given py; as site rectangles
		const auto AlongY = [&](double Px, double A, double B, std::initializer_list<double> Gaps, double GapW = Door)
		{
			double From = A;
			TArray<double> G(Gaps);
			G.Sort();
			for (double Gy : G) { if (Gy - GapW * 50 > From) S.Walls.Add({ PX(From), PX(Gy - GapW * 50), PZ(Px) - T * .5, PZ(Px) + T * .5 }); From = Gy + GapW * 50; }
			if (B > From) S.Walls.Add({ PX(From), PX(B), PZ(Px) - T * .5, PZ(Px) + T * .5 });
		};
		const auto AlongX = [&](double Py, double A, double B, std::initializer_list<double> Gaps, double GapW = Door)
		{
			double From = A;
			TArray<double> G(Gaps);
			G.Sort();
			for (double Gx : G) { if (Gx - GapW * 50 > From) S.Walls.Add({ PX(Py) - T * .5, PX(Py) + T * .5, PZ(From), PZ(Gx - GapW * 50) }); From = Gx + GapW * 50; }
			if (B > From) S.Walls.Add({ PX(Py) - T * .5, PX(Py) + T * .5, PZ(From), PZ(B) });
		};
		// Which floor a wall holds on, by the height of the walker's feet over the ground floor (12 November, after Tom's d60: one list
		// for both floors let nobody rail the stair well, and the basement had no south wall): G the ground floor and the yard (feet
		// over -0.9), B the basement and the areaway (under -2.5), or every height (the shaft, the outer walls with a door on both floors).
		constexpr double GLo = -.9, BHi = -2.5, Every = 1e9;
		const auto Level = [&S](int32 From, double Lo, double Hi) { for (int32 I = From; I < S.Walls.Num(); ++I) { S.Walls[I].Lo = Lo; S.Walls[I].Hi = Hi; } };
		const auto Rect = [&](double Py0, double Py1, double Px0, double Px1) { S.Walls.Add({ PX(Py0), PX(Py1), PZ(Px0), PZ(Px1) }); };
		int32 N0 = 0;
		// the outer walls: E1 in the south (2 m), E3 east at py 2100 with E4 under it (every height: S2 climbs along that wall), the north wall
		AlongX(0, 0, 2800, { 1400 }, Wide);
		AlongX(2400, 0, 2800, {});
		AlongY(2800, 0, 2400, { 2100 });
		// the west wall: E2 at py 2100 on the ground floor, whole in the basement; and the basement's south retaining wall at py 1200
		N0 = S.Walls.Num(); AlongY(0, 0, 2400, { 2100 }); Level(N0, GLo, Every);
		N0 = S.Walls.Num(); AlongY(0, 1200, 2400, {}); AlongX(1200, 0, 2800, {}); Level(N0, -Every, BHi);
		// the corridor's walls: K1..K8 on the ground floor, K9..K12 in the basement, and the shaft's sides at every height
		N0 = S.Walls.Num(); AlongY(1200, 0, 1800, { 200, 600, 1100, 1700 }); AlongY(1600, 0, 1800, { 200, 600, 1100, 1700 }); Level(N0, GLo, Every);
		N0 = S.Walls.Num(); AlongY(1200, 1200, 1800, { 1400, 1700 }); AlongY(1600, 1200, 1800, { 1400, 1700 }); Level(N0, -Every, BHi);
		AlongY(1200, 1800, 2400, {}); AlongY(1600, 1800, 2400, {});
		// the wings' cross walls: D1 (px 300), D2 (800), D3 (300), D4 (2400); the basement's D5 under D3 and D6 under D4
		N0 = S.Walls.Num();
		AlongX(400, 0, 1200, { 300 }); AlongX(800, 0, 1200, { 800 }); AlongX(1600, 0, 1200, { 300 });
		AlongX(400, 1600, 2800, {}); AlongX(800, 1600, 2800, {}); AlongX(1600, 1600, 2800, { 2400 });
		Level(N0, GLo, Every);
		N0 = S.Walls.Num(); AlongX(1600, 0, 1200, { 300 }); AlongX(1600, 1600, 2800, { 2400 }); Level(N0, -Every, BHi);
		// the stair's doors S1a/S1b across the corridor at py 1800 and the spine wall between the flights
		AlongX(1800, 1200, 1600, { 1400 });
		constexpr double TopY = 1800 + 7.5 + 150, MidY = TopY + 28 * 9;   // the top landing's end, the mid landing (caliche_school.py)
		S.Walls.Add({ PX(TopY), PX(MidY), PZ(1400) - .08, PZ(1400) + .08 });
		// Tom, d60 ("fiks trappa"): the steel rail across the well's open edge on the top landing (the east half, over the lower flight),
		// on the ground floor only; and under the upper flight the nook is walled off in the basement (no headroom there).
		N0 = S.Walls.Num(); Rect(TopY - 5, TopY + 5, 1400, 1600); Level(N0, GLo, Every);
		N0 = S.Walls.Num(); Rect(TopY, TopY + 10, 1200, 1400); Level(N0, -Every, BHi);
		// the portal's wall before E1 and the pipe rails down its steps
		Rect(-40, 0, 1100, 1300); Rect(-40, 0, 1500, 1700); Rect(-352, -40, 1094, 1098); Rect(-352, -40, 1702, 1706);
		// E3's deck over the areaway: rails on its east side (down the steps too) and its north end, for one on the deck or in the yard
		N0 = S.Walls.Num(); Rect(1876, 2200, 2958, 2962); Rect(2198, 2202, 2800, 2962); Level(N0, GLo, Every);
		// E4's areaway and S2: the east retaining wall with its railing, the rail on the west side north of the building; the wall under
		// the deck's south edge in the areaway. The stair's top (py 2620 on) is open to the yard.
		Rect(1980, 2640, 3000, 3020); Rect(2400, 2600, 2780, 2800);
		N0 = S.Walls.Num(); Rect(1980, 2000, 2800, 3020); Level(N0, -Every, BHi);
		// Tom, d60 ("lag kjellervinduer"): the light wells before the basement windows, a curb and a rail round each (caliche_school.py WELLS)
		for (const double Py : { 1400.0, 1800.0, 2300.0 }) Rect(Py - 80, Py + 80, -90, 0);            // west wall: the storage, the boiler room
		for (const double Px : { 400.0, 900.0, 1900.0, 2400.0 }) Rect(2400, 2490, Px - 80, Px + 80);  // north wall: the boiler room, the archive
		// (none on the east wall: the land there lies 0.3 m under the yard and would stand in a well to the window's head)
		// floors: the flights and the mid landing first (they win over the ground floor's rectangle), the exits' landings and steps, then
		// the ground floor over the whole plan, the basement under its north half, and the yard
		S.Floors.Add({ PX(TopY), PX(MidY), PZ(1200), PZ(1400), 0, -1.7, TEXT("x") });
		S.Floors.Add({ PX(TopY), PX(MidY), PZ(1400), PZ(1600), -3.4, -1.7, TEXT("x") });
		S.Floors.Add({ PX(MidY), PX(2400), PZ(1200), PZ(1600), -1.7, -1.7, TEXT("x") });
		// The yard: the pad's level (0.6 m under the ground floor, the spec's Z_TERRAIN) from py -600 to 2700, px -400 to 3000 (E2's steps end on it), with
		// banks 5 m wide down to the land beyond; the exits' steps end on the pad, and the walker has the pad and its banks as floors.
		const double Yard = S.ModelLift - S.ModelFloorUp;
		S.PadX0 = PX(-600); S.PadX1 = PX(2700); S.PadZ0 = PZ(-400); S.PadZ1 = PZ(3000); S.PadBank = 5;
		// The pad is drawn as a ring a metre off the walls (the model's own apron lies in that metre, with the wells and the areaway open in it)
		S.PadTops.Add({ PX(-600), PX(-100), PZ(-400), PZ(3000) });
		S.PadTops.Add({ PX(2660), PX(2700), PZ(-400), PZ(3000) });
		S.PadTops.Add({ PX(-100), PX(2660), PZ(-400), PZ(-100) });
		const auto Pad = [&S](double MinX, double MaxX, double MinZ, double MaxZ, double Y0, double Y1, const TCHAR* Axis, bool bToLand = false)
		{
			FSite::FFloorPad R; R.MinX = MinX; R.MaxX = MaxX; R.MinZ = MinZ; R.MaxZ = MaxZ; R.Y0 = Y0; R.Y1 = Y1; R.Axis = Axis; R.bToLand = bToLand; S.Floors.Add(R);
		};
		// (as the model has them: a landing level with the floor, then the steps)
		Pad(PX(-240), PX(0), PZ(1100), PZ(1700), 0, 0, TEXT("x")); Pad(PX(-324), PX(-240), PZ(1100), PZ(1700), Yard, 0, TEXT("x"));       // E1
		Pad(PX(2000), PX(2200), PZ(-160), PZ(0), 0, 0, TEXT("x")); Pad(PX(2000), PX(2200), PZ(-244), PZ(-160), Yard, 0, TEXT("z"));       // E2, steps west
		Pad(PX(1960), PX(2200), PZ(2800), PZ(2960), 0, 0, TEXT("x")); Pad(PX(1876), PX(1960), PZ(2800), PZ(2960), Yard, 0, TEXT("x"));    // E3's deck, steps south along the wall
		Pad(PX(2000), PX(2200), PZ(2800), PZ(3000), -3.4, -3.4, TEXT("x")); Pad(PX(2200), PX(2620), PZ(2800), PZ(3000), -3.4, Yard, TEXT("x")); // E4's areaway under the deck, S2 north up to the yard
		S.Floors.Add({ PX(0), PX(2400), PZ(0), PZ(2800), 0, 0, TEXT("x") });
		S.Floors.Add({ PX(1200), PX(2400), PZ(0), PZ(2800), -3.4, -3.4, TEXT("x") });
		Pad(S.PadX0, S.PadX1, S.PadZ0, S.PadZ1, Yard, Yard, TEXT("x"));
		const double NaN = std::numeric_limits<double>::quiet_NaN(), Bk = S.PadBank;
		Pad(S.PadX1, S.PadX1 + Bk, S.PadZ0, S.PadZ1, Yard, NaN, TEXT("x"), true);   // the banks: east
		Pad(S.PadX0 - Bk, S.PadX0, S.PadZ0, S.PadZ1, NaN, Yard, TEXT("x"), true);   // west
		Pad(S.PadX0, S.PadX1, S.PadZ1, S.PadZ1 + Bk, Yard, NaN, TEXT("z"), true);   // south
		Pad(S.PadX0, S.PadX1, S.PadZ0 - Bk, S.PadZ0, NaN, Yard, TEXT("z"), true);   // north
		S.ModelFoot = { S.PadX0, S.PadX1, S.PadZ0, S.PadZ1 };
		// CALICHE-GROUND: no land inside the ring (the model's apron, its light wells, E4's areaway and S2 are open down to the basement's floor)
		S.GroundCuts.Add({ PX(-100), PX(2660), PZ(-100), PZ(3000) }); S.GroundCutY = -3.4;
		// lights: two fluorescent fixtures in each classroom, one in the office, the staff room, each WC and the stair, three down the
		// corridor; the basement rooms one each; and the lamp over E1 (the stand-in's stood 1.2 m inside the wall)
		// (modest: a night school lit by a few tubes left on, not a showroom; the first night run at 22:00 was twice this)
		const auto In = [&](double Px, double Py, double Over, float Lm = 210.f) { FSite::FLight L; L.X = PX(Py); L.Z = PZ(Px); L.Up = Over; L.Lumens = Lm; L.Reach = 7; S.Lights.Add(L); };
		for (const double Px : { 600.0, 2200.0 }) for (const double Py : { 1000.0, 1400.0, 1800.0, 2200.0 }) In(Px, Py, 3.3);
		In(600, 200, 3.3); In(600, 600, 3.3); In(2200, 200, 3.3, 130.f); In(2200, 600, 3.3, 130.f); In(1400, 2100, 3.3, 130.f);
		for (const double Py : { 300.0, 900.0, 1500.0 }) In(1400, Py, 3.3, 150.f);
		for (const double Px : { 600.0, 2200.0 }) for (const double Py : { 1400.0, 2000.0 }) In(Px, Py, -.7, 150.f);
		In(1400, 1500, -.7, 150.f);   // the basement corridor (dark in the first night run)
		{ FSite::FLight L; L.X = PX(-60); L.Z = PZ(1400); L.Up = 3.0; L.Lumens = 700.f; L.Reach = 16; L.bOutside = true; S.Lights.Add(L); }
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
			S.ModelX = -8; S.ModelZ = 0; S.ModelYaw = 90; S.ModelLift = 3.0; S.ModelFloorUp = 3.6;
			SchoolInside(S);
			// Stage 3 (Tom): the school's outside, from the same boxes the sites are made of, kept beside the model. A footpath from the
			// highway's shoulder to the pad's front; the playground of the eighties south-west of the building, off the pad (swings, a
			// merry-go-round, a see-saw, monkey bars), inside a chain-link frame (posts and a top rail: the mesh itself is not drawn) with
			// gaps on the pad side and the south, so the way round the building (E2, the yard, E3 and E4) stays open; two benches; a bike
			// rack by the front steps. Nothing with a maker's name.
			{
				constexpr uint32 Galv = 0x9a9d99, Rubber = 0x2a2a2a, Concrete = 0xa9a498, Wood = 0x7a5a3c, Paint = 0x9c3b2e;
				S.Yards.Add({ -28, -5, 16, 30 });   // the playground's ground: no shrubs through the swings, and the truck keeps off
				const auto Keep = [&S](FPart P, double Lift = 0, bool bRound = false) { P.Lift = Lift; P.bRound = bRound; P.bKeep = true; S.Parts.Add(P); };
				const auto Post = [&](double X, double Z, double H, double Dia = .06, uint32 C = Galv) { Keep(Box(X, Z, Dia, Dia, H, C, true), 0, true); };
				// the footpath: 1.5 m of concrete from x -42 (the highway's shoulder) to the pad's edge at -26
				Keep(Box(-34, 0, 16, 1.5, .08, Concrete, false));
				int32 G0 = S.Parts.Num();
				const auto Group = [&S, &G0](const TCHAR* Name) { for (int32 I = G0; I < S.Parts.Num(); ++I) S.Parts[I].Group = Name; G0 = S.Parts.Num(); };
				// swings: two frames' posts, a top bar, two seats on their chains (x -20, z 19 to 25)
				for (const double Z : { 19.0, 25.0 }) Post(-20, Z, 2.5, .09);
				Keep(Box(-20, 22, .09, 6.2, .09, Galv, false), 2.42);
				for (const double Z : { 20.8, 23.2 }) { Keep(Box(-20, Z, .5, .22, .05, Rubber, false), .5); for (const double DZ : { -.2, .2 }) Keep(Box(-20, Z + DZ, .03, .03, 1.85, Galv, false), .55); }
				Group(TEXT("swings"));
				// the merry-go-round: a disc on a centre post, four bars (x -14, z 22)
				Keep(Box(-14, 22, 3, 3, .08, Paint, false), .32, true); Post(-14, 22, 1.1, .1);
				Keep(Box(-14, 22, 2.6, .04, .04, Galv, false), .9); Keep(Box(-14, 22, .04, 2.6, .04, Galv, false), .9);
				Group(TEXT("merry"));
				// the see-saw: a plank over its fulcrum (x -9, z 19)
				Keep(Box(-9, 19, 3, .25, .06, Wood, false), .55); Keep(Box(-9, 19, .3, .5, .5, Galv, true));
				Group(TEXT("seesaw"));
				// monkey bars: four posts, two rails and the rungs (x -24, z 24 to 27)
				for (const double X : { -25.5, -22.5 }) for (const double Z : { 24.0, 27.0 }) Post(X, Z, 2.1, .06);
				for (const double Z : { 24.0, 27.0 }) Keep(Box(-24, Z, 3.1, .04, .04, Galv, false), 2.05);
				for (int32 I = 0; I < 7; ++I) Keep(Box(-25.5 + I * .5, 25.5, .03, 3.0, .03, Galv, false), 2.05);
				Group(TEXT("monkey"));
				// the fence frame: posts every 3 m with a top rail, along z 29 (x -27 to -6) and x -6 (z 17 to 29), gaps at z 20 to 23
				// and x -15 to -12 (the way in and the way out), the pad's side open
				for (double X = -27; X <= -6.01; X += 3) if (X < -15.5 || X > -11.5) Post(X, 29, 1.25);
				Keep(Box(-21.5, 29, 11, .04, .04, Galv, false), 1.2); Keep(Box(-8.5, 29, 5, .04, .04, Galv, false), 1.2);
				for (double Z = 17; Z <= 29.01; Z += 3) if (Z < 19.5 || Z > 23.5) Post(-6, Z, 1.25);
				Keep(Box(-6, 18.5, .04, 3, .04, Galv, false), 1.2); Keep(Box(-6, 26.5, .04, 5, .04, Galv, false), 1.2);
				G0 = S.Parts.Num();
				// two benches facing the playground (x -26, z 20 and -10, z 26.5), and the bike rack by the steps' north side
				for (const FVector2D& At : { FVector2D(-26, 20), FVector2D(-10, 26.5) })
				{
					Keep(Box(At.X, At.Y, .4, 1.8, .06, Wood, false), .44);
					for (const double DZ : { -.75, .75 }) Keep(Box(At.X, At.Y + DZ, .35, .06, .44, Galv, true));
				}
				Group(TEXT("benches"));
				S.DressFile = TEXT("school_cosmos.json");
				for (int32 I = 0; I < 6; ++I) Keep(Box(-23.5, -4.5 + I * .5, .04, .04, .8, Galv, false), 0, true);
				Keep(Box(-23.5, -3.25, .04, 2.6, .04, Galv, false), .78);
			}
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
		else if (S.Id == TEXT("cemetery"))
		{
			// A small town's graveyard as New Mexico has them: a plot of bare ground 22 m square behind the church, a low fence of iron
			// pipe rail round it, a gravel path from the church's rear corner in at the west gate and straight through to the back.
			// The stones and the bench are bought pieces (Config/Props/church_cosmos.json; a grey box each without them). Three ways
			// through the fence (AGENTS.md, the rule of two routes): the gate to the church, a gap in the east side to the open
			// land, and a panel gone in the north side where a footpath comes in. The land rises 0.6 m from north to south, as
			// it lies: nothing is levelled, each piece stands on the ground under it.
			S.Sign = TEXT("SAN ISIDRO CEMETERY"); S.Title = TEXT("the cemetery behind the church"); S.MapName = TEXT("San Isidro Cemetery");
			S.Traits = ES47PlaceTrait::Town | ES47PlaceTrait::Open;
			S.Radius = 16; S.Up = 20; S.NameSide = 1; S.bLandmark = true;
			S.PlaceX = 0; S.PlaceZ = 0; S.EntryX = -12.4; S.EntryZ = 0;
			constexpr double X0 = -11, X1 = 11, Z0 = -11, Z1 = 10, GateHalf = 1.1, NorthA = 3.0, NorthB = 4.8;
			constexpr uint32 Iron = 0x1d1d1b, Gravel = 0xa89e88, Dirt = 0x9a8a70;
			S.Yards.Add({ X0, X1, Z0, Z1 });
			const auto Add = [&S](double X, double Z, double W, double D, double H, uint32 Colour, bool bSolid, double Lift)
			{
				FPart P = Box(X, Z, W, D, H, Colour, bSolid); P.Lift = Lift; S.Parts.Add(P);
			};
			// A run of fence between two points of one side: a post every bay (2.2 m at most), three rails; the middle rail holds the walker.
			const auto Run = [&Add](double Ax, double Az, double Bx, double Bz)
			{
				const bool bAlongX = FMath::Abs(Bx - Ax) > FMath::Abs(Bz - Az);
				const double Len = bAlongX ? Bx - Ax : Bz - Az;
				const int32 Bays = FMath::Max(1, FMath::CeilToInt(Len / 2.2));
				for (int32 I = 0; I <= Bays; ++I)
				{
					const double T = double(I) / Bays, Px = Ax + (Bx - Ax) * T, Pz = Az + (Bz - Az) * T;
					Add(Px, Pz, .06, .06, 1.08, Iron, false, 0);
					if (I == Bays) break;
					const double Mx = Px + (Bx - Ax) * .5 / Bays, Mz = Pz + (Bz - Az) * .5 / Bays, L = Len / Bays;
					for (const double Up : { .22, .6, .98 }) Add(Mx, Mz, bAlongX ? L : .04, bAlongX ? .04 : L, .04, Iron, Up == .6, Up);
				}
			};
			Run(X0, Z0, X0, -GateHalf); Run(X0, GateHalf, X0, Z1);       // west, the gate between
			Run(X1, Z0, X1, -GateHalf); Run(X1, GateHalf, X1, Z1);       // east, the gap to the open land
			Run(X0, Z0, NorthA, Z0); Run(NorthB, Z0, X1, Z0);            // north, a panel gone
			Run(X0, Z1, X1, Z1);                                         // south, whole (the neighbour's yard)
			// The gate: two taller posts with a bar over them, both leaves standing open into the plot.
			for (const double Gz : { -GateHalf, GateHalf })
			{
				Add(X0, Gz, .1, .1, 2.35, Iron, false, 0);
				const double Lz = Gz + (Gz < 0 ? .04 : -.04);
				for (const double Up : { .2, .98 }) Add(X0 + .55, Lz, 1.0, .03, .03, Iron, false, Up);
				for (const double Lx : { X0 + .3, X0 + .55, X0 + .8, X0 + 1.05 }) Add(Lx, Lz, .03, .03, .81, Iron, false, .2);
			}
			Add(X0, 0, .05, 2 * GateHalf, .05, Iron, false, 2.3);
			// The paths: gravel from the church's south-east corner along its rear wall, in at the gate and through to the east gap;
			// a trodden footpath from the gap in the north side. Short pieces, so each lies on the land.
			const int32 Path0 = S.Parts.Num();
			// (the gravel's picture thins out at its edges, so the pieces lie half over each other and the path is whole)
			for (double Px = X0 - 1.25; Px < X1 - .4; Px += 1.5) Add(Px, 0, 3.2, 2.0, .03, Gravel, false, 0);   // from 0.15 m off the church's rear wall to just out of the east gap
			for (double Pz = 1.2; Pz < 5.6; Pz += 1.4) Add(X0 - 1.5, Pz, 1.8, 3.0, .03, Gravel, false, 0);
			for (double Pz = Z0 - .6; Pz < -1.5; Pz += 1.3) Add((NorthA + NorthB) * .5, Pz, 1.3, 2.8, .02, Dirt, false, 0);
			for (int32 I = Path0; I < S.Parts.Num(); ++I) S.Parts[I].bGround = true;
			// The name, on a board on the fence north of the gate, read from the church's side.
			Add(X0 - .04, -2.2, .03, 1.5, .5, Board, false, .7);
			S.Words.Add({ TEXT("SAN ISIDRO\nCEMETERY"), X0 - .07, -2.2, .95, 270, .12, true });
			S.DressFile = TEXT("church_cosmos.json");
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

namespace S47TownSitesLocal { TSet<FString> ModelsUpSet; }
const TSet<FString>& S47TownSites::ModelsUp() { return S47TownSitesLocal::ModelsUpSet; }

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

bool S47TownSites::ReadDress(const FString& File, TArray<FSite::FDress>& Out, FString* Error)
{
	const auto Fail = [&](const FString& Why) { if (Error) *Error = File + TEXT(": ") + Why; return false; };
	FString Text;
	TSharedPtr<FJsonObject> J;
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectConfigDir() / TEXT("Props") / File))) return Fail(TEXT("not in Config/Props"));
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) || !J.IsValid()) return Fail(TEXT("does not read"));
	const TArray<TSharedPtr<FJsonValue>>* Pieces = nullptr;
	if (!J->TryGetArrayField(TEXT("pieces"), Pieces)) return Fail(TEXT("has no pieces"));
	for (const TSharedPtr<FJsonValue>& V : *Pieces)
	{
		const TSharedPtr<FJsonObject> P = V->AsObject();
		const TArray<TSharedPtr<FJsonValue>>* At = nullptr;
		if (!P.IsValid() || !P->TryGetArrayField(TEXT("at"), At) || At->Num() != 2) return Fail(TEXT("a piece has no at [x, z]"));
		FSite::FDress D;
		P->TryGetStringField(TEXT("id"), D.Id); P->TryGetStringField(TEXT("asset"), D.Asset); P->TryGetStringField(TEXT("replaces"), D.Replaces);
		// PROPS-REUSE-20261113: or a shared catalogue model by its id (Config/Props/catalogue.json)
		if (FString Model; D.Asset.IsEmpty() && P->TryGetStringField(TEXT("model"), Model)) D.Asset = S47PropCatalogue::Resolve(Model);
		D.X = (*At)[0]->AsNumber(); D.Z = (*At)[1]->AsNumber();
		P->TryGetNumberField(TEXT("yaw"), D.Yaw); P->TryGetNumberField(TEXT("scale"), D.Scale); P->TryGetBoolField(TEXT("collide"), D.bCollide);
		const TArray<TSharedPtr<FJsonValue>>* BoxOf = nullptr;
		if (P->TryGetArrayField(TEXT("box"), BoxOf) && BoxOf->Num() == 3) { D.BoxW = (*BoxOf)[0]->AsNumber(); D.BoxD = (*BoxOf)[1]->AsNumber(); D.BoxH = (*BoxOf)[2]->AsNumber(); }
		Out.Add(MoveTemp(D));
	}
	return true;
}

FBox2D S47TownSites::ModelRect(const FSite& S, const FSite::FWall& R)
{
	// a rectangle of the site's metres in the road's (the site standing square: its corners turned, then the box round them)
	FBox2D Out(ForceInit);
	Out += ToRoad(S, R.MinX, R.MinZ); Out += ToRoad(S, R.MaxX, R.MaxZ);
	return Out;
}

bool S47TownSites::WalkOver(double MinX, double MaxX, double MinZ, double MaxZ)
{
	static const bool bOld = FParse::Param(FCommandLine::Get(), TEXT("S47SchoolWalkOld")); // (the walk test's proof: d60 as it was)
	if (bOld) return false;
	const FVector Road = S47Legs::ORIGINS(TEXT("road"));
	const auto Same = [&](const FBox2D& B) { return FMath::Abs(Road.X + B.Min.X - MinX) < .01 && FMath::Abs(Road.X + B.Max.X - MaxX) < .01 && FMath::Abs(Road.Z + B.Min.Y - MinZ) < .01 && FMath::Abs(Road.Z + B.Max.Y - MaxZ) < .01; };
	for (const FSite& S : Sites())
	{
		if (!ModelsUp().Contains(S.Id)) continue;
		if (Same(ModelRect(S, S.ModelFoot))) return true;
		for (const FSite::FWall& Y : S.Yards) if (Same(ModelRect(S, Y))) return true;
	}
	return false;
}

TArray<FBox2D> S47TownSites::SolidFootprints()
{
	TArray<FBox2D> Out;
	for (const FSite& S : Sites())
	{
		const bool bModel = ModelsUp().Contains(S.Id);
		for (const FPart& P : S.Parts) if (P.bSolid && (!bModel || P.bKeep)) Out.Append(Footprint(S, P));
		if (bModel) { Out.Add(ModelRect(S, S.ModelFoot)); for (const FSite::FWall& Y : S.Yards) Out.Add(ModelRect(S, Y)); }
	}
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
	Check(Sites().Num() == 6 && Find(TEXT("trailers")) && Find(TEXT("school")) && Find(TEXT("grange")) && Find(TEXT("baca")) && Find(TEXT("breather")) && Find(TEXT("cemetery")), TEXT("the trailer park, the school, the Grange hall, the Baca place, the Breather and the cemetery are in the table"));
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
	// SCHOOL-PLAYGROUND: the bought pieces' file (passes with and without the assets)
	for (const FSite& S : Sites()) if (!S.DressFile.IsEmpty())
	{
		TArray<FSite::FDress> Dress; FString Error;
		const bool bRead = ReadDress(S.DressFile, Dress, &Error);
		bool bGood = bRead && Dress.Num() > 0; int32 Here = 0;
		TSet<FString> Ids, Groups;
		for (const FPart& P : S.Parts) if (!P.Group.IsEmpty()) Groups.Add(P.Group);
		for (const FSite::FDress& D : Dress)
		{
			bGood &= !D.Id.IsEmpty() && !Ids.Contains(D.Id) && D.Asset.StartsWith(TEXT("/Game/Licensed/Cosmos")) && D.Scale > .5 && D.Scale < 1.5 && (D.Replaces.IsEmpty() || Groups.Contains(D.Replaces));
			// on the yard or the pad, outside the building
			bGood &= S.Yards.ContainsByPredicate([&D](const FSite::FWall& Y) { return D.X > Y.MinX && D.X < Y.MaxX && D.Z > Y.MinZ && D.Z < Y.MaxZ; });
			Ids.Add(D.Id);
			Here += FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(D.Asset)) ? 1 : 0;
		}
		Check(bGood, FString::Printf(TEXT("%s: %s reads%s: %d pieces with an id, a licensed path and a sane scale, each on the site's own yard, each standing in for a known group of boxes or none; %d of the assets are here"), *S.Id, *S.DressFile, bRead ? TEXT("") : *(TEXT(" NOT (") + Error + TEXT(")")), Dress.Num(), Here));
	}
	// CHURCH: the cemetery is not a trap: three ways through its fence a walker fits (0.3 m each side), and whole runs between them.
	if (const FSite* C = Find(TEXT("cemetery")))
	{
		TArray<FBox2D> Fence;
		for (const FPart& P : C->Parts) if (P.bSolid) Fence.Append(Footprint(*C, P));
		const auto Open = [&](double X, double Z) { const FVector2D At = ToRoad(*C, X, Z); return !Fence.ContainsByPredicate([&At](const FBox2D& B) { return B.ExpandBy(.3).IsInside(At); }); };
		const int32 Ways = (Open(-11, 0) ? 1 : 0) + (Open(11, 0) ? 1 : 0) + (Open(3.9, -11) ? 1 : 0);
		const bool bHeld = !Open(-11, -6) && !Open(11, 6) && !Open(-4, -11) && !Open(0, 10) && !Open(-11, 5);
		const FBox2D Yard = C->Yards.Num() ? ModelRect(*C, C->Yards[0]) : FBox2D(ForceInit);
		Check(C->bLandmark && Ways == 3 && bHeld && Yard.Min.X > 35.5 && Yard.Max.Y < 2108,
			FString::Printf(TEXT("the cemetery: behind the church (road x %.1f to %.1f, z %.1f to %.1f), %d ways through its fence (the gate, the east gap, the north gap), the rest of it whole"), Yard.Min.X, Yard.Max.X, Yard.Min.Y, Yard.Max.Y, Ways));
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
	// CHURCH: the lot gravel S47Lived lays in front of Caliche's buildings, for a path on the land
	const TCHAR* const GravelPath = TEXT("/Game/Lived/Town/Materials/M_Lived_LotGravel");
	UMaterialInterface* GravelDecal = FPackageName::DoesPackageExist(GravelPath) ? LoadObject<UMaterialInterface>(nullptr, *(FString(GravelPath) + TEXT(".M_Lived_LotGravel"))) : nullptr;
	int32 Models = 0;
	S47TownSitesLocal::ModelsUpSet.Reset();
	DressedGroups.Reset();
	for (const FSite& S : Sites())
	{
		// CHURCH: a landmark's own yard is kept bare of plants as the land lies (no pad)
		if (S.bLandmark && S.Yards.Num() && S47TownSitesLocal::Square(S.Yaw))
		{
			TArray<S47GroundEdits::FRect> Bare;
			for (const FSite::FWall& Y : S.Yards) { const FBox2D B = ModelRect(S, Y); S47GroundEdits::FRect O; O.X = B.GetCenter().X; O.Z = B.GetCenter().Y; O.HalfX = B.GetExtent().X + .5; O.HalfZ = B.GetExtent().Y + .5; Bare.Add(O); }
			S47GroundEdits::SetBare(S.Id, Bare);
		}
		// SCHOOL-PLAYGROUND: the bought pieces that are here, each on the land where the file says; the boxes they stand in for go unseen.
		if (!S.DressFile.IsEmpty())
		{
			FString Error;
			TArray<FSite::FDress> Dress;
			if (!ReadDress(S.DressFile, Dress, &Error)) UE_LOG(LogS47TownSites, Warning, TEXT("S47: temp sites: %s: %s"), *S.Id, *Error);
			TArray<FString> Missing;
			int32 Up = 0;
			for (int32 I = 0; I < Dress.Num(); ++I)
			{
				const FSite::FDress& D = Dress[I];
				UStaticMesh* Mesh = FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(D.Asset)) ? LoadObject<UStaticMesh>(nullptr, *D.Asset) : nullptr;
				// (CHURCH: a piece with a box stands as that box while its asset is not here)
				const bool bBox = !Mesh && D.BoxH > 0;
				if (!Mesh) { Missing.Add(D.Id); if (!bBox) continue; Mesh = Cube; }
				UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, *FString::Printf(TEXT("S47SiteDress_%s_%s"), *S.Id, *D.Id));
				C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(W->GetRootComponent());
				C->SetStaticMesh(Mesh); C->SetCastShadow(true);
				if (bBox) C->SetMaterial(0, Mat(0x8f8d86));
				C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore);
				C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
				C->RegisterComponent();
				FBuiltPart B; B.Site = S.Id; B.Dress = I; B.Comp = C; Parts.Add(B);
				if (bBox) continue;
				if (!D.Replaces.IsEmpty()) DressedGroups.Add(S.Id + TEXT(":") + D.Replaces);
				++Up;
			}
			S47TownSitesLocal::DressOf.Add(S.Id, Dress);
			if (Missing.Num()) UE_LOG(LogS47TownSites, Display, TEXT("S47: temp sites: %s: %d of %d Cosmos pieces not here (Tools/cosmosplayground), the boxes stand for them: %s"), *S.Id, Missing.Num(), Dress.Num(), *FString::Join(Missing, TEXT(", ")));
			if (Up) UE_LOG(LogS47TownSites, Display, TEXT("S47: temp sites: %s: %d Cosmos pieces stand on the site, in place of %d groups of boxes"), *S.Id, Up, DressedGroups.Num());
		}
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
			if (S47TownSitesLocal::Square(S.Yaw)) S47TownSitesLocal::ModelsUpSet.Add(S.Id);
			if (S.PadX1 > S.PadX0)
				for (int32 Side = -FMath::Max(1, S.PadTops.Num()); Side < 4; ++Side)
				{
					// (the top's pieces first, then the four banks)
					UStaticMeshComponent* PC = NewObject<UStaticMeshComponent>(W, *FString::Printf(TEXT("S47Site_%s_Pad%d"), *S.Id, Side + 100));
					PC->SetMobility(EComponentMobility::Movable); PC->SetupAttachment(W->GetRootComponent());
					PC->SetStaticMesh(Cube); PC->SetMaterial(0, Mat(S.PadColour)); PC->SetCastShadow(true);
					PC->SetCollisionEnabled(ECollisionEnabled::QueryOnly); PC->SetCollisionResponseToAllChannels(ECR_Ignore);
					PC->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); PC->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
					PC->RegisterComponent();
					FBuiltPart PB; PB.Site = S.Id; PB.PadSide = FMath::Max(Side, -1); PB.PadPiece = Side < 0 ? -Side - 1 : 0; PB.Comp = PC; Parts.Add(PB);
				}
			const FBox MB = Model->GetBoundingBox();
			UE_LOG(LogS47TownSites, Display, TEXT("S47: temp sites: %s stands as its model (bounds x %.0f..%.0f y %.0f..%.0f z %.0f..%.0f cm; %d walls, %d floors, %d lights)"), *S.Id, MB.Min.X, MB.Max.X, MB.Min.Y, MB.Max.Y, MB.Min.Z, MB.Max.Z, S.Walls.Num(), S.Floors.Num(), S.Lights.Num());
		}
		for (int32 I = 0; I < S.Parts.Num(); ++I)
		{
			const FPart& P = S.Parts[I];
			const FString Name = FString::Printf(TEXT("S47TempSite_%s_%d"), *S.Id, I);
			if (P.bGround && GravelDecal)
			{
				UDecalComponent* D = NewObject<UDecalComponent>(W, *Name);
				D->SetupAttachment(W->GetRootComponent()); D->SetDecalMaterial(GravelDecal); D->SetFadeScreenSize(0.f); D->SortOrder = -1;
				D->RegisterComponent();
				FBuiltPart B; B.Site = S.Id; B.Part = I; B.Comp = D; Parts.Add(B);
				continue;
			}
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, *Name);
			C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(W->GetRootComponent());
			C->SetStaticMesh(P.bRound ? Cyl : Cube); C->SetMaterial(0, Mat(P.Colour)); C->SetCastShadow(true);
			// The view, the camera's sight and a use ray stop at a wall; the walker and the truck are held by the colliders below.
			C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore);
			C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
			C->RegisterComponent();
			FBuiltPart B; B.Site = S.Id; B.Part = I; B.bByModel = (Model && !P.bKeep) || (!P.Group.IsEmpty() && DressedGroups.Contains(S.Id + TEXT(":") + P.Group)); B.Comp = C; Parts.Add(B);
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
	// (the model first: it gives the land its pad, and what stands on the yard is seated on that)
	for (int32 Pass = 0; Pass < 2; ++Pass)
	for (const FBuiltPart& B : Parts)
	{
		if (B.Site != Site || (Pass == 0) != B.bModel) continue;
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
				// (the land as it is without any site's pad: S47GroundEdits changes SurfaceY under this very model)
				Hi = FMath::Max(Hi, Road.Y + S47RoadTerrain::NaturalY(At.X, At.Y));
			}
			const FVector2D At = ToRoad(*S, S->ModelX, S->ModelZ);
			C->SetWorldLocationAndRotation(S47Three::ToUE(FVector(Road.X + At.X, Hi - S->ModelLift, Road.Z + At.Y)), FRotator(0, S->Yaw + S->ModelYaw, 0));
			// SCHOOL-V2 stage 3: the walker's floors inside, over the ground floor (Hi - ModelLift + ModelFloorUp), laid down again at
			// each seating; the exits' steps run down to the land at their far end.
			const double FloorY = Hi - S->ModelLift + S->ModelFloorUp;
			ModelFloorY.Add(S->Id, FloorY);
			// CALICHE-GROUND: the pad, the yards beside it and the cuts go to the land itself (S47GroundEdits: SurfaceY follows them while
			// the relief is on). bLand: the installed ground mesh holds them too, so the land draws the pad and its banks, not boxes.
			bool bLand = false;
			if (S->PadX1 > S->PadX0 && S47TownSitesLocal::Square(S->Yaw))
			{
				const auto RectOf = [&](const FSite::FWall& R) { const FBox2D B = ModelRect(*S, R); S47GroundEdits::FRect O; O.X = B.GetCenter().X; O.Z = B.GetCenter().Y; O.HalfX = B.GetExtent().X; O.HalfZ = B.GetExtent().Y; return O; };
				TArray<S47GroundEdits::FPad> Pads; TArray<S47GroundEdits::FCut> Cuts;
				constexpr double LandUnder = .02;   // the land lies 2 cm under the yard's level: the model's apron is at that level and would flicker with it
				{ S47GroundEdits::FPad P; P.R = RectOf({ S->PadX0, S->PadX1, S->PadZ0, S->PadZ1 }); P.Y = Hi - Road.Y - LandUnder; P.Bank = S->PadBank; Pads.Add(P); }
				for (const FSite::FWall& Y : S->Yards) { S47GroundEdits::FPad P; P.R = RectOf(Y); P.Y = Hi - Road.Y - LandUnder; P.Bank = 3; Pads.Add(P); }
				for (const FSite::FWall& G : S->GroundCuts) { S47GroundEdits::FCut K; K.R = RectOf(G); K.Y = FloorY + S->GroundCutY - Road.Y; Cuts.Add(K); }
				S47GroundEdits::Set(S->Id, Pads, Cuts);
				S47FreeformTerrain::SitesChanged(M->Web.Get());
				bLand = S47GroundEdits::Baked(S->Id);
			}
			if (S->PadX1 > S->PadX0)
			{
				// the pad's top level with the yard (Hi), 2 m thick; a bank on each side where the land is more than a step lower
				const bool bQuarter = FMath::IsNearlyEqual(FMath::Fmod(FMath::Abs(S->Yaw), 180.0), 90.0, 1e-6);
				for (FBuiltPart& PB : Parts)
				{
					UStaticMeshComponent* PC = PB.Site == Site && PB.PadSide > -2 ? Cast<UStaticMeshComponent>(PB.Comp.Get()) : nullptr;
					if (!PC) continue;
					const double CX = (S->PadX0 + S->PadX1) * .5, CZ = (S->PadZ0 + S->PadZ1) * .5, WX = S->PadX1 - S->PadX0, WZ = S->PadZ1 - S->PadZ0;
					if (PB.PadSide < 0)
					{
						// (2 m thick: 5 where it is a ring, so its inner sides stand as the ground's wall behind the model's wells and areaway)
						const bool bRing = S->PadTops.IsValidIndex(PB.PadPiece);
						const FSite::FWall Piece = bRing ? S->PadTops[PB.PadPiece] : FSite::FWall{ S->PadX0, S->PadX1, S->PadZ0, S->PadZ1 };
						const double Deep = bRing ? 5.0 : 2.0, PWX = Piece.MaxX - Piece.MinX, PWZ = Piece.MaxZ - Piece.MinZ;
						const FVector2D PAt = ToRoad(*S, (Piece.MinX + Piece.MaxX) * .5, (Piece.MinZ + Piece.MaxZ) * .5);
						// (bLand: the land lies on it; it stays 3 cm under as the ground's wall behind the wells and the areaway)
						PC->SetWorldLocationAndRotation(S47Three::ToUE(FVector(Road.X + PAt.X, Hi - Deep * .5 - (bLand ? .06 : 0), Road.Z + PAt.Y)), FRotator(0, S->Yaw, 0));
						PC->SetWorldScale3D(FVector(bQuarter ? PWZ : PWX, bQuarter ? PWX : PWZ, Deep));
						PC->SetHiddenInGame(false);
						continue;
					}
					// the bank: outward along the side, from the pad's edge down to the land PadBank out (the land sampled there)
					const double OutX = PB.PadSide == 0 ? 1 : PB.PadSide == 1 ? -1 : 0, OutZ = PB.PadSide == 2 ? 1 : PB.PadSide == 3 ? -1 : 0;
					const double EdgeX = OutX > 0 ? S->PadX1 : OutX < 0 ? S->PadX0 : CX, EdgeZ = OutZ > 0 ? S->PadZ1 : OutZ < 0 ? S->PadZ0 : CZ;
					const double L = S->PadBank;
					const FVector2D Far = ToRoad(*S, EdgeX + OutX * L, EdgeZ + OutZ * L);
					const double Drop = Hi - Ground(Far.X, Far.Y, bWithFloor);
					if (Drop < .25 || bLand) { PC->SetHiddenInGame(true); PC->SetCollisionEnabled(ECollisionEnabled::NoCollision); continue; }
					PC->SetCollisionEnabled(ECollisionEnabled::QueryOnly); PC->SetHiddenInGame(false);
					const FVector2D Mid = ToRoad(*S, EdgeX + OutX * L * .5, EdgeZ + OutZ * L * .5);
					const FVector2D OutDir = S47TownSitesLocal::Turn(OutX, OutZ, S->Yaw);
					const FVector Along = S47Three::DirToUE(FVector(OutDir.X * L, -Drop, OutDir.Y * L)).GetSafeNormal();
					FVector Up = FVector::CrossProduct(FVector::CrossProduct(Along, FVector::UpVector), Along).GetSafeNormal();
					if (Up.Z < 0) Up = -Up;
					const FRotator Rot = FRotationMatrix::MakeFromXZ(Along, Up).Rotator();
					const double Len = FMath::Sqrt(L * L + Drop * Drop) + .4, Wide = OutX != 0 ? WZ + 2 * L : WX + 2 * L;
					PC->SetWorldLocationAndRotation(S47Three::ToUE(FVector(Road.X + Mid.X, Hi - Drop * .5 - .35, Road.Z + Mid.Y)), Rot);
					PC->SetWorldScale3D(FVector(Len, Wide, .7));
				}
			}
			const FString Tag = TEXT("site:") + S->Id;
			AS47Props::RemoveFloors(Tag);
			for (int32 FI = S->Floors.Num() - 1; FI >= 0; --FI)
			{
				const FSite::FFloorPad& F = S->Floors[FI];
				const FBox2D FB = ModelRect(*S, { F.MinX, F.MaxX, F.MinZ, F.MaxZ });
				const bool bZ = F.Axis == TEXT("z");
				double Y0 = F.Y0, Y1 = F.Y1;
				if (F.bToLand && bLand) continue; // (the bank is the land's own: SurfaceY has it)
				if (F.bToLand)
				{
					const FVector2D Lo(bZ ? FB.GetCenter().X : FB.Min.X, bZ ? FB.Min.Y : FB.GetCenter().Y), HiEnd(bZ ? FB.GetCenter().X : FB.Max.X, bZ ? FB.Max.Y : FB.GetCenter().Y);
					if (FMath::IsNaN(Y0)) Y0 = Ground(Lo.X, Lo.Y, bWithFloor) - FloorY;
					if (FMath::IsNaN(Y1)) Y1 = Ground(HiEnd.X, HiEnd.Y, bWithFloor) - FloorY;
				}
				// (the site turned: the rectangle's low end of its axis may have swapped with its high end)
				const bool bFlip = S47TownSitesLocal::Square(S->Yaw) && (bZ ? ToRoad(*S, F.MinX, F.MinZ).Y > ToRoad(*S, F.MaxX, F.MaxZ).Y : ToRoad(*S, F.MinX, F.MinZ).X > ToRoad(*S, F.MaxX, F.MaxZ).X);
				if (bFlip) Swap(Y0, Y1);
				AS47Props::AddFloor(Road.X + FB.Min.X, Road.X + FB.Max.X, Road.Z + FB.Min.Y, Road.Z + FB.Max.Y, FloorY + Y0, FloorY + Y1, bZ ? TEXT("z") : TEXT("x"), true, Tag);
			}
			WallLevels(*S);
		}
		else if (B.PadSide > -2) {} // the pad and its banks: placed with the model above
		else if (B.Dress >= 0)
		{
			const TArray<FSite::FDress>* Dress = S47TownSitesLocal::DressOf.Find(Site);
			UStaticMeshComponent* DC = Cast<UStaticMeshComponent>(C);
			if (!Dress || !Dress->IsValidIndex(B.Dress) || !DC || !DC->GetStaticMesh()) continue;
			const FSite::FDress& D = (*Dress)[B.Dress];
			// the bottom centre of its turned and scaled bounds on the land at its place (the packs' pivots are not in the middle)
			const FRotator Rot(0, S->Yaw + D.Yaw, 0);
			// (a stand-in box: the engine's metre cube stretched to the piece's size)
			const FVector Size = DC->GetStaticMesh()->GetPathName() == S47TownSitesLocal::CubePath ? FVector(D.BoxW, D.BoxD, D.BoxH) : FVector(D.Scale);
			const FBox Bd = DC->GetStaticMesh()->GetBoundingBox().TransformBy(FTransform(Rot, FVector::ZeroVector, Size));
			const FVector2D At = ToRoad(*S, D.X, D.Z);
			DC->SetWorldTransform(FTransform(Rot, S47Three::ToUE(FVector(Road.X + At.X, Ground(At.X, At.Y, bWithFloor), Road.Z + At.Y)) - FVector(Bd.GetCenter().X, Bd.GetCenter().Y, Bd.Min.Z), Size));
		}
		else if (!B.bWords)
		{
			const FPart& P = S->Parts[B.Part];
			const FVector2D At = ToRoad(*S, P.X, P.Z);
			// Standing parts go into the ground to the lowest corner; a raised one (the tank, the board) hangs over the highest.
			if (UDecalComponent* D = Cast<UDecalComponent>(C))
			{
				// thrown straight down on the land: the decal's y is the land's z, its z the land's x (S47Lived's lots); every other piece turned round
				D->DecalSize = FVector(80.f, float(P.D * 50), float(P.W * 50));
				D->SetWorldLocationAndRotation(S47Three::ToUE(FVector(Road.X + At.X, (Bottom[B.Part] + Top[B.Part]) * .5, Road.Z + At.Y)), FRotator(-90.f, 0.f, S->Yaw + (B.Part % 2 ? 180.f : 0.f)));
				continue;
			}
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

void FS47TownSites::WallLevels(const S47TownSites::FSite& S)
{
	// The walls' floors as world heights, from where the model's ground floor stands now (set again at each seating).
	const double* FY = ModelFloorY.Find(S.Id);
	if (!M || !M->Web || !FY) return;
	const FString Head = FString::Printf(TEXT("temp:%s:wall:"), *S.Id);
	for (FS47Rect& R : M->Web->CollidersMutable(TEXT("road")))
	{
		if (!R.Id.StartsWith(Head)) continue;
		const int32 I = FCString::Atoi(*R.Id.Mid(Head.Len()));
		if (!S.Walls.IsValidIndex(I)) continue;
		R.MinY = S.Walls[I].Lo < -1e8 ? -1e9 : *FY + S.Walls[I].Lo;
		R.MaxY = S.Walls[I].Hi > 1e8 ? 1e9 : *FY + S.Walls[I].Hi;
	}
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
		const bool bModel = ModelsUp().Contains(S.Id);
		if (bModel && I == 0)
		{
			// SCHOOL-V2 stage 3: the model's walls, the doors as gaps, in the stand-in boxes' place
			int32 KW = 0;
			for (const FSite::FWall& Wl : S.Walls)
			{
				const FBox2D B = ModelRect(S, Wl);
				FS47Rect R; R.MinX = Road.X + B.Min.X; R.MaxX = Road.X + B.Max.X; R.MinZ = Road.Z + B.Min.Y; R.MaxZ = Road.Z + B.Max.Y;
				R.Id = FString::Printf(TEXT("temp:%s:wall:%d"), *S.Id, KW++); R.bEnabled = true;
				Walk.Add(R);
			}
			WallLevels(S);
		}
		if (bModel && I == 0 && S.Id == TEXT("school"))
		{
			// Evidence for the log: the walker's own lists at the school's key spots (the ground floor stands FloorUp over the yard at E1).
			const auto Free = [&Walk, &Road](const FVector2D& P) { for (const FS47Rect& R : Walk) if (R.bEnabled && Road.X + P.X >= R.MinX && Road.X + P.X <= R.MaxX && Road.Z + P.Y >= R.MinZ && Road.Z + P.Y <= R.MaxZ) return false; return true; };
			const double* FY = ModelFloorY.Find(S.Id);
			const auto Spot = [&](const TCHAR* Name, double Py, double Px, double RefOver)
			{
				const FVector2D At = ToRoad(S, S.ModelX + (Py - 1200) * .01, S.ModelZ + (Px - 1400) * .01);
				const double X = Road.X + At.X, Z = Road.Z + At.Y;
				const double F = FY ? AS47Props::FloorAt(X, Z, *FY + RefOver) : TNumericLimits<double>::Max();
				return FString::Printf(TEXT("%s %s floor %+.2f"), Name, Free(At) ? TEXT("free") : TEXT("WALLED"), FY ? F - *FY : 99.0);
			};
			UE_LOG(LogS47TownSites, Display, TEXT("S47: temp sites: school inside: %s; %s; %s; %s; %s; %s; %s; wall test %s"),
				*Spot(TEXT("E1"), 0, 1400, 0), *Spot(TEXT("corridor"), 900, 1400, 0), *Spot(TEXT("K3"), 1100, 1200, 0), *Spot(TEXT("class1"), 1200, 600, 0),
				*Spot(TEXT("S1 mid-flight"), 2080, 1275, 0), *Spot(TEXT("S1 landing"), 2300, 1400, -1.7), *Spot(TEXT("basement corridor"), 1500, 1400, -3.4),
				Free(ToRoad(S, S.ModelX + (900 - 1200) * .01, S.ModelZ + (1200 - 1400) * .01)) ? TEXT("FAILED (the corridor wall is not there)") : TEXT("ok"));
		}
		if (I == 0)
			if (const TArray<FSite::FDress>* Dress = S47TownSitesLocal::DressOf.Find(S.Id))
				for (const FBuiltPart& B : Parts)
				{
					// a bought piece's walk block from its bounds (Unreal x is the world's x, Unreal y its z), 5 cm in
					UPrimitiveComponent* DC = B.Site == S.Id && Dress->IsValidIndex(B.Dress) && (*Dress)[B.Dress].bCollide ? Cast<UPrimitiveComponent>(B.Comp.Get()) : nullptr;
					if (!DC) continue;
					DC->UpdateBounds();
					const FBox Bd = DC->Bounds.GetBox();
					FS47Rect R; R.MinX = Bd.Min.X / 100.0 + .05; R.MaxX = Bd.Max.X / 100.0 - .05; R.MinZ = Bd.Min.Y / 100.0 + .05; R.MaxZ = Bd.Max.Y / 100.0 - .05;
					R.Id = FString::Printf(TEXT("temp:%s:dress:%s"), *S.Id, *(*Dress)[B.Dress].Id); R.bEnabled = true;
					if (R.MaxX > R.MinX && R.MaxZ > R.MinZ) Walk.Add(R);
				}
		if (!S.Parts[I].bSolid || (bModel && !S.Parts[I].bKeep) || (!S.Parts[I].Group.IsEmpty() && DressedGroups.Contains(S.Id + TEXT(":") + S.Parts[I].Group))) continue;
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
	for (const FBox2D& B : S47GroundEdits::ClearBoxes()) Under.Add(B.ExpandBy(.5)); // CALICHE-GROUND: pads, banks and cuts
	// (N1110: and the Breather's landing and surround, inside its fence.)
	for (const S47TownSites::FSite& S : S47TownSites::Sites()) if (S.bLandmark)
	{
		for (const S47TownSites::FPart& P : S.Parts) if (!P.bSolid && P.Lift <= 0) for (const FBox2D& B : S47TownSites::Footprint(S, P)) Under.Add(B.ExpandBy(.5));
		for (const S47TownSites::FSite::FWall& Y : S.Yards) Under.Add(S47TownSites::ModelRect(S, Y).ExpandBy(.5)); // CHURCH: no shrub among the graves
	}
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
		else if (UDecalComponent* D = Cast<UDecalComponent>(B.Comp.Get())) D->SetVisibility(bShow);
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
	const auto Add = [&](const FString& Site, double LocalX, double LocalZ, double Up, float Lumens, double Reach, double From, double To, const FLinearColor& Colour = FLinearColor(1.f, .78f, .52f), const double* OverY = nullptr, float Day = 0.f)
	{
		const FSite* S = Find(Site);
		if (!S) return;
		const FVector2D At = ToRoad(*S, LocalX, LocalZ);
		UPointLightComponent* C = NewObject<UPointLightComponent>(W, *FString::Printf(TEXT("S47TempLamp_%s_%d"), *Site, Lamps.Num()));
		C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(W->GetRootComponent());
		C->SetIntensityUnits(ELightUnits::Lumens); C->SetIntensity(Lumens); C->SetAttenuationRadius(float(Reach * 100.)); C->SetLightColor(Colour); C->SetCastShadows(false);
		C->RegisterComponent(); C->SetVisibility(false);
		C->SetWorldLocation(S47Three::ToUE(FVector(8000 + At.X, (OverY ? *OverY : S47FarmWorld::FloorRoad(At.X, At.Y)) + Up, At.Y)));
		FLamp L; L.Site = Site; L.L = C; L.From = From; L.To = To; L.Lumens = Lumens; L.Day = Day; Lamps.Add(L);
	};
	// SCHOOL-V2 stage 3: a site standing as its model lights its rooms from the plan (cool fluorescent tubes under the ceilings, the lamp
	// over the door warm), at heights over its ground floor; its stand-in lamps are left out.
	for (const FSite& S : Sites()) if (const double* FloorY = ModelFloorY.Find(S.Id))
		for (const FSite::FLight& Lt : S.Lights) Add(S.Id, Lt.X, Lt.Z, Lt.Up, Lt.Lumens, Lt.Reach, 19, 31, Lt.bOutside ? FLinearColor(1.f, .78f, .52f) : FLinearColor(.86f, .92f, 1.f), FloorY, Lt.bOutside ? 0.f : .45f);
	// The trailers' porch lights at the lane ends (north row, and two of the south row), until midnight; the court's yard light under
	// the tank and the school's over its front door, all night.
	for (const double X : { 38.0, 20.0, 2.0, -16.0 }) Add(TEXT("trailers"), X + 1.4, -6.4, 2.4, 260.f, 7, 19, 24);
	for (const double X : { 38.0, 2.0 }) Add(TEXT("trailers"), X - 1.4, 6.4, 2.4, 260.f, 7, 19, 24);
	Add(TEXT("trailers"), -40, 3.6, 13.2, 900.f, 26, 19, 31);
	if (!ModelFloorY.Contains(TEXT("school"))) Add(TEXT("school"), -18.8, 0, 3.6, 700.f, 16, 19, 31);
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
		const bool bHours = H >= L.From && H < L.To;
		C->SetVisibility(bHere && !bDark && (bHours || L.Day > 0.f));
		if (L.Day > 0.f && L.Lumens > 0.f) C->SetIntensity(L.Lumens * (bHours ? 1.f : L.Day));
	}
}

void FS47TownSites::Step(double Dt)
{
	if (!bActive) { if (M && M->IsFreeform() && IsValid(M->Web.Get())) Begin(); return; }
	if (!M || M->Web.Get() != Scene.Get()) { Begin(); return; }
	StepSchoolWalk(Dt);
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

// ---------- -S47SchoolWalkTest (Tom, d60: nobody got into the school; no human needed to know it again) ----------
// The walker itself, by its own keys' path (TestMove, the walker's Resolve and floors): from the footpath by the highway in at E1,
// the corridor, class 1 and D3, down S1 to the basement, the storage and the boiler room, up again, out E2, round the back and
// down S2 in at E4, out again and up E3's steps, and out the front. Points are the plan's centimetres (school_plan.py).
// A step: walk to the point (PASS: arrived, the floor as said); Hold: walk at it and be held (a rail, a wall).
struct FS47SchoolWalk
{
	struct FStep { const TCHAR* Name; double Px, Py, Floor; bool bHold = false; double Tol = .3; };
	TArray<FStep> Steps;
	int32 At = -1, Fails = 0, Passes = 0;
	double T = 0, Since = 0, Best = 1e9, BestAt = 0, StartFloor = 0, Begun = 0;
	bool bDone = false;
};

void FS47TownSites::StepSchoolWalk(double Dt)
{
	using namespace S47TownSites;
	static const bool bWanted = FParse::Param(FCommandLine::Get(), TEXT("S47SchoolWalkTest"));
	if (!bWanted || !M || !M->Player || !M->World || !M->Web) return;
	const FSite* S = Find(TEXT("school"));
	const double* FY = ModelFloorY.Find(TEXT("school"));
	if (!S) return;
	if (!SchoolWalk) SchoolWalk = MakeShared<FS47SchoolWalk>();
	FS47SchoolWalk& K = *SchoolWalk;
	if (K.bDone) return;
	K.T += Dt;
	if (K.T < 6) return;
	AS47Walker* P = M->Player;
	const FVector Road = S47Legs::ORIGINS(TEXT("road"));
	const auto World = [&](double Px, double Py) { const FVector2D R = ToRoad(*S, S->ModelX + (Py - 1200) * .01, S->ModelZ + (Px - 1400) * .01); return FVector2D(Road.X + R.X, Road.Z + R.Y); };
	const auto PlanOf = [&](double X, double Z) { return FVector2D((Z - Road.Z - S->Z - S->ModelZ) * 100 + 1400, (X - Road.X - S->X - S->ModelX) * 100 + 1200); };
	if (K.At < 0)
	{
		if (!FY || !ModelsUp().Contains(TEXT("school"))) { UE_LOG(LogS47TownSites, Warning, TEXT("S47: school walk test: FAIL, the school's model does not stand")); K.bDone = true; return; }
		constexpr double G = 0, Y = -.6, B = -3.4, L = -1.7;
		K.Steps = {
			{ TEXT("the yard before the front steps"), 1560, -450, Y }, { TEXT("E1's landing"), 1400, -150, G }, { TEXT("in through E1"), 1400, 120, G },
			{ TEXT("the corridor at K3"), 1400, 1100, G }, { TEXT("class 1 through K3"), 1000, 1100, G }, { TEXT("class 1 by D3"), 300, 1400, G },
			{ TEXT("class 3 through D3"), 300, 1750, G }, { TEXT("class 3 by K4"), 1000, 1700, G }, { TEXT("the corridor through K4"), 1400, 1700, G },
			{ TEXT("S1's top landing through S1a"), 1400, 1890, G },
			{ TEXT("held by the rail over the well"), 1500, 2080, G, true },
			{ TEXT("the landing's west half"), 1285, 1890, G }, { TEXT("down the upper flight to the mid landing"), 1285, 2300, L },
			{ TEXT("the mid landing's east half"), 1500, 2300, L }, { TEXT("down the lower flight"), 1500, 1900, B },
			{ TEXT("under the top landing's west half"), 1285, 1890, B }, { TEXT("held by the wall under the upper flight"), 1285, 2100, B, true },
			{ TEXT("before S1b"), 1400, 1890, B }, { TEXT("the basement corridor through S1b"), 1400, 1700, B }, { TEXT("the basement corridor at K9"), 1400, 1400, B },
			{ TEXT("held by the basement's south wall"), 1400, 1000, B, true },
			{ TEXT("back at K9"), 1400, 1400, B }, { TEXT("the storage through K9"), 1000, 1400, B }, { TEXT("the storage by D5"), 300, 1450, B },
			{ TEXT("the boiler room through D5"), 300, 1750, B }, { TEXT("the boiler room by K10"), 1000, 1700, B }, { TEXT("the basement corridor through K10"), 1400, 1700, B },
			{ TEXT("under the landing through S1b"), 1400, 1890, B }, { TEXT("the lower flight's foot"), 1500, 1900, B }, { TEXT("up the lower flight"), 1500, 2300, L },
			{ TEXT("the mid landing's west half"), 1285, 2300, L }, { TEXT("up the upper flight"), 1285, 1890, G }, { TEXT("S1a from the landing"), 1400, 1890, G },
			{ TEXT("the corridor again"), 1400, 1700, G }, { TEXT("class 3 through K4"), 1000, 1700, G }, { TEXT("class 3 by E2"), 150, 2100, G },
			{ TEXT("out E2 on its landing"), -90, 2100, G }, { TEXT("the yard west of E2"), -320, 2100, Y },
			{ TEXT("held by the rail of a light well"), -40, 1800, Y, true },
			{ TEXT("the yard's north-west corner"), -300, 2600, Y }, { TEXT("the yard behind the school"), 2900, 2690, Y },
			{ TEXT("down S2 to the areaway"), 2900, 2100, B }, { TEXT("the archive through E4"), 2600, 2100, B }, { TEXT("the archive by D6"), 2400, 1700, B },
			{ TEXT("the workshop through D6"), 2400, 1450, B }, { TEXT("the workshop by K11"), 1800, 1400, B }, { TEXT("the basement corridor through K11"), 1400, 1400, B },
			{ TEXT("the archive through K12"), 1800, 1700, B, false }, { TEXT("the archive by E4"), 2600, 2100, B }, { TEXT("out E4 in the areaway"), 2900, 2100, B },
			{ TEXT("up S2 to the yard"), 2900, 2690, Y }, { TEXT("east of the areaway's wall"), 3060, 2690, Y, false, .9 }, { TEXT("along the areaway's wall"), 3060, 1820, Y, false, .9 },
			{ TEXT("the foot of E3's steps"), 2880, 1820, Y }, { TEXT("E3's deck"), 2880, 2090, G },
			{ TEXT("held by the deck's rail"), 3100, 2090, G, true },
			{ TEXT("class 4 through E3"), 2600, 2100, G }, { TEXT("class 4 by D4"), 2400, 1700, G }, { TEXT("class 2 through D4"), 2400, 1450, G },
			{ TEXT("class 2 by K7"), 1800, 1100, G }, { TEXT("the corridor through K7"), 1400, 1100, G }, { TEXT("the front door from inside"), 1400, 120, G },
			{ TEXT("out E1"), 1400, -150, G }, { TEXT("the yard again"), 1560, -450, Y },
		};
		// (K10 and K12 are at py 1700; K12's row goes from K11's corridor point up the corridor first)
		K.Steps.Insert(FS47SchoolWalk::FStep{ TEXT("the basement corridor at K12"), 1400, 1700, B }, K.Steps.IndexOfByPredicate([](const FS47SchoolWalk::FStep& St) { return FCString::Strcmp(St.Name, TEXT("the archive through K12")) == 0; }));
		const FVector2D From = World(1560, -1050);
		if (M->Web->Area() != TEXT("diner")) M->World->Enter(TEXT("diner"));
		P->Place(From.X, From.Y, P->Yaw);
		K.At = 0; K.Since = K.T; K.Best = 1e9; K.BestAt = K.T; K.Begun = K.T;
		if (!SeatedSites.Contains(TEXT("school"))) Seat(TEXT("school"), true);
		FY = ModelFloorY.Find(TEXT("school"));
		// the land against the yard where the model is open to it: a well or the areaway shows the land in it if the land is higher than its floor
		FString Land;
		for (const FVector2D& Q : { FVector2D(-45, 1400), FVector2D(-45, 1800), FVector2D(-45, 2300), FVector2D(400, 2445), FVector2D(900, 2445), FVector2D(1900, 2445), FVector2D(2400, 2445), FVector2D(2845, 1400), FVector2D(2900, 2100), FVector2D(2900, 2500), FVector2D(1400, 1800), FVector2D(1400, -300) })
		{
			const FVector2D R = ToRoad(*S, S->ModelX + (Q.Y - 1200) * .01, S->ModelZ + (Q.X - 1400) * .01);
			Land += FString::Printf(TEXT(" (%.0f,%.0f) %+.2f"), Q.X, Q.Y, Road.Y + S47RoadTerrain::SurfaceY(R.X, R.Y) - (*FY - .6));
		}
		UE_LOG(LogS47TownSites, Display, TEXT("S47: school walk test: the land against the yard, metres, at plan points:%s"), *Land);
		UE_LOG(LogS47TownSites, Display, TEXT("S47: school walk test: begins on the footpath at %.1f %.1f in %s, floor %+.2f over the school's, %s; %d steps%s"), P->Pos.X, P->Pos.Z, *M->Web->Area(), P->FloorY - *FY,
			P->Walkable(P->Pos.X, P->Pos.Z) ? TEXT("walkable") : TEXT("NOT WALKABLE"), K.Steps.Num(), FParse::Param(FCommandLine::Get(), TEXT("S47SchoolWalkOld")) ? TEXT(" (as d60 was: the pad in the walker's way)") : TEXT(""));
		return;
	}
	if (!FY) return;
	if (K.At >= K.Steps.Num())
	{
		P->TestMove.Reset(); P->bTestRun = false; K.bDone = true;
		if (K.Fails) { UE_LOG(LogS47TownSites, Warning, TEXT("S47: school walk test: FAIL, %d of %d steps failed (%.0f s)"), K.Fails, K.Steps.Num(), K.T - K.Begun); }
		else { UE_LOG(LogS47TownSites, Display, TEXT("S47: school walk test: PASS, %d of %d steps (%.0f s)"), K.Passes, K.Steps.Num(), K.T - K.Begun); }
		return;
	}
	const FS47SchoolWalk::FStep& St = K.Steps[K.At];
	const FVector2D To = World(St.Px, St.Py);
	const double Dx = To.X - P->Pos.X, Dz = To.Y - P->Pos.Z, D = FMath::Sqrt(Dx * Dx + Dz * Dz);
	const FVector2D Plan = PlanOf(P->Pos.X, P->Pos.Z);
	const double Rel = P->FloorY - *FY;
	P->Yaw = FMath::Atan2(-Dx, -Dz); P->Pitch = -0.06; P->TestMove = FVector2D(0, -1); P->bTestRun = true;
	if (D < K.Best - .02) { K.Best = D; K.BestAt = K.T; }
	const auto Next = [&](bool bPass, const FString& Why)
	{
		if (bPass) { ++K.Passes; UE_LOG(LogS47TownSites, Display, TEXT("S47: school walk test: step %d PASS: %s: at plan %.0f %.0f, floor %+.2f%s"), K.At + 1, St.Name, Plan.X, Plan.Y, Rel, *Why); }
		else
		{
			++K.Fails;
			UE_LOG(LogS47TownSites, Warning, TEXT("S47: school walk test: step %d FAIL: %s: at plan %.0f %.0f, floor %+.2f, %.2f m short of %.0f %.0f%s"), K.At + 1, St.Name, Plan.X, Plan.Y, Rel, D, St.Px, St.Py, *Why);
			// put where the step would have ended (a hold: back where it began), so the rest of the way is still tried
			const FVector2D Put = St.bHold && K.At > 0 ? World(K.Steps[K.At - 1].Px, K.Steps[K.At - 1].Py) : To;
			P->FloorY = *FY + (St.bHold && K.At > 0 ? K.Steps[K.At - 1].Floor : St.Floor);
			P->Pos.X = Put.X; P->Pos.Z = Put.Y; P->MoveFrom = P->Pos;
		}
		++K.At; K.Since = K.T; K.Best = 1e9; K.BestAt = K.T;
	};
	if (St.bHold)
	{
		// held: 3 s of walking at it, never there, and the floor never left
		if (D < .45 || FMath::Abs(Rel - St.Floor) > .35) Next(false, D < .45 ? TEXT(" (walked through: nothing held)") : TEXT(" (fell off the floor)"));
		else if (K.T - K.Since > 3) Next(true, FString::Printf(TEXT(", held %.2f m short"), D));
		return;
	}
	if (D < .3)
	{
		if (FMath::Abs(Rel - St.Floor) < St.Tol || K.T - K.BestAt > 1.0) Next(FMath::Abs(Rel - St.Floor) < St.Tol, FMath::Abs(Rel - St.Floor) < St.Tol ? FString() : FString::Printf(TEXT(" (the floor should be %+.2f)"), St.Floor));
		return;
	}
	if (K.T - K.BestAt > 3.5) Next(false, FString::Printf(TEXT(" (held there; %s)"), P->Walkable(P->Pos.X + Dx / D * .5, P->Pos.Z + Dz / D * .5) ? TEXT("the ground ahead is walkable: a collider") : TEXT("the ground ahead is NOT WALKABLE")));
}
