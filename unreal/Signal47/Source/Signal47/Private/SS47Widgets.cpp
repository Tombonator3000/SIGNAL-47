#include "SS47Widgets.h"
#include "S47Keys.h"

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
#include "S47Flashlight.h"
#include "S47Flow.h"
#include "S47GameStyle.h"
#include "S47Image.h"
#include "S47Night.h"
#include "S47ReadAloud.h"
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
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SCanvas.h"
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
}

// ---------- the overlay ----------

namespace
{
	US47Main* Main() { return US47Main::Get(nullptr); }

	// a line of the first part of a long text, cut at a word (the NOTED slip's excerpt)
	FString Cut(const FString& S, int32 Max)
	{
		if (S.Len() <= Max) return S;
		FString C = S.Left(Max);
		const int32 Sp = C.Find(TEXT(" "), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		if (Sp > Max / 2) C = C.Left(Sp);
		while (C.Len() && (C.EndsWith(TEXT(",")) || C.EndsWith(TEXT(":")) || C.EndsWith(TEXT(";")) || C.EndsWith(TEXT(".")))) C.LeftChopInline(1);
		return C + TEXT("...");
	}

	// linear-gradient(90deg, Color, transparent): the chosen action's glow
	class SFadeBar : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFadeBar) {}
			SLATE_ARGUMENT(FLinearColor, Color)
		SLATE_END_ARGS()
		void Construct(const FArguments& A) { Color = A._Color; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8, 8); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool) const override
		{
			const FVector2f S = FVector2f(G.GetLocalSize());
			FLinearColor A = Color, B = Color;
			A.A *= Style.GetColorAndOpacityTint().A; B.A = 0.f;
			FSlateDrawElement::MakeGradient(Out, Layer, G.ToPaintGeometry(), { FSlateGradientStop(FVector2D(0, 0), A), FSlateGradientStop(FVector2D(S.X, 0), B) }, Orient_Vertical);
			return Layer + 1;
		}
	private:
		FLinearColor Color;
	};
}

bool SS47Hud::Playing() const { const US47Main* M = Main(); return !M || M->Mode == US47Main::EMode::Play; }
bool SS47Hud::ModalOpen() const { const US47Main* M = Main(); return M && M->UI && M->UI->Modal(); }

void SS47Hud::Construct(const FArguments& InArgs)
{
	using namespace S47Game;
	WhiteBrush = MakeUnique<FSlateColorBrush>(FLinearColor::White);
	PromptAttr = InArgs._Prompt; AltAttr = InArgs._PromptAlt; ClockAttr = InArgs._Clock;
	ShowPromptAttr = InArgs._ShowPrompt; ShowAimAttr = InArgs._ShowAim;
	auto Aim = [this] { return ShowAimAttr.Get(true); };
	auto Vis = [Aim] { return Aim() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; };
	auto PromptOn = [this] { return ShowPromptAttr.Get(ShowAimAttr.Get(true)); };
	// the watch's face: the seconds clock when it is set (chapter six, the warning), else the night's
	auto WatchText = [this] { return SecondsText.IsEmpty() ? ClockAttr.Get().ToString() : SecondsText; };

	// .obj: the place and the clock in amber over the objective, a soft dark under it
	const TSharedRef<SWidget> Objective = SNew(SBorder).BorderImage(Box(FLinearColor::Transparent)).Padding(0)
		.ColorAndOpacity_Lambda([this] { const float K = FMath::SmoothStep(0.f, 1.f, ObjectiveIn); return FLinearColor(1, 1, 1, 0.2f + 0.8f * K); })
		.Visibility_Lambda([this] { return ObjectiveText.IsEmpty() || Vf.bOn || !Playing() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
		[
			Shadowed(SNew(SBox).MaxDesiredWidth(P(520))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text_Lambda([this] { return FText::FromString(ObjectiveHead.IsEmpty() ? Place : ObjectiveHead); }).Font(Term(22, 3)).ColorAndOpacity(Amber())]
					+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT(" // "))).Font(Term(22, 3)).ColorAndOpacity(C(0x7d786d)).Visibility_Lambda([this] { return ObjectiveHead.IsEmpty() && Place.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })]
					+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text_Lambda([] { const US47Main* M = Main(); return FText::FromString(M && M->Story ? M->Story->ClockText(false) : FString()); }).Font(Term(22, 3)).ColorAndOpacity(Amber())]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, P(4), 0, 0)
				[
					SNew(STextBlock).Text_Lambda([this] { return FText::FromString(ObjectiveBody); }).Font(Type(20)).ColorAndOpacity(C(0xefe7d3))
					.Visibility_Lambda([this] { return ObjectiveBody.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
					.ShadowOffset(FVector2D(0, P(2))).ShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f)).AutoWrapText(true).LineHeightPercentage(LinePercent(Type(20), 28))
				]
			], 36.f, FVector2D(0, 2), 0.3f)
		];

	// .watch: the LCD and the day under it
	const TSharedRef<SWidget> Watch = SNew(SVerticalBox)
		.Visibility_Lambda([WatchText] { return WatchText().IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
		[
			SNew(SBorder).BorderImage(Box(C(0x0a0804, 0.62f), C(0xffb052, 0.25f), 1.f)).Padding(FMargin(P(12), 0))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock).Font(Term(40, 3)).ColorAndOpacity(C(0xffcf8a)).ShadowOffset(FVector2D::ZeroVector)
					.Text_Lambda([WatchText] { const FString T = WatchText(); int32 I = INDEX_NONE; T.FindLastChar(TEXT(':'), I); return FText::FromString(T.Len() > 5 && I > 3 ? T.Left(I) : T); })
				]
				// (the seconds in the brighter amber the console counted the line drop in, .clockline.secs .ss)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock).Font(Term(40, 3)).ColorAndOpacity(C(0xffd27a))
					.Text_Lambda([WatchText] { const FString T = WatchText(); int32 I = INDEX_NONE; T.FindLastChar(TEXT(':'), I); return FText::FromString(T.Len() > 5 && I > 3 ? T.Mid(I) : FString()); })
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, P(4), 0, 0)
		[
			SNew(STextBlock).Font(Term(18, 2)).ColorAndOpacity(C(0x8a857a)).ShadowOffset(FVector2D(0, P(1))).ShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.85f))
			.Text_Lambda([] { const US47Main* M = Main(); return FText::FromString(M && M->Story && M->Mode == US47Main::EMode::Play ? DayLine(M->Story->Clock()) : FString()); })
		];

	// .read: V and what it reads (UI.ts descriptionButton; dimmed when there is no recording of the text)
	const TSharedRef<SWidget> Read = SNew(SBorder).BorderImage(Box(FLinearColor::Transparent)).Padding(0)
		.Visibility_Lambda([this, Vis] { return DescButton.bShown ? Vis() : EVisibility::Collapsed; })
		.ColorAndOpacity_Lambda([this] { return FLinearColor(1, 1, 1, DescButton.bDisabled ? 0.5f : 1.f); })
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, P(6), 0)[Keycap(S47Keys::Label(TEXT("describe")), EKey::Outline, 20)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Font(Term(20, 1)).ColorAndOpacity(Hint()).ShadowOffset(FVector2D(0, P(1))).ShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.85f))
				.Text_Lambda([this] { FString T = DescButton.Text; T.RemoveFromEnd(TEXT(" [V]")); return FText::FromString(T.ToUpper()); })
			]
		];

	// .cross: a small ring; amber and a little larger on something that can be used
	const TSharedRef<SWidget> Cross = SNew(SBox).Visibility_Lambda(Vis)
		.WidthOverride_Lambda([this] { return FOptionalSize(PromptAttr.Get().IsEmpty() ? P(12) : P(16)); })
		.HeightOverride_Lambda([this] { return FOptionalSize(PromptAttr.Get().IsEmpty() ? P(12) : P(16)); })
		[
			SNew(SBorder).Padding(0).BorderImage_Lambda([this]
			{
				return PromptAttr.Get().IsEmpty() ? Box(FLinearColor::Transparent, C(0xffecc8, 0.85f), 2.f, 6.f) : Box(C(0xffb052, 0.12f), C(0xffb052, 0.95f), 2.f, 8.f);
			})
		];

	// .prompt: the keycap and the label on a thin backing with an amber edge; the second thing (Q) on a
	// line of its own; a closer look's words (.examine) under them
	auto PromptLine = [](TAttribute<FText> Label, const FString& Key, bool bMain) -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(P(2))[SNew(SBorder).BorderImage(Box(bMain ? Amber() : C(0xffb052, 0.5f)))]]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBorder).BorderImage(Box(C(0x05070d, 0.6f))).Padding(bMain ? FMargin(P(8), P(6), P(16), P(6)) : FMargin(P(8), P(3), P(14), P(3)))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, P(12), 0)[Keycap(*Key, bMain ? EKey::Amber : EKey::Outline, bMain ? 24 : 20)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Font(Term(bMain ? 28 : 22, 2)).ColorAndOpacity(bMain ? C(0xf4ecdb) : C(0xcfc8b5))
						.Text_Lambda([Label] { return FText::FromString(Label.Get().ToString().ToUpper()); })
					]
				]
			];
	};
	const TSharedRef<SWidget> Prompt = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SBox).Visibility_Lambda([this, PromptOn] { return PromptOn() && !PromptAttr.Get().IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[PromptLine(PromptAttr, S47Keys::Label(TEXT("use")), true)]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, P(6), 0, 0)
		[
			SNew(SBox).Visibility_Lambda([this, PromptOn] { return PromptOn() && !AltAttr.Get().IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[PromptLine(AltAttr, S47Keys::Label(TEXT("alt")), false)]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, P(10), 0, 0)
		[
			SNew(SBorder).BorderImage(Box(C(0x06080c, 0.55f), FLinearColor::Transparent, 0.f, 3.f)).Padding(FMargin(P(10), P(4)))
			.Visibility_Lambda([this, Vis] { return ExamineText.IsEmpty() ? EVisibility::Collapsed : Vis(); })
			[
				SNew(STextBlock).Text_Lambda([this] { return FText::FromString(ExamineText); }).Font(Term(22)).ColorAndOpacity(C(0xf3ead3)).AutoWrapText(true).WrapTextAt(P(640)).Justification(ETextJustify::Center)
			]
		];

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(P(40), P(34), 0, 0)[Objective]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0, P(30), P(40), 0)[Watch]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0, P(112), P(40), 0)[Read]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[Cross]
		// the prompt 28 px under the ring; at the line its top at 64% of the screen (.at-line .prompt)
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top)
		.Padding(TAttribute<FMargin>::CreateLambda([this] { const float H = float(GetCachedGeometry().GetLocalSize().Y); return FMargin(0, bAtLine ? 0.64f * H : 0.5f * H + P(28), 0, 0); }))
		[Prompt]
		// the captions at the foot (.sub), the newest under
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, P(96))[SAssignNew(ToastBox, SVerticalBox)]
		// the NOTED slip (.note), bottom left
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(P(40), 0, 0, P(96))
		[
			SNew(SBorder).BorderImage(Box(FLinearColor::Transparent)).Padding(0)
			.Visibility_Lambda([this] { return SlipT >= 0.f && Playing() && !ModalOpen() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.ColorAndOpacity_Lambda([this]
			{
				const float A = SlipT < 0.35f ? SlipT / 0.35f : SlipT > 3.9f ? FMath::Max(0.f, 1.f - (SlipT - 3.9f) / 0.5f) : 1.f;
				return FLinearColor(1, 1, 1, FMath::SmoothStep(0.f, 1.f, A));
			})
			.RenderTransform_Lambda([this]
			{
				const float In = FMath::SmoothStep(0.f, 1.f, FMath::Min(1.f, SlipT / 0.35f));
				const float Out = SlipT > 3.9f ? FMath::SmoothStep(0.f, 1.f, (SlipT - 3.9f) / 0.5f) : 0.f;
				return TOptional<FSlateRenderTransform>(FSlateRenderTransform(FVector2D(P(-60) * (1.f - In) - P(30) * Out, 0)));
			})
			[SAssignNew(SlipSlot, SBox)]
		]
		// .hint: the keys, as keycaps
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, P(26))
		[
			SAssignNew(HintRow, SHorizontalBox).Visibility_Lambda([this, Vis] { return Vf.bOn ? EVisibility::Collapsed : Vis(); })
		]
	];
}

void SS47Hud::Objective(const FString& Text)
{
	if (Text == ObjectiveText) return;
	ObjectiveIn = 0.f;
	ObjectiveText = Text;
	// "FRAME 01 // INSPECT THE PRINT": the head goes on the tag line, before the clock; without one
	// the tag is the place
	FString Head, Body;
	if (Text.Split(TEXT(" // "), &Head, &Body)) { ObjectiveHead = Head.TrimStartAndEnd(); ObjectiveBody = Body.TrimStartAndEnd(); }
	else { ObjectiveHead.Reset(); ObjectiveBody = Text.TrimStartAndEnd(); }
}

void SS47Hud::Toast(const FString& Text, float Secs, TFunction<double()> Clock)
{
	// Captions already have their own radio/telephone delivery. Do not turn them
	// into a second narrator or replace the last environmental description.
	static const FRegexPattern Caption(TEXT("^(?:RADIO(?:,|:)|CB,|WARD:|DR\\. WARD:|NORA:|N\\. VEGA:|THE LINE:|YOU:|TOM[ÁA]S:)"), ERegexPatternFlags::CaseInsensitive);
	FRegexMatcher M(Caption, Text);
	if (!M.FindNext()) LastToast = Text;
	// UI.toast: the same line twice in a row is noise
	TSharedPtr<FToast> LastLive;
	for (const TSharedRef<FToast>& T : Toasts) if (T->Out < 0.f) LastLive = T;
	if (LastLive.IsValid() && LastLive->Text == Text) return;
	// UI.ts toast(text, secs, clock): recorded captions share the pausable sound clock.
	const double At = Clock ? Clock() : Now;
	const TSharedRef<FToast> T = MakeShared<FToast>();
	T->Text = Text;
	T->Until = At + Secs;
	T->Clock = MoveTemp(Clock);
	// the speaker over the line; a plain line is stamped with the clock it came at
	S47Game::SplitCaption(Text, T->Who, T->Line);
	if (T->Who.IsEmpty()) if (const US47Main* Mn = Main(); Mn && Mn->Story && Mn->Mode == US47Main::EMode::Play) T->Who = Mn->Story->ClockText(false);
	Toasts.Add(T);
	// at most two lines on screen; older ones fade out instead of vanishing mid-read
	int32 Live = 0;
	for (const TSharedRef<FToast>& X : Toasts) Live += X->Out < 0.f;
	for (const TSharedRef<FToast>& X : Toasts) if (Live > 2 && X->Out < 0.f) { X->Out = 0.f; --Live; }
	Rebuild();
}

void SS47Hud::ClearToasts() { Toasts.Empty(); LastToast.Empty(); Rebuild(); }

void SS47Hud::Rebuild()
{
	using namespace S47Game;
	if (!ToastBox.IsValid()) return;
	ToastBox->ClearChildren();
	const bool bLarge = Main() && Main()->UI && Main()->UI->bLargeText;
	const float Px = bLarge ? 26.f : 22.f;
	for (const TSharedRef<FToast>& T : Toasts)
	{
		TSharedRef<SVerticalBox> In = SNew(SVerticalBox);
		if (!T->Who.IsEmpty()) In->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, P(2))[SNew(STextBlock).Text(FText::FromString(T->Who.ToUpper())).Font(Term(22, 3)).ColorAndOpacity(Amber())];
		In->AddSlot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock).Text(FText::FromString(T->Line)).Font(Type(Px)).ColorAndOpacity(C(0xf1ead8)).AutoWrapText(true).WrapTextAt(P(676)).Justification(ETextJustify::Center)
			.LineHeightPercentage(LinePercent(Type(Px), Px * 1.4f))
		];
		const TWeakPtr<FToast> W = T;
		ToastBox->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0, P(3))
		[
			SNew(SBorder).BorderImage(Box(C(0x05070d, 0.72f))).Padding(FMargin(P(22), P(10), P(22), P(12)))
			.ColorAndOpacity_Lambda([W]
			{
				const TSharedPtr<FToast> X = W.Pin();
				if (!X.IsValid()) return FLinearColor::Transparent;
				return FLinearColor(1, 1, 1, X->Out >= 0.f ? FMath::Max(0.f, 1.f - X->Out / 0.6f) : FMath::SmoothStep(0.f, 1.f, X->In));
			})
			.RenderTransform_Lambda([W] { const TSharedPtr<FToast> X = W.Pin(); return TOptional<FSlateRenderTransform>(FSlateRenderTransform(FVector2D(0, X.IsValid() && X->Out < 0.f ? P(6) * (1.f - FMath::SmoothStep(0.f, 1.f, X->In)) : 0.f))); })
			[In]
		];
	}
}

void SS47Hud::Noted(const FString& Text)
{
	// one slip at a time; a burst keeps the newest few
	if (Slips.Num() > 3) Slips.RemoveAt(1);
	Slips.Add(Cut(Text, 90));
	if (SlipT < 0.f) ShowSlip();
}

void SS47Hud::ShowSlip()
{
	using namespace S47Game;
	if (!SlipSlot.IsValid()) return;
	if (!Slips.Num()) { SlipT = -1.f; SlipSlot->SetContent(SNullWidget::NullWidget); return; }
	SlipT = 0.f;
	// .note: paper, the red NOTED stamp and the line in the player's hand, a degree off true
	SlipSlot->SetContent(Turned(Shadowed(
		SNew(SBorder).BorderImage(Box(Paper())).Padding(FMargin(P(12), P(10), P(16), P(10)))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, P(12), 0)[Stamp(TEXT("NOTED"), 20.f, -4.f)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).MaxDesiredWidth(P(300))
				[
					SNew(STextBlock).Text(FText::FromString(Slips[0])).Font(Hand(30)).ColorAndOpacity(InkBlue()).AutoWrapText(true).LineHeightPercentage(LinePercent(Hand(30), 30))
				]
			]
		], 24.f, FVector2D(0, 10), 0.5f), -1.f));
}

void SS47Hud::Fade(bool bOn, const FString& Text)
{
	bFadeOn = bOn;
	FadeText = Text;
}

void SS47Hud::Tick(const FGeometry& G, const double InCurrentTime, const float Dt)
{
	using namespace S47Game;
	SCompoundWidget::Tick(G, InCurrentTime, Dt);
	Now += Dt;
	// the captions: in, held to their time (on their own clock when they have one), then out
	bool bGone = false;
	for (const TSharedRef<FToast>& T : Toasts)
	{
		if (T->Out < 0.f)
		{
			T->In = FMath::Min(1.f, T->In + Dt / 0.35f);
			if (T->Until <= (T->Clock ? T->Clock() : Now)) T->Out = 0.f;
		}
		else T->Out += Dt;
		bGone |= T->Out >= 0.6f;
	}
	if (bGone) { Toasts.RemoveAll([](const TSharedRef<FToast>& T) { return T->Out >= 0.6f; }); Rebuild(); }
	FlashT = FMath::Max(0.f, FlashT - Dt);
	const float Target = bFadeOn ? 1.f : 0.f;
	FadeAlpha += (Target - FadeAlpha) * FMath::Min(1.f, Dt * (bFadeOn ? 2.5f : 1.8f));
	if (FMath::Abs(Target - FadeAlpha) < 0.01f) FadeAlpha = Target;
	ObjectiveIn = FMath::Min(1.f, ObjectiveIn + Dt / 0.6f);
	// the slip's time runs only while it can be seen
	if (SlipT >= 0.f && Playing() && !ModalOpen())
	{
		SlipT += Dt;
		if (SlipT > 4.4f) { Slips.RemoveAt(0); ShowSlip(); }
	}
	// where the player is (placeName) and which keys there are, a few times a second
	PlaceT -= Dt;
	if (PlaceT <= 0.f)
	{
		PlaceT = 0.25f;
		const US47Main* M = Main();
		Place = M && M->Flow && M->Mode == US47Main::EMode::Play ? M->Flow->PlaceName().ToUpper() : FString();
		const bool bBoard = M && bool(M->Board.Open);
		const bool bLight = M && M->Night && M->Night->Flashlight.IsValid() && M->Night->Flashlight->bHave;
		const FString Key = FString::Printf(TEXT("%d%d"), bBoard, bLight);
		if (Key != HintKey && HintRow.IsValid())
		{
			HintKey = Key;
			HintRow->ClearChildren();
			auto Add = [this](const FString& K, const TCHAR* L) { HintRow->AddSlot().AutoWidth().Padding(P(14), 0)[KeyHint(*K, L)]; };
			Add(S47Keys::Label(TEXT("notebook")).ToUpper(), TEXT("NOTEBOOK"));
			if (bBoard) Add(S47Keys::Label(TEXT("board")).ToUpper(), TEXT("CASE BOARD"));
			if (bLight) Add(S47Keys::Label(TEXT("light")).ToUpper(), TEXT("FLASHLIGHT"));
			Add(TEXT("ESC"), TEXT("PAUSE"));
		}
	}
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

namespace
{
	// text.split(/\n\s*\n/).filter((p) => p.trim()): the transcript's paragraphs, as Read description reads them
	TArray<FString> Paragraphs(const FString& Text)
	{
		TArray<FString> Out;
		static const FRegexPattern Gap(TEXT("\\n\\s*\\n"));
		FRegexMatcher Mt(Gap, Text);
		int32 From = 0;
		while (Mt.FindNext()) { const FString P = Text.Mid(From, Mt.GetMatchBeginning() - From); if (!P.TrimStartAndEnd().IsEmpty()) Out.Add(P); From = Mt.GetMatchEnding(); }
		const FString P = Text.Mid(From);
		if (!P.TrimStartAndEnd().IsEmpty()) Out.Add(P);
		return Out;
	}

	// a picture by its data URL or its art name (core/art.ts)
	UTexture2D* Pic(const FString& Name) { return Name.IsEmpty() ? nullptr : Name.StartsWith(TEXT("data:")) ? S47Image::FromDataUrl(Name) : S47Image::Art(Name); }
}

void SS47Document::Construct(const FArguments& InArgs)
{
	using namespace S47Game;
	OnClose = InArgs._OnClose;
	const FS47Doc& D = InArgs._Doc;
	US47UI* UI = InArgs._UI;
	// UI.document's readings: Kinski reads the page (Read document, without its pencil marks and rules),
	// the description reader the transcript (Read description), each when that reader is on
	if (UI)
	{
		if (!D.Page.TrimStartAndEnd().IsEmpty()) ReadDoc = UI->ReadingAction(S47Reading::DocumentReadingParagraphs(D.Page), TEXT("document"));
		ReadDesc = UI->ReadingAction(Paragraphs(D.Transcript), TEXT("description"));
	}
	// the actions: the readings (V the first), then PUT IT DOWN (Esc)
	auto AddRead = [this](const TSharedPtr<FS47ReadingAction>& R, const TCHAR* Label)
	{
		if (!R.IsValid()) return;
		const FString Key = Actions.Num() ? FString() : S47Keys::Label(TEXT("describe"));
		const FString L = Label;
		Actions.Add({ L, Key, [R] { R->Toggle(); }, [R, L] { return R->bActive ? FString(TEXT("STOP READING")) : L; }, R });
	};
	AddRead(ReadDoc, TEXT("READ ALOUD"));
	AddRead(ReadDesc, ReadDoc.IsValid() ? TEXT("READ DESCRIPTION") : TEXT("READ ALOUD"));
	Actions.Add({ TEXT("PUT IT DOWN"), TEXT("ESC"), [this] { if (OnClose) OnClose(); }, nullptr, nullptr });

	const FString KindLine = D.Kind == TEXT("hand") ? TEXT("PAPER · HANDWRITTEN") : D.Kind == TEXT("typed") ? TEXT("PAPER · TYPED") : D.Kind == TEXT("printout") ? TEXT("PRINTOUT")
		: D.Kind == TEXT("photo") ? TEXT("PHOTOGRAPH") : D.Kind == TEXT("map") ? TEXT("MAP") : TEXT("PAPER");

	TSharedRef<SVerticalBox> Btns = SNew(SVerticalBox);
	for (int32 I = 0; I < Actions.Num(); ++I)
	{
		const FAction& A = Actions[I];
		const TSharedPtr<FS47ReadingAction> R = A.Reading;
		const TFunction<FString()> Live = A.Live;
		const FString Label = A.Label;
		Btns->AddSlot().AutoHeight().Padding(0, 0, 0, P(10)).HAlign(HAlign_Left)
		[
			SNew(SButton).ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder")).ContentPadding(0).IsFocusable(false)
			.OnClicked_Lambda([this, I] { Chosen = I; Act(I); return FReply::Handled(); })
			.OnHovered_Lambda([this, I] { Chosen = I; })
			[
				SNew(SOverlay)
				// .b.sel: the amber glow fading to the right and the amber edge
				+ SOverlay::Slot()[SNew(SFadeBar).Color(C(0xffb052, 0.18f)).Visibility_Lambda([this, I] { return Chosen == I ? EVisibility::HitTestInvisible : EVisibility::Hidden; })]
				+ SOverlay::Slot().HAlign(HAlign_Left)[SNew(SBox).WidthOverride(P(3))[SNew(SBorder).BorderImage(Box(Amber())).Visibility_Lambda([this, I] { return Chosen == I ? EVisibility::HitTestInvisible : EVisibility::Hidden; })]]
				+ SOverlay::Slot().Padding(FMargin(P(14), P(6), P(60), P(6)))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Font(Term(26, 2))
						.Text_Lambda([Live, Label] { return FText::FromString(Live ? Live() : Label); })
						.ColorAndOpacity_Lambda([this, I, R] { const FLinearColor Col = Chosen == I ? Amber() : C(0xf4ecdb); return FSlateColor(R.IsValid() && !R->bAvailable && !R->bActive ? Col * FLinearColor(1, 1, 1, 0.45f) : Col); })
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(P(10), P(2), 0, 0)
					[
						SNew(STextBlock).Text(FText::FromString(A.Key)).Font(Term(20, 1)).ColorAndOpacity(Hint())
					]
				]
			]
		];
	}
	// [data-reading-status]: the chosen reading's state (the voice, Loading recording..., or why not)
	auto ChosenReading = [this]() -> TSharedPtr<FS47ReadingAction> { return Actions.IsValidIndex(Chosen) ? Actions[Chosen].Reading : nullptr; };
	Btns->AddSlot().AutoHeight().Padding(P(17), P(2), 0, 0)
	[
		SNew(STextBlock).Font(Type(14)).ColorAndOpacity(Hint()).AutoWrapText(true)
		.Text_Lambda([ChosenReading] { const TSharedPtr<FS47ReadingAction> R = ChosenReading(); return FText::FromString(R.IsValid() ? R->Status() : FString()); })
	];

	ChildSlot
	[
		SNew(SOverlay)
		// the game behind, blurred and darkened (.bg { filter: blur(5px) brightness(.32) })
		+ SOverlay::Slot()[SNew(SBackgroundBlur).BlurStrength(6.f)]
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(Box(FLinearColor(0, 0, 0, 0.68f)))]
		+ SOverlay::Slot()
		[
			SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
			[
				SNew(SBox).WidthOverride(P(1600)).HeightOverride(P(900))
				[
					SNew(SCanvas)
					+ SCanvas::Slot().Position(P2(120, 30)).Size(P2(780, 840))[PageWidget(D)]
					// .side: what it is, its title, its description and the actions
					+ SCanvas::Slot().Position(P2(960, 150)).Size(P2(440, 720))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(KindLine)).Font(Term(22, 2)).ColorAndOpacity(C(0x6f8f78))]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, P(8), 0, P(14))
						[
							SNew(STextBlock).Text(FText::FromString(D.Title)).Font(Display(40)).ColorAndOpacity(C(0xf1ecdf)).AutoWrapText(true)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).MaxDesiredHeight(P(300))
							[
								SNew(SScrollBox).Style(QuietScroll()).ScrollBarStyle(QuietBar(C(0xcfc8b7, 0.25f))).ScrollBarThickness(FVector2D(P(3), P(3)))
								+ SScrollBox::Slot().Padding(FMargin(0, 0, P(10), 0))
								[
									SNew(STextBlock).Text(FText::FromString(D.Transcript)).Font(Type(18)).ColorAndOpacity(C(0xcfc8b7)).AutoWrapText(true)
									.LineHeightPercentage(LinePercent(Type(18), 18 * 1.6f))
								]
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, P(30), 0, 0)[Btns]
					]
				]
			]
		]
	];
}

SS47Document::~SS47Document()
{
	// (UI.document's close stops what was being read from it)
	if (ReadDoc.IsValid() && ReadDoc->bActive) ReadDoc->Stop();
	if (ReadDesc.IsValid() && ReadDesc->bActive) ReadDesc->Stop();
}

TSharedRef<SWidget> SS47Document::PageWidget(const FS47Doc& D)
{
	using namespace S47Game;
	// the box the paper is laid in (mock: 600 x 800 at 300, 50, in a box 780 x 840 at 120, 30)
	const FVector2D Area(780, 840), Centre(480, 420);
	// UI.document: a photograph or a map is the picture itself, a print with a white edge; a paper may
	// carry a picture under its text (a drawing, the transit plan) and may be drawn on its own paper
	// (art: a card, a letter)
	UTexture2D* Image = Pic(D.Image);
	UTexture2D* PaperArt = Pic(D.Paper);
	auto Laid = [&](const TSharedRef<SWidget>& Sheet, FVector2D Size, float Deg) -> TSharedRef<SWidget>
	{
		return SNew(SCanvas)
			+ SCanvas::Slot().Position(P2(Centre.X - Size.X / 2, Centre.Y - Size.Y / 2)).Size(P2(Size.X, Size.Y))
			[
				Turned(Shadowed(Sheet, 70.f, FVector2D(0, 30), 0.7f), Deg)
			];
	};
	if (Image && (D.Kind == TEXT("photo") || D.Kind == TEXT("map")))
	{
		const double Ar = double(Image->GetSizeX()) / FMath::Max(1, Image->GetSizeY());
		const double Edge = D.Kind == TEXT("photo") ? 14 : 0;
		const double MaxW = 720 - 2 * Edge, MaxH = 760 - 2 * Edge;
		const double W = FMath::Min(MaxW, MaxH * Ar), H = W / Ar;
		return Laid(SNew(SBorder).BorderImage(Box(C(0xf4f1e8))).Padding(P(Edge))[SNew(SBox).WidthOverride(P(W)).HeightOverride(P(H))[SNew(SImage).Image(Picture(Image))]],
			FVector2D(W + 2 * Edge, H + 2 * Edge), -1.2f);
	}
	const bool bHand = D.Kind == TEXT("hand"), bPrint = D.Kind == TEXT("printout");
	const float Wrap = 480.f;
	TArray<FString> Lines;
	D.Page.ParseIntoArray(Lines, TEXT("\n"), false);
	// the paper's hand: Reenie Beanie in the pen's blue (as large as lets a short note fit the sheet
	// without scrolling, 37 to 31), the typewriter, or the printer's dots
	float HandPx = 37.f;
	if (bHand && FSlateApplication::IsInitialized())
	{
		const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		const float Room = 800.f - 2 * 56.f - (D.Stamp.IsEmpty() ? 0.f : 34.f);
		for (; HandPx > 31.f; HandPx -= 1.f)
		{
			float H = 0.f;
			for (const FString& L : Lines)
			{
				if (L.StartsWith(TEXT("~ "))) { H += 32.f; continue; }
				const float W = float(Measure->Measure(L, Hand(HandPx)).X) / U;
				H += HandPx * 1.08f * FMath::Max(1.f, FMath::CeilToFloat(W / (Wrap * 0.94f)));
			}
			if (H <= Room) break;
		}
	}
	const FSlateFontInfo Font = bHand ? Hand(HandPx) : bPrint ? Term(24) : Type(19);
	const float Pitch = bHand ? HandPx * 1.08f : bPrint ? 26.f : 29.f;
	const FLinearColor InkCol = bHand ? InkBlue() : bPrint ? C(0x2b2b2b) : Ink();
	TSharedRef<SVerticalBox> Col = SNew(SVerticalBox);
	if (!D.Stamp.IsEmpty()) Col->AddSlot().AutoHeight().Padding(0, 0, 0, P(10))[SNew(STextBlock).Text(FText::FromString(D.Stamp)).Font(Term(22, 2)).ColorAndOpacity(StampRed())];
	for (const FString& L : Lines)
	{
		// "~ " pencil in the margin's grey; a line of dashes is where one sheet ends and the next begins
		const bool bPencil = L.StartsWith(TEXT("~ "));
		if (L.TrimStart().StartsWith(TEXT("- - -")))
		{
			Col->AddSlot().AutoHeight().Padding(0, P(14))[SNew(SBox).HeightOverride(FMath::Max(1.f, P(1)))[SNew(SBorder).BorderImage(Box(C(0x000000, 0.22f)))]];
			continue;
		}
		const FSlateFontInfo F = bPencil ? Hand(30) : Font;
		Col->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(FText::FromString(bPencil ? L.Mid(2) : (L.IsEmpty() ? TEXT(" ") : L))).AutoWrapText(true).WrapTextAt(P(Wrap))
			.Font(F).ColorAndOpacity(bPencil ? C(0x4a4a52) : InkCol).LineHeightPercentage(LinePercent(F, bPencil ? 32.f : Pitch))
		];
	}
	if (Image)
	{
		const float W = 460.f, H = W * Image->GetSizeY() / FMath::Max(1, Image->GetSizeX());
		Col->AddSlot().AutoHeight().Padding(0, P(16), 0, 0).HAlign(HAlign_Left)[SNew(SBox).WidthOverride(P(W)).HeightOverride(P(H))[SNew(SImage).Image(Picture(Image))]];
	}
	const TSharedRef<SWidget> Text = SAssignNew(PageScroll, SScrollBox).Style(QuietScroll()).ScrollBarStyle(QuietBar(C(0x233a7a, 0.2f))).ScrollBarThickness(FVector2D(P(3), P(3)))
		+ SScrollBox::Slot().Padding(FMargin(0, 0, P(8), 0))[Col];
	if (PaperArt)
	{
		// the paper is a picture of its own (a field card, a letter): its shape from its ratio
		const double Ar = D.PaperRatio > 0 ? D.PaperRatio : double(PaperArt->GetSizeX()) / FMath::Max(1, PaperArt->GetSizeY());
		const double W = FMath::Min(740.0, 780.0 * Ar), H = W / Ar;
		return Laid(SNew(SOverlay)
			+ SOverlay::Slot()[SNew(SImage).Image(Picture(PaperArt))]
			+ SOverlay::Slot().Padding(FMargin(P(48), P(40)))[Text], FVector2D(W, H), -1.2f);
	}
	const FLinearColor Sheet = bPrint ? C(0xeef0e4) : bHand ? C(0xf1ead8) : C(0xeee7d4);
	return Laid(SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SS47GameSheet).Color(Sheet).bTorn(bHand)]
		+ SOverlay::Slot().Padding(FMargin(P(60), P(56), P(50), P(56)))[Text], FVector2D(600, 800), -1.2f);
}

void SS47Document::Act(int32 I)
{
	if (Actions.IsValidIndex(I) && Actions[I].Fn) Actions[I].Fn();
}

FReply SS47Document::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey K = InKeyEvent.GetKey();
	if (K == EKeys::Escape || K == EKeys::E || K == EKeys::Tab)
	{
		if (OnClose) OnClose();
		return FReply::Handled();
	}
	// V: the first reading; the arrows choose, Enter acts; the paper scrolls with the page keys and the wheel
	if (K == EKeys::V) { if (Actions.Num() && Actions[0].Reading.IsValid()) { Chosen = 0; Act(0); } return FReply::Handled(); }
	if (K == EKeys::Up || K == EKeys::W) { Chosen = (Chosen + Actions.Num() - 1) % FMath::Max(1, Actions.Num()); return FReply::Handled(); }
	if (K == EKeys::Down || K == EKeys::S) { Chosen = (Chosen + 1) % FMath::Max(1, Actions.Num()); return FReply::Handled(); }
	if (K == EKeys::Enter || K == EKeys::SpaceBar) { Act(Chosen); return FReply::Handled(); }
	if (PageScroll.IsValid() && (K == EKeys::PageDown || K == EKeys::PageUp))
	{
		PageScroll->SetScrollOffset(FMath::Max(0.f, PageScroll->GetScrollOffset() + (K == EKeys::PageDown ? 1.f : -1.f) * S47Game::P(300)));
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
