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
  bool bDone=false,bFailed=false;
  struct FGroundRecord
  {
   double CuriousStart=0,CuriousTravel=0,HuntStart=0,HuntTravel=0;
   int32 CuriousContacts=0,CuriousSounds=0,HuntContacts=0,HuntSounds=0;
   bool bHuntContact=false;
  };
  TMap<int32,FGroundRecord> Ground;
  double NextReport=0;
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
   if (!Ok) { bDone=true; bFailed=true; } return Ok;
  }
  void Shot(const TCHAR* Name)
  { FScreenshotRequest::RequestScreenshot(Dir/(FString::Printf(TEXT("%02d-"),Shots++)+Name+TEXT(".png")),true,false); }
  bool Step(US47Main* M,double Dt)
  {
   if (bDone)
   {
    if (M) { M->bPausedByMenu=false; if (M->Player) { M->Player->TestMove.Reset(); M->Player->bTestRun=false; }
     if (M->Night && M->Night->Glimpses) M->Night->Glimpses->DevClear(); }
    if (bFailed) FPlatformMisc::RequestExitWithStatus(false,1);
    return false;
   }
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
    M->UI->ClearToasts(); M->Player->TestMove.Reset(); M->Player->bTestRun=false; G.DevClear(); Phase=1; T=0;
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
    const auto Spawned=G.Individuals();
    if (!Check(Spawned.Num()==6,TEXT("curiosity begins with six separate ground individuals"))) return true;
    Ground.Reset();
    for (const auto& V:Spawned)
    {
     if (!Check(!V.bRoof,TEXT("curiosity subject is actually on the ground"))) return true;
     FGroundRecord R; R.CuriousStart=V.Distance; Ground.Add(V.Id,R);
    }
    Phase=5; T=0; NextReport=3.; return true;
   }
   if (Phase==5)
   {
    if (Views.Num()!=6) { Check(false,TEXT("all six ground curiosity subjects remain present")); return true; }
    for (const auto& V:Views)
    {
     auto* R=Ground.Find(V.Id);
     if (!R || V.bRoof || V.Mode!=FS47RoswellBrain::EMode::Curious || V.State==FS47RoswellBrain::EState::Hunt)
     { Check(false,TEXT("ordinary curiosity never turns into hunt")); return true; }
     R->CuriousTravel=FMath::Max(R->CuriousTravel,V.Distance-R->CuriousStart);
     R->CuriousContacts=FMath::Max(R->CuriousContacts,V.Contacts); R->CuriousSounds=FMath::Max(R->CuriousSounds,V.Sounds);
     if (T>=NextReport) UE_LOG(LogTemp,Display,TEXT("S47: Roswell ground curious t=%.1f id=%d pos=%s state=%s travel=%.3f contacts=%d sounds=%d"),
      T,V.Id,*V.Pos.ToCompactString(),FS47RoswellBrain::StateName(V.State),R->CuriousTravel,R->CuriousContacts,R->CuriousSounds);
    }
    if (T>=NextReport) NextReport+=3.;
    if (T>2.5+Shots*.25 && Shots<7) Shot(TEXT("curious-motion"));
    if (T>12)
    {
     bool EveryGround=true;
     for (const auto& V:Views)
     {
      auto& R=Ground.FindChecked(V.Id); R.HuntStart=V.Distance;
      R.HuntContacts=V.Contacts; R.HuntSounds=V.Sounds;
      const bool Ok=R.CuriousTravel>.2 && R.CuriousContacts>0 && R.CuriousSounds>0;
      UE_LOG(LogTemp,Display,TEXT("S47: Roswell ground curious result id=%d travel=%.3f contacts=%d sounds=%d pass=%d"),
       V.Id,R.CuriousTravel,R.CuriousContacts,R.CuriousSounds,Ok?1:0);
      EveryGround&=Ok;
     }
     if (!Check(EveryGround,TEXT("every one of six ground curious aliens moves over 0.2m with its own foot contacts and sounds"))) return true;
     if (!Check(Action(M,TEXT("Change active group to hunt")),TEXT("explicit hunt action changes the existing six individuals"))) return true;
     Phase=6; T=0; NextReport=3.; return true;
    }
   }
   if (Phase==6)
   {
    if (Views.Num()!=6) { Check(false,TEXT("all six original hunt subjects remain present")); return true; }
    // Walk away using the ordinary walker and collision, giving the group a
    // fresh audible/visible target instead of piling into a stationary contact.
    // Two 1.6-second walks are about three metres each at normal walking speed.
    const bool Walking=(T>.3 && T<1.9) || (T>6. && T<7.6);
    M->Player->TestMove=Walking?FVector2D(0,1):FVector2D::ZeroVector;
    for (const auto& V:Views)
    {
     auto* R=Ground.Find(V.Id);
     if (!R || V.bRoof || V.Mode!=FS47RoswellBrain::EMode::Hunt)
     { Check(false,TEXT("hunt retains each original ground subject and explicit mode")); return true; }
     R->HuntTravel=FMath::Max(R->HuntTravel,V.Distance-R->HuntStart);
     R->bHuntContact|=V.State==FS47RoswellBrain::EState::Contact;
     if (T>=NextReport) UE_LOG(LogTemp,Display,TEXT("S47: Roswell ground hunt t=%.1f id=%d pos=%s state=%s new-travel=%.3f contacts=%d sounds=%d caught=%d"),
      T,V.Id,*V.Pos.ToCompactString(),FS47RoswellBrain::StateName(V.State),R->HuntTravel,V.Contacts-R->HuntContacts,V.Sounds-R->HuntSounds,R->bHuntContact?1:0);
    }
    if (T>=NextReport) NextReport+=3.;
    if (T>1 && Shots<9) Shot(TEXT("hunt-motion"));
    if (T>14)
    {
     M->Player->TestMove.Reset();
     bool EveryHunter=true;
     for (const auto& V:Views)
     {
      const auto& R=Ground.FindChecked(V.Id); const bool Ok=R.HuntTravel>.2 || R.bHuntContact;
      UE_LOG(LogTemp,Display,TEXT("S47: Roswell ground hunt result id=%d new-travel=%.3f contacts=%d sounds=%d caught=%d pass=%d"),
       V.Id,R.HuntTravel,V.Contacts-R.HuntContacts,V.Sounds-R.HuntSounds,R.bHuntContact?1:0,Ok?1:0);
      EveryHunter&=Ok;
     }
     if (!Check(EveryHunter,TEXT("every one of six hunters independently moves over 0.2m after mode change or contacts the player"))) return true;
     if (!Check(Action(M,TEXT("Clear Roswell group")),TEXT("actual clear developer action"))) return true;
     if (!Check(G.Individuals().IsEmpty(),TEXT("clear developer action removes every individual"))) return true;
     if (!Check(Action(M,TEXT("Roof: curious footsteps")),TEXT("actual roof developer action"))) return true;
     Phase=7; T=0; return true;
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
    if (!bDone)
    {
     if (bRoofOnly) { UE_LOG(LogTemp,Display,TEXT("S47: Roswell AI COMPLETE: roof-only movement/contact/sound sample; ground group was not exercised")); }
     else { UE_LOG(LogTemp,Display,TEXT("S47: Roswell AI COMPLETE: six individually verified ground curiosity/hunt subjects, stand/pause/roof/clear; not a full natural encounter playthrough")); }
    }
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
