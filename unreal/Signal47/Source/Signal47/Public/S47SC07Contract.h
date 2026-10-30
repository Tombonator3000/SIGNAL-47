#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class US47Chapter2;

// Opt-in physical continuation from a real SC06 PASS/checkpoint, never an invented predecessor.
class FS47SC07Contract
{
public:
 static void Step(US47Main* Main,double Dt);
private:
 explicit FS47SC07Contract(US47Main* InMain):M(InMain){}
 US47Main* M=nullptr;
 FString Root,PriorPath,Pending;
 bool Enabled=false,Initialized=false,Done=false,TitleContinued=false;
 int32 Stage=0,Checks=0,Stores=0,Uses=0,Cursor=0,Samples=0;
 double Began=0,Wait=0,Progress=0,Metres=0;
 FVector Last=FVector::ZeroVector,Aim=FVector::ZeroVector;
 TArray<FVector2D> Route;
 TArray<TSharedPtr<FJsonValue>> Executed,Rays,Restores,Photos;
 TMap<FString,FString> Papers;
 S47J::FObj Expected,PriorReceipt;
 US47Chapter2* Act() const;
 void Tick(double Dt);
 bool Check(bool Good,const FString& Why);
 void Finish(bool Good,const FString& Why);
 void Next(int32 Value,double Delay=.2);
 void Path(TArray<FVector2D> Points,int32 NextStage);
 bool Walk();
 void FacePoint(const FVector& Three);
 bool Face(const FString& Id);
 bool Use(const FString& Id);
 bool Documents(bool Remember=false);
 bool Store(const FString& Name,bool Final=false);
 bool Restored();
};
