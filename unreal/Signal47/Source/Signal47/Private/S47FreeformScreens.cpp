#include "S47Audio.h"
#include "S47Freeform.h"
#include "S47Aftermath.h"
#include "S47FreeformTerminal.h"
#include "S47FreeformWorld.h"
#include "S47Main.h"
#include "S47Observing.h"
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
 S47FreeformTerminal::SetClock([this]{return TerminalClock();});
 auto Content=S47FreeformTerminal::Frame(Title,Body);
 S47FreeformTerminal::SetClock(nullptr); // the frame keeps its own copy; Story screens show no stale Freeform clock
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
  if(Owner->Audio)Owner->Audio->ReceiverMonitorOff();
  TerminalView.Reset();ActiveTerminal=-1;bTelemetry=false;Changed();
 });
 TerminalView=View;ActiveTerminal=Screen;bTelemetry=Live;
}
FString FS47FreeformSession::TerminalClock()const
{
 // The world's own. For a while after being taken, it stands at the last time the player is sure of (FS47Aftermath).
 const bool bStuck=Aftermath&&Aftermath->PcStuck();
 const double Shown=bStuck?Aftermath->PcShown():WorldClock;
 const int32 ShownDay=bStuck?FMath::Max(1,int32(FMath::FloorToDouble((Shown-8*3600)/86400.))+1):DayNumber;
 return FString::Printf(TEXT("DAY %02d  |  %s"),ShownDay,*FS47FreeformCases::Clock(FMath::Fmod(Shown,86400.)));
}
TArray<FString> FS47FreeformSession::TerminalLines(int32 Screen)const
{
 TArray<FString> L;if(!Active()||!M||!M->Story||!M->Web)return L;
 const FJob* J=CurrentJob();const FS47Profile P=J?EffectiveProfile(*J):FS47Profile();
 if(Screen==0)
 {
  int32 Remote=0;for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishRemoteAvailable(I))++Remote;
  L={TEXT("SARO ARRAY / STATUS"),FString::Printf(TEXT("REMOTE %02d / %02d"),Remote,M->Web->DishCount()),!bPowerFault?TEXT("FEEDER / SUPPLY NORMAL"):HasPower()?TEXT("FEEDER FAULT / ON GENERATOR"):TEXT("SUPPLY LOST / FIELD CHECK"),Field&&Field->HasArrayFault()?Field->ServiceDishId()+TEXT(" / LOCAL SERVICE DUE"):TEXT("ARRAY / REMOTE AVAILABLE"),TEXT("SELECT A DISH TO DIAGNOSE"),TEXT("LOCAL TEST BEFORE RELEASE")};
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
  // To be seen from across the room: it is only ever on the glass when it is wrong.
  if(Aftermath&&Aftermath->PcStuck())L.Insert(FString::Printf(TEXT("SYSTEM CLOCK %s"),*FS47FreeformCases::Clock(FMath::Fmod(Aftermath->PcShown(),86400.))),1);
  for(const auto& W:Work)if(W.Stage!=EJobStage::Filed&&L.Num()<6)L.Add(W.Id+FString::Printf(TEXT(" / SRC%d %s"),W.Source,W.Stage==EJobStage::Filed?TEXT("FILED"):TEXT("OPEN")));
  L.Add(ServiceGuide==1?TEXT("SERVICE ORDER / IN PROGRESS"):TEXT("SERVICE ORDER / OPTIONAL"));L.Add(TEXT("ORIGINALS / CASE ARCHIVE"));
 }
 if(Screen>=0&&Screen<=2)L.Add(FString::Printf(TEXT("ARRAY %d/27 / RATE %.0f%% / %s"),AvailableDishCount(),100*ArrayMeasurementRate(),AvailableDishCount()<(Field&&Field->ArrayWear().bActive?FS47ArrayWear::MinDishes:24)?TEXT("BLOCKED - REPAIR"):AvailableDishCount()<27?(Field&&Field->ArrayWear().DownCount()>0?TEXT("WORN"):TEXT("DEGRADED")):TEXT("FULL")));
 if(const FString NextLine=TerminalNextLine(Objective(),Screen);!NextLine.IsEmpty())L.Insert(NextLine,1);
 return L;
}
// What the room can read from across the floor. Cut from the HUD's own objective, so the glass and the HUD cannot
// disagree. Says nothing rather than something cut in half.
FString FS47FreeformSession::TerminalNextLine(const FString& Text,int32 Screen)
{
 // "NEXT / " and this much is what stays clear of the curved edge of the glass at the screens' text size.
 static constexpr int32 MaxWhere=31;
 FString From=Text;
 if(From.StartsWith(TEXT("FIRST SHIFT "))){const int32 Bar=From.Find(TEXT(" / "));if(Bar!=INDEX_NONE)From=From.RightChop(Bar+3);}
 // The place stands before the first ": " or ". ". "19:20" is neither.
 const int32 Colon=From.Find(TEXT(": ")),Period=From.Find(TEXT(". "));
 const int32 Cut=Colon==INDEX_NONE?Period:Period==INDEX_NONE?Colon:FMath::Min(Colon,Period);
 FString Where=(Cut==INDEX_NONE?From:From.Left(Cut)).ToUpper();
 Where.RemoveFromEnd(TEXT("."));
 if(Where.IsEmpty()||Where.StartsWith(TEXT("PAUSE MENU")))return FString();
 if(Where.StartsWith(TEXT("ARRAY "))&&Where.Contains(TEXT("BLOCKED")))Where=TEXT("ARRAY STATUS (LEFT-HAND PC)");
 else if(Where.StartsWith(TEXT("EVENING OBSERVATION / ")))Where=Where.RightChop(22);
 static const TCHAR* const Own[4]={TEXT("ARRAY STATUS"),TEXT("RECEIVER (CENTRE PC)"),TEXT("TRACKING (RIGHT-HAND PC)"),TEXT("OPS")};
 if(Screen>=0&&Screen<4&&Where.StartsWith(Own[Screen]))return TEXT("NEXT / USE THIS PC");
 while(Where.Len()>MaxWhere)
 {
  const int32 Bar=Where.Find(TEXT(" / "),ESearchCase::CaseSensitive,ESearchDir::FromEnd);
  if(Bar==INDEX_NONE)return FString();
  Where=Where.Left(Bar);
 }
 return Where.IsEmpty()?FString():TEXT("NEXT / ")+Where;
}
// The older-measurement record at the east wing's calibration bench. Only reads what is retained; alters and pays nothing.
FString FS47FreeformSession::MeasurementRecord()const
{
 const auto IsBaseline=[](const FJob& J){return J.Id.Contains(TEXT("-REFERENCE-"));};
 const auto& Runs=FS47Observing::Runs();
 int32 Originals=0,Baselines=0;FString Rows;
 for(int32 I=Work.Num()-1;I>=0;--I)
 {
  const FJob& J=Work[I];if(J.Stage!=EJobStage::Filed||J.bDuplicateRejected)continue;
  if(IsBaseline(J))++Baselines;
  if(++Originals<=3)Rows+=FString::Printf(TEXT("\nDAY %02d / %s / %s%s"),J.Day,*J.Id,Runs.IsValidIndex(J.Source)?Runs[J.Source].Source:TEXT("UNKNOWN SOURCE"),IsBaseline(J)?TEXT(" / BASELINE"):TEXT(""));
 }
 FString Record=FString::Printf(TEXT("OLDER MEASUREMENTS / %d FILED ORIGINALS / %d REFERENCE BASELINES"),Originals,Baselines)+Rows;
 if(Baselines==0)Record+=TEXT("\nNo reference baseline retained yet. OPS / DAILY WORK offers an optional one once the day's work is done.");
 return Record;
}
