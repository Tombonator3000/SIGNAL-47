#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Widgets/SCompoundWidget.h"

struct FSlateBrush;

// The game's fonts, read from Content/Fonts (VT323 for the terminal, OFL).
namespace S47Style
{
	FSlateFontInfo Font(const TCHAR* File, float Px);
	FSlateFontInfo Mono(float Px);
	const FLinearColor Amber(1.0f, 0.68f, 0.22f);
	const FLinearColor Green(0.45f, 1.0f, 0.6f);
	const FLinearColor Paper(0.93f, 0.9f, 0.84f);
}

// The overlay while walking: a dot, the prompt for what you look at, the station clock.
class SS47Hud : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Hud) {}
		SLATE_ATTRIBUTE(FText, Prompt)
		SLATE_ATTRIBUTE(FText, Clock)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);

private:
	TUniquePtr<FSlateBrush> DotBrush;
};

// The status terminal as a full screen: the survey sweep, in the station's amber phosphor.
// Esc closes it. Fictional readouts (memory.md canon: 1420.405 MHz, az 026, 27 dishes).
class SS47Terminal : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Terminal) {}
		SLATE_ATTRIBUTE(FText, Clock)
		SLATE_ARGUMENT(TFunction<void()>, OnClose)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	TFunction<void()> OnClose;
	TUniquePtr<FSlateBrush> WhiteBrush;
	TUniquePtr<FSlateBrush> PanelBrush;
};
