#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "S47Docs.h"
#include "S47Ops.h"
#include "S47Rx.h"
#include "S47Timeline.h"
#include "S47Story.generated.h"

class AS47Web;
class AS47HUD;
class US47Audio;

// The prologue, "Night Shift", ported from web/src/story/Prologue.ts: the same beats in the
// same order, on game time (stops while a paper or the console is open). The chapters after
// it are not in this version yet; the night ends on a card that says so.
UENUM()
enum class ES47Phase : uint8 { Intro, Shift, Survey, Skip, Residual, Locked, Solving, Printing, Printed, Ringing, Call, Countdown, Event, Turning, End, Ch1 };

struct FS47Task { FString Text; bool bDone = false; };

// Tonight's lines in the console log, which papers were read, relay K3 (NightShiftState); the
// array put back on the schedule (array: story/arrayBack.ts ArraySave, kept as its JSON) and when
// Hondo Valley's telex was filed (hondo: story/saroDark.ts).
struct FS47NightState
{
	TArray<FString> Papers; bool bK3Open = false; TArray<FS47LogLine> Log; TOptional<double> SolveAt; bool bJacket = false;
	TSharedPtr<class FJsonObject> Array;
	TOptional<double> Hondo;
	// Acquisition stamp of the actual paper, distinct from solve time and the later array event.
	TOptional<double> PrintoutAt;
	// When the residual was locked on the console (SC05 at 03:1x in the evening timeline); unset in older saves.
	TOptional<double> LockAt;
	/** (Unreal only, Tom's request) T. Vega's seven keyed on the processor: TAPE A has come (tapeA). */
	bool bTapeA = false;
};
// The array put back on the schedule, as the status screen shows it while a try runs
// (story/arrayBack.ts status()): its mode, its bottom line (empty: none), and whether they are amber.
struct FS47ArrayStatus { FString Mode, Line; bool bWarn = false; };

UCLASS()
class SIGNAL47_API AS47Story : public AActor
{
	GENERATED_BODY()

public:
	AS47Story();
	virtual void Tick(float DeltaSeconds) override;

	void Bind(AS47Web* InSaro, AS47HUD* InHud, US47Audio* InAudio);
	void Start();
 // Set before reset/load. Absent timelineVersion in a saved case means legacy.
 bool bEveningTimeline = false;
 bool bOpeningHeld = false;
 void BeginEveningShift();
 bool NightSequence() const;
 bool FeederOutage() const;
 bool FeederFaultVisible() const;
 void ApplyFeederPower(bool bOut);
 // Completed work can advance a new night, never rewind a later clock or miss a reserved beat.
 void AdvanceClockTo(double Target, const FString& Reason);
 // Stable early-shift checkpoint: before the timed survey starts.
 TSharedPtr<class FJsonObject> SaveRoutine() const;
 void RestoreRoutine(const TSharedPtr<class FJsonObject>& Saved);
 static FS47Doc RestorePrintout(const TSharedPtr<class FJsonObject>& SavedNs, double FallbackClock, bool bSavedCase);
 static bool TimelineSelfTest(TArray<FString>& Out);
 double TimelineTime(double Legacy) const;
 FString TimelineText(const FString& Text) const { return S47Timeline::ResidualText(Text, bEveningTimeline); }
	virtual void Reset() override { ResetAll(); } // Prologue.reset: back to the start of the night, every chapter with it
	// Jump straight to a beat, as the web version's Continue and tests do: shift, residual,
	// locked, printed, countdown, event.
	void Jump(const FString& Target);

	// The player's doings, from the HUD.
	FString Label(const FString& Id) const; // the prompt for a thing, empty when it is inert
	void Use(const FString& Id);
	TArray<FS47Task> Tasks() const;
	const TArray<FString>& Notes() const { return NotesList; }
	const TArray<FS47Doc>& Docs() const { return DocsList; }
	FString ChapterTitle() const;
	double Clock() const { return ClockSec; } // seconds after midnight
	FString ClockText(bool bSeconds = true) const { return S47Text::ClockText(ClockSec, bSeconds); }
	ES47Phase Phase() const { return CurrentPhase; }
	// The residual's lock time for papers and notes: the console's own when there is one, else the authored 02:14.
	FString LockStamp() const;
	bool At(ES47Phase P) const { return (int32)CurrentPhase >= (int32)P; }
	bool IsCinematic() const { return bCinematic; }
	bool IsPaused() const { return bPaused; }
	void SetPaused(bool bOn) { bPaused = bOn; }

	// The receiver and the console (the HUD's console edits Rx directly and calls these).
	FS47Rx Rx;
	bool bResidualVisible = false;
	bool SurveyOnly() const { return Rx.Stage == 2 && !bResidualVisible; }
	void ConsoleAction();   // the console's action button: pass the stage or refuse
	void ConsoleAzimuth();  // the azimuth slider moved: the dishes follow
	void ConsoleClosed();
	bool bConsoleOpen = false;

	// The operations terminal.
	FS47OpsTerminal& Ops() { return *OpsTerminal; }
	TArray<FString> OpsLines; // what its screen in the room shows
	void OpsRan(const FString& Cmd, const FString& Result);

	// For the CRT painters.
	const FS47NightState& Night() const { return NS; }
	FS47NightState& NightMutable() { return NS; } // (story/arrayBack.ts keeps its tries in game.ns.array)
	bool bPowered = false;
	double GameTime = 0;
	double BootT = -1, SolveT = -1, PrintT = -1, LineDeadT = -1;
	bool bRinging = false, bAnswered = false;
	/** How near the receiver console the player is (1 at it, less across the room, faint outside), and
	 *  how far the night's tones step back under a phone or a voice (Unreal: on Tom's request). */
	double ConsoleNear() const;
	double VoicesDuck() const;
	float CrtGlitch = 0.f;
	FString SurveyModeText() const;

	// ---------- the chapters' host (Prologue.ts as ChapterHost) ----------
	// game time (gt) runs while nothing is open and nothing holds it; the clock is the night's
	void After(double Seconds, TFunction<void()> Fn, const FString& Tag = FString()); // on game time
	void Cancel(const FString& Tag);
	void ClearTimers() { Timers.Empty(); } // beginChapterN: this.timers = [] (nothing from before may fire inside the chapter)
	void Note(const FString& S);           // the notebook, once
	void AddDoc(const FS47Doc& Doc);       // a paper filed (replaces one with the same id)
	TArray<FS47Doc>& DocsMutable() { return DocsList; }
	TArray<FString>& NotesMutable() { return NotesList; }
	void Toast(const FString& S, float Secs = 3.2f);
	double Gt() const { return GameTime; }
	void SetClock(double Sec) { ClockSec = Sec; }
	void AimArray(double Az, double El, double Slew) { PointDishes(Az, El, Slew); }
	// The console's card: the evening observing run while one is scheduled (S47Observing), else the stage's.
	const FS47Profile& ConsoleProfile() const;
	double ConsoleQuality() const { return Rx.Quality(ConsoleProfile()); }
	void SetCinematic(bool bOn) { bCinematic = bOn; }
	// the chapters, stepped after the prologue's own beat (Prologue.update: ch1.update runs in
	// every chapter, the others in their own). ChapterKey is 'prologue' until the first begins.
	UPROPERTY() TArray<TObjectPtr<class US47Chapter>> Chapters;
	FString ChapterKey = TEXT("prologue");
	TSet<FString> Begun; // the chapters begun since the night started (a chapter's `started`: part of the saved case)
	class US47Chapter* Chapter(const FString& Key) const;
	// the chapter in play from now on (beginChapterN): the others stop leading
	void BeginChapter(const FString& Key, const TSharedPtr<class FJsonObject>& Saved = nullptr);
	// main.ts chapterBreak: fade, the next chapter begins behind the black, the card, then back
	struct FBreak { FString Closed, Recap, Next, Title; TFunction<void()> Begin; };
	void ChapterBreak(const FBreak& B);
	TFunction<void(const FBreak&)> OnChapterCard; // the card itself (ui.chapterCard), when the UI has one
	// chapterBreak's finish, from 6.4 s on: false while the card's narration still holds the break (asked
	// again a tenth of a second later); true once the card may go, and it has taken it away. Unset: it goes.
	TFunction<bool()> ChapterCardDone;
	// Prologue.ts jump('chapterN'): each chapter registers how to start at it (jumpChapterN)
	TMap<FString, TFunction<void()>> Jumps;
	// Prologue.ts afterPrologue(saved): the night as the prologue left it (or as saved)
	void AfterPrologue(const TSharedPtr<class FJsonObject>& Saved);
	double EventClockSec() const { return EventClock; }
	// the saved case (story/state.ts SavedCase) the chapters restore from, when there is one
	TFunction<TSharedPtr<class FJsonObject>()> LoadCase;
	TFunction<void(const FString&)> OnCheckpoint; // Prologue.onCheckpoint: 'chapter1' ... (autosave)
	// Prologue.doorChanged: a door opened or closed; every chapter hears it
	void DoorChanged(const FString& Id, bool bOpen);

	// ---------- SARO's own (story/saroDark.ts, story/arrayBack.ts, story/dishesFollow.ts) ----------
	/** A line in the console log from outside the story, at its own time (unset: now), in the
	 *  night's order among the others (Prologue.consoleLog). */
	void ConsoleLog(const FString& Text, TOptional<double> At = {});
	/** K3 goes first, in chapter one (story/saroDark.ts plays the clack as it is heard from where
	 *  the player is): it trips (true), or, already open since the impact, it has nothing left to
	 *  trip and the log writes it down anyway (false). */
	bool RelayClack();
	/** The rack's lever and the bank, and the teleprinter, in three's world terms. */
	FVector RackAt() const;
	FVector TelexAt() const;
	/** Bank 3 is in: powered, K3 closed. */
	bool BankIn() const { return bPowered && !NS.bK3Open; }
	/** Hondo Valley's telex has printed: on the roll, in the log (at `At`, in the night's order),
	 *  kept with the case. */
	void HondoTelex(double Filed, TOptional<double> At = {});
	/** What is on the telex roll now: last evening's traffic, and Hondo Valley's once it has come. */
	TArray<FString> TelexRoll() const;
	/** SARO without power: the radio on the cabinets goes with it, and comes back. */
	void PowerOut(bool bOut);
	/** The power is out at SARO (story/saroDark.ts): the screens stay dark. */
	bool bDark = false;
	/** main.ts game.arrayStatus and game.arrayText (story/arrayBack.ts), while a try runs; and each
	 *  dish's own state for the status screen's rows (moving by itself, or rogue or back). */
	TFunction<TOptional<FS47ArrayStatus>()> ArrayStatus;
	TFunction<FString()> ArrayText;
	TFunction<bool(int32)> DishSpinning, DishRogue;
	// Prologue.printerHook: the printer by the rack has a sheet of the archive's phone's in it. Label
	// empty: null (the printer's own words).
	struct FPrinterHook { TFunction<FString()> Label; TFunction<bool()> Use; };
	FPrinterHook PrinterHook;
	/** Prologue.decoder: the signal processor holds the tape once the anomaly is locked (02:14 and
	 *  after; story/Decoder.ts). Made in Bind, once the screen and the things to use are there. */
	TSharedPtr<class FS47DecoderDesk> Decoder;
	/** Unreal only (Tom's request, S47ArrayShot.h): the camera at the window while the array turns. */
	TSharedPtr<class FS47ArrayShot> Shot;

	static constexpr double StartClock = 23 * 3600 + 41 * 60;
	static constexpr double SkipClock = 2 * 3600 + 13 * 60 + 41;
	static constexpr double EventAz = 26, EventEl = 32, ParkAz = 18, SurveyEl = 48;

private:
	TWeakObjectPtr<AS47Web> Saro;
	TWeakObjectPtr<AS47HUD> Hud;
	TWeakObjectPtr<US47Audio> Audio;
	TUniquePtr<FS47OpsTerminal> OpsTerminal;

	ES47Phase CurrentPhase = ES47Phase::Intro;
	double ClockSec = StartClock;
	double EventClock = 2 * 3600 + 15 * 60 + 12;
	bool bCinematic = false, bPaused = false, bLogRead = false, bAlarmOn = false, bSkipQueued = false, bRingPending = false, bRadioOn = false, bRadioHeard = false;
	enum class ECoffee : uint8 { None, Poured, Drunk } Coffee = ECoffee::None;
	TArray<FString> NotesList;
	TArray<FS47Doc> DocsList;
	FS47NightState NS;
	struct FTimer { double At; TFunction<void()> Fn; FString Tag; };
	TArray<FTimer> Timers;
	float Flicker = 0.f, DipT = 0.f, LedT = 0.f, LookAssist = 0.f;
	bool bMugFalling = false;
 bool bFeederPowerHeld = false;

	void SetPhase(ES47Phase P) { CurrentPhase = P; }
	void Log(const FString& Text);
	void FilePaper(const FS47Doc& Doc);
	void SetLeds(bool bOn);
	void ApplyK3(bool bOpen);
	void TripK3(bool bSound = true);
	void ResetK3();
	void PowerUp(bool bInstant);
	void StagePassed();
	void RelayFirst();
	void LightsDip();
	void TimeSkip();
	void EnterResidual();
	void Alarm();
	void StartSolve();
	void StartPrint();
	void TearPrintout();
	void Ring();
	void Answer();
	void StartCountdown();
	void Event();
	void TurnDishes();
	void Arrived();
	void Finish();
	void ToggleRadio();
	void LogClosed();
	void WorkOrderClosed();
	void PointDishes(double Az, double El, double Slew);
	void OpenDoc(const FS47Doc& Doc, TFunction<void()> OnClose);
	void ResetAll();
};
