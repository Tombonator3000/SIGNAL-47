#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47HorrorPack.h"
class US47Main;
class FS47HorrorVisual;
class FS47HorrorThreat;
class FS47HorrorSC03;
class FS47RecordedDialogue;
struct FS47Task;
struct FS47AlbumShot;

// Native SC route. The authored pack presents results; physical controllers own proof.
class FS47Horror
{
public:
 // Revision 1 cases end in legacyContinuation after SC01. Revision 2 continues into SC02,
 // revision 3 also into SC03 after SC02's escape.
 static constexpr int32 InstalledRevision = 3;
 explicit FS47Horror(US47Main* InM);
 ~FS47Horror();
 void Begin(const S47J::FObj& Saved, bool bNewCase);
 void Stop();
 void Step(double Dt);
 bool Use(const FString& Id);
 void DoorChanged(const FString& Id, bool bOpen);
 void Photo(const FS47AlbumShot& Shot);
 void TagPhoto(FS47AlbumShot& Shot);
 void CalibrationChecked();
 S47J::FObj Save() const;
 bool Enabled() const { return bEnabled; }
 bool Complete() const { return bComplete; }
 bool NormalComplete() const;
 bool BaselineChecked() const { return bBaseline; }
 bool ReferencePhotographed() const { return !ReferencePhoto.IsEmpty(); }
 bool ReferenceFrameReserved() const { return bEnabled && !bComplete && ReferencePhoto.IsEmpty(); }
 bool ReferenceInPhotoView() const;
 bool DoorEastTried() const { return bEast; }
 bool DoorCorridorTried() const { return bCorridor; }
 bool ReportAcknowledged() const { return bReport; }
 bool DoorsSecured() const;
 bool Busy() const;
 int32 ContentRevision() const { return Revision; }
 bool HasTasks() const; // SC01 until complete, then only SC02's active meeting
 bool SC02Meeting() const;
 bool SC02Escaped() const;
 FString Objective() const;
 TArray<FS47Task> Tasks() const;
 static bool SelfTest(TArray<FString>& Lines);
 FS47HorrorPack Pack;
 TUniquePtr<FS47HorrorVisual> Visual;
 TUniquePtr<FS47HorrorThreat> Threat;
 TUniquePtr<FS47HorrorSC03> SC03;
 bool SC03Live() const;
private:
 enum class ESC02 : uint8 { Off, T01, Encounter, Escaped, Unwitnessed, Legacy };
 struct FSC02Save { ESC02 Phase = ESC02::Off; bool bAttempt = false, bRetry = false, bSeen = false; FString EscapeStamp, Checkpoint; };
 ESC02 SC02 = ESC02::Off;
 bool bH02Attempt = false, bAttemptPending = false, bAttemptShowing = false, bRetry = false, bRetryShowing = false;
 bool bThreatSeen = false, bCheckpointPending = false;
 FString EscapeStamp;
 // The phone transaction in progress: its scene and any scenes the same call still owes.
 FString CallScene = TEXT("SC01_FIRST_REPORT");
 TArray<FString> CallQueue;
 int32 SC03Seen = -1;
 void StartCall(const TArray<FString>& Scenes);
 void StepSC03(double Dt);
 US47Main* M = nullptr;
 TSharedPtr<FS47RecordedDialogue> Voice;
 bool bEnabled = false, bEast = false, bCorridor = false, bBaseline = false;
 bool bReport = false, bComplete = false, bDialogueOpen = false, bReferenceHeard = false;
 bool bWorldCall = false, bVoiceAvailable = false;
 bool bAudibleSoFar = false;
 bool bResumeRequested = false;
 int32 Revision = 0, CallCursor = 0;
 FString Spoken, ReportStamp, ReferencePhoto;
 double VoiceAt = 0, VoiceLength = 0;
 void Context();
 void Changed();
 void Refile();
 bool Trigger(const FString& Id);
 void Telephone();
 void ShowDialogue();
 void AdvanceDialogue();
 void ObserveVoice();
 void FinishCall();
 bool SceneAllowed(const FS47HorrorSceneRegistration* R) const;
 bool SC02Live() const { return SC02 == ESC02::T01 || SC02 == ESC02::Encounter || SC02 == ESC02::Escaped; }
 bool AtServiceFront() const;
 bool AtSite() const;
 void BeginSC02(const S47J::FObj& Saved);
 void StepSC02(double Dt);
 void TriggerSC02();
 void LocalRetry(const TCHAR* Why);
 static const TCHAR* PhaseName(ESC02 Phase);
 static bool ParsePhase(const FString& Text, ESC02& Out);
 static FString ScenarioName(int32 InRevision, bool bSC01Complete, ESC02 Phase);
 static bool AllowsScenario(int32 InRevision, ESC02 Phase, const TArray<FString>& ScenarioIds);
 static ESC02 AfterSC01(int32 InRevision, bool bThreatAvailable);
 static S47J::FObj WriteSC02(const FSC02Save& In);
 static bool ReadSC02(const S47J::FObj& O, FSC02Save& Out);
 static bool SC02SelfTest(TArray<FString>& Lines);
};
