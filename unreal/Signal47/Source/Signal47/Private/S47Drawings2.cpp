#include "S47Drawings2.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "S47Image.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/SLeafWidget.h"

using S47Drawings2::FCanvas2D;

namespace
{
	const uint32 PAPER = 0xefe8d6, INK = 0x22372e;

	FLinearColor Rgba(uint32 Rgb, double A)
	{
		// the canvas's colours are sRGB: Slate packs a vertex colour as these bytes, and without its
		// gamma step (FWidgetRenderer(false)) they reach the render target as they are, blended as
		// the canvas blends, on the encoded values
		FLinearColor C = FLinearColor::FromSRGBColor(FColor((Rgb >> 16) & 255, (Rgb >> 8) & 255, Rgb & 255));
		C.A = float(FMath::Clamp(A, 0.0, 1.0));
		return C;
	}

	// The canvas's fonts, without hinting: the canvas lays text out on unhinted advances, and hinted
	// ones (S47Style's) round each letter to the pixel and run long over a line.
	FSlateFontInfo SheetFont(const TCHAR* File, double Px)
	{
		static TMap<FString, TSharedPtr<FStandaloneCompositeFont>> Cache;
		TSharedPtr<FStandaloneCompositeFont>& F = Cache.FindOrAdd(File);
		if (!F.IsValid())
		{
			F = MakeShared<FStandaloneCompositeFont>();
			F->DefaultTypeface.AppendFont(TEXT("Regular"), FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Fonts") / File), EFontHinting::None, EFontLoadingPolicy::LazyLoad);
		}
		return FSlateFontInfo(F, float(Px * 0.75), TEXT("Regular")); // CSS pixels to Slate's points at 96 dpi
	}

	// The sheet is drawn at twice its size and averaged down: Slate's antialiased lines fade out
	// over two pixels where the canvas's fade over one, and halved they do the same.
	constexpr int32 Over = 2;

	// A sheet as a Slate widget: the recorded calls in order, one layer each so that Slate keeps
	// their order (it batches within a layer), everything Over times its size.
	class SS47Sheet : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SS47Sheet) {}
			SLATE_ARGUMENT(const FCanvas2D*, Canvas)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Canvas = InArgs._Canvas;
			White = MakeUnique<FSlateColorBrush>(FLinearColor::White);
		}
		virtual FVector2D ComputeDesiredSize(float) const override { return Canvas ? FVector2D(Canvas->W * Over, Canvas->H * Over) : FVector2D(1, 1); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle&, bool) const override
		{
			if (!Canvas) return LayerId;
			const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			int32 L = LayerId;
			for (const FCanvas2D::FCmd& C : Canvas->Cmds)
			{
				switch (C.Kind)
				{
				case FCanvas2D::FCmd::EKind::Rect:
					FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(C.Size * Over, FSlateLayoutTransform(C.At * Over)), White.Get(), ESlateDrawEffect::None, C.Color);
					break;
				case FCanvas2D::FCmd::EKind::Lines:
				{
					TArray<FVector2f> P = C.Points;
					for (FVector2f& V : P) V *= Over;
					FSlateDrawElement::MakeLines(Out, L, G.ToPaintGeometry(), MoveTemp(P), ESlateDrawEffect::None, C.Color, true, C.Width * Over);
					break;
				}
				case FCanvas2D::FCmd::EKind::Dot:
				{
					// lineCap and lineJoin 'round': a disc the line's width across
					const float D = C.Size.X * Over;
					TSharedPtr<FSlateBrush>& B = Discs.FindOrAdd(D);
					if (!B.IsValid()) B = MakeShared<FSlateRoundedBoxBrush>(FLinearColor::White, D * 0.5f);
					FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(FVector2f(D, D), FSlateLayoutTransform(C.At * Over - FVector2f(D * 0.5f, D * 0.5f))), B.Get(), ESlateDrawEffect::None, C.Color);
					break;
				}
				case FCanvas2D::FCmd::EKind::Text:
				{
					// fillText puts the alphabetic baseline at y; Slate puts the top of the line box
					// there, with the baseline at its height plus the (negative) descender
					FSlateFontInfo F = C.Font;
					F.Size *= Over;
					const float LineH = Measure->GetMaxCharacterHeight(F);
					const float Top = C.At.Y * Over - (LineH + Measure->GetBaseline(F));
					FSlateDrawElement::MakeText(Out, L, G.ToPaintGeometry(FVector2f(float(Canvas->W * Over) * 2.f, LineH * 2.f), FSlateLayoutTransform(FVector2f(C.At.X * Over, Top))), C.Text, F, ESlateDrawEffect::None, C.Color);
					break;
				}
				default:
					break;
				}
				++L;
			}
			return L;
		}

	private:
		const FCanvas2D* Canvas = nullptr;
		TUniquePtr<FSlateBrush> White;
		mutable TMap<float, TSharedPtr<FSlateBrush>> Discs;
	};

	// The canvas painted into a render target Over times its size, read back and averaged down to
	// the sheet's own size (top row first). The average is taken on the stored values, as the
	// canvas blends.
	bool Render(const FCanvas2D& G, TArray<FColor>& Px)
	{
		if (!FApp::CanEverRender() || !FSlateApplication::IsInitialized() || !FSlateApplication::Get().GetRenderer()) return false;
		const int32 BW = G.W * Over, BH = G.H * Over;
		UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>();
		RT->ClearColor = FLinearColor::Transparent;
		RT->InitCustomFormat(BW, BH, PF_B8G8R8A8, false);
		RT->UpdateResourceImmediate(true);
		TSharedRef<SS47Sheet> Sheet = SNew(SS47Sheet).Canvas(&G);
		FWidgetRenderer* R = new FWidgetRenderer(false);
		R->DrawWidget(RT, Sheet, FVector2D(BW, BH), 0.f, false);
		TArray<FColor> Big;
		const bool bOk = S47Image::ReadPixels(RT, Big) && Big.Num() == BW * BH;
		BeginCleanup(R);
		RT->MarkAsGarbage();
		if (!bOk) return false;
		Px.SetNumUninitialized(G.W * G.H);
		for (int32 y = 0; y < G.H; ++y) for (int32 x = 0; x < G.W; ++x)
		{
			int32 Sum[3] = { 0, 0, 0 };
			for (int32 j = 0; j < Over; ++j) for (int32 i = 0; i < Over; ++i)
			{
				const FColor& C = Big[(y * Over + j) * BW + x * Over + i];
				Sum[0] += C.R; Sum[1] += C.G; Sum[2] += C.B;
			}
			// the paper covers the sheet (canvas.toDataURL keeps it opaque)
			Px[y * G.W + x] = FColor(uint8((Sum[0] + Over * Over / 2) / (Over * Over)), uint8((Sum[1] + Over * Over / 2) / (Over * Over)), uint8((Sum[2] + Over * Over / 2) / (Over * Over)), 255);
		}
		return true;
	}

	FString RenderUrl(const FCanvas2D& G)
	{
		TArray<FColor> Px;
		if (!Render(G, Px)) return FString();
		return S47Image::ToDataUrl(Px, G.W, G.H, false);
	}
}

// ---------- the canvas ----------

FCanvas2D::FSub& FCanvas2D::Current()
{
	if (Subs.Num() == 0) Subs.AddDefaulted();
	return Subs.Last();
}

void FCanvas2D::BeginPath() { Subs.Reset(); }

void FCanvas2D::MoveTo(double X, double Y)
{
	FSub& S = Subs.AddDefaulted_GetRef();
	S.Pts.Add(FVector2D(X, Y));
	S.Corner.Add(true);
}

void FCanvas2D::LineTo(double X, double Y)
{
	// (lineTo with no subpath is a moveTo)
	if (Subs.Num() == 0 || Subs.Last().Pts.Num() == 0) { MoveTo(X, Y); return; }
	FSub& S = Current();
	S.Pts.Add(FVector2D(X, Y));
	S.Corner.Add(true);
}

void FCanvas2D::ClosePath()
{
	if (Subs.Num() == 0 || Subs.Last().Pts.Num() == 0) return;
	Subs.Last().bClosed = true;
	// a new subpath begins where the closed one began
	const FVector2D Start = Subs.Last().Pts[0];
	MoveTo(Start.X, Start.Y);
}

void FCanvas2D::Arc(double Cx, double Cy, double R, double A0, double A1)
{
	// arc(x, y, r, a0, a1), clockwise: a line from the current point to its start, then the arc
	const double Sweep = FMath::Min(A1 - A0, 2.0 * PI);
	const bool bFull = Sweep >= 2.0 * PI - 1e-9;
	const FVector2D Start(Cx + R * FMath::Cos(A0), Cy + R * FMath::Sin(A0));
	const bool bFresh = Subs.Num() == 0 || Subs.Last().Pts.Num() == 0;
	if (bFresh) MoveTo(Start.X, Start.Y);
	else LineTo(Start.X, Start.Y);
	FSub& S = Current();
	const int32 N = FMath::Clamp(FMath::CeilToInt32(FMath::Abs(Sweep) * R / 2.0), 12, 160);
	for (int32 i = 1; i <= N; ++i)
	{
		const double A = A0 + Sweep * i / N;
		S.Pts.Add(FVector2D(Cx + R * FMath::Cos(A), Cy + R * FMath::Sin(A)));
		S.Corner.Add(false);
	}
	// a whole circle on its own is a closed ring
	if (bFull && bFresh) { S.Pts.Pop(); S.Corner.Pop(); S.Corner[0] = false; S.bClosed = true; }
}

void FCanvas2D::BezierCurveTo(double C1x, double C1y, double C2x, double C2y, double X, double Y)
{
	if (Subs.Num() == 0 || Subs.Last().Pts.Num() == 0) MoveTo(C1x, C1y);
	FSub& S = Current();
	const FVector2D P0 = S.Pts.Last(), P1(C1x, C1y), P2(C2x, C2y), P3(X, Y);
	const int32 N = 48;
	for (int32 i = 1; i <= N; ++i)
	{
		const double T = double(i) / N, U = 1.0 - T;
		S.Pts.Add(P0 * (U * U * U) + P1 * (3 * U * U * T) + P2 * (3 * U * T * T) + P3 * (T * T * T));
		S.Corner.Add(i == N);
	}
}

void FCanvas2D::Stroke()
{
	for (const FSub& S : Subs) StrokeSub(S);
}

void FCanvas2D::StrokeSub(const FSub& S)
{
	if (S.Pts.Num() < 2) return;
	const FLinearColor C = Rgba(LineRgb, LineA * GlobalAlpha);
	// the dash pattern (an odd one is doubled, as the canvas does), from the start of each subpath
	TArray<double> Dash = LineDash;
	bool bAnyOn = false;
	for (double D : Dash) if (D > 0) bAnyOn = true;
	if (Dash.Num() == 0 || !bAnyOn) { EmitLine(S.Pts, S.Corner, S.bClosed, C); return; }
	if (Dash.Num() % 2) Dash.Append(LineDash);
	TArray<FVector2D> Path = S.Pts;
	if (S.bClosed) Path.Add(S.Pts[0]);
	int32 K = 0;
	double Left = Dash[0];
	bool bOn = true;
	TArray<FVector2D> Piece;
	TArray<bool> PieceCorner;
	if (bOn) { Piece.Add(Path[0]); PieceCorner.Add(false); }
	for (int32 i = 1; i < Path.Num(); ++i)
	{
		FVector2D A = Path[i - 1];
		const FVector2D B = Path[i];
		double Seg = FVector2D::Distance(A, B);
		while (Seg > 0)
		{
			if (Seg < Left)
			{
				Left -= Seg; Seg = 0;
				if (bOn) { Piece.Add(B); PieceCorner.Add(false); }
				break;
			}
			// the dash (or gap) ends inside this segment
			const FVector2D P = A + (B - A) * (Left / Seg);
			Seg -= Left;
			A = P;
			if (bOn)
			{
				Piece.Add(P); PieceCorner.Add(false);
				if (Piece.Num() >= 2) EmitLine(Piece, PieceCorner, false, C);
				Piece.Reset(); PieceCorner.Reset();
			}
			else { Piece.Add(P); PieceCorner.Add(false); }
			bOn = !bOn;
			K = (K + 1) % Dash.Num();
			Left = Dash[K];
		}
	}
	if (bOn && Piece.Num() >= 2) EmitLine(Piece, PieceCorner, false, C);
}

void FCanvas2D::EmitLine(const TArray<FVector2D>& Pts, const TArray<bool>& Corner, bool bClosed, const FLinearColor& C)
{
	FCmd Cmd;
	Cmd.Kind = FCmd::EKind::Lines;
	Cmd.Color = C;
	Cmd.Width = float(LineWidth);
	for (const FVector2D& P : Pts) Cmd.Points.Add(FVector2f(P));
	// a closed ring comes back to its start, and one segment further where its ink is opaque, so
	// the two ends of the line do not leave a seam
	if (bClosed) { Cmd.Points.Add(FVector2f(Pts[0])); if (C.A >= 0.999f && Pts.Num() > 1) Cmd.Points.Add(FVector2f(Pts[1])); }
	Cmds.Add(Cmd);
	// lineCap and lineJoin 'round' (sheet()): discs at the ends and the corners. Not under a see-
	// through ink, where they would show as darker spots.
	if (C.A < 0.999f) return;
	auto Disc = [&](const FVector2D& P)
	{
		FCmd D;
		D.Kind = FCmd::EKind::Dot;
		D.Color = C;
		D.At = FVector2f(P);
		D.Size = FVector2f(float(LineWidth), float(LineWidth));
		Cmds.Add(D);
	};
	for (int32 i = 0; i < Pts.Num(); ++i)
	{
		const bool bEnd = !bClosed && (i == 0 || i == Pts.Num() - 1);
		if (bEnd || (Corner.IsValidIndex(i) && Corner[i])) Disc(Pts[i]);
	}
}

void FCanvas2D::FillRect(double X, double Y, double Rw, double Rh)
{
	FCmd C;
	C.Kind = FCmd::EKind::Rect;
	C.Color = Rgba(FillRgb, FillA * GlobalAlpha);
	C.At = FVector2f(float(X), float(Y));
	C.Size = FVector2f(float(Rw), float(Rh));
	Cmds.Add(C);
}

void FCanvas2D::StrokeRect(double X, double Y, double Rw, double Rh)
{
	// its own closed path; the current one is left as it is
	FSub S;
	S.Pts = { FVector2D(X, Y), FVector2D(X + Rw, Y), FVector2D(X + Rw, Y + Rh), FVector2D(X, Y + Rh) };
	S.Corner = { true, true, true, true };
	S.bClosed = true;
	StrokeSub(S);
}

void FCanvas2D::FillText(const FString& S, double X, double Y)
{
	FCmd C;
	C.Kind = FCmd::EKind::Text;
	C.Color = Rgba(FillRgb, FillA * GlobalAlpha);
	C.At = FVector2f(float(X), float(Y));
	C.Text = S;
	// Oswald and Special Elite from Content/Fonts. The web version's Oswald is one static face,
	// Medium (500), declared for the weights 200 to 700 (core/fonts.ts), so the page draws every
	// Oswald in it, the titles' 600 too. Content/Fonts has Oswald as a variable font, which Slate
	// draws at its default 400: Oswald-Medium.ttf is its 500 instance (fontTools varLib.instancer).
	C.Font = SheetFont(FontFamily == TEXT("Oswald") ? TEXT("Oswald-Medium.ttf") : TEXT("SpecialElite-Regular.ttf"), FontPx);
	Cmds.Add(C);
}

// ---------- drawings.ts ----------

void S47Drawings2::PaperBg(FCanvas2D& G, int32 W, int32 H)
{
	// paperBg: the paper, with a little age on it
	G.FillStyle(PAPER); G.FillRect(0, 0, W, H);
	uint32 S = uint32(W * 31 + H);
	// (s * 1664525 + 1013904223) >>> 0: exact in a double below 2^53, so the same as uint32 wrapping
	auto R = [&S]() { S = S * 1664525u + 1013904223u; return double(S) / 4294967296.0; };
	for (int32 i = 0; i < 900; ++i)
	{
		G.FillStyle(0x5a4628, R() * 0.05);   // rgba(90,70,40,a)
		const double X = R() * W, Y = R() * H, Rw = 1 + R() * 2, Rh = 1 + R() * 2;
		G.FillRect(X, Y, Rw, Rh);
	}
}

FString S47Drawings2::Sheet(int32 W, int32 H, TFunctionRef<void(FCanvas2D&)> Draw)
{
	FCanvas2D G(W, H);
	PaperBg(G, W, H);
	G.StrokeStyle(INK); G.FillStyle(INK); G.LineWidth = 3; // lineCap, lineJoin 'round' (EmitLine)
	Draw(G);
	return RenderUrl(G);
}

void S47Drawings2::Mark(FCanvas2D& G, double Cx, double Cy, double Size, bool bBar)
{
	G.BeginPath();
	G.MoveTo(Cx, Cy - Size * 0.58); G.LineTo(Cx - Size * 0.55, Cy + Size * 0.4); G.LineTo(Cx + Size * 0.55, Cy + Size * 0.4); G.ClosePath();
	G.Stroke();
	if (bBar) { G.BeginPath(); G.MoveTo(Cx - Size * 0.4, Cy + Size * 0.62); G.LineTo(Cx + Size * 0.4, Cy + Size * 0.62); G.Stroke(); }
}

FString S47Drawings2::Arrangement(bool bFull)
{
	return Sheet(960, 420, [bFull](FCanvas2D& G)
	{
		G.Font(TEXT("Oswald"), 26, true);
		G.FillText(bFull ? TEXT("ARRANGEMENT / STATION 01 / 1947") : TEXT("ARRANGEMENT / STATION 01 / 1947 / SERVICE COPY"), 40, 56);
		G.Font(TEXT("Special Elite"), 19);
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
			G.BeginPath(); G.Arc(820, 250, 28, 0, PI * 2); G.Stroke();          // C
		}
		G.Font(TEXT("Special Elite"), 28);
		G.FillText(TEXT("A"), 140, 340); G.FillText(TEXT("B"), 490, 340); if (bFull) G.FillText(TEXT("C"), 810, 340);
		G.Font(TEXT("Special Elite"), 17);
		G.FillText(TEXT("N. VEGA"), 790, 396);
	});
}

FString S47Drawings2::SleeveMark()
{
	return Sheet(420, 420, [](FCanvas2D& G)
	{
		G.Font(TEXT("Oswald"), 24, true);
		G.FillText(TEXT("FIXED-POINT SHEET"), 34, 54);
		G.Font(TEXT("Special Elite"), 18);
		G.FillText(TEXT("INDEX DIAGRAM"), 34, 84);
		G.LineWidth = 4;
		Mark(G, 210, 220, 120);
		G.Font(TEXT("Special Elite"), 24);
		G.FillText(TEXT("A  -  B  -  C"), 132, 340);
		G.Font(TEXT("Special Elite"), 18);
		G.FillText(TEXT("STATION 01 / B-12"), 128, 376);
	});
}

FString S47Drawings2::MarkIcon(const FString& Kind)
{
	return Sheet(120, 120, [&Kind](FCanvas2D& G)
	{
		G.LineWidth = 4;
		if (Kind == TEXT("three-bars")) { for (int32 i = 0; i < 3; ++i) { G.BeginPath(); G.MoveTo(26, 38 + i * 22); G.LineTo(94, 38 + i * 22); G.Stroke(); } }
		else Mark(G, 60, 58, 70, Kind == TEXT("triangle-bar"));
	});
}

FString S47Drawings2::CaseMap(bool bStation, bool bMotel)
{
	return Sheet(900, 640, [bStation, bMotel](FCanvas2D& G)
	{
		G.Font(TEXT("Oswald"), 26, true);
		G.FillText(TEXT("CASE MAP / SARO AND FIELD SITES"), 40, 54);
		G.Font(TEXT("Special Elite"), 17);
		G.FillText(TEXT("Sketch from the site register. Not to scale."), 40, 84);
		// north arrow
		G.LineWidth = 3;
		G.BeginPath(); G.MoveTo(830, 150); G.LineTo(830, 100); G.LineTo(820, 118); G.MoveTo(830, 100); G.LineTo(840, 118); G.Stroke();
		G.Font(TEXT("Special Elite"), 22); G.FillText(TEXT("N"), 822, 176);
		// SARO: the array and the buildings
		const double Sx = 520, Sy = 330;
		for (int32 i = 0; i < 9; ++i) { G.BeginPath(); G.Arc(Sx - 90 + (i % 3) * 45, Sy - 120 + (i / 3) * 34, 9, 0, PI * 2); G.Stroke(); }
		G.StrokeRect(Sx - 70, Sy, 70, 46);
		G.StrokeRect(Sx + 20, Sy - 14, 46, 40);
		G.Font(TEXT("Oswald"), 22, true); G.FillText(TEXT("SARO"), Sx - 70, Sy + 82);
		G.Font(TEXT("Special Elite"), 16); G.FillText(TEXT("control room, yard, records"), Sx - 70, Sy + 106);
		G.SetLineDash({ 12, 10 }); G.LineWidth = 2.5;
		if (bStation)
		{
			const double Tx = 170, Ty = 470;
			G.BeginPath(); G.MoveTo(Sx - 80, Sy + 40); G.BezierCurveTo(400, 420, 300, 470, Tx + 40, Ty); G.Stroke();
			G.SetLineDash({}); G.LineWidth = 3;
			Mark(G, Tx, Ty, 46);
			G.Font(TEXT("Oswald"), 20, true); G.FillText(TEXT("OLD SURVEY STATION"), Tx - 70, Ty + 62);
			G.Font(TEXT("Special Elite"), 16); G.FillText(TEXT("STATION 01 / 1947"), Tx - 70, Ty + 86);
			G.SetLineDash({ 12, 10 }); G.LineWidth = 2.5;
		}
		if (bMotel)
		{
			// down the highway south of SARO
			const double Mx = 400, My = 560;
			G.BeginPath(); G.MoveTo(Sx - 30, Sy + 50); G.BezierCurveTo(490, 440, 440, 480, Mx + 20, My - 22); G.Stroke();
			G.SetLineDash({}); G.LineWidth = 3;
			G.StrokeRect(Mx - 50, My - 22, 110, 40);
			G.Font(TEXT("Oswald"), 20, true); G.FillText(TEXT("SIERRA MOTOR COURT"), Mx - 60, My - 40);
			G.Font(TEXT("Special Elite"), 16); G.FillText(TEXT("key holder: N. Vega"), Mx - 50, My + 46);
		}
		G.SetLineDash({});
	});
}

// ---------- self test ----------

bool S47Drawings2::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s drawings: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// the paper's first noise speck, as drawings.ts makes it for a 960 x 420 sheet (node:
	// s = 960 * 31 + 420 = 30180; r() four times after the alpha)
	FCanvas2D P(960, 420);
	PaperBg(P, 960, 420);
	Check(P.Cmds.Num() == 901, TEXT("the paper and its 900 specks"));
	{
		uint32 S = 30180u;
		auto R = [&S]() { S = S * 1664525u + 1013904223u; return double(S) / 4294967296.0; };
		const double A = R() * 0.05, X = R() * 960;
		Check(FMath::IsNearlyEqual(double(P.Cmds[1].At.X), X, 0.001) && FMath::IsNearlyEqual(double(P.Cmds[1].Color.A), A, 1e-6), TEXT("the specks follow the LCG"));
		// node: (30180 * 1664525 + 1013904223) >>> 0 is 4004628467
		Check(uint32(30180u * 1664525u + 1013904223u) == 4004628467u, TEXT("uint32 wrapping equals (s * 1664525 + 1013904223) >>> 0"));
	}
	// dashes: a 100 px line with [12, 10] gives five dashes, the last 12 long (88 to 100)
	{
		FCanvas2D D(120, 20);
		D.LineWidth = 1; D.StrokeStyle(INK, 0.5); D.SetLineDash({ 12, 10 });
		D.BeginPath(); D.MoveTo(0, 10); D.LineTo(100, 10); D.Stroke();
		int32 N = 0; float LastEnd = 0;
		for (const FCanvas2D::FCmd& C : D.Cmds) if (C.Kind == FCanvas2D::FCmd::EKind::Lines) { ++N; LastEnd = C.Points.Last().X; }
		Check(N == 5 && FMath::IsNearlyEqual(LastEnd, 100.f, 0.01f), FString::Printf(TEXT("dashes (%d, last ends at %.1f)"), N, LastEnd));
	}
	// rendered: the paper colour and the ink of the line from A to B
	FCanvas2D G(960, 420);
	G.FillStyle(PAPER); G.FillRect(0, 0, 960, 420);
	G.StrokeStyle(INK); G.LineWidth = 3;
	G.BeginPath(); G.MoveTo(196, 250); G.LineTo(470, 250); G.Stroke();
	TArray<FColor> Px;
	if (!Render(G, Px)) { Lines.Add(TEXT("ok  drawings: nothing renders here (no Slate renderer); the pictures are not checked")); return bOk; }
	const FColor Paper = Px[10 * 960 + 10], Ink = Px[250 * 960 + 300];
	auto Near = [](const FColor& C, uint32 Rgb, int32 Tol) { return FMath::Abs(C.R - int32((Rgb >> 16) & 255)) <= Tol && FMath::Abs(C.G - int32((Rgb >> 8) & 255)) <= Tol && FMath::Abs(C.B - int32(Rgb & 255)) <= Tol; };
	Check(Near(Paper, PAPER, 2), FString::Printf(TEXT("paper #efe8d6 comes out as #%02x%02x%02x"), Paper.R, Paper.G, Paper.B));
	Check(Near(Ink, INK, 3), FString::Printf(TEXT("ink #22372e comes out as #%02x%02x%02x"), Ink.R, Ink.G, Ink.B));
	const FString Url = Arrangement(true);
	UTexture2D* T = S47Image::FromDataUrl(Url);
	Check(Url.StartsWith(TEXT("data:image/png;base64,")) && T && T->GetSizeX() == 960 && T->GetSizeY() == 420, TEXT("E07's arrangement drawing is a 960 x 420 PNG"));
	const FString Map = CaseMap(true, true);
	UTexture2D* M = S47Image::FromDataUrl(Map);
	Check(M && M->GetSizeX() == 900 && M->GetSizeY() == 640 && Map != CaseMap(false, false), TEXT("the case map is 900 x 640, and grows with P05 and the call"));
	return bOk;
}
