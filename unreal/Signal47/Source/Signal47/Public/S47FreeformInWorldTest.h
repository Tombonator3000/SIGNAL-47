#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;

// Short isolated CRT/pump production-callback contract; approaches are explicit fixtures.
class FS47FreeformInWorldTest
{
public:
 explicit FS47FreeformInWorldTest(US47Main* Main):M(Main){}
 void Step(double Dt);
private:
 US47Main* M=nullptr;
 bool bInit=false,bDone=false,bMarkedIn=false,bMarkedOut=false;
 int32 Stage=0,Checks=0,Restores=0,ScreenIndex=0,CashBefore=0,DayBefore=0;
 double Age=0,At=0,ClockBefore=0,FuelBefore=0,VolumeBefore=0,ProgressBefore=0;
 FString Root,CardsBefore;
 TArray<TSharedPtr<FJsonValue>> Evidence;
 bool Check(bool Good,const FString& Text);
 void Go(int32 Next);
 bool OpenScreen(int32 Index);
 void AimAt(const FVector& Point);
 void ParkAtPump();
 bool UseAimedAudio(const FString& Id);
 bool Restore(const FString& Name);
 FString Cards(int32 Day) const;
 void CaptureCards(const FString& Phase);
 void Finish(bool Good,const FString& Why=FString());
};
