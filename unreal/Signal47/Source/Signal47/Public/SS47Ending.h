#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"

class SButton;
class UTexture2D;

// ui/Ending.ts: FLASH over the game, the black, SIGNAL / 47, the credits and the whole 1947
// photograph. Slate's real-time tick replaces window.setTimeout; it keeps running in mode 'end'.
class SIGNAL47_API SS47Ending : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Ending) : _BoughtSfx(false), _LargeText(false) {}
		SLATE_ARGUMENT(bool, BoughtSfx)
		SLATE_ARGUMENT(bool, LargeText)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual void Tick(const FGeometry& G, const double InCurrentTime, const float InDeltaTime) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;

	static TArray<TPair<FString, FString>> CreditsRows(bool bBoughtSfx);
	/** One frame of FLASH over the game (null: none). Opacity below 1 for the slow fades. */
	void Flash(UTexture2D* C, double Opacity = 1);
	/** Black over everything, faded in or out over milliseconds. */
	void Black(bool bOn, double Ms = 600);
	// The async methods' completion callbacks run after precisely the waits in Ending.ts.
	void Title(TFunction<void()> Done = {});
	void Credits(TFunction<void()> Done = {});
	void Photo(UTexture2D* Src, TFunction<void()> Paper, TFunction<void()> Done = {});
	bool Disposed() const { return bGone; }
	void Dispose();

	// The real-time timers, also stepped by the world-free API tests.
	void Advance(double Dt);
	bool Skip();
	bool SkipOffered() const;
	static bool SelfTest(TArray<FString>& Lines);

private:
	enum class ECard : uint8 { None, Title, Credits, Photo };
	ECard Card = ECard::None;
	bool bGone = false, bBoughtSfx = false, bLargeText = false, bPaperPlayed = false;
	double T = 0, CreditsOut = 46.06;
	TFunction<void()> OnDone, OnPaper;
	TStrongObjectPtr<UTexture2D> Frame, Photograph;
	FSlateBrush FrameBrush, PhotoBrush;
	double FrameOpacity = 0;
	double CoverFrom = 0, CoverTo = 0, CoverT = 0, CoverDuration = 0;
	TSharedPtr<SButton> SkipButton;

	struct FCreditLine { FString Text; FSlateFontInfo Font; FLinearColor Color; float Y = 0, Height = 0; };
	mutable FVector2D CreditsSize = FVector2D(-1, -1);
	mutable TArray<FCreditLine> CreditLines;
	mutable float CreditsHeight = 0, CreditsTitleHeight = 0;
	void LayoutCredits(const FVector2D& Size) const;
	double CoverAlpha() const;
	double CardAlpha() const;
	bool CardPresent() const;
	void BeginCard(ECard Kind, TFunction<void()> Done);
	void Complete();
};
