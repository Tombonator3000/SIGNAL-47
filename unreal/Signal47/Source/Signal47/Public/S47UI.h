#pragma once
#include "S47TonightNote.h" // (TONIGHT-NOTE-20261109; on the blank line, so that the generated header's lines stay)
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
	TFunction<void()> OnStart, OnContinue, OnLoad, OnSettings, OnFreeform;
	FString Cont;                         // "chapter · place · clock · played"; empty: nothing to continue
	bool bCanLoad = false;
	bool bCalm = false;                   // flash: { calm, onPick }
	bool bHide47 = false, bErase47 = false; // completed Story title: persisted absence / pending first presentation
	float Erase47InitialTime = 0.f;        // resume a pending presentation after a title rebuild
	TFunction<void()> OnErase47Start, OnErase47Done;
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
	TFunction<void(float)> OnVolume, OnSens, OnMusic, OnRadio; FString MusicReacts = TEXT("on"), Stingers = TEXT("full"); TFunction<void(const FString&)> OnMusicReacts, OnStingers; // SIDEBAND-20261113: on | soft | off, and full | soft | off
	bool bCharacters = true, bSerling = true, bArtBell = true, bRadioCast = true;
	TFunction<void(bool)> OnCharacters, OnSerling, OnArtBell, OnRadioCast;
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
	bool bHeadMotion = true; TFunction<void(bool)> OnHeadMotion; bool bCameraMoments = true; TFunction<void(bool)> OnCameraMoments; // N1110: the short snaps of the view in events (FS47CameraMoment)
	bool bLocationCards = true; TFunction<void(bool)> OnLocationCards; // UI-20261112: the place's name typed out on arriving (S47LocationCards)
	bool bVehicleMirrors = true; TFunction<void(bool)> OnVehicleMirrors; // Graphics: the Jeep's live mirrors (S47Mirrors)
	bool bAbduct = true; TFunction<void(bool)> OnAbduct; // unset: not offered
	// the Unreal version's own: the brightness before the tone curve (0..1, .5 as made), and the keys
	// bound in Settings > Controls (S47Keys; the store keeps them)
	float Brightness = 0.5f; TFunction<void(float)> OnBrightness;
	// HUD-D-20261109: the HUD's colours (S47HudGauges::FHudPalette's id: teal, amber, green, white, red), at once and kept
	FString HudColor = TEXT("teal"); TFunction<void(const FString&)> OnHudColor;
	TFunction<void()> OnKeys;
	FString Tab;                          // the settings' tab to open at: game, picture, sound, voices, controls, accessibility, graphics
	// the pause menu's status and tonight's card
	FString Chapter;                      // "Chapter 2: The Amended Record"
	FString Place;                        // "Control room, SARO"
	double Clock = -1;                    // the night's clock (seconds after midnight); -1: none
	FString CaseLine;                     // "CASE 1 · SAVED 2 MIN AGO"
	FString Objective;                    // the current objective, "CONTROL ROOM // THE SUPERVISOR LINE IS RINGING"
	TArray<FString> OpenTasks;            // tonight's tasks not done yet
	FString LastNote; S47TonightNote::FNote Note; // the latest note in the notebook; TONIGHT-NOTE-20261109: tonight's plan (S47TonightNote)
};
// UI.chapterCard
struct FS47ChapterCardOpts { FString Closed, Recap, Next, Title, Clock; };
// the notebook's map (ui/areaMap.ts MapView), kit (KitItem) and people (PersonCard)
struct FS47MapAt { FString Spot; double T = 0, M = 0, Mi = 0; bool bT = false, bM = false, bMi = false; };
struct FS47MapMark { FString Kind; double Clock = 0; FString Line; TOptional<FS47MapAt> At; };
// Freeform: a place found or heard of, and a bearing from SARO. East of the highway and south of SARO, in metres. LOOP-1-20261109: a pin inside a live bearing's sector is marked (bInSector); a sector from elsewhere than SARO (bFrom: the truck's direction finder) starts at From, road-local x and z; Fade 1 fresh to 0 cold.
struct FS47MapPin { double East = 0, South = 0; FString Label; bool bHeard = false, bInSector = false; };
struct FS47MapSector { double Az = 0, Spread = 0; FString Label; bool bFrom = false; double FromX = 0, FromZ = 0; float Fade = 1.f; };
// Freeform's survey sheet (SS47FieldMap, MAP-20261108): a pencil mark of the player's own, road-local metres (x east, z south).
struct FS47MapNote { double X = 0, Z = 0; int32 N = 0; };
// Freeform's sheet: work the player has now, written in by the place in ink ("S-05 drive fault", "fix the feeder"), road-local.
struct FS47MapJob { double X = 0, Z = 0; FString Text; };
struct FS47MapView
{
	TOptional<FS47MapAt> At; bool bStation = false, bOldRoad = false; TArray<FS47MapMark> Marks; bool bLineC = false; TArray<FS47MapPin> Pins; TArray<FS47MapSector> Sectors; TArray<FVector2D> Crossings; // LOOP-1-20261109: Crossings, where two live bearings from different places cross, road-local (FS47Places::Cross)
	// Freeform: the survey sheet instead of the sketch. Where the player is and which way they face (degrees from north,
	// clockwise), and where the truck stands, all road-local, whatever area each is in (S47FieldMap::Me).
	bool bField = false, bMe = false, bTruck = false, bDriving = false;
	double MeX = 0, MeZ = 0, MeHeading = 0, TruckX = 0, TruckZ = 0, TruckHeading = 0;
	FString MeArea;
	TArray<FS47MapNote> Notes;
	TArray<FS47MapJob> Jobs;
};
struct FS47KitItem { FString Name, Note, State, Art, Tag; }; // Art: a kit drawing (ui/kitArt.ts), empty for none
struct FS47PersonCard { FString Name, Role, Note, Portrait; }; struct FS47NbScribble { FString Side, Box, Role, Text, Follow, FollowText; bool bStruck = false; }; struct FS47NbUnknown { FString Title, Stage, Next, Gave; struct FBox { FString Label, How, Note; bool bFilled = false; }; TArray<FBox> Boxes; }; // EVIDENCE-DOSSIER-20261113: a file of the unknown on THEM (S47Dossier::FFile): "UNKNOWN 2", its stage, how it is kept from here, six boxes. LOOP-DOSSIER-GIVES-20261113: Gave, what reaching the stage gave, one line under it. Portrait: a picture's art name, empty while there is none. MARGINALIA-20261109: a scribble in WHO & WHAT (S47Marginalia): Side "people" or "them", the box it is in, the line, where it points (Follow "map" or "board"), struck when proved wrong
// the album as the notebook shows it (core/album.ts AlbumView): the roll, and each free print as far
// as it has come up (Dev, 0..1); Look is told each time a print is opened big
struct FS47AlbumView
{
	int32 Roll = 36, Left = 0;
	struct FShot { FString Id, Url, Text, Meta; double Dev = 1; bool bOdd = false; };
	TArray<FShot> Shots;
	TFunction<void(const FString&)> Look;
	// Freeform only: throw a print away. Story leaves both unset.
	TFunction<bool(const FString&)> CanDiscard;
	TFunction<void(const FString&)> OnDiscard;
};
struct FS47NbWorkItem
{
	FString Id, Title, Status, NextAction, Location;
	TArray<FString> Hints;
	bool bDone = false, bCanFollow = false, bEarlier = false, bFollowed = false; bool bCard = false; // bCard: a job card, folded under one line on NOW (PT-NOTEBOOK-20261109)
};
struct FS47NbGuideEntry { FString Id, Title, Text; }; struct FS47NbEvent { FString When, Place, Kind, Text; bool bOwn = false; }; // D48-20261110: one line of the EVENTS page (D46-19)
struct FS47NbExtras
{
	bool bFreeform = false;
	TArray<FS47NbWorkItem> WorkItems;
	TArray<FS47NbGuideEntry> GuideEntries;
	TFunction<int32(const FString&)> HintLevel, RevealHint;
	TFunction<bool(const FString&)> EntryRead;
	TFunction<void(const FString&)> MarkRead, OnFollow;
	TOptional<FS47MapView> Map;          // drawn by the notebook (areaMap)
	// Freeform's sheet: a pencil mark put down or rubbed out at road-local x and z, or rubbed out within so many metres of one there; the marks after.
	TFunction<TArray<FS47MapNote>(double, double, double)> OnMapNote;
	TArray<FString> MapDescription;
	FString MapNote;                     // a line in pencil under the map
	TOptional<TArray<FS47KitItem>> Kit;
	TOptional<TArray<FS47PersonCard>> People;
	bool bTouch = false;                 // a phone: its controls first
	TOptional<FS47AlbumView> Album;      /* the free frames (core/album.ts) */ bool bNow = false; TArray<FS47NbNowLine> Now; FString NowClock; TArray<FString> Goals; TOptional<TArray<FS47Doc>> Papers; TArray<FS47NbScribble> Scribbles; bool bEvents = false; TArray<FS47NbEvent> Events; TArray<FString> EventKinds, EventPhrases; FString EventNow, EventPlace; TFunction<bool(int32, int32)> OnLogEvent; TFunction<TArray<FS47NbEvent>()> GetEvents; TArray<FS47NbUnknown> Unknowns; // EVIDENCE-DOSSIER-20261113: THEM's files of the unknown (Freeform). D48-20261110 (D46-19): the EVENTS page, Freeform only. MARGINALIA-20261109: WHO & WHAT's pencil (Freeform). PT-NOTEBOOK-20261109 (Freeform): NOW as the note's lines (FS47NbNowLine), the clock's advice in red pencil, DOCUMENTS as the pinboard's tray (FS47PinBoard::Papers); unset: as before (Story)
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

	void Toast(const FString& Text, float Secs = 3.2f, TFunction<double()> Clock = {}); void Alert(const FString& Text, float Secs = 4.f); // Alert, MSG-ONE-20261109: a line of danger, first in the one queue (FS47MessageQueue)
	// WEEKLY-CASE-20261109: the player's own thought (S47Thought, at the end of this file): through the queue, one at a time. Empty does nothing.
	void Thought(const FString& Text, float Secs = 4.5f);
	// UI-20261112 (Tom): the location card, bottom left: the place, the town and state, the time, typed out a letter at a time
	// (S47LocationCards has what and when; SS47Hud draws and ticks it). False when there is no HUD to show it or one is still up.
	bool LocationCard(const FString& Line1, const FString& Line2, const FString& Line3 = FString());
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
	// wiring, null until then). (ui.speakNora is gone with N. Vega's voice: VOICE-OUT-3.)
	TSharedPtr<class FS47NpcDialogue> NpcDialogue;

	// ---------- the readers (core/readAloud.ts) ----------
	// ui.narrator: the reader with the Serling bank ('serlingNarrator'); ui.earlDialogue: Earl's voice
	// (Caliche's). Set by main.ts's wiring (S47Flow, S47Caliche), null until then. (KINSKI-READ-OUT, Tom
	// 9 November 2026: ui.readAloud, the Kinski reader for papers and conversations, is gone.)
	TSharedPtr<class FS47ReadAloud> Narrator;
	TSharedPtr<class FS47EarlDialogue> EarlDialogue;
	/** get descriptionReader(): the narrator when it is on, else none. */
	TSharedPtr<class FS47ReadAloud> DescriptionReader() const;
	/** stopSpeech(): every reader and voice. */
	void StopSpeech();
	/** onReadingStart(reader): a reading begins (main.ts: the others stop, and the calls and the CB). */
	TFunction<void(class FS47ReadAloud*)> OnReadingStart;
	/** canDescribe (main.ts): a description may be read now (unset: undefined, as the web version's). */
	TFunction<bool()> CanDescribe;
	/** readingControls(parent, texts, label, kind): explicit playback only, the Read button and its status
	 *  (.read-aloud) as a widget to put where the box goes; nothing when the reader is off. 'document' and
	 *  'conversation' have no reader (text only, KINSKI-READ-OUT), the rest the description reader's. */
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
	/** SAVE-SARO-20261108: the small LOG FILED stamp over the HUD's gauges, when the autosave on leaving SARO is written. */
	void Filed(const FString& Line);

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

// WEEKLY-CASE-20261109, patch 3: the player's own thoughts. A line that starts with Mark goes through the one queue (FS47MessageQueue)
// in its speech lane, below Dale and danger (EPri::Radio), so that it is one at a time with every voice and never over one; the HUD
// draws it as a thought, not a caption: no speaker, no box, the hand in pencil, slanted, at the foot. "~ " is the papers' pencil mark.
namespace S47Thought
{
	inline const TCHAR* Mark() { return TEXT("~ "); }
	inline bool Is(const FString& Line) { return Line.StartsWith(Mark()); }
	inline FString Line(const FString& Text) { return FString(Mark()) + Text.TrimStartAndEnd(); }
	inline FString Words(const FString& Line) { return Is(Line) ? Line.Mid(FCString::Strlen(Mark())) : Line; }
}
