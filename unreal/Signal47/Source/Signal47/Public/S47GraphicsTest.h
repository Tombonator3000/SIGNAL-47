#pragma once

#include "CoreMinimal.h"

class US47Main;

// Opt-in graphics settings contract. Writes only inside an explicit isolated /tmp UserDir.
// Run write then read as two processes with the same profile to verify actual settings persistence.
class FS47GraphicsTest
{
public:
 explicit FS47GraphicsTest(US47Main* InM);
 ~FS47GraphicsTest();
 void Step(double Dt);
private:
 struct FState;
 TUniquePtr<FState> State;
 US47Main* M = nullptr;
 bool Check(bool bGood, const FString& What);
 void Finish(bool bSuccess, const FString& Reason = FString());
 void Stage(int32 Next, double Hold = .5);
 bool VerifyRenderer(bool bVeryLow);
 void Shot(const TCHAR* Name);
};
