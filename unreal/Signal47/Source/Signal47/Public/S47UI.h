#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "S47Docs.h"
#include "S47Graphics.h"
#include "UObject/Object.h"
#include "S47UI.generated.h"

class AS47HUD;
class SWidget;
struct FS47Task;

// ---------- the screens' options (UI.ts title, openingCard, pause, chapterCard, notebook) ----------

// UI.title
struct FS47TitleOpts
{
	TFunction<void()> OnStart, OnContinue, OnLoad, OnSettings;
	FString Cont;                         // "chapter · place · clock · played"; empty: nothing to continue
	bool bCanLoad = false;
	bool bCalm = false;                   // flash: { calm, onPick }
	TFunction<void(bool)> OnFlashPick;
	bool bGate = false, bTouch = false;
	TFunction<void()> OnFirstTouch;       // main.ts firstTouch: the first key or click on the title (the music)
	TFunction<void()> OnQuit;             // the desktop's Quit; unset: not offered
	TFunction<void()> OnDev;              // the developer screen; unset: not offered
	TFunction<void(const FString&)> OnSettingsAt; // the settings at a tab (the note's link; US47UI::Title sets it)
	FString Version;                      // the corner's "UNREAL 0.0.1 · TAPE B"
};
// UI.openingCard: its actions, and the view it gives back (OpeningView)
struct FS47OpeningActions { TFunction<void()> Begin, Back, Closed; };
struct FS47OpeningView { TFunction<void()> Close; TFunction<void(const FString&)> State; };
// UI.pause (the settings shared by the pause menu and the title screen)
struct FS47PauseOpts
{
	TFunction<void()> OnResume, OnTitle;
	float Volume = 0.8f, Sens = 1.f, Music = 0.7f, Radio = 1.f;
	TFunction<void(float)> OnVolume, OnSens, OnMusic, OnRadio;
	bool bCharacters = true, bSerling = true, bKinski = false, bArtBell = true, bRadioCast = true;
	TFunction<void(bool)> OnCharacters, OnSerling, OnKinski, OnArtBell, OnRadioCast;
	TFunction<void()> OnSave, OnLoad;     // unset: not offered
	FString Quality = TEXT("high");       // 'ultra' | 'high' | 'low'
	TFunction<void(const FString&)> OnQuality;
	FS47GraphicsSettings Graphics;
	TFunction<void(const FS47GraphicsSettings&)> OnGraphics;
	TFunction<void()> OnReapplyGraphics; // display ApplySettings also resets scalability CVars
	bool bUltraOk = false;
	FString Title;                        // empty: 'Paused'
	bool bSettingsOnly = false;
	FString Picture = TEXT("carpenter");  // 'off' | 'vhs' | 'carpenter' | '16mm' | 'super8'
	TFunction<void(const FString&)> OnPicture;
	float PictureStrength = 1.f; TFunction<void(float)> OnPictureStrength;
	bool bInvertY = false; TFunction<void(bool)> OnInvertY;
	float PadLookSpeed = 1.f; TFunction<void(float)> OnPadLookSpeed;
	bool bPadInvertY = false; TFunction<void(bool)> OnPadInvertY;
	float Fov = 70.f; TFunction<void(float)> OnFov;
	bool bLargeText = false; TFunction<void(bool)> OnLargeText;
	bool bCalmFlash = false; TFunction<void(bool)> OnCalmFlash;
	bool bStillShots = false; TFunction<void(bool)> OnStillShots;
	bool bHeadMotion = true; TFunction<void(bool)> OnHeadMotion;
	bool bAbduct = true; TFunction<void(bool)> OnAbduct; // unset: not offered
	// the Unreal version's own: the brightness before the tone curve (0..1, .5 as made), and the keys
	// bound in Settings > Controls (S47Keys; the store keeps them)
	float Brightness = 0.5f; TFunction<void(float)> OnBrightness;
	TFunction<void()> OnKeys;
	FString Tab;                          // the settings' tab to open at: game, picture, sound, voices, controls, accessibility, graphics
	// the pause menu's status and tonight's card
	FString Chapter;                      // "Chapter 2: The Amended Record"
	FString Place;                        // "Control room, SARO"
	double Clock = -1;                    // the night's clock (seconds after midnight); -1: none
	FString CaseLine;                     // "CASE 1 · SAVED 2 MIN AGO"
	FString Objective;                    // the current objective, "CONTROL ROOM // THE SUPERVISOR LINE IS RINGING"
	TArray<FString> OpenTasks;            // tonight's tasks not done yet
	FString LastNote;                     // the latest note in the notebook
};
// UI.chapterCard
struct FS47ChapterCardOpts { FString Closed, Recap, Next, Title, Clock; };
// the notebook's map (ui/areaMap.ts MapView), kit (KitItem) and people (PersonCard)
struct FS47MapAt { FString Spot; double T = 0, M = 0, Mi = 0; bool bT = false, bM = false, bMi = false; };
struct FS47MapMark { FString Kind; double Clock = 0; FString Line; TOptional<FS47MapAt> At; };
struct FS47MapView { TOptional<FS47MapAt> At; bool bStation = false, bOldRoad = false; TArray<FS47MapMark> Marks; bool bLineC = false; };
struct FS47KitItem { FString Name, Note, State, Art, Tag; }; // Art: a kit drawing (ui/kitArt.ts), empty for none
struct FS47PersonCard { FString Name, Role, Note, Portrait; }; // Portrait: a picture's art name, empty while there is none
// the album as the notebook shows it (core/album.ts AlbumView): the roll, and each free print as far
// as it has come up (Dev, 0..1); Look is told each time a print is opened big
struct FS47AlbumView
{
	int32 Roll = 36, Left = 0;
	struct FShot { FString Id, Url, Text, Meta; double Dev = 1; bool bOdd = false; };
	TArray<FShot> Shots;
	TFunction<void(const FString&)> Look;
};
struct FS47NbExtras
{
	TOptional<FS47MapView> Map;          // drawn by the notebook (areaMap)
	TArray<FString> MapDescription;
	FString MapNote;                     // a line in pencil under the map
	TOptional<TArray<FS47KitItem>> Kit;
	TOptional<TArray<FS47PersonCard>> People;
	bool bTouch = false;                 // a phone: its controls first
	TOptional<FS47AlbumView> Album;      // the free frames (core/album.ts)
};

// UI.viewfinder's options
struct FS47Viewfinder
{
	float X = 0, Y = 0, W = 0, H = 0; // the frame in screen pixels
	FString Frame, Status;
	bool bOk = false;
	bool bTouch = false;
	bool bHasFree = false;
	FString FreeText;
	bool bFreeOk = false;
};

// A reading the in-game screens offer as an action (the paper's READ ALOUD, the notebook's): the
// reader and the texts UI.readingControls takes, without its box. Toggle() starts it (the other
// readings stop first: onReadingStart) or stops it; bActive and Status() follow its state.
struct SIGNAL47_API FS47ReadingAction : public TSharedFromThis<FS47ReadingAction>
{
	TWeakPtr<class FS47ReadAloud> Reader;
	TWeakObjectPtr<class US47UI> UI;
	TArray<FString> Texts;
	bool bAvailable = false;          // the reader has a recording for every text
	bool bActive = false;             // loading or playing (aria-pressed)
	FString State;                    // '' until it has been started; then 'loading' | 'playing' | 'idle' | 'error'
	void Toggle();
	void Stop();
	FString Status() const;           // [data-reading-status]: the reader's name, Loading recording…, or why not
};

// ui/UI.ts: what the game asks of the screen. The names follow the web version, so a call ports
// as it reads there (ui.toast('...', 4) is UI->Toast(TEXT("..."), 4)). One modal at a time:
// a paper, a panel, the notebook or the board; game time stops while one is open.
UCLASS()
class SIGNAL47_API US47UI : public UObject
{
	GENERATED_BODY()

public:
	void Init(AS47HUD* InHud) { Hud = InHud; }

	void Toast(const FString& Text, float Secs = 3.2f, TFunction<double()> Clock = {});
	void ClearToasts();
	FString LastDescription() const;                 // the last toast that was not a caption (V)
	void Objective(const FString& Text);             // empty hides it
	void Examine(const FString& Text);               // a closer look; empty hides it
	void Fade(bool bOn, const FString& Text = FString());
	bool Faded() const;
	void ClockSeconds(const FString& Text);          // the amber seconds clock (chapter six); empty: the plain clock

	// a paper (UI.document)
	void Document(const FS47Doc& Doc, TFunction<void()> OnClose = nullptr);
	// any panel (UI.open / UI.close / UI.modal)
	void Open(TSharedRef<SWidget> Widget, TFunction<void()> OnClose = nullptr);
	void Close(bool bSilent = false);
	bool Modal() const;

	// the field camera (UI.viewfinder, UI.shutterFlash, UI.cameraButton)
	using FViewfinder = FS47Viewfinder;
	void Viewfinder(bool bOn, const FS47Viewfinder& V = FS47Viewfinder());
	void ShutterFlash();
	void CameraButton(bool bOn, bool bRaised = false) { bCameraButton = bOn; bCameraRaised = bRaised; }
	bool bCameraButton = false, bCameraRaised = false;

	// what is offered at the crosshair (UI.prompt): set by the main step from the interaction
	void Prompt(const FString& Label, const FString& Alt = FString());

	// ---------- the screens (S47UIScreens.cpp) ----------
	// UI.title: the title over the game (not a modal); RemoveScreen takes it away (titleEl.remove())
	TSharedPtr<SWidget> Title(const FS47TitleOpts& O);
	void Ungate(const TSharedPtr<SWidget>& El); // the first key or click: the menu comes in
	void RemoveScreen(const TSharedPtr<SWidget>& El);
	// UI.openingCard: a modal; the view closes it and shows the narration's state
	FS47OpeningView OpeningCard(const FS47OpeningActions& A);
	// UI.pause: the pause menu, or the title's settings (bSettingsOnly), at O.Tab or SettingsAt's tab
	void Pause(const FS47PauseOpts& O);
	void SettingsAt(const FString& Tab) { SettingsTab = Tab; }
	FString SettingsTab;
	// UI.chapterCard / chapterCut: the card on the black between two chapters; null takes it away.
	// With the narrator on, it reads the card (the closed line and its recap, then the next chapter, its
	// title and the clock: ChapterStage 'a', then 'b'); OnSpoken comes when that reading has settled (at
	// once without the narrator), as the promise chapterCard returns does.
	void ChapterCard(const FS47ChapterCardOpts* O, TFunction<void()> OnSpoken = nullptr);
	void ChapterCut();
	FString ChapterStage = TEXT("idle");        // 'idle' | 'a' | 'b' | 'text-b' | 'error-b' | 'cancelled'
	// UI.notebook / notebookAt: the journal in its tabs; the last tab used is kept
	void Notebook(const TArray<FS47Task>& Tasks, const TArray<FString>& Notes, const TArray<FS47Doc>& Docs, TFunction<void(const FS47Doc&)> OnDoc,
		const FString& Heading = TEXT("Tonight"), TFunction<void()> OnBoard = nullptr, const FS47NbExtras& Extras = FS47NbExtras());
	void NotebookAt(const FString& Tab) { NbTab = Tab; }
	FString NbTab = TEXT("tasks");
	TFunction<void(const FString&)> OnNotebookTab; // a page of the notebook has been turned to
	bool bLargeText = false;                       // html.text-large
	// ui.npcDialogue: the night's character voices, one queue (core/npcDialogue.ts; set by main.ts's
	// wiring, null until then). ui.speakNora is its SpeakNora.
	TSharedPtr<class FS47NpcDialogue> NpcDialogue;

	// ---------- the readers (core/readAloud.ts) ----------
	// ui.readAloud: Kinski reading papers and conversations ('kinskiReadAloud'); ui.narrator: the same
	// player with the Serling bank ('serlingNarrator'); ui.earlDialogue: Earl's voice (Caliche's). Set by
	// main.ts's wiring (S47Flow, S47Caliche), null until then.
	TSharedPtr<class FS47ReadAloud> ReadAloud, Narrator;
	TSharedPtr<class FS47EarlDialogue> EarlDialogue;
	/** get descriptionReader(): the narrator when it is on, else Kinski when he is on, else none. */
	TSharedPtr<class FS47ReadAloud> DescriptionReader() const;
	/** stopSpeech(): every reader and voice. */
	void StopSpeech();
	/** onReadingStart(reader): a reading begins (main.ts: the others stop, and the calls and the CB). */
	TFunction<void(class FS47ReadAloud*)> OnReadingStart;
	/** canDescribe (main.ts): a description may be read now (unset: undefined, as the web version's). */
	TFunction<bool()> CanDescribe;
	/** readingControls(parent, texts, label, kind): explicit playback only, the Read button and its status
	 *  (.read-aloud) as a widget to put where the box goes; nothing when the reader is off. 'document' and
	 *  'conversation' are Kinski's, the rest the description reader's. */
	TSharedRef<SWidget> ReadingControls(const TArray<FString>& Texts, const FString& Label = TEXT("Read aloud"), const FString& Kind = TEXT("conversation"));
	/** descriptionReadings(parent, selector, kind, extra): read the descriptions currently visible in Parent
	 *  (the texts S47Kit marks readable: .lp-text, .lp-small, .lp-q, .lp-paper p, .lp-steps span, .lp-card
	 *  span), following their changes, not frames. The widget is the box's place (nothing while there is
	 *  nothing to read, or the description reader is off). */
	TSharedRef<SWidget> DescriptionReadings(const TSharedRef<SWidget>& Parent, const FString& Kind = TEXT("panel-description"), TFunction<TArray<FString>()> Extra = nullptr);
	/** readLastDescription(): a gesture-accessible repeat (V), also after a short toast has disappeared. */
	void ReadLastDescription();
	/** A reading as an action of the in-game screens (FS47ReadingAction), for the reader Kind names as
	 *  ReadingControls does; null when that reader is off or there is nothing to read. */
	TSharedPtr<FS47ReadingAction> ReadingAction(const TArray<FString>& Texts, const FString& Kind);
	/** A line written into the notebook (AS47Story::Note): the HUD's NOTED slip (not while a case is
	 *  being opened, when its old lines come back). */
	void Noted(const FString& Text);

private:
	UPROPERTY() TObjectPtr<AS47HUD> Hud;
	// the pause menu at a line, and the settings over it (back to the pause) or over the title
	void PauseScreen(const TSharedRef<FS47PauseOpts>& O, int32 Sel);
	void SettingsScreen(const TSharedRef<FS47PauseOpts>& O, const FString& Tab, TFunction<void()> OnClose);
	TSharedPtr<class SS47ChapterCard> CardEl;
	int32 CardGeneration = 0;
	// examine(): the closer look's words, and its description read once the zoom has held them a moment
	FString ExamineText;
	bool bExamineActive = false, bDescriptionActive = false;
	FTSTicker::FDelegateHandle ExamineTimer;
	void ClearExamineTimer();
	void UpdateDescriptionButton();
	void PlayDescription(const FString& Text, bool bAutomatic = false);
};
