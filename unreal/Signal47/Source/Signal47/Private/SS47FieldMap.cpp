#include "SS47FieldMap.h"
#include "S47EventPlaces.h"
#include "S47FarmBuildings.h"
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
	constexpr double FrameL = 36, FrameT = 66, FrameR = 1164, FrameB = 734;
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
	struct FReliefSheet { bool bBuilt = false, bWithHills = false; TArray<FContour> Lines; TArray<FContourLabel> Labels; TArray<FHeightSpot> Spots; float Low = 0, High = 0; double Millis = 0; };
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
			UE_LOG(LogS47FieldMap, Display, TEXT("S47: survey map contours: %d lines, %d labels, %d spot heights, %.0f to %.0f ft, %.0f ms (relief %s)"),
				Sheet.Lines.Num(), Sheet.Labels.Num(), Sheet.Spots.Num(), FeetOf(Sheet.Low), FeetOf(Sheet.High), Sheet.Millis, bHills ? TEXT("on") : TEXT("off"));
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
		void Dashed(const TArray<FVector2f>& Pts, const FLinearColor& Col, float Thick, int32 Layer, const TArray<float>& Pattern) const
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
				const float Qd[4] = { A.X - float(FrameL - 10), float(FrameR + 10) - A.X, A.Y - float(FrameT - 10), float(FrameB + 10) - A.Y };
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
	Survey();
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

void SS47FieldMap::Survey() const
{
	Blocks.Reset(); Dishes.Reset(); Paths.Reset(); Features.Reset();
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
	if (S47FarmBuildings::Available()) for (const FBox2D& Plot : S47FarmBuildings::Plots()) Blocks.Add({ Plot });
	US47Main* Mn = US47Main::Get(nullptr);
	AS47Web* Web = Mn ? Mn->Web.Get() : nullptr;
	if (Web)
	{
		for (int32 I = 0; I < Web->DishCount(); ++I)
		{
			double X = 0, Z = 0;
			if (S47OpenLand::RoadOf(TEXT("saro"), Web->DishX(I), Web->DishZ(I), X, Z)) Dishes.Add(FVector2D(X, Z));
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
	for (const TCHAR* Id : { TEXT("windmill"), TEXT("tank"), TEXT("mine"), TEXT("shack") })
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
			if (Room(Ctr, FPen::Measure(Txt, Small(11)))) Pen.Text(Txt, Ctr, Small(11), Brown(1.f), LRelief, .5f, .5f, true, Lb.Deg);
		}
	if (Sc >= .14)
		for (const FHeightSpot& Spot : Hills.Spots)
		{
			if (!Visible(Spot.At.X, Spot.At.Y, 0)) continue;
			const FVector2f Ctr = OnPage(Spot.At.X, Spot.At.Y);
			Pen.Lines({ Ctr + FVector2f(-3, -3), Ctr + FVector2f(3, 3) }, Ink(), 1.2f, LRelief);
			Pen.Lines({ Ctr + FVector2f(-3, 3), Ctr + FVector2f(3, -3) }, Ink(), 1.2f, LRelief);
			Pen.Text(FString::FromInt(Spot.Ft), Ctr + FVector2f(6, 0), Small(11), Ink(), LRelief);
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

	// ---- water: the dry wash west of the highway, blue, a dash and two dots ----
	{
		TArray<FVector2f> Wash;
		for (double Z = -100; Z <= S47OpenLand::RimZ1; Z += 40) Wash.Add(OnPage(-330 - .25 * (Z - 500), Z));
		Pen.Dashed(Wash, Water(.9f), 1.6f, LRoad, { 14, 4, 2, 4, 2, 4 });
		const double Lz = FMath::Clamp((Shown.Min.Y + Shown.Max.Y) * .5, 600.0, 3600.0), Lx = -330 - .25 * (Lz - 500);
		if (Visible(Lx, Lz, 0)) Pen.Text(TEXT("Arroyo Seco"), OnPage(Lx, Lz) + FVector2f(-10, 0), Italic(14), Water(1.f), LWords, 1.f, .5f, true, -76.f);
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
		if (Visible(0, LabelZ, 0)) Pen.Text(TEXT("STATE HIGHWAY"), OnPage(0, LabelZ) + FVector2f(-FMath::Max(10.f, float(6 * Sc)) - 8.f, 0), Printed(15), Ink(), LWords, .5f, 1.f, true, -90.f);
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
	if (Sc >= .8) for (const auto& Trail : Paths) Pen.Dashed({ OnPage(Trail.Key.X, Trail.Key.Y), OnPage(Trail.Value.X, Trail.Value.Y) }, Graphite(.6f), .9f, LRoad, { 4, 3 });
	for (const FVector2D& Dish : Dishes)
	{
		if (!Visible(Dish.X, Dish.Y, 10)) continue;
		const FVector2f Ctr = OnPage(Dish.X, Dish.Y);
		Pen.Lines(Pen.Circle(Ctr, FMath::Max(2.2f, float(3.6 * Sc)), 16), Ink(), 1.1f, LBuilt);
		Pen.Fill(Ctr - FVector2f(1, 1), FVector2f(2, 2), Ink(), LBuilt);
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
	for (const FFeature& Feature : Features)
	{
		if (!Visible(Feature.At.X, Feature.At.Y, 30)) continue;
		const FVector2f Ctr = OnPage(Feature.At.X, Feature.At.Y);
		if (Feature.Kind == TEXT("windmill"))
		{
			Pen.Lines(Pen.Circle(Ctr, 2.5f, 10), Ink(), 1.2f, LBuilt);
			for (int32 I = 0; I < 4; ++I) { const float A = PI * .25f + HALF_PI * I; const FVector2f D(FMath::Cos(A), FMath::Sin(A)); Pen.Lines({ Ctr + D * 2.5f, Ctr + D * 7.f }, Ink(), 1.2f, LBuilt); }
			if (Sc >= .2) Pen.Text(TEXT("Windmill"), Ctr + FVector2f(10, 0), Italic(12), Ink(), LWords, 0.f, .5f, true);
		}
		else if (Feature.Kind == TEXT("tank"))
		{
			Pen.Lines(Pen.Circle(Ctr, FMath::Max(3.f, float(3.5 * Sc)), 14), Water(1.f), 1.5f, LBuilt);
			if (Sc >= .2) Pen.Text(TEXT("Tank"), Ctr + FVector2f(9, 0), Italic(12), Water(1.f), LWords, 0.f, .5f, true);
		}
		else if (Feature.Kind == TEXT("mine"))
		{
			// crossed picks
			Pen.Lines({ Ctr + FVector2f(-6, -6), Ctr + FVector2f(6, 6) }, Ink(), 1.5f, LBuilt);
			Pen.Lines({ Ctr + FVector2f(6, -6), Ctr + FVector2f(-6, 6) }, Ink(), 1.5f, LBuilt);
			Pen.Lines({ Ctr + FVector2f(-8, -3), Ctr + FVector2f(-3, -8) }, Ink(), 1.5f, LBuilt);
			Pen.Lines({ Ctr + FVector2f(8, -3), Ctr + FVector2f(3, -8) }, Ink(), 1.5f, LBuilt);
			if (Sc >= .2) Pen.Text(TEXT("Mine"), Ctr + FVector2f(11, 0), Italic(12), Ink(), LWords, 0.f, .5f, true);
		}
		else if (Feature.Kind == TEXT("shack")) Pen.Fill(Ctr - FVector2f(2, 2), FVector2f(FMath::Max(4.f, float(4.5 * Sc)), FMath::Max(4.f, float(3.2 * Sc))), Ink(), LBuilt);
		else if (Feature.Kind == TEXT("feeder"))
		{
			Pen.Lines({ Ctr + FVector2f(-3, -3), Ctr + FVector2f(3, -3), Ctr + FVector2f(3, 3), Ctr + FVector2f(-3, 3), Ctr + FVector2f(-3, -3) }, Ink(), 1.2f, LBuilt);
			if (Sc >= .3) Pen.Text(TEXT("Feeder"), Ctr + FVector2f(8, 0), Italic(12), Ink(), LWords, 0.f, .5f, true);
		}
	}

	// ---- printed names ----
	const auto Name = [&Pen, &OnPage, &Visible, LWords](const FString& Words, double X, double Z, const FVector2f& By, const FSlateFontInfo& Font, float AlignX, float AlignY)
	{
		if (!Visible(X, Z, 200)) return;
		const FVector2f At = OnPage(X, Z) + By;
		if (!InFrame(FVector2D(At.X, At.Y), -40)) return;
		Pen.Text(Words, At, Font, Ink(), LWords, AlignX, AlignY, true);
	};
	{
		const FVector2f Beside(FMath::Max(14.f, float(36 * Sc)), 0);
		Name(TEXT("SARO"), SaroAt.X, SaroAt.Y, Beside, Printed(22), 0.f, 1.f);
		Name(TEXT("Radio Observatory"), SaroAt.X, SaroAt.Y, Beside + FVector2f(0, 2), Italic(13), 0.f, 0.f);
		if (Dishes.Num()) { FVector2D Sum = FVector2D::ZeroVector; for (const FVector2D& Dish : Dishes) Sum += Dish; Sum /= Dishes.Num(); Name(TEXT("Antenna array"), Sum.X, Sum.Y, FVector2f(0, -8), Italic(13), .5f, 1.f); }
		const S47Geo::FRect& Mo = S47Geo::MOTEL_RECT();
		double Mx = 0, Mz = 0; S47OpenLand::RoadOf(TEXT("saro"), Mo.MinX, (Mo.MinZ + Mo.MaxZ) * .5, Mx, Mz);
		Name(TEXT("Sierra Motor Court"), Mx, Mz, FVector2f(-10, 0), Printed(15), 1.f, .5f);
		Name(TEXT("Mesa Diner"), -28, 2000, FVector2f(-10, 0), Printed(15), 1.f, .5f);
		const auto& Town = S47RoadShape::TOWN();
		Name(TEXT("C A L I C H E"), Town.East, (Town.Z0 + Town.Z1) * .5, FVector2f(FMath::Max(16.f, float(10 * Sc)), 0), Printed(Sc >= 1.5 ? 30 : 24), 0.f, .5f);
		Name(TEXT("Alderete Ranch"), 146, 194, FVector2f(FMath::Max(12.f, float(8 * Sc)), 0), Printed(15), 0.f, .5f);
		const S47Geo::FXZ St = S47Geo::StationToRoad(0, 0);
		Name(TEXT("STATION 01"), St.X - 32, St.Z, FVector2f(-10, 0), Printed(15), 1.f, .5f);
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
	for (const FS47MapPin& Pin : View.Pins)
	{
		const double X = Pin.East, Z = Pin.South + SaroZ;
		if (!Visible(X, Z, 40)) continue;
		const FVector2f Ctr = OnPage(X, Z);
		if (Pin.bHeard) Pen.Dashed(Pen.Circle(Ctr, 8, 20), Lead, 1.6f, LPencil, { 4, 3 });
		else Pen.Lines(Pen.Circle(Ctr, 7, 20), Lead, 1.8f, LPencil);
		Pen.Text(Pin.bHeard ? Pin.Label + TEXT("?") : Pin.Label, Ctr + FVector2f(11, 2), Pencil(24), Lead, LPencilWords, 0.f, .5f, true);
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
		Pen.Text(TEXT("truck"), Ctr + FVector2f(Lg + 8, 4), Pencil(24), Lead, LPencilWords, 0.f, .5f, true);
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
			Pen.Text(TEXT("you"), Ctr + FVector2f(16, 14), Pencil(26), YouRed(1.f), LYou, 0.f, .5f, true);
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
		Pen.Text(TEXT("CALICHE QUADRANGLE"), FVector2f(40, 24), Printed(26), Ink(), LMargin, 0.f, .5f);
		Pen.Text(TEXT("NEW MEXICO"), FVector2f(40, 50), Printed(15), Ink(), LMargin, 0.f, .5f);
		Pen.Text(TEXT("7.5 MINUTE SERIES (TOPOGRAPHIC)"), FVector2f(1160, 24), Printed(16), Ink(), LMargin, 1.f, .5f);
		Pen.Text(TEXT("PHOTOREVISED 1981   SARO FIELD COPY"), FVector2f(1160, 48), Small(11), Ink(), LMargin, 1.f, .5f);
		Pen.Text(TEXT("R. 24 E."), FVector2f(600, 52), Printed(14), Survey(.85f), LMargin, .5f, .5f);
		Pen.Text(TEXT("T. 9 S."), FVector2f(18, 400), Printed(14), Survey(.85f), LMargin, .5f, .5f, false, -90.f);
		// Ticks a kilometre apart, with their eastings and northings.
		const int32 E0 = FMath::CeilToInt(Shown.Min.X / 1000.0), E1 = FMath::FloorToInt(Shown.Max.X / 1000.0);
		for (int32 Ek = E0; Ek <= E1; ++Ek)
		{
			const float X = OnPage(Ek * 1000.0, 0).X;
			Pen.Lines({ FVector2f(X, T0f), FVector2f(X, T0f + 9) }, Ink(), 1.f, LMargin);
			Pen.Lines({ FVector2f(X, B0f), FVector2f(X, B0f - 9) }, Ink(), 1.f, LMargin);
			Pen.Text(FString::Printf(TEXT("%d"), int32((EastingOrigin + Ek * 1000.0) / 1000.0)), FVector2f(X, T0f - 6), Small(10), Ink(), LMargin, .5f, 1.f);
		}
		const int32 N0 = FMath::CeilToInt(Shown.Min.Y / 1000.0), N1 = FMath::FloorToInt(Shown.Max.Y / 1000.0);
		for (int32 Nk = N0; Nk <= N1; ++Nk)
		{
			const float Y = OnPage(0, Nk * 1000.0).Y;
			Pen.Lines({ FVector2f(L0, Y), FVector2f(L0 + 9, Y) }, Ink(), 1.f, LMargin);
			Pen.Lines({ FVector2f(R0f, Y), FVector2f(R0f - 9, Y) }, Ink(), 1.f, LMargin);
			Pen.Text(FString::Printf(TEXT("%d"), int32((NorthingOrigin - Nk * 1000.0) / 1000.0)), FVector2f(R0f + 4, Y), Small(10), Ink(), LMargin, 0.f, .5f);
		}
	}
	// The scale, in feet and in metres or miles, for the zoom it is at.
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
		// About 220 page units long, in round numbers.
		const auto Nice = [](double Want, const TArray<double>& Steps) { double Best = Steps[0]; for (const double Step1 : Steps) if (Step1 <= Want) Best = Step1; return Best; };
		const double FeetLen = Nice(220 / Sc / FootM, { 50, 100, 200, 250, 500, 1000, 2000, 2500, 5000, 10000, 20000 });
		Bar(48, 758, FeetLen * FootM, FootM, 4, TEXT("FEET"));
		const double MetreLen = Nice(220 / Sc, { 20, 25, 50, 100, 200, 250, 500, 1000, 2000, 5000 });
		const bool bMiles = MetreLen >= 1000;
		Bar(48, 784, bMiles ? MileM * FMath::Max(.5, FMath::RoundToDouble(MetreLen / MileM * 2) / 2) : MetreLen, bMiles ? MileM : 1.0, 4, bMiles ? TEXT("MILE") : TEXT("METERS"));
	}
	Pen.Text(TEXT("CONTOUR INTERVAL 20 FEET"), FVector2f(640, 756), Printed(15), Ink(), LMargin, .5f, .5f);
	Pen.Text(TEXT("DATUM IS MEAN SEA LEVEL"), FVector2f(640, 778), Small(10), Ink(), LMargin, .5f, .5f);
	// True north and magnetic north (1986).
	{
		const FVector2f Stem(1110, 790);
		Pen.Lines({ Stem, Stem - FVector2f(0, 46) }, Ink(), 1.2f, LMargin);
		Pen.Text(TEXT("TN"), Stem - FVector2f(0, 48), Small(10), Ink(), LMargin, .5f, 1.f);
		const float A = FMath::DegreesToRadians(float(DeclinationDeg));
		const FVector2f Tip = Stem + FVector2f(FMath::Sin(A), -FMath::Cos(A)) * 42.f;
		Pen.Lines({ Stem, Tip }, Ink(), 1.2f, LMargin);
		Pen.Triangle(Tip, Tip + FVector2f(-3.5f, 8), Tip + FVector2f(3.5f, 8), Ink(), LMargin);
		Pen.Text(TEXT("MN"), Tip + FVector2f(4, -2), Small(10), Ink(), LMargin, 0.f, 1.f);
		Pen.Text(TEXT("11½°"), Stem + FVector2f(12, -14), Small(10), Ink(), LMargin, 0.f, .5f);
	}
	return LMargin + 2;
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
				const double Yaw = S47OpenLand::Turned(TEXT("road"), Area, -HALF_PI); // facing east on the land
				double X = 0, Z = 0, Hd = 0;
				bSame &= OnSheet(Area, A.X, A.Z, Yaw, X, Z, Hd) && FMath::IsNearlyEqual(X, Q.X, 1e-6) && FMath::IsNearlyEqual(Z, Q.Y, 1e-6);
				bHeading &= FMath::IsNearlyEqual(Hd, 90.0, 1e-6);
			}
		Check(bSame, TEXT("SARO's, the road's, the diner's and STATION 01's terms put a point at the same place on the sheet"));
		Check(bHeading, TEXT("facing east is 090 on the sheet in every area, STATION 01's turned terms too"));
		double X = 0, Z = 0, Hd = 0;
		Check(OnSheet(TEXT("road"), 8000, 100, 0, X, Z, Hd) && FMath::IsNearlyEqual(Hd, 0.0, 1e-6) && OnSheet(TEXT("road"), 8000, 100, PI, X, Z, Hd) && FMath::IsNearlyEqual(Hd, 180.0, 1e-6), TEXT("yaw 0 is north and yaw pi is south"));
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
		for (const FContour& L : Peaked.Lines)
		{
			const double Want = (100.0 - (L.Ft * FootM)) * 10.0;
			if (Want <= 0 || Want >= 1150) continue;
			++RingCount;
			bClosed &= L.P.Num() > 8 && FVector2f::Distance(L.P[0], L.P.Last()) < 1.f;
			for (const FVector2f& Q : L.P) bOn &= FMath::Abs(Q.Size() - Want) < 12.5 + Want * .02;
		}
		const int32 Planned = FMath::FloorToInt(100.0 / FootM / ContourFeet);
		Check(bOn && bClosed && FMath::Abs(RingCount - Planned) <= 1, FString::Printf(TEXT("a cone's contours are %d closed rings where its slope says (%d expected), within half a cell"), RingCount, Planned));
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
