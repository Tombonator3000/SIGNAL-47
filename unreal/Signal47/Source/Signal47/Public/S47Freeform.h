#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47CaptureSound.h"
#include "S47Journal.h"
#include "S47Rx.h"
#include "S47FreeformJobs.h"
#include "S47FreeformCases.h"
class US47Main;
class FS47FreeformDirector;
class FS47FreeformWorld;
class FS47CaseFieldTests;
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
  // Background timbre captured at accepted acquisition; evidence ticks stay independent.
  S47CaptureSound::FTrack CaptureOriginal,CaptureControl;
  int32 RecordingSoundVersion=0;
  double RecordingGain=0,RecordingBandwidth=0,RecordingQuality=0;
  double ChartFeed=0, PacketFeed=0, DuplicateFeed=0; // Physical printer seconds; retained across Continue.
  TArray<double> TraceTicks;
  bool bLegacyTrace=false;
  bool bGenerated=false;FS47FreeformJobs::FCard Card;
  bool bAnomaly=false,bControl=false,bAudioReviewed=false,bPrinted=false,bPaid=false;
  double ControlAt=0, OriginalCovered=0,ControlCovered=0, MarkStart=-1,MarkEnd=-1;
  double ReceiverClock=0,ReferenceClock=0,ControlReceiverClock=0,ControlReferenceClock=0,DuplicateClock=0,DuplicateInterval=0;
  int32 BeamPoint=0;double BeamSettle=0,BeamSample=0;
  TArray<double> BeamAz,BeamEl,BeamAmplitude;
  bool bBeamVerified=false,bClockMeasured=false,bDuplicateCollected=false,bDuplicateRejected=false; // rejected legacy copy never becomes evidence
  FString OriginalPage,OriginalStamp,DuplicatePage,DuplicateStamp;
  // Case test job (FS47FreeformCases); -1 for ordinary work.
  int32 CaseLine=-1,CaseTest=-1;
  bool bCaseValid=true,bCaseRecorded=false;
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
 bool MotorFeedbackActive() const { return bMotorFeedback; }
 int32 AvailableDishCount() const;
 static double MeasurementRateForCount(int32 Count);
 double ArrayMeasurementRate() const;
 FString ArrayMeasurementStatus() const;
 int32 CommandFeedbackCount() const { return CommandClicks; }
 int32 ServiceFeedbackCount() const { return ServiceClicks; }
 int32 ActiveServiceGuide() const { return ServiceGuide; }
 bool ServiceGuideFocused() const { return ServiceGuide==1&&bGuideFocus&&FieldFocus<0; }
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
 int32 JournalHintLevel(const FString& Id) const;
 int32 RevealJournalHint(const FString& Id);
 bool JournalEntryRead(const FString& Id) const;
 void MarkJournalEntryRead(const FString& Id);
 FString NextAction() const;
 FString JobNextAction(const FJob& J) const;
 FString JobLocation(const FJob& J) const;
 bool ChargeEmergencyFeederFuse();
 bool CanRegisterPhotoEvidence() const;
 TArray<FS47Task> Tasks() const;
 void OpenOperations();
 void OpenTracking();
 void OpenStatus();
 void OpenReceiver();
 void OpenAudio();
 void OpenTapePanel();
 bool TapePanelOpen() const { return bTapePanel; }
 const FString& FeedbackText() const { return Feedback; }
 bool TerminalOnGlass() const;
 bool TerminalWindowRendered() const;
 int32 ActiveTerminalScreen() const { return ActiveTerminal; }
 int32 OperationsSection() const { return OpsSection; }
 TArray<FString> TerminalLines(int32 Screen) const;
 bool ArrayBlocked() const; // too few antennas to measure at all
 void OpenArchive();
 int32 ArchiveTab=0;FString ArchivePick; // presentation only, never saved
 void OpenCaseBoard(int32 Line=-1);
 bool PlanCaseTest(int32 Line,int32 Test);
 bool FocusFieldTest(int32 Line);
 bool FocusFieldCase(int32 Line);
 bool SetCaseHunch(int32 Line,int32 Cause);
 bool CaseTestWindow(int32 Line,double& Start,double& End) const;
 FString CaseWindowText(int32 Line) const;
 bool CaseWaitTarget(int32 Line,double& Target) const;
 FString WaitConsequences(double Target) const;
 bool WaitUntil(double Target);
 int32 FocusedFieldCase() const { return FieldFocus; }
 FString FieldNextAction(int32 Line) const;
 FString AvailableActivities() const;
 bool StartReferenceCheck();
 bool ConcludeCase(int32 Line,int32 Cause);
 FString TonightPlan() const;
 const FS47FreeformCases& CaseFile() const { return Cases; }
 // Field-work case tests (FS47FreeformCases::ECondition::FieldWork) are carried out in the world.
 // The world asks which test is planned and what the hidden cause makes it show, enacts that
 // physically, then reports what the player actually secured. -1 when no field test is planned.
 int32 PlannedFieldTest(int32 Line) const;
 bool CancelFieldTest(int32 Line);
 bool FieldTestExpectsPresent(int32 Line,int32 Test) const;
 bool ReportFieldTest(int32 Line,int32 Test,bool bValid,bool bPresent,const FString& Evidence);
 void OpenServiceGuide();
 bool StartServiceGuide(bool bSupplyTrip);
 bool CompleteServiceGuide();
 bool CanCompleteServiceGuide() const;
 FString ServiceObjective() const;
 bool SelectJob(int32 Index);
 bool ChooseThird(bool bVirgo);
 bool PointSelected();
 bool StartAcquisition();
 bool CollectChart();
 static constexpr double PrinterSeconds=5.2;
 FString PrinterJobId() const;
 double PrinterProgress() const; // -1 empty, otherwise paper fraction 0..1
 bool PrinterFeedbackActive() const { return bPrinterFeedback; }
 bool StartControl();
 bool AudioAction(const FString& Action);
 void SeekAudio(double At);
 // Marks by a place on the tape, without moving it: first click is MARK IN, second MARK OUT, a third starts over.
 bool MarkAudioAt(double At);
 bool CompareInstrument();
 bool RegisterComparison(const FString& Conclusion);
 bool FilePacket();
 bool Rest();
 bool Upgrade(int32 Index);
 bool HasUpgrade(int32 Index) const { return Upgrades.IsValidIndex(Index) && Upgrades[Index]; }
 void RecordEvidence(const FString& Id,const FString& Text);
 void RegisterFinding(const FString& Id,const FString& Text) { RecordEvidence(Id,Text); }
 bool HasFinding(const FString& Id) const { return Findings.Contains(Id); }
 bool InTutorial() const { return Tutorial>0; }
 void AcceptTutorialJob();
 int32 FindingCount() const { return Findings.Num(); }
 void Recover(const FString& Why);
 bool Spend(int32 Amount,bool bEmergency=false);
 void Earn(int32 Amount);
 int32 InventoryCount(const FString& Id) const;
 void AddItem(const FString& Id,int32 Count);
 bool Consume(const FString& Id);
 void SetPowerFault(bool bFault);
 bool PowerFault() const { return bPowerFault; }
 bool HasPowerForReactiveLight() const { return HasPower(); }
 void SetAnomaly(bool bValue);
 bool Anomaly() const { return bAnomaly; }
 void SetPhenomenon(const FString& Id) { PhenomenonId=Id; Changed(); }
 const FString& Phenomenon() const { return PhenomenonId; }
 int32 SelectedJob() const { return FieldFocus>=0?-1:Selected; }
 const TArray<FJob>& Jobs() const { return Work; }
 const FJob* CurrentJob() const;
 S47J::FObj CaptureSoundState() const; // read-only transport proof for native tests
 double AudioPosition() const { return AudioCursor; }
 bool AudioPlaying() const { return bAudioPlaying; }
 bool AudioPreparing() const { return CaptureStartStage!=0; }
 int32 AudioSource() const { return AudioSourceIndex; }
 double AudioLevel() const;
 FString AudioDisplay() const;
 FString AudioCaptureStatus() const;
 const FS47Profile* ConsoleProfile() const;
 TSharedPtr<class FS47FreeformAudioDesk> AudioDesk;
 TSharedPtr<class FS47RoomDecor> Decor;
 TSharedPtr<class FS47FarmSite> FarmSite;
 TSharedPtr<class FS47Taken> Taken;
 // Gone for this long: the clock moves, and work under way is broken off as after contact. Nothing secured is lost.
 void TakenAway(double WorldSeconds);
 TSharedPtr<class FS47FarmNight> FarmNight;
 TSharedPtr<class FS47FarmHouseDoors> HouseDoors;
 // The yellow marker on the tracking plot: the array can be commanded once it is on the source.
 FVector2D AimAzEl();
 bool AimOnTarget() const;
 void SetAim(FVector2D AzEl);
 void AimAtSource(){SetAim(FVector2D(TargetAz(),TargetEl()));}
 TSharedPtr<FS47FreeformDirector> Director;
 TSharedPtr<FS47FreeformWorld> Field;
 TSharedPtr<FS47CaseFieldTests> CaseField;
 static bool SelfTest(TArray<FString>& Lines);
 // Isolated developer controls, no player-state mutation outside this session.
 bool TestAdvance(double WorldSeconds);
 bool SpendTime(double WorldSeconds) { return TestAdvance(WorldSeconds); }
private:
 FVector2D Aim=FVector2D::ZeroVector;FString AimJob;bool bAimLocked=false;
 TSharedPtr<FS47CapturePlayer> LiveCapture,TapeCapture;
 bool UsesCapturedAudio() const;
 int32 CaptureStartStage=0; // 1 prepare; 2 discard build frame; 3 start on stable frame
 bool QueueCapture(S47CaptureSound::FTrack& Track,double From,double To,double Dt,double Rate,const S47CaptureSound::FFrame& Input);
 void InterruptCapture(const FString& Why);

 FS47Journal Journal;
 bool ValidJournalHint(const FString& Id) const;
 bool ValidJournalEntry(const FString& Id) const;
 US47Main* M=nullptr;
 bool bActive=false,bDeadly=false,bDead=false,bPowerFault=false,bAnomaly=false;
 FString ArchiveNote;
 bool bTelemetry=false,bAudioOpen=false,bAudioPlaying=false,bRecovering=false,bTapePanel=false;
 bool bPowerApplied=false,bLastPower=true;
 bool bMotorFeedback=false,bPrinterFeedback=false;
 int32 PrinterIndex() const;
 static int32 PendingPrint(const FJob& J); // 0 original, 1 packet, 2 duplicate; -1 none
 static double& PrintFeed(FJob& J,int32 Kind);
 static double PrintFeed(const FJob& J,int32 Kind);
 FString PrinterHint() const;
 void StepPrinter(double Dt);
 void StopPrinterFeedback();
 int32 LastServiceFeedback=0,CommandClicks=0,ServiceClicks=0;
 TArray<FVector2D> FeedbackEncoders;
 void StepEquipmentFeedback();
 void StopEquipmentFeedback();
 void CommandFeedback();
 int32 RecoveryEpoch=0,JobSeed=470047,ActiveTerminal=-1;
 TWeakPtr<class SS47ScreenView> TerminalView;
 int32 ServiceGuide=0,GuideServiceBaseline=0,GuideSupplyBaseline=0;
 // First shift tutorial (0 done/off, 1 accept a job at OPS, 2.. follows the job to filing).
 int32 Tutorial=0;FString TutorialJobId;void StepTutorial();FString TutorialObjective()const;
 int32 FieldFocus=-1;
 void ClearCompletedSelection();
 bool bGuideSupply=false,bGuideFocus=false;
 int32 DayNumber=1,Cash=120,Loan=0,Selected=0,AudioSourceIndex=0,SelectedDish=0;
 double WorldClock=8*3600,Settle=0,AudioCursor=0,AutosaveAt=0,IntegrationPulse=0;
 int32 CrossPoint=0; double CrossSample=0;
 bool bCrossRunning=false,bShowEarlierJobs=false;
 TArray<FJob> Work;
 TMap<FString,FString> Findings;
 TMap<FString,int32> FindingDays;
 FS47FreeformCases Cases;
 void OpenCaseFor(const FString& FindingId);
 void RecordCaseResult(FJob& J);
 bool StationSealed() const;
 bool CaseObserved(const FJob& J) const;
 FString CasePrecondition(const FJob& J) const;
 int32 CaseAwaitingChoice() const;
 void IngestObservations();
 int32 IngestedObservations=0;
 int32 OpsSection=0,ReceiverTab=0;
 TSharedRef<class SWidget> OpsShell(int32 Section,TSharedRef<class SWidget> Content);
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
 bool ReceiverReady() const;
 FString ReceiverNextAction() const;
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
 double ReceiverReception() const;
 void PrintJob(bool bPacket);
 void SetPanel(TSharedRef<class SWidget> Widget,bool bLive=false,bool bAudio=false);
 void OpenTerminal(int32 Screen,const FString& Title,TSharedRef<SWidget> Body,bool bLive=false);
 void OpenWaitConfirmation(double Target);
 FS47Profile EffectiveProfile(const FJob& Job) const;
 FString JobLine(const FJob& J) const;
 FString Reference(const FJob& J) const;
 static FS47Profile Profile(int32 Source);
};
