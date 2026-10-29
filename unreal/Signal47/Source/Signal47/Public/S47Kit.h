#pragma once

#include "CoreMinimal.h"
#include "Types/ISlateMetaData.h"
#include "Widgets/SWidget.h"

class US47UI;
class UTexture2D;
struct FSlateBrush;

// UI.ts descriptionReadings finds a panel's descriptions by their classes (.lp-text, .lp-small, .lp-q,
// .lp-paper p, .lp-steps span, .lp-card span); here the pieces carry this mark instead. bRead: a text to
// read; not: nothing under it is read (.read-aloud, [data-character-quote]).
class FS47ReadingMark : public ISlateMetaData
{
public:
	SLATE_METADATA_TYPE(FS47ReadingMark, ISlateMetaData)
	explicit FS47ReadingMark(bool bInRead) : bRead(bInRead) {}
	bool bRead = true;
};

// The pieces the web version's panels are made of (style.css .labpanel, .lp-*), so a panel in
// ui/Panels.ts ports to Slate piece for piece. A panel is Shell(title, body) opened with
// UI->Open(...); its close button and a click outside close it.
//
//   UI->Open(S47Kit::Shell(UI, TEXT("WET BENCH"), S47Kit::Col({
//       S47Kit::Sub(Frame),
//       S47Kit::Steps(StepList, Active),
//       S47Kit::Button(TEXT("LOAD SEALED FILM INTO TANK"), [=] { UI->Close(); OnAct(); }, true) })));
namespace S47Kit
{
	using FW = TSharedRef<SWidget>;
	struct FResult { bool bOk = false; FString Text; };

	// the dark panel with its heading (h3) and Close
	// bLow: a dialogue line, at the lower middle so it covers less of the world (Tom 28.10.2026).
	SIGNAL47_API FW Shell(US47UI* UI, const FString& Title, FW Body, float Width = 1040.f, bool bLow = false);

	// text (.lp-text, .lp-sub, .lp-small, .lp-q); Tone: 0 plain, 1 good, -1 bad
	SIGNAL47_API FW Text(const FString& S, int32 Tone = 0);
	SIGNAL47_API FW Text(TAttribute<FString> S, TAttribute<int32> Tone);
	SIGNAL47_API FW Sub(const FString& S);
	SIGNAL47_API FW Small(const FString& S);
	SIGNAL47_API FW Question(const FString& S);
	// a paper inside a panel (.lp-paper): typed paragraphs on paper colour
	SIGNAL47_API FW Paper(const FString& S, bool bReport = false);

	// the steps of a procedure (.lp-steps): done ones struck through, the current one lit
	SIGNAL47_API FW Steps(const TArray<TPair<FString, FString>>& Items, int32 Active);

	// buttons (.lp-btn, .lp-btn.primary) and a choice card (.lp-card: a title and a line)
	SIGNAL47_API FW Button(const FString& Label, TFunction<void()> OnClick, bool bPrimary = false, TAttribute<bool> Enabled = true);
	SIGNAL47_API FW Card(const FString& Title, const FString& Line, TFunction<void()> OnClick, TAttribute<bool> Enabled = true);

	// layout (.lp-row, .lp-col, .lp-two, .lp-cards)
	SIGNAL47_API FW Row(const TArray<FW>& Children);
	SIGNAL47_API FW Col(const TArray<FW>& Children, float Gap = 8.f);
	SIGNAL47_API FW Two(FW Left, FW Right, float LeftShare = 1.f, float RightShare = 1.f);
	SIGNAL47_API FW Cards(const TArray<FW>& Children);

	// a labelled slider (.lp-zoom label + input range)
	SIGNAL47_API FW Slider(const FString& Label, float Min, float Max, float Step, TAttribute<float> Value, TFunction<void(float)> OnChange);
	// a knob (.dsp-knobs label: the name and its value in amber over the range under it)
	SIGNAL47_API FW Knob(const FString& Label, TAttribute<FString> Shown, float Min, float Max, float Step, TAttribute<float> Value, TFunction<void(float)> OnChange);
	// a button whose label follows the state (.lp-btn whose textContent sync() sets)
	SIGNAL47_API FW ButtonLive(TAttribute<FString> Label, TFunction<void()> OnClick, bool bPrimary = false, TAttribute<bool> Enabled = true);
	// a choice that stays pressed (.lp-choice[aria-pressed]); bAuto: as wide as its label (.dsp-mode)
	SIGNAL47_API FW Choice(const FString& Label, TAttribute<bool> Pressed, TFunction<void()> OnClick, bool bAuto = false);

	// a picture at a width (its height from the picture), and the brush it needs kept alive
	SIGNAL47_API FW Image(UTexture2D* T, float Width);
	SIGNAL47_API TSharedPtr<FSlateBrush> BrushFor(UTexture2D* T);

	// colours and fonts of the lab panels (style.css)
	SIGNAL47_API FLinearColor Css(uint32 Rgb, float A = 1.f);

	// the reading marks (FS47ReadingMark): a text the description reader reads, a part it leaves alone
	SIGNAL47_API FW Readable(FW W);
	SIGNAL47_API FW Unread(FW W);
}
