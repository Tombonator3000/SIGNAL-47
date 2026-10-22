#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"

class US47UI;
class SWidget;

// ui/Panels.ts: the panels for chapter one (wet bench, contact print, reference file, B-12
// control, two exposures and the local report), built of S47Kit's pieces in the order of the
// web version's HTML. Text inside the game is English (1986). Each panel opens itself with
// UI->Open, as shell() does. (Chapter two's panels come in their own file and are merged here.)
namespace S47Panels
{
	// ---------- wet bench ----------
	// Step: 0 sealed (load), 1 in the tank (wait), 2 transfer to the fixer, 3 collect. A non-empty
	// Empty says why there is nothing to do.
	SIGNAL47_API void WetBench(US47UI* UI, const FString& Frame, int32 Step, const FString& Empty, TFunction<void()> OnAct);

	// ---------- contact print with a loupe ----------
	// Zoom 1x to 4x, drag or sliders to move, click to mark. Marking never edits the photograph.
	struct FPrintView
	{
		FString Title, Url, Intro;
		TOptional<FVector2D> Mark;                        // in the photograph's u, v
		TFunction<FS47Result(double U, double V)> OnMark;  // unset: look only
		TArray<FString> Hints;
		TFunction<void()> OnClose;
		TSharedPtr<SWidget> Extra;
	};
	SIGNAL47_API void PrintView(US47UI* UI, const FPrintView& O);

	// ---------- reference file ----------
	struct FReferenceFile
	{
		TArray<FString> Sheet;
		bool bRefDone = false;
		FString Hyp;                                        // '' (null), 'light', 'drift'
		TFunction<FS47Result(const FString& C)> OnRef;      // 'B-12' | 'R-07'
		TFunction<FS47Result(const FString& C)> OnHyp;      // 'light' | 'drift' | 'command'
	};
	SIGNAL47_API void ReferenceFile(US47UI* UI, const FReferenceFile& O);

	// ---------- B-12 local control ----------
	// Method '' offers the two controls; 'passive' or 'active' shows the review.
	SIGNAL47_API void B12Control(US47UI* UI, const FString& Method, TFunction<void(const FString& M)> OnChoose, const FString& Review = FString());

	// ---------- two exposures ----------
	struct FTwoExposures
	{
		FString Url1, Url2;
		TOptional<FVector2D> Mark2;
		bool bConcluded = false;
		TFunction<FS47Result(double U, double V)> OnMark;
		TFunction<FS47Result(const FString& C)> OnConclude; // 'lamp' | 'commands' | 'second'; unset with a board
		TFunction<void()> OnBoard;
		TArray<FString> Hints;
	};
	SIGNAL47_API void TwoExposures(US47UI* UI, const FTwoExposures& O);

	// ---------- local report ----------
	SIGNAL47_API void LocalReport(US47UI* UI, const FString& Text, bool bFiled, TFunction<void()> OnFile);

	// What a panel that draws itself again gives back ({ refresh }).
	struct FPanelHandle { TFunction<void()> Refresh; };

	// ---------- chapter three: the null test at the STATION 01 field lamp ----------
	// P07 stays in the world (SPILLDESIGN.md): cover the lamp's tin hood, observe, and the result
	// is pinned on the case board by itself. P06, P08 and P09 are made on the board.
	struct FNullTestState { bool bCovered = false, bObserved = false, bP07 = false; };
	struct FNullTest
	{
		TFunction<FString()> Status;
		TFunction<FNullTestState()> State;
		FString Supported;
		TFunction<void(bool bCover)> OnLamp;
		TFunction<FS47Result()> OnObserve;
		TArray<FString> Hints;
	};
	SIGNAL47_API FPanelHandle NullTest(US47UI* UI, const FNullTest& O);

	// Two developed field prints side by side, for a look before they go into the case file. A
	// print can be looked at closer where it is clicked, and back: OnLook says where, in the
	// photograph's u and v (story/theThird.ts).
	struct FFieldPrint { FString Url, Caption; };
	struct FFieldPrints
	{
		TArray<FFieldPrint> Prints;
		FString Text;
		TFunction<void()> OnClose;
		TFunction<void(int32 I, double U, double V)> OnLook;
	};
	SIGNAL47_API void FieldPrints(US47UI* UI, const FFieldPrints& O);
}
