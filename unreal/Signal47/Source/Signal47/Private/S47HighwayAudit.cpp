// Focused, opt-in packaged checks. No save writes and no hardware input takeover.
#include "S47Web.h"
#include "S47Audio.h"
#include "S47EngineSound.h"
#include "S47Flow.h"
#include "S47HighwayPower.h"
#include "S47Flora.h"
#include "S47WorldLife.h"
#include "S47Json.h"
#include "S47Legs.h"
#include "S47Main.h"
#include "S47RoadArea.h"
#include "S47Startup.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47World.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47HighwayAudit, Log, All);

namespace
{
 S47J::FObj Car(const FString& Leg, const FVector& Pos, double Heading)
 {
  auto O=S47J::New(); S47J::Set(O,TEXT("leg"),Leg); S47J::Set(O,TEXT("x"),Pos.X);
  S47J::Set(O,TEXT("y"),Pos.Y); S47J::Set(O,TEXT("z"),Pos.Z); S47J::Set(O,TEXT("h"),Heading); return O;
 }
 // Replay actual decoded PCM through the loop's measured gain/rate/filter in an isolated graph.
 // This is a numerical loop check, not a claim about the complete audible mix or a listening test.
 double LoopRms(const TSharedPtr<FS47AudioBuffer>& Buffer, const FS47LoopState& State)
 {
  const auto C=FS47AudioContext::Make(48000.f);
  const auto S=C->CreateBufferSource(); S->Buffer=Buffer; S->bLoop=true; S->PlaybackRate.SetValue(State.Rate);
  const auto G=C->CreateGain(); G->Gain.SetValue(State.Gain);
  if(State.LowPass>0) { auto F=C->CreateBiquadFilter(); F->SetType(TEXT("lowpass")); F->Frequency.SetValue(State.LowPass); S->Connect(F); F->Connect(G); }
  else S->Connect(G);
  G->Connect(C->Destination); S->Start();
  TArray<float> PCM; PCM.SetNumZeroed(4800*2); C->Render(PCM.GetData(),4800,2);
  double Sum=0; for(float V:PCM) { if(!FMath::IsFinite(V)) { C->Close(); return -1; } Sum+=double(V)*V; }
  C->Close(); return FMath::Sqrt(Sum/PCM.Num());
 }
 bool AudioCheck(US47World* World, US47Audio* A, const TCHAR* Label)
 {
  const FS47EngineSound* E=World->EngineSound();
  if(!A || !E || !E->On() || !E->LoopsReady() || E->Stopping()) return false;
  double Energy=0;
  for(const auto& Pair : { TPair<FString,FString>(TEXT("truck:idle"),TEXT("engineIdle")), TPair<FString,FString>(TEXT("truck:drive"),TEXT("engineDrive")) })
  {
   const auto State=A->InspectLoop(Pair.Key); const auto Buffer=A->Buf.FindRef(Pair.Value);
   if(!State.IsSet() || !State->bPlaying || !State->bLoop || !Buffer.IsValid() || State->Buffer.Get()!=Buffer.Get()
    || State->LowPass<=0 || State->LowPass>1100.01 || !FMath::IsFinite(State->Gain) || State->Gain<0 || !FMath::IsFinite(State->Rate) || State->Rate<=0) return false;
   const double Rms=LoopRms(Buffer,*State);
   UE_LOG(LogS47HighwayAudit,Display,TEXT("S47: highway audio %s %s actual_buffer=1 gain=%.8f rate=%.5f lowpass=%.2f isolated_loop_pcm_rms=%.9f"),Label,*Pair.Key,State->Gain,State->Rate,State->LowPass,Rms);
   if(Rms<0 || !FMath::IsFinite(Rms)) return false;
   Energy+=Rms;
  }
  return Energy>1e-7 && A->Buf.FindRef(TEXT("engineStart")).IsValid() && A->Buf.FindRef(TEXT("engineOff")).IsValid();
 }
}

void AS47Web::BeginHighwayAudit()
{
 FString Mode; if(!FParse::Value(FCommandLine::Get(),TEXT("S47HighwayAudit="),Mode)) return;
 struct FState
 {
  bool Done=false,HaveEye=false,NorthBefore=false,SouthBefore=false;
  int32 Stage=0,Changes=0; double WallStart=0,At=0,MaxJump=0;
  FVector LastEye=FVector::ZeroVector; FString LastLeg,Shots; S47J::FObj Reentry;
 };
 const auto P=MakeShared<FState>(); FParse::Value(FCommandLine::Get(),TEXT("S47HighwayShots="),P->Shots);
 TWeakObjectPtr<AS47Web> Self=this; FTimerHandle Timer;
 GetWorldTimerManager().SetTimer(Timer,[Self,P,Mode] {
  if(P->Done) return;
  AS47Web* W=Self.Get(); US47Main* M=W?US47Main::Get(W):nullptr;
  const double Now=FPlatformTime::Seconds();
  auto Finish=[&](bool Pass,const FString& Why) {
   P->Done=true;
   if(M && M->World) M->World->TestInput=nullptr;
   if(M && M->Player) { M->Player->TestMove.Reset(); M->Player->bTestRun=false; }
   UE_LOG(LogS47HighwayAudit,Display,TEXT("S47: highway audit %s %s: %s; elapsed=%.2f transitions=%d max_canonical_eye_step=%.4f"),*Mode,Pass?TEXT("PASS"):TEXT("FAIL"),*Why,Now-P->WallStart,P->Changes,P->MaxJump);
  };
  if(P->WallStart>0 && Now-P->WallStart>58) { Finish(false,TEXT("bounded ready-state drive/audio timeout")); return; }
  if(!W || !M || !M->World || !M->Player || !M->Flow || S47Startup::Blocking() || M->Flow->bStarting || M->Mode!=US47Main::EMode::Play) return;
  US47World* World=M->World;
  auto Shot=[&](const TCHAR* Name) {
   if(P->Shots.IsEmpty()) return true;
   IFileManager::Get().MakeDirectory(*P->Shots,true);
   const FString File=P->Shots/(FString(Name)+TEXT(".png"));
   if(IFileManager::Get().FileExists(*File)) return false;
   FScreenshotRequest::RequestScreenshot(File,true,false); return true;
  };
  auto Landscape=[&]() {
   TArray<FString> Lines; bool Ok=W->AuditHighwayLandscape(Lines);
   if(M->Life) for(const auto& Flora:M->Life->Flora) if(Flora && Flora->Shown()) Flora->AuditDrawCounts(Lines);
   for(const FString& Line:Lines) UE_LOG(LogS47HighwayAudit,Display,TEXT("S47: audit %s"),*Line);
   int32 Families=0,Instances=0;
   for(int32 I:W->HighwaySharedNodes) if(const FS47WebNode* N=W->Node(I)) if(const auto* C=Cast<UInstancedStaticMeshComponent>(N->Comp.Get())) {
    if(N->Name.StartsWith(TEXT("highway:overlap:"))) { ++Families; Instances+=C->GetInstanceCount(); Ok&=C->IsVisible() && C->GetInstanceCount()>0; }
   }
   const auto Power=S47HighwayPower::Status(W);
   UE_LOG(LogS47HighwayAudit,Display,TEXT("S47: highway audit actual overlap families=%d instances=%d power_ready=%d main=%d spur=%d wires=%d area=%s leg=%s"),Families,Instances,Power.bReady,Power.MainPoles,Power.SpurPoles,Power.WireSegments,*World->Area,*World->Leg);
   if(World->Leg==TEXT("road")) {
    const FS47WebNode* Group=nullptr; int32 Draws=0;
    for(int32 I=0;I<3;++I) if(const FS47WebNode* G=W->Node(W->Handle(FString::Printf(TEXT("world.flora.patches[%d].group"),I)))) if(G->Name==TEXT("flora:road")) Group=G;
    if(Group) for(int32 Ch:Group->Children) if(const FS47WebNode* N=W->Node(Ch)) {
     if(N->Name!=TEXT("flora-grass+sage") && N->Name!=TEXT("flora-creosote+stalks") && N->Name!=TEXT("flora-succulent") && N->Name!=TEXT("flora-stones") && N->Name!=TEXT("flora-stain")) continue;
     ++Draws; const auto* C=Cast<UInstancedStaticMeshComponent>(N->Comp.Get());
     bool Visible=C && C->IsRegistered() && C->IsVisible();
     for(const USceneComponent* Parent=C?C->GetAttachParent():nullptr;Parent;Parent=Parent->GetAttachParent()) Visible&=Parent->IsVisible();
     const int32 Count=C?C->GetInstanceCount():0;
     UE_LOG(LogS47HighwayAudit,Display,TEXT("S47: highway road flora %s node_enabled=%d rendered_visible=%d actual_instances=%d"),*N->Name,N->bVis,Visible,Count);
     Ok&=N->bVis && Visible && Count>0;
    }
    Ok&=Draws==5;
   }
   return Ok && Families>0 && Instances>100 && Power.bReady;
  };
  if(P->Stage==0) {
   // An opening commute may still own its delayed StartTrip callback.
   // Wait for it before replacing the isolated fixture's starting position.
   if(World->bBusy) return;
   P->WallStart=Now;
   FString UserDir;
   if((Mode!=TEXT("seam") && Mode!=TEXT("barrier")) || !FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir) || UserDir.IsEmpty()
    || !M->Flow->bDevRun || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),FPaths::ConvertRelativePathToFull(UserDir))) {
    Finish(false,TEXT("requires seam/barrier, isolated -UserDir and unsaved developer run (no -S47Case)")); return;
   }
   World->BeginCommute(Car(TEXT("road"),FVector(8001.8,0,Mode==TEXT("seam")?S47Legs::ROAD_TO_SARO()+50:1999.95),0));
   World->Autopilot.Reset(); World->TestInput=[] { return FS47DriveIn(); };
   P->Stage=1; P->At=Now; return;
  }
  if(P->Stage==1) {
   if(!World->bDriving || World->bBusy || !World->CabDrive() || Now-P->At<3) return;
   if(World->Leg!=TEXT("road")) { Finish(false,TEXT("commute did not start on the actual road leg")); return; }
   if(!AudioCheck(World,M->Audio.Get(),TEXT("entry")) || !Landscape()) { Finish(false,TEXT("entry loop/landscape audit")); return; }
   if(Mode==TEXT("barrier")) {
    auto* Road=static_cast<FS47RoadArea*>(World->CabDrive()->Area);
    const bool WasClosed=Road->RoadClosed(); const int32 Count=Road->Obstacles.Num();
    // On foot here the canonical area is diner; road is only the active driving leg.
    const FString WasArea=World->Area; World->Enter(TEXT("diner"));
    auto Probe=[&](bool Closed) {
     Road->SetRoadClosed(Closed); FS47DriveController D(nullptr,Road); D.Place(FVector(8018,0,1999.95),-UE_DOUBLE_PI/2);
     FS47DriveIn In; In.Throttle=1; for(int32 I=0;I<300;++I) D.Update(1.0/60,In,FVector2D::ZeroVector);
     return D.Pos;
    };
    const FVector Closed=Probe(true); const bool ClosedWalk=M->Player->Walkable(8026,1999.95);
    const FVector Open=Probe(false); const bool OpenWalk=M->Player->Walkable(8026,1999.95);
    Road->SetRoadClosed(WasClosed);
    World->Enter(WasArea);
    UE_LOG(LogS47HighwayAudit,Display,TEXT("S47: highway barrier real area closedX=%.6f openX=%.6f closed_walk=%d open_walk=%d obstacle_count=%d restored=%d"),Closed.X,Open.X,ClosedWalk,OpenWalk,Count,Road->RoadClosed()==WasClosed);
    Finish(Closed.X>8022 && Closed.X<8022.6 && Open.X>8025.5 && Open.X<8027.3 && Open.X>8026.8 && FMath::Abs(Open.Z-1999.95)<.02
     && !ClosedWalk && OpenWalk && Road->Obstacles.Num()==Count && Road->RoadClosed()==WasClosed,
     TEXT("actual drive controller: closed posts stop; open passes posts and stops at permanent wall; actual walk gate and state restoration")); return;
   }
   World->TestInput=[] { FS47DriveIn In; In.Throttle=.5; return In; };
   P->Stage=2; P->At=Now; P->LastLeg=World->Leg; P->HaveEye=false; return;
  }
  FS47DriveController* D=World->CabDrive();
  if(P->Stage>=2 && P->Stage<=5 && D) {
   const FVector Eye=W->HighwayMapEye(S47Three::ToThree(M->Player->GetActorLocation()));
   if(P->HaveEye) { const double Step=FVector::Dist(Eye,P->LastEye); P->MaxJump=FMath::Max(P->MaxJump,Step); if(Step>10) { Finish(false,TEXT("canonical player eye jumped over 10m per sampled frame")); return; } }
   P->LastEye=Eye; P->HaveEye=true;
   if(World->Leg!=P->LastLeg) {
    ++P->Changes;
    UE_LOG(LogS47HighwayAudit,Display,TEXT("S47: highway actual transition %s -> %s canonical_eye=%.4f,%.4f,%.4f speed=%.4f"),*P->LastLeg,*World->Leg,Eye.X,Eye.Y,Eye.Z,D->Speed);
    P->LastLeg=World->Leg;
    if(!Landscape() || !AudioCheck(World,M->Audio.Get(),TEXT("transfer"))) { Finish(false,TEXT("actual transition landscape/audio audit")); return; }
   }
  }
  if(P->Stage==2 && D) {
   if(World->Leg==TEXT("road") && !P->NorthBefore && D->Pos.Z<S47Legs::ROAD_TO_SARO()+6) { if(!Shot(TEXT("north-before"))) { Finish(false,TEXT("screenshot destination already exists")); return; } P->NorthBefore=true; }
   if(World->Leg==TEXT("saro")) {
    if(!P->NorthBefore || !Shot(TEXT("north-after"))) { Finish(false,TEXT("north transfer capture ordering")); return; }
    P->Stage=3; return;
   }
  }
  if(P->Stage==3 && D && World->Leg==TEXT("saro")) {
   if(D->Pos.Z<S47Legs::SARO_TO_ROAD()-25) {
    World->TestInput=[World] { FS47DriveIn In; In.Throttle=World->DrivingSpeed()>.15?-1:0; return In; };
    P->Stage=4; return;
   }
  }
  if(P->Stage==4 && D && FMath::Abs(D->Speed)<.15) {
   if(!AudioCheck(World,M->Audio.Get(),TEXT("north-stop"))) { Finish(false,TEXT("idle after real north crossing")); return; }
   // Turn the stopped fixture only. Both boundary crossings themselves use ordinary driving.
   D->Place(D->Pos,UE_DOUBLE_PI);
   World->TestInput=[] { FS47DriveIn In; In.Throttle=.5; return In; };
   P->HaveEye=false; P->Stage=5; return;
  }
  if(P->Stage==5 && D) {
   if(World->Leg==TEXT("saro") && !P->SouthBefore && D->Pos.Z>S47Legs::SARO_TO_ROAD()-6) { if(!Shot(TEXT("south-before"))) { Finish(false,TEXT("screenshot destination already exists")); return; } P->SouthBefore=true; }
   if(World->Leg==TEXT("road")) {
    if(!P->SouthBefore || !Shot(TEXT("south-after"))) { Finish(false,TEXT("south transfer capture ordering")); return; }
    World->TestInput=[World] { FS47DriveIn In; In.Throttle=World->DrivingSpeed()>.15?-1:0; return In; };
    P->Stage=6; return;
   }
  }
  if(P->Stage==6 && D && FMath::Abs(D->Speed)<.15) {
   P->Reentry=Car(World->Leg,D->Pos,D->Heading);
   if(P->Changes!=2 || !World->GetOutHere()) { Finish(false,TEXT("two real transfers followed by normal stopped exit")); return; }
   P->Stage=7; P->At=Now; return;
  }
  if(P->Stage==7) {
   const FS47EngineSound* E=World->EngineSound(); if(World->bBusy || !E || E->Stopping()) return;
   if(E->On() || E->LoopsReady() || M->Audio->InspectLoop(TEXT("truck:idle")).IsSet() || M->Audio->InspectLoop(TEXT("truck:drive")).IsSet()) { Finish(false,TEXT("engine loops remain after normal exit and fade")); return; }
   World->BeginCommute(P->Reentry); World->Autopilot.Reset(); World->TestInput=[] { return FS47DriveIn(); };
   P->Stage=8; P->At=Now; return;
  }
  if(P->Stage==8 && World->bDriving && !World->bBusy && Now-P->At>3) {
   Finish(AudioCheck(World,M->Audio.Get(),TEXT("reentry")) && Landscape(),TEXT("two actual seam crossings, continuous mapped player eye, native landscape/power, decoded loop PCM, normal exit shutdown and re-entry; no full save restore claim"));
  }
 },.1f,true);
}
