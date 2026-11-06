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
#include "Widgets/SNullWidget.h"
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

	// the knobs' range inputs: the core slider with a track thick enough to see on the dark panel
	const FSliderStyle& KnobStyle()
	{
		static FSliderStyle S = []
		{
			// (the track a plain bar the bar colour tints, the thumb a round knob the handle colour tints)
			FSliderStyle X = FCoreStyle::Get().GetWidgetStyle<FSliderStyle>("Slider");
			const FSlateColorBrush Bar(FLinearColor::White);
			const FSlateRoundedBoxBrush Thumb(FLinearColor::White, 8.f * PX, FVector2f(16.f * PX, 16.f * PX));
			X.SetNormalBarImage(Bar).SetHoveredBarImage(Bar).SetDisabledBarImage(Bar);
			X.SetNormalThumbImage(Thumb).SetHoveredThumbImage(Thumb).SetDisabledThumbImage(Thumb);
			X.SetBarThickness(4.f * PX);
			return X;
		}();
		return S;
	}
}

S47Kit::FW S47Kit::Shell(US47UI* UI, const FString& Title, FW Body, float Width, bool bLow)
{
	TWeakObjectPtr<US47UI> U = UI;
	FSlateFontInfo H3 = Term(28.f);
	H3.LetterSpacing = 60;
	return SNew(SBorder)
		// .overlay: the dark over the game; a click on it closes the panel
		.BorderImage(Box(Css(0x020306, 0.72f), FLinearColor::Transparent, 0.f))
		.HAlign(HAlign_Center).VAlign(bLow ? VAlign_Bottom : VAlign_Center).Padding(bLow ? FMargin(12.f, 12.f, 12.f, 72.f * PX) : FMargin(12.f))
		.OnMouseButtonDown_Lambda([U](const FGeometry&, const FPointerEvent&) { if (U.IsValid()) U->Close(); return FReply::Handled(); })
		[
			SNew(SBox).WidthOverride(Width * PX).MaxDesiredHeight(1040.f)
			[
				SNew(SBorder)
				.BorderImage(Box(Css(0x1b1e14, 0.97f), Css(0xd9d0b4, 0.55f)))
				.Padding(FMargin(22 * PX, 18 * PX, 22 * PX, 20 * PX))
				.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10 * PX)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(FText::FromString(Title.ToUpper())).Font(H3).ColorAndOpacity(Css(0xf1e8cf))
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
						// (UI.open: a lab panel's descriptions can be read, the box at its end: descriptionReadings)
						SNew(SScrollBox) + SScrollBox::Slot()[Body] + SScrollBox::Slot()[UI ? UI->DescriptionReadings(Body) : SNullWidget::NullWidget]
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
	return Readable(SNew(STextBlock)
		.Text_Lambda([S] { return FText::FromString(S.Get()); })
		.Font(Type(15.5f)).AutoWrapText(true).LineHeightPercentage(1.15f)
		.ColorAndOpacity_Lambda([Tone] { const int32 T = Tone.Get(); return FSlateColor(T > 0 ? Css(0xb9ffc7) : T < 0 ? Css(0xffb3a0) : Css(0xddd5c2)); })
		.Margin(FMargin(0, 10, 0, 10)));
}

S47Kit::FW S47Kit::Status(const FString& Name, const FString& State, int32 Tone)
{
	// One row: name left, state in its own column, so a list can be read down the right edge.
	const FLinearColor C = Tone > 0 ? Css(0xa4d890) : Tone < 0 ? Css(0xff8a70) : Css(0xcfc8b5);
	return SNew(SBorder).BorderImage(Box(FLinearColor::Transparent, Css(0xd9d0b4, 0.16f))).Padding(FMargin(12, 6))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Name)).Font(Term(21.f)).ColorAndOpacity(Css(0xf1e8cf))]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14, 0, 0, 0)[SNew(STextBlock).Text(FText::FromString(State)).Font(Term(21.f)).ColorAndOpacity(C)]
		];
}

S47Kit::FW S47Kit::Sub(const FString& S)
{
	return SNew(STextBlock).Text(FText::FromString(S)).Font(Term(21.f)).ColorAndOpacity(Css(0xffb052)).AutoWrapText(true).Margin(FMargin(0, 0, 0, 10));
}

S47Kit::FW S47Kit::Small(const FString& S)
{
	return Readable(SNew(STextBlock).Text(FText::FromString(S)).Font(Type(12.5f)).ColorAndOpacity(Css(0x8e877a)).AutoWrapText(true).Margin(FMargin(0, 12, 0, 0)));
}

S47Kit::FW S47Kit::Question(const FString& S)
{
	return Readable(SNew(STextBlock).Text(FText::FromString(S)).Font(Term(20.f)).ColorAndOpacity(Css(0xf0e7d0)).AutoWrapText(true).Margin(FMargin(0, 8, 0, 6)));
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
		Ps->AddSlot().AutoHeight().Padding(0, 0, 0, 10)[Readable(SNew(STextBlock).Text(FText::FromString(Paras[i])).Font(F).ColorAndOpacity(Css(0x26262a)).AutoWrapText(true))];
	}
	TSharedRef<SWidget> W = SNew(SBorder).BorderImage(Box(Css(0xebe3cf), FLinearColor::Transparent, 0.f)).Padding(FMargin(22, 18))[Ps];
	W->SetRenderTransform(FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(-0.4f))));
	W->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	return W;
}

S47Kit::FW S47Kit::Steps(const TArray<TPair<FString, FString>>& Items, int32 Active)
{
	// Numbered rows with their state on the right; the current one is the light strip (Tom, wet bench concept 6 November).
	TSharedRef<SVerticalBox> L = SNew(SVerticalBox);
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const bool bNow = i == Active, bDone = i < Active;
		FString Title = Items[i].Key, Left, Right;
		if (Title.Split(TEXT(" / "), &Left, &Right) && Left.IsNumeric()) Title = Right;
		const FLinearColor Ink = bNow ? Css(0x1a160f) : bDone ? Css(0x8b9284) : Css(0xe9e2cf);
		const FLinearColor Line = bNow ? Css(0x3d3626) : bDone ? Css(0x7f8a78) : Css(0xc4bfae);
		const FString State = bNow ? TEXT("CURRENT STEP") : bDone ? TEXT("DONE") : i == Active + 1 ? TEXT("NEXT") : TEXT("THEN");
		L->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
		[
			SNew(SBorder)
			.BorderImage(bNow ? Box(Css(0xeeddb8), Css(0xeeddb8)) : Box(FLinearColor::Transparent, Css(0x6d7562, bDone ? 0.45f : 0.9f)))
			.Padding(FMargin(12, 9))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 14, 0)
				[
					SNew(SBorder).BorderImage(Box(bNow ? Css(0xe5a33f) : Css(0x3d4a37), bNow ? Css(0xe5a33f) : Css(0x3d4a37))).Padding(FMargin(9, 3))
					[
						SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%02d"), i + 1))).Font(Term(22.f)).ColorAndOpacity(bNow ? Css(0x1a160f) : Css(0xd6d9c4))
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Title)).Font(Term(22.f)).ColorAndOpacity(Ink)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)[Readable(SNew(STextBlock).Text(FText::FromString(Items[i].Value)).Font(Type(15.f)).ColorAndOpacity(Line).AutoWrapText(true))]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14, 0, 0, 0)
				[
					SNew(SBorder).BorderImage(bNow ? Box(Css(0xe5a33f), Css(0xe5a33f)) : Box(FLinearColor::Transparent, FLinearColor::Transparent)).Padding(FMargin(9, 3))
					[
						SNew(STextBlock).Text(FText::FromString(State)).Font(Term(18.f)).ColorAndOpacity(bNow ? Css(0x1a160f) : bDone ? Css(0x8b9284) : Css(0xc4bfae))
					]
				]
			]
		];
	}
	return SNew(SBox).Padding(FMargin(0, 0, 0, 10))[L];
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

S47Kit::FW S47Kit::Action(const FString& Label, TFunction<void()> OnClick)
{
	return SNew(SButton)
		.ButtonStyle([]() -> const FButtonStyle*
		{
			static const FButtonStyle Style = []
			{
				FButtonStyle B = *ButtonStyle(Css(0xffb052), 0.05f, 0.14f);
				return B.SetNormal(*Box(Css(0xe5a33f), Css(0xf0b040))).SetHovered(*Box(Css(0xf0b858), Css(0xf0b040))).SetPressed(*Box(Css(0xcc8f30), Css(0xf0b040)));
			}();
			return &Style;
		}())
		.ContentPadding(FMargin(14, 8)).HAlign(HAlign_Center)
		.OnClicked_Lambda([OnClick] { if (OnClick) OnClick(); return FReply::Handled(); })
		[
			SNew(STextBlock).Text(FText::FromString(Label)).Font(Term(24.f)).ColorAndOpacity(Css(0x0c0a09))
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
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)[Readable(SNew(STextBlock).Text(FText::FromString(Line)).Font(Type(14.5f)).ColorAndOpacity(Css(0xd6cebb)).AutoWrapText(true))]
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

S47Kit::FW S47Kit::Knob(const FString& Label, TAttribute<FString> Shown, float Min, float Max, float Step, TAttribute<float> Value, TFunction<void(float)> OnChange)
{
	// .dsp-knobs label { display: grid; gap: 6px }: the name, its value (span, amber), the range (min-height 32px)
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Text(FText::FromString(Label)).Font(Term(19.f)).ColorAndOpacity(Css(0xf0e7d0))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6 * PX, 0, 0)
		[
			SNew(STextBlock).Text_Lambda([Shown] { return FText::FromString(Shown.Get()); }).Font(Term(19.f)).ColorAndOpacity(Css(0xffb052))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6 * PX, 0, 0)
		[
			SNew(SBox).MinDesiredHeight(32 * PX).VAlign(VAlign_Center)
			[
				// (a range input's track, as wide as the column, its thumb in accent-color #c9d6c2)
				SNew(SSlider).Style(&KnobStyle()).MinValue(Min).MaxValue(Max).StepSize(Step).MouseUsesStep(true)
				.Value(Value).SliderBarColor(Css(0xcfc8b5, 0.55f)).SliderHandleColor(Css(0xc9d6c2))
				.OnValueChanged_Lambda([OnChange](float V) { if (OnChange) OnChange(V); })
			]
		];
}

S47Kit::FW S47Kit::ButtonLive(TAttribute<FString> Label, TFunction<void()> OnClick, bool bPrimary, TAttribute<bool> Enabled)
{
	return SNew(SButton)
		.ButtonStyle(ButtonStyle(bPrimary ? Css(0xffb052) : Css(0xffd6aa, 0.45f), 0.05f, 0.14f))
		.ContentPadding(FMargin(14, 6)).IsEnabled(Enabled)
		.OnClicked_Lambda([OnClick] { if (OnClick) OnClick(); return FReply::Handled(); })
		[
			SNew(SBox).MinDesiredHeight(44.f - 12.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text_Lambda([Label] { return FText::FromString(Label.Get()); }).Font(Term(bPrimary ? 22.f : 20.f)).ColorAndOpacity(bPrimary ? Css(0xffb052) : Css(0xf1e8d2))
			]
		];
}

S47Kit::FW S47Kit::Choice(const FString& Label, TAttribute<bool> Pressed, TFunction<void()> OnClick, bool bAuto, const FString& Right)
{
	// .lp-choice: padding 8px 10px, min-height 48px, a faint line; [aria-pressed=true] filled pale green, dark text
	static FButtonStyle Bare = []
	{
		FButtonStyle B;
		FSlateBrush None;
		None.DrawAs = ESlateBrushDrawType::NoDrawType;
		B.SetNormal(None).SetHovered(None).SetPressed(None).SetDisabled(None);
		B.SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
		return B;
	}();
	const FW Face = SNew(SBorder)
		.BorderImage_Lambda([Pressed] { return Pressed.Get() ? Box(Css(0xc9d6c2), Css(0xc9d6c2)) : Box(FLinearColor::Transparent, Css(0xe9e2cf, 0.25f)); })
		.Padding(FMargin(10 * PX, 8 * PX))
		[
			SNew(SBox).MinDesiredHeight(48 * PX - 16 * PX).VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(Label)).Font(Term(19.f))
					.ColorAndOpacity_Lambda([Pressed] { return FSlateColor(Pressed.Get() ? Css(0x12100e) : Css(0xe9e2cf)); })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14 * PX, 0, 0, 0)
				[
					SNew(STextBlock).Text(FText::FromString(Right)).Font(Term(19.f)).Visibility(Right.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible)
					.ColorAndOpacity_Lambda([Pressed] { return FSlateColor(Pressed.Get() ? Css(0x12100e) : Css(0xc4bfae)); })
				]
			]
		];
	return SNew(SBox).Padding(FMargin(0, bAuto ? 0.f : 6 * PX, 0, 0)).HAlign(bAuto ? HAlign_Left : HAlign_Fill)
		[
			SNew(SButton).ButtonStyle(&Bare).OnClicked_Lambda([OnClick] { if (OnClick) OnClick(); return FReply::Handled(); })[Face]
		];
}

S47Kit::FW S47Kit::Readable(FW W) { W->AddMetadata(MakeShared<FS47ReadingMark>(true)); return W; }
S47Kit::FW S47Kit::Unread(FW W) { W->AddMetadata(MakeShared<FS47ReadingMark>(false)); return W; }

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
