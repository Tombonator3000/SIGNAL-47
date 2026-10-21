#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "S47Docs.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

struct FSlateBrush;
class AS47Story;
class SScrollBox;
class SEditableTextBox;
class SVerticalBox;

// The game's fonts and colours, read from Content/Fonts (VT323 for screens, OFL; a mono
// fallback when it is not there).
namespace S47Style
{
	FSlateFontInfo Font(const TCHAR* File, float Px);
	FSlateFontInfo Mono(float Px);
	FSlateFontInfo Body(float Px);
	const FLinearColor Amber(1.0f, 0.68f, 0.22f);
	const FLinearColor Green(0.55f, 1.0f, 0.64f);
	const FLinearColor GreenDim(0.35f, 0.7f, 0.42f);
	const FLinearColor Paper(0.93f, 0.9f, 0.84f);
	const FLinearColor Ink(0.12f, 0.1f, 0.08f);
	const FLinearColor Pencil(0.3f, 0.28f, 0.3f);
	const FLinearColor Screen(0.008f, 0.06f, 0.025f);
	const FLinearColor Overlay(0.02f, 0.02f, 0.02f, 0.92f);
}

// The overlay while walking: a dot, the prompt for what you look at, the station clock, the
// toasts (lines that appear for a few seconds), and a fade to black with a word on it.
class SS47Hud : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Hud) {}
		SLATE_ATTRIBUTE(FText, Prompt)
		SLATE_ATTRIBUTE(FText, Clock)
		SLATE_ATTRIBUTE(bool, ShowAim)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	void Toast(const FString& Text, float Secs);
	void ClearToasts();
	void Fade(bool bOn, const FString& Text);
	bool IsFaded() const { return bFadeOn; }

private:
	struct FToast { FString Text; double Until; };
	TArray<FToast> Toasts;
	TSharedPtr<SVerticalBox> ToastBox;
	TUniquePtr<FSlateBrush> DotBrush;
	TUniquePtr<FSlateBrush> WhiteBrush;
	bool bFadeOn = false;
	float FadeAlpha = 0.f;
	FString FadeText;
	double Now = 0;
	void Rebuild();
};

// A paper, full screen: the page on the left as it is written (typed, hand or printout), the
// transcript on the right. Lines that start with "~ " are pencil. Esc, E or the button closes.
class SS47Document : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Document) {}
		SLATE_ARGUMENT(FS47Doc, Doc)
		SLATE_ARGUMENT(TFunction<void()>, OnClose)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	TFunction<void()> OnClose;
	TUniquePtr<FSlateBrush> PaperBrush;
	TUniquePtr<FSlateBrush> PanelBrush;
};

// The notebook (Tab): tonight's tasks, the notes, the papers filed so far.
class SS47Notebook : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Notebook) {}
		SLATE_ARGUMENT(AS47Story*, Story)
		SLATE_ARGUMENT(TFunction<void()>, OnClose)
		SLATE_ARGUMENT(TFunction<void(const FS47Doc&)>, OnDoc)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	TFunction<void()> OnClose;
	TUniquePtr<FSlateBrush> PaperBrush;
};

// The receiver console (SARO / RX CONTROL 03): the spectrum, the four controls, the card and
// the action button. Edits the story's receiver directly.
class SS47Console : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Console) {}
		SLATE_ARGUMENT(AS47Story*, Story)
		SLATE_ARGUMENT(TFunction<void()>, OnClose)
		SLATE_ARGUMENT(TFunction<void()>, OnClick)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	AS47Story* Story = nullptr;
	TFunction<void()> OnClose, OnClick;
	TUniquePtr<FSlateBrush> PanelBrush;
	TSharedRef<SWidget> Control(const TCHAR* Label, TFunction<double()> Get, TFunction<void(double)> Set, double Min, double Max, TFunction<FString(double)> Fmt, bool bFine);
};

// The spectrum trace as a widget: the receiver console's and the centre CRT's picture.
class SS47Spectrum : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Spectrum) {}
		SLATE_ARGUMENT(AS47Story*, Story)
		SLATE_ARGUMENT(FVector2D, Size)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return Size; }
	static void Paint(const AS47Story* Story, const FGeometry& G, FSlateWindowElementList& Out, int32 Layer, const FSlateBrush* White);

private:
	AS47Story* Story = nullptr;
	FVector2D Size = FVector2D(900, 300);
	TUniquePtr<FSlateBrush> WhiteBrush;
};

// The operations terminal (SARO OPERATIONS SYSTEM V4.2): the output, the command line and the
// commands as buttons. Esc or LOGOUT closes.
class SS47Ops : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Ops) {}
		SLATE_ARGUMENT(AS47Story*, Story)
		SLATE_ARGUMENT(TFunction<void()>, OnClose)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	AS47Story* Story = nullptr;
	TFunction<void()> OnClose;
	FString Text;
	TSharedPtr<SScrollBox> Scroll;
	TSharedPtr<class STextBlock> Out;
	TSharedPtr<SEditableTextBox> Input;
	TSharedPtr<class SWrapBox> Buttons;
	TUniquePtr<FSlateBrush> PanelBrush;
	void Exec(const FString& Cmd);
	void RebuildButtons();
};

// The card at the end of the prologue.
class SS47EndCard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47EndCard) {}
		SLATE_ARGUMENT(TArray<FString>, Lines)
		SLATE_ARGUMENT(TFunction<void()>, OnAgain)
		SLATE_ARGUMENT(TFunction<void()>, OnQuit)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
};
