#include "S47TripoCreature.h"
#include "S47PaleBody.h"
#include "S47Main.h"
#include "S47World.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47Story.h"
#include "S47RoadTerrain.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Containers/Ticker.h"

void S47CreaturePreview::Start(US47Main* M)
{
 if (!M || !FParse::Param(FCommandLine::Get(),TEXT("S47CreaturePreview"))) return;
 int32 Case=-1; FString UserDir;
 const bool HasCase=FParse::Value(FCommandLine::Get(),TEXT("S47Case="),Case);
 FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);
 FPaths::NormalizeDirectoryName(UserDir);
 if (!HasCase || Case!=0 || !UserDir.StartsWith(TEXT("/tmp/")))
 { UE_LOG(LogTemp,Error,TEXT("S47: creature preview refused: explicit Case0 and /tmp UserDir required")); return; }
 struct FPreview { FS47PaleBody Body; double T=0; bool bPlaced=false; int32 Last=-1; };
 auto Fixture=MakeShared<FPreview>(); TWeakObjectPtr<US47Main> Weak=M;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Fixture](float Dt)
 {
  US47Main* Main=Weak.Get(); if (!Main) return false;
  if (!Main->Web || !Main->Web->IsLoaded() || !Main->World || !Main->Player || Main->Mode!=US47Main::EMode::Play) return true;
  if (!Fixture->bPlaced)
  {
   Main->World->Enter(TEXT("diner"));
   if (Main->Story) Main->Story->SetClock(12*3600);
   Main->Player->Place(7881.5,522,-PI/2); Main->Player->Pitch=-.06;
   if (!Fixture->Body.Build(Main->Web.Get(),TEXT("#root"),TEXT("TripoCreaturePreview")) || !Fixture->Body.Creature.Ready())
   { UE_LOG(LogTemp,Error,TEXT("S47: creature preview missing actual Tripo skeletal asset")); return false; }
   Fixture->bPlaced=true;
   UE_LOG(LogTemp,Display,TEXT("S47: creature preview FIXTURE: daylight Caliche placement, actual Tripo skin, no AI or playthrough claim"));
  }
  Fixture->T+=FMath::Min(double(Dt),.1);
  const int32 Stage=Fixture->T<3?0:Fixture->T<5?1:2;
  const auto Pose=Stage==0?FS47PaleBody::EPose::Walk:Stage==1?FS47PaleBody::EPose::Listen:FS47PaleBody::EPose::Grab;
  Fixture->Body.Apply(true,FVector(7885,S47RoadTerrain::SurfaceY(-115,522),522),PI,PI+.35*FMath::Sin(Fixture->T),
   Stage==0?.85:0.,Fixture->T*3,Fixture->T,Pose);
  if (Stage!=Fixture->Last)
  { Fixture->Last=Stage; UE_LOG(LogTemp,Display,TEXT("S47: creature preview pose %d, actual skin rendered %d"),Stage,Fixture->Body.Rendered()?1:0); }
  if (Fixture->T>=8)
  { UE_LOG(LogTemp,Display,TEXT("S47: creature preview COMPLETE: skin rendered %d, original rig animated; model fixture only"),Fixture->Body.Rendered()?1:0); return false; }
  return true;
 }));
}
