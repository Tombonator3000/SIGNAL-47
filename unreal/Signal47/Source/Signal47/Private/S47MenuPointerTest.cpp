#include "S47MenuPointerTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
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
  bool bLethal=false,bQuit=false,bFullCases=false;
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
   if(Elapsed-StageAt>15) return Fail(TEXT("menu click did not reach its expected next screen"));
   if(Elapsed-StageAt<1) return true;
   const FString Label=bQuit?TEXT("QUIT"):TEXT("NEW FREEFORM");
   if(Stage==0)
   {
    FSlateApplication::Get().ProcessApplicationActivationEvent(true);
    if(ClickText(Label)){Stage=1;StageAt=Elapsed;} // First click dismisses the ordinary title gate.
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
    UE_LOG(LogTemp,Display,TEXT("S47: menu pointer PASS: title -> NEW FREEFORM -> %s -> active day 1 with %d jobs"),bLethal?TEXT("death"):TEXT("local recovery"),M->Freeform->Jobs().Num());
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
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float Dt){return Run->Step(Dt);}));
}
