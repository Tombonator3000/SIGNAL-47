#include "SS47SaveMenu.h"

#include "Engine/Texture2D.h"
#include "S47Image.h"
#include "S47Kit.h"
#include "S47Theme.h"
#include "S47UI.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SaveMenu, Log, All);

FString S47SaveMenu::When(double Ms)
{
	// new Intl.DateTimeFormat('en-US', { month: 'short', day: 'numeric', hour: '2-digit', minute: '2-digit', hour12: false })
	static const TCHAR* Mon[] = { TEXT("Jan"), TEXT("Feb"), TEXT("Mar"), TEXT("Apr"), TEXT("May"), TEXT("Jun"), TEXT("Jul"), TEXT("Aug"), TEXT("Sep"), TEXT("Oct"), TEXT("Nov"), TEXT("Dec") };
	const FDateTime Utc = FDateTime(1970, 1, 1) + FTimespan::FromMilliseconds(Ms);
	const FDateTime L = Utc + (FDateTime::Now() - FDateTime::UtcNow());
	return FString::Printf(TEXT("%s %d, %02d:%02d"), Mon[L.GetMonth() - 1], L.GetDay(), L.GetHour(), L.GetMinute());
}

FString S47SaveMenu::KindName(const FS47SaveMeta& M)
{
	return M.Kind == TEXT("auto") ? FString::Printf(TEXT("AUTOSAVE %d"), M.Slot + 1) : FString::Printf(TEXT("SAVE %d"), M.Slot + 1);
}

void S47SaveMenu::Open(US47UI* UI, const FS47SaveMenuOpts& O)
{
	if (!UI) return;
	// ui.open(el, o.onBack)
	UI->Open(SNew(SS47SaveMenu).UI(UI).Opts(O), O.OnBack);
	UE_LOG(LogS47SaveMenu, Display, TEXT("S47: case menu %s, %d saves"), *O.Mode, O.Metas.Num());
}

namespace S47SaveMenuLocal
{
	// a card or a line in it: the mouse chooses it by hovering, and a click uses it
	class SS47Pick : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SS47Pick) {}
			SLATE_ARGUMENT(TFunction<void()>, OnHover)
			SLATE_ARGUMENT(TFunction<void()>, OnClick)
			SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs) { OnHover = InArgs._OnHover; OnClick = InArgs._OnClick; ChildSlot[InArgs._Content.Widget]; }
		virtual void OnMouseEnter(const FGeometry& G, const FPointerEvent& E) override { SCompoundWidget::OnMouseEnter(G, E); if (OnHover) OnHover(); }
		virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent& E) override
		{
			if (E.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
			if (OnHover) OnHover();
			if (OnClick) OnClick();
			return FReply::Handled();
		}

	private:
		TFunction<void()> OnHover, OnClick;
	};

	const FSlateBrush* Thumb(const FS47SaveMeta* M)
	{
		UTexture2D* T = M && !M->Thumb.IsEmpty() ? S47Image::FromDataUrl(M->Thumb) : nullptr;
		const TSharedPtr<FSlateBrush> B = S47Kit::BrushFor(T);
		return B.Get();
	}
}

void SS47SaveMenu::Construct(const FArguments& InArgs)
{
	UI = InArgs._UI;
	O = InArgs._Opts;
	Metas = O.Metas;
	// the card to start on: the case played last (or the first with a tape); for saving, the first empty slot
	if (Saving())
	{
		Card = 0;
		for (int32 C = 0; C < 3; ++C) if (!CardMetas(C).Num()) { Card = C; break; }
	}
	else
	{
		Card = FMath::Clamp(O.ActiveCase - 1, 0, 2);
		if (!CardMetas(Card).Num()) for (int32 C = 0; C < 3; ++C) if (CardMetas(C).Num()) { Card = C; break; }
	}
	if (Saving() && !O.Block.IsEmpty()) { Status = O.Block; bStatusWarn = true; }
	else if (Saving()) Status = FString::Printf(TEXT("Case %d. Autosaves are made at each new chapter, area and finding. Manual saves stay until you overwrite them."), SaveCase());
	else if (O.bFallback) Status = TEXT("This browser blocks the photo store, so saves are kept in a smaller space and may not all fit.");
	Render();
}

TArray<FS47SaveMeta> SS47SaveMenu::CardMetas(int32 C) const
{
	if (Saving())
	{
		const int32 Cs = SaveCase();
		return Metas.FilterByPredicate([Cs, C](const FS47SaveMeta& M) { return M.CaseId == Cs && M.Kind == TEXT("manual") && M.Slot == C; });
	}
	TArray<FS47SaveMeta> L = Metas.FilterByPredicate([C](const FS47SaveMeta& M) { return M.CaseId == C + 1; });
	L.StableSort([](const FS47SaveMeta& A, const FS47SaveMeta& B) { return A.SavedAt > B.SavedAt; });
	return L;
}

const FS47SaveMeta* SS47SaveMenu::Shown(int32 C) const
{
	const TArray<FS47SaveMeta> L = CardMetas(C);
	if (!L.Num()) return nullptr;
	const FString Id = L[C == Card ? FMath::Clamp(Row, 0, L.Num() - 1) : 0].Id;
	return Metas.FindByPredicate([&Id](const FS47SaveMeta& M) { return M.Id == Id; });
}

void SS47SaveMenu::Close(bool bSilent)
{
	if (UI.IsValid()) UI->Close(bSilent);
}

TSharedRef<SWidget> SS47SaveMenu::CardWidget(int32 C)
{
	S47_USING_THEME
	using S47SaveMenuLocal::SS47Pick;
	const TArray<FS47SaveMeta> L = CardMetas(C);
	const bool bEmpty = L.Num() == 0;
	double Most = 0;
	for (const FS47SaveMeta& X : L) Most = FMath::Max(Most, X.Playtime);
	auto IsSel = [this, C] { return Card == C && !Ask.IsSet(); };

	// .n: CASE n (or SAVE n) and the time played
	TSharedRef<SWidget> Top = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
		[
			SNew(STextBlock).Text(FText::FromString((Saving() ? FString::Printf(TEXT("SAVE %d"), C + 1) : FString::Printf(TEXT("CASE %d"), C + 1)))).Font(Term(30, 3)).ColorAndOpacity(Amber())
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(0, 0, 0, 3 * U)
		[
			SNew(STextBlock).Text(FText::FromString(bEmpty ? FString() : Played(Most))).Font(Term(22, 3)).ColorAndOpacity(Arrow())
		];

	// .shot: the picture at 16:9, the night's clock on it as the tape shows it; NO TAPE when there is none
	TSharedRef<SWidget> Shot = SNew(SScaleBox).Stretch(EStretch::ScaleToFitX)
		[
			SNew(SBox).WidthOverride(405 * U).HeightOverride(228 * U)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()[SNew(SBorder).BorderImage(Box(Css(0x020306), Paper(0.12f), 1.f)).Padding(0)]
				+ SOverlay::Slot().Padding(1)
				[
					SNew(SImage).Image_Lambda([this, C] { return S47SaveMenuLocal::Thumb(Shown(C)); })
					.Visibility_Lambda([this, C] { const FS47SaveMeta* M = Shown(C); return M && !M->Thumb.IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(bEmpty ? TEXT("NO TAPE") : TEXT("NO PICTURE"))).Font(Term(26, 3)).ColorAndOpacity(Css(0x4d483f))
					.Visibility_Lambda([this, C] { const FS47SaveMeta* M = Shown(C); return !M || M->Thumb.IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
				]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(Pad(0, 0, 8, 4))
				[
					SNew(SS47Osd).Px(20).Text_Lambda([this, C] { const FS47SaveMeta* M = Shown(C); return M ? M->Clock : FString(); })
				]
			]
		];

	// .ch and .sub: the chapter, the place and when it was saved
	TSharedRef<SVerticalBox> Words = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Font(Heading(24, 1)).AutoWrapText(true)
			.ColorAndOpacity(bEmpty ? Arrow() : Head())
			.Text_Lambda([this, C]
			{
				const FS47SaveMeta* M = Shown(C);
				if (!M) return FText::FromString(TEXT("EMPTY"));
				const TPair<FString, FString> Ch = ChapterLine(M->Chapter);
				return FText::FromString(Ch.Value.IsEmpty() ? Ch.Key : Ch.Key + TEXT(" · ") + Ch.Value);
			})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4 * U, 0, 0)
		[
			SNew(STextBlock).Font(Type(15)).ColorAndOpacity(PaperDim()).AutoWrapText(true)
			.Text_Lambda([this, C]
			{
				const FS47SaveMeta* M = Shown(C);
				if (!M) return FText::FromString(Saving() ? TEXT("Save the night here.") : TEXT("A new night starts here."));
				return FText::FromString(M->Place + TEXT(" · saved ") + SavedWhen(M->SavedAt));
			})
		];

	// .auto: the case's saves, to choose from (loading); the one chosen in amber with its rule
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	if (!Saving() && !bEmpty)
	{
		List->AddSlot().AutoHeight().Padding(0, 14 * U, 0, 10 * U)[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Paper(0.1f))]];
		for (int32 R = 0; R < L.Num(); ++R)
		{
			const FS47SaveMeta& M = L[R];
			auto Lit = [this, C, R] { return O.Mode == TEXT("load") && Card == C && Row == R && !Ask.IsSet(); };
			const FString Kind = M.Kind == TEXT("auto") ? FString(TEXT("AUTO")) : FString::Printf(TEXT("SAVE %d"), M.Slot + 1);
			List->AddSlot().AutoHeight()
			[
				SNew(SS47Pick).OnHover([this, C, R] { Pick(C, R); }).OnClick([this] { Activate(); })
				[
					SNew(SBorder).Padding(FMargin(0)).BorderImage_Lambda([Lit] { return Lit() ? Box(Amber(0.1f)) : Box(FLinearColor::Transparent); })
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(3 * U)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor_Lambda([Lit] { return FSlateColor(Lit() ? Amber() : FLinearColor::Transparent); })]]
						+ SHorizontalBox::Slot().AutoWidth().Padding(6 * U, 1 * U, 10 * U, 1 * U)
						[
							SNew(SBox).MinDesiredWidth(62 * U)[SNew(STextBlock).Text(FText::FromString(Kind)).Font(Term(21, 1)).ColorAndOpacity_Lambda([Lit] { return FSlateColor(Lit() ? Amber() : Body()); })]
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 1 * U)
						[
							SNew(STextBlock).Text(FText::FromString(M.Clock + TEXT(" · ") + M.Place.ToUpper())).Font(Term(21, 1)).ColorAndOpacity_Lambda([Lit] { return FSlateColor(Lit() ? Css(0xd9b27f) : Mute()); })
						]
					]
				]
			];
		}
	}

	// .case: the card, amber round the one chosen
	return SNew(SS47Pick).OnHover([this, C] { if (Card != C) Pick(C, 0); }).OnClick([this] { Activate(); })
		[
			SNew(SBorder).Padding(1).BorderImage_Lambda([IsSel] { return Box(FLinearColor::Transparent, IsSel() ? Amber(0.4f) : FLinearColor::Transparent, 1.f); })
			[
				SNew(SBorder).Padding(Pad(18, 18, 18, 16))
				.BorderImage_Lambda([IsSel] { return Box(Css(0x080b13, IsSel() ? 0.94f : 0.88f), IsSel() ? Amber() : Paper(0.14f), 1.f); })
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[Top]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 12 * U)[Shot]
					+ SVerticalBox::Slot().AutoHeight()[Words]
					+ SVerticalBox::Slot().AutoHeight()[List]
				]
			]
		];
}

void SS47SaveMenu::Render()
{
	S47_USING_THEME
	const FString TitleText = Saving() ? TEXT("SAVE CASE") : O.Mode == TEXT("new") ? TEXT("NEW NIGHT") : TEXT("LOAD CASE");
	const FString Id = Saving() ? FString::Printf(TEXT("CASE %d // MANUAL SAVES"), SaveCase()) : O.Mode == TEXT("new") ? TEXT("EVERY CASE IS IN USE // CHOOSE ONE TO REPLACE") : TEXT("CASE FILES // SARO ARCHIVE");
	TArray<S47Theme::FHint> Keys;
	if (Saving()) Keys = { { TEXT("LEFT RIGHT"), TEXT("SLOT"), nullptr }, { TEXT("ENTER"), TEXT("SAVE"), [this] { Activate(); } }, { TEXT("ESC"), TEXT("BACK"), [this] { Close(); } } };
	else if (O.Mode == TEXT("new")) Keys = { { TEXT("LEFT RIGHT"), TEXT("CASE"), nullptr }, { TEXT("ENTER"), TEXT("REPLACE"), [this] { Activate(); } }, { TEXT("ESC"), TEXT("BACK"), [this] { Close(); } } };
	else Keys = { { TEXT("LEFT RIGHT"), TEXT("CASE"), nullptr }, { TEXT("UP DOWN"), TEXT("SAVE"), nullptr }, { TEXT("ENTER"), TEXT("LOAD"), [this] { Activate(); } },
		{ TEXT("DEL"), TEXT("ERASE"), [this] { Erase(); } }, { TEXT("ESC"), TEXT("BACK"), [this] { Close(); } } };

	TSharedRef<SHorizontalBox> Cards = SNew(SHorizontalBox);
	for (int32 C = 0; C < 3; ++C) Cards->AddSlot().FillWidth(1.f).VAlign(VAlign_Top).Padding(C ? 28 * U : 0.f, 0, 0, 0)[CardWidget(C)];

	TSharedRef<SOverlay> Root = SNew(SOverlay)
		// .bg { filter: blur(6px) brightness(.4) } and .tint rgba(4,6,11,.6): .16 of the picture is left
		+ SOverlay::Slot()[Backdrop(6.f, Css(0x04060b, 0.84f), false)]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(Pad(110, 74, 0, 0))
		[
			SNew(STextBlock).Text(FText::FromString(TitleText)).Font(Heading(56, 2)).ColorAndOpacity(Head())
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(Pad(0, 96, 110, 0))
		[
			SNew(STextBlock).Text(FText::FromString(Id)).Font(Term(22, 2)).ColorAndOpacity(Sage())
		]
		+ SOverlay::Slot().VAlign(VAlign_Top).Padding(Pad(110, 170, 110, 0))[Cards]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(Pad(110, 0, 110, 70))
		[
			SNew(STextBlock).Text_Lambda([this] { return FText::FromString(Status); }).Font(Term(22, 1)).AutoWrapText(true)
			.ColorAndOpacity_Lambda([this] { return FSlateColor(bStatusWarn ? Css(0xffd27a) : Phos(0.9f)); })
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(Pad(110, 0, 0, 26))[KeyHints(Keys)]
		+ SOverlay::Slot()[Scanlines()];
	if (Ask)
	{
		// the question, over the cards: Enter or its button does it, Escape or Cancel leaves it
		const TFunction<void()> Act = Ask->Act;
		Root->AddSlot()[SNew(SBorder).BorderImage(Box(Night(0.8f))).Padding(0)];
		Root->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(600 * U)
			[
				Frame(
					SNew(SBox).Padding(Pad(36, 28, 36, 26))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Id)).Font(Term(22, 2)).ColorAndOpacity(Sage())]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 10 * U, 0, 22 * U)
						[
							SNew(STextBlock).Text(FText::FromString(Ask->Text)).Font(Heading(30, 1)).ColorAndOpacity(Head()).AutoWrapText(true)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()[Button(Ask->Yes, true, [Act] { if (Act) Act(); }, true, true)]
							+ SHorizontalBox::Slot().AutoWidth().Padding(12 * U, 0, 0, 0)[Button(TEXT("CANCEL"), false, [this] { Ask.Reset(); Render(); })]
							+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[KeyHints({ { TEXT("ENTER"), TEXT("YES"), nullptr }, { TEXT("ESC"), TEXT("NO"), nullptr } }, 20)]
						]
					])
			]
		];
	}
	ChildSlot[Root];
}

void SS47SaveMenu::Tick(const FGeometry& G, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(G, InCurrentTime, InDeltaTime);
	S47Theme::KeepFocus(SharedThis(this));
}

void SS47SaveMenu::Pick(int32 C, int32 R)
{
	if (Ask) return;
	Card = FMath::Clamp(C, 0, 2);
	Row = FMath::Clamp(R, 0, FMath::Max(0, CardMetas(Card).Num() - 1));
}

void SS47SaveMenu::Confirm(const FString& Text, const FString& Yes, TFunction<void()> Act)
{
	Ask = FAsk{ Text, Yes, MoveTemp(Act) };
	Render();
}

void SS47SaveMenu::Activate()
{
	if (Ask) return;
	const TArray<FS47SaveMeta> L = CardMetas(Card);
	if (Saving())
	{
		if (!O.Block.IsEmpty()) { Status = O.Block; bStatusWarn = true; return; }
		const int32 S = Card;
		auto Go = [this, S]
		{
			Ask.Reset();
			FString Err;
			const TOptional<FS47SaveMeta> M = O.OnSave ? O.OnSave(S, Err) : TOptional<FS47SaveMeta>();
			if (M)
			{
				Metas.RemoveAll([&](const FS47SaveMeta& X) { return X.Id == M->Id; });
				Metas.Add(*M);
				Status = FString::Printf(TEXT("Saved to save %d."), S + 1); bStatusWarn = false;
			}
			else { Status = Err.IsEmpty() ? TEXT("The save failed. Your earlier saves are unchanged.") : Err; bStatusWarn = true; }
			Render();
		};
		if (L.Num()) Confirm(FString::Printf(TEXT("Overwrite save %d?"), S + 1), TEXT("OVERWRITE"), Go);
		else Go();
		return;
	}
	if (O.Mode == TEXT("new"))
	{
		const int32 Cs = Card + 1;
		auto Go = [this, Cs] { TFunction<void(int32)> Fn = O.OnNewCase; Close(true); if (Fn) Fn(Cs); };
		if (L.Num()) Confirm(FString::Printf(TEXT("Case %d and all its saves will be deleted."), Cs), TEXT("START NEW NIGHT"), Go);
		else Go();
		return;
	}
	const FS47SaveMeta* M = Shown(Card);
	if (!M) { Status = FString::Printf(TEXT("There is no tape in case %d."), Card + 1); bStatusWarn = false; return; }
	const FS47SaveMeta Chosen = *M;
	TFunction<void(const FS47SaveMeta&)> Fn = O.OnLoad;
	Close(true);
	if (Fn) Fn(Chosen);
}

void SS47SaveMenu::Erase()
{
	if (Ask || O.Mode != TEXT("load")) return;
	const FS47SaveMeta* Found = Shown(Card);
	if (!Found) return;
	const FS47SaveMeta M = *Found;
	Confirm(FString::Printf(TEXT("Delete %s?"), *S47SaveMenu::KindName(M).ToLower()), TEXT("DELETE"), [this, M]
	{
		FString Err;
		if (O.OnDelete && !O.OnDelete(M, Err)) { Status = Err.IsEmpty() ? TEXT("That save could not be deleted.") : Err; bStatusWarn = true; return; } // (a failed delete leaves the question up)
		Metas.RemoveAll([&](const FS47SaveMeta& X) { return X.Id == M.Id; });
		Ask.Reset();
		Row = FMath::Clamp(Row, 0, FMath::Max(0, CardMetas(Card).Num() - 1));
		Status = TEXT("Deleted."); bStatusWarn = false;
		Render();
	});
}

FReply SS47SaveMenu::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	const FKey K = E.GetKey();
	if (Ask)
	{
		if (K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom) { const TFunction<void()> Act = Ask->Act; if (Act) Act(); }
		else if (K == EKeys::Escape || K == EKeys::BackSpace || K == EKeys::Gamepad_FaceButton_Right) { Ask.Reset(); Render(); }
		return FReply::Handled();
	}
	if (K == EKeys::Left || K == EKeys::A || K == EKeys::Gamepad_DPad_Left) { Pick(Card - 1, 0); return FReply::Handled(); }
	if (K == EKeys::Right || K == EKeys::D || K == EKeys::Gamepad_DPad_Right) { Pick(Card + 1, 0); return FReply::Handled(); }
	if (K == EKeys::Up || K == EKeys::W || K == EKeys::Gamepad_DPad_Up) { Pick(Card, Row - 1); return FReply::Handled(); }
	if (K == EKeys::Down || K == EKeys::S || K == EKeys::Gamepad_DPad_Down) { Pick(Card, Row + 1); return FReply::Handled(); }
	if (K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom) { Activate(); return FReply::Handled(); }
	if (K == EKeys::Delete || K == EKeys::BackSpace || K == EKeys::Gamepad_FaceButton_Left) { Erase(); return FReply::Handled(); }
	// main.ts: Escape or P with a menu open closes it (back to where it was opened from)
	if (K == EKeys::Escape || K == EKeys::P || K == EKeys::Gamepad_FaceButton_Right) { Close(); return FReply::Handled(); }
	return FReply::Unhandled();
}

FNavigationReply SS47SaveMenu::OnNavigation(const FGeometry& G, const FNavigationEvent& E)
{
	// a gamepad's stick
	if (Ask) return FNavigationReply::Stop();
	switch (E.GetNavigationType())
	{
	case EUINavigation::Left: Pick(Card - 1, 0); break;
	case EUINavigation::Right: Pick(Card + 1, 0); break;
	case EUINavigation::Up: Pick(Card, Row - 1); break;
	case EUINavigation::Down: Pick(Card, Row + 1); break;
	default: break;
	}
	return FNavigationReply::Stop();
}
