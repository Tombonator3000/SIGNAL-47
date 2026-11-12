#include "S47CaptureSoundTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47FreeformAudioDesk.h"
#include "S47FreeformDirector.h"
#include "S47Flow.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47Inter.h"
#include "S47Walker.h"
#include "S47Audio.h"
#include "S47Three.h"
#include "S47Startup.h"
#include "Containers/Ticker.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
namespace
{
 struct FRun
 {
  TWeakObjectPtr<US47Main> Main;int Stage=0,Checks=0,LoadingSeen=-1,LoadingChecked=-1;bool Done=false,ChangedDials=false;double Began=FPlatformTime::Seconds(),At=0;
  FString Root,Original,Control,Document,RestorePath;double PausedCursor=0;TArray<double> Ticks;TArray<TSharedPtr<FJsonValue>> Evidence;
  bool Finish(bool Good,const FString& Why)
  {
   Done=true;auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("checks"),Checks);
   S47J::Set(O,TEXT("method"),TEXT("Explicit first-pulsar selection, extra-pulse cause, dial and approach fixtures. Actual acquisition, printer eye ray, own reference, native audio-context source completion, transport seek/pause/listening, JSON Session round-trip. No human listening, ordinary menu Continue or natural anomaly scheduling claim."));if(!RestorePath.IsEmpty()){S47J::Set(O,TEXT("method"),TEXT("Load actual prior native Session JSON under explicit NoSound. Production reel transport seek/pause and full 18/8s playback, unchanged retained recipes and evidence. Explicit player placement; no new capture, source-render, audible-listening or ordinary menu Continue claim."));S47J::Set(O,TEXT("sourceSession"),RestorePath);}O->SetArrayField(TEXT("evidence"),Evidence);
   if(!Root.IsEmpty()){IFileManager::Get().MakeDirectory(*Root,true);FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("result.json")));}
   UE_LOG(LogTemp,Display,TEXT("S47 capture v3 %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),*Why);FPlatformMisc::RequestExitWithStatus(false,Good?0:1);return false;
  }
  bool Check(bool Pass,const FString& Why){++Checks;auto O=S47J::New();S47J::Set(O,TEXT("pass"),Pass);S47J::Set(O,TEXT("check"),Why);Evidence.Add(MakeShared<FJsonValueObject>(O));if(!Pass)Finish(false,Why);return Pass;}
  void Go(int Next){Stage=Next;At=FPlatformTime::Seconds();}
  bool TickRestore(US47Main* M,double Now)
  {
   auto& F=*M->Freeform;const auto* J=F.CurrentJob();
   if(Stage==0)
   {
    if(Now-Began<4)return true;Root=FPaths::ProjectSavedDir()/TEXT("CaptureSoundRestoreTest");
    FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
    FString Canonical=FPaths::ConvertRelativePathToFull(RestorePath);FPaths::NormalizeFilename(Canonical);FPaths::CollapseRelativeDirectories(Canonical);
    if(!Check(!FPaths::IsRelative(RestorePath)&&Canonical.StartsWith(TEXT("/tmp/"))&&User.StartsWith(TEXT("/tmp/"))&&M->Flow->bDevRun&&M->Flow->CaseId==0&&FParse::Param(FCommandLine::Get(),TEXT("NoSound")),TEXT("isolated explicit NoSound restore from absolute temporary session")))return false;
    FString Raw;if(!Check(FFileHelper::LoadFileToString(Raw,*Canonical),TEXT("read actual prior native session file")))return false;
    auto Saved=S47J::Read(Raw);if(!Check(Saved.IsValid(),TEXT("prior session JSON parses")))return false;
    const auto Jobs=S47J::Arr(Saved,TEXT("jobs"));const int Selected=S47J::Int(Saved,TEXT("selected"),-1);
    if(!Check(Jobs.IsValidIndex(Selected)&&Jobs[Selected]->Type==EJson::Object,TEXT("prior session selects retained job")))return false;
    const auto Job=Jobs[Selected]->AsObject();Original=S47J::Write(S47J::Obj(Job,TEXT("captureOriginal")));Control=S47J::Write(S47J::Obj(Job,TEXT("captureControl")));
    M->UI->Close(true);F.Begin(Saved);J=F.CurrentJob();if(F.Director)F.Director->Stop();
    if(!Check(J&&J->Source==1&&J->Day==1&&J->Slot==1&&J->CaptureOriginal.Playable()&&J->CaptureControl.Playable()&&S47J::Write(J->CaptureOriginal.Save())==Original&&S47J::Write(J->CaptureControl.Save())==Control,TEXT("actual completed original and control load unchanged, without fabricated capture")))return false;
    Document=J->OriginalPage;Ticks=J->TraceTicks;
    if(!Check(F.AudioDesk&&F.AudioDesk->Ready(),TEXT("restored physical reel deck exists")))return false;
    const auto T=F.AudioDesk->InteractionPoint(TEXT("ff:audio:original"));M->Player->Place(T.X,T.Z-1.2,0);Go(100);return true;
   }
   if(!J)return Finish(false,TEXT("restored job lost"));
   if(F.AudioPreparing()||S47J::Write(J->CaptureOriginal.Save())!=Original||S47J::Write(J->CaptureControl.Save())!=Control||J->OriginalPage!=Document||J->TraceTicks!=Ticks)
    return Finish(false,TEXT("NoSound entered captured loading or changed retained evidence"));
   if(Stage==100)
   {
    if(Now-At<.4)return true;
    if(!Check(F.AudioDesk->ListeningPosition(),TEXT("explicit placement reaches actual deck listening area")))return false;
    F.OpenAudio();F.AudioAction(TEXT("original"));F.SeekAudio(7.25);
    if(!Check(F.AudioAction(TEXT("play"))&&F.AudioPlaying()&&!F.AudioPreparing()&&F.AudioDisplay().Contains(TEXT("SOUND DISABLED")),TEXT("retained original uses immediate explicit sound-disabled compatible transport")))return false;
    Go(101);return true;
   }
   if(Stage==101)
   {
    if(!F.AudioPlaying()&&F.AudioPosition()<8.25)return Finish(false,TEXT("NoSound seek playback paused unexpectedly"));
    if(F.AudioPosition()<8.25)return true;F.AudioAction(TEXT("pause"));PausedCursor=F.AudioPosition();Go(102);return true;
   }
   if(Stage==102)
   {
    if(Now-At<.3)return true;
    if(!Check(!F.AudioPlaying()&&F.AudioPosition()==PausedCursor,TEXT("NoSound pause holds the actual transport cursor")))return false;
    F.AudioAction(TEXT("original"));if(!Check(F.AudioAction(TEXT("play"))&&F.AudioPlaying()&&!F.AudioPreparing(),TEXT("original restarts at zero without captured preparation")))return false;Go(103);return true;
   }
   if(Stage==103||Stage==104)
   {
    const double End=Stage==103?18.:8.;if(!F.AudioPlaying()&&F.AudioPosition()<End)return Finish(false,TEXT("NoSound full playback paused unexpectedly"));
    if(F.AudioPosition()<End)return true;
    if(!Check(!F.AudioPlaying()&&F.AudioPosition()==End,TEXT("compatible transport reaches its complete retained duration")))return false;
    if(Stage==103){F.AudioAction(TEXT("control"));if(!Check(F.AudioAction(TEXT("play"))&&F.AudioPlaying()&&!F.AudioPreparing(),TEXT("control starts without captured preparation")))return false;Go(104);return true;}
    const auto State=F.CaptureSoundState();if(!Check(S47J::Num(State,TEXT("tapeQueued"))==0&&S47J::Num(State,TEXT("tapeRendered"))==0&&S47J::Write(J->CaptureOriginal.Save())==Original&&S47J::Write(J->CaptureControl.Save())==Control,TEXT("silent transport never queues captured PCM and retains actual source recipes")))return false;
    IFileManager::Get().MakeDirectory(*Root,true);FFileHelper::SaveStringToFile(S47J::Write(F.Save(),true),*(Root/TEXT("session.json")));
    return Finish(true,TEXT("actual captured Session restored under NoSound; seek, pause, full original/control compatible transport and unchanged recipes"));
   }
   return true;
  }
  bool Tick(float)
  {
   if(Done)return false;const double Now=FPlatformTime::Seconds();if(Now-Began>(RestorePath.IsEmpty()?180:60))return Finish(false,TEXT("bounded native capture/transport timeout; no progress or phase bypass"));
   auto* M=Main.Get();if(!M||!M->Flow||!M->Freeform||!M->Freeform->Active()||M->Flow->bStarting||S47Startup::Blocking()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->Inter||!M->UI||!M->Audio||(RestorePath.IsEmpty()&&!M->Audio->Ctx)||!M->Story)return true;
   if(!RestorePath.IsEmpty())return TickRestore(M,Now);
   auto& F=*M->Freeform;const auto* J=F.CurrentJob();
   if(Stage==0)
   {
    if(Now-Began<4)return true;
    FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);Root=FPaths::ProjectSavedDir()/TEXT("CaptureSoundTest");
    if(!Check(User.StartsWith(TEXT("/tmp/"))&&M->Flow->bDevRun&&M->Flow->CaseId==0&&!FParse::Param(FCommandLine::Get(),TEXT("NoSound")),TEXT("isolated case0 with enabled audio and ordinary default capture policy")))return false;
    M->UI->Close(true);if(F.Director)F.Director->Stop();if(!Check(F.SelectJob(1),TEXT("select existing first-day pulsar card")))return false;F.SetAnomaly(true);J=F.CurrentJob();
    if(!Check(J&&J->Source==1&&J->Day==1&&J->Slot==1,TEXT("existing actual source1/18s job, no job creation or elapsed assignment")))return false;
    const auto* P=F.ConsoleProfile();if(!Check(P!=nullptr,TEXT("actual job profile available")))return false;
    M->Story->Rx.Frequency=P->F;M->Story->Rx.Gain=P->Gain[0];M->Story->Rx.Bandwidth=P->Bw[0];M->Player->Place(0,-2.15,0);
    if(!Check(F.PointSelected(),TEXT("production pointing command accepted")))return false;F.OpenReceiver();Go(1);return true;
   }
   if(!J)return Finish(false,TEXT("actual selected job lost"));
   if(Stage==1){if(F.StartAcquisition()){Go(2);}return true;}
   if(Stage==2)
   {
    if(!J->CaptureOriginal.Failure.IsEmpty())return Finish(false,J->CaptureOriginal.Failure);
    if(J->Elapsed>=6&&!ChangedDials){const auto* P=F.ConsoleProfile();M->Story->Rx.Gain=P->Gain[1];M->Story->Rx.Bandwidth=P->Bw[1];ChangedDials=true;}
    // GAMEPLAY-LOOP-01: the unusual take is kept as it ends (Recorded, no chart); an older chart step would wait at the printer
    if(J->Stage!=FS47FreeformSession::EJobStage::Recorded&&(J->Stage!=FS47FreeformSession::EJobStage::Chart||F.PrinterProgress()<1))return true;
    if(!Check(J->CaptureOriginal.Playable()&&ChangedDials&&J->CaptureOriginal.Frames[0].Gain!=J->CaptureOriginal.Frames.Last().Gain,TEXT("actual capture retains real mid-recording dial change")))return false;
    const auto State=F.CaptureSoundState();if(!J->CaptureOriginal.bMonitorOnly&&S47J::Num(State,TEXT("liveRendered"))<18*M->Audio->Ctx->SampleRate)return true;
    Evidence.Add(MakeShared<FJsonValueObject>(State));Original=S47J::Write(J->CaptureOriginal.Save());Ticks=J->TraceTicks;
    M->UI->Close(true);const auto* It=M->Inter->Get(TEXT("printer"));const FVector T=It&&It->Object?It->Object.WorldBox().GetCenter():S47Three::ToThree(M->Web->PropLocation(TEXT("printer")));
    M->Player->Place(T.X,T.Z+1,0);const FVector D=T-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());Go(3);return true;
   }
   if(Stage==3)
   {
    if(Now-At<.4)return true;
    if(!Check(M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE())==TEXT("printer")&&bool(M->Player->OnUse),TEXT("actual printer selectable by physical eye ray")))return false;
    M->Player->OnUse(TEXT("printer"));M->UI->Close(true);J=F.CurrentJob();
    if(!Check(J->Stage==FS47FreeformSession::EJobStage::Recorded&&!J->OriginalPage.IsEmpty(),TEXT("actual printed original retained")))return false;Document=J->OriginalPage;
    const auto* P=F.ConsoleProfile();M->Story->Rx.Gain=P->Gain[0];M->Story->Rx.Bandwidth=P->Bw[0];M->Player->Place(0,-2.15,0);F.OpenReceiver();
    if(!Check(F.StartControl(),TEXT("production eight-second internal reference begins with own settings")))return false;Go(4);return true;
   }
   if(Stage==4)
   {
    if(!J->CaptureControl.Failure.IsEmpty())return Finish(false,J->CaptureControl.Failure);
    if(J->ControlAt<8||S47J::Num(F.CaptureSoundState(),TEXT("liveRendered"))<8*M->Audio->Ctx->SampleRate)return true;
    if(!Check(J->CaptureControl.Playable()&&J->CaptureControl.Control&&J->CaptureControl.Ticks.IsEmpty()&&S47J::Write(J->CaptureOriginal.Save())==Original&&J->TraceTicks==Ticks&&J->OriginalPage==Document,TEXT("actual reference fully rendered independently; original and evidence ticks unchanged")))return false;
    Evidence.Add(MakeShared<FJsonValueObject>(F.CaptureSoundState()));Control=S47J::Write(J->CaptureControl.Save());
    M->UI->Close(true);if(!Check(F.AudioDesk&&F.AudioDesk->Ready(),TEXT("physical audio comparator exists")))return false;const auto T=F.AudioDesk->InteractionPoint(TEXT("ff:audio:original"));M->Player->Place(T.X,T.Z-1.2,0);Go(5);return true;
   }
   if(Stage==5)
   {
    if(Now-At<.3)return true;if(!Check(F.AudioDesk->ListeningPosition(),TEXT("actual player stands within powered reel-deck listening area")))return false;F.OpenAudio();F.AudioAction(TEXT("original"));F.SeekAudio(7.25);
    if(!Check(F.AudioAction(TEXT("play"))&&F.AudioPreparing(),TEXT("explicit PLAY requests tape preparation")))return false;
    if(!Check(F.AudioDesk->Use(TEXT("ff:audio:play"))&&!F.AudioPreparing()&&!F.AudioPlaying()&&F.AudioPosition()==7.25&&J->OriginalCovered==0,TEXT("physical PLAY STOP callback cancels preparation without moving cursor or coverage")))return false;
    if(!Check(F.AudioAction(TEXT("play")),TEXT("actual retained transport seeks and plays original")))return false;Go(6);return true;
   }
   if(Stage==6)
   {
    if(F.AudioPreparing()){if(!(F.AudioPosition()==7.25&&J->OriginalCovered==0)){Check(false,TEXT("tape readiness advanced cursor or unheard coverage"));return false;}LoadingSeen=Stage;return true;}
    if(LoadingChecked!=Stage){if(!Check(LoadingSeen==Stage,TEXT("observed tape readiness held cursor and unheard coverage throughout preparation")))return false;LoadingChecked=Stage;}
    if(!F.AudioPlaying()&&!F.AudioPreparing()&&F.AudioPosition()<8.25)return Finish(false,TEXT("seek playback stopped early: ")+F.FeedbackText()+TEXT(" / ")+F.AudioDisplay());
    if(F.AudioPosition()<8.25)return true;F.AudioAction(TEXT("pause"));
    if(!Check(J->OriginalCovered==0,TEXT("seeking across unheard beginning does not grant listening coverage")))return false;
    F.AudioAction(TEXT("control"));if(!Check(F.AudioAction(TEXT("play")),TEXT("actual independent control playback starts")))return false;Go(7);return true;
   }
   if(Stage==7)
   {
    if(F.AudioPreparing()){if(!(F.AudioPosition()==0&&J->ControlCovered==0)){Check(false,TEXT("tape readiness advanced cursor or unheard coverage"));return false;}LoadingSeen=Stage;return true;}
    if(LoadingChecked!=Stage){if(!Check(LoadingSeen==Stage,TEXT("observed tape readiness held cursor and unheard coverage throughout preparation")))return false;LoadingChecked=Stage;}
    if(!F.AudioPlaying()&&!F.AudioPreparing()&&F.AudioPosition()<8)return Finish(false,TEXT("control playback stopped early: ")+F.FeedbackText()+TEXT(" / ")+F.AudioDisplay());
    if(F.AudioPosition()<8||S47J::Num(F.CaptureSoundState(),TEXT("tapeRendered"))<8*M->Audio->Ctx->SampleRate)return true;
    if(!Check(J->ControlCovered>=8,TEXT("complete actual control listening retains existing coverage rule")))return false;
    Evidence.Add(MakeShared<FJsonValueObject>(F.CaptureSoundState()));F.AudioAction(TEXT("original"));if(!Check(F.AudioAction(TEXT("play")),TEXT("original playback restarts at zero after prior seek")))return false;Go(8);return true;
   }
   if(Stage==8)
   {
    if(F.AudioPreparing()){if(!(F.AudioPosition()==0&&J->OriginalCovered==0)){Check(false,TEXT("tape readiness advanced cursor or unheard coverage"));return false;}LoadingSeen=Stage;return true;}
    if(LoadingChecked!=Stage){if(!Check(LoadingSeen==Stage,TEXT("observed tape readiness held cursor and unheard coverage throughout preparation")))return false;LoadingChecked=Stage;}
    if(!F.AudioPlaying()&&!F.AudioPreparing()&&F.AudioPosition()<18)return Finish(false,TEXT("original playback stopped early: ")+F.FeedbackText()+TEXT(" / ")+F.AudioDisplay());
    if(F.AudioPosition()<18||S47J::Num(F.CaptureSoundState(),TEXT("tapeRendered"))<18*M->Audio->Ctx->SampleRate)return true;
    if(!Check(J->OriginalCovered>=18&&J->ControlCovered>=8&&J->TraceTicks==Ticks&&J->OriginalPage==Document,TEXT("full actual playback keeps all evidence positions and original bytes")))return false;
    Evidence.Add(MakeShared<FJsonValueObject>(F.CaptureSoundState()));auto Saved=S47J::Read(S47J::Write(F.Save()));F.Begin(Saved);J=F.CurrentJob();
    if(!Check(J&&J->CaptureOriginal.Playable()&&J->CaptureControl.Playable()&&S47J::Write(J->CaptureOriginal.Save())==Original&&S47J::Write(J->CaptureControl.Save())==Control&&J->OriginalCovered>=18&&J->ControlCovered>=8&&J->TraceTicks==Ticks&&J->OriginalPage==Document,TEXT("explicit Session JSON restore preserves both recipes, original, ticks and listening coverage")))return false;
    IFileManager::Get().MakeDirectory(*Root,true);FFileHelper::SaveStringToFile(S47J::Write(Saved,true),*(Root/TEXT("session.json")));
    return Finish(true,TEXT("actual first pulsar capture/control and source rendering; seek, pause, full listening and explicit JSON restore"));
   }
   return true;
  }
 };
}
void S47CaptureSoundTest::Start(US47Main* M)
{
 FString RestorePath;FParse::Value(FCommandLine::Get(),TEXT("S47CaptureSoundRestore="),RestorePath);
 if(RestorePath.IsEmpty()&&!FParse::Param(FCommandLine::Get(),TEXT("S47CaptureSoundTest")))return;static bool Started=false;if(Started)return;Started=true;
 auto R=MakeShared<FRun>();R->Main=M;R->RestorePath=RestorePath;FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float Dt){return R->Tick(Dt);}));
}
