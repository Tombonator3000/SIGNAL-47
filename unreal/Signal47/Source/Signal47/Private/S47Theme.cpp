#include "S47Theme.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Texture2D.h"
#include "Fonts/CompositeFont.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "S47Image.h"
#include "S47Kit.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

// ---------- colours and fonts ----------

FLinearColor S47Theme::Css(uint32 Rgb, float A)
{
	FLinearColor C = FLinearColor::FromSRGBColor(FColor((Rgb >> 16) & 255, (Rgb >> 8) & 255, Rgb & 255));
	C.A = A;
	return C;
}

namespace S47Theme::Local
{
	// the fonts in Content/Fonts, read from disk (staged with the build), one composite font per file
	FSlateFontInfo LoadFont(const TCHAR* File, float Px, float Spacing)
	{
		static TMap<FString, TSharedPtr<FStandaloneCompositeFont>> Cache;
		FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Fonts") / File);
		if (!IFileManager::Get().FileExists(*Path)) Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Fonts/VT323-Regular.ttf"));
		TSharedPtr<FStandaloneCompositeFont>& F = Cache.FindOrAdd(Path);
		if (!F.IsValid())
		{
			F = MakeShared<FStandaloneCompositeFont>();
			F->DefaultTypeface.AppendFont(TEXT("Regular"), Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		}
		// (Slate's size is in points at 96 dpi: a CSS pixel is .75 of one)
		FSlateFontInfo Info(F, Px * U * 0.75f, TEXT("Regular"));
		// (Slate's tracking is in thousandths of the size in points, not of the em in pixels)
		Info.LetterSpacing = Px > 0.f ? FMath::RoundToInt32(Spacing / (Px * 0.75f) * 1000.f) : 0;
		return Info;
	}

	FColor Vertex(const FLinearColor& C) { return C.ToFColor(true); }
}

FSlateFontInfo S47Theme::Term(float Px, float Spacing) { return Local::LoadFont(TEXT("VT323-Regular.ttf"), Px, Spacing); }
FSlateFontInfo S47Theme::Heading(float Px, float Spacing) { return Local::LoadFont(TEXT("Oswald-Medium.ttf"), Px, Spacing); }
FSlateFontInfo S47Theme::Type(float Px, float Spacing) { return Local::LoadFont(TEXT("SpecialElite-Regular.ttf"), Px, Spacing); }
FSlateFontInfo S47Theme::Hand(float Px, float Spacing) { return Local::LoadFont(TEXT("ReenieBeanie-Regular.ttf"), Px, Spacing); }

// ---------- brushes and painting ----------

const FSlateBrush* S47Theme::White()
{
	static const FSlateColorBrush B(FLinearColor::White);
	return &B;
}

const FSlateBrush* S47Theme::Box(const FLinearColor& Fill, const FLinearColor& Line, float LineW, float Radius)
{
	static TMap<FString, TSharedPtr<FSlateBrush>> Cache;
	const FString Key = Fill.ToString() + Line.ToString() + FString::SanitizeFloat(LineW) + TEXT("/") + FString::SanitizeFloat(Radius);
	TSharedPtr<FSlateBrush>& B = Cache.FindOrAdd(Key);
	if (!B.IsValid()) B = MakeShared<FSlateRoundedBoxBrush>(Fill, Radius, Line, LineW);
	return B.Get();
}

const ISlateStyle& S47Theme::Styles()
{
	static TSharedPtr<FSlateStyleSet> Set;
	if (!Set.IsValid())
	{
		Set = MakeShared<FSlateStyleSet>(TEXT("S47Theme"));
		const FTextBlockStyle Note = FTextBlockStyle().SetFont(Type(15)).SetColorAndOpacity(Mute()).SetShadowOffset(FVector2D::ZeroVector);
		const FTextBlockStyle Em = FTextBlockStyle(Note).SetColorAndOpacity(AmberDim());
		Set->Set(TEXT("Note"), Note);
		Set->Set(TEXT("Em"), Em);
		FButtonStyle None;
		const FSlateNoResource Nothing;
		None.SetNormal(Nothing).SetHovered(Nothing).SetPressed(Nothing).SetDisabled(Nothing);
		Set->Set(TEXT("Link"), FHyperlinkStyle().SetTextStyle(FTextBlockStyle(Em).SetColorAndOpacity(Css(0xc98a3e))).SetUnderlineStyle(None).SetPadding(FMargin(0)));
	}
	return *Set;
}

void S47Theme::Fill(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& Pos, const FVector2f& Size, const FLinearColor& Color)
{
	FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), White(), ESlateDrawEffect::None, Color);
}

void S47Theme::Gradient(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& Pos, const FVector2f& Size,
	const TArray<TPair<float, FLinearColor>>& Stops, bool bAcross)
{
	// (Orient_Vertical: stops placed along x, the colour changes left to right)
	TArray<FSlateGradientStop> S;
	for (const TPair<float, FLinearColor>& P : Stops) S.Add(FSlateGradientStop(FVector2f(P.Key * Size.X, P.Key * Size.Y), P.Value));
	FSlateDrawElement::MakeGradient(Out, Layer, G.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), S, bAcross ? Orient_Vertical : Orient_Horizontal);
}

void S47Theme::Triangle(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& A, const FVector2f& B, const FVector2f& C, const FLinearColor& Color)
{
	if (!FSlateApplication::IsInitialized()) return;
	const FSlateRenderTransform& T = G.GetAccumulatedRenderTransform();
	const FColor Col = Local::Vertex(Color);
	TArray<FSlateVertex> Verts;
	for (const FVector2f& P : { A, B, C }) Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(T, P, FVector2f::ZeroVector, Col));
	TArray<SlateIndex> Idx = { 0, 1, 2 };
	const FSlateResourceHandle Handle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")));
	FSlateDrawElement::MakeCustomVerts(Out, Layer, Handle, Verts, Idx, nullptr, 0, 0);
}

void S47Theme::Vignette(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, float CenterX, float Edge)
{
	// radial-gradient(ellipse at CenterX 50%, transparent 45%, rgba(0,0,0,Edge) 100%): the ellipse has
	// the closest side's proportions and reaches the farthest corner (CSS farthest-corner)
	static TMap<FString, TSharedPtr<FSlateBrush>> Cache;
	const FString Key = FString::Printf(TEXT("%.3f/%.3f"), CenterX, Edge);
	TSharedPtr<FSlateBrush>& B = Cache.FindOrAdd(Key);
	if (!B.IsValid())
	{
		const int32 N = 128;
		const float Near = FMath::Max(0.05f, FMath::Min(CenterX, 1.f - CenterX)), Far = FMath::Max(CenterX, 1.f - CenterX);
		const float K = FMath::Sqrt(FMath::Square(Far / Near) + 1.f);
		TArray<FColor> Px;
		Px.SetNumUninitialized(N * N);
		for (int32 y = 0; y < N; ++y)
		{
			for (int32 x = 0; x < N; ++x)
			{
				const float Dx = ((x + 0.5f) / N - CenterX) / (K * Near), Dy = ((y + 0.5f) / N - 0.5f) / (K * 0.5f);
				const float D = FMath::Sqrt(Dx * Dx + Dy * Dy);
				const float A = Edge * FMath::Clamp((D - 0.45f) / 0.55f, 0.f, 1.f);
				Px[y * N + x] = FColor(0, 0, 0, uint8(FMath::RoundToInt32(A * 255.f)));
			}
		}
		if (UTexture2D* T = S47Image::FromPixels(Px, N, N)) B = S47Kit::BrushFor(T);
	}
	if (B.IsValid()) FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(), B.Get(), ESlateDrawEffect::None, FLinearColor::White);
}

void S47Theme::Shadow(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& Pos, const FVector2f& Size, float OffsetY, float Spread, float Alpha)
{
	const int32 N = 6;
	for (int32 i = N; i >= 1; --i)
	{
		const float S = Spread * i / N;
		FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(Size + FVector2f(2 * S, 2 * S), FSlateLayoutTransform(Pos + FVector2f(-S, OffsetY - S))),
			Box(FLinearColor(0, 0, 0, Alpha / N), FLinearColor::Transparent, 0.f, S), ESlateDrawEffect::None, FLinearColor(0, 0, 0, Alpha / N));
	}
}

// ---------- the night's words ----------

TPair<FString, FString> S47Theme::ChapterLine(const FString& ChapterTitle)
{
	static const TCHAR* Words[] = { TEXT("ZERO"), TEXT("ONE"), TEXT("TWO"), TEXT("THREE"), TEXT("FOUR"), TEXT("FIVE"), TEXT("SIX"), TEXT("SEVEN"), TEXT("EIGHT"), TEXT("NINE"), TEXT("TEN") };
	FString HeadPart, Title;
	if (!ChapterTitle.Split(TEXT(": "), &HeadPart, &Title)) return { ChapterTitle.ToUpper(), FString() };
	if (HeadPart.StartsWith(TEXT("Chapter ")))
	{
		const int32 N = FCString::Atoi(*HeadPart.Mid(8));
		if (N >= 0 && N <= 10) HeadPart = FString(TEXT("Chapter ")) + Words[N];
	}
	return { HeadPart.ToUpper(), Title.ToUpper() };
}

FString S47Theme::DateStamp(double Clock)
{
	// the night of 13 to 14 April 1986: the evening is the 13th, after midnight the 14th
	const double C = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
	return C >= 12 * 3600 ? TEXT("APR.13 1986") : TEXT("APR.14 1986");
}

FString S47Theme::Stamp(double Clock)
{
	const int32 S = FMath::FloorToInt32(FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0));
	return FString::Printf(TEXT("%s  %02d:%02d:%02d"), *DateStamp(Clock), S / 3600, (S / 60) % 60, S % 60);
}

FString S47Theme::SavedWhen(double Ms)
{
	static const TCHAR* Mon[] = { TEXT("Jan"), TEXT("Feb"), TEXT("Mar"), TEXT("Apr"), TEXT("May"), TEXT("Jun"), TEXT("Jul"), TEXT("Aug"), TEXT("Sep"), TEXT("Oct"), TEXT("Nov"), TEXT("Dec") };
	const FTimespan Zone = FDateTime::Now() - FDateTime::UtcNow();
	const FDateTime L = FDateTime(1970, 1, 1) + FTimespan::FromMilliseconds(Ms) + Zone;
	const FDateTime Today = FDateTime::Now().GetDate();
	const FString Hm = FString::Printf(TEXT("%02d:%02d"), L.GetHour(), L.GetMinute());
	if (L.GetDate() == Today) return TEXT("today ") + Hm;
	if (L.GetDate() == Today - FTimespan::FromDays(1)) return TEXT("yesterday ") + Hm;
	return FString::Printf(TEXT("%s %d, %s"), Mon[L.GetMonth() - 1], L.GetDay(), *Hm);
}

FString S47Theme::SavedAgo(double Ms)
{
	const double Now = (FDateTime::UtcNow() - FDateTime(1970, 1, 1)).GetTotalMilliseconds();
	const double S = FMath::Max(0.0, (Now - Ms) / 1000.0);
	if (S < 60) return TEXT("SAVED JUST NOW");
	if (S < 3600) return FString::Printf(TEXT("SAVED %d MIN AGO"), FMath::FloorToInt32(S / 60));
	if (S < 86400) return FString::Printf(TEXT("SAVED %d H AGO"), FMath::FloorToInt32(S / 3600));
	return TEXT("SAVED ") + SavedWhen(Ms).ToUpper();
}

FString S47Theme::Played(double Sec)
{
	// SaveMenu.ts played(), in the case cards' capitals
	const int32 M = FMath::FloorToInt32(Sec / 60);
	if (M < 1) return TEXT("UNDER A MINUTE");
	if (M < 60) return FString::Printf(TEXT("%d MIN"), M);
	return FString::Printf(TEXT("%d H %d MIN"), M / 60, M % 60);
}

bool S47Theme::SelfTest(TArray<FString>& Out)
{
	bool bOk = true;
	auto Check = [&](bool bPass, const TCHAR* What, const FString& Got = FString()) { Out.Add(FString::Printf(TEXT("%s theme: %s %s"), bPass ? TEXT("ok  ") : TEXT("FAIL"), What, *Got)); bOk &= bPass; };
	const TPair<FString, FString> L = ChapterLine(TEXT("Chapter 2: The Amended Record"));
	Check(L.Key == TEXT("CHAPTER TWO") && L.Value == TEXT("THE AMENDED RECORD"), TEXT("chapter line"), L.Key + TEXT(" / ") + L.Value);
	Check(ChapterLine(TEXT("Prologue: Night Shift")).Key == TEXT("PROLOGUE"), TEXT("the prologue's line"));
	Check(Stamp(23 * 3600 + 41 * 60 + 7) == TEXT("APR.13 1986  23:41:07"), TEXT("before midnight"), Stamp(23 * 3600 + 41 * 60 + 7));
	Check(Stamp(86400 + 3 * 3600 + 12 * 60 + 44) == TEXT("APR.14 1986  03:12:44") && Stamp(3 * 3600) == TEXT("APR.14 1986  03:00:00"), TEXT("after midnight"));
	Check(Played(4320) == TEXT("1 H 12 MIN") && Played(30) == TEXT("UNDER A MINUTE"), TEXT("played"));
	const double Now = (FDateTime::UtcNow() - FDateTime(1970, 1, 1)).GetTotalMilliseconds();
	Check(SavedAgo(Now - 125000) == TEXT("SAVED 2 MIN AGO") && SavedWhen(Now).StartsWith(TEXT("today ")), TEXT("when saved"), SavedAgo(Now - 125000));
	return bOk;
}

// ---------- the pieces ----------

namespace S47Theme::Local
{
	// a click anywhere on its content (the arrows, ON / OFF, a tab, a button, a key hint); hovering is IsHovered()
	class SS47Click : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SS47Click) {}
			SLATE_ARGUMENT(TFunction<void()>, OnClick)
			SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs)
		{
			OnClick = InArgs._OnClick;
			ChildSlot[InArgs._Content.Widget];
		}
		void SetContent(const TSharedRef<SWidget>& W) { ChildSlot[W]; }
		virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent& E) override
		{
			if (E.GetEffectingButton() != EKeys::LeftMouseButton || !OnClick) return FReply::Unhandled();
			OnClick();
			return FReply::Handled();
		}
		virtual FCursorReply OnCursorQuery(const FGeometry&, const FPointerEvent&) const override { return OnClick ? FCursorReply::Cursor(EMouseCursor::Hand) : FCursorReply::Unhandled(); }

	private:
		TFunction<void()> OnClick;
	};

	// the 20 segments (.seg): a click or a drag sets the value
	class SS47Segs : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SS47Segs) {}
			SLATE_ATTRIBUTE(float, Frac)
			SLATE_ATTRIBUTE(bool, Usable)
			SLATE_ARGUMENT(TFunction<void(float)>, OnSet)
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs) { Frac = InArgs._Frac; Usable = InArgs._Usable; OnSet = InArgs._OnSet; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D((20 * 13 + 19 * 3) * U, 20 * U); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
		{
			const int32 On = FMath::RoundToInt32(FMath::Clamp(Frac.Get(0.f), 0.f, 1.f) * 20.f);
			const float A = Usable.Get(true) ? 1.f : 0.35f;
			for (int32 i = 0; i < 20; ++i) Fill(Out, Layer, G, FVector2f(i * 16 * U, 0), FVector2f(13 * U, 20 * U), i < On ? Amber(A) : Paper(0.12f * A));
			return Layer;
		}
		virtual FReply OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override
		{
			if (E.GetEffectingButton() != EKeys::LeftMouseButton || !Usable.Get(true)) return FReply::Unhandled();
			Set(G, E);
			return FReply::Handled().CaptureMouse(SharedThis(this));
		}
		virtual FReply OnMouseMove(const FGeometry& G, const FPointerEvent& E) override
		{
			if (!HasMouseCapture()) return FReply::Unhandled();
			Set(G, E);
			return FReply::Handled();
		}
		virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent&) override { return HasMouseCapture() ? FReply::Handled().ReleaseMouseCapture() : FReply::Unhandled(); }

	private:
		TAttribute<float> Frac;
		TAttribute<bool> Usable;
		TFunction<void(float)> OnSet;
		void Set(const FGeometry& G, const FPointerEvent& E)
		{
			// a segment's right edge is its value: a click on the first one is 1/20
			const float X = G.AbsoluteToLocal(E.GetScreenSpacePosition()).X / FMath::Max(1.f, float(G.GetLocalSize().X));
			if (OnSet) OnSet(FMath::Clamp(FMath::CeilToFloat(X * 20.f) / 20.f, 0.f, 1.f));
		}
	};

	// the tape's lines (.scan): 1 px of dark in every 3 screen pixels
	class SS47Scan : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SS47Scan) : _Alpha(0.18f) {}
			SLATE_ARGUMENT(float, Alpha)
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs) { Alpha = InArgs._Alpha; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
		{
			const float Scale = FMath::Max(0.1f, G.Scale);
			const FVector2f S = FVector2f(G.GetLocalSize());
			const float Step = 3.f / Scale, Line = 1.f / Scale;
			for (float Y = 0; Y < S.Y; Y += Step) Fill(Out, Layer, G, FVector2f(0, Y), FVector2f(S.X, Line), FLinearColor(0, 0, 0, Alpha));
			return Layer;
		}

	private:
		float Alpha = 0.18f;
	};

	class SS47Vig : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SS47Vig) : _CenterX(0.6f) {}
			SLATE_ARGUMENT(float, CenterX)
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs) { CenterX = InArgs._CenterX; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
		{
			Vignette(Out, Layer, G, CenterX);
			return Layer;
		}

	private:
		float CenterX = 0.6f;
	};

	// .frame: border 1px rgba(255,176,82,.28), the dark gradient, box-shadow 0 0 0 1px rgba(0,0,0,.6), 0 30px 80px rgba(0,0,0,.6)
	class SS47FrameBox : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SS47FrameBox) {}
			SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs) { ChildSlot[InArgs._Content.Widget]; }
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override
		{
			const FVector2f S = FVector2f(G.GetLocalSize());
			Shadow(Out, Layer, G, FVector2f::ZeroVector, S, 30 * U, 40 * U, 0.6f);
			// (a rounded box's fill is the tint MakeBox is given; its line is the brush's own)
			FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(S + FVector2f(2, 2), FSlateLayoutTransform(FVector2f(-1, -1))), Box(FLinearColor::Transparent, FLinearColor(0, 0, 0, 0.6f), 1.f), ESlateDrawEffect::None, FLinearColor::Transparent);
			Gradient(Out, Layer + 1, G, FVector2f::ZeroVector, S, { { 0.f, Css(0x080b13, 0.92f) }, { 1.f, Css(0x05070d, 0.96f) } }, false);
			FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(), Box(FLinearColor::Transparent, Amber(0.28f), 1.f), ESlateDrawEffect::None, FLinearColor::Transparent);
			return SCompoundWidget::OnPaint(Args, G, Cull, Out, Layer + 3, Style, bParentEnabled);
		}
	};
}

// ---------- SS47Glyph ----------

void SS47Glyph::Construct(const FArguments& InArgs)
{
	S47_USING_THEME
	Kind = InArgs._Kind;
	Size = InArgs._Size;
	Color = InArgs._Color;
}

int32 SS47Glyph::OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool) const
{
	S47_USING_THEME
	const FVector2f S = FVector2f(G.GetLocalSize());
	const float W = S.X, H = S.Y;
	FLinearColor C = Color.Get(FLinearColor::White);
	C.A *= Style.GetColorAndOpacityTint().A;
	auto Tri = [&](FVector2f A, FVector2f B, FVector2f D) { Triangle(Out, Layer, G, A, B, D, C); };
	switch (Kind)
	{
	case EKind::Play:
	case EKind::TriRight: Tri({ 0, 0 }, { W, H / 2 }, { 0, H }); break;
	case EKind::TriLeft: Tri({ W, 0 }, { 0, H / 2 }, { W, H }); break;
	case EKind::Pause:
	{
		const float Bar = W * 0.3f;
		Fill(Out, Layer, G, { 0, 0 }, { Bar, H }, C);
		Fill(Out, Layer, G, { W - Bar, 0 }, { Bar, H }, C);
		break;
	}
	case EKind::Up: Tri({ 0, H * 0.45f }, { W / 2, 0 }, { W, H * 0.45f }); Fill(Out, Layer, G, { W * 0.4f, H * 0.4f }, { W * 0.2f, H * 0.6f }, C); break;
	case EKind::Down: Tri({ 0, H * 0.55f }, { W / 2, H }, { W, H * 0.55f }); Fill(Out, Layer, G, { W * 0.4f, 0 }, { W * 0.2f, H * 0.6f }, C); break;
	case EKind::Left: Tri({ W * 0.45f, 0 }, { 0, H / 2 }, { W * 0.45f, H }); Fill(Out, Layer, G, { W * 0.4f, H * 0.4f }, { W * 0.6f, H * 0.2f }, C); break;
	case EKind::Right: Tri({ W * 0.55f, 0 }, { W, H / 2 }, { W * 0.55f, H }); Fill(Out, Layer, G, { 0, H * 0.4f }, { W * 0.6f, H * 0.2f }, C); break;
	}
	return Layer;
}

// ---------- SS47Osd ----------

void SS47Osd::Construct(const FArguments& InArgs)
{
	S47_USING_THEME
	const float P = InArgs._Px;
	const TAttribute<FString> Text = InArgs._Text;
	const bool bPlay = InArgs._bPlay, bPause = InArgs._bPause;
	// .osd: text-shadow 2px 0 0 rgba(255,60,60,.55), -2px 0 0 rgba(60,200,255,.45) (and a soft glow Slate cannot blur)
	auto Line = [&](const FLinearColor& C, float Dx)
	{
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text_Lambda([Text] { return FText::FromString(Text.Get()); }).Font(Term(P, 2)).ColorAndOpacity(C)
			];
		if (bPlay || bPause)
		{
			// (the pause sign's bars stand in from the edge of the box as the font's glyph does)
			Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(P * (bPause ? 0.75f : 0.55f) * U, 0, bPause ? P * 0.3f * U : 0.f, 0)
			[
				SNew(SS47Glyph).Kind(bPause ? SS47Glyph::EKind::Pause : SS47Glyph::EKind::Play).Size(bPause ? FVector2D(P * 1.24f, P * 0.94f) : FVector2D(P * 0.7f, P * 0.72f)).Color(C)
			];
		}
		return SNew(SBox).RenderTransform(FSlateRenderTransform(FVector2f(Dx * U, 0)))[Row];
	};
	ChildSlot
	[
		SNew(SOverlay).Visibility(EVisibility::HitTestInvisible)
		+ SOverlay::Slot()[Line(FLinearColor(1.f, 0.045f, 0.045f, 0.55f), 2.f)]
		+ SOverlay::Slot()[Line(FLinearColor(0.045f, 0.58f, 1.f, 0.45f), -2.f)]
		+ SOverlay::Slot()[Line(Osd(), 0.f)]
	];
}

// ---------- keycaps and key hints ----------

TSharedRef<SWidget> S47Theme::Keycap(const FString& Tokens, float P, TAttribute<FLinearColor> Text, TAttribute<FLinearColor> Frame)
{
	if (!Text.IsSet() || (!Text.IsBound() && Text.Get() == FLinearColor::Transparent)) Text = Paper();
	if (!Frame.IsSet() || (!Frame.IsBound() && Frame.Get() == FLinearColor::Transparent)) Frame = Rule();
	TArray<FString> Parts;
	Tokens.ParseIntoArray(Parts, TEXT(" "));
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (int32 i = 0; i < Parts.Num(); ++i)
	{
		const FString& T = Parts[i];
		const bool bArrow = T == TEXT("UP") || T == TEXT("DOWN") || T == TEXT("LEFT") || T == TEXT("RIGHT");
		const FMargin Gap(i ? P * 0.5f * U : 0.f, 0, 0, 0);
		if (bArrow)
		{
			const SS47Glyph::EKind K = T == TEXT("UP") ? SS47Glyph::EKind::Up : T == TEXT("DOWN") ? SS47Glyph::EKind::Down : T == TEXT("LEFT") ? SS47Glyph::EKind::Left : SS47Glyph::EKind::Right;
			const bool bTall = K == SS47Glyph::EKind::Up || K == SS47Glyph::EKind::Down;
			Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(Gap)[SNew(SS47Glyph).Kind(K).Size(bTall ? FVector2D(P * 0.42f, P * 0.62f) : FVector2D(P * 0.62f, P * 0.42f)).Color(Text)];
		}
		else
		{
			Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(Gap)[SNew(STextBlock).Text(FText::FromString(T)).Font(Term(P, 1)).ColorAndOpacity_Lambda([Text] { return FSlateColor(Text.Get()); })];
		}
	}
	return SNew(SBorder).Padding(FMargin(6 * U, 0)).VAlign(VAlign_Center)
		.BorderImage_Lambda([Frame] { return Box(FLinearColor::Transparent, Frame.Get(), 1.f, 2.f * U); })
		[
			SNew(SBox).MinDesiredHeight(P * 1.05f * U).VAlign(VAlign_Center)[Row]
		];
}

TSharedRef<SWidget> S47Theme::KeyHints(const TArray<FHint>& Hints, float P)
{
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (int32 i = 0; i < Hints.Num(); ++i)
	{
		const FHint& H = Hints[i];
		const TSharedRef<Local::SS47Click> Item = SNew(Local::SS47Click).OnClick(H.OnClick);
		Item->SetContent(
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6 * U, 0)[Keycap(H.Keys, P)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(H.Label)).Font(Term(P, 1))
				.ColorAndOpacity_Lambda([W = TWeakPtr<Local::SS47Click>(Item), bClick = bool(H.OnClick)] { const TSharedPtr<Local::SS47Click> C = W.Pin(); return FSlateColor(bClick && C.IsValid() && C->IsHovered() ? Paper() : Mute()); })
			]);
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, i + 1 < Hints.Num() ? 28 * U : 0.f, 0)[Item];
	}
	return Row;
}

// ---------- SS47MenuItem ----------

void SS47MenuItem::Construct(const FArguments& InArgs)
{
	S47_USING_THEME
	Selected = InArgs._Selected;
	Usable = InArgs._Usable;
	OnHover = InArgs._OnHover;
	OnClick = InArgs._OnClick;
	bArrow = InArgs._bArrow;
	const FLinearColor Rest = InArgs._Color.Get(Paper(0.82f));
	const TAttribute<FString> Meta = InArgs._Meta;
	const FMargin P = InArgs._Pad;
	auto LabelColor = [this, Rest] { return FSlateColor(!Usable.Get(true) ? Paper(0.32f) : Selected.Get(false) ? Amber() : Rest); };
	auto MetaColor = [this] { return FSlateColor(!Usable.Get(true) ? Paper(0.22f) : Selected.Get(false) ? Css(0xd9b27f) : PaperDim()); };
	ChildSlot.Padding(FMargin((3 + P.Left) * U, P.Top * U, P.Right * U, P.Bottom * U))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SAssignNew(LabelText, STextBlock).Text(FText::FromString(InArgs._Label)).Font(Term(InArgs._Px, InArgs._Spacing)).ColorAndOpacity_Lambda(LabelColor)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, -4 * U, 0, 0)
		[
			SNew(STextBlock).Text_Lambda([Meta] { return FText::FromString(Meta.Get(FString())); }).Font(Term(InArgs._MetaPx, 1)).ColorAndOpacity_Lambda(MetaColor)
			.Visibility_Lambda([Meta] { return Meta.Get(FString()).IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
		]
	];
}

int32 SS47MenuItem::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	S47_USING_THEME
	// .it.sel: background linear-gradient(90deg, rgba(255,176,82,.18), transparent); border-left 3px amber; ::before '▶'
	if (Selected.Get(false) && Usable.Get(true))
	{
		const FVector2f S = FVector2f(G.GetLocalSize());
		Gradient(Out, Layer, G, FVector2f::ZeroVector, S, { { 0.f, Amber(0.18f) }, { 1.f, Amber(0.f) } }, true);
		Fill(Out, Layer, G, FVector2f::ZeroVector, FVector2f(3 * U, S.Y), Amber());
		if (bArrow && LabelText.IsValid())
		{
			const FGeometry& LG = LabelText->GetCachedGeometry();
			const float Cy = G.AbsoluteToLocal(LG.GetAbsolutePositionAtCoordinates(FVector2f(0.5f, 0.5f))).Y;
			const float W = 22 * U, H = 24 * U, X = -27 * U;
			Triangle(Out, Layer, G, FVector2f(X, Cy - H / 2), FVector2f(X + W, Cy), FVector2f(X, Cy + H / 2), Amber());
		}
	}
	return SCompoundWidget::OnPaint(Args, G, Cull, Out, Layer + 1, Style, bParentEnabled);
}

void SS47MenuItem::OnMouseEnter(const FGeometry& G, const FPointerEvent& E)
{
	SCompoundWidget::OnMouseEnter(G, E);
	if (Usable.Get(true) && OnHover) OnHover();
}

FReply SS47MenuItem::OnMouseButtonDown(const FGeometry&, const FPointerEvent& E)
{
	if (E.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
	bPressed = true;
	return FReply::Handled();
}

FReply SS47MenuItem::OnMouseButtonUp(const FGeometry&, const FPointerEvent& E)
{
	if (E.GetEffectingButton() != EKeys::LeftMouseButton || !bPressed) return FReply::Unhandled();
	bPressed = false;
	if (Usable.Get(true) && OnClick) OnClick();
	return FReply::Handled();
}

// ---------- SS47Row ----------

void SS47Row::Construct(const FArguments& InArgs)
{
	S47_USING_THEME
	Selected = InArgs._Selected;
	Usable = InArgs._Usable;
	OnHover = InArgs._OnHover;
	OnClick = InArgs._OnClick;
	ChildSlot
	[
		SNew(SBox).HeightOverride(InArgs._Height * U)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding((3 + 18) * U, 0, 12 * U, 0)
			[
				SNew(STextBlock).Text(FText::FromString(InArgs._Label)).Font(Term(30, 1)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
				.ColorAndOpacity_Lambda([this] { return FSlateColor(!Usable.Get(true) ? Paper(0.35f) : Selected.Get(false) ? Css(0xf6ecd8) : Paper(0.86f)); })
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 18 * U, 0)
			[
				InArgs._Value.Widget
			]
		]
	];
}

int32 SS47Row::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	S47_USING_THEME
	// .row.sel: linear-gradient(90deg, rgba(255,176,82,.16), rgba(255,176,82,.03)); border-left 3px amber
	if (Selected.Get(false))
	{
		const FVector2f S = FVector2f(G.GetLocalSize());
		Gradient(Out, Layer, G, FVector2f::ZeroVector, S, { { 0.f, Amber(0.16f) }, { 1.f, Amber(0.03f) } }, true);
		Fill(Out, Layer, G, FVector2f::ZeroVector, FVector2f(3 * U, S.Y), Amber());
	}
	return SCompoundWidget::OnPaint(Args, G, Cull, Out, Layer + 1, Style, bParentEnabled);
}

void SS47Row::OnMouseEnter(const FGeometry& G, const FPointerEvent& E)
{
	SCompoundWidget::OnMouseEnter(G, E);
	if (OnHover) OnHover();
}

FReply SS47Row::OnMouseButtonDown(const FGeometry&, const FPointerEvent& E)
{
	if (E.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
	if (OnClick && Usable.Get(true)) OnClick();
	return FReply::Handled();
}

// ---------- the controls ----------

TSharedRef<SWidget> S47Theme::Choice(TAttribute<FString> Value, TAttribute<bool> Selected, TFunction<void(int32)> OnStep, TAttribute<bool> Usable)
{
	// .v: ◀ VALUE ▶, the arrows grey at rest and amber on the selected row
	auto ArrowColor = [Selected, Usable] { return !Usable.Get(true) ? Arrow(0.35f) : Selected.Get(false) ? Amber() : Arrow(); };
	auto Side = [&](int32 Dir)
	{
		return SNew(Local::SS47Click).OnClick([OnStep, Usable, Dir] { if (OnStep && Usable.Get(true)) OnStep(Dir); })
			[
				SNew(SBox).Padding(FMargin(4 * U, 6 * U)).VAlign(VAlign_Center)
				[
					SNew(SS47Glyph).Kind(Dir < 0 ? SS47Glyph::EKind::TriLeft : SS47Glyph::EKind::TriRight).Size(FVector2D(22, 21)).Color_Lambda(ArrowColor)
				]
			];
	};
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8 * U, 0)[Side(-1)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text_Lambda([Value] { return FText::FromString(Value.Get(FString())); }).Font(Term(30, 1))
			.ColorAndOpacity_Lambda([Usable] { return FSlateColor(Amber(Usable.Get(true) ? 1.f : 0.4f)); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8 * U, 0, 0, 0)[Side(1)];
}

TSharedRef<SWidget> S47Theme::Segments(TAttribute<float> Frac, TAttribute<FString> Value, TFunction<void(float)> OnSet, TAttribute<bool> Usable)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(Local::SS47Segs).Frac(Frac).Usable(Usable).OnSet(OnSet)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12 * U, 0, 0, 0)
		[
			SNew(SBox).WidthOverride(64 * U).HAlign(HAlign_Right)
			[
				SNew(STextBlock).Text_Lambda([Value] { return FText::FromString(Value.Get(FString())); }).Font(Term(30, 1)).ColorAndOpacity(Amber())
			]
		];
}

TSharedRef<SWidget> S47Theme::Toggle(TAttribute<bool> On, TFunction<void(bool)> OnSet, TAttribute<bool> Usable)
{
	// .tog: ON | OFF, the one in force filled amber
	auto Cell = [&](bool bWhich, const TCHAR* Label)
	{
		return SNew(Local::SS47Click).OnClick([OnSet, Usable, bWhich] { if (OnSet && Usable.Get(true)) OnSet(bWhich); })
			[
				SNew(SBorder).Padding(FMargin(12 * U, 0)).VAlign(VAlign_Center)
				.BorderImage_Lambda([On, bWhich] { const bool bLit = On.Get(false) == bWhich; return Box(bLit ? Amber() : FLinearColor::Transparent, bLit ? Amber() : Css(0x4d483f), 1.f); })
				[
					SNew(STextBlock).Text(FText::FromString(Label)).Font(Term(24, 1))
					.ColorAndOpacity_Lambda([On, bWhich] { return FSlateColor(On.Get(false) == bWhich ? Css(0x1b1408) : Arrow()); })
				]
			];
	};
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()[Cell(true, TEXT("ON"))]
		+ SHorizontalBox::Slot().AutoWidth()[Cell(false, TEXT("OFF"))];
}

TSharedRef<SWidget> S47Theme::Tabs(const TArray<FString>& Names, TAttribute<int32> Current, TFunction<void(int32)> OnPick)
{
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	const int32 N = Names.Num();
	auto Key = [&](const TCHAR* K, int32 Dir)
	{
		// .tab.k: Q and E, quiet, a little lower
		return SNew(Local::SS47Click).OnClick([Current, OnPick, Dir, N] { if (OnPick && N) OnPick((Current.Get(0) + Dir + N) % N); })
			[
				SNew(SBox).Padding(FMargin(18 * U, 10 * U, 18 * U, 10 * U))
				[
					SNew(STextBlock).Text(FText::FromString(K)).Font(Term(22, 2)).ColorAndOpacity(Rule())
				]
			];
	};
	Row->AddSlot().AutoWidth().VAlign(VAlign_Bottom)[Key(TEXT("Q"), -1)];
	for (int32 i = 0; i < N; ++i)
	{
		const TSharedRef<Local::SS47Click> Hit = SNew(Local::SS47Click).OnClick([OnPick, i] { if (OnPick) OnPick(i); });
		Hit->SetContent(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(18 * U, 6 * U, 18 * U, 7 * U)
			[
				SNew(STextBlock).Text(FText::FromString(Names[i])).Font(Term(27, 2))
				.ColorAndOpacity_Lambda([Current, i, W = TWeakPtr<Local::SS47Click>(Hit)] { const TSharedPtr<Local::SS47Click> H = W.Pin(); return FSlateColor(Current.Get(0) == i ? Amber() : H.IsValid() && H->IsHovered() ? Paper() : Mute()); })
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox).HeightOverride(3 * U)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor_Lambda([Current, i] { return FSlateColor(Current.Get(0) == i ? Amber() : FLinearColor::Transparent); })]
			]);
		Row->AddSlot().AutoWidth().VAlign(VAlign_Bottom).Padding(6 * U, 0, 0, 0)[Hit];
	}
	Row->AddSlot().AutoWidth().VAlign(VAlign_Bottom).Padding(6 * U, 0, 0, 0)[Key(TEXT("E"), 1)];
	// border-bottom: 1px solid rgba(235,227,207,.12) under the whole row
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[Row]
		+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Paper(0.12f))]];
}

TSharedRef<SWidget> S47Theme::Button(const FString& Label, bool bAmber, TFunction<void()> OnClick, TAttribute<bool> Usable, TAttribute<bool> Lit)
{
	const TSharedRef<Local::SS47Click> Hit = SNew(Local::SS47Click).OnClick([OnClick, Usable] { if (OnClick && Usable.Get(true)) OnClick(); });
	Hit->SetContent(
			SNew(SBorder).Padding(FMargin(22 * U, 6 * U)).VAlign(VAlign_Center)
			.BorderImage_Lambda([bAmber, Usable, Lit, W = TWeakPtr<Local::SS47Click>(Hit)]
			{
				const TSharedPtr<Local::SS47Click> H = W.Pin();
				const bool bOn = Usable.Get(true) && (Lit.Get(false) || (H.IsValid() && H->IsHovered()));
				const float A = Usable.Get(true) ? 1.f : 0.35f;
				if (bAmber) return Box(bOn ? Amber(0.16f) : FLinearColor::Transparent, Amber(A), 1.f);
				return Box(bOn ? Paper(0.06f) : FLinearColor::Transparent, bOn ? Mute() : Css(0x4d483f, A), 1.f);
			})
			[
				SNew(STextBlock).Text(FText::FromString(Label)).Font(Term(26, 2))
				.ColorAndOpacity_Lambda([bAmber, Usable, Lit, W = TWeakPtr<Local::SS47Click>(Hit)]
				{
					const TSharedPtr<Local::SS47Click> H = W.Pin();
					const bool bOn = Usable.Get(true) && (Lit.Get(false) || (H.IsValid() && H->IsHovered()));
					const float A = Usable.Get(true) ? 1.f : 0.35f;
					return FSlateColor(bAmber ? Amber(A) : bOn ? Paper() : Mute(A));
				})
			]);
	return Hit;
}

TSharedRef<SWidget> S47Theme::Backdrop(float Blur, const FLinearColor& Tint, bool bVignette, float CenterX)
{
	TSharedRef<SOverlay> O = SNew(SOverlay);
	if (Blur > 0.f) O->AddSlot()[SNew(SBackgroundBlur).BlurStrength(Blur * U).bApplyAlphaToBlur(true).Padding(0)[SNullWidget::NullWidget]];
	O->AddSlot()[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Tint).Padding(0)];
	if (bVignette) O->AddSlot()[SNew(Local::SS47Vig).CenterX(CenterX)];
	return O;
}

TSharedRef<SWidget> S47Theme::Scanlines(float Alpha)
{
	return SNew(Local::SS47Scan).Alpha(Alpha).Visibility(EVisibility::HitTestInvisible);
}

TSharedRef<SWidget> S47Theme::Frame(TSharedRef<SWidget> Content)
{
	return SNew(Local::SS47FrameBox)[Content];
}

const FScrollBarStyle* S47Theme::ScrollStyle()
{
	static const FScrollBarStyle Style = []
	{
		FScrollBarStyle S = FCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>(TEXT("ScrollBar"));
		const FSlateNoResource None;
		const FSlateColorBrush Thumb(Amber(0.35f)), Hot(Amber(0.7f));
		S.SetVerticalBackgroundImage(None).SetHorizontalBackgroundImage(None).SetVerticalTopSlotImage(None).SetVerticalBottomSlotImage(None)
			.SetHorizontalTopSlotImage(None).SetHorizontalBottomSlotImage(None).SetNormalThumbImage(Thumb).SetHoveredThumbImage(Hot).SetDraggedThumbImage(Hot);
		return S;
	}();
	return &Style;
}

void S47Theme::KeepFocus(const TSharedRef<SWidget>& Menu)
{
	if (!FSlateApplication::IsInitialized() || Menu->HasAnyUserFocusOrFocusedDescendants()) return;
	FSlateApplication::Get().SetAllUserFocus(Menu, EFocusCause::SetDirectly);
}
