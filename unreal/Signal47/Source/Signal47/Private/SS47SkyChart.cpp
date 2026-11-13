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
	const FLinearColor S47SkyGreen = S47Kit::Css(0x8cffa4), S47SkyAmber = S47Kit::Css(0xffb84a), S47SkyLine = S47Kit::Css(0x3f9a5a), S47SkyDim = S47Kit::Css(0x1d4a2b);
	const FSlateBrush* S47SkyFill() { static const FSlateColorBrush B(FLinearColor::White); return &B; }
	FLinearColor S47SkyFade(FLinearColor C, float A) { C.A = FMath::Clamp(A, 0.f, 1.f); return C; }
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
	// A ring as a polyline; bDashed: every other piece left out.
	void S47SkyRing(FSlateWindowElementList& Out, int32 L, const FGeometry& G, FVector2f At, float Radius, const FLinearColor& C, float W, bool bDashed = false)
	{
		constexpr int32 Steps = 40;
		if (!bDashed)
		{
			TArray<FVector2f> P; P.Reserve(Steps + 1);
			for (int32 I = 0; I <= Steps; ++I) { const float A = 2 * PI * I / Steps; P.Add(At + FVector2f(FMath::Cos(A), FMath::Sin(A)) * Radius); }
			S47SkyLines(Out, L, G, P, C, W);
			return;
		}
		for (int32 I = 0; I < Steps; I += 2)
		{
			const float A = 2 * PI * I / Steps, B = 2 * PI * (I + 1) / Steps;
			S47SkyLines(Out, L, G, { At + FVector2f(FMath::Cos(A), FMath::Sin(A)) * Radius, At + FVector2f(FMath::Cos(B), FMath::Sin(B)) * Radius }, C, W);
		}
	}
	// RX-LOOK-20261113: the table's stars and the galactic plane's specks as places of date (the sky of the game's spring), worked
	// out once.
	struct FS47SkyPlace { double Ra = 0, Dec = 0; float Bright = 1.f; };
	const TArray<FS47SkyPlace>& S47SkyStarPlaces()
	{
		static const TArray<FS47SkyPlace> List = []
		{
			TArray<FS47SkyPlace> Made;
			for (const S47SkySphere::FStar& St : S47SkySphere::Stars)
			{
				FS47SkyPlace P; P.Ra = St.RaHours * 15.; P.Dec = St.Dec; P.Bright = St.Mag; // (for a star: its magnitude)
				S47SkySphere::FromJ2000(P.Ra, P.Dec);
				Made.Add(P);
			}
			return Made;
		}();
		return List;
	}
	const TArray<FS47SkyPlace>& S47SkyDustPlaces()
	{
		static const TArray<FS47SkyPlace> List = []
		{
			TArray<FS47SkyPlace> Made;
			for (int32 I = 0; I < S47SkySphere::DustCount; ++I)
			{
				const S47SkySphere::FDust D = S47SkySphere::Dust(I);
				FS47SkyPlace P; P.Bright = D.Bright;
				S47SkySphere::GalToEq(D.Lon, D.Lat, P.Ra, P.Dec);
				S47SkySphere::FromJ2000(P.Ra, P.Dec);
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
	// The declinations stand beside the rim, the hours of right ascension under it.
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
	auto Slip = [&](FVector2f P) { P.X += S47RxLook::Jitter(Now, P.Y, bCalm); return P; };
	auto At = [&](const FVector2D& AzEl, FVector2f& OutP) { if (!Place(Pl, AzEl, OutP)) return false; OutP = Slip(OutP); return true; };
	auto AtHour = [&](double Ha, double Dec, FVector2f& OutP)
	{
		FVector2D Open;
		if (!Sph::ProjectHaDec(Pl.View, Ha, Dec, Open)) return false;
		OutP = Slip(Pl.Focus + (FVector2f(Open) - Pl.Focus) * Pl.Zoom);
		return true;
	};
	auto InRim = [&](const FVector2f& P, double Shrink) { return Sph::Inside(Pl.View, FVector2D(P), Shrink); };
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
			FVector2f P;
			if (PointAt(I, P)) { Pts.Add(P); continue; }
			Path(Pts, C, W, Layer, Style, Hits);
			Pts.Reset();
		}
		Path(Pts, C, W, Layer, Style, Hits);
	};

	// ---- the sphere's grid: circles of declination, which stand, and the hours of right ascension, which turn with the sky ----
	double MidHa = 0, MidDec = 0;
	Sph::LonLat(Pl.View.Centre, MidHa, MidDec);
	const FLinearColor GridTone = S47SkyFade(S47SkyLine, .5f * Gain), LabelTone = S47SkyGreen * .85f;
	TArray<FVector2f> RimHits;
	// (closed in, the grid is drawn finer: every five degrees and every twenty minutes, the whole ones as before)
	const bool bFine = Pl.Zoom > 1.6f;
	const FLinearColor FineTone = S47SkyFade(S47SkyLine, .3f * Gain);
	for (int32 Circle = -75; Circle <= 75; Circle += bFine ? 5 : 15)
	{
		RimHits.Reset();
		Curve(81, [&](int32 I, FVector2f& P) { return AtHour(MidHa - 120. + 3. * I, double(Circle), P); }, Circle == 0 ? S47SkyFade(S47SkyLine, .9f * Gain) : Circle % 15 ? FineTone : GridTone, 1.f, L, 0, &RimHits);
		// its declination beside the rim, where the circle leaves the picture on the right
		const FVector2f* Best = nullptr;
		for (const FVector2f& Hit : RimHits) if (Hit.X > Rim.X && FMath::Abs(Hit.Y - Rim.Y) < Semi.Y * .66f && (!Best || Hit.X > Best->X)) Best = &Hit;
		if (Best) S47SkyText(Out, L, G, Circle == 0 ? FString(TEXT("0\u00b0")) : FString::Printf(TEXT("%+d\u00b0"), Circle), Best->X + 6, Best->Y - 8, 15, LabelTone);
	}
	TArray<float, TInlineAllocator<24>> HourMarks;
	for (int32 Third = 0; Third < 72; ++Third)
	{
		// (every twenty minutes of right ascension; the whole hours always, the thirds only closed in)
		const int32 Hour = Third / 3;
		const bool bWhole = Third % 3 == 0;
		if (!bWhole && !bFine) continue;
		const double FromMid = Sph::Wrap180(Lst - Third * 5. - MidHa);
		if (FMath::Abs(FromMid) > 112.) continue;
		RimHits.Reset();
		Curve(41, [&](int32 I, FVector2f& P) { return AtHour(MidHa + FromMid, -80. + 4. * I, P); }, !bWhole ? FineTone : Hour % 6 == 0 ? S47SkyFade(S47SkyLine, .8f * Gain) : GridTone, 1.f, L, 0, &RimHits);
		if (!bWhole) continue;
		// its hour under the rim, where it leaves the picture at the foot
		const FVector2f* Best = nullptr;
		for (const FVector2f& Hit : RimHits) if (Hit.Y > Rim.Y + Semi.Y * .5f && FMath::Abs(Hit.X - Rim.X) < Semi.X * .7f) Best = &Hit;
		if (!Best) continue;
		bool bFree = true;
		for (const float Mark : HourMarks) bFree &= FMath::Abs(Mark - Best->X) > 30.f;
		if (!bFree) continue;
		HourMarks.Add(Best->X);
		// (set outward of where it meets the rim, so the hour never lies over the picture)
		S47SkyText(Out, L, G, FString::Printf(TEXT("%02dh"), Hour), Best->X < Rim.X ? Best->X - 22 : Best->X - 4, Best->Y + 4, 15, LabelTone);
	}
	// ---- the galactic plane: a stippled band ----
	for (const FS47SkyPlace& Speck : S47SkyDustPlaces())
	{
		FVector2f P;
		if (!AtHour(Lst - Speck.Ra, Speck.Dec, P) || !InRim(P, .985)) continue;
		const float Side = 1.4f + 1.2f * Speck.Bright;
		S47SkyBox(Out, L, G, P - FVector2f(Side * .5f, Side * .5f), FVector2f(Side, Side), S47SkyFade(S47SkyGreen, (.10f + .24f * Speck.Bright) * Gain), true);
	}
	// ---- the array's window: where the reticle can go, as a dashed outline on the sky ----
	{
		TArray<FVector2f> Edge; Edge.Reserve(96);
		auto Corner = [&](double Az, double El) { FVector2f P; if (At(FVector2D(Az, El), P)) Edge.Add(P); };
		for (double Az = FSky::AzMin; Az <= FSky::AzMax + 1e-6; Az += 4.) Corner(Az, FSky::ElMin);
		for (double El = FSky::ElMin; El <= FSky::ElMax + 1e-6; El += 4.) Corner(FSky::AzMax, El);
		for (double Az = FSky::AzMax; Az >= FSky::AzMin - 1e-6; Az -= 4.) Corner(Az, FSky::ElMax);
		for (double El = FSky::ElMax; El >= FSky::ElMin - 1e-6; El -= 4.) Corner(FSky::AzMin, El);
		Path(Edge, S47SkyFade(S47SkyLine, .75f), 1.f, L, 2, nullptr);
	}
	// ---- the stars, by magnitude, with a soft shimmer, and the stick figures between them ----
	{
		const TArray<FS47SkyPlace>& StarList = S47SkyStarPlaces();
		TArray<FVector2f, TInlineAllocator<256>> StarAt;
		TArray<uint8, TInlineAllocator<256>> StarSeen;
		StarAt.SetNumZeroed(StarList.Num()); StarSeen.SetNumZeroed(StarList.Num());
		for (int32 I = 0; I < StarList.Num(); ++I)
		{
			FVector2f P;
			if (!AtHour(Lst - StarList[I].Ra, StarList[I].Dec, P)) continue;
			StarAt[I] = P; StarSeen[I] = 1;
			if (!InRim(P, .985)) continue;
			const float Mag = StarList[I].Bright, Side = FMath::Clamp(4.f - .7f * Mag, 1.5f, 5.f);
			const float Lit = FMath::Clamp(1.05f - .14f * Mag, .5f, 1.f) * S47RxLook::Twinkle(I, Now, bCalm) * Gain;
			S47SkyBox(Out, L + 1, G, P - FVector2f(Side * .5f, Side * .5f), FVector2f(Side, Side), S47SkyFade(S47SkyGreen, Lit), true);
			if (Mag < 1.6f)
			{
				// the brightest: the beam's glint
				S47SkyLines(Out, L + 1, G, { P - FVector2f(Side + 2, 0), P + FVector2f(Side + 2, 0) }, S47SkyFade(S47SkyGreen, .34f * Lit), 1.f);
				S47SkyLines(Out, L + 1, G, { P - FVector2f(0, Side + 2), P + FVector2f(0, Side + 2) }, S47SkyFade(S47SkyGreen, .34f * Lit), 1.f);
			}
		}
		const FLinearColor FigureTone = S47SkyFade(S47SkyLine, .62f * Gain);
		TArray<FVector2f> Pair; Pair.SetNumZeroed(2);
		for (const Sph::FLink& Link : Sph::Links)
		{
			if (!StarSeen.IsValidIndex(Link.A) || !StarSeen.IsValidIndex(Link.B) || !StarSeen[Link.A] || !StarSeen[Link.B]) continue;
			Pair[0] = StarAt[Link.A]; Pair[1] = StarAt[Link.B];
			if (!InRim(Pair[0], 1.) && !InRim(Pair[1], 1.)) continue;
			Path(Pair, FigureTone, 1.f, L + 1, 0, nullptr);
		}
	}
	S47SkyGlow(Out, L + 1, G, RimPath, S47SkyFade(S47SkyGreen, .78f), 1.6f);

	// ---- the catalogue: the card sources as squares in their boxes, the minor field as small crosses, each with its name ----
	// A name stands beside its mark where it is free of the names already placed and inside the rim (above right first, then the
	// other corners); it is written after the bearings, so that the lines do not lie over the words.
	struct FNameAt { FString Name; FVector2f Spot; int32 Size; FLinearColor Tone; };
	TArray<FNameAt> Names;
	TArray<FSlateRect> NamesTaken;
	auto PlaceName = [&](const FString& Name, const FVector2f& Mark, int32 Size, const FLinearColor& Tone, float Clear)
	{
		const float Wide = Name.Len() * Size * .4f + 2, High = Size + 1.f;
		const FVector2f Tries[4] = { FVector2f(Clear, -High - 5), FVector2f(-Clear - Wide, -High - 5), FVector2f(Clear, 6), FVector2f(-Clear - Wide, 6) };
		for (const FVector2f& Try : Tries)
		{
			const FVector2f P = Mark + Try;
			if (!InRim(P + FVector2f(0, High * .5f), .96) || !InRim(P + FVector2f(Wide, High * .5f), .96)) continue;
			const FSlateRect Area(P.X, P.Y, P.X + Wide, P.Y + High);
			bool bFree = true;
			for (const FSlateRect& Other : NamesTaken) bFree &= !FSlateRect::DoRectanglesIntersect(Other, Area);
			if (!bFree) continue;
			NamesTaken.Add(Area);
			Names.Add({ Name, P, Size, Tone });
			return;
		}
	};
	for (const FSky::FSource& Src : FSky::Catalogue())
	{
		FVector2f C;
		if (!At(Src.At, C) || !InRim(C, .97)) continue;
		if (Src.bMajor)
		{
			const bool bCard = Src.Run == K.CardRun();
			// its box: the rule's own circle of BoxRadius round it, as it lies on the sphere
			TArray<FVector2f> BoxPath; BoxPath.Reserve(25);
			for (int32 I = 0; I <= 24; ++I)
			{
				const double A = 2 * PI * I / 24;
				FVector2f P;
				if (At(Src.At + FVector2D(FMath::Cos(A), FMath::Sin(A)) * FSky::BoxRadius, P)) BoxPath.Add(P);
			}
			Path(BoxPath, bCard ? S47SkyGreen * .8f : S47SkyLine, 1.f, L + 1, bCard ? 0 : 2, nullptr);
			S47SkyBox(Out, L + 1, G, C - FVector2f(4, 4), FVector2f(8, 8), S47SkyGreen, false);
			PlaceName(bCard ? Src.Name + TEXT(" / CARD") : Src.Name, C, 18, S47SkyGreen, 12.f);
		}
		else
		{
			S47SkyLines(Out, L + 1, G, { C - FVector2f(4, 0), C + FVector2f(4, 0) }, S47SkyLine, 1.5f);
			S47SkyLines(Out, L + 1, G, { C - FVector2f(0, 4), C + FVector2f(0, 4) }, S47SkyLine, 1.5f);
			PlaceName(Src.Name, C, 16, S47SkyGreen * .62f, 7.f);
		}
	}

	// ---- the bearings: one line an antenna, each arm's a fan from the rim that closes on the fix: solid where the arm has locked,
	// a faint sweeping one with a dashed middle where it has not ----
	FVector2f Xc = Rim;
	At(K.Cross(), Xc);
	if (bBearings)
	{
		// which way each arm's fan comes in from: three ways a third of a turn apart on the rule's chart (headings 205, 85 and
		// 325 degrees there), which on the glass are the upper left, the right and the foot
		static const double FanFrom[FSky::ArmCount] = { -1., 1., -1. };
		const int32 PerArm = FSky::DishCount / FSky::ArmCount;
		TArray<FVector2f> Ray;
		for (int32 I = 0; I < FSky::DishCount; ++I)
		{
			const FSky::FLine Ln = K.Line(I);
			if (!Ln.bOn) continue;
			const FVector2D Along = Ln.Dir * FanFrom[FMath::Clamp(Ln.Arm, 0, FSky::ArmCount - 1)];
			// past its nearest point to the crossing it runs on a little: far out the three fans cross and make the triangle,
			// on the lock they all end on the fix
			const double Over = FMath::Clamp(1.6 * FMath::Abs(Ln.Off) + .8, .8, 14.);
			// Inside the array's window the line is the rule's own (straight on the rule's chart, so an arc on the sphere); from
			// the window's edge it is carried straight on across the glass to the rim, so that each arm's fan comes in from the rim.
			FVector2D From = Ln.Through - Along * Over, Until = Ln.Through + Along * 400.;
			if (!FSky::ClipSegment(From, Until)) continue;
			const int32 Pieces = FMath::Clamp(int32(FVector2D::Distance(From, Until) / 3.) + 1, 2, 60);
			Ray.Reset();
			for (int32 Piece = 0; Piece <= Pieces; ++Piece)
			{
				FVector2f P;
				if (At(FMath::Lerp(From, Until, double(Piece) / Pieces), P)) Ray.Add(P);
			}
			if (Ray.Num() >= 2)
			{
				const FVector2f Onward = (Ray.Last() - Ray[Ray.Num() - 2]).GetSafeNormal();
				Ray.Add(Ray.Last() + Onward * ((Semi.X + Semi.Y) * 2.f)); // (the path is cut at the rim)
			}
			const bool bMiddle = I % PerArm == PerArm / 2;
			if (Ln.bSolid) Path(Ray, S47SkyFade(S47SkyGreen, (bMiddle ? 1.f : .66f) * Gain), bMiddle ? 1.8f : 1.1f, L + 2, bMiddle ? 1 : 0, nullptr);
			else Path(Ray, S47SkyFade(S47SkyGreen, bMiddle ? .8f : .26f), bMiddle ? 1.6f : 1.f, L + 2, bMiddle ? 2 : 0, nullptr);
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
					FVector2f P;
					if (At(FMath::Lerp(Corner[I], Corner[(I + 1) % FSky::ArmCount], Part / 5.), P)) SidePath.Add(P);
				}
				Path(SidePath, S47SkyAmber, 1.8f, L + 3, 1, nullptr);
			}
		}
	}

	for (const FNameAt& Each : Names) S47SkyText(Out, L + 2, G, Each.Name, Each.Spot.X, Each.Spot.Y, Each.Size, Each.Tone);

	// ---- the search sweep: a slow hand round the sphere with its fading trail, while the search is on ----
	if (bSearch)
	{
		const double Turn = S47RxLook::Sweep(Now, bCalm);
		for (int32 Tail = 0; Turn >= 0 && Tail < 14; ++Tail)
		{
			const float A = float(2 * PI * Turn) - Tail * .026f;
			S47SkyLines(Out, L + 3, G, { Rim, Rim + FVector2f(FMath::Cos(A) * Semi.X, FMath::Sin(A) * Semi.Y) }, S47SkyFade(S47SkyGreen, Tail == 0 ? .32f : .13f * (1.f - Tail / 14.f)), Tail == 0 ? 1.5f : 2.6f);
		}
	}

	// ---- the lock sequence: three rings closing on the crossing, one an arm ----
	if (bLocking || bLocked)
	{
		static const float Final[FSky::ArmCount] = { 9.f, 15.f, 21.f };
		for (int32 Ring = 0; Ring < FSky::ArmCount; ++Ring)
		{
			const double Due = FSky::LockSeconds * (Ring + 1) / (FSky::ArmCount + 1);
			const float Part = bLocked ? 1.f : float(FMath::Clamp(K.LockProgress() * FSky::LockSeconds / Due, 0., 1.));
			const float Ease = 1.f - FMath::Pow(1.f - Part, 3.f);
			// Unusual: the last ring will not come in, and stands open and dashed.
			const bool bOpen = K.Odd() && Ring == FSky::ArmCount - 1;
			const float End = bOpen ? 31.f + (bCalm ? 0.f : 2.5f * float(FMath::Sin(Now * 1.3 * 2 * PI * .5))) : Final[Ring];
			const float Radius = FMath::Lerp(72.f + 26.f * Ring, End, Ease);
			S47SkyRing(Out, L + 3, G, Xc, Radius, S47SkyFade(S47SkyAmber, Part >= 1 ? 1.f : .75f), Part >= 1 && !bOpen ? 2.2f : 1.4f, bOpen);
		}
		if (bLocked)
		{
			if (K.Odd())
			{
				// a point the catalogue does not list: a bare diamond, no name
				S47SkyLines(Out, L + 4, G, { Xc + FVector2f(0, -7), Xc + FVector2f(7, 0), Xc + FVector2f(0, 7), Xc + FVector2f(-7, 0), Xc + FVector2f(0, -7) }, S47SkyAmber, 2.f);
			}
			else
			{
				S47SkyGlow(Out, L + 4, G, { Xc - FVector2f(5, 0), Xc + FVector2f(5, 0) }, S47SkyAmber, 1.8f);
				S47SkyGlow(Out, L + 4, G, { Xc - FVector2f(0, 5), Xc + FVector2f(0, 5) }, S47SkyAmber, 1.8f);
			}
			// the lock moment: rings go out from the fix, one after the other, and fade
			for (int32 Ring = 0; Ring < 3; ++Ring)
			{
				float Grow = 0, Alpha = 0;
				if (S47RxLook::Pulse(SinceLock, Ring, bCalm, Grow, Alpha)) S47SkyRing(Out, L + 3, G, Xc, 24.f + 120.f * Grow, S47SkyFade(S47SkyAmber, .55f * Alpha), 1.4f);
			}
		}
	}

	// ---- the reticle: rings and ticks, four corners that close as it nears the crossing, a filled centre on it ----
	{
		FVector2f R = Rim;
		At(K.ReticleAt(), R);
		const float Near = bBearings ? float(K.Closeness()) : 0.f;
		const bool bOn = bLocking || bLocked || (bBearings && K.Steady() && K.Distance() <= FSky::LockRadius);
		const FLinearColor Col = bOn ? S47SkyAmber : S47SkyGreen;
		const float Gap = bLocked ? 28.f : FMath::Lerp(30.f, 11.f, Near * Near), Leg = 9.f, W = bOn ? 2.6f : 2.f;
		// where it has just been: the phosphor's short memory
		if (Trail.IsEmpty() || FVector2f::Distance(Trail.Last(), R) > 2.f) { Trail.Add(R); TrailAt.Add(Now); }
		while (Trail.Num() > 0 && (Now - TrailAt[0] > .45 || Trail.Num() > 24)) { Trail.RemoveAt(0); TrailAt.RemoveAt(0); }
		for (int32 I = 0; I + 1 < Trail.Num(); ++I)
		{
			const float Left = float(FMath::Exp(-(Now - TrailAt[I]) / .16));
			if (InRim(Trail[I], 1.)) S47SkyRing(Out, L + 3, G, Trail[I], 5.f, S47SkyFade(Col, .3f * Left), 1.f);
		}
		for (const FVector2f Q : { FVector2f(-1, -1), FVector2f(1, -1), FVector2f(1, 1), FVector2f(-1, 1) })
		{
			const FVector2f Kn = R + Q * Gap;
			S47SkyGlow(Out, L + 4, G, { Kn - FVector2f(Q.X * Leg, 0), Kn, Kn - FVector2f(0, Q.Y * Leg) }, Col, W);
		}
		// two rings inside the corners, the outer one with a tick every thirty degrees, and the hairs toward the centre
		S47SkyRing(Out, L + 4, G, R, Gap * .62f, S47SkyFade(Col, .85f), 1.3f);
		S47SkyRing(Out, L + 4, G, R, Gap * .98f, S47SkyFade(Col, .45f), 1.f);
		for (int32 Tick = 0; Tick < 12; ++Tick)
		{
			const float A = 2 * PI * Tick / 12;
			const FVector2f D(FMath::Cos(A), FMath::Sin(A));
			const float Inner = Tick % 3 == 0 ? Gap * .3f : Gap * .86f;
			S47SkyLines(Out, L + 4, G, { R + D * Inner, R + D * (Gap * .98f) }, S47SkyFade(Col, Tick % 3 == 0 ? .8f : .5f), 1.f);
		}
		if (!bLocked) S47SkyBox(Out, L + 4, G, R - FVector2f(bOn ? 3.f : 1.5f, bOn ? 3.f : 1.5f), bOn ? FVector2f(6, 6) : FVector2f(3, 3), Col, true);
		// held on the crossing: a short bar under the reticle fills before the lock starts
		if (bSearch && K.HoldProgress() > 0)
		{
			S47SkyBox(Out, L + 4, G, R + FVector2f(-22, Gap + 8), FVector2f(44, 6), S47SkyAmber, false);
			S47SkyBox(Out, L + 4, G, R + FVector2f(-20, Gap + 10), FVector2f(40 * float(K.HoldProgress()), 2), S47SkyAmber, true);
		}
		// the hint: chevrons from the reticle toward the crossing
		if (K.HintShown() && K.Steady())
		{
			const FVector2f To = Xc - R; const float Far = To.Size();
			if (Far > Gap + 70)
			{
				const FVector2f D = To / Far, N(-D.Y, D.X);
				for (int32 I = 0; I < 3; ++I)
				{
					const FVector2f Mid = R + D * (Gap + 22 + 16 * I);
					S47SkyLines(Out, L + 4, G, { Mid - D * 5 + N * 7, Mid + D * 5, Mid - D * 5 - N * 7 }, S47SkyFade(S47SkyAmber, Breath(1.1, -.22 * I)), 2.2f);
				}
			}
		}
		// the operator's own pointer, while it is inside the rim
		if (bHover && K.HasPointer() && !bLocking && !bLocked)
		{
			S47SkyLines(Out, L + 4, G, { HoverLocal - FVector2f(6, 0), HoverLocal + FVector2f(6, 0) }, S47SkyGreen * .7f, 1.5f);
			S47SkyLines(Out, L + 4, G, { HoverLocal - FVector2f(0, 6), HoverLocal + FVector2f(0, 6) }, S47SkyGreen * .7f, 1.5f);
		}
	}

	// ---- the corners: what the picture is, how the bearings stand, and where the reticle is in the sky ----
	{
		const FLinearColor Quiet = S47SkyGreen * .8f;
		const TCHAR* Word = State == FSky::EPhase::NoBearings ? TEXT("NO BEARINGS") : bLocked ? TEXT("FIX HELD") : bLocking ? TEXT("LOCKING") : !K.Steady() ? TEXT("ARRAY TURNING")
			: K.HoldProgress() > 0 ? TEXT("ON THE CROSSING") : TEXT("SEARCHING");
		constexpr int32 Size = 16;
		float Y = 2;
		auto Left = [&](const FString& S, const FLinearColor& C) { S47SkyText(Out, L + 5, G, S, 4, Y, Size, C); Y += Size; };
		auto Right = [&](const FString& S, const FLinearColor& C) { S47SkyText(Out, L + 5, G, S, Sz.X - S.Len() * Size * .4f - 5, Y, Size, C); Y += Size; };
		Left(TEXT("REF. EPOCH: B1950.0"), Quiet);
		Left(TEXT("PROJ: PERSPECTIVE"), Quiet);
		Left(FString(TEXT("MODE: ")) + (bLocking ? FString::Printf(TEXT("LOCKING %2d %%"), int32(K.LockProgress() * 100)) : FString(Word)), S47SkyAmber);
		Left(FString(TEXT("SRC: ")) + SourceName, Quiet);
		// top right: the bearings, each arm in words
		Y = 2;
		static const TCHAR* const ArmName[FSky::ArmCount] = { TEXT("A"), TEXT("B"), TEXT("C") };
		Right(bBearings ? FString::Printf(TEXT("BEARINGS %02d/%02d IN"), K.InCount(), K.OnCount()) : FString(TEXT("BEARINGS --/--")), S47SkyGreen);
		for (int32 Arm = 0; Arm < FSky::ArmCount; ++Arm)
		{
			const bool bSolid = K.ArmSolid(Arm);
			Right(FString::Printf(TEXT("ARM %s %s"), ArmName[Arm], !bBearings ? TEXT("--") : bSolid ? TEXT("LOCKED") : TEXT("SWEEPING")), bSolid ? S47SkyGreen : S47SkyGreen * .75f);
		}
		Right(bBearings ? FString::Printf(TEXT("SPREAD %4.1f\u00b0"), K.MeanOff()) : FString(TEXT("SPREAD --")), S47SkyGreen);
		// bottom left: the reticle's place, on the ground and in the sky (for the catalogues' epoch), and in the galaxy
		const FVector2D Aim = K.ReticleAt();
		double Ra = 0, Dec = 0;
		Sph::AzElToRaDec(Aim, Lst, Ra, Dec);
		double OldRa = Ra, OldDec = Dec, NewRa = Ra, NewDec = Dec, Lon = 0, Lat = 0;
		Sph::ToB1950(OldRa, OldDec);
		Sph::ToJ2000(NewRa, NewDec);
		Sph::EqToGal(NewRa, NewDec, Lon, Lat);
		Y = Sz.Y - FootRoom - 4 * Size - 2;
		Left(FString::Printf(TEXT("AZ %05.1f  EL %04.1f"), Aim.X, Aim.Y), S47SkyGreen);
		Left(FString(TEXT("RA  ")) + Sph::RaText(OldRa), S47SkyGreen);
		Left(FString(TEXT("DEC ")) + Sph::DecText(OldDec), S47SkyGreen);
		Left(FString::Printf(TEXT("L %06.2f  B %+06.2f"), Lon, Lat), Quiet);
		// bottom right: which way the pole and the east are on the picture
		FVector2D Here, Poleward, Eastward;
		if (Sph::ProjectHaDec(Pl.View, MidHa, MidDec, Here) && Sph::ProjectHaDec(Pl.View, MidHa, MidDec + 1., Poleward) && Sph::ProjectHaDec(Pl.View, MidHa - 1., MidDec, Eastward))
		{
			const FVector2f Hub(Sz.X - 34, Sz.Y - FootRoom - 34);
			const FVector2f Up = FVector2f(Poleward - Here).GetSafeNormal(), East = FVector2f(Eastward - Here).GetSafeNormal();
			S47SkyLines(Out, L + 5, G, { Hub - Up * 16, Hub + Up * 18 }, Quiet, 1.5f);
			S47SkyLines(Out, L + 5, G, { Hub + Up * 18 - Up * 6 + East * 4, Hub + Up * 18, Hub + Up * 18 - Up * 6 - East * 4 }, Quiet, 1.5f);
			S47SkyLines(Out, L + 5, G, { Hub - East * 14, Hub + East * 14 }, Quiet, 1.f);
			S47SkyText(Out, L + 5, G, TEXT("NCP"), Hub.X + Up.X * 26 - 9, Hub.Y + Up.Y * 26 - 9, 14, Quiet);
			S47SkyText(Out, L + 5, G, TEXT("E"), Hub.X + East.X * 21 - 3, Hub.Y + East.Y * 21 - 8, 14, Quiet);
			S47SkyText(Out, L + 5, G, TEXT("W"), Hub.X - East.X * 21 - 3, Hub.Y - East.Y * 21 - 8, 14, Quiet);
		}
	}

	// ---- the lock moment: the whole picture swells once, softly (a veil of the phosphor inside the rim) ----
	if (const float Swell = S47RxLook::Bloom(SinceLock, bCalm); Swell > 0)
	{
		constexpr int32 Strips = 26;
		for (int32 I = 0; I < Strips; ++I)
		{
			const float V = (I + .5f) / Strips * 2 - 1, Half = Semi.X * FMath::Sqrt(FMath::Max(0.f, 1 - V * V));
			S47SkyBox(Out, L + 6, G, FVector2f(Rim.X - Half, Rim.Y + (V - 1.f / Strips) * Semi.Y), FVector2f(Half * 2, Semi.Y * 2 / Strips + .5f), S47SkyFade(S47SkyGreen, .05f * Swell), true);
		}
	}
	return L + 7;
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
		// the rim inside the widget, with room for the declinations beside it and the hours under it
		const bool bRoom = Rim.X - Semi.X >= SideRoom - .01 && Rim.X + Semi.X <= Size.X - SideRoom + .01 && Rim.Y - Semi.Y >= TopRoom - .01 && Rim.Y + Semi.Y <= Size.Y - FootRoom + .01;
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
			FString::Printf(TEXT("%.0f x %.0f: the array's whole window inside the rim, to the glass and back open and closed in, the crossing standing still as the picture closes in (%.2f units a degree up, %.2f across)"), Size.X, Size.Y, FMath::DegreesToRadians(P.View.Radius), FMath::DegreesToRadians(P.View.Radius * P.View.Stretch)));
		// closed in, the rule's lock circle is a comfortable target under a pointer: at least nine units across, as on the flat chart
		if (Size.Y == 440.f) Check(Open * ZoomMost >= 4.5, FString::Printf(TEXT("the lock circle is at least %.1f units across on the open picture and %.1f closed in (the flat chart's promise: nine)"), Open * 2, Open * 2 * ZoomMost));
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
	// the tables as the picture uses them
	Check(S47SkyStarPlaces().Num() == int32(S47SkySphere::Star::Count) && S47SkyDustPlaces().Num() == S47SkySphere::DustCount, TEXT("every star of the table and every speck of the galactic plane has its place of date"));
	return bOk;
}
