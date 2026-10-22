#pragma once

#include "CoreMinimal.h"

class FS47Canvas;

// story/drawings.ts: the drawings that belong to documents, made on a canvas and handed to the
// page as PNG data URLs. They follow the paper test in Docs/Menu14 (archive-reference.svg): A is
// the fixed survey point (an outlined triangle with a short bar under it), B the comparison vane,
// C the closing sight line. The amended copy simply has no C. Nothing is circled or marked: the
// change is found by comparing the two. (The case map and the service map are ported with
// chapter two and five.)
namespace S47Drawings
{
	// the paper, with a little age on it (paperBg), and the sheet a drawing is made on (sheet):
	// paper, ink, line width 3, round caps and joins
	SIGNAL47_API void PaperBg(FS47Canvas& G, int32 W, int32 H);
	SIGNAL47_API FString Sheet(int32 W, int32 H, TFunctionRef<void(FS47Canvas&)> Draw);

	// The fixed-point mark. bBar false is the plain elevation symbol that does not match.
	SIGNAL47_API void Mark(FS47Canvas& G, double Cx, double Cy, double Size, bool bBar = true);

	// ARRANGEMENT drawing attached to the 1947 record. bFull = original (A, B, C).
	SIGNAL47_API FString Arrangement(bool bFull);
	// The index diagram from the archive sleeve: the matching fixed-point mark.
	SIGNAL47_API FString SleeveMark();
	// Small icons for the destination choices: 'triangle-bar' | 'triangle' | 'three-bars'.
	SIGNAL47_API FString MarkIcon(const FString& Kind);

	// The transit record E09A behind the glass at STATION 01: the 1947 plan around the transit,
	// laid out from where A's footing, B and C's stakes really are at the built station (metres
	// from the transit, x east and z south; C is only a direction).
	SIGNAL47_API FString TransitRecord(const FVector2D& A, const FVector2D& B, const FVector2D& C);

	constexpr uint32 PAPER = 0xefe8d6, INK = 0x22372e;

	// checks of the drawing that need no screen (the paper's random speckle, the strokes)
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
