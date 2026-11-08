#include "SS47Dev.h"
#include "S47Theme.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

void SS47Dev::Construct(const FArguments& InArgs)
{
	S47_USING_THEME
	Entries = InArgs._Entries;
	OnBack = InArgs._OnBack;
	bCategories = InArgs._bCategories;
	OpenGroup = InArgs._InitialGroup;
	SAssignNew(ListBox, SVerticalBox);
	ChildSlot
	[
		SNew(SBorder).BorderImage(Box(Night(.94f))).Padding(Pad(96, 56, 96, 40))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(InArgs._Title.IsEmpty() ? FString(TEXT("DEVELOPER / START POINTS")) : InArgs._Title)).Font(Heading(54, 2)).ColorAndOpacity(Head())]
			+ SVerticalBox::Slot().AutoHeight().Padding(Pad(0, 6, 0, 4))
			[
				SNew(STextBlock).AutoWrapText(true).Font(Term(22)).ColorAndOpacity(Body())
				.Text(FText::FromString(!InArgs._Note.IsEmpty() ? InArgs._Note : FString(TEXT("Each start runs in the developer case: your own cases are never saved over. A start point restarts the game at that place. ")
					TEXT("Press F9 anywhere in a night to keep a start point of your own, and F10 for events. Escape: back to the title."))))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(Pad(0, 0, 0, 6))
			[
				SNew(STextBlock).Font(Term(22)).ColorAndOpacity(Amber(.9f))
				.Text_Lambda([this] { return FText::FromString(HelpLine()); })
			]
			+ SVerticalBox::Slot().FillHeight(1)[SAssignNew(Scroll, SScrollBox) + SScrollBox::Slot()[ListBox.ToSharedRef()]]
		]
	];
	Rebuild();
}

FString SS47Dev::HelpLine() const
{
	if (bSearching) return FString::Printf(TEXT("SEARCH: %s_   Enter: start the selected line. Escape: stop searching."), *Search);
	if (!OpenGroup.IsEmpty()) return FString::Printf(TEXT("%s   /: search everything. Escape: back to the groups."), *OpenGroup.ToUpper());
	return bCategories ? TEXT("Enter: open a group. /: search everything. Escape: back to the night.") : TEXT("/: search everything.");
}

void SS47Dev::Rebuild()
{
	S47_USING_THEME
	Shown.Reset(); Rows.Reset(); Sel = 0;
	ListBox->ClearChildren();
	const FString Want = Search.TrimStartAndEnd();
	if (bSearching && !Want.IsEmpty())
	{
		for (int32 i = 0; i < Entries.Num(); ++i)
		{
			const FS47DevEntry& D = Entries[i];
			if (D.Group.Contains(Want) || D.Label.Contains(Want) || D.Note.Contains(Want)) Shown.Add({ i });
		}
	}
	else if (bCategories && OpenGroup.IsEmpty())
	{
		for (const FS47DevEntry& D : Entries)
		{
			FShown* Have = Shown.FindByPredicate([&D](const FShown& S) { return S.Group == D.Group; });
			if (Have) ++Have->Count; else Shown.Add({ INDEX_NONE, D.Group, 1 });
		}
	}
	else
	{
		for (int32 i = 0; i < Entries.Num(); ++i) if (OpenGroup.IsEmpty() || Entries[i].Group == OpenGroup) Shown.Add({ i });
	}
	FString Group;
	for (int32 i = 0; i < Shown.Num(); ++i)
	{
		const FShown& S = Shown[i];
		const bool bEntry = S.Entry != INDEX_NONE;
		if (bEntry && OpenGroup.IsEmpty() && Entries[S.Entry].Group != Group)
		{
			Group = Entries[S.Entry].Group;
			ListBox->AddSlot().AutoHeight().Padding(Pad(0, 14, 0, 4))
			[SNew(STextBlock).Text(FText::FromString(Group.ToUpper())).Font(Term(24, 2)).ColorAndOpacity(Amber(.9f))];
		}
		TSharedRef<SWidget> Row = SNew(SS47MenuItem).Label(bEntry ? Entries[S.Entry].Label : S.Group).Px(26).Spacing(1).Pad(FMargin(12, 2, 14, 2)).bArrow(true)
			.Meta(bEntry ? Entries[S.Entry].Note : FString::Printf(TEXT("%d lines"), S.Count))
			.Selected_Lambda([this, i] { return Sel == i; })
			.OnHover([this, i] { Sel = i; })
			.OnClick([this, i] { Sel = i; Choose(); });
		Rows.Add(Row);
		ListBox->AddSlot().AutoHeight().Padding(0, 0, 0, 2 * U)[Row];
	}
	if (Scroll.IsValid()) Scroll->SetScrollOffset(0);
}

void SS47Dev::Move(int32 Dir)
{
	if (Shown.IsEmpty()) return;
	Sel = (Sel + Dir + Shown.Num()) % Shown.Num();
	if (Scroll.IsValid() && Rows.IsValidIndex(Sel)) Scroll->ScrollDescendantIntoView(Rows[Sel], true, EDescendantScrollDestination::IntoView);
}

void SS47Dev::Choose()
{
	if (!Shown.IsValidIndex(Sel)) return;
	if (Shown[Sel].Entry == INDEX_NONE) { OpenGroup = Shown[Sel].Group; Rebuild(); return; }
	// (a copy: choosing closes this screen)
	const TFunction<void()> Fn = Entries[Shown[Sel].Entry].Fn;
	if (Fn) Fn();
}

FReply SS47Dev::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	const FKey K = E.GetKey();
	if (bSearching)
	{
		if (K == EKeys::Up) { Move(-1); return FReply::Handled(); }
		if (K == EKeys::Down) { Move(1); return FReply::Handled(); }
		if (K == EKeys::Enter) { Choose(); return FReply::Handled(); }
		if (K == EKeys::BackSpace) { Search.LeftChopInline(1); Rebuild(); return FReply::Handled(); }
		if (K == EKeys::Escape) { bSearching = false; Search.Empty(); Rebuild(); return FReply::Handled(); }
		return FReply::Unhandled(); // the letters come through OnKeyChar
	}
	if (K == EKeys::Up || K == EKeys::W || K == EKeys::Gamepad_DPad_Up) { Move(-1); return FReply::Handled(); }
	if (K == EKeys::Down || K == EKeys::S || K == EKeys::Gamepad_DPad_Down) { Move(1); return FReply::Handled(); }
	if (K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom) { Choose(); return FReply::Handled(); }
	if (K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right)
	{
		if (bCategories && !OpenGroup.IsEmpty()) { OpenGroup.Empty(); Rebuild(); return FReply::Handled(); }
		if (OnBack) OnBack();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SS47Dev::OnKeyChar(const FGeometry& G, const FCharacterEvent& E)
{
	const TCHAR C = E.GetCharacter();
	if (!bSearching)
	{
		if (C != TEXT('/')) return FReply::Unhandled();
		bSearching = true; Search.Empty(); return FReply::Handled();
	}
	if (C >= 32 && C != 127) { Search.AppendChar(C); Rebuild(); return FReply::Handled(); }
	return FReply::Unhandled();
}
