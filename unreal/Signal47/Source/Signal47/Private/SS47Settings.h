#pragma once

#include "CoreMinimal.h"
#include "S47UI.h"
#include "Widgets/SCompoundWidget.h"

class SScrollBox;
class SVerticalBox;

// The settings, one screen for the title's Settings and the pause menu's (the approved mockup,
// settings.png): the terminal's frame, the tabs GAME / PICTURE / SOUND / VOICES / CONTROLS /
// ACCESSIBILITY / GRAPHICS, the rows on the left and what the selected one does on the right, the keys at
// the foot, APPLY and REVERT for the display. Every setting of FS47PauseOpts is here, each
// change goes to its callback at once (and so into the store), as UI.pause had it; the display
// (window mode, resolution, frame limit, v-sync) is UGameUserSettings', applied with APPLY and
// kept or put back after ten seconds. The keys (S47Keys) are bound on the Controls tab.
class SS47Settings : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Settings) {}
		SLATE_ARGUMENT(TSharedPtr<FS47PauseOpts>, Opts)
		SLATE_ARGUMENT(FString, Tab)                // game, picture, sound, voices, controls, accessibility, graphics
		SLATE_ARGUMENT(TFunction<void()>, OnBack)   // Escape: the UI's close
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& G, const FKeyEvent& E) override;
	virtual FReply OnPreviewMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override;
	virtual FNavigationReply OnNavigation(const FGeometry& G, const FNavigationEvent& E) override;
	virtual void Tick(const FGeometry& G, const double InCurrentTime, const float InDeltaTime) override;

	static const TArray<FString>& TabIds();
	static int32 TabIndex(const FString& Id);   // -1 when it is none of them
	FString ActiveTabId() const;
	FString SelectedRowLabel() const;

private:
	struct FRow
	{
		enum class EKind : uint8 { Choice, Slider, Toggle, Keys, Action, Info, Sep };
		EKind Kind = EKind::Choice;
		FString Label;
		TFunction<FString()> Value;         // a choice's or a slider's words
		TFunction<float()> Frac;            // a slider's fill
		TFunction<void(float)> SetFrac;     // the mouse on the segments
		TFunction<bool()> On;               // a toggle
		TFunction<void(int32)> Step;        // left and right
		TFunction<void()> Activate;         // Enter or a click
		TFunction<void()> Default;          // R
		TFunction<FString()> Title, Text;   // the description on the right
		TFunction<bool()> Usable;
		FName Action;                       // a key row's
		bool bDisplay = false;              // applied with APPLY
	};
	struct FDisplay
	{
		int32 Mode = 0;                     // 0 fullscreen, 1 borderless, 2 windowed
		FIntPoint Res = FIntPoint(1920, 1080);
		float Fps = 0.f;                    // 0: unlimited
		bool bVSync = false;
		bool operator==(const FDisplay& B) const { return Mode == B.Mode && Res == B.Res && Fps == B.Fps && bVSync == B.bVSync; }
	};

	TSharedPtr<FS47PauseOpts> O;
	TFunction<void()> OnBack;
	int32 Tab = 0, Sel = 0;
	TArray<FRow> Rows;
	TArray<TSharedPtr<SWidget>> RowWidgets;
	TSharedPtr<SVerticalBox> RowBox;
	TSharedPtr<SScrollBox> Scroll;
	int32 BtnSel = 0;                       // with the buttons selected (Sel == Rows.Num()): 0 APPLY, 1 REVERT
	bool bLarge = false, bInv = false;

	FDisplay Shown, Applied, Before;
	TArray<FIntPoint> Resolutions;
	float ConfirmLeft = -1.f;               // seconds left to keep a new display, or -1
	FName Listening;                        // the action whose key is being taken
	int32 KeySlot = 0;
	FString Status;
	float Blink = 0.f;

	void SetTab(int32 T);
	void BuildRows();
	void Move(int32 Dir);
	void Scrolled();
	bool HasButtons() const;
	bool OnButtons() const { return HasButtons() && Sel == Rows.Num(); }
	const FRow* Current() const { return Rows.IsValidIndex(Sel) ? &Rows[Sel] : nullptr; }
	void Back();
	// the display
	void ReadDisplay();
	void ApplyDisplay(const FDisplay& D);
	void Apply();
	void Revert();
	void Keep();
	// the keys
	void Listen(FName Action, int32 Slot);
	void Take(const FKey& K);
	void KeysChanged();
	TSharedRef<SWidget> KeyCaps(FName Action, int32 Index);
	TSharedRef<SWidget> Side();
	TSharedRef<SWidget> Hints();
};
