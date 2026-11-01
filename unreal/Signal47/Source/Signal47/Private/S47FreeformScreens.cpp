#include "S47Freeform.h"
#include "S47FreeformTerminal.h"
#include "S47FreeformWorld.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47UI.h"
#include "Engine/TextureRenderTarget2D.h"
#include "SS47ScreenView.h"

bool FS47FreeformSession::TerminalOnGlass()const
{const auto View=TerminalView.Pin();return ActiveTerminal>=0&&View.IsValid()&&View->OnGlass()&&M&&M->ModalOpen();}
bool FS47FreeformSession::TerminalWindowRendered()const
{const auto V=TerminalView.Pin();return TerminalOnGlass()&&V->RenderedFrames()>0;}
void FS47FreeformSession::OpenTerminal(int32 Screen,const FString& Title,TSharedRef<SWidget> Body,bool Live)
{
 if(!M||!M->UI||!M->Web||!M->Player||!RequirePower())return;
 auto Content=S47FreeformTerminal::Frame(Title,Body);
 if(const auto Current=TerminalView.Pin();Current.IsValid()&&ActiveTerminal==Screen&&M->ModalOpen())
 {Current->SetScreen(Content,Content);bTelemetry=Live;return;}
 const TWeakObjectPtr<US47Main> Owner=M;
 const auto View=SNew(SS47ScreenView).Screen(Screen).Web(M->Web.Get()).Walker(M->Player.Get()).bUseLeaves(true).OnLeft([Owner,this]
 {if(Owner.IsValid()&&Owner->Freeform.Get()==this&&Owner->UI)Owner->UI->Close();});
 if(!View->OnGlass()){M->UI->Toast(TEXT("The physical workstation display is unavailable."),4);return;}
 View->SetScreen(Content,Content);
 M->UI->Open(View,[Owner,this]
 {
  if(IsEngineExitRequested()||!Owner.IsValid()||Owner->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)||Owner->Freeform.Get()!=this)return;
  TerminalView.Reset();ActiveTerminal=-1;bTelemetry=false;Changed();
 });
 TerminalView=View;ActiveTerminal=Screen;bTelemetry=Live;
}
TArray<FString> FS47FreeformSession::TerminalLines(int32 Screen)const
{
 TArray<FString> L;if(!Active()||!M||!M->Story||!M->Web)return L;
 const FJob* J=CurrentJob();const FS47Profile P=J?EffectiveProfile(*J):FS47Profile();
 if(Screen==0)
 {
  int32 Remote=0;for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishRemoteAvailable(I))++Remote;
  L={TEXT("SARO ARRAY / STATUS"),FString::Printf(TEXT("REMOTE %02d / %02d"),Remote,M->Web->DishCount()),HasPower()?TEXT("FEEDER / SUPPLY NORMAL"):TEXT("SUPPLY LOST / FIELD CHECK"),Field&&Field->HasArrayFault()?TEXT("S-03 / LOCAL SERVICE DUE"):TEXT("S-03 / REMOTE AVAILABLE"),TEXT("SELECT A DISH TO DIAGNOSE"),TEXT("LOCAL TEST BEFORE RELEASE")};
 }
 else if(Screen==1)
 {
  L={TEXT("RX BANK 3 / RECEIVER"),J?J->Id:TEXT("NO JOB SELECTED"),FString::Printf(TEXT("F %.3f MHz"),M->Story->Rx.Frequency),FString::Printf(TEXT("GAIN %.0f / BW %.0f"),M->Story->Rx.Gain,M->Story->Rx.Bandwidth),FString::Printf(TEXT("CARD %.3f / AZ %.1f"),P.F,(P.Az[0]+P.Az[1])*.5),J?FString::Printf(TEXT("REC %.1f / %.1f s"),J->Elapsed,SourceSeconds()):FString(),TEXT("ORIGINALS / PRINTER TRAY")};
 }
 else if(Screen==2)
 {
  L={TEXT("TRACKING / ENCODERS"),J?J->Id:TEXT("NO JOB SELECTED"),FString::Printf(TEXT("TARGET AZ %.1f EL %.1f"),TargetAz(),TargetEl()),FString::Printf(TEXT("ACTUAL AZ %.1f EL %.1f"),M->Web->DishAz(0),M->Web->DishEl(0)),PointReady()?TEXT("ENCODERS / ON TARGET"):TEXT("WAIT / SLEW OR SERVICE"),TEXT("COMMAND SOURCE TO START"),TEXT("OBSERVATIONS NEED SETTLE")};
 }
 else
 {
  L={FString::Printf(TEXT("SARO OPS / DAY %02d"),DayNumber),FString::Printf(TEXT("BUDGET $%d / DEBT $%d"),Cash,Loan),TEXT("DAILY OBSERVING QUEUE")};
  for(const auto& W:Work)if(W.Day==DayNumber&&L.Num()<6)L.Add(W.Id+FString::Printf(TEXT(" / SRC%d %s"),W.Source,W.Stage==EJobStage::Filed?TEXT("FILED"):TEXT("OPEN")));
  L.Add(ServiceGuide==1?TEXT("SERVICE ORDER / IN PROGRESS"):TEXT("SERVICE ORDER / OPTIONAL"));L.Add(TEXT("ORIGINALS / CASE ARCHIVE"));
 }
 return L;
}
