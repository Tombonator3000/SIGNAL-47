#include "SS47Screens.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "S47Kit.h"
#include "S47Opening.h"
#include "S47Gamepad.h"
#include "SS47Widgets.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

using namespace S47Css;

// ---------- the CSS pieces ----------

FLinearColor S47Css::C(uint32 Rgb, float A) { return S47Kit::Css(Rgb, A); }
FSlateFontInfo S47Css::Term(float Px) { return S47Style::Mono(Px * PX); }
FSlateFontInfo S47Css::Type(float Px) { return S47Style::Body(Px * PX); }
FSlateFontInfo S47Css::Hand(float Px) { return S47Style::Hand(Px * PX); }
FSlateFontInfo S47Css::Display(float Px) { return S47Style::Display(Px * PX); }

const FSlateBrush* S47Css::Box(const FLinearColor& Fill, const FLinearColor& Line, float LineW, float Radius)
{
	// brushes live for the session; a few dozen colours in all
	static TMap<FString, TSharedPtr<FSlateBrush>> Cache;
	const FString Key = Fill.ToString() + Line.ToString() + FString::SanitizeFloat(LineW) + TEXT("/") + FString::SanitizeFloat(Radius);
	TSharedPtr<FSlateBrush>& B = Cache.FindOrAdd(Key);
	if (!B.IsValid()) B = MakeShared<FSlateRoundedBoxBrush>(Fill, Radius, Line, LineW);
	return B.Get();
}

const FButtonStyle* S47Css::Flat()
{
	static const FButtonStyle Style = []
	{
		FButtonStyle S;
		const FSlateNoResource None;
		S.SetNormal(None).SetHovered(None).SetPressed(None).SetDisabled(None);
		S.SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
		return S;
	}();
	return &Style;
}

TSharedRef<SButton> S47Css::TextButton(const FString& Label, const FSlateFontInfo& Font, const FLinearColor& Color, const FLinearColor& Hover,
	TFunction<void()> OnClick, TAttribute<bool> Enabled, const FLinearColor& Off, bool bArrow, const FMargin& Pad)
{
	TSharedRef<SButton> B = SNew(SButton).ButtonStyle(Flat()).ContentPadding(Pad).IsEnabled(Enabled).HAlign(HAlign_Left)
		.OnClicked_Lambda([OnClick] { if (OnClick) OnClick(); return FReply::Handled(); });
	TWeakPtr<SButton> W = B;
	auto Lit = [W] { const TSharedPtr<SButton> P = W.Pin(); return P.IsValid() && P->IsEnabled() && (P->IsHovered() || P->HasKeyboardFocus()); };
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	if (bArrow)
	{
		// .title-screen .menu button::before: '>' in phosphor, 1.1em wide, shown on hover and focus
		Row->AddSlot().AutoWidth()
		[
			SNew(SBox).WidthOverride(Font.Size / 0.75f * 1.1f)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Font(Font).ColorAndOpacity_Lambda([Lit] { return FSlateColor(C(0x8cffa4, Lit() ? 1.f : 0.f)); })
			]
		];
	}
	Row->AddSlot().AutoWidth()
	[
		SNew(STextBlock).Text(FText::FromString(Label)).Font(Font)
		.ColorAndOpacity_Lambda([W, Lit, Color, Hover, Off]
		{
			const TSharedPtr<SButton> P = W.Pin();
			if (P.IsValid() && !P->IsEnabled()) return FSlateColor(Off);
			return FSlateColor(Lit() ? Hover : Color);
		})
	];
	B->SetContent(Row);
	return B;
}

TSharedRef<SButton> S47Css::TextButton(TAttribute<FText> Label, const FSlateFontInfo& Font, const FLinearColor& Color, const FLinearColor& Hover, TFunction<void()> OnClick, const FMargin& Pad)
{
	TSharedRef<SButton> B = SNew(SButton).ButtonStyle(Flat()).ContentPadding(Pad).HAlign(HAlign_Left)
		.OnClicked_Lambda([OnClick] { if (OnClick) OnClick(); return FReply::Handled(); });
	TWeakPtr<SButton> W = B;
	B->SetContent(SNew(STextBlock).Text(Label).Font(Font)
		.ColorAndOpacity_Lambda([W, Color, Hover] { const TSharedPtr<SButton> P = W.Pin(); return FSlateColor(P.IsValid() && (P->IsHovered() || P->HasKeyboardFocus()) ? Hover : Color); }));
	return B;
}

TSharedRef<SButton> S47Css::BoxButton(const FString& Label, const FSlateFontInfo& Font, const FLinearColor& Text, const FLinearColor& Fill, const FLinearColor& Line, TFunction<void()> OnClick, const FMargin& Pad)
{
	static TMap<FString, TSharedPtr<FButtonStyle>> Styles;
	const FString Key = Fill.ToString() + Line.ToString();
	TSharedPtr<FButtonStyle>& S = Styles.FindOrAdd(Key);
	if (!S.IsValid())
	{
		S = MakeShared<FButtonStyle>();
		const FSlateBrush* N = Box(Fill, Line, Line.A > 0.f ? 1.f : 0.f, 2.f);
		const FSlateBrush* H = Box(Fill * FLinearColor(1.1f, 1.1f, 1.1f, 1.f) + FLinearColor(0.03f, 0.03f, 0.03f, Fill.A > 0.f ? 0.f : 0.06f), Line, Line.A > 0.f ? 1.f : 0.f, 2.f);
		S->SetNormal(*N).SetHovered(*H).SetPressed(*H).SetDisabled(*N);
		S->SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
	}
	return SNew(SButton).ButtonStyle(S.Get()).ContentPadding(Pad).VAlign(VAlign_Center)
		.OnClicked_Lambda([OnClick] { if (OnClick) OnClick(); return FReply::Handled(); })
		[
			SNew(STextBlock).Text(FText::FromString(Label)).Font(Font).ColorAndOpacity(Text)
		];
}

FString S47Css::Played(double Sec)
{
	// SaveMenu.ts played()
	const int32 M = FMath::FloorToInt32(Sec / 60);
	if (M < 1) return TEXT("under a minute");
	if (M < 60) return FString::Printf(TEXT("%d min"), M);
	return FString::Printf(TEXT("%d h %d min"), M / 60, M % 60);
}

namespace
{
	// CSS 'ease' (cubic-bezier(.25, .1, .25, 1)) at x in 0..1
	float Ease(float X)
	{
		X = FMath::Clamp(X, 0.f, 1.f);
		const float X1 = 0.25f, Y1 = 0.1f, X2 = 0.25f, Y2 = 1.f;
		float T = X;
		for (int32 i = 0; i < 8; ++i)
		{
			const float Bx = 3 * (1 - T) * (1 - T) * T * X1 + 3 * (1 - T) * T * T * X2 + T * T * T - X;
			const float D = 3 * (1 - T) * (1 - T) * X1 + 6 * (1 - T) * T * (X2 - X1) + 3 * T * T * (1 - X2);
			if (FMath::Abs(D) < 1e-5f) break;
			T = FMath::Clamp(T - Bx / D, 0.f, 1.f);
		}
		return 3 * (1 - T) * (1 - T) * T * Y1 + 3 * (1 - T) * T * T * Y2 + T * T * T;
	}

	// a plain button with the focus ring (button:focus-visible { outline: 2px solid var(--amber) })
	TSharedRef<SButton> RingButton(const FString& Label, const FSlateFontInfo& Font, const FLinearColor& Color, TFunction<void()> OnClick)
	{
		TSharedRef<SButton> B = SNew(SButton).ButtonStyle(Flat()).ContentPadding(FMargin(0))
			.OnClicked_Lambda([OnClick] { if (OnClick) OnClick(); return FReply::Handled(); });
		TWeakPtr<SButton> W = B;
		B->SetContent(
			SNew(SBorder).Padding(FMargin(6, 4))
			.BorderImage_Lambda([W] { const TSharedPtr<SButton> P = W.Pin(); return P.IsValid() && P->HasKeyboardFocus() ? Box(FLinearColor::Transparent, C(0xffb052), 2.f) : Box(FLinearColor::Transparent); })
			[
				SNew(SBox).MinDesiredHeight(44 * PX - 8).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Label)).Font(Font).ColorAndOpacity(Color)]
			]);
		return B;
	}

	void Focus(const TSharedPtr<SWidget>& W)
	{
		if (W.IsValid() && FSlateApplication::IsInitialized()) FSlateApplication::Get().SetAllUserFocus(W, EFocusCause::SetDirectly); // (not Navigation: Slate would draw its own focus frame over the CSS one)
	}
}

// (the title, UI.title, and the pause menu, UI.pause: SS47Menus.cpp)

// ---------- the opening card (UI.openingCard) ----------

void SS47Opening::Construct(const FArguments& InArgs)
{
	A = InArgs._Actions;
	TSharedRef<SVerticalBox> Copy = SNew(SVerticalBox);
	for (const FString& P : FS47Opening::Paragraphs())
	{
		Copy->AddSlot().AutoHeight().Padding(0, 0, 0, 16 * PX)
		[
			SNew(STextBlock).Text(FText::FromString(P)).Font(Type(18)).ColorAndOpacity(C(0xd6d3ca)).AutoWrapText(true).LineHeightPercentage(1.6f)
		];
	}
	BeginButton = RingButton(TEXT("Begin night"), Type(16), C(0xd6d3ca), [this] { if (A.Begin) A.Begin(); });
	BackButton = RingButton(TEXT("Back to title"), Type(16), C(0xd6d3ca), [this] { if (A.Back) A.Back(); });
	FSlateFontInfo H1 = Term(42);
	H1.LetterSpacing = 130;
	ChildSlot
	[
		SNew(SBorder).BorderImage(Box(FLinearColor::Black)).Padding(FMargin(22 * PX, 18 * PX))
		.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })
		[
			SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(64 * 18 * 0.62f * PX)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 24 * PX)[SNew(STextBlock).Text(FText::FromString(TEXT("SIGNAL / 47"))).Font(H1).ColorAndOpacity(C(0x8cffa4))]
					+ SVerticalBox::Slot().AutoHeight()[Copy]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 24 * PX - 16 * PX, 0, 0)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()[BeginButton.ToSharedRef()]
						+ SHorizontalBox::Slot().AutoWidth().Padding(12 * PX, 0, 0, 0)[BackButton.ToSharedRef()]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 12 * PX, 0, 0)
					[
						SNew(STextBlock).Font(Term(16)).ColorAndOpacity(C(0x9b9b8e)).AutoWrapText(true)
						.Text_Lambda([] { return FText::FromString(S47Gamepad::UsingGamepad() ? TEXT("D-PAD select   A confirm   B back") : TEXT("TAB select   ENTER confirm   ESC back")); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 12 * PX, 0, 0)
					[
						SNew(SBox).MinDesiredHeight(14 * PX * 1.4f)[SNew(STextBlock).Text_Lambda([this] { return FText::FromString(Status); }).Font(Term(14)).ColorAndOpacity(C(0x9b9b8e))]
					]
				]
			]
		]
	];
}

void SS47Opening::SetState(const FString& State)
{
	Status = State == TEXT("loading") ? TEXT("Loading narration…")
		: State == TEXT("playing") ? TEXT("Synthetic Rod Serling parody")
		: State == TEXT("error") ? TEXT("Narration is unavailable. You can begin the night.") : TEXT("");
}

void SS47Opening::FocusBegin() { Focus(BeginButton); }

FReply SS47Opening::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	// Tab goes between the two buttons; Escape cancels the card (main.ts input.onKey)
	const FKey K = E.GetKey();
	if (K == EKeys::Tab)
	{
		Focus(BeginButton.IsValid() && BeginButton->HasKeyboardFocus() ? BackButton : BeginButton);
		return FReply::Handled();
	}
	if (K == EKeys::Gamepad_DPad_Left || K == EKeys::Gamepad_DPad_Right
		|| K == EKeys::Gamepad_DPad_Up || K == EKeys::Gamepad_DPad_Down)
	{
		Focus(K == EKeys::Gamepad_DPad_Left || K == EKeys::Gamepad_DPad_Up ? BeginButton : BackButton);
		return FReply::Handled();
	}
	if (K == EKeys::Gamepad_FaceButton_Bottom)
	{
		if (BackButton.IsValid() && BackButton->HasKeyboardFocus()) { if (A.Back) A.Back(); }
		else if (A.Begin) A.Begin();
		return FReply::Handled();
	}
	if (K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right || K == EKeys::Gamepad_Special_Right)
	{ if (A.Back) A.Back(); return FReply::Handled(); }
	return FReply::Unhandled();
}

FNavigationReply SS47Opening::OnNavigation(const FGeometry& G, const FNavigationEvent& E)
{
	const EUINavigation Direction = E.GetNavigationType();
	if (Direction == EUINavigation::Left || Direction == EUINavigation::Right
		|| Direction == EUINavigation::Up || Direction == EUINavigation::Down)
	{
		Focus(Direction == EUINavigation::Left || Direction == EUINavigation::Up ? BeginButton : BackButton);
		return FNavigationReply::Stop();
	}
	return SCompoundWidget::OnNavigation(G, E);
}

// ---------- the chapter card (UI.chapterCard) ----------

void SS47ChapterCard::Construct(const FArguments& InArgs)
{
	const FS47ChapterCardOpts& Op = InArgs._Opts;
	FSlateFontInfo Closed = Term(24), Next = Term(24), H2 = Display(64);
	Closed.LetterSpacing = 120; Next.LetterSpacing = 300; H2.LetterSpacing = 60;
	auto Phase = [this](bool bA) { return FLinearColor(1, 1, 1, (bA ? AlphaA() : AlphaB()) * AlphaOut()); };
	auto Lift = [this](bool bA)
	{
		// ccIn: up from 6 px below
		const float K = bA ? FMath::Clamp((T - 0.3f) / 0.9f, 0.f, 1.f) : FMath::Clamp((T - 3.8f) / 1.1f, 0.f, 1.f);
		return TOptional<FSlateRenderTransform>(FSlateRenderTransform(FVector2f(0.f, 6.f * PX * (1.f - Ease(K)))));
	};
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(24 * PX)
		[
			SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBorder")).Padding(0)
			.ColorAndOpacity_Lambda([Phase] { return Phase(true); })
			.RenderTransform_Lambda([Lift] { return Lift(true); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text(FText::FromString(Op.Closed)).Font(Closed).ColorAndOpacity(C(0x8cffa4)).Justification(ETextJustify::Center)]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 10 * PX, 0, 0)
				[
					SNew(SBox).MaxDesiredWidth(52 * 19 * 0.62f * PX)
					[
						SNew(STextBlock).Text(FText::FromString(Op.Recap)).Font(Type(19)).ColorAndOpacity(C(0xcfc8b5)).AutoWrapText(true).LineHeightPercentage(1.5f).Justification(ETextJustify::Center)
					]
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(24 * PX)
		[
			SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBorder")).Padding(0)
			.ColorAndOpacity_Lambda([Phase] { return Phase(false); })
			.RenderTransform_Lambda([Lift] { return Lift(false); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text(FText::FromString(Op.Next)).Font(Next).ColorAndOpacity(C(0xffb052))]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6 * PX)[SNew(STextBlock).Text(FText::FromString(Op.Title)).Font(H2).ColorAndOpacity(C(0xe8e2d2))]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text(FText::FromString(Op.Clock)).Font(Term(30)).ColorAndOpacity(C(0x8cffa4))]
			]
		]
	];
}

void SS47ChapterCard::Tick(const FGeometry& G, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(G, InCurrentTime, InDeltaTime);
	T += InDeltaTime;
	if (bOut) OutT += InDeltaTime;
}

float SS47ChapterCard::AlphaA() const
{
	// .voiced: no animation; phase a shows the first half, b the second
	if (!VoicePhase.IsEmpty()) return VoicePhase == TEXT("a") ? 1.f : 0.f;
	// .cc-a: ccIn .9s ease .3s, ccOut .7s ease 3.2s
	if (T < 0.3f) return 0.f;
	if (T < 1.2f) return Ease((T - 0.3f) / 0.9f);
	if (T < 3.2f) return 1.f;
	return 1.f - Ease((T - 3.2f) / 0.7f);
}

float SS47ChapterCard::AlphaB() const
{
	if (!VoicePhase.IsEmpty()) return VoicePhase == TEXT("b") ? 1.f : 0.f;
	// .cc-b: ccIn 1.1s ease 3.8s
	return T < 3.8f ? 0.f : Ease((T - 3.8f) / 1.1f);
}
