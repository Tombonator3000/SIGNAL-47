#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47HorrorPack.h"
class US47Main;
class FS47HorrorVisual;
class FS47RecordedDialogue;
struct FS47Task;
struct FS47AlbumShot;

// Native SC route. The authored pack presents results; physical controllers own proof.
class FS47Horror
{
public:
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
 FString Objective() const;
 TArray<FS47Task> Tasks() const;
 static bool SelfTest(TArray<FString>& Lines);
 FS47HorrorPack Pack;
 TUniquePtr<FS47HorrorVisual> Visual;
private:
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
};
