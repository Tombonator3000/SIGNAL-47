#pragma once
#include "CoreMinimal.h"
#include "InputCoreTypes.h"
class APlayerController;

// Unreal-only Xbox layout. Keyboard rebinding remains independent. Raw SDL/Slate input is also
// observed while UI-only panels own focus; a right-stick pointer makes every mouse panel usable.
namespace S47Gamepad
{
 SIGNAL47_API void Init(APlayerController* PC);
 SIGNAL47_API void Shutdown();
 SIGNAL47_API bool UsingGamepad();
 SIGNAL47_API uint32 Changes();
 SIGNAL47_API FKey Key(FName Action);
 SIGNAL47_API FString Label(FName Action);
 SIGNAL47_API bool Down(const FKey& Key);
 SIGNAL47_API bool Pressed(const FKey& Key);
 SIGNAL47_API FVector2D Move(); // x right, y backwards, radial dead zone with proportional travel
 SIGNAL47_API FVector2D Look(double Dt, double Speed, bool bInvertY); // radians, not mouse pixels
 SIGNAL47_API FVector2D Drive(); // x steering; y throttle (RT minus LT)
 SIGNAL47_API FVector2D DeadZone(FVector2D Value, double Zone = .18);
 SIGNAL47_API bool SelfTest(TArray<FString>& Out);
}
