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
	// ---------- chapter four: a conversation in room 6 ----------
	// N. Vega at the table. The chapter keeps the state and says what is on screen: the last
	// exchange, the evidence that can be put on the table, a question, and the choices. The panel
	// only draws it and passes clicks back.
	struct FTalkLine { FString Who, Text; };
	struct FTalkChip { FString Id, Label, Image; bool bHave = false, bOn = false; }; // Image: a data URL, or empty
	struct FTalkTable { FString Prompt; TArray<FTalkChip> Chips; FString Submit; };
	struct FTalkChoice { FString Id, Label; bool bDisabled = false; };
	struct FTalkGiven { FString Id, Label; };
	struct FTalkView
	{
		FString Status;
		TArray<FTalkLine> Lines;
		TOptional<FTalkTable> Table;
		FString Question;                 // '' for none
		TArray<FTalkChoice> Choices;
		TOptional<FS47Result> Reply;
		TArray<FTalkGiven> Given;
		/** What happens after the lines, told under them a moment later, and never voiced or read aloud
		 *  with them, so the recorded lines keep their voice (story/Chapter4.ts, PHOTO 03 face down). */
		TOptional<FTalkLine> After;
	};
	struct FTalk
	{
		FString Title;
		TFunction<FTalkView()> View;
		TFunction<void(const FString&)> OnChip;
		TFunction<void()> OnTable;
		TFunction<void(const FString&)> OnChoice;
		TFunction<void(const FString&)> OnRead;
		TFunction<void(const TArray<FTalkLine>&)> OnLines;
		/** The studio-door conversation, with its own optional voice (Earl's voice is not in this version). */
		bool bEarl = false;
		/** The character voice of the speaker ('bacall' ...; core/npcDialogue.ts CharacterVoice), or ''. */
		FString Character;
		TFunction<void()> OnClose;
	};
	struct FTalkHandle { TFunction<void()> Refresh; };
	SIGNAL47_API FTalkHandle Talk(US47UI* UI, const FTalk& O);
	/** core/portraits.ts portraitOf(who): the art name of whoever speaks under this name, or '' */
	SIGNAL47_API FString PortraitOf(const FString& Who);

	// ---------- the signal processor on the control room rack ----------
	struct FDecoderView
	{
		FString Tape;
		FString Mode = TEXT("harmonized"); // 'raw' | 'harmonized'
		double Pitch = 0, Stretch = 2, Space = 0.6;
		bool bPlaying = false;
		TFunction<int32()> Current;        // which pulse sounds (0..10), -1 between
	};
	struct FDecoderPanel
	{
		TFunction<FDecoderView()> View;
		TFunction<void(const FString& M)> OnMode;              // 'raw' | 'harmonized'
		TFunction<void(const FString& K, double V)> OnParam;   // 'pitch' | 'stretch' | 'space'
		TFunction<void(bool bPlay)> OnPlay;
		TFunction<void()> OnClose;
	};
	SIGNAL47_API FPanelHandle DecoderPanel(US47UI* UI, const FDecoderPanel& O);
	/** The scope's bars (the eleven pulses of one group cycle): x, height, both in the canvas's 640 x 120 (tests). */
	SIGNAL47_API void DecoderScopeBar(int32 I, const FString& Mode, double H, double& OutX, double& OutHh);
}
