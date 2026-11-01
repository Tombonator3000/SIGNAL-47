#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
// Explicit isolated integration driver. Fixture placement is reported; not human acceptance.
class FS47FreeformTest
{
public:
 explicit FS47FreeformTest(US47Main* Main):M(Main){}
 void Step(double Dt);
private:
 US47Main* M=nullptr;
 bool bInit=false,bDone=false,bAudioMarked=false,bFieldResume=false,bCarRoundTrip=false;
 int32 Stage=0,Checks=0,Restores=0,JobIndex=0,EventIndex=0,DayRuns=0;
 double Age=0,At=0,Hold=0,InitialAz=0,InitialEl=0,PausedAz=0,PausedEl=0;
 FString Root;
 S47J::FObj Snapshot;
 TArray<TSharedPtr<FJsonValue>> Evidence;
 bool Check(bool Good,const FString& Text);
 void Go(int32 Next,double Delay=0);
 bool Restore(const FString& Name);
 bool FileAtBinder();
 void PoseForEvent();
 void Shot(const FString& Name);
 void Finish(bool Good,const FString& Why=FString());
};
