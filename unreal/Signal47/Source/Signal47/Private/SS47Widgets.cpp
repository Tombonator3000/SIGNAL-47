#include "SS47Widgets.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "HAL/FileManager.h"
#include "Internationalization/Regex.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "S47Image.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47UI.h"
#include "Widgets/Layout/SScaleBox.h"
#include "S47Rx.h"
#include "S47Story.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace S47Style
{
	FSlateFontInfo Font(const TCHAR* File, float Px)
	{
		static TMap<FString, TSharedPtr<FStandaloneCompositeFont>> Cache;
		FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Fonts") / File);
		if (!IFileManager::Get().FileExists(*Path))
		{
			Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Fonts/JetBrainsMono-Regular.ttf"));
		}
		TSharedPtr<FStandaloneCompositeFont>& F = Cache.FindOrAdd(Path);
		if (!F.IsValid())
		{
			F = MakeShared<FStandaloneCompositeFont>();
			F->DefaultTypeface.AppendFont(TEXT("Regular"), Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		}
		return FSlateFontInfo(F, Px * 0.75f, TEXT("Regular"));
	}
	FSlateFontInfo Mono(float Px) { return Font(TEXT("VT323-Regular.ttf"), Px); }
	FSlateFontInfo Body(float Px) { return Font(TEXT("SpecialElite-Regular.ttf"), Px); }
	FSlateFontInfo Hand(float Px) { return Font(TEXT("ReenieBeanie-Regular.ttf"), Px); }
	FSlateFontInfo Display(float Px) { return Font(TEXT("Oswald-Regular.ttf"), Px); }

	// A button in the game's plain style.
	TSharedRef<SWidget> Button(const FString& Label, TFunction<void()> OnClick, const FLinearColor& Color = Amber, float Px = 22.f)
	{
		static FButtonStyle Style = []
		{
			FButtonStyle S;
			const FSlateRoundedBoxBrush B(FLinearColor(1, 1, 1, 0.08f), FVector4(4.f, 4.f, 4.f, 4.f), FLinearColor(1, 1, 1, 0.25f), 1.f);
			const FSlateRoundedBoxBrush H(FLinearColor(1, 1, 1, 0.18f), FVector4(4.f, 4.f, 4.f, 4.f), FLinearColor(1, 1, 1, 0.4f), 1.f);
			S.SetNormal(B).SetHovered(H).SetPressed(B).SetDisabled(B);
			S.SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
			return S;
		}();
		return SNew(SButton).ButtonStyle(&Style).ContentPadding(FMargin(14, 6)).OnClicked_Lambda([OnClick] { if (OnClick) OnClick(); return FReply::Handled(); })
			[
				SNew(STextBlock).Text(FText::FromString(Label)).Font(Mono(Px)).ColorAndOpacity(Color)
			];
	}

	// A page of text: lines that start with "~ " are pencil, the rest in the paper's type.
	TSharedRef<SWidget> Page(const FString& Text, const FSlateFontInfo& Type, const FLinearColor& Color)
	{
		TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
		TArray<FString> Lines;
		Text.ParseIntoArray(Lines, TEXT("\n"), false);
		for (const FString& L : Lines)
		{
			const bool bPencil = L.StartsWith(TEXT("~ "));
			Box->AddSlot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::FromString(bPencil ? L.Mid(2) : (L.IsEmpty() ? TEXT(" ") : L))).AutoWrapText(true)
					.Font(bPencil ? Body(22.f) : Type).ColorAndOpacity(bPencil ? Pencil : Color)
			];
		}
		return Box;
	}
}

// ---------- the overlay ----------

void SS47Hud::Construct(const FArguments& InArgs)
{
	DotBrush = MakeUnique<FSlateRoundedBoxBrush>(FLinearColor::White, FVector4(3.f, 3.f, 3.f, 3.f));
	WhiteBrush = MakeUnique<FSlateColorBrush>(FLinearColor::White);
	DescBrush = MakeUnique<FSlateRoundedBoxBrush>(FLinearColor::FromSRGBColor(FColor(6, 8, 12)) * FLinearColor(1, 1, 1, 0.85f), 3.f, FLinearColor::FromSRGBColor(FColor(0x78, 0x6b, 0x51)), 1.f);
	TAttribute<bool> ShowAim = InArgs._ShowAim;
	auto Vis = [ShowAim] { return ShowAim.Get(true) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; };
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(5.f).HeightOverride(5.f).Visibility_Lambda(Vis)
			[
				SNew(SBorder).BorderImage(DotBrush.Get()).BorderBackgroundColor(FLinearColor(1, 1, 1, 0.6f))
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, 140)
		[
			SAssignNew(ToastBox, SVerticalBox)
		]
		// the prompt, a little under the crosshair (.prompt), and a closer look's words under it (.examine);
		// at the line its top at 64% of the screen (.at-line .prompt), the line 26 px high as the 78 has it
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		.Padding(TAttribute<FMargin>::CreateLambda([this] { return FMargin(0, bAtLine ? 2 * (0.64f * float(GetCachedGeometry().GetLocalSize().Y) + 13.f) - float(GetCachedGeometry().GetLocalSize().Y) : 78.f, 0, 0); }))
		[
			SNew(STextBlock).Text(InArgs._Prompt).Font(S47Style::Mono(26.f)).ColorAndOpacity(FLinearColor(0.94f, 0.92f, 0.85f)).ShadowOffset(FVector2D(0, 1)).ShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f)).Visibility_Lambda(Vis)
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0, 190, 0, 0)
		[
			SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.024f, 0.03f, 0.047f, 0.55f)).Padding(FMargin(10, 4))
			.Visibility_Lambda([this, Vis] { return ExamineText.IsEmpty() ? EVisibility::Collapsed : Vis(); })
			[
				SNew(STextBlock).Text_Lambda([this] { return FText::FromString(ExamineText); }).Font(S47Style::Mono(24.f)).ColorAndOpacity(FLinearColor(0.95f, 0.92f, 0.83f)).AutoWrapText(true).WrapTextAt(640.f).Justification(ETextJustify::Center)
			]
		]
		// the objective (top left, amber)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(22, 18, 0, 0)
		[
			SNew(STextBlock).Text_Lambda([this] { return FText::FromString(ObjectiveText); }).Font(S47Style::Mono(22.f)).ColorAndOpacity(FLinearColor(1.f, 0.78f, 0.47f, 0.88f)).ShadowOffset(FVector2D(0, 1)).ShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f)).AutoWrapText(true).WrapTextAt(720.f)
			.Visibility_Lambda([this] { return ObjectiveText.IsEmpty() || Vf.bOn ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0, 24, 32, 0)
		[
			SNew(STextBlock).Text_Lambda([this, Clock = InArgs._Clock] { return SecondsText.IsEmpty() ? Clock.Get() : FText::FromString(SecondsText); }).Font(S47Style::Mono(30.f))
			.ColorAndOpacity_Lambda([this] { return SecondsText.IsEmpty() ? FSlateColor(FLinearColor(0.93f, 0.9f, 0.84f, 0.55f)) : FSlateColor(FLinearColor(1.f, 0.82f, 0.48f)); })
		]
		// .description-read: under the clock at the right, dark with a brown line; dimmed when there is no
		// recording of the text (V reads it: with the mouse held by the game, a label more than a button)
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0, 68, 22, 0)
		[
			SNew(SBorder).BorderImage(DescBrush.Get()).Padding(FMargin(10, 6))
			.Visibility_Lambda([this, Vis] { return DescButton.bShown ? Vis() : EVisibility::Collapsed; })
			.ColorAndOpacity_Lambda([this] { return FLinearColor(1, 1, 1, DescButton.bDisabled ? 0.5f : 1.f); })
			[
				SNew(SBox).MinDesiredHeight(44.f - 12.f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text_Lambda([this] { return FText::FromString(DescButton.Text); }).Font(S47Style::Mono(20.f)).ColorAndOpacity(FLinearColor::FromSRGBColor(FColor(0xf3, 0xea, 0xd3)))
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, 28)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("W A S D  MOVE   MOUSE  LOOK   E  USE   TAB  NOTEBOOK"))).Font(S47Style::Mono(18.f)).ColorAndOpacity(FLinearColor(0.93f, 0.9f, 0.84f, 0.35f)).Visibility_Lambda(Vis)
		]
	];
}

void SS47Hud::Toast(const FString& Text, float Secs, TFunction<double()> Clock)
{
	// Captions already have their own radio/telephone delivery. Do not turn them
	// into a second narrator or replace the last environmental description.
	static const FRegexPattern Caption(TEXT("^(?:RADIO(?:,|:)|CB,|WARD:|DR\\. WARD:|NORA:|N\\. VEGA:|THE LINE:|YOU:|TOM[ÁA]S:)"), ERegexPatternFlags::CaseInsensitive);
	FRegexMatcher M(Caption, Text);
	if (!M.FindNext()) LastToast = Text;
	// UI.toast: the same line twice in a row is noise; at most two on screen
	if (Toasts.Num() && Toasts.Last().Text == Text) return;
	// UI.ts toast(text, secs, clock): recorded captions share the pausable sound clock.
	const double At = Clock ? Clock() : Now;
	Toasts.Add({ Text, At + Secs, MoveTemp(Clock) });
	if (Toasts.Num() > 2) Toasts.RemoveAt(0);
	Rebuild();
}

void SS47Hud::ClearToasts() { Toasts.Empty(); LastToast.Empty(); Rebuild(); }

void SS47Hud::Rebuild()
{
	if (!ToastBox.IsValid()) return;
	ToastBox->ClearChildren();
	for (const FToast& T : Toasts)
	{
		ToastBox->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0, 3)
		[
			SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.75f)).Padding(FMargin(16, 6))
			[
				SNew(STextBlock).Text(FText::FromString(T.Text)).Font(S47Style::Body(22.f)).ColorAndOpacity(S47Style::Paper).AutoWrapText(true).WrapTextAt(760.f).Justification(ETextJustify::Center)
			]
		];
	}
}

void SS47Hud::Fade(bool bOn, const FString& Text)
{
	bFadeOn = bOn;
	FadeText = Text;
}

void SS47Hud::Tick(const FGeometry& G, const double InCurrentTime, const float Dt)
{
	SCompoundWidget::Tick(G, InCurrentTime, Dt);
	Now += Dt;
	const int32 Before = Toasts.Num();
	Toasts.RemoveAll([this](const FToast& T) { return T.Until <= (T.Clock ? T.Clock() : Now); });
	if (Toasts.Num() != Before) Rebuild();
	FlashT = FMath::Max(0.f, FlashT - Dt);
	const float Target = bFadeOn ? 1.f : 0.f;
	FadeAlpha += (Target - FadeAlpha) * FMath::Min(1.f, Dt * (bFadeOn ? 2.5f : 1.8f));
	if (FMath::Abs(Target - FadeAlpha) < 0.01f) FadeAlpha = Target;
}

int32 SS47Hud::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	int32 Top = SCompoundWidget::OnPaint(Args, G, Cull, Out, Layer, Style, bParentEnabled);
	if (Vf.bOn)
	{
		// .viewfinder: the dark round the frame, its corners, the patch in the middle and the words
		const FVector2f S = FVector2f(G.GetLocalSize());
		const FSlateRect R = Vf.Rect;
		const FLinearColor Dark(0, 0, 0, 0.72f), Line(1.f, 0.96f, 0.84f, 0.85f);
		auto Box = [&](float X0, float Y0, float X1, float Y1, const FLinearColor& C)
		{
			if (X1 <= X0 || Y1 <= Y0) return;
			FSlateDrawElement::MakeBox(Out, Top + 1, G.ToPaintGeometry(FVector2f(X1 - X0, Y1 - Y0), FSlateLayoutTransform(FVector2f(X0, Y0))), WhiteBrush.Get(), ESlateDrawEffect::None, C);
		};
		Box(0, 0, S.X, R.Top, Dark); Box(0, R.Bottom, S.X, S.Y, Dark); Box(0, R.Top, R.Left, R.Bottom, Dark); Box(R.Right, R.Top, S.X, R.Bottom, Dark);
		auto Lines = [&](TArray<FVector2f> P, const FLinearColor& C, float W) { FSlateDrawElement::MakeLines(Out, Top + 2, G.ToPaintGeometry(), P, ESlateDrawEffect::None, C, true, W); };
		const float K = 26.f, In = 10.f;
		Lines({ FVector2f(R.Left + In, R.Top + In + K), FVector2f(R.Left + In, R.Top + In), FVector2f(R.Left + In + K, R.Top + In) }, Line, 2.f);
		Lines({ FVector2f(R.Right - In - K, R.Top + In), FVector2f(R.Right - In, R.Top + In), FVector2f(R.Right - In, R.Top + In + K) }, Line, 2.f);
		Lines({ FVector2f(R.Left + In, R.Bottom - In - K), FVector2f(R.Left + In, R.Bottom - In), FVector2f(R.Left + In + K, R.Bottom - In) }, Line, 2.f);
		Lines({ FVector2f(R.Right - In - K, R.Bottom - In), FVector2f(R.Right - In, R.Bottom - In), FVector2f(R.Right - In, R.Bottom - In - K) }, Line, 2.f);
		const FVector2f C((R.Left + R.Right) / 2, (R.Top + R.Bottom) / 2);
		Lines({ C + FVector2f(-23, -15), C + FVector2f(23, -15), C + FVector2f(23, 15), C + FVector2f(-23, 15), C + FVector2f(-23, -15) }, FLinearColor(1.f, 0.84f, 0.55f, 0.7f), 1.f);
		auto Text = [&](const FString& T, float X, float Y, float Px, const FLinearColor& Col, bool bCentre)
		{
			if (T.IsEmpty()) return;
			const FSlateFontInfo F = S47Style::Mono(Px);
			const float W = bCentre ? FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(T, F).X : 0.f;
			FSlateDrawElement::MakeText(Out, Top + 3, G.ToPaintGeometry(FVector2f(900.f, Px * 1.4f), FSlateLayoutTransform(FVector2f(X - W / 2, Y))), T, F, ESlateDrawEffect::None, Col);
		};
		Text(TEXT("SARO / FIELD CAMERA"), R.Left + 46, R.Top + 14, 16.f * 1.3f, Line, false);
		Text(Vf.Frame, R.Right - 46 - 60, R.Top + 14, 18.f * 1.3f, S47Style::Amber, false);
		Text(Vf.Free, R.Right - 46 - 260, R.Top + 40, 16.f * 1.3f, Vf.bFreeOk ? S47Style::Green : FLinearColor(1.f, 0.96f, 0.84f, 0.6f), false);
		const FLinearColor St = Vf.bOk ? S47Style::Green : FLinearColor(1.f, 0.7f, 0.63f);
		Text(Vf.Status, C.X, R.Bottom - 40 - 26, 19.f * 1.3f, Vf.bFreeOk ? St * FLinearColor(1, 1, 1, 0.5f) : St, true);
		Text(Vf.Hint, C.X, R.Bottom - 14 - 22, 15.f * 1.3f, Line * FLinearColor(1, 1, 1, 0.75f), true);
		Top += 4;
	}
	if (FlashT > 0.f)
	{
		// the shutter: black, held a third of the way, then gone
		const float U = 1.f - FlashT / 0.28f;
		const float A = U < 0.35f ? 1.f : 1.f - (U - 0.35f) / 0.65f;
		FSlateDrawElement::MakeBox(Out, Top + 1, G.ToPaintGeometry(), WhiteBrush.Get(), ESlateDrawEffect::None, FLinearColor(0, 0, 0, A));
		Top += 1;
	}
	if (FadeAlpha > 0.f)
	{
		FSlateDrawElement::MakeBox(Out, Top + 1, G.ToPaintGeometry(), WhiteBrush.Get(), ESlateDrawEffect::None, FLinearColor(0, 0, 0, FadeAlpha));
		if (!FadeText.IsEmpty())
		{
			const FVector2f S = FVector2f(G.GetLocalSize());
			FSlateDrawElement::MakeText(Out, Top + 2, G.ToPaintGeometry(FVector2f(400.f, 80.f), FSlateLayoutTransform(FVector2f(S.X / 2 - 60.f, S.Y / 2 - 30.f))), FadeText, S47Style::Mono(64.f), ESlateDrawEffect::None, FLinearColor(0.93f, 0.9f, 0.84f, FadeAlpha));
		}
		return Top + 2;
	}
	return Top;
}

// ---------- a paper ----------

void SS47Document::Construct(const FArguments& InArgs)
{
	OnClose = InArgs._OnClose;
	const FS47Doc& D = InArgs._Doc;
	const bool bHand = D.Kind == TEXT("hand");
	const bool bPrint = D.Kind == TEXT("printout");
	const FLinearColor PaperColor = bPrint ? FLinearColor(0.9f, 0.9f, 0.86f) : bHand ? FLinearColor(0.95f, 0.93f, 0.85f) : FLinearColor(0.92f, 0.9f, 0.82f);
	PaperBrush = MakeUnique<FSlateRoundedBoxBrush>(PaperColor, FVector4(2.f, 2.f, 2.f, 2.f), FLinearColor(0.6f, 0.55f, 0.45f), 1.f);
	PanelBrush = MakeUnique<FSlateRoundedBoxBrush>(FLinearColor(0.05f, 0.05f, 0.05f, 0.9f), FVector4(6.f, 6.f, 6.f, 6.f));
	const FSlateFontInfo Type = bHand ? S47Style::Body(24.f) : bPrint ? S47Style::Mono(26.f) : S47Style::Body(21.f);
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot() [ SNew(SImage).ColorAndOpacity(S47Style::Overlay) ]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(1240.f).HeightOverride(760.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.55f).Padding(0, 0, 16, 0)
				[
					PageWidget(D, Type)
				]
				+ SHorizontalBox::Slot().FillWidth(0.45f)
				[
					SNew(SBorder).BorderImage(PanelBrush.Get()).Padding(FMargin(28, 24))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
						[
							SNew(STextBlock).Text(FText::FromString(D.Title)).Font(S47Style::Mono(30.f)).ColorAndOpacity(S47Style::Paper)
						]
						+ SVerticalBox::Slot().FillHeight(1.f)
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								SNew(STextBlock).Text(FText::FromString(D.Transcript)).AutoWrapText(true).Font(S47Style::Body(19.f)).ColorAndOpacity(FLinearColor(0.85f, 0.83f, 0.78f)).LineHeightPercentage(1.25f)
							]
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							InArgs._Reading.IsValid() ? InArgs._Reading.ToSharedRef() : SNullWidget::NullWidget
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, 14, 0, 0)
						[
							S47Style::Button(TEXT("PUT IT DOWN   [ESC]"), [this] { if (OnClose) OnClose(); })
						]
					]
				]
			]
		]
	];
}

TSharedRef<SWidget> SS47Document::PageWidget(const FS47Doc& D, const FSlateFontInfo& Type)
{
	// UI.document: a photograph or a map is the picture itself; a paper may carry a picture under
	// its text (a drawing, the transit plan) and may be drawn on its own paper (art: a card, a letter)
	auto Pic = [](const FString& Name) -> UTexture2D* { return Name.IsEmpty() ? nullptr : Name.StartsWith(TEXT("data:")) ? S47Image::FromDataUrl(Name) : S47Image::Art(Name); };
	UTexture2D* Image = Pic(D.Image);
	UTexture2D* PaperArt = Pic(D.Paper);
	if (Image && (D.Kind == TEXT("photo") || D.Kind == TEXT("map")))
	{
		ImageBrush = S47Kit::BrushFor(Image);
		return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
				[
					SNew(SImage).Image(ImageBrush.Get())
				]
			];
	}
	TSharedRef<SVerticalBox> Col = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
		[
			SNew(STextBlock).Text(FText::FromString(D.Stamp.IsEmpty() ? TEXT("") : D.Stamp)).Font(S47Style::Mono(20.f)).ColorAndOpacity(FLinearColor(0.55f, 0.15f, 0.12f))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			S47Style::Page(D.Page, Type, S47Style::Ink)
		];
	if (Image)
	{
		ImageBrush = S47Kit::BrushFor(Image);
		const float W = 560.f, H = W * ImageBrush->ImageSize.Y / FMath::Max(1.0, ImageBrush->ImageSize.X);
		Col->AddSlot().AutoHeight().Padding(0, 16, 0, 0).HAlign(HAlign_Left)[SNew(SBox).WidthOverride(W).HeightOverride(H)[SNew(SImage).Image(ImageBrush.Get())]];
	}
	TSharedRef<SWidget> Text = SNew(SScrollBox) + SScrollBox::Slot()[Col];
	if (PaperArt)
	{
		PaperArtBrush = S47Kit::BrushFor(PaperArt);
		return SNew(SOverlay)
			+ SOverlay::Slot()[SNew(SImage).Image(PaperArtBrush.Get())]
			+ SOverlay::Slot().Padding(FMargin(48, 40))[Text];
	}
	return SNew(SBorder).BorderImage(PaperBrush.Get()).Padding(FMargin(40, 36))[Text];
}

FReply SS47Document::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::E || InKeyEvent.GetKey() == EKeys::Tab)
	{
		if (OnClose) OnClose();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ---------- the spectrum ----------

void SS47Spectrum::Construct(const FArguments& InArgs)
{
	Story = InArgs._Story;
	Size = InArgs._Size;
	WhiteBrush = MakeUnique<FSlateColorBrush>(FLinearColor::White);
}

void SS47Spectrum::Paint(const AS47Story* Story, const FGeometry& G, FSlateWindowElementList& Out, int32 Layer, const FSlateBrush* White)
{
	const FVector2f S = FVector2f(G.GetLocalSize());
	FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(), White, ESlateDrawEffect::None, FLinearColor(0.008f, 0.063f, 0.024f));
	if (!Story) return;
	FS47SpectrumGeometry Geo;
	const double T = FPlatformTime::Seconds();
	S47Spectrum::Build(Story->Rx, T, Story->bResidualVisible, S.X, S.Y, [] { return double(FMath::FRand()); }, Geo);
	const FLinearColor Grid(0.24f, 0.7f, 0.37f, 0.22f);
	for (float X : Geo.GridX) FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(X, 0), FVector2f(X, S.Y) }, ESlateDrawEffect::None, Grid, false, 1.f);
	for (float Y : Geo.GridY) FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(0, Y), FVector2f(S.X, Y) }, ESlateDrawEffect::None, Grid, false, 1.f);
	if (!Geo.bPowered) return;
	FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(FVector2f(Geo.BandX1 - Geo.BandX0, S.Y), FSlateLayoutTransform(FVector2f(Geo.BandX0, 0))), White, ESlateDrawEffect::None, FLinearColor(0.27f, 1.f, 0.47f, 0.08f));
	if (Geo.Trace.Num() > 1)
	{
		FSlateDrawElement::MakeLines(Out, Layer + 3, G.ToPaintGeometry(), Geo.Trace, ESlateDrawEffect::None, FLinearColor(0.35f, 1.f, 0.5f, 0.35f), true, FMath::Max(3.f, S.X / 170.f));
		FSlateDrawElement::MakeLines(Out, Layer + 4, G.ToPaintGeometry(), Geo.Trace, ESlateDrawEffect::None, FLinearColor(0.55f, 1.f, 0.64f), true, FMath::Max(1.5f, S.X / 340.f));
	}
	// the tuned frequency as a dashed line
	for (float Y = 0; Y < S.Y; Y += 6.f)
	{
		FSlateDrawElement::MakeLines(Out, Layer + 5, G.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(Geo.TunedX, Y), FVector2f(Geo.TunedX, FMath::Min(S.Y, Y + 3.f)) }, ESlateDrawEffect::None, FLinearColor(1.f, 0.78f, 0.35f, 0.85f), false, 1.f);
	}
}

int32 SS47Spectrum::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	Paint(Story, G, Out, Layer, WhiteBrush.Get());
	return Layer + 6;
}

// ---------- the receiver console ----------

TSharedRef<SWidget> SS47Console::Control(const TCHAR* Label, TFunction<double()> Get, TFunction<void(double)> Set, double Min, double Max, TFunction<FString(double)> Fmt, bool bFine)
{
	TSharedRef<SHorizontalBox> Fine = SNew(SHorizontalBox);
	if (bFine)
	{
		for (double Step : { -0.010, -0.001, 0.001, 0.010 })
		{
			const FString L = Step < 0 ? (Step < -0.005 ? TEXT("--") : TEXT("-")) : (Step > 0.005 ? TEXT("++") : TEXT("+"));
			Fine->AddSlot().AutoWidth().Padding(3, 0)
			[
				S47Style::Button(L, [this, Get, Set, Step, Min, Max] { Set(FMath::Clamp(FMath::RoundToDouble((Get() + Step) * 1000.0) / 1000.0, Min, Max)); if (OnClick) OnClick(); }, S47Style::Green, 20.f)
			];
		}
	}
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 2)
		[
			SNew(STextBlock).Text_Lambda([Label, Get, Fmt] { return FText::FromString(FString::Printf(TEXT("%s: %s"), Label, *Fmt(Get()))); }).Font(S47Style::Mono(24.f)).ColorAndOpacity(S47Style::Green)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SSlider).Value_Lambda([Get, Min, Max] { return float((Get() - Min) / (Max - Min)); })
					.OnValueChanged_Lambda([Set, Min, Max, bFine](float V) { const double R = Min + V * (Max - Min); Set(bFine ? FMath::RoundToDouble(R * 1000.0) / 1000.0 : FMath::RoundToDouble(R)); })
					.SliderBarColor(FLinearColor(0.24f, 0.5f, 0.3f)).SliderHandleColor(S47Style::Green)
					.IsEnabled_Lambda([this] { return Story && Story->Rx.Stage < 3 && !Story->SurveyOnly(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0, 0, 0) [ Fine ]
		];
}

void SS47Console::Construct(const FArguments& InArgs)
{
	Story = InArgs._Story;
	OnClose = InArgs._OnClose;
	OnClick = InArgs._OnClick;
	PanelBrush = MakeUnique<FSlateRoundedBoxBrush>(FLinearColor(0.01f, 0.04f, 0.02f), FVector4(6.f, 6.f, 6.f, 6.f), FLinearColor(0.3f, 0.6f, 0.4f, 0.5f), 1.f);
	FS47Rx* Rx = Story ? &Story->Rx : nullptr;
	auto Az = [this] { if (Story) Story->ConsoleAzimuth(); };
	TSharedPtr<SVerticalBox> LeftCol;
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot() [ SNew(SImage).ColorAndOpacity(S47Style::Overlay) ]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(1320.f)
			[
				SNew(SBorder).BorderImage(PanelBrush.Get()).Padding(FMargin(28, 22))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(0.62f).Padding(0, 0, 24, 0)
					[
						SAssignNew(LeftCol, SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f) [ SNew(STextBlock).Text(FText::FromString(TEXT("SARO / RX CONTROL 03"))).Font(S47Style::Mono(30.f)).ColorAndOpacity(S47Style::Green) ]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(STextBlock).Text_Lambda([this] { const double Q = !Story ? 0 : Story->Rx.Stage >= 3 ? 1.0 : Story->SurveyOnly() ? 0.0 : Story->Rx.Quality(); return FText::FromString(FString::Printf(TEXT("CARRIER %d%%"), FMath::RoundToInt(Q * 100))); }).Font(S47Style::Mono(24.f)).ColorAndOpacity(S47Style::GreenDim)
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 8)
						[
							SNew(STextBlock).Text_Lambda([this] { return FText::FromString(Story ? Story->Rx.Status : FString()); }).Font(S47Style::Mono(24.f))
								.ColorAndOpacity_Lambda([this] { return FSlateColor(Story && (Story->Rx.Status.StartsWith(TEXT("NO LOCK")) || Story->Rx.Status.StartsWith(TEXT("UNLOGGED"))) ? S47Style::Amber : S47Style::Green); })
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).HeightOverride(280.f) [ SNew(SS47Spectrum).Story(Story).Size(FVector2D(800, 280)) ]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 10)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f) [ SNew(STextBlock).Text(FText::FromString(TEXT("1419.5"))).Font(S47Style::Mono(18.f)).ColorAndOpacity(S47Style::GreenDim) ]
							+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center) [ SNew(STextBlock).Text(FText::FromString(TEXT("1419.8"))).Font(S47Style::Mono(18.f)).ColorAndOpacity(S47Style::GreenDim) ]
							+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center) [ SNew(STextBlock).Text(FText::FromString(TEXT("1420.1"))).Font(S47Style::Mono(18.f)).ColorAndOpacity(S47Style::GreenDim) ]
							+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center) [ SNew(STextBlock).Text(FText::FromString(TEXT("1420.4"))).Font(S47Style::Mono(18.f)).ColorAndOpacity(S47Style::GreenDim) ]
							+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right) [ SNew(STextBlock).Text(FText::FromString(TEXT("1420.7 MHz"))).Font(S47Style::Mono(18.f)).ColorAndOpacity(S47Style::GreenDim) ]
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(FMargin(14, 10))
								.BorderBackgroundColor_Lambda([this] { return FSlateColor(Story && Story->Rx.Stage < 3 && !Story->SurveyOnly() && Story->Rx.Profile().bCardIsTaped ? FLinearColor(0.75f, 0.68f, 0.5f) : FLinearColor(0.02f, 0.08f, 0.04f)); })
							[
								// (.refcard: read as the receiver's description, UI.open's descriptionReadings)
								S47Kit::Readable(SNew(STextBlock).AutoWrapText(true).Font(S47Style::Mono(22.f))
									.Text_Lambda([this]
									{
										if (!Story) return FText::GetEmpty();
										if (Story->Rx.Stage >= 3) return FText::FromString(TEXT("Pattern repeats every 5.2 s: four pulses, a gap, seven pulses. Run a direction solve on the tracking terminal."));
										if (Story->SurveyOnly()) return FText::FromString(TEXT("Survey runs unattended. Nothing scheduled until 06:00."));
										return FText::FromString(Story->Rx.Profile().Card);
									})
									.ColorAndOpacity_Lambda([this] { return FSlateColor(Story && Story->Rx.Stage < 3 && !Story->SurveyOnly() && Story->Rx.Profile().bCardIsTaped ? S47Style::Ink : S47Style::Green); }))
							]
						]
					]
					+ SHorizontalBox::Slot().FillWidth(0.38f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight() [ Control(TEXT("FREQUENCY MHz"), [Rx] { return Rx ? Rx->Frequency : 0.0; }, [Rx](double V) { if (Rx) Rx->Frequency = V; }, S47Signal::FMin, S47Signal::FMax, [](double V) { return FString::Printf(TEXT("%.3f"), V); }, true) ]
						+ SVerticalBox::Slot().AutoHeight() [ Control(TEXT("GAIN"), [Rx] { return Rx ? Rx->Gain : 0.0; }, [Rx](double V) { if (Rx) Rx->Gain = V; }, 0, 100, [](double V) { return FString::Printf(TEXT("%.0f"), V); }, false) ]
						+ SVerticalBox::Slot().AutoHeight() [ Control(TEXT("BANDWIDTH kHz"), [Rx] { return Rx ? Rx->Bandwidth : 0.0; }, [Rx](double V) { if (Rx) Rx->Bandwidth = V; }, 4, 100, [](double V) { return FString::Printf(TEXT("%.0f"), V); }, false) ]
						+ SVerticalBox::Slot().AutoHeight() [ Control(TEXT("ARRAY AZ"), [Rx] { return Rx ? Rx->Azimuth : 0.0; }, [Rx, Az](double V) { if (Rx) { Rx->Azimuth = V; Az(); } }, 0, 180, [](double V) { return S47Text::Pad(V, 3); }, false) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 24, 0, 0)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SBox).IsEnabled_Lambda([this] { return Story && Story->Rx.Stage < 3 && !Story->SurveyOnly(); })
								[
									SNew(SButton).ContentPadding(FMargin(16, 8)).OnClicked_Lambda([this] { if (OnClick) OnClick(); if (Story) Story->ConsoleAction(); return FReply::Handled(); })
									[
										SNew(STextBlock).Font(S47Style::Mono(24.f)).ColorAndOpacity(S47Style::Ink)
											.Text_Lambda([this] { return FText::FromString(!Story ? TEXT("") : Story->Rx.Stage >= 3 ? TEXT("LOCKED") : Story->SurveyOnly() ? TEXT("SWEEP RUNNING") : Story->Rx.Profile().Action); })
									]
								]
							]
							+ SHorizontalBox::Slot().FillWidth(1.f) [ SNew(SSpacer) ]
							+ SHorizontalBox::Slot().AutoWidth() [ S47Style::Button(TEXT("EXIT"), [this] { if (OnClick) OnClick(); if (OnClose) OnClose(); }, S47Style::Green) ]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 0)
						[
							SNew(STextBlock).Font(S47Style::Mono(20.f)).ColorAndOpacity(S47Style::GreenDim).AutoWrapText(true)
								.Text_Lambda([this] { return FText::FromString(Story && Story->Rx.Stage >= 3 ? TEXT("SOURCE: UNKNOWN   1420.405 MHz\nS/N 4.71   DIRECTION: NOT SOLVED") : TEXT("")); })
						]
					]
				]
			]
		]
	];
	// the receiver's card can be read as its description (receiver-description), the box under it in .lcol
	if (US47UI* UI = US47Main::Get(nullptr)->UI; UI && LeftCol.IsValid())
		LeftCol->AddSlot().AutoHeight()[UI->DescriptionReadings(LeftCol.ToSharedRef(), TEXT("receiver-description"))];
}

FReply SS47Console::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (OnClose) OnClose();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ---------- the operations terminal ----------

void SS47Ops::Construct(const FArguments& InArgs)
{
	Story = InArgs._Story;
	OnClose = InArgs._OnClose;
	PanelBrush = MakeUnique<FSlateRoundedBoxBrush>(FLinearColor(0.01f, 0.035f, 0.02f), FVector4(6.f, 6.f, 6.f, 6.f), FLinearColor(0.3f, 0.6f, 0.4f, 0.5f), 1.f);
	Text = (Story ? Story->Ops().Header() : FString()) + TEXT("\n");
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot() [ SNew(SImage).ColorAndOpacity(S47Style::Overlay) ]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(1100.f).HeightOverride(760.f)
			[
				SNew(SBorder).BorderImage(PanelBrush.Get()).Padding(FMargin(24, 18))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SAssignNew(Scroll, SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(Out, STextBlock).Text_Lambda([this] { return FText::FromString(Text); }).Font(S47Style::Mono(24.f)).ColorAndOpacity(S47Style::Green)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ SNew(STextBlock).Text(FText::FromString(TEXT("$ "))).Font(S47Style::Mono(24.f)).ColorAndOpacity(S47Style::Green) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SAssignNew(Input, SEditableTextBox).Font(S47Style::Mono(24.f)).ForegroundColor(S47Style::Green).BackgroundColor(FLinearColor(0.01f, 0.05f, 0.02f))
								.OnTextCommitted_Lambda([this](const FText& T, ETextCommit::Type C) { if (C == ETextCommit::OnEnter) { Exec(T.ToString()); if (Input.IsValid()) Input->SetText(FText::GetEmpty()); } })
						]
					]
					+ SVerticalBox::Slot().AutoHeight() [ SAssignNew(Buttons, SWrapBox).UseAllottedSize(true) ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, 10, 0, 0) [ S47Style::Button(TEXT("LOG OUT   [ESC]"), [this] { if (OnClose) OnClose(); }, S47Style::Green) ]
				]
			]
		]
	];
	RebuildButtons();
}

void SS47Ops::RebuildButtons()
{
	if (!Buttons.IsValid() || !Story) return;
	Buttons->ClearChildren();
	for (const FString& B : Story->Ops().Buttons())
	{
		const FString Cmd = B;
		Buttons->AddSlot().Padding(3)
		[
			S47Style::Button(B, [this, Cmd] { Exec(Cmd); }, S47Style::Green, 20.f)
		];
	}
}

void SS47Ops::Exec(const FString& Cmd)
{
	const FString C = Cmd.TrimStartAndEnd();
	if (C.IsEmpty()) return;
	const FString U = C.ToUpper();
	if (U == TEXT("LOGOUT") || U == TEXT("EXIT") || U == TEXT("LO")) { if (OnClose) OnClose(); return; }
	if (!Story) return;
	const FString Res = Story->Ops().Run(C);
	Text += TEXT("$ ") + U + TEXT("\n") + (Res.IsEmpty() ? FString() : Res + TEXT("\n"));
	TArray<FString> Lines;
	Text.ParseIntoArray(Lines, TEXT("\n"), false);
	if (Lines.Num() > 400) { Lines.RemoveAt(0, Lines.Num() - 400); Text = FString::Join(Lines, TEXT("\n")); }
	Story->OpsRan(C, Res);
	RebuildButtons();
	if (Scroll.IsValid()) Scroll->ScrollToEnd();
}

FReply SS47Ops::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (OnClose) OnClose();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ---------- the end card ----------

void SS47EndCard::Construct(const FArguments& InArgs)
{
	TSharedRef<SVerticalBox> Lines = SNew(SVerticalBox);
	for (const FString& L : InArgs._Lines)
	{
		Lines->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4)
		[
			SNew(STextBlock).Text(FText::FromString(L)).Font(S47Style::Mono(L.StartsWith(TEXT("PROLOGUE")) ? 44.f : 24.f)).ColorAndOpacity(S47Style::Paper).Justification(ETextJustify::Center)
		];
	}
	TFunction<void()> Again = InArgs._OnAgain, Quit = InArgs._OnQuit;
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot() [ SNew(SImage).ColorAndOpacity(FLinearColor(0.01f, 0.01f, 0.012f)) ]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight() [ Lines ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 40, 0, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0) [ S47Style::Button(TEXT("PLAY THE NIGHT AGAIN"), Again) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0) [ S47Style::Button(TEXT("QUIT"), Quit) ]
			]
		]
	];
}
