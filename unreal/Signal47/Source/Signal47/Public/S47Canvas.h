#pragma once

#include "CoreMinimal.h"

// The part of the browser's CanvasRenderingContext2D the web version draws its pictures with
// before it hands them on as data URLs (core/FieldCamera.ts makePrint, story/drawings.ts): the
// pixels on the CPU, 8-bit sRGB with source-over blending as a canvas has them, and the text
// through Slate (the glyphs' coverage drawn into a render target, then laid on the pixels in the
// fill colour). Coordinates and sizes are the canvas's pixels.
//
//   FS47Canvas G(960, 420);
//   G.SetFill(0xefe8d6); G.FillRect(0, 0, 960, 420);       // g.fillStyle = '#efe8d6'; g.fillRect(...)
//   G.SetFont(TEXT("SpecialElite-Regular.ttf"), 19);        // g.font = '19px "Special Elite"'
//   G.FillText(TEXT("A"), 140, 340);                        // on its baseline, as fillText
//   const FString Url = G.ToDataUrl(false);                  // canvas.toDataURL('image/png')
class SIGNAL47_API FS47Canvas
{
public:
	FS47Canvas(int32 InW, int32 InH);

	int32 W = 0, H = 0;
	TArray<FColor> Px; // top row first

	// the state (fillStyle, strokeStyle as #rrggbb plus an alpha, as rgba(); lineWidth, globalAlpha,
	// font, textAlign). Line caps and joins are round, as story/drawings.ts sets them.
	void SetFill(uint32 Rgb, double A = 1.0);
	void SetStroke(uint32 Rgb, double A = 1.0);
	double LineWidth = 1.0;
	double GlobalAlpha = 1.0;
	enum class EAlign : uint8 { Left, Right, Center };
	EAlign TextAlign = EAlign::Left;
	void SetFont(const TCHAR* File, double SizePx); // a font in Content/Fonts at a size in canvas pixels

	// paths: beginPath, moveTo, lineTo, closePath, arc, stroke, fill
	void BeginPath();
	void MoveTo(double X, double Y);
	void LineTo(double X, double Y);
	void ClosePath();
	void Arc(double Cx, double Cy, double R, double A0, double A1);
	void Stroke();
	void Fill();

	void FillRect(double X, double Y, double Wd, double Ht);
	// drawImage(img, x, y) at whole pixels, unscaled
	void DrawImage(const TArray<FColor>& Src, int32 SW, int32 SH, int32 X, int32 Y);
	// fillText(text, x, y): on the alphabetic baseline, aligned by TextAlign. False when Slate
	// could not draw it (no renderer: a commandlet, a server).
	bool FillText(const FString& Text, double X, double Y);

	FString ToDataUrl(bool bJpeg, int32 Quality = 92) const;

	// a number stored into a Uint8ClampedArray: clamped, rounded half to even
	static uint8 Clamp8(double V);

private:
	FColor FillCol = FColor::Black, StrokeCol = FColor::Black;
	double FillA = 1.0, StrokeA = 1.0;
	FString FontFile;
	double FontPx = 10.0;
	struct FSub { TArray<FVector2D> P; bool bClosed = false; };
	TArray<FSub> Path;
	void Blend(int32 X, int32 Y, const FColor& C, double A);
};
