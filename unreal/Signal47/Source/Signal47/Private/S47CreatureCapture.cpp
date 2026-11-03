#include "S47CreatureCapture.h"
#include "S47Json.h"
#include "Components/PoseableMeshComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

struct FS47CreatureCapture::FState
{
 TFunction<FSample()> Sample;
 FString Dir,Pending;
 double End=0,Next=0,WallStart=0,RequestWall=0,LastWritten=-1;
 bool bDone=false,bEndSeen=false,bOwnDirectory=false;
 S47J::FObj Row;
 TArray<TSharedPtr<FJsonValue>> Frames;
};

TSharedPtr<FS47CreatureCapture> FS47CreatureCapture::Create(TFunction<FSample()> Sample)
{
 FString Dir;
 if(!FParse::Value(FCommandLine::Get(),TEXT("S47CreatureCapture="),Dir))return {};
 auto C=MakeShared<FS47CreatureCapture>();C->State=MakeShared<FState>();auto& S=*C->State;
 S.Sample=MoveTemp(Sample);S.WallStart=FPlatformTime::Seconds();
 FString UserDir;int32 Case=-1;
 FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);
 FParse::Value(FCommandLine::Get(),TEXT("S47Case="),Case);
 FPaths::NormalizeDirectoryName(UserDir);FPaths::CollapseRelativeDirectories(UserDir);
 FPaths::NormalizeDirectoryName(Dir);FPaths::CollapseRelativeDirectories(Dir);
 const FString Saved=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
 if(Case!=0||FPaths::IsRelative(UserDir)||!UserDir.StartsWith(TEXT("/tmp/"))||
  !FPaths::IsUnderDirectory(Saved,UserDir)||FPaths::IsRelative(Dir)||!Dir.StartsWith(TEXT("/tmp/"))||
  !FParse::Value(FCommandLine::Get(),TEXT("S47CreatureCaptureSeconds="),S.End)||!FMath::IsFinite(S.End)||S.End<1||S.End>30)
 {C->Finish(false,TEXT("requires isolated Case0, /tmp UserDir, fresh absolute /tmp output and explicit capture duration 1..30 seconds"));return C;}
 auto& Files=IFileManager::Get();
 if(Files.DirectoryExists(*Dir)||Files.FileExists(*Dir)||!Files.MakeDirectory(*Dir,true))
 {C->Finish(false,TEXT("output must be a new directory; no overwrite"));return C;}
 S.Dir=Dir;S.bOwnDirectory=true;
 if(UGameViewportClient::OnScreenshotCaptured().IsBound())
 {C->Finish(false,TEXT("foreign screenshot delegate already owns bitmap output"));return C;}
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([C](float){return C->Poll();}));
 UE_LOG(LogTemp,Display,TEXT("S47: creature capture ARMED %s, preview end %.3f; actual Unreal framebuffer, no desktop capture"),*Dir,S.End);
 return C;
}

void FS47CreatureCapture::Finish(bool Complete,const TCHAR* Reason)
{
 auto& S=*State;if(S.bDone)return;S.bDone=true;
 if(S.bOwnDirectory)
 {
  auto J=S47J::New();S47J::Set(J,TEXT("complete"),Complete);S47J::Set(J,TEXT("reason"),Reason);
  S47J::Set(J,TEXT("capture"),TEXT("Unreal game render target; bShowUI=false; no desktop or lock-screen pixels"));
  S47J::Set(J,TEXT("requestedPreviewEnd"),S.End);S47J::Set(J,TEXT("interval"),.12);
  S47J::Set(J,TEXT("timing"),TEXT("previewSeconds and wallSeconds are request timestamps after the actual fixture pose. Screenshot readback occurs at frame end. Use adjacent previewSeconds for animation playback; no fixed-FPS or performance claim. Skipped slots are not duplicated."));
  S47J::Set(J,TEXT("limits"),TEXT("Explicit model/camera fixture; no natural gameplay, AI contact, human input, audio or performance proof. Capture cost can slow wall time. Desktop remains locked."));
  J->SetArrayField(TEXT("frames"),S.Frames);
  if(!FFileHelper::SaveStringToFile(S47J::Write(J,true),*(S.Dir/TEXT("frames.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,&IFileManager::Get(),FILEWRITE_NoReplaceExisting))Complete=false;
 }
 UE_LOG(LogTemp,Display,TEXT("S47: creature capture %s: %s; %d frames"),Complete?TEXT("COMPLETE"):TEXT("FAIL"),Reason,S.Frames.Num());
 FPlatformMisc::RequestExitWithStatus(false,Complete?0:1);
}

bool FS47CreatureCapture::Poll()
{
 auto& S=*State;if(S.bDone)return false;
 const double Now=FPlatformTime::Seconds();
 if(!S.Pending.IsEmpty())
 {
  if(IFileManager::Get().FileSize(*S.Pending)>0)
  {
   S47J::Set(S.Row,TEXT("fileObservedWallSeconds"),Now-S.WallStart);
   S.LastWritten=S47J::Num(S.Row,TEXT("previewSeconds"));S.Frames.Add(MakeShared<FJsonValueObject>(S.Row));
   S.Pending.Reset();S.Row.Reset();
  }
  else if(Now-S.RequestWall>20){Finish(false,TEXT("viewport request did not write its PNG"));return false;}
 }
 if(S.bEndSeen&&S.Pending.IsEmpty())
 {
  if(S.LastWritten+1.e-6>=S.End){Finish(true,TEXT("requested preview span recorded"));return false;}
  OnFrame(); // the completed preview retains its final pose; drain one final framebuffer frame
 }
 if(Now-S.WallStart>180){Finish(false,TEXT("preview/capture did not finish within 180 wall seconds"));return false;}
 return !S.bDone;
}

void FS47CreatureCapture::OnFrame()
{
 auto& S=*State;if(S.bDone)return;
 const auto A=S.Sample();if(!A.bStarted||!FMath::IsFinite(A.PreviewTime))return;
 S.bEndSeen|=A.PreviewTime+1.e-6>=S.End;
 if(!S.Pending.IsEmpty()||FScreenshotRequest::IsScreenshotRequested()||(!S.bEndSeen&&A.PreviewTime<S.Next))return;
 if(UGameViewportClient::OnScreenshotCaptured().IsBound()){Finish(false,TEXT("foreign screenshot delegate appeared"));return;}
 if(S.Frames.Num()>=300){Finish(false,TEXT("bounded frame budget exceeded"));return;}
 auto* Skin=A.Skin.Get();if(!Skin||!GEngine||!GEngine->GameViewport){Finish(false,TEXT("actual skin or viewport missing"));return;}
 const FString Name=FString::Printf(TEXT("frame-%04d-t%08.3f.png"),S.Frames.Num(),A.PreviewTime);
 S.Pending=S.Dir/Name;if(IFileManager::Get().FileExists(*S.Pending)){Finish(false,TEXT("refusing existing frame"));return;}
 S.Row=S47J::New();S47J::Set(S.Row,TEXT("file"),Name);S47J::Set(S.Row,TEXT("previewSeconds"),A.PreviewTime);
 S47J::Set(S.Row,TEXT("stage"),A.Stage);S47J::Set(S.Row,TEXT("renderedAtRequest"),A.bRendered);
 S.RequestWall=FPlatformTime::Seconds();S47J::Set(S.Row,TEXT("wallSeconds"),S.RequestWall-S.WallStart);
 FVector2D Size;GEngine->GameViewport->GetViewportSize(Size);S47J::Set(S.Row,TEXT("viewportSize"),TArray<double>{Size.X,Size.Y});
 for(const TCHAR* Bone:{TEXT("pelvis"),TEXT("foot_l"),TEXT("foot_r"),TEXT("hand_l"),TEXT("hand_r")})
 {const FVector P=Skin->GetBoneLocationByName(FName(Bone),EBoneSpaces::WorldSpace);S47J::Set(S.Row,Bone,TArray<double>{P.X,P.Y,P.Z});}
 S.Next=(FMath::FloorToDouble(A.PreviewTime/.12)+1)*.12;
 // bShowUI=false selects GetViewportScreenShot, not Slate/OS window compositing.
 FScreenshotRequest::RequestScreenshot(S.Pending,false,false,false,FIntRect(),true);
}
