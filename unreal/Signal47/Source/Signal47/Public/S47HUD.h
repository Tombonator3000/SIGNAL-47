#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "S47Docs.h"
#include "S47HUD.generated.h"

class SS47Hud;
class SS47Crt;
class SWidget;
class AS47Story;
class AS47Web;
class AS47Walker;
class US47Audio;
class FWidgetRenderer;

// The night shift at SARO: you walk, you look at things, the story (AS47Story) says what they
// are and what happens. Papers, the receiver console, the operations terminal and the notebook
// open over the game; game time stops while one is open. The four CRTs are drawn here into
// the station's render targets.
// Command line for tests:
//   -S47SmokeTest        log a result line and quit after -S47Seconds (default 3)
//   -S47Face=<spot>      stand at a spot: room, window, outside, dish, or in front of an item (crtCenter, rack, logbook...)
//   -S47Use              use what is in front of you after 2.5 s
//   -S47Jump=<beat>      start at a beat: shift, residual, locked, printed, countdown, event
//   -S47Open=<view>      open a view after 2 s: notebook, console, ops, log, pause, settings[:tab], save, load, doc:<id> (a paper the story holds) ...
//   -S47Press=<k,k,...>  then press these keys (and Click:x:y) half a second apart (S47Keys::TestPresses)
//   -S47Title            start on the title even with other test flags (without any -S47 flag the game starts there)
//   -S47Case=<n>         a test run that plays (and autosaves into) case n
//   -S47SelfTest         run the ported rules' self tests and log "self test OK" or "FAILED"
//   -S47Shot=<file.png>  save a screenshot one second before quitting
UCLASS()
class SIGNAL47_API AS47HUD : public AHUD
{
	GENERATED_BODY()

public:
	AS47HUD();
	virtual void Tick(float DeltaSeconds) override;

	// What the story asks of the screen.
	void Toast(const FString& Text, float Secs = 3.2f);
	void ShowDocument(const FS47Doc& Doc, TFunction<void()> OnClose);
	void OpenConsole();
	void OpenOps();
	void OpenNotebook();
	void Fade(bool bOn, const FString& Text);
	void CloseModal();
	bool HasModal() const { return Modal.IsValid(); }
	TSharedPtr<SWidget> ModalWidget() const { return Modal; }
	void OpenModal(TSharedRef<SWidget> Widget, TFunction<void()> OnClosed) { SetModal(Widget, MoveTemp(OnClosed)); }
	SS47Hud* HudWidget() const { return Hud.Get(); }
	void SetPrompt(const FString& Label, const FString& Alt);
	void Place(double X, double Z, double Yaw); // three.js terms, as Player.place
	void Checkpoint(const FString& Name);
	void Shake(float Strength);
	void EndCard(const TArray<FString>& Lines);
	void LookTowards(const FVector& TargetThree, float K);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	TSharedPtr<SS47Hud> Hud;
	TSharedPtr<SWidget> Modal;
	TFunction<void()> ModalClosed;
	// UI.prompt: what E does (the label) and the second thing (Q); bPromptOwn: set by a module this
	// frame (UI->Prompt), not taken from what the walker looks at
	FString PromptLabel, PromptAlt;
	bool bPromptOwn = false;
	bool bPromptSet = false;
	TWeakObjectPtr<AS47Story> Story;
	TWeakObjectPtr<AS47Web> Saro;
	TWeakObjectPtr<AS47Walker> Walker;
	UPROPERTY() TObjectPtr<US47Audio> Audio;
	FWidgetRenderer* Renderer = nullptr;
	TSharedPtr<SS47Crt> Crts[4];
	float CrtTimer = 0.f;
	FTimerHandle SmokeTimer;
	bool bEnded = false;

	void Start();
	void Use(const FString& Id);
	void SetModal(TSharedPtr<SWidget> Widget, TFunction<void()> OnClosed);
	void DrawCrts(float DeltaSeconds);
	void FinishSmokeTest();
	void OpenView(const FString& View);
	void Face(const FString& Spot);
};
