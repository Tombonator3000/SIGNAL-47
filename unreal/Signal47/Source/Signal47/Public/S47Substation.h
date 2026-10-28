#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
#include "S47T02Photo.h"

class US47Main;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UInstancedStaticMeshComponent;
class UNiagaraComponent;
class UPointLightComponent;
class USceneComponent;
class UStaticMesh;
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
 // SC02 arc at the bus-support insulators (S47SubstationArc.cpp, unreal/NIAGARA_FORSLAG.md). Runtime only, never saved.
 struct FArcGate { bool bMeeting = false, bThreatActive = false, bAppeared = false, bWasAppeared = false, bSeen = false, bNormalResult = false, bChecking = false, bFollowup = false, bSpent = false; };
 static constexpr double ArcDuration = 1.2;
 static constexpr double ArcTailDuration = 1.3;
 static bool ArcAllowed(const FArcGate& G);
 static float ArcLevel(double T, bool bCalm); // 0..1 flash envelope, deterministic
 static float ArcDip(double T, bool bCalm);   // marked-lamp factor, exactly 1 outside the arc
 static int32 ArcSparks(bool bLow, bool bCalm);
 bool ArcActive() const { return ArcT >= 0 && ArcT < ArcDuration; }
 int32 ArcCount() const { return ArcFired; }
 float ArcLumens() const;
 bool ArcSafe() const;
 struct FArcSnapshot
 {
  bool bReady = false, bNiagaraActive = false, bNiagaraPaused = false, bMeshVisible = false, bSafe = false;
  bool bFlashVisible = false, bMaterialAssigned = false;
  int32 Instances = 0, Bursts = 0, SparkCount = 0;
  double Age = -1, NiagaraAge = -1;
  float FlashLumens = 0, MaterialIntensity = 0, LampLumens = 0;
  FLinearColor MaterialColor = FLinearColor::Black, LampEmissive = FLinearColor::Black;
  FVector Origin = FVector::ZeroVector;
 };
 FArcSnapshot ArcSnapshot() const; // measured live component state, not expected envelope values
 // The sustained feeder fault is separate from the short SC02 strike and the inspection lamp.
 // Its state derives from Story's saved outage; it never creates a spark burst on Continue.
 static float FaultLampLevel(double T, bool bCalm);
 struct FFaultSnapshot
 {
  bool bRequested = false, bReady = false, bActive = false, bArcVisible = false, bLampsVisible = false;
  bool bMaterialAssigned = false, bSafe = false;
  int32 ArcInstances = 0, LampInstances = 0;
  double Age = 0;
  float ArcLumens = 0, LampLumens = 0, ArcIntensity = 0, LampIntensity = 0;
  FVector Origin = FVector::ZeroVector;
 };
 FFaultSnapshot FaultSnapshot() const;
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
 TWeakObjectPtr<UNiagaraComponent> Sparks;
 TWeakObjectPtr<UPointLightComponent> ArcLight;
 TWeakObjectPtr<UInstancedStaticMeshComponent> ArcMesh;
 TWeakObjectPtr<UMaterialInstanceDynamic> ArcMid;
 TWeakObjectPtr<UInstancedStaticMeshComponent> FaultArcMesh, FaultLampMesh;
 TWeakObjectPtr<UStaticMeshComponent> FaultLampMounts[2];
 TWeakObjectPtr<UMaterialInstanceDynamic> FaultArcMid, FaultLampMid;
 TWeakObjectPtr<UPointLightComponent> FaultArcLight, FaultLampLights[2];
 double FaultTime = 0;
 bool bFaultShown = false;
 double ArcT = -1;
 bool bArcSpent = false, bArcCalm = false, bArcWasAppeared = false;
 int32 ArcFired = 0, ArcSite = 1, ArcSparkCount = 0, ArcShape = -1;
 float LampDip() const { return ArcReady() && AttemptId.IsEmpty() ? ArcDip(ArcT, bArcCalm) : 1.f; }
 bool ArcReady() const;
 void CancelArc();
 void ShapeArc(int32 Shape);
 void InstallArc();
 void ResetArc();
 void FireArc(int32 At, const TCHAR* Why);
 void StepArc(double Dt);
 bool FaultReady() const;
 void InstallFault(USceneComponent* Parent, UStaticMesh* Cylinder, UMaterialInterface* Material);
 void ClearFault();
 void StepFault(double Dt);
 static bool ArcSelfTest(TArray<FString>& Lines);
};
