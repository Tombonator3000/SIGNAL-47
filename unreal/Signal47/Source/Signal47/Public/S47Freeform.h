#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47CaptureSound.h"
#include "S47Journal.h"
#include "S47Rx.h"
#include "S47FreeformJobs.h"
#include "S47FreeformCases.h"
#include "S47RecordsBinder.h"
#include "S47Dossier.h"
class US47Main;
class FS47FreeformDirector;
class FS47FreeformWorld;
class FS47CaseFieldTests;
struct FS47Task;
struct FS47Doc;
struct FS47MapView;

// Separate, persistent SARO working life. Story progression never grants its jobs or facts.
struct FS47RxLive;
struct FS47RhythmData;
class FS47RxFeed;
class FS47FreeformSession
{
 // The month's assessment test (S47AssessMonthTest.cpp, profile "cases"): files the day's jobs as a fixture.
 friend struct FS47AssessMonthTest;
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
  int32 PaidAmount=-1; // N1110 (T16-20): what was credited when this card paid, for the shift log; -1 before it paid (or in a save from before)
  bool bDocked=false; // the day the player was taken: this card pays half (TakenCosts)
  // Critic 12 Nov (GAMEPLAY-LOOP-01): what the take will be, decided once as the tuned receiver first shows it (DecideTake), so that
  // ANOMALY DETECTED and the take agree whatever the Director says later. -1 not yet, 0 routine, 1 unusual. Saved while Pointed.
  int32 TakeDecided=-1;bool bTakeOut=false;FString TakePhen;
  // RX-ACQUIRE-SKY-20261113: the source found on the receiver's sky chart (ACQUIRE) for this pointing. Saved while Pointed ("sky");
  // a save from before has none, and a job back at the start loses it.
  bool bSky=false;
  // LOOP-CAREFUL-PAYS-20261113 (→ M3, M2): how the fix was made: 0 not known (no chart rule, an older save), 1 by hand (or the
  // programmatic way through, AcquireSky, which counts as a hand fix), 2 the assist pulled the reticle in (no clean bonus). Saved
  // ("skyBy") at every stage from the lock on; a job back at the start loses it with its fix.
  int32 SkyBy=0;
  // LOOP-CAREFUL-PAYS-20261113: both reel-deck marks were the operator's own when the take was confirmed (none filled in). Saved ("ownMarks").
  bool bOwnMarks=false;
 };
 explicit FS47FreeformSession(US47Main* Main);
 ~FS47FreeformSession();
 // bOneNight (STORY-ON-FREEFORM-01 step 2): a new Story night begins on day 1 at 18:00 with no first-shift tutorial. A saved game ignores it.
 void Begin(const S47J::FObj& Saved,bool bLethal=false,bool bOneNight=false);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Active() const { return bActive; }
 bool Held() const;
 bool ArrayMotionHeld() const;
 bool MotorFeedbackActive() const { return bMotorFeedback; }
 int32 AvailableDishCount() const;
 // D45-20261110 (Tom: "array 12 of 27" never moved): the one station-goal count, live. The antennas on remote now, so a relay fault and
 // its repair move it as well as a mended branch; without the world (self-tests) 27 less the worn branches.
 int32 ArrayUpCount() const;
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
 // D48-20261110 (D46-16): the operator says the take sounds like a known reference (the reel deck's MATCH key, the DATA page's MATCH key).
 void MarkReference(int32 Reference);
 void OpenAudio();
 void OpenTapePanel();
 bool TapePanelOpen() const { return bTapePanel; }
 const FString& FeedbackText() const { return Feedback; }
 bool TerminalOnGlass() const;
 bool TerminalWindowRendered() const;
 int32 ActiveTerminalScreen() const { return ActiveTerminal; }
 int32 OperationsSection() const { return OpsSection; }
 bool DriveCheckRunning() const { return DriveCheckAt>=0; }
 const FString& DriveCheckText() const { return DriveCheckResult; }
 bool DriveCheckedToday() const { return DriveCheckAt<0&&DriveCheckDay==DayNumber; }
 const FString& DriveCheckResultText() const { return DriveCheckResult; }
 TArray<FString> TerminalLines(int32 Screen) const;
 // FIX-20261110: the live strip under an idle screen's lines (SS47Crt), and what is on the air
 // around the station (the gear's own level: the lamp, the walkie and the watch feel the same thing).
 struct FTerminalIdle { TArray<uint8> Dish; float Tuned=0,Card=0,Az=0,El=0,AimAz=0,AimEl=0,Rec=-1; bool bReady=false,bWaiting=false; };
 FTerminalIdle TerminalIdle(int32 Screen) const;
 float ScreenInterference() const;
 // One line for screen 0 to 3 from the HUD's objective: "NEXT / USE THIS PC", "NEXT / <place>", or empty if it will not fit.
 static FString TerminalNextLine(const FString& Text,int32 Screen);
 FString MeasurementRecord() const;
 bool ArrayBlocked() const; // too few antennas to measure at all
 // bBinderOffGlass: off the OPS glass, the records-room binder (OpenBinder); false: the paper records page itself.
 void OpenArchive(bool bBinderOffGlass=true);
 void OpenPhotos(); // FIX-20261110: OPS / PHOTOGRAPHS, the film record and the allowance desk
 int32 ArchiveTab=0,ArchivePage=0;FString ArchivePick; // presentation only, never saved
 // RECORDS-BINDER (-> M3): the records-room binder on paper (S47RecordsBinder.h the model, SS47Binder.h the pages; the session's
 // part is S47FreeformBinder.cpp). OpenArchive opens it whenever the records are not on the OPS glass. BinderShow opens it at one
 // entry ("file:station-03", "job:<id>", "case:0", "rec:<finding>"). Where it is open is presentation only; the one thing saved is
 // when each packet went in ("binderFiled", by job id; an older save has none, and its filed pages carry their day alone).
 void OpenBinder();
 void BinderShow(const FString& EntryKey,int32 Page=0);
 S47Binder::FSource BinderSource() const;
 TSharedRef<S47Binder::FView> BinderView=MakeShared<S47Binder::FView>();
 static bool BinderSelfTest(TArray<FString>& Lines); // the stamps across a Continue, and what the session hands the binder
 // EVIDENCE-DOSSIER-20261113 (-> M3, M4), slice 1: one file for each unknown thing the player has logged (S47Dossier.h; the session's
 // part is S47FreeformDossier.cpp). TagEvidence puts a find in its file where the source already knows which one it was, and stamps
 // it on the screen there and then (the file's number and the kind of evidence, nothing else); a find tagged already is left alone.
 // Night 0: today. TagPhotoClaim: a photograph claim ("strange.<group>.n<night>"), when its group is one of the five. Saved as
 // "dossier" once there is a tag; a save from before is rebuilt from what it kept (FS47Dossier::Rebuild).
 FS47Dossier Unknowns;
 // LOOP-DOSSIER-GIVES-20261113: bHere, the find was got where the player stands (its hour and place go in the tag); false for a bench's
 // (an enlargement in the darkroom). What each stage gives: the map's mark (DossierMap), the extra bearing (FS47Places::AfterRegistered
 // asks DossierExtraOpen and DossierSpendExtra), the hours on tonight's note (DossierTasks).
 bool TagEvidence(const FString& Key,S47Dossier::EKind Kind,S47Dossier::EType Type,int32 Night=0,bool bHere=true);
 bool TagPhotoClaim(const FString& ClaimId,bool bHere=true);
 // EVIDENCE-DOSSIER slice 2 (14 November): with the hour and the place given (Hour -1, Where null: not kept), as a tape played back
 // at the desk has them from where it was recorded. The file is the one the player could tie it to (FS47Dossier::Tag).
 bool TagEvidenceAt(const FString& Key,S47Dossier::EKind Kind,S47Dossier::EType Type,int32 Night,int32 Hour,const FVector2D* Where);
 // The receiver's unusual take, heard (S47AlienKinds' kind as an int, the tier of its hour as ES47Allowed): the far two's TAPE.
 void DossierHeardTake(int32 SigNight,const FString& JobId,int32 AlienKind,int32 TierCode,double Cursor);
 static int32 DossierHabitsFrom(const FS47RhythmData& Rules); // the curve's first week with danger in it
 void DossierMap(FS47MapView& View) const;
 void DossierTasks(TArray<FS47Task>& Out) const;
 bool DossierExtraOpen(int32 Night) const;
 bool DossierSpendExtra(int32 Night);
 void BinderUnknowns(S47Binder::FSource& Src) const; // the files as the binder's UNKNOWNS divider has them
 void LoadDossier(const S47J::FObj& Saved);
 void SaveDossier(const S47J::FObj& Record) const;
 // EVIDENCE-DOSSIER slice 3 (14 November). At the binder in the records room (bAt: NearBinder; a test passes it): the player's own
 // name written on a file, two filed as one, one taken out again (until 08:00); the slip's words either way. DossierAct: a binder
 // line's Do ("name:2", "merge:1:3", "out:1:3"). A file FILED (named, at HABITS) is paid FiledAllowance once, under the week's cap
 // (DossierPayFiled). The habits as the player's own thoughts, once a night a file (HabitThought, by number; tonight's saved as "habitCame"): the hour
 // coming up on a night out (DossierHabitStep, every frame from S47Thoughts::Step) and the lamp raised near one (DossierHabitLamp,
 // FS47FarmSite). Dr. Ward's count is Unknowns.KindsFiled (FS47Predecessor::Count).
 bool DossierName(int32 Number,const FString& Typed,FString& Words,bool bAt);
 bool DossierFileAsOne(int32 A,int32 B,FString& Words,bool bAt);
 bool DossierTakeOut(int32 Into,int32 From,FString& Words,bool bAt);
 bool DossierAct(const FString& Do,const FString& Typed,FString& Words);
 int32 DossierPayFiled();
 void DossierHabitStep();
 bool DossierHabitLamp(S47Dossier::EKind Near);
 // REVIEW (14 Nov): the hour's habit is a quiet thought: never while the night is held, a visit is on (at the site or at SARO), the
 // carried set hears something hunting, or a light is taking someone
 static bool HabitHourMay(bool bHeld,bool bVisit,bool bHunted,bool bTaking){return !bHeld&&!bVisit&&!bHunted&&!bTaking;}
 TMap<int32,int32> HabitThought;double HabitLastCheck=-1e9;
 static constexpr int32 FiledAllowance=5;
 FString TestNameDossier();   // F10: every file named by its suggestion, as at the binder
 FString TestMergeDossier();  // F10: the first pair kept side by side filed as one, or taken out again tonight
 FString TestFillDossier(); // F10: sample evidence for three files; what it says
 static bool DossierSelfTest(TArray<FString>& Lines);
 // FIX-20261110: the station's files (S47StationFiles), a new one marks RECORDS. FIX10-10 (12 Nov): the files opened are saved
 // (stationFilesSeen, by key), so a released file stays marked until it is read, across days and Continue.
 uint32 StationFilesSeen=0;bool StationFileDue() const;
 void OpenCaseBoard(int32 Line=-1,bool bNotes=false); // D48-20261110: bNotes opens it with the case notes (traces, leads, records) unfolded
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
 // What there is to do with no job under way. Card: the same in a line the HUD's cassette card has room for (S47HudGauges::CardLine).
 FString AvailableActivities(FString* Card=nullptr) const;
 // The next branch of a worn array, on one line for the HUD; empty once it is whole, or in a save without wear. From one in the
 // morning until daylight it waits (GoalWaits): the antennas are out in the dark, and Dale wants the player in the building.
 FString StationGoalLine() const;
 static constexpr double GoalWaitFrom=1*3600.,GoalWaitUntil=6*3600.;
 static bool GoalWaits(double Clock){const double T=FMath::Fmod(Clock,86400.);return T>=GoalWaitFrom&&T<GoalWaitUntil;}
 // M7-HUD-20261109: the objective as the HUD's card says it, at most S47HudGauges::CardMost characters: the place and the next thing
 // to do. Objective() stays whole for the pause menu and the notebook.
 FString HudObjective() const;
 // Dr. Ward's letter or his assessment waiting (FS47Predecessor::TaskLine "MAILBOX / ...", "OPS / There is a letter ...", "OPS / Dr.
 // Ward's assessment ..."): the next thing to do once the first shift is done.
 static bool LetterWaits(const FString& Line);
 // TASKS-TEN-20261113 (change 3): a line that says how things stand (SARO under review, the post's flag up, a parcel due, YOU / Tired):
 // never a row on the night's list
 static bool IsStatusLine(const FString& Line);
 // From day two the day's third card is a real evening's work: its source is not up until about half an hour before the
 // card's suggested time. Nothing else waits on it, it never expires, and OPS can wait for it. Developer runs leave it
 // open (their fixtures run all three cards in one sitting) unless a test asks.
 // LOOP-NIGHT-WATCH-20261113 (→ M1, M7): from day 2 the third card is the night watch (FS47FreeformJobs::NightWatch): its source is
 // up between 01:15 and 01:45, after midnight on its own shift day, and one not begun by 04:00 (NightWatchCloses) waits for the next
 // night. Both times are on the shift's clock (FS47FreeformJobs::ShiftClock: 08:00 to 08:00, the small hours after 24:00).
 static constexpr double EveningLead=30*60.,EveningOpens=19*3600.,EveningCloses=23.5*3600.,NightWatchCloses=28*3600.;
 // The clock of day this card's source comes up, on the shift's clock: never before seven in the evening.
 static double EveningUp(const FJob& Job){return FMath::Max(EveningOpens,FS47FreeformJobs::ShiftClock(Job.Card.SuggestedClock)-EveningLead);}
 // When one not begun by then waits for the next evening (or night), on the shift's clock.
 static double EveningShut(const FJob& Job){return NightWatch(Job)?NightWatchCloses:EveningCloses;}
 static bool NightWatch(const FJob& Job){return Job.bGenerated&&Job.CaseTest<0&&FS47FreeformJobs::NightWatch(Job.Card);}
 // LOOP-NIGHT-WATCH-20261113: whether OPS may say "or rest" now: not from 19:00 until 02:30 of a shift from day 2 (the night watch
 // keeps him up into the full hours), unless he is worn out (FS47Fatigue at S47Thoughts::ExhaustedFrom). Pure.
 static bool RestSuggestedAt(double AtClock,int32 ShiftDay,bool bLive,bool bWorn);
 // LOOP-NIGHT-WATCH-20261113 (Tom 13 November: «nattevakta gå foran»): from 19:00 until 04:00 on the shift's clock, while one of the
 // night's cards from day 2 (the night watch last) is still to be done, the cards and the night watch's notice come before the station
 // goal on the next step (WorkActivities) and in the notebook (Tasks); the goal is said after them, and leads again by day.
 bool NightBeforeGoal() const;
 // A live bearing slip whose circle on the map has not been stood in yet (FS47Places::SlipRows).
 bool SlipToFollow() const;
 // TASKS-TEN-20261113 (Tom 13 November, «Godtar ti endringene»; change 2, one arbiter): the HUD's ladder (Objective, NextAction,
 // AvailableActivities, WorkActivities) is the one ordering every surface reads: the HUD card, the pause menu's TONIGHT Next, the
 // notebook's arrow and the workstations' NEXT line. Blocking now (paper on the printer, the power, a take under way, RETAIN) > a
 // window open now > a paid card that is up > the night watch or the evening's card with its time > a deadline before noon (the
 // bearing slip, a report, worth a look, what Dale asked) > free time. ObjectiveRow: which of the night's rows the step is (the
 // note marks that one), as the ladder chose it: "cards" (the cards up), "card:<id>" (a card not up yet), a task line's own words,
 // or empty (the step is no row of the list: the note writes it as the HUD has it).
 FString ObjectiveRow() const;
 // D69-20261113: whether the ladder has a step above a deadline (paper on the printer, the telex, a window open now, a paid card
 // up, a planned case test's job, the night's cards from 19:00): Dr. Ward's unread letter waits under those, with the deadlines.
 bool StepAheadOfLetter() const;
 // The field test whose dated window is open now, its step in Next. False: none.
 bool WindowOpenStep(FString& Next) const;
 // A field test made ready whose window has not opened: it waits, named with its time; it is no next step before then.
 bool FieldWindowWaits(int32 Line) const;
 // The first deadline before noon still open: a bearing slip's circle, a report, a worth-a-look, what Dale asked. Empty: none.
 FString DeadlineLine() const;
 // TASKS-TEN-20261113 (change 7): what Dale asks over the walkie (get outside and look, put it on your map, write it in the night log)
 // is kept: a row "DALE ASKED / ..." under FOLLOW-UP on the note and NOW, in his own words (no new line of his), and a line in the
 // night log as he asks; ticked off when it is done (outdoors, the map looked at, a line logged by hand), gone at 08:00. Saved with
 // the notebook's state ("notePlan"; an older save has none). DaleSaid: a line of his heard on the set (FS47FieldRadio::Message).
 struct FDaleAsk { FString Kind, Row; int32 Night=0; int32 Base=0; bool bDone=false; };
 void DaleSaid(const FString& Words);
 TArray<FString> DaleAskRows() const; // the open ones, oldest first
 const TArray<FDaleAsk>& DaleAsks() const { return DaleAskList; }
 // TASKS-TEN-20261113 (change 10): the row the player pinned on NOW ("this is what I am doing"; the notebook's PIN, the due tabs'
 // star). With a pin the ladder's step is the pinned matter's next step, after what blocks now (paper waiting, the power, a take
 // under way); CloseSoonLine is the one line beside it when something else closes within 30 minutes. A pin whose row is done or
 // shut stays inert and clears at 08:00. Saved with the notebook's state ("notePlan"). Pin again (the same key) to take it off.
 void PinRow(const FString& Key,const FString& Row);
 FString PinnedKey() const { return bPinLive?PinKey:FString(); }
 // REVIEW-20261113: whether a pinned Ward matter still stands: "matter:report-N" while week N's report is unsent (Unsent), and
 // "matter:assessment" while the predecessor's line says the station is under review (Standing); any other matter key: true. Pure.
 static bool MatterLive(const FString& Key,const TArray<int32>& Unsent,const FString& Standing);
 bool PinnedStep(FString& Full,FString& Short) const;
 FString CloseSoonLine() const;
 // The soonest of the cards waiting for the evening, and the next time it comes up; null if none is.
 const FJob* NextEveningCard(double& Up) const;
 // The rules a real game plays by. Developer fixtures keep the fixed ones unless a test asks (TestLiveRules).
 bool RulesLive() const{return bTestRules||(M&&LiveFlow());}
 void TestLiveRules(){bTestRules=true;}
 // Whose night a clock falls in: the shift day it belongs to. Bodies keep to their own start each night (S47NightRhythm.h).
 static int32 NightOf(double AtClock){return FMath::Max(1,int32(FMath::FloorToDouble((AtClock-8*3600.)/SecondsInDay))+1);}
 // D45-20261110 (D45-26): where OPS's "Wait until dusk" ends on the day of AtClock: 19:00, and on the first day 18:20, so that the
 // first dusk's light (18:30) starts in front of the player instead of half over by the time the wait is done.
 static double DuskWaitAt(double AtClock,int32 ShiftDay){return FMath::FloorToDouble(AtClock/SecondsInDay)*SecondsInDay+(ShiftDay==1?18*3600.+20*60.:19*3600.);}
 // The last night that is over: from four in the morning after it. A rest that jumps the clock to 08:00 has slept through it, and
 // whatever settles a night in the morning (S47Townsfolk, S47Visitors) settles every night up to this one, not only the night of now.
 static int32 NightsOver(double AtClock){return NightOf(AtClock+4*3600.)-1;}
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
 // M7 (Tom, 9 November): less for a card, more for a card done well. The base by how old the card is, and on top of it $2 for a card
 // registered on its day or the next, $3 for a recording that reads (CleanRecordQuality), and $1 for an anomaly carried through its
 // check (the reel deck's one listen, or an instrument's control: GAMEPLAY-LOOP-01). A good card pays $19 against the old $20; a sloppy late one $10 or less. Quality pays better than quantity.
 static int32 PayAfterDays(int32 Days){return Days<=1?14:Days==2?10:Days==3?7:4;}
 static constexpr int32 OnTimeBonus=2,CleanBonus=3,ControlBonus=1;
 static constexpr double CleanRecordQuality=.6;
 // LOOP-CAREFUL-PAYS-20261113 (→ M3, M2): the clean bonus only for a sky fix the operator made himself (FJob::SkyBy 2: none). Hints cost nothing.
 static bool CleanBonusFor(const FJob& Job){return Job.SkyBy!=2;}
 // LOOP-CAREFUL-PAYS-20261113: marks tight round the extra ticks (9.30 to 12.60 s): covering them (MarkCoversTicks) with at most
 // TightSlack seconds to spare in all, against the 2.7 s the RETAIN check allows. 9.0 to 13.0 is tight; 8.0 to 13.5 is not.
 static constexpr double TightSlack=1.;
 static bool MarksTight(double In,double Out);
 // LOOP-ANOMALY-ANY-CARD-20261113 (→ M7): from day AnyCardFrom the take that is odd because of what is out tonight may fall on the
 // pulsar (card 2) or the third card (hydrogen drift or Virgo A, the night watch), never the pointing cross (card 1: its 12 s take can
 // end before the last extra tick, and it carries the five-point cross). The day's own carrier by the seed (OddCarrier: 1 or 2); the
 // other only once the carrier's card is past without it. At most one a night. Pure.
 static constexpr int32 AnyCardFrom=15;
 static int32 OddCarrier(int32 Seed,int32 Day);
 int32 JobPay(const FJob& Job) const;
 // What of JobPay is the bonus for good work (0 to 6), and the words for it ("on time, clean recording").
 int32 JobBonus(const FJob& Job,FString* Why=nullptr) const;
 // The first evening's training card (the tutorial's) pays the old flat $20 whatever its state: Ward's welcome.
 bool TrainingCard(const FJob& Job) const{return !TutorialJobId.IsEmpty()&&Job.Id==TutorialJobId;}
 // M7 (C's economy model): the county's power line is charged a base and a fault tariff for each antenna that is out of service.
 // CHAIN-5-20261109: it used to be a charge per antenna in service, so every repair made the sheet dearer; now every repair makes it
 // cheaper (a full array costs the base only). The tariff goes up with the weeks: $1 an antenna out on the first sheet, $1.50 on the
 // second and third, $2 from the fourth (UpkeepHalves, half-dollars). The same range as before: $10 to $64.
 static constexpr int32 ReviewPowerBase=10,ReviewPowerPerDish=1,ReviewFuel=15,ArrearsCap=80,LostHoursDiscount=15;
 static int32 UpkeepHalves(int32 Week){return Week<=1?2:Week<=3?3:4;}
 static int32 ReviewPowerFor(int32 Week,int32 Dishes){return ReviewPowerBase+FMath::Clamp(27-Dishes,0,27)*UpkeepHalves(Week)/2;} // Dishes: antennas in service (CHAIN-5-20261109)
 int32 ReviewPowerNow() const{return ReviewPowerFor(ReviewWeek+1,AvailableDishCount());}
 // M7 (Tom, 9 November): money from the field work (copies sold to the souvenir man, the documentation credit for a photograph, a tip
 // read on 1420, the sky-watch van) is worth having, and capped so that it never beats the job: SideCap a week, the rest goes unpaid.
 // CHAIN-5-20261109: in the fiction the cap is the university's field allowance, a fixed sum a week that the weekly sheet names.
 // Returns what was paid. Kind is for the log.
 static constexpr int32 SideCap=35;
 int32 EarnSide(int32 Amount,const TCHAR* Kind);
 int32 SideLeftThisWeek() const;
 // M7 (Tom, 9 November): time is the second currency. Each card registered takes time off the night and adds to how tired you are
 // (FS47Fatigue::Desk); a third card in one evening more. What the evening's cards took is on OPS.
 static constexpr double DeskHoursPerCard=.25,DeskHoursThirdCard=.6;
 // Cards registered at OPS this night (from 08:00 to 08:00), as they were registered; not saved (a Continue starts the count again).
 int32 CardsTonight() const;
 // TASKS-TEN-20261113 (change 4): the job cards up now, counted one way on every surface (the HUD, the note, NOW, the map, OPS):
 // not filed, not dropped, no case test (that is the case's own row), its source up (not EveningGated). A card not up yet is never
 // counted: it is named with its time.
 int32 CardsUp() const;
 // M7 (Tom, 9 November): gear can break near what is out there (FS47Upgrades::StepBreakage): what it costs to put right goes on the next sheet.
 static constexpr int32 RepairCap=12; // a week's sheet at most
 void BreakGear(const FString& What,int32 Fee);
 int32 RepairsDue() const{return Repairs;}
 // The running costs on the weekly sheet besides power and stores (critic, 8 November; Claude's numbers, kept low): truck wear per
 // two kilometres driven, film and darkroom chemicals per print developed, and Ward's re-check of a recording nobody can read.
 static constexpr int32 WearPerTwoKm=1,WearCap=15,FilmPerPrint=2,FilmCap=10,PoorRecordFee=3,FeeCap=15;
 // M7 (Tom, 9 November): cassettes are used up in the field too: a dollar a take recorded that week (FS47Tape), at most TapeCap.
 static constexpr int32 TapePerTake=1,TapeCap=6;
 static constexpr double PoorRecordQuality=.3;
 struct FWeekCosts{int32 Wear=0,Film=0,Fee=0,Poor=0,Prints=0,Tapes=0,Tape=0,Repairs=0;double Km=0;int32 Total() const{return Wear+Film+Fee+Tape+Repairs;}};
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
 // D48-20261110: the balloon find's model, its blinking beacon and its instrument box, and what the box brings (S47Balloon.h).
 TSharedPtr<class FS47Balloon> Balloon;
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
 // M7 (Tom, 9 November): the truck's upgrades and the field gear that opens the night, as bought (S47Outfit.h).
 TSharedPtr<class FS47Outfit> Outfit;
 TSharedPtr<class FS47YardAlarm> YardAlarm;
 TSharedPtr<class FS47TruckCare> Care;
 TSharedPtr<class FS47TripCamera> TripCamera;
 TSharedPtr<class FS47DayRound> Round;
 TSharedPtr<class FS47RoadsideCar> RoadCar;
 // What can happen out in the night (TODO NIGHT31-20261108 proposals 1 to 8), and the counter (GEIGER-20261108): S47Happenings.h.
 TSharedPtr<class FS47Happenings> Happenings;
 // WEEKLY-CASE-20261109: the week's authored case, one owner that whoever asks about danger sees (S47WeekCase.h).
 TSharedPtr<class FS47WeekCase> WeekCase;
 // DIRECTOR-TOOL-20261109: Tom's scripted events (Content/Director/scripted) and their editor behind F10: S47ScriptPlayer.h.
 TSharedPtr<class FS47ScriptPlayer> Scripts;
 // The night's typed log at 06:00, in the notebook and on the telex to Dr. Ward (NIGHTLOG-20261108): S47NightLog.h.
 TSharedPtr<class FS47NightLog> NightLog;
 // The field recorder off the desk by the array's PC, and its cassettes (TAPE-20261108): S47Tape.h.
 TSharedPtr<class FS47Tape> Tape;
 // The old pinboard's data (CASEBOARD-20261108): no screen any more (D46-20261110); kept for the save key, the papers and AtSaro. S47PinBoard.h.
 TSharedPtr<class FS47PinBoard> PinBoard;
 // The photo lab's enlarger, by the wet bench (ENLARGE-20261108): S47Enlarger.h.
 TSharedPtr<class FS47Enlarger> Enlarger;
 // D45-20261110 (D45-17): the lab as a darkroom, its SAFELIGHT switch by the door and the one rule for bench and enlarger: S47Darkroom.h.
 TSharedPtr<class FS47Darkroom> Darkroom;
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
 // N1110 (HORROR-IDEAS-20261106-10 point 6): hiding under the control-room desk or the archive table (S47Hide.h). Nothing saved.
 TSharedPtr<class FS47Hide> Hide;
 // N1110 (N1110-1, N1110-2): how much the player has been bothering them, and what comes of it (S47Attention.h). Saved "attention".
 TSharedPtr<class FS47Attention> Attention;
 // N1110 (N1110-6): the door loop at SARO, the visit's other shape (S47DoorLoop.h). Nothing saved.
 TSharedPtr<class FS47DoorLoop> DoorLoop;
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
 // CHAIN-2-20261109: one record's key is of that kind: a place, marks, what the night left out there, a field time set against the array.
 static bool FieldRecordKey(const FString& Key);
 // N1110 (pace, critic 2): a lesson learned in the field (the stare rule, the still rule held, the still rule broken): its record, once, pays
 // FieldRecordPay like any field record, through the field allowance (EarnSide). Not a field record for what it unlocks.
 static bool LessonRecordKey(const FString& Key){return Key==TEXT("attention_rule")||Key==TEXT("rule-still-held")||Key==TEXT("rule-pale-broken");}
 // Kinds of record the night's events have left (FS47Happenings::IsNightFinding), from that day on: for Ward and the assessment.
 int32 NightRecordKinds(int32 FromDay=0) const;
 bool BuyKit(const FString& Id);
 // Gear that came by post, already paid for (FS47Mail): the station has it now. False: no such gear, or it has it already.
 bool ReceiveKit(const FString& Id);
 void TestGiveKit(const FString& Id){Kit.Add(Id);}
 void TestTakeKit(const FString& Id){Kit.Remove(Id);}
 // Sleeping away from the station. At a motel nothing wakes you; on a cot out on the land whatever comes by does.
 enum class ERest:uint8{Station,Motel,Field};
 static constexpr int32 MotelRoom=8;
 // A room tonight: MotelRoom, or less with the Ledger's coupon clipped (FS47Mail).
 int32 MotelPrice() const;
 // bPaid: the room was paid for already tonight (back to sleep after the night woke you): nothing is charged again.
 bool RestAt(double Target,ERest Where,bool bPaid=false);
 ERest LastRest() const{return RestPlace;}
 float SiteVisitChance() const;
 void TestEveningGate(){bTestEvening=true;}
 static constexpr double SecondsInDay=86400.;
 bool StartReferenceCheck();
 // The optional reference check's job (StartReferenceCheck): free, so it is not paid observing work (FS47Predecessor::Count).
 static bool ReferenceCheck(const FJob& Job){return Job.Id.Contains(TEXT("-REFERENCE-"));}
 bool ConcludeCase(int32 Line,int32 Cause);
 // Critic round 5: the line has something behind it as Dr. Ward counts it (FS47FreeformCases::Supported, with the traces kept):
 // FS47Predecessor::Count and the case board's warning before a filing go by the same sum.
 bool CaseSupported(int32 Line) const;
 // CHAIN-2-20261109: carried through (CaseSupported) with something brought in from out there: OUT ON THE LAND itself, a control done
 // in the field, a lead that is a field record, or a mystery's place away from the station looked at. Ward wants one (Decide).
 bool CaseFromField(int32 Line) const;
 FString TonightPlan() const;
 // CASES-OPS-20261109 (Docs/Research/Gameplay/SAKER_OPS_2026-11-09.md): where an open case stands and its next step, worked out from
 // the case file and the work list as they are (nothing saved). The OPS banner and the HUD (Objective), the task list
 // (Tasks), TONIGHT'S PLAN and the pause menu's note, the notebook's Now and the case board's star (CaseAwaitingChoice) all read it.
 // CHOOSE_TEST: a test (or control) can be planned; for a mystery, or a filing. RUN_PLAN: the planned test can be run now.
 // CONCLUDE: a trial case with one explanation left. FILE: a mystery with one
 // left, its trail cold or no control to be had. AWAIT_REPLY: filed, the morning's reply not in, or in on the telex and not torn off
 // yet (TELEX-20261109). CLOSED: nothing (or not open; the interval case opens when its comparison is registered at OPS).
 // (Tom, 13 November: WAIT_WINDOW, the repeat test's wait for its dated window, went with the repeat test.)
 enum class ECaseStep : uint8 { Closed, ChooseTest, RunPlan, Conclude, File, AwaitReply };
 struct FCaseStep
 {
  ECaseStep Step=ECaseStep::Closed;
  FString Next; // the whole step, as the objective says it: "OPS / CASE BOARD: REGISTER THE SUPPORTED CONCLUSION FOR ..."
  FString Card; // the HUD's card when the case is the objective: "CASE BOARD // ..."
  FString Short;// the HUD card's second line: "THE SAME MINUTE: REPLY DUE 08:00."
  FString Task; // the task list's line: "CASE / THE SAME MINUTE: file your assessment on the OPS case board"; empty: not a task
 };
 FCaseStep CaseStep(int32 Line) const;
 // The open case whose step is most pressing: CONCLUDE, RUN_PLAN, FILE, CHOOSE_TEST, then (bActionOnly false) AWAIT_REPLY; the
 // lower line first among equals. -1: none.
 int32 UrgentCase(bool bActionOnly=false) const;
 // (TASKS-TEN-20261113, change 5: the HUD card's second line for a case, HudCaseLine, is gone: one line, from one arbiter)
 // A case record kept (FS47FreeformCases::IsCaseRecord) as one line under the night log's WORK: "THE REPEATED INTERVAL:
 // off-source pointing registered; rules out Follows the sky source." Empty for any other finding.
 FString CaseRecordLine(const FString& Key) const;
 const FS47FreeformCases& CaseFile() const { return Cases; }
 // CASES-OPS-20261109, step 4 (Docs/Research/Gameplay/SAKER_OPS_2026-11-09.md): the case sheet as OPS / CASE BOARD heads it and the
 // printed result sheet repeats it. NEXT from CaseStep ("NEXT > CONCLUDE: INSTRUMENT ARTEFACT"); the kind ("TRIAL CASE", "ASSESSMENT");
 // how it closes; what closing it brings, before the choice ("ON CLOSE: ..."); where it came from with the original's own clock; and
 // each explanation's standing ("STILL POSSIBLE", "RULED OUT D03"). Worked out from the case file as it is; nothing saved.
 FString CaseNextLine(int32 Line) const;
 static FString CaseKind(int32 Line);
 static FString CaseClosesWhen(int32 Line);
 FString CaseOnClose(int32 Line) const;
 FString CaseSource(int32 Line) const;
 FString CaseStanding(int32 Line,int32 Cause) const;
 // The block of lines at the head of the sheet: kind, closes when, on close, source, then A/B/C with their standing.
 FString CaseSheetHead(int32 Line,int32 MarkTest=-1) const; // MarkTest: the result whose fallen explanations say "<< THIS RESULT"
 // The result sheet (step 4): each test result registered (a case job at OPS, or a field test reported) is printed on the OPS printer,
 // stamped RULED OUT / Dnn or STILL POSSIBLE, after any job's paper. Collected at the printer like an original; it opens as a paper,
 // goes in the documents, and the next visit to OPS opens CASE BOARD on that case. Saved while waiting (an older save has none).
 struct FResultSheet { int32 Line=-1,Test=-1,Day=0; double Feed=0; FString Job,Stamp,Page; };
 const TArray<FResultSheet>& ResultSheets() const { return CaseSheets; }
 // LOOP-1-20261109: a bearing slip (FS47Places::AfterRegistered) waits in the same queue as a sheet of no case: Line and Test -1, Job
 // its document's id. Printed after any job's paper and collected at the printer the same way; saved while waiting. False: the
 // printer already holds as many papers as it takes, and the bearing is on the map without it.
 static bool IsBearingSlip(const FResultSheet& Sh){return Sh.Line<0&&Sh.Test<0&&Sh.Job.StartsWith(TEXT("freeform-bearing-"));}
 bool QueueBearingSlip(int32 Day,const FString& Line,const FString& Note,bool bAgain=false); // bAgain: a second slip that night (LOOP-DOSSIER-GIVES-20261113, MARKS), a paper of its own
 // -1: no result sheet waiting; otherwise its paper fraction 0..1 (it feeds only while no job's paper is waiting).
 double SheetProgress() const;
 // TELEX-20261109 (Tom, 9 November, DECISIONS "Saker og OPS"): the telex in the control room is paper. What comes back to SARO, a
 // filed case's morning reply, Dr. Ward's office on a weekly report (and her query about one) or the receipt for a night log she has
 // initialed, prints there with its motor and type bars while the paper grows out of the top (S47FreeformTelex.cpp). The player tears
 // it off at the machine (E, "Tear off the telex"); only then is it a document in the notebook and a paper IN THE TRAY at OPS. What
 // SARO sends still goes from OPS. The sheets on the machine and the tray are saved ("telex"); an older save has none.
 struct FTelexSheet { FString DocId,Title,Stamp,Page; int32 Day=0; double Feed=0; };
 static constexpr double TelexSeconds=8.;
 // A sheet for the telex. False when that paper is already on the machine or the machine holds MostTelex (the caller files it as
 // before, so that nothing is lost).
 bool QueueTelex(const FString& DocId,const FString& Title,const FString& Stamp,const FString& Page);
 bool OnTelex(const FString& DocId) const; // printing or printed, not torn off yet
 bool InTelexTray(const FString& DocId) const;
 bool TelexReady() const;                   // the first sheet has printed and can be torn off
 int32 TelexWaiting() const { return TelexSheets.Num(); }
 const TArray<FTelexSheet>& TelexQueue() const { return TelexSheets; }
 const TArray<FTelexSheet>& TelexTray() const { return TelexTorn; }
 // Tears off the first printed sheet: its document goes in the notebook (and opens), the paper in the OPS tray. Out: that document.
 bool TearOffTelex(FS47Doc* Out=nullptr);
 // What the machine says: "TELEX: A reply is waiting" (printed), "TELEX: printing" (not yet), empty with nothing on it.
 FString TelexLine() const;
 static constexpr int32 MostTelex=8,MostTray=6;
 static constexpr int32 CaseBoardRecords=12; // LOOP-WHOLE-20261109: field records shown on a case board; the rest are counted
 // LOOP-3-20261109 (HELHETLIG_LOKKE step 3): what comes back from the field comes home as a question. The array keeps its own log of
 // what it caught off schedule, minute by minute (ArrayLog: a landing, whether anyone saw it, a burst off the main beam, the big
 // night's turn, an answer on the monitor channel). A time from the field (a developed photograph from a night out there, a trip
 // camera frame, the loan field logger's loudest minute) is set against it when the player is back at SARO (FS47PinBoard::AtSaro,
 // the session's Step, no button of its own): "SAME MINUTE ON ARRAY: YES/NO" on the telex, and a field record of OUT ON THE LAND. YES
 // strikes Animals and weather there (FS47FreeformCases::RuleOutWind); NO strikes nothing. Asked once a record. Saved under
 // "minutes"; an older save has none and its log is read again from the array's own records (the "At 02:14" in them).
 struct FArrayMark{double From=0,To=0;FString Kind;};
 static constexpr int32 MostArrayMarks=48,MostNightClocks=64,MostAsked=1024,MostAskedAtOnce=3,FieldRecordPay=8;
 static constexpr double SameSlack=60,NightPhotoNear=30*60;
 // The array caught something from From to To (game clock). Kept newest last, at most MostArrayMarks.
 void NoteArrayMinute(double From,double To,const FString& Kind);
 const TArray<FArrayMark>& ArrayLog() const{return ArrayMarks;}
 // Pure: whether the log has something within a minute of Clock.
 static bool SameMinute(const TArray<FArrayMark>& Marks,double Clock);
 // Pure: the first "At 02:14" or " at 02:14" in a record, as seconds into the day; false when it has none.
 static bool ClockIn(const FString& Text,int32& OutSeconds);
 // The night record a photograph taken at ShotClock belongs to (kept within half an hour of it, at night), or empty.
 FString NightPhotoFrom(double ShotClock) const;
 // Sets a field time against the array's log once (RecordId: the photograph, the tape): the telex, the record, the case. True when
 // it was asked now.
 bool AskSameMinute(const FString& RecordId,double Clock,const FString& What);
 // Asked already: its YES or NO record is kept for good (Findings, saved), or it is in Asked (a record primed from an older save, which
 // has none). The records are the mark, so trimming Asked never makes a record due again.
 bool AskedSameMinute(const FString& RecordId) const;
 // At SARO (asked from Step once a second, D46-20261110): every developed photograph from a night out there, trip camera frame and retrieved logger tape not
 // asked yet, the newest first, at most MostAskedAtOnce at a time. The number asked.
 int32 AskWhatCameBack();
 // FIX10-9 (→ M3, "mer å merke for den som blir inne"): what the player logged by hand in the notebook in a night's dark hours
 // (FS47NightLog::LogOwn, EVENTS) is a time too. Once that night is over (06:00) and the player is at SARO, OPS sets one of them against
 // the array's log, the same question and the same telex as a photograph's: the first in the same minute as something the array
 // caught, otherwise the night's first. Asked once a night ("own-N"), only for the night just over: a held night's notes, or an older
 // save's, are never asked all at once. Pure: the index into Clocks, -1 none.
 static int32 OwnNoteFor(const TArray<double>& Clocks,const TArray<FArrayMark>& Marks,int32 Night);
 static FString OwnNoteId(int32 Night){return FString::Printf(TEXT("own-%d"),Night);}
 static constexpr const TCHAR* OwnNoteWhat=TEXT("your own note in the notebook");
 double MinuteLook=0; // D46-20261110: seconds since AskWhatCameBack was last tried at SARO
 // OPS (FS47FreeformDirector::RegisterFieldRecord): a developed photograph from one of the night's happenings out there is a field
 // record the station pays for once, as field money under the week's cap (EarnSide, FieldRecordPay). -1: refused (not developed, not
 // from a night out there, already registered); otherwise what was paid (0 when the week's field money is used up).
 int32 RegisterNightPhoto(const FString& ShotId,double ShotClock,bool bDeveloped,const FString& Title);
 // CHAIN-2-20261109: taken and registered on the night of tonight's lead (FS47Happenings::Lead: the night the card ran off and the slip
 // pointed out there), it pays the same-night rate, still under the week's cap. The receipt says so (SameNightMark).
 static constexpr int32 SameNightPay=4;
 static constexpr const TCHAR* SameNightMark=TEXT("Same-night rate");
 bool NightPhotoRegistered(const FString& ShotId) const{return HasFinding(TEXT("field-registration.")+ShotId);}
 static bool MinuteSelfTest(TArray<FString>& Lines);
 // N1110 CASEBOARD-20261108: one line of the case board's trace book: day, time (none if Clock<0), place (may be empty), kind, text.
 static FString TraceBookLine(int32 Day,double Clock,const FString& Place,const TCHAR* Kind,const FString& Text);
 // Field-work case tests (FS47FreeformCases::ECondition::FieldWork) are carried out in the world.
 // The world asks which test is planned and what the hidden cause makes it show, enacts that
 // physically, then reports what the player actually secured. -1 when no field test is planned.
 int32 PlannedFieldTest(int32 Line) const;
 bool CancelFieldTest(int32 Line);
 bool CancelCaseJob(int32 Line); // a planned receiver test, before it records (CASETEST-20261109: also once pointed, or spoiled)
 bool FieldTestExpectsPresent(int32 Line,int32 Test) const;
 bool ReportFieldTest(int32 Line,int32 Test,bool bValid,bool bPresent,const FString& Evidence);
 // LOOP-WHOLE-20261109, step 2: OUT ON THE LAND's controls are the kit the player has. What a planned one still needs (empty: it can
 // be planned): the trip camera built, the magnetometer bought (the field logger's loan is the test's own access).
 FString LandTestNeeds(int32 Line,int32 Test) const;
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
 // The receiver offers a control only for the instrument discrepancies now (GAMEPLAY-LOOP-01). An extra-tick take has none to
 // record: its 8-second control stays possible at the physical rack, optional and never asked for.
 bool CanRecordControl() const;
 // GAMEPLAY-LOOP-01 (Tom, 12 November): the unusual pulsar take, extra ticks off the card, goes straight through six steps, each one
 // thing at one place: ANOMALY DETECTED on the receiver, record it there, one listen at the reel deck, confirm at OPS, the packet at
 // the printer, the binder. No control to record, no second reel, no chart to fetch: the original is kept as the take ends.
 static bool PlainAnomaly(const FJob& J);
 // One listen at the reel deck: the original heard from the start past its last extra tick.
 static bool TapeHeard(const FJob& J);
 // RX-ANOMALY-FLOW-20261113 (-> M3, Tom 13 November: from RECORD on the receiver to the sound confirmed at the reel deck, with a
 // reference to hear it against). The extra-tick take, once it ends, waits on the receiver's RECORD tab until the operator retains it
 // there (RetainTake): its original page goes into the notebook and the 8-second station reference (antennas out, the internal
 // control) records with it. Only then does the reel deck play it; RETAIN at the deck is the operator's own press (ConfirmOnTape).
 static bool RetainWaits(const FJob& J);
 static FString PlainAnomalyNext(const FJob& J); // the next step of an extra-tick take after its take ends
 bool RetainTake();
 // The receiver's RECORD tab holds the extra-tick take from ANOMALY DETECTED until it is retained there.
 bool RecordTabHoldsAnomaly() const;
 // The receiver tab open and the tab due (-1 none): 0 ACQUIRE, 1 TUNE, 2 ARRAY, 3 RECORD, 4 DATA (read by the native test).
 int32 ReceiverPage() const { return ReceiverTab; }
 int32 ReceiverDuePage() const { return ReceiverDueTab(); }
 // The selected job, tuned and ready, would be an extra-tick take if recorded now (StartAcquisition decides the same way).
 bool AnomalyAhead() const;
 // RX-ACQUIRE-SKY-20261113 (-> M3, Tom 13 November): the receiver's ACQUIRE page is a sky chart; the operator steers a reticle to where
 // the antennas' bearings cross and the receiver takes the fix, then TUNE is the frequency search (S47SkyAcquire.h the rule,
 // SS47SkyChart the picture, S47FreeformSky.cpp the session's part). Where the rule is live (a real game; a developer run only
 // when a test asks) recording waits for the fix: ReceiverReady asks SkyAcquired.
 // AcquireSky: the way through without a hand on the chart (bots, tests, the F10 card player): the fix at once, if the selected
 // job's array is on target and settled. True when the job has its fix (now or already).
 bool AcquireSky();
 bool SkyAcquired() const;
 bool SkyRuleLive() const;
 void TestSkyRule(){bTestSky=true;}
 // The selected job's source is the unusual kind on the chart (the same answer the take will give: AnomalyAhead's, without the tuning).
 bool SkyOdd() const;
 TSharedPtr<class FS47SkyAcquire> SkyFix;
 // RX-SEARCH-SOUND-20261113: Tom's search sound under ACQUIRE and TUNE (S47RxSearch.h), stepped once a frame by the session
 TSharedPtr<class FS47RxSearch> RxSearch;
 void StepRxSearch(double Dt);
 static bool SkySelfTest(TArray<FString>& Lines);
 // RX-LOOK-20261113 (-> M3, M6; Tom 13 November: the receiver to look like his reference picture): what the receiver PC's frame
 // shows this frame (S47RxLook.h): the header's clock and the array's own pointing, the state banner's words (the same order and
 // conditions as ReceiverNextAction and the idle picture's ANOMALY DETECTED), the card's frequency, and the receiver's own numbers
 // for the small scopes. Nothing is decided here. RxFeed: one reading a frame for every painted part. SiderealDeg: the station's
 // sidereal time from the world's clock and date, for the sky chart's stars.
 FS47RxLive RxLive() const;
 TSharedRef<FS47RxFeed> RxFeed() const;
 double SiderealDeg() const;
 bool AudioAction(const FString& Action);
 void SeekAudio(double At);
 // Marks by a place on the tape, without moving it: first click is MARK IN, second MARK OUT, a third starts over.
 bool MarkAudioAt(double At);
 static FString MarkSaid(const FJob& J); // RX-ANOMALY-FLOW-20261113: what the marks cover, in the deck's words
 bool CompareInstrument();
 bool RegisterComparison(const FString& Conclusion);
 bool FilePacket();
 // The player stands at the records-room binder (where a packet can be filed); also read by the F10 card player.
 bool NearBinder() const;
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
 // N1110 (pace, critic 3): the night's one big thing. A record is one if it is a taking on this day (taken-<day>-<clock>), the broken rule kept this
 // day, or the door loop or its chase on this day. The chase, the attention visit, the door loop and a landing the night did not plan wait when one has played.
 static bool BigThingKey(const FString& Key,int32 Day,int32 KeptDay){return Key.StartsWith(FString::Printf(TEXT("taken-%d-"),Day))||(Key==TEXT("rule-pale-broken")&&KeptDay==Day)||Key==FString::Printf(TEXT("door-loop-d%d"),Day)||Key==FString::Printf(TEXT("door-loop-chase-d%d"),Day);}
 bool BigThingToday() const{for(const TPair<FString,int32>& K:FindingDays)if(K.Value==DayNumber&&BigThingKey(K.Key,DayNumber,K.Value))return true;return false;}
 bool InTutorial() const { return Tutorial>0; }
 // SPAWN-MAILBOX-20261109: this new game began at the box at the road (US47Flow), so Dr. Ward's letter there is the HUD's first
 // task, before the first shift's OPS. Not saved: a Continue goes on with the tutorial as it was.
 bool bMailboxStart=false;
 // FIX-20261110 (Tom, points 4 and 8): a new operator finds the station shut up and dark; the keys are in the envelope with
 // Dr. Ward's letter in the box at the road. Saved only while locked: an older save, a developer start and every game after the
 // letter is out of the box have the keys. Never a trap: a locked station with no letter left in the box opens (StationLocked).
 bool StationLocked();
 void LockStation();            // US47Flow, the new game at the box
 void StationKeyFound();        // FS47Mail, the letter taken out
 bool RefuseLocked(const FString& DoorId); // true: this way in is locked, and the player was told so
 static bool StationDoor(const FString& Id);
 bool bStationLocked=false,bDarkTold=false;
 void AcceptTutorialJob();
 int32 FindingCount() const { return Findings.Num(); }
 void Recover(const FString& Why);
 bool Spend(int32 Amount,bool bEmergency=false);
 // The county's tow for a truck that cannot be mended: cash if the budget covers it (true), otherwise the whole fee goes on the
 // weekly sheet as carried arrears (false, capped like any carried bill). Never refused and never touches the requisition.
 bool BillTow(int32 Fee);
 void Earn(int32 Amount);
 // Money handed back for something not had (a parcel that could not be handed over): to cash, never to the requisition.
 void Refund(int32 Amount);
 int32 InventoryCount(const FString& Id) const;
 void AddItem(const FString& Id,int32 Count);
 bool Consume(const FString& Id);
 void SetPowerFault(bool bFault);
 bool PowerFault() const { return bPowerFault; }
 // FIX-20261110 (Tom, point 2): what passes over SARO takes the mains with it. Sag seconds of dips and torn screens first (time to
 // get to the standby generator), then the mains are out for Out seconds. A running generator carries the station through it, and
 // the line recorder keeps writing (SkyRode: the record the passage leaves). Nothing of the passage itself is saved.
 // FIX10-10 (12 Nov, critics: "the power comes back by itself after barely two minutes"): when the mains try to come back, the
 // feeder's protection locks out and leaves an ordinary feeder fault (bPowerFault, saved). The station stays dark until the
 // generator carries it or the remote feeder (ff:feeder) is mended (FS47FreeformWorld, the existing repair); the county line crew
 // restores it by itself SkyFaultMinutes game minutes later (SkyFaultUntil, saved), so nobody is left without a way on. Not on a
 // Story night: there the act's own feeder_fault owns the power_fault and power_restored flags.
 static constexpr double SkyFaultMinutes=180; // Tom 12 Nov: about 12 real minutes at night (night rate ~245 s per game hour)
 double SkyFaultClock() const { return SkyFaultUntil; }
 void SkyCutBegin(double Sag,double Out);
 bool SkySagging() const { return SkySag>0; }
 bool SkyMainsOut() const { return SkySag<=0&&SkyOut>0; }
 bool SkyCutLive() const { return SkySag>0||SkyOut>0; }
 bool SkyRode() const { return bSkyRode&&SkyDark<12; }
 float SkyLevel() const;
 bool HasPowerForReactiveLight() const { return HasPower(); }
 void SetAnomaly(bool bValue);
 bool Anomaly() const { return bAnomaly; }
 // CHAIN-1-20261109: something is out on the land tonight that a bearing may still point at (FS47Happenings::OutTonight): the
 // pulsar card run tonight then shows a discrepancy by itself, and registering it prints the slip toward that place.
 bool OutThereTonight() const;
 void SetPhenomenon(const FString& Id) { if(PhenomenonId!=Id)UndecideTakes(); PhenomenonId=Id; Changed(); }
 // Critic 12 Nov: an instrument fault set or cleared is a new question for what the tuned receiver shows (FJob::TakeDecided).
 void UndecideTakes() { for(FJob& W:Work)if(W.Stage==EJobStage::Pointed)W.TakeDecided=-1; }
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
 // CASES-OPS-20261109: the header's left side, as Story's operations system has it ("SARO OPS V4.2  TERM 1  OPR NIGHT"), and the
 // calendar date of a clock ("28-APR-86").
 static FString TerminalStation(int32 Screen);
 FString TerminalDate(double AtClock) const;
 // The procedure card taped to the desk by the OPS terminal: what the screens used to explain, to be read when wanted.
 static FS47Doc SopCard();
 // OPS's own system lines (S47FreeformScreens.cpp): the queue ("MORE WORK / 2 OPEN TODAY / 1 FROM EARLIER / NEW JOBS 08:00") and the
 // account on the glass ("BUDGET $31 / REQ $12"). REQUISITION, never debt.
 static FString QueueLine(int32 Remaining,int32 Today,int32 Earlier,int32 Rejected);
 static FString OpsAccountLine(int32 Budget,int32 Requisition);
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
 // D45-20261110: how near a plot position (az 0..180, el 0..90) is to the source: 0 far off, 1 inside AimOnTarget's tolerance
 double AimNearness(FVector2D AzEl) const;
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
 FString WorkActivities(FString* Card) const; // AvailableActivities without the letter
 FVector2D Aim=FVector2D::ZeroVector;FString AimJob;bool bAimLocked=false;
 FString ArrayJob; // D48-20261110: the job whose pointing the array and receiver last held (saved "arrayJob"; an older save has none)
 void ReleaseForNewJob(); // D48-20261110: a new job on the same target as the last starts from a released array and a stood-down receiver
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
 double SkySag=0,SkySagAll=1,SkyOut=0,SkyDipAt=0,SkyDip=0,SkyDark=0;bool bSkyRode=false;int32 SkyTold=0;
 double SkyFaultUntil=0; // FIX10-10: game clock at which the county restores a lockout left by a passage; 0 = none
 void StepSky(double Dt);
 FString ArchiveNote;
 // RECORDS-BINDER: the shift day and clock each packet went into the binder, by job id (saved "binderFiled").
 struct FBinderStamp{int32 Day=0;double Clock=0;};
 TMap<FString,FBinderStamp> BinderFiled;
 void ShowBinder();
 bool FileAtBinder(FString& Words);
 void BinderRead(const FString& EntryKey);
 void LoadBinder(const S47J::FObj& Saved);
 void SaveBinder(const S47J::FObj& Record) const;
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
 FString TerminalTitle; // FIX-20261110: the page on the glass (a new one is heard arriving)
 int32 ServiceGuide=0,GuideServiceBaseline=0,GuideSupplyBaseline=0;
 // First shift tutorial (0 done/off, 1 accept a job at OPS, 2.. follows the job to filing).
 int32 Tutorial=0;FString TutorialJobId;void StepTutorial();FString TutorialObjective()const;
 int32 FieldFocus=-1;
 // The case board's "file anyway?" for a filing with nothing behind it yet (critic round 5): its line and cause, -1 none. Not saved.
 int32 FileAskLine=-1,FileAskCause=-1;
 void ClearCompletedSelection();
 bool bGuideSupply=false,bGuideFocus=false;
 int32 DayNumber=1,Cash=120,Loan=0,Selected=0,AudioSourceIndex=0,SelectedDish=0;
 double WorldClock=8*3600,Settle=0,AudioCursor=0,AutosaveAt=0,IntegrationPulse=0;
 int32 WokenNight=0;
 int32 RousedNight=0; // the last night a rest was really broken: never two running
 bool bTestEvening=false,bTestFirstPulse=false,bTestRules=false;
 int32 ReviewWeek=0,Carried=0,StandbyTotal=0,LastPay=20;
 // M7 (Tom, 9 November): the week the side money was counted for (1-based), what it has paid that week, and repairs for the next sheet.
 int32 SideWeek=0,SidePaid=0,Repairs=0;
 FString DeskLine;
 int32 DeskNight=0,DeskCards=0;
 void DeskWork(const FJob& J);
 TSet<FString> Kit;
 // N1110 (PLAY-31-34): the control room's ceiling tubes that have gone out for good in this game (indices 2 to 7 of room.tubes; 0 and 1 are the
 // working light and never go). Saved "brokenTubes", absent = none; AS47Web::TubeLit keeps them dark through a power return, Continue and a change of area.
 TSet<int32> BrokenTubes;
 // PLAY-31-34 (14 November, Tom's «Vurder»): those of BrokenTubes that burned out of age (FS47Calm's tube_wear, from week 2): the day
 // shift puts new ones in at the next 08:00 (AddDay), and Dale says a dry word that evening (tube_wear_dale). Saved "wornTubes",
 // "tubeWornDay", "tubeRemarkDay"; absent = none.
 TSet<int32> WornTubes;
 int32 TubeWornDay=0,TubeRemarkDay=0;
public:
 bool TubeBroken(int32 K) const{return BrokenTubes.Contains(K);}
 int32 BrokenTubeCount() const{return BrokenTubes.Num();}
 // True when it was whole until now. Only tubes 2 to 7.
 bool BreakTube(int32 K){return K>=2&&K<=7&&!BrokenTubes.Contains(K)&&(BrokenTubes.Add(K),true);}
 // N1110: the next N still-working tubes of 2 to 7, lowest first (what every sabotage puts out: {"beat":"tube","next":2}); nothing is changed.
 TArray<int32> NextTubes(int32 N) const{TArray<int32> Out;for(int32 K=2;K<=7&&Out.Num()<N;++K)if(!BrokenTubes.Contains(K))Out.Add(K);return Out;}
 // N1110: a pair of tubes from the store (PARTS STORAGE) puts back the last N that went out, highest first. Returns how many came back.
 int32 MendTubes(int32 N){int32 Mended=0;for(int32 K=7;K>=2&&Mended<N;--K)if(BrokenTubes.Remove(K)){WornTubes.Remove(K);++Mended;}return Mended;}
 // PLAY-31-34 (14 November): a tube burned out of age on shift day Day: dark as a broken one (TubeLit), until the day shift.
 bool WearTube(int32 K,int32 Day){if(!BreakTube(K))return false;WornTubes.Add(K);TubeWornDay=Day;return true;}
 bool TubeWorn(int32 K) const{return WornTubes.Contains(K);}
 int32 WornTubeCount() const{return WornTubes.Num();}
 // At the turn to shift day Day (08:00): the day shift replaces what burned out before it; returns how many. Dale remarks that evening.
 int32 DayShiftTubes(int32 Day){if(WornTubes.IsEmpty()||Day<=TubeWornDay)return 0;const int32 N=WornTubes.Num();for(const int32 K:WornTubes)BrokenTubes.Remove(K);WornTubes.Reset();TubeWornDay=0;TubeRemarkDay=Day;return N;}
 bool TubeRemarkDue(int32 Day) const{return TubeRemarkDay>0&&TubeRemarkDay==Day;}
 void TubeRemarkSaid(){TubeRemarkDay=0;}
 static TSet<int32> TubesFrom(const TArray<double>& Saved);
 static TArray<double> TubesToSave(const TSet<int32>& Broken);
private:
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
 // LOOP-WHOLE-20261109, step 2: a record that answers the land's planned control: the trip camera read in the morning (its own frames
 // say yes or no), a magnetometer reading at a place out there (also one already kept), the field logger's tape back (StepLandTest).
 void LandRecord(const FString& Id);
 void StepLandTest();
 void RecordCaseResult(FJob& J);
 void StepCaseReplies();
 bool StationSealed() const;
 bool CaseObserved(const FJob& J) const;
 FString CasePrecondition(const FJob& J) const;
 // CASETEST-20261109: a broken condition stops the recording and keeps the plan on the same job; a relay fitted for the timing swap
 // goes back to the depot when its recording stops short; Continue observation prefers a case test that can be recorded now.
 void SpoilCaseTest(FJob& J,const FString& Why);
 void UnfitCaseRelay(const FJob& J);
 int32 ContinueJob() const;
 int32 CaseAwaitingChoice() const;
 void IngestObservations();
 int32 IngestedObservations=0;
 int32 OpsSection=0,ReceiverTab=0;
 // RX-ACQUIRE-SKY-20261113: the job the chart's search is for, the job and state the receiver last chose its page for (a tab picked
 // by hand stays), when the page goes over to TUNE after a lock (real seconds; -1 none), and the sounds read once. None of it saved.
 bool bTestSky=false,bSkySoundsTried=false;FString SkyFixJob,SkyTabKey;double SkyHandAt=-1;
 void SyncSkyFix();void SkyChartStep(double Dt);void SkyHeld(FJob& HeldJob,bool bByHand);void StepSkyFix();
 void LoadSkySounds();void SkySound(uint32 Events,int32 Blip);
 FString SkyPrompt() const;
 // (RX-LOOK-20261113: the page reads the frame's feed, and fills the body height the frame tells it)
 TSharedRef<class SWidget> SkyPage(TSharedPtr<class SWidget>& KeysTo,TSharedRef<FS47RxFeed> Feed,TSharedPtr<float> BodyHigh);
 int32 DataRef=0; // D48-20261110 (D46-16): the reference the receiver's DATA page shows beside the take; not saved
 // D45-20261110: ARRAY STATUS's tab (0 STATUS, 1 MAINTENANCE) and the fault picked on MAINTENANCE; presentation only, never saved
 int32 StatusTab=0;FString MaintPick;
 FString LastPage[4]; // N1110 PC-WORLD-DISPLAY-04: the page each PC was last left on, shown on its idle glass; presentation only, never saved
 TSharedRef<class SWidget> MaintenancePage();
 FString MaintenanceLine() const; // the array PC's line across the room: "MAINTENANCE / 3 FAULTS / NEXT S-13"
 // CASES-OPS-20261109: the result sheets on the printer (FResultSheet), the case the next OPS visit opens on (-1: the overview), the
 // test whose sheet was last collected (its row is marked on the board), and whether the board's traces are folded (presentation only).
 TArray<FResultSheet> CaseSheets;
 int32 BoardLine=-1,FreshLine=-1,FreshTest=-1;bool bCaseDetails=false;
 int32 BoardPick=-1,BoardPickLine=-1; // D48-20261110: the test card selected on the board, and the case it belongs to (presentation only)
 void QueueResultSheet(int32 Line,int32 Test,const FString& Job);
 bool CollectResultSheet();
 int32 SheetIndex() const;
 TWeakObjectPtr<class UStaticMeshComponent> SopProp;TWeakObjectPtr<class AS47Web> SopScene;
 void StepSopCard();
 void StopSopCard();
 // TELEX-20261109: the sheets on the machine (the first feeds first) and the ones torn off, newest last; the paper on the machine and
 // its sound (not saved: built again from the sheets).
 TArray<FTelexSheet> TelexSheets,TelexTorn;
 TWeakObjectPtr<class UStaticMeshComponent> TelexPaper;TWeakObjectPtr<class AS47Web> TelexScene;
 bool bTelexMotor=false;int32 TelexStrikes=0;
 void StepTelex(double Dt);
 void StopTelex();
 void LoadTelex(const S47J::FObj& Saved);
 void SaveTelex(const S47J::FObj& Out) const;
 // LOOP-3-20261109: the array's own log, when each night record was kept, and what has been asked (S47FreeformMinute.cpp).
 TArray<FArrayMark> ArrayMarks;
 TMap<FString,double> NightClocks;
 TArray<FString> Asked; // newest last, at most MostAsked; the YES/NO records are the lasting mark (AskedSameMinute)
 // CRIT9: the list's chores that simply stop when done (the post, the truck, the feeder, the tank, the relay): by kind, the line last
 // open tonight, and the one gone from it since (shown ticked off until 08:00, as the note's header promises). In memory only.
 mutable TMap<FString,FString> ChoresOpen,ChoresDone;mutable int32 ChoresNight=0;mutable double ChoresClock=-1;
 mutable FString PickRow; // TASKS-TEN-20261113: the row the ladder's last answer is (ObjectiveRow)
 TArray<FDaleAsk> DaleAskList; // TASKS-TEN-20261113 (change 7)
 FString PinKey,PinText;int32 PinNight=0;bool bPinLive=false;double PinCheckAt=-1; // TASKS-TEN-20261113 (change 10)
 void StepPin();
 void StepDaleAsks();
 void LoadNotePlan(const S47J::FObj& Saved);
 void SaveNotePlan(const S47J::FObj& Out) const;
 void NoteChore(TArray<FS47Task>& Result,const TCHAR* Kind,const FString& OpenLine,const FString& DoneLine) const;
 double PrimeBefore=-1; // an older save: what came back before this clock is not asked again (-1: nothing to prime)
 int32 AskLook=-1;
 void NoteArrayRecord(const FString& Id,const FString& Text,bool bKeptNow=true);
 void LoadMinutes(const S47J::FObj& Saved);
 void SaveMinutes(const S47J::FObj& Out) const;
 // Tom, 9 November: the first case opens when its job is registered at OPS, not at the reel deck's RETAIN. True while the interval
 // case is not open and a reviewed pulsar comparison still waits for its registration: what the reel deck and the event retain then
 // does not open it (OpenCaseFor, Begin); RegisterComparison does (OpenFirstCase).
 bool FirstCaseHeld() const;
 bool OpenFirstCase(const FJob& J);
 TSharedRef<class SWidget> OpsShell(int32 Section,TSharedRef<class SWidget> Content);
 void PhotoDeskRows(TArray<TSharedRef<class SWidget>>& Rows);void PhotoPickSheet();
 bool FieldRecordDue(const struct FS47AlbumShot& P) const;bool PhotoDeskDue() const;
 FString PhotoPick;int32 PhotoPage=0;
 TArray<int32> MysterySteps;
 TArray<bool> Upgrades;
 FString Feedback, PhenomenonId;
 TArray<double> SavedAngles;
 bool bRestoreAngles=false;
 FJob* Job();
 void AddDay(int32 Number);
 int32 LapseUntakenCards(); // CHAIN-5-20261109: at the turn of the day (not TestAdvance) cards nobody began fall away
 void Changed(const FString& Text=FString());
 void UpdateMysteries();
 bool PointReady() const;
 bool ReceiverReady() const;
 FString ReceiverNextAction() const;
 // RX-TABS-DUE-20261113 (Tom 13 Nov: the receiver tab to use next blinks): the receiver tab that holds the next step, read from the
 // same predicates and in the same order as ReceiverNextAction (and CanRecordControl after the take); -1 when there is nothing to do
 // at this receiver, or the step's tab is the one already open. Tabs: 0 ACQUIRE, 1 TUNE, 2 ARRAY, 3 RECORD, 4 DATA.
 enum ERxTab : int32 { RxAcquire=0, RxTune=1, RxArray=2, RxRecord=3, RxData=4 };
 struct FRxDueIn
 {
  bool bJob=false,bGated=false,bTutorialGate=false,bPower=true,bCaseWaits=false,bArrayBlocked=false;
  bool bPointReady=false,bSettled=false,bSky=false,bTuned=false,bCrossRunning=false,bCanControl=false,bDataAsk=false;
  bool bAnomalyAhead=false,bPlainTake=false,bRetain=false; // RX-ANOMALY-FLOW-20261113: the extra-tick take is RECORD's
  EJobStage Stage=EJobStage::New;int32 Open=-1;
 };
 static int32 RxDueTab(const FRxDueIn& In);
 int32 ReceiverDueTab() const;
 bool RxDataAsk(const FJob& J) const; // an unusual take on tape with its case open and comparisons left, its DATA page not yet seen
 FString RxDataSeenJob; // not saved: the job whose DATA page was opened
 mutable uint64 RxDueFrame=~0ull;mutable int32 RxDueCache=-1;
 // The OPS PC's DAILY WORK line is due while the job's next step is an OPS one there (confirm, register, compare): JobNextAction's own words.
 static bool OpsWorkDueFor(EJobStage Stage,const FString& Next);
 bool OpsWorkDue() const;
 bool EncodersAt(double Az,double El) const;
 void PointBeamControl();
 void CollectDuplicate();
 FString ControlSummary(const FJob& J) const;
 bool HasPower() const;
 bool RequirePower();
 double TargetAz() const;
 double TargetEl() const;
 double SourceSeconds() const;
 void PointCross();
 void StepWork(double Dt);
 void StepAudio(double Dt);
 // FIX-20261110 (Tom: the array PC has too little to do): the drive check. The left-hand PC runs every element's drive against a
 // dummy load, one after the other, and says what it heard: a relay that will not last the day (FaultDueLater: the fault the
 // station round catches with a relay in stock), one that is creeping for tomorrow, or nothing. Nothing new is decided here.
 void StartDriveCheck();void StepDriveCheck(double Dt);void FinishDriveCheck();
 double DriveCheckAt=-1;int32 DriveCheckDay=0,DriveCheckDish=-1,DriveCheckTone=0;FString DriveCheckResult;
 void TransportSound(); // FIX-20261110: the reel deck's motor, rewind, tape change and marks, heard
 bool bTransportPrimed=false,bTransportRun=false;int32 TransportSource=0;double TransportCursor=0,TransportMark=0;
 double ReceiverReception() const;
 // D45-20261110 (Tom: the target shows on the receiver only once the array has locked): how much of the source the receiver shows
 // and plays while the array comes round (wide, so it grows over the slew), and the largest dish error left. The recording and
 // its gate keep ReceiverReception, PointReady and Settle.
 double ReceiverApproach() const;
 double ArrayErrorDeg() const;
 void PrintJob(bool bPacket,bool bShow=true);
 // GAMEPLAY-LOOP-01: what StartAcquisition decides, without deciding it (Phen: the phenomenon it would carry; bOutThere: from the land).
 bool TakeAnomalous(const FJob& J,FString& Phen,bool& bOutThere) const;
 // LOOP-ANOMALY-ANY-CARD-20261113: whether J may carry tonight's out-there take (card 2 only before AnyCardFrom), and whether another
 // take tonight already does (decided while pointed, or recorded tonight with no instrument cause).
 bool OddCarries(const FJob& J) const;
 bool OutTakeTonight(const FJob& Except) const;
 // The take's decision kept on the job (FJob::TakeDecided): TakeAnomalous once, then what it said.
 void DecideTake(FJob& J);
 bool DecidedTake(const FJob& J,FString& Phen,bool& bOutThere) const;
 void RetainOriginal(FJob& J); // the original page kept as the take ends, with no chart to fetch
 bool ConfirmOnTape(FJob& J); // the reel deck's one listen: marks the ticks if they are not, keeps the review, names OPS
 // Critic 12 Nov: an extra-tick take waiting for its one listen at the deck; and the deck threading the oldest of them when the
 // selected job is not one (OpenAudio, OpenTapePanel).
 static bool TapeWaits(const FJob& J);
 void FollowTapeJob();
 FString DetectedJob; // the job whose ANOMALY DETECTED was last said aloud (once a take; not saved)
 void SetPanel(TSharedRef<class SWidget> Widget,bool bLive=false,bool bAudio=false);
 // KeysTo (RX-ACQUIRE-SKY-20261113): the widget on the page that has the keys first; none: the page itself, as before.
 void OpenTerminal(int32 Screen,const FString& Title,TSharedRef<SWidget> Body,bool bLive=false,TSharedPtr<SWidget> KeysTo=nullptr);
 // RX-LOOK-20261113: OpenTerminal's two halves, for a screen that brings its own frame (the receiver's, S47FreeformTerminal::RxFrame).
 bool TerminalBegin(int32 Screen,const FString& Title);
 void TerminalShow(int32 Screen,TSharedRef<SWidget> Content,bool bLive,TSharedPtr<SWidget> KeysTo,bool bOwnKeyRow);
 void OpenWaitConfirmation(double Target);
 FS47Profile EffectiveProfile(const FJob& Job) const;
 FString JobLine(const FJob& J) const;
 FString Reference(const FJob& J) const;
 static FS47Profile Profile(int32 Source);
};
