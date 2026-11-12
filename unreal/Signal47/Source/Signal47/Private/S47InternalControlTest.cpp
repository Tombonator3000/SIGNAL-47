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

// GAMEPLAY-LOOP-01 (Tom, 12 November): one unusual observation along its six steps, each one thing at one place: ANOMALY DETECTED on
// the receiver, record it there, one listen at the reel deck, confirm at OPS, the packet at the printer (the binder is walked in the
// card player, S47DevFreeform). No control to record, no second reel, no chart to fetch. (The name is the old internal-control test's.)
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
 if(Stage==1)
 {
  // 1: ANOMALY DETECTED, on the HUD's line and on the receiver's glass, once the card is tuned and the array steady
  if(!F.TerminalLines(1).Contains(TEXT("ANOMALY DETECTED")))return;
  const FString Said=F.JobNextAction(*F.CurrentJob());
  if(!Check(F.AnomalyAhead()&&Said.StartsWith(TEXT("RECEIVER"))&&Said.Contains(TEXT("ANOMALY DETECTED")),TEXT("the tuned receiver says ANOMALY DETECTED before the take: ")+Said))return;
  // 2: record it there
  if(!Check(F.StartAcquisition()&&F.CurrentJob()->bAnomaly&&F.JobNextAction(*F.CurrentJob()).Contains(TEXT("ANOMALY DETECTED")),TEXT("START RECORDING takes the unusual take, still marked as such: ")+F.JobNextAction(*F.CurrentJob())))return;
  Go(2);return;
 }
 if(Stage==2)
 {
  if(F.CurrentJob()->Stage!=J::Recorded)return;
  const auto* Take=F.CurrentJob();
  if(!Check(Take->bAnomaly&&Take->bPrinted&&!Take->OriginalPage.IsEmpty()&&!Take->bControl&&!F.CanRecordControl()&&F.PrinterJobId()!=Take->Id,TEXT("the take is kept as it ends: original in the notebook, no chart at the printer, no control asked for")))return;
  if(!Check(F.JobNextAction(*Take).StartsWith(TEXT("REEL DECK"))&&F.JobLocation(*Take).StartsWith(TEXT("Reel deck"))&&F.TerminalLines(1).Contains(TEXT("ANOMALY DETECTED")),TEXT("hint and location name the reel deck: ")+F.JobNextAction(*Take)))return;
  M->Player->Place(0,-2.15,0);F.OpenReceiver();Go(20,1.);return;
 }
 if(Stage==20){if(!Click(TEXT("ACQUIRE")))return;Go(21,1.);return;}
 if(Stage==21)
 {
  TArray<TSharedRef<SButton>> Keys;bool KeySeen=false;Scan(TEXT("ON TAPE / REEL DECK"),Keys,KeySeen);
  if(!Check(Keys.Num()==1&&!Keys[0]->IsEnabled(),TEXT("ACQUIRE says the take is on tape and the reel deck is next; nothing to press here")))return;
  TArray<TSharedRef<SButton>> Control;bool ControlSeen=false;Scan(TEXT("RECORD CONTROL / 8 s"),Control,ControlSeen);
  if(!Check(Control.Num()==0,TEXT("the receiver offers no control for the unusual take")))return;
  TSharedPtr<SWidget> Up=Keys[0];while(Up.IsValid()&&Up->GetTypeAsString()!=TEXT("SScrollBox"))Up=Up->GetParentWidget();
  const float Over=Up.IsValid()?StaticCastSharedPtr<SScrollBox>(Up)->GetScrollOffsetOfEnd():-1.f;
  UE_LOG(LogTemp,Display,TEXT("S47 receiver ACQUIRE with the tape strip: %.1f units past the glass"),Over);
  if(!Check(Up.IsValid()&&Over<1.f,FString::Printf(TEXT("ACQUIRE with the tape strip fits the glass without scrolling (%.1f over)"),Over)))return;
  if(!Check(Shows(TEXT("ANOMALY ON TAPE")),TEXT("the receiver's banner names the anomaly on tape")))return;
  Shot(TEXT("acquire-after-take.png"));Go(70,1.);return;
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
  if(!Check(Shows(TEXT("PLAY THE TAKE ONCE"))&&Shows(TEXT("CONFIRMED: TO OPS"))&&Shows(TEXT("KNOWN SOUNDS TO COMPARE BY EAR")),TEXT("panel says what to do first")))return;
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
 if(Stage==8)
 {
  // 3: the rest of the take, from where it stopped: one listen confirms it (the marks set by hand are kept)
  F.OpenAudio();if(!Check(F.AudioAction(TEXT("play")),TEXT("PLAY resumes the take at the deck")))return;Go(81);return;
 }
 if(Stage==81)
 {
  if(F.AudioPlaying()||F.AudioPreparing())return;
  const auto* Take=F.CurrentJob();
  if(!Check(Take->bAudioReviewed&&F.HasFinding(TEXT("pulse-review"))&&Take->ControlCovered==0&&FMath::IsNearlyEqual(Take->MarkStart,9.,.05)&&FMath::IsNearlyEqual(Take->MarkEnd,13.,.05),TEXT("one listen confirms the anomaly and keeps the hand marks")))return;
  if(!Check(F.JobNextAction(*Take).StartsWith(TEXT("OPS")),TEXT("then OPS: ")+F.JobNextAction(*Take)))return;
  // 4: confirm at OPS, one button
  M->UI->Close(true);
  if(!Check(!F.RegisterComparison(TEXT("sky"))&&F.CurrentJob()->Stage==J::Recorded,TEXT("OPS takes no origin it cannot show")))return;
  if(!Check(F.RegisterComparison(TEXT("unresolved"))&&F.CurrentJob()->Stage==J::Registered,TEXT("Confirm anomaly registers it")))return;
  Go(82);return;
 }
 if(Stage==82)
 {
  // 5: the packet at the printer, then 6: the binder
  if(F.PrinterProgress()<1)return;
  if(!Check(F.CollectChart()&&F.CurrentJob()->Stage==J::Packet&&F.JobNextAction(*F.CurrentJob()).Contains(TEXT("BINDER")),TEXT("the printer gives the packet and the binder is next")))return;
  M->UI->Close(true);
  Finish(true,TEXT("Unusual take: ANOMALY DETECTED, recorded, one listen at the reel deck, confirmed at OPS, packet printed; the binder is next."));return;
 }
}
