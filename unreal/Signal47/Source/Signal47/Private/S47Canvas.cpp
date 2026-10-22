#include "S47Canvas.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Misc/App.h"
#include "Rendering/SlateRenderer.h"
#include "S47Image.h"
#include "Slate/WidgetRenderer.h"
#include "SS47Widgets.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SLeafWidget.h"

// One line of white text where FS47Canvas::FillText asks for it: its coverage is all that is read.
class SS47TextInk : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47TextInk) {}
	SLATE_END_ARGS()
	void Construct(const FArguments&) {}
	FString Text;
	FSlateFontInfo Font;
	FVector2f At = FVector2f::ZeroVector;
	FVector2D Size = FVector2D(1, 1);
	virtual FVector2D ComputeDesiredSize(float) const override { return Size; }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
	{
		FSlateDrawElement::MakeText(Out, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(At)), Text, Font, ESlateDrawEffect::None, FLinearColor::White);
		return Layer + 1;
	}
};

namespace
{
	FColor Rgb(uint32 C) { return FColor((C >> 16) & 255, (C >> 8) & 255, C & 255, 255); }

	double SegDist(const FVector2D& P, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D D = B - A;
		const double L2 = D.SizeSquared();
		const double T = L2 > 0 ? FMath::Clamp(FVector2D::DotProduct(P - A, D) / L2, 0.0, 1.0) : 0.0;
		return FVector2D::Distance(P, A + D * T);
	}
}

FS47Canvas::FS47Canvas(int32 InW, int32 InH) : W(InW), H(InH)
{
	// a new canvas is transparent black
	Px.Init(FColor(0, 0, 0, 0), FMath::Max(0, W * H));
}

uint8 FS47Canvas::Clamp8(double V)
{
	// ToUint8Clamp: NaN and anything below zero is 0, above 255 is 255, halves go to the even one
	if (!(V > 0)) return 0;
	if (V >= 255) return 255;
	const double F = FMath::FloorToDouble(V);
	const double R = V - F;
	if (R < 0.5) return uint8(F);
	if (R > 0.5) return uint8(F + 1);
	return (int32(F) % 2 == 0) ? uint8(F) : uint8(F + 1);
}

void FS47Canvas::SetFill(uint32 C, double A) { FillCol = Rgb(C); FillA = FMath::RoundToDouble(FMath::Clamp(A, 0.0, 1.0) * 255.0) / 255.0; }
void FS47Canvas::SetStroke(uint32 C, double A) { StrokeCol = Rgb(C); StrokeA = FMath::RoundToDouble(FMath::Clamp(A, 0.0, 1.0) * 255.0) / 255.0; }
void FS47Canvas::SetFont(const TCHAR* File, double SizePx) { FontFile = File; FontPx = SizePx; }

void FS47Canvas::Blend(int32 X, int32 Y, const FColor& C, double A)
{
	// source-over, not premultiplied, in the canvas's own 8-bit sRGB values (as a browser blends)
	if (X < 0 || Y < 0 || X >= W || Y >= H || A <= 0) return;
	if (ClipBox && !ClipBox->IsInside(FVector2D(X + 0.5, Y + 0.5))) return;
	A = FMath::Min(1.0, A);
	FColor& D = Px[Y * W + X];
	const double Da = D.A / 255.0;
	const double Oa = A + Da * (1 - A);
	if (Oa <= 0) return;
	auto Ch = [&](uint8 S, uint8 Dc) { return Clamp8((S * A + Dc * Da * (1 - A)) / Oa); };
	D = FColor(Ch(C.R, D.R), Ch(C.G, D.G), Ch(C.B, D.B), Clamp8(Oa * 255.0));
}

// ---------- paths ----------

void FS47Canvas::BeginPath() { Path.Reset(); }

void FS47Canvas::MoveTo(double X, double Y)
{
	FSub& S = Path.AddDefaulted_GetRef();
	S.P.Add(FVector2D(X, Y));
}

void FS47Canvas::LineTo(double X, double Y)
{
	if (Path.Num() == 0 || Path.Last().bClosed) { MoveTo(X, Y); return; }
	Path.Last().P.Add(FVector2D(X, Y));
}

void FS47Canvas::ClosePath()
{
	if (Path.Num() == 0) return;
	Path.Last().bClosed = true;
	// a path after closePath starts where that one began
	const FVector2D Start = Path.Last().P[0];
	FSub& S = Path.AddDefaulted_GetRef();
	S.P.Add(Start);
}

void FS47Canvas::Arc(double Cx, double Cy, double R, double A0, double A1)
{
	// arc(x, y, r, a0, a1) clockwise on the screen: a line from the current point to its start
	const double Span = A1 - A0;
	const int32 N = FMath::Max(16, FMath::CeilToInt(FMath::Abs(Span) * R / 1.5));
	for (int32 k = 0; k <= N; ++k)
	{
		const double A = A0 + Span * k / N;
		LineTo(Cx + FMath::Cos(A) * R, Cy + FMath::Sin(A) * R);
	}
}

void FS47Canvas::Stroke()
{
	// round caps and joins: the coverage of a pixel is how far its centre is inside half the
	// width of the nearest segment (one pass for the whole path, so joins are not drawn twice)
	TArray<TPair<FVector2D, FVector2D>> Segs;
	// the dash pattern (an odd list is repeated to make it even), in canvas pixels along each subpath
	TArray<double> Dash = LineDash;
	if (Dash.Num() % 2) Dash.Append(LineDash);
	double DashSum = 0;
	for (double D : Dash) DashSum += FMath::Max(0.0, D);
	const bool bDashed = Dash.Num() > 0 && DashSum > 0;
	for (const FSub& S : Path)
	{
		TArray<TPair<FVector2D, FVector2D>> Sub;
		for (int32 i = 1; i < S.P.Num(); ++i) Sub.Add({ S.P[i - 1], S.P[i] });
		if (S.bClosed && S.P.Num() > 2) Sub.Add({ S.P.Last(), S.P[0] });
		if (!bDashed) { Segs.Append(Sub); continue; }
		int32 Di = 0;
		double Left = Dash[0];
		bool bOn = true;
		for (const auto& Sg : Sub)
		{
			const FVector2D D = Sg.Value - Sg.Key;
			const double L = D.Size();
			if (L <= 0) continue;
			const FVector2D U = D / L;
			double At = 0;
			while (At < L)
			{
				const double Step = FMath::Min(Left, L - At);
				if (bOn && Step > 0) Segs.Add({ Sg.Key + U * At, Sg.Key + U * (At + Step) });
				At += Step;
				Left -= Step;
				if (Left <= 1e-9) { Di = (Di + 1) % Dash.Num(); Left = Dash[Di]; bOn = !bOn; }
			}
		}
	}
	if (Segs.Num() == 0) return;
	const double Hw = LineWidth * 0.5;
	FBox2D B(ForceInit);
	for (const auto& Sg : Segs) { B += Sg.Key; B += Sg.Value; }
	const int32 X0 = FMath::Max(0, FMath::FloorToInt(B.Min.X - Hw - 1)), Y0 = FMath::Max(0, FMath::FloorToInt(B.Min.Y - Hw - 1));
	const int32 X1 = FMath::Min(W - 1, FMath::CeilToInt(B.Max.X + Hw + 1)), Y1 = FMath::Min(H - 1, FMath::CeilToInt(B.Max.Y + Hw + 1));
	if (X1 < X0 || Y1 < Y0) return;
	const int32 Bw = X1 - X0 + 1;
	TArray<float> Cov;
	Cov.Init(0.f, Bw * (Y1 - Y0 + 1));
	for (const auto& Sg : Segs)
	{
		const int32 Sx0 = FMath::Max(X0, FMath::FloorToInt(FMath::Min(Sg.Key.X, Sg.Value.X) - Hw - 1));
		const int32 Sx1 = FMath::Min(X1, FMath::CeilToInt(FMath::Max(Sg.Key.X, Sg.Value.X) + Hw + 1));
		const int32 Sy0 = FMath::Max(Y0, FMath::FloorToInt(FMath::Min(Sg.Key.Y, Sg.Value.Y) - Hw - 1));
		const int32 Sy1 = FMath::Min(Y1, FMath::CeilToInt(FMath::Max(Sg.Key.Y, Sg.Value.Y) + Hw + 1));
		for (int32 Y = Sy0; Y <= Sy1; ++Y)
		{
			for (int32 X = Sx0; X <= Sx1; ++X)
			{
				const double D = SegDist(FVector2D(X + 0.5, Y + 0.5), Sg.Key, Sg.Value);
				const float C = float(FMath::Clamp(Hw - D + 0.5, 0.0, 1.0));
				float& Have = Cov[(Y - Y0) * Bw + (X - X0)];
				Have = FMath::Max(Have, C);
			}
		}
	}
	for (int32 Y = Y0; Y <= Y1; ++Y)
	{
		for (int32 X = X0; X <= X1; ++X)
		{
			const float C = Cov[(Y - Y0) * Bw + (X - X0)];
			if (C > 0.f) Blend(X, Y, StrokeCol, C * StrokeA * GlobalAlpha);
		}
	}
}

void FS47Canvas::Fill()
{
	// the non-zero rule over every subpath, closed; 4 x 4 samples a pixel
	TArray<TPair<FVector2D, FVector2D>> Edges;
	FBox2D B(ForceInit);
	for (const FSub& S : Path)
	{
		if (S.P.Num() < 2) continue;
		for (int32 i = 0; i < S.P.Num(); ++i) { Edges.Add({ S.P[i], S.P[(i + 1) % S.P.Num()] }); B += S.P[i]; }
	}
	if (Edges.Num() == 0) return;
	const int32 X0 = FMath::Max(0, FMath::FloorToInt(B.Min.X)), Y0 = FMath::Max(0, FMath::FloorToInt(B.Min.Y));
	const int32 X1 = FMath::Min(W - 1, FMath::CeilToInt(B.Max.X)), Y1 = FMath::Min(H - 1, FMath::CeilToInt(B.Max.Y));
	for (int32 Y = Y0; Y <= Y1; ++Y)
	{
		for (int32 X = X0; X <= X1; ++X)
		{
			int32 In = 0;
			for (int32 Sy = 0; Sy < 4; ++Sy)
			{
				for (int32 Sx = 0; Sx < 4; ++Sx)
				{
					const double Px_ = X + (Sx + 0.5) / 4.0, Py = Y + (Sy + 0.5) / 4.0;
					int32 Wind = 0;
					for (const auto& E : Edges)
					{
						const FVector2D& A = E.Key; const FVector2D& Bv = E.Value;
						if ((A.Y <= Py) == (Bv.Y <= Py)) continue;
						const double Xc = A.X + (Py - A.Y) / (Bv.Y - A.Y) * (Bv.X - A.X);
						if (Xc > Px_) Wind += Bv.Y > A.Y ? 1 : -1;
					}
					if (Wind != 0) ++In;
				}
			}
			if (In) Blend(X, Y, FillCol, In / 16.0 * FillA * GlobalAlpha);
		}
	}
}

void FS47Canvas::Rect(double X, double Y, double Wd, double Ht)
{
	FSub& S = Path.AddDefaulted_GetRef();
	S.P = { FVector2D(X, Y), FVector2D(X + Wd, Y), FVector2D(X + Wd, Y + Ht), FVector2D(X, Y + Ht) };
	S.bClosed = true;
	// and a new subpath at (x, y), as rect() leaves one
	Path.AddDefaulted_GetRef().P.Add(FVector2D(X, Y));
}

void FS47Canvas::Clip()
{
	FBox2D B(ForceInit);
	for (const FSub& S : Path) for (const FVector2D& P : S.P) B += P;
	if (!B.bIsValid) return;
	if (ClipBox) B = FBox2D(FVector2D(FMath::Max(B.Min.X, ClipBox->Min.X), FMath::Max(B.Min.Y, ClipBox->Min.Y)), FVector2D(FMath::Min(B.Max.X, ClipBox->Max.X), FMath::Min(B.Max.Y, ClipBox->Max.Y)));
	ClipBox = B;
}

void FS47Canvas::Save()
{
	Stack.Add({ FillCol, StrokeCol, FillA, StrokeA, LineWidth, GlobalAlpha, TextAlign, FontFile, FontPx, LineDash, ClipBox });
}

void FS47Canvas::Restore()
{
	if (Stack.Num() == 0) return;
	const FState S = Stack.Pop();
	FillCol = S.FillCol; StrokeCol = S.StrokeCol; FillA = S.FillA; StrokeA = S.StrokeA; LineWidth = S.LineWidth; GlobalAlpha = S.GlobalAlpha;
	TextAlign = S.TextAlign; FontFile = S.FontFile; FontPx = S.FontPx; LineDash = S.LineDash; ClipBox = S.ClipBox;
}

void FS47Canvas::StrokeRect(double X, double Y, double Wd, double Ht)
{
	TArray<FSub> Was = MoveTemp(Path);
	Path.Reset();
	FSub& S = Path.AddDefaulted_GetRef();
	S.P = { FVector2D(X, Y), FVector2D(X + Wd, Y), FVector2D(X + Wd, Y + Ht), FVector2D(X, Y + Ht) };
	S.bClosed = true;
	Stroke();
	Path = MoveTemp(Was);
}

void FS47Canvas::FillRect(double X, double Y, double Wd, double Ht)
{
	// each pixel by how much of it the rectangle covers
	if (Wd < 0) { X += Wd; Wd = -Wd; }
	if (Ht < 0) { Y += Ht; Ht = -Ht; }
	const int32 X0 = FMath::Max(0, FMath::FloorToInt(X)), Y0 = FMath::Max(0, FMath::FloorToInt(Y));
	const int32 X1 = FMath::Min(W - 1, FMath::CeilToInt(X + Wd) - 1), Y1 = FMath::Min(H - 1, FMath::CeilToInt(Y + Ht) - 1);
	const double A = FillA * GlobalAlpha;
	for (int32 Py = Y0; Py <= Y1; ++Py)
	{
		const double Cy = FMath::Min(Py + 1.0, Y + Ht) - FMath::Max(double(Py), Y);
		if (Cy <= 0) continue;
		for (int32 Pxx = X0; Pxx <= X1; ++Pxx)
		{
			const double Cx = FMath::Min(Pxx + 1.0, X + Wd) - FMath::Max(double(Pxx), X);
			if (Cx > 0) Blend(Pxx, Py, FillCol, A * Cx * Cy);
		}
	}
}

void FS47Canvas::DrawImage(const TArray<FColor>& Src, int32 SW, int32 SH, int32 X, int32 Y)
{
	if (Src.Num() < SW * SH) return;
	for (int32 j = 0; j < SH; ++j)
	{
		for (int32 i = 0; i < SW; ++i)
		{
			const FColor& S = Src[j * SW + i];
			if (S.A == 255 && GlobalAlpha >= 1.0 && !ClipBox) { if (X + i >= 0 && Y + j >= 0 && X + i < W && Y + j < H) Px[(Y + j) * W + X + i] = S; }
			else Blend(X + i, Y + j, FColor(S.R, S.G, S.B, 255), S.A / 255.0 * GlobalAlpha);
		}
	}
}

bool FS47Canvas::FillText(const FString& Text, double X, double Y)
{
	// Slate draws the line in white into a render target of its own size; what it covers is laid
	// on the pixels in the fill colour. The baseline is where the canvas puts it: Slate's line
	// top plus the font's ascent (its largest height less the descent).
	if (Text.IsEmpty() || FontFile.IsEmpty() || !FSlateApplication::IsInitialized() || !FApp::CanEverRender()) return false;
	TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateFontInfo Font = S47Style::Font(*FontFile, float(FontPx));
	const FVector2D Size = Measure->Measure(Text, Font);
	const double MaxH = Measure->GetMaxCharacterHeight(Font);
	const double Ascent = MaxH + Measure->GetBaseline(Font);
	const double Left = TextAlign == EAlign::Right ? X - Size.X : TextAlign == EAlign::Center ? X - Size.X * 0.5 : X;
	const double Top = Y - Ascent;
	// room round the advance box: a hand font reaches well past it
	const int32 Pad = FMath::Max(6, FMath::CeilToInt(FontPx * 0.5));
	const int32 Ox = FMath::FloorToInt(Left) - Pad, Oy = FMath::FloorToInt(Top) - Pad;
	const int32 Rw = FMath::CeilToInt(Size.X) + 2 * Pad + 2, Rh = FMath::CeilToInt(FMath::Max(Size.Y, MaxH)) + 2 * Pad + 2;

	static FWidgetRenderer* Renderer = nullptr;
	if (!Renderer) Renderer = new FWidgetRenderer(false, true);
	TStrongObjectPtr<UTextureRenderTarget2D> RT(NewObject<UTextureRenderTarget2D>());
	RT->ClearColor = FLinearColor::Transparent;
	RT->InitCustomFormat(Rw, Rh, PF_B8G8R8A8, true);
	RT->UpdateResourceImmediate(true);
	TSharedRef<SS47TextInk> Ink = SNew(SS47TextInk);
	Ink->Text = Text;
	Ink->Font = Font;
	Ink->At = FVector2f(float(Left - Ox), float(Top - Oy));
	Ink->Size = FVector2D(Rw, Rh);
	Renderer->DrawWidget(RT.Get(), Ink, FVector2D(Rw, Rh), 0.f);
	TArray<FColor> Cov;
	if (!S47Image::ReadPixels(RT.Get(), Cov) || Cov.Num() < Rw * Rh) return false;
	for (int32 j = 0; j < Rh; ++j)
	{
		for (int32 i = 0; i < Rw; ++i)
		{
			const uint8 C = Cov[j * Rw + i].R;
			if (C) Blend(Ox + i, Oy + j, FillCol, C / 255.0 * FillA * GlobalAlpha);
		}
	}
	return true;
}

FString FS47Canvas::ToDataUrl(bool bJpeg, int32 Quality) const
{
	return S47Image::ToDataUrl(Px, W, H, bJpeg, Quality);
}
