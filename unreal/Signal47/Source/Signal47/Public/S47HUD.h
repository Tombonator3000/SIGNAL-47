#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "S47HUD.generated.h"

class SS47Hud;
class SS47Terminal;

// The night shift at SARO: you walk, you look at things, and the status terminal opens as a
// full screen when you use it. Esc closes it.
// Command line for tests:
//   -S47SmokeTest        log a result line and quit after -S47Seconds (default 3)
//   -S47Face=<spot>      stand at a spot: room, crt, phone, window, outside, dish
//   -S47Use              use what is in front of you after 2.5 s (the terminal, a door)
//   -S47Shot=<file.png>  save a screenshot one second before quitting
UCLASS()
class SIGNAL47_API AS47HUD : public AHUD
{
	GENERATED_BODY()

public:
	AS47HUD();
	virtual void Tick(float DeltaSeconds) override;

	// The station clock: the prologue starts at 23:41:00.
	double ClockSeconds() const { return 23 * 3600 + 41 * 60 + Elapsed; }
	FString ClockText() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	TSharedPtr<SS47Hud> Hud;
	TSharedPtr<SS47Terminal> Terminal;
	TWeakObjectPtr<class AS47Saro> Saro;
	TWeakObjectPtr<class AS47Walker> Walker;
	FTimerHandle SmokeTimer;
	FText Prompt;
	double Elapsed = 0;
	bool bTerminalOpen = false;

	void Start();
	void Use(const FString& Id);
	void OpenTerminal();
	void CloseTerminal();
	void FinishSmokeTest();
};
