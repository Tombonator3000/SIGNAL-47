#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Journal.h"
#include "S47Saves.h"
#include "S47Graphics.h"
#include "S47SaroSave.h"
#include "S47UI.h"
#include "UObject/Object.h"
#include "S47Flow.generated.h"

class US47Main;
class ACameraActor;
class UAudioComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class FS47Opening;

// The game's flow from web/src/main.ts: the title (boot, showTitle, ungate, the title camera),
// the opening card and a new night (requestNewNight, startGame and the order it puts a save
// back in), leaving a night (leavePlay, toTitle), the pause menu and the settings it shares
// with the title (openPause, settings(), the store), the saves (requestAutosave, flushAutosave,
// writeSave, autosaveNow, saveBlock, placeName, captureThumb), the notebook's keys and its map,
// kit and people, and the chapter card (chapterBreak's card).
//
// The parts of story/Prologue.ts that belong to saving are here as well (composeCase, the saved
// case's shape, checkpointName, saveBlock, snapshotCase, openNotebook), as they read the host
// (AS47Story) from outside; the host keeps its LoadCase and OnCheckpoint hooks, which this sets.
//
// What main.ts saves for the night's systems that are not in the Unreal version yet (the truck,
// the time away, the odometer, and in the case the threads, the array and Hondo's telex) is kept
// from the save that was opened and written back unchanged, so a save from the web version keeps
// its fields. The night's layer (S47Night: the flashlight, townSeen, the seen log, the curve, the
// album) writes its own and is put back in startGame's order.
UCLASS()
class SIGNAL47_API US47Flow : public UObject
{
	GENERATED_BODY()

public:
	void Init(US47Main* InM);
	// boot(): the title, or straight into play when the game is started with a test flag
	// (-S47...; -S47Title forces the title, -S47Case=N plays case N)
	void Boot();
	// main.ts step() for the flow: the title camera; then flushAutosave after the frame
	void Step(double Dt);

	// ---------- saving (main.ts) ----------
	void RequestAutosave(bool bMilestone);
	void FlushAutosave();
	bool AutosaveNow(FString* OutError = nullptr);
	TOptional<FS47SaveMeta> WriteSave(const FString& Kind, int32 Slot, FString& OutError);
	FString SaveBlock() const;                 // why the game cannot be saved now; empty when it can
	FString FreeformSaveBlock() const;         // Freeform's own reasons, for developer runs too: SaveBlock says a developer run never saves
	// SAVE-SARO-20261108 (S47SaroSave.h): the log is filed at the station, and an autosave is written each time the player leaves it
	FString ManualSaveBlock() const;           // the save menu's: in Freeform, anywhere but inside SARO; then SaveBlock
	void DepartingByTruck();                   // US47World::StartTrip, before the trip begins: a departure in a vehicle
	void DevDeparture();                       // F10: armed as if the player had been inside a while; the departure shows its mark
	const FString& DepartureNote() const { return DepartureText; } // what became of the last departure
	FString PlaceName() const;                 // the place on a save's card
	FString PlaceName(const FVector& P) const; // placeName(p): the place of a point (the truck's, in the cab)
	FString CaptureThumb();                    // 192 x 108 JPEG of the player's view
	S47J::FObj GameState();                    // the GameState a save holds (writeSave)

	// ---------- the case (story/Prologue.ts) ----------
	S47J::FObj ComposeCase() const;            // SavedCase: s, ch2..ch5, ns, notes, clock, board
	S47J::FObj SnapshotCase() const;           // null in the prologue, which keeps no case
	FString CheckpointName() const;            // 'start' | 'residual' | 'chapter1' ...
	FString GameSaveBlock() const;             // Prologue.saveBlock
 int32 JournalHintLevel(const FString& Id) const;
 int32 RevealJournalHint(const FString& Id);
 bool JournalEntryRead(const FString& Id) const;
 void MarkJournalEntryRead(const FString& Id);
	void OpenNotebook();                       // Prologue.openNotebook with main.ts's extras

	// ---------- the flow ----------
	void ShowTitle();
	void ShowTitleAgain();
	// The developer screen (SS47Dev.h): start points by relaunching with the test flags, and F9's own.
	void OpenDev();
	void DevRelaunch(const FString& Args);
	void DevSave();
	static FString DevDir();
	void RequestNewNight(int32 InCaseId, bool bReplace = false);
	void RequestNewFreeform(bool bStory = false);
	/** STORY-ON-FREEFORM-01: NEW STORY is a Freeform session marked as Story (saved as storyDirector{v:1}); the story director builds on it. */
	bool StoryNight() const { return bStoryNight; }
	void OpenSave(const FS47SaveMeta& Meta);
	struct FStart { int32 CaseId = 0; double Playtime = 0; S47J::FObj State; FString Jump; bool bDev = false; bool bReplaceCase = false; bool bFreeform = false; bool bLethal = false; bool bStoryNight = false; };
	void StartGame(const FStart& O);
	void LeavePlay();
	void ToTitle();
	void Finale();                             // main.ts finale: the title, credits and 1947 photograph
	/** STORY steg 6 (SC13): the end of a Story night (FS47StoryDirector's end beat). Black, then the old Story's end cards (SS47Ending: the
	 *  title, the credits, the 1947 photograph), the Story marked completed for the title, and back to it. A Story night in play only. */
	void StoryNightEnd();
	void OpenPause();
	FS47PauseOpts Settings();
	/** main.ts stopNarration(except): readAloud, the narrator, Earl and the character voices stop, all but Except. */
	void StopNarration(const void* Except);

	// what the save holds of the night's systems not ported yet (the hooks to give them back once they are)
	TFunction<S47J::FObj()> SaveBoard;                // story/caseBoard.ts layout: { pos: { id: [x, y] } }
	TFunction<void(const S47J::FObj&)> LoadBoard;     // given the case's board when a save is opened (afterPrologue's boardSave)
	// the notebook's kit: the field camera (chapter one's FieldCamera.have) and the album's roll (core/album.ts rollText)
	TFunction<bool()> HaveCamera;
	TFunction<FString()> RollText;

	FS47SaveStore Saves;
	int32 CaseId = 0;               // the case being played, 0 before a night starts
	bool bDevRun = false;           // a run that is never saved
	bool bStoryNight = false;      // this Freeform session is NEW STORY's (STORY-ON-FREEFORM-01)
	bool bStarting = false;         // between Start or Continue and the first playable frame
	bool bRestoringNow = false;
	bool bPendingAuto = false;
	S47J::FObj Restoring;           // the save being opened; the chapters read their case from it
	S47J::FObj Carry;               // the save that was opened, for the fields this version does not write

	// the settings (main.ts store), as they are now
	float Volume = 0.8f, MusicLevel = 0.7f, RadioLevel = 1.f, Sens = 1.f, PadLookSpeed = 1.f, BaseFov = 70.f;
	bool bInvertY = false, bPadInvertY = false, bLargeText = false, bCalmFlash = false, bStillShots = false, bHeadMotion = true, bAbductOn = true;
	bool bCameraMoments = true;   // N1110: the short snaps of the view in events (FS47CameraMoment; settings "cameraMoments")
	bool bCharacters = true, bSerling = true, bArtBell = true, bRadioCast = true;
	bool bVehicleMirrors = true;   // the Jeep's live mirrors (settings "vehicleMirrors"; -S47Mirrors=0/1 for this run only)
	FString Quality = TEXT("high"), Picture = TEXT("carpenter");
	FS47GraphicsSettings Graphics; // device preferences, separate from case saves
	float PictureStrength = 1.f;   // film/tape blend; zero keeps the selected look for later
	float Brightness = 0.5f;        // the Unreal version's own: the light into the tone curve (0..1, .5 as made)

	// tests (-S47Open=<view>): opening, save, load, settings, card
	bool OpenTestView(const FString& View);

	// self test: the pieces that work without the world
	static bool SelfTest(TArray<FString>& Out);
	virtual void BeginDestroy() override;

private:
	bool bPendingFreshThumb = false, bReuseThumb = false;
	FString LastThumb; int32 LastThumbCase = 0; double LastThumbAt = 0;
	// SAVE-SARO-20261108: the departure from SARO and its autosave
	FS47SaroDeparture Departure;
	const void* DepartureSession = nullptr;
	FString DepartureText, DepartureWaitSaid;
	bool bDepartureShow = false;
	FS47SaroDeparture::FIn DepartureIn(double Dt) const;
	bool DepartureLive() const;
	void StepDeparture(double Dt);
	bool FileDeparture(bool bLastChance);
 FS47Journal Journal;
 bool ValidJournalHint(const FString& Id) const;
 bool ValidJournalEntry(const FString& Id) const;
	UPROPERTY() TObjectPtr<US47Main> M;
	UPROPERTY() TObjectPtr<ACameraActor> TitleCamera;
	UPROPERTY() TObjectPtr<UAudioComponent> Music;
	UPROPERTY() TObjectPtr<USceneCaptureComponent2D> ThumbCapture;
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> ThumbTarget;
	TSharedPtr<class SWidget> TitleEl;
 TSharedPtr<class FS47TitleEraseSound> TitleEraseSound;
 float TitleEraseResume=0;
 void RememberTitleErasure();
 bool CompleteStoryTitle();
 bool CompleteStoryNight(); // STORY steg 6: the new Story's completion (a Story night's end, not a developer's run)
 TSharedPtr<class SS47Ending> StoryEnding; // STORY steg 6: the end cards of a Story night (StoryNightEnd)
 void StoryTitleTest(double Dt);
 int32 StoryTitleTestStage=0;
 double StoryTitleTestT=0;
 int32 TitleEraseStarts=0;

	TSharedPtr<FS47Opening> Opening;
	bool bDeathShown = false;
	bool bSoundUnlocked = false;    // audio.ctx: the first key or click on the title (or a night) has been
	S47J::FObj Store;               // main.ts store, kept in Saved/SaveGames/settings.json

	// main.ts store
	void LoadStore();
	void StoreSet(const TCHAR* K, const TSharedPtr<class FJsonValue>& V);
	void StoreSet(const TCHAR* K, double V);
	void StoreSet(const TCHAR* K, bool V);
	void StoreSet(const TCHAR* K, const FString& V);
	// the settings applied (applyQuality, applyPicture, resize, the levels)
	void ApplyQuality(const FString& Q);
	void ApplyGraphics(const FS47GraphicsSettings& Settings, bool bSave = true);
	void ApplyPicture(const FString& P, bool bChosen = true);
	void ApplyLook();
	void ApplyBrightness();
	FS47PauseOpts PauseStatus(FS47PauseOpts O) const; // the pause menu's status and tonight's card
	void ApplyVolume();
	void MenuMusic();
	void StopMusic(float Fade);
	void TitleCam(double T);
	void TitleView(bool bOn);
	void PlayInput();
	void DoorChanged(const FString& Id, bool bOpen);
	void Activation(bool bActive);
	void RequestTitle(TFunction<void()> Resume);
	FDelegateHandle ActivationHandle;
	FS47NbExtras NotebookExtras() const;
public:
	// The case board reads the same view the map draws (bearings, pins).
	FS47MapView MapView() const;
private:
	FS47MapAt MapAt() const;
	TArray<FS47KitItem> KitItems() const;
	TArray<FS47PersonCard> PeopleItems() const;
	TArray<FString> TownSeen() const;          // what the save that was opened has from Caliche (town)
	void StartStory(const FString& Checkpoint);
	void FadeInSoon(float Secs);
	UWorld* GameWorld() const;
};
