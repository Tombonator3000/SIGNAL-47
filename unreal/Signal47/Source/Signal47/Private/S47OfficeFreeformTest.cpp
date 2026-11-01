#include "S47OfficeFreeformTest.h"
#include "S47Main.h"
#include "S47Office.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47Evening.h"
#include "S47Inter.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47UI.h"
#include "S47Startup.h"
#include "S47Three.h"
#include "Containers/Ticker.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace
{
 struct FRun
 {
  TWeakObjectPtr<US47Main> Main;
  int32 Stage=0;double Age=0,At=0;
  FString Root,StoryBefore;
  TArray<TSharedPtr<FJsonValue>> Checks;
  bool Finish(bool Good,const FString& Why)
  {
   auto O=S47J::New();S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("reason"),Why);O->SetArrayField(TEXT("checks"),Checks);
   S47J::Set(O,TEXT("scope"),TEXT("Fresh developer Freeform case0 in isolated /tmp profile. Explicit placement at service cabinet; actual camera ray Inter.Pick and Walker.OnUse for drawer, spare key and office door. Ordinary animation ticks; full GameState written/read as JSON and restored through Flow. No injected completion/key/door facts, natural walking, wall-bug reproduction or story completion claimed."));
   if(!Root.IsEmpty())FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("result.json")));
   UE_LOG(LogTemp,Display,TEXT("S47 office Freeform %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),*Why);
   FPlatformMisc::RequestExitWithStatus(false,Good?0:1);return false;
  }
  bool Check(bool Good,const FString& Why)
  {
   auto O=S47J::New();S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("check"),Why);Checks.Add(MakeShared<FJsonValueObject>(O));
   UE_LOG(LogTemp,Display,TEXT("S47 office Freeform check %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),*Why);
   if(!Good)Finish(false,Why);return Good;
  }
  void Next(int32 S){Stage=S;At=Age;}
  bool Aim(const FString& Id)
  {
   auto* M=Main.Get();const auto Obj=M->Office->Object(Id);
   if(!Check(Obj&&Obj.Shown(),TEXT("visible physical target: ")+Id))return false;
   const FBox Bounds=Obj.WorldBox();const FVector Target=Bounds.IsValid?Bounds.GetCenter():Obj.WorldPosition();
   const FVector D=Target-S47Three::ToThree(M->Player->EyeUE());
   M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());return true;
  }
  bool UseRay(const FString& Id,const FString& Alternate=FString())
  {
   auto* M=Main.Get();const FString Hit=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
   if(!Check(bool(M->Player->OnUse)&&(Hit==Id||(!Alternate.IsEmpty()&&Hit==Alternate)),FString::Printf(TEXT("actual camera ray picks %s (hit %s)"),*Id,*Hit)))return false;
   M->Player->OnUse(Hit);return true;
  }
  bool Step(float Dt)
  {
   Age+=Dt;if(Age>120)return Finish(false,TEXT("120 second timeout"));
   auto* M=Main.Get();if(!M||!M->Flow||!M->Office||!M->Web||!M->Web->IsLoaded()||!M->Inter||!M->Player||!M->Freeform||S47Startup::Blocking()||M->Flow->bStarting)return true;
   if(Stage==0){US47Flow::FStart S;S.bFreeform=true;S.bDev=true;M->Flow->StartGame(S);Next(1);return true;}
   if(!M->Office->Ready())return true;
   if(Age-At<.7)return true;
   if(Stage==1)
   {
    if(!Check(M->IsFreeform()&&M->Freeform->Active()&&M->Flow->CaseId==0&&M->Flow->bDevRun&&!M->Office->HasKey()&&!M->Office->DoorOpen(),TEXT("fresh isolated Freeform office starts locked")))return false;
    M->UI->Close(true);M->Player->Place(.45,5.75,0);M->Player->bHeadMotion=false;M->Player->SetActive(false);
    StoryBefore=M->Evening?S47J::Write(M->Evening->Pack.Save()):FString();Next(2);return true;
   }
   M->Player->SetActive(false);
   if(Stage==2){if(!Aim(TEXT("office:drawer")))return false;Next(3);return true;}
   if(Stage==3){if(!UseRay(TEXT("office:drawer")))return false;Next(4);return true;}
   if(Stage==4)
   {
    if(!M->Office->Object(TEXT("office:key")).Shown()&&Age-At<8)return true;
    if(!Check(S47J::Bool(M->Office->Save(),TEXT("drawerOpen")),TEXT("drawer target is open"))||!Aim(TEXT("office:key")))return false;
    Next(5);return true;
   }
   if(Stage==5)
   {
    if(!UseRay(TEXT("office:key"))||!Check(M->Office->HasKey(),TEXT("visible opened drawer spare is retained")))return false;
    if(!Aim(TEXT("office:door")))return false;Next(6);return true;
   }
   if(Stage==6){if(!UseRay(TEXT("office:door"),TEXT("officeDoor")))return false;Next(7);return true;}
   if(Stage==7)
   {
    if(!M->Office->DoorOpen()&&Age-At<8)return true;
    if(!Check(M->Office->DoorOpen()&&M->Office->WasUnlocked(),TEXT("ordinary office step finishes opening actual leaf")))return false;
    if(!Check(!M->Office->WantsDeparture()&&!M->Office->DepartureReleased()&&(!M->Evening||(!M->Evening->Enabled()&&S47J::Write(M->Evening->Pack.Save())==StoryBefore)),TEXT("room use leaves story controller and departure untouched")))return false;
    const auto State=M->Flow->GameState();const FString File=Root/TEXT("office-roundtrip.json");FString Text;
    if(!Check(FFileHelper::SaveStringToFile(S47J::Write(State),*File)&&FFileHelper::LoadFileToString(Text,*File),TEXT("full ordinary GameState written and read in temporary profile")))return false;
    const auto Read=S47J::Read(Text);if(!Check(bool(Read),TEXT("saved JSON parses")))return false;
    US47Flow::FStart S;S.bDev=true;S.State=Read;M->Flow->StartGame(S);Next(8);return true;
   }
   if(!Check(M->IsFreeform()&&M->Flow->CaseId==0&&M->Office->HasKey()&&M->Office->WasUnlocked()&&M->Office->DoorOpen()&&S47J::Bool(M->Office->Save(),TEXT("drawerOpen")),TEXT("Flow restore retains own spare, unlocked open door and open drawer")))return false;
   if(!Check(!M->Office->WantsDeparture()&&!M->Office->DepartureReleased()&&(!M->Evening||!M->Evening->Enabled()),TEXT("restored Freeform still cannot release story departure")))return false;
   return Finish(true,TEXT("physical drawer/key/door and complete save/restore passed"));
  }
 };
}

void S47OfficeFreeformTest::Start(US47Main* Main)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47OfficeFreeformTest")))return;
 auto Run=MakeShared<FRun>();Run->Main=Main;FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
 if(!User.StartsWith(TEXT("/tmp/S47-office-freeform-"))){Run->Finish(false,TEXT("explicit isolated temporary profile required"));return;}
 Run->Root=FPaths::ProjectSavedDir()/TEXT("OfficeFreeformTest");IFileManager::Get().MakeDirectory(*Run->Root,true);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float Dt){return Run->Step(Dt);}));
}
