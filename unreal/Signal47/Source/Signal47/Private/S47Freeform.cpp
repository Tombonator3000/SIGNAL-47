#include "S47Freeform.h"
#include "S47Image.h"
#include "S47FreeformAudioDesk.h"
#include "S47FreeformTerminal.h"
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
 double SafeNum(const S47J::FObj& O,const TCHAR* K,double D=0) { const double V=S47J::Num(O,K,D);return FMath::IsFinite(V)?V:D; }
 int32 SafeInt(const S47J::FObj& O,const TCHAR* K,int32 D=0) { return int32(FMath::Clamp(SafeNum(O,K,D),-100000000.,100000000.)); }
 const double Ticks[]={9.30,9.48,9.66,9.84,11.40,11.60,11.80,12.00,12.20,12.40,12.60};
 const FVector2D Cross[]={{0,.5},{0,-.5},{.5,0},{-.5,0},{0,0}};
 const double BeamOffsets[]={-.5,.5,0};
 bool Specialized(const FString& Id){return Id==TEXT("wrong_source")||Id==TEXT("time_disagreement")||Id==TEXT("duplicate_print");}
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
 bActive=true;bDeadly=bLethal;bDead=bPowerFault=bAnomaly=bTelemetry=bAudioOpen=bAudioPlaying=bRecovering=false;
 ServiceGuide=GuideServiceBaseline=GuideSupplyBaseline=0;bGuideSupply=bGuideFocus=false;
 // A new Freeform game opens with the first-shift tutorial; dev/test runs opt in with -S47FreeformTutorial.
 Tutorial=!Saved&&M&&M->Flow&&(!M->Flow->bDevRun||FParse::Param(FCommandLine::Get(),TEXT("S47FreeformTutorial")))?1:0;
 TutorialJobId.Empty();FieldFocus=-1;
 DayNumber=1;WorldClock=8*3600;Cash=120;Loan=0;Selected=AudioSourceIndex=SelectedDish=0;
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
  if(!FMath::IsFinite(WorldClock)) {bActive=false;return;}
  DayNumber=FMath::Max(1,int32(FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay))+1);
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
   if(const auto Sound=S47J::Obj(O,TEXT("recordingSound"));Sound&&SafeInt(Sound,TEXT("v"))==1)
   {
    J.RecordingSoundVersion=1;J.RecordingGain=FMath::Clamp(SafeNum(Sound,TEXT("gain")),0.,100.);
    J.RecordingBandwidth=FMath::Clamp(SafeNum(Sound,TEXT("bandwidth")),0.,100.);J.RecordingQuality=FMath::Clamp(SafeNum(Sound,TEXT("quality")),0.,1.);
   }
   J.bAnomaly=S47J::Bool(O,TEXT("anomaly"));J.bControl=S47J::Bool(O,TEXT("control"));J.bAudioReviewed=S47J::Bool(O,TEXT("reviewed"));J.bPrinted=S47J::Bool(O,TEXT("printed"));J.bPaid=S47J::Bool(O,TEXT("paid"));
   J.ControlAt=FMath::Clamp(SafeNum(O,TEXT("controlAt")),0.,8.);J.OriginalCovered=FMath::Clamp(SafeNum(O,TEXT("originalCovered")),0.,18.);J.ControlCovered=FMath::Clamp(SafeNum(O,TEXT("controlCovered")),0.,8.);
   J.TraceTicks=S47J::Nums(O,TEXT("traceTicks"));J.bLegacyTrace=S47J::Bool(O,TEXT("legacyTrace"),!O->HasField(TEXT("traceTicks")));
   if(!O->HasField(TEXT("traceTicks"))&&J.bAnomaly&&!Specialized(J.Phenomenon))for(double At:Ticks)J.TraceTicks.Add(At);
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
   J.bCaseValid=S47J::Bool(O,TEXT("caseValid"),true);J.bCaseRecorded=S47J::Bool(O,TEXT("caseRecorded"));if(J.CaseTest>=0){J.bAnomaly=false;J.Phenomenon.Empty();}
   const auto ValidSamples=[](const TArray<double>& A){if(A.Num()>3)return false;for(double V:A)if(!FMath::IsFinite(V))return false;return true;};
   if(!ValidSamples(J.BeamAz)||!ValidSamples(J.BeamEl)||!ValidSamples(J.BeamAmplitude)){J.BeamAz.Reset();J.BeamEl.Reset();J.BeamAmplitude.Reset();J.bBeamVerified=false;J.bControl=false;J.ControlAt=0;}
   if(const auto C=S47J::Obj(O,TEXT("card"));C&&S47J::Int(C,TEXT("v"))==1)
   {
    auto Card=FS47FreeformJobs::Make(JobSeed,J.Day,J.Slot,J.Source);auto& P=Card.Receiver;
    P.F=SafeNum(C,TEXT("f"),P.F);P.FTol=SafeNum(C,TEXT("ftol"),P.FTol);
    P.Gain[0]=SafeNum(C,TEXT("g0"),P.Gain[0]);P.Gain[1]=SafeNum(C,TEXT("g1"),P.Gain[1]);P.Bw[0]=SafeNum(C,TEXT("bw0"),P.Bw[0]);P.Bw[1]=SafeNum(C,TEXT("bw1"),P.Bw[1]);P.Az[0]=SafeNum(C,TEXT("az0"),P.Az[0]);P.Az[1]=SafeNum(C,TEXT("az1"),P.Az[1]);P.Card=S47J::Str(C,TEXT("text"),P.Card);
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
 // Older saves: an already retained discrepancy opens its case retroactively.
 for(const auto& F:Findings){const int32 Line=FS47FreeformCases::LineForFinding(F.Key);if(Line>=0)Cases.Open(Line,FMath::Max(1,FindingDays.FindRef(F.Key)),20*3600.);}
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
 Director->Begin(S47J::Obj(Saved,TEXT("director")));
 IngestedObservations=0;IngestObservations();
 if(M&&M->Story){M->Story->SetClock(FMath::Fmod(WorldClock,86400.));M->Story->Rx.bPowered=true;}
 StepTutorial();ClearCompletedSelection();
 UpdateMysteries();
}
void FS47FreeformSession::Stop()
{
 StopEquipmentFeedback();StopPrinterFeedback();FeedbackEncoders.Reset();
 if(!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&IsValid(M->Audio)){M->Audio->ReceiverMonitorOff();M->Audio->ReceiverRecordingOff();}
 // UObject purge may destroy the scene before this plain C++ session is released.
 const bool bCanRestore=!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&IsValid(M->Story)&&IsValid(M->Web);
 if(bCanRestore&&bPowerApplied&&!bLastPower)M->Story->ApplyFeederPower(false);
 bPowerApplied=false;
 if(CaseField)CaseField->Stop();
 if(Director)Director->Stop();if(AudioDesk)AudioDesk->Stop();if(Field)Field->Stop();
 TerminalView.Reset();ActiveTerminal=-1;
 if(bCanRestore){M->Web->SetWorkMotionPaused(false);if(bActive)M->Web->SetPrinterPaper(-1.f);}
 bActive=bTelemetry=bAudioOpen=bAudioPlaying=bRecovering=false;++RecoveryEpoch;
}
bool FS47FreeformSession::Held() const
{
 return !bActive||bDead||bRecovering||!M||M->Mode!=US47Main::EMode::Play||M->bPausedByMenu||(M->ModalOpen()&&!TerminalOnGlass())||(M->World&&M->World->bBusy);
}
bool FS47FreeformSession::ArrayMotionHeld() const
{
 return !bActive||bDead||bRecovering||!M||M->bPausedByMenu||(M->World&&M->World->bBusy)||(M->ModalOpen()&&!bTelemetry&&!TerminalOnGlass())||!HasPower();
}
bool FS47FreeformSession::HasPower() const {return !bPowerFault||(Field&&Field->HasBackupPower());}
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
double FS47FreeformSession::ArrayMeasurementRate()const{return MeasurementRateForCount(AvailableDishCount());}
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
 const double Reception=FMath::Clamp(Sum/27.,0.,1.);
 return PointReady()?Reception*(.25+.75*FMath::Clamp(Settle,0.,1.)):FMath::Min(.6,Reception);
}
FString FS47FreeformSession::ArrayMeasurementStatus()const
{
 const int32 N=AvailableDishCount();
 return FString::Printf(TEXT("ARRAY %d/27 / MEASUREMENT RATE %.0f%% / %s"),N,100*MeasurementRateForCount(N),N<24?TEXT("BLOCKED: inspect ARRAY STATUS (left-hand PC); need 24"):N<27?TEXT("DEGRADED: repair restores rate"):TEXT("FULL RATE"));
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
 return Count>=24;
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
 }
 if(!ArrayMotionHeld())StepWork(D);
 if(AudioDesk)AudioDesk->Step(D);
 if(bAudioOpen&&!Held())StepAudio(D);
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
  if(Listen)M->Audio->MonitorReceiver(M->Story->Rx,EffectiveProfile(*Job),FString::Printf(TEXT("freeform-source-%d"),Job->Source),ReceiverReception());
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
   if(bCrossRunning)
   {
    CrossSample+=Dt*ArrayMeasurementRate();if(CrossSample>=2){if(CrossPoint<4){++CrossPoint;PointCross();}else bCrossRunning=false;}
   }
   if(J->Source==1&&M->Audio)
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
   J->ControlAt=FMath::Min(8.,J->ControlAt+Dt);
   if(J->ControlAt>=8)
   {
    J->ControlReferenceClock=WorldClock;J->ControlReceiverClock=WorldClock+(J->Phenomenon==TEXT("time_disagreement")?3.2:0);J->bClockMeasured=J->Phenomenon==TEXT("time_disagreement");
    if(J->bClockMeasured)Changed(TEXT("OPS (corner PC): DAILY WORK / Select this job and press Compare and register the retained instrument measurements to compare original and control clock offsets."));
    else if(J->Phenomenon==TEXT("duplicate_print"))Changed(TEXT("PRINTER: Register check complete; duplicate feed queued. Wait for the printer to finish, then collect and compare it with the retained original at OPS (corner PC)."));
    else Changed(TEXT("REEL DECK (beside telephone): Review the original and control recordings; the internal control is complete."));
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
 if(J->Stage==EJobStage::Chart||J->Stage==EJobStage::Packet||J->Stage==EJobStage::Filed)return JobNextAction(*J);
 if(!HasPower()){if(PrinterProgress()>=1)return PrinterHint();return TEXT("SERVICE BAY: Restore the SARO feeder or start the fuelled backup generator.");}
 if(J->Stage>=EJobStage::Chart)return JobNextAction(*J);
 // A case test states its physical precondition before the player commits the array.
 if(J->CaseTest>=0&&J->Stage<=EJobStage::Pointed){const FString Missing=CasePrecondition(*J);if(!Missing.IsEmpty())return Missing;}
 if(M&&M->Web&&J->Stage<=EJobStage::Integrating&&AvailableDishCount()<24)return ArrayMeasurementStatus();
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
 J->Stage=EJobStage::Integrating;J->Elapsed=0;J->bAnomaly=J->Slot==1&&bAnomaly&&J->CaseTest<0;J->Phenomenon=J->bAnomaly?PhenomenonId:FString();
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
 J->RecordingSoundVersion=1;J->RecordingGain=FMath::Clamp(M->Story->Rx.Gain,0.,100.);J->RecordingBandwidth=FMath::Clamp(M->Story->Rx.Bandwidth,0.,100.);
 J->RecordingQuality=FMath::Clamp(M->Story->Rx.Quality(EffectiveProfile(*J))*ReceiverReception(),0.,1.);
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
 if(J->Phenomenon==TEXT("wrong_source")){J->BeamPoint=0;J->BeamAz.Reset();J->BeamEl.Reset();J->BeamAmplitude.Reset();J->bBeamVerified=false;PointBeamControl();Changed(TEXT("TRACKING (right-hand PC): West, east and centre are measured automatically. Wait for completion, then open DAILY WORK at OPS (corner PC), select this job and press Compare and register the retained instrument measurements."));}
 else Changed(J->Phenomenon==TEXT("duplicate_print")?TEXT("RECEIVER RACK: Checking the master register automatically. When complete, collect the duplicate at the PRINTER; the original is retained."):TEXT("RECEIVER RACK: Eight-second internal reference running automatically; the original is retained. When complete, follow the job hint to OPS for clock measurements or the REEL DECK beside the telephone for audio comparison."));return true;
}
void FS47FreeformSession::SeekAudio(double At){if(!FMath::IsFinite(At))return;AudioCursor=FMath::Clamp(At,0.,AudioSourceIndex==0?18.:8.);bAudioPlaying=false;Changed();}
bool FS47FreeformSession::AudioAction(const FString& Action)
{
 if((Action==TEXT("play")||Action==TEXT("confirm"))&&!RequirePower())return false;
 FJob* J=Job();if(!J||!J->bAnomaly||J->ControlAt<8)return false;
 if(Action==TEXT("original")||Action==TEXT("control")){AudioSourceIndex=Action==TEXT("original")?0:1;AudioCursor=0;bAudioPlaying=false;}
 else if(Action==TEXT("play")){if(!bAudioOpen)return false;if(AudioCursor>=(AudioSourceIndex==0?18.:8.))AudioCursor=0;bAudioPlaying=true;}
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
  if(J->MarkStart<0||J->MarkStart>9.30||J->MarkEnd<12.60||J->MarkEnd-J->MarkStart>6){Feedback=TEXT("Mark both short groups of extra ticks, between about 9 and 13 seconds.");return false;}
  J->bAudioReviewed=true;RecordEvidence(TEXT("pulse-review"),TEXT("Original and internal reference reviewed. Extra ticks are absent from the short control; source remains unresolved."));
  if(Director)Director->Use(TEXT("ff:compare"));Changed(TEXT("OPS (corner PC): Register a supported conclusion; the marked comparison is retained."));
 }
 else return false;
 Changed();return true;
}
void FS47FreeformSession::StepAudio(double Dt)
{
 if(!HasPower()){bAudioPlaying=false;return;}
 FJob* J=Job();if(!J||!bAudioPlaying||!bAudioOpen||!M||!AudioDesk)return;
 if(!AudioDesk->ListeningPosition()){bAudioPlaying=false;Changed(TEXT("Playback paused because you left the reel deck. Return and press PLAY to resume."));return;}
 const double Was=AudioCursor,Duration=AudioSourceIndex==0?18.:8.;AudioCursor=FMath::Min(Duration,Was+Dt);
 const bool Listening=AudioDesk->ListeningPosition();
 double& Covered=AudioSourceIndex==0?J->OriginalCovered:J->ControlCovered;if(Listening&&Was<=Covered+1e-7)Covered=FMath::Max(Covered,AudioCursor);
 if(M->Audio&&Listening)
 {
  if(J->RecordingSoundVersion==1)M->Audio->MonitorReceiverRecording(FString::Printf(TEXT("freeform-source-%d"),J->Source),J->RecordingGain,J->RecordingBandwidth,J->RecordingQuality,AudioCursor);
  else M->Audio->ReceiverRecordingOff(); // Legacy evidence keeps its original beep-only reconstruction.
  for(double At=PulsarPeriod;At<=Duration;At+=PulsarPeriod)if(Was<At&&AudioCursor>=At)M->Audio->Beep(820,.012f,.16f,0,TEXT("square"));
  if(AudioSourceIndex==0)for(double At:J->TraceTicks)if(Was<At&&AudioCursor>=At)M->Audio->Beep(1240,.008f,.1f,0,TEXT("square"));
 }
 if(AudioCursor>=Duration){bAudioPlaying=false;Changed();}
}
double FS47FreeformSession::AudioLevel()const
{
 if(!bAudioPlaying)return .05;double L=.08;
 for(double At=PulsarPeriod;At<=18;At+=PulsarPeriod)L=FMath::Max(L,FMath::Max(0.,1-FMath::Abs(AudioCursor-At)/.06));
 if(AudioSourceIndex==0&&CurrentJob())for(double At:CurrentJob()->TraceTicks)L=FMath::Max(L,FMath::Max(0.,.8-FMath::Abs(AudioCursor-At)/.05));return L;
}
FString FS47FreeformSession::AudioDisplay()const{return FString::Printf(TEXT("%s %04.1f/%02d %s"),AudioSourceIndex==0?TEXT("ORG"):TEXT("REF"),AudioCursor,AudioSourceIndex==0?18:8,bAudioPlaying?TEXT("PLAY"):CurrentJob()&&CurrentJob()->bLegacyTrace?TEXT("PAUSE / LEGACY RECONSTRUCTION"):TEXT("PAUSE"));}
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
 if(M&&M->Audio)M->Audio->PcTransfer();
 if(!J->bAnomaly){if(!J->bPaid){J->bPaid=true;Earn(20);}J->Stage=EJobStage::Filed;}
 if(J->CaseTest>=0&&!J->bCaseRecorded){RecordCaseResult(*J);ClearCompletedSelection();return true;}
 const bool WasTutorial=InTutorial();StepTutorial();ClearCompletedSelection();
 if(WasTutorial&&!InTutorial())return true;
 Feedback=J->Stage==EJobStage::Registered?TEXT("Registered. Packet feed queued; wait for the printer to finish, collect the labelled packet, then file its originals in the binder."):AlreadyPaid?TEXT("Reference check registered. Original retained; no additional payment is due."):TEXT("Routine result registered. $20 credited; the original remains in EARLIER WORK and the notebook.");Changed(Feedback);return true;
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
 FJob* J=Job();if(!J||J->Stage!=EJobStage::Packet)return false;
 if(!NearBinder()){Feedback=TEXT("Take the registered paper packet to the actual records-room binder. The remote archive is read-only.");Changed(Feedback);return false;}
 J->Stage=EJobStage::Filed;if(!J->bPaid){J->bPaid=true;Earn(20);}
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
  if(!bEmergency||Loan>0){Feedback=TEXT("Insufficient budget. Complete reference work or request the one outstanding emergency requisition.");return false;}
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
 Findings.Add(Id,Text);FindingDays.Add(Id,DayNumber);
 if(M&&M->Story)
 {
  FS47Doc D;D.Id=TEXT("freeform-")+Id;D.Title=TEXT("Field record: ")+Id;D.Kind=TEXT("fieldnote");D.Stamp=FString::Printf(TEXT("SARO / DAY %d / %s"),DayNumber,*Id);
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
    if(!bOriginal&&!bFollowup)continue;
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
  bTelemetry=false;TerminalView.Reset();ActiveTerminal=-1;Changed();
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
     A.Add(T::Small(TEXT("Compare the original with the internal reference at the reel deck beside the telephone before registering.")));
     A.Add(T::Small(TEXT("A quiet internal reference does not prove a sky source. Retain the unresolved result; a directional or independent field control is still needed.")));
     A.Add(T::Button(TEXT("Source unresolved; retain both measurements"),[this]{RegisterComparison(TEXT("unresolved"));OpenOperations();},true));
    }
   }
   else A.Add(T::Button(J->CaseTest>=0&&!J->bCaseRecorded?TEXT("Register observation and case test result / $20"):J->bPaid?TEXT("Register retained reference / no additional payment"):TEXT("Register routine result / $20"),[this]{RegisterComparison(TEXT("normal"));OpenOperations();},true));
  }
  return S47Kit::Col(A);
 };
 int32 Remaining=0,TodayRemaining=0,EarlierRemaining=0;
 for(const auto& X:Work)if(X.Stage!=EJobStage::Filed){++Remaining;if(X.Day==DayNumber)++TodayRemaining;else if(X.Day<DayNumber)++EarlierRemaining;}
 const FString QueueExplanation=Remaining==0
  ?FString(TEXT("ALL OBSERVING WORK COMPLETE / Results are retained in EARLIER WORK and CASE ARCHIVE. "))+AvailableActivities()
  :FString::Printf(TEXT("MORE WORK / %d unfinished today, %d carried over. Each morning at 08:00 brings three observing jobs, including a choice for the third. Select another row in DAILY WORK; SHOW EARLIER WORK includes completed results immediately and untouched older jobs. Finish any active recording or control before switching. Unfinished jobs remain available."),TodayRemaining,EarlierRemaining);
 const FJob* J=CurrentJob();
 auto Account=T::Panel(TEXT("STATION ACCOUNT"),S47Kit::Col({
  T::Value(TEXT("OPERATING BUDGET"),FString::Printf(TEXT("$%d"),Cash)),T::Value(TEXT("REQUISITION"),FString::Printf(TEXT("$%d"),Loan)),
  T::Value(TEXT("SELECTED JOB"),FieldFocus>=0?FString(TEXT("FIELD CASE: "))+FS47FreeformCases::Def(FieldFocus).Title:J?FString(FS47Observing::Runs()[J->Source].Source):FString(TEXT("NONE")),true),
  T::Small(TEXT("NEXT STEP")),T::Banner(TAttribute<FString>::CreateLambda([this]{return Objective().ToUpper();}),true)}));
 if(Section==0)
 {
  if(Tutorial==0)
  {
   TArray<S47Kit::FW> Continue;
   const int32 Next=J&&J->Stage!=EJobStage::Filed&&!J->bDuplicateRejected?Selected:Work.IndexOfByPredicate([](const FJob& X){return X.Stage!=EJobStage::Filed&&!X.bDuplicateRejected;});
   if(Work.IsValidIndex(Next))
   {
    const FJob& NextJob=Work[Next];
    Continue.Add(T::Small(TEXT("A retained observation is available now; unfinished work does not expire at dusk.")));
    Continue.Add(T::Button(TEXT("Continue observation / ")+NextJob.Id+TEXT(" / ")+FS47Observing::Runs()[NextJob.Source].Source,[this,Next]
    {
     if(SelectJob(Next)){OpsSection=1;OpenOperations();}
    },true));
   }
   else Continue.Add(T::Small(Remaining>0?TEXT("Only unverifiable legacy observations remain in this queue. Their originals stay retained without a finding or payment. Photo preparation is available below; new daily observations arrive at 08:00."):TEXT("Daily observations are complete. Start the optional reference check below to retain a fresh baseline without extra payment.")));
   FString Photo;
   const FS47Album* Album=M&&M->Night?M->Night->Album.Get():nullptr;
   if(Album&&Album->PendingLabPhoto())Photo=TEXT("PHOTO LAB / WET BENCH: develop, fix and collect your exposed photograph. Keep its original before submitting it at OPS.");
   else if(M&&M->FieldCamera&&!M->FieldCamera->bHave)Photo=TEXT("PHOTO LAB / ARCHIVE BENCH: collect the camera before going out. The reserve-film carton is beside it; prepare for an observation whenever you choose.");
   else if(Album&&Album->Left()<=0)Photo=InventoryCount(TEXT("film"))>0?TEXT("PHOTO LAB / RESERVE-FILM CARTON: load one stock roll into your camera; retained photographs remain unchanged."):TEXT("CALICHE STORE: buy a film roll and deliver it to GARAGE RECEIVING; then load it at PHOTO LAB. Your existing photographs remain retained.");
   else if(Album)Photo=FString::Printf(TEXT("CAMERA READY / %d exposures remain. You can take a field photograph now; return to PHOTO LAB to develop it before OPS registration. A photograph does not guarantee a paid finding."),Album->Left());
   if(!Photo.IsEmpty())Continue.Add(T::Small(Photo));
   Rows.Add(T::Panel(TEXT("CONTINUE TODAY"),S47Kit::Col(Continue)));
  }
  Rows.Add(T::Small(QueueExplanation));
  if(Remaining==0)Rows.Add(T::Button(TEXT("Start an optional reference check / no payment"),[this]{StartReferenceCheck();OpsSection=1;OpenOperations();},true));
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
  const int32 PlanLine=FieldFocus>=0?FieldFocus:J?J->CaseLine:-1;double Target=0;
  if(CaseWaitTarget(PlanLine,Target))Rows.Add(T::Button(TEXT("Wait until five minutes before this test / review consequences"),[this,Target]{OpenWaitConfirmation(Target);},true));
  Rows.Add(T::Columns({{.5f,T::Big(FString(TEXT("REST UNTIL MORNING")),[this]{OpenWaitConfirmation((FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay)+1)*SecondsPerDay+8*3600);})},{.5f,T::Big(FString(TEXT("SERVICE ORDER")),[this]{OpenServiceGuide();})}},10));
  OpenTerminal(3,TEXT("OPS / MAIN MENU"),OpsShell(0,S47Kit::Col(Rows)));
  return;
 }
 if(Section==1)
 {
  Rows.Add(T::Small(QueueExplanation));
  if(Remaining==0)Rows.Add(T::Button(TEXT("Start an optional reference check / no payment"),[this]{StartReferenceCheck();OpenOperations();},true));
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
   if(Shot&&!Shot->IsDeveloped())Claims.Add(T::Small(TEXT("UNDEVELOPED ORIGINAL / PHOTO LAB: Develop, fix and collect this photograph at the wet bench before submitting it.")));
   else if(!Shot||Shot->bUnavailable||Shot->Url.IsEmpty())Claims.Add(T::Small(TEXT("ORIGINAL UNAVAILABLE / This photograph cannot be submitted. Existing incident records are retained.")));
   else
   {
    if(!Claim.bPaid)Claims.Add(T::Button(FString::Printf(TEXT("Register photograph / $%d documentation credit"),Claim.Reward),[this,Id]{Director->RegisterPhotoClaim(Id);OpenOperations();},true,CanRegisterPhotoEvidence()));
    else Claims.Add(T::Small(TEXT("ORIGINAL REGISTERED / Documentation credit already received.")));
    if(Orb)
    {
     const bool Planned=Claim.ControlPlannedAt>=0;
     const auto& AlbumShots=M->Night->Album->Shots();
     const auto* Baseline=Planned?AlbumShots.FindByPredicate([&](const FS47AlbumShot& S){return S.Id==Claim.ControlOriginalId;}):Shot;
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
      if(Claim.ControlPhotoId.IsEmpty())Claims.Add(T::Button(TEXT("Retain comparison / no payment"),[this,Id,ControlId]{if(Director)Director->RegisterPhotoControl(Id,ControlId);OpenOperations();},false,CanRegisterPhotoEvidence()));
      else Claims.Add(T::Small(TEXT("COMPARISON RETAINED / Both originals unchanged. Identity and shared cause unresolved.")));
     }
     else
     {
      Claims.Add(PhotoCard(Baseline,TEXT("FIRST ORIGINAL")));
      Claims.Add(T::Small(Director->PhotoControlHint(Id)));
      if(!Planned&&Shot->Pose.IsSet())Claims.Add(T::Button(TEXT("Plan return to original viewpoint / no payment"),[this,Id]{if(Director)Director->PlanPhotoControl(Id);OpenOperations();},false,CanRegisterPhotoEvidence()));
      if(Planned)Claims.Add(T::Small(TEXT("NEXT / After the passage ends, take the matching view and develop the separate original at PHOTO LAB. Compare both here.")));
     }
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
 auto Plot=T::Panel(FString::Printf(TEXT("AZ / EL PLOT (TARGET %s)"),*Name),T::SkyPlot([this]{return FVector2D(TargetAz(),TargetEl());},ArrayAzEl,Name),true);
 auto Current=T::Panel(TEXT("CURRENT (ARRAY)"),S47Kit::Col({T::Value(TEXT("AZ"),TAttribute<FString>::CreateLambda([ArrayAzEl]{return FString::Printf(TEXT("%.1f°"),ArrayAzEl().X);}),true),T::Value(TEXT("EL"),TAttribute<FString>::CreateLambda([ArrayAzEl]{return FString::Printf(TEXT("%.1f°"),ArrayAzEl().Y);}),true)}),true);
 auto Target=T::Panel(TEXT("TARGET"),S47Kit::Col({T::Value(TEXT("AZ"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.1f°"),TargetAz());}),true),T::Value(TEXT("EL"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.1f°"),TargetEl());}),true)}),true);
 TArray<S47Kit::FW> Right={Current,Target,
  T::Banner(TAttribute<FString>::CreateLambda([this]{const FString State=PointReady()?Settle>=1?TEXT("ON TARGET / STEADY"):TEXT("ON TARGET / SETTLING"):CurrentJob()&&CurrentJob()->Stage==EJobStage::Pointed?TEXT("ARRAY MOVING"):TEXT("READY TO POINT");return State+TEXT("\n")+ArrayMeasurementStatus();}),TAttribute<bool>::CreateLambda([this]{return !PointReady();})),
  T::Big(FString(TEXT("POINT ARRAY")),[this]{if(PointSelected())M->UI->Close();},1,CurrentJob()->Stage<=EJobStage::Pointed)};
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
  Rows.Add(T::Panel(TEXT("RECORD / THIS JOB"),S47Kit::Col({
   T::Value(TEXT("JOB"),J->Id,true),T::Value(TEXT("STAGE"),TAttribute<FString>::CreateLambda([this]{return CurrentJob()?StageText(CurrentJob()->Stage):FString();}),true),
   T::Value(TEXT("RECORDED"),TAttribute<FString>::CreateLambda([this]{const auto* X=CurrentJob();return FString::Printf(TEXT("%.1f / %.0f s"),X?X->Elapsed:0.,SourceSeconds());}),true),
   T::Value(TEXT("ORIGINAL"),J->bPrinted?FString(TEXT("COLLECTED / IN NOTEBOOK")):J->Stage==EJobStage::Chart?FString(TEXT("WAITING AT PRINTER")):FString(TEXT("NOT YET RECORDED")),true),
   T::Small(TEXT("Originals print at the table by the service-yard door. Valid recorded seconds are retained if tuning or pointing is lost."))})));
  Rows.Add(Banner());
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
 if(!AudioDesk->ListeningPosition()){if(M&&M->UI)M->UI->Toast(TEXT("Use the reel deck beside the telephone: ORIGINAL / CONTROL, PLAY, MARK IN / OUT and RETAIN."),5);return;}
 bAudioOpen=true;
}
void FS47FreeformSession::OpenArchive()
{
 if(!M||!M->UI)return;const bool Computer=TerminalOnGlass();TArray<S47Kit::FW> Rows;
 auto Paper=[Computer](const FString& T){return Computer?S47FreeformTerminal::Paper(T):S47Kit::Paper(T);};
 Rows.Add(Paper(TEXT("SARO / CONTINUING CASE ARCHIVE\nObservations, independent controls and hypotheses are kept separately.")));
 static const TCHAR* Names[]={TEXT("The repeated interval"),TEXT("Visitors around the base"),TEXT("Disagreeing records")};
 for(int32 I=0;I<3;++I)if(MysterySteps[I]>0)Rows.Add(S47Kit::Small(FString::Printf(TEXT("%s / %d of 3 / %s"),Names[I],MysterySteps[I],MysterySteps[I]==3?TEXT("CROSS-DAY COMPARISON AVAILABLE"):TEXT("FOLLOW-UP OPEN"))));
 TSharedRef<SScrollBox> List=SNew(SScrollBox);
 const bool HasOriginal=Work.ContainsByPredicate([](const FJob& J){return J.bPrinted||!J.OriginalPage.IsEmpty();});
 if(!HasOriginal&&Findings.IsEmpty())
  List->AddSlot()[Paper(TEXT("NO RETAINED RECORDS YET\nSelect DAILY WORK at OPS, complete an observation and collect its original at the printer. Retained originals and field findings appear here automatically. Register routine results at OPS; an anomaly packet must also be collected and physically filed in the records-room binder. CASE BOARD holds open investigations; this archive does not issue new jobs."))];
 for(const FJob& J:Work)if(J.bPrinted||!J.OriginalPage.IsEmpty())
 {
  const FString Control=J.bControl?FString::Printf(TEXT("INDEPENDENT CONTROL %.1f/8 s"),J.ControlAt):FString(TEXT("CONTROL NOT RECORDED"));
  const FString Mark=J.MarkStart>=0&&J.MarkEnd>J.MarkStart?FString::Printf(TEXT(" / MARK %.2f–%.2f s"),J.MarkStart,J.MarkEnd):FString();
  const FString Conclusion=!J.bAnomaly?TEXT("ROUTINE RESULT"):!J.bAudioReviewed?TEXT("COMPARISON PENDING"):J.Phenomenon==TEXT("wrong_source")?TEXT("OFF-SOURCE RESPONSE"):J.Phenomenon==TEXT("time_disagreement")?TEXT("CLOCK DISAGREEMENT"):J.Phenomenon==TEXT("duplicate_print")?TEXT("DUPLICATE DIFFERS"):TEXT("SOURCE UNRESOLVED");
  List->AddSlot()[Paper(FString::Printf(TEXT("%s / ORIGINAL %s-original\n%s%s / %s / %s"),*J.Id,*J.Id,*Control,*Mark,*Conclusion,*StageText(J.Stage)))];
 }
 TArray<FString> Keys;Findings.GetKeys(Keys);Keys.Sort();
 for(const FString& K:Keys)
 {
  const bool Investigate=K.StartsWith(TEXT("extra_pulse"))||K.StartsWith(TEXT("feeder_fault"))||K.StartsWith(TEXT("wrong_source"))||K.StartsWith(TEXT("roof_steps"))||K.StartsWith(TEXT("observer"))||K.StartsWith(TEXT("landing_trace"))||K.StartsWith(TEXT("time_disagreement"))||K.StartsWith(TEXT("duplicate_print"))||K.StartsWith(TEXT("unsigned_telex"));
  FString Entry=FString::Printf(TEXT("D%d / %s\nRETAINED RECORD / %s"),FindingDays.FindRef(K),*K,*Findings.FindRef(K));
  if(Investigate)Entry+=TEXT("\nSTILL UNKNOWN / Whether this repeats under independent conditions.\nPOSSIBLE NEXT CHECK / Retain a later observation and its independent control; compare with this original.");
  List->AddSlot()[Paper(Entry)];
 }
 Rows.Add(SNew(SBox).HeightOverride(260)[List]);
 if(CurrentJob()&&CurrentJob()->Stage==EJobStage::Packet)
 {
  if(NearBinder())Rows.Add(S47Kit::Button(TEXT("Place registered originals in this binder"),[this]{FilePacket();OpenArchive();},true));
  else Rows.Add(S47Kit::Small(TEXT("Registered packet awaiting physical filing. Take its originals to the records-room binder; this remote archive is read-only.")));
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
 if(Director&&Director->Busy())Text+=TEXT("\nBLOCKED / An encounter is in progress; reach safety before waiting.");
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
bool FS47FreeformSession::WaitUntil(double Target)
{
 if(!bActive||bDead||bRecovering||!FMath::IsFinite(Target)||Target-WorldClock>2*SecondsPerDay)return false;
 if(Target<=WorldClock){Feedback=TEXT("That waiting time has already passed. Check the current window on CASE BOARD; no time was skipped.");Changed(Feedback);return false;}
 if(Held()&&!TerminalOnGlass())return false;
 if((Director&&Director->Busy())||Work.ContainsByPredicate([](const FJob& J){return J.Stage==EJobStage::Integrating||(J.bControl&&J.ControlAt<8);}))
 {Feedback=TEXT("Finish the active observation, control or encounter before waiting.");Changed(Feedback);return false;}
 WorldClock=Target;DayNumber=FMath::Max(1,int32(FMath::FloorToDouble((WorldClock-8*3600.)/SecondsPerDay))+1);AddDay(DayNumber);Settle=0;bAudioPlaying=false;
 // Settle unsampled time before any save request, even while the confirmation
 // screen is still modal. No large Dt is passed through observing work.
 if(Field)Field->AdvanceUnobservedTo(WorldClock);
 if(CaseField)CaseField->AdvanceUnobservedTo(WorldClock);
 if(M&&M->Story)M->Story->SetClock(FMath::Fmod(WorldClock,SecondsPerDay));
 if(M&&M->UI)M->UI->Close();
 Feedback=TEXT("Waiting ended. Unfinished work and originals are retained. ")+NextAction();Changed(Feedback);return true;
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
 const FString Unavailable=FS47FreeformCases::UnavailableReason(Line,Test);
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
FString FS47FreeformSession::AvailableActivities()const
{
 int32 Pending=0;for(const FJob& J:Work)if(J.Stage!=EJobStage::Filed)++Pending;
 if(Pending>0)return FString::Printf(TEXT("OPS (corner PC), DAILY WORK: SELECT ONE OF %d UNFINISHED OBSERVATIONS; TRACK, TUNE AND REGISTER THE NEXT RESULT. COMPLETED RESULTS ARE IN EARLIER WORK."),Pending);
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)if(const auto* C=Cases.Get(Line);C&&C->bOpen&&!C->bConcluded)
  return C->Planned>=0?TEXT("OPS / CASE BOARD: FOLLOW THE RETAINED PLAN FOR ")+FString(FS47FreeformCases::Def(Line).Title):TEXT("OPS / CASE BOARD: CHOOSE A CONTROL FOR ")+FString(FS47FreeformCases::Def(Line).Title);
 if(M&&M->Night&&M->Night->Album&&M->Night->Album->PendingLabPhoto())return TEXT("PHOTO LAB: DEVELOP, FIX AND COLLECT YOUR EXPOSED PHOTOGRAPH AT THE WET BENCH");
 if(ServiceGuide==1)return ServiceObjective();
 if(Field&&Field->LoggerAccess())return Field->FieldLogger().bLoanIssued?TEXT("EAST WING / CALIBRATION BENCH: PREPARE THE LOAN LOGGER FOR A TWO-HOUR LOCAL-NOISE COMPARISON; KEEP A MATCHING SARO ORIGINAL"):TEXT("EAST WING / PARTS STORAGE: COLLECT YOUR FIRST LOAN LOGGER AT EQUIPMENT CHECKOUT; THEN PREPARE IT AT THE CALIBRATION BENCH");
 return TEXT("OPS / DAILY WORK: START AN OPTIONAL REFERENCE CHECK TO RETAIN A FRESH INSTRUMENT BASELINE (NO PAYMENT). PHOTO LAB AND ARRAY SERVICE ARE ALSO AVAILABLE; REST IS OPTIONAL.");
}
bool FS47FreeformSession::StartReferenceCheck()
{
 if(!bActive||Tutorial>0||!RequirePower()||Work.ContainsByPredicate([](const FJob& J){return J.Stage!=EJobStage::Filed;}))return false;
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
 if(Line<0||!Cases.Open(Line,DayNumber,FMath::Fmod(WorldClock,86400.)))return;
 Changed(TEXT("New case opened: ")+FString(FS47FreeformCases::Def(Line).Title)+TEXT(". Three explanations fit. Choose a test on the OPS case board."));
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
  const FString Unavailable=FS47FreeformCases::UnavailableReason(Line,Test);
  if(!Unavailable.IsEmpty()){Feedback=Unavailable;return false;}
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
 if(!Cases.Record(Line,Test,DayNumber,J.bCaseValid,bPresent,First)){Changed(TEXT("Routine result registered. $20 credited; the original remains in the notebook."));return;}
 if(!J.bCaseValid){Changed(TEXT("Observation registered, $20 credited. The case test is INVALID because its condition was broken; plan it again on the case board."));return;}
 const auto& D=FS47FreeformCases::Def(Line);
 TArray<FString> Out;for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)if(FS47FreeformCases::PresentFor(Line,I,Test)!=bPresent)Out.Add(D.Causes[I]);
 const FString Result=FS47FreeformCases::ResultText(Line,bPresent);
 RecordEvidence(FString::Printf(TEXT("case%d-test%d"),Line+1,Test+1),FString::Printf(TEXT("CASE / %s\nTEST / %s / DAY %d / ORIGINAL %s-original\nRESULT / %s\nPREDICTION / %s\nRULES OUT / %s"),D.Title,*FString(D.Tests[Test].Name).ToUpper(),DayNumber,*J.Id,*Result,*FS47FreeformCases::Prediction(Line,Test),*FString::Join(Out,TEXT(", "))));
 FString Text=FString::Printf(TEXT("Case test registered, $20 credited. %s: rules out %s."),*Result,*FString::Join(Out,TEXT(" and ")));
 if(First){AddItem(TEXT("relay"),1);Text+=TEXT(" Station management credits one service relay to the depot for documented test work.");}
 if(Cases.Remaining(Line)==1)Text+=TEXT(" Only one explanation survives: register the conclusion on the case board.");
 Changed(Text);
}
bool FS47FreeformSession::ConcludeCase(int32 Line,int32 Cause)
{
 FString Why;if(!Cases.Conclude(Line,Cause,Why)){Feedback=Why;Changed(Why);return false;}
 if(FieldFocus==Line)FieldFocus=-1;
 const auto& D=FS47FreeformCases::Def(Line);const FString Name=D.Causes[Cause];
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
  if(C->Planned<0){Out.Add(FString::Printf(TEXT("CASE / %s / NO TEST PLANNED\n%d of 3 explanations still possible. Open the case board and choose the next test."),D.Title,Cases.Remaining(Line)));continue;}
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
  Rows.Add(S47FreeformTerminal::Paper(FString::Printf(TEXT("CASE %d / %s\nOPENED DAY %d FROM THE RETAINED ORIGINAL AT %s\n%s"),Line+1,D.Title,C->OpenDay,*FS47FreeformCases::Clock(C->OpenClock),D.Question)));
  for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)
  {
   const bool Out=Cases.Excluded(Line,I);
   const FString State=C->bConcluded?(C->Conclusion==I?TEXT("CONCLUDED / SUPPORTED BY YOUR TESTS"):TEXT("RULED OUT")):Out?TEXT("RULED OUT"):TEXT("STILL POSSIBLE");
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
  if(!C->Leads.IsEmpty()&&Director)
  {
   FString Leads=TEXT("LEADS / retained observations, not yet one cause");
   for(const auto& O:Director->Observations())if(C->Leads.Contains(O.Id))Leads+=FString::Printf(TEXT("\nD%d %s / %s / bearing %s / %s%s"),O.Day,*FS47FreeformCases::Clock(O.Clock),*O.Place,*O.Bearing,O.bHeard?TEXT("heard"):TEXT("seen"),O.PhotoId.IsEmpty()?TEXT(""):TEXT(" / photographed"));
   List->AddSlot()[S47FreeformTerminal::Paper(Leads)];
  }
  for(int32 T=0;T<FS47FreeformCases::TestCount;++T)
  {
   const auto& Res=C->Results[T];
   const FString Unavailable=FS47FreeformCases::UnavailableReason(Line,T);
   FString Card=TEXT("TEST / ")+FString(D.Tests[T].Name).ToUpper()+TEXT("\n")+D.Tests[T].Instruction+TEXT("\nPREDICTION / ")+FS47FreeformCases::Prediction(Line,T);
   if(Res.bDone)Card+=FString::Printf(TEXT("\nRESULT DAY %d / %s"),Res.Day,*FS47FreeformCases::TestResultText(Line,T,Res.bPresent));
   else if(!Unavailable.IsEmpty())Card+=TEXT("\nUNAVAILABLE / ")+Unavailable+(C->Planned==T?TEXT(" Cancel this retained plan to choose another control."):TEXT(""));
   else if(D.Tests[T].Access&&!Cases.HasAccess(D.Tests[T].Access))Card+=TEXT("\nNEEDS / the loan field logger (conclude THE REPEATED INTERVAL)");
   else if(C->Planned==T)Card+=TEXT("\nPLANNED / ")+C->PlannedJob+TEXT("\n")+CaseWindowText(Line);
   List->AddSlot()[S47FreeformTerminal::Paper(Card)];
   if(Unavailable.IsEmpty()&&!Res.bDone&&!C->bConcluded&&C->Planned<0&&!(D.Tests[T].Access&&!Cases.HasAccess(D.Tests[T].Access)))List->AddSlot()[S47FreeformTerminal::Button(TEXT("Plan this test: ")+FString(D.Tests[T].Name),[this,Line,T]{PlanCaseTest(Line,T);OpenCaseBoard(Line);},true)];
  }
  if(PlannedFieldTest(Line)>=0)
  {
   List->AddSlot()[S47FreeformTerminal::Button(FieldFocus==Line?TEXT("FOLLOWING THIS FIELD TEST"):TEXT("Follow this field test / retain observing job"),[this,Line]{FocusFieldTest(Line);OpenCaseBoard(Line);},FieldFocus==Line)];
   List->AddSlot()[S47FreeformTerminal::Small(TEXT("Cancel clears only this field plan. No result or reward is recorded; retained evidence stays unchanged."))];
   List->AddSlot()[S47FreeformTerminal::Button(TEXT("Cancel this field plan"),[this,Line]{CancelFieldTest(Line);OpenCaseBoard(Line);},false)];
  }
  double WaitTarget=0;if(CaseWaitTarget(Line,WaitTarget))List->AddSlot()[S47FreeformTerminal::Button(TEXT("Wait until five minutes before this window / review consequences"),[this,WaitTarget]{OpenWaitConfirmation(WaitTarget);},true)];
  Rows.Add(SNew(SBox).HeightOverride(240)[List]);
  if(!C->bConcluded)
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
 if(Id==TEXT("decoder")||Id==TEXT("signalProcessor")||Id==TEXT("decoderDesk")){OpenAudio();return true;}
 if(Id==TEXT("rack"))
 {
  if(CurrentJob()&&CurrentJob()->Phenomenon==TEXT("wrong_source"))Changed(NextAction());
  else if(!StartControl())Changed(NextAction());return true;
 }
 if(Id==TEXT("printer")){Feedback.Empty();if(!CollectChart())Changed(Feedback.IsEmpty()?TEXT("No paper waiting. ")+NextAction():Feedback);return true;}
 if(Id==TEXT("binder")||Id==TEXT("excBinder")||Id==TEXT("serviceRecord")){OpenArchive();return true;}
 if(Id==TEXT("phone")||Id==TEXT("recPhone")){if(Director)Director->Use(TEXT("ff:phone"));Changed(TEXT("SARO operations line. No scheduled call. Incoming messages are retained at the telex."));return true;}
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
 if(Id==TEXT("decoder")||Id==TEXT("signalProcessor")||Id==TEXT("decoderDesk"))return TEXT("Reel deck by telephone / compare recordings");
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
 if(FieldFocus>=0)return FieldNextAction(FieldFocus);
 if(!HasPower()&&PrinterProgress()>=1)return PrinterHint();
 if(bPowerFault&&!HasPower()&&(!CurrentJob()||(CurrentJob()->Stage!=EJobStage::Chart&&CurrentJob()->Stage!=EJobStage::Packet)))return TEXT("SERVICE BAY: MEASURE THE FEEDER OR TRANSFER TO THE FUELLED GENERATOR");
 if(ServiceGuide==1&&bGuideFocus){const FString Guide=ServiceObjective();if(!Guide.IsEmpty())return Guide;}
 if(Tutorial>0){const FString T=TutorialObjective();if(!T.IsEmpty())return T;}
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
 if(J->CaseTest>=0&&J->Stage<=EJobStage::Pointed){const FString Missing=CasePrecondition(*J);if(!Missing.IsEmpty())return Missing;}
 if(M&&M->Web&&AvailableDishCount()<24&&(J->Stage<=EJobStage::Integrating||(J->Stage==EJobStage::Recorded&&J->Phenomenon==TEXT("wrong_source")&&(!J->bControl||J->ControlAt<8))))return ArrayMeasurementStatus();
 switch(J->Stage)
 {
  case EJobStage::New:return TEXT("TRACKING (right-hand PC): POINT THE ARRAY TO THE SELECTED SOURCE");
  case EJobStage::Pointed:if(&JobValue!=CurrentJob())return TEXT("TRACKING (right-hand PC): FOLLOW THIS JOB AND VERIFY ITS ACTUAL POINTING BEFORE RECORDING");return PointReady()&&Settle>=1?TEXT("RECEIVER (centre PC): TUNE THE CARD AND PRESS START RECORDING"):TEXT("TRACKING (right-hand PC): WAIT FOR ENCODER AGREEMENT, THEN USE RECEIVER (centre PC)");
  case EJobStage::Integrating:return TEXT("RECEIVER (centre PC): KEEP TUNING VALID; COLLECT THE ORIGINAL AT THE PRINTER WHEN RECORDING FINISHES");
  case EJobStage::Chart:return J->ChartFeed<PrinterSeconds?PrinterHint():FString(TEXT("PRINTER: RETAIN THE ORIGINAL OBSERVATION CHART"));
  case EJobStage::Recorded:
   if(J->bDuplicateRejected)return TEXT("OPS / DAILY WORK: SELECT ANOTHER OBSERVATION. LEGACY ORIGINAL RETAINED; UNVERIFIABLE DUPLICATE REMOVED FROM PRINTER QUEUE WITHOUT A FINDING OR PAYMENT.");
   if(!J->bAnomaly)return J->CaseTest>=0&&!J->bCaseRecorded?TEXT("OPS (corner PC): REGISTER THE ROUTINE RESULT AND ITS CASE TEST"):TEXT("OPS (corner PC): REGISTER THE ROUTINE RESULT");
   if(!J->bControl)return J->Phenomenon==TEXT("wrong_source")?TEXT("TRACKING (right-hand PC): MEASURE WEST, EAST AND CENTRE BEAMS"):TEXT("RECEIVER RACK: RECORD AN INDEPENDENT INTERNAL CONTROL");
   if(J->ControlAt<8)
   {
    if(J->Phenomenon==TEXT("wrong_source"))return TEXT("TRACKING (right-hand PC): WAIT FOR AUTOMATIC WEST, EAST AND CENTRE SAMPLES; THEN COMPARE AT OPS (corner PC), DAILY WORK");
    if(J->Phenomenon==TEXT("time_disagreement"))return TEXT("RECEIVER RACK: WAIT FOR THE AUTOMATIC INTERNAL CONTROL; THEN COMPARE CLOCK READINGS AT OPS (corner PC), DAILY WORK");
    if(J->Phenomenon==TEXT("duplicate_print"))return TEXT("RECEIVER RACK: WAIT FOR THE AUTOMATIC REGISTER CHECK; THEN WAIT FOR THE DUPLICATE TO FINISH AT THE PRINTER");
    return TEXT("RECEIVER RACK: WAIT FOR THE AUTOMATIC INTERNAL CONTROL; THEN REVIEW BOTH RECORDINGS AT THE REEL DECK BESIDE THE TELEPHONE");
   }
   if(J->Phenomenon==TEXT("duplicate_print")&&!J->bDuplicateCollected)return J->DuplicateFeed<PrinterSeconds?PrinterHint():FString(TEXT("PRINTER: COLLECT THE SEPARATE DUPLICATE WITHOUT ALTERING THE ORIGINAL; THEN COMPARE AT OPS (corner PC), DAILY WORK"));
   if(Specialized(J->Phenomenon))return J->bAudioReviewed?TEXT("OPS (corner PC): REGISTER THE SUPPORTED INSTRUMENTAL FINDING"):TEXT("OPS (corner PC): DAILY WORK / SELECT THIS JOB AND PRESS COMPARE AND REGISTER THE RETAINED MEASUREMENTS");
   return J->bAudioReviewed?TEXT("OPS (corner PC): REGISTER THE SUPPORTED CONCLUSION"):TEXT("REEL DECK (beside telephone): REVIEW BOTH RECORDINGS AND MARK THE EXTRA TICKS");
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

 if(M&&M->Web&&AvailableDishCount()<24&&(J.Stage<=EJobStage::Integrating||(J.Stage==EJobStage::Recorded&&J.Phenomenon==TEXT("wrong_source")&&(!J.bControl||J.ControlAt<8))))return TEXT("ARRAY STATUS / left-hand PC / inspect isolated antennas");
 if(J.Stage==EJobStage::New)return TEXT("Tracking PC / right-hand screen");
 if(J.Stage==EJobStage::Pointed)return &J==CurrentJob()&&PointReady()&&Settle>=1?TEXT("Receiver / centre screen"):TEXT("Tracking PC / right-hand screen");
 if(J.Stage==EJobStage::Integrating)return TEXT("Receiver / centre screen");
 if(J.Stage==EJobStage::Chart||J.Stage==EJobStage::Registered)return TEXT("Printer / table by SERVICE YARD door, beneath the binder shelf");
 if(J.Stage==EJobStage::Packet)return TEXT("Binder / records room");
 if(J.bDuplicateRejected)return TEXT("OPS / DAILY WORK: retained legacy observation; select another job");
 if(J.Stage==EJobStage::Recorded&&J.bAnomaly&&!J.bAudioReviewed)
 {
  if(!J.bControl||J.ControlAt<8)return J.Phenomenon==TEXT("wrong_source")?TEXT("Tracking PC / right-hand screen"):TEXT("Receiver rack / internal reference controls");
  if(J.Phenomenon==TEXT("duplicate_print")&&!J.bDuplicateCollected)return TEXT("Printer / table by SERVICE YARD door, beneath the binder shelf");
  if(!Specialized(J.Phenomenon))return TEXT("Reel deck / beside telephone");
 }
 return TEXT("OPS / corner PC / DAILY WORK");
}

TArray<FS47Task> FS47FreeformSession::Tasks()const
{
 TArray<FS47Task> Result;
 if(Director){bool Ready=false;const FString Hint=Director->PhotoControlFieldHint(Ready);if(!Hint.IsEmpty())Result.Add({Hint,false});}
 if(bPowerFault)Result.Add({TEXT("FEEDER / Measure, isolate and replace the control fuse; restore SARO power."),false});
 if(Field&&Field->HasArrayFault()&&ServiceGuide!=1)Result.Add({Field->ServiceDishId()+TEXT(" / Diagnose at the status PC, replace the relay and run the local out-and-back test."),false});
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)if(const auto* C=Cases.Get(Line);C&&C->bOpen&&!C->bConcluded)Result.Add({C->Planned>=0?(PlannedFieldTest(Line)>=0?FieldNextAction(Line):TEXT("CASE / ")+CaseTestName(Line,C->Planned)+TEXT(": run job ")+C->PlannedJob):TEXT("CASE / ")+FString(FS47FreeformCases::Def(Line).Title)+TEXT(": choose a test on the OPS case board"),false});
 if(ServiceGuide>0)Result.Add({ServiceGuide==2?FString(TEXT("Service work order / verified and archived")):ServiceObjective(),ServiceGuide==2});
 for(const FJob& J:Work)if(J.Day==DayNumber||J.Stage!=EJobStage::Filed)Result.Add({JobLine(J),J.Stage==EJobStage::Filed});
 return Result;
}
S47J::FObj FS47FreeformSession::Save()const
{
 auto O=S47J::New();S47J::Set(O,TEXT("fieldFocus"),FieldFocus);S47J::Set(O,TEXT("tutorial"),Tutorial);S47J::Set(O,TEXT("tutorialJobId"),TutorialJobId);S47J::Set(O,TEXT("jobSeed"),JobSeed);S47J::Set(O,TEXT("serviceGuide"),ServiceGuide);S47J::Set(O,TEXT("guideServiceBaseline"),GuideServiceBaseline);S47J::Set(O,TEXT("guideSupplyBaseline"),GuideSupplyBaseline);S47J::Set(O,TEXT("guideSupply"),bGuideSupply);S47J::Set(O,TEXT("guideFocus"),bGuideFocus);S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("clock"),WorldClock);S47J::Set(O,TEXT("cash"),Cash);S47J::Set(O,TEXT("loan"),Loan);S47J::Set(O,TEXT("lethal"),bDeadly);S47J::Set(O,TEXT("dead"),bDead);S47J::Set(O,TEXT("powerFault"),bPowerFault);S47J::Set(O,TEXT("anomaly"),bAnomaly);S47J::Set(O,TEXT("phenomenon"),PhenomenonId);
 S47J::Set(O,TEXT("journal"),Journal.Save());S47J::Set(O,TEXT("economyRevision"),2);
 S47J::Set(O,TEXT("selected"),Selected);S47J::Set(O,TEXT("audioSource"),AudioSourceIndex);S47J::Set(O,TEXT("audioCursor"),AudioCursor);S47J::Set(O,TEXT("settle"),Settle);S47J::Set(O,TEXT("cross"),CrossPoint);S47J::Set(O,TEXT("crossSample"),CrossSample);S47J::Set(O,TEXT("crossRunning"),bCrossRunning);
 TArray<TSharedPtr<FJsonValue>> Jobs;
 for(const FJob& J:Work)
 {
  auto A=S47J::New();S47J::Set(A,TEXT("chartFeed"),J.ChartFeed);S47J::Set(A,TEXT("packetFeed"),J.PacketFeed);S47J::Set(A,TEXT("duplicateFeed"),J.DuplicateFeed);S47J::Set(A,TEXT("id"),J.Id);S47J::Set(A,TEXT("day"),J.Day);S47J::Set(A,TEXT("slot"),J.Slot);S47J::Set(A,TEXT("source"),J.Source);S47J::Set(A,TEXT("phenomenon"),J.Phenomenon);S47J::Set(A,TEXT("stage"),int32(J.Stage));S47J::Set(A,TEXT("elapsed"),J.Elapsed);S47J::Set(A,TEXT("quality"),J.Quality);S47J::Set(A,TEXT("receivedLevel"),J.ReceivedLevel);S47J::Set(A,TEXT("anomaly"),J.bAnomaly);S47J::Set(A,TEXT("control"),J.bControl);S47J::Set(A,TEXT("reviewed"),J.bAudioReviewed);S47J::Set(A,TEXT("printed"),J.bPrinted);S47J::Set(A,TEXT("paid"),J.bPaid);S47J::Set(A,TEXT("controlAt"),J.ControlAt);S47J::Set(A,TEXT("originalCovered"),J.OriginalCovered);S47J::Set(A,TEXT("controlCovered"),J.ControlCovered);S47J::Set(A,TEXT("markStart"),J.MarkStart);S47J::Set(A,TEXT("markEnd"),J.MarkEnd);S47J::Set(A,TEXT("receiverClock"),J.ReceiverClock);S47J::Set(A,TEXT("referenceClock"),J.ReferenceClock);S47J::Set(A,TEXT("controlReceiverClock"),J.ControlReceiverClock);S47J::Set(A,TEXT("controlReferenceClock"),J.ControlReferenceClock);S47J::Set(A,TEXT("duplicateClock"),J.DuplicateClock);S47J::Set(A,TEXT("duplicateInterval"),J.DuplicateInterval);
  S47J::Set(A,TEXT("traceTicks"),J.TraceTicks);S47J::Set(A,TEXT("legacyTrace"),J.bLegacyTrace);
  if(J.RecordingSoundVersion==1)
  {
   auto Sound=S47J::New();S47J::Set(Sound,TEXT("v"),1);S47J::Set(Sound,TEXT("gain"),J.RecordingGain);
   S47J::Set(Sound,TEXT("bandwidth"),J.RecordingBandwidth);S47J::Set(Sound,TEXT("quality"),J.RecordingQuality);S47J::Set(A,TEXT("recordingSound"),Sound);
  }
  S47J::Set(A,TEXT("beamPoint"),J.BeamPoint);S47J::Set(A,TEXT("beamSettle"),J.BeamSettle);S47J::Set(A,TEXT("beamSample"),J.BeamSample);S47J::Set(A,TEXT("beamAz"),J.BeamAz);S47J::Set(A,TEXT("beamEl"),J.BeamEl);S47J::Set(A,TEXT("beamAmplitude"),J.BeamAmplitude);S47J::Set(A,TEXT("beamVerified"),J.bBeamVerified);S47J::Set(A,TEXT("clockMeasured"),J.bClockMeasured);S47J::Set(A,TEXT("duplicateCollected"),J.bDuplicateCollected);S47J::Set(A,TEXT("duplicateRejected"),J.bDuplicateRejected);S47J::Set(A,TEXT("originalPage"),J.OriginalPage);S47J::Set(A,TEXT("originalStamp"),J.OriginalStamp);S47J::Set(A,TEXT("duplicatePage"),J.DuplicatePage);S47J::Set(A,TEXT("duplicateStamp"),J.DuplicateStamp);S47J::Set(A,TEXT("caseLine"),J.CaseLine);S47J::Set(A,TEXT("caseTest"),J.CaseTest);S47J::Set(A,TEXT("caseValid"),J.bCaseValid);S47J::Set(A,TEXT("caseRecorded"),J.bCaseRecorded);if(J.bGenerated)
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
 if(CaseField)S47J::Set(O,TEXT("fieldTests"),CaseField->Save());
 if(Field)S47J::Set(O,TEXT("field"),Field->Save());if(Director)S47J::Set(O,TEXT("director"),Director->Save());return O;
}
bool FS47FreeformSession::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;auto Check=[&](bool B,const TCHAR* Name){OK&=B;Lines.Add(FString(B?TEXT("PASS "):TEXT("FAIL "))+Name);};
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
 Case.Stop();
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
