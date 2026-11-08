#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

// The developer screen (Tom 29 October 2026): start points for chapters and events, grouped,
// and the developer start points saved with F9. Opened from the title's DEVELOPER; what each
// entry does is the caller's (US47Flow::OpenDev).
struct FS47DevEntry { FString Group, Label, Note; TFunction<void()> Fn; };

class SS47Dev : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Dev) : _bCategories(false) {}
		SLATE_ARGUMENT(TArray<FS47DevEntry>, Entries)
		SLATE_ARGUMENT(TFunction<void()>, OnBack)
		SLATE_ARGUMENT(FString, Title)
		SLATE_ARGUMENT(FString, Note)
		// Long lists: a page of groups first, Enter opens one, Escape goes back. "/" searches every line in any mode.
		SLATE_ARGUMENT(bool, bCategories)
		SLATE_ARGUMENT(FString, InitialGroup)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& G, const FKeyEvent& E) override;
	virtual FReply OnKeyChar(const FGeometry& G, const FCharacterEvent& E) override;

private:
	TArray<FS47DevEntry> Entries;
	TFunction<void()> OnBack;
	int32 Sel = 0;
	TArray<TSharedPtr<SWidget>> Rows;
	TSharedPtr<class SScrollBox> Scroll;
	void Choose();
	void Move(int32 Dir);
	// What the list shows now: an entry, or (Entry<0) a group to open.
	struct FShown { int32 Entry = INDEX_NONE; FString Group; int32 Count = 0; };
	TArray<FShown> Shown;
	bool bCategories = false, bSearching = false;
	FString OpenGroup, Search;
	TSharedPtr<class SVerticalBox> ListBox;
	void Rebuild();
	FString HelpLine() const;
};
