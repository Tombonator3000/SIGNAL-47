#pragma once

#include "CoreMinimal.h"
#include "S47Decoder.h"
#include "S47Docs.h"
#include "S47Obj.h"
#include "S47Panels.h"
#include "UObject/StrongObjectPtr.h"

class US47UI;
class US47Audio;
class US47Inter;
class AS47Web;
class UTextureRenderTarget2D;
class UTexture;
class FWidgetRenderer;
class SS47DeskDsp;
class FS47ConsoleKit;

// web/src/story/Decoder.ts. The signal processor next to RX bank 3: it holds the tape of the anomaly
// once the receiver has locked on it (from 02:14 in the prologue, and all night after that) and plays
// it back raw or harmonized (core/decoder.ts, FS47DecoderPlayer). Its display and reels follow the
// playback. It belongs to the control room, not to a chapter: AS47Story (Prologue.ts) holds it.
//
// (room.dspScreen is a 256 x 72 canvas in the web version; here the processor's own picture is drawn
// into a render target of that size set on the screen's material, as reel 047's is, S47SaroNight.cpp.
// Reel 047 shows its own target on the same material while it has the display, and gives it back.)
//
// (Unreal only, on Tom's request of 22 October 2026; both depart from the web version:)
//
//   the bought unit  the stacked unit beside the right-hand terminal on the console desk is the signal
//                    processor (S47ConsoleKit.h): 'decoder' is used there, its small green display
//                    shows the canvas, its VU meters' needles follow what plays, and the homemade deck
//                    on the rack is taken away. Reel 047 plays on it too; its reels no longer turn
//                    (the spare reel beside the unit is part of the desk's one clutter mesh). The
//                    panel stays over the game: the unit's display is a strip of 18 x 4 cm (4.5 to
//                    1), too small and too narrow for it to be drawn there as the receiver console
//                    is on its CRT (SS47ScreenView).
//   the answer       the panel has an ANSWER row: seven keys, the seven notes of the 7-group as the
//                    harmonized playback sounds them (1 the first and highest, 7 the last and lowest;
//                    a key lights its bar on the scope and the display). The last seven keys pressed
//                    are checked against T. Vega's (7 5 3 2 1 4 6: the four rising, the octave, then
//                    down), which is on a page of his found under the spare reel once the signal is
//                    locked. Keyed right, the processor gives it back by itself (the four, then the
//                    seven, slower, the reverb opening), the room answers (the array's red lamps blink
//                    as one, at the last note the screens glitch and the lights dip), and it runs on
//                    past the end of tape B into TAPE A, a wire from 1947: someone whistling the four
//                    and the seven down the field line, the seven coming back as the processor has
//                    just played them (the lights stutter with them), the field telephone's crank, T.
//                    Vega's correction (the line's recording), cutters, and nothing (the night outside
//                    hushed). Then a paper filed with a note, kept with the case (NightShiftState.tapeA).
//                    Once a case. No objective and no hint names any of it.

struct FS47DecoderDeskDeps
{
	TWeakObjectPtr<US47UI> UI;
	TWeakObjectPtr<US47Audio> Audio;
	TWeakObjectPtr<AS47Web> Room;
	TWeakObjectPtr<US47Inter> Inter;
	TFunction<bool()> Locked;
	TFunction<FString()> Recorded;
	TFunction<bool()> Busy;
	TFunction<bool()> Power;
	// (Unreal only) reel 047's level off the tape (the VU meters while it plays); the case's papers (one
	// filed, filing one with its note), whether TAPE A has come in this case and marking it, a save; the
	// room as the answer comes back (the screens' glitch, the lights' dip and their flicker, in seconds;
	// the array's red lamps blinking as one, or each in its own phase again; the night outside quiet, as
	// Ambience.hush); the clock for the notes ('02:31')
	TFunction<double()> ReelLevel;
	TFunction<bool(const FString&)> HasPaper;
	TFunction<void(const FS47Doc&, const FString&)> File;
	TFunction<bool()> Answered;
	TFunction<void()> SetAnswered;
	TFunction<void()> Save;
	TFunction<void(double, double, double)> React;
	TFunction<void(bool)> Lamps;
	TFunction<void(double, double, double, double)> Hush;
	TFunction<FString()> Clock;
};

class SIGNAL47_API FS47DecoderDesk
{
public:
	explicit FS47DecoderDesk(FS47DecoderDeskDeps InD);
	~FS47DecoderDesk();

	bool Playing() const;
	/** Reel 047 on the deck (story/reel047.ts, story/archivePhone.ts; set by main.ts, S47Phones.cpp): its
	 *  label and its use while it is in the kit or playing (empty label: null; Use false: not its), and
	 *  whether it has the display and the reels now. */
	struct FReel { TFunction<FString()> Label; TFunction<bool()> Use; TFunction<bool()> Owns; };
	FReel Reel;

	void Stop();
	/** Per frame (Prologue.update: after the chapters). */
	void Update(double Dt);
	/** (Unreal) The playback steps back under a phone or a voice (AS47Story::VoicesDuck). */
	void Duck(double K) { if (Player.IsValid()) Player->Duck(K); }

	/** The interactable 'decoder' (inter.add in the constructor): its label and its use. */
	FString Label() const;
	void Use();

	/** The panel's view, and the panel itself (private open() in Decoder.ts; public here for the
	 *  developer's -S47Decoder run). Open returns the toast it said instead ('' once the panel is up). */
	S47Panels::FDecoderView View() const;
	FString Open();
	/** The panel's onMode, onParam and onPlay. */
	void OnMode(const FString& M);
	void OnParam(const FString& K, double V);
	void OnPlay(bool bPlay);
	/** The parameters as the panel sets them (tests). */
	const FS47DecoderParams& Params() const { return P; }
	/** The display's picture now (tests): 'NO TAPE', or the mode line. */
	FString ScreenLine() const;

	// ---------- (Unreal only) the bought unit and the answer ----------
	/** The bought unit (S47ConsoleKit.h), or null when the export has none (the deck on the rack stays). */
	FS47ConsoleKit* Kit() const { return KitPtr.Get(); }
	/** Where the processor's sound comes from, in three's world terms. */
	FVector SoundAt() const;
	/** The ANSWER row's key K: 1 to 7, the seven of the group as they come (1 the top, 7 the bottom). */
	void OnKey(int32 K);
	/** The last keys pressed (up to seven), and whether the processor is giving the answer back now. */
	const TArray<int32>& Keys() const { return KeyLog; }
	bool Answering() const { return AnswerT >= 0; }
	/** T. Vega's seven (what the line sang back to him), and whether the last seven keys are it. */
	static const TArray<int32>& ANSWER();
	static bool IsAnswer(const TArray<int32>& Last);
	/** His page under the reel ('vegaPage') and TAPE A ('tapeA'); by id, as the case's papers are put
	 *  back (Prologue.paperById). */
	static FS47Doc PageDoc();
	static FS47Doc TapeDoc();
	static TOptional<FS47Doc> PaperById(const FString& Id);
	/** The page is there to find: the bought unit, the signal locked, the page not taken. */
	bool PageThere() const;

	static bool SelfTest(TArray<FString>& Lines);

private:
	TSharedRef<bool> Alive = MakeShared<bool>(true);
	FS47DecoderDeskDeps D;
	TSharedPtr<FS47DecoderPlayer> Player;
	FS47DecoderParams P;
	double ScrT = 0;
	double Spin = 0;
	TArray<FS47Obj> Reels;
	TUniquePtr<FS47ConsoleKit> KitPtr;

	// room.dspScreen: the render target, its material and the widget drawn into it
	TStrongObjectPtr<UTextureRenderTarget2D> Target;
	int32 Mat = -1;
	FWidgetRenderer* Renderer = nullptr;
	TSharedPtr<SS47DeskDsp> Widget;

	// the answer: the keys, the timeline as it plays, and the developer's -S47Answer
	TArray<int32> KeyLog;
	double AnswerT = -1;
	int32 AnswerStep = 0;
	bool bAnswerFirst = false;
	FS47AnswerTimes Ans;
	bool bPanelOpen = false;
	bool bTestAnswer = false;
	FString TestMode;
	double TestT = 0;
	int32 TestK = 0;
	// -S47KitShots=<prefix> -S47KitShotAt=<s>: two screenshots a second apart (the needles moving)
	FString ShotPrefix;
	double ShotAt = -1, LifeT = 0, LogT = 0;
	int32 Shots = 0;

	void Play();
	void DrawScreen();
	void InitScreen();
	bool EnsurePlayer();
	UTexture* CanvasMap() const;
	void ReadPage();
	void BeginAnswer();
	void StepAnswer(double Dt);
	void EndAnswer(bool bUnlock);
	void AbortAnswer();
	void TestAnswer(double Dt);
	FString ClockNow() const { return D.Clock ? D.Clock() : FString(); }
};
