#pragma once

#include "CoreMinimal.h"

class US47Main;

// Native SARO room finishes keep the exported geometry's gameplay and lamp drivers.
class FS47SaroRooms
{
public:
 explicit FS47SaroRooms(US47Main* InM);
 ~FS47SaroRooms();
 void Step(double Dt);
 static void Update(US47Main* Main, double Dt);
private:
 struct FState;
 TUniquePtr<FState> State;
 US47Main* M = nullptr;
 bool bOff = false, bControlOff = false;
 void TryBuild();
 void TryBuildCorridor();
 void TryBuildControlRoom();
};
