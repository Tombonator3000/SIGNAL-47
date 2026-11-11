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
#include "SS47TapeScope.h"
#include "UnrealClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

// One unusual observation through the receiver's RECORD tab: explanation, real button, timed control, next hint.
// D45-20261110: -S47InternalControlEarly takes Tom's new order instead: the control straight after the take on the receiver's own
// ACQUIRE key (the chart left at the printer), the reel deck, then OPS naming the printer, then the chart.
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
 const bool bEarly=FParse::Param(FCommandLine::Get(),TEXT("S47InternalControlEarly"));
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
 if(Stage==2&&bEarly)
 {
  if(F.CurrentJob()->Stage!=J::Chart)return;
  if(!Check(F.CurrentJob()->bAnomaly&&!F.CurrentJob()->bPrinted&&!F.CurrentJob()->bControl&&F.CanRecordControl(),TEXT("D45: the unusual take is on tape with its chart still at the printer, and its control can be recorded")))return;
  if(!Check(F.JobNextAction(*F.CurrentJob()).StartsWith(TEXT("RECEIVER"))&&F.JobLocation(*F.CurrentJob()).StartsWith(TEXT("Receiver")),TEXT("D45: hint and location name the receiver: ")+F.JobNextAction(*F.CurrentJob())))return;
  M->Player->Place(0,-2.15,0);F.OpenReceiver();Go(20,1.);return;
 }
 if(Stage==20){if(!Click(TEXT("ACQUIRE")))return;Go(21,1.);return;}
 if(Stage==21)
 {
  TArray<TSharedRef<SButton>> Keys;bool KeySeen=false;Scan(TEXT("RECORD CONTROL / 8 s"),Keys,KeySeen);
  if(!Check(Keys.Num()==1&&Keys[0]->IsEnabled(),TEXT("D45: ACQUIRE offers RECORD CONTROL / 8 s right after the take")))return;
  TSharedPtr<SWidget> Up=Keys[0];while(Up.IsValid()&&Up->GetTypeAsString()!=TEXT("SScrollBox"))Up=Up->GetParentWidget();
  const float Over=Up.IsValid()?StaticCastSharedPtr<SScrollBox>(Up)->GetScrollOffsetOfEnd():-1.f;
  UE_LOG(LogTemp,Display,TEXT("S47 D45 receiver ACQUIRE with the tape strip: %.1f units past the glass"),Over);
  if(!Check(Up.IsValid()&&Over<1.f,FString::Printf(TEXT("D45: ACQUIRE with the tape strip fits the glass without scrolling (%.1f over)"),Over)))return;
  if(!Check(Shows(TEXT("LOCKED")),TEXT("D45: the ARRAY line shows the lock")))return;
  Shot(TEXT("acquire-after-take.png"));Keys[0]->SimulateClick();
  if(!Check(F.CurrentJob()->bControl&&F.CurrentJob()->Stage==J::Chart,TEXT("D45: the receiver's own key starts the control before the chart is collected")))return;
  Go(22,2.);return;
 }
 if(Stage==22){Shot(TEXT("acquire-control-running.png"));Go(23);return;}
 if(Stage==23)
 {
  if(F.CurrentJob()->ControlAt<8)return;
  if(!Check(F.CurrentJob()->Stage==J::Chart&&F.JobNextAction(*F.CurrentJob()).StartsWith(TEXT("REEL DECK")),TEXT("D45: after the control the reel deck is next, the chart still waiting: ")+F.JobNextAction(*F.CurrentJob())))return;
  Go(70,1.);return;
 }
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
  M->Player->Place(T.X-.5,T.Z-1.45,UE_DOUBLE_PI);M->Player->Pitch=-.12;Go(71,6.);return; // the toast has gone
 }
 if(Stage==71){Shot(TEXT("deck-sign.png"));Go(710,.6);return;}
 if(Stage==710)
 {
  // The receiver stack now stands on the table; looking at it leads to the same panel.
  M->Player->Place(-.75,3.05,UE_DOUBLE_PI);
  const FVector Eye(-.75,M->Player->FloorY+M->Player->Eye,3.05),D=FVector(-.75,.86,4.0)-Eye;
  M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());Go(711,1.);return;
 }
 if(Stage==711)
 {
  const FString Pick=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
  UE_LOG(LogTemp,Display,TEXT("S47 stack pick=%s label=%s"),*Pick,*F.Label(Pick));
  if(!Check(F.Label(Pick).Contains(TEXT("Reel deck")),TEXT("the stack on the table offers the reel deck; picked ")+Pick))return;
  Shot(TEXT("stack-on-table.png"));Go(712,.6);return;
 }
 if(Stage==712){M->Player->Place(2.3,-2.3,0);M->Player->Pitch=-.25;Go(713,1.);return;}
 if(Stage==713)
 {
  Shot(TEXT("console-after.png"));
  const FVector T=F.AudioDesk->InteractionPoint(TEXT("ff:audio:original"));M->Player->Place(T.X,T.Z-1.45,UE_DOUBLE_PI);Go(72,.8);return;
 }
 if(Stage==72)
 {
  if(!Check(F.Label(TEXT("ff:audio:station")).Contains(TEXT("Reel deck"))&&F.AudioDesk->Use(TEXT("ff:audio:station"))&&F.TapePanelOpen()&&M->ModalOpen(),TEXT("using the deck opens the comparison panel")))return;
  Go(73,1.);return;
 }
 if(Stage==73)
 {
  if(!Check(Shows(TEXT("LISTEN TO THE WHOLE ORIGINAL"))&&Shows(TEXT("MARK THE EXTRA TICKS"))&&Shows(TEXT("KNOWN SOUNDS TO COMPARE BY EAR")),TEXT("panel says what to do first")))return;
  // D45-20261110: one faceplate, no scrolling: its last row stands above the way out
  TArray<TSharedRef<SButton>> Last,Out;bool LastSeen=false,OutSeen=false;Scan(TEXT("NEXT REFERENCE"),Last,LastSeen);Scan(TEXT("ESC / RETURN TO ROOM"),Out,OutSeen);
  if(!Check(Last.Num()==1&&Out.Num()==1,TEXT("D45: the deck has its references row and its way out")))return;
  const FGeometry LastG=Last[0]->GetTickSpaceGeometry(),OutG=Out[0]->GetTickSpaceGeometry();
  const FVector2D LastBottom=LastG.LocalToAbsolute(FVector2D(LastG.GetLocalSize())),OutTop=OutG.LocalToAbsolute(FVector2D(0,0));
  UE_LOG(LogTemp,Display,TEXT("S47 D45 reel deck: last row ends at %.1f, the way out starts at %.1f"),LastBottom.Y,OutTop.Y);
  if(!Check(LastG.GetLocalSize().Y>10&&LastBottom.Y<=OutTop.Y+.5,FString::Printf(TEXT("D45: the whole deck fits its faceplate (last row %.1f, way out %.1f)"),LastBottom.Y,OutTop.Y)))return;
  Shot(TEXT("deck-panel.png"));Go(74,.5);return;
 }
 if(Stage==74){if(!Click(TEXT("PLAY")))return;TapeFrom=F.AudioPosition();Go(75,3.);return;}
 if(Stage==75)
 {
  if(!Check((F.AudioPlaying()||F.AudioPreparing())&&!F.Held(),TEXT("tape keeps running with the panel open")))return;
  if(!Check(F.AudioPlaying()&&F.AudioPosition()>TapeFrom,FString::Printf(TEXT("tape position advances, %.1f s"),F.AudioPosition())))return;
  Shot(TEXT("deck-panel-playing.png"));Go(76,.5);return;
 }
 if(Stage==76){if(!Click(TEXT("STOP")))return;Go(760,.3);return;}
 if(Stage==760)
 {
  // Real pointer events on the drawn trace, right to left: the mark still reads left to right, and the tape stays put.
  TSharedPtr<SS47TapeScope> Scope;
  TFunction<void(TSharedRef<SWidget>)> Find=[&](TSharedRef<SWidget> X){if(X->GetTypeAsString()==TEXT("SS47TapeScope"))Scope=StaticCastSharedRef<SS47TapeScope>(X);FChildren* Ch=X->GetChildren();for(int32 I=0;Ch&&I<Ch->Num();++I)Find(Ch->GetChildAt(I));};
  FSlateApplication::Get().ForEachUser([&](FSlateUser& User){auto W=User.GetFocusedWidget();if(!W)return;while(auto P=W->GetParentWidget())W=P;Find(W.ToSharedRef());},true);
  if(!Check(Scope.IsValid(),TEXT("the panel shows the signal trace")))return;
  const FGeometry G=Scope->GetTickSpaceGeometry();const double Tape=F.AudioPosition();
  auto Press=[&](double At){const FVector2D P=G.LocalToAbsolute(FVector2D(Scope->LocalAt(At,FVector2f(G.GetLocalSize()))));return Scope->OnMouseButtonDown(G,FPointerEvent(0,0,P,P,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0.f,FModifierKeysState())).IsEventHandled();};
  if(!Check(Press(13)&&F.CurrentJob()->MarkEnd<0,TEXT("first click on the trace sets one end")))return;
  if(!Check(Press(9)&&FMath::IsNearlyEqual(F.CurrentJob()->MarkStart,9.,.05)&&FMath::IsNearlyEqual(F.CurrentJob()->MarkEnd,13.,.05),FString::Printf(TEXT("second click completes the mark, %.2f to %.2f s"),F.CurrentJob()->MarkStart,F.CurrentJob()->MarkEnd)))return;
  if(!Check(FMath::IsNearlyEqual(F.AudioPosition(),Tape),TEXT("marking does not move the tape")))return;
  if(!Check(Shows(TEXT("COVERS THE TICKS")),TEXT("the panel says the mark covers the ticks")))return;
  Shot(TEXT("deck-panel-marked.png"));Go(77,.3);return;
 }
 if(Stage==77)
 {
  if(!Click(TEXT("ESC / RETURN TO ROOM")))return;
  if(!Check(!F.TapePanelOpen()&&!M->ModalOpen(),TEXT("the visible button leaves the panel and clears its hold exemption")))return;
  if(!Check(F.AudioDesk->Use(TEXT("ff:audio:station"))&&F.TapePanelOpen(),TEXT("the deck opens again")))return;
  Go(78,.3);return;
 }
 if(Stage==78)
 {
  FSlateApplication& App=FSlateApplication::Get();const FKeyEvent Esc(EKeys::Escape,FModifierKeysState(),0,false,0,0);
  App.ProcessKeyDownEvent(Esc);App.ProcessKeyUpEvent(Esc);
  if(!Check(!F.TapePanelOpen()&&!M->ModalOpen()&&!M->bPausedByMenu,TEXT("a real Escape press leaves the panel without opening the pause menu")))return;
  Go(8,.3);return;
 }
 if(Stage==8&&bEarly)
 {
  if(!Check(!F.RegisterComparison(TEXT("unresolved"))&&F.FeedbackText().Contains(TEXT("PRINTER"))&&F.CurrentJob()->Stage==J::Chart,TEXT("D45: OPS names the printer while the chart waits: ")+F.FeedbackText()))return;
  Go(80);return;
 }
 if(Stage==80)
 {
  if(F.PrinterProgress()<1)return;
  if(!Check(F.CollectChart()&&F.CurrentJob()->Stage==J::Recorded&&F.CurrentJob()->bPrinted&&F.CurrentJob()->bControl&&F.CurrentJob()->ControlAt>=8,TEXT("D45: the chart collected after the control keeps the control")))return;
  if(!Check(F.JobNextAction(*F.CurrentJob()).StartsWith(TEXT("REEL DECK")),TEXT("D45: and the reel deck is still the next step: ")+F.JobNextAction(*F.CurrentJob())))return;
  Finish(true,TEXT("D45: unusual take, control at the receiver straight after it, reel deck, OPS naming the printer, chart later."));return;
 }
 if(Stage==8)
 {
  const FString Next=F.JobNextAction(*F.CurrentJob());
  if(!Check(F.CurrentJob()->bPrinted&&Next.Contains(TEXT(":")),TEXT("original kept and next step named: ")+Next))return;
  Finish(true,TEXT("Unusual observation through the RECORD tab: explanation, real button, timed control and next step."));return;
 }
}
