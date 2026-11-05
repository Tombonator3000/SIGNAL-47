#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47FieldLogger.h"
#include "S47Flow.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47Inter.h"
#include "S47Walker.h"
#include "S47Three.h"
#include "S47Startup.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformTime.h"

struct FS47LoggerJourneyState
{
 double Began=0,Heartbeat=0,Start=0,BeforeSeconds=0,BeforeBattery=0,BeforeOverlap=0;
 int32 Cash=0,Debt=0,Findings=0;FString Tape,Job;S47J::FObj Finished;
};
void FS47FreeformTest::StepLoggerJourney(double Dt)
{
 if(!LoggerJourney){LoggerJourney=MakeShared<FS47LoggerJourneyState>();LoggerJourney->Began=FPlatformTime::Seconds();}
 auto& S=*LoggerJourney;const double Wall=FPlatformTime::Seconds();
 if(Wall-S.Began>1200){Finish(false,TEXT("logger journey exceeded 20-minute wall bound; no time or coverage correction allowed"));return;}
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->Inter)return;
 auto& F=*M->Freeform;if(!F.Field)return;auto& W=*F.Field;const auto& L=W.FieldLogger();
 auto Approach=[&](const FString& Id)
 {
  M->UI->Close(true);const auto P=W.StandPoint(Id);auto T=W.InteractionPoint(Id);if(Id==TEXT("ff:logger-a"))T.Y-=.2;M->Player->Place(P.X,P.Z,0);
  const FVector D=T-FVector(P.X,M->Player->FloorY+M->Player->Eye,P.Z);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
 };
 auto Use=[&](const FString& Id)
 {
  const FString Pick=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
  if(!Check(Pick==Id&&bool(M->Player->OnUse),TEXT("actual physical eye-ray selects ")+Id+TEXT("; got ")+Pick))return false;
  M->Player->OnUse(Id);return true;
 };
 auto Tape=[&]()->const FS47FieldLogger::FTape* {return L.Tapes.FindByPredicate([&](const FS47FieldLogger::FTape& T){return T.Id==S.Tape;});};
 if(Stage==0)
 {
  if(Age<3)return;M->UI->Close(true);
  if(!Check(!L.bLoanIssued&&L.Tapes.IsEmpty()&&W.ParcelStage()==0,TEXT("fresh explicit loan journey start; no kit or prior tape")))return;
  S.Cash=F.Budget();S.Debt=F.Debt();auto State=S47J::Clone(F.Save());auto Cases=S47J::Obj(State,TEXT("cases"));
  if(!Check(Cases.IsValid(),TEXT("case access fixture is serializable")))return;
  auto Access=S47J::Strs(Cases,TEXT("access"));Access.AddUnique(TEXT("access.fieldlogger"));S47J::Set(Cases,TEXT("access"),Access);
  F.Begin(State);Check(true,TEXT("DECLARED fixture: access.fieldlogger granted only as test prerequisite; no natural case completion claimed"));Go(9800,.4);return;
 }
 if(Stage==9800)
 {
  if(!Check(W.LoggerAccess()&&F.Budget()==S.Cash&&F.Debt()==S.Debt,TEXT("access fixture preserves economy")))return;
  // Legitimate production acceptance, not a tutorial/stage assignment.
  if(F.InTutorial())F.AcceptTutorialJob();
  if(!Check(F.CurrentJob()&&F.CurrentJob()->Stage==FS47FreeformSession::EJobStage::New,TEXT("existing fresh SARO card available without generated completion fixture")))return;
  S.Job=F.CurrentJob()->Id;Approach(TEXT("ff:depot"));Go(9801,.4);return;
 }
 if(Stage==9801)
 {
  if(!Use(TEXT("ff:depot")))return;
  if(!Check(W.LoggerAction(TEXT("issue"))&&L.bCheckedOut&&L.Checkouts==1&&W.ParcelItem()==TEXT("fieldlogger"),TEXT("production issue callback at actual depot creates one loan kit")))return;
  Approach(TEXT("ff:bench"));Go(9802,.4);return;
 }
 if(Stage==9802)
 {
  if(!Use(TEXT("ff:bench")))return;M->UI->Close(true);
  if(!Check(!L.bCheckedOut&&W.LoggerAction(TEXT("connect"))&&W.LoggerAction(TEXT("connect"))&&W.LoggerAction(TEXT("connect"))&&!W.LoggerAction(TEXT("connect")),TEXT("physical bench return and exactly three production assembly connections")))return;
  if(!Check(W.LoggerAction(TEXT("calibrate"))&&!L.bCalibrated,TEXT("starts real powered bench calibration")))return;Go(9803);return;
 }
 if(Stage==9803)
 {
  if(!L.bCalibrated){if(Age-At>20)Check(false,TEXT("bench calibration did not complete naturally"));return;}
  if(!Check(L.Calibration==3&&W.LoggerAction(TEXT("tune"))&&W.LoggerAction(TEXT("take")),TEXT("actual three-second calibration, card tuning and physical collection")))return;
  Approach(TEXT("ff:logger-a"));Go(9804,.4);return;
 }
 if(Stage==9804)
 {
  if(!Use(TEXT("ff:logger-a")))return;
  if(!Check(L.bRecording&&L.Site==0&&L.Tapes.Num()==1&&W.ParcelStage()==0,TEXT("physical field-post use deploys original device and starts actual recording")))return;
  S.Tape=L.Tapes.Last().Id;S.Start=L.Tapes.Last().Start;
  // Declared receiver approach and dial fixtures. Dish movement/settling and
  // acquisition use production commands and actual elapsed frames.
  M->UI->Close(true);M->Player->Place(0,-2.15,0);const auto* P=F.ConsoleProfile();
  if(!Check(P&&F.PointSelected(),TEXT("production command begins real array pointing")))return;
  M->Story->Rx.Frequency=P->F;M->Story->Rx.Gain=(P->Gain[0]+P->Gain[1])*.5;M->Story->Rx.Bandwidth=(P->Bw[0]+P->Bw[1])*.5;
  Go(9805);return;
 }
 if(Stage==9805)
 {
  if(F.StartAcquisition()){Check(true,TEXT("production acquisition accepts actual settled array and declared dial settings"));Go(9806);return;}
  if(Age-At>90)Check(false,TEXT("real acquisition remained unavailable; no readiness injection"));return;
 }
 if(Stage==9806)
 {
  const auto* J=F.CurrentJob();if(!J||J->Id!=S.Job){Check(false,TEXT("selected original identity changed during acquisition"));return;}
  if(J->Stage!=FS47FreeformSession::EJobStage::Chart){if(Age-At>120)Check(false,TEXT("actual SARO acquisition failed to finish"));return;}
  if(F.PrinterProgress()<1.)return;
  const auto* It=M->Inter->Get(TEXT("printer"));const bool HasObject=It&&bool(It->Object);
  if(!Check(HasObject||M->Web->Item(TEXT("printer")),TEXT("physical printer target available from registered object or exported Web item")))return;
  const FVector T=HasObject?It->Object.WorldBox().GetCenter():S47Three::ToThree(M->Web->PropLocation(TEXT("printer")));M->UI->Close(true);M->Player->Place(T.X,T.Z+1,0);
  const FVector D=T-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());Go(9807,.4);return;
 }
 if(Stage==9807)
 {
  if(!Use(TEXT("printer")))return;M->UI->Close(true);const auto* T=Tape();
  if(!Check(F.CurrentJob()&&F.CurrentJob()->Stage==FS47FreeformSession::EJobStage::Recorded&&T&&T->Job==S.Job&&T->Overlap>=5,TEXT("physical printer yields actual SARO original; deployed tape retains matching acquisition overlap")))return;Go(9808);return;
 }
 if(Stage==9808||Stage==9810)
 {
  const auto* T=Tape();if(!T||T->GapSeconds!=0||T->bBatteryEmpty){Check(false,TEXT("active field window lost original, has a gap or exhausted battery"));return;}
  if(Wall-S.Heartbeat>=30)
  {
   S.Heartbeat=Wall;UE_LOG(LogTemp,Display,TEXT("S47 logger journey heartbeat wall=%.1f clock=%.3f samples=%.3f overlap=%.3f gap=%.3f battery=%.3f held=%d stage=%d"),Wall-S.Began,F.Clock(),T->Seconds,T->Overlap,T->GapSeconds,L.Battery,F.Held(),Stage);
  }
  if(Stage==9808&&T->Seconds>=FS47FieldLogger::Window*.5)
  {
   S.BeforeSeconds=T->Seconds;S.BeforeBattery=L.Battery;S.BeforeOverlap=T->Overlap;
   if(!Restore(TEXT("loggerjourney-mid-active-window")))return;Go(9809,.4);return;
  }
  if(Stage==9810&&!L.bRecording)
  {
   if(!Check(T->Seconds>=FS47FieldLogger::Window-.001&&T->End-T->Start>=FS47FieldLogger::Window-.001&&T->Overlap>=5,TEXT("full two-hour window accrued only through actual active game frames")))return;
   Approach(TEXT("ff:logger-device"));Go(9811,.4);return;
  }
  return;
 }
 if(Stage==9809)
 {
  const auto* T=Tape();if(!Check(T&&L.Site==0&&L.bRecording&&T->Start==S.Start&&T->Seconds>=S.BeforeSeconds&&T->Seconds-S.BeforeSeconds<30&&T->Overlap==S.BeforeOverlap&&T->GapSeconds==0&&L.Battery<=S.BeforeBattery,TEXT("disk/Flow Continue retains active tape and measured history without inventing past overlap")))return;Go(9810);return;
 }
 if(Stage==9811)
 {
  if(!Use(TEXT("ff:logger-device")))return;
  if(!Check(Tape()&&FS47FieldLogger::CompleteWindow(*Tape())&&W.ParcelItem()==TEXT("fieldlogger"),TEXT("actual field retrieval seals a complete matching original")))return;
  Approach(TEXT("ff:bench"));Go(9812,.4);return;
 }
 if(Stage==9812)
 {
  if(!Use(TEXT("ff:bench")))return;M->UI->Close(true);S.Findings=F.FindingCount();
  if(!Check(W.LoggerAction(TEXT("compare"))&&Tape()&&Tape()->bCompared&&Tape()->ComparedJob==S.Job&&FS47FieldLogger::OverlapFor(*Tape(),S.Job)>=5&&F.HasFinding(S.Tape)&&F.FindingCount()==S.Findings+1,TEXT("production comparison callback at physical bench retains one local-noise comparison")))return;
  M->UI->Close(true);if(!Check(W.LoggerAction(TEXT("compare"))&&F.FindingCount()==S.Findings+1&&F.Budget()==S.Cash&&F.Debt()==S.Debt,TEXT("reopening comparison adds neither duplicate finding nor money")))return;
  M->UI->Close(true);S.Finished=S47J::Clone(L.Save());if(!Restore(TEXT("loggerjourney-returned-compared")))return;Go(9813,.4);return;
 }
 if(Stage==9813)
 {
  if(!Check(Tape()&&Tape()->ComparedJob==S.Job&&FS47FieldLogger::CompleteWindow(*Tape(),S.Job)&&FJsonValue::CompareEqual(FJsonValueObject(L.Save()),FJsonValueObject(S.Finished))&&F.HasFinding(S.Tape)&&F.Budget()==S.Cash&&F.Debt()==S.Debt,TEXT("Continue preserves exact returned logger/tape/comparison and unchanged economy")))return;
  auto O=S47J::New();S47J::Set(O,TEXT("method"),TEXT("Access, approach and dial fixtures; production actions; no walking/native-menu or natural access claim"));S47J::Set(O,TEXT("wallSeconds"),Wall-S.Began);S47J::Set(O,TEXT("logger"),L.Save());S47J::Set(O,TEXT("job"),S.Job);S47J::Set(O,TEXT("continues"),Restores);S47J::Set(O,TEXT("visitorsContactEvidence"),false);
  FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("logger-active-window.json")));
  Finish(true,TEXT("Full actively sampled RF field window, actual matching SARO acquisition, physical custody/comparison and two disk/Flow Continues; declared access/approach/dial fixtures."));return;
 }
}
