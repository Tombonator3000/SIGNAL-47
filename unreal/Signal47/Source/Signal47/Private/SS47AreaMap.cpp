#include "SS47AreaMap.h"
#include "S47Main.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "SS47Widgets.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FLinearColor PAPER = FLinearColor::FromSRGBColor(FColor(0xf1, 0xea, 0xd8)), PENCIL = FLinearColor::FromSRGBColor(FColor(0x3b, 0x3b, 0x3f)), RED = FLinearColor::FromSRGBColor(FColor(0xa8, 0x28, 0x1e));
	constexpr float W = 1200, H = 800;
	/** The highway's line on the page. */
	constexpr float HX = 420;
	const FVector2f SARO(532, 202), MOTEL(371, 261), DINER(387, 609);
	const FVector2f MOUTH(HX - 12, 360), STATION(170, 372);

	/** Down the highway, metres south of SARO to the page: SARO, the motel, the track, the diner. */
	float HwyY(double M)
	{
		static const double HWY_Y[4][2] = { { 0, 202 + 14 }, { 690, 261 }, { 815, 360 }, { 2300, 609 } };
		for (int32 i = 1; i < 4; ++i)
		{
			const double M0 = HWY_Y[i - 1][0], Y0 = HWY_Y[i - 1][1], M1 = HWY_Y[i][0], Y1 = HWY_Y[i][1];
			if (M <= M1 || i == 3) return float(Y0 + (Y1 - Y0) * FMath::Min(1.0, FMath::Max(0.0, (M - M0) / (M1 - M0))));
		}
		return float(HWY_Y[0][1]);
	}

	FString Hhmm(double C)
	{
		const double D = FMath::Fmod(FMath::Fmod(C, 86400.0) + 86400.0, 86400.0);
		return FString::Printf(TEXT("%02d:%02d"), int32(FMath::FloorToDouble(D / 3600)), int32(FMath::FloorToDouble(FMath::Fmod(D, 3600.0) / 60)));
	}

	// The page in canvas terms: strokes, arcs and text in the page's own 1200 x 800 pixels.
	struct FPage
	{
		FSlateWindowElementList& Out;
		const FGeometry& Page; // the child geometry: page pixels, scaled to the widget
		int32 Layer;
		float Alpha = 1.f;
		uint32 Seed = 7;
		float LineW = 2.f;
		FLinearColor Stroke = PENCIL;
		TArray<float> Dash;

		// pencil: a little uneven (an LCG, the same strokes every time)
		float R() { Seed = Seed * 1664525u + 1013904223u; return float(double(Seed) / 4294967296.0 - 0.5); }

		FLinearColor Tint(const FLinearColor& C, float A = 1.f) const { return FLinearColor(C.R, C.G, C.B, C.A * Alpha * A); }

		void Stroke1(const TArray<FVector2f>& P, float Wd, const FLinearColor& Col)
		{
			if (P.Num() < 2) return;
			if (Dash.Num() < 2) { FSlateDrawElement::MakeLines(Out, Layer, Page.ToPaintGeometry(), P, ESlateDrawEffect::None, Tint(Col), true, Wd); return; }
			// setLineDash: on and off along the whole path
			int32 Di = 0; float Left = Dash[0]; bool bOn = true;
			for (int32 i = 1; i < P.Num(); ++i)
			{
				FVector2f A = P[i - 1];
				const FVector2f B = P[i];
				float Seg = FVector2f::Distance(A, B);
				const FVector2f Dir = Seg > 0 ? (B - A) / Seg : FVector2f::ZeroVector;
				while (Seg > 0)
				{
					const float Step = FMath::Min(Left, Seg);
					const FVector2f E = A + Dir * Step;
					if (bOn) FSlateDrawElement::MakeLines(Out, Layer, Page.ToPaintGeometry(), TArray<FVector2f>{ A, E }, ESlateDrawEffect::None, Tint(Col), true, Wd);
					A = E; Seg -= Step; Left -= Step;
					if (Left <= 0) { Di = (Di + 1) % Dash.Num(); Left = Dash[Di]; bOn = !bOn; }
				}
			}
		}
		// line(): two passes, each point a little off
		void Line(const TArray<FVector2f>& Pts, float Wd = 2.2f)
		{
			for (int32 Pass = 0; Pass < 2; ++Pass)
			{
				TArray<FVector2f> J;
				for (const FVector2f& P : Pts) { const float Jx = P.X + R() * 1.6f, Jy = P.Y + R() * 1.6f; J.Add(FVector2f(Jx, Jy)); }
				Stroke1(J, Wd, PENCIL);
			}
		}
		void Box(float X0, float Y0, float X1, float Y1) { Line({ { X0, Y0 }, { X1, Y0 }, { X1, Y1 }, { X0, Y1 }, { X0, Y0 } }); }
		TArray<FVector2f> Arc(float X, float Y, float Rad) const
		{
			TArray<FVector2f> P;
			for (int32 i = 0; i <= 28; ++i) { const float A = 2.f * PI * i / 28; P.Add(FVector2f(X + Rad * FMath::Cos(A), Y + Rad * FMath::Sin(A))); }
			return P;
		}
		void Text(const FString& S, float X, float Y, float Size = 24, const FLinearColor& Col = PENCIL)
		{
			// fillText at the baseline: the box starts the font's ascent above it
			const FSlateFontInfo F = S47Style::Hand(Size);
			const TSharedRef<FSlateFontMeasure> M = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			const float Asc = float(M->GetMaxCharacterHeight(F) + M->GetBaseline(F));
			FSlateDrawElement::MakeText(Out, Layer + 1, Page.ToPaintGeometry(FVector2f(800.f, Size * 2.f), FSlateLayoutTransform(FVector2f(X, Y - Asc))), S, F, ESlateDrawEffect::None, Tint(Col));
		}
		// drawings.ts mark(): the fixed-point mark, a triangle and its bar
		void Mark(float Cx, float Cy, float Size)
		{
			Stroke1({ { Cx, Cy - Size * 0.58f }, { Cx - Size * 0.55f, Cy + Size * 0.4f }, { Cx + Size * 0.55f, Cy + Size * 0.4f }, { Cx, Cy - Size * 0.58f } }, LineW, PENCIL);
			Stroke1({ { Cx - Size * 0.4f, Cy + Size * 0.62f }, { Cx + Size * 0.4f, Cy + Size * 0.62f } }, LineW, PENCIL);
		}
	};
}

FVector2f S47AreaMap::SpotXY(const FS47MapAt& At)
{
	if (At.Spot == TEXT("yard")) return FVector2f(SARO.X + 22, SARO.Y - 30);
	if (At.Spot == TEXT("motel")) return MOTEL;
	// the motel's office, at its north end: just off the corner of its box
	if (At.Spot == TEXT("office")) return FVector2f(MOTEL.X - 23, MOTEL.Y - 25);
	if (At.Spot == TEXT("highway")) return FVector2f(HX, HwyY(At.M));
	if (At.Spot == TEXT("diner")) return DINER;
	if (At.Spot == TEXT("track"))
	{
		const float T = FMath::Clamp(float(At.T), 0.f, 1.f);
		return FVector2f(MOUTH.X + (STATION.X + 26 - MOUTH.X) * T, MOUTH.Y + (STATION.Y - MOUTH.Y) * T);
	}
	if (At.Spot == TEXT("station01")) return STATION;
	// 'oldroad': onPage(SERVICE_MAP.post(mi)) needs the service map (story/drawings.ts), not ported
	// yet; the junction at the diner until it is
	if (At.Spot == TEXT("oldroad")) return DINER;
	return SARO;
}

bool S47AreaMap::LinesMeet(const FS47MapView& V)
{
	int32 A = 0, B = 0;
	for (const FS47MapMark& M : V.Marks) { if (M.Line.IsEmpty() || M.Line == TEXT("A")) ++A; else if (M.Line == TEXT("B")) ++B; }
	return V.bLineC && V.bOldRoad && V.bStation && A >= 3 && B >= 3;
}

TArray<FString> S47AreaMap::Description(const FS47MapView& V)
{
	// areaMapDescription: the same facts as the pencil labels
	TArray<FString> T = { TEXT("A pencil sketch of the area round SARO, from memory."), TEXT("The highway."), TEXT("SARO. 27 dishes, azimuth 026."),
		TEXT("Sierra Motor Court. Nora, room 6."), TEXT("Mesa Diner. Open all night."), TEXT("Caliche, population 412.") };
	if (US47Main::Get(nullptr)->IsFreeform())
	{
		T.Add(TEXT("SARO service bench: outside the building by the SARO sign. Unload supplies and test donor controllers here."));
		T.Add(TEXT("Caliche gas station: west side of the highway, just south of Mesa Diner. The exterior supply hatch sells parts and donor controllers, 08:00-20:00; essential emergency requisitions are available after hours."));
		T.Add(TEXT("Drive south from SARO past the motel to Caliche; return north with the parcel secured in the service truck."));
	}
	if (V.bStation) T.Add(TEXT("STATION 01. 1947."));
	T.Add(V.bOldRoad ? TEXT("The old road to Roswell.") : TEXT("The old road is closed."));
	if (V.Marks.Num()) T.Add(TEXT("Small circles mark events and their times."));
	if (V.Marks.ContainsByPredicate([](const FS47MapMark& M) { return M.Kind == TEXT("away"); })) T.Add(TEXT("A ring marks time away."));
	int32 A = 0, B = 0;
	for (const FS47MapMark& M : V.Marks) { if (M.Line.IsEmpty() || M.Line == TEXT("A")) ++A; else if (M.Line == TEXT("B")) ++B; }
	if (A >= 3) T.Add(TEXT("Line A."));
	if (B >= 3) T.Add(TEXT("Line B."));
	if (V.At)
	{
		static const TMap<FString, FString> POS = {
			{ TEXT("saro"), TEXT("The red cross marks me at SARO.") }, { TEXT("yard"), TEXT("The red cross marks me in the service yard.") },
			{ TEXT("motel"), TEXT("The red cross marks me at Sierra Motor Court.") }, { TEXT("office"), TEXT("The red cross marks me at the motel office.") },
			{ TEXT("highway"), TEXT("The red cross marks me on the highway.") }, { TEXT("diner"), TEXT("The red cross marks me at Mesa Diner.") },
			{ TEXT("track"), TEXT("The red cross marks me on the track to STATION 01.") }, { TEXT("station01"), TEXT("The red cross marks me at STATION 01.") },
			{ TEXT("oldroad"), TEXT("The red cross marks me on the old road to Roswell.") } };
		if (const FString* P = POS.Find(V.At->Spot)) T.Add(*P);
	}
	return T;
}

int32 SS47AreaMap::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	const float K = float(G.GetLocalSize().X) / W;
	const FGeometry Pg = G.MakeChild(FVector2f(W, H), FSlateLayoutTransform(K));
	FPage P{ Out, Pg, Layer };
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	// a notebook page: faint ruled lines and a margin
	FSlateDrawElement::MakeBox(Out, Layer, Pg.ToPaintGeometry(), White, ESlateDrawEffect::None, PAPER);
	P.Layer = Layer + 1;
	{
		FLinearColor Rule = FLinearColor::FromSRGBColor(FColor(70, 110, 160)), Margin = FLinearColor::FromSRGBColor(FColor(190, 60, 50));
		Rule.A = 0.16f; Margin.A = 0.25f;
		for (float Y = 60; Y < H; Y += 34) P.Stroke1({ FVector2f(0.f, Y), FVector2f(W, Y) }, 1.5f, Rule);
		P.Stroke1({ FVector2f(70.f, 0.f), FVector2f(70.f, H) }, 1.5f, Margin);
	}
	P.Alpha = 0.9f;
	P.Text(TEXT("the area round SARO, from memory"), 96, 50, 32);
	// north
	P.Line({ { 1120, 740 }, { 1120, 670 } }); P.Line({ { 1110, 690 }, { 1120, 670 }, { 1130, 690 } }); P.Text(TEXT("N"), 1135, 700, 30);
	// the highway, two lines
	P.Line({ { HX - 12, 96 }, { HX - 12, 780 } }, 1.8f); P.Line({ { HX + 12, 96 }, { HX + 12, 780 } }, 1.8f);
	P.Text(TEXT("highway"), HX - 116, 490, 24);
	// SARO east of it, its array to the north, and the service road out to the highway
	for (int32 i = 0; i < 9; ++i)
	{
		const float X = 520.f + (i % 3) * 30, Y = 118.f + (i / 3) * 22;
		const float Jx = P.R() * 2, Jy = P.R() * 2;
		P.Stroke1(P.Arc(X + Jx, Y + Jy, 7), 2.f, PENCIL);
	}
	P.Text(TEXT("27 dishes, az 026"), 620, 140, 22);
	P.Box(506, 186, 558, 218);
	P.Text(TEXT("SARO"), 568, 214, 30);
	P.Line({ { HX + 12, 210 }, { 506, 206 } }, 1.4f);
	// the motel down the highway, on the west side
	P.Box(340, 250, 402, 272);
	P.Text(TEXT("Sierra Motor Ct"), 186, 262, 24); P.Text(TEXT("(Nora, rm 6)"), 214, 288, 20);
	// the diner at the junction, on the west side too
	P.Box(370, 598, 404, 620);
	P.Text(TEXT("Mesa Diner"), 226, 652, 28); P.Text(TEXT("open all night"), 236, 678, 20);
	// Caliche down the street from it, both sides of the highway (world/Town.ts)
	for (int32 i = 0; i < 3; ++i) { P.Box(392, 628 + i * 15, 408, 638 + i * 15); P.Box(HX + 10, 632 + i * 15, HX + 24, 641 + i * 15); }
	P.Text(TEXT("Caliche, pop. 412"), 226, 708, 22);
	if (US47Main::Get(nullptr)->IsFreeform())
	{
		// Use the existing schematic town/building positions, not a second metric scale.
		P.Stroke1(P.Arc(554, 222, 5), 2.f, PENCIL);
		P.Line({ { 554, 222 }, { 630, 248 }, { 650, 248 } }, 1.4f);
		P.Text(TEXT("SARO service bench"), 660, 256, 24);
		P.Text(TEXT("outside, by the SARO sign"), 660, 282, 20);
		P.Stroke1(P.Arc(400, 636, 7), 2.f, PENCIL);
		P.Line({ { 400, 636 }, { 470, 702 }, { 588, 702 } }, 1.4f);
		P.Text(TEXT("Gas station / supply hatch"), 600, 712, 24);
		P.Text(TEXT("Parts + donors 08:00-20:00"), 600, 738, 20);
		P.Text(TEXT("Essential emergency requisitions after hours"), 600, 764, 18);
	}
	// the old road east (open: SERVICE_MAP.road and its mileposts, story/drawings.ts, which is not
	// ported yet; the road is drawn closed until it is)
	{
		P.Line({ { HX + 12, 612 }, { 478, 630 } }, 2.2f);
		P.Text(TEXT("old road (closed)"), 450, 664, 20);
	}
	// the track west to STATION 01
	if (View.bStation)
	{
		P.Dash = { 10, 9 }; P.Line({ MOUTH, { STATION.X + 26, STATION.Y } }, 2); P.Dash.Reset();
		P.LineW = 2.4f; P.Mark(STATION.X, STATION.Y, 22);
		P.Text(TEXT("STATION 01"), STATION.X - 66, STATION.Y - 30, 26); P.Text(TEXT("1947"), STATION.X - 18, STATION.Y + 42, 20);
	}
	// what happened where, with the time; three on one line and the line is drawn through them
	int32 OnA = 0;
	for (const FS47MapMark& M : View.Marks) if (M.Line.IsEmpty() || M.Line == TEXT("A")) ++OnA;
	if (OnA >= 3)
	{
		// A from beyond the town, through it, to STATION 01 and a little past (B and C, and where the
		// three meet, need story/lines.ts CROSSINGS and the service map: drawn once they are ported)
		const FVector2f From(DINER.X, DINER.Y + 30);
		auto At = [&](float Kk) { return From + (STATION - From) * Kk; };
		P.Dash = { 3, 7 }; P.Line({ At(-0.35f), At(1.12f) }, 1.6f); P.Dash.Reset();
		P.Text(TEXT("A"), At(1.12f).X - 28, At(1.12f).Y - 4, 28);
	}
	TArray<FString> InTown;
	int32 Kn = 0;
	TArray<FVector2f> Drawn;
	for (const FS47MapMark& M : View.Marks)
	{
		const FVector2f Pt = M.At ? S47AreaMap::SpotXY(*M.At) : FVector2f(HX - 4 + (Kn % 3) * 14, 640.f + (Kn / 3) * 14);
		if (!M.At) ++Kn;
		P.Stroke1(P.Arc(Pt.X, Pt.Y, 6), 2.f, PENCIL);
		// a time away: a ring round it
		if (M.Kind == TEXT("away")) P.Stroke1(P.Arc(Pt.X, Pt.Y, 13), 2.f, PENCIL);
		const int32 Under = Drawn.FilterByPredicate([&](const FVector2f& D) { return FVector2f::Distance(D, Pt) < 12; }).Num();
		const float Ty = Pt.Y + 6 + Under * 20, Tx = Ty > 246 && Ty < 292 && Pt.X - 64 < 404 && Pt.X - 18 > 338 ? Pt.X + 16 : Pt.X - 64;
		if (M.At) { P.Text(Hhmm(M.Clock), Tx, Ty, 20); Drawn.Add(Pt); } else InTown.Add(Hhmm(M.Clock));
	}
	// the times in town, in a row under its name
	if (InTown.Num()) P.Text(FString::Join(InTown, TEXT(", ")), 226, 734, 20);
	// where the player is
	if (View.At)
	{
		const FVector2f X = S47AreaMap::SpotXY(*View.At);
		P.Alpha = 1.f;
		P.Stroke1({ { X.X - 11, X.Y - 11 }, { X.X + 11, X.Y + 11 } }, 4.f, RED);
		P.Stroke1({ { X.X + 11, X.Y - 11 }, { X.X - 11, X.Y + 11 } }, 4.f, RED);
		P.Text(TEXT("me"), X.X + 14, X.Y - 10, 24, RED);
	}
	return Layer + 3;
}
