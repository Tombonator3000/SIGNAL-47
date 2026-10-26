// SIGNAL / 47: the operations terminal on the west desk (web/src/story/nightshift.ts, OpsTerminal):
// the survey files, the solver exceptions, the tape catalogue and the console log. Text only; the
// screen that shows it belongs to the renderer.

#pragma once

#include "CoreMinimal.h"

/** One console log line: clock (seconds after midnight) and text. */
struct FS47LogLine
{
	double Clock = 0;
	FString Text;
};

/** What the terminal sees of the night when a command runs. */
struct FS47OpsView
{
	double Clock = 0;             // now
	TOptional<double> SolveAt;    // when tonight's range came out negative, if it has
	TArray<FS47LogLine> Log;      // tonight's lines after the seeds, in order
	bool bEveningTimeline = false;
	bool bFuture = false;         // RUN860414_0529.DAT is in the directory (All Night)
};

/** Commands from outside Night Shift (the array's ARRAY RESUME): its answer to one of its own
 *  (unset: not one of them), the buttons it offers now, and its lines in HELP. */
struct FS47OpsMore
{
	TFunction<TOptional<FString>(const FString&)> Run;
	TFunction<TArray<FString>()> Buttons;
	TArray<FString> Help;
};

class FS47OpsTerminal
{
public:
	explicit FS47OpsTerminal(TFunction<FS47OpsView()> InView);

	/** The array's commands (set from outside). */
	TOptional<FS47OpsMore> More;

	/** The two lines above the prompt. */
	FString Header() const;
	/** The commands offered as buttons; a few appear once the screen has shown why. */
	TArray<FString> Buttons() const;
	/** Runs one command line and returns the screen's answer. */
	FString Run(const FString& Input);

	/** Checks the screens against values taken from the TypeScript. Appends one line per check
	 *  ("ok  name" or "FAIL name: got ..., want ...") and returns false on any mismatch. */
	static bool SelfTest(TArray<FString>& Out);

private:
	struct FFile
	{
		FString Name;
		int32 Blocks = 0;
		FString Made;
	};

	TFunction<FS47OpsView()> View;
	TArray<FString> Extra;   // buttons that appeared, in order (a Set in the original)

	TArray<FFile> Files(const FS47OpsView& V) const;
	FString Dir(const FS47OpsView& V);
	FString Type(const FS47OpsView& V, const FString& Name) const;
	FString Exceptions(const FS47OpsView& V) const;
	FString Tapes(int32 A, int32 B) const;
	FString Log(const FS47OpsView& V) const;

	static FString ReelDate(int32 N);
	static int32 RunBlocks(double Clock);
	static int32 Day(double Clock);
	static FString Stamp(double Clock, bool bSecs = false);
};
