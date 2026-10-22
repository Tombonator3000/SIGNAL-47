#pragma once

#include "CoreMinimal.h"
#include "S47UI.h"
#include "Widgets/SLeafWidget.h"

// ui/areaMap.ts: the map in the notebook, the player's own sketch in pencil on a notebook page (SARO
// and the array east of the highway, Sierra Motor Court down it, Mesa Diner at the junction, the
// track to STATION 01 once known, the old road), and a red cross where the player is. The web
// version draws it into a canvas once and shows it as a picture; this draws the same strokes, in
// the same order and with the same pencil jitter, straight into the notebook page (1200 x 800,
// scaled to the width it is given).
namespace S47AreaMap
{
	FVector2f SpotXY(const FS47MapAt& At);                 // spotXY: where a spot is on the page
	bool LinesMeet(const FS47MapView& V);                  // A and B drawn in and C found
	TArray<FString> Description(const FS47MapView& V);     // areaMapDescription
}

class SS47AreaMap : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47AreaMap) : _Width(900.f) {}
		SLATE_ARGUMENT(FS47MapView, View)
		SLATE_ARGUMENT(float, Width)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs) { View = InArgs._View; Width = InArgs._Width; }
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(Width, Width * 800.0 / 1200.0); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;

private:
	FS47MapView View;
	float Width = 900.f;
};
