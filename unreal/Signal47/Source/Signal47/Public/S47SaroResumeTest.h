#pragma once
#include "CoreMinimal.h"

class US47Main;

// Opt-in diagnostic, never ordinary gameplay. Root prepares an isolated /tmp
// profile containing the unchanged original case1 auto1 and starts normal Continue.
class FS47SaroResumeTest
{
public:
 explicit FS47SaroResumeTest(US47Main* Main);
 ~FS47SaroResumeTest();
 void Step(double Dt);
private:
 struct FState;
 TUniquePtr<FState> State;
 US47Main* M=nullptr;
 bool Check(bool Good,const FString& Why);
 void Finish(bool Good,const FString& Why);
 void Write(const FString& Status,const FString& Why);
 void Stage(int32 Next,double Wait=0);
 void AimOps();
 bool UseOps();
 void Shot(const TCHAR* Name);
};
