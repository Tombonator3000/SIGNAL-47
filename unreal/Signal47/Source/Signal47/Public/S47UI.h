#pragma once

#include "CoreMinimal.h"
#include "S47Docs.h"
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
	bool bUltraOk = false;
	FString Title;                        // empty: 'Paused'
	bool bSettingsOnly = false;
	FString Picture = TEXT("vhs");        // 'off' | 'vhs'
	TFunction<void(const FString&)> OnPicture;
	bool bInvertY = false; TFunction<void(bool)> OnInvertY;
	float Fov = 70.f; TFunction<void(float)> OnFov;
	bool bLargeText = false; TFunction<void(bool)> OnLargeText;
	bool bCalmFlash = false; TFunction<void(bool)> OnCalmFlash;
	bool bStillShots = false; TFunction<void(bool)> OnStillShots;
	bool bAbduct = true; TFunction<void(bool)> OnAbduct; // unset: not offered
};
// UI.chapterCard
struct FS47ChapterCardOpts { FString Closed, Recap, Next, Title, Clock; };
// the notebook's map (ui/areaMap.ts MapView), kit (KitItem) and people (PersonCard)
struct FS47MapAt { FString Spot; double T = 0, M = 0, Mi = 0; bool bT = false, bM = false, bMi = false; };
struct FS47MapMark { FString Kind; double Clock = 0; FString Line; TOptional<FS47MapAt> At; };
struct FS47MapView { TOptional<FS47MapAt> At; bool bStation = false, bOldRoad = false; TArray<FS47MapMark> Marks; bool bLineC = false; };
struct FS47KitItem { FString Name, Note, State, Art, Tag; }; // Art: a kit drawing (ui/kitArt.ts), empty for none
struct FS47PersonCard { FString Name, Role, Note, Portrait; }; // Portrait: a picture's art name, empty while there is none
struct FS47NbExtras
{
	TOptional<FS47MapView> Map;          // drawn by the notebook (areaMap)
	TArray<FString> MapDescription;
	FString MapNote;                     // a line in pencil under the map
	TOptional<TArray<FS47KitItem>> Kit;
	TOptional<TArray<FS47PersonCard>> People;
	bool bTouch = false;                 // a phone: its controls first
	// (album: the free frames, core/album.ts, are not in the Unreal version yet)
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

// ui/UI.ts: what the game asks of the screen. The names follow the web version, so a call ports
// as it reads there (ui.toast('...', 4) is UI->Toast(TEXT("..."), 4)). One modal at a time:
// a paper, a panel, the notebook or the board; game time stops while one is open.
UCLASS()
class SIGNAL47_API US47UI : public UObject
{
	GENERATED_BODY()

public:
	void Init(AS47HUD* InHud) { Hud = InHud; }

	void Toast(const FString& Text, float Secs = 3.2f);
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
	// UI.pause: the pause menu, or the title's settings (bSettingsOnly)
	void Pause(const FS47PauseOpts& O);
	// UI.chapterCard / chapterCut: the card on the black between two chapters; null takes it away
	void ChapterCard(const FS47ChapterCardOpts* O);
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

private:
	UPROPERTY() TObjectPtr<AS47HUD> Hud;
	TSharedPtr<class SS47ChapterCard> CardEl;
	int32 CardGeneration = 0;
};
