#include "SS47RxLook.h"
#include "S47RxLook.h"
#include "S47SkyAcquire.h"
#include "S47Kit.h"
#include "SS47Widgets.h"
#include "Brushes/SlateColorBrush.h"
#include "Rendering/DrawElements.h"

// RX-LOOK-20261113 (-> M3, M6): see SS47RxLook.h.
namespace
{
	// The workstation's phosphor (S47FreeformTerminal's palette, which keeps its own copies).
	const FLinearColor S47RxLGreen = S47Kit::Css(0x8cffa4), S47RxLAmber = S47Kit::Css(0xffb84a), S47RxLLine = S47Kit::Css(0x3f9a5a), S47RxLDim = S47Kit::Css(0x1d4a2b), S47RxLHot = S47Kit::Css(0xfff0d0);
	const FSlateBrush* S47RxLFill() { static const FSlateColorBrush B(FLinearColor::White); return &B; }
	FLinearColor S47RxLFade(FLinearColor C, float A) { C.A = A; return C; }
	// VT323: a letter is about four tenths of its size wide.
	float S47RxLWide(int32 Chars, int32 Size) { return Chars * Size * .4f; }
	const FS47RxLive& S47RxLRead(const TSharedPtr<FS47RxFeed>& Feed) { static const FS47RxLive Empty; return Feed.IsValid() ? Feed->Now() : Empty; }

	// The tube's pen for one widget's paint. A bright stroke is drawn as the beam leaves it: a wide faint pass, a middle one and
	// the thin bright core; a bright word has the same word faintly round it.
	struct FS47RxLPen
	{
		FSlateWindowElementList& Out; const FGeometry& G; int32 L;
		void Text(const FString& S, float X, float Y, int32 Size, const FLinearColor& C, int32 Up = 0) const
		{
			FSlateDrawElement::MakeText(Out, L + Up, G.ToPaintGeometry(FVector2f(600.f, float(Size) + 6), FSlateLayoutTransform(FVector2f(X, Y))), S, S47Style::Mono(Size), ESlateDrawEffect::None, C);
		}
		void Halo(const FString& S, float X, float Y, int32 Size, const FLinearColor& C, int32 Up = 0, float Strength = .15f) const
		{
			const float Off = FMath::Max(1.f, Size / 26.f);
			const FLinearColor Soft = S47RxLFade(C, Strength);
			for (const FVector2f D : { FVector2f(-Off, 0), FVector2f(Off, 0), FVector2f(0, -Off), FVector2f(0, Off) }) Text(S, X + D.X, Y + D.Y, Size, Soft, Up);
			Text(S, X, Y, Size, C, Up);
		}
		void Line(const TArray<FVector2f>& P, const FLinearColor& C, float W = 1.5f, int32 Up = 0) const
		{
			if (P.Num() >= 2) FSlateDrawElement::MakeLines(Out, L + Up, G.ToPaintGeometry(), P, ESlateDrawEffect::None, C, true, W);
		}
		void Glow(const TArray<FVector2f>& P, const FLinearColor& C, float W = 1.6f, int32 Up = 0) const
		{
			const float Wide = S47RxLook::Beam(W, C.A);
			Line(P, S47RxLFade(C, C.A * .10f), Wide * 4.2f, Up);
			Line(P, S47RxLFade(C, C.A * .26f), Wide * 2.1f, Up);
			Line(P, C, Wide, Up);
		}
		void Box(FVector2f At, FVector2f Size, const FLinearColor& C, int32 Up = 0) const
		{
			if (Size.X > 0 && Size.Y > 0) FSlateDrawElement::MakeBox(Out, L + Up, G.ToPaintGeometry(Size, FSlateLayoutTransform(At)), S47RxLFill(), ESlateDrawEffect::None, C);
		}
		void Rect(FVector2f At, FVector2f Size, const FLinearColor& C, float W = 1.5f, int32 Up = 0, bool bGlow = false) const
		{
			const TArray<FVector2f> P = { At, At + FVector2f(Size.X, 0), At + Size, At + FVector2f(0, Size.Y), At };
			if (bGlow) Glow(P, C, W, Up);
			else Line(P, C, W, Up);
		}
		void Ring(FVector2f At, float Radius, const FLinearColor& C, float W = 1.2f, int32 Up = 0) const
		{
			constexpr int32 Steps = 36;
			TArray<FVector2f> P; P.Reserve(Steps + 1);
			for (int32 I = 0; I <= Steps; ++I) { const float A = 2 * PI * I / Steps; P.Add(At + FVector2f(FMath::Cos(A), FMath::Sin(A)) * Radius); }
			Line(P, C, W, Up);
		}
	};

	// The banner's letters are written once when the state changes, not every time a page is built again (a tab, a key): what
	// was last written and when is kept here for the one receiver there is.
	FString S47RxLBannerShown;
	double S47RxLBannerAt = -1e9;

	// The waterfall's rows, kept for the one receiver there is, so that a tab changed does not start the picture again.
	constexpr int32 S47RxLFallBins = 40, S47RxLFallMost = 44;
	constexpr double S47RxLFallSeconds = .22;
	struct FS47RxLFall { TArray<S47RxLook::FFallRow> Rows; uint32 Next = 0; double At = -1e9, ElapsedWas = 0; };
	FS47RxLFall& S47RxLFallState() { static FS47RxLFall S; return S; }
	// One step of the waterfall's clock: rows written since the last, each as the receiver was when it was written. A tick of the
	// take is marked in the row that was written as the tape passed it, and only while the take is being recorded.
	void S47RxLFallStep(FS47RxLFall& F, const FS47RxLive& Live, double T)
	{
		if (F.Rows.Num() < S47RxLFallMost || T - F.At > 2.5 || T < F.At)
		{
			// first sight, or the terminal was away: the whole picture as the receiver is now
			F.Rows.Reset();
			for (int32 I = 0; I < S47RxLFallMost; ++I) F.Rows.Add(S47RxLook::FallRow(Live.Scope, F.Next++, false));
			F.At = T; F.ElapsedWas = Live.Elapsed;
			return;
		}
		for (int32 Guard = 0; Guard < 8 && T - F.At >= S47RxLFallSeconds; ++Guard)
		{
			bool bTick = false;
			if (Live.bRecording) for (const double Tick : Live.Ticks) bTick |= Tick > F.ElapsedWas && Tick <= Live.Elapsed;
			F.ElapsedWas = Live.Elapsed;
			F.Rows.RemoveAt(0);
			F.Rows.Add(S47RxLook::FallRow(Live.Scope, F.Next++, bTick));
			F.At += S47RxLFallSeconds;
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------

void SS47RxHeader::Construct(const FArguments& Args)
{
	Live = Args._Feed;
	ForceVolatile(true);
}

int32 SS47RxHeader::OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const
{
	const FVector2f Sz(G.GetLocalSize());
	const FS47RxLive& Now = S47RxLRead(Live);
	const FS47RxLPen Pen{ Out, G, L };
	// ---- the emblem: a dish on its mount, in strokes ----
	{
		const FVector2f Pivot(33, 31), Axis(-.64f, -.77f), Across(.77f, -.64f);
		TArray<FVector2f> Dish; Dish.Reserve(17);
		for (int32 I = -8; I <= 8; ++I) { const float U = I / 8.f; Dish.Add(Pivot + Across * (U * 23.f) + Axis * (U * U * 9.f)); }
		const FVector2f Focus = Pivot + Axis * 15.f;
		Pen.Line({ FVector2f(12, 55), FVector2f(54, 55) }, S47RxLGreen, 2.f);
		Pen.Glow({ FVector2f(22, 55), Pivot, FVector2f(44, 55) }, S47RxLGreen, 1.6f);
		Pen.Line({ FVector2f(27, 44), FVector2f(39, 44) }, S47RxLLine, 1.2f);
		Pen.Line({ Dish[0], Dish.Last() }, S47RxLLine, 1.f);
		Pen.Line({ Pivot, Dish[4] }, S47RxLLine, 1.f); Pen.Line({ Pivot, Dish[12] }, S47RxLLine, 1.f);
		Pen.Line({ Dish[0], Focus, Dish.Last() }, S47RxLFade(S47RxLGreen, .8f), 1.2f);
		Pen.Line({ Pivot, Focus }, S47RxLFade(S47RxLGreen, .8f), 1.2f);
		Pen.Glow(Dish, S47RxLGreen, 2.2f, 1);
		Pen.Box(Focus - FVector2f(2.5f, 2.5f), FVector2f(5, 5), S47RxLAmber, 1);
	}
	// ---- SARO OPS, the station's name as the gate sign has it, the terminal ----
	Pen.Halo(TEXT("SARO OPS"), 72, 0, 54, S47RxLGreen, 1);
	const float BarA = 72 + S47RxLWide(8, 54) + 12;
	Pen.Line({ FVector2f(BarA, 6), FVector2f(BarA, 52) }, S47RxLLine, 1.5f);
	static const TCHAR* const Name[3] = { TEXT("SOUTHWEST ASTRONOMICAL"), TEXT("RESEARCH OBSERVATORY"), TEXT("NEW MEXICO") };
	for (int32 I = 0; I < 3; ++I) Pen.Text(Name[I], BarA + 12, 5 + 17.f * I, 16, S47RxLFade(S47RxLGreen, .92f));
	const float BarB = BarA + 12 + S47RxLWide(22, 16) + 12;
	Pen.Line({ FVector2f(BarB, 6), FVector2f(BarB, 52) }, S47RxLLine, 1.5f);
	{
		// "V4.2  TERM 3  OPR NIGHT": a part a line
		TArray<FString> Parts;
		Now.Term.ParseIntoArray(Parts, TEXT("  "), true);
		for (int32 I = 0; I < Parts.Num() && I < 3; ++I) Pen.Text(Parts[I].TrimStartAndEnd(), BarB + 12, 5 + 17.f * I, 16, S47RxLFade(S47RxLGreen, .8f));
	}
	// ---- the date and clock, and the array's own pointing ----
	const FString Stn = Now.bPointing ? FString::Printf(TEXT("STN: SARO  EL: %.1f°  AZ: %05.1f°"), Now.El, Now.Az) : FString(TEXT("STN: SARO  EL: --.-  AZ: ---.-"));
	const float RightWide = FMath::Max(S47RxLWide(Now.Clock.Len(), 28), S47RxLWide(Stn.Len(), 18));
	Pen.Halo(Now.Clock, Sz.X - S47RxLWide(Now.Clock.Len(), 28) - 2, 1, 28, S47RxLGreen, 1, .12f);
	Pen.Text(Stn, Sz.X - S47RxLWide(Stn.Len(), 18) - 2, 33, 18, S47RxLFade(S47RxLGreen, .9f));
	const float BarC = Sz.X - RightWide - 16;
	if (BarC > BarB + 120) Pen.Line({ FVector2f(BarC, 6), FVector2f(BarC, 52) }, S47RxLLine, 1.5f);
	// ---- the double rule under it ----
	Pen.Line({ FVector2f(0, Sz.Y - 5), FVector2f(Sz.X, Sz.Y - 5) }, S47RxLGreen, 1.6f);
	Pen.Line({ FVector2f(0, Sz.Y - 1.5f), FVector2f(Sz.X, Sz.Y - 1.5f) }, S47RxLLine, 1.f);
	return L + 2;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void SS47RxBanner::Construct(const FArguments& Args)
{
	Live = Args._Feed;
	ForceVolatile(true);
}

int32 SS47RxBanner::OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const
{
	const FVector2f Sz(G.GetLocalSize());
	const FS47RxLive& Now = S47RxLRead(Live);
	const FS47RxLPen Pen{ Out, G, L };
	const double T = FPlatformTime::Seconds();
	// the thin double frame
	Pen.Rect(FVector2f(1, 1), Sz - FVector2f(2, 2), S47RxLLine, 1.5f);
	Pen.Rect(FVector2f(4, 4), Sz - FVector2f(8, 8), S47RxLDim, 1.f);
	// ---- the state: the word, and amber for the live and locked ones ----
	const float StateWide = FMath::Min(344.f, Sz.X * .34f);
	const FLinearColor Tone = Now.State.bAmber ? S47RxLAmber : S47RxLGreen;
	Pen.Rect(FVector2f(7, 7), FVector2f(StateWide, Sz.Y - 14), Tone, 2.f, 1, true);
	if (Now.State.Text != S47RxLBannerShown) { S47RxLBannerShown = Now.State.Text; S47RxLBannerAt = T; }
	const int32 Len = Now.State.Text.Len(), Typed = S47RxLook::Typed(Len, T - S47RxLBannerAt, Now.bCalm);
	// the size that keeps the longest state inside its box
	const int32 Big = S47RxLWide(Len, 38) > StateWide - 24 ? 32 : 38;
	const float TextY = (Sz.Y - Big) * .5f - 1;
	Pen.Halo(Now.State.Text.Left(Typed), 18, TextY, Big, Tone, 1);
	if (Typed < Len)
	{
		// the letter just written is hot, and the beam stands after it
		if (Typed > 0) Pen.Text(Now.State.Text.Mid(Typed - 1, 1), 18 + S47RxLWide(Typed - 1, Big), TextY, Big, S47RxLFade(S47RxLHot, .7f), 1);
		Pen.Box(FVector2f(18 + S47RxLWide(Typed, Big) + 2, TextY + 5), FVector2f(Big * .34f, Big - 10.f), S47RxLFade(Tone, .85f), 1);
	}
	// ---- the array ----
	float X = StateWide + 22;
	Pen.Line({ FVector2f(X - 8, 9), FVector2f(X - 8, Sz.Y - 9) }, S47RxLLine, 1.2f);
	const FString Array = FString::Printf(TEXT("ARRAY %d/27"), Now.Dishes);
	Pen.Text(Array, X + 4, 2, 30, S47RxLGreen);
	Pen.Text(FString::Printf(TEXT("RATE %.0f %%  %s"), 100. * Now.Rate, *Now.RateWord), X + 4, 31, 15, S47RxLFade(Now.Rate > 0 ? S47RxLGreen : S47RxLAmber, .9f));
	X += FMath::Max(S47RxLWide(Array.Len(), 30), S47RxLWide(22, 15)) + 22;
	// ---- the card's frequency, the line it belongs to and its wavelength ----
	Pen.Line({ FVector2f(X - 8, 9), FVector2f(X - 8, Sz.Y - 9) }, S47RxLLine, 1.2f);
	const FString Mhz = Now.bJob ? FString::Printf(TEXT("%.3f MHz"), Now.CardMhz) : FString(TEXT("----.--- MHz"));
	if (Now.bJob) Pen.Halo(Mhz, X + 4, (Sz.Y - 38) * .5f - 1, 38, S47RxLAmber, 1);
	else Pen.Text(Mhz, X + 4, (Sz.Y - 38) * .5f - 1, 38, S47RxLLine);
	X += S47RxLWide(Mhz.Len(), 38) + 22;
	Pen.Line({ FVector2f(X - 8, 9), FVector2f(X - 8, Sz.Y - 9) }, S47RxLLine, 1.2f);
	if (Now.bJob && X + 150 < Sz.X)
	{
		const double Wave = S47RxLook::WavelengthCm(Now.CardMhz);
		Pen.Text(FString::Printf(TEXT("%s  %d cm"), S47RxLook::LineName(Now.CardMhz), FMath::RoundToInt32(Wave)), X + 4, 5, 18, S47RxLGreen);
		// (the font has no Greek: the lambda is two strokes of the beam)
		const FVector2f Lam(X + 5, 28);
		Pen.Line({ Lam + FVector2f(0, 1), Lam + FVector2f(2, 0), Lam + FVector2f(9, 14) }, S47RxLGreen, 1.5f);
		Pen.Line({ Lam + FVector2f(4.6f, 6), Lam + FVector2f(0, 14) }, S47RxLGreen, 1.5f);
		Pen.Text(FString::Printf(TEXT("= %.3f cm"), Wave), X + 20, 25, 18, S47RxLGreen);
		const float SrcX = X + 20 + S47RxLWide(11, 18) + 22;
		if (SrcX + S47RxLWide(Now.Source.Len(), 20) < Sz.X - 8)
		{
			Pen.Text(TEXT("SOURCE"), SrcX, 6, 15, S47RxLFade(S47RxLGreen, .7f));
			Pen.Text(Now.Source, SrcX, 23, 20, S47RxLGreen);
		}
	}
	return L + 2;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void SS47RxFall::Construct(const FArguments& Args)
{
	Live = Args._Feed;
	ForceVolatile(true);
}

int32 SS47RxFall::OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const
{
	const FVector2f Sz(G.GetLocalSize());
	const FS47RxLive& Now = S47RxLRead(Live);
	const FS47RxLPen Pen{ Out, G, L };
	const double T = FPlatformTime::Seconds();
	const FVector2f P0(34, 2), Plot(FMath::Max(20.f, Sz.X - 38), FMath::Max(12.f, Sz.Y - 4));
	Pen.Rect(P0 - FVector2f(1, 1), Plot + FVector2f(2, 2), S47RxLDim, 1.f);
	if (!Now.Scope.bPowered) { Pen.Text(TEXT("RECEIVER OFF"), P0.X + Plot.X * .5f - S47RxLWide(12, 18) * .5f, P0.Y + Plot.Y * .5f - 10, 18, S47RxLAmber, 1); return L + 2; }
	FS47RxLFall& Fall = S47RxLFallState();
	S47RxLFallStep(Fall, Now, T);
	const int32 Rows = FMath::Clamp(int32(Plot.Y / 4.f), 8, FMath::Min(S47RxLFallMost, Fall.Rows.Num()));
	const float Cw = Plot.X / S47RxLFallBins, Ch = Plot.Y / Rows;
	for (int32 R = 0; R < Rows; ++R)
	{
		// the newest row at the top; the row just written is a little hotter, as the phosphor is where the beam has just been
		const S47RxLook::FFallRow& Row = Fall.Rows[Fall.Rows.Num() - 1 - R];
		const float Fresh = Now.bCalm ? 1.f : R == 0 ? 1.3f : R == 1 ? 1.12f : 1.f;
		for (int32 B = 0; B < S47RxLFallBins; ++B)
		{
			const float V = FMath::Min(1.f, S47RxLook::FallCell(Row, Now.Scope.Seed, B, S47RxLFallBins) * Fresh);
			if (V < .11f) continue; // the darkest of the floor is left black
			Pen.Box(P0 + FVector2f(B * Cw, R * Ch), FVector2f(Cw + .4f, Ch + .4f), S47RxLFade(V > .8f ? S47RxLAmber : S47RxLGreen, .08f + .9f * V));
		}
	}
	// the dial's place (the middle of the window), and how far back the picture goes
	Pen.Line({ P0 + FVector2f(Plot.X * .5f, -1), P0 + FVector2f(Plot.X * .5f, 4) }, S47RxLAmber, 1.5f, 1);
	Pen.Line({ P0 + FVector2f(Plot.X * .5f, Plot.Y - 4), P0 + FVector2f(Plot.X * .5f, Plot.Y + 1) }, S47RxLAmber, 1.5f, 1);
	Pen.Text(TEXT("0 s"), 4, P0.Y - 2, 14, S47RxLFade(S47RxLGreen, .85f), 1);
	Pen.Text(FString::Printf(TEXT("-%.0f s"), Rows * S47RxLFallSeconds), 0, P0.Y + Plot.Y - 15, 14, S47RxLFade(S47RxLGreen, .85f), 1);
	return L + 2;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void SS47RxFft::Construct(const FArguments& Args)
{
	Live = Args._Feed;
	ForceVolatile(true);
}

int32 SS47RxFft::OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const
{
	const FVector2f Sz(G.GetLocalSize());
	const FS47RxLive& Now = S47RxLRead(Live);
	const FS47RxLPen Pen{ Out, G, L };
	const double T = FPlatformTime::Seconds();
	const FVector2f P0(34, 3), Plot(FMath::Max(20.f, Sz.X - 40), FMath::Max(12.f, Sz.Y - 20));
	// decibels up the side, megahertz along the foot
	for (int32 I = 0; I <= 3; ++I)
	{
		const float Y = P0.Y + Plot.Y * I / 3.f;
		Pen.Line({ FVector2f(P0.X, Y), FVector2f(P0.X + Plot.X, Y) }, S47RxLDim, 1.f);
		Pen.Text(FString::Printf(TEXT("%d"), -60 - 20 * I), 2, Y - (I == 3 ? 13.f : I == 0 ? 2.f : 7.f), 14, S47RxLFade(S47RxLGreen, .85f));
	}
	const double Half = S47RxLook::HalfSpan(Now.Scope.FTol);
	for (int32 I = 0; I <= 4; ++I)
	{
		const float X = P0.X + Plot.X * I / 4.f;
		Pen.Line({ FVector2f(X, P0.Y), FVector2f(X, P0.Y + Plot.Y) }, I == 2 ? S47RxLLine : S47RxLDim, 1.f);
		if (I % 2 == 0 && Now.bJob) Pen.Text(FString::Printf(TEXT("%.3f"), Now.Scope.Card + Half * (I - 2) / 2.), X - (I == 4 ? S47RxLWide(8, 14) : I == 0 ? 0.f : S47RxLWide(8, 14) * .5f), P0.Y + Plot.Y + 2, 14, S47RxLFade(S47RxLGreen, .85f));
	}
	Pen.Line({ P0, P0 + FVector2f(0, Plot.Y), P0 + Plot }, S47RxLLine, 1.5f);
	if (!Now.Scope.bPowered) { Pen.Text(TEXT("RECEIVER OFF"), P0.X + Plot.X * .5f - S47RxLWide(12, 18) * .5f, P0.Y + Plot.Y * .5f - 10, 18, S47RxLAmber, 1); return L + 2; }
	// the dial: its band and its mark, where it stands in the card's window
	const float Dial = S47RxLook::FftTuned(Now.Scope), BandHalf = float(Now.Scope.Bandwidth / 1000. / (2. * Half)) * .5f;
	const float B0 = P0.X + Plot.X * FMath::Clamp(Dial - BandHalf, 0.f, 1.f), B1 = P0.X + Plot.X * FMath::Clamp(Dial + BandHalf, 0.f, 1.f);
	Pen.Box(FVector2f(B0, P0.Y), FVector2f(B1 - B0, Plot.Y), FLinearColor(.1f, .5f, .2f, .12f));
	constexpr int32 Points = 120;
	TArray<FVector2f> Trace; Trace.Reserve(Points + 1);
	for (int32 I = 0; I <= Points; ++I)
	{
		const float U = I / float(Points);
		Trace.Add(FVector2f(P0.X + Plot.X * U, P0.Y + Plot.Y * (1.f - S47RxLook::FftLevel(Now.Scope, U, T))));
	}
	// amber for the locked thing: the peak standing on the card with the source heard
	Pen.Glow(Trace, Now.Scope.Strength >= .9 ? S47RxLAmber : S47RxLGreen, 1.5f, 1);
	if (Dial >= 0 && Dial <= 1) Pen.Line({ FVector2f(P0.X + Plot.X * Dial, P0.Y), FVector2f(P0.X + Plot.X * Dial, P0.Y + Plot.Y) }, S47RxLFade(S47RxLAmber, .9f), 1.6f, 1);
	else Pen.Text(Dial < 0 ? TEXT("< DIAL") : TEXT("DIAL >"), Dial < 0 ? P0.X + 4 : P0.X + Plot.X - S47RxLWide(6, 16) - 4, P0.Y + 2, 16, S47RxLAmber, 1);
	return L + 2;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void SS47RxIq::Construct(const FArguments& Args)
{
	Live = Args._Feed; bQuad = Args._bQuadrature;
	ForceVolatile(true);
}

int32 SS47RxIq::OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const
{
	const FVector2f Sz(G.GetLocalSize());
	const FS47RxLive& Now = S47RxLRead(Live);
	const FS47RxLPen Pen{ Out, G, L };
	const double T = FPlatformTime::Seconds();
	const float Mid = Sz.Y * .5f, Swing = FMath::Max(2.f, Sz.Y * .5f - 2.5f);
	Pen.Line({ FVector2f(0, Mid), FVector2f(Sz.X, Mid) }, S47RxLDim, 1.f);
	for (int32 I = 1; I < 8; ++I) Pen.Line({ FVector2f(Sz.X * I / 8.f, Mid - 3), FVector2f(Sz.X * I / 8.f, Mid + 3) }, S47RxLDim, 1.f);
	if (!Now.Scope.bPowered) return L + 1;
	constexpr int32 Points = 150;
	TArray<FVector2f> Trace; Trace.Reserve(Points + 1);
	for (int32 I = 0; I <= Points; ++I)
	{
		float InPhase = 0, Quad = 0;
		S47RxLook::Quadrature(Now.Scope, I / float(Points), T, Now.bCalm, InPhase, Quad);
		Trace.Add(FVector2f(Sz.X * I / float(Points), Mid - Swing * (bQuad ? Quad : InPhase)));
	}
	Pen.Glow(Trace, S47RxLGreen, 1.4f, 1);
	return L + 2;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void SS47RxTrack::Construct(const FArguments& Args)
{
	Live = Args._Feed; Sky = Args._Model;
	ForceVolatile(true);
}

float SS47RxTrack::EyeShare(double Degrees)
{
	// square root: the lock radius stands a third of the way out, the capture radius two thirds
	return float(FMath::Sqrt(FMath::Clamp(Degrees / EyeFull, 0., 1.)));
}

int32 SS47RxTrack::OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const
{
	using FSky = FS47SkyAcquire;
	const FVector2f Sz(G.GetLocalSize());
	const FS47RxLive& Now = S47RxLRead(Live);
	const FS47RxLPen Pen{ Out, G, L };
	const double T = FPlatformTime::Seconds();
	// ---- the bull's-eye, at the right: the reticle's offset from the crossing, azimuth across and elevation up ----
	const float EyeR = FMath::Clamp(FMath::Min(Sz.X * .19f, (Sz.Y - 36) * .5f), 16.f, 70.f);
	const FVector2f Eye(Sz.X - EyeR - 6, EyeR + 3);
	const bool bHave = Sky.IsValid() && Sky->HasBearings();
	const bool bLocked = Sky.IsValid() && Sky->Locked(), bLocking = Sky.IsValid() && Sky->State() == FSky::EPhase::Locking;
	Pen.Ring(Eye, EyeR, S47RxLLine, 1.5f);
	Pen.Ring(Eye, EyeR * EyeShare(FSky::CaptureRadius), S47RxLDim, 1.f);
	Pen.Ring(Eye, EyeR * EyeShare(FSky::LockRadius), bHave ? S47RxLFade(S47RxLAmber, .8f) : S47RxLDim, 1.2f);
	Pen.Line({ Eye - FVector2f(EyeR, 0), Eye + FVector2f(EyeR, 0) }, S47RxLDim, 1.f);
	Pen.Line({ Eye - FVector2f(0, EyeR), Eye + FVector2f(0, EyeR) }, S47RxLDim, 1.f);
	FString OffA = TEXT("dAZ   --"), OffE = TEXT("dEL   --"), OffD = TEXT("OFF   --");
	if (bHave)
	{
		const FVector2D Off = Sky->ReticleAt() - Sky->Cross();
		const double Far = Off.Size();
		const FVector2f Dot = Eye + (Far > 1e-9 ? FVector2f(float(Off.X / Far), float(-Off.Y / Far)) * (EyeR * EyeShare(Far)) : FVector2f::ZeroVector);
		const bool bOn = Far <= FSky::LockRadius;
		const FLinearColor Tone = bOn || bLocked || bLocking ? S47RxLAmber : S47RxLGreen;
		// on the crossing: a filled mark; off it: an open one; past the rim: pinned to it, with a stroke outward
		if (Far > EyeFull) Pen.Line({ Dot, Dot + (Dot - Eye).GetSafeNormal() * 6.f }, Tone, 2.f, 1);
		if (bOn) Pen.Box(Dot - FVector2f(3.5f, 3.5f), FVector2f(7, 7), Tone, 1);
		else Pen.Rect(Dot - FVector2f(3.5f, 3.5f), FVector2f(7, 7), Tone, 1.6f, 1);
		Pen.Glow({ Dot - FVector2f(7, 0), Dot + FVector2f(7, 0) }, S47RxLFade(Tone, .9f), 1.2f, 1);
		Pen.Glow({ Dot - FVector2f(0, 7), Dot + FVector2f(0, 7) }, S47RxLFade(Tone, .9f), 1.2f, 1);
		OffA = FString::Printf(TEXT("dAZ %+5.1f"), Off.X); OffE = FString::Printf(TEXT("dEL %+5.1f"), Off.Y); OffD = FString::Printf(TEXT("OFF %5.1f°"), Far);
	}
	const float EyeTextX = Sz.X - S47RxLWide(11, 15) - 4, EyeTextY = Eye.Y + EyeR + 2;
	if (EyeTextY + 30 <= Sz.Y)
	{
		Pen.Text(OffA + TEXT("  ") + OffE, FMath::Min(EyeTextX, Sz.X - S47RxLWide(22, 14) - 2), EyeTextY, 14, S47RxLFade(S47RxLGreen, .9f));
		Pen.Text(OffD, EyeTextX, EyeTextY + 14, 15, S47RxLGreen);
	}
	// ---- the readouts, as the sky rule gives them: they count up over the lock sequence and stand once the fix is held ----
	const int32 Size = Sz.Y >= 150 ? 17 : 15;
	const float Pitch = FMath::Min(float(Size) + 2, (Sz.Y - 2) / 8.f);
	float Y = 0;
	auto Row = [&](const FString& S, const FLinearColor& C) { Pen.Text(S, 2, Y, Size, C); Y += Pitch; };
	static const TCHAR* const ArmName[FSky::ArmCount] = { TEXT("A"), TEXT("B"), TEXT("C") };
	const FLinearColor Quiet = S47RxLFade(S47RxLGreen, .6f);
	if (Sky.IsValid() && (bLocking || bLocked))
	{
		const double Part = Sky->LockProgress();
		Row(FString::Printf(TEXT("S/N    %5.1f dB"), Sky->SignalToNoise() * Part), S47RxLGreen);
		Row(FString::Printf(TEXT("RESID  %5.2f°"), Sky->MeanOff()), S47RxLGreen);
		for (int32 Arm = 0; Arm < FSky::ArmCount; ++Arm) Row(FString::Printf(TEXT("DELAY %s %+7.1f ns"), ArmName[Arm], Sky->ArmDelayNs(Arm) * Part), S47RxLGreen);
		// the rate the longest delay changes at as the sky turns, as a fringe rate at the card's frequency
		double Rate = 0;
		for (int32 Arm = 0; Arm < FSky::ArmCount; ++Arm) { const double R = Sky->ArmDelayRate(Arm) * Now.CardMhz * 1e-3; if (FMath::Abs(R) > FMath::Abs(Rate)) Rate = R; }
		Row(FString::Printf(TEXT("RATE   %+6.3f Hz"), Rate * Part), S47RxLGreen);
		if (Part > .75)
		{
			const FSky::FSource* Listed = Sky->NearestCatalogued(Sky->Cross(), true);
			Row(FString(TEXT("CAT ")) + (Listed ? Listed->Name : FString(TEXT("--"))), S47RxLGreen);
			Row(FString::Printf(TEXT("OFFSET %4.1f°"), Sky->CatalogueOffset()), S47RxLGreen);
		}
		else { Row(TEXT("CAT ........"), Quiet); Row(TEXT("OFFSET ...."), Quiet); }
	}
	else
	{
		Row(TEXT("S/N       -- dB"), Quiet); Row(TEXT("RESID     --"), Quiet);
		for (int32 Arm = 0; Arm < FSky::ArmCount; ++Arm) Row(FString::Printf(TEXT("DELAY %s      -- ns"), ArmName[Arm]), Quiet);
		Row(TEXT("RATE      -- Hz"), Quiet); Row(TEXT("CAT       --"), Quiet); Row(TEXT("OFFSET    --"), Quiet);
	}
	// taking the fix: a small mark walks along under the readouts (still with Calm flashes)
	if (bLocking && !Now.bCalm) Pen.Box(FVector2f(2 + float(FMath::Fmod(T * 60., double(FMath::Max(40.f, Sz.X * .5f)))), Y + 1), FVector2f(10, 2), S47RxLAmber, 1);
	return L + 2;
}

// ---------------------------------------------------------------------------------------------------------------------------------

bool S47RxLookPaint::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bPass, const FString& What) { bOk &= bPass; Lines.Add(FString(bPass ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("receiver look: ") + What); };
	using FSky = FS47SkyAcquire;
	// the bull's-eye: the lock radius well inside, the capture radius outside it, the rim the far edge, never past it
	bool bGrows = true;
	for (double D = .25; D <= 30; D += .25) bGrows &= SS47RxTrack::EyeShare(D) >= SS47RxTrack::EyeShare(D - .25) && SS47RxTrack::EyeShare(D) <= 1.f;
	Check(bGrows && SS47RxTrack::EyeShare(0) == 0.f && SS47RxTrack::EyeShare(FSky::LockRadius) > .25f && SS47RxTrack::EyeShare(FSky::LockRadius) < SS47RxTrack::EyeShare(FSky::CaptureRadius) - .2f && SS47RxTrack::EyeShare(SS47RxTrack::EyeFull) == 1.f,
		TEXT("the bull's-eye: the lock ring a third of the way out, the capture ring two thirds, an offset past the rim pinned to it"));
	// the waterfall's clock: a full picture at first sight, a row every beat after, a tick only in the row written as the tape passes
	// it and only while recording, and nothing written twice
	FS47RxLFall Fall; FS47RxLive Live; Live.Scope.bPowered = true; Live.Scope.Strength = 1.; Live.Ticks = { 1.05, 9.3 };
	S47RxLFallStep(Fall, Live, 100.);
	const bool bFull = Fall.Rows.Num() == S47RxLFallMost && Fall.Next == uint32(S47RxLFallMost);
	int32 Marked = 0;
	for (int32 I = 1; I <= 40; ++I) { Live.Elapsed = I * .05; S47RxLFallStep(Fall, Live, 100. + I * .05); }
	for (const S47RxLook::FFallRow& Row : Fall.Rows) Marked += Row.bTick ? 1 : 0;
	const bool bQuiet = Marked == 0 && Fall.Rows.Num() == S47RxLFallMost && Fall.Next == uint32(S47RxLFallMost) + 9u;
	Live.bRecording = true; Live.Elapsed = 0;
	S47RxLFallStep(Fall, Live, 102.2);
	for (int32 I = 1; I <= 40; ++I) { Live.Elapsed = I * .05; S47RxLFallStep(Fall, Live, 102.2 + I * .05); }
	for (const S47RxLook::FFallRow& Row : Fall.Rows) Marked += Row.bTick ? 1 : 0;
	bool bOrder = true;
	for (int32 I = 1; I < Fall.Rows.Num(); ++I) bOrder &= Fall.Rows[I].Index == Fall.Rows[I - 1].Index + 1u;
	Check(bFull && bQuiet && Marked == 1 && bOrder, FString::Printf(TEXT("the waterfall writes a row every %.2f s, each once and in order; an extra tick is one marked row, and only while the take is recorded (%d)"), S47RxLFallSeconds, Marked));
	return bOk;
}
