#include "S47Freeform.h"
#include "S47DirectorCam.h"
#include "S47DevWaves.h"
#include "S47SaroWallWatch.h"
#include "S47GoalLine.h"
#include "S47StoryDirector.h"
#include "S47RuleBreak.h"
#include "S47IdleLure.h"
#include "S47Balloon.h"
#include "S47SaroSave.h"
#include "S47SaroNight.h"
#include "S47StationFiles.h"
#include "S47SaroPower.h"
#include "S47RoomLights.h"
#include "S47Image.h"
#include "S47FreeformAudioDesk.h"
#include "S47RoomDecor.h"
#include "S47FarmSite.h"
#include "S47Taken.h"
#include "S47DeputyRound.h"
#include "S47NightLog.h"
#include "S47Tape.h"
#include "S47Handling.h"
#include "S47SaroCot.h"
#include "S47Hide.h"
#include "S47Attention.h"
#include "S47DoorLoop.h"
#include "S47PinBoard.h"
#include "S47Enlarger.h"
#include "S47Darkroom.h"
#include "S47WardReport.h"
#include "S47Answer.h"
#include "S47LeftGear.h"
#include "S47LocationCards.h"
#include "S47Openings.h"
#include "S47Outfit.h"
#include "S47Places.h"
#include "S47Witness.h"
#include "S47EventPlaces.h"
#include "S47Predecessor.h"
#include "S47Mail.h"
#include "S47RadioTips.h"
#include "S47TownSites.h"
#include "S47TownUse.h"
#include "S47Townsfolk.h"
#include "S47Visitors.h"
#include "S47MotelNight.h"
#include "S47FieldLife.h"
#include "S47NightRhythm.h"
#include "S47AlienKinds.h"
#include "S47ScriptPlayer.h"
#include "S47Aftermath.h"
#include "S47FarmNight.h"
#include "S47IndoorDread.h"
#include "S47FarmHouse.h"
#include "SS47TapeScope.h"
#include "S47FreeformTerminal.h"
#include "S47GameStyle.h"
#include "S47ReceiverReadout.h"
#include "S47ReceiverSound.h"
#include "S47RxSearch.h"
#include "SS47ScreenView.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47FieldRadio.h"
#include "S47HudGauges.h"
#include "S47TonightNote.h"
#include "S47CaseFieldTests.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Flashlight.h"
#include "S47Album.h"
#include "S47FieldCamera.h"
#include "S47Story.h"
#include "S47Web.h"
#include "S47UI.h"
#include "S47Kit.h"
#include "S47Docs.h"
#include "S47Audio.h"
#include "S47Walker.h"
#include "S47World.h"
#include "S47Flow.h"
#include "S47Happenings.h"
#include "S47WeekCase.h"
#include "S47Fatigue.h"
#include "S47WeatherLive.h"
#include "S47Observing.h"
#include "S47Inter.h"
#include "S47Keys.h"
#include "S47Three.h"
#include "S47Thoughts.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateColorBrush.h"

namespace
{
 constexpr double SecondsPerDay=86400., RealDay=1800., RealNight=2700.;
 constexpr double PulsarPeriod=1.1879;
 bool UseCapturedSound(const S47CaptureSound::FTrack* Capture,bool NoSound) { return !NoSound&&Capture&&Capture->Playable(); }
 double SafeNum(const S47J::FObj& O,const TCHAR* K,double D=0) { const double V=S47J::Num(O,K,D);return FMath::IsFinite(V)?V:D; }
 int32 SafeInt(const S47J::FObj& O,const TCHAR* K,int32 D=0) { return int32(FMath::Clamp(SafeNum(O,K,D),-100000000.,100000000.)); }
 const double Ticks[]={9.30,9.48,9.66,9.84,11.40,11.60,11.80,12.00,12.20,12.40,12.60};
 const FVector2D Cross[]={{0,.5},{0,-.5},{.5,0},{-.5,0},{0,0}};
 const double BeamOffsets[]={-.5,.5,0};
 bool Specialized(const FString& Id){return Id==TEXT("wrong_source")||Id==TEXT("time_disagreement")||Id==TEXT("duplicate_print");}
 bool MarkCoversTicks(double In,double Out){return In>=0&&In<=9.30&&Out>=12.60&&Out-In<=6;}
 // D45-20261110 (Tom: compare and retain looked the same every time): one tape's own look on the displays. The same tape keeps its
 // trace; another job, day or tape (original 0, control 1) has another one. Nothing of it is saved: it follows from the job.
 uint32 TakeSeed(const FS47FreeformSession::FJob& J,int32 Tape){return HashCombine(GetTypeHash(J.Id),GetTypeHash(Tape*7919+J.Day));}
 double BeamLevel(double ActualAz,double SourceAz)
 {return .08+.92*FMath::Exp(-FMath::Square(FMath::FindDeltaAngleDegrees(ActualAz,SourceAz)/.35));}
 FString InstrumentConclusion(const FString& Id)
 {return Id==TEXT("time_disagreement")?TEXT("timing_path"):Id==TEXT("duplicate_print")?TEXT("duplicate_mismatch"):TEXT("off_source");}

 bool JsonVersion(const S47J::FObj& O)
 {
  const auto V=O?O->TryGetField(TEXT("v")):nullptr;
  return V && V->Type==EJson::Number && V->AsNumber()==1.;
 }
 bool EncoderChanged(const FVector2D& Before,const FVector2D& After)
 {
  return !Before.ContainsNaN()&&!After.ContainsNaN()&&
   (FMath::Abs(FMath::FindDeltaAngleDegrees(Before.X,After.X))>1e-5||FMath::Abs(Before.Y-After.Y)>1e-5);
 }
 using ECaseCondition=FS47FreeformCases::ECondition;
 ECaseCondition CaseCondition(int32 Line,int32 Test){return FS47FreeformCases::Def(Line).Tests[FMath::Clamp(Test,0,FS47FreeformCases::TestCount-1)].Condition;}
 FString CaseTestName(int32 Line,int32 Test){return FS47FreeformCases::Def(Line).Tests[FMath::Clamp(Test,0,FS47FreeformCases::TestCount-1)].Name;}
 // M7's late lines with no control to choose (UnavailableReason): the place still to look at, else the filing. Empty: any other line.
 FString LateStep(int32 Line,const TMap<FString,FString>& Kept)
 {
  if(Line<FS47FreeformCases::LateFirst||FS47FreeformCases::UnavailableReason(Line,0).IsEmpty())return FString();
  if(const auto* Y=FS47FreeformCases::Def(Line).Mystery)for(const auto& Clue:Y->Clues)if(!Kept.Contains(Clue.Finding)){FString Place=Clue.Place;if(Place.Len()>1&&FChar::IsLower(Place[1]))Place[0]=FChar::ToLower(Place[0]);return TEXT("look at ")+Place+TEXT(", or file your assessment");}
  return TEXT("file your assessment");
 }
 // CRIT6-20261109: the pay line's reasons as English has them: "a", "a and b", "a, b and c".
 FString JoinAnd(const TArray<FString>& L)
 {
  if(L.Num()<=1)return L.Num()?L[0]:FString();
  FString Out;for(int32 I=0;I<L.Num();++I)Out+=(I==0?FString():I==L.Num()-1?FString(TEXT(" and ")):FString(TEXT(", ")))+L[I];
  return Out;
 }
 // The desk line after a card (CRIT6-20261109): no minutes when the card's start is not known, none either when it began on an
 // earlier night or ran over four hours; the hours left to 06:00 only while the night lasts.
 FString DeskText(bool bKnown,bool bEarlier,double Minutes,int32 Tonight,double Hour)
 {
  FString Took;
  if(bKnown&&bEarlier)Took=TEXT(" That card ran on from an earlier night;");
  else if(bKnown&&Minutes>240)Took=TEXT(" That card took more than four hours of the night;");
  else if(bKnown){const int32 Min=FMath::Max(0,int32(Minutes));Took=FString::Printf(TEXT(" That card took %d %s of the night;"),Min,Min==1?TEXT("minute"):TEXT("minutes"));}
  const FString Left=Hour>=6&&Hour<12?FString(TEXT("the night is over")):Hour>=12&&Hour<18?FString(TEXT("the night still to come")):FString::Printf(TEXT("%.1f hours left to 06:00"),FMath::Max(0.,(Hour>=12?30.:6.)-Hour));
  return Took+FString::Printf(TEXT(" %d %s tonight, %s.%s"),Tonight,Tonight==1?TEXT("card"):TEXT("cards"),*Left,Tonight>=3?TEXT(" A third card at the screens: your eyes feel it."):TEXT(""));
 }
 FString StageText(FS47FreeformSession::EJobStage Stage)
 {
  static const TCHAR* Names[]={TEXT("UNSTARTED"),TEXT("POINTED"),TEXT("ACQUIRING"),TEXT("AT PRINTER"),TEXT("RECORDED"),TEXT("REGISTERED"),TEXT("PACKET COLLECTED"),TEXT("FILED")};
  return Names[FMath::Clamp(int32(Stage),0,7)];
 }
}
double FS47FreeformSession::AdvanceClock(double Clock,double ActiveSeconds)
{
 if(!FMath::IsFinite(Clock)||Clock<0)return 8*3600.;
 if(!FMath::IsFinite(ActiveSeconds)||ActiveSeconds<=0)return Clock;
 // Convert through elapsed active time rather than choosing one rate for the
 // whole frame. This preserves the exact 75-minute cycle across all boundaries.
 constexpr double Dawn=6*3600., Dusk=19*3600.;
 constexpr double NightRate=11*3600./RealNight,DayRate=13*3600./RealDay;
 constexpr double BeforeDawn=Dawn/NightRate;
 const double Days=FMath::FloorToDouble(Clock/SecondsPerDay),Time=Clock-Days*SecondsPerDay;
 const double Phase=Time<Dawn?Time/NightRate:Time<Dusk?BeforeDawn+(Time-Dawn)/DayRate:BeforeDawn+RealDay+(Time-Dusk)/NightRate;
 const double Total=Phase+ActiveSeconds,ExtraDays=FMath::FloorToDouble(Total/(RealDay+RealNight));
 const double P=Total-ExtraDays*(RealDay+RealNight);
 const double NewTime=P<BeforeDawn?P*NightRate:P<BeforeDawn+RealDay?Dawn+(P-BeforeDawn)*DayRate:Dusk+(P-BeforeDawn-RealDay)*NightRate;
 return (Days+ExtraDays)*SecondsPerDay+NewTime;
}
FS47FreeformSession::FS47FreeformSession(US47Main* Main):M(Main) {}
FS47FreeformSession::~FS47FreeformSession() { Stop(); }

void FS47FreeformSession::Begin(const S47J::FObj& Saved,bool bLethal,bool bOneNight)
{
 Stop();
 bOldCalendar=Saved.IsValid()&&!Saved->HasField(TEXT("calendar"));
 bActive=true;bDeadly=bLethal;bDead=bPowerFault=bAnomaly=bTelemetry=bAudioOpen=bAudioPlaying=bRecovering=false;
 SkySag=SkyOut=SkyDip=SkyDipAt=SkyDark=SkyFaultUntil=0;bSkyRode=false;SkyTold=0;StationFilesSeen=0;
 ServiceGuide=GuideServiceBaseline=GuideSupplyBaseline=0;bGuideSupply=bGuideFocus=false;
 // A new Freeform game opens with the first-shift tutorial; dev/test runs opt in with -S47FreeformTutorial.
 Tutorial=!Saved&&M&&M->Flow&&(!M->Flow->bDevRun||FParse::Param(FCommandLine::Get(),TEXT("S47FreeformTutorial")))?1:0;
 DriveCheckAt=-1;DriveCheckDay=DriveCheckDish=DriveCheckTone=0;DriveCheckResult.Reset();TutorialJobId.Empty();FieldFocus=-1;bMailboxStart=false;bStationLocked=bDarkTold=false;
 DayNumber=1;WorldClock=8*3600;Cash=120;Loan=0;Selected=AudioSourceIndex=SelectedDish=0;WokenNight=RousedNight=0;WakeClock=0;ReviewWeek=Carried=StandbyTotal=0;LastPay=20;SideWeek=SidePaid=Repairs=0;
 Settle=CrossSample=AudioCursor=AutosaveAt=IntegrationPulse=0;CrossPoint=0;bCrossRunning=false;ArrayJob.Empty();
 SkyFix.Reset();SkyFixJob.Empty();SkyTabKey.Empty();SkyHandAt=-1; // RX-ACQUIRE-SKY-20261113: no chart, page or hand-over carried from another game
 if(!Saved&&bOneNight){Tutorial=0;WorldClock=17*3600+20*60;} // Story's one night: day 1, from the drive to SARO at 17:20 (Tom 12 Nov: like the old Story; at SARO about 18:00), nothing to teach
 JobSeed=M&&M->Flow&&!M->Flow->bDevRun?int32(FDateTime::UtcNow().GetTicks()^FPlatformTime::Cycles64()):470047;
 Journal.Load(S47J::Obj(Saved,TEXT("journal")));
 LoadNotePlan(Saved); // TASKS-TEN-20261113: what Dale asked, the pinned row ("notePlan"; an older save has none)
 Work.Reset();Findings.Reset();FindingDays.Reset();MysterySteps={0,0,0};Upgrades={false,false,false};SavedAngles.Reset();bRestoreAngles=false;Feedback.Empty();PhenomenonId.Empty();
 LoadBinder(Saved); // RECORDS-BINDER: when each packet went in ("binderFiled"); a save from before has none
 S47LocationCards::Load(this,Saved); // critic 12 Nov: today's location cards ("locationCards"; a save from before has none)
 if(Saved)
 {
  if(!JsonVersion(Saved)) { bActive=false;return; }
  Tutorial=FMath::Clamp(SafeInt(Saved,TEXT("tutorial")),0,6);
  bStationLocked=S47J::Bool(Saved,TEXT("stationLocked"));bDarkTold=!bStationLocked;
  // T16-20/FIX10-10: both added to the record; a save from before has neither and reads as it did (no hint, no check today)
  bMailboxStart=S47J::Bool(Saved,TEXT("mailboxStart"));
  if(const auto Drive=S47J::Obj(Saved,TEXT("driveCheck"));Drive.IsValid()){DriveCheckDay=FMath::Max(0,SafeInt(Drive,TEXT("day")));DriveCheckTone=FMath::Clamp(SafeInt(Drive,TEXT("tone")),0,2);DriveCheckResult=S47J::Str(Drive,TEXT("result"));}
  TutorialJobId=S47J::Str(Saved,TEXT("tutorialJobId"));
  FieldFocus=FMath::Clamp(SafeInt(Saved,TEXT("fieldFocus"),-1),-1,FS47FreeformCases::CaseCount-1);
  JobSeed=int32(FMath::Clamp(SafeNum(Saved,TEXT("jobSeed"),JobSeed),double(MIN_int32),double(MAX_int32)));
  WorldClock=FMath::Clamp(SafeNum(Saved,TEXT("clock"),8*3600),0.,3153600000.);
  // Older saves have no review behind them: nothing is charged for weeks already played.
  ReviewWeek=FMath::Max(0,SafeInt(Saved,TEXT("reviewWeek"),-1));Carried=FMath::Clamp(SafeInt(Saved,TEXT("carried")),0,int32(ArrearsCap));StandbyTotal=FMath::Max(0,SafeInt(Saved,TEXT("standbyPaid")));
  SideWeek=FMath::Max(0,SafeInt(Saved,TEXT("sideWeek")));SidePaid=FMath::Clamp(SafeInt(Saved,TEXT("sidePaid")),0,int32(SideCap));Repairs=FMath::Clamp(SafeInt(Saved,TEXT("repairs")),0,int32(RepairCap));
  WokenNight=FMath::Max(0,SafeInt(Saved,TEXT("wokenNight")));RousedNight=FMath::Max(0,SafeInt(Saved,TEXT("rousedNight")));
  if(!FMath::IsFinite(WorldClock)) {bActive=false;return;}
  DayNumber=FMath::Max(1,int32(FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay))+1);
  if(!Saved->HasField(TEXT("reviewWeek")))ReviewWeek=(DayNumber-1)/7;
  Cash=FMath::Clamp(SafeInt(Saved,TEXT("cash"),120),0,1000080);Loan=FMath::Clamp(SafeInt(Saved,TEXT("loan")),0,1000000);
  ServiceGuide=FMath::Clamp(SafeInt(Saved,TEXT("serviceGuide")),0,2);GuideServiceBaseline=FMath::Max(0,SafeInt(Saved,TEXT("guideServiceBaseline")));GuideSupplyBaseline=FMath::Max(0,SafeInt(Saved,TEXT("guideSupplyBaseline")));bGuideSupply=S47J::Bool(Saved,TEXT("guideSupply"));bGuideFocus=S47J::Bool(Saved,TEXT("guideFocus"),ServiceGuide==1);
  bDeadly=S47J::Bool(Saved,TEXT("lethal"),bLethal);bDead=S47J::Bool(Saved,TEXT("dead"));
  PhenomenonId=S47J::Str(Saved,TEXT("phenomenon"));bPowerFault=S47J::Bool(Saved,TEXT("powerFault"));bAnomaly=S47J::Bool(Saved,TEXT("anomaly"));
  SkyFaultUntil=bPowerFault?FMath::Clamp(SafeNum(Saved,TEXT("skyFaultUntil")),0.,3153600000.):0.;
  // FIX10-10: the station files opened, by key. A save from before has no list: the files released before today count as read
  // (as they did: only the release day was ever marked), so an old game on day 5 is not handed eight marked files at once.
  if(Saved->HasField(TEXT("stationFilesSeen")))
  {
   const TArray<FString> Seen=S47J::Strs(Saved,TEXT("stationFilesSeen"));
   for(int32 I=0;I<S47StationFiles::Count()&&I<32;++I)if(Seen.Contains(S47StationFiles::Get(I).Key))StationFilesSeen|=1u<<I;
  }
  else for(int32 I=0;I<S47StationFiles::Count()&&I<32;++I)if(S47StationFiles::Get(I).Day<DayNumber)StationFilesSeen|=1u<<I;
  ArrayJob=S47J::Str(Saved,TEXT("arrayJob")).Left(64);Settle=FMath::Clamp(SafeNum(Saved,TEXT("settle")),0.,1.);CrossPoint=FMath::Clamp(SafeInt(Saved,TEXT("cross")),0,4);
  CrossSample=FMath::Clamp(SafeNum(Saved,TEXT("crossSample")),0.,2.);bCrossRunning=S47J::Bool(Saved,TEXT("crossRunning"));
  for(const auto& V:S47J::Arr(Saved,TEXT("jobs")))
  {
   if(!V||V->Type!=EJson::Object)continue;const auto O=V->AsObject();if(!O)continue;FJob J;
   J.Phenomenon=S47J::Str(O,TEXT("phenomenon"));J.Id=S47J::Str(O,TEXT("id"));J.Day=FMath::Max(1,SafeInt(O,TEXT("day"),1));J.Slot=FMath::Clamp(SafeInt(O,TEXT("slot")),0,2);J.Source=FMath::Clamp(SafeInt(O,TEXT("source")),0,3);
   J.Stage=EJobStage(FMath::Clamp(SafeInt(O,TEXT("stage")),0,7));J.Elapsed=FMath::Clamp(SafeNum(O,TEXT("elapsed")),0.,40.);J.Quality=FMath::Clamp(SafeNum(O,TEXT("quality"),1),0.,1.);J.ReceivedLevel=FMath::Clamp(SafeNum(O,TEXT("receivedLevel"),1),0.,1.);
   if(const auto Sound=S47J::Obj(O,TEXT("recordingSound"));Sound&&(SafeInt(Sound,TEXT("v"))==1||SafeInt(Sound,TEXT("v"))==2))
   {
    J.RecordingSoundVersion=SafeInt(Sound,TEXT("v"));J.RecordingGain=FMath::Clamp(SafeNum(Sound,TEXT("gain")),0.,100.);
    J.RecordingBandwidth=FMath::Clamp(SafeNum(Sound,TEXT("bandwidth")),0.,100.);J.RecordingQuality=FMath::Clamp(SafeNum(Sound,TEXT("quality")),0.,1.);
   }
   J.bAnomaly=S47J::Bool(O,TEXT("anomaly"));J.bControl=S47J::Bool(O,TEXT("control"));J.bAudioReviewed=S47J::Bool(O,TEXT("reviewed"));J.bPrinted=S47J::Bool(O,TEXT("printed"));J.bPaid=S47J::Bool(O,TEXT("paid"));
   J.ControlAt=FMath::Clamp(SafeNum(O,TEXT("controlAt")),0.,8.);J.OriginalCovered=FMath::Clamp(SafeNum(O,TEXT("originalCovered")),0.,18.);J.ControlCovered=FMath::Clamp(SafeNum(O,TEXT("controlCovered")),0.,8.);
   J.CaptureOriginal.Load(S47J::Obj(O,TEXT("captureOriginal")));J.CaptureControl.Load(S47J::Obj(O,TEXT("captureControl")));
   J.TraceTicks=S47J::Nums(O,TEXT("traceTicks"));J.bLegacyTrace=S47J::Bool(O,TEXT("legacyTrace"),!O->HasField(TEXT("traceTicks")));
   if(!O->HasField(TEXT("traceTicks"))&&J.bAnomaly&&!Specialized(J.Phenomenon))for(double At:Ticks)J.TraceTicks.Add(At);
   if(J.CaptureOriginal.Enabled()&&(J.Source!=1||J.CaptureOriginal.Control||J.CaptureOriginal.Ticks!=J.TraceTicks)){J.CaptureOriginal.Complete=false;J.CaptureOriginal.Interrupt(TEXT("Retained capture does not match this original; compatible reference sound retained."));}
   if(J.CaptureControl.Enabled()&&(J.Source!=1||!J.CaptureControl.Control||Specialized(J.Phenomenon))){J.CaptureControl.Complete=false;J.CaptureControl.Interrupt(TEXT("Retained control capture unavailable; compatible reference sound retained."));}
   J.TraceTicks.RemoveAll([](double At){return !FMath::IsFinite(At)||At<=0||At>18;});if(J.TraceTicks.Num()>64)J.TraceTicks.SetNum(64);J.TraceTicks.Sort();
   J.MarkStart=SafeNum(O,TEXT("markStart"),-1);J.MarkEnd=SafeNum(O,TEXT("markEnd"),-1);
   // Critic 12 Nov: the take decided before the save (absent in older saves: decided again when the receiver is next tuned).
   J.bSky=J.Stage==EJobStage::Pointed&&S47J::Bool(O,TEXT("sky")); // RX-ACQUIRE-SKY-20261113 (absent in older saves: found again on the chart)
   if(const auto Take=S47J::Obj(O,TEXT("takeDecided"));Take.IsValid()&&J.Stage==EJobStage::Pointed){J.TakeDecided=FMath::Clamp(SafeInt(Take,TEXT("odd"),-1),-1,1);J.bTakeOut=S47J::Bool(Take,TEXT("out"));J.TakePhen=S47J::Str(Take,TEXT("phen"));}
   J.SkyBy=J.Stage==EJobStage::New?0:FMath::Clamp(SafeInt(O,TEXT("skyBy")),0,2);J.bOwnMarks=S47J::Bool(O,TEXT("ownMarks")); // LOOP-CAREFUL-PAYS-20261113 (absent: not known, no marks of his own)
   if(J.Stage>EJobStage::Pointed)J.bTakeOut=S47J::Bool(O,TEXT("takeOut")); // LOOP-SLIP-IN-PACKET-20261113 (absent: not said to be the same thing)
   J.ReceiverClock=SafeNum(O,TEXT("receiverClock"));J.ReferenceClock=SafeNum(O,TEXT("referenceClock"));J.ControlReceiverClock=SafeNum(O,TEXT("controlReceiverClock"));J.ControlReferenceClock=SafeNum(O,TEXT("controlReferenceClock"));J.DuplicateClock=SafeNum(O,TEXT("duplicateClock"));J.DuplicateInterval=SafeNum(O,TEXT("duplicateInterval"));
   J.BeamPoint=FMath::Clamp(SafeInt(O,TEXT("beamPoint")),0,2);J.BeamSettle=FMath::Clamp(SafeNum(O,TEXT("beamSettle")),0.,1.);J.BeamSample=FMath::Clamp(SafeNum(O,TEXT("beamSample")),0.,8./3.);J.BeamAz=S47J::Nums(O,TEXT("beamAz"));J.BeamEl=S47J::Nums(O,TEXT("beamEl"));J.BeamAmplitude=S47J::Nums(O,TEXT("beamAmplitude"));
   J.bBeamVerified=S47J::Bool(O,TEXT("beamVerified"));J.bClockMeasured=S47J::Bool(O,TEXT("clockMeasured"));J.bDuplicateCollected=S47J::Bool(O,TEXT("duplicateCollected"));J.bDuplicateRejected=J.Phenomenon==TEXT("duplicate_print")&&!J.bDuplicateCollected&&S47J::Bool(O,TEXT("duplicateRejected"));J.OriginalPage=S47J::Str(O,TEXT("originalPage"));J.OriginalStamp=S47J::Str(O,TEXT("originalStamp"));J.DuplicatePage=S47J::Str(O,TEXT("duplicatePage"));J.DuplicateStamp=S47J::Str(O,TEXT("duplicateStamp"));
   // Legacy queued work was already immediately collectible; never reprint it.
   const int32 LegacyKind=PendingPrint(J);
   J.ChartFeed=FMath::Clamp(SafeNum(O,TEXT("chartFeed"),LegacyKind==0?PrinterSeconds:0),0.,PrinterSeconds);
   J.PacketFeed=FMath::Clamp(SafeNum(O,TEXT("packetFeed"),LegacyKind==1?PrinterSeconds:0),0.,PrinterSeconds);
   J.DuplicateFeed=FMath::Clamp(SafeNum(O,TEXT("duplicateFeed"),LegacyKind==2?PrinterSeconds:0),0.,PrinterSeconds);
   J.CaseLine=FMath::Clamp(SafeInt(O,TEXT("caseLine"),-1),-1,FS47FreeformCases::CaseCount-1);J.CaseTest=J.CaseLine<0?-1:FMath::Clamp(SafeInt(O,TEXT("caseTest"),-1),-1,FS47FreeformCases::TestCount-1);if(J.CaseTest<0)J.CaseLine=-1;
   J.bDocked=S47J::Bool(O,TEXT("docked"));J.PaidAmount=FMath::Clamp(int32(S47J::Num(O,TEXT("paidAmt"),-1)),-1,10000);J.bCaseValid=S47J::Bool(O,TEXT("caseValid"),true);J.bCaseRecorded=S47J::Bool(O,TEXT("caseRecorded"));if(J.CaseTest>=0){J.bAnomaly=false;J.Phenomenon.Empty();}
   // GAMEPLAY-LOOP-01: an extra-tick take saved at the printer (the old chart step) is on the reel deck now; nothing is lost. Its
   // original page is kept when it is confirmed at OPS (RetainOriginal); a control half done (optional now) runs on to its end.
   if(J.Stage==EJobStage::Chart&&PlainAnomaly(J)){J.Stage=EJobStage::Recorded;J.ChartFeed=PrinterSeconds;}
   const auto ValidSamples=[](const TArray<double>& A){if(A.Num()>3)return false;for(double V:A)if(!FMath::IsFinite(V))return false;return true;};
   if(!ValidSamples(J.BeamAz)||!ValidSamples(J.BeamEl)||!ValidSamples(J.BeamAmplitude)){J.BeamAz.Reset();J.BeamEl.Reset();J.BeamAmplitude.Reset();J.bBeamVerified=false;J.bControl=false;J.ControlAt=0;}
   if(const auto C=S47J::Obj(O,TEXT("card"));C&&S47J::Int(C,TEXT("v"))==1)
   {
    auto Card=FS47FreeformJobs::Make(JobSeed,J.Day,J.Slot,J.Source);auto& P=Card.Receiver;
    P.F=SafeNum(C,TEXT("f"),P.F);P.FTol=SafeNum(C,TEXT("ftol"),P.FTol);
    P.Gain[0]=SafeNum(C,TEXT("g0"),P.Gain[0]);P.Gain[1]=SafeNum(C,TEXT("g1"),P.Gain[1]);P.Bw[0]=SafeNum(C,TEXT("bw0"),P.Bw[0]);P.Bw[1]=SafeNum(C,TEXT("bw1"),P.Bw[1]);P.Az[0]=SafeNum(C,TEXT("az0"),P.Az[0]);P.Az[1]=SafeNum(C,TEXT("az1"),P.Az[1]);P.Card=S47J::Str(C,TEXT("text"),P.Card);P.Card.ReplaceInline(TEXT(" / FLEXIBLE ORDER, NO EXPIRY"),TEXT(" / FULL PAY FOR TWO DAYS"));
    Card.El=SafeNum(C,TEXT("el"),Card.El);Card.Seconds=SafeNum(C,TEXT("seconds"),Card.Seconds);Card.SuggestedClock=SafeNum(C,TEXT("suggested"),Card.SuggestedClock);
    if(FS47FreeformJobs::Valid(Card)){J.Card=Card;J.bGenerated=true;}
   }
   if(!J.Id.IsEmpty()&&!Work.ContainsByPredicate([&](const FJob& X){return X.Id==J.Id;}))Work.Add(J);
  }
  Selected=FMath::Clamp(SafeInt(Saved,TEXT("selected")),-1,FMath::Max(-1,Work.Num()-1));AudioSourceIndex=FMath::Clamp(SafeInt(Saved,TEXT("audioSource")),0,1);
  AudioCursor=FMath::Clamp(SafeNum(Saved,TEXT("audioCursor")),0.,AudioSourceIndex==0?18.:8.);
  const auto F=S47J::Obj(Saved,TEXT("findings"));if(F)for(const auto& P:F->Values){FString T;if(P.Value->TryGetString(T))Findings.Add(FString(*P.Key),T);}
  const auto D=S47J::Obj(Saved,TEXT("findingDays"));if(D)for(const auto& P:D->Values){double N;if(P.Value&&P.Value->TryGetNumber(N)&&FMath::IsFinite(N)&&N>=1&&N<=36500&&FMath::FloorToDouble(N)==N)FindingDays.Add(FString(*P.Key),int32(N));}
  const auto U=S47J::Nums(Saved,TEXT("upgrades"));for(int32 I=0;I<FMath::Min(3,U.Num());++I)Upgrades[I]=U[I]>0;
  if(SafeInt(Saved,TEXT("economyRevision"))<2&&Upgrades[1])
  {
   const int32 Paid=FMath::Min(80,Loan);Loan-=Paid;Cash+=80-Paid;Upgrades[1]=false;
   Feedback=TEXT("Reference comparison is included. Its former $80 charge has been credited, against the requisition first.");
  }
  const auto Q=S47J::Nums(Saved,TEXT("mysteries"));for(int32 I=0;I<FMath::Min(3,Q.Num());++I)MysterySteps[I]=FMath::Clamp(int32(Q[I]),0,3);
  SavedAngles=S47J::Nums(Saved,TEXT("angles"));bRestoreAngles=SavedAngles.Num()==108;
  if(bRestoreAngles)for(int32 I=0;I<108;++I)
  {
   const double V=SavedAngles[I];const bool Elevation=I%2==1;
   if(!FMath::IsFinite(V)||V<(Elevation?-5.:-360.)||V>(Elevation?90.:720.)){bRestoreAngles=false;break;}
  }
  if(!SavedAngles.IsEmpty()&&!bRestoreAngles)
  {
   SavedAngles.Reset();Settle=0;bCrossRunning=false;
   if(FJob* Current=Job())
   {
    if(Current->Stage==EJobStage::Pointed||Current->Stage==EJobStage::Integrating){UnfitCaseRelay(*Current);Current->Stage=EJobStage::New;Current->Elapsed=0;}
    if(Current->Phenomenon==TEXT("wrong_source")&&Current->bControl&&Current->ControlAt<8){Current->bControl=false;Current->ControlAt=0;Current->BeamAz.Reset();Current->BeamEl.Reset();Current->BeamAmplitude.Reset();Current->bBeamVerified=false;}
   }
   Feedback=TEXT("Invalid saved encoder data was discarded. Retained papers are safe; command fresh pointing before resuming unfinished measurements.");
  }
 }
 Cases.Load(S47J::Obj(Saved,TEXT("cases")),JobSeed);
 // Tom, 13 November: the repeat test is withdrawn. An older save's repeat job not yet recording goes, as a cancelled plan does
 // (CancelCaseJob) but without a word; one already recorded is ordinary work now, paid as routine. A registered one stays as it was.
 for(int32 I=Work.Num()-1;I>=0;--I)
 {
  FJob& J=Work[I];if(J.CaseTest<0||J.bCaseRecorded||FS47FreeformCases::Offered(J.CaseLine,J.CaseTest))continue;
  if(J.Stage<=EJobStage::Pointed)
  {
   if(Selected==I){Settle=0;bCrossRunning=false;}
   Work.RemoveAt(I);if(Selected>I)--Selected;else if(Selected==I)Selected=Work.IsEmpty()?-1:FMath::Max(0,Work.IndexOfByPredicate([](const FJob& X){return X.Stage!=EJobStage::Filed;}));
   continue;
  }
  J.CaseLine=J.CaseTest=-1;
  FString& Card=J.Card.Receiver.Card;const int32 Body=Card.StartsWith(TEXT("CASE TEST / "))?Card.Find(TEXT("\n\n")):INDEX_NONE;
  if(Body!=INDEX_NONE)Card=Card.Mid(Body+2);
 }
 // CASES-OPS-20261109: result sheets still on the printer (QueueResultSheet); an older save has none and prints none.
 CaseSheets.Reset();BoardLine=FreshLine=FreshTest=-1;bCaseDetails=false;BoardPick=BoardPickLine=-1;
 for(const auto& V:S47J::Arr(Saved,TEXT("caseSheets")))
 {
  if(!V||V->Type!=EJson::Object)continue;const auto O=V->AsObject();
  FResultSheet Sh;Sh.Line=SafeInt(O,TEXT("line"),-1);Sh.Test=SafeInt(O,TEXT("test"),-1);Sh.Day=FMath::Clamp(SafeInt(O,TEXT("day")),1,36500);
  Sh.Feed=FMath::Clamp(SafeNum(O,TEXT("feed")),0.,PrinterSeconds);Sh.Job=S47J::Str(O,TEXT("job"));Sh.Stamp=S47J::Str(O,TEXT("stamp"));Sh.Page=S47J::Str(O,TEXT("page"));
  const bool bSlip=IsBearingSlip(Sh)&&Sh.Line==-1&&Sh.Test==-1; // LOOP-1-20261109: a bearing slip, of no case
  if((!bSlip&&(Sh.Line<0||Sh.Line>=FS47FreeformCases::CaseCount||Sh.Test<0||Sh.Test>=FS47FreeformCases::TestCount))||Sh.Page.IsEmpty()||Sh.Page.Len()>4000||CaseSheets.Num()>=8)continue;
  CaseSheets.Add(Sh);
 }
 LoadTelex(Saved); // TELEX-20261109: what is on the telex and in the tray
 LoadMinutes(Saved); // LOOP-3-20261109: the array's own log and what has been asked of it
 TakenCount=0;for(const auto& Kept:Findings)TakenCount+=Kept.Key.StartsWith(TEXT("taken-"))?1:0;
 // Older saves: an already retained discrepancy opens its case retroactively. The interval case waits for its registration at OPS
 // while a reviewed comparison is still unregistered (Tom, 9 November: FirstCaseHeld).
 for(const auto& F:Findings){const int32 Line=FS47FreeformCases::LineForFinding(F.Key);if(Line>=0&&!(Line==0&&FirstCaseHeld()))Cases.Open(Line,FMath::Max(1,FindingDays.FindRef(F.Key)),20*3600.);if(F.Key.StartsWith(TEXT("taken-"))||F.Key.StartsWith(TEXT("flood-room-")))Cases.RuleOutWind(Line);
  if(F.Key.StartsWith(TEXT("minute_yes_")))Cases.RuleOutWind(Line); // LOOP-3-20261109: the same minute on the array, out on the land
  // LOOP-WHOLE-20261109: a save from before OUT ON THE LAND: what it kept out there is the land's (leads moved from the visitors keep
  // their place; the trip camera, things moved and the road were never leads before). Once: a newer save has them already.
  if(Line==FS47FreeformCases::LandLine&&Cases.FromBeforeLand())Cases.AddLead(Line,F.Key,FMath::Max(1,FindingDays.FindRef(F.Key)),20*3600.);}
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)if(const auto* C=Cases.Get(Line);C&&C->Planned>=0)
 {
  // Field work has its own retained plan ID and deliberately creates no receiver job.
  const bool FieldPlan=C->bOpen&&!C->bConcluded&&!C->Results[C->Planned].bDone&&CaseCondition(Line,C->Planned)==ECaseCondition::FieldWork;
  if(!FieldPlan&&!Work.ContainsByPredicate([&](const FJob& J){return J.Id==C->PlannedJob&&J.CaseLine==Line&&J.CaseTest==C->Planned&&!J.bCaseRecorded;}))Cases.CancelPlan(Line);
 }
 if(FieldFocus>=0){const auto* Focus=Cases.Get(FieldFocus);if(!Focus||!Focus->bOpen||Focus->bConcluded)FieldFocus=-1;}
 // CASES-OPS-20261109: an older save's reel-deck case has its window from the review; the first original it came from says the clock
 // (SetReference keeps it only while no test has a result).
 if(const auto* First=Cases.Get(0);First&&First->bOpen&&First->RefDay==First->OpenDay&&First->RefClock==First->OpenClock)
  if(const FJob* Original=Work.FindByPredicate([](const FJob& J){return J.bAnomaly&&J.bAudioReviewed&&J.CaseTest<0&&!Specialized(J.Phenomenon)&&J.ReferenceClock>0;}))
   Cases.SetReference(0,NightOf(Original->ReferenceClock),FMath::Fmod(Original->ReferenceClock,86400.));
 AddDay(DayNumber);
 if(Tutorial>0)
 {
  int32 Index=Work.IndexOfByPredicate([this](const FJob& J){return J.Id==TutorialJobId&&J.CaseTest<0&&!J.bAnomaly;});
  if(Index==INDEX_NONE)Index=Work.IndexOfByPredicate([](const FJob& J){return J.CaseTest<0&&!J.bAnomaly&&J.Stage!=EJobStage::Filed;});
  if(Index==INDEX_NONE){Tutorial=0;TutorialJobId.Empty();}
  else {Selected=Index;TutorialJobId=Work[Index].Id;}
 }
 if(!Director)Director=MakeShared<FS47FreeformDirector>(M);
 if(!Field)Field=MakeShared<FS47FreeformWorld>(M);
 Field->Begin(S47J::Obj(Saved,TEXT("field")));
 if(!CaseField)CaseField=MakeShared<FS47CaseFieldTests>(M);CaseField->Load(S47J::Obj(Saved,TEXT("fieldTests")));
 LastServiceFeedback=Field->ServiceCompletions();CommandClicks=ServiceClicks=0;FeedbackEncoders.Reset();
 if(!AudioDesk)AudioDesk=FS47FreeformAudioDesk::Shared(M);AudioDesk->Begin();
 if(!Decor)Decor=MakeShared<FS47RoomDecor>(M);Decor->Begin(S47J::Obj(Saved,TEXT("decor")));
 if(!Taken)Taken=MakeShared<FS47Taken>(M);
 if(!Aftermath)Aftermath=MakeShared<FS47Aftermath>(M);Aftermath->Begin(S47J::Obj(Saved,TEXT("aftermath")));
 if(!FarmSite)FarmSite=MakeShared<FS47FarmSite>(M);FarmSite->Begin(S47J::Obj(Saved,TEXT("farmSite")));
 if(!Deputy)Deputy=MakeShared<FS47DeputyRound>(M);Deputy->Begin(S47J::Obj(Saved,TEXT("deputy")));
 if(!Gear)Gear=MakeShared<FS47LeftGear>(M);Gear->Begin(S47J::Obj(Saved,TEXT("gear")));
 Kit.Reset();for(const FString& Have:S47J::Strs(Saved,TEXT("kit")))if(FS47Openings::Def(Have))Kit.Add(Have);
 BrokenTubes=TubesFrom(S47J::Nums(Saved,TEXT("brokenTubes"))); // N1110: optional, absent = none
 if(!Places)Places=MakeShared<FS47Places>(M);Places->Begin(S47J::Obj(Saved,TEXT("places")));
 if(!EventPlaces)EventPlaces=MakeShared<FS47EventPlaces>(M);EventPlaces->Begin(S47J::Obj(Saved,TEXT("eventPlaces")));
 if(!Balloon)Balloon=MakeShared<FS47Balloon>(M);Balloon->Begin(S47J::Obj(Saved,TEXT("balloon"))); // D48-20261110 (after Places: it stands where the place does)
 // STORY-ON-FREEFORM-01 (Tom 11 Nov): the Story player is Reyes, and Freeform comes after Story, when Reyes is gone. On a Story night his
 // disappearance's aftermath (the predecessor: his locker, Dale and Ward on the vanished Reyes, his mail) is not there yet.
 if(M&&M->Flow&&M->Flow->StoryNight())Predecessor.Reset();
 else{if(!Predecessor)Predecessor=MakeShared<FS47Predecessor>(M);Predecessor->Begin(S47J::Obj(Saved,TEXT("predecessor")),Saved.IsValid(),DayNumber);}
 if(!Tips)Tips=MakeShared<FS47RadioTips>(M);Tips->Begin(S47J::Obj(Saved,TEXT("tips")));
 if(!TownUse)TownUse=MakeShared<FS47TownUse>(M);TownUse->Begin(S47J::Obj(Saved,TEXT("townUse")));
 if(!TownSites)TownSites=MakeShared<FS47TownSites>(M);TownSites->Begin();
 if(!Townsfolk)Townsfolk=MakeShared<FS47Townsfolk>(M);Townsfolk->Begin(S47J::Obj(Saved,TEXT("townsfolk")));
 if(!Visitors)Visitors=MakeShared<FS47Visitors>(M);Visitors->Begin(S47J::Obj(Saved,TEXT("visitors")));
 if(!MotelNight)MotelNight=MakeShared<FS47MotelNight>(M);MotelNight->Begin(S47J::Obj(Saved,TEXT("motelNight")));
 if(!TruckLamp)TruckLamp=MakeShared<FS47TruckLamp>(M);
 if(!Outfit)Outfit=MakeShared<FS47Outfit>(M);Outfit->Begin(S47J::Obj(Saved,TEXT("outfit")));
 if(!YardAlarm)YardAlarm=MakeShared<FS47YardAlarm>(M);YardAlarm->Begin(S47J::Obj(Saved,TEXT("alarm")));
 if(!Care)Care=MakeShared<FS47TruckCare>(M);Care->Begin(S47J::Obj(Saved,TEXT("care")));
 if(!TripCamera)TripCamera=MakeShared<FS47TripCamera>(M);TripCamera->Begin(S47J::Obj(Saved,TEXT("tripCamera")));
 if(!Round)Round=MakeShared<FS47DayRound>(M);Round->Begin(S47J::Obj(Saved,TEXT("round")));
 if(!RoadCar)RoadCar=MakeShared<FS47RoadsideCar>(M);RoadCar->Begin(S47J::Obj(Saved,TEXT("roadCar")));
 if(!Happenings)Happenings=MakeShared<FS47Happenings>(M);Happenings->Begin(S47J::Obj(Saved,TEXT("happenings")));
 if(!WeekCase)WeekCase=MakeShared<FS47WeekCase>(M);WeekCase->Begin(S47J::Obj(Saved,TEXT("weekCase")));
 if(!Scripts)Scripts=MakeShared<FS47ScriptPlayer>(M);Scripts->Begin(S47J::Obj(Saved,TEXT("scripts")));
 if(!NightLog)NightLog=MakeShared<FS47NightLog>(M);NightLog->Begin(S47J::Obj(Saved,TEXT("nightLog")));
 if(!Tape)Tape=MakeShared<FS47Tape>(M);Tape->Begin(S47J::Obj(Saved,TEXT("tape")));
 if(!PinBoard)PinBoard=MakeShared<FS47PinBoard>(M);PinBoard->Begin(S47J::Obj(Saved,TEXT("pinBoard")));
 if(!Enlarger)Enlarger=MakeShared<FS47Enlarger>(M);
 if(!Darkroom)Darkroom=MakeShared<FS47Darkroom>(M);Darkroom->Begin(S47J::Obj(Saved,TEXT("darkroom"))); // D45-20261110
 if(!WardReport)WardReport=MakeShared<FS47WardReport>(M);WardReport->Begin(S47J::Obj(Saved,TEXT("wardReport")));
 if(!SkyAnswer)SkyAnswer=MakeShared<FS47Answer>(M);SkyAnswer->Begin(S47J::Obj(Saved,TEXT("answer")));
 if(!Weather)Weather=MakeShared<FS47WeatherLive>(M);Weather->Stop();
 if(!Fatigue)Fatigue=MakeShared<FS47Fatigue>(M);Fatigue->Begin(S47J::Obj(Saved,TEXT("fatigue")));
 if(!Handling)Handling=MakeShared<FS47Handling>(M);Handling->Begin();
 if(!Cot)Cot=MakeShared<FS47SaroCot>(M);Cot->Begin();
 if(!Hide)Hide=MakeShared<FS47Hide>(M);Hide->Begin(); // N1110
 if(!Attention)Attention=MakeShared<FS47Attention>(M);Attention->Begin(S47J::Obj(Saved,TEXT("attention"))); // N1110
 if(!DoorLoop)DoorLoop=MakeShared<FS47DoorLoop>(M);DoorLoop->Begin(); // N1110 (N1110-6)
 if(!Mail)Mail=MakeShared<FS47Mail>(M);Mail->Begin(S47J::Obj(Saved,TEXT("mail")),Saved.IsValid());
 if(!FarmNight)FarmNight=MakeShared<FS47FarmNight>(M);
 if(!HouseDoors)HouseDoors=MakeShared<FS47FarmHouseDoors>(M);HouseDoors->Begin(S47J::Obj(Saved,TEXT("houseDoors")));
 if(!IndoorDread)IndoorDread=MakeShared<FS47IndoorDread>(M);IndoorDread->Begin(S47J::Obj(Saved,TEXT("indoorDread")));
 Director->Begin(S47J::Obj(Saved,TEXT("director")));
 LoadDossier(Saved); // EVIDENCE-DOSSIER: "dossier"; a save from before is rebuilt from its findings and the Director's photograph claims
 IngestedObservations=0;IngestObservations();
 if(M&&M->Story){M->Story->SetClock(FMath::Fmod(WorldClock,86400.));M->Story->Rx.bPowered=true;}
 StepTutorial();ClearCompletedSelection();
 UpdateMysteries();
 UE_LOG(LogTemp,Display,TEXT("S47: freeform calendar: day %d is %s (%s)"),DayNumber,*S47Game::DateLine(Date()),bOldCalendar?TEXT("a game begun on 13 April keeps that calendar"):TEXT("began 28 April"));
}
FDateTime FS47FreeformSession::DateOfDay(int32 ShiftDay)const{return S47Game::GameDate(true,ShiftDay,0,bOldCalendar);}
void FS47FreeformSession::Stop()
{
 LiveCapture.Reset();TapeCapture.Reset();CaptureStartStage=0;
 S47AlienKinds::Hush(TEXT("take"));S47AlienKinds::Hush(TEXT("reel")); // D48-20261110
 StopEquipmentFeedback();StopPrinterFeedback();FeedbackEncoders.Reset();
 if(!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&IsValid(M->Audio)){M->Audio->ReceiverMonitorOff();M->Audio->ReceiverRecordingOff();if(RxSearch)RxSearch->Off(M->Audio);}
 // UObject purge may destroy the scene before this plain C++ session is released.
 const bool bCanRestore=!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&IsValid(M->Story)&&IsValid(M->Web);
 if(bCanRestore&&bPowerApplied&&!bLastPower)M->Story->ApplyFeederPower(false);
 bPowerApplied=false;
 if(CaseField)CaseField->Stop();
 StopSopCard();StopTelex();
 if(Director)Director->Stop();if(AudioDesk)AudioDesk->Stop();if(Decor)Decor->Stop();if(IndoorDread)IndoorDread->Stop();if(Taken)Taken->Stop();if(Aftermath)Aftermath->Stop();if(Deputy)Deputy->Stop();if(Gear)Gear->Stop();if(Places)Places->Stop();if(Balloon)Balloon->Stop();if(Predecessor)Predecessor->Stop();if(TruckLamp)TruckLamp->Stop();if(YardAlarm)YardAlarm->Stop();if(TripCamera)TripCamera->Stop();if(RoadCar)RoadCar->Stop();if(FarmSite)FarmSite->Stop();if(FarmNight)FarmNight->Stop();if(HouseDoors)HouseDoors->Stop();if(Field)Field->Stop();
 if(Happenings)Happenings->Stop();
 if(WeekCase)WeekCase->Stop();
 if(Scripts&&bCanRestore)Scripts->Stop();
 // CRIT6-20261109: the cab's controller is in the world, which the purge may already have taken.
 if(Outfit&&bCanRestore)Outfit->Stop();
 if(bCanRestore){if(TownUse)TownUse->Stop();if(TownSites)TownSites->Stop();}
 // At exit or in the purge the town's figures may already be gone: only the plain list of who is away is cleared then.
 if(Townsfolk){if(bCanRestore)Townsfolk->Stop();else Townsfolk->ResetAway();}
 if(Visitors&&bCanRestore)Visitors->Stop();
 if(NightLog)NightLog->Stop();
 if(Tape)Tape->Stop();
 if(PinBoard)PinBoard->Stop();
 if(Darkroom)Darkroom->Stop(); // D45-20261110: white light back for Story or the next case (it checks the world itself)
 if(WardReport)WardReport->Stop();
 if(SkyAnswer)SkyAnswer->Stop();
 if(bCanRestore&&Weather)Weather->Stop();else S47Weather::Live().bOn=false;
 if(bCanRestore&&Fatigue)Fatigue->Stop();
 if(bCanRestore&&Handling)Handling->Stop();
 if(bCanRestore&&Cot)Cot->Stop();
 if(Hide)Hide->Stop(); // N1110: out from under the table
 if(Attention)Attention->Stop(); // N1110
 if(DoorLoop&&bCanRestore)DoorLoop->Stop(); // N1110 (N1110-6): the lights and the eye as they were
 if(Mail)Mail->Stop();
 TerminalView.Reset();ActiveTerminal=-1;
 if(bCanRestore){M->Web->SetWorkMotionPaused(false);if(bActive)M->Web->SetPrinterPaper(-1.f);}
 bActive=bTelemetry=bAudioOpen=bAudioPlaying=bRecovering=false;++RecoveryEpoch;
}
bool FS47FreeformSession::Held() const
{
 // DIRECTOR-MODE-01: the director's free camera holds the night as an open panel does (its clock stands; S47DirectorCam.h).
 return !bActive||bDead||bRecovering||!M||M->Mode!=US47Main::EMode::Play||M->bPausedByMenu||(M->ModalOpen()&&!TerminalOnGlass()&&!bTapePanel)||(M->World&&M->World->bBusy)||S47DirectorCam::On(M)||FS47DevWaves::On(M); // (and the F10 wave shooter, a dev toy: S47DevWaves.h)
}
bool FS47FreeformSession::ArrayMotionHeld() const
{
 return !bActive||bDead||bRecovering||!M||M->bPausedByMenu||(M->World&&M->World->bBusy)||(M->ModalOpen()&&!bTelemetry&&!TerminalOnGlass())||!HasPower();
}
bool FS47FreeformSession::HasPower() const
{
 // Closed: the county line is open at the meter (FS47Predecessor::Close). Only a fuelled generator runs anything.
 if(Predecessor&&Predecessor->StationClosed())return Field&&Field->HasBackupPower();
 return (!bPowerFault&&!SkyMainsOut())||(Field&&Field->HasBackupPower());
}
// FIX-20261110 (Tom, point 2): the mains under a passage. See SkyCutBegin in the header.
void FS47FreeformSession::SkyCutBegin(double Sag,double Out)
{
 if(!bActive||SkyCutLive()||bPowerFault||(Predecessor&&Predecessor->StationClosed()))return;
 SkySag=SkySagAll=FMath::Max(1.,Sag);SkyOut=FMath::Max(1.,Out);SkyDipAt=FMath::Min(2.,SkySag*.2);SkyDip=SkyDark=0;bSkyRode=false;SkyTold=0;
 UE_LOG(LogTemp,Display,TEXT("S47: sky cut: the feeder sags for %.0f s, then the mains are out for %.0f s"),SkySag,SkyOut);
}
float FS47FreeformSession::SkyLevel() const
{
 if(SkySag>0)return float(.18+.42*(1-SkySag/SkySagAll));
 return SkyOut>0?.45f:0.f;
}
void FS47FreeformSession::StepSky(double Dt)
{
 if(!SkyCutLive()||!M)return;
 const bool bAtSaro=M->Web&&M->Web->Area()==TEXT("saro")&&M->World&&M->World->Area==TEXT("saro");
 const bool bIn=bAtSaro&&S47SaroSave::InStation(M);
 const bool Backup=Field&&Field->HasBackupPower();
 auto Tell=[&](const TCHAR* Text,float Seconds){if(bAtSaro&&M->UI)M->UI->Toast(Text,Seconds);};
 auto DipOver=[&]{if(SkyDip>0&&M->Web){M->Web->SetLightsScale(1.f,1.f);M->Web->SetTubes(true);}SkyDip=0;};
 if(bPowerFault){DipOver();SkySag=SkyOut=0;return;} // an ordinary feeder fault on top of it: that one is the operator's to mend
 if(SkySag>0)
 {
  SkySag-=Dt;
  if(SkyDip>0){SkyDip-=Dt;if(SkyDip<=0)DipOver();}
  else if(!Backup&&SkySag>1.2&&(SkyDipAt-=Dt)<=0&&M->Web&&!(M->SaroNight&&M->SaroNight->Power&&M->SaroNight->Power->Out()))
  {
   // The room sags for a moment, as Story's dip after the relay does (AS47Story::LightsDip); the switches' own state is untouched.
   SkyDip=FMath::FRandRange(.16,.5);SkyDipAt=FMath::FRandRange(3.5,8.)*FMath::Clamp(SkySag/SkySagAll+.35,.4,1.);
   M->Web->SetLightsScale(.24f,.35f);M->Web->SetTubes(false);
   if(M->Audio&&bAtSaro){M->Audio->Sfx(TEXT("rack_switch"),FS47Sound{.Gain=bIn?.32f:.1f,.Rate=FMath::FRandRange(.7f,.9f)});M->Audio->Beep(52.f,float(SkyDip)+.25f,bIn?.05f:.02f,0.f,TEXT("sine"));}
   if(SkyTold==0){SkyTold=1;Tell(bIn?TEXT("The lamps dip and come back. On the rack the line-voltage needle is sagging, and there is not a cloud out there."):TEXT("The station's lamps dip and come back. Not a cloud in the sky."),6);}
   else if(SkyTold==1&&SkySag<SkySagAll*.55)
   {
    SkyTold=2;
    Tell(HasKit(TEXT("transfer"))?TEXT("Again, and deeper. The feeder is going. The transfer switch in the service bay will throw the standby generator in, if it has fuel.")
     :TEXT("Again, and deeper. The feeder is going. The standby generator is outside, behind the control room."),7);
   }
  }
  if(SkySag<=0)
  {
   SkySag=0;DipOver();
   const int32 Thrown=Backup||!Field?-1:Field->SkyTransfer();
   const bool Up=Field&&Field->HasBackupPower();bSkyRode=Up;SkyDark=0;
   if(M->Audio&&bAtSaro&&!Up){M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.5f,.Rate=.7f});M->Audio->Beep(110.f,.7f,.05f,0.f,TEXT("sine"));}
   Tell(Thrown==1?TEXT("The mains drop out. Down in the service bay the transfer switch clunks over, and behind the control room the standby generator coughs and takes the load.")
    :Thrown==0?TEXT("The mains drop out. The transfer switch clunks over and the standby generator turns and does not catch: no fuel for it. Every lamp in the station is out.")
    :Up?TEXT("The mains drop out. The standby generator takes the load: the lamps dip once and hold, and the line recorder goes on writing.")
    :TEXT("Every lamp in the station goes out at once. No storm, no fault bell. The mains are just gone."),8);
   UE_LOG(LogTemp,Display,TEXT("S47: sky cut: mains out for %.0f s, standby %s"),SkyOut,Up?TEXT("carrying the station"):Thrown==0?TEXT("dry"):TEXT("not running"));
  }
  return;
 }
 SkyOut-=Dt;if(!Backup)SkyDark+=Dt;else if(SkyDark<12)bSkyRode=true;
 if(SkyOut<=0)
 {
  SkyOut=0;
  if(M->Audio&&bAtSaro)for(int32 K=0;K<3;++K)M->Audio->Sfx(TEXT("rack_switch"),FS47Sound{.Gain=bIn?.4f:.12f,.Rate=.8f+.1f*K,.When=.22f*K});
  if(FS47StoryDirector::Of(M))
  {
   // Story's night: the act's own feeder_fault owns the power flags, so the passage leaves no fault of its own.
   Tell(Backup?TEXT("The mains are back. Relays clatter through the rack. The standby generator is still running on its tank; it can be stopped.")
    :TEXT("The mains come back. Relays clatter through the rack and the tubes strike one after another."),7);
   UE_LOG(LogTemp,Display,TEXT("S47: sky cut: mains back (Story night; dark for %.0f s, standby %s)"),SkyDark,Backup?TEXT("running"):TEXT("off"));
  }
  else
  {
   // FIX10-10: the mains try to come back and the feeder's protection locks out. An ordinary feeder fault from here: the remote
   // feeder (ff:feeder, road) is mended as any other, the generator carries the station meanwhile, and the county crew gets to it in the end.
   SetPowerFault(true);SkyFaultUntil=WorldClock+SkyFaultMinutes*60;
   // POWER-CHOICE-20261112: the mains the sabotage took (FS47Attention): the same lockout, said as what it is, with the night's choice in it.
   if(Attention&&Attention->Sabotage()==FS47Attention::ESab::Mains)
    Tell(Backup?TEXT("Relays clatter through the rack, catch and drop out again: the feeder has locked out, and the protection names no fault. No storm, no load. Something was done to it out at the remote feeder on the county road. The standby generator carries the station, loud, for as long as its tank lasts.")
     :TEXT("Relays clatter through the rack, catch and drop out again. The feeder has locked out and the protection names no fault: no storm, no load. Something was done to it out at the remote feeder on the county road. Mend it out there in the dark, or start the loud standby generator behind the control room."),9);
   else Tell(Backup?TEXT("Relays clatter through the rack, catch and drop out again: the feeder has locked out. The standby generator carries the station on its tank; the remote feeder out on the county road needs mending.")
    :TEXT("Relays clatter through the rack, catch and drop out again. The feeder has locked out and the station stays dark. Mend the remote feeder on the county road, or start the standby generator behind the control room."),9);
   UE_LOG(LogTemp,Display,TEXT("S47: sky cut: the feeder locks out (dark for %.0f s, standby %s); the county restores it at clock %.0f"),SkyDark,Backup?TEXT("running"):TEXT("off"),SkyFaultUntil);
  }
 }
}
bool FS47FreeformSession::RequirePower()
{
 if(HasPower())return true;
 bAudioPlaying=false;Feedback=TEXT("No power to this instrument. Restore the feeder or transfer to the fuelled generator. Retained papers can still be read.");Changed(Feedback);return false;
}
void FS47FreeformSession::AddDay(int32 Number)
{
 if(Work.ContainsByPredicate([&](const FJob& J){return J.Day==Number;}))return;
 for(const auto& Card:FS47FreeformJobs::Generate(JobSeed,Number)){FJob J;J.Id=FString::Printf(TEXT("FF-D%04d-J%d"),Number,Card.Slot+1);J.Day=Number;J.Slot=Card.Slot;J.Source=Card.Source;J.Card=Card;J.bGenerated=true;Work.Add(J);}
}
int32 FS47FreeformSession::LapseUntakenCards()
{
 // CHAIN-5-20261109: a card nobody began (still New, nothing recorded) does not wait for ever: at the turn of the day it falls away.
 // Kept: case tests (a planned test stays until recorded), the training card, cards begun, paid or already worked, and today's cards.
 // It leaves no mark on Ward's count: she counts filed work and its quality, and a card that lapses was never filed (not taken, not wrong).
 if(!RulesLive()||Tutorial>0)return 0;
 const FString Was=Work.IsValidIndex(Selected)?Work[Selected].Id:FString();
 const int32 Gone=Work.RemoveAll([this](const FJob& J){return J.bGenerated&&J.CaseTest<0&&!J.bCaseRecorded&&!TrainingCard(J)&&J.Day<DayNumber&&J.Stage==EJobStage::New&&J.Elapsed<=0&&!J.bPaid&&!J.bPrinted&&J.OriginalPage.IsEmpty();});
 if(Gone<=0)return 0;
 const int32 Now=Work.IndexOfByPredicate([&Was](const FJob& J){return J.Id==Was;});
 Selected=Now>=0?Now:FMath::Max(0,Work.IndexOfByPredicate([](const FJob& J){return J.Stage!=EJobStage::Filed;}));
 UE_LOG(LogTemp,Display,TEXT("S47: %d untaken cards lapsed at the turn of the day"),Gone);
 return Gone;
}
FS47FreeformSession::FJob* FS47FreeformSession::Job(){return Work.IsValidIndex(Selected)?&Work[Selected]:nullptr;}
const FS47FreeformSession::FJob* FS47FreeformSession::CurrentJob()const{return Work.IsValidIndex(Selected)?&Work[Selected]:nullptr;}
FS47Profile FS47FreeformSession::Profile(int32 Source){return FS47Observing::Runs()[FMath::Clamp(Source,0,3)].P;}
FS47Profile FS47FreeformSession::EffectiveProfile(const FJob& J)const{return J.bGenerated?J.Card.Receiver:Profile(J.Source);}
const FS47Profile* FS47FreeformSession::ConsoleProfile()const
{
 if(!Active()||!CurrentJob())return nullptr;
 static FS47Profile P;P=EffectiveProfile(*CurrentJob());return &P;
}
double FS47FreeformSession::TargetAz()const{if(const FJob* J=CurrentJob()){auto P=EffectiveProfile(*J);return(P.Az[0]+P.Az[1])/2+(J->CaseTest>=0&&CaseCondition(J->CaseLine,J->CaseTest)==ECaseCondition::OffSource?FS47FreeformCases::OffSourceDegrees:0);}return 0;}
double FS47FreeformSession::TargetEl()const{return CurrentJob()?(CurrentJob()->bGenerated?CurrentJob()->Card.El:FS47Observing::Runs()[CurrentJob()->Source].El):25;}
double FS47FreeformSession::SourceSeconds()const{return CurrentJob()?(CurrentJob()->bGenerated?CurrentJob()->Card.Seconds:FS47Observing::Runs()[CurrentJob()->Source].Seconds):18;}
FString FS47FreeformSession::Reference(const FJob& J)const{return TEXT("SARO / ")+J.Id;}
FString FS47FreeformSession::JobLine(const FJob& J)const
{
 return FString::Printf(TEXT("D%02d / %s / %s / %s%s%s"),J.Day,*J.Id,FS47Observing::Runs()[J.Source].Source,*StageText(J.Stage),J.bAnomaly?TEXT(" / INVESTIGATE"):TEXT(""),J.CaseTest>=0?*(TEXT(" / CASE TEST: ")+CaseTestName(J.CaseLine,J.CaseTest).ToUpper()):TEXT(""));
}
int32 FS47FreeformSession::AvailableDishCount()const
{
 int32 Count=0;if(M&&M->Web)for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishRemoteAvailable(I))++Count;
 return Count;
}
int32 FS47FreeformSession::ArrayUpCount()const
{
 // D45-20261110: one count wherever the station goal is shown (the goal line, OPS, the notebook, the toasts)
 if(M&&M->Web&&M->Web->DishCount()>0)return AvailableDishCount();
 return Field&&Field->ArrayWear().bActive?27-Field->ArrayWear().DownCount():27;
}
double FS47FreeformSession::MeasurementRateForCount(int32 Count)
{
 // Gameplay balance from available antenna pairs, not a full radiometer simulation.
 const int32 N=FMath::Clamp(Count,0,27);
 return N<24?0.:double(N*(N-1))/(27.*26.);
}
bool FS47FreeformSession::ArrayBlocked()const{return AvailableDishCount()<(Field&&Field->ArrayWear().bActive?FS47ArrayWear::MinDishes:24);}
double FS47FreeformSession::ArrayMeasurementRate()const{return Field&&Field->ArrayWear().bActive?FS47ArrayWear::RateForCount(AvailableDishCount()):MeasurementRateForCount(AvailableDishCount());}
double FS47FreeformSession::ReceiverReception()const
{
 if(!CurrentJob()||!M||!M->Web||!HasPower())return 0;
 double Sum=0;const double Az=TargetAz(),El=TargetEl();
 for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishRemoteAvailable(I))
 {
  const double AzError=FMath::FindDeltaAngleDegrees(M->Web->DishAz(I),Az);
  const double ElError=M->Web->DishEl(I)-El;
  Sum+=FMath::Exp(-FMath::Square(AzError/.35)-FMath::Square(ElError/.5));
 }
 // A commanded bearing is not an encoder measurement. Let a moving/settling
 // array reveal the source gradually, without changing acquisition requirements.
 // A worn array sounds thin rather than silent; a whole one is unchanged.
 const double Reception=Field&&Field->ArrayWear().bActive?FMath::Sqrt(FMath::Clamp(Sum/27.,0.,1.)):FMath::Clamp(Sum/27.,0.,1.);
 return PointReady()?Reception*(.25+.75*FMath::Clamp(Settle,0.,1.)):FMath::Min(.6,Reception);
}
// D45-20261110 (Tom: the target does not show on the receiver until the array has locked): what the receiver shows and plays of the
// source while the array comes round. Before pointing a trace of it in the side lobes; while slewing it grows with how close the
// dishes are (a beam of some degrees, not the recording's narrow one); on target it rises with the settle. Picture and sound only.
double FS47FreeformSession::ReceiverApproach()const
{
 const FJob* J=CurrentJob();if(!J||!M||!M->Web||!HasPower())return 0;
 if(J->Stage==EJobStage::New)return .12;
 if(PointReady())return FMath::Lerp(.8,1.,FMath::Clamp(Settle,0.,1.));
 double Sum=0;int32 N=0;const double Az=TargetAz(),El=TargetEl();
 for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishRemoteAvailable(I))
 {++N;const double E=FMath::Sqrt(FMath::Square(FMath::FindDeltaAngleDegrees(M->Web->DishAz(I),Az))+FMath::Square(M->Web->DishEl(I)-El));Sum+=FMath::Exp(-FMath::Square(E/6.));}
 return FMath::Clamp(.12+.68*(N?Sum/N:0.),0.,.8);
}
double FS47FreeformSession::ArrayErrorDeg()const
{
 if(!CurrentJob()||!M||!M->Web)return 0;
 double Worst=0;const double Az=TargetAz(),El=TargetEl();
 for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishRemoteAvailable(I))Worst=FMath::Max(Worst,FMath::Max(FMath::Abs(FMath::FindDeltaAngleDegrees(M->Web->DishAz(I),Az)),FMath::Abs(M->Web->DishEl(I)-El)));
 return Worst;
}
FString FS47FreeformSession::ArrayMeasurementStatus()const
{
 const int32 N=AvailableDishCount();const bool Worn=Field&&Field->ArrayWear().bActive;const int32 Least=Worn?FS47ArrayWear::MinDishes:24;
 return FString::Printf(TEXT("ARRAY %d/27 / MEASUREMENT RATE %.0f%% / %s"),N,100*ArrayMeasurementRate(),N<Least?*FString::Printf(TEXT("BLOCKED: inspect ARRAY STATUS (left-hand PC); need %d"),Least):N<27?(Worn&&Field->ArrayWear().DownCount()>0?TEXT("WORN: restore the outer branches to speed up measuring"):TEXT("DEGRADED: repair restores rate")):TEXT("FULL RATE"));
}
FVector2D FS47FreeformSession::AimAzEl()
{
 const FJob* J=CurrentJob();if(!J||!M||!M->Web)return FVector2D::ZeroVector;
 if(AimJob!=J->Id)
 {
  // A new job starts from where the array points now; the player finds the source from there.
  double Az=0,El=0;int32 N=0;
  for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishRemoteAvailable(I)){Az+=M->Web->DishAz(I);El+=M->Web->DishEl(I);++N;}
  AimJob=J->Id;Aim=N?FVector2D(FMath::Clamp(Az/N,0.,180.),FMath::Clamp(El/N,0.,90.)):FVector2D(90,45);bAimLocked=AimOnTarget();
 }
 return Aim;
}
bool FS47FreeformSession::AimOnTarget()const
{
 // The plot covers azimuth 0 to 180, so a target is compared where the plot can show it.
 static constexpr double Tolerance=3;
 return CurrentJob()&&AimJob==CurrentJob()->Id&&FMath::Abs(Aim.X-FMath::Clamp(TargetAz(),0.,180.))<=Tolerance&&FMath::Abs(Aim.Y-FMath::Clamp(TargetEl(),0.,90.))<=Tolerance;
}
double FS47FreeformSession::AimNearness(FVector2D AzEl)const
{
 // D45-20261110: the tracking plot's search tick and cursor; the larger of the two axis errors, so 1 is exactly where AimOnTarget locks
 static constexpr double Tolerance=3,Far=60;
 if(!CurrentJob())return 0;
 const double E=FMath::Max(FMath::Abs(AzEl.X-FMath::Clamp(TargetAz(),0.,180.)),FMath::Abs(AzEl.Y-FMath::Clamp(TargetEl(),0.,90.)));
 return FMath::IsFinite(E)?1-FMath::Clamp((E-Tolerance)/(Far-Tolerance),0.,1.):0.;
}
void FS47FreeformSession::SetAim(FVector2D AzEl)
{
 if(!CurrentJob())return;AimAzEl();
 Aim=FVector2D(FMath::Clamp(AzEl.X,0.,180.),FMath::Clamp(AzEl.Y,0.,90.));
 const bool bOn=AimOnTarget();
 if(bOn)Aim=FVector2D(FMath::Clamp(TargetAz(),0.,180.),FMath::Clamp(TargetEl(),0.,90.)); // Settles on the source.
 if(bOn&&!bAimLocked&&M&&M->Audio)M->Audio->Beep(1320.f,.12f,.16f);
 bAimLocked=bOn;
}
bool FS47FreeformSession::PointReady()const
{
 const FVector2D Off=bCrossRunning?Cross[CrossPoint]:FVector2D::ZeroVector;
 return CurrentJob()&&EncodersAt(TargetAz()+Off.X,TargetEl()+Off.Y);
}
bool FS47FreeformSession::EncodersAt(double Az,double El)const
{
 if(!M||!M->Web||M->Web->DishCount()!=27||!HasPower())return false;
 int32 Count=0;for(int32 I=0;I<27;++I)if(M->Web->DishRemoteAvailable(I))
 {++Count;if(FMath::Abs(FMath::FindDeltaAngleDegrees(M->Web->DishAz(I),Az))>.1||FMath::Abs(M->Web->DishEl(I)-El)>.1)return false;}
 return Count>=(Field&&Field->ArrayWear().bActive?FS47ArrayWear::MinDishes:24);
}
S47J::FObj FS47FreeformSession::CaptureSoundState()const
{
 auto O=S47J::New();S47J::Set(O,TEXT("liveQueued"),double(LiveCapture?LiveCapture->QueuedSamples():0));S47J::Set(O,TEXT("liveRendered"),double(LiveCapture?LiveCapture->RenderedSamples():0));S47J::Set(O,TEXT("tapeQueued"),double(TapeCapture?TapeCapture->QueuedSamples():0));S47J::Set(O,TEXT("tapeRendered"),double(TapeCapture?TapeCapture->RenderedSamples():0));return O;
}
bool FS47FreeformSession::UsesCapturedAudio() const
{
 const auto* J=CurrentJob();return UseCapturedSound(J?(AudioSourceIndex==0?&J->CaptureOriginal:&J->CaptureControl):nullptr,FParse::Param(FCommandLine::Get(),TEXT("NoSound")));
}
FString FS47FreeformSession::AudioCaptureStatus() const
{
 if(FParse::Param(FCommandLine::Get(),TEXT("NoSound")))return TEXT("SOUND DISABLED / COMPATIBLE TRANSPORT");
 const auto* J=CurrentJob();const auto* C=J?(AudioSourceIndex==0?&J->CaptureOriginal:&J->CaptureControl):nullptr;
 return C&&C->Enabled()?(UsesCapturedAudio()?FString(TEXT("CAPTURED RECEIVER SOUND")):FString(TEXT("REFERENCE SOUND: CAPTURE INTERRUPTED"))):FString();
}
bool FS47FreeformSession::QueueCapture(S47CaptureSound::FTrack& Track,double From,double To,double Dt,double Rate,const S47CaptureSound::FFrame& Input)
{
 if(!Track.Eligible()||Track.Complete)return false;
 if(!M||!M->Audio||!M->Audio->Ctx||!M->Audio->SfxBus){Track.Interrupt(TEXT("Receiver capture unavailable; retained measurements use compatible reference sound."));return false;}
 if(!LiveCapture)LiveCapture=MakeShared<FS47CapturePlayer>(M->Audio->Ctx.ToSharedRef(),M->Audio->SfxBus);
 if(!Track.Append(From,To,Dt,Rate,Input.Frequency,Input.Gain,Input.Bandwidth,Input.Quality))
 {Track.Interrupt(TEXT("Receiver capture was interrupted; retained measurements use compatible reference sound."));LiveCapture->Stop();return false;}
 // Live blocks need real-time signal. A worn array records slower, so only the tape keeps the capture.
 if(Rate<1-1e-6&&!Track.bMonitorOnly){Track.bMonitorOnly=true;LiveCapture->Stop();}
 const bool bLive=!Track.bMonitorOnly;
 if(bLive&&!LiveCapture->Queue(Track))
 {Track.Interrupt(TEXT("Receiver capture missed its audio deadline; retained measurements use compatible reference sound."));LiveCapture->Stop();return false;}
 if(To>=Track.Duration-1e-6)Track.Complete=true;
 return bLive;
}
void FS47FreeformSession::InterruptCapture(const FString& Why)
{
 bool Active=false;if(auto* J=Job()){Active=(J->CaptureOriginal.Eligible()&&!J->CaptureOriginal.Complete)||(J->CaptureControl.Eligible()&&!J->CaptureControl.Complete);J->CaptureOriginal.Interrupt(Why);J->CaptureControl.Interrupt(Why);}
 if(Active&&LiveCapture)LiveCapture->Stop();
}
void FS47FreeformSession::Step(double Dt)
{
 if(!bActive||!M||!M->Web||!M->Story||!FMath::IsFinite(Dt)||Dt<=0)return;
 // FIX-20261110 (Tom, point 4): the first step inside the dark station says where the switches are, once.
 if(bMailboxStart&&!bDarkTold&&!bStationLocked&&M->RoomLights&&S47SaroSave::InStation(M))
 {
  bDarkTold=true;
  if(!M->RoomLights->ManualEnabled(TEXT("corridor"))&&M->UI)M->UI->Toast(TEXT("Dark inside, and it smells of dust and warm electronics. The light switches glow orange on the walls."),7);
 }
 if(bRestoreAngles&&M->Web->IsLoaded())
 {
  bRestoreAngles=false;for(int32 I=0;I<27;++I){M->Web->SnapDish(I,SavedAngles[I*4],SavedAngles[I*4+1]);M->Web->PointDish(I,SavedAngles[I*4+2],SavedAngles[I*4+3],9,true);}
 }
 const double D=FMath::Min(Dt,.1);
 if(!Held())StepSky(D);
 // FIX10-10: a lockout left by a passage that nobody mended: the county line crew restores the feeder in the end.
 if(bPowerFault&&SkyFaultUntil>0&&WorldClock>=SkyFaultUntil)
 {
  SetPowerFault(false);
  const bool bHere=M->Web->Area()==TEXT("saro");
  if(bHere&&M->UI)M->UI->Toast(TEXT("The county line crew has been out to the feeder. Relays clatter through the rack and the tubes strike: the mains are back."),8);
  UE_LOG(LogTemp,Display,TEXT("S47: sky cut: the county line crew restored the feeder"));
 }
 const bool Powered=HasPower();
 if(Dt>.100001||ArrayMotionHeld()||!Powered)InterruptCapture(TEXT("Continuous receiver capture interrupted; retained measurements use compatible reference sound."));
 if(CaptureStartStage&&(!Powered||Held()||!bAudioOpen||!AudioDesk||!AudioDesk->ListeningPosition()))
 {NoteAudioStop(TEXT("start cancelled: held, unpowered or away from the deck"),Dt);CaptureStartStage=0;if(TapeCapture)TapeCapture->Stop();bAudioPlaying=false;}
 if(Dt>.100001&&UsesCapturedAudio()&&TapeCapture&&bAudioPlaying){NoteAudioStop(TEXT("timing: frame over 100 ms"),Dt);TapeCapture->Stop();bAudioPlaying=false;Changed(TEXT("Playback paused after a timing interruption. Press PLAY to resume at the retained position."));}
 if(TapeCapture&&(!Powered||Held()||!bAudioOpen||!AudioDesk||!AudioDesk->ListeningPosition()||(!bAudioPlaying&&AudioCursor<(AudioSourceIndex==0?18.:8.)-.001)))TapeCapture->Stop();
 if(!bPowerApplied||Powered!=bLastPower)
 {
  // Keep physical circuits and telemetry in agreement; retained targets resume
  // only once the feeder or fuelled backup supplies the actual hardware again.
  TArray<FVector2D> Targets;for(int32 I=0;I<M->Web->DishCount();++I)Targets.Add({M->Web->DishTargetAz(I),M->Web->DishTargetEl(I)});
  M->Story->ApplyFeederPower(!Powered);
  for(int32 I=0;I<Targets.Num();++I)
  {
   const bool Local=Field&&Field->ServiceStage()>=7&&Field->ServiceStage()<=8&&M->Web->DishId(I)==Field->ServiceDishId();
   M->Web->PointDish(I,Targets[I].X,Targets[I].Y,Local||bCrossRunning?2.2:9.,true);
  }
  if(bPowerApplied&&!bLastPower&&Powered)S47FreeformTerminal::PowerOn(); // D45-20261110: the sets come on with the supply (not on load, not on sitting down)
  M->Story->Rx.bPowered=Powered;bPowerApplied=true;bLastPower=Powered;
  if(!Powered&&TerminalOnGlass()&&M->UI)M->UI->Close();
 }
 M->Web->SetWorkMotionPaused(ArrayMotionHeld());
 if(!Held())
 {
  WorldClock=AdvanceClock(WorldClock,D);int32 NowDay=FMath::Max(1,int32(FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay))+1);
  if(NowDay!=DayNumber){DayNumber=NowDay;AddDay(DayNumber);const int32 Lapsed=LapseUntakenCards();Changed(Lapsed>0?FString::Printf(TEXT("New daily observing jobs are on the operations terminal. %d card%s nobody began lapsed at 08:00; work under way remains available."),Lapsed,Lapsed==1?TEXT(""):TEXT("s")):FString(TEXT("New daily observing jobs are on the operations terminal. Work under way remains available.")));}
  M->Story->SetClock(FMath::Fmod(WorldClock,86400.));
  AutosaveAt+=D;if(AutosaveAt>=30){AutosaveAt=0;Changed();}
  StepCaseReplies();
  StepWeeklyReview();
 }
 if(!ArrayMotionHeld())StepWork(D);
 StepSkyFix(); // RX-ACQUIRE-SKY-20261113: a lock on the sky chart becomes the job's fix, and the page goes over to TUNE
 if(AudioDesk)AudioDesk->Step(D);
 if(Decor)Decor->Step();
 if(FarmSite)FarmSite->Step(D);
 if(Taken)Taken->Step(D);
 if(Aftermath)Aftermath->Step(D);
 if(Deputy)Deputy->Step(D);
 if(Gear)Gear->Step(D);
 if(Places)Places->Step(D);
 if(Balloon)Balloon->Step(D); // D48-20261110: the beacon, the box's errands
 if(Predecessor)Predecessor->Step(D);
 if(Mail)Mail->Step(D);
 if(Tips&&!Held())Tips->Step(D);
 if(TownUse)TownUse->Step(D);
 if(TownSites)TownSites->Step(D);
 if(Townsfolk&&!Held())Townsfolk->Step(D);
 if(Visitors&&!Held())Visitors->Step(D);
 if(MotelNight)MotelNight->Step(D);
 if(TruckLamp)TruckLamp->Step(D);
 if(Outfit)Outfit->Step(D);
 if(YardAlarm)YardAlarm->Step(D);
 if(Care&&!Held())Care->Step(D);
 if(TripCamera)TripCamera->Step(D);
 if(Round&&!Held())Round->Step(D);
 if(RoadCar)RoadCar->Step(D);
 if(Happenings)Happenings->Step(D);
 if(WeekCase)WeekCase->Step(D);
 if(Scripts)Scripts->Step(D);
 if(NightLog)NightLog->Step(D);
 if(Tape)Tape->Step(Dt);
 if(PinBoard&&Active()){MinuteLook+=Dt;if(MinuteLook>=1.){MinuteLook=0;if(PinBoard->AtSaro())AskWhatCameBack();}} // D46-20261110: the same-minute question at SARO, once a second, now that no pinboard asks it
 if(Darkroom)Darkroom->Step(D);
 if(WardReport)WardReport->Step(Dt);
 if(SkyAnswer)SkyAnswer->Step(Dt);
 if(Weather)Weather->Step(D);
 if(Fatigue)Fatigue->Step(D);
 if(Handling)Handling->Step(D);
 if(Cot)Cot->Step(D);
 if(Hide)Hide->Step(D); // N1110
 if(Attention&&!Held())Attention->Step(D); // N1110
 if(DoorLoop&&!Held())DoorLoop->Step(D); // N1110 (N1110-6)
 if(IndoorDread)IndoorDread->Step(D);
 if(FarmNight)FarmNight->Step(D);
 if(HouseDoors)HouseDoors->Step(D);
 if(bAudioOpen&&!Held())StepAudio(CaptureStartStage?Dt:D);
 TransportSound();
 StepDriveCheck(Dt);
 if(M->Audio&&(!bAudioPlaying||!bAudioOpen||Held()||!HasPower()||!AudioDesk||!AudioDesk->ListeningPosition()))M->Audio->ReceiverRecordingOff();
 // D48-20261110 (D46-15): an unusual take that points out at the land (extra ticks, no instrument cause) carries the night's visitor
 // under its noise, round the ticks (S47AlienKinds): live at the receiver while it records, again at the reel deck while the original
 // plays there.
 // The kind is the night's lean, the level the tier of the hour it was recorded; nothing new is saved.
 if(M->Audio)
 {
  const FJob* SigJob=Job();
  const bool bSigTake=SigJob&&SigJob->bAnomaly&&SigJob->Phenomenon.IsEmpty()&&SigJob->CaseTest<0&&SigJob->ReferenceClock>0; // (LOOP-ANOMALY-ANY-CARD-20261113: on any card that carries it)
  const int32 SigNight=bSigTake?NightOf(SigJob->ReferenceClock):1;
  const ES47AlienKind SigKind=S47AlienKinds::NightLean(Seed(),SigNight);
  const uint32 SigSeed=bSigTake?S47AlienKinds::TakeSeed(Seed(),SigJob->Id):0u;
  const float SigLevel=bSigTake?S47AlienKinds::TierLevel(S47NightTiers::AtClock(SigJob->ReferenceClock,SigNight)):0.f;
  const bool bSigLive=bSigTake&&SigJob->Stage==EJobStage::Integrating&&!ArrayMotionHeld()&&HasPower()&&Settle>=1&&M->Story&&M->Story->Rx.Pass(EffectiveProfile(*SigJob))
   &&!Held()&&TerminalOnGlass()&&ActiveTerminal==1&&M->World&&M->World->Area==TEXT("saro"); // heard where the monitor is heard
  const bool bSigReel=bSigTake&&bAudioPlaying&&bAudioOpen&&!Held()&&HasPower()&&AudioDesk&&AudioDesk->ListeningPosition()&&AudioSourceIndex==0;
  S47AlienKinds::Follow(M->Audio,TEXT("take"),bSigLive,SigJob?SigJob->Elapsed:0.,SigKind,SigSeed,SigLevel);
  S47AlienKinds::Follow(M->Audio,TEXT("reel"),bSigReel,AudioCursor,SigKind,SigSeed,SigLevel);
 }
 if(Field)Field->Step(D);
 if(CaseField)CaseField->Step(D);
 StepLandTest(); // LOOP-WHOLE-20261109
 if(Director)Director->Step(D);
 StepPrinter(D);
 StepSopCard();
 StepTelex(D); // TELEX-20261109
 IngestObservations();
 StepEquipmentFeedback();
 StepTutorial();
 StepDaleAsks(); // TASKS-TEN-20261113 (change 7)
 StepPin(); // TASKS-TEN-20261113 (change 10)
 // Unlike the original Story console, the physical Freeform receiver owns its monitor here.
 if(M->Audio)
 {
  const FJob* Job=CurrentJob();
  const bool Listen=Job&&Powered&&M->Story->Rx.bPowered&&!Held()&&TerminalOnGlass()&&ActiveTerminal==1&&M->World&&M->World->Area==TEXT("saro");
  if(LiveCapture)LiveCapture->Audible(Listen);
  if(LiveCapture&&LiveCapture->Pending())M->Audio->ReceiverMonitorOff();
  // D45-20261110: what is heard fades in over the slew instead of stepping in at lock (the capture keeps ReceiverReception)
  else if(Listen)M->Audio->MonitorReceiver(M->Story->Rx,EffectiveProfile(*Job),FString::Printf(TEXT("freeform-source-%d"),Job->Source),FMath::Max(ReceiverReception(),ReceiverApproach()*.6));
  else M->Audio->ReceiverMonitorOff();
 }
 StepRxSearch(D); // RX-SEARCH-SOUND-20261113: Tom's search layer, heard where the monitor is (S47FreeformSky.cpp)
 // M7-HUD-20261109: the card gets the short line; the pause menu and the notebook keep the whole of it
 // TASKS-TEN-20261113 (change 5): one line only: the second line for a case (HudCaseLine) is gone; the cases are on NOW's open matters
 // TASKS-TEN-20261113 (change 10): with a pin, one line beside it when something else closes within 30 minutes
 if(M->UI&&M->Mode==US47Main::EMode::Play){const FString Card=HudObjective();const FString Soon=bPinLive?CloseSoonLine():FString();M->UI->Objective(Soon.IsEmpty()?Card:Card+TEXT("\n")+Soon);}
}
int32 FS47FreeformSession::PendingPrint(const FJob& J)
{
 if(J.Stage==EJobStage::Chart)return 0;
 if(J.Stage==EJobStage::Registered)return 1;
 if(J.Stage==EJobStage::Recorded&&J.Phenomenon==TEXT("duplicate_print")&&J.bControl&&J.ControlAt>=8&&!J.bDuplicateCollected&&!J.bDuplicateRejected)return 2;
 return -1;
}
double& FS47FreeformSession::PrintFeed(FJob& J,int32 Kind){return Kind==0?J.ChartFeed:Kind==1?J.PacketFeed:J.DuplicateFeed;}
double FS47FreeformSession::PrintFeed(const FJob& J,int32 Kind){return Kind==0?J.ChartFeed:Kind==1?J.PacketFeed:J.DuplicateFeed;}
int32 FS47FreeformSession::PrinterIndex()const
{
 // A started sheet stays on the platen until collected, regardless of selected job.
 int32 First=INDEX_NONE;
 for(int32 I=0;I<Work.Num();++I)if(const int32 Kind=PendingPrint(Work[I]);Kind>=0)
 {
  if(PrintFeed(Work[I],Kind)>0)return I;
  if(First==INDEX_NONE)First=I;
 }
 return First;
}
FString FS47FreeformSession::PrinterJobId()const{const int32 I=PrinterIndex();return I==INDEX_NONE?FString():Work[I].Id;}
double FS47FreeformSession::PrinterProgress()const{const int32 I=PrinterIndex();return I==INDEX_NONE?-1.:PrintFeed(Work[I],PendingPrint(Work[I]))/PrinterSeconds;}
int32 FS47FreeformSession::SheetIndex()const
{
 // CASES-OPS-20261109: a result sheet waits behind any job's paper, so that the work's own papers keep their order.
 return CaseSheets.IsEmpty()||PrinterIndex()!=INDEX_NONE?-1:0;
}
double FS47FreeformSession::SheetProgress()const{return CaseSheets.IsEmpty()?-1.:CaseSheets[0].Feed/PrinterSeconds;}
FString FS47FreeformSession::PrinterHint()const
{
 const int32 I=PrinterIndex();
 if(I==INDEX_NONE&&!CaseSheets.IsEmpty())
 {
  const FResultSheet& Sh=CaseSheets[0];
  if(IsBearingSlip(Sh))return Sh.Feed>=PrinterSeconds?FString(TEXT("PRINTER: Collect the bearing slip."))
   :FString::Printf(TEXT("PRINTER: bearing slip %s. %s"),HasPower()?TEXT("feeding paper"):TEXT("feed paused"),HasPower()?TEXT("Wait for the printer to finish before collecting."):TEXT("Restore power to finish; progress is retained.")); // D45-20261110: no ticking figure on the task card
  if(Sh.Feed>=PrinterSeconds)return FString::Printf(TEXT("PRINTER: Collect the result sheet for %s."),FS47FreeformCases::Def(Sh.Line).Title); // Tom 13 Nov: cases carry no number
  return FString::Printf(TEXT("PRINTER: case result sheet %s. %s"),HasPower()?TEXT("feeding paper"):TEXT("feed paused"),HasPower()?TEXT("Wait for the printer to finish before collecting."):TEXT("Restore power to finish; progress is retained."));
 }
 if(I==INDEX_NONE)return TEXT("PRINTER: No paper waiting.");
 const auto& J=Work[I];const int32 Kind=PendingPrint(J);const double Progress=PrintFeed(J,Kind);
 if(Progress>=PrinterSeconds)return TEXT("PRINTER: Collect ")+J.Id+(Kind==0?TEXT(" original chart."):Kind==1?TEXT(" registered packet."):TEXT(" separate duplicate; retain the original."));
 return FString::Printf(TEXT("PRINTER: %s %s. %s"),*J.Id,HasPower()?TEXT("feeding paper"):TEXT("feed paused"),HasPower()?TEXT("Wait for the printer to finish before collecting."):TEXT("Restore power to finish; progress is retained."));
}
void FS47FreeformSession::StopPrinterFeedback()
{
 if(bPrinterFeedback&&!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&IsValid(M->Audio))M->Audio->Stop(TEXT("ffPrinter"),.04f);
 bPrinterFeedback=false;
}
void FS47FreeformSession::StepPrinter(double Dt)
{
 const int32 I=PrinterIndex();
 // CASES-OPS-20261109: with no job's paper waiting, a case's result sheet (SheetIndex) feeds the same way, with the same sound.
 const int32 Sheet=I==INDEX_NONE?SheetIndex():-1;
 const bool Running=(I!=INDEX_NONE||Sheet>=0)&&HasPower()&&!Held();
 if(Running)
 {
  double& Feed=I!=INDEX_NONE?PrintFeed(Work[I],PendingPrint(Work[I])):CaseSheets[Sheet].Feed;const double Before=Feed;
  Feed=FMath::Min(PrinterSeconds,Feed+FMath::Max(0.,Dt));
  if(Before<PrinterSeconds&&Feed>=PrinterSeconds)Changed(PrinterHint());
 }
 const double Progress=I!=INDEX_NONE?PrinterProgress():SheetProgress();
 const bool OnSite=M&&M->Web&&M->Web->IsLoaded()&&M->Web->Area()==TEXT("saro")&&M->World&&M->World->Area==TEXT("saro");
 if(OnSite)M->Web->SetPrinterPaper(Progress<0?-1.f:FMath::Clamp(float(Progress),.001f,1.f));
 const bool Audible=Running&&Progress<1&&OnSite&&M->Audio;
 if(Audible&&!bPrinterFeedback){M->Audio->Loop(TEXT("ffPrinter"),TEXT("printer"),.8f,M->Web->PropLocation(TEXT("printer")));bPrinterFeedback=true;}
 else if(!Audible)StopPrinterFeedback();
}
void FS47FreeformSession::StopEquipmentFeedback()
{
 if(bMotorFeedback&&!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&IsValid(M->Audio))M->Audio->StopMotors();
 bMotorFeedback=false;
}
void FS47FreeformSession::CommandFeedback()
{
 if(!M||!M->Web||!M->Audio||ArrayMotionHeld()||M->Web->Area()!=TEXT("saro"))return;
 bool Accepted=false;for(int32 I=0;I<M->Web->DishCount();++I)Accepted|=M->Web->DishRemoteAvailable(I);
 if(!Accepted)return;
 M->Audio->RelayClack(M->Story->RackAt(),false);++CommandClicks;
}
void FS47FreeformSession::StepEquipmentFeedback()
{
 const bool Available=M&&M->Web&&M->Web->IsLoaded()&&M->Web->Area()==TEXT("saro")&&M->World&&M->World->Area==TEXT("saro")&&M->Mode==US47Main::EMode::Play&&!ArrayMotionHeld()&&HasPower();
 TArray<FVector2D> Current;
 if(M&&M->Web&&M->Web->IsLoaded())for(int32 I=0;I<M->Web->DishCount();++I)Current.Add({M->Web->DishAz(I),M->Web->DishEl(I)});
 bool Moved=false;
 if(Available&&Current.Num()==FeedbackEncoders.Num())for(int32 I=0;I<Current.Num();++I)Moved|=EncoderChanged(FeedbackEncoders[I],Current[I]);
 // Rebaseline even while held: restoring an encoder pose or returning to SARO
 // must not sound like a movement that occurred in front of the player.
 FeedbackEncoders=MoveTemp(Current);
 if(Moved&&M->Audio&&!bMotorFeedback){M->Audio->StartMotors();bMotorFeedback=true;}
 else if(!Moved)StopEquipmentFeedback();
 if(Field)
 {
  const int32 Completed=Field->ServiceCompletions();
  if(Completed>LastServiceFeedback&&Available&&M->Audio)
  {
   // The saved World counter changes only after the real out-and-back test.
   const FVector At=Field->InteractionPoint(TEXT("ff:service"));
   M->Audio->RelayClack(At,false);++ServiceClicks;
  }
  LastServiceFeedback=Completed;
 }
}
void FS47FreeformSession::StepWork(double Dt)
{
 FJob* J=Job();if(!J)return;
 if(J->Stage==EJobStage::New&&!ArrayJob.IsEmpty()&&ArrayJob!=J->Id)ReleaseForNewJob(); // D48-20261110: a job selected some other way (planned case test, OPS)
 S47CaptureSound::FFrame CaptureInput;
 if(M&&M->Story){CaptureInput.Frequency=M->Story->Rx.Frequency;CaptureInput.Gain=M->Story->Rx.Gain;CaptureInput.Bandwidth=M->Story->Rx.Bandwidth;CaptureInput.Quality=M->Story->Rx.Quality(EffectiveProfile(*J))*ReceiverReception();}
 if(J->Stage==EJobStage::Integrating&&J->CaptureOriginal.Eligible()&&(!PointReady()||Settle<1||!M||!M->Story||!M->Story->Rx.Pass(EffectiveProfile(*J))))InterruptCapture(TEXT("Receiver lock interrupted; retained measurements use compatible reference sound."));
 // Critic 12 Nov: a job back at the start is decided again when it is next tuned; tuned now, the take is decided once (DecideTake).
 if(J->Stage==EJobStage::New&&J->TakeDecided>=0){J->TakeDecided=-1;J->bTakeOut=false;J->TakePhen.Empty();}
 if(J->Stage==EJobStage::Pointed&&J->TakeDecided<0&&J->CaseTest<0&&Tutorial!=1&&ReceiverReady())DecideTake(*J);
 if(J->Stage==EJobStage::Pointed&&DetectedJob!=J->Id&&ReceiverReady()&&AnomalyAhead()){DetectedJob=J->Id;Changed(TEXT("ANOMALY DETECTED. RECEIVER (centre PC), RECORD tab: press START RECORDING."));if(M)S47Thoughts::Later(TEXT("anomaly"),1.5);} // UI-20261112: the player's own thought as the anomaly shows
 if(J->Stage==EJobStage::Pointed||J->Stage==EJobStage::Integrating)
 {
  Settle=PointReady()?FMath::Min(1.,Settle+Dt):0;
  if(J->Stage==EJobStage::Integrating&&J->CaseTest>=0&&J->bCaseValid)
  {
   const auto Cond=CaseCondition(J->CaseLine,J->CaseTest);
   // CASETEST-20261109: stopped at once and kept planned (SpoilCaseTest); the same job is recorded again.
   if(Cond==ECaseCondition::Generator&&Field&&M&&M->Web&&M->Web->IsLoaded()&&!Field->GeneratorRunning())SpoilCaseTest(*J,TEXT("the standby generator stopped, so the recording was no longer off the mains. Refuel or restart it behind the control room,"));
   // (judged only with SARO loaded and the player in it: a Continue's first frames, before the room is up, spoil nothing)
   else if(Cond==ECaseCondition::DoorsSealed&&M&&M->Web&&M->Web->IsLoaded()&&M->Player&&M->World&&M->World->Area==TEXT("saro")&&!StationSealed())SpoilCaseTest(*J,TEXT("the station was opened, or you left the control room, during the sealed-station test. Shut both doors and stay inside,"));
  }
  if(J->Stage==EJobStage::Integrating&&Settle>=1&&M&&M->Story&&M->Story->Rx.Pass(EffectiveProfile(*J)))
  {
   const double Was=J->Elapsed;
   J->Elapsed=FMath::Min(SourceSeconds(),J->Elapsed+Dt*ArrayMeasurementRate());
   const bool Captured=QueueCapture(J->CaptureOriginal,Was,J->Elapsed,Dt,ArrayMeasurementRate(),CaptureInput);
   if(bCrossRunning)
   {
    CrossSample+=Dt*ArrayMeasurementRate();if(CrossSample>=2){if(CrossPoint<4){++CrossPoint;PointCross();}else bCrossRunning=false;}
   }
   // LOOP-ANOMALY-ANY-CARD-20261113: the extra ticks are heard live on whichever card carries them; the pulsar's own beat on the pulsar's
   if((J->Source==1||PlainAnomaly(*J))&&M->Audio&&!Captured)
   {
    for(double At:J->TraceTicks)if(Was<At&&J->Elapsed>=At)M->Audio->Beep(1240,.008f,.1f,0,TEXT("square"));
    if(J->Source==1)for(double At=PulsarPeriod;At<=SourceSeconds();At+=PulsarPeriod)if(Was<At&&J->Elapsed>=At)M->Audio->Beep(820,.012f,.16f,0,TEXT("square"));
   }
   // GAMEPLAY-LOOP-01: the receiver says it as soon as the first extra tick is on tape (once a take)
   if(PlainAnomaly(*J)&&!J->TraceTicks.IsEmpty()&&Was<J->TraceTicks[0]&&J->Elapsed>=J->TraceTicks[0]&&M->UI)M->UI->Toast(TEXT("ANOMALY DETECTED / extra ticks off the work card. Let the take run."),4);
   if(J->Elapsed>=SourceSeconds()&&!bCrossRunning)
   {
    // GAMEPLAY-LOOP-01 (Tom, 12 November): the unusual take has no chart to fetch. RX-ANOMALY-FLOW-20261113 (Tom 13 November): it is
    // retained on the receiver's RECORD tab, with its 8-second reference (RetainTake), before the reel deck. The instrument
    // discrepancies and routine work keep their chart.
    if(PlainAnomaly(*J)){J->Stage=EJobStage::Recorded;Changed(TEXT("ANOMALY ON TAPE. RECEIVER (centre PC), RECORD tab: press RETAIN TAKE."));}
    else {J->Stage=EJobStage::Chart;Changed(TEXT("PRINTER: Recording complete; paper feed queued. Wait for the printer to finish, then collect the original chart."));}
   }
  }
 }
 if(J->bControl&&J->ControlAt<8)
 {
  if(J->Phenomenon==TEXT("wrong_source"))
  {
   const double Az=TargetAz()+BeamOffsets[J->BeamPoint];
   J->BeamSettle=EncodersAt(Az,TargetEl())?FMath::Min(1.,J->BeamSettle+Dt):0;
   if(J->BeamSettle>=1)
   {
    const double Sample=FMath::Min(Dt*ArrayMeasurementRate(),8./3.-J->BeamSample);
    J->BeamSample+=Sample;J->ControlAt=FMath::Min(8.,J->ControlAt+Sample);
    if(J->BeamSample>=8./3.-1e-6)
    {
     double ActualAz=0,ActualEl=0;int32 Count=0;
     for(int32 I=0;I<27;++I)if(M->Web->DishRemoteAvailable(I)){ActualAz+=M->Web->DishAz(I);ActualEl+=M->Web->DishEl(I);++Count;}
     ActualAz/=FMath::Max(1,Count);ActualEl/=FMath::Max(1,Count);
     J->BeamAz.Add(ActualAz);J->BeamEl.Add(ActualEl);J->BeamAmplitude.Add(BeamLevel(ActualAz,TargetAz()+.5));
     if(J->BeamPoint<2){++J->BeamPoint;PointBeamControl();}
     else {J->bBeamVerified=true;J->ControlAt=8;Changed(TEXT("OPS (corner PC): DAILY WORK / Select this job and press Compare and register the retained instrument measurements. West, east and centre sampling is complete; the array is back at centre."));if(Director)Director->Use(TEXT("ff:control"));}
    }
   }
  }
  else
  {
   const double ControlWas=J->ControlAt;
   J->ControlAt=FMath::Min(8.,J->ControlAt+Dt);
   CaptureInput.Quality=1.; // independently driven internal reference, not a sky reception claim
   QueueCapture(J->CaptureControl,ControlWas,J->ControlAt,Dt,1,CaptureInput); // The internal reference runs at full speed whatever the array can do.
   if(J->ControlAt>=8)
   {
    J->ControlReferenceClock=WorldClock;J->ControlReceiverClock=WorldClock+(J->Phenomenon==TEXT("time_disagreement")?3.2:0);J->bClockMeasured=J->Phenomenon==TEXT("time_disagreement");
    if(J->bClockMeasured)Changed(TEXT("OPS (corner PC): DAILY WORK / Select this job and press Compare and register the retained instrument measurements to compare original and control clock offsets."));
    else if(J->Phenomenon==TEXT("duplicate_print"))Changed(TEXT("PRINTER: Register check complete; duplicate feed queued. Wait for the printer to finish, then collect and compare it with the retained original at OPS (corner PC)."));
    else if(PlainAnomaly(*J))Changed(J->bAudioReviewed?FString():FString(J->bPrinted?TEXT("REFERENCE ON TAPE. NEXT: REEL DECK (wall table), play the take and its reference, mark the extra ticks, press RETAIN."):TEXT("REFERENCE ON TAPE. RECEIVER (centre PC), RECORD tab: press RETAIN TAKE."))); // RX-ANOMALY-FLOW-20261113
    else Changed(TEXT("REEL DECK (wall table): Review the original and control recordings; the internal control is complete."));
    // (D45-20261110: a control recorded before the chart was collected says the same; the chart waits at the printer for OPS)
    if(Director)Director->Use(TEXT("ff:control"));
   }
  }
 }
}
void FS47FreeformSession::PointCross()
{
 Settle=CrossSample=0;if(M&&M->Web)M->Web->PointDishes(TargetAz()+Cross[CrossPoint].X,TargetEl()+Cross[CrossPoint].Y,2.2);
}
void FS47FreeformSession::PointBeamControl()
{
 FJob* J=Job();if(!J)return;J->BeamSettle=J->BeamSample=0;
 if(M&&M->Web)M->Web->PointDishes(TargetAz()+BeamOffsets[J->BeamPoint],TargetEl(),2.2);
}
void FS47FreeformSession::ReleaseForNewJob()
{
 // D48-20261110 (Tom: many array tests in a row with only RECORDS and POINT ARRAY): a job is its own pointing and its own tuning. When the
 // last job's array and receiver would still serve this one (the same bearing, or a card that is the same), the array is released to
 // zenith and the receiver stands down, so this job has to be pointed and tuned for real. A different target needed the slew anyway.
 const FJob* J=CurrentJob();if(!J||ArrayJob.IsEmpty()||ArrayJob==J->Id||J->Stage!=EJobStage::New||!M||!M->Web||!M->Story||!M->Web->IsLoaded()){return;}
 ArrayJob.Empty();AimJob.Empty();
 bool bThere=false;int32 Count=0;
 for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishRemoteAvailable(I)){++Count;if(FMath::Abs(FMath::FindDeltaAngleDegrees(M->Web->DishAz(I),TargetAz()))<=.5&&FMath::Abs(M->Web->DishEl(I)-TargetEl())<=.5)bThere=true;else{bThere=false;break;}}
 auto& R=M->Story->Rx;const auto P=EffectiveProfile(*J);
 const bool bTuned=R.Pass(P);
 if(!(Count>0&&bThere)&&!bTuned)return;
 if(Count>0&&bThere)M->Web->PointDishes(0,90,9);
 if(bTuned)
 {
  R.Frequency=P.F+(P.F+.4<=S47Signal::FMax?.4:-.4);
  R.Gain=FMath::Clamp(P.Gain[0]>=10?P.Gain[0]-8:P.Gain[1]+8,0.,100.);
  R.Bandwidth=FMath::Clamp(P.Bw[0]>=10?P.Bw[0]-8:P.Bw[1]+8,0.,100.);
 }
 Feedback=TEXT("TRACKING (right-hand PC): The array was left on the last target and the receiver on its tuning. Point the array for this job and tune its work card.");
}
bool FS47FreeformSession::SelectJob(int32 Index)
{
 if(!Work.IsValidIndex(Index))return false;
 if(Tutorial>0&&Work[Index].Id!=TutorialJobId){Changed(TEXT("Finish your FIRST SHIFT job before switching work orders. The other jobs will still be waiting in DAILY WORK."));return false;}
 const FJob* Current=CurrentJob();if(Current&&(Current->Stage==EJobStage::Integrating||(Current->bControl&&Current->ControlAt<8))){Changed(TEXT("Finish the active recording or control before selecting another job."));return false;}
 CaptureStartStage=0;if(TapeCapture)TapeCapture->Stop();
 Selected=Index;FieldFocus=-1;bGuideFocus=false;Settle=0;bCrossRunning=false;bAudioPlaying=false;AudioCursor=0;
 // D45-20261110: the card's bearing is the receiver's bearing from the moment the job is chosen, so the target is on the receiver at
 // once; pointing is still judged by the encoders (PointReady, ReceiverReady).
 if(M&&M->Story)M->Story->Rx.Azimuth=TargetAz();
 ReleaseForNewJob(); // D48-20261110
 Changed();return true;
}
bool FS47FreeformSession::ChooseThird(bool bVirgo)
{
 FJob* J=Job();if(!J||J->Slot!=2||J->Stage!=EJobStage::New)return false;J->Source=bVirgo?3:2;J->Card=FS47FreeformJobs::Make(JobSeed,J->Day,J->Slot,J->Source);J->bGenerated=true;if(M&&M->Story)M->Story->Rx.Azimuth=TargetAz();Changed();return true; // D45-20261110
}
bool FS47FreeformSession::PointSelected()
{
 if(Tutorial==1){Feedback=TEXT("OPS (corner PC): Open DAILY WORK, read the job card and accept your first job before pointing the array.");Changed(Feedback);return false;}
 FJob* J=Job();if(!J||J->Stage>EJobStage::Pointed||!M||!M->Web||!HasPower())return false;
 if(EveningGated(*J)){Feedback=EveningLine(*J);Changed(Feedback);return false;}
 FieldFocus=-1;bGuideFocus=false;
 J->Stage=EJobStage::Pointed;Settle=0;bCrossRunning=false;ArrayJob=J->Id; // D48-20261110: this job now holds the array
 M->Story->Rx.Azimuth=TargetAz(); // The physical pointing command is the receiver bearing too.
 M->Web->PointDishes(TargetAz(),TargetEl(),9);CommandFeedback();Changed(SkyAcquired()?TEXT("TRACKING (right-hand PC): Command sent. Wait for encoder agreement, then tune and start recording at RECEIVER (centre PC)."):TEXT("TRACKING (right-hand PC): Command sent. Wait for encoder agreement, then find the source on the sky chart, tune and record at RECEIVER (centre PC)."));return true; // RX-ACQUIRE-SKY-20261113
}
bool FS47FreeformSession::ReceiverReady() const
{
 // RX-ACQUIRE-SKY-20261113: and the source found on the sky chart, where that rule is live (SkyAcquired).
 const FJob* J=CurrentJob();return J&&J->Stage==EJobStage::Pointed&&HasPower()&&PointReady()&&Settle>=1&&SkyAcquired()&&M&&M->Story&&M->Story->Rx.Pass(EffectiveProfile(*J));
}
FString FS47FreeformSession::ReceiverNextAction() const
{
 const FJob* J=CurrentJob();if(!J)return TEXT("OPS (corner PC): Select an observation in DAILY WORK.");
 if(EveningGated(*J))return AvailableActivities();
 if(J->Stage==EJobStage::Chart||J->Stage==EJobStage::Packet||J->Stage==EJobStage::Filed)return JobNextAction(*J);
 if(!HasPower()){if(PrinterProgress()>=1)return PrinterHint();return SkyMainsOut()&&!bPowerFault?TEXT("MAINS OUT, NO FAULT ON THE FEEDER YET. The fuelled standby generator behind the control room carries the station."):TEXT("Restore the SARO feeder, or start the fuelled standby generator outside behind the control room.");}
 if(J->Stage>=EJobStage::Chart)return JobNextAction(*J);
 // A case test states its physical precondition before the player commits the array.
 if(J->CaseTest>=0&&J->Stage<=EJobStage::Pointed){const FString Missing=CasePrecondition(*J);if(!Missing.IsEmpty())return Missing;}
 if(M&&M->Web&&J->Stage<=EJobStage::Integrating&&ArrayBlocked())return ArrayMeasurementStatus();
 if(J->Stage==EJobStage::New)return TEXT("TRACKING (right-hand PC): Point the array at the selected source.");
 if(J->Stage==EJobStage::Integrating)
 {
  if(!M||!M->Story||!M->Story->Rx.Pass(EffectiveProfile(*J)))return TEXT("RECEIVER (centre PC): Restore the work card tuning to resume recording; valid seconds are retained.");
  // Critic 12 Nov: the extra-tick take keeps its original as it ends (RetainOriginal): its next place is the reel deck, not the printer.
  if((!PointReady()||Settle<1)&&bCrossRunning&&PlainAnomaly(*J))return TEXT("TRACKING (right-hand PC): Automatic reference scan in progress. Wait for the samples; the take goes on, then RETAIN TAKE on the RECORD tab.");
  if(!PointReady()||Settle<1)return bCrossRunning?TEXT("TRACKING (right-hand PC): Automatic reference scan in progress. Wait for the samples; collect the original at the PRINTER when recording finishes."):TEXT("TRACKING (right-hand PC): Restore array pointing to resume recording; valid seconds are retained.");
 }
 // GAMEPLAY-LOOP-01: the unusual take says so for as long as it is written
 if(J->Stage==EJobStage::Integrating&&PlainAnomaly(*J))return TEXT("RECEIVER (centre PC): ANOMALY DETECTED. Let the take run to the end; then RETAIN TAKE on the RECORD tab.");
 // D45-20261110 (Tom: the task card flickered with the count): the line names the step, the receiver's own readout counts.
 if(J->Stage==EJobStage::Integrating)return FString::Printf(TEXT("RECEIVER (centre PC): Recording at the receiver (%d/27 antennas). Keep the tuning, pointing and power valid until the take ends."),AvailableDishCount());
 if(J->Stage!=EJobStage::Pointed)return JobNextAction(*J);
 if(!PointReady())return TEXT("TRACKING (right-hand PC): Wait for the commanded direction. If movement stops, inspect isolated elements at ARRAY STATUS (left-hand PC).");
 if(Settle<1)return SkyAcquired()?TEXT("RECEIVER (centre PC): Wait one second for the array to settle, then tune the work card."):TEXT("RECEIVER (centre PC): Wait one second for the array to settle, then find the source on the ACQUIRE sky chart.");
 // RX-ACQUIRE-SKY-20261113: the array is on the card; the receiver has to find the source on its chart before the frequency search
 if(!SkyAcquired())return TEXT("RECEIVER (centre PC): ACQUIRE. Steer the reticle to where the bearings cross.");
 const auto P=EffectiveProfile(*J);const auto& R=M->Story->Rx;
 if(R.Pass(P))return AnomalyAhead()?TEXT("RECEIVER (centre PC): ANOMALY DETECTED. RECORD tab: press START RECORDING, then RETAIN TAKE."):TEXT("RECEIVER (centre PC): Press START RECORDING to record this source.");
 if(FMath::Abs(R.Frequency-P.F)>P.FTol)return FString::Printf(TEXT("RECEIVER (centre PC): Tune FREQUENCY to %.3f MHz (now %.3f). Watch the carrier peak and received percentage."),P.F,R.Frequency);
 if(R.Gain<P.Gain[0]||R.Gain>P.Gain[1])return FString::Printf(TEXT("RECEIVER (centre PC): Set GAIN between %.0f and %.0f (now %.0f)."),P.Gain[0],P.Gain[1],R.Gain);
 if(R.Bandwidth<P.Bw[0]||R.Bandwidth>P.Bw[1])return FString::Printf(TEXT("RECEIVER (centre PC): Set BANDWIDTH between %.0f and %.0f kHz (now %.0f)."),P.Bw[0],P.Bw[1],R.Bandwidth);
 return TEXT("RECEIVER (centre PC): Fine-tune until the received carrier reaches 100%.");
}
// RX-TABS-DUE-20261113 (Tom 13 Nov: «På receiver: tab som Tuning etc. må blinke når de skal brukes»): ReceiverNextAction's steps, in its
// order, as the tab that holds each one. A step done elsewhere (tracking, the reel deck, OPS, the printer, the binder) or a wait marks
// no tab; nor does the tab already open (its own key carries the cue there: START RECORDING, RECORD INTERNAL CONTROL).
int32 FS47FreeformSession::RxDueTab(const FRxDueIn& In)
{
 auto Pick=[&In](int32 Tab){return Tab==In.Open?-1:Tab;};
 if(!In.bJob||In.bGated||In.bTutorialGate)return -1;
 // After the take: JobNextAction. Only the internal control is the receiver's (its RECORD tab); an unusual take with its case open
 // may be compared on DATA until that page has been seen.
 // RX-ANOMALY-FLOW-20261113: the extra-tick take is retained on RECORD before anything else.
 if(In.Stage>=EJobStage::Chart)return In.bRetain||In.bCanControl?Pick(RxRecord):In.bDataAsk?Pick(RxData):-1;
 if(!In.bPower||In.bCaseWaits)return -1;
 // Tom 13 Nov («ARRAY ikke pulser før tracking»): the pointing is done at the TRACKING PC, so before it nothing is due here, not
 // even ARRAY for too few antennas (TRACKING's own line says so); from Pointed on the rules below hold.
 if(In.Stage==EJobStage::New)return -1;
 if(In.bArrayBlocked)return Pick(RxArray); // isolated antennas: the ARRAY tab shows which
 if(In.Stage==EJobStage::Integrating)
 {
  if(!In.bTuned)return Pick(RxTune);
  if(!In.bPointReady||!In.bSettled)return In.bCrossRunning?-1:Pick(RxArray); // the reference scan is a wait
  return In.bPlainTake?Pick(RxRecord):-1; // recording: keep it valid (RX-ANOMALY-FLOW-20261113: the extra-tick take is retained on RECORD next)
 }
 if(!In.bPointReady)return Pick(RxArray); // slewing
 if(!In.bSky)return Pick(RxAcquire);
 // tune the card, then START RECORDING on TUNE; RX-ANOMALY-FLOW-20261113 (Tom): ANOMALY DETECTED leads to RECORD, which records and retains it
 return In.bAnomalyAhead?Pick(RxRecord):Pick(RxTune);
}
bool FS47FreeformSession::RxDataAsk(const FJob& J)const
{
 if(!PlainAnomaly(J)||J.TraceTicks.IsEmpty()||J.Stage<EJobStage::Chart||RxDataSeenJob==J.Id)return false;
 const auto* C=Cases.Get(0);return C&&C->bOpen&&!C->bConcluded&&Cases.RefTriesLeft(0)>0;
}
int32 FS47FreeformSession::ReceiverDueTab()const
{
 // Asked by each tab every frame: worked out once a frame.
 if(RxDueFrame==GFrameCounter)return RxDueCache;
 FRxDueIn In;In.Open=ReceiverTab;
 if(const FJob* J=CurrentJob())
 {
  In.bJob=true;In.Stage=J->Stage;In.bGated=EveningGated(*J);In.bTutorialGate=Tutorial==1;In.bPower=HasPower();
  In.bCaseWaits=J->CaseTest>=0&&J->Stage<=EJobStage::Pointed&&!CasePrecondition(*J).IsEmpty();
  In.bArrayBlocked=M&&M->Web&&J->Stage<=EJobStage::Integrating&&ArrayBlocked();
  In.bPointReady=PointReady();In.bSettled=Settle>=1;In.bSky=SkyAcquired();
  In.bTuned=M&&M->Story&&M->Story->Rx.Pass(EffectiveProfile(*J));
  In.bCrossRunning=bCrossRunning;In.bCanControl=CanRecordControl();In.bDataAsk=RxDataAsk(*J);
  In.bAnomalyAhead=J->Stage==EJobStage::Pointed&&ReceiverReady()&&AnomalyAhead();In.bPlainTake=J->Stage==EJobStage::Integrating&&PlainAnomaly(*J);In.bRetain=RetainWaits(*J);
 }
 RxDueFrame=GFrameCounter;RxDueCache=RxDueTab(In);return RxDueCache;
}
bool FS47FreeformSession::OpsWorkDueFor(EJobStage Stage,const FString& Next)
{
 return (Stage==EJobStage::Chart||Stage==EJobStage::Recorded)&&(Next.StartsWith(TEXT("OPS (corner PC)"))||Next.StartsWith(TEXT("OPS / DAILY WORK")));
}
bool FS47FreeformSession::OpsWorkDue()const
{
 const FJob* J=CurrentJob();return J&&Tutorial==0&&OpsWorkDueFor(J->Stage,JobNextAction(*J));
}
bool FS47FreeformSession::FirstPulseDue(const FJob& Candidate) const
{
 // The game's first question: the first pulsar anyone records here has the extra ticks on it. Never again by this rule, and
 // never in a save that has already recorded one. Developer fixtures keep their own (TestFirstPulse).
 if((M&&M->Flow&&M->Flow->bDevRun&&!bTestFirstPulse)||(!M&&!bTestFirstPulse))return false;
 if(Tutorial>0||Candidate.Source!=1||Candidate.Slot!=1||Candidate.CaseTest>=0||Findings.Contains(TEXT("pulse-review")))return false;
 return !Work.ContainsByPredicate([&](const FJob& J){return &J!=&Candidate&&J.Source==1&&J.Stage>EJobStage::Pointed;});
}
bool FS47FreeformSession::StartAcquisition()
{
 if(Tutorial==1){Feedback=TEXT("Accept your FIRST SHIFT job in OPS / DAILY WORK before recording.");return false;}
 FJob* J=Job();if(!ReceiverReady()){Feedback=ReceiverNextAction();return false;}
 if(J->CaseTest>=0)
 {
  if(!Cases.Get(J->CaseLine)||!Cases.Get(J->CaseLine)->bOpen||Cases.Get(J->CaseLine)->bConcluded){Feedback=TEXT("This case test no longer belongs to an open case.");return false;}
  const FString Missing=CasePrecondition(*J);if(!Missing.IsEmpty()){Feedback=Missing;Changed(Feedback);return false;}
  if(CaseCondition(J->CaseLine,J->CaseTest)==ECaseCondition::RelaySwap&&!Consume(TEXT("relay"))){Feedback=TEXT("Case test: no spare relay in the depot. Buy one at Caliche.");Changed(Feedback);return false;}
 }
 FieldFocus=-1;bGuideFocus=false;
 FString TakePhenomenon;bool bOut=false;
 // GAMEPLAY-LOOP-01: the receiver said ANOMALY DETECTED by the same rule; critic 12 Nov: by the same answer, decided once (DecideTake).
 const bool bOdd=DecidedTake(*J,TakePhenomenon,bOut);
 J->Stage=EJobStage::Integrating;J->Elapsed=0;J->bAnomaly=bOdd;J->Phenomenon=TakePhenomenon;
 J->bTakeOut=bOdd&&bOut; // LOOP-SLIP-IN-PACKET-20261113: kept past the pointing, so the packet's bearing can say it is the same thing
 if(bOut)UE_LOG(LogTemp,Display,TEXT("S47: freeform: card %s (slot %d, source %d) shows a discrepancy from what is out there tonight"),*J->Id,J->Slot+1,J->Source);
 J->TraceTicks.Reset();J->bLegacyTrace=false;if(J->bAnomaly&&!Specialized(J->Phenomenon))for(double At:Ticks)J->TraceTicks.Add(At);
 J->ReferenceClock=WorldClock;J->ReceiverClock=WorldClock+(J->Phenomenon==TEXT("time_disagreement")?3.2:0);
 // The hidden cause decides what the physical test actually records; the player hears or reads it.
 if(J->CaseTest>=0)
 {
  J->bCaseValid=true;const auto* C=Cases.Get(J->CaseLine);const bool bPresent=C&&FS47FreeformCases::PresentFor(J->CaseLine,C->Truth,J->CaseTest);
  if(FS47FreeformCases::Def(J->CaseLine).Observable==FS47FreeformCases::EObservable::ExtraTicks){if(bPresent)for(double At:Ticks)J->TraceTicks.Add(At);}
  else J->ReceiverClock=WorldClock+(bPresent?FS47FreeformCases::ClockOffset:0);
 }
 int32 Available=0;for(int32 I=0;I<27;++I)Available+=M->Web->DishRemoteAvailable(I)?1:0;J->Quality=double(Available)/27;
 double ActualAz=0;for(int32 I=0;I<27;++I)if(M->Web->DishRemoteAvailable(I))ActualAz+=M->Web->DishAz(I);
 ActualAz/=FMath::Max(1,Available);J->ReceivedLevel=BeamLevel(ActualAz,TargetAz()+(J->Phenomenon==TEXT("wrong_source")?.5:0));
 J->RecordingSoundVersion=2;J->RecordingGain=FMath::Clamp(M->Story->Rx.Gain,0.,100.);J->RecordingBandwidth=FMath::Clamp(M->Story->Rx.Bandwidth,0.,100.);
 J->RecordingQuality=FMath::Clamp(M->Story->Rx.Quality(EffectiveProfile(*J))*ReceiverReception(),0.,1.);
 if(!FParse::Param(FCommandLine::Get(),TEXT("NoSound"))&&J->Source==1&&J->Day==1&&J->Slot==1&&J->CaseTest<0&&FMath::IsNearlyEqual(SourceSeconds(),18.,1e-6))
 {J->CaptureOriginal.Begin(J->Id+TEXT("/original"),false,J->TraceTicks);if(LiveCapture)LiveCapture->Stop();}
 bCrossRunning=J->Source==0;CrossPoint=0;IntegrationPulse=0;if(bCrossRunning)PointCross();
 if(PlainAnomaly(*J)){DetectedJob=J->Id;Changed(TEXT("ANOMALY DETECTED. RECEIVER (centre PC): Recording. Let the take run to the end, then RETAIN TAKE on the RECORD tab."));return true;}
 Changed(TEXT("RECEIVER (centre PC): Recording started. Keep tuning, pointing and power valid until the take ends."));return true; // D45-20261110: the end of the take says what comes next
}
bool FS47FreeformSession::TakeAnomalous(const FJob& J,FString& Phen,bool& bOutThere)const
{
 const bool bFirstPulse=FirstPulseDue(J);
 // CHAIN-1-20261109: what is out on the land tonight unsettles the pulsar card (no instrument fault of the Director's needed): the
 // plain discrepancy, extra ticks outside the reference period, with no instrument cause to find.
 // LOOP-ANOMALY-ANY-CARD-20261113 (→ M7): from day AnyCardFrom on the third card too (OddCarries); the instrument faults stay the pulsar's.
 bOutThere=OddCarries(J)&&J.CaseTest<0&&!bAnomaly&&!bFirstPulse&&OutThereTonight();
 const bool bOdd=(J.Slot==1&&bAnomaly&&J.CaseTest<0)||bFirstPulse||bOutThere;
 Phen=bOdd&&!bOutThere&&!(bFirstPulse&&!bAnomaly)?PhenomenonId:FString();
 return bOdd;
}
int32 FS47FreeformSession::OddCarrier(int32 Seed,int32 Day)
{
 if(Day<AnyCardFrom)return 1;
 // explicit integer mixing (no hash implementation dependency): the same seed and day, the same card, in any Continue
 uint32 V=uint32(Seed)*0x9E3779B1u^(uint32(Day)+0x4f444431u)*0x85EBCA77u;V^=V>>15;V*=0x2C1B3C6Du;V^=V>>12;V*=0x297A2D39u;V^=V>>15;
 return (V&1u)?2:1;
}
bool FS47FreeformSession::OutTakeTonight(const FJob& Except)const
{
 const int32 Night=NightOf(WorldClock);
 return Work.ContainsByPredicate([&](const FJob& X)
 {
  if(&X==&Except||X.CaseTest>=0)return false;
  if(X.Stage==EJobStage::Pointed)return X.TakeDecided==1&&X.bTakeOut;
  return X.Stage>EJobStage::Pointed&&X.bAnomaly&&X.Phenomenon.IsEmpty()&&X.ReferenceClock>0&&NightOf(X.ReferenceClock)==Night;
 });
}
bool FS47FreeformSession::OddCarries(const FJob& J)const
{
 // Before AnyCardFrom, as ever: the pulsar's card (an incident card is a pulsar card too).
 if(J.Day<AnyCardFrom||J.CaseTest>=0)return J.Slot==1;
 if(J.Slot!=1&&J.Slot!=2)return false; // never the pointing cross
 if(OutTakeTonight(J))return false; // at most one a night
 const int32 Carrier=OddCarrier(JobSeed,J.Day);
 if(J.Slot==Carrier)return true;
 // the other card only once the day's carrier is past without it (worked plainly: its gates did not fit when it was tuned)
 return Work.ContainsByPredicate([&](const FJob& X){return &X!=&J&&X.bGenerated&&X.Day==J.Day&&X.Slot==Carrier&&X.CaseTest<0&&X.Stage>EJobStage::Pointed&&!X.bAnomaly;});
}
bool FS47FreeformSession::MarksTight(double In,double Out)
{
 return MarkCoversTicks(In,Out)&&Out-In<=(Ticks[UE_ARRAY_COUNT(Ticks)-1]-Ticks[0])+TightSlack+1e-6;
}
void FS47FreeformSession::DecideTake(FJob& J)
{
 FString Phen;bool bOutThere=false;
 J.TakeDecided=TakeAnomalous(J,Phen,bOutThere)?1:0;J.TakePhen=Phen;J.bTakeOut=bOutThere;
}
bool FS47FreeformSession::DecidedTake(const FJob& J,FString& Phen,bool& bOutThere)const
{
 if(J.TakeDecided<0||J.CaseTest>=0)return TakeAnomalous(J,Phen,bOutThere);
 Phen=J.TakePhen;bOutThere=J.bTakeOut;return J.TakeDecided==1;
}
bool FS47FreeformSession::AnomalyAhead()const
{
 const FJob* J=CurrentJob();if(!J||J->Stage!=EJobStage::Pointed||J->CaseTest>=0||Tutorial==1)return false;
 // Critic 12 Nov: the decided answer once there is one (asked every frame by the readouts); before that, what it would be.
 if(J->TakeDecided>=0)return J->TakeDecided==1&&!Specialized(J->TakePhen);
 FString Phen;bool bOutThere=false;
 return TakeAnomalous(*J,Phen,bOutThere)&&!Specialized(Phen);
}
// RX-ACQUIRE-SKY-20261113: the same answer for the sky chart, which is seen before the receiver is tuned (and after the take: what
// the take was). Nothing is decided here; a lock on the chart decides it (SkyHeld), as the tuned receiver otherwise would.
bool FS47FreeformSession::SkyOdd()const
{
 const FJob* J=CurrentJob();if(!J||J->CaseTest>=0||Tutorial==1)return false;
 if(J->Stage>EJobStage::Pointed)return PlainAnomaly(*J);
 if(J->TakeDecided>=0)return J->TakeDecided==1&&!Specialized(J->TakePhen);
 FString Phen;bool bOutThere=false;
 return TakeAnomalous(*J,Phen,bOutThere)&&!Specialized(Phen);
}
bool FS47FreeformSession::PlainAnomaly(const FJob& J){return J.bAnomaly&&J.CaseTest<0&&!Specialized(J.Phenomenon);}
bool FS47FreeformSession::TapeHeard(const FJob& J)
{
 const double Last=J.TraceTicks.IsEmpty()?Ticks[UE_ARRAY_COUNT(Ticks)-1]:J.TraceTicks.Last();
 return J.OriginalCovered>=FMath::Min(18.,Last+.3)-1e-6;
}
void FS47FreeformSession::RetainOriginal(FJob& J)
{
 // The page is the one the chart would have carried (PrintJob), kept in the notebook as the take ends; it goes into the packet.
 if(J.bPrinted||!M||!M->Story)return;
 const int32 Was=Selected;const int32 Index=Work.IndexOfByPredicate([&](const FJob& X){return &X==&J;});if(Index==INDEX_NONE)return;
 Selected=Index;J.bPrinted=true;J.ChartFeed=PrinterSeconds;PrintJob(false,false);Selected=Was;
}
bool FS47FreeformSession::RetainWaits(const FJob& J)
{
 // (a take already confirmed on tape by an older build keeps going to OPS, where RegisterComparison keeps its page)
 return PlainAnomaly(J)&&(J.Stage==EJobStage::Chart||J.Stage==EJobStage::Recorded)&&!J.bPrinted&&!J.bAudioReviewed;
}
bool FS47FreeformSession::RecordTabHoldsAnomaly()const
{
 const FJob* J=CurrentJob();if(!J)return false;
 if(J->Stage==EJobStage::Pointed)return ReceiverReady()&&AnomalyAhead();
 if(!PlainAnomaly(*J))return false;
 return J->Stage==EJobStage::Integrating||((J->Stage==EJobStage::Chart||J->Stage==EJobStage::Recorded)&&!J->bAudioReviewed);
}
bool FS47FreeformSession::RetainTake()
{
 // RX-ANOMALY-FLOW-20261113: the receiver's RECORD tab, RETAIN TAKE / RECORD REFERENCE 8 s. Every refusal says why.
 FJob* J=Job();
 if(!J||!PlainAnomaly(*J)){Feedback=TEXT("RECEIVER: Nothing unusual is waiting to be retained here.");return false;}
 if(J->Stage==EJobStage::Integrating){Feedback=TEXT("RECEIVER: The take is still recording. Let it run to the end, then RETAIN TAKE.");return false;}
 if(!RetainWaits(*J)){Feedback=J->bPrinted?TEXT("RECEIVER: This take is already retained. REEL DECK (wall table) next."):TEXT("RECEIVER: This take is past the receiver.");return false;}
 if(!RequirePower()){Feedback=TEXT("RECEIVER: No power. Restore the feeder or start the standby generator, then RETAIN TAKE.");return false;}
 if(J->Stage==EJobStage::Chart)J->Stage=EJobStage::Recorded;
 RetainOriginal(*J);
 if(!J->bPrinted){Feedback=TEXT("RECEIVER: The original page could not be written. Try RETAIN TAKE again.");return false;}
 // the reference: the station's own 8 seconds with the antennas out (the internal control), so the deck has the take and what the
 // station sounds like without the sky. One already recorded at the rack is kept.
 if(!J->bControl&&StartControl())Feedback=TEXT("ORIGINAL RETAINED. The 8-second station reference (antennas out) records by itself. NEXT: REEL DECK (wall table), play both, mark the extra ticks, press RETAIN.");
 else {Feedback=J->bControl?TEXT("ORIGINAL RETAINED. Its 8-second reference is on tape. NEXT: REEL DECK (wall table), play both, mark the extra ticks, press RETAIN."):TEXT("ORIGINAL RETAINED. NEXT: REEL DECK (wall table), play the take, mark the extra ticks, press RETAIN.");Changed(Feedback);}
 return true;
}
bool FS47FreeformSession::ConfirmOnTape(FJob& J)
{
 if(!PlainAnomaly(J)||J.Stage<EJobStage::Chart||J.Stage>=EJobStage::Registered)return false;
 if(RetainWaits(J)){Feedback=TEXT("REEL DECK: Retain the take first: RECEIVER (centre PC), RECORD tab, RETAIN TAKE.");return false;}
 if(!TapeHeard(J)){Feedback=FString::Printf(TEXT("REEL DECK: Not heard yet (%.1f of 18 s). PLAY the take from the start, past the extra ticks, then RETAIN."),FMath::Min(18.,J.OriginalCovered));return false;}
 // RX-ANOMALY-FLOW-20261113 (Tom: MARK IN and MARK OUT must work): marks the operator set are the operator's. Both set and the ticks
 // outside them: refused, with where they are; one set: the other is filled if that covers; none: filled from before the first tick.
 if(J.MarkStart>=0&&J.MarkEnd>=0&&!MarkCoversTicks(J.MarkStart,J.MarkEnd))
 {Feedback=FString::Printf(TEXT("REEL DECK: The marks (%.1f to %.1f s) miss the extra ticks. MARK IN before 9.3 s and MARK OUT after 12.6 s, then RETAIN."),J.MarkStart,J.MarkEnd);return false;}
 // LOOP-CAREFUL-PAYS-20261113: both marks the operator's own (none filled in below): what the packet's bearing can narrow by and name
 if(!J.bAudioReviewed)J.bOwnMarks=J.MarkStart>=0&&J.MarkEnd>=0;
 if(!MarkCoversTicks(J.MarkStart,J.MarkEnd))
 {
  const double In=J.MarkStart>=0?J.MarkStart:9.,Out=J.MarkEnd>=0?J.MarkEnd:12.9;
  if(!MarkCoversTicks(In,Out)){Feedback=FString::Printf(TEXT("REEL DECK: The %s at %.1f s misses the extra ticks. Set it again (MARK IN before 9.3 s, MARK OUT after 12.6 s), then RETAIN."),J.MarkStart>=0?TEXT("MARK IN"):TEXT("MARK OUT"),J.MarkStart>=0?J.MarkStart:J.MarkEnd);return false;}
  J.MarkStart=In;J.MarkEnd=Out;
 }
 const bool bFirstTime=!J.bAudioReviewed;J.bAudioReviewed=true;
 Feedback=TEXT("ANOMALY CONFIRMED ON TAPE. NEXT: OPS (corner PC) / DAILY WORK, Confirm anomaly: source unresolved.");
 if(!bFirstTime)return true;
 RecordEvidence(TEXT("pulse-review"),TEXT("Original heard at the reel deck. Extra ticks outside the reference period; source unresolved."));
 // Tom, 9 November: the check says the next step, nothing more. The case opens when the job is confirmed at OPS (FirstCaseHeld,
 // OpenFirstCase), and its SOURCE line keeps this original's own clock. The reel stands in for the old control here.
 if(Director){Director->Use(TEXT("ff:control"));Director->Use(TEXT("ff:compare"));}
 Changed(Feedback);return true;
}
void FS47FreeformSession::PrintJob(bool bPacket,bool bShow)
{
 FJob* J=Job();if(!J||!M||!M->Story)return;
 FS47Doc D;D.Id=J->Id+(bPacket?TEXT("-packet"):TEXT("-original"));D.Title=(bPacket?TEXT("Registered packet: "):TEXT("Original observation: "))+J->Id;D.Kind=TEXT("printout");D.Stamp=Reference(*J)+FString::Printf(TEXT(" / ORIGINAL COUNTER %.3f"),J->ReferenceClock);
 D.Page=Reference(*J)+TEXT("\n")+FS47Observing::Runs()[J->Source].Source+FString::Printf(TEXT("\nACTUAL ARRAY / QUALITY %.0f %%\nF %.3f MHz / AZ %.1f / EL %.1f\nINTEGRATION %.1f SECONDS\n"),J->Quality*100,EffectiveProfile(*J).F,TargetAz(),TargetEl(),J->Elapsed);
 if(J->Phenomenon==TEXT("time_disagreement"))D.Page+=FString::Printf(TEXT("ORIGINAL SIMULTANEOUS CLOCK READBACK\nRX COUNTER %.3f s / INDEPENDENT COUNTER %.3f s / DELTA %+.3f s\nRun the internal reference before interpreting this mismatch.\n"),J->ReceiverClock,J->ReferenceClock,J->ReceiverClock-J->ReferenceClock);
 else if(J->Phenomenon==TEXT("wrong_source"))D.Page+=FString::Printf(TEXT("CENTRE-BEAM RECEIVED LEVEL %.3f / NORMAL REFERENCE 1.000\nEncoder pointing matches command. An adjacent-beam control is still required; no off-source origin has been established.\n"),J->ReceivedLevel);
 else if(J->Phenomenon==TEXT("duplicate_print"))D.Page+=TEXT("PROVENANCE: ORIGINAL RX RECORD. A printer duplicate has been queued separately. Preserve this original before collecting or comparing the later copy.\n");
 else if(J->bAnomaly)D.Page+=TEXT("Extra ticks outside the reference period. Original retained.\n");
 else if(J->CaseTest>=0)
 {
  D.Page+=TEXT("CASE TEST / ")+CaseTestName(J->CaseLine,J->CaseTest).ToUpper()+TEXT("\nPREDICTION / ")+FS47FreeformCases::Prediction(J->CaseLine,J->CaseTest)+TEXT("\n");
  if(FS47FreeformCases::Def(J->CaseLine).Observable==FS47FreeformCases::EObservable::ClockDelta)D.Page+=FString::Printf(TEXT("SIMULTANEOUS CLOCK READBACK / RX COUNTER %.3f s / INDEPENDENT COUNTER %.3f s / DELTA %+.3f s\n"),J->ReceiverClock,J->ReferenceClock,J->ReceiverClock-J->ReferenceClock);
  if(!J->bCaseValid)D.Page+=TEXT("TEST INVALID / its condition was broken during the recording. It counts for nothing; registering it plans the test again.\n");
  else D.Page+=TEXT("RESULT / ")+FS47FreeformCases::ResultText(J->CaseLine,CaseObserved(*J))+TEXT("\n");
 }
 else D.Page+=TEXT("Result consistent with the current reference; no anomalous origin established.\n");
 D.Page+=FString::Printf(TEXT("CAPTURE CLOCK %.3f SECONDS / OBSERVED INTERVAL %.3f SECONDS\n"),J->ReferenceClock,J->Elapsed);
 if(!bPacket)
 {
  if(J->OriginalPage.IsEmpty()){J->OriginalPage=D.Page;J->OriginalStamp=D.Stamp;}
  else {D.Page=J->OriginalPage;D.Stamp=J->OriginalStamp;}
 }
 if(bPacket)
 {
  if(Specialized(J->Phenomenon))D.Page+=ControlSummary(*J)+TEXT("\nREGISTERED / PHYSICAL FILING PENDING.");
  else if(!J->bAnomaly)D.Page+=TEXT("FIRST SHIFT TRAINING COPY / ROUTINE RESULT REGISTERED AND PAID.\nTake this labelled packet to the records-room binder. Later routine work is registered digitally; discrepancies require retained originals.\nPHYSICAL FILING PENDING.");
  else D.Page+=(J->bControl?FString::Printf(TEXT("REGISTERED / INTERNAL CONTROL %.1f SECONDS\n"),J->ControlAt):FString(TEXT("REGISTERED / CHECKED ON TAPE AT THE REEL DECK\n")))
   +FString::Printf(TEXT("EXTRA TICKS %.1f TO %.1f SECONDS / CONCLUSION: SOURCE UNRESOLVED\nPHYSICAL FILING PENDING."),J->MarkStart,J->MarkEnd); // GAMEPLAY-LOOP-01
  // LOOP-SLIP-IN-PACKET-20261113 (→ M3): the bearing the registration gave, as the packet's last section (same feed, one collect)
  if(Places)if(const FString Section=Places->PacketSection(J->Id);!Section.IsEmpty())D.Page+=TEXT("\n\n")+Section;
 }
 D.Transcript=D.Page;M->Story->AddDoc(D);if(bShow&&M->UI)M->UI->Document(D);
}
bool FS47FreeformSession::CollectChart()
{
 const int32 Pending=PrinterIndex();
 // CASES-OPS-20261109: a printed result sheet comes off the printer on its own, or while a job's paper is still feeding.
 if((Pending==INDEX_NONE||PrintFeed(Work[Pending],PendingPrint(Work[Pending]))<PrinterSeconds)&&CollectResultSheet())return true;
 if(Pending==INDEX_NONE){if(!CaseSheets.IsEmpty()){Feedback=PrinterHint();Changed(Feedback);}return false;}
 const int32 Kind=PendingPrint(Work[Pending]);
 if(PrintFeed(Work[Pending],Kind)<PrinterSeconds){Feedback=PrinterHint();Changed(Feedback);return false;}
 if(Pending!=Selected&&!SelectJob(Pending)){Feedback=TEXT("Paper is waiting for ")+Work[Pending].Id+TEXT(". Finish the active recording or control, then collect it here.");return false;}
 FJob* J=Job();
 // The motor needs power; an already completed sheet does not.
 if(Kind==0)
 {
  J->Stage=EJobStage::Recorded;J->bPrinted=true;PrintJob(false);
  // (the anomaly thought now comes as ANOMALY DETECTED shows, GAMEPLAY-LOOP-01; the plain anomaly has no chart)
  // CHAIN-5B-20261109: under the chart, once a night, a line of the array's own log about what is out there tonight (WordOfTonight).
  if(Happenings){const FString Said=Happenings->WordOfTonight(TEXT("printer"));if(!Said.IsEmpty()&&M&&M->UI)M->UI->Toast(Said,8);}
 }
 else if(Kind==2){CollectDuplicate();if(!J->bDuplicateCollected)return false;}
 else
 {
  J->Stage=EJobStage::Packet;PrintJob(true);
  // GAMEPLAY-LOOP-01: an extra-tick take has no chart to collect; the array's line of tonight comes under its packet instead
  if(PlainAnomaly(*J)&&Happenings){const FString Said=Happenings->WordOfTonight(TEXT("printer"));if(!Said.IsEmpty()&&M&&M->UI)M->UI->Toast(Said,8);}
  // LOOP-SLIP-IN-PACKET-20261113 (→ M3): the packet's last page is the bearing: on the map, in the field record and the toast now, and
  // in the notebook under its own name as the slip was ("Bearing slip, D0N"), without a second paper to open.
  FString SlipPage;int32 SlipDay=0;bool bSlipExtra=false;
  if(Places&&Places->ReleasePacket(J->Id,&SlipPage,&SlipDay,&bSlipExtra)&&M&&M->Story)
  {
   FS47Doc Slip;Slip.Id=FString::Printf(TEXT("freeform-bearing-d%d%s"),FMath::Max(1,SlipDay),bSlipExtra?TEXT("-again"):TEXT(""));Slip.Title=FString::Printf(TEXT("Bearing slip, D%02d"),FMath::Max(1,SlipDay));
   Slip.Kind=TEXT("printout");Slip.Stamp=TEXT("CHECK BEFORE NOON");
   Slip.Page=FString::Printf(TEXT("SARO OPS V4.2  BEARING SLIP  %s\nLAST PAGE OF THE ANOMALY PACKET %s\n\n%s"),*TerminalDate(WorldClock),*J->Id,*SlipPage);Slip.Transcript=Slip.Page;
   M->Story->AddDoc(Slip);
  }
 }
 FieldFocus=-1;bGuideFocus=false;StopPrinterFeedback();StepPrinter(0);Changed();return true;
}

void FS47FreeformSession::CollectDuplicate()
{
 FJob* J=Job();if(!J||!J->bPrinted||J->bDuplicateCollected||J->bDuplicateRejected||!M||!M->Story)return;
 if(J->OriginalPage.IsEmpty())if(const FS47Doc* D=M->Story->Docs().FindByPredicate([&](const FS47Doc& X){return X.Id==J->Id+TEXT("-original");})){J->OriginalPage=D->Page;J->OriginalStamp=D->Stamp;}
 if(J->OriginalPage.IsEmpty())return;
 const FString OriginalLine=FString::Printf(TEXT("CAPTURE CLOCK %.3f SECONDS / OBSERVED INTERVAL %.3f SECONDS"),J->ReferenceClock,J->Elapsed);
 if(!J->OriginalPage.Contains(OriginalLine,ESearchCase::CaseSensitive))
 {
  // Reject only this unprovable copy, not the retained observation. Persist the
  // disposition so this ready legacy sheet cannot monopolise the printer again.
  J->bDuplicateRejected=true;
  Feedback=TEXT("Legacy duplicate rejected: this original has no independent capture-counter record. Original retained unchanged; no finding or payment. The printer is available for other observations. Select another job on OPS / DAILY WORK.");
  StopPrinterFeedback();StepPrinter(0);Changed(Feedback);return;
 }
 J->DuplicateClock=J->ReferenceClock+2.4;J->DuplicateInterval=FMath::Max(0.,J->Elapsed-1.);
 const FString DuplicateLine=FString::Printf(TEXT("CAPTURE CLOCK %.3f SECONDS / OBSERVED INTERVAL %.3f SECONDS"),J->DuplicateClock,J->DuplicateInterval);
 J->DuplicatePage=TEXT("SEPARATE PRINTER COPY / ORIGIN UNVERIFIED\n\n")+J->OriginalPage.Replace(*OriginalLine,*DuplicateLine,ESearchCase::CaseSensitive);
 J->DuplicateStamp=Reference(*J)+FString::Printf(TEXT(" / COPY COUNTER %.3f"),J->DuplicateClock);
 FS47Doc D;D.Id=J->Id+TEXT("-duplicate");D.Title=TEXT("Separate printer duplicate: ")+J->Id;D.Kind=TEXT("printout");D.Stamp=J->DuplicateStamp;D.Page=J->DuplicatePage;D.Transcript=D.Page;
 M->Story->AddDoc(D);J->bDuplicateCollected=true;if(M->UI)M->UI->Document(D);Changed();
}
FString FS47FreeformSession::ControlSummary(const FJob& J)const
{
 if(J.bDuplicateRejected)return TEXT("LEGACY DUPLICATE REJECTED / The retained original has no independent capture-counter record. No comparison or finding can be established. Original unchanged; select another observation on OPS / DAILY WORK.");
 if(J.Phenomenon==TEXT("wrong_source"))
 {
  FString Text=TEXT("ADJACENT-BEAM CONTROL / ACTUAL ENCODERS AND SAMPLED AMPLITUDE\n");
  for(int32 I=0;I<J.BeamAz.Num()&&I<J.BeamEl.Num()&&I<J.BeamAmplitude.Num();++I)Text+=FString::Printf(TEXT("%s: AZ %.3f EL %.3f / LEVEL %.3f\n"),I==0?TEXT("WEST"):I==1?TEXT("EAST"):TEXT("CENTRE"),J.BeamAz[I],J.BeamEl[I],J.BeamAmplitude[I]);
  return Text+(J.bBeamVerified?TEXT("All samples follow one second of actual encoder settling. Compare the off-centre maximum against the schedule."):TEXT("Control incomplete; no off-source conclusion yet."));
 }
 if(J.Phenomenon==TEXT("time_disagreement"))return FString::Printf(TEXT("SIMULTANEOUS DEVICE CLOCK READBACKS\nORIGINAL: RX %.3f / INDEPENDENT %.3f / DELTA %+.3f s\nINTERNAL CONTROL: RX %.3f / INDEPENDENT %.3f / DELTA %+.3f s\n%s"),J.ReceiverClock,J.ReferenceClock,J.ReceiverClock-J.ReferenceClock,J.ControlReceiverClock,J.ControlReferenceClock,J.ControlReceiverClock-J.ControlReferenceClock,J.bClockMeasured?TEXT("The clocks still disagree when checked against the station's own reference. That alone doesn't prove time went missing, or that anything in the sky caused it."):TEXT("Timed independent control still required.")); // D45-20261110: plain words, no "not evidence"
 if(J.Phenomenon==TEXT("duplicate_print"))return FString::Printf(TEXT("ORIGINAL: COUNTER %.3f / INTERVAL %.3f s\nSEPARATE COPY: COUNTER %.3f / INTERVAL %.3f s\n%s"),J.ReferenceClock,J.Elapsed,J.DuplicateClock,J.DuplicateInterval,J.bDuplicateCollected?TEXT("The physical copies are retained separately. Compare them with the independent master register; never amend the original."):TEXT("Collect the separate copy at the printer before comparing."));
 return TEXT("Original and internal-reference recordings retained.");
}
bool FS47FreeformSession::CompareInstrument()
{
 if(!RequirePower())return false;
 FJob* J=Job();if(!J||J->Stage!=EJobStage::Recorded||!J->bControl||J->ControlAt<8||!Specialized(J->Phenomenon)){Feedback=J?JobNextAction(*J):FString(TEXT("OPS (corner PC): Select the retained observation in DAILY WORK before comparing."));return false;}
 if(J->bDuplicateRejected){Feedback=ControlSummary(*J);return false;}
 bool Verified=false;
 if(J->Phenomenon==TEXT("wrong_source"))Verified=J->bBeamVerified&&J->BeamAz.Num()==3&&J->BeamEl.Num()==3&&J->BeamAmplitude.Num()==3&&J->BeamAmplitude[1]>J->BeamAmplitude[0]+.25&&J->BeamAmplitude[1]>J->BeamAmplitude[2]+.25;
 else if(J->Phenomenon==TEXT("time_disagreement"))Verified=J->bClockMeasured&&FMath::IsNearlyEqual(J->ReceiverClock-J->ReferenceClock,3.2,.001)&&FMath::IsNearlyEqual(J->ControlReceiverClock-J->ControlReferenceClock,3.2,.001);
 else if(J->Phenomenon==TEXT("duplicate_print")&&J->bDuplicateCollected&&M&&M->Story)
 {
  const FS47Doc* Original=M->Story->Docs().FindByPredicate([&](const FS47Doc& D){return D.Id==J->Id+TEXT("-original");});
  const FS47Doc* Copy=M->Story->Docs().FindByPredicate([&](const FS47Doc& D){return D.Id==J->Id+TEXT("-duplicate");});
  Verified=Original&&Copy&&Original->Page==J->OriginalPage&&Original->Stamp==J->OriginalStamp&&Copy->Page==J->DuplicatePage&&Copy->Stamp==J->DuplicateStamp&&J->DuplicatePage!=J->OriginalPage&&FMath::IsNearlyEqual(J->DuplicateClock-J->ReferenceClock,2.4,.001)&&FMath::IsNearlyEqual(J->Elapsed-J->DuplicateInterval,1.,.001);
 }
 if(!Verified)
 {
  Feedback=J->Phenomenon==TEXT("duplicate_print")?TEXT("PRINTER: Collect the separate duplicate and retain the original before comparing them at OPS (corner PC), DAILY WORK."):
   J->Phenomenon==TEXT("wrong_source")?TEXT("TRACKING (right-hand PC): Complete the west, east and centre measurements before comparing them at OPS (corner PC), DAILY WORK."):
   TEXT("RECEIVER RACK: Complete the independent clock control before comparing original and control readings at OPS (corner PC), DAILY WORK.");return false;
 }
 J->bAudioReviewed=true;RecordEvidence(J->Phenomenon,ControlSummary(*J));if(Director)Director->Use(TEXT("ff:compare"));Changed(TEXT("OPS (corner PC): Register the supported instrumental conclusion; the measured comparison is retained."));return true;
}
bool FS47FreeformSession::CanRecordControl()const
{
 // D45-20261110: the extra-tick take is on tape the moment it ends (Chart); its chart is only the paper copy, collected later.
 // The instrument discrepancies keep their own order (printer first; their controls are compared at OPS).
 // GAMEPLAY-LOOP-01: never for the extra-tick take; the receiver asks nothing of it after the take.
 const FJob* J=CurrentJob();
 return J&&J->bAnomaly&&!PlainAnomaly(*J)&&!J->bControl&&HasPower()&&J->Stage==EJobStage::Recorded&&J->Phenomenon!=TEXT("wrong_source");
}
bool FS47FreeformSession::StartControl()
{
 // GAMEPLAY-LOOP-01: an extra-tick take may still have one recorded at the physical rack (optional; Use("rack")), never asked for.
 FJob* J=Job();if(!J||!(J->Stage==EJobStage::Recorded||(J->Stage==EJobStage::Chart&&!Specialized(J->Phenomenon)))||!J->bAnomaly||J->bControl||!HasPower())return false;
 FieldFocus=-1;bGuideFocus=false;J->bControl=true;J->ControlAt=0;
 if(!FParse::Param(FCommandLine::Get(),TEXT("NoSound"))&&J->CaptureOriginal.Enabled()&&J->Source==1&&!Specialized(J->Phenomenon)&&J->CaseTest<0)
 {J->CaptureControl.Begin(J->Id+TEXT("/control"),true,{});if(LiveCapture)LiveCapture->Stop();}
 if(J->Phenomenon==TEXT("wrong_source")){J->BeamPoint=0;J->BeamAz.Reset();J->BeamEl.Reset();J->BeamAmplitude.Reset();J->bBeamVerified=false;PointBeamControl();Changed(TEXT("TRACKING (right-hand PC): West, east and centre are measured automatically. Wait for completion, then open DAILY WORK at OPS (corner PC), select this job and press Compare and register the retained instrument measurements."));}
 else Changed(J->Phenomenon==TEXT("duplicate_print")?TEXT("RECEIVER RACK: Checking the master register automatically. When complete, collect the duplicate at the PRINTER; the original is retained."):PlainAnomaly(*J)?(J->bPrinted?TEXT("ORIGINAL RETAINED. The 8-second station reference (antennas out) records by itself. NEXT: REEL DECK (wall table), play both, mark the extra ticks, press RETAIN."):TEXT("RECEIVER RACK: The 8-second station reference is running. Then retain the take: RECEIVER (centre PC), RECORD tab, RETAIN TAKE.")) // RX-ANOMALY-FLOW-20261113
  :TEXT("RECEIVER RACK: Eight-second internal reference running automatically; the original is retained. When complete, follow the job hint to OPS for clock measurements or the REEL DECK on the wall table for audio comparison."));
 return true;
}
void FS47FreeformSession::SeekAudio(double At){if(!FMath::IsFinite(At))return;CaptureStartStage=0;if(TapeCapture)TapeCapture->Stop();AudioCursor=FMath::Clamp(At,0.,AudioSourceIndex==0?18.:8.);bAudioPlaying=false;Changed();}
bool FS47FreeformSession::MarkAudioAt(double At)
{
 FJob* J=Job();if(!J||!J->bAnomaly||!(PlainAnomaly(*J)?J->Stage>=EJobStage::Chart:J->ControlAt>=8)||!FMath::IsFinite(At)){Feedback=TEXT("REEL DECK: Nothing unusual is on this tape to mark.");return false;}
 if(RetainWaits(*J)){Feedback=TEXT("REEL DECK: Retain the take first: RECEIVER (centre PC), RECORD tab, RETAIN TAKE.");return false;}
 if(J->Stage>=EJobStage::Registered){Feedback=TEXT("Registered comparison is sealed. Playback is available; its marks cannot be changed.");return false;}
 if(Specialized(J->Phenomenon)){Feedback=TEXT("This is an instrument comparison. Use its measured control at OPS; no extra-tick marks are required.");return false;}
 if(AudioSourceIndex!=0){Feedback=TEXT("REEL DECK: The reference is loaded. Press ORIGINAL, then mark the take.");return false;}
 if(J->bAudioReviewed&&PlainAnomaly(*J)){Feedback=TEXT("REEL DECK: This take is retained with its marks. NEXT: OPS (corner PC), DAILY WORK.");return false;}
 At=FMath::Clamp(At,0.,18.);
 // RX-ANOMALY-FLOW-20261113: a click on the trace sets the next mark there (MARK IN, then MARK OUT, then a new MARK IN), in order,
 // and says what it set; the tape stays where it is.
 if(J->MarkStart<0||J->MarkEnd>=0){J->MarkStart=At;J->MarkEnd=-1;Feedback=FString::Printf(TEXT("MARK IN %.1f s. Click again after the last extra tick for MARK OUT."),At);}
 else{J->MarkEnd=FMath::Max(J->MarkStart,At);J->MarkStart=FMath::Min(J->MarkStart,At);Feedback=MarkSaid(*J);}
 if(!PlainAnomaly(*J))J->bAudioReviewed=false; // GAMEPLAY-LOOP-01: the extra-tick take's marks are notes; its one listen stands
 Changed();return true;
}
FString FS47FreeformSession::MarkSaid(const FJob& J)
{
 if(J.MarkStart<0)return TEXT("MARK: none yet.");
 if(J.MarkEnd<0)return FString::Printf(TEXT("MARK IN %.1f s. MARK OUT still to set."),J.MarkStart);
 return FString::Printf(TEXT("MARK %.1f to %.1f s: %s"),J.MarkStart,J.MarkEnd,MarkCoversTicks(J.MarkStart,J.MarkEnd)?TEXT("covers the extra ticks. Press RETAIN when heard."):TEXT("misses extra ticks. MARK IN before 9.3 s, MARK OUT after 12.6 s."));
}
bool FS47FreeformSession::AudioAction(const FString& Action)
{
 if((Action==TEXT("play")||Action==TEXT("confirm"))&&!RequirePower())return false;
 // GAMEPLAY-LOOP-01: the extra-tick take is on the deck from the end of the take, with or without a control; the instrument
 // discrepancies still need theirs.
 FJob* J=Job();if(!J||!J->bAnomaly||!(PlainAnomaly(*J)?J->Stage>=EJobStage::Chart:J->ControlAt>=8)){Feedback=TEXT("Nothing unusual on tape yet. An unusual take is recorded and retained at the receiver (centre PC) first.");return false;}
 // RX-ANOMALY-FLOW-20261113 (Tom: from RECORD on the receiver to the reel deck): the take is on the deck once it is retained there.
 if(RetainWaits(*J)&&Action!=TEXT("pause")){Feedback=TEXT("REEL DECK: Retain the take first: RECEIVER (centre PC), RECORD tab, RETAIN TAKE. Then it plays here with its reference.");return false;}
 if(Action==TEXT("control")&&(!J->bControl||J->ControlAt<8)){Feedback=PlainAnomaly(*J)&&J->bControl?FString::Printf(TEXT("REEL DECK: The reference is still recording at the receiver (%.0f of 8 s)."),J->ControlAt):PlainAnomaly(*J)?FString(TEXT("REEL DECK: No reference on this take (retained before the reference was kept). Play the original.")):FString(TEXT("No control on this take. The job needs only the original: press PLAY."));return false;}
 if(Action==TEXT("original")||Action==TEXT("control")||Action==TEXT("pause"))CaptureStartStage=0;
 if(TapeCapture&&(Action==TEXT("original")||Action==TEXT("control")||Action==TEXT("pause")||(Action==TEXT("play")&&(!bAudioPlaying||AudioCursor>=(AudioSourceIndex==0?18.:8.)))))TapeCapture->Stop();
 if(Action==TEXT("original")||Action==TEXT("control")){AudioSourceIndex=Action==TEXT("original")?0:1;AudioCursor=0;bAudioPlaying=false;}
 else if(Action==TEXT("play"))
 {
  if(!bAudioOpen)return false;if(AudioCursor>=(AudioSourceIndex==0?18.:8.))AudioCursor=0;
  if(UsesCapturedAudio()){if(!bAudioPlaying&&!CaptureStartStage){CaptureStartStage=1;AudioStopWhy.Empty();}}
  else bAudioPlaying=true;
 }
 else if(Action==TEXT("pause"))bAudioPlaying=false;
 else if(Action==TEXT("mark_start")||Action==TEXT("mark_end"))
 {
  if(J->Stage>=EJobStage::Registered){Feedback=TEXT("Registered comparison is sealed. Playback is available; its marks cannot be changed.");return false;}
  if(Specialized(J->Phenomenon)){Feedback=TEXT("This is an instrument comparison. Use its measured control at OPS; no extra-tick marks are required.");return false;}
  if(AudioSourceIndex!=0){Feedback=PlainAnomaly(*J)?TEXT("REEL DECK: The reference is loaded. Press ORIGINAL, then mark the take."):TEXT("Mark the original recording, not the quiet internal control.");return false;}
  if(PlainAnomaly(*J)&&J->bAudioReviewed){Feedback=TEXT("REEL DECK: This take is retained with its marks. NEXT: OPS (corner PC), DAILY WORK.");return false;}
  // RX-ANOMALY-FLOW-20261113 (Tom: MARK IN and MARK OUT must work): each key sets its end at the tape head and says so; an end on
  // the wrong side of the other is refused with the reason (MARK IN after MARK OUT clears the old MARK OUT instead).
  if(Action==TEXT("mark_start"))
  {
   J->MarkStart=AudioCursor;if(J->MarkEnd>=0&&J->MarkEnd<J->MarkStart)J->MarkEnd=-1;
   Feedback=J->MarkEnd<0?FString::Printf(TEXT("MARK IN %.1f s. Play on past the last extra tick and press MARK OUT."),AudioCursor):MarkSaid(*J);
  }
  else
  {
   if(J->MarkStart>=0&&AudioCursor<J->MarkStart){Feedback=FString::Printf(TEXT("REEL DECK: MARK OUT (%.1f s) must come after MARK IN (%.1f s). Play on, or set MARK IN again."),AudioCursor,J->MarkStart);return false;}
   J->MarkEnd=AudioCursor;Feedback=J->MarkStart<0?FString::Printf(TEXT("MARK OUT %.1f s. MARK IN still to set (rewind and press it before the first extra tick)."),AudioCursor):MarkSaid(*J);
  }
  if(!PlainAnomaly(*J))J->bAudioReviewed=false;
 }
 else if(Action==TEXT("confirm"))
 {
  if(J->Stage>=EJobStage::Registered){Feedback=TEXT("This comparison is already registered and retained.");return false;}
  if(Specialized(J->Phenomenon))return CompareInstrument();
  // GAMEPLAY-LOOP-01: RETAIN is the same one listen the deck takes by itself (ConfirmOnTape); no second reel, no marks asked for.
  // Seeking does not play skipped audio.
  if(!ConfirmOnTape(*J))return false;
 }
 else return false;
 Changed();return true;
}
void FS47FreeformSession::NoteAudioStop(const TCHAR* Why,double Dt)
{
 AudioStopWhy=Why;UE_LOG(LogTemp,Display,TEXT("S47 capture playback stopped: %s cursor=%.3f source=%d dt=%.3f"),Why,AudioCursor,AudioSourceIndex,Dt);
}
// FIX-20261110 (Tom: more sound on the reel). The deck's transport is heard as a machine: the motor
// taking up and letting go, the rewind running back, a tape changed, a mark set. Read off the state
// each tick, so the wall deck's own keys and the panel's sound the same.
void FS47FreeformSession::TransportSound()
{
 if(!M||!M->Audio||!AudioDesk)return;
 const FJob* J=CurrentJob();const bool Run=bAudioPlaying||CaptureStartStage!=0;const double MarkSum=J?J->MarkStart+J->MarkEnd*3:0;
 const bool Hear=bAudioOpen&&AudioDesk->ListeningPosition()&&!Held();
 if(Hear&&bTransportPrimed)
 {
  US47Audio* A=M->Audio;
  if(Run&&!bTransportRun){A->Play(TEXT("rack_switch"),.5f,FVector::ZeroVector,.8f);A->Beep(96.f,.30f,.05f,.02f,TEXT("sine"));A->Beep(192.f,.18f,.02f,.05f,TEXT("sine"));}
  else if(!Run&&bTransportRun){A->Play(TEXT("click"),.5f,FVector::ZeroVector,.7f);A->Beep(72.f,.14f,.05f,.01f,TEXT("sine"));}
  if(AudioSourceIndex!=TransportSource){A->Play(TEXT("thud_soft"),.36f);A->Play(TEXT("click"),.4f,FVector::ZeroVector,.6f);}
  else if(!Run&&FMath::Abs(AudioCursor-TransportCursor)>.5)
  {
   // the tape runs back (or on) to the place: a whirr as long as the way, and the stop at the end
   const double Way=FMath::Abs(AudioCursor-TransportCursor);const int32 N=FMath::Clamp(int32(Way*1.6),3,22);const bool Back=AudioCursor<TransportCursor;
   A->Play(TEXT("small_switch"),.4f);
   for(int32 K=0;K<N;++K){const float U=float(K)/N;A->Beep((Back?620.f+520.f*U:1140.f-380.f*U),.034f,.022f*(1-.5f*U),.03f+.03f*K,TEXT("sawtooth"));}
  }
  if(J&&!FMath::IsNearlyEqual(MarkSum,TransportMark)){A->Beep(1568.f,.03f,.05f,0,TEXT("square"));A->Beep(2093.f,.05f,.04f,.04f,TEXT("square"));}
 }
 bTransportPrimed=true;bTransportRun=Run;TransportCursor=AudioCursor;TransportSource=AudioSourceIndex;TransportMark=MarkSum;
}
void FS47FreeformSession::StepAudio(double Dt)
{
 if(!HasPower()){if(bAudioPlaying)NoteAudioStop(TEXT("no power"),Dt);bAudioPlaying=false;return;}
 FJob* J=Job();if(!J||!bAudioOpen||!M||!AudioDesk)return;
 if(CaptureStartStage)
 {
  const auto& Capture=AudioSourceIndex==0?J->CaptureOriginal:J->CaptureControl;
  if(!AudioDesk->ListeningPosition()||!UsesCapturedAudio()||!M->Audio||!M->Audio->Ctx||!M->Audio->SfxBus)
  {NoteAudioStop(TEXT("start cancelled: away from the deck or no audio"),Dt);CaptureStartStage=0;if(TapeCapture)TapeCapture->Stop();bAudioPlaying=false;return;}
  if(CaptureStartStage==1)
  {
   if(!TapeCapture)TapeCapture=MakeShared<FS47CapturePlayer>(M->Audio->Ctx.ToSharedRef(),M->Audio->SfxBus);
   if(!TapeCapture->Prepare(Capture)){NoteAudioStop(TEXT("tape preparation failed"),Dt);CaptureStartStage=0;Changed(TEXT("Tape preparation failed. Press PLAY to try again."));return;}
   CaptureStartStage=2;return;
  }
  if(CaptureStartStage==2){CaptureStartStage=3;return;}
  if(Dt>.100001)return;
  CaptureStartStage=0;bAudioPlaying=TapeCapture&&TapeCapture->Ready(Capture)&&TapeCapture->Play(Capture,AudioCursor);
  if(bAudioPlaying){TapeCapture->Audible(true);M->Audio->ReceiverRecordingOff();}
  else{NoteAudioStop(TEXT("start failed: audio context not running or tape not ready"),Dt);Changed(TEXT("Playback could not start. Press PLAY to try again."));}
  return;
 }
 if(!bAudioPlaying)return;
 if(!AudioDesk->ListeningPosition()){NoteAudioStop(TEXT("left the deck"),Dt);bAudioPlaying=false;Changed(TEXT("Playback paused because you left the reel deck. Return and press PLAY to resume."));return;}
 if(UsesCapturedAudio()&&TapeCapture&&!TapeCapture->Aligned(FMath::Min(AudioSourceIndex==0?18.:8.,AudioCursor+Dt)))
 {NoteAudioStop(TEXT("timing: audio clock drifted from the cursor"),Dt);TapeCapture->Stop();bAudioPlaying=false;Changed(TEXT("Playback paused after a timing interruption. Press PLAY to resume at the retained position."));return;}
 const double Was=AudioCursor,Duration=AudioSourceIndex==0?18.:8.;AudioCursor=FMath::Min(Duration,Was+Dt);
 const bool Listening=AudioDesk->ListeningPosition();
 const bool bHeardBefore=TapeHeard(*J);
 double& Covered=AudioSourceIndex==0?J->OriginalCovered:J->ControlCovered;if(Listening&&Was<=Covered+1e-7)Covered=FMath::Max(Covered,AudioCursor);
 // RX-ANOMALY-FLOW-20261113 (Tom: RETAIN could not be pressed): hearing the take no longer confirms it by itself; the deck says the
 // listen is done and RETAIN is the operator's (ConfirmOnTape).
 if(AudioSourceIndex==0&&!bHeardBefore&&TapeHeard(*J)&&!J->bAudioReviewed&&PlainAnomaly(*J))
  Feedback=J->MarkStart>=0&&J->MarkEnd>=0?TEXT("TAKE HEARD PAST THE EXTRA TICKS. Press RETAIN."):TEXT("TAKE HEARD PAST THE EXTRA TICKS. MARK IN before them, MARK OUT after (keys or a click on the trace), then RETAIN.");
 if(M->Audio&&Listening)
 {
  const auto& Capture=AudioSourceIndex==0?J->CaptureOriginal:J->CaptureControl;
  bool Captured=false;
  if(UsesCapturedAudio()&&M->Audio->Ctx&&M->Audio->SfxBus)
  {
   if(!TapeCapture)TapeCapture=MakeShared<FS47CapturePlayer>(M->Audio->Ctx.ToSharedRef(),M->Audio->SfxBus);
   Captured=TapeCapture->Play(Capture,AudioCursor);if(Captured){TapeCapture->Audible(true);M->Audio->ReceiverRecordingOff();}
  }
  if(!Captured&&TapeCapture)TapeCapture->Stop();
  if(!Captured&&(J->RecordingSoundVersion==1||J->RecordingSoundVersion==2))M->Audio->MonitorReceiverRecording(FString::Printf(TEXT("freeform-source-%d"),J->Source),J->RecordingGain,J->RecordingBandwidth,J->RecordingQuality,AudioCursor,J->RecordingSoundVersion);
  else M->Audio->ReceiverRecordingOff(); // Legacy evidence keeps its original beep-only reconstruction.
  if(!Captured)for(double At=PulsarPeriod;At<=Duration;At+=PulsarPeriod)if(Was<At&&AudioCursor>=At)M->Audio->Beep(820,.012f,.16f,0,TEXT("square"));
  if(!Captured&&AudioSourceIndex==0)for(double At:J->TraceTicks)if(Was<At&&AudioCursor>=At)M->Audio->Beep(1240,.008f,.1f,0,TEXT("square"));
 }
 if(AudioCursor>=Duration){bAudioPlaying=false;Changed();}
}
double FS47FreeformSession::AudioLevel()const
{
 if(!bAudioPlaying)return .05;double L=.08;
 for(double At=PulsarPeriod;At<=18;At+=PulsarPeriod)L=FMath::Max(L,FMath::Max(0.,1-FMath::Abs(AudioCursor-At)/.06));
 if(AudioSourceIndex==0&&CurrentJob())for(double At:CurrentJob()->TraceTicks)L=FMath::Max(L,FMath::Max(0.,.8-FMath::Abs(AudioCursor-At)/.05));return L;
}
FString FS47FreeformSession::AudioDisplay()const{const FString Status=AudioCaptureStatus(),Note=Status.IsEmpty()?FString():TEXT(" / ")+Status;return FString::Printf(TEXT("%s %04.1f/%02d %s"),AudioSourceIndex==0?TEXT("ORG"):TEXT("REF"),AudioCursor,AudioSourceIndex==0?18:8,CaptureStartStage?TEXT("LOADING TAPE"):bAudioPlaying?TEXT("PLAY"):CurrentJob()&&CurrentJob()->bLegacyTrace?TEXT("PAUSE / LEGACY RECONSTRUCTION"):TEXT("PAUSE"))+Note;}
bool FS47FreeformSession::RegisterComparison(const FString& Conclusion)
{
 if(!RequirePower())return false;
 FJob* J=Job();
 // GAMEPLAY-LOOP-01: an extra-tick take left at the printer by an older build has no chart to fetch any more; its original is kept here.
 if(J&&J->Stage==EJobStage::Chart&&PlainAnomaly(*J))J->Stage=EJobStage::Recorded;
 // D45-20261110: an instrument discrepancy is registered with its paper: say where it is, never refuse silently.
 if(J&&J->Stage==EJobStage::Chart&&J->bAnomaly){Feedback=J->ChartFeed<PrinterSeconds?PrinterHint():FString::Printf(TEXT("PRINTER: Collect the original chart of %s first. The comparison is registered together with its paper."),*J->Id);Changed(Feedback);return false;}
 if(!J||J->Stage!=EJobStage::Recorded)return false;
 if(J->bDuplicateRejected){Feedback=ControlSummary(*J);return false;}
 if(J->bAnomaly&&Specialized(J->Phenomenon)&&!CompareInstrument())return false;
 if(J->bAnomaly&&(!J->bAudioReviewed||Conclusion!=(Specialized(J->Phenomenon)?InstrumentConclusion(J->Phenomenon):FString(TEXT("unresolved"))))){Feedback=Specialized(J->Phenomenon)?TEXT("Complete the measured comparison and register its instrumental finding. The controls do not prove an alien origin."):!J->bAudioReviewed?(RetainWaits(*J)?TEXT("RECEIVER (centre PC): RECORD tab, RETAIN TAKE first; then the reel deck."):TEXT("REEL DECK (wall table): Play the take, mark the extra ticks and press RETAIN first.")):TEXT("The tape shows extra ticks, not where they came from. Confirm it as source unresolved.");return false;}
 if(PlainAnomaly(*J)){RetainOriginal(*J);if(!MarkCoversTicks(J->MarkStart,J->MarkEnd)){J->MarkStart=9.;J->MarkEnd=12.9;}} // GAMEPLAY-LOOP-01: the packet carries both
 const bool AlreadyPaid=J->bPaid;
 J->Stage=EJobStage::Registered;
 // Tom, 9 November: the reviewed pulsar comparison, registered as unresolved, is what opens THE REPEATED INTERVAL.
 const bool bOpened=J->bAnomaly&&!Specialized(J->Phenomenon)&&J->CaseTest<0&&OpenFirstCase(*J);
 // The OPS computer has accepted an actual result; opening its screen alone is silent.
 // The whole modem recording: nothing in the panel transition cuts it short.
 if(M&&M->Audio)M->Audio->PcTransferAtOps(M->Web,27.);
 FString PayWhy;
 if(!J->bAnomaly){if(!J->bPaid){J->bPaid=true;LastPay=JobPay(*J);J->PaidAmount=LastPay;JobBonus(*J,&PayWhy);Earn(LastPay);DeskWork(*J);}J->Stage=EJobStage::Filed;}
 if(J->CaseTest>=0&&!J->bCaseRecorded){RecordCaseResult(*J);ClearCompletedSelection();return true;}
 // LOOP-1-20261109: a discrepancy points out even with no branch mended. LOOP-SLIP-IN-PACKET-20261113 (→ M3): a discrepancy's bearing is
 // the last page of its packet, not a slip of its own (FS47Places::FTake); LOOP-CAREFUL-PAYS-20261113: a hand fix and tight marks narrow it.
 bool bBearingInPacket=false;
 if(Places)
 {
  if(J->bAnomaly&&J->Stage==EJobStage::Registered)
  {
   FS47Places::FTake Take;Take.Job=J->Id;Take.bHandFix=J->SkyBy==1;Take.bTicks=PlainAnomaly(*J);Take.bTightMarks=Take.bTicks&&J->bOwnMarks&&MarksTight(J->MarkStart,J->MarkEnd);
   Take.bOut=J->bTakeOut;Take.bOwnMarks=J->bOwnMarks;Take.MarkIn=J->MarkStart;Take.MarkOut=J->MarkEnd;
   bBearingInPacket=Places->AfterRegistered(true,&Take)&&Places->HeldFor(J->Id);
  }
  else Places->AfterRegistered(J->bAnomaly);
 }
 const bool WasTutorial=InTutorial();StepTutorial();ClearCompletedSelection();
 if(WasTutorial&&!InTutorial())return true;
 const TCHAR* const Last=bBearingInPacket?TEXT(" Its last page is an off-beam bearing."):TEXT("");
 Feedback=bOpened?FString::Printf(TEXT("CASE OPENED: %s (OPS / CASE BOARD). Registered: the packet is printing%s Collect it and file its originals in the binder."),FS47FreeformCases::Def(0).Title,bBearingInPacket?TEXT("; its last page is an off-beam bearing."):TEXT("."))
  :J->Stage==EJobStage::Registered?FString::Printf(TEXT("Registered. Packet feed queued; wait for the printer to finish, collect the labelled packet, then file its originals in the binder.%s"),Last):AlreadyPaid?FString(TEXT("Reference check registered. Original retained; no additional payment is due.")):FString::Printf(TEXT("Routine result registered. $%d credited%s; the original remains in EARLIER WORK and the notebook.%s"),LastPay,*PayWhy,*DeskLine);Changed(Feedback);return true;
}
bool FS47FreeformSession::NearBinder() const
{
 if(!M||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->World||M->World->Area!=TEXT("saro")||M->Web->Area()!=TEXT("saro")||M->World->bDriving||M->World->bBusy)return false;
 FVector At;bool bFound=false;
 if(const auto* Item=M->Web->Item(TEXT("binder")))
 {
  // Do not turn a missing prop's fallback zero position into a valid filing site.
  if(Item->bBox||M->Web->ObjAt(Item->Node)){At=S47Three::ToThree(M->Web->PropLocation(TEXT("binder")));bFound=true;}
 }
 if(!bFound&&M->Inter)if(const auto* Item=M->Inter->Get(TEXT("binder"));Item&&Item->Object){At=Item->Object.WorldPosition();bFound=true;}
 if(!bFound||At.ContainsNaN())return false;
 const FVector P=M->Player->Pos;
 return M->Web->Rect(TEXT("annex.zone.records")).Contains(P)&&FVector2D(P.X-At.X,P.Z-At.Z).Size()<=2.4&&FMath::Abs(M->Player->FloorY-At.Y)<=2.;
}
bool FS47FreeformSession::OutThereTonight() const
{
 if(!M||!RulesLive()||InTutorial()||!Happenings||!Places)return false;
 const int32 Night=NightOf(WorldClock);
 // The same gates as the slip itself (FS47Places::AfterRegistered), so that the card never promises one that cannot print.
 if(Director&&!Director->MayPointOut(Night))return false;
 if(Happenings->BigOwns(Night))return false;
 FString Event,Title;double X=0,Z=0;bool bAhead=false;
 return Happenings->OutTonight(Night,Event,Title,X,Z,bAhead)&&(!Director||Director->MayPointAt(Event,WorldClock,Night));
}
bool FS47FreeformSession::FilePacket()
{
 // Another job may have been selected since the packet was collected; its paper still waits here.
 FJob* J=Work.FindByPredicate([](const FJob& X){return X.Stage==EJobStage::Packet;});if(Job()&&Job()->Stage==EJobStage::Packet)J=Job();if(!J)return false;
 if(!NearBinder()){Feedback=TEXT("Take the registered paper packet to the actual records-room binder. The remote archive is read-only.");Changed(Feedback);return false;}
 J->Stage=EJobStage::Filed;if(!J->bPaid){J->bPaid=true;LastPay=JobPay(*J);J->PaidAmount=LastPay;Earn(LastPay);DeskWork(*J);}
 if(J->bAnomaly)
 {
  RecordEvidence(TEXT("pulse-archive"),TEXT("Original, tape check and registered conclusion physically filed under one reference."));
  if(Director)Director->Use(TEXT("ff:archive"));
 }
 const bool WasTutorial=InTutorial();StepTutorial();ClearCompletedSelection();
 if(!(WasTutorial&&!InTutorial()))Changed(TEXT("OPS (corner PC): Select DAILY WORK or RECORDS for follow-up. Originals filed; work credited."));return true;
}

bool FS47FreeformSession::Spend(int32 Amount,bool bEmergency)
{
 if(Amount<0||Amount>10000)return false;
 if(Cash<Amount)
 {
  if(!bEmergency||Loan>0){Feedback=bEmergency?TEXT("A requisition balance is still standing. OPS: Registered daily work clears it first. An outage's feeder-repair fuse is the one exception."):TEXT("Insufficient budget. OPS: Register paid daily work, or request an essential requisition at the Caliche counter.");return false;}
  Loan=Amount-Cash;Cash=0;
 }
 else Cash-=Amount;
 Changed();return true;
}
bool FS47FreeformSession::CanRegisterPhotoEvidence() const
{
 if(!HasPower()||!M||!M->Player||!M->World||!M->Web||!M->Web->IsLoaded()||M->World->Area!=TEXT("saro")||M->Web->Area()!=TEXT("saro")||M->World->bDriving||M->World->bBusy)return false;
 const auto* Item=M->Web->Item(TEXT("opsTerminal"));if(!Item||(!Item->bBox&&!M->Web->ObjAt(Item->Node)))return false;
 const FVector At=S47Three::ToThree(M->Web->PropLocation(TEXT("opsTerminal"))),P=M->Player->Pos;
 return !At.ContainsNaN()&&FVector2D(P.X-At.X,P.Z-At.Z).Size()<=2.8&&FMath::Abs(M->Player->FloorY-At.Y)<=2.;
}
bool FS47FreeformSession::ChargeEmergencyFeederFuse()
{
 const int32 Missing=FMath::Max(0,5-Cash);if(Loan>1000000-Missing)return false;
 Cash=FMath::Max(0,Cash-5);Loan+=Missing;Changed();return true;
}
void FS47FreeformSession::Earn(int32 Amount)
{
 if(Amount<=0||Amount>10000)return;
 // M7: pay clears the requisition, then what is carried on the weekly sheet, and only then goes into the budget, so that work just
 // before the assessment still counts against the carried line.
 const int32 Paid=FMath::Min(Amount,Loan);Loan-=Paid;
 const int32 Sheet=FMath::Min(Amount-Paid,Carried);Carried-=Sheet;
 Cash=FMath::Min(1000080,Cash+Amount-Paid-Sheet);Changed();
}
void FS47FreeformSession::Refund(int32 Amount){if(Amount<=0||Amount>10000)return;Cash=FMath::Min(1000080,Cash+Amount);Changed();}
int32 FS47FreeformSession::InventoryCount(const FString& Id)const{return Field?Field->InventoryCount(Id):0;}
void FS47FreeformSession::AddItem(const FString& Id,int32 Count){if(Field)Field->AddItem(Id,Count);}
bool FS47FreeformSession::Consume(const FString& Id){return Field&&Field->Consume(Id);}
void FS47FreeformSession::SetPowerFault(bool bFault){if(bPowerFault==bFault)return;bPowerFault=bFault;SkyFaultUntil=0;if(!HasPower())StopPrinterFeedback();Changed(bFault?TEXT("Feeder voltage is unstable. Measure the circuit or transfer to the generator."):TEXT("Feeder supply restored and verified."));}
void FS47FreeformSession::SetAnomaly(bool bValue)
{
 if(bAnomaly==bValue)return;bAnomaly=bValue;UndecideTakes();
 if(bValue)
 {
  const bool Available=Work.ContainsByPredicate([this](const FJob& J){return J.Day==DayNumber&&J.Slot==1&&J.CaseTest<0&&J.Stage<=EJobStage::Pointed;});
  if(!Available)
  {
   FJob J;J.Id=FString::Printf(TEXT("FF-D%04d-INCIDENT-%04d"),DayNumber,Work.Num());J.Day=DayNumber;J.Slot=1;J.Source=1;J.bPaid=true;Work.Add(J);
   if(CurrentJob()&&CurrentJob()->Stage==EJobStage::Filed)Selected=Work.Num()-1;
  }
 }
 Changed(bValue?TEXT("Pulsar monitor discrepancy. Select the pulsar or incident job at OPS, then record it at the receiver."):FString());
}
void FS47FreeformSession::RecordEvidence(const FString& Id,const FString& Text)
{
 if(Id.IsEmpty())return;
 // (a magnetometer reading at a place already read is kept once, but it is read again: the land's control takes it, LandRecord)
 if(Findings.Contains(Id)){LandRecord(Id);return;}
 Findings.Add(Id,Text);FindingDays.Add(Id,DayNumber);TakenCount+=Id.StartsWith(TEXT("taken-"))?1:0;
 if(M)S47Thoughts::Later(S47Thoughts::FindKey(Id)); // UI-20261112 (Tom): a new find out on the land: the player's own thought (empty key: none)
 if(LessonRecordKey(Id)&&RulesLive())
 {
  const int32 Paid=EarnSide(FieldRecordPay,TEXT("field record, a lesson"));
  if(Paid>0&&M&&M->UI)M->UI->Toast(FString::Printf(TEXT("Field record kept. $%d from the field allowance."),Paid),3.5f);
 }
 NoteArrayRecord(Id,Text); // LOOP-3-20261109: the array's own log, and when a night record out there was kept
 // CASES-OPS-20261109: a case's own record (a test result, a conclusion, a reply) is no field record and no card on the board; its
 // telex is its one document and the night log has it under WORK (FS47NightLog, CaseRecordLine).
 const bool bCaseRecord=FS47FreeformCases::IsCaseRecord(Id);
 if(PinBoard&&!bCaseRecord)PinBoard->Kept(Id);
 if(M&&M->Story&&!bCaseRecord)
 {
  // Named by what it says, never by its key, and stamped with where the player is, not always the station.
  FString Named=Text;int32 Stop=INDEX_NONE;
  if(Named.FindChar(TEXT('\n'),Stop))Named=Named.Left(Stop);
  if(Named.FindChar(TEXT('.'),Stop)&&Stop>0)Named=Named.Left(Stop);
  Named.TrimStartAndEndInline();
  if(Named.Len()>60){const int32 Space=Named.Left(60).Find(TEXT(" "),ESearchCase::CaseSensitive,ESearchDir::FromEnd);Named=Named.Left(Space>20?Space:60)+TEXT("...");}
  const bool bTrace=FS47FreeformCases::LineForFinding(Id)==FS47FreeformCases::MysteryLine;
  // The station's own ground, not the farm that shares its area.
  const bool bStation=M->Web&&M->Web->Area()==TEXT("saro")&&M->Player&&FVector2D(M->Player->Pos.X,M->Player->Pos.Z).Size()<220;
  const bool bField=Id!=TEXT("carrier_saro")&&(bTrace||Id.StartsWith(TEXT("taken-"))||Id.StartsWith(TEXT("flood-room-"))||Id.Contains(TEXT("_visit"))||Id.Contains(TEXT("_trace"))||!bStation);
  FS47Doc D;D.Id=TEXT("freeform-")+Id;D.Title=Named.IsEmpty()?FString(TEXT("Field record")):TEXT("Field record: ")+Named;D.Kind=TEXT("fieldnote");D.Stamp=FString::Printf(TEXT("%s / DAY %d"),bField?TEXT("IN THE FIELD"):TEXT("SARO"),DayNumber);
  D.Page=D.Stamp+TEXT("\n\nOBSERVATION\n")+Text+TEXT("\n\nInterpretation remains separate from the retained observation. Compare independent sources before assigning an origin.");D.Transcript=D.Page;
  M->Story->AddDoc(D);
 }
 OpenCaseFor(Id);
 UpdateMysteries();Changed();
 LandRecord(Id);
}
void FS47FreeformSession::UpdateMysteries()
{
 static const TCHAR* Families[3][3]={{TEXT("extra_pulse"),TEXT("feeder_fault"),TEXT("wrong_source")},{TEXT("roof_steps"),TEXT("observer"),TEXT("landing_trace")},{TEXT("time_disagreement"),TEXT("duplicate_print"),TEXT("unsigned_telex")}};
 static const TCHAR* Names[]={TEXT("The repeated interval"),TEXT("Visitors around the base"),TEXT("Disagreeing records")};
 for(int32 Line=0;Line<3;++Line)
 {
  int32 Count=0;int32 First=MAX_int32,Last=0;TArray<FString> Sources;
  for(int32 I=0;I<3;++I)
  {
   const FString Base=Families[Line][I],Prefix=Base+TEXT(".day");bool bFamilySeen=false;
   for(const auto& Record:Findings)
   {
    const bool bOriginal=Record.Key==Base;
    const FString Suffix=Record.Key.StartsWith(Prefix)?Record.Key.Mid(Prefix.Len()):FString();
    bool bDigits=!Suffix.IsEmpty();for(TCHAR C:Suffix)bDigits&=C>=TEXT('0')&&C<=TEXT('9');
    const bool bFollowup=bDigits&&FCString::Atoi(*Suffix)>0;
    // Something at the window, a light come back to the room: these are the same family as a figure seen at the base.
    const bool bKin=Base==TEXT("observer")&&(Record.Key.StartsWith(TEXT("peek-"))||Record.Key.StartsWith(TEXT("flood-room-"))||Record.Key.StartsWith(TEXT("door-visit-"))||Record.Key.StartsWith(TEXT("door-marks-")));
    if(!bOriginal&&!bFollowup&&!bKin)continue;
    const int32 D=FindingDays.FindRef(Record.Key);if(D<1)continue;
    bFamilySeen=true;Sources.Add(Record.Key);First=FMath::Min(First,D);Last=FMath::Max(Last,D);
   }
   Count+=bFamilySeen?1:0;
  }
  Sources.Sort([this](const FString& A,const FString& B){const int32 DA=FindingDays.FindRef(A),DB=FindingDays.FindRef(B);return DA==DB?A<B:DA<DB;});
  // A same-day series remains an initial finding. A later retained follow-up
  // establishes repetition without changing the first day's immutable source.
  const int32 New=Count>=3&&Last>First?3:Count>=2&&Last>First?2:Count>0?1:0;
  const int32 Before=MysterySteps.IsValidIndex(Line)?MysterySteps[Line]:0;
  if(MysterySteps.IsValidIndex(Line))MysterySteps[Line]=FMath::Max(Before,New);
  if(New==3&&Before<3&&M&&M->Story)
  {
   FS47Doc D;D.Id=FString::Printf(TEXT("freeform-mystery-%d"),Line);D.Title=Names[Line];D.Kind=TEXT("comparison");D.Stamp=TEXT("CROSS-DAY COMPARISON / RETAINED SOURCES");
   D.Page=FString(Names[Line])+TEXT("\n\nThe three observation families now have retained records spanning more than one day.\n");
   for(const FString& Key:Sources)D.Page+=FString::Printf(TEXT("DAY %d / %s\n%s\n\n"),FindingDays.FindRef(Key),*Key,*Findings.FindRef(Key));
   D.Page+=TEXT("The pattern is repeatable across days; its origin remains unresolved. Follow-up may continue without ending the station's work.");D.Transcript=D.Page;M->Story->AddDoc(D);
   if(M->UI)M->UI->Toast(TEXT("Cross-day comparison available in the archive."),5);
  }
 }
}
bool FS47FreeformSession::Upgrade(int32 Index)
{
 if(Index==1){Feedback=TEXT("Manual reference comparison is available without an upgrade.");return false;}
 static const int32 Prices[]={60,80,50};if(Index<0||Index>=3||Upgrades[Index]||!Spend(Prices[Index]))return false;
 Upgrades[Index]=true;
 if(Index==0)AddItem(TEXT("relay"),1);
 if(Index==2){AddItem(TEXT("fuse"),2);AddItem(TEXT("relay"),1);}
 Changed(TEXT("Upgrade installed. Its function is listed on the operations work order."));return true;
}
bool FS47FreeformSession::Rest()
{
 return WaitUntil((FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay)+1)*SecondsPerDay+8*3600);
}
void FS47FreeformSession::TakenAway(double WorldSeconds)
{
 if(!bActive||bDead||!FMath::IsFinite(WorldSeconds)||WorldSeconds<=0)return;
 bAudioPlaying=false;
 FJob* J=Job();if(J&&J->Stage==EJobStage::Integrating){UnfitCaseRelay(*J);J->Stage=EJobStage::Pointed;J->Elapsed=0;Settle=0;bCrossRunning=false;} // CASETEST-20261109: a fitted swap relay back
 TestAdvance(WorldSeconds);
 if(Fatigue)Fatigue->Lost(WorldSeconds); // FATIGUE-20261108: no sleep in it
}
void FS47FreeformSession::Recover(const FString& Why)
{
 if(!bActive||bDead)return;
 bAudioPlaying=false;
 if(bDeadly){bDead=true;Changed(TEXT("Contact. Load the last saved case to continue."));return;}
 FJob* J=Job();if(J&&J->Stage==EJobStage::Integrating){UnfitCaseRelay(*J);J->Stage=EJobStage::Pointed;J->Elapsed=0;Settle=0;bCrossRunning=false;} // CASETEST-20261109: a fitted swap relay back, once, as TakenAway
 WorldClock+=15*60;DayNumber=FMath::Max(1,int32(FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay))+1);AddDay(DayNumber);LapseUntakenCards();
 if(M&&M->UI)M->UI->Fade(true,TEXT("CONTACT LOST"));
 bRecovering=M&&M->Story;const int32 Epoch=++RecoveryEpoch;
 if(M&&M->Player)M->Player->Place(0,-1.2,0);
 if(M&&M->Web){M->Web->SetDoorById(TEXT("doorSouth"),false,true);M->Web->SetDoorById(TEXT("doorEast"),false,true);}
 RecordEvidence(FString::Printf(TEXT("recovery-%d-%d"),DayNumber,Findings.Num()),TEXT("Returned to shelter after contact. Current uncompleted recording interrupted; secured records and repairs retained. ")+Why);
 if(M&&M->Story)
 {
  TWeakObjectPtr<US47Main> Weak=M;
  M->Story->After(.45,[this,Weak,Epoch]
  {
   if(!Weak.IsValid()||!Weak->IsFreeform()||Weak->Freeform.Get()!=this||RecoveryEpoch!=Epoch)return;
   bRecovering=false;if(Weak->UI)Weak->UI->Fade(false);Changed(TEXT("You regain your bearings in the control room. Fifteen minutes have passed; secured work is retained."));
  },TEXT("freeform-recovery"));
 }
 Changed();
}
bool FS47FreeformSession::TestAdvance(double WorldSeconds)
{
 if(!bActive||!FMath::IsFinite(WorldSeconds)||WorldSeconds<0)return false;
 WorldClock+=WorldSeconds;DayNumber=FMath::Max(1,int32(FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay))+1);AddDay(DayNumber);if(M&&M->Story)M->Story->SetClock(FMath::Fmod(WorldClock,86400.));return true;
}
void FS47FreeformSession::Changed(const FString& Text)
{
 if(M){M->RequestAutosave();if(!Text.IsEmpty()&&M->UI)M->UI->Toast(Text,5);}
}
void FS47FreeformSession::SetPanel(TSharedRef<SWidget> Widget,bool bLive,bool bAudio)
{
 if(!M||!M->UI)return;
 const TWeakObjectPtr<US47Main> Owner=M;
 M->UI->Open(Widget,[this,Owner]
 {
  if(IsEngineExitRequested()||!Owner.IsValid())return;
  US47Main* Main=Owner.Get();
  if(Main->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)||Main->Freeform.Get()!=this)return;
  if(Main->Audio){Main->Audio->ReceiverMonitorOff();if(RxSearch)RxSearch->Off(Main->Audio);}
  bTelemetry=false;bTapePanel=false;TerminalView.Reset();ActiveTerminal=-1;Changed();
 });bTelemetry=bLive;bAudioOpen=bAudio;
}
namespace
{
 namespace T=S47FreeformTerminal;
 // D45-20261110: a stock item in the station's own words, for the array PC's orders and the OPS SUPPLIES page alike
 FString SupplyPartName(const FString& Part)
 {
  return Part==TEXT("fuse")?FString(TEXT("CONTROL FUSE")):Part==TEXT("relay")?FString(TEXT("DRIVE RELAY")):Part==TEXT("splice_kit")?FString(TEXT("SPLICE KIT")):Part==TEXT("drive_motor")?FString(TEXT("DRIVE MOTOR"))
   :Part==TEXT("cell")?FString(TEXT("DRY CELLS")):Part==TEXT("donor")?FString(TEXT("DONOR CONTROLLERS")):Part==TEXT("fuel")?FString(TEXT("FUEL CANS")):Part==TEXT("film")?FString(TEXT("FILM ROLLS")):Part==TEXT("tubes")?FString(TEXT("TUBE PAIRS")):Part.ToUpper();
 }
 FString StatusWord(const FS47FreeformSession::FJob& J,bool bSelected)
 {
  using E=FS47FreeformSession::EJobStage;
  if(J.CaseTest>=0&&!J.bCaseValid&&J.Stage<E::Filed)return TEXT("TEST SPOILED"); // stays on the row, not only in a toast
  if(J.Stage==E::New)return bSelected?TEXT("READY"):J.Slot==2?TEXT("OPTIONAL"):TEXT("QUEUED");
  if(J.Stage==E::Pointed)return TEXT("POINTED");if(J.Stage==E::Integrating)return TEXT("RECORDING");if(J.Stage==E::Chart)return J.bAnomaly&&!Specialized(J.Phenomenon)?TEXT("ANOMALY"):TEXT("AT PRINTER"); // GAMEPLAY-LOOP-01
  if(J.Stage==E::Recorded)return J.bAnomaly&&!Specialized(J.Phenomenon)?(J.bAudioReviewed?TEXT("CONFIRM"):TEXT("ANOMALY")):J.bAnomaly?TEXT("INVESTIGATE"):TEXT("REGISTER");if(J.Stage==E::Registered)return TEXT("PACKET");if(J.Stage==E::Packet)return TEXT("FILE");return TEXT("FILED");
 }
}
// A station file as pages: whole paragraphs where they fit, a long one broken between two sentences.
static TArray<FString> S47StationPages(const FString& Text,int32 Wide,int32 Budget)
{
 TArray<FString> Pages,Paras;Text.ParseIntoArray(Paras,TEXT("\n\n"),true);FString Page;int32 Used=0;
 const auto Lines=[Wide](const FString& P){TArray<FString> Ls;P.ParseIntoArray(Ls,TEXT("\n"),false);int32 N=0;for(const FString& L:Ls)N+=FMath::Max(1,FMath::CeilToInt32(L.Len()/float(Wide)));return N;};
 const auto Flush=[&]{if(!Page.IsEmpty())Pages.Add(Page);Page.Empty();Used=0;};
 for(FString Para:Paras)while(!Para.IsEmpty())
 {
  const int32 Gap=Page.IsEmpty()?0:1,Room=Budget-Used-Gap;
  if(Lines(Para)<=Room){if(Gap)Page+=TEXT("\n\n");Page+=Para;Used+=Gap+Lines(Para);break;}
  int32 Cut=INDEX_NONE;
  if(Room>=4)for(int32 I=Para.Len()-2;I>1;--I) // a list breaks between its lines, running text between two sentences
   if((Para[I]==TEXT('\n')||(!Para.Contains(TEXT("\n"))&&Para[I]==TEXT(' ')&&(Para[I-1]==TEXT('.')||Para[I-1]==TEXT('?')||Para[I-1]==TEXT('"'))))&&Lines(Para.Left(I))<=Room){Cut=I;break;}
  if(Cut!=INDEX_NONE&&Lines(Para.Left(Cut))>=2){if(Gap)Page+=TEXT("\n\n");Page+=Para.Left(Cut);Para=Para.Mid(Cut+1);Flush();}
  else if(Page.IsEmpty()){Page=Para;Para.Empty();Flush();} // longer than a page and nowhere to break: let it run
  else Flush();
 }
 Flush();if(Pages.IsEmpty())Pages.Add(Text);return Pages;
}
bool FS47FreeformSession::StationFileDue()const
{
 // A file released (this morning or before) and never opened marks RECORDS. FIX10-10: the opened set is saved.
 for(int32 I=0;I<S47StationFiles::Count()&&I<32;++I)if(S47StationFiles::Get(I).Day<=Day()&&!(StationFilesSeen&(1u<<I)))return true;
 return false;
}
TSharedRef<SWidget> FS47FreeformSession::OpsShell(int32 Section,TSharedRef<SWidget> Content)
{
 OpsSection=Section;
 const bool CaseWaiting=CaseAwaitingChoice()>=0;
 TArray<T::FNav> Items;
 Items.Add({TEXT("OVERVIEW"),Section==0,[this]{Feedback.Empty();OpsSection=0;OpenOperations();}});
 Items.Add({TEXT("DAILY WORK"),Section==1,[this]{Feedback.Empty();OpsSection=1;OpenOperations();},Section!=1&&OpsWorkDue()}); // RX-TABS-DUE-20261113: confirm, register, compare
 Items.Add({TEXT("CASE BOARD"),Section==2,[this]{Feedback.Empty();OpenCaseBoard(-1);},CaseWaiting});
 Items.Add({TEXT("RECORDS"),Section==3,[this]{Feedback.Empty();if(StationFileDue())ArchiveTab=4;OpenArchive();},StationFileDue()}); // CASES-OPS-20261109: was CASE ARCHIVE
 // D48-20261110 (Tom): no SERVICE here; the array PC's MAINTENANCE tab owns service. OVERVIEW keeps the summary that points there (OpenServiceGuide).
 Items.Add({TEXT("SUPPLIES"),Section==5,[this]{Feedback.Empty();OpsSection=5;OpenOperations();}});
 Items.Add({TEXT("PHOTOGRAPHS"),Section==6,[this]{Feedback.Empty();OpenPhotos();},PhotoDeskDue()}); // FIX-20261110: the film record and its allowance desk
 return T::Columns({{.21f,S47Kit::Col({T::Nav(Items),T::Small(TEXT("ESC / EXIT  SOP / CARD AT DESK"))})},{.79f,Content}},16);
}
void FS47FreeformSession::OpenOperations()
{
 if(!RequirePower()||!M||!M->UI)return;
 if(OpsSection==6){OpenPhotos();return;} // FIX-20261110: a refresh of the film record stays on it
 const int32 Section=OpsSection==1||OpsSection==5?OpsSection:0;
 TArray<S47Kit::FW> Rows;
 auto Rows4=[this](bool bAll)
 {
  TArray<T::FRow> Out;
  for(int32 Group=0;Group<3;++Group)
  {
   if(Group==2&&!(bAll&&bShowEarlierJobs))continue;
   for(int32 I=0;I<Work.Num();++I)
   {
    const auto& J=Work[I];const bool Active=J.Stage>EJobStage::New&&J.Stage<EJobStage::Filed;
    const int32 InGroup=J.Stage==EJobStage::Filed?2:Active||I==Selected?0:J.Day==DayNumber||(J.CaseTest>=0&&!J.bCaseRecorded)?1:2;if(InGroup!=Group)continue; // a planned case test stays listed on later days
    const int32 Index=I;T::FRow R;R.Cells={J.Id,FS47Observing::Runs()[J.Source].Source,StatusWord(J,I==Selected)};R.bSelected=I==Selected&&FieldFocus<0;
    R.Badge=J.CaseTest>=0?TEXT("CASE"):J.bAnomaly?TEXT("ANOM"):FString::Printf(TEXT("P%d"),J.Slot+1);
    R.Action=[this,Index]{SelectJob(Index);OpenOperations();};Out.Add(R);
   }
  }
  return Out;
 };
 auto Actions=[this]()->TSharedRef<SWidget>
 {
  TArray<S47Kit::FW> A;const FJob* J=CurrentJob();if(!J)return S47Kit::Col(A);
  if(Tutorial==1)A.Add(T::Button(TEXT("Accept this job and start the shift"),[this]{AcceptTutorialJob();OpenOperations();},true));
  if(J->Slot==2&&J->Stage==EJobStage::New)A.Add(S47Kit::Row({T::Button(TEXT("Third job: hydrogen drift"),[this]{ChooseThird(false);OpenOperations();}),T::Button(TEXT("Third job: Virgo A"),[this]{ChooseThird(true);OpenOperations();})}));
  if(J->Stage==EJobStage::Recorded)
  {
   if(J->bAnomaly)
   {
    if(Specialized(J->Phenomenon))
    {
     A.Add(T::Small(ControlSummary(*J)));
     if(!J->bDuplicateRejected)A.Add(T::Small(TEXT("COMPARE HERE / CONTROL FIRST / MEASUREMENTS KEPT UNCHANGED")));
     const FString Conclusion=InstrumentConclusion(J->Phenomenon);
     const FString Name=J->Phenomenon==TEXT("wrong_source")?TEXT("Compare and register an off-centre maximum; origin unresolved"):J->Phenomenon==TEXT("time_disagreement")?TEXT("Compare and register disagreement in the receiver timing path"):TEXT("Compare and register mismatched duplicate; preserve original");
     if(!J->bDuplicateRejected)A.Add(T::Button(Name,[this,Conclusion]{RegisterComparison(Conclusion);OpenOperations();},true));
    }
    else
    {
     // GAMEPLAY-LOOP-01: one button, once the reel deck has heard it
     A.Add(T::Small(J->bAudioReviewed?TEXT("ANOMALY CHECKED ON TAPE / CONFIRM IT HERE"):TEXT("ANOMALY DETECTED / PLAY THE TAKE AT THE REEL DECK (WALL TABLE) FIRST")));
     A.Add(T::Button(TEXT("Confirm anomaly: source unresolved"),[this]{RegisterComparison(TEXT("unresolved"));OpenOperations();},true));
    }
   }
   else A.Add(T::Button(J->CaseTest>=0&&!J->bCaseRecorded?FString::Printf(TEXT("Register observation and case test result / $%d"),JobPay(*J)):J->bPaid?FString(TEXT("Register retained reference / no additional payment")):FString::Printf(TEXT("Register routine result / $%d"),JobPay(*J)),[this]{RegisterComparison(TEXT("normal"));OpenOperations();},true));
  }
  // GAMEPLAY-LOOP-01: an extra-tick take from an older build, still at Chart: the same one button (RegisterComparison keeps its original)
  else if(J->Stage==EJobStage::Chart&&PlainAnomaly(*J)&&J->bAudioReviewed)
  {
   A.Add(T::Small(TEXT("ANOMALY CHECKED ON TAPE / CONFIRM IT HERE")));
   A.Add(T::Button(TEXT("Confirm anomaly: source unresolved"),[this]{RegisterComparison(TEXT("unresolved"));OpenOperations();},true));
  }
  return S47Kit::Col(A);
 };
 int32 Remaining=0,TodayRemaining=0,EarlierRemaining=0,Rejected=0;
 for(const auto& X:Work)if(X.Stage!=EJobStage::Filed&&X.bDuplicateRejected)++Rejected;
 for(const auto& X:Work)if(X.Stage!=EJobStage::Filed&&!X.bDuplicateRejected){++Remaining;if(X.Day==DayNumber)++TodayRemaining;else if(X.Day<DayNumber)++EarlierRemaining;}
 const FString QueueExplanation=QueueLine(Remaining,TodayRemaining,EarlierRemaining,Rejected); // CASES-OPS-20261109: a system line
 const FJob* J=CurrentJob();
 auto Account=T::Panel(TEXT("STATION ACCOUNT"),S47Kit::Col({
  T::Value(TEXT("OPERATING BUDGET"),FString::Printf(TEXT("$%d"),Cash)),T::Value(TEXT("REQUISITION"),FString::Printf(TEXT("$%d"),Loan)),
  T::Value(TEXT("SARO"),Predecessor?Predecessor->AccountLine():FString(TEXT("IN SERVICE"))),
  T::Value(TEXT("NEXT REVIEW"),NextReviewText()),
  T::Value(TEXT("SELECTED JOB"),FieldFocus>=0?FString(TEXT("FIELD CASE: "))+FS47FreeformCases::Def(FieldFocus).Title:J?FString(FS47Observing::Runs()[J->Source].Source):FString(TEXT("NONE")),true),
  T::Small(TEXT("NEXT STEP")),T::Banner(TAttribute<FString>::CreateLambda([this]{return Objective().ToUpper();}),true)}));
 if(Section==0)
 {
  {
   // CASES-OPS-20261109: the morning's case replies wait in the tray that day, with Ward's and Reyes's papers. TELEX-20261109: what
   // was torn off the telex lies there, newest first, the day it was torn off and the next; what is still on the machine is not here.
   TArray<S47Kit::FW> Replies;
   if(!TelexSheets.IsEmpty())Replies.Add(T::Small(TelexSheets.Num()==1?FString(TEXT("TELEX / ONE PAPER ON THE MACHINE / TEAR IT OFF THERE")):FString::Printf(TEXT("TELEX / %d PAPERS ON THE MACHINE / TEAR THEM OFF THERE"),TelexSheets.Num())));
   for(int32 I=TelexTorn.Num()-1;I>=0;--I)
   {
    const FTelexSheet& Paper=TelexTorn[I];if(DayNumber-Paper.Day>1)continue;
    Replies.Add(T::Button(TEXT("READ / ")+Paper.Title,[this,Paper]
    {
     if(!M||!M->UI)return;
     const FS47Doc* Doc=M->Story?M->Story->Docs().FindByPredicate([&Paper](const FS47Doc& D){return D.Id==Paper.DocId;}):nullptr;
     if(Doc){M->UI->Document(*Doc);return;}
     FS47Doc Copy;Copy.Id=Paper.DocId;Copy.Title=Paper.Title;Copy.Kind=TEXT("telex");Copy.Stamp=Paper.Stamp;Copy.Page=Paper.Page;Copy.Transcript=Paper.Page;M->UI->Document(Copy);
    },true));
   }
   for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)
   {
    const FString Key=FString::Printf(TEXT("case%d-reply"),Line+1);const auto* Y=FS47FreeformCases::Def(Line).Mystery;
    // (an older save's reply, or one the full machine filed at once: there is no telex paper for it)
    if(!Y||!HasFinding(Key)||FindingDays.FindRef(Key)!=DayNumber||OnTelex(TEXT("freeform-")+Key)||InTelexTray(TEXT("freeform-")+Key))continue;
    Replies.Add(T::Button(TEXT("READ / ")+FString(Y->ReplyTitle),[this,DocId=TEXT("freeform-")+Key]
    {
     if(!M||!M->Story||!M->UI)return;
     if(const FS47Doc* Doc=M->Story->Docs().FindByPredicate([&DocId](const FS47Doc& D){return D.Id==DocId;}))M->UI->Document(*Doc);
    },true));
   }
   if(Predecessor)Predecessor->OpsRows(Rows,Replies);else if(!Replies.IsEmpty())Rows.Add(T::Panel(TEXT("IN THE TRAY"),S47Kit::Col(Replies)));
  }
  if(NightLog)NightLog->OpsRows(Rows);
  if(WardReport)WardReport->OpsRows(Rows);
  if(Tutorial==0)
  {
   TArray<S47Kit::FW> Continue;
   const int32 Next=ContinueJob(); // CASETEST-20261109: a case test that can be recorded now first
   if(Work.IsValidIndex(Next))
   {
    const FJob& NextJob=Work[Next];const bool bCaseNext=NextJob.CaseTest>=0&&!NextJob.bCaseRecorded;
    Continue.Add(T::Small(bCaseNext?TEXT("CASE TEST READY NOW / ITS WINDOW IS OPEN AND ITS CONDITION MET"):TEXT("JOB AVAILABLE NOW / UNFINISHED WORK KEEPS")));
    Continue.Add(T::Button(TEXT("Continue observation / ")+(bCaseNext?TEXT("CASE TEST: ")+CaseTestName(NextJob.CaseLine,NextJob.CaseTest).ToUpper()+(NextJob.bCaseValid?FString():FString(TEXT(", TAKE IT AGAIN")))+TEXT(" / "):FString())+NextJob.Id+TEXT(" / ")+FS47Observing::Runs()[NextJob.Source].Source,[this,Next]
    {
     if(SelectJob(Next)){OpsSection=1;OpenOperations();}
    },true));
   }
   else if(double LaterUp=0;const FJob* Later=NextEveningCard(LaterUp))Continue.Add(T::Small(EveningLine(*Later))); // LOOP-NIGHT-WATCH-20261113: the soonest
   else Continue.Add(T::Small(Rejected>0?TEXT("QUEUE / LEGACY ORIGINALS ONLY / REFERENCE CHECK BELOW, NO PAYMENT / NEW JOBS 08:00"):TEXT("QUEUE / DAILY OBSERVATIONS COMPLETE / REFERENCE CHECK BELOW, NO PAYMENT")));
   FString Photo;
   const FS47Album* Album=M&&M->Night?M->Night->Album.Get():nullptr;
   if(Album&&Album->PendingLabPhoto())Photo=TEXT("PHOTO LAB / WET BENCH: close the door and switch on the safelight, then develop, fix and collect your exposed photograph. Keep its original before submitting it at OPS."); // D45-20261110
   else if(CameraLeftOut())Photo=TEXT("YOUR CAMERA is still out where you stood before the gap. Your film and prints are safe here; go and get it.");
   else if(M&&M->FieldCamera&&!M->FieldCamera->bHave)Photo=TEXT("PHOTO LAB / ARCHIVE BENCH: collect the camera before going out. The reserve-film carton is beside it; prepare for an observation whenever you choose.");
   else if(Album&&Album->Left()<=0)Photo=InventoryCount(TEXT("film"))>0?TEXT("PHOTO LAB / RESERVE-FILM CARTON: load one stock roll into your camera; retained photographs remain unchanged."):TEXT("CALICHE STORE: buy a film roll and deliver it to GARAGE RECEIVING; then load it at PHOTO LAB. Your existing photographs remain retained.");
   else if(Album)Photo=FString::Printf(TEXT("CAMERA READY / %d exposures remain. You can take a field photograph now; return to PHOTO LAB to develop it before OPS registration. A photograph does not guarantee a paid finding."),Album->Left());
   if(!Photo.IsEmpty())Continue.Add(T::Small(Photo));
   if(const FString Goal=StationGoalLine();!Goal.IsEmpty())Continue.Add(T::Small(Goal));
   Rows.Add(T::Panel(TEXT("CONTINUE TODAY"),S47Kit::Col(Continue)));
  }
  Rows.Add(T::Small(QueueExplanation));
  if(!Work.ContainsByPredicate([this](const FJob& X){return X.Stage!=EJobStage::Filed&&!X.bDuplicateRejected&&!EveningGated(X);}))Rows.Add(T::Button(TEXT("Start an optional reference check / no payment"),[this]{StartReferenceCheck();OpsSection=1;OpenOperations();},true));
  if(!Work.ContainsByPredicate([](const FJob& X){return X.Source==0&&X.Stage==EJobStage::Filed;}))
   Rows.Add(T::Small(TEXT("OPTIONAL / 3C 286 CALIBRATION CROSS / A NORMAL REFERENCE FOR LATER COMPARISONS")));
  auto Today=T::Panel(TEXT("TODAY'S WORK"),S47Kit::Col({T::Table({TEXT("JOB"),TEXT("SOURCE"),TEXT("STATUS")},{1.25f,1.35f,1.f},Rows4(false)),
   T::Columns({{.6f,T::Big(FString(TEXT("OPEN SELECTED JOB")),[this]{OpsSection=1;OpenOperations();},1)},{.4f,T::Big(FString(TEXT("EARLIER WORK")),[this]{bShowEarlierJobs=true;OpsSection=1;OpenOperations();})}},10)}));
  Rows.Add(T::Columns({{.64f,Today},{.36f,Account}},12));
  Rows.Add(Actions());
  if(const FString Plan=TonightPlan();!Plan.IsEmpty())Rows.Add(T::Panel(FString(),T::Small(Plan)));
  if(!Feedback.IsEmpty())Rows.Add(T::Small(Feedback));
  const double Dusk=DuskWaitAt(WorldClock,DayNumber); // D45-20261110: 18:20 on the first day
  if(Dusk>WorldClock+60)Rows.Add(T::Button(TEXT("Wait until dusk / review time and equipment first"),[this,Dusk]{OpenWaitConfirmation(Dusk);}));
  double Up=0;
  if(const FJob* Later=NextEveningCard(Up))
  {
   Rows.Add(T::Button(FString::Printf(TEXT("Wait until %s is up at %s / review time and equipment first"),FS47Observing::Runs()[Later->Source].Source,*FS47FreeformCases::Clock(Up)),[this,At=Up]{OpenWaitConfirmation(At);},true));
  }
  const int32 PlanLine=FieldFocus>=0?FieldFocus:J?J->CaseLine:-1;double Target=0;
  if(CaseWaitTarget(PlanLine,Target))Rows.Add(T::Button(TEXT("Wait until five minutes before this test / review consequences"),[this,Target]{OpenWaitConfirmation(Target);},true));
  Rows.Add(T::Columns({{.5f,T::Big(FString(TEXT("REST UNTIL MORNING")),[this]{OpenWaitConfirmation((FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay)+1)*SecondsPerDay+8*3600);})},{.5f,T::Big(FString(TEXT("ARRAY PC / MAINTENANCE")),[this]{OpenServiceGuide();})}},10));
  if(Predecessor)Predecessor->OpsKeys(Rows);
  OpenTerminal(3,TEXT("OPS / MAIN MENU"),OpsShell(0,S47Kit::Col(Rows)));
  return;
 }
 if(Section==1)
 {
  Rows.Add(T::Small(QueueExplanation));
  if(!Work.ContainsByPredicate([this](const FJob& X){return X.Stage!=EJobStage::Filed&&!X.bDuplicateRejected&&!EveningGated(X);}))Rows.Add(T::Button(TEXT("Start an optional reference check / no payment"),[this]{StartReferenceCheck();OpenOperations();},true));
  if(FieldFocus>=0)Rows.Add(T::Panel(TEXT("ACTIVE FIELD TEST"),T::Banner(TAttribute<FString>::CreateLambda([this]{return FieldNextAction(FieldFocus).ToUpper();}),true)));
  auto Queue=T::Panel(FString::Printf(TEXT("WORK QUEUE (DAY %02d)"),DayNumber),S47Kit::Col({T::Table({TEXT("ID"),TEXT("SOURCE"),TEXT("STATE")},{1.4f,1.3f,1.f},Rows4(true)),
   T::Big(FString(bShowEarlierJobs?TEXT("HIDE EARLIER WORK"):TEXT("SHOW EARLIER WORK")),[this]{bShowEarlierJobs=!bShowEarlierJobs;OpenOperations();})}));
  TArray<S47Kit::FW> D;
  if(J)
  {
   const auto P=EffectiveProfile(*J);
   D.Add(T::Heading(FS47Observing::Runs()[J->Source].Source,44));
   D.Add(T::Value(TEXT("AZ"),FString::Printf(TEXT("%.1f°"),TargetAz())));D.Add(T::Value(TEXT("EL"),FString::Printf(TEXT("%.1f°"),TargetEl())));
   D.Add(T::Value(TEXT("FREQUENCY"),FString::Printf(TEXT("%.3f MHz"),P.F)));D.Add(T::Value(TEXT("INTEGRATION"),FString::Printf(TEXT("%.0f s"),SourceSeconds())));
   if(!J->bPaid)D.Add(T::Value(TEXT("PAYS NOW"),FString::Printf(TEXT("$%d%s%s"),JobPay(*J),J->bDocked?TEXT(" / HALF RATE"):DayNumber-J->Day>1?TEXT(" / LATE"):TEXT(""),
    RulesLive()&&!TrainingCard(*J)&&!CleanBonusFor(*J)?TEXT(" / FIX BY ASSIST / NO CLEAN BONUS"):RulesLive()&&!TrainingCard(*J)&&J->Stage<EJobStage::Recorded?*FString::Printf(TEXT(" / +$%d IF THE RECORDING READS"),CleanBonus):TEXT("")))); // LOOP-CAREFUL-PAYS-20261113
   // M7: what the evening's cards have taken (time is the second currency)
   if(RulesLive())D.Add(T::Value(TEXT("TONIGHT"),FString::Printf(TEXT("%d CARDS FILED / FIELD ALLOWANCE LEFT THIS WEEK $%d OF $%d (PHOTOS, RECORDS AND TIPS PAY FROM IT)"),CardsTonight(),SideLeftThisWeek(),int32(SideCap))));
   D.Add(T::Small(TEXT("NEXT STEP")));D.Add(T::Banner(TAttribute<FString>::CreateLambda([this]{return Objective().ToUpper();}),true));
  }
  else D.Add(T::Small(TEXT("NO JOB SELECTED")));
  Rows.Add(T::Columns({{.55f,Queue},{.45f,T::Panel(TEXT("SELECTED JOB"),S47Kit::Col(D))}}));
  Rows.Add(Actions());
  if(!Feedback.IsEmpty())Rows.Add(T::Small(Feedback));
  OpenTerminal(3,TEXT("OPS / DAILY WORK"),OpsShell(1,S47Kit::Col(Rows)));
  return;
 }
 // SUPPLIES: stock, purchases and documentation credits.
 // D45-20261110 (Tom: the stores list must be sorted better): what the work in hand needs on top (the next branch's parts, an open relay
 // order, the lamp's dry cells), the depot's stock beside it; then the stores list in bands: what can be bought now (what the budget
 // covers first, then by price), what is coming (by its first day, then price) and what is in hand. Each with TRUCK, FIELD or STATION.
 // The work block stands outside the list's box, so it is always in sight; the box takes what the 1200 x 900 tube has left.
 {
  TArray<S47Kit::FW> NowRows;
  if(Field&&Field->ArrayWear().bActive)if(const auto* GoalBranch=Field->ArrayWear().NextBranch())
  {
   NowRows.Add(T::Small(FString::Printf(TEXT("STATION GOAL / %s AT %s (+%d ANTENNAS)"),GoalBranch->Title,GoalBranch->Junction,GoalBranch->Dishes.Num())));
   for(const auto& Need:GoalBranch->Needs){const int32 Have=InventoryCount(Need.Item);NowRows.Add(T::Value(SupplyPartName(Need.Item),FString::Printf(TEXT("NEED %d / HAVE %d"),Need.Count,Have),Have<Need.Count));}
   NowRows.Add(T::Small(FString(GoalBranch->Id)==TEXT("centre")?TEXT("The splice is made up at CALIBRATION / REPAIR from two good antenna leads and foil."):TEXT("The CALICHE STORE sells the parts. Unpack the parcel at GARAGE RECEIVING.")));
  }
  if(Field&&(Field->HasArrayFault()||ServiceGuide==1))
  {
   const int32 Have=InventoryCount(TEXT("relay"));const bool bFitted=Field->ServiceStage()>=5;
   NowRows.Add(T::Value(Field->ServiceDishId()+TEXT(" RELAY ORDER"),bFitted?FString(TEXT("FITTED")):FString::Printf(TEXT("NEED 1 / HAVE %d"),Have),!bFitted&&Have<1));
  }
  if(M&&M->Night&&M->Night->Flashlight&&M->Night->Flashlight->bHave)
   NowRows.Add(T::Value(TEXT("FLASHLIGHT"),FString::Printf(TEXT("%.0f%% / DRY CELLS %d"),100*M->Night->Flashlight->BatteryLevel(),InventoryCount(TEXT("cell"))),M->Night->Flashlight->Low()&&InventoryCount(TEXT("cell"))<1));
  if(NowRows.IsEmpty())NowRows.Add(T::Small(TEXT("NOTHING ON ORDER FOR THE WORK IN HAND.")));
  NowRows.Add(T::Value(TEXT("OPERATING BUDGET"),Loan>0?FString::Printf(TEXT("$%d / REQUISITION $%d"),Cash,Loan):FString::Printf(TEXT("$%d"),Cash)));
  TArray<S47Kit::FW> Stock;
  for(const TCHAR* Item:{TEXT("fuse"),TEXT("relay"),TEXT("drive_motor"),TEXT("splice_kit"),TEXT("donor"),TEXT("cell"),TEXT("fuel"),TEXT("film"),TEXT("tubes")})Stock.Add(T::Value(SupplyPartName(Item),FString::Printf(TEXT("%d"),InventoryCount(Item))));
  if(Field)Stock.Add(T::Value(TEXT("GENERATOR TANK"),FString::Printf(TEXT("%.1f h"),Field->GeneratorHours())));
  Rows.Add(T::Columns({{.52f,T::Panel(TEXT("FOR THE WORK NOW"),S47Kit::Col(NowRows))},{.48f,T::Panel(TEXT("DEPOT STOCK"),S47Kit::Col(Stock))}},12));
 }
 static const TCHAR* Names[]={TEXT("Relay reserve / $60"),TEXT("Reference comparison / $80"),TEXT("Field service kit / $50")};
 if(Upgrades[0]&&Upgrades[2])Rows.Add(T::Small(TEXT("UPGRADES / RELAY RESERVE AND FIELD SERVICE KIT INSTALLED.")));
 else
 {
  TArray<S47Kit::FW> U;for(int32 I=0;I<3;++I){if(I==1)continue;const int32 Index=I;U.Add(T::Button(Upgrades[I]?FString(Names[I])+TEXT(" / INSTALLED"):FString(Names[I]),[this,Index]{Upgrade(Index);OpenOperations();},false,!Upgrades[I]));}
  Rows.Add(T::Panel(TEXT("UPGRADES"),S47Kit::Col({T::Small(TEXT("A relay reserve means fewer repeat failures. The field kit has two fuses and a relay, and you can compare readings by hand.")),S47Kit::Row(U)})));
 }
 {
  // What is on offer, what it costs and what it waits for. Everything the station can get hold of stands here, in its band.
  struct FOffer{const FS47KitOffer* Def;int32 Price;FString Waits;};
  TArray<FOffer> CanBuy,Coming,InHand;
  for(const auto& Offer:FS47Openings::Defs())
  {
   const FOffer Entry{&Offer,KitPrice(Offer.Id),KitWaits(Offer.Id)};
   (HasKit(Offer.Id)?InHand:Entry.Waits.IsEmpty()?CanBuy:Coming).Add(Entry);
  }
  CanBuy.StableSort([this](const FOffer& A,const FOffer& B){const bool CA=Cash>=A.Price,CB=Cash>=B.Price;return CA!=CB?CA:A.Price<B.Price;});
  Coming.StableSort([](const FOffer& A,const FOffer& B){const int32 DA=FS47Openings::FirstDay(A.Def->Id),DB=FS47Openings::FirstDay(B.Def->Id);return DA!=DB?DA<DB:A.Price<B.Price;});
  const auto Tagged=[](const FOffer& O){return FString::Printf(TEXT("%s / %s"),FS47Openings::Kind(O.Def->Id),O.Def->Title);};
  const auto Under=[](const FString& Words){return SNew(SBox).Padding(FMargin(22,0,0,6))[T::Small(Words)];};
  TSharedRef<SScrollBox> Offers=SNew(SScrollBox);
  Offers->AddSlot()[T::Text(CanBuy.IsEmpty()?FString(TEXT("CAN BUY NOW / NOTHING YET")):FString::Printf(TEXT("CAN BUY NOW / BUDGET $%d"),Cash),1)];
  for(const FOffer& O:CanBuy)
  {
   const FString Id=O.Def->Id;
   Offers->AddSlot()[T::Button(FString::Printf(TEXT("%s / $%d"),*Tagged(O),O.Price),[this,Id]{BuyKit(Id);OpenOperations();},false,Cash>=O.Price)];
   Offers->AddSlot()[Under(O.Def->What)];
  }
  if(!Coming.IsEmpty())Offers->AddSlot()[T::Text(TEXT("COMING"),1)];
  for(int32 I=0;I<Coming.Num();++I)
  {
   const FOffer& O=Coming[I];const int32 From=FS47Openings::FirstDay(O.Def->Id);
   if(DayNumber<From)
   {
    Offers->AddSlot()[T::Small(FString::Printf(TEXT("%s / $%d / FROM DAY %d"),*Tagged(O),O.Price,From))];
    if(I<2)Offers->AddSlot()[Under(TEXT("Then it waits for ")+O.Waits+TEXT("."))];
   }
   else Offers->AddSlot()[T::Small(FString::Printf(TEXT("%s / $%d / WAITS FOR %s"),*Tagged(O),O.Price,*O.Waits.ToUpper()))];
  }
  if(!InHand.IsEmpty())
  {
   TArray<FString> HeldNames;for(const FOffer& O:InHand)HeldNames.Add(Tagged(O));
   Offers->AddSlot()[T::Text(TEXT("IN HAND"),1)];
   Offers->AddSlot()[T::Small(FString::Join(HeldNames,TEXT("; ")))];
  }
  Rows.Add(T::Panel(TEXT("STORES LIST"),SNew(SBox).HeightOverride(Upgrades[0]&&Upgrades[2]?330:270)[Offers]));
 }
 if(Director&&PhotoDeskDue())Rows.Add(T::Small(TEXT("FIELD PHOTOGRAPHS / An allowance is waiting under PHOTOGRAPHS.")));
 if(!Feedback.IsEmpty())Rows.Add(T::Small(Feedback));
 OpenTerminal(3,TEXT("OPS / SUPPLIES"),OpsShell(5,S47Kit::Col(Rows)));
}

// FIX-20261110 (Tom: OPS gets its own tab for the photographs the operator takes). The film record: every frame exposed, read off the
// station's video digitizer in the tube's own phosphor, with what the lab and the allowance desk have done with it. The claims and the
// field records OPS pays for stand here now (they stood under SUPPLIES); a frame that is not developed has nothing to read yet.
static constexpr int32 PhotoStrip=6; // D48-20261110: frames on one contact strip (was a sheet of nine)
bool FS47FreeformSession::FieldRecordDue(const FS47AlbumShot& P) const
{
 if(P.bUnavailable||P.Url.IsEmpty()||!P.Enlarged.IsEmpty()||NightPhotoRegistered(P.Id))return false;
 if(!P.Text.StartsWith(TEXT("Trip camera"))&&NightPhotoFrom(P.Clock).IsEmpty())return false;
 return !Director||!Director->PhotoClaims().ContainsByPredicate([&P](const auto& C){return C.PhotoId==P.Id;});
}
bool FS47FreeformSession::PhotoDeskDue() const
{
 if(Director)for(const auto& C:Director->PhotoClaims())if(!C.bPaid)return true;
 if(M&&M->Night&&M->Night->Album){const auto& Shots=M->Night->Album->Shots();for(int32 I=Shots.Num()-1;I>=0&&I>=Shots.Num()-12;--I)if(Shots[I].IsDeveloped()&&FieldRecordDue(Shots[I]))return true;}
 return false;
}
void FS47FreeformSession::PhotoDeskRows(TArray<TSharedRef<SWidget>>& Rows)
{
 if(Director)
 {
  TArray<S47Kit::FW> Claims;
  for(const auto& Claim:Director->PhotoClaims())
  {
   const FString Id=Claim.Id;
   const bool Orb=Id.StartsWith(TEXT("dusk_orb."));
   if(Claim.bPaid&&!Orb)continue;
   // D48-20261110: the frame read on the screen has its register key beside the picture (OpenPhotos); the desk keeps the rest
   const bool bBeside=Claim.PhotoId==PhotoPick;
   if(bBeside&&!Orb)continue;
   Claims.Add(T::Small(Claim.Title+TEXT(" / ORIGINAL ")+Claim.PhotoId));
   const auto* Shot=M->Night&&M->Night->Album?M->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& P){return P.Id==Claim.PhotoId;}):nullptr;
   if(!Shot||Shot->bUnavailable||Shot->Url.IsEmpty())Claims.Add(T::Small(TEXT("ORIGINAL UNAVAILABLE / This photograph cannot be submitted. Existing incident records are retained.")));
   else if(!Shot->IsDeveloped())Claims.Add(T::Small(TEXT("UNDEVELOPED ORIGINAL / PHOTO LAB: Develop, fix and collect this photograph at the wet bench before submitting it.")));
   else
   {
    if(!Claim.bPaid){if(!bBeside)Claims.Add(T::Button(FString::Printf(TEXT("Register photograph / $%d field allowance"),Claim.Reward),[this,Id]{Director->RegisterPhotoClaim(Id);OpenPhotos();},true,CanRegisterPhotoEvidence()));}
    else Claims.Add(T::Small(TEXT("ORIGINAL REGISTERED / Field allowance already received.")));
   }
   // Payment follows the current best incident image; an existing control plan
   // retains its independently frozen original even if that newer image is absent
   // or still undeveloped. New plans still require an available developed image.
   if(Orb&&(Claim.ControlPlannedAt>=0||(Shot&&!Shot->bUnavailable&&!Shot->Url.IsEmpty()&&Shot->IsDeveloped())))
   {
     const bool Planned=Claim.ControlPlannedAt>=0;
     const auto& AlbumShots=M->Night->Album->Shots();
     const auto* Baseline=Planned?AlbumShots.FindByPredicate([&](const FS47AlbumShot& S){return S.Id==Claim.ControlOriginalId;}):Shot;
     if(Planned&&Claim.ControlPhotoId.IsEmpty())
     {
      const bool Available=Baseline&&!Baseline->bUnavailable&&!Baseline->Url.IsEmpty()&&Baseline->IsDeveloped()&&Baseline->Pose.IsSet()&&Baseline->Pose->Valid();
      if(Director->PhotoControlFocusId()==Id)Claims.Add(T::Small(TEXT("FOLLOWING THIS RETURN PLAN / Camera and NOW guidance")));
      else Claims.Add(T::Button(TEXT("Follow this return plan"),[this,Id]{if(Director)Director->FollowPhotoControl(Id);OpenPhotos();},false,Available&&CanRegisterPhotoEvidence()));
     }
     const auto* Control=Planned?AlbumShots.FindByPredicate([&](const FS47AlbumShot& S){return !S.bUnavailable&&!S.Url.IsEmpty()&&S.IsDeveloped()&&(Claim.ControlPhotoId.IsEmpty()?S.Targets.ContainsByPredicate([&](const TPair<FString,FVector2D>& T){return T.Key==TEXT("freeform-control:")+Id;}):S.Id==Claim.ControlPhotoId);}):nullptr;
     auto PhotoCard=[](const FS47AlbumShot* S,const FString& Label)->S47Kit::FW
     {
      if(!S||S->bUnavailable||!S->IsDeveloped()||S->Url.IsEmpty())return T::Small(Label+TEXT(" / ORIGINAL UNAVAILABLE"));
      const int32 Seconds=int32(FMath::Fmod(FMath::Max(0.,S->Clock),86400.)),Day=FMath::Max(1,int32(FMath::FloorToDouble((S->Clock-28800.)/86400.))+1);
      return S47Kit::Col({T::Small(Label),
       SNew(SBox).HAlign(HAlign_Center)[T::Picture(S47Image::FromDataUrl(S->Url),240.f)],
       T::Small(FString::Printf(TEXT("%s / D%d %02d:%02d:%02d"),*S->Id,Day,Seconds/3600,(Seconds/60)%60,Seconds%60))});
     };
     if(Control)
     {
      Claims.Add(T::Columns({{.5f,PhotoCard(Baseline,TEXT("FIRST ORIGINAL"))},{.5f,PhotoCard(Control,TEXT("LATER CONTROL"))}},10));
      const FString ControlId=Control->Id;
      if(Claim.ControlPhotoId.IsEmpty()&&!Director->CanRegisterPhotoControl(Id,ControlId))Claims.Add(T::Small(TEXT("COMPARISON UNAVAILABLE / Powered OPS and both developed originals with matching-view records are required. Existing records stay unchanged.")));
      if(Claim.ControlPhotoId.IsEmpty())Claims.Add(T::Button(TEXT("Retain comparison / no payment"),[this,Id,ControlId]{if(Director)Director->RegisterPhotoControl(Id,ControlId);OpenPhotos();},false,Director->CanRegisterPhotoControl(Id,ControlId)));
      else Claims.Add(T::Small(TEXT("COMPARISON RETAINED / Both originals unchanged. Identity and shared cause unresolved.")));
     }
     else
     {
      Claims.Add(PhotoCard(Baseline,TEXT("FIRST ORIGINAL")));
      Claims.Add(T::Small(Director->PhotoControlHint(Id)));
      if(!Planned&&Shot&&Shot->Pose.IsSet())Claims.Add(T::Button(TEXT("Plan return to original viewpoint / no payment"),[this,Id]{if(Director)Director->PlanPhotoControl(Id);OpenPhotos();},false,CanRegisterPhotoEvidence()));
      if(Planned)Claims.Add(T::Small(!Baseline||Baseline->bUnavailable||Baseline->Url.IsEmpty()||!Baseline->IsDeveloped()||!Baseline->Pose.IsSet()||!Baseline->Pose->Valid()?TEXT("CONTROL UNAVAILABLE / The frozen original view is unavailable. This plan and existing originals remain retained without a comparison."):TEXT("NEXT / After the passage ends, take the matching view and develop the separate original at PHOTO LAB. Compare both here.")));
     }
   }
   // Optional research idea comes after the independent free return-photo route.
   if(Orb)
   {
    if(Claim.LocalNoisePlannedAt>=0)Claims.Add(T::Small(Director->LocalNoisePlanHint(Id)));
    else if(Shot&&!Shot->bUnavailable&&!Shot->Url.IsEmpty()&&Shot->IsDeveloped())
    {
     Claims.Add(T::Small(TEXT("Optional: keep an idea for a later local-noise recording. It does not explain the light or unlock equipment. You can repeat the photograph now.")));
     Claims.Add(T::Button(TEXT("Keep a local-noise check for later / no payment"),[this,Id]{if(Director)Director->PlanLocalNoiseCheck(Id);OpenPhotos();},false,CanRegisterPhotoEvidence()));
    }
   }
  }
  if(!Claims.IsEmpty())Rows.Add(T::Panel(TEXT("FIELD PHOTOGRAPHS / FIELD ALLOWANCE"),S47Kit::Col(Claims)));
  // LOOP-3-20261109 (HELHETLIG_LOKKE step 3): a photograph from one of the night's happenings out there, or a trip camera frame, is a
  // field record OPS registers once for field allowance under the week's cap (FS47FreeformDirector::RegisterFieldRecord). Dr. Ward counts
  // the album either way; this only pays. The newest few.
  TArray<S47Kit::FW> Records;int32 Listed=0;
  if(M&&M->Night&&M->Night->Album)
  {
   const auto& Shots=M->Night->Album->Shots();
   for(int32 I=Shots.Num()-1;I>=0&&Listed<4;--I)
   {
    const FS47AlbumShot& P=Shots[I];
    if(!FieldRecordDue(P)||P.Id==PhotoPick)continue; // D48-20261110: the frame on the screen registers beside the picture
    ++Listed;const FString ShotId=P.Id;
    Records.Add(T::Small(FString::Printf(TEXT("%s / %s / %s"),*P.Id,*FS47Places::SlipClock(P.Clock),*P.Text.Left(60))));
    if(!P.IsDeveloped())Records.Add(T::Small(TEXT("UNDEVELOPED ORIGINAL / PHOTO LAB: Develop, fix and collect it at the wet bench first.")));
    else Records.Add(T::Button(FString::Printf(TEXT("Register field record / $%d field allowance"),int32(FieldRecordPay)),[this,ShotId]{if(Director)Director->RegisterFieldRecord(ShotId);OpenPhotos();},false,CanRegisterPhotoEvidence()));
   }
  }
  if(!Records.IsEmpty())Rows.Add(T::Panel(TEXT("FIELD RECORDS / A NIGHT OUT ON THE LAND"),S47Kit::Col(Records)));
 }
}
void FS47FreeformSession::OpenPhotos()
{
 if(!RequirePower()||!M||!M->UI)return;
 static const TArray<FS47AlbumShot> None;
 const TArray<FS47AlbumShot>& Shots=M->Night&&M->Night->Album?M->Night->Album->Shots():None;
 auto Lost=[](const FS47AlbumShot& P){return P.bUnavailable||P.Url.IsEmpty();};
 auto Paid=[this](const FS47AlbumShot& P){return NightPhotoRegistered(P.Id)||(Director&&Director->PhotoClaims().ContainsByPredicate([&P](const auto& C){return C.PhotoId==P.Id&&C.bPaid;}));};
 auto Due=[this,&Lost](const FS47AlbumShot& P){return !Lost(P)&&((Director&&Director->PhotoClaims().ContainsByPredicate([&P](const auto& C){return C.PhotoId==P.Id&&!C.bPaid;}))||FieldRecordDue(P));};
 int32 Latent=0,Registered=0,Owed=0,Pick=INDEX_NONE,FirstDue=INDEX_NONE;
 for(int32 I=Shots.Num()-1;I>=0;--I)
 {
  const FS47AlbumShot& P=Shots[I];
  if(!Lost(P)&&!P.IsDeveloped())++Latent;
  if(Paid(P))++Registered;else if(Due(P)){++Owed;if(FirstDue<0&&P.IsDeveloped())FirstDue=I;}
  if(P.Id==PhotoPick)Pick=I;
 }
 // D48-20261110 (Tom: the tab and what is on it must read better). The frame read is the main element, large at the left; its facts
 // and its one amber key stand beside it; one contact strip of six larger thumbnails runs under them, turned a strip at a time; the
 // allowance desk's retained plans and comparisons come last. Everything the common night needs stands on one 1200 x 900 screen.
 const int32 Strips=FMath::Max(1,(Shots.Num()+PhotoStrip-1)/PhotoStrip);
 if(Pick<0&&Shots.Num()){Pick=FirstDue>=0?FirstDue:Shots.Num()-1;PhotoPick=Shots[Pick].Id;PhotoPage=(Shots.Num()-1-Pick)/PhotoStrip;}
 PhotoPage=FMath::Clamp(PhotoPage,0,Strips-1);
 TArray<S47Kit::FW> Rows,Desk;
 PhotoDeskRows(Desk);
 // one line: the count, and the one thing that matters most about it
 const FString Standing=Owed>0?FString::Printf(TEXT("%d ALLOWANCE%s DUE"),Owed,Owed==1?TEXT(""):TEXT("S")):Latent>0?FString::Printf(TEXT("%d NOT DEVELOPED"),Latent):FString::Printf(TEXT("%d REGISTERED"),Registered);
 Rows.Add(T::Banner(FString::Printf(TEXT("FILM RECORD / %d FRAME%s / %s"),Shots.Num(),Shots.Num()==1?TEXT(""):TEXT("S"),*Standing),Owed>0));
 if(Shots.IsEmpty())
  Rows.Add(T::Paper(TEXT("NO FRAMES ON RECORD.\n\nCarry the field camera when you go out. Develop, fix and collect the film at the PHOTO LAB wet bench.\n\nThis terminal reads developed originals off the video digitizer. A photograph the station can put a time and a place to is worth a field allowance, registered here.")));
 else
 {
  const FS47AlbumShot& P=Shots[Pick];const bool bRead=!Lost(P)&&P.IsDeveloped();
  const int32 Seconds=int32(FMath::Fmod(FMath::Max(0.,P.Clock),86400.)),Day=FMath::Max(1,int32(FMath::FloorToDouble((P.Clock-28800.)/86400.))+1);
  const auto* Claim=Director?Director->PhotoClaims().FindByPredicate([&P](const auto& K){return K.PhotoId==P.Id;}):nullptr;
  const bool bClaimDue=Claim&&!Claim->bPaid&&bRead,bRecordDue=!Claim&&bRead&&FieldRecordDue(P);
  // the place in plain words: the album writes "DAY 2 / Service yard"; a frame of one named thing ("Alderete farm.") puts its name first
  FString Where=P.Place;int32 Slash=INDEX_NONE;
  if(Where.StartsWith(TEXT("DAY "))&&Where.FindChar(TEXT('/'),Slash))Where=Where.Mid(Slash+1).TrimStartAndEnd();
  FString Named=P.Text;if(Named.EndsWith(TEXT(".")))Named.LeftChopInline(1);
  const bool bOneName=bRead&&!Named.IsEmpty()&&Named.Len()<=24&&!Named.Contains(TEXT(" and "))&&!Named.Contains(TEXT(","))&&!Named.StartsWith(TEXT("Trip camera"))&&!Named.StartsWith(TEXT("Too dark"));
  if(bOneName&&!Where.Equals(Named,ESearchCase::IgnoreCase))
  {
   if(Where.Len()>1&&FChar::IsLower(Where[1]))Where[0]=FChar::ToLower(Where[0]);
   Where=Where.IsEmpty()?Named:Named+TEXT(", ")+Where;
  }
  if(Where.IsEmpty())Where=TEXT("Not recorded");
  // what the station has put to the frame: its claim, the night's record it is from, or what the camera saw
  FString Shows;
  if(Lost(P))Shows=TEXT("Nothing to read; the original is gone.");
  else if(!P.IsDeveloped())Shows=TEXT("Nothing read yet; the film is not developed.");
  else if(Claim)Shows=Claim->Title;
  else if(P.Text.StartsWith(TEXT("Trip camera")))Shows=P.Text;
  else if(const FString Record=NightPhotoFrom(P.Clock);!Record.IsEmpty()){int32 Dot=INDEX_NONE;Shows=TEXT("From the night's record: ")+(Record.FindLastChar(TEXT('.'),Dot)?Record.Mid(Dot+1):Record).Replace(TEXT("-"),TEXT(" ")).Replace(TEXT("_"),TEXT(" "));}
  else if(!bOneName&&!P.Text.IsEmpty())Shows=P.Text;
  else Shows=TEXT("Nothing registered.");
  if(bRead&&P.bOdd)Shows+=TEXT(" Operator mark: something is in this frame.");
  if(!P.Enlarged.IsEmpty()){const int32 Of=Shots.IndexOfByPredicate([&P](const FS47AlbumShot& S){return S.Id==P.Enlarged;});Shows=(Of>=0?FString::Printf(TEXT("A print of F%02d. "),Of+1):FString(TEXT("A print. ")))+Shows;}
  Shows=Shows.Left(1).ToUpper()+Shows.Mid(1);
  const FString State=Lost(P)?TEXT("ORIGINAL LOST"):P.LabStage==0?TEXT("NOT DEVELOPED / EXPOSED"):P.LabStage==1?TEXT("NOT DEVELOPED / IN THE TANK"):P.LabStage==2?TEXT("NOT DEVELOPED / NOT COLLECTED")
   :Paid(P)?TEXT("REGISTERED / ALLOWANCE PAID"):Due(P)?TEXT("DEVELOPED / ALLOWANCE DUE"):TEXT("DEVELOPED / NOTHING TO CLAIM");
  // the one amber key: register what pays; otherwise the next thing to do with the frame is in the darkroom, so the operator stands up
  auto StandUp=[this](const FString& Note){if(M&&M->UI)M->UI->Toast(Note,4.f);if(const auto View=TerminalView.Pin())View->Leave();};
  const S47Kit::FW Act=[&]()->S47Kit::FW
  {
   if(Lost(P))return T::Small(TEXT("The original is gone. The record stays as it is."));
   if(!P.IsDeveloped())return T::Button(TEXT("GO TO THE DARKROOM"),[StandUp]{StandUp(TEXT("PHOTO LAB / Develop, fix and collect the film at the wet bench."));},true);
   if(bClaimDue){const FString ClaimId=Claim->Id;return T::Button(FString::Printf(TEXT("REGISTER / $%d"),Claim->Reward),[this,ClaimId]{if(Director)Director->RegisterPhotoClaim(ClaimId);OpenPhotos();},true,CanRegisterPhotoEvidence());}
   if(bRecordDue){const FString ShotId=P.Id;return T::Button(FString::Printf(TEXT("REGISTER RECORD / $%d"),int32(FieldRecordPay)),[this,ShotId]{if(Director)Director->RegisterFieldRecord(ShotId);OpenPhotos();},true,CanRegisterPhotoEvidence());}
   if(P.Enlarged.IsEmpty())return T::Button(TEXT("MAKE A PRINT"),[StandUp]{StandUp(TEXT("DARKROOM / Put this frame in the enlarger for a print."));},true);
   return T::Small(TEXT("A print. Nothing further to do with it here."));
  }();
  auto Fact=[](const FString& Name,const FString& Shown){return S47Kit::Col({T::Small(Name),T::Text(Shown)},0);};
  TArray<S47Kit::FW> Facts;
  Facts.Add(T::Value(TEXT("FRAME"),FString::Printf(TEXT("%02d OF %02d"),Pick+1,Shots.Num())));
  Facts.Add(T::Value(TEXT("TAKEN"),FString::Printf(TEXT("DAY %02d / %02d:%02d"),Day,Seconds/3600,(Seconds/60)%60)));
  Facts.Add(Fact(TEXT("PLACE"),Where));
  Facts.Add(Fact(TEXT("SHOWS"),Shows));
  Facts.Add(T::Banner(State,Due(P)&&bRead));
  Facts.Add(Act);
  const FString Title=FString::Printf(TEXT("FRAME %02d / %s %02d:%02d"),Pick+1,*TerminalDate(P.Clock),Seconds/3600,(Seconds/60)%60);
  Rows.Add(T::Columns({{.58f,T::Panel(Title,SNew(SBox).HAlign(HAlign_Center)[T::Picture(bRead?S47Image::FromDataUrl(P.Url):nullptr,440.f)],true)},{.42f,T::Panel(FString(),S47Kit::Col(Facts))}},12));
  // the strip: newest first, six to a strip, the keys at its ends turning it
  TArray<TPair<float,S47Kit::FW>> Cells;
  Cells.Add({.07f,T::Button(TEXT("<"),[this]{--PhotoPage;PhotoPick.Reset();PhotoPickSheet();},false,PhotoPage>0)});
  for(int32 C=0;C<PhotoStrip;++C)
  {
   const int32 I=Shots.Num()-1-(PhotoPage*PhotoStrip+C);
   if(I<0){Cells.Add({.86f/PhotoStrip,S47Kit::FW(SNew(SBox))});continue;}
   const FS47AlbumShot& S=Shots[I];const FString Id=S.Id;const bool bSeen=!Lost(S)&&S.IsDeveloped();
   const TCHAR* Mark=Lost(S)?TEXT(" LOST"):!S.IsDeveloped()?TEXT(" FILM"):Paid(S)?TEXT(" PAID"):Due(S)?TEXT(" DUE"):TEXT("");
   Cells.Add({.86f/PhotoStrip,T::Picture(bSeen?S47Image::FromDataUrl(S.Url):nullptr,96.f,FString::Printf(TEXT("F%02d%s"),I+1,Mark),I==Pick,[this,Id]{PhotoPick=Id;OpenPhotos();},bSeen&&Due(S))});
  }
  Cells.Add({.07f,T::Button(TEXT(">"),[this]{++PhotoPage;PhotoPick.Reset();PhotoPickSheet();},false,PhotoPage<Strips-1)});
  Rows.Add(T::Panel(Strips>1?FString::Printf(TEXT("CONTACT STRIP / %d OF %d / NEWEST FIRST"),PhotoPage+1,Strips):FString(TEXT("CONTACT STRIP / NEWEST FIRST")),T::Columns(Cells,8)));
 }
 // The desk's retained plans and comparisons under the strip (what pays on the frame read stands beside its picture).
 Rows.Append(Desk);
 if(!Feedback.IsEmpty())Rows.Add(T::Small(Feedback));
 OpenTerminal(3,TEXT("OPS / PHOTOGRAPHS"),OpsShell(6,S47Kit::Col(Rows)));
}
// Turning the sheet: the newest frame on the sheet turned to is the one read.
void FS47FreeformSession::PhotoPickSheet()
{
 if(M&&M->Night&&M->Night->Album){const auto& Shots=M->Night->Album->Shots();const int32 I=Shots.Num()-1-FMath::Max(0,PhotoPage)*PhotoStrip;if(Shots.IsValidIndex(I))PhotoPick=Shots[I].Id;}
 OpenPhotos();
}
void FS47FreeformSession::OpenTracking()
{
 if(!RequirePower()||!M||!M->UI||!M->Web||!CurrentJob())return;
 const auto P=EffectiveProfile(*CurrentJob());const FString Name=FS47Observing::Runs()[CurrentJob()->Source].Source;
 auto ArrayAzEl=[this]{double Az=0,El=0;int32 N=0;for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishRemoteAvailable(I)){Az+=M->Web->DishAz(I);El+=M->Web->DishEl(I);++N;}return N?FVector2D(Az/N,El/N):FVector2D(0,0);};
 auto Source=T::Panel(TEXT("SOURCE"),S47Kit::Col({T::Value(TEXT("TARGET"),Name,true),T::Value(TEXT("FREQ"),FString::Printf(TEXT("%.3f MHz"),P.F)),
  T::Value(TEXT("AZ"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.1f°"),TargetAz());})),T::Value(TEXT("EL"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.1f°"),TargetEl());}))}),true);
 auto Done=[this]{const FJob* J=CurrentJob();if(!J||J->Source!=0)return 0;return J->Stage>=EJobStage::Chart?5:bCrossRunning?CrossPoint:0;};
 auto Cal=T::Panel(TEXT("CALIBRATION / 5-POINT"),S47Kit::Col({T::FivePoint(Done,[this]{return bCrossRunning;},[this]{return CurrentJob()&&CurrentJob()->Source==0;}),
  T::Value(TEXT("PROGRESS"),TAttribute<FString>::CreateLambda([Done]{return FString::Printf(TEXT("%d / 5"),Done());}))}),true);
 auto Nudge=[this](double Az,double El){return [this,Az,El]{const FVector2D A=AimAzEl();SetAim(FVector2D(A.X+Az,A.Y+El));};};
 auto Plot=T::Panel(FString::Printf(TEXT("AZ / EL PLOT (TARGET %s)"),*Name),S47Kit::Col({T::SkyPlot([this]{return AimAzEl();},ArrayAzEl,Name,[this](FVector2D At){SetAim(At);},[this]{return AimOnTarget();}),
  // Without a mouse: 5° steps always land within the marker's 3° of the source.
  T::Columns({{1.f,T::Button(TEXT("AZ -5"),Nudge(-5,0))},{1.f,T::Button(TEXT("AZ +5"),Nudge(5,0))},{1.f,T::Button(TEXT("EL -5"),Nudge(0,-5))},{1.f,T::Button(TEXT("EL +5"),Nudge(0,5))}},6)}),true);
 auto Current=T::Panel(TEXT("CURRENT (ARRAY)"),S47Kit::Col({T::Value(TEXT("AZ"),TAttribute<FString>::CreateLambda([ArrayAzEl]{return FString::Printf(TEXT("%.1f°"),ArrayAzEl().X);}),true),T::Value(TEXT("EL"),TAttribute<FString>::CreateLambda([ArrayAzEl]{return FString::Printf(TEXT("%.1f°"),ArrayAzEl().Y);}),true)}),true);
 auto Target=T::Panel(TEXT("TARGET"),S47Kit::Col({T::Value(TEXT("AZ"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.1f°"),TargetAz());}),true),T::Value(TEXT("EL"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.1f°"),TargetEl());}),true)}),true);
 TArray<S47Kit::FW> Right={Current,Target,
  T::Banner(TAttribute<FString>::CreateLambda([this]{if(const FJob* G=CurrentJob();G&&EveningGated(*G))return EveningLine(*G);const FString State=PointReady()?Settle>=1?TEXT("ON TARGET / STEADY"):TEXT("ON TARGET / SETTLING"):CurrentJob()&&CurrentJob()->Stage==EJobStage::Pointed?TEXT("ARRAY MOVING"):TEXT("READY TO POINT");return State+TEXT("\n")+ArrayMeasurementStatus();}),TAttribute<bool>::CreateLambda([this]{return !PointReady();})),
  T::Text(TAttribute<FString>::CreateLambda([this]{AimAzEl();return FString::Printf(TEXT("YELLOW MARKER AZ %.1f° EL %.1f°\n%s"),Aim.X,Aim.Y,AimOnTarget()?TEXT("ON THE SOURCE"):TEXT("DRAG IT ONTO THE SOURCE'S AZ / EL"));}),TAttribute<int32>(0)),
  T::Big(FString(TEXT("POINT ARRAY")),[this]{if(PointSelected())M->UI->Close();},1,TAttribute<bool>::CreateLambda([this]{AimAzEl();const auto* J=CurrentJob();return J&&J->Stage<=EJobStage::Pointed&&AimOnTarget()&&!EveningGated(*J);}))};
 // ANSWER-20261108: any point of the sky, off schedule, held until the array is pointed elsewhere.
 Right.Add(T::Button(TEXT("HOLD THE ARRAY HERE / OFF SCHEDULE"),[this]{AimAzEl();if(SkyAnswer&&SkyAnswer->Hold(Aim.X,Aim.Y))M->UI->Close();},false,TAttribute<bool>::CreateLambda([this]{AimAzEl();const auto* J=CurrentJob();return SkyAnswer.IsValid()&&!AimOnTarget()&&Aim.Y>=FS47Answer::MinEl&&!(J&&J->Stage==EJobStage::Integrating);})));
 if(CurrentJob()->Phenomenon==TEXT("wrong_source")&&CurrentJob()->Stage==EJobStage::Recorded)
 {
  Right.Add(T::Text(TAttribute<FString>::CreateLambda([this]{const auto* J=CurrentJob();return J?FString::Printf(TEXT("ADJACENT CONTROL %d/3 / SETTLE %.1f s / SAMPLE %.1f s"),J->BeamPoint+1,J->BeamSettle,J->BeamSample):FString();}),TAttribute<int32>(0)));
  Right.Add(T::Button(TEXT("Measure west, east and centre beams"),[this]{if(StartControl())M->UI->Close();},true,!CurrentJob()->bControl));
 }
 TArray<S47Kit::FW> Rows;
 Rows.Add(T::Columns({{.28f,S47Kit::Col({Source,Cal})},{.46f,Plot},{.26f,S47Kit::Col(Right)}},12));
 Rows.Add(T::Small(TEXT("Watch the actual dishes through the window. Up to three isolated antennas permit slower acquisition (24/27: 79% rate); fewer than 24 block array measurements until repaired.")));
 OpenTerminal(2,TEXT("TRACKING / ARRAY CONTROL"),S47Kit::Col(Rows),true);
}
void FS47FreeformSession::StartDriveCheck()
{
 if(DriveCheckAt>=0||!RequirePower()||!M||!M->Web)return;
 DriveCheckAt=0;DriveCheckDish=-1;DriveCheckResult.Reset();DriveCheckTone=0;
 if(M->Audio){M->Audio->Play(TEXT("rack_switch"),.40f);M->Audio->Beep(180.f,.30f,.030f,.02f,TEXT("sawtooth"));}
}
void FS47FreeformSession::StepDriveCheck(double Dt)
{
 if(DriveCheckAt<0||!M||!M->Web)return;
 // The check belongs to the terminal: leaving it, or losing the feeder, drops the run and nothing is learnt.
 if(ActiveTerminal!=0||!HasPower()){DriveCheckAt=-1;DriveCheckDish=-1;if(M->Audio)M->Audio->Beep(220.f,.12f,.03f);return;}
 if(Held())return;
 DriveCheckAt+=Dt;
 const int32 Count=M->Web->DishCount(),Now=int32(DriveCheckAt/.3);
 if(Now>=Count){FinishDriveCheck();return;}
 if(Now!=DriveCheckDish)
 {
  DriveCheckDish=Now;
  if(M->Audio)
  {
   const bool bUp=M->Web->DishRemoteAvailable(Now);
   M->Audio->Play(TEXT("small_switch"),bUp?.22f:.12f,FVector::ZeroVector,bUp?1.35f:.8f);
   M->Audio->Beep(bUp?620.f+14.f*Now:240.f,.045f,.020f,.03f,bUp?TEXT("square"):TEXT("sawtooth"));
  }
 }
}
void FS47FreeformSession::FinishDriveCheck()
{
 DriveCheckAt=-1;DriveCheckDish=-1;DriveCheckDay=DayNumber;
 const int32 Count=M&&M->Web?M->Web->DishCount():27,Up=AvailableDishCount();
 const FString Next=Field?Field->NextServiceDish():FString();
 const bool bReserve=HasUpgrade(0);
 if(Field&&Field->HasArrayFault()){DriveCheckTone=2;DriveCheckResult=FString::Printf(TEXT("%s: DRIVE FAULT CONFIRMED / SEE MAINTENANCE"),*Field->ServiceDishId());} // D45-20261110: the order is issued on this PC now
 else if(FaultDueLater())
 {
  DriveCheckTone=2;
  DriveCheckResult=FString::Printf(TEXT("%s: RELAY CHATTERS UNDER LOAD. IT WILL NOT LAST PAST %02d:00. A STATION ROUND WITH A RELAY IN STOCK CATCHES IT / RELAYS IN STOCK %d"),*Next,int32(FS47NightRhythm::FaultHour(JobSeed,DayNumber)),InventoryCount(TEXT("relay")));
 }
 else if(RulesLive()&&FS47NightRhythm::FaultDay(JobSeed,DayNumber+1,bReserve))
 {
  DriveCheckTone=1;
  DriveCheckResult=FString::Printf(TEXT("%s: CONTACT RESISTANCE CREEPING. NOT TODAY. HAVE A RELAY IN STOCK TOMORROW / RELAYS IN STOCK %d"),*Next,InventoryCount(TEXT("relay")));
 }
 else if(Up<Count){DriveCheckTone=1;DriveCheckResult=FString::Printf(TEXT("%d OF %d DRIVES ANSWER. THE REST ARE ISOLATED AT THE PEDESTAL: FIELD WORK"),Up,Count);}
 else{DriveCheckTone=0;DriveCheckResult=FString::Printf(TEXT("%d DRIVES WITHIN LIMITS"),Count);}
 if(M&&M->Audio)
 {
  if(DriveCheckTone==2){M->Audio->Beep(440.f,.16f,.045f);M->Audio->Beep(330.f,.26f,.045f,.20f);M->Audio->Beep(440.f,.16f,.045f,.56f);M->Audio->Beep(330.f,.26f,.045f,.76f);}
  else if(DriveCheckTone==1){M->Audio->Beep(660.f,.10f,.035f);M->Audio->Beep(495.f,.20f,.035f,.14f);}
  else{M->Audio->Beep(880.f,.07f,.03f);M->Audio->Beep(1175.f,.07f,.03f,.09f);M->Audio->Beep(1568.f,.12f,.03f,.18f);}
 }
 UE_LOG(LogTemp,Display,TEXT("S47: drive check: day %d, %s"),DayNumber,*DriveCheckResult);
}
void FS47FreeformSession::OpenStatus()
{
 if(!RequirePower()||!M||!M->UI||!M->Web)return;
 // D45-20261110 (Tom: maintenance is done on this PC): STATUS is the page it was, MAINTENANCE the faults, their order and the work orders
 static const TCHAR* StatusTabs[]={TEXT("STATUS"),TEXT("MAINTENANCE")};
 TArray<T::FNav> TabItems;for(int32 I=0;I<2;++I)TabItems.Add({StatusTabs[I],StatusTab==I,[this,I]{StatusTab=I;OpenStatus();}});
 if(StatusTab==1){OpenTerminal(0,TEXT("ARRAY STATUS / MAINTENANCE"),S47Kit::Col({T::Tabs(TabItems),MaintenancePage()}),true);return;}
 const int32 Count=M->Web->DishCount();SelectedDish=FMath::Clamp(SelectedDish,0,FMath::Max(0,Count-1));
 auto Short=[this](int32 I){return M->Web->DishId(I).Replace(TEXT("-"),TEXT(""));};
 auto State=[this](int32 I){if(DriveCheckAt>=0)return I==DriveCheckDish?1:!M->Web->DishRemoteAvailable(I)?2:0;if(I==SelectedDish)return 1;if(!M->Web->DishRemoteAvailable(I))return 2;return FMath::Abs(FMath::FindDeltaAngleDegrees(M->Web->DishAz(I),M->Web->DishTargetAz(I)))>.1||FMath::Abs(M->Web->DishEl(I)-M->Web->DishTargetEl(I))>.1?3:0;};
 // D45-20261110: a fault picked on the grid is also the one MAINTENANCE opens on
 auto Pick=[this](int32 I){SelectedDish=I;if(Field){if(const auto* Down=Field->ArrayWear().BranchOfDish(M->Web->DishId(I)))MaintPick=Down->Id;else if(M->Web->DishId(I)==Field->ServiceDishId()&&(Field->HasArrayFault()||ServiceGuide==1))MaintPick=TEXT("relay");}
  if(Field&&M->Web->DishId(I)==Field->ServiceDishId()){Field->DiagnoseService();RecordEvidence(TEXT("service-diagnosed-")+Field->ServiceDishId(),Field->ServiceDishId()+TEXT(" command/encoder status inspected at the status terminal."));}OpenStatus();};
 auto Grid=T::Panel(FString::Printf(TEXT("ARRAY STATUS / %d ANTENNAS"),Count),S47Kit::Col({T::AntennaGrid(Count,Short,State,Pick),
  T::Columns({{.5f,T::Banner(TAttribute<FString>::CreateLambda([this,Count]{return FString::Printf(TEXT("%d / %d AVAILABLE"),AvailableDishCount(),Count);}),TAttribute<bool>::CreateLambda([this,Count]{return AvailableDishCount()<Count;}))},
              {.5f,T::Banner(TAttribute<FString>::CreateLambda([this]{return bPowerFault?FString(HasPower()?TEXT("FEEDER FAULT / ON GENERATOR"):TEXT("FEEDER VOLTAGE UNSTABLE")):SkySag>0?FString(TEXT("LINE VOLTAGE SAGGING")):SkyOut>0?FString(TEXT("MAINS LOST / ON GENERATOR")):FString(TEXT("FEEDER NORMAL"));}),TAttribute<bool>::CreateLambda([this]{return bPowerFault||SkyCutLive();}))}}),
  // FIX-20261110 (Tom: the array PC has too little to do): the drive check runs every element against a dummy load and says what
  // it heard (see StartDriveCheck); the grid above shows the element under test.
  T::Columns({{.62f,T::Meter(TEXT("DRIVE CHECK"),TAttribute<FString>::CreateLambda([this,Short]{return DriveCheckAt>=0&&DriveCheckDish>=0?Short(DriveCheckDish)+TEXT(" / DUMMY LOAD"):DriveCheckDay==DayNumber?FString(TEXT("RUN TODAY")):FString(TEXT("NOT RUN TODAY"));}),
    TAttribute<float>::CreateLambda([this,Count]{return DriveCheckAt>=0?float(FMath::Clamp(DriveCheckAt/(.3*FMath::Max(1,Count)),0.,1.)):DriveCheckDay==DayNumber?1.f:0.f;}))},
   {.38f,T::Big(FString(TEXT("RUN DRIVE CHECK")),[this]{StartDriveCheck();},1,TAttribute<bool>::CreateLambda([this]{return DriveCheckAt<0;}))}}),
  T::Text(TAttribute<FString>::CreateLambda([this]{return DriveCheckAt>=0?FString(TEXT("TESTING / KEEP THE TERMINAL OPEN")):DriveCheckDay==DayNumber?DriveCheckResult:FString(TEXT("Runs every drive against a dummy load. About 8 seconds."));}),
   TAttribute<int32>::CreateLambda([this]{return DriveCheckAt<0&&DriveCheckDay==DayNumber&&DriveCheckTone>0?-1:0;})),
  T::Small(TEXT("KEY / outline READY / amber SELECTED OR UNDER TEST / red ISOLATED OR FAULT / blinking MOVING"))}));
 const int32 Dish=SelectedDish;
 auto Detail=T::Panel(Short(Dish)+TEXT(" / ANTENNA DETAIL"),S47Kit::Col({
  T::Value(TEXT("MODE"),TAttribute<FString>::CreateLambda([this,Dish]{return M->Web->DishRemoteAvailable(Dish)?FString(TEXT("REMOTE")):FString(TEXT("ISOLATED / LOCAL"));}),true),
  T::Value(TEXT("ACTUAL"),TAttribute<FString>::CreateLambda([this,Dish]{return FString::Printf(TEXT("AZ %.1f° EL %.1f°"),M->Web->DishAz(Dish),M->Web->DishEl(Dish));})),
  T::Value(TEXT("TARGET"),TAttribute<FString>::CreateLambda([this,Dish]{return FString::Printf(TEXT("AZ %.1f° EL %.1f°"),M->Web->DishTargetAz(Dish),M->Web->DishTargetEl(Dish));})),
  T::Value(TEXT("DRIVE"),TAttribute<FString>::CreateLambda([this,Dish]{return !M->Web->DishRemoteAvailable(Dish)?FString(TEXT("SERVICE")):FMath::Abs(FMath::FindDeltaAngleDegrees(M->Web->DishAz(Dish),M->Web->DishTargetAz(Dish)))>.1||FMath::Abs(M->Web->DishEl(Dish)-M->Web->DishTargetEl(Dish))>.1?FString(TEXT("MOVING")):FString(TEXT("READY"));}),true),
  T::Value(TEXT("RELAY"),TAttribute<FString>::CreateLambda([this,Dish]{return Field&&M->Web->DishId(Dish)==Field->ServiceDishId()&&Field->HasArrayFault()?FString(TEXT("FAULT")):FString(TEXT("OK"));}),true)}),true);
 TArray<S47Kit::FW> Service;
 // D45-20261110: the fault list and the work orders are on the MAINTENANCE tab; here only what is open and the way there
 Service.Add(T::Text(TAttribute<FString>::CreateLambda([this]{return MaintenanceLine();}),TAttribute<int32>::CreateLambda([this]{return Field&&(Field->HasArrayFault()||Field->ArrayWear().DownCount()>0||ServiceGuide==1)?-1:0;})));
 Service.Add(T::Big(FString(TEXT("OPEN MAINTENANCE")),[this]{StatusTab=1;OpenStatus();},1));
 Service.Add(T::Big(FString(TEXT("INSPECT SERVICE RECORD")),[this]{OpenArchive(false);},2));
 TArray<S47Kit::FW> Rows;Rows.Add(T::Tabs(TabItems));
 Rows.Add(T::Text(TAttribute<FString>::CreateLambda([this]{return ArrayMeasurementStatus();}),TAttribute<int32>(0)));
 // FIX-20261110: what the array has been pulled toward (FS47Places' bearings, as the slips print them) and what it is picking up
 // out of band right now (the same interference the screens tear with).
 TArray<S47Kit::FW> Beam;
 Beam.Add(T::Meter(TEXT("OUT-OF-BAND PICKUP"),TAttribute<FString>::CreateLambda([this]{const float L=ScreenInterference();return FString(L>.55f?TEXT("STRONG / NOT A SOURCE ON THE CARD"):L>.18f?TEXT("RISING"):L>.04f?TEXT("TRACE"):TEXT("QUIET"));}),
  TAttribute<float>::CreateLambda([this]{return ScreenInterference();})));
 if(Places&&Places->Bearings().Num())
 {
  const auto& Fixes=Places->Bearings();
  for(int32 I=Fixes.Num()-1;I>=0&&I>=Fixes.Num()-3;--I)
  {
   // D45-20261110: in plain words and no degree sign, as the slip has it now
   const auto& B=Fixes[I];const bool bWarm=FS47Places::Live(B,WorldClock);
   Beam.Add(T::Text(FString::Printf(TEXT("DAY %d %s / %03.0f DEG (%s), GIVE OR TAKE %.0f%s / %s"),B.Day,B.Clock>=0?*FS47Places::SlipClock(B.Clock):TEXT("--:--"),B.Az,*FS47Places::CompassWord(B.Az).ToUpper(),B.Spread,B.bCar?TEXT(" / FROM THE TRUCK"):TEXT(""),
    bWarm?(B.bRing?TEXT("CIRCLED ON YOUR MAP UNTIL NOON"):TEXT("ON YOUR MAP UNTIL NOON")):TEXT("EXPIRED")),bWarm?-1:0));
  }
 }
 else Beam.Add(T::Small(TEXT("NO BEARINGS YET. When a registered observation picks up something away from where the dishes were pointed, its direction from the station shows here and on your map.")));
 Rows.Add(T::Columns({{.6f,S47Kit::Col({Grid,T::Panel(TEXT("OFF THE MAIN BEAM"),S47Kit::Col(Beam))})},{.4f,S47Kit::Col({Detail,T::Panel(TEXT("MAINTENANCE"),S47Kit::Col(Service),true)})}},12));
 OpenTerminal(0,TEXT("ARRAY STATUS / MAINTENANCE"),S47Kit::Col(Rows),true);
}
FString FS47FreeformSession::MaintenanceLine()const
{
 // D45-20261110: open faults (branches and the relay order) and the one the operator works next
 if(!Field)return TEXT("MAINTENANCE / NO OPEN FAULTS");
 const auto& Wear=Field->ArrayWear();const bool bRelay=Field->HasArrayFault()||ServiceGuide==1;
 int32 Open=bRelay?1:0;if(Wear.bActive)Open+=Wear.Ordered().Num();
 if(Open==0)return TEXT("MAINTENANCE / NO OPEN FAULTS");
 const auto* Next=Wear.NextBranch();
 const FString Unit=bRelay&&(!Next||(ServiceGuide==1&&bGuideFocus))?Field->ServiceDishId():FString(Next->Junction);
 return FString::Printf(TEXT("MAINTENANCE / %d FAULT%s / NEXT %s"),Open,Open==1?TEXT(""):TEXT("S"),*Unit);
}
TSharedRef<SWidget> FS47FreeformSession::MaintenancePage()
{
 // D45-20261110 (Tom: the array PC is where maintenance is done, and the player picks what is mended first and in what order, as long
 // as the parts are there): every open fault with the parts on hand, the order it is worked in (MAKE THIS NEXT / MOVE UP / MOVE DOWN),
 // and the relay work order issued and signed off here. The work itself stays at the cabinets. At most six rows: no scrolling.
 struct FFault{FString Id,Unit,Where,Parts;int32 Kind=0;}; // Kind 0 branch, 1 relay work order, 2 off remote for another reason
 TArray<FFault> Faults;
 const bool bWear=Field&&Field->ArrayWear().bActive;
 const bool bRelay=Field&&(Field->HasArrayFault()||ServiceGuide==1);
 const bool bRelayFirst=bRelay&&(ServiceGuide==1&&bGuideFocus);
 auto AddRelay=[this,&Faults]{const int32 S=Field->ServiceStage();Faults.Add({TEXT("relay"),Field->ServiceDishId()+TEXT(" RELAY"),Field->ServiceDishId(),S==9?TEXT("TESTED"):S>=5?TEXT("FITTED"):Field->InventoryCount(TEXT("relay"))>0?TEXT("READY"):TEXT("SHORT"),1});};
 if(bRelayFirst)AddRelay();
 if(bWear)for(const auto* B:Field->ArrayWear().Ordered())Faults.Add({B->Id,B->Title,FString::Printf(TEXT("%s +%d"),B->Junction,B->Dishes.Num()-1),Field->BranchNeeds(*B).IsEmpty()?TEXT("READY"):TEXT("SHORT"),0});
 if(bRelay&&!bRelayFirst)AddRelay();
 if(M&&M->Web)for(int32 I=0;I<M->Web->DishCount()&&Faults.Num()<6;++I)
 {
  const FString Dish=M->Web->DishId(I);
  if(M->Web->DishRemoteAvailable(I)||(Field&&(Field->ArrayWear().BranchOfDish(Dish)||(bRelay&&Dish==Field->ServiceDishId()))))continue;
  Faults.Add({TEXT("dish:")+Dish,Dish+TEXT(" OFF REMOTE"),Dish,TEXT("-"),2});
 }
 // "planned": a relay work order with no fault open yet (the first S-03 service, or planned maintenance)
 if(!Faults.ContainsByPredicate([this](const FFault& Fa){return Fa.Id==MaintPick;})&&!(MaintPick==TEXT("planned")&&!bRelay))MaintPick=Faults.Num()?Faults[0].Id:FString(TEXT("planned"));
 TArray<T::FRow> Lines;
 for(int32 I=0;I<Faults.Num();++I)
 {
  const FFault& Fa=Faults[I];const FString Pick=Fa.Id;
  Lines.Add({{Fa.Kind==2?FString(TEXT("-")):FString::FromInt(I+1),Fa.Unit,Fa.Where,Fa.Parts},Fa.Id==MaintPick,[this,Pick]{MaintPick=Pick;OpenStatus();},I==0&&Fa.Kind!=2?FString(TEXT("NEXT")):FString()});
 }
 TArray<S47Kit::FW> Left;
 if(Faults.Num())Left.Add(T::Table({TEXT("#"),TEXT("FAULT"),TEXT("ANTENNAS"),TEXT("PARTS")},{.08f,.40f,.27f,.25f},Lines));
 else Left.Add(T::Text(TEXT("NO OPEN FAULTS. RUN THE DRIVE CHECK ON THE STATUS TAB TO CATCH THE NEXT ONE BEFORE IT FAILS."),0));
 if(Faults.Num()&&!bRelay)Left.Add(T::Button(ServiceGuide==0?TEXT("FIRST RELAY SERVICE / S-03"):TEXT("PLANNED RELAY SERVICE"),[this]{MaintPick=TEXT("planned");OpenStatus();}));
 Left.Add(T::Small(TEXT("Parts go in your pouch from GARAGE RECEIVING. The repair itself is done at the cabinet in the field.")));
 TArray<S47Kit::FW> R;FString Title;
 if(const auto* B=bWear?FS47ArrayWear::Branch(MaintPick):nullptr;B&&!Field->ArrayWear().IsRestored(MaintPick))
 {
  Title=FString(B->Title)+TEXT(" / ORDER");
  R.Add(T::Text(FString(B->Fault),0));
  for(const auto& Need:B->Needs){const int32 Have=Field->InventoryCount(Need.Item);R.Add(T::Text(FString::Printf(TEXT("%s / NEED %d / HAVE %d"),*SupplyPartName(Need.Item),Need.Count,Have),Have<Need.Count?-1:0));}
  R.Add(T::Small(FString::Printf(TEXT("Work at the junction cabinet beside %s. %d antennas come back with it."),B->Junction,B->Dishes.Num())));
  const TArray<const FS47ArrayWear::FBranch*> Order=Field->ArrayWear().Ordered();const int32 At=Order.IndexOfByKey(B);
  const bool bFirst=Field->ArrayWear().NextBranch()==B&&!bRelayFirst;const FString Id(B->Id);
  R.Add(T::Big(FString(TEXT("MAKE THIS NEXT")),[this,Id]{
   Field->PlanBranch(Id,0);bGuideFocus=false; // an open relay order is kept, only no longer first
   if(const auto* Picked=FS47ArrayWear::Branch(Id))Feedback=FString::Printf(TEXT("%s IS NEXT. The station goal and the lamps in the field now point to its junction at %s. Marked on your map."),Picked->Title,Picked->Junction); // D45-20261110
   Changed(Feedback);OpenStatus();},1,!bFirst));
  R.Add(T::Columns({{.5f,T::Button(TEXT("MOVE UP"),[this,Id,At]{Field->PlanBranch(Id,At-1);OpenStatus();},false,At>0)},{.5f,T::Button(TEXT("MOVE DOWN"),[this,Id,At]{Field->PlanBranch(Id,At+1);OpenStatus();},false,At>=0&&At<Order.Num()-1)}},10));
 }
 else if(MaintPick.StartsWith(TEXT("dish:")))
 {
  const FString Dish=MaintPick.Mid(5);Title=Dish;
  R.Add(T::Text(FString::Printf(TEXT("%s is held off remote control by other work. No fault is logged against it here, and nothing needs ordering."),*Dish),0));
 }
 else
 {
  const FString Dish=Field?Field->ServiceDishId():FString(TEXT("S-03"));const int32 Have=Field?Field->InventoryCount(TEXT("relay")):0;
  Title=((Field&&Field->HasArrayFault())||ServiceGuide==1?Dish+TEXT(" RELAY"):FString(TEXT("RELAY SERVICE")))+TEXT(" / ORDER");
  if(ServiceGuide==1&&Field)
  {
   R.Add(T::Text(ServiceObjective(),-1));
   R.Add(T::Text(FString::Printf(TEXT("DRIVE RELAY / NEED %d / HAVE %d"),Field->ServiceStage()<5?1:0,Have),Field->ServiceStage()<5&&Have<1?-1:0));
   if(CanCompleteServiceGuide())R.Add(T::Big(FString(TEXT("SIGN OFF: REMOTE RESTORED")),[this]{CompleteServiceGuide();OpenStatus();},1));
   else R.Add(T::Big(FString(TEXT("MAKE THIS NEXT")),[this]{bGuideFocus=true;FieldFocus=-1;Feedback=ServiceObjective();Changed(Feedback);OpenStatus();},1,!bGuideFocus));
  }
  else
  {
   R.Add(T::Text(Field&&Field->HasArrayFault()?FString::Printf(TEXT("%s: its command relay has failed. The work order keeps it off remote until a new relay is in and its movement test has passed."),*Dish)
    :ServiceGuide==0?FString(TEXT("FIRST RELAY SERVICE / S-03. Learn to change a drive relay and run its movement test. The order takes S-03 off remote until then."))
    :FString(TEXT("PLANNED RELAY SERVICE. The order picks one antenna and takes it off remote until its relay is changed and tested.")),0));
   R.Add(T::Text(FString::Printf(TEXT("DRIVE RELAY / NEED 1 / HAVE %d"),Have),Have<1?-1:0));
   R.Add(T::Big(FString(TEXT("ISSUE WORK ORDER / USE STOCK")),[this]{if(StartServiceGuide(false))MaintPick=TEXT("relay");OpenStatus();},1));
   R.Add(T::Button(TEXT("ISSUE WORK ORDER / BUY AND HAUL PARTS"),[this]{if(StartServiceGuide(true))MaintPick=TEXT("relay");OpenStatus();}));
   R.Add(T::Small(TEXT("Buy and haul: a relay or donor bought in Caliche, carried in the truck and unpacked at GARAGE RECEIVING. Donors are tested at CALIBRATION / REPAIR first.")));
  }
 }
 return T::Columns({{.58f,T::Panel(TEXT("FAULT LIST / YOUR ORDER"),S47Kit::Col(Left))},{.42f,T::Panel(Title,S47Kit::Col(R),true)}},12);
}
void FS47FreeformSession::OpenReceiver()
{
 if(!RequirePower()||!M||!M->UI||!M->Story||!CurrentJob())return;
 auto& R=M->Story->Rx;const auto P=EffectiveProfile(*CurrentJob());
 R.Azimuth=TargetAz(); // No second, contradictory azimuth dial. D45-20261110: the card's bearing from its selection on (SelectJob).
 const FString Name=FS47Observing::Runs()[CurrentJob()->Source].Source;
 // RX-ACQUIRE-SKY-20261113 (Tom 13 November: ACQUIRE and TUNE were nearly the same page): ACQUIRE is the sky chart, where the source
 // is found; TUNE is the frequency search, with the recording key and the tape. A job opens on the chart until its source is
 // acquired and on TUNE from then on; a tab picked by hand stays until that changes.
 {const FString TabKey=CurrentJob()->Id+(SkyAcquired()?TEXT("+"):TEXT("-"));if(TabKey!=SkyTabKey){SkyTabKey=TabKey;ReceiverTab=SkyAcquired()?1:0;}}
 TSharedPtr<SWidget> SkyKeys;
 static const TCHAR* TabNames[]={TEXT("ACQUIRE"),TEXT("TUNE"),TEXT("ARRAY"),TEXT("RECORD"),TEXT("DATA")};
 // RX-TABS-DUE-20261113: the tab that holds the next step breathes amber with a marker until it is opened (ReceiverDueTab).
 if(ReceiverTab==RxData)RxDataSeenJob=CurrentJob()->Id;
 TArray<T::FNav> TabItems;for(int32 I=0;I<5;++I){T::FNav Tab{TabNames[I],ReceiverTab==I,[this,I]{ReceiverTab=I;OpenReceiver();}};Tab.Due=[this,I]{return ReceiverDueTab()==I;};TabItems.Add(Tab);}
 TArray<S47Kit::FW> Rows;Rows.Add(T::Tabs(TabItems));
 // D45-20261110 (Tom: the target showed only once the array had locked): what the array is doing stands beside the measurement
 // line on every tab, from the job's selection on. Recording keeps its strict rule (ReceiverReady).
 auto ArrayState=[this]{const FJob* J=CurrentJob();if(!J)return FString();
  if(PointReady())return Settle>=1?FString(TEXT("LOCKED")):FString(TEXT("SETTLING"));
  return J->Stage==EJobStage::New?FString(TEXT("NOT POINTED")):FString::Printf(TEXT("SLEWING / %.1f\u00b0 TO GO"),ArrayErrorDeg());};
 Rows.Add(T::Columns({{1.55f,T::Text(TAttribute<FString>::CreateLambda([this]{return ArrayMeasurementStatus();}),TAttribute<int32>(0))},{1.f,T::Value(TEXT("ARRAY"),TAttribute<FString>::CreateLambda(ArrayState),true)}}));
 if(ReceiverTab>1)Rows.Add(T::Small(TEXT("Carrier lock = tuning. Measurement rate = observing seconds."))); // ACQUIRE has the chart, TUNE the tape strip instead
 auto Quality=[this]{const auto* J=CurrentJob();return J?M->Story->Rx.Quality(EffectiveProfile(*J)):0.;};
 auto Controls=[this]{return T::Columns({
  {1.f,T::Control(TEXT("FREQUENCY"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.3f MHz"),M->Story->Rx.Frequency);}),1419.5f,1420.7f,.001f,TAttribute<float>::CreateLambda([this]{return float(M->Story->Rx.Frequency);}),[this](float V){M->Story->Rx.Frequency=V;})},
  {1.f,T::Control(TEXT("GAIN"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.0f"),M->Story->Rx.Gain);}),0,100,1,TAttribute<float>::CreateLambda([this]{return float(M->Story->Rx.Gain);}),[this](float V){M->Story->Rx.Gain=V;})},
  {1.f,T::Control(TEXT("BANDWIDTH"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.0f kHz"),M->Story->Rx.Bandwidth);}),1,100,1,TAttribute<float>::CreateLambda([this]{return float(M->Story->Rx.Bandwidth);}),[this](float V){M->Story->Rx.Bandwidth=V;})}},10);};
 auto Banner=[this]{return T::Banner(TAttribute<FString>::CreateLambda([this]{return ReceiverNextAction().ToUpper();}),TAttribute<bool>::CreateLambda([this]{return !ReceiverReady();}));};
 if(ReceiverTab==0)
 {
  // RX-ACQUIRE-SKY-20261113: the sky chart (S47FreeformSky.cpp). It has the keys first while there is a search to steer.
  Rows.Add(SkyPage(SkyKeys));
 }
 else if(ReceiverTab==1)
 {
  // RX-ACQUIRE-SKY-20261113: the frequency search, on the page ACQUIRE used to share with it: the zoomed spectrum (at ACQUIRE's old
  // height, so the page with the tape strip still fits the glass), the dials, the recording key and the tape.
  // D48-20261110 (D46-14, Tom: every receiver tab needs something to do): the target zoomed so one dial step shows, and a carrier-lock
  // meter that moves while the dials turn; the work card's windows stand in one line under it.
  Rows.Add(T::Panel(FString(),S47Kit::Col({T::TitleRow(TEXT("FINE TUNING / SPECTRUM ZOOM (MHz)"),FString::Printf(TEXT("TARGET %.3f +/- %.3f  |  GAIN %.0f-%.0f  |  BW %.0f-%.0f kHz"),P.F,P.FTol,P.Gain[0],P.Gain[1],P.Bw[0],P.Bw[1])),
   T::TuneZoom([this]{return M->Story->Rx;},[this]{return EffectiveProfile(*CurrentJob());},Quality,170)})));
  Rows.Add(Banner());
  Rows.Add(Controls());
  // D45-20261110 (Tom: record the control at the receiver right after the take): the same key records an unusual take's control.
  auto Start=T::Big(TAttribute<FString>::CreateLambda([this]
  {
   const auto* J=CurrentJob();if(!J)return FString(TEXT("START RECORDING"));
   if(J->Stage==EJobStage::Integrating)return FString(TEXT("RECORDING..."));
   if(J->bControl&&J->ControlAt<8&&J->Phenomenon!=TEXT("wrong_source"))return PlainAnomaly(*J)?FString::Printf(TEXT("REFERENCE %.1f / 8 s"),J->ControlAt):FString::Printf(TEXT("CONTROL %.1f / 8 s"),J->ControlAt);
   if(CanRecordControl())return FString(TEXT("RECORD CONTROL / 8 s"));
   if(RetainWaits(*J))return FString(TEXT("RETAIN: RECORD TAB")); // RX-ANOMALY-FLOW-20261113: the take is retained on RECORD
   if(PlainAnomaly(*J)&&J->Stage>=EJobStage::Chart&&J->Stage<=EJobStage::Recorded&&!J->bAudioReviewed)return FString(TEXT("ON TAPE / REEL DECK")); // GAMEPLAY-LOOP-01
   return J->Stage>=EJobStage::Chart?FString(TEXT("RECORDING RETAINED")):FString(TEXT("START RECORDING"));
  }),[this]{if(CanRecordControl())StartControl();else StartAcquisition();},1,TAttribute<bool>::CreateLambda([this]{return ReceiverReady()||CanRecordControl();}));
  Start=T::DueRing(Start,[this]{return (ReceiverReady()&&!AnomalyAhead())||CanRecordControl();}); // RX-TABS-DUE-20261113: the key the TUNE page is due for (RX-ANOMALY-FLOW-20261113: an anomaly is RECORD's)
  Rows.Add(T::Split({
   {1.15f,T::Readout(TEXT("CARRIER LOCK"),TAttribute<FString>::CreateLambda([Quality]{const int32 Percent=S47Signal::CarrierPercent(Quality());return FString::Printf(TEXT("%d %% %s"),Percent,Percent>=100?TEXT("LOCKED"):Percent>=70?TEXT("CLOSE"):TEXT("SEARCHING"));}))},
   {.9f,T::Readout(TEXT("OFF TARGET"),TAttribute<FString>::CreateLambda([this]{const auto* J=CurrentJob();return J?FString::Printf(TEXT("%+.3f MHz"),M->Story->Rx.Frequency-EffectiveProfile(*J).F):FString();}))},
   {.85f,T::Readout(TEXT("RECORDING"),TAttribute<FString>::CreateLambda([this]{const auto* J=CurrentJob();return FString::Printf(TEXT("%.0f / %.0f s"),J?J->Elapsed:0.,SourceSeconds());}))},
   {1.1f,Start}}));
  // D45-20261110 (Tom: cooler animations when you record): the receiver's own tape, seen while the take or its control is written and
  // while an unusual take waits for its control: REC lamp, reels, needles and the pen. Collapsed otherwise; an idle page stays still.
  auto OnControl=[this]{const auto* J=CurrentJob();return J&&J->bControl&&J->Phenomenon!=TEXT("wrong_source");};
  auto Writing=[this,OnControl]{const auto* J=CurrentJob();return J&&(J->Stage==EJobStage::Integrating||(OnControl()&&J->ControlAt<8));};
  auto Rolling=[this,Writing]{const auto* J=CurrentJob();if(!J||!Writing())return false;return J->Stage!=EJobStage::Integrating||(PointReady()&&Settle>=1&&M&&M->Story&&M->Story->Rx.Pass(EffectiveProfile(*J)));};
  auto TapeAt=[this,OnControl]{const auto* J=CurrentJob();return !J?0.:OnControl()?J->ControlAt:J->Elapsed;};
  auto TapeSpan=[this,OnControl]{return OnControl()?8.:SourceSeconds();};
  auto TapeTicks=[this,OnControl]{const auto* J=CurrentJob();return J&&!OnControl()?J->TraceTicks:TArray<double>();};
  auto TapeKey=[this,OnControl]{const auto* J=CurrentJob();return J?TakeSeed(*J,OnControl()?1:0):0u;};
  auto TapePulse=[this]{const auto* J=CurrentJob();return J&&J->Source==1?PulsarPeriod:0.;};
  auto TapeName=[this,OnControl]{const auto* J=CurrentJob();return !J?FString():OnControl()?FString(TEXT("CONTROL / ANTENNAS OUT, THE STATION'S OWN REFERENCE")):TEXT("TAKE ")+J->Id+TEXT(" / WHAT THE ANTENNAS HEAR");};
  const TAttribute<EVisibility> StripShown=TAttribute<EVisibility>::CreateLambda([this,Writing]{const auto* J=CurrentJob();return J&&(Writing()||(J->bAnomaly&&!Specialized(J->Phenomenon)&&(J->Stage==EJobStage::Chart||J->Stage==EJobStage::Recorded)))?EVisibility::Visible:EVisibility::Collapsed;});
  Rows.Add(SNew(SBox).Visibility(StripShown).HeightOverride(78)[T::Columns({ // D45: 120 ran 37 over the glass (InternalControlEarly)
   {1.f,SNew(SS47TapeReels).Phosphor(true).Recording_Lambda(Writing).Playing_Lambda(Rolling).Control_Lambda(OnControl).Cursor_Lambda(TapeAt).Length_Lambda(TapeSpan).Ticks_Lambda(TapeTicks).Seed_Lambda(TapeKey).Period_Lambda(TapePulse)},
   {1.35f,SNew(SS47TapeScope).Phosphor(true).Recording(true).Playing_Lambda(Rolling).Control_Lambda(OnControl).Cursor_Lambda(TapeAt).Length_Lambda(TapeSpan).Ticks_Lambda(TapeTicks).Seed_Lambda(TapeKey).Period_Lambda(TapePulse).Caption_Lambda(TapeName)}},10)]);
 }
 else if(ReceiverTab==2)
 {
  // D48-20261110 (D46-14): the 27 antennas as the status PC draws them (T::AntennaGrid), here live for this job: lit steady where an
  // antenna is on remote and on the target, pulsing while it slews, red where it is off remote.
  const int32 Dishes=M->Web->DishCount();
  auto DishOff=[this](int32 I){return !M->Web->DishRemoteAvailable(I);};
  auto DishState=[this,DishOff](int32 I){if(DishOff(I))return 2;return FMath::Abs(FMath::FindDeltaAngleDegrees(M->Web->DishAz(I),TargetAz()))>.1||FMath::Abs(M->Web->DishEl(I)-TargetEl())>.1?3:0;};
  auto Short=[this](int32 I){return M->Web->DishId(I).Replace(TEXT("-"),TEXT(""));};
  auto Slew=[this]{return PointReady()?1.f:float(FMath::Clamp(1.-ArrayErrorDeg()/90.,0.,1.));};
  Rows.Add(T::Panel(FString(),S47Kit::Col({T::TitleRow(TEXT("ARRAY / ANTENNAS ON THIS TARGET"),FString::Printf(TEXT("%s  |  AZ %.1f EL %.1f"),*Name,TargetAz(),TargetEl())),T::AntennaGrid(Dishes,Short,DishState,nullptr)})));
  Rows.Add(T::Columns({
   {1.3f,T::Meter(TEXT("SLEW PROGRESS"),TAttribute<FString>::CreateLambda([this]{return PointReady()?FString(Settle>=1?TEXT("ON TARGET / STEADY"):TEXT("ON TARGET / SETTLING")):FString::Printf(TEXT("%.1f\u00b0 TO GO"),ArrayErrorDeg());}),TAttribute<float>::CreateLambda(Slew))},
   {1.f,T::Value(TEXT("POINTING / OFF"),TAttribute<FString>::CreateLambda([this,Dishes,DishState]{int32 On=0,Off=0;for(int32 I=0;I<Dishes;++I){const int32 St=DishState(I);On+=St==0;Off+=St==2;}return FString::Printf(TEXT("%d / %d"),On,Off);}),true)}},10));
  Rows.Add(T::Small(TEXT("Steady = on remote and on target. Pulsing = still moving. Red = off remote. Antennas are commanded from the right-hand tracking PC; this receiver records only while pointing, tuning and power are all valid.")));
  Rows.Add(Banner());
 }
 else if(ReceiverTab==3)
 {
  const FJob* J=CurrentJob();
  const bool ControlPanel=J->bAnomaly&&!PlainAnomaly(*J)&&J->Stage==EJobStage::Recorded; // GAMEPLAY-LOOP-01: the extra-tick take asks for no control
  // D48-20261110 (D46-14): the tape strip of the last take, the levels it was written at and the take list. The job's own facts stand in two
  // columns. While an unusual take waits for its control the page keeps to the control (the strip and the job), as it always did.
  const bool OnCtl=J->bControl&&J->Phenomenon!=TEXT("wrong_source");
  auto TapeAt=[this]{const auto* X=CurrentJob();return !X?0.:X->Stage>=EJobStage::Chart||X->Stage==EJobStage::Integrating?X->Elapsed:0.;};
  auto TapeKey=[this]{const auto* X=CurrentJob();return X?TakeSeed(*X,0):0u;};
  auto TapePulse=[this]{const auto* X=CurrentJob();return X&&X->Source==1?PulsarPeriod:0.;};
  // RX-ANOMALY-FLOW-20261113: the strip writes the take while it records here (the anomaly is recorded on this page)
  auto Writing=[this]{const auto* X=CurrentJob();return X&&X->Stage==EJobStage::Integrating;};
  auto Rolling=[this]{const auto* X=CurrentJob();return X&&X->Stage==EJobStage::Integrating&&PointReady()&&Settle>=1&&M&&M->Story&&M->Story->Rx.Pass(EffectiveProfile(*X));};
  Rows.Add(SNew(SBox).HeightOverride(78)[T::Columns({
   {1.f,SNew(SS47TapeReels).Phosphor(true).Recording_Lambda(Writing).Playing_Lambda(Rolling).Control(OnCtl).Cursor_Lambda(TapeAt).Length_Lambda([this]{return SourceSeconds();}).Ticks_Lambda([this]{const auto* X=CurrentJob();return X?X->TraceTicks:TArray<double>();}).Seed_Lambda(TapeKey).Period_Lambda(TapePulse)},
   {1.35f,SNew(SS47TapeScope).Phosphor(true).Recording_Lambda(Writing).Playing_Lambda(Rolling).Control(OnCtl).Cursor_Lambda(TapeAt).Length_Lambda([this]{return SourceSeconds();}).Ticks_Lambda([this]{const auto* X=CurrentJob();return X?X->TraceTicks:TArray<double>();}).Seed_Lambda(TapeKey).Period_Lambda(TapePulse).Caption_Lambda([this]{const auto* X=CurrentJob();return X?FString(TEXT("TAKE "))+X->Id+TEXT(" / ON TAPE"):FString();})}},10)]);
  // RX-ANOMALY-FLOW-20261113 (Tom 13 November: from the receiver's RECORD tab to the sound confirmed at the reel deck, with a reference):
  // from ANOMALY DETECTED until it is retained, this page holds the extra-tick take: START RECORDING, the take written on the strip,
  // then RETAIN TAKE, which keeps the original page and records the 8-second station reference. It stands where the levels and the
  // take list stand otherwise, so the page keeps its height.
  auto AnomalyHere=[this]{return RecordTabHoldsAnomaly();};
  if(!ControlPanel)
  {
   auto Key=T::Big(TAttribute<FString>::CreateLambda([this]
   {
    const auto* X=CurrentJob();if(!X)return FString();
    if(X->Stage==EJobStage::Pointed)return FString(TEXT("START RECORDING"));
    if(X->Stage==EJobStage::Integrating)return FString::Printf(TEXT("RECORDING %.0f / %.0f s"),X->Elapsed,SourceSeconds());
    if(RetainWaits(*X))return FString(TEXT("RETAIN TAKE / RECORD REFERENCE 8 s"));
    if(X->bControl&&X->ControlAt<8)return FString::Printf(TEXT("REFERENCE %.1f / 8 s"),X->ControlAt);
    return FString(TEXT("RETAINED / REEL DECK NEXT"));
   }),[this]{const auto* X=CurrentJob();if(X&&X->Stage==EJobStage::Pointed){if(!StartAcquisition())Changed(Feedback);}else if(!RetainTake())Changed(Feedback);},1,
   TAttribute<bool>::CreateLambda([this]{const auto* X=CurrentJob();return X&&((X->Stage==EJobStage::Pointed&&ReceiverReady())||RetainWaits(*X));}));
   Key=T::DueRing(Key,[this]{const auto* X=CurrentJob();return X&&((X->Stage==EJobStage::Pointed&&ReceiverReady()&&AnomalyAhead())||RetainWaits(*X));});
   auto TakeState=[this]{const auto* X=CurrentJob();if(!X)return FString();
    if(X->Stage<=EJobStage::Pointed)return FString(TEXT("NOT RECORDED"));
    if(X->Stage==EJobStage::Integrating)return FString::Printf(TEXT("RECORDING %.0f / %.0f s"),X->Elapsed,SourceSeconds());
    return FString(X->bPrinted?TEXT("RETAINED / IN NOTEBOOK"):TEXT("ON TAPE / NOT RETAINED"));};
   auto RefState=[this]{const auto* X=CurrentJob();if(!X||!X->bControl)return FString(TEXT("NOT RECORDED"));return X->ControlAt<8?FString::Printf(TEXT("RECORDING %.1f / 8 s"),X->ControlAt):FString(TEXT("ON TAPE"));};
   Rows.Add(SNew(SBox).Visibility_Lambda([AnomalyHere]{return AnomalyHere()?EVisibility::Visible:EVisibility::Collapsed;})[T::Panel(TEXT("ANOMALY / RECORD IT, THEN RETAIN IT"),S47Kit::Col({
    T::Small(TEXT("Record the take here. RETAIN TAKE keeps its page in your notebook and records 8 s of the station's own reference, antennas out. The reel deck plays both.")),
    T::Columns({{1.f,T::Value(TEXT("TAKE"),TAttribute<FString>::CreateLambda(TakeState),true)},{1.f,T::Value(TEXT("REFERENCE"),TAttribute<FString>::CreateLambda(RefState),true)}},10),
    Key}))]);
  }
  if(!ControlPanel)
  {
   TArray<T::FRow> Takes;
   for(int32 I=Work.Num()-1;I>=0&&Takes.Num()<4;--I)
   {
    const FJob& X=Work[I];
    Takes.Add({{X.Id,FString(FS47Observing::Runs()[X.Source].Source).Left(10),StageText(X.Stage)},X.Id==J->Id,nullptr,X.bAnomaly?FString(TEXT("ODD")):FString()});
   }
   Rows.Add(SNew(SBox).Visibility_Lambda([AnomalyHere]{return AnomalyHere()?EVisibility::Collapsed:EVisibility::Visible;})[T::Columns({
    {.9f,T::Panel(FString(),S47Kit::Col({T::TitleRow(TEXT("LEVELS"),FString()),
      T::Meter(TEXT("SIGNAL LEVEL"),TAttribute<FString>::CreateLambda([this]{const auto* X=CurrentJob();return FString::Printf(TEXT("%.0f %%"),100*FMath::Clamp(X?X->ReceivedLevel:0.,0.,1.));}),TAttribute<float>::CreateLambda([this]{const auto* X=CurrentJob();return float(FMath::Clamp(X?X->ReceivedLevel:0.,0.,1.));})),
      T::Meter(TEXT("CARRIER LOCK"),TAttribute<FString>::CreateLambda([Quality]{return FString::Printf(TEXT("%d %%"),S47Signal::CarrierPercent(Quality()));}),TAttribute<float>::CreateLambda([Quality]{return float(Quality());})),
      T::Meter(TEXT("TAPE USED"),TAttribute<FString>::CreateLambda([this]{const auto* X=CurrentJob();return FString::Printf(TEXT("%.0f / %.0f s"),X?X->Elapsed:0.,SourceSeconds());}),TAttribute<float>::CreateLambda([this]{const auto* X=CurrentJob();return X?float(FMath::Clamp(X->Elapsed/FMath::Max(1.,SourceSeconds()),0.,1.)):0.f;}))}))},
    {1.1f,T::Panel(FString(),S47Kit::Col({T::TitleRow(TEXT("TAKE LIST"),FString()),T::Table({TEXT("TAKE"),TEXT("SOURCE"),TEXT("STAGE")},{.8f,1.f,1.4f},Takes)}))}},10)]);
  }
  TArray<S47Kit::FW> JobRows={
   T::Columns({{1.f,T::Value(TEXT("JOB"),J->Id,true)},{1.4f,T::Value(TEXT("STAGE"),TAttribute<FString>::CreateLambda([this]{return CurrentJob()?StageText(CurrentJob()->Stage):FString();}),true)}},10),
   T::Columns({{1.f,T::Value(TEXT("RECORDED"),TAttribute<FString>::CreateLambda([this]{const auto* X=CurrentJob();return FString::Printf(TEXT("%.1f / %.0f s"),X?X->Elapsed:0.,SourceSeconds());}),true)},
    {1.4f,T::Value(TEXT("ORIGINAL"),TAttribute<FString>::CreateLambda([this]{const auto* X=CurrentJob();if(!X)return FString(); // RX-ANOMALY-FLOW-20261113: live, it changes on this page
     return X->bPrinted?FString(PlainAnomaly(*X)?TEXT("KEPT / IN NOTEBOOK"):TEXT("COLLECTED / IN NOTEBOOK")):RetainWaits(*X)?FString(TEXT("ON TAPE / RETAIN IT HERE")):X->Stage==EJobStage::Chart?FString(TEXT("WAITING AT PRINTER")):FString(TEXT("NOT YET RECORDED"));}),true)}},10)};
  if(!ControlPanel)JobRows.Add(SNew(SBox).Visibility_Lambda([AnomalyHere]{return AnomalyHere()?EVisibility::Collapsed:EVisibility::Visible;})[T::Small(TEXT("Originals print at the table by the service-yard door. Valid recorded seconds are retained if tuning or pointing is lost."))]);
  Rows.Add(T::Panel(FString(),S47Kit::Col(JobRows)));
  if(ControlPanel)
  {
   const bool Beam=J->Phenomenon==TEXT("wrong_source");
   const FString Why=Beam
    ?TEXT("The chart may not belong to this source. Short samples west, east and back on centre show whether the array was really hearing it here. The original is not changed.")
    :TEXT("Something in the original does not match the work card. The control takes the antennas out of the path and records eight seconds of the station's own reference signal, kept apart from the original. If the oddity is in the control too, the equipment is making it. If not, it came from outside and stays unexplained.");
   const FString After=Beam?TEXT("Start it from the right-hand tracking PC. Then compare at OPS, DAILY WORK.")
    :J->Phenomenon==TEXT("time_disagreement")?TEXT("It runs by itself. Then compare the clock readings at OPS, DAILY WORK.")
    :J->Phenomenon==TEXT("duplicate_print")?TEXT("It runs by itself. The printer then feeds a separate copy to compare at OPS, DAILY WORK.")
    :TEXT("It runs by itself. Then listen to both recordings at the reel deck on the wall table.");
   Rows.Add(T::Panel(TEXT("INTERNAL CONTROL / THE SKY OR THE STATION?"),S47Kit::Col({
    T::Small(Why),
    T::Value(TEXT("CONTROL"),TAttribute<FString>::CreateLambda([this]{const auto* X=CurrentJob();return !X||!X->bControl?FString(TEXT("NOT RECORDED")):X->ControlAt<8?FString::Printf(TEXT("RUNNING %.1f / 8 s"),X->ControlAt):FString(TEXT("COMPLETE"));}),true),
    T::Small(After)})));
   if(!Beam&&!J->bControl)Rows.Add(T::DueRing(T::Big(FString(TEXT("RECORD INTERNAL CONTROL / 8 SECONDS")),[this]{if(StartControl())OpenReceiver();else Changed(NextAction());},1,true),[this]{return CanRecordControl();})); // RX-TABS-DUE-20261113
  }
  if(!ControlPanel||J->bControl||J->Phenomenon==TEXT("wrong_source"))Rows.Add(Banner());
 }
 else
 {
  // D48-20261110 (D46-14, D46-16): the last take as a spectrogram beside a known source to compare it with. The reference is chosen here and
  // heard at the reel deck; MATCH says the take sounds like it. A man-made source in the take's hash shows in its picture; an unusual take
  // shows none of them. The work card stands beside them.
  DataRef=FMath::Clamp(DataRef,0,FS47ReceiverSound::Count()-1);
  auto HasTake=[this]{const auto* J=CurrentJob();return J&&J->Elapsed>0;};
  auto TakeLook=[this](float At,float Pitch)->float
  {
   const auto* J=CurrentJob();if(!J)return 0.f;
   if(At*SourceSeconds()>J->Elapsed)return 0.f; // not recorded yet
   float V=FS47ReceiverSound::Signature(J->Source==1?6:5,At,Pitch);
   const bool bOdd=J->bAnomaly&&!J->TraceTicks.IsEmpty();
   if(bOdd)
   {
    // The hidden cause of the first case leaves its mark under the take: the feeder's lines or a local transmitter's sweeps. The sky leaves none.
    const auto* C=Cases.Get(0);const int32 Cause=C?C->Truth:2;
    if(Cause==0)V=FMath::Max(V,.6f*FS47ReceiverSound::Signature(8,At,Pitch));
    else if(Cause==1)V=FMath::Max(V,.6f*FS47ReceiverSound::Signature(TakeSeed(*J,0)%2?9:7,At,Pitch));
    for(const double Tick:J->TraceTicks)if(FMath::Abs(At*SourceSeconds()-Tick)<.5*SourceSeconds()/56.)V=FMath::Max(V,.95f);
   }
   return V;
  };
  const int32 Refs=FS47ReceiverSound::Count();
  auto RefLook=[this](float At,float Pitch){return FS47ReceiverSound::Signature(DataRef,At,Pitch);};
  auto RefShown=[this]{return FString::Printf(TEXT("%s / %s"),FS47ReceiverSound::NameOf(DataRef),FS47ReceiverSound::KindOf(DataRef)==1?TEXT("MAN-MADE"):TEXT("NATURAL"));};
  auto Odd=[this]{const auto* J=CurrentJob();return J&&J->bAnomaly&&!J->TraceTicks.IsEmpty()&&J->Stage>=EJobStage::Chart;};
  const TArray<S47Kit::FW> Left={T::Paper(JobLine(*CurrentJob())),T::Paper(P.Card)};
  Rows.Add(T::Columns({
   {.8f,T::Panel(TEXT("WORK CARD"),S47Kit::Col(Left))},
   {1.2f,T::Panel(FString(),S47Kit::Col({
    T::TitleRow(TEXT("LAST TAKE / FREQUENCY OVER TIME"),FString::Printf(TEXT("%.0f s"),SourceSeconds())),T::Spectrogram(TakeLook,HasTake,150),
    T::TitleRow(TEXT("KNOWN SOURCE FOR COMPARISON"),TAttribute<FString>::CreateLambda(RefShown)),T::Spectrogram(RefLook,[]{return true;},110)}))}},10));
  Rows.Add(T::Note(TAttribute<FString>::CreateLambda([this]{return FString(FS47ReceiverSound::BlurbOf(DataRef));}),20,TAttribute<FLinearColor>(S47Kit::Css(0x8cffa4))));
  Rows.Add(T::Columns({
   {1.f,T::Button(TEXT("< PREVIOUS"),[this,Refs]{DataRef=(DataRef+Refs-1)%Refs;OpenReceiver();})},
   {1.f,T::Button(TEXT("NEXT >"),[this,Refs]{DataRef=(DataRef+1)%Refs;OpenReceiver();})},
   {1.6f,T::Big(TAttribute<FString>::CreateLambda([this,Odd]{const auto* C=Cases.Get(0);return Odd()?FString::Printf(TEXT("THE TAKE SOUNDS LIKE THIS / %d LEFT"),C?Cases.RefTriesLeft(0):0):FString(TEXT("MATCH: AN UNUSUAL TAKE IS NEEDED"));}),[this]{MarkReference(DataRef);OpenReceiver();},1,TAttribute<bool>::CreateLambda(Odd))}},10));
  Rows.Add(T::Small(TEXT("Listen to each source at the reel deck on the wall table (NEXT REFERENCE, PLAY REFERENCE). A match that is right rules an explanation out on the OPS case board; a take of something new matches none.")));
 }
 OpenTerminal(1,TEXT("RECEIVER / ACQUISITION"),S47Kit::Col(Rows),true,SkyKeys);
}
// D48-20261110 (D46-16): the take against a known source. Only an unusual pulsar take with its case open can be compared; the case logic
// (FS47FreeformCases::MarkReference) decides, so a wrong or a natural sound, or a take of the sky, answers the same.
void FS47FreeformSession::MarkReference(int32 Reference)
{
 const FJob* J=CurrentJob();if(!J||!M||Reference<0||Reference>=FS47ReceiverSound::Count())return;
 const FString Name=FString(FS47ReceiverSound::NameOf(Reference));
 if(!J->bAnomaly||J->TraceTicks.IsEmpty()||J->Stage<EJobStage::Chart){Changed(TEXT("Nothing unusual is on tape to compare yet."));return;}
 const auto* C=Cases.Get(0);
 if(!C||!C->bOpen||C->bConcluded){Changed(TEXT("No question is open on this take. Register it at OPS first and the case board opens one."));return;}
 switch(Cases.MarkReference(0,FS47ReceiverSound::FeederCause(Reference)))
 {
 case FS47FreeformCases::EMatch::Matched:
  Changed(Name+TEXT(" is under this take. Something on the ground makes that sound, so the extra ticks do not follow the sky. See the OPS case board."));break;
 case FS47FreeformCases::EMatch::NoMatch:
  Changed(Name+TEXT(" does not match this take.")+(Cases.RefTriesLeft(0)>0?FString::Printf(TEXT(" %d comparison%s left."),Cases.RefTriesLeft(0),Cases.RefTriesLeft(0)==1?TEXT(""):TEXT("s")):FString(TEXT(" No more comparisons on this take; the tests on the case board settle it."))));break;
 default:
  Changed(TEXT("No more comparisons on this take. The tests on the case board settle it."));break;
 }
}
bool FS47FreeformSession::TapeWaits(const FJob& J)
{
 return PlainAnomaly(J)&&J.Stage>=EJobStage::Chart&&J.Stage<EJobStage::Registered&&!J.bAudioReviewed&&!RetainWaits(J); // RX-ANOMALY-FLOW-20261113: retained first
}
void FS47FreeformSession::FollowTapeJob()
{
 // Critic 12 Nov: the deck plays the selected job's take. Another job chosen after the take, the deck would have nothing to play for
 // the anomaly: it threads the oldest take still waiting for its one listen instead (the selected one if it is one).
 const FJob* Now=CurrentJob();
 if(Now&&TapeWaits(*Now))return;
 int32 Oldest=INDEX_NONE;
 for(int32 I=0;I<Work.Num();++I)if(TapeWaits(Work[I])&&(Oldest==INDEX_NONE||Work[I].Day<Work[Oldest].Day))Oldest=I;
 if(Oldest!=INDEX_NONE)SelectJob(Oldest);
}
void FS47FreeformSession::OpenAudio()
{
 if(!RequirePower()||!AudioDesk||!AudioDesk->Ready())return;
 if(!AudioDesk->ListeningPosition()){if(M&&M->UI)M->UI->Toast(TEXT("Recordings are compared at the reel deck on the wall table."),5);return;}
 FollowTapeJob();
 bAudioOpen=true;
}
void FS47FreeformSession::OpenTapePanel()
{
 namespace T=S47FreeformTerminal;
 if(!M||!M->UI||!RequirePower()||!AudioDesk||!AudioDesk->Ready())return;
 if(!AudioDesk->ListeningPosition()){OpenAudio();return;}
 FollowTapeJob();
 const T::FReelLook ReelLook;
 // FIX-20261110: this is a machine, not a terminal: its keys are heavy and nothing beeps (the motor is heard in TransportSound)
 {const TWeakObjectPtr<US47Main> Main=M;T::SetSound([Main](int32 Kind){if(Main.IsValid()&&Main->Audio&&Main->Freeform&&Main->Freeform->TapePanelOpen()&&Kind<3)Main->Audio->Play(Kind==1?TEXT("rack_switch"):TEXT("small_switch"),Kind==1?.42f:.38f,FVector::ZeroVector,Kind==2?1.1f:.95f);});}
 auto Press=[this](const TCHAR* Action){return [this,Action]{if(AudioDesk)AudioDesk->Use(FString(TEXT("ff:audio:"))+Action);};};
 // GAMEPLAY-LOOP-01 (Tom, 12 November): the extra-tick take is on the deck once it is retained at the receiver (RX-ANOMALY-FLOW-
 // 20261113), with its 8-second reference on the REFERENCE key; heard past its ticks and marked, RETAIN (the operator's press) confirms it.
 auto CanCompare=TAttribute<bool>::CreateLambda([this]{const auto* J=CurrentJob();return J&&PlainAnomaly(*J)&&J->Stage>=EJobStage::Chart;});
 // RX-ANOMALY-FLOW-20261113 (Tom: MARK IN, MARK OUT and a click on the trace must work): the trace takes a click whenever the take
 // could be marked, and the keys are live whenever the take is on the deck; what cannot be done is said (MarkAudioAt, AudioAction),
 // not left as a key that does nothing.
 auto Marking=TAttribute<bool>::CreateLambda([this]{const auto* J=CurrentJob();return J&&PlainAnomaly(*J)&&J->Stage>=EJobStage::Chart&&J->Stage<EJobStage::Registered&&!J->bAudioReviewed;});
 auto Pick=[this](int32 Index){return TAttribute<bool>::CreateLambda([this,Index]{return AudioSourceIndex==Index;});};
 TArray<S47Kit::FW> Rows;
 auto Ready=[this]{const auto* J=CurrentJob();return J&&PlainAnomaly(*J)&&J->Stage>=EJobStage::Chart&&!RetainWaits(*J);};
 // D45-20261110 (Tom: a 1980s high-tech deck that fits one screen without scrolling): one fixed faceplate, every row its own height.
 // A: the four steps as lamps (amber now, green done, dark to come), or the one line that says why there is nothing to compare yet.
 const FLinearColor LampOff=S47Kit::Css(0x3a3e2b),LampOn=S47Kit::Css(0xf0b040),LampDone=S47Kit::Css(0xa4d890),Cream=S47Kit::Css(0xf1e8cf);
 // RX-ANOMALY-FLOW-20261113: the take, its reference (when it has one; optional to hear), the marks and RETAIN
 auto StepNow=[this]
 {
  const auto* J=CurrentJob();if(!J)return 0;if(J->Stage>=EJobStage::Registered||J->bAudioReviewed)return 4;
  if(!TapeHeard(*J))return 1;
  if(J->bControl&&J->ControlAt>=8&&J->ControlCovered<8)return 2;
  return 3;
 };
 auto StepCell=[StepNow,LampOff,LampOn,LampDone,Cream](int32 N,TFunction<FString()> Says)
 {
  const auto Tone=[StepNow,N,LampOff,LampOn,LampDone]{const int32 Current=StepNow();return N<Current?LampDone:N==Current?LampOn:LampOff;};
  return S47Kit::FW(SNew(SHorizontalBox)
   +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)[T::Lamp(TAttribute<FLinearColor>::CreateLambda(Tone),14)]
   +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[T::Note(TAttribute<FString>::CreateLambda(Says),16,TAttribute<FLinearColor>::CreateLambda([Tone,LampOff,Cream]{const FLinearColor C=Tone();return C==LampOff?Cream*FLinearColor(1,1,1,.6f):C;}),false)]);
 };
 const S47Kit::FW Lamps=T::Columns({
  {1.25f,StepCell(1,[this]{const auto* J=CurrentJob();return FString::Printf(TEXT("1 PLAY THE TAKE %02.0f/18"),J?FMath::Min(18.,J->OriginalCovered):0.);})},
  {1.25f,StepCell(2,[this]{const auto* J=CurrentJob();if(!J||!J->bControl)return FString(TEXT("2 NO REFERENCE TAPE"));return J->ControlAt<8?FString::Printf(TEXT("2 REFERENCE RECORDING %.0f/8"),J->ControlAt):FString::Printf(TEXT("2 PLAY THE REFERENCE %01.0f/8"),FMath::Min(8.,J->ControlCovered));})},
  {1.3f,StepCell(3,[this]{const auto* J=CurrentJob();return FString(J&&J->Stage>=EJobStage::Registered?TEXT("3 CONFIRMED AT OPS"):J&&J->bAudioReviewed?TEXT("3 RETAINED: TO OPS"):TEXT("3 MARK THE TICKS, RETAIN"));})}});
 const S47Kit::FW Why=T::Note(TAttribute<FString>::CreateLambda([this]
 {
  const auto* J=CurrentJob();
  if(!J||!J->bAnomaly||J->Stage<EJobStage::Chart)return FString(TEXT("Nothing unusual on tape yet. An unusual take is recorded and retained at the receiver (centre PC) first."));
  if(RetainWaits(*J))return FString(TEXT("Not retained yet: RECEIVER (centre PC), RECORD tab, RETAIN TAKE. Then it plays here."));
  if(Specialized(J->Phenomenon))return FString(TEXT("This observation is compared at OPS (corner PC), DAILY WORK, not on tape."));
  return FString();
 }),20,TAttribute<FLinearColor>(LampOn));
 Rows.Add(SNew(SBox).HeightOverride(46).VAlign(VAlign_Center)[SNew(SOverlay)
  +SOverlay::Slot().VAlign(VAlign_Center)[SNew(SBox).Visibility_Lambda([Ready]{return Ready()?EVisibility::Visible:EVisibility::Collapsed;})[Lamps]]
  +SOverlay::Slot().VAlign(VAlign_Center)[SNew(SBox).Visibility_Lambda([Ready]{return Ready()?EVisibility::Collapsed:EVisibility::Visible;})[Why]]]);
 // B: the transport, the take's label on it, the counter's window with the memory marks; C: the tape head's trace and the band display.
 auto TakeKey=[this]{const auto* J=CurrentJob();return J?TakeSeed(*J,AudioSourceIndex):0u;};
 auto Kept=[this]{const auto* J=CurrentJob();return J&&J->bAudioReviewed;};
 auto Pulse=[this]{const auto* J=CurrentJob();return J&&J->Source==1?PulsarPeriod:0.;};
 Rows.Add(SNew(SBox).HeightOverride(200)[SNew(SS47TapeReels)
  .Cursor_Lambda([this]{return AudioCursor;}).Length_Lambda([this]{return AudioSourceIndex==0?18.:8.;})
  .Playing_Lambda([this]{return bAudioPlaying&&!CaptureStartStage;}).Loading_Lambda([this]{return CaptureStartStage!=0;})
  .Control_Lambda([this]{return AudioSourceIndex!=0;})
  .Ticks_Lambda([this]{const auto* J=CurrentJob();return J&&AudioSourceIndex==0?J->TraceTicks:TArray<double>();})
  .Seed_Lambda(TakeKey).Retained_Lambda(Kept).Period_Lambda(Pulse)
  .MarkIn_Lambda([this]{const auto* J=CurrentJob();return J?J->MarkStart:-1.;}).MarkOut_Lambda([this]{const auto* J=CurrentJob();return J?J->MarkEnd:-1.;})
  .Caption_Lambda([this]{const auto* J=CurrentJob();return !J?FString():FString::Printf(TEXT("TAKE %s / %s"),*J->Id,AudioSourceIndex==0?TEXT("ORIGINAL 18 s"):TEXT("CONTROL 8 s"));})]);
 const auto Scope=SNew(SS47TapeScope)
  .Cursor_Lambda([this]{return AudioCursor;}).Length_Lambda([this]{return AudioSourceIndex==0?18.:8.;})
  .Control_Lambda([this]{return AudioSourceIndex!=0;}).Playing_Lambda([this]{return bAudioPlaying&&!CaptureStartStage;})
  .MarkIn_Lambda([this]{const auto* J=CurrentJob();return J&&AudioSourceIndex==0?J->MarkStart:-1.;}).MarkOut_Lambda([this]{const auto* J=CurrentJob();return J&&AudioSourceIndex==0?J->MarkEnd:-1.;})
  .Ticks_Lambda([this]{const auto* J=CurrentJob();return J&&AudioSourceIndex==0?J->TraceTicks:TArray<double>();})
  .Seed_Lambda(TakeKey).Retained_Lambda(Kept).Period_Lambda(Pulse)
  .CanMark(Marking).OnMark([this](double At){MarkAudioAt(At);});
 Rows.Add(SNew(SBox).HeightOverride(240)[Scope]);
 // D: one row of transport keys with a lamp over each (lit: the tape loaded, playing, a mark set, the comparison retained).
 auto Keyed=[LampOff,LampOn](TFunction<bool()> On,S47Kit::FW Key)
 {
  return S47Kit::FW(SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0,0,0,5)[T::Lamp(TAttribute<FLinearColor>::CreateLambda([On,LampOff,LampOn]{return On&&On()?LampOn:LampOff;}),10)]
   +SVerticalBox::Slot().AutoHeight()[Key]);
 };
 Rows.Add(SNew(SBox).HeightOverride(76)[T::Columns({
  {1.5f,Keyed([this]{return AudioSourceIndex==0;},T::Toggle(TEXT("ORIGINAL 18 s"),Pick(0),Press(TEXT("original")),CanCompare))},
  {1.5f,Keyed([this]{return AudioSourceIndex==1;},T::Toggle(TEXT("REFERENCE 8 s"),Pick(1),Press(TEXT("control")),CanCompare))},
  {1.f,Keyed(nullptr,T::Button(TEXT("REWIND"),Press(TEXT("rewind")),false,CanCompare))},
  {1.f,Keyed([this]{return bAudioPlaying||CaptureStartStage!=0;},T::ButtonLive(TAttribute<FString>::CreateLambda([this]{return FString(bAudioPlaying||CaptureStartStage?TEXT("STOP"):TEXT("PLAY"));}),Press(TEXT("play")),true,CanCompare))},
  {1.05f,Keyed([this]{const auto* J=CurrentJob();return J&&J->MarkStart>=0;},T::Button(TEXT("MARK IN"),Press(TEXT("mark_start")),false,CanCompare))},
  {1.1f,Keyed([this]{const auto* J=CurrentJob();return J&&J->MarkEnd>=0;},T::Button(TEXT("MARK OUT"),Press(TEXT("mark_end")),false,CanCompare))},
  {1.1f,Keyed([this]{const auto* J=CurrentJob();return J&&J->bAudioReviewed;},T::Filled(TEXT("RETAIN"),Press(TEXT("retain")),TAttribute<bool>::CreateLambda([this]{const auto* J=CurrentJob();return J&&PlainAnomaly(*J)&&J->Stage>=EJobStage::Chart&&J->Stage<EJobStage::Registered&&!J->bAudioReviewed;})))}},10)]);
 // E: what the mark covers and what the deck last said, in two lines at most.
 Rows.Add(SNew(SBox).HeightOverride(56)[T::Columns({
  {1.f,T::Note(TAttribute<FString>::CreateLambda([this]{const auto* J=CurrentJob();if(!J||J->MarkStart<0)return FString(TEXT("MARK IN before the extra ticks, MARK OUT after: the keys at the tape head, or click the trace."));if(J->MarkEnd<0)return FString::Printf(TEXT("MARK IN %.1f s / MARK OUT still to set"),J->MarkStart);return MarkCoversTicks(J->MarkStart,J->MarkEnd)?FString::Printf(TEXT("MARK %.1f-%.1f s COVERS THE TICKS"),J->MarkStart,J->MarkEnd):FString::Printf(TEXT("MARK %.1f-%.1f s MISSES TICKS: from before the first to after the last"),J->MarkStart,J->MarkEnd);}),20,TAttribute<FLinearColor>(LampOn))},
  // RX-ANOMALY-FLOW-20261113: what the deck last said comes first (a refusal, a mark set); the capture's state only when it says nothing.
  // (The first pulsar's capture status stood here all night, so a key that refused left no word on the panel.)
  {1.3f,T::Note(TAttribute<FString>::CreateLambda([this]{const FString Status=AudioCaptureStatus();return FeedbackText().IsEmpty()?Status:FeedbackText();}),20,TAttribute<FLinearColor>(Cream))}},14)]);
 // F: the listening references. The chip says what they are for, in plain words (D45-20261110).
 Rows.Add(SNew(SBox).HeightOverride(60)[T::Columns({
  {1.25f,SNew(SBox).HAlign(HAlign_Left).VAlign(VAlign_Center)[S47Kit::Badge(TEXT("KNOWN SOUNDS TO COMPARE BY EAR"),-1)]},
  {1.6f,SNew(SBox).VAlign(VAlign_Center)[T::Note(TAttribute<FString>::CreateLambda([this]{return AudioDesk?AudioDesk->ReferenceLabel():FString();}),18,TAttribute<FLinearColor>(Cream))]},
  {1.f,T::Button(TEXT("NEXT REFERENCE"),Press(TEXT("ref_next")),false,true)},
  {1.1f,T::ButtonLive(TAttribute<FString>::CreateLambda([this]{return FString(AudioDesk&&AudioDesk->ReferencePlaying()?TEXT("STOP REFERENCE"):TEXT("PLAY REFERENCE"));}),Press(TEXT("ref_play")),false,true)},
  {.9f,T::Button(TEXT("MATCH"),[this]{if(AudioDesk&&AudioDesk->ReferenceIndex()>=0)MarkReference(AudioDesk->ReferenceIndex());},false,TAttribute<bool>::CreateLambda([this]{return AudioDesk&&AudioDesk->ReferenceIndex()>=0;}))}},12)]);
 auto Leave=[this]{if(M&&M->UI)M->UI->Close();};
 // The faceplate is drawn at 1200 x 900 and scaled to the view, so it is one screen at any size; the room is dimmed around it.
 SetPanel(SNew(SS47TapePanel).OnLeave(Leave)
  [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.006f,.007f,.004f,.94f)).Padding(0)
   [SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::Both)
    [SNew(SBox).WidthOverride(1200).HeightOverride(900)[T::Faceplate(TEXT("REEL DECK / COMPARE RECORDINGS"),S47Kit::Col(Rows),T::Button(TEXT("ESC / RETURN TO ROOM"),Leave))]]]],false,true);bTapePanel=true;
}
void FS47FreeformSession::OpenArchive(bool bBinderOffGlass)
{
 if(!M||!M->UI)return;const bool Computer=TerminalOnGlass();TArray<S47Kit::FW> Rows;
 // RECORDS-BINDER (-> M3): off the glass the records-room binder is the binder itself (S47FreeformBinder.cpp); what follows is the
 // OPS page. Critic 12 Nov: the logbook, the work order, the shift log, the exception binder and the service record are not the
 // binder: they keep their own paper page (bBinderOffGlass false), and the page's own tabs and rows stay on it.
 if(!Computer&&bBinderOffGlass){OpenBinder();return;}
 auto Paper=[Computer](const FString& T){return Computer?S47FreeformTerminal::Paper(T):S47Kit::Paper(T);};
 auto Plain=[Computer](const FString& T){return Computer?S47FreeformTerminal::Small(T):S47Kit::Small(T);};
 // What needs doing comes first; the record itself is only read.
 if(!ArchiveNote.IsEmpty()){Rows.Add(Paper(ArchiveNote));ArchiveNote.Empty();}
 const FJob* Waiting=CurrentJob()&&CurrentJob()->Stage==EJobStage::Packet?CurrentJob():Work.FindByPredicate([](const FJob& X){return X.Stage==EJobStage::Packet;});
 if(Waiting)
 {
  if(NearBinder())
  {
   Rows.Add(Plain(FString::Printf(TEXT("TO FILE / PACKET %s IN HAND"),*Waiting->Id)));
   Rows.Add(S47Kit::Action(TEXT("FILE PACKET IN THIS BINDER"),[this,Id=Waiting->Id]
   {
    const bool Filed=FilePacket();UE_LOG(LogTemp,Display,TEXT("S47: binder filing %s: %s"),*Id,Filed?TEXT("filed"):*Feedback);
    ArchiveNote=Filed?FString::Printf(TEXT("FILED / %s IN THE BINDER / CREDITED"),*Id):Feedback;
    OpenArchive(false);
   }));
  }
  else Rows.Add(Paper(FString::Printf(TEXT("TO FILE / PACKET %s WAITING / BINDER: RECORDS ROOM / THIS COPY READ ONLY"),*Waiting->Id)));
 }
 // CASES-OPS-20261109: the old three-family count ("READY TO COMPARE") had nothing to do here: it is gone. The cases are on CASE BOARD.
 if(const FString Gap=Predecessor?Predecessor->ArchiveLabel():FString();!Gap.IsEmpty())
 {
  const auto Open=[this]{if(Predecessor)Predecessor->OpenGap();};
  Rows.Add(Computer?S47FreeformTerminal::Button(Gap,Open):S47Kit::Button(Gap,Open));
 }
 struct FEntry{FString Key,Title,Tag,Detail;};
 TArray<FEntry> Tabs[5];
 // FIX-20261110: the station's own files, as the duty officer releases them. Read only, nothing saved.
 int32 FilesHeld=0;
 for(int32 I=0;I<S47StationFiles::Count();++I)
 {
  const auto& F=S47StationFiles::Get(I);if(F.Day>Day()){++FilesHeld;continue;}
  Tabs[4].Add({F.Key,F.Title,FString::Printf(TEXT("RELEASED DAY %02d / READ ONLY"),F.Day),F.Text});
 }
 for(const FJob& J:Work)if(J.bPrinted||!J.OriginalPage.IsEmpty())
 {
  const FString Control=J.bControl?FString::Printf(TEXT("INDEPENDENT CONTROL %.1f/8 s"),J.ControlAt):J.bAnomaly&&!Specialized(J.Phenomenon)?FString(J.bAudioReviewed?TEXT("CHECKED ON TAPE"):TEXT("TAPE CHECK PENDING")):FString(TEXT("CONTROL NOT RECORDED"));
  const FString Mark=J.MarkStart>=0&&J.MarkEnd>J.MarkStart?FString::Printf(TEXT(" / MARK %.2f–%.2f s"),J.MarkStart,J.MarkEnd):FString();
  const FString Conclusion=!J.bAnomaly?TEXT("ROUTINE RESULT"):!J.bAudioReviewed?TEXT("COMPARISON PENDING"):J.Phenomenon==TEXT("wrong_source")?TEXT("OFF-SOURCE RESPONSE"):J.Phenomenon==TEXT("time_disagreement")?TEXT("CLOCK DISAGREEMENT"):J.Phenomenon==TEXT("duplicate_print")?TEXT("DUPLICATE ORIGINAL"):TEXT("SOURCE UNRESOLVED");
  Tabs[0].Add({J.Id,FString::Printf(TEXT("%s / ORIGINAL %s-original"),*J.Id,*J.Id),Conclusion+TEXT(" / ")+StageText(J.Stage),FString::Printf(TEXT("%s%s\n%s / %s"),*Control,*Mark,*Conclusion,*StageText(J.Stage))});
 }
 TArray<FString> Keys;Findings.GetKeys(Keys);Keys.Sort([this](const FString& A,const FString& B){const int32 X=FindingDays.FindRef(A),Y=FindingDays.FindRef(B);return X!=Y?X>Y:A<B;});
 for(const FString& K:Keys)
 {
  if(FS47FreeformCases::IsCaseRecord(K))continue; // CASES-OPS-20261109: a case's own records are under CASES
  const bool Investigate=K.StartsWith(TEXT("extra_pulse"))||K.StartsWith(TEXT("feeder_fault"))||K.StartsWith(TEXT("wrong_source"))||K.StartsWith(TEXT("roof_steps"))||K.StartsWith(TEXT("observer"))||K.StartsWith(TEXT("peek-"))||K.StartsWith(TEXT("flood-room-"))||K.StartsWith(TEXT("door-visit-"))||K.StartsWith(TEXT("door-marks-"))||K.StartsWith(TEXT("landing_trace"))||K.StartsWith(TEXT("time_disagreement"))||K.StartsWith(TEXT("duplicate_print"))||K.StartsWith(TEXT("carrier_"));
  const FString Record=Findings.FindRef(K);const bool Bench=K.StartsWith(TEXT("service"))||K.StartsWith(TEXT("workshop"))||K.StartsWith(TEXT("donor"))||K.StartsWith(TEXT("feeder-repair"))||K.StartsWith(TEXT("sample"));
  // The record's own first sentence names it; internal keys stay internal.
  int32 Stop=INDEX_NONE;Record.FindChar(TEXT('.'),Stop);FString Title=Stop>0?Record.Left(Stop):Record;if(Title.Len()>58)Title=Title.Left(55)+TEXT("...");
  FString Entry=FString::Printf(TEXT("DAY %d\nRETAINED RECORD / %s"),FindingDays.FindRef(K),*Record);
  if(Investigate)Entry+=TEXT("\nSTILL UNKNOWN / Whether this repeats under independent conditions.\nPOSSIBLE NEXT CHECK / Retain a later observation and its independent control; compare with this original.");
  Tabs[Bench?2:1].Add({K,Title,FString::Printf(TEXT("DAY %d"),FindingDays.FindRef(K)),Entry});
 }
 // CASES-OPS-20261109: the cases closed, one line each, the sheet and what came back under it.
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)if(const auto* C=Cases.Get(Line);C&&C->bOpen&&C->bConcluded)
 {
  const auto& D=FS47FreeformCases::Def(Line);
  const FString Tag=D.Mystery?(C->bReplied?FString::Printf(TEXT("FILED D%02d / REPLY IN"),C->ConcludeDay):FString::Printf(TEXT("FILED D%02d / REPLY 08:00"),C->ConcludeDay)):FString(TEXT("CONCLUDED"));
  FString Detail=FString::Printf(TEXT("CASE / %s / %s\n%s\n%s"),D.Title,*CaseKind(Line),*CaseNextLine(Line),*CaseSheetHead(Line));
  for(int32 T=0;T<FS47FreeformCases::TestCount;++T)if(C->Results[T].bDone)Detail+=FString::Printf(TEXT("\nTEST %d / %s / D%02d / %s"),T+1,*FString(D.Tests[T].Name).ToUpper(),C->Results[T].Day,*FS47FreeformCases::TestResultText(Line,T,C->Results[T].bPresent));
  if(const FString Reply=FindingText(FString::Printf(TEXT("case%d-reply"),Line+1));!Reply.IsEmpty())Detail+=TEXT("\n\n")+Reply;
  Tabs[3].Add({FString::Printf(TEXT("case-%d"),Line),FString(D.Title),Tag,Detail});
 }
 const bool bNoneRetained=Tabs[0].IsEmpty()&&Tabs[1].IsEmpty()&&Tabs[2].IsEmpty()&&Tabs[3].IsEmpty();
 const FString NoneYet=TEXT("NO RETAINED RECORDS YET. ORIGINALS, FIELD FINDINGS, SERVICE AND CLOSED CASES ARE INDEXED HERE AS THEY ARE RETAINED. PROCEDURE: THE SOP CARD AT THE TERMINAL.");
 {
  ArchiveTab=FMath::Clamp(ArchiveTab,0,4);
  if(Tabs[ArchiveTab].IsEmpty())ArchiveTab=!Tabs[0].IsEmpty()?0:!Tabs[1].IsEmpty()?1:!Tabs[2].IsEmpty()?2:!Tabs[3].IsEmpty()?3:4;
  static const TCHAR* TabNames[]={TEXT("ORIGINALS"),TEXT("FIELD FINDINGS"),TEXT("SERVICE AND BENCH"),TEXT("CASES"),TEXT("STATION FILES")};
  static const TCHAR* GlassTabs[]={TEXT("ORIGINALS"),TEXT("FINDINGS"),TEXT("SERVICE"),TEXT("CASES"),TEXT("STATION")}; // five fit the glass
  const auto& List=Tabs[ArchiveTab];const FEntry* Pick=List.FindByPredicate([&](const FEntry& E){return E.Key==ArchivePick;});if(!Pick)Pick=&List[0];
  TSharedRef<SScrollBox> Scroll=SNew(SScrollBox);
  if(ArchiveTab==4)
  {
   // A file is longer than the glass: it is read a page at a time.
   for(int32 I=0;I<S47StationFiles::Count()&&I<32;++I)if(Pick->Key==S47StationFiles::Get(I).Key)StationFilesSeen|=1u<<I;
   const TArray<FString> Pages=S47StationPages(Pick->Detail,Computer?54:68,Computer?24:30);
   ArchivePage=FMath::Clamp(ArchivePage,0,Pages.Num()-1);
   TArray<S47Kit::FW> Sheet;Sheet.Add(Plain(Pages[ArchivePage]));
   if(Pages.Num()>1)
   {
    const FString Turn=FString::Printf(TEXT("PAGE %d OF %d / %s"),ArchivePage+1,Pages.Num(),ArchivePage+1<Pages.Num()?TEXT("NEXT PAGE"):TEXT("BACK TO PAGE 1"));
    const auto Next=[this,N=Pages.Num()]{ArchivePage=(ArchivePage+1)%N;OpenArchive(false);};
    Sheet.Add(Computer?S47FreeformTerminal::Button(Turn,Next,true):S47Kit::Button(Turn,Next,true));
   }
   const FString Held=FilesHeld>0?FString::Printf(TEXT("%d MORE WITH THE DUTY OFFICER / RELEASED AT 08:00, NOT EVERY MORNING"),FilesHeld):FString(TEXT("ALL FILES RELEASED / READ ONLY"));
   if(Computer)
   {
    TArray<T::FNav> Bar;for(int32 I=0;I<5;++I)Bar.Add({FString::Printf(TEXT("%s %d"),GlassTabs[I],Tabs[I].Num()),I==ArchiveTab,[this,I]{ArchiveTab=I;ArchivePick.Empty();ArchivePage=0;OpenArchive(false);}});
    Rows.Add(T::Tabs(Bar));
    TArray<T::FRow> Lines;for(const auto& E:List)Lines.Add({{E.Title},&E==Pick,[this,Key=E.Key]{ArchivePick=Key;ArchivePage=0;OpenArchive(false);},FString()});
    TArray<S47Kit::FW> Left;Left.Add(T::Table({TEXT("STATION FILE")},{1.f},Lines));Left.Add(T::Small(Held));if(bNoneRetained)Left.Add(T::Small(NoneYet));
    Rows.Add(T::Columns({{.37f,S47Kit::Col(Left)},{.63f,T::Panel(Pick->Tag,S47Kit::Col(Sheet))}},14));
   }
   else
   {
    TArray<S47Kit::FW> Bar;for(int32 I=0;I<5;++I)Bar.Add(S47Kit::Choice(FString::Printf(TEXT("%s %d"),TabNames[I],Tabs[I].Num()),I==ArchiveTab,[this,I]{ArchiveTab=I;ArchivePick.Empty();ArchivePage=0;OpenArchive(false);}));
    Rows.Add(S47Kit::Row(Bar));
    TArray<S47Kit::FW> Left;for(const auto& E:List)Left.Add(S47Kit::Choice(E.Title,&E==Pick,[this,Key=E.Key]{ArchivePick=Key;ArchivePage=0;OpenArchive(false);}));
    Left.Add(S47Kit::Small(Held));if(bNoneRetained)Left.Add(S47Kit::Small(NoneYet));
    Rows.Add(SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(.36f).Padding(0,0,18,0)[S47Kit::Col(Left)]+SHorizontalBox::Slot().FillWidth(.64f)[S47Kit::Col(Sheet)]);
   }
  }
  else if(Computer)
  {
   TArray<T::FNav> Bar;for(int32 I=0;I<5;++I)Bar.Add({FString::Printf(TEXT("%s %d"),GlassTabs[I],Tabs[I].Num()),I==ArchiveTab,[this,I]{ArchiveTab=I;ArchivePick.Empty();ArchivePage=0;OpenArchive(false);}});
   Rows.Add(T::Tabs(Bar));
   TArray<T::FRow> Lines;for(const auto& E:List)Lines.Add({{E.Title,E.Tag},&E==Pick,[this,Key=E.Key]{ArchivePick=Key;OpenArchive(false);},FString()});
   Scroll->AddSlot()[T::Table({TEXT("RECORD"),TEXT("STATE")},{.56f,.44f},Lines)];
  }
  else
  {
   TArray<S47Kit::FW> Bar;for(int32 I=0;I<5;++I)Bar.Add(S47Kit::Choice(FString::Printf(TEXT("%s %d"),TabNames[I],Tabs[I].Num()),I==ArchiveTab,[this,I]{ArchiveTab=I;ArchivePick.Empty();ArchivePage=0;OpenArchive(false);}));
   Rows.Add(S47Kit::Row(Bar));
   for(const auto& E:List)Scroll->AddSlot()[S47Kit::Choice(E.Title,&E==Pick,[this,Key=E.Key]{ArchivePick=Key;OpenArchive(false);},false,E.Tag)];
  }
  if(ArchiveTab!=4){Rows.Add(SNew(SBox).HeightOverride(210).VAlign(VAlign_Top)[Scroll]);Rows.Add(Paper(Pick->Detail));}
 }
 if(Computer)OpenTerminal(3,TEXT("OPS / RECORDS"),OpsShell(3,S47Kit::Col(Rows)));
 else SetPanel(S47Kit::Shell(M->UI,TEXT("SARO / RECORDS"),S47Kit::Col(Rows),1000));
}
bool FS47FreeformSession::StationSealed()const
{
 if(!M||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->World||M->World->Area!=TEXT("saro"))return false;
 return !M->Web->IsDoorOpen(TEXT("doorSouth"))&&!M->Web->IsDoorOpen(TEXT("doorEast"))&&M->Web->Rect(TEXT("room.bounds")).Contains(M->Player->Pos);
}
bool FS47FreeformSession::CaseObserved(const FJob& J)const
{
 if(FS47FreeformCases::Def(J.CaseLine).Observable==FS47FreeformCases::EObservable::ExtraTicks)return !J.TraceTicks.IsEmpty();
 return FMath::Abs(J.ReceiverClock-J.ReferenceClock)>1.;
}
FString FS47FreeformSession::CasePrecondition(const FJob& J)const
{
 if(J.CaseTest<0)return FString();const auto* C=Cases.Get(J.CaseLine);if(!C)return FString();
 switch(CaseCondition(J.CaseLine,J.CaseTest))
 {
  case ECaseCondition::Generator:return Field&&Field->GeneratorRunning()?FString():FString(TEXT("BEHIND THE CONTROL ROOM: START THE FUELLED STANDBY GENERATOR; KEEP THE WHOLE CASE TEST RECORDING ON GENERATOR POWER, OFF THE MAINS"));
  case ECaseCondition::DoorsSealed:return StationSealed()?FString():FString(TEXT("SARO CONTROL ROOM: CLOSE THE SOUTH AND EAST OUTSIDE DOORS AND STAY INSIDE THROUGH THE WHOLE CASE TEST RECORDING"));
  case ECaseCondition::RelaySwap:return InventoryCount(TEXT("relay"))>0||J.Stage>EJobStage::Pointed?FString():FString(TEXT("CALICHE GAS STATION: BUY A RELAY FOR THE TIMING SWAP; PARTS STORAGE AT SARO HAS NONE"));
  default:return FString();
 }
}
void FS47FreeformSession::UnfitCaseRelay(const FJob& J)
{
 // CASETEST-20261109: StartAcquisition fits the spare; a recording stopped short (spoiled, taken, contact) gives it back, unused.
 if(J.CaseTest>=0&&!J.bCaseRecorded&&J.Stage==EJobStage::Integrating&&CaseCondition(J.CaseLine,J.CaseTest)==ECaseCondition::RelaySwap)AddItem(TEXT("relay"),1);
}
void FS47FreeformSession::SpoilCaseTest(FJob& J,const FString& Why)
{
 // CASETEST-20261109: a broken condition used to let the recording run on, register as INVALID and drop the plan. Now it stops at
 // once and the plan stays on the same job: put it right, START RECORDING again. TEST SPOILED stays on the row until then.
 UnfitCaseRelay(J);J.bCaseValid=false;J.Stage=EJobStage::Pointed;J.Elapsed=0;J.TraceTicks.Reset();Settle=0;bCrossRunning=false;
 Feedback=TEXT("TEST SPOILED: ")+Why+TEXT(" then press START RECORDING again for job ")+J.Id+TEXT(". The spoiled run counts for nothing; the test stays planned, no new plan needed.");Changed(Feedback);
}
int32 FS47FreeformSession::ContinueJob()const
{
 // CASETEST-20261109: a planned case test that can be recorded now (its window open, its condition met) comes before a routine job
 // not yet begun; work already under way keeps its place.
 const FJob* J=CurrentJob();
 const bool bBusy=J&&J->Stage>EJobStage::New&&J->Stage!=EJobStage::Filed&&!J->bDuplicateRejected&&!EveningGated(*J);
 if(!bBusy)
 {
  const int32 Case=Work.IndexOfByPredicate([this](const FJob& X)
  {
   const auto* C=Cases.Get(X.CaseLine);
   return X.CaseTest>=0&&!X.bCaseRecorded&&X.Stage<=EJobStage::Pointed&&C&&C->bOpen&&!C->bConcluded&&C->Planned==X.CaseTest&&C->PlannedJob==X.Id
    &&!EveningGated(X)&&CasePrecondition(X).IsEmpty();
  });
  if(Case!=INDEX_NONE)return Case;
 }
 if(J&&J->Stage!=EJobStage::Filed&&!J->bDuplicateRejected&&!EveningGated(*J))return Selected;
 return Work.IndexOfByPredicate([this](const FJob& X){return X.Stage!=EJobStage::Filed&&!X.bDuplicateRejected&&!EveningGated(X);});
}
namespace
{
 // CASES-OPS-20261109: how pressing a case's step is; 0: nothing to do or see.
 int32 CaseStepRank(FS47FreeformSession::ECaseStep S)
 {
  using E=FS47FreeformSession::ECaseStep;
  switch(S){case E::Conclude:return 6;case E::RunPlan:return 5;case E::File:return 4;case E::ChooseTest:return 3;case E::AwaitReply:return 1;default:return 0;}
 }
}
int32 FS47FreeformSession::CaseAwaitingChoice()const
{
 // The star on CASE BOARD: only when there is something to do there (CaseStep), the most pressing first.
 int32 Best=-1,BestRank=0;
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)
 {
  const ECaseStep S=CaseStep(Line).Step;
  if(S!=ECaseStep::ChooseTest&&S!=ECaseStep::Conclude&&S!=ECaseStep::File)continue;
  if(const int32 R=CaseStepRank(S);R>BestRank){Best=Line;BestRank=R;}
 }
 return Best;
}
FS47FreeformSession::FCaseStep FS47FreeformSession::CaseStep(int32 Line)const
{
 FCaseStep S;const auto* C=Cases.Get(Line);if(!C||!C->bOpen)return S;
 const auto& D=FS47FreeformCases::Def(Line);const FString Title=D.Title;
 if(C->bConcluded)
 {
  if(D.Mystery&&!C->bReplied)
  {
   S.Step=ECaseStep::AwaitReply;S.Next=TEXT("TELEX (control room): THE REPLY ON ")+Title+TEXT(" COMES WITH THE MORNING, AT 08:00.");
   S.Card=TEXT("CASE // ")+Title+TEXT(": REPLY DUE 08:00.");S.Short=Title+TEXT(": REPLY DUE 08:00.");
  }
  // TELEX-20261109: in, and still on the machine: the step is to tear it off there.
  else if(D.Mystery&&OnTelex(FString::Printf(TEXT("freeform-case%d-reply"),Line+1)))
  {
   S.Step=ECaseStep::AwaitReply;S.Next=TEXT("TELEX (control room): THE REPLY ON ")+Title+TEXT(" IS IN. TEAR IT OFF AT THE TELEX.");
   S.Card=TEXT("TELEX // TEAR OFF THE REPLY ON ")+Title+TEXT(".");S.Short=Title+TEXT(": TEAR OFF THE REPLY AT THE TELEX.");
   S.Task=TEXT("CASE / ")+Title+TEXT(": tear off the reply at the telex in the control room");
  }
  return S;
 }
 // One explanation left: that is the step, whatever is still planned (an older save's plan; Plan refuses a new one).
 const int32 Left=Cases.Remaining(Line);
 if(C->Planned>=0&&Left>1)
 {
  const FString Test=CaseTestName(Line,C->Planned);
  S.Card=TEXT("CASE BOARD // FOLLOW THE PLAN FOR ")+Title+TEXT(".");
  if(PlannedFieldTest(Line)>=0){S.Step=ECaseStep::RunPlan;S.Next=FieldNextAction(Line);S.Task=S.Next;S.Short=Title+TEXT(": FIELD TEST, ")+Test.ToUpper()+TEXT(".");return S;}
  S.Task=TEXT("CASE / ")+Test+TEXT(": run job ")+C->PlannedJob;
  const FJob* J=Work.FindByPredicate([&](const FJob& X){return X.Id==C->PlannedJob&&X.CaseLine==Line&&X.CaseTest==C->Planned;});
  S.Step=ECaseStep::RunPlan;S.Short=Title+TEXT(": RUN THE PLANNED TEST.");
  S.Next=TEXT("OPS / DAILY WORK: RUN CASE TEST JOB ")+C->PlannedJob+TEXT(" (")+Test.ToUpper()+TEXT(") FOR ")+Title+TEXT(".");
  // CASETEST-20261109: a spoiled run says so, and that the same job is simply recorded again
  if(J&&!J->bCaseValid&&J->Stage<=EJobStage::Pointed){S.Next=TEXT("OPS / DAILY WORK: TEST SPOILED. RECORD CASE TEST JOB ")+C->PlannedJob+TEXT(" (")+Test.ToUpper()+TEXT(") AGAIN FOR ")+Title+TEXT("; NO NEW PLAN NEEDED.");S.Short=Title+TEXT(": TEST SPOILED, TAKE IT AGAIN.");}
  return S;
 }
 if(Left<=1&&!D.Mystery)
 {
  int32 Cause=-1;for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)if(!Cases.Excluded(Line,I))Cause=I;
  S.Step=ECaseStep::Conclude;
  S.Next=TEXT("OPS / CASE BOARD: REGISTER THE SUPPORTED CONCLUSION FOR ")+Title+(Cause>=0?TEXT(": ")+FString(D.Causes[Cause]).ToUpper():FString())+TEXT(".");
  S.Card=TEXT("CASE BOARD // REGISTER THE CONCLUSION FOR ")+Title+TEXT(".");S.Short=Title+TEXT(": REGISTER THE CONCLUSION.");
  S.Task=TEXT("CASE / ")+Title+TEXT(": register the supported conclusion on the OPS case board");return S;
 }
 const bool bCold=Cases.TrailCold(Line,DayNumber);
 bool bTest=false;
 if(!bCold&&Left>1)for(int32 T=0;T<FS47FreeformCases::TestCount;++T)
  bTest|=!C->Results[T].bDone&&FS47FreeformCases::UnavailableReason(Line,T,HasKit(TEXT("logger2"))).IsEmpty()&&!(D.Tests[T].Access&&!Cases.HasAccess(D.Tests[T].Access));
 const FString Late=LateStep(Line,Findings);
 if(D.Mystery&&(Left<=1||(!bTest&&(Late.IsEmpty()||Late==TEXT("file your assessment")))))
 {
  S.Step=ECaseStep::File;
  S.Next=TEXT("OPS / CASE BOARD: FILE YOUR ASSESSMENT OF ")+Title+(bCold?TEXT(". THE TRAIL HAS GONE COLD."):TEXT("."));
  S.Card=TEXT("CASE BOARD // FILE YOUR ASSESSMENT OF ")+Title+TEXT(".");S.Short=Title+TEXT(": FILE YOUR ASSESSMENT.");
  S.Task=TEXT("CASE / ")+Title+TEXT(": file your assessment on the OPS case board");return S;
 }
 S.Step=ECaseStep::ChooseTest;
 if(!Late.IsEmpty())
 {
  S.Next=TEXT("OPS / CASE BOARD: ")+Title+TEXT(": ")+Late.ToUpper()+TEXT(".");S.Card=TEXT("CASE // ")+Title+TEXT(": ")+Late.ToUpper()+TEXT(".");
  S.Short=Title+TEXT(": LOOK AT THE OTHER PLACE, OR FILE.");S.Task=TEXT("CASE / ")+Title+TEXT(": ")+Late;
 }
 else if(D.Mystery)
 {
  S.Next=TEXT("OPS / CASE BOARD: CHOOSE A CONTROL OR FILE YOUR ASSESSMENT FOR ")+Title+TEXT(".");S.Card=TEXT("CASE BOARD // A CONTROL OR A FILING FOR ")+Title+TEXT(".");
  S.Short=Title+TEXT(": A CONTROL, OR FILE IT.");S.Task=TEXT("CASE / ")+Title+TEXT(": choose a control or file your assessment on the OPS case board");
 }
 else
 {
  S.Next=FString::Printf(TEXT("OPS / CASE BOARD: CHOOSE A TEST FOR %s. %d OF 3 EXPLANATIONS STILL POSSIBLE."),*Title,Left);S.Card=TEXT("CASE BOARD // CHOOSE A TEST FOR ")+Title+TEXT(".");
  S.Short=Title+TEXT(": CHOOSE A TEST.");S.Task=TEXT("CASE / ")+Title+TEXT(": choose a test on the OPS case board");
 }
 return S;
}
int32 FS47FreeformSession::UrgentCase(bool bActionOnly)const
{
 int32 Best=-1,BestRank=0;
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)
 {
  const int32 R=CaseStepRank(CaseStep(Line).Step);
  if(R<=0||(bActionOnly&&R<3))continue;
  if(R>BestRank){Best=Line;BestRank=R;}
 }
 return Best;
}
FString FS47FreeformSession::CaseRecordLine(const FString& Key)const
{
 if(!FS47FreeformCases::IsCaseRecord(Key))return FString();
 FString Rest;const int32 Case=FCString::Atoi(*Key.Mid(4));const int32 Line=Case-1;
 const auto* C=Cases.Get(Line);if(!C||!Key.Split(TEXT("-"),nullptr,&Rest))return FString();
 const auto& D=FS47FreeformCases::Def(Line);const FString Head=FString(D.Title)+TEXT(": "); // Tom 13 Nov: the title only, no case number (FS47NightLog::CaseLine knows both)
 if(Rest.StartsWith(TEXT("test")))
 {
  const int32 Test=FCString::Atoi(*Rest.Mid(4))-1;if(Test<0||Test>=FS47FreeformCases::TestCount||!C->Results[Test].bDone)return FString();
  TArray<FString> Out;for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)if(FS47FreeformCases::PresentFor(Line,I,Test)!=C->Results[Test].bPresent)Out.Add(D.Causes[I]);
  return Head+FString(D.Tests[Test].Name).ToLower()+TEXT(" registered")+(Out.IsEmpty()?FString(TEXT(".")):TEXT("; rules out ")+JoinAnd(Out)+TEXT("."));
 }
 if(Rest==TEXT("conclusion"))
 {
  if(!C->bConcluded||C->Conclusion<0)return FString();
  return Head+(D.Mystery?TEXT("assessment filed, "):TEXT("conclusion registered, "))+FString(D.Causes[C->Conclusion]).ToLower()+TEXT(".");
 }
 if(Rest==TEXT("reply"))return Head+TEXT("the reply came in on the telex.");
 return FString();
}
// ---- CASES-OPS-20261109, step 4: the case sheet (OPS / CASE BOARD, the printed result sheet, RECORDS / CASES) ----------------------
FString FS47FreeformSession::CaseKind(int32 Line){return FS47FreeformCases::Def(Line).Mystery?TEXT("ASSESSMENT"):TEXT("TRIAL CASE");}
FString FS47FreeformSession::CaseClosesWhen(int32 Line)
{
 // Tom, 9 November (DECISIONS): the two closing rules stay, and the sheet says which one holds: a trial case is answered at once, an
 // assessment the next morning.
 return FS47FreeformCases::Def(Line).Mystery?TEXT("CLOSES WHEN YOU FILE AN ASSESSMENT, ON ANY EVIDENCE. ANSWERED NEXT MORNING: REPLY BY TELEX 08:00.")
  :TEXT("CLOSES WHEN YOUR TESTS RULE OUT ALL BUT ONE. ANSWERED AT ONCE: RECORDS REPLY BY TELEX.");
}
FString FS47FreeformSession::CaseOnClose(int32 Line)const
{
 // What closing it brings, as the code will do it (ConcludeCase, StepCaseReplies, SheriffTrust, FS47Predecessor::Count), and only
 // what does not give away the hidden cause: a reply's spare or call-out follows the cause, so it is said as what may come.
 const auto& D=FS47FreeformCases::Def(Line);TArray<FString> Parts;
 if(!D.Mystery){Parts.Add(TEXT("RECORDS REPLY AT ONCE"));if(Line==0)Parts.Add(TEXT("FIELD LOGGER ON LOAN"));}
 else
 {
  Parts.Add(TEXT("REPLY 08:00"));Parts.Add(FString::Printf(TEXT("A CALL-OUT MAY COST $%d"),int32(FS47FreeformCases::CallOutCharge)));
  if(Line==FS47FreeformCases::LostLine)Parts.Add(FString::Printf(TEXT("MAGNETOMETER $%d OFF"),LostHoursDiscount));
 }
 if(!DeputyOnRoute())Parts.Add(TEXT("COUNTY PATROL TAKES IN SARO"));
 if(Predecessor&&Predecessor->Live())Parts.Add(!D.Mystery||CaseSupported(Line)?TEXT("COUNTS IN DR. WARD'S REVIEW"):Line==FS47FreeformCases::LandLine?TEXT("ONE NIGHT: DR. WARD WILL NOT COUNT IT"):TEXT("ONE TRACE: DR. WARD WILL NOT COUNT IT"));
 return TEXT("ON CLOSE: ")+FString::Join(Parts,TEXT(" / "));
}
FString FS47FreeformSession::CaseSource(int32 Line)const
{
 const auto* C=Cases.Get(Line);if(!C||!C->bOpen)return FString();
 const auto& D=FS47FreeformCases::Def(Line);
 const TCHAR* From=Line<=1?TEXT("RETAINED ORIGINAL"):Line==FS47FreeformCases::LostLine?TEXT("LOST TIME"):D.Mystery?TEXT("FIELD TRACE"):TEXT("FIELD OBSERVATION");
 // The original's own clock (FS47FreeformCases::SetReference) when it differs from the opening.
 if(C->RefDay!=C->OpenDay||C->RefClock!=C->OpenClock)
  return FString::Printf(TEXT("SOURCE: %s, REC D%02d %s / OPENED D%02d %s"),From,C->RefDay,*FS47FreeformCases::Clock(C->RefClock),C->OpenDay,*FS47FreeformCases::Clock(C->OpenClock));
 return FString::Printf(TEXT("SOURCE: %s %s D%02d %s"),From,Line<=1?TEXT("/ REC AND OPENED"):TEXT("/ OPENED"),C->OpenDay,*FS47FreeformCases::Clock(C->OpenClock));
}
FString FS47FreeformSession::CaseStanding(int32 Line,int32 Cause)const
{
 const auto* C=Cases.Get(Line);if(!C||!C->bOpen||Cause<0||Cause>=FS47FreeformCases::CauseCount)return FString();
 const bool bMystery=FS47FreeformCases::Def(Line).Mystery!=nullptr;
 if(C->bConcluded&&C->Conclusion==Cause)return bMystery?TEXT("FILED AS YOUR ASSESSMENT"):TEXT("CONCLUDED / SUPPORTED BY YOUR TESTS");
 // The day of the first retained result that rules it out.
 int32 Day=0;
 for(int32 T=0;T<FS47FreeformCases::TestCount;++T)if(C->Results[T].bDone&&FS47FreeformCases::PresentFor(Line,Cause,T)!=C->Results[T].bPresent&&(Day==0||C->Results[T].Day<Day))Day=FMath::Max(1,C->Results[T].Day);
 if(Day>0&&Cases.Excluded(Line,Cause))return FString::Printf(TEXT("RULED OUT D%02d"),Day); // (the land's records may disagree)
 if(Cases.Excluded(Line,Cause))return Line==FS47FreeformCases::LandLine?TEXT("RULED OUT / SAME MINUTE ON ARRAY"):TEXT("RULED OUT / LOST TIME"); // the visitors' wind, after a taking or a room lit from outside; the land's animals and weather, after the same minute on the array (LOOP-3-20261109)
 if(C->bConcluded)return bMystery?TEXT("NOT FILED"):TEXT("RULED OUT");
 return TEXT("STILL POSSIBLE");
}
FString FS47FreeformSession::CaseNextLine(int32 Line)const
{
 const auto* C=Cases.Get(Line);if(!C||!C->bOpen)return FString();
 const auto& D=FS47FreeformCases::Def(Line);const FCaseStep S=CaseStep(Line);
 switch(S.Step)
 {
  case ECaseStep::Conclude:
  {
   int32 Cause=-1;for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)if(!Cases.Excluded(Line,I))Cause=I;
   return TEXT("NEXT > CONCLUDE")+(Cause>=0?TEXT(": ")+FString(D.Causes[Cause]).ToUpper():FString());
  }
  case ECaseStep::File:return FString(TEXT("NEXT > FILE YOUR ASSESSMENT"))+(Cases.TrailCold(Line,DayNumber)?TEXT(" / THE TRAIL IS COLD"):TEXT(""));
  case ECaseStep::ChooseTest:
  {
   const FString Late=LateStep(Line,Findings);
   if(!Late.IsEmpty())return TEXT("NEXT > ")+Late.ToUpper();
   return D.Mystery?FString(TEXT("NEXT > PLAN A CONTROL BELOW, OR FILE YOUR ASSESSMENT")):FString::Printf(TEXT("NEXT > PLAN A TEST BELOW / %d OF 3 STILL POSSIBLE"),Cases.Remaining(Line));
  }
  case ECaseStep::RunPlan:
   if(PlannedFieldTest(Line)>=0)return TEXT("NEXT > ")+FieldNextAction(Line).ToUpper();
   if(Work.ContainsByPredicate([&](const FJob& X){return X.Id==C->PlannedJob&&X.CaseLine==Line&&!X.bCaseValid&&X.Stage<=EJobStage::Pointed;})) // CASETEST-20261109
    return FString::Printf(TEXT("NEXT > TEST SPOILED: RECORD JOB %s (%s) AGAIN AT DAILY WORK, NO NEW PLAN"),*C->PlannedJob,*CaseTestName(Line,C->Planned).ToUpper());
   return FString::Printf(TEXT("NEXT > RUN JOB %s (%s) AT DAILY WORK"),*C->PlannedJob,*CaseTestName(Line,C->Planned).ToUpper());
  case ECaseStep::AwaitReply:return C->bReplied?FString(TEXT("NEXT > THE REPLY IS IN: TEAR IT OFF AT THE TELEX")):FString(TEXT("NEXT > THE REPLY COMES BY TELEX AT 08:00"));
  default:break;
 }
 if(!C->bConcluded||C->Conclusion<0)return FString();
 const FString Cause=FString(D.Causes[C->Conclusion]).ToUpper();
 if(D.Mystery)return FString::Printf(TEXT("CLOSED D%02d / FILED: %s%s"),C->ConcludeDay,*Cause,C->bReplied?TEXT(" / REPLY RECEIVED"):TEXT(""));
 return TEXT("CLOSED / CONCLUDED: ")+Cause;
}
FString FS47FreeformSession::CaseSheetHead(int32 Line,int32 MarkTest)const
{
 const auto* C=Cases.Get(Line);if(!C||!C->bOpen)return FString();
 const auto& D=FS47FreeformCases::Def(Line);
 TArray<FString> Out={CaseKind(Line)+TEXT(" / ")+CaseClosesWhen(Line)};
 if(!C->bConcluded)Out.Add(CaseOnClose(Line));
 Out.Add(CaseSource(Line));
 static const TCHAR* const Letter[FS47FreeformCases::CauseCount]={TEXT("A"),TEXT("B"),TEXT("C")};
 for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)
 {
  // The explanation the marked test ruled out (a result sheet just collected) says so.
  const bool bFell=MarkTest>=0&&MarkTest<FS47FreeformCases::TestCount&&C->Results[MarkTest].bDone&&FS47FreeformCases::PresentFor(Line,I,MarkTest)!=C->Results[MarkTest].bPresent&&Cases.Excluded(Line,I);
  Out.Add(FString::Printf(TEXT(" %s  %-26s %s%s"),Letter[I],*FString(D.Causes[I]).ToUpper().Left(26),*CaseStanding(Line,I),bFell?TEXT("  << THIS RESULT"):TEXT("")));
 }
 return FString::Join(Out,TEXT("\n"));
}
void FS47FreeformSession::QueueResultSheet(int32 Line,int32 Test,const FString& Job)
{
 const auto* C=Cases.Get(Line);if(!C||Test<0||Test>=FS47FreeformCases::TestCount||!C->Results[Test].bDone||CaseSheets.Num()>=8)return;
 const auto& D=FS47FreeformCases::Def(Line);const bool bPresent=C->Results[Test].bPresent;
 int32 Fell=0;for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)Fell+=FS47FreeformCases::PresentFor(Line,I,Test)!=bPresent&&Cases.Excluded(Line,I)?1:0;
 FResultSheet Sh;Sh.Line=Line;Sh.Test=Test;Sh.Day=FMath::Max(1,DayNumber);Sh.Job=Job;
 Sh.Stamp=Fell>0?FString::Printf(TEXT("RULED OUT / D%02d"),Sh.Day):FString(TEXT("STILL POSSIBLE"));
 // The sheet as the case stands the moment the result is in; printed later, it does not change.
 Sh.Page=FString::Printf(TEXT("SARO OPS V4.2  CASE TEST RESULT  %s %s\nCASE / %s\nTEST %d / %s%s\nRESULT / %s\nPREDICTION / %s\n\n%s\n\n%s\n\n%s"),
  *TerminalDate(WorldClock),*FS47FreeformCases::Clock(FMath::Fmod(WorldClock,86400.)),D.Title,Test+1,*FString(D.Tests[Test].Name).ToUpper(),Job.IsEmpty()?TEXT(""):*(TEXT(" / ")+Job),
  *FS47FreeformCases::TestResultText(Line,Test,bPresent),*FS47FreeformCases::Prediction(Line,Test),*CaseSheetHead(Line,Test),*CaseNextLine(Line),*Sh.Stamp);
 CaseSheets.Add(Sh);
}
bool FS47FreeformSession::QueueBearingSlip(int32 Day,const FString& Line,const FString& Note,bool bAgain)
{
 if(Line.IsEmpty()||CaseSheets.Num()>=8)return false;
 // D45-20261110 (Tom: "bearing slip er for rart skrevet"): plain words; Note says what a branch is and where the circle is (FS47Places::SlipNote)
 FResultSheet Sh;Sh.Day=FMath::Max(1,Day);Sh.Job=FString::Printf(TEXT("freeform-bearing-d%d%s"),Sh.Day,bAgain?TEXT("-again"):TEXT(""));Sh.Stamp=TEXT("CHECK BEFORE NOON");
 if(CaseSheets.ContainsByPredicate([&](const FResultSheet& X){return X.Job==Sh.Job;}))return false;
 Sh.Page=FString::Printf(TEXT("SARO OPS V4.2  BEARING SLIP  %s %s\n%s\n\nTHE ARRAY PICKED UP SOMETHING AWAY FROM WHERE THE DISHES WERE POINTED.\nIT GIVES A DIRECTION ONLY. IT DOES NOT SAY HOW FAR OUT IT IS OR WHAT IT IS.\n%s"),
  *TerminalDate(WorldClock),*FS47Places::SlipClock(WorldClock),*Line,*Note);
 CaseSheets.Add(Sh);Changed();return true;
}
bool FS47FreeformSession::CollectResultSheet()
{
 if(CaseSheets.IsEmpty()||CaseSheets[0].Feed<PrinterSeconds)return false;
 const FResultSheet Sh=CaseSheets[0];CaseSheets.RemoveAt(0);
 if(IsBearingSlip(Sh))
 {
  // LOOP-1-20261109: the slip off the printer is a paper in the notebook; no case board to open on.
  FS47Doc Doc;Doc.Id=Sh.Job;Doc.Title=FString::Printf(TEXT("Bearing slip, D%02d"),Sh.Day);Doc.Kind=TEXT("printout");Doc.Stamp=Sh.Stamp;Doc.Page=Sh.Page;Doc.Transcript=Sh.Page;
  StopPrinterFeedback();StepPrinter(0);
  if(M&&M->Audio)M->Audio->Sfx(TEXT("click"),FS47Sound{.Gain=.45f});
  if(M&&M->Story)M->Story->AddDoc(Doc);
  if(M&&M->UI)M->UI->Document(Doc);
  Feedback=FString::Printf(TEXT("BEARING SLIP: a new circle on your map (%s). Drive out and look before noon."),*S47Keys::MapSaid());Changed(Feedback);return true; // D45-20261110
 }
 FS47Doc Doc;Doc.Id=FString::Printf(TEXT("freeform-case%d-sheet%d-d%d"),Sh.Line+1,Sh.Test+1,Sh.Day);Doc.Title=FString::Printf(TEXT("Result sheet: %s, test %d"),FS47FreeformCases::Def(Sh.Line).Title,Sh.Test+1);
 Doc.Kind=TEXT("printout");Doc.Stamp=Sh.Stamp;Doc.Page=Sh.Page;Doc.Transcript=Sh.Page;
 // The next visit to OPS opens CASE BOARD on this case with the explanation that fell marked (Use, OpenCaseBoard).
 BoardLine=FreshLine=Sh.Line;FreshTest=Sh.Test;
 StopPrinterFeedback();StepPrinter(0);
 if(M&&M->Audio)M->Audio->Sfx(TEXT("click"),FS47Sound{.Gain=.45f});
 if(M&&M->Story)M->Story->AddDoc(Doc);
 if(M&&M->UI)M->UI->Document(Doc);
 Feedback=FString::Printf(TEXT("%s: %s. OPS opens on the case board."),*Sh.Stamp,FS47FreeformCases::Def(Sh.Line).Title);Changed(Feedback);return true;
}
void FS47FreeformSession::IngestObservations()
{
 if(!Director)return;const auto& List=Director->Observations();
 if(IngestedObservations>List.Num())IngestedObservations=0;
 for(;IngestedObservations<List.Num();++IngestedObservations)
 {
  const auto& O=List[IngestedObservations];const int32 Line=FS47FreeformCases::LineForObservation(O.Event);if(Line<0)continue;
  const bool WasOpen=Cases.Get(Line)&&Cases.Get(Line)->bOpen;
  if(Cases.AddLead(Line,O.Id,O.Day,FMath::Fmod(O.Clock,86400.)))
  {
   if(!WasOpen)Changed(TEXT("New case opened: ")+FString(FS47FreeformCases::Def(Line).Title)+TEXT(". Retained observations are leads, not yet one cause. See the OPS case board."));
   else Changed();
  }
 }
}
int32 FS47FreeformSession::PlannedFieldTest(int32 Line)const
{
 const auto* C=Cases.Get(Line);if(!C||!C->bOpen||C->bConcluded||C->Planned<0)return -1;
 return CaseCondition(Line,C->Planned)==ECaseCondition::FieldWork?C->Planned:-1;
}
bool FS47FreeformSession::CaseTestWindow(int32 Line,double& Start,double& End) const
{
 const auto* C=Cases.Get(Line);if(!C||!C->bOpen||C->bConcluded||C->Planned<0)return false;
 const auto Kind=CaseCondition(Line,C->Planned);
 // Only the visitors' field controls have a dated window now: the repeat that had the other one is withdrawn (Tom, 13 November).
 return Kind==ECaseCondition::FieldWork&&Line==2&&CaseField&&CaseField->Window(Start,End);
}
FString FS47FreeformSession::CaseWindowText(int32 Line) const
{
 double Start=0,End=0;if(!CaseTestWindow(Line,Start,End))return FString();
 const auto Stamp=[](double At)
 {
  const int32 Day=FMath::Max(1,int32(FMath::FloorToDouble((At-8*3600.)/SecondsPerDay))+1);
  return FString::Printf(TEXT("SHIFT DAY %02d / %s%s"),Day,*FS47FreeformCases::Clock(At),FMath::Fmod(At,SecondsPerDay)<8*3600.?TEXT(" (after midnight)"):TEXT(""));
 };
 FString Text=TEXT("WINDOW / ")+Stamp(Start)+TEXT(" TO ")+Stamp(End);
 if(Line==2&&CaseField&&!CaseField->Prepared())Text+=TEXT("\nPREVIEW ONLY / Prepare physically first. The window must start at least two hours after preparation; late preparation moves it to the following night.");
 else Text+=WorldClock<Start?TEXT("\nSCHEDULED / Keep this window. Other work is available while you wait."):WorldClock<End?TEXT("\nOPEN NOW / Carry out the required action; waiting does not create a measurement."):TEXT("\nWINDOW ENDED / Inspect the retained field setup for the outcome.");
 return Text;
}
bool FS47FreeformSession::CaseWaitTarget(int32 Line,double& Target) const
{
 double Start=0,End=0;if(!CaseTestWindow(Line,Start,End))return false;
 if(Line==2&&CaseField&&!CaseField->Prepared())return false;
 Target=Start-5*60.;return Target>WorldClock+1.;
}
FString FS47FreeformSession::WaitConsequences(double Target) const
{
 const double Hours=FMath::Max(0.,Target-WorldClock)/3600.;
 const int32 Day=FMath::Max(1,int32(FMath::FloorToDouble((Target-8*3600.)/SecondsPerDay))+1);
 FString Text=FString::Printf(TEXT("WAIT %.2f HOURS / UNTIL SHIFT DAY %02d, %s\nNo observing, listening, printing or physical test work is performed during this interval. Unfinished work and original records stay intact."),Hours,Day,*FS47FreeformCases::Clock(Target));
 if(Fatigue&&RulesLive())Text+=TEXT("\n")+Fatigue->BedLine(Target-WorldClock,FS47Fatigue::EBed::Station); // what kind of sleep it will be (M5 critic 3)
 const bool Recording=Work.ContainsByPredicate([](const FJob& J){return J.Stage==EJobStage::Integrating||(J.bControl&&J.ControlAt<8);});
 if(Recording)Text+=TEXT("\nBLOCKED / Finish the active observation or internal control before waiting.");
 const FString Block=Director?Director->RestBlock():FString();
 if(!Block.IsEmpty())Text+=TEXT("\nBLOCKED / ")+Block;
 else if(Director&&Director->Busy())Text+=TEXT("\nOUTSIDE / Whatever is going on now will be over by then.");
 if(RulesLive()&&Tutorial==0)Text+=FString::Printf(TEXT("\nSTANDBY / Mains and heating with nobody on shift: $%d. Waived if the budget is short."),StandbyFor(Target-WorldClock));
 if(Field)
 {
  const bool Generator=Field->GeneratorRunning();
  Text+=Generator?FString::Printf(TEXT("\nGENERATOR / Running, %.2f hours left. Waiting uses %.2f hours of fuel%s."),Field->GeneratorHours(),FMath::Min(Hours,Field->GeneratorHours()),Hours>=Field->GeneratorHours()?TEXT(" and empties the tank"):TEXT("")):TEXT("\nGENERATOR / Stopped; no generator fuel is used.");
  Text+=PowerFault()?TEXT("\nMAINS / Fault remains. If backup fuel runs out, station power is lost."):TEXT("\nMAINS / Existing supply state is retained.");
  const auto& Logger=Field->FieldLogger();
  Text+=Logger.bRecording?FString::Printf(TEXT("\nFIELD LOGGER / Recording with %.2f battery hours left. The skipped interval is an explicit unsampled gap and consumes battery; it cannot count as a valid continuous comparison."),Logger.Battery/3600.):TEXT("\nFIELD LOGGER / Not recording; no field samples or tape coverage will be created.");
 }
 if(CaseField&&CaseField->Prepared())Text+=TEXT("\nFIELD TEST / A skipped observation window is not measured. Roof listening or gate monitoring cannot be completed by waiting.");
 return Text;
}
bool FS47FreeformSession::WakeBefore(double Target,double& Out)
{
 if(Tutorial>0||!Director||!FarmSite||!M||!M->Flow||(M->Flow->bDevRun&&!bTestWake&&!bTestRules))return false;
 // Behind a motel door nothing does, but the first big night (PT-BIGNIGHT-20261109, Tom): it wakes whoever sleeps anywhere on its
 // night, the motel too, so that it comes while the player is there (FS47Happenings::BigWakes, FS47MotelNight::BigWakeAt). On a cot
 // out on the land, on any night with anything abroad, it will.
 if(RestPlace==ERest::Motel)
 {
  if(!Happenings)return false;
  for(double Midnight=FMath::FloorToDouble(WorldClock/SecondsPerDay)*SecondsPerDay;Midnight<Target;Midnight+=SecondsPerDay)
  {
   const int32 Night=NightOf(Midnight);
   if(Night<2||Night==WokenNight||!Happenings->BigWakes(Night))continue;
   const double At=FS47MotelNight::BigWakeAt(Midnight,Happenings->BigPlan().Hour,WorldClock,Target,FMath::FRand());
   if(At<0)continue;
   WokenNight=Night;RousedNight=Night;Out=At;
   UE_LOG(LogTemp,Display,TEXT("S47: a night at the motel is broken for the big night %d"),Night);
   return true;
  }
  return false;
 }
 // Each night's own start, not a fixed two in the morning. A calm night wakes nobody: its dice are not even thrown.
 for(double Midnight=FMath::FloorToDouble(WorldClock/SecondsPerDay)*SecondsPerDay;Midnight<Target;Midnight+=SecondsPerDay)
 {
  const int32 Night=NightOf(Midnight);
  const double Two=Midnight+BodyStartHour(Night)*3600.;
  if(Two<=WorldClock)continue;
  if(Two>=Target)break;
  if(Night<2||Night==WokenNight||QuietNight(Night))continue;
  if(RestPlace==ERest::Field)
  {
   WokenNight=Night;RousedNight=Night;Out=Two+FMath::FRandRange(60.f,900.f);
   if(Out>=Target)continue;
   FarmSite->TestFieldRoll(0.f); // Whatever it was is still out here: the night's field draw is theirs.
   return true;
  }
  // The night's own dice, thrown here because the scheduler would have slept through them. Spent whatever they say.
  WokenNight=Night;
  // Never two nights running: a night that always ends the same way is no longer a night.
  if(RousedNight>0&&Night-RousedNight<=1)continue;
  const double Wake=Two+FMath::FRandRange(300.f,3600.f);
  const bool bDue=Director->DrawForNight(Night,Wake)||FarmSite->WillStandAt(Two);
  if(!bDue)continue;
  Out=Wake;
  if(Out<Target){RousedNight=Night;return true;}
 }
 return false;
}
bool FS47FreeformSession::WaitUntil(double Target)
{
 if(!bActive||bDead||bRecovering||!FMath::IsFinite(Target)||Target-WorldClock>2*SecondsPerDay)return false;
 if(Target<=WorldClock){Feedback=TEXT("That waiting time has already passed. Check the current window on CASE BOARD; no time was skipped.");Changed(Feedback);return false;}
 if(Held()&&!TerminalOnGlass())return false;
 // Only what is close or after the player stops this, and it says so. Lights in the distance do not.
 if(const FString Block=Director?Director->RestBlock():FString();!Block.IsEmpty()){Feedback=Block;Changed(Feedback);return false;}
 if(Work.ContainsByPredicate([](const FJob& J){return J.Stage==EJobStage::Integrating;})){Feedback=TEXT("A recording is running at the receiver. Let it finish before you wait.");Changed(Feedback);return false;}
 if(Work.ContainsByPredicate([](const FJob& J){return J.bControl&&J.ControlAt<8;})){Feedback=TEXT("A control is running at the receiver rack. Let it finish before you wait.");Changed(Feedback);return false;}
 if(Director)Director->EndForRest();
 double WokenAt=0;const bool bWoken=WakeBefore(Target,WokenAt);
 // Standby: mains and heating with nobody on shift. What is not there is waived.
 int32 Standby=0;
 if(RulesLive()&&Tutorial==0&&RestPlace==ERest::Station){Standby=FMath::Min(Cash,StandbyFor((bWoken?WokenAt:Target)-WorldClock));Cash-=Standby;StandbyTotal+=Standby;}
 const double SleptFrom=WorldClock;
 S47SaroWallWatch::Mark(M,TEXT("before rest")); // WORLD-04 (N1110)
 WorldClock=bWoken?WokenAt:Target;DayNumber=FMath::Max(1,int32(FMath::FloorToDouble((WorldClock-8*3600.)/SecondsPerDay))+1);AddDay(DayNumber);LapseUntakenCards();Settle=0;bAudioPlaying=false;
 // FATIGUE-20261108: the rest was sleep, where it was had (a cot sleeps worst)
 if(Fatigue)Fatigue->Slept(WorldClock-SleptFrom,RestPlace==ERest::Motel?FS47Fatigue::EBed::Motel:RestPlace==ERest::Field?FS47Fatigue::EBed::Cot:FS47Fatigue::EBed::Station);
 // Settle unsampled time before any save request, even while the confirmation
 // screen is still modal. No large Dt is passed through observing work.
 if(Field)Field->AdvanceUnobservedTo(WorldClock);
 if(CaseField)CaseField->AdvanceUnobservedTo(WorldClock);
 if(M&&M->Story)M->Story->SetClock(FMath::Fmod(WorldClock,SecondsPerDay));
 if(M&&M->UI)M->UI->Close();
 S47SaroWallWatch::Mark(M,TEXT("after rest")); // WORLD-04 (N1110)
 if(bWoken)
 {
  WakeClock=WorldClock;
  // A call on the set about the marks if there are marks, otherwise only a sound outside that ends the sleep. (At the motel only the
  // big night wakes anyone: no call about the farm's marks there.)
  const bool bCalled=RestPlace!=ERest::Motel&&FarmSite&&FarmSite->WakeCall();
  if(!bCalled&&M&&M->Audio&&M->Player)M->Audio->Sfx(TEXT("alienRadio"),FS47Sound{.Gain=.45f,.Rate=.8f,.At=FVector(M->Player->Pos.X-6,M->Player->FloorY+2,M->Player->Pos.Z-9)});
  const int32 Secs=int32(FMath::Fmod(WorldClock,SecondsPerDay));
  Feedback=RestPlace==ERest::Field?FString::Printf(TEXT("%02d:%02d. Wide awake. Nothing by the truck that you can see."),Secs/3600,(Secs/60)%60):FString::Printf(TEXT("%02d:%02d. Something woke you. Wait again when you are ready to go on."),Secs/3600,(Secs/60)%60);
  UE_LOG(LogTemp,Display,TEXT("S47: rest broken at clock %.0f, day %d, call %d"),WorldClock,DayNumber,bCalled);
  Changed(Feedback);return true;
 }
 Feedback=(Standby>0?FString::Printf(TEXT("Waiting ended. Standby $%d. "),Standby):FString(TEXT("Waiting ended. ")))+TEXT("Unfinished work and originals are retained. ")+NextAction();Changed(Feedback);return true;
}
void FS47FreeformSession::OpenWaitConfirmation(double Target)
{
 if(!M||!M->UI)return;
 TArray<S47Kit::FW> Rows;
 Rows.Add(SNew(SBox).HeightOverride(300)[SNew(SScrollBox)+SScrollBox::Slot()[T::Paper(WaitConsequences(Target))]]);
 Rows.Add(T::Button(TEXT("Wait until the stated time"),[this,Target]{if(!WaitUntil(Target))OpenOperations();},true));
 Rows.Add(T::Button(TEXT("Keep working / do not advance time"),[this]{OpenOperations();}));
 OpenTerminal(3,TEXT("OPS / CONFIRM WAIT"),OpsShell(0,S47Kit::Col(Rows)));
}

bool FS47FreeformSession::FocusFieldTest(int32 Line)
{return PlannedFieldTest(Line)>=0&&FocusFieldCase(Line);}
bool FS47FreeformSession::FocusFieldCase(int32 Line)
{
 const auto* C=Cases.Get(Line);if(!C||!C->bOpen||C->bConcluded||Tutorial>0||FS47FreeformCases::Def(Line).Observable!=FS47FreeformCases::EObservable::FieldTrace)return false;
 const FJob* J=CurrentJob();
 if(J&&(J->Stage==EJobStage::Integrating||(J->bControl&&J->ControlAt<8)))
 {Changed(TEXT("Finish the active recording or control before following the field case. The plan is retained."));return false;}
 FieldFocus=Line;bGuideFocus=false;bAudioPlaying=false;
 Changed(FieldNextAction(Line));return true;
}
FString FS47FreeformSession::FieldNextAction(int32 Line)const
{
 const int32 Test=PlannedFieldTest(Line);
 if(Test<0)return TEXT("OPS / CASE BOARD: READ THE FIELD RESULTS SO FAR AND PLAN THE NEXT CONTROL. A TEST LEFT HALF DONE SHOWS NOTHING EITHER WAY.");
 const FString Unavailable=FS47FreeformCases::UnavailableReason(Line,Test,HasKit(TEXT("logger2")));
 if(!Unavailable.IsEmpty())return Unavailable+TEXT(" Cancel this retained plan to choose another control.");
 if(Line==2&&CaseField){const FString Actual=CaseField->Objective();if(!Actual.IsEmpty())return Actual;}
 const auto& T=FS47FreeformCases::Def(Line).Tests[Test];
 // D48-20261110 (D46-24): the field map marks where to set the trip camera out
 FVector2D MarkRoad;FString MarkTitle;
 const bool bMarked=Line==FS47FreeformCases::LandLine&&Test==FS47FreeformCases::LandCamera&&TripCamera&&TripCamera->SetOutPlace(MarkRoad,MarkTitle);
 return TEXT("FIELD TEST / ")+FString(T.Name).ToUpper()+TEXT(": ")+T.Instruction+(bMarked?TEXT(" Marked on your map."):TEXT(""));
}
// ---- LOOP-WHOLE-20261109, step 2: OUT ON THE LAND's controls, with the kit there is (FS47FreeformCases::LandLine) -----------------
FString FS47FreeformSession::LandTestNeeds(int32 Line,int32 Test)const
{
 if(Line!=FS47FreeformCases::LandLine)return FString();
 if(Test==FS47FreeformCases::LandCamera&&!(Field&&Field->Workshop().IsBuilt(TEXT("trip_camera"))))return TEXT("the trip camera, built at the workshop on the east wing bench");
 if(Test==FS47FreeformCases::LandNeedle&&!HasKit(TEXT("magnetometer")))return TEXT("the survey magnetometer, bought on OPS / SUPPLIES");
 return FString();
}
void FS47FreeformSession::LandRecord(const FString& Id)
{
 constexpr int32 Line=FS47FreeformCases::LandLine;const int32 Test=PlannedFieldTest(Line);
 if(Test<0||FS47FreeformCases::IsCaseRecord(Id))return;
 const FString Text=Findings.FindRef(Id);
 if(Test==FS47FreeformCases::LandCamera)
 {
  // Read in the morning (FS47TripCamera::Collect keeps "tripcam_d<night>" while it still knows what it caught). Its frames are the
  // result, not the hidden cause: a light or a shape is yes, nothing or an animal is no. A night before the plan does not count.
  if(!Id.StartsWith(TEXT("tripcam_d")))return;
  const int32 Night=FCString::Atoi(*Id.Mid(9));const auto* C=Cases.Get(Line);
  const int32 Planned=C?FCString::Atoi(*C->PlannedJob.Mid(7)):0; // FIELD-D0012-...
  if(Night<Planned)return;
  const bool bSeen=TripCamera&&TripCamera->CaughtTonight()!=FS47TripCamera::ECatch::None;
  // T16-20: a blank roll only counts when the camera really stood watch; else the test is not answered (not valid), and may be set again.
  const bool bCovered=bSeen||!TripCamera||TripCamera->CoveredEnough();
  ReportFieldTest(Line,Test,bCovered,bSeen,Text);return;
 }
 if(Test==FS47FreeformCases::LandNeedle)
 {
  // A reading at a place out on the land (FS47Places: "place_<id>_needle", kept once, read every time): set against the survey sheet.
  if(!Id.StartsWith(TEXT("place_"))||!Id.EndsWith(TEXT("_needle"))||Id==TEXT("place_motel_needle"))return;
  ReportFieldTest(Line,Test,true,FieldTestExpectsPresent(Line,Test),Text);
 }
}
void FS47FreeformSession::StepLandTest()
{
 constexpr int32 Line=FS47FreeformCases::LandLine;
 if(PlannedFieldTest(Line)!=FS47FreeformCases::LandLogger||!Field)return;
 const auto* C=Cases.Get(Line);int32 From=0;
 if(int32 At=C?C->PlannedJob.Find(TEXT("-TAPE")):INDEX_NONE;At!=INDEX_NONE)From=FMath::Max(0,FCString::Atoi(*C->PlannedJob.Mid(At+5)));
 const auto& Tapes=Field->FieldLogger().Tapes;
 for(int32 I=From;I<Tapes.Num();++I)
 {
  const auto& T=Tapes[I];if(!T.bRetrieved)continue;
  // A full window at night, unbroken: then the tape says something about that night out on the land. Anything less is no result.
  const double Hour=FMath::Fmod(T.Start,86400.)/3600.;
  const bool bNight=Hour>=19||Hour<5;
  // CASETEST-20261109: up to FS47FreeformCases::UnwatchedAllowance of it may have gone unwatched (a wait, a drive); the board says why not.
  const bool bFull=T.End-T.Start>=FS47FieldLogger::Window-.001;
  const bool bValid=bNight&&bFull&&FS47FreeformCases::WindowKept(FS47FieldLogger::Window,T.Seconds,T.GapSeconds)&&!T.bBatteryEmpty;
  const FString Why=bValid?FString():!bNight?FString(TEXT(": not started at night (19:00 to 05:00)")):T.bBatteryEmpty?FString(TEXT(": the battery ran out")):!bFull?FString(TEXT(": retrieved before its full two hours"))
   :FString::Printf(TEXT(": %.0f minutes of it went by unwatched, more than the %.0f allowed"),T.GapSeconds/60.,FS47FreeformCases::UnwatchedAllowance/60.);
  const FString Evidence=FString::Printf(TEXT("Field logger tape %s, from %s, %.0f minutes%s."),*T.Id,*FS47FreeformCases::Clock(T.Start),T.Seconds/60.,*Why);
  ReportFieldTest(Line,FS47FreeformCases::LandLogger,bValid,bValid&&FieldTestExpectsPresent(Line,FS47FreeformCases::LandLogger),Evidence);
  return;
 }
}
bool FS47FreeformSession::SetCaseHunch(int32 Line,int32 Cause)
{
 if(!Cases.SetHunch(Line,Cause))return false;
 Changed(TEXT("Hunch noted. It's only a note for you: it doesn't count as a result and doesn't pay."));return true; // D45-20261110: plain words
}
void FS47FreeformSession::ClearCompletedSelection()
{
 if(CurrentJob()&&CurrentJob()->Stage==EJobStage::Filed){Selected=-1;bAudioPlaying=false;AudioCursor=0;}
}
FString FS47FreeformSession::StationGoalLine()const
{
 if(!Field||!Field->ArrayWear().bActive)return FString();
 const auto* Next=Field->ArrayWear().NextBranch();if(!Next)return FString();
 const FString Missing=Field->BranchNeeds(*Next);
 const FString Parts=Missing.IsEmpty()?FString(TEXT("YOU HAVE THE PARTS.")):TEXT("NEEDS ")+Missing.ToUpper()+TEXT(".");
 // M7-HUD-20261109: not out to the antennas in the small hours (Dale: "after two... be in the building")
 // D45-20261110: the live count (ArrayUpCount), and what this repair brings back
 if(GoalWaits(WorldClock))return FString::Printf(TEXT("STATION GOAL / ARRAY %d OF 27: THE %s AT ANTENNA %s (+%d ANTENNAS) WAITS FOR DAYLIGHT. %s"),ArrayUpCount(),Next->Title,Next->Junction,Next->Dishes.Num(),*Parts);
 return FString::Printf(TEXT("STATION GOAL / ARRAY %d OF 27: REPAIR THE %s AT ANTENNA %s (+%d ANTENNAS). %s"),ArrayUpCount(),Next->Title,Next->Junction,Next->Dishes.Num(),*Parts);
}
bool FS47FreeformSession::EveningGated(const FJob& J)const
{
 // From day two every card is the evening's, and one not begun by half past eleven waits for the next evening. Day one is the
 // first shift's. Once begun a card runs out whenever it was begun, and a case test keeps its own window.
 if(!J.bGenerated||J.Stage!=EJobStage::New||J.Day<2||J.CaseTest>=0||Tutorial>0)return false;
 if(M&&M->Flow&&M->Flow->bDevRun&&!bTestEvening)return false;
 // LOOP-NIGHT-WATCH-20261113: on the shift's clock, so that the night watch's window (01:15 to 04:00) is one window after midnight
 const double Now=FS47FreeformJobs::ShiftClock(FMath::Fmod(WorldClock,SecondsPerDay));
 return Now<EveningUp(J)||Now>=EveningShut(J);
}
const FS47FreeformSession::FJob* FS47FreeformSession::NextEveningCard(double& Up)const
{
 // the midnight before the shift day began (08:00 to 08:00): its cards' times on the shift's clock count from there
 const FJob* Best=nullptr;Up=0;const double Base=FMath::FloorToDouble((WorldClock-8*3600.)/SecondsPerDay)*SecondsPerDay;
 for(const FJob& J:Work)if(EveningGated(J))
 {
  double At=Base+EveningUp(J);if(At<=WorldClock)At+=SecondsPerDay;
  if(!Best||At<Up){Best=&J;Up=At;}
 }
 return Best;
}
int32 FS47FreeformSession::CardsUp()const
{
 int32 Up=0;
 for(const FJob& J:Work)Up+=J.Stage!=EJobStage::Filed&&!J.bDuplicateRejected&&J.CaseTest<0&&!EveningGated(J)?1:0;
 return Up;
}
int32 FS47FreeformSession::JobPay(const FJob& J)const
{
 if(!RulesLive()||TrainingCard(J))return 20;
 const int32 Pay=PayAfterDays(FMath::Max(0,DayNumber-J.Day))+JobBonus(J);
 // docked (the night the player was taken): half of it, never under the latest card's base
 return J.bDocked?FMath::Max(PayAfterDays(4),Pay/2):Pay;
}
int32 FS47FreeformSession::JobBonus(const FJob& J,FString* Why)const
{
 if(!RulesLive()||TrainingCard(J)){if(Why)Why->Empty();return 0;}
 TArray<FString> For;int32 B=0;
 if(DayNumber-J.Day<=1){B+=OnTimeBonus;For.Add(TEXT("being on time"));}
 // a recording made with the newer sound (version 2) is judged by its quality; an older one has nothing to judge it by
 // LOOP-CAREFUL-PAYS-20261113 (→ M3, M2): only for a sky fix the operator made himself; one the assist pulled in earns none (CleanBonusFor)
 const bool bClean=J.Stage>=EJobStage::Recorded&&(J.RecordingSoundVersion<2||J.RecordingQuality>=CleanRecordQuality);
 if(bClean&&CleanBonusFor(J)){B+=CleanBonus;For.Add(TEXT("a clean recording"));}
 const bool bByAssist=bClean&&!CleanBonusFor(J);
 if(J.bAnomaly&&(J.bControl||J.bAudioReviewed)){B+=ControlBonus;For.Add(TEXT("checking the anomaly"));} // GAMEPLAY-LOOP-01: the reel check is the extra-tick take's
 if(Why)*Why=B>0?FString::Printf(TEXT(" ($%d of it for %s%s)"),B,*JoinAnd(For),bByAssist?TEXT("; no clean bonus, the sky fix was by assist"):TEXT(""))
  :bByAssist?FString(TEXT(" (no bonus: late, and the sky fix was by assist)")):FString(TEXT(" (no bonus: late, and the recording does not read)"));
 return B;
}
int32 FS47FreeformSession::CardsTonight()const{return DeskNight==NightOf(WorldClock)?DeskCards:0;}
void FS47FreeformSession::DeskWork(const FJob& J)
{
 DeskLine.Empty();
 if(!RulesLive())return;
 // the card's time: from the start of the acquisition to now, as the night's clock has it (pointing and walking not counted)
 // (CRIT6-20261109: begun on an earlier night, or unknown, it has no minutes of its own: DeskText)
 const bool bKnown=J.ReferenceClock>0&&J.ReferenceClock<=WorldClock;
 const bool bEarlier=bKnown&&NightOf(J.ReferenceClock)!=NightOf(WorldClock);
 const int32 Minutes=bKnown?FMath::Clamp(int32((WorldClock-J.ReferenceClock)/60.),0,6000):0;
 if(DeskNight!=NightOf(WorldClock)){DeskNight=NightOf(WorldClock);DeskCards=0;}
 const int32 Tonight=++DeskCards;
 const double Hours=Tonight>=3?DeskHoursThirdCard:DeskHoursPerCard;
 if(Fatigue)Fatigue->Desk(Hours);
 const double Hour=FMath::Fmod(WorldClock,SecondsPerDay)/3600.;
 DeskLine=DeskText(bKnown,bEarlier,Minutes,Tonight,Hour);
 UE_LOG(LogTemp,Display,TEXT("S47: card %s filed: %d min, %d tonight, desk %.2f h"),*J.Id,Minutes,Tonight,Hours);
}
int32 FS47FreeformSession::SideLeftThisWeek()const
{
 const int32 Week=(DayNumber-1)/7+1;return SideWeek==Week?FMath::Max(0,SideCap-SidePaid):SideCap;
}
int32 FS47FreeformSession::EarnSide(int32 Amount,const TCHAR* Kind)
{
 if(Amount<=0)return 0;
 if(!RulesLive()){Earn(Amount);return Amount;}
 const int32 Week=(DayNumber-1)/7+1;
 if(SideWeek!=Week){SideWeek=Week;SidePaid=0;}
 const int32 Pay=FMath::Min(Amount,SideCap-SidePaid);
 UE_LOG(LogTemp,Display,TEXT("S47: side money %s: $%d offered, $%d paid, $%d of $%d this week"),Kind?Kind:TEXT("?"),Amount,FMath::Max(0,Pay),SidePaid+FMath::Max(0,Pay),int32(SideCap));
 if(Pay<=0)return 0;
 SidePaid+=Pay;Earn(Pay);return Pay;
}
void FS47FreeformSession::BreakGear(const FString& What,int32 Fee)
{
 Fee=FMath::Clamp(Fee,0,int32(RepairCap));if(Fee<=0)return;
 Repairs=FMath::Min(int32(RepairCap),Repairs+Fee);
 RecordEvidence(FString::Printf(TEXT("broken-%s-d%d"),*What.Replace(TEXT(" "),TEXT("_")),DayNumber),FString::Printf(TEXT("Day %d: the %s gave out close to something out there. $%d for the repair goes on the next weekly sheet."),DayNumber,*What,Fee));
 UE_LOG(LogTemp,Display,TEXT("S47: gear broken near an event: %s, $%d on the sheet (%d due)"),*What,Fee,Repairs);
 Changed();
}
FString FS47FreeformSession::NextReviewText()const
{
 const FWeekCosts X=WeekCosts(ReviewWeek+1);
 return FString::Printf(TEXT("DAY %d / $%d SO FAR"),NextReviewDay(),ReviewPowerNow()+ReviewFuel+X.Total()+Carried);
}
FS47FreeformSession::FWeekCosts FS47FreeformSession::WeekCosts(int32 Week)const
{
 FWeekCosts X;
 if(!RulesLive()||Week<1)return X;
 const int32 First=(Week-1)*7+1,Last=Week*7;
 if(Care){X.Km=Care->WeekDriven()/1000.;X.Wear=FMath::Min(WearCap,int32(X.Km/2.)*WearPerTwoKm);}
 if(M&&M->Night&&M->Night->Album)for(const FS47AlbumShot& P:M->Night->Album->Shots())
 {
  if(P.LabStage!=3||P.bUnavailable)continue; // developed at the wet bench: the chemicals were used
  const int32 On=FMath::Max(1,int32(FMath::FloorToDouble((P.Clock-8*3600.)/SecondsPerDay))+1);
  if(On>=First&&On<=Last)++X.Prints;
 }
 X.Film=FMath::Min(FilmCap,X.Prints*FilmPerPrint);
 for(const FJob& J:Work)if(J.CaseTest<0&&J.Stage==EJobStage::Filed&&J.Day>=First&&J.Day<=Last&&J.RecordingSoundVersion>=2&&J.RecordingQuality<PoorRecordQuality)++X.Poor;
 X.Fee=FMath::Min(FeeCap,X.Poor*PoorRecordFee);
 if(Tape)for(const auto& T:Tape->Takes())if(T.Day>=First&&T.Day<=Last)++X.Tapes;
 X.Tape=FMath::Min(TapeCap,X.Tapes*TapePerTake);
 X.Repairs=Repairs;
 return X;
}
void FS47FreeformSession::StepWeeklyReview()
{
 if(!RulesLive()||Tutorial>0)return;
 const int32 Week=(DayNumber-1)/7;
 while(ReviewWeek<Week)
 {
  ++ReviewWeek;
  const FWeekCosts X=WeekCosts(ReviewWeek);
  const int32 Before=Carried,Bill=ReviewPowerFor(ReviewWeek,AvailableDishCount())+ReviewFuel+X.Total()+Before;
  const int32 Paid=FMath::Min(Cash,Bill);
  Cash-=Paid;Carried=FMath::Min(ArrearsCap,Bill-Paid);
  if(Care)Care->WeekBilled();
  Repairs=0;
  FString Running=FString::Printf(TEXT("\nTRUCK: TIRES, OIL AND WEAR, %.0f KM $%d\nFILM AND DARKROOM CHEMICALS, %d %s $%d"),X.Km,X.Wear,X.Prints,X.Prints==1?TEXT("PRINT"):TEXT("PRINTS"),X.Film);
  Running+=FString::Printf(TEXT("\nFIELD ALLOWANCE FROM THE UNIVERSITY, FIXED AT $%d A WEEK, NOT A DOLLAR MORE. RECEIVED $%d THIS WEEK"),int32(SideCap),SideWeek==ReviewWeek?SidePaid:0); // CHAIN-5-20261109
  if(X.Tapes>0)Running+=FString::Printf(TEXT("\nCASSETTES, %d %s $%d"),X.Tapes,X.Tapes==1?TEXT("TAKE"):TEXT("TAKES"),X.Tape);
  if(X.Repairs>0)Running+=FString::Printf(TEXT("\nREPAIRS TO GEAR DAMAGED IN THE FIELD $%d"),X.Repairs);
  if(X.Poor>0)Running+=FString::Printf(TEXT("\nDR. WARD: RE-CHECK OF %d %s NOBODY CAN READ $%d. Tune before you record."),X.Poor,X.Poor==1?TEXT("RECORDING"):TEXT("RECORDINGS"),X.Fee);
  RecordEvidence(FString::Printf(TEXT("week-review-%d"),ReviewWeek),FString::Printf(TEXT("SARO OPERATIONS / WEEKLY REVIEW / WEEK %d\nCOUNTY LINE AND FAULT TARIFF, %d ANTENNAS OUT OF SERVICE AT $%d.%02d $%d\nFUEL AND STORES ACCOUNT $%d%s\nCARRIED FROM LAST REVIEW $%d\nPAID FROM THE BUDGET $%d\nCARRIED TO THE NEXT REVIEW $%d\nWhat is carried stays on this sheet. It does not touch the requisition and nothing is cut off.%s"),ReviewWeek,27-AvailableDishCount(),UpkeepHalves(ReviewWeek)/2,UpkeepHalves(ReviewWeek)%2*50,ReviewPowerFor(ReviewWeek,AvailableDishCount()),ReviewFuel,*Running,Before,Paid,Carried,Predecessor?*Predecessor->ReviewLine():TEXT("")));
  // The statement comes by post as well (FS47Mail): the same figures, on paper.
  if(Mail)Mail->PostWeekStatement(ReviewWeek);
  Changed(FString::Printf(TEXT("Weekly review: $%d paid%s. Next one day %d."),Paid,Carried>0?*FString::Printf(TEXT(", $%d carried"),Carried):TEXT(""),NextReviewDay()));
  UE_LOG(LogTemp,Display,TEXT("S47: weekly review %d: bill %d, paid %d, carried %d, cash %d"),ReviewWeek,Bill,Paid,Carried,Cash);
 }
}
bool FS47FreeformSession::BillTow(int32 Fee)
{
 Fee=FMath::Max(0,Fee);
 // The county's bill follows by post (FS47Mail): what was charged, and how.
 if(Cash>=Fee){Cash-=Fee;if(Mail)Mail->PostTowBill(Fee,true);return true;}
 Carried=FMath::Min(int32(ArrearsCap),Carried+Fee);if(Mail)Mail->PostTowBill(Fee,false);return false;
}
bool FS47FreeformSession::CameraLeftOut()const{return Gear&&Gear->IsLeft(TEXT("camera"));}
FString FS47FreeformSession::TakenCosts(double SafeClock)
{
 if(!RulesLive())return FString();
 const int32 Then=NightOf(SafeClock);int32 Docked=0,Missed=0,Unpointed=0;
 const double Base=FMath::FloorToDouble(SafeClock/SecondsPerDay)*SecondsPerDay;
 for(FJob& J:Work)
 {
  if(J.CaseTest>=0||J.bPaid||J.Stage==EJobStage::Filed)continue;
  if((J.Day==Then||J.Day==DayNumber)&&!J.bDocked){J.bDocked=true;++Docked;}
  // The array stood idle, and the shift must point it again. What was recorded and kept stays.
  if(J.Stage==EJobStage::Pointed){J.Stage=EJobStage::New;J.Elapsed=0;++Unpointed;}
  if(J.bGenerated&&J.Day>=2&&J.Stage==EJobStage::New)for(int32 Offset=-1;Offset<=1;++Offset)
  {
   const double Open=Base+Offset*SecondsPerDay+EveningUp(J),Shut=Base+Offset*SecondsPerDay+EveningShut(J);
   if(Open<WorldClock&&Shut>SafeClock){++Missed;break;}
  }
 }
 Settle=0;bCrossRunning=false;
 const int32 Lost=FS47FreeformCases::LostLine;
 if(Cases.Open(Lost,DayNumber,FMath::Fmod(WorldClock,SecondsPerDay)))Changed(TEXT("New case opened: ")+FString(FS47FreeformCases::Def(Lost).Title)+TEXT(". See the OPS case board."));
 UE_LOG(LogTemp,Display,TEXT("S47: taken costs: docked %d, windows gone by %d, array let go %d"),Docked,Missed,Unpointed);
 FString Text;
 if(Missed>0)Text+=FString::Printf(TEXT(" %d observation %s went by with nobody at the receiver."),Missed,Missed==1?TEXT("window"):TEXT("windows"));
 if(Docked>0)Text+=TEXT(" The time sheet has a hole in it: the day's unpaid work goes in at half rate.");
 return Text;
}
void FS47FreeformSession::LostTimeTrace(int32 Clue)
{
 if(!RulesLive())return;
 const int32 Lost=FS47FreeformCases::LostLine;const auto* Y=FS47FreeformCases::Def(Lost).Mystery;const auto* C=Cases.Get(Lost);
 if(!Y||!C||!C->bOpen||Clue<0||Clue>=FS47FreeformCases::ClueCount)return;
 RecordEvidence(Y->Clues[Clue].Finding,Cases.ClueText(Lost,Clue));
}
FString FS47FreeformSession::KitWaits(const FString& Id)const
{
 const auto* O=FS47Openings::Def(Id);if(!O)return TEXT("nothing");
 const bool bOpen=Id==TEXT("spotlight")?(!Field||!Field->ArrayWear().bActive||!Field->ArrayWear().Restored.IsEmpty())
  :Id==TEXT("cot")?FindingCount()>0
  // The first field work away from the station opens it; being taken is not the way in (critic, 8 November). An assessment on
  // THE LOST HOURS still opens it, and makes it cheaper (KitPrice).
  :Id==TEXT("magnetometer")?(FieldRecordKept()||Cases.HasAccess(TEXT("access.lostHours")))
  :Id==TEXT("logger2")?Cases.HasAccess(TEXT("access.fieldlogger"))
  :Id==TEXT("film")?HasFinding(TEXT("body_photo"))
  :Id==TEXT("geiger_tube")?FieldRecordKept()
  // M7 (C): the dearer kit of weeks three and four, once there is money to spend and a reason to
  :Id==TEXT("transfer")?DayNumber>=15
  :Id==TEXT("array_alarm")?(DayNumber>=15&&HasKit(TEXT("alarm")))
  :Id==TEXT("alarm")?SheriffTrust()>0
  // M7 (Tom, 9 November): the truck's upgrades and the better field gear from week 2, the direction finder from week 3
  // CHAIN-2-20261109: what the field brings in opens it, money buys it. The truck's from day 8 with a record from out there; the engine
  // with two kinds of night record; the direction finder from day 15 with three, or a field time that matched the array.
  :Id==TEXT("truck_tires")||Id==TEXT("truck_rack")||Id==TEXT("truck_tank")?(DayNumber>=FS47Openings::FirstDay(Id)&&FieldRecordKept())
  :Id==TEXT("truck_engine")?(DayNumber>=FS47Openings::FirstDay(Id)&&NightRecordKinds()>=2)
  :Id==TEXT("field_recorder")?(DayNumber>=FS47Openings::FirstDay(Id)&&Tape&&Tape->Takes().ContainsByPredicate([](const FS47Tape::FTake& T){return T.bOutdoors;}))
  :Id==TEXT("night_camera")?(DayNumber>=FS47Openings::FirstDay(Id)&&HasKit(TEXT("film")))
  :Id==TEXT("rdf")?(DayNumber>=FS47Openings::FirstDay(Id)&&(NightRecordKinds()>=3||[this]{for(const auto& Kept:Findings)if(Kept.Key.StartsWith(TEXT("minute_yes_")))return true;return false;}())):false;
 return bOpen?FString():FString(O->Waits);
}
bool FS47FreeformSession::FieldRecordKept()const
{
 for(const auto& Kept:Findings)if(FieldRecordKey(Kept.Key)&&!Kept.Key.StartsWith(TEXT("minute_")))return true;
 return false;
}
bool FS47FreeformSession::FieldRecordKey(const FString& K)
{
 if((K.StartsWith(TEXT("place_"))&&!K.EndsWith(TEXT("_needle")))||K.StartsWith(TEXT("farm_trace"))||K.StartsWith(TEXT("lights_trace_"))||K.StartsWith(TEXT("field_night")))return true;
 // What the night's events left out on the land: the cattle, the cow, the camper, the pickup, the rig, the ring.
 if(FS47Happenings::IsNightFinding(K))return true;
 // CHAIN-2-20261109: a time from the field set against the array (FS47FreeformMinute: minute_yes_, minute_no_)
 return K.StartsWith(TEXT("minute_yes_"))||K.StartsWith(TEXT("minute_no_"));
}
int32 FS47FreeformSession::NightRecordKinds(int32 FromDay)const
{
 TSet<FString> Sorts;
 for(const auto& Kept:Findings)if(FS47Happenings::IsNightFinding(Kept.Key)&&FindingDays.FindRef(Kept.Key)>=FromDay)Sorts.Add(FS47Happenings::NightFindingKind(Kept.Key));
 return Sorts.Num();
}
int32 FS47FreeformSession::KitPrice(const FString& Id)const
{
 const auto* O=FS47Openings::Def(Id);if(!O)return 0;
 if(Id==TEXT("magnetometer")&&Cases.HasAccess(TEXT("access.lostHours")))return FMath::Max(5,O->Price-LostHoursDiscount);
 return O->Price;
}
bool FS47FreeformSession::ReceiveKit(const FString& Id)
{
 if(!FS47Openings::Def(Id)||Kit.Contains(Id))return false;
 Kit.Add(Id);
 if(Id==TEXT("film")&&M&&M->Night&&M->Night->Album)M->Night->Album->AddFilm();
 UE_LOG(LogTemp,Display,TEXT("S47: opening come by post: %s"),*Id);
 Changed();return true;
}
TArray<FString> FS47FreeformSession::FindingIdsFrom(int32 FromDay)const
{
 TArray<FString> Ids;
 for(const auto& Kept:FindingDays)if(Kept.Value>=FromDay)Ids.Add(Kept.Key);
 Ids.Sort([this](const FString& A,const FString& B){const int32 DA=FindingDays.FindRef(A),DB=FindingDays.FindRef(B);return DA==DB?A<B:DA<DB;});
 return Ids;
}
bool FS47FreeformSession::BuyKit(const FString& Id)
{
 const auto* O=FS47Openings::Def(Id);if(!O||Kit.Contains(Id)||!KitWaits(Id).IsEmpty())return false;
 const int32 Price=KitPrice(Id);
 if(Cash<Price){Feedback=FString::Printf(TEXT("%s costs $%d. The budget has $%d."),O->Title,Price,Cash);Changed(Feedback);return false;}
 Cash-=Price;Kit.Add(Id);
 if(Id==TEXT("film")&&M&&M->Night&&M->Night->Album)M->Night->Album->AddFilm();
 Feedback=FString::Printf(TEXT("%s: delivered and signed for, $%d. %s"),O->Title,Price,O->What);
 UE_LOG(LogTemp,Display,TEXT("S47: opening bought: %s for $%d, cash %d"),*Id,Price,Cash);
 Changed(Feedback);return true;
}
int32 FS47FreeformSession::MotelPrice()const{return Mail?Mail->MotelPrice(MotelRoom):MotelRoom;}
bool FS47FreeformSession::RestAt(double Target,ERest Where,bool bPaid)
{
 // The price is the one at the desk tonight, before the night has gone by and the Ledger's coupon with it.
 const int32 Room=Where==ERest::Motel&&!bPaid?MotelPrice():0;
 if(Where==ERest::Motel&&Cash<Room)return false;
 RestPlace=Where;
 const bool bGood=WaitUntil(Target);
 if(bGood&&Where==ERest::Motel&&Room>0){Cash-=Room;Changed(FString::Printf(TEXT("A night at the Sierra Motor Court, $%d."),Room));}
 UE_LOG(LogTemp,Display,TEXT("S47: rest away from the station, kind %d: %s"),int32(Where),bGood?TEXT("slept"):TEXT("refused"));
 RestPlace=ERest::Station;return bGood;
}
bool FS47FreeformSession::LiveFlow()const{return M&&M->Flow&&!M->Flow->bDevRun;}
double FS47FreeformSession::BodyStartHour(int32 Night)const{return RulesLive()?FS47NightRhythm::StartHour(JobSeed,Night,TakenCount):2.;}
bool FS47FreeformSession::QuietNight(int32 Night)const{return RulesLive()&&FS47NightRhythm::Quiet(JobSeed,Night);}
bool FS47FreeformSession::NightStarted(double AtClock)const
{
 const double Hour=FMath::Fmod(FMath::Max(0.,AtClock),SecondsPerDay)/3600.;
 return Hour>=BodyStartHour(NightOf(AtClock))&&Hour<FS47NightRhythm::EndHour;
}
bool FS47FreeformSession::TheirHoursNow(double AtClock)const{return !QuietNight(NightOf(AtClock))&&NightStarted(AtClock);}
float FS47FreeformSession::SiteVisitChance()const{return RulesLive()?FS47NightRhythm::VisitChance(DayNumber):.6f;}
bool FS47FreeformSession::FaultDueLater()const
{
 const double Hour=FMath::Fmod(WorldClock,SecondsPerDay)/3600.;
 return RulesLive()&&FS47NightRhythm::FaultDay(JobSeed,DayNumber,HasUpgrade(0))&&Hour<FS47NightRhythm::FaultHour(JobSeed,DayNumber);
}
int32 FS47FreeformSession::RecordsOnDay(int32 OnDay)const
{
 int32 Count=0;for(const auto& Kept:FindingDays)Count+=Kept.Value==OnDay?1:0;return Count;
}
bool FS47FreeformSession::FaultDue()const
{
 const double Hour=FMath::Fmod(WorldClock,SecondsPerDay)/3600.;
 if(!RulesLive())return DayNumber>1&&DayNumber%(HasUpgrade(0)?6:3)==2&&Hour>=10;
 return FS47NightRhythm::FaultDay(JobSeed,DayNumber,HasUpgrade(0))&&Hour>=FS47NightRhythm::FaultHour(JobSeed,DayNumber);
}
FString FS47FreeformSession::EveningLine(const FJob& J)const
{
 // LOOP-NIGHT-WATCH-20261113 (Tom 13 November, after the critics): the hours between the evening's two cards and the night watch are
 // the player's own: the slip an evening anomaly printed, the camera, the town. Never rest before the watch. The first sentence stays
 // short: the workstations' NEXT line (TerminalNextLine) is the place and the time.
 if(NightWatch(J))
 {
  int32 EarlyIn=0;for(const FJob& X:Work)EarlyIn+=X.Day==J.Day&&X.Slot<2&&X.CaseTest<0&&X.bGenerated&&X.Stage>=EJobStage::Registered?1:0;
  const bool bTwoIn=EarlyIn>=2;
  return FString::Printf(TEXT("NIGHT WATCH / %s IN VIEW FROM %s. %sTHE WATCH IS WORKED AT SARO INTO THE SMALL HOURS. TILL THEN THE TIME IS YOURS: %sOUT WITH THE CAMERA, OR INTO TOWN. BE BACK IN THE CONTROL ROOM BY %s."),
   *FString(FS47Observing::Runs()[J.Source].Source).ToUpper(),*FS47FreeformCases::Clock(EveningUp(J)),bTwoIn?TEXT("TONIGHT'S TWO CARDS ARE IN. "):TEXT(""),
   SlipToFollow()?TEXT("THE BEARING SLIP'S CIRCLE ON THE MAP, "):TEXT(""),*FS47FreeformCases::Clock(EveningUp(J)));
 }
 return FString::Printf(TEXT("EVENING OBSERVATION / %s IN VIEW FROM %s. TILL THEN: DAY WORK, REPAIRS, SUPPLIES, OR OUT WITH THE CAMERA. AFTER 02:00 IT IS BETTER INSIDE: ADVICE, NOT A RULE."),*FString(FS47Observing::Runs()[J.Source].Source).ToUpper(),*FS47FreeformCases::Clock(EveningUp(J)));
}
bool FS47FreeformSession::RestSuggestedAt(double AtClock,int32 ShiftDay,bool bLive,bool bWorn)
{
 if(!bLive||ShiftDay<2||bWorn)return true;
 const double Now=FS47FreeformJobs::ShiftClock(FMath::Fmod(FMath::Max(0.,AtClock),SecondsPerDay));
 return Now<EveningOpens||Now>=26.5*3600.;
}
bool FS47FreeformSession::NightBeforeGoal()const
{
 const double Now=FS47FreeformJobs::ShiftClock(FMath::Fmod(FMath::Max(0.,WorldClock),SecondsPerDay));
 if(Now<EveningOpens||Now>=NightWatchCloses)return false;
 return Work.ContainsByPredicate([](const FJob& J){return J.bGenerated&&J.CaseTest<0&&J.Day>=2&&J.Stage!=EJobStage::Filed&&!J.bDuplicateRejected;});
}
bool FS47FreeformSession::SlipToFollow()const
{
 if(!Places)return false;
 for(const TPair<FString,bool>& Slip:Places->SlipRows(WorldClock))if(!Slip.Value)return true;
 return false;
}
bool FS47FreeformSession::IsStatusLine(const FString& Line)
{
 // TASKS-TEN-20261113 (change 3): lines that say how things stand, or what waits on somebody else, not something to do: the station's
 // standing, the outgoing post with its flag up, a parcel due, the tired state, a file's logged hours, the card count
 return Line.StartsWith(TEXT("SARO under review"))||Line.StartsWith(TEXT("SARO KEPT OPEN"))||Line.StartsWith(TEXT("SARO CLOSES"))||Line.StartsWith(TEXT("MAIL ORDER /"))
  ||(Line.StartsWith(TEXT("MAILBOX / "))&&Line.EndsWith(TEXT("the flag up for the carrier.")))||Line.StartsWith(TEXT("YOU / "))||Line.StartsWith(TEXT("Job cards:"));
}
bool FS47FreeformSession::LetterWaits(const FString& Line)
{
 return Line.StartsWith(TEXT("MAILBOX / "))||Line.StartsWith(TEXT("OPS / There is a letter"))||Line.StartsWith(TEXT("OPS / Dr. Ward's assessment"));
}
FString FS47FreeformSession::AvailableActivities(FString* Card)const
{
 // M7-HUD-20261109: Dr. Ward's letter (why the player is here at all) or his assessment, once the first shift is done: that comes
 // first, and the day's work after it. Not out to the box at the road in the small hours, when it can wait for the morning.
 if(Predecessor&&Tutorial==0)
 {
  const FString Letter=Predecessor->TaskLine();
  // TASKS-TEN-20261113 (change 10, seen running): a pinned matter is the step; the letter is no paper waiting and does not cut in
  FString PinFull,PinShort;
  if(LetterWaits(Letter)&&!(Letter.StartsWith(TEXT("MAILBOX"))&&GoalWaits(WorldClock))&&!PinnedStep(PinFull,PinShort))
  {
   const FString Then=WorkActivities(nullptr);
   if(Card)*Card=S47HudGauges::CardLine(Letter);
   PickRow=Letter; // TASKS-TEN-20261113: the letter's own row
   return Letter+TEXT(" THEN: ")+Then;
  }
 }
 return WorkActivities(Card);
}
FString FS47FreeformSession::ObjectiveRow()const
{
 PickRow.Reset();
 Objective();
 return PickRow;
}
bool FS47FreeformSession::FieldWindowWaits(int32 Line)const
{
 double Start=0,End=0;
 if(PlannedFieldTest(Line)<0||!CaseTestWindow(Line,Start,End))return false;
 return Line==2&&CaseField&&CaseField->Prepared()&&WorldClock<Start;
}
bool FS47FreeformSession::WindowOpenStep(FString& Next)const
{
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)
 {
  double Start=0,End=0;
  if(PlannedFieldTest(Line)<0||!CaseTestWindow(Line,Start,End))continue;
  if(Line==2&&(!CaseField||!CaseField->Prepared()))continue;
  if(WorldClock>=Start&&WorldClock<End){Next=FieldNextAction(Line);return !Next.IsEmpty();}
 }
 return false;
}
void FS47FreeformSession::DaleSaid(const FString& Words)
{
 if(!bActive||Words.IsEmpty())return;
 // his sentences, as he said them
 TArray<FString> Said;FString Piece;
 for(int32 I=0;I<Words.Len();++I)
 {
  Piece.AppendChar(Words[I]);
  if((Words[I]==TEXT('.')||Words[I]==TEXT('?')||Words[I]==TEXT('!'))&&(I+1==Words.Len()||Words[I+1]==TEXT(' '))){Said.Add(Piece.TrimStartAndEnd());Piece.Reset();}
 }
 if(!Piece.TrimStartAndEnd().IsEmpty())Said.Add(Piece.TrimStartAndEnd());
 const int32 Night=NightOf(WorldClock);
 for(int32 I=0;I<Said.Num();++I)
 {
  const FString Low=Said[I].ToLower();
  FString Kind,Row;
  if(Low.Contains(TEXT("get outside"))||Low.Contains(TEXT("of the station, over "))){Kind=TEXT("look");Row=TEXT("DALE ASKED / OUTSIDE: ")+Said[I];}
  else if(Low.Contains(TEXT("put it on your map")))
  {
   // with where he said it, from the sentence before: "Put it on your map: out at the salt lick, a mile and a half from the station."
   FString Where=I>0?Said[I-1]:FString();Where.RemoveFromEnd(TEXT("."));
   if(!Where.IsEmpty())Where=Where.Left(1).ToLower()+Where.Mid(1);
   Kind=TEXT("map");Row=TEXT("DALE ASKED / MAP: Put it on your map")+(Where.IsEmpty()?FString(TEXT(".")):TEXT(": ")+Where+TEXT("."));
  }
  else if(Low.Contains(TEXT("night log"))){FString Ask=Said[I];Ask.RemoveFromStart(TEXT("And "));Ask=Ask.Left(1).ToUpper()+Ask.Mid(1);Kind=TEXT("log");Row=TEXT("DALE ASKED / NIGHT LOG: ")+Ask;}
  if(Kind.IsEmpty()||!S47Script::LintText(Row).IsEmpty())continue;
  if(DaleAskList.ContainsByPredicate([&](const FDaleAsk& A){return A.Night==Night&&A.Kind==Kind&&!A.bDone;}))continue;
  FDaleAsk A;A.Kind=Kind;A.Row=Row;A.Night=Night;
  A.Base=Kind==TEXT("log")?(NightLog?NightLog->OwnCount():0):Kind==TEXT("map")?int32(S47TonightNote::MapLooks()):0;
  if(DaleAskList.Num()>=12)DaleAskList.RemoveAt(0);
  DaleAskList.Add(A);
  if(NightLog)NightLog->Note(TEXT("Dale asked on the walkie: ")+Said[I]);
  UE_LOG(LogTemp,Display,TEXT("S47: Dale asked (%s): %s"),*Kind,*Row);
 }
}
TArray<FString> FS47FreeformSession::DaleAskRows()const
{
 TArray<FString> Out;
 const int32 Night=NightOf(WorldClock);
 for(const FDaleAsk& A:DaleAskList)if(A.Night==Night&&!A.bDone)Out.Add(A.Row);
 return Out;
}
void FS47FreeformSession::StepDaleAsks()
{
 // gone at 08:00 (the night they were asked in is over); ticked off once done
 const int32 Night=NightOf(WorldClock);
 DaleAskList.RemoveAll([Night](const FDaleAsk& A){return A.Night!=Night;});
 for(FDaleAsk& A:DaleAskList)
 {
  if(A.bDone)continue;
  if(A.Kind==TEXT("look"))A.bDone=M&&M->World&&M->Player&&!M->World->bDriving&&!M->World->Indoors(M->Player->Pos);
  else if(A.Kind==TEXT("map"))A.bDone=int32(S47TonightNote::MapLooks())>A.Base;
  else if(A.Kind==TEXT("log"))A.bDone=NightLog&&NightLog->OwnCount()>A.Base;
  if(A.bDone)Changed();
 }
}
void FS47FreeformSession::PinRow(const FString& Key,const FString& Row)
{
 if(Key.IsEmpty()||Key.Len()>200||Row.Len()>600)return;
 if(PinKey==Key){PinKey.Reset();PinText.Reset();bPinLive=false;}
 else{PinKey=Key;PinText=Row;PinNight=NightOf(WorldClock);bPinLive=true;}
 PinCheckAt=-1;
 UE_LOG(LogTemp,Display,TEXT("S47: pinned row: %s"),PinKey.IsEmpty()?TEXT("(none)"):*PinKey);
 Changed();
 if(M&&M->Flow)M->Flow->RequestAutosave(false);
}
void FS47FreeformSession::StepPin()
{
 if(PinKey.IsEmpty())return;
 // a new night (08:00) clears it, whatever became of it
 if(PinNight!=NightOf(WorldClock)){PinKey.Reset();PinText.Reset();bPinLive=false;Changed();return;}
 if(PinCheckAt>=0&&WorldClock>=PinCheckAt&&WorldClock<PinCheckAt+60)return; // once a game minute is enough
 PinCheckAt=WorldClock;
 bool bLive=false;
 if(PinKey==TEXT("cards"))bLive=CardsUp()>0;
 else if(PinKey.StartsWith(TEXT("card:"))){const FString Id=PinKey.Mid(5);bLive=Work.ContainsByPredicate([&Id](const FJob& J){return J.Id==Id&&J.Stage!=EJobStage::Filed&&!J.bDuplicateRejected;});}
 else if(PinKey==TEXT("matter:station"))bLive=!StationGoalLine().IsEmpty()||ServiceGuide==1;
 else if(PinKey.StartsWith(TEXT("matter:")))bLive=true; // Ward's: until 08:00
 else{const TArray<FS47Task> Now=Tasks();bLive=Now.ContainsByPredicate([this](const FS47Task& T){return T.Text==PinText&&!T.bDone;});}
 if(bLive!=bPinLive){bPinLive=bLive;Changed();}
}
bool FS47FreeformSession::PinnedStep(FString& Full,FString& Short)const
{
 Full.Reset();Short.Reset();
 if(!bPinLive||PinKey.IsEmpty())return false;
 if(PinKey==TEXT("cards"))
 {
  const int32 Up=CardsUp();if(Up<=0)return false;
  Full=FString::Printf(TEXT("OPS (corner PC), DAILY WORK: SELECT ONE OF %d UNFINISHED OBSERVATIONS; TRACK, TUNE AND REGISTER THE NEXT RESULT. COMPLETED RESULTS ARE IN EARLIER WORK."),Up);
  Short=FString::Printf(TEXT("OPS // %d JOB CARD%s UP."),Up,Up==1?TEXT(""):TEXT("S"));return true;
 }
 if(PinKey.StartsWith(TEXT("card:")))
 {
  const FString Id=PinKey.Mid(5);const FJob* J=Work.FindByPredicate([&Id](const FJob& X){return X.Id==Id;});
  if(!J||J->Stage==EJobStage::Filed)return false;
  Full=EveningGated(*J)?EveningLine(*J):JobNextAction(*J);
  if(EveningGated(*J))Short=FString::Printf(TEXT("TRACKING PC // %s AT %s."),*FString(FS47Observing::Runs()[J->Source].Source).ToUpper(),*FS47FreeformCases::Clock(EveningUp(*J)));
  return true;
 }
 if(PinKey==TEXT("matter:station")){Full=ServiceGuide==1?ServiceObjective():StationGoalLine();return !Full.IsEmpty();}
 // a case's row: its step as the case board says it
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line){const FCaseStep S=CaseStep(Line);if(!S.Task.IsEmpty()&&S.Task==PinText){Full=S.Next;Short=S.Card;return !Full.IsEmpty();}}
 Full=PinText;
 return !Full.IsEmpty();
}
FString FS47FreeformSession::CloseSoonLine()const
{
 // the one line beside a pinned step: something else that shuts within 30 minutes
 const double Now=WorldClock,Base=FMath::FloorToDouble((Now-8*3600.)/SecondsPerDay)*SecondsPerDay;
 const auto Within=[Now](double At){return At>Now&&At-Now<=30*60.;};
 if(PinKey!=TEXT("cards"))for(const FJob& J:Work)
 {
  if(J.Stage!=EJobStage::New||J.bDuplicateRejected||J.CaseTest>=0||J.Day<2||EveningGated(J))continue;
  if(const double Shut=Base+EveningShut(J);Within(Shut))return FString::Printf(TEXT("CLOSES %s // A JOB CARD AT OPS."),*FS47FreeformCases::Clock(Shut));
 }
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)
 {
  double Start=0,End=0;
  if(PlannedFieldTest(Line)<0||!CaseTestWindow(Line,Start,End)||Now<Start||(Line==2&&(!CaseField||!CaseField->Prepared())))continue;
  if(Within(End)&&CaseStep(Line).Task!=PinText)return FString::Printf(TEXT("CLOSES %s // THE CASE WINDOW."),*FS47FreeformCases::Clock(End));
 }
 if(SlipToFollow()&&!PinText.StartsWith(TEXT("BEARING SLIP /")))
 {
  double Noon=FMath::FloorToDouble(Now/SecondsPerDay)*SecondsPerDay+12*3600.;if(Noon<=Now)Noon+=SecondsPerDay;
  if(Within(Noon))return TEXT("CLOSES 12:00 // THE BEARING SLIP.");
 }
 return FString();
}
void FS47FreeformSession::LoadNotePlan(const S47J::FObj& Saved)
{
 DaleAskList.Reset();PinKey.Reset();PinText.Reset();PinNight=0;bPinLive=false;PinCheckAt=-1;
 const S47J::FObj Plan=S47J::Obj(Saved,TEXT("notePlan"));if(!Plan.IsValid())return;
 {const FString K=S47J::Str(Plan,TEXT("pin")),T=S47J::Str(Plan,TEXT("pinRow"));if(!K.IsEmpty()&&K.Len()<=200&&T.Len()<=600){PinKey=K;PinText=T;PinNight=S47J::Int(Plan,TEXT("pinNight"));bPinLive=true;}}
 for(const TSharedPtr<FJsonValue>& V:S47J::Arr(Plan,TEXT("daleAsked")))
 {
  const TSharedPtr<FJsonObject>* O=nullptr;if(!V||!V->TryGetObject(O)||!O)continue;
  FDaleAsk A;A.Kind=S47J::Str(*O,TEXT("kind"));A.Row=S47J::Str(*O,TEXT("row"));A.Night=S47J::Int(*O,TEXT("night"));A.Base=S47J::Int(*O,TEXT("base"));A.bDone=S47J::Bool(*O,TEXT("done"));
  if((A.Kind==TEXT("look")||A.Kind==TEXT("map")||A.Kind==TEXT("log"))&&A.Row.StartsWith(TEXT("DALE ASKED / "))&&A.Row.Len()<=400&&DaleAskList.Num()<12)DaleAskList.Add(A);
 }
}
void FS47FreeformSession::SaveNotePlan(const S47J::FObj& Out)const
{
 // nothing kept writes nothing: an older save reads the same
 if(DaleAskList.IsEmpty()&&PinKey.IsEmpty())return;
 const S47J::FObj Plan=S47J::New();
 if(!PinKey.IsEmpty()){S47J::Set(Plan,TEXT("pin"),PinKey);S47J::Set(Plan,TEXT("pinRow"),PinText);S47J::Set(Plan,TEXT("pinNight"),PinNight);}
 TArray<TSharedPtr<FJsonValue>> Asks;
 for(const FDaleAsk& A:DaleAskList){const S47J::FObj O=S47J::New();S47J::Set(O,TEXT("kind"),A.Kind);S47J::Set(O,TEXT("row"),A.Row);S47J::Set(O,TEXT("night"),A.Night);S47J::Set(O,TEXT("base"),A.Base);S47J::Set(O,TEXT("done"),A.bDone);Asks.Add(MakeShared<FJsonValueObject>(O));}
 Plan->SetArrayField(TEXT("daleAsked"),Asks);
 S47J::Set(Out,TEXT("notePlan"),Plan);
}
FString FS47FreeformSession::DeadlineLine()const
{
 // (in the list's own words, so that the note marks that row: FS47FreeformSession::Tasks)
 if(Places)for(const TPair<FString,bool>& Slip:Places->SlipRows(WorldClock))if(!Slip.Value)return Slip.Key;
 if(Director){const FString Reported=Director->ReportedLine();if(!Reported.IsEmpty()&&!Director->ReportDone())return Reported;}
 if(Director)for(const TPair<FString,bool>& Trail:Director->MysteryTrailRows())if(!Trail.Value)return Trail.Key;
 const TArray<FString> Asks=DaleAskRows(); // TASKS-TEN-20261113 (change 7)
 return Asks.IsEmpty()?FString():Asks[0];
}
FString FS47FreeformSession::WorkActivities(FString* Card)const
{
 // TASKS-TEN-20261113 (change 4): the cards up now, one count everywhere (CardsUp); a case test is the case's own row (step 3 below)
 const int32 Pending=CardsUp();const FJob* Evening=nullptr;
 for(const FJob& J:Work)if(J.Stage!=EJobStage::Filed&&!J.bDuplicateRejected&&EveningGated(J)&&!Evening)Evening=&J;
 // LOOP-NIGHT-WATCH-20261113: the one coming up soonest (with two early cards and the night watch, the first in the list is not it)
 if(Evening){double Soon=0;const FJob* Soonest=NextEveningCard(Soon);if(Soonest&&!Soonest->bDuplicateRejected)Evening=Soonest;}
 // The array is what the whole station is working toward: whatever else is on the line, it stays in sight.
 const FString Goal=StationGoalLine();
 const auto* Next=Field&&Field->ArrayWear().bActive?Field->ArrayWear().NextBranch():nullptr;
 // D45-20261110 (Tom: the same number everywhere reads as static text): the count stands once, on the STATION GOAL line of the note and
 // the notebook (StationGoalLine), not after every other task and on every card as well.
 // M7-WALKIE-20261109: once the first shift is done, the field radio on the control room desk is Dale's way to the player and the
 // night's rules; until it is taken every line says where it is. (Out in the field where the player was taken, it is not on the desk.)
 const bool bOnDesk=Tutorial==0&&M&&M->FieldRadio&&M->FieldRadio->Ready()&&!M->FieldRadio->Owned()&&!M->FieldRadio->LeftOut();
 const bool bHall=bOnDesk&&M->FieldRadio->InHall(); // FIX-20261110: on the hall dresser
 const FString Radio=bOnDesk?FString(bHall?TEXT(" HALL DRESSER: TAKE THE FIELD RADIO (DALE, CHANNEL 1)."):TEXT(" CONTROL ROOM DESK: TAKE THE FIELD RADIO (DALE, CHANNEL 1).")):FString();
 // The card: the task in a line it has room for, then the walkie while there is room.
 const FString RadioCard=bOnDesk?FString(bHall?TEXT(" WALKIE: ON THE HALL DRESSER."):TEXT(" WALKIE: ON THE DESK.")):FString();
 const bool bDark=GoalWaits(WorldClock);
 // TASKS-TEN-20261113 (change 2): each answer says which of the night's rows it is (PickRow, ObjectiveRow): by default its own words
 auto Say=[this,Card,&Radio,&RadioCard](const FString& Full,const FString& Short=FString(),const FString& Row=FString())
 {
  if(Card)*Card=S47HudGauges::CardWith(Short.IsEmpty()?Full:Short,{RadioCard});
  PickRow=Row.IsEmpty()?Full:Row;
  return Full+Radio;
 };
 auto SayGoal=[&]()
 {
  const FString Missing=Field->BranchNeeds(*Next);
  const FString Short=Missing.IsEmpty()?FString::Printf(TEXT("ANTENNA %s // REPAIR THE %s: PARTS IN HAND."),Next->Junction,Next->Title)
   :FString::Printf(TEXT("ANTENNA %s // THE %s NEEDS %s."),Next->Junction,Next->Title,*Missing.ToUpper());
  return Say(Goal,Short);
 };
 // 1. BLOCKING NOW. CASES-OPS-20261109: a case's result sheet on the printer: it says what the test showed. LOOP-1-20261109: a bearing
 // slip once it is the printer's turn (a job's paper first). (The power, a take under way and RETAIN are the Objective's and the
 // receiver's own steps, before this ladder: Objective, NextAction.)
 if(!CaseSheets.IsEmpty()&&IsBearingSlip(CaseSheets[0])){if(SheetIndex()==0)return Say(FString(TEXT("PRINTER: COLLECT THE BEARING SLIP.")),TEXT("PRINTER // BEARING SLIP."));}
 else if(!CaseSheets.IsEmpty())return Say(FString::Printf(TEXT("PRINTER: COLLECT THE RESULT SHEET FOR %s."),FS47FreeformCases::Def(CaseSheets[0].Line).Title),TEXT("PRINTER // CASE RESULT SHEET."));
 // TELEX-20261109 (Tom's words on the HUD, "TELEX: A reply is waiting"): once the HUD's second line, now (TASKS-TEN-20261113, change 5)
 // a paper waiting like the printer's
 if(const FString Telex=TelexLine();!Telex.IsEmpty())return Say(TEXT("TELEX (control room): ")+Telex.Mid(7)+TEXT(". TEAR IT OFF AT THE TELEX."),TEXT("TELEX // ")+Telex.Mid(7).ToUpper()+TEXT("."));
 // TASKS-TEN-20261113 (change 10): the pinned matter's next step, once nothing blocks
 {FString PinFull,PinShort;if(PinnedStep(PinFull,PinShort))return Say(PinFull,PinShort,PinKey.StartsWith(TEXT("card"))||PinKey.StartsWith(TEXT("matter:"))?PinKey:PinText);}
 // 2. A WINDOW OPEN NOW (TASKS-TEN-20261113): be there for it.
 {FString Open;if(WindowOpenStep(Open))return Say(Open);}
 const int32 Urgent=UrgentCase(true);const FCaseStep Step=Urgent>=0?CaseStep(Urgent):FCaseStep();
 // 3. A PAID CARD THAT IS UP: a planned case test's job, then the cards at OPS.
 if(Step.Step==ECaseStep::RunPlan&&PlannedFieldTest(Urgent)<0)return Say(Step.Next,Step.Card,Step.Task);
 if(Pending>0)return Say(FString::Printf(TEXT("OPS (corner PC), DAILY WORK: SELECT ONE OF %d UNFINISHED OBSERVATIONS; TRACK, TUNE AND REGISTER THE NEXT RESULT. COMPLETED RESULTS ARE IN EARLIER WORK."),Pending),
  FString::Printf(TEXT("OPS // %d JOB CARD%s UP."),Pending,Pending==1?TEXT(""):TEXT("S")),TEXT("cards"));
 // 4. THE NIGHT WATCH OR THE EVENING'S CARD WITH ITS TIME. LOOP-NIGHT-WATCH-20261113 (Tom 13 November: «nattevakta gå foran»): from
 // 19:00 the night's cards and the night watch's notice lead (NightBeforeGoal), and the goal is said after them.
 const bool bNightFirst=NightBeforeGoal();
 const FString CardRow=Evening?TEXT("card:")+Evening->Id:FString();
 auto SayEvening=[&]()
 {
  if(NightWatch(*Evening))return Say(Goal.IsEmpty()?EveningLine(*Evening):EveningLine(*Evening)+TEXT(" ")+Goal,
   FString::Printf(TEXT("NIGHT WATCH // %s AT %s. TILL THEN%s"),*FString(FS47Observing::Runs()[Evening->Source].Source).ToUpper(),*FS47FreeformCases::Clock(EveningUp(*Evening)),
    SlipToFollow()?TEXT(": THE SLIP ON THE MAP."):TEXT(", YOUR OWN TIME.")),CardRow);
  return Say(Goal.IsEmpty()?EveningLine(*Evening):EveningLine(*Evening)+TEXT(" ")+Goal,
   FString::Printf(TEXT("TRACKING PC // %s IN VIEW FROM %s. TILL THEN, DAY WORK."),*FString(FS47Observing::Runs()[Evening->Source].Source).ToUpper(),*FS47FreeformCases::Clock(EveningUp(*Evening))),CardRow);
 };
 if(Evening&&bNightFirst)return SayEvening();
 // 5. A DEADLINE BEFORE NOON: the slip's circle, a report, worth a look, what Dale asked.
 if(const FString Due=DeadlineLine();!Due.IsEmpty())return Say(Due);
 // 6. FREE TIME. With the parts in hand the station's own goal comes first of it, in daylight (today's rule).
 if(Next&&Field->BranchNeeds(*Next).IsEmpty()&&!bDark&&!bNightFirst)return SayGoal();
 // a case's step (a field test made ready waits for its window: it is named with its time on the note, and is no next step before)
 if(Urgent>=0&&!FieldWindowWaits(Urgent))return Say(Step.Next,Step.Card,Step.Task.IsEmpty()?Step.Next:Step.Task);
 if(M&&M->Night&&M->Night->Album&&M->Night->Album->PendingLabPhoto())return Say(FString(TEXT("PHOTO LAB: DEVELOP, FIX AND COLLECT YOUR EXPOSED PHOTOGRAPH AT THE WET BENCH.")),TEXT("PHOTO LAB // DEVELOP YOUR EXPOSED FILM AT THE WET BENCH."));
 if(ServiceGuide==1)return Say(ServiceObjective());
 if(Gear&&Gear->AnyLeft())return Say(Gear->TaskLine(),Gear->TaskLine());
 // An idle loan logger is an option, not the next thing to do: it never again stands in front of the goal (T16-17, 10).
 if(Field&&Field->LoggerAccess()&&(Field->FieldLogger().Pressing()||(Goal.IsEmpty()&&!Evening)))return Say(Field->FieldLogger().NextAction());
 if(Evening)return SayEvening();
 // Said only when it is true: what stops a rest says so itself. LOOP-NIGHT-WATCH-20261113: and not from 19:00 until 02:30 of a shift
 // from day 2 (RestSuggestedAt), unless he is worn out: the small hours are the night's, and the night watch keeps him in them.
 const bool bWorn=Fatigue&&Fatigue->Level()>=S47Thoughts::ExhaustedFrom;
 const bool bCanRest=(!Director||Director->RestBlock().IsEmpty())&&RestSuggestedAt(WorldClock,DayNumber,RulesLive(),bWorn);
 // The reference check pays nothing: a player short of money needs to hear where paid work comes from.
 if(Cash<5||Loan>0)return Say(bCanRest?TEXT("OPS: TODAY'S PAID WORK IS DONE. REST UNTIL MORNING FOR NEW PAID OBSERVATIONS; THEIR PAY GOES FIRST TO WHAT YOU OWE STORES (REQ ON OPS)."):TEXT("OPS: TODAY'S PAID WORK IS DONE. NEW PAID OBSERVATIONS COME WITH THE MORNING; THEIR PAY GOES FIRST TO WHAT YOU OWE STORES (REQ ON OPS)."),
  bCanRest?TEXT("OPS // TODAY'S PAID WORK IS DONE. REST UNTIL MORNING FOR MORE."):TEXT("OPS // TODAY'S PAID WORK IS DONE. MORE COMES WITH THE MORNING."));
 // TASKS-TEN-20261113 (change 3): a goal that only waits for daylight is no task: never the line itself, only the half-line said after
 // the night's last line (as LOOP-NIGHT-WATCH-20261113 had it from day 2; now always).
 if(!Goal.IsEmpty()&&!bDark)return SayGoal();
 // FIX10-3 (M3): the loop from here is out and back: the camera, then the photo lab's wet bench.
 const FString DoneLine=bCanRest?TEXT("OPS: TODAY'S PAID WORK IS DONE. GO OUT WITH THE CAMERA: PHOTOGRAPHS AND SAMPLES ARE YOUR EVIDENCE. DEVELOP FILM AT THE WET BENCH IN THE PHOTO LAB. ARRAY REPAIRS: ARRAY STATUS (left-hand PC). OPTIONAL: A REFERENCE CHECK AT DAILY WORK, NO PAY. OR REST."):TEXT("OPS: TODAY'S PAID WORK IS DONE. GO OUT WITH THE CAMERA: PHOTOGRAPHS AND SAMPLES ARE YOUR EVIDENCE. DEVELOP FILM AT THE WET BENCH IN THE PHOTO LAB. ARRAY REPAIRS: ARRAY STATUS (left-hand PC). OPTIONAL: A REFERENCE CHECK AT DAILY WORK, NO PAY.");
 return Say(Goal.IsEmpty()?DoneLine:DoneLine+TEXT(" ")+Goal,
  bCanRest?TEXT("OPS // PAID WORK DONE. GO OUT WITH THE CAMERA, OR REST."):TEXT("OPS // PAID WORK DONE. GO OUT WITH THE CAMERA."),DoneLine);
}
bool FS47FreeformSession::StartReferenceCheck()
{
 if(!bActive||Tutorial>0||!RequirePower()||Work.ContainsByPredicate([this](const FJob& J){return J.Stage!=EJobStage::Filed&&!J.bDuplicateRejected&&!EveningGated(J);}))return false;
 FJob J;J.Id=FString::Printf(TEXT("FF-D%04d-REFERENCE-%04d"),DayNumber,Work.Num());J.Day=DayNumber;J.Slot=0;J.Source=0;
 J.Card=FS47FreeformJobs::Make(JobSeed,DayNumber,0,0);J.bGenerated=true;J.bPaid=true;
 J.Card.Receiver.Card=TEXT("OPTIONAL REFERENCE CHECK / NO PAYMENT\nRetain a fresh instrument baseline using the usual tracking cross and receiver. This check creates no incident or cause by itself.\n\n")+J.Card.Receiver.Card;
 Work.Add(J);Selected=Work.Num()-1;FieldFocus=-1;bGuideFocus=false;Settle=0;bCrossRunning=false;bAudioPlaying=false;AudioCursor=0;
 Changed(TEXT("TRACKING (right-hand PC): POINT TO 3C 286 FOR THE REFERENCE CHECK. ITS ORIGINAL IS KEPT AS A BASELINE; NO PAY."));return true;
}
bool FS47FreeformSession::CancelFieldTest(int32 Line)
{
 const int32 Test=PlannedFieldTest(Line);const auto* C=Cases.Get(Line);
 if(Test<0||!C||C->PlannedJob.IsEmpty()||C->Results[Test].bDone)return false;
 Cases.CancelPlan(Line);if(Line==2&&CaseField)CaseField->ResetPlan();if(FieldFocus==Line)FieldFocus=-1;
 Changed(TEXT("OPS / CASE BOARD: Field plan cleared, no result recorded. You may plan another field test."));return true;
}
bool FS47FreeformSession::CancelCaseJob(int32 Line)
{
 // A receiver test that cannot be met (no fuel, no relay, window gone) must not lock the case. Only before anything is pointed or recorded.
 const auto* C=Cases.Get(Line);if(!C||C->Planned<0||PlannedFieldTest(Line)>=0||C->PlannedJob.IsEmpty())return false;
 const int32 I=Work.IndexOfByPredicate([&](const FJob& J){return J.Id==C->PlannedJob&&J.CaseLine==Line&&!J.bCaseRecorded;});
 // CASETEST-20261109: pointed (or stopped by a spoil) is still before the recording: it can go. Not while it records.
 if(Work.IsValidIndex(I)&&Work[I].Stage>EJobStage::Pointed){Feedback=TEXT("That test is recording now: let it finish and register it. Cancel it on the case board only before START RECORDING.");Changed(Feedback);return false;}
 if(Work.IsValidIndex(I)){if(Selected==I){Settle=0;bCrossRunning=false;}Work.RemoveAt(I);if(Selected>I)--Selected;else if(Selected==I)Selected=FMath::Max(0,Work.IndexOfByPredicate([](const FJob& J){return J.Stage!=EJobStage::Filed;}));}
 Cases.CancelPlan(Line);
 Changed(TEXT("OPS / CASE BOARD: Test plan cleared, no result recorded. You may plan another test."));return true;
}
bool FS47FreeformSession::FieldTestExpectsPresent(int32 Line,int32 Test)const
{
 // Only the planned field test is answered, so world code cannot read the hidden cause freely.
 const auto* C=Cases.Get(Line);return C&&PlannedFieldTest(Line)==Test&&FS47FreeformCases::PresentFor(Line,C->Truth,Test);
}
bool FS47FreeformSession::ReportFieldTest(int32 Line,int32 Test,bool bValid,bool bPresent,const FString& Evidence)
{
 if(PlannedFieldTest(Line)!=Test)return false;bool First=false;
 const FString Plan=Cases.Get(Line)?Cases.Get(Line)->PlannedJob:FString();
 if(!Cases.Record(Line,Test,DayNumber,bValid,bPresent,First))return false;
 const auto& D=FS47FreeformCases::Def(Line);
 if(!bValid)
 {
  // CASETEST-20261109: an invalid control stays planned, so it is taken again with no new plan. A fresh plan ID all the same: the
  // visitors' run (FS47CaseFieldTests) and the land's tapes (StepLandTest) are matched by it, so nothing of the failed run is borrowed.
  FString Again=FString::Printf(TEXT("FIELD-D%04d-CASE%d-T%d"),DayNumber,Line+1,Test+1);
  if(Line==FS47FreeformCases::LandLine&&Test==FS47FreeformCases::LandLogger)Again+=FString::Printf(TEXT("-TAPE%d"),Field?Field->FieldLogger().Tapes.Num():0);
  const bool bAgain=!Cases.TrailCold(Line,DayNumber)&&Cases.Plan(Line,Test,Again);
  if(bAgain&&Line==2&&CaseField)CaseField->ResetPlan();
  Feedback=TEXT("Field test INVALID: ")+Evidence.Left(200)+(bAgain?TEXT(" It stays planned: prepare it again and take it once more, no new plan needed."):TEXT(" Plan it again on the case board."));Changed(Feedback);return true;
 }
 TArray<FString> Out;for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)if(FS47FreeformCases::PresentFor(Line,I,Test)!=bPresent&&Cases.Excluded(Line,I))Out.Add(D.Causes[I]);
 const FString Result=FS47FreeformCases::TestResultText(Line,Test,bPresent);
 QueueResultSheet(Line,Test,Plan); // CASES-OPS-20261109: printed at SARO as the record comes in
 RecordEvidence(FString::Printf(TEXT("case%d-test%d"),Line+1,Test+1),FString::Printf(TEXT("CASE / %s\nFIELD TEST / %s / DAY %d\nRESULT / %s\nPREDICTION / %s\nRULES OUT / %s\nRECORD / %s"),D.Title,*FString(D.Tests[Test].Name).ToUpper(),DayNumber,*Result,*FS47FreeformCases::Prediction(Line,Test),*FString::Join(Out,TEXT(", ")),*Evidence.Left(400)));
 FString Text=Out.IsEmpty()?FString::Printf(TEXT("Field test retained. %s: the records disagree, and rule nothing out."),*Result):FString::Printf(TEXT("Field test retained. %s: rules out %s."),*Result,*FString::Join(Out,TEXT(" and ")));
 if(First){AddItem(TEXT("relay"),1);Text+=TEXT(" Station management credits one service relay to the depot for documented test work.");}
 if(Cases.Remaining(Line)==1)Text+=TEXT(" Only one explanation survives: register the conclusion on the case board.");
 Changed(Text+TEXT(" Its result sheet is at the SARO printer."));return true;
}
void FS47FreeformSession::OpenCaseFor(const FString& FindingId)
{
 const int32 Line=FS47FreeformCases::LineForFinding(FindingId);
 // Whether the case is new or not, the wind no longer covers it.
 const bool bBeyondWind=FindingId.StartsWith(TEXT("taken-"))||FindingId.StartsWith(TEXT("flood-room-"));
 if(bBeyondWind)Cases.RuleOutWind(Line);
 // A night's record out on the land is a lead in the case it belongs to (FS47Happenings), and opens it if it is the first.
 if(Line>=0&&(FS47Happenings::IsNightFinding(FindingId)||Line==FS47FreeformCases::LandLine))
 {
  const auto* Was=Cases.Get(Line);const bool bNew=!(Was&&Was->bOpen);
  Cases.AddLead(Line,FindingId,DayNumber,FMath::Fmod(WorldClock,86400.));
  const auto* CaseNow=Cases.Get(Line);
  if(!bNew||!(CaseNow&&CaseNow->bOpen))return;
 }
 // Tom, 9 November: the interval case waits for its comparison's registration at OPS (FirstCaseHeld, OpenFirstCase).
 else if(Line<0||(Line==0&&FirstCaseHeld())||!Cases.Open(Line,DayNumber,FMath::Fmod(WorldClock,86400.)))return;
 // The reel deck's review says it in its own one message, with the job's next step (CASES-OPS-20261109).
 if(FindingId==TEXT("pulse-review")){Changed();return;}
 // A late line (M7) has no test to choose until there is something to compare: its other place first.
 Changed(TEXT("New case opened: ")+FString(FS47FreeformCases::Def(Line).Title)+(bBeyondWind?TEXT(". Wind is ruled out. Two explanations are left. Choose a test on the OPS case board."):Line==FS47FreeformCases::LandLine?TEXT(". Three explanations fit. A control uses the kit you have; see the OPS case board."):Line>=FS47FreeformCases::LateFirst?TEXT(". Three explanations fit. The OPS case board says where else to look."):TEXT(". Three explanations fit. Choose a test on the OPS case board.")));
}
bool FS47FreeformSession::FirstCaseHeld()const
{
 if(const auto* C=Cases.Get(0);!C||C->bOpen)return false;
 // D45-20261110: a comparison retained before its chart was collected (Chart) waits for its registration the same way
 return Work.ContainsByPredicate([](const FJob& J){return J.bAnomaly&&J.bAudioReviewed&&J.CaseTest<0&&!Specialized(J.Phenomenon)&&(J.Stage==EJobStage::Chart||J.Stage==EJobStage::Recorded)&&!J.bDuplicateRejected;});
}
bool FS47FreeformSession::OpenFirstCase(const FJob& J)
{
 // Opened now, the day and clock of the registration; the SOURCE line keeps the original's own clock (SetReference).
 if(!Cases.Open(0,DayNumber,FMath::Fmod(WorldClock,86400.)))return false;
 if(J.ReferenceClock>0)Cases.SetReference(0,NightOf(J.ReferenceClock),FMath::Fmod(J.ReferenceClock,86400.));
 UpdateMysteries();return true;
}
bool FS47FreeformSession::PlanCaseTest(int32 Line,int32 Test)
{
 if(Tutorial>0){Feedback=TEXT("Finish the FIRST SHIFT observation before planning a separate case test.");Changed(Feedback);return false;}
 const auto* C=Cases.Get(Line);
 if(!C||!C->bOpen||C->bConcluded||Test<0||Test>=FS47FreeformCases::TestCount){Feedback=TEXT("No open case can take that test.");return false;}
 if(C->Planned>=0){Feedback=TEXT("A test for this case is already planned: ")+C->PlannedJob+TEXT(". Run it before planning another.");return false;}
 if(C->Results[Test].bDone){Feedback=TEXT("That test is already retained. Its result does not change by repeating it.");return false;}
 {
  const auto& Def=FS47FreeformCases::Def(Line).Tests[Test];
  if(Def.Access&&!Cases.HasAccess(Def.Access)){Feedback=TEXT("This test needs the field logger on loan. Conclude THE REPEATED INTERVAL to have one approved.");return false;}
  const FString Unavailable=FS47FreeformCases::UnavailableReason(Line,Test,HasKit(TEXT("logger2")));
  if(!Unavailable.IsEmpty()){Feedback=Unavailable;return false;}
  if(Cases.Remaining(Line)<=1){Feedback=FS47FreeformCases::Def(Line).Mystery?TEXT("Only one explanation is left: file your assessment on the case board."):TEXT("Only one explanation is left: register the supported conclusion on the case board.");Changed(Feedback);return false;}
  if(Cases.TrailCold(Line,DayNumber)){Feedback=TEXT("The trail has gone cold: a fresh control would no longer show anything about that night. You can still file your assessment.");Changed(Feedback);return false;}
  if(const FString Need=LandTestNeeds(Line,Test);!Need.IsEmpty()){Feedback=TEXT("This control needs ")+Need+TEXT(".");Changed(Feedback);return false;}
  if(Def.Condition==ECaseCondition::FieldWork)
  {
   FString Id=FString::Printf(TEXT("FIELD-D%04d-CASE%d-T%d"),DayNumber,Line+1,Test+1);
   // The land's logger control takes a tape started after it is planned: the plan says how many there were (StepLandTest).
   if(Line==FS47FreeformCases::LandLine&&Test==FS47FreeformCases::LandLogger)Id+=FString::Printf(TEXT("-TAPE%d"),Field?Field->FieldLogger().Tapes.Num():0);
   if(!Cases.Plan(Line,Test,Id))return false;
   if(Line==2&&CaseField)CaseField->ResetPlan();
   const bool Focused=FocusFieldTest(Line);
   Feedback=Focused?TEXT("Field test planned. ")+FieldNextAction(Line):TEXT("Field plan retained. Finish the active recording or control, then choose Follow this field test on CASE BOARD.");Changed(Feedback);return true;
  }
 }
 FJob J;J.Id=FString::Printf(TEXT("FF-D%04d-CT%d-%d"),DayNumber,Test+1,Work.Num());J.Day=DayNumber;J.Slot=1;J.Source=1; // Tom 13 Nov: no case number in the job id shown at DAILY WORK; the job keeps CaseLine
 J.Card=FS47FreeformJobs::Make(JobSeed,DayNumber,1,1);J.bGenerated=true;J.CaseLine=Line;J.CaseTest=Test;
 const auto& T=FS47FreeformCases::Def(Line).Tests[Test];
 J.Card.Receiver.Card=TEXT("CASE TEST / ")+FString(T.Name).ToUpper()+TEXT("\n")+T.Instruction+TEXT("\n\n")+J.Card.Receiver.Card;
 if(!Cases.Plan(Line,Test,J.Id))return false;
 FieldFocus=-1;bGuideFocus=false;
 Work.Add(J);
 const FJob* Current=CurrentJob();if(!Current||Current->Stage==EJobStage::New||Current->Stage==EJobStage::Filed)Selected=Work.Num()-1;
 Feedback=TEXT("Case test planned: ")+FString(T.Name)+TEXT(". Job ")+J.Id+(Selected==Work.Num()-1?TEXT(" is selected: carry it out at the tracking PC and receiver."):TEXT(" is on the OPS list, but another job is still selected. Finish that job, or select the CASE row in DAILY WORK, before recording."));Changed(Feedback);return true;
}
void FS47FreeformSession::RecordCaseResult(FJob& J)
{
 J.bCaseRecorded=true;const int32 Line=J.CaseLine,Test=J.CaseTest;const bool bPresent=CaseObserved(J);bool First=false;
 if(!Cases.Record(Line,Test,DayNumber,J.bCaseValid,bPresent,First)){Changed(FString::Printf(TEXT("Recording registered, $%d credited. It was NOT counted as a case test: the case no longer has this test planned. Open OPS / CASE BOARD to see what the case needs."),LastPay));return;}
 if(!J.bCaseValid)
 {
  // CASETEST-20261109: only a recording spoiled before SpoilCaseTest stopped them (an older save) gets here. Planned again at once, as
  // a new job (this one is paid and filed); J is not used after PlanCaseTest, which adds to Work.
  const bool bAgain=PlanCaseTest(Line,Test);
  Feedback=FString::Printf(TEXT("Observation registered, $%d credited. The case test was spoiled during the recording and counts for nothing. %s"),LastPay,bAgain?*(TEXT("It is planned again as job ")+Work.Last().Id+TEXT(": record it once more.")):TEXT("Plan it again on the case board."));Changed(Feedback);return;
 }
 const auto& D=FS47FreeformCases::Def(Line);
 TArray<FString> Out;for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)if(FS47FreeformCases::PresentFor(Line,I,Test)!=bPresent)Out.Add(D.Causes[I]);
 const FString Result=FS47FreeformCases::ResultText(Line,bPresent);
 QueueResultSheet(Line,Test,J.Id); // CASES-OPS-20261109: the result on paper, at the printer
 RecordEvidence(FString::Printf(TEXT("case%d-test%d"),Line+1,Test+1),FString::Printf(TEXT("CASE / %s\nTEST / %s / DAY %d / ORIGINAL %s-original\nRESULT / %s\nPREDICTION / %s\nRULES OUT / %s"),D.Title,*FString(D.Tests[Test].Name).ToUpper(),DayNumber,*J.Id,*Result,*FS47FreeformCases::Prediction(Line,Test),*FString::Join(Out,TEXT(", "))));
 FString Text=FString::Printf(TEXT("Case test registered, $%d credited. %s: rules out %s."),LastPay,*Result,*FString::Join(Out,TEXT(" and ")));
 if(First){AddItem(TEXT("relay"),1);Text+=TEXT(" Station management credits one service relay to the depot for documented test work.");}
 if(Cases.Remaining(Line)==1)Text+=TEXT(" Only one explanation survives: register the conclusion on the case board.");
 Changed(Text+TEXT(" The result sheet is printing: collect it at the printer."));
}
void FS47FreeformSession::StepCaseReplies()
{
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)
 {
  if(!Cases.ReplyDue(Line,DayNumber))continue;
  const auto* Answer=Cases.Reply(Line);const auto* Y=FS47FreeformCases::Def(Line).Mystery;
  // Spent first: what follows happens once, and a Continue finds it done.
  Cases.MarkReplied(Line);if(!Answer||!Y)continue;
  const FString Page=Answer->Text;
  RecordEvidence(FString::Printf(TEXT("case%d-reply"),Line+1),Page);
  // TELEX-20261109: it prints on the telex, and is a paper only once it is torn off there (TearOffTelex). A full machine (or no room
  // for it) files it as before, so that nothing is lost.
  const FString DocId=FString::Printf(TEXT("freeform-case%d-reply"),Line+1),Stamp=FString::Printf(TEXT("SARO TELEX / DAY %d"),DayNumber);
  const bool bOnMachine=QueueTelex(DocId,Y->ReplyTitle,Stamp,Page);
  if(!bOnMachine&&M&&M->Story)
  {
   FS47Doc Doc;Doc.Id=DocId;Doc.Title=Y->ReplyTitle;Doc.Kind=TEXT("telex");Doc.Stamp=Stamp;
   Doc.Page=Page;Doc.Transcript=Page;M->Story->AddDoc(Doc);
  }
  FString Note=bOnMachine?FString(TEXT("The telex in the control room is printing: "))+Y->ReplyTitle+TEXT(". Tear it off at the machine."):FString(Y->ReplyTitle)+TEXT(" has come in on the telex. It is in the tray at OPS.");
  switch(Answer->Effect)
  {
   case FS47FreeformCases::EReply::Spare:AddItem(TEXT("fuse"),1);break;
   case FS47FreeformCases::EReply::CallOut:
   {
    // Never into debt over this: what is there goes, and that is the end of it.
    const int32 Charge=FMath::Min(Cash,int32(FS47FreeformCases::CallOutCharge));
    if(Charge>0&&Spend(Charge))Note+=FString::Printf(TEXT(" $%d charged."),Charge);
    break;
   }
   case FS47FreeformCases::EReply::FeederFault:SetPowerFault(true);break;
   default:break;
  }
  Changed(Note);
 }
}
int32 FS47FreeformSession::SheriffTrust() const
{
 // Follows from what has been filed, so an older save has it too. Nothing is stored. Any case carried to a conclusion and filed
 // counts, whatever it concluded: the county trusts a station that finishes what it starts, not one that says what it wants to hear.
 int32 Trust=0;
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)if(const auto* C=Cases.Get(Line);C&&C->bConcluded)++Trust;
 // And a missing man's truck called in (FS47Happenings): the county remembers who rang.
 for(const auto& Kept:Findings)Trust+=Kept.Key.StartsWith(TEXT("missing_reported_"))?1:0;
 return Trust;
}
bool FS47FreeformSession::DeputyOnRoute() const{return SheriffTrust()>0;}
int32 FS47FreeformSession::TownTrust() const
{
 // Places seen, and kinds of thing seen out on the land at night (FS47Happenings): somebody who is out there and says what is there.
 int32 Seen=0;for(const auto& Kept:Findings)Seen+=Kept.Key.StartsWith(TEXT("place_"))&&!Kept.Key.EndsWith(TEXT("_needle"))?1:0;
 return SheriffTrust()+(Seen+NightRecordKinds()>=4?1:0);
}
bool FS47FreeformSession::CaseSupported(int32 Line) const
{
 bool Kept[FS47FreeformCases::ClueCount]={};
 if(Line==FS47FreeformCases::LandLine)
 {
  // LOOP-WHOLE-20261109: OUT ON THE LAND opens on one record; a filing has something behind it with a record from another night too
  // (folded ones were many), or with a control.
  if(const auto* C=Cases.Get(Line))
  {
   TSet<int32> Nights;for(const FString& L:C->Leads)if(FindingDays.Contains(L))Nights.Add(FindingDays.FindRef(L));
   Kept[0]=!C->Leads.IsEmpty()||C->FoldedLeads>0;Kept[1]=Nights.Num()>=2||C->FoldedLeads>0;
  }
  return Cases.Supported(Line,Kept);
 }
 if(const auto* Y=FS47FreeformCases::Def(Line).Mystery)for(int32 I=0;I<FS47FreeformCases::ClueCount;++I)Kept[I]=HasFinding(Y->Clues[I].Finding);
 return Cases.Supported(Line,Kept);
}
bool FS47FreeformSession::CaseFromField(int32 Line) const
{
 // CHAIN-2-20261109: Ward's rule, the station is not kept on desk work alone.
 const auto* C=Cases.Get(Line);
 if(!C||!C->bConcluded||!CaseSupported(Line))return false;
 if(Line==FS47FreeformCases::LandLine)return true;
 const auto& D=FS47FreeformCases::Def(Line);
 for(int32 T=0;T<FS47FreeformCases::TestCount;++T)if(C->Results[T].bDone&&D.Tests[T].Condition==FS47FreeformCases::ECondition::FieldWork)return true;
 if(D.Mystery)for(const auto& Clue:D.Mystery->Clues)
 {
  const FString Where=Clue.Place;
  if(HasFinding(Clue.Finding)&&!Where.StartsWith(TEXT("SARO"))&&!Where.StartsWith(TEXT("The OPS")))return true;
 }
 for(const FString& L:C->Leads)if(FieldRecordKey(L))return true;
 return false;
}
bool FS47FreeformSession::ConcludeCase(int32 Line,int32 Cause)
{
 const bool bFieldPlan=PlannedFieldTest(Line)>=0;
 FString Why;if(!Cases.Conclude(Line,Cause,Why,DayNumber)){Feedback=Why;Changed(Why);return false;}
 if(FieldFocus==Line)FieldFocus=-1;
 const auto& D=FS47FreeformCases::Def(Line);const FString Name=D.Causes[Cause];
 // A test recording not yet started no longer tests anything: it goes back to being a day's ordinary paid work. One already under
 // way runs out as routine, since the case no longer has a plan for it to report to (FS47FreeformCases::Record refuses it). The same
 // for a trial case as for a filing (CASES-OPS-20261109); a field plan is put away with it.
 bool bFreed=false;
 for(FJob& J:Work)if(J.CaseLine==Line&&!J.bCaseRecorded&&J.Stage<=EJobStage::Pointed)
 {J.CaseLine=-1;J.CaseTest=-1;J.Stage=EJobStage::New;J.Elapsed=0;J.Card=FS47FreeformJobs::Make(JobSeed,J.Day,J.Slot,J.Source);bFreed=true;}
 if(bFreed)Settle=0;
 if(bFieldPlan&&Line==2&&CaseField)CaseField->ResetPlan();
 if(D.Mystery)
 {
  RecordEvidence(FString::Printf(TEXT("case%d-conclusion"),Line+1),FString::Printf(TEXT("ASSESSMENT FILED / %s\nYOUR ASSESSMENT / %s\nDAY %d. It rests on the retained traces and tests and has not been checked."),D.Title,*Name,DayNumber));
  if(M&&M->Story)
  {
   FS47Doc Doc;Doc.Id=FString::Printf(TEXT("freeform-case%d-records"),Line+1);Doc.Title=D.TelexTitle;Doc.Kind=TEXT("telex");Doc.Stamp=FString::Printf(TEXT("SARO STATION LOG / DAY %d"),DayNumber);
   Doc.Page=Cases.ConclusionText(Line);Doc.Transcript=Doc.Page;M->Story->AddDoc(Doc);
  }
  Feedback=TEXT("Assessment filed: ")+Name+TEXT(". The reply comes in the morning.");Changed(Feedback);return true;
 }
 // What the reply says has been done, is done. Once: a case concludes once. The patrol follows from the case itself (DeputyOnRoute).
 if(Line==0&&Cause==0)AddItem(TEXT("fuse"),1);
 if(Line==1&&Cause==0)AddItem(TEXT("relay"),2);
 RecordEvidence(FString::Printf(TEXT("case%d-conclusion"),Line+1),FString::Printf(TEXT("CASE CONCLUDED / %s\nSUPPORTED EXPLANATION / %s\nThe retained tests exclude the other explanations. Interpretation beyond this remains open."),D.Title,*Name));
 if(M&&M->Story)
 {
  FS47Doc Doc;Doc.Id=FString::Printf(TEXT("freeform-case%d-records"),Line+1);Doc.Title=D.TelexTitle;Doc.Kind=TEXT("telex");Doc.Stamp=FString::Printf(TEXT("SARO TELEX / DAY %d"),DayNumber);
  Doc.Page=Cases.ConclusionText(Line);Doc.Transcript=Doc.Page;M->Story->AddDoc(Doc);if(M->UI)M->UI->Document(Doc);
 }
 Feedback=TEXT("Case concluded: ")+Name+TEXT(". A records reply has arrived at the telex.");Changed(Feedback);return true;
}
FString FS47FreeformSession::TonightPlan()const
{
 TArray<FString> Out;
 // TELEX-20261109: what waits on the telex comes first; it is only a paper once it is torn off.
 if(!TelexSheets.IsEmpty())
 {
  TArray<FString> Titles;for(const FTelexSheet& T:TelexSheets)Titles.Add(T.Title);
  const FString Head=TelexSheets.Num()==1?FString(TEXT("A REPLY IS WAITING")):FString::Printf(TEXT("%d PAPERS ARE WAITING"),TelexSheets.Num());
  Out.Add(FString::Printf(TEXT("TELEX / %s\nIn the control room: %s. Tear it off at the machine; then it is in your notebook and in the tray at OPS."),*Head,*FString::Join(Titles,TEXT("; "))));
 }
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)
 {
  const auto* C=Cases.Get(Line);if(!C||!C->bOpen||C->bConcluded)continue;const auto& D=FS47FreeformCases::Def(Line);
  if(C->Planned<0)
  {
   // CASES-OPS-20261109: what is next, as CaseStep has it: a test to choose, a conclusion to register, an assessment to file.
   const ECaseStep Step=CaseStep(Line).Step;
   const FString Hint=Step==ECaseStep::Conclude?FString(TEXT("Only one explanation is left: register the supported conclusion on the case board."))
    :Step==ECaseStep::File?FString(Cases.TrailCold(Line,DayNumber)?TEXT("The trail has gone cold: file your assessment on the case board."):TEXT("Nothing is left to test: file your assessment on the case board."))
    :!LateStep(Line,Findings).IsEmpty()?TEXT("Next: ")+LateStep(Line,Findings)+TEXT("."):D.Mystery?FString(TEXT("Open the case board: choose a control or file your assessment.")):FString(TEXT("Open the case board and choose the next test."));
   Out.Add(FString::Printf(TEXT("CASE / %s / NO TEST PLANNED\n%d of 3 explanations still possible. %s"),D.Title,Cases.Remaining(Line),*Hint));continue;
  }
  const auto& T=D.Tests[C->Planned];
  FString Plan=TEXT("TONIGHT'S PLAN / ")+FString(D.Title)+TEXT(" / CASE TEST: ")+FString(T.Name).ToUpper()+TEXT(" / JOB ")+C->PlannedJob+TEXT("\n")+T.Instruction;
  switch(T.Condition)
  {
   case ECaseCondition::Generator:
   {
    const bool Running=Field&&Field->GeneratorRunning();const double Tank=Field?Field->GeneratorHours():0;const int32 Cans=InventoryCount(TEXT("fuel"));
    Plan+=FString::Printf(TEXT("\nREADINESS / generator %s / tank %.1f h / fuel cans in depot %d"),Running?TEXT("RUNNING"):TEXT("STOPPED"),Tank,Cans);
    if(!Running&&Tank<=0&&Cans<=0)Plan+=TEXT("\nMISSING / one fuel can. Buy it at Caliche and carry it back.");break;
   }
   case ECaseCondition::RelaySwap:
    Plan+=FString::Printf(TEXT("\nREADINESS / spare relays in depot %d"),InventoryCount(TEXT("relay")));if(InventoryCount(TEXT("relay"))<=0)Plan+=TEXT("\nMISSING / one relay. Buy it at Caliche.");break;
   case ECaseCondition::DoorsSealed:Plan+=TEXT("\nREADINESS / shut the south and east doors before you start; do not open them until the original prints.");break;
   case ECaseCondition::FieldWork:Plan+=TEXT("\n")+FieldNextAction(Line);break;
   default:Plan+=TEXT("\nREADINESS / no extra equipment. The tracking PC applies the offset.");break;
  }
  if(const FString Window=CaseWindowText(Line);!Window.IsEmpty())Plan+=TEXT("\n")+Window;
  Out.Add(Plan);
 }
 return FString::Join(Out,TEXT("\n\n"));
}
void FS47FreeformSession::OpenCaseBoard(int32 Line,bool bNotes)
{
 if(!RequirePower()||!M||!M->UI)return;
 if(Line<0){Line=CaseAwaitingChoice();if(Line<0)Line=UrgentCase();if(Line<0)for(int32 I=0;I<FS47FreeformCases::CaseCount;++I)if(Cases.Get(I)&&Cases.Get(I)->bOpen&&!Cases.Get(I)->bConcluded){Line=I;break;}if(Line<0)Line=0;}
 // D48-20261110 (Tom: a better design and a logical layout): one glass, no page scroll. Left, the cases and what belongs to the case
 // itself: where it came from, its notes, and the filing. Right, the work: the amber NEXT line, the three explanations with their
 // standing, the tests as rows with one selected, that test's words and predictions, and the one amber key. The rules in a footer.
 // Behaviour, data, keys and hooks are those of CASES-OPS-20261109 (Docs/Research/Gameplay/SAKER_OPS_2026-11-09.md).
 if(bNotes)bCaseDetails=true;
 if(BoardPickLine!=Line){BoardPick=-1;BoardPickLine=Line;}
 static const TCHAR* const Letter[FS47FreeformCases::CauseCount]={TEXT("A"),TEXT("B"),TEXT("C")};
 // Words folded to a column's width, at the spaces (a Table cell does not wrap by itself).
 auto Wrap=[](const FString& Text,int32 Width)
 {
  TArray<FString> Words;Text.ParseIntoArray(Words,TEXT(" "),true);FString Out,Row;
  for(const FString& W:Words){if(!Row.IsEmpty()&&Row.Len()+1+W.Len()>Width){Out+=Row+TEXT("\n");Row.Empty();}if(!Row.IsEmpty())Row+=TEXT(" ");Row+=W;}
  return Out+Row;
 };
 TArray<S47Kit::FW> Rows,Left,Right;const auto* C=Cases.Get(Line);const auto& D=FS47FreeformCases::Def(Line);
 // ---- Left: the cases, each with its standing; the one open here filled amber. At most five rows; the rest are counted. ----
 {
  auto StateOf=[this](int32 I){const auto* X=Cases.Get(I);return !X||!X->bConcluded?0:FS47FreeformCases::Def(I).Mystery&&!X->bReplied?1:2;};
  TArray<int32> Order;for(int32 I=0;I<FS47FreeformCases::CaseCount;++I)if(const auto* X=Cases.Get(I);X&&X->bOpen)Order.Add(I);
  Order.StableSort([&StateOf](int32 A,int32 B){return StateOf(A)<StateOf(B);});
  constexpr int32 Cap=5;TArray<int32> Shown;int32 Hidden=0;
  for(int32 I:Order){if(Shown.Num()<Cap)Shown.Add(I);else if(I==Line){Shown.Last()=I;++Hidden;}else ++Hidden;}
  static const TCHAR* const StateWord[3]={TEXT("OPEN"),TEXT("WAITING FOR REPLY"),TEXT("CLOSED")};
  TArray<T::FRow> CaseRows;
  for(int32 I:Shown)
  {
   T::FRow R;R.Cells={FString::Printf(TEXT("%d %s\n%s"),I+1,*Wrap(FS47FreeformCases::Def(I).Title,17),StateWord[StateOf(I)])};R.bSelected=I==Line;
   R.Action=[this,I]{Feedback.Empty();OpenCaseBoard(I);};
   if(CaseStepRank(CaseStep(I).Step)>=3&&CaseStep(I).Step!=ECaseStep::RunPlan)R.Badge=TEXT("*"); // something to do here (as the nav's star)
   CaseRows.Add(R);
  }
  if(!CaseRows.IsEmpty())Left.Add(T::Table({TEXT("CASES")},{1.f},CaseRows,19));
  if(Hidden>0)Left.Add(T::Small(FString::Printf(TEXT("+%d MORE / RECORDS > CASES"),Hidden)));
 }
 if(!C||!C->bOpen)
 {
  Rows.Add(T::Paper(TEXT("OPS / CASE BOARD\nNO OPEN CASE. A COMPARISON REGISTERED AT DAILY WORK, OR A FIELD TRACE, OPENS ONE HERE.")));
  if(!Left.IsEmpty())Rows.Add(T::Columns({{.27f,S47Kit::Col(Left,6)},{.73f,S47Kit::FW(SNullWidget::NullWidget)}},14));
  if(!Feedback.IsEmpty())Rows.Add(T::Small(Feedback));
  OpenTerminal(3,TEXT("OPS / CASE BOARD"),OpsShell(2,S47Kit::Col(Rows,4)));return;
 }
 // A result sheet just collected marks the test and the explanation it ruled out, this once.
 const int32 Mark=FreshLine==Line?FreshTest:-1;if(FreshLine==Line)FreshLine=FreshTest=-1;
 const bool bCold=Cases.TrailCold(Line,DayNumber);const int32 Remaining=Cases.Remaining(Line);
 const bool bHaveLogger2=HasKit(TEXT("logger2"));const int32 FieldPlan=PlannedFieldTest(Line);
 // The question's first sentence, where it came from, and the notes folded behind one key.
 {
  FString Question=Cases.QuestionText(Line);int32 Stop=INDEX_NONE;
  for(int32 I=0;I<Question.Len();++I)if(Question[I]==TEXT('?')||Question[I]==TEXT('.')){Stop=I;break;}
  if(Stop>0)Question=Question.Left(Stop+1);
  Left.Add(T::Small(Question));
  Left.Add(T::Small(CaseSource(Line)));
 }
 // Traces, leads and field records: the notes (shown on the right in place of the tests when unfolded).
 TArray<FString> Notes;int32 NoteCount=0;
 {
  // (OUT ON THE LAND has no places of its own: its traces are its field records, below)
  if(const auto* Y=Line==FS47FreeformCases::LandLine?nullptr:D.Mystery)
  {
   FString Trail=TEXT("TRACES / what each place showed. One of them tells it straight; where they disagree, a control tells them apart."); // CHAIN-3-20261109
   for(int32 Index=0;Index<FS47FreeformCases::ClueCount;++Index)
   {
    const auto& Clue=Y->Clues[Index];
    const bool bKept=Findings.Contains(Clue.Finding);
    // The feeder has nothing to show before midnight (Director, MysteryClueHere); the chart is there as soon as it is there.
    const bool bLost=Line==FS47FreeformCases::LostLine;
    // M7's late lines say where their own places are (FClueDef::Hint).
    const TCHAR* Open=Clue.Hint?Clue.Hint:bLost?(Index==0?TEXT("not looked at yet. Log on at OPS: it still shows the hour you lost."):TEXT("not looked at yet. Go back to where it happened.")):Index==1?TEXT("not looked at yet. Worth a look after midnight; whatever is there will not last past noon."):TEXT("not looked at yet. Whatever is there will not last past noon.");
    Trail+=FString::Printf(TEXT("\n%s: %s"),Clue.Place,bKept?*Findings.FindRef(Clue.Finding):Clue.Hint||bLost||(Director&&Director->MysteryTrailOpen())?Open:TEXT("nothing retained."));
    ++NoteCount;
   }
   Notes.Add(Trail);
  }
  if(!C->Leads.IsEmpty()&&Director)
  {
   FString Leads=TEXT("LEADS / retained observations, not yet one cause");int32 Shown=0;
   for(const auto& O:Director->Observations())if(C->Leads.Contains(O.Id)){++Shown;Leads+=FString::Printf(TEXT("\nD%d %s / %s / bearing %s / %s%s"),O.Day,*FS47FreeformCases::Clock(O.Clock),*O.Place,*O.Bearing,O.bHeard?TEXT("heard"):TEXT("seen"),O.PhotoId.IsEmpty()?TEXT(""):TEXT(" / photographed"));}
   // LOOP-WHOLE-20261109: leads folded at the cap are said, not dropped (the land's are counted with its field records, below).
   if(C->FoldedLeads>0&&Line!=FS47FreeformCases::LandLine){Leads+=FString::Printf(TEXT("\nEARLIER / %d OLDER LEAD%s FOLDED"),C->FoldedLeads,C->FoldedLeads==1?TEXT(""):TEXT("S"));++Shown;}
   if(Shown>0){Notes.Add(Leads);NoteCount+=Shown;}
  }
  {
   // What the night's events left out on the land, as the field records have it: its first sentence and its day.
   FString Land=TEXT("FIELD RECORDS / out on the land, night by night");int32 Shown=0;
   // LOOP-WHOLE-20261109: the newest dozen; the rest, and any folded at the cap, are counted here and kept in the notebook.
   TArray<FString> Kept;for(const FString& L:C->Leads)if(Findings.Contains(L))Kept.Add(L);
   const int32 Earlier=FMath::Max(0,Kept.Num()-CaseBoardRecords)+C->FoldedLeads;
   if(Earlier>0)Land+=FString::Printf(TEXT("\nEARLIER / %d OLDER RECORD%s, EACH STILL IN YOUR NOTEBOOK"),Earlier,Earlier==1?TEXT(""):TEXT("S"));
   for(int32 I=FMath::Max(0,Kept.Num()-CaseBoardRecords);I<Kept.Num();++I)
   {
    const FString& L=Kept[I];FString First=Findings.FindRef(L);int32 Stop=INDEX_NONE;
    if(First.FindChar(TEXT('.'),Stop)&&Stop>0)First=First.Left(Stop+1);
    Land+=FString::Printf(TEXT("\nD%d %s"),FindingDays.FindRef(L),*First.Left(150));++Shown;
   }
   if(Shown>0){Notes.Add(Land);NoteCount+=Shown;}
  }
  {
   // N1110 CASEBOARD-20261108: the trace book fills itself. Bearings, photographs, field records and witness statements that belong to
   // this case (by its leads, its places and its nights), newest first, each with day, time, place and kind. Eight lines; the rest are counted.
   struct FTrace{double Key=0;int32 Day=0;double Clock=-1;FString Place,Kind,Text;};
   TArray<FTrace> Trace;TSet<FString> Here;
   if(const auto* Y=Line==FS47FreeformCases::LandLine?nullptr:D.Mystery)for(int32 Index=0;Index<FS47FreeformCases::ClueCount;++Index)Here.Add(FString(Y->Clues[Index].Place).ToLower());
   if(Director)for(const auto& O:Director->Observations())if(C->Leads.Contains(O.Id))
   {
    Here.Add(O.Place.ToLower());
    FTrace Row;Row.Key=O.Clock;Row.Day=O.Day;Row.Clock=O.Clock;Row.Place=O.Place;
    Row.Kind=O.Bearing.IsEmpty()?(O.bHeard?TEXT("HEARD"):TEXT("SEEN")):TEXT("BEARING");
    Row.Text=O.Bearing.IsEmpty()?FString(TEXT("noted")):FString::Printf(TEXT("%s, %s"),*O.Bearing,O.bHeard?TEXT("heard"):TEXT("seen"));
    Trace.Add(Row);
    if(!O.PhotoId.IsEmpty())
    {
     const auto* Claim=Director->PhotoClaims().FindByPredicate([&](const FS47FreeformDirector::FPhotoClaim& P){return P.PhotoId==O.PhotoId;});
     FTrace Pic=Row;Pic.Key=O.Clock+1;Pic.Kind=TEXT("PHOTO");Pic.Text=Claim?Claim->Title+(Claim->bPaid?TEXT(", registered"):TEXT(", not yet registered")):FString(TEXT("exposed"));Trace.Add(Pic);
    }
   }
   for(const FString& L:C->Leads)if(Findings.Contains(L))
   {
    FTrace Row;Row.Day=FindingDays.FindRef(L);Row.Key=8*3600.+Row.Day*86400.-1;Row.Kind=TEXT("RECORD");
    FString First=Findings.FindRef(L);int32 Stop=INDEX_NONE;if(First.FindChar(TEXT('.'),Stop)&&Stop>0)First=First.Left(Stop+1);
    Row.Text=First;Trace.Add(Row);
   }
   if(Tips)for(const FS47Witness::FRec& R:Tips->Witness().Records())
   {
    const FS47Places::FDef* Def=FS47Places::Def(R.PlaceId);
    if(!Def||!(Here.Contains(FString(Def->Title).ToLower())||Here.Contains(R.PlaceId.ToLower())))continue;
    FTrace Row;Row.Key=R.Clock;Row.Clock=R.Clock;Row.Day=FMath::Max(1,int32(FMath::FloorToDouble((R.Clock-8*3600)/SecondsPerDay))+1);Row.Place=Def->Title;Row.Kind=TEXT("STATEMENT");Row.Text=FS47Witness::NoteText(R);Trace.Add(Row);
   }
   if(!Trace.IsEmpty())
   {
    Trace.StableSort([](const FTrace& A,const FTrace& B){return A.Key>B.Key;});
    FString Book=TEXT("TRACE BOOK / what you gathered here, newest first");const int32 Shown=FMath::Min(8,Trace.Num());
    for(int32 Index=0;Index<Shown;++Index)Book+=TEXT("\n")+TraceBookLine(Trace[Index].Day,Trace[Index].Clock,Trace[Index].Place,*Trace[Index].Kind,Trace[Index].Text);
    if(Trace.Num()>Shown)Book+=FString::Printf(TEXT("\nEARLIER / %d MORE, EACH STILL IN YOUR NOTEBOOK"),Trace.Num()-Shown);
    Notes.Insert(Book,0);NoteCount+=Shown;
   }
  }
  if(!Notes.IsEmpty())Left.Add(T::Button(FString::Printf(TEXT("%s CASE NOTES (%d)\nTRACES / LEADS / RECORDS"),bCaseDetails?TEXT("[-]"):TEXT("[+]"),NoteCount),[this,Line]{bCaseDetails=!bCaseDetails;OpenCaseBoard(Line);},false));
  else bCaseDetails=false;
 }
 // ---- Right: NEXT, the explanations, the tests, the selected test, the key. ----
 Right.Add(T::Small(FString::Printf(TEXT("CASE / %s / %s"),D.Title,*CaseKind(Line))));
 Right.Add(T::Heading(CaseNextLine(Line),22));
 {
  TArray<T::FRow> Ex;
  for(int32 Cause=0;Cause<FS47FreeformCases::CauseCount;++Cause)
  {
   // The first retained result that rules it out names its test; the sheet just collected is marked (as CaseSheetHead).
   int32 By=-1;for(int32 Test=0;Test<FS47FreeformCases::TestCount;++Test)if(C->Results[Test].bDone&&FS47FreeformCases::PresentFor(Line,Cause,Test)!=C->Results[Test].bPresent&&(By<0||C->Results[Test].Day<C->Results[By].Day))By=Test;
   const bool bOut=Cases.Excluded(Line,Cause);
   const bool bFell=Mark>=0&&Mark<FS47FreeformCases::TestCount&&C->Results[Mark].bDone&&FS47FreeformCases::PresentFor(Line,Cause,Mark)!=C->Results[Mark].bPresent&&bOut;
   FString Named=Wrap(FString(D.Causes[Cause]).ToUpper(),25);Named.ReplaceInline(TEXT("\n"),TEXT("\n   ")); // the letter before the first line only
   T::FRow R;R.Cells={FString(Letter[Cause])+TEXT("  ")+Named,Wrap(CaseStanding(Line,Cause),20)};
   TArray<FString> Marks;if(bFell)Marks.Add(TEXT("THIS SHEET"));else if(bOut&&By>=0)Marks.Add(FString::Printf(TEXT("TEST %d"),By+1));
   if(C->Hunch==Cause&&!C->bConcluded)Marks.Add(TEXT("HUNCH"));
   R.Badge=FString::Join(Marks,TEXT(" / "));
   // Clicking an explanation notes it as the working hunch: a note, never evidence (CHAIN-3-20261109).
   if(!C->bConcluded&&!bOut)R.Action=[this,Line,Cause]{SetCaseHunch(Line,Cause);OpenCaseBoard(Line);};
   Ex.Add(R);
  }
  Right.Add(T::Table({},{.60f,.40f},Ex,19));
  if(!C->bConcluded)Right.Add(T::Small(!D.Mystery&&C->Hunch<0?TEXT("YOUR WORKING HUNCH (OPTIONAL) / click an explanation to note it. Then each test says what it would show if you're right."):TEXT("YOUR WORKING HUNCH (OPTIONAL) / click an explanation to note it; just a note for yourself"))); // D45-20261110: plain words, no "not evidence"
 }
 TArray<S47Kit::FW> Acts;
 if(bCaseDetails&&!Notes.IsEmpty())
 {
  // The notes unfolded, in place of the tests: the whole question first. Only this box scrolls; the glass does not.
  TSharedRef<SScrollBox> List=SNew(SScrollBox);
  List->AddSlot()[T::Small(Cases.QuestionText(Line))];
  for(const FString& N:Notes)List->AddSlot()[T::Small(N)];
  Right.Add(T::Panel(TEXT("CASE NOTES"),SNew(SBox).HeightOverride(330)[List]));
 }
 else
 {
  struct FCard{int32 Test=-1;bool bDone=false,bCanPlan=false,bAccess=true;FString State,Unavailable,Need;};
  TArray<FCard> Cards;
  for(int32 Test=0;Test<FS47FreeformCases::TestCount;++Test)
  {
   const auto& Res=C->Results[Test];FCard K;K.Test=Test;K.bDone=Res.bDone;
   K.Unavailable=FS47FreeformCases::UnavailableReason(Line,Test,bHaveLogger2);
   // A control the station cannot carry out is not offered at all. A result or a retained plan from an older save still shows.
   if(!K.Unavailable.IsEmpty()&&!Res.bDone&&C->Planned!=Test)continue;
   K.bAccess=!(D.Tests[Test].Access&&!Cases.HasAccess(D.Tests[Test].Access));
   K.Need=LandTestNeeds(Line,Test); // LOOP-WHOLE-20261109
   K.bCanPlan=!bCold&&K.Unavailable.IsEmpty()&&!Res.bDone&&!C->bConcluded&&C->Planned<0&&Remaining>1&&K.bAccess&&K.Need.IsEmpty();
   K.State=Res.bDone?FString::Printf(TEXT("RESULT DAY %d"),Res.Day):FString(!K.Unavailable.IsEmpty()?TEXT("NOT HERE"):!K.bAccess?TEXT("NO ACCESS YET"):!K.Need.IsEmpty()&&C->Planned!=Test?TEXT("NEEDS KIT")
    :C->Planned==Test?TEXT("PLANNED"):C->bConcluded?TEXT("NOT RUN"):bCold?TEXT("TRAIL COLD"):Remaining<=1?TEXT("NOT NEEDED"):TEXT("OPEN"));
   Cards.Add(K);
  }
  // The selected card: the one picked, else the plan, the sheet just collected, the first that can be planned, the first.
  int32 Pick=-1;
  auto Index=[&Cards](int32 Test){return Test<0?INDEX_NONE:Cards.IndexOfByPredicate([Test](const FCard& K){return K.Test==Test;});};
  if(Index(BoardPick)!=INDEX_NONE)Pick=BoardPick;
  else if(Index(C->Planned)!=INDEX_NONE)Pick=C->Planned;
  else if(Index(Mark)!=INDEX_NONE)Pick=Mark;
  else if(const FCard* First=Cards.FindByPredicate([](const FCard& K){return K.bCanPlan;}))Pick=First->Test;
  else if(!Cards.IsEmpty())Pick=Cards[0].Test;
  BoardPick=Pick;BoardPickLine=Line;
  if(!Cards.IsEmpty())
  {
   TArray<T::FRow> TestRows;
   for(const FCard& K:Cards)
   {
    T::FRow R;R.Cells={FString(D.Tests[K.Test].Name),K.State};R.bSelected=K.Test==Pick;if(K.Test==Mark)R.Badge=TEXT("THIS SHEET");
    const int32 Test=K.Test;R.Action=[this,Line,Test]{BoardPick=Test;BoardPickLine=Line;OpenCaseBoard(Line);};
    TestRows.Add(R);
   }
   Right.Add(T::Table({TEXT("TESTS"),TEXT("STATE")},{.58f,.42f},TestRows,19));
  }
  if(const FCard* K=Cards.FindByPredicate([Pick](const FCard& X){return X.Test==Pick;}))
  {
   const auto& Res=C->Results[K->Test];const auto& TD=D.Tests[K->Test];
   FString Words=FString::Printf(TEXT("TEST %d / %s%s%s"),K->Test+1,*FString(TD.Name).ToUpper(),TD.Instruction[0]?TEXT(": "):TEXT(""),TD.Instruction);
   if(Res.bDone)Words+=FString::Printf(TEXT("\nRESULT DAY %d / %s%s"),Res.Day,*FS47FreeformCases::TestResultText(Line,K->Test,Res.bPresent),K->Test==Mark?TEXT("  << THIS SHEET"):TEXT(""));
   else if(!K->Unavailable.IsEmpty())Words+=TEXT("\nUNAVAILABLE / ")+K->Unavailable+(C->Planned==K->Test?TEXT(" Cancel this retained plan to choose another control."):TEXT(""));
   else if(!K->bAccess)Words+=TEXT("\nNEEDS / the loan field logger (conclude THE REPEATED INTERVAL) / NOT YET");
   else if(!K->Need.IsEmpty()&&C->Planned!=K->Test)Words+=TEXT("\nNEEDS / ")+K->Need+TEXT(" / NOT YET");
   else if(Line==FS47FreeformCases::LandLine)Words+=FString(TEXT("\nNEEDS / "))+(K->Test==FS47FreeformCases::LandCamera?TEXT("the trip camera"):K->Test==FS47FreeformCases::LandNeedle?TEXT("the survey magnetometer"):TEXT("the loan field logger"))+TEXT(" / HAVE");
   if(!Res.bDone&&K->Unavailable.IsEmpty())
   {
    if(C->Planned==K->Test)Words+=TEXT("\nPLANNED / ")+C->PlannedJob+TEXT("\n")+CaseWindowText(Line);
    else if(bCold&&!C->bConcluded)Words+=TEXT("\nTRAIL COLD / too long since that night for a fresh control to show anything");
   }
   Right.Add(T::Small(Words));
   // CHAIN-3-20261109: a tested case says what each explanation predicts once a working hunch is noted, or the result is in.
   const bool bShowPrediction=D.Mystery||C->Hunch>=0||C->bConcluded||Res.bDone;
   if(bShowPrediction)
   {
    TArray<FString> Ifs;for(int32 Cause=0;Cause<FS47FreeformCases::CauseCount;++Cause)Ifs.Add(FString::Printf(TEXT("IF %s %s: %s"),Letter[Cause],*FString(D.Short[Cause]).ToUpper(),*FS47FreeformCases::PredictionFor(Line,K->Test,Cause)));
    Right.Add(T::Small(FString::Join(Ifs,TEXT(" / "))));
   }
   else Right.Add(T::Small(TEXT("NOTE YOUR WORKING HUNCH FIRST: CLICK AN EXPLANATION ABOVE. THEN THE BOARD SHOWS WHAT EACH ONE PREDICTS HERE.")));
   if(K->bCanPlan){const int32 Test=K->Test;Acts.Add(T::Button(TEXT("Plan this test: ")+FString(TD.Name),[this,Line,Test]{PlanCaseTest(Line,Test);OpenCaseBoard(Line);},true));}
  }
 }
 if(FieldPlan>=0)
 {
  Acts.Add(T::Button(FieldFocus==Line?TEXT("FOLLOWING THIS FIELD TEST"):TEXT("Follow this field test / retain observing job"),[this,Line]{FocusFieldTest(Line);OpenCaseBoard(Line);},FieldFocus==Line));
  Acts.Add(T::Button(TEXT("Cancel this field plan"),[this,Line]{CancelFieldTest(Line);OpenCaseBoard(Line);},false));
 }
 else if(C->Planned>=0&&!C->bConcluded)Acts.Add(T::Button(TEXT("Cancel this planned test / no result, no reward"),[this,Line]{CancelCaseJob(Line);OpenCaseBoard(Line);},false));
 double WaitTarget=0;if(CaseWaitTarget(Line,WaitTarget))Acts.Add(T::Button(TEXT("Wait until five minutes before this window / review consequences"),[this,WaitTarget]{OpenWaitConfirmation(WaitTarget);},true));
 // A trial case concludes here, once the tests leave one explanation.
 if(!D.Mystery&&!C->bConcluded&&Remaining==1)for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)if(!Cases.Excluded(Line,I))Acts.Add(T::Button(FString(TEXT("Register supported conclusion: "))+D.Causes[I],[this,Line,I]{ConcludeCase(Line,I);OpenCaseBoard(Line);},true));
 if(!Acts.IsEmpty())Right.Add(S47Kit::Row(Acts));
 if(FieldPlan>=0)Right.Add(T::Small(TEXT("CANCEL / CLEARS THIS FIELD PLAN ONLY / NO RESULT, NO REWARD / RETAINED EVIDENCE UNCHANGED")));
 // ---- The filing (left, under the case), and what a closed case says. ----
 if(!C->bConcluded&&D.Mystery)
 {
  // Critic round 5: a filing with nothing behind it yet (one trace, no control) is said to be one before it goes in, and asked
  // again: Dr. Ward does not count it (FS47Predecessor::Count), and the line is closed for good once filed.
  const bool bBacked=CaseSupported(Line);const bool bFileNext=CaseStep(Line).Step==ECaseStep::File;
  if(!bBacked&&FileAskLine==Line&&FileAskCause>=0&&FileAskCause<FS47FreeformCases::CauseCount&&!Cases.Excluded(Line,FileAskCause))
  {
   const int32 Cause=FileAskCause;
   Left.Add(T::Small(FString::Printf(TEXT("FILE ANYWAY? / %s, on what you have now. It cannot be taken back, and it will not count in the review."),D.Causes[Cause])));
   Left.Add(T::Button(FString(TEXT("File anyway: "))+D.Short[Cause],[this,Line,Cause]{FileAskLine=FileAskCause=-1;ConcludeCase(Line,Cause);OpenCaseBoard(Line);},false));
   Left.Add(T::Button(TEXT("Not yet"),[this,Line]{FileAskLine=FileAskCause=-1;OpenCaseBoard(Line);},true));
  }
  else
  {
   Left.Add(T::Small(bBacked?TEXT("FILE YOUR ASSESSMENT / acted on as filed; not checked first"):Line==FS47FreeformCases::LandLine?TEXT("FILE YOUR ASSESSMENT / one night only: Dr. Ward will not count it yet"):TEXT("FILE YOUR ASSESSMENT / one trace only: Dr. Ward will not count it yet")));
   for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)if(!Cases.Excluded(Line,I))Left.Add(T::Button(FString(TEXT("File assessment: "))+D.Short[I],[this,Line,I]
   {
    if(!CaseSupported(Line)){FileAskLine=Line;FileAskCause=I;OpenCaseBoard(Line);return;}
    FileAskLine=FileAskCause=-1;ConcludeCase(Line,I);OpenCaseBoard(Line);
   },bFileNext));
  }
 }
 else if(C->bConcluded&&D.Mystery)Left.Add(T::Small(C->bReplied?TEXT("ASSESSMENT FILED / The reply is in your documents."):TEXT("ASSESSMENT FILED / The reply comes in the morning.")));
 else if(C->bConcluded)Left.Add(T::Small(TEXT("CASE CLOSED / The records reply is in your documents.")));
 if(!Feedback.IsEmpty())Left.Add(T::Small(Feedback));
 Rows.Add(T::Columns({{.27f,S47Kit::Col(Left,6)},{.73f,S47Kit::Col(Right,4)}},14));
 // The rules, once, small: how it closes and what closing brings (Tom, 9 November, DECISIONS).
 {
  FString Rules=C->bConcluded?CaseClosesWhen(Line):FString(D.Mystery?TEXT("CLOSES WHEN YOU FILE, ON ANY EVIDENCE"):TEXT("CLOSES WHEN YOUR TESTS RULE OUT ALL BUT ONE"));
  if(!C->bConcluded){FString On=CaseOnClose(Line);On.RemoveFromStart(TEXT("ON CLOSE: "));Rules+=TEXT(" / ")+On;if(D.Mystery)Rules+=TEXT(" / A FILING IS FINAL");}
  Rows.Add(T::Small(Rules));
 }
 OpenTerminal(3,TEXT("OPS / CASE BOARD"),OpsShell(2,S47Kit::Col(Rows,4)));
}
bool FS47FreeformSession::Use(const FString& Id)
{
 if(!bActive)return false;
 if(RefuseLocked(Id))return true; // FIX-20261110: the station's doors, before the keys are out of the mailbox
 if(DoorLoop&&DoorLoop->Use(Id))return true; // N1110 (N1110-6): an outer door of SARO that lets the player in at the other
 if(Fatigue&&Id==TEXT("diner:waitress"))Fatigue->AtDiner(); // the cup she pours is drunk (M5 critic 3; FS47Fatigue checks she is there)
 if(Townsfolk&&Townsfolk->Use(Id))return true; // the Mesa's waitress (TOWNSFOLK-20261108)
 if(AudioDesk&&AudioDesk->Use(Id))return true;
 if(Field&&Field->Use(Id))return true;
 if(Id==TEXT("crtCenter")){OpenReceiver();return true;}
 if(Id==TEXT("crtLeft")){OpenStatus();return true;}
 if(Id==TEXT("crtRight")){OpenTracking();return true;}
 // TELEX-20261109: a printed paper is torn off by hand, power or not; one still printing is waited for.
 if(Id==TEXT("telex")&&TelexReady()){TearOffTelex();return true;}
 if(Id==TEXT("telex")&&!TelexSheets.IsEmpty()&&HasPower()){Feedback=TelexLine()+TEXT(". The telex is still printing.");Changed(Feedback);return true;}
 if(Id==TEXT("opsTerminal")||Id==TEXT("telex")||Id==TEXT("logbook")||Id==TEXT("workOrder")||Id==TEXT("anomaly:shiftlog"))
 {
  if(!HasPower()&&(Id==TEXT("logbook")||Id==TEXT("workOrder")||Id==TEXT("anomaly:shiftlog"))){OpenArchive(false);return true;}
  if(!RequirePower())return true;
  if(Id==TEXT("telex")&&Director&&Director->Use(TEXT("ff:telex")))return true;
  // CASES-OPS-20261109: a result sheet just collected: OPS comes up on its case, the explanation that fell marked (OpenCaseBoard).
  if(Id==TEXT("opsTerminal")&&BoardLine>=0){const int32 Line=BoardLine;BoardLine=-1;Feedback.Empty();OpenCaseBoard(Line);return true;}
  OpsSection=0;OpenOperations();return true;
 }
 if(Id==TEXT("decoder")||Id==TEXT("signalProcessor")||Id==TEXT("decoderDesk")){OpenTapePanel();return true;}
 if(Id==TEXT("rack"))
 {
  if(CurrentJob()&&CurrentJob()->Phenomenon==TEXT("wrong_source"))Changed(NextAction());
  else if(!StartControl())Changed(NextAction());return true;
 }
 if(Id==TEXT("printer")){Feedback.Empty();if(!CollectChart())Changed(Feedback.IsEmpty()?TEXT("No paper waiting. ")+NextAction():Feedback);return true;}
 if(Id==TEXT("binder")){OpenArchive();return true;}
 if(Id==TEXT("excBinder")||Id==TEXT("serviceRecord")){OpenArchive(false);return true;}
 if(Id==TEXT("phone")||Id==TEXT("recPhone")){if(FS47StoryDirector::UsePhone(M))return true; /* STORY-ON-FREEFORM-01 step 5: a Story night's call */ if(Director&&Director->Use(TEXT("ff:phone")))return true;if(Predecessor&&Predecessor->UsePhone())return true;Changed(TEXT("SARO operations line. No scheduled call. Incoming messages are retained at the telex."));return true;}
 if(Fatigue&&Fatigue->Use(Id))return true; // the coffee (FATIGUE-20261108)
 return Director&&Director->Use(Id);
}
FString FS47FreeformSession::Label(const FString& Id)const
{
 if(!bActive)return FString();
 // TELEX-20261109
 if(Id==TEXT("telex")&&TelexReady())return TEXT("Tear off the telex");
 if(Id==TEXT("telex")&&!TelexSheets.IsEmpty()&&HasPower())return TEXT("The telex is printing");
 if(!HasPower()&&(Id==TEXT("crtCenter")||Id==TEXT("crtLeft")||Id==TEXT("crtRight")||Id==TEXT("opsTerminal")||Id==TEXT("telex")||Id==TEXT("decoder")||Id==TEXT("signalProcessor")||Id==TEXT("decoderDesk")))return TEXT("Instrument unpowered / restore feeder or generator");
 if(AudioDesk){const FString L=AudioDesk->Label(Id);if(!L.IsEmpty())return L;}
 if(Field){const FString L=Field->Label(Id);if(!L.IsEmpty())return L;}
 if(Id==TEXT("crtCenter"))return TEXT("Use the receiver / selected daily observation");
 if(Id==TEXT("crtLeft"))return TEXT("Inspect array status and individual drives");
 if(Id==TEXT("crtRight"))return TEXT("Command tracking for the selected source");
 if(Id==TEXT("opsTerminal"))return TEXT("Daily work, registration and operating budget");
 if(Id==TEXT("telex")||Id==TEXT("logbook")||Id==TEXT("workOrder")||Id==TEXT("anomaly:shiftlog"))return TEXT("Read the daily work order and incoming messages");
 if(Id==TEXT("phone")||Id==TEXT("recPhone"))return TEXT("Check the operations line");
 if(Id==TEXT("decoder")||Id==TEXT("signalProcessor")||Id==TEXT("decoderDesk"))return TEXT("Reel deck / compare recordings");
 if(Id==TEXT("rack")){const FJob* RJ=CurrentJob();return RJ&&PlainAnomaly(*RJ)?TEXT("Receiver rack / the take's 8-second reference (also kept by RETAIN TAKE on the receiver)"):TEXT("Record an independent receiver control");}
 if(Id==TEXT("printer"))return PrinterHint();
 if(Id==TEXT("binder")||Id==TEXT("excBinder")||Id==TEXT("serviceRecord"))return TEXT("File originals and read the station's records");
 if(Fatigue){const FString L=Fatigue->Label(Id);if(!L.IsEmpty())return L;}
 return Director?Director->Label(Id):FString();
}
// ---- First shift tutorial ----------------------------------------------------------------------
// One real routine job end to end, at the real stations: accept at OPS, point, record, collect the
// original and register (paid), exactly like later routine work. Older training packets remain fileable.
void FS47FreeformSession::AcceptTutorialJob()
{
 if(Tutorial!=1||!CurrentJob()||CurrentJob()->Id!=TutorialJobId)return;
 Tutorial=2;
 Changed(FString::Printf(TEXT("FIRST SHIFT / Job %s accepted. Point the array, record, collect the original at the printer, then register it here at OPS: $20."),*CurrentJob()->Id));
}
void FS47FreeformSession::StepTutorial()
{
 if(Tutorial<=1)return;
 const FJob* J=Work.FindByPredicate([this](const FJob& X){return X.Id==TutorialJobId;});if(!J)return;
 const int32 Step=J->Stage==EJobStage::New?2:J->Stage==EJobStage::Pointed?3:J->Stage<=EJobStage::Chart?4:J->Stage==EJobStage::Recorded?5:J->Stage<EJobStage::Filed?6:7;
 if(Step==Tutorial)return;
 if(Step==7)
 {
  Tutorial=0;RecordEvidence(TEXT("first-shift"),J->PacketFeed>0?TEXT("FIRST SHIFT / legacy training observation: pointed, recorded, original retained, registered and its training packet filed."):TEXT("FIRST SHIFT / one normal routine observation: pointed, recorded, original retained and registered for payment at OPS. Physical case filing is introduced when an unusual result needs controls."));
  if(M&&M->UI)M->UI->ClearToasts();
  // M7-HUD-20261109: the whole next step stays on the OPS screen; the toast says the shift is done, and the HUD's card comes in with
  // what is next (Ward's letter in the box at the road, then the day's work).
  Feedback=TEXT("FIRST SHIFT COMPLETE / Your normal observation is registered and paid. ")+AvailableActivities();Changed(TEXT("FIRST SHIFT COMPLETE / Your normal observation is registered and paid."));
  return;
 }
 static const TCHAR* Why[]={nullptr,nullptr,nullptr,
  TEXT("Pointed. The dishes now follow the source across the sky."),
  TEXT("Recording. Keep the receiver tuned; the chart prints when it finishes."),
  TEXT("Original retained. Registering it at OPS pays the job."),
  TEXT("Registered and paid. The printed packet goes in the binder to close the job.")};
 if(Step>Tutorial&&Step<UE_ARRAY_COUNT(Why)&&Why[Step])Changed(FString(TEXT("FIRST SHIFT / "))+Why[Step]);
 Tutorial=Step;
}
// FIX-20261110 (Tom, points 4 and 8): the ways into the station. Every one is shut at a new game (US47Flow), so a locked latch on
// each keeps the operator out until the letter and its keys are in hand. Closing is never refused.
bool FS47FreeformSession::StationDoor(const FString& Id)
{
 return Id==TEXT("exitDoor")||Id==TEXT("doorEast")||Id==TEXT("labDoor")||Id==TEXT("eastwing:personnel-door")||Id==TEXT("eastwing:service-door")||Id.StartsWith(TEXT("eastwing:garage-door-"));
}
bool FS47FreeformSession::StationLocked()
{
 if(!bStationLocked)return false;
 // No letter left to find the keys with (a box emptied some other way, or no post in this run): the latch is simply open.
 if(!Mail||!Mail->Holds(TEXT("ward:letter"))){bStationLocked=false;bDarkTold=true;UE_LOG(LogTemp,Display,TEXT("S47: station unlocked: no letter in the box to hold its keys"));}
 return bStationLocked;
}
void FS47FreeformSession::LockStation()
{
 if(!bActive||!Mail||!Mail->Holds(TEXT("ward:letter")))return;
 bStationLocked=true;bDarkTold=false;
 UE_LOG(LogTemp,Display,TEXT("S47: station locked: the keys are in the mailbox with Dr. Ward's letter"));
}
void FS47FreeformSession::StationKeyFound()
{
 if(!bStationLocked)return;
 bStationLocked=false;
 if(M&&M->Audio)
 {
  // A ring of keys tipped out of an envelope into the hand: a handful of small bright strikes, none of them a tone.
  const float Ring[]={3100.f,4350.f,3720.f,5200.f,2890.f,4680.f};const float At[]={.42f,.47f,.55f,.60f,.71f,.78f};
  for(int32 I=0;I<UE_ARRAY_COUNT(Ring);++I)M->Audio->Beep(Ring[I],.035f,.022f,At[I],TEXT("triangle"));
  M->Audio->Sfx(TEXT("metalTap"),FS47Sound{.Gain=.22f,.Rate=1.9f,.When=.5f});
 }
 if(M&&M->UI)M->UI->Toast(TEXT("A ring of keys slides out of the envelope with the letter: SARO, every outside door. The station is yours to open."),7);
 UE_LOG(LogTemp,Display,TEXT("S47: station keys taken out of the mailbox, day %d"),DayNumber);
 if(M)M->RequestAutosave();
}
bool FS47FreeformSession::RefuseLocked(const FString& DoorId)
{
 if(!bStationLocked||!StationDoor(DoorId)||!StationLocked())return false;
 if(M&&M->Audio)M->Audio->Sfx(M->Audio->Or(TEXT("door_latch"),TEXT("switch")),FS47Sound{.Gain=.5f,.Rate=.85f});
 if(M&&M->UI)M->UI->Toast(TEXT("LOCKED. Nobody is in, and nobody gave you a key. Dr. Ward's office said there would be a letter in the box at the road."),5);
 UE_LOG(LogTemp,Display,TEXT("S47: station door %s refused: locked, keys still in the mailbox"),*DoorId);
 return true;
}

FString FS47FreeformSession::TutorialObjective()const
{
 if(Tutorial==1)return TEXT("FIRST SHIFT 1/5 / OPS (corner PC): OPEN DAILY WORK AND ACCEPT YOUR FIRST JOB"); // whole on the HUD's card (critic round 5)
 const FJob* J=CurrentJob();if(!J)return FString();
 return FString::Printf(TEXT("FIRST SHIFT %d/%d / %s"),FMath::Clamp(Tutorial,2,6),J->Stage>=EJobStage::Registered?6:5,*JobNextAction(*J));
}

FString FS47FreeformSession::Objective()const
{
 PickRow.Reset(); // TASKS-TEN-20261113: set again by the ladder, when the step is one of the night's rows
 if(bDead)return TEXT("PAUSE MENU: LOAD A SAVED CASE TO CONTINUE AFTER CONTACT");
 // Stranded away from SARO: the only thing that matters is the way back. An empty tank at SARO stays a task below.
 if(Field&&!Field->HasVehicleFuel()&&M&&M->World&&M->World->Area!=TEXT("saro"))return TEXT("SERVICE TRUCK: TANK EMPTY. GET OUT AND USE THE TRUCK TO CALL ROADSIDE RETURN; IT COSTS MONEY AND A LONG WAIT");
 // STORY-ON-FREEFORM-01 (step 4): a Story night's objective goes on top; empty, and so nothing, in any other night.
 {const FString Story=FS47StoryDirector::ObjectiveOf(M);if(!Story.IsEmpty())return Story;}
 // TASKS-TEN-20261113 (change 2): a field test followed is the step, except while it only waits for its window (FieldWindowWaits)
 if(FieldFocus>=0&&!FieldWindowWaits(FieldFocus)){PickRow=CaseStep(FieldFocus).Task;return FieldNextAction(FieldFocus);}
 if(!HasPower()&&PrinterProgress()>=1)return PrinterHint();
 if(bPowerFault&&!HasPower()&&(!CurrentJob()||(CurrentJob()->Stage!=EJobStage::Chart&&CurrentJob()->Stage!=EJobStage::Packet)))
 {
  if(Field&&Field->EmergencyFuseAvailable()&&Field->InventoryCount(TEXT("fuel"))<=0&&Field->GeneratorHours()<=0)return TEXT("NO POWER, NO SPARE FUSE: THE CALICHE STORE COUNTER ISSUES ONE FEEDER-REPAIR FUSE ON CREDIT, DAY OR NIGHT");
  return TEXT("SERVICE BAY: MEASURE THE FEEDER, OR START THE FUELLED STANDBY GENERATOR OUTSIDE");
 }
 if(ServiceGuide==1&&bGuideFocus){const FString Guide=ServiceObjective();if(!Guide.IsEmpty())return Guide;}
 // SPAWN-MAILBOX-20261109 (Tom): a new game starts at the box at the road; Dr. Ward's letter in it comes first, then the HUD's card
 // points in to OPS in the control room (the first shift's first step), as before. Once inside, the shift's step whatever the box.
 if(bStationLocked&&Mail&&Mail->Holds(TEXT("ward:letter")))return TEXT("MAILBOX / The keys to the station are in the box.");
 if(bMailboxStart&&Tutorial==1&&Mail&&Mail->Holds(TEXT("ward:letter"))&&M&&M->World&&M->Player&&M->World->Area==TEXT("saro")&&!M->World->Indoors(M->Player->Pos))
  return TEXT("MAILBOX / A letter from Dr. Ward's office is in the box at the road.");
 if(Tutorial>0){const FString T=TutorialObjective();if(!T.IsEmpty())return T;}
 // A tip is only worth the top line when it is not in the way of the way home, the power or a recording under way.
 // It stands in the task list all the same.
 if(Director&&!Director->TipLine().IsEmpty()&&(!CurrentJob()||CurrentJob()->Stage==EJobStage::New||CurrentJob()->Stage==EJobStage::Filed))return Director->TipLine();
 return NextAction();
}
FString FS47FreeformSession::HudObjective()const
{
 const FString Full=Objective();
 FString Card;
 if(!Full.IsEmpty()&&Full==AvailableActivities(&Card)&&!Card.IsEmpty())return Card;
 // Stranded with an empty tank: what to do, in the card's words.
 if(Full.StartsWith(TEXT("SERVICE TRUCK: TANK EMPTY")))return TEXT("TRUCK // TANK EMPTY. GET OUT; USE THE TRUCK TO CALL ROADSIDE RETURN.");
 return S47HudGauges::CardLine(Full);
}
FString FS47FreeformSession::NextAction()const
{
 if(ServiceGuide==1&&bGuideFocus)return ServiceObjective();
 if(Tutorial>0)return TutorialObjective();
 if(FieldFocus>=0&&!FieldWindowWaits(FieldFocus)){PickRow=CaseStep(FieldFocus).Task;return FieldNextAction(FieldFocus);}
 if(!CurrentJob()||CurrentJob()->Stage==EJobStage::Filed)return AvailableActivities();
 // TASKS-TEN-20261113 (change 2): a window open now comes before a card only selected (not begun, or still waiting for its source);
 // work under way keeps its place (blocking now)
 {const FJob* J=CurrentJob();FString Open;if((J->Stage==EJobStage::New||EveningGated(*J))&&(WindowOpenStep(Open)||!TelexLine().IsEmpty()||(bPinLive&&!PinKey.IsEmpty())))return AvailableActivities();}
 if(const FJob* J=CurrentJob())PickRow=J->CaseTest>=0?CaseStep(J->CaseLine).Task:EveningGated(*J)?TEXT("card:")+J->Id:FString(TEXT("cards"));
 return ReceiverNextAction();
}
// RX-ANOMALY-FLOW-20261113 (Tom 13 November): after the take, RECORD on the receiver (retain it with its reference), the reel deck
// (play both, mark the extra ticks, RETAIN), then OPS.
FString FS47FreeformSession::PlainAnomalyNext(const FJob& J)
{
 if(J.bAudioReviewed)return TEXT("OPS (corner PC): CONFIRM THE ANOMALY IN DAILY WORK");
 if(RetainWaits(J))return TEXT("RECEIVER (centre PC): RECORD TAB, RETAIN THE TAKE WITH ITS 8 s REFERENCE");
 return TEXT("REEL DECK (wall table): PLAY TAKE AND REFERENCE, MARK THE TICKS, PRESS RETAIN");
}
FString FS47FreeformSession::JobNextAction(const FJob& JobValue)const
{
 const FJob* J=&JobValue;
 if(EveningGated(JobValue))return EveningLine(JobValue);
 if(J->CaseTest>=0&&J->Stage<=EJobStage::Pointed){const FString Missing=CasePrecondition(*J);if(!Missing.IsEmpty())return Missing;}
 if(M&&M->Web&&ArrayBlocked()&&(J->Stage<=EJobStage::Integrating||(J->Stage==EJobStage::Recorded&&J->Phenomenon==TEXT("wrong_source")&&(!J->bControl||J->ControlAt<8))))return ArrayMeasurementStatus();
 switch(J->Stage)
 {
  case EJobStage::New:return FS47Observing::Runs().IsValidIndex(J->Source)?FString(TEXT("TRACKING (right-hand PC): POINT THE ARRAY AT "))+FString(FS47Observing::Runs()[J->Source].Source).ToUpper():FString(TEXT("TRACKING (right-hand PC): POINT THE ARRAY AT THE SELECTED SOURCE"));
  case EJobStage::Pointed:if(&JobValue!=CurrentJob())return TEXT("TRACKING (right-hand PC): FOLLOW THIS JOB AND VERIFY ITS ACTUAL POINTING BEFORE RECORDING");return PointReady()&&Settle>=1?(ReceiverReady()&&AnomalyAhead()?TEXT("RECEIVER (centre PC): ANOMALY DETECTED. RECORD TAB: PRESS START RECORDING"):!SkyAcquired()?TEXT("RECEIVER (centre PC): ACQUIRE THE SOURCE ON THE SKY CHART, THEN TUNE"):TEXT("RECEIVER (centre PC): TUNE THE CARD AND PRESS START RECORDING")):TEXT("TRACKING (right-hand PC): WAIT FOR THE ARRAY TO SETTLE, THEN USE RECEIVER (centre PC)");
  case EJobStage::Integrating:return PlainAnomaly(*J)?TEXT("RECEIVER (centre PC): ANOMALY DETECTED. RECORDING; THEN RETAIN IT ON RECORD"):TEXT("RECEIVER (centre PC): KEEP TUNING; THEN THE ORIGINAL IS AT THE PRINTER"); // the printer stays on the card (critic round 5)
  case EJobStage::Chart:
   // GAMEPLAY-LOOP-01 (Tom, 12 November): ANOMALY DETECTED, record, reel deck, OPS, printer, binder. An extra-tick take has no chart
   // to fetch (an older save's waits here until OPS keeps it).
   if(PlainAnomaly(*J))return PlainAnomalyNext(*J);
   return J->ChartFeed<PrinterSeconds?PrinterHint():FString(TEXT("PRINTER: RETAIN THE ORIGINAL OBSERVATION CHART"));
  case EJobStage::Recorded:
   if(PlainAnomaly(*J))return PlainAnomalyNext(*J);
   if(J->bDuplicateRejected)return TEXT("OPS / DAILY WORK: SELECT ANOTHER OBSERVATION. THE OLD ORIGINAL IS KEPT; ITS UNCHECKED DUPLICATE WAS DROPPED, NO PAY.");
   if(!J->bAnomaly)return J->CaseTest>=0&&!J->bCaseRecorded?TEXT("OPS (corner PC): REGISTER THE ROUTINE RESULT AND ITS CASE TEST"):TEXT("OPS (corner PC): REGISTER THE ROUTINE RESULT");
   if(!J->bControl)return J->Phenomenon==TEXT("wrong_source")?TEXT("TRACKING (right-hand PC): MEASURE WEST, EAST AND CENTRE OF THE SOURCE"):TEXT("RECEIVER (centre PC), RECORD TAB: RECORD THE INTERNAL CONTROL");
   if(J->ControlAt<8)
   {
    if(J->Phenomenon==TEXT("wrong_source"))return TEXT("TRACKING (right-hand PC): WAIT FOR AUTOMATIC WEST, EAST AND CENTRE READINGS; THEN COMPARE AT OPS (corner PC), DAILY WORK");
    if(J->Phenomenon==TEXT("time_disagreement"))return TEXT("RECEIVER RACK: WAIT FOR THE AUTOMATIC INTERNAL CONTROL; THEN COMPARE CLOCK READINGS AT OPS (corner PC), DAILY WORK");
    if(J->Phenomenon==TEXT("duplicate_print"))return TEXT("RECEIVER RACK: WAIT FOR THE AUTOMATIC REGISTER CHECK; THEN WAIT FOR THE DUPLICATE TO FINISH AT THE PRINTER");
    return TEXT("RECEIVER RACK: WAIT FOR THE AUTOMATIC INTERNAL CONTROL");
   }
   if(J->Phenomenon==TEXT("duplicate_print")&&!J->bDuplicateCollected)return J->DuplicateFeed<PrinterSeconds?PrinterHint():FString(TEXT("PRINTER: COLLECT THE SEPARATE DUPLICATE WITHOUT ALTERING THE ORIGINAL; THEN COMPARE AT OPS (corner PC), DAILY WORK"));
   if(Specialized(J->Phenomenon))return J->bAudioReviewed?TEXT("OPS (corner PC): REGISTER THE SUPPORTED INSTRUMENTAL FINDING"):TEXT("OPS (corner PC): DAILY WORK / SELECT THIS JOB AND PRESS COMPARE AND REGISTER");
   return J->bAudioReviewed?TEXT("OPS (corner PC): REGISTER THE COMPARISON: SOURCE UNRESOLVED"):TEXT("REEL DECK (wall table): PLAY THE TAKE ONCE TO CONFIRM IT");
  case EJobStage::Registered:return J->PacketFeed<PrinterSeconds?PrinterHint():FString(PlainAnomaly(*J)?TEXT("PRINTER: COLLECT THE ANOMALY PACKET"):TEXT("PRINTER: COLLECT THE LABELLED REGISTERED PACKET"));
  case EJobStage::Packet:return TEXT("RECORDS ROOM: PLACE THE ORIGINAL PACKET IN THE BINDER");
  default:return TEXT("OPS (corner PC): SELECT DAILY WORK OR RECORDS");
 }
}
FString FS47FreeformSession::JobLocation(const FJob& J)const
{
 if(J.CaseTest>=0&&J.Stage<=EJobStage::Pointed&&!CasePrecondition(J).IsEmpty())
 {
  switch(CaseCondition(J.CaseLine,J.CaseTest))
  {
   case ECaseCondition::Generator:return TEXT("Behind the control room / standby generator");
   case ECaseCondition::RelaySwap:return TEXT("Caliche gas station / replacement relay counter");
   case ECaseCondition::DoorsSealed:return TEXT("SARO / south and east outside doors");
   default:break;
  }
 }

 if(M&&M->Web&&ArrayBlocked()&&(J.Stage<=EJobStage::Integrating||(J.Stage==EJobStage::Recorded&&J.Phenomenon==TEXT("wrong_source")&&(!J.bControl||J.ControlAt<8))))return TEXT("ARRAY STATUS / left-hand PC / inspect isolated antennas");
 if(EveningGated(J))return TEXT("Tracking PC / right-hand screen, from ")+FS47FreeformCases::Clock(EveningUp(J));
 if(J.Stage==EJobStage::New)return TEXT("Tracking PC / right-hand screen");
 if(J.Stage==EJobStage::Pointed)return &J==CurrentJob()&&PointReady()&&Settle>=1?TEXT("Receiver / centre screen"):TEXT("Tracking PC / right-hand screen");
 if(J.Stage==EJobStage::Integrating)return TEXT("Receiver / centre screen");
 // GAMEPLAY-LOOP-01: the extra-tick take goes from the receiver to the reel deck, then OPS; it has no chart to fetch
 if((J.Stage==EJobStage::Chart||J.Stage==EJobStage::Recorded)&&PlainAnomaly(J))return J.bAudioReviewed?TEXT("OPS / corner PC / DAILY WORK"):RetainWaits(J)?TEXT("Receiver / centre screen / RECORD tab"):TEXT("Reel deck / wall table"); // RX-ANOMALY-FLOW-20261113
 if(J.Stage==EJobStage::Chart||J.Stage==EJobStage::Registered)return TEXT("Printer / table by SERVICE YARD door, beneath the binder shelf");
 if(J.Stage==EJobStage::Packet)return TEXT("Binder / records room");
 if(J.bDuplicateRejected)return TEXT("OPS / DAILY WORK: retained legacy observation; select another job");
 if(J.Stage==EJobStage::Recorded&&J.bAnomaly&&!J.bAudioReviewed)
 {
  if(!J.bControl&&J.Phenomenon!=TEXT("wrong_source"))return TEXT("Receiver / centre screen, RECORD tab");
  if(!J.bControl||J.ControlAt<8)return J.Phenomenon==TEXT("wrong_source")?TEXT("Tracking PC / right-hand screen"):TEXT("Receiver rack / internal reference controls");
  if(J.Phenomenon==TEXT("duplicate_print")&&!J.bDuplicateCollected)return TEXT("Printer / table by SERVICE YARD door, beneath the binder shelf");
  if(!Specialized(J.Phenomenon))return TEXT("Reel deck / wall table");
 }
 return TEXT("OPS / corner PC / DAILY WORK");
}

// CRIT9: a chore open now is remembered by its kind; gone from the list since (done), its last line (or DoneLine) is added ticked
// off for the rest of the night, so that the note keeps it struck (S47TonightNote::FDone) as it does the cards filed.
void FS47FreeformSession::NoteChore(TArray<FS47Task>& Result,const TCHAR* Kind,const FString& OpenLine,const FString& DoneLine) const
{
 if(!OpenLine.IsEmpty()){ChoresOpen.Add(Kind,OpenLine);ChoresDone.Remove(Kind);return;}
 if(const FString* Was=ChoresOpen.Find(Kind)){ChoresDone.Add(Kind,DoneLine.IsEmpty()?*Was:DoneLine);ChoresOpen.Remove(Kind);}
 if(const FString* Done=ChoresDone.Find(Kind))Result.Add({*Done,true});
}
TArray<FS47Task> FS47FreeformSession::Tasks()const
{
 TArray<FS47Task> Result;
 {const FString Story=FS47StoryDirector::ObjectiveOf(M);if(!Story.IsEmpty())Result.Add({Story,false});} // STORY-ON-FREEFORM-01 (step 4): a Story night's objective, first
 if(Director){const FString Reported=Director->ReportedLine();if(!Reported.IsEmpty())Result.Add({Reported,Director->ReportDone()});} // PT-FEEDER-20261109
 if(Director){const FString Watch=Director->EveningLightsLine();if(!Watch.IsEmpty())Result.Add({Watch,false});} // D45-20261110: the first evening's appointment
 if(Places)for(const TPair<FString,bool>& Slip:Places->SlipRows(Clock()))Result.Add({Slip.Key,Slip.Value}); // D45-20261110: the slip's circle to look at, ticked once stood in
 if(Balloon){const FString Sonde=Balloon->TaskLine();if(!Sonde.IsEmpty())Result.Add({Sonde,false});} // D48-20261110: the balloon's instrument box and what to do with it
 if(Director){bool Ready=false;const FString Hint=Director->PhotoControlFieldHint(Ready);if(!Hint.IsEmpty())Result.Add({Hint,false});}
 if(Gear&&Gear->AnyLeft())Result.Add({Gear->TaskLine(),false});
 // TASKS-TEN-20261113 (change 3): what the station is ("SARO under review", "KEPT OPEN", "CLOSES") is no task: only her papers waiting
 if(Predecessor){const FString Line=Predecessor->TaskLine();if(!Line.IsEmpty()&&!IsStatusLine(Line))Result.Add({Line,false});}
 DossierTasks(Result); // LOOP-DOSSIER-GIVES-20261113: MARKS's extra bearing while it can come tonight, HABITS's hours (S47FreeformDossier.cpp)
 // CRIT9: a new night (or a clock gone back, a load) forgets the chores done; those that stop when done stay ticked off tonight.
 {const double C=Clock();const int32 N=NightOf(C);if(N!=ChoresNight||C+60<ChoresClock){ChoresOpen.Reset();ChoresDone.Reset();ChoresNight=N;}ChoresClock=C;}
 // TASKS-TEN-20261113 (change 3): the flag up for the carrier and a parcel due are no tasks: the map's mail flag and the kit say them
 if(Mail){const FString Post=Mail->TaskLine();const bool bPost=!Post.IsEmpty()&&!IsStatusLine(Post);if(bPost)Result.Add({Post,false});NoteChore(Result,TEXT("post"),bPost?Post:FString(),FString());}
 if(Round){const FString Rounds=Round->TaskLine();if(!Rounds.IsEmpty())Result.Add({Rounds,false});}
 if(Care&&!Care->StatusLine().IsEmpty()){const FString Steps=Care->Steps(InventoryCount(TEXT("spare")),InventoryCount(TEXT("water")));Result.Add({TEXT("TRUCK / ")+(Steps.IsEmpty()?Care->StatusLine():Steps),false});}
 if(Care)NoteChore(Result,TEXT("truck"),Care->StatusLine().IsEmpty()?FString():TEXT("TRUCK / ")+Care->StatusLine(),FString());
 const FString Feeder=TEXT("FEEDER / Measure, isolate and replace the control fuse; restore SARO power.");
 if(bPowerFault)Result.Add({Feeder,false});
 NoteChore(Result,TEXT("feeder"),bPowerFault?Feeder:FString(),FString());
 const FString Tank=TEXT("SERVICE TRUCK / Tank empty: use the truck for a stock fuel can, or the county's roadside return: a charge on the budget or the weekly sheet, and a long wait.");
 if(Field&&!Field->HasVehicleFuel())Result.Add({Tank,false});
 if(Field)NoteChore(Result,TEXT("tank"),Field->HasVehicleFuel()?FString():Tank,TEXT("SERVICE TRUCK / Tank filled."));
 if(Field&&Field->HasArrayFault()&&ServiceGuide!=1)Result.Add({Field->ServiceDishId()+TEXT(" / Diagnose at the status PC, replace the relay and run the local out-and-back test."),false});
 // The relay under the service guide is the guide's own line (ServiceGuide==2 is ticked off below): only the plain fault here.
 if(Field&&ServiceGuide!=1)NoteChore(Result,TEXT("relay"),Field->HasArrayFault()?Field->ServiceDishId()+TEXT(" / Diagnose at the status PC, replace the relay and run the local out-and-back test."):FString(),FString());
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)if(const FString Step=CaseStep(Line).Task;!Step.IsEmpty())Result.Add({Step,false}); // CASES-OPS-20261109
 if(Director)for(const TPair<FString,bool>& Trail:Director->MysteryTrailRows())Result.Add({Trail.Key,Trail.Value}); // PT-NOTEBOOK-20261109: ticked off, not dropped
 for(const FDaleAsk& Ask:DaleAskList)if(Ask.Night==NightOf(Clock()))Result.Add({Ask.Row,Ask.bDone}); // TASKS-TEN-20261113 (change 7): ticked off once done, gone at 08:00
 // LOOP-NIGHT-WATCH-20261113: in the night the night's cards first, the station goal after them (NightBeforeGoal)
 // TASKS-TEN-20261113 (change 3): a goal that only waits for daylight is no task at night (it is the half-line after the night's last line)
 const bool bNightFirst=NightBeforeGoal();const FString Goal=GoalWaits(WorldClock)?FString():StationGoalLine();
 if(!Goal.IsEmpty()&&!bNightFirst)Result.Add({Goal,false});
 if(ServiceGuide>0)Result.Add({ServiceGuide==2?FString(TEXT("Service work order / verified and archived")):ServiceObjective(),ServiceGuide==2});
 for(const FJob& J:Work)if(J.Day==DayNumber||J.Stage!=EJobStage::Filed)Result.Add({JobLine(J),J.Stage==EJobStage::Filed});
 if(!Goal.IsEmpty()&&bNightFirst)Result.Add({Goal,false});
 // TASKS-TEN-20261113 (change 3): "YOU / Tired" is no task: the tired state has its own signs (the eyelids, the kit's line, the yawns)
 return Result;
}
S47J::FObj FS47FreeformSession::Save()const
{
 auto O=S47J::New();if(bStationLocked)S47J::Set(O,TEXT("stationLocked"),true);if(bMailboxStart)S47J::Set(O,TEXT("mailboxStart"),true);if(DriveCheckDay>0&&DriveCheckAt<0){auto Drive=S47J::New();S47J::Set(Drive,TEXT("day"),DriveCheckDay);S47J::Set(Drive,TEXT("tone"),DriveCheckTone);S47J::Set(Drive,TEXT("result"),DriveCheckResult);S47J::Set(O,TEXT("driveCheck"),Drive);}S47J::Set(O,TEXT("fieldFocus"),FieldFocus);S47J::Set(O,TEXT("tutorial"),Tutorial);S47J::Set(O,TEXT("tutorialJobId"),TutorialJobId);S47J::Set(O,TEXT("jobSeed"),JobSeed);S47J::Set(O,TEXT("serviceGuide"),ServiceGuide);S47J::Set(O,TEXT("guideServiceBaseline"),GuideServiceBaseline);S47J::Set(O,TEXT("guideSupplyBaseline"),GuideSupplyBaseline);S47J::Set(O,TEXT("guideSupply"),bGuideSupply);S47J::Set(O,TEXT("guideFocus"),bGuideFocus);S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("clock"),WorldClock);if(WokenNight>0)S47J::Set(O,TEXT("wokenNight"),WokenNight);if(RousedNight>0)S47J::Set(O,TEXT("rousedNight"),RousedNight);if(RulesLive()||ReviewWeek>0)S47J::Set(O,TEXT("reviewWeek"),ReviewWeek);if(Carried>0)S47J::Set(O,TEXT("carried"),Carried);if(StandbyTotal>0)S47J::Set(O,TEXT("standbyPaid"),StandbyTotal);if(SideWeek>0)S47J::Set(O,TEXT("sideWeek"),SideWeek);if(SidePaid>0)S47J::Set(O,TEXT("sidePaid"),SidePaid);if(Repairs>0)S47J::Set(O,TEXT("repairs"),Repairs);S47J::Set(O,TEXT("cash"),Cash);S47J::Set(O,TEXT("loan"),Loan);S47J::Set(O,TEXT("lethal"),bDeadly);S47J::Set(O,TEXT("dead"),bDead);S47J::Set(O,TEXT("powerFault"),bPowerFault);if(bPowerFault&&SkyFaultUntil>0)S47J::Set(O,TEXT("skyFaultUntil"),SkyFaultUntil);{TArray<FString> Seen;for(int32 I=0;I<S47StationFiles::Count()&&I<32;++I)if(StationFilesSeen&(1u<<I))Seen.Add(S47StationFiles::Get(I).Key);S47J::Set(O,TEXT("stationFilesSeen"),Seen);}S47J::Set(O,TEXT("anomaly"),bAnomaly);S47J::Set(O,TEXT("phenomenon"),PhenomenonId);
 S47J::Set(O,TEXT("journal"),Journal.Save());SaveNotePlan(O);S47J::Set(O,TEXT("economyRevision"),2);
 SaveBinder(O); // RECORDS-BINDER: "binderFiled", only when a packet has gone in
 SaveDossier(O); // EVIDENCE-DOSSIER: "dossier", only once something is tagged
 S47LocationCards::Save(this,DayNumber,O); // critic 12 Nov: "locationCards", today's only
 S47J::Set(O,TEXT("selected"),Selected);S47J::Set(O,TEXT("audioSource"),AudioSourceIndex);S47J::Set(O,TEXT("audioCursor"),AudioCursor);S47J::Set(O,TEXT("settle"),Settle);if(!ArrayJob.IsEmpty())S47J::Set(O,TEXT("arrayJob"),ArrayJob);S47J::Set(O,TEXT("cross"),CrossPoint);S47J::Set(O,TEXT("crossSample"),CrossSample);S47J::Set(O,TEXT("crossRunning"),bCrossRunning);
 TArray<TSharedPtr<FJsonValue>> Jobs;
 for(const FJob& J:Work)
 {
  auto A=S47J::New();S47J::Set(A,TEXT("chartFeed"),J.ChartFeed);S47J::Set(A,TEXT("packetFeed"),J.PacketFeed);S47J::Set(A,TEXT("duplicateFeed"),J.DuplicateFeed);S47J::Set(A,TEXT("id"),J.Id);S47J::Set(A,TEXT("day"),J.Day);S47J::Set(A,TEXT("slot"),J.Slot);S47J::Set(A,TEXT("source"),J.Source);S47J::Set(A,TEXT("phenomenon"),J.Phenomenon);S47J::Set(A,TEXT("stage"),int32(J.Stage));S47J::Set(A,TEXT("elapsed"),J.Elapsed);S47J::Set(A,TEXT("quality"),J.Quality);S47J::Set(A,TEXT("receivedLevel"),J.ReceivedLevel);S47J::Set(A,TEXT("anomaly"),J.bAnomaly);S47J::Set(A,TEXT("control"),J.bControl);S47J::Set(A,TEXT("reviewed"),J.bAudioReviewed);S47J::Set(A,TEXT("printed"),J.bPrinted);S47J::Set(A,TEXT("paid"),J.bPaid);S47J::Set(A,TEXT("controlAt"),J.ControlAt);S47J::Set(A,TEXT("originalCovered"),J.OriginalCovered);S47J::Set(A,TEXT("controlCovered"),J.ControlCovered);S47J::Set(A,TEXT("markStart"),J.MarkStart);S47J::Set(A,TEXT("markEnd"),J.MarkEnd);S47J::Set(A,TEXT("receiverClock"),J.ReceiverClock);S47J::Set(A,TEXT("referenceClock"),J.ReferenceClock);S47J::Set(A,TEXT("controlReceiverClock"),J.ControlReceiverClock);S47J::Set(A,TEXT("controlReferenceClock"),J.ControlReferenceClock);S47J::Set(A,TEXT("duplicateClock"),J.DuplicateClock);S47J::Set(A,TEXT("duplicateInterval"),J.DuplicateInterval);
  S47J::Set(A,TEXT("traceTicks"),J.TraceTicks);S47J::Set(A,TEXT("legacyTrace"),J.bLegacyTrace);
  if(J.Stage==EJobStage::Pointed&&J.bSky)S47J::Set(A,TEXT("sky"),true); // RX-ACQUIRE-SKY-20261113 (optional)
  if(J.Stage==EJobStage::Pointed&&J.TakeDecided>=0){auto Take=S47J::New();S47J::Set(Take,TEXT("odd"),J.TakeDecided);S47J::Set(Take,TEXT("out"),J.bTakeOut);S47J::Set(Take,TEXT("phen"),J.TakePhen);S47J::Set(A,TEXT("takeDecided"),Take);} // critic 12 Nov (optional)
  // LOOP-CAREFUL-PAYS-20261113, LOOP-SLIP-IN-PACKET-20261113 (optional): how the fix was made, the operator's own marks, and a take odd
  // because of what is out tonight, kept past the pointing for the packet's bearing
  if(J.Stage!=EJobStage::New&&J.SkyBy>0)S47J::Set(A,TEXT("skyBy"),J.SkyBy);
  if(J.bOwnMarks)S47J::Set(A,TEXT("ownMarks"),true);
  if(J.Stage>EJobStage::Pointed&&J.bTakeOut)S47J::Set(A,TEXT("takeOut"),true);
  if(J.CaptureOriginal.Enabled())S47J::Set(A,TEXT("captureOriginal"),J.CaptureOriginal.Save());
  if(J.CaptureControl.Enabled())S47J::Set(A,TEXT("captureControl"),J.CaptureControl.Save());
  if(J.RecordingSoundVersion==1||J.RecordingSoundVersion==2)
  {
   auto Sound=S47J::New();S47J::Set(Sound,TEXT("v"),J.RecordingSoundVersion);S47J::Set(Sound,TEXT("gain"),J.RecordingGain);
   S47J::Set(Sound,TEXT("bandwidth"),J.RecordingBandwidth);S47J::Set(Sound,TEXT("quality"),J.RecordingQuality);S47J::Set(A,TEXT("recordingSound"),Sound);
  }
  S47J::Set(A,TEXT("beamPoint"),J.BeamPoint);S47J::Set(A,TEXT("beamSettle"),J.BeamSettle);S47J::Set(A,TEXT("beamSample"),J.BeamSample);S47J::Set(A,TEXT("beamAz"),J.BeamAz);S47J::Set(A,TEXT("beamEl"),J.BeamEl);S47J::Set(A,TEXT("beamAmplitude"),J.BeamAmplitude);S47J::Set(A,TEXT("beamVerified"),J.bBeamVerified);S47J::Set(A,TEXT("clockMeasured"),J.bClockMeasured);S47J::Set(A,TEXT("duplicateCollected"),J.bDuplicateCollected);S47J::Set(A,TEXT("duplicateRejected"),J.bDuplicateRejected);S47J::Set(A,TEXT("originalPage"),J.OriginalPage);S47J::Set(A,TEXT("originalStamp"),J.OriginalStamp);S47J::Set(A,TEXT("duplicatePage"),J.DuplicatePage);S47J::Set(A,TEXT("duplicateStamp"),J.DuplicateStamp);S47J::Set(A,TEXT("caseLine"),J.CaseLine);S47J::Set(A,TEXT("caseTest"),J.CaseTest);S47J::Set(A,TEXT("caseValid"),J.bCaseValid);S47J::Set(A,TEXT("caseRecorded"),J.bCaseRecorded);if(J.bDocked)S47J::Set(A,TEXT("docked"),true);if(J.PaidAmount>=0)S47J::Set(A,TEXT("paidAmt"),J.PaidAmount);if(J.bGenerated)
  {
   auto C=S47J::New();const auto& P=J.Card.Receiver;S47J::Set(C,TEXT("v"),1);S47J::Set(C,TEXT("f"),P.F);S47J::Set(C,TEXT("ftol"),P.FTol);S47J::Set(C,TEXT("g0"),P.Gain[0]);S47J::Set(C,TEXT("g1"),P.Gain[1]);S47J::Set(C,TEXT("bw0"),P.Bw[0]);S47J::Set(C,TEXT("bw1"),P.Bw[1]);S47J::Set(C,TEXT("az0"),P.Az[0]);S47J::Set(C,TEXT("az1"),P.Az[1]);S47J::Set(C,TEXT("el"),J.Card.El);S47J::Set(C,TEXT("seconds"),J.Card.Seconds);S47J::Set(C,TEXT("suggested"),J.Card.SuggestedClock);S47J::Set(C,TEXT("text"),P.Card);S47J::Set(A,TEXT("card"),C);
  }
  Jobs.Add(MakeShared<FJsonValueObject>(A));
 }
 O->SetArrayField(TEXT("jobs"),Jobs);
 auto F=S47J::New(),D=S47J::New();for(const auto& P:Findings){S47J::Set(F,*P.Key,P.Value);S47J::Set(D,*P.Key,FindingDays.FindRef(P.Key));}S47J::Set(O,TEXT("findings"),F);S47J::Set(O,TEXT("findingDays"),D);
 TArray<double> U,Q;for(bool V:Upgrades)U.Add(V?1:0);for(int32 V:MysterySteps)Q.Add(V);S47J::Set(O,TEXT("upgrades"),U);S47J::Set(O,TEXT("mysteries"),Q);
 TArray<double> Angles=SavedAngles;
 if(!bRestoreAngles&&M&&M->Web&&M->Web->IsLoaded())
 {
  Angles.Reset();
  for(int32 I=0;I<M->Web->DishCount();++I)
  {
   // The run up after KEPT OPEN (critic round 5): saved as where the dish was pointed, so that a Continue does not leave it at the zenith.
   double Az=M->Web->DishAz(I),El=M->Web->DishEl(I),ToAz=M->Web->DishTargetAz(I),ToEl=M->Web->DishTargetEl(I),WasAz=0,WasEl=0;
   if(Predecessor&&Predecessor->SaluteTarget(I,WasAz,WasEl)){Az=ToAz=WasAz;El=ToEl=WasEl;}
   Angles.Append({Az,El,ToAz,ToEl});
  }
 }
 S47J::Set(O,TEXT("angles"),Angles);
 S47J::Set(O,TEXT("cases"),Cases.Save());
 if(!CaseSheets.IsEmpty())
 {
  TArray<TSharedPtr<FJsonValue>> Waiting;
  for(const FResultSheet& Sh:CaseSheets){auto X=S47J::New();S47J::Set(X,TEXT("line"),Sh.Line);S47J::Set(X,TEXT("test"),Sh.Test);S47J::Set(X,TEXT("day"),Sh.Day);S47J::Set(X,TEXT("feed"),Sh.Feed);S47J::Set(X,TEXT("job"),Sh.Job);S47J::Set(X,TEXT("stamp"),Sh.Stamp);S47J::Set(X,TEXT("page"),Sh.Page);Waiting.Add(MakeShared<FJsonValueObject>(X));}
  O->SetArrayField(TEXT("caseSheets"),Waiting);
 }
 SaveTelex(O); // TELEX-20261109
 SaveMinutes(O); // LOOP-3-20261109
 if(!bOldCalendar)S47J::Set(O,TEXT("calendar"),2);
 if(Happenings){const auto Nights=Happenings->Save();if(Nights->Values.Num()>0)S47J::Set(O,TEXT("happenings"),Nights);}
 if(WeekCase){const S47J::FObj Week=WeekCase->Save();if(Week.IsValid())S47J::Set(O,TEXT("weekCase"),Week);}
 if(Scripts){const auto Played=Scripts->Save();if(Played->Values.Num()>0)S47J::Set(O,TEXT("scripts"),Played);}
 if(NightLog){const auto Sheets=NightLog->Save();if(Sheets->Values.Num()>0)S47J::Set(O,TEXT("nightLog"),Sheets);}
 if(Tape){const auto Cassettes=Tape->Save();if(Cassettes->Values.Num()>0)S47J::Set(O,TEXT("tape"),Cassettes);}
 if(Fatigue)S47J::Set(O,TEXT("fatigue"),S47J::FObj(Fatigue->Save()));
 if(!BrokenTubes.IsEmpty())S47J::Set(O,TEXT("brokenTubes"),TubesToSave(BrokenTubes)); // N1110: optional, absent = none
 if(Attention){const S47J::FObj Heed=Attention->Save();if(Heed->Values.Num()>0)S47J::Set(O,TEXT("attention"),Heed);} // N1110: optional, absent = nothing yet
 if(PinBoard){const auto Pinned=PinBoard->Save();if(Pinned->Values.Num()>0)S47J::Set(O,TEXT("pinBoard"),Pinned);}
 if(Darkroom){const auto Dark=Darkroom->Save();if(Dark->Values.Num()>0)S47J::Set(O,TEXT("darkroom"),Dark);} // D45-20261110: optional, absent = white
 if(WardReport){const auto Weekly=WardReport->Save();if(Weekly->Values.Num()>0)S47J::Set(O,TEXT("wardReport"),Weekly);}
 if(SkyAnswer){const auto Calls=SkyAnswer->Save();if(Calls->Values.Num()>0)S47J::Set(O,TEXT("answer"),Calls);}
 if(Predecessor)S47J::Set(O,TEXT("predecessor"),Predecessor->Save());
 if(Mail)S47J::Set(O,TEXT("mail"),Mail->Save());
 if(CaseField)S47J::Set(O,TEXT("fieldTests"),CaseField->Save());
 if(Field)S47J::Set(O,TEXT("field"),Field->Save());if(Decor)S47J::Set(O,TEXT("decor"),Decor->Save());if(FarmSite)S47J::Set(O,TEXT("farmSite"),FarmSite->Save());if(Deputy){const auto DeputyState=Deputy->Save();if(DeputyState->Values.Num()>0)S47J::Set(O,TEXT("deputy"),DeputyState);}if(Gear){const auto Lying=Gear->Save();if(Lying->Values.Num()>0)S47J::Set(O,TEXT("gear"),Lying);}if(!Kit.IsEmpty()){TArray<FString> KitList=Kit.Array();KitList.Sort();S47J::Set(O,TEXT("kit"),KitList);}if(Places){const auto Known=Places->Save();if(Known->Values.Num()>0)S47J::Set(O,TEXT("places"),Known);}if(Balloon){const auto Sonde=Balloon->Save();if(Sonde->Values.Num()>0)S47J::Set(O,TEXT("balloon"),Sonde);}if(EventPlaces){const auto Where=EventPlaces->Save();if(Where->Values.Num()>0)S47J::Set(O,TEXT("eventPlaces"),Where);}if(Tips){const auto Said=Tips->Save();if(Said->Values.Num()>0)S47J::Set(O,TEXT("tips"),Said);}if(YardAlarm){const auto Horn=YardAlarm->Save();if(Horn->Values.Num()>0)S47J::Set(O,TEXT("alarm"),Horn);}if(Care){const auto Truck=Care->Save();if(Truck->Values.Num()>0)S47J::Set(O,TEXT("care"),Truck);}if(TripCamera){const auto Trip=TripCamera->Save();if(Trip->Values.Num()>0)S47J::Set(O,TEXT("tripCamera"),Trip);}if(Outfit){const auto Rolled=Outfit->Save();if(Rolled->Values.Num()>0)S47J::Set(O,TEXT("outfit"),Rolled);}if(Round){const auto Sheet=Round->Save();if(Sheet->Values.Num()>0)S47J::Set(O,TEXT("round"),Sheet);}if(RoadCar){const auto Pickup=RoadCar->Save();if(Pickup->Values.Num()>0)S47J::Set(O,TEXT("roadCar"),Pickup);}if(HouseDoors)S47J::Set(O,TEXT("houseDoors"),HouseDoors->Save());if(IndoorDread){const auto Dread=IndoorDread->Save();if(Dread->Values.Num()>0)S47J::Set(O,TEXT("indoorDread"),Dread);}if(Aftermath){const auto After=Aftermath->Save();if(After->Values.Num()>0)S47J::Set(O,TEXT("aftermath"),After);}if(TownUse){const auto Town=TownUse->Save();if(Town->Values.Num()>0)S47J::Set(O,TEXT("townUse"),Town);}if(Townsfolk){const auto Folk=Townsfolk->Save();if(Folk->Values.Num()>0)S47J::Set(O,TEXT("townsfolk"),Folk);}if(Visitors){const auto Out=Visitors->Save();if(Out->Values.Num()>0)S47J::Set(O,TEXT("visitors"),Out);}if(MotelNight){const auto Motel=MotelNight->Save();if(Motel->Values.Num()>0)S47J::Set(O,TEXT("motelNight"),Motel);}if(Director)S47J::Set(O,TEXT("director"),Director->Save());return O;
}
TSet<int32> FS47FreeformSession::TubesFrom(const TArray<double>& Saved)
{
 TSet<int32> Out;
 for(const double D:Saved)if(FMath::IsFinite(D)&&D>=2&&D<=7)Out.Add(int32(D)); // N1110: nonsense in a save is dropped
 return Out;
}
TArray<double> FS47FreeformSession::TubesToSave(const TSet<int32>& Broken)
{
 TArray<int32> Sorted=Broken.Array();Sorted.Sort();
 TArray<double> Out;
 for(const int32 K:Sorted)Out.Add(K);
 return Out;
}
FString FS47FreeformSession::TraceBookLine(int32 Day,double Clock,const FString& Place,const TCHAR* Kind,const FString& Text)
{
 // N1110 CASEBOARD-20261108: "D2 22:10 / ARRAY SERVICE AREA / BEARING / NE, heard"; no time if Clock<0, no place if empty.
 FString Out=FString::Printf(TEXT("D%d"),FMath::Max(1,Day));
 if(Clock>=0)Out+=TEXT(" ")+FS47FreeformCases::Clock(Clock);
 if(!Place.IsEmpty())Out+=TEXT(" / ")+Place.ToUpper();
 return Out+FString::Printf(TEXT(" / %s / %s"),Kind,*Text.Left(150));
}
bool FS47FreeformSession::SelfTest(TArray<FString>& Lines)
{
 bool OK=S47CaptureSound::SelfTest(Lines);auto Check=[&](bool B,const TCHAR* Name){OK&=B;Lines.Add(FString(B?TEXT("PASS "):TEXT("FAIL "))+Name);};
 { // N1110 CASEBOARD-20261108: the trace book's line carries day, time, place and kind; no time or place when there is none
  const FString A=TraceBookLine(2,22*3600.+600,TEXT("array service area"),TEXT("BEARING"),TEXT("NE, heard"));
  const FString B2=TraceBookLine(0,-1,FString(),TEXT("RECORD"),TEXT("A light over the ridge."));
  Check(A==TEXT("D2 22:10 / ARRAY SERVICE AREA / BEARING / NE, heard"),TEXT("trace book line: day, time, place, kind"));
  Check(B2==TEXT("D1 / RECORD / A light over the ridge."),TEXT("trace book line: no time or place"));
 }
 { // FIX10-10: the drive check's result of the day and the mailbox start survive a Continue; a record from before has neither
  FS47FreeformSession Old(nullptr);Old.Begin(nullptr);auto OldState=Old.Save();Old.Stop();
  FS47FreeformSession Fresh(nullptr);Fresh.Begin(OldState);
  Check(!Fresh.bMailboxStart&&!Fresh.DriveCheckedToday()&&!OldState->HasField(TEXT("mailboxStart"))&&!OldState->HasField(TEXT("driveCheck")),TEXT("FIX10-10: a record without the drive check or the mailbox start reads as before, and writes none"));
  Fresh.Stop();
  S47J::Set(OldState,TEXT("mailboxStart"),true);auto Drive=S47J::New();S47J::Set(Drive,TEXT("day"),1);S47J::Set(Drive,TEXT("tone"),2);S47J::Set(Drive,TEXT("result"),TEXT("TEST RESULT"));S47J::Set(OldState,TEXT("driveCheck"),Drive);
  FS47FreeformSession Kept(nullptr);Kept.Begin(S47J::Read(S47J::Write(OldState)));auto KeptState=Kept.Save();
  Check(Kept.bMailboxStart&&Kept.DriveCheckedToday()&&Kept.DriveCheckResultText()==TEXT("TEST RESULT")&&S47J::Bool(KeptState,TEXT("mailboxStart"))&&S47J::Obj(KeptState,TEXT("driveCheck")).IsValid(),TEXT("FIX10-10: the mailbox start and today's drive check survive a Continue"));
  Kept.Stop();
 }
 OK&=SS47TapeScope::SelfTest(Lines); // D45-20261110: each take's own look on the reel deck's displays
 OK&=FS47Attention::SelfTest(Lines); // N1110: the attention curve, its thresholds, sabotage and the visit
 OK&=FS47DoorLoop::SelfTest(Lines); // N1110 (N1110-6): the door loop's steps, cap, reset and words
 OK&=S47GoalLine::SelfTest(Lines); // N1110 (N1110-9): the three goal lines on NOW
 OK&=FS47StoryDirector::SelfTest(Lines); // STORY-ON-FREEFORM-01 (step 3): the story director's acts, flags, save and own text check
 OK&=FS47RuleBreak::SelfTest(Lines); // N1110 (N1110-6): the rule taught, and broken once
 OK&=FS47IdleLure::SelfTest(Lines); // IDLE-LURE-20261112 (Tom): a visit drawn to the truck left idling: its touch, its strike, and what ends it
 { // N1110 (PLAY-31-34): broken ceiling tubes keep through a save; nonsense is dropped; 0 and 1 (the working light) never break
  FS47FreeformSession Tubes(nullptr);
  const bool bFirst=Tubes.BreakTube(3),bAgain=Tubes.BreakTube(3),bWork=Tubes.BreakTube(0)||Tubes.BreakTube(1)||Tubes.BreakTube(8);
  const TSet<int32> Back=TubesFrom(TubesToSave(Tubes.BrokenTubes));
  Check(bFirst&&!bAgain&&!bWork&&Tubes.TubeBroken(3)&&!Tubes.TubeBroken(2)&&!Tubes.TubeBroken(0)&&!Tubes.TubeBroken(1),TEXT("Tubes: a ceiling tube breaks once; the working light never does"));
  { // N1110: every sabotage takes the next two working tubes of 2 to 7, lowest first, until only the working light is left; a pair from the store puts back the last two
   FS47FreeformSession Dark(nullptr);bool bOrder=true;
   for(int32 Round=0;Round<3;++Round){const TArray<int32> Hit=Dark.NextTubes(2);bOrder&=Hit.Num()==2&&Hit[0]==2+Round*2&&Hit[1]==3+Round*2;for(const int32 K:Hit){Dark.BreakTube(K);}}
   const bool bNone=Dark.NextTubes(2).IsEmpty()&&!Dark.TubeBroken(0)&&!Dark.TubeBroken(1)&&Dark.BrokenTubes.Num()==6;
   const int32 Back2=Dark.MendTubes(2);
   Check(bOrder&&bNone&&Back2==2&&!Dark.TubeBroken(6)&&!Dark.TubeBroken(7)&&Dark.TubeBroken(5)&&Dark.NextTubes(2).Num()==2&&Dark.MendTubes(9)==4&&Dark.BrokenTubes.IsEmpty(),TEXT("Tubes: each sabotage puts out the next two working ones until the room is dark but for the working light; a pair from the store puts the last two back"));
  }
  Check(Back.Num()==1&&Back.Contains(3)&&TubesFrom({0.,1.,2.,9.,-1.,NAN}).Num()==1&&TubesFrom({}).IsEmpty(),TEXT("Tubes: the break is saved and read back; an older save has none; nonsense is dropped"));
 }
 S47CaptureSound::FTrack SoundPolicy;SoundPolicy.Begin(TEXT("sound-policy-original"),false,{});
 for(int I=0;I<360;++I)SoundPolicy.Append(I*.05,(I+1)*.05,.05,1,1420.11,50,50,1);SoundPolicy.Complete=true;
 const FString RetainedSound=S47J::Write(SoundPolicy.Save());
 Check(UseCapturedSound(&SoundPolicy,false)&&!UseCapturedSound(&SoundPolicy,true)&&!UseCapturedSound(nullptr,false),TEXT("Captured transport requires retained playable sound and is disabled by explicit NoSound"));
 Check(S47J::Write(SoundPolicy.Save())==RetainedSound,TEXT("NoSound playback policy does not mutate retained capture recipe"));
 SoundPolicy.Complete=false;SoundPolicy.Interrupt(TEXT("test interrupted capture"));
 Check(!UseCapturedSound(&SoundPolicy,false),TEXT("Interrupted capture uses compatible transport without captured drift guard"));
 Check(!EncoderChanged({10,20},{10,20})&&EncoderChanged({10,20},{10.01,20})&&EncoderChanged({10,20},{10,20.01}),TEXT("Equipment sound follows actual azimuth or elevation delta, not a pending command"));
 Check(!EncoderChanged({0,20},{360,20}),TEXT("Equivalent wrapped encoder bearings do not invent motor movement"));
 auto Close=[](double A,double B){return FMath::Abs(A-B)<.00001;};
 Check(Close(AdvanceClock(6*3600.,1800),19*3600.),TEXT("Freeform daylight lasts thirty active minutes"));
 Check(Close(AdvanceClock(19*3600.,2700),30*3600.),TEXT("Freeform night lasts forty-five active minutes across midnight"));
 Check(Close(AdvanceClock(8*3600.,4500),32*3600.)&&Close(AdvanceClock(23*3600.,9000),71*3600.),TEXT("Freeform cycles retain seventy-five minutes from any starting phase"));
 Check(Close(AdvanceClock(19*3600.-13,1),19*3600.+11*3600./2700./2),TEXT("Freeform clock frame splits precisely at dusk"));
 Check(Close(AdvanceClock(8*3600.,0),8*3600.),TEXT("Freeform no active time leaves the clock unchanged"));
 FS47FreeformSession S(nullptr);S.Begin(nullptr,false);
 const FString SavedCard=S.ConsoleProfile()->Card;const auto Generated=S.Save();FS47FreeformSession Replay(nullptr);Replay.Begin(S47J::Read(S47J::Write(Generated)),false);
 Check(Replay.JobSeed==S.JobSeed&&Replay.Jobs().Num()==3&&Replay.Jobs()[0].bGenerated&&Replay.ConsoleProfile()->Card==SavedCard,TEXT("Freeform retains the actual generated observing card and seed through JSON save/Continue"));
 auto Legacy=S47J::Clone(Generated);Legacy->RemoveField(TEXT("jobSeed"));for(const auto& V:S47J::Arr(Legacy,TEXT("jobs")))if(V&&V->Type==EJson::Object)V->AsObject()->RemoveField(TEXT("card"));
 FS47FreeformSession Old(nullptr);Old.Begin(Legacy,false);Check(!Old.Jobs()[0].bGenerated&&Old.ConsoleProfile()->F==Profile(0).F,TEXT("Older Freeform cards retain their original fixed receiver profile rather than rerolling"));
 Check(S.Active()&&S.Day()==1&&S.Clock()==8*3600&&S.Budget()==120&&S.Jobs().Num()==3,TEXT("Freeform starts a separate day with three jobs and operating budget"));
 S.Spend(120);Check(!S.Spend(5)&&S.Spend(5,true)&&S.Debt()==5&&!S.Spend(5,true),TEXT("Freeform emergency requisition is bounded and cannot repeatedly mint stock"));
 S.Earn(20);Check(S.Budget()==15&&S.Debt()==0,TEXT("Freeform job income repays emergency credit before cash"));
 {FS47FreeformSession P(nullptr);P.Begin(nullptr,false);P.Carried=30;P.Earn(20);Check(P.Arrears()==10&&P.Budget()==120,TEXT("M7: pay clears what is carried on the weekly sheet before the budget"));P.Earn(20);Check(P.Arrears()==0&&P.Budget()==130,TEXT("M7: and the rest goes into the budget"));P.Stop();}
 S.RecordEvidence(TEXT("extra_pulse"),TEXT("retained"));S.RecordEvidence(TEXT("extra_pulse"),TEXT("changed"));Check(S.Findings.FindRef(TEXT("extra_pulse"))==TEXT("retained"),TEXT("Freeform retained original facts are not overwritten by repeats"));
 S.TestAdvance(86400);Check(S.Day()==2&&S.Jobs().Num()==6,TEXT("Freeform new days retain unfinished work and create distinct daily job identities"));
 {
  // CHAIN-5-20261109: at the turn of the day cards nobody began fall away; work begun stays, and so does a planned case test.
  FS47FreeformSession Lp(nullptr);Lp.Begin(nullptr);Lp.Tutorial=0;Lp.TestLiveRules();Lp.Work[0].Stage=EJobStage::Recorded;
  FJob CaseJob;CaseJob.Id=TEXT("FF-D0001-CASE1-T1-9");CaseJob.Day=1;CaseJob.bGenerated=true;CaseJob.CaseLine=0;CaseJob.CaseTest=0;Lp.Work.Add(CaseJob);
  Lp.TestAdvance(86400);Check(Lp.Jobs().Num()==7,TEXT("Lapse: TestAdvance alone drops nothing"));
  Lp.LapseUntakenCards();bool bBegun=true;for(const FJob& J:Lp.Work)if(J.Day==1&&J.CaseTest<0)bBegun&=J.Stage!=EJobStage::New;
  Check(Lp.Jobs().Num()==5&&bBegun&&Lp.Work.ContainsByPredicate([](const FJob& J){return J.CaseTest==0;}),TEXT("Lapse: cards nobody began fall away at the new day; the begun card and the planned case test stay"));
  Lp.Stop();
 }
 auto Saved=S.Save();FS47FreeformSession R(nullptr);R.Begin(S47J::Read(S47J::Write(Saved)));Check(R.Day()==2&&R.Budget()==15&&R.HasFinding(TEXT("extra_pulse"))&&R.Jobs().Num()==6&&!R.AudioPlaying(),TEXT("Freeform disk JSON restore retains economy, days, evidence and paused audio"));
 R.RecordEvidence(TEXT("feeder_fault"),TEXT("control"));R.RecordEvidence(TEXT("wrong_source"),TEXT("adjacent beam"));Check(R.MysterySteps[0]==3,TEXT("Freeform mystery comparison requires independent records across days"));
 FS47FreeformSession Followup(nullptr);Followup.Begin(nullptr);
 Followup.RecordEvidence(TEXT("extra_pulse"),TEXT("first-day original pulse"));Followup.RecordEvidence(TEXT("feeder_fault"),TEXT("first-day feeder control"));Followup.RecordEvidence(TEXT("wrong_source"),TEXT("first-day adjacent sample"));
 Check(Followup.MysterySteps[0]==1,TEXT("all three mystery sources on one day remain an initial finding, not cross-day proof"));
 Followup.TestAdvance(86400);Followup.RecordEvidence(TEXT("extra_pulse.daylight"),TEXT("unrelated label"));
 Check(Followup.MysterySteps[0]==1,TEXT("a similarly prefixed label is not mistaken for a dated mystery follow-up"));
 Followup.RecordEvidence(TEXT("extra_pulse.day2"),TEXT("independent second-day repeat"));
 Check(Followup.MysterySteps[0]==3&&Followup.Findings.FindRef(TEXT("extra_pulse"))==TEXT("first-day original pulse")&&Followup.FindingDays.FindRef(TEXT("extra_pulse"))==1&&Followup.FindingDays.FindRef(TEXT("extra_pulse.day2"))==2,TEXT("retained later-day follow-up completes a same-day mystery set without rewriting its first source"));
 auto FollowupSave=S47J::Read(S47J::Write(Followup.Save()));FS47FreeformSession FollowupRead(nullptr);FollowupRead.Begin(FollowupSave);
 Check(FollowupRead.MysterySteps[0]==3&&FollowupRead.Findings.FindRef(TEXT("extra_pulse.day2"))==TEXT("independent second-day repeat"),TEXT("cross-day follow-up provenance and completed comparison survive JSON restore"));
 Followup.Stop();FollowupRead.Stop();
 FS47FreeformSession Instrument(nullptr);Instrument.Begin(nullptr);Instrument.Selected=1;
 auto* I=Instrument.Job();I->Stage=EJobStage::Recorded;I->bAnomaly=I->bControl=true;I->ControlAt=8;I->Phenomenon=TEXT("wrong_source");
 Check(!Instrument.CompareInstrument(),TEXT("off-source comparison cannot be granted by elapsed time without three settled beam samples"));
 I->BeamAz={99.5,100.5,100};I->BeamEl={30,30,30};I->BeamAmplitude={.08,1,.2};I->bBeamVerified=true;I->OriginalPage=TEXT("retained original bytes");
 Check(Instrument.CompareInstrument()&&Instrument.HasFinding(TEXT("wrong_source")),TEXT("three verified beam samples retain an off-centre comparison, not an alien-origin claim"));
 auto InstrumentSaved=S47J::Read(S47J::Write(Instrument.Save()));FS47FreeformSession Readback(nullptr);Readback.Begin(InstrumentSaved);
 Check(Readback.CurrentJob()&&Readback.CurrentJob()->BeamAz.Num()==3&&Readback.CurrentJob()->BeamAz[1]==100.5&&Readback.CurrentJob()->OriginalPage==TEXT("retained original bytes"),TEXT("measured encoders and immutable original snapshot survive JSON restore"));
 I=Instrument.Job();I->bAudioReviewed=false;I->Phenomenon=TEXT("time_disagreement");I->ReceiverClock=103.2;I->ReferenceClock=100;I->ControlReceiverClock=203.2;I->ControlReferenceClock=200;
 Check(!Instrument.CompareInstrument(),TEXT("clock numbers alone do not substitute for the timed independent-control readback"));
 I->bClockMeasured=true;Check(Instrument.CompareInstrument(),TEXT("simultaneous original and control device readings establish a repeated timing-path discrepancy"));
 I->bAudioReviewed=false;I->Phenomenon=TEXT("duplicate_print");I->bDuplicateCollected=true;I->DuplicatePage=TEXT("different copy");
 Check(!Instrument.CompareInstrument(),TEXT("duplicate metadata cannot grant comparison without two actual retained document originals"));
 Instrument.Stop();Readback.Stop();
 Check(MeasurementRateForCount(27)==1.&&FMath::IsNearlyEqual(MeasurementRateForCount(26),650./702.,1e-12)&&FMath::IsNearlyEqual(MeasurementRateForCount(24),552./702.,1e-12),TEXT("Array measurement rate follows available-pair gameplay balance at 27, 26 and 24 antennas"));
 Check(MeasurementRateForCount(23)==0.&&MeasurementRateForCount(0)==0.&&MeasurementRateForCount(-1)==0.&&MeasurementRateForCount(99)==1.,TEXT("Array measurement rate blocks fewer than 24 and cannot exceed full capacity"));
 // Printer state is separate from receiver selection and from already retained documents.
 FS47FreeformSession Printer(nullptr);Printer.Begin(nullptr);
 Check(Printer.PrinterProgress()<0&&!Printer.PrinterFeedbackActive(),TEXT("Idle Freeform has no printer paper or loop"));
 Printer.Work[0].Stage=EJobStage::Chart;Printer.Work[0].ChartFeed=2.1;Printer.Selected=1;
 const FString PrintId=Printer.Work[0].Id;const int32 PrintCash=Printer.Budget();
 Check(Printer.PrinterJobId()==PrintId&&!Printer.CollectChart()&&Printer.FeedbackText().Contains(TEXT("Wait for the printer"))&&!Printer.Work[0].bPrinted&&Printer.Selected==1&&Printer.Budget()==PrintCash,TEXT("Partial printer feed rejects collection independently of selected work without granting a document or payment"));
 FS47FreeformSession PrinterRead(nullptr);PrinterRead.Begin(S47J::Read(S47J::Write(Printer.Save())));
 Check(PrinterRead.PrinterJobId()==PrintId&&FMath::Abs(PrinterRead.Work[0].ChartFeed-2.1)<1e-9&&!PrinterRead.PrinterFeedbackActive(),TEXT("Continue preserves printer job identity and partial seconds without restoring a stale audio handle"));
 PrinterRead.SetPowerFault(true);PrinterRead.StepPrinter(3);
 Check(PrinterRead.Work[0].ChartFeed==2.1&&!PrinterRead.CollectChart()&&PrinterRead.FeedbackText().Contains(TEXT("Restore power")),TEXT("Power loss preserves feed progress and explains why the incomplete sheet cannot be collected"));
 PrinterRead.Work[0].ChartFeed=PrinterSeconds;
 Check(PrinterRead.CollectChart()&&PrinterRead.Work[0].Stage==EJobStage::Recorded&&PrinterRead.Work[0].bPrinted&&PrinterRead.Budget()==PrintCash,TEXT("Completed original remains collectible without power and cannot pay twice"));
 // D48-20261110: the job that last held the array is kept across a save; an older save has none and starts with a free array
 {
  FS47FreeformSession Held(nullptr);Held.Begin(nullptr);Held.ArrayJob=TEXT("FF-D0001-HELD");
  FS47FreeformSession HeldRead(nullptr);HeldRead.Begin(S47J::Read(S47J::Write(Held.Save())));
  auto HeldOld=Held.Save();HeldOld->RemoveField(TEXT("arrayJob"));FS47FreeformSession HeldOldRead(nullptr);HeldOldRead.Begin(S47J::Read(S47J::Write(HeldOld)));
  Check(HeldRead.ArrayJob==TEXT("FF-D0001-HELD")&&HeldOldRead.ArrayJob.IsEmpty(),TEXT("The job that held the array survives a save; an older save has none"));
  // without a station there is nothing to release: selecting a second job leaves the session as it was and never throws
  Held.Work.Reset();FJob HeldA,HeldB;HeldA.Id=TEXT("FF-D0001-HELD");HeldB.Id=TEXT("FF-D0001-NEXT");Held.Work.Add(HeldA);Held.Work.Add(HeldB);Held.Selected=0;
  Check(Held.SelectJob(1)&&Held.ArrayJob==TEXT("FF-D0001-HELD")&&Held.CurrentJob()&&Held.CurrentJob()->Id==TEXT("FF-D0001-NEXT"),TEXT("Without a station a new job selection keeps the record of the last holder and selects normally"));
 }
 auto OldPrinter=Printer.Save();for(const auto& V:S47J::Arr(OldPrinter,TEXT("jobs"))){const auto O=V->AsObject();O->RemoveField(TEXT("chartFeed"));O->RemoveField(TEXT("packetFeed"));O->RemoveField(TEXT("duplicateFeed"));}
 FS47FreeformSession LegacyPrinter(nullptr);LegacyPrinter.Begin(OldPrinter);
 Check(LegacyPrinter.PrinterProgress()==1&&LegacyPrinter.Work[0].PacketFeed==0&&LegacyPrinter.Work[0].DuplicateFeed==0,TEXT("Legacy pending original stays ready while future packet and duplicate still require feed"));
 Printer.Stop();PrinterRead.Stop();LegacyPrinter.Stop();
 FS47FreeformSession Unpowered(nullptr);Unpowered.Begin(nullptr);Unpowered.Selected=1;auto* UJ=Unpowered.Job();UJ->Stage=EJobStage::Recorded;UJ->bAnomaly=UJ->bControl=true;UJ->ControlAt=8;UJ->OriginalCovered=18;UJ->ControlCovered=8;UJ->MarkStart=9.1;UJ->MarkEnd=13;
 Unpowered.SetPowerFault(true);Check(!Unpowered.AudioAction(TEXT("confirm"))&&!UJ->bAudioReviewed&&!Unpowered.RegisterComparison(TEXT("unresolved")),TEXT("unpowered instruments cannot grant audio review or computer registration"));
 UJ->Stage=EJobStage::Registered;Check(!Unpowered.CollectChart()&&UJ->Stage==EJobStage::Registered,TEXT("a dead printer cannot create a new registered packet"));
 UJ->Stage=EJobStage::Packet;const int32 BeforeRemoteFiling=Unpowered.Budget();
 Check(!Unpowered.FilePacket()&&UJ->Stage==EJobStage::Packet&&Unpowered.Budget()==BeforeRemoteFiling,TEXT("remote archive access cannot file a packet or grant its payment without the actual binder and nearby player"));
 auto Corrupt=S.Save();TArray<double> InvalidAngles;InvalidAngles.Init(20,108);for(int32 A=1;A<108;A+=2)InvalidAngles[A]=30;InvalidAngles[5]=500;S47J::Set(Corrupt,TEXT("angles"),InvalidAngles);
 auto BadDays=S47J::Obj(Corrupt,TEXT("findingDays"));S47J::Set(BadDays,TEXT("extra_pulse"),1e100);FS47FreeformSession Guarded(nullptr);Guarded.Begin(Corrupt);
 Check(!Guarded.bRestoreAngles&&Guarded.SavedAngles.IsEmpty()&&!Guarded.FindingDays.Contains(TEXT("extra_pulse")),TEXT("out-of-range saved encoders and overflowing finding days are discarded before native transforms or integer casts"));
 Unpowered.Stop();Guarded.Stop();
 // Guidance follows the retained workflow state, including a resumed automatic control.
 FS47FreeformSession Guidance(nullptr);Guidance.Begin(nullptr);auto* GJ=Guidance.Job();
 Check(Guidance.NextAction().StartsWith(TEXT("TRACKING (right-hand PC):")),TEXT("New observing job names the actual tracking workstation"));
 GJ->Stage=EJobStage::Recorded;GJ->bAnomaly=true;GJ->Phenomenon=TEXT("wrong_source");
 Check(Guidance.NextAction().StartsWith(TEXT("TRACKING (right-hand PC):"))&&Guidance.NextAction().Contains(TEXT("WEST, EAST AND CENTRE")),TEXT("Off-source discrepancy directs the player to physical tracking controls"));
 GJ->bControl=true;GJ->ControlAt=3;
 FS47FreeformSession GuidanceRestored(nullptr);GuidanceRestored.Begin(Guidance.Save());
 Check(GuidanceRestored.NextAction().Contains(TEXT("WAIT FOR AUTOMATIC"))&&GuidanceRestored.NextAction().Contains(TEXT("THEN COMPARE AT OPS")),TEXT("Continue during beam control preserves the automatic-wait and next-workstation instruction"));
 GJ=GuidanceRestored.Job();GJ->ControlAt=8;
 Check(GuidanceRestored.NextAction().StartsWith(TEXT("OPS (corner PC):"))&&GuidanceRestored.NextAction().Contains(TEXT("COMPARE")),TEXT("Completed beam control directs comparison to OPS without repeating the measurement"));
 GJ->bAudioReviewed=true;
 Check(GuidanceRestored.NextAction().StartsWith(TEXT("OPS (corner PC):"))&&GuidanceRestored.NextAction().Contains(TEXT("REGISTER")),TEXT("Reviewed beam measurements direct registration to the same OPS workstation"));
 Guidance.Stop();GuidanceRestored.Stop();
 FS47FreeformSession Workflow(nullptr);Workflow.Begin(nullptr);auto* WJ=Workflow.Job();
 WJ->Stage=EJobStage::Recorded;WJ->bPrinted=true;
 Check(Workflow.NextAction().Contains(TEXT("REGISTER THE ROUTINE"))&&!Workflow.NextAction().Contains(TEXT("PRINTER")),TEXT("Collected routine original directs to OPS, never back to printer"));
 Check(TerminalNextLine(Workflow.NextAction(),3)==TEXT("NEXT / USE THIS PC")&&TerminalNextLine(Workflow.NextAction(),2)==TEXT("NEXT / OPS (CORNER PC)"),TEXT("A workstation's NEXT line is the HUD's own step: this PC on the one that owns it, the place on the others"));
 {
  bool NextFits=true;const EJobStage NextWas=WJ->Stage;
  for(const EJobStage NextStage:{EJobStage::New,EJobStage::Pointed,EJobStage::Integrating,EJobStage::Chart,EJobStage::Recorded,EJobStage::Registered,EJobStage::Packet})
  {
   WJ->Stage=NextStage;
   for(int32 NextScreen=0;NextScreen<4;++NextScreen){const FString NextShown=TerminalNextLine(Workflow.JobNextAction(*WJ),NextScreen);NextFits&=NextShown.StartsWith(TEXT("NEXT / "))&&NextShown.Len()<=38;}
  }
  WJ->Stage=NextWas;
  Check(NextFits,TEXT("Every stage of a job gives every workstation a NEXT line that fits the glass"));
 }
 {
  // GAMEPLAY-LOOP-01 (Tom, 12 November): an unusual take goes ANOMALY DETECTED, record, reel deck, OPS, printer, binder. After the take
  // the reel deck is next (no control, no chart to fetch), one listen confirms it, OPS's one button registers it and the packet prints;
  // a save from the old chart step lands on the reel deck, and the instrument discrepancies keep their printer-first order.
  // (the old chart step is a take whose original is not collected yet: the routine check above left this card collected, bPrinted)
  const bool D45Anomaly=WJ->bAnomaly,D45Printed=WJ->bPrinted;const EJobStage D45Stage=WJ->Stage;bool D45Order=true;
  WJ->bAnomaly=true;WJ->Stage=EJobStage::Chart;WJ->ChartFeed=PrinterSeconds;WJ->bPrinted=false;WJ->bControl=WJ->bAudioReviewed=false;WJ->ControlAt=0;
  {
   // the old chart step, saved: on Continue it waits on the receiver's RECORD tab to be retained (RX-ANOMALY-FLOW-20261113), then the deck
   FS47FreeformSession D45Again(nullptr);D45Again.Begin(Workflow.Save());const FJob* D45J=D45Again.CurrentJob();
   D45Order&=D45J&&D45J->Stage==EJobStage::Recorded&&D45J->bAnomaly&&!D45J->bControl&&RetainWaits(*D45J)&&TerminalNextLine(D45Again.JobNextAction(*D45J),3)==TEXT("NEXT / RECEIVER (CENTRE PC)")
    &&D45Again.JobNextAction(*D45J).Contains(TEXT("RECORD TAB"))&&D45Again.JobLocation(*D45J).Contains(TEXT("RECORD tab"))&&!D45Again.CanRecordControl()&&D45Again.PrinterProgress()<0;D45Again.Stop();
  }
  WJ->Phenomenon=TEXT("time_disagreement");
  D45Order&=!Workflow.CanRecordControl()&&!Workflow.StartControl()&&Workflow.JobNextAction(*WJ).StartsWith(TEXT("PRINTER"))&&StatusWord(*WJ,true)==TEXT("AT PRINTER");
  WJ->Phenomenon.Empty();WJ->bAnomaly=D45Anomaly;WJ->bPrinted=D45Printed;WJ->Stage=D45Stage;WJ->ChartFeed=0;Workflow.Feedback.Empty();
  Check(D45Order,TEXT("GAMEPLAY-LOOP-01: an old save's extra-tick take at the printer waits on the receiver's RECORD tab on Continue; the instrument discrepancies keep their chart"));
  FS47FreeformSession Loop(nullptr);Loop.Begin(nullptr);Loop.Tutorial=0;
  const int32 LoopI=Loop.Work.IndexOfByPredicate([](const FJob& X){return X.Source==1&&X.CaseTest<0;});
  bool LoopOrder=LoopI!=INDEX_NONE;
  if(LoopOrder)
  {
   Loop.Selected=LoopI;FJob& LJ=Loop.Work[LoopI];
   LJ.Stage=EJobStage::Integrating;LJ.bAnomaly=true;LJ.Phenomenon=TEXT("extra_pulse");for(double At:Ticks)LJ.TraceTicks.Add(At);
   LoopOrder&=Loop.JobNextAction(LJ).Contains(TEXT("ANOMALY DETECTED"))&&TerminalNextLine(Loop.JobNextAction(LJ),1)==TEXT("NEXT / USE THIS PC");
   // RX-ANOMALY-FLOW-20261113: the take ended waits on the receiver's RECORD tab; the deck and OPS refuse it, each saying where
   LJ.Stage=EJobStage::Recorded;LJ.Elapsed=18;LJ.ChartFeed=PrinterSeconds;
   LoopOrder&=RetainWaits(LJ)&&TerminalNextLine(Loop.JobNextAction(LJ),3)==TEXT("NEXT / RECEIVER (CENTRE PC)")&&Loop.JobNextAction(LJ).Contains(TEXT("RECORD TAB, RETAIN THE TAKE"))
    &&TerminalNextLine(Loop.JobNextAction(LJ),1)==TEXT("NEXT / USE THIS PC")&&Loop.JobLocation(LJ).Contains(TEXT("RECORD tab"))&&!Loop.TapeWaits(LJ);
   LJ.OriginalCovered=18;LoopOrder&=!Loop.AudioAction(TEXT("play"))&&Loop.FeedbackText().Contains(TEXT("RETAIN TAKE"))&&!Loop.AudioAction(TEXT("confirm"))&&!LJ.bAudioReviewed
    &&!Loop.MarkAudioAt(10.)&&LJ.MarkStart<0&&!Loop.RegisterComparison(TEXT("unresolved"))&&Loop.FeedbackText().Contains(TEXT("RECORD tab"));
   LJ.OriginalCovered=0;LJ.bPrinted=true; // retained (RetainTake itself, with the reference, is walked in the native test S47InternalControlTest)
   LoopOrder&=!RetainWaits(LJ)&&Loop.TapeWaits(LJ)&&TerminalNextLine(Loop.JobNextAction(LJ),3)==TEXT("NEXT / REEL DECK (WALL TABLE)")&&Loop.JobNextAction(LJ).Contains(TEXT("PRESS RETAIN"))&&Loop.JobLocation(LJ).StartsWith(TEXT("Reel deck"))&&!Loop.CanRecordControl()
    &&StatusWord(LJ,true)==TEXT("ANOMALY")&&Loop.PrinterProgress()<0;
   LoopOrder&=!Loop.RegisterComparison(TEXT("unresolved"))&&Loop.FeedbackText().StartsWith(TEXT("REEL DECK"))&&LJ.Stage==EJobStage::Recorded;
   LJ.OriginalCovered=8;LoopOrder&=!Loop.AudioAction(TEXT("confirm"))&&!LJ.bAudioReviewed; // the ticks are not heard yet
   // RX-ANOMALY-FLOW-20261113: marks set by hand are the operator's: missing the ticks, RETAIN refuses and says where they are
   LJ.OriginalCovered=13;LJ.MarkStart=10.;LJ.MarkEnd=11.;LoopOrder&=!Loop.AudioAction(TEXT("confirm"))&&!LJ.bAudioReviewed&&Loop.FeedbackText().Contains(TEXT("miss the extra ticks"))&&LJ.MarkStart==10.;
   LJ.MarkStart=9.1;LJ.MarkEnd=-1;LoopOrder&=Loop.AudioAction(TEXT("confirm"))&&LJ.bAudioReviewed&&FMath::IsNearlyEqual(LJ.MarkStart,9.1)&&LJ.MarkEnd>=12.6&&LJ.ControlCovered==0&&MarkCoversTicks(LJ.MarkStart,LJ.MarkEnd)&&Loop.HasFinding(TEXT("pulse-review"))
    &&Loop.FeedbackText().Contains(TEXT("Confirm anomaly: source unresolved"));
   LoopOrder&=TerminalNextLine(Loop.JobNextAction(LJ),1)==TEXT("NEXT / OPS (CORNER PC)")&&TerminalNextLine(Loop.JobNextAction(LJ),3)==TEXT("NEXT / USE THIS PC")&&StatusWord(LJ,true)==TEXT("CONFIRM");
   LoopOrder&=Loop.RegisterComparison(TEXT("unresolved"))&&LJ.Stage==EJobStage::Registered&&TerminalNextLine(Loop.JobNextAction(LJ),3)==TEXT("NEXT / PRINTER");
   LJ.PacketFeed=PrinterSeconds;LoopOrder&=Loop.JobNextAction(LJ).Contains(TEXT("COLLECT THE ANOMALY PACKET"));
   LJ.Stage=EJobStage::Packet;LoopOrder&=Loop.JobNextAction(LJ).Contains(TEXT("BINDER"));
  }
  Check(LoopOrder,TEXT("GAMEPLAY-LOOP-01, RX-ANOMALY-FLOW-20261113: ANOMALY DETECTED, the take retained on the receiver's RECORD tab, the reel deck (heard, marked, RETAIN), OPS's one button, the packet, the binder"));
  Loop.Stop();
  {
   // RX-ANOMALY-FLOW-20261113 (Tom 13 November): the reel deck's marks by its keys and by a click on the trace, every refusal with its
   // reason; the take's 8-second reference on its own key; RETAIN only by the operator's press.
   FS47FreeformSession Mk(nullptr);Mk.Begin(nullptr);Mk.Tutorial=0;
   const int32 MkI=Mk.Work.IndexOfByPredicate([](const FJob& X){return X.Source==1&&X.CaseTest<0;});
   bool Keys=MkI!=INDEX_NONE,Clicks=Keys,Ref=Keys,Retain=Keys;
   if(Keys)
   {
    Mk.Selected=MkI;FJob& MJ=Mk.Work[MkI];MJ.bAnomaly=true;MJ.Phenomenon=TEXT("extra_pulse");for(double At:Ticks)MJ.TraceTicks.Add(At);
    MJ.Stage=EJobStage::Recorded;MJ.Elapsed=18;MJ.ChartFeed=PrinterSeconds;
    Retain&=RetainWaits(MJ)&&!Mk.RetainTake()&&Mk.FeedbackText().Contains(TEXT("could not be written")); // no notebook here: refused, with why
    MJ.bPrinted=true;MJ.bControl=true;MJ.ControlAt=8;Mk.bAudioOpen=true;
    Retain&=!RetainWaits(MJ)&&!Mk.RetainTake()&&Mk.FeedbackText().Contains(TEXT("already retained"));
    Mk.AudioCursor=9.1;Keys&=Mk.AudioAction(TEXT("mark_start"))&&FMath::IsNearlyEqual(MJ.MarkStart,9.1)&&MJ.MarkEnd<0&&Mk.FeedbackText().StartsWith(TEXT("MARK IN 9.1"));
    Mk.AudioCursor=8.;Keys&=!Mk.AudioAction(TEXT("mark_end"))&&MJ.MarkEnd<0&&Mk.FeedbackText().Contains(TEXT("must come after MARK IN"));
    Mk.AudioCursor=13.;Keys&=Mk.AudioAction(TEXT("mark_end"))&&FMath::IsNearlyEqual(MJ.MarkEnd,13.)&&Mk.FeedbackText().Contains(TEXT("covers the extra ticks"));
    Mk.AudioCursor=14.;Keys&=Mk.AudioAction(TEXT("mark_start"))&&MJ.MarkEnd<0&&FMath::IsNearlyEqual(MJ.MarkStart,14.); // MARK IN after MARK OUT starts the mark again
    MJ.MarkStart=MJ.MarkEnd=-1.;
    Clicks&=Mk.MarkAudioAt(13.)&&MJ.MarkEnd<0&&Mk.MarkAudioAt(9.)&&FMath::IsNearlyEqual(MJ.MarkStart,9.)&&FMath::IsNearlyEqual(MJ.MarkEnd,13.)&&MarkCoversTicks(MJ.MarkStart,MJ.MarkEnd)
     &&Mk.FeedbackText().Contains(TEXT("covers the extra ticks"))&&FMath::IsNearlyEqual(Mk.AudioCursor,14.); // two clicks, right to left; the tape stays put
    Ref&=Mk.AudioAction(TEXT("control"))&&Mk.AudioSourceIndex==1&&Mk.AudioAction(TEXT("play"))&&!Mk.MarkAudioAt(10.)&&Mk.FeedbackText().Contains(TEXT("reference is loaded"))&&FMath::IsNearlyEqual(MJ.MarkStart,9.);
    Mk.bAudioPlaying=false;MJ.ControlAt=4;Ref&=!Mk.AudioAction(TEXT("control"))&&Mk.FeedbackText().Contains(TEXT("still recording"));MJ.ControlAt=8;
    Ref&=Mk.AudioAction(TEXT("original"))&&Mk.AudioSourceIndex==0;
    MJ.OriginalCovered=18.;Retain&=!MJ.bAudioReviewed; // heard: nothing confirms itself
    MJ.MarkStart=10.;MJ.MarkEnd=11.;Retain&=!Mk.AudioAction(TEXT("confirm"))&&!MJ.bAudioReviewed&&Mk.FeedbackText().Contains(TEXT("miss the extra ticks"));
    MJ.MarkStart=9.;MJ.MarkEnd=13.;Retain&=Mk.AudioAction(TEXT("confirm"))&&MJ.bAudioReviewed&&FMath::IsNearlyEqual(MJ.MarkStart,9.)&&FMath::IsNearlyEqual(MJ.MarkEnd,13.)
     &&!Mk.MarkAudioAt(5.)&&Mk.FeedbackText().Contains(TEXT("retained with its marks"))&&Mk.JobNextAction(MJ).StartsWith(TEXT("OPS"));
   }
   Check(Keys,TEXT("RX-ANOMALY-FLOW-20261113: reel deck marks by MARK IN / MARK OUT at the tape head, in order, each saying what it set or why not"));
   Check(Clicks,TEXT("RX-ANOMALY-FLOW-20261113: reel deck marks by two clicks on the trace, in either order, without moving the tape"));
   Check(Ref,TEXT("RX-ANOMALY-FLOW-20261113: the take's 8-second reference plays on its own key; it cannot be marked, and a reference still recording says so"));
   Check(Retain,TEXT("RX-ANOMALY-FLOW-20261113: RETAIN TAKE refuses with its reason; hearing confirms nothing; RETAIN refuses marks that miss the ticks and keeps marks that cover them"));
   Mk.Stop();
  }
  { // RX-TABS-DUE-20261113: the receiver tab that holds the next step (state to due tab), and the OPS line after the take
   using E=EJobStage;
   auto Due=[](E Stage,int32 Open,TFunction<void(FRxDueIn&)> Set){FRxDueIn In;In.bJob=true;In.Stage=Stage;In.Open=Open;if(Set)Set(In);return RxDueTab(In);};
   bool Rule=RxDueTab(FRxDueIn())==-1; // no job: nothing
   Rule&=Due(E::New,RxAcquire,nullptr)==-1&&Due(E::New,RxTune,nullptr)==-1&&Due(E::New,RxArray,nullptr)==-1; // TRACKING next: no receiver tab pulses before the pointing
   Rule&=Due(E::New,RxTune,[](FRxDueIn& X){X.bArrayBlocked=true;})==-1; // nor ARRAY for isolated antennas before it
   Rule&=Due(E::Pointed,RxTune,[](FRxDueIn& X){X.bArrayBlocked=X.bPointReady=X.bSettled=true;})==RxArray; // pointed: too few antennas is ARRAY again
   Rule&=Due(E::Pointed,RxAcquire,nullptr)==RxArray; // slewing
   Rule&=Due(E::Pointed,RxTune,[](FRxDueIn& X){X.bPointReady=true;})==RxAcquire; // settling, then the sky chart
   Rule&=Due(E::Pointed,RxTune,[](FRxDueIn& X){X.bPointReady=X.bSettled=true;})==RxAcquire; // on the card: find it on the chart
   Rule&=Due(E::Pointed,RxAcquire,[](FRxDueIn& X){X.bPointReady=X.bSettled=X.bSky=true;})==RxTune; // after the fix: TUNE
   Rule&=Due(E::Pointed,RxTune,[](FRxDueIn& X){X.bPointReady=X.bSettled=X.bSky=X.bTuned=true;})==-1; // tuned on TUNE: START RECORDING (its key carries the cue)
   Rule&=Due(E::Pointed,RxData,[](FRxDueIn& X){X.bPointReady=X.bSettled=X.bSky=X.bTuned=true;})==RxTune; // ANOMALY DETECTED / START RECORDING lives on TUNE
   Rule&=Due(E::Pointed,RxTune,[](FRxDueIn& X){X.bPower=false;})==-1&&Due(E::New,RxTune,[](FRxDueIn& X){X.bGated=true;})==-1
    &&Due(E::Pointed,RxTune,[](FRxDueIn& X){X.bCaseWaits=true;})==-1&&Due(E::New,RxTune,[](FRxDueIn& X){X.bTutorialGate=true;})==-1; // power, evening, case test, first shift: elsewhere
   Rule&=Due(E::Pointed,RxTune,[](FRxDueIn& X){X.bArrayBlocked=true;})==RxArray; // isolated antennas
   Rule&=Due(E::Integrating,RxTune,[](FRxDueIn& X){X.bPointReady=X.bSettled=X.bSky=X.bTuned=true;})==-1; // recording: nothing to do
   Rule&=Due(E::Integrating,RxArray,[](FRxDueIn& X){X.bPointReady=X.bSettled=X.bSky=true;})==RxTune; // tuning lost
   Rule&=Due(E::Integrating,RxTune,[](FRxDueIn& X){X.bTuned=X.bSky=true;})==RxArray&&Due(E::Integrating,RxTune,[](FRxDueIn& X){X.bTuned=X.bSky=X.bCrossRunning=true;})==-1; // pointing lost; the reference scan waits
   Rule&=Due(E::Recorded,RxTune,[](FRxDueIn& X){X.bCanControl=true;})==RxRecord&&Due(E::Recorded,RxRecord,[](FRxDueIn& X){X.bCanControl=true;})==-1; // the internal control
   Rule&=Due(E::Chart,RxTune,nullptr)==-1&&Due(E::Registered,RxTune,[](FRxDueIn& X){X.bDataAsk=true;})==RxData&&Due(E::Registered,RxData,[](FRxDueIn& X){X.bDataAsk=true;})==-1;
   Check(Rule,TEXT("RX-TABS-DUE-20261113: the due receiver tab: nothing before TRACKING has pointed the job, then ARRAY for too few antennas or slewing, ACQUIRE until the fix, TUNE to tune and record, RECORD for the control, DATA to compare; never the open tab, nothing while waiting"));
   // RX-ANOMALY-FLOW-20261113 (Tom 13 November): an anomalous take is RECORD's from ANOMALY DETECTED until it is retained there
   bool AnomalyRule=Due(E::Pointed,RxTune,[](FRxDueIn& X){X.bPointReady=X.bSettled=X.bSky=X.bTuned=X.bAnomalyAhead=true;})==RxRecord
    &&Due(E::Pointed,RxRecord,[](FRxDueIn& X){X.bPointReady=X.bSettled=X.bSky=X.bTuned=X.bAnomalyAhead=true;})==-1;
   AnomalyRule&=Due(E::Integrating,RxTune,[](FRxDueIn& X){X.bPointReady=X.bSettled=X.bSky=X.bTuned=X.bPlainTake=true;})==RxRecord
    &&Due(E::Integrating,RxArray,[](FRxDueIn& X){X.bPointReady=X.bSettled=X.bSky=X.bPlainTake=true;})==RxTune; // tuning lost still goes first
   AnomalyRule&=Due(E::Recorded,RxTune,[](FRxDueIn& X){X.bRetain=true;})==RxRecord&&Due(E::Recorded,RxRecord,[](FRxDueIn& X){X.bRetain=true;})==-1
    &&Due(E::Recorded,RxTune,nullptr)==-1; // retained: the reel deck, no receiver tab
   Check(AnomalyRule,TEXT("RX-ANOMALY-FLOW-20261113: anomalous take, RECORD is due: at ANOMALY DETECTED, while it records and until RETAIN TAKE; then no receiver tab"));
   FS47FreeformSession Flow(nullptr);Flow.Begin(nullptr);Flow.Tutorial=0;
   const int32 FI=Flow.Work.IndexOfByPredicate([](const FJob& X){return X.Source==1&&X.CaseTest<0;});
   bool Chain=FI!=INDEX_NONE;
   if(Chain)
   {
    Flow.Selected=FI;FJob& FJ=Flow.Work[FI];Flow.ReceiverTab=RxTune;
    auto Rx=[&Flow]{Flow.RxDueFrame=~0ull;return Flow.ReceiverDueTab();};
    // ANOMALY DETECTED on a tuned, settled, fixed receiver: TUNE has START RECORDING
    Chain&=Due(E::Pointed,RxAcquire,[](FRxDueIn& X){X.bPointReady=X.bSettled=X.bSky=X.bTuned=true;})==RxTune;
    FJ.bAnomaly=true;FJ.Phenomenon=TEXT("extra_pulse");for(double At:Ticks)FJ.TraceTicks.Add(At);
    FJ.Stage=E::Recorded;FJ.Elapsed=18;FJ.ChartFeed=PrinterSeconds;
    // (DATA only while the take's case is open with comparisons left and its page unseen: RxDataAsk; a fresh station has no case open)
    auto Data=[&Flow,&FJ]{return Flow.RxDataAsk(FJ)?int32(RxData):-1;};
    Chain&=Rx()==RxRecord&&!Flow.OpsWorkDue(); // RX-ANOMALY-FLOW-20261113: on tape, not retained: RECORD, for RETAIN TAKE
    FJ.bPrinted=true;
    Chain&=Rx()==Data()&&!Flow.OpsWorkDue(); // on tape: the reel deck, no receiver tab to do, not OPS yet
    FJ.bAudioReviewed=true;
    Chain&=Rx()==Data()&&Flow.OpsWorkDue(); // heard: OPS, DAILY WORK
    FJ.Stage=E::Registered;Chain&=!Flow.OpsWorkDue()&&Rx()==Data(); // the printer
    FJ.Stage=E::Packet;Chain&=!Flow.OpsWorkDue()&&Rx()==Data(); // the binder
    Flow.RxDataSeenJob=FJ.Id;Chain&=Rx()==-1; // its DATA page seen: no more
    // a routine take waiting to be registered marks DAILY WORK; one still at the printer does not
    FJ.bAnomaly=false;FJ.Phenomenon.Empty();FJ.TraceTicks.Reset();FJ.bAudioReviewed=false;FJ.Stage=E::Recorded;Chain&=Flow.OpsWorkDue()&&Rx()==-1;
    FJ.Stage=E::Chart;FJ.ChartFeed=0;Chain&=!Flow.OpsWorkDue();
   }
   Check(Chain,TEXT("RX-TABS-DUE-20261113: the anomaly chain: TUNE for START RECORDING, no receiver tab at the reel deck, DAILY WORK marked at OPS once heard, none at the printer or binder"));
   Flow.Stop();
   Check(OpsWorkDueFor(E::Recorded,TEXT("OPS (corner PC): CONFIRM THE ANOMALY IN DAILY WORK"))&&!OpsWorkDueFor(E::Filed,TEXT("OPS (corner PC): SELECT DAILY WORK OR RECORDS"))&&!OpsWorkDueFor(E::Recorded,TEXT("REEL DECK (wall table): PLAY THE TAKE ONCE TO CONFIRM IT")),
    TEXT("RX-TABS-DUE-20261113: OPS marks DAILY WORK only for an OPS step after the take (confirm, register, compare)"));
  }
  Check(Workflow.ReceiverApproach()==0&&Workflow.ArrayErrorDeg()==0,TEXT("D45: with no array loaded the receiver shows nothing of the source and no slew"));
 }
 Check(TerminalNextLine(TEXT("FIRST SHIFT 1/5 / OPS (corner PC): OPEN DAILY WORK AND ACCEPT YOUR FIRST JOB"),3)==TEXT("NEXT / USE THIS PC"),TEXT("NEXT line reads past the first-shift counter"));
 Check(TerminalNextLine(TEXT("ARRAY 12/27 / MEASUREMENT RATE 43% / BLOCKED: inspect ARRAY STATUS (left-hand PC); need 24"),0)==TEXT("NEXT / USE THIS PC")&&TerminalNextLine(TEXT("ARRAY 12/27 / MEASUREMENT RATE 43% / BLOCKED: inspect ARRAY STATUS (left-hand PC); need 24"),1)==TEXT("NEXT / ARRAY STATUS (LEFT-HAND PC)"),TEXT("A blocked array sends every workstation to the status PC"));
 Check(TerminalNextLine(TEXT("EVENING OBSERVATION / 3C 286 IN VIEW FROM 19:20. TILL THEN: DAY WORK, REPAIRS, SUPPLIES, OR OUT WITH THE CAMERA. AFTER 02:00 IT IS BETTER INSIDE: ADVICE, NOT A RULE."),2)==TEXT("NEXT / 3C 286 IN VIEW FROM 19:20")&&TerminalNextLine(Workflow.EveningLine(*WJ),2).Contains(TEXT("IN VIEW FROM")),TEXT("An evening observation shows its time, and a clock's colon does not cut the line"));
 Check(TerminalNextLine(TEXT("S-03 CABLE JUNCTION / LOCAL BOX: SWITCH TO LOCAL AND OPEN THE RELAY HATCH"),1)==TEXT("NEXT / S-03 CABLE JUNCTION / LOCAL BOX")&&TerminalNextLine(TEXT("A very long sentence with no station in it that cannot fit on the glass at all"),1).IsEmpty()&&TerminalNextLine(TEXT("PAUSE MENU: LOAD A SAVED CASE TO CONTINUE AFTER CONTACT"),3).IsEmpty()&&TerminalNextLine(FString(),3).IsEmpty(),TEXT("NEXT line keeps a place that fits and says nothing rather than half a sentence"));
 {
  FS47FreeformSession Older(nullptr);Older.Begin(nullptr);
  Check(Older.MeasurementRecord().Contains(TEXT("0 FILED ORIGINALS / 0 REFERENCE BASELINES"))&&Older.MeasurementRecord().Contains(TEXT("No reference baseline")),TEXT("The bench's measurement record starts empty and says where a baseline comes from"));
  Older.Work[0].Stage=EJobStage::Filed;Older.Work[0].Id=TEXT("FF-D0002-REFERENCE-0005");const int32 OlderCash=Older.Budget();
  const FString OlderRecord=Older.MeasurementRecord();
  Check(OlderRecord.Contains(TEXT("1 FILED ORIGINALS / 1 REFERENCE BASELINES"))&&OlderRecord.Contains(TEXT("FF-D0002-REFERENCE-0005"))&&OlderRecord.Contains(TEXT("/ BASELINE"))&&!OlderRecord.Contains(TEXT("No reference baseline"))&&Older.Budget()==OlderCash,TEXT("The bench's measurement record lists a filed baseline and pays nothing"));
  Older.Stop();
 }
 Workflow.ServiceGuide=1;Workflow.bGuideFocus=true;
 Check(Workflow.ServiceGuideFocused()&&Workflow.SelectedJob()==0,TEXT("service focus is explicit without changing selected receiver job identity"));
 Workflow.FieldFocus=2;Check(!Workflow.ServiceGuideFocused()&&Workflow.SelectedJob()==-1,TEXT("field focus retains priority over legacy simultaneous service-focus flag"));
 Workflow.FieldFocus=-1;Workflow.ServiceGuide=2;Check(!Workflow.ServiceGuideFocused(),TEXT("archived service cannot remain followed"));Workflow.ServiceGuide=0;
 Workflow.bGuideFocus=true;Check(Workflow.SelectJob(1)&&!Workflow.bGuideFocus,TEXT("Explicit job selection returns focus from service to observing"));
 Workflow.Work[0].Stage=EJobStage::Chart;Workflow.Work[0].ChartFeed=PrinterSeconds;Workflow.Work[1].Stage=EJobStage::Integrating;
 Check(!Workflow.CollectChart()&&Workflow.FeedbackText().Contains(TEXT("Paper is waiting")),TEXT("Printer explains its pending queue when another recording prevents job switching"));Workflow.Work[1].Stage=EJobStage::New;
 Check(Workflow.CollectChart()&&Workflow.Selected==0&&Workflow.Work[0].Stage==EJobStage::Recorded,TEXT("Printer finds a waiting original even when another work order is selected"));
 WJ=Workflow.Job();WJ->Stage=EJobStage::Chart;Workflow.bPowerFault=true;Check(Workflow.NextAction().Contains(TEXT("PRINTER")),TEXT("An already printed original remains collectible during power loss"));WJ->Stage=EJobStage::Packet;Check(Workflow.NextAction().Contains(TEXT("BINDER")),TEXT("A collected packet can be physically filed without power"));Workflow.bPowerFault=false;
 WJ->Stage=EJobStage::Registered;WJ->bAnomaly=WJ->bControl=WJ->bAudioReviewed=true;WJ->ControlAt=8;WJ->MarkStart=9.2;WJ->MarkEnd=12.8;Workflow.AudioCursor=2;
 Check(!Workflow.AudioAction(TEXT("mark_start"))&&WJ->MarkStart==9.2&&WJ->bAudioReviewed,TEXT("Registered comparison marks and review are immutable while playback remains available"));
 const FString HintKey=WJ->Id+TEXT("/5");const auto BeforeHint=Workflow.Save();
 Check(Workflow.JournalHintLevel(HintKey)==0&&Workflow.RevealJournalHint(TEXT("unknown"))==0,TEXT("Journal reads and unknown hint keys do not invent hint state"));
 Check(Workflow.RevealJournalHint(HintKey)==1&&Workflow.RevealJournalHint(HintKey)==2&&Workflow.RevealJournalHint(HintKey)==3&&Workflow.RevealJournalHint(HintKey)==3&&WJ->Stage==EJobStage::Registered,TEXT("Explicit hints reveal at most three levels without completing work"));
 WJ->TraceTicks={9.3,11.4};WJ->bLegacyTrace=true;auto JournalSave=Workflow.Save();FS47FreeformSession JournalAgain(nullptr);JournalAgain.Begin(JournalSave);
 Check(JournalAgain.JournalHintLevel(HintKey)==3&&JournalAgain.CurrentJob()->TraceTicks.Num()==2,TEXT("Per-case hints and legacy trace survive Continue without duplicated ticks"));
 Check(JournalAgain.CurrentJob()->RecordingSoundVersion==0,TEXT("Legacy recordings remain beep-only without invented capture settings"));
 WJ->RecordingSoundVersion=1;WJ->RecordingGain=63;WJ->RecordingBandwidth=24;WJ->RecordingQuality=.85;
 FS47FreeformSession SoundAgain(nullptr);SoundAgain.Begin(Workflow.Save());const auto* Captured=SoundAgain.CurrentJob();
 Check(Captured&&Captured->RecordingSoundVersion==1&&Captured->RecordingGain==63&&Captured->RecordingBandwidth==24&&Captured->RecordingQuality==.85&&Captured->TraceTicks==WJ->TraceTicks,TEXT("Captured background settings and authoritative ticks survive Continue independently of current tuning"));SoundAgain.Stop();
 WJ->RecordingSoundVersion=2;FS47FreeformSession SoundV2(nullptr);SoundV2.Begin(Workflow.Save());
 Check(SoundV2.CurrentJob()&&SoundV2.CurrentJob()->RecordingSoundVersion==2&&SoundV2.CurrentJob()->RecordingGain==63&&SoundV2.CurrentJob()->RecordingBandwidth==24&&SoundV2.CurrentJob()->RecordingQuality==.85&&SoundV2.CurrentJob()->TraceTicks==WJ->TraceTicks,TEXT("v2 atmosphere version round-trips independently of current settings without changing authoritative evidence ticks"));SoundV2.Stop();
 auto Refund=Workflow.Save();Refund->RemoveField(TEXT("economyRevision"));S47J::Set(Refund,TEXT("upgrades"),TArray<double>{0,1,0});S47J::Set(Refund,TEXT("cash"),0);S47J::Set(Refund,TEXT("loan"),30);
 FS47FreeformSession RefundCase(nullptr);RefundCase.Begin(Refund);Check(RefundCase.Budget()==50&&RefundCase.Debt()==0&&!RefundCase.HasUpgrade(1)&&!RefundCase.Upgrade(1),TEXT("Retired comparison upgrade refunds debt first and cannot be bought again"));
 FS47FreeformSession RefundAgain(nullptr);RefundAgain.Begin(RefundCase.Save());Check(RefundAgain.Budget()==50&&RefundAgain.Debt()==0,TEXT("Refund is idempotent across save and Continue"));
 S47J::Set(Refund,TEXT("cash"),1000000);S47J::Set(Refund,TEXT("loan"),0);RefundCase.Begin(Refund);RefundAgain.Begin(RefundCase.Save());Check(RefundAgain.Budget()==1000080,TEXT("Legacy refund is not silently lost at the old cash ceiling"));
 Workflow.Stop();JournalAgain.Stop();RefundCase.Stop();RefundAgain.Stop();
 FS47FreeformSession Case(nullptr);Case.Begin(nullptr);
 Check(!Case.PlanCaseTest(0,0)&&Case.TonightPlan().IsEmpty(),TEXT("No case test can be planned before a reviewed discrepancy opens a case"));
 Case.RecordEvidence(TEXT("pulse-review"),TEXT("reviewed"));
 Check(Case.CaseFile().Get(0)->bOpen&&Case.TonightPlan().Contains(TEXT("NO TEST PLANNED")),TEXT("A reviewed extra-pulse discrepancy opens the interval case and shows tonight's plan"));
 const int32 JobsBefore=Case.Jobs().Num();
 Check(Case.PlanCaseTest(0,1)&&Case.Jobs().Num()==JobsBefore+1&&Case.Jobs().Last().CaseTest==1&&!Case.PlanCaseTest(0,0)&&Case.TonightPlan().Contains(TEXT("OFF-SOURCE")),TEXT("Planning a case test adds one dedicated job, shows it as tonight's plan and blocks a parallel plan"));
 Case.Selected=Case.Work.Num()-1;{const auto P=Case.EffectiveProfile(*Case.CurrentJob());Check(FMath::IsNearlyEqual(Case.TargetAz()-(P.Az[0]+P.Az[1])/2,FS47FreeformCases::OffSourceDegrees,1e-9),TEXT("The off-source case test points the array beside the pulsar"));}
 {
  auto& CJ=Case.Work.Last();const auto Truth=Case.CaseFile().Get(0)->Truth;const bool bHeard=FS47FreeformCases::PresentFor(0,Truth,1);
  if(bHeard)CJ.TraceTicks={9.3,11.4};CJ.Stage=EJobStage::Recorded;CJ.bPrinted=true;const int32 CashBefore=Case.Budget(),RelaysBefore=Case.InventoryCount(TEXT("relay"));
  Check(Case.RegisterComparison(TEXT("normal"))&&Case.Budget()==CashBefore+20&&Case.Work.Last().bCaseRecorded&&Case.CaseFile().Get(0)->Results[1].bDone&&Case.CaseFile().Get(0)->Results[1].bPresent==bHeard&&Case.InventoryCount(TEXT("relay"))==RelaysBefore+1,TEXT("A registered case test pays as routine work, retains its physical result and credits one service relay the first time"));
  FS47FreeformSession CaseAgain(nullptr);CaseAgain.Begin(S47J::Read(S47J::Write(Case.Save())));
  Check(CaseAgain.CaseFile().Get(0)->ValidTests==1&&CaseAgain.CaseFile().Get(0)->Planned<0&&CaseAgain.Jobs().Last().CaseTest==1&&CaseAgain.Jobs().Last().bCaseRecorded,TEXT("Case progress and the case job survive JSON save/Continue"));
  Check(!CaseAgain.ConcludeCase(0,Truth)&&CaseAgain.FeedbackText().Contains(TEXT("not excluded")),TEXT("One test cannot support a conclusion, even a correct guess"));
  CaseAgain.Stop();
 }
 {
  // Case 2 reads the receiver clock offset instead of the extra ticks.
  Case.RecordEvidence(TEXT("time_disagreement"),TEXT("timing path retained"));
  Check(Case.CaseFile().Get(1)&&Case.CaseFile().Get(1)->bOpen&&Case.CaseAwaitingChoice()>=0&&Case.TonightPlan().Contains(TEXT("DISAGREEING RECORDS")),TEXT("A retained timing discrepancy opens the disagreeing-records case alongside the first"));
  Check(Case.PlanCaseTest(1,1)&&Case.Jobs().Last().CaseLine==1,TEXT("The second case plans its own generator test job"));
  auto& DJ=Case.Work.Last();const int32 Truth=Case.CaseFile().Get(1)->Truth;const bool bOff=FS47FreeformCases::PresentFor(1,Truth,1);
  DJ.ReferenceClock=1000;DJ.ReceiverClock=1000+(bOff?FS47FreeformCases::ClockOffset:0);DJ.Stage=EJobStage::Recorded;DJ.bPrinted=true;
  Case.Selected=Case.Work.Num()-1;const int32 RelaysBefore=Case.InventoryCount(TEXT("relay"));
  Check(Case.RegisterComparison(TEXT("normal"))&&Case.CaseFile().Get(1)->Results[1].bDone&&Case.CaseFile().Get(1)->Results[1].bPresent==bOff&&Case.InventoryCount(TEXT("relay"))==RelaysBefore,TEXT("The clock-delta result is retained from the original counters; the first-test relay is not granted twice"));
 }
 {
  // Case 3: observation leads open the visitors case; its tests are field work reported by the world.
  FS47FreeformObservation O;O.Id=TEXT("observer.day1.1");O.Event=TEXT("observer");O.Place=TEXT("array service area");O.Bearing=TEXT("NE");O.Day=1;O.Clock=22*3600.;
  Case.Director->RetainObservation(O);Case.IngestObservations();
  Check(Case.CaseFile().Get(2)&&Case.CaseFile().Get(2)->bOpen&&Case.CaseFile().Get(2)->Leads.Contains(O.Id),TEXT("A retained visitor observation opens the visitors case as a lead"));
  Check(Case.PlanCaseTest(1,0),TEXT("Receiver-plan Continue fixture creates a new unfinished test after the previous result"));
  const int32 JobsBefore3=Case.Jobs().Num();
  Check(!Case.PlanCaseTest(2,2)&&Case.FeedbackText().Contains(TEXT("field logger")),TEXT("The two-site test waits for the loan field logger from the first case"));
  auto LoggerUnlocked=Case.Save();auto UnlockedCases=S47J::Obj(LoggerUnlocked,TEXT("cases"));
  S47J::Set(UnlockedCases,TEXT("access"),TArray<FString>{TEXT("access.fieldlogger")});
  FS47FreeformSession Unsupported(nullptr);Unsupported.Begin(S47J::Read(S47J::Write(LoggerUnlocked)));
  Check(Unsupported.CaseFile().HasAccess(TEXT("access.fieldlogger"))&&!Unsupported.PlanCaseTest(2,2)&&Unsupported.PlannedFieldTest(2)<0&&Unsupported.Jobs().Num()==JobsBefore3&&Unsupported.FeedbackText().Contains(TEXT("unavailable")),TEXT("Loan RF logger access does not enable unsupported two-site contact recording"));
  auto LegacyPlans=S47J::Arr(UnlockedCases,TEXT("cases"));
  S47J::Set(LegacyPlans[2]->AsObject(),TEXT("planned"),2);S47J::Set(LegacyPlans[2]->AsObject(),TEXT("plannedJob"),TEXT("legacy-two-sites"));
  Unsupported.Stop();Unsupported.Begin(S47J::Read(S47J::Write(LoggerUnlocked)));
  Check(Unsupported.PlannedFieldTest(2)==2&&Unsupported.FieldNextAction(2).Contains(TEXT("unavailable"))&&Unsupported.CancelFieldTest(2)&&Unsupported.PlanCaseTest(2,0),TEXT("Continue preserves the unavailable legacy plan and allows cancellation followed by a supported roof control"));
  Unsupported.Stop();
  Check(Case.PlanCaseTest(2,1)&&Case.Jobs().Num()==JobsBefore3&&Case.PlannedFieldTest(2)==1,TEXT("A field test is planned without a receiver job"));
  const FString FieldPlanId=Case.CaseFile().Get(2)->PlannedJob;
  FS47FreeformSession FieldAgain(nullptr);FieldAgain.Begin(S47J::Read(S47J::Write(Case.Save())));
  Check(FieldAgain.PlannedFieldTest(2)==1&&FieldAgain.CaseFile().Get(2)->PlannedJob==FieldPlanId&&FieldAgain.Jobs().Num()==JobsBefore3,TEXT("A planned field test survives JSON save/Continue with its original ID and no receiver job"));
  Check(FieldAgain.CaseFile().Get(1)->Planned==0,TEXT("A matching unfinished receiver case job remains planned after Continue"));
  Check(!FieldAgain.CancelFieldTest(1)&&FieldAgain.CaseFile().Get(1)->Planned==0,TEXT("Field-plan cancellation cannot cancel an active receiver case job"));
  auto MissingJob=Case.Save();const auto SavedCases=S47J::Arr(S47J::Obj(MissingJob,TEXT("cases")),TEXT("cases"));
  S47J::Set(SavedCases[1]->AsObject(),TEXT("plannedJob"),TEXT("missing-receiver-job"));
  FS47FreeformSession MissingAgain(nullptr);MissingAgain.Begin(S47J::Read(S47J::Write(MissingJob)));
  Check(MissingAgain.CaseFile().Get(1)->Planned<0&&MissingAgain.PlannedFieldTest(2)==1,TEXT("Continue discards an orphan receiver plan while preserving independent field work"));
  FieldAgain.Stop();MissingAgain.Stop();
  // CASETEST-20261109: an invalid field control is no result, and stays planned to take again with no new plan
  Check(Case.ReportFieldTest(2,1,false,false,TEXT("Too much of the window went unwatched."))&&Case.PlannedFieldTest(2)==1&&!Case.CaseFile().Get(2)->Results[1].bDone&&Case.CaseFile().Get(2)->ValidTests==0&&Case.FeedbackText().Contains(TEXT("stays planned")),TEXT("An invalid field control records nothing and stays planned to take again"));
  const bool bTrace=Case.FieldTestExpectsPresent(2,1);
  Check(!Case.ReportFieldTest(2,0,true,true,TEXT("x"))&&Case.ReportFieldTest(2,1,true,bTrace,TEXT("seal and strips inspected"))&&Case.CaseFile().Get(2)->Results[1].bDone&&Case.PlannedFieldTest(2)==-1&&!Case.FieldTestExpectsPresent(2,1),TEXT("Only the planned field test is answered and reported; the hidden cause is not readable afterwards"));
 }
 {
  // The night's mystery through the session. A trace opens it, an assessment goes in on that alone, and the next day's
  // reply follows what it was, not what was filed, and comes once. Under every cause, right and wrong.
  const int32 MLine=FS47FreeformCases::MysteryLine;
  for(int32 MWas=0;MWas<FS47FreeformCases::CauseCount;++MWas)
  {
   int32 MSeed=1;for(FS47FreeformCases MProbe;MSeed<512;++MSeed){MProbe.Reset(MSeed);if(MProbe.Get(MLine)->Truth==MWas)break;}
   FS47FreeformSession MBase(nullptr);MBase.Begin(nullptr);auto MState=MBase.Save();MBase.Stop();S47J::Set(MState,TEXT("jobSeed"),MSeed);S47J::Set(MState,TEXT("tutorial"),0);
   FS47FreeformSession MRight(nullptr);MRight.Begin(MState);
   Check(!MRight.CaseFile().Get(MLine)->bOpen&&MRight.CaseFile().Get(MLine)->Truth==MWas,TEXT("Mystery: closed until something is found, its cause set by the seed"));
   MRight.RecordEvidence(TEXT("carrier_saro"),MRight.CaseFile().ClueText(MLine,0));
   Check(MRight.CaseFile().Get(MLine)->bOpen&&MRight.FindingText(TEXT("carrier_saro"))==MRight.CaseFile().ClueText(MLine,0)&&MRight.TonightPlan().Contains(TEXT("file your assessment")),TEXT("Mystery: a kept trace opens the case and says what can be done"));
   const int32 MFuses=MRight.InventoryCount(TEXT("fuse"));const int32 MCash=MRight.Budget();
   Check(MRight.ConcludeCase(MLine,MWas)&&MRight.FeedbackText().Contains(TEXT("in the morning"))&&!MRight.FeedbackText().Contains(TEXT("right")),TEXT("Mystery: filing the right answer says nothing about being right"));
   MRight.StepCaseReplies();Check(!MRight.HasFinding(TEXT("case4-reply")),TEXT("Mystery: no reply the same day"));
   MRight.TestAdvance(86400.);MRight.StepCaseReplies();
   Check(MRight.HasFinding(TEXT("case4-reply"))&&!MRight.PowerFault()&&MRight.Budget()==MCash&&MRight.InventoryCount(TEXT("fuse"))==MFuses+(MWas==2?0:1),TEXT("Mystery: filed right, the next day brings no cost, a spare fuse, and nothing at all for the unexplained"));
   FS47FreeformSession MAgain(nullptr);MAgain.Begin(MRight.Save());MAgain.StepCaseReplies();
   Check(MAgain.CaseFile().Get(MLine)->bReplied&&MAgain.InventoryCount(TEXT("fuse"))==MRight.InventoryCount(TEXT("fuse")),TEXT("Mystery: the reply comes once, also after Continue"));
   MRight.Stop();MAgain.Stop();
   const int32 MFiled=(MWas+1)%FS47FreeformCases::CauseCount;
   FS47FreeformSession MWrong(nullptr);MWrong.Begin(MState);MWrong.RecordEvidence(TEXT("carrier_feeder"),MWrong.CaseFile().ClueText(MLine,1));
   // A plan not yet started goes back to ordinary work when the filing goes in.
   const int32 MJobs=MWrong.Jobs().Num();
   Check(MWrong.PlanCaseTest(MLine,1)&&MWrong.Jobs().Num()==MJobs+1&&MWrong.Jobs().Last().CaseTest==1,TEXT("Mystery: a control is one dedicated job"));
   Check(MWrong.ConcludeCase(MLine,MFiled)&&MWrong.Jobs().Last().CaseTest==-1&&MWrong.CaseFile().Get(MLine)->Planned<0&&MWrong.CaseFile().Get(MLine)->bConcluded,TEXT("Mystery: an assessment can be filed against the truth, and frees the planned job"));
   MWrong.TestAdvance(86400.);MWrong.StepCaseReplies();
   Check(MWrong.HasFinding(TEXT("case4-reply"))&&MWrong.InventoryCount(TEXT("fuse"))==MFuses,TEXT("Mystery: filed wrong, the next day never pays"));
   Check(!MWrong.ConcludeCase(MLine,MWas)&&MWrong.Active()&&MWrong.CaseFile().Get(MLine)->Conclusion==MFiled,TEXT("Mystery: a wrong filing stands, and the session goes on"));
   MWrong.Stop();
   // Nothing filed: the controls close after a while, the filing does not.
   FS47FreeformSession MLate(nullptr);MLate.Begin(MState);MLate.RecordEvidence(TEXT("carrier_saro"),MLate.CaseFile().ClueText(MLine,0));MLate.TestAdvance(3*86400.);
   Check(!MLate.PlanCaseTest(MLine,0)&&MLate.FeedbackText().Contains(TEXT("gone cold"))&&MLate.ConcludeCase(MLine,MWas),TEXT("Mystery: a cold trail refuses a new control and still takes the filing"));
   MLate.Stop();
  }
 }
 {
  // M7's late lines through the session (S47PredecessorLate.cpp keeps their traces): the place opens the case, a person is not
  // tested, the printed minute takes a control, a filing is answered next morning as for the first mystery, and the county trusts a
  // station that files.
  FS47FreeformSession LBase(nullptr);LBase.Begin(nullptr);auto LState=LBase.Save();LBase.Stop();S47J::Set(LState,TEXT("tutorial"),0);
  FS47FreeformSession L(nullptr);L.Begin(LState);
  const int32 Post=FS47FreeformCases::PostLine,Minute=FS47FreeformCases::MinuteLine;
  L.RecordEvidence(TEXT("post_empty"),L.CaseFile().ClueText(Post,0));
  Check(L.CaseFile().Get(Post)->bOpen&&!L.CaseFile().Get(Minute)->bOpen&&!L.PlanCaseTest(Post,1)&&L.FeedbackText().Contains(TEXT("other place")),TEXT("Late cases: the empty post opens on its place, and offers no recording to test a person"));
  L.RecordEvidence(TEXT("minute_chart"),L.CaseFile().ClueText(Minute,0));
  Check(L.CaseFile().Get(Minute)->bOpen&&L.PlanCaseTest(Minute,0),TEXT("Late cases: the printed minute opens on its chart and takes a generator control"));
  const int32 Trust=L.SheriffTrust();
  Check(L.ConcludeCase(Post,2)&&L.SheriffTrust()==Trust+1,TEXT("Late cases: an assessment on the empty post goes in on any evidence, and the county counts it"));
  L.TestAdvance(86400.);L.StepCaseReplies();
  Check(L.HasFinding(TEXT("case6-reply"))&&L.CaseFile().Get(Post)->bReplied,TEXT("Late cases: the reply comes the next morning"));
  L.Stop();
 }
 {
  // LOOP-WHOLE-20261109, step 2: OUT ON THE LAND through the session. A record out there opens it and not the visitors'; what is the
  // base's still opens the visitors'; its controls answer to the kit's own records (a magnetometer reading, also at a place read
  // before; the trip camera's own frames); CaseStep and the case sheet have it, and the filed case is in RECORDS' terms; an older
  // save's land leads move over on Continue, once.
  constexpr int32 Land=FS47FreeformCases::LandLine;
  int32 LSeed=1;for(FS47FreeformCases Probe;LSeed<512;++LSeed){Probe.Reset(LSeed);if(Probe.Get(Land)->Truth==0)break;}
  FS47FreeformSession LBase(nullptr);LBase.Begin(nullptr);auto LState=LBase.Save();LBase.Stop();S47J::Set(LState,TEXT("tutorial"),0);S47J::Set(LState,TEXT("jobSeed"),LSeed);
  FS47FreeformSession L(nullptr);L.Begin(LState);
  L.RecordEvidence(TEXT("cattle_ring_d1"),TEXT("Seven head of cattle standing close in a ring, all facing in."));
  const auto* LC=L.CaseFile().Get(Land);
  Check(LC->bOpen&&!L.CaseFile().Get(2)->bOpen&&LC->Leads.Contains(TEXT("cattle_ring_d1"))&&L.CaseStep(Land).Step!=ECaseStep::Closed,TEXT("Land: a record out there opens OUT ON THE LAND, not the visitors' case, and CaseStep has it"));
  L.RecordEvidence(TEXT("door-visit-1"),TEXT("By a door that was tried in the night."));
  Check(L.CaseFile().Get(2)->bOpen&&!L.CaseFile().Get(2)->Leads.Contains(TEXT("cattle_ring_d1")),TEXT("Land: what is the base's still opens the visitors' case"));
  Check(L.CaseSheetHead(Land).StartsWith(TEXT("ASSESSMENT / CLOSES WHEN YOU FILE"))&&!L.CaseSupported(Land)&&!L.PlanCaseTest(Land,FS47FreeformCases::LandNeedle)&&L.FeedbackText().Contains(TEXT("magnetometer")),
   TEXT("Land: the case sheet says it is an assessment and how it closes; one night has nothing behind it; a control waits for its kit"));
  L.TestGiveKit(TEXT("magnetometer"));
  Check(L.PlanCaseTest(Land,FS47FreeformCases::LandNeedle)&&L.PlannedFieldTest(Land)==FS47FreeformCases::LandNeedle&&L.CaseStep(Land).Step==ECaseStep::RunPlan,TEXT("Land: the magnetometer control is planned as field work"));
  L.RecordEvidence(TEXT("place_windmill_needle"),TEXT("Magnetometer at the windmill on the Baca lease. All steel."));
  Check(LC->Results[FS47FreeformCases::LandNeedle].bDone&&!LC->Results[FS47FreeformCases::LandNeedle].bPresent&&L.PlannedFieldTest(Land)<0&&L.HasFinding(TEXT("case9-test2"))&&L.CaseFile().Remaining(Land)==2,
   TEXT("Land: a magnetometer reading out there is the control's result, set against the survey sheet"));
  L.Field->TestBuiltTripCamera();
  Check(L.PlanCaseTest(Land,FS47FreeformCases::LandCamera),TEXT("Land: with the trip camera built, its control is planned"));
  L.RecordEvidence(TEXT("place_windmill_needle"),TEXT("again"));
  Check(L.PlannedFieldTest(Land)==FS47FreeformCases::LandCamera&&!LC->Results[FS47FreeformCases::LandCamera].bDone,TEXT("Land: a magnetometer reading does not answer the camera's control"));
  // T16-20: a blank roll from a camera that barely stood watch answers nothing; with its cover it is the result as before.
  if(L.TripCamera)
  {
   L.TripCamera->TestCover(600);L.RecordEvidence(TEXT("tripcam_d1"),TEXT("Trip camera at the windmill, read in the morning. The counter has not moved."));
   Check(!LC->Results[FS47FreeformCases::LandCamera].bDone&&L.PlannedFieldTest(Land)==FS47FreeformCases::LandCamera,TEXT("T16-20: a blank roll from a camera that stood watch ten minutes does not answer the land test"));
   L.TripCamera->TestCover(FS47TripCamera::EnoughCover);
  }
  L.RecordEvidence(TEXT("tripcam_d1"),TEXT("Trip camera at the windmill, read in the morning. The counter has not moved."));
  Check(LC->Results[FS47FreeformCases::LandCamera].bDone&&!LC->Results[FS47FreeformCases::LandCamera].bPresent&&L.CaseFile().Excluded(Land,2)&&L.CaseFile().Remaining(Land)==1&&LC->Leads.Contains(TEXT("tripcam_d1"))&&L.CaseStep(Land).Step==ECaseStep::File,
   TEXT("Land: the trip camera's own frames are the result: a quiet night rules out the unaccounted, and it is a field record of the land"));
  Check(L.CaseSupported(Land)&&L.ConcludeCase(Land,0)&&L.CaseStep(Land).Step==ECaseStep::AwaitReply&&L.CaseSheetHead(Land).Contains(TEXT("FILED AS YOUR ASSESSMENT")),TEXT("Land: with a control behind it the assessment is filed, as RECORDS shows a closed case"));
  L.TestAdvance(86400.);L.StepCaseReplies();
  Check(L.HasFinding(TEXT("case9-reply")),TEXT("Land: the reply comes the next morning"));
  // An older save (eight lines) whose visitors' case holds a land record: Continue moves it to the land, with the trip camera record it
  // kept, never a lead before; the visitors' case, opened by it alone, closes; the next Continue finds the same.
  FS47FreeformSession OBase(nullptr);OBase.Begin(LState);
  OBase.RecordEvidence(TEXT("cattle_ring_d1"),TEXT("Cattle in a ring."));OBase.RecordEvidence(TEXT("tripcam_d1"),TEXT("Trip camera, read in the morning."));
  auto OState=OBase.Save();OBase.Stop();
  auto OCases=S47J::Obj(OState,TEXT("cases"));auto OList=S47J::Arr(OCases,TEXT("cases"));
  auto Visitors=S47J::New();S47J::Set(Visitors,TEXT("open"),true);S47J::Set(Visitors,TEXT("openDay"),1);S47J::Set(Visitors,TEXT("leads"),TArray<FString>{TEXT("cattle_ring_d1")});
  if(OList.Num()==FS47FreeformCases::CaseCount){OList[2]=MakeShared<FJsonValueObject>(Visitors);OList.SetNum(Land);OCases->SetArrayField(TEXT("cases"),OList);}
  FS47FreeformSession O(nullptr);O.Begin(S47J::Read(S47J::Write(OState)));
  const auto* OL=O.CaseFile().Get(Land);
  Check(OL&&OL->bOpen&&OL->Leads.Contains(TEXT("cattle_ring_d1"))&&OL->Leads.Contains(TEXT("tripcam_d1"))&&!O.CaseFile().Get(2)->bOpen&&O.CaseFile().FromBeforeLand(),
   TEXT("Land: an older save's land leads move to OUT ON THE LAND on Continue, and the visitors' case, opened by them alone, closes"));
  FS47FreeformSession O2(nullptr);O2.Begin(S47J::Read(S47J::Write(O.Save())));
  Check(OL&&O2.CaseFile().Get(Land)->Leads==OL->Leads&&!O2.CaseFile().FromBeforeLand()&&!O2.CaseFile().Get(2)->bOpen,TEXT("Land: the next Continue finds the same"));
  O.Stop();O2.Stop();L.Stop();
 }
 Case.Stop();
 {
  // CASES-OPS-20261109: CaseStep through the session; a trial case concluded with a test still planned frees its job, which then
  // registers as routine work and records nothing for the closed case. Tom, 13 November: the repeat is withdrawn, never planned, and
  // an older save's repeat job goes on Continue without a word (or, already recorded, is routine work).
  FS47FreeformSession P(nullptr);P.Begin(nullptr);P.Tutorial=0;
  Check(P.CaseStep(0).Step==ECaseStep::Closed&&P.UrgentCase()<0&&!P.HudObjective().Contains(TEXT("\n")),TEXT("CaseStep: no case, no step, and the HUD card has one line (TASKS-TEN change 5)"));
  P.RecordEvidence(TEXT("pulse-review"),TEXT("reviewed"));
  Check(P.CaseStep(0).Step==ECaseStep::ChooseTest&&P.CaseAwaitingChoice()==0&&P.CaseStep(0).Task.Contains(TEXT("choose a test"))&&!P.HudObjective().Contains(TEXT("\n")),TEXT("CaseStep: an opened case asks for a test on its own row, and the HUD card keeps one line (TASKS-TEN change 5)"));
  const int32 Truth=P.CaseFile().Get(0)->Truth;
  const int32 UpBefore=P.CardsUp();
  Check(P.PlanCaseTest(0,1)&&P.CaseStep(0).Step==ECaseStep::RunPlan&&P.CaseAwaitingChoice()<0&&P.Tasks().ContainsByPredicate([](const FS47Task& T){return T.Text.Contains(TEXT(": run job "));}),TEXT("CaseStep: a planned test is run, and the case board's star is off"));
  {
   // TASKS-TEN-20261113 (change 8): a planned case test is never an "unfinished observation": the cards up stay as they were, and the
   // test is the case's own row (WORK, the station's), the ladder's step naming it (a job case test has no dated window: Tom withdrew
   // the repeat's on 13 November; the field controls' windows are FieldWindowWaits')
   P.WorkActivities(nullptr);
   Check(P.CardsUp()==UpBefore&&P.PickRow==P.CaseStep(0).Task&&P.CaseStep(0).Task.Contains(TEXT(": run job ")),
    TEXT("TASKS-TEN (change 8): a planned case test is not counted among the cards up; it is named on its own row, and the ladder's step is that row"));
  }
  auto St=P.Save();const auto List=S47J::Arr(S47J::Obj(St,TEXT("cases")),TEXT("cases"));
  TArray<TSharedPtr<FJsonValue>> Kept;
  // (tests 1 and 3 kept: the third is the withdrawn repeat, an older save's result, still evidence)
  for(int32 T=0;T<FS47FreeformCases::TestCount;++T){auto Y=S47J::New();if(T!=1){S47J::Set(Y,TEXT("done"),true);S47J::Set(Y,TEXT("present"),FS47FreeformCases::PresentFor(0,Truth,T));S47J::Set(Y,TEXT("day"),1);}Kept.Add(MakeShared<FJsonValueObject>(Y));}
  List[0]->AsObject()->SetArrayField(TEXT("results"),Kept);
  FS47FreeformSession Q(nullptr);Q.Begin(S47J::Read(S47J::Write(St)));Q.Tutorial=0;
  Check(Q.CaseFile().Get(0)->Planned==1&&Q.CaseFile().Remaining(0)==1&&Q.CaseStep(0).Step==ECaseStep::Conclude&&Q.UrgentCase()==0,TEXT("CaseStep: one explanation left is a conclusion to register, first of anything"));
  Check(!Q.PlanCaseTest(0,1)||Q.CaseFile().Get(0)->Planned==1,TEXT("CaseStep: no second plan beside the first"));
  Check(Q.ConcludeCase(0,Truth)&&Q.CaseFile().Get(0)->Planned<0&&Q.Jobs().Last().CaseTest==-1&&Q.Jobs().Last().CaseLine==-1&&Q.CaseStep(0).Step==ECaseStep::Closed,TEXT("Conclusion: a trial case's planned test job goes back to ordinary work"));
  {
   auto& QJ=Q.Work.Last();QJ.Stage=EJobStage::Recorded;QJ.bPrinted=true;Q.Selected=Q.Work.Num()-1;
   Check(Q.RegisterComparison(TEXT("normal"))&&!Q.CaseFile().Get(0)->Results[1].bDone&&Q.CaseFile().Get(0)->bConcluded,TEXT("Conclusion: the freed job registers as routine and records nothing for the closed case"));
  }
  Check(Q.CaseRecordLine(TEXT("case1-test1")).Contains(TEXT("registered"))&&Q.CaseRecordLine(TEXT("case1-conclusion")).StartsWith(TEXT("THE REPEATED INTERVAL: "))&&FS47NightLog::CaseLine(Q.CaseRecordLine(TEXT("case1-conclusion")))&&FS47NightLog::CaseLine(TEXT("Case 1, THE REPEATED INTERVAL: old sheet"))&&!Q.CaseRecordLine(TEXT("case1-test1")).Contains(TEXT("Case 1"))&&Q.CaseRecordLine(TEXT("carrier_saro")).IsEmpty(),TEXT("Case records: one WORK line each for the night log"));
  FS47FreeformSession Wait(nullptr);Wait.Begin(nullptr);Wait.Tutorial=0;Wait.RecordEvidence(TEXT("pulse-review"),TEXT("reviewed"));
  {
   const int32 Before=Wait.Work.Num();
   Check(!Wait.PlanCaseTest(0,2)&&Wait.Work.Num()==Before&&Wait.CaseFile().Get(0)->Planned<0&&Wait.CaseStep(0).Step==ECaseStep::ChooseTest,TEXT("Repeat withdrawn: the repeat test cannot be planned, and the case still asks for a test"));
   // An older save: a repeat planned on a receiver job (as off-source is here, then rewritten to the third test), and a second one
   // already recorded. On Continue the first job and the plan go, the second is routine work; nothing says so.
   Check(Wait.PlanCaseTest(0,1),TEXT("Repeat withdrawn fixture: a receiver test planned"));
   auto RepeatSave=S47J::Read(S47J::Write(Wait.Save()));const FString PlannedId=Wait.CaseFile().Get(0)->PlannedJob;
   {
    const auto CaseList=S47J::Arr(S47J::Obj(RepeatSave,TEXT("cases")),TEXT("cases"));if(CaseList.Num()>0)S47J::Set(CaseList[0]->AsObject(),TEXT("planned"),2);
    TArray<TSharedPtr<FJsonValue>> SavedJobs=S47J::Arr(RepeatSave,TEXT("jobs"));
    for(const auto& V:SavedJobs)if(V&&V->Type==EJson::Object&&S47J::Str(V->AsObject(),TEXT("id"))==PlannedId)
    {
     const auto O=V->AsObject();S47J::Set(O,TEXT("caseTest"),2);
     const auto Copy=S47J::Clone(O);S47J::Set(Copy,TEXT("id"),PlannedId+TEXT("-B"));S47J::Set(Copy,TEXT("stage"),int32(EJobStage::Recorded));S47J::Set(Copy,TEXT("printed"),true);
     if(const auto Card=S47J::Obj(Copy,TEXT("card")))S47J::Set(Card,TEXT("text"),FString(TEXT("CASE TEST / REPEAT IN THE SAME WINDOW\nRecord the pulsar again.\n\nPULSAR ROUTINE")));
     SavedJobs.Add(MakeShared<FJsonValueObject>(Copy));break;
    }
    RepeatSave->SetArrayField(TEXT("jobs"),SavedJobs);
   }
   FS47FreeformSession Gone(nullptr);Gone.Begin(RepeatSave);Gone.Tutorial=0;
   const FJob* Freed=Gone.Jobs().FindByPredicate([&](const FJob& X){return X.Id==PlannedId+TEXT("-B");});
   Check(!Gone.Jobs().ContainsByPredicate([&](const FJob& X){return X.Id==PlannedId;})&&Gone.CaseFile().Get(0)->Planned<0&&Gone.CaseStep(0).Step==ECaseStep::ChooseTest
    &&Freed&&Freed->CaseTest<0&&Freed->CaseLine<0&&Freed->Card.Receiver.Card==TEXT("PULSAR ROUTINE")&&!Gone.FeedbackText().Contains(TEXT("SAME WINDOW"))&&!Gone.FeedbackText().Contains(TEXT("REPEAT TEST"))
    &&!Gone.TonightPlan().Contains(TEXT("SAME WINDOW"))&&!Gone.TonightPlan().Contains(TEXT("REPEAT WINDOW"))&&!Gone.CaseNextLine(0).Contains(TEXT("WINDOW")),TEXT("Repeat withdrawn: an older save's repeat job goes on Continue, a recorded one is routine work, and nothing mentions it"));
   Gone.Stop();
  }
  Wait.RecordEvidence(TEXT("case1-test3"),TEXT("CASE / test"));
  Check(Wait.HasFinding(TEXT("case1-test3"))&&FS47FreeformCases::IsCaseRecord(TEXT("case1-test3")),TEXT("Case records stay findings (for the night log) without a field record"));
  P.Stop();Q.Stop();Wait.Stop();
 }
 {
  // CASETEST-20261109: a spoiled receiver test stops at once and stays planned on the same job (TEST SPOILED on its row), is recorded
  // again from there or can still be cancelled; a relay fitted for the timing swap goes back to the depot; Continue observation takes
  // a case test that can be recorded now over a routine job not yet begun, but never one that waits for its window.
  FS47FreeformSession Sp(nullptr);Sp.Begin(nullptr);Sp.Tutorial=0;Sp.RecordEvidence(TEXT("pulse-review"),TEXT("reviewed"));
  const int32 Routine=Sp.Work.IndexOfByPredicate([](const FJob& X){return X.CaseTest<0&&X.Stage==EJobStage::New;});
  Check(Routine!=INDEX_NONE&&Sp.PlanCaseTest(0,1),TEXT("Continue fixture: a routine job not yet begun, and an off-source case test planned"));
  Sp.Selected=Routine;
  Check(Sp.ContinueJob()==Sp.Work.Num()-1,TEXT("Continue observation: an off-source case test, ready now, comes before a routine job not yet begun"));
  Check(Sp.CancelCaseJob(0)&&Sp.PlanCaseTest(0,0),TEXT("Spoiled test fixture: a generator test planned"));
  const FString SpId=Sp.Work.Last().Id;const int32 SpJobs=Sp.Work.Num();Sp.Selected=Routine;
  Check(Sp.ContinueJob()==Routine,TEXT("Continue observation: a case test whose condition is not met (generator stopped) is not offered first"));
  Sp.Selected=SpJobs-1;
  {auto& SJ=Sp.Work.Last();SJ.Stage=EJobStage::Integrating;SJ.Elapsed=9;SJ.bCaseValid=true;Sp.SpoilCaseTest(SJ,TEXT("the standby generator stopped,"));}
  {
   const auto& SJ=Sp.Work.Last();
   Check(SJ.Id==SpId&&Sp.Work.Num()==SpJobs&&SJ.Stage==EJobStage::Pointed&&SJ.Elapsed==0&&!SJ.bCaseValid&&!SJ.bCaseRecorded&&Sp.CaseFile().Get(0)->Planned==0&&Sp.CaseFile().Get(0)->PlannedJob==SpId
    &&StatusWord(SJ,true)==TEXT("TEST SPOILED")&&Sp.FeedbackText().Contains(TEXT("START RECORDING again"))&&Sp.CaseNextLine(0).Contains(TEXT("TEST SPOILED"))&&Sp.CaseStep(0).Next.Contains(TEXT("AGAIN")),
    TEXT("Spoiled receiver test: the recording stops, the same job stays planned and says TEST SPOILED, take it again"));
  }
  FS47FreeformSession SpAgain(nullptr);SpAgain.Begin(S47J::Read(S47J::Write(Sp.Save())));
  Check(SpAgain.CaseFile().Get(0)->Planned==0&&SpAgain.CaseFile().Get(0)->PlannedJob==SpId&&SpAgain.Jobs().Last().Id==SpId&&!SpAgain.Jobs().Last().bCaseValid,TEXT("Spoiled receiver test: its plan and its TEST SPOILED survive JSON save/Continue"));
  SpAgain.Stop();
  Check(Sp.CancelCaseJob(0)&&Sp.CaseFile().Get(0)->Planned<0&&Sp.Work.Num()==SpJobs-1,TEXT("Spoiled receiver test: still cancellable on the case board, since it is not recording"));
  Sp.RecordEvidence(TEXT("time_disagreement"),TEXT("timing path retained"));
  Check(Sp.CaseFile().Get(1)->bOpen&&Sp.PlanCaseTest(1,0),TEXT("Relay fixture: the timing swap planned"));
  Sp.AddItem(TEXT("relay"),1);const int32 Relays=Sp.InventoryCount(TEXT("relay"));Sp.Selected=Sp.Work.Num()-1;
  {auto& RJ=Sp.Work.Last();Check(Sp.Consume(TEXT("relay")),TEXT("Relay fixture: the spare is fitted as the recording starts"));RJ.Stage=EJobStage::Integrating;Sp.SpoilCaseTest(RJ,TEXT("a test fixture stopped it,"));}
  Check(Sp.InventoryCount(TEXT("relay"))==Relays&&Sp.Work.Last().Stage==EJobStage::Pointed,TEXT("Timing swap: a recording stopped short gives its fitted relay back to the depot"));
  // M3: given back once. A second stop on the stopped job, or a stop of a run already registered, gives nothing more.
  Sp.SpoilCaseTest(Sp.Work.Last(),TEXT("a test fixture stopped it again,"));Sp.UnfitCaseRelay(Sp.Work.Last());
  {auto& RJ=Sp.Work.Last();RJ.Stage=EJobStage::Integrating;RJ.bCaseRecorded=true;Sp.UnfitCaseRelay(RJ);RJ.bCaseRecorded=false;RJ.Stage=EJobStage::Pointed;}
  Check(Sp.InventoryCount(TEXT("relay"))==Relays,TEXT("Timing swap: the fitted relay comes back once, never for a second stop or a run already registered (M3)"));
  // M3: lost time in the middle of the swap gives the relay back once too, and the test stays planned on the same job.
  {auto& RJ=Sp.Work.Last();Check(Sp.Consume(TEXT("relay")),TEXT("Relay fixture: the spare fitted again"));RJ.Stage=EJobStage::Integrating;RJ.Elapsed=5;}
  const FString SwapId=Sp.Work.Last().Id;Sp.TakenAway(600.);Sp.TakenAway(600.);
  Check(Sp.InventoryCount(TEXT("relay"))==Relays&&Sp.Work.Last().Id==SwapId&&Sp.Work.Last().Stage==EJobStage::Pointed&&!Sp.Work.Last().bCaseRecorded&&Sp.CaseFile().Get(1)->Planned==0
   &&Sp.CaseFile().Get(1)->PlannedJob==SwapId&&!Sp.CaseFile().Get(1)->Results[0].bDone,TEXT("Timing swap: lost time mid-recording gives the relay back once, no result, the same job still planned (M3)"));
  Sp.Stop();
 }
 {
  // CASES-OPS-20261109, step 4: the case sheet says NEXT, the kind and ON CLOSE before any choice; a registered test result is printed
  // as a stamped result sheet after the job's papers, kept across a Continue, collected at the printer, and OPS then opens on its case.
  FS47FreeformSession Sh(nullptr);Sh.Begin(nullptr);Sh.Tutorial=0;Sh.RecordEvidence(TEXT("pulse-review"),TEXT("reviewed"));
  const FString Head=Sh.CaseSheetHead(0);
  Check(Sh.CaseNextLine(0).StartsWith(TEXT("NEXT > PLAN A TEST"))&&Head.Contains(TEXT("TRIAL CASE / CLOSES WHEN"))&&Head.Contains(TEXT("ON CLOSE: RECORDS REPLY AT ONCE"))
   &&Head.Contains(TEXT("SOURCE: RETAINED ORIGINAL"))&&Sh.CaseStanding(0,0)==TEXT("STILL POSSIBLE")&&Sh.CaseStanding(0,2)==TEXT("STILL POSSIBLE"),TEXT("Case sheet: NEXT from CaseStep, the kind, how it closes, ON CLOSE and the source, all three explanations still possible"));
  Sh.Cases.Open(FS47FreeformCases::MysteryLine,1,2*3600.);
  Check(Head.Contains(TEXT("ANSWERED AT ONCE")),TEXT("Case sheet: a trial case says it is answered at once"));
  Check(CaseKind(FS47FreeformCases::MysteryLine)==TEXT("ASSESSMENT")&&Sh.CaseSheetHead(FS47FreeformCases::MysteryLine).Contains(TEXT("ASSESSMENT / CLOSES WHEN YOU FILE"))&&Sh.CaseSheetHead(FS47FreeformCases::MysteryLine).Contains(TEXT("ANSWERED NEXT MORNING"))&&Sh.CaseOnClose(FS47FreeformCases::MysteryLine).Contains(TEXT("REPLY 08:00"))&&Sh.CaseNextLine(FS47FreeformCases::MysteryLine).StartsWith(TEXT("NEXT > ")),TEXT("Case sheet: an assessment says so, and that its reply comes at 08:00"));
  Check(Sh.PlanCaseTest(0,1),TEXT("Result sheet fixture: off-source pointing planned"));
  {auto& SJ=Sh.Work.Last();SJ.Stage=EJobStage::Recorded;SJ.bPrinted=true;Sh.Selected=Sh.Work.Num()-1;}
  const FString JobId=Sh.Work.Last().Id;
  Check(Sh.RegisterComparison(TEXT("normal"))&&Sh.CaseFile().Get(0)->Results[1].bDone&&Sh.CaseSheets.Num()==1&&Sh.SheetIndex()==0&&Sh.PrinterHint().Contains(TEXT("result sheet")),TEXT("Result sheet: registering a case test queues one sheet on the printer"));
  const auto& Res=Sh.CaseFile().Get(0)->Results[1];int32 Fell=0;
  for(int32 Cause=0;Cause<FS47FreeformCases::CauseCount;++Cause)Fell+=FS47FreeformCases::PresentFor(0,Cause,1)!=Res.bPresent?1:0;
  const FResultSheet Q0=Sh.CaseSheets[0];
  Check(Q0.Test==1&&Q0.Job==JobId&&(Fell>0?Q0.Stamp==FString::Printf(TEXT("RULED OUT / D%02d"),Q0.Day):Q0.Stamp==TEXT("STILL POSSIBLE"))&&Q0.Page.Contains(TEXT("NEXT > "))&&Q0.Page.Contains(TEXT("RESULT / "))&&Q0.Page.Contains(TEXT("ON CLOSE: "))
   &&(Fell==0||(Q0.Page.Contains(TEXT("<< THIS RESULT"))&&Q0.Page.Contains(TEXT("RULED OUT D")))),TEXT("Result sheet: stamped RULED OUT / Dnn when an explanation fell, STILL POSSIBLE when none did, with NEXT and the fallen row marked"));
  Check(Sh.Objective().Contains(TEXT("RESULT SHEET")),TEXT("Result sheet: the objective sends the player to the printer"));
  FS47FreeformSession ShR(nullptr);ShR.Begin(S47J::Read(S47J::Write(Sh.Save())));
  Check(ShR.CaseSheets.Num()==1&&ShR.CaseSheets[0].Page==Q0.Page&&ShR.CaseSheets[0].Stamp==Q0.Stamp,TEXT("Result sheet: a sheet still on the printer survives Save and Continue unchanged"));
  ShR.Work[0].Stage=EJobStage::Chart;ShR.Work[0].ChartFeed=0;
  Check(ShR.SheetIndex()<0&&ShR.PrinterJobId()==ShR.Work[0].Id,TEXT("Result sheet: a job's paper prints first; the sheet waits behind it"));
  Check(!Sh.CollectChart()&&Sh.CaseSheets.Num()==1,TEXT("Result sheet: a sheet not yet printed cannot be collected"));
  Sh.CaseSheets[0].Feed=PrinterSeconds;
  Check(Sh.CollectChart()&&Sh.CaseSheets.IsEmpty()&&Sh.BoardLine==0&&Sh.FreshLine==0&&Sh.FreshTest==1&&Sh.FeedbackText().StartsWith(Q0.Stamp),TEXT("Result sheet: collected at the printer, and the next visit to OPS opens CASE BOARD on its case"));
  // REQUISITION, never debt, in what OPS says (Tom's word for the balance on STATION ACCOUNT).
  FString Said=SopCard().Page+OpsAccountLine(31,12)+QueueLine(0,0,0,0)+QueueLine(0,0,0,1)+QueueLine(3,2,1,0)+Q0.Page;
  for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)Said+=Sh.CaseSheetHead(Line)+Sh.CaseNextLine(Line)+CaseClosesWhen(Line)+Sh.CaseOnClose(Line);
  {auto Was=S47J::New();S47J::Set(Was,TEXT("economyRevision"),1);S47J::Set(Was,TEXT("upgrades"),TArray<double>{0,1,0});FS47FreeformSession E(nullptr);E.Begin(Was);Said+=E.FeedbackText();E.Stop();}
  Check(!Said.Contains(TEXT("DEBT"),ESearchCase::IgnoreCase)&&Said.Contains(TEXT("REQ $12")),TEXT("OPS text says REQUISITION, never DEBT"));
  Check(TerminalStation(3)==TEXT("SARO OPS V4.2  TERM 1  OPR NIGHT")&&!TerminalStation(1).Contains(TEXT("REYES"))&&Sh.TerminalDate(8*3600.).Len()==9&&Sh.TerminalDate(8*3600.).Contains(TEXT("-86")),TEXT("OPS header as Story's: SARO OPS V4.2, terminal, a new operator and DD-MON-86"));
  Sh.Stop();ShR.Stop();
 }
 {
  // LOOP-1-20261109: a bearing slip waits on the printer as a paper of no case, one a day, survives a Continue, and is collected there.
  FS47FreeformSession Sl(nullptr);Sl.Begin(nullptr);Sl.Tutorial=0;
  // D45-20261110: the slip as FS47Places writes it now (plain words, no degree sign), and its row on the list from the bearing it left
  FS47Places::FBearing Fix=FS47Places::StationFix(4,Sl.WorldClock,0,33,-300,-560,1340,.5f,TEXT("mine"));
  const bool bQueued=Sl.QueueBearingSlip(4,FS47Places::SlipLine(Fix),FS47Places::SlipNote(Fix,0))&&!Sl.QueueBearingSlip(4,TEXT("again"),TEXT("again"));
  FS47FreeformSession SlR(nullptr);SlR.Begin(S47J::Read(S47J::Write(Sl.Save())));
  const bool bKept=SlR.CaseSheets.Num()==1&&IsBearingSlip(SlR.CaseSheets[0])&&SlR.CaseSheets[0].Page==Sl.CaseSheets[0].Page;
  bool bAscii=true;for(const TCHAR Ch:Sl.CaseSheets[0].Page)bAscii&=Ch==TEXT('\n')||(Ch>=32&&Ch<127);
  Check(bQueued&&Sl.CaseSheets.Num()==1&&Sl.SheetIndex()==0&&Sl.PrinterHint().Contains(TEXT("bearing slip"))&&bKept&&Sl.CaseSheets[0].Page.Contains(TEXT("GOOD UNTIL NOON"))&&Sl.CaseSheets[0].Page.Contains(TEXT("DIRECTION ONLY"))
   &&Sl.CaseSheets[0].Page.Contains(TEXT("CIRCLED ON YOUR MAP"))&&Sl.CaseSheets[0].Page.Contains(TEXT("A BRANCH IS ONE ARM"))&&bAscii&&!Sl.CaseSheets[0].Page.Contains(TEXT("47"))&&S47Script::LintText(Sl.CaseSheets[0].Page).IsEmpty(),
   TEXT("Bearing slip: queued on the printer once a day, kept over a Continue, in plain words with no degree sign, and no 47 on it"));
  {
   // the bearing's own row on the list (FS47Places::SlipRows), through a save as the game keeps it
   auto Fx=S47J::New();S47J::Set(Fx,TEXT("day"),4);S47J::Set(Fx,TEXT("az"),Fix.Az);S47J::Set(Fx,TEXT("spread"),Fix.Spread);S47J::Set(Fx,TEXT("target"),Fix.Target);
   S47J::Set(Fx,TEXT("clock"),Fix.Clock);S47J::Set(Fx,TEXT("until"),Fix.Until);S47J::Set(Fx,TEXT("to"),TArray<double>{Fix.ToX,Fix.ToZ});S47J::Set(Fx,TEXT("ring"),TArray<double>{Fix.RingX,Fix.RingZ,Fix.RingR});
   auto Pl=S47J::New();TArray<TSharedPtr<FJsonValue>> One;One.Add(MakeShared<FJsonValueObject>(Fx));Pl->SetArrayField(TEXT("fixes"),One);
   if(Sl.Places)Sl.Places->Begin(Pl);
   const TArray<FS47Task> Listed=Sl.Tasks();
   Check(Fix.bRing&&Sl.Places&&Listed.ContainsByPredicate([](const FS47Task& Tk){return Tk.Text.StartsWith(TEXT("BEARING SLIP / New circle on your map, "))&&Tk.Text.EndsWith(TEXT("Drive out and look before noon."))&&!Tk.bDone;}),
    TEXT("Bearing slip: a live slip is on the task list, and so on NOW: a new circle on the map, look before noon"));
  }
  Sl.CaseSheets[0].Feed=PrinterSeconds;
  Check(Sl.CollectChart()&&Sl.CaseSheets.IsEmpty()&&Sl.BoardLine<0&&Sl.FeedbackText().StartsWith(TEXT("BEARING SLIP"))&&Sl.FeedbackText().Contains(TEXT("before noon")),TEXT("Bearing slip: collected at the printer like a result sheet, with no case board to open on"));
  Sl.Stop();SlR.Stop();
 }
 {
  // Tom, 9 November: RETAIN at the reel deck keeps the comparison and opens no case, even with the event's own record after it; a
  // Continue in between keeps it so; registering it at OPS opens THE REPEATED INTERVAL and says CASE OPENED, its SOURCE line from
  // the original's own clock. An older save whose case is already open goes on as before.
  FS47FreeformSession Fc(nullptr);Fc.Begin(nullptr);Fc.Tutorial=0;Fc.WorldClock=22*3600.;
  const int32 FcI=Fc.Work.IndexOfByPredicate([](const FJob& X){return X.Source==1&&X.CaseTest<0;});
  Check(FcI!=INDEX_NONE,TEXT("First case fixture: a pulsar job on the first day"));
  if(FcI!=INDEX_NONE)
  {
   Fc.Selected=FcI;
   {
    // GAMEPLAY-LOOP-01: the take heard once at the reel deck; no control, no marks set by hand
    FJob& FcJ=Fc.Work[FcI];FcJ.Stage=EJobStage::Recorded;FcJ.bPrinted=true;FcJ.bAnomaly=true;FcJ.bControl=false;FcJ.ControlAt=0;
    FcJ.OriginalCovered=18;FcJ.ControlCovered=0;FcJ.ReferenceClock=21.5*3600.;
   }
   const bool bRetained=Fc.AudioAction(TEXT("confirm"));
   Fc.RecordEvidence(TEXT("extra_pulse"),TEXT("An extra pulse group survives a separate receiver check. The source is unresolved.")); // the event's own, as the Director keeps it
   Check(bRetained&&Fc.Work[FcI].bAudioReviewed&&Fc.HasFinding(TEXT("pulse-review"))&&!Fc.CaseFile().Get(0)->bOpen&&Fc.CaseStep(0).Step==ECaseStep::Closed&&Fc.CaseAwaitingChoice()<0
    &&Fc.FeedbackText().Contains(TEXT("Confirm anomaly: source unresolved"))&&!Fc.FeedbackText().Contains(TEXT("CASE OPENED")),TEXT("First case: one listen at the reel deck keeps the take, opens no case and says only the next step, the confirmation at OPS"));
   const auto Between=S47J::Read(S47J::Write(Fc.Save()));
   FS47FreeformSession Fc2(nullptr);Fc2.Begin(Between);Fc2.Tutorial=0;
   Check(!Fc2.CaseFile().Get(0)->bOpen&&Fc2.Work[FcI].bAudioReviewed&&Fc2.Work[FcI].Stage==EJobStage::Recorded,TEXT("First case: a Continue between RETAIN and the registration opens no case either"));
   Fc2.Selected=FcI;
   Check(Fc2.RegisterComparison(TEXT("unresolved"))&&Fc2.Work[FcI].Stage==EJobStage::Registered&&Fc2.CaseFile().Get(0)->bOpen&&Fc2.CaseStep(0).Step==ECaseStep::ChooseTest
    &&Fc2.FeedbackText().StartsWith(TEXT("CASE OPENED: THE REPEATED INTERVAL"))&&Fc2.CaseFile().Get(0)->RefDay==1&&FMath::IsNearlyEqual(Fc2.CaseFile().Get(0)->RefClock,21.5*3600.),TEXT("First case: registering the comparison at OPS opens it, says CASE OPENED, and keeps the original's clock for its SOURCE line"));
   // An older save: the case was opened at RETAIN, the comparison not registered yet.
   auto Older=S47J::Read(S47J::Write(Fc.Save()));
   {const auto List=S47J::Arr(S47J::Obj(Older,TEXT("cases")),TEXT("cases"));if(List.Num()>0){auto O0=List[0]->AsObject();S47J::Set(O0,TEXT("open"),true);S47J::Set(O0,TEXT("openDay"),1);S47J::Set(O0,TEXT("openClock"),21.75*3600.);}}
   FS47FreeformSession Fc3(nullptr);Fc3.Begin(Older);Fc3.Tutorial=0;Fc3.Selected=FcI;
   Check(Fc3.CaseFile().Get(0)->bOpen&&Fc3.RegisterComparison(TEXT("unresolved"))&&Fc3.CaseFile().Get(0)->bOpen&&Fc3.CaseFile().Get(0)->OpenDay==1&&!Fc3.FeedbackText().Contains(TEXT("CASE OPENED")),TEXT("First case: an older save with the case open at RETAIN goes on as before"));
   Fc2.Stop();Fc3.Stop();
  }
  Fc.Stop();
 }
 {
  // TELEX-20261109: a filed case's morning reply prints on the telex and is no paper until it is torn off there: not in the tray, the
  // case's step, the HUD's second line, TONIGHT'S PLAN and the machine say so; it survives a Continue on the machine; torn off it is a
  // document (into the notebook with a running game) and lies in the OPS tray, also after a Continue.
  const int32 ML=FS47FreeformCases::MysteryLine;
  FS47FreeformSession X(nullptr);X.Begin(nullptr);X.Tutorial=0;
  X.RecordEvidence(TEXT("carrier_saro"),X.CaseFile().ClueText(ML,0));
  int32 Filed=0;for(int32 Cause=0;Cause<FS47FreeformCases::CauseCount;++Cause)if(!X.CaseFile().Excluded(ML,Cause)){Filed=Cause;break;}
  Check(X.ConcludeCase(ML,Filed),TEXT("Telex fixture: an assessment filed on a trace"));
  X.TestAdvance(86400.);X.StepCaseReplies();
  const FString Doc=FString::Printf(TEXT("freeform-case%d-reply"),ML+1);
  Check(X.HasFinding(FString::Printf(TEXT("case%d-reply"),ML+1))&&X.CaseFile().Get(ML)->bReplied&&X.OnTelex(Doc)&&!X.InTelexTray(Doc)&&X.TelexTray().IsEmpty()&&!X.TelexReady()&&!X.TearOffTelex(),
   TEXT("Telex: the morning reply comes in on the telex, not straight into the tray, and cannot be torn off while it prints"));
  const FCaseStep XS=X.CaseStep(ML);
  Check(XS.Step==ECaseStep::AwaitReply&&XS.Task.Contains(TEXT("tear off the reply at the telex"))&&X.CaseNextLine(ML).Contains(TEXT("TEAR IT OFF AT THE TELEX"))&&X.AvailableActivities().StartsWith(TEXT("TELEX (control room): A reply is waiting"))
   &&X.TonightPlan().StartsWith(TEXT("TELEX / A REPLY IS WAITING"))&&X.Label(TEXT("telex"))==TEXT("The telex is printing"),TEXT("Telex: CaseStep, the HUD's one line (paper waiting, blocking now: TASKS-TEN change 5), TONIGHT'S PLAN and the machine say a reply is waiting there"));
  if(!X.AvailableActivities().StartsWith(TEXT("TELEX (control room): A reply is waiting")))UE_LOG(LogTemp,Warning,TEXT("S47: telex self-test: the next step reads %s"),*X.AvailableActivities());
  FS47FreeformSession Y(nullptr);Y.Begin(S47J::Read(S47J::Write(X.Save())));Y.Tutorial=0;
  Check(Y.OnTelex(Doc)&&Y.TelexQueue().Num()==1&&Y.TelexQueue()[0].Page==X.TelexQueue()[0].Page&&!Y.InTelexTray(Doc)&&Y.CaseStep(ML).Step==ECaseStep::AwaitReply,TEXT("Telex: a paper on the machine survives Save and Continue, still not in the tray"));
  Y.TelexSheets[0].Feed=TelexSeconds;
  FS47Doc Torn;
  Check(Y.TelexReady()&&Y.Label(TEXT("telex"))==TEXT("Tear off the telex")&&Y.TearOffTelex(&Torn)&&Torn.Id==Doc&&Torn.Kind==TEXT("telex")&&Torn.Page==X.TelexQueue()[0].Page&&!Y.OnTelex(Doc)&&Y.InTelexTray(Doc)
   &&Y.TelexLine().IsEmpty()&&!Y.AvailableActivities().StartsWith(TEXT("TELEX"))&&Y.CaseStep(ML).Step==ECaseStep::Closed,TEXT("Telex: torn off, the reply is a document for the notebook and lies in the OPS tray; the case has nothing left"));
  FS47FreeformSession Z(nullptr);Z.Begin(S47J::Read(S47J::Write(Y.Save())));
  Check(Z.InTelexTray(Doc)&&Z.TelexQueue().IsEmpty()&&Z.TelexTray().Last().Page==Torn.Page,TEXT("Telex: the torn-off paper is still in the tray after Continue"));
  Check(Z.QueueTelex(TEXT("telex-selftest-1"),TEXT("Test"),TEXT("TELEX"),TEXT("PAGE"))&&!Z.QueueTelex(TEXT("telex-selftest-1"),TEXT("Test"),TEXT("TELEX"),TEXT("PAGE"))&&!Z.QueueTelex(Doc,TEXT("Test"),TEXT("TELEX"),TEXT("PAGE")),
   TEXT("Telex: a paper prints once, and one already torn off is not printed again"));
  {FS47FreeformSession TxOld(nullptr);TxOld.Begin(S47J::Read(TEXT("{\"v\":1}")));Check(TxOld.TelexQueue().IsEmpty()&&TxOld.TelexTray().IsEmpty()&&!TxOld.Save()->HasField(TEXT("telex")),TEXT("Telex: an older save has nothing on the machine and writes nothing"));TxOld.Stop();}
  X.Stop();Y.Stop();Z.Stop();
 }
 {
  // The first pulsar has the extra ticks; nothing else does by this rule; an older save with one recorded is left alone.
  FS47FreeformSession Pulse(nullptr);Pulse.Begin(nullptr);Pulse.TestFirstPulse();Pulse.Tutorial=0;
  auto& First=Pulse.Work[1];
  Check(First.Source==1&&First.Slot==1&&Pulse.FirstPulseDue(First)&&!Pulse.FirstPulseDue(Pulse.Work[0]),TEXT("Discrepancy: the first pulsar recording is the one that has it"));
  First.Stage=EJobStage::Recorded;Pulse.AddDay(2);
  const auto* Later=Pulse.Work.FindByPredicate([](const FJob& J){return J.Day==2&&J.Source==1;});
  Check(Later&&!Pulse.FirstPulseDue(*Later),TEXT("Discrepancy: never a second time by this rule, nor in a save that has recorded one"));
  FS47FreeformSession Dev(nullptr);Dev.Begin(nullptr);Dev.Tutorial=0;Check(!Dev.FirstPulseDue(Dev.Work[1]),TEXT("Discrepancy: developer fixtures keep their own"));
  Pulse.Stop();Dev.Stop();
 }
 {
  // LOOP-ANOMALY-ANY-CARD-20261113 (→ M7): before day 15 only the pulsar's card carries the out-there take; from day 15 the day's carrier
  // by the seed (the pulsar or the third card, never the pointing cross), the other once the carrier is past without it; one a night.
  bool bEarly=true,bSame=true,bBoth1=false,bBoth2=false;
  for(int32 Sd=1;Sd<=200;++Sd)for(int32 Day=1;Day<=40;++Day)
  {
   const int32 C=OddCarrier(Sd,Day);bSame&=C==OddCarrier(Sd,Day)&&(C==1||C==2);
   if(Day<AnyCardFrom)bEarly&=C==1;else{bBoth1|=C==1;bBoth2|=C==2;}
  }
  Check(bEarly&&bSame&&bBoth1&&bBoth2&&AnyCardFrom==15,TEXT("LOOP-ANOMALY-ANY-CARD: the carrier is the pulsar before day 15, then the pulsar or the third card by the seed and day, the same every time"));
  FS47FreeformSession Odd(nullptr);Odd.Begin(nullptr);Odd.AddDay(14);Odd.AddDay(15);
  const auto Find=[&Odd](int32 Day,int32 Slot){return Odd.Work.IndexOfByPredicate([Day,Slot](const FJob& X){return X.Day==Day&&X.Slot==Slot&&X.bGenerated&&X.CaseTest<0;});};
  const int32 E0=Find(14,0),E1=Find(14,1),E2=Find(14,2),L0=Find(15,0),L1=Find(15,1),L2=Find(15,2);
  if(E0<0||E1<0||E2<0||L0<0||L1<0||L2<0)Check(false,TEXT("LOOP-ANOMALY-ANY-CARD: days 14 and 15 have their three cards"));
  else
  {
   const int32 Carrier=OddCarrier(Odd.JobSeed,15),Other=Carrier==1?2:1;const int32 LC=Carrier==1?L1:L2,LO=Carrier==1?L2:L1;
   const bool bDay14=!Odd.OddCarries(Odd.Work[E0])&&Odd.OddCarries(Odd.Work[E1])&&!Odd.OddCarries(Odd.Work[E2]);
   const bool bFirst=Odd.OddCarries(Odd.Work[LC])&&!Odd.OddCarries(Odd.Work[LO])&&!Odd.OddCarries(Odd.Work[L0]);
   // the carrier's card worked plainly: the other may take it now
   Odd.Work[LC].Stage=EJobStage::Filed;Odd.Work[LC].bAnomaly=false;
   const bool bThen=Odd.OddCarries(Odd.Work[LO])&&!Odd.OddCarries(Odd.Work[L0]);
   // one a night: a take tonight already odd from what is out there (recorded tonight, or decided while pointed) shuts every other card
   Odd.Work[LC].bAnomaly=true;Odd.Work[LC].ReferenceClock=FMath::Max(1.,Odd.Clock());
   const bool bShutRecorded=!Odd.OddCarries(Odd.Work[LO]);
   Odd.Work[LC].bAnomaly=false;Odd.Work[LC].Stage=EJobStage::Pointed;Odd.Work[LC].TakeDecided=1;Odd.Work[LC].bTakeOut=true;
   const bool bShutDecided=!Odd.OddCarries(Odd.Work[LO])&&Odd.OddCarries(Odd.Work[LC]);
   Check(bDay14&&bFirst&&bThen&&bShutRecorded&&bShutDecided&&Other!=Carrier,
    TEXT("LOOP-ANOMALY-ANY-CARD: day 14 only card 2; day 15 the seed's carrier first, the other once it is past plainly, never card 1, and at most one a night"));
  }
  Odd.Stop();
 }
 {
  // LOOP-CAREFUL-PAYS-20261113 (→ M3, M2): tight marks are close round the ticks; a loose pair still covers them but is not tight.
  Check(MarksTight(9.,13.)&&MarksTight(9.3,12.6)&&MarksTight(8.9,13.2)&&!MarksTight(8.,13.5)&&!MarksTight(9.5,13.)&&!MarksTight(-1,-1)&&MarkCoversTicks(8.,13.5),
   TEXT("LOOP-CAREFUL-PAYS: marks are tight with at most a second to spare round the extra ticks; looser marks still pass RETAIN"));
 }
 {
  // The evening's cards, from the planner's own rules, for every source and slot.
  FS47FreeformSession Eve(nullptr);Eve.Begin(nullptr);Eve.TestAdvance(86400);
  TArray<const FJob*> Day2;for(const FJob& J:Eve.Work)if(J.Day==2)Day2.Add(&J);
  // LOOP-NIGHT-WATCH-20261113: the two early cards come up at 19:00 and about 19:30, the night watch between 01:15 and 01:45 (shift clock)
  bool bAll=Day2.Num()==3,bRange=Day2.Num()==3;
  for(const auto* J:Day2)bAll&=Eve.EveningGated(*J);
  if(bRange)bRange=EveningUp(*Day2[0])==EveningOpens&&EveningUp(*Day2[1])>=19*3600.+20*60.&&EveningUp(*Day2[1])<=19*3600.+40*60.&&!NightWatch(*Day2[0])&&!NightWatch(*Day2[1])
   &&NightWatch(*Day2[2])&&EveningUp(*Day2[2])>=25*3600.+15*60.&&EveningUp(*Day2[2])<=25*3600.+45*60.&&EveningShut(*Day2[2])==NightWatchCloses&&EveningShut(*Day2[0])==EveningCloses;
  Check(bAll&&bRange,TEXT("Day: from day two all three cards wait; the first is up at 19:00, the second between 19:20 and 19:40, the night watch between 01:15 and 01:45 after midnight (LOOP-NIGHT-WATCH)"));
  Check(!Eve.EveningGated(Eve.Work[0]),TEXT("Day: the first day's own cards are not held back"));
  double UpAt=0;const FJob* Soonest=Eve.NextEveningCard(UpAt);
  Check(Soonest&&UpAt==86400+EveningUp(*Soonest)&&UpAt>Eve.Clock(),TEXT("Day: OPS can name the soonest source and when"));
  Eve.TestAdvance(FMath::Fmod(86400.*2+22*3600.-Eve.Clock(),86400.));
  Check(!Eve.EveningGated(*Day2[0])&&!Eve.EveningGated(*Day2[1])&&Eve.EveningGated(*Day2[2]),TEXT("Day: at ten in the evening the two early sources are up, the night watch not yet"));
  {
   // TASKS-TEN-20261113 (change 4): one count everywhere: the cards up now, the night watch not among them
   int32 DayOne=0;for(const FJob& J:Eve.Work)DayOne+=J.Day==1&&J.Stage!=EJobStage::Filed&&!J.bDuplicateRejected&&J.CaseTest<0?1:0;
   FString UpCard;Eve.WorkActivities(&UpCard);
   Check(Eve.CardsUp()==2+DayOne&&UpCard.StartsWith(FString::Printf(TEXT("OPS // %d JOB CARD"),Eve.CardsUp()))&&Eve.PickRow==TEXT("cards"),
    TEXT("TASKS-TEN (change 4): at ten the cards up are counted once (CardsUp, the HUD, the map's OPS), the night watch not up is not counted"));
  }
  {
   double WatchAt=0;const FJob* Next=Eve.NextEveningCard(WatchAt);
   Check(Next==Day2[2]&&WatchAt==86400.+EveningUp(*Day2[2])&&FS47FreeformSession::NightOf(WatchAt)==2,TEXT("Day: at ten OPS names the night watch, after midnight on shift day 2"));
  }
  Eve.TestAdvance(2*3600.);
  Check(Eve.EveningGated(*Day2[0]),TEXT("Day: a card not begun by half past eleven waits for the next evening"));
  auto* Begun=const_cast<FJob*>(Day2[0]);Begun->Stage=EJobStage::Pointed;
  Check(!Eve.EveningGated(*Begun),TEXT("Day: a card already begun runs out whenever it was begun"));
  // LOOP-NIGHT-WATCH-20261113: the night watch opens after midnight on its own shift day, pays as on its day, and waits from 04:00.
  Eve.TestAdvance(1.75*3600.+5*60.);
  Check(!Eve.EveningGated(*Day2[2])&&Eve.Day()==Day2[2]->Day&&FS47FreeformSession::NightOf(Eve.Clock())==2&&FMath::Max(0,Eve.Day()-Day2[2]->Day)==0,
   TEXT("Day: at 01:50 the night watch is up, on shift day 2: its day, full pay (LOOP-NIGHT-WATCH)"));
  Eve.TestAdvance(2.25*3600.);
  Check(Eve.EveningGated(*Day2[2])&&Eve.Day()==2,TEXT("Day: a night watch not begun by 04:00 waits for the next night"));
  Eve.Stop();
  // LOOP-NIGHT-WATCH-20261113: OPS's next step through an ordinary second night, the two early cards in by 20:30: once with a working
  // array (as an older save has it, no station goal), once with the repair parts in hand (Tom 13 November: «nattevakta gå foran»: the
  // night watch's line leads and the repair is said after it; at 02:42 the rest line is not hidden behind the goal's daylight wait).
  // (money enough that the line is not the one about stores)
  for(const bool bPartsWalk:{false,true})
  {
   auto WalkSave=[]{FS47FreeformSession W0(nullptr);W0.Begin(nullptr);auto Sv=S47J::Read(S47J::Write(W0.Save()));W0.Stop();return Sv;}();
   if(const auto Fd=S47J::Obj(WalkSave,TEXT("field"))){FS47ArrayWear WalkWear;if(bPartsWalk)WalkWear.Start();S47J::Set(Fd,TEXT("arrayWear"),WalkWear.Save());}
   FS47FreeformSession Walk(nullptr);Walk.Begin(WalkSave);Walk.Tutorial=0;Walk.ServiceGuide=0;Walk.Cash=50;Walk.Loan=0;Walk.TestLiveRules();
   const auto* WalkBranch=Walk.Field&&Walk.Field->ArrayWear().bActive?Walk.Field->ArrayWear().NextBranch():nullptr;
   if(!bPartsWalk)Check(!Walk.Field||Walk.StationGoalLine().IsEmpty(),TEXT("Next step on night 2: the walk's fixture has a working array (no station goal)"));
   else Check(WalkBranch&&Walk.Field->BranchNeeds(*WalkBranch).IsEmpty()&&!Walk.StationGoalLine().IsEmpty(),TEXT("Next step on night 2: the second walk's fixture has a worn array and the repair parts in hand"));
   for(FJob& J:Walk.Work)J.Stage=EJobStage::Filed;
   Walk.TestAdvance(86400);
   FJob* Watch=nullptr;for(FJob& J:Walk.Work)if(J.Day==2){if(J.Slot<2)J.Stage=EJobStage::Filed;else Watch=&J;}
   bool bGap=Watch!=nullptr,bNoRest=true,bGoalAfter=true,bOneArbiter=true;
   // TASKS-TEN-20261113 (change 2): one arbiter: at every hour the note's Next is the HUD's own row, and the workstations read the same
   // Objective the HUD's card is made from
   const auto Agree=[&Walk]()
   {
    S47TonightNote::FIn NoteIn;NoteIn.bFreeform=true;NoteIn.Clock=Walk.Clock();NoteIn.Next=Walk.HudObjective();NoteIn.NextRow=Walk.ObjectiveRow();
    for(const FS47Task& T:Walk.Tasks())NoteIn.Tasks.Add({T.Text,T.bDone});
    const S47TonightNote::FNote Plan=S47TonightNote::Build(NoteIn);
    const S47TonightNote::FLine* NextLine=Plan.Lines.FindByPredicate([](const S47TonightNote::FLine& L){return L.Mark==S47TonightNote::EMark::Next;});
    const bool bRow=NextLine&&(NextLine->Key==S47TonightNote::KeyOf(NoteIn.Next)||NextLine->Key==NoteIn.NextRow||NextLine->Key==S47TonightNote::KeyOf(NoteIn.NextRow));
    const FString Obj=Walk.Objective();
    return bRow&&!TerminalNextLine(Obj,0).IsEmpty()&&(Walk.HudObjective()==S47HudGauges::CardLine(Obj)||Walk.AvailableActivities(nullptr)==Obj||Walk.NextAction()==Obj);
   };
   for(const double H:{20.5,22.,23.5,24.5,25.1})
   {
    Walk.TestAdvance(FMath::Max(0.,86400.+H*3600.-Walk.Clock()));
    bOneArbiter&=Agree();
    FString WalkCard;const FString Line=Walk.WorkActivities(&WalkCard);
    bGap&=Line.StartsWith(TEXT("NIGHT WATCH / "))&&Line.Contains(TEXT("TONIGHT'S TWO CARDS ARE IN."))&&Line.Contains(TEXT("TILL THEN THE TIME IS YOURS"))&&TerminalNextLine(Line,2).Contains(TEXT("IN VIEW FROM 01:"))
     &&WalkCard.StartsWith(TEXT("NIGHT WATCH // "))&&WalkCard.Contains(TEXT("YOUR OWN TIME"));
    bNoRest&=!Line.Contains(TEXT("REST"));
    // the repair after the watch's line, never dropped: until one in the morning to be done, after it waiting for daylight; in the
    // notebook the night watch's card before the goal
    if(bPartsWalk)
    {
     const TArray<FS47Task> WalkTasks=Walk.Tasks();const FString WalkGoal=Walk.StationGoalLine();const FString WatchLine=Watch?Walk.JobLine(*Watch):FString();
     const int32 GoalRow=WalkTasks.IndexOfByPredicate([&WalkGoal](const FS47Task& T){return T.Text==WalkGoal;});
     const int32 WatchRow=WalkTasks.IndexOfByPredicate([&WatchLine](const FS47Task& T){return T.Text==WatchLine;});
     // (TASKS-TEN-20261113, change 3: from one in the morning the goal only waits for daylight: no row of its own, only the half-line)
     bGoalAfter&=Line.Find(TEXT("STATION GOAL / "))>Line.Find(TEXT("BE BACK IN THE CONTROL ROOM BY"))&&Line.Contains(H<25.?TEXT("REPAIR THE"):TEXT("WAITS FOR DAYLIGHT"))
      &&WatchRow>=0&&(H<25.?GoalRow>WatchRow:GoalRow==INDEX_NONE);
    }
   }
   Walk.TestAdvance(FMath::Max(0.,86400.+25.8*3600.-Walk.Clock()));
   const bool bUp=Walk.WorkActivities(nullptr).StartsWith(TEXT("OPS (corner PC), DAILY WORK"));
   bOneArbiter&=Agree();
   if(Watch)Watch->Stage=EJobStage::Filed;
   Walk.TestAdvance(FMath::Max(0.,86400.+26.2*3600.-Walk.Clock()));
   const FString Done=Walk.WorkActivities(nullptr);
   bOneArbiter&=Agree();
   Walk.TestAdvance(FMath::Max(0.,86400.+26.7*3600.-Walk.Clock()));
   FString LaterCard;const FString Later=Walk.WorkActivities(&LaterCard);
   bOneArbiter&=Agree();
   Check(bOneArbiter,TEXT("TASKS-TEN (change 2): one arbiter across night 2 (20:30 to 02:42): the note's Next is the HUD's own row at every hour, and the workstations read the Objective the HUD's card is made from"));
   if(bPartsWalk)bGoalAfter&=Done.StartsWith(TEXT("OPS: TODAY'S PAID WORK IS DONE."))&&Later.StartsWith(TEXT("OPS: TODAY'S PAID WORK IS DONE."))&&LaterCard.Contains(TEXT("OR REST"))
     &&Done.Contains(TEXT("WAITS FOR DAYLIGHT"))&&Later.Find(TEXT("STATION GOAL / "))>Later.Find(TEXT("OR REST."));
   Check(bGap&&bNoRest&&bUp&&Done.Contains(TEXT("PAID WORK IS DONE"))&&!Done.Contains(TEXT("REST"))&&Later.Contains(TEXT("OR REST")),
    bPartsWalk?TEXT("Next step on night 2, the repair parts in hand: 20:30 to 01:15 the night watch's hour and his own time, never rest; the watch up at 01:48; done at 02:12 no rest yet; at 02:42 the old line, or rest (LOOP-NIGHT-WATCH)")
     :TEXT("Next step on night 2: 20:30 to 01:15 the night watch's hour and his own time, never rest; the watch up at 01:48; done at 02:12 no rest yet; at 02:42 the old line, or rest (LOOP-NIGHT-WATCH)"));
   if(bPartsWalk)Check(bGoalAfter,TEXT("Next step on night 2, the repair parts in hand: the night watch leads from 20:30 and the repair is said after it (in the notebook too); at 02:12 and 02:42 the paid-work line, or rest, before the goal's daylight wait (Tom: nattevakta gå foran)"));
   Walk.Stop();
  }
  // TASKS-TEN-20261113 (change 7): what Dale asks on the walkie is kept, in his own words, under FOLLOW-UP, as the ladder's deadline;
  // it survives a Continue, is ticked off when done and goes at 08:00
  {
   FS47FreeformSession Ask(nullptr);Ask.Begin(nullptr);Ask.Tutorial=0;Ask.TestAdvance(FMath::Max(0.,86400.+22*3600.-Ask.Clock()));
   for(FJob& J:Ask.Work)J.Stage=EJobStage::Filed;
   Ask.DaleSaid(TEXT("Get outside and look south of the station, over the salt lick. The sky there is lit like a ballpark. Go and look, before it stops."));
   Ask.DaleSaid(TEXT("Out at the salt lick, a mile and a half from the station. Put it on your map. Not tonight: by daylight, with the walkie, and you look at the ground before you walk on it."));
   Ask.DaleSaid(TEXT("And write it in the night log while you still have it: the hour, the bearing, what you saw with your own eyes. Ward wants a case carried through, with times in it. Not stories."));
   Ask.DaleSaid(TEXT("Put it on your map.")); // the same ask again tonight is one row
   const TArray<FString> Rows=Ask.DaleAskRows();
   const bool bRows=Rows.Num()==3&&Rows[0]==TEXT("DALE ASKED / OUTSIDE: Get outside and look south of the station, over the salt lick.")
    &&Rows[1]==TEXT("DALE ASKED / MAP: Put it on your map: out at the salt lick, a mile and a half from the station.")
    &&Rows[2].StartsWith(TEXT("DALE ASKED / NIGHT LOG: Write it in the night log"));
   const TArray<FS47Task> AskTasks=Ask.Tasks();
   S47TonightNote::FIn AskIn;AskIn.bFreeform=true;AskIn.Clock=Ask.Clock();for(const FS47Task& T:AskTasks)AskIn.Tasks.Add({T.Text,T.bDone});
   const S47TonightNote::FNote AskNote=S47TonightNote::Gather(AskIn,true);
   const bool bFollow=AskNote.Lines.FilterByPredicate([](const S47TonightNote::FLine& L){return L.Source.StartsWith(TEXT("DALE ASKED / "))&&L.Kind==1&&S47TonightNote::TagText(L)==TEXT("THE LAND / until 08:00 / unknown");}).Num()==3;
   const bool bDue=Ask.DeadlineLine()==Rows[0];
   FS47FreeformSession Back(nullptr);Back.Begin(S47J::Read(S47J::Write(Ask.Save())));Back.TestAdvance(FMath::Max(0.,Ask.Clock()-Back.Clock()));
   const bool bKept=Back.DaleAskRows()==Rows;
   ++S47TonightNote::MapLooks();Back.StepDaleAsks();
   const bool bTicked=Back.DaleAskRows().Num()==2&&Back.Tasks().ContainsByPredicate([&Rows](const FS47Task& T){return T.Text==Rows[1]&&T.bDone;});
   Back.TestAdvance(FMath::Max(0.,2*86400.+8.5*3600.-Back.Clock()));Back.StepDaleAsks();
   const bool bGone=Back.DaleAskRows().IsEmpty()&&!Back.Tasks().ContainsByPredicate([](const FS47Task& T){return T.Text.StartsWith(TEXT("DALE ASKED / "));});
   Check(bRows&&bFollow&&bDue&&bKept&&bTicked&&bGone,
    TEXT("TASKS-TEN (change 7): Dale's asks (outside, the map, the night log) are kept as DALE ASKED rows under FOLLOW-UP in his own words, the ladder's deadline, kept over a Continue, ticked off when done, gone at 08:00"));
   Ask.Stop();Back.Stop();
  }
  // TASKS-TEN-20261113 (change 10): a pinned row is the HUD's step (and the note's Next); a sheet waiting on the printer breaks through;
  // the pin is kept over a Continue, marked on the note, and clears at 08:00
  {
   FS47FreeformSession Pn(nullptr);Pn.Begin(nullptr);Pn.Tutorial=0;Pn.TestAdvance(FMath::Max(0.,86400.+21*3600.-Pn.Clock()));
   Pn.DaleSaid(TEXT("And write it in the night log while you still have it: the hour, the bearing, what you saw with your own eyes."));
   const FString Row=Pn.DaleAskRows().Num()?Pn.DaleAskRows()[0]:FString();
   Pn.WorkActivities(nullptr);const FString Before=Pn.PickRow;
   Pn.PinRow(S47TonightNote::KeyOf(Row),Row);Pn.StepPin();
   FString PinCard;Pn.WorkActivities(&PinCard);
   const bool bShows=!Row.IsEmpty()&&Before!=Row&&Pn.PickRow==Row&&Pn.PinnedKey()==S47TonightNote::KeyOf(Row)&&PinCard.Contains(TEXT("NIGHT LOG"));
   S47TonightNote::FIn PinIn;PinIn.bFreeform=true;PinIn.Clock=Pn.Clock();PinIn.Next=Pn.HudObjective();PinIn.NextRow=Pn.ObjectiveRow();PinIn.Pinned=Pn.PinnedKey();
   for(const FS47Task& T:Pn.Tasks())PinIn.Tasks.Add({T.Text,T.bDone});
   const S47TonightNote::FNote PinNote=S47TonightNote::Build(PinIn);
   const bool bMarked=PinNote.Lines.ContainsByPredicate([&Row](const S47TonightNote::FLine& L){return L.Source==Row&&L.bPinned&&L.Mark==S47TonightNote::EMark::Next;});
   FResultSheet Sheet;Sheet.Line=0;Sheet.Test=0;Sheet.Job=TEXT("pin-selftest");Pn.CaseSheets.Add(Sheet);
   const bool bBreaks=Pn.WorkActivities(nullptr).StartsWith(TEXT("PRINTER: COLLECT THE RESULT SHEET"));
   Pn.CaseSheets.Reset();
   FS47FreeformSession PinBack(nullptr);PinBack.Begin(S47J::Read(S47J::Write(Pn.Save())));PinBack.TestAdvance(FMath::Max(0.,Pn.Clock()-PinBack.Clock()));
   const bool bKept=PinBack.PinnedKey()==Pn.PinnedKey();
   PinBack.TestAdvance(FMath::Max(0.,2*86400.+8.25*3600.-PinBack.Clock()));PinBack.StepPin();
   Check(bShows&&bMarked&&bBreaks&&bKept&&PinBack.PinnedKey().IsEmpty(),
    TEXT("TASKS-TEN (change 10): a pinned row is the HUD's step and the note's starred Next; paper on the printer breaks through; kept over a Continue; cleared at 08:00"));
   Pn.Stop();PinBack.Stop();
  }
  // The next-step line across an ordinary night: no rest before the night watch (19:00 to 02:30), unless worn out; from 02:30 it may.
  {
   bool bNone=true;for(const double H:{19.,20.5,22.,23.5,24.,25.,26.,26.4})bNone&=!RestSuggestedAt(86400.+H*3600.,2,true,false);
   Check(bNone&&RestSuggestedAt(86400.+26.6*3600.,2,true,false)&&RestSuggestedAt(86400.+23*3600.,2,true,true)&&RestSuggestedAt(86400.+14*3600.,2,true,false)&&RestSuggestedAt(22*3600.,1,true,false)&&RestSuggestedAt(86400.+22*3600.,2,false,false),
    TEXT("Rest: OPS says no rest from 19:00 to 02:30 from day two, unless worn out; from 02:30, by day, on the first night and in a developer run as before (LOOP-NIGHT-WATCH)"));
  }
  FS47FreeformSession Hours(nullptr);Hours.Begin(nullptr);
  const double Night5=4*86400.+24*3600.;
  Check(Hours.TheirHoursNow(Night5+2.5*3600.)&&!Hours.TheirHoursNow(Night5+1.5*3600.)&&!Hours.QuietNight(5),TEXT("Hours: a developer run keeps the fixed two in the morning"));
  Hours.TestLiveRules();
  int32 Started=0,Calm=0;bool bNever=true;
  for(int32 N=2;N<=40;++N)
  {
   const double Mid=(N-1)*86400.+24*3600.;
   Check(FS47FreeformSession::NightOf(Mid+3600.)==N,TEXT("Hours: one in the morning belongs to the evening before"));
   bNever&=!Hours.TheirHoursNow(Mid-3600.)&&!Hours.TheirHoursNow(Mid+.5*3600.)&&!Hours.TheirHoursNow(Mid+7*3600.);
   Started+=Hours.NightStarted(Mid+3.01*3600.)?1:0;Calm+=Hours.QuietNight(N)?1:0;
   if(Hours.QuietNight(N))bNever&=!Hours.TheirHoursNow(Mid+3.5*3600.);
  }
  Check(bNever&&Started==39&&Calm>0,TEXT("Hours: no body before one in the morning, none after six, none on a calm night, and every night has begun by three"));
  const int32 Before=Hours.TimesTaken();Hours.RecordEvidence(TEXT("taken-3-100"),TEXT("gap"));
  Check(Hours.TimesTaken()==Before+1&&Hours.BodyStartHour(9)<=FS47NightRhythm::StartHour(Hours.JobSeed,9,0),TEXT("Hours: they come earlier to someone they have taken"));
  FS47FreeformSession Reread(nullptr);Reread.Begin(S47J::Read(S47J::Write(Hours.Save())));Check(Reread.TimesTaken()==Before+1,TEXT("Hours: that follows from the findings after a Continue, with nothing new saved"));
  Hours.Stop();Reread.Stop();
 }
 {
  Check(PayAfterDays(0)==14&&PayAfterDays(1)==14&&PayAfterDays(2)==10&&PayAfterDays(3)==7&&PayAfterDays(4)==4&&PayAfterDays(30)==4,TEXT("M7: a card's base pays in full ($14) on its day and the next, then less, and never nothing"));
  Check(PayAfterDays(0)+OnTimeBonus+CleanBonus==19&&OnTimeBonus+CleanBonus+ControlBonus>=4&&OnTimeBonus+CleanBonus+ControlBonus<=6&&PayAfterDays(0)>=12&&PayAfterDays(0)<=14,TEXT("M7: base $12 to $14 and $4 to $6 for good work: a good card $19, against the old flat $20"));
  Check(StandbyFor(0)==0&&StandbyFor(3600)==1&&StandbyFor(9*3600.)==3&&StandbyFor(11*3600.)==4&&StandbyFor(40*3600.)==4,TEXT("Time: waiting costs a dollar per three hours, at most four"));
  FS47FreeformSession Econ(nullptr);Econ.Begin(nullptr);
  Check(Econ.JobPay(Econ.Work[0])==20,TEXT("Time: developer runs keep the flat rate"));
  Econ.TestLiveRules();Econ.TestAdvance(2*86400);
  Check(Econ.JobPay(Econ.Work[0])==10&&Econ.JobBonus(Econ.Work[0])==0,TEXT("M7: a card two days old, not yet recorded, pays its $10 base and no bonus"));
  Econ.Work[0].bDocked=true;Check(Econ.JobPay(Econ.Work[0])==5,TEXT("Time: docked, half that"));Econ.Work[0].bDocked=false;
  {
   // M7: quality pays better than quantity. The same card on its day: clean, on time, the anomaly controlled; or a recording nobody can read.
   FS47FreeformSession Q(nullptr);Q.Begin(nullptr);Q.TestLiveRules();FJob& C=Q.Work[0];C.Day=Q.Day();C.Stage=EJobStage::Recorded;C.RecordingSoundVersion=2;C.RecordingQuality=.8;
   FString Why;const int32 Good=Q.JobPay(C);Q.JobBonus(C,&Why);
   C.bAnomaly=C.bControl=true;const int32 Controlled=Q.JobPay(C);C.bAnomaly=C.bControl=false;
   C.RecordingQuality=.2;const int32 Muddy=Q.JobPay(C);
   Q.TestAdvance(3*86400);const int32 LateMuddy=Q.JobPay(Q.Work[0]);
   Check(Why.Contains(TEXT("for being on time and a clean recording"))&&JoinAnd({TEXT("a"),TEXT("b"),TEXT("c")})==TEXT("a, b and c"),TEXT("CRIT6: the pay line's reasons in plain English: for being on time and a clean recording"));
   Check(DeskText(true,false,95,1,23.5).Contains(TEXT("took 95 minutes"))&&DeskText(true,false,95,1,23.5).Contains(TEXT("6.5 hours left to 06:00"))&&!DeskText(true,false,300,2,1).Contains(TEXT("300"))&&DeskText(true,true,700,1,1).Contains(TEXT("earlier night"))&&!DeskText(true,true,700,1,1).Contains(TEXT("minutes"))&&DeskText(true,false,30,1,7).Contains(TEXT("the night is over"))&&!DeskText(true,false,30,1,7).Contains(TEXT("0.0 hours"))&&!DeskText(false,false,0,1,23).Contains(TEXT("took")),
    TEXT("CRIT6: the desk line: its minutes up to four hours, none from an earlier night or unknown, and after 06:00 the night is over"));
   Check(Good==19&&Controlled==20&&Muddy==16&&LateMuddy==7&&Why.Contains(TEXT("clean recording")),*FString::Printf(TEXT("M7: good card $%d, anomaly controlled $%d, muddy on time $%d, muddy three days late $%d"),Good,Controlled,Muddy,LateMuddy));
   // two good cards are worth more than three muddy late ones
   Check(2*Good>3*LateMuddy,TEXT("M7: two good cards pay more than three muddy late ones"));
   {
    // LOOP-CAREFUL-PAYS-20261113 (→ M3, M2): the clean bonus only for a fix by hand; the assist's withholds it and says so; the rest stands
    FS47FreeformSession Fix(nullptr);Fix.Begin(nullptr);Fix.TestLiveRules();FJob& K=Fix.Work[0];K.Day=Fix.Day();K.Stage=EJobStage::Recorded;K.RecordingSoundVersion=2;K.RecordingQuality=.8;
    K.SkyBy=1;FString HandWhy;const int32 Hand=Fix.JobPay(K);Fix.JobBonus(K,&HandWhy);
    K.SkyBy=0;const int32 Unknown=Fix.JobPay(K);
    K.SkyBy=2;FString AssistWhy;const int32 Assisted=Fix.JobPay(K);const int32 AssistBonus=Fix.JobBonus(K,&AssistWhy);
    K.RecordingQuality=.2;FString MuddyWhy;const int32 MuddyAssisted=Fix.JobPay(K);Fix.JobBonus(K,&MuddyWhy);
    Check(Hand==19&&Unknown==19&&Assisted==Hand-CleanBonus&&AssistBonus==OnTimeBonus&&AssistWhy.Contains(TEXT("no clean bonus, the sky fix was by assist"))&&!HandWhy.Contains(TEXT("assist"))
     &&MuddyAssisted==Assisted&&!MuddyWhy.Contains(TEXT("assist"))&&CleanBonusFor(K)==false,
     *FString::Printf(TEXT("LOOP-CAREFUL-PAYS: a fix by hand $%d (an older save's, not known, $%d), by assist $%d with no clean bonus and a line that says so"),Hand,Unknown,Assisted));
    // saved from the lock on; a job back at the start has none
    K.SkyBy=2;K.bOwnMarks=true;FS47FreeformSession FixAgain(nullptr);FixAgain.Begin(S47J::Read(S47J::Write(Fix.Save())));
    const bool bKept=FixAgain.Work.Num()==Fix.Work.Num()&&FixAgain.Work[0].SkyBy==2&&FixAgain.Work[0].bOwnMarks;
    K.Stage=EJobStage::New;FS47FreeformSession FixNew(nullptr);FixNew.Begin(S47J::Read(S47J::Write(Fix.Save())));
    Check(bKept&&FixNew.Work[0].SkyBy==0,TEXT("LOOP-CAREFUL-PAYS: how the fix was made and the operator's own marks survive a Continue; a job back at the start has no fix"));
    Fix.Stop();FixAgain.Stop();FixNew.Stop();
   }
   Q.Stop();
  }
  {
   // M7: side money is capped by the week, and the cap comes back the next week.
   FS47FreeformSession Side(nullptr);Side.Begin(nullptr);Side.TestLiveRules();const int32 Was=Side.Budget();
   const int32 A=Side.EarnSide(20,TEXT("test")),B=Side.EarnSide(20,TEXT("test")),C2=Side.EarnSide(5,TEXT("test"));
   Check(A==20&&B==SideCap-20&&C2==0&&Side.Budget()==Was+SideCap&&Side.SideLeftThisWeek()==0,*FString::Printf(TEXT("M7: field-work money is capped at $%d a week (paid %d, %d, %d)"),int32(SideCap),A,B,C2));
   FS47FreeformSession SideAgain(nullptr);SideAgain.Begin(S47J::Read(S47J::Write(Side.Save())));SideAgain.TestLiveRules();
   Check(SideAgain.SideLeftThisWeek()==0&&SideAgain.EarnSide(10,TEXT("test"))==0,TEXT("M7: the week's side money survives a Continue"));
   SideAgain.TestAdvance(7*86400);Check(SideAgain.SideLeftThisWeek()==SideCap&&SideAgain.EarnSide(10,TEXT("test"))==10,TEXT("M7: a new week, a new cap"));
   Check(SideCap*4<7*2*PayAfterDays(0),TEXT("M7: a month's capped side money is less than one week of two base cards an evening"));
   {
    // CRIT6-20261109: the pickup's reward, the geiger jar and the stuck rig go through the cap too (FS47Happenings): two jars after it is used, nothing.
    const int32 Before=Side.Budget();const int32 Jar1=Side.EarnSide(10,TEXT("geiger jar")),Jar2=Side.EarnSide(10,TEXT("geiger jar")),Reward=Side.EarnSide(20,TEXT("pickup reward")),Rig=Side.EarnSide(15,TEXT("stuck rig"));
    Check(Jar1==0&&Jar2==0&&Reward==0&&Rig==0&&Side.Budget()==Before,TEXT("CRIT6: past the week's cap two geiger jars, the pickup's reward and the stuck rig pay nothing"));
   }
   Side.Stop();SideAgain.Stop();
  }
  const int32 Start=Econ.Budget(),Week=Econ.ReviewPowerNow()+ReviewFuel;Econ.TestAdvance(5*86400);Econ.StepWeeklyReview();
  Check(Econ.Day()==8&&Econ.Budget()==Start-Week&&Econ.Arrears()==0&&Econ.HasFinding(TEXT("week-review-1"))&&Econ.Debt()==0&&Econ.NextReviewDay()==15,TEXT("Time: the first review falls on day eight, power by the antennas on the line and fuel, on a sheet"));
  Econ.StepWeeklyReview();Check(Econ.Budget()==Start-Week,TEXT("Time: a review is paid once"));
  const int32 Week2=ReviewPowerFor(2,Econ.AvailableDishCount())+ReviewFuel;
  Econ.Cash=10;Econ.TestAdvance(7*86400);Econ.StepWeeklyReview();
  Check(Econ.Budget()==0&&Econ.Arrears()==Week2-10&&Econ.Debt()==0,TEXT("Time: short of money it takes what is there, carries the rest, and never touches the requisition"));
  FS47FreeformSession EconAgain(nullptr);EconAgain.Begin(S47J::Read(S47J::Write(Econ.Save())));
  Check(EconAgain.Arrears()==Week2-10&&EconAgain.NextReviewDay()==22,TEXT("Time: what is carried survives a Continue, and is not charged again"));
  auto OldEcon=Econ.Save();OldEcon->RemoveField(TEXT("reviewWeek"));OldEcon->RemoveField(TEXT("carried"));S47J::Set(OldEcon,TEXT("cash"),60);
  FS47FreeformSession OldGame(nullptr);OldGame.Begin(OldEcon);OldGame.TestLiveRules();OldGame.StepWeeklyReview();
  Check(OldGame.Budget()==60&&OldGame.Arrears()==0,TEXT("Time: an older save on day fifteen is not charged for the weeks behind it"));
  Econ.Stop();EconAgain.Stop();OldGame.Stop();
  // The running costs: a recording nobody can read costs Ward's re-check on the sheet.
  FS47FreeformSession Fees(nullptr);Fees.Begin(nullptr);Fees.TestLiveRules();
  Fees.Work[0].Stage=EJobStage::Filed;Fees.Work[0].CaseTest=-1;Fees.Work[0].Day=1;Fees.Work[0].RecordingSoundVersion=2;Fees.Work[0].RecordingQuality=.1;
  const int32 FeeStart=Fees.Budget();Fees.TestAdvance(7*86400);Fees.StepWeeklyReview();
  Check(Fees.Budget()==FeeStart-ReviewPowerFor(1,Fees.AvailableDishCount())-ReviewFuel-PoorRecordFee&&Fees.FindingText(TEXT("week-review-1")).Contains(TEXT("RE-CHECK")),TEXT("Time: Ward charges for a recording nobody can read, on the sheet, and not much"));
  Check(WearCap+FilmCap+FeeCap+TapeCap+RepairCap+ReviewPowerFor(4,0)+ReviewFuel<=140,TEXT("M7: the worst week (week four, every antenna out) costs at most $140, about half a week of two good cards"));
  Check(ReviewPowerFor(1,0)<ReviewPowerFor(2,0)&&ReviewPowerFor(3,0)<ReviewPowerFor(4,0)&&ReviewPowerFor(1,0)==37&&ReviewPowerFor(4,0)==64,TEXT("M7: the fault tariff goes up with the weeks, $37 to $64 with every antenna out"));
  Check(ReviewPowerFor(1,12)==25&&ReviewPowerFor(1,13)<ReviewPowerFor(1,12)&&ReviewPowerFor(4,26)<ReviewPowerFor(4,25)&&ReviewPowerFor(4,27)==ReviewPowerBase,TEXT("CHAIN-5-20261109: every repaired antenna makes the sheet cheaper, a full array costs the base only"));
  {
   // M7: gear broken in the field goes on the next sheet once, then is cleared
   FS47FreeformSession Br(nullptr);Br.Begin(nullptr);Br.TestLiveRules();Br.BreakGear(TEXT("flashlight"),4);Br.BreakGear(TEXT("flashlight"),40);
   const int32 B0=Br.Budget(),Due=Br.RepairsDue();Br.TestAdvance(7*86400);Br.StepWeeklyReview();
   Check(Due==RepairCap&&Br.Budget()==B0-ReviewPowerFor(1,Br.AvailableDishCount())-ReviewFuel-Due&&Br.RepairsDue()==0&&Br.FindingText(TEXT("week-review-1")).Contains(TEXT("REPAIRS")),TEXT("M7: field repairs go on the next sheet, capped, and only once"));
   Br.Stop();
  }
  Fees.Stop();
  // Being taken, in the session's books.
  FS47FreeformSession Gone(nullptr);Gone.Begin(nullptr);Gone.TestLiveRules();Gone.TestAdvance(86400+17*3600.);
  const double Safe=Gone.Clock();Gone.Work.Last().Stage=EJobStage::Pointed;Gone.TakenAway(5*3600.);const FString Cost=Gone.TakenCosts(Safe);
  const auto* LostCase=Gone.CaseFile().Get(FS47FreeformCases::LostLine);
  Check(LostCase&&LostCase->bOpen&&Cost.Contains(TEXT("half rate"))&&Gone.Work.Last().Stage==EJobStage::New&&Gone.Work.Last().bDocked&&Gone.JobPay(Gone.Work.Last())==8,TEXT("Taken: the case opens, today's cards go in at half rate, and a pointed array has to be pointed again"));
  Gone.LostTimeTrace(1);Check(Gone.HasFinding(TEXT("lost_ground"))&&FS47FreeformCases::LineForFinding(TEXT("lost_clock"))==FS47FreeformCases::LostLine,TEXT("Taken: going back gives the case a trace"));
  // Critic round 5: one trace of the lost hours is not a case carried through; the clock at OPS as well is.
  {const bool bOne=Gone.CaseSupported(FS47FreeformCases::LostLine);FS47FreeformSession Both(nullptr);Both.Begin(S47J::Read(S47J::Write(Gone.Save())));Both.TestLiveRules();Both.LostTimeTrace(0);
   Check(!bOne&&Both.CaseSupported(FS47FreeformCases::LostLine),TEXT("Taken: one trace of the lost hours has nothing behind it for Ward; the OPS clock and the place together have"));Both.Stop();}
  FS47FreeformSession GoneAgain(nullptr);GoneAgain.Begin(S47J::Read(S47J::Write(Gone.Save())));
  Check(GoneAgain.Work.Last().bDocked&&GoneAgain.CaseFile().Get(FS47FreeformCases::LostLine)->bOpen,TEXT("Taken: all of that survives a Continue"));
  Check(GoneAgain.ConcludeCase(FS47FreeformCases::LostLine,1)&&GoneAgain.CaseFile().HasAccess(TEXT("access.lostHours")),TEXT("Taken: an assessment can be filed, and opens what follows"));
  Gone.Stop();GoneAgain.Stop();
 }
 {
  // The stores list: priced, waiting on something real, kept by name.
  FS47FreeformSession Shop(nullptr);Shop.Begin(nullptr);
  Check(!Shop.KitWaits(TEXT("magnetometer")).IsEmpty()&&!Shop.BuyKit(TEXT("magnetometer"))&&Shop.Budget()==120,TEXT("Stores: nothing is sold before what it waits for"));
  Check(!Shop.KitWaits(TEXT("logger2")).IsEmpty()&&!Shop.KitWaits(TEXT("film")).IsEmpty()&&!Shop.KitWaits(TEXT("alarm")).IsEmpty()&&!Shop.BuyKit(TEXT("nonsense")),TEXT("Stores: each waits for its own thing"));
  Shop.RecordEvidence(TEXT("anything"),TEXT("a record"));
  Check(Shop.KitWaits(TEXT("cot")).IsEmpty()&&Shop.BuyKit(TEXT("cot"))&&Shop.HasKit(TEXT("cot"))&&Shop.Budget()==100&&!Shop.BuyKit(TEXT("cot")),TEXT("Stores: paid for once, at the price it says"));
  Check(!Shop.KitWaits(TEXT("magnetometer")).IsEmpty(),TEXT("Stores: a record at the station does not open the magnetometer"));
  Shop.RecordEvidence(TEXT("place_windmill"),TEXT("a place"));
  Check(Shop.KitWaits(TEXT("magnetometer")).IsEmpty()&&Shop.KitPrice(TEXT("magnetometer"))==40,TEXT("Stores: the first place found opens the magnetometer, at full price, without being taken"));
  Shop.Cash=5;Shop.TestGiveKit(TEXT("alarm"));Check(!Shop.BuyKit(TEXT("alarm"))&&Shop.Budget()==5,TEXT("Stores: what is in hand is not sold again"));
  FS47FreeformSession ShopAgain(nullptr);ShopAgain.Begin(S47J::Read(S47J::Write(Shop.Save())));Check(ShopAgain.HasKit(TEXT("cot"))&&ShopAgain.HasKit(TEXT("alarm")),TEXT("Stores: what was bought survives a Continue"));
  auto Oldest=Shop.Save();Oldest->RemoveField(TEXT("kit"));FS47FreeformSession OldShop(nullptr);OldShop.Begin(Oldest);Check(!OldShop.HasKit(TEXT("cot")),TEXT("Stores: an older save has none, and reads without complaint"));
  S47J::Set(Oldest,TEXT("kit"),TArray<FString>{TEXT("cot"),TEXT("flying carpet")});FS47FreeformSession OddShop(nullptr);OddShop.Begin(Oldest);Check(OddShop.HasKit(TEXT("cot"))&&!OddShop.HasKit(TEXT("flying carpet")),TEXT("Stores: nonsense in a save is dropped"));
  Check(FS47FreeformCases::UnavailableReason(2,2,false).Contains(TEXT("unavailable"))&&FS47FreeformCases::UnavailableReason(2,2,true).IsEmpty(),TEXT("Two sites: real once the second logger is in hand"));
  Shop.Stop();ShopAgain.Stop();OldShop.Stop();OddShop.Stop();
  // CHAIN-2-20261109: what the field brings in opens it, money buys it; the week alone opens nothing on the truck.
  FS47FreeformSession Truck(nullptr);Truck.Begin(nullptr);Truck.DayNumber=8;
  Check(!Truck.KitWaits(TEXT("truck_tires")).IsEmpty()&&!Truck.KitWaits(TEXT("truck_rack")).IsEmpty()&&!Truck.KitWaits(TEXT("truck_engine")).IsEmpty(),TEXT("CHAIN-2: on day 8 the truck's upgrades still wait for something from out on the land"));
  Truck.RecordEvidence(TEXT("place_windmill"),TEXT("a place"));
  Check(Truck.KitWaits(TEXT("truck_tires")).IsEmpty()&&Truck.KitWaits(TEXT("truck_rack")).IsEmpty()&&Truck.KitWaits(TEXT("truck_tank")).IsEmpty()&&!Truck.KitWaits(TEXT("truck_engine")).IsEmpty(),TEXT("CHAIN-2: a place found opens the tires, the rack and the tank; the engine waits for two kinds of night record"));
  Truck.RecordEvidence(TEXT("cattle_ring_d7"),TEXT("a ring"));Truck.RecordEvidence(TEXT("carcass_d7"),TEXT("a cow"));
  Check(Truck.KitWaits(TEXT("truck_engine")).IsEmpty()&&!Truck.KitWaits(TEXT("rdf")).IsEmpty(),TEXT("CHAIN-2: two kinds of night record open the engine; the direction finder waits for day 15"));
  Truck.DayNumber=15;Check(!Truck.KitWaits(TEXT("rdf")).IsEmpty(),TEXT("CHAIN-2: on day 15 the direction finder still wants a third kind, or a field time that matched the array"));
  Truck.RecordEvidence(TEXT("minute_yes_F-0012"),TEXT("SAME MINUTE ON ARRAY: YES"));Check(Truck.KitWaits(TEXT("rdf")).IsEmpty(),TEXT("CHAIN-2: a field time that matched the array opens the direction finder"));
  Truck.Stop();
  Check(FieldRecordKey(TEXT("place_windmill"))&&!FieldRecordKey(TEXT("place_windmill_needle"))&&FieldRecordKey(TEXT("minute_no_F-0001"))&&FieldRecordKey(TEXT("carcass_d3"))&&!FieldRecordKey(TEXT("carrier_saro")),TEXT("CHAIN-2: a field record is a place, marks, what the night left out there, or a field time"));
  {
   // CHAIN-2-20261109: the same minute, with the feeder on the county road looked at, is a case with something from out on the land.
   FS47FreeformSession ChainDesk(nullptr);ChainDesk.Begin(nullptr);ChainDesk.TestLiveRules();
   const int32 ChainLine=FS47FreeformCases::MysteryLine;
   for(int32 ChainClue=0;ChainClue<FS47FreeformCases::ClueCount;++ChainClue)ChainDesk.RecordEvidence(FS47FreeformCases::Def(ChainLine).Mystery->Clues[ChainClue].Finding,ChainDesk.CaseFile().ClueText(ChainLine,ChainClue));
   int32 ChainCause=0;while(ChainCause<FS47FreeformCases::CauseCount-1&&ChainDesk.CaseFile().Excluded(ChainLine,ChainCause))++ChainCause;
   const bool bFiled=ChainDesk.CaseSupported(ChainLine)&&ChainDesk.ConcludeCase(ChainLine,ChainCause);
   Check(bFiled&&ChainDesk.CaseFromField(ChainLine)&&!ChainDesk.CaseFromField(FS47FreeformCases::LostLine),TEXT("CHAIN-2: the same minute with the remote feeder looked at counts as a case from out on the land; a case not carried through does not"));
   ChainDesk.Stop();
  }
 }
 Check(FS47FarmSite::ByDaylight(7*3600.)&&!FS47FarmSite::ByDaylight(2*3600.)&&!FS47FarmSite::ByDaylight(21*3600.),TEXT("Marks: daylight is the morning after"));
 {
  int32 Reads[3]={0,0,0};bool bStable=true;
  for(int32 D=1;D<=300;++D){const int32 R0=FS47FarmSite::DaylightRead(47,TEXT("farm"),D);++Reads[R0];bStable&=R0==FS47FarmSite::DaylightRead(47,TEXT("farm"),D);}
  Check(bStable&&Reads[0]>30&&Reads[1]>30&&Reads[2]>30,TEXT("Marks: the second look reads ordinary, undecided or strange, and looking again never changes it"));
  bool bWords=true;for(int32 R0=0;R0<3;++R0)for(bool bFarmSite:{true,false}){const FString T0=FS47FarmSite::DaylightText(R0,bFarmSite);bWords&=T0.Contains(TEXT("by daylight"))&&!T0.Contains(TEXT("records"));}
  Check(bWords,TEXT("Marks: every daylight record says what is there and no more"));
 }
 {bool bNamed=true;for(int32 L0=0;L0<FS47FarmSite::SecondCallLines;++L0)bNamed&=FS47FarmSite::SecondCallLine(L0,TEXT("the church")).Contains(TEXT("the church"));Check(bNamed,TEXT("Calls: every second call names the place and says nothing about what it is"));}
 Check(FS47FreeformCases::LineForFinding(TEXT("farm_trace_daylight_d3"))==2,TEXT("Marks: a daylight record goes to the visitors' case"));
 {
  // PT-FEEDER-20261109 (d37: «lights at the remote feeder», only sounds and a flickering lamp, the line never ticked off). A reported
  // place always gives at least one find, at every hour: an arrival record of its own for each tier and pick, naming the place, never
  // the same at two hours; a lead in OUT ON THE LAND (case 8) and a field record in the notebook; and the report is then done.
  bool bEvery=true,bApart=true;
  for(int32 T0=0;T0<FS47FarmSite::ArrivalTiers;++T0)for(int32 P0=0;P0<FS47FarmSite::ArrivalPicks;++P0)
  {
   const FString A0=FS47FarmSite::ArrivalText(T0,P0,TEXT("the remote feeder on the county road"));
   bEvery&=A0.StartsWith(TEXT("The remote feeder on the county road, "))&&A0.Contains(TEXT("dark patches in the dirt"))&&A0.Len()>80&&!FS47FarmSite::ArrivalToast(T0).IsEmpty()&&!A0.Contains(TEXT("47"));
   for(int32 T1=T0+1;T1<FS47FarmSite::ArrivalTiers;++T1)for(int32 P1=0;P1<FS47FarmSite::ArrivalPicks;++P1)bApart&=A0!=FS47FarmSite::ArrivalText(T1,P1,TEXT("the remote feeder on the county road"));
  }
  const FString Key=FS47FarmSite::ArrivalKey(TEXT("feeder"),2);
  FS47FreeformSession Out(nullptr);Out.Begin(nullptr);
  const bool bBefore=!FS47FarmSite::CheckedIn(Out,TEXT("feeder"),2,false);
  Out.RecordEvidence(Key,FS47FarmSite::ArrivalText(2,0,TEXT("the remote feeder on the county road")));
  const auto* Land=Out.CaseFile().Get(FS47FreeformCases::LandLine);
  const bool bLead=FS47FreeformCases::LineForFinding(Key)==FS47FreeformCases::LandLine&&FS47FreeformCases::LineForFinding(TEXT("lights_trace_feeder"))==FS47FreeformCases::LandLine
   &&FS47FreeformCases::LineForFinding(TEXT("lights_trace_feeder_daylight_d3"))==FS47FreeformCases::LandLine&&Land&&Land->bOpen&&Land->Leads.Contains(Key)&&Out.HasFinding(Key)&&Out.FieldRecordKept();
  const bool bDone=FS47FarmSite::CheckedIn(Out,TEXT("feeder"),2,false)&&!FS47FarmSite::CheckedIn(Out,TEXT("feeder"),3,false)&&!FS47FarmSite::CheckedIn(Out,TEXT("church"),2,false);
  FS47FreeformSession Looked(nullptr);Looked.Begin(nullptr);Looked.RecordEvidence(TEXT("lights_trace_church"),TEXT("Open ground outside the church, the night of the lights."));
  FS47FreeformSession Morning(nullptr);Morning.Begin(nullptr);Morning.RecordEvidence(TEXT("farm_trace_daylight_d3"),TEXT("Pasture at the Alderete place, by daylight."));
  const bool bOther=FS47FarmSite::CheckedIn(Looked,TEXT("church"),4,false)&&FS47FarmSite::CheckedIn(Morning,TEXT("farm"),2,false)&&FS47FarmSite::CheckedIn(Out,TEXT("motel"),2,true);
  const bool bNote=FS47FreeformDirector::ReportDoneWith(false,true,true)&&!FS47FreeformDirector::ReportDoneWith(true,true,true)&&!FS47FreeformDirector::ReportDoneWith(false,true,false)&&!FS47FreeformDirector::ReportDoneWith(false,false,true);
  Check(bEvery&&bApart&&bBefore&&bLead&&bDone&&bOther&&bNote,TEXT("Reported place: at every hour a find of its own, kept in the notebook and as a lead out on the land, and the report ticked off once the place is found or found empty"));
  // The big night has the night: marks standing tonight keep what they are, but nothing more comes and no second call goes out.
  FS47FarmSite Yield(nullptr);Yield.TestArm(5*86400.);const bool bWould=Yield.WillStandAt(4*86400.)&&!Yield.Nudged();
  Yield.YieldTo(TEXT("big_night"));
  Check(bWould&&!Yield.WillStandAt(4*86400.)&&Yield.Nudged()&&Yield.Yielded()&&Yield.VisitStage()==4&&Yield.Armed(),TEXT("Reported place: marks standing on the big night's night yield to it, and stay to be found"));
  Out.Stop();Looked.Stop();Morning.Stop();
 }
 {
  FS47FreeformSession Trust(nullptr);Trust.Begin(nullptr);
  Check(!Trust.DeputyOnRoute()&&Trust.SheriffTrust()==0,TEXT("Patrol: nobody has written to the county yet"));
  Trust.TestLiveRules();Trust.TestAdvance(86400);Trust.TakenAway(3600);Trust.TakenCosts(Trust.Clock()-3600);
  Check(Trust.ConcludeCase(FS47FreeformCases::LostLine,1)&&Trust.SheriffTrust()==1&&Trust.DeputyOnRoute(),TEXT("Patrol: any case carried to a conclusion and filed earns the county's trust, whatever it says"));
  Trust.Stop();
 }
 {
  // M7-HUD-20261109: the HUD's card (the place and the next thing, in seventy characters), the station goal by night, the letter first
  FS47FreeformSession H(nullptr);H.Begin(nullptr);
  auto Sv=S47J::Read(S47J::Write(H.Save()));H.Stop();
  if(const auto Fd=S47J::Obj(Sv,TEXT("field"))){FS47ArrayWear Aw;Aw.Start();S47J::Set(Fd,TEXT("arrayWear"),Aw.Save());}
  FS47FreeformSession G(nullptr);G.Begin(Sv);G.Tutorial=0;G.ServiceGuide=0;G.Cash=50;G.Loan=0;
  const auto* Next=G.Field&&G.Field->ArrayWear().bActive?G.Field->ArrayWear().NextBranch():nullptr;
  Check(Next!=nullptr,TEXT("HUD card: the fixture's array is worn and has a next branch"));
  const double Base=G.WorldClock-FMath::Fmod(G.WorldClock,86400.);
  if(Next)
  {
   FString Card;G.WorldClock=Base+10*3600.;
   const FString Work=G.AvailableActivities(&Card);
   Check(Work.Contains(TEXT("UNFINISHED OBSERVATIONS"))&&Card.StartsWith(TEXT("OPS // "))&&Card.Contains(TEXT("JOB CARDS UP"))&&!Card.Contains(TEXT("OF 27"))&&!Work.Contains(TEXT("OF 27"))&&Card.Len()<=S47HudGauges::CardMost,
    TEXT("D45: HUD card: the day's work in one short line; the array count is not repeated on it (it stands on the STATION GOAL line)"));
   Check(G.StationGoalLine().Contains(TEXT("REPAIR THE"))&&!G.StationGoalLine().Contains(TEXT("DAYLIGHT")),TEXT("Station goal: by day, the repair"));
   // D45-20261110: the count is the live one, and the line says what this repair brings back
   Check(G.ArrayUpCount()==27-G.Field->ArrayWear().DownCount()&&G.StationGoalLine().Contains(FString::Printf(TEXT("ARRAY %d OF 27"),G.ArrayUpCount()))
    &&G.StationGoalLine().Contains(FString::Printf(TEXT("(+%d ANTENNAS)"),Next->Dishes.Num())),TEXT("D45: Station goal: the live array count once, and what the next repair brings back"));
   G.WorldClock=Base+2.5*3600.;
   Check(G.StationGoalLine().Contains(TEXT("WAITS FOR DAYLIGHT"))&&!G.StationGoalLine().Contains(TEXT("REPAIR THE")),TEXT("Station goal: at 02:30 it waits for daylight"));
   G.WorldClock=Base+.5*3600.;const bool bLate=!G.StationGoalLine().Contains(TEXT("DAYLIGHT"));G.WorldClock=Base+6*3600.;
   Check(bLate&&!G.StationGoalLine().Contains(TEXT("DAYLIGHT"))&&GoalWaits(Base+1*3600.)&&GoalWaits(Base+5.9*3600.)&&!GoalWaits(Base+23*3600.),TEXT("Station goal: it waits from one in the morning until six, not before or after"));
   for(FJob& J:G.Work)J.Stage=EJobStage::Filed;
   const bool bParts=G.Field->BranchNeeds(*Next).IsEmpty();
   G.WorldClock=Base+10*3600.;Card.Reset();G.AvailableActivities(&Card);
   Check(Card.StartsWith(FString(TEXT("ANTENNA "))+Next->Junction+TEXT(" // "))&&Card.Contains(bParts?TEXT("PARTS IN HAND"):TEXT("NEEDS"))&&Card.Len()<=S47HudGauges::CardMost,TEXT("HUD card: with the day's work done, the antenna and the repair"));
   G.WorldClock=Base+2.5*3600.;Card.Reset();const FString Night=G.AvailableActivities(&Card);
   // TASKS-TEN-20261113 (change 3): waiting for daylight is no task: the card says the night's own line, the wait is the half-line after it
   Check(Night.Contains(TEXT("WAITS FOR DAYLIGHT"))&&Night.Find(TEXT("STATION GOAL / "))>0&&!Card.Contains(TEXT("WAITS FOR DAYLIGHT"))&&Card.StartsWith(TEXT("OPS // "))&&Card.Len()<=S47HudGauges::CardMost
    &&G.HudObjective()==Card,TEXT("HUD card: at 02:30 the station goal's wait for daylight is only the half-line after the night's own line, never the card (TASKS-TEN change 3)"));
  }
  // every task the jobs and the first shift can give fits the card
  bool bFits=true;FString Longest;
  auto Try=[&](const FString& Task){const FString C=S47HudGauges::CardLine(Task);if(C.Len()>S47HudGauges::CardMost||(C.IsEmpty()&&!Task.IsEmpty())){bFits=false;Longest=C;}};
  for(int32 Stage=0;Stage<=int32(EJobStage::Filed);++Stage)for(int32 Kind=0;Kind<8;++Kind)
  {
   FJob J=G.Work[0];J.Stage=EJobStage(Stage);J.bAnomaly=Kind>0;J.bControl=Kind>1&&Kind<6;J.ControlAt=Kind>2&&Kind<6?8:0;J.bAudioReviewed=Kind==5||Kind==7;
   // GAMEPLAY-LOOP-01: 6 and 7 are the extra-tick take, before and after its one listen
   J.Phenomenon=Kind==3?TEXT("wrong_source"):Kind==4?TEXT("duplicate_print"):Kind>=6?TEXT("extra_pulse"):TEXT("time_disagreement");Try(G.JobNextAction(J));
  }
  for(int32 T=1;T<=6;++T){G.Tutorial=T;Try(G.TutorialObjective());}
  G.Tutorial=0;Try(G.Objective());
  Check(bFits,*(FString(TEXT("HUD card: every job step and the first shift fit in seventy characters "))+Longest));
  Check(LetterWaits(TEXT("MAILBOX / A letter from Dr. Ward's office is in the box at the road."))&&LetterWaits(TEXT("OPS / There is a letter from Dr. Ward's office in the tray."))
   &&LetterWaits(TEXT("OPS / Dr. Ward's assessment is in the tray."))&&!LetterWaits(TEXT("SARO under review / Dr. Ward assesses the station on May 12."))&&!LetterWaits(FString()),
   TEXT("HUD card: Ward's letter or his assessment, in the box or the tray, comes before the day's work"));
  G.Stop();
 }
 auto Future=S47J::New();S47J::Set(Future,TEXT("v"),2);R.Begin(Future);Check(!R.Active(),TEXT("Unknown Freeform save revisions do not restart a fresh session"));
 OK&=MinuteSelfTest(Lines); // LOOP-3-20261109 (S47FreeformMinute.cpp)
 OK&=SkySelfTest(Lines); // RX-ACQUIRE-SKY-20261113 (S47FreeformSky.cpp, S47SkyAcquire.cpp, SS47SkyChart.cpp)
 S.Stop();R.Stop();return OK;
}

bool FS47FreeformSession::ValidJournalHint(const FString& Id)const
{
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)if(Id==FString::Printf(TEXT("field-plan/%d"),Line)){const auto* C=Cases.Get(Line);return C&&C->bOpen;}
 for(const auto& J:Work)if(Id==J.Id+FString::Printf(TEXT("/%d"),int32(J.Stage)))return true;
 return ServiceGuide>0&&Id==FString::Printf(TEXT("service/%d"),ServiceGuide);
}
bool FS47FreeformSession::ValidJournalEntry(const FString& Id)const
{return FS47Journal::Guide(Id)||(M&&M->Story&&M->Story->Docs().ContainsByPredicate([&](const FS47Doc& D){return D.Id==Id;}));}
int32 FS47FreeformSession::JournalHintLevel(const FString& Id)const{return Journal.Hints.FindRef(Id);}
int32 FS47FreeformSession::RevealJournalHint(const FString& Id){if(!ValidJournalHint(Id))return 0;int32& L=Journal.Hints.FindOrAdd(Id);L=FMath::Min(3,L+1);if(M&&M->Flow)M->Flow->RequestAutosave(false);return L;}
bool FS47FreeformSession::JournalEntryRead(const FString& Id)const{return Journal.Read.Contains(Id);}
void FS47FreeformSession::MarkJournalEntryRead(const FString& Id){if(ValidJournalEntry(Id)&&!Journal.Read.Contains(Id)){Journal.Read.Add(Id);if(M&&M->Flow)M->Flow->RequestAutosave(false);}}
