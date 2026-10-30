#pragma once

#include "CoreMinimal.h"

class FS47Canvas;
class UTexture;

// story/drawings.ts: the drawings that belong to documents, made on a canvas and handed to the
// page as PNG data URLs. They follow the paper test in Docs/Menu14 (archive-reference.svg): A is
// the fixed survey point (an outlined triangle with a short bar under it), B the comparison vane,
// C the closing sight line. The amended copy simply has no C. Nothing is circled or marked: the
// change is found by comparing the two. (The case map is ported with chapter two, S47Drawings2;
// the service map, the tracing and the diner's clipping with chapter five, below.)
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

	// ---------- All Night: SARO 07's service map and the tracing of E09A ----------
	// The map is north up, 1200 x 800, not to any survey scale. The old Roswell road leaves the
	// highway at the diner; its mileposts count from there. C runs due east from the transit at
	// STATION 01 (as the stakes do at the built station), so on the map it is the horizontal through
	// STATION 01. The road is laid so that line meets it 8.15 miles from the junction, just past the
	// eight-mile post, where a survey bolt sits in the shoulder.
	struct FServiceMap
	{
		int32 W = 1200, H = 800;
		FVector2D Station = FVector2D::ZeroVector;
		TArray<FVector2D> Road;
		double PerMile = 0, Miles = 0;
		// the bolts: the one where C crosses, and three others along the shoulder
		struct FBolt { double Mi = 0; FVector2D At = FVector2D::ZeroVector; };
		TArray<FBolt> Bolts;
		double Snap = 18;
		// where a milepost stands on the map (post(mi))
		FVector2D Post(double Mi) const;
	};
	SIGNAL47_API const FServiceMap& SERVICE_MAP();
	// E14, the service map as a picture for the case file (a PNG data URL)
	SIGNAL47_API FString ServiceMap();
	// serviceMapCanvas: the service map painted on a canvas of SERVICE_MAP's size, for the overlay at
	// the diner (SS47MapOverlay)
	SIGNAL47_API void ServiceMapCanvas(FS47Canvas& G);
	// New SC12 only: an additional transfer annotation, never painted into a legacy E14.
	SIGNAL47_API void HandoverMarks(FS47Canvas& G);
	SIGNAL47_API FString HandoverMap();
	// E09A traced onto onion-skin: the transit at (X, Y), C running east to the edge of the sheet, A
	// and B close by, and the north arrow. Drawn over the service map.
	SIGNAL47_API void DrawTracing(FS47Canvas& G, double X, double Y);

	// The clipping as it hangs by the diner's door (Chapter5.ts docClipping): world/Diner.ts draws it
	// on a 1024 x 1536 canvas, and the export has that canvas as the texture on the frame. It is read
	// back here, the right way up, as a JPEG data URL at 0.85 (canvas.toDataURL('image/jpeg', 0.85));
	// empty when nothing can be rendered.
	SIGNAL47_API FString Clipping(UTexture* Tex);

	// checks of the drawing that need no screen (the paper's random speckle, the strokes)
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
