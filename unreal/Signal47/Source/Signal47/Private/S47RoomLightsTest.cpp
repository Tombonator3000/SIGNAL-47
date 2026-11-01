#include "S47RoomLightsTest.h"
#include "S47Main.h"
#include "S47RoomLights.h"
#include "S47Office.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47SaroNight.h"
#include "S47SaroPower.h"
#include "S47Inter.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47UI.h"
#include "S47Startup.h"
#include "S47Three.h"
#include "Components/RectLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Containers/Ticker.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"
#include "Engine/World.h"

namespace
{
 const TCHAR* Rooms[]={TEXT("control"),TEXT("corridor"),TEXT("records"),TEXT("office")};
 struct FRun
 {
  TWeakObjectPtr<US47Main> Main;
  int32 Stage=0,Room=0;double Age=0,At=0;
  FString Root;S47J::FObj Restore;
  TArray<TSharedPtr<FJsonValue>> Checks;
  bool Finish(bool Good,const FString& Why)
  {
   auto O=S47J::New();S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("reason"),Why);O->SetArrayField(TEXT("checks"),Checks);
   S47J::Set(O,TEXT("scope"),TEXT("Isolated developer Freeform case0. Explicit room placement and camera settling, actual Inter.Pick/Walker.OnUse for four wall switches. Native rect-light intensity/visibility, CRT output, upstream SaroPower Cut/Restore fixture, full JSON/Flow restore and legacy missing-key restore. No natural walking, Story progression or wall-bug reproduction claimed."));
   if(!Root.IsEmpty())FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("result.json")));
   UE_LOG(LogTemp,Display,TEXT("S47 room lights %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),*Why);
   FPlatformMisc::RequestExitWithStatus(false,Good?0:1);return false;
  }
  bool Check(bool Good,const FString& Why)
  {
   auto O=S47J::New();S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("check"),Why);Checks.Add(MakeShared<FJsonValueObject>(O));
   UE_LOG(LogTemp,Display,TEXT("S47 room lights check %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),*Why);
   if(!Good)Finish(false,Why);return Good;
  }
  void Next(int32 S){Stage=S;At=Age;}
  void Place(int32 I)
  {
   auto* M=Main.Get();const FVector P=M->RoomLights->ApproachPoint(Rooms[I]);
   // Stand on the reachable room side, never open a Story door or inject a key.
   M->Player->Place(P.X,P.Z,0);M->Player->bHeadMotion=false;M->Player->SetActive(false);
  }
  void Aim(int32 I)
  {
   auto* M=Main.Get();const FVector D=M->RoomLights->InteractionPoint(Rooms[I])-S47Three::ToThree(M->Player->EyeUE());
   M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
  }
  bool CaptureRoom() const { return Room==0||Room==3; }
  void ViewRoom()
  {
   auto* M=Main.Get();const FVector Target=Room==0?FVector(0,1.35,0):FVector(.47,1.35,9.4);
   const FVector D=Target-S47Three::ToThree(M->Player->EyeUE());
   M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
  }
  FString ImagePath(bool On) const { return Root/(FString(Rooms[Room])+(On?TEXT("-on.png"):TEXT("-off.png"))); }
  void Capture(bool On)
  {
   IFileManager::Get().Delete(*ImagePath(On),false,true);
   FScreenshotRequest::RequestScreenshot(ImagePath(On),true,false);
   UE_LOG(LogTemp,Display,TEXT("S47 room lights capture %s"),*ImagePath(On));
  }
  bool AdvanceRoom()
  {
   if(++Room<4){Place(Room);Next(2);return true;}
   Place(0);Next(5);return true;
  }
  void RayDiagnostic(const FString& Id)
  {
   auto* M=Main.Get();const FVector Eye=M->Player->EyeUE();
   UE_LOG(LogTemp,Warning,TEXT("S47 room lights ray miss %s eye=%s target=%s label=%s"),*Id,*S47Three::ToThree(Eye).ToString(),*M->RoomLights->InteractionPoint(Rooms[Room]).ToString(),*M->RoomLights->Label(Id));
   TArray<FHitResult> Hits;FCollisionQueryParams Params;
   M->GetWorld()->LineTraceMultiByChannel(Hits,Eye,Eye+M->Player->ForwardUE()*220.,ECC_GameTraceChannel1,Params);
   for(const auto& H:Hits)UE_LOG(LogTemp,Warning,TEXT("S47 room lights ray component=%s distance=%.2f blocking=%d"),*GetNameSafe(H.GetComponent()),H.Distance,H.bBlockingHit);
  }
  bool Native(const FString& Tag,bool On)
  {
   auto* M=Main.Get();int32 Found=0,Lit=0;float Sum=0;TArray<URectLightComponent*> Lights;M->Web->GetComponents(Lights);
   for(auto* L:Lights)if(L&&L->ComponentHasTag(FName(*Tag)))
   {++Found;Sum+=L->Intensity;if(L->IsVisible()&&!L->bHiddenInGame&&L->Intensity>.001f)++Lit;}
   return Check(Found>0&&(On?Lit>0:(Lit==0&&Sum<.001f)),FString::Printf(TEXT("native %s %s: components=%d lit=%d lumens=%.4f"),*Tag,On?TEXT("on"):TEXT("off"),Found,Lit,Sum));
  }
  bool Choices(bool On)
  {
   auto* M=Main.Get();const auto O=M->RoomLights->Save();bool Good=true;
   for(const TCHAR* R:Rooms)Good&=S47J::Bool(O,R,!On)==On;
   return Check(Good,On?TEXT("all four saved choices default on"):TEXT("all four saved choices remain off"));
  }
  bool StartState(const S47J::FObj& State)
  {
   auto* M=Main.Get();US47Flow::FStart S;S.bDev=true;S.State=State;M->Flow->StartGame(S);return true;
  }
  bool Step(float Dt)
  {
   Age+=Dt;if(Age>150)return Finish(false,TEXT("150 second timeout"));
   auto* M=Main.Get();if(!M||!M->Flow||!M->RoomLights||!M->Office||!M->Web||!M->Web->IsLoaded()||!M->Inter||!M->Player||!M->Freeform||S47Startup::Blocking()||M->Flow->bStarting)return true;
   if(Stage==0){US47Flow::FStart S;S.bDev=true;S.bFreeform=true;M->Flow->StartGame(S);Next(1);return true;}
   if(!M->Office->Ready()||Age-At<.8)return true;
   M->Player->SetActive(false);
   if(Stage==1)
   {
    if(!Check(M->IsFreeform()&&M->Flow->CaseId==0&&M->Flow->bDevRun,TEXT("fresh developer Freeform case0"))||!Choices(true))return false;
    M->UI->Close(true);Place(Room);Next(2);return true;
   }
   if(Stage==2)
   {
    if(CaptureRoom()){ViewRoom();Next(20);}else{Aim(Room);Next(3);}return true;
   }
   if(Stage==20){Capture(true);Next(21);return true;}
   if(Stage==21)
   {
    if(!IFileManager::Get().FileExists(*ImagePath(true))&&Age-At<5)return true;
    if(!Check(IFileManager::Get().FileExists(*ImagePath(true)),TEXT("before screenshot saved: ")+ImagePath(true)))return false;
    Aim(Room);Next(3);return true;
   }
   if(Stage==40){Capture(false);Next(41);return true;}
   if(Stage==41)
   {
    if(!IFileManager::Get().FileExists(*ImagePath(false))&&Age-At<5)return true;
    if(!Check(IFileManager::Get().FileExists(*ImagePath(false)),TEXT("after screenshot saved: ")+ImagePath(false)))return false;
    return AdvanceRoom();
   }
   if(Stage==3)
   {
    if(!Native(FString(TEXT("S47RoomCeiling:"))+Rooms[Room],true))return false;
    if(Room==0&&!Native(TEXT("S47CrtSurface"),true))return false;
    const FString Want=FString(TEXT("roomlight:"))+Rooms[Room];const FString Hit=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
    if(Hit!=Want)RayDiagnostic(Want);
    if(!Check(Hit==Want&&bool(M->Player->OnUse),TEXT("actual camera ray: ")+Want+TEXT(" hit=")+Hit))return false;
    M->Player->OnUse(Hit);Next(4);return true;
   }
   if(Stage==4)
   {
    if(!Check(!S47J::Bool(M->RoomLights->Save(),Rooms[Room],true),FString(TEXT("physical switch saves off: "))+Rooms[Room])||!Native(FString(TEXT("S47RoomCeiling:"))+Rooms[Room],false))return false;
    if(Room==0&&!Native(TEXT("S47CrtSurface"),true))return false;
    if(CaptureRoom()){ViewRoom();Next(40);return true;}
    return AdvanceRoom();
   }
   if(Stage==5)
   {
    if(!Choices(false)||!Native(TEXT("S47CrtSurface"),true)||!Check(M->SaroNight&&M->SaroNight->Power.IsValid(),TEXT("upstream SARO supply exists")))return false;
    M->SaroNight->Power->Cut();Next(6);return true;
   }
   if(Stage==6)
   {
    if(!Choices(false)||!Native(TEXT("S47RoomCeiling:control"),false)||!Native(TEXT("S47CrtSurface"),false))return false;
    M->SaroNight->Power->Restore();Next(7);return true;
   }
   if(Stage==7)
   {
    if(!Choices(false)||!Native(TEXT("S47RoomCeiling:control"),false)||!Native(TEXT("S47CrtSurface"),true))return false;
    const FString File=Root/TEXT("room-lights-roundtrip.json");FString Text;
    if(!Check(FFileHelper::SaveStringToFile(S47J::Write(M->Flow->GameState()),*File)&&FFileHelper::LoadFileToString(Text,*File),TEXT("ordinary full GameState JSON written/read")))return false;
    Restore=S47J::Read(Text);if(!Check(bool(Restore)&&bool(S47J::Obj(Restore,TEXT("roomLights"))),TEXT("GameState includes roomLights")))return false;
    StartState(Restore);Next(8);return true;
   }
   if(Stage==8)
   {
    if(!Choices(false)||!Check(M->IsFreeform()&&!M->Office->WantsDeparture()&&!M->Office->DepartureReleased(),TEXT("Flow restore stays Freeform without Story departure")))return false;
    Place(0);Next(9);return true;
   }
   if(Stage==9)
   {
    if(!Native(TEXT("S47RoomCeiling:control"),false)||!Native(TEXT("S47CrtSurface"),true))return false;
    Restore=M->Flow->GameState();Restore->RemoveField(TEXT("roomLights"));StartState(Restore);Next(10);return true;
   }
   if(Stage==10){if(!Choices(true))return false;Place(0);Next(11);return true;}
   if(!Native(TEXT("S47RoomCeiling:control"),true)||!Native(TEXT("S47CrtSurface"),true))return false;
   return Finish(true,TEXT("four physical switches, independent equipment power, supply restoration, Continue JSON and legacy defaults passed"));
  }
 };
}
void S47RoomLightsTest::Start(US47Main* Main)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47RoomLightsTest")))return;
 auto Run=MakeShared<FRun>();Run->Main=Main;FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
 if(!User.StartsWith(TEXT("/tmp/S47-room-lights-"))){Run->Finish(false,TEXT("explicit isolated temporary profile required"));return;}
 Run->Root=FPaths::ProjectSavedDir()/TEXT("RoomLightsTest");IFileManager::Get().MakeDirectory(*Run->Root,true);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float Dt){return Run->Step(Dt);}));
}
