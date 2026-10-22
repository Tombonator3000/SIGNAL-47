#pragma once

#include "CoreMinimal.h"
#include "S47Saves.h"
#include "Widgets/SCompoundWidget.h"

class US47UI;
struct FSlateBrush;

// ui/SaveMenu.ts: the case menus. Load a save, save to one of the case's manual slots, or pick a
// case slot for a new night. Overwriting and deleting ask first.
//
// In the approved look (load.png): three case cards side by side, each with its picture, the
// play time, the chapter, the place and when it was saved, and the case's saves listed under it
// to choose from; an empty case shows NO TAPE. Saving shows the case's three manual saves the
// same way. Left and right go between the cards, up and down between a case's saves, Enter
// loads (or saves, or starts the night), Delete erases, Escape goes back; the mouse chooses by
// hovering and clicking.
struct FS47SaveMenuOpts
{
	FString Mode;                                    // 'load' | 'save' | 'new'
	TArray<FS47SaveMeta> Metas;
	int32 ActiveCase = 0;                            // 0: null
	FString Block;                                   // save mode: why saving is not possible right now
	bool bFallback = false;                          // the photo store could not be used
	TFunction<void(const FS47SaveMeta&)> OnLoad;
	TFunction<TOptional<FS47SaveMeta>(int32 Slot, FString& OutError)> OnSave;
	TFunction<bool(const FS47SaveMeta&, FString& OutError)> OnDelete;
	TFunction<void(int32 CaseId)> OnNewCase;
	TFunction<void()> OnBack;
};

namespace S47SaveMenu
{
	// saveMenu(ui, o): opens the menu as the UI's modal
	void Open(US47UI* UI, const FS47SaveMenuOpts& O);
	FString When(double Ms);                          // "Oct 22, 14:05" (en-US, 24 hours, local time)
	FString KindName(const FS47SaveMeta& M);          // AUTOSAVE 1, SAVE 2
}

class SS47SaveMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47SaveMenu) {}
		SLATE_ARGUMENT(US47UI*, UI)
		SLATE_ARGUMENT(FS47SaveMenuOpts, Opts)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& G, const FKeyEvent& E) override;
	virtual FNavigationReply OnNavigation(const FGeometry& G, const FNavigationEvent& E) override;
	virtual void Tick(const FGeometry& G, const double InCurrentTime, const float InDeltaTime) override;

private:
	TWeakObjectPtr<US47UI> UI;
	FS47SaveMenuOpts O;
	TArray<FS47SaveMeta> Metas;
	int32 Card = 0, Row = 0;           // the card chosen (0..2) and the save in it
	FString Status;
	bool bStatusWarn = false;
	// a question over the cards (confirmIn), so nothing is overwritten or deleted by one stray key
	struct FAsk { FString Text, Yes; TFunction<void()> Act; };
	TOptional<FAsk> Ask;

	bool Saving() const { return O.Mode == TEXT("save"); }
	int32 SaveCase() const { return O.ActiveCase ? O.ActiveCase : 1; }
	// what a card holds: a case's saves, newest first (load, new), or the save in a manual slot (save)
	TArray<FS47SaveMeta> CardMetas(int32 C) const;
	const FS47SaveMeta* Shown(int32 C) const;          // the save the card shows: the one chosen in it, else its newest
	void Render();
	TSharedRef<SWidget> CardWidget(int32 C);
	void Pick(int32 C, int32 R);
	void Activate();
	void Erase();
	void Confirm(const FString& Text, const FString& Yes, TFunction<void()> Act);
	void Close(bool bSilent = false);
};
