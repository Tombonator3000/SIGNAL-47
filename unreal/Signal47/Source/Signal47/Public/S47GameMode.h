#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "S47GameMode.generated.h"

// SARO, the night of the prologue: the station is built at run time (AS47Saro), the player
// walks (AS47Walker) and the screens are Slate (AS47HUD).
UCLASS()
class SIGNAL47_API AS47GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AS47GameMode();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override { return nullptr; }
};
