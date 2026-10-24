#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"

class US47Main;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

// Original Caliche Junction inspection front. This local circuit is independent of SARO K3,
// the receiver and STATION 01. No chapter, trip, clock or later T02 event is started here.
class FS47Substation
{
public:
 explicit FS47Substation(US47Main* InM);
 void Begin(const S47J::FObj& Saved = nullptr);
 void Step(double Dt);
 S47J::FObj Save() const;
 static bool SelfTest(TArray<FString>& Lines);
 bool Ready() const { return bReady; }
 static FVector Site(const FVector& Blender); // authored metres, Z up -> world Three
private:
 US47Main* M = nullptr;
 bool bBuilt = false, bReady = false, bJournal = false, bComplete = false, bObserved = false;
 double Running = 0, CompletedAt = 0, ObservedAt = 0;
 FString CompletedStamp;
 FS47Obj Root, Lamp, Lever;
 TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> LampMids;
 TWeakObjectPtr<UPointLightComponent> Light;
 TWeakObjectPtr<UStaticMeshComponent> LeverComp;
 FTransform LeverRest;
 bool bTest = false, bTestDone = false, bDiskTest = false;
 FString TestStamp;
 double TestCompletedAt = 0, TestDiskAt = 0;
 FVector TestPose = FVector::ZeroVector;
 int32 TestStage = 0;
 double TestTime = 0;
 bool Here() const;
 bool Install();
 FS47Obj Target(const FString& Name, const FVector& At, const FVector& Size);
 void ReadJournal();
 void StartCheck();
 void Observe();
 void Apply();
 bool RenderedResult() const;
 void SyncFacts();
 void Commit(const FString& Scene);
 void Refile();
 void TestStep(double Dt);
};
