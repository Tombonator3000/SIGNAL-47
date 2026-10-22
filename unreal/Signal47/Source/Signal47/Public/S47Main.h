#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "S47BoardRules.h"
#include "S47Seen.h"
#include "UObject/Object.h"
#include "S47Main.generated.h"

class AS47Web;
class AS47Story;
class AS47HUD;
class AS47Walker;
class US47Audio;
class US47Inter;
class US47UI;
class US47FieldCamera;

// What the web version's main.ts holds in its boot() closure and hands to every module as its
// deps: the world, the story host (the prologue, as Prologue.ts is the host of the chapters),
// the screen, the sound, the things that can be used and the player. A ported module keeps a
// pointer to this and reaches the rest through it (M->Web, M->UI, M->Audio ...), so it reads
// as its TypeScript did (this.d.ui.toast(...) is M->UI->Toast(...)).
//
// The systems main.ts builds and steps one after another (the night's curve, the radio, the
// album, the saves ...) are added here as they are ported, with their step in Step().
UCLASS()
class SIGNAL47_API US47Main : public UObject
{
	GENERATED_BODY()

public:
	static US47Main* Get(const UObject* WorldContext);

	UPROPERTY() TObjectPtr<AS47Web> Web;
	UPROPERTY() TObjectPtr<AS47Story> Story;  // Prologue.ts: the host of the chapters
	UPROPERTY() TObjectPtr<AS47HUD> Hud;
	UPROPERTY() TObjectPtr<AS47Walker> Player;
	UPROPERTY() TObjectPtr<US47Audio> Audio;
	UPROPERTY() TObjectPtr<US47Inter> Inter;
	UPROPERTY() TObjectPtr<US47UI> UI;
	UPROPERTY() TObjectPtr<class US47World> World;   // world/World.ts: areas and trips
	// main.ts's flow: the title, the saves, the pause menu, the notebook (S47Flow.h)
	UPROPERTY() TObjectPtr<class US47Flow> Flow;
	// main.ts requestAutosave: progress asks for an autosave (milestone: a new generation, as a
	// chapter or an area begins); it is written after the frame, once the game may be saved
	void RequestAutosave(bool bMilestone = false);
	UPROPERTY() TObjectPtr<US47FieldCamera> FieldCamera; // core/FieldCamera.ts (made with chapter one)
	UPROPERTY() TObjectPtr<class US47CaseBoard> CaseBoard;

	// The case board (story/caseBoard.ts): every chapter's rule book, and the hook the chapters
	// open and refresh it with. Unset Board.Open: no board yet; a chapter then falls back to the
	// panels it had before the board (ui/Panels.ts referenceFile, the conclusion buttons).
	TArray<class FS47RuleBook*> Books;
	FS47BoardHook Board;

	// The night's layer (S47Night.h): the seen log, the curve, the night's events, the calls, the
	// glimpses, the flashlight and the album, with main.ts's saw(), seenStep() and curveStep().
	UPROPERTY() TObjectPtr<class US47Night> Night;
	// SARO's own happenings (S47SaroNight.h): SARO dark and Hondo's telex, the array put back, the
	// dishes that follow, the lights on B, the reels, the recording's walk
	UPROPERTY() TObjectPtr<class US47SaroNight> SaroNight;
	// main.ts townSeen: what the night has had, by key; a key added fresh is written in the seen log
	FS47TownSeen TownSeen;
	// main.ts oneAtATime(): one of the night's things is going, so nothing else of the night may start
	bool OneAtATime() const;
	// The phones, the archive and the records (S47Phones.h): the archive's phone and the dishes it tips,
	// the motel office's telephone, the steps on the roof, the field book, the thread nobody tied, the looks.
	UPROPERTY() TObjectPtr<class US47Phones> Phones;
	// The time away and the trucks (S47Away.h): the abduction, taken in the truck, the truck on B, the lost
	// nights, the third one, no stars at 026, FLASH's frames.
	UPROPERTY() TObjectPtr<class US47Away> Away;
	// main.ts zoomK: the closer look, 0 to 1 (right mouse held)
	double ZoomK = 0;

	// main.ts: mode and the flags that hold the game
	enum class EMode : uint8 { Title, Play, End };
	EMode Mode = EMode::Play;
	bool bPausedByMenu = false;
	double T = 0;        // seconds since start (main.ts t)
	double Playtime = 0;

	// main.ts input.onKey: a key the game answers while walking (no panel open). A module adds
	// its keys here (C raises the camera, B opens the board, ...); the first handler that returns
	// true takes the press. bEvenModal: also while a panel is open (Esc, Tab).
	void OnKey(const FKey& Key, TFunction<bool()> Fn, bool bEvenModal = false) { Keys.Add({ Key, MoveTemp(Fn), bEvenModal }); }
	struct FKeyHandler { FKey Key; TFunction<bool()> Fn; bool bEvenModal = false; };
	TArray<FKeyHandler> Keys;

	// What main.ts's step() does each frame after the player moved (the story is stepped by its
	// own actor; the systems ported from main.ts go here in main.ts's order).
	void Step(double Dt);
	// main.ts: the airwaves, the two receivers, the programme on the way and the place's score.
	void InitRadio();
	void RadioStep(double Dt);
	void TurnRadio();
	void ResetRadio(double Clock);
	TSharedPtr<class FS47Radio> Radio;
	TSharedPtr<class FS47Receiver> CarRadio, DinerRadio;
	TSharedPtr<class FS47TripRadio> TripRadio;
	TSharedPtr<class FS47Score> Score;
	bool bRadioHeard = false, bCarNear = false;
	FVector RadioAt = FVector::ZeroVector, CounterAt = FVector::ZeroVector;
	FString RadioTest; // -S47Radio=<station>: an audible receiver beside the test camera
	bool bRadioTestLogged = false;
	FString Space; // the acoustic space the sound is in: 'room' | 'yard' | 'lab' (main.ts)
	bool ModalOpen() const;
};
