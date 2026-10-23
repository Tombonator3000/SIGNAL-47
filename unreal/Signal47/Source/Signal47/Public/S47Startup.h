#pragma once

#include "CoreMinimal.h"

class APlayerController;
class UWorld;
class US47Main;

// Unreal only: a real loading phase, not a delay on the night or its test timers. The world
// renders behind the terminal while async PSOs finish and the first rendered passes settle.
namespace S47Startup
{
	void Initialize();
	void Shutdown();
	void Begin(UWorld* World);
	void Prepare(APlayerController* PC, US47Main* Main, TFunction<void()> OnReady);
	void Tick();
	bool Blocking();
	bool Ready();
	double Elapsed();
	double SecondsSinceReady();
}
