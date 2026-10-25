#include "SS47Widgets.h"
#include "S47Evening.h"
#include "S47Keys.h"
#include "S47Gamepad.h"

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
#include "S47Theme.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableText.h"
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
			Shadowed(SNew(SBox).MinDesiredWidth(P(400)).MaxDesiredWidth(P(520))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text_Lambda([this] { return FText::FromString(ObjectiveHead.IsEmpty() ? Place : ObjectiveHead); }).Font(Term(22, 3)).ColorAndOpacity(Amber())]
					+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT(" // "))).Font(Term(22, 3)).ColorAndOpacity(C(0x7d786d)).Visibility_Lambda([this] { return ObjectiveHead.IsEmpty() && Place.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })]
					+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text_Lambda([] { const US47Main* M = Main(); return FText::FromString(M && M->Evening && M->Evening->Commute() ? FString(TEXT("LATE AFTERNOON")) : M && M->Story ? M->Story->ClockText(false) : FString()); }).Font(Term(22, 3)).ColorAndOpacity(Amber())]
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
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, P(6), 0)[ActionKeycap(TEXT("describe"), EKey::Outline, 20)]
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
	auto PromptLine = [](TAttribute<FText> Label, FName Action, bool bMain) -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(P(2))[SNew(SBorder).BorderImage(Box(bMain ? Amber() : C(0xffb052, 0.5f)))]]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBorder).BorderImage(Box(C(0x05070d, 0.6f))).Padding(bMain ? FMargin(P(8), P(6), P(16), P(6)) : FMargin(P(8), P(3), P(14), P(3)))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, P(12), 0)[ActionKeycap(Action, bMain ? EKey::Amber : EKey::Outline, bMain ? 24 : 20)]
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
			[PromptLine(PromptAttr, TEXT("use"), true)]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, P(6), 0, 0)
		[
			SNew(SBox).Visibility_Lambda([this, PromptOn] { return PromptOn() && !AltAttr.Get().IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[PromptLine(AltAttr, TEXT("alt"), false)]
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
	if (T->Who.IsEmpty()) if (const US47Main* Mn = Main(); Mn && Mn->Story && Mn->Mode == US47Main::EMode::Play) T->Who = Mn->Evening && Mn->Evening->Commute() ? FString(TEXT("LATE AFTERNOON")) : Mn->Story->ClockText(false);
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
			SNew(STextBlock).Text(FText::FromString(T->Line)).Font(Type(Px)).ColorAndOpacity(C(0xf1ead8)).AutoWrapText(T->Line.Len() > 42).WrapTextAt(P(676)).Justification(ETextJustify::Center)
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
		const FString Key = FString::Printf(TEXT("%d%d:%u"), bBoard, bLight, S47Keys::Changes());
		if (Key != HintKey && HintRow.IsValid())
		{
			HintKey = Key;
			HintRow->ClearChildren();
			auto Add = [this](const FString& K, const TCHAR* L) { HintRow->AddSlot().AutoWidth().Padding(P(14), 0)[KeyHint(*K, L)]; };
			Add(S47Keys::Label(TEXT("notebook")).ToUpper(), TEXT("NOTEBOOK"));
			if (bBoard) Add(S47Keys::Label(TEXT("board")).ToUpper(), TEXT("CASE BOARD"));
			if (bLight) Add(S47Keys::Label(TEXT("light")).ToUpper(), TEXT("FLASHLIGHT"));
			Add(S47Keys::Label(TEXT("pause")), TEXT("PAUSE"));
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
	Btns->AddSlot().AutoHeight().Padding(P(14), P(8))
	[SNew(STextBlock).Font(Term(18)).ColorAndOpacity(Hint()).AutoWrapText(true)
	.Visibility_Lambda([] { return S47Gamepad::UsingGamepad() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
	.Text(FText::FromString(TEXT("D-PAD: ACTION   A: SELECT   X: READ   LB/RB: SCROLL   B: PUT IT DOWN")))];
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
	if (K == EKeys::Escape || K == EKeys::E || K == EKeys::Tab || K == EKeys::Gamepad_FaceButton_Right)
	{
		if (OnClose) OnClose();
		return FReply::Handled();
	}
	// V: the first reading; the arrows choose, Enter acts; the paper scrolls with the page keys and the wheel
	if (K == EKeys::V || K == EKeys::Gamepad_FaceButton_Left) { if (Actions.Num() && Actions[0].Reading.IsValid()) { Chosen = 0; Act(0); } return FReply::Handled(); }
	if (K == EKeys::Up || K == EKeys::W || K == EKeys::Gamepad_DPad_Up) { Chosen = (Chosen + Actions.Num() - 1) % FMath::Max(1, Actions.Num()); return FReply::Handled(); }
	if (K == EKeys::Down || K == EKeys::S || K == EKeys::Gamepad_DPad_Down) { Chosen = (Chosen + 1) % FMath::Max(1, Actions.Num()); return FReply::Handled(); }
	if (K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom) { Act(Chosen); return FReply::Handled(); }
	if (PageScroll.IsValid() && (K == EKeys::PageDown || K == EKeys::PageUp || K == EKeys::Gamepad_RightShoulder || K == EKeys::Gamepad_LeftShoulder))
	{
		PageScroll->SetScrollOffset(FMath::Max(0.f, PageScroll->GetScrollOffset() + (K == EKeys::PageDown || K == EKeys::Gamepad_RightShoulder ? 1.f : -1.f) * S47Game::P(300)));
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ---------- a CRT's pieces (style.css: .crt, .kbtn, its range inputs, .refcard, .termview) ----------

namespace
{
	// --crt-bg, --phosphor, --phosphor-dim and the .crt's other greens
	FLinearColor RxBg(float A = 1.f) { return S47Theme::Css(0x021006, A); }
	FLinearColor RxPhos(float A = 1.f) { return S47Theme::Css(0x8cffa4, A); }
	FLinearColor RxDim(float A = 1.f) { return S47Theme::Css(0x2f8a49, A); }
	FLinearColor RxGlow() { return S47Theme::Css(0x5aff82, 0.55f); }   // text-shadow: 0 0 6px rgba(90,255,130,.55)
	FLinearColor RxWarn() { return S47Theme::Css(0xffd27a); }           // .status.warn
	FLinearColor RxWarnGlow() { return S47Theme::Css(0xffbe50, 0.6f); }

	// focus that came from the keys (Tab, the arrows), any user's: on the glass the screen's virtual
	// user has it, not the keyboard's own (.kbtn:focus-visible)
	bool RxKeyed(const SWidget& W)
	{
		const TOptional<EFocusCause> C = W.HasAnyUserFocus();
		return C.IsSet() && C.GetValue() == EFocusCause::Navigation;
	}

	TAttribute<FText> RxStr(TFunction<FString()> F) { return TAttribute<FText>::CreateLambda([F] { return FText::FromString(F()); }); }

	// a rectangle and its rim in plain colour boxes: a rounded box brush comes out as a solid pale
	// block in a render target's picture, so nothing drawn for the glass uses one
	void RxRect(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& P, const FVector2f& S, const FLinearColor& Fill, const FLinearColor& Line = FLinearColor::Transparent, float W = 0.f)
	{
		if (Fill.A > 0.f) S47Theme::Fill(Out, Layer, G, P, S, Fill);
		if (Line.A <= 0.f) return;
		if (W <= 0.f) W = S47Theme::Px(1);
		S47Theme::Fill(Out, Layer, G, P, FVector2f(S.X, W), Line);
		S47Theme::Fill(Out, Layer, G, FVector2f(P.X, P.Y + S.Y - W), FVector2f(S.X, W), Line);
		S47Theme::Fill(Out, Layer, G, FVector2f(P.X, P.Y + W), FVector2f(W, S.Y - 2 * W), Line);
		S47Theme::Fill(Out, Layer, G, FVector2f(P.X + S.X - W, P.Y + W), FVector2f(W, S.Y - 2 * W), Line);
	}

	// a thin rim round its slot (the canvas's border: 1px solid rgba(140,255,164,.25))
	class SRxRim : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SRxRim) {}
			SLATE_ARGUMENT(FLinearColor, Color)
		SLATE_END_ARGS()
		void Construct(const FArguments& A) { Color = A._Color; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
		{
			RxRect(Out, Layer, G, FVector2f(0, 0), FVector2f(G.GetLocalSize()), FLinearColor::Transparent, Color);
			return Layer;
		}

	private:
		FLinearColor Color;
	};

	// a line of the screen's text, glowing
	TSharedRef<SS47GlowText> RxText(TAttribute<FText> Text, float Px, TAttribute<FLinearColor> Color = RxPhos(), TAttribute<FLinearColor> Glow = RxGlow(), float Spacing = 0.f)
	{
		return SNew(SS47GlowText).Text(Text).Font(S47Theme::Term(Px, Spacing)).Color(Color).Glow(Glow);
	}

	// .kbtn: a key on the screen, phosphor on a faint green in a thin rim; :hover lights it, held it
	// sinks a pixel, opacity .4 when it cannot be used (:disabled); Enter or Space presses it
	class SRxKey : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SRxKey) : _Px(22.f), _MinHeight(40.f), _bPrimary(false), _Usable(true) {}
			SLATE_ATTRIBUTE(FString, Label)
			SLATE_ARGUMENT(float, Px)
			SLATE_ARGUMENT(float, MinHeight)
			SLATE_ARGUMENT(bool, bPrimary)
			SLATE_ATTRIBUTE(bool, Usable)
			SLATE_ARGUMENT(TFunction<void()>, OnClick)
		SLATE_END_ARGS()
		void Construct(const FArguments& A)
		{
			Usable = A._Usable;
			OnClick = A._OnClick;
			bPrimary = A._bPrimary;
			const TAttribute<FString> L = A._Label;
			// .kbtn { padding: 4px 12px; min-height: 40px; min-width: 52px } .primary { padding: 6px 20px }
			ChildSlot
			[
				SNew(SBox).MinDesiredWidth(S47Theme::Px(52)).MinDesiredHeight(S47Theme::Px(A._MinHeight)).HAlign(HAlign_Center).VAlign(VAlign_Center)
				.Padding(bPrimary ? S47Theme::Pad(20, 6) : S47Theme::Pad(12, 4))
				.RenderTransform_Lambda([this] { return bDown && IsHovered() ? FSlateRenderTransform(FVector2f(0.f, S47Theme::Px(1))) : TOptional<FSlateRenderTransform>(); })
				[
					SNew(SS47GlowText).Text(RxStr([L] { return L.Get(); })).Font(S47Theme::Term(A._Px)).Color(RxPhos()).Glow(RxGlow())
				]
			];
		}
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bEnabled) const override
		{
			const bool bOn = Usable.Get(true);
			const bool bHot = bOn && (IsHovered() || RxKeyed(*this));
			const bool bIn = bOn && bDown && IsHovered();
			const float A = bOn ? 1.f : 0.4f;
			// background: rgba(40,140,70,.08); :hover rgba(80,255,130,.16); border 1px var(--phosphor-dim),
			// .primary var(--phosphor)
			// (a little brighter than the page's .16 on hover: the glass is seen from further off)
			FLinearColor Fill = bIn ? S47Theme::Css(0x50ff82, 0.34f) : bHot ? S47Theme::Css(0x50ff82, 0.2f) : S47Theme::Css(0x288c46, 0.08f);
			FLinearColor Line = bPrimary || bHot ? RxPhos() : RxDim();
			Fill.A *= A;
			Line.A *= A;
			const FVector2f Sz(G.GetLocalSize());
			RxRect(Out, Layer, G, FVector2f(0, 0), Sz, Fill, Line);
			// the keys' choice: a second rim, inside
			if (bOn && RxKeyed(*this))
			{
				const float In = S47Theme::Px(3);
				RxRect(Out, Layer, G, FVector2f(In, In), Sz - FVector2f(2 * In, 2 * In), FLinearColor::Transparent, RxPhos(0.5f));
			}
			FWidgetStyle S = Style;
			S.BlendColorAndOpacityTint(FLinearColor(1, 1, 1, A));
			return SCompoundWidget::OnPaint(Args, G, Cull, Out, Layer + 1, S, bEnabled);
		}
		virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent& E) override
		{
			if (E.GetEffectingButton() != EKeys::LeftMouseButton || !Usable.Get(true)) return FReply::Unhandled();
			bDown = true;
			return FReply::Handled().CaptureMouse(SharedThis(this));
		}
		virtual FReply OnMouseButtonDoubleClick(const FGeometry& G, const FPointerEvent& E) override { return OnMouseButtonDown(G, E); }
		virtual FReply OnMouseButtonUp(const FGeometry& G, const FPointerEvent& E) override
		{
			if (E.GetEffectingButton() != EKeys::LeftMouseButton || !bDown) return FReply::Unhandled();
			bDown = false;
			const bool bClick = Usable.Get(true) && G.IsUnderLocation(E.GetScreenSpacePosition());
			FReply R = FReply::Handled().ReleaseMouseCapture();
			if (bClick && OnClick) OnClick();
			return R;
		}
		virtual void OnMouseCaptureLost(const FCaptureLostEvent&) override { bDown = false; }
		virtual FCursorReply OnCursorQuery(const FGeometry&, const FPointerEvent&) const override { return Usable.Get(true) ? FCursorReply::Cursor(EMouseCursor::Hand) : FCursorReply::Unhandled(); }
		virtual bool SupportsKeyboardFocus() const override { return Usable.Get(true); }
		virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& E) override
		{
			const FKey K = E.GetKey();
			if ((K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom) && Usable.Get(true))
			{
				if (OnClick) OnClick();
				return FReply::Handled();
			}
			return FReply::Unhandled();
		}

	private:
		TAttribute<bool> Usable;
		TFunction<void()> OnClick;
		bool bPrimary = false, bDown = false;
	};

	// .crt input[type=range]: a 4 px track in the dim phosphor and a 22 x 30 thumb with its glow
	// (box-shadow: 0 0 8px rgba(90,255,130,.7)); a click or a drag sets it, Left and Right step it
	// when the keys have it (Up and Down move between the controls)
	class SRxSlider : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SRxSlider) : _Step(0.01f), _Usable(true) {}
			SLATE_ATTRIBUTE(float, Value)
			SLATE_ARGUMENT(float, Step)
			SLATE_ATTRIBUTE(bool, Usable)
			SLATE_ARGUMENT(TFunction<void(float)>, OnChanged)
		SLATE_END_ARGS()
		void Construct(const FArguments& A) { Value = A._Value; Step = A._Step; Usable = A._Usable; OnChanged = A._OnChanged; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(S47Theme::Px(120), S47Theme::Px(36)); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool) const override
		{
			const FVector2f S(G.GetLocalSize());
			const float Tw = S47Theme::Px(22), Th = S47Theme::Px(30), Tk = S47Theme::Px(4), A = Style.GetColorAndOpacityTint().A;
			const float X = Tw / 2 + FMath::Clamp(Value.Get(0.f), 0.f, 1.f) * FMath::Max(0.f, S.X - Tw);
			const bool bHot = Usable.Get(true) && (IsHovered() || HasMouseCapture() || RxKeyed(*this));
			S47Theme::Fill(Out, Layer, G, FVector2f(0, (S.Y - Tk) / 2), FVector2f(S.X, Tk), RxDim(A));
			for (int32 i = 3; i >= 1; --i)
			{
				const float R = S47Theme::Px(8) * i / 3.f;
				RxRect(Out, Layer + 1, G, FVector2f(X - Tw / 2 - R, (S.Y - Th) / 2 - R), FVector2f(Tw + 2 * R, Th + 2 * R), S47Theme::Css(0x5aff82, (bHot ? 0.16f : 0.11f) * A));
			}
			RxRect(Out, Layer + 2, G, FVector2f(X - Tw / 2, (S.Y - Th) / 2), FVector2f(Tw, Th), bHot ? S47Theme::Css(0xc8ffd2, A) : RxPhos(A));
			if (RxKeyed(*this))
			{
				const float In = S47Theme::Px(4);
				RxRect(Out, Layer + 2, G, FVector2f(X - Tw / 2 - In, (S.Y - Th) / 2 - In), FVector2f(Tw + 2 * In, Th + 2 * In), FLinearColor::Transparent, RxPhos(0.8f * A));
			}
			return Layer + 3;
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
		virtual bool SupportsKeyboardFocus() const override { return Usable.Get(true); }
		virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& E) override
		{
			const int32 D = E.GetKey() == EKeys::Left || E.GetKey() == EKeys::Gamepad_DPad_Left ? -1 : E.GetKey() == EKeys::Right || E.GetKey() == EKeys::Gamepad_DPad_Right ? 1 : 0;
			if (!D || !Usable.Get(true)) return FReply::Unhandled();
			if (OnChanged) OnChanged(FMath::Clamp(Value.Get(0.f) + D * Step, 0.f, 1.f));
			return FReply::Handled();
		}

	private:
		TAttribute<float> Value;
		TAttribute<bool> Usable;
		float Step = 0.01f;
		TFunction<void(float)> OnChanged;
		void Set(const FGeometry& G, const FPointerEvent& E)
		{
			const float Tw = S47Theme::Px(22), W = FMath::Max(1.f, float(G.GetLocalSize().X) - Tw);
			if (OnChanged) OnChanged(FMath::Clamp((float(G.AbsoluteToLocal(E.GetScreenSpacePosition()).X) - Tw / 2) / W, 0.f, 1.f));
		}
	};

	// .refcard: the taped reference card (Special Elite on paper, turned -.6 degrees, two strips of
	// tape over its top edge, a shadow) or, .refcard.screen, the same words in the phosphor. The
	// words are a plain STextBlock the description reader reads.
	class SRxCard : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SRxCard) {}
			SLATE_ATTRIBUTE(FString, Text)
			SLATE_ATTRIBUTE(bool, Taped)
		SLATE_END_ARGS()
		void Construct(const FArguments& A)
		{
			Text = A._Text;
			Taped = A._Taped;
			SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			SetRenderTransform(TAttribute<TOptional<FSlateRenderTransform>>::CreateSP(this, &SRxCard::Turn));
			Build();
		}
		virtual void Tick(const FGeometry&, const double, const float) override { if (Taped.Get(false) != bTaped) Build(); }
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bEnabled) const override
		{
			const FVector2f S(G.GetLocalSize());
			if (bTaped)
			{
				// box-shadow: 0 3px 10px rgba(0,0,0,.5); background: var(--paper)
				for (int32 i = 3; i >= 1; --i)
				{
					const float R = S47Theme::Px(3) * i;
					RxRect(Out, Layer, G, FVector2f(-R, S47Theme::Px(3) - R), S + FVector2f(2 * R, 2 * R), FLinearColor(0, 0, 0, 0.5f / 4.f));
				}
				S47Theme::Fill(Out, Layer + 1, G, FVector2f(0, 0), S, S47Theme::Paper());
			}
			int32 L = SCompoundWidget::OnPaint(Args, G, Cull, Out, Layer + 2, Style, bEnabled);
			if (bTaped)
			{
				// ::before, ::after: tape 64 x 18 at top -8px, 18px in from each side, turned -4 and 5 degrees
				const FVector2f T(S47Theme::Px(64), S47Theme::Px(18));
				for (int32 i = 0; i < 2; ++i)
				{
					const float X = i == 0 ? S47Theme::Px(18) : S.X - S47Theme::Px(18) - T.X;
					FSlateDrawElement::MakeRotatedBox(Out, L + 1, G.ToPaintGeometry(T, FSlateLayoutTransform(FVector2f(X, -S47Theme::Px(8)))), S47Theme::White(), ESlateDrawEffect::None,
						FMath::DegreesToRadians(i == 0 ? -4.f : 5.f), TOptional<FVector2f>(), FSlateDrawElement::RelativeToElement, S47Theme::Css(0xe6dcb4, 0.75f));
				}
				++L;
			}
			return L;
		}

	private:
		TAttribute<FString> Text;
		TAttribute<bool> Taped;
		bool bTaped = false;
		TOptional<FSlateRenderTransform> Turn() const { return bTaped ? FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(-0.6f))) : TOptional<FSlateRenderTransform>(); }
		void Build()
		{
			bTaped = Taped.Get(false);
			const TAttribute<FText> Words = RxStr([T = Text] { return T.Get(); });
			TSharedRef<SWidget> Body = bTaped
				? S47Kit::Readable(SNew(STextBlock).Text(Words).Font(S47Theme::Type(15)).ColorAndOpacity(S47Theme::Ink()).AutoWrapText(true).LineHeightPercentage(1.25f))
				: StaticCastSharedRef<SWidget>(SNew(SOverlay)
					+ SOverlay::Slot()[SNew(SS47GlowText).Text(Words).Font(S47Theme::Term(20)).Color(RxPhos()).Glow(RxGlow()).AutoWrapText(true).bHaloOnly(true)]
					+ SOverlay::Slot()[S47Kit::Readable(SNew(STextBlock).Text(Words).Font(S47Theme::Term(20)).ColorAndOpacity(RxPhos()).AutoWrapText(true))]);
			// (max-width: 640px; padding 10px 14px on paper, 4px 0 on the screen)
			ChildSlot
			[
				SNew(SBox).WidthOverride(S47Theme::Px(640)).Padding(bTaped ? S47Theme::Pad(14, 10) : S47Theme::Pad(0, 4))[Body]
			];
		}
	};

	// a box-shadow's inset blur along a rectangle's four edges: half the colour at the edge, gone at W
	void RxInset(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& P, const FVector2f& S, float W, const FLinearColor& C)
	{
		auto At = [&C](float K) { FLinearColor X = C; X.A *= K; return X; };
		const TArray<TPair<float, FLinearColor>> In = { { 0.f, At(0.5f) }, { 0.25f, At(0.3f) }, { 0.5f, At(0.14f) }, { 0.75f, At(0.05f) }, { 1.f, At(0.f) } };
		TArray<TPair<float, FLinearColor>> Back;
		for (const TPair<float, FLinearColor>& Stop : In) Back.Insert(TPair<float, FLinearColor>(1.f - Stop.Key, Stop.Value), 0);
		S47Theme::Gradient(Out, Layer, G, P, FVector2f(S.X, W), In, false);
		S47Theme::Gradient(Out, Layer, G, FVector2f(P.X, P.Y + S.Y - W), FVector2f(S.X, W), Back, false);
		S47Theme::Gradient(Out, Layer, G, P, FVector2f(W, S.Y), In, true);
		S47Theme::Gradient(Out, Layer, G, FVector2f(P.X + S.X - W, P.Y), FVector2f(W, S.Y), Back, true);
	}
}

void SS47CrtGlass::Construct(const FArguments& InArgs)
{
	bBezel = InArgs._bBezel;
	Radius = InArgs._Radius;
	Ring = InArgs._Ring;
	Edge = InArgs._Edge;
	Scan = InArgs._Scan;
	ChildSlot.Padding(bBezel ? (Ring + Edge) * S47Theme::U : 0.f)[InArgs._Content.Widget];
}

FSlateRect SS47CrtGlass::Glass(const FGeometry& G) const
{
	const float B = bBezel ? (Ring + Edge) * S47Theme::U : 0.f;
	const FVector2f S(G.GetLocalSize());
	return FSlateRect(B, B, S.X - B, S.Y - B);
}

int32 SS47CrtGlass::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	S47_USING_THEME
	const FVector2f S(G.GetLocalSize());
	const FSlateRect R = Glass(G);
	const FVector2f P0(R.Left, R.Top), Sz(R.GetSize());
	const float Rad = Radius * U;
	int32 L = Layer;
	if (bBezel)
	{
		// box-shadow: 0 0 0 14px #c9bfa1, 0 0 0 16px #8d8570, 0 20px 60px rgba(0,0,0,.8)
		Shadow(Out, L, G, FVector2f(0, 0), S, Px(20), Px(40), 0.8f);
		FSlateDrawElement::MakeBox(Out, L + 1, G.ToPaintGeometry(), Box(Css(0x8d8570), FLinearColor::Transparent, 0.f, Rad + (Ring + Edge) * U), ESlateDrawEffect::None, FLinearColor::White);
		FSlateDrawElement::MakeBox(Out, L + 2, G.ToPaintGeometry(S - FVector2f(2 * Edge * U, 2 * Edge * U), FSlateLayoutTransform(FVector2f(Edge * U, Edge * U))),
			Box(Css(0xc9bfa1), FLinearColor::Transparent, 0.f, Rad + Ring * U), ESlateDrawEffect::None, FLinearColor::White);
		L += 3;
	}
	// background: var(--crt-bg); box-shadow: inset 0 0 90px rgba(0,0,0,.85), inset 0 0 18px rgba(60,255,110,.12)
	// (a plain box, the corners masked below: a render target's picture has to be opaque everywhere)
	FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(Sz, FSlateLayoutTransform(P0)), White(), ESlateDrawEffect::None, RxBg());
	RxInset(Out, L + 1, G, P0, Sz, Px(90), FLinearColor(0, 0, 0, 0.85f));
	RxInset(Out, L + 1, G, P0, Sz, Px(18), Css(0x3cff6e, 0.12f));
	L = SCompoundWidget::OnPaint(Args, G, Cull, Out, L + 2, Style, bParentEnabled) + 1;
	// ::after: repeating-linear-gradient(rgba(0,0,0,.22) 0 1px, transparent 1px 3px), in the pixels of
	// whatever this is drawn into (the screen, or the CRT's render target: there a line in four, a
	// pixel and a half thick, so the tube's lines survive being drawn smaller on the glass)
	const float Dev = 1.f / FMath::Max(0.1f, G.Scale);
	const bool bTarget = !bBezel;
	const float Pitch = (bTarget ? 4.f : 3.f) * Dev, Line = (bTarget ? 1.5f : 1.f) * Dev;
	for (float Y = P0.Y; Y < R.Bottom; Y += Pitch)
	{
		const float Dy = Y < P0.Y + Rad ? P0.Y + Rad - Y : Y > R.Bottom - Rad ? Y - (R.Bottom - Rad) : 0.f;
		const float Dx = Dy > 0.f ? Rad - FMath::Sqrt(FMath::Max(0.f, Rad * Rad - Dy * Dy)) : 0.f;
		Fill(Out, L, G, FVector2f(P0.X + Dx, Y), FVector2f(Sz.X - 2 * Dx, Line), FLinearColor(0, 0, 0, Scan));
	}
	// the glass's round corners over what the edges' shading drew square
	const FLinearColor Corner = bBezel ? Css(0xc9bfa1) : FLinearColor(0.f, 0.f, 0.f, 1.f);
	const FVector2f Corners[4] = { P0, FVector2f(R.Right, R.Top), FVector2f(R.Right, R.Bottom), FVector2f(R.Left, R.Bottom) };
	for (int32 c = 0; c < 4; ++c)
	{
		const FVector2f In(c == 0 || c == 3 ? Rad : -Rad, c < 2 ? Rad : -Rad);
		const FVector2f Ctr = Corners[c] + In;
		const float A0 = FMath::Atan2(-In.Y, -In.X);
		for (int32 k = 0; k < 8; ++k)
		{
			const float Aa = A0 - UE_PI / 4.f + UE_PI / 2.f * k / 8.f, Ab = A0 - UE_PI / 4.f + UE_PI / 2.f * (k + 1) / 8.f;
			Triangle(Out, L + 1, G, Corners[c], Ctr + FVector2f(FMath::Cos(Aa), FMath::Sin(Aa)) * Rad, Ctr + FVector2f(FMath::Cos(Ab), FMath::Sin(Ab)) * Rad, Corner);
		}
	}
	return L + 2;
}

// ---------- the spectrum ----------

void SS47Spectrum::Construct(const FArguments& InArgs)
{
	Story = InArgs._Story;
	Size = InArgs._Size;
	Canvas = InArgs._Canvas;
	WhiteBrush = MakeUnique<FSlateColorBrush>(FLinearColor::White);
}

void SS47Spectrum::Paint(const AS47Story* Story, const FGeometry& G, FSlateWindowElementList& Out, int32 Layer, const FSlateBrush* White, FVector2D Canvas)
{
	// Signal.ts drawSpectrum, in its canvas's pixels, the colours as the canvas has them (sRGB)
	const FVector2f S = FVector2f(G.GetLocalSize());
	const FVector2f C = Canvas.X > 0 && Canvas.Y > 0 ? FVector2f(Canvas) : S;
	const FVector2f K(S.X / C.X, S.Y / C.Y);
	FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(), White, ESlateDrawEffect::None, S47Theme::Css(0x021006));
	if (!Story) return;
	FS47SpectrumGeometry Geo;
	const double T = FPlatformTime::Seconds();
	S47Spectrum::Build(Story->Rx, T, Story->bResidualVisible, C.X, C.Y, [] { return double(FMath::FRand()); }, Geo);
	const FLinearColor Grid = S47Theme::Css(0x3cb45f, 0.22f);
	for (float X : Geo.GridX) FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(X * K.X, 0), FVector2f(X * K.X, S.Y) }, ESlateDrawEffect::None, Grid, false, 1.f);
	for (float Y : Geo.GridY) FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(0, Y * K.Y), FVector2f(S.X, Y * K.Y) }, ESlateDrawEffect::None, Grid, false, 1.f);
	if (!Geo.bPowered) return;
	FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(FVector2f((Geo.BandX1 - Geo.BandX0) * K.X, S.Y), FSlateLayoutTransform(FVector2f(Geo.BandX0 * K.X, 0))), White, ESlateDrawEffect::None, S47Theme::Css(0x46ff78, 0.08f));
	if (Geo.Trace.Num() > 1)
	{
		TArray<FVector2f> Pts;
		Pts.Reserve(Geo.Trace.Num());
		for (const FVector2f& P : Geo.Trace) Pts.Add(FVector2f(P.X * K.X, P.Y * K.Y));
		// lineWidth max(1.5, w / 340), shadowBlur 6 in rgba(90,255,130,.8): the glow as wider, fainter
		// strokes under the line
		const float W = FMath::Max(1.5f, C.X / 340.f) * FMath::Sqrt(K.X * K.Y);
		static const float Halo[3][2] = { { 5.f, 0.07f }, { 3.2f, 0.12f }, { 1.9f, 0.22f } };
		for (const auto& H : Halo) FSlateDrawElement::MakeLines(Out, Layer + 3, G.ToPaintGeometry(), Pts, ESlateDrawEffect::None, S47Theme::Css(0x5aff82, H[1]), true, W * H[0]);
		FSlateDrawElement::MakeLines(Out, Layer + 4, G.ToPaintGeometry(), Pts, ESlateDrawEffect::None, S47Theme::Css(0x8cffa4), true, W);
	}
	// the tuned frequency: strokeStyle rgba(255,200,90,.85), setLineDash([3, 3])
	const float Dash = 3.f * K.Y, Tx = Geo.TunedX * K.X;
	for (float Y = 0; Y < S.Y; Y += 2 * Dash)
	{
		FSlateDrawElement::MakeLines(Out, Layer + 5, G.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(Tx, Y), FVector2f(Tx, FMath::Min(S.Y, Y + Dash)) }, ESlateDrawEffect::None, S47Theme::Css(0xffc85a, 0.85f), false, FMath::Max(1.f, K.X));
	}
}

int32 SS47Spectrum::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	Paint(Story, G, Out, Layer, WhiteBrush.Get(), Canvas);
	return Layer + 6;
}

// ---------- the receiver console ----------

bool SS47Console::Usable() const { return Story && Story->Rx.Stage < 3 && !Story->SurveyOnly(); }

TSharedRef<SWidget> SS47Console::Control(const TCHAR* Label, TFunction<double()> Get, TFunction<void(double)> Set, double Min, double Max, TFunction<FString(double)> Fmt, bool bFine)
{
	S47_USING_THEME
	// .ctl: grid-template-columns minmax(170px, 230px) 1fr auto; gap 8px 14px; margin 6px 0
	TSharedRef<SHorizontalBox> Fine = SNew(SHorizontalBox);
	if (bFine)
	{
		int32 i = 0;
		for (double Step : { -0.010, -0.001, 0.001, 0.010 })
		{
			const FString L = Step < 0 ? (Step < -0.005 ? TEXT("--") : TEXT("-")) : (Step > 0.005 ? TEXT("++") : TEXT("+"));
			// (RxConsole.ts: the fine keys are never disabled, only the ranges)
			Fine->AddSlot().AutoWidth().Padding(i++ ? Px(6) : 0.f, 0, 0, 0)
			[
				SNew(SRxKey).Label(L).OnClick([this, Get, Set, Step, Min, Max] { Set(FMath::Clamp(FMath::RoundToDouble((Get() + Step) * 1000.0) / 1000.0, Min, Max)); if (OnClick) OnClick(); })
			];
		}
	}
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, Px(6))
		[
			SNew(SBox).WidthOverride(Px(230))
			[
				RxText(RxStr([Label, Get, Fmt] { return FString::Printf(TEXT("%s: %s"), Label, *Fmt(Get())); }), 22.f)
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(Px(14), 0, 0, 0)
		[
			SNew(SRxSlider)
			.Value_Lambda([Get, Min, Max] { return float((Get() - Min) / (Max - Min)); })
			.Step(float((bFine ? 0.001 : 1.0) / (Max - Min)))
			.Usable_Lambda([this] { return Usable(); })
			.OnChanged([Set, Min, Max, bFine](float V) { const double R = Min + V * (Max - Min); Set(bFine ? FMath::RoundToDouble(R * 1000.0) / 1000.0 : FMath::RoundToDouble(R)); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(Px(14), 0, 0, 0)[Fine];
}

void SS47Console::Construct(const FArguments& InArgs)
{
	S47_USING_THEME
	Story = InArgs._Story;
	OnClose = InArgs._OnClose;
	OnClick = InArgs._OnClick;
	FS47Rx* Rx = Story ? &Story->Rx : nullptr;
	auto Az = [this] { if (Story) Story->ConsoleAzimuth(); };
	auto Warn = [this] { return Story && (Story->Rx.Status.StartsWith(TEXT("NO LOCK")) || Story->Rx.Status.StartsWith(TEXT("UNLOGGED"))); };
	const TAttribute<FLinearColor> StatusColor = TAttribute<FLinearColor>::CreateLambda([Warn] { return Warn() ? RxWarn() : S47Theme::Css(0xc8ffd2); });
	const TAttribute<FLinearColor> StatusGlow = TAttribute<FLinearColor>::CreateLambda([Warn] { return Warn() ? RxWarnGlow() : RxGlow(); });
	// .axis: the five frequencies, spread out (justify-content: space-between)
	TSharedRef<SHorizontalBox> Axis = SNew(SHorizontalBox);
	static const TCHAR* Ticks[] = { TEXT("1419.5"), TEXT("1419.8"), TEXT("1420.1"), TEXT("1420.4"), TEXT("1420.7 MHz") };
	for (int32 i = 0; i < 5; ++i)
	{
		if (i) Axis->AddSlot().FillWidth(1.f)[SNullWidget::NullWidget];
		Axis->AddSlot().AutoWidth()[RxText(FText::FromString(Ticks[i]), 17.f, RxDim())];
	}
	TSharedPtr<SVerticalBox> LeftCol;
	ChildSlot
	.Padding(Pad(26, 22, 26, 20))
	.VAlign(VAlign_Center)
	[
		SNew(SVerticalBox)
		// .lcol
		+ SVerticalBox::Slot().AutoHeight()
		[
			SAssignNew(LeftCol, SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)[RxText(FText::FromString(TEXT("SARO / RX CONTROL 03")), 30.f, RxPhos(), RxGlow(), 1.2f)]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					RxText(RxStr([this] { const double Q = !Story ? 0 : Story->Rx.Stage >= 3 ? 1.0 : Story->SurveyOnly() ? 0.0 : Story->Rx.Quality(); return FString::Printf(TEXT("CARRIER %d%%"), FMath::RoundToInt(Q * 100)); }), 22.f)
				]
			]
			// .status: the receiver's line, with the terminal's cursor after it
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox).MinDesiredHeight(Px(26))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[RxText(RxStr([this] { return Story ? Story->Rx.Status : FString(); }), 22.f, StatusColor, StatusGlow)]
					+ SHorizontalBox::Slot().AutoWidth()[RxText(RxStr([] { return FMath::Fmod(FPlatformTime::Seconds(), 1.06) < 0.53 ? FString(TEXT("_")) : FString(TEXT(" ")); }), 22.f, StatusColor, StatusGlow)]
				]
			]
			// the canvas, 900 x 300, shown 210 px high in a thin rim (border: 1px solid rgba(140,255,164,.25))
			+ SVerticalBox::Slot().AutoHeight().Padding(0, Px(10), 0, Px(6))
			[
				SNew(SBox).HeightOverride(Px(210))
				[
					SNew(SOverlay)
					+ SOverlay::Slot()[SNew(SS47Spectrum).Story(Story).Size(FVector2D(Px(900), Px(210))).Canvas(FVector2D(900, 300))]
					+ SOverlay::Slot()[SNew(SRxRim).Color(RxPhos(0.25f)).Visibility(EVisibility::HitTestInvisible)]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, Px(8))[Axis]
			// (.refcard: read as the receiver's description, UI.open's descriptionReadings)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0, Px(8), 0, Px(12))
			[
				SNew(SRxCard)
				.Text_Lambda([this]
				{
					if (!Story) return FString();
					if (Story->Rx.Stage >= 3) return FString(TEXT("Pattern repeats every 5.2 s: four pulses, a gap, seven pulses. Run a direction solve on the tracking terminal."));
					if (Story->SurveyOnly()) return FString(TEXT("Survey runs unattended. Nothing scheduled until 06:00."));
					return Story->Rx.Profile().Card;
				})
				.Taped_Lambda([this] { return Usable() && Story->Rx.Profile().bCardIsTaped; })
			]
		]
		// .rcol
		+ SVerticalBox::Slot().AutoHeight()[Control(TEXT("FREQUENCY MHz"), [Rx] { return Rx ? Rx->Frequency : 0.0; }, [Rx](double V) { if (Rx) Rx->Frequency = V; }, S47Signal::FMin, S47Signal::FMax, [](double V) { return FString::Printf(TEXT("%.3f"), V); }, true)]
		+ SVerticalBox::Slot().AutoHeight()[Control(TEXT("GAIN"), [Rx] { return Rx ? Rx->Gain : 0.0; }, [Rx](double V) { if (Rx) Rx->Gain = V; }, 0, 100, [](double V) { return FString::Printf(TEXT("%.0f"), V); }, false)]
		+ SVerticalBox::Slot().AutoHeight()[Control(TEXT("BANDWIDTH kHz"), [Rx] { return Rx ? Rx->Bandwidth : 0.0; }, [Rx](double V) { if (Rx) Rx->Bandwidth = V; }, 4, 100, [](double V) { return FString::Printf(TEXT("%.0f"), V); }, false)]
		+ SVerticalBox::Slot().AutoHeight()[Control(TEXT("ARRAY AZ"), [Rx] { return Rx ? Rx->Azimuth : 0.0; }, [Rx, Az](double V) { if (Rx) { Rx->Azimuth = V; Az(); } }, 0, 180, [](double V) { return S47Text::Pad(V, 3); }, false)]
		// .actions: the action, the readout, EXIT
		+ SVerticalBox::Slot().AutoHeight().Padding(0, Px(14), 0, 0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SRxKey).bPrimary(true).Px(26.f)
				.Label_Lambda([this] { return FString(!Story ? TEXT("") : Story->Rx.Stage >= 3 ? TEXT("LOCKED") : Story->SurveyOnly() ? TEXT("SWEEP RUNNING") : *Story->Rx.Profile().Action); })
				.Usable_Lambda([this] { return Usable(); })
				.OnClick([this] { if (OnClick) OnClick(); if (Story) Story->ConsoleAction(); })
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(Px(12), 0)
			[
				RxText(RxStr([this] { return FString(Story && Story->Rx.Stage >= 3 ? TEXT("SOURCE: UNKNOWN   1420.405 MHz\nS/N 4.71   DIRECTION: NOT SOLVED") : TEXT("")); }), 19.f, Css(0xb9ffc7))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SRxKey).Label(FString(TEXT("EXIT"))).OnClick([this] { if (OnClick) OnClick(); if (OnClose) OnClose(); })
			]
		]
	];
	if (LeftCol.IsValid()) LeftCol->AddSlot().AutoHeight()
	[RxText(RxStr([] { return S47Gamepad::UsingGamepad() ? FString(TEXT("D-PAD: CONTROLS / ADJUST   A: PRESS   B: EXIT   RIGHT STICK: POINTER")) : FString(); }), 17.f, RxDim())];
	// the receiver's card can be read as its description (receiver-description), the box under it in .lcol
	if (US47UI* UI = US47Main::Get(nullptr)->UI; UI && LeftCol.IsValid())
		LeftCol->AddSlot().AutoHeight()[UI->DescriptionReadings(LeftCol.ToSharedRef(), TEXT("receiver-description"))];
}

FReply SS47Console::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right)
	{
		if (OnClose) OnClose();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ---------- the operations terminal ----------

void SS47Ops::Construct(const FArguments& InArgs)
{
	S47_USING_THEME
	Story = InArgs._Story;
	OnClose = InArgs._OnClose;
	Text = (Story ? Story->Ops().Header() : FString()) + TEXT("\n");
	// .term: VT323 22px, the phosphor, text-shadow 0 0 6px rgba(140,255,164,.45); the line takes the
	// keys (caret-color: var(--phosphor))
	const FLinearColor Glow = Css(0x8cffa4, 0.45f);
	static const FEditableTextStyle LineStyle = []
	{
		FEditableTextStyle S = FCoreStyle::Get().GetWidgetStyle<FEditableTextStyle>(TEXT("NormalEditableText"));
		S.SetFont(S47Theme::Term(22.f)).SetColorAndOpacity(RxPhos());
		S.SetCaretImage(FSlateColorBrush(RxPhos())).SetBackgroundImageSelected(FSlateColorBrush(RxPhos(0.3f)));
		return S;
	}();
	static const FScrollBarStyle Bar = []
	{
		FScrollBarStyle S = *S47Theme::ScrollStyle();
		S.SetNormalThumbImage(FSlateColorBrush(RxPhos(0.3f))).SetHoveredThumbImage(FSlateColorBrush(RxPhos(0.6f))).SetDraggedThumbImage(FSlateColorBrush(RxPhos(0.6f)));
		return S;
	}();
	ChildSlot
	.Padding(Pad(18, 14))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SAssignNew(Scroll, SScrollBox).ScrollBarStyle(&Bar)
			+ SScrollBox::Slot()
			[
				RxText(RxStr([this] { return Text.EndsWith(TEXT("\n")) ? Text.LeftChop(1) : Text; }), 22.f, RxPhos(), Glow)
			]
			// .term-line: "$ " and the command
			+ SScrollBox::Slot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[RxText(FText::FromString(TEXT("$ ")), 22.f, RxPhos(), Glow)]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SAssignNew(Input, SEditableText).Style(&LineStyle).ClearKeyboardFocusOnCommit(false).SelectAllTextWhenFocused(false)
					.OnTextCommitted_Lambda([this](const FText& T, ETextCommit::Type C) { if (C == ETextCommit::OnEnter) { Exec(T.ToString()); if (Input.IsValid()) Input->SetText(FText::GetEmpty()); } })
				]
			]
		]
		// .term-cmds: the commands as keys
		+ SVerticalBox::Slot().AutoHeight().Padding(0, Px(10), 0, 0)[SAssignNew(Buttons, SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(Px(8), Px(8)))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, Px(8), 0, 0)
		[RxText(RxStr([] { return S47Gamepad::UsingGamepad() ? FString(TEXT("D-PAD: COMMAND   A: RUN   LB/RB: SCROLL   RIGHT STICK: POINTER")) : FString(); }), 17.f, RxDim())]
		// .term-foot
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, Px(10), 0, 0)
		[
			SNew(SRxKey).Label_Lambda([] { return FString(S47Gamepad::UsingGamepad() ? TEXT("LOG OUT   [B]") : TEXT("LOG OUT   [ESC]")); }).Px(20.f).MinHeight(44.f).OnClick([this] { if (OnClose) OnClose(); })
		]
	];
	RebuildButtons();
}

TSharedPtr<SWidget> SS47Ops::Line() const { return Input; }

FReply SS47Ops::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	// (Terminal.ts: input.focus() as it opens, where there is a keyboard)
	if (Input.IsValid()) return FReply::Handled().SetUserFocus(Input.ToSharedRef(), EFocusCause::SetDirectly);
	return FReply::Unhandled();
}

void SS47Ops::RebuildButtons()
{
	if (!Buttons.IsValid() || !Story) return;
	Buttons->ClearChildren();
	PadCommands = Story->Ops().Buttons();
	PadCommand = PadCommands.IsValidIndex(PadCommand) ? PadCommand : INDEX_NONE;
	for (int32 I = 0; I < PadCommands.Num(); ++I)
	{
		const FString Cmd = PadCommands[I];
		Buttons->AddSlot()
		[
			SNew(SRxKey).Label_Lambda([this, I, Cmd] { return S47Gamepad::UsingGamepad() && PadCommand == I ? TEXT("> ") + Cmd + TEXT("  [A]") : Cmd; })
			.Px(19.f).MinHeight(44.f).OnClick([this, I, Cmd] { PadCommand = I; Exec(Cmd); })
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
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right)
	{
		if (OnClose) OnClose();
		return FReply::Handled();
	}
	const FKey K = InKeyEvent.GetKey();
	if (K == EKeys::Gamepad_LeftShoulder || K == EKeys::Gamepad_RightShoulder)
	{
		if (Scroll.IsValid()) Scroll->SetScrollOffset(FMath::Max(0.f, Scroll->GetScrollOffset() + (K == EKeys::Gamepad_LeftShoulder ? -1.f : 1.f) * S47Theme::Px(300)));
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// The editable command line retains keyboard typing. Raw pad events choose the existing
// command buttons before that text field can treat A/B as virtual-keyboard input.
FReply SS47Ops::OnPreviewKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	const FKey K = E.GetKey();
	if (K == EKeys::Gamepad_FaceButton_Right || K == EKeys::Gamepad_LeftShoulder || K == EKeys::Gamepad_RightShoulder) return OnKeyDown(G, E);
	const bool Prev = K == EKeys::Gamepad_DPad_Up || K == EKeys::Gamepad_DPad_Left;
	const bool Next = K == EKeys::Gamepad_DPad_Down || K == EKeys::Gamepad_DPad_Right;
	if (Prev || Next)
	{
		if (!PadCommands.IsEmpty()) PadCommand = PadCommand == INDEX_NONE ? (Prev ? PadCommands.Num() - 1 : 0) : (PadCommand + PadCommands.Num() + (Prev ? -1 : 1)) % PadCommands.Num();
		return FReply::Handled();
	}
	if (K == EKeys::Gamepad_FaceButton_Bottom)
	{
		if (!E.IsRepeat() && !PadCommands.IsEmpty())
		{
			if (!PadCommands.IsValidIndex(PadCommand)) PadCommand = 0;
			const FString Cmd = PadCommands[PadCommand]; Exec(Cmd);
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

bool SS47Document::SelfTest(TArray<FString>& Lines)
{
	bool Ok = true;
	auto Check = [&](bool Pass, const TCHAR* Name) { Lines.Add(FString(Pass ? TEXT("ok  document pad: ") : TEXT("FAIL document pad: ")) + Name); Ok &= Pass; };
	Check(FArguments()._UI == nullptr, TEXT("optional reader defaults to null"));
	int32 Closed = 0, Used = 0;
	FS47Doc D; D.Kind = TEXT("typed"); D.Title = TEXT("Input sample"); D.Page = TEXT("A paper.");
	const TSharedRef<SS47Document> W = SNew(SS47Document).Doc(D).UI(nullptr).OnClose([&Closed] { ++Closed; });
	W->Actions.Insert({ TEXT("TEST ACTION"), FString(), [&Used] { ++Used; }, nullptr, nullptr }, 0);
	auto Key = [&](const FKey& K) { return W->OnKeyDown(FGeometry(), FKeyEvent(K, FModifierKeysState(), 0, false, 0, 0)); };
	Key(EKeys::Gamepad_DPad_Down);
	Check(W->Chosen == 1 && Closed == 0, TEXT("D-pad chooses without executing"));
	Key(EKeys::Gamepad_DPad_Up); Key(EKeys::Gamepad_FaceButton_Bottom);
	Check(W->Chosen == 0 && Used == 1 && Closed == 0, TEXT("A executes the selected action exactly once"));
	Check(Key(EKeys::Gamepad_RightShoulder).IsEventHandled() && Key(EKeys::Gamepad_LeftShoulder).IsEventHandled(), TEXT("shoulders route to paper scrolling"));
	Key(EKeys::Gamepad_FaceButton_Right);
	Check(Closed == 1, TEXT("B puts the paper down"));
	return Ok;
}

bool SS47Ops::SelfTest(TArray<FString>& Lines)
{
	bool Ok = true;
	auto Check = [&](bool Pass, const TCHAR* Name) { Lines.Add(FString(Pass ? TEXT("ok  ops pad: ") : TEXT("FAIL ops pad: ")) + Name); Ok &= Pass; };
	int32 Closed = 0;
	const TSharedRef<SS47Ops> W = SNew(SS47Ops).Story(nullptr).OnClose([&Closed] { ++Closed; });
	W->PadCommands = { TEXT("HELP"), TEXT("LOGOUT") };
	auto Key = [&](const FKey& K, bool Repeat = false) { return W->OnPreviewKeyDown(FGeometry(), FKeyEvent(K, FModifierKeysState(), 0, Repeat, 0, 0)); };
	Key(EKeys::Gamepad_DPad_Down);
	Check(W->PadCommand == 0 && Closed == 0, TEXT("first D-pad input chooses the first existing command"));
	Key(EKeys::Gamepad_DPad_Right);
	Check(W->PadCommand == 1 && Closed == 0, TEXT("D-pad advances without running a command"));
	Key(EKeys::Gamepad_FaceButton_Bottom, true);
	Check(Closed == 0, TEXT("held A cannot repeatedly execute commands"));
	Key(EKeys::Gamepad_FaceButton_Bottom);
	Check(Closed == 1, TEXT("A runs the selected LOGOUT command"));
	W->Input->SetText(FText::FromString(TEXT("SHOW LOG")));
	Check(!Key(EKeys::L).IsEventHandled() && W->Input->GetText().ToString() == TEXT("SHOW LOG"), TEXT("keyboard typing remains on the editable command line"));
	return Ok;
}

bool S47WidgetInput::SelfTest(TArray<FString>& Lines)
{
	bool Ok = true;
	auto Check = [&](bool Pass, const TCHAR* Name) { Lines.Add(FString(Pass ? TEXT("ok  receiver pad: ") : TEXT("FAIL receiver pad: ")) + Name); Ok &= Pass; };
	int32 Used = 0;
	const TSharedRef<SRxKey> Button = SNew(SRxKey).Label(FString(TEXT("ACT"))).OnClick([&Used] { ++Used; });
	const auto Key = [](const FKey& K) { return FKeyEvent(K, FModifierKeysState(), 0, false, 0, 0); };
	Check(Button->OnKeyDown(FGeometry(), Key(EKeys::Gamepad_FaceButton_Bottom)).IsEventHandled() && Used == 1, TEXT("A presses a focused receiver key"));
	const TSharedRef<SRxKey> Disabled = SNew(SRxKey).Label(FString(TEXT("LOCKED"))).Usable(false).OnClick([&Used] { ++Used; });
	Check(!Disabled->OnKeyDown(FGeometry(), Key(EKeys::Gamepad_FaceButton_Bottom)).IsEventHandled() && Used == 1, TEXT("A respects disabled receiver actions"));
	float Value = 0.5f;
	const TSharedRef<SRxSlider> Slider = SNew(SRxSlider).Value_Lambda([&Value] { return Value; }).Step(0.1f).OnChanged([&Value](float V) { Value = V; });
	Slider->OnKeyDown(FGeometry(), Key(EKeys::Gamepad_DPad_Left));
	Check(FMath::IsNearlyEqual(Value, 0.4f), TEXT("D-pad left decreases the actual slider value"));
	Slider->OnKeyDown(FGeometry(), Key(EKeys::Gamepad_DPad_Right));
	Check(FMath::IsNearlyEqual(Value, 0.5f), TEXT("D-pad right increases the actual slider value"));
	return Ok;
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
