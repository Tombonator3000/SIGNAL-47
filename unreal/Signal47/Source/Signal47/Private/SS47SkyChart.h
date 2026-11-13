#pragma once
#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Widgets/SLeafWidget.h"

class FS47SkyAcquire;

// RX-ACQUIRE-SKY-20261113 (-> M3): the receiver's ACQUIRE page, the sky chart on the tube. A 1986 vector terminal's drawing of
// FS47SkyAcquire: the az/el grid, the catalogue as labelled points, one bearing line an antenna, the triangle the three arms' bands
// make, the reticle, the lock sequence's rings and the readouts counting up beside it. Picture and input only; the rule is the
// model's and the job is the session's. It is a schematic on the terminal: nothing here reads or draws the game's star sky.
//
// Input: the reticle follows the pointer while it is on the chart (a mouse, or a controller's right-stick pointer), and the arrow
// keys, the walk keys and a controller's d-pad steer it while the chart has the keys (it is the screen's first key target, and a
// click on it takes them).
//
// Nothing flashes: the only things that pulse are small marks, slower than twice a second, and with Calm flashes chosen they stand
// still. Every state is also said in words or shape (LOCKED / SWEEPING, a dashed band against a solid one, a diamond against a square).
class SS47SkyChart : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47SkyChart) {}
		SLATE_ARGUMENT(TSharedPtr<FS47SkyAcquire>, Model)
		// Once a painted frame, with the seconds since the last: the owner tells the model what the array does, steps it and plays
		// what happened. Without one the chart steps the model itself.
		SLATE_ARGUMENT(TFunction<void(double)>, OnStep)
		SLATE_ARGUMENT(TFunction<bool()>, Calm)
		// The operator's hand on the chart, for its sound: 0 a steering key or a click taken, 1 a click refused (no bearings yet).
		SLATE_ARGUMENT(TFunction<void(int32)>, OnKey)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1040, 384); }
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& G, const FKeyEvent& E) override;
	virtual FReply OnKeyUp(const FGeometry& G, const FKeyEvent& E) override;
	virtual void OnFocusLost(const FFocusEvent& E) override;
	virtual FReply OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override;
	virtual FReply OnMouseMove(const FGeometry& G, const FPointerEvent& E) override;
	virtual void OnMouseLeave(const FPointerEvent& E) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& G, const FPointerEvent& E) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 L, const FWidgetStyle& Style, bool bEnabled) const override;

	// The plot inside the widget: where the chart's window sits, at one scale on both axes, with the readouts to its right.
	struct FPlot { FVector2f Origin = FVector2f::ZeroVector, Extent = FVector2f::ZeroVector; float Scale = 1.f, ReadoutX = 0.f; };
	static constexpr float ReadoutWide = 262.f;
	static FPlot PlotFor(const FVector2f& Size);
	static FVector2f ToLocal(const FPlot& P, const FVector2D& AzEl);
	static FVector2D ToSky(const FPlot& P, const FVector2f& Local);
	static bool Contains(const FPlot& P, const FVector2f& Local);
	// The four ways a key can steer (-1, 0 or 1 on each axis; elevation up is positive).
	static FVector2D SteerOf(const FKey& Key);
	static bool SelfTest(TArray<FString>& Lines);

private:
	TSharedPtr<FS47SkyAcquire> Sky;
	TFunction<void(double)> StepFn;
	TFunction<bool()> CalmFn;
	TFunction<void(int32)> KeyFn;
	TSet<FKey> HeldKeys;
	FVector2f HoverLocal = FVector2f::ZeroVector;
	bool bHover = false;
	mutable double LastPaint = 0;
	FVector2D SteerNow() const;
	bool Steerable() const;
};
