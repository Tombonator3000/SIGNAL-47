#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

// The developer screen (Tom 29 October 2026): start points for chapters and events, grouped,
// and the developer start points saved with F9. Opened from the title's DEVELOPER; what each
// entry does is the caller's (US47Flow::OpenDev).
// FIX10-6 (Tom 10 November 2026: "better than one long list you scroll in"): with bCategories the screen is two panes. Left: the
// groups under eight headings, with what was used last on top. Right: the lines of the group the cursor is on, and under them
// the whole note of the selected line. Typing searches every line at once (any words, any case) and shows the search where it
// is typed; the left pane then lists the groups that have a match. The screen opens where it was left.
struct FS47DevEntry { FString Group, Label, Note; TFunction<void()> Fn; };

class SS47Dev : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Dev) : _bCategories(false) {}
		SLATE_ARGUMENT(TArray<FS47DevEntry>, Entries)
		SLATE_ARGUMENT(TFunction<void()>, OnBack)
		SLATE_ARGUMENT(FString, Title)
		SLATE_ARGUMENT(FString, Note)
		// Long lists: the two panes above. Without it: one list with its group headings (the title's start points).
		SLATE_ARGUMENT(bool, bCategories)
		SLATE_ARGUMENT(FString, InitialGroup)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& G, const FKeyEvent& E) override;
	virtual FReply OnKeyChar(const FGeometry& G, const FCharacterEvent& E) override;
	// The heading a group stands under on the left (the registry's own group names; one not listed: OTHER).
	static FString SectionOf(const FString& Group);
	// Which lines a search finds: every word somewhere in the group, the label or the note, in any case.
	static bool Matches(const FString& LowerHay, const FString& Search);
	static bool SelfTest(TArray<FString>& Out);
	// For -S47DevMenuTest: what the screen holds now, and the keys as the keyboard would give them.
	FString State() const;
	void TestKey(const FKey& K);
	void TestType(const FString& Text);

private:
	enum EKind : int32 { KindGroup, KindAll, KindRecent };
	struct FGroupRow { FString Section, Name; int32 Count = 0; int32 Kind = KindGroup; };
	TArray<FS47DevEntry> Entries;
	TArray<FString> Hay; // lower case, one per entry
	TFunction<void()> OnBack;
	TArray<FGroupRow> Groups;
	TArray<int32> Lines; // the entries on the right now
	int32 GroupSel = 0, LineSel = 0, Pane = 0, Hidden = 0;
	TArray<TSharedPtr<SWidget>> GroupRows, LineRows;
	TSharedPtr<class SScrollBox> GroupScroll, LineScroll;
	TSharedPtr<class SVerticalBox> GroupBox, LineBox;
	bool bCategories = false;
	FString Search;
	void Choose();
	void Move(int32 Dir);
	void ToPane(int32 To);
	void RebuildGroups(const FString& Keep);
	void RebuildLines();
	FString HelpLine() const;
	FString PaneTitle() const;
	FString NoteLine() const;
};
