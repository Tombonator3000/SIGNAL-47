#pragma once
#include "CoreMinimal.h"
#include "S47PaleBody.h"

class US47Main;

// The fog density a developer event holds over the scene's own (S47Lighting), or below zero: none.
extern SIGNAL47_API float GS47DevFogDensity;

// The in-game developer events (Tom 29 October 2026): F10 in a night opens a list of things to
// make happen now, for trying them where the player stands. Any system adds its own with Add
// (aliens, sky, weather, light ...); the list is built when the menu opens, in the order added.
// Nothing here is saved, and nothing is offered on the title.
class SIGNAL47_API FS47DevEvents
{
public:
 struct FEvent { FString Group, Label, Note; TFunction<void()> Fn; };
 explicit FS47DevEvents(US47Main* InM);
 void Add(const FString& Group, const FString& Label, const FString& Note, TFunction<void()> Fn);
 void Open();
 void Step(double Dt);
 void Stop();
 void ControlTest(double Dt);
 const TArray<FEvent>& Events() const { return List; }
private:
 US47Main* M = nullptr;
 TArray<FEvent> List;
 double ShowerLeft = 0, ShowerNext = 0;
 FS47PaleBody Pale;
 double PaleLeft = 0, PaleClock = 0;
 FVector PaleAt = FVector::ZeroVector;
 double PaleYaw = 0;
 FString TestDir, ShotName; double TestT = 0, ShotAt = 0; int32 TestStep = 0; bool bTestInit = false, bTestDone = false;
 int32 RoswellNumber=1;
 void SpawnRoswell(int32 Mode,bool bRoof=false);
 void AddBuiltIn();
 void SpawnPale();
 void SpawnCraft(bool bSaucer);
 TWeakObjectPtr<class UStaticMeshComponent> CraftPreview;
};
