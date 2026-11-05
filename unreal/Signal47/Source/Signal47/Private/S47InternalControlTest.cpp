#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47Flow.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47Story.h"
#include "S47Startup.h"
#include "S47Inter.h"
#include "S47FreeformAudioDesk.h"
#include "UnrealClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

// One unusual observation through the receiver's RECORD tab: explanation, real button, timed control, next hint.
void FS47FreeformTest::StepInternalControl(double Dt)
{
 using J=FS47FreeformSession::EJobStage;
 static double TapeFrom=0;
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>150){Finish(false,FString::Printf(TEXT("internal control exceeded its time bound at stage %d"),Stage));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI)return;
 auto& F=*M->Freeform;
 auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Root/Name,true,false);};
 auto Scan=[&](const FString& Label,TArray<TSharedRef<SButton>>& Found,bool& TextSeen)
 {
  TFunction<void(TSharedRef<SWidget>)> Visit=[&](TSharedRef<SWidget> X)
  {
   if(X->GetTypeAsString()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(X)->GetText().ToString().Contains(Label))TextSeen=true;
   if(X->GetTypeAsString()==TEXT("SButton"))
   {
    bool Match=false;TFunction<void(TSharedRef<SWidget>)> Text=[&](TSharedRef<SWidget> W){if(W->GetTypeAsString()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(W)->GetText().ToString().TrimStartAndEnd()==Label)Match=true;FChildren* Ch=W->GetChildren();for(int32 I=0;Ch&&I<Ch->Num();++I)Text(Ch->GetChildAt(I));};
    Text(X);if(Match)Found.Add(StaticCastSharedRef<SButton>(X));
   }
   FChildren* Ch=X->GetChildren();for(int32 I=0;Ch&&I<Ch->Num();++I)Visit(Ch->GetChildAt(I));
  };
  TSet<const SWidget*> Seen;
  if(FSlateApplication::IsInitialized())FSlateApplication::Get().ForEachUser([&](FSlateUser& User)
  {auto W=User.GetFocusedWidget();if(!W)return;while(auto P=W->GetParentWidget())W=P;if(!Seen.Contains(W.Get())){Seen.Add(W.Get());Visit(W.ToSharedRef());}},true);
 };
 auto Click=[&](const FString& Label)
 {
  TArray<TSharedRef<SButton>> Found;bool TextSeen=false;Scan(Label,Found,TextSeen);
  if(!Check(Found.Num()==1&&Found[0]->IsEnabled(),TEXT("unique enabled button: ")+Label))return false;
  Found[0]->SimulateClick();return true;
 };
 auto Shows=[&](const FString& Text){TArray<TSharedRef<SButton>> Found;bool TextSeen=false;Scan(Text,Found,TextSeen);return TextSeen;};
 if(Stage==0)
 {
  if(Age<3)return;M->UI->Close(true);F.Director->Stop();
  int32 Pending=INDEX_NONE;for(int32 I=0;I<F.Jobs().Num();++I){const auto& Job=F.Jobs()[I];if(Job.Day==F.Day()&&Job.Source==1&&Job.Stage<=J::Pointed){Pending=I;break;}}
  if(!Check(Pending!=INDEX_NONE&&F.SelectJob(Pending),TEXT("existing observation selected")))return;
  F.SetAnomaly(true);
  const auto* P=F.ConsoleProfile();if(!Check(P&&F.CurrentJob(),TEXT("observation has a work card")))return;
  auto& Rx=M->Story->Rx;Rx.Frequency=P->F;Rx.Azimuth=(P->Az[0]+P->Az[1])/2;Rx.Gain=(P->Gain[0]+P->Gain[1])/2;Rx.Bandwidth=(P->Bw[0]+P->Bw[1])/2;
  if(!Check(F.PointSelected(),TEXT("array commanded")))return;
  Go(1);return;
 }
 if(Stage==1){if(!F.StartAcquisition())return;Go(2);return;}
 if(Stage==2)
 {
  if(F.CurrentJob()->Stage!=J::Chart||F.PrinterProgress()<1)return;
  if(!Check(F.CollectChart()&&F.CurrentJob()->Stage==J::Recorded&&F.CurrentJob()->bAnomaly&&!F.CurrentJob()->bControl,TEXT("unusual original retained, control not yet recorded")))return;
  if(!Check(F.JobNextAction(*F.CurrentJob()).Contains(TEXT("RECORD TAB"))&&F.JobLocation(*F.CurrentJob()).Contains(TEXT("RECORD tab")),TEXT("hint and location name the RECORD tab: ")+F.JobNextAction(*F.CurrentJob())))return;
  M->Player->Place(0,-2.15,0);F.OpenReceiver();Go(3,1.);return;
 }
 // The other way in: the physical receiver rack by the window.
 if(Stage==3&&FParse::Param(FCommandLine::Get(),TEXT("S47InternalControlRack")))
 {
  M->UI->Close(true);M->Player->Place(4.4,-2.24,0);
  const FVector Eye(4.4,M->Player->FloorY+M->Player->Eye,-2.24),D=FVector(5.265,1.07,-2.24)-Eye;
  M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());Go(30,1.);return;
 }
 if(Stage==30)
 {
  const FString Pick=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
  if(!Check(Pick==TEXT("rack")&&bool(M->Player->OnUse),TEXT("actual eye-ray selects the receiver rack; got ")+Pick+TEXT(", label ")+F.Label(TEXT("rack"))))return;
  Shot(TEXT("rack-aimed.png"));M->Player->OnUse(TEXT("rack"));
  if(!Check(F.CurrentJob()->bControl,TEXT("using the physical rack starts the control")))return;
  Go(31);return;
 }
 if(Stage==31)
 {
  if(F.CurrentJob()->ControlAt<8)return;
  if(!Check(F.CurrentJob()->bPrinted,TEXT("control completes with the original kept")))return;
  Finish(true,TEXT("Physical receiver rack starts the timed control."));return;
 }
 if(Stage==3){if(!Click(TEXT("RECORD")))return;Go(4,1.);return;}
 if(Stage==4)
 {
  if(!Check(Shows(TEXT("THE SKY OR THE STATION"))&&Shows(TEXT("NOT RECORDED"))&&Shows(TEXT("reference signal")),TEXT("RECORD tab explains the control and shows it is not recorded")))return;
  Shot(TEXT("control-before.png"));Go(5,1.);return;
 }
 if(Stage==5)
 {
  if(!Click(TEXT("RECORD INTERNAL CONTROL / 8 SECONDS")))return;
  if(!Check(F.CurrentJob()->bControl,TEXT("the on-screen button starts the control")))return;
  Go(6,2.);return;
 }
 if(Stage==6)
 {
  if(!Check(Shows(TEXT("RUNNING"))&&F.CurrentJob()->ControlAt>0&&F.CurrentJob()->ControlAt<8,FString::Printf(TEXT("panel shows the control running, %.1f s"),F.CurrentJob()->ControlAt)))return;
  TArray<TSharedRef<SButton>> Found;bool TextSeen=false;Scan(TEXT("RECORD INTERNAL CONTROL / 8 SECONDS"),Found,TextSeen);
  if(!Check(Found.Num()==0,TEXT("start button is gone while it runs")))return;
  Shot(TEXT("control-running.png"));Go(7);return;
 }
 if(Stage==7)
 {
  if(F.CurrentJob()->ControlAt<8)return;
  if(!Check(Shows(TEXT("COMPLETE")),TEXT("panel shows the control complete")))return;
  Shot(TEXT("control-complete.png"));Go(70,1.);return;
 }
 if(Stage==70)
 {
  M->UI->Close(true);const FVector T=F.AudioDesk->InteractionPoint(TEXT("ff:audio:original"));
  M->Player->Place(T.X-.2,T.Z-1.45,UE_DOUBLE_PI);M->Player->Pitch=-.12;Go(71,6.);return; // the toast has gone
 }
 if(Stage==71){Shot(TEXT("deck-sign.png"));Go(72,.6);return;}
 if(Stage==72)
 {
  if(!Check(F.Label(TEXT("ff:audio:station")).Contains(TEXT("Reel deck"))&&F.AudioDesk->Use(TEXT("ff:audio:station"))&&F.TapePanelOpen()&&M->ModalOpen(),TEXT("using the deck opens the comparison panel")))return;
  Go(73,1.);return;
 }
 if(Stage==73)
 {
  if(!Check(Shows(TEXT("WHAT TO DO HERE"))&&Shows(TEXT("Listen to the whole ORIGINAL")),TEXT("panel says what to do first")))return;
  Shot(TEXT("deck-panel.png"));Go(74,.5);return;
 }
 if(Stage==74){if(!Click(TEXT("PLAY")))return;TapeFrom=F.AudioPosition();Go(75,3.);return;}
 if(Stage==75)
 {
  if(!Check((F.AudioPlaying()||F.AudioPreparing())&&!F.Held(),TEXT("tape keeps running with the panel open")))return;
  if(!Check(F.AudioPlaying()&&F.AudioPosition()>TapeFrom,FString::Printf(TEXT("tape position advances, %.1f s"),F.AudioPosition())))return;
  Shot(TEXT("deck-panel-playing.png"));Go(76,.5);return;
 }
 if(Stage==76){if(!Click(TEXT("STOP")))return;M->UI->Close(true);if(!Check(!F.TapePanelOpen(),TEXT("closing the panel clears its hold exemption")))return;Go(8,.3);return;}
 if(Stage==8)
 {
  const FString Next=F.JobNextAction(*F.CurrentJob());
  if(!Check(F.CurrentJob()->bPrinted&&Next.Contains(TEXT(":")),TEXT("original kept and next step named: ")+Next))return;
  Finish(true,TEXT("Unusual observation through the RECORD tab: explanation, real button, timed control and next step."));return;
 }
}
