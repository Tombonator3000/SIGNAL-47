#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"

// story/drawings.ts, the drawings chapter two uses: the arrangement drawings clipped to the 1947
// record (E07, and the service copy E06 without C), the archive sleeve's fixed-point sheet (E08),
// the mark icons, and the case map. They follow the paper test in Docs/Menu14
// (archive-reference.svg): A is the fixed survey point (an outlined triangle with a short bar
// under it), B the comparison vane, C the closing sight line. The amended copy simply has no C.
// Nothing is circled or marked: the change is found by comparing the two.
//
// The web version draws them on a 2D canvas and hands the page a PNG data URL. Here the same
// calls are recorded on FCanvas2D (the subset of CanvasRenderingContext2D they use), painted
// with Slate into a render target of the same size, read back and written as a PNG data URL
// (S47Image). Chapter one's helper ports the other drawings to S47Drawings at the same time;
// the two are to be merged into one later.
namespace S47Drawings2
{
	// The calls the drawings make on their canvas, kept as a list and painted in order.
	class SIGNAL47_API FCanvas2D
	{
	public:
		FCanvas2D(int32 InW, int32 InH) : W(InW), H(InH) {}
		int32 W, H;

		// the state (fillStyle, strokeStyle, lineWidth, globalAlpha, setLineDash, font)
		void FillStyle(uint32 Rgb, double A = 1.0) { FillRgb = Rgb; FillA = A; }
		void StrokeStyle(uint32 Rgb, double A = 1.0) { LineRgb = Rgb; LineA = A; }
		double LineWidth = 1.0;
		double GlobalAlpha = 1.0;
		void SetLineDash(const TArray<double>& Dash) { LineDash = Dash; }
		// font: a family the game has ('Oswald', 'Special Elite') at a size in CSS pixels; bBold
		// the weight 600 the titles ask for (the page has one Oswald, Medium, for every weight)
		void Font(const FString& Family, double Px, bool bBold = false) { FontFamily = Family; FontPx = Px; bFontBold = bBold; }

		// the path
		void BeginPath();
		void MoveTo(double X, double Y);
		void LineTo(double X, double Y);
		void ClosePath();
		void Arc(double Cx, double Cy, double R, double A0, double A1);
		void BezierCurveTo(double C1x, double C1y, double C2x, double C2y, double X, double Y);
		void Stroke();

		// rectangles and text
		void FillRect(double X, double Y, double Rw, double Rh);
		void StrokeRect(double X, double Y, double Rw, double Rh);
		void FillText(const FString& S, double X, double Y);

		// what is painted, in order
		struct FCmd
		{
			enum class EKind : uint8 { Rect, Lines, Dot, Text } Kind = EKind::Rect;
			FLinearColor Color;
			TArray<FVector2f> Points;   // Lines: a polyline
			FVector2f At = FVector2f::ZeroVector, Size = FVector2f::ZeroVector; // Rect, Dot (centre, diameter in Size.X)
			float Width = 1.f;
			FString Text;
			FSlateFontInfo Font;
		};
		TArray<FCmd> Cmds;

	private:
		uint32 FillRgb = 0x000000, LineRgb = 0x000000;
		double FillA = 1.0, LineA = 1.0;
		TArray<double> LineDash;
		FString FontFamily = TEXT("Special Elite");
		double FontPx = 10.0;
		bool bFontBold = false;
		struct FSub { TArray<FVector2D> Pts; TArray<bool> Corner; bool bClosed = false; };
		TArray<FSub> Subs;
		FSub& Current();
		void StrokeSub(const FSub& S);
		void EmitLine(const TArray<FVector2D>& Pts, const TArray<bool>& Corner, bool bClosed, const FLinearColor& C);
	};

	// The fixed-point mark. bBar false is the plain elevation symbol that does not match.
	SIGNAL47_API void Mark(FCanvas2D& G, double Cx, double Cy, double Size, bool bBar = true);

	// sheet(w, h, draw): the paper with a little age on it, ink set up, the drawing, and the PNG
	// data URL (empty when nothing can be rendered: no Slate renderer)
	SIGNAL47_API FString Sheet(int32 W, int32 H, TFunctionRef<void(FCanvas2D&)> Draw);
	SIGNAL47_API void PaperBg(FCanvas2D& G, int32 W, int32 H);

	// ARRANGEMENT drawing attached to the 1947 record. bFull = original (A, B, C).
	SIGNAL47_API FString Arrangement(bool bFull);
	// The index diagram from the archive sleeve: the matching fixed-point mark.
	SIGNAL47_API FString SleeveMark();
	// Small icons for the destination choices: 'triangle-bar' | 'triangle' | 'three-bars'.
	SIGNAL47_API FString MarkIcon(const FString& Kind);
	// The case map: SARO first, the old survey station once P05 is recorded, the motel once its
	// number has been called. Dashed lines are travel, not walkable ground. No scale.
	SIGNAL47_API FString CaseMap(bool bStation, bool bMotel);

	// the paper's noise as drawings.ts makes it (its LCG), and a drawing rendered and read back
	bool SelfTest(TArray<FString>& Lines);
}
