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
struct FS47Doc;

// Separate, persistent SARO working life. Story progression never grants its jobs or facts.
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
 void OpenArchive();
 void OpenPhotos(); // FIX-20261110: OPS / PHOTOGRAPHS, the film record and the allowance desk
 int32 ArchiveTab=0,ArchivePage=0;FString ArchivePick; // presentation only, never saved
 uint32 StationFilesSeen=0;bool StationFileDue() const; // FIX-20261110: the station's files (S47StationFiles), a new one marks RECORDS
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
 // control. A good card pays $19 against the old $20; a sloppy late one $10 or less. Quality pays better than quantity.
 static int32 PayAfterDays(int32 Days){return Days<=1?14:Days==2?10:Days==3?7:4;}
 static constexpr int32 OnTimeBonus=2,CleanBonus=3,ControlBonus=1;
 static constexpr double CleanRecordQuality=.6;
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
 // N1110 (N1110-1, N1110-2): how much the player has been bothering them, and what comes of it (S47Attention.h). Saved "attention".
 TSharedPtr<class FS47Attention> Attention;
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
 // the case file and the work list as they are (nothing saved). The OPS banner and the HUD (Objective, HudCaseLine), the task list
 // (Tasks), TONIGHT'S PLAN and the pause menu's note, the notebook's Now and the case board's star (CaseAwaitingChoice) all read it.
 // CHOOSE_TEST: a test (or control) can be planned; for a mystery, or a filing. RUN_PLAN: the planned test can be run now.
 // WAIT_WINDOW: planned, its dated window not open yet. CONCLUDE: a trial case with one explanation left. FILE: a mystery with one
 // left, its trail cold or no control to be had. AWAIT_REPLY: filed, the morning's reply not in, or in on the telex and not torn off
 // yet (TELEX-20261109). CLOSED: nothing (or not open; the interval case opens when its comparison is registered at OPS).
 enum class ECaseStep : uint8 { Closed, ChooseTest, RunPlan, WaitWindow, Conclude, File, AwaitReply };
 struct FCaseStep
 {
  ECaseStep Step=ECaseStep::Closed;
  FString Next; // the whole step, as the objective says it: "OPS / CASE BOARD: REGISTER THE SUPPORTED CONCLUSION FOR ..."
  FString Card; // the HUD's card when the case is the objective: "CASE BOARD // ..."
  FString Short;// the HUD card's second line: "THE SAME MINUTE: REPLY DUE 08:00."
  FString Task; // the task list's line: "CASE / THE SAME MINUTE: file your assessment on the OPS case board"; empty: not a task
 };
 FCaseStep CaseStep(int32 Line) const;
 // The open case whose step is most pressing: CONCLUDE, RUN_PLAN, FILE, CHOOSE_TEST, then (bActionOnly false) WAIT_WINDOW and
 // AWAIT_REPLY; the lower line first among equals. -1: none.
 int32 UrgentCase(bool bActionOnly=false) const;
 // The HUD card's second line while a case is open: the most pressing case not already on the card's first line. Empty: none.
 FString HudCaseLine(const FString& FirstLine) const;
 // A case record kept (FS47FreeformCases::IsCaseRecord) as one line under the night log's WORK: "Case 1, THE REPEATED INTERVAL:
 // off-source pointing registered; rules out Follows the sky source." Empty for any other finding.
 FString CaseRecordLine(const FString& Key) const;
 // A planned case test that cannot run before its dated window (the repeat): waiting, not open work. It is not counted with the
 // cards waiting and does not hold back the reference check.
 bool CaseTestWaits(const FJob& Job) const;
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
 bool QueueBearingSlip(int32 Day,const FString& Line,const FString& Note);
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
 // D45-20261110 (Tom: record the control at the receiver right after the take): an unusual take that is on tape (its chart may
 // still be at the printer) and has no control yet can have its 8-second control recorded now. The old order stays legal.
 bool CanRecordControl() const;
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
 // get to the standby generator), then the mains are out for Out seconds and come back by themselves. A running generator carries
 // the station through it, and the line recorder keeps writing (SkyRode: the record the passage leaves). Nothing of it is saved:
 // a passage is one sitting, and a save opened in the middle of one has its mains.
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
 // CASES-OPS-20261109: the header's left side, as Story's operations system has it ("SARO OPS V4.2  TERM 1  OPR NIGHT"), and the
 // calendar date of a clock ("28-APR-86").
 static FString TerminalStation(int32 Screen);
 FString TerminalDate(double AtClock) const;
 // The procedure card taped to the desk by the OPS terminal: what the screens used to explain, to be read when wanted.
 static FS47Doc SopCard();
 // OPS's own system lines (S47FreeformScreens.cpp): the queue ("MORE WORK / 2 OPEN TODAY / 1 CARRIED / NEXT ISSUE 08:00") and the
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
 void StepSky(double Dt);
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
public:
 bool TubeBroken(int32 K) const{return BrokenTubes.Contains(K);}
 int32 BrokenTubeCount() const{return BrokenTubes.Num();}
 // True when it was whole until now. Only tubes 2 to 7.
 bool BreakTube(int32 K){return K>=2&&K<=7&&!BrokenTubes.Contains(K)&&(BrokenTubes.Add(K),true);}
 // N1110: the next N still-working tubes of 2 to 7, lowest first (what every sabotage puts out: {"beat":"tube","next":2}); nothing is changed.
 TArray<int32> NextTubes(int32 N) const{TArray<int32> Out;for(int32 K=2;K<=7&&Out.Num()<N;++K)if(!BrokenTubes.Contains(K))Out.Add(K);return Out;}
 // N1110: a pair of tubes from the store (PARTS STORAGE) puts back the last N that went out, highest first. Returns how many came back.
 int32 MendTubes(int32 N){int32 Mended=0;for(int32 K=7;K>=2&&Mended<N;--K)if(BrokenTubes.Remove(K))++Mended;return Mended;}
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
 int32 DataRef=0; // D48-20261110 (D46-16): the reference the receiver's DATA page shows beside the take; not saved
 // D45-20261110: ARRAY STATUS's tab (0 STATUS, 1 MAINTENANCE) and the fault picked on MAINTENANCE; presentation only, never saved
 int32 StatusTab=0;FString MaintPick;
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
 void PrintJob(bool bPacket);
 void SetPanel(TSharedRef<class SWidget> Widget,bool bLive=false,bool bAudio=false);
 void OpenTerminal(int32 Screen,const FString& Title,TSharedRef<SWidget> Body,bool bLive=false);
 void OpenWaitConfirmation(double Target);
 FS47Profile EffectiveProfile(const FJob& Job) const;
 FString JobLine(const FJob& J) const;
 FString Reference(const FJob& J) const;
 static FS47Profile Profile(int32 Source);
};
