#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;

// Isolated callback/physics journey contract. Long-distance placement is explicit.
class FS47FreeformJourneyTest
{
public:
 explicit FS47FreeformJourneyTest(US47Main* Main):M(Main){}
 void Step(double Dt);
private:
 US47Main* M=nullptr;
 bool bInit=false,bDone=false,bSawOutward=false;
 int32 Stage=0,Checks=0,Restores=0,DeliveryBefore=0,ServiceBefore=0;
 double Age=0,At=0,InitialAz=0;
 FVector Before=FVector::ZeroVector;
 FString Root;
 TArray<TSharedPtr<FJsonValue>> Evidence;
 bool Check(bool Good,const FString& Text);
 void Go(int32 Next);
 void PlaceAt(const FString& Id,double Offset=1.4);
 bool Restore(const FString& Name,bool bLegacy=false);
 void Finish(bool Good,const FString& Why=FString());
};
