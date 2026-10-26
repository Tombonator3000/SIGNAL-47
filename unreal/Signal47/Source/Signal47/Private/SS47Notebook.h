#pragma once

#include "CoreMinimal.h"
#include "S47Docs.h"
#include "S47Story.h"
#include "S47UI.h"
#include "Widgets/SCompoundWidget.h"

class SBox;
class SScrollBox;
struct FS47ReadingAction;

// The journal (ui/UI.ts notebook) as the approved mockup has it (mock/notebook.html): a leather
// notebook open over the blurred game, two lined pages on a spiral, and coloured paper tabs down
// the right-hand edge: TASKS, NOTES, FINDINGS, PAPERS, ALBUM, MAP, KIT, PEOPLE and CONTROLS, and the
// case BOARD, which opens its own screen. The left page lists a tab's entries, the right page shows
// the chosen one (a drawing of the kit, a paper, a finding, a face); the map lies across the spread.
// Q and E turn the tabs, the arrows choose, Enter opens, V reads aloud, Tab or Esc closes; the mouse
// clicks the tabs and the entries and the wheel scrolls. The last tab used is kept (UI.NbTab). The
// hand in Reenie Beanie, the typed in Special Elite, as style.css .notebook has them.
class SS47Notebook : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Notebook) : _UI(nullptr) {}
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
	virtual ~SS47Notebook() override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& G, const FKeyEvent& E) override;
	// the CONTROLS page built again when a key is bound anew (S47Keys::Changes)
	virtual void Tick(const FGeometry& G, const double Now, const float Dt) override;
	void Show(const FString& T);

	// the excerpt a finding shows on its line (UI.ts excerpt), for the tests too
	static FString Excerpt(const FS47Doc& D);
	// the keys on a notebook made of sample pages (Q and E, the arrows, Enter)
	static bool SelfTest(TArray<FString>& Lines);

private:
	TWeakObjectPtr<US47UI> UI;
	FString Tab = TEXT("tasks");
	TArray<FString> Order;       // the tabs this notebook has, top to bottom
	TFunction<void()> OnBoard;
	TFunction<void(const FS47Doc&)> OnDoc;
	FString Heading;
	TArray<FS47Task> Tasks;
	TArray<FString> Notes;
	TArray<FS47Doc> Photos, Findings, Papers;
	FS47NbExtras X;
	TMap<FString, int32> Sel;    // the chosen entry on each tab
	uint32 KeysShown = 0;        // S47Keys::Changes() when the CONTROLS page was built
	uint32 PadShown = 0;         // active input when the control pages were built

	// the spread: the left page, the right page, a sheet across both (the map), the actions at the foot
	TSharedPtr<SBox> LeftSlot, RightSlot, SpreadSlot, UseSlot;
	TSharedPtr<SScrollBox> LeftScroll, RightScroll;
	TArray<TSharedPtr<SWidget>> Entries;   // the entries that can be chosen, in order (scrolled into view)
	TSharedPtr<FS47ReadingAction> Reading; // V: what this spread reads aloud
	bool bRightIsEntries = false;          // the album's right page holds entries too

	void Close();
	void Build();                // the spread for Tab
	void BuildRight();           // the right page for the chosen entry, and the actions
	int32 Count() const;         // how many entries the tab has to choose from
	int32 Columns() const;       // and how many to a row
	int32 Chosen() const { const int32* S = Sel.Find(Tab); return S ? *S : 0; }
	void Choose(int32 I);
	void Activate();             // Enter: what the chosen entry opens
	void Turn(int32 Dir);        // Q and E: the tab before or after
	void Press(const FKey& Key); // the notebook closed, the game's own key (C raises the camera, F the light)
	void Scroll(float By);
	TSharedRef<SWidget> Entry(int32 I, const TSharedRef<SWidget>& Content, bool bCard = false);
	TSharedRef<SWidget> TabStack();
	TSharedRef<SWidget> UseRow(const TArray<TTuple<FString, FString, TFunction<void()>>>& Acts);

	// ui/album.ts: the free prints, and one of them opened big over the notebook (big()), with the
	// previous and the next beside it; putting it back leaves the album open
	TOptional<FS47AlbumView> Album;
	int32 Big = INDEX_NONE;
	TSharedPtr<SBox> BigSlot;
	TSharedRef<SWidget> Print(int32 I, float W, bool bBig);
	void ShowBig(int32 I);
	void CloseBig();
};
