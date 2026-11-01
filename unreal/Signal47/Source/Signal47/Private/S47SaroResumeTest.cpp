#include "S47SaroResumeTest.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47Story.h"
#include "S47Spine.h"
#include "S47SC04.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Inter.h"
#include "S47Walker.h"
#include "S47UI.h"
#include "S47Three.h"
#include "S47Obj.h"
#include "S47Json.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "InputCoreTypes.h"
#include "DynamicRHI.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HighResScreenshot.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SaroResumeTest,Log,All);
namespace
{
 double Cvar(const TCHAR* Name)
 {const auto* V=IConsoleManager::Get().FindConsoleVariable(Name);return V?V->GetFloat():-1;}
 FS47Obj OpsObject(US47Main* M)
 {
  if(!M||!M->Inter||!M->Web)return {};
  if(const auto* I=M->Inter->Get(TEXT("opsTerminal")))return I->Object;
  if(const auto* I=M->Web->Item(TEXT("opsTerminal")))return M->Web->ObjAt(I->Node);
  return {};
 }
 FVector OpsTarget(US47Main* M)
 {const auto O=OpsObject(M);if(!O)return {};const auto B=O.WorldBox();return B.IsValid?B.GetCenter():O.WorldPosition();}
}
struct FS47SaroResumeTest::FState
{
 bool bEnabled=false,bDone=false,bValidated=false;
 int32 Step=0,Checks=0,Pauses=0,Resumes=0,TerminalUses=0;
 double Created=FPlatformTime::Seconds(),ReadyAt=0,Since=0,WakeAt=0,TerminalAt=0,NextHeartbeat=0;
 uint64 FirstFrame=0,TerminalFrame=0,FinalShotFrame=0;
 FString Root,Profile;
 TArray<TSharedPtr<FJsonValue>> Evidence;
 S47J::FObj StartRenderer;
};
FS47SaroResumeTest::FS47SaroResumeTest(US47Main* Main):State(MakeUnique<FState>()),M(Main)
 {State->bEnabled=FParse::Param(FCommandLine::Get(),TEXT("S47SaroResumeTest"));}
FS47SaroResumeTest::~FS47SaroResumeTest()=default;
void FS47SaroResumeTest::Write(const FString& Status,const FString& Why)
{
 auto& S=*State;if(S.Root.IsEmpty())return;
 auto R=S47J::New();S47J::Set(R,TEXT("status"),Status);S47J::Set(R,TEXT("reason"),Why);S47J::Set(R,TEXT("stage"),S.Step);
 S47J::Set(R,TEXT("checks"),S.Checks);S47J::Set(R,TEXT("pauseOpens"),S.Pauses);S47J::Set(R,TEXT("pauseResumes"),S.Resumes);S47J::Set(R,TEXT("terminalUses"),S.TerminalUses);
 S47J::Set(R,TEXT("playWallSeconds"),S.ReadyAt>0?FPlatformTime::Seconds()-S.ReadyAt:0);
 S47J::Set(R,TEXT("afterTerminalWallSeconds"),S.TerminalAt>0?FPlatformTime::Seconds()-S.TerminalAt:0);
 S47J::Set(R,TEXT("nativeFramesAfterTerminal"),S.TerminalFrame>0?double(GFrameCounter-S.TerminalFrame):0);
 S47J::Set(R,TEXT("RHI"),GDynamicRHI?FString(GDynamicRHI->GetName()):FString(TEXT("none")));
 S47J::Set(R,TEXT("rayTracing"),Cvar(TEXT("r.RayTracing")));S47J::Set(R,TEXT("lumenHardwareRayTracing"),Cvar(TEXT("r.Lumen.HardwareRayTracing")));
 S47J::Set(R,TEXT("residentGeometryPoolMiB"),Cvar(TEXT("r.RayTracing.ResidentGeometryMemoryPoolSizeInMB")));
 S47J::Set(R,TEXT("rendererAtStart"),S.StartRenderer);
 if(M&&M->Web)S47J::Set(R,TEXT("rendererAtCheckpoint"),M->Web->GraphicsReadback());
 S47J::Set(R,TEXT("source"),TEXT("Ordinary Continue in isolated case1 from the unchanged original 02:57 auto1; source pose and unheld schedule verified from Flow carry."));
 S47J::Set(R,TEXT("fixture"),TEXT("Player placement only: original road pose is moved to the existing control-room approach (-4.2,2.0). No Story flag reset, replacement state, new case, physical road drive or human input is claimed."));
 S47J::Set(R,TEXT("method"),TEXT("Four production Flow pause opens and focused Slate Escape resumes; actual Walker camera rays and OnUse production callback at OPS; persisted AUTO HOLD verified; 120 seconds after first terminal use and a real final rendered PNG required."));
 S47J::Set(R,TEXT("limits"),TEXT("Bounded High/HWRT/Vulkan presentation/terminal diagnostic. Not the original 21-minute session, explicit swapchain resize/focus reproduction, whole-story acceptance, Freeform-night acceptance, or a proven Vulkan fix. A PASS also requires normal process exit verified by the runner."));
 R->SetArrayField(TEXT("evidence"),S.Evidence);
 if(!FFileHelper::SaveStringToFile(S47J::Write(R,true),*(S.Root/TEXT("result.json"))))
  UE_LOG(LogS47SaroResumeTest,Error,TEXT("S47: SARO resume diagnostic could not write its native result"));
}
bool FS47SaroResumeTest::Check(bool Good,const FString& Why)
{
 auto& S=*State;++S.Checks;
 if(!Good){Finish(false,Why);return false;}
 auto Row=S47J::New();S47J::Set(Row,TEXT("check"),Why);S47J::Set(Row,TEXT("stage"),S.Step);S47J::Set(Row,TEXT("frame"),double(GFrameCounter));S47J::Set(Row,TEXT("playWallSeconds"),S.ReadyAt>0?FPlatformTime::Seconds()-S.ReadyAt:0);
 S.Evidence.Add(MakeShared<FJsonValueObject>(Row));UE_LOG(LogS47SaroResumeTest,Display,TEXT("S47: SARO resume PASS: %s"),*Why);Write(TEXT("RUNNING"),Why);return true;
}
void FS47SaroResumeTest::Finish(bool Good,const FString& Why)
{
 auto& S=*State;if(S.bDone)return;S.bDone=true;
 if(M&&M->Player){M->Player->TestMove.Reset();M->Player->bTestRun=false;}
 Write(Good?TEXT("PASS"):TEXT("FAIL"),Why);
 UE_LOG(LogS47SaroResumeTest,Display,TEXT("S47: SARO resume %s COMPLETE: %d checks, %d pauses/%d resumes, %d production terminal uses; %s"),Good?TEXT("PASS"):TEXT("FAIL"),S.Checks,S.Pauses,S.Resumes,S.TerminalUses,*Why);
 FPlatformMisc::RequestExitWithStatus(false,Good?0:1);
}
void FS47SaroResumeTest::Stage(int32 Next,double Wait)
 {auto& S=*State;S.Step=Next;S.Since=FPlatformTime::Seconds();S.WakeAt=S.Since+Wait;}
void FS47SaroResumeTest::Shot(const TCHAR* Name)
 {FScreenshotRequest::RequestScreenshot(State->Root/(FString(Name)+TEXT(".png")),true,false);}
void FS47SaroResumeTest::AimOps()
{
 const FVector Eye(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z),D=OpsTarget(M)-Eye;
 M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
}
bool FS47SaroResumeTest::UseOps()
{
 if(!Check(bool(OpsObject(M)),TEXT("OPS has a physical exported/native interaction object")))return false;
 const FVector Eye=M->Player->EyeUE(),Direction=(S47Three::ToUE(OpsTarget(M))-Eye).GetSafeNormal();
 const FString Picked=M->Inter->Pick(Eye,Direction);
 if(!Check(Picked==TEXT("opsTerminal")&&FVector::DotProduct(Direction,M->Player->ForwardUE())>.94&&bool(M->Player->OnUse),TEXT("actual aimed camera ray selects OPS in ordinary interaction range")))return false;
 M->Player->OnUse(Picked);++State->TerminalUses;
 return Check(M->ModalOpen(),TEXT("production OPS use opens its real document or terminal panel"));
}
void FS47SaroResumeTest::Step(double)
{
 auto& S=*State;if(!S.bEnabled||S.bDone)return;const double Now=FPlatformTime::Seconds();
 if(!S.bValidated)
 {
  FString User;
  if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)){Finish(false,TEXT("Explicit isolated /tmp UserDir is required"));return;}
  User=FPaths::ConvertRelativePathToFull(User);FPaths::NormalizeDirectoryName(User);FPaths::CollapseRelativeDirectories(User);
  const FString Saved=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
  if(!(User.StartsWith(TEXT("/tmp/"))||User.StartsWith(TEXT("/var/tmp/")))||!Saved.StartsWith(User+TEXT("/")))
  {Finish(false,TEXT("Diagnostic refuses a non-isolated profile"));return;}
  S.Profile=User;S.Root=FPaths::ProjectSavedDir()/TEXT("SaroResumeTest");IFileManager::Get().MakeDirectory(*S.Root,true);S.bValidated=true;
  if(!Check(!FParse::Param(FCommandLine::Get(),TEXT("RenderOffscreen"))&&!FParse::Param(FCommandLine::Get(),TEXT("nullrhi")),TEXT("real window presentation required; offscreen/null RHI are rejected")))return;
 }
 if(S.ReadyAt==0)
 {
  if(Now-S.Created>120){Finish(false,TEXT("ordinary Continue did not reach the isolated original case within startup bound"));return;}
  if(!M||!M->Flow||!M->Story||!M->Player||!M->Web||!M->Inter||!M->UI||!M->World||!M->Spine||M->Flow->bStarting||M->World->bBusy||M->Mode!=US47Main::EMode::Play||M->Flow->CaseId==0)return;
  S.ReadyAt=Now;S.FirstFrame=GFrameCounter;
  if(!Check(M->Flow->CaseId==1&&!M->Flow->bDevRun&&!M->IsFreeform(),TEXT("ordinary case1 Story Continue is active; not a replacement/dev/Freeform state")))return;
  const auto Carry=M->Flow->Carry,SC04=S47J::Obj(S47J::Obj(Carry,TEXT("spine")),TEXT("sc04d")),Pose=S47J::Obj(Carry,TEXT("pose"));
  if(!Check(Carry&&S47J::Str(Carry,TEXT("checkpoint"))==TEXT("nightService")&&S47J::Str(Carry,TEXT("area"))==TEXT("saro")&&!S47J::Bool(SC04,TEXT("schedule"))&&S47J::Num(Pose,TEXT("z"))>600&&S47J::Num(Pose,TEXT("z"))<800,TEXT("loaded source retains original auto1 road pose and schedule=false without a flag reset")))return;
  const auto Window=GEngine&&GEngine->GameViewport?GEngine->GameViewport->GetWindow():TSharedPtr<SWindow>();
  if(!Check(Window.IsValid()&&Window->IsVisible()&&GDynamicRHI&&FString(GDynamicRHI->GetName()).Contains(TEXT("Vulkan")),TEXT("native visible game window is using Vulkan")))return;
  if(!Check(M->Flow->Quality.ToLower()==TEXT("high")&&Cvar(TEXT("r.RayTracing"))==1&&Cvar(TEXT("r.Lumen.HardwareRayTracing"))==1,TEXT("existing High and hardware ray tracing are active without graphics changes")))return;
  if(!Check(!M->World->bDriving,TEXT("Continue exposes an on-foot player; the diagnostic never forcibly exits a cab")))return;
  S.StartRenderer=M->Web->GraphicsReadback();Shot(TEXT("continued-original-road"));
  M->Player->Place(-4.2,2.,PI/2);M->Player->Pitch=0;M->Player->TestMove.Reset();M->Player->bTestRun=false;
  UE_LOG(LogS47SaroResumeTest,Display,TEXT("S47: SARO resume explicit PLAYER-POSE FIXTURE from original road to control-room approach; no Story flags or records changed"));
  S.NextHeartbeat=Now+10;Stage(10,1);return;
 }
 if(Now-S.ReadyAt>150){Finish(false,TEXT("bounded 150-second post-Continue window expired before all native checks and rendered proof"));return;}
 if(Now>=S.NextHeartbeat)
 {
  S.NextHeartbeat=Now+10;
  UE_LOG(LogS47SaroResumeTest,Display,TEXT("S47: SARO resume heartbeat %.1fs afterterminal %.1fs frame%llu stage%d pauses%d resumes%d uses%d"),Now-S.ReadyAt,S.TerminalAt?Now-S.TerminalAt:0,(unsigned long long)GFrameCounter,S.Step,S.Pauses,S.Resumes,S.TerminalUses);
 }
 if(Now<S.WakeAt)return;
 if(S.Step==10)
 {
  if(S.Pauses>=4){Stage(20,.3);return;}
  if(!Check(!M->bPausedByMenu&&!M->ModalOpen(),TEXT("ordinary world is resumed before the next pause cycle")))return;
  M->Flow->OpenPause();++S.Pauses;
  if(!Check(M->bPausedByMenu&&M->ModalOpen(),TEXT("production Flow pause opens the real paused menu")))return;
  Stage(11,1.);return;
 }
 if(S.Step==11)
 {
  if(!Check(FSlateApplication::IsInitialized(),TEXT("Slate is available for ordinary pause-menu Escape")))return;
  const bool Handled=FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Escape,FModifierKeysState(),0,false,0,0));
  FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(EKeys::Escape,FModifierKeysState(),0,false,0,0));
  if(!Check(Handled&&!M->bPausedByMenu&&!M->ModalOpen(),TEXT("focused Slate Escape executes the production pause Resume callback")))return;
  ++S.Resumes;Stage(10,1.2);return;
 }
 if(S.Step==20)
 {
  if(!Check(M->Player->Walkable(M->Player->Pos.X,M->Player->Pos.Z),TEXT("control-room fixture rests on an ordinary walkable location")))return;
  AimOps();Stage(21,.4);return;
 }
 if(S.Step==21)
 {
  if(!UseOps())return;
  if(S.TerminalUses==1)
  {
   S.TerminalAt=Now;S.TerminalFrame=GFrameCounter;
   const auto Current=M->Flow->GameState();
   if(!Check(S47J::Bool(S47J::Obj(S47J::Obj(Current,TEXT("spine")),TEXT("sc04d")),TEXT("schedule"))&&M->Story->Docs().ContainsByPredicate([](const FS47Doc& D){return D.Id==TEXT("sc04_schedule");}),TEXT("first actual OPS use records AUTO HOLD through production state and its original document")))return;
   Shot(TEXT("actual-auto-hold"));
  }
  Stage(22,1.4);return;
 }
 if(S.Step==22)
 {
  M->UI->Close(true);
  if(!Check(!M->ModalOpen()&&!M->bPausedByMenu,TEXT("ordinary terminal/document close returns to the native control-room view")))return;
  Stage(S.TerminalUses<3?20:30,1.8);return;
 }
 if(S.Step==30)
 {
  if(Now-S.TerminalAt<120)return;
  if(!Check(S.Pauses==4&&S.Resumes==4&&S.TerminalUses==3,TEXT("four genuine pause/resume cycles and three production OPS uses completed")))return;
  if(!Check(M->Mode==US47Main::EMode::Play&&M->Flow->CaseId==1&&!M->IsFreeform()&&!M->ModalOpen()&&!M->bPausedByMenu,TEXT("same ordinary case remains interactive after the bounded observation")))return;
  if(!Check(Cvar(TEXT("r.RayTracing"))==1&&Cvar(TEXT("r.Lumen.HardwareRayTracing"))==1&&M->Flow->Quality.ToLower()==TEXT("high"),TEXT("High/HWRT settings were not reduced during the observation")))return;
  Shot(TEXT("after-terminal-120s"));S.FinalShotFrame=GFrameCounter;Stage(31,.6);return;
 }
 if(S.Step==31)
 {
  const int64 Bytes=IFileManager::Get().FileSize(*(S.Root/TEXT("after-terminal-120s.png")));
  if(Bytes<1024||GFrameCounter<S.FinalShotFrame+3)return;
  if(!Check(true,TEXT("actual final PNG rendered after 120 seconds and subsequent native frames advanced")))return;
  Finish(true,TEXT("Bounded original-case terminal/pause observation completed; Vulkan root cause remains unconfirmed."));return;
 }
}
