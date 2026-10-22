#pragma once

#include "CoreMinimal.h"
#include "S47UI.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SCompoundWidget.h"

class SButton;
class SVerticalBox;
struct FSlateBrush;

// The web version's CSS as Slate pieces for the screens of ui/UI.ts and ui/SaveMenu.ts (style.css:
// .title-screen, .opening-card, .panel, .savemenu, .notebook, .chapter-card). Sizes are the CSS
// pixels times PX, as the lab panels (S47Kit) have them.
namespace S47Css
{
	constexpr float PX = 1.25f;
	FLinearColor C(uint32 Rgb, float A = 1.f);           // a CSS colour (sRGB) as Slate's
	FSlateFontInfo Term(float Px);                        // --term: VT323
	FSlateFontInfo Type(float Px);                        // --type: Special Elite
	FSlateFontInfo Hand(float Px);                        // --hand: Reenie Beanie
	FSlateFontInfo Display(float Px);                     // --display: Oswald
	const FSlateBrush* Box(const FLinearColor& Fill, const FLinearColor& Line = FLinearColor::Transparent, float LineW = 0.f, float Radius = 0.f);
	const FButtonStyle* Flat();                           // a button that draws nothing of its own (button { background: none })
	// a button that is its text: Color, Hover (and keyboard focus), Off when disabled; Arrow: the
	// title's '>' in front while hovered or focused
	TSharedRef<SButton> TextButton(const FString& Label, const FSlateFontInfo& Font, const FLinearColor& Color, const FLinearColor& Hover,
		TFunction<void()> OnClick, TAttribute<bool> Enabled = true, const FLinearColor& Off = FLinearColor(0.15f, 0.15f, 0.15f), bool bArrow = false, const FMargin& Pad = FMargin(0, 6 * PX));
	// a button with a label that can change (the settings' On / Off)
	TSharedRef<SButton> TextButton(TAttribute<FText> Label, const FSlateFontInfo& Font, const FLinearColor& Color, const FLinearColor& Hover,
		TFunction<void()> OnClick, const FMargin& Pad = FMargin(0, 6 * PX));
	// a framed button (.save-btns button: amber, or .quiet: outlined)
	TSharedRef<SButton> BoxButton(const FString& Label, const FSlateFontInfo& Font, const FLinearColor& Text, const FLinearColor& Fill, const FLinearColor& Line, TFunction<void()> OnClick, const FMargin& Pad);
	FString Played(double Sec);                           // SaveMenu.ts played()
}

// .title-screen: the name, the line under it and the menu, at the foot of the picture with the
// title camera drifting behind. Gated: only the name and "Click or press any key" until the
// first key or click (main.ts firstTouch), then the menu comes in.
class SS47Title : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Title) {}
		SLATE_ARGUMENT(FS47TitleOpts, Opts)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& G, const double InCurrentTime, const float InDeltaTime) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& G, const FKeyEvent& E) override;
	virtual FReply OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override;
	void Ungate();
	bool IsGated() const { return bGated; }
	void FocusMenu();
	void FocusStart();

private:
	FS47TitleOpts O;
	bool bGated = false, bCalm = false;
	float T = 0.f, InT = 1.f; // the gate's blink, the menu's coming in
	bool bFocusPending = true; // the menu is focused once the title is on the screen
	FVector2D Size = FVector2D(1920, 1080);
	TSharedPtr<SButton> ContButton, StartButton;
	void First();
};

// .opening-card: the five paragraphs on the black, Begin night and Back to title (UI.openingCard)
class SS47Opening : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Opening) {}
		SLATE_ARGUMENT(FS47OpeningActions, Actions)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& G, const FKeyEvent& E) override;
	void SetState(const FString& State);
	void FocusBegin();

private:
	FS47OpeningActions A;
	FString Status;
	TSharedPtr<SButton> BeginButton, BackButton;
};

// .panel.settings-panel: the pause menu, or the title's settings (UI.pause)
class SS47Pause : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Pause) {}
		SLATE_ARGUMENT(FS47PauseOpts, Opts)
		SLATE_ARGUMENT(TFunction<void(bool)>, OnClose) // the UI's close (silent or not)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& G, const FKeyEvent& E) override;

private:
	FS47PauseOpts O;
	TFunction<void(bool)> OnClose;
	bool bArtBell = true, bRadioCast = true, bCharacters = true, bSerling = true, bKinski = false, bInv = false, bBig = false, bCalm = false, bStill = false, bAb = true;
	FString Q, Pic;
	float Vol = 0.8f, Mus = 0.7f, Rad = 1.f, Sens = 1.f, Fov = 70.f;
};

// .chapter-card: what was just closed and its line, then the next chapter's title and the clock,
// on the black over the game (the night does not stop for it)
class SS47ChapterCard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47ChapterCard) {}
		SLATE_ARGUMENT(FS47ChapterCardOpts, Opts)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& G, const double InCurrentTime, const float InDeltaTime) override;
	void Out(bool bCut) { bOut = true; bCutOut = bCut; OutT = 0.f; }
	bool Gone() const { return bOut && OutT >= (bCutOut ? 0.1f : 0.8f); }
	/** .voiced[data-voice-phase]: the narrator reads it, so the halves come as the reading reaches them
	 *  ('a' the closed line and recap, 'b' the next chapter), not on the animations' times. */
	void Voice(const FString& Phase) { VoicePhase = Phase; }

private:
	FString VoicePhase;
	float T = 0.f, OutT = 0.f;
	bool bOut = false, bCutOut = false;
	float AlphaA() const;
	float AlphaB() const;
	float AlphaOut() const { return bOut ? FMath::Clamp(1.f - OutT / (bCutOut ? 0.1f : 0.8f), 0.f, 1.f) : 1.f; }
};
