#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class FS47SC08;

// Consumes actual SC07 PASS + hash-bound sc07-complete.disk.json. No placement fixture.
// -S47Case=0 -S47EveningTimeline -S47Jump=chapter2 -S47SpineContract=sc08|sc08long|sc08retry
// -S47SC07Checkpoint=<absolute checkpoint> and a fresh isolated /tmp UserDir.
class FS47SC08Contract
{
public:
 static void Step(US47Main* Main,double Dt);
private:
 explicit FS47SC08Contract(US47Main* Main):M(Main){}
 US47Main* M=nullptr;
 FString Mode,Root,PriorPath,Pending,ChainId;
 int32 Stage=0,Checks=0,Uses=0,Stores=0,Cursor=0,WalkSamples=0,CaptureBefore=0;
 double Began=0,Mark=0,Wait=0,Walked=0,OdoStart=0,OdoOut=0,ClockBeforeCapture=0;
 bool Started=false,Done=false,NormalContinue=false,BodyWitness=false,CoverWitness=false,SearchRead=false,GrabNegative=false;
 FVector Aim=FVector::ZeroVector,LastWalk=FVector::ZeroVector;
 TArray<FVector2D> Route;
 TArray<FString> Pictures;
 TArray<TSharedPtr<FJsonValue>> Executed,Rays,States,Restores,Photos,Negatives;
 TMap<FString,FString> Papers;
 S47J::FObj PriorReceipt,Expected,FirstNative,SecondNative;
 FS47SC08* Act() const;
 void Tick(double Dt);
 bool Check(bool Good,const FString& Why);
 void Finish(bool Good,const FString& Why);
 void Next(int32 Value,double Delay=.2);
 bool Clear(const FVector2D& From,const FVector2D& To) const;
 bool Plan(const FVector2D& Goal,int32 NextStage);
 void Path(TArray<FVector2D> Points,int32 NextStage);
 bool Walk(bool Run=true);
 bool Approach(const FString& Id,int32 NextStage);
 void FacePoint(const FVector& Three);
 bool Face(const FString& Id);
 bool Use(const FString& Id);
 bool Documents(bool Remember=false);
 bool Store(const FString& Name,bool Final=false);
 bool Restored();
 bool RenderBody();
 void ObserveCover();
 void Snapshot(const FString& Name);
 void Shot(const FString& Name);
};
