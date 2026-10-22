#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Layout/Margin.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

class ISlateStyle;
class STextBlock;
struct FSlateBrush;

// The menus' look (the mockups Tom approved in October 2026): a 1986 VHS tape over a SARO
// operations terminal. VT323 for menu text, Oswald for headings, Special Elite for body text,
// Reenie Beanie for handwriting; the night, amber, phosphor and paper; an amber selection bar with
// a rule on its left; the tape's on-screen display with its red and cyan edges.
//
// Sizes are the mockups' pixels at 1600 x 900. Slate scales the screen by its shortest side
// (1080 is 1), so a mockup pixel is U Slate units and the menus look the same at every size.
// The pieces here are meant for the in-game interface as well.
namespace S47Theme
{
	constexpr float U = 1.2f;
	inline float Px(float MockupPx) { return MockupPx * U; }
	inline FMargin Pad(float L, float T, float R, float B) { return FMargin(L * U, T * U, R * U, B * U); }
	inline FMargin Pad(float H, float V) { return FMargin(H * U, V * U); }

	// ---------- colours (the mockups' CSS, sRGB, as Slate's linear) ----------
	SIGNAL47_API FLinearColor Css(uint32 Rgb, float A = 1.f);
	inline FLinearColor Night(float A = 1.f) { return Css(0x05070d, A); }   // --night
	inline FLinearColor Amber(float A = 1.f) { return Css(0xffb052, A); }   // --amber
	inline FLinearColor AmberDim(float A = 1.f) { return Css(0xa8641f, A); }
	inline FLinearColor Phos(float A = 1.f) { return Css(0x8cffa4, A); }    // --phos
	inline FLinearColor Paper(float A = 1.f) { return Css(0xebe3cf, A); }   // --paper
	inline FLinearColor PaperDim(float A = 1.f) { return Css(0xa39e92, A); }
	inline FLinearColor Neon(float A = 1.f) { return Css(0xff5a45, A); }    // --neon
	inline FLinearColor Ink(float A = 1.f) { return Css(0x1e2430, A); }     // --ink
	inline FLinearColor Head(float A = 1.f) { return Css(0xf1ecdf, A); }    // headings
	inline FLinearColor Body(float A = 1.f) { return Css(0xcfc8b7, A); }    // descriptions
	inline FLinearColor Mute(float A = 1.f) { return Css(0x8f8a7e, A); }    // key hints, quiet lines
	inline FLinearColor Rule(float A = 1.f) { return Css(0x5d584e, A); }    // keycaps' frame
	inline FLinearColor Sage(float A = 1.f) { return Css(0x6f8f78, A); }    // the terminal's ids
	inline FLinearColor Arrow(float A = 1.f) { return Css(0x6c665b, A); }   // a choice's arrows, at rest
	inline FLinearColor Osd(float A = 1.f) { return Css(0xf4f1e8, A); }     // the tape's display

	// ---------- fonts (size and letter spacing in mockup pixels) ----------
	SIGNAL47_API FSlateFontInfo Term(float Px, float Spacing = 0.f);  // VT323
	SIGNAL47_API FSlateFontInfo Heading(float Px, float Spacing = 0.f); // Oswald Medium
	SIGNAL47_API FSlateFontInfo Type(float Px, float Spacing = 0.f);  // Special Elite
	SIGNAL47_API FSlateFontInfo Hand(float Px, float Spacing = 0.f);  // Reenie Beanie

	// ---------- brushes and painting ----------
	SIGNAL47_API const FSlateBrush* White();
	// a box: fill, a line round it (width in Slate units), rounded corners; kept for the session
	SIGNAL47_API const FSlateBrush* Box(const FLinearColor& Fill, const FLinearColor& Line = FLinearColor::Transparent, float LineW = 0.f, float Radius = 0.f);
	SIGNAL47_API const ISlateStyle& Styles(); // rich text: "Note", "Em" and the hyperlink "Link" (the title's note)
	// a gradient over a rectangle in local units: stops at 0..1 across (bAcross) or down
	SIGNAL47_API void Gradient(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& Pos, const FVector2f& Size,
		const TArray<TPair<float, FLinearColor>>& Stops, bool bAcross);
	SIGNAL47_API void Fill(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& Pos, const FVector2f& Size, const FLinearColor& Color);
	SIGNAL47_API void Triangle(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& A, const FVector2f& B, const FVector2f& C, const FLinearColor& Color);
	// .vig: radial-gradient(ellipse at CenterX 50%, transparent 45%, rgba(0,0,0,Edge) 100%)
	SIGNAL47_API void Vignette(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, float CenterX = 0.6f, float Edge = 0.65f);
	// a soft shadow under a box (box-shadow without Slate's blur: a few widening layers)
	SIGNAL47_API void Shadow(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& Pos, const FVector2f& Size, float OffsetY, float Spread, float Alpha);

	// ---------- the night's words ----------
	// "Chapter 2: The Amended Record" as the menus write it: { "CHAPTER TWO", "THE AMENDED RECORD" }
	SIGNAL47_API TPair<FString, FString> ChapterLine(const FString& ChapterTitle);
	// the tape's date stamp for the night's clock (seconds after midnight): 13 to 14 April 1986
	SIGNAL47_API FString DateStamp(double Clock);       // "APR.13 1986" before midnight, "APR.14 1986" after
	SIGNAL47_API FString Stamp(double Clock);           // "APR.13 1986  23:41:07"
	SIGNAL47_API FString SavedWhen(double Ms);          // "today 11:02", "yesterday 23:18", "Oct 20, 14:05"
	SIGNAL47_API FString SavedAgo(double Ms);           // "SAVED 2 MIN AGO"
	SIGNAL47_API FString Played(double Sec);            // "1 H 12 MIN"
	SIGNAL47_API bool SelfTest(TArray<FString>& Out);
}

// The theme's names for a function body outside the namespace (the SS47 widgets' members), as
// block-scope using-declarations: they are found before any namespace's, so a unity build that
// has another `using namespace` in force (S47Css has a Term, a Type, a Box ...) stays unambiguous.
// (No file in this module says `using namespace S47Theme;` for the same reason.)
#define S47_USING_THEME \
	using S47Theme::U; using S47Theme::Px; using S47Theme::Pad; using S47Theme::Css; using S47Theme::Night; \
	using S47Theme::Amber; using S47Theme::AmberDim; using S47Theme::Phos; using S47Theme::Paper; \
	using S47Theme::PaperDim; using S47Theme::Neon; using S47Theme::Ink; using S47Theme::Head; \
	using S47Theme::Body; using S47Theme::Mute; using S47Theme::Rule; using S47Theme::Sage; \
	using S47Theme::Arrow; using S47Theme::Osd; using S47Theme::Term; using S47Theme::Heading; \
	using S47Theme::Type; using S47Theme::Hand; using S47Theme::White; using S47Theme::Box; \
	using S47Theme::Styles; using S47Theme::Gradient; using S47Theme::Fill; using S47Theme::Triangle; \
	using S47Theme::Vignette; using S47Theme::Shadow; using S47Theme::ChapterLine; using S47Theme::DateStamp; \
	using S47Theme::Stamp; using S47Theme::SavedWhen; using S47Theme::SavedAgo; using S47Theme::Played; \
	using S47Theme::Keycap; using S47Theme::KeyHints; using S47Theme::Choice; using S47Theme::Segments; \
	using S47Theme::Toggle; using S47Theme::Tabs; using S47Theme::Button; using S47Theme::Backdrop; \
	using S47Theme::Scanlines; using S47Theme::Frame;

// The shapes the fonts do not have (VT323 has no arrows): the tape's play and pause, a choice's
// triangles and a keycap's arrows.
class SIGNAL47_API SS47Glyph : public SLeafWidget
{
public:
	enum class EKind : uint8 { Play, Pause, TriLeft, TriRight, Up, Down, Left, Right };
	SLATE_BEGIN_ARGS(SS47Glyph) : _Kind(EKind::Play), _Size(FVector2D(22, 24)), _Color(FLinearColor::White) {}
		SLATE_ARGUMENT(EKind, Kind)
		SLATE_ARGUMENT(FVector2D, Size)   // mockup pixels
		SLATE_ATTRIBUTE(FLinearColor, Color)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual FVector2D ComputeDesiredSize(float) const override { return Size * S47Theme::U; }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;

private:
	EKind Kind = EKind::Play;
	FVector2D Size;
	TAttribute<FLinearColor> Color;
};

// The tape's on-screen display (.osd): VT323 in near white with its red and cyan edges, and the
// play or pause sign after the words when there is one.
class SIGNAL47_API SS47Osd : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Osd) : _Px(34.f), _bPlay(false), _bPause(false) {}
		SLATE_ATTRIBUTE(FString, Text)
		SLATE_ARGUMENT(float, Px)
		SLATE_ARGUMENT(bool, bPlay)
		SLATE_ARGUMENT(bool, bPause)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
};

// A keycap (.keys b): the key's name in a thin frame. Tokens are split on spaces; UP, DOWN, LEFT
// and RIGHT are drawn as arrows ("UP DOWN" is the menus' select key).
namespace S47Theme
{
	SIGNAL47_API TSharedRef<SWidget> Keycap(const FString& Tokens, float Px = 22.f, TAttribute<FLinearColor> Text = FLinearColor::Transparent, TAttribute<FLinearColor> Frame = FLinearColor::Transparent);
	// .keys: keycaps with what they do; an entry with OnClick is a button as well
	struct FHint { FString Keys, Label; TFunction<void()> OnClick; };
	SIGNAL47_API TSharedRef<SWidget> KeyHints(const TArray<FHint>& Hints, float Px = 22.f);
}

// A line of a menu (.it): the words, an optional line under them (.meta), the amber bar with its
// rule on the left when it is the one selected, dim when it cannot be chosen. The title's has the
// amber triangle outside, on the left. The menu that holds it has the keys; the mouse selects by
// hovering and chooses by clicking.
class SIGNAL47_API SS47MenuItem : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47MenuItem) : _Px(42.f), _MetaPx(22.f), _Spacing(2.f), _Pad(FMargin(18, 4)), _bArrow(false), _Selected(false), _Usable(true) {}
		SLATE_ARGUMENT(FString, Label)
		SLATE_ATTRIBUTE(FString, Meta)
		SLATE_ARGUMENT(float, Px)
		SLATE_ARGUMENT(float, MetaPx)
		SLATE_ARGUMENT(float, Spacing)
		SLATE_ARGUMENT(FMargin, Pad)              // mockup pixels, inside the 3 px rule
		SLATE_ARGUMENT(bool, bArrow)
		SLATE_ARGUMENT(TOptional<FLinearColor>, Color) // at rest (unset: paper at .82)
		SLATE_ATTRIBUTE(bool, Selected)
		SLATE_ATTRIBUTE(bool, Usable)
		SLATE_ARGUMENT(TFunction<void()>, OnHover)
		SLATE_ARGUMENT(TFunction<void()>, OnClick)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
	virtual void OnMouseEnter(const FGeometry& G, const FPointerEvent& E) override;
	virtual FReply OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override;
	virtual FReply OnMouseButtonUp(const FGeometry& G, const FPointerEvent& E) override;

private:
	TAttribute<bool> Selected, Usable;
	TFunction<void()> OnHover, OnClick;
	bool bArrow = false, bPressed = false;
	TSharedPtr<STextBlock> LabelText;
};

// A row of the settings (.row): the name on the left, its control on the right, 58 px high; the
// amber bar when selected.
class SIGNAL47_API SS47Row : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Row) : _Height(58.f), _Selected(false), _Usable(true) {}
		SLATE_ARGUMENT(FString, Label)
		SLATE_ARGUMENT(float, Height)
		SLATE_ATTRIBUTE(bool, Selected)
		SLATE_ATTRIBUTE(bool, Usable)
		SLATE_ARGUMENT(TFunction<void()>, OnHover)
		SLATE_ARGUMENT(TFunction<void()>, OnClick)
		SLATE_DEFAULT_SLOT(FArguments, Value)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
	virtual void OnMouseEnter(const FGeometry& G, const FPointerEvent& E) override;
	virtual FReply OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override;

private:
	TAttribute<bool> Selected, Usable;
	TFunction<void()> OnHover, OnClick;
};

// A setting's controls: a choice (◀ VALUE ▶), the 20 segments with the value after them, ON / OFF.
namespace S47Theme
{
	SIGNAL47_API TSharedRef<SWidget> Choice(TAttribute<FString> Value, TAttribute<bool> Selected, TFunction<void(int32)> OnStep, TAttribute<bool> Usable = true);
	SIGNAL47_API TSharedRef<SWidget> Segments(TAttribute<float> Frac, TAttribute<FString> Value, TFunction<void(float)> OnSet, TAttribute<bool> Usable = true);
	SIGNAL47_API TSharedRef<SWidget> Toggle(TAttribute<bool> On, TFunction<void(bool)> OnSet, TAttribute<bool> Usable = true);
	// .tabs: Q, the tabs, E; the one shown in amber with its bar under it
	SIGNAL47_API TSharedRef<SWidget> Tabs(const TArray<FString>& Names, TAttribute<int32> Current, TFunction<void(int32)> OnPick);
	// .btn: APPLY (amber) or a quiet one (REVERT); Lit: chosen by the keys
	SIGNAL47_API TSharedRef<SWidget> Button(const FString& Label, bool bAmber, TFunction<void()> OnClick, TAttribute<bool> Usable = true, TAttribute<bool> Lit = false);
	// the screen behind a menu: the game blurred (Blur in mockup pixels, 0 for none), darkened by Tint
	SIGNAL47_API TSharedRef<SWidget> Backdrop(float Blur, const FLinearColor& Tint, bool bVignette = true, float CenterX = 0.6f);
	// .scan: the tape's lines over everything (HitTestInvisible)
	SIGNAL47_API TSharedRef<SWidget> Scanlines(float Alpha = 0.18f);
	// .frame: the terminal's panel, with its shadow; the content inside
	SIGNAL47_API TSharedRef<SWidget> Frame(TSharedRef<SWidget> Content);
	// A menu that is the UI's modal keeps the keys: closing the one before it gives the focus back to
	// the game's viewport (FInputModeGameOnly) after the new one has taken it. Called from its Tick.
	SIGNAL47_API void KeepFocus(const TSharedRef<SWidget>& Menu);
	// a scroll box's bar: a thin amber thumb, no track
	SIGNAL47_API const struct FScrollBarStyle* ScrollStyle();
}
