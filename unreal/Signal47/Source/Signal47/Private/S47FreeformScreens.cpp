#include "S47Audio.h"
#include "S47Freeform.h"
#include "S47GearFlicker.h"
#include "S47FreeformDirector.h"
#include "S47Aftermath.h"
#include "S47FreeformTerminal.h"
#include "S47FreeformWorld.h"
#include "S47Docs.h"
#include "S47Inter.h"
#include "S47Three.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
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
namespace
{
 // FIX-20261110 (Tom: more feedback and sound on the screens). What the hand hears at a 1986
 // station: the key's own click and the terminal's bell chip answering it, by kind of control
 // (S47FreeformTerminal::SetSound). Quiet: this is heard a thousand times a night.
 void TerminalKey(US47Audio* A,int32 Kind)
 {
  if(!IsValid(A))return;
  if(Kind==4){A->Beep(1900.f,.012f,.025f,0,TEXT("square"));return;}                   // the cursor arrives on a control (critics, 10 Nov: 3.1 kHz for 4 ms was not heard at all)
  if(Kind==3){A->Beep(2300.f,.007f,.028f,0,TEXT("square"));return;}                    // a knob's detent
  A->Play(TEXT("click"),Kind==1?.42f:.30f,FVector::ZeroVector,Kind==2?1.25f:1.5f);
  if(Kind==1){A->Beep(1320.f,.030f,.040f,.01f,TEXT("square"));A->Beep(1760.f,.045f,.040f,.055f,TEXT("square"));} // the action taken
  else if(Kind==2)A->Beep(990.f,.020f,.030f,.01f,TEXT("square"));                      // a menu line or tab
  else A->Beep(1480.f,.016f,.030f,.01f,TEXT("square"));                                // any other key
 }
 // D45-20261110 (Tom: a sound that says you are getting closer): the tracking plot's search tick, Near 0 far off to 1 on the
 // source. Low and slow far off, higher as the pointer closes in (the plot quickens it); as quiet as a key's detent.
 void TerminalProbe(US47Audio* A,float Near)
 {
  if(!IsValid(A))return;
  if(Near>=1.f){A->Beep(1480.f,.022f,.030f,0,TEXT("square"));return;}                 // on the source: a steady fast tick
  A->Beep(FMath::Lerp(420.f,1350.f,Near),.016f,FMath::Lerp(.016f,.028f,Near),0,TEXT("square"));
 }
 // A page written to the glass: the short chatter of characters arriving down the line.
 void TerminalPage(US47Audio* A,const FString& Title)
 {
  if(!IsValid(A))return;
  uint32 H=GetTypeHash(Title);
  for(int32 K=0;K<7;++K){H=H*1664525u+1013904223u;A->Beep(900.f+float((H>>16)%1200),.014f,.030f,.028f*K,TEXT("square"));}
 }
}
float FS47FreeformSession::ScreenInterference()const{return FMath::Max(SkyLevel(),M&&M->Gear?float(FMath::Clamp(M->Gear->Level(),0.,1.)):0.f);}
FS47FreeformSession::FTerminalIdle FS47FreeformSession::TerminalIdle(int32 Screen)const
{
 FTerminalIdle I;if(!Active()||!M||!M->Story||!M->Web)return I;
 const FJob* J=CurrentJob();
 if(Screen==0)
 {
  // per dish: 0 on the line, 1 not answering remotely, 2 moving
  for(int32 D=0;D<M->Web->DishCount();++D)I.Dish.Add(M->Web->DishMoving(D)?2:M->Web->DishRemoteAvailable(D)?0:1);
 }
 else if(Screen==1)
 {
  const double Span=S47Signal::FMax-S47Signal::FMin;
  I.Tuned=float((M->Story->Rx.Frequency-S47Signal::FMin)/Span);I.Card=J?float((EffectiveProfile(*J).F-S47Signal::FMin)/Span):-1.f;
  if(J&&J->Stage<EJobStage::Recorded&&J->Elapsed>0&&SourceSeconds()>0)I.Rec=float(FMath::Clamp(J->Elapsed/SourceSeconds(),0.,1.));
 }
 else if(Screen==2){I.Az=float(M->Web->DishAz(0));I.El=float(M->Web->DishEl(0));I.AimAz=float(TargetAz());I.AimEl=float(TargetEl());I.bReady=PointReady();}
 else
 {
  I.bWaiting=CaseAwaitingChoice()>=0||ServiceGuide==1||(Field&&Field->HasArrayFault());
  if(Director)for(const auto& C:Director->PhotoClaims())I.bWaiting|=!C.bPaid;
 }
 return I;
}
void FS47FreeformSession::OpenTerminal(int32 Screen,const FString& Title,TSharedRef<SWidget> Body,bool Live)
{
 if(!M||!M->UI||!M->Web||!M->Player||!RequirePower())return;
 const TWeakObjectPtr<US47Main> Main=M;
 const bool bSame=TerminalView.IsValid()&&ActiveTerminal==Screen&&M->ModalOpen();
 S47FreeformTerminal::SetSound([Main](int32 Kind){if(Main.IsValid())TerminalKey(Main->Audio,Kind);});
 S47FreeformTerminal::SetInterference([Main]{return Main.IsValid()&&Main->Freeform?Main->Freeform->ScreenInterference():0.f;});
 S47FreeformTerminal::SetProbeSound([Main](float Near){if(Main.IsValid())TerminalProbe(Main->Audio,Near);});
 S47FreeformTerminal::SetNearness([Main](FVector2D AzEl){return Main.IsValid()&&Main->Freeform?Main->Freeform->AimNearness(AzEl):0.;});
 if(!bSame)
 {
  // The set is taken over: the relay behind the glass goes, the coil thumps. D45-20261110: the picture does not open from a
  // line here; the set was already on (PowerOn comes with the supply, FS47FreeformSession::Step).
  if(M->Audio){M->Audio->Play(TEXT("rack_switch"),.34f);M->Audio->Beep(58.f,.42f,.060f,.02f,TEXT("sine"));M->Audio->Beep(116.f,.26f,.026f,.02f,TEXT("sine"));}
 }
 if(Title!=TerminalTitle){TerminalPage(M->Audio,Title);TerminalTitle=Title;}
 S47FreeformTerminal::SetClock([this]{return TerminalClock();});S47FreeformTerminal::SetStation(TerminalStation(Screen));S47FreeformTerminal::SetHumScreen(Screen);
 auto Content=S47FreeformTerminal::Frame(Title,Body);
 S47FreeformTerminal::SetClock(nullptr);S47FreeformTerminal::SetStation(FString());S47FreeformTerminal::SetHumScreen(-1); // the frame keeps its own copy; Story screens show no stale Freeform clock
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
  if(Owner->Audio){Owner->Audio->ReceiverMonitorOff();Owner->Audio->Play(TEXT("click"),.26f,FVector::ZeroVector,.8f);Owner->Audio->Beep(660.f,.05f,.03f,.02f,TEXT("square"));}
  S47FreeformTerminal::SetSound(nullptr);S47FreeformTerminal::SetProbeSound(nullptr);S47FreeformTerminal::SetNearness(nullptr);TerminalTitle.Reset();
  TerminalView.Reset();ActiveTerminal=-1;bTelemetry=false;Changed();
 });
 TerminalView=View;ActiveTerminal=Screen;bTelemetry=Live;
}
FString FS47FreeformSession::TerminalClock()const
{
 // The world's own. For a while after being taken, it stands at the last time the player is sure of (FS47Aftermath).
 const bool bStuck=Aftermath&&Aftermath->PcStuck();
 const double Shown=bStuck?Aftermath->PcShown():WorldClock;
 // CASES-OPS-20261109: as Story's operations system dates its screen (S47Ops, "14-APR-1986 02:16"), the calendar day of the clock shown,
 // which turns at midnight; the shift's day number is on STATION ACCOUNT. Still ends with the clock (S47MenuPointerTest reads it so).
 return TerminalDate(Shown)+TEXT(" ")+FS47FreeformCases::Clock(FMath::Fmod(Shown,86400.));
}
FString FS47FreeformSession::TerminalDate(double AtClock)const
{
 static const TCHAR* const Mon[12]={TEXT("JAN"),TEXT("FEB"),TEXT("MAR"),TEXT("APR"),TEXT("MAY"),TEXT("JUN"),TEXT("JUL"),TEXT("AUG"),TEXT("SEP"),TEXT("OCT"),TEXT("NOV"),TEXT("DEC")};
 const int32 Calendar=FMath::Max(1,int32(FMath::FloorToDouble(FMath::Max(0.,AtClock)/86400.))+1);
 const FDateTime D=DateOfDay(Calendar);
 return FString::Printf(TEXT("%02d-%s-%02d"),D.GetDay(),Mon[FMath::Clamp(D.GetMonth()-1,0,11)],D.GetYear()%100);
}
FString FS47FreeformSession::OpsAccountLine(int32 Budget,int32 Requisition){return FString::Printf(TEXT("BUDGET $%d / REQ $%d"),Budget,Requisition);}
FString FS47FreeformSession::QueueLine(int32 Remaining,int32 Today,int32 Earlier,int32 Rejected)
{
 // CASES-OPS-20261109: one system line; how the queue works is on the SOP card by the terminal (SopCard).
 if(Remaining==0)return Rejected>0?TEXT("QUEUE / NO ACTIONABLE OBSERVATIONS / LEGACY ORIGINALS HELD, UNPAID / NEXT ISSUE 08:00"):TEXT("QUEUE / ALL OBSERVING WORK COMPLETE / NEXT ISSUE 08:00");
 return FString::Printf(TEXT("MORE WORK / %d OPEN TODAY / %d CARRIED / NEXT ISSUE 08:00"),Today,Earlier);
}
FString FS47FreeformSession::TerminalStation(int32 Screen)
{
 // One operations system on every SARO terminal: OPS is terminal 1, then the array's, the receiver's and the tracking PC. The night
 // operator is new and has no name on the system yet; Reyes was the operator before (FS47Predecessor), never this one.
 const int32 Term=Screen==3?1:Screen==0?2:Screen==1?3:Screen==2?4:1;
 return FString::Printf(TEXT("SARO OPS V4.2  TERM %d  OPR NIGHT"),Term);
}
TArray<FString> FS47FreeformSession::TerminalLines(int32 Screen)const
{
 TArray<FString> L;if(!Active()||!M||!M->Story||!M->Web)return L;
 const FJob* J=CurrentJob();const FS47Profile P=J?EffectiveProfile(*J):FS47Profile();
 if(Screen==0)
 {
  int32 Remote=0;for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishRemoteAvailable(I))++Remote;
  L={TEXT("SARO ARRAY / STATUS"),FString::Printf(TEXT("REMOTE %02d / %02d"),Remote,M->Web->DishCount()),!bPowerFault?TEXT("FEEDER / SUPPLY NORMAL"):HasPower()?TEXT("FEEDER FAULT / ON GENERATOR"):TEXT("SUPPLY LOST / FIELD CHECK"),Field&&Field->HasArrayFault()?Field->ServiceDishId()+TEXT(" / LOCAL SERVICE DUE"):TEXT("ARRAY / REMOTE AVAILABLE"),MaintenanceLine(),TEXT("LOCAL TEST BEFORE RELEASE")}; // D45-20261110: the open faults and the next one, from across the room
 }
 else if(Screen==1)
 {
  // D45-20261110: the array's state where the printer tray's static line was, so the line count stays the same
  const FString Array=!J?FString(TEXT("ARRAY / NO JOB")):PointReady()?FString(Settle>=1?TEXT("ARRAY / LOCKED"):TEXT("ARRAY / SETTLING")):J->Stage==EJobStage::New?FString(TEXT("ARRAY / NOT POINTED")):FString(TEXT("ARRAY / SLEWING"));
  L={TEXT("RX BANK 3 / RECEIVER"),J?J->Id:TEXT("NO JOB SELECTED"),FString::Printf(TEXT("F %.3f MHz"),M->Story->Rx.Frequency),FString::Printf(TEXT("GAIN %.0f / BW %.0f"),M->Story->Rx.Gain,M->Story->Rx.Bandwidth),FString::Printf(TEXT("CARD %.3f / AZ %.1f"),P.F,(P.Az[0]+P.Az[1])*.5),J?FString::Printf(TEXT("REC %.1f / %.1f s"),J->Elapsed,SourceSeconds()):FString(),Array};
 }
 else if(Screen==2)
 {
  L={TEXT("TRACKING / ENCODERS"),J?J->Id:TEXT("NO JOB SELECTED"),FString::Printf(TEXT("TARGET AZ %.1f EL %.1f"),TargetAz(),TargetEl()),FString::Printf(TEXT("ACTUAL AZ %.1f EL %.1f"),M->Web->DishAz(0),M->Web->DishEl(0)),PointReady()?TEXT("ENCODERS / ON TARGET"):TEXT("WAIT / SLEW OR SERVICE"),TEXT("COMMAND SOURCE TO START"),TEXT("OBSERVATIONS NEED SETTLE")};
 }
 else
 {
  L={FString::Printf(TEXT("SARO OPS / DAY %02d"),DayNumber),OpsAccountLine(Cash,Loan),TEXT("DAILY OBSERVING QUEUE")};
  // To be seen from across the room: it is only ever on the glass when it is wrong.
  if(Aftermath&&Aftermath->PcStuck())L.Insert(FString::Printf(TEXT("SYSTEM CLOCK %s"),*FS47FreeformCases::Clock(FMath::Fmod(Aftermath->PcShown(),86400.))),1);
  for(const auto& W:Work)if(W.Stage!=EJobStage::Filed&&L.Num()<6)L.Add(W.Id+FString::Printf(TEXT(" / SRC%d %s"),W.Source,W.Stage==EJobStage::Filed?TEXT("FILED"):TEXT("OPEN")));
  L.Add(ServiceGuide==1?TEXT("WORK ORDER OPEN / ARRAY PC"):TEXT("MAINTENANCE / AT ARRAY PC"));L.Add(TEXT("ORIGINALS / RECORDS")); // D45-20261110: work orders are issued and signed there
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
// ---- CASES-OPS-20261109: the procedure card by the OPS terminal ----------------------------------------------------------------
// What the OPS screens used to explain in prose is typed on a card taped to the desk beside the terminal, for the player to read when
// they want it; the screens keep to short system lines. A thin card built at run time (as FS47LeftGear builds its gear), beside the
// terminal's own box, and an interactable of its own: read it (the document) or look at it.
FS47Doc FS47FreeformSession::SopCard()
{
 FS47Doc D;D.Id=TEXT("freeform-ops-sop");D.Title=TEXT("SOP card: the OPS terminal");D.Kind=TEXT("typed");D.Stamp=TEXT("SARO / SOP 4");
 D.Page=TEXT("SARO / STANDING OPERATING PROCEDURE\nOPS TERMINAL (TERM 1)\n\n")
  TEXT("1. 08:00 each morning the system issues three observing jobs for that evening. The third is a choice of two sources.\n")
  TEXT("2. DAILY WORK: select the job. TRACKING (right-hand PC) points the array, RECEIVER (centre PC) tunes and records, the PRINTER gives the original. Register the result here.\n")
  TEXT("3. Finish a recording or a control before you switch jobs. Unfinished jobs keep. They pay in full on their day and the next, then less.\n")
  TEXT("4. EARLIER WORK shows what is done and what is carried over. RECORDS keeps originals, field findings, service and closed cases. Read only.\n")
  TEXT("5. An unusual original is never registered on its own. Record its internal control at the receiver right after the take, compare both tapes at the reel deck (wall table), collect the original chart at the printer, then register the comparison as unresolved (that opens its case on CASE BOARD). Collect the packet at the printer and file it in the records-room binder. Payment follows the filing.\n") // D45-20261110
  TEXT("6. A quiet internal reference does not prove a sky source. Only a directional or independent control rules an explanation out.\n")
  TEXT("7. CASE BOARD. A trial case closes when your tests rule out all but one explanation; the records reply comes at once. An assessment can be filed when you choose; its reply prints on the telex next morning. Every test registered prints a result sheet: collect it at the printer.\n")
  TEXT("8. TELEX (by the corridor door). Replies and Dr. Ward's receipts print there. Tear them off at the machine; they go in the tray here. Send night logs from OVERVIEW.\n")
  TEXT("9. Queue empty: an optional reference check keeps a fresh baseline. No payment.\n")
  TEXT("10. OVERVIEW is the main menu. ESC leaves the terminal.\n\n")
  TEXT("Do not write on this card. - D.");
 D.Transcript=D.Page;return D;
}
void FS47FreeformSession::StepSopCard()
{
 if(!M||!IsValid(M->Web.Get())||!M->Web->IsLoaded()||!IsValid(M->Inter))return;
 if(SopScene.Get()==M->Web.Get()&&SopProp.IsValid())
 {
  // Put back after a Stop (StopSopCard): the Web's own area visibility is left to it.
  if(SopProp->bHiddenInGame){SopProp->SetHiddenInGame(false);SopProp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);}
  return;
 }
 SopProp.Reset();
 const FS47WebItem* Item=M->Web->Item(TEXT("opsTerminal"));const int32 Root=M->Web->Handle(TEXT("room.group"));
 if(!Item||!Item->bBox||Root<0)return; // no terminal box in this scene: nothing to tape the card beside
 auto* Box=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));if(!Box||!Base)return;
 auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web.Get());Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.78f,.74f,.6f));Mat->SetScalarParameterValue(TEXT("Roughness"),.9f);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
 auto* C=NewObject<UStaticMeshComponent>(M->Web.Get(),TEXT("ff:opsSop"));C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(M->Web->GetRootComponent());C->SetStaticMesh(Box);C->SetMaterial(0,Mat);C->SetCastShadow(false);
 C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
 // Flat on the desk at the terminal's side (three's metres: x, y up, z), a card 12 by 17 cm. SEE IN THE GAME: the side may be the
 // wrong one for the desk in this room; the card is placed from the terminal's own box, never from a fixed world position.
 const FVector Mid=(Item->Min+Item->Max)*.5;const FVector At(Mid.X,Item->Min.Y+.003,Item->Min.Z-.13);
 C->SetWorldLocation(S47Three::ToUE(At));C->SetWorldRotation(FRotator(0,8,0));C->SetWorldScale3D(FVector(.12,.17,.004));
 C->RegisterComponent();
 const int32 Node=M->Web->AdoptNode(C,Root,TEXT("ff:opsSop"));if(Node<0){C->DestroyComponent();return;}
 SopProp=C;SopScene=M->Web;
 // By the owner, never the session: the registry outlives a session, and outside Freeform the card says and does nothing.
 const TWeakObjectPtr<US47Main> Owner=M;
 FS47Interactable I;I.Id=TEXT("ff:opsSop");I.Object=M->Web->ObjAt(Node);I.Range=2.4;
 I.Label=[Owner]{return Owner.IsValid()&&Owner->Freeform&&Owner->Freeform->Active()?FString(TEXT("Read the OPS procedure card")):FString();};
 I.Use=[Owner]{if(Owner.IsValid()&&Owner->Freeform&&Owner->Freeform->Active()&&Owner->UI)Owner->UI->Document(FS47FreeformSession::SopCard());};
 I.Examine=[]{return FString(TEXT("A typed card taped to the desk by the terminal. STANDING OPERATING PROCEDURE, initialled D."));};
 M->Inter->Add(I);M->Web->MarkUsable(Node,I.Id);
}
void FS47FreeformSession::StopSopCard()
{
 if(SopProp.IsValid()&&!IsEngineExitRequested()&&!IsGarbageCollecting()){SopProp->SetHiddenInGame(true);SopProp->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
}
