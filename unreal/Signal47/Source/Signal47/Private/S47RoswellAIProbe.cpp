#include "S47RoswellAIProbe.h"
#include "S47RoswellAudio.h"
#include "S47DevEvents.h"
#include "S47Glimpses.h"
#include "S47Flow.h"
#include "S47Flashlight.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Containers/Ticker.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

// A bounded opt-in exercise of the ordinary group and actual developer actions.
// Uses an isolated new case; never opens or writes a personal Continue save.
namespace
{
 struct FProbe
 {
  FString Dir; int32 Phase=0,Shots=0; double T=0,Age=0,Distance=0;
  bool bRoofOnly=false;
  bool bDone=false,bMotion=false,bHunt=false; int32 Contacts=0,Sounds=0;
  bool Action(US47Main* M,const FString& Label)
  {
   if (M->UI && M->UI->Modal()) M->UI->Close(true);
   for (const auto& E:M->DevEvents->Events()) if (E.Label==Label)
   { const auto Fn=E.Fn; Fn(); if (M->UI && M->UI->Modal()) M->UI->Close(true); return true; }
   return false;
  }
  bool Check(bool Ok,const TCHAR* What)
  {
   UE_LOG(LogTemp,Display,TEXT("S47: Roswell AI %s: %s"),Ok?TEXT("ok"):TEXT("FAIL"),What);
   if (!Ok) bDone=true; return Ok;
  }
  void Shot(const TCHAR* Name)
  { FScreenshotRequest::RequestScreenshot(Dir/(FString::Printf(TEXT("%02d-"),Shots++)+Name+TEXT(".png")),true,false); }
  bool Step(US47Main* M,double Dt)
  {
   if (bDone) { M->bPausedByMenu=false; M->Night->Glimpses->DevClear(); return false; }
   if (!M || !M->Web || !M->Web->IsLoaded() || !M->Flow || !M->World || !M->Player || !M->Night || !M->DevEvents ||
    S47Startup::Blocking() || M->Flow->bStarting || M->World->bBusy || M->Mode!=US47Main::EMode::Play) return true;
   auto& G=*M->Night->Glimpses;
   T+=FMath::Min(Dt,.1);
   if (Phase==0)
   {
    FString UserDir; FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);
    if (!Check(M->Flow->bDevRun && M->Flow->CaseId==0 && UserDir.StartsWith(TEXT("/tmp/")) &&
     FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),FPaths::ConvertRelativePathToFull(UserDir)),TEXT("isolated Case0 /tmp save path"))) return true;
    if (!Check(M->World->Area==TEXT("saro") && !M->World->bDriving && G.HasNativeBody(),TEXT("native Roswell in actual SARO"))) return true;
    IFileManager::Get().MakeDirectory(*Dir,true);
    M->Player->Place(6.95,-8.35,PI); M->Player->Pitch=-.16;
    M->Story->SetClock(22*3600);
    if (M->Night->Flashlight) { M->Night->Flashlight->bHave=true; M->Night->Flashlight->bOn=true; }
    M->UI->ClearToasts(); G.DevClear(); Phase=1; T=0;
    if (bRoofOnly) { Action(M,TEXT("Roof: curious footsteps")); Phase=7; } return true;
   }
   if (Phase==1 && T>2)
   {
    Shot(TEXT("before"));
    for (int I=0;I<3;++I)
    { FString Label; for (const auto& E:M->DevEvents->Events()) if (E.Group==TEXT("Roswell aliens") && E.Label.StartsWith(TEXT("Number:"))) Label=E.Label; Action(M,Label); }
    if (!Check(Action(M,TEXT("Stand for 8 seconds")),TEXT("actual stand developer action"))) return true;
    Phase=2; T=0; return true;
   }
   auto Views=G.Individuals();
   if (Phase==2 && T>1)
   {
    if (!Check(Views.Num()==6,TEXT("six independent native individuals placed"))) return true;
    for (const auto& V:Views) UE_LOG(LogTemp,Display,TEXT("S47: Roswell AI body id=%d age=%.2f pos=%s rendered=%d"),V.Id,V.Age,*V.Pos.ToCompactString(),V.bRendered?1:0);
    if (!Check(Views[0].Age>.1 && Views.ContainsByPredicate([](const auto& V){return V.bRendered;}),TEXT("native group is ticking and rendered"))) return true;
    Age=Views[0].Age; Distance=Views[0].Distance; Shot(TEXT("six-standing"));
    M->bPausedByMenu=true; Phase=3; T=0; return true;
   }
   if (Phase==3 && T>1)
   {
    if (!Check(Views.Num()==6 && FMath::Abs(Views[0].Age-Age)<.01 && Views[0].Distance==Distance,TEXT("menu pause freezes clocks and planted bodies"))) return true;
    M->bPausedByMenu=false; Phase=4; T=0; return true;
   }
   if (Phase==4 && (Views.IsEmpty() || T>13))
   {
    if (!Check(Views.IsEmpty(),TEXT("stand mode ends at eight active seconds"))) return true;
    if (!Check(Action(M,TEXT("Curious: stalk and hide")),TEXT("actual curious developer action"))) return true;
    Phase=5; T=0; return true;
   }
   if (Phase==5)
   {
    for (const auto& V:Views)
    {
     if (V.Mode!=FS47RoswellBrain::EMode::Curious || V.State==FS47RoswellBrain::EState::Hunt)
     { Check(false,TEXT("ordinary curiosity never turns into hunt")); return true; }
     bMotion|=V.Distance>.2; Contacts=FMath::Max(Contacts,V.Contacts); Sounds=FMath::Max(Sounds,V.Sounds);
    }
    if (T>2.5+Shots*.25 && Shots<7) Shot(TEXT("curious-motion"));
    if (T>7)
    {
     if (!Check(bMotion && Contacts>0 && Sounds>0,TEXT("collision-resolved curiosity produces actual foot contacts"))) return true;
     Action(M,TEXT("Change active group to hunt")); Phase=6; T=0; return true;
    }
   }
   if (Phase==6)
   {
    for (const auto& V:Views) bHunt|=V.Mode==FS47RoswellBrain::EMode::Hunt && (V.Speed>1.3 || V.State==FS47RoswellBrain::EState::Contact);
    if (T>1 && Shots<9) Shot(TEXT("hunt-motion"));
    if (T>5)
    {
     if (!Check(bHunt,TEXT("explicit hunt reaches running or contact state"))) return true;
     Action(M,TEXT("Clear Roswell group"));
     if (!Check(G.Individuals().IsEmpty(),TEXT("clear developer action removes every individual"))) return true;
     Action(M,TEXT("Roof: curious footsteps")); Phase=7; T=0; return true;
    }
   }
   if (Phase==7 && T>10)
   {
    bool RoofMotion=false;
    for (const auto& V:Views) {
     UE_LOG(LogTemp,Display,TEXT("S47: Roswell roof id=%d age=%.2f pos=%s state=%s distance=%.3f contacts=%d"),V.Id,V.Age,*V.Pos.ToCompactString(),FS47RoswellBrain::StateName(V.State),V.Distance,V.Contacts);
     RoofMotion|=V.bRoof && FMath::Abs(V.Pos.Y-3.2)<.16 && V.Distance>.2 && V.Contacts>0 && V.Sounds>0; }
    if (!Check(RoofMotion,TEXT("physical Annex roof bodies move and make overhead contacts"))) return true;
    Shot(TEXT("roof"));
    Check(FS47RoswellAudio::RenderTour(Dir/TEXT("roswell-foley.wav")),TEXT("separate contact Foley listening sample"));
    Action(M,TEXT("Clear Roswell group"));
    UE_LOG(LogTemp,Display,TEXT("S47: Roswell AI COMPLETE: dev count/stand/pause/curiosity/motion/hunt/roof/clear; not a full natural encounter playthrough"));
    bDone=true;
   }
   return true;
  }
 };
}
void S47RoswellAIProbe::Start(US47Main* M)
{
 FString Dir;
 if (!M || !FParse::Value(FCommandLine::Get(),TEXT("S47RoswellAIContract="),Dir)) return;
 const auto P=MakeShared<FProbe>(); P->Dir=Dir; P->bRoofOnly=FParse::Param(FCommandLine::Get(),TEXT("S47RoswellRoofOnly")); const TWeakObjectPtr<US47Main> Weak=M;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,P](float Dt){return Weak.IsValid() && P->Step(Weak.Get(),Dt);}));
}
