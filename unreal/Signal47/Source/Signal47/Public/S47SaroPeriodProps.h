#pragma once

#include "CoreMinimal.h"

class US47Main;

// Optional period props retain the original phone/handset/jacket scene nodes,
// gameplay queries, saved state and the unchanged floor collision rectangles.
class FS47SaroPeriodProps
{
public:
 explicit FS47SaroPeriodProps(US47Main* InMain);
 ~FS47SaroPeriodProps();
 void Step(double Dt);
 static void Update(US47Main* Main, double Dt);
private:
 struct FState;
 TUniquePtr<FState> State;
 US47Main* M = nullptr;
 bool bOff = false;
 void TryBuild();
 void TryBuildChairs();
};
