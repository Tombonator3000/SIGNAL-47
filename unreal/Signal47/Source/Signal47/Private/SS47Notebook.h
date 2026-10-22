#pragma once

#include "CoreMinimal.h"
#include "S47Docs.h"
#include "S47Story.h"
#include "S47UI.h"
#include "Widgets/SCompoundWidget.h"

class SButton;

// The journal (ui/UI.ts notebook): in tabs, the case board first (when there is one), tonight's
// tasks, the observations, the findings recorded so far, the papers, the photographs (Album),
// the map, the kit, the people and every control. The last tab used is kept (UI.NbTab). On
// paper, the hand in Reenie Beanie, as style.css .notebook has it.
class SS47Notebook : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Notebook) {}
		SLATE_ARGUMENT(US47UI*, UI)
		SLATE_ARGUMENT(TArray<FS47Task>, Tasks)
		SLATE_ARGUMENT(TArray<FString>, Notes)
		SLATE_ARGUMENT(TArray<FS47Doc>, Docs)
		SLATE_ARGUMENT(TFunction<void(const FS47Doc&)>, OnDoc)
		SLATE_ARGUMENT(FString, Heading)
		SLATE_ARGUMENT(TFunction<void()>, OnBoard)
		SLATE_ARGUMENT(FS47NbExtras, Extras)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& G, const FKeyEvent& E) override;
	void Show(const FString& T);

	// the excerpt a finding shows on its line (UI.ts excerpt), for the tests too
	static FString Excerpt(const FS47Doc& D);

private:
	TWeakObjectPtr<US47UI> UI;
	FString Tab = TEXT("tasks");
	TSet<FString> Has;      // the pages this notebook has
	TFunction<void()> OnBoard;
	void Close();
	TSharedRef<SWidget> TabButton(const FString& T, const FString& Label, int32 Count);
	TSharedRef<SWidget> Page(const FString& T, bool bRuled, TSharedRef<SWidget> Body);
};
