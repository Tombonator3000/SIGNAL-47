#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "S47Docs.h"
#include "S47Ops.h"
#include "S47Rx.h"
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

// Tonight's lines in the console log, which papers were read, relay K3 (NightShiftState).
struct FS47NightState { TArray<FString> Papers; bool bK3Open = false; TArray<FS47LogLine> Log; TOptional<double> SolveAt; bool bJacket = false; };

UCLASS()
class SIGNAL47_API AS47Story : public AActor
{
	GENERATED_BODY()

public:
	AS47Story();
	virtual void Tick(float DeltaSeconds) override;

	void Bind(AS47Web* InSaro, AS47HUD* InHud, US47Audio* InAudio);
	void Start();
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
	bool bPowered = false;
	double GameTime = 0;
	double BootT = -1, SolveT = -1, PrintT = -1, LineDeadT = -1;
	bool bRinging = false, bAnswered = false;
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
	void SetCinematic(bool bOn) { bCinematic = bOn; }
	// the chapters, stepped after the prologue's own beat (Prologue.update: ch1.update runs in
	// every chapter, the others in their own). ChapterKey is 'prologue' until the first begins.
	UPROPERTY() TArray<TObjectPtr<class US47Chapter>> Chapters;
	FString ChapterKey = TEXT("prologue");
	class US47Chapter* Chapter(const FString& Key) const;
	// the chapter in play from now on (beginChapterN): the others stop leading
	void BeginChapter(const FString& Key, const TSharedPtr<class FJsonObject>& Saved = nullptr);
	// main.ts chapterBreak: fade, the next chapter begins behind the black, the card, then back
	struct FBreak { FString Closed, Recap, Next, Title; TFunction<void()> Begin; };
	void ChapterBreak(const FBreak& B);
	TFunction<void(const FBreak&)> OnChapterCard; // the card itself (ui.chapterCard), when the UI has one
	// Prologue.ts jump('chapterN'): each chapter registers how to start at it (jumpChapterN)
	TMap<FString, TFunction<void()>> Jumps;
	// Prologue.ts afterPrologue(saved): the night as the prologue left it (or as saved)
	void AfterPrologue(const TSharedPtr<class FJsonObject>& Saved);
	double EventClockSec() const { return EventClock; }
	// the saved case (story/state.ts SavedCase) the chapters restore from, when there is one
	TFunction<TSharedPtr<class FJsonObject>()> LoadCase;
	TFunction<void(const FString&)> OnCheckpoint; // Prologue.onCheckpoint: 'chapter1' ... (autosave)

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
