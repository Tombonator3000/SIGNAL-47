// Opt-in packaged integration evidence for the actual waitress, dialogue audio and face rig.
#include "S47Person.h"
#include "S47Audio.h"
#include "S47Evening.h"
#include "S47Flow.h"
#include "S47Main.h"
#include "S47Saves.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47World.h"
#include "Animation/AnimSequence.h"
#include "AnimNodes/AnimNode_CopyPoseFromMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "GroomComponent.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "UObject/UnrealType.h"
#include "UnrealClient.h"

namespace S47PersonReview
{
 struct FReview
 {
  FString Directory, Csv = TEXT("elapsed,clip,take_seconds,weight,body_jaw,face_jaw,rig_jaw_angle,blink,head_yaw\n");
  bool Initialized=false, Started=false, Done=false, Paused=false, PauseChecked=false, Restored=false;
  double Elapsed=0, LineElapsed=0, NextSample=0, PauseAt=0, PauseClock=0, RestoredAt=0;
  int32 Samples=0, CopyNodes=0;
  float BodyMax=0, FaceMax=0, JawMin=MAX_flt, JawMax=-MAX_flt;
  TSet<FString> Clips, Shots;
 };
 static FReview S;
 static void Fail(const FString& Reason)
 {
  UE_LOG(LogTemp, Error, TEXT("S47: waitress review FAIL: %s"), *Reason);
  S.Done=true;
  FFileHelper::SaveStringToFile(Reason, *FPaths::Combine(S.Directory,TEXT("FAIL.txt")));
 }
 void Tick(US47PersonComponent* Person, float Dt)
 {
  if (!S.Initialized)
  {
   S.Initialized=true;
   if (!FParse::Value(FCommandLine::Get(),TEXT("S47WaitressReview="),S.Directory)) return;
   FString UserDir;
   if (!FParse::Param(FCommandLine::Get(),TEXT("S47SmokeTest")) || !FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir))
   { S.Directory.Empty(); return; }
   // An explicit developer run and separate UserDir are required; normal play never enters this path.
   S.Directory=FPaths::ConvertRelativePathToFull(S.Directory);
   IFileManager::Get().MakeDirectory(*S.Directory,true);
  }
  if (S.Directory.IsEmpty() || S.Done || !Person || !Person->Body || !Person->Anim) return;
  US47Main* M=US47Main::Get(Person);
  if (!M || !M->Flow || !M->Flow->bDevRun || !M->Evening || !M->Audio || !M->Audio->Ctx || !M->Player || !M->UI) return;
  S.Elapsed+=Dt; S.LineElapsed+=Dt;
  if (S.Elapsed>52) { Fail(TEXT("timed out before measured speech, face movement and restore")); return; }
  if (!S.Started)
  {
   if (S.Elapsed<4) return;
   M->UI->Close(false);
   M->Audio->bCharacterVoices=true;
   M->Evening->Begin(); M->Evening->BootstrapTest(TEXT("diner"));
   S.Started=true; S.LineElapsed=0;
   UE_LOG(LogTemp,Display,TEXT("S47: waitress review START actual recorded diner dialogue; camera fixture, not human input"));
  }
  M->Player->Fov=38; M->Player->Pitch=-0.045;
  USkeletalMeshComponent* Face=nullptr;
  for (USkeletalMeshComponent* C : TInlineComponentArray<USkeletalMeshComponent*>(Person->GetOwner()))
   if (C->GetName()==TEXT("Face")) Face=C;
  if (!Face || !Face->GetAnimInstance() || !Face->GetPostProcessInstance()) { Fail(TEXT("face animation or RigLogic postprocess missing")); return; }
  int32 CopyNodes=0;
  UAnimInstance* FaceAnim=Face->GetAnimInstance();
  for (TFieldIterator<FStructProperty> It(FaceAnim->GetClass());It;++It)
   if (It->Struct==FAnimNode_CopyPoseFromMesh::StaticStruct())
   {
    const auto* Node=It->ContainerPtrToValuePtr<FAnimNode_CopyPoseFromMesh>(FaceAnim);
    if (Node->bCopyCurves && Node->bUseAttachedParent && Node->SourceMeshComponent==Person->Body) ++CopyNodes;
   }
  if (!CopyNodes) { Fail(TEXT("face CopyPose lost its body/curve connection after initialization")); return; }
  S.CopyNodes=CopyNodes;
  FString Clip; double Seconds=0;
  const bool Speaking=M->Evening->PhysicalSpeech(Clip,Seconds);
  if (Speaking) S.Clips.Add(Clip);
  const FName Jaw(TEXT("CTRL_expressions_jawOpen"));
  const float BodyJaw=Person->Anim->GetCurveValue(Jaw), FaceJaw=FaceAnim->GetCurveValue(Jaw);
  const float RigJaw=Face->GetSocketTransform(TEXT("FACIAL_C_Jaw"),RTS_Component).GetRotation().Rotator().Pitch;
  if (Speaking)
  {
   S.BodyMax=FMath::Max(S.BodyMax,FMath::Abs(BodyJaw)); S.FaceMax=FMath::Max(S.FaceMax,FMath::Abs(FaceJaw));
   S.JawMin=FMath::Min(S.JawMin,RigJaw); S.JawMax=FMath::Max(S.JawMax,RigJaw);
   if (Seconds>.55 && !S.Shots.Contains(Clip))
   {
    S.Shots.Add(Clip); FScreenshotRequest::RequestScreenshot(FPaths::Combine(S.Directory,Clip+TEXT(".png")),false,false);
   }
   if (!S.PauseChecked && !S.Paused && S.Clips.Num()>=2 && Seconds>.6)
   {
    M->Audio->Suspend(true); S.Paused=true; S.PauseAt=S.Elapsed; S.PauseClock=M->Audio->Ctx->CurrentTime();
   }
  }
  if (S.Paused)
  {
   if (M->Audio->Ctx->CurrentTime()!=S.PauseClock) { M->Audio->Suspend(false); Fail(TEXT("audio clock advanced during suspension")); return; }
   if (S.Elapsed-S.PauseAt>=.55) { M->Audio->Suspend(false); S.Paused=false; S.PauseChecked=true; UE_LOG(LogTemp,Display,TEXT("S47: waitress review PASS speech clock suspension")); }
  }
  if (S.Elapsed>=S.NextSample)
  {
   S.NextSample=S.Elapsed+.05; ++S.Samples;
   S.Csv+=FString::Printf(TEXT("%.4f,%s,%.4f,%.4f,%.6f,%.6f,%.6f,%.4f,%.4f\n"),S.Elapsed,*Clip,Seconds,Person->Anim->Pose.TalkWeight,BodyJaw,FaceJaw,RigJaw,Person->Anim->Pose.Blink,Person->Anim->Pose.LookYaw);
  }
  const FS47EveningLine* Line=M->Evening->Pack.CurrentLine();
  if (!S.Restored && Line && !Speaking && !S.Paused && S.LineElapsed>FMath::Max(2.5,Line->Duration+1.0))
  {
   M->UI->Close(false); M->Evening->Pack.Advance(); M->Evening->ShowDialogue(); S.LineElapsed=0;
  }
  if (!S.Restored && S.Clips.Num()>=3 && !Speaking && S.LineElapsed>2)
  {
   if (S.BodyMax<.01 || S.FaceMax<.01 || S.JawMax-S.JawMin<.05 || !S.PauseChecked)
   { Fail(FString::Printf(TEXT("unproven facial movement body %.4f face %.4f joint range %.4f pause %d"),S.BodyMax,S.FaceMax,S.JawMax-S.JawMin,S.PauseChecked)); return; }
   FFileHelper::SaveStringToFile(S.Csv,*FPaths::Combine(S.Directory,TEXT("speech.csv")));
   FS47SaveStore Store(FPaths::Combine(S.Directory,TEXT("disk-store"))); Store.Init(); FS47SaveCard Card;
   Card.Chapter=TEXT("Waitress integration review"); Card.Place=TEXT("Mesa Diner"); Card.Clock=TEXT("Late afternoon");
   FString Error; const auto Meta=Store.Save(1,TEXT("manual"),0,M->Flow->GameState(),Card,Error);
   if (!Meta) { Fail(TEXT("isolated disk save failed: ")+Error); return; }
   FS47SaveStore Reader(FPaths::Combine(S.Directory,TEXT("disk-store"))); Reader.Init(); const auto Loaded=Reader.Load(Meta->Id);
   if (!Loaded) { Fail(TEXT("fresh disk store load failed")); return; }
   S.Restored=true; S.RestoredAt=S.Elapsed;
   US47Flow::FStart Restore; Restore.State=Loaded; Restore.CaseId=0; Restore.bDev=true; Restore.Playtime=M->Playtime;
   M->Flow->StartGame(Restore); return; // the current actor/component can have been rebuilt
  }
  if (S.Restored && S.Elapsed-S.RestoredAt>2)
  {
   int32 Count=0; for (TActorIterator<AActor> It(Person->GetWorld());It;++It) if (It->ActorHasTag(TEXT("S47Waitress")) && !It->IsActorBeingDestroyed()) ++Count;
   if (Count!=1) { Fail(FString::Printf(TEXT("restore produced %d waitress actors"),Count)); return; }
   FScreenshotRequest::RequestScreenshot(FPaths::Combine(S.Directory,TEXT("restored.png")),false,false);
   const FString Result=FString::Printf(TEXT("PASS: actual speech clips=%d samples=%d bodyJaw=%.6f faceJaw=%.6f rigJawRange=%.6f copyPoseNodes=%d audioPause=1 diskRestoreActors=%d\n"),S.Clips.Num(),S.Samples,S.BodyMax,S.FaceMax,S.JawMax-S.JawMin,S.CopyNodes,Count);
   FFileHelper::SaveStringToFile(Result,*FPaths::Combine(S.Directory,TEXT("PASS.txt")));
   UE_LOG(LogTemp,Display,TEXT("S47: waitress review %s"),*Result); S.Done=true;
  }
 }
}
