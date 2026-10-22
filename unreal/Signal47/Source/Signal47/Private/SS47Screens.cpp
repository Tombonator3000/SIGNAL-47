#include "SS47Screens.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "S47Kit.h"
#include "S47Opening.h"
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

// ---------- the title (UI.title) ----------

void SS47Title::Construct(const FArguments& InArgs)
{
	O = InArgs._Opts;
	bGated = O.bGate;
	bCalm = O.bCalm;
	const FLinearColor MenuC = C(0xe9e4d6), Dis = C(0x6e6a62);
	const FMargin Gap(0, 6 * PX, 0, 0);

	// .menu: Continue (and where it would take you), New night, Load case, Settings
	TSharedRef<SVerticalBox> Menu = SNew(SVerticalBox);
	ContButton = TextButton(TEXT("Continue"), Term(34), MenuC, MenuC, [this] { if (O.OnContinue) O.OnContinue(); }, !O.Cont.IsEmpty(), Dis, true);
	Menu->AddSlot().AutoHeight()[ContButton.ToSharedRef()];
	if (!O.Cont.IsEmpty())
	{
		// .cont-info, under Continue, 1.4em in
		Menu->AddSlot().AutoHeight().Padding(1.4f * 14 * PX, 0, 0, 6 * PX)[SNew(STextBlock).Text(FText::FromString(O.Cont)).Font(Type(14)).ColorAndOpacity(C(0xa39e92))];
	}
	StartButton = TextButton(TEXT("New night"), Term(34), MenuC, MenuC, [this] { if (O.OnStart) O.OnStart(); }, true, Dis, true);
	Menu->AddSlot().AutoHeight().Padding(Gap)[StartButton.ToSharedRef()];
	Menu->AddSlot().AutoHeight().Padding(Gap)[TextButton(TEXT("Load case"), Term(34), MenuC, MenuC, [this] { if (O.OnLoad) O.OnLoad(); }, O.bCanLoad, Dis, true)];
	Menu->AddSlot().AutoHeight().Padding(Gap)[TextButton(TEXT("Settings"), Term(34), MenuC, MenuC, [this] { if (O.OnSettings) O.OnSettings(); }, true, Dis, true)];

	// .flash-note with its two choices
	auto Pick = [this](const FString& Label, bool bCalmChoice)
	{
		return SNew(SButton).ButtonStyle(Flat()).ContentPadding(FMargin(0))
			.OnClicked_Lambda([this, bCalmChoice] { bCalm = bCalmChoice; if (O.OnFlashPick) O.OnFlashPick(bCalmChoice); return FReply::Handled(); })
			[
				SNew(SBorder).Padding(FMargin(8 * PX, 1 * PX))
				.BorderImage_Lambda([this, bCalmChoice] { return Box(FLinearColor::Transparent, bCalm == bCalmChoice ? C(0xa49c88) : C(0x4a463e), 1.f); })
				[
					SNew(STextBlock).Text(FText::FromString(Label)).Font(Type(14 * 0.95f))
					.ColorAndOpacity_Lambda([this, bCalmChoice] { return FSlateColor(bCalm == bCalmChoice ? C(0xf3efe4) : C(0x8f8a7c)); })
				]
			];
	};
	const float NoteW = 46 * 14 * 0.62f * PX; // max-width: 46ch
	TSharedRef<SWidget> Notes = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 26 * PX, 0, 0)
		[
			SNew(SBox).MaxDesiredWidth(NoteW).HAlign(HAlign_Left)
			[
				SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6 * PX, 4))
				+ SWrapBox::Slot()[SNew(STextBlock).Text(FText::FromString(TEXT("Near the end of the night there is a short sequence of flashing images."))).Font(Type(14)).ColorAndOpacity(C(0xb9b2a0)).AutoWrapText(true).LineHeightPercentage(1.5f)]
				+ SWrapBox::Slot()[Pick(TEXT("Flashes"), false)]
				+ SWrapBox::Slot()[Pick(TEXT("Slow fades"), true)]
			]
		]
		// (.rotate-hint: phones held upright only)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 26 * PX, 0, 0)
		[
			SNew(SBox).MaxDesiredWidth(NoteW).HAlign(HAlign_Left)
			[
				SNew(STextBlock).Font(Type(14)).ColorAndOpacity(C(0xa39e92)).AutoWrapText(true).LineHeightPercentage(1.5f)
				.Text(FText::FromString(TEXT("The whole night, from the control room at 23:41 to the old Roswell road at dawn. Headphones help. Desktop: WASD, mouse, E to use, Tab for notes, B for the case board, C for the camera, F for a flashlight once you have one, right mouse for a closer look, H for all of them. Phone: left thumb walks, right thumb looks, tap things to use them. The notebook has every control on its Controls page.")))
			]
		];

	// h1: SIGNAL / 47 (Oswald 500 in the web version; the glow round it is a text-shadow Slate has no blur for)
	const float H1 = 168.f;
	FSlateFontInfo Big = Display(H1);
	Big.LetterSpacing = 20;
	auto MenuVis = [this] { return bGated ? EVisibility::Collapsed : EVisibility::Visible; };
	ChildSlot
	[
		SNew(SBox).Padding_Lambda([this] { return FMargin(0.07f * Size.X, 0.06f * Size.Y, 0.07f * Size.X, 0.07f * Size.Y); })
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(1.f)[SNew(SSpacer)]
			// line-height .86: Oswald's line (about 1.48 em) stands out of the line box by half the
			// difference above and below
			+ SVerticalBox::Slot().AutoHeight().Padding(0, -0.31f * H1 * PX, 0, -0.31f * H1 * PX)
			[
				SNew(SBox)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("SIGNAL"))).Font(Big).ColorAndOpacity(C(0xf3efe4))]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.12f * H1 * PX, 0)[SNew(STextBlock).Text(FText::FromString(TEXT("/"))).Font(Big).ColorAndOpacity(C(0xff5a45))]
					+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("47"))).Font(Big).ColorAndOpacity(C(0xf3efe4))]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 14 * PX, 0, 34 * PX)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("New Mexico, 1986. Night shift at SARO."))).Font(Term(28)).ColorAndOpacity(C(0xffb052))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				// .gate: blinks (opacity 1 to .35 and back, 1.4 s each way)
				SNew(STextBlock).Text(FText::FromString(O.bTouch ? TEXT("Tap to begin") : TEXT("Click or press any key"))).Font(Term(32))
				.ColorAndOpacity_Lambda([this]
				{
					const float Ph = FMath::Fmod(T, 2.8f), U = Ph < 1.4f ? Ph / 1.4f : 2.f - Ph / 1.4f;
					return FSlateColor(C(0x8cffa4, 1.f - 0.65f * FMath::SmoothStep(0.f, 1.f, U)));
				})
				.Visibility_Lambda([this] { return bGated ? EVisibility::Visible : EVisibility::Collapsed; })
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBorder")).Padding(0).Visibility_Lambda(MenuVis)
				.ColorAndOpacity_Lambda([this] { return FLinearColor(1, 1, 1, 1.f - FMath::Square(1.f - FMath::Clamp(InT / 0.5f, 0.f, 1.f))); })
				[
					Menu
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox).Visibility_Lambda(MenuVis)[Notes]
			]
		]
	];
}

void SS47Title::Tick(const FGeometry& G, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(G, InCurrentTime, InDeltaTime);
	Size = G.GetLocalSize();
	T += InDeltaTime;
	InT += InDeltaTime;
	// (focus() as the title is shown: Slate can only focus a widget that is on the screen)
	if (bFocusPending) { bFocusPending = false; FocusMenu(); }
}

int32 SS47Title::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	// background: linear-gradient(to top, rgba(3,4,8,.92) 0%, rgba(3,4,8,.45) 38%, transparent 70%)
	const FVector2f S = FVector2f(G.GetLocalSize());
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	const int32 Strips = 96;
	for (int32 i = 0; i < Strips; ++i)
	{
		const float Up = (i + 0.5f) / Strips; // 0 at the foot
		const float A = Up < 0.38f ? FMath::Lerp(0.92f, 0.45f, Up / 0.38f) : Up < 0.7f ? FMath::Lerp(0.45f, 0.f, (Up - 0.38f) / 0.32f) : 0.f;
		if (A <= 0.f) continue;
		const float Y0 = S.Y * (1.f - (i + 1.f) / Strips), H = S.Y / Strips + 1.f;
		FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(S.X, H), FSlateLayoutTransform(FVector2f(0.f, Y0))), White, ESlateDrawEffect::None, C(0x030408, A));
	}
	return SCompoundWidget::OnPaint(Args, G, Cull, Out, Layer + 1, Style, bParentEnabled);
}

void SS47Title::First()
{
	TFunction<void()> Fn = O.OnFirstTouch;
	O.OnFirstTouch = nullptr;
	if (Fn) Fn();
}

FReply SS47Title::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	// main.ts firstTouch: the first key on the gated title brings the menu in (and the music)
	if (bGated) { Ungate(); First(); return FReply::Handled(); }
	First();
	return FReply::Unhandled();
}

FReply SS47Title::OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E)
{
	if (bGated) { Ungate(); First(); return FReply::Handled(); }
	First();
	return FReply::Unhandled();
}

void SS47Title::Ungate()
{
	// UI.ungate
	if (!bGated) return;
	bGated = false;
	InT = 0.f;
	FocusMenu();
}

void SS47Title::FocusMenu()
{
	if (bGated) { Focus(SharedThis(this)); return; }
	Focus(ContButton.IsValid() && ContButton->IsEnabled() ? ContButton : StartButton);
}

void SS47Title::FocusStart()
{
	if (!bGated) Focus(StartButton);
}

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
	if (E.GetKey() == EKeys::Tab)
	{
		Focus(BeginButton.IsValid() && BeginButton->HasKeyboardFocus() ? BackButton : BeginButton);
		return FReply::Handled();
	}
	if (E.GetKey() == EKeys::Escape) { if (A.Back) A.Back(); return FReply::Handled(); }
	return FReply::Unhandled();
}

// ---------- the pause menu and the settings (UI.pause) ----------

namespace
{
	const TCHAR* QualityName(const FString& Q) { return Q == TEXT("ultra") ? TEXT("Ultra") : Q == TEXT("low") ? TEXT("Low") : TEXT("High"); }
	// PICTURE: clean or the 1986 tape look (the worn tape, 'heavy', is not in M_WebTone yet)
	const TCHAR* PictureName(const FString& P) { return P == TEXT("off") ? TEXT("Clean") : P == TEXT("heavy") ? TEXT("Worn VHS") : TEXT("VHS"); }
}

void SS47Pause::Construct(const FArguments& InArgs)
{
	O = InArgs._Opts;
	OnClose = InArgs._OnClose;
	bArtBell = O.bArtBell; bRadioCast = O.bRadioCast; bCharacters = O.bCharacters; bSerling = O.bSerling; bKinski = O.bKinski;
	bInv = O.bInvertY; bBig = O.bLargeText; bCalm = O.bCalmFlash; bStill = O.bStillShots; bAb = O.bAbduct;
	Q = O.Quality; Pic = O.Picture;
	Vol = O.Volume; Mus = O.Music; Rad = O.Radio; Sens = O.Sens; Fov = O.Fov;

	const FLinearColor Amber = C(0xffb052);
	auto Opt = [&](const FString& Label, TFunction<void()> Fn)
	{
		return SNew(SBox).MinDesiredHeight(44 * PX)[TextButton(Label, Term(28), C(0xe8e2d2), Amber, MoveTemp(Fn), true, C(0x6e6a62), false, FMargin(0, 6 * PX))];
	};
	auto Set = [&](const FString& Label, TSharedRef<SWidget> Ctl)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(150 * PX)[SNew(STextBlock).Text(FText::FromString(Label)).Font(Term(22)).ColorAndOpacity(C(0xcfc8b5)).AutoWrapText(true)]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(10 * PX, 0, 0, 0)[Ctl];
	};
	auto Range = [&](float Min, float Max, float Step, float* V, TFunction<void(float)> Fn)
	{
		return SNew(SBox).HeightOverride(32 * PX)
		[
			SNew(SSlider).MinValue(Min).MaxValue(Max).StepSize(Step).MouseUsesStep(true)
			.Value_Lambda([V] { return *V; })
			.OnValueChanged_Lambda([V, Fn](float X) { *V = X; if (Fn) Fn(X); })
			// (accent-color: the amber handle on the dark scheme's grey track)
			.SliderBarColor(C(0x5a5a5e)).SliderHandleColor(Amber)
		];
	};
	auto Qual = [&](TFunction<FString()> Text, TFunction<void()> Fn)
	{
		return SNew(SBox).MinDesiredHeight(36 * PX).VAlign(VAlign_Center)
			[TextButton(TAttribute<FText>::CreateLambda([Text] { return FText::FromString(Text()); }), Term(22), Amber, Amber, MoveTemp(Fn), FMargin(0))];
	};
	auto OnOff = [&](bool* B, TFunction<void(bool)> Fn)
	{
		return Qual([B] { return FString(*B ? TEXT("On") : TEXT("Off")); }, [B, Fn] { *B = !*B; if (Fn) Fn(*B); });
	};
	auto Note = [&](const FString& S)
	{
		return SNew(STextBlock).Text(FText::FromString(S)).Font(Type(13)).ColorAndOpacity(C(0x8e887a)).AutoWrapText(true).LineHeightPercentage(1.5f);
	};
	auto Row = [](TSharedRef<SVerticalBox>& Box, TSharedRef<SWidget> W, const FMargin& M = FMargin(0, 8 * PX)) { Box->AddSlot().AutoHeight().Padding(M)[W]; };

	FSlateFontInfo H3 = Display(34);
	H3.LetterSpacing = 30;
	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	Row(Body, SNew(STextBlock).Text(FText::FromString(O.Title.IsEmpty() ? TEXT("Paused") : O.Title)).Font(H3).ColorAndOpacity(C(0xf0ead8)), FMargin(0, 0, 0, 18 * PX));
	Row(Body, Opt(O.bSettingsOnly ? TEXT("Back") : TEXT("Resume"), [this] { if (OnClose) OnClose(false); }), FMargin(0));
	if (O.OnSave) Row(Body, Opt(TEXT("Save case"), [this] { if (O.OnSave) O.OnSave(); }), FMargin(0));
	if (O.OnLoad) Row(Body, Opt(TEXT("Load case"), [this] { if (O.OnLoad) O.OnLoad(); }), FMargin(0));
	Row(Body, Set(TEXT("Volume"), Range(0.f, 1.f, 0.01f, &Vol, O.OnVolume)));
	Row(Body, Set(TEXT("Music"), Range(0.f, 1.f, 0.01f, &Mus, O.OnMusic)));
	Row(Body, Set(TEXT("Radio"), Range(0.f, 1.f, 0.01f, &Rad, O.OnRadio)));
	const FMargin NoteM(0, 0, 0, 14 * PX);
	Row(Body, Set(TEXT("Art Bell radio host"), OnOff(&bArtBell, O.OnArtBell)));
	Row(Body, Note(TEXT("Synthetic Art Bell parody for Earl on Night Line, AM 1420, and when you speak with him in Caliche.")), NoteM);
	Row(Body, Set(TEXT("Radio callers & bulletin"), OnOff(&bRadioCast, O.OnRadioCast)));
	Row(Body, Note(TEXT("Synthetic callers and the 1947 news bulletin on AM 1420.")), NoteM);
	Row(Body, Set(TEXT("Character voices"), OnOff(&bCharacters, O.OnCharacters)));
	Row(Body, Note(TEXT("Synthetic parody voices for the attendant, deputy, souvenir man, Nora and the recorded field line.")), NoteM);
	Row(Body, Set(TEXT("Rod Serling narrator"), OnOff(&bSerling, O.OnSerling)));
	Row(Body, Note(TEXT("Synthetic Rod Serling parody for descriptions and chapter transitions. Hold Zoom for a spoken closer look; V repeats a description.")), NoteM);
	Row(Body, Set(TEXT("Kinski read-aloud"), OnOff(&bKinski, O.OnKinski)));
	Row(Body, Note(TEXT("Synthetic Kinski parody for papers, optional conversation reading and other phone voices. Also reads descriptions when the Rod Serling narrator is Off.")), NoteM);
	Row(Body, Set(TEXT("Look speed"), Range(0.3f, 2.5f, 0.05f, &Sens, O.OnSens)));
	Row(Body, Set(TEXT("Field of view"), Range(60.f, 90.f, 1.f, &Fov, O.OnFov)));
	Row(Body, Set(TEXT("Invert look"), OnOff(&bInv, O.OnInvertY)));
	Row(Body, Set(TEXT("Text"), Qual([this] { return FString(bBig ? TEXT("Large") : TEXT("Normal")); }, [this] { bBig = !bBig; if (O.OnLargeText) O.OnLargeText(bBig); })));
	Row(Body, Set(TEXT("Graphics"), Qual([this] { return FString(QualityName(Q)); }, [this]
	{
		// Ultra (shadows, occlusion, bloom) is offered on a computer; phones keep High and Low
		const TArray<FString> Order = O.bUltraOk ? TArray<FString>{ TEXT("ultra"), TEXT("high"), TEXT("low") } : TArray<FString>{ TEXT("high"), TEXT("low") };
		Q = Order[(Order.IndexOfByKey(Q) + 1) % Order.Num()];
		if (O.OnQuality) O.OnQuality(Q);
	})));
	Row(Body, Set(TEXT("Picture"), Qual([this] { return FString(PictureName(Pic)); }, [this]
	{
		// off -> vhs -> heavy -> off in the web version; Clean and VHS here (see PictureName)
		Pic = Pic == TEXT("off") ? TEXT("vhs") : TEXT("off");
		if (O.OnPicture) O.OnPicture(Pic);
	})));
	Row(Body, Set(TEXT("Flashing images"), Qual([this] { return FString(bCalm ? TEXT("Slow fades") : TEXT("Flashes")); }, [this] { bCalm = !bCalm; if (O.OnCalmFlash) O.OnCalmFlash(bCalm); })));
	if (O.OnAbduct) Row(Body, Set(TEXT("Missing time"), OnOff(&bAb, O.OnAbduct)));
	Row(Body, Set(TEXT("Ending camera"), Qual([this] { return FString(bStill ? TEXT("Still shots") : TEXT("Moving")); }, [this] { bStill = !bStill; if (O.OnStillShots) O.OnStillShots(bStill); })));
	if (!O.bSettingsOnly) Row(Body, Opt(TEXT("Quit to title"), [this] { if (OnClose) OnClose(true); if (O.OnTitle) O.OnTitle(); }), FMargin(0));
	Row(Body, Note(O.bSettingsOnly ? TEXT("Settings are kept on this device.") : TEXT("Nothing in the control room moves on while the game is paused.")), FMargin(0, 18 * PX, 0, 0));

	ChildSlot
	[
		// .overlay
		SNew(SBorder).BorderImage(Box(C(0x020306, 0.72f))).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(12)
		[
			SNew(SBox).WidthOverride(460 * PX).MaxDesiredHeight_Lambda([this] { return FOptionalSize(FMath::Max(200.f, float(GetCachedGeometry().GetLocalSize().Y) - 24.f)); })
			[
				SNew(SBorder).BorderImage(Box(C(0x080b13, 0.94f), FLinearColor(1, 1, 1, 0.12f), 1.f)).Padding(FMargin(30 * PX, 28 * PX))
				[
					SNew(SScrollBox) + SScrollBox::Slot()[Body]
				]
			]
		]
	];
}

FReply SS47Pause::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	// main.ts: Escape or P with the menu open closes it (and resumes)
	if (E.GetKey() == EKeys::Escape || E.GetKey() == EKeys::P) { if (OnClose) OnClose(false); return FReply::Handled(); }
	return FReply::Unhandled();
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
	// .cc-a: ccIn .9s ease .3s, ccOut .7s ease 3.2s
	if (T < 0.3f) return 0.f;
	if (T < 1.2f) return Ease((T - 0.3f) / 0.9f);
	if (T < 3.2f) return 1.f;
	return 1.f - Ease((T - 3.2f) / 0.7f);
}

float SS47ChapterCard::AlphaB() const
{
	// .cc-b: ccIn 1.1s ease 3.8s
	return T < 3.8f ? 0.f : Ease((T - 3.8f) / 1.1f);
}
