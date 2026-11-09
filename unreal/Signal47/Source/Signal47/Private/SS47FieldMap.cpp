#include "SS47FieldMap.h"
#include "S47EventPlaces.h"
#include "S47FieldLife.h"
#include "S47FreeformWorld.h"
#include "S47Mail.h"
#include "S47Predecessor.h"
#include "S47FarmBuildings.h"
#include "S47TownSites.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47Geo.h"
#include "S47Main.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47Relief.h"
#include "S47RoadShape.h"
#include "S47RoadTerrain.h"
#include "S47Truck.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "SS47Widgets.h"

#include "Algo/Reverse.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "Input/Events.h"
#include "Layout/Clipping.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47FieldMap, Log, All);

// The sheet's page is 1200 x 800 units, scaled to the width it is given, as the pencil sketch's (SS47AreaMap) is. The map itself
// is inside the neat line (FrameL..FrameR, FrameT..FrameB); the margin round it has the title, the ticks, the scale and the
// declination, as a quadrangle's has.
namespace S47FieldMapLocal
{
	constexpr double FrameL = 30, FrameT = 34, FrameR = 852, FrameB = 772;
	// The legend's column, right of the neat line: the title, the scale, the road classification, the revision note, the desk's stamp.
	constexpr float PanelL = 872, PanelR = 1180;
	constexpr double FrameMidX = (FrameL + FrameR) * .5, FrameMidY = (FrameT + FrameB) * .5;
	constexpr double ScaleLeast = .1, ScaleMost = 9, ScaleFirst = .55;
	constexpr double MileM = 1609.344, FootM = .3048;
	// The road's datum above sea level: the flat at SARO is about 3,890 feet (Roswell is 3,570, a long way east and down).
	constexpr double DatumM = 1185;
	// The township's northwest corner, road-local: six miles a side, sections a mile, numbered as the Public Land Survey has them.
	constexpr double TownshipX = -3000, TownshipZ = -2700;
	// The grid's ticks: a kilometre apart, eastings and northings as a sheet's margin prints them (a made-up zone, not a real one).
	constexpr double EastingOrigin = 561000, NorthingOrigin = 3719000;
	constexpr double DeclinationDeg = 11.5;
	constexpr double ContourFeet = 20, ReliefCell = 25;

	FLinearColor Hue(uint32 Hex, float A = 1.f)
	{
		FLinearColor Out = FLinearColor::FromSRGBColor(FColor(uint8((Hex >> 16) & 255), uint8((Hex >> 8) & 255), uint8(Hex & 255)));
		Out.A = A;
		return Out;
	}
	FLinearColor Paper() { return Hue(0xf4efe1); }
	FLinearColor Ink() { return Hue(0x1d1c1a); }
	FLinearColor Brown(float A) { return Hue(0x9c5f2a, A); }
	FLinearColor Survey(float A) { return Hue(0xc23b2c, A); }   // section lines, the highway's fill
	FLinearColor Water(float A) { return Hue(0x2a68a6, A); }
	FLinearColor Graphite(float A) { return Hue(0x3a3a40, A); }
	FLinearColor YouRed(float A) { return Hue(0xc8231e, A); }
	FLinearColor Revised(float A = 1.f) { return Hue(0x8a3f9c, A); } // a photorevision's purple: what is newer than the survey

	FVector2f F2(const FVector2D& V) { return FVector2f(float(V.X), float(V.Y)); }
	FVector2D Project(const FVector2D& Middle, double Scale, double X, double Z) { return FVector2D(FrameMidX + (X - Middle.X) * Scale, FrameMidY + (Z - Middle.Y) * Scale); }
	FVector2D Unproject(const FVector2D& Middle, double Scale, const FVector2D& Page) { return FVector2D(Middle.X + (Page.X - FrameMidX) / Scale, Middle.Y + (Page.Y - FrameMidY) / Scale); }
	bool InFrame(const FVector2D& Pt, double Inset = 0) { return Pt.X >= FrameL + Inset && Pt.X <= FrameR - Inset && Pt.Y >= FrameT + Inset && Pt.Y <= FrameB - Inset; }
	// Sections in a township: 6 to 1 across the top row, 7 to 12 back across the next, and so on to 36.
	int32 SectionNumber(int32 Row, int32 Col) { return Row % 2 == 0 ? Row * 6 + (6 - Col) : Row * 6 + Col + 1; }

	// Where the sheet was left: the next one opens there, on the player when they have moved.
	struct FViewMemory { FVector2D Middle = FVector2D(33, -300); double Scale = ScaleFirst; FVector2D LastMe = FVector2D(1e9, 1e9); };
	FViewMemory& Memory() { static FViewMemory Kept; return Kept; }

	// ---------- the contours ----------
	struct FContour { TArray<FVector2f> P; FBox2f Box; double Ft = 0; bool bIndex = false; };
	struct FContourLabel { FVector2f At; float Deg = 0; int32 Ft = 0; };
	struct FHeightSpot { FVector2f At; int32 Ft = 0; };
	// Shade: the light from the northwest on each cell of the grid (-1 dark side .. 1 lit), a quadrangle's shaded relief. Streams: where
	// water would gather off the land's own slopes (flow from every cell to its lowest neighbour, enough of it together): dry washes.
	struct FReliefSheet
	{
		bool bBuilt = false, bWithHills = false; TArray<FContour> Lines; TArray<FContourLabel> Labels; TArray<FHeightSpot> Spots; float Low = 0, High = 0; double Millis = 0;
		TArray<float> Shade; int32 GridW = 0, GridH = 0; double GridX = 0, GridZ = 0, GridCell = 25;
		TArray<FContour> Streams;
		// Where the land's names go, found in the heights themselves: the highest block of land each side of the highway (and
		// which way it runs), and the largest closed basin, where water stands after a storm: the playa.
		bool bHighWest = false, bHighEast = false; FVector2f HighWest, HighEast; float HighWestDeg = 0, HighEastDeg = 0;
		FVector2f Playa; float PlayaR = 0;
	};
	FReliefSheet& ReliefSheet() { static FReliefSheet Kept; return Kept; }

	double FeetOf(double Height) { return (DatumM + Height) / FootM; }

	// Marching squares over a grid of heights, every ContourFeet, chained into lines, smoothed once. Index lines (every 100 ft) get
	// their height along them every so far, and the high points a spot height.
	void BuildRelief(FReliefSheet& Sheet, const TFunction<double(double, double)>& HeightAt, double MinX, double MinZ, double MaxX, double MaxZ, double Cell)
	{
		const double Began = FPlatformTime::Seconds();
		Sheet.Lines.Reset(); Sheet.Labels.Reset(); Sheet.Spots.Reset();
		const int32 NX = FMath::Max(2, int32((MaxX - MinX) / Cell) + 1), NZ = FMath::Max(2, int32((MaxZ - MinZ) / Cell) + 1);
		TArray<float> Hg; Hg.SetNumUninitialized(NX * NZ);
		float Lo = TNumericLimits<float>::Max(), Hi = TNumericLimits<float>::Lowest();
		for (int32 J = 0; J < NZ; ++J) for (int32 I = 0; I < NX; ++I)
		{
			const double V = HeightAt(MinX + I * Cell, MinZ + J * Cell);
			const float Vf = FMath::IsFinite(V) ? float(V) : 0.f;
			Hg[J * NX + I] = Vf; Lo = FMath::Min(Lo, Vf); Hi = FMath::Max(Hi, Vf);
		}
		Sheet.Low = Lo; Sheet.High = Hi;
		const auto Hat = [&Hg, NX](int32 I, int32 J) { return Hg[J * NX + I]; };
		const auto Corner = [MinX, MinZ, Cell](int32 I, int32 J) { return FVector2f(float(MinX + I * Cell), float(MinZ + J * Cell)); };
		const int32 Across = (NX - 1) * NZ; // horizontal edges first, then the vertical ones
		for (int32 Lk = FMath::CeilToInt(FeetOf(Lo) / ContourFeet); Lk * ContourFeet <= FeetOf(Hi); ++Lk)
		{
			const double Ft = Lk * ContourFeet;
			const float Lv = float(Ft * FootM - DatumM);
			TMap<int32, FVector2f> EdgeAt;
			const auto CrossAt = [&](int32 E) -> FVector2f
			{
				if (const FVector2f* Got = EdgeAt.Find(E)) return *Got;
				FVector2f Pt;
				if (E < Across)
				{
					const int32 J = E / (NX - 1), I = E % (NX - 1);
					const float A = Hat(I, J) - Lv, B = Hat(I + 1, J) - Lv;
					Pt = FMath::Lerp(Corner(I, J), Corner(I + 1, J), FMath::Clamp(A / (A - B), 0.f, 1.f));
				}
				else
				{
					const int32 E2 = E - Across, J = E2 / NX, I = E2 % NX;
					const float A = Hat(I, J) - Lv, B = Hat(I, J + 1) - Lv;
					Pt = FMath::Lerp(Corner(I, J), Corner(I, J + 1), FMath::Clamp(A / (A - B), 0.f, 1.f));
				}
				EdgeAt.Add(E, Pt);
				return Pt;
			};
			TArray<int32> SegA, SegB;
			for (int32 J = 0; J + 1 < NZ; ++J) for (int32 I = 0; I + 1 < NX; ++I)
			{
				const float A = Hat(I, J) - Lv, B = Hat(I + 1, J) - Lv, Cv = Hat(I + 1, J + 1) - Lv, D = Hat(I, J + 1) - Lv;
				const int32 Case = (A >= 0 ? 1 : 0) | (B >= 0 ? 2 : 0) | (Cv >= 0 ? 4 : 0) | (D >= 0 ? 8 : 0);
				if (Case == 0 || Case == 15) continue;
				const int32 ETop = J * (NX - 1) + I, EBottom = (J + 1) * (NX - 1) + I, ELeft = Across + J * NX + I, ERight = Across + J * NX + I + 1;
				const auto Seg = [&SegA, &SegB](int32 E1, int32 E2) { SegA.Add(E1); SegB.Add(E2); };
				const bool bMiddleUp = (A + B + Cv + D) * .25f >= 0;
				switch (Case)
				{
				case 1: case 14: Seg(ELeft, ETop); break;
				case 2: case 13: Seg(ETop, ERight); break;
				case 3: case 12: Seg(ELeft, ERight); break;
				case 4: case 11: Seg(ERight, EBottom); break;
				case 6: case 9: Seg(ETop, EBottom); break;
				case 7: case 8: Seg(ELeft, EBottom); break;
				case 5: if (bMiddleUp) { Seg(ETop, ERight); Seg(EBottom, ELeft); } else { Seg(ELeft, ETop); Seg(ERight, EBottom); } break;
				case 10: if (bMiddleUp) { Seg(ELeft, ETop); Seg(ERight, EBottom); } else { Seg(ETop, ERight); Seg(EBottom, ELeft); } break;
				default: break;
				}
			}
			// Chain the segments through the edges they share (an edge is shared by two cells at most).
			TMap<int32, TArray<int32, TInlineAllocator<2>>> Touch;
			for (int32 Si = 0; Si < SegA.Num(); ++Si) { Touch.FindOrAdd(SegA[Si]).Add(Si); Touch.FindOrAdd(SegB[Si]).Add(Si); }
			TBitArray<> Used(false, SegA.Num());
			for (int32 S0 = 0; S0 < SegA.Num(); ++S0)
			{
				if (Used[S0]) continue;
				Used[S0] = true;
				TArray<int32> Edges = { SegA[S0], SegB[S0] };
				const auto Extend = [&]()
				{
					for (;;)
					{
						const int32 Tail = Edges.Last();
						int32 Next = INDEX_NONE;
						if (const auto* Sharing = Touch.Find(Tail)) for (const int32 Cand : *Sharing) if (!Used[Cand]) { Next = Cand; break; }
						if (Next == INDEX_NONE) break;
						Used[Next] = true;
						Edges.Add(SegA[Next] == Tail ? SegB[Next] : SegA[Next]);
					}
				};
				Extend();
				Algo::Reverse(Edges);
				Extend();
				TArray<FVector2f> Raw;
				Raw.Reserve(Edges.Num());
				for (const int32 E : Edges)
				{
					const FVector2f Pt = CrossAt(E);
					if (Raw.IsEmpty() || FVector2f::DistSquared(Raw.Last(), Pt) > .25f) Raw.Add(Pt);
				}
				if (Raw.Num() < 2) continue;
				// One pass of corner cutting: the grid's steps read as a drawn line. The ends stay where they are.
				FContour L; L.Ft = Ft; L.bIndex = FMath::RoundToInt(Ft) % 100 == 0;
				L.P.Reserve(Raw.Num() * 2);
				L.P.Add(Raw[0]);
				for (int32 Q = 0; Q + 1 < Raw.Num(); ++Q) { L.P.Add(FMath::Lerp(Raw[Q], Raw[Q + 1], .25f)); L.P.Add(FMath::Lerp(Raw[Q], Raw[Q + 1], .75f)); }
				L.P.Add(Raw.Last());
				L.Box = FBox2f(L.P);
				if (L.bIndex)
				{
					// Its height every so far along it, the right way up.
					double Along = 0, NextAt = 450;
					for (int32 Q = 1; Q < L.P.Num(); ++Q)
					{
						const FVector2f D = L.P[Q] - L.P[Q - 1];
						Along += D.Size();
						if (Along < NextAt) continue;
						float Deg = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
						if (Deg > 90.f) Deg -= 180.f; else if (Deg < -90.f) Deg += 180.f;
						Sheet.Labels.Add({ L.P[Q], Deg, FMath::RoundToInt(Ft) });
						NextAt = Along + 1500;
					}
				}
				Sheet.Lines.Add(MoveTemp(L));
			}
		}
		// Spot heights: the highest points of their own neighbourhood, a good way apart.
		TArray<FHeightSpot> Tops;
		const int32 Around = 6;
		for (int32 J = Around; J + Around < NZ; J += 2) for (int32 I = Around; I + Around < NX; I += 2)
		{
			const float V = Hat(I, J);
			bool bTop = V > Lo + 6.f;
			for (int32 Dj = -Around; Dj <= Around && bTop; ++Dj) for (int32 Di = -Around; Di <= Around && bTop; ++Di) if ((Di || Dj) && Hat(I + Di, J + Dj) > V) bTop = false;
			if (bTop) Tops.Add({ Corner(I, J), FMath::RoundToInt(FeetOf(V)) });
		}
		Tops.Sort([](const FHeightSpot& A, const FHeightSpot& B) { return A.Ft > B.Ft; });
		for (const FHeightSpot& Peak : Tops)
		{
			if (Sheet.Spots.Num() >= 18) break;
			if (!Sheet.Spots.ContainsByPredicate([&Peak](const FHeightSpot& Have) { return FVector2f::Distance(Have.At, Peak.At) < 700.f; })) Sheet.Spots.Add(Peak);
		}
		// The shaded relief: light from the northwest, 45 degrees up.
		Sheet.GridW = NX; Sheet.GridH = NZ; Sheet.GridX = MinX; Sheet.GridZ = MinZ; Sheet.GridCell = Cell;
		Sheet.Shade.SetNumZeroed(NX * NZ);
		const FVector Sun = FVector(-1, 1.414, -1).GetSafeNormal(); // x east, y up, z south: from the northwest, up
		for (int32 J = 1; J + 1 < NZ; ++J) for (int32 I = 1; I + 1 < NX; ++I)
		{
			const double Dx = (Hat(I + 1, J) - Hat(I - 1, J)) / (2 * Cell), Dz = (Hat(I, J + 1) - Hat(I, J - 1)) / (2 * Cell);
			const FVector Up = FVector(-Dx, 1, -Dz).GetSafeNormal();
			Sheet.Shade[J * NX + I] = float(FMath::Clamp((FVector::DotProduct(Up, Sun) - FVector::DotProduct(FVector(0, 1, 0), Sun)) * 3.0, -1.0, 1.0));
		}
		// The washes: each cell drains to its lowest neighbour; where enough of the land drains through a cell, a stream runs.
		{
			TArray<int32> Down; Down.Init(INDEX_NONE, NX * NZ);
			for (int32 J = 1; J + 1 < NZ; ++J) for (int32 I = 1; I + 1 < NX; ++I)
			{
				float Low = Hat(I, J) - .05f; int32 Best = INDEX_NONE;
				for (int32 Dj = -1; Dj <= 1; ++Dj) for (int32 Di = -1; Di <= 1; ++Di)
				{
					if (!Di && !Dj) continue;
					const float V = Hat(I + Di, J + Dj) + ((Di && Dj) ? .02f : 0.f);
					if (V < Low) { Low = V; Best = (J + Dj) * NX + I + Di; }
				}
				Down[J * NX + I] = Best;
			}
			TArray<int32> Order; Order.Reserve(NX * NZ);
			for (int32 K = 0; K < NX * NZ; ++K) Order.Add(K);
			Order.Sort([&Hg](int32 A, int32 B) { return Hg[A] > Hg[B]; });
			TArray<int32> Flow; Flow.Init(1, NX * NZ);
			for (const int32 K : Order) if (Down[K] != INDEX_NONE) Flow[Down[K]] += Flow[K];
			const int32 Enough = 260; // cells: about 16 hectares draining through it
			TArray<int32> Ups; Ups.Init(0, NX * NZ);
			for (int32 K = 0; K < NX * NZ; ++K) if (Flow[K] >= Enough && Down[K] != INDEX_NONE && Flow[Down[K]] >= Enough) ++Ups[Down[K]];
			// The playa: the sink (no lower neighbour, off the grid's edge) the most land drains into, and its flat floor.
			int32 Sink = INDEX_NONE;
			for (int32 K = 0; K < NX * NZ; ++K)
			{
				const int32 Si = K % NX, Sj = K / NX;
				if (Down[K] != INDEX_NONE || Si < 2 || Sj < 2 || Si > NX - 3 || Sj > NZ - 3) continue;
				if (Sink == INDEX_NONE || Flow[K] > Flow[Sink]) Sink = K;
			}
			if (Sink != INDEX_NONE && Flow[Sink] >= 400)
			{
				const int32 Si = Sink % NX, Sj = Sink / NX;
				int32 Floor = 0; FVector2f Sum(0, 0);
				for (int32 Dj = -12; Dj <= 12; ++Dj) for (int32 Di = -12; Di <= 12; ++Di)
				{
					const int32 Fi = Si + Di, Fj = Sj + Dj;
					if (Fi < 0 || Fj < 0 || Fi >= NX || Fj >= NZ || Hat(Fi, Fj) > Hat(Si, Sj) + 1.f) continue;
					++Floor; Sum += Corner(Fi, Fj);
				}
				Sheet.Playa = Sum / float(FMath::Max(1, Floor));
				Sheet.PlayaR = float(FMath::Clamp(FMath::Sqrt(Floor / PI) * Cell, 30.0, 220.0));
			}
			TBitArray<> Drawn(false, NX * NZ);
			for (int32 K = 0; K < NX * NZ; ++K)
			{
				if (Flow[K] < Enough || Ups[K] > 0) continue; // only from a head
				FContour Wash;
				int32 At = K, Guard = 0;
				while (At != INDEX_NONE && Flow[At] >= Enough && ++Guard < 4000)
				{
					Wash.P.Add(Corner(At % NX, At / NX));
					if (Drawn[At]) break; // joined one already drawn
					Drawn[At] = true;
					At = Down[At];
				}
				if (Wash.P.Num() >= 4) { Wash.Box = FBox2f(Wash.P); Sheet.Streams.Add(MoveTemp(Wash)); }
			}
		}
		// The high ground each side of the highway: the 250 m block highest on average, if it stands well above that side's land,
		// and the way the high cells around it run (their long axis), for the name to follow.
		{
			const int32 Bin = FMath::Max(1, int32(250 / Cell));
			for (int32 Side = 0; Side < 2; ++Side)
			{
				double BestMean = -1e9, SideSum = 0; int32 SideN = 0, BestI = -1, BestJ = -1;
				for (int32 Bj = 0; Bj + Bin <= NZ; Bj += Bin) for (int32 Bi = 0; Bi + Bin <= NX; Bi += Bin)
				{
					const double Cx = MinX + (Bi + Bin * .5) * Cell;
					if (Side == 0 ? Cx > -400 : Cx < 400) continue;
					double Mean = 0;
					for (int32 J = Bj; J < Bj + Bin; ++J) for (int32 I = Bi; I < Bi + Bin; ++I) Mean += Hat(I, J);
					Mean /= Bin * Bin; SideSum += Mean; ++SideN;
					if (Mean > BestMean) { BestMean = Mean; BestI = Bi; BestJ = Bj; }
				}
				if (BestI < 0 || SideN < 4 || BestMean - SideSum / SideN < 12) continue;
				// The long axis of the high cells within a kilometre.
				const double Ax = MinX + (BestI + Bin * .5) * Cell, Az = MinZ + (BestJ + Bin * .5) * Cell;
				double Sx = 0, Sz = 0, Sxx = 0, Szz = 0, Sxz = 0; int32 Hn = 0;
				const int32 Reach1 = int32(1000 / Cell);
				for (int32 J = FMath::Max(0, BestJ - Reach1); J < FMath::Min(NZ, BestJ + Reach1); ++J) for (int32 I = FMath::Max(0, BestI - Reach1); I < FMath::Min(NX, BestI + Reach1); ++I)
				{
					if (Hat(I, J) < BestMean - 8) continue;
					const double Hx = MinX + I * Cell - Ax, Hz = MinZ + J * Cell - Az;
					Sx += Hx; Sz += Hz; Sxx += Hx * Hx; Szz += Hz * Hz; Sxz += Hx * Hz; ++Hn;
				}
				float Deg = 0;
				FVector2f Mid2{float(Ax), float(Az)};
				if (Hn > 8)
				{
					const double Mx = Sx / Hn, Mz = Sz / Hn, Cxx = Sxx / Hn - Mx * Mx, Czz = Szz / Hn - Mz * Mz, Cxz = Sxz / Hn - Mx * Mz;
					Deg = float(FMath::RadiansToDegrees(.5 * FMath::Atan2(2 * Cxz, Cxx - Czz)));
					Mid2 += FVector2f(float(Mx), float(Mz));
				}
				if (Deg > 90) Deg -= 180; else if (Deg <= -90) Deg += 180;
				(Side == 0 ? Sheet.HighWest : Sheet.HighEast) = Mid2;
				(Side == 0 ? Sheet.HighWestDeg : Sheet.HighEastDeg) = Deg;
				(Side == 0 ? Sheet.bHighWest : Sheet.bHighEast) = true;
			}
		}
		Sheet.Millis = (FPlatformTime::Seconds() - Began) * 1000.0;
		Sheet.bBuilt = true;
	}

	const FReliefSheet& Relief()
	{
		FReliefSheet& Sheet = ReliefSheet();
		const bool bHills = S47Relief::Active();
		if (!Sheet.bBuilt || Sheet.bWithHills != bHills)
		{
			Sheet.bWithHills = bHills;
			BuildRelief(Sheet, [](double X, double Z) { return S47FieldMap::LandHeight(X, Z); }, S47OpenLand::RimX0, S47OpenLand::RimZ0, S47OpenLand::RimX1, S47OpenLand::RimZ1, ReliefCell);
			UE_LOG(LogS47FieldMap, Display, TEXT("S47: survey map contours: %d lines, %d labels, %d spot heights, %d washes, playa %.0f m, high ground %s%s, %.0f to %.0f ft, %.0f ms (relief %s)"),
				Sheet.Lines.Num(), Sheet.Labels.Num(), Sheet.Spots.Num(), Sheet.Streams.Num(), Sheet.PlayaR, Sheet.bHighWest ? TEXT("west ") : TEXT(""), Sheet.bHighEast ? TEXT("east") : TEXT(""), FeetOf(Sheet.Low), FeetOf(Sheet.High), Sheet.Millis, bHills ? TEXT("on") : TEXT("off"));
		}
		return Sheet;
	}

	// ---------- drawing ----------
	struct FPen
	{
		FSlateWindowElementList& Out;
		const FGeometry& Page;
		void Lines(const TArray<FVector2f>& Pts, const FLinearColor& Col, float Thick, int32 Layer) const
		{
			if (Pts.Num() >= 2) FSlateDrawElement::MakeLines(Out, Layer, Page.ToPaintGeometry(), Pts, ESlateDrawEffect::None, Col, true, Thick);
		}
		// Dashes along a line, the pattern on, off, on... Only the stretch inside the frame (and a little round it) is drawn: the
		// rest only moves the pattern on, so a fence many miles long costs nothing where it is off the sheet.
		// bFramed: kept inside the map's frame (the legend's own samples are not).
		void Dashed(const TArray<FVector2f>& Pts, const FLinearColor& Col, float Thick, int32 Layer, const TArray<float>& Pattern, bool bFramed = true) const
		{
			if (Pts.Num() < 2 || Pattern.Num() < 2) { Lines(Pts, Col, Thick, Layer); return; }
			float Cycle = 0;
			for (const float Piece : Pattern) Cycle += Piece;
			if (Cycle <= 0) { Lines(Pts, Col, Thick, Layer); return; }
			float Phase = 0;
			for (int32 I = 1; I < Pts.Num(); ++I)
			{
				const FVector2f A = Pts[I - 1], B = Pts[I];
				const float Seg = FVector2f::Distance(A, B);
				if (Seg <= 0) continue;
				const FVector2f Dir = (B - A) / Seg;
				// Liang-Barsky against the frame grown by 10: [Tin, Tout] along this stretch is what shows.
				float Tin = 0, Tout = Seg;
				const float Pd[4] = { -Dir.X, Dir.X, -Dir.Y, Dir.Y };
				const float Grow = bFramed ? 10.f : 4000.f;
				const float Qd[4] = { A.X - (float(FrameL) - Grow), (float(FrameR) + Grow) - A.X, A.Y - (float(FrameT) - Grow), (float(FrameB) + Grow) - A.Y };
				bool bShows = true;
				for (int32 K = 0; K < 4 && bShows; ++K)
				{
					if (FMath::IsNearlyZero(Pd[K])) { bShows = Qd[K] >= 0; continue; }
					const float Cut = Qd[K] / Pd[K];
					if (Pd[K] < 0) Tin = FMath::Max(Tin, Cut); else Tout = FMath::Min(Tout, Cut);
					bShows = Tin < Tout;
				}
				if (bShows)
				{
					float Pos = Tin, Ph = FMath::Fmod(Phase + Tin, Cycle);
					int32 Guard = 0;
					while (Pos < Tout && ++Guard < 4000)
					{
						// Which piece of the pattern this is, and how much of it is left.
						float Acc = 0; int32 K = 0;
						while (K < Pattern.Num() - 1 && Acc + Pattern[K] <= Ph) { Acc += Pattern[K]; ++K; }
						const float Step = FMath::Max(.01f, FMath::Min(Acc + Pattern[K] - Ph, Tout - Pos));
						if (K % 2 == 0) FSlateDrawElement::MakeLines(Out, Layer, Page.ToPaintGeometry(), TArray<FVector2f>{ A + Dir * Pos, A + Dir * (Pos + Step) }, ESlateDrawEffect::None, Col, true, Thick);
						Pos += Step; Ph = FMath::Fmod(Ph + Step, Cycle);
					}
				}
				Phase = FMath::Fmod(Phase + Seg, Cycle);
			}
		}
		void Fill(const FVector2f& Min, const FVector2f& Size, const FLinearColor& Col, int32 Layer) const
		{
			FSlateDrawElement::MakeBox(Out, Layer, Page.ToPaintGeometry(Size, FSlateLayoutTransform(Min)), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, Col);
		}
		TArray<FVector2f> Circle(const FVector2f& Centre, float Radius, int32 Sides = 24) const
		{
			TArray<FVector2f> Pts;
			for (int32 I = 0; I <= Sides; ++I) { const float A = 2.f * PI * I / Sides; Pts.Add(Centre + FVector2f(FMath::Cos(A), FMath::Sin(A)) * Radius); }
			return Pts;
		}
		// A filled triangle as a fan of strokes from its tip across its base.
		void Triangle(const FVector2f& Tip, const FVector2f& BaseA, const FVector2f& BaseB, const FLinearColor& Col, int32 Layer) const
		{
			const int32 Strokes = FMath::Clamp(int32(FVector2f::Distance(BaseA, BaseB) / 1.2f), 4, 40);
			for (int32 I = 0; I <= Strokes; ++I) Lines({ Tip, FMath::Lerp(BaseA, BaseB, float(I) / Strokes) }, Col, 1.8f, Layer);
			Lines({ Tip, BaseA, BaseB, Tip }, Col, 1.4f, Layer);
		}
		static FVector2f Measure(const FString& Words, const FSlateFontInfo& Font)
		{
			if (!FSlateApplication::IsInitialized()) return FVector2f(float(Words.Len()) * Font.Size * .55f, Font.Size * 1.2f);
			return FVector2f(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Words, Font));
		}
		// Text with its anchor at (AlignX, AlignY) of its box (0 left or top, .5 middle, 1 right or bottom), turned Deg about its
		// middle, on a paper halo when asked: what keeps a name readable over the contours.
		FVector2f Text(const FString& Words, const FVector2f& At, const FSlateFontInfo& Font, const FLinearColor& Col, int32 Layer, float AlignX = 0, float AlignY = .5f, bool bHalo = false, float Deg = 0) const
		{
			const FVector2f Size = Measure(Words, Font);
			const FVector2f TopLeft = At - FVector2f(Size.X * AlignX, Size.Y * AlignY);
			const FSlateRenderTransform Turn(FQuat2f(FMath::DegreesToRadians(Deg)));
			if (bHalo)
			{
				const FVector2f HaloSize = Size + FVector2f(6, 0);
				const FLinearColor Under = Paper();
				FSlateDrawElement::MakeBox(Out, Layer, Page.ToPaintGeometry(HaloSize, FSlateLayoutTransform(TopLeft - FVector2f(3, 0)), Turn, FVector2f(.5f, .5f)), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, FLinearColor(Under.R, Under.G, Under.B, .82f));
			}
			FSlateDrawElement::MakeText(Out, Layer + 1, Page.ToPaintGeometry(Size, FSlateLayoutTransform(TopLeft), Turn, FVector2f(.5f, .5f)), Words, Font, ESlateDrawEffect::None, Col);
			return Size;
		}
	};

	FSlateFontInfo Printed(float Px) { return S47Style::Display(Px); }
	FSlateFontInfo Small(float Px) { return FCoreStyle::GetDefaultFontStyle("Regular", int32(Px)); }
	FSlateFontInfo Italic(float Px) { return FCoreStyle::GetDefaultFontStyle("Italic", int32(Px)); }
	FSlateFontInfo Pencil(float Px) { return S47Style::Hand(Px); }

	// Where a line from the middle of the frame towards P leaves it (inset): the edge arrow for one off the sheet.
	FVector2D EdgeToward(const FVector2D& Page, double Inset)
	{
		const FVector2D Middle(FrameMidX, FrameMidY), D = Page - Middle;
		double Cut = 1;
		if (D.X > 0) Cut = FMath::Min(Cut, (FrameR - Inset - Middle.X) / D.X);
		if (D.X < 0) Cut = FMath::Min(Cut, (FrameL + Inset - Middle.X) / D.X);
		if (D.Y > 0) Cut = FMath::Min(Cut, (FrameB - Inset - Middle.Y) / D.Y);
		if (D.Y < 0) Cut = FMath::Min(Cut, (FrameT + Inset - Middle.Y) / D.Y);
		return Middle + D * FMath::Max(0., Cut);
	}
	// The part of a page segment inside the frame: false when none of it is.
	bool ClipToFrame(FVector2D& A, FVector2D& B)
	{
		double In = 0, Out = 1;
		const FVector2D D = B - A;
		const double Pd[4] = { -D.X, D.X, -D.Y, D.Y }, Qd[4] = { A.X - FrameL, FrameR - A.X, A.Y - FrameT, FrameB - A.Y };
		for (int32 I = 0; I < 4; ++I)
		{
			if (FMath::IsNearlyZero(Pd[I])) { if (Qd[I] < 0) return false; continue; }
			const double Cut = Qd[I] / Pd[I];
			if (Pd[I] < 0) In = FMath::Max(In, Cut); else Out = FMath::Min(Out, Cut);
			if (In > Out) return false;
		}
		const FVector2D A0 = A;
		A = A0 + D * In; B = A0 + D * Out;
		return true;
	}
	FString YardsOrMiles(double Metres)
	{
		const double Yards = Metres / .9144;
		if (Yards < 1500) return FString::Printf(TEXT("%d yards"), FMath::Max(10, FMath::RoundToInt(Yards / 10) * 10));
		return FString::Printf(TEXT("%.1f miles"), Metres / MileM);
	}
}

// ---------- where things are on the land ----------

bool S47FieldMap::OnSheet(const FString& Area, double X, double Z, double Yaw, double& OutX, double& OutZ, double& OutHeading)
{
	FString Leg = Area;
	double Ax = X, Az = Z;
	if (Area == TEXT("room6"))
	{
		// Room 6's inside is built far off on its own: on the sheet it is the motel's row of rooms.
		const S47Geo::FRect& Court = S47Geo::MOTEL_RECT();
		Leg = TEXT("saro"); Ax = (Court.MinX + Court.MaxX) * .5; Az = (Court.MinZ + Court.MaxZ) * .5;
	}
	double Rx = 0, Rz = 0;
	if (!S47OpenLand::RoadOf(Leg, Ax, Az, Rx, Rz)) return false;
	// The way the yaw points (-sin, -cos in the area's x and z), through the same terms, a metre on.
	double Fx = 0, Fz = 0;
	S47OpenLand::RoadOf(Leg, Ax - FMath::Sin(Yaw), Az - FMath::Cos(Yaw), Fx, Fz);
	OutX = Rx; OutZ = Rz;
	OutHeading = FMath::Fmod(FMath::RadiansToDegrees(FMath::Atan2(Fx - Rx, -(Fz - Rz))) + 720.0, 360.0);
	return true;
}

bool S47FieldMap::Me(const US47Main* M, double& OutX, double& OutZ, double& OutHeading, FString& OutArea, bool& bOutDriving)
{
	bOutDriving = false;
	if (!M || !M->Player || !M->Web) return false;
	OutArea = M->Web->Area();
	// At the wheel: the cab, in the area the drive is in.
	if (M->World && M->World->bDriving)
		if (const FS47Truck* Cab = M->World->CabTruck())
		{
			bOutDriving = true;
			return OnSheet(OutArea, Cab->Position.X, Cab->Position.Z, Cab->Rotation.Y, OutX, OutZ, OutHeading);
		}
	return OnSheet(OutArea, M->Player->Pos.X, M->Player->Pos.Z, M->Player->Yaw, OutX, OutZ, OutHeading);
}

bool S47FieldMap::Truck(const US47Main* M, double& OutX, double& OutZ, double& OutHeading)
{
	if (!M || !M->World || M->World->bDriving) return false;
	const S47J::FObj Kept = M->World->TruckSave();
	if (!Kept.IsValid()) return false;
	const S47J::FObj Pose = S47J::Obj(Kept, TEXT("pose"));
	if (!Pose.IsValid()) return false;
	return OnSheet(S47J::Str(Kept, TEXT("area")), S47J::Num(Pose, TEXT("x")), S47J::Num(Pose, TEXT("z")), S47J::Num(Pose, TEXT("h")), OutX, OutZ, OutHeading);
}

void S47FieldMap::GatherJobs(US47Main* M, FS47MapView& V)
{
	if (!M || !M->Freeform) return;
	FS47FreeformSession& Fs = *M->Freeform;
	const auto Write = [&V](const FString& Area, double X, double Z, const FString& Text)
	{
		double Rx = 0, Rz = 0, Hd = 0;
		if (!OnSheet(Area, X, Z, 0, Rx, Rz, Hd)) return;
		FS47MapJob Job; Job.X = Rx; Job.Z = Rz; Job.Text = Text;
		V.Jobs.Add(Job);
	};
	// A dish that is down, by its number.
	if (Fs.Field && Fs.Field->HasArrayFault() && M->Web)
		for (int32 I = 0; I < M->Web->DishCount(); ++I)
			if (M->Web->DishId(I) == Fs.Field->ServiceDishId()) { Write(TEXT("saro"), M->Web->DishX(I), M->Web->DishZ(I), Fs.Field->ServiceDishId() + TEXT(" drive fault")); break; }
	// The feeder, when the station's power is off.
	if (Fs.PowerFault() && Fs.EventPlaces)
		if (const FS47EventPlace* Feeder = Fs.EventPlaces->Find(TEXT("feeder"))) { FS47MapJob Job; Job.X = Feeder->EntryX; Job.Z = Feeder->EntryZ; Job.Text = TEXT("fix the feeder"); V.Jobs.Add(Job); }
	// The round's stops left today.
	if (Fs.Round && Fs.Field)
	{
		const FString Due = Fs.Round->TaskLine();
		if (!Due.IsEmpty())
			for (const FString& Stop : FS47DayRound::StopsFor(Fs.Seed(), Fs.Day()))
			{
				const FString Name = FS47DayRound::StopName(Stop);
				if (!Due.Contains(Name)) continue;
				const FVector At = Fs.Field->InteractionPoint(Stop);
				if (!At.IsZero()) Write(TEXT("saro"), At.X, At.Z, TEXT("round: ") + Name);
			}
	}
	// Observing work waiting at OPS, and Ward's paper in the tray.
	{
		int32 Waiting = 0;
		for (const auto& J : Fs.Jobs()) Waiting += J.Stage != FS47FreeformSession::EJobStage::Filed && !J.bDuplicateRejected && J.CaseTest < 0 && !Fs.EveningGated(J) ? 1 : 0;
		FString Ops;
		if (Waiting > 0) Ops = FString::Printf(TEXT("OPS: %d %s to do"), Waiting, Waiting == 1 ? TEXT("job") : TEXT("jobs"));
		if (Fs.Predecessor && !Fs.Predecessor->TaskLine().IsEmpty() && Fs.Predecessor->TaskLine().Contains(TEXT("tray"))) Ops += Ops.IsEmpty() ? FString(TEXT("OPS: letter in the tray")) : FString(TEXT(", letter in the tray"));
		if (!Ops.IsEmpty()) Write(TEXT("saro"), 0, 0, Ops);
	}
	// The mailbox's flag.
	if (Fs.Mail && Fs.Mail->MailIn())
	{
		FString Area; double X = 0, Z = 0;
		if (Fs.Mail->WorldSpot(Area, X, Z)) Write(Area, X, Z, TEXT("mail in the box"));
	}
	// The truck, when it is in trouble where it stands.
	if (Fs.Care && Fs.Care->NeedsRescue())
	{
		const FString What = Fs.Care->Stuck() ? TEXT("truck: dig it out") : Fs.Care->Flat() ? TEXT("truck: change the wheel") : TEXT("truck: water the radiator");
		if (V.bTruck) { FS47MapJob Job; Job.X = V.TruckX; Job.Z = V.TruckZ; Job.Text = What; V.Jobs.Add(Job); }
		else if (V.bDriving && V.bMe) { FS47MapJob Job; Job.X = V.MeX; Job.Z = V.MeZ; Job.Text = What; V.Jobs.Add(Job); }
	}
}

FString S47FieldMap::Compass(double Deg)
{
	static const TCHAR* const Points[] = { TEXT("north"), TEXT("north-northeast"), TEXT("northeast"), TEXT("east-northeast"), TEXT("east"), TEXT("east-southeast"), TEXT("southeast"), TEXT("south-southeast"),
		TEXT("south"), TEXT("south-southwest"), TEXT("southwest"), TEXT("west-southwest"), TEXT("west"), TEXT("west-northwest"), TEXT("northwest"), TEXT("north-northwest") };
	const double D = FMath::Fmod(FMath::Fmod(Deg, 360.0) + 360.0, 360.0);
	return Points[int32(FMath::RoundToDouble(D / 22.5)) % 16];
}

double S47FieldMap::LandHeight(double X, double Z)
{
	double Y = S47FarmWorld::FloorRoad(X, Z);
	// The range stands over the land where its table says; past the table's edge HillAbove says so with a very large number.
	const double Above = S47Relief::HillAbove(X, Z);
	if (FMath::IsFinite(Above) && Above > 0 && Above < 4000)
	{
		const double Lifted = S47Relief::HillLift(X, Z);
		if (Above > Lifted) Y += Above - Lifted;
	}
	return FMath::IsFinite(Y) ? Y : 0.0;
}

double S47FieldMap::Feet(double Height) { return S47FieldMapLocal::FeetOf(Height); }

void S47FieldMap::DevScale(double NewScale)
{
	auto& Kept = S47FieldMapLocal::Memory();
	Kept.Scale = FMath::Clamp(NewScale, S47FieldMapLocal::ScaleLeast, S47FieldMapLocal::ScaleMost);
	Kept.LastMe = FVector2D(1e9, 1e9);
}

double S47FieldMap::Scale() { return S47FieldMapLocal::Memory().Scale; }

TArray<FString> S47FieldMap::Description(const FS47MapView& V)
{
	TArray<FString> Said;
	Said.Add(TEXT("A survey map of the land around SARO, the Caliche quadrangle, with your own pencil on it. Brown contour lines every 20 feet, red section lines a mile apart."));
	const US47Main* M = US47Main::Get(nullptr);
	const FS47EventPlaces* Places = M && M->Freeform ? M->Freeform->EventPlaces.Get() : nullptr;
	if (V.bMe)
	{
		FString Where;
		if (const FS47EventPlace* Close = Places ? Places->NearestTo(FVector2D(V.MeX, V.MeZ), 2500) : nullptr)
		{
			const double Dm = FVector2D::Distance(FVector2D(V.MeX, V.MeZ), FVector2D(Close->X, Close->Z));
			const double Az = FS47Places::AzimuthOf(Close->X, Close->Z, V.MeX, V.MeZ);
			Where = Dm < 40 ? FString::Printf(TEXT("at %s"), *Close->Title) : FString::Printf(TEXT("%s %s of %s"), *S47FieldMapLocal::YardsOrMiles(Dm), *Compass(Az), *Close->Title);
		}
		else Where = FString::Printf(TEXT("%s from SARO"), *S47FieldMapLocal::YardsOrMiles(FVector2D::Distance(FVector2D(V.MeX, V.MeZ), FVector2D(33, -300))));
		Said.Add(FString::Printf(TEXT("The red arrow is you, %s, facing %s%s."), *Where, *Compass(V.MeHeading), V.bDriving ? TEXT(", at the wheel") : TEXT("")));
	}
	else Said.Add(TEXT("You are off this sheet."));
	if (V.bTruck && V.bMe)
	{
		const double D = FVector2D::Distance(FVector2D(V.MeX, V.MeZ), FVector2D(V.TruckX, V.TruckZ));
		Said.Add(D < 15 ? FString(TEXT("The truck is right here.")) : FString::Printf(TEXT("The truck is %s %s of you."), *S47FieldMapLocal::YardsOrMiles(D), *Compass(FS47Places::AzimuthOf(V.MeX, V.MeZ, V.TruckX, V.TruckZ))));
	}
	TArray<FString> Pins;
	for (const FS47MapPin& Pin : V.Pins) Pins.Add(Pin.bHeard ? Pin.Label + TEXT(" (heard of)") : Pin.Label);
	if (Pins.Num()) Said.Add(TEXT("Pencilled in: ") + FString::Join(Pins, TEXT(", ")) + TEXT("."));
	TArray<FString> Fixes;
	for (const FS47MapSector& Sec : V.Sectors) Fixes.Add(FString::Printf(TEXT("%s about %03.0f degrees"), *Sec.Label, Sec.Az));
	if (Fixes.Num()) Said.Add(TEXT("Bearings from SARO: ") + FString::Join(Fixes, TEXT(", ")) + TEXT("."));
	TArray<FString> Work;
	for (const FS47MapJob& Job : V.Jobs) Work.Add(Job.Text);
	if (Work.Num()) Said.Add(TEXT("Written in by the places: ") + FString::Join(Work, TEXT("; ")) + TEXT("."));
	if (V.Notes.Num()) Said.Add(FString::Printf(TEXT("Your own marks: %d."), V.Notes.Num()));
	return Said;
}

// ---------- the widget ----------

void SS47FieldMap::Construct(const FArguments& InArgs)
{
	View = InArgs._View;
	Width = InArgs._Width;
	OnNote = InArgs._OnNote;
	auto& Kept = S47FieldMapLocal::Memory();
	Sc = Kept.Scale;
	Mid = Kept.Middle;
	// Opened after the player has moved: on them. Turned to another tab and back: where it was left.
	if (View.bMe && FVector2D::Distance(Kept.LastMe, FVector2D(View.MeX, View.MeZ)) > 1.0) CentreOnMe();
	Clamp();
	SurveyWorld();
	// Out at night the sheet is read by a flashlight; indoors at SARO, under its lamps.
	if (const US47Main* Mn = US47Main::Get(nullptr); Mn && Mn->IsFreeform() && Mn->Freeform && Mn->Player && Mn->Web)
	{
		const double Hour = FMath::Fmod(Mn->Freeform->Clock(), 86400.0) / 3600.0;
		const bool bNight = Hour >= 20 || Hour < 5.5;
		const FVector EyeAt(Mn->Player->Pos.X, Mn->Player->FloorY + Mn->Player->Eye, Mn->Player->Pos.Z);
		const bool bUnderLamps = Mn->Web->Area() == TEXT("saro") && Mn->World && Mn->World->Indoors(EyeAt);
		NightDim = bNight && !bUnderLamps ? .35f : 0.f;
	}
}

FVector2D SS47FieldMap::ToPage(double X, double Z) const { return S47FieldMapLocal::Project(Mid, Sc, X, Z); }
FVector2D SS47FieldMap::ToRoad(const FVector2D& Page) const { return S47FieldMapLocal::Unproject(Mid, Sc, Page); }

void SS47FieldMap::Clamp()
{
	Sc = FMath::Clamp(Sc, S47FieldMapLocal::ScaleLeast, S47FieldMapLocal::ScaleMost);
	Mid.X = FMath::Clamp(Mid.X, S47OpenLand::RimX0, S47OpenLand::RimX1);
	Mid.Y = FMath::Clamp(Mid.Y, S47OpenLand::RimZ0, S47OpenLand::RimZ1);
	auto& Kept = S47FieldMapLocal::Memory();
	Kept.Middle = Mid; Kept.Scale = Sc;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SS47FieldMap::CentreOnMe()
{
	if (View.bMe)
	{
		Mid = FVector2D(View.MeX, View.MeZ);
		S47FieldMapLocal::Memory().LastMe = Mid;
	}
	else Mid = SaroAt;
	Clamp();
}

void SS47FieldMap::ZoomAbout(double Factor, const FVector2D& Page)
{
	const FVector2D Before = ToRoad(Page);
	Sc = FMath::Clamp(Sc * Factor, S47FieldMapLocal::ScaleLeast, S47FieldMapLocal::ScaleMost);
	Mid += Before - ToRoad(Page);
	Clamp();
}

void SS47FieldMap::Note(const FVector2D& Road)
{
	if (!OnNote) return;
	View.Notes = OnNote(Road.X, Road.Y, 14.0 / Sc);
	Invalidate(EInvalidateWidgetReason::Paint);
}

bool SS47FieldMap::Key(const FKey& K)
{
	const double Move = 90.0 / Sc;
	if (K == EKeys::Up || K == EKeys::W || K == EKeys::Gamepad_DPad_Up) { Mid.Y -= Move; Clamp(); return true; }
	if (K == EKeys::Down || K == EKeys::S || K == EKeys::Gamepad_DPad_Down) { Mid.Y += Move; Clamp(); return true; }
	if (K == EKeys::Left || K == EKeys::A || K == EKeys::Gamepad_DPad_Left) { Mid.X -= Move; Clamp(); return true; }
	if (K == EKeys::Right || K == EKeys::D || K == EKeys::Gamepad_DPad_Right) { Mid.X += Move; Clamp(); return true; }
	const FVector2D Middle(S47FieldMapLocal::FrameMidX, S47FieldMapLocal::FrameMidY);
	if (K == EKeys::Equals || K == EKeys::Add || K == EKeys::PageUp) { ZoomAbout(1.4, Middle); return true; }
	if (K == EKeys::Hyphen || K == EKeys::Subtract || K == EKeys::PageDown) { ZoomAbout(1.0 / 1.4, Middle); return true; }
	if (K == EKeys::Home || K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom || K == EKeys::Gamepad_LeftThumbstick) { CentreOnMe(); return true; }
	if (K == EKeys::X || K == EKeys::Insert || K == EKeys::Gamepad_RightThumbstick) { Note(Mid); return true; }
	return false;
}

void SS47FieldMap::Stick(const FKey& Axis, float Value)
{
	if (Axis == EKeys::Gamepad_LeftX) Pad.X = Value;
	else if (Axis == EKeys::Gamepad_LeftY) Pad.Y = Value;
}

void SS47FieldMap::Wheel(float Delta)
{
	ZoomAbout(FMath::Pow(1.25, double(Delta)), FVector2D(S47FieldMapLocal::FrameMidX, S47FieldMapLocal::FrameMidY));
}

void SS47FieldMap::Step(float Dt)
{
	const double L = Pad.Size();
	if (L <= .2) return;
	const FVector2D Way = Pad / L * FMath::Clamp((L - .2) / .8, 0.0, 1.0);
	// Up on the stick is north: up the page.
	Mid += FVector2D(Way.X, -Way.Y) * (520.0 / Sc) * FMath::Min(double(Dt), .05);
	Clamp();
}

FReply SS47FieldMap::OnMouseWheel(const FGeometry& G, const FPointerEvent& E)
{
	const FVector2D At = FVector2D(G.AbsoluteToLocal(E.GetScreenSpacePosition())) / PageScale(G);
	ZoomAbout(FMath::Pow(1.25, double(E.GetWheelDelta())), S47FieldMapLocal::InFrame(At) ? At : FVector2D(S47FieldMapLocal::FrameMidX, S47FieldMapLocal::FrameMidY));
	return FReply::Handled();
}

FReply SS47FieldMap::OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E)
{
	const FVector2D At = FVector2D(G.AbsoluteToLocal(E.GetScreenSpacePosition())) / PageScale(G);
	if (E.GetEffectingButton() == EKeys::RightMouseButton) { if (S47FieldMapLocal::InFrame(At)) Note(ToRoad(At)); return FReply::Handled(); }
	if (E.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
	bDrag = true; DragAt = At; DragMoved = 0;
	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SS47FieldMap::OnMouseMove(const FGeometry& G, const FPointerEvent& E)
{
	if (!bDrag || !HasMouseCapture()) return FReply::Unhandled();
	const FVector2D At = FVector2D(G.AbsoluteToLocal(E.GetScreenSpacePosition())) / PageScale(G);
	const FVector2D By = At - DragAt;
	DragAt = At; DragMoved += float(By.Size());
	Mid -= By / Sc;
	Clamp();
	return FReply::Handled();
}

FReply SS47FieldMap::OnMouseButtonUp(const FGeometry& G, const FPointerEvent& E)
{
	if (E.GetEffectingButton() != EKeys::LeftMouseButton || !bDrag) return FReply::Unhandled();
	bDrag = false;
	return FReply::Handled().ReleaseMouseCapture();
}

FCursorReply SS47FieldMap::OnCursorQuery(const FGeometry& G, const FPointerEvent& E) const
{
	return FCursorReply::Cursor(bDrag ? EMouseCursor::GrabHandClosed : EMouseCursor::GrabHand);
}

void SS47FieldMap::SurveyWorld() const
{
	Blocks.Reset(); Dishes.Reset(); DishIds.Reset(); Paths.Reset(); Features.Reset();
	double Sx = 0, Sz = 0;
	if (S47OpenLand::RoadOf(TEXT("saro"), 9, 0, Sx, Sz)) { SaroAt = FVector2D(Sx, Sz); SaroZ = Sz; }
	const auto Block = [](const FString& Area, double XMin, double XMax, double ZMin, double ZMax) -> FBox2D
	{
		double Ax = 0, Az = 0, Bx = 0, Bz = 0;
		if (!S47OpenLand::RoadOf(Area, XMin, ZMin, Ax, Az) || !S47OpenLand::RoadOf(Area, XMax, ZMax, Bx, Bz)) return FBox2D(ForceInit);
		return FBox2D(FVector2D(FMath::Min(Ax, Bx), FMath::Min(Az, Bz)), FVector2D(FMath::Max(Ax, Bx), FMath::Max(Az, Bz)));
	};
	// SARO's own building is walls, not one solid block: the main block and its south rooms, the lab, the east wing, as the
	// site's rooms and the ground round them have it (manifest walks, saro-local).
	static const double SaroWalls[][4] = { { -6.75, 6.55, -5.95, 5.95 }, { -6.75, 2.65, 5.95, 11.15 }, { 12.65, 19.25, -6.05, 1.05 }, { 20.15, 39.05, -6.05, 5.95 } };
	for (const auto& W4 : SaroWalls) { const FBox2D B = Block(TEXT("saro"), W4[0], W4[1], W4[2], W4[3]); if (B.bIsValid) Blocks.Add({ B }); }
	// Mesa Diner is walls too (the diner area's colliders, road-local).
	Blocks.Add({ FBox2D(FVector2D(-28.05, 1990.95), FVector2D(-18.05, 2008.95)) });
	// Caliche's buildings, road-local, as the web export's town stood them (its colliders) and the native town and CalicheAccess
	// rebuilt them in place (the sheriff's room, for one: root -16 2106, footprint -10..0 by -6..6). Once a building can be walked
	// into, what stops a walker is only its walls, so the town is not left to the colliders below.
	static const double TownBlocks[][4] = {
		{ -27, -17, 2066, 2078 }, { -27, -17.4, 2078, 2088 },   // the gas station
		{ -26, -16, 2100, 2112 }, { -25, -15, 2126, 2140 },     // the sheriff, the laundromat
		{ -29, -15, 2152, 2172 }, { -31, -15, 2190, 2214 },     // the Plaza, the feed store
		{ 20, 30, 2030, 2042 }, { 20.5, 29.5, 2058, 2070 },     // 1420 AM, the souvenir shop
		{ 21, 35, 2091, 2100 },                                 // the church
		{ 24, 33, 2128, 2138 }, { 24.5, 32, 2152, 2161 }, { 25, 34, 2180, 2190 }, { -28, -19, 2236, 2246 } }; // the four houses
	for (const auto& T4 : TownBlocks) Blocks.Add({ FBox2D(FVector2D(T4[0], T4[2]), FVector2D(T4[1], T4[3])) });
	// M4's temporary boxes (S47TownSites; the sheet is Freeform's): drawn as the buildings they stand for, from their one table.
	for (const FBox2D& Temp : S47TownSites::SolidFootprints()) Blocks.Add({ Temp });
	if (S47FarmBuildings::Available()) for (const FBox2D& Plot : S47FarmBuildings::Plots()) Blocks.Add({ Plot });
	US47Main* Mn = US47Main::Get(nullptr);
	AS47Web* Web = Mn ? Mn->Web.Get() : nullptr;
	if (Web)
	{
		for (int32 I = 0; I < Web->DishCount(); ++I)
		{
			double X = 0, Z = 0;
			if (S47OpenLand::RoadOf(TEXT("saro"), Web->DishX(I), Web->DishZ(I), X, Z)) { Dishes.Add(FVector2D(X, Z)); DishIds.Add(Web->DishId(I)); }
		}
		// What stops a walker, where it is the size of a building: the town, the motel, STATION 01's hut, a shed. Not a wall, a
		// fence, a car or a chair, and not a dish's footing (the dish is drawn as itself).
		for (const TCHAR* Area : { TEXT("saro"), TEXT("road"), TEXT("diner"), TEXT("station01") })
		{
			if (!Web->HasWalkArea(Area)) continue;
			for (const FS47Rect& Rc : Web->CollidersMutable(Area))
			{
				if (!Rc.bEnabled) continue;
				const double Wide = Rc.MaxX - Rc.MinX, Deep = Rc.MaxZ - Rc.MinZ;
				if (FMath::Min(Wide, Deep) < 2.4 || Wide * Deep < 14 || FMath::Max(Wide, Deep) > 70) continue;
				const FBox2D B = Block(Area, Rc.MinX, Rc.MaxX, Rc.MinZ, Rc.MaxZ);
				if (!B.bIsValid || !S47OpenLand::InRim(B.GetCenter().X, B.GetCenter().Y)) continue;
				if (Dishes.ContainsByPredicate([&B](const FVector2D& D) { return FVector2D::Distance(D, B.GetCenter()) < 5; })) continue;
				if (Blocks.ContainsByPredicate([&B](const FBlock& K) { return K.Box.Min.Equals(B.Min, .3) && K.Box.Max.Equals(B.Max, .3); })) continue;
				Blocks.Add({ B });
			}
		}
		// The array's walks: the spine, the rows and the spurs to each dish, down their middles.
		if (Web->HasWalkArea(TEXT("saro")))
			for (const FS47Rect& Rc : Web->ZonesMutable(TEXT("saro")))
			{
				if (!Rc.Id.StartsWith(TEXT("fSpine")) && !Rc.Id.StartsWith(TEXT("fRow")) && !Rc.Id.StartsWith(TEXT("fSpur"))) continue;
				const bool bLong = Rc.MaxX - Rc.MinX >= Rc.MaxZ - Rc.MinZ;
				const double Cx = (Rc.MinX + Rc.MaxX) * .5, Cz = (Rc.MinZ + Rc.MaxZ) * .5;
				double Ax = 0, Az = 0, Bx = 0, Bz = 0;
				if (bLong) { S47OpenLand::RoadOf(TEXT("saro"), Rc.MinX, Cz, Ax, Az); S47OpenLand::RoadOf(TEXT("saro"), Rc.MaxX, Cz, Bx, Bz); }
				else { S47OpenLand::RoadOf(TEXT("saro"), Cx, Rc.MinZ, Ax, Az); S47OpenLand::RoadOf(TEXT("saro"), Cx, Rc.MaxZ, Bx, Bz); }
				Paths.Add({ FVector2D(Ax, Az), FVector2D(Bx, Bz) });
			}
	}
	const FS47EventPlaces* Catalogue = Mn && Mn->Freeform ? Mn->Freeform->EventPlaces.Get() : nullptr;
	if (Catalogue) if (const FS47EventPlace* Church = Catalogue->Find(TEXT("church")))
		for (FBlock& B : Blocks) if (B.Box.ExpandBy(4).IsInside(FVector2D(Church->EntryX, Church->EntryZ))) { B.bChurch = true; break; }
	// What a survey party would have put on the sheet years ago: the windmill, the stock tank, the mine and the line shack. Not
	// what anyone has found there since.
	const FS47Places* Land = Mn && Mn->Freeform ? Mn->Freeform->Places.Get() : nullptr;
	for (const TCHAR* Id : { TEXT("windmill"), TEXT("tank"), TEXT("mine"), TEXT("shack"), TEXT("trough") })
	{
		double X = 0, Z = 0;
		if (Land && Land->Spot(Id, X, Z)) Features.Add({ FVector2D(X, Z), Id });
		else if (const FS47Places::FDef* D = FS47Places::Def(Id)) Features.Add({ FVector2D(D->X, D->Z), Id });
	}
	if (Catalogue) if (const FS47EventPlace* Feeder = Catalogue->Find(TEXT("feeder"))) Features.Add({ FVector2D(Feeder->EntryX, Feeder->EntryZ), TEXT("feeder") });
}

int32 SS47FieldMap::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	using namespace S47FieldMapLocal;
	const float Kp = float(G.GetLocalSize().X / 1200.0);
	const FGeometry Pg = G.MakeChild(FVector2f(1200.f, 800.f), FSlateLayoutTransform(Kp));
	const FPen Pen{ Out, Pg };
	// Layers: paper, relief and grid, water and roads, buildings, printed words, pencil, pencil words, you, the margin.
	const int32 LPaper = Layer, LRelief = Layer + 1, LRoad = Layer + 2, LBuilt = Layer + 4, LWords = Layer + 6, LPencil = Layer + 8, LPencilWords = Layer + 9, LYou = Layer + 11, LMargin = Layer + 13;
	Pen.Fill(FVector2f(0, 0), FVector2f(1200, 800), Paper(), LPaper);
	const auto OnPage = [this](double X, double Z) { return F2(ToPage(X, Z)); };
	const FVector2D ViewA = ToRoad(FVector2D(FrameL, FrameT)), ViewB = ToRoad(FVector2D(FrameR, FrameB));
	const FBox2D Shown(ViewA, ViewB);
	const auto Visible = [&Shown](double X, double Z, double Slack) { return X >= Shown.Min.X - Slack && X <= Shown.Max.X + Slack && Z >= Shown.Min.Y - Slack && Z <= Shown.Max.Y + Slack; };

	Out.PushClip(FSlateClippingZone(Pg.ToPaintGeometry(FVector2f(float(FrameR - FrameL), float(FrameB - FrameT)), FSlateLayoutTransform(FVector2f(float(FrameL), float(FrameT))))));
	// Off the land: nothing surveyed. A little darker past the rim's fence.
	{
		const FVector2f A = OnPage(S47OpenLand::RimX0, S47OpenLand::RimZ0), B = OnPage(S47OpenLand::RimX1, S47OpenLand::RimZ1);
		const FLinearColor Beyond = Hue(0xe6dfcd);
		const float L0 = float(FrameL), T0f = float(FrameT), Wf = float(FrameR - FrameL);
		Pen.Fill(FVector2f(L0, T0f), FVector2f(Wf, FMath::Max(0.f, A.Y - T0f)), Beyond, LPaper);
		Pen.Fill(FVector2f(L0, B.Y), FVector2f(Wf, FMath::Max(0.f, float(FrameB) - B.Y)), Beyond, LPaper);
		Pen.Fill(FVector2f(L0, A.Y), FVector2f(FMath::Max(0.f, A.X - L0), B.Y - A.Y), Beyond, LPaper);
		Pen.Fill(FVector2f(B.X, A.Y), FVector2f(FMath::Max(0.f, float(FrameR) - B.X), B.Y - A.Y), Beyond, LPaper);
	}
	// Caliche's built-up tint, as a quadrangle shades a town.
	{
		const auto& Town = S47RoadShape::TOWN();
		const FVector2f A = OnPage(Town.West, Town.Z0), B = OnPage(Town.East, Town.Z1);
		Pen.Fill(A, B - A, Hue(0xeab3a0, .38f), LPaper);
	}

	// ---- contours ----
	const FReliefSheet& Hills = Relief();
	// The shaded relief: blocks of the grid, at least 12 page units a side, only where the slope turns from or to the light.
	if (Hills.GridW > 2)
	{
		const int32 Coarse = FMath::Max(1, FMath::CeilToInt(7.0 / (Hills.GridCell * Sc)));
		const double Step = Hills.GridCell * Coarse;
		const int32 I0 = FMath::Max(0, int32((Shown.Min.X - Hills.GridX) / Hills.GridCell) - Coarse), I1 = FMath::Min(Hills.GridW - 1, int32((Shown.Max.X - Hills.GridX) / Hills.GridCell) + Coarse);
		const int32 J0 = FMath::Max(0, int32((Shown.Min.Y - Hills.GridZ) / Hills.GridCell) - Coarse), J1 = FMath::Min(Hills.GridH - 1, int32((Shown.Max.Y - Hills.GridZ) / Hills.GridCell) + Coarse);
		const float Side = float(Step * Sc);
		for (int32 J = J0 - J0 % Coarse; J <= J1; J += Coarse) for (int32 I = I0 - I0 % Coarse; I <= I1; I += Coarse)
		{
			if (!Hills.Shade.IsValidIndex(J * Hills.GridW + I)) continue;
			const float Lit = Hills.Shade[J * Hills.GridW + I];
			if (FMath::Abs(Lit) < .06f) continue;
			const FVector2f Ctr = OnPage(Hills.GridX + I * Hills.GridCell, Hills.GridZ + J * Hills.GridCell);
			Pen.Fill(Ctr - FVector2f(Side, Side) * .5f, FVector2f(Side, Side), Lit < 0 ? Hue(0x5e4a36, FMath::Min(.13f, -Lit * .16f)) : Hue(0xfffaf0, FMath::Min(.14f, Lit * .17f)), LPaper);
		}
	}
	const bool bAll = Sc >= .24;
	for (const FContour& Line : Hills.Lines)
	{
		if (!Line.bIndex && !bAll) continue;
		if (Line.Box.Max.X < Shown.Min.X || Line.Box.Min.X > Shown.Max.X || Line.Box.Max.Y < Shown.Min.Y || Line.Box.Min.Y > Shown.Max.Y) continue;
		TArray<FVector2f> Pts;
		Pts.Reserve(Line.P.Num());
		const float Thin = Sc < .3 ? 3.f : 0.f; // fewer points where the sheet is small
		for (const FVector2f& Q : Line.P)
		{
			const FVector2f Sp = OnPage(Q.X, Q.Y);
			if (Pts.Num() && Thin > 0 && FVector2f::DistSquared(Pts.Last(), Sp) < Thin * Thin) continue;
			Pts.Add(Sp);
		}
		if (Pts.Num() < 2) Pts.Add(OnPage(Line.P.Last().X, Line.P.Last().Y));
		Pen.Lines(Pts, Line.bIndex ? Brown(.85f) : Brown(.45f), Line.bIndex ? 1.5f : .9f, LRelief);
	}
	TArray<FBox2f> Placed;
	const auto Room = [&Placed](const FVector2f& Ctr, const FVector2f& Extent)
	{
		const FBox2f Want(Ctr - Extent * .5f - FVector2f(4, 4), Ctr + Extent * .5f + FVector2f(4, 4));
		for (const FBox2f& Prev : Placed) if (Prev.Intersect(Want)) return false;
		Placed.Add(Want);
		return true;
	};
	if (Sc >= .16)
		for (const FContourLabel& Lb : Hills.Labels)
		{
			if (!Visible(Lb.At.X, Lb.At.Y, 0)) continue;
			const FString Txt = FString::FromInt(Lb.Ft);
			const FVector2f Ctr = OnPage(Lb.At.X, Lb.At.Y);
			if (Room(Ctr, FPen::Measure(Txt, Small(13)))) Pen.Text(Txt, Ctr, Small(13), Brown(1.f), LRelief, .5f, .5f, true, Lb.Deg);
		}
	if (Sc >= .14)
		for (const FHeightSpot& Spot : Hills.Spots)
		{
			if (!Visible(Spot.At.X, Spot.At.Y, 0)) continue;
			const FVector2f Ctr = OnPage(Spot.At.X, Spot.At.Y);
			Pen.Lines({ Ctr + FVector2f(-3, -3), Ctr + FVector2f(3, 3) }, Ink(), 1.2f, LRelief);
			Pen.Lines({ Ctr + FVector2f(-3, 3), Ctr + FVector2f(3, -3) }, Ink(), 1.2f, LRelief);
			Pen.Text(FString::FromInt(Spot.Ft), Ctr + FVector2f(6, 0), Small(13), Ink(), LRelief);
		}

	// ---- section lines and their numbers ----
	{
		const int32 C0 = FMath::FloorToInt((Shown.Min.X - TownshipX) / MileM), C1 = FMath::CeilToInt((Shown.Max.X - TownshipX) / MileM);
		const int32 R0 = FMath::FloorToInt((Shown.Min.Y - TownshipZ) / MileM), R1 = FMath::CeilToInt((Shown.Max.Y - TownshipZ) / MileM);
		for (int32 Cn = C0; Cn <= C1; ++Cn) { const double X = TownshipX + Cn * MileM; Pen.Lines({ OnPage(X, Shown.Min.Y - 10), OnPage(X, Shown.Max.Y + 10) }, Survey(.55f), 1.1f, LRelief); }
		for (int32 Rw = R0; Rw <= R1; ++Rw) { const double Z = TownshipZ + Rw * MileM; Pen.Lines({ OnPage(Shown.Min.X - 10, Z), OnPage(Shown.Max.X + 10, Z) }, Survey(.55f), 1.1f, LRelief); }
		if (Sc * MileM >= 120)
			for (int32 Cn = C0; Cn <= C1; ++Cn) for (int32 Rw = R0; Rw <= R1; ++Rw)
			{
				if (Cn < 0 || Cn > 5 || Rw < 0 || Rw > 5) continue;
				const double X = TownshipX + (Cn + .5) * MileM, Z = TownshipZ + (Rw + .5) * MileM;
				if (!Visible(X, Z, 0)) continue;
				Pen.Text(FString::FromInt(SectionNumber(Rw, Cn)), OnPage(X, Z), Printed(Sc * MileM >= 400 ? 34 : 24), Survey(.55f), LRelief, .5f, .5f);
			}
	}

	// ---- water: the dry wash west of the highway, blue, a dash and two dots, and the washes the land's slopes gather ----
	if (Sc >= .1)
		for (const FContour& Wash : Hills.Streams)
		{
			if (Wash.Box.Max.X < Shown.Min.X || Wash.Box.Min.X > Shown.Max.X || Wash.Box.Max.Y < Shown.Min.Y || Wash.Box.Min.Y > Shown.Max.Y) continue;
			TArray<FVector2f> Way;
			for (const FVector2f& Q : Wash.P) Way.Add(OnPage(Q.X, Q.Y));
			Pen.Dashed(Way, Water(.75f), 1.2f, LRoad, { 10, 3, 2, 3 });
		}
	{
		TArray<FVector2f> Wash;
		for (double Z = -100; Z <= S47OpenLand::RimZ1; Z += 40) Wash.Add(OnPage(-330 - .25 * (Z - 500), Z));
		Pen.Dashed(Wash, Water(.9f), 1.6f, LRoad, { 14, 4, 2, 4, 2, 4 });
		const double Lz = FMath::Clamp((Shown.Min.Y + Shown.Max.Y) * .5, 600.0, 3600.0), Lx = -330 - .25 * (Lz - 500);
		if (Visible(Lx, Lz, 0)) Pen.Text(TEXT("Arroyo Seco"), OnPage(Lx, Lz) + FVector2f(-10, 0), Italic(16), Water(1.f), LWords, 1.f, .5f, true, -76.f);
	}

	// ---- roads ----
	const auto Cased = [&Pen](const TArray<FVector2f>& Way, float Thick, const FLinearColor& FillCol, int32 Lr)
	{
		Pen.Lines(Way, Ink(), Thick + 2.2f, Lr);
		Pen.Lines(Way, FillCol, Thick, Lr + 1);
	};
	// The state highway: red, cased.
	{
		const double North = FMath::Max(double(S47RoadTerrain::HWY.Z0), double(S47OpenLand::RimZ0)), South = FMath::Min(double(S47RoadTerrain::HWY.Z1), double(S47OpenLand::RimZ1));
		Cased({ OnPage(0, North), OnPage(0, South) }, FMath::Max(3.2f, float(9.6 * Sc)), Survey(1.f), LRoad);
		const double LabelZ = FMath::Clamp((Shown.Min.Y + Shown.Max.Y) * .5 + 420 / Sc, North + 100, South - 100);
		if (Visible(0, LabelZ, 0)) Pen.Text(TEXT("STATE HIGHWAY"), OnPage(0, LabelZ) + FVector2f(-FMath::Max(10.f, float(6 * Sc)) - 8.f, 0), Printed(17), Ink(), LWords, .5f, 1.f, true, -90.f);
	}
	// SARO's service road, off the highway and round to the pad (S47Flow OnRoad, saro-local).
	{
		TArray<FVector2f> Way;
		const auto Add = [&Way, &OnPage](double Sx, double Sz) { double X = 0, Z = 0; S47OpenLand::RoadOf(TEXT("saro"), Sx, Sz, X, Z); Way.Add(OnPage(X, Z)); };
		Add(-24, 24.5); Add(5.8, 24.5);
		for (int32 I = 1; I <= 6; ++I) { const double A = HALF_PI * I / 6; Add(5.8 + 4 * FMath::Sin(A), 20.5 + 4 * FMath::Cos(A)); }
		Add(9.8, 16.6);
		Cased(Way, FMath::Max(2.2f, float(5 * Sc)), Paper(), LRoad);
	}
	// The ranch road to the Alderete place.
	{
		TArray<FVector2f> Way;
		for (double X = 6; X <= S47RoadTerrain::RANCH_TO; X += 12) Way.Add(OnPage(X, S47RoadTerrain::RanchZ(X)));
		Cased(Way, FMath::Max(2.f, float(4 * Sc)), Paper(), LRoad);
	}
	// The old road's first stretch east, across from the diner.
	{
		const auto& Stub = S47RoadTerrain::OldStub();
		TArray<FVector2f> Way;
		for (const auto& Q : Stub) Way.Add(OnPage(Q.X, Q.Z));
		// Unnamed on this sheet: a county road east, as far as the survey went.
		Cased(Way, FMath::Max(2.f, float(6.4 * Sc)), Paper(), LRoad);
	}
	// The survey track out to STATION 01: unimproved, dashed.
	{
		TArray<FVector2f> Way;
		for (const FVector2D& Q : S47RoadTerrain::Track().Pts) Way.Add(OnPage(Q.X, Q.Y));
		if (Sc >= 2) Pen.Lines(Way, Paper(), float(4.6 * Sc), LRoad);
		Pen.Dashed(Way, Ink(), 1.5f, LRoad + 1, { 9, 5 });
	}
	// Fences: the rim's, and STATION 01's.
	{
		Pen.Dashed({ OnPage(S47OpenLand::RimX0, S47OpenLand::RimZ0), OnPage(S47OpenLand::RimX1, S47OpenLand::RimZ0), OnPage(S47OpenLand::RimX1, S47OpenLand::RimZ1), OnPage(S47OpenLand::RimX0, S47OpenLand::RimZ1), OnPage(S47OpenLand::RimX0, S47OpenLand::RimZ0) }, Ink(), 1.f, LRoad, { 7, 4 });
		const S47Geo::FFence& Gate = S47Geo::STATION_FENCE;
		TArray<FVector2f> Wire;
		for (const FVector2D& Cr : { FVector2D(Gate.X0, Gate.Z0), FVector2D(Gate.X1, Gate.Z0), FVector2D(Gate.X1, Gate.Z1), FVector2D(Gate.X0, Gate.Z1), FVector2D(Gate.X0, Gate.Z0) }) { const S47Geo::FXZ Q = S47Geo::StationToRoad(Cr.X, Cr.Y); Wire.Add(OnPage(Q.X, Q.Z)); }
		Pen.Dashed(Wire, Ink(), 1.f, LRoad, { 5, 3 });
	}

	// ---- the array, the buildings, the land's own features ----
	if (Sc >= .8) for (const auto& Trail : Paths) Pen.Dashed({ OnPage(Trail.Key.X, Trail.Key.Y), OnPage(Trail.Value.X, Trail.Value.Y) }, Revised(.6f), .9f, LRoad, { 4, 3 });
	for (int32 Di = 0; Di < Dishes.Num(); ++Di)
	{
		const FVector2D& Dish = Dishes[Di];
		if (!Visible(Dish.X, Dish.Y, 10)) continue;
		const FVector2f Ctr = OnPage(Dish.X, Dish.Y);
		const float Rad = FMath::Max(2.2f, float(3.6 * Sc));
		Pen.Lines(Pen.Circle(Ctr, Rad, 16), Revised(), 1.2f, LBuilt);
		Pen.Fill(Ctr - FVector2f(1, 1), FVector2f(2, 2), Revised(), LBuilt);
		// Close enough in to tell them apart: each its number, as the array's PC and the jobs have it.
		if (Sc >= .9 && DishIds.IsValidIndex(Di)) Pen.Text(DishIds[Di], Ctr + FVector2f(Rad + 3, -Rad - 1), Small(14), Revised(), LWords, 0.f, .5f, true);
	}
	for (const FBlock& Bk : Blocks)
	{
		if (!Visible(Bk.Box.GetCenter().X, Bk.Box.GetCenter().Y, 80)) continue;
		const FVector2f A = OnPage(Bk.Box.Min.X, Bk.Box.Min.Y), B = OnPage(Bk.Box.Max.X, Bk.Box.Max.Y);
		const FVector2f Ctr = (A + B) * .5f;
		FVector2f Size = B - A;
		Size.X = FMath::Max(Size.X, 2.6f); Size.Y = FMath::Max(Size.Y, 2.6f);
		Pen.Fill(Ctr - Size * .5f, Size, Ink(), LBuilt);
		if (Bk.bChurch)
		{
			const FVector2f Crest = Ctr - FVector2f(0, Size.Y * .5f + 3);
			Pen.Lines({ Crest, Crest - FVector2f(0, 10) }, Ink(), 1.4f, LBuilt);
			Pen.Lines({ Crest - FVector2f(4, 7), Crest - FVector2f(-4, 7) }, Ink(), 1.4f, LBuilt);
		}
	}
	// The land's own names (the catalogue's: the west mesa, the east rim, the playa), and bench marks along the highway.
	if (Sc >= .1)
	{
		if (Hills.bHighWest && Visible(Hills.HighWest.X, Hills.HighWest.Y, 400)) Pen.Text(TEXT("W E S T   M E S A"), OnPage(Hills.HighWest.X, Hills.HighWest.Y), Printed(20), Hue(0x1d1c1a, .85f), LWords, .5f, .5f, false, Hills.HighWestDeg);
		if (Hills.bHighEast && Visible(Hills.HighEast.X, Hills.HighEast.Y, 400)) Pen.Text(TEXT("E A S T   R I M"), OnPage(Hills.HighEast.X, Hills.HighEast.Y), Printed(20), Hue(0x1d1c1a, .85f), LWords, .5f, .5f, false, Hills.HighEastDeg);
		if (Hills.PlayaR > 0 && Visible(Hills.Playa.X, Hills.Playa.Y, Hills.PlayaR))
		{
			const FVector2f Pc = OnPage(Hills.Playa.X, Hills.Playa.Y);
			const float Rx = FMath::Max(16.f, float(Hills.PlayaR * 1.3 * Sc)), Rz = FMath::Max(9.f, float(Hills.PlayaR * .8 * Sc));
			TArray<FVector2f> Shore;
			for (int32 I = 0; I <= 28; ++I) { const float A = 2.f * PI * I / 28; Shore.Add(Pc + FVector2f(FMath::Cos(A) * Rx, FMath::Sin(A) * Rz)); }
			Pen.Dashed(Shore, Water(.85f), 1.3f, LRoad, { 5, 3 });
			Pen.Text(TEXT("Playa"), Pc, Italic(16), Water(1.f), LWords, .5f, .5f);
		}
	}
	if (Sc >= .16)
		for (double Bz = -1500; Bz <= 3000; Bz += 1000)
		{
			if (!Visible(0, Bz, 0)) continue;
			const FVector2f B = OnPage(10, Bz);
			Pen.Lines({ B + FVector2f(FMath::Max(6.f, float(5 * Sc)), -3), B + FVector2f(FMath::Max(6.f, float(5 * Sc)), 3) }, Ink(), 1.f, LWords);
			Pen.Text(FString::Printf(TEXT("BM %d"), FMath::RoundToInt(S47FieldMap::Feet(S47FieldMap::LandHeight(0, Bz)))), B + FVector2f(FMath::Max(10.f, float(5 * Sc)) + 4, 0), Small(12), Ink(), LWords, 0.f, .5f, true);
		}
	// The features' names go on after the places' names, wherever they are clear of them.
	struct FLater { FString Words; FVector2f At; FLinearColor Col; };
	TArray<FLater> Later;
	for (const FFeature& Feature : Features)
	{
		if (!Visible(Feature.At.X, Feature.At.Y, 30)) continue;
		const FVector2f Ctr = OnPage(Feature.At.X, Feature.At.Y);
		if (Feature.Kind == TEXT("windmill"))
		{
			Pen.Lines(Pen.Circle(Ctr, 2.5f, 10), Ink(), 1.2f, LBuilt);
			for (int32 I = 0; I < 4; ++I) { const float A = PI * .25f + HALF_PI * I; const FVector2f D(FMath::Cos(A), FMath::Sin(A)); Pen.Lines({ Ctr + D * 2.5f, Ctr + D * 7.f }, Ink(), 1.2f, LBuilt); }
			if (Sc >= .2) Later.Add({ TEXT("Windmill"), Ctr, Ink() });
		}
		else if (Feature.Kind == TEXT("tank"))
		{
			Pen.Lines(Pen.Circle(Ctr, FMath::Max(3.f, float(3.5 * Sc)), 14), Water(1.f), 1.5f, LBuilt);
			if (Sc >= .2) Later.Add({ TEXT("Tank"), Ctr, Water(1.f) });
		}
		else if (Feature.Kind == TEXT("mine"))
		{
			// crossed picks
			Pen.Lines({ Ctr + FVector2f(-6, -6), Ctr + FVector2f(6, 6) }, Ink(), 1.5f, LBuilt);
			Pen.Lines({ Ctr + FVector2f(6, -6), Ctr + FVector2f(-6, 6) }, Ink(), 1.5f, LBuilt);
			Pen.Lines({ Ctr + FVector2f(-8, -3), Ctr + FVector2f(-3, -8) }, Ink(), 1.5f, LBuilt);
			Pen.Lines({ Ctr + FVector2f(8, -3), Ctr + FVector2f(3, -8) }, Ink(), 1.5f, LBuilt);
			if (Sc >= .2) Later.Add({ TEXT("Mine"), Ctr, Ink() });
		}
		else if (Feature.Kind == TEXT("shack"))
		{
			Pen.Fill(Ctr - FVector2f(2, 2), FVector2f(FMath::Max(4.f, float(4.5 * Sc)), FMath::Max(4.f, float(3.2 * Sc))), Ink(), LBuilt);
			if (Sc >= .2) Later.Add({ TEXT("Line Camp"), Ctr, Ink() });
		}
		else if (Feature.Kind == TEXT("trough"))
		{
			Pen.Lines({ Ctr + FVector2f(-4, 0), Ctr + FVector2f(4, 0) }, Ink(), 2.f, LBuilt);
			if (Sc >= .2) Later.Add({ TEXT("Trough"), Ctr, Ink() });
		}
		else if (Feature.Kind == TEXT("feeder"))
		{
			Pen.Lines({ Ctr + FVector2f(-3, -3), Ctr + FVector2f(3, -3), Ctr + FVector2f(3, 3), Ctr + FVector2f(-3, 3), Ctr + FVector2f(-3, -3) }, Ink(), 1.2f, LBuilt);
			if (Sc >= .3) Later.Add({ TEXT("Feeder"), Ctr, Ink() });
		}
	}

	// ---- printed names ----
	// What is already written where: the printed names and the player's arrow, which the ink below keeps clear of.
	TArray<FBox2f> Occupied;
	const auto Name = [&Pen, &OnPage, &Visible, &Occupied, LWords](const FString& Words, double X, double Z, const FVector2f& By, const FSlateFontInfo& Font, float AlignX, float AlignY, const FLinearColor& Col = FLinearColor::Transparent)
	{
		if (!Visible(X, Z, 200)) return;
		const FVector2f At = OnPage(X, Z) + By;
		if (!InFrame(FVector2D(At.X, At.Y), -40)) return;
		const FVector2f Size = Pen.Text(Words, At, Font, Col.A > 0 ? Col : Ink(), LWords, AlignX, AlignY, true);
		const FVector2f TopLeft = At - FVector2f(Size.X * AlignX, Size.Y * AlignY);
		Occupied.Add(FBox2f(TopLeft, TopLeft + Size));
	};
	{
		const FVector2f Beside(FMath::Max(14.f, float(36 * Sc)), 0);
		Name(TEXT("SARO"), SaroAt.X, SaroAt.Y, Beside, Printed(22), 0.f, 1.f, Revised());
		Name(TEXT("Radio Observatory"), SaroAt.X, SaroAt.Y, Beside + FVector2f(0, 2), Italic(16), 0.f, 0.f, Revised());
		if (Dishes.Num()) { FVector2D Sum = FVector2D::ZeroVector; for (const FVector2D& Dish : Dishes) Sum += Dish; Sum /= Dishes.Num(); Name(TEXT("Antenna array"), Sum.X, Sum.Y, FVector2f(0, -8), Italic(16), .5f, 1.f, Revised()); }
		const S47Geo::FRect& Mo = S47Geo::MOTEL_RECT();
		double Mx = 0, Mz = 0; S47OpenLand::RoadOf(TEXT("saro"), Mo.MinX, (Mo.MinZ + Mo.MaxZ) * .5, Mx, Mz);
		Name(TEXT("Sierra Motor Court"), Mx, Mz, FVector2f(-10, 0), Printed(17), 1.f, .5f);
		Name(TEXT("Mesa Diner"), -28, 2000, FVector2f(-10, 0), Printed(17), 1.f, .5f);
		const auto& Town = S47RoadShape::TOWN();
		Name(TEXT("C A L I C H E"), Town.East, (Town.Z0 + Town.Z1) * .5, FVector2f(FMath::Max(16.f, float(10 * Sc)), 0), Printed(Sc >= 1.5 ? 30 : 24), 0.f, .5f);
		Name(TEXT("Alderete Ranch"), 146, 194, FVector2f(FMath::Max(12.f, float(8 * Sc)), 0), Printed(17), 0.f, .5f);
		// M4's temporary boxes, named beside them on the side away from the highway, once the town is near enough to read.
		if (Sc >= .45)
			for (const S47TownSites::FSite& Site : S47TownSites::Sites())
			{
				double Reach = 0;
				for (const S47TownSites::FPart& Part : Site.Parts) if (Part.bSolid) for (const FBox2D& Foot : S47TownSites::Footprint(Site, Part))
					Reach = FMath::Max(Reach, Site.NameSide > 0 ? Foot.Max.X - Site.X : Site.X - Foot.Min.X);
				Name(Site.MapName, Site.X, Site.Z, FVector2f(float(Site.NameSide) * (float(Reach * Sc) + 8.f), 0), Printed(15), Site.NameSide > 0 ? 0.f : 1.f, .5f);
			}
		const S47Geo::FXZ St = S47Geo::StationToRoad(0, 0);
		Name(TEXT("STATION 01"), St.X - 32, St.Z, FVector2f(-10, 0), Printed(17), 1.f, .5f);
	}

	// The features' names: to the right of the symbol, or left, above or below, wherever nothing printed is yet.
	for (const FLater& Lab : Later)
	{
		const FVector2f Size = FPen::Measure(Lab.Words, Italic(16));
		static const FVector2f Tries[] = { FVector2f(11, 0), FVector2f(-11, 0), FVector2f(0, -16), FVector2f(0, 16) };
		static const FVector2f Aligns[] = { FVector2f(0, .5f), FVector2f(1, .5f), FVector2f(.5f, 1), FVector2f(.5f, 0) };
		int32 Pick = 0;
		for (int32 Ti = 0; Ti < 4; ++Ti)
		{
			const FVector2f TopLeft = Lab.At + Tries[Ti] - Size * Aligns[Ti];
			const FBox2f Want(TopLeft, TopLeft + Size);
			if (!Occupied.ContainsByPredicate([&Want](const FBox2f& Had) { return Had.Intersect(Want); })) { Pick = Ti; break; }
		}
		const FVector2f TopLeft = Lab.At + Tries[Pick] - Size * Aligns[Pick];
		Occupied.Add(FBox2f(TopLeft, TopLeft + Size));
		Pen.Text(Lab.Words, Lab.At + Tries[Pick], Italic(16), Lab.Col, LWords, Aligns[Pick].X, Aligns[Pick].Y, true);
	}

	// ---- pencil: what the player has found, heard of and marked ----
	const FLinearColor Lead = Graphite(.92f);
	for (const FS47MapSector& Sec : View.Sectors)
	{
		const auto Ray = [this](double Deg) { const double A = FMath::DegreesToRadians(Deg); return ToPage(SaroAt.X + FMath::Sin(A) * 9000, SaroAt.Y - FMath::Cos(A) * 9000); };
		const FVector2D From = ToPage(SaroAt.X, SaroAt.Y);
		for (const double D : { Sec.Az - Sec.Spread, Sec.Az + Sec.Spread }) Pen.Dashed({ F2(From), F2(Ray(D)) }, Lead, 1.2f, LPencil, { 7, 6 });
		FVector2D A = From, B = Ray(Sec.Az);
		if (ClipToFrame(A, B)) Pen.Text(Sec.Label, F2(FMath::Lerp(A, B, .8)), Pencil(24), Lead, LPencilWords, .5f, .5f, true);
	}
	// What to look into (heard of on the CB or the radio, in the paper, from Dale or Ward, a bearing's end): a circle with a question
	// mark and its word. Looked into: the word struck through and ticked. Far out on the sheet only the circles, not the words.
	for (const FS47MapPin& Pin : View.Pins)
	{
		const double X = Pin.East, Z = Pin.South + SaroZ;
		if (!Visible(X, Z, 40)) continue;
		const FVector2f Ctr = OnPage(X, Z);
		if (Pin.bHeard)
		{
			Pen.Lines(Pen.Circle(Ctr, 9, 22), Lead, 1.7f, LPencil);
			Pen.Text(TEXT("?"), Ctr + FVector2f(0, 1), Pencil(24), Lead, LPencilWords, .5f, .5f);
		}
		else
		{
			Pen.Lines(Pen.Circle(Ctr, 6, 18), Lead, 1.5f, LPencil);
			Pen.Lines({ Ctr + FVector2f(-4, 0), Ctr + FVector2f(-1, 4), Ctr + FVector2f(6, -6) }, Lead, 2.f, LPencil);
		}
		if (Sc < .15) continue;
		const FVector2f Size = Pen.Text(Pin.Label, Ctr + FVector2f(13, 2), Pencil(24), Lead, LPencilWords, 0.f, .5f, true);
		if (!Pin.bHeard) Pen.Lines({ Ctr + FVector2f(12, 3), Ctr + FVector2f(14 + Size.X, 1) }, Lead, 1.6f, LPencilWords + 2);
	}
	// The work the player has now, written in ink by its place, with a line to it: where the words fit, clear of each other.
	{
		const FLinearColor Blue = Hue(0x24407f, .95f);
		TArray<FBox2f> Written = Occupied;
		if (View.bMe) { const FVector2f MeAt = OnPage(View.MeX, View.MeZ); Written.Add(FBox2f(MeAt - FVector2f(52, 26), MeAt + FVector2f(26, 30))); }
		for (const FS47MapJob& Job : View.Jobs)
		{
			if (!Visible(Job.X, Job.Z, 600)) continue;
			const FVector2f Spot = OnPage(Job.X, Job.Z);
			const FVector2f Size = FPen::Measure(Job.Text, Pencil(26));
			static const FVector2f Offsets[] = { FVector2f(34, -30), FVector2f(34, 30), FVector2f(-34, -30), FVector2f(-34, 30), FVector2f(34, -64), FVector2f(-34, -64), FVector2f(34, 64), FVector2f(-34, 64), FVector2f(60, -100), FVector2f(-60, 100) };
			FVector2f Corner = Spot + Offsets[0]; bool bLeft = false;
			for (const FVector2f& Off : Offsets)
			{
				const bool bL = Off.X < 0;
				const FVector2f TopLeft = Spot + Off - FVector2f(bL ? Size.X : 0.f, Size.Y * .5f);
				const FBox2f Want(TopLeft - FVector2f(4, 2), TopLeft + Size + FVector2f(4, 2));
				if (!Written.ContainsByPredicate([&Want](const FBox2f& Had) { return Had.Intersect(Want); })) { Corner = Spot + Off; bLeft = bL; Written.Add(Want); break; }
			}
			Pen.Lines(Pen.Circle(Spot, 5, 16), Blue, 1.8f, LPencil);
			const FVector2f From = Corner + FVector2f(bLeft ? 4.f : -4.f, 0);
			Pen.Lines({ From, Spot + (From - Spot).GetSafeNormal() * 7.f }, Blue, 1.6f, LPencil);
			Pen.Text(Job.Text, Corner, Pencil(26), Blue, LPencilWords, bLeft ? 1.f : 0.f, .5f, true);
		}
	}
	for (const FS47MapNote& Mark : View.Notes)
	{
		if (!Visible(Mark.X, Mark.Z, 40)) continue;
		const FVector2f Ctr = OnPage(Mark.X, Mark.Z);
		Pen.Lines({ Ctr + FVector2f(-6, -6), Ctr + FVector2f(6, 6) }, Lead, 2.f, LPencil);
		Pen.Lines({ Ctr + FVector2f(6, -6), Ctr + FVector2f(-6, 6) }, Lead, 2.f, LPencil);
		const FVector2f Ring = Ctr + FVector2f(14, -12);
		Pen.Lines(Pen.Circle(Ring, 10, 18), Lead, 1.3f, LPencil);
		Pen.Text(FString::FromInt(Mark.N), Ring, Pencil(22), Lead, LPencilWords, .5f, .5f);
	}
	if (View.bTruck && Visible(View.TruckX, View.TruckZ, 40))
	{
		const double A = FMath::DegreesToRadians(View.TruckHeading);
		const FVector2f Fwd(float(FMath::Sin(A)), float(-FMath::Cos(A))), Side(-Fwd.Y, Fwd.X);
		const float Lg = FMath::Max(12.f, float(5.2 * Sc)) * .5f, Wd = FMath::Max(6.f, float(2.2 * Sc)) * .5f;
		const FVector2f Ctr = OnPage(View.TruckX, View.TruckZ);
		Pen.Lines({ Ctr + Fwd * Lg + Side * Wd, Ctr + Fwd * Lg - Side * Wd, Ctr - Fwd * Lg - Side * Wd, Ctr - Fwd * Lg + Side * Wd, Ctr + Fwd * Lg + Side * Wd }, Lead, 1.8f, LPencil);
		Pen.Lines({ Ctr + Fwd * (Lg * .45f) + Side * Wd, Ctr + Fwd * (Lg * .45f) - Side * Wd }, Lead, 1.4f, LPencil);
		Pen.Text(TEXT("truck"), Ctr + FVector2f(0, Lg + 6), Pencil(24), Lead, LPencilWords, .5f, 0.f, true);
	}

	// ---- you ----
	if (View.bMe)
	{
		const FVector2D At = ToPage(View.MeX, View.MeZ);
		if (InFrame(At, 4))
		{
			const double A = FMath::DegreesToRadians(View.MeHeading);
			const FVector2f Ctr = F2(At), Fwd(float(FMath::Sin(A)), float(-FMath::Cos(A))), Side(-Fwd.Y, Fwd.X);
			Pen.Lines(Pen.Circle(Ctr, 12, 28), YouRed(.95f), 2.2f, LYou);
			Pen.Triangle(Ctr + Fwd * 22.f, Ctr + Fwd * 6.f + Side * 8.f, Ctr + Fwd * 6.f - Side * 8.f, YouRed(1.f), LYou);
			// The exact spot: a dot in the middle of the ring.
			Pen.Fill(Ctr - FVector2f(1.6f, 1.6f), FVector2f(3.2f, 3.2f), YouRed(1.f), LYou);
			// Down and to the left of the arrow: the places' names stand to the right of what they name.
			Pen.Text(TEXT("you"), Ctr + FVector2f(-15, 16), Pencil(26), YouRed(1.f), LYou, 1.f, .5f, true);
		}
		else
		{
			// Off this part of the sheet: an arrow at its edge, toward you.
			const FVector2D Rim = EdgeToward(At, 22);
			const FVector2D Way = (At - FVector2D(FrameMidX, FrameMidY)).GetSafeNormal();
			const FVector2f Ctr = F2(Rim), Fwd = F2(Way), Side(-Fwd.Y, Fwd.X);
			Pen.Triangle(Ctr + Fwd * 14.f, Ctr - Fwd * 6.f + Side * 9.f, Ctr - Fwd * 6.f - Side * 9.f, YouRed(1.f), LYou);
			const double Far = FVector2D::Distance(FVector2D(View.MeX, View.MeZ), Mid);
			Pen.Text(TEXT("you, ") + YardsOrMiles(Far), Ctr - Fwd * 30.f, Pencil(24), YouRed(1.f), LYou, .5f, .5f, true);
		}
	}
	Out.PopClip();

	// ---- the margin ----
	{
		const float L0 = float(FrameL), T0f = float(FrameT), R0f = float(FrameR), B0f = float(FrameB);
		Pen.Lines({ FVector2f(L0, T0f), FVector2f(R0f, T0f), FVector2f(R0f, B0f), FVector2f(L0, B0f), FVector2f(L0, T0f) }, Ink(), 1.6f, LMargin);
		// The corners' latitude and longitude, as a quadrangle prints them (this sheet's own).
		Pen.Text(TEXT("33\u00B022'30\""), FVector2f(L0 - 2, T0f - 4), Small(11), Ink(), LMargin, 0.f, 1.f);
		Pen.Text(TEXT("104\u00B037'30\""), FVector2f(R0f, T0f - 4), Small(11), Ink(), LMargin, 1.f, 1.f);
		Pen.Text(TEXT("33\u00B015'"), FVector2f(L0 - 2, B0f + 4), Small(11), Ink(), LMargin, 0.f, 0.f);
		Pen.Text(TEXT("R. 24 E."), FVector2f((L0 + R0f) * .5f, B0f + 4), Printed(14), Survey(.85f), LMargin, .5f, 0.f);
		Pen.Text(TEXT("T. 9 S."), FVector2f(14, (T0f + B0f) * .5f), Printed(14), Survey(.85f), LMargin, .5f, .5f, false, -90.f);
		// Ticks a kilometre apart, with their eastings and northings.
		const int32 E0 = FMath::CeilToInt(Shown.Min.X / 1000.0), E1 = FMath::FloorToInt(Shown.Max.X / 1000.0);
		for (int32 Ek = E0; Ek <= E1; ++Ek)
		{
			const float X = OnPage(Ek * 1000.0, 0).X;
			Pen.Lines({ FVector2f(X, T0f), FVector2f(X, T0f + 9) }, Ink(), 1.f, LMargin);
			Pen.Lines({ FVector2f(X, B0f), FVector2f(X, B0f - 9) }, Ink(), 1.f, LMargin);
			if (X > L0 + 40 && X < R0f - 40) Pen.Text(FString::Printf(TEXT("%d"), int32((EastingOrigin + Ek * 1000.0) / 1000.0)), FVector2f(X, T0f + 11), Small(11), Ink(), LMargin, .5f, 0.f, true);
		}
		const int32 N0 = FMath::CeilToInt(Shown.Min.Y / 1000.0), N1 = FMath::FloorToInt(Shown.Max.Y / 1000.0);
		for (int32 Nk = N0; Nk <= N1; ++Nk)
		{
			const float Y = OnPage(0, Nk * 1000.0).Y;
			Pen.Lines({ FVector2f(L0, Y), FVector2f(L0 + 9, Y) }, Ink(), 1.f, LMargin);
			Pen.Lines({ FVector2f(R0f, Y), FVector2f(R0f - 9, Y) }, Ink(), 1.f, LMargin);
			if (Y > T0f + 30 && Y < B0f - 30) Pen.Text(FString::Printf(TEXT("%d"), int32((NorthingOrigin - Nk * 1000.0) / 1000.0)), FVector2f(R0f - 12, Y), Small(11), Ink(), LMargin, 1.f, .5f, true);
		}
	}
	// ---- the legend's column ----
	{
		const float Px = PanelL;
		Pen.Text(TEXT("CALICHE QUADRANGLE"), FVector2f(Px, 50), Printed(28), Ink(), LMargin, 0.f, .5f);
		Pen.Text(TEXT("NEW MEXICO / CALICHE CO."), FVector2f(Px, 76), Printed(16), Ink(), LMargin, 0.f, .5f);
		Pen.Text(TEXT("7.5 MINUTE SERIES (TOPOGRAPHIC)"), FVector2f(Px, 96), Small(12), Ink(), LMargin, 0.f, .5f);
		Pen.Lines({ FVector2f(Px, 110), FVector2f(PanelR, 110) }, Ink(), 1.f, LMargin);
		// The scale, for the zoom it is at: feet, and metres or a mile.
		{
			const double ScNow = Sc;
			const auto Bar = [&Pen, LMargin, ScNow](float From, float Y, double Metres, double PerUnit, int32 Cuts, const FString& Unit)
			{
				const float Len = float(Metres * ScNow);
				for (int32 I = 0; I < Cuts; ++I)
				{
					const float A = From + Len * I / Cuts, B = From + Len * (I + 1) / Cuts;
					if (I % 2 == 0) Pen.Fill(FVector2f(A, Y - 2.5f), FVector2f(B - A, 5.f), Ink(), LMargin);
					Pen.Text(FString::Printf(TEXT("%g"), FMath::RoundToDouble(Metres / PerUnit * I / Cuts * 100) / 100), FVector2f(A, Y - 6), Small(10), Ink(), LMargin, .5f, 1.f);
				}
				Pen.Lines({ FVector2f(From, Y - 2.5f), FVector2f(From + Len, Y - 2.5f), FVector2f(From + Len, Y + 2.5f), FVector2f(From, Y + 2.5f), FVector2f(From, Y - 2.5f) }, Ink(), 1.f, LMargin);
				Pen.Text(FString::Printf(TEXT("%g %s"), FMath::RoundToDouble(Metres / PerUnit * 100) / 100, *Unit), FVector2f(From + Len + 6, Y), Small(11), Ink(), LMargin, 0.f, .5f);
			};
			// About 200 page units long, in round numbers.
			const auto Nice = [](double Want, const TArray<double>& Steps) { double Best = Steps[0]; for (const double Step1 : Steps) if (Step1 <= Want) Best = Step1; return Best; };
			Pen.Text(TEXT("SCALE"), FVector2f((Px + PanelR) * .5f, 124), Printed(14), Ink(), LMargin, .5f, .5f);
			const double FeetLen = Nice(200 / Sc / FootM, { 50, 100, 200, 250, 500, 1000, 2000, 2500, 5000, 10000, 20000 });
			Bar(Px + 8, 152, FeetLen * FootM, FootM, 4, TEXT("FEET"));
			const double MetreLen = Nice(200 / Sc, { 20, 25, 50, 100, 200, 250, 500, 1000, 2000, 5000 });
			const bool bMiles = MetreLen >= 1000;
			Bar(Px + 8, 180, bMiles ? MileM * FMath::Max(.5, FMath::RoundToDouble(MetreLen / MileM * 2) / 2) : MetreLen, bMiles ? MileM : 1.0, 4, bMiles ? TEXT("MILE") : TEXT("METERS"));
		}
		Pen.Text(TEXT("CONTOUR INTERVAL 20 FEET"), FVector2f((Px + PanelR) * .5f, 206), Printed(15), Ink(), LMargin, .5f, .5f);
		Pen.Text(TEXT("DATUM IS MEAN SEA LEVEL"), FVector2f((Px + PanelR) * .5f, 224), Small(11), Ink(), LMargin, .5f, .5f);
		// True north and magnetic north, 1984.
		{
			const FVector2f Stem(Px + 30, 300);
			Pen.Lines({ Stem, Stem - FVector2f(0, 50) }, Ink(), 1.2f, LMargin);
			Pen.Text(TEXT("*"), Stem - FVector2f(0, 52), Printed(16), Ink(), LMargin, .5f, 1.f);
			const float A = FMath::DegreesToRadians(float(DeclinationDeg));
			const FVector2f Tip = Stem + FVector2f(FMath::Sin(A), -FMath::Cos(A)) * 46.f;
			Pen.Lines({ Stem, Tip }, Ink(), 1.2f, LMargin);
			Pen.Triangle(Tip, Tip + FVector2f(-3.5f, 8), Tip + FVector2f(3.5f, 8), Ink(), LMargin);
			Pen.Text(TEXT("MN"), Tip + FVector2f(4, -2), Small(11), Ink(), LMargin, 0.f, 1.f);
			Pen.Text(TEXT("11\u00BD\u00B0"), Stem + FVector2f(10, -12), Small(11), Ink(), LMargin, 0.f, .5f);
			Pen.Text(TEXT("UTM GRID AND 1984 MAGNETIC NORTH"), FVector2f(Px + 92, 262), Small(10), Ink(), LMargin, 0.f, .5f);
			Pen.Text(TEXT("DECLINATION AT CENTER OF SHEET"), FVector2f(Px + 92, 276), Small(10), Ink(), LMargin, 0.f, .5f);
		}
		// Road classification and the other symbols, each drawn as the sheet draws it.
		{
			float Y = 334;
			Pen.Text(TEXT("ROAD CLASSIFICATION"), FVector2f(Px, Y), Printed(15), Ink(), LMargin, 0.f, .5f);
			const auto Row = [&Pen, &Y, Px, LMargin](const TCHAR* Words, TFunction<void(const FVector2f&, const FVector2f&)> Symbol)
			{
				Y += 22;
				Symbol(FVector2f(Px, Y), FVector2f(Px + 46, Y));
				Pen.Text(Words, FVector2f(Px + 56, Y), Small(12), Ink(), LMargin, 0.f, .5f);
			};
			Row(TEXT("Primary highway, hard surface"), [&Pen, LMargin](const FVector2f& A, const FVector2f& B) { Pen.Lines({ A, B }, Ink(), 6.f, LMargin); Pen.Lines({ A, B }, Survey(1.f), 3.8f, LMargin + 1); });
			Row(TEXT("Light-duty road, gravel"), [&Pen, LMargin](const FVector2f& A, const FVector2f& B) { Pen.Lines({ A, B }, Ink(), 4.2f, LMargin); Pen.Lines({ A, B }, Paper(), 2.f, LMargin + 1); });
			Row(TEXT("Unimproved road, dirt"), [&Pen, LMargin](const FVector2f& A, const FVector2f& B) { Pen.Dashed({ A, B }, Ink(), 1.5f, LMargin, { 9, 5 }, false); });
			Row(TEXT("Fence line"), [&Pen, LMargin](const FVector2f& A, const FVector2f& B) { Pen.Dashed({ A, B }, Ink(), 1.f, LMargin, { 7, 4 }, false); });
			Row(TEXT("Intermittent stream"), [&Pen, LMargin](const FVector2f& A, const FVector2f& B) { Pen.Dashed({ A, B }, Water(.9f), 1.6f, LMargin, { 14, 4, 2, 4, 2, 4 }, false); });
			Row(TEXT("Building"), [&Pen, LMargin](const FVector2f& A, const FVector2f&) { Pen.Fill(A + FVector2f(20, -3), FVector2f(6, 6), Ink(), LMargin); });
			Row(TEXT("Radio telescope"), [&Pen, LMargin](const FVector2f& A, const FVector2f&) { Pen.Lines(Pen.Circle(A + FVector2f(23, 0), 4, 14), Revised(), 1.3f, LMargin); });
			Y += 30;
			for (const TCHAR* Line : { TEXT("Revisions shown in purple compiled from aerial"), TEXT("photographs taken 1984. This information not"), TEXT("field checked.") }) { Pen.Text(Line, FVector2f(Px, Y), Small(11), Revised(), LMargin, 0.f, .5f); Y += 15; }
			Y += 6;
			Pen.Text(TEXT("Red lines: Public Land Survey, sections 1 mile square."), FVector2f(Px, Y), Small(11), Survey(.9f), LMargin, 0.f, .5f);
			Y += 22;
			for (const TCHAR* Line : { TEXT("Topography by photogrammetric methods from aerial"), TEXT("photographs taken 1977. Field checked 1979.") }) { Pen.Text(Line, FVector2f(Px, Y), Small(11), Ink(), LMargin, 0.f, .5f); Y += 15; }
		}
		// The night desk's stamp, in the same purple, a little crooked; and a ring where a cup stood on the sheet.
		{
			const FVector2f StampAt(Px + 150, 668);
			// Its frame, turned with it: two rules round the words.
			const float Tw = FMath::DegreesToRadians(-4.f);
			const FVector2f Ux(FMath::Cos(Tw), FMath::Sin(Tw)), Uy(-FMath::Sin(Tw), FMath::Cos(Tw));
			for (const float Grow : { 0.f, 4.f })
			{
				const float Hw = 125 + Grow, Hh = 33 + Grow;
				Pen.Lines({ StampAt - Ux * Hw - Uy * Hh, StampAt + Ux * Hw - Uy * Hh, StampAt + Ux * Hw + Uy * Hh, StampAt - Ux * Hw + Uy * Hh, StampAt - Ux * Hw - Uy * Hh }, Revised(.65f), Grow > 0 ? 2.2f : 1.2f, LMargin);
			}
			Pen.Text(TEXT("S A R O  /  N I G H T   D E S K"), StampAt - FVector2f(0, 12), Printed(16), Revised(.75f), LMargin, .5f, .5f, false, -4.f);
			Pen.Text(TEXT("NOT TO LEAVE THE BUILDING"), StampAt + FVector2f(0, 13), Printed(13), Revised(.7f), LMargin, .5f, .5f, false, -4.f);
			const FVector2f Cup(Px + 236, 548);
			for (int32 Ring = 0; Ring < 3; ++Ring)
			{
				TArray<FVector2f> Arc;
				const float R0 = 44.f + Ring * 2.2f, From = .3f + Ring * 1.1f, Span = 4.2f - Ring * .7f;
				for (int32 K = 0; K <= 30; ++K) { const float A = From + Span * K / 30; Arc.Add(Cup + FVector2f(FMath::Cos(A), FMath::Sin(A)) * R0); }
				Pen.Lines(Arc, Hue(0x7a5230, .12f + .04f * Ring), 3.f - Ring * .8f, LMargin);
			}
		}
		Pen.Text(TEXT("CALICHE, N. MEX."), FVector2f(PanelR, 744), Printed(18), Ink(), LMargin, 1.f, .5f);
		Pen.Text(TEXT("1979"), FVector2f(PanelR, 762), Small(11), Ink(), LMargin, 1.f, .5f);
		Pen.Text(TEXT("PHOTOREVISED 1984"), FVector2f(PanelR, 777), Small(11), Revised(), LMargin, 1.f, .5f);
	}
	// Out in the night, read by a flashlight: the sheet at about two thirds. Under the lamps at SARO, full.
	if (NightDim > 0) Pen.Fill(FVector2f(0, 0), FVector2f(1200, 800), FLinearColor(.02f, .015f, .03f, NightDim), LMargin + 2);
	return LMargin + 4;
}

bool S47FieldMap::SelfTest(TArray<FString>& Lines)
{
	using namespace S47FieldMapLocal;
	bool bAll = true;
	const auto Check = [&](bool bOk, const FString& What) { bAll &= bOk; Lines.Add(FString(bOk ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("survey map: ") + What); };
	// Every area's terms come to the same road-local point, and the heading with them (STATION 01's are turned a quarter round).
	{
		bool bSame = true, bHeading = true;
		const S47OpenLand::FSuspend Fixed; // the web version's numbers, whatever a running game has switched on
		for (const TCHAR* Area : { TEXT("saro"), TEXT("road"), TEXT("diner"), TEXT("station01") })
			for (const FVector2D& Q : { FVector2D(33, -300), FVector2D(-603, 532), FVector2D(14, 2095.5), FVector2D(170, 720) })
			{
				const FVector A = S47OpenLand::FromRoad(Area, Q.X, Q.Y);
				const double Yaw = S47OpenLand::Turned(TEXT("road"), Area, -UE_DOUBLE_HALF_PI); // facing east on the land (double: PI and HALF_PI are floats)
				double X = 0, Z = 0, Hd = 0;
				bSame &= OnSheet(Area, A.X, A.Z, Yaw, X, Z, Hd) && FMath::IsNearlyEqual(X, Q.X, 1e-6) && FMath::IsNearlyEqual(Z, Q.Y, 1e-6);
				bHeading &= FMath::IsNearlyEqual(Hd, 90.0, 1e-6);
			}
		Check(bSame, TEXT("SARO's, the road's, the diner's and STATION 01's terms put a point at the same place on the sheet"));
		Check(bHeading, TEXT("facing east is 090 on the sheet in every area, STATION 01's turned terms too"));
		double X = 0, Z = 0, Hd = 0;
		Check(OnSheet(TEXT("road"), 8000, 100, 0, X, Z, Hd) && FMath::IsNearlyEqual(Hd, 0.0, 1e-6) && OnSheet(TEXT("road"), 8000, 100, UE_DOUBLE_PI, X, Z, Hd) && FMath::IsNearlyEqual(Hd, 180.0, 1e-6), TEXT("yaw 0 is north and yaw pi is south"));
		Check(!OnSheet(TEXT("roswell"), 0, 0, 0, X, Z, Hd) && OnSheet(TEXT("room6"), 0, -8000, 0, X, Z, Hd) && FVector2D::Distance(FVector2D(X, Z), FVector2D(-17, 386)) < 30, TEXT("Roswell is off the sheet; room 6 is the motel"));
	}
	Check(Compass(0) == TEXT("north") && Compass(90) == TEXT("east") && Compass(202.5) == TEXT("south-southwest") && Compass(-22.5) == TEXT("north-northwest") && Compass(359) == TEXT("north"), TEXT("the sixteen points"));
	Check(SectionNumber(0, 0) == 6 && SectionNumber(0, 5) == 1 && SectionNumber(1, 0) == 7 && SectionNumber(5, 0) == 31 && SectionNumber(5, 5) == 36 && SectionNumber(2, 5) == 13, TEXT("sections run back and forth, 1 at the northeast corner, 36 at the southeast"));
	// Zoom and move: a page point and the land under it go both ways, and zooming keeps the land under the cursor still.
	{
		const FVector2D Middle(120, 900);
		const FVector2D Q = Project(Middle, 2.5, 140, 880), Back = Unproject(Middle, 2.5, Q);
		bool bAbout = Back.Equals(FVector2D(140, 880), 1e-9) && Q.Equals(FVector2D(FrameMidX + 50, FrameMidY - 50), 1e-9);
		const FVector2D Cursor(300, 200), Before = Unproject(Middle, 1.0, Cursor);
		const FVector2D Moved = Middle + Before - Unproject(Middle, 2.0, Cursor);
		bAbout &= Unproject(Moved, 2.0, Cursor).Equals(Before, 1e-9);
		Check(bAbout, TEXT("page to land and back; the land under the cursor stays put while zooming"));
		FVector2D A(FrameMidX, FrameMidY), B(FrameMidX + 5000, FrameMidY);
		Check(ClipToFrame(A, B) && FMath::IsNearlyEqual(B.X, FrameR, 1e-6) && InFrame(EdgeToward(FVector2D(5000, 9000), 22)), TEXT("a line and an arrow off the sheet stop at its edge"));
	}
	// Contours of a known hill: a cone 100 m high, falling a metre in ten. Its 20 ft lines must lie where the cone says, as closed rings.
	{
		FReliefSheet Peaked;
		const auto Slope = [](double X, double Z) { return 100.0 - FMath::Sqrt(X * X + Z * Z) / 10.0 - DatumM; };
		BuildRelief(Peaked, Slope, -1200, -1200, 1200, 1200, 25);
		bool bOn = true, bClosed = true;
		int32 RingCount = 0;
		double Worst = 0;
		for (const FContour& L : Peaked.Lines)
		{
			const double Want = (100.0 - (L.Ft * FootM)) * 10.0;
			if (Want <= 0 || Want >= 1150) continue;
			++RingCount;
			bClosed &= L.P.Num() > 8 && FVector2f::Distance(L.P[0], L.P.Last()) < 1.f;
			for (const FVector2f& Q : L.P) Worst = FMath::Max(Worst, FMath::Abs(Q.Size() - Want));
		}
		bOn = Worst < 15;
		// Every 20 ft from the foot of what fits on the grid (r 1150 m, 15 m under the datum) to the top (100 m).
		const int32 Planned = FMath::FloorToInt(100.0 / FootM / ContourFeet) - FMath::CeilToInt(-15.0 / FootM / ContourFeet) + 1;
		Check(RingCount == Planned, FString::Printf(TEXT("a cone has a contour every 20 feet: %d rings (%d expected)"), RingCount, Planned));
		Check(bClosed && bOn, FString::Printf(TEXT("each ring is closed and lies where the slope says, within %.1f m on a 25 m grid"), Worst));
		Check(Peaked.Spots.Num() >= 1 && Peaked.Spots[0].At.Size() < 40 && FMath::Abs(Peaked.Spots[0].Ft - FMath::RoundToInt(100 / FootM)) <= 1, TEXT("its top gets a spot height"));
		Check(Peaked.Labels.Num() > 0 && Peaked.Labels.ContainsByPredicate([](const FContourLabel& Lb) { return Lb.Ft % 100 == 0; }), TEXT("index lines carry their heights"));
	}
	Check(FMath::IsNearlyEqual(Feet(0), DatumM / FootM, 1e-6) && FMath::RoundToInt(Feet(0)) == 3888, TEXT("the road's datum is about 3,890 feet"));
	{
		FS47MapView V; V.bField = V.bMe = true; V.MeX = 33; V.MeZ = -260; V.MeHeading = 180;
		const TArray<FString> D = Description(V);
		Check(D.Num() >= 2 && D[1].Contains(TEXT("facing south")), TEXT("the reading says where you are and which way you face"));
	}
	return bAll;
}
