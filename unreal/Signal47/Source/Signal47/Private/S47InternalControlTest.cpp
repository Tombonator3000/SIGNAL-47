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
#include "S47SkyAcquire.h"
#include "S47RxLook.h"
#include "SS47TapeScope.h"
#include "UnrealClient.h"
#include "Containers/Ticker.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Layout/WidgetPath.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
 // RX-ANOMALY-FLOW-20261113: a hand's click on the reel deck panel (it stands over the game, not on a CRT's glass): the pointer moves
 // there, the button goes down now and comes up a frame later, and Slate hit-tests the place, so an overlay, a clipped box or a
 // disabled key takes or drops the click as it would for Tom.
 void IcPointerAt(const FVector2D& At)
 {
  FSlateApplication& App=FSlateApplication::Get();const TSet<FKey> None,Left={EKeys::LeftMouseButton};
  App.ProcessMouseMoveEvent(FPointerEvent(0,At,At-FVector2D(1,0),None,EKeys::Invalid,0.f,App.GetModifierKeys()));
  App.ProcessMouseButtonDownEvent(nullptr,FPointerEvent(0,At,At,Left,EKeys::LeftMouseButton,0.f,App.GetModifierKeys()));
  FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([At](float)
  {
   if(!FSlateApplication::IsInitialized())return false;
   FSlateApplication& UpApp=FSlateApplication::Get();
   UpApp.ProcessMouseButtonUpEvent(FPointerEvent(0,At,At,TSet<FKey>(),EKeys::LeftMouseButton,0.f,UpApp.GetModifierKeys()));
   return false;
  }));
 }
 // The widget is what Slate finds under the pointer at that place (it, or one of its children, is on the hit path).
 bool IcUnderPointer(const TSharedRef<SWidget>& W,const FVector2D& At)
 {
  FSlateApplication& App=FSlateApplication::Get();
  const FWidgetPath Path=App.LocateWindowUnderMouse(At,App.GetInteractiveTopLevelWindows());
  if(!Path.IsValid())return false;
  for(int32 I=0;I<Path.Widgets.Num();++I)if(&Path.Widgets[I].Widget.Get()==&W.Get())return true;
  return false;
 }
}

// GAMEPLAY-LOOP-01 (Tom, 12 November) and RX-ANOMALY-FLOW-20261113 (Tom, 13 November: «Må gå fra RECORD på receiver til bekreft lyd
// på reel ... MARK IN og MARK OUT og også trykke på stedet der lyden har avvik ... referanselyd»): one unusual observation along its
// steps, each one thing at one place, through the keys a player uses: ANOMALY DETECTED on the receiver, its RECORD tab due; START
// RECORDING there; RETAIN TAKE / RECORD REFERENCE 8 s there; the reel deck: PLAY the take, MARK IN and MARK OUT at the tape head,
// the reference on its own key, marks by clicks on the trace, RETAIN (refused while the marks miss the ticks); confirm at OPS; the
// packet at the printer (the binder is walked in the card player, S47DevFreeform). The receiver is clicked by SimulateClick (its
// glass is a virtual window reached by the camera's ray); the reel deck panel by real Slate pointer events. (The name is the old
// internal-control test's.)
void FS47FreeformTest::StepInternalControl(double Dt)
{
 using J=FS47FreeformSession::EJobStage;
 static double TapeFrom=0;
 static double LookFrom=0;static bool bLookCalmWas=false; // RX-LOOK-20261113 (stages 101 to 107)
 static TArray<float> CalmPicture;static double CalmAt=0; // RX-LOOK gjennomgang 14 Nov (stages 102 and 103)
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>240){Finish(false,FString::Printf(TEXT("internal control exceeded its time bound at stage %d"),Stage));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI)return;
 auto& F=*M->Freeform;
 auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Root/Name,true,false);};
 // what a player can see: collapsed widgets (the other half of a page that swaps) are not looked into
 auto Scan=[&](const FString& Label,TArray<TSharedRef<SButton>>& Found,bool& TextSeen)
 {
  TFunction<void(TSharedRef<SWidget>)> Visit=[&](TSharedRef<SWidget> X)
  {
   if(!X->GetVisibility().IsVisible())return;
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
 // the reel deck by the pointer: the key must be the one thing Slate finds under its centre, and enabled
 auto Hand=[&](const FString& Label)
 {
  TArray<TSharedRef<SButton>> Found;bool TextSeen=false;Scan(Label,Found,TextSeen);
  if(!Check(Found.Num()==1&&Found[0]->IsEnabled(),TEXT("reel deck: unique enabled key ")+Label))return false;
  const FGeometry G=Found[0]->GetCachedGeometry();const FVector2D Mid=G.LocalToAbsolute(G.GetLocalSize()*.5f);
  if(!Check(G.GetLocalSize().X>4&&IcUnderPointer(Found[0],Mid),TEXT("reel deck: the key is under the pointer at its own centre (nothing over it): ")+Label))return false;
  IcPointerAt(Mid);return true;
 };
 auto FindScope=[&]
 {
  TSharedPtr<SS47TapeScope> Found;
  TFunction<void(TSharedRef<SWidget>)> Find=[&](TSharedRef<SWidget> X){if(X->GetTypeAsString()==TEXT("SS47TapeScope"))Found=StaticCastSharedRef<SS47TapeScope>(X);FChildren* Ch=X->GetChildren();for(int32 I=0;Ch&&I<Ch->Num();++I)Find(Ch->GetChildAt(I));};
  FSlateApplication::Get().ForEachUser([&](FSlateUser& User){auto W=User.GetFocusedWidget();if(!W)return;while(auto P=W->GetParentWidget())W=P;Find(W.ToSharedRef());},true);
  return Found;
 };
 // a click on the drawn trace at a tape second, through hit testing
 auto Trace=[&](double Seconds)
 {
  const TSharedPtr<SS47TapeScope> Scope=FindScope();if(!Check(Scope.IsValid(),TEXT("the reel deck shows the signal trace")))return false;
  const FGeometry G=Scope->GetCachedGeometry();const FVector2D Mid=G.LocalToAbsolute(FVector2D(Scope->LocalAt(Seconds,FVector2f(G.GetLocalSize()))));
  if(!Check(IcUnderPointer(Scope.ToSharedRef(),Mid),FString::Printf(TEXT("reel deck: the trace is under the pointer at %.1f s"),Seconds)))return false;
  IcPointerAt(Mid);return true;
 };
 // RX-LOOK gjennomgang 14 Nov: the same for a page with no key of its own (ARRAY): found by a line of its text
 auto FitsText=[&](const FString& Label,const TCHAR* What)
 {
  TSharedPtr<SWidget> Found;
  TFunction<void(TSharedRef<SWidget>)> Find=[&](TSharedRef<SWidget> X){if(Found.IsValid()||!X->GetVisibility().IsVisible())return;if(X->GetTypeAsString()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(X)->GetText().ToString().Contains(Label)){Found=X;return;}FChildren* Ch=X->GetChildren();for(int32 I=0;Ch&&I<Ch->Num();++I)Find(Ch->GetChildAt(I));};
  FSlateApplication::Get().ForEachUser([&](FSlateUser& User){auto W=User.GetFocusedWidget();if(!W)return;while(auto P=W->GetParentWidget())W=P;Find(W.ToSharedRef());},true);
  TSharedPtr<SWidget> Up=Found;while(Up.IsValid()&&Up->GetTypeAsString()!=TEXT("SScrollBox"))Up=Up->GetParentWidget();
  const float Over=Up.IsValid()?StaticCastSharedPtr<SScrollBox>(Up)->GetScrollOffsetOfEnd():-1.f;
  UE_LOG(LogTemp,Display,TEXT("S47 receiver %s: %.1f units past the glass"),What,Over);
  return Check(Found.IsValid()&&Up.IsValid()&&Over<1.f,FString::Printf(TEXT("%s fits the glass without scrolling (%.1f over)"),What,Over));
 };
 auto Fits=[&](const FString& Label,const TCHAR* What)
 {
  TArray<TSharedRef<SButton>> Keys;bool Seen=false;Scan(Label,Keys,Seen);if(!Check(Keys.Num()==1,FString(What)+TEXT(": its key ")+Label))return false;
  TSharedPtr<SWidget> Up=Keys[0];while(Up.IsValid()&&Up->GetTypeAsString()!=TEXT("SScrollBox"))Up=Up->GetParentWidget();
  const float Over=Up.IsValid()?StaticCastSharedPtr<SScrollBox>(Up)->GetScrollOffsetOfEnd():-1.f;
  UE_LOG(LogTemp,Display,TEXT("S47 receiver %s: %.1f units past the glass"),What,Over);
  return Check(Up.IsValid()&&Over<1.f,FString::Printf(TEXT("%s fits the glass without scrolling (%.1f over)"),What,Over));
 };
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
  // 1: ANOMALY DETECTED, on the HUD's line and on the receiver's glass, once the card is tuned and the array steady; RECORD is due
  if(!F.TerminalLines(1).Contains(TEXT("ANOMALY DETECTED")))return;
  const FString Said=F.JobNextAction(*F.CurrentJob());
  if(!Check(F.AnomalyAhead()&&Said.StartsWith(TEXT("RECEIVER"))&&Said.Contains(TEXT("ANOMALY DETECTED"))&&Said.Contains(TEXT("RECORD TAB")),TEXT("the tuned receiver says ANOMALY DETECTED and names its RECORD tab: ")+Said))return;
  M->Player->Place(0,-2.15,0);F.OpenReceiver();Go(10,1.);return;
 }
 if(Stage==10)
 {
  // the receiver opens on TUNE (the fix is held); the due cue points at RECORD, not at TUNE's own key
  if(!Check(F.ReceiverPage()==1&&F.ReceiverDuePage()==3,FString::Printf(TEXT("anomalous take: RECORD is the due tab (open %d, due %d)"),F.ReceiverPage(),F.ReceiverDuePage())))return;
  if(!Check(Shows(TEXT("RECORD TAB: PRESS START RECORDING")),TEXT("TUNE's banner sends the anomaly to the RECORD tab")))return;
  Shot(TEXT("tune-anomaly-detected.png"));
  // RX-LOOK-20261113: first a look at the sky chart before the fix. The job is pointed and a developer run does not wait for the
  // chart, so its search is still on: the sphere while it searches, the same with Calm flashes, closed in on the crossing, and at
  // the lock moment. Then RECORD as before (the fix is then held by hand, as in a real game).
  if(!Click(TEXT("ACQUIRE")))return;Go(101,1.5);return;
 }
 if(Stage==101)
 {
  if(!Check(F.ReceiverPage()==0&&Shows(TEXT("EQUATORIAL CELESTIAL SPHERE"))&&Shows(TEXT("SIGNAL WATERFALL"))&&Shows(TEXT("ACQUISITION / TRACKING"))&&F.SkyFix.IsValid(),TEXT("RX-LOOK: ACQUIRE shows the celestial sphere, the waterfall and the tracking panel in the receiver's frame")))return;
  if(!Fits(TEXT("TUNE >"),TEXT("the sky chart while it searches")))return;
  {
   // RX-LOOK gjennomgang 14 Nov: the step keys take no keyboard focus (a click on one took the arrows from the chart), and the chart has the keys
   bool bKeysFree=true;int32 StepKeys=0;
   for(const TCHAR* StepKey:{TEXT("UP 5°"),TEXT("LEFT 5°"),TEXT("DOWN 5°"),TEXT("RIGHT 5°")}){TArray<TSharedRef<SButton>> Keys;bool KeySeen=false;Scan(StepKey,Keys,KeySeen);StepKeys+=Keys.Num();for(const TSharedRef<SButton>& Key:Keys)bKeysFree&=!Key->SupportsKeyboardFocus();}
   FString HasKeys;FSlateApplication::Get().ForEachUser([&](FSlateUser& User){if(const TSharedPtr<SWidget> W=User.GetFocusedWidget())HasKeys+=W->GetTypeAsString()+TEXT(" ");});
   if(!Check(StepKeys==4&&bKeysFree&&HasKeys.Contains(TEXT("SS47SkyChart")),TEXT("RX-LOOK: the chart's four step keys take no keyboard focus, and the chart has the keys (focus: ")+HasKeys+TEXT(")")))return;
  }
  UE_LOG(LogTemp,Display,TEXT("S47 RX-LOOK: sky chart state %d, reticle %.1f degrees off the crossing"),int32(F.SkyFix->State()),F.SkyFix->Distance());
  Shot(TEXT("acquire-searching.png"));bLookCalmWas=M->Flow->bCalmFlash;M->Flow->bCalmFlash=true;Go(102,.6);return;
 }
 if(Stage==102)
 {
  // RX-LOOK gjennomgang 14 Nov: with Calm flashes nothing of the receiver's picture moves: what moves with time (S47RxLook::MovingPicture)
  // is read now and again on the next stage, a third of a second later, and must be the same
  if(!Check(F.RxLive().bCalm,TEXT("RX-LOOK: the receiver's frame reads the Calm flashes setting")))return;
  CalmPicture=S47RxLook::MovingPicture(F.RxLive(),FPlatformTime::Seconds());CalmAt=FPlatformTime::Seconds();
  Shot(TEXT("acquire-searching-calm.png"));Go(103,.3);return;
 }
 if(Stage==103)
 {
  {
   const double Now=FPlatformTime::Seconds();
   if(!Check(Now-CalmAt>.2&&S47RxLook::MovingPicture(F.RxLive(),Now)==CalmPicture,FString::Printf(TEXT("RX-LOOK: with Calm flashes the receiver's picture is the same %.2f s later (spectrum, I and Q, sweep, stars, slips, jump)"),Now-CalmAt)))return;
  }
  M->Flow->bCalmFlash=bLookCalmWas;
  // the reticle put just inside the capture radius: the receiver eases it onto the crossing, the picture closes in, the fix is taken
  F.SkyFix->Nudge(F.SkyFix->Cross()+FVector2D(4.,2.5)-F.SkyFix->ReticleAt());LookFrom=Age;Go(104,1.2);return;
 }
 if(Stage==104){Shot(TEXT("acquire-closing-in.png"));Go(105);return;}
 if(Stage==105)
 {
  if(!F.SkyFix->Locked()&&Age-LookFrom<15)return;
  UE_LOG(LogTemp,Display,TEXT("S47 RX-LOOK: fix %s %.1f s after the reticle was put by the crossing"),F.SkyFix->Locked()?TEXT("held"):TEXT("NOT held"),Age-LookFrom);
  // RX-LOOK gjennomgang 14 Nov: the picture that follows is the lock moment, so the fix must be held (it was only logged)
  if(!Check(F.SkyFix->Locked(),FString::Printf(TEXT("RX-LOOK: the fix is held %.1f s after the reticle was put by the crossing"),Age-LookFrom)))return;
  Go(106,.35);return;
 }
 if(Stage==106){Shot(TEXT("acquire-lock-moment.png"));Go(107,2.2);return;}
 if(Stage==107)
 {
  // (taken by hand, the page goes over to TUNE a moment after the fix; either way RECORD is next)
  UE_LOG(LogTemp,Display,TEXT("S47 RX-LOOK: after the fix the receiver is on page %d"),F.ReceiverPage());
  // RX-LOOK round 2: first TUNE as it stands before the take (the pictures had it only with the banner's first line and after
  // the take): the zoomed spectrum and the waterfall fill the body now
  if(F.ReceiverPage()!=1){if(!Click(TEXT("TUNE")))return;}
  Go(108,1.);return;
 }
 if(Stage==108)
 {
  if(!Check(F.ReceiverPage()==1&&Shows(TEXT("FINE TUNING (MHz)"))&&Shows(TEXT("SIGNAL WATERFALL")),TEXT("RX-LOOK: TUNE before the take shows the zoomed spectrum and the waterfall beside it")))return;
  if(!Fits(TEXT("START RECORDING"),TEXT("TUNE before the take")))return;
  Shot(TEXT("tune-before-take.png"));
  if(!Click(TEXT("RECORD")))return;Go(11,1.);return;
 }
 if(Stage==11)
 {
  if(!Check(F.ReceiverPage()==3&&F.ReceiverDuePage()==-1&&Shows(TEXT("ANOMALY / RECORD IT, THEN RETAIN IT"))&&!Shows(TEXT("TAKE LIST")),TEXT("RECORD holds the anomaly (its panel in the take list's place) and is no longer due once open")))return;
  if(!Fits(TEXT("START RECORDING"),TEXT("RECORD with the anomaly before the take")))return;
  Shot(TEXT("record-before-take.png"));
  // 2: record it there
  if(!Click(TEXT("START RECORDING")))return;Go(12,.5);return;
 }
 if(Stage==12)
 {
  if(!Check(F.CurrentJob()->Stage==J::Integrating&&F.CurrentJob()->bAnomaly&&F.JobNextAction(*F.CurrentJob()).Contains(TEXT("RETAIN IT ON RECORD")),TEXT("START RECORDING on RECORD takes the unusual take; the hint names RETAIN on the RECORD tab: ")+F.JobNextAction(*F.CurrentJob())))return;
  Go(13,.2);return;
 }
 if(Stage==13)
 {
  if(F.CurrentJob()->Stage==J::Integrating)return;
  // (a beat for the panel: Slate refreshes a key's enabled state on its next frame, not in the tick the take ends)
  Go(131,.5);return;
 }
 if(Stage==131)
 {
  // 3: the take ended waits on RECORD: nothing retained by itself, no chart, no control asked for, the deck and OPS refuse it with the reason
  const auto* Take=F.CurrentJob();
  if(!Check(Take->Stage==J::Recorded&&FS47FreeformSession::RetainWaits(*Take)&&!Take->bPrinted&&Take->OriginalPage.IsEmpty()&&!Take->bControl&&!F.CanRecordControl()&&F.PrinterJobId()!=Take->Id,TEXT("the take ends on tape, waiting to be retained; no chart, nothing kept by itself")))return;
  if(!Check(F.JobNextAction(*Take).Contains(TEXT("RECORD TAB, RETAIN THE TAKE"))&&F.JobLocation(*Take).Contains(TEXT("RECORD tab"))&&F.ReceiverDuePage()==-1&&F.TerminalLines(1).Contains(TEXT("ANOMALY DETECTED")),TEXT("hint and location name RETAIN on the RECORD tab: ")+F.JobNextAction(*Take)))return;
  if(!Check(!F.AudioAction(TEXT("play"))&&F.FeedbackText().Contains(TEXT("RETAIN TAKE"))&&!F.RegisterComparison(TEXT("unresolved"))&&F.FeedbackText().Contains(TEXT("RECORD tab")),TEXT("the reel deck and OPS refuse a take not retained, and say where to retain it")))return;
  if(!Fits(TEXT("RETAIN TAKE / RECORD REFERENCE 8 s"),TEXT("RECORD after the take")))return;
  Shot(TEXT("record-retain-due.png"));
  if(!Click(TEXT("RETAIN TAKE / RECORD REFERENCE 8 s")))return;Go(14,.5);return;
 }
 if(Stage==14)
 {
  // 4: retained there: the original page in the notebook, the 8-second station reference recording; then the reel deck
  const auto* Take=F.CurrentJob();
  if(!Check(Take->bPrinted&&!Take->OriginalPage.IsEmpty()&&Take->bControl&&!FS47FreeformSession::RetainWaits(*Take)&&F.JobNextAction(*Take).StartsWith(TEXT("REEL DECK"))&&F.JobLocation(*Take).StartsWith(TEXT("Reel deck")),TEXT("RETAIN TAKE keeps the original, records its reference and names the reel deck: ")+F.JobNextAction(*Take)))return;
  if(Take->ControlAt<8)return;
  if(!Check(Shows(TEXT("RETAINED / REEL DECK NEXT"))&&F.ReceiverDuePage()==-1,TEXT("RECORD says the take is retained and the reel deck is next; no receiver tab is due")))return;
  if(!FitsText(TEXT("RETAINED / REEL DECK NEXT"),TEXT("RECORD with the take retained")))return; // RX-LOOK gjennomgang 14 Nov
  Shot(TEXT("record-retained.png"));Go(20,.3);return;
 }
 if(Stage==20){if(!Click(TEXT("TUNE")))return;Go(21,1.);return;} // RX-ACQUIRE-SKY-20261113: the recording key and the tape strip are TUNE's too
 if(Stage==21)
 {
  TArray<TSharedRef<SButton>> Keys;bool KeySeen=false;Scan(TEXT("ON TAPE / REEL DECK"),Keys,KeySeen);
  if(!Check(Keys.Num()==1&&!Keys[0]->IsEnabled(),TEXT("TUNE says the take is on tape and the reel deck is next; nothing to press here")))return;
  TArray<TSharedRef<SButton>> Control;bool ControlSeen=false;Scan(TEXT("RECORD CONTROL / 8 s"),Control,ControlSeen);
  if(!Check(Control.Num()==0,TEXT("the receiver offers no second control for the unusual take")))return;
  if(!Fits(TEXT("ON TAPE / REEL DECK"),TEXT("TUNE with the tape strip")))return;
  if(!Check(Shows(TEXT("PLAY TAKE AND REFERENCE")),TEXT("the receiver's banner names the reel deck's steps")))return;
  Shot(TEXT("tune-after-take.png"));Go(22,1.);return;
 }
 // RX-ACQUIRE-SKY-20261113: ACQUIRE is the sky chart; after the take it shows the fix as held, fits the glass, and leads on to TUNE.
 if(Stage==22){if(!Click(TEXT("ACQUIRE")))return;Go(23,1.5);return;}
 if(Stage==23)
 {
  TArray<TSharedRef<SButton>> Onward;bool OnwardSeen=false;Scan(TEXT("TUNE >"),Onward,OnwardSeen);
  // RX-LOOK-20261113: the chart is the celestial sphere, in the receiver's own frame (the state banner, the scopes beside it)
  if(!Check(Shows(TEXT("EQUATORIAL CELESTIAL SPHERE"))&&Shows(TEXT("SIGNAL WATERFALL"))&&Shows(TEXT("ACQUISITION / TRACKING"))&&Onward.Num()==1&&Onward[0]->IsEnabled()&&F.SkyFix.IsValid()&&F.SkyAcquired(),TEXT("ACQUIRE is the sky chart: after the take its fix is held and TUNE is the way on")))return;
  if(!Fits(TEXT("TUNE >"),TEXT("the sky chart")))return;
  Shot(TEXT("acquire-sky-chart.png"));Go(24,1.);return;
 }
 // RX-LOOK round 2: the two tabs the pictures did not cover, in the receiver's frame, for the look. (Here, after everything the
 // receiver's due cue is asked about: opening DATA marks its page as seen.) How far DATA runs past the glass is logged, not judged.
 if(Stage==24){if(!Click(TEXT("ARRAY")))return;Go(25,1.);return;}
 if(Stage==25)
 {
  if(!Check(F.ReceiverPage()==2&&Shows(TEXT("ARRAY / ANTENNAS ON THIS TARGET")),TEXT("RX-LOOK: the ARRAY tab opens in the receiver's frame")))return;
  if(!FitsText(TEXT("ARRAY / ANTENNAS ON THIS TARGET"),TEXT("the ARRAY tab")))return; // RX-LOOK gjennomgang 14 Nov
  Shot(TEXT("array-tab.png"));
  if(!Click(TEXT("DATA")))return;Go(26,1.);return;
 }
 if(Stage==26)
 {
  if(!Check(F.ReceiverPage()==4&&Shows(TEXT("WORK CARD")),TEXT("RX-LOOK: the DATA tab opens in the receiver's frame")))return;
  // RX-LOOK gjennomgang 14 Nov: DATA fits the glass like the other tabs (it was only logged)
  if(!Fits(TEXT("NEXT >"),TEXT("the DATA tab")))return;
  Shot(TEXT("data-tab.png"));Go(70,1.);return;
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
  if(!Check(Shows(TEXT("1 PLAY THE TAKE"))&&Shows(TEXT("2 PLAY THE REFERENCE"))&&Shows(TEXT("3 MARK THE TICKS, RETAIN"))&&Shows(TEXT("KNOWN SOUNDS TO COMPARE BY EAR")),TEXT("panel says the steps: the take, its reference, marks and RETAIN")))return;
  // D45-20261110: one faceplate, no scrolling: its last row stands above the way out
  TArray<TSharedRef<SButton>> Last,Out;bool LastSeen=false,OutSeen=false;Scan(TEXT("NEXT REFERENCE"),Last,LastSeen);Scan(TEXT("ESC / RETURN TO ROOM"),Out,OutSeen);
  if(!Check(Last.Num()==1&&Out.Num()==1,TEXT("D45: the deck has its references row and its way out")))return;
  const FGeometry LastG=Last[0]->GetTickSpaceGeometry(),OutG=Out[0]->GetTickSpaceGeometry();
  const FVector2D LastBottom=LastG.LocalToAbsolute(FVector2D(LastG.GetLocalSize())),OutTop=OutG.LocalToAbsolute(FVector2D(0,0));
  UE_LOG(LogTemp,Display,TEXT("S47 D45 reel deck: last row ends at %.1f, the way out starts at %.1f"),LastBottom.Y,OutTop.Y);
  if(!Check(LastG.GetLocalSize().Y>10&&LastBottom.Y<=OutTop.Y+.5,FString::Printf(TEXT("D45: the whole deck fits its faceplate (last row %.1f, way out %.1f)"),LastBottom.Y,OutTop.Y)))return;
  Shot(TEXT("deck-panel.png"));
  // 5: the take, by the pointer
  if(!Hand(TEXT("PLAY")))return;Go(74,3.);return;
 }
 if(Stage==74)
 {
  if(!Check((F.AudioPlaying()||F.AudioPreparing())&&!F.Held(),TEXT("PLAY by the pointer: the tape runs with the panel open")))return;
  if(!Check(F.AudioPlaying()&&F.AudioPosition()>0,FString::Printf(TEXT("tape position advances, %.1f s"),F.AudioPosition())))return;
  TapeFrom=F.AudioPosition();Shot(TEXT("deck-panel-playing.png"));Go(75);return;
 }
 if(Stage==75)
 {
  // MARK IN by its key as the tape reaches the ticks
  if(F.AudioPosition()<9.f)return;
  if(!Hand(TEXT("MARK IN")))return;Go(76,.25);return;
 }
 if(Stage==76)
 {
  const auto* Take=F.CurrentJob();
  if(!Check(Take->MarkStart>=9.&&Take->MarkStart<=9.3&&Take->MarkEnd<0&&F.FeedbackText().StartsWith(TEXT("MARK IN"))&&Shows(TEXT("MARK OUT still to set"))&&F.AudioPlaying(),FString::Printf(TEXT("MARK IN by its key sets the start at the tape head (%.2f s), says so on the panel, and the tape plays on"),Take->MarkStart)))return;
  Go(760);return;
 }
 if(Stage==760)
 {
  if(F.AudioPosition()<13.f)return;
  if(!Hand(TEXT("MARK OUT")))return;Go(77,.25);return;
 }
 if(Stage==77)
 {
  const auto* Take=F.CurrentJob();
  if(!Check(Take->MarkEnd>=13.&&Take->MarkEnd<=13.6&&Shows(TEXT("COVERS THE TICKS")),FString::Printf(TEXT("MARK OUT by its key sets the end (%.2f s); the deck says the mark covers the ticks"),Take->MarkEnd)))return;
  Go(78);return;
 }
 if(Stage==78)
 {
  if(F.AudioPlaying()||F.AudioPreparing())return;
  const auto* Take=F.CurrentJob();
  // heard through: nothing is confirmed by itself; RETAIN is the operator's
  if(!Check(!Take->bAudioReviewed&&FS47FreeformSession::TapeHeard(*Take)&&!F.HasFinding(TEXT("pulse-review")),TEXT("the take heard through leaves RETAIN to the operator")))return;
  // 6: the reference, on its own key
  if(!Hand(TEXT("REFERENCE 8 s")))return;Go(79,.4);return;
 }
 if(Stage==79)
 {
  if(!Check(F.AudioSource()==1,TEXT("REFERENCE 8 s loads the take's station reference")))return;
  if(!Trace(4.))return;Go(790,.3);return; // a click on the reference's trace: it is not the take
 }
 if(Stage==790)
 {
  if(!Check(F.FeedbackText().Contains(TEXT("reference is loaded"))&&F.CurrentJob()->MarkStart>=9.&&F.CurrentJob()->MarkStart<=9.3&&F.CurrentJob()->MarkEnd>=13.,TEXT("a click on the reference's trace marks nothing and says why")))return;
  if(!Hand(TEXT("PLAY")))return;Go(791,2.);return;
 }
 if(Stage==791)
 {
  if(F.AudioPlaying()||F.AudioPreparing())return;
  if(!Check(F.CurrentJob()->ControlCovered>=8&&F.AudioSource()==1,FString::Printf(TEXT("the reference plays through (%.1f of 8 s heard)"),F.CurrentJob()->ControlCovered)))return;
  if(!Hand(TEXT("ORIGINAL 18 s")))return;Go(80,.4);return;
 }
 // 7: marks by clicks on the trace: two that miss the ticks, refused at RETAIN with the reason; two that cover them, kept
 if(Stage==80){if(!Check(F.AudioSource()==0,TEXT("ORIGINAL 18 s loads the take again")))return;if(!Trace(10.5))return;Go(801,.3);return;}
 if(Stage==801){if(!Check(F.CurrentJob()->MarkEnd<0&&FMath::IsNearlyEqual(F.CurrentJob()->MarkStart,10.5,.1),FString::Printf(TEXT("a click on the trace starts a new mark there (%.2f s)"),F.CurrentJob()->MarkStart)))return;if(!Trace(11.5))return;Go(802,.3);return;}
 if(Stage==802)
 {
  if(!Check(FMath::IsNearlyEqual(F.CurrentJob()->MarkEnd,11.5,.1)&&Shows(TEXT("MISSES TICKS")),TEXT("the second click ends it; the deck says it misses the ticks")))return;
  if(!Hand(TEXT("RETAIN")))return;Go(803,.3);return;
 }
 if(Stage==803)
 {
  if(!Check(!F.CurrentJob()->bAudioReviewed&&F.FeedbackText().Contains(TEXT("miss the extra ticks"))&&Shows(TEXT("miss the extra ticks")),TEXT("RETAIN refuses marks that miss the ticks and says so on the panel")))return;
  TapeFrom=F.AudioPosition();if(!Trace(13.))return;Go(804,.3);return;
 }
 if(Stage==804){if(!Trace(9.))return;Go(805,.3);return;}
 if(Stage==805)
 {
  const auto* Take=F.CurrentJob();
  if(!Check(FMath::IsNearlyEqual(Take->MarkStart,9.,.1)&&FMath::IsNearlyEqual(Take->MarkEnd,13.,.1)&&Shows(TEXT("COVERS THE TICKS"))&&FMath::IsNearlyEqual(F.AudioPosition(),TapeFrom),FString::Printf(TEXT("two clicks right to left mark %.2f to %.2f s, the tape stays put"),Take->MarkStart,Take->MarkEnd)))return;
  Shot(TEXT("deck-panel-marked.png"));
  // 8: RETAIN, the operator's press
  if(!Hand(TEXT("RETAIN")))return;Go(81,.4);return;
 }
 if(Stage==81)
 {
  const auto* Take=F.CurrentJob();
  if(!Check(Take->bAudioReviewed&&F.HasFinding(TEXT("pulse-review"))&&FMath::IsNearlyEqual(Take->MarkStart,9.,.1)&&FMath::IsNearlyEqual(Take->MarkEnd,13.,.1)&&F.FeedbackText().Contains(TEXT("Confirm anomaly")),TEXT("RETAIN confirms the anomaly, keeps the hand marks and names OPS")))return;
  if(!Check(F.JobNextAction(*Take).StartsWith(TEXT("OPS"))&&Shows(TEXT("3 RETAINED: TO OPS")),TEXT("then OPS: ")+F.JobNextAction(*Take)))return;
  Shot(TEXT("deck-panel-retained.png"));Go(82,.3);return;
 }
 if(Stage==82)
 {
  if(!Click(TEXT("ESC / RETURN TO ROOM")))return;
  if(!Check(!F.TapePanelOpen()&&!M->ModalOpen(),TEXT("the visible button leaves the panel and clears its hold exemption")))return;
  if(!Check(F.AudioDesk->Use(TEXT("ff:audio:station"))&&F.TapePanelOpen(),TEXT("the deck opens again")))return;
  Go(83,.3);return;
 }
 if(Stage==83)
 {
  FSlateApplication& App=FSlateApplication::Get();const FKeyEvent Esc(EKeys::Escape,FModifierKeysState(),0,false,0,0);
  App.ProcessKeyDownEvent(Esc);App.ProcessKeyUpEvent(Esc);
  if(!Check(!F.TapePanelOpen()&&!M->ModalOpen()&&!M->bPausedByMenu,TEXT("a real Escape press leaves the panel without opening the pause menu")))return;
  // 9: confirm at OPS, one button
  if(!Check(!F.RegisterComparison(TEXT("sky"))&&F.CurrentJob()->Stage==J::Recorded,TEXT("OPS takes no origin it cannot show")))return;
  if(!Check(F.RegisterComparison(TEXT("unresolved"))&&F.CurrentJob()->Stage==J::Registered,TEXT("Confirm anomaly registers it")))return;
  Go(84);return;
 }
 if(Stage==84)
 {
  // 10: the packet at the printer, then the binder
  if(F.PrinterProgress()<1)return;
  if(!Check(F.CollectChart()&&F.CurrentJob()->Stage==J::Packet&&F.JobNextAction(*F.CurrentJob()).Contains(TEXT("BINDER")),TEXT("the printer gives the packet and the binder is next")))return;
  // LOOP-SLIP-IN-PACKET-20261113: no separate bearing slip waits behind the anomaly's packet (its bearing, where there is one, is the packet's last page)
  if(!Check(!F.ResultSheets().ContainsByPredicate([](const FS47FreeformSession::FResultSheet& Sh){return FS47FreeformSession::IsBearingSlip(Sh);})&&!F.NextAction().Contains(TEXT("BEARING SLIP")),TEXT("no separate bearing slip after the anomaly's packet")))return;
  M->UI->Close(true);
  Finish(true,TEXT("Unusual take: ANOMALY DETECTED, RECORD tab due, recorded and retained there with its 8-second reference, reel deck by the pointer (play, MARK IN / OUT, reference, marks by clicks, RETAIN), confirmed at OPS, packet printed; the binder is next."));return;
 }
}
