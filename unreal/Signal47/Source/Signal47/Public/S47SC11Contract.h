#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"

class US47Main;
class FS47SC11;

// Opt-in sc11 / sc11long / sc11retry. The static owner survives a full Flow restore.
class FS47SC11Contract
{
public:
 static void Step(US47Main* Main, double Dt);
private:
 explicit FS47SC11Contract(US47Main* Main) : M(Main) {}
 US47Main* M = nullptr;
 FString Mode, Root, Pending;
 int32 Stage = 0, Checks = 0, Uses = 0, Stores = 0, WalkSamples = 0, Cursor = 0, CapturesBefore = 0;
 double Began = 0, Hold = 0, Mark = 0, Walked = 0, CaptureClock = 0;
 bool Started = false, Done = false, NormalContinue = false, RenderWitness = false;
 FVector Aim = FVector::ZeroVector, LastWalk = FVector::ZeroVector;
 TArray<FVector2D> Route;
 TArray<FString> Pictures;
 TArray<TSharedPtr<FJsonValue>> Executed, Rays, States, Restores, Photos, Sight;
 TMap<FString,FString> OriginalDocs;
 TMap<FString,FString> SourcePages;
 TMap<FString,int32> OriginalNodes;
 S47J::FObj Expected;
 FS47SC11* Act() const;
 void Tick(double Dt);
 void Finish(bool Good, const FString& Why);
 bool Check(bool Good, const FString& Why);
 void Next(int32 Value, double Wait = .25);
 void Path(TArray<FVector2D> Points);
 bool Walk(bool Run = false);
 void FacePoint(const FVector& Three);
 bool Face(const FString& Id);
 bool Use(const FString& Id);
 bool Document(const FString& Id, const FString& Fragment);
 bool Sources(const FString& Context);
 void Snapshot(const FString& Name);
 bool Restore(const FString& Name);
 bool Restored();
 bool SightChecks();
 bool RenderChecks();
 void EscapeRoute();
 void Shot(const FString& Name);
};
