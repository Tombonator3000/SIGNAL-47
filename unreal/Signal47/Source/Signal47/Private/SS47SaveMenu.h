#pragma once

#include "CoreMinimal.h"
#include "S47Saves.h"
#include "Widgets/SCompoundWidget.h"

class US47UI;
class SVerticalBox;

// ui/SaveMenu.ts: the case menus. Load a save, save to one of the case's manual slots, or pick a
// case slot for a new night. Saves are cards with a small picture, the chapter, the place, the
// night's clock, play time and the real date. Overwriting and deleting ask first.
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

private:
	TWeakObjectPtr<US47UI> UI;
	FS47SaveMenuOpts O;
	TArray<FS47SaveMeta> Metas;
	// a question in place of a card's buttons (confirmIn), so nothing is overwritten or deleted by one stray click
	struct FAsk { FString Host, Text, Yes; TFunction<void()> Act; };
	TOptional<FAsk> Ask;
	void Render(const FString& Status = FString());
	TArray<FS47SaveMeta> CaseMetas(int32 C) const;
	TSharedRef<SWidget> Card(const FS47SaveMeta& M, TSharedRef<SWidget> Buttons) const;
	TSharedRef<SWidget> Buttons(const FString& Host, TArray<TSharedRef<SWidget>> Normal);
	void ConfirmIn(const FString& Host, const FString& Text, const FString& Yes, TFunction<void()> Act);
	void Close(bool bSilent = false);
};
