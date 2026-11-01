#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Rx.h"
class US47Main;
class FS47FreeformDirector;
class FS47FreeformWorld;
struct FS47Task;

// Separate, persistent SARO working life. Story progression never grants its jobs or facts.
class FS47FreeformSession
{
public:
 enum class EJobStage : uint8 { New, Pointed, Integrating, Chart, Recorded, Registered, Packet, Filed };
 struct FJob
 {
  FString Id, Phenomenon;
  int32 Day=1, Slot=0, Source=0;
  EJobStage Stage=EJobStage::New;
  double Elapsed=0, Quality=1, ReceivedLevel=1;
  bool bAnomaly=false,bControl=false,bAudioReviewed=false,bPrinted=false,bPaid=false;
  double ControlAt=0, OriginalCovered=0,ControlCovered=0, MarkStart=-1,MarkEnd=-1;
  double ReceiverClock=0,ReferenceClock=0,ControlReceiverClock=0,ControlReferenceClock=0,DuplicateClock=0,DuplicateInterval=0;
  int32 BeamPoint=0;double BeamSettle=0,BeamSample=0;
  TArray<double> BeamAz,BeamEl,BeamAmplitude;
  bool bBeamVerified=false,bClockMeasured=false,bDuplicateCollected=false;
  FString OriginalPage,OriginalStamp,DuplicatePage,DuplicateStamp;
 };
 explicit FS47FreeformSession(US47Main* Main);
 ~FS47FreeformSession();
 void Begin(const S47J::FObj& Saved,bool bLethal=false);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Active() const { return bActive; }
 bool Held() const;
 bool ArrayMotionHeld() const;
 int32 Day() const { return DayNumber; }
 double Clock() const { return WorldClock; }
 // Daylight 06–19 takes 30 active minutes; night 19–06 takes 45.
 // Pure conversion also handles a step crossing midnight or either phase boundary.
 static double AdvanceClock(double Clock,double ActiveSeconds);
 int32 Budget() const { return Cash; }
 int32 Debt() const { return Loan; }
 bool Lethal() const { return bDeadly; }
 bool Dead() const { return bDead; }
 bool Use(const FString& Id);
 FString Label(const FString& Id) const;
 FString Objective() const;
 TArray<FS47Task> Tasks() const;
 void OpenOperations();
 void OpenTracking();
 void OpenStatus();
 void OpenReceiver();
 void OpenAudio();
 void OpenArchive();
 void OpenServiceGuide();
 bool StartServiceGuide(bool bSupplyTrip);
 bool CompleteServiceGuide();
 FString ServiceObjective() const;
 bool SelectJob(int32 Index);
 bool ChooseThird(bool bVirgo);
 bool PointSelected();
 bool StartAcquisition();
 bool CollectChart();
 bool StartControl();
 bool AudioAction(const FString& Action);
 void SeekAudio(double At);
 bool CompareInstrument();
 bool RegisterComparison(const FString& Conclusion);
 bool FilePacket();
 bool Rest();
 bool Upgrade(int32 Index);
 bool HasUpgrade(int32 Index) const { return Upgrades.IsValidIndex(Index) && Upgrades[Index]; }
 void RecordEvidence(const FString& Id,const FString& Text);
 void RegisterFinding(const FString& Id,const FString& Text) { RecordEvidence(Id,Text); }
 bool HasFinding(const FString& Id) const { return Findings.Contains(Id); }
 int32 FindingCount() const { return Findings.Num(); }
 void Recover(const FString& Why);
 bool Spend(int32 Amount,bool bEmergency=false);
 void Earn(int32 Amount);
 int32 InventoryCount(const FString& Id) const;
 void AddItem(const FString& Id,int32 Count);
 bool Consume(const FString& Id);
 void SetPowerFault(bool bFault);
 bool PowerFault() const { return bPowerFault; }
 void SetAnomaly(bool bValue);
 bool Anomaly() const { return bAnomaly; }
 void SetPhenomenon(const FString& Id) { PhenomenonId=Id; Changed(); }
 const FString& Phenomenon() const { return PhenomenonId; }
 int32 SelectedJob() const { return Selected; }
 const TArray<FJob>& Jobs() const { return Work; }
 const FJob* CurrentJob() const;
 double AudioPosition() const { return AudioCursor; }
 bool AudioPlaying() const { return bAudioPlaying; }
 int32 AudioSource() const { return AudioSourceIndex; }
 double AudioLevel() const;
 FString AudioDisplay() const;
 const FS47Profile* ConsoleProfile() const;
 TSharedPtr<FS47FreeformDirector> Director;
 TSharedPtr<FS47FreeformWorld> Field;
 static bool SelfTest(TArray<FString>& Lines);
 // Isolated developer controls, no player-state mutation outside this session.
 bool TestAdvance(double WorldSeconds);
 bool SpendTime(double WorldSeconds) { return TestAdvance(WorldSeconds); }
private:
 US47Main* M=nullptr;
 bool bActive=false,bDeadly=false,bDead=false,bPowerFault=false,bAnomaly=false;
 bool bTelemetry=false,bAudioOpen=false,bAudioPlaying=false,bRecovering=false;
 bool bPowerApplied=false,bLastPower=true;
 int32 RecoveryEpoch=0;
 int32 ServiceGuide=0,GuideServiceBaseline=0,GuideSupplyBaseline=0;
 bool bGuideSupply=false,bGuideFocus=false;
 int32 DayNumber=1,Cash=120,Loan=0,Selected=0,AudioSourceIndex=0,SelectedDish=0;
 double WorldClock=8*3600,Settle=0,AudioCursor=0,AutosaveAt=0,IntegrationPulse=0;
 int32 CrossPoint=0; double CrossSample=0;
 bool bCrossRunning=false;
 TArray<FJob> Work;
 TMap<FString,FString> Findings;
 TMap<FString,int32> FindingDays;
 TArray<int32> MysterySteps;
 TArray<bool> Upgrades;
 FString Feedback, PhenomenonId;
 TArray<double> SavedAngles;
 bool bRestoreAngles=false;
 FJob* Job();
 void AddDay(int32 Number);
 void Changed(const FString& Text=FString());
 void UpdateMysteries();
 bool PointReady() const;
 bool EncodersAt(double Az,double El) const;
 void PointBeamControl();
 void CollectDuplicate();
 FString ControlSummary(const FJob& J) const;
 bool HasPower() const;
 bool RequirePower();
 bool NearBinder() const;
 double TargetAz() const;
 double TargetEl() const;
 double SourceSeconds() const;
 void PointCross();
 void StepWork(double Dt);
 void StepAudio(double Dt);
 void PrintJob(bool bPacket);
 void SetPanel(TSharedRef<class SWidget> Widget,bool bLive=false,bool bAudio=false);
 FString JobLine(const FJob& J) const;
 FString Reference(const FJob& J) const;
 static FS47Profile Profile(int32 Source);
};
