#include "S47Kit.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateImageBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Texture2D.h"
#include "S47UI.h"
#include "SS47Widgets.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

// Sizes: the web version's CSS pixels times this, as the HUD's text (a 1080p screen reads them
// at about the size the browser draws them at its default zoom).
static constexpr float PX = 1.25f;

FLinearColor S47Kit::Css(uint32 Rgb, float A)
{
	FLinearColor C = FLinearColor::FromSRGBColor(FColor((Rgb >> 16) & 255, (Rgb >> 8) & 255, Rgb & 255));
	C.A = A;
	return C;
}

namespace
{
	using namespace S47Kit;

	const FSlateBrush* Box(const FLinearColor& Fill, const FLinearColor& Line, float LineW = 1.f)
	{
		// brushes live for the session; a handful of colours in all
		static TMap<FString, TSharedPtr<FSlateBrush>> Cache;
		const FString Key = Fill.ToString() + Line.ToString() + FString::SanitizeFloat(LineW);
		TSharedPtr<FSlateBrush>& B = Cache.FindOrAdd(Key);
		if (!B.IsValid()) B = MakeShared<FSlateRoundedBoxBrush>(Fill, 0.f, Line, LineW);
		return B.Get();
	}

	const FButtonStyle* ButtonStyle(const FLinearColor& Line, float Fill, float FillHover)
	{
		static TMap<FString, TSharedPtr<FButtonStyle>> Cache;
		const FString Key = Line.ToString() + FString::SanitizeFloat(Fill);
		TSharedPtr<FButtonStyle>& S = Cache.FindOrAdd(Key);
		if (!S.IsValid())
		{
			S = MakeShared<FButtonStyle>();
			const FLinearColor F = Css(0xffc896, Fill), H = Css(0xffc896, FillHover);
			S->SetNormal(*Box(F, Line)).SetHovered(*Box(H, Line)).SetPressed(*Box(H, Line)).SetDisabled(*Box(Css(0x808080, 0.03f), Line * FLinearColor(1, 1, 1, 0.4f)));
			S->SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
		}
		return S.Get();
	}

	FSlateFontInfo Term(float Px) { return S47Style::Mono(Px * PX); }
	FSlateFontInfo Type(float Px) { return S47Style::Body(Px * PX); }
}

S47Kit::FW S47Kit::Shell(US47UI* UI, const FString& Title, FW Body, float Width)
{
	TWeakObjectPtr<US47UI> U = UI;
	FSlateFontInfo H3 = Term(26.f);
	H3.LetterSpacing = 320;
	return SNew(SBorder)
		// .overlay: the dark over the game; a click on it closes the panel
		.BorderImage(Box(Css(0x020306, 0.72f), FLinearColor::Transparent, 0.f))
		.HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(12.f)
		.OnMouseButtonDown_Lambda([U](const FGeometry&, const FPointerEvent&) { if (U.IsValid()) U->Close(); return FReply::Handled(); })
		[
			SNew(SBox).WidthOverride(Width * PX).MaxDesiredHeight(1040.f)
			[
				SNew(SBorder)
				.BorderImage(Box(Css(0x0c0a09, 0.96f), Css(0xff8c6e, 0.28f)))
				.Padding(FMargin(22 * PX, 18 * PX, 22 * PX, 20 * PX))
				.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10 * PX)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(FText::FromString(Title.ToUpper())).Font(H3).ColorAndOpacity(Css(0xffb7a4))
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton).ButtonStyle(ButtonStyle(FLinearColor::Transparent, 0.f, 0.08f)).ContentPadding(FMargin(10, 4))
							.OnClicked_Lambda([U] { if (U.IsValid()) U->Close(); return FReply::Handled(); })
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Close"))).Font(Term(22.f)).ColorAndOpacity(Css(0xcfc8b5))
							]
						]
					]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SNew(SScrollBox) + SScrollBox::Slot()[Body]
					]
				]
			]
		];
}

S47Kit::FW S47Kit::Text(const FString& S, int32 Tone)
{
	return Text(TAttribute<FString>(S), TAttribute<int32>(Tone));
}

S47Kit::FW S47Kit::Text(TAttribute<FString> S, TAttribute<int32> Tone)
{
	return SNew(STextBlock)
		.Text_Lambda([S] { return FText::FromString(S.Get()); })
		.Font(Type(15.5f)).AutoWrapText(true).LineHeightPercentage(1.15f)
		.ColorAndOpacity_Lambda([Tone] { const int32 T = Tone.Get(); return FSlateColor(T > 0 ? Css(0xb9ffc7) : T < 0 ? Css(0xffb3a0) : Css(0xddd5c2)); })
		.Margin(FMargin(0, 10, 0, 10));
}

S47Kit::FW S47Kit::Sub(const FString& S)
{
	return SNew(STextBlock).Text(FText::FromString(S)).Font(Term(19.f)).ColorAndOpacity(Css(0xffb052)).AutoWrapText(true).Margin(FMargin(0, 0, 0, 10));
}

S47Kit::FW S47Kit::Small(const FString& S)
{
	return SNew(STextBlock).Text(FText::FromString(S)).Font(Type(12.5f)).ColorAndOpacity(Css(0x8e877a)).AutoWrapText(true).Margin(FMargin(0, 12, 0, 0));
}

S47Kit::FW S47Kit::Question(const FString& S)
{
	return SNew(STextBlock).Text(FText::FromString(S)).Font(Term(20.f)).ColorAndOpacity(Css(0xf0e7d0)).AutoWrapText(true).Margin(FMargin(0, 8, 0, 6));
}

S47Kit::FW S47Kit::Paper(const FString& S, bool bReport)
{
	TSharedRef<SVerticalBox> Ps = SNew(SVerticalBox);
	TArray<FString> Paras;
	S.ParseIntoArray(Paras, TEXT("\n\n"), false);
	for (int32 i = 0; i < Paras.Num(); ++i)
	{
		FSlateFontInfo F = Type(bReport && i == 0 ? 17.f : 14.5f);
		if (bReport && i == 0) F.LetterSpacing = 40;
		Ps->AddSlot().AutoHeight().Padding(0, 0, 0, 10)[SNew(STextBlock).Text(FText::FromString(Paras[i])).Font(F).ColorAndOpacity(Css(0x26262a)).AutoWrapText(true)];
	}
	TSharedRef<SWidget> W = SNew(SBorder).BorderImage(Box(Css(0xebe3cf), FLinearColor::Transparent, 0.f)).Padding(FMargin(22, 18))[Ps];
	W->SetRenderTransform(FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(-0.4f))));
	W->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	return W;
}

S47Kit::FW S47Kit::Steps(const TArray<TPair<FString, FString>>& Items, int32 Active)
{
	TSharedRef<SVerticalBox> L = SNew(SVerticalBox);
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const bool bNow = i == Active, bDone = i < Active;
		FTextBlockStyle Head = FTextBlockStyle().SetFont(Term(20.f)).SetColorAndOpacity(bNow ? Css(0xffcf9f) : bDone ? Css(0x7f8a78) : Css(0xbdb5a3));
		if (bDone) Head.SetStrikeBrush(*FCoreStyle::Get().GetBrush("WhiteBrush"));
		static TArray<TSharedPtr<FTextBlockStyle>> Keep; // the style must outlive the widget
		Keep.Add(MakeShared<FTextBlockStyle>(Head));
		L->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
		[
			SNew(SBorder)
			.BorderImage(bNow ? Box(Css(0xff3c1e, 0.06f), Css(0xff785a, 0.6f)) : Box(FLinearColor::Transparent, FLinearColor(1, 1, 1, 0.1f)))
			.Padding(FMargin(12, 8))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Items[i].Key)).TextStyle(Keep.Last().Get())]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)[SNew(STextBlock).Text(FText::FromString(Items[i].Value)).Font(Type(14.f)).ColorAndOpacity(Css(0x9b9483)).AutoWrapText(true)]
			]
		];
	}
	return SNew(SBox).Padding(FMargin(0, 0, 0, 14))[L];
}

S47Kit::FW S47Kit::Button(const FString& Label, TFunction<void()> OnClick, bool bPrimary, TAttribute<bool> Enabled)
{
	return SNew(SButton)
		.ButtonStyle(ButtonStyle(bPrimary ? Css(0xffb052) : Css(0xffd6aa, 0.45f), 0.05f, 0.14f))
		.ContentPadding(FMargin(14, 6)).IsEnabled(Enabled)
		.OnClicked_Lambda([OnClick] { if (OnClick) OnClick(); return FReply::Handled(); })
		[
			SNew(SBox).MinDesiredHeight(44.f - 12.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(Label)).Font(Term(bPrimary ? 22.f : 20.f)).ColorAndOpacity(bPrimary ? Css(0xffb052) : Css(0xf1e8d2))
			]
		];
}

S47Kit::FW S47Kit::Card(const FString& Title, const FString& Line, TFunction<void()> OnClick, TAttribute<bool> Enabled)
{
	return SNew(SButton)
		.ButtonStyle(ButtonStyle(Css(0xffd6aa, 0.4f), 0.04f, 0.12f))
		.ContentPadding(FMargin(16, 14)).IsEnabled(Enabled)
		.OnClicked_Lambda([OnClick] { if (OnClick) OnClick(); return FReply::Handled(); })
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Title)).Font(Term(22.f)).ColorAndOpacity(Css(0xffb052))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)[SNew(STextBlock).Text(FText::FromString(Line)).Font(Type(14.5f)).ColorAndOpacity(Css(0xd6cebb)).AutoWrapText(true)]
		];
}

S47Kit::FW S47Kit::Row(const TArray<FW>& Children)
{
	TSharedRef<SWrapBox> W = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(10, 10));
	for (const FW& C : Children) W->AddSlot()[C];
	return SNew(SBox).Padding(FMargin(0, 6))[W];
}

S47Kit::FW S47Kit::Col(const TArray<FW>& Children, float Gap)
{
	TSharedRef<SVerticalBox> V = SNew(SVerticalBox);
	for (int32 i = 0; i < Children.Num(); ++i) V->AddSlot().AutoHeight().Padding(0, i ? Gap : 0, 0, 0)[Children[i]];
	return V;
}

S47Kit::FW S47Kit::Two(FW Left, FW Right, float LeftShare, float RightShare)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(LeftShare).Padding(0, 0, 10, 0)[Left]
		+ SHorizontalBox::Slot().FillWidth(RightShare).Padding(10, 0, 0, 0)[Right];
}

S47Kit::FW S47Kit::Cards(const TArray<FW>& Children)
{
	TSharedRef<SUniformGridPanel> G = SNew(SUniformGridPanel).SlotPadding(FMargin(7));
	for (int32 i = 0; i < Children.Num(); ++i) G->AddSlot(i % 2, i / 2)[Children[i]];
	return G;
}

S47Kit::FW S47Kit::Slider(const FString& Label, float Min, float Max, float Step, TAttribute<float> Value, TFunction<void(float)> OnChange)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(70 * PX)[SNew(STextBlock).Text(FText::FromString(Label)).Font(Term(18.f)).ColorAndOpacity(Css(0xcfc8b5))]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(8, 0, 0, 0)
		[
			SNew(SSlider).MinValue(Min).MaxValue(Max).StepSize(Step).MouseUsesStep(true)
			.Value(Value).SliderBarColor(Css(0xa8641f)).SliderHandleColor(Css(0xffb052))
			.OnValueChanged_Lambda([OnChange](float V) { if (OnChange) OnChange(V); })
		];
}

TSharedPtr<FSlateBrush> S47Kit::BrushFor(UTexture2D* T)
{
	if (!T) return nullptr;
	static TMap<TWeakObjectPtr<UTexture2D>, TSharedPtr<FSlateBrush>> Cache;
	TSharedPtr<FSlateBrush>& B = Cache.FindOrAdd(T);
	if (!B.IsValid())
	{
		B = MakeShared<FSlateBrush>();
		B->SetResourceObject(T);
		B->ImageSize = FVector2D(T->GetSizeX(), T->GetSizeY());
		B->DrawAs = ESlateBrushDrawType::Image;
	}
	return B;
}

S47Kit::FW S47Kit::Image(UTexture2D* T, float Width)
{
	TSharedPtr<FSlateBrush> B = BrushFor(T);
	if (!B.IsValid()) return SNew(SBox);
	const float H = Width * B->ImageSize.Y / FMath::Max(1.0, B->ImageSize.X);
	return SNew(SBox).WidthOverride(Width).HeightOverride(H)[SNew(SImage).Image(B.Get())];
}
