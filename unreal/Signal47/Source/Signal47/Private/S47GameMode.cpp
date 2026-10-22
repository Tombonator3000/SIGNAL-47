#include "S47GameMode.h"
#include "S47HUD.h"
#include "S47Web.h"
#include "S47Story.h"
#include "S47Walker.h"
#include "Engine/World.h"

AS47GameMode::AS47GameMode()
{
	HUDClass = AS47HUD::StaticClass();
	DefaultPawnClass = AS47Walker::StaticClass();
}

void AS47GameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	// The station is built before the player is spawned, so there is a floor to stand on.
	GetWorld()->SpawnActor<AS47Web>();
	GetWorld()->SpawnActor<AS47Story>();
}
