#include "SS47Widgets.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/CompositeFont.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
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
			// The fallback when the game font is not there: the mono font from the other project.
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
}

void SS47Hud::Construct(const FArguments& InArgs)
{
	DotBrush = MakeUnique<FSlateRoundedBoxBrush>(FLinearColor::White, FVector4(3.f, 3.f, 3.f, 3.f));
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(5.f).HeightOverride(5.f)
			[
				SNew(SBorder).BorderImage(DotBrush.Get()).BorderBackgroundColor(FLinearColor(1, 1, 1, 0.6f))
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, 70)
		[
			SNew(STextBlock).Text(InArgs._Prompt).Font(S47Style::Mono(26.f)).ColorAndOpacity(S47Style::Paper)
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0, 24, 32, 0)
		[
			SNew(STextBlock).Text(InArgs._Clock).Font(S47Style::Mono(30.f)).ColorAndOpacity(FLinearColor(0.93f, 0.9f, 0.84f, 0.55f))
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, 28)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("W A S D  MOVE   MOUSE  LOOK   E  USE"))).Font(S47Style::Mono(18.f)).ColorAndOpacity(FLinearColor(0.93f, 0.9f, 0.84f, 0.35f))
		]
	];
}

void SS47Terminal::Construct(const FArguments& InArgs)
{
	OnClose = InArgs._OnClose;
	WhiteBrush = MakeUnique<FSlateColorBrush>(FLinearColor::White);
	PanelBrush = MakeUnique<FSlateRoundedBoxBrush>(FLinearColor(0.02f, 0.025f, 0.02f), FVector4(6.f, 6.f, 6.f, 6.f), FLinearColor(0.25f, 0.2f, 0.1f), 1.f);
	const FSlateFontInfo F = S47Style::Mono(34.f);
	const FSlateFontInfo Small = S47Style::Mono(24.f);
	auto Line = [&](const TCHAR* Left, TAttribute<FText> Right)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(STextBlock).Text(FText::FromString(Left)).Font(F).ColorAndOpacity(S47Style::Amber)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock).Text(Right).Font(F).ColorAndOpacity(S47Style::Amber)
			];
	};
	auto Fixed = [&](const TCHAR* Left, const TCHAR* Right) { return Line(Left, FText::FromString(Right)); };
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SImage).ColorAndOpacity(FLinearColor(0.01f, 0.012f, 0.01f))
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(900.f)
			[
				SNew(SBorder).BorderImage(PanelBrush.Get()).Padding(FMargin(48, 36))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 24) [ Line(TEXT("SARO ARRAY CONTROL"), InArgs._Clock) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6) [ Fixed(TEXT("SURVEY SWEEP"), TEXT("RUN 0113")) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6) [ Fixed(TEXT("RECEIVER"), TEXT("1420.405 MHz   BW 2.4 kHz")) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6) [ Fixed(TEXT("ARRAY"), TEXT("AZ 026.0   EL 032.0")) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6) [ Fixed(TEXT("DISHES SLAVED"), TEXT("27 / 27")) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6) [ Fixed(TEXT("RELAY K3"), TEXT("CLOSED")) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 24) [ Fixed(TEXT("SIGNAL"), TEXT("NONE")) ]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("ESC  LEAVE TERMINAL"))).Font(Small).ColorAndOpacity(FLinearColor(1.0f, 0.68f, 0.22f, 0.5f))
					]
				]
			]
		]
	];
}

FReply SS47Terminal::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::E)
	{
		if (OnClose) OnClose();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

int32 SS47Terminal::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	int32 Top = SCompoundWidget::OnPaint(Args, G, Cull, Out, Layer, Style, bParentEnabled);
	// Scanlines over the whole screen, faint.
	const FVector2f S = FVector2f(G.GetLocalSize());
	for (float Y = 0; Y < S.Y; Y += 4.f)
	{
		FSlateDrawElement::MakeBox(Out, Top + 1, G.ToPaintGeometry(FVector2f(S.X, 1.f), FSlateLayoutTransform(FVector2f(0, Y))), WhiteBrush.Get(), ESlateDrawEffect::None, FLinearColor(0, 0, 0, 0.18f));
	}
	return Top + 1;
}
