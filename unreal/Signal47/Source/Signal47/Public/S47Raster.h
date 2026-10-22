#pragma once

#include "CoreMinimal.h"

// The part of the browser's CanvasRenderingContext2D the single frames of the time away and of
// FLASH are drawn with (story/abduction.ts grain, painted, frameLight, frameHand, the frost; story/
// flashFrames.ts; core/roadside.ts makeBent): pixels on the CPU, 8-bit sRGB values blended as a
// canvas blends them (source-over, or 'lighter'), paths under a transform (translate, rotate,
// scale; lines, arcs, ellipses, Bezier and quadratic curves, rounded rectangles) filled by the
// non-zero rule with their edges anti-aliased, stroked, or used as a clip; the radial and linear
// gradients; CSS's blur, grayscale, contrast and brightness filters; drawImage with a source
// rectangle, scaled bilinearly; and the pixels themselves (getImageData / putImageData).
//
//   FS47Pix G(640, 400);
//   G.SetFill(0x000000); G.FillRect(0, 0, 640, 400);          // g.fillStyle = '#000'; g.fillRect(...)
//   FS47Pix::FPaint L = FS47Pix::Radial(320, 40, 10, 320, 160, 420);
//   L.Stop(0, 0xffffff); L.Stop(1, 0x101214); G.SetFill(L);    // a gradient as the fill style
//   G.Filter = { { FS47Pix::EFilter::Blur, 2.5 } };            // g.filter = 'blur(2.5px)'
//
// Kept apart from S47Canvas (the papers' canvas, with Slate's text) so the two can be merged later
// without either changing its callers: FS47Pix hands text to FS47Canvas (Text below).
class SIGNAL47_API FS47Pix
{
public:
	FS47Pix(int32 InW = 1, int32 InH = 1);
	int32 W = 0, H = 0;
	/** Premultiplied RGBA, sRGB values 0..1, the top row first. */
	TArray<FVector4f> P;

	// ---------- the state ----------
	struct FPaint
	{
		enum class EKind : uint8 { Solid, Radial, Linear } Kind = EKind::Solid;
		FVector4f C = FVector4f(0, 0, 0, 1);   // the colour (straight, sRGB 0..1)
		double X0 = 0, Y0 = 0, R0 = 0, X1 = 0, Y1 = 0, R1 = 0;
		TArray<TPair<double, FVector4f>> Stops;
		/** addColorStop(t, 'rgba(r, g, b, a)') with a #rrggbb colour. */
		FPaint& Stop(double T, uint32 Rgb, double A = 1.0);
	};
	static FPaint Solid(uint32 Rgb, double A = 1.0);
	static FPaint Radial(double X0, double Y0, double R0, double X1, double Y1, double R1);
	static FPaint Linear(double X0, double Y0, double X1, double Y1);
	FPaint FillStyle = Solid(0x000000), StrokeStyle = Solid(0x000000);
	void SetFill(uint32 Rgb, double A = 1.0) { FillStyle = Solid(Rgb, A); }
	void SetFill(const FPaint& Pt) { FillStyle = Pt; }
	void SetStroke(uint32 Rgb, double A = 1.0) { StrokeStyle = Solid(Rgb, A); }
	double LineWidth = 1.0;
	double GlobalAlpha = 1.0;
	/** globalCompositeOperation: 'source-over' (false) or 'lighter' (true). */
	bool bLighter = false;
	/** filter: CSS's filter functions in their order ('none': empty). */
	enum class EFilter : uint8 { Blur, Grayscale, Contrast, Brightness };
	TArray<TPair<EFilter, double>> Filter;

	void Save();
	void Restore();
	void Translate(double X, double Y);
	void Rotate(double A);
	void Scale(double X, double Y);

	// ---------- paths ----------
	void BeginPath();
	void MoveTo(double X, double Y);
	void LineTo(double X, double Y);
	void QuadraticCurveTo(double Cx, double Cy, double X, double Y);
	void BezierCurveTo(double C1x, double C1y, double C2x, double C2y, double X, double Y);
	void Arc(double Cx, double Cy, double R, double A0, double A1, bool bCcw = false);
	void Ellipse(double Cx, double Cy, double Rx, double Ry, double Rot, double A0, double A1, bool bCcw = false);
	void RoundRect(double X, double Y, double Wd, double Ht, double R);
	void ClosePath();
	void Fill();
	void Stroke();
	void Clip();

	void FillRect(double X, double Y, double Wd, double Ht);
	/** clearRect at whole pixels (transparent black). */
	void ClearRect(int32 X, int32 Y, int32 Wd, int32 Ht);
	/** drawImage(src, sx, sy, sw, sh, dx, dy, dw, dh), under the transform. */
	void DrawImage(const FS47Pix& Src, double Sx, double Sy, double Sw, double Sh, double Dx, double Dy, double Dw, double Dh);
	void DrawImage(const FS47Pix& Src, double Dx, double Dy, double Dw, double Dh) { DrawImage(Src, 0, 0, Src.W, Src.H, Dx, Dy, Dw, Dh); }
	/** fillText in the fill colour (a solid one), a font of Content/Fonts at a size in pixels (S47Canvas). */
	bool Text(const FString& S, double X, double Y, const TCHAR* FontFile, double SizePx);

	// ---------- the pixels ----------
	/** getImageData: straight 8-bit RGBA, the top row first. */
	TArray<FColor> Colors() const;
	/** putImageData at 0, 0. */
	void SetColors(const TArray<FColor>& C);
	static FS47Pix FromColors(const TArray<FColor>& C, int32 InW, int32 InH);
	/** Uint8ClampedArray: clamped, rounded half to even. */
	static uint8 Clamp8(double V);

	static bool SelfTest(TArray<FString>& Lines);

private:
	struct FMat { double A = 1, B = 0, C = 0, D = 1, E = 0, F = 0; FVector2D Apply(double X, double Y) const { return FVector2D(A * X + C * Y + E, B * X + D * Y + F); } FMat Inverse() const; };
	struct FState { FMat M; FPaint Fill, Stroke; double LineWidth, GlobalAlpha; bool bLighter; TArray<TPair<EFilter, double>> Filter; TSharedPtr<TArray<float>> Clip; };
	FMat M;
	TArray<FState> Stack;
	TSharedPtr<TArray<float>> ClipMask;
	struct FSub { TArray<FVector2D> Pts; bool bClosed = false; };
	TArray<FSub> Subs;
	FSub& Cur();
	double Seg(double DeviceLen) const;
	void Polys(TArray<TArray<FVector2D>>& Out, bool bForStroke) const;
	/** Coverage (0..1) of the polygons by the non-zero rule, over the canvas. */
	void Cover(const TArray<TArray<FVector2D>>& Polys, TArray<float>& Out, FIntRect& Box) const;
	FVector4f PaintAt(const FPaint& Pt, double X, double Y, const FMat& Inv) const;
	/** Lays coverage in a paint onto the pixels, through the filter, the clip and the composite. */
	void Composite(const TArray<float>& Cov, const FIntRect& Box, const FPaint& Pt);
	void Layer(TArray<FVector4f>& L, const FIntRect& Box);
	void ApplyFilters(TArray<FVector4f>& L, FIntRect& Box) const;
};

namespace S47Raster
{
	/** core/art.ts artImage: a picture shipped with the build (Content/WebData/art), its pixels; false when it is not there. */
	SIGNAL47_API bool Art(const FString& Name, FS47Pix& Out);
	SIGNAL47_API bool ArtLoaded(const FString& Name);
	/** core/textures.ts rng: the same generator (seeded, a uint32 step), 0 <= r < 1. */
	struct FRng { uint32 S; explicit FRng(double Seed) : S(uint32(int64(Seed))) {} double operator()() { S = S * 1664525u + 1013904223u; return S / 4294967296.0; } };
}
