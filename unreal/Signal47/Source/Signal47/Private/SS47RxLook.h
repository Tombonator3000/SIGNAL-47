#pragma once
#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class FS47RxFeed;
class FS47SkyAcquire;

// RX-LOOK-20261113 (-> M3, M6; Tom 13 November: «Få receiver til grafisk å se mer ut som bilde nr. to. Masse FX.»): the painted
// parts of the receiver PC's frame, after Tom's reference picture: the header with its dish emblem, the state banner, and the
// small scopes (waterfall, spectrum, acquisition and tracking, I and Q). Pictures only: every number comes from the feed
// (FS47FreeformSession::RxLive) or from the sky rule (FS47SkyAcquire), and the pictures themselves are S47RxLook's functions.
// Bright strokes are drawn as a tube draws them (a wide faint pass under a thin bright one); nothing pulses or types itself in
// with Calm flashes chosen. Buttons and everything a test clicks stay ordinary widgets beside these.

// The header: the dish emblem in strokes, SARO OPS, the station's name as the gate sign has it, the terminal, the date and clock
// the terminal always showed, and the array's own pointing.
class SS47RxHeader : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47RxHeader) {}
		SLATE_ARGUMENT(TSharedPtr<FS47RxFeed>, Feed)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1000, 64); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const override;
private:
	TSharedPtr<FS47RxFeed> Live;
};

// The state banner: the state in words (amber for the live and locked things, and always the word), the antennas on remote and
// the measurement rate, the card's frequency in large digits, the line it belongs to and its wavelength.
class SS47RxBanner : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47RxBanner) {}
		SLATE_ARGUMENT(TSharedPtr<FS47RxFeed>, Feed)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1000, 50); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const override;
private:
	TSharedPtr<FS47RxFeed> Live;
};

// The waterfall: the newest row at the top, the window centred on the dial.
class SS47RxFall : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47RxFall) {}
		SLATE_ARGUMENT(TSharedPtr<FS47RxFeed>, Feed)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(300, 96); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const override;
private:
	TSharedPtr<FS47RxFeed> Live;
};

// The small spectrum: the TUNE page's zoom window, with its decibel and megahertz ticks.
class SS47RxFft : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47RxFft) {}
		SLATE_ARGUMENT(TSharedPtr<FS47RxFeed>, Feed)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(300, 110); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const override;
private:
	TSharedPtr<FS47RxFeed> Live;
};

// One of the two strips under the main panel: the in-phase trace, or the quadrature one.
class SS47RxIq : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47RxIq) : _bQuadrature(false) {}
		SLATE_ARGUMENT(TSharedPtr<FS47RxFeed>, Feed)
		SLATE_ARGUMENT(bool, bQuadrature)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(300, 40); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const override;
private:
	TSharedPtr<FS47RxFeed> Live;
	bool bQuad = false;
};

// Acquisition and tracking: the sky rule's own readouts (signal to noise, residual, each arm's delay and the rate it changes at
// as the sky turns, the nearest catalogued source and how far off it the bearings cross) and a bull's-eye of how far the reticle
// stands off the crossing, in the rule's own degrees of azimuth and elevation.
class SS47RxTrack : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47RxTrack) {}
		SLATE_ARGUMENT(TSharedPtr<FS47RxFeed>, Feed)
		SLATE_ARGUMENT(TSharedPtr<FS47SkyAcquire>, Model)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(300, 150); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const override;
	// Where an offset of so many degrees stands from the bull's-eye's middle, as a share of its radius (the lock radius a third out).
	static float EyeShare(double Degrees);
	static constexpr double EyeFull = 14.;
private:
	TSharedPtr<FS47RxFeed> Live;
	TSharedPtr<FS47SkyAcquire> Sky;
};

namespace S47RxLookPaint
{
	bool SelfTest(TArray<FString>& Lines);
}
