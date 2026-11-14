#pragma once
#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "S47SkySphere.h"
#include "Widgets/SLeafWidget.h"

class FS47SkyAcquire;

// RX-ACQUIRE-SKY-20261113 (-> M3): the receiver's ACQUIRE page, the sky chart on the tube. A 1986 vector terminal's drawing of
// FS47SkyAcquire: the catalogue as labelled points, one bearing line an antenna, the triangle the three arms' bands make, the
// reticle, the lock sequence's rings. Picture and input only; the rule is the model's and the job is the session's. It is a
// schematic on the terminal: nothing here reads or draws the game's star sky.
//
// RX-LOOK-20261113 (-> M3, M6; Tom 13 November: the receiver to look like his reference picture): the chart is the equatorial
// celestial sphere (S47SkySphere): the grid of right ascension and declination seen through an elliptical rim, real bright stars
// with their stick figures, the galactic plane as a stippled band, the array's window as a dashed outline, each arm's bearings as
// a fan from the rim that closes on the fix, and the readouts in the panel's corners (epoch, projection, mode, the reticle's place
// in the sky). The rule is untouched and still works in azimuth and elevation; the sphere is only where things stand on the
// glass. The stars and the hours of right ascension drift west as the game's clock runs.
//
// The picture closes in: as the pointer (or, with the keys, the reticle) nears the crossing, the sphere swells about the crossing,
// up to ZoomMost. On a sphere a degree of azimuth is fewer units across high up than it was on the old flat chart; closed in, the
// rule's lock circle is as large on the glass as it was there.
//
// Input: the reticle follows the pointer while it is inside the rim (a mouse, or a controller's right-stick pointer), and the arrow
// keys, the walk keys and a controller's d-pad steer it the way they point on the glass while the chart has the keys (it is the
// screen's first key target, and a click on it takes them).
//
// Nothing flashes: the search sweep, the stars' shimmer, the rings going out at the lock and the picture's one soft swell there
// all stand still with Calm flashes chosen. Every state is also said in words or shape (the mode in words, a dashed fan against a
// solid one, a diamond against a cross, a filled centre and a bar under the reticle on the crossing).
//
// Round 2 (13 November, the first look in the engine against Tom's reference):
// - Upright (S47SkySphere::FitView): the pole on the rim's top with air round the oval, the declinations written round the rim
//   where each circle leaves it, the hours of right ascension in a row under it (each under the place its meridian crosses the
//   lower part of the picture). The cost is scale: the open picture is two thirds as large a degree, and ZoomMost is 3.6 for it.
// - A full sky: a faint field under the table's stars (more of it the further the picture has closed in), the table's stars in
//   three classes (a white-hot point in a halo with a glint, a bright point in a halo, a plain point), the figures' lines
//   near-white and clearly over the graticule, the galactic plane a band that reads at a glance.
// - The bearings are thin lines at every stage, never a wedge: no glow, every other line starts further out, on the lock all of
//   them stop short of the reticle's rings, and rungs cross each arm's fan. The fan that comes in from high up is drawn wider
//   than the rule's seven degrees (the sphere would squeeze it to a strip); each line still passes through its own point of the
//   band and the middle line is the rule's own. The search sweep is a bead running round the rim, not a hand that looks like a
//   fourth fan.
// - The reticle is amber and the brightest thing on the glass at every stage, larger, with corner brackets, three rings and
//   hairs. Being on the crossing is said by shape (the filled centre, the bar that fills), no longer by the colour.
// - No word under the next step's size (S47RxLook::TextFloor): the corner blocks are three short lines, the minor catalogue's
//   names stand only near the reticle or closed in, and each arm is its number at its fan (RX-LOOK gjennomgang 14 Nov: 1, 2
//   and 3, as the tracking panel's DELAY 1 to 3: the letters A to C had a lone capital C, which is not Freeform's).
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
		// RX-LOOK-20261113: the station's sidereal time in degrees, from the game's clock (none: the sky stands), and the card's
		// source for the corner readout.
		SLATE_ARGUMENT(TFunction<double()>, Sidereal)
		SLATE_ARGUMENT(FString, Source)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(716, 440); }
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& G, const FKeyEvent& E) override;
	virtual FReply OnKeyUp(const FGeometry& G, const FKeyEvent& E) override;
	virtual void OnFocusLost(const FFocusEvent& E) override;
	virtual FReply OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override;
	virtual FReply OnMouseMove(const FGeometry& G, const FPointerEvent& E) override;
	virtual void OnMouseLeave(const FPointerEvent& E) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& G, const FPointerEvent& E) const override;
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const override;

	// The picture inside the widget: the sphere's view through its rim (with room for the declinations round it, the pole's over
	// it and the hours under it), and how far it has closed in about a place on the glass (Focus: where the crossing stands on
	// the open picture).
	struct FPlot { S47SkySphere::FView View; FVector2f Focus = FVector2f::ZeroVector; float Zoom = 1.f; };
	static constexpr float SideRoom = 58.f, FootRoom = 26.f, TopRoom = 24.f;
	static constexpr float ZoomMost = 3.6f;
	// Degrees off the crossing on the open picture: closed in all the way, and not at all. (RX-LOOK gjennomgang 14 Nov: 6, was 4, so
	// that a reticle anywhere inside the rule's lock circle stands on a picture closed in all the way; see ZoomForTrue.)
	static constexpr double ZoomNear = 6., ZoomFar = 24.;
	static FPlot PlotFor(const FVector2f& Size);
	// False when the direction is behind the picture (Out is then far off the glass).
	static bool Place(const FPlot& P, const FVector2D& AzEl, FVector2f& Out);
	static FVector2f ToLocal(const FPlot& P, const FVector2D& AzEl);
	static FVector2D ToSky(const FPlot& P, const FVector2f& Local);
	static bool Contains(const FPlot& P, const FVector2f& Local);
	// How far the picture closes in for a pointer or a reticle so many of the rule's degrees off the crossing.
	static float ZoomFor(double DegreesOff);
	// RX-LOOK gjennomgang 14 Nov: the same curve for a reticle so many of the rule's degrees off the crossing (the keys, the assist):
	// the zoom at which the reticle stands where a pointer would have put it, so that its distance on the glass (true degrees times
	// the zoom) grows with the true distance and a reticle steered toward the crossing never seems to back off it. (The old curve
	// read the true distance as an open one: steering from 24 to 13 degrees took the reticle from 24 to some 32 open degrees off.)
	static float ZoomForTrue(double TrueOff);
	// One frame of the picture closing in or opening: eased toward Want as before; with the reticle moved by the keys or the assist
	// (OffWas, OffNow: its true distance last frame and now, OffWas below zero: none), never so fast that its distance on the glass
	// grows while it nears the crossing, nor shrinks while it goes away.
	static float ZoomStep(float Was, float Want, double Dt, double OffWas, double OffNow);
	// RX-LOOK gjennomgang 14 Nov: the reticle's inner mark, the rule's lock circle as it lies on the glass (an ellipse: a degree of
	// azimuth is cos(elevation) degrees of arc), so that the crossing is inside it exactly when the fix can be taken. Open, it is
	// drawn at the inner ring's old size (Ring, its mean radius) in the ellipse's shape; it comes to its true size as the picture
	// closes in (all of it at ZoomMost, which the picture is whenever the crossing can be that near). Out: the closed path.
	static void LockMark(const FPlot& P, const FVector2D& Reticle, float Ring, TArray<FVector2f>& Out);
	// The four ways a key can steer on the glass (-1, 0 or 1 across and up).
	static FVector2D SteerOf(const FKey& Key);
	// The step in azimuth and elevation (a unit vector) that moves the reticle that way on the glass, where it stands now: for
	// the page's own step keys.
	FVector2D SkyStep(const FVector2D& GlassDir) const;
	// The open picture as last painted (before the first frame: the picture at the chart's usual size), for the tracking panel's
	// bull's-eye, which lies the way the chart does.
	S47SkySphere::FView OpenView() const { return (ShownSize.X > 0 ? Shown : PlotFor(FVector2f(716, 440))).View; }
	// RX-LOOK gjennomgang 14 Nov: the words the chart writes for the rule's state, as it writes them (the self-test and the
	// receiver test read the same): the corner blocks, a declination round the rim, an hour under it, and every word it can
	// write at all for this state (the corners, every hour and declination, the pole, the catalogue's names, the arms, the rose).
	struct FCorners { FString TopLeft[3], TopRight[2], Foot[3]; };
	static FCorners CornerWords(const FS47SkyAcquire& K, double Lst, const FString& Source);
	static FString DecWords(int32 Circle);
	static FString HourWords(int32 Hour);
	static TArray<FString> AllWords(const FS47SkyAcquire& K, double Lst, const FString& Source);
	static bool SelfTest(TArray<FString>& Lines);

private:
	TSharedPtr<FS47SkyAcquire> Sky;
	TFunction<void(double)> StepFn;
	TFunction<bool()> CalmFn;
	TFunction<void(int32)> KeyFn;
	TFunction<double()> SiderealFn;
	FString SourceName;
	TSet<FKey> HeldKeys;
	FVector2f HoverLocal = FVector2f::ZeroVector;
	bool bHover = false;
	mutable double LastPaint = 0;
	// as last painted: the pointer is read through the picture the operator sees
	mutable FPlot Shown;
	mutable FVector2f ShownSize = FVector2f::ZeroVector;
	mutable float ZoomNow = 1.f;
	mutable double ZoomOffWas = -1.; // the reticle's true distance last frame while the keys or the assist moved it (else below zero)
	// the lock moment (for the rings going out and the picture's one swell), and the reticle's fading trail
	mutable int32 PhaseSeen = -1;
	mutable double LockedAt = -1e9;
	mutable TArray<FVector2f> Trail;
	mutable TArray<double> TrailAt;
	FVector2D SteerNow() const;
	bool Steerable() const;
	FPlot PlotNow(const FVector2f& Size) const; // the picture as last painted at this size, else the open one
};
