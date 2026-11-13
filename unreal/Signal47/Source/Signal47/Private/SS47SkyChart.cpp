#include "SS47SkyChart.h"
#include "S47SkyAcquire.h"
#include "S47Keys.h"
#include "S47Kit.h"
#include "SS47Widgets.h"
#include "Brushes/SlateColorBrush.h"
#include "Rendering/DrawElements.h"

// RX-ACQUIRE-SKY-20261113 (-> M3): see SS47SkyChart.h.
namespace
{
	// The workstation's phosphor (S47FreeformTerminal's palette, which keeps its own copies).
	const FLinearColor S47SkyGreen = S47Kit::Css(0x8cffa4), S47SkyAmber = S47Kit::Css(0xffb84a), S47SkyLine = S47Kit::Css(0x3f9a5a), S47SkyDim = S47Kit::Css(0x1d4a2b), S47SkyBlack = S47Kit::Css(0x020a05);
	const FSlateBrush* S47SkyFill() { static const FSlateColorBrush B(FLinearColor::White); return &B; }
	FLinearColor S47SkyFade(FLinearColor C, float A) { C.A = A; return C; }
	void S47SkyText(FSlateWindowElementList& Out, int32 L, const FGeometry& G, const FString& S, float X, float Y, int32 Size, const FLinearColor& C)
	{
		FSlateDrawElement::MakeText(Out, L, G.ToPaintGeometry(FVector2f(420.f, float(Size) + 6), FSlateLayoutTransform(FVector2f(X, Y))), S, S47Style::Mono(Size), ESlateDrawEffect::None, C);
	}
	void S47SkyLines(FSlateWindowElementList& Out, int32 L, const FGeometry& G, const TArray<FVector2f>& P, const FLinearColor& C, float W = 1.5f)
	{
		FSlateDrawElement::MakeLines(Out, L, G.ToPaintGeometry(), P, ESlateDrawEffect::None, C, true, W);
	}
	void S47SkyDash(FSlateWindowElementList& Out, int32 L, const FGeometry& G, FVector2f A, FVector2f B, const FLinearColor& C, float Dash = 9, float W = 1.5f)
	{
		const float Len = FVector2f::Distance(A, B);
		if (Len < 1) return;
		const FVector2f D = (B - A) / Len;
		for (float T = 0; T < Len; T += Dash * 2) S47SkyLines(Out, L, G, { A + D * T, A + D * FMath::Min(Len, T + Dash) }, C, W);
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
}

void SS47SkyChart::Construct(const FArguments& Args)
{
	Sky = Args._Model; StepFn = Args._OnStep; CalmFn = Args._Calm; KeyFn = Args._OnKey;
	ForceVolatile(true);
}

SS47SkyChart::FPlot SS47SkyChart::PlotFor(const FVector2f& Size)
{
	// Elevation labels to the left, azimuth labels under it, the readouts in a column to the right.
	constexpr float Left = 44, Top = 8, Bottom = 26, Gap = 14;
	const double AzSpan = FS47SkyAcquire::AzMax - FS47SkyAcquire::AzMin, ElSpan = FS47SkyAcquire::ElMax - FS47SkyAcquire::ElMin;
	const float RoomX = FMath::Max(40.f, Size.X - Left - Gap - ReadoutWide), RoomY = FMath::Max(40.f, Size.Y - Top - Bottom);
	FPlot P;
	P.Scale = FMath::Min(RoomX / float(AzSpan), RoomY / float(ElSpan));
	P.Extent = FVector2f(float(AzSpan) * P.Scale, float(ElSpan) * P.Scale);
	P.Origin = FVector2f(Left, Top + (RoomY - P.Extent.Y) * .5f);
	P.ReadoutX = Left + P.Extent.X + Gap;
	return P;
}

FVector2f SS47SkyChart::ToLocal(const FPlot& P, const FVector2D& AzEl)
{
	return P.Origin + FVector2f(float(AzEl.X - FS47SkyAcquire::AzMin) * P.Scale, float(FS47SkyAcquire::ElMax - AzEl.Y) * P.Scale);
}

FVector2D SS47SkyChart::ToSky(const FPlot& P, const FVector2f& Local)
{
	const float S = FMath::Max(P.Scale, 1e-3f);
	return FVector2D(FS47SkyAcquire::AzMin + (Local.X - P.Origin.X) / S, FS47SkyAcquire::ElMax - (Local.Y - P.Origin.Y) / S);
}

bool SS47SkyChart::Contains(const FPlot& P, const FVector2f& Local)
{
	return Local.X >= P.Origin.X && Local.Y >= P.Origin.Y && Local.X <= P.Origin.X + P.Extent.X && Local.Y <= P.Origin.Y + P.Extent.Y;
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
	const FPlot P = PlotFor(FVector2f(G.GetLocalSize()));
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
	const FPlot P = PlotFor(FVector2f(G.GetLocalSize()));
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
	// over the plot the tube draws its own pointer mark while the search is on
	return bHover && Steerable() ? FCursorReply::Cursor(EMouseCursor::None) : FCursorReply::Unhandled();
}

int32 SS47SkyChart::OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const
{
	using FSky = FS47SkyAcquire;
	const FVector2f Sz(G.GetLocalSize());
	const FPlot Pl = PlotFor(Sz);
	const double Now = FPlatformTime::Seconds();
	const double Dt = LastPaint > 0 ? FMath::Clamp(Now - LastPaint, 0., .1) : 0.;
	LastPaint = Now;
	const FVector2f P0 = Pl.Origin, P1 = Pl.Origin + Pl.Extent;
	if (!Sky.IsValid())
	{
		S47SkyBox(Out, L, G, P0, Pl.Extent, S47SkyLine, false);
		return L + 1;
	}
	FSky& K = *Sky;
	K.Steer(Steerable() ? SteerNow() : FVector2D::ZeroVector);
	if (StepFn) StepFn(Dt);
	else K.Step(Dt);
	const bool bCalm = CalmFn && CalmFn();
	// A slow breath for small marks (never the picture): standing still with Calm flashes chosen.
	auto Breath = [&](double Hz, double Phase = 0.) { return bCalm ? 1.f : .72f + .28f * float(FMath::Sin((Now * Hz + Phase) * 2 * PI)); };
	auto At = [&](const FVector2D& AzEl) { return ToLocal(Pl, AzEl); };
	const FSky::EPhase State = K.State();
	const bool bLocked = State == FSky::EPhase::Locked, bLocking = State == FSky::EPhase::Locking, bSearch = State == FSky::EPhase::Search;
	const bool bBearings = K.HasBearings();

	// ---- the grid: azimuth across, elevation up, a tick every ten degrees ----
	for (int32 Az = 60; Az <= 160; Az += 10)
	{
		const float X = At(FVector2D(Az, FSky::ElMin)).X;
		S47SkyLines(Out, L, G, { { X, P0.Y }, { X, P1.Y } }, S47SkyDim, 1);
		if (Az % 20 == 0) S47SkyText(Out, L, G, FString::Printf(TEXT("%03d"), Az), X - 11, P1.Y + 4, 16, S47SkyGreen * .85f);
	}
	for (int32 El = 20; El <= 70; El += 10)
	{
		const float Y = At(FVector2D(FSky::AzMin, El)).Y;
		S47SkyLines(Out, L, G, { { P0.X, Y }, { P1.X, Y } }, S47SkyDim, 1);
		S47SkyText(Out, L, G, FString::Printf(TEXT("%02d"), El), P0.X - 24, Y - 9, 16, S47SkyGreen * .85f);
	}
	S47SkyBox(Out, L, G, P0, Pl.Extent, S47SkyLine, false);
	S47SkyText(Out, L, G, TEXT("EL"), 2, P0.Y - 6, 16, S47SkyGreen * .85f);
	S47SkyText(Out, L, G, TEXT("AZ"), P1.X - 46, P1.Y + 4, 16, S47SkyGreen * .85f);

	// ---- the catalogue: the card sources as squares in their boxes, the minor field as small crosses, each with its name ----
	for (const FSky::FSource& Src : FSky::Catalogue())
	{
		const FVector2f C = At(Src.At);
		if (Src.bMajor)
		{
			const bool bCard = Src.Run == K.CardRun();
			S47SkyRing(Out, L + 1, G, C, float(FSky::BoxRadius) * Pl.Scale, bCard ? S47SkyGreen * .8f : S47SkyLine, 1.f, !bCard);
			S47SkyBox(Out, L + 1, G, C - FVector2f(4, 4), FVector2f(8, 8), S47SkyGreen, false);
			const FString Name = bCard ? Src.Name + TEXT(" / CARD") : Src.Name;
			const float Wide = Name.Len() * 7.4f;
			const float X = C.X + 24 + Wide > P1.X ? C.X - 24 - Wide : C.X + 24;
			S47SkyText(Out, L + 1, G, Name, X, C.Y - 24, 18, S47SkyGreen);
		}
		else
		{
			S47SkyLines(Out, L + 1, G, { C - FVector2f(4, 0), C + FVector2f(4, 0) }, S47SkyLine, 1.5f);
			S47SkyLines(Out, L + 1, G, { C - FVector2f(0, 4), C + FVector2f(0, 4) }, S47SkyLine, 1.5f);
			const float Wide = Src.Name.Len() * 6.6f;
			S47SkyText(Out, L + 1, G, Src.Name, C.X + 8 + Wide > P1.X ? C.X - 8 - Wide : C.X + 8, C.Y - 8, 16, S47SkyGreen * .62f);
		}
	}

	// ---- the bearings: one line an antenna, a solid band where an arm has locked, a faint sweeping one where it has not ----
	const FVector2f Xc = At(K.Cross());
	if (bBearings)
	{
		const int32 PerArm = FSky::DishCount / FSky::ArmCount;
		for (int32 I = 0; I < FSky::DishCount; ++I)
		{
			const FSky::FLine Ln = K.Line(I);
			if (!Ln.bOn) continue;
			FVector2D A = Ln.Through - Ln.Dir * 400., B = Ln.Through + Ln.Dir * 400.;
			if (!FSky::ClipSegment(A, B)) continue;
			const FVector2f LA = At(A), LB = At(B);
			const bool bMiddle = I % PerArm == PerArm / 2;
			if (Ln.bSolid)
			{
				if (bMiddle) S47SkyLines(Out, L + 2, G, { LA, LB }, S47SkyFade(S47SkyGreen, .16f), 5.f);
				S47SkyLines(Out, L + 2, G, { LA, LB }, S47SkyFade(S47SkyGreen, bMiddle ? 1.f : .7f), bMiddle ? 2.f : 1.2f);
			}
			else if (bMiddle) S47SkyDash(Out, L + 2, G, LA, LB, S47SkyFade(S47SkyGreen, .8f), 9, 1.6f);
			else S47SkyLines(Out, L + 2, G, { LA, LB }, S47SkyFade(S47SkyGreen, .26f), 1.f);
		}
		// the triangle the three bands make: what has to close
		FVector2D Corner[FSky::ArmCount];
		if (K.Triangle(Corner) && K.MeanOff() > .12)
		{
			for (int32 I = 0; I < FSky::ArmCount; ++I)
			{
				FVector2D A = Corner[I], B = Corner[(I + 1) % FSky::ArmCount];
				if (FSky::ClipSegment(A, B)) S47SkyLines(Out, L + 3, G, { At(A), At(B) }, S47SkyAmber, 2.2f);
			}
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
				S47SkyLines(Out, L + 4, G, { Xc - FVector2f(5, 0), Xc + FVector2f(5, 0) }, S47SkyAmber, 2.f);
				S47SkyLines(Out, L + 4, G, { Xc - FVector2f(0, 5), Xc + FVector2f(0, 5) }, S47SkyAmber, 2.f);
			}
		}
	}

	// ---- the reticle: four corners that close as it nears the crossing, a filled centre on it ----
	{
		const FVector2f R = At(K.ReticleAt());
		const float Near = bBearings ? float(K.Closeness()) : 0.f;
		const bool bOn = bLocking || bLocked || (bBearings && K.Steady() && K.Distance() <= FSky::LockRadius);
		const FLinearColor Col = bOn ? S47SkyAmber : S47SkyGreen;
		const float Gap = bLocked ? 28.f : FMath::Lerp(30.f, 11.f, Near * Near), Leg = 9.f, W = bOn ? 2.6f : 2.f;
		for (const FVector2f Q : { FVector2f(-1, -1), FVector2f(1, -1), FVector2f(1, 1), FVector2f(-1, 1) })
		{
			const FVector2f Kn = R + Q * Gap;
			S47SkyLines(Out, L + 4, G, { Kn - FVector2f(Q.X * Leg, 0), Kn, Kn - FVector2f(0, Q.Y * Leg) }, Col, W);
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
		// the operator's own pointer, while it is on the chart
		if (bHover && K.HasPointer() && !bLocking && !bLocked)
		{
			S47SkyLines(Out, L + 4, G, { HoverLocal - FVector2f(6, 0), HoverLocal + FVector2f(6, 0) }, S47SkyGreen * .7f, 1.5f);
			S47SkyLines(Out, L + 4, G, { HoverLocal - FVector2f(0, 6), HoverLocal + FVector2f(0, 6) }, S47SkyGreen * .7f, 1.5f);
		}
	}

	// ---- the readouts, to the right of the chart ----
	{
		const float X = Pl.ReadoutX; float Y = P0.Y - 4;
		auto Row = [&](const FString& S, const FLinearColor& C, int32 Size = 18) { S47SkyText(Out, L + 5, G, S, X, Y, Size, C); Y += Size + 2; };
		const TCHAR* Word = State == FSky::EPhase::NoBearings ? TEXT("NO BEARINGS") : bLocked ? TEXT("FIX HELD") : bLocking ? TEXT("LOCKING") : !K.Steady() ? TEXT("ARRAY TURNING")
			: K.HoldProgress() > 0 ? TEXT("ON THE CROSSING") : TEXT("SEARCHING");
		Row(bLocking ? FString::Printf(TEXT("LOCKING %2d %%"), int32(K.LockProgress() * 100)) : FString(Word), S47SkyAmber, 22);
		const FVector2D R = K.ReticleAt(); double Ha = 0, Dec = 0;
		FSky::HourAngleDec(R, Ha, Dec);
		const int32 HaMin = FMath::RoundToInt32(FMath::Abs(Ha) * 60);
		Row(TEXT("PHASE CENTRE"), S47SkyLine * 1.5f);
		Row(FString::Printf(TEXT("AZ %05.1f   EL %04.1f"), R.X, R.Y), S47SkyGreen);
		Row(FString::Printf(TEXT("HA %c%02dH%02dM DEC %+05.1f"), Ha < 0 ? TEXT('-') : TEXT('+'), HaMin / 60, HaMin % 60, Dec), S47SkyGreen);
		Y += 6;
		Row(bBearings ? FString::Printf(TEXT("BEARINGS %02d/%02d IN"), K.InCount(), K.OnCount()) : FString(TEXT("BEARINGS --/--")), S47SkyGreen);
		for (int32 Arm = 0; Arm < FSky::ArmCount; ++Arm)
		{
			const bool bSolid = K.ArmSolid(Arm);
			Row(FString::Printf(TEXT("ARM %c  %s"), TCHAR(TEXT('A') + Arm), !bBearings ? TEXT("--") : bSolid ? TEXT("LOCKED") : TEXT("SWEEPING")), bSolid ? S47SkyGreen : S47SkyGreen * .75f);
		}
		Row(bBearings ? FString::Printf(TEXT("SPREAD %4.1f DEG"), K.MeanOff()) : FString(TEXT("SPREAD --")), S47SkyGreen);
		if (bLocking || bLocked)
		{
			Y += 6;
			const double Part = K.LockProgress();
			for (int32 Arm = 0; Arm < FSky::ArmCount; ++Arm)
				Row(FString::Printf(TEXT("DELAY %c %+07.1f NS"), TCHAR(TEXT('A') + Arm), K.ArmDelayNs(Arm) * Part), S47SkyGreen);
			Row(FString::Printf(TEXT("S/N %4.1f DB"), K.SignalToNoise() * Part), S47SkyGreen);
			Row(FString::Printf(TEXT("RESIDUAL %.2f DEG"), K.MeanOff()), S47SkyGreen);
			if (Part > .75)
			{
				const FSky::FSource* Listed = K.NearestCatalogued(K.Cross(), true);
				Row(FString(TEXT("CAT ")) + (Listed ? Listed->Name : FString(TEXT("--"))), S47SkyGreen);
				Row(FString::Printf(TEXT("OFFSET %.1f DEG"), K.CatalogueOffset()), S47SkyGreen);
			}
			else { Row(TEXT("CAT ........"), S47SkyGreen * .75f); Row(TEXT("OFFSET ...."), S47SkyGreen * .75f); }
		}
	}
	return L + 6;
}

bool SS47SkyChart::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bPass, const FString& What) { bOk &= bPass; Lines.Add(FString(bPass ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("sky chart: ") + What); };
	for (const FVector2f Size : { FVector2f(1040, 384), FVector2f(1047, 384), FVector2f(900, 300), FVector2f(1200, 600) })
	{
		const FPlot P = PlotFor(Size);
		const FVector2f A = ToLocal(P, FVector2D(FS47SkyAcquire::AzMin, FS47SkyAcquire::ElMax)), B = ToLocal(P, FVector2D(FS47SkyAcquire::AzMax, FS47SkyAcquire::ElMin));
		const FVector2D Back = ToSky(P, ToLocal(P, FVector2D(112, 54)));
		const bool bFits = A.Equals(P.Origin, .01f) && B.Equals(P.Origin + P.Extent, .01f) && P.Origin.X >= 40 && P.Origin.Y >= 8 && B.Y <= Size.Y - 25.9f && P.ReadoutX + ReadoutWide <= Size.X + .01f && P.ReadoutX > B.X;
		// the lock radius is a comfortable target under a pointer: at least nine units across on the glass
		Check(bFits && Back.Equals(FVector2D(112, 54), 1e-3) && Contains(P, ToLocal(P, FVector2D(112, 54))) && !Contains(P, P.Origin - FVector2f(2, 2)) && FS47SkyAcquire::LockRadius * P.Scale >= 4.5f,
			FString::Printf(TEXT("%.0f x %.0f: the chart's window at one scale (%.2f units a degree), elevation up, with the readouts beside it"), Size.X, Size.Y, P.Scale));
	}
	Check(SteerOf(EKeys::Up) == FVector2D(0, 1) && SteerOf(EKeys::Down) == FVector2D(0, -1) && SteerOf(EKeys::Left) == FVector2D(-1, 0) && SteerOf(EKeys::Right) == FVector2D(1, 0)
		&& SteerOf(EKeys::Gamepad_DPad_Up) == FVector2D(0, 1) && SteerOf(EKeys::Gamepad_DPad_Left) == FVector2D(-1, 0) && SteerOf(EKeys::Escape).IsZero(),
		TEXT("the arrow keys and the d-pad steer; Escape stays the way out"));
	return bOk;
}
