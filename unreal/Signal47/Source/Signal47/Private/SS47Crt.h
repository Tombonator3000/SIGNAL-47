#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class AS47Story;
class AS47Web;
struct FSlateBrush;

// The pictures on the four CRTs in the control room, after Prologue.ts drawCrts: the array
// status (left), the spectrum (centre), the tracking terminal (right) and the operations
// terminal on the west desk. Drawn at 512 by 384 into a render target a few times a second.
class SS47Crt : public SLeafWidget
{
public:
	enum class EKind : uint8 { Status, Spectrum, Tracking, Ops };
	SLATE_BEGIN_ARGS(SS47Crt) {}
		SLATE_ARGUMENT(EKind, Kind)
		SLATE_ARGUMENT(AS47Story*, Story)
		SLATE_ARGUMENT(AS47Web*, Saro)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(512, 384); }

private:
	EKind Kind = EKind::Status;
	AS47Story* Story = nullptr;
	AS47Web* Saro = nullptr;
	TUniquePtr<FSlateBrush> WhiteBrush;
	void Txt(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer, const FString& S, float X, float Y, float Size = 24.f, const FLinearColor& Color = FLinearColor(0.55f, 1.f, 0.64f)) const;
	void Clear(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer) const;
	void Boot(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer, double E) const;
	void Status(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer) const;
	void Spectrum(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer) const;
	void Tracking(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer) const;
	void Ops(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer) const;
	void Scanlines(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer) const;
};
