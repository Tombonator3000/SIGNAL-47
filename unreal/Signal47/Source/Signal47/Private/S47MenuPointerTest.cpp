#include "S47MenuPointerTest.h"
#include "S47Main.h"
#include "S47ArrayWear.h"
#include "S47CalicheAccess.h"
#include "S47TownLayer.h"
#include "S47Herd.h"
#include "S47Taken.h"
#include "S47Aftermath.h"
#include "S47FieldRadio.h"
#include "S47IndoorDread.h"
#include "S47RoswellWorld.h"
#include "S47FreeformWorld.h"
#include "S47Story.h"
#include "S47TripoCreature.h"
#include "S47Freeform.h"
#include "S47FreeformAudioDesk.h"
#include "S47Album.h"
#include "S47Flow.h"
#include "S47Night.h"
#include "S47Panels.h"
#include "S47Kit.h"
#include "S47Glimpses.h"
#include "Components/PoseableMeshComponent.h"
#include "UObject/UObjectIterator.h"
#include "S47FarmHouse.h"
#include "S47FarmBuildings.h"
#include "S47FarmWorld.h"
#include "S47MotelProbe.h"
#include "S47RoadShape.h"
#include "S47World.h"
#include "S47Inter.h"
#include "S47FreeformDirector.h"
#include "S47FreeformCases.h"
#include "S47Flashlight.h"
#include "S47FarmSite.h"
#include "S47FarmNight.h"
#include "S47Flora.h"
#include "S47WorldLife.h"
#include "S47Three.h"
#include "S47Web.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "S47FieldCamera.h"
#include "S47RoomDecor.h"
#include "S47Walker.h"
#include "S47UI.h"
#include "S47Json.h"
#include "S47HUD.h"
#include "Framework/Application/SlateUser.h"
#include "Misc/FileHelper.h"
#include "Widgets/Input/SButton.h"
#include "ImageUtils.h"
#include "Misc/Base64.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/Children.h"
#include "Layout/WidgetPath.h"
#include "Misc/CommandLine.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
 TSharedPtr<SWidget> FindText(const TSharedRef<SWidget>& W,const FString& Text)
 {
  if(!W->GetVisibility().IsVisible()) return nullptr;
  if(W->GetType()==FName(TEXT("STextBlock")) && StaticCastSharedRef<STextBlock>(W)->GetText().ToString()==Text)
  {
   const FVector2D Size=W->GetCachedGeometry().GetLocalSize();
   if(Size.X>1 && Size.Y>1) return W;
  }
  if(FChildren* Children=W->GetChildren())
   for(int32 I=0;I<Children->Num();++I) if(auto Found=FindText(Children->GetChildAt(I),Text)) return Found;
  return nullptr;
 }

 bool ClickText(const FString& Text)
 {
  if(!GEngine || !GEngine->GameViewport || !FSlateApplication::IsInitialized()) return false;
  const auto Window=GEngine->GameViewport->GetWindow();
  if(!Window) return false;
  const auto Widget=FindText(Window.ToSharedRef(),Text);
  if(!Widget) return false;
  const FGeometry& G=Widget->GetCachedGeometry();
  const FVector2D At=G.LocalToAbsolute(G.GetLocalSize()*.5f);
  auto& App=FSlateApplication::Get();
  const FWidgetPath Hit=App.LocateWindowUnderMouse(At,App.GetInteractiveTopLevelWindows());
  UE_LOG(LogTemp,Display,TEXT("S47: menu pointer hit '%s': %s"),*Text,Hit.IsValid()?*Hit.GetLastWidget()->GetTypeAsString():TEXT("no widget"));
  const TSet<FKey> None,Left={EKeys::LeftMouseButton};
  App.ProcessMouseMoveEvent(FPointerEvent(0,At,At-FVector2D(1,0),None,EKeys::Invalid,0.f,App.GetModifierKeys()));
  App.ProcessMouseButtonDownEvent(nullptr,FPointerEvent(0,At,At,Left,EKeys::LeftMouseButton,0.f,App.GetModifierKeys()));
  // Cross at least one frame like a physical click. Capture only coordinates and
  // label: keeping the found widget/path alive here could hide lifetime bugs.
  FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([At,Text](float)
  {
   if(!FSlateApplication::IsInitialized()) return false;
   auto& ReleaseApp=FSlateApplication::Get();
   const TSet<FKey> Released;
   ReleaseApp.ProcessMouseButtonUpEvent(FPointerEvent(0,At,At,Released,EKeys::LeftMouseButton,0.f,ReleaseApp.GetModifierKeys()));
   UE_LOG(LogTemp,Display,TEXT("S47: menu pointer clicked '%s' at %.1f %.1f (synthetic Slate pointer, down/up across frames, no direct callback)"),*Text,At.X,At.Y);
   return false;
  }),.12f);
  return true;
 }

 // The case board is on the station's glass, in its own virtual window: found from the focused widget, as the case-field
 // test finds it. Buttons take a simulated click there; the pointer route above needs the viewport's own window.
 TArray<TSharedRef<SWidget>> MysteryRoots(US47Main* M)
 {
  TArray<TSharedRef<SWidget>> Out;TSet<const SWidget*> Seen;
  if(FSlateApplication::IsInitialized())FSlateApplication::Get().ForEachUser([&](FSlateUser& U)
  {auto W=U.GetFocusedWidget();if(!W)return;while(auto Parent=W->GetParentWidget())W=Parent;if(!Seen.Contains(W.Get())){Seen.Add(W.Get());Out.Add(W.ToSharedRef());}},true);
  if(Out.IsEmpty()&&M&&M->Hud)if(auto Modal=M->Hud->ModalWidget())Out.Add(Modal.ToSharedRef());
  return Out;
 }
 void MysteryTexts(const TSharedRef<SWidget>& W,TArray<FString>& Out)
 {
  if(!W->GetVisibility().IsVisible())return;
  if(W->GetType()==FName(TEXT("STextBlock")))Out.Add(StaticCastSharedRef<STextBlock>(W)->GetText().ToString());
  if(FChildren* Children=W->GetChildren())for(int32 I=0;I<Children->Num();++I)MysteryTexts(Children->GetChildAt(I),Out);
 }
 void MysteryButtons(const TSharedRef<SWidget>& W,const FString& Label,TArray<TSharedRef<SButton>>& Out)
 {
  if(!W->GetVisibility().IsVisible())return;
  if(W->GetType()==FName(TEXT("SButton")))
  {
   TArray<FString> Words;MysteryTexts(W,Words);
   if(Words.Contains(Label))Out.AddUnique(StaticCastSharedRef<SButton>(W));
  }
  if(FChildren* Children=W->GetChildren())for(int32 I=0;I<Children->Num();++I)MysteryButtons(Children->GetChildAt(I),Label,Out);
 }
 bool MysteryClick(US47Main* M,const FString& Label)
 {
  TArray<TSharedRef<SButton>> Buttons;for(const auto& Root:MysteryRoots(M))MysteryButtons(Root,Label,Buttons);
  if(Buttons.Num()!=1||!Buttons[0]->IsEnabled())return false;
#if !UE_BUILD_SHIPPING
  Buttons[0]->SimulateClick();return true;
#else
  return false;
#endif
 }

 struct FRun
 {
  TWeakObjectPtr<US47Main> Main;
  int32 Stage=0;
  double Elapsed=0,StageAt=0;
  bool bLethal=false,bQuit=false,bFullCases=false,bAlbum=false;
  int32 LeftBefore=0,SkyBefore=0,TownAt=0;
  double DreadFrom=0,DreadAt=0,DreadFirst=0,DreadClock=0,DreadLevels[3]={0,0,0};int32 DreadRound=0,DreadSample=0;FString DreadPlace;FVector DreadPos=FVector::ZeroVector;
  FString TownPlace;
  double DoorHeld=0,DoorFirst=0;int32 DoorStep=0;FVector DoorBodyFrom=FVector::ZeroVector;
  double AfterAt=0,AfterPc=0;FString AfterLine;
  S47J::FObj MysteryBefore;FString MysteryBoard;int32 MysteryTruth=0,MysterySub=0,MysteryFuses=0,MysteryCash=0;double MysteryHeld=0;
  double VisitClock=0,VisitFrom=0;double ChainFrom=0,ChainAt=0,ChainAge=0;bool ChainPaused=false;FString ChainEvent;double CowFrom=0;FVector CowA=FVector::ZeroVector,CowB=FVector::ZeroVector;int32 CowCalls=0;FString TakenPlace;bool TakenRoom=false,TakenEscape=false;int32 TakenKind=0;FVector TakenPos=FVector::ZeroVector;double TakenAt=0;bool TakenShot[5]={false,false,false,false,false};FVector FieldFrom=FVector::ZeroVector;int32 FieldShot=0;bool BlockTest=false;bool ShotPending=false;TArray<FVector> WalkTo;int32 WalkAt=0;double WalkFrom=0;
  // From the yard, at the farm: the lights' clear line of sight, measured on 6 November.
  void FaceFarm(US47Main* M)
  {
   const FVector Target(84,20,492);const FVector2D At(0,30);const FVector D=Target-FVector(At.X,1.7,At.Y);
   M->Player->Place(At.X,At.Y,FMath::Atan2(-D.X,-D.Z));M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
  }
  // Takes a real photograph and counts bright pixels in the middle of it, where the farm is.
  int32 Exposed(US47Main* M)
  {
   M->FieldCamera->Raise(true);M->FieldCamera->Sync();const auto Photo=M->Night->Album->Expose();M->FieldCamera->Raise(false);M->UI->Viewfinder(false);
   FString Base64;if(!Photo.Url.Split(TEXT(","),nullptr,&Base64))return -1;
   TArray<uint8> Bytes;if(!FBase64::Decode(Base64,Bytes))return -1;
   auto& Images=FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
   const EImageFormat Format=Images.DetectImageFormat(Bytes.GetData(),Bytes.Num());if(Format==EImageFormat::Invalid)return -1;
   const auto Image=Images.CreateImageWrapper(Format);if(!Image.IsValid()||!Image->SetCompressed(Bytes.GetData(),Bytes.Num()))return -1;
   TArray<uint8> Raw;if(!Image->GetRaw(ERGBFormat::RGBA,8,Raw))return -1;
   const int32 W=Image->GetWidth(),H=Image->GetHeight();int32 Count=0;
   for(int32 Y=H*2/5;Y<H*3/5;++Y)for(int32 X=W*3/10;X<W*7/10;++X){const uint8* P=&Raw[(Y*W+X)*4];if(P[0]>=170&&P[1]>=170&&P[2]>=150)++Count;}
   return Count;
  }
  void Shot(const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("AlbumTest")/Name,true,false);}
  void Press(const FKey& Key)
  {
   auto& App=FSlateApplication::Get();
   const FKeyEvent E(Key,App.GetModifierKeys(),0,false,0,0);
   App.ProcessKeyDownEvent(E);App.ProcessKeyUpEvent(E);
  }
  bool Sees(const TCHAR* Text) const
  {
   const auto Window=GEngine&&GEngine->GameViewport?GEngine->GameViewport->GetWindow():nullptr;
   return Window&&FindText(Window.ToSharedRef(),Text).IsValid();
  }
  // Two prints: an ordinary one, and one the game saw something in, which has to stay.
  void GiveAlbum(US47Main* M)
  {
   TArray<FColor> Pixels;Pixels.SetNum(320*240);
   for(int32 I=0;I<Pixels.Num();++I)Pixels[I]=FColor(uint8(40+(I%320)/2),uint8(60+(I/320)/2),90);
   TArray64<uint8> Png;FImageUtils::PNGCompressImageArray(320,240,Pixels,Png);
   const FString Url=TEXT("data:image/png;base64,")+FBase64::Encode(Png.GetData(),int32(Png.Num()));
   TArray<TSharedPtr<FJsonValue>> Shots;
   for(int32 I=0;I<2;++I)
   {
    const auto O=S47J::New();
    S47J::Set(O,TEXT("id"),FString::Printf(TEXT("albumtest%d@10800"),I));S47J::Set(O,TEXT("url"),Url);S47J::Set(O,TEXT("clock"),10800.);
    S47J::Set(O,TEXT("place"),TEXT("Service yard"));S47J::Set(O,TEXT("text"),I?TEXT("The print the game saw something in."):TEXT("An ordinary print."));S47J::Set(O,TEXT("labStage"),3);
    if(I){const auto T=S47J::New();S47J::Set(T,TEXT("anomaly:albumtest"),TArray<double>{.5,.5});S47J::Set(O,TEXT("targets"),T);}
    Shots.Add(MakeShared<FJsonValueObject>(O));
   }
   const auto V=S47J::New();V->SetArrayField(TEXT("shots"),Shots);
   M->Night->Album->Restore(V);
   LeftBefore=M->Night->Album->Left();
  }
  bool Fail(const TCHAR* Why)
  {
   UE_LOG(LogTemp,Error,TEXT("S47: menu pointer FAIL: %s (stage %d)"),Why,Stage);
   FPlatformMisc::RequestExitWithStatus(false,1);
   return false;
  }
  void DreadLookAt(US47Main* M,const FVector& Target)
  {
   const FVector Eye=M->Night->GlimpseIn(true).Cam.Pos,D=Target-Eye;
   M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
  }
  bool DreadToClock(US47Main* M,double Hours,double ExtraDays=0)
  {
   const double Today=FMath::Fmod(M->Freeform->Clock(),86400.);
   return M->Freeform->TestAdvance(ExtraDays*86400.+FMath::Fmod(Hours*3600.-Today+86400.,86400.));
  }
  // -S47MenuPointerMystery: one night's mystery from the planner's own draw, with the cause as the seed makes it (so it
  // differs from run to run; the selftests go through every cause). Set up: night two at 19:30 in the control room; the
  // mystery's own dice set to yes (its hour is still the planner's); every other draw set to no and any other event
  // cancelled as it starts; scheduler clock 40 times over, world clock 560 times over until the notice comes and real time
  // after; the player moved to the feeder and to OPS; the clock skipped to midnight and to morning; the control's receiver
  // settings set as the other tests set them. Natural: the draw, its hour, the start at the station, the record, the case
  // opening, the job, its recording and result, the filing by the board's own buttons, the reply and its effect.
  // PASS only if: no body before two; the notice waits for its hour; a menu holds the notice and the morning's reply; a
  // disk Continue in the notice neither loses nor repeats it; nothing is at the feeder before midnight and something is
  // after; the two places read differently; what the player can read on the board, the plan and the tasks is word for word
  // the same under another cause; a real control closes what it should and the board stops offering it; filed right, the
  // morning brings what the table says for it, once, also after a disk Continue; back from before the control, filed wrong,
  // it is accepted, survives a Continue before the reply, is answered next morning with no gain, and Rest still goes on.
  void MysteryBoardOpen(US47Main* M)
  {
   M->UI->Close(true);const FVector P=S47Three::ToThree(M->Web->PropLocation(TEXT("opsTerminal")));
   M->Player->Place(P.X,P.Z+1.1,0);M->Freeform->Use(TEXT("opsTerminal"));M->Freeform->OpenCaseBoard(FS47FreeformCases::MysteryLine);
  }
  // What the player can read about it, less the running clock.
  FString MysteryShown(US47Main* M) const
  {
   TArray<FString> Out;for(const auto& Root:MysteryRoots(M))MysteryTexts(Root,Out);
   Out.RemoveAll([](const FString& Word){return Word.Contains(TEXT("  |  "));});
   Out.Add(M->Freeform->TonightPlan());
   for(const auto& Task:M->Freeform->Tasks())if(Task.Text.StartsWith(TEXT("CASE /")))Out.Add(Task.Text);
   Out.Sort();return FString::Join(Out,TEXT("\n"));
  }
  // The ordinary way back: the whole state to disk and in again through Flow. The test's own settings are not saved.
  bool MysteryRestore(US47Main* M,const S47J::FObj& State)
  {
   const FString File=FPaths::ProjectSavedDir()/TEXT("MysteryTest")/TEXT("state.json");FString Disk;
   if(!FFileHelper::SaveStringToFile(S47J::Write(State),*File)||!FFileHelper::LoadFileToString(Disk,*File))return Fail(TEXT("the state must reach the disk and come back"));
   US47Flow::FStart Start;Start.bDev=true;Start.State=S47J::Read(Disk);M->Flow->StartGame(Start);
   if(!M->IsFreeform()||!M->Freeform||!M->Freeform->Active()||!M->Freeform->Director)return Fail(TEXT("Continue must start Freeform"));
   M->Freeform->Director->TestSchedule(0.f,-1,40.);M->Freeform->Director->TestMysteryDice(1.f);
   DreadAt=Elapsed;return true;
  }
  // Next morning's reply against the table, for what was filed and what it was.
  bool MysteryReplyHolds(US47Main* M)
  {
   auto& F=*M->Freeform;const auto* C=F.CaseFile().Get(FS47FreeformCases::MysteryLine);const auto* Reply=F.CaseFile().Reply(FS47FreeformCases::MysteryLine);
   if(!C||!Reply||!C->bReplied||F.FindingText(TEXT("case4-reply"))!=FString(Reply->Text))return Fail(TEXT("the reply must be the table's, and on file"));
   const int32 Fuses=F.InventoryCount(TEXT("fuse"));const int32 Cash=F.Budget();
   using ER=FS47FreeformCases::EReply;
   if(Fuses!=MysteryFuses+(Reply->Effect==ER::Spare?1:0))return Fail(TEXT("a spare comes only with the table's say"));
   if(Reply->Effect==ER::Spare&&C->Conclusion!=C->Truth)return Fail(TEXT("a wrong filing must not pay"));
   if(Cash!=MysteryCash-(Reply->Effect==ER::CallOut?FMath::Min(MysteryCash,int32(FS47FreeformCases::CallOutCharge)):0))return Fail(TEXT("the call-out is what it says, and no debt"));
   if(F.Debt()!=0)return Fail(TEXT("no debt from a reply"));
   if(F.PowerFault()!=(Reply->Effect==ER::FeederFault))return Fail(TEXT("the feeder fails only with the table's say"));
   if(Reply->Effect==ER::FeederFault&&!F.Objective().Contains(TEXT("FEEDER")))return Fail(TEXT("a feeder fault must say where to go"));
   UE_LOG(LogTemp,Display,TEXT("S47: mystery: filed %d, it was %d, effect %d, fuses %d, cash %d"),C->Conclusion,C->Truth,int32(Reply->Effect),Fuses,Cash);
   return true;
  }
  bool StepMystery(US47Main* M)
  {
   StageAt=Elapsed;
   auto& F=*M->Freeform;auto& D=*F.Director;
   const int32 L=FS47FreeformCases::MysteryLine;const auto& Def=FS47FreeformCases::Def(L);const auto* C=F.CaseFile().Get(L);
   if(!Def.Mystery||!C)return Fail(TEXT("the mystery's line must be there"));
   const double Hour=FMath::Fmod(F.Clock(),86400.)/3600.,Late=Hour<12?Hour+24:Hour;
   if(!FS47FarmSite::TheirHours(F.Clock())&&(!M->Night->Glimpses->Individuals().IsEmpty()||F.IndoorDread->PeekActive()))return Fail(TEXT("no body before two in the morning"));
   // Nothing else crowds it. An event stopped this soon has not touched anything, but a fault must not hide the reply's.
   if(!D.ActiveEvent().IsEmpty())
   {
    UE_LOG(LogTemp,Display,TEXT("S47: mystery: %s started at %.2f, stopped"),*D.ActiveEvent(),Hour);
    if(!D.CancelForTest())return Fail(TEXT("event would not stop"));
   }
   if(!C->bReplied&&F.PowerFault())F.SetPowerFault(false);
   const FString SaroClue=Def.Mystery->Clues[0].Finding,FeederClue=Def.Mystery->Clues[1].Finding;
   if(Stage==490)
   {
    if(D.MysteryNoticeClue()<0)
    {
     if(Late>=29)return Fail(TEXT("the drawn mystery must come by itself to a player at the station"));
     if(D.MysteryDrawnNight()>0&&MysterySub==0){UE_LOG(LogTemp,Display,TEXT("S47: mystery: the planner drew it at %.2f"),Hour);MysterySub=1;}
     if(!F.TestAdvance(FMath::Min(Elapsed-DreadAt,.25)*560.))return Fail(TEXT("no clock"));
     DreadAt=Elapsed;return true;
    }
    if(MysterySub==0)return Fail(TEXT("the notice must follow the planner's draw"));
    if(D.MysteryNoticeClue()!=0||Late<20||C->bOpen||F.HasFinding(SaroClue))return Fail(TEXT("the first notice at the station, after its hour, before any record"));
    UE_LOG(LogTemp,Display,TEXT("S47: mystery: the notice came at %.2f"),Hour);
    M->bPausedByMenu=true;MysteryHeld=D.MysteryNoticeAge();VisitClock=F.Clock();DreadAt=Elapsed;Stage=491;return true;
   }
   if(Stage==491)
   {
    if(Elapsed-DreadAt<2)return true;
    if(D.MysteryNoticeAge()!=MysteryHeld||F.Clock()!=VisitClock||F.HasFinding(SaroClue))return Fail(TEXT("a menu holds the notice and the clock"));
    M->bPausedByMenu=false;
    const auto State=M->Flow->GameState();
    if(!MysteryRestore(M,State))return false;
    Stage=492;return true;
   }
   if(Stage==492)
   {
    if(F.HasFinding(SaroClue))
    {
     if(!C->bOpen||F.FindingText(SaroClue)!=F.CaseFile().ClueText(L,0)||!D.MysteryTrailOpen()||D.MysteryDrawnNight()!=0)return Fail(TEXT("the record opens the case, and leaves the other place worth a look"));
     MysteryTruth=C->Truth;M->World->Enter(TEXT("road"));DreadAt=Elapsed;MysterySub=0;Stage=493;return true;
    }
    if(Elapsed-DreadAt>.5&&D.MysteryNoticeClue()!=0)return Fail(TEXT("a Continue must keep the notice under way"));
    if(Elapsed-DreadAt>15)return Fail(TEXT("the notice must end in a record"));
    return true;
   }
   if(Stage==493)
   {
    if(MysterySub==0)
    {
     if(M->Web->Area()!=TEXT("road")||Elapsed-DreadAt<1)return Elapsed-DreadAt>15?Fail(TEXT("the road must load")):true;
     if(M->World->bDriving)return Fail(TEXT("the test walks up to the feeder"));
     const FVector P=F.Field->InteractionPoint(TEXT("ff:feeder"));M->Player->Place(P.X,P.Z+4.,0);DreadAt=Elapsed;MysterySub=1;return true;
    }
    if(MysterySub==1)
    {
     // In the evening there is nothing here to find, however long.
     if(Hour>=12)
     {
      if(D.MysteryNoticeClue()>=0||F.HasFinding(FeederClue))return Fail(TEXT("nothing at the feeder before midnight"));
      if(Elapsed-DreadAt<8)return true;
      UE_LOG(LogTemp,Display,TEXT("S47: mystery: 8 s at the feeder at %.2f, nothing"),Hour);
      if(!DreadToClock(M,.5))return Fail(TEXT("no clock"));
     }
     DreadAt=Elapsed;MysterySub=2;return true;
    }
    if(!F.HasFinding(FeederClue))return Elapsed-DreadAt>30?Fail(TEXT("after midnight the feeder has something to find")):true;
    const int32 A=F.CaseFile().ClueReading(L,0),B=F.CaseFile().ClueReading(L,1);
    if(A==B||(A!=MysteryTruth&&B!=MysteryTruth)||F.FindingText(FeederClue)!=F.CaseFile().ClueText(L,1))return Fail(TEXT("the two places must read differently, one of them straight"));
    M->World->Enter(TEXT("saro"));DreadAt=Elapsed;MysterySub=0;Stage=494;return true;
   }
   if(Stage==494)
   {
    if(Elapsed-DreadAt<1)return true;
    if(M->Web->Area()!=TEXT("saro"))return Elapsed-DreadAt>15?Fail(TEXT("SARO must load")):true;
    // Word for word the same under another cause. Both from a Continue, so that neither carries the last message.
    if(MysterySub==0)
    {
     M->UI->Close(true);M->Player->Place(0,-1.2,0);MysteryBefore=M->Flow->GameState();
     if(!MysteryRestore(M,MysteryBefore))return false;
     MysterySub=1;return true;
    }
    if(MysterySub==1){MysteryBoardOpen(M);DreadAt=Elapsed;MysterySub=2;return true;}
    if(MysterySub==2)
    {
     MysteryBoard=MysteryShown(M);
     if(!MysteryBoard.Contains(F.FindingText(SaroClue))||!MysteryBoard.Contains(F.FindingText(FeederClue)))return Fail(TEXT("the board shows both traces"));
     for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)if(!MysteryBoard.Contains(FString(TEXT("File assessment: "))+Def.Short[I]))return Fail(TEXT("with no test run, any explanation can be filed"));
     int32 Other=1;for(FS47FreeformCases Probe;Other<512;++Other){Probe.Reset(Other);if(Probe.Get(L)->Truth!=MysteryTruth)break;}
     const auto Swapped=S47J::Clone(MysteryBefore);S47J::Set(S47J::Obj(Swapped,TEXT("freeform")),TEXT("jobSeed"),Other);
     M->UI->Close(true);if(!MysteryRestore(M,Swapped))return false;
     MysterySub=3;return true;
    }
    if(MysterySub==3)
    {
     if(C->Truth==MysteryTruth)return Fail(TEXT("the other seed must mean another cause"));
     MysteryBoardOpen(M);DreadAt=Elapsed;MysterySub=4;return true;
    }
    if(MysterySub==4)
    {
     const FString Shown=MysteryShown(M);
     if(Shown!=MysteryBoard){UE_LOG(LogTemp,Error,TEXT("S47: mystery: one cause reads\n%s\nand another\n%s"),*MysteryBoard,*Shown);return Fail(TEXT("nothing the player can read may depend on the cause"));}
     M->UI->Close(true);if(!MysteryRestore(M,MysteryBefore))return false;
     MysterySub=5;return true;
    }
    if(C->Truth!=MysteryTruth)return Fail(TEXT("Continue must bring back the same cause"));
    MysteryBoardOpen(M);DreadAt=Elapsed;MysterySub=0;Stage=495;return true;
   }
   if(Stage==495)
   {
    // A real control, on the real job. Off the beam needs no fuel and no second night.
    if(Elapsed-DreadAt<.6)return true;
    if(MysterySub==0)
    {
     if(!MysteryClick(M,FString(TEXT("Plan this test: "))+Def.Tests[1].Name))return Fail(TEXT("the board offers the control"));
     DreadAt=Elapsed;MysterySub=1;return true;
    }
    if(MysterySub==1)
    {
     if(C->Planned!=1)return Fail(TEXT("the control is planned"));
     M->UI->Close(true);
     if(!F.SelectJob(F.Jobs().Num()-1)||F.CurrentJob()->CaseLine!=L)return Fail(TEXT("the control is its own job"));
     const auto* Card=F.ConsoleProfile();if(!Card)return Fail(TEXT("the job has a card"));
     auto& Rx=M->Story->Rx;Rx.Frequency=Card->F;Rx.Gain=(Card->Gain[0]+Card->Gain[1])/2;Rx.Bandwidth=(Card->Bw[0]+Card->Bw[1])/2;Rx.Azimuth=(Card->Az[0]+Card->Az[1])/2;
     if(!F.PointSelected())return Fail(TEXT("the array takes the command"));
     DreadAt=Elapsed;MysterySub=2;return true;
    }
    if(MysterySub==2)
    {
     if(!F.StartAcquisition())return Elapsed-DreadAt>40?Fail(TEXT("the recording must start once the dishes settle")):true;
     DreadAt=Elapsed;MysterySub=3;return true;
    }
    if(MysterySub==3)
    {
     // At twelve of twenty-seven antennas the eighteen seconds of the pulsar take 42 s at 43 percent, the chart 5.2 s more,
     // and a frame slower than a tenth of a second counts as a tenth. The array is the worn one here, on purpose.
     if(!F.CurrentJob()||F.CurrentJob()->Stage!=FS47FreeformSession::EJobStage::Chart||F.PrinterProgress()<1)return Elapsed-DreadAt>30.+3.*18./FMath::Max(.05,F.ArrayMeasurementRate())?Fail(TEXT("the recording must print")):true;
     M->UI->Close(true);if(!F.CollectChart())return Fail(TEXT("the chart must come off the printer"));
     M->UI->Close(true);if(!F.RegisterComparison(TEXT("normal")))return Fail(TEXT("the control must register"));
     const auto& Result=C->Results[1];
     if(!Result.bDone||Result.bPresent!=FS47FreeformCases::PresentFor(L,MysteryTruth,1))return Fail(TEXT("the control shows what the cause makes it show"));
     if(F.CaseFile().Remaining(L)>=FS47FreeformCases::CauseCount)return Fail(TEXT("the control must close something"));
     MysteryBoardOpen(M);DreadAt=Elapsed;MysterySub=4;return true;
    }
    // Filed right. What the control ruled out is no longer offered, on the board or behind it.
    const FString Shown=MysteryShown(M);
    for(int32 I=0;I<FS47FreeformCases::CauseCount;++I)
    {
     const bool bOffered=Shown.Contains(FString(TEXT("File assessment: "))+Def.Short[I]);
     if(bOffered==F.CaseFile().Excluded(L,I))return Fail(TEXT("the board offers exactly what the tests leave"));
     if(F.CaseFile().Excluded(L,I)&&F.ConcludeCase(L,I))return Fail(TEXT("a ruled-out explanation cannot be filed"));
    }
    MysteryFuses=F.InventoryCount(TEXT("fuse"));MysteryCash=F.Budget();
    if(!MysteryClick(M,FString(TEXT("File assessment: "))+Def.Short[MysteryTruth]))return Fail(TEXT("the board takes the filing"));
    DreadAt=Elapsed;MysterySub=0;Stage=496;return true;
   }
   if(Stage==496)
   {
    if(Elapsed-DreadAt<.6)return true;
    if(MysterySub==0)
    {
     const auto* Reply=F.CaseFile().Reply(L);
     if(!C->bConcluded||C->Conclusion!=MysteryTruth||!Reply||C->bReplied)return Fail(TEXT("the filing stands, with its reply still to come"));
     const FString Shown=MysteryShown(M)+F.FeedbackText()+F.FindingText(TEXT("case4-conclusion"));
     if(Shown.Contains(Reply->Text)||F.HasFinding(TEXT("case4-reply")))return Fail(TEXT("nothing is known of it before the morning"));
     // The menu holds the morning too.
     M->UI->Close(true);M->bPausedByMenu=true;if(!DreadToClock(M,8.5))return Fail(TEXT("no clock"));
     DreadAt=Elapsed;MysterySub=1;return true;
    }
    if(MysterySub==1)
    {
     if(Elapsed-DreadAt<2)return true;
     if(C->bReplied||F.Day()<=C->ConcludeDay)return Fail(TEXT("a menu holds the reply, and the morning must be a new day"));
     M->bPausedByMenu=false;DreadAt=Elapsed;MysterySub=2;return true;
    }
    if(MysterySub==2)
    {
     if(!C->bReplied)return Elapsed-DreadAt>5?Fail(TEXT("the morning brings the reply")):true;
     if(!MysteryReplyHolds(M))return false;
     MysteryFuses=F.InventoryCount(TEXT("fuse"));MysteryCash=F.Budget();
     const auto State=M->Flow->GameState();if(!MysteryRestore(M,State))return false;
     MysterySub=3;return true;
    }
    if(!C->bReplied||!C->bConcluded||F.InventoryCount(TEXT("fuse"))!=MysteryFuses||F.Budget()!=MysteryCash)return Fail(TEXT("after a Continue the reply stands and does not come again"));
    // Back from before the control: this time the wrong answer, with nothing but the traces.
    if(!MysteryRestore(M,MysteryBefore))return false;
    MysterySub=0;Stage=497;return true;
   }
   if(Stage==497)
   {
    if(Elapsed-DreadAt<1)return true;
    if(MysterySub==0){MysteryBoardOpen(M);DreadAt=Elapsed;MysterySub=1;return true;}
    if(MysterySub==1)
    {
     MysteryFuses=F.InventoryCount(TEXT("fuse"));MysteryCash=F.Budget();
     const int32 Wrong=(MysteryTruth+1)%FS47FreeformCases::CauseCount;
     if(C->Truth!=MysteryTruth||C->bConcluded||!MysteryClick(M,FString(TEXT("File assessment: "))+Def.Short[Wrong]))return Fail(TEXT("the wrong answer can be filed with no test run"));
     DreadAt=Elapsed;MysterySub=2;return true;
    }
    if(MysterySub==2)
    {
     if(!C->bConcluded||C->Conclusion==MysteryTruth||!F.FeedbackText().Contains(TEXT("in the morning")))return Fail(TEXT("the wrong filing stands, and it says only that the reply will come"));
     M->UI->Close(true);const auto State=M->Flow->GameState();if(!MysteryRestore(M,State))return false;
     MysterySub=3;return true;
    }
    if(MysterySub==3)
    {
     if(!C->bConcluded||C->bReplied||C->Conclusion==MysteryTruth)return Fail(TEXT("a Continue keeps the filing and its reply still to come"));
     if(!DreadToClock(M,8.5))return Fail(TEXT("no clock"));
     DreadAt=Elapsed;MysterySub=4;return true;
    }
    if(!C->bReplied)return Elapsed-DreadAt>5?Fail(TEXT("the morning brings the reply")):true;
    if(!MysteryReplyHolds(M))return false;
    // Nothing locks: the next day comes as any other.
    const int32 Today=F.Day();
    if(!F.Rest()||F.Day()!=Today+1)return Fail(TEXT("after a wrong filing the days go on"));
    UE_LOG(LogTemp,Display,TEXT("S47: mystery PASS: both ways, under cause %d"),MysteryTruth);
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   return Fail(TEXT("no such mystery stage"));
  }
  // -S47MenuPointerKinds: from the visit's own dice (a set roll, through PickWho) at the marks. PASS only if: nothing at
  // all in 25 s of standing there at 21:00; at 03:30 each kind comes on its own after the wait; the tall one stands 25 m
  // or more off in a clear line, stays put at first, has its head up; the radio rises as it nears and crackles; leaving
  // ends each with the light out and the body gone; the crab's head is down near the ground, and if it reaches the
  // player they lose its hours and wake 150 m or more away.
  // -S47MenuPointerPeek [-S47PeekPlace=house|saro|cinema|feed|church|gas]: PASS only if: nothing in 30 s indoors at
  // 21:00; at 02:30 the night's draw alone brings it (a lost draw moves on a night, at most 14); its start pose is hidden
  // from the player's actual eye (no pop-in) and its raised head is in a clear line; the radio hears it; a menu stops its
  // clock; it goes and leaves a record of what was seen.
  // -S47MenuPointerReflood: the first light is set up; PASS only if: getting out of it remembers the room; it does not
  // come back in 32 s in the room straight after, nor at 21:00 three days later; at 02:30 its own draw brings it (at most
  // 14 nights); getting out again counts, restarts the rest and leaves a record.
  // -S47MenuPointerNight: the real scheduler from 19:00 to 06:00 on night two, with the world clock and its active-time
  // clock run 40 times over. Set up: the lights' draw comes out yes with its hour at 01:12, every other event but the
  // lights is cancelled as soon as it starts, the lights too once started, and the site's 60 percent. Natural: every roll
  // but the lights', every start, the visit from standing at the marks. PASS only if: no body, no visit, no peek and no
  // body-event before 02:00; the lights start past midnight, not before their hour; a visit at the marks waits for 02:00.
  // -S47MenuPointerRest: the real Rest with the Director running. Set up: armed marks, then dice forced to yes, then to
  // no, in that order. PASS only if: rest over marks stops 02:05 to 03:00 with the call given; the next goes to morning;
  // the next night, dice yes or not, sleeps through (never two running); the night after, dice yes stops it again and an
  // event starts by itself; rest is refused, with the reason, only if what started is close, and goes through once over;
  // dice no sleeps straight through two nights.
  // -S47MenuPointerPlanner: what the planner does around the player. Set up: night two at 22:00, every draw no but the
  // lights' at 22:00, scheduler clock 40 times over; the real jeep and getting out. Natural: the draw, the wait, the start,
  // the tip, the task row. PASS only if: nothing starts at the wheel and the draw is not spent; parked, the lights start by
  // themselves; a rest under them is accepted and ends them, leaving their marks and the task row; a rest under
  // service_presence is refused with its reason; s03_failure (TestBegin: its dice not awaited, the rest of its life natural)
  // leaves the fault in the equipment and no event; empty_car ends when the player leaves; a failed save starts nothing;
  // the third card of a day is not up in the morning, says when, and can be pointed at that evening.
  bool StepNight(US47Main* M)
  {
   StageAt=Elapsed;
   auto& F=*M->Freeform;auto& Site=*F.FarmSite;auto& D=*F.Director;
   const double Hour=FMath::Fmod(F.Clock(),86400.)/3600.,Late=Hour<12?Hour+24:Hour;
   const auto Marks=Site.Centre();
   const bool bTheirs=FS47FarmSite::TheirHours(F.Clock());
   if(!bTheirs&&(!M->Night->Glimpses->Individuals().IsEmpty()||(Site.VisitStage()>=1&&Site.VisitStage()<=3)||F.IndoorDread->PeekActive()))
    return Fail(TEXT("no body before two in the morning"));
   if(Stage==460)
   {
    if(Late>=30)
    {
     if(DreadFirst<=0)return Fail(TEXT("the night's lights never started"));
     UE_LOG(LogTemp,Display,TEXT("S47: night PASS: lights at %.2f, the visit waited for two, %d events in all"),DreadFirst-24,DreadRound);
     FPlatformMisc::RequestExitWithStatus(false,0);return false;
    }
    const FString Id=D.ActiveEvent();
    if(Id.IsEmpty()){if(!F.TestAdvance(FMath::Min(Elapsed-DreadAt,.25)*560.))return Fail(TEXT("no clock"));DreadAt=Elapsed;return true;}
    ++DreadRound;
    UE_LOG(LogTemp,Display,TEXT("S47: night: %s started by itself at %.2f"),*Id,Hour);
    if((Id==TEXT("observer")||Id==TEXT("service_presence")||Id==TEXT("feeder_presence")||Id==TEXT("array_presence")||Id==TEXT("close_pass"))&&!bTheirs)return Fail(TEXT("no such event before two in the morning"));
    const bool bLights=Id==TEXT("farm_lights")||Id==TEXT("town_lights");
    if(bLights&&(Late<25.2||Late>=30))return Fail(TEXT("the lights must keep their hour, past midnight"));
    if(!D.CancelForTest())return Fail(TEXT("event would not stop"));
    if(bLights&&DreadFirst<=0)
    {
     DreadFirst=Late;
     if(!Site.Armed())return Fail(TEXT("the lights must leave marks"));
     if(Site.PlaceId()!=TEXT("farm")){M->World->PlaceTruck(TEXT("diner"));M->World->Enter(TEXT("diner"));}
     // The scheduler back to its own pace while the player stands at the marks: nothing else may crowd the visit.
     D.TestSchedule(-1.f,25.2,1.);
     Site.TestReadyVisit();DreadAt=Elapsed;Stage=461;
    }
    return true;
   }
   if(Stage>=461&&Stage<=463&&!D.ActiveEvent().IsEmpty())
   {
    UE_LOG(LogTemp,Display,TEXT("S47: night: %s started at %.2f, stopped so the visit can be judged"),*D.ActiveEvent(),Hour);
    D.CancelForTest();return true;
   }
   if(Stage==461)
   {
    if(Elapsed-DreadAt<2)return true;
    M->Player->Place(Marks.X+6,Marks.Z+6,0);M->Player->Pitch=0;DreadAt=Elapsed;Stage=462;return true;
   }
   if(Stage==462)
   {
    // Standing there with the clock held: before two nothing comes, however long.
    if(!bTheirs&&Elapsed-DreadAt<16)return true;
    if(!bTheirs)UE_LOG(LogTemp,Display,TEXT("S47: night: 16 s at the marks at %.2f, nothing came"),Hour);
    if(!bTheirs&&!DreadToClock(M,2.2))return Fail(TEXT("no clock"));
    DreadAt=Elapsed;Stage=463;return true;
   }
   if(Stage==463)
   {
    if(Site.VisitStage()>=1){UE_LOG(LogTemp,Display,TEXT("S47: night: visit began at %.2f, kind %d"),Hour,Site.VisitKind());M->Player->Place(Marks.X+90,Marks.Z,0);DreadAt=Elapsed;Stage=464;return true;}
    if(Elapsed-DreadAt>25)return Fail(TEXT("standing at the marks in the small hours must bring the visit"));
    return true;
   }
   if(Stage==464)
   {
    if(Site.VisitStage()!=4){if(Elapsed-DreadAt>8)return Fail(TEXT("leaving must end it"));return true;}
    D.TestSchedule(-1.f,25.2,40.);DreadAt=Elapsed;Stage=460;return true;
   }
   // What the planner does around the player.
   if(Stage==470)
   {
    if(Elapsed-DreadAt<1)return true;
    const auto Choice=M->World->TruckChoice(TEXT("saro"));
    if(!Choice.Use)return Fail(TEXT("the jeep must offer the drive"));
    Choice.Use();DreadAt=Elapsed;Stage=471;return true;
   }
   if(Stage==471)
   {
    if(M->World->bBusy||!M->World->bDriving){if(Elapsed-DreadAt>15)return Fail(TEXT("the player must be at the wheel"));DreadAt=M->World->bDriving?Elapsed:DreadAt;return true;}
    if(!D.ActiveEvent().IsEmpty())return Fail(TEXT("nothing may start at the wheel"));
    if(Elapsed-DreadAt<14)return true;
    if(D.LightsDrawnNight()!=F.Day())return Fail(TEXT("the night's draw must wait for the player to park"));
    if(!M->World->GetOutHere())return Fail(TEXT("the player must be able to get out"));
    DreadAt=Elapsed;Stage=472;return true;
   }
   if(Stage==472)
   {
    const FString Id=D.ActiveEvent();
    if(Id.IsEmpty())return Elapsed-DreadAt>30?Fail(TEXT("parked, the night's lights must start by themselves")):true;
    if(Id!=TEXT("farm_lights")&&Id!=TEXT("town_lights"))return Fail(TEXT("the lights come first in the night's order"));
    UE_LOG(LogTemp,Display,TEXT("S47: planner: %s started by itself after parking"),*Id);
    // The scheduler back to its own pace: the next tick is minutes off, and the fixtures below have the night to themselves.
    D.TestSchedule(0.f,-1,1);
    DreadAt=Elapsed;Stage=473;return true;
   }
   const auto Reported=[&F]{for(const auto& Task:F.Tasks())if(Task.Text.StartsWith(TEXT("Reported: lights at")))return true;return false;};
   if(Stage==473)
   {
    if(Elapsed-DreadAt<2)return true;
    if(!F.Objective().Contains(TEXT("RADIO TIP"))||!Reported())return Fail(TEXT("the tip stands on the objective line and in the task list"));
    if(!D.RestBlock().IsEmpty())return Fail(TEXT("lights in the distance do not stop a rest"));
    const double Before=F.Clock();
    if(!F.WaitUntil(Before+1800.)||F.Clock()<=Before)return Fail(TEXT("a rest under far lights must be accepted"));
    if(!D.ActiveEvent().IsEmpty()||!Site.Armed()||!Reported())return Fail(TEXT("sleeping ends them and leaves their marks and the task row"));
    DreadAt=Elapsed;Stage=474;return true;
   }
   if(Stage==474)
   {
    if(Elapsed-DreadAt<1)return true;
    if(!D.TestBegin(TEXT("service_presence")))return Fail(TEXT("the presence must start for this"));
    const double Before=F.Clock();
    if(F.WaitUntil(Before+1800.)||F.Clock()!=Before||!F.FeedbackText().Contains(TEXT("service yard")))return Fail(TEXT("a rest under what is close is refused, and says what stops it"));
    if(!D.CancelForTest())return Fail(TEXT("event would not stop"));
    if(!F.Field->HasArrayFault()&&!D.TestBegin(TEXT("s03_failure")))return Fail(TEXT("the fault must start for this")); // The planner may have brought one already.
    DreadAt=Elapsed;Stage=475;return true;
   }
   if(Stage==475)
   {
    if(Elapsed-DreadAt<1.5)return true;
    if(!D.ActiveEvent().IsEmpty()||!F.Field->HasArrayFault())return Fail(TEXT("a fault stays in the equipment, not in the event"));
    if(!D.TestBegin(TEXT("empty_car")))return Fail(TEXT("the empty car must start for this"));
    DreadAt=Elapsed;Stage=476;return true;
   }
   if(Stage==476)
   {
    if(Elapsed-DreadAt<1)return true;
    if(D.ActiveEvent()!=TEXT("empty_car"))return Fail(TEXT("the empty car stands while the player is here"));
    M->World->Enter(TEXT("road"));DreadAt=Elapsed;Stage=477;return true;
   }
   if(Stage==477)
   {
    if(M->Web->Area()!=TEXT("road")||Elapsed-DreadAt<1)return Elapsed-DreadAt>15?Fail(TEXT("the road must load")):true;
    if(!D.ActiveEvent().IsEmpty())return Fail(TEXT("leaving ends what has no hardware behind it"));
    M->World->Enter(TEXT("saro"));DreadAt=Elapsed;Stage=478;return true;
   }
   if(Stage==478)
   {
    if(M->Web->Area()!=TEXT("saro")||Elapsed-DreadAt<1)return Elapsed-DreadAt>15?Fail(TEXT("SARO must load")):true;
    M->Player->Place(0,-1.2,0);
    D.TestSaveFails(true);if(!D.TestBegin(TEXT("flyover")))return Fail(TEXT("the flyover must start for this"));
    DreadAt=Elapsed;Stage=479;return true;
   }
   if(Stage==479)
   {
    if(Elapsed-DreadAt<1)return true;
    D.TestSaveFails(false);
    if(!D.ActiveEvent().IsEmpty())return Fail(TEXT("no save, no event"));
    // The evening's work. Only the card is in question: no power would hide it.
    F.TestEveningGate();if(!DreadToClock(M,9.))return Fail(TEXT("no clock"));
    const int32 Today=F.Day();const int32 Index=F.Jobs().IndexOfByPredicate([Today](const FS47FreeformSession::FJob& J){return J.Day==Today&&J.Slot==2;});
    if(Index==INDEX_NONE||!F.SelectJob(Index))return Fail(TEXT("the day has a third card"));
    if(F.PointSelected())return Fail(TEXT("the evening's source is not up in the morning"));
    if(!F.JobNextAction(*F.CurrentJob()).Contains(TEXT("EVENING OBSERVATION")))return Fail(TEXT("it says when"));
    if(F.PowerFault())F.SetPowerFault(false);
    if(!DreadToClock(M,23.))return Fail(TEXT("no clock"));
    if(!F.PointSelected())return Fail(TEXT("in the evening it can be pointed at"));
    UE_LOG(LogTemp,Display,TEXT("S47: planner PASS"));
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   // The rest.
   if(Stage==480)
   {
    if(Elapsed-DreadFrom<2)return true;
    const int32 Day=F.Day();
    if(!F.Rest())return Fail(TEXT("rest must be accepted"));
    if(F.LastWakeClock()!=F.Clock())return Fail(TEXT("this rest must have been broken, not finished"));
    const double Now=FMath::Fmod(F.Clock(),86400.)/3600.;
    if(Now<2+4.9/60.||Now>3.01||F.Day()!=Day)return Fail(TEXT("rest over standing marks must stop between 02:05 and 03:00, the same day"));
    if(!Site.Nudged())return Fail(TEXT("waking must give the call about the marks"));
    UE_LOG(LogTemp,Display,TEXT("S47: rest: woken at %.2f by the call"),Now);
    DreadAt=Elapsed;Stage=481;return true;
   }
   if(Stage==481)
   {
    if(Elapsed-DreadAt<2)return true;
    const int32 Day=F.Day();
    if(!F.Rest())return Fail(TEXT("a second rest must be accepted"));
    if(F.Day()!=Day+1||!FMath::IsNearlyEqual(FMath::Fmod(F.Clock(),86400.),8.*3600.,1.))return Fail(TEXT("the second rest must reach the morning"));
    D.TestSchedule(1.f,25.,1);DreadAt=Elapsed;Stage=482;return true;
   }
   if(Stage==482)
   {
    // The night after a broken night, whatever is drawn.
    if(Elapsed-DreadAt<2)return true;
    const int32 Day=F.Day();
    if(!F.Rest())return Fail(TEXT("rest must be accepted"));
    if(F.Day()!=Day+1||!FMath::IsNearlyEqual(FMath::Fmod(F.Clock(),86400.),8.*3600.,1.))return Fail(TEXT("never two broken nights running"));
    DreadAt=Elapsed;Stage=486;return true;
   }
   if(Stage==486)
   {
    if(Elapsed-DreadAt<2)return true;
    const int32 Day=F.Day();
    if(!F.Rest())return Fail(TEXT("rest must be accepted"));
    const double Now=FMath::Fmod(F.Clock(),86400.)/3600.;
    if(Now<2+4.9/60.||Now>3.01||F.Day()!=Day)return Fail(TEXT("a drawn night must stop the rest between 02:05 and 03:00"));
    UE_LOG(LogTemp,Display,TEXT("S47: rest: woken at %.2f by the night's draw"),Now);
    DreadAt=Elapsed;Stage=483;return true;
   }
   if(Stage==483)
   {
    const FString Id=D.ActiveEvent();
    if(Id.IsEmpty()){if(Elapsed-DreadAt>30)return Fail(TEXT("what woke them must follow by itself"));return true;}
    UE_LOG(LogTemp,Display,TEXT("S47: rest: %s started by itself after waking"),*Id);
    const double Before=F.Clock();
    const bool bClose=!D.RestBlock().IsEmpty();
    if(F.WaitUntil(F.Clock()+60.)==bClose||(bClose&&F.Clock()!=Before))return Fail(TEXT("no sleeping through what is close, and nothing else stops it"));
    if(M->UI)M->UI->Close(true);
    if(!D.CancelForTest())return Fail(TEXT("event would not stop"));
    D.TestSchedule(0.f,-1,1);DreadAt=Elapsed;Stage=484;return true;
   }
   if(Stage==484)
   {
    if(Elapsed-DreadAt<2)return true;
    const int32 Day=F.Day();
    if(!F.Rest())return Fail(TEXT("rest after the encounter must not lock"));
    if(F.Day()!=Day+1||!FMath::IsNearlyEqual(FMath::Fmod(F.Clock(),86400.),8.*3600.,1.))return Fail(TEXT("rest after waking must reach the morning"));
    DreadAt=Elapsed;Stage=485;return true;
   }
   if(Stage==485)
   {
    if(Elapsed-DreadAt<2)return true;
    const int32 Day=F.Day();
    if(!F.Rest())return Fail(TEXT("rest must be accepted"));
    if(F.Day()!=Day+1||!FMath::IsNearlyEqual(FMath::Fmod(F.Clock(),86400.),8.*3600.,1.))return Fail(TEXT("a night with nothing drawn sleeps straight through"));
    // The first of these is the night after the broken one, which proves nothing about the dice. The second does.
    if(++DreadRound<2){DreadAt=Elapsed;return true;}
    UE_LOG(LogTemp,Display,TEXT("S47: rest PASS"));
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   return Fail(TEXT("no such night stage"));
  }
  // -S47MenuPointerAfter: taken for real, by the crab at the marks at 02:30, from the visit's own dice (only the dice of
  // the line and the cow are set to yes). PASS only if: before it, nothing is wrong and nothing is saved; after it the
  // workstation shows the time they were taken and the watch does not, the line is in the notes, and the cow is planned;
  // both survive a disk Continue; next morning the ninth is gone from both herds, out of sight, and stays gone through a
  // Continue; counting them on the spot keeps one record; OPS shows the old time for a moment and then the right one, and
  // that survives a Continue; days later the cow is back, also after a Continue; a clock nobody logs on to is right
  // after a day. Pictures: after-cow-gone, after-pc-stuck, after-pc-right.
  // -S47MenuPointerCarLight: the empty car at 03:00 with its own dice set to yes. PASS only if: the light begins with a
  // warning; walking off costs no time and leaves no taking; going up to the car afterwards keeps its own record; a second
  // time, staying beside it, they are taken and the event ends. Picture: after-car-warning.
  bool AfterContinue(US47Main* M,int32 Next)
  {
   M->UI->Close(true);const auto State=M->Flow->GameState();
   if(!MysteryRestore(M,State))return false;
   if(M->Freeform->Director)M->Freeform->Director->Stop();
   Stage=Next;AfterAt=Elapsed;return true;
  }
  // -S47MenuPointerDoor [-S47DoorPlace=house|cinema|feed|church]: something at a shut door, by the draw's own rule. Set up:
  // the Director off, the draw's dice set to yes, the player put beside the door it chose, the clock moved on between rounds,
  // and in town the room's door shut for them (it stands open as built). Natural: the wait indoors, the choice of door, the
  // whole visit, the E through the interactable. PASS only if: nothing in 32 s at 21:00; at 02:30 it comes by itself; a menu
  // holds it; E while the handle moves holds the door, it goes, the door stays shut, a record and three marks are left and
  // the marks can be looked at; the next night nothing comes, the night after it does; left alone the door opens by itself,
  // something stands there, stays while not looked at and goes when looked at, with its own record and two marks; a save
  // in the middle gives a quiet room and spends that night; E while it waits opens the door on nothing, with its own record
  // and one mark; pushed from the doorway the door does not shut and that is no holding, from clear of it it shuts and it is;
  // something bigger (set up: a Dread begun by hand) at the handle leaves the door shut, no record, the night handed back, and
  // it comes again that night; the same with the door opening behind the player's back shuts the door again and does not come
  // back that night, but the next; the same in plain view leaves the door open, and the body goes, with a record and two marks.
  // Nobody is taken, and nothing hunts.
  bool StepDoor(US47Main* M)
  {
   StageAt=Elapsed;
   auto& F=*M->Freeform;auto& Dread=*F.IndoorDread;auto& Visit=Dread.Door();
   using EV=FS47DoorVisit::EStage;
   const FString Today=FString::FromInt(F.Day());
   const auto NobodyHarmed=[&]
   {
    if(F.Taken->Busy())return false;
    for(const auto& I:M->Night->Glimpses->Individuals())if(I.Mode==FS47RoswellBrain::EMode::Hunt)return false;
    return true;
   };
   const auto StandBy=[&](double Off)
   {
    const FVector Spot=Visit.DoorMiddle()-Visit.DoorOutward()*Off;
    M->Player->Place(Spot.X,Spot.Z,M->Player->Yaw);
   };
   const auto Finding=[&](const TCHAR* Prefix){return F.HasFinding(FString(Prefix)+Today);};
   if(Stage==540)
   {
    if(Elapsed-DreadFrom<3)return true;
    FVector Room=S47FarmHouse::Point(6,4.4)+FVector(-24,0,300);
    if(DreadPlace!=TEXT("house"))
    {
     if(!S47CalicheAccess::RoomMiddle(DreadPlace,Room))return Fail(TEXT("no such room"));
     if(S47CalicheAccess::IsOpen(DreadPlace))S47CalicheAccess::UseDoor(DreadPlace);
    }
    M->Player->Place(Room.X,Room.Z,0);M->Player->Pitch=0;Visit.TestDice(0.f);
    DreadAt=Elapsed;Stage=541;return true;
   }
   if(Stage==541)
   {
    if(Visit.Active())return Fail(TEXT("nothing may come to a door at 21:00"));
    if(Elapsed-DreadAt<32)return true;
    UE_LOG(LogTemp,Display,TEXT("S47: door: 32 s indoors at 21:00 in %s, nothing"),*DreadPlace);
    if(!DreadToClock(M,2.5))return Fail(TEXT("the door test needs the small hours"));
    DreadRound=0;DreadAt=Elapsed;Stage=542;return true;
   }
   if(Stage==542)
   {
    if(!Visit.Active()){if(Elapsed-DreadAt>110)return Fail(TEXT("it must come by itself: a shut door, indoors, in their hours"));return true;}
    if(!FS47FarmSite::TheirHours(F.Clock()))return Fail(TEXT("not before two"));
    if(Visit.LastDayOf()!=F.Day()||Visit.StageNow()!=EV::Trying)return Fail(TEXT("it starts at the handle, and the night's one is spent"));
    UE_LOG(LogTemp,Display,TEXT("S47: door: round %d, %s, after %.0f s indoors"),DreadRound,*Visit.DoorKeyNow(),Elapsed-DreadAt);
    DoorStep=0;DreadAt=Elapsed;Stage=DreadRound==0?543:DreadRound==1?548:DreadRound==2?552:DreadRound==3?554:DreadRound==4?557:DreadRound==5?558:559;return true;
   }
   // Round 0: hold it.
   if(Stage==543)
   {
    StandBy(1.7);DreadLookAt(M,Visit.DoorMiddle()+FVector(0,1.1,0));
    if(Elapsed-DreadAt<1)return true;
    Shot(TEXT("door-trying.png"));M->bPausedByMenu=true;DoorHeld=Visit.VisitClock();DreadAt=Elapsed;Stage=544;return true;
   }
   if(Stage==544)
   {
    if(Elapsed-DreadAt<2)return true;
    M->bPausedByMenu=false;
    if(FMath::Abs(Visit.VisitClock()-DoorHeld)>.001)return Fail(TEXT("a menu must hold it"));
    DreadAt=Elapsed;Stage=545;return true;
   }
   if(Stage==545)
   {
    StandBy(1.7);DreadLookAt(M,Visit.DoorMiddle()+FVector(0,1.1,0));
    if(M->Inter->Update(M->Player->EyeUE(),M->Player->ForwardUE())!=Visit.DoorKeyNow())
    {
     if(Elapsed-DreadAt>3)return Fail(TEXT("looking at the door from beside it must offer it"));
     return true;
    }
    if(Visit.StageNow()!=EV::Trying)return Fail(TEXT("the test must press while the handle moves"));
    if(M->Inter->LabelOf(Visit.DoorKeyNow())!=TEXT("Hold the door"))return Fail(TEXT("the prompt must say what E does now"));
    M->Inter->UseId(Visit.DoorKeyNow());
    if(Visit.StageNow()!=EV::Holding)return Fail(TEXT("E while the handle moves must hold it"));
    DreadAt=Elapsed;Stage=546;return true;
   }
   if(Stage==546)
   {
    if(Visit.Active()){if(Elapsed-DreadAt>10)return Fail(TEXT("held, it must go"));return true;}
    if(Visit.DoorOpenNow())return Fail(TEXT("a held door stays shut"));
    if(!Finding(TEXT("door-visit-")))return Fail(TEXT("holding it must leave a record"));
    if(Visit.MarksOut()!=3||Visit.TraceKind()!=FS47DoorVisit::EEnding::Held)return Fail(TEXT("three marks, from holding it"));
    if(FS47FreeformCases::LineForFinding(TEXT("door-visit-")+Today)!=2)return Fail(TEXT("it belongs with the visitors around the base"));
    if(!NobodyHarmed())return Fail(TEXT("nothing may follow a door visit"));
    if(!Visit.UseMarks())return Fail(TEXT("the marks must be there to look at"));
    if(!Finding(TEXT("door-marks-")))return Fail(TEXT("looking at the marks must keep a record"));
    M->UI->Close(true);
    UE_LOG(LogTemp,Display,TEXT("S47: door: held, %d marks looked at"),Visit.MarksOut());
    if(!F.TestAdvance(86400.))return Fail(TEXT("no next night"));
    DreadAt=Elapsed;Stage=547;return true;
   }
   if(Stage==547)
   {
    if(Visit.Active())return Fail(TEXT("at most one in two nights"));
    if(Elapsed-DreadAt<34)return true;
    if(Visit.RollDayOf()!=F.Day()||Visit.DrawnTonight())return Fail(TEXT("the night after may not draw it, with the dice at yes"));
    if(!F.TestAdvance(86400.))return Fail(TEXT("no next night"));
    DreadRound=1;DreadAt=Elapsed;Stage=542;return true;
   }
   // Round 1: walk off into the room and leave it.
   if(Stage==548)
   {
    StandBy(4.5);
    // Back to it, so that it can be there and not be looked at.
    DreadLookAt(M,Visit.DoorMiddle()-Visit.DoorOutward()*30+FVector(0,1.6,0));
    if(Visit.StageNow()==EV::Opening&&!Visit.DoorOpenNow())return Fail(TEXT("opening means the door no longer holds"));
    if(Visit.StageNow()==EV::Holding)return Fail(TEXT("nobody pressed anything"));
    if(Visit.StageNow()!=EV::Standing){if(Elapsed-DreadAt>25)return Fail(TEXT("left alone, the door must come open"));return true;}
    if(!Visit.BodyEye().IsSet())return Fail(TEXT("something must stand there"));
    DreadAt=Elapsed;Stage=549;return true;
   }
   if(Stage==549)
   {
    if(Visit.StageNow()!=EV::Standing)return Fail(TEXT("not looked at, it stays"));
    if(Elapsed-DreadAt<3)return true;
    DreadLookAt(M,*Visit.BodyEye());DreadAt=Elapsed;Stage=550;return true;
   }
   if(Stage==550)
   {
    if(Visit.StageNow()==EV::Standing||Visit.StageNow()==EV::Leaving)
    {
     if(Elapsed-DreadAt>10)return Fail(TEXT("looked at, it must go"));
     if(Visit.BodyEye().IsSet())DreadLookAt(M,*Visit.BodyEye());
     if(Elapsed-DreadAt>.3&&DoorFirst==0){DoorFirst=1;Shot(TEXT("door-standing.png"));}
     return true;
    }
    if(Visit.Active())return Fail(TEXT("it can only go from there"));
    if(!Visit.DoorOpenNow())return Fail(TEXT("the door is left open"));
    if(!Finding(TEXT("door-visit-")))return Fail(TEXT("what was seen must be in the field records"));
    if(Visit.MarksOut()!=2||Visit.TraceKind()!=FS47DoorVisit::EEnding::Walked)return Fail(TEXT("two marks, from leaving it"));
    if(!NobodyHarmed())return Fail(TEXT("nothing may follow a door visit"));
    // Shut it again, the ordinary way: it is an ordinary door now.
    M->Inter->UseId(Visit.DoorKeyNow());DreadAt=Elapsed;Stage=551;return true;
   }
   if(Stage==551)
   {
    if(Elapsed-DreadAt<1.2)return true;
    if(Visit.DoorOpenNow())return Fail(TEXT("with nothing there, E is the door's own again"));
    if(!F.TestAdvance(2*86400.))return Fail(TEXT("no next night"));
    DreadRound=2;DreadAt=Elapsed;Stage=542;return true;
   }
   // Round 2: a save in the middle of it.
   if(Stage==552)
   {
    const FString Key=Visit.DoorKeyNow();const int32 Spent=Visit.LastDayOf();
    Dread.Begin(Dread.Save());
    if(Visit.Active()||Visit.LastDayOf()!=Spent)return Fail(TEXT("Continue finds a quiet room, and the night's one spent"));
    if(Dread.DoorLabel(Key).Len()>0)return Fail(TEXT("after Continue it is an ordinary door"));
    Visit.TestDice(0.f);DreadAt=Elapsed;Stage=553;return true;
   }
   if(Stage==553)
   {
    if(Visit.Active())return Fail(TEXT("the night's one must not come again after Continue"));
    if(Elapsed-DreadAt<30)return true;
    if(!F.TestAdvance(2*86400.))return Fail(TEXT("no next night"));
    DreadRound=3;DreadAt=Elapsed;Stage=542;return true;
   }
   // Round 3: open it yourself.
   if(Stage==554)
   {
    StandBy(1.7);DreadLookAt(M,Visit.DoorMiddle()+FVector(0,1.1,0));
    if(Visit.StageNow()==EV::Trying)return true;
    if(Visit.StageNow()!=EV::Waiting)return Fail(TEXT("after the handle, it waits"));
    if(M->Inter->LabelOf(Visit.DoorKeyNow())!=TEXT("Open the door"))return Fail(TEXT("the prompt must say what E does now"));
    if(M->Inter->Update(M->Player->EyeUE(),M->Player->ForwardUE())!=Visit.DoorKeyNow())return true;
    M->Inter->UseId(Visit.DoorKeyNow());
    if(Visit.Active())return Fail(TEXT("opened, it is over at once, and the sound with it"));
    if(!Visit.DoorOpenNow())return Fail(TEXT("the door must be open"));
    if(Visit.BodyEye().IsSet())return Fail(TEXT("an empty threshold"));
    if(!Finding(TEXT("door-visit-")))return Fail(TEXT("opening it must leave a record"));
    if(Visit.MarksOut()!=1||Visit.TraceKind()!=FS47DoorVisit::EEnding::Opened)return Fail(TEXT("one mark, from opening it"));
    DreadAt=Elapsed;Stage=555;return true;
   }
   if(Stage==555)
   {
    if(Elapsed-DreadAt<1)return true;
    DreadLookAt(M,Visit.DoorMiddle()+Visit.DoorOutward()*.55);
    Shot(TEXT("door-threshold.png"));DreadAt=Elapsed;Stage=556;return true;
   }
   if(Stage==556)
   {
    if(Elapsed-DreadAt<.5)return true;
    if(!NobodyHarmed())return Fail(TEXT("nothing may follow a door visit"));
    // Shut it again, for the rounds below.
    M->Inter->UseId(Visit.DoorKeyNow());
    if(Visit.DoorOpenNow())return Fail(TEXT("with nothing there, E is the door's own again"));
    if(!F.TestAdvance(2*86400.))return Fail(TEXT("no next night"));
    DreadRound=4;DreadAt=Elapsed;Stage=542;return true;
   }
   // Round 4: pushed from the doorway, then from clear of it.
   if(Stage==557)
   {
    StandBy(1.7);DreadLookAt(M,Visit.DoorMiddle()+FVector(0,1.1,0));
    if(Visit.StageNow()!=EV::Opening){if(Elapsed-DreadAt>25)return Fail(TEXT("left alone, the door must come open"));return true;}
    StandBy(0);M->Inter->UseId(Visit.DoorKeyNow());
    if(Visit.StageNow()!=EV::Opening||!Visit.DoorOpenNow())return Fail(TEXT("from the doorway it cannot be pushed shut, and it goes on"));
    if(Finding(TEXT("door-visit-")))return Fail(TEXT("a door that did not shut is no holding it"));
    StandBy(1.7);M->Inter->UseId(Visit.DoorKeyNow());
    if(Visit.Active()||Visit.DoorOpenNow())return Fail(TEXT("from clear of it, it shuts"));
    if(!Finding(TEXT("door-visit-"))||Visit.TraceKind()!=FS47DoorVisit::EEnding::Held)return Fail(TEXT("that one is holding it"));
    UE_LOG(LogTemp,Display,TEXT("S47: door: the doorway refuses, and clear of it it shuts"));
    DoorFirst=Visit.LastDayOf();
    if(!F.TestAdvance(2*86400.))return Fail(TEXT("no next night"));
    DreadRound=5;DreadAt=Elapsed;Stage=542;return true;
   }
   // Round 5: something bigger, while the handle moves.
   if(Stage==558)
   {
    if(DoorStep==0)
    {
     if(!F.Taken->Begin(ES47Taker::Roswell,TEXT("the door test")))return Fail(TEXT("the test needs something bigger"));
     DoorStep=1;DreadAt=Elapsed;return true;
    }
    if(Visit.Active()){if(Elapsed-DreadAt>2)return Fail(TEXT("it gives way at once"));return true;}
    F.Taken->GiveUp();
    if(Visit.DoorOpenNow())return Fail(TEXT("the door stays shut"));
    if(Finding(TEXT("door-visit-"))||Visit.MarksOut()!=0)return Fail(TEXT("this one was never here: no record, no marks"));
    if(Visit.LastDayOf()!=int32(DoorFirst)||!Visit.DrawnTonight())return Fail(TEXT("and the night is handed back"));
    UE_LOG(LogTemp,Display,TEXT("S47: door: gave way at the handle, the night handed back"));
    DreadRound=6;DreadAt=Elapsed;Stage=542;return true;
   }
   // Rounds 6 and 7: something bigger with the door coming open, first behind the player's back, then in plain view.
   if(Stage==559)
   {
    const bool bSeen=DreadRound==7;
    if(DoorStep==0)
    {
     if(bSeen){StandBy(1.7);DreadLookAt(M,Visit.DoorMiddle()+FVector(0,1.1,0));}
     else{StandBy(4.5);DreadLookAt(M,Visit.DoorMiddle()-Visit.DoorOutward()*30+FVector(0,1.6,0));}
     if(Visit.StageNow()!=EV::Opening){if(Elapsed-DreadAt>25)return Fail(TEXT("left alone, the door must come open"));return true;}
     if(bSeen&&!Visit.SawItOpen())return true;
     if(!bSeen&&Visit.SawItOpen())return Fail(TEXT("behind the player's back nobody saw it"));
     if(!F.Taken->Begin(ES47Taker::Roswell,TEXT("the door test")))return Fail(TEXT("the test needs something bigger"));
     DoorStep=1;DreadAt=Elapsed;return true;
    }
    if(Visit.Active()){if(Elapsed-DreadAt>(bSeen?8:2))return Fail(TEXT("it must be over soon"));return true;}
    F.Taken->GiveUp();
    if(!bSeen)
    {
     if(Visit.DoorOpenNow())return Fail(TEXT("nobody saw it open, so it is shut again"));
     if(Finding(TEXT("door-visit-")))return Fail(TEXT("this one was never here: no record"));
     if(Visit.LastDayOf()!=int32(DoorFirst)||Visit.DrawnTonight())return Fail(TEXT("the night is handed back, but it comes back only once a night"));
     UE_LOG(LogTemp,Display,TEXT("S47: door: gave way unseen, the door shut again"));
     if(!F.TestAdvance(86400.))return Fail(TEXT("no next night"));
     DreadRound=7;DreadAt=Elapsed;Stage=542;return true;
    }
    if(!Visit.DoorOpenNow())return Fail(TEXT("seen open, it stays open"));
    if(!Finding(TEXT("door-visit-")))return Fail(TEXT("what was seen must be in the field records"));
    if(Visit.MarksOut()!=2||Visit.TraceKind()!=FS47DoorVisit::EEnding::Walked)return Fail(TEXT("two marks, from leaving it"));
    if(Visit.BodyEye().IsSet())return Fail(TEXT("and nothing is left standing there"));
    if(!NobodyHarmed())return Fail(TEXT("nothing may follow a door visit"));
    UE_LOG(LogTemp,Display,TEXT("S47: door PASS in %s: held, left alone and looked at, saved in the middle, opened, doorway, given way three ways"),*DreadPlace);
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   return Fail(TEXT("no such door stage"));
  }
  // -S47MenuPointerAttention: what a body on a visit does about the lamp and noise. Set up: the visit's dice (a small one,
  // then the crab), the lamp in hand, the lamp's dice, the player put near it, the hunt clock stopped so that the reactions
  // can be looked at one by one (its rule is read off the clock above, not here). Natural: when it notices, what it does,
  // the sight line, the patience. PASS only if: a lamp on it stops it and it draws back slowly, the mode unchanged; a lamp
  // on it later blinks instead, and comes back; with the lamp out and the player still its patience runs at double rate, and
  // with the lamp on at the plain rate; a run turns it; a voice on the set turns it; a menu holds all of it; leaving ends it
  // with nothing left over; and the crab, moved by the site itself, stops and draws back too.
  bool StepAttention(US47Main* M)
  {
   StageAt=Elapsed;
   auto& Site=*M->Freeform->FarmSite;auto& Lamp=*M->Night->Flashlight;
   using EA=FS47VisitAttention::EDo;
   const auto Views=M->Night->Glimpses->Individuals();
   const auto AimAtBody=[&]
   {
    TOptional<FVector> Eye=Site.VisitBodyEye();
    if(!Eye.IsSet()&&!Views.IsEmpty())Eye=Views[0].Eye;
    if(Eye.IsSet())DreadLookAt(M,*Eye);
    return Eye;
   };
   const FVector Marks=Site.Centre();
   if(Stage==560)
   {
    if(Site.VisitStage()==4)return Fail(TEXT("the visit ended with no body"));
    if(Site.VisitStage()<2){if(Elapsed-DreadAt>40)return Fail(TEXT("a body must come"));return true;}
    if(Views.IsEmpty())return Fail(TEXT("the small one's visit has no body"));
    Site.TestAttentionRoll(0.f);Lamp.bHave=true;if(!Lamp.bOn)Lamp.Toggle();
    DreadAt=DreadFirst=Elapsed;Stage=561;return true;
   }
   if(Stage==561)
   {
    // Near it, in the open if the land allows; it may have gone to cover, so go on trying.
    if(Views.IsEmpty())return Fail(TEXT("the small one must stay while the player does"));
    if(Elapsed>=DreadFirst){DreadFirst=Elapsed+5;const FVector To=Views[0].Pos+(FVector(Marks.X+6,Views[0].Pos.Y,Marks.Z+6)-Views[0].Pos).GetSafeNormal()*10;M->Player->Place(To.X,To.Z,0);}
    AimAtBody();
    if(Site.VisitAttention().Doing()==EA::Watch)
    {
     UE_LOG(LogTemp,Display,TEXT("S47: attention: the lamp found it after %.1f s, state %d"),Elapsed-DreadAt,int32(Views[0].State));
     DoorBodyFrom=Views[0].Pos;DreadAt=Elapsed;Stage=562;return true;
    }
    if(Elapsed-DreadAt>40)return Fail(TEXT("a lit lamp on it in a clear line must be noticed"));
    return true;
   }
   if(Stage==562)
   {
    AimAtBody();
    if(Views.IsEmpty())return Fail(TEXT("the body must stay"));
    const EA Do=Site.VisitAttention().Doing();
    if(Do==EA::Watch)
    {
     if(FVector::Dist(Views[0].Pos,DoorBodyFrom)>.3)return Fail(TEXT("it must stop while it looks"));
     if(Elapsed-DreadAt>4)return Fail(TEXT("it looks for a moment, no more"));
     return true;
    }
    if(Do!=EA::Back)return Fail(TEXT("with this draw it must draw back"));
    DreadAt=Elapsed;Stage=563;return true;
   }
   if(Stage==563)
   {
    AimAtBody();
    if(Views.IsEmpty())return Fail(TEXT("the body must stay"));
    if(Views[0].Speed>1.)return Fail(TEXT("it draws back slowly"));
    if(Views[0].Mode!=FS47RoswellBrain::EMode::Curious)return Fail(TEXT("no lamp may turn it into a hunter"));
    if(Site.VisitAttention().Holding()){if(Elapsed-DreadAt>8)return Fail(TEXT("the moment must end"));return true;}
    const double Moved=FVector::Dist(Views[0].Pos,DoorBodyFrom);
    UE_LOG(LogTemp,Display,TEXT("S47: attention: it drew back %.1f m in %.1f s"),Moved,Elapsed-DreadAt);
    if(Moved<.2)return Fail(TEXT("it must really have moved back"));
    Site.TestAttentionRoll(1.f);DreadAt=DreadFirst=Elapsed;DoorHeld=1e9;Stage=564;return true;
   }
   if(Stage==564)
   {
    if(Views.IsEmpty())return Fail(TEXT("the body must stay"));
    if(Elapsed>=DreadFirst){DreadFirst=Elapsed+5;const FVector To=Views[0].Pos+(FVector(Marks.X+6,Views[0].Pos.Y,Marks.Z+6)-Views[0].Pos).GetSafeNormal()*10;M->Player->Place(To.X,To.Z,0);}
    AimAtBody();
    if(Site.VisitAttention().Doing()!=EA::Blink){if(Elapsed-DreadAt>40)return Fail(TEXT("the lamp must count again after the pause, and this draw blinks it"));return true;}
    DreadAt=Elapsed;DoorHeld=1e9;Stage=565;return true;
   }
   if(Stage==565)
   {
    AimAtBody();
    DoorHeld=FMath::Min(DoorHeld,Lamp.Intensity());
    if(Site.VisitAttention().Doing()==EA::Blink){if(Elapsed-DreadAt>3)return Fail(TEXT("it comes back on its own"));return true;}
    UE_LOG(LogTemp,Display,TEXT("S47: attention: lamp low %.2f during the blink, %.2f after"),DoorHeld,Lamp.Intensity());
    if(DoorHeld>20)return Fail(TEXT("the lamp must really go out for a moment")); // 64 when steady, about 5 when out
    // Lamp out, standing still.
    Lamp.Toggle();Site.TestFreshPatience();DreadAt=Elapsed;Stage=566;return true;
   }
   if(Stage==566)
   {
    if(Elapsed-DreadAt<1.5)return true;
    DoorHeld=Site.VisitPatienceLeft();DreadAt=Elapsed;Stage=567;return true;
   }
   if(Stage==567)
   {
    if(Elapsed-DreadAt<4)return true;
    const double Rate=(DoorHeld-Site.VisitPatienceLeft())/(Elapsed-DreadAt);
    UE_LOG(LogTemp,Display,TEXT("S47: attention: lamp out and still, its patience runs at %.2f"),Rate);
    if(Rate<1.6)return Fail(TEXT("a dark, still player must be given up on sooner"));
    Lamp.Toggle();DoorHeld=Site.VisitPatienceLeft();DreadAt=Elapsed;Stage=568;return true;
   }
   if(Stage==568)
   {
    if(Elapsed-DreadAt<4)return true;
    const double Rate=(DoorHeld-Site.VisitPatienceLeft())/(Elapsed-DreadAt);
    UE_LOG(LogTemp,Display,TEXT("S47: attention: lamp on, its patience runs at %.2f"),Rate);
    if(Rate>1.4)return Fail(TEXT("with the lamp on it stays interested for the ordinary time"));
    // Out again and off at a run. Looking away, so that the lamp plays no part.
    Lamp.Toggle();M->Player->Yaw+=PI;M->Player->TestMove=FVector2D(0,-.9);M->Player->bTestRun=true;DreadAt=Elapsed;Stage=569;return true;
   }
   if(Stage==569)
   {
    if(Site.VisitAttention().Doing()!=EA::Turn){if(Elapsed-DreadAt>12)return Fail(TEXT("running must be heard"));return true;}
    M->Player->TestMove.Reset();M->Player->bTestRun=false;
    UE_LOG(LogTemp,Display,TEXT("S47: attention: it turned at the run"));
    M->bPausedByMenu=true;DoorHeld=Site.VisitPatienceLeft();DreadAt=Elapsed;Stage=570;return true;
   }
   if(Stage==570)
   {
    if(Elapsed-DreadAt<2)return true;
    M->bPausedByMenu=false;
    if(FMath::Abs(Site.VisitPatienceLeft()-DoorHeld)>.001)return Fail(TEXT("a menu must hold it"));
    DreadAt=Elapsed;Stage=571;return true;
   }
   if(Stage==571)
   {
    // Long enough for the turn to end and the ear to come back. Then a voice on the set.
    if(Elapsed-DreadAt<9)return true;
    if(Site.VisitAttention().Holding())return Fail(TEXT("the turn must end by itself"));
    if(Views.IsEmpty())return Fail(TEXT("the body must stay"));
    const FVector To=Views[0].Pos+(FVector(Marks.X+6,Views[0].Pos.Y,Marks.Z+6)-Views[0].Pos).GetSafeNormal()*12;M->Player->Place(To.X,To.Z,0);
    Site.TestFreshPatience();DreadAt=Elapsed;Stage=572;return true;
   }
   if(Stage==572)
   {
    if(Elapsed-DreadAt<1)return true;
    M->FieldRadio->Message(TEXT("A voice on the set, too broken to make out."));
    DreadAt=Elapsed;Stage=573;return true;
   }
   if(Stage==573)
   {
    if(Site.VisitAttention().Doing()!=EA::Turn){if(Elapsed-DreadAt>2)return Fail(TEXT("a voice on the set must be heard"));return true;}
    UE_LOG(LogTemp,Display,TEXT("S47: attention: it turned at the set"));
    M->Player->Place(Marks.X+90,Marks.Z,0);DreadAt=Elapsed;Stage=574;return true;
   }
   if(Stage==574)
   {
    if(Site.VisitStage()!=4){if(Elapsed-DreadAt>6)return Fail(TEXT("leaving must end the visit"));return true;}
    if(!Views.IsEmpty())return Fail(TEXT("leaving must clear the body"));
    // Now the crab, whom the site moves itself.
    Site.TestAttentionRoll(0.f);Site.TestVisit(1);M->Player->Place(Marks.X+6,Marks.Z+6,0);M->Player->Pitch=0;if(!Lamp.bOn)Lamp.Toggle();
    DreadAt=Elapsed;Stage=575;return true;
   }
   if(Stage==575)
   {
    if(Site.VisitStage()==4)return Fail(TEXT("the crab's visit ended with no body"));
    if(Site.VisitStage()<2){if(Elapsed-DreadAt>35)return Fail(TEXT("the crab must come"));return true;}
    AimAtBody();
    if(Site.VisitAttention().Holding())
    {
     DoorFirst=Site.VisitBodyDistance();DreadAt=Elapsed;Stage=576;return true;
    }
    if(Elapsed-DreadAt>30)return Fail(TEXT("a lamp on the crab must stop it"));
    return true;
   }
   if(Stage==576)
   {
    AimAtBody();
    const EA Do=Site.VisitAttention().Doing();
    if(Do==EA::Watch)
    {
     if(FMath::Abs(Site.VisitBodyDistance()-DoorFirst)>.25)return Fail(TEXT("the crab must stop while it looks"));
     return true;
    }
    if(Do!=EA::Back)return Fail(TEXT("with this draw the crab must draw back"));
    DreadAt=Elapsed;Stage=577;return true;
   }
   if(Stage==577)
   {
    AimAtBody();
    if(Elapsed-DreadAt<2)return true;
    UE_LOG(LogTemp,Display,TEXT("S47: attention: the crab from %.1f m to %.1f m"),DoorFirst,Site.VisitBodyDistance());
    if(Site.VisitBodyDistance()<=DoorFirst)return Fail(TEXT("the crab must really draw back"));
    Shot(TEXT("attention-crab.png"));M->Player->Place(Marks.X+90,Marks.Z,0);DreadAt=Elapsed;Stage=578;return true;
   }
   if(Stage==578)
   {
    if(Site.VisitStage()!=4){if(Elapsed-DreadAt>6)return Fail(TEXT("leaving must end the visit"));return true;}
    if(M->Freeform->Taken->Busy())return Fail(TEXT("a lamp and a noise take nobody"));
    UE_LOG(LogTemp,Display,TEXT("S47: attention PASS: lamp back, lamp blink, dark patience, run, set, menu, leaving, crab"));
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   return Fail(TEXT("no such attention stage"));
  }
  bool StepAfter(US47Main* M)
  {
   StageAt=Elapsed;
   auto& F=*M->Freeform;auto& Af=*F.Aftermath;auto& Tk=*F.Taken;auto& Site=*F.FarmSite;
   if(!M->TownLayer||M->TownLayer->Herds.Num()<2)return Fail(TEXT("no herds"));
   FS47Herd& H0=*M->TownLayer->Herds[0];FS47Herd& H1=*M->TownLayer->Herds[1];
   const FString Real=FS47FreeformCases::Clock(FMath::Fmod(F.Clock(),86400.));
   const FVector Room(0,0,-1.2);
   if(Stage==500)
   {
    if(Tk.LastNote().IsEmpty()||Tk.Busy()||Site.VisitStage()!=4){if(Elapsed-AfterAt>150)return Fail(TEXT("the crab must take a player who stays"));return true;}
    AfterAt=Elapsed;Stage=501;return true;
   }
   if(Stage==501)
   {
    if(Elapsed-AfterAt<1.5)return true;
    UE_LOG(LogTemp,Display,TEXT("S47: after: watch %s, workstation %s, line '%s', cow planned %d. %s"),*Real,*F.TerminalClock(),*Af.LastLine(),Af.CowPlanned(),*Tk.LastNote());
    if(!Af.PcStuck()||F.Clock()-Af.PcShown()<100*60)return Fail(TEXT("the workstation must stand at the time they were taken"));
    if(F.TerminalClock().EndsWith(Real))return Fail(TEXT("the workstation and the watch must disagree"));
    if(!F.TerminalLines(3).Contains(FString(TEXT("SYSTEM CLOCK "))+FS47FreeformCases::Clock(FMath::Fmod(Af.PcShown(),86400.))))return Fail(TEXT("the wrong time must be on the OPS glass"));
    if(Af.LastLine().IsEmpty()||!M->Story->Notes().Contains(Af.LastLine()))return Fail(TEXT("the line must be in the player's notes"));
    if(!Af.CowPlanned())return Fail(TEXT("a night with a visit at the farm must be able to cost a cow"));
    AfterPc=Af.PcShown();AfterLine=Af.LastLine();
    M->Player->Place(Room.X,Room.Z,0);
    return AfterContinue(M,502);
   }
   if(Stage==502)
   {
    if(Elapsed-AfterAt<2.5)return true;
    if(!Af.PcStuck()||!FMath::IsNearlyEqual(Af.PcShown(),AfterPc))return Fail(TEXT("a Continue must keep the clock standing where it stood"));
    if(M->Story->Notes().FilterByPredicate([&](const FString& Text){return Text==AfterLine;}).Num()!=1)return Fail(TEXT("a Continue must keep the line, once"));
    if(!Af.CowPlanned())return Fail(TEXT("a Continue must keep the cow's morning"));
    if(!DreadToClock(M,9.))return Fail(TEXT("the morning must come"));
    AfterAt=Elapsed;Stage=503;return true;
   }
   if(Stage==503)
   {
    if(!Af.CowOut()||H0.Missing!=S47Herd::MISSING||H1.Missing!=S47Herd::MISSING){if(Elapsed-AfterAt>10)return Fail(TEXT("the ninth must be gone in the morning, from both herds"));return true;}
    FVector Where;if(H0.CowAt(S47Herd::MISSING,Where))return Fail(TEXT("nothing may stand where she stood"));
    return AfterContinue(M,504);
   }
   if(Stage==504)
   {
    if(Elapsed-AfterAt<2.5)return true;
    if(!Af.CowOut()||H0.Missing!=S47Herd::MISSING)return Fail(TEXT("a Continue must not bring her back"));
    if(!Af.PcStuck())return Fail(TEXT("nobody has logged on yet"));
    const FVector Mid=H0.Middle();M->Player->Place(Mid.X+6,Mid.Z+6,0);M->Player->Pitch=-.05;
    AfterAt=Elapsed;Stage=505;return true;
   }
   if(Stage==505)
   {
    if(!F.HasFinding(Af.CowFindingId())){if(Elapsed-AfterAt>8)return Fail(TEXT("counting them on the spot must keep a record"));return true;}
    if(Elapsed-AfterAt<1.5)return true;
    Shot(TEXT("after-cow-gone.png"));
    const FVector P=S47Three::ToThree(M->Web->PropLocation(TEXT("opsTerminal")));
    M->Player->Place(P.X,P.Z+1.1,0);AfterAt=Elapsed;Stage=506;return true;
   }
   if(Stage==506)
   {
    if(Elapsed-AfterAt<1)return true;
    if(!F.OpsOnGlass()&&!M->ModalOpen())F.Use(TEXT("opsTerminal"));
    if(!F.OpsOnGlass()){if(Elapsed-AfterAt>6)return Fail(TEXT("OPS must open"));return true;}
    if(!F.TerminalClock().EndsWith(FS47FreeformCases::Clock(FMath::Fmod(AfterPc,86400.))))return Fail(TEXT("logging on must first show the old time"));
    Shot(TEXT("after-pc-stuck.png"));AfterAt=Elapsed;Stage=507;return true;
   }
   if(Stage==507)
   {
    if(Af.PcStuck()){if(Elapsed-AfterAt>FS47Aftermath::LogOnSeconds+30)return Fail(TEXT("logging on must put the clock right"));return true;}
    if(Elapsed-AfterAt<FS47Aftermath::LogOnSeconds-1.5)return Fail(TEXT("it must be seen wrong for a moment first"));
    if(!F.TerminalClock().EndsWith(Real))return Fail(TEXT("the workstation must now agree with the watch"));
    Shot(TEXT("after-pc-right.png"));AfterAt=Elapsed;Stage=508;return true;
   }
   if(Stage==508)
   {
    if(Elapsed-AfterAt<1)return true;
    M->UI->Close(true);M->Player->Place(Room.X,Room.Z,0);
    return AfterContinue(M,509);
   }
   if(Stage==509)
   {
    if(Elapsed-AfterAt<2.5)return true;
    if(Af.PcStuck()||!F.TerminalClock().EndsWith(Real))return Fail(TEXT("once right, a Continue must leave it right"));
    if(!DreadToClock(M,9.,5.))return Fail(TEXT("the days must pass"));
    AfterAt=Elapsed;Stage=510;return true;
   }
   if(Stage==510)
   {
    FVector Where;
    if(Af.CowOut()||H0.Missing!=-1||H1.Missing!=-1||!H0.CowAt(S47Herd::MISSING,Where)){if(Elapsed-AfterAt>10)return Fail(TEXT("she must be back after some days"));return true;}
    return AfterContinue(M,511);
   }
   if(Stage==511)
   {
    if(Elapsed-AfterAt<2.5)return true;
    if(Af.CowOut()||Af.CowPlanned()||H0.Missing!=-1)return Fail(TEXT("back is back, also after a Continue"));
    // Nobody logs on: the clock puts itself right after a day. A second taking, without its long way round.
    Af.AfterTaken(0,F.Clock());
    if(!Af.PcStuck()||!F.TestAdvance(86400.+60.))return Fail(TEXT("the clock's day must be able to pass"));
    AfterAt=Elapsed;Stage=512;return true;
   }
   if(Stage==512)
   {
    if(Af.PcStuck()){if(Elapsed-AfterAt>3)return Fail(TEXT("a day must put the clock right with nobody at it"));return true;}
    UE_LOG(LogTemp,Display,TEXT("S47: after PASS: line '%s'"),*AfterLine);
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   // The empty car, once walked away from, once stood beside.
   auto& D=*F.Director;
   const auto TakenFor=[&]{return F.Clock()-AfterPc;};
   if(Stage==519)
   {
    if(Elapsed-AfterAt<1)return true;
    if(!D.TestBegin(TEXT("empty_car"))||!D.CarLightDrawn())return Fail(TEXT("the car must start, with its light drawn"));
    AfterAt=Elapsed;Stage=520;return true;
   }
   if(Stage==520)
   {
    if(!Tk.Busy()){if(Elapsed-AfterAt>5)return Fail(TEXT("the light must start for someone near in the open"));return true;}
    if(Tk.Stage()!=FS47Taken::EStage::Flood)return Fail(TEXT("it must be the light, not the steps overhead"));
    AfterPc=F.Clock();AfterAt=Elapsed;Stage=521;return true;
   }
   if(Stage==521)
   {
    // The warning: faint, and it has not begun to look yet. Turn and see it; then go.
    const FVector A=D.EventAnchor();const FVector Eye(M->Player->Pos.X,M->Player->FloorY+1.6,M->Player->Pos.Z);
    M->Player->Yaw=FMath::Atan2(Eye.X-A.X,Eye.Z-A.Z);M->Player->Pitch=.1;
    if(Tk.LightOn()>0)return Fail(TEXT("the warning must not count as being in the light"));
    if(Elapsed-AfterAt<3)return true;
    Shot(TEXT("after-car-warning.png"));M->Player->Place(A.X,A.Z+80,0);AfterAt=Elapsed;Stage=522;return true;
   }
   if(Stage==522)
   {
    if(Tk.Busy()){if(Elapsed-AfterAt>90)return Fail(TEXT("the light must give up on someone who left"));return true;}
    if(TakenFor()>10*60||!Tk.LastNote().IsEmpty())return Fail(TEXT("walking off must cost nothing"));
    if(D.ActiveEvent()!=TEXT("empty_car")||D.CarLightStage()!=2){if(Elapsed-AfterAt>92)return Fail(TEXT("the car must still be there to go up to"));return true;}
    const FVector A=D.EventAnchor();M->Player->Place(A.X+2.5,A.Z,0);AfterAt=Elapsed;Stage=523;return true;
   }
   if(Stage==523)
   {
    if(Elapsed-AfterAt<1)return true;
    if(!D.Use(TEXT("ff:event-check"))||!D.ActiveEvent().IsEmpty())return Fail(TEXT("the car must be there to inspect"));
    if(!F.FindingText(TEXT("empty_car")).Contains(TEXT("stopped at")))return Fail(TEXT("going up to it must keep its own record"));
    M->Player->Place(27,24,PI);AfterAt=Elapsed;Stage=524;return true;
   }
   if(Stage==524)
   {
    if(Elapsed-AfterAt<1)return true;
    if(!D.TestBegin(TEXT("empty_car"))||!D.CarLightDrawn())return Fail(TEXT("the car must start a second time"));
    const FVector A=D.EventAnchor();M->Player->Place(A.X+5,A.Z+5,0);AfterAt=Elapsed;Stage=525;return true;
   }
   if(Stage==525)
   {
    if(Tk.LastNote().IsEmpty()||Tk.Busy()){if(Elapsed-AfterAt>120)return Fail(TEXT("the light must reach someone who stays"));return true;}
    if(!D.ActiveEvent().IsEmpty()){if(Elapsed-AfterAt>125)return Fail(TEXT("being taken must end the event"));return true;}
    UE_LOG(LogTemp,Display,TEXT("S47: car light PASS: %s"),*Tk.LastNote());
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   return Fail(TEXT("no such after stage"));
  }
  bool StepDread(US47Main* M)
  {
   if(Stage>=460&&Stage<=489)return StepNight(M);
   if(Stage>=490&&Stage<=499)return StepMystery(M);
   StageAt=Elapsed;
   auto& Site=*M->Freeform->FarmSite;auto& Tk=*M->Freeform->Taken;auto& Dread=*M->Freeform->IndoorDread;
   const FVector Marks=Site.Centre();
   const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
   const FVector EyeNow=M->Night->GlimpseIn(true).Cam.Pos;
   static const float Rolls[3]={.35f,.9f,.05f};static const int32 Kinds[3]={2,0,1};
   const auto NextRound=[&]
   {
    Site.TestRollWho(Rolls[DreadRound]);Site.TestReadyVisit();M->Player->Place(Marks.X+6,Marks.Z+6,0);M->Player->Pitch=0;
    DreadAt=Elapsed;DreadSample=0;Stage=402;
   };
   if(Stage==400)
   {
    if(Elapsed-DreadFrom<25)
    {
     if(Site.VisitStage()!=0)return Fail(TEXT("no body may come at the marks at 21:00"));
     if(Site.VisitHushed())return Fail(TEXT("nothing may go quiet for it at 21:00"));
     return true;
    }
    UE_LOG(LogTemp,Display,TEXT("S47: kinds: 25 s at the marks at 21:00, nothing came"));
    if(!DreadToClock(M,3.5))return Fail(TEXT("the kinds test needs the small hours"));
    DreadRound=0;NextRound();return true;
   }
   if(Stage==402)
   {
    if(Site.VisitStage()>=1)
    {
     if(Site.VisitKind()!=Kinds[DreadRound])return Fail(TEXT("the dice must decide who comes"));
     UE_LOG(LogTemp,Display,TEXT("S47: kinds: kind %d's visit began after %.1f s standing there"),Site.VisitKind(),Elapsed-DreadAt);
     DreadAt=Elapsed;Stage=403;return true;
    }
    if(Elapsed-DreadAt>25)return Fail(TEXT("standing at the marks at 03:30 must bring the visit"));
    return true;
   }
   if(Stage==403)
   {
    if(Site.VisitStage()==4)return Fail(TEXT("the visit ended with nothing out: no ground for it, or no model in this build"));
    if(Site.VisitStage()<2){if(Elapsed-DreadAt>35)return Fail(TEXT("a body must come"));return true;}
    const int32 Kind=Site.VisitKind();
    DreadFirst=Site.VisitBodyDistance();
    UE_LOG(LogTemp,Display,TEXT("S47: kinds: kind %d is out %.1f m off"),Kind,DreadFirst);
    if(Kind==2&&DreadFirst<25)return Fail(TEXT("the tall one must first stand far off"));
    if(Kind==1&&DreadFirst>24)return Fail(TEXT("the crab starts within twenty-odd metres"));
    if(Kind!=0)
    {
     const auto BodyEye=Site.VisitBodyEye();
     if(!BodyEye.IsSet())return Fail(TEXT("the body must be out"));
     FHitResult Block;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47KindsSight),true);
     if(M->Web->GetWorld()->LineTraceSingleByChannel(Block,S47Three::ToUE(EyeNow),S47Three::ToUE(*BodyEye),ECC_Visibility,Q)&&FVector::Dist(S47Three::ToThree(Block.ImpactPoint),*BodyEye)>1.5)
      return Fail(TEXT("it must stand in a clear line from the player"));
    }
    DreadAt=Elapsed;Stage=404;return true;
   }
   if(Stage==404)
   {
    const int32 Kind=Site.VisitKind();
    if(Site.VisitStage()==4)return Fail(TEXT("the visit must last while the player stays"));
    if(Kind==2)
    {
     const double To=Site.VisitBodyDistance();
     if(Elapsed-DreadAt<10&&FMath::Abs(To-DreadFirst)>1)return Fail(TEXT("the tall one must first stand still"));
     if(DreadSample==0&&Elapsed-DreadAt>1&&Site.VisitHeadHeight()<1.4){UE_LOG(LogTemp,Error,TEXT("S47: kinds: tall head %.2f m"),Site.VisitHeadHeight());return Fail(TEXT("the tall one must stand tall"));}
     // Further in, louder: three readings of what the set is actually playing.
     const double Base=FMath::Min(DreadFirst,44.)-2,Spacing=FMath::Clamp((DreadFirst-27)/2.5,.5,6.);
     if(DreadSample<3&&To<=Base-Spacing*DreadSample)
     {
      DreadLevels[DreadSample]=M->FieldRadio->RadioLevelNow();
      UE_LOG(LogTemp,Display,TEXT("S47: kinds: radio %.3f at %.1f m, %d crackles"),DreadLevels[DreadSample],To,M->FieldRadio->CrackleCount());
      ++DreadSample;
     }
     if(DreadSample==3)
     {
      if(!(DreadLevels[0]>0&&DreadLevels[0]<DreadLevels[1]&&DreadLevels[1]<DreadLevels[2]))return Fail(TEXT("the radio must rise as it nears"));
      if(M->FieldRadio->CrackleCount()<=0)return Fail(TEXT("the radio must crackle"));
      M->Player->Place(Marks.X+90,Marks.Z,0);DreadAt=Elapsed;Stage=405;return true;
     }
     if(Elapsed-DreadAt>80)return Fail(TEXT("the tall one must come on"));
     return true;
    }
    if(Kind==0)
    {
     if(M->Night->Glimpses->Individuals().IsEmpty())return Fail(TEXT("the small one's visit has no body"));
     if(Elapsed-DreadAt<6)return true;
     M->Player->Place(Marks.X+90,Marks.Z,0);DreadAt=Elapsed;Stage=405;return true;
    }
    // The crab: down on all fours within seconds, then left to it.
    if(Elapsed-DreadAt<3)return true;
    const double Head=Site.VisitHeadHeight();
    UE_LOG(LogTemp,Display,TEXT("S47: kinds: crab head %.2f m over its feet"),Head);
    if(Head<0||Head>1.1)return Fail(TEXT("the crab must go along the ground"));
    DreadClock=M->Freeform->Clock();DreadPos=Feet;DreadAt=Elapsed;Stage=406;return true;
   }
   if(Stage==405)
   {
    if(Site.VisitStage()!=4){if(Elapsed-DreadAt>6)return Fail(TEXT("leaving must end the visit"));return true;}
    if(Tk.Busy()||Site.VisitBodyEye().IsSet()||!M->Night->Glimpses->Individuals().IsEmpty())return Fail(TEXT("leaving must put out the light and take the body"));
    if(!Tk.LastNote().IsEmpty())return Fail(TEXT("leaving must cost nothing"));
    ++DreadRound;NextRound();return true;
   }
   if(Stage==406)
   {
    if(Site.VisitStage()!=4||Tk.Busy()){if(Elapsed-DreadAt>60)return Fail(TEXT("the crab must reach a player who stays"));return true;}
    if(Tk.LastNote().IsEmpty())return Fail(TEXT("the crab must have taken them"));
    const FS47TakenTerms& Terms=FS47Taken::Terms(ES47Taker::Crab);
    const double Gone=(M->Freeform->Clock()-DreadClock)/60.;
    const double Moved=FVector2D::Distance(FVector2D(Feet.X,Feet.Z),FVector2D(DreadPos.X,DreadPos.Z));
    UE_LOG(LogTemp,Display,TEXT("S47: kinds: crab took them, %.0f minutes, moved %.0f m. %s"),Gone,Moved,*Tk.LastNote());
    if(Gone<Terms.MinMinutes-3||Gone>Terms.MaxMinutes+12)return Fail(TEXT("the crab's hours are its own"));
    if(Moved<Terms.WakeMin-1)return Fail(TEXT("the crab leaves them far from there"));
    UE_LOG(LogTemp,Display,TEXT("S47: kinds PASS: tall from %.0f m, radio %.3f, %.3f, %.3f, %d crackles; small; crab"),FMath::Min(DreadFirst,58.),DreadLevels[0],DreadLevels[1],DreadLevels[2],M->FieldRadio->CrackleCount());
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   if(Stage==420)
   {
    if(Elapsed-DreadFrom<3)return true;
    FVector Room=S47FarmHouse::Point(2.5,2.4)+FVector(-24,0,300);
    // In the control room, looking north at its windows; yaw 0 faces -Z.
    if(DreadPlace==TEXT("saro"))Room=FVector(0,0,-1.2);
    else if(DreadPlace!=TEXT("house")&&!S47CalicheAccess::RoomMiddle(DreadPlace,Room))return Fail(TEXT("no such room"));
    M->Player->Place(Room.X,Room.Z,0);
    M->Player->Pitch=0;DreadAt=Elapsed;Stage=421;return true;
   }
   if(Stage==421)
   {
    if(Elapsed-DreadAt<1)return true;
    if(!Dread.PlayerInPeekPlace())return Fail(TEXT("this must count as indoors at a place it looks in"));
    if(Dread.PeekActive())return Fail(TEXT("nothing may look in at 21:00"));
    if(Elapsed-DreadAt<30)return true;
    UE_LOG(LogTemp,Display,TEXT("S47: peek: 30 s indoors at 21:00 in %s, nothing"),*DreadPlace);
    if(!DreadToClock(M,2.5))return Fail(TEXT("the peek test needs the small hours"));
    DreadRound=0;DreadAt=Elapsed;Stage=422;return true;
   }
   if(Stage==422)
   {
    if(Dread.PeekActive()){UE_LOG(LogTemp,Display,TEXT("S47: peek: the night's own draw, after %d lost draws"),DreadRound);DreadAt=Elapsed;Stage=423;return true;}
    if(Dread.PeekRollDayOf()==M->Freeform->Day()&&!Dread.PeekDrawnTonight())
    {
     if(++DreadRound>14)return Fail(TEXT("fourteen nights without the draw: the odds are wrong"));
     if(!M->Freeform->TestAdvance(86400.))return Fail(TEXT("no next night"));
     DreadAt=Elapsed;return true;
    }
    if(Elapsed-DreadAt>120){UE_LOG(LogTemp,Error,TEXT("S47: peek: %d empty rounds from this room"),Dread.SearchesEmpty());return Fail(TEXT("nowhere to look in from, or it never came"));}
    return true;
   }
   if(Stage>=423&&Stage<=426&&!Dread.PeekActive())return Fail(TEXT("it left before the test had seen it"));
   if(Stage==423)
   {
    FS47RoswellWorld Sight;Sight.Init(M);
    if(Sight.ClearSight(EyeNow,Dread.PeekHiddenHead()))return Fail(TEXT("it must start out of sight: this would pop in"));
    UE_LOG(LogTemp,Display,TEXT("S47: peek: %s, %.1f m off, its start is hidden from the eye"),Dread.PeekIsWindow()?TEXT("window"):TEXT("corner"),FVector::Dist(EyeNow,Dread.PeekRaisedHead()));
    Stage=424;return true;
   }
   if(Stage==424)
   {
    if(Dread.PeekClock()<1.3)return true;
    FS47RoswellWorld Sight;Sight.Init(M);
    if(!Sight.ClearSight(EyeNow,Dread.PeekRaisedHead()))return Fail(TEXT("raised, its head must be in a clear line"));
    DreadLookAt(M,Dread.PeekRaisedHead());
    if(M->FieldRadio->RadioLevelNow()<=0)return Fail(TEXT("the carried radio must hear it"));
    UE_LOG(LogTemp,Display,TEXT("S47: peek: up, exposure %.2f, radio %.3f"),Dread.PeekExposure(),M->FieldRadio->RadioLevelNow());
    DreadAt=Elapsed;Stage=425;return true;
   }
   if(Stage==425)
   {
    if(Elapsed-DreadAt<.4)return true;
    Shot(TEXT("peek-up.png"));M->bPausedByMenu=true;DreadClock=Dread.PeekClock();DreadAt=Elapsed;Stage=426;return true;
   }
   if(Stage==426)
   {
    if(Elapsed-DreadAt<2.5)return true;
    M->bPausedByMenu=false;
    if(FMath::Abs(Dread.PeekClock()-DreadClock)>.001)return Fail(TEXT("a menu must hold it"));
    DreadAt=Elapsed;Stage=427;return true;
   }
   if(Stage==427)
   {
    if(Dread.PeekActive())
    {
     if(Elapsed-DreadAt>25)return Fail(TEXT("it must go"));
     DreadLookAt(M,Dread.PeekRaisedHead());
     return true;
    }
    if(!M->Freeform->HasFinding(FString::Printf(TEXT("peek-%d"),M->Freeform->Day())))return Fail(TEXT("what was seen must be in the field records"));
    UE_LOG(LogTemp,Display,TEXT("S47: peek PASS in %s, %s, %d lost draws"),*DreadPlace,Dread.PeekIsWindow()?TEXT("window"):TEXT("corner"),DreadRound);
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   if(Stage==440)
   {
    if(Elapsed-DreadFrom<1.5)return true;
    if(!Tk.Begin(ES47Taker::Roswell,TEXT("the old house")))return Fail(TEXT("the first light must start"));
    if(Tk.Stage()!=FS47Taken::EStage::Dread)return Fail(TEXT("under a roof it must begin overhead"));
    DreadPos=FVector(M->Player->Pos.X,0,M->Player->Pos.Z);DreadAt=Elapsed;Stage=441;return true;
   }
   if(Stage==441||Stage==446)
   {
    // Into the next room as the light comes.
    if(Tk.Stage()==FS47Taken::EStage::Flood)
    {
     const FVector Aside=S47FarmHouse::Point(3,6.5)+FVector(-24,0,300);M->Player->Place(Aside.X,Aside.Z,0);
     DreadAt=Elapsed;++Stage;return true;
    }
    if(!Tk.Busy())return Fail(TEXT("no way in for the light in this room"));
    if(Elapsed-DreadAt>30)return Fail(TEXT("the light must come"));
    return true;
   }
   if(Stage==442)
   {
    if(Tk.Stage()==FS47Taken::EStage::Lift||Tk.Stage()==FS47Taken::EStage::Away)return Fail(TEXT("the next room must be a way out"));
    if(Tk.Busy()){if(Elapsed-DreadAt>40)return Fail(TEXT("the light must give up"));return true;}
    if(!Dread.HasFloodRoom())return Fail(TEXT("the room must be remembered"));
    if(FVector2D::Distance(FVector2D(Dread.FloodRoomAt().X,Dread.FloodRoomAt().Z),FVector2D(DreadPos.X,DreadPos.Z))>2)return Fail(TEXT("the room remembered must be the one it found them in"));
    DreadClock=Dread.FloodRoomClock();
    M->Player->Place(DreadPos.X,DreadPos.Z,0);DreadAt=Elapsed;Stage=443;
    UE_LOG(LogTemp,Display,TEXT("S47: reflood: out of the light, room remembered"));
    return true;
   }
   if(Stage==443)
   {
    if(Tk.Busy())return Fail(TEXT("it must rest after the first time"));
    if(Elapsed-DreadAt<32)return true;
    if(!DreadToClock(M,21,3))return Fail(TEXT("the reflood test needs three days"));
    UE_LOG(LogTemp,Display,TEXT("S47: reflood: 32 s back in the room straight after, nothing; now %.0f"),M->Freeform->Clock());
    DreadAt=Elapsed;Stage=444;return true;
   }
   if(Stage==444)
   {
    if(Tk.Busy())return Fail(TEXT("not at 21:00"));
    if(Elapsed-DreadAt<32)return true;
    if(!DreadToClock(M,2.5))return Fail(TEXT("the reflood test needs the small hours"));
    if(M->Freeform->Clock()<DreadClock+FS47IndoorDread::FloodRestSeconds)return Fail(TEXT("test clock short of the rest"));
    UE_LOG(LogTemp,Display,TEXT("S47: reflood: 32 s in the room at 21:00, nothing"));
    DreadRound=0;DreadAt=Elapsed;Stage=445;return true;
   }
   if(Stage==445)
   {
    if(Tk.Busy()){UE_LOG(LogTemp,Display,TEXT("S47: reflood: back, on the room's own draw, after %d lost draws"),DreadRound);DreadAt=Elapsed;Stage=446;return true;}
    if(Dread.FloodRoomRollDay()==M->Freeform->Day())
    {
     if(++DreadRound>14)return Fail(TEXT("fourteen nights without the draw: the odds are wrong"));
     if(!M->Freeform->TestAdvance(86400.))return Fail(TEXT("no next night"));
     DreadAt=Elapsed;return true;
    }
    if(Elapsed-DreadAt>60)return Fail(TEXT("the room's own draw must come"));
    return true;
   }
   if(Stage==447)
   {
    if(Tk.Stage()==FS47Taken::EStage::Lift||Tk.Stage()==FS47Taken::EStage::Away)return Fail(TEXT("the next room must still be a way out"));
    if(Tk.Busy()){if(Elapsed-DreadAt>40)return Fail(TEXT("the light must give up again"));return true;}
    if(Dread.FloodsSurvived()!=2||Dread.FloodRoomClock()<=DreadClock)return Fail(TEXT("the second time must count and restart the rest"));
    if(!M->Freeform->HasFinding(FString::Printf(TEXT("flood-room-%d-2"),M->Freeform->Day())))return Fail(TEXT("the second time must leave a record"));
    UE_LOG(LogTemp,Display,TEXT("S47: reflood PASS"));
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   return Fail(TEXT("no such dread stage"));
  }
  bool Step(float Dt)
  {
   Elapsed+=Dt;
   if(!Main.IsValid()) return Fail(TEXT("main disappeared"));
   auto* M=Main.Get();
   if(Elapsed-StageAt>45) return Fail(TEXT("menu click did not reach its expected next screen"));
   if(Stage!=77&&Stage!=212&&!(Stage>=400&&Stage<530)&&!(Stage>=540&&Stage<600&&Stage!=599)&&Elapsed-StageAt<1) return true;
   if(Stage>=400&&Stage<500)return StepDread(M);
   if(Stage>=500&&Stage<530)return StepAfter(M);
   if(Stage>=540&&Stage<560)return StepDoor(M);
   if(Stage>=560&&Stage<590)return StepAttention(M);
   const FString Label=bQuit?TEXT("QUIT"):TEXT("NEW FREEFORM");
   if(Stage==0)
   {
    FSlateApplication::Get().ProcessApplicationActivationEvent(true);
    if(ClickText(TEXT("PRESS ANY KEY")) || ClickText(Label)){Stage=1;StageAt=Elapsed;} // Ordinary title gate, then the visible menu.
    return true;
   }
   if(Stage==1)
   {
    if(!M->ModalOpen()) ClickText(Label); // Second click selects it after the gate, if necessary.
    Stage=bQuit?4:2;StageAt=Elapsed;
    return true;
   }
   if(Stage==2)
   {
    if(ClickText(bLethal?TEXT("Death / reload last save"):TEXT("Local recovery / recommended")))
    {Stage=bFullCases?5:3;StageAt=Elapsed;}
    return true;
   }
   if(Stage==5)
   {
    if(M->Mode==US47Main::EMode::Play) return Fail(TEXT("full-cases fixture unexpectedly bypassed replacement choice"));
    if(ClickText(TEXT("CASE 1"))){Stage=6;StageAt=Elapsed;}
    return true;
   }
   if(Stage==6)
   {
    if(ClickText(TEXT("START NEW FREEFORM"))){Stage=3;StageAt=Elapsed;}
    return true;
   }
   if(Stage==3)
   {
    if(M->Mode!=US47Main::EMode::Play) return true;
    if(!M->IsFreeform() || !M->Freeform || !M->Freeform->Active()) return Fail(TEXT("menu started a non-Freeform session"));
    if(M->Freeform->Lethal()!=bLethal || M->Freeform->Day()!=1 || M->Freeform->Jobs().Num()==0 || M->ModalOpen())
     return Fail(TEXT("Freeform consequences, daily work or closed-menu state is incorrect"));
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerEyes")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     if(!M->Freeform->TestAdvance(13.*3600.))return Fail(TEXT("the eye test needs the night"));
     M->Player->Place(3.18,3.7,0);M->Player->Pitch=-.2;
     Stage=99;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerVisit")))
    {
     M->Flow->bDevRun=true;VisitFrom=Elapsed;
     if(!M->Freeform->TestAdvance(13.*3600.))return Fail(TEXT("the visit test needs the night"));
     if(FParse::Param(FCommandLine::Get(),TEXT("S47VisitTown")))
     {
      M->World->PlaceTruck(TEXT("diner"));M->World->Enter(TEXT("diner"));Stage=139;VisitFrom=Elapsed;StageAt=Elapsed;return true;
     }
     M->Freeform->FarmSite->ArmFarm(M->Freeform->Clock());
     const FVector C=M->Freeform->FarmSite->Centre();M->Player->Place(C.X+8,C.Z+8,0);
     VisitClock=M->Freeform->Clock();M->Freeform->FarmSite->TestVisit();Stage=140;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerTaken")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     if(!M->Freeform->TestAdvance(13.*3600.))return Fail(TEXT("the taken test needs the night"));
     FParse::Value(FCommandLine::Get(),TEXT("S47TakenKind="),TakenKind);TakenEscape=FParse::Param(FCommandLine::Get(),TEXT("S47TakenEscape"));
     FParse::Value(FCommandLine::Get(),TEXT("S47TakenPlace="),TakenPlace);
     if(!TakenPlace.IsEmpty())
     {
      M->World->PlaceTruck(TEXT("diner"));M->World->Enter(TEXT("diner"));VisitFrom=Elapsed;Stage=149;StageAt=Elapsed;return true;
     }
     M->Freeform->FarmSite->ArmFarm(M->Freeform->Clock());
     // Inside the old house for the light through its windows, out on the pasture for the crab.
     const FVector C=M->Freeform->FarmSite->Centre();const FVector In=S47FarmHouse::Point(2.5,2.4)+FVector(-24,0,300);
     if(TakenKind==1)M->Player->Place(C.X+8,C.Z+8,0);else M->Player->Place(In.X,In.Z,0);
     M->Player->Yaw=0;M->Player->Pitch=0;
     VisitClock=M->Freeform->Clock();VisitFrom=Elapsed;TakenPos=FVector(M->Player->Pos.X,0,M->Player->Pos.Z);
     M->Freeform->FarmSite->TestVisit(TakenKind);Stage=150;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerCows")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     M->Player->Place(0,-1.2,PI);M->Player->Pitch=-.05;CowFrom=Elapsed;Stage=160;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerChain")))
    {
     M->Flow->bDevRun=true;
     if(!M->Freeform->TestAdvance(37.*3600.))return Fail(TEXT("the chain needs night two"));
     M->Freeform->TestEndFirstShift();M->Freeform->Director->TestLightsTonight();ChainFrom=Elapsed;Stage=170;StageAt=Elapsed;
     UE_LOG(LogTemp,Display,TEXT("S47: chain: night two, day %d, clock %.0f, area %s"),M->Freeform->Day(),M->Freeform->Clock(),*M->Web->Area());
     return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerPanels")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();M->Freeform->TestEndFirstShift();
     M->Freeform->Field->AddItem(TEXT("relay"),3);M->Freeform->Field->AddItem(TEXT("fuse"),2);
     Stage=180;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerScreens")))
    {
     // PC-WORLD-DISPLAY-20261105-04 and MODE-03B, from the real first shift. PASS only if: all four screens fit the glass and
     // carry the NEXT line the HUD's objective gives; OPS owns the first step, tracking the next. Shots: screens-ops.png,
     // screens-tracking.png, panel-bench-record.png. Legibility across the room is for the eye.
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     Stage=620;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerStare")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     if(!M->Freeform->TestAdvance(13.*3600.))return Fail(TEXT("the stare needs the night"));
     M->Freeform->FarmSite->SetCalm(1e12);M->Freeform->FarmSite->ArmFarm(M->Freeform->Clock());
     const FVector C=M->Freeform->FarmSite->Centre();M->Player->Place(C.X+6,C.Z+18,0);
     CowFrom=Elapsed;Stage=190;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerField")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     M->Freeform->Field->TestService(TEXT("S-08"),3);Stage=120;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerRoad")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     if(!M->Freeform->TestAdvance(13.*3600.))return Fail(TEXT("the road test needs the night"));
     M->World->Enter(TEXT("road"));Stage=80;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerOffice")))
    {
     // PLAY-31-01 / FIX-04: read-only look at the motel office from inside and out, and at the pool fence. -S47OfficeHour=N sets the hour (default: as it starts).
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     int32 At=0;if(FParse::Value(FCommandLine::Get(),TEXT("S47OfficeHour="),At)&&!M->Freeform->TestAdvance(FMath::Fmod(At*3600.-FMath::Fmod(M->Freeform->Clock(),86400.)+86400.,86400.)))return Fail(TEXT("the office test could not set the hour"));
     M->World->Enter(TEXT("saro"));Stage=699;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerBuildings")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     int32 At=0;if(FParse::Value(FCommandLine::Get(),TEXT("S47BuildingsHour="),At)&&!M->Freeform->TestAdvance(FMath::Fmod(At*3600.-FMath::Fmod(M->Freeform->Clock(),86400.)+86400.,86400.)))return Fail(TEXT("the buildings test could not set the hour"));
     // Experiments on the ground's colour: a second day, or a trip to another area first.
     if(FParse::Param(FCommandLine::Get(),TEXT("S47BuildingsDay2"))&&!M->Freeform->TestAdvance(86400.))return Fail(TEXT("the buildings test could not reach the second day"));
     if(FParse::Param(FCommandLine::Get(),TEXT("S47BuildingsTrip"))){M->World->Enter(TEXT("road"));Stage=199;StageAt=Elapsed;return true;}
     Stage=200;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerHouse")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();Stage=70;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerTown")))
    {
     M->Flow->bDevRun=true;
     if(!M->Freeform->TestAdvance(13.*3600.))return Fail(TEXT("the town test needs the night"));
     M->World->PlaceTruck(TEXT("diner"));M->World->Enter(TEXT("diner"));Stage=60;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerNature")))
    {
     // The rules of nature in Freeform: day shots at eleven from six places, and the pasture at night.
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     const double Today=FMath::Fmod(M->Freeform->Clock(),86400.);
     if(!M->Freeform->TestAdvance(FMath::Fmod(11*3600.-Today+86400.,86400.)))return Fail(TEXT("the nature test needs the day"));
     Stage=300;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerFarm")))
    {
     M->Flow->bDevRun=true;
     if(!M->Freeform->TestAdvance(13.*3600.))return Fail(TEXT("the farm test needs the night"));
     FaceFarm(M);Stage=49;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerKinds")))
    {
     // Night two at 21:00, at marks the lights left, the player on foot beside them and carrying the radio.
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();M->Freeform->TestEndFirstShift();
     if(!M->Freeform->TestAdvance(37.*3600.))return Fail(TEXT("the kinds test needs night two"));
     M->FieldRadio->TestOwn();M->Freeform->FarmSite->ArmFarm(M->Freeform->Clock());M->Freeform->FarmSite->TestReadyVisit();
     const FVector Marks=M->Freeform->FarmSite->Centre();M->Player->Place(Marks.X+6,Marks.Z+6,0);M->Player->Pitch=0;
     DreadFrom=Elapsed;Stage=400;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerDoor")))
    {
     // Night two at 21:00, indoors with a shut door near. See StepDoor for what is set up and what is not.
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();M->Freeform->TestEndFirstShift();
     if(!M->Freeform->TestAdvance(37.*3600.))return Fail(TEXT("the door test needs night two"));
     M->FieldRadio->TestOwn();DreadPlace=TEXT("house");FParse::Value(FCommandLine::Get(),TEXT("S47DoorPlace="),DreadPlace);
     if(DreadPlace!=TEXT("house")){M->World->PlaceTruck(TEXT("diner"));M->World->Enter(TEXT("diner"));}
     DreadFrom=Elapsed;Stage=540;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerAttention")))
    {
     // Night two at 02:30, on foot at marks the lights left. See StepAttention for what is set up and what is not.
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();M->Freeform->TestEndFirstShift();
     if(!M->Freeform->TestAdvance(42.5*3600.))return Fail(TEXT("the attention test needs night two"));
     M->FieldRadio->TestOwn();auto& Site=*M->Freeform->FarmSite;Site.ArmFarm(M->Freeform->Clock());Site.TestRollWho(.9f);Site.TestHoldHunt();Site.TestReadyVisit();
     const FVector Marks=Site.Centre();M->Player->Place(Marks.X+6,Marks.Z+6,0);M->Player->Pitch=0;
     DreadAt=Elapsed;Stage=560;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerPeek")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();M->Freeform->TestEndFirstShift();
     M->Freeform->IndoorDread->Door().TestDice(1.f); // This test is about the look-in: no door visit in the way.
     if(!M->Freeform->TestAdvance(37.*3600.))return Fail(TEXT("the peek test needs night two"));
     M->FieldRadio->TestOwn();DreadPlace=TEXT("house");FParse::Value(FCommandLine::Get(),TEXT("S47PeekPlace="),DreadPlace);
     if(DreadPlace!=TEXT("house")&&DreadPlace!=TEXT("saro")){M->World->PlaceTruck(TEXT("diner"));M->World->Enter(TEXT("diner"));}
     DreadFrom=Elapsed;Stage=420;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerNight")))
    {
     // The Director stays on. From 19:00 of night two, in the control room, the world and scheduler clocks run fast.
     M->Flow->bDevRun=true;M->Freeform->TestEndFirstShift();
     if(!M->Freeform->TestAdvance(35.*3600.))return Fail(TEXT("the night test needs night two"));
     M->Player->Place(0,-1.2,0);M->Player->Pitch=0;
     M->Freeform->Director->TestSchedule(-1.f,25.2,40.);
     DreadFirst=0;DreadRound=0;DreadFrom=DreadAt=Elapsed;Stage=460;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerRest")))
    {
     // The Director stays on. 22:00 of night two, marks standing, nothing else drawn.
     M->Flow->bDevRun=true;M->Freeform->TestEndFirstShift();M->Freeform->TestAllowWake();
     if(!M->Freeform->TestAdvance(38.*3600.))return Fail(TEXT("the rest test needs night two"));
     M->Player->Place(0,-1.2,0);M->Player->Pitch=0;
     M->Freeform->Director->TestSchedule(0.f,-1,1);
     M->Freeform->FarmSite->ArmFarm(M->Freeform->Clock());M->Freeform->FarmSite->TestReadyVisit();
     DreadRound=0;DreadFrom=DreadAt=Elapsed;Stage=480;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerPlanner")))
    {
     // The Director stays on. 22:00 of night two, in the control room, the lights drawn for 22:00 and nothing else.
     M->Flow->bDevRun=true;M->Freeform->TestEndFirstShift();
     if(!M->Freeform->TestAdvance(38.*3600.))return Fail(TEXT("the planner test needs night two"));
     M->Player->Place(0,-1.2,0);M->Player->Pitch=0;
     M->Freeform->Director->TestSchedule(0.f,22.,40.);
     DreadFrom=DreadAt=Elapsed;Stage=470;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerAfter")))
    {
     // Night two at 02:30, on foot at marks the lights left. The crab comes by the visit's own dice.
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();M->Freeform->TestEndFirstShift();
     if(M->Freeform->Aftermath->Save()->Values.Num()>0||M->Freeform->Aftermath->PcStuck())return Fail(TEXT("nothing has happened yet, so nothing may be wrong"));
     if(M->TownLayer&&!M->TownLayer->Herds.IsEmpty()&&M->TownLayer->Herds[0]->Missing!=-1)return Fail(TEXT("the herd is whole until something happens"));
     if(!M->Freeform->TestAdvance(42.5*3600.))return Fail(TEXT("the aftermath test needs night two"));
     M->Freeform->Aftermath->TestDice(0.f,0.f);
     auto& Site=*M->Freeform->FarmSite;Site.ArmFarm(M->Freeform->Clock());Site.TestRollWho(.05f);Site.TestReadyVisit();
     const FVector Marks=Site.Centre();M->Player->Place(Marks.X+6,Marks.Z+6,0);M->Player->Pitch=0;
     AfterAt=Elapsed;Stage=500;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerCarLight")))
    {
     // 03:00, in the open with the car behind them, so that it may appear. Everything else the Director could draw is off.
     M->Flow->bDevRun=true;M->Freeform->TestEndFirstShift();
     if(!M->Freeform->TestAdvance(43.*3600.))return Fail(TEXT("the car test needs the small hours"));
     auto& D=*M->Freeform->Director;D.TestSchedule(0.f,-1,1.);D.TestCarDice(0.f);
     M->Player->Place(27,24,PI);
     AfterAt=Elapsed;Stage=519;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerMystery")))
    {
     // The Director stays on, its own draws set to no but for the mystery's. 19:30 of night two, in the control room.
     M->Flow->bDevRun=true;M->Freeform->TestEndFirstShift();
     if(!M->Freeform->TestAdvance(35.5*3600.))return Fail(TEXT("the mystery needs night two"));
     M->Player->Place(0,-1.2,0);M->Player->Pitch=0;
     M->Freeform->Director->TestSchedule(0.f,-1,40.);M->Freeform->Director->TestMysteryDice(1.f);
     DreadFrom=DreadAt=Elapsed;Stage=490;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerReflood")))
    {
     // Night two at 02:30, in a room of the old house. The first light is set up; what comes back afterwards is not.
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();M->Freeform->TestEndFirstShift();
     M->Freeform->IndoorDread->Door().TestDice(1.f); // This test is about the light: no door visit in the way.
     if(!M->Freeform->TestAdvance(42.5*3600.))return Fail(TEXT("the reflood test needs night two"));
     const FVector Room=S47FarmHouse::Point(2.5,2.4)+FVector(-24,0,300);M->Player->Place(Room.X,Room.Z,0);M->Player->Pitch=0;
     DreadFrom=Elapsed;Stage=440;StageAt=Elapsed;return true;
    }
    if(bAlbum){GiveAlbum(M);M->UI->NotebookAt(TEXT("photos"));M->Flow->OpenNotebook();Stage=10;StageAt=Elapsed;return true;}
    UE_LOG(LogTemp,Display,TEXT("S47: menu pointer PASS: title -> NEW FREEFORM -> %s -> active day 1 with %d jobs"),bLethal?TEXT("death"):TEXT("local recovery"),M->Freeform->Jobs().Num());
    FPlatformMisc::RequestExitWithStatus(false,0);
    return false;
   }
   // The motel office (PLAY-31-01) and the pool fence (FIX-04), in the saro frame. Floors are the assembly's: reception tiles x -48.1..-43.7, z 672.3..677.5, door at x -43.7, z about 674.1.
   if(Stage==699)
   {
    if(Elapsed-StageAt<5)return true;
    if(M->Web->Area()!=TEXT("saro")){M->World->Enter(TEXT("saro"));return true;}
    S47MotelProbe::LogOffice(M);Stage=700;StageAt=Elapsed;return true;
   }
   if(Stage>=700&&Stage<740)
   {
    struct FOfficeView{const TCHAR* Name;double X,Z,LookX,LookHeight,LookZ;bool bRays;};
    static const FOfficeView Views[]={
     {TEXT("in-west"),-44.6,674.1,-48.0,1.5,674.1,true},{TEXT("in-north"),-44.6,674.1,-44.6,1.5,677.4,true},{TEXT("in-south"),-44.6,674.1,-44.6,1.5,672.4,true},
     {TEXT("in-door"),-44.6,674.1,-41.0,1.4,674.1,true},{TEXT("in-corner-nw"),-47.3,676.5,-44.0,1.3,673.0,true},{TEXT("in-corner-sw"),-47.3,672.9,-44.0,1.3,676.6,true},
     {TEXT("in-ceiling"),-46.0,674.9,-45.0,3.0,674.9,false},{TEXT("in-floor"),-45.5,674.9,-44.8,0.1,674.9,false},
     {TEXT("out-door"),-41.0,674.1,-45.0,1.4,674.1,false},{TEXT("out-door-close"),-43.0,674.1,-46.0,1.4,674.1,false},
     {TEXT("out-oblique-south"),-41.0,672.0,-43.8,1.5,674.1,false},{TEXT("out-oblique-north"),-41.5,677.0,-44.0,1.3,673.0,false},{TEXT("out-wall"),-41.5,672.3,-44.0,1.3,675.5,false},
     {TEXT("pool-approach"),-50.5,684.0,-56.0,1.0,700.0,false},{TEXT("pool-fence-south"),-50.0,689.0,-65.0,0.4,691.0,false},{TEXT("pool-fence-corner"),-52.0,696.0,-54.5,0.5,706.0,false}};
    const int32 I=Stage-700;
    if(I>=int32(UE_ARRAY_COUNT(Views)))
    {
     S47MotelProbe::LogPool(M);
     UE_LOG(LogTemp,Display,TEXT("S47: office PASS: %d views in Saved/AlbumTest/office-*.png; read the S47 office and S47 pool log lines (a diagnostic, not a verdict)"),int32(UE_ARRAY_COUNT(Views)));
     FPlatformMisc::RequestExitWithStatus(false,0);return false;
    }
    if(ShotPending)
    {
     if(Elapsed-StageAt<.8)return true;
     if(Views[I].bRays)S47MotelProbe::LogRays(M,Views[I].Name);
     Shot(*FString::Printf(TEXT("office-%02d-%s.png"),I,Views[I].Name));ShotPending=false;Stage=Stage+1;StageAt=Elapsed;return true;
    }
    if(Elapsed-StageAt<.4)return true;
    const FVector2D D(Views[I].LookX-Views[I].X,Views[I].LookZ-Views[I].Z);
    M->Player->Place(Views[I].X,Views[I].Z,FMath::Atan2(-D.X,-D.Y));M->Player->Pitch=FMath::Atan2(Views[I].LookHeight-1.62,D.Size());ShotPending=true;
    StageAt=Elapsed;return true;
   }
   // The new farm buildings, in the saro frame: a look at each from the yard by day and by night and one inside the
   // barn, then in through its door on foot and on until the back wall stops the walk.
   if(Stage==199)
   {
    if(Elapsed-StageAt<5)return true;
    if(M->Web->Area()!=TEXT("saro")){M->World->Enter(TEXT("saro"));return true;}
    Stage=200;StageAt=Elapsed;return true;
   }
   if(Stage==205)
   {
    if(!M->Freeform->TestAdvance(13.*3600.))return Fail(TEXT("the buildings test needs the night"));
    Stage=206;StageAt=Elapsed;return true;
   }
   if(Stage>=200&&Stage<=210)
   {
    if(!S47FarmWorld::BuildingsShown(M))return Fail(TEXT("the four farm buildings must stand and show; was the import run?"));
    struct FView{const TCHAR* Name;double X,Z,LookX,LookHeight,LookZ;};
    const FVector Aisle=S47FarmBuildings::BarnPoint(1.3,-4),BackOfAisle=S47FarmBuildings::BarnPoint(1.3,4.6);
    const FView Views[]={{TEXT("barn"),137.5,186,136,3,172},{TEXT("house"),125,191,110,1.8,190},{TEXT("shedA"),117,180,116.5,1.5,170},{TEXT("shedB"),90,213,84.5,1.5,206.5},
     {TEXT("barn-inside"),Aisle.X,Aisle.Z,BackOfAisle.X,1.8,BackOfAisle.Z}};
    const bool bNight=Stage>=206;const int32 I=Stage-(bNight?206:200);
    if(ShotPending)
    {
     if(I==4&&!FMath::IsNearlyEqual(M->Player->FloorY,S47FarmBuildings::BarnPoint(0,0).Y,.01))return Fail(TEXT("the barn's floor must be level"));
     {
      const FVector Foot=S47Three::ToUE(M->Player->Pos);FCollisionQueryParams Q;Q.bTraceComplex=true;TArray<FHitResult> Hits;
      M->Web->GetWorld()->LineTraceMultiByChannel(Hits,Foot+FVector(0,0,120),Foot-FVector(0,0,800),ECC_Visibility,Q);
      for(const FHitResult& H:Hits){const auto* C=Cast<UPrimitiveComponent>(H.GetComponent());const auto* SM=Cast<UStaticMeshComponent>(C);const auto* Mat=C?C->GetMaterial(0):nullptr;
       UE_LOG(LogTemp,Display,TEXT("S47: buildings underfoot %s: %s mesh %s material %s"),Views[I].Name,C?*C->GetName():TEXT("none"),SM&&SM->GetStaticMesh()?*SM->GetStaticMesh()->GetName():TEXT("-"),Mat?*Mat->GetName():TEXT("-"));}
     }
     Shot(*FString::Printf(TEXT("buildings-%s-%s.png"),bNight?TEXT("night"):TEXT("day"),Views[I].Name));ShotPending=false;Stage=Stage==210?211:Stage+1;StageAt=Elapsed;return true;
    }
    const FVector O(-24,0,300);const FVector2D D(Views[I].LookX-Views[I].X,Views[I].LookZ-Views[I].Z);
    M->Player->Place(Views[I].X+O.X,Views[I].Z+O.Z,FMath::Atan2(-D.X,-D.Y));M->Player->Pitch=FMath::Atan2(Views[I].LookHeight-1.62,D.Size());ShotPending=true;
    StageAt=Elapsed;return true;
   }
   if(Stage==211)
   {
    if(S47FarmWorld::WindowGlowsAtHouse(M)!=3)return Fail(TEXT("the lit window's three glows must follow the house"));
    const FVector O(-24,0,300);
    WalkTo={S47FarmBuildings::BarnPoint(1.3,-8)+O,S47FarmBuildings::BarnPoint(1.3,-2)+O,S47FarmBuildings::BarnPoint(1.3,7)+O};WalkAt=1;
    M->Player->Place(WalkTo[0].X,WalkTo[0].Z,0);Stage=212;StageAt=Elapsed;WalkFrom=Elapsed;return true;
   }
   if(Stage==212)
   {
    StageAt=Elapsed;
    const FVector V=WalkTo[WalkAt]-M->Player->Pos;const double Left=FVector2D(V.X,V.Z).Size();
    if(WalkAt==1)
    {
     if(Elapsed-WalkFrom>20)return Fail(TEXT("the barn's door must let the player in"));
     if(Left<.3)
     {
      if(!FMath::IsNearlyEqual(M->Player->FloorY,S47FarmBuildings::BarnPoint(0,0).Y,.02))return Fail(TEXT("the barn's floor must carry the player"));
      WalkAt=2;WalkFrom=Elapsed;return true;
     }
    }
    else if(Elapsed-WalkFrom>8)
    {
     // Its road-local z falls toward the back wall. Stopped close to it, and not through it.
     M->Player->TestMove.Reset();
     const double Reached=M->Player->Pos.Z-300;const double WallZ=S47FarmBuildings::BarnPoint(1.3,5.4).Z,NearZ=S47FarmBuildings::BarnPoint(1.3,4.6).Z;
     if(Reached<WallZ)return Fail(TEXT("the barn's back wall must stop the player"));
     if(Reached>NearZ)return Fail(TEXT("the barn's aisle must run clear to the back wall"));
     Shot(TEXT("buildings-night-barn-wall.png"));
     UE_LOG(LogTemp,Display,TEXT("S47: buildings PASS: four buildings shown day and night, glow at the house, in through the barn's door, level floor, stopped %.2f m from the back wall"),Reached-WallZ);
     FPlatformMisc::RequestExitWithStatus(false,0);return false;
    }
    M->Player->Yaw=FMath::Atan2(-V.X,-V.Z);M->Player->Pitch=-.08;M->Player->TestMove=FVector2D(0,-FMath::Min(1.,Left/.4));
    return true;
   }
   // The abandoned house, in the saro frame the farm lies in: a look from each named spot, then the real walk.
   if(Stage>=70&&Stage<=75)
   {
    struct FView{const TCHAR* Name;double X,Y,LookX,LookY;};
    static const FView Views[]={{TEXT("outside"),6,16,6,4},{TEXT("hall"),6,8.3,6,0},{TEXT("bedroom"),4.8,2.25,1,2.25},{TEXT("living"),7.2,6.55,11,6.55},{TEXT("kitchen"),4.8,6.55,1,6.55},{TEXT("back"),8.4,2.25,8.4,1}};
    const int32 I=Stage-70;
    if(ShotPending){Shot(*FString::Printf(TEXT("house-%s.png"),Views[I].Name));ShotPending=false;Stage=I==5?76:Stage+1;StageAt=Elapsed;return true;}
    const FVector O(-24,0,300);const FVector At=S47FarmHouse::Point(Views[I].X,Views[I].Y)+O,Look=S47FarmHouse::Point(Views[I].LookX,Views[I].LookY)+O;const FVector D=Look-At;
    M->Player->Place(At.X,At.Z,FMath::Atan2(-D.X,-D.Z));M->Player->Pitch=0;ShotPending=true;
    if(I>0&&!FMath::IsNearlyEqual(M->Player->FloorY,S47FarmHouse::PadY(),.01))return Fail(TEXT("the floor inside the house must be level"));
    StageAt=Elapsed;return true;
   }
   if(Stage==76)
   {
    // First against the shut front door: it has to stop the player, then opening it has to clear the way.
    const FVector O(-24,0,300);auto& H=*M->Freeform->HouseDoors;
    if(H.Num()!=7||H.IsOpen(H.IndexOf(TEXT("south0"))))return Fail(TEXT("the house has seven doors and its front door starts shut"));
    WalkTo={S47FarmHouse::Point(6,9.6)+O,S47FarmHouse::Point(6,4.5)+O};WalkAt=0;BlockTest=true;
    M->Player->Place(WalkTo[0].X,WalkTo[0].Z-2,0);Stage=77;StageAt=Elapsed;WalkFrom=Elapsed;return true;
   }
   if(Stage==77)
   {
    StageAt=Elapsed;
    if(BlockTest&&Elapsed-WalkFrom>7)
    {
     const FVector Door=S47FarmHouse::Point(6,8.85)+FVector(-24,0,300);
     // The hall is farther into +Z than the door: past it means the shut door failed.
     if(M->Player->Pos.Z>Door.Z+.05)return Fail(TEXT("a shut door must stop the player"));
     M->Player->TestMove.Reset();auto& H=*M->Freeform->HouseDoors;
     const auto Saved=H.Save();
     if(!H.Use(H.IndexOf(TEXT("south0")))||!H.Use(H.IndexOf(TEXT("north0"))))return Fail(TEXT("the doors must open"));
     H.Begin(H.Save());if(!H.IsOpen(H.IndexOf(TEXT("south0"))))return Fail(TEXT("door state must survive a reload"));
     H.Begin(Saved);if(H.IsOpen(H.IndexOf(TEXT("south0"))))return Fail(TEXT("an older saved state must restore"));
     H.Use(H.IndexOf(TEXT("south0")));H.Use(H.IndexOf(TEXT("north0")));
     Shot(TEXT("house-door-open.png"));
     const FVector O(-24,0,300);WalkTo={S47FarmHouse::Point(6,4.5)+O,S47FarmHouse::Point(6,-1)+O};WalkAt=0;BlockTest=false;WalkFrom=Elapsed;return true;
    }
    if(Elapsed-WalkFrom>30)return Fail(TEXT("the hall must be walkable from the front door to the back door"));
    if(BlockTest&&WalkAt>=WalkTo.Num())return Fail(TEXT("a shut door must stop the player"));
    if(WalkAt>=WalkTo.Num())
    {
     M->Player->TestMove.Reset();
     UE_LOG(LogTemp,Display,TEXT("S47: farmhouse PASS: level floor, shut door stopped the player, opened, walked through and out of the back in %.1f s"),Elapsed-WalkFrom);
     FPlatformMisc::RequestExitWithStatus(false,0);return false;
    }
    const FVector V=WalkTo[WalkAt]-M->Player->Pos;const double Left=FVector2D(V.X,V.Z).Size();
    if(Left<.3){++WalkAt;return true;}
    M->Player->Yaw=FMath::Atan2(-V.X,-V.Z);M->Player->Pitch=-.08;M->Player->TestMove=FVector2D(0,-FMath::Min(1.,Left/.4));
    return true;
   }
   // The way out at night, from the road side and from the farm, in the frame each is really seen in.
   if(Stage>=80&&Stage<87)
   {
    struct FV{const TCHAR* Name;const TCHAR* Area;double X,Z,LX,LZ;double Lift;};
    const FVector Sign=FS47FarmNight::SignWorld();
    const FV V[]={{TEXT("turnoff-far"),TEXT("road"),8001,110,8007,196,6},{TEXT("turnoff-near"),TEXT("road"),8002,178,8007,196,3},{TEXT("sign"),TEXT("road"),Sign.X,Sign.Z-24,Sign.X,Sign.Z,3},{TEXT("road-in"),TEXT("saro"),-16,497,16,497,0},{TEXT("road-mid"),TEXT("saro"),30,501,70,497,0},{TEXT("road-end"),TEXT("saro"),100,503,60,497,0},{TEXT("trafo-entry"),TEXT("road"),8003,460,7997,520,4}};
    const int32 I=Stage-80;
    if(ShotPending){Shot(*FString::Printf(TEXT("road-%s.png"),V[I].Name));ShotPending=false;Stage=I==6?90:Stage+1;StageAt=Elapsed;return true;}
    if(M->Web->Area()!=V[I].Area){M->World->Enter(V[I].Area);StageAt=Elapsed;return true;}
    const FVector D(V[I].LX-V[I].X,V[I].Lift,V[I].LZ-V[I].Z);M->Player->Place(V[I].X,V[I].Z,FMath::Atan2(-D.X,-D.Z));M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());ShotPending=true;StageAt=Elapsed;
    return true;
   }
   if(Stage==90)
   {
    if(!M->Freeform->FarmNight->LampsOn())return Fail(TEXT("the lamps must be lit at night"));
    M->World->Enter(TEXT("saro"));const FVector C=FVector(84,0,492);M->Player->Place(C.X,C.Z,0);Stage=91;StageAt=Elapsed;WalkFrom=Elapsed;return true;
   }
   if(Stage==91)
   {
    StageAt=Elapsed;
    if(Elapsed-WalkFrom<45&&M->Freeform->FarmNight->FarmSounds()<2)return true;
    if(M->Freeform->FarmNight->FarmSounds()<2)return Fail(TEXT("the farm must make sounds of its own"));
    if(!M->Freeform->TestAdvance(12.*3600.))return Fail(TEXT("advancing to day"));
    Stage=92;WalkFrom=Elapsed;return true;
   }
   if(Stage==92)
   {
    StageAt=Elapsed;if(Elapsed-WalkFrom<1)return true;
    if(M->Freeform->FarmNight->LampsOn())return Fail(TEXT("the lamps must be off by day"));
    UE_LOG(LogTemp,Display,TEXT("S47: farm road PASS: lamps lit at night and off by day, %d farm sounds played"),M->Freeform->FarmNight->FarmSounds());
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   // The nature: the pasture, the roadside south of SARO, flat desert towards the mountains, the antenna field, the edge of Caliche and
   // STATION 01 by day, then the pasture at night. Each view waits for the plants to fill in before its picture.
   if(Stage>=300&&Stage<=312)
   {
    struct FN{const TCHAR* Name;const TCHAR* Area;double X,Z,LX,LZ,Lift;};
    const auto Saro=[](double X,double Z){return S47FarmWorld::RoadToArea(TEXT("saro"),FVector(X,0,Z));};
    const FVector Past0=Saro(70,240),Past1=Saro(76,251);
    const double Edge=S47RoadShape::TOWN().Z0;
    const FN V[]={{TEXT("pasture-day"),TEXT("saro"),Past0.X,Past0.Z,Past1.X,Past1.Z,0},
     {TEXT("roadside"),TEXT("road"),8010,40,8013,140,0},
     {TEXT("flat-desert"),TEXT("road"),8070,300,8300,300,12},
     {TEXT("antenna-field"),TEXT("saro"),-1,-30,-1,-110,0},
     {TEXT("caliche-edge"),TEXT("diner"),7995,Edge-50,7995,Edge+30,0},
     {TEXT("station01"),TEXT("station01"),-4,8014,-11,8003,0},
     {TEXT("pasture-night"),TEXT("saro"),Past0.X,Past0.Z,Past1.X,Past1.Z,0},
     {TEXT("yard"),TEXT("saro"),Saro(125,195).X,Saro(125,195).Z,Saro(110,190).X,Saro(110,190).Z,0},
     {TEXT("east-shed"),TEXT("saro"),Saro(155,174).X,Saro(155,174).Z,Saro(142,174).X,Saro(142,174).Z,0},
     {TEXT("saro-yard"),TEXT("saro"),-6,20,-30,20,0}};
    static double NatureJumpAt=-1;
    if(NatureJumpAt>=0)
    {
     if(Elapsed<NatureJumpAt)return true;
     NatureJumpAt=-1;if(!M->Freeform->TestAdvance(12*3600.))return Fail(TEXT("the nature test needs twelve more hours"));
     StageAt=Elapsed;return true;
    }
    const int32 I=Stage>=310?Stage-303:Stage-300;
    StageAt=Elapsed;
    if(ShotPending)
    {
     if(Elapsed-WalkFrom<4)return true;
     Shot(*FString::Printf(TEXT("nature-%s.png"),V[I].Name));ShotPending=false;
     UE_LOG(LogTemp,Display,TEXT("S47: nature frame %s: %.1f ms over the last frames"),V[I].Name,FApp::GetDeltaTime()*1000.);
     {
      // Which surface is underfoot, by name, since a shared look can hide which mesh actually shows.
      const FVector Foot=S47Three::ToUE(M->Player->Pos);FHitResult Hit;FCollisionQueryParams Q;Q.bTraceComplex=true;
      const bool bHit=M->Web->GetWorld()->LineTraceSingleByChannel(Hit,Foot+FVector(0,0,300),Foot-FVector(0,0,800),ECC_Visibility,Q);
      const auto* C=bHit?Cast<UPrimitiveComponent>(Hit.GetComponent()):nullptr;const auto* Mat=C?C->GetMaterial(0):nullptr;const auto* SM=Cast<UStaticMeshComponent>(C);
      UE_LOG(LogTemp,Display,TEXT("S47: nature underfoot %s: %s mesh %s material %s"),V[I].Name,C?*C->GetName():TEXT("none"),SM&&SM->GetStaticMesh()?*SM->GetStaticMesh()->GetName():TEXT("-"),Mat?*Mat->GetName():TEXT("-"));
     }
     // A shot is taken at the end of the frame after this one: the clock waits for it, or the picture shows the next hour.
     if(I==5){NatureJumpAt=Elapsed+1;Stage=306;}
     else if(I==6){NatureJumpAt=Elapsed+1;Stage=310;}
     else Stage=I==9?320:Stage+1;
     return true;
    }
    if(M->Web->Area()!=V[I].Area){M->World->Enter(V[I].Area);return true;}
    const FVector D(V[I].LX-V[I].X,V[I].Lift,V[I].LZ-V[I].Z);
    M->Player->Place(V[I].X,V[I].Z,FMath::Atan2(-D.X,-D.Z));M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
    ShotPending=true;WalkFrom=Elapsed;
    return true;
   }
   // The desert plants: first what the planner itself drew round the flat desert (the views above show it too, and none of it is forced), then
   // one copy of each of the eight at scale 1 on an arc 15 m before the eye, from there, from 55 m back (the far LODs) and from 6 m.
   if(Stage>=320&&Stage<=323)
   {
    StageAt=Elapsed;
    if(M->Web->Area()!=TEXT("road")){M->World->Enter(TEXT("road"));return true;}
    FS47Flora* RoadPatch=nullptr;
    if(M->Life)for(const TSharedPtr<FS47Flora>& Patch:M->Life->Flora)if(Patch&&Patch->Spec.Name==TEXT("road"))RoadPatch=Patch.Get();
    if(!RoadPatch)return Fail(TEXT("the desert plants' view needs the road's plants"));
    const double RowX=8070,RowZ=300,RowStep[]={0,0,-55,9};
    const double EyeX=RowX+RowStep[Stage-320];
    if(ShotPending)
    {
     if(Elapsed-WalkFrom<4)return true;
     ShotPending=false;
     if(Stage==320)
     {
      const TMap<ES47FloraKind,int32> Counted=RoadPatch->Drawn();
      const int32 NatYucca=Counted.FindRef(ES47FloraKind::Yucca),NatCholla=Counted.FindRef(ES47FloraKind::Cholla),NatMesquite=Counted.FindRef(ES47FloraKind::Mesquite),NatOld=Counted.FindRef(ES47FloraKind::Succulent);
      UE_LOG(LogTemp,Display,TEXT("S47: nature desert plants as the planner drew them round the flat desert: %d meshes in use, yucca %d, cholla %d, mesquite %d, old succulents %d"),RoadPatch->DesertInUse(),NatYucca,NatCholla,NatMesquite,NatOld);
      if(RoadPatch->DesertInUse()==0){UE_LOG(LogTemp,Display,TEXT("S47: nature: no desert plants imported, the old succulents stay"));Stage=324;return true;}
      if(NatOld>0)return Fail(TEXT("the old succulents must be gone where the desert plants stand"));
      if(NatYucca<=0||NatCholla<=0||NatMesquite<=0)return Fail(TEXT("the planner must draw each desert species by itself"));
      UE_LOG(LogTemp,Display,TEXT("S47: nature desert arc: %d plants stood"),RoadPatch->TestDesertArc(FVector(RowX,0,RowZ),FVector(RowX+15,0,RowZ),15));
     }
     else Shot(*FString::Printf(TEXT("nature-desert-%s.png"),Stage==321?TEXT("row"):Stage==322?TEXT("far"):TEXT("near")));
     ++Stage;
     return true;
    }
    M->Player->Place(EyeX,RowZ,FMath::Atan2(-15.,0.));M->Player->Pitch=FMath::Atan2(.4,15.);
    ShotPending=true;WalkFrom=Elapsed;
    return true;
   }
   if(Stage==324)
   {
    UE_LOG(LogTemp,Display,TEXT("S47: nature shown"));
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   if(Stage==99)
   {
    if(Elapsed-StageAt<3)return true;
    if(M->Night->Glimpses->DevSpawn(1,FS47RoswellBrain::EMode::Stand)!=1)return Fail(TEXT("the eye test needs one body"));
    Stage=100;StageAt=Elapsed;return true;
   }
   // Right up to the face, then out in the open at night from a distance, the player's own height and no torch.
   if(Stage>=100&&Stage<=108)
   {
    UPoseableMeshComponent* Skin=nullptr;
    for(TObjectIterator<UPoseableMeshComponent> It;It;++It)if(It->GetWorld()==M->Web->GetWorld()&&It->IsVisible()&&It->GetName().Contains(TEXT("Roswell")))Skin=*It;
    if(!Skin)return Fail(TEXT("no visible Roswell body"));
    const FVector Head=S47Three::ToThree(Skin->GetBoneLocationByName(TEXT("head"),EBoneSpaces::WorldSpace));
    const FVector Face=(S47Three::ToThree(Skin->GetBoneLocationByName(TEXT("head"),EBoneSpaces::WorldSpace)+Skin->GetRightVector()*100)-Head).GetSafeNormal();
    auto Look=[&](double Away)
    {
     const FVector At=Head+Face*Away;M->Player->Place(At.X,At.Z,0);
     M->Player->Eye=Away<1?Head.Y-M->Player->FloorY+.08:1.62;
     const FVector Eye(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z),D=Head+FVector(0,Away<1?.08:Away*.03,0)-Eye; // From afar, aim a little over the head so the crosshair does not sit on the eyes.
     M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
    };
    switch(Stage)
    {
     case 100:Look(.5);break;
     case 101:Shot(TEXT("eyes-close.png"));break;
     case 102:M->Night->Glimpses->DevClear();M->Player->Eye=1.62;M->Player->Place(0,30,0);M->Player->Pitch=-.05;Stage=109;StageAt=Elapsed;return true;
     case 103:Look(10);break;
     case 104:Shot(TEXT("eyes-10m.png"));break;
     case 105:Look(25);break;
     case 106:Shot(TEXT("eyes-25m.png"));break;
     case 107:
      if(FS47TripoCreature::ShownEyeGlints()<2)return Fail(TEXT("the eyes must show at 25 m at night"));
      Look(60);Stage=110;StageAt=Elapsed;return true;
    }
    ++Stage;StageAt=Elapsed;return true;
   }
   if(Stage==149)
   {
    StageAt=Elapsed;
    if(Elapsed-VisitFrom<2.5)return true;
    FVector In;
    if(!S47CalicheAccess::RoomMiddle(TakenPlace,In))return Fail(TEXT("no such room"));
    const FVector P=M->Freeform->Director->LightPlacePoint(TakenPlace);
    M->Freeform->FarmSite->Arm(M->Freeform->Clock(),TakenPlace,TEXT("the place"),TEXT("diner"),P.IsZero()?In:P);
    M->Player->Place(In.X,In.Z,0);M->Player->Pitch=0;
    UE_LOG(LogTemp,Display,TEXT("S47: taken test in %s at %s"),*TakenPlace,*In.ToCompactString());
    VisitClock=M->Freeform->Clock();VisitFrom=Elapsed;TakenPos=FVector(M->Player->Pos.X,0,M->Player->Pos.Z);
    M->Freeform->FarmSite->TestVisit(TakenKind);Stage=150;return true;
   }
   // Left where they stand: each kind must take them, and leave them where and when its terms say.
   if(Stage>=150&&Stage<=154)
   {
    StageAt=Elapsed;
    auto& Tk=*M->Freeform->Taken;auto& Site=*M->Freeform->FarmSite;
    // Walls can hold, in a town room: staying put and unseen is one way for it to pass.
    if(!TakenPlace.IsEmpty()&&Site.VisitStage()==4&&!Tk.Busy()&&Tk.LastNote().IsEmpty())
    {UE_LOG(LogTemp,Display,TEXT("S47: taken PASS in %s: the walls held"),*TakenPlace);FPlatformMisc::RequestExitWithStatus(false,0);return false;}
    if(Elapsed-VisitFrom>170)return Fail(TEXT("the taker must take a player who stays"));
    // The way out: stand out of the light, and it passes with nothing lost.
    if(TakenEscape)
    {
     if(Tk.Stage()==FS47Taken::EStage::Flood&&!TakenShot[1])
     {
      TakenShot[1]=true;const FVector Aside=S47FarmHouse::Point(3,6.5)+FVector(-24,0,300);M->Player->Place(Aside.X,Aside.Z,0);
      UE_LOG(LogTemp,Display,TEXT("S47: taken test: out of the light, into the next room"));return true;
     }
     if(TakenShot[1]&&!Tk.Busy()&&Site.VisitStage()!=4)
     {
      // The body is still about; this test is the light. Done with the visit.
      if(M->Freeform->Clock()>VisitClock+10*60)return Fail(TEXT("getting out of the light must cost no time"));
      UE_LOG(LogTemp,Display,TEXT("S47: taken PASS: out of the light, %.1f minutes passed"),(M->Freeform->Clock()-VisitClock)/60.);
      FPlatformMisc::RequestExitWithStatus(false,0);return false;
     }
     return true;
    }
    {static double NextLog=0;if(Elapsed>=NextLog){NextLog=Elapsed+5;UE_LOG(LogTemp,Display,TEXT("S47: taken watch, stage %d, visit %d, in light %.1f, body %.1f m, indoors %d"),int32(Tk.Stage()),Site.VisitStage(),Tk.LightOn(),Site.VisitBodyDistance(),Tk.Indoors());}}
    if(Stage==150)
    {
     if(Tk.Stage()==FS47Taken::EStage::Dread&&!TakenShot[0]&&Elapsed-VisitFrom>3){TakenShot[0]=true;Shot(TEXT("taken-dread.png"));return true;}
     if(Tk.Stage()==FS47Taken::EStage::Flood&&!TakenShot[1])
     {
      TakenShot[1]=true;TakenAt=Elapsed;
     }
     if(TakenShot[1]&&!TakenShot[2]&&Tk.Stage()==FS47Taken::EStage::Flood&&Elapsed-TakenAt>1.2)
     {
      // Look at where it comes in, then round at the room it lights.
      const FVector D=Tk.LightFrom()-FVector(M->Player->Pos.X,M->Player->FloorY+1.6,M->Player->Pos.Z);
      M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
      TakenShot[2]=true;TakenAt=Elapsed;return true;
     }
     if(TakenShot[2]&&!TakenShot[3]&&Elapsed-TakenAt>.6){TakenShot[3]=true;Shot(TEXT("taken-flood.png"));TakenAt=Elapsed;M->Player->Yaw+=PI;M->Player->Pitch=-.25;return true;}
     if(TakenKind==1&&!TakenShot[3]&&Site.VisitBodyDistance()>0&&Site.VisitBodyDistance()<7)
     {
      const FVector Fe(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);(void)Fe;
      TakenShot[3]=true;Shot(TEXT("taken-near.png"));return true;
     }
     if(TakenShot[3]&&!TakenRoom&&Elapsed-TakenAt>.6&&Tk.Stage()==FS47Taken::EStage::Flood){TakenRoom=true;Shot(TEXT("taken-room.png"));return true;}
     if(Tk.Stage()==FS47Taken::EStage::Lift){TakenAt=Elapsed;Stage=151;return true;}
     if(Tk.Stage()==FS47Taken::EStage::Away){Stage=152;return true;}
     return true;
    }
    if(Stage==151)
    {
     if(Elapsed-TakenAt>3.5&&!TakenShot[4]){TakenShot[4]=true;Shot(TEXT("taken-lift.png"));}
     if(Tk.Stage()==FS47Taken::EStage::Away)Stage=152;
     return true;
    }
    if(Stage==152)
    {
     if(Tk.Busy())return true;
     const FS47TakenTerms& T=FS47Taken::Terms(TakenKind==1?ES47Taker::Crab:TakenKind==2?ES47Taker::Tall:ES47Taker::Roswell);
     const double Gone=(M->Freeform->Clock()-VisitClock)/60.;
     const double Moved=FVector(M->Player->Pos.X-TakenPos.X,0,M->Player->Pos.Z-TakenPos.Z).Size();
     UE_LOG(LogTemp,Display,TEXT("S47: taken: %.0f minutes gone, moved %.0f m, area %s. %s"),Gone,Moved,*M->Web->Area(),*Tk.LastNote());
     if(Gone<T.MinMinutes-3||Gone>T.MaxMinutes+12)return Fail(TEXT("the time gone must be within this taker's terms"));
     if(T.bHome&&FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(9.2,1.2))>2)return Fail(TEXT("the tall one must leave them at SARO"));
     if(T.WakeMax>0&&Moved<T.WakeMin-1)return Fail(TEXT("the crab must leave them away from the place"));
     if(!T.bHome&&T.WakeMax==0&&Moved>4)return Fail(TEXT("the small ones must leave them where they stood"));
     if(M->Player->CameraOverride&&Tk.Busy())return Fail(TEXT("the camera must be given back"));
     TakenAt=Elapsed;Stage=153;return true;
    }
    if(Stage==153)
    {
     if(Elapsed-TakenAt<1.5)return true;
     const double C0=M->Freeform->Clock();(void)C0;
     Shot(TEXT("taken-wake.png"));TakenAt=Elapsed;Stage=154;return true;
    }
    if(Elapsed-TakenAt<1)return true;
    if(Site.VisitStage()!=4)return Fail(TEXT("the visit must be over after waking"));
    UE_LOG(LogTemp,Display,TEXT("S47: taken PASS kind %d"),TakenKind);
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   // Air in the rooms, and cattle that live: they wander, they call, and something near sends them off.
   if(Stage>=160&&Stage<=166)
   {
    StageAt=Elapsed;
    auto* H=M->TownLayer&&!M->TownLayer->Herds.IsEmpty()?M->TownLayer->Herds[0].Get():nullptr;
    if(!H)return Fail(TEXT("no herd"));
    if(Stage==160){if(Elapsed-CowFrom<3)return true;Shot(TEXT("haze-control.png"));CowFrom=Elapsed;Stage=161;return true;}
    if(Stage==161)
    {
     if(Elapsed-CowFrom<1)return true;
     // Out in the pasture among them, looking across the herd.
     FVector Herd;H->CowAt(4,Herd);M->Player->Place(Herd.X+3,Herd.Z+24,0);M->Player->Pitch=-.04;
     H->CowAt(0,CowA);CowFrom=Elapsed;Stage=162;return true;
    }
    if(Stage==162)
    {
     if(Elapsed-CowFrom<25)return true;
     FVector Now;H->CowAt(0,Now);H->CowAt(1,CowB);
     UE_LOG(LogTemp,Display,TEXT("S47: cows: first moved %.2f m in 25 s, %d calls"),FVector::Dist(CowA,Now),M->Freeform->FarmNight->MooCount());
     Shot(TEXT("cows-calm.png"));CowA=Now;
     FVector Mid;H->CowAt(4,Mid);M->Freeform->FarmNight->TestAlarmAt=FVector(Mid.X,0,Mid.Z);CowCalls=M->Freeform->FarmNight->MooCount();CowFrom=Elapsed;Stage=163;return true;
    }
    if(Stage==163)
    {
     if(Elapsed-CowFrom<5)return true;
     Shot(TEXT("cows-panic.png"));Stage=164;return true;
    }
    if(Elapsed-CowFrom<9)return true;
    FVector Now;H->CowAt(1,Now);const double Ran=FVector::Dist(Now,CowB);const int32 Calls=M->Freeform->FarmNight->MooCount()-CowCalls;
    UE_LOG(LogTemp,Display,TEXT("S47: cows: ran %.1f m, %d frightened calls"),Ran,Calls);
    if(Ran<8)return Fail(TEXT("frightened cattle must run"));
    if(Calls<2)return Fail(TEXT("frightened cattle must call"));
    UE_LOG(LogTemp,Display,TEXT("S47: cows PASS"));
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   // The whole way with nothing forced: the scheduler's own draw, the lights, going there, the marks, standing about,
   // running into a building. Only the waits and the journey are shortened.
   if(Stage>=170&&Stage<=176)
   {
    StageAt=Elapsed;
    auto& D=*M->Freeform->Director;auto& Site=*M->Freeform->FarmSite;
    if(Elapsed-ChainFrom>270)return Fail(TEXT("the chain stopped")); // Lights in town wait about 184 s for someone driving there from SARO.
    if(Stage==170)
    {
     if(D.ActiveEvent().IsEmpty())return true;
     if(D.ActiveEvent()!=TEXT("farm_lights")&&D.ActiveEvent()!=TEXT("town_lights"))return true;
     ChainEvent=D.ActiveEvent();
     UE_LOG(LogTemp,Display,TEXT("S47: chain: the night's draw started %s; objective: %s"),*ChainEvent,*M->Freeform->Objective());
     if(!M->Freeform->Objective().Contains(TEXT("RADIO TIP")))return Fail(TEXT("the tip must stand on the objective line"));
     // Go there as the player would end up: on foot at the place, in its own area.
     if(ChainEvent==TEXT("town_lights")){M->World->PlaceTruck(TEXT("diner"));M->World->Enter(TEXT("diner"));}
     ChainAt=Elapsed;Stage=171;return true;
    }
    if(Stage==171)
    {
     if(Elapsed-ChainAt<1.5)return true;
     const FVector A=D.EventAnchor();M->Player->Place(A.X+30,A.Z,0);
     // In town the lights rise when someone is near, however long they have been there.
     ChainAt=Elapsed;Stage=172;return true;
    }
    if(Stage==172)
    {
     if(!D.ActiveEvent().IsEmpty()){const FVector A=D.EventAnchor();M->Player->Place(A.X+10,A.Z,0);return true;}
     if(!Site.Armed())return Fail(TEXT("the lights must leave marks behind"));
     UE_LOG(LogTemp,Display,TEXT("S47: chain: lights gone, marks at %s, %s"),*Site.PlaceId(),*Site.Centre().ToCompactString());
     // The night passes: nothing comes for anyone before about two. S47MenuPointerKinds holds that rule to account.
     {const double ChainToday=FMath::Fmod(M->Freeform->Clock(),86400.);if(!M->Freeform->TestAdvance(FMath::Fmod(2.5*3600.-ChainToday+86400.,86400.)))return Fail(TEXT("the chain needs its small hours"));}
     Site.TestReadyVisit(); // the 60 percent, taken as given
     const FVector C=Site.Centre();M->Player->Place(C.X+6,C.Z+6,0);ChainAt=Elapsed;Stage=173;return true;
    }
    if(Stage==173)
    {
     // Twelve seconds standing about, on its own.
     if(Site.VisitStage()>=1){UE_LOG(LogTemp,Display,TEXT("S47: chain: the visit began after %.1f s standing there"),Elapsed-ChainAt);ChainAt=Elapsed;Stage=174;return true;}
     if(Elapsed-ChainAt>25)return Fail(TEXT("standing at the marks must bring the visit"));
     return true;
    }
    if(Stage==174)
    {
     // Run for a roof: the old house at the farm, any building in town.
     auto& Tk=*M->Freeform->Taken;
     if(Site.PlaceId()==TEXT("farm"))
     {
      const FVector In=S47FarmHouse::Point(2.5,2.4)+FVector(-24,0,300);M->Player->Place(In.X,In.Z,0);
      if(Tk.Indoors()&&Tk.Busy()){UE_LOG(LogTemp,Display,TEXT("S47: chain: steps overhead after running in"));Stage=175;ChainAt=Elapsed;return true;}
      if(Elapsed-ChainAt>12)return Fail(TEXT("running under a roof must start the steps overhead"));
      return true;
     }
     UE_LOG(LogTemp,Display,TEXT("S47: chain: in town, indoors %d"),Tk.Indoors());Stage=175;ChainAt=Elapsed;return true;
    }
    if(Stage==175)
    {
     // And a menu holds it: no time passes in the light behind a panel.
     auto& Tk=*M->Freeform->Taken;
     if(Tk.Busy()&&!ChainPaused){M->bPausedByMenu=true;ChainPaused=true;ChainAt=Elapsed;ChainAge=Tk.LightOn();return true;}
     if(ChainPaused&&Elapsed-ChainAt<3)return true;
     if(ChainPaused&&M->bPausedByMenu)
     {
      M->bPausedByMenu=false;
      if(Tk.LightOn()>ChainAge+.01)return Fail(TEXT("the light must not count behind a menu"));
     }
     Stage=176;ChainAt=Elapsed;return true;
    }
    UE_LOG(LogTemp,Display,TEXT("S47: chain PASS"));
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   // A look at each simplified panel.
   if(Stage>=180&&Stage<=188)
   {
    if(Elapsed-StageAt<.9)return true;
    auto& F=*M->Freeform->Field;
    const auto Close=[&]{if(M->UI)M->UI->Close(true);};
    switch(Stage)
    {
     case 180:{const FVector B=F.InteractionPoint(TEXT("ff:bench"));M->Player->Place(B.X,B.Z-1,0);F.OpenWorkshop();break;}
     case 181:Shot(TEXT("panel-workshop.png"));break;
     case 182:Close();F.OpenBranch(FS47ArrayWear::Branches()[0].Id);break;
     case 183:Shot(TEXT("panel-branch.png"));break;
     // Two retained records so the archive shows its tabs, rows and state column.
     case 184:Close();M->Freeform->RegisterFinding(TEXT("service-paneltest"),TEXT("Logger battery pack built at the SARO bench from tested donor parts."));M->Freeform->RegisterFinding(TEXT("wrong_source-paneltest"),TEXT("The odd sound was only on the original."));M->Freeform->OpenArchive();break;
     case 185:Shot(TEXT("panel-archive.png"));break;
     case 186:Close();{const FVector T=F.TruckPointForTest();M->Player->Place(T.X,T.Z+3,0);}F.Use(TEXT("ff:cargo"));break;
     case 187:Shot(TEXT("panel-cargo.png"));break;
     // The photo lab and the reel deck follow in stages 600 and up.
     case 188:Close();Stage=599;break;
    }
    ++Stage;StageAt=Elapsed;return true;
   }
   // The photo lab (all three step states, then the empty bench) and the reel deck at its listening place.
   if(Stage>=600&&Stage<=607)
   {
    if(Elapsed-StageAt<.9)return true;
    const auto Close=[&]{if(M->UI)M->UI->Close(true);};
    switch(Stage)
    {
     case 600:Close();S47Panels::WetBench(M->UI,TEXT("SARO / PHOTO LAB  |  FILM: free1067653  |  Control room"),2,FString(),[]{});break;
     case 601:Shot(TEXT("panel-wetbench.png"));break;
     case 602:Close();S47Panels::WetBench(M->UI,FString(),0,TEXT("The tank is empty.\nExpose a frame from the S-03 apron before loading film."),[]{});break;
     case 603:Shot(TEXT("panel-wetbench-empty.png"));break;
     case 604:
     {
      Close();
      if(!M->Freeform->AudioDesk||!M->Freeform->AudioDesk->Ready())return Fail(TEXT("the reel deck is not built"));
      const FVector T=M->Freeform->AudioDesk->InteractionPoint(TEXT("ff:audio:original"));M->Player->Place(T.X,T.Z-1.45,PI);
      break;
     }
     case 605:M->Freeform->OpenTapePanel();if(!M->Freeform->TapePanelOpen())return Fail(TEXT("the reel deck panel did not open"));break;
     case 606:Shot(TEXT("panel-reeldeck.png"));break;
     case 607:Close();UE_LOG(LogTemp,Display,TEXT("S47: panels shown"));FPlatformMisc::RequestExitWithStatus(false,0);return false;
    }
    ++Stage;StageAt=Elapsed;return true;
   }
   // What the four workstations say from across the room, and the record at the bench.
   if(Stage>=620&&Stage<=626)
   {
    if(Elapsed-StageAt<.9)return true;
    const auto ScreensAgree=[&](int32 OwnScreen)
    {
     const FString Objective=M->Freeform->Objective();
     for(int32 ScreenId=0;ScreenId<4;++ScreenId)
     {
      const TArray<FString> ScreenLines=M->Freeform->TerminalLines(ScreenId);
      const FString Want=FS47FreeformSession::TerminalNextLine(Objective,ScreenId);
      for(const FString& ScreenLine:ScreenLines)UE_LOG(LogTemp,Display,TEXT("S47: screen %d: %s"),ScreenId,*ScreenLine);
      if(ScreenLines.Num()>9||Want.IsEmpty()||Want.Len()>38||!ScreenLines.Contains(Want))return false;
      if((Want==TEXT("NEXT / USE THIS PC"))!=(ScreenId==OwnScreen))return false;
     }
     return true;
    };
    const auto ScreenAt=[&](const TCHAR* PropId,double OffX,double OffZ)
    {
     const FVector Prop=S47Three::ToThree(M->Web->PropLocation(PropId));M->Player->Place(Prop.X+OffX,Prop.Z+OffZ,0);
     const FVector Look=Prop-S47Three::ToThree(M->Player->EyeUE());M->Player->Yaw=FMath::Atan2(-Look.X,-Look.Z);M->Player->Pitch=FMath::Atan2(Look.Y,FVector2D(Look.X,Look.Z).Size());
    };
    switch(Stage)
    {
     case 620:ScreenAt(TEXT("opsTerminal"),1.1,0);if(!ScreensAgree(3))return Fail(TEXT("the first shift's step must stand on all four screens, with OPS as this PC"));break;
     case 621:Shot(TEXT("screens-ops.png"));break;
     case 622:M->Freeform->AcceptTutorialJob();ScreenAt(TEXT("crtRight"),0,-1.1);if(!ScreensAgree(2))return Fail(TEXT("after accepting the job, tracking must be this PC and the others must send the player to it"));break;
     case 623:Shot(TEXT("screens-tracking.png"));break;
     case 624:{const FVector B=M->Freeform->Field->InteractionPoint(TEXT("ff:bench"));M->Player->Place(B.X,B.Z-1,0);M->Freeform->Field->OpenWorkbench();if(!M->ModalOpen())return Fail(TEXT("the workbench did not open"));break;}
     case 625:Shot(TEXT("panel-bench-record.png"));break;
     case 626:if(M->UI)M->UI->Close(true);UE_LOG(LogTemp,Display,TEXT("S47: screens and bench record shown"));FPlatformMisc::RequestExitWithStatus(false,0);return false;
    }
    ++Stage;StageAt=Elapsed;return true;
   }
   // One of them, stock still, head up, looking at the marks.
   if(Stage==190)
   {
    StageAt=Elapsed;
    auto* H=M->TownLayer&&!M->TownLayer->Herds.IsEmpty()?M->TownLayer->Herds[0].Get():nullptr;
    if(!H)return Fail(TEXT("no herd"));
    if(Elapsed-CowFrom<3)return true;
    if(H->Starer()<0)return Fail(TEXT("one cow must stand and stare at the marks"));
    FVector A;H->CowAt(H->Starer(),A);CowA=A;CowFrom=Elapsed;Stage=191;
    UE_LOG(LogTemp,Display,TEXT("S47: cow %d stares"),H->Starer());return true;
   }
   if(Stage==191)
   {
    StageAt=Elapsed;
    auto* H=M->TownLayer->Herds[0].Get();
    if(Elapsed-CowFrom<20)return true;
    if(H->Starer()<0)return Fail(TEXT("the stare must last"));
    FVector B;H->CowAt(H->Starer(),B);
    if(FVector::Dist(FVector(CowA.X,0,CowA.Z),FVector(B.X,0,B.Z))>.7)return Fail(TEXT("the staring cow must stay put"));
    UE_LOG(LogTemp,Display,TEXT("S47: stare PASS"));
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   if(Stage==139)
   {
    if(Elapsed-VisitFrom<2)return true;
    auto& D=*M->Freeform->Director;const FVector P=D.LightPlacePoint(TEXT("cinema"));
    if(P.IsZero())return Fail(TEXT("the cinema must be a place"));
    M->Freeform->FarmSite->Arm(M->Freeform->Clock(),TEXT("cinema"),TEXT("the Plaza cinema"),TEXT("diner"),P);
    const FVector C=M->Freeform->FarmSite->Centre();M->Player->Place(C.X,C.Z,0);
    UE_LOG(LogTemp,Display,TEXT("S47: visit test in town, site %s"),*C.ToCompactString());
    VisitClock=M->Freeform->Clock();M->Freeform->FarmSite->TestVisit();Stage=140;VisitFrom=Elapsed;StageAt=Elapsed;return true;
   }
   // Stay and it comes for you; walk off and it lets go.
   if(Stage>=140&&Stage<=146)
   {
    auto& Site=*M->Freeform->FarmSite;
    StageAt=Elapsed; // long waits by design; the visit keeps its own time
    if(Stage==140)
    {
     if(Site.VisitStage()>=2){Stage=141;VisitFrom=Elapsed;StageAt=Elapsed;return true;}
     if(Elapsed-VisitFrom>35)return Fail(TEXT("a body must come out at the site"));
     return true;
    }
    if(Stage==141)
    {
     auto Views=M->Night->Glimpses->Individuals();
     if(Views.IsEmpty())return Fail(TEXT("the visit has no body"));
     if(Elapsed-VisitFrom<9)return true;
     const FVector D=Views[0].Pos-FVector(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
     M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=-.12;
     UE_LOG(LogTemp,Display,TEXT("S47: visit body %.1f m away, state %d, rendered %d"),D.Size(),int32(Views[0].State),Views[0].bRendered);
     Stage=142;VisitFrom=Elapsed;StageAt=Elapsed;return true;
    }
    if(Stage==142){if(Elapsed-VisitFrom<1)return true;Shot(TEXT("visit-body.png"));Stage=143;VisitFrom=Elapsed;StageAt=Elapsed;return true;}
    if(Stage==143)
    {
     const double Before=VisitClock;
     if(Site.VisitStage()==4)
     {
      M->Player->TestMove.Reset();
      UE_LOG(LogTemp,Display,TEXT("S47: visit reached the player after %.1f s"),Elapsed-VisitFrom);
      if(M->Freeform->Clock()<VisitClock+14*60)return Fail(TEXT("contact must take about fifteen minutes"));
      Stage=144;VisitFrom=Elapsed;StageAt=Elapsed;return true;
     }
     (void)Before;
     // The player goes to look. It must come for them once it hunts and they are near.
     {
      const auto Views=M->Night->Glimpses->Individuals();
      if(!Views.IsEmpty()&&Site.VisitStage()==3)
      {
       const FVector D=Views[0].Pos-FVector(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
       M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=-.1;M->Player->TestMove=FVector2D(0,-.8);
       static double NextLog=0;
       if(Elapsed>=NextLog){NextLog=Elapsed+4;UE_LOG(LogTemp,Display,TEXT("S47: visit watch %.1f m, state %d, speed %.2f"),D.Size(),int32(Views[0].State),Views[0].Speed);}
      }
     }
     if(Elapsed-VisitFrom>100)return Fail(TEXT("staying must end in contact"));
     return true;
    }
    if(Stage==144)
    {
     if(Elapsed-VisitFrom<2.5)return true;
     Site.TestVisit();Stage=145;VisitFrom=Elapsed;StageAt=Elapsed;return true;
    }
    if(Stage==145)
    {
     if(Site.VisitStage()==4)return Fail(TEXT("a second body must come"));
     if(Site.VisitStage()<2)return true;
     const FVector C=Site.Centre();M->Player->Place(C.X+90,C.Z,0);Stage=146;VisitFrom=Elapsed;StageAt=Elapsed;return true;
    }
    if(Site.VisitStage()==4)
    {
     if(!M->Night->Glimpses->Individuals().IsEmpty())return Fail(TEXT("leaving must clear the body"));
     UE_LOG(LogTemp,Display,TEXT("S47: visit PASS: warned, came out, reached the player who stayed, let go of the one who left"));
     FPlatformMisc::RequestExitWithStatus(false,0);return false;
    }
    if(Elapsed-VisitFrom>6)return Fail(TEXT("the visit must end when the player leaves"));
    return true;
   }
   // The service door on a far antenna, then the truck out over the field and the walk from where it stops.
   if(Stage>=120&&Stage<=132)
   {
    auto* F=M->Freeform->Field.Get();
    if(Elapsed-StageAt<(Stage==124||Stage==127?2.5:Stage==129?7.:Stage==130?5.:.8))return true;
    switch(Stage)
    {
     case 120:
     {
      const FVector Face=F->ServiceDoorFace();const double Y=F->ServiceDoorYaw();
      if(Face.IsNearlyZero())return Fail(TEXT("no service door at S-08"));
      // Stand 1.6 m out from the face, a little to the hinge side, looking at it.
      const FVector Eye=Face+FVector(-1.6*FMath::Cos(Y)+.5*FMath::Sin(Y),0,1.6*FMath::Sin(Y)+.5*FMath::Cos(Y));
      M->Player->Place(Eye.X,Eye.Z,0);
      const FVector D=Face-Eye;M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=-.25;
      break;
     }
     case 121:Shot(TEXT("field-door-shut.png"));break;
     case 122:if(!M->Freeform->Field->Use(TEXT("ff:service")))return Fail(TEXT("the service door does not take E"));break;
     case 123:if(F->ServiceStage()!=4)return Fail(TEXT("using S-08 must open the hatch"));if(F->ServiceDoorOpen()<=.05||F->ServiceDoorOpen()>=.98)UE_LOG(LogTemp,Display,TEXT("S47: door k %.2f"),F->ServiceDoorOpen());break;
     case 124:if(F->ServiceDoorOpen()<.99)return Fail(TEXT("the door must swing fully open"));Shot(TEXT("field-door-open.png"));break;
     case 125:M->Player->Place(-24,-60,0);M->World->DriveOut();break;
     case 126:
      if(!M->World->bDriving)return Fail(TEXT("not driving"));
      if(!M->World->MoveDriven(-24,-150,PI/2))return Fail(TEXT("cannot move the truck to the highway"));
      M->World->CabDrive()->bEngine=true;
      M->World->TestInput=[]{FS47DriveIn In;In.Throttle=.8;return In;};
      break;
     case 127:break;
     case 128:Shot(TEXT("field-drive.png"));break;
     case 129:
     {
      const FVector P=M->World->CabDrive()->Pos;
      UE_LOG(LogTemp,Display,TEXT("S47: field drive at %.1f %.1f"),P.X,P.Z);
      if(FMath::Abs(P.X+24)<12)return Fail(TEXT("the truck must leave the highway onto the field"));
      M->World->TestInput=[M]{FS47DriveIn In;const auto* C=M->World->CabDrive();In.Throttle=C&&C->Speed>.08?-1.:0.;return In;};
      break;
     }
     case 130:
      if(!M->World->GetOutHere(true))return Fail(TEXT("must get out in the field"));
      FieldFrom=FVector(M->Player->Pos.X,0,M->Player->Pos.Z);
      M->World->TestInput=nullptr;
      M->Player->TestMove=FVector2D(0,-1);
      break;
     case 131:Shot(TEXT("field-out.png"));break;
     case 132:
     {
      const double Moved=FVector(M->Player->Pos.X-FieldFrom.X,0,M->Player->Pos.Z-FieldFrom.Z).Size();
      M->Player->TestMove.Reset();
      if(Moved<1)return Fail(TEXT("must walk on the field after getting out"));
      UE_LOG(LogTemp,Display,TEXT("S47: field PASS: S-08 door opened with a swing, truck drove onto the field, walked %.1f m from it"),Moved);
      Stage=133;StageAt=Elapsed;return true;
     }
    }
    ++Stage;StageAt=Elapsed;return true;
   }
   // The door on the three other ways the drive boxes stand round their antennas.
   if(Stage>=133&&Stage<=134)
   {
    static const TCHAR* Dishes[3]={TEXT("S-01"),TEXT("S-02"),TEXT("S-04")};
    if(Elapsed-StageAt<1.)return true;
    auto* F=M->Freeform->Field.Get();
    if(FieldShot>=3){FPlatformMisc::RequestExitWithStatus(false,0);return false;}
    if(Stage==133)
    {
     F->TestService(Dishes[FieldShot],4);
     const FVector Face=F->ServiceDoorFace();const double Y=F->ServiceDoorYaw();
     const FVector Eye=Face+FVector(-1.7*FMath::Cos(Y)+.6*FMath::Sin(Y),0,1.7*FMath::Sin(Y)+.6*FMath::Cos(Y));
     M->Player->Place(Eye.X,Eye.Z,0);const FVector D=Face-Eye;M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=-.25;
     Stage=134;StageAt=Elapsed;return true;
    }
    Shot(*FString::Printf(TEXT("field-door-%d.png"),FieldShot));++FieldShot;Stage=133;StageAt=Elapsed;return true;
   }
   if(Stage==110)
   {
    if(Elapsed-StageAt<.5)return true;
    if(FS47TripoCreature::ShownEyeGlints()!=0)return Fail(TEXT("no eye glint past 50 m"));
    Shot(TEXT("eyes-60m.png"));
    if(M->Story)M->Story->SetClock(12*3600);
    Stage=111;StageAt=Elapsed;return true;
   }
   if(Stage==111)
   {
    if(Elapsed-StageAt<1.5)return true;
    if(FS47TripoCreature::ShownEyeGlints()!=0)return Fail(TEXT("no eye glint by day"));
    UE_LOG(LogTemp,Display,TEXT("S47: eye test PASS: night 10 and 25 m show, 60 m and daylight do not"));FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   if(Stage==109)
   {
    if(Elapsed-StageAt<3)return true;
    if(M->Night->Glimpses->DevSpawn(1,FS47RoswellBrain::EMode::Stand)!=1)return Fail(TEXT("the eye test needs a body outdoors"));
    Stage=103;StageAt=Elapsed;return true;
   }
   if(Stage==60)
   {
    // Each place in turn: the lights start, are presented, leave when approached and leave a site.
    static const TCHAR* Places[]={TEXT("cinema"),TEXT("feed"),TEXT("church"),TEXT("gas")};
    if(TownAt>=UE_ARRAY_COUNT(Places))
    {
     UE_LOG(LogTemp,Display,TEXT("S47: town lights PASS: four places presented, left when approached, each site found and kept"));
     FPlatformMisc::RequestExitWithStatus(false,0);return false;
    }
    TownPlace=Places[TownAt];auto& D=*M->Freeform->Director;const FVector P=D.LightPlacePoint(TownPlace);
    if(P.IsZero())return Fail(TEXT("a town place has no interactable to anchor on"));
    // Along the street, which is walkable ground at every one of these fronts.
    M->Player->Place(7995,P.Z+85,0);
    UE_LOG(LogTemp,Display,TEXT("S47: town test at %s floor %.2f, place %.1f %.1f %.1f"),*TownPlace,M->Player->FloorY,P.X,P.Y,P.Z);
    if(M->FieldRadio)M->FieldRadio->TestOwn();
    if(!D.ForceLights(TownPlace))return Fail(TEXT("town lights must start at each place"));
    Stage=61;StageAt=Elapsed;return true;
   }
   if(Stage==61)
   {
    // The event's own age: a paused game would otherwise pass here with lights that never faded in.
    if(M->Freeform->Director->EventAge()<14){if(Elapsed-StageAt>25)return Fail(TEXT("the town lights' event must keep running"));StageAt=StageAt+0;return true;}
    if(!M->Freeform->Director->PhysicalObjectPresented())return Fail(TEXT("the town lights must be presented"));
    const FVector A=M->Freeform->Director->EventAnchor();const FVector E(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);const FVector D=A-E;
    M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
    Stage=62;StageAt=Elapsed;return true;
   }
   if(Stage==62)
   {
    Shot(*FString::Printf(TEXT("town-lights-%s.png"),*TownPlace));Stage=66;StageAt=Elapsed;return true;
   }
   if(Stage==66)
   {
    const int32 Lit=Exposed(M);UE_LOG(LogTemp,Display,TEXT("S47: town lights at %s, bright pixels in the photograph %d"),*TownPlace,Lit);
    const FVector P=M->Freeform->Director->LightPlacePoint(TownPlace);M->Player->Place(7995,P.Z+12,0);
    Stage=63;StageAt=Elapsed;return true;
   }
   if(Stage==63)
   {
    if(!M->Freeform->Director->ActiveEvent().IsEmpty()){if(Elapsed-StageAt>12)return Fail(TEXT("the town lights must leave when approached"));return true;}
    auto& Site=*M->Freeform->FarmSite;
    if(!Site.Armed()||Site.PlaceId()!=TownPlace)return Fail(TEXT("the lights must leave a site at that place"));
    const FVector C=Site.Centre();const FVector2D At(C.X+5,C.Z+5);
    M->Player->Place(At.X,At.Y,0);
    const FVector D=C-FVector(At.X,M->Player->FloorY+1.7,At.Y);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
    UE_LOG(LogTemp,Display,TEXT("S47: town test site %s centre %.1f %.1f %.1f floor %.2f"),*TownPlace,C.X,C.Y,C.Z,M->Player->FloorY);
    Stage=64;StageAt=Elapsed;return true;
   }
   if(Stage==64)
   {
    Shot(*FString::Printf(TEXT("town-site-%s.png"),*TownPlace));Stage=65;StageAt=Elapsed;return true;
   }
   if(Stage==65)
   {
    auto& Site=*M->Freeform->FarmSite;
    if(Site.Label().IsEmpty()||!Site.Use()||!M->Freeform->HasFinding(Site.FindingId()))return Fail(TEXT("a town site must keep one field record"));
    M->UI->Close(true);++TownAt;Stage=60;StageAt=Elapsed-.5;return true;
   }
   if(Stage==49)
   {
    SkyBefore=Exposed(M);
    if(!M->Freeform->Director->Force(TEXT("farm_lights")))return Fail(TEXT("farm lights must start at night"));
    Stage=50;StageAt=Elapsed;return true;
   }
   if(Stage==50)
   {
    if(Elapsed-StageAt<15)return true; // Faded in.
    if(!M->Freeform->Director->PhysicalObjectPresented())return Fail(TEXT("the farm lights must be presented from the yard"));
    Shot(TEXT("farm-lights.png"));Stage=51;StageAt=Elapsed;return true;
   }
   if(Stage==51)
   {
    const int32 Lit=Exposed(M);
    const auto& Claims=M->Freeform->Director->PhotoClaims();
    // The same view before and after: the lights have to be bright in the actual photograph, not just present.
    UE_LOG(LogTemp,Display,TEXT("S47: farm lights photo bright pixels %d before, %d with lights"),SkyBefore,Lit);
    if(Claims.Num()!=1||!Claims[0].Id.StartsWith(TEXT("farm_lights.")))return Fail(TEXT("a photograph from the yard must qualify the farm lights"));
    if(Lit<SkyBefore+6)return Fail(TEXT("the farm lights must actually be bright in the photograph"));
    M->Player->Place(40,450,0);Stage=52;StageAt=Elapsed;return true;
   }
   if(Stage==52)
   {
    if(!M->Freeform->Director->ActiveEvent().IsEmpty()){if(Elapsed-StageAt>10)return Fail(TEXT("the farm lights must leave when someone comes close"));return true;}
    if(!M->Freeform->FarmSite->Armed())return Fail(TEXT("the lights must leave something behind"));
    const FVector C=M->Freeform->FarmSite->Centre();const FVector2D At(C.X+7,C.Z+7);const FVector D=C-FVector(At.X,C.Y+1.7,At.Y);
    M->Player->Place(At.X,At.Y,FMath::Atan2(-D.X,-D.Z));M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
    Stage=53;StageAt=Elapsed;return true;
   }
   if(Stage==53)
   {
    Shot(TEXT("farm-site.png"));Stage=54;StageAt=Elapsed;return true;
   }
   if(Stage==54)
   {
    auto& Site=*M->Freeform->FarmSite;
    if(Site.Label().IsEmpty()||!Site.Use()||!M->Freeform->HasFinding(Site.FindingId()))return Fail(TEXT("looking closer must keep one field record"));
    Stage=55;StageAt=Elapsed;return true;
   }
   if(Stage==55)
   {
    Shot(TEXT("farm-site-panel.png"));Stage=56;StageAt=Elapsed;return true;
   }
   if(Stage==56)
   {
    M->UI->Close(true);auto& Site=*M->Freeform->FarmSite;
    const auto Saved=Site.Save();Site.Begin(Saved);if(!Site.Armed())return Fail(TEXT("the site must survive a reload"));
    if(!M->Freeform->TestAdvance(16.*3600.))return Fail(TEXT("advancing past noon"));
    Stage=57;StageAt=Elapsed;return true;
   }
   if(Stage==57)
   {
    if(M->Freeform->FarmSite->Armed())return Fail(TEXT("the site must be gone by the next noon"));
    UE_LOG(LogTemp,Display,TEXT("S47: farm lights PASS: bright in the photograph, left when approached, site found, kept, reloaded and gone by morning"));
    FPlatformMisc::RequestExitWithStatus(false,0);
    return false;
   }
   if(Stage==10)
   {
    if(!Sees(TEXT("Album")))return Fail(TEXT("the notebook did not open on the Album"));
    Shot(TEXT("album-tab.png"));Press(EKeys::Enter);Stage=11;StageAt=Elapsed;return true;
   }
   if(Stage==11)
   {
    if(!Sees(TEXT("X  THROW AWAY PHOTO")))return Fail(TEXT("an open ordinary print offers no way to throw it away"));
    Shot(TEXT("album-print.png"));Press(EKeys::X);Stage=12;StageAt=Elapsed;return true;
   }
   if(Stage==12)
   {
    if(!Sees(TEXT("X  AGAIN TO THROW IT AWAY"))||M->Night->Album->Shots().Num()!=2)return Fail(TEXT("the first X must only ask"));
    Shot(TEXT("album-armed.png"));Press(EKeys::X);Stage=13;StageAt=Elapsed;return true;
   }
   if(Stage==13)
   {
    if(M->Night->Album->Shots().Num()!=1||M->Night->Album->Left()!=LeftBefore||!M->ModalOpen())return Fail(TEXT("the second X must remove the print and keep its frame used"));
    Shot(TEXT("album-after.png"));Press(EKeys::Enter);Stage=14;StageAt=Elapsed;return true;
   }
   if(Stage==14)
   {
    if(!Sees(TEXT("KEPT: USED IN YOUR WORK")))return Fail(TEXT("a print the game saw something in must be kept"));
    Shot(TEXT("album-kept.png"));Press(EKeys::X);Press(EKeys::X);Stage=15;StageAt=Elapsed;return true;
   }
   if(Stage==15)
   {
    if(M->Night->Album->Shots().Num()!=1)return Fail(TEXT("a kept print was thrown away"));
    UE_LOG(LogTemp,Display,TEXT("S47: album PASS: tab, open print, two-step throw away, frame stays used, kept print refused"));
    M->UI->Close(true);
    const auto& Decor=M->Freeform->Decor;const FString Kept=M->Night->Album->Shots()[0].Id;
    if(!Decor||!Decor->Hang(1,Kept)||!Decor->HoldsPhoto(Kept))return Fail(TEXT("a developed print must go up in a frame"));
    const TArray<FString> Saved=S47J::Strs(Decor->Save(),TEXT("frames"));
    if(Saved.Num()!=FS47RoomDecor::SlotCount||Saved[1]!=Kept)return Fail(TEXT("the frame must save which print it holds"));
    M->Player->Place(-3.4,2.2,PI/2);Stage=20;StageAt=Elapsed;return true;
   }
   if(Stage==20)
   {
    Shot(TEXT("decor-wall.png"));Stage=22;StageAt=Elapsed;return true;
   }
   if(Stage==22)
   {
    M->Freeform->Decor->Use(TEXT("ff:decor:0"));Stage=23;StageAt=Elapsed;return true;
   }
   if(Stage==23)
   {
    Shot(TEXT("decor-panel.png"));Stage=21;StageAt=Elapsed;return true;
   }
   if(Stage==21)
   {
    M->UI->Close(true);
    // A reload puts the same print back; a print that no longer exists leaves the frame empty.
    const auto Saved=M->Freeform->Decor->Save();const FString Kept=M->Freeform->Decor->SlotPhoto(1);
    M->Freeform->Decor->Begin(Saved);M->Freeform->Decor->Step();
    if(M->Freeform->Decor->SlotPhoto(1)!=Kept)return Fail(TEXT("a reloaded frame must hold the same print"));
    const auto Gone=S47J::New();S47J::Set(Gone,TEXT("frames"),TArray<FString>{TEXT("no-such-print"),Kept});
    M->Freeform->Decor->Begin(Gone);M->Freeform->Decor->Step();
    if(!M->Freeform->Decor->SlotPhoto(0).IsEmpty())return Fail(TEXT("a missing print must leave its frame empty"));
    UE_LOG(LogTemp,Display,TEXT("S47: room decor PASS: hang, save, reload, missing print"));
    S47Panels::WetBench(M->UI,TEXT("SARO / PHOTO LAB  |  FILM: free1067653  |  Control room"),0,FString(),[]{});Stage=16;StageAt=Elapsed;return true;
   }
   if(Stage==16)
   {
    Shot(TEXT("wetbench.png"));M->UI->Close(true);
    M->UI->Open(S47Kit::Shell(M->UI,TEXT("SARO / RECORDS"),S47Kit::Col({
     S47Kit::Choice(TEXT("Logger battery pack"),true,[]{},false,TEXT("DAY 1")),
     S47Kit::Choice(TEXT("D01-01 / ORIGINAL D01-01-original"),false,[]{},false,TEXT("COMPARISON PENDING / RECORDED")),
     S47Kit::Paper(TEXT("DAY 1\nRETAINED RECORD / Logger battery pack built at the SARO bench from tested donor parts."))}),1000));
    Stage=18;StageAt=Elapsed;return true;
   }
   if(Stage==18)
   {
    Shot(TEXT("records.png"));Stage=17;StageAt=Elapsed;return true;
   }
   if(Stage==17)
   {
    FPlatformMisc::RequestExitWithStatus(false,0);
    return false;
   }
   // QUIT must exit through the ordinary menu callback. A swallowed click times out above.
   return true;
  }
 };
}

void S47MenuPointerTest::Start(US47Main* Main)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerTest"))) return;
 FString User;
 FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
 if(!User.StartsWith(TEXT("/tmp/S47-menu-pointer-")) || !FParse::Param(FCommandLine::Get(),TEXT("S47Title")))
 {
  UE_LOG(LogTemp,Error,TEXT("S47: menu pointer FAIL: requires -S47Title and isolated /tmp/S47-menu-pointer-* UserDir"));
  FPlatformMisc::RequestExitWithStatus(false,1);
  return;
 }
 const auto Run=MakeShared<FRun>();Run->Main=Main;
 Run->bLethal=FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerLethal"));
 Run->bQuit=FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerQuit"));
 Run->bFullCases=FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerFullCases"));
 Run->bAlbum=FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerAlbum"));
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float Dt){return Run->Step(Dt);}));
}
