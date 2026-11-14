#include "SS47SkyChart.h"
#include "S47SkyAcquire.h"
#include "S47RxLook.h"
#include "S47Keys.h"
#include "S47Kit.h"
#include "SS47Widgets.h"
#include "Brushes/SlateColorBrush.h"
#include "Rendering/DrawElements.h"

// RX-ACQUIRE-SKY-20261113 (-> M3) and RX-LOOK-20261113 (-> M3, M6): see SS47SkyChart.h.
namespace
{
	// The workstation's phosphor (S47FreeformTerminal's palette, which keeps its own copies).
	const FLinearColor S47SkyGreen = S47Kit::Css(0x8cffa4), S47SkyAmber = S47Kit::Css(0xffb84a), S47SkyLine = S47Kit::Css(0x3f9a5a), S47SkyHot = S47Kit::Css(0xfff0d0);
	const FSlateBrush* S47SkyFill() { static const FSlateColorBrush B(FLinearColor::White); return &B; }
	FLinearColor S47SkyFade(FLinearColor C, float A) { C.A = FMath::Clamp(A, 0.f, 1.f); return C; }
	// (round 2) nothing on the chart is written smaller than the next step's line
	constexpr int32 S47SkyLetters = S47RxLook::TextFloor;
	void S47SkyText(FSlateWindowElementList& Out, int32 L, const FGeometry& G, const FString& S, float X, float Y, int32 Size, const FLinearColor& C)
	{
		FSlateDrawElement::MakeText(Out, L, G.ToPaintGeometry(FVector2f(420.f, float(Size) + 6), FSlateLayoutTransform(FVector2f(X, Y))), S, S47Style::Mono(Size), ESlateDrawEffect::None, C);
	}
	void S47SkyLines(FSlateWindowElementList& Out, int32 L, const FGeometry& G, const TArray<FVector2f>& P, const FLinearColor& C, float W = 1.5f)
	{
		if (P.Num() >= 2) FSlateDrawElement::MakeLines(Out, L, G.ToPaintGeometry(), P, ESlateDrawEffect::None, C, true, W);
	}
	// RX-LOOK-20261113: a bright stroke as the beam leaves it: a wide faint pass, a middle one and the thin bright core (a little
	// wider where it is brighter).
	void S47SkyGlow(FSlateWindowElementList& Out, int32 L, const FGeometry& G, const TArray<FVector2f>& P, const FLinearColor& C, float W = 1.6f)
	{
		const float Wide = S47RxLook::Beam(W, C.A);
		S47SkyLines(Out, L, G, P, S47SkyFade(C, C.A * .10f), Wide * 4.2f);
		S47SkyLines(Out, L, G, P, S47SkyFade(C, C.A * .26f), Wide * 2.1f);
		S47SkyLines(Out, L, G, P, C, Wide);
	}
	// Round 2: one more step of it for the reticle alone, the brightest thing on the glass: a wider, fainter pass under the
	// glow, and the stroke's middle near white, as a tube's phosphor is where the beam stands longest.
	void S47SkyGlowHot(FSlateWindowElementList& Out, int32 L, const FGeometry& G, const TArray<FVector2f>& P, const FLinearColor& C, float W = 1.6f)
	{
		const float Wide = S47RxLook::Beam(W, C.A);
		S47SkyLines(Out, L, G, P, S47SkyFade(C, C.A * .05f), Wide * 7.5f);
		S47SkyGlow(Out, L, G, P, C, W);
		S47SkyLines(Out, L, G, P, S47SkyFade(S47SkyHot, C.A * .8f), FMath::Max(1.f, Wide * .4f));
	}
	// A dashed path: every other stretch of Dash units left out, the dashes carried round the path's corners.
	void S47SkyDashPath(FSlateWindowElementList& Out, int32 L, const FGeometry& G, const TArray<FVector2f>& P, const FLinearColor& C, float Dash = 7, float W = 1.5f)
	{
		float Phase = 0; // how far into the current dash-and-gap the path is
		TArray<FVector2f> Piece;
		for (int32 I = 0; I + 1 < P.Num(); ++I)
		{
			const float Len = FVector2f::Distance(P[I], P[I + 1]);
			if (Len < 1e-3f) continue;
			const FVector2f D = (P[I + 1] - P[I]) / Len;
			float At = 0;
			while (At < Len)
			{
				const bool bInk = Phase < Dash;
				const float Step = FMath::Min(Len - At, (bInk ? Dash : Dash * 2) - Phase);
				if (bInk) { if (Piece.IsEmpty()) Piece.Add(P[I] + D * At); Piece.Add(P[I] + D * (At + Step)); }
				At += Step; Phase += Step;
				if (Phase >= Dash && !Piece.IsEmpty()) { S47SkyLines(Out, L, G, Piece, C, W); Piece.Reset(); }
				if (Phase >= Dash * 2 - 1e-4f) Phase = 0;
			}
		}
		S47SkyLines(Out, L, G, Piece, C, W);
	}
	void S47SkyBox(FSlateWindowElementList& Out, int32 L, const FGeometry& G, FVector2f At, FVector2f Size, const FLinearColor& C, bool bSolid)
	{
		if (bSolid) { FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(Size, FSlateLayoutTransform(At)), S47SkyFill(), ESlateDrawEffect::None, C); return; }
		S47SkyLines(Out, L, G, { At, At + FVector2f(Size.X, 0), At + Size, At + FVector2f(0, Size.Y), At }, C, 1.5f);
	}
	// A point of light: a square about a place.
	void S47SkyDot(FSlateWindowElementList& Out, int32 L, const FGeometry& G, FVector2f At, float Side, const FLinearColor& C)
	{
		S47SkyBox(Out, L, G, At - FVector2f(Side * .5f, Side * .5f), FVector2f(Side, Side), C, true);
	}
	// A ring's path.
	TArray<FVector2f> S47SkyRingPath(FVector2f At, float Radius)
	{
		constexpr int32 Steps = 40;
		TArray<FVector2f> P; P.Reserve(Steps + 1);
		for (int32 I = 0; I <= Steps; ++I) { const float A = 2 * PI * I / Steps; P.Add(At + FVector2f(FMath::Cos(A), FMath::Sin(A)) * Radius); }
		return P;
	}
	// A ring as a polyline; bDashed: every other piece left out.
	void S47SkyRing(FSlateWindowElementList& Out, int32 L, const FGeometry& G, FVector2f At, float Radius, const FLinearColor& C, float W, bool bDashed = false)
	{
		if (!bDashed) { S47SkyLines(Out, L, G, S47SkyRingPath(At, Radius), C, W); return; }
		constexpr int32 Steps = 40;
		for (int32 I = 0; I < Steps; I += 2)
		{
			const float A = 2 * PI * I / Steps, B = 2 * PI * (I + 1) / Steps;
			S47SkyLines(Out, L, G, { At + FVector2f(FMath::Cos(A), FMath::Sin(A)) * Radius, At + FVector2f(FMath::Cos(B), FMath::Sin(B)) * Radius }, C, W);
		}
	}
	// The place a length along a path, counted from its first point. False: the path is shorter.
	bool S47SkyAlong(const TArray<FVector2f>& P, int32 From, int32 Count, float Length, FVector2f& OutAt)
	{
		float Gone = 0;
		for (int32 I = From; I + 1 < From + Count; ++I)
		{
			const float Len = FVector2f::Distance(P[I], P[I + 1]);
			if (Gone + Len >= Length && Len > 1e-4f) { OutAt = P[I] + (P[I + 1] - P[I]) * ((Length - Gone) / Len); return true; }
			Gone += Len;
		}
		return false;
	}
	// RX-LOOK-20261113: the table's stars, the galactic plane's specks and (round 2) the faint field as places of date (the sky of
	// the game's spring), each a direction with x to the equinox and z to the pole, worked out once. A frame turns them with the
	// sidereal time (S47SkySphere::Turned): no angle is worked out again.
	struct FS47ChartPlace { FVector Unit = FVector(1, 0, 0); float Bright = 1.f; };
	const TArray<FS47ChartPlace>& S47SkyStarPlaces()
	{
		static const TArray<FS47ChartPlace> List = []
		{
			TArray<FS47ChartPlace> Made;
			for (const S47SkySphere::FStar& St : S47SkySphere::Stars)
			{
				double Ra = St.RaHours * 15., Dec = St.Dec;
				S47SkySphere::FromJ2000(Ra, Dec);
				FS47ChartPlace P; P.Unit = S47SkySphere::Dir(Ra, Dec); P.Bright = St.Mag; // (for a star: its magnitude)
				Made.Add(P);
			}
			return Made;
		}();
		return List;
	}
	const TArray<FS47ChartPlace>& S47SkyDustPlaces()
	{
		static const TArray<FS47ChartPlace> List = []
		{
			TArray<FS47ChartPlace> Made;
			for (int32 I = 0; I < S47SkySphere::DustCount; ++I)
			{
				const S47SkySphere::FDust D = S47SkySphere::Dust(I);
				double Ra = 0, Dec = 0;
				S47SkySphere::GalToEq(D.Lon, D.Lat, Ra, Dec);
				S47SkySphere::FromJ2000(Ra, Dec);
				FS47ChartPlace P; P.Unit = S47SkySphere::Dir(Ra, Dec); P.Bright = D.Bright;
				Made.Add(P);
			}
			return Made;
		}();
		return List;
	}
	const TArray<FS47ChartPlace>& S47SkyFaintPlaces()
	{
		static const TArray<FS47ChartPlace> List = []
		{
			TArray<FS47ChartPlace> Made;
			for (int32 I = 0; I < S47SkySphere::FieldCount; ++I)
			{
				const S47SkySphere::FFaint F = S47SkySphere::Field(I);
				FS47ChartPlace P; P.Unit = S47SkySphere::Dir(F.Ra, F.Dec); P.Bright = F.Bright;
				Made.Add(P);
			}
			return Made;
		}();
		return List;
	}
}

void SS47SkyChart::Construct(const FArguments& Args)
{
	Sky = Args._Model; StepFn = Args._OnStep; CalmFn = Args._Calm; KeyFn = Args._OnKey; SiderealFn = Args._Sidereal; SourceName = Args._Source;
	ForceVolatile(true);
}

SS47SkyChart::FPlot SS47SkyChart::PlotFor(const FVector2f& Size)
{
	// The declinations stand round the rim (the pole's over it), the hours of right ascension under it.
	const FVector2D Semi(FMath::Max(40., double(Size.X) * .5 - SideRoom), FMath::Max(30., (double(Size.Y) - FootRoom - TopRoom) * .5));
	FPlot P;
	P.View = S47SkySphere::FitView(FVector2D(double(Size.X) * .5, TopRoom + Semi.Y), Semi);
	P.Focus = FVector2f(P.View.Rim);
	return P;
}

SS47SkyChart::FPlot SS47SkyChart::PlotNow(const FVector2f& Size) const
{
	return ShownSize.X > 0 && Size.Equals(ShownSize, .5f) ? Shown : PlotFor(Size);
}

bool SS47SkyChart::Place(const FPlot& P, const FVector2D& AzEl, FVector2f& Out)
{
	FVector2D Open;
	if (!S47SkySphere::ProjectAzEl(P.View, AzEl, Open)) { Out = FVector2f(P.View.Rim) + FVector2f(1e5f, 1e5f); return false; }
	Out = P.Focus + (FVector2f(Open) - P.Focus) * P.Zoom;
	return true;
}

FVector2f SS47SkyChart::ToLocal(const FPlot& P, const FVector2D& AzEl)
{
	FVector2f Out;
	Place(P, AzEl, Out);
	return Out;
}

FVector2D SS47SkyChart::ToSky(const FPlot& P, const FVector2f& Local)
{
	const FVector2f Open = P.Focus + (Local - P.Focus) / FMath::Max(P.Zoom, .1f);
	return S47SkySphere::UnprojectAzEl(P.View, FVector2D(Open));
}

bool SS47SkyChart::Contains(const FPlot& P, const FVector2f& Local)
{
	return S47SkySphere::Inside(P.View, FVector2D(Local));
}

float SS47SkyChart::ZoomFor(double DegreesOff)
{
	if (!FMath::IsFinite(DegreesOff)) return 1.f;
	const double X = FMath::Clamp((DegreesOff - ZoomNear) / (ZoomFar - ZoomNear), 0., 1.);
	return 1.f + (ZoomMost - 1.f) * float(1. - X * X * (3. - 2. * X));
}

FVector2D SS47SkyChart::SteerOf(const FKey& Key)
{
	if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left || S47Keys::Is(TEXT("left"), Key)) return FVector2D(-1, 0);
	if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right || S47Keys::Is(TEXT("right"), Key)) return FVector2D(1, 0);
	if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || S47Keys::Is(TEXT("forward"), Key)) return FVector2D(0, 1);
	if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || S47Keys::Is(TEXT("back"), Key)) return FVector2D(0, -1);
	return FVector2D::ZeroVector;
}

FVector2D SS47SkyChart::SteerNow() const
{
	FVector2D V = FVector2D::ZeroVector;
	for (const FKey& Key : HeldKeys) V += SteerOf(Key);
	return FVector2D(FMath::Clamp(V.X, -1., 1.), FMath::Clamp(V.Y, -1., 1.));
}

FVector2D SS47SkyChart::SkyStep(const FVector2D& GlassDir) const
{
	if (!Sky.IsValid()) return FVector2D::ZeroVector;
	const FPlot P = ShownSize.X > 0 ? Shown : PlotFor(FVector2f(716, 440));
	return S47SkySphere::SteerOnGlass(P.View, Sky->ReticleAt(), GlassDir);
}

bool SS47SkyChart::Steerable() const
{
	return Sky.IsValid() && (Sky->State() == FS47SkyAcquire::EPhase::Search || Sky->State() == FS47SkyAcquire::EPhase::NoBearings);
}

FReply SS47SkyChart::OnKeyDown(const FGeometry&, const FKeyEvent& E)
{
	// Only the steering keys, and only while there is something to steer: every other key goes on to the screen as before.
	if (!Steerable() || SteerOf(E.GetKey()).IsZero()) return FReply::Unhandled();
	if (!E.IsRepeat() && !HeldKeys.Contains(E.GetKey()) && KeyFn) KeyFn(0);
	HeldKeys.Add(E.GetKey());
	return FReply::Handled();
}

FReply SS47SkyChart::OnKeyUp(const FGeometry&, const FKeyEvent& E)
{
	return HeldKeys.Remove(E.GetKey()) > 0 ? FReply::Handled() : FReply::Unhandled();
}

void SS47SkyChart::OnFocusLost(const FFocusEvent& E)
{
	HeldKeys.Reset();
	SLeafWidget::OnFocusLost(E);
}

FReply SS47SkyChart::OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E)
{
	if (E.GetEffectingButton() != EKeys::LeftMouseButton || !Steerable()) return FReply::Unhandled();
	const FVector2f Local(G.AbsoluteToLocal(E.GetScreenSpacePosition()));
	const FPlot P = PlotNow(FVector2f(G.GetLocalSize()));
	if (!Contains(P, Local)) return FReply::Unhandled();
	HoverLocal = Local; bHover = true;
	if (KeyFn) KeyFn(Sky->HasBearings() ? 0 : 1);
	Sky->PointAt(ToSky(P, Local));
	// a click on the chart gives it the keys
	return FReply::Handled().SetUserFocus(AsShared(), EFocusCause::Mouse);
}

FReply SS47SkyChart::OnMouseMove(const FGeometry& G, const FPointerEvent& E)
{
	if (!Sky.IsValid()) return FReply::Unhandled();
	const FVector2f Local(G.AbsoluteToLocal(E.GetScreenSpacePosition()));
	const FPlot P = PlotNow(FVector2f(G.GetLocalSize()));
	bHover = Contains(P, Local);
	HoverLocal = Local;
	if (bHover && Steerable()) Sky->PointAt(ToSky(P, Local));
	else Sky->ClearPointer();
	return FReply::Unhandled();
}

void SS47SkyChart::OnMouseLeave(const FPointerEvent& E)
{
	bHover = false;
	if (Sky.IsValid()) Sky->ClearPointer();
	SLeafWidget::OnMouseLeave(E);
}

FCursorReply SS47SkyChart::OnCursorQuery(const FGeometry&, const FPointerEvent&) const
{
	// inside the rim the tube draws its own pointer mark while the search is on
	return bHover && Steerable() ? FCursorReply::Cursor(EMouseCursor::None) : FCursorReply::Unhandled();
}

int32 SS47SkyChart::OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const
{
	using FSky = FS47SkyAcquire;
	namespace Sph = S47SkySphere;
	using S47RxLook::Num;
	const FVector2f Sz(G.GetLocalSize());
	const double Now = FPlatformTime::Seconds();
	const double Dt = LastPaint > 0 ? FMath::Clamp(Now - LastPaint, 0., .1) : 0.;
	LastPaint = Now;
	if (ShownSize.X <= 0 || !Sz.Equals(ShownSize, .5f)) { Shown = PlotFor(Sz); ShownSize = Sz; ZoomNow = 1.f; Trail.Reset(); TrailAt.Reset(); }
	FPlot Pl = Shown;
	Pl.Zoom = 1.f;
	const FVector2f Rim(Pl.View.Rim), Semi(Pl.View.Semi);
	// the rim itself
	TArray<FVector2f> RimPath; RimPath.Reserve(73);
	for (int32 I = 0; I <= 72; ++I) { const float A = 2 * PI * I / 72; RimPath.Add(Rim + FVector2f(FMath::Cos(A) * Semi.X, FMath::Sin(A) * Semi.Y)); }
	if (!Sky.IsValid())
	{
		S47SkyLines(Out, L, G, RimPath, S47SkyLine, 1.5f);
		return L + 1;
	}
	FSky& K = *Sky;
	const bool bCalm = CalmFn && CalmFn();
	const double Lst = SiderealFn ? SiderealFn() : 0.;
	// the keys steer the way they point on the glass, at the rule's own rate
	K.Steer(Steerable() ? Sph::SteerOnGlass(Pl.View, K.ReticleAt(), SteerNow()) : FVector2D::ZeroVector);
	// the picture closes in about the crossing as the pointer nears it (with the keys: as the reticle does), all the way while the
	// fix is taken, and opens again once it is held. The pointer is then read through the picture as it stands.
	{
		double Off = -1.;
		if (K.HasBearings() && K.State() == FSky::EPhase::Search)
		{
			Off = bHover && K.HasPointer() ? FVector2D::Distance(FSky::ClampToChart(Sph::UnprojectAzEl(Pl.View, FVector2D(HoverLocal))), K.Cross()) : K.Distance();
		}
		const float Want = K.State() == FSky::EPhase::Locking ? ZoomMost : Off >= 0 ? ZoomFor(Off) : 1.f;
		ZoomNow = FMath::Clamp(ZoomNow + (Want - ZoomNow) * float(FMath::Min(1., Dt * (Want > ZoomNow ? 5. : 1.6))), 1.f, ZoomMost);
		FVector2D Crossing;
		Pl.Focus = Sph::ProjectAzEl(Pl.View, K.Cross(), Crossing) ? FVector2f(Crossing) : Rim;
		Pl.Zoom = ZoomNow;
	}
	if (bHover && Steerable() && K.HasPointer()) K.PointAt(ToSky(Pl, HoverLocal));
	if (StepFn) StepFn(Dt);
	else K.Step(Dt);
	Shown.Focus = Pl.Focus; Shown.Zoom = Pl.Zoom;

	const FSky::EPhase State = K.State();
	const bool bLocked = State == FSky::EPhase::Locked, bLocking = State == FSky::EPhase::Locking, bSearch = State == FSky::EPhase::Search;
	const bool bBearings = K.HasBearings();
	// the lock moment, seen here: the rings go out from it and the picture swells once (not for a fix that was already held)
	if (PhaseSeen == int32(FSky::EPhase::Locking) && bLocked) LockedAt = Now;
	PhaseSeen = int32(State);
	const double SinceLock = Now - LockedAt;
	// A slow breath for small marks (never the picture): standing still with Calm flashes chosen.
	auto Breath = [&](double Hz, double Phase = 0.) { return bCalm ? 1.f : .72f + .28f * float(FMath::Sin((Now * Hz + Phase) * 2 * PI)); };
	// every stroke a little brighter through the one soft swell at the lock
	const float Gain = 1.f + .4f * S47RxLook::Bloom(SinceLock, bCalm);
	// a band of the picture slips a unit sideways now and then, as a tube's does
	auto Slip = [&](FVector2f Spot) { Spot.X += S47RxLook::Jitter(Now, Spot.Y, bCalm); return Spot; };
	auto At = [&](const FVector2D& AzEl, FVector2f& OutP) { if (!Place(Pl, AzEl, OutP)) return false; OutP = Slip(OutP); return true; };
	// a direction of the hour-angle frame (the grid, and a star's place turned with the sidereal time)
	auto AtDir = [&](const FVector& Toward, FVector2f& OutP)
	{
		FVector2D Open;
		if (!Sph::Project(Pl.View, Toward, Open)) return false;
		OutP = Slip(Pl.Focus + (FVector2f(Open) - Pl.Focus) * Pl.Zoom);
		return true;
	};
	auto AtHour = [&](double Ha, double Dec, FVector2f& OutP) { return AtDir(Sph::Dir(Ha, Dec), OutP); };
	auto InRim = [&](const FVector2f& Spot, double Shrink) { return Sph::Inside(Pl.View, FVector2D(Spot), Shrink); };
	// A path of the picture cut to the rim. Style: 0 plain, 1 bright (the beam's glow), 2 dashed. Hits: where it meets the rim.
	auto Path = [&](const TArray<FVector2f>& Pts, const FLinearColor& C, float W, int32 Layer, int32 Style, TArray<FVector2f>* Hits)
	{
		TArray<FVector2f> Run;
		auto Flush = [&]
		{
			if (Run.Num() >= 2)
			{
				if (Style == 1) S47SkyGlow(Out, Layer, G, Run, C, W);
				else if (Style == 2) S47SkyDashPath(Out, Layer, G, Run, C, 7, W);
				else S47SkyLines(Out, Layer, G, Run, C, W);
			}
			Run.Reset();
		};
		for (int32 I = 0; I + 1 < Pts.Num(); ++I)
		{
			FVector2D A(Pts[I]), B(Pts[I + 1]);
			bool bCutA = false, bCutB = false;
			if (!Sph::CutToRim(Pl.View, A, B, bCutA, bCutB)) { Flush(); continue; }
			if (Run.IsEmpty() || bCutA) { Flush(); Run.Add(FVector2f(A)); }
			Run.Add(FVector2f(B));
			if (Hits && bCutA) Hits->Add(FVector2f(A));
			if (Hits && bCutB) Hits->Add(FVector2f(B));
			if (bCutB) Flush();
		}
		Flush();
	};
	// A curve of the sphere: Count points, each from PointAt (false: that point is behind the picture, and the curve breaks there).
	auto Curve = [&](int32 Count, TFunctionRef<bool(int32, FVector2f&)> PointAt, const FLinearColor& C, float W, int32 Layer, int32 Style, TArray<FVector2f>* Hits)
	{
		TArray<FVector2f> Pts; Pts.Reserve(Count);
		for (int32 I = 0; I < Count; ++I)
		{
			FVector2f Spot;
			if (PointAt(I, Spot)) { Pts.Add(Spot); continue; }
			Path(Pts, C, W, Layer, Style, Hits);
			Pts.Reset();
		}
		Path(Pts, C, W, Layer, Style, Hits);
	};

	// ---- where the reticle and the crossing stand, and how large the reticle is (the names and the bearings keep clear of it) ----
	FVector2f Ret = Rim, Xc = Rim;
	At(K.ReticleAt(), Ret);
	At(K.Cross(), Xc);
	constexpr float LockGap = 46.f; // the reticle's half-width once the fix is held; its three rings are 0.30, 0.50 and 0.72 of it
	const float Near = bBearings ? float(K.Closeness()) : 0.f;
	const float Gap = bLocked ? LockGap : FMath::Lerp(52.f, 34.f, Near * Near);

	// ---- words on the chart (round 2): none under the next step's size. The corner blocks first, so that nothing is written
	// over them: what the picture is and where the reticle is in the sky ----
	constexpr int32 Letters = S47SkyLetters;
	constexpr float Pitch = 21.f;
	const FLinearColor Quiet = S47SkyGreen * .85f, LabelTone = S47SkyGreen * .85f;
	const TCHAR* Word = State == FSky::EPhase::NoBearings ? TEXT("NO BEARINGS") : bLocked ? TEXT("FIX HELD") : bLocking ? TEXT("LOCKING") : !K.Steady() ? TEXT("ARRAY TURNING")
		: K.HoldProgress() > 0 ? TEXT("ON THE CROSSING") : TEXT("SEARCHING");
	const FVector2D Aim = K.ReticleAt();
	double AimRa = 0, AimDec = 0;
	Sph::AzElToRaDec(Aim, Lst, AimRa, AimDec);
	Sph::ToB1950(AimRa, AimDec);
	const FString TopLeft[3] = { FString(TEXT("EPOCH B1950.0")), FString(TEXT("MODE ")) + (bLocking ? FString::Printf(TEXT("LOCKING %s %%"), *Num(K.LockProgress() * 100., 0, 2)) : FString(Word)), FString(TEXT("SRC ")) + SourceName.Left(14) };
	const FString TopRight[2] = { bBearings ? FString::Printf(TEXT("BEARINGS %s/%s"), *Num(K.InCount(), 0, 2, false, true), *Num(K.OnCount(), 0, 2, false, true)) : FString(TEXT("BEARINGS --/--")),
		bBearings ? FString::Printf(TEXT("SPREAD %s\u00b0"), *Num(K.MeanOff(), 1)) : FString(TEXT("SPREAD --")) };
	const FString Foot[3] = { FString::Printf(TEXT("AZ %s EL %s"), *Num(Aim.X, 1, 5, false, true), *Num(Aim.Y, 1, 4, false, true)), FString(TEXT("RA ")) + Sph::RaText(AimRa), FString(TEXT("DEC ")) + Sph::DecText(AimDec) };
	TArray<FSlateRect, TInlineAllocator<48>> Taken;
	auto Free = [&](const FSlateRect& Area)
	{
		if (Area.Left < 0 || Area.Top < 0 || Area.Right > Sz.X || Area.Bottom > Sz.Y) return false;
		for (const FSlateRect& Other : Taken) if (FSlateRect::DoRectanglesIntersect(Other, Area)) return false;
		return true;
	};
	{
		int32 LeftMost = 0, RightMost = 0, FootMost = 0;
		for (const FString& Each : TopLeft) LeftMost = FMath::Max(LeftMost, Each.Len());
		for (const FString& Each : TopRight) RightMost = FMath::Max(RightMost, Each.Len());
		for (const FString& Each : Foot) FootMost = FMath::Max(FootMost, Each.Len());
		Taken.Add(FSlateRect(0, 0, 10 + S47RxLook::TextWide(LeftMost, Letters), 6 + 3 * Pitch));
		Taken.Add(FSlateRect(Sz.X - 12 - S47RxLook::TextWide(RightMost, Letters), 0, Sz.X, 6 + 2 * Pitch));
		Taken.Add(FSlateRect(0, Sz.Y - 6 - 3 * Pitch, 10 + S47RxLook::TextWide(FootMost, Letters), Sz.Y));
		Taken.Add(FSlateRect(Sz.X - 96, Sz.Y - 100, Sz.X, Sz.Y)); // the rose
	}
	// A word just outside the rim where a line of the grid leaves it, clear of every word already there. False: no room for it.
	auto RimLabel = [&](const FVector2f& Hit, const FString& Label)
	{
		const FVector2f Outward = FVector2f((Hit.X - Rim.X) / (Semi.X * Semi.X), (Hit.Y - Rim.Y) / (Semi.Y * Semi.Y)).GetSafeNormal();
		const float Wide = S47RxLook::TextWide(Label.Len(), Letters), High = 16.f;
		const FVector2f Mid = Hit + Outward * (FMath::Abs(Outward.X) * Wide * .5f + FMath::Abs(Outward.Y) * High * .5f + 5.f);
		const FSlateRect Area(Mid.X - Wide * .5f, Mid.Y - High * .5f, Mid.X + Wide * .5f, Mid.Y + High * .5f);
		if (!Free(Area)) return false;
		Taken.Add(Area);
		S47SkyText(Out, L + 6, G, Label, Area.Left, Mid.Y - 12.f, Letters, LabelTone);
		return true;
	};

	// ---- the sphere's grid: circles of declination, which stand, and the hours of right ascension, which turn with the sky ----
	double MidHa = 0, MidDec = 0;
	Sph::LonLat(Pl.View.Centre, MidHa, MidDec);
	const FLinearColor GridTone = S47SkyFade(S47SkyLine, .46f * Gain);
	TArray<FVector2f> RimHits;
	// (closed in, the grid is drawn finer: every five degrees and every twenty minutes, the whole ones as before)
	const bool bFine = Pl.Zoom > 1.6f;
	const FLinearColor FineTone = S47SkyFade(S47SkyLine, .3f * Gain);
	for (int32 Pass = 0; Pass < (bFine ? 2 : 1); ++Pass)
	{
		// (the fifteens first, so that their declinations are the ones written where two would stand on each other)
		for (int32 Circle = -75; Circle <= 75; Circle += 5)
		{
			if ((Circle % 15 == 0) != (Pass == 0)) continue;
			RimHits.Reset();
			// (the points stand half a step off the hemisphere's edge, so the circle always crosses the rim inside a stretch of it)
			Curve(80, [&](int32 I, FVector2f& Spot) { return AtHour(MidHa - 118.5 + 3. * I, double(Circle), Spot); }, Circle == 0 ? S47SkyFade(S47SkyLine, .9f * Gain) : Circle % 15 ? FineTone : GridTone, 1.f, L, 0, &RimHits);
			// its declination round the rim, where the circle leaves the picture on the right
			const FVector2f* Best = nullptr;
			for (const FVector2f& Hit : RimHits) if (Hit.X > Rim.X + 4 && (!Best || Hit.X > Best->X)) Best = &Hit;
			if (Best) RimLabel(*Best, Circle == 0 ? FString(TEXT("0\u00b0")) : Num(double(Circle), 0, 0, true) + TEXT("\u00b0"));
		}
	}
	{
		// the pole, while it stands on the rim (the open picture)
		FVector2f PoleAt;
		if (AtDir(FVector(0, 0, 1), PoleAt) && InRim(PoleAt, 1.02)) RimLabel(FVector2f(Rim.X, Rim.Y - Semi.Y), TEXT("+90\u00b0"));
	}
	{
		// the hours: each under the place its meridian crosses the lower part of the picture
		const float HourLine = Rim.Y + Semi.Y * .62f, HourWide = S47RxLook::TextWide(3, Letters);
		TArray<FVector2f> Meridian; Meridian.Reserve(41);
		for (int32 Third = 0; Third < 72; ++Third)
		{
			// (every twenty minutes of right ascension; the whole hours always, the thirds only closed in)
			const int32 Hour = Third / 3;
			const bool bWhole = Third % 3 == 0;
			if (!bWhole && !bFine) continue;
			const double FromMid = Sph::Wrap180(Lst - Third * 5. - MidHa);
			if (FMath::Abs(FromMid) > 112.) continue;
			const FLinearColor Tone = !bWhole ? FineTone : Hour % 6 == 0 ? S47SkyFade(S47SkyLine, .8f * Gain) : GridTone;
			Meridian.Reset();
			FVector2f Was = FVector2f::ZeroVector;
			bool bWas = false, bCrossed = false;
			float CrossX = 0;
			for (int32 I = 0; I <= 40; ++I)
			{
				FVector2f Spot;
				if (!AtHour(MidHa + FromMid, -88. + 4.4 * I, Spot)) { Path(Meridian, Tone, 1.f, L, 0, nullptr); Meridian.Reset(); bWas = false; continue; }
				if (bWas && !bCrossed && (Was.Y - HourLine) * (Spot.Y - HourLine) <= 0 && Was.Y != Spot.Y) { CrossX = Was.X + (Spot.X - Was.X) * (HourLine - Was.Y) / (Spot.Y - Was.Y); bCrossed = true; }
				Meridian.Add(Spot); Was = Spot; bWas = true;
			}
			Path(Meridian, Tone, 1.f, L, 0, nullptr);
			// (the even hours on the open picture, every hour closed in)
			if (!bWhole || !bCrossed || (!bFine && Hour % 2 != 0) || !InRim(FVector2f(CrossX, HourLine), .98)) continue;
			const FSlateRect Area(CrossX - HourWide * .5f - 5, Rim.Y + Semi.Y + 6, CrossX + HourWide * .5f + 5, Rim.Y + Semi.Y + 22);
			if (!Free(Area)) continue;
			Taken.Add(Area);
			S47SkyText(Out, L + 6, G, Num(double(Hour), 0, 2, false, true) + TEXT("h"), CrossX - HourWide * .5f, Rim.Y + Semi.Y + 2, Letters, LabelTone);
		}
	}
	// ---- the galactic plane, a stippled band, and the faint field under the table's stars (more of it closed in) ----
	for (const FS47ChartPlace& Speck : S47SkyDustPlaces())
	{
		FVector2f Spot;
		if (!AtDir(Sph::Turned(Speck.Unit, Lst), Spot) || !InRim(Spot, .985)) continue;
		S47SkyDot(Out, L, G, Spot, 1.4f + 1.2f * Speck.Bright, S47SkyFade(S47SkyGreen, (.22f + .42f * Speck.Bright) * Gain));
	}
	{
		const TArray<FS47ChartPlace>& Faint = S47SkyFaintPlaces();
		const int32 FaintShown = FMath::Min(Faint.Num(), Sph::FieldShown(Pl.Zoom));
		for (int32 I = 0; I < FaintShown; ++I)
		{
			FVector2f Spot;
			if (!AtDir(Sph::Turned(Faint[I].Unit, Lst), Spot) || !InRim(Spot, .985)) continue;
			S47SkyDot(Out, L, G, Spot, 1.2f + Faint[I].Bright, S47SkyFade(S47SkyGreen, (.3f + .45f * Faint[I].Bright) * Gain));
		}
	}
	// ---- the array's window: where the reticle can go, as a dashed outline on the sky ----
	{
		TArray<FVector2f> Edge; Edge.Reserve(96);
		auto Corner = [&](double Az, double El) { FVector2f Spot; if (At(FVector2D(Az, El), Spot)) Edge.Add(Spot); };
		for (double Az = FSky::AzMin; Az <= FSky::AzMax + 1e-6; Az += 4.) Corner(Az, FSky::ElMin);
		for (double El = FSky::ElMin; El <= FSky::ElMax + 1e-6; El += 4.) Corner(FSky::AzMax, El);
		for (double Az = FSky::AzMax; Az >= FSky::AzMin - 1e-6; Az -= 4.) Corner(Az, FSky::ElMax);
		for (double El = FSky::ElMax; El >= FSky::ElMin - 1e-6; El -= 4.) Corner(FSky::AzMin, El);
		Path(Edge, S47SkyFade(S47SkyLine, .6f), 1.f, L, 2, nullptr);
	}
	// ---- the stick figures, near white and clearly over the graticule, and the table's stars in three classes: a white-hot
	// point in a halo with the beam's glint, a bright point in a halo, a plain point; a soft shimmer on all of them ----
	{
		const TArray<FS47ChartPlace>& StarList = S47SkyStarPlaces();
		TArray<FVector2f, TInlineAllocator<256>> StarAt;
		TArray<uint8, TInlineAllocator<256>> StarSeen;
		StarAt.SetNumZeroed(StarList.Num()); StarSeen.SetNumZeroed(StarList.Num());
		for (int32 I = 0; I < StarList.Num(); ++I) StarSeen[I] = AtDir(Sph::Turned(StarList[I].Unit, Lst), StarAt[I]) ? 1 : 0;
		const FLinearColor FigureTone = S47SkyFade(FMath::Lerp(S47SkyGreen, FLinearColor::White, .5f), .9f * Gain);
		TArray<FVector2f> Pair; Pair.SetNumZeroed(2);
		for (const Sph::FLink& Link : Sph::Links)
		{
			if (!StarSeen.IsValidIndex(Link.A) || !StarSeen.IsValidIndex(Link.B) || !StarSeen[Link.A] || !StarSeen[Link.B]) continue;
			Pair[0] = StarAt[Link.A]; Pair[1] = StarAt[Link.B];
			if (!InRim(Pair[0], 1.) && !InRim(Pair[1], 1.)) continue;
			Path(Pair, FigureTone, 1.3f, L + 1, 0, nullptr);
		}
		const FLinearColor StarHot = FMath::Lerp(S47SkyGreen, FLinearColor::White, .7f), StarWarm = FMath::Lerp(S47SkyGreen, FLinearColor::White, .4f);
		for (int32 I = 0; I < StarList.Num(); ++I)
		{
			if (!StarSeen[I] || !InRim(StarAt[I], .985)) continue;
			const FVector2f Spot = StarAt[I];
			const float Mag = StarList[I].Bright, Lit = S47RxLook::Twinkle(I, Now, bCalm) * Gain;
			if (Mag < 1.5f)
			{
				S47SkyDot(Out, L + 2, G, Spot, 8.f, S47SkyFade(S47SkyGreen, .2f * Lit));
				S47SkyBox(Out, L + 2, G, Spot - FVector2f(6.5f, 1.5f), FVector2f(13, 3), S47SkyFade(S47SkyGreen, .42f * Lit), true);
				S47SkyBox(Out, L + 2, G, Spot - FVector2f(1.5f, 6.5f), FVector2f(3, 13), S47SkyFade(S47SkyGreen, .42f * Lit), true);
				S47SkyDot(Out, L + 2, G, Spot, 4.4f, S47SkyFade(StarHot, Lit));
			}
			else if (Mag < 3.f)
			{
				S47SkyDot(Out, L + 2, G, Spot, 6.f, S47SkyFade(S47SkyGreen, .24f * Lit));
				S47SkyDot(Out, L + 2, G, Spot, 3.2f, S47SkyFade(StarWarm, Lit));
			}
			else S47SkyDot(Out, L + 2, G, Spot, 2.4f, S47SkyFade(S47SkyGreen, .9f * Lit));
		}
	}
	S47SkyGlow(Out, L + 2, G, RimPath, S47SkyFade(S47SkyGreen, .78f), 1.6f);

	// ---- the catalogue: the card sources as squares in their boxes, the minor field as small crosses ----
	// A name stands beside its mark where it is free of the names already placed and inside the rim (above right first, then the
	// other corners); with the fix taken or held also clear of the reticle, beside it. It is written after the bearings, so that
	// the lines do not lie over the words. The minor field's names stand only near the reticle or closed in (round 2: fewer
	// words rather than smaller ones).
	struct FNameAt { FString Name; FVector2f Spot; FLinearColor Tone; };
	TArray<FNameAt> Names;
	TArray<FSlateRect> NamesTaken;
	const bool bKeepClear = bLocking || bLocked;
	if (bKeepClear) NamesTaken.Add(FSlateRect(Ret.X - Gap - 6, Ret.Y - Gap - 6, Ret.X + Gap + 6, Ret.Y + Gap + 6));
	auto PlaceName = [&](const FString& Name, const FVector2f& Mark, const FLinearColor& Tone, float Clear)
	{
		const float Wide = S47RxLook::TextWide(Name.Len(), Letters) + 2, High = Letters + 1.f, Reach = Gap + 12.f;
		const FVector2f Tries[8] = { FVector2f(Clear, -High - 5), FVector2f(-Clear - Wide, -High - 5), FVector2f(Clear, 6), FVector2f(-Clear - Wide, 6),
			FVector2f(Reach, -High * .5f), FVector2f(-Reach - Wide, -High * .5f), FVector2f(-Wide * .5f, Reach), FVector2f(-Wide * .5f, -Reach - High) };
		for (int32 Try = 0; Try < (bKeepClear ? 8 : 4); ++Try)
		{
			const FVector2f Spot = Mark + Tries[Try];
			if (!InRim(Spot + FVector2f(0, High * .5f), .96) || !InRim(Spot + FVector2f(Wide, High * .5f), .96)) continue;
			const FSlateRect Area(Spot.X, Spot.Y, Spot.X + Wide, Spot.Y + High);
			bool bFree = true;
			for (const FSlateRect& Other : NamesTaken) bFree &= !FSlateRect::DoRectanglesIntersect(Other, Area);
			if (!bFree) continue;
			NamesTaken.Add(Area);
			Names.Add({ Name, Spot, Tone });
			return;
		}
	};
	for (const FSky::FSource& Src : FSky::Catalogue())
	{
		FVector2f Mark;
		if (!At(Src.At, Mark) || !InRim(Mark, .97)) continue;
		if (Src.bMajor)
		{
			const bool bCard = Src.Run == K.CardRun();
			// its box: the rule's own circle of BoxRadius round it, as it lies on the sphere
			TArray<FVector2f> BoxPath; BoxPath.Reserve(25);
			for (int32 I = 0; I <= 24; ++I)
			{
				const double A = 2 * PI * I / 24;
				FVector2f Spot;
				if (At(Src.At + FVector2D(FMath::Cos(A), FMath::Sin(A)) * FSky::BoxRadius, Spot)) BoxPath.Add(Spot);
			}
			Path(BoxPath, bCard ? S47SkyGreen * .8f : S47SkyLine, 1.f, L + 2, bCard ? 0 : 2, nullptr);
			S47SkyBox(Out, L + 2, G, Mark - FVector2f(4, 4), FVector2f(8, 8), S47SkyGreen, false);
			PlaceName(bCard ? Src.Name + TEXT(" / CARD") : Src.Name, Mark, S47SkyGreen, 12.f);
		}
		else
		{
			S47SkyLines(Out, L + 2, G, { Mark - FVector2f(4, 0), Mark + FVector2f(4, 0) }, S47SkyLine, 1.5f);
			S47SkyLines(Out, L + 2, G, { Mark - FVector2f(0, 4), Mark + FVector2f(0, 4) }, S47SkyLine, 1.5f);
			if (bFine || FVector2D::Distance(Src.At, K.ReticleAt()) < 12.) PlaceName(Src.Name, Mark, S47SkyGreen * .62f, 7.f);
		}
	}

	// ---- the bearings: one line an antenna, each arm's a fan from the rim that closes on the fix: solid where the arm has
	// locked, fainter with a dashed middle where it still sweeps. Round 2: thin lines at every stage, never a wedge (no glow;
	// every other line starts further out; with the fix taken or held all of them stop short of the reticle's rings), with
	// rungs across each fan and the arm's number by the rim (RX-LOOK gjennomgang 14 Nov: numbers, not letters) ----
	if (bBearings)
	{
		// which way each arm's fan comes in from: three ways a third of a turn apart on the rule's chart (headings 205, 85 and
		// 325 degrees there), which on the glass are the upper left, the upper right and the foot
		static const double FanFrom[FSky::ArmCount] = { -1., 1., -1. };
		// how much wider than the rule's own the fan is drawn. The arm that comes in from high up runs toward the zenith, where
		// the sphere squeezes the rule's seven degrees to a strip: its lines are turned further out of the arm's middle, about
		// the points of the band they pass through. The middle line of every arm is the rule's own.
		static const double FanWiden[FSky::ArmCount] = { 1., 2.6, 1.3 };
		const int32 PerArm = FSky::DishCount / FSky::ArmCount;
		TArray<FVector2f> Rays; Rays.Reserve(FSky::DishCount * 64);
		int32 RayFrom[FSky::DishCount], RayCount[FSky::DishCount];
		TArray<FVector2f> Ray;
		for (int32 I = 0; I < FSky::DishCount; ++I)
		{
			RayFrom[I] = Rays.Num(); RayCount[I] = 0;
			const FSky::FLine Ln = K.Line(I);
			if (!Ln.bOn) continue;
			const int32 Arm = FMath::Clamp(Ln.Arm, 0, FSky::ArmCount - 1), InArm = I - Arm * PerArm;
			const FVector2D MidDir = K.Line(Arm * PerArm + PerArm / 2).Dir;
			const double Off = FMath::Clamp(FMath::Atan2(FVector2D::CrossProduct(MidDir, Ln.Dir), FVector2D::DotProduct(MidDir, Ln.Dir)), -.3, .3) * (FanWiden[Arm] - 1.);
			const FVector2D Drawn(Ln.Dir.X * FMath::Cos(Off) - Ln.Dir.Y * FMath::Sin(Off), Ln.Dir.X * FMath::Sin(Off) + Ln.Dir.Y * FMath::Cos(Off));
			const FVector2D Along = Drawn * FanFrom[Arm];
			// past its nearest point to the crossing it runs on a little: far out the three fans cross and make the triangle,
			// on the lock they all end on the fix
			const double Over = FMath::Clamp(1.6 * FMath::Abs(Ln.Off) + .8, .8, 14.);
			// Inside the array's window the line is straight on the rule's chart (so an arc on the sphere); from the window's
			// edge it is carried straight on across the glass to the rim, so that each arm's fan comes in from the rim.
			FVector2D From = Ln.Through - Along * Over, Until = Ln.Through + Along * 400.;
			if (!FSky::ClipSegment(From, Until)) continue;
			const int32 Pieces = FMath::Clamp(int32(FVector2D::Distance(From, Until) / 3.) + 1, 2, 60);
			for (int32 Piece = 0; Piece <= Pieces; ++Piece)
			{
				FVector2f Spot;
				if (At(FMath::Lerp(From, Until, double(Piece) / Pieces), Spot)) Rays.Add(Spot);
			}
			if (Rays.Num() - RayFrom[I] >= 2)
			{
				const FVector2f Onward = (Rays.Last() - Rays[Rays.Num() - 2]).GetSafeNormal();
				Rays.Add(Rays.Last() + Onward * ((Semi.X + Semi.Y) * 2.f)); // (the path is cut at the rim)
			}
			RayCount[I] = Rays.Num() - RayFrom[I];
			if (RayCount[I] < 2) continue;
			// where the drawn line begins: the middle line at its start, the others a step or two further out, and with the
			// fix taken or held nothing nearer the crossing than the reticle's rings
			const bool bMiddle = InArm == PerArm / 2;
			const float Trim = bMiddle ? 0.f : InArm % 2 ? 26.f : 52.f, Clear = bKeepClear ? LockGap * .76f : 0.f;
			Ray.Reset();
			float Gone = 0;
			for (int32 At0 = RayFrom[I]; At0 + 1 < RayFrom[I] + RayCount[I]; ++At0)
			{
				const FVector2f A = Rays[At0], B = Rays[At0 + 1];
				const float Len = FVector2f::Distance(A, B);
				if (Ray.IsEmpty())
				{
					// the first place on this stretch that is far enough along and far enough from the crossing
					float Start = Len > 1e-4f ? FMath::Clamp((Trim - Gone) / Len, 0.f, 1.f) : 1.f;
					const float FarA = FVector2f::Distance(A, Xc), FarB = FVector2f::Distance(B, Xc);
					if (Clear > 0 && FarA < Clear) Start = FarB > Clear ? FMath::Max(Start, (Clear - FarA) / FMath::Max(FarB - FarA, 1e-4f)) : 1.f;
					if (Gone + Len >= Trim && Start < 1.f) Ray.Add(A + (B - A) * Start);
				}
				if (!Ray.IsEmpty()) Ray.Add(B);
				Gone += Len;
			}
			if (Ln.bSolid) Path(Ray, S47SkyFade(S47SkyGreen, (bMiddle ? .9f : .5f) * Gain), bMiddle ? 1.6f : 1.f, L + 3, 0, nullptr);
			else Path(Ray, S47SkyFade(S47SkyGreen, bMiddle ? .7f : .3f), 1.f, L + 3, bMiddle ? 2 : 0, nullptr);
		}
		// the rungs: across each arm's fan at set lengths along its lines, and the arm's number where its middle line nears the rim
		static const float RungAt[] = { 70.f, 115.f, 165.f, 220.f, 280.f, 345.f, 415.f };
		TArray<FVector2f> Rung;
		for (int32 Arm = 0; Arm < FSky::ArmCount; ++Arm)
		{
			const FLinearColor RungTone = S47SkyFade(S47SkyGreen, K.ArmSolid(Arm) ? .42f * Gain : .22f);
			for (const float Length : RungAt)
			{
				Rung.Reset();
				for (int32 InArm = 0; InArm < PerArm; ++InArm)
				{
					const int32 Dish = Arm * PerArm + InArm;
					FVector2f Spot;
					if (RayCount[Dish] >= 2 && S47SkyAlong(Rays, RayFrom[Dish], RayCount[Dish], Length, Spot) && (!bKeepClear || FVector2f::Distance(Spot, Xc) > LockGap * .76f)) Rung.Add(Spot);
				}
				Path(Rung, RungTone, 1.f, L + 3, 0, nullptr);
			}
			const int32 Mid = Arm * PerArm + PerArm / 2;
			if (RayCount[Mid] < 2) continue;
			for (float Length = 40.f; Length < 900.f; Length += 10.f)
			{
				FVector2f Spot;
				if (S47SkyAlong(Rays, RayFrom[Mid], RayCount[Mid], Length, Spot) && InRim(Spot, .88)) continue;
				if (S47SkyAlong(Rays, RayFrom[Mid], RayCount[Mid], FMath::Max(30.f, Length - 34.f), Spot) && InRim(Spot, .95))
				{
					S47SkyText(Out, L + 6, G, S47RxLook::ArmLabel(Arm), Spot.X - 4, Spot.Y - 11, Letters, K.ArmSolid(Arm) ? S47SkyGreen : S47SkyGreen * .7f);
				}
				break;
			}
		}
		// the triangle the three bands make: what has to close
		FVector2D Corner[FSky::ArmCount];
		if (K.Triangle(Corner) && K.MeanOff() > .12)
		{
			TArray<FVector2f> SidePath;
			for (int32 I = 0; I < FSky::ArmCount; ++I)
			{
				SidePath.Reset();
				for (int32 Part = 0; Part <= 5; ++Part)
				{
					FVector2f Spot;
					if (At(FMath::Lerp(Corner[I], Corner[(I + 1) % FSky::ArmCount], Part / 5.), Spot)) SidePath.Add(Spot);
				}
				Path(SidePath, S47SkyAmber, 1.8f, L + 4, 1, nullptr);
			}
		}
	}

	for (const FNameAt& Each : Names) S47SkyText(Out, L + 3, G, Each.Name, Each.Spot.X, Each.Spot.Y, Letters, Each.Tone);

	// ---- the search sweep, while the search is on: a bead running slowly round the rim with its fading trail, and a faint
	// hand to it (round 2: a hand with a trail looked like a fourth fan of bearings) ----
	if (bSearch)
	{
		const double Turn = S47RxLook::Sweep(Now, bCalm);
		if (Turn >= 0)
		{
			constexpr int32 TailCount = 18;
			constexpr float TailStep = .034f;
			auto OnRim = [&](float Angle) { return Rim + FVector2f(FMath::Cos(Angle) * Semi.X, FMath::Sin(Angle) * Semi.Y); };
			const float Head = float(2 * PI * Turn);
			S47SkyLines(Out, L + 4, G, { Rim, OnRim(Head) }, S47SkyFade(S47SkyGreen, .12f), 1.f);
			for (int32 Tail = 0; Tail < TailCount; ++Tail)
			{
				S47SkyLines(Out, L + 4, G, { OnRim(Head - Tail * TailStep), OnRim(Head - (Tail + 1) * TailStep) }, S47SkyFade(S47SkyGreen, .9f * (1.f - float(Tail) / TailCount)), Tail < 3 ? 4.f : 3.f);
			}
		}
	}

	// ---- the lock sequence: three rings closing on the crossing, one an arm; closed, they are the reticle's own rings ----
	if (bLocking || bLocked)
	{
		static const float Final[FSky::ArmCount] = { LockGap * .3f, LockGap * .5f, LockGap * .72f };
		for (int32 Ring = 0; Ring < FSky::ArmCount; ++Ring)
		{
			const double Due = FSky::LockSeconds * (Ring + 1) / (FSky::ArmCount + 1);
			const float Part = bLocked ? 1.f : float(FMath::Clamp(K.LockProgress() * FSky::LockSeconds / Due, 0., 1.));
			const float Ease = 1.f - FMath::Pow(1.f - Part, 3.f);
			// Unusual: the last ring will not come in, and stands open and dashed.
			const bool bOpen = K.Odd() && Ring == FSky::ArmCount - 1;
			const float End = bOpen ? LockGap * .96f + (bCalm ? 0.f : 2.5f * float(FMath::Sin(Now * 1.3 * 2 * PI * .5))) : Final[Ring];
			const float Radius = FMath::Lerp(90.f + 30.f * Ring, End, Ease);
			if (bOpen) S47SkyRing(Out, L + 4, G, Xc, Radius, S47SkyFade(S47SkyAmber, Part >= 1 ? 1.f : .75f), 1.4f, true);
			else if (Part >= 1) S47SkyGlowHot(Out, L + 4, G, S47SkyRingPath(Xc, Radius), S47SkyAmber, Ring == 0 ? 2.2f : 1.6f);
			else S47SkyRing(Out, L + 4, G, Xc, Radius, S47SkyFade(S47SkyAmber, .75f), 1.4f);
		}
		if (bLocked)
		{
			if (K.Odd())
			{
				// a point the catalogue does not list: a bare diamond, no name
				S47SkyLines(Out, L + 5, G, { Xc + FVector2f(0, -8), Xc + FVector2f(8, 0), Xc + FVector2f(0, 8), Xc + FVector2f(-8, 0), Xc + FVector2f(0, -8) }, S47SkyAmber, 2.f);
			}
			else
			{
				S47SkyDot(Out, L + 5, G, Xc, 11.f, S47SkyFade(S47SkyAmber, .3f));
				S47SkyDot(Out, L + 5, G, Xc, 5.f, S47SkyHot);
			}
			// the lock moment: rings go out from the fix, one after the other, and fade
			for (int32 Ring = 0; Ring < 3; ++Ring)
			{
				float Grow = 0, Alpha = 0;
				if (S47RxLook::Pulse(SinceLock, Ring, bCalm, Grow, Alpha)) S47SkyRing(Out, L + 4, G, Xc, 40.f + 140.f * Grow, S47SkyFade(S47SkyAmber, .55f * Alpha), 1.4f);
			}
		}
	}

	// ---- the reticle (round 2): amber and the brightest thing on the glass at every stage. Four corners that close as it nears
	// the crossing, three rings, hairs to the corners and a hot centre; on the crossing the centre fills and a bar under it
	// fills (said by shape: the colour is the same throughout) ----
	{
		const bool bOn = bLocking || bLocked || (bBearings && K.Steady() && K.Distance() <= FSky::LockRadius);
		const FLinearColor Col = S47SkyAmber;
		const float Leg = 14.f, W = bOn ? 2.8f : 2.2f;
		// where it has just been: the phosphor's short memory
		if (Trail.IsEmpty() || FVector2f::Distance(Trail.Last(), Ret) > 2.f) { Trail.Add(Ret); TrailAt.Add(Now); }
		while (Trail.Num() > 0 && (Now - TrailAt[0] > .45 || Trail.Num() > 24)) { Trail.RemoveAt(0); TrailAt.RemoveAt(0); }
		for (int32 I = 0; I + 1 < Trail.Num(); ++I)
		{
			const float Left = float(FMath::Exp(-(Now - TrailAt[I]) / .16));
			if (InRim(Trail[I], 1.)) S47SkyRing(Out, L + 4, G, Trail[I], 6.f, S47SkyFade(Col, .3f * Left), 1.f);
		}
		for (const FVector2f Q : { FVector2f(-1, -1), FVector2f(1, -1), FVector2f(1, 1), FVector2f(-1, 1) })
		{
			const FVector2f Kn = Ret + Q * Gap;
			S47SkyGlowHot(Out, L + 5, G, { Kn - FVector2f(Q.X * Leg, 0), Kn, Kn - FVector2f(0, Q.Y * Leg) }, Col, W);
		}
		// three rings inside the corners (with the fix taken or held they are the lock sequence's own), and the hairs
		if (!bLocking && !bLocked)
		{
			S47SkyGlowHot(Out, L + 5, G, S47SkyRingPath(Ret, Gap * .3f), Col, 1.5f);
			S47SkyRing(Out, L + 5, G, Ret, Gap * .5f, S47SkyFade(Col, .8f), 1.3f);
			S47SkyRing(Out, L + 5, G, Ret, Gap * .72f, S47SkyFade(Col, .6f), 1.1f);
		}
		for (int32 Tick = 0; Tick < 12; ++Tick)
		{
			const float A = 2 * PI * Tick / 12;
			const FVector2f D(FMath::Cos(A), FMath::Sin(A));
			const bool bHair = Tick % 3 == 0;
			S47SkyLines(Out, L + 5, G, { Ret + D * (bHair ? Gap * .12f : Gap * .62f), Ret + D * (bHair ? Gap * .98f : Gap * .72f) }, S47SkyFade(Col, bHair ? .9f : .6f), bHair ? 1.3f : 1.f);
		}
		if (!bLocked)
		{
			S47SkyDot(Out, L + 5, G, Ret, bOn ? 13.f : 9.f, S47SkyFade(Col, .3f));
			S47SkyDot(Out, L + 5, G, Ret, bOn ? 7.f : 4.f, S47SkyHot);
		}
		// held on the crossing: a short bar under the reticle fills before the lock starts
		if (bSearch && K.HoldProgress() > 0)
		{
			S47SkyBox(Out, L + 5, G, Ret + FVector2f(-22, Gap + 8), FVector2f(44, 6), S47SkyAmber, false);
			S47SkyBox(Out, L + 5, G, Ret + FVector2f(-20, Gap + 10), FVector2f(40 * float(K.HoldProgress()), 2), S47SkyAmber, true);
		}
		// the hint: chevrons from the reticle toward the crossing
		if (K.HintShown() && K.Steady())
		{
			const FVector2f To = Xc - Ret; const float Far = To.Size();
			if (Far > Gap + 70)
			{
				const FVector2f D = To / Far, N(-D.Y, D.X);
				for (int32 I = 0; I < 3; ++I)
				{
					const FVector2f Mid = Ret + D * (Gap + 22 + 16 * I);
					S47SkyLines(Out, L + 5, G, { Mid - D * 5 + N * 7, Mid + D * 5, Mid - D * 5 - N * 7 }, S47SkyFade(S47SkyAmber, Breath(1.1, -.22 * I)), 2.2f);
				}
			}
		}
		// the operator's own pointer, while it is inside the rim
		if (bHover && K.HasPointer() && !bLocking && !bLocked)
		{
			S47SkyLines(Out, L + 5, G, { HoverLocal - FVector2f(6, 0), HoverLocal + FVector2f(6, 0) }, S47SkyGreen * .7f, 1.5f);
			S47SkyLines(Out, L + 5, G, { HoverLocal - FVector2f(0, 6), HoverLocal + FVector2f(0, 6) }, S47SkyGreen * .7f, 1.5f);
		}
	}

	// ---- the corners: three short lines each (round 2: fewer words rather than smaller ones), and which way the pole and
	// the east are (upright: north is straight up and east is to the left, as the sky is seen) ----
	{
		for (int32 Row = 0; Row < 3; ++Row) S47SkyText(Out, L + 6, G, TopLeft[Row], 4, 2 + Row * Pitch, Letters, Row == 1 ? S47SkyAmber : Quiet);
		for (int32 Row = 0; Row < 2; ++Row) S47SkyText(Out, L + 6, G, TopRight[Row], Sz.X - S47RxLook::TextWide(TopRight[Row].Len(), Letters) - 6, 2 + Row * Pitch, Letters, S47SkyGreen);
		for (int32 Row = 0; Row < 3; ++Row) S47SkyText(Out, L + 6, G, Foot[Row], 4, Sz.Y - 4 - (3 - Row) * Pitch, Letters, S47SkyGreen);
		const FVector2f Hub(Sz.X - 46, Sz.Y - 50);
		S47SkyLines(Out, L + 6, G, { Hub + FVector2f(0, 18), Hub + FVector2f(0, -20) }, Quiet, 1.5f);
		S47SkyLines(Out, L + 6, G, { Hub + FVector2f(-4, -14), Hub + FVector2f(0, -20), Hub + FVector2f(4, -14) }, Quiet, 1.5f);
		S47SkyLines(Out, L + 6, G, { Hub - FVector2f(16, 0), Hub + FVector2f(16, 0) }, Quiet, 1.f);
		S47SkyText(Out, L + 6, G, TEXT("NCP"), Hub.X - S47RxLook::TextWide(3, Letters) * .5f, Hub.Y - 44, Letters, Quiet);
		S47SkyText(Out, L + 6, G, TEXT("E"), Hub.X - 30, Hub.Y - 12, Letters, Quiet);
		S47SkyText(Out, L + 6, G, TEXT("W"), Hub.X + 22, Hub.Y - 12, Letters, Quiet);
	}

	// ---- the lock moment: the whole picture swells once, softly (a veil of the phosphor inside the rim) ----
	if (const float Swell = S47RxLook::Bloom(SinceLock, bCalm); Swell > 0)
	{
		constexpr int32 Strips = 26;
		for (int32 I = 0; I < Strips; ++I)
		{
			const float V = (I + .5f) / Strips * 2 - 1, Half = Semi.X * FMath::Sqrt(FMath::Max(0.f, 1 - V * V));
			S47SkyBox(Out, L + 7, G, FVector2f(Rim.X - Half, Rim.Y + (V - 1.f / Strips) * Semi.Y), FVector2f(Half * 2, Semi.Y * 2 / Strips + .5f), S47SkyFade(S47SkyGreen, .05f * Swell), true);
		}
	}
	return L + 8;
}

bool SS47SkyChart::SelfTest(TArray<FString>& Lines)
{
	using FSky = FS47SkyAcquire;
	bool bOk = true;
	auto Check = [&](bool bPass, const FString& What) { bOk &= bPass; Lines.Add(FString(bPass ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("sky chart: ") + What); };
	for (const FVector2f Size : { FVector2f(716, 480), FVector2f(716, 440), FVector2f(640, 360), FVector2f(900, 600) })
	{
		const FPlot P = PlotFor(Size);
		const FVector2D Rim = P.View.Rim, Semi = P.View.Semi;
		// the rim inside the widget, with room for the declinations round it, the pole's over it and the hours under it
		const bool bRoom = Rim.X - Semi.X >= SideRoom - .01 && Rim.X + Semi.X <= Size.X - SideRoom + .01 && Rim.Y - Semi.Y >= TopRoom - .01 && Rim.Y + Semi.Y <= Size.Y - FootRoom + .01
			&& FMath::Abs(Rim.X - Size.X * .5) < .01 && SideRoom >= S47RxLook::TextWide(4, S47SkyLetters) + 12 && TopRoom >= 22 && FootRoom >= 24;
		bool bRound = true, bInside = true;
		double Open = 1e9;
		for (const float Zoom : { 1.f, 1.7f, ZoomMost })
		{
			FPlot Z = P;
			Z.Zoom = Zoom; Z.Focus = ToLocal(P, FVector2D(112, 54));
			for (double Az = FSky::AzMin; Az <= FSky::AzMax + 1e-9; Az += 6.) for (double El = FSky::ElMin; El <= FSky::ElMax + 1e-9; El += 4.)
			{
				FVector2f At;
				if (!Place(Z, FVector2D(Az, El), At)) { bRound = false; continue; }
				// (the glass is counted in single precision: a thousandth of a degree is far finer than a unit of it)
				bRound &= ToSky(Z, At).Equals(FVector2D(Az, El), 2e-3);
				if (Zoom != 1.f) continue;
				bInside &= Contains(Z, At);
				for (double Turn = 0; Turn < 180; Turn += 15) Open = FMath::Min(Open, S47SkySphere::UnitsPerDegree(P.View, FVector2D(Az, El), FVector2D(FMath::Cos(FMath::DegreesToRadians(Turn)), FMath::Sin(FMath::DegreesToRadians(Turn)))) * FSky::LockRadius);
			}
		}
		// the open picture is fixed to the ground: the crossing's place does not move when the picture closes in about it
		FPlot Closed = P;
		Closed.Zoom = ZoomMost; Closed.Focus = ToLocal(P, FVector2D(112, 54));
		const bool bHeld = ToLocal(Closed, FVector2D(112, 54)).Equals(Closed.Focus, .01f) && FVector2f::Distance(ToLocal(Closed, FVector2D(118, 54)), Closed.Focus) > FVector2f::Distance(ToLocal(P, FVector2D(118, 54)), Closed.Focus) * (ZoomMost - .01f);
		Check(bRoom && bRound && bInside && bHeld && !Contains(P, FVector2f(2, 2)) && Contains(P, FVector2f(Rim)),
			FString::Printf(TEXT("%.0f x %.0f: the oval in the middle with room for the words round it, the array's whole window inside the rim, to the glass and back open and closed in, the crossing standing still as the picture closes in (%.2f units a degree up, %.2f across)"), Size.X, Size.Y, FMath::DegreesToRadians(P.View.Radius), FMath::DegreesToRadians(P.View.Radius * P.View.Stretch)));
		// closed in, the rule's lock circle is a comfortable target under a pointer: about nine units across, as on the flat chart
		if (Size.Y == 440.f) Check(Open * ZoomMost >= 4.4, FString::Printf(TEXT("the lock circle is at least %.1f units across on the open picture and %.1f closed in (the flat chart's promise: nine)"), Open * 2, Open * 2 * ZoomMost));
	}
	// closing in: all the way near the crossing, not at all far from it, and a pointer nearer the crossing on the glass is always
	// nearer it in the sky (the picture never folds under the hand)
	bool bZoom = ZoomFor(0.) == ZoomMost && ZoomFor(ZoomNear) == ZoomMost && ZoomFor(ZoomFar) == 1.f && ZoomFor(90.) == 1.f, bOrder = true;
	double Was = -1.;
	for (double Off = 0; Off <= 60; Off += .1)
	{
		const double InSky = Off / ZoomFor(Off);
		bZoom &= ZoomFor(Off) >= 1.f && ZoomFor(Off) <= ZoomMost && (Off < .05 || ZoomFor(Off) <= ZoomFor(Off - .1) + 1e-6f);
		bOrder &= InSky >= Was;
		Was = InSky;
	}
	Check(bZoom && bOrder && 5. / ZoomFor(5.) < 2.3, FString::Printf(TEXT("the picture closes in %.1f times near the crossing: a pointer five degrees off on the open picture stands %.1f off once it has, and a nearer pointer is always nearer"), ZoomMost, 5. / ZoomFor(5.)));
	Check(SteerOf(EKeys::Up) == FVector2D(0, 1) && SteerOf(EKeys::Down) == FVector2D(0, -1) && SteerOf(EKeys::Left) == FVector2D(-1, 0) && SteerOf(EKeys::Right) == FVector2D(1, 0)
		&& SteerOf(EKeys::Gamepad_DPad_Up) == FVector2D(0, 1) && SteerOf(EKeys::Gamepad_DPad_Left) == FVector2D(-1, 0) && SteerOf(EKeys::Escape).IsZero(),
		TEXT("the arrow keys and the d-pad steer; Escape stays the way out"));
	// the tables as the picture uses them: every place a direction, and a path's lengths as the bearings' rungs count them
	bool bUnit = S47SkyStarPlaces().Num() == int32(S47SkySphere::Star::Count) && S47SkyDustPlaces().Num() == S47SkySphere::DustCount && S47SkyFaintPlaces().Num() == S47SkySphere::FieldCount;
	for (const FS47ChartPlace& Each : S47SkyStarPlaces()) bUnit &= FMath::Abs(Each.Unit.Size() - 1.) < 1e-9;
	for (const FS47ChartPlace& Each : S47SkyDustPlaces()) bUnit &= FMath::Abs(Each.Unit.Size() - 1.) < 1e-9 && Each.Bright > 0;
	for (const FS47ChartPlace& Each : S47SkyFaintPlaces()) bUnit &= FMath::Abs(Each.Unit.Size() - 1.) < 1e-9;
	const TArray<FVector2f> Bent = { FVector2f(0, 0), FVector2f(10, 0), FVector2f(10, 10) };
	FVector2f OnIt, PastIt;
	const bool bAlong = S47SkyAlong(Bent, 0, 3, 15.f, OnIt) && OnIt.Equals(FVector2f(10, 5), 1e-4f) && !S47SkyAlong(Bent, 0, 3, 25.f, PastIt) && S47SkyAlong(Bent, 1, 2, 4.f, OnIt) && OnIt.Equals(FVector2f(10, 4), 1e-4f);
	Check(bUnit && bAlong && S47SkyLetters >= 22, TEXT("every star of the table, every speck of the galactic plane and every star of the faint field has its place of date; a rung stands a set length along its bearing; no word on the chart is smaller than the next step's"));
	return bOk;
}
