#include "S47Freeform.h"
#include "S47Image.h"
#include "S47FreeformAudioDesk.h"
#include "S47RoomDecor.h"
#include "S47FarmSite.h"
#include "S47Taken.h"
#include "S47DeputyRound.h"
#include "S47LeftGear.h"
#include "S47Openings.h"
#include "S47Places.h"
#include "S47EventPlaces.h"
#include "S47Predecessor.h"
#include "S47RadioTips.h"
#include "S47FieldLife.h"
#include "S47NightRhythm.h"
#include "S47Aftermath.h"
#include "S47FarmNight.h"
#include "S47IndoorDread.h"
#include "S47FarmHouse.h"
#include "SS47TapeScope.h"
#include "S47FreeformTerminal.h"
#include "S47GameStyle.h"
#include "S47ReceiverReadout.h"
#include "SS47ScreenView.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47CaseFieldTests.h"
#include "S47Main.h"
#include "S47Night.h"
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
#include "S47Observing.h"
#include "S47Inter.h"
#include "S47Three.h"
#include "Widgets/Layout/SScrollBox.h"
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

void FS47FreeformSession::Begin(const S47J::FObj& Saved,bool bLethal)
{
 Stop();
 bOldCalendar=Saved.IsValid()&&!Saved->HasField(TEXT("calendar"));
 bActive=true;bDeadly=bLethal;bDead=bPowerFault=bAnomaly=bTelemetry=bAudioOpen=bAudioPlaying=bRecovering=false;
 ServiceGuide=GuideServiceBaseline=GuideSupplyBaseline=0;bGuideSupply=bGuideFocus=false;
 // A new Freeform game opens with the first-shift tutorial; dev/test runs opt in with -S47FreeformTutorial.
 Tutorial=!Saved&&M&&M->Flow&&(!M->Flow->bDevRun||FParse::Param(FCommandLine::Get(),TEXT("S47FreeformTutorial")))?1:0;
 TutorialJobId.Empty();FieldFocus=-1;
 DayNumber=1;WorldClock=8*3600;Cash=120;Loan=0;Selected=AudioSourceIndex=SelectedDish=0;WokenNight=RousedNight=0;WakeClock=0;ReviewWeek=Carried=StandbyTotal=0;LastPay=20;
 Settle=CrossSample=AudioCursor=AutosaveAt=IntegrationPulse=0;CrossPoint=0;bCrossRunning=false;
 JobSeed=M&&M->Flow&&!M->Flow->bDevRun?int32(FDateTime::UtcNow().GetTicks()^FPlatformTime::Cycles64()):470047;
 Journal.Load(S47J::Obj(Saved,TEXT("journal")));
 Work.Reset();Findings.Reset();FindingDays.Reset();MysterySteps={0,0,0};Upgrades={false,false,false};SavedAngles.Reset();bRestoreAngles=false;Feedback.Empty();PhenomenonId.Empty();
 if(Saved)
 {
  if(!JsonVersion(Saved)) { bActive=false;return; }
  Tutorial=FMath::Clamp(SafeInt(Saved,TEXT("tutorial")),0,6);
  TutorialJobId=S47J::Str(Saved,TEXT("tutorialJobId"));
  FieldFocus=FMath::Clamp(SafeInt(Saved,TEXT("fieldFocus"),-1),-1,FS47FreeformCases::CaseCount-1);
  JobSeed=int32(FMath::Clamp(SafeNum(Saved,TEXT("jobSeed"),JobSeed),double(MIN_int32),double(MAX_int32)));
  WorldClock=FMath::Clamp(SafeNum(Saved,TEXT("clock"),8*3600),0.,3153600000.);
  // Older saves have no review behind them: nothing is charged for weeks already played.
  ReviewWeek=FMath::Max(0,SafeInt(Saved,TEXT("reviewWeek"),-1));Carried=FMath::Clamp(SafeInt(Saved,TEXT("carried")),0,int32(ArrearsCap));StandbyTotal=FMath::Max(0,SafeInt(Saved,TEXT("standbyPaid")));
  WokenNight=FMath::Max(0,SafeInt(Saved,TEXT("wokenNight")));RousedNight=FMath::Max(0,SafeInt(Saved,TEXT("rousedNight")));
  if(!FMath::IsFinite(WorldClock)) {bActive=false;return;}
  DayNumber=FMath::Max(1,int32(FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay))+1);
  if(!Saved->HasField(TEXT("reviewWeek")))ReviewWeek=(DayNumber-1)/7;
  Cash=FMath::Clamp(SafeInt(Saved,TEXT("cash"),120),0,1000080);Loan=FMath::Clamp(SafeInt(Saved,TEXT("loan")),0,1000000);
  ServiceGuide=FMath::Clamp(SafeInt(Saved,TEXT("serviceGuide")),0,2);GuideServiceBaseline=FMath::Max(0,SafeInt(Saved,TEXT("guideServiceBaseline")));GuideSupplyBaseline=FMath::Max(0,SafeInt(Saved,TEXT("guideSupplyBaseline")));bGuideSupply=S47J::Bool(Saved,TEXT("guideSupply"));bGuideFocus=S47J::Bool(Saved,TEXT("guideFocus"),ServiceGuide==1);
  bDeadly=S47J::Bool(Saved,TEXT("lethal"),bLethal);bDead=S47J::Bool(Saved,TEXT("dead"));
  PhenomenonId=S47J::Str(Saved,TEXT("phenomenon"));bPowerFault=S47J::Bool(Saved,TEXT("powerFault"));bAnomaly=S47J::Bool(Saved,TEXT("anomaly"));
  Settle=FMath::Clamp(SafeNum(Saved,TEXT("settle")),0.,1.);CrossPoint=FMath::Clamp(SafeInt(Saved,TEXT("cross")),0,4);
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
   J.ReceiverClock=SafeNum(O,TEXT("receiverClock"));J.ReferenceClock=SafeNum(O,TEXT("referenceClock"));J.ControlReceiverClock=SafeNum(O,TEXT("controlReceiverClock"));J.ControlReferenceClock=SafeNum(O,TEXT("controlReferenceClock"));J.DuplicateClock=SafeNum(O,TEXT("duplicateClock"));J.DuplicateInterval=SafeNum(O,TEXT("duplicateInterval"));
   J.BeamPoint=FMath::Clamp(SafeInt(O,TEXT("beamPoint")),0,2);J.BeamSettle=FMath::Clamp(SafeNum(O,TEXT("beamSettle")),0.,1.);J.BeamSample=FMath::Clamp(SafeNum(O,TEXT("beamSample")),0.,8./3.);J.BeamAz=S47J::Nums(O,TEXT("beamAz"));J.BeamEl=S47J::Nums(O,TEXT("beamEl"));J.BeamAmplitude=S47J::Nums(O,TEXT("beamAmplitude"));
   J.bBeamVerified=S47J::Bool(O,TEXT("beamVerified"));J.bClockMeasured=S47J::Bool(O,TEXT("clockMeasured"));J.bDuplicateCollected=S47J::Bool(O,TEXT("duplicateCollected"));J.bDuplicateRejected=J.Phenomenon==TEXT("duplicate_print")&&!J.bDuplicateCollected&&S47J::Bool(O,TEXT("duplicateRejected"));J.OriginalPage=S47J::Str(O,TEXT("originalPage"));J.OriginalStamp=S47J::Str(O,TEXT("originalStamp"));J.DuplicatePage=S47J::Str(O,TEXT("duplicatePage"));J.DuplicateStamp=S47J::Str(O,TEXT("duplicateStamp"));
   // Legacy queued work was already immediately collectible; never reprint it.
   const int32 LegacyKind=PendingPrint(J);
   J.ChartFeed=FMath::Clamp(SafeNum(O,TEXT("chartFeed"),LegacyKind==0?PrinterSeconds:0),0.,PrinterSeconds);
   J.PacketFeed=FMath::Clamp(SafeNum(O,TEXT("packetFeed"),LegacyKind==1?PrinterSeconds:0),0.,PrinterSeconds);
   J.DuplicateFeed=FMath::Clamp(SafeNum(O,TEXT("duplicateFeed"),LegacyKind==2?PrinterSeconds:0),0.,PrinterSeconds);
   J.CaseLine=FMath::Clamp(SafeInt(O,TEXT("caseLine"),-1),-1,FS47FreeformCases::CaseCount-1);J.CaseTest=J.CaseLine<0?-1:FMath::Clamp(SafeInt(O,TEXT("caseTest"),-1),-1,FS47FreeformCases::TestCount-1);if(J.CaseTest<0)J.CaseLine=-1;
   J.bDocked=S47J::Bool(O,TEXT("docked"));J.bCaseValid=S47J::Bool(O,TEXT("caseValid"),true);J.bCaseRecorded=S47J::Bool(O,TEXT("caseRecorded"));if(J.CaseTest>=0){J.bAnomaly=false;J.Phenomenon.Empty();}
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
   Feedback=TEXT("Reference comparison is included. Its former $80 charge has been credited, repaying requisition debt first.");
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
    if(Current->Stage==EJobStage::Pointed||Current->Stage==EJobStage::Integrating){Current->Stage=EJobStage::New;Current->Elapsed=0;}
    if(Current->Phenomenon==TEXT("wrong_source")&&Current->bControl&&Current->ControlAt<8){Current->bControl=false;Current->ControlAt=0;Current->BeamAz.Reset();Current->BeamEl.Reset();Current->BeamAmplitude.Reset();Current->bBeamVerified=false;}
   }
   Feedback=TEXT("Invalid saved encoder data was discarded. Retained papers are safe; command fresh pointing before resuming unfinished measurements.");
  }
 }
 Cases.Load(S47J::Obj(Saved,TEXT("cases")),JobSeed);
 TakenCount=0;for(const auto& Kept:Findings)TakenCount+=Kept.Key.StartsWith(TEXT("taken-"))?1:0;
 // Older saves: an already retained discrepancy opens its case retroactively.
 for(const auto& F:Findings){const int32 Line=FS47FreeformCases::LineForFinding(F.Key);if(Line>=0)Cases.Open(Line,FMath::Max(1,FindingDays.FindRef(F.Key)),20*3600.);if(F.Key.StartsWith(TEXT("taken-"))||F.Key.StartsWith(TEXT("flood-room-")))Cases.RuleOutWind(Line);}
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)if(const auto* C=Cases.Get(Line);C&&C->Planned>=0)
 {
  // Field work has its own retained plan ID and deliberately creates no receiver job.
  const bool FieldPlan=C->bOpen&&!C->bConcluded&&!C->Results[C->Planned].bDone&&CaseCondition(Line,C->Planned)==ECaseCondition::FieldWork;
  if(!FieldPlan&&!Work.ContainsByPredicate([&](const FJob& J){return J.Id==C->PlannedJob&&J.CaseLine==Line&&J.CaseTest==C->Planned&&!J.bCaseRecorded;}))Cases.CancelPlan(Line);
 }
 if(FieldFocus>=0){const auto* Focus=Cases.Get(FieldFocus);if(!Focus||!Focus->bOpen||Focus->bConcluded)FieldFocus=-1;}
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
 if(!Places)Places=MakeShared<FS47Places>(M);Places->Begin(S47J::Obj(Saved,TEXT("places")));
 if(!EventPlaces)EventPlaces=MakeShared<FS47EventPlaces>(M);EventPlaces->Begin(S47J::Obj(Saved,TEXT("eventPlaces")));
 if(!Predecessor)Predecessor=MakeShared<FS47Predecessor>(M);Predecessor->Begin(S47J::Obj(Saved,TEXT("predecessor")),Saved.IsValid(),DayNumber);
 if(!Tips)Tips=MakeShared<FS47RadioTips>(M);Tips->Begin(S47J::Obj(Saved,TEXT("tips")));
 if(!TruckLamp)TruckLamp=MakeShared<FS47TruckLamp>(M);
 if(!YardAlarm)YardAlarm=MakeShared<FS47YardAlarm>(M);YardAlarm->Begin(S47J::Obj(Saved,TEXT("alarm")));
 if(!Care)Care=MakeShared<FS47TruckCare>(M);Care->Begin(S47J::Obj(Saved,TEXT("care")));
 if(!TripCamera)TripCamera=MakeShared<FS47TripCamera>(M);TripCamera->Begin(S47J::Obj(Saved,TEXT("tripCamera")));
 if(!Round)Round=MakeShared<FS47DayRound>(M);Round->Begin(S47J::Obj(Saved,TEXT("round")));
 if(!RoadCar)RoadCar=MakeShared<FS47RoadsideCar>(M);RoadCar->Begin(S47J::Obj(Saved,TEXT("roadCar")));
 if(!FarmNight)FarmNight=MakeShared<FS47FarmNight>(M);
 if(!HouseDoors)HouseDoors=MakeShared<FS47FarmHouseDoors>(M);HouseDoors->Begin(S47J::Obj(Saved,TEXT("houseDoors")));
 if(!IndoorDread)IndoorDread=MakeShared<FS47IndoorDread>(M);IndoorDread->Begin(S47J::Obj(Saved,TEXT("indoorDread")));
 Director->Begin(S47J::Obj(Saved,TEXT("director")));
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
 StopEquipmentFeedback();StopPrinterFeedback();FeedbackEncoders.Reset();
 if(!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&IsValid(M->Audio)){M->Audio->ReceiverMonitorOff();M->Audio->ReceiverRecordingOff();}
 // UObject purge may destroy the scene before this plain C++ session is released.
 const bool bCanRestore=!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&IsValid(M->Story)&&IsValid(M->Web);
 if(bCanRestore&&bPowerApplied&&!bLastPower)M->Story->ApplyFeederPower(false);
 bPowerApplied=false;
 if(CaseField)CaseField->Stop();
 if(Director)Director->Stop();if(AudioDesk)AudioDesk->Stop();if(Decor)Decor->Stop();if(IndoorDread)IndoorDread->Stop();if(Taken)Taken->Stop();if(Aftermath)Aftermath->Stop();if(Deputy)Deputy->Stop();if(Gear)Gear->Stop();if(Places)Places->Stop();if(Predecessor)Predecessor->Stop();if(TruckLamp)TruckLamp->Stop();if(YardAlarm)YardAlarm->Stop();if(TripCamera)TripCamera->Stop();if(RoadCar)RoadCar->Stop();if(FarmSite)FarmSite->Stop();if(FarmNight)FarmNight->Stop();if(HouseDoors)HouseDoors->Stop();if(Field)Field->Stop();
 TerminalView.Reset();ActiveTerminal=-1;
 if(bCanRestore){M->Web->SetWorkMotionPaused(false);if(bActive)M->Web->SetPrinterPaper(-1.f);}
 bActive=bTelemetry=bAudioOpen=bAudioPlaying=bRecovering=false;++RecoveryEpoch;
}
bool FS47FreeformSession::Held() const
{
 return !bActive||bDead||bRecovering||!M||M->Mode!=US47Main::EMode::Play||M->bPausedByMenu||(M->ModalOpen()&&!TerminalOnGlass()&&!bTapePanel)||(M->World&&M->World->bBusy);
}
bool FS47FreeformSession::ArrayMotionHeld() const
{
 return !bActive||bDead||bRecovering||!M||M->bPausedByMenu||(M->World&&M->World->bBusy)||(M->ModalOpen()&&!bTelemetry&&!TerminalOnGlass())||!HasPower();
}
bool FS47FreeformSession::HasPower() const
{
 // Closed: the county line is open at the meter (FS47Predecessor::Close). Only a fuelled generator runs anything.
 if(Predecessor&&Predecessor->StationClosed())return Field&&Field->HasBackupPower();
 return !bPowerFault||(Field&&Field->HasBackupPower());
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
 if(bRestoreAngles&&M->Web->IsLoaded())
 {
  bRestoreAngles=false;for(int32 I=0;I<27;++I){M->Web->SnapDish(I,SavedAngles[I*4],SavedAngles[I*4+1]);M->Web->PointDish(I,SavedAngles[I*4+2],SavedAngles[I*4+3],9,true);}
 }
 const double D=FMath::Min(Dt,.1);
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
  M->Story->Rx.bPowered=Powered;bPowerApplied=true;bLastPower=Powered;
  if(!Powered&&TerminalOnGlass()&&M->UI)M->UI->Close();
 }
 M->Web->SetWorkMotionPaused(ArrayMotionHeld());
 if(!Held())
 {
  WorldClock=AdvanceClock(WorldClock,D);int32 NowDay=FMath::Max(1,int32(FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay))+1);
  if(NowDay!=DayNumber){DayNumber=NowDay;AddDay(DayNumber);Changed(TEXT("New daily observing jobs are on the operations terminal. Unfinished work remains available."));}
  M->Story->SetClock(FMath::Fmod(WorldClock,86400.));
  AutosaveAt+=D;if(AutosaveAt>=30){AutosaveAt=0;Changed();}
  StepCaseReplies();
  StepWeeklyReview();
 }
 if(!ArrayMotionHeld())StepWork(D);
 if(AudioDesk)AudioDesk->Step(D);
 if(Decor)Decor->Step();
 if(FarmSite)FarmSite->Step(D);
 if(Taken)Taken->Step(D);
 if(Aftermath)Aftermath->Step(D);
 if(Deputy)Deputy->Step(D);
 if(Gear)Gear->Step(D);
 if(Places)Places->Step(D);
 if(Predecessor)Predecessor->Step(D);
 if(Tips&&!Held())Tips->Step(D);
 if(TruckLamp)TruckLamp->Step(D);
 if(YardAlarm)YardAlarm->Step(D);
 if(Care&&!Held())Care->Step(D);
 if(TripCamera)TripCamera->Step(D);
 if(Round&&!Held())Round->Step(D);
 if(RoadCar)RoadCar->Step(D);
 if(IndoorDread)IndoorDread->Step(D);
 if(FarmNight)FarmNight->Step(D);
 if(HouseDoors)HouseDoors->Step(D);
 if(bAudioOpen&&!Held())StepAudio(CaptureStartStage?Dt:D);
 if(M->Audio&&(!bAudioPlaying||!bAudioOpen||Held()||!HasPower()||!AudioDesk||!AudioDesk->ListeningPosition()))M->Audio->ReceiverRecordingOff();
 if(Field)Field->Step(D);
 if(CaseField)CaseField->Step(D);
 if(Director)Director->Step(D);
 StepPrinter(D);
 IngestObservations();
 StepEquipmentFeedback();
 StepTutorial();
 // Unlike the original Story console, the physical Freeform receiver owns its monitor here.
 if(M->Audio)
 {
  const FJob* Job=CurrentJob();
  const bool Listen=Job&&Powered&&M->Story->Rx.bPowered&&!Held()&&TerminalOnGlass()&&ActiveTerminal==1&&M->World&&M->World->Area==TEXT("saro");
  if(LiveCapture)LiveCapture->Audible(Listen);
  if(LiveCapture&&LiveCapture->Pending())M->Audio->ReceiverMonitorOff();
  else if(Listen)M->Audio->MonitorReceiver(M->Story->Rx,EffectiveProfile(*Job),FString::Printf(TEXT("freeform-source-%d"),Job->Source),ReceiverReception());
  else M->Audio->ReceiverMonitorOff();
 }
 if(M->UI&&M->Mode==US47Main::EMode::Play)M->UI->Objective(Objective());
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
FString FS47FreeformSession::PrinterHint()const
{
 const int32 I=PrinterIndex();if(I==INDEX_NONE)return TEXT("PRINTER: No paper waiting.");
 const auto& J=Work[I];const int32 Kind=PendingPrint(J);const double Progress=PrintFeed(J,Kind);
 if(Progress>=PrinterSeconds)return TEXT("PRINTER: Collect ")+J.Id+(Kind==0?TEXT(" original chart."):Kind==1?TEXT(" registered packet."):TEXT(" separate duplicate; retain the original."));
 return FString::Printf(TEXT("PRINTER: %s %s (%.0f%%). %s"),*J.Id,HasPower()?TEXT("feeding paper"):TEXT("feed paused"),100*Progress/PrinterSeconds,HasPower()?TEXT("Wait for the printer to finish before collecting."):TEXT("Restore power to finish; progress is retained."));
}
void FS47FreeformSession::StopPrinterFeedback()
{
 if(bPrinterFeedback&&!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&IsValid(M->Audio))M->Audio->Stop(TEXT("ffPrinter"),.04f);
 bPrinterFeedback=false;
}
void FS47FreeformSession::StepPrinter(double Dt)
{
 const int32 I=PrinterIndex();
 const bool Running=I!=INDEX_NONE&&HasPower()&&!Held();
 if(Running)
 {
  double& Feed=PrintFeed(Work[I],PendingPrint(Work[I]));const double Before=Feed;
  Feed=FMath::Min(PrinterSeconds,Feed+FMath::Max(0.,Dt));
  if(Before<PrinterSeconds&&Feed>=PrinterSeconds)Changed(PrinterHint());
 }
 const double Progress=PrinterProgress();
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
 S47CaptureSound::FFrame CaptureInput;
 if(M&&M->Story){CaptureInput.Frequency=M->Story->Rx.Frequency;CaptureInput.Gain=M->Story->Rx.Gain;CaptureInput.Bandwidth=M->Story->Rx.Bandwidth;CaptureInput.Quality=M->Story->Rx.Quality(EffectiveProfile(*J))*ReceiverReception();}
 if(J->Stage==EJobStage::Integrating&&J->CaptureOriginal.Eligible()&&(!PointReady()||Settle<1||!M||!M->Story||!M->Story->Rx.Pass(EffectiveProfile(*J))))InterruptCapture(TEXT("Receiver lock interrupted; retained measurements use compatible reference sound."));
 if(J->Stage==EJobStage::Pointed||J->Stage==EJobStage::Integrating)
 {
  Settle=PointReady()?FMath::Min(1.,Settle+Dt):0;
  if(J->Stage==EJobStage::Integrating&&J->CaseTest>=0&&J->bCaseValid)
  {
   const auto Cond=CaseCondition(J->CaseLine,J->CaseTest);
   if(Cond==ECaseCondition::Generator&&!(Field&&Field->GeneratorRunning())){J->bCaseValid=false;Changed(TEXT("Generator stopped during the case test. The recording continues, but it no longer runs off the mains; plan the test again."));}
   if(Cond==ECaseCondition::DoorsSealed&&!StationSealed()){J->bCaseValid=false;Changed(TEXT("The station was opened during the sealed-station test. The recording continues, but the test is spoiled; plan it again."));}
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
   if(J->Source==1&&M->Audio&&!Captured)
   {
    for(double At:J->TraceTicks)if(Was<At&&J->Elapsed>=At)M->Audio->Beep(1240,.008f,.1f,0,TEXT("square"));
    for(double At=PulsarPeriod;At<=SourceSeconds();At+=PulsarPeriod)if(Was<At&&J->Elapsed>=At)M->Audio->Beep(820,.012f,.16f,0,TEXT("square"));
   }
   if(J->Elapsed>=SourceSeconds()&&!bCrossRunning)
   {
    J->Stage=EJobStage::Chart;Changed(TEXT("PRINTER: Recording complete; paper feed queued. Wait for the printer to finish, then collect the original chart."));
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
    else Changed(TEXT("REEL DECK (wall table): Review the original and control recordings; the internal control is complete."));
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
bool FS47FreeformSession::SelectJob(int32 Index)
{
 if(!Work.IsValidIndex(Index))return false;
 if(Tutorial>0&&Work[Index].Id!=TutorialJobId){Changed(TEXT("Finish your FIRST SHIFT job before switching work orders. The other jobs will still be waiting in DAILY WORK."));return false;}
 const FJob* Current=CurrentJob();if(Current&&(Current->Stage==EJobStage::Integrating||(Current->bControl&&Current->ControlAt<8))){Changed(TEXT("Finish the active recording or control before selecting another job."));return false;}
 CaptureStartStage=0;if(TapeCapture)TapeCapture->Stop();
 Selected=Index;FieldFocus=-1;bGuideFocus=false;Settle=0;bCrossRunning=false;bAudioPlaying=false;AudioCursor=0;Changed();return true;
}
bool FS47FreeformSession::ChooseThird(bool bVirgo)
{
 FJob* J=Job();if(!J||J->Slot!=2||J->Stage!=EJobStage::New)return false;J->Source=bVirgo?3:2;J->Card=FS47FreeformJobs::Make(JobSeed,J->Day,J->Slot,J->Source);J->bGenerated=true;Changed();return true;
}
bool FS47FreeformSession::PointSelected()
{
 if(Tutorial==1){Feedback=TEXT("OPS (corner PC): Open DAILY WORK, read the job card and accept your first job before pointing the array.");Changed(Feedback);return false;}
 FJob* J=Job();if(!J||J->Stage>EJobStage::Pointed||!M||!M->Web||!HasPower())return false;
 if(EveningGated(*J)){Feedback=EveningLine(*J);Changed(Feedback);return false;}
 FieldFocus=-1;bGuideFocus=false;
 J->Stage=EJobStage::Pointed;Settle=0;bCrossRunning=false;
 M->Story->Rx.Azimuth=TargetAz(); // The physical pointing command is the receiver bearing too.
 M->Web->PointDishes(TargetAz(),TargetEl(),9);CommandFeedback();Changed(TEXT("TRACKING (right-hand PC): Command sent. Wait for encoder agreement, then tune and start recording at RECEIVER (centre PC)."));return true;
}
bool FS47FreeformSession::ReceiverReady() const
{
 const FJob* J=CurrentJob();return J&&J->Stage==EJobStage::Pointed&&HasPower()&&PointReady()&&Settle>=1&&M&&M->Story&&M->Story->Rx.Pass(EffectiveProfile(*J));
}
FString FS47FreeformSession::ReceiverNextAction() const
{
 const FJob* J=CurrentJob();if(!J)return TEXT("OPS (corner PC): Select an observation in DAILY WORK.");
 if(EveningGated(*J))return AvailableActivities();
 if(J->Stage==EJobStage::Chart||J->Stage==EJobStage::Packet||J->Stage==EJobStage::Filed)return JobNextAction(*J);
 if(!HasPower()){if(PrinterProgress()>=1)return PrinterHint();return TEXT("SERVICE BAY: Restore the SARO feeder or start the fuelled backup generator.");}
 if(J->Stage>=EJobStage::Chart)return JobNextAction(*J);
 // A case test states its physical precondition before the player commits the array.
 if(J->CaseTest>=0&&J->Stage<=EJobStage::Pointed){const FString Missing=CasePrecondition(*J);if(!Missing.IsEmpty())return Missing;}
 if(M&&M->Web&&J->Stage<=EJobStage::Integrating&&ArrayBlocked())return ArrayMeasurementStatus();
 if(J->Stage==EJobStage::New)return TEXT("TRACKING (right-hand PC): Point the array at the selected source.");
 if(J->Stage==EJobStage::Integrating)
 {
  if(!M||!M->Story||!M->Story->Rx.Pass(EffectiveProfile(*J)))return TEXT("RECEIVER (centre PC): Restore the work card tuning to resume recording; valid seconds are retained.");
  if(!PointReady()||Settle<1)return bCrossRunning?TEXT("TRACKING (right-hand PC): Automatic reference scan in progress. Wait for the samples; collect the original at the PRINTER when recording finishes."):TEXT("TRACKING (right-hand PC): Restore array pointing to resume recording; valid seconds are retained.");
 }
 if(J->Stage==EJobStage::Integrating)return FString::Printf(TEXT("RECEIVER (centre PC): Recording %.0f%% at %.0f%% measurement rate (%d/27 antennas). Keep tuning valid; collect the original at the PRINTER when complete."),100*J->Elapsed/SourceSeconds(),100*ArrayMeasurementRate(),AvailableDishCount());
 if(J->Stage!=EJobStage::Pointed)return JobNextAction(*J);
 if(!PointReady())return TEXT("TRACKING (right-hand PC): Wait for the commanded direction. If movement stops, inspect isolated elements at ARRAY STATUS (left-hand PC).");
 if(Settle<1)return TEXT("RECEIVER (centre PC): Wait one second for the array to settle, then tune the work card.");
 const auto P=EffectiveProfile(*J);const auto& R=M->Story->Rx;
 if(R.Pass(P))return TEXT("RECEIVER (centre PC): Press START RECORDING to record this source.");
 if(FMath::Abs(R.Frequency-P.F)>P.FTol)return FString::Printf(TEXT("RECEIVER (centre PC): Tune FREQUENCY to %.3f MHz (now %.3f). Watch the carrier peak and received percentage."),P.F,R.Frequency);
 if(R.Gain<P.Gain[0]||R.Gain>P.Gain[1])return FString::Printf(TEXT("RECEIVER (centre PC): Set GAIN between %.0f and %.0f (now %.0f)."),P.Gain[0],P.Gain[1],R.Gain);
 if(R.Bandwidth<P.Bw[0]||R.Bandwidth>P.Bw[1])return FString::Printf(TEXT("RECEIVER (centre PC): Set BANDWIDTH between %.0f and %.0f kHz (now %.0f)."),P.Bw[0],P.Bw[1],R.Bandwidth);
 return TEXT("RECEIVER (centre PC): Fine-tune until the received carrier reaches 100%.");
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
  if(!Cases.Get(J->CaseLine)||!Cases.Get(J->CaseLine)->bOpen){Feedback=TEXT("This case test no longer belongs to an open case.");return false;}
  const FString Missing=CasePrecondition(*J);if(!Missing.IsEmpty()){Feedback=Missing;Changed(Feedback);return false;}
  if(CaseCondition(J->CaseLine,J->CaseTest)==ECaseCondition::RelaySwap&&!Consume(TEXT("relay"))){Feedback=TEXT("Case test: no spare relay in the depot. Buy one at Caliche.");Changed(Feedback);return false;}
 }
 FieldFocus=-1;bGuideFocus=false;
 const bool bFirst=FirstPulseDue(*J);
 J->Stage=EJobStage::Integrating;J->Elapsed=0;J->bAnomaly=(J->Slot==1&&bAnomaly&&J->CaseTest<0)||bFirst;J->Phenomenon=J->bAnomaly&&!(bFirst&&!bAnomaly)?PhenomenonId:FString();
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
 Changed(TEXT("RECEIVER (centre PC): Recording started. Keep tuning, pointing and power valid; collect the original at the PRINTER when complete."));return true;
}
void FS47FreeformSession::PrintJob(bool bPacket)
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
  if(!J->bCaseValid)D.Page+=TEXT("TEST INVALID / its condition was broken during the recording. Plan the test again.\n");
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
  else D.Page+=FString::Printf(TEXT("REGISTERED / INTERNAL CONTROL %.1f SECONDS\nMARK %.1f TO %.1f SECONDS / CONCLUSION: SOURCE UNRESOLVED\nPHYSICAL FILING PENDING."),J->ControlAt,J->MarkStart,J->MarkEnd);
 }
 D.Transcript=D.Page;M->Story->AddDoc(D);if(M->UI)M->UI->Document(D);
}
bool FS47FreeformSession::CollectChart()
{
 const int32 Pending=PrinterIndex();if(Pending==INDEX_NONE)return false;
 const int32 Kind=PendingPrint(Work[Pending]);
 if(PrintFeed(Work[Pending],Kind)<PrinterSeconds){Feedback=PrinterHint();Changed(Feedback);return false;}
 if(Pending!=Selected&&!SelectJob(Pending)){Feedback=TEXT("Paper is waiting for ")+Work[Pending].Id+TEXT(". Finish the active recording or control, then collect it here.");return false;}
 FJob* J=Job();
 // The motor needs power; an already completed sheet does not.
 if(Kind==0){J->Stage=EJobStage::Recorded;J->bPrinted=true;PrintJob(false);}
 else if(Kind==2){CollectDuplicate();if(!J->bDuplicateCollected)return false;}
 else {J->Stage=EJobStage::Packet;PrintJob(true);}
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
 if(J.Phenomenon==TEXT("time_disagreement"))return FString::Printf(TEXT("SIMULTANEOUS DEVICE CLOCK READBACKS\nORIGINAL: RX %.3f / INDEPENDENT %.3f / DELTA %+.3f s\nINTERNAL CONTROL: RX %.3f / INDEPENDENT %.3f / DELTA %+.3f s\n%s"),J.ReceiverClock,J.ReferenceClock,J.ReceiverClock-J.ReferenceClock,J.ControlReceiverClock,J.ControlReferenceClock,J.ControlReceiverClock-J.ControlReferenceClock,J.bClockMeasured?TEXT("The timing discrepancy survives the internal reference; it is not evidence of missing time or a sky source."):TEXT("Timed independent control still required."));
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
bool FS47FreeformSession::StartControl()
{
 FJob* J=Job();if(!J||J->Stage!=EJobStage::Recorded||!J->bAnomaly||J->bControl||!HasPower())return false;
 FieldFocus=-1;bGuideFocus=false;J->bControl=true;J->ControlAt=0;
 if(!FParse::Param(FCommandLine::Get(),TEXT("NoSound"))&&J->CaptureOriginal.Enabled()&&J->Source==1&&!Specialized(J->Phenomenon)&&J->CaseTest<0)
 {J->CaptureControl.Begin(J->Id+TEXT("/control"),true,{});if(LiveCapture)LiveCapture->Stop();}
 if(J->Phenomenon==TEXT("wrong_source")){J->BeamPoint=0;J->BeamAz.Reset();J->BeamEl.Reset();J->BeamAmplitude.Reset();J->bBeamVerified=false;PointBeamControl();Changed(TEXT("TRACKING (right-hand PC): West, east and centre are measured automatically. Wait for completion, then open DAILY WORK at OPS (corner PC), select this job and press Compare and register the retained instrument measurements."));}
 else Changed(J->Phenomenon==TEXT("duplicate_print")?TEXT("RECEIVER RACK: Checking the master register automatically. When complete, collect the duplicate at the PRINTER; the original is retained."):TEXT("RECEIVER RACK: Eight-second internal reference running automatically; the original is retained. When complete, follow the job hint to OPS for clock measurements or the REEL DECK on the wall table for audio comparison."));return true;
}
void FS47FreeformSession::SeekAudio(double At){if(!FMath::IsFinite(At))return;CaptureStartStage=0;if(TapeCapture)TapeCapture->Stop();AudioCursor=FMath::Clamp(At,0.,AudioSourceIndex==0?18.:8.);bAudioPlaying=false;Changed();}
bool FS47FreeformSession::MarkAudioAt(double At)
{
 FJob* J=Job();if(!J||!J->bAnomaly||J->ControlAt<8||!FMath::IsFinite(At))return false;
 if(J->Stage>=EJobStage::Registered){Feedback=TEXT("Registered comparison is sealed. Playback is available; its marks cannot be changed.");return false;}
 if(Specialized(J->Phenomenon)){Feedback=TEXT("This is an instrument comparison. Use its measured control at OPS; no extra-tick marks are required.");return false;}
 if(AudioSourceIndex!=0){Feedback=TEXT("Mark the original recording, not the quiet internal control.");return false;}
 At=FMath::Clamp(At,0.,18.);
 if(J->MarkStart<0||J->MarkEnd>=0){J->MarkStart=At;J->MarkEnd=-1;}
 else{J->MarkEnd=FMath::Max(J->MarkStart,At);J->MarkStart=FMath::Min(J->MarkStart,At);}
 J->bAudioReviewed=false;Changed();return true;
}
bool FS47FreeformSession::AudioAction(const FString& Action)
{
 if((Action==TEXT("play")||Action==TEXT("confirm"))&&!RequirePower())return false;
 FJob* J=Job();if(!J||!J->bAnomaly||J->ControlAt<8)return false;
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
  if(AudioSourceIndex!=0){Feedback=TEXT("Mark the original recording, not the quiet internal control.");return false;}
  if(Action==TEXT("mark_start"))J->MarkStart=AudioCursor;else J->MarkEnd=AudioCursor;J->bAudioReviewed=false;
 }
 else if(Action==TEXT("confirm"))
 {
  if(J->Stage>=EJobStage::Registered){Feedback=TEXT("This comparison is already registered and retained.");return false;}
  if(Specialized(J->Phenomenon))return CompareInstrument();
  if(J->OriginalCovered<18||J->ControlCovered<8){Feedback=TEXT("Play both recordings through. Seeking does not play skipped audio.");return false;}
  if(!MarkCoversTicks(J->MarkStart,J->MarkEnd)){Feedback=TEXT("Mark the original from just before the first extra ticks to just after the last ones, about 9 to 13 seconds.");return false;}
  J->bAudioReviewed=true;RecordEvidence(TEXT("pulse-review"),TEXT("Original and internal reference reviewed. Extra ticks are absent from the short control; source remains unresolved."));
  if(Director)Director->Use(TEXT("ff:compare"));Changed(TEXT("OPS (corner PC): Register a supported conclusion; the marked comparison is retained."));
 }
 else return false;
 Changed();return true;
}
void FS47FreeformSession::NoteAudioStop(const TCHAR* Why,double Dt)
{
 AudioStopWhy=Why;UE_LOG(LogTemp,Display,TEXT("S47 capture playback stopped: %s cursor=%.3f source=%d dt=%.3f"),Why,AudioCursor,AudioSourceIndex,Dt);
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
 double& Covered=AudioSourceIndex==0?J->OriginalCovered:J->ControlCovered;if(Listening&&Was<=Covered+1e-7)Covered=FMath::Max(Covered,AudioCursor);
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
 FJob* J=Job();if(!J||J->Stage!=EJobStage::Recorded)return false;
 if(J->bDuplicateRejected){Feedback=ControlSummary(*J);return false;}
 if(J->bAnomaly&&Specialized(J->Phenomenon)&&!CompareInstrument())return false;
 if(J->bAnomaly&&(!J->bAudioReviewed||Conclusion!=(Specialized(J->Phenomenon)?InstrumentConclusion(J->Phenomenon):FString(TEXT("unresolved"))))){Feedback=Specialized(J->Phenomenon)?TEXT("Complete the measured comparison and register its instrumental finding. The controls do not prove an alien origin."):TEXT("A quiet short control does not establish a sky source. Review both recordings and retain the unresolved conclusion.");return false;}
 const bool AlreadyPaid=J->bPaid;
 J->Stage=EJobStage::Registered;
 // The OPS computer has accepted an actual result; opening its screen alone is silent.
 // The whole modem recording: nothing in the panel transition cuts it short.
 if(M&&M->Audio)M->Audio->PcTransferAtOps(M->Web,27.);
 if(!J->bAnomaly){if(!J->bPaid){J->bPaid=true;LastPay=JobPay(*J);Earn(LastPay);}J->Stage=EJobStage::Filed;}
 if(J->CaseTest>=0&&!J->bCaseRecorded){RecordCaseResult(*J);ClearCompletedSelection();return true;}
 if(Places)Places->AfterRegistered();
 const bool WasTutorial=InTutorial();StepTutorial();ClearCompletedSelection();
 if(WasTutorial&&!InTutorial())return true;
 Feedback=J->Stage==EJobStage::Registered?FString(TEXT("Registered. Packet feed queued; wait for the printer to finish, collect the labelled packet, then file its originals in the binder.")):AlreadyPaid?FString(TEXT("Reference check registered. Original retained; no additional payment is due.")):FString::Printf(TEXT("Routine result registered. $%d credited; the original remains in EARLIER WORK and the notebook."),LastPay);Changed(Feedback);return true;
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
bool FS47FreeformSession::FilePacket()
{
 // Another job may have been selected since the packet was collected; its paper still waits here.
 FJob* J=Work.FindByPredicate([](const FJob& X){return X.Stage==EJobStage::Packet;});if(Job()&&Job()->Stage==EJobStage::Packet)J=Job();if(!J)return false;
 if(!NearBinder()){Feedback=TEXT("Take the registered paper packet to the actual records-room binder. The remote archive is read-only.");Changed(Feedback);return false;}
 J->Stage=EJobStage::Filed;if(!J->bPaid){J->bPaid=true;Earn(JobPay(*J));}
 if(J->bAnomaly)
 {
  RecordEvidence(TEXT("pulse-archive"),TEXT("Original, internal control and registered conclusion physically filed under one reference."));
  if(Director)Director->Use(TEXT("ff:archive"));
 }
 const bool WasTutorial=InTutorial();StepTutorial();ClearCompletedSelection();
 if(!(WasTutorial&&!InTutorial()))Changed(TEXT("OPS (corner PC): Select DAILY WORK or CASE ARCHIVE for follow-up. Originals filed; work credited."));return true;
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
 const int32 Paid=FMath::Min(Amount,Loan);Loan-=Paid;Cash=FMath::Min(1000080,Cash+Amount-Paid);Changed();
}
int32 FS47FreeformSession::InventoryCount(const FString& Id)const{return Field?Field->InventoryCount(Id):0;}
void FS47FreeformSession::AddItem(const FString& Id,int32 Count){if(Field)Field->AddItem(Id,Count);}
bool FS47FreeformSession::Consume(const FString& Id){return Field&&Field->Consume(Id);}
void FS47FreeformSession::SetPowerFault(bool bFault){if(bPowerFault==bFault)return;bPowerFault=bFault;if(!HasPower())StopPrinterFeedback();Changed(bFault?TEXT("Feeder voltage is unstable. Measure the circuit or transfer to the generator."):TEXT("Feeder supply restored and verified."));}
void FS47FreeformSession::SetAnomaly(bool bValue)
{
 if(bAnomaly==bValue)return;bAnomaly=bValue;
 if(bValue)
 {
  const bool Available=Work.ContainsByPredicate([this](const FJob& J){return J.Day==DayNumber&&J.Slot==1&&J.CaseTest<0&&J.Stage<=EJobStage::Pointed;});
  if(!Available)
  {
   FJob J;J.Id=FString::Printf(TEXT("FF-D%04d-INCIDENT-%04d"),DayNumber,Work.Num());J.Day=DayNumber;J.Slot=1;J.Source=1;J.bPaid=true;Work.Add(J);
   if(CurrentJob()&&CurrentJob()->Stage==EJobStage::Filed)Selected=Work.Num()-1;
  }
 }
 Changed(bValue?TEXT("Pulsar monitor discrepancy. Select the pulsar or incident job at OPS, retain an original, then compare an independent control."):FString());
}
void FS47FreeformSession::RecordEvidence(const FString& Id,const FString& Text)
{
 if(Id.IsEmpty()||Findings.Contains(Id))return;
 Findings.Add(Id,Text);FindingDays.Add(Id,DayNumber);TakenCount+=Id.StartsWith(TEXT("taken-"))?1:0;
 if(M&&M->Story)
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
 FJob* J=Job();if(J&&J->Stage==EJobStage::Integrating){J->Stage=EJobStage::Pointed;J->Elapsed=0;Settle=0;bCrossRunning=false;}
 TestAdvance(WorldSeconds);
}
void FS47FreeformSession::Recover(const FString& Why)
{
 if(!bActive||bDead)return;
 bAudioPlaying=false;
 if(bDeadly){bDead=true;Changed(TEXT("Contact. Load the last saved case to continue."));return;}
 FJob* J=Job();if(J&&J->Stage==EJobStage::Integrating){J->Stage=EJobStage::Pointed;J->Elapsed=0;Settle=0;bCrossRunning=false;}
 WorldClock+=15*60;DayNumber=FMath::Max(1,int32(FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay))+1);AddDay(DayNumber);
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
  if(Main->Audio)Main->Audio->ReceiverMonitorOff();
  bTelemetry=false;bTapePanel=false;TerminalView.Reset();ActiveTerminal=-1;Changed();
 });bTelemetry=bLive;bAudioOpen=bAudio;
}
namespace
{
 namespace T=S47FreeformTerminal;
 FString StatusWord(const FS47FreeformSession::FJob& J,bool bSelected)
 {
  using E=FS47FreeformSession::EJobStage;
  if(J.Stage==E::New)return bSelected?TEXT("READY"):J.Slot==2?TEXT("OPTIONAL"):TEXT("QUEUED");
  if(J.Stage==E::Pointed)return TEXT("POINTED");if(J.Stage==E::Integrating)return TEXT("RECORDING");if(J.Stage==E::Chart)return TEXT("AT PRINTER");
  if(J.Stage==E::Recorded)return J.bAnomaly?TEXT("INVESTIGATE"):TEXT("REGISTER");if(J.Stage==E::Registered)return TEXT("PACKET");if(J.Stage==E::Packet)return TEXT("FILE");return TEXT("FILED");
 }
}
TSharedRef<SWidget> FS47FreeformSession::OpsShell(int32 Section,TSharedRef<SWidget> Content)
{
 OpsSection=Section;
 const bool CaseWaiting=CaseAwaitingChoice()>=0,Service=ServiceGuide==1||(Field&&Field->HasArrayFault());
 bool Photos=false;if(Director)for(const auto& C:Director->PhotoClaims())Photos|=!C.bPaid;
 TArray<T::FNav> Items;
 Items.Add({TEXT("OVERVIEW"),Section==0,[this]{Feedback.Empty();OpsSection=0;OpenOperations();}});
 Items.Add({TEXT("DAILY WORK"),Section==1,[this]{Feedback.Empty();OpsSection=1;OpenOperations();}});
 Items.Add({TEXT("CASE BOARD"),Section==2,[this]{Feedback.Empty();OpenCaseBoard(-1);},CaseWaiting});
 Items.Add({TEXT("CASE ARCHIVE"),Section==3,[this]{Feedback.Empty();OpenArchive();}});
 Items.Add({TEXT("SERVICE"),Section==4,[this]{Feedback.Empty();OpenServiceGuide();},Service});
 Items.Add({TEXT("SUPPLIES"),Section==5,[this]{Feedback.Empty();OpsSection=5;OpenOperations();},Photos});
 return T::Columns({{.21f,S47Kit::Col({T::Nav(Items),T::Small(TEXT("OVERVIEW returns to the OPS main menu. ESC returns to the room."))})},{.79f,Content}},16);
}
void FS47FreeformSession::OpenOperations()
{
 if(!RequirePower()||!M||!M->UI)return;
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
    const int32 InGroup=J.Stage==EJobStage::Filed?2:Active||I==Selected?0:J.Day==DayNumber?1:2;if(InGroup!=Group)continue;
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
     if(!J->bDuplicateRejected)A.Add(T::Small(TEXT("COMPARE HERE / OPS, DAILY WORK. Finish the physical control first; for a duplicate check, collect the original and separate duplicate. One registration compares the retained measurements without changing them, then retains the supported finding.")));
     const FString Conclusion=InstrumentConclusion(J->Phenomenon);
     const FString Name=J->Phenomenon==TEXT("wrong_source")?TEXT("Compare and register an off-centre maximum; origin unresolved"):J->Phenomenon==TEXT("time_disagreement")?TEXT("Compare and register disagreement in the receiver timing path"):TEXT("Compare and register mismatched duplicate; preserve original");
     if(!J->bDuplicateRejected)A.Add(T::Button(Name,[this,Conclusion]{RegisterComparison(Conclusion);OpenOperations();},true));
    }
    else
    {
     A.Add(T::Small(TEXT("Compare the original with the internal reference at the reel deck on the wall table before registering.")));
     A.Add(T::Small(TEXT("A quiet internal reference does not prove a sky source. Retain the unresolved result; a directional or independent field control is still needed.")));
     A.Add(T::Button(TEXT("Source unresolved; retain both measurements"),[this]{RegisterComparison(TEXT("unresolved"));OpenOperations();},true));
    }
   }
   else A.Add(T::Button(J->CaseTest>=0&&!J->bCaseRecorded?FString::Printf(TEXT("Register observation and case test result / $%d"),JobPay(*J)):J->bPaid?FString(TEXT("Register retained reference / no additional payment")):FString::Printf(TEXT("Register routine result / $%d"),JobPay(*J)),[this]{RegisterComparison(TEXT("normal"));OpenOperations();},true));
  }
  return S47Kit::Col(A);
 };
 int32 Remaining=0,TodayRemaining=0,EarlierRemaining=0,Rejected=0;
 for(const auto& X:Work)if(X.Stage!=EJobStage::Filed&&X.bDuplicateRejected)++Rejected;
 for(const auto& X:Work)if(X.Stage!=EJobStage::Filed&&!X.bDuplicateRejected){++Remaining;if(X.Day==DayNumber)++TodayRemaining;else if(X.Day<DayNumber)++EarlierRemaining;}
 const FString QueueExplanation=Remaining==0
  ?FString(Rejected>0?TEXT("NO ACTIONABLE OBSERVATIONS REMAIN / Unverifiable legacy originals remain retained without completion, finding or payment. "):TEXT("ALL OBSERVING WORK COMPLETE / Results are retained in EARLIER WORK and CASE ARCHIVE. "))+AvailableActivities()
  :FString::Printf(TEXT("MORE WORK / %d unfinished today, %d carried over. Each morning at 08:00 brings three observing jobs for that evening, including a choice for the third. Select another row in DAILY WORK; SHOW EARLIER WORK includes completed results immediately and untouched older jobs. Finish any active recording or control before switching. Unfinished jobs remain available; they pay in full on their day and the next, then less."),TodayRemaining,EarlierRemaining);
 const FJob* J=CurrentJob();
 auto Account=T::Panel(TEXT("STATION ACCOUNT"),S47Kit::Col({
  T::Value(TEXT("OPERATING BUDGET"),FString::Printf(TEXT("$%d"),Cash)),T::Value(TEXT("REQUISITION"),FString::Printf(TEXT("$%d"),Loan)),
  T::Value(TEXT("SARO"),Predecessor?Predecessor->AccountLine():FString(TEXT("IN SERVICE"))),
  T::Value(TEXT("NEXT REVIEW"),NextReviewText()),
  T::Value(TEXT("SELECTED JOB"),FieldFocus>=0?FString(TEXT("FIELD CASE: "))+FS47FreeformCases::Def(FieldFocus).Title:J?FString(FS47Observing::Runs()[J->Source].Source):FString(TEXT("NONE")),true),
  T::Small(TEXT("NEXT STEP")),T::Banner(TAttribute<FString>::CreateLambda([this]{return Objective().ToUpper();}),true)}));
 if(Section==0)
 {
  if(Predecessor)Predecessor->OpsRows(Rows);
  if(Tutorial==0)
  {
   TArray<S47Kit::FW> Continue;
   const int32 Next=J&&J->Stage!=EJobStage::Filed&&!J->bDuplicateRejected&&!EveningGated(*J)?Selected:Work.IndexOfByPredicate([this](const FJob& X){return X.Stage!=EJobStage::Filed&&!X.bDuplicateRejected&&!EveningGated(X);});
   if(Work.IsValidIndex(Next))
   {
    const FJob& NextJob=Work[Next];
    Continue.Add(T::Small(TEXT("A retained observation is available now; unfinished work does not expire at dusk.")));
    Continue.Add(T::Button(TEXT("Continue observation / ")+NextJob.Id+TEXT(" / ")+FS47Observing::Runs()[NextJob.Source].Source,[this,Next]
    {
     if(SelectJob(Next)){OpsSection=1;OpenOperations();}
    },true));
   }
   else if(const FJob* Later=Work.FindByPredicate([this](const FJob& X){return EveningGated(X);}))Continue.Add(T::Small(EveningLine(*Later)));
   else Continue.Add(T::Small(Rejected>0?TEXT("Only unverifiable legacy observations remain in this queue. Their originals stay retained without a finding or payment. Start the optional reference check below for a fresh baseline without payment, or prepare your camera. New daily observations arrive at 08:00."):TEXT("Daily observations are complete. Start the optional reference check below to retain a fresh baseline without extra payment.")));
   FString Photo;
   const FS47Album* Album=M&&M->Night?M->Night->Album.Get():nullptr;
   if(Album&&Album->PendingLabPhoto())Photo=TEXT("PHOTO LAB / WET BENCH: develop, fix and collect your exposed photograph. Keep its original before submitting it at OPS.");
   else if(CameraLeftOut())Photo=TEXT("YOUR CAMERA is where you were taken. Your film and prints are safe here; go and get it.");
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
   Rows.Add(T::Small(TEXT("OPTIONAL FIRST JOB / Establish a normal reference: complete the 3C 286 calibration cross and register it here, so later discrepancies can be compared against working instruments.")));
  auto Today=T::Panel(TEXT("TODAY'S WORK"),S47Kit::Col({T::Table({TEXT("JOB"),TEXT("SOURCE"),TEXT("STATUS")},{1.25f,1.35f,1.f},Rows4(false)),
   T::Columns({{.6f,T::Big(FString(TEXT("OPEN SELECTED JOB")),[this]{OpsSection=1;OpenOperations();},1)},{.4f,T::Big(FString(TEXT("EARLIER WORK")),[this]{bShowEarlierJobs=true;OpsSection=1;OpenOperations();})}},10)}));
  Rows.Add(T::Columns({{.64f,Today},{.36f,Account}},12));
  Rows.Add(Actions());
  if(const FString Plan=TonightPlan();!Plan.IsEmpty())Rows.Add(T::Panel(FString(),T::Small(Plan)));
  if(!Feedback.IsEmpty())Rows.Add(T::Small(Feedback));
  const double Dusk=FMath::FloorToDouble(WorldClock/SecondsPerDay)*SecondsPerDay+19*3600.;
  if(Dusk>WorldClock+60)Rows.Add(T::Button(TEXT("Wait until dusk / review time and equipment first"),[this,Dusk]{OpenWaitConfirmation(Dusk);}));
  double Up=0;
  if(const FJob* Later=NextEveningCard(Up))
  {
   Rows.Add(T::Button(FString::Printf(TEXT("Wait until %s is up at %s / review time and equipment first"),FS47Observing::Runs()[Later->Source].Source,*FS47FreeformCases::Clock(Up)),[this,At=Up]{OpenWaitConfirmation(At);},true));
  }
  const int32 PlanLine=FieldFocus>=0?FieldFocus:J?J->CaseLine:-1;double Target=0;
  if(CaseWaitTarget(PlanLine,Target))Rows.Add(T::Button(TEXT("Wait until five minutes before this test / review consequences"),[this,Target]{OpenWaitConfirmation(Target);},true));
  Rows.Add(T::Columns({{.5f,T::Big(FString(TEXT("REST UNTIL MORNING")),[this]{OpenWaitConfirmation((FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay)+1)*SecondsPerDay+8*3600);})},{.5f,T::Big(FString(TEXT("SERVICE ORDER")),[this]{OpenServiceGuide();})}},10));
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
   if(!J->bPaid)D.Add(T::Value(TEXT("PAYS NOW"),FString::Printf(TEXT("$%d%s"),JobPay(*J),J->bDocked?TEXT(" / HALF RATE"):DayNumber-J->Day>1?TEXT(" / LATE"):TEXT(""))));
   D.Add(T::Small(TEXT("NEXT STEP")));D.Add(T::Banner(TAttribute<FString>::CreateLambda([this]{return Objective().ToUpper();}),true));
  }
  else D.Add(T::Small(TEXT("Select a job in the queue.")));
  Rows.Add(T::Columns({{.55f,Queue},{.45f,T::Panel(TEXT("SELECTED JOB"),S47Kit::Col(D))}}));
  Rows.Add(Actions());
  if(!Feedback.IsEmpty())Rows.Add(T::Small(Feedback));
  OpenTerminal(3,TEXT("OPS / DAILY WORK"),OpsShell(1,S47Kit::Col(Rows)));
  return;
 }
 // SUPPLIES: stock, purchases and documentation credits.
 TArray<S47Kit::FW> Stock;
 for(const TCHAR* Item:{TEXT("fuse"),TEXT("relay"),TEXT("donor"),TEXT("fuel"),TEXT("film")})Stock.Add(T::Value(FString(Item).ToUpper(),FString::Printf(TEXT("%d"),InventoryCount(Item))));
 if(Field)Stock.Add(T::Value(TEXT("GENERATOR TANK"),FString::Printf(TEXT("%.1f h"),Field->GeneratorHours())));
 Rows.Add(T::Columns({{.5f,T::Panel(TEXT("DEPOT STOCK"),S47Kit::Col(Stock))},{.5f,Account}}));
 static const TCHAR* Names[]={TEXT("Relay reserve / $60"),TEXT("Reference comparison / $80"),TEXT("Field service kit / $50")};
 TArray<S47Kit::FW> U;for(int32 I=0;I<3;++I){if(I==1)continue;const int32 Index=I;U.Add(T::Button(Upgrades[I]?FString(Names[I])+TEXT(" / INSTALLED"):FString(Names[I]),[this,Index]{Upgrade(Index);OpenOperations();},false,!Upgrades[I]));}
 Rows.Add(T::Panel(TEXT("UPGRADES"),S47Kit::Col({T::Small(TEXT("Relay reserve reduces recurrent failures; the field kit includes two fuses and a relay. Manual reference comparison is included.")),S47Kit::Row(U)})));
 {
  // What is on offer, what it costs and what it waits for. Everything the station can get hold of stands here.
  TSharedRef<SScrollBox> Offers=SNew(SScrollBox);
  for(const auto& Offer:FS47Openings::Defs())
  {
   const FString Id=Offer.Id,Waits=KitWaits(Id);
   if(HasKit(Id))Offers->AddSlot()[T::Small(FString::Printf(TEXT("%s / IN HAND. %s"),Offer.Title,Offer.What))];
   else if(!Waits.IsEmpty())Offers->AddSlot()[T::Small(FString::Printf(TEXT("%s / $%d / WAITS FOR %s"),Offer.Title,KitPrice(Id),*Waits))];
   else
   {
    const int32 Price=KitPrice(Id);
    Offers->AddSlot()[T::Small(Offer.What)];
    Offers->AddSlot()[T::Button(FString::Printf(TEXT("%s / $%d"),Offer.Title,Price),[this,Id]{BuyKit(Id);OpenOperations();},false,Cash>=Price)];
   }
  }
  Rows.Add(T::Panel(TEXT("STORES LIST"),SNew(SBox).HeightOverride(150)[Offers]));
 }
 if(Director)
 {
  TArray<S47Kit::FW> Claims;
  for(const auto& Claim:Director->PhotoClaims())
  {
   const FString Id=Claim.Id;
   const bool Orb=Id.StartsWith(TEXT("dusk_orb."));
   if(Claim.bPaid&&!Orb)continue;
   Claims.Add(T::Small(Claim.Title+TEXT(" / ORIGINAL ")+Claim.PhotoId));
   const auto* Shot=M->Night&&M->Night->Album?M->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& P){return P.Id==Claim.PhotoId;}):nullptr;
   if(!Shot||Shot->bUnavailable||Shot->Url.IsEmpty())Claims.Add(T::Small(TEXT("ORIGINAL UNAVAILABLE / This photograph cannot be submitted. Existing incident records are retained.")));
   else if(!Shot->IsDeveloped())Claims.Add(T::Small(TEXT("UNDEVELOPED ORIGINAL / PHOTO LAB: Develop, fix and collect this photograph at the wet bench before submitting it.")));
   else
   {
    if(!Claim.bPaid)Claims.Add(T::Button(FString::Printf(TEXT("Register photograph / $%d documentation credit"),Claim.Reward),[this,Id]{Director->RegisterPhotoClaim(Id);OpenOperations();},true,CanRegisterPhotoEvidence()));
    else Claims.Add(T::Small(TEXT("ORIGINAL REGISTERED / Documentation credit already received.")));
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
      else Claims.Add(T::Button(TEXT("Follow this return plan"),[this,Id]{if(Director)Director->FollowPhotoControl(Id);OpenOperations();},false,Available&&CanRegisterPhotoEvidence()));
     }
     const auto* Control=Planned?AlbumShots.FindByPredicate([&](const FS47AlbumShot& S){return !S.bUnavailable&&!S.Url.IsEmpty()&&S.IsDeveloped()&&(Claim.ControlPhotoId.IsEmpty()?S.Targets.ContainsByPredicate([&](const TPair<FString,FVector2D>& T){return T.Key==TEXT("freeform-control:")+Id;}):S.Id==Claim.ControlPhotoId);}):nullptr;
     auto PhotoCard=[](const FS47AlbumShot* S,const FString& Label)->S47Kit::FW
     {
      if(!S||S->bUnavailable||!S->IsDeveloped()||S->Url.IsEmpty())return T::Small(Label+TEXT(" / ORIGINAL UNAVAILABLE"));
      const int32 Seconds=int32(FMath::Fmod(FMath::Max(0.,S->Clock),86400.)),Day=FMath::Max(1,int32(FMath::FloorToDouble((S->Clock-28800.)/86400.))+1);
      return S47Kit::Col({T::Small(Label),
       SNew(SBox).HAlign(HAlign_Center)[S47Kit::Image(S47Image::FromDataUrl(S->Url),240.f)],
       T::Small(FString::Printf(TEXT("%s / D%d %02d:%02d:%02d"),*S->Id,Day,Seconds/3600,(Seconds/60)%60,Seconds%60))});
     };
     if(Control)
     {
      Claims.Add(T::Columns({{.5f,PhotoCard(Baseline,TEXT("FIRST ORIGINAL"))},{.5f,PhotoCard(Control,TEXT("LATER CONTROL"))}},10));
      const FString ControlId=Control->Id;
      if(Claim.ControlPhotoId.IsEmpty()&&!Director->CanRegisterPhotoControl(Id,ControlId))Claims.Add(T::Small(TEXT("COMPARISON UNAVAILABLE / Powered OPS and both developed originals with matching-view records are required. Existing records stay unchanged.")));
      if(Claim.ControlPhotoId.IsEmpty())Claims.Add(T::Button(TEXT("Retain comparison / no payment"),[this,Id,ControlId]{if(Director)Director->RegisterPhotoControl(Id,ControlId);OpenOperations();},false,Director->CanRegisterPhotoControl(Id,ControlId)));
      else Claims.Add(T::Small(TEXT("COMPARISON RETAINED / Both originals unchanged. Identity and shared cause unresolved.")));
     }
     else
     {
      Claims.Add(PhotoCard(Baseline,TEXT("FIRST ORIGINAL")));
      Claims.Add(T::Small(Director->PhotoControlHint(Id)));
      if(!Planned&&Shot&&Shot->Pose.IsSet())Claims.Add(T::Button(TEXT("Plan return to original viewpoint / no payment"),[this,Id]{if(Director)Director->PlanPhotoControl(Id);OpenOperations();},false,CanRegisterPhotoEvidence()));
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
     Claims.Add(T::Button(TEXT("Keep a local-noise check for later / no payment"),[this,Id]{if(Director)Director->PlanLocalNoiseCheck(Id);OpenOperations();},false,CanRegisterPhotoEvidence()));
    }
   }
  }
  if(!Claims.IsEmpty())Rows.Add(T::Panel(TEXT("FIELD PHOTOGRAPHS / DOCUMENTATION CREDIT"),S47Kit::Col(Claims)));
 }
 if(!Feedback.IsEmpty())Rows.Add(T::Small(Feedback));
 OpenTerminal(3,TEXT("OPS / SUPPLIES"),OpsShell(5,S47Kit::Col(Rows)));
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
void FS47FreeformSession::OpenStatus()
{
 if(!RequirePower()||!M||!M->UI||!M->Web)return;
 const int32 Count=M->Web->DishCount();SelectedDish=FMath::Clamp(SelectedDish,0,FMath::Max(0,Count-1));
 auto Short=[this](int32 I){return M->Web->DishId(I).Replace(TEXT("-"),TEXT(""));};
 auto State=[this](int32 I){if(I==SelectedDish)return 1;if(!M->Web->DishRemoteAvailable(I))return 2;return FMath::Abs(FMath::FindDeltaAngleDegrees(M->Web->DishAz(I),M->Web->DishTargetAz(I)))>.1||FMath::Abs(M->Web->DishEl(I)-M->Web->DishTargetEl(I))>.1?3:0;};
 auto Pick=[this](int32 I){SelectedDish=I;if(Field&&M->Web->DishId(I)==Field->ServiceDishId()){Field->DiagnoseService();RecordEvidence(TEXT("service-diagnosed-")+Field->ServiceDishId(),Field->ServiceDishId()+TEXT(" command/encoder status inspected at the status terminal."));}OpenStatus();};
 auto Grid=T::Panel(FString::Printf(TEXT("ARRAY STATUS / %d ANTENNAS"),Count),S47Kit::Col({T::AntennaGrid(Count,Short,State,Pick),
  T::Columns({{.5f,T::Banner(TAttribute<FString>::CreateLambda([this,Count]{return FString::Printf(TEXT("%d / %d AVAILABLE"),AvailableDishCount(),Count);}),TAttribute<bool>::CreateLambda([this,Count]{return AvailableDishCount()<Count;}))},
              {.5f,T::Banner(TAttribute<FString>::CreateLambda([this]{return bPowerFault?FString(HasPower()?TEXT("FEEDER FAULT / ON GENERATOR"):TEXT("FEEDER VOLTAGE UNSTABLE")):FString(TEXT("FEEDER NORMAL"));}),TAttribute<bool>::CreateLambda([this]{return bPowerFault;}))}}),
  T::Small(TEXT("KEY / outline READY / amber SELECTED / red ISOLATED OR FAULT / blinking MOVING"))}));
 const int32 Dish=SelectedDish;
 auto Detail=T::Panel(Short(Dish)+TEXT(" / ANTENNA DETAIL"),S47Kit::Col({
  T::Value(TEXT("MODE"),TAttribute<FString>::CreateLambda([this,Dish]{return M->Web->DishRemoteAvailable(Dish)?FString(TEXT("REMOTE")):FString(TEXT("ISOLATED / LOCAL"));}),true),
  T::Value(TEXT("ACTUAL"),TAttribute<FString>::CreateLambda([this,Dish]{return FString::Printf(TEXT("AZ %.1f° EL %.1f°"),M->Web->DishAz(Dish),M->Web->DishEl(Dish));})),
  T::Value(TEXT("TARGET"),TAttribute<FString>::CreateLambda([this,Dish]{return FString::Printf(TEXT("AZ %.1f° EL %.1f°"),M->Web->DishTargetAz(Dish),M->Web->DishTargetEl(Dish));})),
  T::Value(TEXT("DRIVE"),TAttribute<FString>::CreateLambda([this,Dish]{return !M->Web->DishRemoteAvailable(Dish)?FString(TEXT("SERVICE")):FMath::Abs(FMath::FindDeltaAngleDegrees(M->Web->DishAz(Dish),M->Web->DishTargetAz(Dish)))>.1||FMath::Abs(M->Web->DishEl(Dish)-M->Web->DishTargetEl(Dish))>.1?FString(TEXT("MOVING")):FString(TEXT("READY"));}),true),
  T::Value(TEXT("RELAY"),TAttribute<FString>::CreateLambda([this,Dish]{return Field&&M->Web->DishId(Dish)==Field->ServiceDishId()&&Field->HasArrayFault()?FString(TEXT("FAULT")):FString(TEXT("OK"));}),true)}),true);
 TArray<S47Kit::FW> Service;
 if(Field)Service.Add(T::Text(TAttribute<FString>::CreateLambda([this]{return Field?Field->ServiceStatus():FString();}),TAttribute<int32>(0)));
 Service.Add(T::Big(FString(TEXT("INSPECT SERVICE RECORD")),[this]{OpenArchive();},2));
 TArray<S47Kit::FW> Rows;
 Rows.Add(T::Text(TAttribute<FString>::CreateLambda([this]{return ArrayMeasurementStatus();}),TAttribute<int32>(0)));
 Rows.Add(T::Columns({{.6f,Grid},{.4f,S47Kit::Col({Detail,T::Panel(TEXT("MAINTENANCE"),S47Kit::Col(Service),true)})}},12));
 OpenTerminal(0,TEXT("ARRAY STATUS / MAINTENANCE"),S47Kit::Col(Rows),true);
}
void FS47FreeformSession::OpenReceiver()
{
 if(!RequirePower()||!M||!M->UI||!M->Story||!CurrentJob())return;
 auto& R=M->Story->Rx;const auto P=EffectiveProfile(*CurrentJob());
 if(CurrentJob()->Stage!=EJobStage::New)R.Azimuth=TargetAz(); // No second, contradictory azimuth dial after physical tracking.
 const FString Name=FS47Observing::Runs()[CurrentJob()->Source].Source;
 static const TCHAR* TabNames[]={TEXT("ACQUIRE"),TEXT("TUNE"),TEXT("ARRAY"),TEXT("RECORD"),TEXT("DATA")};
 TArray<T::FNav> TabItems;for(int32 I=0;I<5;++I)TabItems.Add({TabNames[I],ReceiverTab==I,[this,I]{ReceiverTab=I;OpenReceiver();}});
 TArray<S47Kit::FW> Rows;Rows.Add(T::Tabs(TabItems));
 Rows.Add(T::Text(TAttribute<FString>::CreateLambda([this]{return ArrayMeasurementStatus();}),TAttribute<int32>(0)));
 Rows.Add(T::Small(TEXT("Carrier lock = tuning. Measurement rate = observing seconds.")));
 auto Quality=[this]{const auto* J=CurrentJob();return J?M->Story->Rx.Quality(EffectiveProfile(*J)):0.;};
 auto Controls=[this]{return T::Columns({
  {1.f,T::Control(TEXT("FREQUENCY"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.3f MHz"),M->Story->Rx.Frequency);}),1419.5f,1420.7f,.001f,TAttribute<float>::CreateLambda([this]{return float(M->Story->Rx.Frequency);}),[this](float V){M->Story->Rx.Frequency=V;})},
  {1.f,T::Control(TEXT("GAIN"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.0f"),M->Story->Rx.Gain);}),0,100,1,TAttribute<float>::CreateLambda([this]{return float(M->Story->Rx.Gain);}),[this](float V){M->Story->Rx.Gain=V;})},
  {1.f,T::Control(TEXT("BANDWIDTH"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.0f kHz"),M->Story->Rx.Bandwidth);}),1,100,1,TAttribute<float>::CreateLambda([this]{return float(M->Story->Rx.Bandwidth);}),[this](float V){M->Story->Rx.Bandwidth=V;})}},10);};
 auto Banner=[this]{return T::Banner(TAttribute<FString>::CreateLambda([this]{return ReceiverNextAction().ToUpper();}),TAttribute<bool>::CreateLambda([this]{return !ReceiverReady();}));};
 if(ReceiverTab==0)
 {
  Rows.Add(T::Panel(FString(),S47Kit::Col({T::TitleRow(TEXT("FREQUENCY SPECTRUM (MHz)"),FString::Printf(TEXT("SOURCE: %s  |  REQ GAIN: %.0f-%.0f  |  REQ BW: %.0f-%.0f kHz"),*Name,P.Gain[0],P.Gain[1],P.Bw[0],P.Bw[1])),
   T::Spectrum([this]{return M->Story->Rx;},[this]{return EffectiveProfile(*CurrentJob());},Quality)})));
  Rows.Add(Banner());
  Rows.Add(Controls());
  auto Start=T::Big(TAttribute<FString>::CreateLambda([this]{const auto* J=CurrentJob();return J&&J->Stage>=EJobStage::Chart?FString(TEXT("RECORDING RETAINED")):J&&J->Stage==EJobStage::Integrating?FString(TEXT("RECORDING...")):FString(TEXT("START RECORDING"));}),[this]{StartAcquisition();},1,TAttribute<bool>::CreateLambda([this]{return ReceiverReady();}));
  Rows.Add(T::Split({
   {1.f,T::Readout(TEXT("CARRIER LOCK"),TAttribute<FString>::CreateLambda([Quality]{return FString::Printf(TEXT("%d %%"),S47Signal::CarrierPercent(Quality()));}))},
   {1.f,T::Readout(TEXT("RECORDING"),TAttribute<FString>::CreateLambda([this]{const auto* J=CurrentJob();return FString::Printf(TEXT("%.0f / %.0f s"),J?J->Elapsed:0.,SourceSeconds());}))},
   {1.1f,Start}}));
 }
 else if(ReceiverTab==1)
 {
  Rows.Add(T::Panel(TEXT("TUNE / WORK CARD WINDOW"),S47Kit::Col({
   T::Value(TEXT("TARGET FREQUENCY"),FString::Printf(TEXT("%.3f MHz +/- %.3f"),P.F,P.FTol),true),
   T::Value(TEXT("GAIN WINDOW"),FString::Printf(TEXT("%.0f - %.0f"),P.Gain[0],P.Gain[1]),true),
   T::Value(TEXT("BANDWIDTH WINDOW"),FString::Printf(TEXT("%.0f - %.0f kHz"),P.Bw[0],P.Bw[1]),true),
   T::Value(TEXT("CARRIER LOCK"),TAttribute<FString>::CreateLambda([Quality]{return FString::Printf(TEXT("%d %%"),S47Signal::CarrierPercent(Quality()));}),true)})));
  Rows.Add(Banner());Rows.Add(Controls());
 }
 else if(ReceiverTab==2)
 {
   Rows.Add(T::Panel(TEXT("ARRAY / POINTING"),S47Kit::Col({
   T::Value(TEXT("TARGET"),FString::Printf(TEXT("%s / AZ %.1f EL %.1f"),*Name,TargetAz(),TargetEl()),true),
   T::Value(TEXT("ANTENNAS AVAILABLE"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%d / %d"),AvailableDishCount(),M->Web->DishCount());}),true),
   T::Value(TEXT("ENCODERS"),TAttribute<FString>::CreateLambda([this]{return PointReady()?Settle>=1?FString(TEXT("ON TARGET / STEADY")):FString(TEXT("ON TARGET / SETTLING")):FString(TEXT("NOT ON TARGET"));}),true),
   T::Small(TEXT("The array is commanded from the right-hand tracking PC. This receiver records only while pointing, tuning and power are all valid."))})));
  Rows.Add(Banner());
 }
 else if(ReceiverTab==3)
 {
  const FJob* J=CurrentJob();
  const bool ControlPanel=J->bAnomaly&&J->Stage==EJobStage::Recorded;
  TArray<S47Kit::FW> JobRows={
   T::Value(TEXT("JOB"),J->Id,true),T::Value(TEXT("STAGE"),TAttribute<FString>::CreateLambda([this]{return CurrentJob()?StageText(CurrentJob()->Stage):FString();}),true),
   T::Value(TEXT("RECORDED"),TAttribute<FString>::CreateLambda([this]{const auto* X=CurrentJob();return FString::Printf(TEXT("%.1f / %.0f s"),X?X->Elapsed:0.,SourceSeconds());}),true),
   T::Value(TEXT("ORIGINAL"),J->bPrinted?FString(TEXT("COLLECTED / IN NOTEBOOK")):J->Stage==EJobStage::Chart?FString(TEXT("WAITING AT PRINTER")):FString(TEXT("NOT YET RECORDED")),true)};
  if(!ControlPanel)JobRows.Add(T::Small(TEXT("Originals print at the table by the service-yard door. Valid recorded seconds are retained if tuning or pointing is lost.")));
  Rows.Add(T::Panel(TEXT("RECORD / THIS JOB"),S47Kit::Col(JobRows)));
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
   if(!Beam&&!J->bControl)Rows.Add(T::Big(FString(TEXT("RECORD INTERNAL CONTROL / 8 SECONDS")),[this]{if(StartControl())OpenReceiver();else Changed(NextAction());},1,true));
  }
  if(!ControlPanel||J->bControl||J->Phenomenon==TEXT("wrong_source"))Rows.Add(Banner());
 }
 else
 {
  Rows.Add(T::Panel(TEXT("DATA / WORK CARD"),S47Kit::Col({T::Paper(JobLine(*CurrentJob())),T::Paper(P.Card)})));
 }
 OpenTerminal(1,TEXT("RECEIVER / ACQUISITION"),S47Kit::Col(Rows),true);
}
void FS47FreeformSession::OpenAudio()
{
 if(!RequirePower()||!AudioDesk||!AudioDesk->Ready())return;
 if(!AudioDesk->ListeningPosition()){if(M&&M->UI)M->UI->Toast(TEXT("Recordings are compared at the reel deck on the wall table."),5);return;}
 bAudioOpen=true;
}
void FS47FreeformSession::OpenTapePanel()
{
 namespace T=S47FreeformTerminal;
 if(!M||!M->UI||!RequirePower()||!AudioDesk||!AudioDesk->Ready())return;
 if(!AudioDesk->ListeningPosition()){OpenAudio();return;}
 const T::FReelLook ReelLook;
 auto Press=[this](const TCHAR* Action){return [this,Action]{if(AudioDesk)AudioDesk->Use(FString(TEXT("ff:audio:"))+Action);};};
 auto CanCompare=TAttribute<bool>::CreateLambda([this]{const auto* J=CurrentJob();return J&&J->bAnomaly&&J->Stage>=EJobStage::Recorded&&J->bControl&&J->ControlAt>=8&&!Specialized(J->Phenomenon);});
 auto Marking=TAttribute<bool>::CreateLambda([this]{const auto* J=CurrentJob();return J&&J->bAnomaly&&J->Stage>=EJobStage::Recorded&&J->bControl&&J->ControlAt>=8&&!Specialized(J->Phenomenon)&&AudioSourceIndex==0;});
 auto Pick=[this](int32 Index){return TAttribute<bool>::CreateLambda([this,Index]{return AudioSourceIndex==Index;});};
 TArray<S47Kit::FW> Rows;
 auto Ready=[this]{const auto* J=CurrentJob();return J&&J->bAnomaly&&J->Stage>=EJobStage::Recorded&&J->bControl&&J->ControlAt>=8&&!Specialized(J->Phenomenon);};
 // Light strip first (concept 08): the one sentence of what to do, then the two recordings explained.
 Rows.Add(S47Kit::Banner(TEXT("WHAT TO DO HERE"),TEXT("Listen to the whole original and the whole control. Mark the extra ticks on the original, then retain."),
  TEXT("HERE you compare two recordings of the same job. ORIGINAL: what the antennas recorded. CONTROL: the station's own reference, antennas out of the path. If the odd sound is on both, the equipment makes it. If it is only on the original, it came from outside."),TEXT("COMPARE"),0));
  const S47Kit::FW Steps=T::Text(TAttribute<FString>::CreateLambda([this]
  {
   const auto* J=CurrentJob();
   if(!J||!J->bAnomaly||J->Stage<EJobStage::Recorded)return FString(TEXT("Nothing to compare yet. An unusual observation needs its original chart from the printer first."));
   if(!J->bControl||J->ControlAt<8)return FString(TEXT("The control is missing. Record it first: receiver (centre PC), RECORD tab."));
   if(Specialized(J->Phenomenon))return FString(TEXT("This observation is compared at OPS (corner PC), DAILY WORK, not on tape."));
   if(J->Stage>=EJobStage::Registered||J->bAudioReviewed)return FString::Printf(TEXT("JOB %s: comparison retained. Register the conclusion at OPS (corner PC)."),*J->Id);
   const bool Heard=J->OriginalCovered>=18,Marked=MarkCoversTicks(J->MarkStart,J->MarkEnd),Control=J->ControlCovered>=8;
   const int32 Now=!Heard?1:!Marked?2:!Control?3:4;
   auto Line=[Now](int32 N,const FString& Text){return FString::Printf(TEXT("%s %d  %s\n"),N<Now?TEXT("[X]"):N==Now?TEXT("[>]"):TEXT("[ ]"),N,*Text);};
   FString Mark=TEXT("MARK THE EXTRA TICKS ON THE ORIGINAL: click the trace where they begin, then where they end");
   if(J->MarkStart>=0&&J->MarkEnd>=0&&!Marked)Mark=TEXT("MARK AGAIN: the mark must run from before the first extra tick to after the last one");
   return FString::Printf(TEXT("JOB %s\n"),*J->Id)
    +Line(1,FString::Printf(TEXT("LISTEN TO THE WHOLE ORIGINAL: %.0f / 18 s heard"),J->OriginalCovered))
    +Line(2,Mark)
    +Line(3,FString::Printf(TEXT("LISTEN TO THE WHOLE CONTROL: %.0f / 8 s heard. Are the ticks there?"),J->ControlCovered))
    +Line(4,TEXT("PRESS RETAIN, then register the conclusion at OPS (corner PC)"));
  }),TAttribute<int32>(0));
 const auto Scope=SNew(SS47TapeScope)
  .Cursor_Lambda([this]{return AudioCursor;}).Length_Lambda([this]{return AudioSourceIndex==0?18.:8.;})
  .Control_Lambda([this]{return AudioSourceIndex!=0;}).Playing_Lambda([this]{return bAudioPlaying&&!CaptureStartStage;})
  .MarkIn_Lambda([this]{const auto* J=CurrentJob();return J&&AudioSourceIndex==0?J->MarkStart:-1.;}).MarkOut_Lambda([this]{const auto* J=CurrentJob();return J&&AudioSourceIndex==0?J->MarkEnd:-1.;})
  .Ticks_Lambda([this]{const auto* J=CurrentJob();return J&&AudioSourceIndex==0?J->TraceTicks:TArray<double>();})
  .CanMark(Marking).OnMark([this](double At){MarkAudioAt(At);});
 auto Mark=[this](bool bIn){return TAttribute<FString>::CreateLambda([this,bIn]{const auto* J=CurrentJob();const double At=!J?-1.:bIn?J->MarkStart:J->MarkEnd;return At<0?FString(TEXT("--")):FString::Printf(TEXT("%.1f s"),At);});};
 const S47Kit::FW Tape=T::Panel(TEXT("TAPE"),S47Kit::Col({
  T::Columns({{1.f,T::Toggle(TEXT("ORIGINAL 18 s"),Pick(0),Press(TEXT("original")),CanCompare)},{1.f,T::Toggle(TEXT("CONTROL 8 s"),Pick(1),Press(TEXT("control")),CanCompare)}}),
  T::Display(TEXT("POSITION"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%04.1f / %.0f s"),AudioCursor,AudioSourceIndex==0?18.:8.);}),
   TAttribute<FString>::CreateLambda([this]{return FString(CaptureStartStage?TEXT("LOADING"):bAudioPlaying?TEXT("PLAYING"):TEXT("STOPPED"));})),
  T::Heading(TEXT("LISTENED"),24),
  T::Meter(TEXT("ORIGINAL"),TAttribute<FString>::CreateLambda([this]{const auto* J=CurrentJob();return FString::Printf(TEXT("%.1f / 18 s"),J?J->OriginalCovered:0.);}),TAttribute<float>::CreateLambda([this]{const auto* J=CurrentJob();return J?float(FMath::Clamp(J->OriginalCovered/18.,0.,1.)):0.f;})),
  T::Meter(TEXT("CONTROL"),TAttribute<FString>::CreateLambda([this]{const auto* J=CurrentJob();return FString::Printf(TEXT("%.1f / 8 s"),J?J->ControlCovered:0.);}),TAttribute<float>::CreateLambda([this]{const auto* J=CurrentJob();return J?float(FMath::Clamp(J->ControlCovered/8.,0.,1.)):0.f;})),
  T::Split({{1.f,T::Text(FString(TEXT("MARKED ON ORIGINAL")),0)},{1.f,S47Kit::Col({T::Value(TEXT("IN"),Mark(true),true),T::Value(TEXT("OUT"),Mark(false),true)},0.f)}}),
  T::Text(TAttribute<FString>::CreateLambda([this]{const auto* J=CurrentJob();if(!J||J->MarkStart<0)return FString(TEXT("MARK: NOT SET"));if(J->MarkEnd<0)return FString(TEXT("MARK: the end is still missing"));return MarkCoversTicks(J->MarkStart,J->MarkEnd)?FString(TEXT("MARK COVERS THE TICKS")):FString(TEXT("MARK MISSES TICKS"));}),TAttribute<int32>(0)),
  T::Columns({{1.f,T::ButtonLive(TAttribute<FString>::CreateLambda([this]{return FString(bAudioPlaying||CaptureStartStage?TEXT("STOP"):TEXT("PLAY"));}),Press(TEXT("play")),true,CanCompare)},{1.f,T::Button(TEXT("REWIND"),Press(TEXT("rewind")),false,CanCompare)}}),
  T::Columns({{1.f,T::Button(TEXT("MARK IN"),Press(TEXT("mark_start")),false,Marking)},{1.f,T::Button(TEXT("MARK OUT"),Press(TEXT("mark_end")),false,Marking)},{1.f,T::Filled(TEXT("RETAIN"),Press(TEXT("retain")),TAttribute<bool>::CreateLambda([this,Ready]{const auto* J=CurrentJob();return Ready()&&J->Stage<EJobStage::Registered&&!J->bAudioReviewed&&J->OriginalCovered>=18&&J->ControlCovered>=8&&MarkCoversTicks(J->MarkStart,J->MarkEnd);}))}}),
  T::Text(TAttribute<FString>::CreateLambda([this]{const FString Status=AudioCaptureStatus();return Status.IsEmpty()?FeedbackText():Status;}),TAttribute<int32>(0))}));
 Rows.Add(T::Columns({{1.45f,S47Kit::Col({T::Panel(TEXT("SIGNAL / TAPE HEAD"),SNew(SBox).HeightOverride(340)[Scope]),T::Panel(TEXT("NEXT STEP"),Steps)})},{1.f,Tape}}));
 // The chip says what the references are for: a listening aid, never proof.
 Rows.Add(T::Panel(TEXT("LISTENING REFERENCES"),S47Kit::Col({
  T::Columns({{1.f,SNew(SBox).HAlign(HAlign_Left).VAlign(VAlign_Center)[S47Kit::Badge(TEXT("FOR YOUR EAR, NOT EVIDENCE"),-1)]},{1.4f,T::Text(TAttribute<FString>::CreateLambda([this]{return AudioDesk?AudioDesk->ReferenceLabel():FString();}),TAttribute<int32>(0))}}),
  T::Columns({{1.f,T::Button(TEXT("NEXT REFERENCE"),Press(TEXT("ref_next")),false,true)},{1.f,T::ButtonLive(TAttribute<FString>::CreateLambda([this]{return FString(AudioDesk&&AudioDesk->ReferencePlaying()?TEXT("STOP REFERENCE"):TEXT("PLAY REFERENCE"));}),Press(TEXT("ref_play")),false,true)}})})));
 auto Leave=[this]{if(M&&M->UI)M->UI->Close();};
 SetPanel(SNew(SS47TapePanel).OnLeave(Leave)[T::Frame(TEXT("REEL DECK / COMPARE RECORDINGS"),S47Kit::Col(Rows),T::Button(TEXT("ESC / RETURN TO ROOM"),Leave))],false,true);bTapePanel=true;
}
void FS47FreeformSession::OpenArchive()
{
 if(!M||!M->UI)return;const bool Computer=TerminalOnGlass();TArray<S47Kit::FW> Rows;
 auto Paper=[Computer](const FString& T){return Computer?S47FreeformTerminal::Paper(T):S47Kit::Paper(T);};
 auto Plain=[Computer](const FString& T){return Computer?S47FreeformTerminal::Small(T):S47Kit::Small(T);};
 // What needs doing comes first; the record itself is only read.
 if(!ArchiveNote.IsEmpty()){Rows.Add(Paper(ArchiveNote));ArchiveNote.Empty();}
 const FJob* Waiting=CurrentJob()&&CurrentJob()->Stage==EJobStage::Packet?CurrentJob():Work.FindByPredicate([](const FJob& X){return X.Stage==EJobStage::Packet;});
 if(Waiting)
 {
  if(NearBinder())
  {
   Rows.Add(Plain(FString::Printf(TEXT("TO FILE / The registered packet for %s is in your hands. Press the button to put it in this binder."),*Waiting->Id)));
   Rows.Add(S47Kit::Action(TEXT("FILE PACKET IN THIS BINDER"),[this,Id=Waiting->Id]
   {
    const bool Filed=FilePacket();UE_LOG(LogTemp,Display,TEXT("S47: binder filing %s: %s"),*Id,Filed?TEXT("filed"):*Feedback);
    ArchiveNote=Filed?FString::Printf(TEXT("FILED / %s is in the binder and the work is credited. Next: DAILY WORK at OPS (corner PC)."),*Id):Feedback;
    OpenArchive();
   }));
  }
  else Rows.Add(Paper(FString::Printf(TEXT("TO FILE / The registered packet for %s is waiting. Take it to the binder in the records room; this copy of the archive is read-only."),*Waiting->Id)));
 }
 static const TCHAR* Names[]={TEXT("The repeated interval"),TEXT("Visitors around the base"),TEXT("Disagreeing records")};
 TArray<FString> CaseLines;for(int32 I=0;I<3;++I)if(MysterySteps[I]>0)CaseLines.Add(FString::Printf(TEXT("%s %d/3%s"),Names[I],MysterySteps[I],MysterySteps[I]==3?TEXT(" READY TO COMPARE"):TEXT("")));
 if(!CaseLines.IsEmpty())Rows.Add(Plain(FString::Join(CaseLines,TEXT("   |   "))));
 if(const FString Gap=Predecessor?Predecessor->ArchiveLabel():FString();!Gap.IsEmpty())
 {
  const auto Open=[this]{if(Predecessor)Predecessor->OpenGap();};
  Rows.Add(Computer?S47FreeformTerminal::Button(Gap,Open):S47Kit::Button(Gap,Open));
 }
 struct FEntry{FString Key,Title,Tag,Detail;};
 TArray<FEntry> Tabs[3];
 for(const FJob& J:Work)if(J.bPrinted||!J.OriginalPage.IsEmpty())
 {
  const FString Control=J.bControl?FString::Printf(TEXT("INDEPENDENT CONTROL %.1f/8 s"),J.ControlAt):FString(TEXT("CONTROL NOT RECORDED"));
  const FString Mark=J.MarkStart>=0&&J.MarkEnd>J.MarkStart?FString::Printf(TEXT(" / MARK %.2f–%.2f s"),J.MarkStart,J.MarkEnd):FString();
  const FString Conclusion=!J.bAnomaly?TEXT("ROUTINE RESULT"):!J.bAudioReviewed?TEXT("COMPARISON PENDING"):J.Phenomenon==TEXT("wrong_source")?TEXT("OFF-SOURCE RESPONSE"):J.Phenomenon==TEXT("time_disagreement")?TEXT("CLOCK DISAGREEMENT"):J.Phenomenon==TEXT("duplicate_print")?TEXT("DUPLICATE ORIGINAL"):TEXT("SOURCE UNRESOLVED");
  Tabs[0].Add({J.Id,FString::Printf(TEXT("%s / ORIGINAL %s-original"),*J.Id,*J.Id),Conclusion+TEXT(" / ")+StageText(J.Stage),FString::Printf(TEXT("%s%s\n%s / %s"),*Control,*Mark,*Conclusion,*StageText(J.Stage))});
 }
 TArray<FString> Keys;Findings.GetKeys(Keys);Keys.Sort([this](const FString& A,const FString& B){const int32 X=FindingDays.FindRef(A),Y=FindingDays.FindRef(B);return X!=Y?X>Y:A<B;});
 for(const FString& K:Keys)
 {
  const bool Investigate=K.StartsWith(TEXT("extra_pulse"))||K.StartsWith(TEXT("feeder_fault"))||K.StartsWith(TEXT("wrong_source"))||K.StartsWith(TEXT("roof_steps"))||K.StartsWith(TEXT("observer"))||K.StartsWith(TEXT("peek-"))||K.StartsWith(TEXT("flood-room-"))||K.StartsWith(TEXT("door-visit-"))||K.StartsWith(TEXT("door-marks-"))||K.StartsWith(TEXT("landing_trace"))||K.StartsWith(TEXT("time_disagreement"))||K.StartsWith(TEXT("duplicate_print"))||K.StartsWith(TEXT("carrier_"));
  const FString Record=Findings.FindRef(K);const bool Bench=K.StartsWith(TEXT("service"))||K.StartsWith(TEXT("workshop"))||K.StartsWith(TEXT("donor"))||K.StartsWith(TEXT("feeder-repair"))||K.StartsWith(TEXT("sample"));
  // The record's own first sentence names it; internal keys stay internal.
  int32 Stop=INDEX_NONE;Record.FindChar(TEXT('.'),Stop);FString Title=Stop>0?Record.Left(Stop):Record;if(Title.Len()>58)Title=Title.Left(55)+TEXT("...");
  FString Entry=FString::Printf(TEXT("DAY %d\nRETAINED RECORD / %s"),FindingDays.FindRef(K),*Record);
  if(Investigate)Entry+=TEXT("\nSTILL UNKNOWN / Whether this repeats under independent conditions.\nPOSSIBLE NEXT CHECK / Retain a later observation and its independent control; compare with this original.");
  Tabs[Bench?2:1].Add({K,Title,FString::Printf(TEXT("DAY %d"),FindingDays.FindRef(K)),Entry});
 }
 if(Tabs[0].IsEmpty()&&Tabs[1].IsEmpty()&&Tabs[2].IsEmpty())
  Rows.Add(Paper(TEXT("NO RETAINED RECORDS YET\nSelect DAILY WORK at OPS, complete an observation and collect its original at the printer. Retained originals and field findings appear here automatically. Register routine results at OPS; an anomaly packet must also be collected and placed in the records-room binder.")));
 else
 {
  if(Tabs[FMath::Clamp(ArchiveTab,0,2)].IsEmpty())ArchiveTab=!Tabs[0].IsEmpty()?0:!Tabs[1].IsEmpty()?1:2;
  static const TCHAR* TabNames[]={TEXT("ORIGINALS"),TEXT("FIELD FINDINGS"),TEXT("SERVICE AND BENCH")};
  const auto& List=Tabs[ArchiveTab];const FEntry* Pick=List.FindByPredicate([&](const FEntry& E){return E.Key==ArchivePick;});if(!Pick)Pick=&List[0];
  TSharedRef<SScrollBox> Scroll=SNew(SScrollBox);
  if(Computer)
  {
   TArray<T::FNav> Bar;for(int32 I=0;I<3;++I)Bar.Add({FString::Printf(TEXT("%s %d"),TabNames[I],Tabs[I].Num()),I==ArchiveTab,[this,I]{ArchiveTab=I;ArchivePick.Empty();OpenArchive();}});
   Rows.Add(T::Tabs(Bar));
   TArray<T::FRow> Lines;for(const auto& E:List)Lines.Add({{E.Title,E.Tag},&E==Pick,[this,Key=E.Key]{ArchivePick=Key;OpenArchive();},FString()});
   Scroll->AddSlot()[T::Table({TEXT("RECORD"),TEXT("STATE")},{.56f,.44f},Lines)];
  }
  else
  {
   TArray<S47Kit::FW> Bar;for(int32 I=0;I<3;++I)Bar.Add(S47Kit::Choice(FString::Printf(TEXT("%s %d"),TabNames[I],Tabs[I].Num()),I==ArchiveTab,[this,I]{ArchiveTab=I;ArchivePick.Empty();OpenArchive();}));
   Rows.Add(S47Kit::Row(Bar));
   for(const auto& E:List)Scroll->AddSlot()[S47Kit::Choice(E.Title,&E==Pick,[this,Key=E.Key]{ArchivePick=Key;OpenArchive();},false,E.Tag)];
  }
  Rows.Add(SNew(SBox).HeightOverride(210).VAlign(VAlign_Top)[Scroll]);
  Rows.Add(Paper(Pick->Detail));
 }
 if(Computer)OpenTerminal(3,TEXT("OPS / CASE ARCHIVE"),OpsShell(3,S47Kit::Col(Rows)));
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
  case ECaseCondition::Generator:return Field&&Field->GeneratorRunning()?FString():FString(TEXT("SERVICE BAY: START THE FUELLED BACKUP GENERATOR; KEEP THE WHOLE CASE TEST RECORDING ON GENERATOR POWER, OFF THE MAINS"));
  case ECaseCondition::Repeat:return Cases.InRepeatWindow(J.CaseLine,DayNumber,FMath::Fmod(WorldClock,86400.))?FString():TEXT("OPS (corner PC): CHECK THE CASE BOARD; WAIT FOR THE DATED REPEAT WINDOW. ")+CaseWindowText(J.CaseLine);
  case ECaseCondition::DoorsSealed:return StationSealed()?FString():FString(TEXT("SARO CONTROL ROOM: CLOSE BOTH OUTSIDE DOORS AND STAY INSIDE THROUGH THE WHOLE CASE TEST RECORDING"));
  case ECaseCondition::RelaySwap:return InventoryCount(TEXT("relay"))>0||J.Stage>EJobStage::Pointed?FString():FString(TEXT("CALICHE GAS STATION: BUY A RELAY FOR THE TIMING SWAP; THE SARO DEPOT HAS NONE"));
  default:return FString();
 }
}
int32 FS47FreeformSession::CaseAwaitingChoice()const
{
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)if(const auto* C=Cases.Get(Line);C&&C->bOpen&&!C->bConcluded&&C->Planned<0)return Line;
 return -1;
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
 if(Kind==ECaseCondition::FieldWork)return Line==2&&CaseField&&CaseField->Window(Start,End);
 if(Kind!=ECaseCondition::Repeat)return false;
 // Case days, like OPS, begin at 08:00. Clamp a midnight-crossing interval to
 // the first eligible shift; this is the same condition as InRepeatWindow.
 const double Earliest=C->OpenDay*SecondsPerDay+8*3600.;
 const double Base=FMath::FloorToDouble(WorldClock/SecondsPerDay)*SecondsPerDay;
 for(int32 Offset=-1;Offset<=3;++Offset)
 {
  const double Centre=Base+Offset*SecondsPerDay+C->OpenClock;
  const double A=FMath::Max(Earliest,Centre-FS47FreeformCases::RepeatWindow),B=Centre+FS47FreeformCases::RepeatWindow;
  if(A<=B&&B>WorldClock){Start=A;End=B;return true;}
 }
 return false;
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
 FString Text=FString::Printf(TEXT("WAIT %.2f GAME HOURS / UNTIL SHIFT DAY %02d, %s\nNo observing, listening, printing or physical test work is performed during this interval. Unfinished work and original records stay intact."),Hours,Day,*FS47FreeformCases::Clock(Target));
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
 // Behind a motel door nothing does. On a cot out on the land, on any night with anything abroad, it will.
 if(RestPlace==ERest::Motel)return false;
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
 WorldClock=bWoken?WokenAt:Target;DayNumber=FMath::Max(1,int32(FMath::FloorToDouble((WorldClock-8*3600.)/SecondsPerDay))+1);AddDay(DayNumber);Settle=0;bAudioPlaying=false;
 // Settle unsampled time before any save request, even while the confirmation
 // screen is still modal. No large Dt is passed through observing work.
 if(Field)Field->AdvanceUnobservedTo(WorldClock);
 if(CaseField)CaseField->AdvanceUnobservedTo(WorldClock);
 if(M&&M->Story)M->Story->SetClock(FMath::Fmod(WorldClock,SecondsPerDay));
 if(M&&M->UI)M->UI->Close();
 if(bWoken)
 {
  WakeClock=WorldClock;
  // A call on the set about the marks if there are marks, otherwise only a sound outside that ends the sleep.
  const bool bCalled=FarmSite&&FarmSite->WakeCall();
  if(!bCalled&&M&&M->Audio&&M->Player)M->Audio->Sfx(TEXT("alienRadio"),FS47Sound{.Gain=.45f,.Rate=.8f,.At=FVector(M->Player->Pos.X-6,M->Player->FloorY+2,M->Player->Pos.Z-9)});
  const int32 Secs=int32(FMath::Fmod(WorldClock,SecondsPerDay));
  Feedback=RestPlace==ERest::Field?FString::Printf(TEXT("%02d:%02d. Something is walking round the truck."),Secs/3600,(Secs/60)%60):FString::Printf(TEXT("%02d:%02d. Something woke you. Wait again when you are ready to go on."),Secs/3600,(Secs/60)%60);
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
 if(Test<0)return TEXT("OPS / CASE BOARD: REVIEW THE RETAINED FIELD RESULTS AND CHOOSE THE NEXT CONTROL. AN INCOMPLETE TEST ESTABLISHES NO PRESENCE OR ABSENCE.");
 const FString Unavailable=FS47FreeformCases::UnavailableReason(Line,Test,HasKit(TEXT("logger2")));
 if(!Unavailable.IsEmpty())return Unavailable+TEXT(" Cancel this retained plan to choose another control.");
 if(Line==2&&CaseField){const FString Actual=CaseField->Objective();if(!Actual.IsEmpty())return Actual;}
 const auto& T=FS47FreeformCases::Def(Line).Tests[Test];
 return TEXT("FIELD TEST / ")+FString(T.Name).ToUpper()+TEXT(": ")+T.Instruction;
}
bool FS47FreeformSession::SetCaseHunch(int32 Line,int32 Cause)
{
 if(!Cases.SetHunch(Line,Cause))return false;
 Changed(TEXT("Working hunch noted. This is not evidence and grants no result or reward."));return true;
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
 return FString::Printf(TEXT("STATION GOAL / ARRAY %d OF 27: REPAIR THE %s AT ANTENNA %s. %s"),27-Field->ArrayWear().DownCount(),Next->Title,Next->Junction,Missing.IsEmpty()?TEXT("YOU HAVE THE PARTS."):*(TEXT("NEEDS ")+Missing.ToUpper()+TEXT(".")));
}
bool FS47FreeformSession::EveningGated(const FJob& J)const
{
 // From day two every card is the evening's, and one not begun by half past eleven waits for the next evening. Day one is the
 // first shift's. Once begun a card runs out whenever it was begun, and a case test keeps its own window.
 if(!J.bGenerated||J.Stage!=EJobStage::New||J.Day<2||J.CaseTest>=0||Tutorial>0)return false;
 if(M&&M->Flow&&M->Flow->bDevRun&&!bTestEvening)return false;
 const double Now=FMath::Fmod(WorldClock,SecondsPerDay);
 return Now<EveningUp(J)||Now>=EveningCloses;
}
const FS47FreeformSession::FJob* FS47FreeformSession::NextEveningCard(double& Up)const
{
 const FJob* Best=nullptr;Up=0;const double Base=FMath::FloorToDouble(WorldClock/SecondsPerDay)*SecondsPerDay;
 for(const FJob& J:Work)if(EveningGated(J))
 {
  double At=Base+EveningUp(J);if(At<=WorldClock)At+=SecondsPerDay;
  if(!Best||At<Up){Best=&J;Up=At;}
 }
 return Best;
}
int32 FS47FreeformSession::JobPay(const FJob& J)const
{
 if(!RulesLive())return 20;
 const int32 Pay=PayAfterDays(FMath::Max(0,DayNumber-J.Day));
 return J.bDocked?FMath::Max(5,(Pay/10)*5):Pay;
}
FString FS47FreeformSession::NextReviewText()const
{
 const FWeekCosts X=WeekCosts(ReviewWeek+1);
 return FString::Printf(TEXT("DAY %d / $%d SO FAR"),NextReviewDay(),ReviewPower+ReviewFuel+X.Total()+Carried);
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
  const int32 Before=Carried,Bill=ReviewPower+ReviewFuel+X.Total()+Before;
  const int32 Paid=FMath::Min(Cash,Bill);
  Cash-=Paid;Carried=FMath::Min(ArrearsCap,Bill-Paid);
  if(Care)Care->WeekBilled();
  FString Running=FString::Printf(TEXT("\nTRUCK: TYRES, OIL AND WEAR, %.0f KM $%d\nFILM AND DARKROOM CHEMICALS, %d %s $%d"),X.Km,X.Wear,X.Prints,X.Prints==1?TEXT("PRINT"):TEXT("PRINTS"),X.Film);
  if(X.Poor>0)Running+=FString::Printf(TEXT("\nDR. WARD: RE-CHECK OF %d %s NOBODY CAN READ $%d. Tune before you record."),X.Poor,X.Poor==1?TEXT("RECORDING"):TEXT("RECORDINGS"),X.Fee);
  RecordEvidence(FString::Printf(TEXT("week-review-%d"),ReviewWeek),FString::Printf(TEXT("SARO OPERATIONS / WEEKLY REVIEW / WEEK %d\nCOUNTY LINE, POWER $%d\nFUEL AND STORES ACCOUNT $%d%s\nCARRIED FROM LAST REVIEW $%d\nPAID FROM THE BUDGET $%d\nCARRIED TO THE NEXT REVIEW $%d\nWhat is carried stays on this sheet. It does not touch the requisition and nothing is cut off.%s"),ReviewWeek,ReviewPower,ReviewFuel,*Running,Before,Paid,Carried,Predecessor?*Predecessor->ReviewLine():TEXT("")));
  Changed(FString::Printf(TEXT("Weekly review: $%d paid%s. Next one day %d."),Paid,Carried>0?*FString::Printf(TEXT(", $%d carried"),Carried):TEXT(""),NextReviewDay()));
  UE_LOG(LogTemp,Display,TEXT("S47: weekly review %d: bill %d, paid %d, carried %d, cash %d"),ReviewWeek,Bill,Paid,Carried,Cash);
 }
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
   const double Open=Base+Offset*SecondsPerDay+EveningUp(J),Shut=Base+Offset*SecondsPerDay+EveningCloses;
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
  :Id==TEXT("alarm")?SheriffTrust()>0:false;
 return bOpen?FString():FString(O->Waits);
}
bool FS47FreeformSession::FieldRecordKept()const
{
 for(const auto& Kept:Findings)
 {
  const FString& K=Kept.Key;
  if((K.StartsWith(TEXT("place_"))&&!K.EndsWith(TEXT("_needle")))||K.StartsWith(TEXT("farm_trace"))||K.StartsWith(TEXT("lights_trace_"))||K.StartsWith(TEXT("field_night")))return true;
 }
 return false;
}
int32 FS47FreeformSession::KitPrice(const FString& Id)const
{
 const auto* O=FS47Openings::Def(Id);if(!O)return 0;
 if(Id==TEXT("magnetometer")&&Cases.HasAccess(TEXT("access.lostHours")))return FMath::Max(5,O->Price-LostHoursDiscount);
 return O->Price;
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
bool FS47FreeformSession::RestAt(double Target,ERest Where)
{
 if(Where==ERest::Motel&&Cash<MotelRoom)return false;
 RestPlace=Where;
 const bool bGood=WaitUntil(Target);
 if(bGood&&Where==ERest::Motel){Cash-=MotelRoom;Changed(FString::Printf(TEXT("A night at the Sierra Motor Court, $%d. Nothing came through the door."),MotelRoom));}
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
 return FString::Printf(TEXT("EVENING OBSERVATION / %s IS UP FROM %s. UNTIL THEN: DAY WORK, REPAIRS, SUPPLIES. OPS CAN WAIT FOR IT."),*FString(FS47Observing::Runs()[J.Source].Source).ToUpper(),*FS47FreeformCases::Clock(EveningUp(J)));
}
FString FS47FreeformSession::AvailableActivities()const
{
 int32 Pending=0;const FJob* Evening=nullptr;
 for(const FJob& J:Work)if(J.Stage!=EJobStage::Filed&&!J.bDuplicateRejected){if(!EveningGated(J))++Pending;else if(!Evening)Evening=&J;}
 // The array is what the whole station is working toward: whatever else is on the line, it stays in sight.
 const FString Goal=StationGoalLine();
 const auto* Next=Field&&Field->ArrayWear().bActive?Field->ArrayWear().NextBranch():nullptr;
 const FString Tail=Next?FString::Printf(TEXT(" STATION GOAL: ARRAY %d OF 27."),27-Field->ArrayWear().DownCount()):FString();
 if(Pending>0)return FString::Printf(TEXT("OPS (corner PC), DAILY WORK: SELECT ONE OF %d UNFINISHED OBSERVATIONS; TRACK, TUNE AND REGISTER THE NEXT RESULT. COMPLETED RESULTS ARE IN EARLIER WORK."),Pending)+Tail;
 // With the parts in hand the station's own goal is the next thing to do, whatever the budget or a case says.
 if(Next&&Field->BranchNeeds(*Next).IsEmpty())return Goal;
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)if(const auto* C=Cases.Get(Line);C&&C->bOpen&&!C->bConcluded)
  return (C->Planned>=0?TEXT("OPS / CASE BOARD: FOLLOW THE RETAINED PLAN FOR ")+FString(FS47FreeformCases::Def(Line).Title):TEXT("OPS / CASE BOARD: CHOOSE A CONTROL FOR ")+FString(FS47FreeformCases::Def(Line).Title))+TEXT(".")+Tail;
 if(M&&M->Night&&M->Night->Album&&M->Night->Album->PendingLabPhoto())return FString(TEXT("PHOTO LAB: DEVELOP, FIX AND COLLECT YOUR EXPOSED PHOTOGRAPH AT THE WET BENCH."))+Tail;
 if(ServiceGuide==1)return ServiceObjective();
 if(Gear&&Gear->AnyLeft())return Gear->TaskLine()+Tail;
 // An idle loan logger is an option, not the next thing to do: it never again stands in front of the goal (T16-17, 10).
 if(Field&&Field->LoggerAccess()&&(Field->FieldLogger().Pressing()||(Goal.IsEmpty()&&!Evening)))return Field->FieldLogger().NextAction();
 if(Evening)return Goal.IsEmpty()?EveningLine(*Evening):EveningLine(*Evening)+TEXT(" ")+Goal;
 // Said only when it is true: what stops a rest says so itself.
 const bool bCanRest=!Director||Director->RestBlock().IsEmpty();
 // The reference check pays nothing: a player short of money needs to hear where paid work comes from.
 if(Cash<5||Loan>0)return bCanRest?TEXT("OPS: TODAY'S PAID WORK IS DONE. REST UNTIL MORNING FOR NEW PAID OBSERVATIONS; THEY CLEAR ANY REQUISITION BALANCE FIRST."):TEXT("OPS: TODAY'S PAID WORK IS DONE. NEW PAID OBSERVATIONS COME WITH THE MORNING; THEY CLEAR ANY REQUISITION BALANCE FIRST.");
 if(!Goal.IsEmpty())return Goal;
 return bCanRest?TEXT("OPS / DAILY WORK: START AN OPTIONAL REFERENCE CHECK TO RETAIN A FRESH INSTRUMENT BASELINE (NO PAYMENT). PHOTO LAB AND ARRAY SERVICE ARE ALSO AVAILABLE; REST IS OPTIONAL."):TEXT("OPS / DAILY WORK: START AN OPTIONAL REFERENCE CHECK TO RETAIN A FRESH INSTRUMENT BASELINE (NO PAYMENT). PHOTO LAB AND ARRAY SERVICE ARE ALSO AVAILABLE.");
}
bool FS47FreeformSession::StartReferenceCheck()
{
 if(!bActive||Tutorial>0||!RequirePower()||Work.ContainsByPredicate([this](const FJob& J){return J.Stage!=EJobStage::Filed&&!J.bDuplicateRejected&&!EveningGated(J);}))return false;
 FJob J;J.Id=FString::Printf(TEXT("FF-D%04d-REFERENCE-%04d"),DayNumber,Work.Num());J.Day=DayNumber;J.Slot=0;J.Source=0;
 J.Card=FS47FreeformJobs::Make(JobSeed,DayNumber,0,0);J.bGenerated=true;J.bPaid=true;
 J.Card.Receiver.Card=TEXT("OPTIONAL REFERENCE CHECK / NO PAYMENT\nRetain a fresh instrument baseline using the usual tracking cross and receiver. This check creates no incident or cause by itself.\n\n")+J.Card.Receiver.Card;
 Work.Add(J);Selected=Work.Num()-1;FieldFocus=-1;bGuideFocus=false;Settle=0;bCrossRunning=false;bAudioPlaying=false;AudioCursor=0;
 Changed(TEXT("TRACKING (right-hand PC): POINT TO 3C 286 FOR YOUR OPTIONAL REFERENCE CHECK. KEEP THE ORIGINAL AS AN INSTRUMENT BASELINE; NO EXTRA PAYMENT IS DUE."));return true;
}
bool FS47FreeformSession::CancelFieldTest(int32 Line)
{
 const int32 Test=PlannedFieldTest(Line);const auto* C=Cases.Get(Line);
 if(Test<0||!C||C->PlannedJob.IsEmpty()||C->Results[Test].bDone)return false;
 Cases.CancelPlan(Line);if(Line==2&&CaseField)CaseField->ResetPlan();if(FieldFocus==Line)FieldFocus=-1;
 Changed(TEXT("OPS / CASE BOARD: Field plan cleared, no result recorded. You may plan another field test."));return true;
}
bool FS47FreeformSession::FieldTestExpectsPresent(int32 Line,int32 Test)const
{
 // Only the planned field test is answered, so world code cannot read the hidden cause freely.
 const auto* C=Cases.Get(Line);return C&&PlannedFieldTest(Line)==Test&&FS47FreeformCases::PresentFor(Line,C->Truth,Test);
}
bool FS47FreeformSession::ReportFieldTest(int32 Line,int32 Test,bool bValid,bool bPresent,const FString& Evidence)
{
 if(PlannedFieldTest(Line)!=Test)return false;bool First=false;
 if(!Cases.Record(Line,Test,DayNumber,bValid,bPresent,First))return false;
 const auto& D=FS47FreeformCases::Def(Line);
 if(!bValid){Changed(TEXT("Field test INVALID: ")+Evidence.Left(160)+TEXT(" Plan it again on the case board."));return true;}
 TArray<FString> Out;for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)if(FS47FreeformCases::PresentFor(Line,I,Test)!=bPresent)Out.Add(D.Causes[I]);
 const FString Result=FS47FreeformCases::TestResultText(Line,Test,bPresent);
 RecordEvidence(FString::Printf(TEXT("case%d-test%d"),Line+1,Test+1),FString::Printf(TEXT("CASE / %s\nFIELD TEST / %s / DAY %d\nRESULT / %s\nPREDICTION / %s\nRULES OUT / %s\nRECORD / %s"),D.Title,*FString(D.Tests[Test].Name).ToUpper(),DayNumber,*Result,*FS47FreeformCases::Prediction(Line,Test),*FString::Join(Out,TEXT(", ")),*Evidence.Left(400)));
 FString Text=FString::Printf(TEXT("Field test retained. %s: rules out %s."),*Result,*FString::Join(Out,TEXT(" and ")));
 if(First){AddItem(TEXT("relay"),1);Text+=TEXT(" Station management credits one service relay to the depot for documented test work.");}
 if(Cases.Remaining(Line)==1)Text+=TEXT(" Only one explanation survives: register the conclusion on the case board.");
 Changed(Text);return true;
}
void FS47FreeformSession::OpenCaseFor(const FString& FindingId)
{
 const int32 Line=FS47FreeformCases::LineForFinding(FindingId);
 // Whether the case is new or not, the wind no longer covers it.
 const bool bBeyondWind=FindingId.StartsWith(TEXT("taken-"))||FindingId.StartsWith(TEXT("flood-room-"));
 if(bBeyondWind)Cases.RuleOutWind(Line);
 if(Line<0||!Cases.Open(Line,DayNumber,FMath::Fmod(WorldClock,86400.)))return;
 Changed(TEXT("New case opened: ")+FString(FS47FreeformCases::Def(Line).Title)+(bBeyondWind?TEXT(". This was not the wind. Two explanations are left. Choose a test on the OPS case board."):TEXT(". Three explanations fit. Choose a test on the OPS case board.")));
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
  if(Cases.TrailCold(Line,DayNumber)){Feedback=TEXT("The trail has gone cold: a fresh control would no longer show anything about that night. You can still file your assessment.");Changed(Feedback);return false;}
  if(Def.Condition==ECaseCondition::FieldWork)
  {
   const FString Id=FString::Printf(TEXT("FIELD-D%04d-CASE%d-T%d"),DayNumber,Line+1,Test+1);
   if(!Cases.Plan(Line,Test,Id))return false;
   if(Line==2&&CaseField)CaseField->ResetPlan();
   const bool Focused=FocusFieldTest(Line);
   Feedback=Focused?TEXT("Field test planned. ")+FieldNextAction(Line):TEXT("Field plan retained. Finish the active recording or control, then choose Follow this field test on CASE BOARD.");Changed(Feedback);return true;
  }
 }
 FJob J;J.Id=FString::Printf(TEXT("FF-D%04d-CASE%d-T%d-%d"),DayNumber,Line+1,Test+1,Work.Num());J.Day=DayNumber;J.Slot=1;J.Source=1;
 J.Card=FS47FreeformJobs::Make(JobSeed,DayNumber,1,1);J.bGenerated=true;J.CaseLine=Line;J.CaseTest=Test;
 const auto& T=FS47FreeformCases::Def(Line).Tests[Test];
 J.Card.Receiver.Card=TEXT("CASE TEST / ")+FString(T.Name).ToUpper()+TEXT("\n")+T.Instruction+TEXT("\n\n")+J.Card.Receiver.Card;
 if(!Cases.Plan(Line,Test,J.Id))return false;
 FieldFocus=-1;bGuideFocus=false;
 Work.Add(J);
 const FJob* Current=CurrentJob();if(!Current||Current->Stage==EJobStage::New||Current->Stage==EJobStage::Filed)Selected=Work.Num()-1;
 Feedback=TEXT("Case test planned: ")+FString(T.Name)+TEXT(". Job ")+J.Id+TEXT(" is on the OPS list.");Changed(Feedback);return true;
}
void FS47FreeformSession::RecordCaseResult(FJob& J)
{
 J.bCaseRecorded=true;const int32 Line=J.CaseLine,Test=J.CaseTest;const bool bPresent=CaseObserved(J);bool First=false;
 if(!Cases.Record(Line,Test,DayNumber,J.bCaseValid,bPresent,First)){Changed(FString::Printf(TEXT("Routine result registered. $%d credited; the original remains in the notebook."),LastPay));return;}
 if(!J.bCaseValid){Changed(FString::Printf(TEXT("Observation registered, $%d credited. The case test is INVALID because its condition was broken; plan it again on the case board."),LastPay));return;}
 const auto& D=FS47FreeformCases::Def(Line);
 TArray<FString> Out;for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)if(FS47FreeformCases::PresentFor(Line,I,Test)!=bPresent)Out.Add(D.Causes[I]);
 const FString Result=FS47FreeformCases::ResultText(Line,bPresent);
 RecordEvidence(FString::Printf(TEXT("case%d-test%d"),Line+1,Test+1),FString::Printf(TEXT("CASE / %s\nTEST / %s / DAY %d / ORIGINAL %s-original\nRESULT / %s\nPREDICTION / %s\nRULES OUT / %s"),D.Title,*FString(D.Tests[Test].Name).ToUpper(),DayNumber,*J.Id,*Result,*FS47FreeformCases::Prediction(Line,Test),*FString::Join(Out,TEXT(", "))));
 FString Text=FString::Printf(TEXT("Case test registered, $%d credited. %s: rules out %s."),LastPay,*Result,*FString::Join(Out,TEXT(" and ")));
 if(First){AddItem(TEXT("relay"),1);Text+=TEXT(" Station management credits one service relay to the depot for documented test work.");}
 if(Cases.Remaining(Line)==1)Text+=TEXT(" Only one explanation survives: register the conclusion on the case board.");
 Changed(Text);
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
  if(M&&M->Story)
  {
   FS47Doc Doc;Doc.Id=FString::Printf(TEXT("freeform-case%d-reply"),Line+1);Doc.Title=Y->ReplyTitle;Doc.Kind=TEXT("telex");Doc.Stamp=FString::Printf(TEXT("SARO TELEX / DAY %d"),DayNumber);
   Doc.Page=Page;Doc.Transcript=Page;M->Story->AddDoc(Doc);
  }
  FString Note=FString(Y->ReplyTitle)+TEXT(" has come in at the telex.");
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
 return Trust;
}
bool FS47FreeformSession::DeputyOnRoute() const{return SheriffTrust()>0;}
int32 FS47FreeformSession::TownTrust() const
{
 int32 Seen=0;for(const auto& Kept:Findings)Seen+=Kept.Key.StartsWith(TEXT("place_"))&&!Kept.Key.EndsWith(TEXT("_needle"))?1:0;
 return SheriffTrust()+(Seen>=4?1:0);
}
bool FS47FreeformSession::ConcludeCase(int32 Line,int32 Cause)
{
 FString Why;if(!Cases.Conclude(Line,Cause,Why,DayNumber)){Feedback=Why;Changed(Why);return false;}
 if(FieldFocus==Line)FieldFocus=-1;
 const auto& D=FS47FreeformCases::Def(Line);const FString Name=D.Causes[Cause];
 if(D.Mystery)
 {
  // A test recording not yet started no longer tests anything: it goes back to being a day's ordinary paid work. One
  // already under way runs out as routine, since the case no longer has a plan for it to report to.
  for(FJob& J:Work)if(J.CaseLine==Line&&!J.bCaseRecorded&&J.Stage<=EJobStage::Pointed)
  {J.CaseLine=-1;J.CaseTest=-1;J.Stage=EJobStage::New;J.Elapsed=0;J.Card=FS47FreeformJobs::Make(JobSeed,J.Day,J.Slot,J.Source);}
  Settle=0;
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
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)
 {
  const auto* C=Cases.Get(Line);if(!C||!C->bOpen||C->bConcluded)continue;const auto& D=FS47FreeformCases::Def(Line);
  if(C->Planned<0){Out.Add(FString::Printf(TEXT("CASE / %s / NO TEST PLANNED\n%d of 3 explanations still possible. %s"),D.Title,Cases.Remaining(Line),D.Mystery?TEXT("Open the case board: choose a control or file your assessment."):TEXT("Open the case board and choose the next test.")));continue;}
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
   case ECaseCondition::Repeat:
    Plan+=TEXT("\nREADINESS / TRACKING AND RECEIVER: repeat the retained source during the dated window below, on a later shift.");break;
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
void FS47FreeformSession::OpenCaseBoard(int32 Line)
{
 if(!RequirePower()||!M||!M->UI)return;
 if(Line<0){Line=CaseAwaitingChoice();if(Line<0)for(int32 I=0;I<FS47FreeformCases::CaseCount;++I)if(Cases.Get(I)&&Cases.Get(I)->bOpen&&!Cases.Get(I)->bConcluded){Line=I;break;}if(Line<0)Line=0;}
 TArray<S47Kit::FW> Rows;const auto* C=Cases.Get(Line);const auto& D=FS47FreeformCases::Def(Line);
 TArray<S47Kit::FW> Tabs;
 for(int32 I=0;I<FS47FreeformCases::CaseCount;++I)if(const auto* X=Cases.Get(I);X&&X->bOpen&&I!=Line)Tabs.Add(S47FreeformTerminal::Button(FString::Printf(TEXT("Case %d: %s"),I+1,*FString(FS47FreeformCases::Def(I).Title).ToLower()),[this,I]{Feedback.Empty();OpenCaseBoard(I);},X->Planned<0&&!X->bConcluded));
 if(!Tabs.IsEmpty())Rows.Add(S47Kit::Row(Tabs));
 if(!C||!C->bOpen)
 {
  Rows.Add(S47FreeformTerminal::Paper(TEXT("OPS / CASE BOARD\nNo open case. Review a discrepancy against its independent control; a retained comparison opens a case here.")));
 }
 else
 {
  Rows.Add(S47FreeformTerminal::Paper(FString::Printf(TEXT("CASE %d / %s\nOPENED DAY %d FROM THE RETAINED ORIGINAL AT %s\n%s"),Line+1,D.Title,C->OpenDay,*FS47FreeformCases::Clock(C->OpenClock),*Cases.QuestionText(Line))));
  for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)
  {
   const bool Out=Cases.Excluded(Line,I);
   const FString State=C->bConcluded&&D.Mystery?(C->Conclusion==I?TEXT("FILED AS YOUR ASSESSMENT"):Out?TEXT("RULED OUT"):TEXT("NOT FILED")):C->bConcluded?(C->Conclusion==I?TEXT("CONCLUDED / SUPPORTED BY YOUR TESTS"):TEXT("RULED OUT")):Out?TEXT("RULED OUT"):TEXT("STILL POSSIBLE");
   Rows.Add(S47FreeformTerminal::Small(TEXT("EXPLANATION / ")+FString(D.Causes[I])+TEXT(" / ")+State));
  }
  if(!C->bConcluded)
  {
   TArray<S47Kit::FW> Hunches;
   for(int32 Cause=0;Cause<FS47FreeformCases::CauseCount;++Cause)Hunches.Add(S47FreeformTerminal::Button(FString(C->Hunch==Cause?TEXT("HUNCH: "):TEXT("Consider: "))+D.Short[Cause],[this,Line,Cause]{SetCaseHunch(Line,Cause);OpenCaseBoard(Line);},C->Hunch==Cause));
   Rows.Add(S47FreeformTerminal::Small(TEXT("OPTIONAL WORKING HUNCH / a note, not evidence. Choose any test without marking a hunch.")));
   Rows.Add(S47Kit::Row(Hunches));
  }
  TSharedRef<SScrollBox> List=SNew(SScrollBox);
  if(const auto* Y=D.Mystery)
  {
   FString Trail=TEXT("TRACES / what each place showed. They need not agree.");
   for(int32 Index=0;Index<FS47FreeformCases::ClueCount;++Index)
   {
    const auto& Clue=Y->Clues[Index];
    const bool bKept=Findings.Contains(Clue.Finding);
    // The feeder has nothing to show before midnight (Director, MysteryClueHere); the chart is there as soon as it is there.
    const bool bLost=Line==FS47FreeformCases::LostLine;
    const TCHAR* Open=bLost?(Index==0?TEXT("not looked at yet. Log on at OPS: it still shows the hour you lost."):TEXT("not looked at yet. Go back to where it happened.")):Index==1?TEXT("not looked at yet. Worth a look after midnight; whatever is there will not last past noon."):TEXT("not looked at yet. Whatever is there will not last past noon.");
    Trail+=FString::Printf(TEXT("\n%s: %s"),Clue.Place,bKept?*Findings.FindRef(Clue.Finding):bLost||(Director&&Director->MysteryTrailOpen())?Open:TEXT("nothing retained."));
   }
   List->AddSlot()[S47FreeformTerminal::Paper(Trail)];
  }
  const bool bCold=Cases.TrailCold(Line,DayNumber);
  if(!C->Leads.IsEmpty()&&Director)
  {
   FString Leads=TEXT("LEADS / retained observations, not yet one cause");
   for(const auto& O:Director->Observations())if(C->Leads.Contains(O.Id))Leads+=FString::Printf(TEXT("\nD%d %s / %s / bearing %s / %s%s"),O.Day,*FS47FreeformCases::Clock(O.Clock),*O.Place,*O.Bearing,O.bHeard?TEXT("heard"):TEXT("seen"),O.PhotoId.IsEmpty()?TEXT(""):TEXT(" / photographed"));
   List->AddSlot()[S47FreeformTerminal::Paper(Leads)];
  }
  for(int32 T=0;T<FS47FreeformCases::TestCount;++T)
  {
   const auto& Res=C->Results[T];
   const FString Unavailable=FS47FreeformCases::UnavailableReason(Line,T,HasKit(TEXT("logger2")));
   // A control the station cannot carry out is not offered at all. A result or a retained plan from an older save still shows.
   if(!Unavailable.IsEmpty()&&!Res.bDone&&C->Planned!=T)continue;
   FString Card=TEXT("TEST / ")+FString(D.Tests[T].Name).ToUpper()+TEXT("\n")+D.Tests[T].Instruction+TEXT("\nPREDICTION / ")+FS47FreeformCases::Prediction(Line,T);
   if(Res.bDone)Card+=FString::Printf(TEXT("\nRESULT DAY %d / %s"),Res.Day,*FS47FreeformCases::TestResultText(Line,T,Res.bPresent));
   else if(!Unavailable.IsEmpty())Card+=TEXT("\nUNAVAILABLE / ")+Unavailable+(C->Planned==T?TEXT(" Cancel this retained plan to choose another control."):TEXT(""));
   else if(D.Tests[T].Access&&!Cases.HasAccess(D.Tests[T].Access))Card+=TEXT("\nNEEDS / the loan field logger (conclude THE REPEATED INTERVAL)");
   else if(C->Planned==T)Card+=TEXT("\nPLANNED / ")+C->PlannedJob+TEXT("\n")+CaseWindowText(Line);
   else if(bCold&&!C->bConcluded)Card+=TEXT("\nTRAIL COLD / Too long since that night for a fresh control to show anything.");
   List->AddSlot()[S47FreeformTerminal::Paper(Card)];
   if(!bCold&&Unavailable.IsEmpty()&&!Res.bDone&&!C->bConcluded&&C->Planned<0&&!(D.Tests[T].Access&&!Cases.HasAccess(D.Tests[T].Access)))List->AddSlot()[S47FreeformTerminal::Button(TEXT("Plan this test: ")+FString(D.Tests[T].Name),[this,Line,T]{PlanCaseTest(Line,T);OpenCaseBoard(Line);},true)];
  }
  if(PlannedFieldTest(Line)>=0)
  {
   List->AddSlot()[S47FreeformTerminal::Button(FieldFocus==Line?TEXT("FOLLOWING THIS FIELD TEST"):TEXT("Follow this field test / retain observing job"),[this,Line]{FocusFieldTest(Line);OpenCaseBoard(Line);},FieldFocus==Line)];
   List->AddSlot()[S47FreeformTerminal::Small(TEXT("Cancel clears only this field plan. No result or reward is recorded; retained evidence stays unchanged."))];
   List->AddSlot()[S47FreeformTerminal::Button(TEXT("Cancel this field plan"),[this,Line]{CancelFieldTest(Line);OpenCaseBoard(Line);},false)];
  }
  double WaitTarget=0;if(CaseWaitTarget(Line,WaitTarget))List->AddSlot()[S47FreeformTerminal::Button(TEXT("Wait until five minutes before this window / review consequences"),[this,WaitTarget]{OpenWaitConfirmation(WaitTarget);},true)];
  Rows.Add(SNew(SBox).HeightOverride(240)[List]);
  if(!C->bConcluded&&D.Mystery)
  {
   Rows.Add(S47FreeformTerminal::Small(TEXT("FILE / What you file is acted on. Nothing here checks it first, and it cannot be taken back. Only your own retained tests can close an explanation.")));
   TArray<S47Kit::FW> Filings;
   for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)if(!Cases.Excluded(Line,I))Filings.Add(S47FreeformTerminal::Button(FString(TEXT("File assessment: "))+D.Short[I],[this,Line,I]{ConcludeCase(Line,I);OpenCaseBoard(Line);},true));
   Rows.Add(S47Kit::Row(Filings));
  }
  else if(C->bConcluded&&D.Mystery)Rows.Add(S47FreeformTerminal::Small(C->bReplied?TEXT("ASSESSMENT FILED / The reply is in your documents."):TEXT("ASSESSMENT FILED / The reply comes in the morning.")));
  else if(!C->bConcluded)
  {
   Rows.Add(S47FreeformTerminal::Small(TEXT("CONCLUDE / A conclusion is accepted only when your retained tests exclude the other explanations. Guessing right is not enough.")));
   TArray<S47Kit::FW> Choices;
   if(Cases.Remaining(Line)==1)
   {for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)if(!Cases.Excluded(Line,I))Choices.Add(S47FreeformTerminal::Button(FString(TEXT("Register supported conclusion: "))+D.Causes[I],[this,Line,I]{ConcludeCase(Line,I);OpenCaseBoard(Line);},true));}
   else Rows.Add(S47FreeformTerminal::Small(TEXT("NO SUPPORTED CONCLUSION YET / choose a test whose predictions separate the remaining explanations. Marking a hunch alone changes no evidence.")));
   if(!Choices.IsEmpty())Rows.Add(S47Kit::Row(Choices));
  }
  else Rows.Add(S47FreeformTerminal::Small(TEXT("CASE CLOSED / The records reply is in your documents.")));
 }
 if(!Feedback.IsEmpty())Rows.Add(S47FreeformTerminal::Small(Feedback));
 OpenTerminal(3,TEXT("OPS / CASE BOARD"),OpsShell(2,S47Kit::Col(Rows)));
}
bool FS47FreeformSession::Use(const FString& Id)
{
 if(!bActive)return false;
 if(AudioDesk&&AudioDesk->Use(Id))return true;
 if(Field&&Field->Use(Id))return true;
 if(Id==TEXT("crtCenter")){OpenReceiver();return true;}
 if(Id==TEXT("crtLeft")){OpenStatus();return true;}
 if(Id==TEXT("crtRight")){OpenTracking();return true;}
 if(Id==TEXT("opsTerminal")||Id==TEXT("telex")||Id==TEXT("logbook")||Id==TEXT("workOrder")||Id==TEXT("anomaly:shiftlog"))
 {
  if(!HasPower()&&(Id==TEXT("logbook")||Id==TEXT("workOrder")||Id==TEXT("anomaly:shiftlog"))){OpenArchive();return true;}
  if(!RequirePower())return true;
  if(Id==TEXT("telex")&&Director&&Director->Use(TEXT("ff:telex")))return true;OpsSection=0;OpenOperations();return true;
 }
 if(Id==TEXT("decoder")||Id==TEXT("signalProcessor")||Id==TEXT("decoderDesk")){OpenTapePanel();return true;}
 if(Id==TEXT("rack"))
 {
  if(CurrentJob()&&CurrentJob()->Phenomenon==TEXT("wrong_source"))Changed(NextAction());
  else if(!StartControl())Changed(NextAction());return true;
 }
 if(Id==TEXT("printer")){Feedback.Empty();if(!CollectChart())Changed(Feedback.IsEmpty()?TEXT("No paper waiting. ")+NextAction():Feedback);return true;}
 if(Id==TEXT("binder")||Id==TEXT("excBinder")||Id==TEXT("serviceRecord")){OpenArchive();return true;}
 if(Id==TEXT("phone")||Id==TEXT("recPhone")){if(Director&&Director->Use(TEXT("ff:phone")))return true;if(Predecessor&&Predecessor->UsePhone())return true;Changed(TEXT("SARO operations line. No scheduled call. Incoming messages are retained at the telex."));return true;}
 return Director&&Director->Use(Id);
}
FString FS47FreeformSession::Label(const FString& Id)const
{
 if(!bActive)return FString();
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
 if(Id==TEXT("rack"))return TEXT("Record an independent receiver control");
 if(Id==TEXT("printer"))return PrinterHint();
 if(Id==TEXT("binder")||Id==TEXT("excBinder")||Id==TEXT("serviceRecord"))return TEXT("File originals and inspect the continuing case archive");
 return Director?Director->Label(Id):FString();
}
// ---- First shift tutorial ----------------------------------------------------------------------
// One real routine job end to end, at the real stations: accept at OPS, point, record, collect the
// original and register (paid), exactly like later routine work. Older training packets remain fileable.
void FS47FreeformSession::AcceptTutorialJob()
{
 if(Tutorial!=1||!CurrentJob()||CurrentJob()->Id!=TutorialJobId)return;
 Tutorial=2;
 Changed(FString::Printf(TEXT("FIRST SHIFT / Job %s accepted. Point, record, keep the original and register it at OPS for $20. This is the normal daily workflow; unusual results later require controls and a physically filed packet."),*CurrentJob()->Id));
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
  Feedback=TEXT("FIRST SHIFT COMPLETE / Your normal observation is registered and paid. ")+AvailableActivities();Changed(Feedback);
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
FString FS47FreeformSession::TutorialObjective()const
{
 if(Tutorial==1)return TEXT("FIRST SHIFT 1/5 / OPS (corner PC): OPEN DAILY WORK, READ THE JOB CARD AND ACCEPT YOUR FIRST JOB");
 const FJob* J=CurrentJob();if(!J)return FString();
 return FString::Printf(TEXT("FIRST SHIFT %d/%d / %s"),FMath::Clamp(Tutorial,2,6),J->Stage>=EJobStage::Registered?6:5,*JobNextAction(*J));
}

FString FS47FreeformSession::Objective()const
{
 if(bDead)return TEXT("PAUSE MENU: LOAD A SAVED CASE TO CONTINUE AFTER CONTACT");
 // Stranded away from SARO: the only thing that matters is the way back. An empty tank at SARO stays a task below.
 if(Field&&!Field->HasVehicleFuel()&&M&&M->World&&M->World->Area!=TEXT("saro"))return TEXT("SERVICE TRUCK: TANK EMPTY. GET OUT AND USE THE TRUCK TO CALL ROADSIDE RETURN; IT COSTS TIME, NOT MONEY");
 if(FieldFocus>=0)return FieldNextAction(FieldFocus);
 if(!HasPower()&&PrinterProgress()>=1)return PrinterHint();
 if(bPowerFault&&!HasPower()&&(!CurrentJob()||(CurrentJob()->Stage!=EJobStage::Chart&&CurrentJob()->Stage!=EJobStage::Packet)))
 {
  if(Field&&Field->EmergencyFuseAvailable()&&Field->InventoryCount(TEXT("fuel"))<=0&&Field->GeneratorHours()<=0)return TEXT("NO POWER, NO SPARE FUSE: THE CALICHE STORE COUNTER ISSUES ONE FEEDER-REPAIR FUSE ON CREDIT, DAY OR NIGHT");
  return TEXT("SERVICE BAY: MEASURE THE FEEDER OR TRANSFER TO THE FUELLED GENERATOR");
 }
 if(ServiceGuide==1&&bGuideFocus){const FString Guide=ServiceObjective();if(!Guide.IsEmpty())return Guide;}
 if(Tutorial>0){const FString T=TutorialObjective();if(!T.IsEmpty())return T;}
 // A tip is only worth the top line when it is not in the way of the way home, the power or a recording under way.
 // It stands in the task list all the same.
 if(Director&&!Director->TipLine().IsEmpty()&&(!CurrentJob()||CurrentJob()->Stage==EJobStage::New||CurrentJob()->Stage==EJobStage::Filed))return Director->TipLine();
 return NextAction();
}
FString FS47FreeformSession::NextAction()const
{
 if(ServiceGuide==1&&bGuideFocus)return ServiceObjective();
 if(Tutorial>0)return TutorialObjective();
 if(FieldFocus>=0)return FieldNextAction(FieldFocus);
 if(!CurrentJob()||CurrentJob()->Stage==EJobStage::Filed)return AvailableActivities();
 return ReceiverNextAction();
}
FString FS47FreeformSession::JobNextAction(const FJob& JobValue)const
{
 const FJob* J=&JobValue;
 if(EveningGated(JobValue))return EveningLine(JobValue);
 if(J->CaseTest>=0&&J->Stage<=EJobStage::Pointed){const FString Missing=CasePrecondition(*J);if(!Missing.IsEmpty())return Missing;}
 if(M&&M->Web&&ArrayBlocked()&&(J->Stage<=EJobStage::Integrating||(J->Stage==EJobStage::Recorded&&J->Phenomenon==TEXT("wrong_source")&&(!J->bControl||J->ControlAt<8))))return ArrayMeasurementStatus();
 switch(J->Stage)
 {
  case EJobStage::New:return TEXT("TRACKING (right-hand PC): POINT THE ARRAY TO THE SELECTED SOURCE");
  case EJobStage::Pointed:if(&JobValue!=CurrentJob())return TEXT("TRACKING (right-hand PC): FOLLOW THIS JOB AND VERIFY ITS ACTUAL POINTING BEFORE RECORDING");return PointReady()&&Settle>=1?TEXT("RECEIVER (centre PC): TUNE THE CARD AND PRESS START RECORDING"):TEXT("TRACKING (right-hand PC): WAIT FOR ENCODER AGREEMENT, THEN USE RECEIVER (centre PC)");
  case EJobStage::Integrating:return TEXT("RECEIVER (centre PC): KEEP TUNING VALID; COLLECT THE ORIGINAL AT THE PRINTER WHEN RECORDING FINISHES");
  case EJobStage::Chart:return J->ChartFeed<PrinterSeconds?PrinterHint():FString(TEXT("PRINTER: RETAIN THE ORIGINAL OBSERVATION CHART"));
  case EJobStage::Recorded:
   if(J->bDuplicateRejected)return TEXT("OPS / DAILY WORK: SELECT ANOTHER OBSERVATION. LEGACY ORIGINAL RETAINED; UNVERIFIABLE DUPLICATE REMOVED FROM PRINTER QUEUE WITHOUT A FINDING OR PAYMENT.");
   if(!J->bAnomaly)return J->CaseTest>=0&&!J->bCaseRecorded?TEXT("OPS (corner PC): REGISTER THE ROUTINE RESULT AND ITS CASE TEST"):TEXT("OPS (corner PC): REGISTER THE ROUTINE RESULT");
   if(!J->bControl)return J->Phenomenon==TEXT("wrong_source")?TEXT("TRACKING (right-hand PC): MEASURE WEST, EAST AND CENTRE BEAMS"):TEXT("RECEIVER (centre PC), RECORD TAB: RECORD THE INTERNAL CONTROL");
   if(J->ControlAt<8)
   {
    if(J->Phenomenon==TEXT("wrong_source"))return TEXT("TRACKING (right-hand PC): WAIT FOR AUTOMATIC WEST, EAST AND CENTRE SAMPLES; THEN COMPARE AT OPS (corner PC), DAILY WORK");
    if(J->Phenomenon==TEXT("time_disagreement"))return TEXT("RECEIVER RACK: WAIT FOR THE AUTOMATIC INTERNAL CONTROL; THEN COMPARE CLOCK READINGS AT OPS (corner PC), DAILY WORK");
    if(J->Phenomenon==TEXT("duplicate_print"))return TEXT("RECEIVER RACK: WAIT FOR THE AUTOMATIC REGISTER CHECK; THEN WAIT FOR THE DUPLICATE TO FINISH AT THE PRINTER");
    return TEXT("RECEIVER RACK: WAIT FOR THE AUTOMATIC INTERNAL CONTROL; THEN REVIEW BOTH RECORDINGS AT THE REEL DECK ON THE WALL TABLE");
   }
   if(J->Phenomenon==TEXT("duplicate_print")&&!J->bDuplicateCollected)return J->DuplicateFeed<PrinterSeconds?PrinterHint():FString(TEXT("PRINTER: COLLECT THE SEPARATE DUPLICATE WITHOUT ALTERING THE ORIGINAL; THEN COMPARE AT OPS (corner PC), DAILY WORK"));
   if(Specialized(J->Phenomenon))return J->bAudioReviewed?TEXT("OPS (corner PC): REGISTER THE SUPPORTED INSTRUMENTAL FINDING"):TEXT("OPS (corner PC): DAILY WORK / SELECT THIS JOB AND PRESS COMPARE AND REGISTER THE RETAINED MEASUREMENTS");
   return J->bAudioReviewed?TEXT("OPS (corner PC): REGISTER THE SUPPORTED CONCLUSION"):TEXT("REEL DECK (wall table): REVIEW BOTH RECORDINGS AND MARK THE EXTRA TICKS");
  case EJobStage::Registered:return J->PacketFeed<PrinterSeconds?PrinterHint():FString(TEXT("PRINTER: COLLECT THE LABELLED REGISTERED PACKET"));
  case EJobStage::Packet:return TEXT("RECORDS ROOM: PLACE THE ORIGINAL PACKET IN THE BINDER");
  default:return TEXT("OPS (corner PC): SELECT DAILY WORK OR CASE ARCHIVE");
 }
}
FString FS47FreeformSession::JobLocation(const FJob& J)const
{
 if(J.CaseTest>=0&&J.Stage<=EJobStage::Pointed&&!CasePrecondition(J).IsEmpty())
 {
  switch(CaseCondition(J.CaseLine,J.CaseTest))
  {
   case ECaseCondition::Generator:return TEXT("Service bay / backup generator");
   case ECaseCondition::Repeat:return TEXT("OPS / CASE BOARD / dated repeat window");
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

TArray<FS47Task> FS47FreeformSession::Tasks()const
{
 TArray<FS47Task> Result;
 if(Director){const FString Reported=Director->ReportedLine();if(!Reported.IsEmpty())Result.Add({Reported,false});}
 if(Director){bool Ready=false;const FString Hint=Director->PhotoControlFieldHint(Ready);if(!Hint.IsEmpty())Result.Add({Hint,false});}
 if(Gear&&Gear->AnyLeft())Result.Add({Gear->TaskLine(),false});
 if(Predecessor){const FString Line=Predecessor->TaskLine();if(!Line.IsEmpty())Result.Add({Line,false});}
 if(Round){const FString Rounds=Round->TaskLine();if(!Rounds.IsEmpty())Result.Add({Rounds,false});}
 if(Care&&!Care->StatusLine().IsEmpty())Result.Add({TEXT("TRUCK / ")+Care->StatusLine(),false});
 if(bPowerFault)Result.Add({TEXT("FEEDER / Measure, isolate and replace the control fuse; restore SARO power."),false});
 if(Field&&!Field->HasVehicleFuel())Result.Add({TEXT("SERVICE TRUCK / Tank empty: use the truck for a stock fuel can or roadside return. It costs time, not money."),false});
 if(Field&&Field->HasArrayFault()&&ServiceGuide!=1)Result.Add({Field->ServiceDishId()+TEXT(" / Diagnose at the status PC, replace the relay and run the local out-and-back test."),false});
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)if(const auto* C=Cases.Get(Line);C&&C->bOpen&&!C->bConcluded)Result.Add({C->Planned>=0?(PlannedFieldTest(Line)>=0?FieldNextAction(Line):TEXT("CASE / ")+CaseTestName(Line,C->Planned)+TEXT(": run job ")+C->PlannedJob):TEXT("CASE / ")+FString(FS47FreeformCases::Def(Line).Title)+(FS47FreeformCases::Def(Line).Mystery?TEXT(": choose a control or file your assessment on the OPS case board"):TEXT(": choose a test on the OPS case board")),false});
 if(Director){const FString Trail=Director->MysteryTrailHint();if(!Trail.IsEmpty())Result.Add({Trail,false});}
 if(const FString Goal=StationGoalLine();!Goal.IsEmpty())Result.Add({Goal,false});
 if(ServiceGuide>0)Result.Add({ServiceGuide==2?FString(TEXT("Service work order / verified and archived")):ServiceObjective(),ServiceGuide==2});
 for(const FJob& J:Work)if(J.Day==DayNumber||J.Stage!=EJobStage::Filed)Result.Add({JobLine(J),J.Stage==EJobStage::Filed});
 return Result;
}
S47J::FObj FS47FreeformSession::Save()const
{
 auto O=S47J::New();S47J::Set(O,TEXT("fieldFocus"),FieldFocus);S47J::Set(O,TEXT("tutorial"),Tutorial);S47J::Set(O,TEXT("tutorialJobId"),TutorialJobId);S47J::Set(O,TEXT("jobSeed"),JobSeed);S47J::Set(O,TEXT("serviceGuide"),ServiceGuide);S47J::Set(O,TEXT("guideServiceBaseline"),GuideServiceBaseline);S47J::Set(O,TEXT("guideSupplyBaseline"),GuideSupplyBaseline);S47J::Set(O,TEXT("guideSupply"),bGuideSupply);S47J::Set(O,TEXT("guideFocus"),bGuideFocus);S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("clock"),WorldClock);if(WokenNight>0)S47J::Set(O,TEXT("wokenNight"),WokenNight);if(RousedNight>0)S47J::Set(O,TEXT("rousedNight"),RousedNight);if(RulesLive()||ReviewWeek>0)S47J::Set(O,TEXT("reviewWeek"),ReviewWeek);if(Carried>0)S47J::Set(O,TEXT("carried"),Carried);if(StandbyTotal>0)S47J::Set(O,TEXT("standbyPaid"),StandbyTotal);S47J::Set(O,TEXT("cash"),Cash);S47J::Set(O,TEXT("loan"),Loan);S47J::Set(O,TEXT("lethal"),bDeadly);S47J::Set(O,TEXT("dead"),bDead);S47J::Set(O,TEXT("powerFault"),bPowerFault);S47J::Set(O,TEXT("anomaly"),bAnomaly);S47J::Set(O,TEXT("phenomenon"),PhenomenonId);
 S47J::Set(O,TEXT("journal"),Journal.Save());S47J::Set(O,TEXT("economyRevision"),2);
 S47J::Set(O,TEXT("selected"),Selected);S47J::Set(O,TEXT("audioSource"),AudioSourceIndex);S47J::Set(O,TEXT("audioCursor"),AudioCursor);S47J::Set(O,TEXT("settle"),Settle);S47J::Set(O,TEXT("cross"),CrossPoint);S47J::Set(O,TEXT("crossSample"),CrossSample);S47J::Set(O,TEXT("crossRunning"),bCrossRunning);
 TArray<TSharedPtr<FJsonValue>> Jobs;
 for(const FJob& J:Work)
 {
  auto A=S47J::New();S47J::Set(A,TEXT("chartFeed"),J.ChartFeed);S47J::Set(A,TEXT("packetFeed"),J.PacketFeed);S47J::Set(A,TEXT("duplicateFeed"),J.DuplicateFeed);S47J::Set(A,TEXT("id"),J.Id);S47J::Set(A,TEXT("day"),J.Day);S47J::Set(A,TEXT("slot"),J.Slot);S47J::Set(A,TEXT("source"),J.Source);S47J::Set(A,TEXT("phenomenon"),J.Phenomenon);S47J::Set(A,TEXT("stage"),int32(J.Stage));S47J::Set(A,TEXT("elapsed"),J.Elapsed);S47J::Set(A,TEXT("quality"),J.Quality);S47J::Set(A,TEXT("receivedLevel"),J.ReceivedLevel);S47J::Set(A,TEXT("anomaly"),J.bAnomaly);S47J::Set(A,TEXT("control"),J.bControl);S47J::Set(A,TEXT("reviewed"),J.bAudioReviewed);S47J::Set(A,TEXT("printed"),J.bPrinted);S47J::Set(A,TEXT("paid"),J.bPaid);S47J::Set(A,TEXT("controlAt"),J.ControlAt);S47J::Set(A,TEXT("originalCovered"),J.OriginalCovered);S47J::Set(A,TEXT("controlCovered"),J.ControlCovered);S47J::Set(A,TEXT("markStart"),J.MarkStart);S47J::Set(A,TEXT("markEnd"),J.MarkEnd);S47J::Set(A,TEXT("receiverClock"),J.ReceiverClock);S47J::Set(A,TEXT("referenceClock"),J.ReferenceClock);S47J::Set(A,TEXT("controlReceiverClock"),J.ControlReceiverClock);S47J::Set(A,TEXT("controlReferenceClock"),J.ControlReferenceClock);S47J::Set(A,TEXT("duplicateClock"),J.DuplicateClock);S47J::Set(A,TEXT("duplicateInterval"),J.DuplicateInterval);
  S47J::Set(A,TEXT("traceTicks"),J.TraceTicks);S47J::Set(A,TEXT("legacyTrace"),J.bLegacyTrace);
  if(J.CaptureOriginal.Enabled())S47J::Set(A,TEXT("captureOriginal"),J.CaptureOriginal.Save());
  if(J.CaptureControl.Enabled())S47J::Set(A,TEXT("captureControl"),J.CaptureControl.Save());
  if(J.RecordingSoundVersion==1||J.RecordingSoundVersion==2)
  {
   auto Sound=S47J::New();S47J::Set(Sound,TEXT("v"),J.RecordingSoundVersion);S47J::Set(Sound,TEXT("gain"),J.RecordingGain);
   S47J::Set(Sound,TEXT("bandwidth"),J.RecordingBandwidth);S47J::Set(Sound,TEXT("quality"),J.RecordingQuality);S47J::Set(A,TEXT("recordingSound"),Sound);
  }
  S47J::Set(A,TEXT("beamPoint"),J.BeamPoint);S47J::Set(A,TEXT("beamSettle"),J.BeamSettle);S47J::Set(A,TEXT("beamSample"),J.BeamSample);S47J::Set(A,TEXT("beamAz"),J.BeamAz);S47J::Set(A,TEXT("beamEl"),J.BeamEl);S47J::Set(A,TEXT("beamAmplitude"),J.BeamAmplitude);S47J::Set(A,TEXT("beamVerified"),J.bBeamVerified);S47J::Set(A,TEXT("clockMeasured"),J.bClockMeasured);S47J::Set(A,TEXT("duplicateCollected"),J.bDuplicateCollected);S47J::Set(A,TEXT("duplicateRejected"),J.bDuplicateRejected);S47J::Set(A,TEXT("originalPage"),J.OriginalPage);S47J::Set(A,TEXT("originalStamp"),J.OriginalStamp);S47J::Set(A,TEXT("duplicatePage"),J.DuplicatePage);S47J::Set(A,TEXT("duplicateStamp"),J.DuplicateStamp);S47J::Set(A,TEXT("caseLine"),J.CaseLine);S47J::Set(A,TEXT("caseTest"),J.CaseTest);S47J::Set(A,TEXT("caseValid"),J.bCaseValid);S47J::Set(A,TEXT("caseRecorded"),J.bCaseRecorded);if(J.bDocked)S47J::Set(A,TEXT("docked"),true);if(J.bGenerated)
  {
   auto C=S47J::New();const auto& P=J.Card.Receiver;S47J::Set(C,TEXT("v"),1);S47J::Set(C,TEXT("f"),P.F);S47J::Set(C,TEXT("ftol"),P.FTol);S47J::Set(C,TEXT("g0"),P.Gain[0]);S47J::Set(C,TEXT("g1"),P.Gain[1]);S47J::Set(C,TEXT("bw0"),P.Bw[0]);S47J::Set(C,TEXT("bw1"),P.Bw[1]);S47J::Set(C,TEXT("az0"),P.Az[0]);S47J::Set(C,TEXT("az1"),P.Az[1]);S47J::Set(C,TEXT("el"),J.Card.El);S47J::Set(C,TEXT("seconds"),J.Card.Seconds);S47J::Set(C,TEXT("suggested"),J.Card.SuggestedClock);S47J::Set(C,TEXT("text"),P.Card);S47J::Set(A,TEXT("card"),C);
  }
  Jobs.Add(MakeShared<FJsonValueObject>(A));
 }
 O->SetArrayField(TEXT("jobs"),Jobs);
 auto F=S47J::New(),D=S47J::New();for(const auto& P:Findings){S47J::Set(F,*P.Key,P.Value);S47J::Set(D,*P.Key,FindingDays.FindRef(P.Key));}S47J::Set(O,TEXT("findings"),F);S47J::Set(O,TEXT("findingDays"),D);
 TArray<double> U,Q;for(bool V:Upgrades)U.Add(V?1:0);for(int32 V:MysterySteps)Q.Add(V);S47J::Set(O,TEXT("upgrades"),U);S47J::Set(O,TEXT("mysteries"),Q);
 TArray<double> Angles=SavedAngles;if(!bRestoreAngles&&M&&M->Web&&M->Web->IsLoaded()){Angles.Reset();for(int32 I=0;I<M->Web->DishCount();++I)Angles.Append({M->Web->DishAz(I),M->Web->DishEl(I),M->Web->DishTargetAz(I),M->Web->DishTargetEl(I)});}S47J::Set(O,TEXT("angles"),Angles);
 S47J::Set(O,TEXT("cases"),Cases.Save());
 if(!bOldCalendar)S47J::Set(O,TEXT("calendar"),2);
 if(Predecessor)S47J::Set(O,TEXT("predecessor"),Predecessor->Save());
 if(CaseField)S47J::Set(O,TEXT("fieldTests"),CaseField->Save());
 if(Field)S47J::Set(O,TEXT("field"),Field->Save());if(Decor)S47J::Set(O,TEXT("decor"),Decor->Save());if(FarmSite)S47J::Set(O,TEXT("farmSite"),FarmSite->Save());if(Deputy){const auto DeputyState=Deputy->Save();if(DeputyState->Values.Num()>0)S47J::Set(O,TEXT("deputy"),DeputyState);}if(Gear){const auto Lying=Gear->Save();if(Lying->Values.Num()>0)S47J::Set(O,TEXT("gear"),Lying);}if(!Kit.IsEmpty()){TArray<FString> KitList=Kit.Array();KitList.Sort();S47J::Set(O,TEXT("kit"),KitList);}if(Places){const auto Known=Places->Save();if(Known->Values.Num()>0)S47J::Set(O,TEXT("places"),Known);}if(EventPlaces){const auto Where=EventPlaces->Save();if(Where->Values.Num()>0)S47J::Set(O,TEXT("eventPlaces"),Where);}if(Tips){const auto Said=Tips->Save();if(Said->Values.Num()>0)S47J::Set(O,TEXT("tips"),Said);}if(YardAlarm){const auto Horn=YardAlarm->Save();if(Horn->Values.Num()>0)S47J::Set(O,TEXT("alarm"),Horn);}if(Care){const auto Truck=Care->Save();if(Truck->Values.Num()>0)S47J::Set(O,TEXT("care"),Truck);}if(TripCamera){const auto Trip=TripCamera->Save();if(Trip->Values.Num()>0)S47J::Set(O,TEXT("tripCamera"),Trip);}if(Round){const auto Sheet=Round->Save();if(Sheet->Values.Num()>0)S47J::Set(O,TEXT("round"),Sheet);}if(RoadCar){const auto Pickup=RoadCar->Save();if(Pickup->Values.Num()>0)S47J::Set(O,TEXT("roadCar"),Pickup);}if(HouseDoors)S47J::Set(O,TEXT("houseDoors"),HouseDoors->Save());if(IndoorDread){const auto Dread=IndoorDread->Save();if(Dread->Values.Num()>0)S47J::Set(O,TEXT("indoorDread"),Dread);}if(Aftermath){const auto After=Aftermath->Save();if(After->Values.Num()>0)S47J::Set(O,TEXT("aftermath"),After);}if(Director)S47J::Set(O,TEXT("director"),Director->Save());return O;
}
bool FS47FreeformSession::SelfTest(TArray<FString>& Lines)
{
 bool OK=S47CaptureSound::SelfTest(Lines);auto Check=[&](bool B,const TCHAR* Name){OK&=B;Lines.Add(FString(B?TEXT("PASS "):TEXT("FAIL "))+Name);};
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
 S.RecordEvidence(TEXT("extra_pulse"),TEXT("retained"));S.RecordEvidence(TEXT("extra_pulse"),TEXT("changed"));Check(S.Findings.FindRef(TEXT("extra_pulse"))==TEXT("retained"),TEXT("Freeform retained original facts are not overwritten by repeats"));
 S.TestAdvance(86400);Check(S.Day()==2&&S.Jobs().Num()==6,TEXT("Freeform new days retain unfinished work and create distinct daily job identities"));
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
 Check(TerminalNextLine(TEXT("FIRST SHIFT 1/5 / OPS (corner PC): OPEN DAILY WORK, READ THE JOB CARD AND ACCEPT YOUR FIRST JOB"),3)==TEXT("NEXT / USE THIS PC"),TEXT("NEXT line reads past the first-shift counter"));
 Check(TerminalNextLine(TEXT("ARRAY 12/27 / MEASUREMENT RATE 43% / BLOCKED: inspect ARRAY STATUS (left-hand PC); need 24"),0)==TEXT("NEXT / USE THIS PC")&&TerminalNextLine(TEXT("ARRAY 12/27 / MEASUREMENT RATE 43% / BLOCKED: inspect ARRAY STATUS (left-hand PC); need 24"),1)==TEXT("NEXT / ARRAY STATUS (LEFT-HAND PC)"),TEXT("A blocked array sends every workstation to the status PC"));
 Check(TerminalNextLine(TEXT("EVENING OBSERVATION / 3C 286 IS UP FROM 19:20. UNTIL THEN: DAY WORK, REPAIRS, SUPPLIES. OPS CAN WAIT FOR IT."),2)==TEXT("NEXT / 3C 286 IS UP FROM 19:20")&&TerminalNextLine(Workflow.EveningLine(*WJ),2).Contains(TEXT("IS UP FROM")),TEXT("An evening observation shows its time, and a clock's colon does not cut the line"));
 Check(TerminalNextLine(TEXT("S-03 CABLE JUNCTION / LOCAL BOX: INSPECT, ISOLATE AND OPEN THE RELAY HATCH"),1)==TEXT("NEXT / S-03 CABLE JUNCTION / LOCAL BOX")&&TerminalNextLine(TEXT("A very long sentence with no station in it that cannot fit on the glass at all"),1).IsEmpty()&&TerminalNextLine(TEXT("PAUSE MENU: LOAD A SAVED CASE TO CONTINUE AFTER CONTACT"),3).IsEmpty()&&TerminalNextLine(FString(),3).IsEmpty(),TEXT("NEXT line keeps a place that fits and says nothing rather than half a sentence"));
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
 Case.Stop();
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
  // The evening's cards, from the planner's own rules, for every source and slot.
  FS47FreeformSession Eve(nullptr);Eve.Begin(nullptr);Eve.TestAdvance(86400);
  TArray<const FJob*> Day2;for(const FJob& J:Eve.Work)if(J.Day==2)Day2.Add(&J);
  bool bAll=Day2.Num()==3,bRange=true;for(const auto* J:Day2){bAll&=Eve.EveningGated(*J);const double Up=EveningUp(*J);bRange&=Up>=EveningOpens&&Up<EveningCloses;}
  Check(bAll&&bRange,TEXT("Day: from day two all three cards wait for the evening, and come up between 19:00 and 23:30"));
  Check(!Eve.EveningGated(Eve.Work[0]),TEXT("Day: the first day's own cards are not held back"));
  double UpAt=0;const FJob* Soonest=Eve.NextEveningCard(UpAt);
  Check(Soonest&&UpAt==86400+EveningUp(*Soonest)&&UpAt>Eve.Clock(),TEXT("Day: OPS can name the soonest source and when"));
  Eve.TestAdvance(FMath::Fmod(86400.*2+22*3600.-Eve.Clock(),86400.));
  Check(!Eve.EveningGated(*Day2[0])&&!Eve.EveningGated(*Day2[1]),TEXT("Day: at ten in the evening the sources are up"));
  Eve.TestAdvance(2*3600.);
  Check(Eve.EveningGated(*Day2[0]),TEXT("Day: a card not begun by half past eleven waits for the next evening"));
  auto* Begun=const_cast<FJob*>(Day2[0]);Begun->Stage=EJobStage::Pointed;
  Check(!Eve.EveningGated(*Begun),TEXT("Day: a card already begun runs out whenever it was begun"));
  Eve.Stop();
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
  Check(PayAfterDays(0)==20&&PayAfterDays(1)==20&&PayAfterDays(2)==15&&PayAfterDays(3)==10&&PayAfterDays(4)==5&&PayAfterDays(30)==5,TEXT("Time: a card pays in full on its day and the next, then less, and never nothing"));
  Check(StandbyFor(0)==0&&StandbyFor(3600)==1&&StandbyFor(9*3600.)==3&&StandbyFor(11*3600.)==4&&StandbyFor(40*3600.)==4,TEXT("Time: waiting costs a dollar per three hours, at most four"));
  FS47FreeformSession Econ(nullptr);Econ.Begin(nullptr);
  Check(Econ.JobPay(Econ.Work[0])==20,TEXT("Time: developer runs keep the flat rate"));
  Econ.TestLiveRules();Econ.TestAdvance(2*86400);
  Check(Econ.JobPay(Econ.Work[0])==15,TEXT("Time: a card two days old pays $15"));
  Econ.Work[0].bDocked=true;Check(Econ.JobPay(Econ.Work[0])==5,TEXT("Time: docked, half that to the nearest five"));Econ.Work[0].bDocked=false;
  const int32 Start=Econ.Budget();Econ.TestAdvance(5*86400);Econ.StepWeeklyReview();
  Check(Econ.Day()==8&&Econ.Budget()==Start-40&&Econ.Arrears()==0&&Econ.HasFinding(TEXT("week-review-1"))&&Econ.Debt()==0&&Econ.NextReviewDay()==15,TEXT("Time: the first review falls on day eight, $40 for power and fuel, on a sheet"));
  Econ.StepWeeklyReview();Check(Econ.Budget()==Start-40,TEXT("Time: a review is paid once"));
  Econ.Cash=10;Econ.TestAdvance(7*86400);Econ.StepWeeklyReview();
  Check(Econ.Budget()==0&&Econ.Arrears()==30&&Econ.Debt()==0,TEXT("Time: short of money it takes what is there, carries the rest, and never touches the requisition"));
  FS47FreeformSession EconAgain(nullptr);EconAgain.Begin(S47J::Read(S47J::Write(Econ.Save())));
  Check(EconAgain.Arrears()==30&&EconAgain.NextReviewDay()==22,TEXT("Time: what is carried survives a Continue, and is not charged again"));
  auto OldEcon=Econ.Save();OldEcon->RemoveField(TEXT("reviewWeek"));OldEcon->RemoveField(TEXT("carried"));S47J::Set(OldEcon,TEXT("cash"),60);
  FS47FreeformSession OldGame(nullptr);OldGame.Begin(OldEcon);OldGame.TestLiveRules();OldGame.StepWeeklyReview();
  Check(OldGame.Budget()==60&&OldGame.Arrears()==0,TEXT("Time: an older save on day fifteen is not charged for the weeks behind it"));
  Econ.Stop();EconAgain.Stop();OldGame.Stop();
  // The running costs: a recording nobody can read costs Ward's re-check on the sheet.
  FS47FreeformSession Fees(nullptr);Fees.Begin(nullptr);Fees.TestLiveRules();
  Fees.Work[0].Stage=EJobStage::Filed;Fees.Work[0].CaseTest=-1;Fees.Work[0].Day=1;Fees.Work[0].RecordingSoundVersion=2;Fees.Work[0].RecordingQuality=.1;
  const int32 FeeStart=Fees.Budget();Fees.TestAdvance(7*86400);Fees.StepWeeklyReview();
  Check(Fees.Budget()==FeeStart-40-PoorRecordFee&&Fees.FindingText(TEXT("week-review-1")).Contains(TEXT("RE-CHECK")),TEXT("Time: Ward charges for a recording nobody can read, on the sheet, and not much"));
  Check(WearCap+FilmCap+FeeCap+ReviewPower+ReviewFuel<=100,TEXT("Time: the worst week costs at most $100, under two days' full pay"));
  Fees.Stop();
  // Being taken, in the session's books.
  FS47FreeformSession Gone(nullptr);Gone.Begin(nullptr);Gone.TestLiveRules();Gone.TestAdvance(86400+17*3600.);
  const double Safe=Gone.Clock();Gone.Work.Last().Stage=EJobStage::Pointed;Gone.TakenAway(5*3600.);const FString Cost=Gone.TakenCosts(Safe);
  const auto* LostCase=Gone.CaseFile().Get(FS47FreeformCases::LostLine);
  Check(LostCase&&LostCase->bOpen&&Cost.Contains(TEXT("half rate"))&&Gone.Work.Last().Stage==EJobStage::New&&Gone.Work.Last().bDocked&&Gone.JobPay(Gone.Work.Last())==10,TEXT("Taken: the case opens, today's cards go in at half rate, and a pointed array has to be pointed again"));
  Gone.LostTimeTrace(1);Check(Gone.HasFinding(TEXT("lost_ground"))&&FS47FreeformCases::LineForFinding(TEXT("lost_clock"))==FS47FreeformCases::LostLine,TEXT("Taken: going back gives the case a trace"));
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
 }
 Check(FS47FarmSite::ByDaylight(7*3600.)&&!FS47FarmSite::ByDaylight(2*3600.)&&!FS47FarmSite::ByDaylight(21*3600.),TEXT("Marks: daylight is the morning after"));
 {
  int32 Reads[3]={0,0,0};bool bStable=true;
  for(int32 D=1;D<=300;++D){const int32 R0=FS47FarmSite::DaylightRead(47,TEXT("farm"),D);++Reads[R0];bStable&=R0==FS47FarmSite::DaylightRead(47,TEXT("farm"),D);}
  Check(bStable&&Reads[0]>30&&Reads[1]>30&&Reads[2]>30,TEXT("Marks: the second look reads ordinary, undecided or strange, and looking again never changes it"));
  bool bWords=true;for(int32 R0=0;R0<3;++R0)for(bool bFarmSite:{true,false}){const FString T0=FS47FarmSite::DaylightText(R0,bFarmSite);bWords&=T0.Contains(TEXT("by daylight"))&&T0.Contains(TEXT("what is on the ground"));}
  Check(bWords,TEXT("Marks: every daylight record says what is there and no more"));
 }
 {bool bNamed=true;for(int32 L0=0;L0<FS47FarmSite::SecondCallLines;++L0)bNamed&=FS47FarmSite::SecondCallLine(L0,TEXT("the church")).Contains(TEXT("the church"));Check(bNamed,TEXT("Calls: every second call names the place and says nothing about what it is"));}
 Check(FS47FreeformCases::LineForFinding(TEXT("farm_trace_daylight_d3"))==2,TEXT("Marks: a daylight record goes to the visitors' case"));
 {
  FS47FreeformSession Trust(nullptr);Trust.Begin(nullptr);
  Check(!Trust.DeputyOnRoute()&&Trust.SheriffTrust()==0,TEXT("Patrol: nobody has written to the county yet"));
  Trust.TestLiveRules();Trust.TestAdvance(86400);Trust.TakenAway(3600);Trust.TakenCosts(Trust.Clock()-3600);
  Check(Trust.ConcludeCase(FS47FreeformCases::LostLine,1)&&Trust.SheriffTrust()==1&&Trust.DeputyOnRoute(),TEXT("Patrol: any case carried to a conclusion and filed earns the county's trust, whatever it says"));
  Trust.Stop();
 }
 auto Future=S47J::New();S47J::Set(Future,TEXT("v"),2);R.Begin(Future);Check(!R.Active(),TEXT("Unknown Freeform save revisions do not restart a fresh session"));
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
