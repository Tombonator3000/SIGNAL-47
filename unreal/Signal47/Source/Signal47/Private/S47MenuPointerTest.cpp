#include "S47MenuPointerTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47Album.h"
#include "S47Flow.h"
#include "S47Night.h"
#include "S47Panels.h"
#include "S47Kit.h"
#include "S47FreeformDirector.h"
#include "S47FieldCamera.h"
#include "S47RoomDecor.h"
#include "S47Walker.h"
#include "S47UI.h"
#include "S47Json.h"
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

 struct FRun
 {
  TWeakObjectPtr<US47Main> Main;
  int32 Stage=0;
  double Elapsed=0,StageAt=0;
  bool bLethal=false,bQuit=false,bFullCases=false,bAlbum=false;
  int32 LeftBefore=0;
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
  bool Step(float Dt)
  {
   Elapsed+=Dt;
   if(!Main.IsValid()) return Fail(TEXT("main disappeared"));
   auto* M=Main.Get();
   if(Elapsed-StageAt>30) return Fail(TEXT("menu click did not reach its expected next screen"));
   if(Elapsed-StageAt<1) return true;
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
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerFarm")))
    {
     M->Flow->bDevRun=true;
     if(!M->Freeform->TestAdvance(13.*3600.)||!M->Freeform->Director->Force(TEXT("farm_lights")))return Fail(TEXT("farm lights must start at night"));
     Stage=50;StageAt=Elapsed;return true;
    }
    if(bAlbum){GiveAlbum(M);M->UI->NotebookAt(TEXT("photos"));M->Flow->OpenNotebook();Stage=10;StageAt=Elapsed;return true;}
    UE_LOG(LogTemp,Display,TEXT("S47: menu pointer PASS: title -> NEW FREEFORM -> %s -> active day 1 with %d jobs"),bLethal?TEXT("death"):TEXT("local recovery"),M->Freeform->Jobs().Num());
    FPlatformMisc::RequestExitWithStatus(false,0);
    return false;
   }
   if(Stage==50)
   {
    if(Elapsed-StageAt<15)return true; // Faded in.
    const FVector Target=M->Freeform->Director->EventAnchor();const FVector2D At(0,30);const FVector D=Target-FVector(At.X,1.7,At.Y);
    M->Player->Place(At.X,At.Y,FMath::Atan2(-D.X,-D.Z));M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
    Stage=51;StageAt=Elapsed;return true;
   }
   if(Stage==51)
   {
    if(!M->Freeform->Director->PhysicalObjectPresented())return Fail(TEXT("the farm lights must be presented from the yard"));
    Shot(TEXT("farm-lights.png"));Stage=52;StageAt=Elapsed;return true;
   }
   if(Stage==52)
   {
    M->FieldCamera->Raise(true);M->FieldCamera->Sync();const auto Photo=M->Night->Album->Expose();M->FieldCamera->Raise(false);M->UI->Viewfinder(false);
    const auto& Claims=M->Freeform->Director->PhotoClaims();
    if(Photo.bUnavailable||Claims.Num()!=1||!Claims[0].Id.StartsWith(TEXT("farm_lights.")))return Fail(TEXT("a photograph from the yard must qualify the farm lights"));
    UE_LOG(LogTemp,Display,TEXT("S47: farm lights PASS: presented and photographed from the yard, quality %.3f"),Claims[0].Quality);
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
