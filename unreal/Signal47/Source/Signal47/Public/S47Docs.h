// SIGNAL / 47: the papers of Night Shift and the Prologue, ported from web/src/story/time.ts,
// web/src/story/nightshift.ts and web/src/story/Prologue.ts. Text only: the pictures (sheets,
// paper, poster image) stay with the renderer. Lines that start with "~ " are pencil on the paper.

#pragma once

#include "CoreMinimal.h"

/** One page of a paper that has several (DocSpec.sheets). */
struct FS47DocSheet
{
	FString Page, Paper, Strip, Term;
};

/** One readable document: what UI.document shows in the web version (ui/UI.ts DocSpec). */
struct FS47Doc
{
	FString Id;
	FString Title;
	FString Kind;        // 'hand', 'typed', 'printout', 'photo' or 'map'
	FString Stamp;       // the stamp in the corner; empty for none
	FString Page;        // the paper's text, exactly
	FString Transcript;  // what the player reads aloud
	FString Image;       // a picture on it: a data URL (a photo, a drawing) or an art name (core/art.ts)
	FString Paper;       // an art name for the paper itself (a hand page on a card, a letter)
	double PaperRatio = 0; // its width over height when it has one
	TArray<FS47DocSheet> Sheets;
};

namespace S47Text
{
	/** String(Math.floor(n)).padStart(w, '0')  (time.ts pad) */
	FString Pad(double N, int32 W = 2);

	/** An antenna's angle as the readouts print it: whole degrees padded to W figures and the tenth
	 *  (truncated, as the encoders do), with a minus sign below zero (a dish below the horizon). */
	FString AngleText(double V, int32 W = 2);

	/** Clock text from seconds after midnight; wraps at 24 h. */
	FString ClockText(double Sec, bool bWithSeconds = true);

	/** JavaScript String.prototype.padStart / padEnd with a single fill character. */
	FString PadStart(const FString& S, int32 W, TCHAR Fill = TEXT(' '));
	FString PadEnd(const FString& S, int32 W, TCHAR Fill = TEXT(' '));

	/** JavaScript String.prototype.split(sep): keeps empty pieces. */
	TArray<FString> Split(const FString& S, const FString& Sep);
}

namespace S47Docs
{
	// Prologue
	FS47Doc ShiftLog(bool bEvening = false);                 // Dale's shift log
	FS47Doc WorkOrder();                // the night work order
	FS47Doc Printout(double Clock);     // the direction solve printout, stamped with the clock

	// Night Shift
	FS47Doc Binder();                   // solver exceptions binder
	FS47Doc ServiceRecord();            // RX bank 3 service record
	FS47Doc InterferenceCard(bool bLocked);
	FS47Doc HalleyPoster();

	// The telex roll
	TArray<FString> TelexEvening(bool bEveningTimeline = false);                       // last evening's two messages
	FS47Doc Telex(const TArray<FString>& Messages, bool bEveningTimeline = false);
	FString TelexHondo(double Filed);                     // Hondo Valley's message, filed at that clock
	FString HondoNote(double Filed);                      // the note the first reading of it leaves
	TArray<FString> TelexLines(const TArray<FString>& Messages);

	/** Checks a few exact strings against values taken from the TypeScript. Appends one line per
	 *  check ("ok  name" or "FAIL name: got ..., want ...") and returns false on any mismatch. */
	bool SelfTest(TArray<FString>& Out);
}
