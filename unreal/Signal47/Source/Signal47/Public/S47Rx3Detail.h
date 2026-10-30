#pragma once

#include "CoreMinimal.h"

class US47Main;

// Passive RX3 hardware retains the live rack's LEDs, lever and service record.
class FS47Rx3Detail
{
public:
 explicit FS47Rx3Detail(US47Main* InM);
 ~FS47Rx3Detail();
 static void Update(US47Main* Main, double Dt);
 void Step(double Dt);
private:
 struct FState;
 TUniquePtr<FState> State;
 US47Main* M = nullptr;
 bool bOff = false;
 void TryBuild();
};
