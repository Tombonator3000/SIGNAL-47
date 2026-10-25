#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
#include "S47T02Photo.h"

class US47Main;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;
struct FS47Doc;
struct FS47Task;

// Original Caliche Junction inspection front. This local circuit is independent of SARO K3,
// the receiver and STATION 01. T02 has its own later attempt; the normal T01 record is historical.
class FS47Substation
{
public:
 explicit FS47Substation(US47Main* InM);
 void Begin(const S47J::FObj& Saved = nullptr);
 void Step(double Dt);
 S47J::FObj Save() const;
 static bool SelfTest(TArray<FString>& Lines);
 bool Ready() const { return bReady; }
 bool HasJournal() const { return bJournal; }
 // Only the physical lamp comparison records this result; the narrative host reads it.
 bool HasNormalResult() const { return bJournal && bComplete && bObserved && !CompletedStamp.IsEmpty() && FMath::IsFinite(CompletedAt); }
 const FString& NormalCompletionStamp() const { return CompletedStamp; }
 static FVector Site(const FVector& Blender); // authored metres, Z up -> world Three
 // The camera adapter reads a physical current attempt; it never manufactures control facts.
 bool FollowupAvailable() const;
 bool FollowupCameraContext() const;
 bool ReferenceMatched() const;
 FString FollowupAttemptId() const { return AttemptId; }
 static FVector FollowupReferencePosition();
 static double FollowupReferenceRadius() { return 1.6; }
 TArray<TPair<FString, FVector>> FollowupFeatures() const;
 TArray<FS47Obj> FollowupOccluders() const;
 bool CanCaptureFollowup(FString& Reason) const;
 bool OnFollowupCapture(const FS47T02PhotoRecord& Photo);
 bool FollowupVerified() const;
 bool FollowupVerifiedProof() const { return FollowupVerified(); }
 bool FollowupFiled() const { return bFollowupFiled && ReferenceMatched() && (FiledResolution == TEXT("unresolved") || (FiledResolution == TEXT("verified") && FollowupVerified())); }
 FString FollowupFiledResolution() const { return FollowupFiled() ? FiledResolution : FString(); }
 bool TryFileFollowup();
 S47J::FObj FollowupSnapshot() const;
 FS47Doc FollowupDocument(bool bIncludePhoto = false) const;
 FS47Doc FiledFollowupDocument(bool bIncludePhoto = false) const;
 TArray<FS47Task> FollowupTasks() const;
 FString FollowupObjective() const;
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
 // Separate later cycle. A retry clears every result/photo from that cycle, never T01.
 bool bFollowupBuilt = false, bReferenceArmed = false, bReferenceMatched = false;
 bool bIsolated = false, bBackupExcluded = false, bIndicatorCompared = false, bLampObserved = false, bFollowupFiled = false;
 bool bIsolationResponseSeen = false, bAuxiliaryPulseSeen = false;
 FString AttemptId, BaselineStamp, FiledResolution;
 int32 AttemptNumber = 0, FollowupRunning = 0;
 double AttemptAt = 0, BaselineAt = 0, BaselineObservedAt = 0, ReferenceAt = 0;
 double IsolationAt = 0, BackupAt = 0, IndicatorAt = 0, LampAt = 0, FiledAt = 0;
 double FollowupElapsed = 0, FollowupVisualTime = 0, LampViewSeconds = 0;
 float LampViewMin = TNumericLimits<float>::Max(), LampViewMax = 0;
 TOptional<FS47T02PhotoRecord> FollowupPhoto;
 S47J::FObj ReferenceView;
 TArray<S47J::FObj> FiledHistory;
 FS47Obj FollowupPanel, IsolationResponse, AuxiliaryResponse, ReferenceMarker;
 TArray<FS47Obj> FollowupVisuals; // actual rendered meshes, never hidden Target() interaction proxies
 TWeakObjectPtr<UMaterialInstanceDynamic> PanelMid, IsolationMid, AuxiliaryMid;
 TWeakObjectPtr<UStaticMeshComponent> IsolationHandle, AuxiliaryHandle;
 FTransform IsolationRest, AuxiliaryRest;
 void InstallFollowup();
 FS47Obj FollowupProp(const FString& Name, const FVector& At, const FVector& Size, const FLinearColor& Color, TWeakObjectPtr<UMaterialInstanceDynamic>* Mid = nullptr);
 void BeginFollowup(const S47J::FObj& Saved);
 void StepFollowup(double Dt);
 void ApplyFollowup();
 void SyncFollowupFacts();
 void StartFollowup();
 void StartIsolation();
 void StartAuxiliary();
 void CompareIndicator();
 void ObserveFollowupLamp();
 bool FollowupRecordValid() const;
 bool ReferenceViewValid() const;
 bool FollowupControlsValid() const;
 bool FollowupVisible(const FVector& Point, double MaxDistance = 80) const;
 bool FollowupLampRendered() const;
 float FollowupLampStrength() const;
 static bool FollowupSelfTest(TArray<FString>& Lines);
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
