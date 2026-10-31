#include "S47Freeform.h"
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
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateColorBrush.h"

namespace
{
 constexpr double SecondsPerDay=86400., ClockRate=86400./4500.;
 constexpr double PulsarPeriod=1.1879;
 double SafeNum(const S47J::FObj& O,const TCHAR* K,double D=0) { const double V=S47J::Num(O,K,D);return FMath::IsFinite(V)?V:D; }
 int32 SafeInt(const S47J::FObj& O,const TCHAR* K,int32 D=0) { return int32(FMath::Clamp(SafeNum(O,K,D),-100000000.,100000000.)); }
 const double Ticks[]={9.30,9.48,9.66,9.84,11.40,11.60,11.80,12.00,12.20,12.40,12.60};
 const FVector2D Cross[]={{0,.5},{0,-.5},{.5,0},{-.5,0},{0,0}};
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
FS47FreeformSession::FS47FreeformSession(US47Main* Main):M(Main) {}
FS47FreeformSession::~FS47FreeformSession() { Stop(); }

void FS47FreeformSession::Begin(const S47J::FObj& Saved,bool bLethal)
{
 Stop();
 bActive=true;bDeadly=bLethal;bDead=bPowerFault=bAnomaly=bTelemetry=bAudioOpen=bAudioPlaying=bRecovering=false;
 DayNumber=1;WorldClock=8*3600;Cash=120;Loan=0;Selected=AudioSourceIndex=SelectedDish=0;
 Settle=CrossSample=AudioCursor=AutosaveAt=IntegrationPulse=0;CrossPoint=0;bCrossRunning=false;
 Work.Reset();Findings.Reset();FindingDays.Reset();MysterySteps={0,0,0};Upgrades={false,false,false};SavedAngles.Reset();bRestoreAngles=false;Feedback.Empty();PhenomenonId.Empty();
 if(Saved)
 {
  if(!JsonVersion(Saved)) { bActive=false;return; }
  WorldClock=FMath::Clamp(SafeNum(Saved,TEXT("clock"),8*3600),0.,3153600000.);
  if(!FMath::IsFinite(WorldClock)) {bActive=false;return;}
  DayNumber=FMath::Max(1,int32(FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay))+1);
  Cash=FMath::Clamp(SafeInt(Saved,TEXT("cash"),120),0,1000000);Loan=FMath::Clamp(SafeInt(Saved,TEXT("loan")),0,1000000);
  bDeadly=S47J::Bool(Saved,TEXT("lethal"),bLethal);bDead=S47J::Bool(Saved,TEXT("dead"));
  PhenomenonId=S47J::Str(Saved,TEXT("phenomenon"));bPowerFault=S47J::Bool(Saved,TEXT("powerFault"));bAnomaly=S47J::Bool(Saved,TEXT("anomaly"));
  Settle=FMath::Clamp(SafeNum(Saved,TEXT("settle")),0.,1.);CrossPoint=FMath::Clamp(SafeInt(Saved,TEXT("cross")),0,4);
  CrossSample=FMath::Clamp(SafeNum(Saved,TEXT("crossSample")),0.,2.);bCrossRunning=S47J::Bool(Saved,TEXT("crossRunning"));
  for(const auto& V:S47J::Arr(Saved,TEXT("jobs")))
  {
   if(!V||V->Type!=EJson::Object)continue;const auto O=V->AsObject();if(!O)continue;FJob J;
   J.Phenomenon=S47J::Str(O,TEXT("phenomenon"));J.Id=S47J::Str(O,TEXT("id"));J.Day=FMath::Max(1,SafeInt(O,TEXT("day"),1));J.Slot=FMath::Clamp(SafeInt(O,TEXT("slot")),0,2);J.Source=FMath::Clamp(SafeInt(O,TEXT("source")),0,3);
   J.Stage=EJobStage(FMath::Clamp(SafeInt(O,TEXT("stage")),0,7));J.Elapsed=FMath::Clamp(SafeNum(O,TEXT("elapsed")),0.,40.);J.Quality=FMath::Clamp(SafeNum(O,TEXT("quality"),1),0.,1.);
   J.bAnomaly=S47J::Bool(O,TEXT("anomaly"));J.bControl=S47J::Bool(O,TEXT("control"));J.bAudioReviewed=S47J::Bool(O,TEXT("reviewed"));J.bPrinted=S47J::Bool(O,TEXT("printed"));J.bPaid=S47J::Bool(O,TEXT("paid"));
   J.ControlAt=FMath::Clamp(SafeNum(O,TEXT("controlAt")),0.,8.);J.OriginalCovered=FMath::Clamp(SafeNum(O,TEXT("originalCovered")),0.,18.);J.ControlCovered=FMath::Clamp(SafeNum(O,TEXT("controlCovered")),0.,8.);
   J.MarkStart=SafeNum(O,TEXT("markStart"),-1);J.MarkEnd=SafeNum(O,TEXT("markEnd"),-1);
   if(!J.Id.IsEmpty()&&!Work.ContainsByPredicate([&](const FJob& X){return X.Id==J.Id;}))Work.Add(J);
  }
  Selected=FMath::Clamp(SafeInt(Saved,TEXT("selected")),0,FMath::Max(0,Work.Num()-1));AudioSourceIndex=FMath::Clamp(SafeInt(Saved,TEXT("audioSource")),0,1);
  AudioCursor=FMath::Clamp(SafeNum(Saved,TEXT("audioCursor")),0.,AudioSourceIndex==0?18.:8.);
  const auto F=S47J::Obj(Saved,TEXT("findings"));if(F)for(const auto& P:F->Values){FString T;if(P.Value->TryGetString(T))Findings.Add(P.Key,T);}
  const auto D=S47J::Obj(Saved,TEXT("findingDays"));if(D)for(const auto& P:D->Values){double N;if(P.Value->TryGetNumber(N))FindingDays.Add(P.Key,FMath::Max(1,int32(N)));}
  const auto U=S47J::Nums(Saved,TEXT("upgrades"));for(int32 I=0;I<FMath::Min(3,U.Num());++I)Upgrades[I]=U[I]>0;
  const auto Q=S47J::Nums(Saved,TEXT("mysteries"));for(int32 I=0;I<FMath::Min(3,Q.Num());++I)MysterySteps[I]=FMath::Clamp(int32(Q[I]),0,3);
  SavedAngles=S47J::Nums(Saved,TEXT("angles"));bRestoreAngles=SavedAngles.Num()==108;
 }
 AddDay(DayNumber);
 if(!Director)Director=MakeShared<FS47FreeformDirector>(M);
 if(!Field)Field=MakeShared<FS47FreeformWorld>(M);
 Field->Begin(S47J::Obj(Saved,TEXT("field")));Director->Begin(S47J::Obj(Saved,TEXT("director")));
 if(M&&M->Story){M->Story->SetClock(FMath::Fmod(WorldClock,86400.));M->Story->Rx.bPowered=true;}
 UpdateMysteries();
}
void FS47FreeformSession::Stop()
{
 if(Director)Director->Stop();if(Field)Field->Stop();
 if(M&&M->Web)M->Web->SetWorkMotionPaused(false);
 bActive=bTelemetry=bAudioOpen=bAudioPlaying=bRecovering=false;++RecoveryEpoch;
}
bool FS47FreeformSession::Held() const
{
 return !bActive||bDead||bRecovering||!M||M->Mode!=US47Main::EMode::Play||M->bPausedByMenu||M->ModalOpen()||(M->World&&M->World->bBusy);
}
bool FS47FreeformSession::ArrayMotionHeld() const
{
 return !bActive||bDead||bRecovering||!M||M->bPausedByMenu||(M->ModalOpen()&&!bTelemetry)||!HasPower();
}
bool FS47FreeformSession::HasPower() const {return !bPowerFault||(Field&&Field->HasBackupPower());}
void FS47FreeformSession::AddDay(int32 Number)
{
 if(Work.ContainsByPredicate([&](const FJob& J){return J.Day==Number;}))return;
 for(int32 I=0;I<3;++I){FJob J;J.Id=FString::Printf(TEXT("FF-D%04d-J%d"),Number,I+1);J.Day=Number;J.Slot=I;J.Source=I;Work.Add(J);}
}
FS47FreeformSession::FJob* FS47FreeformSession::Job(){return Work.IsValidIndex(Selected)?&Work[Selected]:nullptr;}
const FS47FreeformSession::FJob* FS47FreeformSession::CurrentJob()const{return Work.IsValidIndex(Selected)?&Work[Selected]:nullptr;}
FS47Profile FS47FreeformSession::Profile(int32 Source){return FS47Observing::Runs()[FMath::Clamp(Source,0,3)].P;}
const FS47Profile* FS47FreeformSession::ConsoleProfile()const
{
 if(!Active()||!CurrentJob())return nullptr;
 static FS47Profile P;P=Profile(CurrentJob()->Source);return &P;
}
double FS47FreeformSession::TargetAz()const{if(const FJob* J=CurrentJob()){auto P=Profile(J->Source);return(P.Az[0]+P.Az[1])/2;}return 0;}
double FS47FreeformSession::TargetEl()const{return CurrentJob()?FS47Observing::Runs()[CurrentJob()->Source].El:25;}
double FS47FreeformSession::SourceSeconds()const{return CurrentJob()?FS47Observing::Runs()[CurrentJob()->Source].Seconds:18;}
FString FS47FreeformSession::Reference(const FJob& J)const{return TEXT("SARO / ")+J.Id;}
FString FS47FreeformSession::JobLine(const FJob& J)const
{
 return FString::Printf(TEXT("D%02d / %s / %s / %s%s"),J.Day,*J.Id,FS47Observing::Runs()[J.Source].Source,*StageText(J.Stage),J.bAnomaly?TEXT(" / INVESTIGATE"):TEXT(""));
}
bool FS47FreeformSession::PointReady()const
{
 if(!M||!M->Web||M->Web->DishCount()!=27||!CurrentJob()||!HasPower())return false;
 const FVector2D Off=bCrossRunning?Cross[CrossPoint]:FVector2D::ZeroVector;int32 Count=0;
 for(int32 I=0;I<27;++I)if(M->Web->DishRemoteAvailable(I))
 {
  ++Count;if(FMath::Abs(FMath::FindDeltaAngleDegrees(M->Web->DishAz(I),TargetAz()+Off.X))>.1||FMath::Abs(M->Web->DishEl(I)-TargetEl()-Off.Y)>.1)return false;
 }
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
 M->Web->SetWorkMotionPaused(ArrayMotionHeld());
 if(!Held())
 {
  WorldClock+=D*ClockRate;int32 NowDay=FMath::Max(1,int32(FMath::FloorToDouble((WorldClock-8*3600)/SecondsPerDay))+1);
  if(NowDay!=DayNumber){DayNumber=NowDay;AddDay(DayNumber);Changed(TEXT("New daily observing jobs are on the operations terminal. Unfinished work remains available."));}
  M->Story->SetClock(FMath::Fmod(WorldClock,86400.));
  AutosaveAt+=D;if(AutosaveAt>=30){AutosaveAt=0;Changed();}
 }
 if(!ArrayMotionHeld())StepWork(D);
 if(bAudioOpen&&!M->bPausedByMenu&&!bDead)StepAudio(D);
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
  J->ControlAt=FMath::Min(8.,J->ControlAt+Dt);
  if(J->ControlAt>=8){Changed(TEXT("Internal control complete. Review original and control at the signal processor."));if(Director)Director->Use(TEXT("ff:control"));}
 }
}
void FS47FreeformSession::PointCross()
{
 Settle=CrossSample=0;if(M&&M->Web)M->Web->PointDishes(TargetAz()+Cross[CrossPoint].X,TargetEl()+Cross[CrossPoint].Y,2.2);
}
bool FS47FreeformSession::SelectJob(int32 Index)
{
 if(!Work.IsValidIndex(Index))return false;
 const FJob* Current=CurrentJob();if(Current&&(Current->Stage==EJobStage::Integrating||(Current->bControl&&Current->ControlAt<8)))return false;
 Selected=Index;Settle=0;bCrossRunning=false;bAudioPlaying=false;AudioCursor=0;Changed();return true;
}
bool FS47FreeformSession::ChooseThird(bool bVirgo)
{
 FJob* J=Job();if(!J||J->Slot!=2||J->Stage!=EJobStage::New)return false;J->Source=bVirgo?3:2;Changed();return true;
}
bool FS47FreeformSession::PointSelected()
{
 FJob* J=Job();if(!J||J->Stage>EJobStage::Pointed||!M||!M->Web||!HasPower())return false;
 J->Stage=EJobStage::Pointed;Settle=0;bCrossRunning=false;
 M->Web->PointDishes(TargetAz(),TargetEl(),9);Changed(TEXT("Tracking command sent. Actual encoder agreement is required before acquisition."));return true;
}
bool FS47FreeformSession::StartAcquisition()
{
 FJob* J=Job();if(!J||J->Stage!=EJobStage::Pointed||!PointReady()||Settle<1||!M||!M->Story)return false;
 if(!M->Story->Rx.Pass(Profile(J->Source))){Feedback=TEXT("Receiver off source. Check frequency, gain, bandwidth and azimuth against the card.");return false;}
 J->Stage=EJobStage::Integrating;J->Elapsed=0;J->bAnomaly=J->Slot==1&&bAnomaly;J->Phenomenon=J->bAnomaly?PhenomenonId:FString();
 int32 Available=0;for(int32 I=0;I<27;++I)Available+=M->Web->DishRemoteAvailable(I)?1:0;J->Quality=double(Available)/27;
 bCrossRunning=J->Source==0;CrossPoint=0;IntegrationPulse=0;if(bCrossRunning)PointCross();
 Changed(TEXT("Acquisition started. Only physically settled pointing is integrated."));return true;
}
void FS47FreeformSession::PrintJob(bool bPacket)
{
 FJob* J=Job();if(!J||!M||!M->Story)return;
 FS47Doc D;D.Id=J->Id+(bPacket?TEXT("-packet"):TEXT("-original"));D.Title=(bPacket?TEXT("Registered packet: "):TEXT("Original observation: "))+J->Id;D.Kind=TEXT("printout");D.Stamp=Reference(*J);
 D.Page=Reference(*J)+TEXT("\n")+FS47Observing::Runs()[J->Source].Source+FString::Printf(TEXT("\nACTUAL ARRAY / QUALITY %.0f %%\nF %.3f MHz / AZ %.1f / EL %.1f\nINTEGRATION %.1f SECONDS\n"),J->Quality*100,Profile(J->Source).F,TargetAz(),TargetEl(),J->Elapsed);
 if(J->Phenomenon==TEXT("time_disagreement"))D.Page+=TEXT("RX CLOCK vs independent reference: +3.2 s offset at the same pulse. Check the internal reference before interpreting the discrepancy.\n");
 else if(J->Phenomenon==TEXT("wrong_source"))D.Page+=TEXT("Signal persists in the adjacent beam. Encoder pointing matches command; signal origin does not yet agree with the scheduled source.\n");
 else if(J->Phenomenon==TEXT("duplicate_print"))D.Page+=TEXT("PROVENANCE: ORIGINAL RX RECORD. A separate duplicate was received with an inconsistent timestamp; this original is retained unchanged.\n");
 else if(J->bAnomaly)D.Page+=TEXT("Extra ticks outside the reference period. Original retained.\n");else D.Page+=TEXT("Result consistent with the current reference; no anomalous origin established.\n");
 if(bPacket)D.Page+=FString::Printf(TEXT("REGISTERED / INTERNAL CONTROL %.1f SECONDS\nMARK %.1f TO %.1f SECONDS / CONCLUSION: SOURCE UNRESOLVED\nPHYSICAL FILING PENDING."),J->ControlAt,J->MarkStart,J->MarkEnd);
 D.Transcript=D.Page;M->Story->AddDoc(D);if(M->UI)M->UI->Document(D);
}
bool FS47FreeformSession::CollectChart()
{
 FJob* J=Job();if(!J)return false;
 if(J->Stage==EJobStage::Chart){J->Stage=EJobStage::Recorded;J->bPrinted=true;PrintJob(false);Changed();return true;}
 if(J->Stage==EJobStage::Registered&&J->bAnomaly){J->Stage=EJobStage::Packet;PrintJob(true);Changed();return true;}
 return false;
}
bool FS47FreeformSession::StartControl()
{
 FJob* J=Job();if(!J||J->Stage!=EJobStage::Recorded||!J->bAnomaly||J->bControl||!HasPower())return false;
 J->bControl=true;J->ControlAt=0;Changed(TEXT("External input terminated. Eight-second internal reference running; original retained."));return true;
}
void FS47FreeformSession::SeekAudio(double At){if(!FMath::IsFinite(At))return;AudioCursor=FMath::Clamp(At,0.,AudioSourceIndex==0?18.:8.);bAudioPlaying=false;Changed();}
bool FS47FreeformSession::AudioAction(const FString& Action)
{
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
 FJob* J=Job();if(!J||!bAudioPlaying||!bAudioOpen||!M||!M->UI||!M->UI->Modal())return;
 const double Was=AudioCursor,Duration=AudioSourceIndex==0?18.:8.;AudioCursor=FMath::Min(Duration,Was+Dt);
 double& Covered=AudioSourceIndex==0?J->OriginalCovered:J->ControlCovered;if(Was<=Covered+1e-7)Covered=FMath::Max(Covered,AudioCursor);
 if(M->Audio)
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
 FJob* J=Job();if(!J||J->Stage!=EJobStage::Recorded)return false;
 if(J->bAnomaly&&(!J->bAudioReviewed||Conclusion!=TEXT("unresolved"))){Feedback=TEXT("A quiet short control does not establish a sky source. Review both recordings and retain the unresolved conclusion.");return false;}
 J->Stage=EJobStage::Registered;
 if(!J->bAnomaly){if(!J->bPaid){J->bPaid=true;Earn(20);}J->Stage=EJobStage::Filed;}
 Changed(J->bAnomaly?TEXT("Registered. Collect the labelled packet at the printer, then file its originals in the binder."):TEXT("Routine result registered. $20 credited; the original remains in the notebook."));return true;
}
bool FS47FreeformSession::FilePacket()
{
 FJob* J=Job();if(!J||J->Stage!=EJobStage::Packet)return false;
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
  int32 Count=0;int32 First=MAX_int32,Last=0;
  for(int32 I=0;I<3;++I)if(Findings.Contains(Families[Line][I])){++Count;const int32 D=FindingDays.FindRef(Families[Line][I]);First=FMath::Min(First,D);Last=FMath::Max(Last,D);}
  // A same-day series remains an initial finding. Independent follow-up must survive a new day.
  const int32 New=Count>=3&&Last>First?3:Count>=2&&Last>First?2:Count>0?1:0;
  const int32 Before=MysterySteps.IsValidIndex(Line)?MysterySteps[Line]:0;
  if(MysterySteps.IsValidIndex(Line))MysterySteps[Line]=FMath::Max(Before,New);
  if(New==3&&Before<3&&M&&M->Story)
  {
   FS47Doc D;D.Id=FString::Printf(TEXT("freeform-mystery-%d"),Line);D.Title=Names[Line];D.Kind=TEXT("comparison");D.Stamp=TEXT("CROSS-DAY COMPARISON / RETAINED SOURCES");
   D.Page=FString(Names[Line])+TEXT("\n\nThree independently retained records now span more than one day.\n");
   for(int32 I=0;I<3;++I)D.Page+=FString(Families[Line][I])+TEXT(": ")+Findings.FindRef(Families[Line][I])+TEXT("\n\n");
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
 if(Index==2){AddItem(TEXT("fuse"),2);AddItem(TEXT("battery"),1);}
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
 M->UI->Open(Widget,[this]{bTelemetry=bAudioOpen=bAudioPlaying=false;Changed();});bTelemetry=bLive;bAudioOpen=bAudio;
}
void FS47FreeformSession::OpenOperations()
{
 if(!M||!M->UI)return;
 TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Paper(FString::Printf(TEXT("SARO / FREEFORM / DAY %d\nOPERATING BUDGET $%d / REQUISITION BALANCE $%d\nObserve, verify, retain. An unresolved result is still useful work."),DayNumber,Cash,Loan)));
 TSharedRef<SScrollBox> List=SNew(SScrollBox);
 for(int32 I=0;I<Work.Num();++I)
 {
  const int32 Index=I;
  if(Work[I].Day<DayNumber-2&&Work[I].Stage==EJobStage::Filed)continue;
  List->AddSlot()[S47Kit::Button(JobLine(Work[I]),[this,Index]{if(SelectJob(Index))OpenOperations();},Selected==I)];
 }
 Rows.Add(SNew(SBox).HeightOverride(180)[List]);
 if(const FJob* J=CurrentJob())
 {
  if(J->Slot==2&&J->Stage==EJobStage::New)Rows.Add(S47Kit::Row({S47Kit::Button(TEXT("Third job: hydrogen drift"),[this]{ChooseThird(false);OpenOperations();}),S47Kit::Button(TEXT("Third job: Virgo A"),[this]{ChooseThird(true);OpenOperations();})}));
  if(J->Stage==EJobStage::Recorded)
  {
   if(J->bAnomaly)
   {
    Rows.Add(S47Kit::Small(TEXT("Compare the original with the internal reference at the signal processor before registering.")));
    Rows.Add(S47Kit::Button(TEXT("The quiet control proves a sky source"),[this]{RegisterComparison(TEXT("sky"));OpenOperations();}));
    Rows.Add(S47Kit::Button(TEXT("Source unresolved; retain both measurements"),[this]{RegisterComparison(TEXT("unresolved"));OpenOperations();},true));
   }
   else Rows.Add(S47Kit::Button(TEXT("Register routine result / $20"),[this]{RegisterComparison(TEXT("normal"));OpenOperations();},true));
  }
 }
 Rows.Add(S47Kit::Small(Feedback));
 Rows.Add(S47Kit::Row({S47Kit::Button(TEXT("Case archive"),[this]{OpenArchive();}),S47Kit::Button(TEXT("Rest until morning"),[this]{Rest();})}));
 static const TCHAR* Labels[]={TEXT("Relay reserve / $60"),TEXT("Reference comparison / $80"),TEXT("Field service kit / $50")};
 TArray<S47Kit::FW> U;for(int32 I=0;I<3;++I){const int32 Index=I;U.Add(S47Kit::Button(Upgrades[I]?FString(Labels[I])+TEXT(" / INSTALLED"):FString(Labels[I]),[this,Index]{Upgrade(Index);OpenOperations();},false,!Upgrades[I]));}Rows.Add(S47Kit::Row(U));
 SetPanel(S47Kit::Shell(M->UI,TEXT("SARO / OPERATIONS AND DAILY WORK"),S47Kit::Col(Rows),1000));
}
void FS47FreeformSession::OpenTracking()
{
 if(!M||!M->UI||!CurrentJob())return;
 auto P=Profile(CurrentJob()->Source);
 TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Paper(JobLine(*CurrentJob())+FString::Printf(TEXT("\nCOMMAND AZ %.1f / EL %.1f\n"),TargetAz(),TargetEl())+P.Card));
 Rows.Add(S47Kit::Text(TAttribute<FString>::CreateLambda([this]{return PointReady()?Settle>=1?TEXT("ACTUAL ENCODERS STEADY / READY"):TEXT("ON TARGET / SETTLING"):TEXT("ARRAY MOVING OR DEGRADED / WAIT FOR ENCODERS");}),TAttribute<int32>(0)));
 Rows.Add(S47Kit::Button(TEXT("Point array to selected source"),[this]{if(PointSelected())M->UI->Close();},true,CurrentJob()->Stage<=EJobStage::Pointed));
 Rows.Add(S47Kit::Small(TEXT("Watch the actual dishes through the window. Up to three isolated antennas permit degraded acquisition. Fewer than 24 require service.")));
 SetPanel(S47Kit::Shell(M->UI,TEXT("SARO / TRACKING"),S47Kit::Col(Rows),900),true);
}
void FS47FreeformSession::OpenStatus()
{
 if(!M||!M->UI||!M->Web)return;
 TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Small(TEXT("Inspect actual and commanded direction. Select an element before local service.")));
 TSharedRef<SScrollBox> List=SNew(SScrollBox);
 for(int32 I=0;I<M->Web->DishCount();++I)
 {
  const int32 Index=I;
  List->AddSlot()[S47Kit::ButtonLive(TAttribute<FString>::CreateLambda([this,Index]
  {return FString::Printf(TEXT("%s ACT %.1f/%.1f CMD %.1f/%.1f %s"),*M->Web->DishId(Index),M->Web->DishAz(Index),M->Web->DishEl(Index),M->Web->DishTargetAz(Index),M->Web->DishTargetEl(Index),M->Web->DishRemoteAvailable(Index)?TEXT("REMOTE"):TEXT("ISOLATED / SERVICE"));}),[this,Index]{SelectedDish=Index;if(M->Web->DishId(Index)==TEXT("S-03")){if(Field)Field->DiagnoseService();RecordEvidence(TEXT("s03-diagnosed"),TEXT("S-03 command/encoder status inspected at the status terminal."));}OpenStatus();},I==SelectedDish)];
 }
 Rows.Add(SNew(SBox).HeightOverride(280)[List]);
 Rows.Add(S47Kit::Small(bPowerFault?TEXT("FEEDER VOLTAGE UNSTABLE / USE FIELD METER OR BACKUP"):TEXT("FEEDER SUPPLY NORMAL")));
 SetPanel(S47Kit::Shell(M->UI,TEXT("SARO / ARRAY STATUS"),S47Kit::Col(Rows),980),true);
}
void FS47FreeformSession::OpenReceiver()
{
 if(!M||!M->UI||!M->Story||!CurrentJob())return;
 auto& R=M->Story->Rx;const auto P=Profile(CurrentJob()->Source);TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Paper(JobLine(*CurrentJob())+TEXT("\n")+P.Card));
 Rows.Add(S47Kit::Knob(TEXT("FREQUENCY MHz"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.3f"),M->Story->Rx.Frequency);}),1419.5f,1420.7f,.001f,TAttribute<float>::CreateLambda([this]{return float(M->Story->Rx.Frequency);}),[this](float V){M->Story->Rx.Frequency=V;}));
 Rows.Add(S47Kit::Row({S47Kit::Knob(TEXT("GAIN"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.0f"),M->Story->Rx.Gain);}),0,100,1,TAttribute<float>::CreateLambda([this]{return float(M->Story->Rx.Gain);}),[this](float V){M->Story->Rx.Gain=V;}),S47Kit::Knob(TEXT("BANDWIDTH kHz"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.0f"),M->Story->Rx.Bandwidth);}),1,100,1,TAttribute<float>::CreateLambda([this]{return float(M->Story->Rx.Bandwidth);}),[this](float V){M->Story->Rx.Bandwidth=V;})}));
 Rows.Add(S47Kit::Knob(TEXT("AZIMUTH"),TAttribute<FString>::CreateLambda([this]{return FString::Printf(TEXT("%.1f"),M->Story->Rx.Azimuth);}),0,359,.1f,TAttribute<float>::CreateLambda([this]{return float(M->Story->Rx.Azimuth);}),[this](float V){M->Story->Rx.Azimuth=V;}));
 Rows.Add(S47Kit::Text(TAttribute<FString>::CreateLambda([this]{const FJob* J=CurrentJob();return J?FString::Printf(TEXT("%s / %.1f s / ACTUAL ARRAY QUALITY %.0f %%"),*StageText(J->Stage),J->Elapsed,J->Quality*100):FString();}),TAttribute<int32>(0)));
 Rows.Add(S47Kit::Button(TEXT("Start acquisition"),[this]{if(StartAcquisition())M->UI->Close();else{Feedback=TEXT("Check receiver settings and tracking readiness. The array must physically settle before acquisition.");OpenReceiver();}},true,CurrentJob()->Stage==EJobStage::Pointed));
 Rows.Add(S47Kit::Small(Feedback));SetPanel(S47Kit::Shell(M->UI,TEXT("SARO / RECEIVER"),S47Kit::Col(Rows),980),true);
}
namespace
{
 class SS47FreeformTrace:public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SS47FreeformTrace){} SLATE_ARGUMENT(FS47FreeformSession*,Session) SLATE_END_ARGS()
  void Construct(const FArguments& A){Session=A._Session;}
  virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(740,110);}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool)const override
  {
   if(!Session)return Layer;
   const FVector2D Size=G.GetLocalSize();const float W=Size.X,H=Size.Y;
   const double Duration=Session->AudioSource()==0?18.:8.;
   auto Line=[&](float X,float Y,float X1,float Y1,FLinearColor C,float Width=1.f){TArray<FVector2D> P={FVector2D(X,Y),FVector2D(X1,Y1)};FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,Width);};
   for(int32 I=0;I<=int32(Duration);++I)Line(I/Duration*W,5,I/Duration*W,H-10,FLinearColor(.12,.2,.13,.7));
   for(double At=PulsarPeriod;At<=Duration;At+=PulsarPeriod)Line(At/Duration*W,H*.65,At/Duration*W,H*.2,FLinearColor(.5,1,.65,1),2);
   if(Session->AudioSource()==0)for(double At:Ticks)Line(At/Duration*W,H*.65,At/Duration*W,H*.43,FLinearColor(1,.65,.18,1),2);
   if(const auto* J=Session->CurrentJob())if(Session->AudioSource()==0&&J->MarkStart>=0&&J->MarkEnd>=J->MarkStart)
   {
    Line(J->MarkStart/Duration*W,1,J->MarkStart/Duration*W,H,FLinearColor(1,.7,.2,.8),2);Line(J->MarkEnd/Duration*W,1,J->MarkEnd/Duration*W,H,FLinearColor(1,.7,.2,.8),2);
   }
   Line(Session->AudioPosition()/Duration*W,0,Session->AudioPosition()/Duration*W,H,FLinearColor::White,2);return Layer+1;
  }
 private:FS47FreeformSession* Session=nullptr;
 };
}
void FS47FreeformSession::OpenAudio()
{
 if(!M||!M->UI||!CurrentJob())return;
 const FJob* J=CurrentJob();TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Paper(Reference(*J)+TEXT("\nORIGINAL / 18 s   INTERNAL CONTROL / 8 s")));
 if(!J->bAnomaly||J->ControlAt<8)
 {
  Rows.Add(S47Kit::Small(TEXT("Retain an anomalous original at the printer, then run the internal reference at the receiver rack. A normal job can be registered directly at OPS.")));
 }
 else
 {
  Rows.Add(S47Kit::Text(TAttribute<FString>::CreateLambda([this]{return AudioDisplay();}),TAttribute<int32>(0)));
  if(HasUpgrade(1))Rows.Add(S47Kit::Small(TEXT("REFERENCE ALIGNMENT: pulsar intervals match. Extra original events occupy 9.3 to 12.6 s; internal reference has none. This comparison locates a discrepancy, not its origin.")));
  Rows.Add(SNew(SS47FreeformTrace).Session(this));
  Rows.Add(S47Kit::Row({S47Kit::Button(TEXT("Original / 18 s"),[this]{AudioAction(TEXT("original"));OpenAudio();}),S47Kit::Button(TEXT("Control / 8 s"),[this]{AudioAction(TEXT("control"));OpenAudio();})}));
  Rows.Add(S47Kit::Row({S47Kit::Button(TEXT("Play / pause"),[this]{AudioAction(bAudioPlaying?TEXT("pause"):TEXT("play"));},true),S47Kit::Button(TEXT("Rewind"),[this]{SeekAudio(0);})}));
  Rows.Add(S47Kit::Slider(TEXT("Position"),0,AudioSourceIndex==0?18:8,.1,TAttribute<float>::CreateLambda([this]{return float(AudioCursor);}),[this](float V){SeekAudio(V);}));
  Rows.Add(S47Kit::Row({S47Kit::Button(TEXT("Mark start"),[this]{AudioAction(TEXT("mark_start"));}),S47Kit::Button(TEXT("Mark end"),[this]{AudioAction(TEXT("mark_end"));})}));
  Rows.Add(S47Kit::Text(TAttribute<FString>::CreateLambda([this]{const auto* C=CurrentJob();return C?FString::Printf(TEXT("Original heard %.1f/18 / control heard %.1f/8 / mark %.1f to %.1f s\n%s"),C->OriginalCovered,C->ControlCovered,C->MarkStart,C->MarkEnd,*Feedback):FString();}),TAttribute<int32>(0)));
  Rows.Add(S47Kit::Button(TEXT("Retain marked comparison"),[this]{if(AudioAction(TEXT("confirm")))M->UI->Close();},true));
 }
 SetPanel(S47Kit::Shell(M->UI,TEXT("SARO / SIGNAL PROCESSOR"),S47Kit::Col(Rows),960),false,true);
}
void FS47FreeformSession::OpenArchive()
{
 if(!M||!M->UI)return;TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Paper(TEXT("SARO / CONTINUING CASE ARCHIVE\nObservations, independent controls and hypotheses are kept separately.")));
 static const TCHAR* Names[]={TEXT("The repeated interval"),TEXT("Visitors around the base"),TEXT("Disagreeing records")};
 for(int32 I=0;I<3;++I)Rows.Add(S47Kit::Small(FString::Printf(TEXT("%s / %d of 3 / %s"),Names[I],MysterySteps[I],MysterySteps[I]==3?TEXT("CROSS-DAY COMPARISON AVAILABLE"):TEXT("FOLLOW-UP OPEN"))));
 TSharedRef<SScrollBox> List=SNew(SScrollBox);
 TArray<FString> Keys;Findings.GetKeys(Keys);Keys.Sort();
 for(const FString& K:Keys)List->AddSlot()[S47Kit::Paper(FString::Printf(TEXT("D%d / %s\n%s"),FindingDays.FindRef(K),*K,*Findings.FindRef(K)))];
 Rows.Add(SNew(SBox).HeightOverride(260)[List]);
 if(CurrentJob()&&CurrentJob()->Stage==EJobStage::Packet)Rows.Add(S47Kit::Button(TEXT("Place registered originals in this binder"),[this]{FilePacket();OpenArchive();},true));
 SetPanel(S47Kit::Shell(M->UI,TEXT("SARO / RECORDS"),S47Kit::Col(Rows),1000));
}
bool FS47FreeformSession::Use(const FString& Id)
{
 if(!bActive)return false;
 if(Field&&Field->Use(Id))return true;
 if(Id==TEXT("crtCenter")){OpenReceiver();return true;}
 if(Id==TEXT("crtLeft")){OpenStatus();return true;}
 if(Id==TEXT("crtRight")){OpenTracking();return true;}
 if(Id==TEXT("opsTerminal")||Id==TEXT("telex")||Id==TEXT("logbook")||Id==TEXT("workOrder")||Id==TEXT("anomaly:shiftlog"))
 {
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
 if(Field){const FString L=Field->Label(Id);if(!L.IsEmpty())return L;}
 if(Id==TEXT("crtCenter"))return TEXT("Use the receiver / selected daily observation");
 if(Id==TEXT("crtLeft"))return TEXT("Inspect array status and individual drives");
 if(Id==TEXT("crtRight"))return TEXT("Command tracking for the selected source");
 if(Id==TEXT("opsTerminal"))return TEXT("Daily work, registration and operating budget");
 if(Id==TEXT("telex")||Id==TEXT("logbook")||Id==TEXT("workOrder")||Id==TEXT("anomaly:shiftlog"))return TEXT("Read the daily work order and incoming messages");
 if(Id==TEXT("phone")||Id==TEXT("recPhone"))return TEXT("Check the operations line");
 if(Id==TEXT("decoder")||Id==TEXT("signalProcessor")||Id==TEXT("decoderDesk"))return TEXT("Compare original and internal-reference recordings");
 if(Id==TEXT("rack"))return TEXT("Record an independent receiver control");
 if(Id==TEXT("printer"))return TEXT("Collect original chart or registered packet");
 if(Id==TEXT("binder")||Id==TEXT("excBinder")||Id==TEXT("serviceRecord"))return TEXT("File originals and inspect the continuing case archive");
 return Director?Director->Label(Id):FString();
}
FString FS47FreeformSession::Objective()const
{
 if(bDead)return TEXT("CONTACT // LOAD A SAVED CASE TO CONTINUE");
 if(bPowerFault&&!HasPower())return TEXT("POWER // MEASURE THE FEEDER OR TRANSFER TO THE FUELLED GENERATOR");
 const FJob* J=CurrentJob();if(!J)return TEXT("OPERATIONS // SELECT DAILY OBSERVING WORK");
 switch(J->Stage)
 {
  case EJobStage::New:return TEXT("TRACKING // POINT THE ARRAY TO THE SELECTED SOURCE");
  case EJobStage::Pointed:return PointReady()&&Settle>=1?TEXT("RECEIVER // TUNE THE CARD AND START ACQUISITION"):TEXT("ARRAY MOVING // WAIT FOR ACTUAL ENCODER AGREEMENT");
  case EJobStage::Integrating:return TEXT("OBSERVATION RUNNING // ONLY SETTLED POINTING IS RECORDED");
  case EJobStage::Chart:return TEXT("PRINTER // RETAIN THE ORIGINAL OBSERVATION CHART");
  case EJobStage::Recorded:
   if(!J->bAnomaly)return TEXT("OPERATIONS // REGISTER THE ROUTINE RESULT");
   if(!J->bControl)return TEXT("RECEIVER RACK // RECORD AN INDEPENDENT INTERNAL CONTROL");
   if(J->ControlAt<8)return TEXT("INTERNAL CONTROL RUNNING // ORIGINAL RETAINED");
   return J->bAudioReviewed?TEXT("OPERATIONS // REGISTER THE SUPPORTED CONCLUSION"):TEXT("SIGNAL PROCESSOR // REVIEW BOTH RECORDINGS AND MARK THE EXTRA TICKS");
  case EJobStage::Registered:return TEXT("PRINTER // COLLECT THE LABELLED REGISTERED PACKET");
  case EJobStage::Packet:return TEXT("RECORDS // PLACE THE ORIGINAL PACKET IN THE BINDER");
  default:return TEXT("OPERATIONS // SELECT MORE WORK OR FOLLOW UP THE CASE ARCHIVE");
 }
}
TArray<FS47Task> FS47FreeformSession::Tasks()const
{
 TArray<FS47Task> Result;
 for(const FJob& J:Work)if(J.Day==DayNumber||J.Stage!=EJobStage::Filed)Result.Add({JobLine(J),J.Stage==EJobStage::Filed});
 return Result;
}
S47J::FObj FS47FreeformSession::Save()const
{
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("clock"),WorldClock);S47J::Set(O,TEXT("cash"),Cash);S47J::Set(O,TEXT("loan"),Loan);S47J::Set(O,TEXT("lethal"),bDeadly);S47J::Set(O,TEXT("dead"),bDead);S47J::Set(O,TEXT("powerFault"),bPowerFault);S47J::Set(O,TEXT("anomaly"),bAnomaly);S47J::Set(O,TEXT("phenomenon"),PhenomenonId);
 S47J::Set(O,TEXT("selected"),Selected);S47J::Set(O,TEXT("audioSource"),AudioSourceIndex);S47J::Set(O,TEXT("audioCursor"),AudioCursor);S47J::Set(O,TEXT("settle"),Settle);S47J::Set(O,TEXT("cross"),CrossPoint);S47J::Set(O,TEXT("crossSample"),CrossSample);S47J::Set(O,TEXT("crossRunning"),bCrossRunning);
 TArray<TSharedPtr<FJsonValue>> Jobs;
 for(const FJob& J:Work)
 {
  auto A=S47J::New();S47J::Set(A,TEXT("id"),J.Id);S47J::Set(A,TEXT("day"),J.Day);S47J::Set(A,TEXT("slot"),J.Slot);S47J::Set(A,TEXT("source"),J.Source);S47J::Set(A,TEXT("phenomenon"),J.Phenomenon);S47J::Set(A,TEXT("stage"),int32(J.Stage));S47J::Set(A,TEXT("elapsed"),J.Elapsed);S47J::Set(A,TEXT("quality"),J.Quality);S47J::Set(A,TEXT("anomaly"),J.bAnomaly);S47J::Set(A,TEXT("control"),J.bControl);S47J::Set(A,TEXT("reviewed"),J.bAudioReviewed);S47J::Set(A,TEXT("printed"),J.bPrinted);S47J::Set(A,TEXT("paid"),J.bPaid);S47J::Set(A,TEXT("controlAt"),J.ControlAt);S47J::Set(A,TEXT("originalCovered"),J.OriginalCovered);S47J::Set(A,TEXT("controlCovered"),J.ControlCovered);S47J::Set(A,TEXT("markStart"),J.MarkStart);S47J::Set(A,TEXT("markEnd"),J.MarkEnd);Jobs.Add(MakeShared<FJsonValueObject>(A));
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
 FS47FreeformSession S(nullptr);S.Begin(nullptr,false);
 Check(S.Active()&&S.Day()==1&&S.Clock()==8*3600&&S.Budget()==120&&S.Jobs().Num()==3,TEXT("Freeform starts a separate day with three jobs and operating budget"));
 S.Spend(120);Check(!S.Spend(5)&&S.Spend(5,true)&&S.Debt()==5&&!S.Spend(5,true),TEXT("Freeform emergency requisition is bounded and cannot repeatedly mint stock"));
 S.Earn(20);Check(S.Budget()==15&&S.Debt()==0,TEXT("Freeform job income repays emergency credit before cash"));
 S.RecordEvidence(TEXT("extra_pulse"),TEXT("retained"));S.RecordEvidence(TEXT("extra_pulse"),TEXT("changed"));Check(S.Findings.FindRef(TEXT("extra_pulse"))==TEXT("retained"),TEXT("Freeform retained original facts are not overwritten by repeats"));
 S.TestAdvance(86400);Check(S.Day()==2&&S.Jobs().Num()==6,TEXT("Freeform new days retain unfinished work and create distinct daily job identities"));
 auto Saved=S.Save();FS47FreeformSession R(nullptr);R.Begin(S47J::Read(S47J::Write(Saved)));Check(R.Day()==2&&R.Budget()==15&&R.HasFinding(TEXT("extra_pulse"))&&R.Jobs().Num()==6&&!R.AudioPlaying(),TEXT("Freeform disk JSON restore retains economy, days, evidence and paused audio"));
 R.RecordEvidence(TEXT("feeder_fault"),TEXT("control"));R.RecordEvidence(TEXT("wrong_source"),TEXT("adjacent beam"));Check(R.MysterySteps[0]==3,TEXT("Freeform mystery comparison requires independent records across days"));
 auto Future=S47J::New();S47J::Set(Future,TEXT("v"),2);R.Begin(Future);Check(!R.Active(),TEXT("Unknown Freeform save revisions do not restart a fresh session"));
 S.Stop();R.Stop();return OK;
}
