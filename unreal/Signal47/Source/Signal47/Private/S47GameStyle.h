#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SWidget.h"

struct FSlateBrush;
class UTexture2D;

// The in-game look (the HUD, the notebook and the papers) as the approved mockups have it
// (scratchpad mock/hud.html, notebook.html, document.html and their base.css): paper, ink, the
// player's hand, tape, the red stamps and the keycaps. Sizes are the mockups' pixels at 1600 x 900;
// U turns one into Slate units, so the screens keep the mockups' proportions at any 16:9 size
// (the default DPI curve scales Slate by the screen's height over 1080, and 1080 / 900 = 1.2).
// (The menus' own pieces are in S47Theme; the two are to be one style later.)
namespace S47Game
{
	constexpr float U = 1.2f;
	inline float P(float Mock) { return Mock * U; }
	inline FVector2D P2(float X, float Y) { return FVector2D(X * U, Y * U); }

	// a CSS colour (sRGB) as Slate's
	FLinearColor C(uint32 Rgb, float A = 1.f);
	// base.css :root and the mockups' own
	inline FLinearColor Night() { return C(0x05070d); }
	inline FLinearColor Amber() { return C(0xffb052); }
	inline FLinearColor Paper() { return C(0xebe3cf); }
	inline FLinearColor PaperDim() { return C(0xa39e92); }
	inline FLinearColor Ink() { return C(0x1e2430); }
	inline FLinearColor InkBlue() { return C(0x233a7a); }     // the player's pen
	inline FLinearColor StampRed() { return C(0xa0332a); }
	inline FLinearColor Page() { return C(0xece4cf); }        // a notebook page
	inline FLinearColor Leather() { return C(0x3a2417); }
	inline FLinearColor Hint() { return C(0x8f8a7e); }        // .keys: the words beside the keycaps

	// the fonts at a mockup size, with the letter spacing in mockup pixels (CSS letter-spacing)
	FSlateFontInfo Term(float Px, float Spacing = 0.f);       // VT323: screens, keys, stamps
	FSlateFontInfo Type(float Px, float Spacing = 0.f);       // Special Elite: typewriter
	FSlateFontInfo Hand(float Px, float Spacing = 0.f);       // Reenie Beanie: the player's hand
	FSlateFontInfo Display(float Px, float Spacing = 0.f);    // Oswald Medium: titles
	// STextBlock's LineHeightPercentage that sets a font's lines Pitch mockup pixels apart (the lines on a page)
	float LinePercent(const FSlateFontInfo& Font, float Pitch);

	// a flat or rounded box (Radius in mockup pixels; Radii per corner: top left, top right, bottom right, bottom left)
	const FSlateBrush* Box(const FLinearColor& Fill, const FLinearColor& Line = FLinearColor::Transparent, float LineW = 0.f, float Radius = 0.f);
	const FSlateBrush* Box4(const FLinearColor& Fill, const FVector4& Radii);
	// a soft shadow (box-shadow's blur): a nine-sliced blurred square, Blur mockup pixels at each edge
	const FSlateBrush* Soft(float Blur);
	// a picture's brush, kept for the session
	const FSlateBrush* Picture(UTexture2D* T);

	// a scroll box without the shadows at its ends, and a thin bar in Thumb (the pen's blue on paper)
	const struct FScrollBoxStyle* QuietScroll();
	const struct FScrollBarStyle* QuietBar(const FLinearColor& Thumb);

	// box-shadow: Content over its soft shadow (Offset and Blur in mockup pixels)
	TSharedRef<SWidget> Shadowed(const TSharedRef<SWidget>& Content, float Blur, FVector2D Offset, float Alpha);
	// a turned piece (transform: rotate(Deg)); the layout keeps its unturned box
	TSharedRef<SWidget> Turned(const TSharedRef<SWidget>& W, float Deg);
	// filter: drop-shadow on a cut-out drawing: the drawing over its own silhouette, Down pixels lower
	TSharedRef<SWidget> Drawing(UTexture2D* T, float MaxW, float MaxH, float Down = 5.f, float Alpha = 0.32f);

	// the keycaps (base.css .keys b, hud.html .key, notebook.html .use b)
	enum class EKey : uint8 { Outline, Amber, Ink };
	TSharedRef<SWidget> Keycap(const FString& Key, EKey Style = EKey::Outline, float Px = 22.f);
	// a keycap and its word (.keys span); Dim: the word's colour
	TSharedRef<SWidget> KeyHint(const FString& Key, const FString& Label, float Px = 22.f, const FLinearColor& Word = Hint(), EKey Style = EKey::Outline);
	// a strip of tape (.tape), and a rubber stamp (.st: the word in a red frame, a little crooked)
	TSharedRef<SWidget> Tape(float W = 54.f, float H = 16.f, float Deg = -6.f);
	TSharedRef<SWidget> Stamp(const FString& Text, float Px = 20.f, float Deg = -4.f, const FLinearColor& Color = StampRed());
	// the VHS on-screen display (.osd: the date and the clock with the colours split)
	TSharedRef<SWidget> Osd(TAttribute<FText> Text, float Px = 26.f);

	// the night's date (13 to 14 April 1986): SUN APR 13 before midnight, MON APR 14 after
	FString DayLine(double Clock);
	// the camcorder's: APR.13 1986  23:58
	FString OsdLine(double Clock);
	// a caption's speaker: "N. VEGA: \"Line\"" is { N. VEGA, "Line" }; Who is empty for a plain line
	void SplitCaption(const FString& Text, FString& Who, FString& Line);
	// a kit note in two hands: its first sentence (Lead) and the rest
	void SplitNote(const FString& Note, FString& Lead, FString& Rest);

	bool SelfTest(TArray<FString>& Lines);
}

// A notebook page (notebook.html .page): the paper, its blue lines Pitch apart, the red margin on
// a left-hand page, and the shade towards the spine.
class SS47GamePage : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47GamePage) : _bLeft(true), _Pitch(34.f), _First(33.f), _Margin(58.f) {}
		SLATE_ARGUMENT(bool, bLeft)
		SLATE_ARGUMENT(float, Pitch)   // mockup pixels between the lines
		SLATE_ARGUMENT(float, First)   // the first line's top
		SLATE_ARGUMENT(float, Margin)  // the red margin's x on a left page (0: none)
	SLATE_END_ARGS()
	void Construct(const FArguments& A) { bLeft = A._bLeft; Pitch = A._Pitch; First = A._First; Margin = A._Margin; }
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8, 8); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
	bool bLeft = true;
	float Pitch = 34.f, First = 33.f, Margin = 58.f;
};

// A loose paper (document.html .paper): the sheet, a little darker towards the foot, with the
// torn edge's ticks along its top where it came out of a pad (bTorn).
class SS47GameSheet : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47GameSheet) : _Color(FLinearColor::White), _bTorn(true) {}
		SLATE_ARGUMENT(FLinearColor, Color)
		SLATE_ARGUMENT(bool, bTorn)
	SLATE_END_ARGS()
	void Construct(const FArguments& A) { Color = A._Color; bTorn = A._bTorn; }
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8, 8); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
	FLinearColor Color;
	bool bTorn = true;
};

// A dashed frame (the kit's chosen drawing: outline 2px dashed, inset 4px).
class SS47GameDash : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47GameDash) : _Color(FLinearColor::Black), _Inset(4.f) {}
		SLATE_ARGUMENT(FLinearColor, Color)
		SLATE_ARGUMENT(float, Inset)
	SLATE_END_ARGS()
	void Construct(const FArguments& A) { Color = A._Color; Inset = A._Inset; }
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8, 8); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
	FLinearColor Color;
	float Inset = 4.f;
};
