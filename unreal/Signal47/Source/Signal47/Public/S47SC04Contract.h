#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class FS47SC04;
struct FHitResult;

// -S47Case=0 -S47EveningTimeline -S47Jump=residual -S47SpineContract=sc04|sc04hide|sc04long.
// Or no Jump + -S47SC03Checkpoint=<actual same-night SC03 PASS>/sc03-complete.disk.json.
// Explicit fixture stays separate from the actual case1/disk/Flow/title consumer.
class FS47SC04Contract
{
public:
 static void Step(US47Main* Main,double Dt);
private:
 explicit FS47SC04Contract(US47Main* Main):M(Main){}
 US47Main* M=nullptr;
 FString Mode,Root,Pending,InputFile,InputResultFile,InputHash,InputResultHash,Chain;
 bool ActualInput=false;
 S47J::FObj InputState,InputResult;
 int32 Stage=0,Checks=0,Uses=0,Stores=0,Cursor=0,WalkSamples=0,CaptureBefore=0;
 double Began=0,Mark=0,Hold=0,Walked=0,GrabClock=0,GrabAt=0,DoorAt=0,VisibilityAt=0;
 bool Started=false,Done=false,NormalContinue=false,RenderWitness=false,DoorNegative=false,GrabNegative=false,HeardOutside=false,SawListen=false,RecordsRouteSeen=false;
 FVector Aim=FVector::ZeroVector,LastWalk=FVector::ZeroVector;
 TArray<FVector2D> Route;
 TArray<FString> Pictures;
 TArray<TSharedPtr<FJsonValue>> Executed,Rays,States,Restores,Photos,Negatives;
 TMap<FString,FString> Documents;
 TMap<FString,int32> OriginalNodes;
 S47J::FObj Expected;
 const FS47SC04* Act() const;
 void Tick(double Dt);
 void Finish(bool Good,const FString& Why);
 bool Check(bool Good,const FString& Why);
 void Next(int32 Value,double Wait=.2);
 void Path(TArray<FVector2D> Points);
 bool Walk(bool Run=false);
 void FacePoint(const FVector& Three);
 bool Face(const FString& Id);
 bool Use(const FString& Id);
 bool Sources(const FString& Context,bool RememberNew=false);
 void Snapshot(const FString& Name);
 bool Restore(const FString& Name);
 bool Restored();
 bool BodySight(FHitResult& Hit,TArray<FString>& Ignored) const;
 bool WaitForBody(const FString& Context);
 bool RenderChecks();
 bool Input();
 bool Approach(const FString& Id);
 bool Fixture();
 void Shot(const FString& Name);
};
