#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"
#include "UObject/StrongObjectPtr.h"

class AS47Web;
class FS47Group;
class UTexture;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
class FWidgetRenderer;
class SS47VuFace;
class SS47TornPage;

// (Unreal only, on Tom's request of 22 October 2026: it departs from the web version.) The bought unit
// beside the right-hand terminal on the console desk becomes the signal processor: the signal analyzer
// with its small green display, the VU unit with its two lit meters on top of it, the headphones in
// front and the desk microphone (Vintage Liminal Props, licensed: web/src/assets/licensed/models/
// console_kit.glb, placed by world/ControlRoom.ts placeModels as one merged mesh, 'lic:console_kit').
// In the web version it is dressing and the processor is the homemade deck on the rack beside RX bank
// 3 (room.objs.decoder, its reels room.reels, its display room.dspScreen); here that deck is taken
// away, so there is one tape machine in the room, and the processor's display and meters are on the
// bought unit:
//
//   the display  the analyzer's display (console_kit.glb extras.display: 0.180 x 0.040 m) shows what
//                room.dspScreen's canvas shows (the processor's own picture, or reel 047's while it
//                has the deck): a quad over it with the canvas's map, the canvas's 256 x 72 at its
//                own shape, on a dark glass the width of the display
//   the meters   the needles are painted into the meter faces of the export (one mesh), so each face
//                is drawn again with a needle that moves, into a render target on a quad over its
//                window (extras.meters), backlit as the faces are (unlit); the needles follow the
//                level of what the processor gives out (the tape, the keys, reel 047), with a VU
//                meter's ballistics
//   the page     T. Vega's page under the spare reel beside the unit (S47DecoderDesk: the answer):
//                a folded field-book page in pencil, built here, taken in as a node so it can be used
//
// Without the bought unit in the export the homemade deck stays, as in the web version.

/** A VU meter's needle (IEC 60268-17): a second-order movement that reaches 99 % of a step in 0.3 s
 *  and goes 1.5 % past it. X is its place along the scale, 0 at the left stop, 1 at +3 VU. */
struct SIGNAL47_API FS47VuNeedle
{
	double X = 0, V = 0;
	double Omega = 13.1, Zeta = 0.8;
	void Step(double Target, double Dt);
	/** Where a mark in VU sits along the scale: the scale is linear in voltage, 0 VU at 1 / 1.413. */
	static double Mark(double Vu);
	/** A level (RMS) as the needle's target: Ref reads 0 VU. */
	static double Target(double Rms, double Ref);
};

class SIGNAL47_API FS47ConsoleKit
{
public:
	explicit FS47ConsoleKit(AS47Web* InWeb);
	~FS47ConsoleKit();

	/** The bought unit in the export ('lic:console_kit' in the control room), or -1. */
	static int32 Find(AS47Web* W);
	bool IsValid() const { return Node >= 0; }
	FS47Obj Obj() const;
	/** Where the processor's sound comes from, in three's world terms: the front of the stack. */
	FVector SoundAt() const;
	/** The middle of the stack (what the camera's album names). */
	FVector At() const;

	/** Each frame: the needles to the level (RMS) the processor gives out, the display to the canvas's
	 *  picture (room.dspScreen's map now), and the parts shown with the room. */
	void Update(double Dt, double Level, UTexture* Display);
	/** The needles now (tests, the log). */
	double Needle(int32 I) const { return I == 0 ? Left.X : Right.X; }

	/** T. Vega's page under the spare reel: its node (the things to use name it), shown or not. */
	FS47Obj Page() const;
	void ShowPage(bool bOn);

	/** The level that reads 0 VU (RMS at the processor's output). */
	static constexpr double REF = 0.075;

	static bool SelfTest(TArray<FString>& Lines);

private:
	TWeakObjectPtr<AS47Web> Web;
	int32 Node = -1, PageNode = -1;
	TUniquePtr<FS47Group> G, PageG;
	bool bShown = true;
	// the display: the dark glass and the picture over it, showing the canvas's map
	TStrongObjectPtr<UMaterialInstanceDynamic> PicMid;
	TWeakObjectPtr<UTexture> Shown;
	// the meters: a face each, drawn with its needle
	FS47VuNeedle Left, Right;
	TStrongObjectPtr<UTextureRenderTarget2D> Face[2];
	TStrongObjectPtr<UTextureRenderTarget2D> PageTex;
	FWidgetRenderer* Renderer = nullptr;
	TSharedPtr<SS47VuFace> Vu;
	TSharedPtr<SS47TornPage> PageW;
	double Drawn[2] = { -1, -1 };
	double DrawT = 0;
	bool bPageDrawn = false;

	void TakeAwayTheDeck();
	void Build();
	void DrawFaces(bool bAll);
	void DrawPage();
};
