// story/drawings.ts (the drawings chapter one and the chapters after it use from it)
#include "S47Drawings.h"

#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/App.h"
#include "Rendering/SlateRenderer.h"
#include "S47Canvas.h"
#include "S47Image.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Images/SImage.h"

namespace
{
	// fonts as the drawings name them: '600 26px Oswald' (only Oswald's regular weight is in
	// Content/Fonts, so the 600 is drawn as 400), '19px "Special Elite"'
	const TCHAR* OSWALD = TEXT("Oswald-Regular.ttf");
	const TCHAR* ELITE = TEXT("SpecialElite-Regular.ttf");
	// (chapter five's drawings: the page's Oswald is one variable face, 200 to 700; of the two in
	// Content/Fonts, Medium is the nearer to the 600 they ask for)
	const TCHAR* OSWALD_600 = TEXT("Oswald-Medium.ttf");
}

void S47Drawings::PaperBg(FS47Canvas& G, int32 W, int32 H)
{
	// paperBg
	G.SetFill(PAPER); G.FillRect(0, 0, W, H);
	uint32 S = uint32(W * 31 + H);
	auto R = [&S] { S = S * 1664525u + 1013904223u; return S / 4294967296.0; };
	for (int32 i = 0; i < 900; ++i)
	{
		G.SetFill(0x5a4628, R() * 0.05); // rgba(90,70,40,r*0.05)
		const double X = R() * W, Y = R() * H, Rw = 1 + R() * 2, Rh = 1 + R() * 2;
		G.FillRect(X, Y, Rw, Rh);
	}
}

FString S47Drawings::Sheet(int32 W, int32 H, TFunctionRef<void(FS47Canvas&)> Draw)
{
	// sheet
	FS47Canvas G(W, H);
	PaperBg(G, W, H);
	G.SetStroke(INK); G.SetFill(INK); G.LineWidth = 3; // lineCap and lineJoin round (FS47Canvas strokes so)
	Draw(G);
	return G.ToDataUrl(false);
}

void S47Drawings::Mark(FS47Canvas& G, double Cx, double Cy, double Size, bool bBar)
{
	// mark
	G.BeginPath();
	G.MoveTo(Cx, Cy - Size * 0.58); G.LineTo(Cx - Size * 0.55, Cy + Size * 0.4); G.LineTo(Cx + Size * 0.55, Cy + Size * 0.4); G.ClosePath();
	G.Stroke();
	if (bBar) { G.BeginPath(); G.MoveTo(Cx - Size * 0.4, Cy + Size * 0.62); G.LineTo(Cx + Size * 0.4, Cy + Size * 0.62); G.Stroke(); }
}

FString S47Drawings::Arrangement(bool bFull)
{
	// arrangement(full)
	return Sheet(960, 420, [bFull](FS47Canvas& G)
	{
		G.SetFont(OSWALD, 26);
		G.FillText(bFull ? TEXT("ARRANGEMENT / STATION 01 / 1947") : TEXT("ARRANGEMENT / STATION 01 / 1947 / SERVICE COPY"), 40, 56);
		G.SetFont(ELITE, 19);
		G.FillText(bFull ? TEXT("A: fixed survey point     B: comparison vane     C: closing sight line") : TEXT("A: fixed survey point     B: comparison vane"), 40, 92);
		G.GlobalAlpha = 0.5; G.LineWidth = 1.5;
		G.BeginPath(); G.MoveTo(40, 112); G.LineTo(920, 112); G.Stroke();
		G.GlobalAlpha = 1; G.LineWidth = 3;
		Mark(G, 150, 250, 64);
		G.BeginPath(); G.MoveTo(196, 250); G.LineTo(470, 250); G.Stroke();      // A to B
		G.BeginPath(); G.MoveTo(500, 218); G.LineTo(500, 282); G.Stroke();      // B, the vane
		if (bFull)
		{
			G.BeginPath(); G.MoveTo(506, 250); G.LineTo(790, 250); G.Stroke();    // the closing sight line
			G.BeginPath(); G.Arc(820, 250, 28, 0, PI * 2); G.Stroke();           // C
		}
		G.SetFont(ELITE, 28);
		G.FillText(TEXT("A"), 140, 340); G.FillText(TEXT("B"), 490, 340); if (bFull) G.FillText(TEXT("C"), 810, 340);
		G.SetFont(ELITE, 17);
		G.FillText(TEXT("N. VEGA"), 790, 396);
	});
}

FString S47Drawings::SleeveMark()
{
	// sleeveMark
	return Sheet(420, 420, [](FS47Canvas& G)
	{
		G.SetFont(OSWALD, 24);
		G.FillText(TEXT("FIXED-POINT SHEET"), 34, 54);
		G.SetFont(ELITE, 18);
		G.FillText(TEXT("INDEX DIAGRAM"), 34, 84);
		G.LineWidth = 4;
		Mark(G, 210, 220, 120);
		G.SetFont(ELITE, 24);
		G.FillText(TEXT("A  -  B  -  C"), 132, 340);
		G.SetFont(ELITE, 18);
		G.FillText(TEXT("STATION 01 / B-12"), 128, 376);
	});
}

FString S47Drawings::MarkIcon(const FString& Kind)
{
	// markIcon(kind)
	return Sheet(120, 120, [&Kind](FS47Canvas& G)
	{
		G.LineWidth = 4;
		if (Kind == TEXT("three-bars")) { for (int32 i = 0; i < 3; ++i) { G.BeginPath(); G.MoveTo(26, 38 + i * 22); G.LineTo(94, 38 + i * 22); G.Stroke(); } }
		else Mark(G, 60, 58, 70, Kind == TEXT("triangle-bar"));
	});
}

FString S47Drawings::TransitRecord(const FVector2D& A, const FVector2D& B, const FVector2D& C)
{
	// transitRecord({ a, b, c })
	return Sheet(960, 620, [&A, &B, &C](FS47Canvas& G)
	{
		G.SetFont(OSWALD, 26);
		G.FillText(TEXT("FIELD TRANSIT / STATION 01 / E09A"), 40, 56);
		G.SetFont(ELITE, 19);
		G.FillText(TEXT("Plan, 1947. Fixed points set in concrete footings."), 40, 92);
		G.GlobalAlpha = 0.5; G.LineWidth = 1.5;
		G.BeginPath(); G.MoveTo(40, 112); G.LineTo(920, 112); G.Stroke();
		G.GlobalAlpha = 1; G.LineWidth = 3;
		// fit A and B (C is a direction) into the sheet around the transit
		const double Reach = FMath::Max3(4.0, A.Size(), B.Size());
		const double K = 200 / Reach, Cx = 480, Cy = 360;
		auto P = [&](const FVector2D& V) { return FVector2D(Cx + V.X * K, Cy + V.Y * K); };
		const FVector2D Pa = P(A), Pb = P(B);
		const double Ax = Pa.X, Ay = Pa.Y, Bx = Pb.X, By = Pb.Y;
		// transit: a circle with cross hairs on the pier
		G.BeginPath(); G.Arc(Cx, Cy, 16, 0, PI * 2); G.Stroke();
		G.BeginPath(); G.MoveTo(Cx - 24, Cy); G.LineTo(Cx + 24, Cy); G.MoveTo(Cx, Cy - 24); G.LineTo(Cx, Cy + 24); G.Stroke();
		// the transit sight line through A, dashed past it
		G.SetLineDash({ 10, 8 }); G.LineWidth = 2;
		double Dl = FMath::Sqrt(FMath::Square(Ax - Cx) + FMath::Square(Ay - Cy));
		if (Dl == 0) Dl = 1;
		G.BeginPath(); G.MoveTo(Cx, Cy); G.LineTo(Cx + (Ax - Cx) / Dl * (Dl + 70), Cy + (Ay - Cy) / Dl * (Dl + 70)); G.Stroke();
		// C: the closing sight line, off the sheet
		double Cl = C.Size();
		if (Cl == 0) Cl = 1;
		const double Ex = Cx + C.X / Cl * 400, Ey = Cy + C.Y / Cl * 400;
		G.SetLineDash({}); G.LineWidth = 3;
		G.Save(); G.BeginPath(); G.Rect(40, 120, 880, 470); G.Clip();
		G.BeginPath(); G.MoveTo(Cx, Cy); G.LineTo(Ex, Ey); G.Stroke();
		G.Restore();
		// B, the comparison marker
		G.BeginPath(); G.MoveTo(Cx, Cy); G.LineTo(Bx, By); G.Stroke();
		G.BeginPath(); G.MoveTo(Bx, By - 30); G.LineTo(Bx, By + 30); G.Stroke();
		// A in its square footing
		G.LineWidth = 2; G.StrokeRect(Ax - 34, Ay - 34, 68, 68); G.LineWidth = 3;
		Mark(G, Ax, Ay - 2, 40);
		G.SetFont(ELITE, 30);
		G.FillText(TEXT("A"), Ax + 42, Ay + 10); G.FillText(TEXT("B"), Bx - 12, By + 62);
		const double Lx = FMath::Min(890.0, FMath::Max(60.0, Cx + C.X / Cl * 300)), Ly = FMath::Min(570.0, FMath::Max(150.0, Cy + C.Y / Cl * 300));
		G.FillText(TEXT("C"), Lx + 6, Ly - 14);
		G.SetFont(ELITE, 17);
		G.FillText(TEXT("TRANSIT"), Cx + 26, Cy + 40);
		G.FillText(TEXT("footing"), Ax - 34, Ay + 56);
		// north arrow
		G.BeginPath(); G.MoveTo(880, 210); G.LineTo(880, 150); G.LineTo(870, 170); G.MoveTo(880, 150); G.LineTo(890, 170); G.Stroke();
		G.SetFont(ELITE, 22); G.FillText(TEXT("N"), 872, 236);
		G.SetFont(ELITE, 17);
		G.FillText(TEXT("FIELD COPY / KEEP WITH THE TRANSIT"), 40, 600);
	});
}

// ---------- All Night: SARO 07's service map and the tracing of E09A ----------

namespace
{
	const TArray<FVector2D>& ROAD()
	{
		static const TArray<FVector2D> R = { { 200, 560 }, { 330, 585 }, { 470, 600 }, { 610, 600 }, { 740, 575 }, { 850, 520 }, { 930, 440 }, { 985, 360 }, { 1030, 270 }, { 1080, 180 }, { 1140, 110 }, { 1190, 70 } };
		return R;
	}
	const FVector2D STATION(470, 360);
	constexpr double CROSS_MILES = 8.15;

	// distance along the road to a point on it, and the point at a distance along it (along)
	struct FAlong
	{
		TArray<FVector2D> Pts;
		TArray<double> Cum;
		double Total = 0;
		explicit FAlong(const TArray<FVector2D>& P) : Pts(P)
		{
			Cum.Add(0);
			for (int32 i = 1; i < Pts.Num(); ++i) Cum.Add(Cum[i - 1] + FVector2D::Distance(Pts[i], Pts[i - 1]));
			Total = Cum.Last();
		}
		FVector2D At(double D) const
		{
			for (int32 i = 1; i < Pts.Num(); ++i)
			{
				if (D <= Cum[i])
				{
					const double K = (D - Cum[i - 1]) / (Cum[i] - Cum[i - 1]);
					return Pts[i - 1] + (Pts[i] - Pts[i - 1]) * K;
				}
			}
			return Pts.Last();
		}
	};

	// where the line east from STATION 01 meets the road, as a distance along it (crossing)
	double Crossing()
	{
		const TArray<FVector2D>& R = ROAD();
		const FAlong A(R);
		for (int32 i = 1; i < R.Num(); ++i)
		{
			const double Ax = R[i - 1].X, Ay = R[i - 1].Y, Bx = R[i].X, By = R[i].Y;
			if ((Ay - STATION.Y) * (By - STATION.Y) <= 0 && Ay != By)
			{
				const double K = (STATION.Y - Ay) / (By - Ay), X = Ax + (Bx - Ax) * K;
				if (X > STATION.X) return A.Cum[i - 1] + FVector2D::Distance(R[i], R[i - 1]) * K;
			}
		}
		return A.Cum.Last();
	}

	// g.save(); g.translate(X, Y); g.rotate(-Math.PI / 2); g.fillText(text, 0, 0); g.restore(): the
	// line drawn on a canvas of its own and laid on G turned a quarter round, reading upwards (the
	// turn is exact on whole pixels: FS47Canvas has no transform of its own)
	void FillTextTurned(FS47Canvas& G, const TCHAR* Font, double Px, uint32 Rgb, const FString& Text, int32 X, int32 Y)
	{
		const int32 Tw = 16 + FMath::CeilToInt(Text.Len() * Px), Th = FMath::CeilToInt(3 * Px), Bx = 8, By = FMath::CeilToInt(2 * Px);
		FS47Canvas T(Tw, Th);
		T.SetFont(Font, Px);
		T.SetFill(Rgb);
		if (!T.FillText(Text, Bx, By)) return;
		// local (u, v) goes to (X + v, Y - u): the pixel (tx, ty) to (X + ty - By, Y - 1 - tx + Bx)
		TArray<FColor> R;
		R.Init(FColor(0, 0, 0, 0), Tw * Th);
		for (int32 Ty = 0; Ty < Th; ++Ty) for (int32 Tx = 0; Tx < Tw; ++Tx) R[(Tw - 1 - Tx) * Th + Ty] = T.Px[Ty * Tw + Tx];
		G.DrawImage(R, Th, Tw, X - By, Y - Tw + Bx);
	}

	void DrawServiceMap(FS47Canvas& G)
	{
		// drawServiceMap
		const S47Drawings::FServiceMap& Mp = S47Drawings::SERVICE_MAP();
		G.SetFill(S47Drawings::INK); G.SetStroke(S47Drawings::INK); // lineCap and lineJoin round (FS47Canvas strokes so)
		G.SetFont(OSWALD_600, 30); G.FillText(TEXT("SERVICE TRUCK / ROUTE MAP"), 40, 54);
		G.SetFont(ELITE, 19); G.FillText(TEXT("Access and old survey roads. Mileposts from the junction at the diner. BM: survey bolt in the shoulder."), 40, 86, 1120);
		// north arrow
		G.LineWidth = 3;
		G.BeginPath(); G.MoveTo(1130, 200); G.LineTo(1130, 140); G.LineTo(1120, 160); G.MoveTo(1130, 140); G.LineTo(1140, 160); G.Stroke();
		G.SetFont(ELITE, 24); G.FillText(TEXT("N"), 1121, 228);
		// the highway, north to south
		G.LineWidth = 9; G.SetStroke(0x3d4a44);
		G.BeginPath(); G.MoveTo(200, 110); G.LineTo(200, 780); G.Stroke();
		G.LineWidth = 2; G.SetStroke(S47Drawings::PAPER); G.SetLineDash({ 14, 12 });
		G.BeginPath(); G.MoveTo(200, 110); G.LineTo(200, 780); G.Stroke();
		G.SetLineDash({}); G.SetStroke(S47Drawings::INK);
		G.Save(); G.SetFont(ELITE, 18); FillTextTurned(G, ELITE, 18, S47Drawings::INK, TEXT("HIGHWAY"), 176, 760); G.Restore();
		// SARO and its array, Sierra Motor Court down the highway
		G.LineWidth = 2.5;
		for (int32 i = 0; i < 9; ++i) { G.BeginPath(); G.Arc(300 + (i % 3) * 30, 128 + (i / 3) * 22, 7, 0, PI * 2); G.Stroke(); }
		G.StrokeRect(286, 200, 52, 32);
		G.SetFont(OSWALD_600, 22); G.FillText(TEXT("SARO"), 348, 224);
		G.StrokeRect(96, 296, 66, 24);
		G.SetFont(ELITE, 17); G.FillText(TEXT("SIERRA MOTOR CT"), 40, 348);
		// the diner at the junction
		G.FillRect(150, 548, 34, 22);
		G.SetFont(OSWALD_600, 20); G.FillText(TEXT("MESA DINER"), 40, 600);
		// Caliche: its main street is the highway, south from the diner
		for (int32 i = 0; i < 4; ++i) { G.FillRect(162, 580 + i * 16, 22, 10); if (i) G.FillRect(216, 584 + i * 16, 18, 9); }
		G.SetFont(ELITE, 17); G.FillText(TEXT("CALICHE"), 40, 628);
		// the old Roswell road
		G.LineWidth = 6; G.SetStroke(0x4a5550);
		G.BeginPath();
		for (int32 i = 0; i < Mp.Road.Num(); ++i) { if (i) G.LineTo(Mp.Road[i].X, Mp.Road[i].Y); else G.MoveTo(Mp.Road[i].X, Mp.Road[i].Y); }
		G.Stroke();
		G.SetStroke(S47Drawings::INK);
		G.SetFont(ELITE, 18);
		G.FillText(TEXT("OLD ROAD TO ROSWELL"), 560, 632); // (g.save(); g.translate(560, 632); fillText at 0, 0; g.restore())
		// mileposts: a tick across the road and the number
		G.LineWidth = 2;
		for (int32 Mi = 1; Mi <= FMath::FloorToInt(Mp.Miles); ++Mi)
		{
			const FVector2D P = Mp.Post(Mi), P2 = Mp.Post(Mi + 0.02);
			const double Nx = -(P2.Y - P.Y), Ny = P2.X - P.X;
			double L = FMath::Sqrt(Nx * Nx + Ny * Ny);
			if (L == 0) L = 1;
			G.BeginPath(); G.MoveTo(P.X - Nx / L * 9, P.Y - Ny / L * 9); G.LineTo(P.X + Nx / L * 9, P.Y + Ny / L * 9); G.Stroke();
			G.SetFont(OSWALD_600, 19);
			G.FillText(FString::FromInt(Mi), P.X + Nx / L * 16 - 5, P.Y + Ny / L * 16 + 7);
		}
		// survey bolts on the shoulder: a small triangle with a dot
		for (const S47Drawings::FServiceMap::FBolt& B : Mp.Bolts)
		{
			const double X = B.At.X, Y = B.At.Y;
			G.BeginPath(); G.MoveTo(X - 9, Y - 13); G.LineTo(X + 9, Y - 13); G.LineTo(X, Y - 27); G.ClosePath(); G.Stroke();
			G.BeginPath(); G.Arc(X, Y - 18, 2.2, 0, PI * 2); G.Fill();
			G.SetFont(ELITE, 16); G.FillText(TEXT("BM"), X - 30, Y - 16);
			if (FMath::IsNearlyEqual(B.Mi, 10.4))
			{
				// The courier lies beyond C. This label never changes P13's crossing at 8.15.
				G.SetFont(OSWALD_600, 20); G.FillText(TEXT("COURIER 10.4"), X - 170, Y - 32);
				G.BeginPath(); G.MoveTo(X - 30, Y - 30); G.LineTo(X - 10, Y - 20); G.Stroke();
			}
		}
		// STATION 01 north of the road, with its fence
		const double Sx = Mp.Station.X, Sy = Mp.Station.Y;
		G.LineWidth = 2; G.SetLineDash({ 5, 5 }); G.StrokeRect(Sx - 30, Sy - 24, 60, 48); G.SetLineDash({});
		G.LineWidth = 3; S47Drawings::Mark(G, Sx, Sy + 2, 22);
		G.SetFont(OSWALD_600, 20); G.FillText(TEXT("STATION 01"), Sx - 52, Sy - 34);
		G.SetFont(ELITE, 16); G.FillText(TEXT("old survey station, 1947"), Sx - 82, Sy + 50);
		G.SetFont(ELITE, 16);
		G.FillText(TEXT("SERVICE TRUCK / KEEP IN THE DOOR POCKET"), 40, 780);
	}
}

FVector2D S47Drawings::FServiceMap::Post(double Mi) const
{
	static const FAlong A(ROAD());
	return A.At(Mi * PerMile);
}

const S47Drawings::FServiceMap& S47Drawings::SERVICE_MAP()
{
	static const FServiceMap M = []
	{
		FServiceMap S;
		const FAlong A(ROAD());
		const double Cross = Crossing();
		S.PerMile = Cross / CROSS_MILES;
		for (double Mi : { 3.3, 6.0, CROSS_MILES, 10.4 }) S.Bolts.Add({ Mi, A.At(Mi * S.PerMile) });
		S.W = 1200; S.H = 800; S.Station = STATION; S.Road = ROAD(); S.Miles = A.Total / S.PerMile; S.Snap = 18;
		return S;
	}();
	return M;
}

FString S47Drawings::ServiceMap()
{
	// serviceMap: sheet(SERVICE_MAP.w, SERVICE_MAP.h, drawServiceMap)
	return Sheet(SERVICE_MAP().W, SERVICE_MAP().H, [](FS47Canvas& G) { DrawServiceMap(G); });
}

void S47Drawings::ServiceMapCanvas(FS47Canvas& G)
{
	// serviceMapCanvas: the paper and the map, on the canvas it is given (SERVICE_MAP's size)
	PaperBg(G, G.W, G.H);
	DrawServiceMap(G);
}

void S47Drawings::DrawTracing(FS47Canvas& G, double X, double Y)
{
	// drawTracing
	const double W = 620, H = 300, Left = X - 70, Top = Y - H / 2;
	G.Save();
	G.SetFill(0xd6e2e8, 0.38); G.SetStroke(0x28466e, 0.55); G.LineWidth = 1.5; // rgba(214,226,232,.38), rgba(40,70,110,.55)
	G.FillRect(Left, Top, W, H); G.StrokeRect(Left, Top, W, H);
	G.SetStroke(0x1d3f7a); G.SetFill(0x1d3f7a); // lineCap 'round' (FS47Canvas strokes so)
	// the transit
	G.LineWidth = 2.5;
	G.BeginPath(); G.Arc(X, Y, 12, 0, PI * 2); G.Stroke();
	G.BeginPath(); G.MoveTo(X - 18, Y); G.LineTo(X + 18, Y); G.MoveTo(X, Y - 18); G.LineTo(X, Y + 18); G.Stroke();
	// C, the closing line, east to the edge of the sheet
	G.LineWidth = 3;
	G.BeginPath(); G.MoveTo(X + 12, Y); G.LineTo(Left + W - 8, Y); G.Stroke();
	G.SetFont(ELITE, 24); G.FillText(TEXT("C"), Left + W - 34, Y - 12);
	// A to the north on the sight line, B to the west
	G.LineWidth = 2; G.SetLineDash({ 6, 5 });
	G.BeginPath(); G.MoveTo(X, Y - 12); G.LineTo(X, Y - 60); G.Stroke(); G.SetLineDash({});
	G.SetFont(ELITE, 19); G.FillText(TEXT("A"), X + 8, Y - 52); G.FillText(TEXT("B"), X - 58, Y + 6);
	G.BeginPath(); G.MoveTo(X - 12, Y); G.LineTo(X - 44, Y); G.Stroke();
	// north arrow and the note
	G.BeginPath(); G.MoveTo(Left + W - 30, Top + 74); G.LineTo(Left + W - 30, Top + 30); G.LineTo(Left + W - 37, Top + 44); G.MoveTo(Left + W - 30, Top + 30); G.LineTo(Left + W - 23, Top + 44); G.Stroke();
	G.SetFont(ELITE, 15);
	G.FillText(TEXT("E09A, traced. Transit on the station."), Left + 12, Top + H - 14);
	G.Restore();
}

FString S47Drawings::Clipping(UTexture* Tex)
{
	// Chapter5.ts docClipping: the canvas the frame shows, as a JPEG. The texture is drawn by Slate
	// into a render target of its own size and read back; the export keeps a canvas as three uploads
	// it (flipY: its bottom row first), so the rows are turned back.
	if (!Tex || !FApp::CanEverRender() || !FSlateApplication::IsInitialized() || !FSlateApplication::Get().GetRenderer()) return FString();
	const int32 W = FMath::RoundToInt(Tex->GetSurfaceWidth()), H = FMath::RoundToInt(Tex->GetSurfaceHeight());
	if (W <= 0 || H <= 0) return FString();
	// every mip in memory first (the frame on the wall may have been streamed at a smaller one)
	if (UTexture2D* T2 = Cast<UTexture2D>(Tex)) { T2->SetForceMipLevelsToBeResident(30.f); T2->WaitForStreaming(); }
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>();
	RT->ClearColor = FLinearColor::Black;
	RT->InitCustomFormat(W, H, PF_B8G8R8A8, false);
	RT->UpdateResourceImmediate(true);
	FSlateBrush Brush;
	Brush.SetResourceObject(Tex);
	Brush.ImageSize = FVector2D(W, H);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	TSharedRef<SImage> Img = SNew(SImage).Image(&Brush);
	TArray<FColor> Px;
	// (no gamma correction: the sRGB target encodes what the sRGB texture decoded, so the canvas's own
	// values come back; with it, they came back a gamma lighter)
	FWidgetRenderer* R = new FWidgetRenderer(false);
	R->DrawWidget(RT, Img, FVector2D(W, H), 0.f, false);
	const bool bOk = S47Image::ReadPixels(RT, Px) && Px.Num() == W * H;
	BeginCleanup(R);
	RT->MarkAsGarbage();
	if (!bOk) return FString();
	TArray<FColor> Up;
	Up.SetNumUninitialized(W * H);
	for (int32 y = 0; y < H; ++y) FMemory::Memcpy(&Up[y * W], &Px[(H - 1 - y) * W], W * sizeof(FColor));
	for (FColor& C : Up) C.A = 255;
	return S47Image::ToDataUrl(Up, W, H, true, 85);
}

bool S47Drawings::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s drawings: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// the paper's speckle runs the same LCG as paperBg (values from node, a 120 x 120 sheet)
	{
		uint32 S = 120 * 31 + 120;
		auto R = [&S] { S = S * 1664525u + 1013904223u; return S / 4294967296.0; };
		const double Want[5] = { 0.724269293, 0.581025135, 0.099299545, 0.810711992, 0.614507199 };
		bool bSame = true;
		for (double V : Want) bSame &= FMath::Abs(R() - V) < 1e-8;
		Check(bSame, TEXT("paper speckle as paperBg"));
	}
	// Uint8ClampedArray stores (node: 0.5, 1.5, 2.5, -3, 254.5, 300 -> 0, 2, 2, 0, 254, 255)
	Check(FS47Canvas::Clamp8(0.5) == 0 && FS47Canvas::Clamp8(1.5) == 2 && FS47Canvas::Clamp8(2.5) == 2 && FS47Canvas::Clamp8(-3) == 0 && FS47Canvas::Clamp8(254.5) == 254 && FS47Canvas::Clamp8(300) == 255, TEXT("clamped bytes round half to even"));
	// a stroke: the mark's bar is ink in the middle and paper well off it
	{
		FS47Canvas G(120, 120);
		G.SetFill(PAPER); G.FillRect(0, 0, 120, 120);
		G.SetStroke(INK); G.LineWidth = 4;
		Mark(G, 60, 58, 70, true);
		const FColor On = G.Px[int32(58 + 70 * 0.62) * 120 + 60], Off = G.Px[5 * 120 + 5];
		Check(On.R == 0x22 && On.G == 0x37 && On.B == 0x2e && Off.R == 0xef && Off.G == 0xe8 && Off.B == 0xd6, FString::Printf(TEXT("the mark's bar in ink (%d %d %d), the paper clean (%d %d %d)"), On.R, On.G, On.B, Off.R, Off.G, Off.B));
	}
	// setLineDash([10, 8]): ink, a gap, ink again along a line; a clip keeps a stroke inside it
	{
		FS47Canvas G(40, 8);
		G.SetFill(0x000000); G.FillRect(0, 0, 40, 8);
		G.SetStroke(0xffffff); G.LineWidth = 2;
		G.SetLineDash({ 10, 8 });
		G.BeginPath(); G.MoveTo(0, 4); G.LineTo(40, 4); G.Stroke();
		const uint8 On1 = G.Px[3 * 40 + 5].R, Gap = G.Px[3 * 40 + 14].R, On2 = G.Px[3 * 40 + 22].R;
		G.SetLineDash({});
		G.Save(); G.BeginPath(); G.Rect(0, 0, 20, 8); G.Clip();
		G.SetStroke(0xff0000); G.BeginPath(); G.MoveTo(0, 1); G.LineTo(40, 1); G.Stroke();
		G.Restore();
		const uint8 In = G.Px[0 * 40 + 10].R, Out = G.Px[0 * 40 + 30].R;
		G.SetStroke(0x00ff00); G.LineWidth = 1; G.StrokeRect(30.5, 5.5, 6, 2);
		Check(On1 == 255 && Gap == 0 && On2 == 255 && In == 255 && Out == 0 && G.Px[5 * 40 + 33].G == 255 && G.Px[6 * 40 + 33].G == 0,
			FString::Printf(TEXT("dashes (%d %d %d), the clip (%d %d) and strokeRect"), On1, Gap, On2, In, Out));
	}
	// a half-covered pixel of a rectangle blends half way
	{
		FS47Canvas G(4, 4);
		G.SetFill(0x000000); G.FillRect(0, 0, 4, 4);
		G.SetFill(0xffffff); G.FillRect(1.5, 0, 2, 4);
		Check(G.Px[1].R == 128 && G.Px[2].R == 255 && G.Px[3].R == 128 && G.Px[0].R == 0, FString::Printf(TEXT("rectangle coverage (%d %d %d %d)"), G.Px[0].R, G.Px[1].R, G.Px[2].R, G.Px[3].R));
	}
	// the service map (node: perMile 107.82436653, miles 11.48696417, the bolts at 3.3, 6, 8.15 and
	// 10.4 miles, the one where C crosses at 985, 360; post(1) at 305.884, 580.362)
	{
		const FServiceMap& Mp = SERVICE_MAP();
		const FVector2D P1 = Mp.Post(1);
		Check(FMath::Abs(Mp.PerMile - 107.82436653) < 1e-6 && FMath::Abs(Mp.Miles - 11.48696417) < 1e-6 && Mp.Bolts.Num() == 4
			&& FVector2D::Distance(Mp.Bolts[2].At, FVector2D(985, 360)) < 1e-6 && FVector2D::Distance(Mp.Bolts[1].At, FVector2D(830.67781116, 529.66109442)) < 1e-6
			&& FVector2D::Distance(P1, FVector2D(305.88422308, 580.36235059)) < 1e-6 && Mp.Station == FVector2D(470, 360) && Mp.Snap == 18,
			FString::Printf(TEXT("SERVICE_MAP: %.4f a mile, %.3f miles, C crosses at %.1f, %.1f"), Mp.PerMile, Mp.Miles, Mp.Bolts[2].At.X, Mp.Bolts[2].At.Y));
	}
	// the tracing: onion-skin over the paper (rgba(214,226,232,.38) over #efe8d6), C in blue along the transit's row
	{
		FS47Canvas G(800, 400);
		G.SetFill(PAPER); G.FillRect(0, 0, 800, 400);
		DrawTracing(G, 100, 200);
		const FColor Skin = G.Px[300 * 800 + 300], Line = G.Px[200 * 800 + 300], Off = G.Px[20 * 800 + 20];
		const auto Over = [](uint8 S, uint8 D) { return FS47Canvas::Clamp8(S * (97 / 255.0) + D * (1 - 97 / 255.0)); };
		Check(Skin.R == Over(214, 0xef) && Skin.G == Over(226, 0xe8) && Skin.B == Over(232, 0xd6) && Line.R == 0x1d && Line.G == 0x3f && Line.B == 0x7a && Off.R == 0xef,
			FString::Printf(TEXT("the tracing over the map (%d %d %d), C (%d %d %d)"), Skin.R, Skin.G, Skin.B, Line.R, Line.G, Line.B));
	}
	return bOk;
}
