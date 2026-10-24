#include "S47Keys.h"
#include "S47GameStyle.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateImageBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "SS47Widgets.h"
#include "Styling/CoreStyle.h"
#include "TextureResource.h"
#include "UObject/StrongObjectPtr.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace S47Game
{
	FLinearColor C(uint32 Rgb, float A)
	{
		FLinearColor Col = FLinearColor::FromSRGBColor(FColor((Rgb >> 16) & 255, (Rgb >> 8) & 255, Rgb & 255));
		Col.A = A;
		return Col;
	}

	namespace
	{
		FSlateFontInfo Spaced(FSlateFontInfo F, float Px, float Spacing)
		{
			// letter-spacing in pixels to Slate's thousandths of an em
			if (Spacing != 0.f && Px > 0.f) F.LetterSpacing = FMath::RoundToInt(Spacing / Px * 1000.f);
			return F;
		}
	}

	FSlateFontInfo Term(float Px, float Spacing) { return Spaced(S47Style::Mono(Px * U), Px, Spacing); }
	FSlateFontInfo Type(float Px, float Spacing) { return Spaced(S47Style::Body(Px * U), Px, Spacing); }
	FSlateFontInfo Hand(float Px, float Spacing) { return Spaced(S47Style::Hand(Px * U), Px, Spacing); }
	FSlateFontInfo Display(float Px, float Spacing) { return Spaced(S47Style::Font(TEXT("Oswald-Medium.ttf"), Px * U), Px, Spacing); }

	float LinePercent(const FSlateFontInfo& Font, float Pitch)
	{
		if (!FSlateApplication::IsInitialized()) return 1.f;
		const float H = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->GetMaxCharacterHeight(Font);
		return H > 0.f ? Pitch * U / H : 1.f;
	}

	const FSlateBrush* Box(const FLinearColor& Fill, const FLinearColor& Line, float LineW, float Radius)
	{
		// brushes live for the session; a few dozen in all
		static TMap<FString, TSharedPtr<FSlateBrush>> Cache;
		const FString Key = Fill.ToString() + Line.ToString() + FString::SanitizeFloat(LineW) + TEXT("/") + FString::SanitizeFloat(Radius);
		TSharedPtr<FSlateBrush>& B = Cache.FindOrAdd(Key);
		if (!B.IsValid())
		{
			if (Radius <= 0.f && LineW <= 0.f) B = MakeShared<FSlateColorBrush>(Fill);
			else B = MakeShared<FSlateRoundedBoxBrush>(Fill, FVector4(P(Radius), P(Radius), P(Radius), P(Radius)), Line, P(LineW));
		}
		return B.Get();
	}

	const FSlateBrush* Box4(const FLinearColor& Fill, const FVector4& Radii)
	{
		static TMap<FString, TSharedPtr<FSlateBrush>> Cache;
		const FString Key = Fill.ToString() + Radii.ToString();
		TSharedPtr<FSlateBrush>& B = Cache.FindOrAdd(Key);
		if (!B.IsValid()) B = MakeShared<FSlateRoundedBoxBrush>(Fill, Radii * U);
		return B.Get();
	}

	const FSlateBrush* Soft(float Blur)
	{
		// one blurred square for every shadow: full in the middle, falling off over the outer half
		// (smoothstep along each edge, so the corners round off as a gaussian's do), nine-sliced so
		// its edges are Blur wide whatever the size
		static TStrongObjectPtr<UTexture2D> Tex;
		if (!Tex.IsValid())
		{
			constexpr int32 N = 64;
			UTexture2D* T = UTexture2D::CreateTransient(N, N, PF_B8G8R8A8);
			if (!T) return FCoreStyle::Get().GetBrush("NoBorder");
			T->SRGB = false;
			T->Filter = TF_Bilinear;
			TArray<FColor> Px;
			Px.SetNum(N * N);
			auto Fall = [](int32 I) { const float D = FMath::Min(I + 0.5f, N - I - 0.5f) / (N * 0.5f); const float S = FMath::Clamp(D, 0.f, 1.f); return S * S * (3.f - 2.f * S); };
			for (int32 Y = 0; Y < N; ++Y) for (int32 X = 0; X < N; ++X) Px[Y * N + X] = FColor(255, 255, 255, uint8(FMath::RoundToInt(255.f * Fall(X) * Fall(Y))));
			void* Data = T->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(Data, Px.GetData(), Px.Num() * sizeof(FColor));
			T->GetPlatformData()->Mips[0].BulkData.Unlock();
			T->UpdateResource();
			Tex.Reset(T);
		}
		static TMap<int32, TSharedPtr<FSlateBrush>> Cache;
		const int32 K = FMath::Max(1, FMath::RoundToInt(Blur));
		TSharedPtr<FSlateBrush>& B = Cache.FindOrAdd(K);
		if (!B.IsValid())
		{
			B = MakeShared<FSlateBrush>();
			B->SetResourceObject(Tex.Get());
			B->ImageSize = FVector2D(2 * K * U, 2 * K * U);
			B->DrawAs = ESlateBrushDrawType::Box;
			B->Margin = FMargin(0.5f);
			B->TintColor = FLinearColor::Black;
		}
		return B.Get();
	}

	const FSlateBrush* Picture(UTexture2D* T)
	{
		static TMap<TWeakObjectPtr<UTexture2D>, TSharedPtr<FSlateBrush>> Cache;
		if (!T) return FCoreStyle::Get().GetBrush("NoBorder");
		TSharedPtr<FSlateBrush>& B = Cache.FindOrAdd(T);
		if (!B.IsValid())
		{
			B = MakeShared<FSlateBrush>();
			B->SetResourceObject(T);
			B->ImageSize = FVector2D(T->GetSizeX(), T->GetSizeY());
			B->DrawAs = ESlateBrushDrawType::Image;
		}
		return B.Get();
	}

	const FScrollBoxStyle* QuietScroll()
	{
		static const FScrollBoxStyle St = []
		{
			FScrollBoxStyle S = FCoreStyle::Get().GetWidgetStyle<FScrollBoxStyle>("ScrollBox");
			const FSlateBrush* None = FCoreStyle::Get().GetBrush("NoBorder");
			S.SetTopShadowBrush(*None).SetBottomShadowBrush(*None).SetLeftShadowBrush(*None).SetRightShadowBrush(*None);
			return S;
		}();
		return &St;
	}

	const FScrollBarStyle* QuietBar(const FLinearColor& Thumb)
	{
		static TMap<FString, TSharedPtr<FScrollBarStyle>> Cache;
		TSharedPtr<FScrollBarStyle>& St = Cache.FindOrAdd(Thumb.ToString());
		if (!St.IsValid())
		{
			St = MakeShared<FScrollBarStyle>(FCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>("ScrollBar"));
			const FSlateBrush* None = FCoreStyle::Get().GetBrush("NoBorder");
			FLinearColor Hot = Thumb;
			Hot.A = FMath::Min(1.f, Thumb.A * 1.8f);
			St->SetHorizontalBackgroundImage(*None).SetVerticalBackgroundImage(*None).SetVerticalTopSlotImage(*None).SetVerticalBottomSlotImage(*None)
				.SetHorizontalTopSlotImage(*None).SetHorizontalBottomSlotImage(*None)
				.SetNormalThumbImage(*Box(Thumb, FLinearColor::Transparent, 0.f, 2.f)).SetHoveredThumbImage(*Box(Hot, FLinearColor::Transparent, 0.f, 2.f)).SetDraggedThumbImage(*Box(Hot, FLinearColor::Transparent, 0.f, 2.f));
		}
		return St.Get();
	}

	TSharedRef<SWidget> Shadowed(const TSharedRef<SWidget>& Content, float Blur, FVector2D Offset, float Alpha)
	{
		// the shadow reaches Blur past the box on every side, moved by Offset (overlay slots may be negative)
		const float B = P(Blur);
		return SNew(SOverlay)
			+ SOverlay::Slot().Padding(FMargin(-B + P(Offset.X), -B + P(Offset.Y), -B - P(Offset.X), -B - P(Offset.Y)))
			[
				SNew(SImage).Image(Soft(Blur)).ColorAndOpacity(FLinearColor(0, 0, 0, Alpha))
			]
			+ SOverlay::Slot()[Content];
	}

	TSharedRef<SWidget> Turned(const TSharedRef<SWidget>& W, float Deg)
	{
		if (Deg != 0.f)
		{
			W->SetRenderTransform(FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(Deg))));
			W->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		}
		return W;
	}

	TSharedRef<SWidget> Drawing(UTexture2D* T, float MaxW, float MaxH, float Down, float Alpha)
	{
		if (!T) return SNew(SBox).WidthOverride(P(MaxW)).HeightOverride(P(MaxH));
		// object-fit: contain in MaxW x MaxH
		const float Iw = float(T->GetSizeX()), Ih = float(FMath::Max(1, T->GetSizeY()));
		const float K = FMath::Min(MaxW / Iw, MaxH / Ih);
		const float W = Iw * K, H = Ih * K;
		const FSlateBrush* Br = Picture(T);
		return SNew(SBox).WidthOverride(P(W)).HeightOverride(P(H + Down))
			[
				SNew(SOverlay)
				// drop-shadow(0 5px 6px rgba(60,40,20,.35)): the silhouette, brown and faint, a little lower
				+ SOverlay::Slot().Padding(FMargin(P(1), P(Down), P(-1), 0))[SNew(SBox).WidthOverride(P(W)).HeightOverride(P(H))[SNew(SImage).Image(Br).ColorAndOpacity(FLinearColor(0.045f, 0.022f, 0.008f, Alpha))]]
				+ SOverlay::Slot().Padding(FMargin(0, 0, 0, P(Down)))[SNew(SBox).WidthOverride(P(W)).HeightOverride(P(H))[SNew(SImage).Image(Br)]]
			];
	}

	TSharedRef<SWidget> Keycap(const FString& Key, EKey Style, float Px)
	{
		// .keys b: a thin frame, the key in paper; .key: amber, the key dark; .use b: ink brown on the page
		const FLinearColor Fill = Style == EKey::Amber ? Amber() : Style == EKey::Ink ? C(0x5d4f3a) : FLinearColor::Transparent;
		const FLinearColor Line = Style == EKey::Outline ? C(0x5d584e) : FLinearColor::Transparent;
		const FLinearColor Text = Style == EKey::Amber ? C(0x1b1408) : Style == EKey::Ink ? C(0xece4cf) : Paper();
		const float PadX = Style == EKey::Amber ? 8.f : Style == EKey::Ink ? 7.f : 6.f;
		return SNew(SBorder).BorderImage(Box(Fill, Line, Line.A > 0 ? 1.f : 0.f, Style == EKey::Outline ? 2.f : 0.f)).Padding(FMargin(P(PadX), 0))
			[
				SNew(STextBlock).Text(FText::FromString(Key)).Font(Term(Px)).ColorAndOpacity(Text)
			];
	}

	TSharedRef<SWidget> ActionKeycap(FName Action, EKey Style, float Px)
	{
		// .keys b: a thin frame, the key in paper; .key: amber, the key dark; .use b: ink brown on the page
		const FLinearColor Fill = Style == EKey::Amber ? Amber() : Style == EKey::Ink ? C(0x5d4f3a) : FLinearColor::Transparent;
		const FLinearColor Line = Style == EKey::Outline ? C(0x5d584e) : FLinearColor::Transparent;
		const FLinearColor Text = Style == EKey::Amber ? C(0x1b1408) : Style == EKey::Ink ? C(0xece4cf) : Paper();
		const float PadX = Style == EKey::Amber ? 8.f : Style == EKey::Ink ? 7.f : 6.f;
		return SNew(SBorder).BorderImage(Box(Fill, Line, Line.A > 0 ? 1.f : 0.f, Style == EKey::Outline ? 2.f : 0.f)).Padding(FMargin(P(PadX), 0))
			[
				SNew(STextBlock).Text_Lambda([Action] { return FText::FromString(S47Keys::Label(Action)); }).Font(Term(Px)).ColorAndOpacity(Text)
			];
	}

	TSharedRef<SWidget> KeyHint(const FString& Key, const FString& Label, float Px, const FLinearColor& Word, EKey Style)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, P(6), 0)[Keycap(Key, Style, Px)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Label)).Font(Term(Px, 1.f)).ColorAndOpacity(Word)];
	}

	TSharedRef<SWidget> Tape(float W, float H, float Deg)
	{
		// background: rgba(235,225,190,.75); box-shadow: 0 1px 2px rgba(0,0,0,.15)
		return Turned(Shadowed(SNew(SBox).WidthOverride(P(W)).HeightOverride(P(H))[SNew(SBorder).BorderImage(Box(C(0xebe1be, 0.75f)))], 2.f, FVector2D(0, 1), 0.15f), Deg);
	}

	TSharedRef<SWidget> Stamp(const FString& Text, float Px, float Deg, const FLinearColor& Color)
	{
		return Turned(SNew(SBorder).BorderImage(Box(FLinearColor::Transparent, Color, 2.f, 1.f)).Padding(FMargin(P(6), 0))
			[
				SNew(STextBlock).Text(FText::FromString(Text)).Font(Term(Px, 2.f)).ColorAndOpacity(Color)
			], Deg);
	}

	TSharedRef<SWidget> Osd(TAttribute<FText> Text, float Px)
	{
		// text-shadow: 2px 0 red, -2px 0 cyan, a white glow; the letters in paper white
		const FSlateFontInfo F = Term(Px, 2.f);
		auto Layer = [&](const FLinearColor& Col, float Dx) { return SNew(STextBlock).Text(Text).Font(F).ColorAndOpacity(Col).RenderTransform(FSlateRenderTransform(FVector2D(P(Dx), 0))); };
		return SNew(SOverlay)
			+ SOverlay::Slot()[Layer(FLinearColor(1.f, 0.05f, 0.05f, 0.55f), 2.f)]
			+ SOverlay::Slot()[Layer(FLinearColor(0.05f, 0.6f, 1.f, 0.45f), -2.f)]
			+ SOverlay::Slot()[SNew(STextBlock).Text(Text).Font(F).ColorAndOpacity(C(0xf4f1e8))];
	}

	FString DayLine(double Clock)
	{
		const double D = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
		return D >= 12 * 3600 ? TEXT("SUN APR 13") : TEXT("MON APR 14");
	}

	FString OsdLine(double Clock)
	{
		const double D = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
		return FString::Printf(TEXT("%s 1986  %02d:%02d"), D >= 12 * 3600 ? TEXT("APR.13") : TEXT("APR.14"), int32(D / 3600), int32(FMath::Fmod(D, 3600.0) / 60));
	}

	void SplitCaption(const FString& Text, FString& Who, FString& Line)
	{
		// the speaker in capitals before ": " (WARD:, N. VEGA:, RADIO, KOB:, CB, Breaker:); anything
		// else is a plain line
		Who.Reset();
		Line = Text;
		const int32 At = Text.Find(TEXT(": "));
		if (At <= 0 || At > 40) return;
		const FString Head = Text.Left(At);
		bool bCaps = false;
		for (const TCHAR Ch : Head)
		{
			if (FChar::IsLower(Ch) && !Head.StartsWith(TEXT("CB, ")) && !Head.StartsWith(TEXT("RADIO, "))) return;
			if (!(FChar::IsAlnum(Ch) || Ch == TEXT(' ') || Ch == TEXT('.') || Ch == TEXT(',') || Ch == TEXT('\'') || Ch == TEXT('-') || Ch > 127)) return;
			bCaps |= FChar::IsUpper(Ch);
		}
		if (!bCaps) return;
		Who = Head;
		Line = Text.Mid(At + 2);
	}

	void SplitNote(const FString& Note, FString& Lead, FString& Rest)
	{
		// the first ". " that ends a sentence (not "Dr. ", not an initial)
		Lead = Note; Rest.Reset();
		for (int32 I = 1; I + 1 < Note.Len(); ++I)
		{
			if (Note[I] != TEXT('.') || Note[I + 1] != TEXT(' ')) continue;
			const int32 WordStart = Note.Left(I).Find(TEXT(" "), ESearchCase::CaseSensitive, ESearchDir::FromEnd) + 1;
			const FString Word = Note.Mid(WordStart, I - WordStart);
			if (Word.Len() <= 1 || Word == TEXT("Dr") || Word == TEXT("Mr") || Word == TEXT("Mrs")) continue;
			Lead = Note.Left(I + 1);
			Rest = Note.Mid(I + 2).TrimStart();
			return;
		}
	}

	bool SelfTest(TArray<FString>& Lines)
	{
		bool bOk = true;
		auto Check = [&](const TCHAR* Name, bool bPass, const FString& Got = FString())
		{
			Lines.Add(bPass ? FString::Printf(TEXT("ok  game style: %s"), Name) : FString::Printf(TEXT("FAIL game style: %s %s"), Name, *Got));
			bOk &= bPass;
		};
		Check(TEXT("day before midnight"), DayLine(23 * 3600 + 41 * 60) == TEXT("SUN APR 13"));
		Check(TEXT("day after midnight"), DayLine(2 * 3600 + 13 * 60) == TEXT("MON APR 14") && DayLine(86400 + 600) == TEXT("MON APR 14"));
		Check(TEXT("osd line"), OsdLine(23 * 3600 + 58 * 60 + 30) == TEXT("APR.13 1986  23:58"), OsdLine(23 * 3600 + 58 * 60 + 30));
		FString Who, Line;
		SplitCaption(TEXT("N. VEGA: \"Sierra Motor Court.\""), Who, Line);
		Check(TEXT("caption speaker"), Who == TEXT("N. VEGA") && Line == TEXT("\"Sierra Motor Court.\""), Who + TEXT("|") + Line);
		SplitCaption(TEXT("CB, Breaker: \"Anybody out there?\""), Who, Line);
		Check(TEXT("caption CB"), Who == TEXT("CB, Breaker"), Who);
		SplitCaption(TEXT("Dale left twenty minutes ago."), Who, Line);
		Check(TEXT("plain line"), Who.IsEmpty() && Line == TEXT("Dale left twenty minutes ago."), Who);
		SplitCaption(TEXT("Note: the lever is red."), Who, Line);
		Check(TEXT("not a speaker"), Who.IsEmpty(), Who);
		FString Lead, Rest;
		SplitNote(TEXT("The SARO field camera. C raises it, Space takes the picture."), Lead, Rest);
		Check(TEXT("note lead"), Lead == TEXT("The SARO field camera.") && Rest == TEXT("C raises it, Space takes the picture."), Lead + TEXT("|") + Rest);
		SplitNote(TEXT("Tab opens it."), Lead, Rest);
		Check(TEXT("note one sentence"), Lead == TEXT("Tab opens it.") && Rest.IsEmpty(), Lead + TEXT("|") + Rest);
		return bOk;
	}
}

// ---------- the pieces that paint ----------

int32 SS47GamePage::OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const
{
	using namespace S47Game;
	const FVector2f S = FVector2f(G.GetLocalSize());
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	// the paper, rounded on its outer side (6px 0 0 6px on the left page)
	// (MakeBox takes the colour itself; a widget would multiply the brush's tint in first)
	FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(), Box4(Page(), bLeft ? FVector4(6, 0, 0, 6) : FVector4(0, 6, 6, 0)), ESlateDrawEffect::None, Page());
	// the lines (repeating-linear-gradient: transparent 0 33px, rgba(35,58,122,.17) 33px 34px)
	for (float Y = P(First); Y < S.Y - 1.f; Y += P(Pitch))
		FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(FVector2f(S.X, FMath::Max(1.f, P(1))), FSlateLayoutTransform(FVector2f(0.f, Y))), White, ESlateDrawEffect::None, C(0x233a7a, 0.17f));
	// the red margin of a left-hand page
	if (bLeft && Margin > 0.f)
		FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(FVector2f(FMath::Max(1.f, P(1)), S.Y), FSlateLayoutTransform(FVector2f(P(Margin), 0.f))), White, ESlateDrawEffect::None, FLinearColor(0.4f, 0.02f, 0.02f, 0.35f));
	// inset 0 0 40px rgba(120,95,50,.18): the page darker towards its edges
	const FLinearColor Edge = C(0x785f32, 0.18f), None = C(0x785f32, 0.f);
	const float E = P(40);
	auto Grad = [&](FVector2f At, FVector2f Size, const FLinearColor& A, const FLinearColor& B, EOrientation O)
	{
		FSlateDrawElement::MakeGradient(Out, Layer + 2, G.ToPaintGeometry(Size, FSlateLayoutTransform(At)), { FSlateGradientStop(FVector2D(0, 0), A), FSlateGradientStop(FVector2D(Size.X, Size.Y), B) }, O);
	};
	Grad(FVector2f(0, 0), FVector2f(S.X, E), Edge, None, Orient_Horizontal);
	Grad(FVector2f(0, S.Y - E), FVector2f(S.X, E), None, Edge, Orient_Horizontal);
	Grad(FVector2f(0, 0), FVector2f(E, S.Y), Edge, None, Orient_Vertical);
	Grad(FVector2f(S.X - E, 0), FVector2f(E, S.Y), None, Edge, Orient_Vertical);
	// inset 18px 0 28px -16px rgba(60,40,20,.45): the shade where the page goes into the spine
	const FLinearColor Spine = C(0x3c2814, 0.45f), Clear = C(0x3c2814, 0.f);
	const float Sw = P(30);
	if (bLeft) Grad(FVector2f(S.X - Sw, 0), FVector2f(Sw, S.Y), Clear, Spine, Orient_Vertical);
	else Grad(FVector2f(0, 0), FVector2f(Sw, S.Y), Spine, Clear, Orient_Vertical);
	return Layer + 3;
}

int32 SS47GameSheet::OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const
{
	using namespace S47Game;
	const FVector2f S = FVector2f(G.GetLocalSize());
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(), White, ESlateDrawEffect::None, Color);
	// linear-gradient(180deg, transparent, rgba(120,95,50,.07))
	FSlateDrawElement::MakeGradient(Out, Layer + 1, G.ToPaintGeometry(), { FSlateGradientStop(FVector2D(0, 0), C(0x785f32, 0.f)), FSlateGradientStop(FVector2D(0, S.Y), C(0x785f32, 0.07f)) }, Orient_Horizontal);
	if (bTorn)
	{
		// the ticks the pad's perforation left (every 10px, as deep as the torn edge's zigzag)
		static const float Zig[][2] = { { 0, .6f }, { .04f, .35f }, { .1f, .65f }, { .18f, .4f }, { .26f, .7f }, { .33f, .36f }, { .4f, .66f }, { .48f, .42f }, { .55f, .72f }, { .62f, .38f }, { .7f, .65f }, { .77f, .4f }, { .84f, .7f }, { .9f, .35f }, { .96f, .6f }, { 1.f, .3f } };
		const float Hgt = P(46);
		for (float X = P(9); X < S.X; X += P(10))
		{
			const float U01 = X / S.X;
			int32 I = 0;
			while (I < int32(UE_ARRAY_COUNT(Zig)) - 2 && Zig[I + 1][0] < U01) ++I;
			const float T = (U01 - Zig[I][0]) / FMath::Max(0.001f, Zig[I + 1][0] - Zig[I][0]);
			const float Depth = FMath::Lerp(Zig[I][1], Zig[I + 1][1], FMath::Clamp(T, 0.f, 1.f)) * Hgt;
			FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(FVector2f(FMath::Max(1.f, P(1)), Depth), FSlateLayoutTransform(FVector2f(X, 0))), White, ESlateDrawEffect::None, FLinearColor(0, 0, 0, 0.08f));
		}
	}
	return Layer + 3;
}

int32 SS47GameDash::OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const
{
	using namespace S47Game;
	const FVector2f S = FVector2f(G.GetLocalSize());
	const float In = P(Inset) + P(1), Dash = P(6), Gap = P(4), W = P(2);
	auto Side = [&](FVector2f A, FVector2f B)
	{
		const float Len = FVector2f::Distance(A, B);
		const FVector2f Dir = (B - A) / FMath::Max(Len, 0.001f);
		for (float T = 0; T < Len; T += Dash + Gap)
			FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), TArray<FVector2f>{ A + Dir * T, A + Dir * FMath::Min(Len, T + Dash) }, ESlateDrawEffect::None, Color, false, W);
	};
	const FVector2f A(In, In), B(S.X - In, In), Cc(S.X - In, S.Y - In), D(In, S.Y - In);
	Side(A, B); Side(B, Cc); Side(Cc, D); Side(D, A);
	return Layer + 1;
}
