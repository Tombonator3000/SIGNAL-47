#include "S47FinalePreview.h"
#include "S47Chapter6.h"
#include "S47FinaleSky.h"
#include "S47Flow.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47World.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HighResScreenshot.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void FS47FinalePreview::Step(US47Main* M,double Dt)
{
 FString List;
 if(!FParse::Value(FCommandLine::Get(),TEXT("S47FinalePreview="),List,false)||!M)return;
 struct FFrame { double Phase=0;FString Name;S47J::FObj Tune; };
 struct FState { bool Init=false,Done=false,Pull=false;double Start=FPlatformTime::Seconds(),ReadyAt=0,RequestedAt=0;int32 Index=0,Applied=-1;TArray<FFrame> Frames;FString Dir,Pending; };
 static TMap<TWeakObjectPtr<US47Main>,FState> States;
 for(auto It=States.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
 auto& S=States.FindOrAdd(M);if(S.Done)return;
 auto Fail=[&](const TCHAR* Why){UE_LOG(LogTemp,Error,TEXT("S47 finale preview FAIL: %s"),Why);S.Done=true;FPlatformMisc::RequestExitWithStatus(false,1);};
 if(FPlatformTime::Seconds()-S.Start>180){Fail(TEXT("frame preview timeout"));return;}
 auto* W=M->World.Get();auto* H=M->Story.Get();
 auto* C=H?Cast<US47Chapter6>(H->Chapter(TEXT("ch6"))):nullptr;
 if(!W||!H||!M->Player||!C||!C->bActive||W->Area!=TEXT("roswell")||!W->OldDrive()||!W->OldRoad())return;
 if(!S.Init)
 {
  FString UserDir;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);
  if(!UserDir.StartsWith(TEXT("/tmp/"))&&!UserDir.StartsWith(TEXT("/var/tmp/"))){Fail(TEXT("explicit isolated /tmp UserDir required"));return;}
  if(!FParse::Value(FCommandLine::Get(),TEXT("S47FinaleShots="),S.Dir)||FPaths::IsRelative(S.Dir)){Fail(TEXT("absolute shot directory required"));return;}
  FString SweepPath;
  if(FParse::Value(FCommandLine::Get(),TEXT("S47FinaleSweep="),SweepPath))
  {
   FString Data;if(!FFileHelper::LoadFileToString(Data,*SweepPath)){Fail(TEXT("cannot read visual sweep"));return;}
   auto Spec=S47J::Read(Data);
   for(const auto& V:S47J::Arr(Spec,TEXT("frames")))
   {
    auto O=V->AsObject();FFrame F;F.Phase=FMath::Clamp(S47J::Num(O,TEXT("phase")),0.,80.);
    F.Name=S47J::Str(O,TEXT("name"));F.Tune=S47J::Obj(O,TEXT("tune"));
    if(F.Name.IsEmpty()||F.Name.Len()>80){Fail(TEXT("invalid frame name"));return;}
    for(TCHAR Ch:F.Name)if(!FChar::IsAlnum(Ch)&&Ch!='-'&&Ch!='_'){Fail(TEXT("unsafe frame name"));return;}
    S.Frames.Add(F);
   }
  }
  else
  {
   TArray<FString> Values;List.ParseIntoArray(Values,TEXT(","),true);
   for(const auto& V:Values){FFrame F;F.Phase=FMath::Clamp(FCString::Atod(*V),0.,80.);S.Frames.Add(F);}
  }
  if(S.Frames.IsEmpty()){Fail(TEXT("no phases"));return;}
  S.Frames.StableSort([](const FFrame& A,const FFrame& B){return A.Phase<B.Phase;});
  IFileManager::Get().MakeDirectory(*S.Dir,true);
  const auto Pose=W->OldRoad()->PoseAt(8.15);W->OldDrive()->Place(Pose.Pos,Pose.Heading);W->OldDrive()->Speed=0;W->Autopilot=0;
  C->StartEvent();S.Init=true;S.ReadyAt=FPlatformTime::Seconds()+5;
  UE_LOG(LogTemp,Display,TEXT("S47 finale preview: fixed C8.15 cab/ending frames; not ordinary travel; %d frames"),S.Frames.Num());
 }
 const auto& Frame=S.Frames[S.Index];const double Phase=Frame.Phase;
 if(S.Applied!=S.Index)
 {
  if(Frame.Tune)for(const auto& Pair:Frame.Tune->Values)
  {
   // Artist allowlist: only registered r.S47.Finale.* CVars, including StarScale/StarDim.
   const FString Key=FString(TEXT("r.S47.Finale."))+FString(*Pair.Key);
   auto* Var=IConsoleManager::Get().FindConsoleVariable(*Key);double Value=0;
   if(!Var||!Pair.Value->TryGetNumber(Value)){Fail(TEXT("unknown finale artist parameter"));return;}
   Var->Set(float(Value),ECVF_SetByConsole);
  }
  S.Applied=S.Index;
 }
 if(Phase<47){C->Stage=TEXT("event");C->Et=Phase;}
 else
 {
  if(!S.Pull){C->Et=47;C->StartPullout();S.Pull=true;}
  C->Stage=TEXT("pullout");C->Et=47;C->Pt=Phase-47;
 }
 W->OldDrive()->Speed=0;W->Autopilot=0;
 if(C->FinaleSky)C->FinaleSky->Step(8.15,C->Stage,C->Et,C->Pt);
 if(M->UI)M->UI->ClearToasts();
 if(!S.Pending.IsEmpty())
 {
  if(IFileManager::Get().FileSize(*S.Pending)>32768&&!FScreenshotRequest::IsScreenshotRequested())
  {
   auto R=C->FinaleSkyReadback();S47J::Set(R,TEXT("previewPhase"),Phase);S47J::Set(R,TEXT("previewName"),Frame.Name);S47J::Set(R,TEXT("developerFrameFixture"),true);
   FFileHelper::SaveStringToFile(S47J::Write(R,true),*FPaths::ChangeExtension(S.Pending,TEXT("json")));
   UE_LOG(LogTemp,Display,TEXT("S47 finale frame written %s"),*S.Pending);S.Pending.Reset();++S.Index;
   if(S.Index==S.Frames.Num()){S.Done=true;UE_LOG(LogTemp,Display,TEXT("S47 finale preview COMPLETE"));FPlatformMisc::RequestExit(false);return;}
   S.ReadyAt=FPlatformTime::Seconds()+2.5;
  }
  return;
 }
 if(FPlatformTime::Seconds()<S.ReadyAt)return;
 if(C->FinaleSky&&!S47J::Bool(C->FinaleSkyReadback(),TEXT("ready")))return;
 S.Pending=S.Dir/FString::Printf(TEXT("phase-%05.1f%s.png"),Phase,Frame.Name.IsEmpty()?TEXT(""):*(TEXT("-")+Frame.Name));
 if(IFileManager::Get().FileExists(*S.Pending)){Fail(TEXT("refusing previous frame overwrite"));return;}
 const bool ShowUI=!FParse::Param(FCommandLine::Get(),TEXT("S47FinaleClean"));
 FScreenshotRequest::RequestScreenshot(S.Pending,ShowUI,false);S.RequestedAt=FPlatformTime::Seconds();
}
