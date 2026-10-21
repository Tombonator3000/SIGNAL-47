#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "S47BoardRules.h"
#include "UObject/Object.h"
#include "S47Main.generated.h"

class AS47Web;
class AS47Story;
class AS47HUD;
class AS47Walker;
class US47Audio;
class US47Inter;
class US47UI;

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

	// The case board (story/caseBoard.ts): every chapter's rule book, and the hook the chapters
	// open and refresh it with. Unset Board.Open: no board yet; a chapter then falls back to the
	// panels it had before the board (ui/Panels.ts referenceFile, the conclusion buttons).
	TArray<class FS47RuleBook*> Books;
	FS47BoardHook Board;

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
	bool ModalOpen() const;
};
