#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SLeafWidget.h"

class UTexture2D;

// What story/abduction.ts puts on the page over the game: the white light from above (the class
// .abduct-light, a radial gradient at 50% -10%, white to a cold grey-white, at the opacity --abduct),
// and the frame canvas over everything (640 x 400, object-fit: cover, on black). chapter six's FLASH
// frames (story/flashFrames.ts) can be shown on it the same way. Added to the viewport over the HUD by
// US47Away.
class SS47AwayOverlay : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47AwayOverlay) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1, 1); }

	/** --abduct with .abduct-light on (unset: the class off). */
	void SetLight(TOptional<double> K) { Light = K; }
	/** canvas.style.display = 'block' with this drawn on black at alpha (null: black), or 'none'. */
	void ShowFrame(UTexture2D* T, double Alpha, bool bFadeBlack = false);
	void HideFrame() { bFrame = false; Frame.Reset(); }
	bool FrameShown() const { return bFrame; }

private:
	TOptional<double> Light;
	bool bFrame = false;
	bool bFrameFadeBlack = false; // Ending.flash group opacity; abduction keeps its opaque black
	double Alpha = 1;
	TStrongObjectPtr<UTexture2D> Frame;
	TStrongObjectPtr<UTexture2D> LightTex;
	FSlateBrush FrameBrush, LightBrush;
	TUniquePtr<FSlateBrush> Black;
};
