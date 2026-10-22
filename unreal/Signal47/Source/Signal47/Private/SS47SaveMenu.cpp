#include "SS47SaveMenu.h"

#include "Engine/Texture2D.h"
#include "S47Image.h"
#include "S47Kit.h"
#include "S47UI.h"
#include "SS47Screens.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

using namespace S47Css;

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
}

void SS47SaveMenu::Construct(const FArguments& InArgs)
{
	UI = InArgs._UI;
	O = InArgs._Opts;
	Metas = O.Metas;
	Render();
}

TArray<FS47SaveMeta> SS47SaveMenu::CaseMetas(int32 C) const
{
	TArray<FS47SaveMeta> L = Metas.FilterByPredicate([C](const FS47SaveMeta& M) { return M.CaseId == C; });
	L.StableSort([](const FS47SaveMeta& A, const FS47SaveMeta& B) { return A.SavedAt > B.SavedAt; });
	return L;
}

void SS47SaveMenu::Close(bool bSilent)
{
	if (UI.IsValid()) UI->Close(bSilent);
}

namespace
{
	TSharedRef<SWidget> Amber(const FString& Label, TFunction<void()> Fn)
	{
		// .save-btns button: the amber one
		return SNew(SBox).MinDesiredHeight(40 * PX)[BoxButton(Label, Term(20), C(0x11140f), C(0xffb052), FLinearColor::Transparent, MoveTemp(Fn), FMargin(12 * PX, 2 * PX))];
	}
	TSharedRef<SWidget> Quiet(const FString& Label, TFunction<void()> Fn)
	{
		// .save-btns button.quiet
		return SNew(SBox).MinDesiredHeight(40 * PX)[BoxButton(Label, Term(20), C(0xcfc8b5), FLinearColor::Transparent, FLinearColor(1, 1, 1, 0.2f), MoveTemp(Fn), FMargin(12 * PX, 2 * PX))];
	}
	TSharedRef<SWidget> Thumb(const FString& Url)
	{
		// .save-thumb: 112 x 63, the picture covering it
		UTexture2D* T = Url.IsEmpty() ? nullptr : S47Image::FromDataUrl(Url);
		TSharedRef<SBox> B = SNew(SBox).WidthOverride(112 * PX).HeightOverride(63 * PX);
		if (T) B->SetContent(SNew(SBorder).BorderImage(Box(C(0x10141c), FLinearColor(1, 1, 1, 0.1f), 1.f)).Padding(1)[SNew(SImage).Image(S47Kit::BrushFor(T).Get())]);
		else B->SetContent(SNew(SBorder).BorderImage(Box(C(0x10141c), FLinearColor(1, 1, 1, 0.1f), 1.f)));
		return B;
	}
}

TSharedRef<SWidget> SS47SaveMenu::Card(const FS47SaveMeta& M, TSharedRef<SWidget> Btns) const
{
	FSlateFontInfo Kind = Term(16);
	Kind.LetterSpacing = 80;
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(Box(FLinearColor(1, 1, 1, 0.08f)))]]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 8 * PX)
		[
			// .save-card: picture, words, buttons
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Thumb(M.Thumb)]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(12 * PX, 0)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(S47SaveMenu::KindName(M))).Font(Kind).ColorAndOpacity(C(0x8cffa4))]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(M.Chapter)).Font(Display(18)).ColorAndOpacity(C(0xf0ead8))]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(M.Place + TEXT(" · ") + M.Clock)).Font(Type(14)).ColorAndOpacity(C(0xb9b2a2))]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("Saved ") + S47SaveMenu::When(M.SavedAt) + TEXT(" · ") + Played(M.Playtime) + TEXT(" played"))).Font(Type(14)).ColorAndOpacity(C(0xb9b2a2))]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Btns]
		];
}

TSharedRef<SWidget> SS47SaveMenu::Buttons(const FString& Host, TArray<TSharedRef<SWidget>> Normal)
{
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	if (Ask && Ask->Host == Host)
	{
		// <span class="ask">text</span><button data-yes>yes</button><button class="quiet" data-no>Cancel</button>
		const TFunction<void()> Act = Ask->Act;
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Ask->Text)).Font(Type(14)).ColorAndOpacity(C(0xffd27a))];
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(8 * PX, 0, 0, 0)[Amber(Ask->Yes, [Act] { if (Act) Act(); })];
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(8 * PX, 0, 0, 0)[Quiet(TEXT("Cancel"), [this] { Ask.Reset(); Render(); })];
		return Row;
	}
	for (int32 i = 0; i < Normal.Num(); ++i) Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(i ? 8 * PX : 0, 0, 0, 0)[Normal[i]];
	return Row;
}

void SS47SaveMenu::ConfirmIn(const FString& Host, const FString& Text, const FString& Yes, TFunction<void()> Act)
{
	Ask = FAsk{ Host, Text, Yes, MoveTemp(Act) };
	Render();
}

void SS47SaveMenu::Render(const FString& Status)
{
	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	auto Note = [](const FString& S, const FLinearColor& Col, float Px = 15.f)
	{
		return SNew(SBox).Padding(FMargin(0, 8 * PX, 0, 12 * PX))[SNew(STextBlock).Text(FText::FromString(S)).Font(Type(Px)).ColorAndOpacity(Col).AutoWrapText(true).LineHeightPercentage(1.45f)];
	};
	if (O.Mode == TEXT("save"))
	{
		const int32 Cs = O.ActiveCase ? O.ActiveCase : 1;
		Body->AddSlot().AutoHeight()[Note(FString::Printf(TEXT("Case %d. Autosaves are made at each new chapter, area and finding. Manual saves stay until you overwrite them."), Cs), C(0xa39e92))];
		if (!O.Block.IsEmpty()) Body->AddSlot().AutoHeight()[Note(O.Block, C(0xffd27a), 16.f)];
		for (int32 S = 0; S < S47Saves::MANUAL; ++S)
		{
			const FS47SaveMeta* Have = Metas.FindByPredicate([Cs, S](const FS47SaveMeta& X) { return X.CaseId == Cs && X.Kind == TEXT("manual") && X.Slot == S; });
			TArray<TSharedRef<SWidget>> B;
			if (O.Block.IsEmpty())
			{
				const bool bExisting = Have != nullptr;
				B.Add(Amber(bExisting ? TEXT("Overwrite") : TEXT("Save here"), [this, S, Cs]
				{
					const bool bThere = Metas.ContainsByPredicate([Cs, S](const FS47SaveMeta& X) { return X.CaseId == Cs && X.Kind == TEXT("manual") && X.Slot == S; });
					auto Go = [this, S]
					{
						Ask.Reset();
						Render(TEXT("Saving..."));
						FString Err;
						const TOptional<FS47SaveMeta> M = O.OnSave ? O.OnSave(S, Err) : TOptional<FS47SaveMeta>();
						if (M)
						{
							Metas.RemoveAll([&](const FS47SaveMeta& X) { return X.Id == M->Id; });
							Metas.Add(*M);
							Render(FString::Printf(TEXT("Saved to save %d."), S + 1));
						}
						else Render(Err.IsEmpty() ? TEXT("The save failed. Your earlier saves are unchanged.") : Err);
					};
					if (bThere) ConfirmIn(FString::Printf(TEXT("save:%d"), S), FString::Printf(TEXT("Overwrite save %d?"), S + 1), TEXT("Overwrite"), Go);
					else Go();
				}));
			}
			const FString Host = FString::Printf(TEXT("save:%d"), S);
			if (Have) Body->AddSlot().AutoHeight()[Card(*Have, Buttons(Host, B))];
			else
			{
				// .save-card.empty
				FSlateFontInfo Kind = Term(16);
				Kind.LetterSpacing = 80;
				Body->AddSlot().AutoHeight()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(Box(FLinearColor(1, 1, 1, 0.08f)))]]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8 * PX)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Thumb(FString())]
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(12 * PX, 0)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("SAVE %d"), S + 1))).Font(Kind).ColorAndOpacity(C(0x8cffa4))]
							+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("Empty"))).Font(Type(14)).ColorAndOpacity(C(0xb9b2a2))]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Buttons(Host, B)]
					]
				];
			}
		}
	}
	else
	{
		for (int32 Cs = 1; Cs <= S47Saves::CASES; ++Cs)
		{
			const TArray<FS47SaveMeta> List = CaseMetas(Cs);
			double Most = 0;
			for (const FS47SaveMeta& X : List) Most = FMath::Max(Most, X.Playtime);
			const FString Head = List.Num() ? FString::Printf(TEXT("Last saved %s · %s played"), *S47SaveMenu::When(List[0].SavedAt), *Played(Most)) : FString(TEXT("Empty"));
			FSlateFontInfo H4 = Term(24);
			H4.LetterSpacing = 60;
			// .save-case: CASE n (current), the line under it
			TSharedRef<SHorizontalBox> Title = SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("CASE %d"), Cs))).Font(H4).ColorAndOpacity(C(0xffb052))];
			if (O.ActiveCase == Cs) Title->AddSlot().AutoWidth().VAlign(VAlign_Bottom).Padding(8 * PX, 0, 0, 2)[SNew(STextBlock).Text(FText::FromString(TEXT("current"))).Font(Term(16)).ColorAndOpacity(C(0x8cffa4))];
			Body->AddSlot().AutoHeight().Padding(0, 14 * PX, 0, 0)[Title];
			Body->AddSlot().AutoHeight().Padding(0, 2 * PX, 0, 8 * PX)[SNew(STextBlock).Text(FText::FromString(Head)).Font(Type(14)).ColorAndOpacity(C(0xa39e92))];
			if (O.Mode == TEXT("new"))
			{
				const bool bUsed = List.Num() > 0;
				TArray<TSharedRef<SWidget>> B;
				B.Add(Amber(bUsed ? TEXT("Replace with a new night") : TEXT("Start a new night here"), [this, Cs, bUsed]
				{
					auto Go = [this, Cs] { TFunction<void(int32)> Fn = O.OnNewCase; Close(true); if (Fn) Fn(Cs); };
					if (bUsed) ConfirmIn(FString::Printf(TEXT("new:%d"), Cs), FString::Printf(TEXT("Case %d and all its saves will be deleted."), Cs), TEXT("Start new night"), Go);
					else Go();
				}));
				Body->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(0, 4 * PX, 0, 6 * PX)[Buttons(FString::Printf(TEXT("new:%d"), Cs), B)];
				if (List.Num()) Body->AddSlot().AutoHeight()[Card(List[0], SNew(SBox))];
			}
			else
			{
				for (const FS47SaveMeta& M : List)
				{
					TArray<TSharedRef<SWidget>> B;
					B.Add(Amber(TEXT("Load"), [this, M] { TFunction<void(const FS47SaveMeta&)> Fn = O.OnLoad; Close(true); if (Fn) Fn(M); }));
					B.Add(Quiet(TEXT("Delete"), [this, M]
					{
						ConfirmIn(TEXT("del:") + M.Id, FString::Printf(TEXT("Delete %s?"), *S47SaveMenu::KindName(M).ToLower()), TEXT("Delete"), [this, M]
						{
							FString Err;
							if (O.OnDelete && !O.OnDelete(M, Err)) return; // (a failed delete leaves the question up)
							Metas.RemoveAll([&](const FS47SaveMeta& X) { return X.Id == M.Id; });
							Ask.Reset();
							Render(TEXT("Deleted."));
						});
					}));
					Body->AddSlot().AutoHeight()[Card(M, Buttons(TEXT("del:") + M.Id, B))];
				}
			}
		}
	}
	const FString TitleText = O.Mode == TEXT("save") ? TEXT("Save case") : O.Mode == TEXT("new") ? TEXT("New case") : TEXT("Load case");
	FSlateFontInfo H3 = Display(32);
	H3.LetterSpacing = 30;
	TSharedRef<SVerticalBox> Menu = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(TitleText)).Font(H3).ColorAndOpacity(C(0xf0ead8))]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).MinDesiredHeight(44 * PX)[TextButton(TEXT("Back"), Term(24), C(0xcfc8b5), C(0xffb052), [this] { Close(); }, true, C(0xcfc8b5), false, FMargin(0))]
			]
		];
	if (O.bFallback) Menu->AddSlot().AutoHeight()[Note(TEXT("This browser blocks the photo store, so saves are kept in a smaller space and may not all fit."), C(0xffd27a))];
	Menu->AddSlot().FillHeight(1.f)[SNew(SScrollBox) + SScrollBox::Slot()[Body]];
	Menu->AddSlot().AutoHeight().Padding(0, 10 * PX, 0, 0)[SNew(SBox).MinDesiredHeight(24 * PX)[SNew(STextBlock).Text(FText::FromString(Status)).Font(Term(20)).ColorAndOpacity(C(0x8cffa4))]];

	ChildSlot
	[
		// .overlay.savemenu-wrap: a click on the dark closes the menu
		SNew(SBorder).BorderImage(Box(C(0x020306, 0.72f))).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(12)
		.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent&) { Close(); return FReply::Handled(); })
		[
			SNew(SBox).WidthOverride(820 * PX).MaxDesiredHeight_Lambda([this] { return FOptionalSize(FMath::Max(240.f, float(GetCachedGeometry().GetLocalSize().Y) - 24.f)); })
			[
				SNew(SBorder).BorderImage(Box(C(0x080b13, 0.96f), FLinearColor(1, 1, 1, 0.12f), 1.f)).Padding(FMargin(26 * PX, 22 * PX))
				.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })
				[
					Menu
				]
			]
		]
	];
}

FReply SS47SaveMenu::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	// main.ts: Escape or P with a menu open closes it (back to where it was opened from)
	if (E.GetKey() == EKeys::Escape || E.GetKey() == EKeys::P) { Close(); return FReply::Handled(); }
	return FReply::Unhandled();
}
