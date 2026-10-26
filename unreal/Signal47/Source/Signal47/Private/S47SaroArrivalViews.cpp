// Opt-in player-height views of the arrival work. This is a camera fixture, not a driving test.
#include "S47Web.h"
#include "S47SaroArrival.h"
#include "S47Flow.h"
#include "S47Main.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47World.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47ArrivalViews, Log, All);

bool AS47Web::AuditSaroArrival(TArray<FString>& Lines) const
{
 return SaroArrival && SaroArrival->Audit(Lines);
}

void AS47Web::BeginSaroArrivalViews()
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47SaroViews")))return;
 struct FState { bool Done=false,Placed=false;int32 Index=0;double Since=0;FString Shots; };
 const auto P=MakeShared<FState>();FParse::Value(FCommandLine::Get(),TEXT("S47SaroShots="),P->Shots);
 const TWeakObjectPtr<AS47Web> Self=this;FTimerHandle Timer;
 GetWorldTimerManager().SetTimer(Timer,[Self,P] {
  if(P->Done)return;
  AS47Web* W=Self.Get();US47Main* M=W?US47Main::Get(W):nullptr;
  if(!M||!M->Flow||!M->Story||!M->World||!M->Player||S47Startup::Blocking()||M->Flow->bStarting||M->World->bBusy||M->Mode!=US47Main::EMode::Play)return;
  auto Fail=[P](const TCHAR* Why){P->Done=true;UE_LOG(LogS47ArrivalViews,Error,TEXT("S47: SARO arrival views FAIL: %s"),Why);};
  FString UserDir;
  if(!M->Flow->bDevRun||M->Flow->CaseId!=0||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||
   !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),FPaths::ConvertRelativePathToFull(UserDir))||P->Shots.IsEmpty())
  {Fail(TEXT("requires isolated unsaved developer profile and S47SaroShots"));return;}
  struct FView {const TCHAR* Name;FVector Foot,Target;};
  static const FView Views[]={
   {TEXT("arrival"),FVector(-22.8,0,27),FVector(-11,1,14.8)},
   {TEXT("sign"),FVector(-11.3,0,20.8),FVector(-7.3,.9,15.5)},
   {TEXT("visitor-sign"),FVector(-15.8,0,13.8),FVector(-15.8,.85,10.85)},
   {TEXT("entrance"),FVector(-14,0,8.2),FVector(-6.7,1.25,5.7)},
   {TEXT("entrance-close"),FVector(-9.9,0,5.7),FVector(-6.25,1.35,5.7)},
   {TEXT("windows"),FVector(-4,0,-8),FVector(0,1.8,-3.45)},
   {TEXT("room"),FVector(2.3,0,1.75),FVector(.2,1.5,-3.45)}};
  static const TCHAR* Lights[]={TEXT("day"),TEXT("dusk"),TEXT("night")};
  static const double Clocks[]={-8.5*3600,-4.5*3600,-2*3600};
  const int32 View=P->Index%UE_ARRAY_COUNT(Views),Light=P->Index/UE_ARRAY_COUNT(Views);
  const double Now=FPlatformTime::Seconds();
  if(!P->Placed)
  {
   if(M->World->Area!=TEXT("saro")||M->World->bDriving){Fail(TEXT("start with S47Evening=parking"));return;}
   const auto& V=Views[View];
   if(!M->Player->Walkable(V.Foot.X,V.Foot.Z)){Fail(TEXT("view fixture is outside walkable SARO"));return;}
   M->Story->SetClock(Clocks[Light]);
   M->Player->Place(V.Foot.X,V.Foot.Z,0);
   const FVector D=V.Target-FVector(V.Foot.X,M->Player->FloorY+M->Player->Eye,V.Foot.Z);
   M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FMath::Sqrt(D.X*D.X+D.Z*D.Z));
   P->Placed=true;P->Since=Now;return;
  }
  if(Now-P->Since<2)return;
  TArray<FString> Lines;const bool Ok=W->AuditSaroArrival(Lines);
  if(P->Index==0)for(const FString& Line:Lines)UE_LOG(LogS47ArrivalViews,Display,TEXT("S47: arrival %s"),*Line);
  if(!Ok){Fail(TEXT("native arrival geometry/collision audit"));return;}
  IFileManager::Get().MakeDirectory(*P->Shots,true);
  const FString Name=FString(Lights[Light])+TEXT("-")+Views[View].Name;
  const FString File=P->Shots/(Name+TEXT(".png"));
  if(IFileManager::Get().FileExists(*File)){Fail(TEXT("refusing to overwrite view evidence"));return;}
  // Camera setup/parking toasts obscure the lot in short developer fixtures; retain the HUD.
  if(M->UI)M->UI->ClearToasts();
  FScreenshotRequest::RequestScreenshot(File,true,false);
  UE_LOG(LogS47ArrivalViews,Display,TEXT("S47: SARO arrival view %s floor=%.4f eye=%.4f clock=%.3f; camera fixture"),*Name,M->Player->FloorY,M->Player->FloorY+M->Player->Eye,M->Story->Clock());
  ++P->Index;P->Placed=false;
  if(P->Index==UE_ARRAY_COUNT(Views)*UE_ARRAY_COUNT(Lights)){P->Done=true;UE_LOG(LogS47ArrivalViews,Display,TEXT("S47: SARO arrival views PASS %d player-height fixtures; driving and input checked separately"),P->Index);}
 },.1f,true);
}
