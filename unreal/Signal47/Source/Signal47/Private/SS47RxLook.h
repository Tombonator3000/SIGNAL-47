#pragma once
#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "S47SkySphere.h"

class FS47RxFeed;
class FS47SkyAcquire;

// RX-LOOK-20261113 (-> M3, M6; Tom 13 November: «Få receiver til grafisk å se mer ut som bilde nr. to. Masse FX.»): the painted
// parts of the receiver PC's frame, after Tom's reference picture: the header with its dish emblem, the state banner, and the
// small scopes (waterfall, spectrum, acquisition and tracking, I and Q). Pictures only: every number comes from the feed
// (FS47FreeformSession::RxLive) or from the sky rule (FS47SkyAcquire), and the pictures themselves are S47RxLook's functions.
// Bright strokes are drawn as a tube draws them (a wide faint pass under a thin bright one); nothing pulses or types itself in
// with Calm flashes chosen. Buttons and everything a test clicks stay ordinary widgets beside these.
//
// Round 2 (13 November, the first look in the engine): no word under the next step's size (S47RxLook::TextFloor; fewer words
// rather than smaller ones), every number written by S47RxLook::Num (none shows a 47), one more step of glow on the three
// headline words (SARO OPS, the state, the card's frequency), the header's right block in from the glass's rounded corner,
// the waterfall as thin rows of grain with a soft line, and a little noise riding on the locked I and Q traces.

// The header: the dish emblem in strokes, SARO OPS, the station's name as the gate sign has it, the terminal, the date and clock
// the terminal always showed, and the array's own pointing.
class SS47RxHeader : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47RxHeader) {}
		SLATE_ARGUMENT(TSharedPtr<FS47RxFeed>, Feed)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1000, 68); }
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

// The waterfall: the newest row at the top, the window centred on the dial, the seconds down its left side.
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
// as the sky turns, the nearest catalogued source and how far off it the bearings cross), a name and its number a line, and a
// bull's-eye of how far the reticle stands off the crossing: as far out as the rule's own distance, the way it lies on the glass.
// RX-LOOK gjennomgang 14 Nov: it was drawn azimuth across and elevation up, while the keys steer the way they point on the upright
// equatorial chart; the two differ by the parallactic angle (some -64 degrees at 3C 286), so UP on the keys moved the mark
// diagonally. Now its direction is the chart's own (EyeAt), so UP on the keys is up on the bull's-eye.
class SS47RxTrack : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47RxTrack) {}
		SLATE_ARGUMENT(TSharedPtr<FS47RxFeed>, Feed)
		SLATE_ARGUMENT(TSharedPtr<FS47SkyAcquire>, Model)
		// The chart's open picture as it was last painted (none: the chart's picture at its usual size).
		SLATE_ARGUMENT(TFunction<S47SkySphere::FView()>, View)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(300, 150); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const override;
	// Where an offset of so many degrees stands from the bull's-eye's middle, as a share of its radius (the lock radius a third out).
	static float EyeShare(double Degrees);
	static constexpr double EyeFull = 14.;
	// The reticle's mark on the bull's-eye (x to the right, y up, a share of its radius): the way from the crossing to the reticle
	// on the chart's picture, as far out as EyeShare of the rule's distance.
	static FVector2D EyeAt(const S47SkySphere::FView& View, const FVector2D& Reticle, const FVector2D& Cross);
	// RX-LOOK gjennomgang 14 Nov: the lines the panel writes for the rule's state, as it writes them (the self-test and the receiver
	// test read the same): a name and its value, and whether it is a quiet one. bAllArms: every arm's delay, else the longest only.
	struct FRow { FString Name, Value; bool bQuiet = false; FString Line() const { return Name.RightPad(8) + Value; } };
	static TArray<FRow> Rows(const FS47SkyAcquire* Sky, double CardMhz, bool bAllArms);
	static FString OffWords(const FS47SkyAcquire* Sky);
	// RX-LOOK gjennomgang 14 Nov: how the rows stand in a panel so high: whether every arm's delay has a row (else the longest
	// only), and the pitch, so that the last row (OFFSET) stands whole above the panel's foot. RowHigh is what a row of the next
	// step's size takes on the glass; the pitch is never more than the old 22 and never less than a letter's height (19).
	static constexpr float RowHigh = 24.f, RowPitchMost = 22.f, RowPitchLeast = 19.f;
	static bool AllArms(float High);
	static float RowPitch(float High, int32 Count);
private:
	TSharedPtr<FS47RxFeed> Live;
	TSharedPtr<FS47SkyAcquire> Sky;
	TFunction<S47SkySphere::FView()> ViewFn;
};

namespace S47RxLookPaint
{
	// RX-LOOK gjennomgang 14 Nov: the words the painted parts write for a state, as they write them. Each painter takes its lines
	// from here, so that the self-test and the receiver test read exactly what is drawn.
	FString StationLine(const FS47RxLive& Now);                    // the header's "STN SARO  EL ..  AZ .." line
	struct FBannerWords { FString State, Array, Rate, Mhz, Band, Length, Source; };
	FBannerWords BannerWords(const FS47RxLive& Now);
	FString FallSeconds(double Back);                               // the waterfall's "-4 s"
	FString FftDecibels(int32 Level);                               // the spectrum's "-90"
	FString FftMhz(const FS47RxLive& Now, int32 Tick);              // the spectrum's foot, ticks 0, 2 and 4
	// Every line the header, the banner, the waterfall, the spectrum and the tracking panel can write for this state (with the
	// tracking panel's rows for every arm and for the longest).
	TArray<FString> FrameWords(const FS47RxLive& Now, const FS47SkyAcquire* Sky);
	bool SelfTest(TArray<FString>& Lines);
}
