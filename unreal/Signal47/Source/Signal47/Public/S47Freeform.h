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
  bool bDocked=false; // the day the player was taken: this card pays half (TakenCosts)
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
 // The shift's date. New games start on Monday 28 April 1986; a save from before that keeps the calendar it began on (OldCalendar),
 // so the date never moves under a game in progress. The field "calendar" says which: an older save does not have it.
 FDateTime Date() const { return DateOfDay(DayNumber); }
 FDateTime DateOfDay(int32 ShiftDay) const;
 bool OldCalendar() const { return bOldCalendar; }
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
 bool EveningGated(const FJob& Job) const;
 FString EveningLine(const FJob& Job) const;
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
 // One line for screen 0 to 3 from the HUD's objective: "NEXT / USE THIS PC", "NEXT / <place>", or empty if it will not fit.
 static FString TerminalNextLine(const FString& Text,int32 Screen);
 FString MeasurementRecord() const;
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
 // A long wait that crosses two in the morning, on a night when something is due, stops between 02:05 and 03:00 and
 // something wakes the player. Once a night: the next rest goes straight through. Developer runs sleep through it
 // (their fixtures count on Rest reaching the morning) unless a test asks.
 void TestAllowWake(){bTestWake=true;}
 int32 LastRousedNight() const{return RousedNight;}
 double LastWakeClock() const{return WakeClock;}
 int32 FocusedFieldCase() const { return FieldFocus; }
 FString FieldNextAction(int32 Line) const;
 FString AvailableActivities() const;
 // The next branch of a worn array, on one line for the HUD; empty once it is whole, or in a save without wear.
 FString StationGoalLine() const;
 // From day two the day's third card is a real evening's work: its source is not up until about half an hour before the
 // card's suggested time. Nothing else waits on it, it never expires, and OPS can wait for it. Developer runs leave it
 // open (their fixtures run all three cards in one sitting) unless a test asks.
 static constexpr double EveningLead=30*60.,EveningOpens=19*3600.,EveningCloses=23.5*3600.;
 // The clock of day this card's source comes up: never before seven in the evening.
 static double EveningUp(const FJob& Job){return FMath::Max(EveningOpens,Job.Card.SuggestedClock-EveningLead);}
 // The soonest of the cards waiting for the evening, and the next time it comes up; null if none is.
 const FJob* NextEveningCard(double& Up) const;
 // The rules a real game plays by. Developer fixtures keep the fixed ones unless a test asks (TestLiveRules).
 bool RulesLive() const{return bTestRules||(M&&LiveFlow());}
 void TestLiveRules(){bTestRules=true;}
 // Whose night a clock falls in: the shift day it belongs to. Bodies keep to their own start each night (S47NightRhythm.h).
 static int32 NightOf(double AtClock){return FMath::Max(1,int32(FMath::FloorToDouble((AtClock-8*3600.)/SecondsInDay))+1);}
 double BodyStartHour(int32 Night) const;
 bool QuietNight(int32 Night) const;
 bool TheirHoursNow(double AtClock) const;
 // Past the night's start, calm or not: a call can come on a calm night too. Leads nowhere then.
 bool NightStarted(double AtClock) const;
 int32 TimesTaken() const{return TakenCount;}
 bool FaultDue() const;
 // Time has a price (Tom, 7 November), low enough not to rush anyone. A card pays in full on its day and the next, then less.
 // Waiting costs a little in standby. Each week the station's review takes power and fuel. Nothing here can lock the game:
 // what cannot be paid is waived (standby) or carried, capped, to the next review, and never touches the requisition.
 static int32 PayAfterDays(int32 Days){return Days<=1?20:Days==2?15:Days==3?10:5;}
 int32 JobPay(const FJob& Job) const;
 static constexpr int32 ReviewPower=25,ReviewFuel=15,ArrearsCap=80,LostHoursDiscount=15;
 // The running costs on the weekly sheet besides power and stores (critic, 8 November; Claude's numbers, kept low): truck wear per
 // two kilometres driven, film and darkroom chemicals per print developed, and Ward's re-check of a recording nobody can read.
 static constexpr int32 WearPerTwoKm=1,WearCap=15,FilmPerPrint=2,FilmCap=10,PoorRecordFee=3,FeeCap=15;
 static constexpr double PoorRecordQuality=.3;
 struct FWeekCosts{int32 Wear=0,Film=0,Fee=0,Poor=0,Prints=0;double Km=0;int32 Total() const{return Wear+Film+Fee;}};
 // For the review that falls at the end of this week (1-based): what it will charge besides power and stores.
 FWeekCosts WeekCosts(int32 Week) const;
 static int32 StandbyFor(double WaitSeconds){return FMath::Clamp(FMath::CeilToInt(WaitSeconds/(3*3600.)),0,4);}
 int32 StandbyPaid() const{return StandbyTotal;}
 int32 Arrears() const{return Carried;}
 int32 NextReviewDay() const{return (ReviewWeek+1)*7+1;}
 FString NextReviewText() const;
 void StepWeeklyReview();
 // Being taken costs (Tom, 7 November): cards docked, windows gone by, a case of its own. Secured work, money earned, film and originals stay.
 FString TakenCosts(double SafeClock);
 void LostTimeTrace(int32 Clue);
 bool CameraLeftOut() const;
 TSharedPtr<class FS47LeftGear> Gear;
 TSharedPtr<class FS47Places> Places;
 // Where a night's event happens: one list for the whole land, the farm one place on it (S47EventPlaces.h).
 TSharedPtr<class FS47EventPlaces> EventPlaces;
 // The operator before you, Ward's review, and the keys (Tom, 7 November).
 TSharedPtr<class FS47Predecessor> Predecessor;
 // The box at the road and the post that comes to it: Ward's papers, the Ledger, the catalog and its parcels, bills (S47Mail.h).
 TSharedPtr<class FS47Mail> Mail;
 TSharedPtr<class FS47RadioTips> Tips;
 // M4: Caliche's counters (1420 AM's tips and parts, the Feed & Seed, the souvenir shop), TOWN-USE-20261108: S47TownUse.h. And the
 // temporary boxes where new buildings will stand (the trailer park, the school, the Grange hall), NEW-PLACES-20261108: S47TownSites.h.
 TSharedPtr<class FS47TownUse> TownUse;
 TSharedPtr<class FS47TownSites> TownSites;
 // Caliche's regulars, who remember, talk, and are missed when they are gone (TOWNSFOLK-20261108): S47Townsfolk.h.
 TSharedPtr<class FS47Townsfolk> Townsfolk;
 // From outside town: the old neighbor, the sky-watch van, the military after big nights (NEIGHBOR, ENTHUSIASTS, MILITARY): S47Visitors.h.
 TSharedPtr<class FS47Visitors> Visitors;
 // A night at the Sierra Motor Court, and its own dangers; not filed until back at SARO (MOTEL-SLEEP-20261108): S47MotelNight.h.
 TSharedPtr<class FS47MotelNight> MotelNight;
 TSharedPtr<class FS47TruckLamp> TruckLamp;
 TSharedPtr<class FS47YardAlarm> YardAlarm;
 TSharedPtr<class FS47TruckCare> Care;
 TSharedPtr<class FS47TripCamera> TripCamera;
 TSharedPtr<class FS47DayRound> Round;
 TSharedPtr<class FS47RoadsideCar> RoadCar;
 // What can happen out in the night (TODO NIGHT31-20261108 proposals 1 to 8), and the counter (GEIGER-20261108): S47Happenings.h.
 TSharedPtr<class FS47Happenings> Happenings;
 // The night's typed log at 06:00, in the notebook and on the telex to Dr. Ward (NIGHTLOG-20261108): S47NightLog.h.
 TSharedPtr<class FS47NightLog> NightLog;
 // The field recorder off the desk by the array's PC, and its cassettes (TAPE-20261108): S47Tape.h.
 TSharedPtr<class FS47Tape> Tape;
 // The case board's field material, put up at SARO, with the station's loose threads and the player's own (CASEBOARD-20261108):
 // S47PinBoard.h.
 TSharedPtr<class FS47PinBoard> PinBoard;
 // The photo lab's enlarger, by the wet bench (ENLARGE-20261108): S47Enlarger.h.
 TSharedPtr<class FS47Enlarger> Enlarger;
 // The weekly report to Dr. Ward, made from the night logs, what goes in the player's choice (WARD-REPORT-20261108): S47WardReport.h.
 TSharedPtr<class FS47WardReport> WardReport;
 // A point of the sky held off schedule, and what may answer from it the next night (ANSWER-20261108): S47Answer.h.
 TSharedPtr<class FS47Answer> SkyAnswer;
 // The day's weather on the world (WEATHER-20261108, S47Weather::Day is the truth the paper prints): S47WeatherLive.h.
 TSharedPtr<class FS47WeatherLive> Weather;
 // The hours awake, sleep and the coffee pot (FATIGUE-20261108): S47Fatigue.h.
 TSharedPtr<class FS47Fatigue> Fatigue;
 // Loose things in the hand, carried and thrown, and what hears them land (PHYSICS-20261108): S47Handling.h. Nothing of it is saved.
 TSharedPtr<class FS47Handling> Handling;
 // The cot at SARO, the same rest as OPS (SARO-COT): S47SaroCot.h.
 TSharedPtr<class FS47SaroCot> Cot;
 // A fault is drawn for today and has not yet come: what a round can still catch.
 bool FaultDueLater() const;
 int32 RecordsOnDay(int32 OnDay) const;
 // What the station has got hold of (FS47Openings). Saved by name; an older save has none.
 bool HasKit(const FString& Id) const{return Kit.Contains(Id);}
 // Empty when it is on offer; otherwise, what it is waiting for.
 FString KitWaits(const FString& Id) const;
 // What it costs now: an assessment on THE LOST HOURS takes something off the magnetometer.
 int32 KitPrice(const FString& Id) const;
 // A place found out on the land, or marks at a site kept as a record: the first field work away from the station.
 bool FieldRecordKept() const;
 // Kinds of record the night's events have left (FS47Happenings::IsNightFinding), from that day on: for Ward and the assessment.
 int32 NightRecordKinds(int32 FromDay=0) const;
 bool BuyKit(const FString& Id);
 // Gear that came by post, already paid for (FS47Mail): the station has it now. False: no such gear, or it has it already.
 bool ReceiveKit(const FString& Id);
 void TestGiveKit(const FString& Id){Kit.Add(Id);}
 // Sleeping away from the station. At a motel nothing wakes you; on a cot out on the land whatever comes by does.
 enum class ERest:uint8{Station,Motel,Field};
 static constexpr int32 MotelRoom=8;
 // A room tonight: MotelRoom, or less with the Ledger's coupon clipped (FS47Mail).
 int32 MotelPrice() const;
 bool RestAt(double Target,ERest Where);
 ERest LastRest() const{return RestPlace;}
 float SiteVisitChance() const;
 void TestEveningGate(){bTestEvening=true;}
 static constexpr double SecondsInDay=86400.;
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
 bool FirstPulseDue(const FJob& Candidate) const;
 void TestFirstPulse(){bTestFirstPulse=true;}
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
 FString FindingText(const FString& Id) const { return Findings.FindRef(Id); }
 // The day a finding was kept on (0: never), and every finding kept on or after a day: what the Ledger reads for its front page.
 int32 FindingDay(const FString& Id) const { return FindingDays.FindRef(Id); }
 TArray<FString> FindingIdsFrom(int32 FromDay) const;
 bool InTutorial() const { return Tutorial>0; }
 void AcceptTutorialJob();
 int32 FindingCount() const { return Findings.Num(); }
 void Recover(const FString& Why);
 bool Spend(int32 Amount,bool bEmergency=false);
 // The county's tow for a truck that cannot be mended: cash if the budget covers it (true), otherwise the whole fee goes on the
 // weekly sheet as carried arrears (false, capped like any carried bill). Never refused and never touches the requisition.
 bool BillTow(int32 Fee);
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
 const FString& AudioStopReason() const { return AudioStopWhy; } // why the reel transport last stopped; a toast does not reach FeedbackText
 int32 AudioSource() const { return AudioSourceIndex; }
 double AudioLevel() const;
 FString AudioDisplay() const;
 FString AudioCaptureStatus() const;
 const FS47Profile* ConsoleProfile() const;
 TSharedPtr<class FS47FreeformAudioDesk> AudioDesk;
 TSharedPtr<class FS47RoomDecor> Decor;
 TSharedPtr<class FS47FarmSite> FarmSite;
 TSharedPtr<class FS47Taken> Taken;
 // What stays a little wrong for a while after being taken, or after a night with them at the farm.
 TSharedPtr<class FS47Aftermath> Aftermath;
 // The county's night patrol, once a letter has said it would come by.
 TSharedPtr<class FS47DeputyRound> Deputy;
 bool DeputyOnRoute() const;
 int32 SheriffTrust() const;
 // The county, plus Caliche: someone seen out at a few places gets told more on the air.
 int32 TownTrust() const;
 int32 Seed() const{return JobSeed;}
 // The workstation's own day and clock, as its header shows them. Only what it shows: WorldClock is never touched.
 FString TerminalClock() const;
 bool OpsOnGlass() const { return ActiveTerminal==3&&TerminalOnGlass(); }
 // Gone for this long: the clock moves, and work under way is broken off as after contact. Nothing secured is lost.
 void TakenAway(double WorldSeconds);
 void TestEndFirstShift(){Tutorial=0;}
 TSharedPtr<class FS47FarmNight> FarmNight;
 TSharedPtr<class FS47FarmHouseDoors> HouseDoors;
 TSharedPtr<class FS47IndoorDread> IndoorDread;
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
 FString AudioStopWhy;
 void NoteAudioStop(const TCHAR* Why,double Dt);
 bool QueueCapture(S47CaptureSound::FTrack& Track,double From,double To,double Dt,double Rate,const S47CaptureSound::FFrame& Input);
 void InterruptCapture(const FString& Why);

 FS47Journal Journal;
 bool ValidJournalHint(const FString& Id) const;
 bool ValidJournalEntry(const FString& Id) const;
 US47Main* M=nullptr;
 bool bActive=false,bDeadly=false,bDead=false,bPowerFault=false,bAnomaly=false,bOldCalendar=false;
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
 int32 WokenNight=0;
 int32 RousedNight=0; // the last night a rest was really broken: never two running
 bool bTestEvening=false,bTestFirstPulse=false,bTestRules=false;
 int32 ReviewWeek=0,Carried=0,StandbyTotal=0,LastPay=20;
 TSet<FString> Kit;
 ERest RestPlace=ERest::Station;
 int32 TakenCount=0; // follows from the findings: an older save has it too, nothing new is stored
 bool LiveFlow() const;
 double WakeClock=0;
 bool bTestWake=false;
 bool WakeBefore(double Target,double& Out);
 int32 CrossPoint=0; double CrossSample=0;
 bool bCrossRunning=false,bShowEarlierJobs=false;
 TArray<FJob> Work;
 TMap<FString,FString> Findings;
 TMap<FString,int32> FindingDays;
 FS47FreeformCases Cases;
 void OpenCaseFor(const FString& FindingId);
 void RecordCaseResult(FJob& J);
 void StepCaseReplies();
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
