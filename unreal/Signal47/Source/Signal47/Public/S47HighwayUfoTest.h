#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;

// Opt-in native route probe. Requires an isolated /tmp profile and case 0.
class FS47HighwayUfoTest
{
public:
 explicit FS47HighwayUfoTest(US47Main* Main);
 ~FS47HighwayUfoTest();
 void Step(double Dt);
private:
 US47Main* M=nullptr;
 bool bEnabled=false,bDone=false,bFollow=false,bKeepDriving=false,bPauseChecked=false,bRestored=false,bPassShot=false,bStopChecked=false;
 int32 Stage=0,Checks=0,Restores=0;
 double Started=0,At=0,PausedAge=0,StoppedCarZ=0,PlayerStartZ=0,LastZ=0,Travel=0;
 FVector PausePlayer=FVector::ZeroVector,PauseUfo=FVector::ZeroVector,PauseCar=FVector::ZeroVector;
 FString Root;
 TArray<TSharedPtr<FJsonValue>> Evidence;
 bool Check(bool Good,const FString& Text);
 void Finish(bool Good,const FString& Why);
 void Shot(const TCHAR* Name);
 bool Persist();
};
