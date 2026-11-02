#include "S47ReactiveLightTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47RoomLights.h"
#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47UI.h"
#include "S47Startup.h"
#include "S47Three.h"
#include "Containers/Ticker.h"
#include "Components/RectLightComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace
{
 struct FRun
 {
  TWeakObjectPtr<US47Main> Main;
  FString Root;bool bFinished=false;
  int32 Stage=0,Index=0,Serial=0,Restores=0;
  double Age=0,At=0,ProgressAt=0;
  FVector Last=FVector::ZeroVector;
  TArray<FVector> Route;
  TArray<TSharedPtr<FJsonValue>> Checks;
  S47J::FObj BeforeForce;
  bool Finish(bool Good,const FString& Why)
  {
   bFinished=true;
   if(auto* M=Main.Get())if(M->Player){M->Player->TestMove.Reset();M->Player->bTestRun=false;}
   auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("seconds"),Age);S47J::Set(O,TEXT("flowRestores"),Restores);O->SetArrayField(TEXT("checks"),Checks);if(BeforeForce)S47J::Set(O,TEXT("beforeForce"),BeforeForce);
   S47J::Set(O,TEXT("method"),TEXT("Isolated developer Freeform case0; initial switch-side placement fixture only. Native eye-ray Pick/Walker OnUse OFF, actual TestMove walking through the production east door to >25m and back, ordinary active 60-second candidate aging. Before the first restore, the sole scheduler fixture delays director.nextAt to actual activeTime+600 to avoid a competing random incident; Force selects only the reactive incident (no candidate-gate bypass). Full disk JSON/Flow restores before effect, after effect and after physical EXAMINE. Final real ray OnUse invalidates the override. No completed findings, actor positions along the route, input serials, candidate age or other Director state injected."));
   S47J::Set(O,TEXT("limits"),TEXT("Developer camera aim and authored foot-route inputs are automated. Competing random-choice timer is deliberately delayed in the first saved fixture; this does not test natural scheduling frequency. Not human-input acceptance or a full-night regression. Initial approach placement is explicit. Timeout180s."));
   if(!Root.IsEmpty())FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("result.json")));
   UE_LOG(LogTemp,Display,TEXT("S47 reactive light %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),*Why);FPlatformMisc::RequestExitWithStatus(false,Good?0:1);return false;
  }
  bool Check(bool Good,const FString& Why)
  {
   auto O=S47J::New();S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("check"),Why);Checks.Add(MakeShared<FJsonValueObject>(O));
   UE_LOG(LogTemp,Display,TEXT("S47 reactive light check %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),*Why);if(!Good)Finish(false,Why);return Good;
  }
  void Next(int32 S){Stage=S;At=Age;UE_LOG(LogTemp,Display,TEXT("S47 reactive light stage %d at %.2fs"),Stage,Age);}
  void Aim(const FVector& Target)
  {
   auto* M=Main.Get();const FVector D=Target-S47Three::ToThree(M->Player->EyeUE());M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
  }
  bool NativeLight(bool On)
  {
   auto* M=Main.Get();int32 Found=0,Lit=0;TArray<URectLightComponent*> Lights;M->Web->GetComponents(Lights);
   for(auto* L:Lights)if(L&&L->ComponentHasTag(FName(TEXT("S47RoomCeiling:control")))){++Found;if(L->IsVisible()&&!L->bHiddenInGame&&L->Intensity>.001f)++Lit;}
   return Check(Found>0&&(On?Lit>0:Lit==0),On?TEXT("actual control-room ceiling lights are on"):TEXT("actual control-room ceiling lights are off"));
  }
  bool PickUse(const FString& Id)
  {
   auto* M=Main.Get();const FString Hit=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
   if(Hit!=Id)UE_LOG(LogTemp,Warning,TEXT("S47 reactive ray miss requested=%s actual=%s player=%s eye=%s forward=%s yaw=%.4f pitch=%.4f"),*Id,*Hit,*M->Player->Pos.ToString(),*S47Three::ToThree(M->Player->EyeUE()).ToString(),*S47Three::ToThree(M->Player->EyeUE()+M->Player->ForwardUE()*100).ToString(),M->Player->Yaw,M->Player->Pitch);
   if(!Check(Hit==Id&&bool(M->Player->OnUse),TEXT("native eye ray use: ")+Id+TEXT(" hit=")+Hit))return false;
   M->Player->OnUse(Hit);return true;
  }
  void Walk(std::initializer_list<FVector> Points,int32 S)
  {
   Route=Points;Index=0;ProgressAt=Age;Last=Main.Get()->Player->Pos;Next(S);
  }
  bool Walking()
  {
   AS47Walker* P=Main.Get()->Player.Get();
   if(Index>=Route.Num()){P->TestMove.Reset();return false;}
   const FVector D=Route[Index]-P->Pos;const double Distance=FVector2D(D.X,D.Z).Size();
   if(Distance<.13){++Index;P->TestMove.Reset();ProgressAt=Age;return true;}
   if(FVector2D::Distance({Last.X,Last.Z},{P->Pos.X,P->Pos.Z})>.04){Last=P->Pos;ProgressAt=Age;}
   if(Age-ProgressAt>10){Finish(false,FString::Printf(TEXT("physical foot route blocked at waypoint %d / %s"),Index,*P->Pos.ToString()));return true;}
   P->Yaw=FMath::Atan2(-D.X,-D.Z);P->Pitch=0;P->SetActive(true);P->TestMove=FVector2D(0,-FMath::Min(1.,Distance*2));return true;
  }
  bool Restore(const FString& Name,int32 NextStage)
  {
   auto* M=Main.Get();M->Player->TestMove.Reset();const FString Path=Root/(Name+TEXT(".json"));FString Text;
   auto State=M->Flow->GameState();
   if(Name==TEXT("before-effect"))
   {
    // Scheduler-only fixture: preserve real OFF provenance/age and all other
    // actual Flow state, postponing only the next competing random choice.
    auto Director=S47J::Obj(S47J::Obj(State,TEXT("freeform")),TEXT("director"));
    if(!Check(bool(Director),TEXT("scheduler fixture targets the actual saved Freeform Director")))return false;
    S47J::Set(Director,TEXT("nextAt"),S47J::Num(Director,TEXT("activeTime"))+600.);
   }
   if(!Check(FFileHelper::SaveStringToFile(S47J::Write(State),*Path)&&FFileHelper::LoadFileToString(Text,*Path),TEXT("full JSON write/read: ")+Name))return false;
   US47Flow::FStart S;S.bDev=true;S.State=S47J::Read(Text);if(!Check(bool(S.State),TEXT("valid retained JSON: ")+Name))return false;
   ++Restores;M->Flow->StartGame(S);Next(NextStage);return true;
  }
  bool Step(float Dt)
  {
   if(bFinished)return false;
   Age+=FMath::Clamp(double(Dt),0.,.25);if(Age>180)return Finish(false,TEXT("180 second bounded timeout"));
   auto* M=Main.Get();if(!M||!M->Flow||!M->Freeform||!M->RoomLights||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->Inter||M->Flow->bStarting||S47Startup::Blocking())return true;
   if(Stage==0){US47Flow::FStart S;S.bDev=true;S.bFreeform=true;M->Flow->StartGame(S);Next(1);return true;}
   if(Age-At<.8)return true;
   auto& L=*M->RoomLights;auto& F=*M->Freeform;
   if(Stage==1)
   {
    if(!Check(M->IsFreeform()&&M->Flow->CaseId==0&&M->Flow->bDevRun&&L.InputSerial()==0,TEXT("fresh isolated Freeform has no invented manual-input provenance")))return false;
    M->UI->Close(true);const FVector P=L.ApproachPoint(TEXT("control"));M->Player->Place(P.X,P.Z,0);M->Player->bHeadMotion=false;Next(20);return true;
   }
   if(Stage==20){Aim(L.InteractionPoint(TEXT("control")));Next(2);return true;}
   if(Stage==2)
   {
    if(!PickUse(TEXT("roomlight:control")))return false;Serial=L.InputSerial();
    if(!Check(Serial>0&&!L.RoomEnabled(TEXT("control"))&&!F.Director->Force(TEXT("roomlight_return")),TEXT("real OFF arms input but cannot trigger before physical departure and age")))return false;
    return Restore(TEXT("before-effect"),3);
   }
   if(Stage==3)
   {
    if(!Check(L.InputSerial()==Serial&&!L.RoomEnabled(TEXT("control")),TEXT("Continue preserves actual OFF candidate without relighting"))||!NativeLight(false))return false;
    Walk({{-3.25,0,2.7},{-3.25,0,-1.2},{0,0,-1.2},{4.2,0,-1.2},{4.2,0,1.6}},4);return true;
   }
   if(Stage==4)
   {
    if(Walking())return true;
    // Authored doors belong to Web ItemList, with Inter fallback use/label.
    // Dynamically registered Inter objects take precedence, as in HorrorTest.
    FS47Obj Door;
    if(const auto* Dynamic=M->Inter->Get(TEXT("doorEast")))Door=Dynamic->Object;
    if(!Door)if(const auto* Authored=M->Web->Item(TEXT("doorEast")))Door=M->Web->ObjAt(Authored->Node);
    if(!Check(bool(Door)&&M->Web->IsDoor(TEXT("doorEast")),TEXT("east-door object resolves through dynamic Inter or authored Web fallback")))return false;
    const FBox Bounds=Door.WorldBox();Aim(Bounds.IsValid?Bounds.GetCenter():Door.WorldPosition());Next(5);return true;
   }
   if(Stage==5)
   {
    if(!M->Web->IsDoorOpen(TEXT("doorEast"))&&!PickUse(TEXT("doorEast")))return false;
    // Existing SC04 exterior withdrawal: walk -> gSteps -> gNorth -> west.
    // The landing ends at x11; never aim straight through its east boundary.
    Walk({{5.2,0,1.6},{6.1,0,1.6},{9,0,1.6},{9.5,0,0},{9.5,0,-6},{9.5,0,-12},{5,0,-12},{5,0,-8},{-2,0,-8},{-9,0,-8},{-26,0,-8}},6);return true;
   }
   if(Stage==6){if(Walking())return true;M->Player->Yaw=PI/2;M->Player->Pitch=0;Next(7);return true;}
   if(Stage==7)
   {
    const auto R=S47J::Obj(F.Director->Save(),TEXT("reactiveLight"));if(S47J::Num(R,TEXT("age"))<61)return true;
    BeforeForce=S47J::New();S47J::Set(BeforeForce,TEXT("director"),F.Director->Save());S47J::Set(BeforeForce,TEXT("roomLights"),L.Save());
    const FVector P=M->Player->Pos,Eye=S47Three::ToThree(M->Player->EyeUE());
    S47J::Set(BeforeForce,TEXT("player"),TArray<double>{P.X,P.Y,P.Z});S47J::Set(BeforeForce,TEXT("eye"),TArray<double>{Eye.X,Eye.Y,Eye.Z});S47J::Set(BeforeForce,TEXT("yaw"),M->Player->Yaw);S47J::Set(BeforeForce,TEXT("pitch"),M->Player->Pitch);
    FFileHelper::SaveStringToFile(S47J::Write(BeforeForce,true),*(Root/TEXT("before-force.json")));
    if(!Check(S47J::Bool(R,TEXT("left"))&&!L.PlayerInRoom(TEXT("control"))&&F.Director->Force(TEXT("roomlight_return")),TEXT("aged actual departure permits selecting one ordinary-context reactive incident")))return false;
    Next(8);return true;
   }
   if(Stage==8)
   {
    if(!Check(L.EventLightMatches(TEXT("control"),Serial)&&!L.ManualEnabled(TEXT("control"))&&L.RoomEnabled(TEXT("control"))&&F.HasFinding(FString::Printf(TEXT("roomlight-original-%d"),Serial)),TEXT("event turns on physical room while retaining OFF preference and original operation")))return false;
    if(!NativeLight(true))return false;return Restore(TEXT("after-effect"),9);
   }
   if(Stage==9)
   {
    if(!Check(L.EventLightMatches(TEXT("control"),Serial)&&S47J::Bool(F.Director->Save(),TEXT("effectsApplied")),TEXT("Continue keeps already-applied light without replay")))return false;
    Walk({{-9,0,-8},{-2,0,-8},{5,0,-8},{5,0,-12},{9.5,0,-12},{9.5,0,-6},{9.5,0,0},{9,0,1.6},{6.1,0,1.6},{5.2,0,1.6},{4.2,0,1.6},{4.2,0,-1.2},{0,0,-1.2},{-3.25,0,-1.2},{-3.25,0,2.7},L.ApproachPoint(TEXT("control"))},10);return true;
   }
   if(Stage==10){if(Walking())return true;Aim(L.InteractionPoint(TEXT("control")));Next(11);return true;}
   if(Stage==11)
   {
    FString Id,Text;
    if(!Check(M->Inter->ExamineAt(M->Player->EyeUE(),M->Player->ForwardUE(),Id,Text)&&Id==TEXT("roomlight:control")&&Text.Contains(TEXT("RETAINED CHECK"))&&F.HasFinding(FString::Printf(TEXT("roomlight-control-%d"),Serial)),TEXT("physical EXAMINE compares and archives actual ON readback before manual toggle")))return false;
    return Restore(TEXT("after-inspection"),12);
   }
   if(Stage==12)
   {
    if(!Check(F.HasFinding(FString::Printf(TEXT("roomlight-control-%d"),Serial))&&L.EventLightMatches(TEXT("control"),Serial),TEXT("archived readback and applied lamp survive third Continue")))return false;
    Aim(L.InteractionPoint(TEXT("control")));Next(13);return true;
   }
   if(!PickUse(TEXT("roomlight:control")))return false;
   if(!Check(!L.EventLightMatches(TEXT("control"),Serial)&&L.InputSerial()!=Serial&&!L.RoomEnabled(TEXT("control"))&&F.HasFinding(FString::Printf(TEXT("roomlight-control-%d"),Serial)),TEXT("new real switch input clears override without erasing retained evidence")))return false;
   return Finish(true,TEXT("physical OFF, walked departure and return, applied-once event, three Continue restores, inspected archive and manual invalidation passed"));
  }
 };
}
void S47ReactiveLightTest::Start(US47Main* Main)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47ReactiveLightTest")))return;
 auto Run=MakeShared<FRun>();Run->Main=Main;FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
 if(!User.StartsWith(TEXT("/tmp/S47-reactive-light-"))){Run->Finish(false,TEXT("explicit isolated temporary profile required"));return;}
 Run->Root=FPaths::ProjectSavedDir()/TEXT("ReactiveLightTest");IFileManager::Get().MakeDirectory(*Run->Root,true);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float Dt){return Run->Step(Dt);}));
}
