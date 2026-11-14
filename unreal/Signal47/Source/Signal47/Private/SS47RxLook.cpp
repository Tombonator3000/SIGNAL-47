#include "SS47RxLook.h"
#include "S47RxLook.h"
#include "S47SkyAcquire.h"
#include "SS47SkyChart.h"
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
	FLinearColor S47RxLFade(FLinearColor C, float A) { C.A = FMath::Clamp(A, 0.f, 1.f); return C; }
	// How wide so many letters stand on the glass (S47RxLook::GlyphWide: a letter's advance is a whole number of the picture's pixels).
	float S47RxLWide(int32 Chars, int32 Size) { return S47RxLook::TextWide(Chars, Size); }
	// Round 2: nothing the frame draws is written smaller than the next step's line.
	constexpr int32 S47RxLSmall = S47RxLook::TextFloor;
	const FS47RxLive& S47RxLRead(const TSharedPtr<FS47RxFeed>& Feed) { static const FS47RxLive Empty; return Feed.IsValid() ? Feed->Now() : Empty; }
	// the station's name as the gate sign has it, three lines of the header
	const TCHAR* const S47RxLHeaderName[3] = { TEXT("SOUTHWEST ASTRONOMICAL"), TEXT("RESEARCH OBSERVATORY"), TEXT("NEW MEXICO") };

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
		// Round 2 (the first look in the engine: the headline words stood nearly flat): one more step of the glow for the three
		// headline words alone (SARO OPS, the state, the card's frequency): the same word wider and fainter round the halo.
		void Headline(const FString& S, float X, float Y, int32 Size, const FLinearColor& C, int32 Up = 0) const
		{
			const float Off = FMath::Max(1.f, Size / 26.f) * 2.3f, Skew = Off * .7f;
			const FLinearColor Far = S47RxLFade(C, .07f);
			for (const FVector2f D : { FVector2f(-Off, 0), FVector2f(Off, 0), FVector2f(0, -Off), FVector2f(0, Off), FVector2f(-Skew, -Skew), FVector2f(Skew, -Skew), FVector2f(-Skew, Skew), FVector2f(Skew, Skew) }) Text(S, X + D.X, Y + D.Y, Size, Far, Up);
			Halo(S, X, Y, Size, C, Up, .2f);
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

	// The banner's letters are written once when the state changes, not every time a page is built again (a tab, a key) and not
	// when only a number in it moves (the seconds recorded, the degrees to go): which state was last written and when is kept
	// here for the one receiver there is.
	int32 S47RxLBannerState = -1;
	double S47RxLBannerAt = -1e9;

	// The waterfall's rows, kept for the one receiver there is, so that a tab changed does not start the picture again. Round 2
	// (the first look in the engine: big square cells): a row is two units high (never more than S47RxLFallMost of them on a
	// tall panel) and is drawn as runs of grain along it, the line as a core with two fainter steps.
	constexpr int32 S47RxLFallMost = 72;
	constexpr float S47RxLFallRowHigh = 2.f;
	constexpr double S47RxLFallSeconds = .22;
	struct FS47RxLFall { TArray<S47RxLook::FFallRow> Rows; uint32 Next = 0; double At = -1e9, ElapsedWas = 0; };
	FS47RxLFall& S47RxLFallState() { static FS47RxLFall S; return S; }
	// How many rows a plot so high shows.
	int32 S47RxLFallRows(float PlotHigh) { return FMath::Clamp(int32(PlotHigh / S47RxLFallRowHigh), 8, S47RxLFallMost); }
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
	// What one row of the waterfall is drawn with: its runs of grain that are bright enough to show, the line's three steps and
	// a tick's dash. Draw (when given) is called for each box as a stretch of the window (0..1 from, 0..1 to), its level and its
	// kind (0 grain, 1 to 3 the line's far step to its core, 4 a tick). Returns how many boxes.
	int32 S47RxLFallBoxes(const S47RxLook::FFallRow& Row, uint32 Seed, float Fresh, TFunctionRef<void(float, float, float, int32)> Draw)
	{
		int32 Boxes = 0;
		S47RxLook::FFallRun Runs[S47RxLook::FallRunsMost];
		const int32 Count = S47RxLook::FallRuns(Row, Seed, Runs);
		float From = 0.f;
		for (int32 I = 0; I < Count; ++I)
		{
			const float Level = FMath::Min(1.f, Runs[I].Level * Fresh);
			// (the darkest of the floor is left black: the grain is light on dark, not a filled panel)
			if (Level >= .15f) { Draw(From, Runs[I].End, Level, 0); ++Boxes; }
			From = Runs[I].End;
		}
		for (int32 Step = S47RxLook::FallLineSteps - 1; Step >= 0; --Step)
		{
			float Half = 0, Level = 0;
			if (!S47RxLook::FallLine(Row, Seed, Step, Half, Level)) continue;
			const float Left = FMath::Max(0.f, Row.Line - Half), Right = FMath::Min(1.f, Row.Line + Half);
			if (Right > Left) { Draw(Left, Right, FMath::Min(1.f, Level * Fresh), S47RxLook::FallLineSteps - Step); ++Boxes; }
		}
		float TickHalf = 0, TickLevel = 0;
		if (S47RxLook::FallTick(Row, TickHalf, TickLevel))
		{
			const float Left = FMath::Max(0.f, Row.Line - TickHalf), Right = FMath::Min(1.f, Row.Line + TickHalf);
			if (Right > Left) { Draw(Left, Right, TickLevel, 4); ++Boxes; }
		}
		return Boxes;
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
	using S47RxLook::Num;
	const FVector2f Sz(G.GetLocalSize());
	const FS47RxLive& Now = S47RxLRead(Live);
	const FS47RxLPen Pen{ Out, G, L };
	// ---- the emblem: a dish on its mount, in strokes ----
	{
		const FVector2f Pivot(33, 33), Axis(-.64f, -.77f), Across(.77f, -.64f);
		TArray<FVector2f> Dish; Dish.Reserve(17);
		for (int32 I = -8; I <= 8; ++I) { const float U = I / 8.f; Dish.Add(Pivot + Across * (U * 23.f) + Axis * (U * U * 9.f)); }
		const FVector2f Focus = Pivot + Axis * 15.f;
		Pen.Line({ FVector2f(12, 57), FVector2f(54, 57) }, S47RxLGreen, 2.f);
		Pen.Glow({ FVector2f(22, 57), Pivot, FVector2f(44, 57) }, S47RxLGreen, 1.6f);
		Pen.Line({ FVector2f(27, 46), FVector2f(39, 46) }, S47RxLLine, 1.2f);
		Pen.Line({ Dish[0], Dish.Last() }, S47RxLLine, 1.f);
		Pen.Line({ Pivot, Dish[4] }, S47RxLLine, 1.f); Pen.Line({ Pivot, Dish[12] }, S47RxLLine, 1.f);
		Pen.Line({ Dish[0], Focus, Dish.Last() }, S47RxLFade(S47RxLGreen, .8f), 1.2f);
		Pen.Line({ Pivot, Focus }, S47RxLFade(S47RxLGreen, .8f), 1.2f);
		Pen.Glow(Dish, S47RxLGreen, 2.2f, 1);
		Pen.Box(Focus - FVector2f(2.5f, 2.5f), FVector2f(5, 5), S47RxLAmber, 1);
	}
	// ---- SARO OPS, the station's name as the gate sign has it, the terminal (round 2: three lines of the next step's size) ----
	Pen.Headline(TEXT("SARO OPS"), 72, 2, 54, S47RxLGreen, 1);
	const float BarA = 72 + S47RxLWide(8, 54) + 12, RuleFoot = Sz.Y - 12;
	Pen.Line({ FVector2f(BarA, 6), FVector2f(BarA, RuleFoot) }, S47RxLLine, 1.5f);
	constexpr float Pitch = 20.f;
	for (int32 I = 0; I < 3; ++I) Pen.Text(S47RxLHeaderName[I], BarA + 12, 1 + Pitch * I, S47RxLSmall, S47RxLFade(S47RxLGreen, .92f));
	const float BarB = BarA + 12 + S47RxLWide(22, S47RxLSmall) + 12;
	Pen.Line({ FVector2f(BarB, 6), FVector2f(BarB, RuleFoot) }, S47RxLLine, 1.5f);
	float TermEnd = BarB + 12;
	{
		// "V4.2  TERM 3  OPR NIGHT": a part a line
		TArray<FString> Parts;
		Now.Term.ParseIntoArray(Parts, TEXT("  "), true);
		for (int32 I = 0; I < Parts.Num() && I < 3; ++I)
		{
			const FString Part = Parts[I].TrimStartAndEnd();
			Pen.Text(Part, BarB + 12, 1 + Pitch * I, S47RxLSmall, S47RxLFade(S47RxLGreen, .8f));
			TermEnd = FMath::Max(TermEnd, BarB + 12 + S47RxLWide(Part.Len(), S47RxLSmall));
		}
	}
	// ---- the date and clock, and the array's own pointing. Round 2: the block stands in from the glass's rounded corner
	// (it was cut there), and its width is counted as the glass draws the letters ----
	constexpr float CornerClear = 26.f;
	const FString Stn = S47RxLookPaint::StationLine(Now);
	const float Edge = Sz.X - CornerClear, RightWide = FMath::Max(S47RxLWide(Now.Clock.Len(), 28), S47RxLWide(Stn.Len(), S47RxLSmall));
	Pen.Halo(Now.Clock, Edge - S47RxLWide(Now.Clock.Len(), 28), 4, 28, S47RxLGreen, 1, .12f);
	Pen.Text(Stn, Edge - S47RxLWide(Stn.Len(), S47RxLSmall), 34, S47RxLSmall, S47RxLFade(S47RxLGreen, .9f));
	const float BarC = Edge - RightWide - 14;
	if (BarC > TermEnd + 16) Pen.Line({ FVector2f(BarC, 6), FVector2f(BarC, RuleFoot) }, S47RxLLine, 1.5f);
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
	using S47RxLook::Num;
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
	if (int32(Now.State.Id) != S47RxLBannerState) { S47RxLBannerState = int32(Now.State.Id); S47RxLBannerAt = T; }
	const int32 Len = Now.State.Text.Len(), Typed = S47RxLook::Typed(Len, T - S47RxLBannerAt, Now.bCalm);
	// the size that keeps the longest state inside its box
	const int32 Big = S47RxLWide(Len, 38) > StateWide - 24 ? 32 : 38;
	const float TextY = (Sz.Y - Big) * .5f - 1;
	Pen.Headline(Now.State.Text.Left(Typed), 18, TextY, Big, Tone, 1);
	if (Typed < Len)
	{
		// the letter just written is hot, and the beam stands after it
		if (Typed > 0) Pen.Text(Now.State.Text.Mid(Typed - 1, 1), 18 + S47RxLWide(Typed - 1, Big), TextY, Big, S47RxLFade(S47RxLHot, .7f), 1);
		Pen.Box(FVector2f(18 + S47RxLWide(Typed, Big) + 2, TextY + 5), FVector2f(Big * .34f, Big - 10.f), S47RxLFade(Tone, .85f), 1);
	}
	// Round 2: two lines of the next step's size in each block (they were 15 and 18, and did not read)
	constexpr float RowA = 3.f, RowB = 25.f;
	// ---- the array ----
	float X = StateWide + 22;
	Pen.Line({ FVector2f(X - 8, 9), FVector2f(X - 8, Sz.Y - 9) }, S47RxLLine, 1.2f);
	const S47RxLookPaint::FBannerWords Words = S47RxLookPaint::BannerWords(Now);
	const FString& Array = Words.Array;
	const FString& Rate = Words.Rate;
	Pen.Text(Array, X + 4, RowA - 1, 24, S47RxLGreen);
	Pen.Text(Rate, X + 4, RowB, S47RxLSmall, S47RxLFade(Now.Rate > 0 ? S47RxLGreen : S47RxLAmber, .9f));
	X += FMath::Max(S47RxLWide(Array.Len(), 24), S47RxLWide(Rate.Len(), S47RxLSmall)) + 22;
	// ---- the card's frequency, the line it belongs to and its wavelength ----
	Pen.Line({ FVector2f(X - 8, 9), FVector2f(X - 8, Sz.Y - 9) }, S47RxLLine, 1.2f);
	const FString& Mhz = Words.Mhz;
	if (Now.bJob) Pen.Headline(Mhz, X + 4, (Sz.Y - 38) * .5f - 1, 38, S47RxLAmber, 1);
	else Pen.Text(Mhz, X + 4, (Sz.Y - 38) * .5f - 1, 38, S47RxLLine);
	X += S47RxLWide(Mhz.Len(), 38) + 22;
	Pen.Line({ FVector2f(X - 8, 9), FVector2f(X - 8, Sz.Y - 9) }, S47RxLLine, 1.2f);
	if (Now.bJob && X + 130 < Sz.X)
	{
		const FString& Band = Words.Band;
		const FString& Length = Words.Length;
		Pen.Text(Band, X + 4, RowA, S47RxLSmall, S47RxLGreen);
		// (the font has no Greek: the lambda is two strokes of the beam)
		const FVector2f Lam(X + 5, RowB + 4);
		Pen.Line({ Lam + FVector2f(0, 1), Lam + FVector2f(2, 0), Lam + FVector2f(10, 15) }, S47RxLGreen, 1.5f);
		Pen.Line({ Lam + FVector2f(5, 6.5f), Lam + FVector2f(0, 15) }, S47RxLGreen, 1.5f);
		Pen.Text(Length, X + 20, RowB, S47RxLSmall, S47RxLGreen);
		const float SrcX = X + FMath::Max(S47RxLWide(Band.Len(), S47RxLSmall) + 4, 20 + S47RxLWide(Length.Len(), S47RxLSmall)) + 22;
		if (SrcX + S47RxLWide(Now.Source.Len(), S47RxLSmall) < Sz.X - 8)
		{
			Pen.Line({ FVector2f(SrcX - 12, 9), FVector2f(SrcX - 12, Sz.Y - 9) }, S47RxLLine, 1.2f);
			Pen.Text(TEXT("SOURCE"), SrcX, RowA, S47RxLSmall, S47RxLFade(S47RxLGreen, .7f));
			Pen.Text(Now.Source, SrcX, RowB, S47RxLSmall, S47RxLGreen);
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
	using S47RxLook::Num;
	const FVector2f Sz(G.GetLocalSize());
	const FS47RxLive& Now = S47RxLRead(Live);
	const FS47RxLPen Pen{ Out, G, L };
	const double T = FPlatformTime::Seconds();
	// (the seconds stand down the left side, in letters of the next step's size)
	const FVector2f P0(S47RxLWide(5, S47RxLSmall) + 6, 2), Plot(FMath::Max(20.f, Sz.X - P0.X - 4), FMath::Max(12.f, Sz.Y - 4));
	Pen.Rect(P0 - FVector2f(1, 1), Plot + FVector2f(2, 2), S47RxLDim, 1.f);
	if (!Now.Scope.bPowered) { Pen.Text(TEXT("RECEIVER OFF"), P0.X + Plot.X * .5f - S47RxLWide(12, S47RxLSmall) * .5f, P0.Y + Plot.Y * .5f - 12, S47RxLSmall, S47RxLAmber, 1); return L + 2; }
	FS47RxLFall& Fall = S47RxLFallState();
	S47RxLFallStep(Fall, Now, T);
	const int32 Rows = FMath::Min(S47RxLFallRows(Plot.Y), Fall.Rows.Num());
	const float Ch = Plot.Y / FMath::Max(Rows, 1);
	const FLinearColor Core = FMath::Lerp(S47RxLAmber, S47RxLHot, .55f);
	for (int32 R = 0; R < Rows; ++R)
	{
		// the newest row at the top; the row just written is a little hotter, as the phosphor is where the beam has just been
		const float Fresh = Now.bCalm ? 1.f : R == 0 ? 1.3f : R == 1 ? 1.12f : 1.f;
		const float Y = P0.Y + R * Ch;
		S47RxLFallBoxes(Fall.Rows[Fall.Rows.Num() - 1 - R], Now.Scope.Seed, Fresh, [&](float From, float To, float Level, int32 Kind)
		{
			// grain in the phosphor's green; the line amber, soft at its edges and near white in its core; a tick's dash amber
			const FLinearColor Tint = Kind == 0 ? S47RxLFade(S47RxLGreen, .06f + .95f * Level) : Kind == S47RxLook::FallLineSteps ? S47RxLFade(Core, Level) : S47RxLFade(S47RxLAmber, Level);
			Pen.Box(FVector2f(P0.X + From * Plot.X, Y), FVector2f((To - From) * Plot.X + .4f, Ch + .3f), Tint, Kind == 0 ? 0 : 1);
		});
	}
	// the dial's place (the middle of the window), and how far back the picture goes
	Pen.Line({ P0 + FVector2f(Plot.X * .5f, -1), P0 + FVector2f(Plot.X * .5f, 4) }, S47RxLAmber, 1.5f, 1);
	Pen.Line({ P0 + FVector2f(Plot.X * .5f, Plot.Y - 4), P0 + FVector2f(Plot.X * .5f, Plot.Y + 1) }, S47RxLAmber, 1.5f, 1);
	const FLinearColor Quiet = S47RxLFade(S47RxLGreen, .85f);
	// (whole seconds, each beside the row that was written so long ago)
	const double Span = Rows * S47RxLFallSeconds;
	auto Seconds = [&](double Back)
	{
		const FString Label = S47RxLookPaint::FallSeconds(Back);
		const float LabelY = FMath::Clamp(P0.Y + Plot.Y * float(Back / FMath::Max(Span, 1e-6)) - 12.f, P0.Y - 5.f, P0.Y + Plot.Y - 21.f);
		Pen.Text(Label, P0.X - 5 - S47RxLWide(Label.Len(), S47RxLSmall), LabelY, S47RxLSmall, Quiet, 1);
	};
	Seconds(0.);
	if (Plot.Y >= 84) Seconds(FMath::FloorToDouble(Span * .5));
	if (Plot.Y >= 46) Seconds(FMath::FloorToDouble(Span));
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
	using S47RxLook::Num;
	const FVector2f Sz(G.GetLocalSize());
	const FS47RxLive& Now = S47RxLRead(Live);
	const FS47RxLPen Pen{ Out, G, L };
	const double T = FPlatformTime::Seconds();
	// decibels up the side, megahertz along the foot, in letters of the next step's size (round 2: three of each, not five)
	const FVector2f P0(S47RxLWide(4, S47RxLSmall) + 6, 3), Plot(FMath::Max(20.f, Sz.X - P0.X - 6), FMath::Max(12.f, Sz.Y - 3 - 24));
	const FLinearColor Quiet = S47RxLFade(S47RxLGreen, .85f);
	for (int32 I = 0; I <= 3; ++I)
	{
		const float Y = P0.Y + Plot.Y * I / 3.f;
		Pen.Line({ FVector2f(P0.X, Y), FVector2f(P0.X + Plot.X, Y) }, S47RxLDim, 1.f);
	}
	auto Decibels = [&](int32 Level, float Y)
	{
		const FString Label = S47RxLookPaint::FftDecibels(Level);
		Pen.Text(Label, P0.X - 4 - S47RxLWide(Label.Len(), S47RxLSmall), Y, S47RxLSmall, Quiet);
	};
	Decibels(-60, P0.Y - 5);
	if (Plot.Y >= 70) Decibels(-90, P0.Y + Plot.Y * .5f - 12);
	if (Plot.Y >= 40) Decibels(-120, P0.Y + Plot.Y - 20);
	const double Half = S47RxLook::HalfSpan(Now.Scope.FTol);
	for (int32 I = 0; I <= 4; ++I)
	{
		const float X = P0.X + Plot.X * I / 4.f;
		Pen.Line({ FVector2f(X, P0.Y), FVector2f(X, P0.Y + Plot.Y) }, I == 2 ? S47RxLLine : S47RxLDim, 1.f);
		if (I % 2 != 0 || !Now.bJob) continue;
		const FString Label = S47RxLookPaint::FftMhz(Now, I);
		const float Wide = S47RxLWide(Label.Len(), S47RxLSmall);
		// (the outer two only where the three do not stand on each other)
		if (I != 2 && Plot.X < Wide * 3 + 24) continue;
		Pen.Text(Label, X - (I == 4 ? Wide : I == 0 ? 0.f : Wide * .5f), P0.Y + Plot.Y + 1, S47RxLSmall, Quiet);
	}
	Pen.Line({ P0, P0 + FVector2f(0, Plot.Y), P0 + Plot }, S47RxLLine, 1.5f);
	if (!Now.Scope.bPowered) { Pen.Text(TEXT("RECEIVER OFF"), P0.X + Plot.X * .5f - S47RxLWide(12, S47RxLSmall) * .5f, P0.Y + Plot.Y * .5f - 12, S47RxLSmall, S47RxLAmber, 1); return L + 2; }
	// the dial: its band and its mark, where it stands in the card's window
	const float Dial = S47RxLook::FftTuned(Now.Scope), BandHalf = float(Now.Scope.Bandwidth / 1000. / (2. * Half)) * .5f;
	const float B0 = P0.X + Plot.X * FMath::Clamp(Dial - BandHalf, 0.f, 1.f), B1 = P0.X + Plot.X * FMath::Clamp(Dial + BandHalf, 0.f, 1.f);
	Pen.Box(FVector2f(B0, P0.Y), FVector2f(B1 - B0, Plot.Y), FLinearColor(.1f, .5f, .2f, .12f));
	constexpr int32 Points = 120;
	TArray<FVector2f> Trace; Trace.Reserve(Points + 1);
	for (int32 I = 0; I <= Points; ++I)
	{
		const float U = I / float(Points);
		Trace.Add(FVector2f(P0.X + Plot.X * U, P0.Y + Plot.Y * (1.f - S47RxLook::FftLevel(Now.Scope, U, T, Now.bCalm))));
	}
	// amber for the locked thing: the peak standing on the card with the source heard
	Pen.Glow(Trace, Now.Scope.Strength >= .9 ? S47RxLAmber : S47RxLGreen, 1.5f, 1);
	if (Dial >= 0 && Dial <= 1) Pen.Line({ FVector2f(P0.X + Plot.X * Dial, P0.Y), FVector2f(P0.X + Plot.X * Dial, P0.Y + Plot.Y) }, S47RxLFade(S47RxLAmber, .9f), 1.6f, 1);
	else Pen.Text(Dial < 0 ? TEXT("< DIAL") : TEXT("DIAL >"), Dial < 0 ? P0.X + 4 : P0.X + Plot.X - S47RxLWide(6, S47RxLSmall) - 4, P0.Y + 1, S47RxLSmall, S47RxLAmber, 1);
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
	// (round 2: the locked traces carry a little noise, S47RxLook::Quadrature; enough points for its fine grain to show)
	constexpr int32 Points = 180;
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
	Live = Args._Feed; Sky = Args._Model; ViewFn = Args._View;
	ForceVolatile(true);
}

float SS47RxTrack::EyeShare(double Degrees)
{
	// square root: the lock radius stands a third of the way out, the capture radius two thirds
	return float(FMath::Sqrt(FMath::Clamp(Degrees / EyeFull, 0., 1.)));
}

FVector2D SS47RxTrack::EyeAt(const S47SkySphere::FView& View, const FVector2D& Reticle, const FVector2D& Cross)
{
	// RX-LOOK gjennomgang 14 Nov: the way the reticle stands from the crossing on the chart's picture (the picture closes in about
	// the crossing alike in every direction, so the open picture's way is the glass's), y turned up; as far out as the rule says
	const double Far = FVector2D::Distance(Reticle, Cross);
	if (Far < 1e-9) return FVector2D::ZeroVector;
	FVector2D AtRet, AtCross, Way = FVector2D(Reticle.X - Cross.X, Reticle.Y - Cross.Y);
	if (S47SkySphere::ProjectAzEl(View, Reticle, AtRet) && S47SkySphere::ProjectAzEl(View, Cross, AtCross) && FVector2D::Distance(AtRet, AtCross) > 1e-9) Way = FVector2D(AtRet.X - AtCross.X, AtCross.Y - AtRet.Y);
	return Way.GetSafeNormal() * double(EyeShare(Far));
}

TArray<SS47RxTrack::FRow> SS47RxTrack::Rows(const FS47SkyAcquire* Sky, double CardMhz, bool bAllArms)
{
	using FSky = FS47SkyAcquire;
	using S47RxLook::Num;
	TArray<FRow> Out;
	auto Row = [&Out](const FString& Name, const FString& Value, bool bQuiet) { FRow R; R.Name = Name; R.Value = Value; R.bQuiet = bQuiet; Out.Add(R); };
	// (the arms by number, S47RxLook::ArmLabel; a lone capital C is not Freeform's)
	auto ArmName = [](int32 Arm) { return FString(TEXT("DELAY ")) + S47RxLook::ArmLabel(Arm); };
	const bool bLocked = Sky && Sky->Locked(), bLocking = Sky && Sky->State() == FSky::EPhase::Locking;
	if (Sky && (bLocking || bLocked))
	{
		const double Part = Sky->LockProgress();
		Row(TEXT("S/N"), Num(Sky->SignalToNoise() * Part, 1) + TEXT(" dB"), false);
		Row(TEXT("RESID"), Num(Sky->MeanOff(), 2) + TEXT("\u00b0"), false);
		// the rate the longest delay changes at as the sky turns, as a fringe rate at the card's frequency
		double Rate = 0; int32 Longest = 0;
		for (int32 Arm = 0; Arm < FSky::ArmCount; ++Arm)
		{
			const double ArmRate = Sky->ArmDelayRate(Arm) * CardMhz * 1e-3;
			if (FMath::Abs(ArmRate) > FMath::Abs(Rate)) Rate = ArmRate;
			if (FMath::Abs(Sky->ArmDelayNs(Arm)) > FMath::Abs(Sky->ArmDelayNs(Longest))) Longest = Arm;
		}
		for (int32 Arm = 0; Arm < FSky::ArmCount; ++Arm) if (bAllArms || Arm == Longest) Row(ArmName(Arm), Num(Sky->ArmDelayNs(Arm) * Part, 1, 0, true) + TEXT(" ns"), false);
		Row(TEXT("RATE"), Num(Rate * Part, 3, 0, true) + TEXT(" Hz"), false);
		if (Part > .75)
		{
			const FSky::FSource* Listed = Sky->NearestCatalogued(Sky->Cross(), true);
			Row(TEXT("CAT"), Listed ? Listed->Name : FString(TEXT("--")), false);
			Row(TEXT("OFFSET"), Num(Sky->CatalogueOffset(), 1) + TEXT("\u00b0"), false);
		}
		else { Row(TEXT("CAT"), TEXT("...."), true); Row(TEXT("OFFSET"), TEXT("...."), true); }
	}
	else
	{
		Row(TEXT("S/N"), TEXT("-- dB"), true); Row(TEXT("RESID"), TEXT("--"), true);
		for (int32 Arm = 0; Arm < FSky::ArmCount; ++Arm) if (bAllArms || Arm == 0) Row(ArmName(Arm), TEXT("-- ns"), true);
		Row(TEXT("RATE"), TEXT("-- Hz"), true); Row(TEXT("CAT"), TEXT("--"), true); Row(TEXT("OFFSET"), TEXT("--"), true);
	}
	return Out;
}

bool SS47RxTrack::AllArms(float High)
{
	// eight rows: S/N, RESID, three delays, RATE, CAT, OFFSET; at a letter's height apart they need this much
	return High >= RowHigh + 7.f * RowPitchLeast;
}

float SS47RxTrack::RowPitch(float High, int32 Count)
{
	if (Count <= 1) return RowPitchMost;
	return FMath::Clamp((High - RowHigh) / float(Count - 1), RowPitchLeast, RowPitchMost);
}

FString SS47RxTrack::OffWords(const FS47SkyAcquire* Sky)
{
	return Sky && Sky->HasBearings() ? FString::Printf(TEXT("OFF %s\u00b0"), *S47RxLook::Num(Sky->Distance(), 1)) : FString(TEXT("OFF --"));
}

int32 SS47RxTrack::OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const
{
	using FSky = FS47SkyAcquire;
	using S47RxLook::Num;
	const FVector2f Sz(G.GetLocalSize());
	const FS47RxLive& Now = S47RxLRead(Live);
	const FS47RxLPen Pen{ Out, G, L };
	const double T = FPlatformTime::Seconds();
	// ---- the bull's-eye, at the right: the reticle's offset from the crossing, the way it lies on the chart (EyeAt) ----
	const float EyeR = FMath::Clamp(FMath::Min(Sz.X * .19f, (Sz.Y - 30) * .5f), 16.f, 70.f);
	const FVector2f Eye(Sz.X - EyeR - 6, EyeR + 3);
	const bool bHave = Sky.IsValid() && Sky->HasBearings();
	const bool bLocked = Sky.IsValid() && Sky->Locked(), bLocking = Sky.IsValid() && Sky->State() == FSky::EPhase::Locking;
	Pen.Ring(Eye, EyeR, S47RxLLine, 1.5f);
	Pen.Ring(Eye, EyeR * EyeShare(FSky::CaptureRadius), S47RxLDim, 1.f);
	Pen.Ring(Eye, EyeR * EyeShare(FSky::LockRadius), bHave ? S47RxLFade(S47RxLAmber, .8f) : S47RxLDim, 1.2f);
	Pen.Line({ Eye - FVector2f(EyeR, 0), Eye + FVector2f(EyeR, 0) }, S47RxLDim, 1.f);
	Pen.Line({ Eye - FVector2f(0, EyeR), Eye + FVector2f(0, EyeR) }, S47RxLDim, 1.f);
	// (round 2: one line under it, how far off, in the next step's size; which way is the mark's own place)
	const FString OffD = OffWords(Sky.Get());
	if (bHave)
	{
		const double Far = Sky->Distance();
		const FVector2D Share = EyeAt(ViewFn ? ViewFn() : SS47SkyChart::PlotFor(FVector2f(716, 440)).View, Sky->ReticleAt(), Sky->Cross());
		const FVector2f Dot = Eye + FVector2f(float(Share.X), float(-Share.Y)) * EyeR;
		const bool bOn = Far <= FSky::LockRadius;
		const FLinearColor Tone = bOn || bLocked || bLocking ? S47RxLAmber : S47RxLGreen;
		// on the crossing: a filled mark; off it: an open one; past the rim: pinned to it, with a stroke outward
		if (Far > EyeFull) Pen.Line({ Dot, Dot + (Dot - Eye).GetSafeNormal() * 6.f }, Tone, 2.f, 1);
		if (bOn) Pen.Box(Dot - FVector2f(3.5f, 3.5f), FVector2f(7, 7), Tone, 1);
		else Pen.Rect(Dot - FVector2f(3.5f, 3.5f), FVector2f(7, 7), Tone, 1.6f, 1);
		Pen.Glow({ Dot - FVector2f(7, 0), Dot + FVector2f(7, 0) }, S47RxLFade(Tone, .9f), 1.2f, 1);
		Pen.Glow({ Dot - FVector2f(0, 7), Dot + FVector2f(0, 7) }, S47RxLFade(Tone, .9f), 1.2f, 1);
	}
	const float EyeTextY = Eye.Y + EyeR + 2;
	if (EyeTextY + 22 <= Sz.Y) Pen.Text(OffD, FMath::Min(Eye.X - S47RxLWide(OffD.Len(), S47RxLSmall) * .5f, Sz.X - S47RxLWide(OffD.Len(), S47RxLSmall) - 2), EyeTextY, S47RxLSmall, S47RxLGreen);
	// ---- the readouts, as the sky rule gives them: they count up over the lock sequence and stand once the fix is held. A
	// name and its number a line, in the next step's size; on a low panel only the longest delay of the three ----
	// (RX-LOOK gjennomgang 14 Nov: the pitch leaves the last row whole, RowPitch; it was min(22, (h - 2) / 8), which cut OFFSET)
	const bool bAllArms = AllArms(Sz.Y);
	const TArray<FRow> Lines = Rows(Sky.Get(), Now.CardMhz, bAllArms);
	const float Pitch = RowPitch(Sz.Y, Lines.Num());
	float Y = 0;
	const FLinearColor Quiet = S47RxLFade(S47RxLGreen, .6f);
	// (RX-LOOK gjennomgang 14 Nov: the rows as Rows writes them for the rule's state, the same the tests read)
	for (const FRow& Each : Lines) { Pen.Text(Each.Line(), 2, Y, S47RxLSmall, Each.bQuiet ? Quiet : S47RxLGreen); Y += Pitch; }
	// taking the fix: a small mark walks along under the readouts (still with Calm flashes)
	if (bLocking && !Now.bCalm) Pen.Box(FVector2f(2 + float(FMath::Fmod(T * 60., double(FMath::Max(40.f, Sz.X * .5f)))), FMath::Min(Y + 1, Sz.Y - 2)), FVector2f(10, 2), S47RxLAmber, 1);
	return L + 2;
}

// ---------------------------------------------------------------------------------------------------------------------------------

FString S47RxLookPaint::StationLine(const FS47RxLive& Now)
{
	using S47RxLook::Num;
	return Now.bPointing ? FString::Printf(TEXT("STN SARO  EL %s\u00b0  AZ %s\u00b0"), *Num(Now.El, 1), *Num(Now.Az, 1, 5, false, true)) : FString(TEXT("STN SARO  EL --.-  AZ ---.-"));
}

S47RxLookPaint::FBannerWords S47RxLookPaint::BannerWords(const FS47RxLive& Now)
{
	using S47RxLook::Num;
	FBannerWords W;
	W.State = Now.State.Text;
	W.Array = FString::Printf(TEXT("ARRAY %s/27"), *Num(Now.Dishes));
	W.Rate = FString::Printf(TEXT("RATE %s %% %s"), *Num(100. * Now.Rate), *Now.RateWord);
	W.Mhz = Now.bJob ? Num(Now.CardMhz, 3) + TEXT(" MHz") : FString(TEXT("----.--- MHz"));
	const double Wave = S47RxLook::WavelengthCm(Now.CardMhz);
	W.Band = FString::Printf(TEXT("%s  %s cm"), S47RxLook::LineName(Now.CardMhz), *Num(FMath::RoundToDouble(Wave)));
	W.Length = FString::Printf(TEXT("= %s cm"), *Num(Wave, 3));
	W.Source = Now.Source;
	return W;
}

FString S47RxLookPaint::FallSeconds(double Back) { return (Back > 0 ? TEXT("-") : TEXT("")) + S47RxLook::Num(Back) + TEXT(" s"); }
FString S47RxLookPaint::FftDecibels(int32 Level) { return S47RxLook::Num(double(Level)); }
FString S47RxLookPaint::FftMhz(const FS47RxLive& Now, int32 Tick) { return S47RxLook::Num(Now.Scope.Card + S47RxLook::HalfSpan(Now.Scope.FTol) * (Tick - 2) / 2., 3); }

TArray<FString> S47RxLookPaint::FrameWords(const FS47RxLive& Now, const FS47SkyAcquire* Sky)
{
	TArray<FString> Out;
	// the header
	for (const TCHAR* Line : S47RxLHeaderName) Out.Add(Line);
	TArray<FString> Parts;
	Now.Term.ParseIntoArray(Parts, TEXT("  "), true);
	for (const FString& Part : Parts) Out.Add(Part.TrimStartAndEnd());
	Out.Add(Now.Clock); Out.Add(StationLine(Now)); Out.Add(TEXT("SARO OPS"));
	// the banner
	const FBannerWords B = BannerWords(Now);
	Out.Append({ B.State, B.Array, B.Rate, B.Mhz, B.Band, B.Length, FString(TEXT("SOURCE")), B.Source });
	// the waterfall's seconds (for every height of panel) and the spectrum's foot and side
	for (int32 Rows = 8; Rows <= S47RxLFallMost; ++Rows) { const double Span = Rows * S47RxLFallSeconds; Out.Add(FallSeconds(0.)); Out.Add(FallSeconds(FMath::FloorToDouble(Span * .5))); Out.Add(FallSeconds(FMath::FloorToDouble(Span))); }
	for (const int32 Level : { -60, -90, -120 }) Out.Add(FftDecibels(Level));
	for (const int32 Tick : { 0, 2, 4 }) Out.Add(FftMhz(Now, Tick));
	Out.Append({ FString(TEXT("RECEIVER OFF")), FString(TEXT("< DIAL")), FString(TEXT("DIAL >")) });
	// the tracking panel
	Out.Add(SS47RxTrack::OffWords(Sky));
	for (const bool bAll : { true, false }) for (const SS47RxTrack::FRow& Row : SS47RxTrack::Rows(Sky, Now.CardMhz, bAll)) Out.Add(Row.Line());
	return Out;
}

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
	{
		// RX-LOOK gjennomgang 14 Nov: the tracking panel's rows stand whole in every panel from the lowest the page gives it to the
		// tallest: the last row (OFFSET) above the foot, the rows a letter's height apart at least, every arm where there is room.
		// The old pitch, min(22, (h - 2) / 8), is measured beside it.
		bool bWhole = true; int32 Cut = 0;
		for (float High = 120.f; High <= 400.f; High += .5f)
		{
			const bool bAll = SS47RxTrack::AllArms(High);
			const int32 Count = bAll ? 8 : 6;
			const float Pitch = SS47RxTrack::RowPitch(High, Count);
			bWhole &= (Count - 1) * Pitch + SS47RxTrack::RowHigh <= High + 1e-3f && Pitch >= SS47RxTrack::RowPitchLeast && Pitch <= SS47RxTrack::RowPitchMost;
			const bool bOldAll = High >= 160.f;
			const float OldPitch = FMath::Min(22.f, (High - 2) / (bOldAll ? 8.f : 6.f));
			Cut += (bOldAll ? 7 : 5) * OldPitch + SS47RxTrack::RowHigh > High ? 1 : 0;
		}
		Check(bWhole && Cut > 0 && SS47RxTrack::Rows(nullptr, 1419.6, true).Num() == 8 && SS47RxTrack::Rows(nullptr, 1419.6, false).Num() == 6 && SS47RxTrack::AllArms(181.f),
			FString::Printf(TEXT("the tracking panel's rows stand whole in every panel 120 to 400 units high (the old pitch cut the last row in %d of them); at the page's own height every arm has its row"), Cut));
	}
	{
		// RX-LOOK gjennomgang 14 Nov: the bull's-eye lies the way the chart does. At each card source, a reticle a step off the
		// crossing the way a key points on the glass (S47SkySphere::SteerOnGlass) stands that way on the bull's-eye, and UP held
		// from below brings the mark straight up. The old way (azimuth across, elevation up) is measured beside it: it was off by
		// the parallactic angle.
		const S47SkySphere::FView View = SS47SkyChart::PlotFor(FVector2f(716, 440)).View;
		double Worst = 0, OldWorst = 0, Sideways = 0;
		bool bUp = true;
		int32 Sources = 0;
		auto AngleOff = [](const FVector2D& A, const FVector2D& B) { return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector2D::DotProduct(A.GetSafeNormal(), B.GetSafeNormal()), -1., 1.))); };
		for (const FSky::FSource& Src : FSky::Catalogue())
		{
			if (!Src.bMajor) continue;
			++Sources;
			for (const FVector2D Way : { FVector2D(0, 1), FVector2D(0, -1), FVector2D(-1, 0), FVector2D(1, 0), FVector2D(.6, .8) })
			{
				const FVector2D Ret = Src.At + S47SkySphere::SteerOnGlass(View, Src.At, Way) * 3.;
				Worst = FMath::Max(Worst, AngleOff(SS47RxTrack::EyeAt(View, Ret, Src.At), Way));
				OldWorst = FMath::Max(OldWorst, AngleOff(Ret - Src.At, Way));
			}
			// UP held from below the crossing on the glass, in steps the keys take
			FVector2D Below;
			if (!S47SkySphere::ProjectAzEl(View, Src.At, Below)) { bUp = false; continue; }
			FVector2D Ret = S47SkySphere::UnprojectAzEl(View, Below + FVector2D(0, 20));
			double WasY = SS47RxTrack::EyeAt(View, Ret, Src.At).Y;
			for (int32 Step = 0; Step < 16; ++Step)
			{
				Ret += S47SkySphere::SteerOnGlass(View, Ret, FVector2D(0, 1)) * .25;
				const FVector2D Mark = SS47RxTrack::EyeAt(View, Ret, Src.At);
				bUp &= Mark.Y > WasY;
				if (Mark.Size() > .15) Sideways = FMath::Max(Sideways, FMath::Abs(Mark.X) / Mark.Size());
				WasY = Mark.Y;
			}
		}
		Check(Sources >= 3 && Worst < 4. && bUp && Sideways < .1 && OldWorst > 20. && SS47RxTrack::EyeAt(View, FVector2D(100, 40), FVector2D(100, 40)).IsZero() && FMath::IsNearlyEqual(SS47RxTrack::EyeAt(View, FVector2D(100, 44), FVector2D(100, 40)).Size(), double(SS47RxTrack::EyeShare(4.)), 1e-6),
			FString::Printf(TEXT("the bull's-eye lies the way the chart does: a key's step at %d card sources stands within %.1f degrees of the key's way (azimuth across and elevation up was %.0f off), and UP held brings the mark straight up (%.2f of it sideways at most)"), Sources, Worst, OldWorst, Sideways));
	}
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
	// round 2: the waterfall's picture. Thin rows of grain with a soft line, inside the frame's budget of boxes: what a frame
	// draws for the small panel beside the sphere and for the tall one on TUNE, against the 40 square cells a row it was
	{
		int32 Small = 0, Tall = 0, Tick = 0, Lines3 = 0;
		bool bInside = true;
		const int32 SmallRows = S47RxLFallRows(100.f), TallRows = S47RxLFallRows(260.f);
		for (int32 R = 0; R < TallRows; ++R)
		{
			int32 Kinds[5] = { 0, 0, 0, 0, 0 };
			float CoreFrom = 0, CoreTo = 0, FarFrom = 0, FarTo = 0;
			const int32 Boxes = S47RxLFallBoxes(Fall.Rows[Fall.Rows.Num() - 1 - R], Live.Scope.Seed, 1.f, [&](float From, float To, float Level, int32 Kind)
			{
				bInside &= From >= 0 && To <= 1 && To > From && Level > 0 && Level <= 1 && Kind >= 0 && Kind <= 4;
				if (Kind >= 0 && Kind <= 4) ++Kinds[Kind];
				if (Kind == S47RxLook::FallLineSteps) { CoreFrom = From; CoreTo = To; }
				if (Kind == 1) { FarFrom = From; FarTo = To; }
			});
			Tall += Boxes; if (R < SmallRows) Small += Boxes;
			Tick += Kinds[4];
			// the line: a narrow core inside two wider steps
			if (Kinds[1] == 1 && Kinds[2] == 1 && Kinds[3] == 1 && CoreTo - CoreFrom < .02f && FarTo - FarFrom > .08f && CoreFrom > FarFrom && CoreTo < FarTo) ++Lines3;
		}
		const int32 Before = FMath::Clamp(int32(100.f / 4.f), 8, 44) * 40, BeforeTall = 44 * 40;
		Check(bInside && SmallRows == 50 && TallRows == S47RxLFallMost && Lines3 == TallRows && Tick == 1 && Small <= 1200 && Tall <= 1700 && Small >= SmallRows * 8,
			FString::Printf(TEXT("the waterfall's picture: %d rows of grain two units high beside the sphere (%d boxes a frame; it was %d square cells in 25 rows), %d rows on TUNE (%d boxes; it was %d), the line a narrow core in two wider steps in every row"), SmallRows, Small, Before, TallRows, Tall, BeforeTall));
	}
	{
		// RX-LOOK gjennomgang 14 Nov: every line the painted parts write, read through the same functions they draw with, for the
		// states the frame goes through: the header's clock through every minute of a day, the array's pointing through every
		// tenth of a degree, the banner's state while recording 0 to 18 s, the reference 0 to 8 s and slewing through 0 to 180
		// degrees, the dishes and the rate, the card's frequency across the dial, and the tracking panel while the sky rule
		// searches, takes the fix and holds it. None reads 47, has a lone C or another of the text rule's words.
		int32 Read = 0;
		FString Bad;
		auto Hold = [&](const FString& Shown) { ++Read; if (Bad.IsEmpty() && !S47RxLook::TextFault(Shown).IsEmpty()) Bad = Shown + TEXT(" (") + S47RxLook::TextFault(Shown) + TEXT(")"); };
		FS47RxLive Base; Base.bJob = true; Base.Term = TEXT("V4.2  TERM 3  OPR NIGHT"); Base.Source = TEXT("PSR B1133+16"); Base.CardMhz = Base.Scope.Card = 1419.6; Base.Scope.FTol = .03;
		Base.Scope.bPowered = true; Base.Dishes = 27; Base.Rate = 1.; Base.RateWord = TEXT("FULL"); Base.bPointing = true; Base.El = 54.3; Base.Az = 112.;
		Base.Clock = S47RxLook::ClockLine(TEXT("28-APR-86 21:14"));
		for (const FString& Each : FrameWords(Base, nullptr)) Hold(Each);
		for (int32 Minute = 0; Minute < 24 * 60; ++Minute) Hold(S47RxLook::ClockLine(FString::Printf(TEXT("28-APR-86 %02d:%02d"), Minute / 60, Minute % 60)));
		FS47RxLive Turning = Base;
		for (int32 Tenth = 0; Tenth <= 3600; ++Tenth) { Turning.Az = Tenth * .1; Turning.El = FMath::Fmod(Tenth * .1, 90.); Hold(StationLine(Turning)); }
		S47RxLook::FStateIn In; In.bJob = true; In.Stage = 2; In.bTuned = In.bPointReady = In.bSettled = true; In.Seconds = 18;
		for (int32 Step = 0; Step <= 18 * 20; ++Step) { In.Elapsed = Step * .05; Hold(S47RxLook::StateOf(In).Text); }
		In.Stage = 4; In.bReference = true;
		for (int32 Step = 0; Step <= 8 * 20; ++Step) { In.ReferenceAt = Step * .05; Hold(S47RxLook::StateOf(In).Text); }
		for (int32 Step = 0; Step <= 3600; ++Step) Hold(S47RxLook::SlewWords(Step * .05));
		FS47RxLive Counting = Base;
		for (int32 Dishes = 0; Dishes <= 27; ++Dishes) { Counting.Dishes = Dishes; Hold(BannerWords(Counting).Array); }
		for (int32 Share = 0; Share <= 1000; ++Share) { Counting.Rate = Share * .001; Hold(BannerWords(Counting).Rate); }
		for (int32 Step = 0; Step <= 1200; ++Step)
		{
			Counting.CardMhz = Counting.Scope.Card = 1419.5 + Step * .001;
			const FBannerWords Words = BannerWords(Counting);
			Hold(Words.Mhz); Hold(Words.Band); Hold(Words.Length);
			for (const int32 Tick : { 0, 2, 4 }) Hold(FftMhz(Counting, Tick));
		}
		// the tracking panel while the rule searches, takes the fix and holds it, for each card source and the unusual kind
		for (const FS47SkyAcquire::FSource& Src : FS47SkyAcquire::Catalogue())
		{
			if (!Src.bMajor) continue;
			for (const bool bOdd : { false, true })
			{
				FS47SkyAcquire Sky;
				Sky.Begin(7u + uint32(Src.Run) * 31u, Src.At, Src.Run, bOdd);
				TArray<FS47SkyAcquire::FDish> Dishes;
				for (int32 I = 0; I < FS47SkyAcquire::DishCount; ++I)
				{
					FS47SkyAcquire::FDish D; D.bOn = true; D.Point = Src.At;
					const double Arm = FMath::DegreesToRadians(120. * FS47SkyAcquire::ArmOf(I)), Reach = 40. * (1 + I % (FS47SkyAcquire::DishCount / FS47SkyAcquire::ArmCount));
					D.Ground = FVector2D(FMath::Sin(Arm) * Reach, FMath::Cos(Arm) * Reach);
					Dishes.Add(D);
				}
				for (const FString& Each : FrameWords(Base, &Sky)) Hold(Each);
				for (int32 Frame = 0; Frame < 60 * 45 && !Sky.Locked(); ++Frame)
				{
					Sky.SetArray(Dishes, true, true);
					Sky.Steer(Sky.Distance() > 1. ? (Sky.Cross() - Sky.ReticleAt()).GetSafeNormal() : FVector2D::ZeroVector);
					Sky.Step(1. / 60.);
					if (Frame % 6 == 0 || Sky.State() == FS47SkyAcquire::EPhase::Locking)
					{
						Hold(SS47RxTrack::OffWords(&Sky));
						for (const bool bAll : { true, false }) for (const SS47RxTrack::FRow& Row : SS47RxTrack::Rows(&Sky, Base.CardMhz, bAll)) Hold(Row.Line());
					}
				}
				for (const FString& Each : FrameWords(Base, &Sky)) Hold(Each);
			}
		}
		Check(Bad.IsEmpty() && Read > 20000, FString::Printf(TEXT("%d lines the frame's painted parts write, read through the functions they draw with, keep to the text rule%s"), Read, Bad.IsEmpty() ? TEXT("") : *(TEXT(": NOT ") + Bad)));
	}
	return bOk;
}
