#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "S47Docs.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

struct FSlateBrush;
struct FS47ReadingAction;
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
	FSlateFontInfo Body(float Px);    // Special Elite: the web version's --type (body text, toasts, papers)
	FSlateFontInfo Hand(float Px);    // Reenie Beanie: --hand (handwriting, notebook, pencil)
	FSlateFontInfo Display(float Px); // Oswald: --display (titles, chapter cards)
	const FLinearColor Amber(1.0f, 0.68f, 0.22f);
	const FLinearColor Green(0.55f, 1.0f, 0.64f);
	const FLinearColor GreenDim(0.35f, 0.7f, 0.42f);
	const FLinearColor Paper(0.93f, 0.9f, 0.84f);
	const FLinearColor Ink(0.12f, 0.1f, 0.08f);
	const FLinearColor Pencil(0.3f, 0.28f, 0.3f);
	const FLinearColor Screen(0.008f, 0.06f, 0.025f);
	const FLinearColor Overlay(0.02f, 0.02f, 0.02f, 0.92f);
	// style.css
	const FLinearColor InkBlue = FLinearColor::FromSRGBColor(FColor(0x23, 0x3a, 0x7a));
	const FLinearColor Phosphor = FLinearColor::FromSRGBColor(FColor(0x8c, 0xff, 0xa4));
}

// The overlay while walking (mock/hud.html): the objective top left under its place and clock, the
// wristwatch top right, the ring at the centre with the use prompt under it (a keycap, the label,
// the second thing on a line of its own), the captions at the foot on a dark band (the speaker in
// amber over the line), the slip that says a line went into the notebook, the key hints, and a fade
// to black with a word on it. The viewfinder and the shutter are drawn over it.
class SS47Hud : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Hud) {}
		SLATE_ATTRIBUTE(FText, Prompt)       // what E does here (UI.prompt's label), empty for nothing
		SLATE_ATTRIBUTE(FText, PromptAlt)    // the second thing (Q), empty for none
		SLATE_ATTRIBUTE(bool, ShowPrompt)
		SLATE_ATTRIBUTE(FText, Clock)
		SLATE_ATTRIBUTE(bool, ShowAim)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	void Toast(const FString& Text, float Secs, TFunction<double()> Clock = {});
	void ClearToasts();
	void Fade(bool bOn, const FString& Text);
	bool IsFaded() const { return bFadeOn; }
	// UI.ts: the objective line (top left), a closer look's words (under the crosshair), the
	// amber seconds clock, the field camera's viewfinder and its shutter
	void Objective(const FString& Text);
	void Examine(const FString& Text) { ExamineText = Text; }
	void ClockSeconds(const FString& Text) { SecondsText = Text; }
	struct FViewfinder { bool bOn = false; FSlateRect Rect; FString Frame, Status, Free, Hint; bool bOk = false, bFreeOk = false; };
	void Viewfinder(const FViewfinder& V) { Vf = V; }
	void ShutterFlash() { FlashT = 0.28f; }
	/** A line written into the notebook (AS47Story::Note): the NOTED slip, bottom left, in turn. */
	void Noted(const FString& Text);
	/** UI.ts lastDescription: the last toast that was not a caption (radio, telephone, the CB). */
	FString LastToast;
	/** .description-read (UI.ts descriptionButton): Read description [V] at the top right, while the
	 *  description reader is on and there is something to read (US47UI sets it). */
	struct FDescButton { bool bShown = false; FString Text; bool bDisabled = false; };
	FDescButton DescButton;
	/** html.at-line (main.ts, during THE EVENT): the prompt sits lower (.at-line .prompt { top: 64% }). */
	bool bAtLine = false;

private:
	struct FToast { FString Text, Who, Line; double Until = 0; TFunction<double()> Clock; float In = 0.f, Out = -1.f; };
	TArray<TSharedRef<FToast>> Toasts;
	TSharedPtr<SVerticalBox> ToastBox;
	TUniquePtr<FSlateBrush> WhiteBrush;
	TAttribute<FText> PromptAttr, AltAttr, ClockAttr;
	TAttribute<bool> ShowPromptAttr, ShowAimAttr;
	bool bFadeOn = false;
	float FadeAlpha = 0.f;
	FString FadeText;
	double Now = 0;
	FString ObjectiveText, ObjectiveHead, ObjectiveBody, ExamineText, SecondsText;
	float ObjectiveIn = 1.f;          // the objective coming in after it changed
	FString Place;                    // where the player is (placeName), for the objective's tag
	float PlaceT = 0.f;
	FString HintKey;                  // the key hints shown, to rebuild them when they change
	TSharedPtr<class SHorizontalBox> HintRow;
	FViewfinder Vf;
	float FlashT = 0.f;
	// the NOTED slips: the one showing (its time on screen) and the ones waiting
	TArray<FString> Slips;
	float SlipT = -1.f;
	TSharedPtr<class SBox> SlipSlot;
	void Rebuild();
	void ShowSlip();
	bool Playing() const;
	bool ModalOpen() const;
};

namespace S47WidgetInput { bool SelfTest(TArray<FString>& Lines); }

// A paper (mock/document.html): the sheet large and a little turned over the blurred game, as it is
// written (typed, hand or printout; a photograph or a map is the picture), with the side panel: what
// it is, its title, its description (the transcript) and the actions (READ ALOUD with V when a reader
// has it, PUT IT DOWN with Esc). Lines that start with "~ " are pencil. Esc, E, Tab or the action
// puts it down; long papers scroll.
class SS47Document : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Document) {}
		SLATE_ARGUMENT(FS47Doc, Doc)
		SLATE_ARGUMENT(TFunction<void()>, OnClose)
		SLATE_ARGUMENT(class US47UI*, UI) // the readers (Read document, Read description); none: no reading
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual ~SS47Document() override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	static bool SelfTest(TArray<FString>& Lines);

private:
	TFunction<void()> OnClose;
	struct FAction { FString Label, Key; TFunction<void()> Fn; TFunction<FString()> Live; TSharedPtr<FS47ReadingAction> Reading; };
	TArray<FAction> Actions;
	int32 Chosen = 0;
	TSharedPtr<FS47ReadingAction> ReadDoc, ReadDesc;
	TSharedPtr<class SScrollBox> PageScroll;
	TSharedRef<SWidget> PageWidget(const FS47Doc& Doc);
	void Act(int32 I);
};

// (the notebook is SS47Notebook.h, ported from ui/UI.ts notebook)

// The receiver console (SARO / RX CONTROL 03, ui/RxConsole.ts): the spectrum, the four controls,
// the card and the action button, as the web version's .crt has them, drawn for the CRT's 4:3
// glass (SS47ScreenView puts it on the centre screen and sends it the mouse and the keys; without
// the room it stands over the game in a beige bezel). Edits the story's receiver directly.
class SS47Console : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Console) : _Story(nullptr) {}
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
	TSharedRef<SWidget> Control(const TCHAR* Label, TFunction<double()> Get, TFunction<void(double)> Set, double Min, double Max, TFunction<FString(double)> Fmt, bool bFine);
	bool Usable() const;
};

// The spectrum trace as a widget: the receiver console's and the centre CRT's picture. The trace
// is worked out on the web version's canvas (Canvas: 900 x 300 on the console, the CRT's 512 x 384)
// and stretched to the widget, as the browser stretches the canvas to its box.
class SS47Spectrum : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Spectrum) : _Story(nullptr), _Canvas(FVector2D::ZeroVector) {}
		SLATE_ARGUMENT(AS47Story*, Story)
		SLATE_ARGUMENT(FVector2D, Size)
		SLATE_ARGUMENT(FVector2D, Canvas)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return Size; }
	static void Paint(const AS47Story* Story, const FGeometry& G, FSlateWindowElementList& Out, int32 Layer, const FSlateBrush* White, FVector2D Canvas = FVector2D::ZeroVector);

private:
	AS47Story* Story = nullptr;
	FVector2D Size = FVector2D(900, 300), Canvas = FVector2D::ZeroVector;
	TUniquePtr<FSlateBrush> WhiteBrush;
};

// The operations terminal (SARO OPERATIONS SYSTEM V4.2, ui/Terminal.ts): the output with the
// command line under it, the commands as buttons, LOG OUT. Esc or LOGOUT closes. On the west desk's
// CRT through SS47ScreenView, the line taking the keys.
class SS47Ops : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Ops) : _Story(nullptr) {}
		SLATE_ARGUMENT(AS47Story*, Story)
		SLATE_ARGUMENT(TFunction<void()>, OnClose)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;
	/** The command line (the screen view gives it the keys). */
	TSharedPtr<SWidget> Line() const;

	virtual FReply OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	static bool SelfTest(TArray<FString>& Lines);

private:
	AS47Story* Story = nullptr;
	TFunction<void()> OnClose;
	FString Text;
	TSharedPtr<SScrollBox> Scroll;
	TSharedPtr<class SEditableText> Input;
	TSharedPtr<class SWrapBox> Buttons;
	TArray<FString> PadCommands;
	int32 PadCommand = INDEX_NONE;
	void Exec(const FString& Cmd);
	void RebuildButtons();
};

// A CRT's picture for SS47Console and SS47Ops: the phosphor's dark green, the tube darker at its
// edges with a faint green rim, the scan lines over everything and the glass's round corners.
// Without the room (bBezel) it also has the web version's beige bezel and its shadow.
class SS47CrtGlass : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47CrtGlass) : _bBezel(false), _Radius(22.f), _Ring(14.f), _Edge(2.f), _Scan(0.22f) {}
		SLATE_ARGUMENT(bool, bBezel)
		SLATE_ARGUMENT(float, Radius)   // the glass's corners, mockup px
		SLATE_ARGUMENT(float, Ring)     // the bezel: its beige ring and the darker edge round it
		SLATE_ARGUMENT(float, Edge)
		SLATE_ARGUMENT(float, Scan)     // the scan lines' darkness
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
	// the glass keeps its clicks (over the game, a click beside it closes the screen)
	virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }

private:
	bool bBezel = false;
	float Radius = 22.f, Ring = 14.f, Edge = 2.f, Scan = 0.22f;
	FSlateRect Glass(const FGeometry& G) const;
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
