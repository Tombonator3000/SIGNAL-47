#include "S47TripoCreature.h"
#include "S47Three.h"
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
#include "S47CreatureCapture.h"

void S47CreaturePreview::Start(US47Main* M)
{
 if (!M || !FParse::Param(FCommandLine::Get(),TEXT("S47CreaturePreview"))) return;
 int32 Case=-1; FString UserDir;
 const bool HasCase=FParse::Value(FCommandLine::Get(),TEXT("S47Case="),Case);
 FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);
 FPaths::NormalizeDirectoryName(UserDir);
 if (!HasCase || Case!=0 || !UserDir.StartsWith(TEXT("/tmp/")))
 { UE_LOG(LogTemp,Error,TEXT("S47: creature preview refused: explicit Case0 and /tmp UserDir required")); return; }
 struct FPreview { FS47PaleBody Body; bool bReported=false; double StageError=0; int32 Style=0; double X=7885,Z=522,Yaw=PI; double T=0; bool bPlaced=false; int32 Last=-1, Contacts=0; double MaxPlantError=0, MaxPlantAngle=0, MaxSoleProbeError=0, NextLog=0; };
 auto Fixture=MakeShared<FPreview>(); TWeakObjectPtr<US47Main> Weak=M;
 FString Style;FParse::Value(FCommandLine::Get(),TEXT("S47CreatureStyle="),Style);
 Fixture->Style=Style==TEXT("crab")?2:Style==TEXT("tall")?1:0;
 if(Fixture->Style==1)Fixture->Body.Style=ES47PaleStyle::Tall;
 auto Capture=FS47CreatureCapture::Create([Fixture]
 { return FS47CreatureCapture::FSample{Fixture->T,Fixture->Last,Fixture->bPlaced,Fixture->Body.Rendered(),Fixture->Body.Creature.Mesh}; });
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Fixture,Capture](float Dt)
 {
  FS47CreatureCapture::FFrameScope CaptureFrame{Capture.Get()}; // observes the actual pose on every return
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
  const double T=Fixture->T;
  if(Fixture->Style!=0)
  {
   // Tall and crab: stand or fold, wait, walk ahead, step sideways, run, turn on the spot and stop.
   const bool bCrab=Fixture->Style==2;
   const double Sd=FMath::Min(double(Dt),.1);
   const int32 St=T<3?0:T<5?1:T<9?2:T<12?3:T<14?4:T<16?5:6;
   if(T>=3)
   {
    const double Ahead=St==2?(bCrab?1.1:1.0):St==4?(bCrab?2.8:1.9):0,Across=St==3&&bCrab?.8:0;
    Fixture->X+=FMath::Cos(Fixture->Yaw)*Ahead*Sd-FMath::Sin(Fixture->Yaw)*Across*Sd;
    Fixture->Z+=FMath::Sin(Fixture->Yaw)*Ahead*Sd+FMath::Cos(Fixture->Yaw)*Across*Sd;
    if(St==5)Fixture->Yaw+=.65*Sd;
   }
   const FVector At(Fixture->X,S47RoadTerrain::SurfaceY(Fixture->X-8000,Fixture->Z),Fixture->Z);
   if(bCrab)
   {
    Fixture->Body.ApplyCrab(true,At,Fixture->Yaw,Fixture->Yaw+.3*FMath::Sin(T*.7),T,T/3.,St==4);
    Fixture->MaxPlantError=FMath::Max(Fixture->MaxPlantError,Fixture->Body.Creature.CrabContacts.MaxPlantErrorCm);
    for(bool bStep:Fixture->Body.Creature.CrabContacts.bStep)Fixture->Contacts+=int32(bStep);
   }
   else
   {
    Fixture->Body.Apply(true,At,Fixture->Yaw,Fixture->Yaw+.25*FMath::Sin(T*.7),St==2?.85:St==4?1.4:0.,T*3,T,St==4?FS47PaleBody::EPose::Chase:FS47PaleBody::EPose::Walk);
    const auto& Plant=Fixture->Body.Creature.Contacts;
    Fixture->Contacts+=int32(Plant.bLeft)+int32(Plant.bRight);
    Fixture->MaxPlantError=FMath::Max(Fixture->MaxPlantError,Plant.MaxPlantErrorCm);
   }
   Fixture->StageError=FMath::Max(Fixture->StageError,bCrab?Fixture->Body.Creature.CrabContacts.MaxPlantErrorCm:Fixture->Body.Creature.Contacts.MaxPlantErrorCm);
   // The camera holds a quarter view from ahead, at the player's height.
   const double Cam=Fixture->Yaw+.7,Dist=bCrab?3.4:4.6;
   const FVector Eye(At.X+FMath::Cos(Cam)*Dist,At.Y+1.62,At.Z+FMath::Sin(Cam)*Dist),To(At.X,At.Y+(bCrab?.5:1.2),At.Z);
   const FVector D=To-Eye;
   Main->Player->Place(Eye.X,Eye.Z,FMath::Atan2(-D.X,-D.Z));Main->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
   if(St!=Fixture->Last){UE_LOG(LogTemp,Display,TEXT("S47: creature preview %s stage %d done, worst planted %.2f cm"),bCrab?TEXT("crab"):TEXT("tall"),Fixture->Last,Fixture->StageError);Fixture->StageError=0;Fixture->Last=St;}
   if(T>=17.4&&!Fixture->bReported)
   {
    Fixture->bReported=true;
    UE_LOG(LogTemp,Display,TEXT("S47: creature preview COMPLETE %s: %d steps, worst planted error %.3f cm, skin rendered %d"),bCrab?TEXT("crab"):TEXT("tall"),Fixture->Contacts,Fixture->MaxPlantError,Fixture->Body.Rendered());
   }
   return true;
  }
  const int32 Stage=T<2?0:T<4?1:T<8?2:T<11?3:T<14?4:T<15.2?5:6;
  const double Travel=FMath::Clamp(T-4.,0.,4.)*1.15+FMath::Clamp(T-8.,0.,3.)*2.1;
  const FVector Feet(7885-Travel,S47RoadTerrain::SurfaceY(-115-Travel,522),522);
  const double Yaw=PI+(Stage==4?FMath::Clamp((T-11.)/2.,0.,1.)*.8:Stage>4?.8:0.);
  const auto Pose=Stage==1?FS47PaleBody::EPose::Listen:Stage==3?FS47PaleBody::EPose::Chase:Stage==5?FS47PaleBody::EPose::Grab:FS47PaleBody::EPose::Walk;
  const FVector Contact=Feet+FVector(FMath::Cos(Yaw)*.9,Main->Player->Eye*.85,FMath::Sin(Yaw)*.9);
  Fixture->Body.Apply(true,Feet,Yaw,Yaw+.25*FMath::Sin(T),Stage==2?.85:Stage==3?1.4:0.,T*3,T,Pose,
   Stage==5?(T-14.)/.7:-1,TOptional<FVector>(S47Three::ToUE(Contact)));
  const auto& Plant=Fixture->Body.Creature.Contacts;
  Fixture->Contacts+=int32(Plant.bLeft)+int32(Plant.bRight);
  Fixture->MaxPlantError=FMath::Max(Fixture->MaxPlantError,Plant.MaxPlantErrorCm);
  Fixture->MaxPlantAngle=FMath::Max(Fixture->MaxPlantAngle,Plant.MaxPlantAngleDeg);
  Fixture->MaxSoleProbeError=FMath::Max(Fixture->MaxSoleProbeError,Plant.MaxSoleProbeErrorCm);
  if (T>=Fixture->NextLog)
  { Fixture->NextLog=T+.5; UE_LOG(LogTemp,Display,TEXT("S47: creature preview sample t=%.3f stage=%d travel=%.3f m plantedError=%.4f cm maxError=%.4f cm angle=%.6f deg maxAngle=%.6f deg soleProbe=%.4f cm maxSoleProbe=%.4f cm footYaw=(%.3f,%.3f) contacts=%d valid=%d rendered=%d"),T,Stage,Travel,Plant.MaxPlantErrorCm,Fixture->MaxPlantError,Plant.MaxPlantAngleDeg,Fixture->MaxPlantAngle,Plant.MaxSoleProbeErrorCm,Fixture->MaxSoleProbeError,Plant.RotationUE[0].Rotator().Yaw,Plant.RotationUE[1].Rotator().Yaw,Fixture->Contacts,Plant.bValid?1:0,Fixture->Body.Rendered()?1:0); }
  // Explicit model fixture camera follows the ordinary supported root; never claims AI/player capture.
  Main->Player->Place(Feet.X-3.5,Feet.Z,-PI/2);Main->Player->Pitch=-.06;
  if (Stage!=Fixture->Last)
  { Fixture->Last=Stage; UE_LOG(LogTemp,Display,TEXT("S47: creature preview stage %d (idle/listen/walk/run/turn/grab/release), skin rendered %d, actual travel %.2f m; model-only fixture"),Stage,Fixture->Body.Rendered()?1:0,Travel); }
  if (T>=17)
  { UE_LOG(LogTemp,Display,TEXT("S47: creature preview COMPLETE: skin rendered %d; original rig, actual travel and model-only phases; no AI/contact-clearance claim"),Fixture->Body.Rendered()?1:0); return false; }
  return true;
 }));
}
