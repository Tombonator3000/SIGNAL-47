#include "SS47Dev.h"
#include "S47Theme.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Dev, Log, All);

namespace
{
	// The headings on the left, in their order, and the groups under each by the start of the group's name.
	struct FSection { const TCHAR* Name; TArray<const TCHAR*> Starts; };
	const TArray<FSection>& Sections()
	{
		static const TArray<FSection> All = {
			{ TEXT("RUN AND TIME"), { TEXT("Run the night"), TEXT("Time"), TEXT("Director scripts") } },
			{ TEXT("THE STATION"), { TEXT("Director:"), TEXT("Television"), TEXT("SARO"), TEXT("Station"), TEXT("Generator"), TEXT("First start"), TEXT("Screens") } },
			{ TEXT("ALIENS AND DANGER"), { TEXT("Aliens"), TEXT("Roswell"), TEXT("Indoors:"), TEXT("Marks and visits"), TEXT("Highway"), TEXT("Taken:"), TEXT("Afterwards"), TEXT("UFO encounters"), TEXT("Eye glint"), TEXT("Tripo new"), TEXT("Dread sound"), TEXT("No line of their own") } },
			{ TEXT("SKY AND LIGHTS"), { TEXT("Sky"), TEXT("UFO formations"), TEXT("Tripo craft"), TEXT("Massive light"), TEXT("Lights over"), TEXT("Low lights"), TEXT("Reach and skies"), TEXT("Moon") } },
			{ TEXT("NIGHT EVENTS"), { TEXT("Night events"), TEXT("Unease"), TEXT("First big night"), TEXT("Week case"), TEXT("Late cases") } },
			{ TEXT("TOWN AND PLACES"), { TEXT("Places out"), TEXT("Anywhere:"), TEXT("What the shops"), TEXT("Town"), TEXT("Visitors"), TEXT("Motel"), TEXT("Farm"), TEXT("Mail:") } },
			{ TEXT("CASES AND PAPERWORK"), { TEXT("Reyes:"), TEXT("Ward"), TEXT("Map and night log"), TEXT("Tape"), TEXT("Case board"), TEXT("Enlarger"), TEXT("Answer"), TEXT("Economy"), TEXT("Evidence dossier") } },
			{ TEXT("WORLD AND SENSES"), { TEXT("Gear near"), TEXT("Night sounds"), TEXT("Sideband"), TEXT("Animals"), TEXT("Weather"), TEXT("Physics"), TEXT("Looks"), TEXT("World fixes") } },
		};
		return All;
	}
	const TCHAR* const Other = TEXT("OTHER");
	int32 SectionOrder(const FString& Section)
	{
		const TArray<FSection>& All = Sections();
		for (int32 i = 0; i < All.Num(); ++i) if (Section == All[i].Name) return i;
		return All.Num();
	}

	// Where the screen was left, and what was started last (kept between runs of the game: Saved/S47DevRecent.txt).
	FString GLastGroup; int32 GLastLine = 0;
	TArray<FString> GRecent; bool bRecentRead = false;
	constexpr int32 MostRecent = 10, MostShown = 140;
	FString RecentFile() { return FPaths::ProjectSavedDir() / TEXT("S47DevRecent.txt"); }
	FString RecentKey(const FS47DevEntry& D) { return D.Group + TEXT("\t") + D.Label; }
	void ReadRecent()
	{
		if (bRecentRead) return;
		bRecentRead = true;
		FFileHelper::LoadFileToStringArray(GRecent, *RecentFile());
		GRecent.RemoveAll([](const FString& S) { return S.IsEmpty(); });
		if (GRecent.Num() > MostRecent) GRecent.SetNum(MostRecent);
	}
	void Remember(const FS47DevEntry& D)
	{
		const FString Key = RecentKey(D);
		GRecent.Remove(Key); GRecent.Insert(Key, 0);
		if (GRecent.Num() > MostRecent) GRecent.SetNum(MostRecent);
		FFileHelper::SaveStringArrayToFile(GRecent, *RecentFile());
	}
}

FString SS47Dev::SectionOf(const FString& Group)
{
	for (const FSection& S : Sections()) for (const TCHAR* Start : S.Starts) if (Group.StartsWith(Start, ESearchCase::CaseSensitive)) return S.Name;
	return Other;
}

bool SS47Dev::Matches(const FString& LowerHay, const FString& Search)
{
	TArray<FString> Words;
	Search.ToLower().ParseIntoArrayWS(Words);
	for (const FString& W : Words) if (!LowerHay.Contains(W, ESearchCase::CaseSensitive)) return false;
	return true;
}

void SS47Dev::Construct(const FArguments& InArgs)
{
	S47_USING_THEME
	Entries = InArgs._Entries;
	OnBack = InArgs._OnBack;
	bCategories = InArgs._bCategories;
	Hay.Reserve(Entries.Num());
	for (const FS47DevEntry& D : Entries) Hay.Add((D.Group + TEXT(" ") + D.Label + TEXT(" ") + D.Note).ToLower());
	if (bCategories) ReadRecent();
	SAssignNew(GroupBox, SVerticalBox);
	SAssignNew(LineBox, SVerticalBox);
	auto PaneHead = [this](int32 Which) { return FSlateColor(Pane == Which ? S47Theme::Amber(.95f) : S47Theme::Mute()); };
	ChildSlot
	[
		SNew(SBorder).BorderImage(Box(Night(.95f))).Padding(Pad(72, 40, 72, 30))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(InArgs._Title.IsEmpty() ? FString(TEXT("DEVELOPER / START POINTS")) : InArgs._Title)).Font(Heading(44, 2)).ColorAndOpacity(Head())]
				+ SHorizontalBox::Slot().FillWidth(1)
				// The search, where it is typed.
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).MinDesiredWidth(620 * U)
					[
						SNew(SBorder).Padding(Pad(14, 5, 14, 6))
						.BorderImage_Lambda([this] { return S47Theme::Box(S47Theme::Night(.6f), Search.IsEmpty() ? S47Theme::Rule() : S47Theme::Amber(), 2 * S47Theme::U); })
						[
							SNew(STextBlock).Font(Term(26, 1))
							.ColorAndOpacity_Lambda([this] { return FSlateColor(Search.IsEmpty() ? S47Theme::Mute() : S47Theme::Amber()); })
							.Text_Lambda([this]
							{
								if (Search.IsEmpty()) return FText::FromString(FString::Printf(TEXT("SEARCH   type to look in all %d lines"), Entries.Num()));
								return FText::FromString(FString::Printf(TEXT("SEARCH   %s%s"), *Search, FMath::Fmod(FPlatformTime::Seconds(), 1.0) < .55 ? TEXT("_") : TEXT(" ")));
							})
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(Pad(0, 4, 0, 2))
			[
				SNew(STextBlock).AutoWrapText(true).Font(Term(20)).ColorAndOpacity(Body())
				.Text(FText::FromString(!InArgs._Note.IsEmpty() ? InArgs._Note : FString(TEXT("Each start runs in the developer case: your own cases are never saved over. A start point restarts the game at that place. ")
					TEXT("Press F9 anywhere in a night to keep a start point of your own, and F10 for events."))))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(Pad(0, 0, 0, 8))
			[
				SNew(STextBlock).Font(Term(20)).ColorAndOpacity(Mute())
				.Text_Lambda([this] { return FText::FromString(HelpLine()); })
			]
			+ SVerticalBox::Slot().FillHeight(1)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(.36f).Padding(Pad(0, 0, 18, 0))
				[
					SNew(SVerticalBox).Visibility(bCategories ? EVisibility::Visible : EVisibility::Collapsed)
					+ SVerticalBox::Slot().AutoHeight().Padding(Pad(0, 0, 0, 4))[SNew(STextBlock).Font(Term(24, 2)).ColorAndOpacity_Lambda([PaneHead] { return PaneHead(0); })
						.Text_Lambda([this] { return FText::FromString(Search.IsEmpty() ? FString(TEXT("GROUPS")) : FString(TEXT("GROUPS WITH A MATCH"))); })]
					+ SVerticalBox::Slot().FillHeight(1)[SAssignNew(GroupScroll, SScrollBox) + SScrollBox::Slot()[GroupBox.ToSharedRef()]]
				]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(bCategories ? 2 * U : 0)[SNew(SBorder).BorderImage(Box(Rule(.7f)))]]
				+ SHorizontalBox::Slot().FillWidth(bCategories ? .64f : 1.f).Padding(Pad(bCategories ? 18 : 0, 0, 0, 0))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(Pad(0, 0, 0, 4))[SNew(STextBlock).Font(Term(24, 2)).ColorAndOpacity_Lambda([PaneHead] { return PaneHead(1); })
						.Text_Lambda([this] { return FText::FromString(PaneTitle()); })]
					+ SVerticalBox::Slot().FillHeight(1)[SAssignNew(LineScroll, SScrollBox) + SScrollBox::Slot()[LineBox.ToSharedRef()]]
					// The whole note of the line the cursor is on (the rows themselves stay one line high, so a group fits on the screen).
					+ SVerticalBox::Slot().AutoHeight().Padding(Pad(0, 8, 0, 0))
					[
						SNew(SBox).MinDesiredHeight(84 * U)
						[
							SNew(SBorder).BorderImage(Box(Night(.5f), Rule(.8f), 1 * U)).Padding(Pad(14, 6, 14, 6))
							[
								SNew(STextBlock).AutoWrapText(true).Font(Term(22)).ColorAndOpacity(Body())
								.Text_Lambda([this] { return FText::FromString(NoteLine()); })
							]
						]
					]
				]
			]
		]
	];
	Pane = bCategories ? 0 : 1;
	const FString Want = !InArgs._InitialGroup.IsEmpty() ? InArgs._InitialGroup : GLastGroup;
	RebuildGroups(bCategories ? Want : FString());
	if (bCategories && !InArgs._InitialGroup.IsEmpty() && Groups.IsValidIndex(GroupSel) && Groups[GroupSel].Name == InArgs._InitialGroup && !Lines.IsEmpty())
	{
		// A line that opened this screen again (a setting being stepped) comes back with the cursor on itself.
		Pane = 1;
		LineSel = InArgs._InitialGroup == GLastGroup ? FMath::Clamp(GLastLine, 0, Lines.Num() - 1) : 0;
		if (LineScroll.IsValid() && LineRows.IsValidIndex(LineSel)) LineScroll->ScrollDescendantIntoView(LineRows[LineSel], false, EDescendantScrollDestination::IntoView);
	}
	if (bCategories)
	{
		TArray<FString> Loose;
		for (const FGroupRow& G : Groups) if (G.Kind == KindGroup && G.Section == Other) Loose.Add(G.Name);
		if (!Loose.IsEmpty()) UE_LOG(LogS47Dev, Display, TEXT("S47: F10 groups under OTHER: %s"), *FString::Join(Loose, TEXT("; ")));
	}
}

FString SS47Dev::HelpLine() const
{
	if (!Search.IsEmpty()) return TEXT("Up, Down: move.   Enter: start the selected line.   Left: narrow to one group.   Backspace: erase.   Escape: clear the search.");
	if (!bCategories) return TEXT("Up, Down: move.   Enter: start.   Type: search.   Escape: back.");
	return Pane == 0 ? TEXT("Up, Down: group.   Right or Enter: its lines.   Type: search every line.   Escape: back to the night.")
		: TEXT("Up, Down: line.   Enter: start it.   Left or Escape: the groups.   Type: search every line.");
}

FString SS47Dev::PaneTitle() const
{
	if (!Groups.IsValidIndex(GroupSel)) return Search.IsEmpty() ? FString(TEXT("NOTHING HERE")) : FString::Printf(TEXT("NOTHING MATCHES \"%s\""), *Search.ToUpper());
	const FGroupRow& G = Groups[GroupSel];
	const int32 N = Lines.Num() + Hidden;
	if (G.Kind == KindAll) return Search.IsEmpty() ? FString::Printf(TEXT("EVERY LINE   %d"), N) : FString::Printf(TEXT("EVERY MATCH FOR \"%s\"   %d"), *Search.ToUpper(), N);
	if (G.Kind == KindRecent) return FString::Printf(TEXT("USED LAST   %d"), N);
	return FString::Printf(TEXT("%s   %d"), *G.Name.ToUpper(), N);
}

FString SS47Dev::NoteLine() const
{
	if (Pane != 1 || !Lines.IsValidIndex(LineSel)) return Lines.IsEmpty() ? FString(TEXT("Nothing to show. Backspace widens the search.")) : FString(TEXT("Right or Enter goes to the lines; the note of a line shows here."));
	const FS47DevEntry& D = Entries[Lines[LineSel]];
	return D.Note.IsEmpty() ? D.Label : D.Label + TEXT(":  ") + D.Note;
}

void SS47Dev::RebuildGroups(const FString& Keep)
{
	S47_USING_THEME
	Groups.Reset(); GroupRows.Reset(); GroupSel = 0;
	GroupBox->ClearChildren();
	const bool bSearch = !Search.TrimStartAndEnd().IsEmpty();
	int32 Found = 0;
	if (bCategories)
	{
		for (int32 i = 0; i < Entries.Num(); ++i)
		{
			if (bSearch && !Matches(Hay[i], Search)) continue;
			++Found;
			const FString& Name = Entries[i].Group;
			FGroupRow* Have = Groups.FindByPredicate([&Name](const FGroupRow& G) { return G.Name == Name; });
			if (Have) ++Have->Count; else Groups.Add({ SectionOf(Name), Name, 1, KindGroup });
		}
		// (stable: under one heading the groups keep the order they were registered in)
		Groups.StableSort([](const FGroupRow& A, const FGroupRow& B) { return SectionOrder(A.Section) < SectionOrder(B.Section); });
		if (bSearch) { if (Found) Groups.Insert({ FString(), TEXT("Every match"), Found, KindAll }, 0); }
		else
		{
			int32 Live = 0;
			for (const FString& Key : GRecent) if (Entries.ContainsByPredicate([&Key](const FS47DevEntry& D) { return RecentKey(D) == Key; })) ++Live;
			if (Live) Groups.Insert({ FString(), TEXT("Used last"), Live, KindRecent }, 0);
		}
	}
	else Groups.Add({ FString(), TEXT("Every line"), Entries.Num(), KindAll });
	FString Section;
	for (int32 i = 0; i < Groups.Num(); ++i)
	{
		const FGroupRow& G = Groups[i];
		if (G.Kind == KindGroup && G.Name == Keep) GroupSel = i;
		if (G.Kind == KindGroup && G.Section != Section)
		{
			Section = G.Section;
			GroupBox->AddSlot().AutoHeight().Padding(Pad(0, 12, 0, 2))[SNew(STextBlock).Text(FText::FromString(Section)).Font(Term(20, 2)).ColorAndOpacity(AmberDim())];
		}
		TSharedRef<SWidget> Row = SNew(SS47MenuItem).Label(FString::Printf(TEXT("%s   %d"), *G.Name, G.Count)).Px(24).Spacing(1).Pad(FMargin(10, 0, 10, 0))
			.Color(G.Kind == KindGroup ? TOptional<FLinearColor>() : TOptional<FLinearColor>(Head()))
			.Selected_Lambda([this, i] { return GroupSel == i; })
			.OnClick([this, i] { GroupSel = i; RebuildLines(); ToPane(1); });
		GroupRows.Add(Row);
		GroupBox->AddSlot().AutoHeight()[Row];
	}
	if (GroupScroll.IsValid())
	{
		GroupScroll->SetScrollOffset(0);
		if (GroupRows.IsValidIndex(GroupSel) && GroupSel > 0) GroupScroll->ScrollDescendantIntoView(GroupRows[GroupSel], false, EDescendantScrollDestination::Center);
	}
	RebuildLines();
}

void SS47Dev::RebuildLines()
{
	S47_USING_THEME
	Lines.Reset(); LineRows.Reset(); LineSel = 0; Hidden = 0;
	LineBox->ClearChildren();
	if (!Groups.IsValidIndex(GroupSel)) { if (LineScroll.IsValid()) LineScroll->SetScrollOffset(0); return; }
	const FGroupRow& G = Groups[GroupSel];
	const bool bSearch = !Search.TrimStartAndEnd().IsEmpty();
	if (G.Kind == KindRecent)
	{
		for (const FString& Key : GRecent)
		{
			const int32 At = Entries.IndexOfByPredicate([&Key](const FS47DevEntry& D) { return RecentKey(D) == Key; });
			if (At != INDEX_NONE) Lines.Add(At);
		}
	}
	else
	{
		for (int32 i = 0; i < Entries.Num(); ++i)
		{
			if (G.Kind == KindGroup && Entries[i].Group != G.Name) continue;
			if (bSearch && !Matches(Hay[i], Search)) continue;
			// (a very wide search: the first lines only, and how many more there are)
			if (bCategories && G.Kind == KindAll && Lines.Num() >= MostShown) { ++Hidden; continue; }
			Lines.Add(i);
		}
	}
	FString Group;
	for (int32 i = 0; i < Lines.Num(); ++i)
	{
		const FS47DevEntry& D = Entries[Lines[i]];
		if (G.Kind != KindGroup && D.Group != Group)
		{
			Group = D.Group;
			LineBox->AddSlot().AutoHeight().Padding(Pad(0, i ? 12 : 0, 0, 2))[SNew(STextBlock).Text(FText::FromString(Group.ToUpper())).Font(Term(20, 2)).ColorAndOpacity(AmberDim())];
		}
		TSharedRef<SWidget> Row = SNew(SS47MenuItem).Label(D.Label).Px(26).Spacing(1).Pad(FMargin(12, 0, 14, 0)).bArrow(true)
			.Selected_Lambda([this, i] { return Pane == 1 && LineSel == i; })
			.OnHover([this, i] { LineSel = i; Pane = 1; })
			.OnClick([this, i] { LineSel = i; Pane = 1; Choose(); });
		LineRows.Add(Row);
		LineBox->AddSlot().AutoHeight()[Row];
	}
	if (Hidden) LineBox->AddSlot().AutoHeight().Padding(Pad(15, 10, 0, 0))[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("... and %d more. Another word narrows it."), Hidden))).Font(Term(22)).ColorAndOpacity(Mute())];
	if (LineScroll.IsValid()) LineScroll->SetScrollOffset(0);
}

void SS47Dev::ToPane(int32 To)
{
	if (!bCategories) { Pane = 1; return; }
	Pane = To == 1 && !Lines.IsEmpty() ? 1 : 0;
}

void SS47Dev::Move(int32 Dir)
{
	if (Pane == 0)
	{
		if (Groups.IsEmpty()) return;
		GroupSel = FMath::Abs(Dir) > 1 ? FMath::Clamp(GroupSel + Dir, 0, Groups.Num() - 1) : (GroupSel + Dir + Groups.Num()) % Groups.Num();
		if (GroupScroll.IsValid() && GroupRows.IsValidIndex(GroupSel)) GroupScroll->ScrollDescendantIntoView(GroupRows[GroupSel], true, EDescendantScrollDestination::IntoView);
		RebuildLines();
		return;
	}
	if (Lines.IsEmpty()) return;
	LineSel = FMath::Abs(Dir) > 1 ? FMath::Clamp(LineSel + Dir, 0, Lines.Num() - 1) : (LineSel + Dir + Lines.Num()) % Lines.Num();
	if (LineScroll.IsValid() && LineRows.IsValidIndex(LineSel)) LineScroll->ScrollDescendantIntoView(LineRows[LineSel], true, EDescendantScrollDestination::IntoView);
}

void SS47Dev::Choose()
{
	if (Pane == 0) { ToPane(1); return; }
	if (!Lines.IsValidIndex(LineSel)) return;
	const FS47DevEntry& D = Entries[Lines[LineSel]];
	if (bCategories)
	{
		// Where to come back to: the line's own group, and its place in it.
		GLastGroup = D.Group; GLastLine = 0;
		for (int32 i = 0; i < Lines[LineSel]; ++i) if (Entries[i].Group == D.Group) ++GLastLine;
		Remember(D);
	}
	// (a copy: choosing closes this screen)
	const TFunction<void()> Fn = D.Fn;
	if (Fn) Fn();
}

FReply SS47Dev::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	const FKey K = E.GetKey();
	if (K == EKeys::Up || K == EKeys::Gamepad_DPad_Up) { Move(-1); return FReply::Handled(); }
	if (K == EKeys::Down || K == EKeys::Gamepad_DPad_Down) { Move(1); return FReply::Handled(); }
	if (K == EKeys::PageUp) { Move(-8); return FReply::Handled(); }
	if (K == EKeys::PageDown) { Move(8); return FReply::Handled(); }
	if (K == EKeys::Home) { Move(-100000); return FReply::Handled(); }
	if (K == EKeys::End) { Move(100000); return FReply::Handled(); }
	if (K == EKeys::Right || K == EKeys::Tab || K == EKeys::Gamepad_DPad_Right) { ToPane(K == EKeys::Tab ? 1 - Pane : 1); return FReply::Handled(); }
	if (K == EKeys::Left || K == EKeys::Gamepad_DPad_Left) { ToPane(0); return FReply::Handled(); }
	if (K == EKeys::Enter || K == EKeys::Gamepad_FaceButton_Bottom || (K == EKeys::SpaceBar && Search.IsEmpty())) { Choose(); return FReply::Handled(); }
	if (K == EKeys::BackSpace)
	{
		if (!Search.IsEmpty()) { Search.LeftChopInline(1); const FString Keep = Groups.IsValidIndex(GroupSel) ? Groups[GroupSel].Name : FString(); RebuildGroups(Keep); if (Search.IsEmpty()) ToPane(0); else Pane = Lines.IsEmpty() ? 0 : 1; }
		return FReply::Handled();
	}
	if (K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right)
	{
		if (!Search.IsEmpty()) { Search.Empty(); RebuildGroups(GLastGroup); ToPane(0); return FReply::Handled(); }
		if (bCategories && Pane == 1) { ToPane(0); return FReply::Handled(); }
		if (OnBack) OnBack();
		return FReply::Handled();
	}
	return FReply::Unhandled(); // the letters come through OnKeyChar
}

FReply SS47Dev::OnKeyChar(const FGeometry& G, const FCharacterEvent& E)
{
	const TCHAR C = E.GetCharacter();
	if (C < 32 || C == 127) return FReply::Unhandled();
	// (the old way in was "/": it still starts a search, and is not part of it; a space only between words)
	if (Search.IsEmpty() && (C == TEXT('/') || C == TEXT(' '))) return FReply::Handled();
	if (Search.Len() >= 40) return FReply::Handled();
	Search.AppendChar(C);
	// Every match first, so Enter starts the best one at once.
	RebuildGroups(FString());
	Pane = Lines.IsEmpty() ? (bCategories ? 0 : 1) : 1;
	return FReply::Handled();
}

FString SS47Dev::State() const
{
	return FString::Printf(TEXT("pane %d; %d groups; first '%s'; group '%s'; %d lines; line '%s'; search '%s'"), Pane, Groups.Num(), Groups.IsEmpty() ? TEXT("") : *Groups[0].Name,
		Groups.IsValidIndex(GroupSel) ? *Groups[GroupSel].Name : TEXT(""), Lines.Num() + Hidden, Lines.IsValidIndex(LineSel) ? *Entries[Lines[LineSel]].Label : TEXT(""), *Search);
}

void SS47Dev::TestKey(const FKey& K) { OnKeyDown(FGeometry(), FKeyEvent(K, FModifierKeysState(), 0, false, 0, 0)); }

void SS47Dev::TestType(const FString& Text) { for (const TCHAR C : Text) OnKeyChar(FGeometry(), FCharacterEvent(C, FModifierKeysState(), 0, false)); }

bool SS47Dev::SelfTest(TArray<FString>& Out)
{
	bool bOk = true;
	auto Check = [&](bool bGood, const TCHAR* What) { Out.Add(FString::Printf(TEXT("%s devMenu: %s"), bGood ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bGood; };
	Check(SectionOf(TEXT("Television (T)")) == TEXT("THE STATION") && SectionOf(TEXT("Director: figures")) == TEXT("THE STATION"), TEXT("the station's groups stand under THE STATION"));
	Check(SectionOf(TEXT("Night events: the rest")) == TEXT("NIGHT EVENTS") && SectionOf(TEXT("Town day (T)")) == TEXT("TOWN AND PLACES") && SectionOf(TEXT("Time and place")) == TEXT("RUN AND TIME"), TEXT("a group is placed by the start of its name"));
	Check(SectionOf(TEXT("Sky lights (T16-05)")) == TEXT("SKY AND LIGHTS") && SectionOf(TEXT("Dread sound (T)")) == TEXT("ALIENS AND DANGER"), TEXT("sky and danger are apart"));
	Check(SectionOf(TEXT("Something nobody listed")) == Other, TEXT("a group nobody listed still has a place"));
	const FString H = FString(TEXT("Television (T) TV: the station signs off now The flag and the anthem")).ToLower();
	Check(Matches(H, TEXT("sign")) && Matches(H, TEXT("ANTHEM tv")) && Matches(H, TEXT("  flag  ")), TEXT("a search finds words in any case and any order"));
	Check(!Matches(H, TEXT("sign cow")) && Matches(H, TEXT("")), TEXT("every word must be there, and no words find everything"));
	return bOk;
}
