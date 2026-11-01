#include "S47Freeform.h"
#include "S47FreeformAudioDesk.h"
#include "S47FreeformTerminal.h"
#include "S47ReceiverReadout.h"
#include "SS47ScreenView.h"
#include "CoreGlobals.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47Main.h"
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
 DayNumber=1;WorldClock=8*3600;Cash=120;Loan=0;Selected=AudioSourceIndex=SelectedDish=0;
 Settle=CrossSample=AudioCursor=AutosaveAt=IntegrationPulse=0;CrossPoint=0;bCrossRunning=false;
 JobSeed=M&&M->Flow&&!M->Flow->bDevRun?int32(FDateTime::UtcNow().GetTicks()^FPlatformTime::Cycles64()):470047;
 Work.Reset();Findings.Reset();FindingDays.Reset();MysterySteps={0,0,0};Upgrades={false,false,false};SavedAngles.Reset();bRestoreAngles=false;Feedback.Empty();PhenomenonId.Empty();
 if(Saved)
 {
  if(!JsonVersion(Saved)) { bActive=false;return; }
  JobSeed=int32(FMath::Clamp(SafeNum(Saved,TEXT("jobSeed"),JobSeed),double(MIN_int32),double(MAX_int32)));
  WorldClock=FMath::Clamp(SafeNum(Saved,TEXT("clock"),8*3600),0.,3153600000.);
  if(!FMath::IsFinite(WorldClock)) {bActive=false;return;}
  DayNumber=FMath::Max(1,int32(FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay))+1);
  Cash=FMath::Clamp(SafeInt(Saved,TEXT("cash"),120),0,1000000);Loan=FMath::Clamp(SafeInt(Saved,TEXT("loan")),0,1000000);
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
   J.bAnomaly=S47J::Bool(O,TEXT("anomaly"));J.bControl=S47J::Bool(O,TEXT("control"));J.bAudioReviewed=S47J::Bool(O,TEXT("reviewed"));J.bPrinted=S47J::Bool(O,TEXT("printed"));J.bPaid=S47J::Bool(O,TEXT("paid"));
   J.ControlAt=FMath::Clamp(SafeNum(O,TEXT("controlAt")),0.,8.);J.OriginalCovered=FMath::Clamp(SafeNum(O,TEXT("originalCovered")),0.,18.);J.ControlCovered=FMath::Clamp(SafeNum(O,TEXT("controlCovered")),0.,8.);
   J.MarkStart=SafeNum(O,TEXT("markStart"),-1);J.MarkEnd=SafeNum(O,TEXT("markEnd"),-1);
   J.ReceiverClock=SafeNum(O,TEXT("receiverClock"));J.ReferenceClock=SafeNum(O,TEXT("referenceClock"));J.ControlReceiverClock=SafeNum(O,TEXT("controlReceiverClock"));J.ControlReferenceClock=SafeNum(O,TEXT("controlReferenceClock"));J.DuplicateClock=SafeNum(O,TEXT("duplicateClock"));J.DuplicateInterval=SafeNum(O,TEXT("duplicateInterval"));
   J.BeamPoint=FMath::Clamp(SafeInt(O,TEXT("beamPoint")),0,2);J.BeamSettle=FMath::Clamp(SafeNum(O,TEXT("beamSettle")),0.,1.);J.BeamSample=FMath::Clamp(SafeNum(O,TEXT("beamSample")),0.,8./3.);J.BeamAz=S47J::Nums(O,TEXT("beamAz"));J.BeamEl=S47J::Nums(O,TEXT("beamEl"));J.BeamAmplitude=S47J::Nums(O,TEXT("beamAmplitude"));
   J.bBeamVerified=S47J::Bool(O,TEXT("beamVerified"));J.bClockMeasured=S47J::Bool(O,TEXT("clockMeasured"));J.bDuplicateCollected=S47J::Bool(O,TEXT("duplicateCollected"));J.OriginalPage=S47J::Str(O,TEXT("originalPage"));J.OriginalStamp=S47J::Str(O,TEXT("originalStamp"));J.DuplicatePage=S47J::Str(O,TEXT("duplicatePage"));J.DuplicateStamp=S47J::Str(O,TEXT("duplicateStamp"));
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
  Selected=FMath::Clamp(SafeInt(Saved,TEXT("selected")),0,FMath::Max(0,Work.Num()-1));AudioSourceIndex=FMath::Clamp(SafeInt(Saved,TEXT("audioSource")),0,1);
  AudioCursor=FMath::Clamp(SafeNum(Saved,TEXT("audioCursor")),0.,AudioSourceIndex==0?18.:8.);
  const auto F=S47J::Obj(Saved,TEXT("findings"));if(F)for(const auto& P:F->Values){FString T;if(P.Value->TryGetString(T))Findings.Add(FString(*P.Key),T);}
  const auto D=S47J::Obj(Saved,TEXT("findingDays"));if(D)for(const auto& P:D->Values){double N;if(P.Value&&P.Value->TryGetNumber(N)&&FMath::IsFinite(N)&&N>=1&&N<=36500&&FMath::FloorToDouble(N)==N)FindingDays.Add(FString(*P.Key),int32(N));}
  const auto U=S47J::Nums(Saved,TEXT("upgrades"));for(int32 I=0;I<FMath::Min(3,U.Num());++I)Upgrades[I]=U[I]>0;
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
 AddDay(DayNumber);
 if(!Director)Director=MakeShared<FS47FreeformDirector>(M);
 if(!Field)Field=MakeShared<FS47FreeformWorld>(M);
 Field->Begin(S47J::Obj(Saved,TEXT("field")));
 if(!AudioDesk)AudioDesk=MakeShared<FS47FreeformAudioDesk>(M);AudioDesk->Begin();
 Director->Begin(S47J::Obj(Saved,TEXT("director")));
 if(M&&M->Story){M->Story->SetClock(FMath::Fmod(WorldClock,86400.));M->Story->Rx.bPowered=true;}
 UpdateMysteries();
}
void FS47FreeformSession::Stop()
{
 // UObject purge may destroy the scene before this plain C++ session is released.
 const bool bCanRestore=!IsEngineExitRequested()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&IsValid(M->Story)&&IsValid(M->Web);
 if(bCanRestore&&bPowerApplied&&!bLastPower)M->Story->ApplyFeederPower(false);
 bPowerApplied=false;
 if(Director)Director->Stop();if(AudioDesk)AudioDesk->Stop();if(Field)Field->Stop();
 TerminalView.Reset();ActiveTerminal=-1;
 if(bCanRestore)M->Web->SetWorkMotionPaused(false);
 bActive=bTelemetry=bAudioOpen=bAudioPlaying=bRecovering=false;++RecoveryEpoch;
}
bool FS47FreeformSession::Held() const
{
 return !bActive||bDead||bRecovering||!M||M->Mode!=US47Main::EMode::Play||M->bPausedByMenu||(M->ModalOpen()&&!TerminalOnGlass())||(M->World&&M->World->bBusy);
}
bool FS47FreeformSession::ArrayMotionHeld() const
{
 return !bActive||bDead||bRecovering||!M||M->bPausedByMenu||(M->ModalOpen()&&!bTelemetry&&!TerminalOnGlass())||!HasPower();
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
double FS47FreeformSession::TargetAz()const{if(const FJob* J=CurrentJob()){auto P=EffectiveProfile(*J);return(P.Az[0]+P.Az[1])/2;}return 0;}
double FS47FreeformSession::TargetEl()const{return CurrentJob()?(CurrentJob()->bGenerated?CurrentJob()->Card.El:FS47Observing::Runs()[CurrentJob()->Source].El):25;}
double FS47FreeformSession::SourceSeconds()const{return CurrentJob()?(CurrentJob()->bGenerated?CurrentJob()->Card.Seconds:FS47Observing::Runs()[CurrentJob()->Source].Seconds):18;}
FString FS47FreeformSession::Reference(const FJob& J)const{return TEXT("SARO / ")+J.Id;}
FString FS47FreeformSession::JobLine(const FJob& J)const
{
 return FString::Printf(TEXT("D%02d / %s / %s / %s%s"),J.Day,*J.Id,FS47Observing::Runs()[J.Source].Source,*StageText(J.Stage),J.bAnomaly?TEXT(" / INVESTIGATE"):TEXT(""));
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
   const bool Local=Field&&Field->ServiceStage()>=7&&Field->ServiceStage()<=8&&M->Web->DishId(I)==TEXT("S-03");
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
 if(Field)Field->Step(D);
 if(Director)Director->Step(D);
 if(M->UI&&M->Mode==US47Main::EMode::Play)M->UI->Objective(Objective());
}
void FS47FreeformSession::StepWork(double Dt)
{
 FJob* J=Job();if(!J)return;
 if(J->Stage==EJobStage::Pointed||J->Stage==EJobStage::Integrating)
 {
  Settle=PointReady()?FMath::Min(1.,Settle+Dt):0;
  if(J->Stage==EJobStage::Integrating&&Settle>=1)
  {
   J->Elapsed=FMath::Min(SourceSeconds(),J->Elapsed+Dt);
   if(bCrossRunning)
   {
    CrossSample+=Dt;if(CrossSample>=2){if(CrossPoint<4){++CrossPoint;PointCross();}else bCrossRunning=false;}
   }
   if(J->Source==1&&M->Audio)
   {
    IntegrationPulse+=Dt;while(IntegrationPulse>=PulsarPeriod){IntegrationPulse-=PulsarPeriod;M->Audio->Beep(820,.012f,.16f,0,TEXT("square"));}
   }
   if(J->Elapsed>=SourceSeconds()&&!bCrossRunning)
   {
    J->Stage=EJobStage::Chart;Changed(TEXT("Observation retained. Collect its original chart from the printer."));
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
    const double Sample=FMath::Min(Dt,8./3.-J->BeamSample);
    J->BeamSample+=Sample;J->ControlAt=FMath::Min(8.,J->ControlAt+Sample);
    if(J->BeamSample>=8./3.-1e-6)
    {
     double ActualAz=0,ActualEl=0;int32 Count=0;
     for(int32 I=0;I<27;++I)if(M->Web->DishRemoteAvailable(I)){ActualAz+=M->Web->DishAz(I);ActualEl+=M->Web->DishEl(I);++Count;}
     ActualAz/=FMath::Max(1,Count);ActualEl/=FMath::Max(1,Count);
     J->BeamAz.Add(ActualAz);J->BeamEl.Add(ActualEl);J->BeamAmplitude.Add(BeamLevel(ActualAz,TargetAz()+.5));
     if(J->BeamPoint<2){++J->BeamPoint;PointBeamControl();}
     else {J->bBeamVerified=true;J->ControlAt=8;Changed(TEXT("Three real beam samples retained; the array has returned to centre. Compare their signal levels at OPS."));if(Director)Director->Use(TEXT("ff:control"));}
    }
   }
  }
  else
  {
   J->ControlAt=FMath::Min(8.,J->ControlAt+Dt);
   if(J->ControlAt>=8)
   {
    J->ControlReferenceClock=WorldClock;J->ControlReceiverClock=WorldClock+(J->Phenomenon==TEXT("time_disagreement")?3.2:0);J->bClockMeasured=J->Phenomenon==TEXT("time_disagreement");
    if(J->bClockMeasured)Changed(TEXT("Both device counters read at the same internal-reference pulse. Compare original and control clock offsets at OPS."));
    else if(J->Phenomenon==TEXT("duplicate_print"))Changed(TEXT("Master acquisition register checked. Collect the separate duplicate at the printer, then compare its stamp with the retained original."));
    else Changed(TEXT("Internal control complete. Review original and control at the signal processor."));
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
 const FJob* Current=CurrentJob();if(Current&&(Current->Stage==EJobStage::Integrating||(Current->bControl&&Current->ControlAt<8)))return false;
 Selected=Index;Settle=0;bCrossRunning=false;bAudioPlaying=false;AudioCursor=0;Changed();return true;
}
bool FS47FreeformSession::ChooseThird(bool bVirgo)
{
 FJob* J=Job();if(!J||J->Slot!=2||J->Stage!=EJobStage::New)return false;J->Source=bVirgo?3:2;J->Card=FS47FreeformJobs::Make(JobSeed,J->Day,J->Slot,J->Source);J->bGenerated=true;Changed();return true;
}
bool FS47FreeformSession::PointSelected()
{
 FJob* J=Job();if(!J||J->Stage>EJobStage::Pointed||!M||!M->Web||!HasPower())return false;
 J->Stage=EJobStage::Pointed;Settle=0;bCrossRunning=false;
 M->Story->Rx.Azimuth=TargetAz(); // The physical pointing command is the receiver bearing too.
 M->Web->PointDishes(TargetAz(),TargetEl(),9);Changed(TEXT("Tracking command sent. Actual encoder agreement is required before acquisition."));return true;
}
bool FS47FreeformSession::ReceiverReady() const
{
 const FJob* J=CurrentJob();return J&&J->Stage==EJobStage::Pointed&&HasPower()&&PointReady()&&Settle>=1&&M&&M->Story&&M->Story->Rx.Pass(EffectiveProfile(*J));
}
FString FS47FreeformSession::ReceiverNextAction() const
{
 const FJob* J=CurrentJob();if(!J)return TEXT("Select an observation at the operations PC.");
 if(!HasPower())return TEXT("Restore the SARO feeder or start the backup generator.");
 if(J->Stage==EJobStage::New)return TEXT("Use the right-hand tracking PC to point the array at this source.");
 if(J->Stage==EJobStage::Integrating)return FString::Printf(TEXT("Recording %.0f%%. Keep the receiver tuned; the trace shows the live signal."),100*J->Elapsed/SourceSeconds());
 if(J->Stage!=EJobStage::Pointed)return TEXT("Recording retained. Collect the chart at the printer, then follow the work order.");
 if(!PointReady())return TEXT("Wait for the dishes to reach the commanded direction. Inspect isolated elements on the left status PC if movement stops.");
 if(Settle<1)return TEXT("The array is on source. Wait one second for it to settle.");
 const auto P=EffectiveProfile(*J);const auto& R=M->Story->Rx;
 if(R.Pass(P))return TEXT("READY. Select START ACQUISITION to record this source.");
 if(FMath::Abs(R.Frequency-P.F)>P.FTol)return FString::Printf(TEXT("Tune FREQUENCY to %.3f MHz (now %.3f). Watch the carrier peak and received percentage."),P.F,R.Frequency);
 if(R.Gain<P.Gain[0]||R.Gain>P.Gain[1])return FString::Printf(TEXT("Set GAIN between %.0f and %.0f (now %.0f)."),P.Gain[0],P.Gain[1],R.Gain);
 if(R.Bandwidth<P.Bw[0]||R.Bandwidth>P.Bw[1])return FString::Printf(TEXT("Set BANDWIDTH between %.0f and %.0f kHz (now %.0f)."),P.Bw[0],P.Bw[1],R.Bandwidth);
 return TEXT("Fine-tune the receiver until the received carrier reaches 100%.");
}
bool FS47FreeformSession::StartAcquisition()
{
 FJob* J=Job();if(!ReceiverReady()){Feedback=ReceiverNextAction();return false;}
 J->Stage=EJobStage::Integrating;J->Elapsed=0;J->bAnomaly=J->Slot==1&&bAnomaly;J->Phenomenon=J->bAnomaly?PhenomenonId:FString();
 J->ReferenceClock=WorldClock;J->ReceiverClock=WorldClock+(J->Phenomenon==TEXT("time_disagreement")?3.2:0);
 int32 Available=0;for(int32 I=0;I<27;++I)Available+=M->Web->DishRemoteAvailable(I)?1:0;J->Quality=double(Available)/27;
 double ActualAz=0;for(int32 I=0;I<27;++I)if(M->Web->DishRemoteAvailable(I))ActualAz+=M->Web->DishAz(I);
 ActualAz/=FMath::Max(1,Available);J->ReceivedLevel=BeamLevel(ActualAz,TargetAz()+(J->Phenomenon==TEXT("wrong_source")?.5:0));
 bCrossRunning=J->Source==0;CrossPoint=0;IntegrationPulse=0;if(bCrossRunning)PointCross();
 Changed(TEXT("Acquisition started. Only physically settled pointing is integrated."));return true;
}
void FS47FreeformSession::PrintJob(bool bPacket)
{
 FJob* J=Job();if(!J||!M||!M->Story)return;
 FS47Doc D;D.Id=J->Id+(bPacket?TEXT("-packet"):TEXT("-original"));D.Title=(bPacket?TEXT("Registered packet: "):TEXT("Original observation: "))+J->Id;D.Kind=TEXT("printout");D.Stamp=Reference(*J)+FString::Printf(TEXT(" / ORIGINAL COUNTER %.3f"),J->ReferenceClock);
 D.Page=Reference(*J)+TEXT("\n")+FS47Observing::Runs()[J->Source].Source+FString::Printf(TEXT("\nACTUAL ARRAY / QUALITY %.0f %%\nF %.3f MHz / AZ %.1f / EL %.1f\nINTEGRATION %.1f SECONDS\n"),J->Quality*100,EffectiveProfile(*J).F,TargetAz(),TargetEl(),J->Elapsed);
 if(J->Phenomenon==TEXT("time_disagreement"))D.Page+=FString::Printf(TEXT("ORIGINAL SIMULTANEOUS CLOCK READBACK\nRX COUNTER %.3f s / INDEPENDENT COUNTER %.3f s / DELTA %+.3f s\nRun the internal reference before interpreting this mismatch.\n"),J->ReceiverClock,J->ReferenceClock,J->ReceiverClock-J->ReferenceClock);
 else if(J->Phenomenon==TEXT("wrong_source"))D.Page+=FString::Printf(TEXT("CENTRE-BEAM RECEIVED LEVEL %.3f / NORMAL REFERENCE 1.000\nEncoder pointing matches command. An adjacent-beam control is still required; no off-source origin has been established.\n"),J->ReceivedLevel);
 else if(J->Phenomenon==TEXT("duplicate_print"))D.Page+=TEXT("PROVENANCE: ORIGINAL RX RECORD. A printer duplicate has been queued separately. Preserve this original before collecting or comparing the later copy.\n");
 else if(J->bAnomaly)D.Page+=TEXT("Extra ticks outside the reference period. Original retained.\n");else D.Page+=TEXT("Result consistent with the current reference; no anomalous origin established.\n");
 D.Page+=FString::Printf(TEXT("CAPTURE CLOCK %.3f SECONDS / OBSERVED INTERVAL %.3f SECONDS\n"),J->ReferenceClock,J->Elapsed);
 if(!bPacket)
 {
  if(J->OriginalPage.IsEmpty()){J->OriginalPage=D.Page;J->OriginalStamp=D.Stamp;}
  else {D.Page=J->OriginalPage;D.Stamp=J->OriginalStamp;}
 }
 if(bPacket)
 {
  if(Specialized(J->Phenomenon))D.Page+=ControlSummary(*J)+TEXT("\nREGISTERED / PHYSICAL FILING PENDING.");
  else D.Page+=FString::Printf(TEXT("REGISTERED / INTERNAL CONTROL %.1f SECONDS\nMARK %.1f TO %.1f SECONDS / CONCLUSION: SOURCE UNRESOLVED\nPHYSICAL FILING PENDING."),J->ControlAt,J->MarkStart,J->MarkEnd);
 }
 D.Transcript=D.Page;M->Story->AddDoc(D);if(M->UI)M->UI->Document(D);
}
bool FS47FreeformSession::CollectChart()
{
 FJob* J=Job();if(!J)return false;
 if(J->Stage!=EJobStage::Chart&&!RequirePower())return false;
 if(J->Stage==EJobStage::Chart){J->Stage=EJobStage::Recorded;J->bPrinted=true;PrintJob(false);Changed();return true;}
 if(J->Stage==EJobStage::Recorded&&J->Phenomenon==TEXT("duplicate_print")&&!J->bDuplicateCollected){CollectDuplicate();return J->bDuplicateCollected;}
 if(J->Stage==EJobStage::Registered&&J->bAnomaly){J->Stage=EJobStage::Packet;PrintJob(true);Changed();return true;}
 return false;
}
void FS47FreeformSession::CollectDuplicate()
{
 if(!RequirePower())return;
 FJob* J=Job();if(!J||!J->bPrinted||J->bDuplicateCollected||!M||!M->Story)return;
 if(J->OriginalPage.IsEmpty())if(const FS47Doc* D=M->Story->Docs().FindByPredicate([&](const FS47Doc& X){return X.Id==J->Id+TEXT("-original");})){J->OriginalPage=D->Page;J->OriginalStamp=D->Stamp;}
 if(J->OriginalPage.IsEmpty())return;
 J->DuplicateClock=J->ReferenceClock+2.4;J->DuplicateInterval=FMath::Max(0.,J->Elapsed-1.);
 const FString OriginalLine=FString::Printf(TEXT("CAPTURE CLOCK %.3f SECONDS / OBSERVED INTERVAL %.3f SECONDS"),J->ReferenceClock,J->Elapsed);
 const FString DuplicateLine=FString::Printf(TEXT("CAPTURE CLOCK %.3f SECONDS / OBSERVED INTERVAL %.3f SECONDS"),J->DuplicateClock,J->DuplicateInterval);
 if(!J->OriginalPage.Contains(OriginalLine,ESearchCase::CaseSensitive)){Feedback=TEXT("This retained legacy original has no independent capture-counter record. Keep it intact and make a new incident observation.");return;}
 J->DuplicatePage=TEXT("SEPARATE PRINTER COPY / ORIGIN UNVERIFIED\n\n")+J->OriginalPage.Replace(*OriginalLine,*DuplicateLine,ESearchCase::CaseSensitive);
 J->DuplicateStamp=Reference(*J)+FString::Printf(TEXT(" / COPY COUNTER %.3f"),J->DuplicateClock);
 FS47Doc D;D.Id=J->Id+TEXT("-duplicate");D.Title=TEXT("Separate printer duplicate: ")+J->Id;D.Kind=TEXT("printout");D.Stamp=J->DuplicateStamp;D.Page=J->DuplicatePage;D.Transcript=D.Page;
 M->Story->AddDoc(D);J->bDuplicateCollected=true;if(M->UI)M->UI->Document(D);Changed();
}
FString FS47FreeformSession::ControlSummary(const FJob& J)const
{
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
 FJob* J=Job();if(!J||J->Stage!=EJobStage::Recorded||!J->bControl||J->ControlAt<8||!Specialized(J->Phenomenon))return false;
 bool Verified=false;
 if(J->Phenomenon==TEXT("wrong_source"))Verified=J->bBeamVerified&&J->BeamAz.Num()==3&&J->BeamEl.Num()==3&&J->BeamAmplitude.Num()==3&&J->BeamAmplitude[1]>J->BeamAmplitude[0]+.25&&J->BeamAmplitude[1]>J->BeamAmplitude[2]+.25;
 else if(J->Phenomenon==TEXT("time_disagreement"))Verified=J->bClockMeasured&&FMath::IsNearlyEqual(J->ReceiverClock-J->ReferenceClock,3.2,.001)&&FMath::IsNearlyEqual(J->ControlReceiverClock-J->ControlReferenceClock,3.2,.001);
 else if(J->Phenomenon==TEXT("duplicate_print")&&J->bDuplicateCollected&&M&&M->Story)
 {
  const FS47Doc* Original=M->Story->Docs().FindByPredicate([&](const FS47Doc& D){return D.Id==J->Id+TEXT("-original");});
  const FS47Doc* Copy=M->Story->Docs().FindByPredicate([&](const FS47Doc& D){return D.Id==J->Id+TEXT("-duplicate");});
  Verified=Original&&Copy&&Original->Page==J->OriginalPage&&Original->Stamp==J->OriginalStamp&&Copy->Page==J->DuplicatePage&&Copy->Stamp==J->DuplicateStamp&&J->DuplicatePage!=J->OriginalPage&&FMath::IsNearlyEqual(J->DuplicateClock-J->ReferenceClock,2.4,.001)&&FMath::IsNearlyEqual(J->Elapsed-J->DuplicateInterval,1.,.001);
 }
 if(!Verified){Feedback=TEXT("The required independent measurements or retained originals are incomplete. Do the physical control before comparing.");return false;}
 J->bAudioReviewed=true;RecordEvidence(J->Phenomenon,ControlSummary(*J));if(Director)Director->Use(TEXT("ff:compare"));Changed(TEXT("Measured comparison retained. Register the supported instrumental conclusion at OPS."));return true;
}
bool FS47FreeformSession::StartControl()
{
 FJob* J=Job();if(!J||J->Stage!=EJobStage::Recorded||!J->bAnomaly||J->bControl||!HasPower())return false;
 J->bControl=true;J->ControlAt=0;
 if(J->Phenomenon==TEXT("wrong_source")){J->BeamPoint=0;J->BeamAz.Reset();J->BeamEl.Reset();J->BeamAmplitude.Reset();J->bBeamVerified=false;PointBeamControl();Changed(TEXT("Adjacent-beam control commanded: west, east, then centre. Each sample waits for real encoder settling."));}
 else Changed(J->Phenomenon==TEXT("duplicate_print")?TEXT("Checking the independent master acquisition register; original paper retained."):TEXT("External input terminated. Eight-second internal reference running; original retained."));return true;
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
  if(AudioSourceIndex!=0){Feedback=TEXT("Mark the original recording, not the quiet internal control.");return false;}
  if(Action==TEXT("mark_start"))J->MarkStart=AudioCursor;else J->MarkEnd=AudioCursor;J->bAudioReviewed=false;
 }
 else if(Action==TEXT("confirm"))
 {
  if(Specialized(J->Phenomenon))return CompareInstrument();
  if(J->OriginalCovered<18||J->ControlCovered<8){Feedback=TEXT("Play both recordings through. Seeking does not play skipped audio.");return false;}
  if(J->MarkStart<0||J->MarkStart>9.30||J->MarkEnd<12.60||J->MarkEnd-J->MarkStart>6){Feedback=TEXT("Mark both short groups of extra ticks, between about 9 and 13 seconds.");return false;}
  J->bAudioReviewed=true;RecordEvidence(TEXT("pulse-review"),TEXT("Original and internal reference reviewed. Extra ticks are absent from the short control; source remains unresolved."));
  if(Director)Director->Use(TEXT("ff:compare"));Changed(TEXT("Marked comparison retained. Register a supported conclusion at OPS."));
 }
 else return false;
 Changed();return true;
}
void FS47FreeformSession::StepAudio(double Dt)
{
 if(!HasPower()){bAudioPlaying=false;return;}
 FJob* J=Job();if(!J||!bAudioPlaying||!bAudioOpen||!M||!AudioDesk)return;
 const double Was=AudioCursor,Duration=AudioSourceIndex==0?18.:8.;AudioCursor=FMath::Min(Duration,Was+Dt);
 const bool Listening=AudioDesk->ListeningPosition();
 double& Covered=AudioSourceIndex==0?J->OriginalCovered:J->ControlCovered;if(Listening&&Was<=Covered+1e-7)Covered=FMath::Max(Covered,AudioCursor);
 if(M->Audio&&Listening)
 {
  for(double At=PulsarPeriod;At<=Duration;At+=PulsarPeriod)if(Was<At&&AudioCursor>=At)M->Audio->Beep(820,.012f,.16f,0,TEXT("square"));
  if(AudioSourceIndex==0)for(double At:Ticks)if(Was<At&&AudioCursor>=At)M->Audio->Beep(1240,.008f,.1f,0,TEXT("square"));
 }
 if(AudioCursor>=Duration){bAudioPlaying=false;Changed();}
}
double FS47FreeformSession::AudioLevel()const
{
 if(!bAudioPlaying)return .05;double L=.08;
 for(double At=PulsarPeriod;At<=18;At+=PulsarPeriod)L=FMath::Max(L,FMath::Max(0.,1-FMath::Abs(AudioCursor-At)/.06));
 if(AudioSourceIndex==0)for(double At:Ticks)L=FMath::Max(L,FMath::Max(0.,.8-FMath::Abs(AudioCursor-At)/.05));return L;
}
FString FS47FreeformSession::AudioDisplay()const{return FString::Printf(TEXT("%s %04.1f/%02d %s"),AudioSourceIndex==0?TEXT("ORG"):TEXT("REF"),AudioCursor,AudioSourceIndex==0?18:8,bAudioPlaying?TEXT("PLAY"):TEXT("PAUSE"));}
bool FS47FreeformSession::RegisterComparison(const FString& Conclusion)
{
 if(!RequirePower())return false;
 FJob* J=Job();if(!J||J->Stage!=EJobStage::Recorded)return false;
 if(J->bAnomaly&&(!J->bAudioReviewed||Conclusion!=(Specialized(J->Phenomenon)?InstrumentConclusion(J->Phenomenon):FString(TEXT("unresolved"))))){Feedback=Specialized(J->Phenomenon)?TEXT("Complete the measured comparison and register its instrumental finding. The controls do not prove an alien origin."):TEXT("A quiet short control does not establish a sky source. Review both recordings and retain the unresolved conclusion.");return false;}
 if(J->bAnomaly&&Specialized(J->Phenomenon)&&!CompareInstrument())return false;
 J->Stage=EJobStage::Registered;
 if(!J->bAnomaly){if(!J->bPaid){J->bPaid=true;Earn(20);}J->Stage=EJobStage::Filed;}
 Changed(J->bAnomaly?TEXT("Registered. Collect the labelled packet at the printer, then file its originals in the binder."):TEXT("Routine result registered. $20 credited; the original remains in the notebook."));return true;
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
 RecordEvidence(TEXT("pulse-archive"),TEXT("Original, internal control and registered conclusion physically filed under one reference."));
 if(Director)Director->Use(TEXT("ff:archive"));Changed(TEXT("Originals filed. Work credited; follow-up remains in the case archive."));return true;
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
void FS47FreeformSession::Earn(int32 Amount)
{
 if(Amount<=0||Amount>10000)return;
 const int32 Paid=FMath::Min(Amount,Loan);Loan-=Paid;Cash=FMath::Min(1000000,Cash+Amount-Paid);Changed();
}
int32 FS47FreeformSession::InventoryCount(const FString& Id)const{return Field?Field->InventoryCount(Id):0;}
void FS47FreeformSession::AddItem(const FString& Id,int32 Count){if(Field)Field->AddItem(Id,Count);}
bool FS47FreeformSession::Consume(const FString& Id){return Field&&Field->Consume(Id);}
void FS47FreeformSession::SetPowerFault(bool bFault){if(bPowerFault==bFault)return;bPowerFault=bFault;Changed(bFault?TEXT("Feeder voltage is unstable. Measure the circuit or transfer to the generator."):TEXT("Feeder supply restored and verified."));}
void FS47FreeformSession::SetAnomaly(bool bValue)
{
 if(bAnomaly==bValue)return;bAnomaly=bValue;
 if(bValue)
 {
  const bool Available=Work.ContainsByPredicate([this](const FJob& J){return J.Day==DayNumber&&J.Slot==1&&J.Stage<=EJobStage::Pointed;});
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
 static const int32 Prices[]={60,80,50};if(Index<0||Index>=3||Upgrades[Index]||!Spend(Prices[Index]))return false;
 Upgrades[Index]=true;
 if(Index==0)AddItem(TEXT("relay"),1);
 if(Index==2){AddItem(TEXT("fuse"),2);AddItem(TEXT("relay"),1);}
 Changed(TEXT("Upgrade installed. Its function is listed on the operations work order."));return true;
}
bool FS47FreeformSession::Rest()
{
 const FJob* J=CurrentJob();if(Held()&&(!M||!M->ModalOpen()))return false;
 if((Director&&Director->Busy())||(J&&(J->Stage==EJobStage::Integrating||(J->bControl&&J->ControlAt<8)))){Feedback=TEXT("Finish or safely interrupt the active work before resting.");return false;}
 WorldClock=(FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay)+1)*SecondsPerDay+8*3600;
 DayNumber=FMath::Max(1,int32(FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay))+1);AddDay(DayNumber);Settle=0;
 if(M&&M->Story)M->Story->SetClock(8*3600);
 if(M&&M->UI)M->UI->Close();Changed(TEXT("Morning. New work is available; retained records and unfinished jobs remain."));return true;
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
  bTelemetry=false;TerminalView.Reset();ActiveTerminal=-1;Changed();
 });bTelemetry=bLive;bAudioOpen=bAudio;
}
void FS47FreeformSession::OpenOperations()
{
 if(!RequirePower())return;
 if(!M||!M->UI)return;
 TArray<S47Kit::FW> Rows;
 Rows.Add(S47FreeformTerminal::Paper(FString::Printf(TEXT("SARO / FREEFORM / DAY %d\nOPERATING BUDGET $%d / REQUISITION BALANCE $%d\nObserve, verify, retain. An unresolved result is still useful work."),DayNumber,Cash,Loan)));
 TSharedRef<SScrollBox> List=SNew(SScrollBox);
 for(int32 I=0;I<Work.Num();++I)
 {
  const int32 Index=I;
  if(Work[I].Day<DayNumber-2&&Work[I].Stage==EJobStage::Filed)continue;
  List->AddSlot()[S47FreeformTerminal::Button(JobLine(Work[I]),[this,Index]{if(SelectJob(Index))OpenOperations();},Selected==I)];
 }
 Rows.Add(SNew(SBox).HeightOverride(180)[List]);
 if(const FJob* J=CurrentJob())
 {
  if(J->Slot==2&&J->Stage==EJobStage::New)Rows.Add(S47Kit::Row({S47FreeformTerminal::Button(TEXT("Third job: hydrogen drift"),[this]{ChooseThird(false);OpenOperations();}),S47FreeformTerminal::Button(TEXT("Third job: Virgo A"),[this]{ChooseThird(true);OpenOperations();})}));
  if(J->Stage==EJobStage::Recorded)
  {
   if(J->bAnomaly)
   {
    if(Specialized(J->Phenomenon))
    {
     Rows.Add(S47FreeformTerminal::Small(ControlSummary(*J)));
     Rows.Add(S47FreeformTerminal::Button(TEXT("Compare the retained instrument measurements"),[this]{CompareInstrument();OpenOperations();}));
     const FString Conclusion=InstrumentConclusion(J->Phenomenon);
     const FString Label=J->Phenomenon==TEXT("wrong_source")?TEXT("Register an off-centre maximum; origin unresolved"):J->Phenomenon==TEXT("time_disagreement")?TEXT("Register disagreement in the receiver timing path"):TEXT("Register mismatched duplicate; preserve original");
     Rows.Add(S47FreeformTerminal::Button(Label,[this,Conclusion]{RegisterComparison(Conclusion);OpenOperations();},true));
    }
    else
    {
     Rows.Add(S47FreeformTerminal::Small(TEXT("Compare the original with the internal reference at the reel deck beside the telephone before registering.")));
     Rows.Add(S47FreeformTerminal::Button(TEXT("The quiet control proves a sky source"),[this]{RegisterComparison(TEXT("sky"));OpenOperations();}));
     Rows.Add(S47FreeformTerminal::Button(TEXT("Source unresolved; retain both measurements"),[this]{RegisterComparison(TEXT("unresolved"));OpenOperations();},true));
    }
   }
   else Rows.Add(S47FreeformTerminal::Button(TEXT("Register routine result / $20"),[this]{RegisterComparison(TEXT("normal"));OpenOperations();},true));
  }
 }
 Rows.Add(S47FreeformTerminal::Small(Feedback));
 Rows.Add(S47FreeformTerminal::Small(TEXT("UPGRADES: relay reserve reduces recurrent failures; comparison locates extra ticks; field kit includes two fuses and a relay.")));
 Rows.Add(S47Kit::Row({S47FreeformTerminal::Button(TEXT("Case archive"),[this]{OpenArchive();}),S47FreeformTerminal::Button(TEXT("Service work order"),[this]{OpenServiceGuide();}),S47FreeformTerminal::Button(TEXT("Rest until morning"),[this]{if(!Rest())OpenOperations();})}));
 static const TCHAR* Labels[]={TEXT("Relay reserve / $60"),TEXT("Reference comparison / $80"),TEXT("Field service kit / $50")};
 TArray<S47Kit::FW> U;for(int32 I=0;I<3;++I){const int32 Index=I;U.Add(S47FreeformTerminal::Button(Upgrades[I]?FString(Labels[I])+TEXT(" / INSTALLED"):FString(Labels[I]),[this,Index]{Upgrade(Index);OpenOperations();},false,!Upgrades[I]));}Rows.Add(S47Kit::Row(U));
 OpenTerminal(3,TEXT("OPS / DAILY WORK"),S47Kit::Col(Rows));
}
void FS47FreeformSession::OpenTracking()
{
 if(!RequirePower())return;
 if(!M||!M->UI||!CurrentJob())return;
 auto P=EffectiveProfile(*CurrentJob());
 TArray<S47Kit::FW> Rows;
 Rows.Add(S47FreeformTerminal::Paper(JobLine(*CurrentJob())+FString::Printf(TEXT("\nCOMMAND AZ %.1f / EL %.1f\n"),TargetAz(),TargetEl())+P.Card));
 Rows.Add(S47FreeformTerminal::Text(TAttribute<FString>::CreateLambda([this]{return PointReady()?Settle>=1?TEXT("ACTUAL ENCODERS STEADY / READY"):TEXT("ON TARGET / SETTLING"):TEXT("ARRAY MOVING OR DEGRADED / WAIT FOR ENCODERS");}),TAttribute<int32>(0)));
 Rows.Add(S47FreeformTerminal::Button(TEXT("Point array to selected source"),[this]{if(PointSelected())M->UI->Close();},true,CurrentJob()->Stage<=EJobStage::Pointed));
 Rows.Add(S47FreeformTerminal::Small(TEXT("Watch the actual dishes through the window. Up to three isolated antennas permit degraded acquisition. Fewer than 24 require service.")));
 if(CurrentJob()->Phenomenon==TEXT("wrong_source")&&CurrentJob()->Stage==EJobStage::Recorded)
 {
  Rows.Add(S47FreeformTerminal::Text(TAttribute<FString>::CreateLambda([this]{const auto* J=CurrentJob();return J?FString::Printf(TEXT("ADJACENT CONTROL %d/3 / SETTLE %.1f s / SAMPLE %.1f s\n%s"),J->BeamPoint+1,J->BeamSettle,J->BeamSample,*ControlSummary(*J)):FString();}),TAttribute<int32>(0)));
  Rows.Add(S47FreeformTerminal::Button(TEXT("Measure west, east and centre beams"),[this]{if(StartControl())M->UI->Close();},true,!CurrentJob()->bControl));
 }
 OpenTerminal(2,TEXT("TRACKING / ARRAY CONTROL"),S47Kit::Col(Rows),true);
}
void FS47FreeformSession::OpenStatus()
{
 if(!RequirePower())return;
 if(!M||!M->UI||!M->Web)return;
 TArray<S47Kit::FW> Rows;
 Rows.Add(S47FreeformTerminal::Small(TEXT("Inspect actual and commanded direction. Select an element before local service.")));
 TSharedRef<SScrollBox> List=SNew(SScrollBox);
 for(int32 I=0;I<M->Web->DishCount();++I)
 {
  const int32 Index=I;
  List->AddSlot()[S47FreeformTerminal::ButtonLive(TAttribute<FString>::CreateLambda([this,Index]
  {return FString::Printf(TEXT("%s ACT %.1f/%.1f CMD %.1f/%.1f %s"),*M->Web->DishId(Index),M->Web->DishAz(Index),M->Web->DishEl(Index),M->Web->DishTargetAz(Index),M->Web->DishTargetEl(Index),M->Web->DishRemoteAvailable(Index)?TEXT("REMOTE"):TEXT("ISOLATED / SERVICE"));}),[this,Index]{SelectedDish=Index;if(M->Web->DishId(Index)==TEXT("S-03")){if(Field)Field->DiagnoseService();RecordEvidence(TEXT("s03-diagnosed"),TEXT("S-03 command/encoder status inspected at the status terminal."));}OpenStatus();},I==SelectedDish)];
 }
 Rows.Add(SNew(SBox).HeightOverride(220)[List]);
 if(Field)Rows.Add(S47FreeformTerminal::Text(TAttribute<FString>::CreateLambda([this]{return Field?Field->ServiceStatus():FString();}),TAttribute<int32>(0)));
 Rows.Add(S47FreeformTerminal::Small(bPowerFault?TEXT("FEEDER VOLTAGE UNSTABLE / USE FIELD METER OR BACKUP"):TEXT("FEEDER SUPPLY NORMAL")));
 OpenTerminal(0,TEXT("ARRAY STATUS / MAINTENANCE"),S47Kit::Col(Rows),true);
}
void FS47FreeformSession::OpenReceiver()
{
 if(!RequirePower())return;
 if(!M||!M->UI||!M->Story||!CurrentJob())return;
 auto& R=M->Story->Rx;const auto P=EffectiveProfile(*CurrentJob());TArray<S47Kit::FW> Rows;
 if(CurrentJob()->Stage!=EJobStage::New)R.Azimuth=TargetAz(); // No second, contradictory azimuth dial after physical tracking.
 S47ReceiverReadout::FData D;
 D.GetReceiver=[this]{return M->Story->Rx;};D.GetProfile=[this]{return EffectiveProfile(*CurrentJob());};
 D.GetQuality=[this]{return M->Story->Rx.Quality(EffectiveProfile(*CurrentJob()));};
 D.GetProgress=[this]{return CurrentJob()?CurrentJob()->Elapsed/FMath::Max(1.,SourceSeconds()):0.;};
 D.GetArrayQuality=[this]{int32 N=0;for(int32 I=0;I<M->Web->DishCount();++I)N+=M->Web->DishRemoteAvailable(I)?1:0;return double(N)/FMath::Max(1,M->Web->DishCount());};
 D.GetReady=[this]{return ReceiverReady();};D.GetTrackingReady=[this]{return PointReady()&&Settle>=1;};
 D.GetRecording=[this]{return CurrentJob()&&CurrentJob()->Stage==EJobStage::Integrating;};D.GetNextAction=[this]{return ReceiverNextAction();};
 Rows.Add(S47ReceiverReadout::Build(D));
 Rows.Add(S47FreeformTerminal::Small(JobLine(*CurrentJob())+FString::Printf(TEXT("\nF %.3f MHz / GAIN %.0f–%.0f / BW %.0f–%.0f kHz / %.0f seconds"),P.F,P.Gain[0],P.Gain[1],P.Bw[0],P.Bw[1],SourceSeconds())));
 Rows.Add(S47Kit::Knob(TEXT("FREQUENCY MHz"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.3f"),M->Story->Rx.Frequency);}),1419.5f,1420.7f,.001f,TAttribute<float>::CreateLambda([this]{return float(M->Story->Rx.Frequency);}),[this](float V){M->Story->Rx.Frequency=V;}));
 Rows.Add(S47FreeformTerminal::Pair(S47Kit::Knob(TEXT("GAIN"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.0f"),M->Story->Rx.Gain);}),0,100,1,TAttribute<float>::CreateLambda([this]{return float(M->Story->Rx.Gain);}),[this](float V){M->Story->Rx.Gain=V;}),S47Kit::Knob(TEXT("BANDWIDTH kHz"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.0f"),M->Story->Rx.Bandwidth);}),1,100,1,TAttribute<float>::CreateLambda([this]{return float(M->Story->Rx.Bandwidth);}),[this](float V){M->Story->Rx.Bandwidth=V;})));
 Rows.Add(S47FreeformTerminal::Button(TEXT("Start acquisition"),[this]{StartAcquisition();},true,CurrentJob()->Stage==EJobStage::Pointed));
 Rows.Add(S47FreeformTerminal::Small(TEXT("Pointing follows the right-hand tracking PC. Tune the carrier to 100%, then start. The recording advances here.")));OpenTerminal(1,TEXT("RECEIVER / ACQUISITION"),S47Kit::Col(Rows),true);
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
 for(int32 I=0;I<3;++I)Rows.Add(S47Kit::Small(FString::Printf(TEXT("%s / %d of 3 / %s"),Names[I],MysterySteps[I],MysterySteps[I]==3?TEXT("CROSS-DAY COMPARISON AVAILABLE"):TEXT("FOLLOW-UP OPEN"))));
 TSharedRef<SScrollBox> List=SNew(SScrollBox);
 TArray<FString> Keys;Findings.GetKeys(Keys);Keys.Sort();
 for(const FString& K:Keys)List->AddSlot()[Paper(FString::Printf(TEXT("D%d / %s\n%s"),FindingDays.FindRef(K),*K,*Findings.FindRef(K)))];
 Rows.Add(SNew(SBox).HeightOverride(260)[List]);
 if(CurrentJob()&&CurrentJob()->Stage==EJobStage::Packet)
 {
  if(NearBinder())Rows.Add(S47Kit::Button(TEXT("Place registered originals in this binder"),[this]{FilePacket();OpenArchive();},true));
  else Rows.Add(S47Kit::Small(TEXT("Registered packet awaiting physical filing. Take its originals to the records-room binder; this remote archive is read-only.")));
 }
 if(Computer)OpenTerminal(3,TEXT("OPS / CASE ARCHIVE"),S47Kit::Col(Rows));
 else SetPanel(S47Kit::Shell(M->UI,TEXT("SARO / RECORDS"),S47Kit::Col(Rows),1000));
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
  if(Id==TEXT("telex")&&Director&&Director->Use(TEXT("ff:telex")))return true;OpenOperations();return true;
 }
 if(Id==TEXT("decoder")||Id==TEXT("signalProcessor")||Id==TEXT("decoderDesk")){OpenAudio();return true;}
 if(Id==TEXT("rack"))
 {
  if(!StartControl())Changed(TEXT("Receiver rack normal. Secure an anomalous original before recording the internal control."));return true;
 }
 if(Id==TEXT("printer")){if(!CollectChart())Changed(TEXT("No uncollected chart. Register retained routine results at OPS; anomalous packets need comparison first."));return true;}
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
 if(Id==TEXT("printer"))return TEXT("Collect original chart or registered packet");
 if(Id==TEXT("binder")||Id==TEXT("excBinder")||Id==TEXT("serviceRecord"))return TEXT("File originals and inspect the continuing case archive");
 return Director?Director->Label(Id):FString();
}
FString FS47FreeformSession::Objective()const
{
 if(bDead)return TEXT("CONTACT // LOAD A SAVED CASE TO CONTINUE");
 if(bPowerFault&&!HasPower())return TEXT("POWER // MEASURE THE FEEDER OR TRANSFER TO THE FUELLED GENERATOR");
 if(ServiceGuide==1&&bGuideFocus){const FString Guide=ServiceObjective();if(!Guide.IsEmpty())return Guide;}
 const FJob* J=CurrentJob();if(!J)return TEXT("OPERATIONS // SELECT DAILY OBSERVING WORK");
 switch(J->Stage)
 {
  case EJobStage::New:return TEXT("TRACKING // POINT THE ARRAY TO THE SELECTED SOURCE");
  case EJobStage::Pointed:return PointReady()&&Settle>=1?TEXT("RECEIVER // TUNE THE CARD AND START ACQUISITION"):TEXT("ARRAY MOVING // WAIT FOR ACTUAL ENCODER AGREEMENT");
  case EJobStage::Integrating:return TEXT("OBSERVATION RUNNING // ONLY SETTLED POINTING IS RECORDED");
  case EJobStage::Chart:return TEXT("PRINTER // RETAIN THE ORIGINAL OBSERVATION CHART");
  case EJobStage::Recorded:
   if(!J->bAnomaly)return TEXT("OPERATIONS // REGISTER THE ROUTINE RESULT");
   if(!J->bControl)return J->Phenomenon==TEXT("wrong_source")?TEXT("TRACKING // MEASURE WEST, EAST AND CENTRE BEAMS"):TEXT("RECEIVER RACK // RECORD AN INDEPENDENT INTERNAL CONTROL");
   if(J->ControlAt<8)return J->Phenomenon==TEXT("wrong_source")?TEXT("ADJACENT-BEAM CONTROL // REAL ENCODERS SETTLE BEFORE EACH SAMPLE"):TEXT("INTERNAL CONTROL RUNNING // ORIGINAL RETAINED");
   if(J->Phenomenon==TEXT("duplicate_print")&&!J->bDuplicateCollected)return TEXT("PRINTER // COLLECT THE SEPARATE DUPLICATE WITHOUT ALTERING THE ORIGINAL");
   if(Specialized(J->Phenomenon))return J->bAudioReviewed?TEXT("OPERATIONS // REGISTER THE SUPPORTED INSTRUMENTAL FINDING"):TEXT("OPERATIONS // COMPARE THE RETAINED INSTRUMENT MEASUREMENTS");
   return J->bAudioReviewed?TEXT("OPERATIONS // REGISTER THE SUPPORTED CONCLUSION"):TEXT("REEL DECK BY TELEPHONE // REVIEW BOTH RECORDINGS AND MARK THE EXTRA TICKS");
  case EJobStage::Registered:return TEXT("PRINTER // COLLECT THE LABELLED REGISTERED PACKET");
  case EJobStage::Packet:return TEXT("RECORDS // PLACE THE ORIGINAL PACKET IN THE BINDER");
  default:return TEXT("OPERATIONS // SELECT MORE WORK OR FOLLOW UP THE CASE ARCHIVE");
 }
}
TArray<FS47Task> FS47FreeformSession::Tasks()const
{
 TArray<FS47Task> Result;
 if(bPowerFault)Result.Add({TEXT("FEEDER / Measure, isolate and replace the control fuse; restore SARO power."),false});
 if(Field&&Field->HasArrayFault()&&ServiceGuide!=1)Result.Add({TEXT("S-03 / Diagnose at the status PC, replace the relay and run the local out-and-back test."),false});
 if(ServiceGuide>0)Result.Add({ServiceGuide==2?FString(TEXT("S-03 service work order / verified and archived")):ServiceObjective(),ServiceGuide==2});
 for(const FJob& J:Work)if(J.Day==DayNumber||J.Stage!=EJobStage::Filed)Result.Add({JobLine(J),J.Stage==EJobStage::Filed});
 return Result;
}
S47J::FObj FS47FreeformSession::Save()const
{
 auto O=S47J::New();S47J::Set(O,TEXT("jobSeed"),JobSeed);S47J::Set(O,TEXT("serviceGuide"),ServiceGuide);S47J::Set(O,TEXT("guideServiceBaseline"),GuideServiceBaseline);S47J::Set(O,TEXT("guideSupplyBaseline"),GuideSupplyBaseline);S47J::Set(O,TEXT("guideSupply"),bGuideSupply);S47J::Set(O,TEXT("guideFocus"),bGuideFocus);S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("clock"),WorldClock);S47J::Set(O,TEXT("cash"),Cash);S47J::Set(O,TEXT("loan"),Loan);S47J::Set(O,TEXT("lethal"),bDeadly);S47J::Set(O,TEXT("dead"),bDead);S47J::Set(O,TEXT("powerFault"),bPowerFault);S47J::Set(O,TEXT("anomaly"),bAnomaly);S47J::Set(O,TEXT("phenomenon"),PhenomenonId);
 S47J::Set(O,TEXT("selected"),Selected);S47J::Set(O,TEXT("audioSource"),AudioSourceIndex);S47J::Set(O,TEXT("audioCursor"),AudioCursor);S47J::Set(O,TEXT("settle"),Settle);S47J::Set(O,TEXT("cross"),CrossPoint);S47J::Set(O,TEXT("crossSample"),CrossSample);S47J::Set(O,TEXT("crossRunning"),bCrossRunning);
 TArray<TSharedPtr<FJsonValue>> Jobs;
 for(const FJob& J:Work)
 {
  auto A=S47J::New();S47J::Set(A,TEXT("id"),J.Id);S47J::Set(A,TEXT("day"),J.Day);S47J::Set(A,TEXT("slot"),J.Slot);S47J::Set(A,TEXT("source"),J.Source);S47J::Set(A,TEXT("phenomenon"),J.Phenomenon);S47J::Set(A,TEXT("stage"),int32(J.Stage));S47J::Set(A,TEXT("elapsed"),J.Elapsed);S47J::Set(A,TEXT("quality"),J.Quality);S47J::Set(A,TEXT("receivedLevel"),J.ReceivedLevel);S47J::Set(A,TEXT("anomaly"),J.bAnomaly);S47J::Set(A,TEXT("control"),J.bControl);S47J::Set(A,TEXT("reviewed"),J.bAudioReviewed);S47J::Set(A,TEXT("printed"),J.bPrinted);S47J::Set(A,TEXT("paid"),J.bPaid);S47J::Set(A,TEXT("controlAt"),J.ControlAt);S47J::Set(A,TEXT("originalCovered"),J.OriginalCovered);S47J::Set(A,TEXT("controlCovered"),J.ControlCovered);S47J::Set(A,TEXT("markStart"),J.MarkStart);S47J::Set(A,TEXT("markEnd"),J.MarkEnd);S47J::Set(A,TEXT("receiverClock"),J.ReceiverClock);S47J::Set(A,TEXT("referenceClock"),J.ReferenceClock);S47J::Set(A,TEXT("controlReceiverClock"),J.ControlReceiverClock);S47J::Set(A,TEXT("controlReferenceClock"),J.ControlReferenceClock);S47J::Set(A,TEXT("duplicateClock"),J.DuplicateClock);S47J::Set(A,TEXT("duplicateInterval"),J.DuplicateInterval);
  S47J::Set(A,TEXT("beamPoint"),J.BeamPoint);S47J::Set(A,TEXT("beamSettle"),J.BeamSettle);S47J::Set(A,TEXT("beamSample"),J.BeamSample);S47J::Set(A,TEXT("beamAz"),J.BeamAz);S47J::Set(A,TEXT("beamEl"),J.BeamEl);S47J::Set(A,TEXT("beamAmplitude"),J.BeamAmplitude);S47J::Set(A,TEXT("beamVerified"),J.bBeamVerified);S47J::Set(A,TEXT("clockMeasured"),J.bClockMeasured);S47J::Set(A,TEXT("duplicateCollected"),J.bDuplicateCollected);S47J::Set(A,TEXT("originalPage"),J.OriginalPage);S47J::Set(A,TEXT("originalStamp"),J.OriginalStamp);S47J::Set(A,TEXT("duplicatePage"),J.DuplicatePage);S47J::Set(A,TEXT("duplicateStamp"),J.DuplicateStamp);if(J.bGenerated)
  {
   auto C=S47J::New();const auto& P=J.Card.Receiver;S47J::Set(C,TEXT("v"),1);S47J::Set(C,TEXT("f"),P.F);S47J::Set(C,TEXT("ftol"),P.FTol);S47J::Set(C,TEXT("g0"),P.Gain[0]);S47J::Set(C,TEXT("g1"),P.Gain[1]);S47J::Set(C,TEXT("bw0"),P.Bw[0]);S47J::Set(C,TEXT("bw1"),P.Bw[1]);S47J::Set(C,TEXT("az0"),P.Az[0]);S47J::Set(C,TEXT("az1"),P.Az[1]);S47J::Set(C,TEXT("el"),J.Card.El);S47J::Set(C,TEXT("seconds"),J.Card.Seconds);S47J::Set(C,TEXT("suggested"),J.Card.SuggestedClock);S47J::Set(C,TEXT("text"),P.Card);S47J::Set(A,TEXT("card"),C);
  }
  Jobs.Add(MakeShared<FJsonValueObject>(A));
 }
 O->SetArrayField(TEXT("jobs"),Jobs);
 auto F=S47J::New(),D=S47J::New();for(const auto& P:Findings){S47J::Set(F,*P.Key,P.Value);S47J::Set(D,*P.Key,FindingDays.FindRef(P.Key));}S47J::Set(O,TEXT("findings"),F);S47J::Set(O,TEXT("findingDays"),D);
 TArray<double> U,Q;for(bool V:Upgrades)U.Add(V?1:0);for(int32 V:MysterySteps)Q.Add(V);S47J::Set(O,TEXT("upgrades"),U);S47J::Set(O,TEXT("mysteries"),Q);
 TArray<double> Angles=SavedAngles;if(!bRestoreAngles&&M&&M->Web&&M->Web->IsLoaded()){Angles.Reset();for(int32 I=0;I<M->Web->DishCount();++I)Angles.Append({M->Web->DishAz(I),M->Web->DishEl(I),M->Web->DishTargetAz(I),M->Web->DishTargetEl(I)});}S47J::Set(O,TEXT("angles"),Angles);
 if(Field)S47J::Set(O,TEXT("field"),Field->Save());if(Director)S47J::Set(O,TEXT("director"),Director->Save());return O;
}
bool FS47FreeformSession::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;auto Check=[&](bool B,const TCHAR* Name){OK&=B;Lines.Add(FString(B?TEXT("PASS "):TEXT("FAIL "))+Name);};
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
 FS47FreeformSession Unpowered(nullptr);Unpowered.Begin(nullptr);Unpowered.Selected=1;auto* UJ=Unpowered.Job();UJ->Stage=EJobStage::Recorded;UJ->bAnomaly=UJ->bControl=true;UJ->ControlAt=8;UJ->OriginalCovered=18;UJ->ControlCovered=8;UJ->MarkStart=9.1;UJ->MarkEnd=13;
 Unpowered.SetPowerFault(true);Check(!Unpowered.AudioAction(TEXT("confirm"))&&!UJ->bAudioReviewed&&!Unpowered.RegisterComparison(TEXT("unresolved")),TEXT("unpowered instruments cannot grant audio review or computer registration"));
 UJ->Stage=EJobStage::Registered;Check(!Unpowered.CollectChart()&&UJ->Stage==EJobStage::Registered,TEXT("a dead printer cannot create a new registered packet"));
 UJ->Stage=EJobStage::Packet;const int32 BeforeRemoteFiling=Unpowered.Budget();
 Check(!Unpowered.FilePacket()&&UJ->Stage==EJobStage::Packet&&Unpowered.Budget()==BeforeRemoteFiling,TEXT("remote archive access cannot file a packet or grant its payment without the actual binder and nearby player"));
 auto Corrupt=S.Save();TArray<double> InvalidAngles;InvalidAngles.Init(20,108);for(int32 A=1;A<108;A+=2)InvalidAngles[A]=30;InvalidAngles[5]=500;S47J::Set(Corrupt,TEXT("angles"),InvalidAngles);
 auto BadDays=S47J::Obj(Corrupt,TEXT("findingDays"));S47J::Set(BadDays,TEXT("extra_pulse"),1e100);FS47FreeformSession Guarded(nullptr);Guarded.Begin(Corrupt);
 Check(!Guarded.bRestoreAngles&&Guarded.SavedAngles.IsEmpty()&&!Guarded.FindingDays.Contains(TEXT("extra_pulse")),TEXT("out-of-range saved encoders and overflowing finding days are discarded before native transforms or integer casts"));
 Unpowered.Stop();Guarded.Stop();
 auto Future=S47J::New();S47J::Set(Future,TEXT("v"),2);R.Begin(Future);Check(!R.Active(),TEXT("Unknown Freeform save revisions do not restart a fresh session"));
 S.Stop();R.Stop();return OK;
}
