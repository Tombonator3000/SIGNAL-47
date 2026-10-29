#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;

// Explicit opt-in API/Walker/disk driver. Run only with a fresh /tmp UserDir,
// developer case zero, -S47Jump=chapter2 and -S47DaleDocumentsContract.
class FS47DaleDocumentsTest
{
public:
 explicit FS47DaleDocumentsTest(US47Main* InM);
 void Step(double Dt);
private:
 US47Main* M = nullptr;
 bool bEnabled = false, bDone = false, bLegacyTyped = false;
 int32 Stage = 0, Checks = 0, Stores = 0, ReadIndex = 0, WalkIndex = 0, ScrollKeys = 0, TotalScrollKeys = 0;
 int32 DeskPaperIndex = 0, ArtAudits = 0;
 float PreviousScroll = 0;
 double Elapsed = 0, Hold = 3, LastProgress = 0, WalkMetres = 0, DeskWalkStart = 0;
 FString Root, PendingRestore;
 FVector LastWalk = FVector::ZeroVector;
 TArray<FVector> Points;
 TArray<int32> PaperNodes;
 TArray<FBox> PaperBounds;
 TArray<TSharedPtr<FJsonValue>> PhysicalPapers;
 S47J::FObj ActualRead, PaperReceipt, RestoreReceipt;
 bool Check(bool Good, const FString& Why);
 void Fail(const FString& Why);
 void Next(int32 Value, double Wait = .25);
 bool StoreRestore(const FString& Name, const S47J::FObj& Snapshot);
 bool AuditArt(const FString& Context);
 bool OfficeFacts(const S47J::FObj& Office, int32 Completed, const FString& Context);
 bool FinishRestore();
 void Shot(const FString& Name);
};
